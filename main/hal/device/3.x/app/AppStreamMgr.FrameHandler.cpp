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

#include <iostream>
#include "MyUtils.h"
#include "AppStreamMgr.h"
#include <mtkcam3/pipeline/stream/StreamId.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
//
// #if (MTKCAM_HAVE_AEE_FEATURE == 1)
#if 0
#include <aee.h>
#define AEE_ASSERT(fmt, arg...) \
    do { \
        android::String8 const str = android::String8::format(fmt, ##arg); \
        CAM_LOGE("ASSERT(%s) fail", str.string()); \
        aee_system_exception( \
            "mtkcam/Metadata", \
            NULL, \
            DB_OPT_DEFAULT, \
            str.string()); \
    } while(0)
#else
#define AEE_ASSERT(fmt, arg...)
#endif
//
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::Utils::ULog;

#define ThisNamespace   AppStreamMgr::FrameHandler

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
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
static auto stateToText(ThisNamespace::State::T state) -> android::String8
{
    switch (state) {
    case ThisNamespace::State::IN_FLIGHT:   return String8("in-flight");
    case ThisNamespace::State::PRE_RELEASE: return String8("pre-release");
    case ThisNamespace::State::VALID:       return String8("valid");
    case ThisNamespace::State::ERROR:       return String8("error");
    }
    return String8("never happen");
};


/******************************************************************************
 *
 ******************************************************************************/
static auto historyToText (android::BitSet32 v) -> android::String8
{
    String8 s("");
    if (v.hasBit(ThisNamespace::HistoryBit::RETURNED)) {
        s += "returned";
    }
    if (v.hasBit(ThisNamespace::HistoryBit::ERROR_SENT_FRAME)) {
        s += "error-sent-frame";
    }
    if (v.hasBit(ThisNamespace::HistoryBit::ERROR_SENT_META)) {
        s += "error-sent-meta";
    }
    if (v.hasBit(ThisNamespace::HistoryBit::ERROR_SENT_IMAGE)) {
        s += "error-sent-image";
    }
    return s;
};


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
FrameHandler(std::shared_ptr<CommonInfo> pCommonInfo, android::sp<BatchHandler> pBatchHandler)
    : RefBase()
    , mInstanceName{std::to_string(pCommonInfo->mInstanceId) + "-FrameHandler"}
    , mCommonInfo(pCommonInfo)
    , mBatchHandler(pBatchHandler)
{
    bIsDejitterEnabled = (::property_get_int32("vendor.debug.camera.dejitter.enable", 1) > 0);
    bEnableMetaPending = ::property_get_int32("vendor.debug.camera.enableMetaPending", 1);
    bEnableMetaHitched = ::property_get_int32("vendor.debug.camera.enableMetaHitched", 1);
    MY_LOGI("IsDejitterEnabled = %d, EnableMetaPending = %d, EnableMetaHitched = %d",
            bIsDejitterEnabled, bEnableMetaPending, bEnableMetaHitched);

    MY_LOGD("mImageConfigMap.size()=%lu", mImageConfigMap.size());

    MY_LOGD("[FrameHandler] mImageConfigMap:%p",
            &mImageConfigMap);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
destroy() -> void
{
    mBatchHandler = nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
setOperationMode(uint32_t operationMode) -> void
{
    if ( mOperationMode != operationMode ) {
        MY_LOGI("operationMode change: %#x -> %#x", mOperationMode, operationMode);
        mOperationMode = operationMode;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
removeUnusedConfigStream(std::unordered_set<StreamId_T>const& usedStreamIds) -> void
{
    Mutex::Autolock _l(mImageConfigMapLock);
    for (ssize_t i = mImageConfigMap.size() - 1; i >= 0; i--)
    {
        auto const streamId = mImageConfigMap.keyAt(i);
        auto const it = usedStreamIds.find(streamId);
        if ( it == usedStreamIds.end() ) {
            //  remove unsued stream
            MY_LOGD("remove unused streamId:%02" PRIu64 "", streamId);
            mImageConfigMap.removeItemsAt(i);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
// auto
// ThisNamespace::
// addConfigStream(AppMetaStreamInfo* pStreamInfo) -> void
// {
//     MetaConfigItem item;
//     item.pStreamInfo = pStreamInfo;
//     mMetaConfigMap.add(pStreamInfo->getStreamId(), item);
// }


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getConfigImageStream(StreamId_T streamId) const -> android::sp<AppImageStreamInfo>
{
    Mutex::Autolock _l(mImageConfigMapLock);
    ssize_t const index = mImageConfigMap.indexOfKey(streamId);
    if  ( 0 <= index ) {
        return mImageConfigMap.valueAt(index).pStreamInfo;
    }
    return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
// auto
// ThisNamespace::
// getConfigMetaStream(StreamId_T streamId) const -> sp<AppStreamMgr::AppMetaStreamInfo>
// {
//     ssize_t const index = mMetaConfigMap.indexOfKey(streamId);
//     if  ( 0 <= index ) {
//         return mMetaConfigMap.valueAt(index).pStreamInfo;
//     }
//     else{
//         MY_LOGE("Cannot find MetaStreamInfo for stream %#" PRIx64 " in mMetaConfigMap", streamId);
//         return nullptr;
//     }
// }


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
setConfigMap(
    ImageConfigMap& imageConfigMap,
    MetaConfigMap& metaConfigMap
) -> void
{
    MY_LOGD("[debug] imageConfigMap.size()=%lu, metaConfigMap.size()=%lu",
                imageConfigMap.size(), metaConfigMap.size());
    Mutex::Autolock _l(mImageConfigMapLock);
    mImageConfigMap.clear();
    // mMetaConfigMap.clear();
    // for (size_t i = 0; i < metaConfigMap.size(); i++) {
    //     mMetaConfigMap.add(metaConfigMap.keyAt(i), metaConfigMap.valueAt(i));
    // }
    for (size_t i = 0; i < imageConfigMap.size(); i++) {
        mImageConfigMap.add(imageConfigMap.keyAt(i), ImageConfigItem{.pStreamInfo=imageConfigMap.valueAt(i)});
    }
    // MY_LOGD("[debug] mImageConfigMap.size()=%u, mMetaConfigMap.size()=%u",
    //             mImageConfigMap.size(), mMetaConfigMap.size());
    MY_LOGD("[debug] mImageConfigMap.size()=%lu", mImageConfigMap.size());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
registerFrame(Request const& rRequest) -> int
{
    sp<FrameParcel> pFrame = new FrameParcel;
    {
        Mutex::Autolock _l(mFrameQueueLock);
        mFrameQueue.push_back(pFrame);
    }
    pFrame->requestNo = rRequest.requestNo;
    {
        Mutex::Autolock _l(mShutterQueueLock);
        mShutterQueue.add(pFrame->requestNo, pFrame);
    }
    if (auto pTool = NSCam::Utils::LogTool::get()) {
        pTool->getCurrentLogTime(&pFrame->requestTimestamp);
    }
    //
    // mark zslStillCapture and reprocessing requests.
    auto& ctrlMeta = rRequest.vInputMetaBuffers.valueFor(eSTREAMID_META_APP_CONTROL);
    IMetadata* const pCtrlMeta = ctrlMeta->tryReadLock(LOG_TAG);
    if (pCtrlMeta != nullptr) {
        // mark zslStillCapture and reprocessing requests.
        MUINT8 enableZsl = 0;
        MUINT8 captureIntent = 0;
        bool enableZslEntry = IMetadata::getEntry<MUINT8>(
            pCtrlMeta, MTK_CONTROL_ENABLE_ZSL, enableZsl);
        if (!enableZslEntry) {
            MY_LOGE("enableZsl entry does not exist");
        }
        bool captureIntentEntry = IMetadata::getEntry<MUINT8>(
            pCtrlMeta, MTK_CONTROL_CAPTURE_INTENT, captureIntent);
        if (!captureIntentEntry) {
            MY_LOGE("captureIntent entry does not exist");
        }
        ctrlMeta->unlock(LOG_TAG, pCtrlMeta);
        bool zslStillCapture = (enableZsl == MTK_CONTROL_ENABLE_ZSL_TRUE) &&
                                (captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE);
        bool reprocessing = rRequest.vInputImageBuffers.size() > 0;
        // The order in Camera3OutputUtils is reprocessing -> zslStillCapture -> normal.
        if (reprocessing) {
            pFrame->eRequestType = RequestType::REPROCESSING;
        } else if (zslStillCapture) {
            pFrame->eRequestType = RequestType::ZSL_STILL_CAPTURE;
        }
        MY_LOGD_IF(zslStillCapture || reprocessing,
                "zslStillCapture=%d, reprocessing=%d, requestType=%d",
                zslStillCapture, reprocessing, pFrame->eRequestType);
    }
    else {
        MY_LOGE("pointer to pCtrlMeta is null");
    }
    //
    //  Request::vInputImageBuffers
    //  Request::vOutputImageBuffers
    {
        registerStreamBuffers(rRequest.vOutputImageBuffers, pFrame, &pFrame->vOutputImageItem);
        // registerStreamBuffers(rRequest.vOutputImageStreams, pFrame, &pFrame->vOutputImageItem);
        registerStreamBuffers(rRequest.vInputImageBuffers,  pFrame, &pFrame->vInputImageItem);
    }
    //
    //  Request::vInputMetaBuffers (Needn't register)
    //  Request::vOutputMetaBuffers
    {
        //registerStreamBuffers(rRequest.vOutputMetaBuffers, pFrame, &pFrame->vOutputMetaItem);
        registerStreamBuffers(rRequest.vInputMetaBuffers,  pFrame, &pFrame->vInputMetaItem);
    }
    //
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
registerStreamBuffers(
    android::KeyedVector<
        StreamId_T,
        android::sp<AppImageStreamBuffer>
                        > const& buffers,
    android::sp<FrameParcel> const pFrame,
    ImageItemSet*const pItemSet
)   -> int
{
    //  Request::v[Input|Output]ImageBuffers
    //  -> FrameParcel
    //  -> mImageConfigMap::vItemFrameQueue
    //  -> mImageConfigMap::pBufferHandleCache
    //
    Mutex::Autolock _l(mImageConfigMapLock);
    for (size_t i = 0; i < buffers.size(); i++)
    {
        sp<AppImageStreamBuffer> const pBuffer = static_cast<AppImageStreamBuffer*>(buffers[i].get());
        //
        StreamId_T const streamId = buffers.keyAt(i);
        //
        ssize_t const index = mImageConfigMap.indexOfKey(streamId);
        if  ( 0 > index ) {
            MY_LOGE("[requestNo:%u] bad streamId:%#" PRIx64, pFrame->requestNo, streamId);
            return NAME_NOT_FOUND;
        }

        {// add inflight ImageItem
            ImageItemFrameQueue& rItemFrameQueue = mImageConfigMap.editValueAt(index).vItemFrameQueue;
            //
            sp<ImageItem> pItem = new ImageItem;
            //
            rItemFrameQueue.push_back(pItem);
            //
            pItem->pFrame = pFrame.get();
            pItem->pItemSet = pItemSet;
            pItem->buffer = pBuffer;
            pItem->buffer_error = false;
            pItem->streamId = streamId;
            pItem->iter = --rItemFrameQueue.end();
            //
            pItemSet->add(streamId, pItem);
        }
    }
    //
    return OK;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateStreamBuffer(
        uint32_t requestNo,
        StreamId_T streamId,
        android::sp<IImageStreamBuffer> const pBuffer
)   -> int
{
    MY_LOGD_IF(mCommonInfo->mLogLevel>=1,
        "requestNo %d, streamId %" PRIu64 ", AppImageStreamBuffer %p", requestNo, streamId, pBuffer.get());
    {
        Mutex::Autolock _l(mImageConfigMapLock);
        // search framequeue and update the buffer pointers
        ssize_t const index = mImageConfigMap.indexOfKey(streamId);
        if(index<0) {
            MY_LOGW("requestNo %d, streamId %" PRIu64 " not found", requestNo, streamId);
            return NAME_NOT_FOUND;
        }

        ImageItemFrameQueue& rItemFrameQueue = mImageConfigMap.editValueAt(index).vItemFrameQueue;
        auto appBuffer = static_cast<AppImageStreamBuffer*>(pBuffer.get());

        for(ImageItemFrameQueue::iterator it=rItemFrameQueue.begin();it!=rItemFrameQueue.end(); it++) {
            sp<ImageItem> p = *it;
            if(p->pFrame && p->pFrame->requestNo == requestNo) {
                if(appBuffer) {
                    MY_LOGD_IF(mCommonInfo->mLogLevel>=1, "update buffer ptr=%p", appBuffer);
                    p->buffer = appBuffer;
                    p->buffer_error = false;
                    // cacheBuffer(streamId, appBuffer->getBufferId(),
                    //         *appBuffer->getImageBufferHeap()->getBufferHandlePtr(), appBuffer->mpAppBufferHandleHolder);
                }
                else {
                    MY_LOGD("Mark buffer_error. requestNo %d, streamId %" PRIu64, requestNo, streamId);
                    p->buffer_error = true;
                }
                return OK;
            }
        }
        return NAME_NOT_FOUND;
    }
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
registerStreamBuffers(
    android::KeyedVector<
        StreamId_T,
        android::sp<IImageStreamInfo>
                        > const& streams,
    android::sp<FrameParcel> const pFrame,
    ImageItemSet*const pItemSet
)   -> int
{
    //  Request::v[Input|Output]ImageBuffers
    //  -> FrameParcel
    //  -> mImageConfigMap::vItemFrameQueue
    //  -> mImageConfigMap::pBufferHandleCache
    //
    Mutex::Autolock _l(mImageConfigMapLock);
    for (size_t i = 0; i < streams.size(); i++)
    {
        sp<AppImageStreamInfo> const pInfo = static_cast<AppImageStreamInfo*>(streams[i].get());
        //
        StreamId_T const streamId = streams[i]->getStreamId();
        //
        ssize_t const index = mImageConfigMap.indexOfKey(streamId);
        if  ( 0 > index ) {
            MY_LOGE("[requestNo:%u] bad streamId:%#" PRIx64, pFrame->requestNo, streamId);
            return NAME_NOT_FOUND;
        }

        {// add inflight ImageItem
            ImageItemFrameQueue& rItemFrameQueue = mImageConfigMap.editValueAt(index).vItemFrameQueue;
            //
            sp<ImageItem> pItem = new ImageItem;
            //
            rItemFrameQueue.push_back(pItem);
            //
            pItem->pFrame = pFrame.get();
            pItem->pItemSet = pItemSet;
            pItem->buffer = nullptr; // it will modified later via updateStreamBuffer()
            pItem->buffer_error = false; // it will be true if allocate buffer fail
            pItem->streamId = streamId;
            pItem->iter = --rItemFrameQueue.end();
            //
            pItemSet->add(streamId, pItem);
        }
    }
    //
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
registerStreamBuffers(
    android::KeyedVector<
        StreamId_T,
        android::sp<IMetaStreamBuffer>
                        > const& buffers,
    android::sp<FrameParcel> const pFrame,
    MetaItemSet*const pItemSet
)   -> int
{
    //  Request::v[Input|Output]MetaBuffers
    //  -> FrameParcel
    //
    for (size_t i = 0; i < buffers.size(); i++)
    {
        sp<IMetaStreamBuffer> const pBuffer = buffers[i];
        //
        StreamId_T const streamId = buffers.keyAt(i);
        //
        sp<MetaItem> pItem = new MetaItem;
        pItem->pFrame = pFrame.get();
        pItem->pItemSet = pItemSet;
        pItem->buffer = pBuffer;
        //
        pItemSet->add(streamId, pItem);
    }
    //
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
MINT
ThisNamespace::
checkRequestError(FrameParcel const& frame)
{
    /**
     * @return
     *      ==0: uncertain
     *      > 0: it is indeed a request error
     *      < 0: it is indeed NOT a request error
     */
    //  It's impossible to be a request error if:
    //  1) any valid output image streams exist, or
    //  2) all valid output meta streams exist
    //
    //[NOT a request error]
    //
    MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
            "vOutputImageItem.numValidStreams %d vOutputMetaItem.numValidStreams %d vOutputMetaItem.size %d hasLastPartial %d ",
            (int)frame.vOutputImageItem.numValidStreams,
            (int)frame.vOutputMetaItem.numValidStreams,
            (int)frame.vOutputMetaItem.size(),
            (int)frame.vOutputMetaItem.hasLastPartial);
    MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
            "vOutputImageItem.numErrorStreams %d vOutputImageItem.size %d vOutputMetaItem.numErrorStreams %d",
            (int)frame.vOutputImageItem.numErrorStreams,
            (int)frame.vOutputImageItem.size(),
            (int)frame.vOutputMetaItem.numErrorStreams);

    if  (
            frame.vOutputImageItem.numValidStreams > 0
        ||  (frame.vOutputMetaItem.numValidStreams == frame.vOutputMetaItem.size()
        &&   frame.vOutputMetaItem.hasLastPartial)
        )
    {
        return -1;
    }
    //
    //[A request error]
    //
    if  (
            frame.vOutputImageItem.numErrorStreams == frame.vOutputImageItem.size()
        &&  frame.vOutputMetaItem.numErrorStreams > 0
        )
    {
        return 1;
    }
    //
    //[uncertain]
    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareErrorFrame(
    CallbackParcel& rCbParcel,
    android::sp<FrameParcel> const& pFrame
)   -> void
{
    rCbParcel.valid = MTRUE;
    //
    {
        CallbackParcel::Error error;
        error.errorCode = ErrorCode::ERROR_REQUEST;
        //
        rCbParcel.vError.add(error);
        //
    }
    //
    //Note:
    //FrameParcel::vInputImageItem
    //We're not sure whether input image streams are returned or not.
    //
    //FrameParcel::vOutputImageItem
    for (size_t i = 0; i < pFrame->vOutputImageItem.size(); i++) {
        prepareReturnImage(rCbParcel, pFrame->vOutputImageItem.valueAt(i));
    }
    //
    pFrame->errors.markBit(HistoryBit::ERROR_SENT_FRAME);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareErrorMetaIfPossible(
    CallbackParcel& rCbParcel,
    android::sp<MetaItem> const& pItem
)   -> void
{
    sp<FrameParcel> const pFrame = pItem->pFrame;
    if  ( ! pFrame->errors.hasBit(HistoryBit::ERROR_SENT_META) ) {
        pFrame->errors.markBit(HistoryBit::ERROR_SENT_META);
        //
        CallbackParcel::Error error;
        error.errorCode = ErrorCode::ERROR_RESULT;
        //
        rCbParcel.vError.add(error);
        rCbParcel.valid = MTRUE;
    }
    //
    //  Actually, this item will be set to NULL, and it is not needed for
    //  the following statements.
    //
    pItem->history.markBit(HistoryBit::ERROR_SENT_META);
    //
    if  ( 0 == pFrame->shutterTimestamp ) {
        MY_LOGW("[requestNo:%u] CAMERA3_MSG_ERROR_RESULT with shutter timestamp = 0", pFrame->requestNo);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareErrorImage(
    CallbackParcel& rCbParcel,
    android::sp<ImageItem> const& pItem
)   -> void
{
    rCbParcel.valid = MTRUE;
    //
    {
        Mutex::Autolock _l(mImageConfigMapLock);
        StreamId_T const streamId = pItem->streamId;
        ImageConfigItem const& rConfigItem = mImageConfigMap.valueFor(streamId);
        //
        CallbackParcel::Error error;
        error.errorCode = ErrorCode::ERROR_BUFFER;
        error.stream = rConfigItem.pStreamInfo;
        //
        rCbParcel.vError.add(error);
        MY_LOGW_IF(1, "(Error Status) streamId:%#" PRIx64 "(%s)", streamId, rConfigItem.pStreamInfo->getStreamName());
    }
    //
    //  Actually, this item will be set to NULL, and it is not needed for
    //  the following statements.
    //
    pItem->history.markBit(HistoryBit::ERROR_SENT_IMAGE);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareShutterNotificationIfPossible(
    CallbackParcel& rCbParcel,
    android::sp<MetaItem> const& pItem
)   -> bool
{
    sp<FrameParcel> const pFrame = pItem->pFrame;
    if ( false == pFrame->bShutterCallbacked && 0 != pFrame->shutterTimestamp )
    {
        pFrame->bShutterCallbacked = true;
        //
        rCbParcel.shutter = new CallbackParcel::Shutter;
        rCbParcel.shutter->timestamp = pFrame->shutterTimestamp;
        rCbParcel.valid = MTRUE;
        return MTRUE;
    }
    return MFALSE;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateShutterTimeIfPossible(
    android::sp<MetaItem> &pItem
)   -> void
{
    sp<FrameParcel> pFrame = pItem->pFrame;
    if  ( 0 == pFrame->shutterTimestamp ) {
        IMetadata* pMetadata = pItem->buffer->tryReadLock(LOG_TAG);
        if ( pMetadata ) {
            IMetadata::IEntry const eisEntry = pMetadata->entryFor(MTK_EIS_FEATURE_ISNEED_OVERRIDE_TIMESTAMP);
            MUINT8 needOverrideTime = 0;
            MUINT8 timeOverrided = 0;
            if(eisEntry.count() >= 2 && eisEntry.tag() == (MUINT32)MTK_EIS_FEATURE_ISNEED_OVERRIDE_TIMESTAMP){
                needOverrideTime = eisEntry.itemAt(0, Type2Type<MUINT8>());
                timeOverrided = eisEntry.itemAt(1, Type2Type<MUINT8>());
            }
            MBOOL timestampValid = (needOverrideTime == 0) || (needOverrideTime > 0 && timeOverrided > 0);

            // NOTE : VR Timestamp P2S will override by gralloc extra, and it need the shutter time send to framework
            // If shutter time choosing mechanism changed, please notify P2S owner.
            IMetadata::IEntry const entry = pMetadata->entryFor(MTK_SENSOR_TIMESTAMP);
            pItem->buffer->unlock(LOG_TAG, pMetadata);
            //
            if  ( timestampValid && ! entry.isEmpty() && entry.tag() == MTK_SENSOR_TIMESTAMP ) {
                MINT64 const timestamp = entry.itemAt(0, Type2Type<MINT64>());
                //
                pFrame->shutterTimestamp = timestamp;

            }
        }
    }
    return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareReturnMeta(
    CallbackParcel& rCbParcel,
    android::sp<MetaItem> const& pItem
)   -> void
{
    rCbParcel.valid = MTRUE;
    //
    {
        pItem->history.markBit(HistoryBit::RETURNED);
        pItem->pItemSet->numReturnedStreams++;
        //
        Vector<CallbackParcel::MetaItem>* pvCbItem = &rCbParcel.vOutputMetaItem;
        CallbackParcel::MetaItem& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
        rCbItem.buffer = pItem->buffer;

        if  ( pItem->bufferNo == mCommonInfo->mAtMostMetaStreamCount ) {
            rCbItem.bufferNo = mCommonInfo->mAtMostMetaStreamCount;
            //
//#warning "[FIXME] hardcode: REQUEST_PIPELINE_DEPTH=4"
            IMetadata::IEntry entry(MTK_REQUEST_PIPELINE_DEPTH);
            entry.push_back(4, Type2Type<MUINT8>());
            //
            if ( IMetadata* pMetadata = rCbItem.buffer->tryWriteLock(LOG_TAG) ) {
                pMetadata->update(MTK_REQUEST_PIPELINE_DEPTH, entry);
                rCbItem.buffer->unlock(LOG_TAG, pMetadata);
            }
        }
        else {
            rCbItem.bufferNo = pItem->pItemSet->numReturnedStreams;
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
preparePhysicReturnMeta(
    CallbackParcel& rCbParcel,
    android::sp<FrameParcel> const& rFrameParcel
)   -> void
{
    rCbParcel.valid = MTRUE;
    //
    for (size_t i = 0; i < rFrameParcel->vOutputPhysicMetaItem.size(); i++){
        sp<PhysicMetaItem> const& pItem = rFrameParcel->vOutputPhysicMetaItem.valueAt(i);
        //
        pItem->pItemSet->numReturnedStreams++;
        //
        MBOOL isExistingPhysicId = MFALSE;
        Vector<CallbackParcel::PhysicMetaItem>* pvCbItem = &rCbParcel.vOutputPhysicMetaItem;
        for (size_t i = 0; i < (*pvCbItem).size(); i++){
            if ( (*pvCbItem)[i].camId == pItem->physicCameraId ){
                // Append the current PhysicMetaItem to the existing one
                isExistingPhysicId = MTRUE;
                CallbackParcel::PhysicMetaItem& rCbItem = pvCbItem->editItemAt(i);
                {
                    IMetadata* pSrcMeta = pItem->buffer->tryReadLock(mInstanceName.c_str());
                    IMetadata* pDstMeta = rCbItem.buffer->tryWriteLock(mInstanceName.c_str());
                    if(pSrcMeta != nullptr && pDstMeta != nullptr){
                        //MUINT count = pSrcMeta->count();
                        (*pDstMeta) += (*pSrcMeta);
                    }
                    else{
                        MY_LOGW("Physical Meta %d is null: requestNo:%u, camId:%s, src:%p, dst:%p",
                                pItem->bufferNo, pItem->pFrame->requestNo, pItem->physicCameraId.c_str(),
                                pSrcMeta, pDstMeta);
                    }
                    rCbItem.buffer->unlock(mInstanceName.c_str(), pDstMeta);
                    pItem->buffer->unlock(mInstanceName.c_str(), pSrcMeta);
                }
                break;
            }
        }
        if ( ! isExistingPhysicId ){
            CallbackParcel::PhysicMetaItem& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
            rCbItem.camId = pItem->physicCameraId;
            rCbItem.buffer = pItem->buffer;

//#warning "[FIXME] hardcode: REQUEST_PIPELINE_DEPTH=4"
            IMetadata::IEntry entry(MTK_REQUEST_PIPELINE_DEPTH);
            entry.push_back(4, Type2Type<MUINT8>());
            //
            if ( IMetadata* pMetadata = rCbItem.buffer->tryWriteLock(LOG_TAG) ) {
                pMetadata->update(MTK_REQUEST_PIPELINE_DEPTH, entry);
                rCbItem.buffer->unlock(LOG_TAG, pMetadata);
            }
#if 1 //workaround for physical stream errors
            if ( rCbItem.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR) ){
                MY_LOGW("Workaround for physical stream errors in Android P, which should be fixed in Android Q");
                IMetadata::IEntry entry(MTK_SENSOR_TIMESTAMP);
                MINT64 timestamp = (MINT64)pItem->pFrame->shutterTimestamp;
                if ( timestamp == 0 ){
                    MY_LOGE("Physical metadata of requestNo:%u, camId:%s, callbacked with shutter timestamp is 0",
                              pItem->pFrame->requestNo, pItem->physicCameraId.c_str());
                }
                entry.push_back(timestamp, Type2Type< MINT64 >());
                if ( IMetadata* pMetadata = rCbItem.buffer->tryWriteLock(LOG_TAG) ){
                    pMetadata->update(MTK_SENSOR_TIMESTAMP, entry);
                    rCbItem.buffer->unlock(LOG_TAG, pMetadata);
                }
            }
#endif
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
isShutterReturnable(
    android::sp<MetaItem> const& pItem
)   const -> bool
{
    // check this shutter can return or not
    auto itFrame = mFrameQueue.begin();
    while ( itFrame != mFrameQueue.end() ) {
        if  ((*itFrame)->eRequestType == pItem->pFrame->eRequestType) {
            if ( (*itFrame)->requestNo != pItem->pFrame->requestNo &&
                (*itFrame)->bShutterCallbacked == false ) {
                MY_LOGW_IF(pItem->pFrame->shutterTimestamp != 0, "previous shutter (%u:%p) is not ready for requestNo:%u",
                        (*itFrame)->requestNo,(*itFrame).get(),pItem->pFrame->requestNo);
                return MFALSE;
            } else if ( (*itFrame)->requestNo == pItem->pFrame->requestNo ) {
                break;
            }
        }
        ++itFrame;
    }
    //
    return MTRUE;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
isReturnable(
    android::sp<MetaItem> const& pItem,
    android::String8& debug_log
)   const -> bool
{
    //1. check if any real-time partial arrives or not,
    //   if not, wait until last partial if the meta buffer is non-real time
    if ( bEnableMetaPending == 1 ){
        if ( ! pItem->pItemSet->hasLastPartial && pItem->pItemSet->realTimePartial == RealTime::NON ){
            return  MFALSE;
        }
        if ( bEnableMetaHitched == 1 && pItem->pItemSet->realTimePartial == RealTime::SOFT &&
            ! bHitchable ){
            return  MFALSE;
        }
    }

    //2. check return rules
    if (pItem->bufferNo == mCommonInfo->mAtMostMetaStreamCount) {
        //the final meta result to return must keep the order submitted.

        FrameQueue::const_iterator itFrame = mFrameQueue.begin();
        while (1)
        {
            sp<FrameParcel> const& pFrame = *itFrame;
            if (pFrame->eRequestType == pItem->pFrame->eRequestType) {
                if  ( pFrame == pItem->pFrame ) {
                    break;
                }
                //
//#warning "FIXME pFrame->vOutputMetaItem.isEmpty() in isReturnable()"
                if ( pFrame->vOutputMetaItem.isEmpty() ) {
                    MY_LOGW("[%d/%d] vOutputMetaItem:%zu", pFrame->requestNo, pItem->pFrame->requestNo, pFrame->vOutputMetaItem.size());
#if 0   // FIXME: this might happen in p1node bypass case
                    dumpLocked();
                    AEE_ASSERT("Skip requestNo %d.", pFrame->requestNo);
#endif
                    return  MFALSE;
                }
                MBOOL isAllMetaReturned = (pFrame->vOutputMetaItem.size() == pFrame->vOutputMetaItem.numReturnedStreams);
                if( (  pFrame->vOutputMetaItem.hasLastPartial && !isAllMetaReturned ) ||
                    ( !pFrame->vOutputMetaItem.hasLastPartial ))
                {
                    if ( debug_log.size() == 0 ){
                        debug_log += String8::format("Final meta blocked by requestNo:%u (%zu|%zu partial:%d) : ",
                                                        pFrame->requestNo,
                                                        pFrame->vOutputMetaItem.numReturnedStreams,
                                                        pFrame->vOutputMetaItem.size(),
                                                        pFrame->vOutputMetaItem.hasLastPartial
                                                    );
                    }
                    debug_log += String8::format("%u ", pItem->pFrame->requestNo);
                    return  MFALSE;
                }
            }
            //
            ++itFrame;
        }
    }
    //
    return MTRUE;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareReturnImage(
    CallbackParcel& rCbParcel,
    android::sp<ImageItem> const& pItem
)   -> void
{
    rCbParcel.valid = MTRUE;
    //
    if ( ! pItem->history.hasBit(HistoryBit::RETURNED) ) {
        Mutex::Autolock _l(mImageConfigMapLock);
        pItem->history.markBit(HistoryBit::RETURNED);
        pItem->pItemSet->numReturnedStreams++;
        //
        StreamId_T const streamId = pItem->streamId;
        ImageConfigItem& rConfigItem = mImageConfigMap.editValueFor(streamId);
        rConfigItem.vItemFrameQueue.erase(pItem->iter);
        //
        Vector<CallbackParcel::ImageItem>* pvCbItem = ( pItem->pItemSet->asInput )
                                                    ? &rCbParcel.vInputImageItem
                                                    : &rCbParcel.vOutputImageItem
                                                    ;
        CallbackParcel::ImageItem& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
        rCbItem.buffer = pItem->buffer;
        rCbItem.stream = rConfigItem.pStreamInfo;
        if (pItem->buffer == nullptr)
            return;

        //set the start timstamp of frame for preview buffers if needed
        if (bIsDejitterEnabled){
            int64_t timestamp = pItem->pFrame->startTimestamp;
            if ( timestamp != 0 ){
                MUINT64 consumerUsage = rConfigItem.pStreamInfo->getUsageForConsumer();
                if ( (consumerUsage & GRALLOC_USAGE_HW_TEXTURE) || (consumerUsage & GRALLOC_USAGE_HW_COMPOSER) ){
                    auto pImageBufferHeap = pItem->buffer->getImageBufferHeap();
                    if(pImageBufferHeap != nullptr && (pImageBufferHeap->getBufferHandlePtr()) != nullptr){
                        int rc = IGrallocHelper::singleton()->setBufferSOF(*pImageBufferHeap->getBufferHandlePtr(), (uint64_t)timestamp);
                        if ( OK != rc ){
                            MY_LOGE("Failed to set SOF:%" PRId64 " for streamId %#" PRIx64 "of requestNo %u", timestamp, streamId, pItem->pFrame->requestNo);
                        }
                    }
                }
            }
        }

        //CameraBlob
        if ( HAL_PIXEL_FORMAT_BLOB == static_cast<android_pixel_format_t>(rCbItem.stream->getStream().format)
        && ! rCbItem.buffer->hasStatus(STREAM_BUFFER_STATUS::ERROR) )
        {
            auto buffer = rCbItem.buffer;
            auto pImageBufferHeap = buffer->getImageBufferHeap();
            auto const dataspace = rCbItem.stream->getStream().dataSpace;

            std::string name{std::to_string(mCommonInfo->mInstanceId) + ":AppStreamMgr:FrameHandler"};

            if ( pItem->state != State::PRE_RELEASE ){
                mcam::GrallocStaticInfo staticInfo;
                buffer_handle_t pGraphicBufferHandle = pImageBufferHeap->getBufferHandlePtr() ?
                                                       *pImageBufferHeap->getBufferHandlePtr() : nullptr;
                if ( pGraphicBufferHandle == nullptr ) {
                    AEE_ASSERT("requestNo %u, get null buffer handle", pItem->pFrame->requestNo);
                    return;
                }
                int rc = IGrallocHelper::singleton()->query(pGraphicBufferHandle, &staticInfo, NULL);
                if  ( OK == rc && pImageBufferHeap->lockBuf(name.c_str(), GRALLOC_USAGE_SW_WRITE_OFTEN|GRALLOC_USAGE_SW_READ_OFTEN) ) {
                    MINTPTR jpegBuf = pImageBufferHeap->getBufVA(0);
                    size_t jpegDataSize = pImageBufferHeap->getBitstreamSize();
                    size_t jpegBufSize = staticInfo.widthInPixels;
                    CameraBlob* pTransport = reinterpret_cast<CameraBlob*>(jpegBuf + jpegBufSize - sizeof(CameraBlob));
                    if (dataspace == HAL_DATASPACE_V0_JFIF) // For Jpeg capture
                        pTransport->blobId = CameraBlobId::JPEG;
                    else if ((int)dataspace == (int)HAL_DATASPACE_JPEG_APP_SEGMENTS) // For Heic capture
                        pTransport->blobId = CameraBlobId::JPEG_APP_SEGMENTS;
                    pTransport->blobSize = jpegDataSize;
                    if ( ! pImageBufferHeap->unlockBuf(name.c_str()) ) {
                        MY_LOGE("failed on pImageBufferHeap->unlockBuf");
                    }
                    MY_LOGD("CameraBlob added: bufferId:%" PRIu64 " jpegBuf:%#" PRIxPTR " bufsize:%zu datasize:%zu", buffer->getBufferId(), jpegBuf, jpegBufSize, jpegDataSize);
                }
                else {
                    MY_LOGE("Fail to lock jpeg buffer - rc:%d", rc);
                }
            }
            else{
                MY_LOGD("Pre-release CameraBlob added: bufferId:%" PRIu64 "", buffer->getBufferId());
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
isReturnable(
    android::sp<ImageItem> const& pItem
)   const -> bool
{
    Mutex::Autolock _l(mImageConfigMapLock);

    StreamId_T const streamId = pItem->streamId;
    ImageItemFrameQueue const& rItemFrameQueue = mImageConfigMap.valueFor(streamId).vItemFrameQueue;
    //
    ImageItemFrameQueue::const_iterator it = rItemFrameQueue.begin();
    for (; it != pItem->iter; it++) {
        if  ( (*it)->pFrame->requestNo == pItem->pFrame->requestNo ) {
            break;
        }
        if  ( (*it)->pFrame->eRequestType == pItem->pFrame->eRequestType &&
              State::IN_FLIGHT == (*it)->state ) {
            return false;
        }
    }
    //
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
isFrameRemovable(
    android::sp<FrameParcel> const& pFrame
)   const -> bool
{
    //Not all output image streams have been returned.
    if  ( pFrame->vOutputImageItem.size() != pFrame->vOutputImageItem.numReturnedStreams ) {
        return MFALSE;
    }
    //
    //Not all input image streams have been returned.
    if  ( pFrame->vInputImageItem.size() != pFrame->vInputImageItem.numReturnedStreams ) {
        return MFALSE;
    }
    //
    //
    if  ( pFrame->errors.hasBit(HistoryBit::ERROR_SENT_FRAME) ) {
        //frame error was sent.
        return MTRUE;
    }
    else
    if  ( pFrame->errors.hasBit(HistoryBit::ERROR_SENT_META) ) {
        //meta error was sent.
        if  ( 0 == pFrame->shutterTimestamp ) {
            MY_LOGW("[requestNo:%u] shutter not sent with meta error", pFrame->requestNo);
        }
    }
    else {
        //Not all meta streams have been returned.
        MBOOL isAllMetaReturned = (pFrame->vOutputMetaItem.size() == pFrame->vOutputMetaItem.numReturnedStreams) &&
                                    (pFrame->vOutputPhysicMetaItem.size() == pFrame->vOutputPhysicMetaItem.numReturnedStreams);
        if( !pFrame->vOutputMetaItem.hasLastPartial || !isAllMetaReturned ) {
            return MFALSE;
        }
        //
        //No shutter timestamp has been sent.
        if  ( 0 == pFrame->shutterTimestamp ) {
            MY_LOGW("[requestNo:%u] shutter not sent @ no meta error", pFrame->requestNo);
            return MFALSE;
        }
    }
    //
    return MTRUE;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareCallbackIfPossible(
    CallbackParcel& rCbParcel,
    MetaItemSet& rItemSet,
    android::String8& debug_log
)   -> bool
{
    MBOOL anyUpdate = MFALSE;
    //
    for ( size_t i = 0; i < rItemSet.size(); i++ )
    {
        sp<MetaItem> pItem = rItemSet[i];
        if  ( pItem == 0 ) {
            continue;
        }
        //
        sp<FrameParcel> const pFrame = pItem->pFrame;
        //
        switch  ( pItem->state )
        {
        case State::VALID:{
            //Valid Buffer but Not Returned
            if  ( ! pItem->history.hasBit(HistoryBit::RETURNED) ) {
                // separate shutter and metadata
                updateShutterTimeIfPossible(pItem);
                if ( isShutterReturnable(pItem) &&
                    prepareShutterNotificationIfPossible(rCbParcel, pItem) ) {
                    anyUpdate = MTRUE;
                    //update the shutter queue
                    {
                        Mutex::Autolock _l(mShutterQueueLock);
                        mShutterQueue.removeItem(pFrame->requestNo);
                    }
                }
                if  ( isReturnable(pItem, debug_log) ) {
                    prepareReturnMeta(rCbParcel, pItem);
                    if ( pItem-> bufferNo == mCommonInfo->mAtMostMetaStreamCount &&
                         pFrame->vOutputPhysicMetaItem.size() > 0 ){
                        preparePhysicReturnMeta(rCbParcel, pItem->pFrame);
                    }
                    anyUpdate = MTRUE;
                }
            }
            }break;
            //
        case State::ERROR:{
            //Error Buffer but Not Error Sent Yet
            if  ( ! pItem->history.hasBit(HistoryBit::ERROR_SENT_META) ) {
                if  ( checkRequestError(*pFrame) < 0 ) {
                    //Not a request error
                    prepareErrorMetaIfPossible(rCbParcel, pItem);
                    anyUpdate = MTRUE;
                }
                else {
                    MY_LOGD("requestNo:%u Result Error Pending", pFrame->requestNo);
                }
            }
            }break;
            //
        default:
            break;
        }
        //
        MBOOL const needRelease =
              ( (pItem->buffer && pItem->buffer->haveAllUsersReleased() == OK) // all users released
              )
            &&( pItem->history.hasBit(HistoryBit::RETURNED)
              ||pItem->history.hasBit(HistoryBit::ERROR_SENT_FRAME)
              ||pItem->history.hasBit(HistoryBit::ERROR_SENT_META)
              ||pItem->history.hasBit(HistoryBit::ERROR_SENT_IMAGE)
              ) ;
        if  ( needRelease ) {
            rItemSet.editValueAt(i) = NULL;
        }
    }
    //
    return anyUpdate;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareCallbackIfPossible(
    CallbackParcel& rCbParcel,
    ImageItemSet& rItemSet
)   -> bool
{
    MBOOL anyUpdate = MFALSE;
    //
    for ( size_t i = 0; i < rItemSet.size(); i++ )
    {
        sp<ImageItem> pItem = rItemSet[i];
        if  ( pItem == nullptr ) {
            continue;
        }
        //
        sp<FrameParcel> const pFrame = pItem->pFrame;
        //
        switch  ( pItem->state )
        {
        case State::PRE_RELEASE:{
            //Pre-Release but Not Returned
            if  ( ! pItem->history.hasBit(HistoryBit::RETURNED) ) {
                if  ( isReturnable(pItem) ) {
                    prepareReturnImage(rCbParcel, pItem);
                    anyUpdate = MTRUE;
                }
            }
            }break;
            //
        case State::VALID:{
            //Valid Buffer but Not Returned
            if  ( ! pItem->history.hasBit(HistoryBit::RETURNED) ) {
                if  ( isReturnable(pItem) ) {
                    prepareReturnImage(rCbParcel, pItem);
                    anyUpdate = MTRUE;
                }
            }
            }break;
            //
        case State::ERROR:{
            //Valid Buffer but Not Returned
            if  ( ! pItem->history.hasBit(HistoryBit::RETURNED) ) {
                if  ( checkRequestError(*pFrame) < 0 ) {
                    //Not a request error
                    if  ( isReturnable(pItem) ) {
                        if  ( ! pItem->history.hasBit(HistoryBit::ERROR_SENT_IMAGE) ) {
                            //Error Buffer but Not Error Sent Yet
                            prepareErrorImage(rCbParcel, pItem);
                        }
                        prepareReturnImage(rCbParcel, pItem);
                        anyUpdate = MTRUE;
                    }
                }
                else if ( CC_LIKELY(pItem->buffer && pItem->buffer->getStreamInfo() != nullptr) ){
                    MY_LOGV("requestNo:%u Buffer Error Pending, streamId:%#" PRIx64, pFrame->requestNo, pItem->buffer->getStreamInfo()->getStreamId());
                }
            }
            }break;
            //
        default:
            break;
        }
        //
        MBOOL const needRelease =
              ( (pItem->buffer && pItem->buffer->haveAllUsersReleased() == OK) // all users released
               ||pItem->buffer_error // buffer error
              )
            &&( pItem->history.hasBit(HistoryBit::RETURNED)
              ||pItem->history.hasBit(HistoryBit::ERROR_SENT_FRAME)
              ||pItem->history.hasBit(HistoryBit::ERROR_SENT_META)
              ||pItem->history.hasBit(HistoryBit::ERROR_SENT_IMAGE)
              ) ;
        if  ( needRelease ) {
            rItemSet.editValueAt(i) = NULL;
        }
    }
    //
    return anyUpdate;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateItemSet(MetaItemSet& rItemSet) -> void
{
    auto pReleaseHandler = [=](
            StreamId_T const        streamId,
            MetaItem* const         pItem,
            IMetaStreamBuffer*      pStreamBuffer
        )
        {
            MBOOL const isError = pStreamBuffer->hasStatus(STREAM_BUFFER_STATUS::ERROR);
            if  ( isError ) {
                // RELEASE & ERROR BUFFER
                pItem->state = State::ERROR;
                pItem->pItemSet->numErrorStreams++;
                MY_LOGW(
                    "[Meta Stream Buffer] Error happens..."
                    " - requestNo:%u streamId:%#" PRIx64 " %s",
                    pItem->pFrame->requestNo, streamId, pStreamBuffer->getName()
                );
            }
            else {
                // RELEASE & VALID BUFFER
                pItem->state = State::VALID;
                pItem->pItemSet->numValidStreams++;
            }
        };

    for (size_t i = 0; i < rItemSet.size(); i++)
    {
        StreamId_T const streamId = rItemSet.keyAt(i);
        sp<MetaItem> pItem = rItemSet.valueAt(i);
        if  ( pItem == 0 ) {
            //MY_LOGV("Meta streamId:%#" PRIx64 " NULL MetaItem", streamId);
            continue;
        }
        //
        if  ( State::VALID != pItem->state && State::ERROR != pItem->state )
        {
            sp<IMetaStreamBuffer> pStreamBuffer = pItem->buffer;
            //
            if  (
                    ( pStreamBuffer->getStreamInfo()
                &&    pStreamBuffer->getStreamInfo()->getStreamType() != eSTREAMTYPE_META_IN
                    )
                &&  ( pStreamBuffer->haveAllProducerUsersReleased() == OK
                      || pStreamBuffer->haveAllProducerUsersReleasedOrPreReleased() == OK
                    )
                )
            {
                pReleaseHandler(streamId, pItem.get(), pStreamBuffer.get());
            }
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateItemSet(PhysicMetaItemSet& rItemSet) -> void
{
    auto pReleaseHandler = [=](
            StreamId_T const        streamId,
            PhysicMetaItem* const pItem,
            IMetaStreamBuffer*      pStreamBuffer
        )
        {
            MBOOL const isError = pStreamBuffer->hasStatus(STREAM_BUFFER_STATUS::ERROR);
            if  ( isError ) {
                // RELEASE & ERROR BUFFER
                pItem->state = State::ERROR;
                pItem->pItemSet->numErrorStreams++;
                MY_LOGW(
                    "[Physical Meta Stream Buffer] Error happens..."
                    " - requestNo:%u streamId:%#" PRIx64 " %s",
                    pItem->pFrame->requestNo, streamId, pStreamBuffer->getName()
                );
            }
            else {
                // RELEASE & VALID BUFFER
                pItem->state = State::VALID;
                pItem->pItemSet->numValidStreams++;
            }
        };

    for (size_t i = 0; i < rItemSet.size(); i++)
    {
        StreamId_T const streamId = rItemSet.keyAt(i);
        sp<PhysicMetaItem> pItem = rItemSet.valueAt(i);
        if  ( pItem == 0 ) {
            //MY_LOGV("Physical Meta streamId:%#" PRIx64 " NULL PhysicMetaItem", streamId);
            continue;
        }
        //
        if  ( State::VALID != pItem->state && State::ERROR != pItem->state )
        {
            sp<IMetaStreamBuffer> pStreamBuffer = pItem->buffer;
            //
            if ( CC_LIKELY(pStreamBuffer != nullptr && pStreamBuffer->getStreamInfo() != nullptr)
                &&  pStreamBuffer->getStreamInfo()->getStreamType() != eSTREAMTYPE_META_IN
                &&  pStreamBuffer->haveAllProducerUsersReleased() == OK
                )
            {
                pReleaseHandler(streamId, pItem.get(), pStreamBuffer.get());
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateItemSet(ImageItemSet& rItemSet) -> void
{
    auto pReleaseHandler = [=](
            StreamId_T const        streamId,
            ImageItem* const        pItem,
            AppImageStreamBuffer*   pStreamBuffer
        )
        {
            MBOOL const isError = pStreamBuffer == nullptr || pStreamBuffer->hasStatus(STREAM_BUFFER_STATUS::ERROR);
            if  ( isError ) {
                const char* name = "nullptr";
                if(pStreamBuffer)
                    name = pStreamBuffer->getName();

                MY_LOGW(
                    "[Image Stream Buffer] Error happens and all users release"
                    " - requestNo:%u streamId:%#" PRIx64 " %s state:%s->ERROR",
                    pItem->pFrame->requestNo, streamId, name, (( State::IN_FLIGHT == pItem->state ) ? "IN-FLIGHT" : "PRE-RELEASE")
                );
                //We should:
                //  RF = ( ACQUIRE ) ? -1 : AF
                //For simplity, however, no matter if acquire_fence was
                //waited on or not, we just:
                //  RF = AF
                if(pStreamBuffer) {
                    MINT AF = pStreamBuffer->getAcquireFence();
                    MINT dupAF = -1;
                    if (AF != -1)
                    {
                        dupAF = ::dup(AF);
                    }
                    pStreamBuffer->setReleaseFence(dupAF);
                    pStreamBuffer->setAcquireFence(-1);
                }
                //
                pItem->state = State::ERROR;
                pItem->pItemSet->numErrorStreams++;
                MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
                        "pReleaseHandler streamId %d numErrorStreams++ %d", (int)streamId, (int)pItem->pItemSet->numErrorStreams);
            }
            else {
                pStreamBuffer->setReleaseFence(-1);
                pStreamBuffer->setAcquireFence(-1);
                //
                pItem->state = State::VALID;
                pItem->pItemSet->numValidStreams++;
                MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
                        "pReleaseHandler streamId %d numValidStreams++ %d", (int)streamId, (int)pItem->pItemSet->numValidStreams);
            }
        };

    auto pPreReleaseHandler = [=](
            StreamId_T const        streamId,
            ImageItem* const        pItem,
            AppImageStreamBuffer*   pStreamBuffer
        )
        {
            MY_LOGD("[Image Stream Buffer] pre-release"
                    " - requestNo:%u streamId:%#" PRIx64 " bufferId:%" PRIu64 " %s state:IN-FLIGHT->PRE-RELEASE",
                pItem->pFrame->requestNo, streamId, pStreamBuffer->getBufferId(), pStreamBuffer->getName()
            );
            //mark the buffer would be freed by others, as there's still user need to use it
            Mutex::Autolock _l(mImageConfigMapLock);
            ssize_t const index = mImageConfigMap.indexOfKey(streamId);
            if ( index > 0 ){
                pStreamBuffer->mpAppBufferHandleHolder->freeByOthers = true;
                MY_LOGD("bufferId:%" PRIu64 " buffer:%p, would be freed by others",
                        pStreamBuffer->getBufferId(), pStreamBuffer->mpAppBufferHandleHolder->bufferHandle);
                // auto pBufferHandleCache = mImageConfigMap.valueAt(index).pBufferHandleCache;
                // pBufferHandleCache->markBufferFreeByOthers(pStreamBuffer->getBufferId());
            }
            else{
                MY_LOGE("Cannot find the streamId:%#" PRIx64 " in mImageConfigMap", streamId);
            }
            //
            pStreamBuffer->setReleaseFence(-1);
            pStreamBuffer->setAcquireFence(-1);
            pItem->state = State::PRE_RELEASE;
            pItem->pItemSet->numValidStreams++;
            MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
                    "pPreReleaseHandler streamId %d numValidStreams++ %d", (int)streamId, (int)pItem->pItemSet->numValidStreams);
        };

    for (size_t i = 0; i < rItemSet.size(); i++)
    {
        StreamId_T const streamId = rItemSet.keyAt(i);
        sp<ImageItem> pItem = rItemSet.valueAt(i);
        if  ( pItem == 0 ) {
            //MY_LOGV("Image streamId:%#" PRIx64 " NULL ImageItem", streamId);
            continue;
        }
        //
        switch  (pItem->state)
        {
        case State::IN_FLIGHT:{
            MUINT32 allUsersStatus = 0;// default status is None

            if(pItem->buffer) // Status available when buffer is ready
                allUsersStatus = pItem->buffer->getAllUsersStatus();
            else if(pItem->buffer_error)
                allUsersStatus = IUsersManager::UserStatus::RELEASE;

            MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
                    "IN_FLIGHT allUsersStatus %d", allUsersStatus);
            //  IN_FLIGHT && all users release ==> VALID/ERROR
            if ( allUsersStatus == IUsersManager::UserStatus::RELEASE )
            {
                pReleaseHandler(streamId, pItem.get(), pItem->buffer.get());
            }
            //
            //  IN-IN_FLIGHT && all users release or pre-release ==> PRE_RELEASE
            else
            if ( allUsersStatus == IUsersManager::UserStatus::PRE_RELEASE )
            {
                pPreReleaseHandler(streamId, pItem.get(), pItem->buffer.get());
            }
            }break;
            //
        case State::PRE_RELEASE:{
            //  PRE_RELEASE && all users release ==> VALID/ERROR
            MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
                    "PRE_RELEASE haveAllUsersReleased %d", pItem->buffer->haveAllUsersReleased() );
            if  ( OK == pItem->buffer->haveAllUsersReleased() )
            {
                pReleaseHandler(streamId, pItem.get(), pItem->buffer.get());
            }
            }break;
            //
        default:
            break;
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateResult(ResultQueueT const& rvResult) -> void
{
    bHitchable = false;
    if  ( mFrameQueue.empty() ) {
        MY_LOGD("Empty FrameQueue:%p %p", &mFrameQueue, this);
        return;
    }
    //
    for (size_t iResult = 0; iResult < rvResult.size(); iResult++)
    {
        FrameQueue::iterator itFrame = mFrameQueue.begin();
        android::sp<ResultItem> resultItem = rvResult.valueAt(iResult);
        if(resultItem == nullptr)
        {
            MY_LOGE("NULL ResultItem, need check user update");
        }
        MUINT32 const requestNo = resultItem->requestNo;
        for (; itFrame != mFrameQueue.end(); itFrame++) {
            //
            sp<FrameParcel>& pFrame = *itFrame;
            if  ( requestNo != pFrame->requestNo ) {
                continue;
            }
            // update the timestamp start of the frame if needed
            if ( resultItem->timestampStartOfFrame != 0){
                pFrame->startTimestamp = resultItem->timestampStartOfFrame;
            }
            // put output meta into vOutputMetaItem
            sp<MetaItem> pMetaItem = nullptr;
            if ( resultItem->buffer.size() != 0 ){
                MetaItemSet* pItemSet = &pFrame->vOutputMetaItem;
                pItemSet->realTimePartial = resultItem->realTimePartial;
                Vector< sp<IMetaStreamBuffer> >::iterator it = resultItem->buffer.begin();
                for(; it != resultItem->buffer.end(); it++)
                {
                    sp<IMetaStreamBuffer> const pBuffer = *it;
                    //
                    if( CC_LIKELY(pBuffer != nullptr && pBuffer->getStreamInfo() != nullptr) ){
                        StreamId_T const streamId = pBuffer->getStreamInfo()->getStreamId();
                        //
                        sp<MetaItem> pItem = new MetaItem;
                        pItem->pFrame = pFrame.get();
                        pItem->pItemSet = pItemSet;
                        pItem->buffer = pBuffer;
                        pItem->bufferNo = pItemSet->size() + 1;
                        pMetaItem = pItem;
                        //
                        pItemSet->add(streamId, pItem);
                    }
                }
            }

            // put physical output meta into vOutputPhysicMetaItem
            sp<PhysicMetaItem> pPhysicMetaItem = nullptr;
            if ( resultItem->physicBuffer.size() > 0 ){
                PhysicMetaItemSet* pPhysicItemSet = &pFrame->vOutputPhysicMetaItem;
                //update for each physical metadata
                for (size_t i = 0; i < resultItem->physicBuffer.size(); i++){
                    std::string camId = resultItem->physicBuffer.keyAt(i);
                    //
                    Vector< sp<IMetaStreamBuffer> > bufVec = resultItem->physicBuffer.valueAt(i);
                    Vector< sp<IMetaStreamBuffer> >::iterator it = bufVec.begin();
                    if ( it == nullptr ) {
                        AEE_ASSERT("nullptr IMetaStreamBuffer");
                        return;
                    }
                    for(; it != bufVec.end(); it++)
                    {
                        sp<IMetaStreamBuffer> const pBuffer = *it;
                        if ( pBuffer ) {
                            StreamId_T const streamId = pBuffer->getStreamInfo()->getStreamId();
                            //
                            sp<PhysicMetaItem> pItem = new PhysicMetaItem;
                            pItem->pFrame = pFrame.get();
                            pItem->pItemSet = pPhysicItemSet;
                            pItem->buffer = pBuffer;
                            pItem->bufferNo = pPhysicItemSet->size() + 1;
                            pItem->physicCameraId = camId;
                            pPhysicMetaItem = pItem;
                            //
                            pPhysicItemSet->add(streamId, pItem);
                        }
                        else {
                            MY_LOGE("get null phy meta buffer from user");
                        }
                    }
                }
            }

            // update for last partial
            if ( resultItem -> lastPartial ){
                if ( resultItem->physicBuffer.size() > 0 || resultItem->buffer.size() > 0 ){
                    pFrame->vOutputMetaItem.hasLastPartial = true;
                    bHitchable = true;
                    //
                    if ( pMetaItem == nullptr || pMetaItem->pItemSet->realTimePartial != RealTime::NON){
                        // last partial from pipeline has only physical metadata
                        // so we need to create a new logical metadata for physical meta callback
                        //
                        // create a new MetaStreamBuffer
                        sp<IMetaStreamInfo> pStreamInfo = new AppMetaStreamInfo(
                                                              "Meta:App:Last",
                                                              eSTREAMID_END_OF_INTERNAL,
                                                              eSTREAMTYPE_META_OUT,
                                                              0
                                                          );
                        sp<AppMetaStreamBuffer> pLastMetaBuffer = AppMetaStreamBuffer::Allocator(pStreamInfo.get())();
                        if ( pLastMetaBuffer == nullptr ) {
                            MY_LOGF("allocate dummy meta stream buffer failed");
                        }
                        pLastMetaBuffer->finishUserSetup();

                        // create a new MetaItem
                        StreamId_T const streamId = eSTREAMID_END_OF_INTERNAL;
                        sp<MetaItem> pItem = new MetaItem;
                        pItem->pFrame = pFrame.get();
                        pItem->pItemSet = &pFrame->vOutputMetaItem;
                        pItem->buffer = pLastMetaBuffer;
                        pItem->bufferNo = pFrame->vOutputMetaItem.size() + 1;
                        //
                        pFrame->vOutputMetaItem.add(streamId, pItem);
                        //
                        pMetaItem = pItem;
                    }
                    pMetaItem->bufferNo = mCommonInfo->mAtMostMetaStreamCount;
                    //
                    if (pFrame->vOutputPhysicMetaItem.size() > 0){
                        auto& lastPhysicMetaItem = pFrame->vOutputPhysicMetaItem.editValueAt(pFrame->vOutputPhysicMetaItem.size() - 1);
                        lastPhysicMetaItem->bufferNo = mCommonInfo->mAtMostMetaStreamCount;
                    }
                }
            }
            updateItemSet(pFrame->vOutputMetaItem);
            updateItemSet(pFrame->vOutputPhysicMetaItem);
            updateItemSet(pFrame->vOutputImageItem);
            updateItemSet(pFrame->vInputImageItem);
            MY_LOGD_IF((resultItem->realTimePartial != RealTime::NON) || resultItem->lastPartial,
                       "requestNo %u, real time:%u, last partial:%d",
                       requestNo, resultItem->realTimePartial, resultItem->lastPartial);
            break;
        }
        //
        if  ( itFrame == mFrameQueue.end() ) {
            if ( (resultItem->buffer).size() != 0 ) {
                String8 log = String8::format("requestNo:%u is not in FrameQueue; FrameQueue:", requestNo);
                for (auto const& v : mFrameQueue) {
                    log += String8::format(" %u", v->requestNo);
                }
                log += String8("; ResultQueue:");
                for (size_t v = 0; v < rvResult.size(); v++) {
                    log += String8::format(" %u", rvResult.valueAt(v)->requestNo);
                }
                MY_LOGW("%s", log.c_str());
            }
            itFrame = mFrameQueue.begin();
        }
    }
    //
    MUINT32 const latestResultRequestNo = rvResult.valueAt(rvResult.size() - 1)->requestNo;
    if  (0 < (MINT32)(latestResultRequestNo - mFrameQueue.latestResultRequestNo) ) {
        mFrameQueue.latestResultRequestNo = latestResultRequestNo;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateCallback(std::list<CallbackParcel>& rCbList) -> void
{
    String8 returnableDebugLog("");
    //
    FrameQueue::iterator itFrame = mFrameQueue.begin();
    while ( itFrame != mFrameQueue.end() )
    {
        MUINT32 const requestNo = (*itFrame)->requestNo;
        if  ( 0 < (MINT32)(requestNo - mFrameQueue.latestResultRequestNo) ) {
            MY_LOGV("stop updating frame => requestNo: this(%u) > latest(%u) ", requestNo, mFrameQueue.latestResultRequestNo);
            break;
        }
        //
        CallbackParcel cbParcel;
        cbParcel.valid = MFALSE;
        cbParcel.requestNo = requestNo;
        cbParcel.timestampShutter = (*itFrame)->shutterTimestamp;
        if((*itFrame)->vOutputImageItem.numReturnedStreams == (*itFrame)->vOutputImageItem.size()
            && (*itFrame)->vInputImageItem.numReturnedStreams == (*itFrame)->vInputImageItem.size()
            && ((*itFrame)->vOutputMetaItem.numErrorStreams > 0 || (*itFrame)->vInputMetaItem.numErrorStreams > 0)){
            // If Framework has received some error and all image returned, it will clear records
            // If meta or shutter callback after Framework clear record, it will cause fatal error.
            // After Android O1, framework can not receive callback if all buffers return and :
            //    (a) received error result meta
            //    (b) received request error ( our checkRequestError() already handle this)
            cbParcel.needIgnore = MTRUE;
            MY_LOGI("requestNo (%d) need ignore callback", requestNo);
        }
        //
        if  ( checkRequestError(**itFrame) > 0 && ! (*itFrame)->errors.hasBit(HistoryBit::ERROR_SENT_FRAME) ) {
            //It's a request error
            //Here we're still not sure that the input image stream is returned or not.
            MY_LOGW("requestNo:%u Request Error", (*itFrame)->requestNo);
            prepareErrorFrame(cbParcel, *itFrame);
        }
        else {
            if ( ! (*itFrame)->errors.hasBit(HistoryBit::ERROR_SENT_META) ) {
                prepareCallbackIfPossible(cbParcel, (*itFrame)->vOutputMetaItem, returnableDebugLog);
            }
            prepareCallbackIfPossible(cbParcel, (*itFrame)->vOutputImageItem);
        }
        prepareCallbackIfPossible(cbParcel, (*itFrame)->vInputImageItem);

        //
        if  ( isFrameRemovable(*itFrame) ) {
            //remove this frame from the frame queue.
            CAM_ULOG_EXIT_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, (*itFrame)->requestNo);
            cbParcel.needRemove = MTRUE;
            itFrame = mFrameQueue.erase(itFrame);
        }
        else {
            //next iteration
            itFrame++;
        }
        //
        if  ( cbParcel.valid ) {
            rCbList.push_back(cbParcel);
        }
        //check if there's another pending shutters can be callbacked
        while ( ! mShutterQueue.isEmpty() ){
            android::sp<FrameParcel> frameParcel = nullptr;
            {
                Mutex::Autolock _l(mShutterQueueLock);
                frameParcel = mShutterQueue.valueAt(0).promote();
            }
            if( frameParcel == nullptr ){
                MY_LOGW("requestNo %u is removed before shutter callbacked, there MUST have error frames or meta be sent for the request", mShutterQueue.keyAt(0));
                mShutterQueue.removeItemsAt(0);
            }
            else if ( false == frameParcel->bShutterCallbacked && 0 != frameParcel->shutterTimestamp ){
                //shutter for current frame is ready
                frameParcel->bShutterCallbacked = true;
                //
                CallbackParcel shutterCbParcel;
                shutterCbParcel.requestNo = frameParcel->requestNo;
                shutterCbParcel.needIgnore = MFALSE;
                shutterCbParcel.shutter = new CallbackParcel::Shutter;
                shutterCbParcel.shutter->timestamp = frameParcel->shutterTimestamp;
                shutterCbParcel.valid = MTRUE;
                //
                rCbList.push_back(shutterCbParcel);
                {
                    Mutex::Autolock _l(mShutterQueueLock);
                    mShutterQueue.removeItemsAt(0);
                }
            }
            else
                break;
        }
    }
    //
    MY_LOGW_IF(returnableDebugLog.size() != 0, "%s", returnableDebugLog.c_str());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
update(ResultQueueT const& rvResult) -> void
{
    std::list<CallbackParcel> cbList;
    {
        Mutex::Autolock _l(mFrameQueueLock);
        removeEmptyResult(const_cast<ResultQueueT&>(rvResult));
        if ( rvResult.size() == 0) {
            return;
        }
        updateResult(rvResult);
        updateCallback(cbList);
    }
    if ( cbList.size() )
        mBatchHandler->push(cbList);

    if  ( mFrameQueue.empty() ) {
        mFrameQueueCond.broadcast();
    }
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
        while ( ! queue.empty() )
        {
            err = cond.waitRelative(lock, timeoutToWait());
            if  ( OK != err ) {
                break;
            }
        }
        //
        if  ( queue.empty() ) { return OK; }
        return err;
    };
    //
    //
    int err = OK;
    if  ( OK != (err = waitEmpty(mFrameQueueLock, mFrameQueueCond, mFrameQueue)))
    {
        Mutex::Autolock _l(mFrameQueueLock);
        MY_LOGW(
            "mFrameQueue:#%zu timeout(ns):%" PRId64 " elapsed(ns):%" PRId64 " err:%d(%s)",
            mFrameQueue.size(), timeout, (::systemTime() - startTime), err, ::strerror(-err)
        );
        dumpLocked();
    }
    //
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
ThisNamespace::
removeEmptyResult(ResultQueueT & rvResult)
{
    for (size_t iResult = 0; iResult < rvResult.size(); iResult++)
    {
        FrameQueue::iterator itFrame = mFrameQueue.begin();
        android::sp<ResultItem> resultItem = rvResult.valueAt(iResult);
        if(resultItem == nullptr)
        {
            MY_LOGE("NULL ResultItem, need check user update");
        }
        MUINT32 const requestNo = resultItem->requestNo;
        bool bHasMeta    = !resultItem->buffer.isEmpty();
        bool bHasPhyMeta = !resultItem->physicBuffer.isEmpty();
        bool bHasImg     = false;
        for (; itFrame != mFrameQueue.end(); itFrame++) {
            //
            sp<FrameParcel>& pFrame = *itFrame;
            if  ( requestNo != pFrame->requestNo ) {
                continue;
            }
            for (size_t i = 0; i < pFrame->vOutputImageItem.size(); i++) {
                android::sp<AppImageStreamBuffer> pBuffer = pFrame->vOutputImageItem.valueAt(i) ?
                                                        pFrame->vOutputImageItem.valueAt(i)->buffer : nullptr;
                if ( pBuffer && pBuffer->getAllUsersStatus() != 0)
                        bHasImg = true;
            }
            for (size_t i = 0; i < pFrame->vInputImageItem.size(); i++) {
                android::sp<AppImageStreamBuffer> pBuffer = pFrame->vOutputImageItem.valueAt(i) ?
                                                        pFrame->vOutputImageItem.valueAt(i)->buffer : nullptr;
                if ( pBuffer && pBuffer->getAllUsersStatus() != 0)
                        bHasImg = true;
            }
        }
        // remove empty result
        if ( !bHasMeta && !bHasPhyMeta && !bHasImg ) {
            MY_LOGD_IF(mCommonInfo->mLogLevel >= 1,
                       "remove reultItem, requestNo %u", resultItem->requestNo);
            rvResult.removeItemsAt(iResult);
        }
    }
    if ( rvResult.isEmpty() )
        return true;
    return false;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpStateLocked(android::Printer& printer) const -> void
{
    auto logImage = [&](char const* tag, ImageItemSet const& rItems) {
        String8 strCaption = String8::format("      %s - return#:%zu valid#:%zu error#:%zu",
            tag, rItems.numReturnedStreams, rItems.numValidStreams, rItems.numErrorStreams);
        printer.printLine(strCaption.c_str());
        for (size_t i = 0; i < rItems.size(); i++) {
            StreamId_T const streamId = rItems.keyAt(i);
            ImageItem* pItem = rItems.valueAt(i).get();
            String8 str = String8::format("          streamId:%02" PRIx64 " ", streamId);
            if ( pItem ) {
                if(pItem->buffer)
                    str += String8::format("buffer %p, bufferId:%02" PRIu64 " ", pItem->buffer.get(), pItem->buffer->getBufferId());
                else
                    str += String8::format("buffer %p" PRIu64 " ", pItem->buffer.get());
                str += stateToText(pItem->state) + " " + historyToText(pItem->history);
            }
            printer.printLine(str.c_str());
        }
    };

    auto logMeta = [&](char const* tag, MetaItemSet const& rItems) {
        String8 strCaption = String8::format("      %s - ", tag);
        if (rItems.asInput) {
            strCaption += (rItems.valueAt(0)->buffer->isRepeating() ? "REPEAT" : "CHANGE");
        }
        else {
            strCaption += String8::format("return#:%zu valid#:%zu error#:%zu", rItems.numReturnedStreams, rItems.numValidStreams, rItems.numErrorStreams);
        }
        printer.printLine(strCaption.c_str());

        if (rItems.asInput) { return; }

        for (size_t i = 0; i < rItems.size(); i++) {
            StreamId_T const streamId = rItems.keyAt(i);
            MetaItem* pItem = rItems.valueAt(i).get();
            String8 str = String8::format("          streamId:%02" PRIx64 " ", streamId);
            if ( pItem ) {
                str += stateToText(pItem->state) + " " + historyToText(pItem->history);
            }
            printer.printLine(str.c_str());
        }
    };

    printer.printLine(" *Stream Configuration*");
    printer.printFormatLine("  operationMode:%#x", mOperationMode);
    {
        Mutex::Autolock _l(mImageConfigMapLock);
        for (size_t i = 0; i < mImageConfigMap.size(); i++) {
            auto const& item = mImageConfigMap.valueAt(i);
            item.pStreamInfo->dumpState(printer, 2);
        }
    }

    printer.printLine("");
    printer.printLine(" *In-flight requests*");
    FrameQueue::const_iterator itFrame = mFrameQueue.begin();
    for (; itFrame != mFrameQueue.end(); itFrame++)
    {
        auto const& item = (*itFrame);
        String8 caption = String8::format("  requestNo:%u", item->requestNo);
        if (auto pTool = NSCam::Utils::LogTool::get()) {
            caption += String8::format(" %s :", pTool->convertToFormattedLogTime(&item->requestTimestamp).c_str());
        }
        if (0 != item->shutterTimestamp) {
            caption += String8::format(" shutter:%" PRId64 "", item->shutterTimestamp);
        }
        if (0 != item->errors.value) {
            caption += String8::format(" errors:%#x", item->errors.value);
        }
        printer.printLine(caption.c_str());
        //
        //  Input Meta
        {
            logMeta(" IN Meta ", item->vInputMetaItem);
        }
        //
        //  Output Meta
        {
            logMeta("OUT Meta ", item->vOutputMetaItem);
        }
        //
        //  Input Image
        if ( 0 < item->vInputImageItem.size() )
        {
            logImage(" IN Image", item->vInputImageItem);
        }
        //
        //  Output Image
        {
            logImage("OUT Image", item->vOutputImageItem);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpLocked() const -> void
{
    ULogPrinter p(__ULOG_MODULE_ID, LOG_TAG);
    dumpStateLocked(p);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dump() const -> void
{
    Mutex::Autolock _l(mFrameQueueLock);
    dumpLocked();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpState(android::Printer& printer, const std::vector<std::string>& /*options*/) -> void
{
    Mutex::Autolock _l(mFrameQueueLock);
    dumpStateLocked(printer);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpIfHasInflightRequest() const -> void
{
    Mutex::Autolock _l(mFrameQueueLock);
    if ( mFrameQueue.size() )
        dumpLocked();
}
