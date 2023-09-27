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

#ifndef MAIN_MTKHAL_DEVICE_3_X_CUSTCALLBACK_H_
#define MAIN_MTKHAL_DEVICE_3_X_CUSTCALLBACK_H_

#include "ICustCallback.h"
#include "ICallbackRecommender.h"
#include "MyUtils.h"

#include <mtkcam/utils/debug/debug.h>
//
#include <time.h>
//
#include <utils/BitSet.h>
#include <utils/Condition.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#include <utils/Thread.h>
#include <utils/String8.h>
//
#include <atomic>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <memory>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {

android::String8 dumpCaptureResult(CaptureResult res);
android::String8 dumpNotifyMsg(NotifyMsg msg);

class CustCallback : public ICustCallback {
  public:
    class ResultHandler;
    class FrameHandler;
    class CallbackHandler;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definition
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  struct FrameParcel {
    std::vector<CaptureResult> vResult;
    std::vector<NotifyMsg> vMsg;
    int partialCount = 0; // handle partial count align AOSP rule
  };
    using ResultMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
  struct CommonInfo {
    int32_t mLogLevel = 0;
    int32_t mInstanceId = -1;
    android::sp<ICameraDeviceCallback> mDeviceCallback = nullptr;
    std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;
    uint32_t mResultMetadataQueueSize = 0;
    size_t mAtMostMetaStreamCount = 0;
  };
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data member
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  protected:
    std::string const mInstanceName;
    mutable android::Mutex mInterfaceLock;
    std::shared_ptr<CommonInfo> mCommonInfo;

    android::sp<ResultHandler> mResultHandler = nullptr;
    android::sp<FrameHandler> mFrameHandler = nullptr;
    android::sp<CallbackHandler> mCallbackHandler = nullptr;

    std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICustStreamController Interface
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  public:
    virtual auto destroy() -> void;

    virtual auto configureStreams(
        const ::android::hardware::camera::device::V3_4::StreamConfiguration&
            requestedConfiguration
    ) -> int;

    virtual auto submitRequests(
        const hidl_vec<CaptureRequest>& requests) -> int;

    virtual auto processCaptureResult(
        const hidl_vec<CaptureResult>& param) ->  void;

    virtual auto notify(
        const hidl_vec<NotifyMsg>& param) -> void;

    virtual auto flushRequest(
        const hidl_vec<CaptureRequest>& requests) -> void;

    virtual auto waitUntilDrained(nsecs_t const timeout) -> int;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  CustCallback Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  public:
    explicit CustCallback(
        const CreationInfo& creationInfo);

    auto initialize() -> bool;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  protected:  ////    Operations.
    auto getLogLevel() const -> int32_t {
      return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
    }
    auto dumpStateLocked(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;
};


/**
 * Result Handler
 */
class CustCallback::ResultHandler : public android::Thread {
  friend class CustCallback;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  member data
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  private:
    std::string const mInstanceName;
    std::shared_ptr<CommonInfo> mCommonInfo;

    android::sp<FrameHandler> mFrameHandler = nullptr;

    mutable android::Mutex mResultQueueLock;
    android::Condition mResultQueueCond;
    std::vector<CaptureResult> mResultQueue;
    std::vector<NotifyMsg> mMsgQueue;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interface
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  public:
    ResultHandler(std::shared_ptr<CommonInfo> pCommonInfo,
                  android::sp<FrameHandler> pFrameHandler);

    virtual auto enqueResult(
        const hidl_vec<CaptureResult>& rResult,
        const hidl_vec<NotifyMsg>& rMsg) -> int;

    virtual auto destroy() -> void;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  operation
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  protected:
    auto getLogLevel() const -> int32_t {
      return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
    }

    auto dequeResult(std::vector<CaptureResult>& rvResult,
                     std::vector<NotifyMsg>& rvMsg) -> int;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Thread Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  public:  ////
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual auto requestExit() -> void;

    // Good place to do one-time initializations
    virtual auto readyToRun() -> android::status_t;

  private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual auto threadLoop() -> bool;
};

/**
 * Frame Handler
 */
class CustCallback::FrameHandler
    : public android::RefBase {
  friend class CustCallback;
  using ICallbackRecommender = mcam::core::Utils::ICallbackRecommender;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definition
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  public:
    struct RecommenderQueue {
      /* for notify */
      std::vector<uint32_t> availableShutters;
      std::vector<ICallbackRecommender::ErrorMessage> availableErrors;
      /* for processCaptureResult */
      std::vector<ICallbackRecommender::CaptureResult> availableBuffers;
      std::vector<ICallbackRecommender::CaptureResult> availableResults;
      /* for completed requests */
      std::vector<uint32_t> removableRequests;
    };
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Member.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  protected:  ////    Data Members.
    std::string const mInstanceName;
    std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
    android::sp<CallbackHandler> mCallbackHandler = nullptr;

    mutable android::Mutex mPendingParcelLock;
    android::Condition mPendingParcelCond;
    std::map<uint32_t, std::shared_ptr<FrameParcel>> mPendingParcels;

    mutable android::Mutex mAppStreamLock;
    std::vector<Stream> mAppStreams;

    std::shared_ptr<ICallbackRecommender> mCbRcmder = nullptr;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interface
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  public:
    FrameHandler(
        std::shared_ptr<CommonInfo> pCommonInfo,
        android::sp<CallbackHandler> pCallbackHandler);
    /**
     * @brief register configured streams
     */
    virtual auto configureStreams(
        const hidl_vec<Stream>& rStreams) -> void;
    /**
     * @brief register inflight request.
     */
    virtual auto registerFrame(
        const CaptureRequest& rRequest) -> int;

    virtual auto update(
        std::vector<CaptureResult>& rvResult,
        std::vector<NotifyMsg>& rvMsg) -> void;

    virtual auto destroy() -> void;

    virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

    virtual auto waitUntilDrained(nsecs_t const timeout) -> int;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  operation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  protected:
    virtual auto onHandleRecommenderFeedback(
        RecommenderQueue& requestQueue,
        RecommenderQueue& resultQueue) -> void;

    auto getSafeFrameParcel(
        uint32_t requestNo,
        std::shared_ptr<FrameParcel>& rpItem) -> bool;

    virtual auto queryRequestType(
        const CaptureRequest& rRequest) -> ICallbackRecommender::RequestType;

    virtual auto queryPartialCount(
        CaptureResult& rResult,
        std::shared_ptr<FrameParcel>& rpCbParcel) -> uint32_t;

    virtual auto updateResult(
        std::vector<CaptureResult>& rvResult,
        std::vector<NotifyMsg>& rvMsg,
        RecommenderQueue& resultQueue,
        FrameParcel& cbList) -> void;

    virtual auto updateMetaCb(
        CaptureResult& rResult,
        std::shared_ptr<FrameParcel>& rpCbParcel,
        RecommenderQueue& requestQueue,
        FrameParcel& cbList) -> void;

    virtual auto updatePhysicMetaCb(
        CaptureResult& rResult,
        std::shared_ptr<FrameParcel>& rpCbParcel,
        RecommenderQueue& requestQueue) -> void;

    virtual auto updateImageCb(
        CaptureResult& rResult,
        std::shared_ptr<FrameParcel>& rpCbParcel,
        RecommenderQueue& requestQueue) -> void;

    virtual auto updateShutterCb(
        NotifyMsg& rMsg,
        std::shared_ptr<FrameParcel>& rpCbParcel,
        RecommenderQueue& requestQueue) -> void;

    virtual auto updateErrorCb(
        NotifyMsg& rMsg,
        std::shared_ptr<FrameParcel>& rpCbParcel,
        RecommenderQueue& requestQueue) -> void;

    virtual auto updateCallback(
        RecommenderQueue& recommendQueue,
        FrameParcel& rCbList) -> void;

    virtual auto prepareShutterNotificationIfPossible(
        FrameParcel& rCbList,
        const uint32_t& requestNo) -> bool;

    virtual auto prepareErrorNotificationIfPossible(
        FrameParcel& rCbList,
        const ICallbackRecommender::ErrorMessage& error) -> bool;

    virtual auto prepareMetaCallbackIfPossible(
        FrameParcel& rCbList,
        const ICallbackRecommender::CaptureResult&  rAvailableResult) -> bool;

    virtual auto prepareImageCallbackIfPossible(
        FrameParcel& rCbList,
        const ICallbackRecommender::CaptureResult& rAvailableResult) -> bool;

    virtual auto dumpStateLocked(IPrinter& printer) const -> void;

    virtual auto dumpLocked() const -> void;

    virtual auto dump() const -> void;

    virtual auto dumpIfHasInflightRequest() const -> void;
};

/**
 * Callback Handler
 */
class CustCallback::CallbackHandler : public android::Thread {
  friend class CustCallback;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Data Members.
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue = nullptr;

  mutable android::Mutex mQueue1Lock;
  android::Condition mQueue1Cond;
  std::vector<FrameParcel> mQueue1;

  mutable android::Mutex mQueue2Lock;
  android::Condition mQueue2Cond;
  std::vector<FrameParcel> mQueue2;

 protected:  ////    Operations.
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  operation
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  auto waitUntilQueue1NotEmpty() -> bool;

  auto performCallback() -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Thread Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  // Ask this object's thread to exit. This function is asynchronous, when the
  // function returns the thread might still be running. Of course, this
  // function can be called from a different thread.
  virtual auto requestExit() -> void;

  // Good place to do one-time initializations
  virtual auto readyToRun() -> android::status_t;

 private:
  // Derived class must implement threadLoop(). The thread starts its life
  // here. There are two ways of using the Thread object:
  // 1) loop: if threadLoop() returns true, it will be called again if
  //          requestExit() wasn't called.
  // 2) once: if threadLoop() returns false, the thread will exit upon return.
  virtual auto threadLoop() -> bool;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:                                                  ////    Operations.
  explicit CallbackHandler(std::shared_ptr<CommonInfo> pCommonInfo);

  virtual auto destroy() -> void;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  virtual auto waitUntilDrained(nsecs_t const timeout) -> int;

  virtual auto push(FrameParcel& item) -> void;
};

};      // namespace v3
};      // namespace NSCam
#endif  // MAIN_MTKHAL_DEVICE_3_X_CUSTCALLBACK_H_