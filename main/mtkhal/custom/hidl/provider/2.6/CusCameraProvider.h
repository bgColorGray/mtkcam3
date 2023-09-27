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

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_PROVIDER_2_6_CUSCAMERAPROVIDER_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_PROVIDER_2_6_CUSCAMERAPROVIDER_H_
//
#include <utils/Mutex.h>
#include <utils/StrongPointer.h>
//
#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
#include <android/hardware/camera/provider/2.6/ICameraProviderCallback.h>
#include <android/hardware/camera/provider/2.6/types.h>
//
#include <dlfcn.h>
// TODO(MTK): remove HidlCameraProvider
#include <main/mtkhal/hidl/provider/2.x/HidlCameraProvider.h>
//
/******************************************************************************
 *
 ******************************************************************************/
namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_6 {
//
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::VendorTagSection;
using mcam::hidl::HidlCameraProvider;

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraProvider : public ICameraProvider,
                          public android::hardware::hidl_death_recipient,
                          public ICameraProviderCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                Data Members.
  hidl_vec<VendorTagSection> mVendorTagSections;

  ::android::sp<V2_4::ICameraProviderCallback> mProviderCallback;
  ::android::Mutex mProviderCallbackLock;
  ::android::hidl::base::V1_0::DebugInfo mProviderCallbackDebugInfo;

  // HAL3+
  void* mLibHandle = NULL;
  const char* camProviderLib = "libmtkcam_hal_hidl_provider.so";
  mutable android::Mutex mHidlCameraProviderLock;
  ::android::sp<HidlCameraProvider> mHidlCameraProvider = nullptr;

 protected:  ////                Operations.
  // HAL3+
  auto getSafeHidlCameraProvider() const -> ::android::sp<HidlCameraProvider>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  virtual ~CusCameraProvider();
  explicit CusCameraProvider(const char* providerName);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::hidl_death_recipient
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  virtual void serviceDied(uint64_t cookie,
                           const wp<hidl::base::V1_0::IBase>& who);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::provider::Vx_x::ICameraProvider Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// V2_4
  virtual Return<Status> setCallback(
      const ::android::sp<V2_4::ICameraProviderCallback>& callback);

  virtual Return<void> getVendorTags(getVendorTags_cb _hidl_cb);

  virtual Return<void> getCameraIdList(getCameraIdList_cb _hidl_cb);

  virtual Return<void> isSetTorchModeSupported(
      isSetTorchModeSupported_cb _hidl_cb);

  virtual Return<void> getCameraDeviceInterface_V1_x(
      const hidl_string& cameraDeviceName,
      getCameraDeviceInterface_V1_x_cb _hidl_cb);

  virtual Return<void> getCameraDeviceInterface_V3_x(
      const hidl_string& cameraDeviceName,
      getCameraDeviceInterface_V3_x_cb _hidl_cb);

 public:  //// V2_5
  virtual Return<void> notifyDeviceStateChange(
      hardware::hidl_bitfield<V2_5::DeviceState> newState);

 public:  //// V2_6
  virtual Return<void> getConcurrentStreamingCameraIds(
      getConcurrentStreamingCameraIds_cb _hidl_cb);

  virtual Return<void> isConcurrentStreamCombinationSupported(
      const hidl_vec<CameraIdAndStreamCombination>& configs,
      isConcurrentStreamCombinationSupported_cb _hidl_cb);

 public:  ////
  virtual Return<void> debug(
      const ::android::hardware::hidl_handle& fd,
      const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
          options);

  /******************************************************************************
   * ::android::hardware::camera::provider::Vx_x::ICameraProviderCallback
   *Interfaces.
   ******************************************************************************/
 public:
  virtual Return<void> cameraDeviceStatusChange(
      const hidl_string& cameraDeviceName,
      ::android::hardware::camera::common::V1_0::CameraDeviceStatus newStatus);

  virtual Return<void> torchModeStatusChange(
      const hidl_string& cameraDeviceName,
      ::android::hardware::camera::common::V1_0::TorchModeStatus newStatus);

  virtual Return<void> physicalCameraDeviceStatusChange(
      const hidl_string& cameraDeviceName,
      const hidl_string& physicalCameraDeviceName,
      ::android::hardware::camera::common::V1_0::CameraDeviceStatus newStatus);
};

}  // namespace V2_6
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android

// the types of the class factories
typedef mcam::hidl::HidlCameraProvider* CreateHidlProviderInstance_t(
    const char*);

#endif  // MAIN_MTKHAL_CUSTOM_HIDL_PROVIDER_2_6_CUSCAMERAPROVIDER_H_
