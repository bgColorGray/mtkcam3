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

#ifndef MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICECALLBACK_H_
#define MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICECALLBACK_H_

#include <mtkcam3/main/mtkhal/android/device/3.x/IACameraDeviceCallback.h>
#include <mtkcam3/main/mtkhal/android/device/3.x/types.h>

#include <HidlCameraDevice.h>

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include <memory>
#include <string>
#include <vector>

using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using mcam::android::IACameraDeviceCallback;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace hidl {

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraDeviceCallback : public IACameraDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  struct CreationInfo {
    int32_t instanceId;
    ::android::sp<ICameraDeviceCallback> pCallback = nullptr;
  };

  using ResultMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  int32_t mInstanceId = -1;
  std::string const mLogPrefix;
  ::android::sp<ICameraDeviceCallback> mCameraDeviceCallback = nullptr;

 protected:  //// Metadata Fast Message Queue (FMQ)
  std::unique_ptr<ResultMetadataQueue> mResultMetadataQueue;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~HidlCameraDeviceCallback();
  explicit HidlCameraDeviceCallback(const CreationInfo& info);
  static auto create(const CreationInfo& info) -> HidlCameraDeviceCallback*;

 public:  ////        IACameraDeviceCallback.
  virtual auto processCaptureResult(
      const std::vector<mcam::android::CaptureResult>& results) -> void;

  virtual auto notify(const std::vector<mcam::android::NotifyMsg>& msgs)
      -> void;

  virtual auto requestStreamBuffers(
      const std::vector<mcam::android::BufferRequest>& vBufferRequests,
      std::vector<mcam::android::StreamBufferRet>* pvBufferReturns)
      -> mcam::android::BufferRequestStatus;

  virtual auto returnStreamBuffers(
      const std::vector<mcam::android::StreamBuffer>& buffers) -> void;

 public:  ////        For HidlCameraDeviceSession.
  virtual auto getCaptureResultMetadataQueueDesc()
      -> const ResultMetadataQueue::Descriptor*;

  virtual auto getCameraDeviceCallback()
      -> ::android::sp<ICameraDeviceCallback>;

 private:  ////       interface/structure conversion helper
  auto const& getLogPrefix() const { return mLogPrefix; }

  virtual auto convertToHidlCaptureResults(
      const std::vector<mcam::android::CaptureResult>& hal_captureResults,
      std::vector<::android::hardware::camera::device::V3_4::CaptureResult>*
          hidl_captureResults) -> ::android::status_t;

  virtual auto convertToHidlCaptureResult(
      const mcam::android::CaptureResult& hal_captureResult,
      ::android::hardware::camera::device::V3_4::CaptureResult*
          hidl_captureResult) -> ::android::status_t;

  virtual auto convertToHidlStreamBuffer(
      const mcam::android::StreamBuffer& hal_buffer,
      ::android::hardware::camera::device::V3_2::StreamBuffer* hidl_buffer)
      -> ::android::status_t;

  virtual auto convertToHidlNotifyMsgs(
      const std::vector<mcam::android::NotifyMsg>& hal_messages,
      std::vector<::android::hardware::camera::device::V3_2::NotifyMsg>*
          hidl_messages) -> ::android::status_t;

  virtual auto convertToHidlErrorMessage(
      const mcam::android::ErrorMsg& hal_error,
      ::android::hardware::camera::device::V3_2::ErrorMsg* hidl_error)
      -> ::android::status_t;

  virtual auto convertToHidlShutterMessage(
      const mcam::android::ShutterMsg& hal_shutter,
      ::android::hardware::camera::device::V3_2::ShutterMsg* hidl_shutter)
      -> ::android::status_t;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace hidl
};      // namespace mcam
#endif  // MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICECALLBACK_H_
