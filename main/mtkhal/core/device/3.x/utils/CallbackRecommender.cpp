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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#define LOG_TAG "mtkcam-cb"

#include <main/mtkhal/core/device/3.x/utils/include/CallbackRecommender.h>

#include <cutils/properties.h>
#include <mtkcam/utils/std/ULog.h>  // will include <log/log.h> if LOG_TAG was defined

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
                    CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
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

#if 0
#define FUNC_START MY_LOGD("+")
#define FUNC_END MY_LOGD("-")
#else
#define FUNC_START
#define FUNC_END
#endif

namespace mcam {
namespace core {
namespace Utils {

/******************************************************************************
 *  Instaintiate.
 ******************************************************************************/
auto ICallbackRecommender::createInstance(CreationInfo& creationInfo)
    -> std::shared_ptr<ICallbackRecommender> {
  auto pInstance = std::make_shared<CallbackRecommender>(creationInfo);
  if (!pInstance) {
    return nullptr;
  }
  return std::static_pointer_cast<ICallbackRecommender>(pInstance);
}

/******************************************************************************
 *
 ******************************************************************************/
CallbackRecommender::CallbackRecommender(const CreationInfo& creationInfo)
    : ICallbackRecommender(),
      mPartialResultCount(creationInfo.partialResultCount),
      mLogLevel(
          property_get_int32("vendor.camera.debug.callbackRecommender", 0)),
      mShutterQueueMap(StreamType::OUTPUT),
      mResultQueueMap(StreamType::OUTPUT),
      mPhysicalResultQueueMapSet(),
      mBufferQueueMapSet(),
      mRequestMap() {
  MY_LOGD("ctor");
}

/******************************************************************************
 *
 ******************************************************************************/
CallbackRecommender::~CallbackRecommender() {
  MY_LOGD("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
CallbackRecommender::Item::~Item() {
  MY_LOGD_IF(0, "dtor request %" PRIu32 "", this->frameNumber);
}

/******************************************************************************
 *  Configuration.
 ******************************************************************************/
auto CallbackRecommender::configureStreams(
    const StreamConfiguration& streamInfos) -> int {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  mNextRequestNumber = 0;
  // clear queue
  mShutterQueueMap.reset();
  mResultQueueMap.reset();
  mPhysicalResultQueueMapSet.clear();
  mBufferQueueMapSet.clear();
  mRequestMap.clear();
  for (auto& stream : streamInfos.streams) {
    mBufferQueueMapSet.insert(std::pair<int64_t, ItemQueueMap>(
        stream.id, ItemQueueMap(stream.streamType)));
    MY_LOGD_IF(mLogLevel >= 1, "add %s streamId %" PRId64 "",
               stream.streamType == StreamType::OUTPUT ? "output" : "input",
               stream.id);
    if (stream.physicalCameraId != -1) {
      mPhysicalResultQueueMapSet.insert(std::pair<int64_t, ItemQueueMap>(
          stream.physicalCameraId, ItemQueueMap(StreamType::OUTPUT)));
      MY_LOGD_IF(mLogLevel >= 1, "add physicalCameraId %" PRId64 "",
                 stream.physicalCameraId);
    }
  }
  FUNC_END;
  return 0;
}

/******************************************************************************
 *  Request.
 ******************************************************************************/
auto CallbackRecommender::registerRequests(
    const std::vector<CaptureRequest>& requests) -> int {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  //
  for (auto& captureRequest : requests) {
    auto& number = captureRequest.frameNumber;
    auto& type = captureRequest.requestType;
    MY_LOGD_IF(mLogLevel >= 1, "register %" PRIu32 " with type %d",
               number, type);
    if (mNextRequestNumber == number) {
      mNextRequestNumber++;
    } else {
      MY_LOGE("Requests registered out-of-order, "
              "next should be %" PRIu32 " but get %" PRIu32 "",
              mNextRequestNumber, number);
      mNextRequestNumber = number + 1;
    }
    Request request = {
        .requestType = type,
    };
    // shutter
    {
      auto& queue = mShutterQueueMap.at(type);
      auto pItem = std::make_shared<Item>(number);
      queue.insert(std::pair<uint32_t, std::shared_ptr<Item>>(number, pItem));
      request.pShutter = pItem;
    }
    // result
    {
      auto& queue = mResultQueueMap.at(type);
      auto pItem = std::make_shared<Item>(number);
      queue.insert(std::pair<uint32_t, std::shared_ptr<Item>>(number, pItem));
      request.pResult = pItem;
    }
    // physical result
    for (auto& element : mPhysicalResultQueueMapSet) {
      auto& id = element.first;
      auto& queue = mPhysicalResultQueueMapSet.at(id).at(type);
      auto pItem = std::make_shared<Item>(number);
      queue.insert(std::pair<uint32_t, std::shared_ptr<Item>>(number, pItem));
      request.pPhysicalResultMap.insert(
          std::pair<int64_t, std::shared_ptr<Item>>(id, pItem));
      MY_LOGD_IF(mLogLevel >= 2, "physicalCameraId %" PRId64 "", id);
    }
    // input buffer
    for (auto& id : captureRequest.vInputStreamIds) {
      auto& queue = mBufferQueueMapSet.at(id).at(type);
      auto pItem = std::make_shared<Item>(number);
      queue.insert(std::pair<uint32_t, std::shared_ptr<Item>>(number, pItem));
      request.pBufferMap.insert(
          std::pair<int64_t, std::shared_ptr<Item>>(id, pItem));
      MY_LOGD_IF(mLogLevel >= 2, "inputStreamId %" PRId64 "", id);
    }
    // output buffer
    for (auto& id : captureRequest.vOutputStreamIds) {
      auto& queue = mBufferQueueMapSet.at(id).at(type);
      auto pItem = std::make_shared<Item>(number);
      queue.insert(std::pair<uint32_t, std::shared_ptr<Item>>(number, pItem));
      request.pBufferMap.insert(
          std::pair<int64_t, std::shared_ptr<Item>>(id, pItem));
      MY_LOGD_IF(mLogLevel >= 2, "outputStreamId %" PRId64 "", id);
    }
    //
    mRequestMap.insert(std::pair<uint32_t, Request>(number, request));
  }
  FUNC_END;
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::removeRequests(
    const std::vector<uint32_t>& requestNumbers,
    availableCallback_cb _available_cb_ __attribute__((unused))) -> int {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  //
  std::string numberString = "";
  for (auto& number : requestNumbers) {
    numberString += " " + std::to_string(number);
  }
  removeRequestsAndItems(requestNumbers);
  MY_LOGW("Requests%s are removed by user", numberString.c_str());
  // debug
  if (mLogLevel >= 2) {
    dumpAllAvailable({}, {}, {}, {}, requestNumbers);
  }
  FUNC_END;
  return 0;
}

/******************************************************************************
 *  Notify.
 ******************************************************************************/
auto CallbackRecommender::notifyErrors(
    const std::vector<ErrorMsg>& errorMessages,
    availableCallback_cb _available_cb_) -> void {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  //
  std::vector<uint32_t> availableShutters;
  std::vector<ErrorMsg> availableErrors;
  std::vector<CaptureResult> availableBuffers;
  std::vector<CaptureResult> availableResults;
  std::vector<uint32_t> removableRequests;
  //
  for (auto& message : errorMessages) {
    auto& number = message.frameNumber;
    auto& request = mRequestMap.at(number);
    MY_LOGI("input number=%" PRIu32 " id=%" PRId64 " error=%d", number,
            message.errorStreamId, message.errorCode);
    //
    switch (message.errorCode) {
      case ErrorCode::ERROR_DEVICE:
        MY_LOGE("Error device, request %" PRIu32 "", number);
        availableErrors.push_back(message);
        break;
      case ErrorCode::ERROR_REQUEST:
        MY_LOGW("Error request, request %" PRIu32 "", number);
        request.pShutter->status |= eITEM_ERROR;
        request.pResult->status |= eITEM_ERROR;
        // for (auto& element : request.pPhysicalResultMap) {
        //     element.second->status |= eITEM_ERROR;
        // }
        for (auto& element : request.pBufferMap) {
          element.second->status |= eITEM_NOTIFING;
        }
        break;
      case ErrorCode::ERROR_RESULT:
        request.pShutter->status |= eITEM_ERROR;
        request.pResult->status |= eITEM_ERROR;
        // for (auto& element : request.pPhysicalResultMap) {
        //     element.second->status |= eITEM_ERROR;
        // }
        break;
      case ErrorCode::ERROR_BUFFER:
        request.pBufferMap.at(message.errorStreamId)->status |= eITEM_NOTIFING;
        break;
      default:
        MY_LOGE("Unknown error type %d.", message.errorCode);
        break;
    }
    prepareCallbackShutters(number, availableShutters);
    prepareCallbackErrors(number, availableErrors);
    prepareCallbackBuffers(number, availableBuffers);
    prepareCallbackResults(number, availableResults);
    prepareRemoveRequests(number, removableRequests);
  }
  // debug
  if (mLogLevel >= 2) {
    dumpAllAvailable(availableShutters, availableErrors, availableBuffers,
                     availableResults, removableRequests);
  }
  // callback
  _available_cb_(availableShutters, availableErrors, availableBuffers,
                 availableResults, removableRequests);
  FUNC_END;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::notifyShutters(
    const std::vector<uint32_t>& requestNumbers,
    availableCallback_cb _available_cb_) -> void {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  //
  std::vector<uint32_t> availableShutters;
  std::vector<ErrorMsg> availableErrors;
  std::vector<CaptureResult> availableBuffers;
  std::vector<CaptureResult> availableResults;
  std::vector<uint32_t> removableRequests;
  //
  for (auto& number : requestNumbers) {
    MY_LOGD_IF(mLogLevel >= 1, "input number=%" PRIu32 "", number);
    if (mRequestMap.find(number) == mRequestMap.end()) {
      MY_LOGE("Request %" PRIu32 " is not found", number);
      continue;
    }
    auto& request = mRequestMap.at(number);
    auto& status = request.pShutter->status;
    MY_LOGD_IF(mLogLevel >= 1, "number=%" PRIu32 " status=%d", number, status);
    if (status != eITEM_INFLIGHT) {
      MY_LOGW_IF(mLogLevel >= 1,
                 "Ignore shutter of request %" PRIu32 ", status is %d", number,
                 status);
      continue;
    }
    status |= eITEM_VALID;
    //
    prepareCallbackShutters(number, availableShutters);
    prepareRemoveRequests(number, removableRequests);
  }
  // debug
  if (mLogLevel >= 2) {
    dumpAllAvailable(availableShutters, availableErrors, availableBuffers,
                     availableResults, removableRequests);
  }
  // callback
  _available_cb_(availableShutters, availableErrors, availableBuffers,
                 availableResults, removableRequests);
  FUNC_END;
}

/******************************************************************************
 *  ProcessCaptureResult.
 ******************************************************************************/
auto CallbackRecommender::receiveMetadataResults(
    const std::vector<CaptureResult>& results,
    availableCallback_cb _available_cb_) -> void {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  //
  std::vector<uint32_t> availableShutters;
  std::vector<ErrorMsg> availableErrors;
  std::vector<CaptureResult> availableBuffers;
  std::vector<CaptureResult> availableResults;
  std::vector<uint32_t> removableRequests;
  //
  for (auto& result : results) {
    auto& number = result.frameNumber;
    if (mRequestMap.find(number) == mRequestMap.end()) {
      MY_LOGE("Request %" PRIu32 " is not found", number);
      continue;
    }
    auto& request = mRequestMap.at(number);
    auto& status = request.pResult->status;
    MY_LOGD_IF(mLogLevel >= 1, "input number=%" PRIu32 " requestStatus=%d",
               number, status);
    // result
    if (result.hasLastPartial) {
      if (status != eITEM_INFLIGHT) {
        MY_LOGW_IF(mLogLevel >= 1,
                   "Ignore result of request %" PRIu32 ", status is %d", number,
                   status);
      } else {
        status |= eITEM_VALID;
      }
    }
    // physicalResult
    auto& physicalResultIds = result.physicalCameraIdsOfPhysicalResults;
    if (physicalResultIds.size() > 0) {
      for (auto& id : physicalResultIds) {
        auto& physicalStatus = request.pPhysicalResultMap.at(id)->status;
        if (physicalStatus != eITEM_INFLIGHT) {
          MY_LOGW_IF(mLogLevel >= 1,
                     "Ignore physical result of request %" PRIu32 ", "
                     "status is %d",
                     number, status);
        } else {
          physicalStatus |= eITEM_VALID;
        }
      }
    }
    //
    prepareCallbackResults(number, availableResults);
    prepareRemoveRequests(number, removableRequests);
  }
  // debug
  if (mLogLevel >= 2) {
    dumpAllAvailable(availableShutters, availableErrors, availableBuffers,
                     availableResults, removableRequests);
  }
  // callback
  _available_cb_(availableShutters, availableErrors, availableBuffers,
                 availableResults, removableRequests);
  FUNC_END;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::receiveBuffers(
    const std::vector<CaptureResult>& results,
    availableCallback_cb _available_cb_) -> void {
  FUNC_START;
  std::lock_guard<std::mutex> guard(mFunctionMutex);
  //
  std::vector<uint32_t> availableShutters;
  std::vector<ErrorMsg> availableErrors;
  std::vector<CaptureResult> availableBuffers;
  std::vector<CaptureResult> availableResults;
  std::vector<uint32_t> removableRequests;
  //
  for (auto& result : results) {
    auto& number = result.frameNumber;
    if (mRequestMap.find(number) == mRequestMap.end()) {
      MY_LOGE("Request %" PRIu32 " is not found", number);
      continue;
    }
    auto& request = mRequestMap.at(number);
    for (auto& buffer : result.vInputStreams) {
      auto& id = buffer.streamId;
      MY_LOGD_IF(mLogLevel >= 1,
                 "input number=%" PRIu32 " id=%" PRId64 " "
                 "status=%d requestStatus=%d",
                 number, id, buffer.status,
                 request.pBufferMap.at(id)->status);
      updateBufferStatus(number, request, buffer);
    }
    for (auto& buffer : result.vOutputStreams) {
      auto& id = buffer.streamId;
      MY_LOGD_IF(mLogLevel >= 1,
                 "input number=%" PRIu32 " id=%" PRId64 " "
                 "status=%d requestStatus=%d",
                 number, id, buffer.status,
                 request.pBufferMap.at(id)->status);
      updateBufferStatus(number, request, buffer);
    }
    //
    prepareCallbackErrors(number, availableErrors);
    prepareCallbackBuffers(number, availableBuffers);
    prepareRemoveRequests(number, removableRequests);
  }
  // debug
  if (mLogLevel >= 2) {
    dumpAllAvailable(availableShutters, availableErrors, availableBuffers,
                     availableResults, removableRequests);
  }
  // callback
  _available_cb_(availableShutters, availableErrors, availableBuffers,
                 availableResults, removableRequests);
  FUNC_END;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::prepareCallbackShutters(
    const uint32_t& targetNumber,
    std::vector<uint32_t>& availableShutters) -> void {
  Request& request = mRequestMap.at(targetNumber);
  auto& queue = mShutterQueueMap.at(request.requestType);
  for (auto it = queue.begin(); it != queue.end(); ++it) {
    auto& number = it->second->frameNumber;
    auto& status = it->second->status;
    MY_LOGD_IF(mLogLevel >= 3, "number=%" PRIu32 " status=%d", number, status);
    if (status & eITEM_RELEASED) {
      // continue;
    } else if (status & eITEM_ERROR) {
      status |= eITEM_RELEASED;
      // ignore callback
      // continue;
    } else if (status & eITEM_VALID) {
      availableShutters.push_back(number);
      status |= eITEM_RELEASED;
    } else if (status == eITEM_INFLIGHT) {
      auto& targetStatus = request.pShutter->status;
      if (targetStatus & eITEM_RELEASED || targetStatus == eITEM_INFLIGHT) {
        // do nothing
      } else if (targetStatus & eITEM_ERROR) {
        // ignore error shutter
        targetStatus |= eITEM_RELEASED;
      } else {
        MY_LOGD_IF(number < targetNumber,
                   "Shutter of request %" PRIu32
                   " is blocked by request %" PRIu32 "",
                   targetNumber, number);
      }
      break;
    } else {
      MY_LOGE("Unexpected status, target=%" PRIu32 " current=%" PRIu32 " "
              "status=%d",
              targetNumber, number, status);
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::prepareCallbackErrors(
    const uint32_t& targetNumber,
    std::vector<ErrorMsg>& availableErrors) -> void {
  Request& request = mRequestMap.at(targetNumber);
  RequestType& type = request.requestType;
  bool allBufferError = true;
  bool oneBufferValid = false;
  // buffer
  for (auto& element : request.pBufferMap) {
    auto& id = element.first;
    auto& status = element.second->status;
    MY_LOGD_IF(mLogLevel >= 3, "number=%" PRIu32 " status=%d", targetNumber,
               status);
    if (status & eITEM_NOTIFIED) {
      // do nothing
    } else if (status & (eITEM_ERROR | eITEM_NOTIFING)) {
      ErrorMsg message = {
          .frameNumber = targetNumber,
          .errorStreamId = id,
          .errorCode = ErrorCode::ERROR_BUFFER,
      };
      availableErrors.push_back(message);
      status |= eITEM_NOTIFIED;
    } else if (status & eITEM_VALID) {
      allBufferError = false;
      oneBufferValid = true;
    } else if (status == eITEM_INFLIGHT) {
      allBufferError = false;
    }
  }
  // result
  auto& status = request.pResult->status;
  if ((status & eITEM_ERROR) && !(status & eITEM_NOTIFIED)) {
    ErrorMsg message = {
        .frameNumber = targetNumber,
    };
    if (allBufferError) {
      message.errorCode = ErrorCode::ERROR_REQUEST;
      availableErrors.push_back(message);
      status |= eITEM_NOTIFIED;
    } else if (oneBufferValid) {
      message.errorCode = ErrorCode::ERROR_RESULT;
      availableErrors.push_back(message);
      status |= eITEM_NOTIFIED;
    } else {
      // pending error result
      MY_LOGI("request %" PRIu32 " pending error result", targetNumber);
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::prepareCallbackBuffers(
    const uint32_t& targetNumber,
    std::vector<CaptureResult>& availableBuffers) -> void {
  Request& request = mRequestMap.at(targetNumber);
  RequestType& type = request.requestType;
  std::map<uint32_t, std::vector<Stream>> availableBufferMap;
  for (auto& element : request.pBufferMap) {
    auto& id = element.first;
    auto& targetStatus = element.second->status;
    auto& queue = mBufferQueueMapSet.at(id).at(type);
    for (auto it = queue.begin(); it != queue.end(); ++it) {
      auto& number = it->second->frameNumber;
      auto& status = it->second->status;
      MY_LOGD_IF(mLogLevel >= 3,
                 "number=%" PRIu32 " buffer=%" PRId64 " status=%d",
                 number, id, status);
      if (status & eITEM_RELEASED) {
        continue;
      } else if (status & eITEM_ERROR) {
        Stream stream = {.streamId = id, .status = BufferStatus::ERROR};
        availableBufferMap[number].push_back(stream);
        status |= eITEM_RELEASED;
      } else if (status & eITEM_VALID) {
        Stream stream = {.streamId = id, .status = BufferStatus::OK};
        availableBufferMap[number].push_back(stream);
        status |= eITEM_RELEASED;
      } else if (status == eITEM_INFLIGHT) {
        if (targetStatus & eITEM_RELEASED || targetStatus == eITEM_INFLIGHT) {
          // do nothing
        } else if (targetStatus & eITEM_ERROR) {
          // camera3.h: flush: 3.6
          // failed buffers do not have to follow the strict ordering valid
          // buffers do.
          Stream stream = {.streamId = id, .status = BufferStatus::ERROR};
          availableBufferMap[targetNumber].push_back(stream);
          targetStatus |= eITEM_RELEASED;
        } else if (targetStatus & eITEM_VALID) {
          MY_LOGD_IF(number < targetNumber,
                     "Buffer %" PRId64 " of request %" PRIu32 " "
                     "is blocked by request %" PRIu32 "",
                     id, targetNumber, number);
        }
        break;
      } else if (status & eITEM_NOTIFING) {
        continue;
      } else {
        MY_LOGE("Unexpected status, "
                "target=%" PRIu32 " current=%" PRIu32 " status=%d",
                targetNumber, number, status);
      }
    }
  }
  //
  for (auto& element : availableBufferMap) {
    auto& number = element.first;
    CaptureResult captureResult = {
        .frameNumber = number,
    };
    for (auto& stream : element.second) {
      auto& streamType = mBufferQueueMapSet.at(stream.streamId).streamType;
      if (streamType == StreamType::OUTPUT) {
        captureResult.vOutputStreams.push_back(stream);
      } else if (streamType == StreamType::INPUT) {
        captureResult.vInputStreams.push_back(stream);
      } else {
        MY_LOGE("Unexpected stream type, "
                "request=%" PRIu32 " id=%" PRId64 " type=%d",
                number, stream.streamId, streamType);
      }
    }
    availableBuffers.push_back(captureResult);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::prepareCallbackResults(
    const uint32_t& targetNumber,
    std::vector<CaptureResult>& availableResults) -> void {
  Request& request = mRequestMap.at(targetNumber);
  RequestType& type = request.requestType;
  auto& queue = mResultQueueMap.at(type);
  for (auto it = queue.begin(); it != queue.end(); ++it) {
    auto& number = it->second->frameNumber;
    auto& status = it->second->status;
    auto& physicalResultMap = mRequestMap.at(number).pPhysicalResultMap;
    MY_LOGD_IF(mLogLevel >= 3, "number=%" PRIu32 " status=%d", number, status);
    if (status & eITEM_RELEASED) {
      // continue;
    } else if (status & eITEM_ERROR) {
      status |= eITEM_RELEASED;
    } else if (status & eITEM_VALID) {
      bool allPhysicalResultValid = true;
      CaptureResult captureResult = {
          .frameNumber = number,
          .hasLastPartial = true,
      };
      auto& capturePhysicalResults =
          captureResult.physicalCameraIdsOfPhysicalResults;
      for (auto& element : physicalResultMap) {
        auto& physicalId = element.first;
        auto& physicalStatus = element.second->status;
        if (physicalStatus == eITEM_INFLIGHT) {
          allPhysicalResultValid = false;
        } else if (physicalStatus & eITEM_VALID) {
          capturePhysicalResults.push_back(physicalId);
        } else {
          MY_LOGE("Unexpected physical status, "
                  "number=%" PRIu32 " id=%" PRId64 " status=%d",
                  number, physicalId, status);
        }
      }
      if (allPhysicalResultValid) {
        availableResults.push_back(captureResult);
        status |= eITEM_RELEASED;
        for (auto& element : physicalResultMap) {
          element.second->status |= eITEM_RELEASED;
        }
      } else {
        MY_LOGD_IF((mLogLevel >= 2) && (status & eITEM_VALID),
                   "Waiting for physical result, "
                   "request=%" PRIu32 " physicalResult=%zu/%zu",
                   number, capturePhysicalResults.size(),
                   physicalResultMap.size());
        break;
      }
    } else if (status == eITEM_INFLIGHT) {
      auto& targetStatus = request.pResult->status;
      if (targetStatus & eITEM_RELEASED || targetStatus == eITEM_INFLIGHT) {
        // do nothing
      } else if (targetStatus & eITEM_ERROR) {
        // ignore error result
        targetStatus |= eITEM_RELEASED;
      } else {
        MY_LOGD_IF(number < targetNumber,
                   "Result of request %" PRIu32
                   " is blocked by request %" PRIu32 "",
                   targetNumber, number);
      }
      break;
    } else {
      MY_LOGE("Unexpected status, "
              "target=%" PRIu32 " current=%" PRIu32 " status=%d",
              targetNumber, number, status);
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::prepareRemoveRequests(
    const uint32_t& targetNumber,
    std::vector<uint32_t>& removableRequests) -> void {
  std::vector<uint32_t> removingRequests;
  for (auto& request : mRequestMap) {
    auto& number = request.first;
    MY_LOGD_IF(mLogLevel >= 3, "checking %" PRIu32 "", number);
    if (isRemovable(request.second)) {
      MY_LOGD_IF(mLogLevel >= 2, "Request %" PRIu32 " is ready to be removed",
                 number);
      removingRequests.push_back(number);
    } else if (number < targetNumber &&
               isRemovable(mRequestMap.at(targetNumber))) {
      MY_LOGD_IF(mLogLevel >= 2, "Request %" PRIu32 " is ready to be removed",
                 targetNumber);
      removingRequests.push_back(targetNumber);
      break;
    } else {
      break;
    }
  }
  // remove
  removeRequestsAndItems(removingRequests);
  // prepare callback
  removableRequests.insert(removableRequests.end(), removingRequests.begin(),
                           removingRequests.end());
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::removeRequestsAndItems(
    const std::vector<uint32_t>& requestNumbers) -> int {
  for (auto& number : requestNumbers) {
    if (mRequestMap.find(number) == mRequestMap.end()) {
      MY_LOGW("Request %" PRIu32 " was already removed", number);
      continue;
    }
    Request& request = mRequestMap.at(number);
    RequestType& type = request.requestType;
    // shutter & result
    mShutterQueueMap.at(type).erase(number);
    mResultQueueMap.at(type).erase(number);
    // physicalResults
    for (auto& element : request.pPhysicalResultMap) {
      mPhysicalResultQueueMapSet.at(element.first).at(type).erase(number);
    }
    // buffers
    for (auto& element : request.pBufferMap) {
      mBufferQueueMapSet.at(element.first).at(type).erase(number);
    }
    // remove request
    mRequestMap.erase(number);
    MY_LOGD_IF(mLogLevel >= 1, "Request %" PRIu32 " is removed", number);
  }
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::updateBufferStatus(const uint32_t& targetNumber,
                                             const Request& request,
                                             const Stream& stream) -> void {
  auto& id = stream.streamId;
  auto& targetStatus = stream.status;
  auto& currentStatus = request.pBufferMap.at(id)->status;
  //
  if (currentStatus == eITEM_INFLIGHT) {
    if (targetStatus == BufferStatus::ERROR) {
      currentStatus |= eITEM_ERROR;
    } else if (targetStatus == BufferStatus::OK) {
      currentStatus |= eITEM_VALID;
    } else {
      MY_LOGE("Unknown status of buffer %" PRId64 " request %" PRIu32 ", "
              "status=%d",
              id, targetNumber, targetStatus);
    }
  } else if (currentStatus & eITEM_NOTIFING) {
    if (targetStatus == BufferStatus::ERROR) {
      currentStatus |= eITEM_ERROR;
    } else if (targetStatus == BufferStatus::OK) {
      currentStatus |= eITEM_ERROR;
      MY_LOGE("Buffer %" PRId64 " of request %" PRIu32 " "
              "has been notified error but received OK",
              id, targetNumber);
    } else {
      MY_LOGE("Unknown status of buffer %" PRId64 " request %" PRIu32 ", "
              "status=%d",
              id, targetNumber, targetStatus);
    }
  } else {
    MY_LOGW_IF(mLogLevel >= 2,
               "Ignore buffer %" PRId64 " of request %" PRIu32 ", "
               "status is %d",
               id, targetNumber, currentStatus);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::isRemovable(Request request) -> bool {
  // shutter & result
  if (!(request.pShutter->status & eITEM_RELEASED) ||
      !(request.pResult->status & eITEM_RELEASED)) {
    MY_LOGD_IF(mLogLevel >= 3, "pShutter->status=%d pResult->status=%d",
               request.pShutter->status, request.pResult->status);
    return false;
  }
  // physicalResults follow with is last partial
  // for (auto& element : request.pPhysicalResultMap) {
  //     if (!(element.second->status & eITEM_RELEASED)) {
  //         MY_LOGD_IF(mLogLevel>=3,
  //                 "pPhysicalResultMap[%" PRId64 "]->status=%d",
  //                 element.first, element.second->status);
  //         return false;
  //     }
  // }
  // buffers
  for (auto& element : request.pBufferMap) {
    if (!(element.second->status & eITEM_RELEASED)) {
      MY_LOGD_IF(mLogLevel >= 3, "pBufferMap[%" PRId64 "]->status=%d",
                 element.first, element.second->status);
      return false;
    }
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackRecommender::dumpAllAvailable(
    const std::vector<uint32_t>& availableShutters,
    const std::vector<ErrorMsg>& availableErrors,
    const std::vector<CaptureResult>& availableBuffers,
    const std::vector<CaptureResult>& availableResults,
    const std::vector<uint32_t>& removableRequests) -> void {
  // availableShutters
  if (availableShutters.size() > 0) {
    std::string log = "shutter";
    for (auto& number : availableShutters) {
      log += " [" + std::to_string(number) + "]";
    }
    MY_LOGD("available %s", log.c_str());
  }
  // availableErrors
  if (availableErrors.size() > 0) {
    std::string log = "error";
    for (auto& message : availableErrors) {
      log += " [" + std::to_string(message.frameNumber) + "," +
             std::to_string(message.errorStreamId);
      switch (message.errorCode) {
        case (ErrorCode::ERROR_DEVICE):
          log += ",DEVICE]";
          break;
        case (ErrorCode::ERROR_REQUEST):
          log += ",REQUEST]";
          break;
        case (ErrorCode::ERROR_RESULT):
          log += ",RESULT]";
          break;
        case (ErrorCode::ERROR_BUFFER):
          log += ",BUFFER]";
          break;
        default:
          log += ",UNKNOWN]";
          break;
      }
    }
    MY_LOGD("available %s", log.c_str());
  }
  // availableBuffers
  if (availableBuffers.size() > 0) {
    std::string log = "buffer";
    for (auto& result : availableBuffers) {
      log += " [" + std::to_string(result.frameNumber);
      if (result.vInputStreams.size() > 0) {
        log += ",IN{";
        for (auto& stream : result.vInputStreams) {
          log += std::to_string(stream.streamId) + ":";
          log += stream.status == BufferStatus::OK ? "OK," : "ERROR,";
        }
        log += "}";
      }
      if (result.vOutputStreams.size() > 0) {
        log += ",OUT{";
        for (auto& stream : result.vOutputStreams) {
          log += std::to_string(stream.streamId) + ":";
          log += stream.status == BufferStatus::OK ? "OK," : "ERROR,";
        }
        log += "}";
      }
      log += "]";
    }
    MY_LOGD("available %s", log.c_str());
  }
  // availableResults
  if (availableResults.size() > 0) {
    std::string log = "result";
    for (auto& result : availableResults) {
      log += " [" + std::to_string(result.frameNumber);
      if (result.physicalCameraIdsOfPhysicalResults.size() > 0) {
        log += ",PHY{";
        for (auto& id : result.physicalCameraIdsOfPhysicalResults) {
          log += std::to_string(id) + ",";
        }
        log += "}";
      }
      log += "]";
    }
    MY_LOGD("available %s", log.c_str());
  }
  // removableRequests
  if (removableRequests.size() > 0) {
    std::string log = "remove";
    for (auto& number : removableRequests) {
      log += " [" + std::to_string(number) + "]";
    }
    MY_LOGD("available %s", log.c_str());
  }
}

/******************************************************************************
 *
 ******************************************************************************/
// auto CallbackRecommender::dumpStatus() -> void {
//   std::string log = "";
//   for (auto& element : mShutterQueueMap) {
//     auto& requestType = element.first;
//     auto& itemQueue = element.second;
//   }
//   return;
// }

};  // namespace Utils
};  // namespace core
};  // namespace mcam
