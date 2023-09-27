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

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_FEATURE_BASIC_BASICSESSION_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_FEATURE_BASIC_BASICSESSION_H_

#include <IFeatureSession.h>
#include <utils/Mutex.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// #include "../../cuscallback/ICustCallback.h"

/******************************************************************************
 * A sample module derived from IFeatureSession for bypass mode.
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {
namespace feature {
namespace basic {

/******************************************************************************
 *
 ******************************************************************************/
class BasicSession : public IFeatureSession {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~BasicSession();

  static auto makeInstance(CreationParams const& rParams)
      -> std::shared_ptr<IFeatureSession>;

  explicit BasicSession(CreationParams const& rParams);

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }

 public:
  // Return<Status> flush();
  // Return<void> close();

  // Return<void> configureStreams(
  //     const ::android::hardware::camera::device::V3_5::StreamConfiguration&
  //         requestedConfiguration,
  //     android::hardware::camera::device::V3_6::ICameraDeviceSession::configureStreams_3_6_cb
  //     _hidl_cb);

  // Return<void> processCaptureRequest(
  //     const
  //     hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
  //     requests, const hidl_vec<BufferCache>& cachesToRemove, const
  //     std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
  //     android::hardware::camera::device::V3_4::ICameraDeviceSession::processCaptureRequest_3_4_cb
  //     _hidl_cb);

  Return<Status> flush() override;
  Return<void> close() override;

  Return<void> configureStreams(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration&
          requestedConfiguration,
      android::hardware::camera::device::V3_6::ICameraDeviceSession::
          configureStreams_3_6_cb _hidl_cb) override;

  Return<void> processCaptureRequest(
      const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
          requests,
      const hidl_vec<BufferCache>& cachesToRemove,
      const std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
      android::hardware::camera::device::V3_4::ICameraDeviceSession::
          processCaptureRequest_3_4_cb _hidl_cb) override;

  auto setMetadataQueue(
      std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
      std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue)
      -> void override;

//   auto getSafeCustCallback() const -> ::android::sp<ICustCallback>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  Return<void> notify(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) override;

  Return<void> processCaptureResult(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_4::CaptureResult>& results)
      override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  auto setExtCameraDeviceSession(
      const ::android::sp<IExtCameraDeviceSession>& session)
      -> ::android::status_t {
    return android::OK;
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  ::android::sp<::android::hardware::camera::device::V3_6::ICameraDeviceSession>
      mSession = nullptr;
  ::android::sp<
      ::android::hardware::camera::device::V3_5::ICameraDeviceCallback>
      mCameraDeviceCallback = nullptr;
  int32_t mInstanceId = -1;
  const camera_metadata* mCameraCharacteristics;
  std::vector<int32_t> mvSensorId;

 protected:
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;

 protected:  ////                Metadata Fast Message Queue (FMQ)
  std::shared_ptr<RequestMetadataQueue> mRequestMetadataQueue;
  std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

 private:
  camera_metadata* mLastSettings = nullptr;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  mutable ::android::Mutex mCustCallbackLock;
//   ::android::sp<ICustCallback> mCustCallback = nullptr;
  bool bEnableCustCallback = false;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace basic
};      // namespace feature
};      // namespace custom_dev3
};      // namespace NSCam
#endif  // MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_FEATURE_BASIC_BASICSESSION_H_
