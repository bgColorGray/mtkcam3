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

#include <mtkcam/utils/std/Sync.h>
#include "AppStreamMgr.h"
#include "MyUtils.h"
#include <mtkcam3/pipeline/stream/StreamId.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
//
using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::Utils::ULog;
using namespace NSCam::Utils::Sync;
using AppMetaStreamBufferAllocatorT = AppMetaStreamBuffer::Allocator;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
        mCommonInfo->mDebugPrinter->printFormatLine(#level" [%s] " fmt, __FUNCTION__, ##arg); \
    } while(0)

#define MY_WARN(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
        mCommonInfo->mWarningPrinter->printFormatLine(#level" [%s] " fmt, __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
        mCommonInfo->mErrorPrinter->printFormatLine(#level" [%s] " fmt, __FUNCTION__, ##arg); \
    } while(0)

#define MY_LOGV(...)                MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...)                MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...)                MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...)                MY_WARN (W, __VA_ARGS__)
#define MY_LOGE(...)                MY_ERROR(E, __VA_ARGS__)
#define MY_LOGA(...)                MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
 *
 ******************************************************************************/
const std::string AppStreamMgr::MyDebuggee::mName{"NSCam::v3::IAppStreamManager"};
auto AppStreamMgr::MyDebuggee::debug(android::Printer& printer, const std::vector<std::string>& options) -> void
{
    auto p = mContext.promote();
    if ( p != nullptr ) {
        p->dumpState(printer, options);
    }
}


/******************************************************************************
 *
 ******************************************************************************/

auto
IAppStreamManager::
create(
    const CreationInfo& creationInfo
) -> IAppStreamManager*
{
    auto pInstance = new AppStreamMgr(creationInfo);
    if  ( ! pInstance || ! pInstance->initialize() ) {
        delete pInstance;
        return nullptr;
    }
    return pInstance;
}


/******************************************************************************
 *
 ******************************************************************************/
AppStreamMgr::
AppStreamMgr(const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>())
    , mInstanceName{std::to_string(creationInfo.mInstanceId) + ":AppStreamMgr"}
    , mStreamInfoBuilderFactory(std::make_shared<Utils::Camera3StreamInfoBuilderFactory>())
{
    if ( creationInfo.mErrorPrinter == nullptr
        || creationInfo.mWarningPrinter == nullptr
        || creationInfo.mDebugPrinter == nullptr
        || creationInfo.mCameraDeviceCallback == nullptr
        || creationInfo.mMetadataProvider == nullptr
        || creationInfo.mMetadataConverter == nullptr )
    {
        MY_LOGE("mErrorPrinter:%p mWarningPrinter:%p mDebugPrinter:%p mCameraDeviceCallback:%p mMetadataProvider:%p mMetadataConverter:%p",
            creationInfo.mErrorPrinter.get(),
            creationInfo.mWarningPrinter.get(),
            creationInfo.mDebugPrinter.get(),
            creationInfo.mCameraDeviceCallback.get(),
            creationInfo.mMetadataProvider.get(),
            creationInfo.mMetadataConverter.get());
        mCommonInfo = nullptr;
        return;
    }

    IGrallocHelper* pGrallocHelper = IGrallocHelper::singleton();
    if ( pGrallocHelper == nullptr ) {
        MY_LOGE("IGrallocHelper::singleton(): nullptr");
        mCommonInfo = nullptr;
        return;
    }

    if ( mCommonInfo != nullptr )
    {
        size_t aAtMostMetaStreamCount = 1;
        IMetadata::IEntry const& entry = creationInfo.mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_REQUEST_PARTIAL_RESULT_COUNT);
        if ( entry.isEmpty() ) {
            MY_LOGE("no static REQUEST_PARTIAL_RESULT_COUNT");
            aAtMostMetaStreamCount = 1;
        }
        else {
            aAtMostMetaStreamCount = entry.itemAt(0, Type2Type<MINT32>());
        }
        //
        int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
        if ( loglevel == 0 ) {
            loglevel = ::property_get_int32("vendor.debug.camera.log.AppStreamMgr", 0);
        }

        mCommonInfo->mLogLevel = loglevel;
        mCommonInfo->mInstanceId = creationInfo.mInstanceId;
        mCommonInfo->mErrorPrinter = creationInfo.mErrorPrinter;
        mCommonInfo->mWarningPrinter = creationInfo.mWarningPrinter;
        mCommonInfo->mDebugPrinter = creationInfo.mDebugPrinter;
        mCommonInfo->mDeviceCallback = creationInfo.mCameraDeviceCallback;
        mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
        mCommonInfo->mPhysicalMetadataProviders = creationInfo.mPhysicalMetadataProviders;
        mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
        mCommonInfo->mGrallocHelper = pGrallocHelper;
        mCommonInfo->mAtMostMetaStreamCount = aAtMostMetaStreamCount;
    }
}

/******************************************************************************
 *
 ******************************************************************************/
bool
AppStreamMgr::
initialize()
{
    status_t status = OK;
    //
    if  ( mCommonInfo == nullptr ) {
        MY_LOGE("Bad mCommonInfo");
        return false;
    }
    //
    {
        //
        mCamDejitter = CamDejitter::create(mCommonInfo);
        //
        mCallbackHandler = new CallbackHandler(mCommonInfo, mCamDejitter);
        if  ( mCallbackHandler == nullptr ) {
            MY_LOGE("Bad mCallbackHandler");
            return false;
        }
        //
        mBatchHandler = new BatchHandler(mCommonInfo, mCallbackHandler);
        if  ( mBatchHandler == nullptr ) {
            MY_LOGE("Bad mBatchHandler");
            return false;
        }
        //
        mFrameHandler = new FrameHandler(mCommonInfo, mBatchHandler);
        if  ( mFrameHandler == nullptr ) {
            MY_LOGE("Bad mFrameHandler");
            return false;
        }
        //
        mResultHandler = new ResultHandler(mCommonInfo, mFrameHandler);
        if  ( mResultHandler == nullptr ) {
            MY_LOGE("Bad mResultHandler");
            return false;
        }
        else {
            const std::string threadName{std::to_string(mCommonInfo->mInstanceId)+":AppMgr-RstHdl"};
            status = mResultHandler->run(threadName.c_str());
            if  ( OK != status ) {
                MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(), status, ::strerror(-status));
                return false;
            }
        }
    }
    //
    mDebuggee = std::make_shared<MyDebuggee>(this);
    if ( auto pDbgMgr = IDebuggeeManager::get() ) {
        mDebuggee->mCookie = pDbgMgr->attach(mDebuggee, 1);
    }
    //
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
destroy() -> void
{
    auto resLock = mInterfaceLock.timedLock(::ms2ns(1000));
    if  ( OK != resLock ) {
        MY_LOGW("timedLock failed; still go on to destroy");
    }

    mFrameHandler->dumpIfHasInflightRequest();

    mResultHandler->destroy();
    mResultHandler = nullptr;

    mFrameHandler->destroy();
    mFrameHandler = nullptr;

    mBatchHandler->destroy();
    mBatchHandler = nullptr;

    mCallbackHandler->destroy();
    mCallbackHandler = nullptr;

    mCamDejitter->destroy();
    mCamDejitter = nullptr;

    // if(mHalBufHandler) {
    //     mHalBufHandler->destroy();
    //     mHalBufHandler = nullptr;
    // }

    if ( mDebuggee != nullptr ) {
        if ( auto pDbgMgr = IDebuggeeManager::get() ) {
            pDbgMgr->detach(mDebuggee->mCookie);
        }
        mDebuggee = nullptr;
    }

    if  ( OK == resLock ) {
        mInterfaceLock.unlock();
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
dumpState(android::Printer& printer __unused, const std::vector<std::string>& options __unused) -> void
{
    printer.printFormatLine("## App Stream Manager  [%u]\n", mCommonInfo->mInstanceId);

    if  ( OK == mInterfaceLock.timedLock(::ms2ns(500)) ) {

        dumpStateLocked(printer, options);

        mInterfaceLock.unlock();
    }
    else {
        printer.printLine("mInterfaceLock.timedLock timeout");
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
dumpStateLocked(android::Printer& printer, const std::vector<std::string>& options) -> void
{
    if ( mFrameHandler != nullptr ) {
        mFrameHandler->dumpState(printer, options);
    }
    if ( mBatchHandler != nullptr ) {
        printer.printLine(" ");
        mBatchHandler->dumpState(printer, options);
    }
    if ( mCallbackHandler != nullptr ) {
        printer.printLine(" ");
        mCallbackHandler->dumpState(printer, options);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
setConfigMap(
    ImageConfigMap& imageConfigMap,
    MetaConfigMap& metaConfigMap
) -> void
{
    MY_LOGD("[debug] imageConfigMap.size()=%lu, metaConfigMap.size()=%lu",
                imageConfigMap.size(), metaConfigMap.size());
    mFrameHandler->setConfigMap(imageConfigMap, metaConfigMap);
}
/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
configureStreams(
    const StreamConfiguration& requestedConfiguration,
    const ConfigAppStreams& rStreams
) -> int
{
    CAM_ULOGM_FUNCLIFE();

    Mutex::Autolock _l(mInterfaceLock);

    ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG, "[AppMgr-configureStreams] ");
    int status = OK;

    mFrameHandler->setOperationMode(rStreams.operationMode);
    // clear old BatchStreamId & remove Unused Config Stream
    mBatchHandler->resetBatchStreamId();
    std::unordered_set<StreamId_T> usedStreamIds;
    for (size_t i = 0; i < rStreams.vImageStreams.size(); i++) {
        StreamId_T const streamId = rStreams.vImageStreams.keyAt(i);
        auto pStreamInfo = mFrameHandler->getConfigImageStream(streamId);
        if ( pStreamInfo == nullptr ) {
            MY_LOGE(
                "no image stream info for stream id %" PRId64 " - %zu/%zu",
                streamId, i, rStreams.vImageStreams.size()
            );
            return -ENODEV;
        }

        mBatchHandler->checkStreamUsageforBatchMode(pStreamInfo);
        usedStreamIds.insert(streamId);
        pStreamInfo->dumpState(logPrinter);
    }

    mFrameHandler->removeUnusedConfigStream(usedStreamIds);

    mCamDejitter->configure(requestedConfiguration);

    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
flushRequest(
    const vector<CaptureRequest>& requests
)   -> void
{
    auto prepareBuffer = [this](StreamBuffer const& buffer, auto& rvImageItem) {
        if  ( -1 != buffer.streamId ) {
            if ( 0 != buffer.bufferId  ) {
                auto const& pStreamInfo = mFrameHandler->getConfigImageStream(buffer.streamId);
                int acquire_fence = -1;
                if (buffer.acquireFenceOp.type == FenceType::HDL) {
                    if (buffer.acquireFenceOp.hdl != nullptr) {
                       acquire_fence = ::dup(buffer.acquireFenceOp.hdl->data[0]);
                    }
                } else {
                    acquire_fence = buffer.acquireFenceOp.fd;
                }

                rvImageItem.push_back(
                    CallbackParcel::ImageItem{
                        .buffer = AppErrorImageStreamBuffer::Allocator()(
                                    buffer.bufferId,
                                    pStreamInfo.get(),
                                    acquire_fence),
                        .stream = pStreamInfo,
                    }
                );
            }
            // TODO : use another condition
            // TODO : handle externalbuffermanager
            // if( mHalBufHandler || 0 != buffer.bufferId ) {
            //     auto const& pStreamInfo = mFrameHandler->getConfigImageStream(buffer.streamId);
            //     rvImageItem.push_back(
            //         CallbackParcel::ImageItem{
            //             .buffer = Utils::AppErrorImageStreamBuffer::Allocator()(
            //                         buffer.bufferId,
            //                         pStreamInfo.get(),
            //                         (buffer.acquireFence != nullptr ? ::dup(buffer.acquireFence->data[0]) : -1)),
            //             .stream = pStreamInfo,
            //         }
            //     );

            // }
        }
    };

    Mutex::Autolock _l(mInterfaceLock);
    std::list<CallbackParcel> cbList;
    for (auto const& req : requests) {
        CallbackParcel cbParcel = {
            .shutter = nullptr,
            .timestampShutter = 0,
            .requestNo = req.frameNumber,
            .valid = MTRUE,
        };

        //  cbParcel.vError
        cbParcel.vError.push_back(
            CallbackParcel::Error{
                . errorCode = ErrorCode::ERROR_REQUEST,
            }
        );

        //  cbParcel.vInputImageItem <- req.inputBuffer
        prepareBuffer(req.inputBuffer, cbParcel.vInputImageItem);

        //  cbParcel.vOutputImageItem <- req.outputBuffers
        for (auto const& buffer : req.outputBuffers) {
            prepareBuffer(buffer, cbParcel.vOutputImageItem);
        }
        cbList.push_back(std::move(cbParcel));
    }
    mBatchHandler->push(cbList);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
abortRequest(
    const android::Vector<Request>& requests
) -> void
{
    auto pHandler = mResultHandler;
    String8 log = String8::format("Abort Request, requestNo:");
    for( auto const& req : requests ) {
        log += String8::format(" %u", req.requestNo);
        UpdateResultParams param = {
            .requestNo = req.requestNo,
        };
        // mark error on a created last partial meta
        sp<IMetaStreamInfo> pStreamInfo = new AppMetaStreamInfo(
                                                            "Meta:App:ErrorLast",
                                                            eSTREAMID_LAST_METADATA,
                                                            eSTREAMTYPE_META_OUT,
                                                            0
                                                            );
        sp<IMetaStreamBuffer> pStreamBuffer = AppMetaStreamBuffer::Allocator(pStreamInfo.get())();
        if ( pStreamBuffer ) {
            pStreamBuffer->finishUserSetup();
            pStreamBuffer->markStatus(STREAM_BUFFER_STATUS::ERROR);
            param.resultMeta.add(pStreamBuffer);
        }
        param.hasLastPartial = true;
        // mark error on all registered image buffer
        for (size_t i = 0; i < req.vOutputImageStreams.size(); i++)
        {
            StreamId_T const streamId = req.vOutputImageStreams.keyAt(i);
            mFrameHandler->updateStreamBuffer(req.requestNo, streamId, nullptr);
        }

        // output buffer
        for (size_t i = 0; i < req.vOutputImageBuffers.size(); i++)
        {
            sp<IImageStreamBuffer> pOutImageBuffer = req.vOutputImageBuffers.valueAt(i);
            pOutImageBuffer->finishUserSetup();
            pOutImageBuffer->markStatus(STREAM_BUFFER_STATUS::ERROR);
        }
        // input buffer
        for (size_t i = 0; i < req.vInputImageBuffers.size(); i++)
        {
            sp<IImageStreamBuffer> pInImageBuffer = req.vInputImageBuffers.valueAt(i);
            pInImageBuffer->finishUserSetup();
            pInImageBuffer->markStatus(STREAM_BUFFER_STATUS::ERROR);
        }
        if ( CC_LIKELY(pHandler != nullptr) ) {
            pHandler->enqueResult(param);
        }
    }
    MY_LOGW("%s", log.c_str());
}


/******************************************************************************
 *
 ******************************************************************************/
auto AppStreamMgr::
earlyCallbackMeta(
    const Request& request
) -> void
{
    // early callback metadata tag list
    std::list<IMetadata::Tag_t> tagList = {
        MTK_REQUEST_ID, // for framework api1 stopPreview
    };
    auto pHandler = mResultHandler;
    auto& ctrlMeta = request.vInputMetaBuffers.valueFor(eSTREAMID_META_APP_CONTROL);
    IMetadata* const pSrcMeta = ctrlMeta->tryWriteLock(LOG_TAG);
    if (pSrcMeta == nullptr) {
        MY_LOGW("pointer to SrcMeta is null, disable early callback");
        return;
    }
    IMetadata::IEntry eSrc = pSrcMeta->entryFor(MTK_CONTROL_CAPTURE_INTENT);
    if (!eSrc.isEmpty()) {
        MUINT8 capture_intent = eSrc.itemAt(0, Type2Type<MUINT8>());
        // check if it is not preview, skip early callback.
        if (capture_intent != 1) {
            ctrlMeta->unlock(LOG_TAG, pSrcMeta);
            return;
        }
    }
    String8 log = String8::format("early callback metadata, requestNo: %u", request.requestNo);
    UpdateResultParams param = {
        .requestNo = request.requestNo,
        .realTimePartial = RealTime::SOFT,
    };
    sp<IMetaStreamInfo> pStreamInfo = new AppMetaStreamInfo(
        "Meta:App:EarlyMeta",
        eSTREAMID_META_APP_DYNAMIC_DEVICE,
        eSTREAMTYPE_META_OUT,
        0);
    sp<IMetaStreamBuffer> pStreamBuffer = AppMetaStreamBuffer::Allocator(pStreamInfo.get())();
    if (pStreamBuffer) {
        // extract early callback metadata
        IMetadata* const pDstMeta = pStreamBuffer->tryWriteLock(LOG_TAG);
        if (pDstMeta == nullptr) {
            MY_LOGW("pointer to DstMeta is null, disable early callback");
            return;
        }
        for (IMetadata::Tag_t tag : tagList) {
            eSrc = pSrcMeta->entryFor(tag);
            pDstMeta->update(tag, eSrc);
            pSrcMeta->remove(tag);
        }
        ctrlMeta->unlock(LOG_TAG, pSrcMeta);
        pStreamBuffer->unlock(LOG_TAG, pDstMeta);
        // mark as used
        pStreamBuffer->finishUserSetup();
        pStreamBuffer->markStatus(STREAM_BUFFER_STATUS::WRITE_OK);
        param.resultMeta.add(pStreamBuffer);
        //
        if (CC_LIKELY(pHandler != nullptr)) {
            pHandler->enqueResult(param);
        }
    }
    else {
        log += String8::format(" (failed)");
    }
    MY_LOGI("%s", log.c_str());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
submitRequests(
    /*const vector<CaptureRequest>& requests,*/
    const android::Vector<Request>& rRequests
)   -> int
{
    Mutex::Autolock _l(mInterfaceLock);
    CAM_ULOGM_FUNCLIFE();
    for ( const auto& rRequest : rRequests ) {
        int err = mFrameHandler->registerFrame(rRequest);
        if ( err != OK )
            return err;
    }
    if(rRequests.size() > 1)
        mBatchHandler->registerBatch(rRequests);
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
updateStreamBuffer(
    uint32_t frameNo,
    StreamId_T streamId,
    android::sp<IImageStreamBuffer> const pBuffer
)   -> int
{
    return mFrameHandler->updateStreamBuffer(frameNo, streamId, pBuffer);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
updateResult(UpdateResultParams const& params) -> void
{
    String8 log =
        String8::format(
            "User %#"  PRIxPTR " of requestNo %u, real time:%d, last partial:%d, ",
            params.userId, params.requestNo, params.realTimePartial, params.hasLastPartial);
    //logical metadata
    const android::Vector<android::sp<IMetaStreamBuffer>>& streamBuf = params.resultMeta;
    for(int i = 0; i < streamBuf.size(); i++){
        IMetadata* pMetadata = streamBuf[i]->tryReadLock(mInstanceName.c_str());;
        if(pMetadata != nullptr){
            MUINT count = pMetadata->count();
            log += String8::format("Logical Meta %d: size = %u, ", i, count);
            streamBuf[i]->unlock(mInstanceName.c_str(), pMetadata);
        }
        else{
            MY_LOGW("Logical Meta is null in streamBuf %s", streamBuf[i]->getName());
        }
    }
    //physical metadata
    for(int j = 0; j < params.physicalResult.size(); j++){
        sp<PhysicalResult> physicRes = params.physicalResult[j];
        log += String8::format("Physical Meta %d for physical Cam:%s, ",
                               j, physicRes->physicalCameraId.c_str());
        IMetadata* pPhysicMetadata = (physicRes->physicalResultMeta)->tryReadLock(mInstanceName.c_str());
        if(pPhysicMetadata != nullptr){
            MUINT count = pPhysicMetadata->count();
            log += String8::format("Physical Meta %d: size = %u, ", j, count);
        }
        else{
            MY_LOGW("Physical Meta %d is null", j);
        }
        (physicRes->physicalResultMeta)->unlock(mInstanceName.c_str(), pPhysicMetadata);
    }
    // update SOF to cam dejitter
    if (params.timestampStartOfFrame) {
        mCamDejitter->storeSofTimestamp(params.requestNo, params.timestampStartOfFrame);
    }
    //
    MY_LOGD_IF(mCommonInfo->mLogLevel >= 1, "%s", log.c_str());
    //
    auto pHandler = mResultHandler;
    if ( CC_LIKELY(pHandler != nullptr) ) {
        pHandler->enqueResult(params);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
updateAvailableMetaResult(const sp<UpdateAvailableParams>& param) -> void
{
    MY_LOGD("User %s of frame %u", param->callerName.c_str(), param->frameNo);
    UpdateResultParams tempParams = {
        .requestNo      = param->frameNo,
        .userId         = param->userId,
        .hasLastPartial = 0,
    };
    sp<IMetaStreamInfo> earlyCbInfo = nullptr;
    earlyCbInfo = new AppMetaStreamInfo(
                "App:Meta:EarlyCb",
                eSTREAMID_META_APP_DYNAMIC_EARLYCB,
                eSTREAMTYPE_META_OUT,
                0
                );
    if (!earlyCbInfo) {
        MY_LOGE("create earlyCb streamInfo failed");
        return;
    }
    sp<AppMetaStreamBuffer> pEarlyCbStreamBuffer =
        AppMetaStreamBufferAllocatorT(earlyCbInfo.get())();
    if (pEarlyCbStreamBuffer == nullptr) {
        MY_LOGE("create earlyCb streamBuffer failed");
        return;
    }
    IMetadata* meta = pEarlyCbStreamBuffer->tryWriteLock(LOG_TAG);
    if (meta == nullptr) {
        return;
    }
    MUINT8 af_state = 0;
    if (IMetadata::getEntry<MUINT8>(&param->resultMetadata, MTK_CONTROL_AF_STATE, af_state)) {
        IMetadata::setEntry<MUINT8>(meta, MTK_CONTROL_AF_STATE, af_state);
    } else {
        MY_LOGE("Cannot find af_state in earlyCb, Update af_state failed.");
        return;
    }
    pEarlyCbStreamBuffer->unlock(LOG_TAG, meta);
    pEarlyCbStreamBuffer->finishUserSetup();

    tempParams.resultMeta.setCapacity(1);
    for ( size_t i=0; i<1; ++i ) {
        tempParams.resultMeta.add(pEarlyCbStreamBuffer);
    }
    updateResult(tempParams);

}

/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
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
    //
    int err = OK;
    Mutex::Autolock _l(mInterfaceLock);

    (void)((OK == (err = mFrameHandler->waitUntilDrained(timeout)))
        && (OK == (err = mBatchHandler->waitUntilDrained(timeoutToWait())))
        && (OK == (err = mCallbackHandler->waitUntilDrained(timeoutToWait()))))
        // && (!mHalBufHandler || OK == (err = mHalBufHandler->waitUntilDrained(timeoutToWait()))))
            ;

    if (OK != err) {
        MY_LOGW("timeout(ns):%" PRId64 " err:%d(%s)", timeout, -err, ::strerror(-err));
        ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG, "[waitUntilDrained] ");
        dumpStateLocked(logPrinter, {});
    }

    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
getAppStreamInfoBuilderFactory() const -> std::shared_ptr<IStreamInfoBuilderFactory>
{
    return mStreamInfoBuilderFactory;
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppStreamMgr::flush() -> void {
  if (mCamDejitter) {
    mCamDejitter->flush();
  }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppStreamMgr::
markStreamError(uint32_t requestNo, StreamId_T streamId) -> void
{
//    Mutex::Autolock _l(mInterfaceLock);
    if(mFrameHandler)
        mFrameHandler->updateStreamBuffer(requestNo, streamId, nullptr);
}
