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

#ifndef MAIN_MTKHAL_ANDROID_DEVICE_ACBADATPOR_ACALLBACKADAPTOR_H_
#define MAIN_MTKHAL_ANDROID_DEVICE_ACBADATPOR_ACALLBACKADAPTOR_H_
//
#include <main/mtkhal/core/device/3.x/include/ICallbackCore.h>
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
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
//
#include "IACallbackAdaptor.h"
// #include "../include/IACallbackAdaptor.h"

using NSCam::IGrallocHelper;
using NSCam::IMetadataConverter;
using NSCam::IMetadataProvider;

using mcam::core::ICallbackCore;
using IPrinter = NSCam::IPrinter;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace android {

class ACallbackAdaptor : public IACallbackAdaptor {
  friend class IACallbackAdaptor;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  using CallbackParcel = ICallbackCore::CallbackParcel;

 public:
  struct CommonInfo {
    int32_t mLogLevel = 0;
    int32_t mInstanceId = -1;
    std::weak_ptr<IACameraDeviceCallback> mDeviceCallback;
    ::android::sp<IMetadataProvider> mMetadataProvider = nullptr;
    std::map<uint32_t, ::android::sp<IMetadataProvider>>
        mPhysicalMetadataProviders;
    IGrallocHelper* mGrallocHelper = nullptr;
    bool mSupportBufferManagement = false;
    size_t mAtMostMetaStreamCount = 0;
    std::shared_ptr<core::CoreSetting> coreSetting = nullptr;
  };

  struct CallbackQueue : public std::list<CallbackParcel> {};

  struct ACallbackParcel {
    std::set<uint32_t> vFrameNumber;
    std::vector<NotifyMsg> vNotifyMsg;
    std::vector<NotifyMsg> vErrorMsg;
    std::vector<CaptureResult> vCaptureResult;
    std::vector<CaptureResult> vBufferResult;
  };

  struct ACallbackQueue : public std::list<ACallbackParcel> {};
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  class CoreConverter;
  class CallbackHandler;

 protected:  ////  Data Members.
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  mutable ::android::Mutex mInterfaceLock;

  std::shared_ptr<CoreConverter> mCoreConverter = nullptr;
  ::android::sp<CallbackHandler> mCallbackHandler = nullptr;
  std::shared_ptr<ICallbackCore> mCallbackCore = nullptr;

  // surface dejitter
  bool bIsDejitterEnabled = false;
  mutable ::android::Mutex mSOFMapLock;
  std::map<uint32_t, int64_t> mSOFMap;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IACallbackAdaptor Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    IACallbackAdaptor Interfaces.
  virtual auto destroy() -> void;

  // virtual auto dumpState(IPrinter& printer,
  //                        const std::vector<std::string>& options) -> void;

  // copy config image/meta map into AppStreamMgr.FrameHandler
  virtual auto beginConfiguration(
      const mcam::StreamConfiguration& rStreams) -> void;

  virtual auto endConfiguration(
      const mcam::HalStreamConfiguration& rHalStreams) -> void;

  virtual auto submitRequests(
      const std::vector<mcam::CaptureRequest>& rRequests) -> int;

  virtual auto waitUntilDrained(int64_t const timeout) -> int;

  virtual auto flushRequest(
      const std::vector<mcam::CaptureRequest>& requests) -> void;

  virtual auto processCaptureResult(
      const std::vector<mcam::CaptureResult>& mtkResults) -> void;

  virtual auto notify(const std::vector<mcam::NotifyMsg>& mtkMsgs)
      -> void;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Interfaces.
  explicit ACallbackAdaptor(const CreationInfo& creationInfo);

  auto initialize() -> bool;

 protected:  ////    Operations.
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }
  auto dumpStateLocked(IPrinter& printer,
                       const std::vector<std::string>& options) -> void;

  static void onReceiveCbParcels(
      std::list<ICallbackCore::CallbackParcel>& cbList,
      void* pUser);

  virtual auto storeSFDejitterSOF(
      const std::vector<mcam::CaptureResult>& mtkResults) -> void;

  virtual auto applyGrallocBehavior(ICallbackCore::CallbackParcel& parcel)
      -> bool;
};

/**
 * Callback Handler
 */
class ACallbackAdaptor::CoreConverter {
  friend class ACallbackAdaptor;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  explicit CoreConverter(std::shared_ptr<CommonInfo> CommonInfo);

  virtual ~CoreConverter() {}

  ////  callback stage : UpdateResultParams -> ICallbackCore::UpdateResultParams
  virtual auto convertResult(
      std::vector<mcam::CaptureResult> const& params,
      ICallbackCore::ResultQueue& resultQueue) -> void;
  virtual auto convertResult(std::vector<mcam::NotifyMsg> const& params,
                             ICallbackCore::ResultQueue& resultQueue) -> void;

  //  callback stage :
  //  ICallbackCore::CallbackParcel -> ::Android::CaptureResult
  virtual auto convertCallbackParcel(std::list<CallbackParcel>& cbParcels)
      -> ACallbackParcel;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Data Members.
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;

  bool bEnableMetaMerge;

 protected:  ////    Operations.
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

  auto traceDisplayIf(uint32_t requestNo,
                      uint64_t timestampShutter,
                      const CallbackParcel::ImageItem& item) -> void;

 protected:  ////    mcam::android -> ICallbackCore
  virtual auto convertAndFillShutter(
      mcam::ShutterMsg const& params,
      std::map<uint32_t, std::shared_ptr<ICallbackCore::ShutterResult>>& rMap,
      ::android::String8& log) -> void;

  virtual auto convertAndFillErrorResult(
      mcam::ErrorMsg const& params,
      std::map<uint32_t, std::shared_ptr<mcam::ErrorMsg>>& rMap,
      ::android::String8& log) -> void;

  virtual auto appendSensorTimestampMeta(
      mcam::ShutterMsg const& params,
      std::map<uint32_t, ICallbackCore::VecMetaResult>& rMap,
      ::android::String8& log) -> void;

 protected:  ////    ICallbackCore::CallbackParcel -> ::mtkcamhal_core
  virtual auto convertShutterResult(CallbackParcel const& cbParcel,
                                    std::vector<NotifyMsg>& rvMsg) -> void;

  virtual auto convertErrorResult(CallbackParcel const& cbParcel,
                                  std::vector<NotifyMsg>& rvMsg) -> void;

  virtual auto convertMetaResult(CallbackParcel const& cbParcel,
                                 std::vector<CaptureResult>& rvResult) -> void;

  virtual auto convertPhysicMetaResult(CallbackParcel const& cbParcel,
                                       CaptureResult& rvResult) -> void;

  virtual auto convertMetaResultWithMergeEnabled(
      CallbackParcel const& cbParcel,
      std::vector<CaptureResult>& rvResult) -> void;

  virtual auto convertImageResult(CallbackParcel const& cbParcel,
                                  std::vector<CaptureResult>& rvResult) -> void;

  virtual auto handleAOSPFenceRule(
      std::shared_ptr<mcam::StreamBuffer> pBuffer) -> void;
};

/**
 * Callback Handler
 */
class ACallbackAdaptor::CallbackHandler : public ::android::Thread {
  friend class ACallbackAdaptor;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////    Data Members.
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;

  mutable ::android::Mutex mQueue1Lock;
  ::android::Condition mQueue1Cond;
  ACallbackQueue mQueue1;

  mutable ::android::Mutex mQueue2Lock;
  ::android::Condition mQueue2Cond;
  ACallbackQueue mQueue2;

 protected:  ////    Operations.
  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

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

  virtual auto waitUntilDrained(int64_t const timeout) -> int;

  virtual auto push(const ACallbackParcel& item) -> void;
};
/******************************************************************************
 *
 ******************************************************************************/
};      // namespace android
};      // namespace mcam
#endif  // MAIN_MTKHAL_ANDROID_DEVICE_ACBADATPOR_ACALLBACKADAPTOR_H_
