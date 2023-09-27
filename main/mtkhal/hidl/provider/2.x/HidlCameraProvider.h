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

#ifndef MAIN_MTKHAL_HIDL_PROVIDER_2_X_HIDLCAMERAPROVIDER_H_
#define MAIN_MTKHAL_HIDL_PROVIDER_2_X_HIDLCAMERAPROVIDER_H_

#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
#include <android/hardware/camera/provider/2.6/ICameraProviderCallback.h>
#include <android/hardware/camera/provider/2.6/types.h>
//
#include <main/mtkhal/hidl/provider/2.x/HidlCameraProviderCallback.h>
#include <mtkcam3/main/mtkhal/android/provider/2.x/IACameraProvider.h>
//
#include <utils/Mutex.h>
#include <utils/StrongPointer.h>
//
#include <memory>

using ::android::hardware::Return;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::camera::provider::V2_5::DeviceState;
using ::android::hardware::camera::provider::V2_6::CameraIdAndStreamCombination;
using ::android::hardware::camera::provider::V2_6::ICameraProvider;
using mcam::android::IACameraProvider;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace hidl {
//
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::VendorTagSection;

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraProvider : public ICameraProvider,
                           public ::android::hardware::hidl_death_recipient {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  struct CreationInfo {
    const char* providerName;
    std::shared_ptr<IACameraProvider> pProvider = nullptr;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                Data Members.
  std::shared_ptr<IACameraProvider> mProvider;
  hidl_vec<VendorTagSection> mVendorTagSections;

  ::android::sp<ICameraProviderCallback> mProviderCallback;
  ::android::Mutex mProviderCallbackLock;
  ::android::hidl::base::V1_0::DebugInfo mProviderCallbackDebugInfo;
  std::shared_ptr<HidlCameraProviderCallback> mHidlProviderCallback = nullptr;

 protected:  ////                Operations.
  bool setupVendorTags();

  // Helper for getCameraDeviceInterface_VN_x to use.
  template <class InterfaceT, class InterfaceCallbackT>
  void getCameraDeviceInterface(const hidl_string& cameraDeviceName,
                                InterfaceCallbackT _hidl_cb);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  virtual ~HidlCameraProvider();
  explicit HidlCameraProvider(const CreationInfo& info);
  static auto create(const CreationInfo& info) -> HidlCameraProvider*;

  virtual bool initialize();

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::hidl_death_recipient
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  virtual void serviceDied(
      uint64_t cookie,
      const ::android::wp<::android::hidl::base::V1_0::IBase>& who);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::provider::Vx_x::ICameraProvider Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// V2_4
  virtual Return<Status> setCallback(
      const ::android::sp<ICameraProviderCallback>& callback);

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
      ::android::hardware::hidl_bitfield<DeviceState> newState);

 public:  //// V2_6
  virtual Return<void> getConcurrentStreamingCameraIds(
      getConcurrentStreamingCameraIds_cb _hidl_cb);

  virtual Return<void> isConcurrentStreamCombinationSupported(
      const hidl_vec<CameraIdAndStreamCombination>& configs,
      isConcurrentStreamCombinationSupported_cb _hidl_cb);

 public:  //// Legacy Custom Provider
  virtual Return<void> debug(
      const ::android::hardware::hidl_handle& fd,
      const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
          options);

 private:
  virtual auto convertStreamConfigurationFromHidl(
      const ::android::hardware::camera::device::V3_4::StreamConfiguration&
          srcStreams,
      mcam::android::StreamConfiguration& dstStreams) -> int;
};

/******************************************************************************
 *
 ******************************************************************************/
// the types of the class factories
using mcam::hidl::HidlCameraProvider;
typedef HidlCameraProvider* CreateHidlProviderInstance_t(const char*);

}  // namespace hidl
}  // namespace mcam

#endif  // MAIN_MTKHAL_HIDL_PROVIDER_2_X_HIDLCAMERAPROVIDER_H_
