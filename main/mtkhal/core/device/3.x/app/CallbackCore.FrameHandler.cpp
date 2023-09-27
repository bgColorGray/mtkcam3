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

#include <mtkcam/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
#include "CallbackCore.h"
#include "MyUtils.h"
// #include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
//
// #if (MTKCAM_HAVE_AEE_FEATURE == 1)
#if 0
#include <aee.h>
#define AEE_ASSERT(fmt, arg...)                                   \
  do {                                                            \
    String8 const str = String8::format(fmt, ##arg);              \
    CAM_LOGE("ASSERT(%s) fail", str.string());                    \
    aee_system_exception("mtkcam/Metadata", NULL, DB_OPT_DEFAULT, \
                         str.string());                           \
  } while (0)
#else
#define AEE_ASSERT(fmt, arg...)
#endif

using ::android::Condition;
using ::android::KeyedVector;
using ::android::Mutex;
using ::android::NAME_NOT_FOUND;
using ::android::OK;
using ::android::sp;
using ::android::String8;
using ::android::Vector;
using NSCam::eSTREAMID_BEGIN_OF_INTERNAL;
using NSCam::eSTREAMID_BEGIN_OF_PHYSIC_ID;
using NSCam::eSTREAMID_END_OF_INTERNAL;
using NSCam::eSTREAMID_END_OF_PHYSIC_ID;
using NSCam::eSTREAMID_META_APP_CONTROL;
using NSCam::Type2Type;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::REQ_APP_REQUEST;
using NSCam::Utils::ULog::ULogPrinter;
using NSCam::v3::eSTREAMTYPE_IMAGE_OUT;
using NSCam::v3::eSTREAMTYPE_META_IN;
using NSCam::v3::eSTREAMTYPE_META_OUT;
using NSCam::v3::STREAM_BUFFER_STATUS;
// using NSCam::IGrallocHelper;
// using NSCam::GrallocStaticInfo;

#define ThisNamespace CallbackCore::FrameHandler

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_DEBUG(level, fmt, arg...)                                       \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_WARN(level, fmt, arg...)                                        \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_ERROR_ULOG(level, fmt, arg...)                                  \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                                     \
  do {                                                                   \
    CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                   ##arg);                                               \
  } while (0)

#define MY_LOGV(...) MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...) MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...) MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...) MY_WARN(W, __VA_ARGS__)
#define MY_LOGE(...) MY_ERROR_ULOG(E, __VA_ARGS__)
#define MY_LOGA(...) MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...) MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGV(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGI_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGI(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace mcam {
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
static auto bufferStatusToText(BufferStatus status) -> String8 {
  switch (status) {
    case BufferStatus::OK:
      return String8("valid");
    case BufferStatus::ERROR:
      return String8("error");
  }
  return String8("never happen");
};

/******************************************************************************
 *
 ******************************************************************************/
static auto historyToText(::android::BitSet32 v) -> String8 {
  String8 s("");
  if (v.hasBit(ThisNamespace::HistoryBit::INQUIRED)) {
    s += "inquired,";
  }
  if (v.hasBit(ThisNamespace::HistoryBit::AVAILABLE)) {
    s += "available,";
  }
  if (v.hasBit(ThisNamespace::HistoryBit::RETURNED)) {
    s += "returned,";
  }
  if (v.hasBit(ThisNamespace::HistoryBit::ERROR_SENT_FRAME)) {
    s += "error-sent-frame,";
  }
  if (v.hasBit(ThisNamespace::HistoryBit::ERROR_SENT_META)) {
    s += "error-sent-meta,";
  }
  if (v.hasBit(ThisNamespace::HistoryBit::ERROR_SENT_IMAGE)) {
    s += "error-sent-image,";
  }
  if (s == "") {
    s += "not received";
  }
  return s;
};

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::FrameHandler(std::shared_ptr<CommonInfo> pCommonInfo,
                            sp<BatchHandler> pBatchHandler)
    : RefBase(),
      mInstanceName{pCommonInfo->userName + "-" +
                    std::to_string(pCommonInfo->mInstanceId) + "-FrameHandler"},
      mCommonInfo(pCommonInfo),
      mBatchHandler(pBatchHandler) {

  MY_LOGD("mImageConfigMap.size()=%lu", mImageConfigMap.size());

  ICallbackRecommender::CreationInfo creationInfo;
  creationInfo.partialResultCount = mCommonInfo->mAtMostMetaStreamCount;
  mCbRcmder = ICallbackRecommender::createInstance(creationInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  mBatchHandler = nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::setOperationMode(StreamConfigurationMode operationMode)
    -> void {
  if (mOperationMode != operationMode) {
    MY_LOGI("operationMode change: %#x -> %#x", mOperationMode, operationMode);
    mOperationMode = operationMode;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getConfigImageStream(StreamId_T streamId) const
    -> std::shared_ptr<StreamInfo> {
  Mutex::Autolock _l(mImageConfigMapLock);
  if (mImageConfigMap.count(streamId) > 0) {
    return mImageConfigMap.at(streamId);
  }
  return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::beginConfiguration(StreamConfiguration const& rStream)
    -> void {
  Mutex::Autolock _l(mImageConfigMapLock);
  mImageConfigMap.clear();
  for (const auto& item : rStream.streams) {
    StreamId_T streamId = item.id;
    std::shared_ptr<StreamInfo> pStreamInfo = std::make_shared<StreamInfo>();
    //
    pStreamInfo->id = streamId;
    pStreamInfo->streamType = item.streamType;
    pStreamInfo->width = item.width;
    pStreamInfo->height = item.height;
    pStreamInfo->usage = item.usage;
    pStreamInfo->physicalCameraId = item.physicalCameraId;
    pStreamInfo->transform = item.transform;
    // no need, set in endConfiguration
    // pStreamInfo->format = item.format; // no need
    // pStreamInfo->dataSpace = item.dataSpace;
    mImageConfigMap.insert(std::make_pair(streamId, pStreamInfo));
    if (pStreamInfo->physicalCameraId != -1)
      mIsAllowPhyMeta = true;
  }
  MY_LOGD("[debug] mImageConfigMap.size()=%lu", mImageConfigMap.size());
  // recommender
  if (mCbRcmder) {
    mCbRcmder->configureStreams(rStream);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::endConfiguration(HalStreamConfiguration const& rHalStreams)
    -> void {
  Mutex::Autolock _l(mImageConfigMapLock);
  for (const auto& item : rHalStreams.streams) {
    StreamId_T streamId = item.id;
    if (mImageConfigMap.count(streamId) <= 0) {
      MY_LOGE(
          "ERROR, non-existed stream in configure stage, "
          "streamId=%" PRIu64 "",
          streamId);
      continue;
    }
    // update HalStream info
    std::shared_ptr<StreamInfo> pTargetStream = mImageConfigMap.at(streamId);
    pTargetStream->format = item.overrideFormat;
    pTargetStream->dataSpace = item.overrideDataSpace;
    pTargetStream->producerUsage = item.producerUsage;
    pTargetStream->consumerUsage = item.consumerUsage;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::registerFrame(CaptureRequest const& rRequest) -> int {
  std::shared_ptr<FrameParcel> pFrame = std::make_shared<FrameParcel>();
  {
    Mutex::Autolock _l(mFrameMapLock);
    mFrameMap.insert(std::make_pair(rRequest.frameNumber, pFrame));
  }
  pFrame->frameNumber = rRequest.frameNumber;
  // create message item
  std::shared_ptr<ErrorMsg> pMsg = std::make_shared<ErrorMsg>();
  pMsg->frameNumber = rRequest.frameNumber;
  pMsg->errorCode = ErrorCode::ERROR_NONE;
  std::shared_ptr<MessageItem> pMsgItem = std::make_shared<MessageItem>();
  pMsgItem->pFrame = pFrame.get();
  pMsgItem->mData = pMsg;
  pFrame->mMessageItem = pMsgItem;  // link to FrameParcel
  //
  if (auto pTool = NSCam::Utils::LogTool::get()) {
    pTool->getCurrentLogTime(&pFrame->requestTimestamp);
  }
  //
  // create ICallbackRecommender::CaptureRequest
  ICallbackRecommender::CaptureRequest cr_CaptureRequest;
  cr_CaptureRequest.frameNumber = rRequest.frameNumber;
  IMetadata ctrlMeta = rRequest.settings;
  if (ctrlMeta.count() != 0) {
    MUINT8 enableZsl = 0;
    MUINT8 captureIntent = 0;
    IMetadata::getEntry<MUINT8>(&ctrlMeta, MTK_CONTROL_ENABLE_ZSL, enableZsl);
    IMetadata::getEntry<MUINT8>(&ctrlMeta, MTK_CONTROL_CAPTURE_INTENT,
                                captureIntent);
    bool zslStillCapture =
        (enableZsl == MTK_CONTROL_ENABLE_ZSL_TRUE) &&
        (captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE);
    bool reprocessing = rRequest.inputBuffers.size() > 0;
    if (reprocessing) {
      cr_CaptureRequest.requestType =
          ICallbackRecommender::RequestType::eREQUEST_REPROCESSING;
    } else if (zslStillCapture) {
      cr_CaptureRequest.requestType =
          ICallbackRecommender::RequestType::eREQUEST_ZSL_STILL_CAPTURE;
    }
    MY_LOGD_IF(zslStillCapture || reprocessing,
               "zslStillCapture=%d, reprocessing=%d, requestType=%d",
               zslStillCapture, reprocessing, cr_CaptureRequest.requestType);
  } else {
    MY_LOGE("request.settings is nullptr");
  }
  //
  //  Request::vInputImageBuffers
  //  Request::vOutputImageBuffers
  {
    registerStreamBuffers(rRequest.outputBuffers, pFrame, cr_CaptureRequest);
    // registerStreamBuffers(rRequest.vOutputImageStreams, pFrame,
    // &pFrame->vOutputImageItem);
    registerStreamBuffers(rRequest.inputBuffers, pFrame, cr_CaptureRequest);
  }
  //
  //  Request::vInputMetaBuffers (Needn't register)
  //  Request::vOutputMetaBuffers
  {
    // registerStreamBuffers(rRequest.vOutputMetaBuffers, pFrame,
    // &pFrame->vOutputMetaItem);
    registerStreamBuffers(rRequest.settings, pFrame);
  }
  //
  // register recommender
  std::vector<ICallbackRecommender::CaptureRequest> vCrReq;
  vCrReq.emplace_back(cr_CaptureRequest);
  mCbRcmder->registerRequests(vCrReq);
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::registerStreamBuffers(
    std::vector<StreamBuffer> const& buffers,
    std::shared_ptr<FrameParcel>& pFrame,
    ICallbackRecommender::CaptureRequest& cr_CaptureRequest) -> int {
  //  Request::[input|output]Buffers
  //  -> FrameParcel
  //  -> Recommender
  //
  Mutex::Autolock _l(mImageConfigMapLock);
  for (size_t i = 0; i < buffers.size(); i++) {
    StreamId_T const streamId = buffers[i].streamId;
    //
    if (mImageConfigMap.count(streamId) <= 0) {
      MY_LOGE("[frameNumber:%u] bad streamId:%#" PRIx64, pFrame->frameNumber,
              streamId);
      return NAME_NOT_FOUND;
    }
    bool isOutputBuffer =
        mImageConfigMap.at(streamId)->streamType == StreamType::OUTPUT;
    //
    std::shared_ptr<ImageItem> pItem = std::make_shared<ImageItem>();
    ImageItemSet* pItemSet =
        isOutputBuffer ? &pFrame->vOutputImageItem : &pFrame->vInputImageItem;
    pItem->pFrame = pFrame.get();
    pItem->pItemSet = pItemSet;
    pItem->mData = std::make_shared<StreamBuffer>(buffers[i]);
    pItemSet->insert(std::make_pair(streamId, pItem));
    // recommender
    if (isOutputBuffer) {
      cr_CaptureRequest.vOutputStreamIds.push_back(streamId);
    } else {
      cr_CaptureRequest.vInputStreamIds.push_back(streamId);
    }
  }
  //
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::registerStreamBuffers(IMetadata const& metadata,
                                          std::shared_ptr<FrameParcel>& pFrame)
    -> int {
  //  Request::settings
  //  -> FrameParcel
  //
  constructMetaItem(pFrame, &pFrame->vInputMetaItem, metadata);
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
MINT ThisNamespace::checkRequestError(
    std::shared_ptr<FrameParcel> const& pFrame) {
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
  auto isBufferError = [&](std::shared_ptr<ImageItem> const& pItem) -> bool {
    return pItem->history.hasBit(HistoryBit::INQUIRED) &&
           pItem->mData->status == BufferStatus::ERROR;
  };
  //
  // [NOT a request error]
  //
  bool isAnyStreamValid = false;
  for (auto& item : pFrame->vOutputImageItem) {
    if (isBufferError(item.second))
      isAnyStreamValid = true;
  }
  for (auto& item : pFrame->vInputImageItem) {
    if (isBufferError(item.second))
      isAnyStreamValid = true;
  }
  if (isAnyStreamValid && pFrame->vOutputMetaItem.hasLastPartial) {
    return -1;
  }
  //
  // [A request error]
  //
  bool isAllStreamError = true;
  for (auto& item : pFrame->vOutputImageItem) {
    if (!isBufferError(item.second))
      isAllStreamError = false;
  }
  for (auto& item : pFrame->vInputImageItem) {
    if (!isBufferError(item.second))
      isAllStreamError = false;
  }
  if (isAllStreamError && pFrame->getErrorCode() == ErrorCode::ERROR_RESULT) {
    return 1;
  }
  //
  // [uncertain]
  //
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareErrorFrame(
    CallbackParcel& rCbParcel,
    std::shared_ptr<FrameParcel> const& pFrame) -> void {
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
  // Note:
  // FrameParcel::vInputImageItem
  // We're not sure whether input image streams are returned or not.
  //
  // FrameParcel::vOutputImageItem
  for (auto& pItem : pFrame->vOutputImageItem) {
    pItem.second->mData->status = BufferStatus::ERROR;
    prepareReturnImage(rCbParcel, pItem.second);
  }
  //
  pFrame->mMessageItem->history.markBit(HistoryBit::ERROR_SENT_FRAME);
  MY_LOGI("[frameNumber %u] request error", pFrame->frameNumber);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareErrorMetaIfPossible(
    CallbackParcel& rCbParcel,
    std::shared_ptr<MessageItem> const& pItem) -> void {
  //
  CallbackParcel::Error error;
  error.errorCode = ErrorCode::ERROR_RESULT;
  //
  rCbParcel.vError.add(error);
  rCbParcel.valid = MTRUE;
  //
  //  Actually, this item will be set to NULL, and it is not needed for
  //  the following statements.
  //
  pItem->history.markBit(HistoryBit::ERROR_SENT_META);
  MY_LOGI("[frameNumber %u], result error", pItem->mData->frameNumber);
  //
  if (0 == pItem->pFrame->getShutterTimestamp()) {
    MY_LOGW(
        "[frameNumber:%u] CAMERA3_MSG_ERROR_RESULT with shutter timestamp = 0",
        pItem->mData->frameNumber);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareErrorImage(CallbackParcel& rCbParcel,
                                      std::shared_ptr<ImageItem> const& pItem)
    -> void {
  rCbParcel.valid = MTRUE;
  //
  {
    Mutex::Autolock _l(mImageConfigMapLock);
    StreamId_T const streamId = pItem->mData->streamId;
    std::shared_ptr<StreamInfo> const& pStreamInfo =
        mImageConfigMap.at(streamId);
    //
    CallbackParcel::Error error;
    error.errorCode = ErrorCode::ERROR_BUFFER;
    error.stream = pStreamInfo;
    //
    rCbParcel.vError.add(error);
    MY_LOGW_IF(1, "(Error Status) streamId:%#" PRIx64 "", streamId);
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
auto ThisNamespace::getSafeCbParecl(
    uint32_t frameNumber,
    std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
    bool needAcquireFromMap = false) -> std::shared_ptr<CallbackParcel> {
  if (rCbParcelMap.count(frameNumber) > 0) {
    return rCbParcelMap.at(frameNumber);
  } else {
    if (needAcquireFromMap) {
      return nullptr;
    } else {
      std::shared_ptr<CallbackParcel> pCbParcel =
          std::make_shared<CallbackParcel>();
      pCbParcel->valid = MFALSE;
      pCbParcel->frameNumber = frameNumber;
      rCbParcelMap.insert(make_pair(frameNumber, pCbParcel));
      return pCbParcel;
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareShutterNotificationIfPossible(
    std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
    uint32_t const& frameNumber) -> bool {
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber %u, prepare shutter callback failed", frameNumber);
    return false;
  }
  std::shared_ptr<ShutterItem>& pItem = pFrame->mShutterItem;
  auto rData = pItem->mData;
  //
  pItem->history.markBit(HistoryBit::AVAILABLE);
  if (!pItem->history.hasBit(HistoryBit::RETURNED) && rData->timestamp != 0) {
    std::shared_ptr<CallbackParcel> pCbParcel =
        getSafeCbParecl(frameNumber, rCbParcelMap);
    //
    pCbParcel->shutter = rData;
    pCbParcel->valid = MTRUE;
    //
    pItem->history.markBit(HistoryBit::RETURNED);
    return true;
  }
  MY_LOGW("frameNumber %u, prepare shutter callback again, ignore",
          frameNumber);
  return false;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareErrorNotificationIfPossible(
    std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
    ErrorMsg const& error) -> bool {
  // ERROR_BUFFER is handle in image callback flow
  if (error.errorCode == ErrorCode::ERROR_BUFFER) {
    return true;
  }
  uint32_t frameNumber = error.frameNumber;
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber %u, prepare error callback failed", frameNumber);
    return false;
  }
  std::shared_ptr<MessageItem>& pItem = pFrame->mMessageItem;
  auto& rData = pItem->mData;
  //
  pItem->history.markBit(HistoryBit::AVAILABLE);
  if (!pItem->history.hasBit(HistoryBit::RETURNED)) {
    std::shared_ptr<CallbackParcel> pCbParcel =
        getSafeCbParecl(frameNumber, rCbParcelMap);
    // update FrameParcel->mMessageItem
    rData->errorCode = error.errorCode;
    if (rData->errorCode == ErrorCode::ERROR_RESULT) {
      prepareErrorMetaIfPossible(*pCbParcel, pItem);
    } else if (rData->errorCode == ErrorCode::ERROR_REQUEST) {
      prepareErrorFrame(*pCbParcel, pFrame);
    }
    pItem->history.markBit(HistoryBit::RETURNED);
    pCbParcel->valid = MTRUE;
    return true;
  }
  MY_LOGW(
      "frameNumber %u, prepare message callback again, ignore"
      "message : %d -> %d",
      frameNumber, error.errorCode, rData->errorCode);
  return false;
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareReturnMeta(CallbackParcel& rCbParcel,
                                      std::shared_ptr<MetaItem> const& pItem)
    -> void {
  rCbParcel.valid = MTRUE;
  //
  {
    pItem->history.markBit(HistoryBit::RETURNED);
    //
    Vector<CallbackParcel::MetaItem>* pvCbItem = &rCbParcel.vOutputMetaItem;
    CallbackParcel::MetaItem& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
    rCbItem.metadata = pItem->mData;
    if (pItem->bufferNo == mCommonInfo->mAtMostMetaStreamCount) {
      rCbItem.bufferNo = mCommonInfo->mAtMostMetaStreamCount;
      //
      // #warning "[FIXME] hardcode: REQUEST_PIPELINE_DEPTH=4"
      IMetadata::IEntry entry(MTK_REQUEST_PIPELINE_DEPTH);
      entry.push_back(4, Type2Type<MUINT8>());
      rCbItem.metadata.update(MTK_REQUEST_PIPELINE_DEPTH, entry);
    } else {
      rCbItem.bufferNo = pItem->bufferNo;
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareReturnHalMeta(CallbackParcel& rCbParcel,
                                         std::shared_ptr<MetaItem> const& pItem)
    -> void {
  rCbParcel.valid = MTRUE;
  //
  {
    pItem->history.markBit(HistoryBit::RETURNED);
    //
    Vector<CallbackParcel::MetaItem>* pvCbItem = &rCbParcel.vOutputHalMetaItem;
    CallbackParcel::MetaItem& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
    rCbItem.metadata = pItem->mData;
    rCbItem.bufferNo = pItem->bufferNo;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::preparePhysicReturnMeta(
    CallbackParcel& rCbParcel,
    std::shared_ptr<PhysicMetaItem> const& pItem) -> void {
  rCbParcel.valid = MTRUE;
  //
  auto& rData = pItem->mData;
  //
  MBOOL isExistingPhysicId = MFALSE;
  Vector<PhysicalCameraMetadata>* pvCbItem = &rCbParcel.vOutputPhysicMetaItem;
  for (size_t i = 0; i < (*pvCbItem).size(); i++) {
    if ((*pvCbItem)[i].physicalCameraId == rData->physicalCameraId) {
      // Append the current PhysicMetaItem to the existing one
      isExistingPhysicId = MTRUE;
      PhysicalCameraMetadata& rCbItem = pvCbItem->editItemAt(i);
      {
        IMetadata& rSrcMeta = rData->metadata;
        IMetadata& rDstMeta = rCbItem.metadata;
        if (rSrcMeta.count() > 0) {
          rDstMeta += rSrcMeta;
        } else {
          MY_LOGW(
              "Physical Meta %d is null: frameNumber:%u, camId:%d, src:%p, "
              "dst:%p",
              pItem->bufferNo, pItem->pFrame->frameNumber,
              rData->physicalCameraId, &rSrcMeta, &rDstMeta);
        }
      }
      break;
    }
  }
  if (!isExistingPhysicId) {
    PhysicalCameraMetadata& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
    rCbItem = *rData;
#if 1  // workaround for physical stream errors, same as ERROR_RESULT
    if (rCbItem.metadata.count() == 0) {
      MY_LOGW(
          "Workaround for physical stream errors in Android P, which should "
          "be fixed in Android Q");
      IMetadata::IEntry entry(MTK_SENSOR_TIMESTAMP);
      MINT64 timestamp = (MINT64)pItem->pFrame->getShutterTimestamp();
      if (timestamp == 0) {
        MY_LOGE(
            "Physical metadata of frameNumber:%u, camId:%d, callbacked with "
            "shutter timestamp is 0",
            pItem->pFrame->frameNumber, rData->physicalCameraId);
      }
      entry.push_back(timestamp, Type2Type<MINT64>());
      rCbItem.metadata.update(MTK_SENSOR_TIMESTAMP, entry);
    }
#endif
    // #warning "[FIXME] hardcode: REQUEST_PIPELINE_DEPTH=4"
    IMetadata::IEntry entry(MTK_REQUEST_PIPELINE_DEPTH);
    entry.push_back(4, Type2Type<MUINT8>());
    rCbItem.metadata.update(MTK_REQUEST_PIPELINE_DEPTH, entry);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::isReturnable(
    std::shared_ptr<MetaItem> const& pItem) const
    -> bool {
  // 1. check if the meta is returned already
  if (pItem->history.hasBit(HistoryBit::RETURNED)) {
    return false;
  }
  // 2. check if any real-time partial arrives or not,
  //   if not, wait until last partial if the meta buffer is non-real time
  if (mCommonInfo->coreSetting->enableMetaPending()) {
    if (!pItem->pItemSet->hasLastPartial &&
        !pItem->pItemSet->hasRealTimePartial) {
      return false;
    }
  }
  // 3. check last partial rule
  //   if not available, wait until recommender feedback
  if (!pItem->history.hasBit(HistoryBit::AVAILABLE)) {
    return false;
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::isReturnable(
    std::shared_ptr<PhysicMetaItem> const& pItem,
    bool hasLogMetaCB) const
    -> bool {
  // 1. check if the meta is returned already
  if (pItem->history.hasBit(HistoryBit::RETURNED)) {
    return false;
  }
  // 2. check if any real-time partial arrives or not,
  //   if not, wait until last partial if the meta buffer is non-real time
  if (mCommonInfo->coreSetting->enableMetaPending()) {
    if (!hasLogMetaCB) {
      return false;
    }
  }
  // 3. check if meta is returnable
  if (!pItem->history.hasBit(HistoryBit::AVAILABLE)) {
    return false;
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareReturnImage(CallbackParcel& rCbParcel,
                                       std::shared_ptr<ImageItem> const& pItem)
    -> void {
  rCbParcel.valid = MTRUE;
  //
  if (!pItem->history.hasBit(HistoryBit::RETURNED)) {
    Mutex::Autolock _l(mImageConfigMapLock);
    pItem->history.markBit(HistoryBit::RETURNED);
    //
    StreamId_T const streamId = pItem->mData->streamId;
    std::shared_ptr<StreamInfo>& pStreamInfo = mImageConfigMap.at(streamId);
    //
    Vector<CallbackParcel::ImageItem>* pvCbItem =
        (pItem->pItemSet->asInput) ? &rCbParcel.vInputImageItem
                                   : &rCbParcel.vOutputImageItem;
    CallbackParcel::ImageItem& rCbItem = pvCbItem->editItemAt(pvCbItem->add());
    rCbItem.buffer = pItem->mData;
    rCbItem.stream = pStreamInfo;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::isReturnable(std::shared_ptr<ImageItem> const& pItem) const
    -> bool {
  // 1. check if returned already
  if (pItem->history.hasBit(HistoryBit::RETURNED))
    return false;

  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareMetaCallbackIfPossible(
    std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
    std::shared_ptr<FrameParcel>& pFrame,
    std::vector<ICallbackRecommender::CaptureResult>& rAvailableResults)
    -> bool {
  // check if has ERROR_RESULT or ERROR_REQUEST
  if (pFrame->getErrorCode() != ErrorCode::ERROR_NONE) {
    return false;
  }
  //
  uint32_t frameNumber = pFrame->frameNumber;
  MBOOL anyUpdate = MFALSE;
  for (auto& item : pFrame->vOutputMetaItem) {
    std::shared_ptr<MetaItem> pItem = item.second;
    if (pItem == nullptr) {
      continue;
    }
    if (isReturnable(pItem)) {
      std::shared_ptr<CallbackParcel> pCbParcel =
          getSafeCbParecl(frameNumber, rCbParcelMap);
      prepareReturnMeta(*pCbParcel, pItem);
      anyUpdate = true;
    }
  }
  //
  MBOOL hasLogMetaCB = anyUpdate;
  for (auto& item : pFrame->vOutputPhysicMetaItem) {
    std::shared_ptr<PhysicMetaItem> pItem = item.second;
    if (isReturnable(pItem, hasLogMetaCB)) {
      std::shared_ptr<CallbackParcel> pCbParcel =
          getSafeCbParecl(frameNumber, rCbParcelMap);
      preparePhysicReturnMeta(*pCbParcel, pItem);
      anyUpdate = true;
    }
  }
  //
  for (auto& item : pFrame->vOutputHalMetaItem) {
    std::shared_ptr<MetaItem> pItem = item.second;
    if (pItem == nullptr) {
      continue;
    }
    if (isReturnable(pItem)) {
      std::shared_ptr<CallbackParcel> pCbParcel =
          getSafeCbParecl(frameNumber, rCbParcelMap);
      prepareReturnHalMeta(*pCbParcel, pItem);
      anyUpdate = true;
    }
  }
  return anyUpdate;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareImageCallbackIfPossible(
    std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
    ICallbackRecommender::CaptureResult const& rResult) -> bool {
  auto onPrepareImageCallback = [&](BufferStatus status,
                                    std::shared_ptr<ImageItem> pItem) -> void {
    if (isReturnable(pItem)) {
      std::shared_ptr<CallbackParcel> pCbParcel =
          getSafeCbParecl(rResult.frameNumber, rCbParcelMap);
      if (status == BufferStatus::ERROR) {
        prepareErrorImage(*pCbParcel, pItem);
      }
      prepareReturnImage(*pCbParcel, pItem);
    }
  };
  uint32_t frameNumber = rResult.frameNumber;
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  //
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber %u, prepare image callback failed", frameNumber);
    return false;
  }
  // check error request
  ErrorCode curError = pFrame->getErrorCode();
  if (curError == ErrorCode::ERROR_REQUEST) {
    return false;
  }
  //
  for (auto const& item : rResult.vOutputStreams) {
    ImageItemSet& rItemSet = pFrame->vOutputImageItem;
    std::shared_ptr<ImageItem> pItem = rItemSet.at(item.streamId);
    onPrepareImageCallback(item.status, pItem);
  }
  for (auto const& item : rResult.vInputStreams) {
    ImageItemSet& rItemSet = pFrame->vInputImageItem;
    std::shared_ptr<ImageItem> pItem = rItemSet.at(item.streamId);
    onPrepareImageCallback(item.status, pItem);
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateResult(ResultQueue const& rvResult,
                                 RecommenderQueue& recommendQueue) -> void {
  if (mFrameMap.empty()) {
    MY_LOGD("Empty FrameMap:%p %p", &mFrameMap, this);
    return;
  }
  RecommenderQueue requestQueue;
  //
  // update ShutterItem
  for (auto& item : rvResult.vShutterResult) {
    updateShutterItem(item.first, item.second, requestQueue.availableShutters);
  }
  // update MessageItem
  for (auto& item : rvResult.vMessageResult) {
    updateMessageItem(item.first, item.second, requestQueue.availableErrors);
  }
  // update Hal MetaItem
  for (auto& item : rvResult.vHalMetaResult) {
    updateMetaItem(true, item.first, item.second,
                   requestQueue.availableResults);
  }
  // update MetaItem
  for (auto& item : rvResult.vMetaResult) {
    updateMetaItem(false, item.first, item.second,
                   requestQueue.availableResults);
  }
  // udpate physical meta
  for (auto& item : rvResult.vPhysicMetaResult) {
    updatePhysicMetaItem(item.first, item.second,
                         requestQueue.availableResults);
  }
  // update Output IamgeItem
  for (auto& vItem : rvResult.vOutputImageResult) {
    updateImageItem(vItem.first, vItem.second, true,
                    requestQueue.availableBuffers);
  }
  // update Input ImageItem
  for (auto& vItem : rvResult.vInputImageResult) {
    updateImageItem(vItem.first, vItem.second, false,
                    requestQueue.availableBuffers);
  }
  //
  // ask CallbackRecommender and append result
  onHandleRecommenderCb(requestQueue, recommendQueue);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onHandleRecommenderCb(RecommenderQueue& reqeustQueue,
                                          RecommenderQueue& resultQueue)
    -> void {
  // append result
  Utils::ICallbackRecommender::availableCallback_cb rcmd_cb =
      [&](const std::vector<uint32_t>& cbShutters,
          const std::vector<ErrorMsg>& cbErrors,
          const std::vector<ICallbackRecommender::CaptureResult>& cbBuffers,
          const std::vector<ICallbackRecommender::CaptureResult>& cbResults,
          const std::vector<uint32_t>& cbRemoveRequest) {
        if (!cbShutters.empty())
          resultQueue.availableShutters.insert(
              resultQueue.availableShutters.end(), cbShutters.begin(),
              cbShutters.end());
        if (!cbErrors.empty())
          resultQueue.availableErrors.insert(resultQueue.availableErrors.end(),
                                             cbErrors.begin(), cbErrors.end());
        if (!cbBuffers.empty())
          resultQueue.availableBuffers.insert(
              resultQueue.availableBuffers.end(), cbBuffers.begin(),
              cbBuffers.end());
        if (!cbResults.empty())
          resultQueue.availableResults.insert(
              resultQueue.availableResults.end(), cbResults.begin(),
              cbResults.end());
        if (!cbRemoveRequest.empty())
          resultQueue.removableRequests.insert(
              resultQueue.removableRequests.end(), cbRemoveRequest.begin(),
              cbRemoveRequest.end());
      };
  //
  //
  mCbRcmder->notifyShutters(reqeustQueue.availableShutters, rcmd_cb);
  mCbRcmder->notifyErrors(reqeustQueue.availableErrors, rcmd_cb);
  mCbRcmder->receiveMetadataResults(reqeustQueue.availableResults, rcmd_cb);
  mCbRcmder->receiveBuffers(reqeustQueue.availableBuffers, rcmd_cb);
  // remove recommender's comment about metadata if not apply AOSP Meta Rule
  if (!mCommonInfo->coreSetting->enableAOSPMetaRule()) {
    resultQueue.availableResults.clear();
  }
  // mark AVAILABLE to last partial meta
  for (auto& result : resultQueue.availableResults) {
    if (result.hasLastPartial) {
      std::shared_ptr<FrameParcel> pFrame;
      getSafeFrameParcel(result.frameNumber, pFrame);
      pFrame->vOutputMetaItem.pLastItem->history.markBit(HistoryBit::AVAILABLE);
    }
  }
  // mark AVAILAVBLE to physical app meta
  auto markPhysicalMetaAvailable = [&](
      MUINT32 camId,
      std::map<MUINT32, std::shared_ptr<PhysicMetaItem>>& rMap) {
    for (auto& pair : rMap) {
      auto& item = pair.second;
      MY_LOGD("bufNo %d, item->mData->physicalCameraId = %d, camId = %d",
              pair.first, item->mData->physicalCameraId, camId);
      if (item->mData->physicalCameraId == camId) {
        item->history.markBit(HistoryBit::AVAILABLE);
        MY_LOGD("marked");
      }
    }
  };
  if (mIsAllowPhyMeta) {
    for (auto& result : resultQueue.availableResults) {
      for (auto& camId : result.physicalCameraIdsOfPhysicalResults) {
        std::shared_ptr<FrameParcel> pFrame;
        getSafeFrameParcel(result.frameNumber, pFrame);
        markPhysicalMetaAvailable(camId, pFrame->vOutputPhysicMetaItem);
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::forceRequestErrorAvailable(
    std::shared_ptr<FrameParcel>& pFrame,
    std::vector<ErrorMsg>& rvAvaibleErrors) -> void {
  // reset message to be returnable
  std::shared_ptr<MessageItem>& pItem = pFrame->mMessageItem;
  if (pItem->history.hasBit(HistoryBit::RETURNED))
    pItem->history.clearBit(HistoryBit::RETURNED);
  // change error type to reqeust error
  auto& rData = pItem->mData;
  rData->errorCode = ErrorCode::ERROR_REQUEST;
  // update resultQueue.availableErrors
  bool isAlreadyInResultQueue = false;
  for (size_t i = 0; i < rvAvaibleErrors.size(); ++i) {
    if (rData->frameNumber == rvAvaibleErrors[i].frameNumber)
      isAlreadyInResultQueue = true;
  }
  if (!isAlreadyInResultQueue) {
    rvAvaibleErrors.push_back(*rData);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::isFrameRemovable(
    std::shared_ptr<FrameParcel> const& pFrame) const -> bool {
  // Not all output image streams have been returned.
  for (auto const& item : pFrame->vOutputImageItem) {
    if (!item.second->history.hasBit(HistoryBit::RETURNED))
      return false;
  }
  //
  // Not all input image streams have been returned.
  for (auto const& item : pFrame->vInputImageItem) {
    if (!item.second->history.hasBit(HistoryBit::RETURNED))
      return false;
  }
  //
  //
  if (pFrame->getErrorCode() == ErrorCode::ERROR_REQUEST) {
    // frame error was ensured.
    return true;
  } else if (pFrame->getErrorCode() == ErrorCode::ERROR_RESULT) {
    // meta error was ensured.
    if (0 == pFrame->getShutterTimestamp()) {
      MY_LOGW("[frameNumber %u] shutter not sent with meta error",
              pFrame->frameNumber);
    }
  } else {
    // Not all meta streams have been returned.
    if (!pFrame->vOutputMetaItem.hasLastPartial) {
      return false;
    }
    //
    // No shutter timestamp has been sent.
    if (0 == pFrame->getShutterTimestamp()) {
      MY_LOGW("[frameNumber %u] shutter not sent @ no meta error",
              pFrame->frameNumber);
      return false;
    }
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getSafeFrameParcel(uint32_t frameNumber,
                                       std::shared_ptr<FrameParcel>& rpItem)
    -> bool {
  if (mFrameMap.count(frameNumber) <= 0) {
    String8 log = String8::format(
        "frameNumber %u is not in FrameMap; FrameMap:", frameNumber);
    for (auto& item : mFrameMap) {
      log += String8::format(" %u", item.first);
    }
    MY_LOGW("%s", log.c_str());
    return false;
  } else {
    rpItem = mFrameMap.at(frameNumber);
  }
  // null check
  if (rpItem == nullptr) {
    MY_LOGW("null frame, frameNumber %u", frameNumber);
    return false;
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::constructMetaItem(std::shared_ptr<FrameParcel> pFrame,
                                      MetaItemSet* pItemSet,
                                      IMetadata data) -> void {
  // create MetaItem
  std::shared_ptr<MetaItem> pItem = std::make_shared<MetaItem>();
  pItem->pFrame = pFrame.get();
  pItem->pItemSet = pItemSet;
  pItem->mData = data;
  pItem->bufferNo = pItemSet->size() + 1;
  pItem->history.markBit(HistoryBit::AVAILABLE);
  // update meta item set
  pItemSet->insert(std::make_pair(pItem->bufferNo, pItem));
  pItemSet->pLastItem = pItem;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateShutterItem(
    const uint32_t frameNumber,
    const std::shared_ptr<ShutterResult>& pResult,
    std::vector<uint32_t>& rRequestShutterQueue) -> void {
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber:%u, update shutter failed", frameNumber);
    return;
  }
  if (pFrame->mShutterItem) {
    MY_LOGW(
        "[frameNumber:%u] shutter received already, ignore"
        "(ori:%" PRId64 ", new:%" PRId64 ")",
        frameNumber, pFrame->getShutterTimestamp(), pResult->timestamp);
    return;
  }
  // handle ShutterResult
  std::shared_ptr<ShutterItem> pItem = std::make_shared<ShutterItem>();
  pFrame->mShutterItem = pItem;
  pItem->pFrame = pFrame.get();
  pItem->mData = pResult;
  // Recommender
  pFrame->mShutterItem->history.markBit(HistoryBit::INQUIRED);
  rRequestShutterQueue.emplace_back(frameNumber);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateMessageItem(const uint32_t frameNumber,
                                      const std::shared_ptr<ErrorMsg>& pResult,
                                      std::vector<ErrorMsg>& rRequestErrorQueue)
    -> void {
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (pResult->errorCode == ErrorCode::ERROR_BUFFER) {
    MY_LOGW(
        "frameNumber %u, CallbackCore not support ERROR_BUFFER"
        "processCaptureResult is necessary",
        frameNumber);
    return;
  }
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber %u, update message failed", frameNumber);
    return;
  }
  // check if message exists
  std::shared_ptr<MessageItem> pItem = pFrame->mMessageItem;
  if (pItem == nullptr) {
    MY_LOGE("[frameNumber %u] null message Item", frameNumber);
    return;
  }
  // check if error received already
  ErrorCode curError = pFrame->getErrorCode();
  if (curError == ErrorCode::ERROR_RESULT ||
      curError == ErrorCode::ERROR_REQUEST) {
    MY_LOGW("[frameNumber %u] receive result error before, ignore",
            frameNumber);
    return;
  }
  // update ErrorMsg
  pItem->mData = pResult;
  // Recommender
  pItem->history.markBit(HistoryBit::INQUIRED);
  rRequestErrorQueue.emplace_back(
      *(pItem->mData));  // need change recommender I/F
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateMetaItem(
    const bool isHalMeta,
    const uint32_t frameNumber,
    const VecMetaResult& vResult,
    std::vector<ICallbackRecommender::CaptureResult>& rRequestMetaQueue)
    -> void {
  //
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("reqeustNo %u, update meta failed", frameNumber);
    return;
  }
  // check if error buffer
  ErrorCode curError = pFrame->getErrorCode();
  if (curError == ErrorCode::ERROR_RESULT ||
      curError == ErrorCode::ERROR_REQUEST) {
    MY_LOGW("[frameNumber %u] already receive error buffer, ignore meta",
            frameNumber);
    return;
  }
  // check if receive last partial before
  if (pFrame->vOutputMetaItem.hasLastPartial) {
    MY_LOGW(
        "[frameNumber %u]  already receive last partial, ignore meta callback",
        frameNumber);
    return;
  }
  //
  MetaItemSet* pItemSet =
      isHalMeta ? &pFrame->vOutputHalMetaItem : &pFrame->vOutputMetaItem;
  //
  // handle real-time partial
  pItemSet->hasRealTimePartial =
      pItemSet->isHalMeta ? true : vResult.isRealTimePartial;
  // handle MetaResult
  for (size_t i = 0; i < vResult.size(); ++i) {
    constructMetaItem(pFrame, pItemSet, vResult.at(i));
  }
  // handle last partial
  if (vResult.hasLastPartial) {
    // pLastItem need to be called by reference
    std::shared_ptr<MetaItem>& pLastItem = pItemSet->pLastItem;
    if (pLastItem == nullptr) {
      MY_LOGE("frameNumber %u, receive last partial tag w/o meta", frameNumber);
    }
    if (pLastItem->history.hasBit(HistoryBit::RETURNED)) {
      // create a dummy meta for last partial
      IMetadata dummyMeta = IMetadata();
      constructMetaItem(pFrame, pItemSet, dummyMeta);
    }
    pItemSet->hasLastPartial = true;
    pLastItem->bufferNo = mCommonInfo->mAtMostMetaStreamCount;
    // Recommender
    if (mCommonInfo->coreSetting->enableAOSPMetaRule()) {
      // change status from AVAILABLE to INQUIRED
      pLastItem->history.clearBit(HistoryBit::AVAILABLE);
      pLastItem->history.markBit(HistoryBit::INQUIRED);
    }
    rRequestMetaQueue.emplace_back(ICallbackRecommender::CaptureResult{
        .frameNumber = frameNumber,
        .hasLastPartial = true,
    });
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updatePhysicMetaItem(
    const uint32_t frameNumber,
    const VecPhysicMetaResult& vResult,
    std::vector<ICallbackRecommender::CaptureResult>& rRequestMetaQueue)
    -> void {
  // check if physical meta is allowed
  if (!mIsAllowPhyMeta) {
    return;
  }
  //
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber %u, update physic meta failed", frameNumber);
    return;
  }
  // check if error buffer
  ErrorCode curError = pFrame->getErrorCode();
  if (curError == ErrorCode::ERROR_RESULT ||
      curError == ErrorCode::ERROR_REQUEST) {
    MY_LOGW(
        "[frameNumber %u] already receive error buffer, ignore physical meta",
        pFrame->frameNumber);
    return;
  }
  // handle physical meta
  ICallbackRecommender::CaptureResult cr_CaptureResult{.frameNumber =
                                                           frameNumber};
  PhysicMetaItemSet* pPhysicItemSet = &pFrame->vOutputPhysicMetaItem;
  for (const auto& res : vResult) {
    int32_t camId = res->physicalCameraId;
    IMetadata metadata = res->metadata;
    std::shared_ptr<PhysicMetaItem> pItem = std::make_shared<PhysicMetaItem>();
    // FrameParcel
    pItem->pFrame = pFrame.get();
    pItem->pItemSet = pPhysicItemSet;
    pItem->mData = res;
    pItem->bufferNo = pPhysicItemSet->size() + 1;
    // update phyItemSet
    pPhysicItemSet->insert(std::make_pair(pItem->bufferNo, pItem));
    // Recommender
    pItem->history.markBit(HistoryBit::INQUIRED);
    if (!mCommonInfo->coreSetting->enableAOSPMetaRule()) {
      pItem->history.markBit(HistoryBit::AVAILABLE);
    } else {
      cr_CaptureResult.physicalCameraIdsOfPhysicalResults.push_back(camId);
    }
  }
  if (!cr_CaptureResult.physicalCameraIdsOfPhysicalResults.empty()) {
    rRequestMetaQueue.emplace_back(cr_CaptureResult);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateImageItem(
    const uint32_t frameNumber,
    const VecImageResult& vResult,
    const bool isOutputStream,
    std::vector<ICallbackRecommender::CaptureResult>& rRequestImageQueue)
    -> void {
  auto pReleaseHandler =
      [=](const std::shared_ptr<StreamBuffer>& pBuffer,
          std::shared_ptr<ImageItem>& pItem,
          ICallbackRecommender::CaptureResult& cr_CaptureResult,
          const bool isOutputStream) {
        //
        // update recommender
        ICallbackRecommender::Stream cr_Stream{
            .streamId = pBuffer->streamId,
            .status = pBuffer->status,
        };
        if (isOutputStream)
          cr_CaptureResult.vOutputStreams.push_back(cr_Stream);
        else
          cr_CaptureResult.vInputStreams.push_back(cr_Stream);
        //
        // update ImageItem
        auto& rData = pItem->mData;
        if (pBuffer->status == BufferStatus::ERROR) {
          MY_LOGW(
              "[Image Stream Buffer] Error happens"
              " - frameNumber:%u streamId:%#" PRIx64 "",
              pItem->pFrame->frameNumber, pBuffer->streamId);
          rData->status = BufferStatus::ERROR;
        } else {
          rData->status = BufferStatus::OK;
        }
        pItem->history.markBit(HistoryBit::INQUIRED);
      };
  //
  std::shared_ptr<FrameParcel> pFrame = nullptr;
  if (!getSafeFrameParcel(frameNumber, pFrame)) {
    MY_LOGW("frameNumber %u, update image failed", frameNumber);
    return;
  }
  //
  ICallbackRecommender::CaptureResult cr_CaptureResult{
      .frameNumber = frameNumber,
  };
  for (const auto& res : vResult) {
    StreamId_T const streamId = res->streamId;
    std::shared_ptr<ImageItem> pItem =
        isOutputStream ? pFrame->vOutputImageItem.at(streamId)
                       : pFrame->vInputImageItem.at(streamId);
    if (pItem == nullptr) {
      MY_LOGE("unregister image item, frameNumber %u, streamId:%#" PRIx64 "",
              frameNumber, streamId);
      continue;
    }
    // handle image item
    if (!pItem->history.hasBit(HistoryBit::INQUIRED)) {
      pReleaseHandler(res, pItem, cr_CaptureResult, isOutputStream);
    } else {
      MY_LOGW(
          "the history shows user callback the image twice"
          ", streamId:%#" PRIx64 "",
          pItem->mData->streamId);
    }
  }
  rRequestImageQueue.emplace_back(cr_CaptureResult);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateCallback(std::list<CallbackParcel>& rCbList,
                                   RecommenderQueue& recommendQueue) -> void {
  String8 returnableDebugLog("");
  //
  std::map<uint32_t, std::shared_ptr<CallbackParcel>> cbParcelMap;
  //
  for (auto const& item : recommendQueue.availableShutters) {
    prepareShutterNotificationIfPossible(cbParcelMap, item);
  }
  //
  for (auto const& item : recommendQueue.availableErrors) {
    prepareErrorNotificationIfPossible(cbParcelMap, item);
  }
  //
  for (auto& item : mFrameMap) {
    prepareMetaCallbackIfPossible(cbParcelMap, item.second,
                                  recommendQueue.availableResults);
  }
  //
  for (auto const& item : recommendQueue.availableBuffers) {
    prepareImageCallbackIfPossible(cbParcelMap, item);
  }
  //
  for (auto const& item : recommendQueue.removableRequests) {
    std::shared_ptr<FrameParcel> pFrame = nullptr;
    if (!getSafeFrameParcel(item, pFrame)) {
      MY_LOGW("frameNumber %u, failed to remove FrameParcel");
      continue;
    }
    if (pFrame->mShutterItem == nullptr ||
        !pFrame->mShutterItem->history.hasBit(HistoryBit::RETURNED)) {
      MY_LOGW(
          "frameNumber %u is removed before shutter callbacked, there MUST "
          "have error frames or meta be sent for the request",
          item);
    }
    std::shared_ptr<CallbackParcel> pCbParcel =
        getSafeCbParecl(item, cbParcelMap, true);
    // remove this frame from the frame queue.
    CAM_ULOG_EXIT_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, item);
    if (pCbParcel)
      pCbParcel->needRemove = MTRUE;
    mFrameMap.erase(item);
  }
  //
  for (auto const& item : cbParcelMap) {
    rCbList.push_back(*(item.second));
  }
  //
  MY_LOGW_IF(returnableDebugLog.size() != 0, "%s", returnableDebugLog.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::update(ResultQueue const& rvResult) -> void {
  std::list<CallbackParcel> cbList;
  {
    Mutex::Autolock _l(mFrameMapLock);
    RecommenderQueue recommendQueue;
    updateResult(rvResult, recommendQueue);
    updateCallback(cbList, recommendQueue);
  }
  if (cbList.size())
    mBatchHandler->push(cbList);

  if (mFrameMap.empty()) {
    mFrameMapCond.broadcast();
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::waitUntilDrained(nsecs_t const timeout) -> int {
  nsecs_t const startTime = ::systemTime();
  //
  auto timeoutToWait = [=]() {
    nsecs_t const elapsedInterval = (::systemTime() - startTime);
    nsecs_t const timeoutToWait =
        (timeout > elapsedInterval) ? (timeout - elapsedInterval) : 0;
    return timeoutToWait;
  };
  //
  auto waitEmpty = [=](Mutex& lock, Condition& cond, auto const& queue) -> int {
    int err = OK;
    Mutex::Autolock _l(lock);
    while (!queue.empty()) {
      err = cond.waitRelative(lock, timeoutToWait());
      if (OK != err) {
        break;
      }
    }
    //
    if (queue.empty()) {
      return OK;
    }
    return err;
  };
  //
  //
  int err = OK;
  if (OK != (err = waitEmpty(mFrameMapLock, mFrameMapCond, mFrameMap))) {
    MY_LOGW("mFrameMap:#%zu timeout(ns):%" PRId64 " elapsed(ns):%" PRId64
            " err:%d(%s)",
            mFrameMap.size(), timeout, (::systemTime() - startTime), err,
            ::strerror(-err));
    dump();
  }
  //
  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpStateLocked(IPrinter& printer) const -> void {
  auto logImage = [&](char const* tag, ImageItemSet const& rItems) {
    // String8 strCaption =
    //     String8::format("      %s - return#:%zu valid#:%zu error#:%zu", tag,
    //                     rItems.numReturnedStreams, rItems.numValidStreams,
    //                     rItems.numErrorStreams);
    // printer.printLine(strCaption.c_str());
    for (auto const& item : rItems) {
      StreamId_T const streamId = item.first;
      ImageItem* pItem = item.second.get();
      String8 str =
          String8::format("          streamId:%02" PRIx64 " ", streamId);
      if (pItem) {
        if (pItem->mData)
          str += String8::format("buffer %p, bufferId:%02" PRIu64 " ",
                                 pItem->mData.get(), pItem->mData->bufferId);
        else
          str += String8::format("buffer %p" PRIu64 " ", pItem->mData.get());
        str += bufferStatusToText(pItem->mData->status);
        str += String8::format(" ") + historyToText(pItem->history);
      }
      printer.printLine(str.c_str());
    }
  };

  auto logMeta = [&](char const* tag, MetaItemSet const& rItems) {
    String8 strCaption = String8::format("      %s - ", tag);
    // if (rItems.asInput) {
    //   strCaption +=
    //       (rItems.valueAt(0)->buffer->isRepeating() ? "REPEAT" : "CHANGE");
    // } else {
    //   strCaption += String8::format(
    //       "return#:%zu valid#:%zu error#:%zu", rItems.numReturnedStreams,
    //       rItems.numValidStreams, rItems.numErrorStreams);
    // }
    printer.printLine(strCaption.c_str());

    if (rItems.asInput) {
      return;
    }

    for (auto const& item : rItems) {
      MetaItem* pItem = item.second.get();
      String8 str = String8::format("          bufferNo:%u count:%u",
                                    pItem->bufferNo, pItem->mData.count());
      str += String8::format(" ") + historyToText(pItem->history);
      printer.printLine(str.c_str());
    }
  };

  printer.printLine(" *Stream Configuration*");
  printer.printFormatLine("  operationMode:%#x", mOperationMode);
  {
    Mutex::Autolock _l(mImageConfigMapLock);
    for (auto const& item : mImageConfigMap) {
      // fix it
      // MY_LOGD("%s", toString(item));
    }
  }

  printer.printLine("");
  printer.printLine(" *In-flight requests*");
  for (auto const& it : mFrameMap) {
    auto const& item = it.second;
    String8 caption = String8::format("  frameNumber:%u", it.first);
    if (auto pTool = NSCam::Utils::LogTool::get()) {
      caption += String8::format(
          " %s :",
          pTool->convertToFormattedLogTime(&item->requestTimestamp).c_str());
    }
    if (0 != item->getShutterTimestamp()) {
      caption +=
          String8::format(" shutter:%" PRId64 "", item->getShutterTimestamp());
    }
    if (ErrorCode::ERROR_NONE != item->getErrorCode()) {
      caption += String8::format(" errors:%#x", item->getErrorCode());
    }
    printer.printLine(caption.c_str());
    //
    //  Input Meta
    { logMeta(" IN Meta ", item->vInputMetaItem); }
    //
    //  Output Meta
    { logMeta("OUT Meta ", item->vOutputMetaItem); }
    //
    //  Output Hal Meta
    { logMeta("OUT Hal Meta ", item->vOutputHalMetaItem); }
    //
    //  Input Image
    if (0 < item->vInputImageItem.size()) {
      logImage(" IN Image", item->vInputImageItem);
    }
    //
    //  Output Image
    { logImage("OUT Image", item->vOutputImageItem); }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpLocked() const -> void {
  struct MyPrinter : public IPrinter {
    std::shared_ptr<::android::FdPrinter> mFdPrinter;
    ULogPrinter mLogPrinter;
    explicit MyPrinter()
        : mFdPrinter(nullptr),
          mLogPrinter(__ULOG_MODULE_ID, LOG_TAG) {}
    ~MyPrinter() {}
    virtual void printLine(const char* string) {
      mLogPrinter.printLine(string);
    }
    virtual void printFormatLine(const char* format, ...) {
      va_list arglist;
      va_start(arglist, format);
      char* formattedString;
      if (vasprintf(&formattedString, format, arglist) < 0) {
        CAM_ULOGME("Failed to format string");
        va_end(arglist);
        return;
      }
      va_end(arglist);
      printLine(formattedString);
      free(formattedString);
    }
    virtual std::shared_ptr<::android::FdPrinter> getFDPrinter() {
      return mFdPrinter;
    }
  } printer;
  dumpStateLocked(printer);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dump() const -> void {
  Mutex::Autolock _l(mFrameMapLock);
  dumpLocked();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer,
                              const std::vector<std::string> &
                              /*options*/) -> void {
  Mutex::Autolock _l(mFrameMapLock);
  dumpStateLocked(printer);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpIfHasInflightRequest() const -> void {
  Mutex::Autolock _l(mFrameMapLock);
  if (mFrameMap.size())
    dumpLocked();
}
};  // namespace core
};  // namespace mcam
