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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_CALLBACKRECOMMENDER_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_CALLBACKRECOMMENDER_H_

#include <ICallbackRecommender.h>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace mcam {
namespace core {
namespace Utils {

class CallbackRecommender : public ICallbackRecommender {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  enum ItemStatus {
    eITEM_INFLIGHT = 0,
    eITEM_VALID = (1 << 0),
    eITEM_ERROR = (1 << 1),  // received error buffer
    eITEM_NOTIFING = (1 << 2),  // received error message
    eITEM_NOTIFIED = (1 << 3),  // returned error message
    eITEM_RELEASED = (1 << 4),
  };

  // enum StreamType {
  //   eSTREAM_OUTPUT = 0,
  //   eSTREAM_INPUT = 1,
  // };

  struct Item {
    uint32_t frameNumber = -1;
    int status = eITEM_INFLIGHT;  // ItemStatus
    explicit Item(uint32_t number) : frameNumber(number) {}
    ~Item();
  };

  // ItemQueue: frameNumber -> Item
  struct ItemQueue : public std::map<uint32_t, std::shared_ptr<Item>> {};

  struct ItemQueueMap : public std::map<RequestType, ItemQueue> {
    StreamType streamType;
    explicit ItemQueueMap(StreamType type) : streamType(type) {
      this->insert(std::pair<RequestType, ItemQueue>(
          eREQUEST_NORMAL, ItemQueue()));
      this->insert(std::pair<RequestType, ItemQueue>(
          eREQUEST_ZSL_STILL_CAPTURE, ItemQueue()));
      this->insert(std::pair<RequestType, ItemQueue>(
          eREQUEST_REPROCESSING, ItemQueue()));
    }

    void reset() {
      this->at(eREQUEST_NORMAL).clear();
      this->at(eREQUEST_ZSL_STILL_CAPTURE).clear();
      this->at(eREQUEST_REPROCESSING).clear();
    }
  };

  struct Request {
    RequestType requestType = eREQUEST_NORMAL;
    std::shared_ptr<Item> pShutter;
    std::shared_ptr<Item> pResult;
    std::map<int64_t, std::shared_ptr<Item>> pPhysicalResultMap;
    std::map<int64_t, std::shared_ptr<Item>> pBufferMap;
  };

  // RequestMap: frameNumber -> Request
  // 1. User will only bring request number after registerRequest,
  //    so we need to record requestType to save the search time.
  // 2. The element will be remove after request is completed.
  struct RequestMap : public std::map<uint32_t, Request> {};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Member.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  // creation
  uint32_t mPartialResultCount = 0;
  uint32_t mLogLevel;
  std::mutex mFunctionMutex;
  uint32_t mNextRequestNumber = 0;

  // Shutter
  ItemQueueMap mShutterQueueMap;
  // Result
  ItemQueueMap mResultQueueMap;
  // PhysicalResult: PhysicalCameraId -> ItemQueue
  std::map<int64_t, ItemQueueMap> mPhysicalResultQueueMapSet;
  // Buffer: StreamId -> ItemQueue
  std::map<int64_t, ItemQueueMap> mBufferQueueMapSet;
  // RequestMap: frameNumber -> Request -> iterator
  RequestMap mRequestMap;

 public:  // configuration
  explicit CallbackRecommender(const CreationInfo& creationInfo);

  virtual ~CallbackRecommender();

 public:  // configuration
  virtual auto configureStreams(
      const StreamConfiguration& streamInfos) -> int;

 public:  // request
  virtual auto registerRequests(
      const std::vector<CaptureRequest>& requests) -> int;

  virtual auto removeRequests(
      const std::vector<uint32_t>& requestNumbers,
      availableCallback_cb _available_cb_) -> int;

 public:  // notify
  virtual auto notifyErrors(
      const std::vector<ErrorMsg>& errorCodes,
      availableCallback_cb _available_cb_) -> void;

  virtual auto notifyShutters(
      const std::vector<uint32_t>& requestNumbers,
      availableCallback_cb _available_cb_) -> void;

 public:  // processCaptureResult
  virtual auto receiveMetadataResults(
      const std::vector<CaptureResult>& results,
      availableCallback_cb _available_cb_) -> void;

  virtual auto receiveBuffers(
      const std::vector<CaptureResult>& buffers,
      availableCallback_cb _available_cb_) -> void;

 protected:  // prepareCallback
  virtual auto prepareCallbackShutters(
      const uint32_t& targetNumber,
      std::vector<uint32_t>& availableShutters) -> void;

  virtual auto prepareCallbackErrors(
      const uint32_t& targetNumber,
      std::vector<ErrorMsg>& availableErrors) -> void;

  virtual auto prepareCallbackBuffers(
      const uint32_t& targetNumber,
      std::vector<CaptureResult>& availableBuffers) -> void;

  virtual auto prepareCallbackResults(
      const uint32_t& targetNumber,
      std::vector<CaptureResult>& availableResults) -> void;

  virtual auto prepareRemoveRequests(
      const uint32_t& targetNumber,
      std::vector<uint32_t>& removableRequests) -> void;

 protected:
  virtual auto removeRequestsAndItems(
      const std::vector<uint32_t>& requestNumbers) -> int;

  virtual auto updateBufferStatus(const uint32_t& targetNumber,
                                  const Request& request,
                                  const Stream& stream) -> void;

  virtual auto isRemovable(Request request) -> bool;

  virtual auto dumpAllAvailable(
      const std::vector<uint32_t>& availableShutters,
      const std::vector<ErrorMsg>& availableErrors,
      const std::vector<CaptureResult>& availableBuffers,
      const std::vector<CaptureResult>& availableResults,
      const std::vector<uint32_t>& removableRequests) -> void;

  // virtual auto    dumpStatus() -> void;
};

};  // namespace Utils
};  // namespace core
};  // namespace mcam

#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_CALLBACKRECOMMENDER_H_
