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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_MTKCAMERADEVICE_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_MTKCAMERADEVICE_H_
//
// #include <mtkcam/utils/debug/debug.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDevice.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h>
#include <mtkcam3/main/mtkhal/devicemgr/ICameraDeviceManager.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
//
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
//
#include "../include/IMtkcamVirtualDeviceSession.h"

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {

/******************************************************************************
 *
 ******************************************************************************/
class MtkCameraDevice : public ICameraDeviceManager::IVirtualDevice,
                        public IMtkcamDevice,
                        public std::enable_shared_from_this<MtkCameraDevice> {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  typedef ICameraDeviceManager::IVirtualDevice IVirtualDevice;

  //     struct  MyDebuggee : public IDebuggee
  //     {
  //         static const std::string        mName;
  //         std::shared_ptr<IDebuggeeCookie>mCookie = nullptr;
  //         android::wp<MtkCameraDevice>  mContext = nullptr;

  //                         MyDebuggee(MtkCameraDevice* p) : mContext(p) {}
  //         virtual auto    debuggeeName() const -> std::string { return mName;
  //         } virtual auto    debug(
  //                             IPrinter& printer,
  //                             const std::vector<std::string>& options
  //                         ) -> void;
  //     };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  //  setup during constructor
  int32_t mLogLevel = 0;                              //  log level.
  ICameraDeviceManager* mDeviceManager = nullptr;     //  device manager.
  std::shared_ptr<Info> mStaticDeviceInfo = nullptr;  //  device info
  // std::shared_ptr<MyDebuggee>             mDebuggee = nullptr;
  ::android::sp<IMetadataProvider> mMetadataProvider = nullptr;
  ::android::sp<IMetadataConverter> mMetadataConverter = nullptr;
  std::shared_ptr<IMtkcamVirtualDeviceSession> mSession = nullptr;
  std::map<uint32_t, ::android::sp<IMetadataProvider>>
      mPhysicalMetadataProviders;
  mutable std::mutex mGetResourceLock;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  virtual ~MtkCameraDevice();
  MtkCameraDevice(ICameraDeviceManager* deviceManager,
                    IMetadataProvider* metadataProvider,
                    std::map<uint32_t, ::android::sp<IMetadataProvider>> const&
                        physicalMetadataProviders,
//                    char const* deviceType,
                    int32_t instanceId,
                    int32_t virtualInstanceId);

  virtual auto initialize(std::shared_ptr<IMtkcamVirtualDeviceSession> session)
      -> bool;

 public:  ////    Operations.
  auto getLogLevel() const { return mLogLevel; }
  auto const& getStaticDeviceInfo() const { return mStaticDeviceInfo; }
  auto const& getMetadataConverter() const { return mMetadataConverter; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    ICameraDeviceManager::IVirtualDevice Interfaces.
  virtual auto getDeviceInterface(
          std::shared_ptr<IVirtualDevice>& rpDevice) const
      -> int;

  virtual auto getDeviceInfo() const -> Info const&;

 public:  ////    IMtkcamDevice Interfaces implement.
  virtual auto getResourceCost(CameraResourceCost& resCost) const -> int;

  virtual auto setTorchMode(TorchMode mode) -> int;

  virtual auto open(const std::shared_ptr<IMtkcamDeviceCallback>& callback)
      -> std::shared_ptr<IMtkcamDeviceSession>;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void;

  virtual auto getCameraCharacteristics(IMetadata& cameraCharacteristics) const
      -> int;

  virtual auto getPhysicalCameraCharacteristics(
      int physicalId,
      IMetadata& physicalMetadata) const -> int;

  virtual auto isStreamCombinationSupported(
      const StreamConfiguration& streams,
      bool& isSupported) const -> int;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_MTKCAMERADEVICE_H_
