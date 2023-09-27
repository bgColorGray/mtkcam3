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

#include "CustCallback.h"
#include "MyUtils.h"

#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <stdlib.h> // atoi

#include <system/camera_metadata.h> // camera_metadata_t

using ::android::Condition;
using ::android::DEAD_OBJECT;
using ::android::Mutex;
using ::android::NO_ERROR;
using ::android::OK;
using ::android::status_t;
using ::android::String8;
using ::android::Vector;

using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::REQ_APP_REQUEST;
using NSCam::Utils::ULog::ULogPrinter;

#define ThisNamespace CustCallback::FrameHandler

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
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
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace NSCam {
namespace custom_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::FrameHandler(
    std::shared_ptr<CommonInfo> pCommonInfo,
    android::sp<CallbackHandler> pCallbackHandler)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) +
                    "-FrameHandler"},
      mCallbackHandler(pCallbackHandler),
      mCommonInfo(pCommonInfo) {
  ICallbackRecommender::CreationInfo creationInfo{
    .partialResultCount = (uint32_t)pCommonInfo->mAtMostMetaStreamCount,
  };
  mCbRcmder = ICallbackRecommender::createInstance(creationInfo);
  if (mCbRcmder == nullptr) {
    MY_LOGE("failed to create ICallbackRecommdner");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  mCallbackHandler = nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::configureStreams(
    const hidl_vec<Stream>& rStreams) -> void {
  Mutex::Autolock _l(mAppStreamLock);
  mAppStreams.clear();
  mAppStreams = rStreams;
  // configure recommender
  if (mCbRcmder) {
    std::vector<ICallbackRecommender::StreamInfo> vInputStreams, vOutputStreams;
    for (auto& stream : mAppStreams) {
      ICallbackRecommender::StreamInfo streamInfo{
        .streamId = stream.v3_2.id,
      };
      if (!stream.physicalCameraId.empty()) {
        streamInfo.physicalCameraId = std::atoi(stream.physicalCameraId.c_str());
      }
      if (stream.v3_2.streamType == StreamType::OUTPUT) {
        vOutputStreams.push_back(streamInfo);
      } else {
        vInputStreams.push_back(streamInfo);
      }
    }
    mCbRcmder->configureStreams(
      ICallbackRecommender::ConfiguredStreamInfos{
        .vInputStreams = vInputStreams,
        .vOutputStreams = vOutputStreams,
    });
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::registerFrame(const CaptureRequest & rRequest) -> int {
  std::shared_ptr<FrameParcel> pFrame = std::make_shared<FrameParcel>();
  {
    Mutex::Autolock _l(mPendingParcelLock);
    mPendingParcels.insert(std::make_pair(rRequest.v3_2.frameNumber, pFrame));
  }
  // if (auto pTool = NSCam::Utils::LogTool::get()) {
  //   pTool->getCurrentLogTime(&pFrame->requestTimestamp);
  // }
  //
  // register recommender
  //
  std::vector<int64_t> vInputStreamIds, vOutputStreamIds;
  if (rRequest.v3_2.inputBuffer.streamId != -1) {
    vInputStreamIds.push_back(rRequest.v3_2.inputBuffer.streamId);
  }
  for (const auto& item : rRequest.v3_2.outputBuffers) {
    vOutputStreamIds.push_back(item.streamId);
  }
  std::vector<ICallbackRecommender::CaptureRequest> vReq;
  vReq.push_back(ICallbackRecommender::CaptureRequest{
    .requestNumber = rRequest.v3_2.frameNumber,
    .vInputStreamIds = vInputStreamIds,
    .vOutputStreamIds = vOutputStreamIds,
    .requestType = queryRequestType(rRequest),
  });
  mCbRcmder->registerRequests(vReq);
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getSafeFrameParcel(
    uint32_t requestNo,
    std::shared_ptr<FrameParcel>& rpItem) -> bool {
  if (mPendingParcels.count(requestNo) <= 0) {
    String8 log = String8::format(
      "requestNo %u is not in mPendingParcels: ", requestNo);
    for(auto& item : mPendingParcels) {
      log += String8::format(" %u", item.first);
    }
    MY_LOGW("%s", log.c_str());
    return false;
  } else {
    rpItem = mPendingParcels.at(requestNo);
  }
  // null check
  if (rpItem == nullptr) {
    MY_LOGW("null parcel, requestNo %u", requestNo);
    return false;
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::queryRequestType(
    const CaptureRequest& rRequest) -> ICallbackRecommender::RequestType {
  const camera_metadata_t* inputSettings = reinterpret_cast<const camera_metadata_t*>(
                  rRequest.v3_2.settings.data());
  if (get_camera_metadata_size(inputSettings) == 0) {
    return ICallbackRecommender::RequestType::eREQUEST_NORMAL;
  }
  bool isStillCapture = false, enableZsl = false;
  // check capture intent
  camera_metadata_ro_entry_t e;
  find_camera_metadata_ro_entry(inputSettings,
                                ANDROID_CONTROL_CAPTURE_INTENT,
                                &e);
  if ((e.count > 0) &&
      (e.data.u8[0] == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE)) {
    isStillCapture = true;
  }
  // check zsl enable
  find_camera_metadata_ro_entry(inputSettings,
                               ANDROID_CONTROL_ENABLE_ZSL,
                               &e);
  if ((e.count > 0) &&
      (e.data.u8[0] == ANDROID_CONTROL_ENABLE_ZSL_TRUE)) {
    enableZsl = true;
  }
  // set request type
  ICallbackRecommender::RequestType requestType =
      ICallbackRecommender::RequestType::eREQUEST_NORMAL;
  bool isZslCapture = isStillCapture && enableZsl;
  bool isReprocessing = rRequest.v3_2.inputBuffer.streamId != -1;
  if (isZslCapture) {
    requestType =
        ICallbackRecommender::RequestType::eREQUEST_ZSL_STILL_CAPTURE;
  } else if (isReprocessing) {
    requestType =
        ICallbackRecommender::RequestType::eREQUEST_REPROCESSING;
  }
  MY_LOGD_IF(isZslCapture || isReprocessing,
              "isZslCapture=%d, isReprocessing=%d, requestType=%d",
              isZslCapture, isReprocessing, requestType);
  return requestType;
};

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::queryPartialCount(
    CaptureResult& rResult,
    std::shared_ptr<FrameParcel>& rpCbParcel) -> uint32_t {
  // handle partialResult if not the last partial
  if (rResult.v3_2.partialResult == mCommonInfo->mAtMostMetaStreamCount)
    return rResult.v3_2.partialResult;
  rpCbParcel->partialCount++;
  MY_LOGW_IF(
      rpCbParcel->partialCount > mCommonInfo->mAtMostMetaStreamCount,
      "requestNo %u, receive too many meta (up to %u)",
      rResult.v3_2.frameNumber, mCommonInfo->mAtMostMetaStreamCount);
  return rpCbParcel->partialCount;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::update(
    std::vector<CaptureResult>& rvResult,
    std::vector<NotifyMsg>& rvMsg) -> void {
  FrameParcel cbList;
  {
    Mutex::Autolock _l(mPendingParcelLock);
    RecommenderQueue recommendQueue;
    updateResult(rvResult, rvMsg, recommendQueue, cbList);
    updateCallback(recommendQueue, cbList);
  }
  if (!cbList.vResult.empty() ||
      !cbList.vMsg.empty()) {
    mCallbackHandler->push(cbList);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateResult(
    std::vector<CaptureResult>& rvResult,
    std::vector<NotifyMsg>& rvMsg,
    RecommenderQueue& resultQueue,
    FrameParcel& cbList) -> void {
  if (mPendingParcels.empty()) {
    MY_LOGD("Empty mPendingParcels:%p %p", &mPendingParcels, this);
    return;
  }
  RecommenderQueue requestQueue;
  //
  // handle CaptureResult
  for (auto& result : rvResult) {
    uint32_t requestNo = result.v3_2.frameNumber;
    if (mPendingParcels.count(requestNo) <= 0) {
      MY_LOGW("requestNo %u, not exists in mPendingParcels", requestNo);
      continue;
    }
    std::shared_ptr<FrameParcel> pFrame = mPendingParcels.at(requestNo);
    // handle CaptureResult - Result
    updateMetaCb(result, pFrame, requestQueue, cbList);
    updatePhysicMetaCb(result, pFrame, requestQueue);
    updateImageCb(result, pFrame, requestQueue);
  }
  //
  // handle NotifyMsg
  for (auto& item : rvMsg) {
    uint32_t requestNo = item.type == MsgType::SHUTTER ?
                         item.msg.shutter.frameNumber :
                         item.msg.error.frameNumber;
    if (mPendingParcels.count(requestNo) <= 0) {
      MY_LOGW("requestNo %u, not exists in mPendingParcels", requestNo);
      continue;
    }
    std::shared_ptr<FrameParcel> pFrame = mPendingParcels.at(requestNo);
    // handle NotifyMsg
    updateShutterCb(item, pFrame, requestQueue);
    updateErrorCb(item, pFrame, requestQueue);
  }
  // ask CallbackRecommender and append result
  onHandleRecommenderFeedback(requestQueue, resultQueue);
  //
  if (mCommonInfo->mLogLevel >= 1) {
    String8 log = String8::format("mPendingParcels : ");
    for (const auto& frame : mPendingParcels) {
      log += String8::format("%u:[%zu/%zu] ", frame.first,
                            frame.second->vResult.size(),
                            frame.second->vMsg.size());
    }
    MY_LOGD("%s", log.c_str());
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onHandleRecommenderFeedback(
    RecommenderQueue& reqeustQueue,
    RecommenderQueue& resultQueue) -> void {
  // append result
  ICallbackRecommender::availableCallback_cb rcmd_cb = [&](
      const std::vector<uint32_t>& cbShutters,
      const std::vector<ICallbackRecommender::ErrorMessage>& cbErrors,
      const std::vector<ICallbackRecommender::CaptureResult>& cbBuffers,
      const std::vector<ICallbackRecommender::CaptureResult>& cbResults,
      const std::vector<uint32_t>& cbRemoveRequest) {
    if (!cbShutters.empty())
      resultQueue.availableShutters.insert(
          resultQueue.availableShutters.end(),
          cbShutters.begin(), cbShutters.end());
    if (!cbErrors.empty())
      resultQueue.availableErrors.insert(
          resultQueue.availableErrors.end(),
          cbErrors.begin(), cbErrors.end());
    if (!cbBuffers.empty())
      resultQueue.availableBuffers.insert(
          resultQueue.availableBuffers.end(),
          cbBuffers.begin(), cbBuffers.end());
    if (!cbResults.empty())
      resultQueue.availableResults.insert(
          resultQueue.availableResults.end(),
          cbResults.begin(), cbResults.end());
    if (!cbRemoveRequest.empty())
      resultQueue.removableRequests.insert(
          resultQueue.removableRequests.end(),
          cbRemoveRequest.begin(), cbRemoveRequest.end());
  };
  //
  //
  mCbRcmder->notifyShutters(reqeustQueue.availableShutters, rcmd_cb);
  mCbRcmder->notifyErrors(reqeustQueue.availableErrors, rcmd_cb);
  mCbRcmder->receiveMetadataResults(reqeustQueue.availableResults, rcmd_cb);
  mCbRcmder->receiveBuffers(reqeustQueue.availableBuffers, rcmd_cb);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateMetaCb(
    CaptureResult& rResult,
    std::shared_ptr<FrameParcel>& rpCbParcel,
    RecommenderQueue& requestQueue,
    FrameParcel& cbList) -> void {
  if (rResult.v3_2.result.size() == 0)
    return;
  uint32_t requestNo = rResult.v3_2.frameNumber;
  // create ::V3_4::CaptureResult
  CaptureResult metaResult{
    .v3_2.frameNumber = requestNo,
    .v3_2.result = rResult.v3_2.result,
    .v3_2.partialResult = queryPartialCount(rResult, rpCbParcel),
    .v3_2.inputBuffer = StreamBuffer{.streamId = -1},
  };
  // create ICallbackRecommender::CaptureResult
  bool isNeedAskRecommender =
      rResult.v3_2.partialResult == mCommonInfo->mAtMostMetaStreamCount;
  if (isNeedAskRecommender) {
    ICallbackRecommender::CaptureResult cr_CaptureResult;
    cr_CaptureResult.requestNumber = requestNo;
    cr_CaptureResult.hasLastPartial = true;
    requestQueue.availableResults.push_back(cr_CaptureResult);
    rpCbParcel->vResult.push_back(metaResult);
  } else {
    // directly send to available parcel queue w/o asking recommender
    cbList.vResult.push_back(std::move(metaResult));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updatePhysicMetaCb(
    CaptureResult& rResult,
    std::shared_ptr<FrameParcel>& rpCbParcel,
    RecommenderQueue& requestQueue) -> void {
  if (rResult.physicalCameraMetadata.size() == 0)
    return;
  // create ::V3_4::CaptureResult
  CaptureResult physicMetaResult{
    .v3_2.frameNumber = rResult.v3_2.frameNumber,
    .v3_2.partialResult = (uint32_t)mCommonInfo->mAtMostMetaStreamCount,
    .v3_2.inputBuffer = StreamBuffer{.streamId = -1},
    .physicalCameraMetadata = rResult.physicalCameraMetadata,
  };
  rpCbParcel->vResult.push_back(physicMetaResult);
  // create ICallbackRecommender::CaptureResult
  ICallbackRecommender::CaptureResult cr_CaptureResult;
  cr_CaptureResult.requestNumber = rResult.v3_2.frameNumber;
  for (const auto& item : rResult.physicalCameraMetadata) {
    cr_CaptureResult.physicalCameraIdsOfPhysicalResults.push_back(
        atoi(item.physicalCameraId.c_str()));
  }
  requestQueue.availableResults.push_back(cr_CaptureResult);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateImageCb(
    CaptureResult& rResult,
    std::shared_ptr<FrameParcel>& rpCbParcel,
    RecommenderQueue& requestQueue) -> void {
  /**
   * For image callback, each image buffer is hold by a single CaptureResult
   */
  auto isValid = [&](uint32_t streamId) -> bool {
    Mutex::Autolock _l(mAppStreamLock);
    for (const auto& item : mAppStreams) {
      if (streamId == item.v3_2.id)
        return true;
    }
    return false;
  };
  //
  // input buffers
  //
  int32_t inputStreamId = rResult.v3_2.inputBuffer.streamId;
  if (inputStreamId != -1) {
    if (!isValid(inputStreamId)) {
      MY_LOGW("recognized streamId :%#" PRIx64 ", maybe not App Stream",
              inputStreamId);
    }
    // create ::V3_4::CaptureResult
    //
    // input buffers
    //
    CaptureResult inputImageResult{
      .v3_2.frameNumber = rResult.v3_2.frameNumber,
      .v3_2.partialResult = 0,
      .v3_2.inputBuffer = rResult.v3_2.inputBuffer,
    };
    rpCbParcel->vResult.push_back(inputImageResult);
    // create ICallbackRecommender::CaptureResult
    ICallbackRecommender::Stream inputStream {
      .streamId = inputStreamId,
      .bufferStatus = rResult.v3_2.inputBuffer.status == BufferStatus::OK ?
                      ICallbackRecommender::BufferStatus::eBUFFER_OK :
                      ICallbackRecommender::BufferStatus::eBUFFER_ERROR,
    };
    std::vector<ICallbackRecommender::Stream> vInputStreams;
    vInputStreams.push_back(inputStream);
    ICallbackRecommender::CaptureResult cr_CaptureResult{
      .requestNumber = rResult.v3_2.frameNumber,
      .vInputStreams = vInputStreams,
    };
    requestQueue.availableBuffers.push_back(std::move(cr_CaptureResult));
  }
  //
  // output buffers
  //
  for (const auto& item : rResult.v3_2.outputBuffers) {
    int32_t outputStreamId = item.streamId;
    if (!isValid(outputStreamId)) {
      MY_LOGW("recognized streamId :%#" PRIx64 ", maybe not App Stream",
              outputStreamId);
    }
    // create ::V3_4::CaptureResult
    hidl_vec<StreamBuffer> outputBuffer;
    outputBuffer.resize(1);
    outputBuffer[0] = item;
    CaptureResult outputImageResult{
      .v3_2.frameNumber = rResult.v3_2.frameNumber,
      .v3_2.partialResult = 0,
      .v3_2.outputBuffers = outputBuffer,
      .v3_2.inputBuffer = StreamBuffer{.streamId = -1},
    };
    rpCbParcel->vResult.push_back(std::move(outputImageResult));
    // create ICallbackRecommender::CaptureResult
    ICallbackRecommender::Stream outputStream {
      .streamId = outputStreamId,
      .bufferStatus = rResult.v3_2.inputBuffer.status == BufferStatus::OK ?
                      ICallbackRecommender::BufferStatus::eBUFFER_OK :
                      ICallbackRecommender::BufferStatus::eBUFFER_ERROR,
    };
    std::vector<ICallbackRecommender::Stream> vOutputStreams;
    vOutputStreams.push_back(outputStream);
    ICallbackRecommender::CaptureResult cr_CaptureResult{
      .requestNumber = rResult.v3_2.frameNumber,
      .vOutputStreams = vOutputStreams,
    };
    requestQueue.availableBuffers.push_back(std::move(cr_CaptureResult));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateShutterCb(
    NotifyMsg& rMsg,
    std::shared_ptr<FrameParcel>& rpCbParcel,
    RecommenderQueue& requestQueue) -> void {
  if (rMsg.type != MsgType::SHUTTER)
    return;
  // create ::V3_2::NotifyMsg
  rpCbParcel->vMsg.push_back(rMsg);
  // create ICallbackRecommender::CaptureResult
  requestQueue.availableShutters.push_back(rMsg.msg.shutter.frameNumber);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateErrorCb(
    NotifyMsg& rMsg,
    std::shared_ptr<FrameParcel>& rpCbParcel,
    RecommenderQueue& requestQueue) -> void {
  if (rMsg.type != MsgType::ERROR)
    return;
  // create ::V3_2::NotifyMsg
  rpCbParcel->vMsg.push_back(rMsg);
  // create ICallbackRecommender::ErrorMessage
  ICallbackRecommender::ErrorMessage cr_msg {
    .requestNumber = rMsg.msg.error.frameNumber,
  };
  ErrorCode resError = rMsg.msg.error.errorCode;
  if (resError == ErrorCode::ERROR_BUFFER) {
    cr_msg.errorType = ICallbackRecommender::ErrorType::eERROR_BUFFER;
    cr_msg.errorStreamId = rMsg.msg.error.errorStreamId;
  } else if (resError == ErrorCode::ERROR_RESULT) {
    cr_msg.errorType = ICallbackRecommender::ErrorType::eERROR_RESULT;
  } else if (resError == ErrorCode::ERROR_REQUEST) {
    cr_msg.errorType = ICallbackRecommender::ErrorType::eERROR_REQUEST;
  } else {
    MY_LOGE("no cr_msg in the msg");
  }
  requestQueue.availableErrors.push_back(std::move(cr_msg));
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateCallback(
  RecommenderQueue& recommendQueue,
  FrameParcel& rCbList) -> void {
  //
  for (const auto& item : recommendQueue.availableShutters) {
    prepareShutterNotificationIfPossible(rCbList, item);
  }
  //
  for (const auto& item : recommendQueue.availableErrors) {
    prepareErrorNotificationIfPossible(rCbList, item);
  }
  //
  for (const auto& item : recommendQueue.availableResults) {
    prepareMetaCallbackIfPossible(rCbList, item);
  }
  //
  for (const auto& item : recommendQueue.availableBuffers) {
    prepareImageCallbackIfPossible(rCbList, item);
  }
  //
  for (const auto& item : recommendQueue.removableRequests) {
    std::shared_ptr<FrameParcel> pParcel = nullptr;
    if (!getSafeFrameParcel(item, pParcel)) {
      MY_LOGW("reqeustNo %u, remove parcel failed", item);
      continue;
    }
    // remove this frame from the frame queue.
    mPendingParcels.erase(item);
    MY_LOGD("remove requestNo %u from mPendingParcels", item);
  }
  //
  if (mCommonInfo->mLogLevel >= 1) {
    String8 log = String8::format("mPendingParcels : ");
    for (const auto& frame : mPendingParcels) {
      log += String8::format("%u:[%zu/%zu] ", frame.first,
                            frame.second->vResult.size(),
                            frame.second->vMsg.size());
    }
    MY_LOGD("%s", log.c_str());
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareShutterNotificationIfPossible(
    FrameParcel& rCbList,
    const uint32_t& requestNo) -> bool {
  std::shared_ptr<FrameParcel> pParcel = nullptr;
  if (!getSafeFrameParcel(requestNo, pParcel)) {
    MY_LOGW("reqeustNo %u, prepare shutter failed", requestNo);
    return false;
  }
  auto& rPendingShutter = pParcel->vMsg;
  auto it = rPendingShutter.begin();
  while (it != rPendingShutter.end()) {
    if ((*it).type == MsgType::SHUTTER) {
      rCbList.vMsg.push_back(*it);
      it = rPendingShutter.erase(it);
      break;
    } else {
      it++;
    }
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareErrorNotificationIfPossible(
    FrameParcel& rCbList,
    const ICallbackRecommender::ErrorMessage& error) -> bool {
  uint32_t requestNo = error.requestNumber;
  std::shared_ptr<FrameParcel> pParcel = nullptr;
  if (!getSafeFrameParcel(requestNo, pParcel)) {
    MY_LOGW("reqeustNo %u, prepare error failed", requestNo);
    return false;
  }
  // message is a liitle different, when notify CallbackRecommender ERROR_REQUEST
  // it will output BUFFER_ERROR first, then output ERROR_REQUEST.
  //
  // - always create error message same as CallbackRecommder's output.
  NotifyMsg errorMsg {
    .type = MsgType::ERROR,
    .msg.error.frameNumber = requestNo,
    .msg.error.errorStreamId = (int32_t)error.errorStreamId,
  };
  if (error.errorType == ICallbackRecommender::ErrorType::eERROR_RESULT) {
    errorMsg.msg.error.errorCode = ErrorCode::ERROR_RESULT;
  } else if (error.errorType == ICallbackRecommender::ErrorType::eERROR_REQUEST) {
    errorMsg.msg.error.errorCode = ErrorCode::ERROR_REQUEST;
  } else if (error.errorType == ICallbackRecommender::ErrorType::eERROR_BUFFER) {
    errorMsg.msg.error.errorCode = ErrorCode::ERROR_BUFFER;
  } else {
    MY_LOGE("unknown error type from recommender");
  }
  rCbList.vMsg.push_back(errorMsg);
  // remove from pending parcels
  auto& rPendingMsg = pParcel->vMsg;
  auto it = rPendingMsg.begin();
  while (it != rPendingMsg.end()) {
    if ((*it).type == MsgType::ERROR) {
      it = rPendingMsg.erase(it);
      break;
    } else {
      it++;
    }
  }
  // remove from pendingMsg
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareMetaCallbackIfPossible(
    FrameParcel& rCbList,
    const ICallbackRecommender::CaptureResult& rAvailableResult) -> bool {
  uint32_t requestNo = rAvailableResult.requestNumber;
  std::shared_ptr<FrameParcel> pParcel = nullptr;
  if (!getSafeFrameParcel(requestNo, pParcel)) {
    MY_LOGW("reqeustNo %u, prepare Meta failed", requestNo);
    return false;
  }
  auto& rPendingRes = pParcel->vResult;
  auto it = rPendingRes.begin();
  while (it != rPendingRes.end()) {
    bool bLastPartial = rAvailableResult.hasLastPartial &&
                        (*it).v3_2.partialResult == mCommonInfo->mAtMostMetaStreamCount;
    bool bHasPhysic = !rAvailableResult.physicalCameraIdsOfPhysicalResults.empty() &&
                      (*it).physicalCameraMetadata.size() > 0;
    if (bLastPartial || bHasPhysic) {
      rCbList.vResult.push_back(*it);
      it = rPendingRes.erase(it);
      break;
    } else {
      it++;
    }
  }
  return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareImageCallbackIfPossible(
    FrameParcel& rCbList,
    const ICallbackRecommender::CaptureResult& rAvailableResult) -> bool {
  std::shared_ptr<FrameParcel> pParcel = nullptr;
  uint32_t requestNo = rAvailableResult.requestNumber;
  if (!getSafeFrameParcel(requestNo, pParcel)) {
    MY_LOGW("reqeustNo %u, prepare image failed", requestNo);
    return false;
  }
  auto isAvailable = [&](CaptureResult& result) -> const bool {
    // check input available
    for (const auto& stream : rAvailableResult.vInputStreams) {
      if (stream.streamId == result.v3_2.inputBuffer.streamId)
        return true;
    }
    // check output available
    for (const auto& stream : rAvailableResult.vOutputStreams) {
      for (const auto& buf : result.v3_2.outputBuffers) {
        if (stream.streamId == buf.streamId)
          return true;
      }
    }
    return false;
  };
  //
  auto& rPendingRes = pParcel->vResult;
  auto it = rPendingRes.begin();
  while (it != rPendingRes.end()) {
    if (isAvailable(*it)) {
      rCbList.vResult.push_back(*it);
      it = rPendingRes.erase(it);
      break;
    } else {
      it++;
    }
  }
  return true;
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
  if (OK != (err = waitEmpty(mPendingParcelLock, mPendingParcelCond, mPendingParcels))) {
    MY_LOGW("mPendingParcels:#%zu timeout(ns):%" PRId64 " elapsed(ns):%" PRId64
            " err:%d(%s)",
            mPendingParcels.size(), timeout, (::systemTime() - startTime), err,
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
  printer.printLine("");
  printer.printLine(" *In-flight requests*");
  for (auto const& item : mPendingParcels) {
    for (auto const& res : item.second->vResult) {
      MY_LOGD("%s", dumpCaptureResult(res).c_str());
    }
  }
  for (auto const& item : mPendingParcels) {
    for (auto const& msg : item.second->vMsg) {
      MY_LOGD("%s", dumpNotifyMsg(msg).c_str());
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpLocked() const -> void {
  ULogPrinter p(__ULOG_MODULE_ID, LOG_TAG);
  dumpStateLocked(p);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dump() const -> void {
  Mutex::Autolock _l(mPendingParcelLock);
  dumpLocked();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer,
                              const std::vector<std::string> &
                              /*options*/) -> void {
  Mutex::Autolock _l(mPendingParcelLock);
  dumpStateLocked(printer);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpIfHasInflightRequest() const -> void {
  Mutex::Autolock _l(mPendingParcelLock);
  if (mPendingParcels.size())
    dumpLocked();
}
};  // namespace v3
};  // namespace NSCam
