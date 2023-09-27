/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "AppStreamMgr.h"
#include "MyUtils.h"

#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>

#include <vector>

using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;

#define ThisNamespace   AppStreamMgr::CallbackHandler
#define OPPO_HIGH_FRAMERATE_OPMODE (0x8021)

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mDebugPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_WARN(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mWarningPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR_ULOG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mErrorPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mErrorPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_LOGV(...)                MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...)                MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...)                MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...)                MY_WARN (W, __VA_ARGS__)
#define MY_LOGE(...)                MY_ERROR_ULOG(E, __VA_ARGS__)
#define MY_LOGA(...)                MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
static const int kDumpLockRetries = 50;
static const int kDumpLockSleep = 60000;

static bool tryLock(Mutex& mutex)
{
    bool locked = false;
    for (int i = 0; i < kDumpLockRetries; ++i) {
        if (mutex.tryLock() == NO_ERROR) {
            locked = true;
            break;
        }
        usleep(kDumpLockSleep);
    }
    return locked;
}


/******************************************************************************
 *
 ******************************************************************************/
static void convertToDebugString(AppStreamMgr::CallbackParcel const& cbParcel, android::Vector<android::String8>& out)
{
/*
    requestNo:80 errors#:10 shutter:10 o:meta#:10 i:image#:10 o:image#:10
        {ERROR_DEVICE/ERROR_REQUEST/ERROR_RESULT}
        {ERROR_BUFFER streamId:123 bufferId:456}
        OUT Meta  -
            partial#:123 "xxxxxxxxxx"
        IN Image -
            streamId:01 bufferId:04 OK
        OUT Image -
            streamId:02 bufferId:04 ERROR with releaseFence
*/
    String8 log = String8::format("requestNo:%u", cbParcel.requestNo);
    if  ( ! cbParcel.vError.isEmpty() ) {
        log += String8::format(" errors#:%zu", cbParcel.vError.size());
    }
    if  ( cbParcel.shutter != 0 ) {
        log += String8::format(" shutter:%" PRId64 "", cbParcel.shutter->timestamp);
    }
    if  ( ! cbParcel.vOutputMetaItem.isEmpty() ) {
        log += String8::format(" o:meta#:%zu", cbParcel.vOutputMetaItem.size());
    }
    if  ( ! cbParcel.vOutputPhysicMetaItem.isEmpty() ) {
        log += String8::format(" o:physic meta#:%zu", cbParcel.vOutputPhysicMetaItem.size());
    }
    if  ( ! cbParcel.vInputImageItem.isEmpty() ) {
        log += String8::format(" i:image#:%zu", cbParcel.vInputImageItem.size());
    }
    if  ( ! cbParcel.vOutputImageItem.isEmpty() ) {
        log += String8::format(" o:image#:%zu", cbParcel.vOutputImageItem.size());
    }
    out.push_back(log);
    //
    for (auto const& item : cbParcel.vError) {
        out.push_back(String8::format("    {%s %s}", toString(item.errorCode).c_str(), (item.stream == 0 ? "" : item.stream->getStreamName())));
    }
    //
    if ( ! cbParcel.vOutputMetaItem.isEmpty() ) {
        out.push_back(String8("    OUT Meta  -"));
        for (auto const& item: cbParcel.vOutputMetaItem) {
            out.push_back(String8::format("        streamId:%02" PRIx64 " partial#:%u %s",
                item.buffer->getStreamInfo()->getStreamId(), item.bufferNo, item.buffer->getName()));
        }
    }
    //
    if ( ! cbParcel.vOutputPhysicMetaItem.isEmpty() ) {
        out.push_back(String8("    OUT Physical Meta  -"));
        for (auto const& item: cbParcel.vOutputPhysicMetaItem) {
            out.push_back(String8::format("        streamId:%02" PRIx64 " physical CamId#:%s %s",
                item.buffer->getStreamInfo()->getStreamId(), item.camId.c_str(), item.buffer->getName()));
        }
    }
    //
    if  ( ! cbParcel.vInputImageItem.isEmpty() ) {
        out.push_back(String8("     IN Image -"));
        for (auto const& item: cbParcel.vInputImageItem) {
            out.push_back(String8::format("        streamId:%02" PRIx64 " bufferId:%02" PRIu64 " %s %s",
                item.stream->getStreamId(), item.buffer->getBufferId(),
                (item.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR) ? "ERROR" : "OK"),
                (item.buffer->getReleaseFence()==-1 ? "" : "with releaseFence")
            ));
        }
    }
    //
    if  ( ! cbParcel.vOutputImageItem.isEmpty() ) {
        out.push_back(String8("    OUT Image -"));
        for (auto const& item: cbParcel.vOutputImageItem) {
            if(item.buffer) {
                out.push_back(String8::format("        streamId:%02" PRIx64 " bufferId:%02" PRIu64 " %s %s",
                    item.stream->getStreamId(), item.buffer->getBufferId(),
                    (item.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR) ? "ERROR" : "OK"),
                    (item.buffer->getReleaseFence()==-1 ? "" : "with releaseFence")
                ));
            }
            else {
                out.push_back(String8::format("        streamId:%02" PRIx64 " buffer is nullptr", item.stream->getStreamId()));
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void convertToDebugString(HidlCallbackParcel const& cbParcel, android::Vector<android::String8>& out)
{
/*
    requestNo:80 errors#:10 shutter:10 o:meta#:10 i:image#:10 o:image#:10
        {ERROR_DEVICE/ERROR_REQUEST/ERROR_RESULT}
        {ERROR_BUFFER streamId:123 bufferId:456}
        OUT Meta  -
            partial#:123 "xxxxxxxxxx"
        IN Image -
            streamId:01 bufferId:04 OK
        OUT Image -
            streamId:02 bufferId:04 ERROR with releaseFence
*/
    std::map<uint32_t, uint32_t> NotifyMsgIdMap;
    std::map<uint32_t, std::vector<uint32_t>> ErrorMsgIdMap;
    std::map<uint32_t, std::vector<uint32_t>> CaptureResultIdMap;
    std::map<uint32_t, std::vector<uint32_t>> BufferResultIdMap;

    // update notify msg id map
    for ( size_t i = 0; i < cbParcel.vNotifyMsg.size(); ++i) {
        uint32_t requestNo = cbParcel.vNotifyMsg[i].msg.shutter.frameNumber;
        NotifyMsgIdMap[requestNo] = i;
    }
    // update error msg id map
    for ( size_t i = 0; i < cbParcel.vErrorMsg.size(); ++i) {
        uint32_t requestNo = cbParcel.vErrorMsg[i].msg.error.frameNumber;
        ErrorMsgIdMap[requestNo].push_back(i);
    }
    // update capture result & buffer result
    #define UPDATE_CAPTURE_RESULT_ID(vData, map) \
        for ( size_t i = 0; i < vData.size(); ++i) { \
            uint32_t requestNo = vData[i].frameNumber; \
            map[requestNo].push_back(i); \
        }
    UPDATE_CAPTURE_RESULT_ID(cbParcel.vCaptureResult, CaptureResultIdMap);
    UPDATE_CAPTURE_RESULT_ID(cbParcel.vBufferResult, BufferResultIdMap);
    #undef UPDATE_CAPTURE_RESULT_ID

    for ( auto const& requestNo : cbParcel.vRequestNo ) {
        String8 log("");
        log += String8::format("requestNo:%u", requestNo);
        if ( ! ErrorMsgIdMap[requestNo].empty() ) {
            log += String8::format(" errors#:%zu", ErrorMsgIdMap[requestNo].size());
        }
        if ( NotifyMsgIdMap.find(requestNo) != NotifyMsgIdMap.end() ) {
            log += String8::format(" shutter:%" PRId64 "", cbParcel.vNotifyMsg[NotifyMsgIdMap[requestNo]].msg.shutter.timestamp);
        }
        if  ( ! CaptureResultIdMap[requestNo].empty() ) {
            size_t phyMetaSize = 0;
            for ( auto const& id : CaptureResultIdMap[requestNo] ) {
                phyMetaSize += cbParcel.vCaptureResult[id].physicalCameraMetadata.size();
            }
            log += String8::format(" o:meta#:%zu", CaptureResultIdMap[requestNo].size());
            if( phyMetaSize )
                log += String8::format(" o:physic meta#:%zu", phyMetaSize);
        }
        if  ( ! BufferResultIdMap[requestNo].empty() ) {
            size_t inImgSize = 0, outImgSize = 0;
            for ( auto const& id : BufferResultIdMap[requestNo] ) {
                if ( cbParcel.vBufferResult[id].inputBuffer.streamId != -1 )
                    inImgSize += 1;
                outImgSize += cbParcel.vBufferResult[id].outputBuffers.size();
            }
            if( inImgSize )
                log += String8::format(" i:image#:%zu", inImgSize);
            if( outImgSize )
                log += String8::format(" o:image#:%zu", outImgSize);
        }
        out.push_back(log);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
CallbackHandler(
    std::shared_ptr<CommonInfo> pCommonInfo,
    std::shared_ptr<CamDejitter> pCamDejitter
)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) + "-CallbackHandler"}
    , mCommonInfo(pCommonInfo)
    , mCamDejitter(pCamDejitter)
    // , mHalBufHandler(pHalBufHandler)
    // , mResultMetadataQueue(pResultMetadataQueue)
{
    bEnableMetaMerge = ::property_get_int32("vendor.debug.camera.enableMetaMerge", 1);
    bEnableDejitter  = ::property_get_int32("vendor.debug.camera.EnableDejitter", 1);
    MY_LOGI("enableMetaMerge %d, bEnableDejitter %d", bEnableMetaMerge, bEnableDejitter);

    // create Message thread
    const std::string msgThreadName{std::to_string(mCommonInfo->mInstanceId)+":AppMgr-MsgCbHelper"};
    mMsgCbHelper = new CallbackHelper(mCommonInfo, msgThreadName, this, nullptr, nullptr);
    if  ( mMsgCbHelper == nullptr ) {
        MY_LOGE("Bad mMsgCbHelper");
    }

    // create Result thread
    const std::string resThreadName{std::to_string(mCommonInfo->mInstanceId)+":AppMgr-ResCbHelper"};
    mResCbHelper = new CallbackHelper(mCommonInfo, resThreadName, this, mMsgCbHelper, pCamDejitter);
    if  ( mResCbHelper == nullptr ) {
        MY_LOGE("Bad mResCbHelper");
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
destroy() -> void
{
    //
    if (mResCbHelper) {
        mResCbHelper->destroy();
        mResCbHelper = nullptr;
    }
    //
    if (mMsgCbHelper) {
        mMsgCbHelper->destroy();
        mMsgCbHelper = nullptr;
    }
}



/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpState(android::Printer& printer __unused, const std::vector<std::string>& options __unused) -> void
{
    auto logQueue = [&](auto const& queue) {
        for (auto const& item : queue) {
            Vector<String8> logs;
            convertToDebugString(item, logs);
            for (auto const& log : logs) {
                printer.printFormatLine("  %s", log.c_str());
            }
        }
    };

    printer.printLine(" *Pending callback results*");
    {
        mMsgCbHelper->dumpState(printer, options);
        mResCbHelper->dumpState(printer, options);
    }
    printer.printLine("");
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
waitUntilDrained(nsecs_t const timeout) -> int
{
    nsecs_t const startTime = ::systemTime();
    //
    auto timeoutToWait = [=](){
        nsecs_t const elapsedInterval = (::systemTime() - startTime);
        nsecs_t const timeoutToWait = (timeout > elapsedInterval)
                                    ? (timeout - elapsedInterval)
                                    :   0
                                    ;
        return timeoutToWait;
    };
    //
    int err = OK;
    err = mResCbHelper->waitUntilDrained(timeoutToWait());
    err = mMsgCbHelper->waitUntilDrained(timeoutToWait());
    //
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
push(std::list<CallbackParcel>& item) -> void
{

    //Transfers all elements from item to mQueue1 in CallbackHelper.
    //After that, item is empty.
    HidlCallbackQueue  resHCPs;
    HidlCallbackParcel msgHCP;
    convertCallbackParcelToHidl(item, resHCPs, msgHCP);
    resHCPs.push_back(msgHCP);

    // trigger result callback thread
    if (!resHCPs.empty()) {
        CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),"mResCbHelper::push")
        mResCbHelper->push(resHCPs);
        CAM_ULOGM_DTAG_END()
    }
}

/******************************************************************************
 *
 ******************************************************************************/

auto
ThisNamespace::
convertCallbackParcelToHidl(
    std::list<CallbackParcel>& cblist,
    HidlCallbackQueue& rResHCPs,
    HidlCallbackParcel& rMsgHCP
) -> void
{
    std::unordered_set<uint32_t> ErroFrameNumbers;
    //
    for (auto const& cbParcel : cblist) {
        for (size_t i = 0; i < cbParcel.vError.size(); i++) {
            if (cbParcel.vError[i].errorCode != ErrorCode::ERROR_BUFFER){
                ErroFrameNumbers.insert(cbParcel.requestNo);
            }
        }
    }
    //
    HidlCallbackParcel resHCP;
    for (auto const& cbParcel : cblist) {
        CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                                "Dev-%d:convert|request:%d", mCommonInfo->mInstanceId, cbParcel.requestNo);

        //
        rMsgHCP.vRequestNo.push_back(cbParcel.requestNo);
        //CallbackParcel::shutter
        convertShutterResult(cbParcel, rMsgHCP.vNotifyMsg);
        //
        //CallbackParcel::vError
        convertErrorResult(cbParcel, rMsgHCP.vErrorMsg);
        //
        //
        resHCP.vRequestNo.push_back(cbParcel.requestNo);
        //CallbackParcel::vOutputMetaItem
        //block the metadata callbacks which follow the error result
        if (ErroFrameNumbers.count(cbParcel.requestNo) == 0){
            if ( bEnableMetaMerge == 1 ){
                convertMetaResultWithMergeEnabled(cbParcel, resHCP.vCaptureResult, resHCP.vTempMetadataResult);
            }
            else{
                convertMetaResult(cbParcel, resHCP.vCaptureResult, resHCP.vTempMetadataResult);
            }
        }
        else{
            for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
            MY_LOGI("Ignore Metadata callback because error already occured, requestNo(%d)/partialResult(%u)",
                    cbParcel.requestNo, cbParcel.vOutputMetaItem[i].bufferNo);
            }
        }
        //
        //CallbackParcel::vOutputImageItem
        //CallbackParcel::vInputImageItem
        if ( mCamDejitter && mCamDejitter->enable() ) {
            // ensure only one djitter image buffer in a parcel
            convertImageResultWithDejitter(cbParcel, rResHCPs);
        }
        else {
            convertImageResult(cbParcel, resHCP.vBufferResult);
        }

        if (CC_UNLIKELY(getLogLevel() >= 1)) {
            Vector<String8> logs;
            convertToDebugString(cbParcel, logs);
            for (auto const& l : logs) {
                MY_LOGD("%s", l.c_str());
            }
        }

        CAM_ULOGM_DTAG_END()
    }
    if ( !resHCP.vBufferResult.empty() || !resHCP.vCaptureResult.empty() )
        rResHCPs.push_back(resHCP);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertShutterResult(CallbackParcel const& cbParcel, std::vector<NotifyMsg>& rvMsg) -> void
{
    CAM_ULOGM_FUNCLIFE();

    //CallbackParcel::shutter
    if  ( cbParcel.shutter != 0 )
    {
        MY_LOGI_IF(cbParcel.needIgnore,
                       "Ignore Shutter callback because error already occured, requestNo(%d)/timestamp(%lu)",
                       cbParcel.requestNo, cbParcel.shutter->timestamp);
        if  ( !cbParcel.needIgnore )
        {
            CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                                "Dev-%d:convertShutter|request:%d =timestamp(ns):%" PRId64 " needIgnore:%d",
                                mCommonInfo->mInstanceId, cbParcel.requestNo, cbParcel.shutter->timestamp, cbParcel.needIgnore);
            CAM_ULOGM_DTAG_END();

            rvMsg.push_back(NotifyMsg{
                .type = MsgType::SHUTTER,
                .msg  = {
                    .shutter = {
                        .frameNumber = cbParcel.requestNo,
                        .timestamp = cbParcel.shutter->timestamp,
                }}});
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertErrorResult(CallbackParcel const& cbParcel, std::vector<NotifyMsg>& rvMsg) -> void
{
    CAM_ULOGM_FUNCLIFE();

    //CallbackParcel::vError
    for (size_t i = 0; i < cbParcel.vError.size(); i++) {
        CallbackParcel::Error const& rError = cbParcel.vError[i];
        auto const errorStreamId = static_cast<int32_t>((rError.stream != 0) ? rError.stream->getStreamId() : -1);

        CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                            "Dev-%d:convertError|request:%d =errorCode:%d errorStreamId:%d",
                            mCommonInfo->mInstanceId, cbParcel.requestNo, rError.errorCode, errorStreamId);
        CAM_ULOGM_DTAG_END();

        rvMsg.push_back(NotifyMsg{
            .type = MsgType::ERROR,
            .msg  = {
                .error = {
                    .frameNumber = cbParcel.requestNo,
                    .errorCode = rError.errorCode,
                    .errorStreamId = errorStreamId,
            }}});
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertMetaResult(
    CallbackParcel const& cbParcel,
    std::vector<CaptureResult>& rvResult,
    std::vector<camera_metadata*>& rvResultMetadata
) -> void
{
    CAM_ULOGM_FUNCLIFE();

    //CallbackParcel::vOutputMetaItem
    for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
        auto const& rCbMetaItem = cbParcel.vOutputMetaItem[i];
        //
        camera_metadata *p_camera_metadata = nullptr;
        size_t camera_metadata_size = 0;
        IMetadata* pMetadata = rCbMetaItem.buffer->tryReadLock(mInstanceName.c_str());
        {
            MY_LOGI_IF(cbParcel.needIgnore,
                       "Ignore Metadata callback because error already occured, requestNo(%d)/partialResult(%u)",
                       cbParcel.requestNo, rCbMetaItem.bufferNo);

            if (CC_UNLIKELY(
                ! mCommonInfo->mMetadataConverter->convert(*pMetadata, p_camera_metadata, &camera_metadata_size)
            ||  ! p_camera_metadata
            ||  0 == camera_metadata_size ))
            {
                MY_LOGE("fail: IMetadata -> camera_metadata(%p) size:%zu", p_camera_metadata, camera_metadata_size);
            }
            rvResultMetadata.push_back(p_camera_metadata);

            switch (getLogLevel())
            {
            case 3:
                mCommonInfo->mMetadataConverter->dumpAll(*pMetadata, cbParcel.requestNo);
                break;
            case 2:
                mCommonInfo->mMetadataConverter->dump(*pMetadata, cbParcel.requestNo);
                break;
            default:
                break;
            }
        }
        rCbMetaItem.buffer->unlock(mInstanceName.c_str(), pMetadata);
        //
        if  (!cbParcel.needIgnore) {
            CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                                "Dev-%d:convertMeta|request:%d =partialResult:%d size:%zu",
                                    mCommonInfo->mInstanceId, cbParcel.requestNo, rCbMetaItem.bufferNo, camera_metadata_size);

            CaptureResult res = CaptureResult{
                .frameNumber    = cbParcel.requestNo,
                .result         = p_camera_metadata,
                .resultSize     = camera_metadata_size,
                .inputBuffer    = {.streamId = -1,},     // force assign -1 indicating no input buffer
                .partialResult  = rCbMetaItem.bufferNo,
            };
            //
            if ( res.partialResult == mCommonInfo->mAtMostMetaStreamCount){
                convertPhysicMetaResult(cbParcel, res, rvResultMetadata);
            }
            //
            rvResult.push_back(res);
            CAM_ULOGM_DTAG_END();
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertMetaResultWithMergeEnabled(
    CallbackParcel const& cbParcel,
    std::vector<CaptureResult>& rvResult,
    std::vector<camera_metadata*>& rvResultMetadata
) -> void
{
    CAM_ULOGM_FUNCLIFE();
    if ( cbParcel.vOutputMetaItem.size() > 0 ){
        IMetadata resultMeta = IMetadata();
        size_t resultBufNo = 0;
        String8 log = String8::format("requestNo:%u, bufferNo:", cbParcel.requestNo);
        //Merge the metadata items
        {
            for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
                auto const& SrcMetaItem = cbParcel.vOutputMetaItem[i];
                MY_LOGI_IF(cbParcel.needIgnore,
                           "Ignore Metadata callback because error already occured, requestNo(%d)/partialResult(%u)",
                           cbParcel.requestNo, SrcMetaItem.bufferNo);

                IMetadata* pSrcMeta = SrcMetaItem.buffer->tryReadLock(mInstanceName.c_str());
                if ( pSrcMeta == nullptr ){
                    MY_LOGW("Src Meta is null");
                }
                else {
                    if ( resultBufNo < SrcMetaItem.bufferNo ){
                        resultBufNo = SrcMetaItem.bufferNo;  //update the bufferNo to the last meta item in cbParcel
                    }
                    resultMeta += (*pSrcMeta);
                }
                log += String8::format(" %d", SrcMetaItem.bufferNo);
                //
                SrcMetaItem.buffer->unlock(mInstanceName.c_str(), pSrcMeta);
            }
        }

        //Convert the result metadata, and add it to CaptureResult
        camera_metadata *p_camera_metadata = nullptr;
        size_t camera_metadata_size = 0;
        {
            if (CC_UNLIKELY(
                ! mCommonInfo->mMetadataConverter->convert(resultMeta, p_camera_metadata, &camera_metadata_size)
            ||  ! p_camera_metadata
            ||  0 == camera_metadata_size ))
            {
                MY_LOGE("fail: IMetadata -> camera_metadata(%p) size:%zu", p_camera_metadata, camera_metadata_size);
            }
            rvResultMetadata.push_back(p_camera_metadata);

            switch (getLogLevel())
            {
            case 3:
                mCommonInfo->mMetadataConverter->dumpAll(resultMeta, cbParcel.requestNo);
                break;
            case 2:
                mCommonInfo->mMetadataConverter->dump(resultMeta, cbParcel.requestNo);
                break;
            default:
                break;
            }
        }
        //
        if  (!cbParcel.needIgnore) {
            CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                                "Dev-%d:convertMeta|request:%d =partialResult:%lu size:%zu",
                                        mCommonInfo->mInstanceId, cbParcel.requestNo, resultBufNo, camera_metadata_size);
            vector<uint8_t> temp;

            CaptureResult res = CaptureResult{
                .frameNumber    = cbParcel.requestNo,
                .resultSize     = camera_metadata_size,
                .result         = p_camera_metadata,
                .inputBuffer    = {.streamId = -1,},     // force assign -1 indicating no input buffer
                .partialResult  = (uint32_t)resultBufNo
            };
            //
            if ( res.partialResult == mCommonInfo->mAtMostMetaStreamCount){
                convertPhysicMetaResult(cbParcel, res, rvResultMetadata);
            }
            //
            if (CC_UNLIKELY(getLogLevel() >= 1))
            {
                log += String8::format(", totalSize:%zu", get_camera_metadata_size(res.result));
                MY_LOGD("%s", log.c_str());
            }
            rvResult.push_back(res);
            CAM_ULOGM_DTAG_END();
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertPhysicMetaResult(
    CallbackParcel const& cbParcel,
    CaptureResult& rvResult,
    std::vector<camera_metadata*>& rvResultMetadata
) -> void
{
    rvResult.physicalCameraMetadata.resize(cbParcel.vOutputPhysicMetaItem.size());
    for (size_t i = 0; i < cbParcel.vOutputPhysicMetaItem.size(); i++) {
        auto const& rCbPhysicMetaItem = cbParcel.vOutputPhysicMetaItem[i];
        camera_metadata *p_metadata = nullptr;
        size_t metadata_size = 0;
        IMetadata* pPhysicMetadata = rCbPhysicMetaItem.buffer->tryReadLock(mInstanceName.c_str());
        {
            if (CC_UNLIKELY(
                ! mCommonInfo->mMetadataConverter->convert(*pPhysicMetadata, p_metadata, &metadata_size)
            ||  ! p_metadata
            ||  0 == metadata_size ))
            {
                MY_LOGE("fail: Physical IMetadata -> camera_metadata(%p) size:%zu", p_metadata, metadata_size);
            }
            rvResultMetadata.push_back(p_metadata);

            switch (getLogLevel())
            {
                case 3:
                    mCommonInfo->mMetadataConverter->dumpAll(*pPhysicMetadata, cbParcel.requestNo);
                    break;
                case 2:
                    mCommonInfo->mMetadataConverter->dump(*pPhysicMetadata, cbParcel.requestNo);
                    break;
                default:
                    break;
            }
        }
        rCbPhysicMetaItem.buffer->unlock(mInstanceName.c_str(), pPhysicMetadata);
        PhysicalCameraMetadata physicalCameraMetadata = {
            .resultSize = metadata_size,
            .physicalCameraId = rCbPhysicMetaItem.camId,
            .metadata = p_metadata,
        };
        rvResult.physicalCameraMetadata[i] = physicalCameraMetadata;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertImageResult(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult) -> void
{
    CAM_ULOGM_FUNCLIFE();

    //CallbackParcel::vOutputImageItem
    //CallbackParcel::vInputImageItem
    //
    auto createNativeHandle = [](int dup_fd) -> native_handle_t* {
        if  ( -1 != dup_fd ) {
            auto handle = ::native_handle_create(/*numFds*/1, /*numInts*/0);
            if  ( CC_LIKELY(handle) ) {
                handle->data[0] = dup_fd;
                return handle;
            }
        }
        return nullptr;
    };
    //
    auto convert = [=](auto const& rCbImageItem, auto& rStreamBuffer){
        rStreamBuffer.streamId = rCbImageItem.stream->getStreamId();

        if( rCbImageItem.buffer) {
            rStreamBuffer.bufferId = rCbImageItem.buffer->getBufferId();
            rStreamBuffer.buffer   = (rCbImageItem.buffer.get() && rCbImageItem.buffer->getImageBufferHeap() &&
                                        rCbImageItem.buffer->getImageBufferHeap()->getBufferHandlePtr())?
                                        *(rCbImageItem.buffer->getImageBufferHeap()->getBufferHandlePtr()) :
                                        nullptr;
            // rStreamBuffer.buffer   = nullptr;
            rStreamBuffer.status   = rCbImageItem.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR)
                                        ? BufferStatus::ERROR
                                        : BufferStatus::OK
                                        ;
        }
        else {
            rStreamBuffer.bufferId = 0;
            rStreamBuffer.buffer   = nullptr;
            rStreamBuffer.status   = BufferStatus::ERROR;
        }
        rStreamBuffer.acquireFenceOp.hdl = nullptr;
        rStreamBuffer.acquireFenceOp.fd = -1;

        if(rCbImageItem.buffer) { // handle releaseFence only when buffer available
            rStreamBuffer.releaseFenceOp.fd = rCbImageItem.buffer->getReleaseFence();
            rStreamBuffer.releaseFenceOp.hdl = createNativeHandle(rCbImageItem.buffer->getReleaseFence());
        }
        CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                            "Dev-%d:convertImage stream %#" PRIx64 " |request:%d =%s",
                            mCommonInfo->mInstanceId, rCbImageItem.stream->getStreamId(), cbParcel.requestNo, toString(rStreamBuffer).c_str());
        CAM_ULOGM_DTAG_END();
    };
    //
    if  ( ! cbParcel.vOutputImageItem.isEmpty() || ! cbParcel.vInputImageItem.empty() )
    {
        vector<StreamBuffer> vOutBuffers;
        vOutBuffers.resize(cbParcel.vOutputImageItem.size());

        vector<StreamBuffer> vInputBuffers;
        vInputBuffers.resize(cbParcel.vInputImageItem.size());

        //Output
        for (size_t i = 0; i < cbParcel.vOutputImageItem.size(); i++) {
            CallbackParcel::ImageItem const& rCbImageItem = cbParcel.vOutputImageItem[i];
            convert(rCbImageItem, vOutBuffers[i]);
            // traceDisplayIf(cbParcel.requestNo, cbParcel.timestampShutter, rCbImageItem);
        }
        //
        //Input
        MY_LOGW_IF( cbParcel.vInputImageItem.size() > 1,
                    "input buffer should exceeds one; requestNo:%u, vInputBuffers:%zu",
                    cbParcel.requestNo, cbParcel.vInputImageItem.size() );
        for (size_t i = 0; i < cbParcel.vInputImageItem.size(); i++) {
            CallbackParcel::ImageItem const& rCbImageItem = cbParcel.vInputImageItem[i];
            convert(rCbImageItem, vInputBuffers[i]);
        }
        //
        //
        rvResult.push_back(CaptureResult{
            .frameNumber    = cbParcel.requestNo,
            // .fmqResultSize  = 0,
            // .result
            .outputBuffers  = std::move(vOutBuffers),
            .inputBuffer    = vInputBuffers.size() ? vInputBuffers[0] : StreamBuffer{.streamId = -1,},
            .partialResult  = 0,
        });
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
convertImageResultWithDejitter(
    CallbackParcel const& cbParcel,
    HidlCallbackQueue& rResHCPs
) -> void
{
    CAM_ULOGM_FUNCLIFE();

    //CallbackParcel::vOutputImageItem
    //CallbackParcel::vInputImageItem
    //
    auto createNativeHandle = [](int dup_fd) -> native_handle_t* {
        if  ( -1 != dup_fd ) {
            auto handle = ::native_handle_create(/*numFds*/1, /*numInts*/0);
            if  ( CC_LIKELY(handle) ) {
                handle->data[0] = dup_fd;
                return handle;
            }
        }
        return nullptr;
    };
    //
    auto convert = [=](auto const& rCbImageItem, auto& rStreamBuffer){
        rStreamBuffer.streamId = rCbImageItem.stream->getStreamId();

        if( rCbImageItem.buffer) {
            rStreamBuffer.bufferId = rCbImageItem.buffer->getBufferId();
            rStreamBuffer.buffer   = (rCbImageItem.buffer.get() && rCbImageItem.buffer->getImageBufferHeap() &&
                                        rCbImageItem.buffer->getImageBufferHeap()->getBufferHandlePtr())?
                                        *(rCbImageItem.buffer->getImageBufferHeap()->getBufferHandlePtr()) :
                                        nullptr;
            // rStreamBuffer.buffer   = nullptr;
            rStreamBuffer.status   = rCbImageItem.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR)
                                        ? BufferStatus::ERROR
                                        : BufferStatus::OK
                                        ;
        }
        else {
            rStreamBuffer.bufferId = 0;
            rStreamBuffer.buffer   = nullptr;
            rStreamBuffer.status   = BufferStatus::ERROR;
        }
        rStreamBuffer.acquireFenceOp.hdl = nullptr;
        rStreamBuffer.acquireFenceOp.fd = -1;

        if(rCbImageItem.buffer) { // handle releaseFence only when buffer available
            rStreamBuffer.releaseFenceOp.fd = rCbImageItem.buffer->getReleaseFence();
            rStreamBuffer.releaseFenceOp.hdl = createNativeHandle(rCbImageItem.buffer->getReleaseFence());
        }
        CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                            "Dev-%d:convertImage stream %#" PRIx64 " |request:%d =%s",
                            mCommonInfo->mInstanceId, rCbImageItem.stream->getStreamId(), cbParcel.requestNo, toString(rStreamBuffer).c_str());
        CAM_ULOGM_DTAG_END();
    };
    //
    if  ( ! cbParcel.vOutputImageItem.isEmpty() || ! cbParcel.vInputImageItem.empty() )
    {
        int32_t targetStreamId = mCamDejitter ?
                                 mCamDejitter->getDejitterStreamId() : -1;

        vector<StreamBuffer> vOutBuffers;
        vector<StreamBuffer> vInputBuffers;

        //Output
        bool hasDisplayBuffer = false;
        for (size_t i = 0; i < cbParcel.vOutputImageItem.size(); i++) {
            CallbackParcel::ImageItem const& rCbImageItem = cbParcel.vOutputImageItem[i];
            StreamBuffer dstBuffer;
            convert(rCbImageItem, dstBuffer);
            if (rCbImageItem.stream->getStreamId() == targetStreamId) {
                // create one parcel for display buffer for CameraHal Detjitter
                HidlCallbackParcel parcel;
                parcel.mDejitterInfo.hasDisplayBuf = true;
                parcel.mDejitterInfo.frameNumber = cbParcel.requestNo;
                parcel.vRequestNo.push_back(cbParcel.requestNo);
                parcel.vBufferResult.push_back(CaptureResult{
                    .frameNumber = cbParcel.requestNo,
                    // .fmqResultSize  = 0,
                    // .result
                    .outputBuffers = std::vector<StreamBuffer>(1, dstBuffer),
                    .result = nullptr,
                    .partialResult = 0,
                });
                rResHCPs.push_back(std::move(parcel));
            } else {
                vOutBuffers.push_back(std::move(dstBuffer));
            }
        }
        //
        //Input
        MY_LOGW_IF( cbParcel.vInputImageItem.size() > 1,
                    "input buffer should exceeds one; requestNo:%u, vInputBuffers:%zu",
                    cbParcel.requestNo, cbParcel.vInputImageItem.size() );
        for (size_t i = 0; i < cbParcel.vInputImageItem.size(); i++) {
            CallbackParcel::ImageItem const& rCbImageItem = cbParcel.vInputImageItem[i];
            StreamBuffer dstBuffer;
            convert(rCbImageItem, dstBuffer);
            vInputBuffers.push_back(std::move(dstBuffer));
        }
        //
        //
        if (!vOutBuffers.empty() || !vInputBuffers.empty()) {
            HidlCallbackParcel parcel;
            parcel.mDejitterInfo.hasDisplayBuf = hasDisplayBuffer;
            parcel.mDejitterInfo.frameNumber = cbParcel.requestNo;
            parcel.vRequestNo.push_back(cbParcel.requestNo);
            parcel.vBufferResult.push_back(CaptureResult{
                .frameNumber    = cbParcel.requestNo,
                // .fmqResultSize  = 0,
                // .result
                .outputBuffers  = std::move(vOutBuffers),
                .inputBuffer    = vInputBuffers.size() ? vInputBuffers[0] : StreamBuffer{.streamId = -1,},
                .partialResult  = 0,
            });
            rResHCPs.push_back(std::move(parcel));
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
traceDisplayIf(
    uint32_t requestNo,
    uint64_t timestampShutter,
    const CallbackParcel::ImageItem& item
)   -> void
{
    if  ( ATRACE_ENABLED() )
    {
        String8 s8Trace_HwComposer, s8Trace_HwTexture;

        auto isTarget = [=](MUINT const usage) {
            return  0 != (item.stream->getUsageForConsumer() & usage)
                &&  0 == item.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR)
                && -1 == item.buffer->getReleaseFence()
                &&  0 != timestampShutter
                    ;
        };

        auto traceTarget = [=](char const* szPrefix, MUINT const usage, String8& str) {
            if  ( isTarget(usage) ) {
                str = String8::format(
                    "Cam:%d:Fwk:%s|timestamp(ns):%" PRId64 " duration(ns):%" PRId64 " u:%" PRIX64 " %dx%d request:%d",
                    mCommonInfo->mInstanceId, szPrefix, timestampShutter, ::systemTime()-timestampShutter,
                    item.stream->getUsageForConsumer(), item.stream->getImgSize().w, item.stream->getImgSize().h,
                    requestNo
                );
                return true;
            }
            return false;
        };

        //Trace display
        (void)(traceTarget("Hwc", GRALLOC_USAGE_HW_COMPOSER, s8Trace_HwComposer)
            || traceTarget("Gpu", GRALLOC_USAGE_HW_TEXTURE, s8Trace_HwTexture));

        if  ( ! s8Trace_HwComposer.isEmpty() ) {
            CAM_ULOGM_DTAG_BEGIN(true, "%s",s8Trace_HwComposer.c_str());
            CAM_ULOGM_DTAG_END();
        }
        else if  ( ! s8Trace_HwTexture.isEmpty() ) {
            CAM_ULOGM_DTAG_BEGIN(true, "%s",s8Trace_HwTexture.c_str());
            CAM_ULOGM_DTAG_END();
        }
    }
}
