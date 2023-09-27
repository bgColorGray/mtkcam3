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

#ifndef MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICE_H_
#define MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICE_H_

// #include <mtkcam/utils/metadata/IMetadataConverter.h>
// #include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam3/main/mtkhal/core/common/1.x/types.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDevice.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>

#include <memory>
#include <string>
#include <vector>

using mcam::CameraResourceCost;
using mcam::IMtkcamDevice;
using mcam::IMtkcamDeviceCallback;
using mcam::IMtkcamDeviceSession;
using mcam::StreamConfiguration;
using NSCam::IMetadata;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace custom {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraDevice : public mcam::IMtkcamDevice {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  //  setup during constructor
  int32_t mLogLevel = 0;  //  log level.
  int32_t mInstanceId = -1;
  std::shared_ptr<mcam::IMtkcamDevice> mDevice = nullptr;
  NSCam::IMetadata mStaticInfo;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  virtual ~CusCameraDevice();
  CusCameraDevice(std::shared_ptr<mcam::IMtkcamDevice>& device,
                  int32_t instanceId);
  static auto create(std::shared_ptr<mcam::IMtkcamDevice>& device,
                     int32_t instanceId) -> CusCameraDevice*;

 public:  ////    Operations.
  auto getLogLevel() const { return mLogLevel; }
  auto getInstanceId() const { return mInstanceId; }
  NSCam::IMetadata const& getStaticMetadata() const { return mStaticInfo; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraDevice Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  int getResourceCost(mcam::CameraResourceCost& mtkCost) const override;

  int getCameraCharacteristics(IMetadata& cameraCharacteristics) const override;

  int setTorchMode(mcam::TorchMode mode) override;

  std::shared_ptr<mcam::IMtkcamDeviceSession> open(
      const std::shared_ptr<mcam::IMtkcamDeviceCallback>& callback)
      override;

  void dumpState(NSCam::IPrinter& printer,
                 const std::vector<std::string>& options) override;

  //   Return<void> debug(
  //       const ::android::hardware::hidl_handle& fd,
  //       const
  //       ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
  //           options) override;

  int getPhysicalCameraCharacteristics(
      int32_t physicalId,
      NSCam::IMetadata& physicalMetadata) const override;

  int isStreamCombinationSupported(
      const mcam::StreamConfiguration& streams,
      bool& isSupported) const override;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace custom
};      // namespace mcam
#endif  // MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICE_H_
