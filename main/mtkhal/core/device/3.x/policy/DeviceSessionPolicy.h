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
/* MediaTek Inc. (C) 2020. All rights reserved.
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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_DEVICESESSIONPOLICY_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_DEVICESESSIONPOLICY_H_

#include <mtkcam3/main/mtkhal/core/device/3.x/policy/IDeviceSessionPolicy.h>

#include <IAppConfigUtil.h>
#include <IAppRequestUtil.h>

#include <memory>
#include <string>

using mcam::core::Utils::IAppConfigUtil;
using mcam::core::Utils::IAppRequestUtil;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace device {
namespace policy {

class DeviceSessionPolicy : public IDeviceSessionPolicy {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  //// static (unchagable)
  std::shared_ptr<DeviceSessionStaticInfo const> mStaticInfo;
  std::string const mInstanceName;

  //// configure (change only in configuration)
  std::shared_ptr<DeviceSessionConfigInfo const> mConfigInfo;

  mutable std::mutex mConfigPolicyLock;
  std::shared_ptr<IAppConfigUtil> mConfigPolicy = nullptr;

  mutable std::mutex mRequestPolicyLock;
  std::shared_ptr<IAppRequestUtil> mRequestPolicy = nullptr;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  typedef IDeviceSessionPolicyFactory::CreationParams CreationParams;

  explicit DeviceSessionPolicy(CreationParams const& creationParams);
  virtual ~DeviceSessionPolicy();

  virtual auto initialize() -> int;

 public:
  virtual auto evaluateConfiguration(ConfigurationInputParams const* in,
                                     ConfigurationOutputParams* out) -> int;

  virtual auto evaluateRequest(RequestInputParams const* in,
                               RequestOutputParams* out) -> int;

  virtual auto processResult(
      // ResultInputParams const* in,
      // ResultOutputParams* out
      ResultParams* params) -> int;

  virtual auto destroy() -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IStateCallback Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// public:  ////        Operations.
//  virtual void onConfigureFailed();
//
//  virtual void onConfigured();
//
//  virtual void onReady();
//
//  virtual void onSensorStateChanged(
//      SensorStateChangedResult const& sensorState);

 private:
  virtual auto onEvaluateConfiguration(ConfigurationInputParams const* in,
                                       ConfigurationOutputParams* out) -> int;

  virtual auto onEvaluateRequest(RequestInputParams const* in,
                                 RequestOutputParams* out) -> int;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace policy
};      // namespace device
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_DEVICESESSIONPOLICY_H_
