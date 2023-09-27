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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_APP_CALLBACKCORE_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_APP_CALLBACKCORE_H_
//
#include <mtkcam/utils/gralloc/IGrallocHelper.h>
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
//
#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
//
#include "../include/ICallbackCore.h"
#include "../include/ICallbackRecommender.h"

// using NSCam::IMetadataProvider;
// using NSCam::IMetadataConverter;
// using NSCam::IGrallocHelper;
using NSCam::IDebuggee;
using NSCam::IDebuggeeCookie;
using NSCam::IPrinter;
using NSCam::v3::StreamId_T;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {

/**
 * An implementation of App stream manager.
 */
class CallbackCore : public ICallbackCore {
  friend class ICallbackCore;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Common Info.
  using ICallbackRecommender = Utils::ICallbackRecommender;

  // struct CoreControl {
  //   enum SOF_RECEIVE_TYPE {
  //     FROM_SOF_INTERFACE = 0,
  //     FROM_HAL_META = 1,
  //   }
  //   bool enableMetaMerge;
  //   bool enableMetaPending;
  //   SOF_RECEIVE_TYPE sofType;
  //   bool enableAOSPMetaRule;
  // }

  struct CommonInfo {
    int32_t mInstanceId;
    int32_t mLogLevel;
    bool mSupportBufferManagement = false;
    size_t mAtMostMetaStreamCount = 0;
    //
    PFN_CALLBACK_T mpfnCallback;
    void* mpUser;
    std::string userName;
    std::shared_ptr<CoreSetting> coreSetting;
  };

  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;

 public:  ////    Debugging
  struct MyDebuggee : public IDebuggee {
    static const char* mName;
    std::shared_ptr<IDebuggeeCookie> mCookie = nullptr;
    ::android::wp<CallbackCore> mContext = nullptr;

    explicit MyDebuggee(CallbackCore* p) : mContext(p) {}
    virtual auto debuggeeName() const -> std::string { return mName; }
    virtual auto debug(::android::Printer& printer,
                       const std::vector<std::string>& options) -> void;
  };

 public:  ////
  //             +-----------------------------------------------------------+
  //             |                    Camera Frameworks                      |
  //             +-----------------------------------------------------------+
  //                                          ^                     ^
  //                                 +-----------------+   +-----------------+
  //                                 | CallbackHandler |   |  HalBufHandler  | <
  //                                 [IImageStreamBufferProvider]
  //                                 +-----------------+   +-----------------+
  //                                          ^                     v
  //                                 +-----------------+            |
  //             +---------------+   |   BatchHandler  |<-----------+
  //  [config] > | ConfigHandler | > |        ^        |            v
  //             +---------------+   |                 |   +----------------+
  //                                 |   FrameHandler  | < | RequestHandler | <
  //                                 [request]
  //                                 +-----------------+   +----------------+
  //                                          ^
  //                                 +----------------+
  //                                 |  ResultHandler |
  //                                 +----------------+
  //                                          ^
  //                                      [result]
  class ResultHandler;
  class FrameHandler;
  class BatchHandler;
  class CallbackHandler;
  class HalBufHandler;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////   Data Members.
  std::string const mInstanceName;
  std::shared_ptr<MyDebuggee> mDebuggee = nullptr;
  mutable ::android::Mutex mInterfaceLock;

 protected:  ////   Data Members (CONFIG/REQUEST/RESULT/FRAME/CALLBACK)
  ::android::sp<ResultHandler> mResultHandler = nullptr;
  ::android::sp<FrameHandler> mFrameHandler = nullptr;
  ::android::sp<BatchHandler> mBatchHandler = nullptr;
  // ::android::sp<CallbackHandler> mCallbackHandler = nullptr;
  // ::android::sp<HalBufHandler>  mHalBufHandler = nullptr;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Operations.
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }
  auto dumpStateLocked(IPrinter& printer,
                       const std::vector<std::string>& options) -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICallbackCore Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Interfaces.
  explicit CallbackCore(const CreationInfo& creationInfo);

  auto initialize() -> bool;

 public:  ////    ICallbackCore Interfaces.
  virtual auto destroy() -> void;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  virtual auto beginConfiguration(StreamConfiguration const& rStreams) -> void;

  virtual auto endConfiguration(HalStreamConfiguration const& rHalStreams)
      -> void;

  virtual auto flushRequest(const std::vector<CaptureRequest>& requests)
      -> void;

  virtual auto submitRequests(
      /*const std::vector<CaptureRequest>& requests,*/
      const std::vector<CaptureRequest>& rRequests) -> int;

  virtual auto updateResult(ResultQueue const& param) -> void;

  // virtual auto updateStreamBuffer(
  //     uint32_t frameNumber,
  //     StreamId_T streamId,
  //     std::shared_ptr<IImageStreamBuffer> const pBuffer) -> int;

  virtual auto waitUntilDrained(nsecs_t const timeout) -> int;

  virtual auto abortRequest(const std::vector<uint32_t> frameNumbers) -> void;

  // virtual auto getAppStreamInfoBuilderFactory() const
  //     -> std::shared_ptr<IStreamInfoBuilderFactory>;

  // virtual auto
  // getHalBufManagerStreamProvider(::android::sp<ICallbackCore>
  // pAppStreamManager) -> std::shared_ptr<IImageStreamBufferProvider>; virtual
  // auto    requestStreamBuffer( ::android::sp<IImageStreamBuffer>&
  // rpImageStreamBuffer, IImageStreamBufferProvider::RequestStreamBuffer const&
  // in) -> int;
  virtual auto markStreamError(uint32_t frameNumber, StreamId_T streamId)
      -> void;
  // virtual auto    waitHalBufCntAvailable(const std::vector<CaptureRequest>&
  // requests) -> void;
};

/**
 * Result Handler
 */
class CallbackCore::ResultHandler : public ::android::Thread {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  ::android::sp<FrameHandler> mFrameHandler = nullptr;

 protected:  ////
  mutable ::android::Mutex mResultQueueLock;
  ::android::Condition mResultQueueCond;
  ResultQueue mResultQueue;

 protected:  ////
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

  auto dequeResult(ResultQueue& rvResult) -> int;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Thread Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  // Ask this object's thread to exit. This function is asynchronous, when the
  // function returns the thread might still be running. Of course, this
  // function can be called from a different thread.
  virtual auto requestExit() -> void;

  // Good place to do one-time initializations
  virtual auto readyToRun() -> ::android::status_t;

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
 public:  ////    Operations.
  ResultHandler(std::shared_ptr<CommonInfo> pCommonInfo,
                ::android::sp<FrameHandler> pFrameHandler);

  virtual auto destroy() -> void;

  virtual auto enqueResult(ResultQueue const& rQueue) -> int;
};

/**
 * Frame Handler
 */
class CallbackCore::FrameHandler : public ::android::RefBase {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  using ICallbackRecommender = Utils::ICallbackRecommender;

  struct FrameParcel;
  struct CommonItem;
  struct ShutterItem;
  struct MessageItem;
  struct MetaItem;
  struct MetaItemSet;
  struct PhysicMetaItem;
  struct PhysicMetaItemSet;
  struct ImageItem;
  struct ImageItemSet;

  /**
   * IN_FLIGHT    -> PRE_RELEASE
   * IN_FLIGHT    -> VALID
   * IN_FLIGHT    -> ERROR
   * PRE_RELEASE  -> VALID
   * PRE_RELEASE  -> ERROR
   */
  struct State {
    enum T {
      IN_FLIGHT,
      PRE_RELEASE,
      VALID,
      ERROR,
    };
  };

  struct HistoryBit {
    enum {
      INQUIRED,
      AVAILABLE,
      RETURNED,
      ERROR_SENT_FRAME,
      ERROR_SENT_META,
      ERROR_SENT_IMAGE,
    };
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions: Meta Stream
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  struct CommonItem {
    FrameParcel* pFrame = nullptr;
    ::android::BitSet32 history;
  };

  struct ShutterItem : public CommonItem {
    std::shared_ptr<ShutterResult> mData;
  };

  // only handle request error & result error
  struct MessageItem : public CommonItem {
    std::shared_ptr<ErrorMsg> mData;
  };

  struct MetaItem : public CommonItem {
    IMetadata mData;
    MetaItemSet* pItemSet = nullptr;
    MUINT32 bufferNo = 0;  // partial_result
  };

  struct PhysicMetaItem : public CommonItem {
    std::shared_ptr<PhysicalCameraMetadata> mData;
    PhysicMetaItemSet* pItemSet = nullptr;
    MUINT32 bufferNo = 0;  // partial_result
  };

  struct ImageItem : public CommonItem {
    std::shared_ptr<StreamBuffer> mData;
    ImageItemSet* pItemSet = nullptr;
  };

  struct MetaItemSet : public std::map<MUINT32, std::shared_ptr<MetaItem>> {
    bool asInput;
    bool isHalMeta;
    bool hasLastPartial;
    bool hasRealTimePartial;
    std::shared_ptr<MetaItem> pLastItem = nullptr;
    //
    explicit MetaItemSet(bool _asInput, bool _isHalMeta)
        : asInput(_asInput),
          hasLastPartial(false),
          hasRealTimePartial(false),
          isHalMeta(_isHalMeta) {}
  };

  struct PhysicMetaItemSet
      : public std::map<MUINT32, std::shared_ptr<PhysicMetaItem>> {
    //
    PhysicMetaItemSet() {}
  };

  struct ImageItemSet
      : public std::map<StreamId_T, std::shared_ptr<ImageItem>> {
    bool asInput;
    //
    explicit ImageItemSet(MBOOL _asInput) : asInput(_asInput) {}
  };

  struct FrameParcel {
    uint32_t frameNumber;
    std::shared_ptr<ShutterItem> mShutterItem = nullptr;
    std::shared_ptr<MessageItem> mMessageItem = nullptr;
    ImageItemSet vOutputImageItem = ImageItemSet(false);
    ImageItemSet vInputImageItem = ImageItemSet(true);
    MetaItemSet vOutputMetaItem = MetaItemSet(false, false);
    MetaItemSet vOutputHalMetaItem = MetaItemSet(false, true);
    MetaItemSet vInputMetaItem = MetaItemSet(true, false);
    PhysicMetaItemSet vOutputPhysicMetaItem;
    // debug
    timespec requestTimestamp{0, 0};
    //
    int64_t getShutterTimestamp() {
      return mShutterItem == nullptr ? 0 : mShutterItem->mData->timestamp;
    }
    int64_t getStartOfFrameTimestamp() {
      return mShutterItem == nullptr ? 0 : mShutterItem->mData->startOfFrame;
    }
    const ErrorCode getErrorCode() {
      if (!mMessageItem)
        return ErrorCode::ERROR_NONE;
      else
        return mMessageItem->mData->errorCode;
    }
  };

  struct FrameMap : public std::map<uint32_t, std::shared_ptr<FrameParcel>> {};

  struct RecommenderQueue {
    /* for notify */
    std::vector<uint32_t> availableShutters;
    std::vector<ErrorMsg> availableErrors;
    /* for processCaptureResult */
    std::vector<ICallbackRecommender::CaptureResult> availableBuffers;
    std::vector<ICallbackRecommender::CaptureResult> availableResults;
    /* for completed requests */
    std::vector<uint32_t> removableRequests;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Data Members.
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  ::android::sp<BatchHandler> mBatchHandler = nullptr;

 protected:  ////    Data Members (CONFIG/REQUEST)
  mutable ::android::Mutex mFrameMapLock;
  ::android::Condition mFrameMapCond;
  FrameMap mFrameMap;

  mutable ::android::Mutex mImageConfigMapLock;
  std::map<StreamId_T, std::shared_ptr<StreamInfo>> mImageConfigMap;
  StreamConfigurationMode mOperationMode = StreamConfigurationMode::NORMAL_MODE;

  // [HAL3+] CallbackRecommander
  std::shared_ptr<ICallbackRecommender> mCbRcmder = nullptr;

  //
  bool mIsAllowPhyMeta = false;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Operations.
  auto dumpStateLocked(IPrinter& printer) const -> void;
  auto dumpLocked() const -> void;
  auto dump() const -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations: Request
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Operations.
  virtual auto registerStreamBuffers(
      std::vector<StreamBuffer> const& buffers,
      std::shared_ptr<FrameParcel>& pFrame,
      ICallbackRecommender::CaptureRequest& cr_CaptureRequest) -> int;

  virtual auto registerStreamBuffers(IMetadata const& metadata,
                                     std::shared_ptr<FrameParcel>& pFrame)
      -> int;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations: Result
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 protected:  ////    Operations.
  virtual auto prepareErrorFrame(CallbackParcel& rCbParcel,
                                 std::shared_ptr<FrameParcel> const& pFrame)
      -> void;

  virtual auto prepareErrorMetaIfPossible(
      CallbackParcel& rCbParcel,
      std::shared_ptr<MessageItem> const& pItem) -> void;

  virtual auto prepareErrorImage(CallbackParcel& rCbParcel,
                                 std::shared_ptr<ImageItem> const& pItem)
      -> void;

 protected:  ////    Operations.
  virtual auto prepareReturnMeta(CallbackParcel& rCbParcel,
                                 std::shared_ptr<MetaItem> const& pItem)
      -> void;

  virtual auto prepareReturnHalMeta(CallbackParcel& rCbParcel,
                                    std::shared_ptr<MetaItem> const& pItem)
      -> void;

  virtual auto preparePhysicReturnMeta(
      CallbackParcel& rCbParcel,
      std::shared_ptr<PhysicMetaItem> const& rFrameParcel) -> void;

  virtual auto isReturnable(
      std::shared_ptr<MetaItem> const& pItem) const
      -> bool;

  virtual auto isReturnable(
      std::shared_ptr<PhysicMetaItem> const& pItem,
      bool hasLogMetaCB) const
      -> bool;

 protected:  ////    Operations.
  virtual auto prepareReturnImage(CallbackParcel& rCbParcel,
                                  std::shared_ptr<ImageItem> const& pItem)
      -> void;

  virtual auto isReturnable(std::shared_ptr<ImageItem> const& pItem) const
      -> bool;

 protected:  ////    Operations.
  virtual auto prepareShutterNotificationIfPossible(
      std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
      uint32_t const& frameNumber) -> bool;

  virtual auto prepareErrorNotificationIfPossible(
      std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
      ErrorMsg const& error) -> bool;

  virtual auto prepareMetaCallbackIfPossible(
      std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
      std::shared_ptr<FrameParcel>& pFrame,
      std::vector<ICallbackRecommender::CaptureResult>& rAvailableResults)
      -> bool;

  virtual auto prepareImageCallbackIfPossible(
      std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
      ICallbackRecommender::CaptureResult const& rResult) -> bool;

 protected:  ////    Operations.
  virtual auto getSafeFrameParcel(uint32_t frameNumber,
                                  std::shared_ptr<FrameParcel>& rpItem) -> bool;

  virtual auto getSafeCbParecl(
      uint32_t frameNumber,
      std::map<uint32_t, std::shared_ptr<CallbackParcel>>& rCbParcelMap,
      bool needAcquireFromMap) -> std::shared_ptr<CallbackParcel>;

  virtual auto constructMetaItem(std::shared_ptr<FrameParcel> pFrame,
                                 MetaItemSet* pItemSet,
                                 IMetadata data) -> void;

  virtual auto updateShutterItem(const uint32_t frameNumber,
                                 const std::shared_ptr<ShutterResult>& pResult,
                                 std::vector<uint32_t>& rRequestShutterQueue)
      -> void;

  virtual auto updateMessageItem(const uint32_t frameNumber,
                                 const std::shared_ptr<ErrorMsg>& pResult,
                                 std::vector<ErrorMsg>& rRequestErrorQueue)
      -> void;

  virtual auto updateMetaItem(
      const bool isHalMeta,
      const uint32_t frameNumber,
      const VecMetaResult& vResult,
      std::vector<ICallbackRecommender::CaptureResult>& rRequestMetaQueue)
      -> void;

  virtual auto updatePhysicMetaItem(
      const uint32_t frameNumber,
      const VecPhysicMetaResult& vResult,
      std::vector<ICallbackRecommender::CaptureResult>& rRequestMetaQueue)
      -> void;

  virtual auto updateImageItem(
      const uint32_t frameNumber,
      const VecImageResult& vResult,
      const bool isOutputStream,
      std::vector<ICallbackRecommender::CaptureResult>& rRequestMetaQueue)
      -> void;

  virtual auto updateResult(ResultQueue const& rvResult,
                            RecommenderQueue& recommendQueue) -> void;

  virtual auto updateCallback(std::list<CallbackParcel>& rCbList,
                              RecommenderQueue& recommendQueue) -> void;

  virtual auto onHandleRecommenderCb(RecommenderQueue& requestQueue,
                                     RecommenderQueue& resultQueue) -> void;

 protected:  ////    Operations. worked in bypass mode only
  /**
   * @param[in] frame: a given frame to check.
   *
   * @return
   *      ==0: uncertain
   *      > 0: it is indeed a request error
   *      < 0: it is indeed NOT a request error
   */
  MINT checkRequestError(std::shared_ptr<FrameParcel> const& frame);

  virtual auto isFrameRemovable(
      std::shared_ptr<FrameParcel> const& pFrame) const -> bool;

  virtual auto forceRequestErrorAvailable(
      std::shared_ptr<FrameParcel>& pFrame,
      std::vector<ErrorMsg>& rvAvaibleErrors) -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  FrameHandler(std::shared_ptr<CommonInfo> pCommonInfo,
               ::android::sp<BatchHandler> pBatchHandler);

  virtual auto destroy() -> void;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  virtual auto setOperationMode(StreamConfigurationMode operationMode) -> void;

  virtual auto getConfigImageStream(StreamId_T streamId) const
      -> std::shared_ptr<StreamInfo>;

  virtual auto beginConfiguration(StreamConfiguration const& rStreams) -> void;

  virtual auto endConfiguration(HalStreamConfiguration const& rHalStreams)
      -> void;

  virtual auto registerFrame(CaptureRequest const& rRequest) -> int;

  virtual auto update(ResultQueue const& rvResult) -> void;

  virtual auto waitUntilDrained(nsecs_t const timeout) -> int;

  // virtual auto updateStreamBuffer(
  //     uint32_t frameNumber,
  //     StreamId_T streamId,
  //     std::shared_ptr<IImageStreamBuffer> const pBuffer) -> int;

  virtual auto dumpIfHasInflightRequest() const -> void;
};

/**
 * Batch Handler
 */
class CallbackCore::BatchHandler : public ::android::RefBase {
  friend class CallbackCore;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:
  struct InFlightBatch {
    // Protect access to entire struct. Acquire this lock before read/write any
    // data or calling any methods. processCaptureResult and notify will compete
    // for this lock HIDL IPCs might be issued while the lock is held
    mutable ::android::Mutex mLock;

    uint32_t mBatchNo;
    uint32_t mFirstFrame;
    uint32_t mLastFrame;
    uint32_t mBatchSize;
    std::vector<uint32_t> mHasLastPartial;
    std::vector<uint32_t> mShutterReturned;
    std::vector<uint32_t> mFrameRemoved;
    // bool                    mRemoved = false;
    uint32_t mFrameHasMetaResult = 0;
    uint32_t mFrameHasImageResult = 0;
    uint32_t mFrameRemovedCount = 0;
  };

 protected:  ////
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  // ::android::sp<CallbackHandler> mCallbackHandler = nullptr;

  // to record id of batched streams.
  std::vector<uint32_t> mBatchedStreams;
  uint32_t mBatchCounter;
  mutable ::android::Mutex mLock;
  std::vector<std::shared_ptr<InFlightBatch>> mInFlightBatches;
  mutable ::android::Mutex mMergedParcelLock;
  std::list<CallbackParcel> mMergedParcels;

 public:
  enum { NOT_BATCHED = -1 };

 protected:  ////
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  explicit BatchHandler(std::shared_ptr<CommonInfo> pCommonInfo);

  virtual auto destroy() -> void;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  virtual auto dumpStateLocked(IPrinter& printer) const -> void;

  virtual auto waitUntilDrained(nsecs_t const timeout) -> int;

  virtual auto push(std::list<CallbackParcel>& item) -> void;

  // Operations to be modified.
  virtual auto resetBatchStreamId() -> void;

  virtual auto checkStreamUsageforBatchMode(const std::shared_ptr<StreamInfo>)
      -> bool;

  virtual auto registerBatch(const std::vector<CaptureRequest>) -> int;

  virtual auto getBatchLocked(uint32_t frameNumber)
      -> std::shared_ptr<InFlightBatch>;

  virtual auto removeBatchLocked(uint32_t batchNum) -> void;

  virtual auto appendParcel(CallbackParcel srcParcel, CallbackParcel& dstParcel)
      -> void;

  virtual auto updateCallback(std::list<CallbackParcel> cbParcels,
                              std::list<CallbackParcel>& rUpdatedList) -> void;
};

/**
 * Callback Handler
 */
class CallbackCore::CallbackHandler : public ::android::Thread {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  struct CallbackQueue : public std::list<CallbackParcel> {};

  struct HidlCallbackParcel {
    std::vector<uint32_t> vframeNumber;
    std::vector<NotifyMsg> vNotifyMsg;
    std::vector<NotifyMsg> vErrorMsg;
    std::vector<CaptureResult> vCaptureResult;
    std::vector<CaptureResult> vBufferResult;
  };

  struct HidlCallbackQueue : public std::list<HidlCallbackParcel> {};

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Data Members.
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  // ::android::sp<HalBufHandler>              mHalBufHandler = nullptr;

  mutable ::android::Mutex mQueue1Lock;
  ::android::Condition mQueue1Cond;
  HidlCallbackQueue mQueue1;

  mutable ::android::Mutex mQueue2Lock;
  ::android::Condition mQueue2Cond;
  HidlCallbackQueue mQueue2;

 protected:  ////    Operations.
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

  auto waitUntilQueue1NotEmpty() -> bool;

  auto convertCallbackParcelToHidl(std::list<CallbackParcel>& item)
      -> HidlCallbackParcel;

  auto performCallback() -> void;

  // auto            convertShutterToHidl(CallbackParcel const& cbParcel,
  // std::vector<NotifyMsg>& rvMsg) -> void; auto
  // convertErrorToHidl(CallbackParcel const& cbParcel, std::vector<NotifyMsg>&
  // rvMsg) -> void; auto            convertMetaToHidl(CallbackParcel const&
  // cbParcel, std::vector<CaptureResult>& rvResult,
  // std::vector<camera_metadata*>& rvResultMetadata) -> void; auto
  // convertPhysicMetaToHidl(CallbackParcel const& cbParcel, CaptureResult&
  // rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void; auto
  // convertMetaToHidlWithMergeEnabled(CallbackParcel const& cbParcel,
  // std::vector<CaptureResult>& rvResult, std::vector<camera_metadata*>&
  // rvResultMetadata) -> void; auto convertImageToHidl(CallbackParcel const&
  // cbParcel, std::vector<CaptureResult>& rvResult) -> void;

  auto convertShutterResult(CallbackParcel const& cbParcel,
                            std::vector<NotifyMsg>& rvMsg) -> void;
  auto convertErrorResult(CallbackParcel const& cbParcel,
                          std::vector<NotifyMsg>& rvMsg) -> void;
  auto convertMetaResult(CallbackParcel const& cbParcel,
                         std::vector<CaptureResult>& rvResult) -> void;
  auto convertPhysicMetaResult(CallbackParcel const& cbParcel,
                               CaptureResult& rvResult) -> void;
  auto convertHalMetaResult(CallbackParcel const& cbParcel,
                            std::vector<CaptureResult>& rvResult) -> void;
  auto convertMetaResultWithMergeEnabled(CallbackParcel const& cbParcel,
                                         std::vector<CaptureResult>& rvResult)
      -> void;
  auto convertImageResult(CallbackParcel const& cbParcel,
                          std::vector<CaptureResult>& rvResult) -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Thread Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  // Ask this object's thread to exit. This function is asynchronous, when the
  // function returns the thread might still be running. Of course, this
  // function can be called from a different thread.
  virtual auto requestExit() -> void;

  // Good place to do one-time initializations
  virtual auto readyToRun() -> ::android::status_t;

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
 public:  ////    Operations.
  explicit CallbackHandler(std::shared_ptr<CommonInfo> pCommonInfo);

  virtual auto destroy() -> void;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  virtual auto waitUntilDrained(nsecs_t const timeout) -> int;

  virtual auto push(std::list<CallbackParcel>& item) -> void;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_APP_CALLBACKCORE_H_
