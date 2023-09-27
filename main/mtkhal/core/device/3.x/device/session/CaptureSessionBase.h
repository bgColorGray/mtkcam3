/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_SESSION_CAPTURESESSIONBASE_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_SESSION_CAPTURESESSIONBASE_H_
//
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <mtkcam3/pipeline/model/IPipelineModelManager.h>
#pragma GCC diagnostic pop
//
#include <mtkcam3/main/mtkhal/core/device/3.x/policy/IDeviceSessionPolicy.h>
//
#include <functional>
#include <future>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
//
#include <IAppConfigUtil.h>
#include <IAppRequestUtil.h>
#include <IAppStreamManager.h>
#include <ICaptureSession.h>
#include <IHalBufHandler.h>
#include "../../utils/include/AppImageStreamBufferProvider.h"
#include "../../utils/include/ZoomRatioConverter.h"
#include "../EventLog.h"

using mcam::core::Utils::AppImageStreamBufferProvider;
using mcam::core::Utils::IAppConfigUtil;
using mcam::core::Utils::IAppRequestUtil;
using mcam::core::Utils::IHalBufHandler;
//using UserConfigurationParams =
//  mcam::core::device::policy::AppUserConfiguration;
using PipelineUserConfiguration =
  mcam::core::device::policy::PipelineUserConfiguration;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {

// class PipelineModelCallback; // forward declaration
/******************************************************************************
 *
 ******************************************************************************/
class CaptureSessionBase
    : public ICaptureSession,
      public std::enable_shared_from_this<CaptureSessionBase>,
      public pipeline::model::IPipelineModelCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  static const nsecs_t _ONE_MS = 1000000;

  static bool isLowMemoryDevice() {
    return ::property_get_bool("ro.config.low_ram", false);
  }

  nsecs_t getFlushAndWaitTimeout() const {
#if (MTKCAM_TARGET_BUILD_VARIANT == 2)
    if (isLowMemoryDevice()) {
      return ::property_get_int32("vendor.cam3dev.flushandwaittimeout", 9000) *
             _ONE_MS;
    }
#endif

    return 3000000000;  // 3s
  }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  //  setup during constructor
  CreationInfo mStaticInfo;
  std::string const mLogPrefix;
  mutable std::mutex mOpsLock;

  //  setup during opening camera
  int32_t mLogLevel = 0;
  EventLog mStateLog;

  int32_t mUseLegacyAppStreamMgr = 0;
  mutable std::mutex mAppStreamManagerLock;
  ::android::sp<IAppStreamManager> mAppStreamManager = nullptr;
  std::shared_ptr<EventLogPrinter> mAppStreamManagerErrorState;
  std::shared_ptr<EventLogPrinter> mAppStreamManagerWarningState;
  std::shared_ptr<EventLogPrinter> mAppStreamManagerDebugState;
  // AppRequestUtil
  mutable std::mutex mAppRequestUtilLock;
  std::shared_ptr<IAppRequestUtil> mAppRequestUtil = nullptr;
  // HalBufHandler
  mutable std::mutex mHalBufHandlerLock;
  ::android::sp<IHalBufHandler> mHalBufHandler;

  mutable std::mutex mDeviceSessionPolicyLock;
  std::shared_ptr<device::policy::IDeviceSessionPolicy> mDeviceSessionPolicy;

  int32_t mUseAppImageSBProvider = 0;
  std::shared_ptr<AppImageStreamBufferProvider> mAppImageSBProvider = nullptr;

  mutable std::mutex mPipelineModelLock;
  ::android::sp<NSCam::v3::pipeline::model::IPipelineModel>
                                            mPipelineModel = nullptr;

  //  setup during configuring streams
  mutable std::mutex mStreamConfigCounterLock;
  MUINT32 mStreamConfigCounter = 1;

  //  setup during submitting requests.
  mutable std::mutex mRequestingLock;
  std::atomic_int mRequestingAllowed = 0;
  bool m1stRequestNotSent = true;

  // scenario timestamp
  uint64_t mConfigTimestamp;

  // Config Maps
  mutable std::mutex mImageConfigMapLock;
  ImageConfigMap mImageConfigMap;
  MetaConfigMap mMetaConfigMap;

  // scenario timestamp
  std::shared_ptr<ZoomRatioConverter> mpZoomRatioConverter = nullptr;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~CaptureSessionBase();
  explicit CaptureSessionBase(std::string const& name,
                              CreationInfo const& info);

 public:  ////        Operations.
  auto const& getDeviceInfo() const { return *mStaticInfo.mStaticDeviceInfo; }
  auto const& getInstanceName() const { return getDeviceInfo().mInstanceName; }
  int32_t getInstanceId() const { return getDeviceInfo().mInstanceId; }
  auto getLogLevel() const { return mLogLevel; }
  auto const& getLogPrefix() const { return mLogPrefix; }

 protected:  ////        Operations.
  auto getSafeAppStreamManager() const -> ::android::sp<IAppStreamManager>;
  auto getSafeDeviceSessionPolicy() const
      -> std::shared_ptr<device::policy::IDeviceSessionPolicy>;

  auto getSafeHalBufHandler() const -> ::android::sp<IHalBufHandler>;
  auto getSafePipelineModel() const -> ::android::sp<NSCam::v3::pipeline::model::IPipelineModel>;
  auto flushAndWait() -> int;
  auto initialize() -> bool;
//  auto markStreamError(std::shared_ptr<RequestSet> requestSet,
//                       std::set<StreamId_T>& vskipStreamList,
//                       std::vector<uint32_t>& vskipSensorList) -> void;
//  auto updateRawStreamInfo(
//      android::Vector<Request>& appRequests,
//      std::shared_ptr<RequestSet> requestSet,
//      SensorMap<device::policy::AdditionalRawInfo> const& vAdditionalRawInfo)
//      -> void;

 protected:  ////        Operations.
  auto enableRequesting() -> void;
  auto disableRequesting() -> void;

 protected:  ////        [Template method] Operations.
  virtual auto onEndConfiguration() -> void{};
  virtual auto onUpdateResult(UpdateResultParams& params) -> void;

 protected:
  virtual auto onProcessCaptureRequest(
      const std::vector<CaptureRequest>& wrappedRequests,
      uint32_t& numRequestProcessed) -> int;

  virtual auto waitUntilOpenDoneLocked() -> bool;

  virtual auto onCloseLocked() -> void;

  virtual auto onConfigureStreamsLocked(
      const StreamConfiguration& requestedConfiguration,
      HalStreamConfiguration& halConfiguration)  // output
      -> int;

  virtual auto onFlushLocked() -> int;

  virtual auto prepareUserConfigurationParamsLocked(
      const uint64_t& configureTimeStamp,
      const StreamConfiguration& requestedConfiguration,
      HalStreamConfiguration& halConfiguration,
      std::shared_ptr<PipelineUserConfiguration>& rCfgParams)
      -> int;

  virtual auto getConfigImageStream(StreamId_T streamId) const
      -> android::sp<AppImageStreamInfo>;

  virtual auto checkRequestParams(const std::vector<CaptureRequest>& vRequests)
      -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NSCam::ICaptureSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CaptureSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto configureStreams(
      const StreamConfiguration& requestedConfiguration,
      HalStreamConfiguration& halConfiguration)  // output
      -> int;

  virtual auto processCaptureRequest(
      const std::vector<CaptureRequest>& requests,
      uint32_t& numRequestProcessed)  // output
      -> int;

  virtual auto flush() -> int;
  virtual auto close() -> int;

  virtual auto signalStreamFlush(const std::vector<int32_t>& streamIds,
                                 uint32_t streamConfigCounter) -> void;

  virtual auto isReconfigurationRequired(
      const IMetadata& oldSessionParams,
      const IMetadata& newSessionParams,
      bool& reconfigurationNeeded)  // output
      -> int;

  virtual auto switchToOffline(const std::vector<int64_t>& streamsToKeep,
               mcam::CameraOfflineSessionInfo& offlineSessionInfo)
      -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineModelCallback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Operations.
    virtual auto    onFrameUpdated(
                  NSCam::v3::pipeline::model::UserOnFrameUpdated const& params
              ) -> void;

    virtual auto    onMetaResultAvailable(
                  NSCam::v3::pipeline::model::UserOnMetaResultAvailable&& arg
              ) -> void;

    virtual auto    onImageBufferReleased(
                  NSCam::v3::pipeline::model::UserOnImageBufferReleased&& arg
              ) -> void;

    virtual auto    onRequestCompleted(
                  NSCam::v3::pipeline::model::UserOnRequestCompleted&& arg
              ) -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ExtCameraDeviceSession Interfaces
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto getConfigurationResults(
      const uint32_t streamConfigCounter,
      mcam::ExtConfigurationResults& ConfigurationResults) -> int;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_SESSION_CAPTURESESSIONBASE_H_
