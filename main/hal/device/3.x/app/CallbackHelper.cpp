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

#define ThisNamespace   AppStreamMgr::CallbackHandler::CallbackHelper

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
CallbackHelper(
    std::shared_ptr<CommonInfo> pCommonInfo,
    std::string const sInstanceName,
    android::sp<CallbackHandler> pCallbackHandler,
    android::sp<CallbackHelper> pCallbackHelper_Msg,
    std::shared_ptr<CamDejitter> pCamDejitter
)
    : mInstanceName(sInstanceName)
    , mCommonInfo(pCommonInfo)
    , mCallbackHandler(pCallbackHandler)
    , mCallbackHelper_Msg(pCallbackHelper_Msg)
    , mCamDejitter(pCamDejitter)
{
    status_t status = OK;
    if(pCallbackHelper_Msg)
        mCbHasRes = true;
    status = this->run(mInstanceName.c_str());
    if  ( OK != status ) {
        MY_LOGE("Fail to run the thread %s - status:%d(%s)", mInstanceName.c_str(), status, ::strerror(-status));
    }
}


/******************************************************************************
 *
 ******************************************************************************/
// Good place to do one-time initializations
auto
ThisNamespace::
readyToRun() -> status_t
{
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
threadLoop() -> bool
{
    if ( !waitUntilQueue1NotEmpty() ) {
        MY_LOGD_IF(1, "Queue1 is empty");
        return true;
    }
    //
    //
    bool bDejitterEnalbe = mCamDejitter && mCamDejitter->enable();
    if (bDejitterEnalbe) {
        {
            Mutex::Autolock _l1(mQueue1Lock);
            mCamDejitter->scanSrcQueue(mQueue1);
        }
        if (mCamDejitter->waitForDejitter())
            return true;
        {
            Mutex::Autolock _l1(mQueue1Lock);
            Mutex::Autolock _l2(mQueue2Lock);
            mCamDejitter->splice(mQueue2, mQueue1);
            mQueue1Cond.broadcast();
        }
    } else {
        Mutex::Autolock _l1(mQueue1Lock);
        Mutex::Autolock _l2(mQueue2Lock);
        // Transfers all elements from mQueue1 to mQueue2.
        // After that, mQueue1 is empty.
        mQueue2.splice(mQueue2.end(), mQueue1);
        mQueue1Cond.broadcast();
    }
    //
    performCallback();
    if (bDejitterEnalbe)
        mCamDejitter->postCallback();
    //
    return  true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
waitUntilQueue1NotEmpty() -> bool
{
    Mutex::Autolock _l(mQueue1Lock);

    while ( ! exitPending() && mQueue1.empty() )
    {
        int err = mQueue1Cond.wait(mQueue1Lock);
        MY_LOGW_IF(
            OK != err,
            "exitPending:%d mQueue1#:%zu err:%d(%s)",
            exitPending(), mQueue1.size(), err, ::strerror(-err)
        );
    }
    return ! mQueue1.empty();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::push(HidlCallbackParcel& item) -> void {
    Mutex::Autolock _l(mQueue1Lock);

    // Transfers all elements from item to mQueue1.
    // After that, item is empty.
    mQueue1.push_back(std::move(item));
    if (mCamDejitter)
        mCamDejitter->wakeupAndRefresh();
    mQueue1Cond.broadcast();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::push(HidlCallbackQueue& items) -> void {
    Mutex::Autolock _l(mQueue1Lock);

    // Transfers all elements from item to mQueue1.
    // After that, item is empty.
    mQueue1.splice(mQueue1.end(), items);
    if (mCamDejitter)
        mCamDejitter->wakeupAndRefresh();
    mQueue1Cond.broadcast();
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
destroy() -> void
{
    mCommonInfo->mDebugPrinter->printFormatLine("[destroy] %s->join +", mInstanceName.c_str());
    this->requestExit();
    this->join();
    mCommonInfo->mDebugPrinter->printFormatLine("[destroy] %s->join -", mInstanceName.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
requestExit() -> void
{
    MY_LOGD("+ %s", mInstanceName.c_str());
    //
    {
        Mutex::Autolock _l1(mQueue1Lock);
        Mutex::Autolock _l2(mQueue2Lock);
        Thread::requestExit();
        mQueue1Cond.broadcast();
        mQueue2Cond.broadcast();
    }
    //
    MY_LOGD("- %s", mInstanceName.c_str());
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
    auto waitEmpty = [=](Mutex& lock, Condition& cond, auto const& queue) -> int {
        int err = OK;
        Mutex::Autolock _l(lock);
        while ( ! exitPending() && ! queue.empty() )
        {
            err = cond.waitRelative(lock, timeoutToWait());
            if  ( OK != err ) {
                break;
            }
        }
        //
        if  ( queue.empty() ) { return OK; }
        if  ( exitPending() ) { return DEAD_OBJECT; }
        return err;
    };
    //
    //
    int err = OK;
    if  (   OK != (err = waitEmpty(mQueue1Lock, mQueue1Cond, mQueue1))
        ||  OK != (err = waitEmpty(mQueue2Lock, mQueue2Cond, mQueue2)))
    {
        MY_LOGW(
            "mQueue1:#%zu mQueue2:#%zu exitPending:%d timeout(ns):%" PRId64 " elapsed(ns):%" PRId64 " err:%d(%s)",
            mQueue1.size(), mQueue2.size(), exitPending(), timeout, (::systemTime() - startTime), err, ::strerror(-err)
        );
    }
    //
    return err;
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
        Mutex::Autolock _l1(mQueue1Lock);
        logQueue(mQueue1);

        if  ( tryLock(mQueue2Lock) )
        {
            if ( ! mQueue1.empty() ) {
                printer.printLine("  ***");
            }
            logQueue(mQueue2);
            mQueue2Lock.unlock();
        }
    }

    printer.printLine("");
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
performCallback() -> void
{
    CAM_ULOGM_FUNCLIFE();
    HidlCallbackParcel hidlCbParcelRes;
    HidlCallbackParcel hidlCbParcelShutter;
    HidlCallbackParcel hidlCbParcelError;

    auto appendCbData = [](auto& src, auto& dst) -> void {
        if (dst.empty()) {
            dst = std::move(src);
        }
        else {
            dst.reserve(dst.size() + src.size());
            std::move(std::begin(src), std::end(src), std::back_inserter(dst));
            src.clear();
        }
    };

    {
        Mutex::Autolock _l2(mQueue2Lock);

        for (auto& parcel : mQueue2) {
            //
            if (CC_UNLIKELY(getLogLevel() >= 1)) {
                Vector<String8> logs;
                convertToDebugString(parcel, logs);
                for (auto const& l : logs) {
                    MY_LOGD("%s", l.c_str());
                }
            }
            appendCbData(parcel.vRequestNo, hidlCbParcelRes.vRequestNo);
            appendCbData(parcel.vNotifyMsg, hidlCbParcelShutter.vNotifyMsg);
            appendCbData(parcel.vErrorMsg, hidlCbParcelError.vErrorMsg);
            appendCbData(parcel.vCaptureResult, hidlCbParcelRes.vCaptureResult);
            appendCbData(parcel.vBufferResult, hidlCbParcelRes.vBufferResult);
            appendCbData(parcel.vTempMetadataResult, hidlCbParcelRes.vTempMetadataResult);
        }
        mQueue2.clear();
        mQueue2Cond.broadcast();    //inform anyone of empty mQueue2
    }

    //  send shutter callbacks
    if  ( ! hidlCbParcelShutter.vNotifyMsg.empty() ) {
        if(mCbHasRes) {
            if(mCallbackHelper_Msg.get()) {
                mCallbackHelper_Msg->push(hidlCbParcelShutter);
            }
        } else {
            if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
                cb->notify(hidlCbParcelShutter.vNotifyMsg);
            } else {
                MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                        mCommonInfo->mDeviceCallback.use_count());
            }
        }
    }

    //  send image callback
    if  ( ! hidlCbParcelRes.vBufferResult.empty() ) {
        if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
            cb->processCaptureResult(hidlCbParcelRes.vBufferResult);
        } else {
            MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                    mCommonInfo->mDeviceCallback.use_count());
        }
    }
    //  send metadata callbacks
    if  ( ! hidlCbParcelRes.vCaptureResult.empty() ) {
        if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
            cb->processCaptureResult(hidlCbParcelRes.vCaptureResult);
        } else {
            MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                    mCommonInfo->mDeviceCallback.use_count());
        }
    }

    if  ( ! hidlCbParcelError.vErrorMsg.empty() ) {
    if(mCbHasRes) {
        if(mCallbackHelper_Msg.get()) {
            mCallbackHelper_Msg->push(hidlCbParcelError);
        }
        } else {
            if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
                cb->notify(hidlCbParcelError.vErrorMsg);
            } else {
                MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                         mCommonInfo->mDeviceCallback.use_count());
            }
        }
    }

    //  free the memory of camera_metadata.
    for (auto& v : hidlCbParcelRes.vTempMetadataResult) {
        mCommonInfo->mMetadataConverter->freeCameraMetadata(v);
        v = nullptr;
    }

    MY_LOGD_IF(getLogLevel() >= 1, "-");
}
