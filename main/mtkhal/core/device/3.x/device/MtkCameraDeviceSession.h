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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_MTKCAMERADEVICESESSION_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_MTKCAMERADEVICESESSION_H_
//
#include <mtkcam/utils/sys/Cam3CPUCtrl.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h>
//
#include <future>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
//
#include <IMtkcamVirtualDeviceSession.h>
#include <ICaptureSession.h>
#include "DisplayIdleDelayUtil.h"
#include "EventLog.h"

using mcam::core::device::policy::PipelineSessionType;
using mcam::core::device::policy::PolicyStaticInfo;
using NSCam::IMetadata;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
class MtkCameraDeviceSession : public IMtkcamVirtualDeviceSession,
                                 public IMtkcamDeviceSession {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  //  setup during constructor
  CreationInfo mStaticInfo;
  std::string const mLogPrefix;
  int32_t mLogLevel;
  EventLog mStateLog;

  mutable std::mutex mOpsLock;

  mutable std::mutex mCaptureSessionLock;
  android::sp<ICaptureSession> mCaptureSession = nullptr;

  std::vector<int32_t> mSensorId;

  std::weak_ptr<IMtkcamDeviceCallback> mCameraDeviceCallback;

  std::shared_ptr<PolicyStaticInfo> mPolicyStaticInfo;

  // CPU Control
  Cam3CPUCtrl* mpCpuCtrl = nullptr;
  MUINT32 mCpuPerfTime = 1000;  // 1 sec
  DisplayIdleDelayUtil mDisplayIdleDelayUtil;

 protected:  ////        Operations.
  auto getSafeCaptureSession() const -> android::sp<ICaptureSession>;
  auto waitUntilOpenDone() -> bool;
  auto initCameraInfo(const PipelineSessionType&,
                      const StreamConfiguration&,
                      CameraInfoMapT&) -> std::vector<int32_t>;
  auto initPolicyStaticInfo() -> void;
  auto decideCaptureSession(const StreamConfiguration& requestedConfiguration)
      -> PipelineSessionType;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~MtkCameraDeviceSession();
  explicit MtkCameraDeviceSession(CreationInfo const& info);

 public:  ////        Operations.
  auto const& getDeviceInfo() const { return *mStaticInfo.mStaticDeviceInfo; }
  auto const& getInstanceName() const { return getDeviceInfo().mInstanceName; }
  int32_t getInstanceId() const { return getDeviceInfo().mInstanceId; }
  auto getLogLevel() const { return mLogLevel; }
  auto const& getLogPrefix() const { return mLogPrefix; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  NSCam::IMtkcamVirtualDeviceSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto open(
      const std::shared_ptr<IMtkcamDeviceCallback> callback)
      -> int;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  NSCam::IMtkcamDeviceSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto constructDefaultRequestSettings(
      const RequestTemplate type, IMetadata& requestTemplate) -> int;

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
      bool& reconfigurationNeeded) const  // output
      -> int;

  virtual auto switchToOffline(const std::vector<int64_t>& streamsToKeep,
               mcam::CameraOfflineSessionInfo& offlineSessionInfo,
               std::shared_ptr<IMtkcamOfflineSession>& offlineSession)
      -> int;
#if 1
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ExtCameraDeviceSession Interfaces
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto getConfigurationResults(
      const uint32_t streamConfigCounter,
      mcam::ExtConfigurationResults& ConfigurationResults) -> int;
#endif
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_MTKCAMERADEVICESESSION_H_
