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

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICECALLBACK_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICECALLBACK_H_

#include <CusCameraCommon.h>
#include <CusCameraDevice.h>
#include <CusCameraDeviceSessionCallback.h>
#include "CusCameraDeviceSession.h"

#include <hidl/MQDescriptor.h>
#include <utils/Mutex.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraDeviceCallback
    : public ::android::hardware::camera::device::V3_5::ICameraDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~CusCameraDeviceCallback();
  explicit CusCameraDeviceCallback(int32_t InstanceId);
  static auto create(int32_t InstanceId) -> CusCameraDeviceCallback*;

 public:  ////        Operations.
  virtual auto setCustomSessionCallback(
      const std::shared_ptr<CustomCameraDeviceSessionCallback> callback)
      -> ::android::status_t;

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }
  auto getInstanceId() const { return mInstanceId; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  Return<void> processCaptureResult(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::CaptureResult>& results)
      override;

  Return<void> notify(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) override;

  Return<void> processCaptureResult_3_4(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_4::CaptureResult>& results)
      override;

  Return<void> requestStreamBuffers(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_5::BufferRequest>& bufReqs,
      requestStreamBuffers_cb _hidl_cb) override;

  Return<void> returnStreamBuffers(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::StreamBuffer>& buffers)
      override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  std::shared_ptr<CustomCameraDeviceSessionCallback>
      mCustomCameraDeviceCallback;
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;
  int32_t mInstanceId = -1;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace custom_dev3
};      // namespace NSCam
#endif  // MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICECALLBACK_H_
