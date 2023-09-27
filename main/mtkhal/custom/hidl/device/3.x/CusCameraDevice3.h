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

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICE3_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICE3_H_

#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/debug/debug.h>
#include <utils/Mutex.h>
#include <utils/RWLock.h>
#include <utils/String8.h>
//
#include <CusCameraDevice.h>
//
#include <memory>
#include <string>
#include <vector>
//
//#include "../../../../device/3.x/device/CameraDevice3Impl.h"

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::camera::common::V1_0::CameraResourceCost;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::TorchMode;
// using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_5::ICameraDevice;
// using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hidl::base::V1_0::IBase;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraDevice3
    : public ::android::hardware::camera::device::V3_6::ICameraDevice {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  //  setup during constructor
  int32_t mLogLevel = 0;  //  log level.
  int32_t mInstanceId = -1;
  ::android::sp<ICameraDevice> mDevice = nullptr;
  camera_metadata* mStaticInfo;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  virtual ~CusCameraDevice3();
  CusCameraDevice3(
      ::android::sp<::android::hardware::camera::device::V3_6::ICameraDevice>
          device,
      int32_t instanceId);
  static auto create(
      ::android::sp<::android::hardware::camera::device::V3_6::ICameraDevice>
          device,
      int32_t instanceId)
      -> ::android::hardware::camera::device::V3_2::ICameraDevice*;
  //-> ::android::hidl::base::V1_0::IBase*;
 public:  ////    Operations.
  auto getLogLevel() const { return mLogLevel; }
  auto getInstanceId() const { return mInstanceId; }
  camera_metadata const* getStaticMetadata() const { return mStaticInfo; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraDevice Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  Return<void> getResourceCost(getResourceCost_cb _hidl_cb) override;

  Return<void> getCameraCharacteristics(
      getCameraCharacteristics_cb _hidl_cb) override;

  Return<Status> setTorchMode(TorchMode mode) override;

  Return<void> open(
      const ::android::sp<
          ::android::hardware::camera::device::V3_2::ICameraDeviceCallback>&
          callback,
      open_cb _hidl_cb) override;

  Return<void> dumpState(
      const ::android::hardware::hidl_handle& handle) override;

  Return<void> debug(
      const ::android::hardware::hidl_handle& fd,
      const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
          options) override;

  Return<void> getPhysicalCameraCharacteristics(
      const ::android::hardware::hidl_string& physicalCameraId,
      getPhysicalCameraCharacteristics_cb _hidl_cb) override;

  Return<void> isStreamCombinationSupported(
      const ::android::hardware::camera::device::V3_4::StreamConfiguration&
          streams,
      isStreamCombinationSupported_cb _hidl_cb) override;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace custom_dev3
};      // namespace NSCam
#endif  // MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICE3_H_
