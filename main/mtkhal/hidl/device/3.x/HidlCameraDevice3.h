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

#ifndef MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICE3_H_
#define MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICE3_H_

#include <mtkcam3/main/mtkhal/android/device/3.x/IACameraDevice.h>
#include <mtkcam/utils/debug/debug.h>
#include <utils/Mutex.h>
#include <utils/RWLock.h>
#include <utils/String8.h>
//
#include <HidlCameraDevice.h>
//
#include <memory>
#include <string>
#include <vector>

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::camera::common::V1_0::CameraResourceCost;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::device::V3_6::ICameraDevice;
using ::android::hidl::base::V1_0::IBase;
using mcam::android::IACameraDevice;
using NSCam::IDebuggee;
using NSCam::IDebuggeeCookie;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace hidl {

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraDevice3 : public ICameraDevice {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  struct CreationInfo {
    int32_t instanceId;
    std::shared_ptr<IACameraDevice> pDevice = nullptr;
  };

  struct MyDebuggee : public IDebuggee {
    static const char* mName;
    std::shared_ptr<IDebuggeeCookie> mCookie = nullptr;
    ::android::wp<HidlCameraDevice3> mContext = nullptr;

    explicit MyDebuggee(HidlCameraDevice3* p) : mContext(p) {}
    virtual auto debuggeeName() const -> std::string { return mName; }
    virtual auto debug(::android::Printer& printer,
                       const std::vector<std::string>& options) -> void;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  //  setup during constructor
  int32_t mLogLevel = 0;  //  log level.
  int32_t mInstanceId = -1;
  std::shared_ptr<MyDebuggee> mDebuggee = nullptr;
  std::shared_ptr<IACameraDevice> mDevice = nullptr;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  virtual ~HidlCameraDevice3();
  explicit HidlCameraDevice3(const CreationInfo& info);
  static auto create(const CreationInfo& info)
      -> ::android::hidl::base::V1_0::IBase*;

 public:  ////    Operations.
  auto getLogLevel() const { return mLogLevel; }
  auto getInstanceId() const { return mInstanceId; }
  auto debug(IPrinter& printer, const std::vector<std::string>& options)
      -> void;

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
          callback_3_2,
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

 private:  ////        interface/structure conversion helper
  virtual auto convertFromHidl(
      const ::android::hardware::camera::device::V3_4::StreamConfiguration&
          srcStreams,
      mcam::android::StreamConfiguration& dstStreams) -> void;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace hidl
};      // namespace mcam
#endif  // MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICE3_H_
