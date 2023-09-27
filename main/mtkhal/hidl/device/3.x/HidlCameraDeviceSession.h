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

#ifndef MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICESESSION_H_
#define MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICESESSION_H_

#include <mtkcam3/main/mtkhal/android/device/3.x/IACameraDeviceSession.h>
#include <main/mtkhal/hidl/device/3.x/HidlCameraDeviceCallback.h>

#include <HidlCameraDevice.h>
#include <IBufferHandleCacheMgr.h>
// #include <mtkcam3/main/mtkhal3plus/extension/device/IExtCameraDeviceSession.h>

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <utils/Mutex.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_6::ICameraDeviceSession;
using mcam::android::IACameraDeviceSession;

/******************************************************************************
 *
 ******************************************************************************/

namespace mcam {
namespace hidl {

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraDeviceSession
    : public virtual ICameraDeviceSession,
      // public ::NSCam::IExtCameraDeviceSession,
      public ::android::hardware::hidl_death_recipient {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  struct CreationInfo {
    int32_t instanceId;
    std::shared_ptr<IACameraDeviceSession> pSession = nullptr;
    std::shared_ptr<HidlCameraDeviceCallback> pHidlCallback = nullptr;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~HidlCameraDeviceSession();
  explicit HidlCameraDeviceSession(const CreationInfo& info);
  static auto create(const CreationInfo& info) -> HidlCameraDeviceSession*;

 public:  ////        For HidlCameraDevice3
  virtual auto open(const ::android::sp<ICameraDeviceCallback>& callback)
      -> ::android::status_t;

 private:  ////        Operations.
  virtual auto initialize() -> ::android::status_t;
  virtual auto getSafeBufferHandleCacheMgr() const
      -> ::android::sp<IBufferHandleCacheMgr>;
  auto const& getLogPrefix() const { return mLogPrefix; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraDeviceSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  Return<void> constructDefaultRequestSettings(
      RequestTemplate type,
      constructDefaultRequestSettings_cb _hidl_cb) override;
  Return<void> configureStreams(
      const ::android::hardware::camera::device::V3_2::StreamConfiguration&
          requestedConfiguration,
      configureStreams_cb _hidl_cb) override;
  Return<void> getCaptureRequestMetadataQueue(
      getCaptureRequestMetadataQueue_cb _hidl_cb) override;
  Return<void> getCaptureResultMetadataQueue(
      getCaptureResultMetadataQueue_cb _hidl_cb) override;
  Return<void> processCaptureRequest(
      const hidl_vec<::android::hardware::camera::device::V3_2::CaptureRequest>&
          requests,
      const hidl_vec<BufferCache>& cachesToRemove,
      processCaptureRequest_cb _hidl_cb) override;
  Return<Status> flush() override;
  Return<void> close() override;

  // ::android::hardware::camera::device::V3_4
  Return<void> configureStreams_3_3(
      const ::android::hardware::camera::device::V3_2::StreamConfiguration&
          requestedConfiguration,
      configureStreams_3_3_cb _hidl_cb) override;
  Return<void> configureStreams_3_4(
      const ::android::hardware::camera::device::V3_4::StreamConfiguration&
          requestedConfiguration,
      configureStreams_3_4_cb _hidl_cb) override;
  Return<void> processCaptureRequest_3_4(
      const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
          requests,
      const hidl_vec<BufferCache>& cachesToRemove,
      processCaptureRequest_3_4_cb _hidl_cb) override;

  // ::android::hardware::camera::device::V3_5
  Return<void> configureStreams_3_5(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration&
          requestedConfiguration,
      configureStreams_3_5_cb _hidl_cb) override;
  Return<void> signalStreamFlush(const hidl_vec<int32_t>& streamIds,
                                 uint32_t streamConfigCounter) override;
  Return<void> isReconfigurationRequired(
      const hidl_vec<uint8_t>& oldSessionParams,
      const hidl_vec<uint8_t>& newSessionParams,
      isReconfigurationRequired_cb _hidl_cb) override;

  // ::android::hardware::camera::device::V3_6
  Return<void> configureStreams_3_6(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration&
          requestedConfiguration,
      configureStreams_3_6_cb _hidl_cb) override;
  Return<void> switchToOffline(const hidl_vec<int32_t>& streamsToKeep,
                               switchToOffline_cb _hidl_cb) override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::hidl_death_recipient
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  void serviceDied(
      uint64_t cookie,
      const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IExtCameraDeviceSession Interfaces
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  // auto getConfigurationResults(const uint32_t streamConfigCounter,  // input
  //     getConfigurationResults_cb _ext_cb)  // output
  //     -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:
  std::shared_ptr<IACameraDeviceSession> mSession = nullptr;
  ::android::sp<ICameraDeviceCallback> mCameraDeviceCallback = nullptr;
  std::shared_ptr<HidlCameraDeviceCallback> mHidlCallback = nullptr;

 protected:
  std::mutex mInterfaceLock;
  int32_t mInstanceId = -1;
  std::string const mLogPrefix;
  int32_t mConvertTimeDebug;

  // linkToDeath
  ::android::hidl::base::V1_0::DebugInfo mLinkToDeathDebugInfo;

  mutable ::android::Mutex mBufferHandleCacheMgrLock;
  ::android::sp<IBufferHandleCacheMgr> mBufferHandleCacheMgr;

 protected:  ////                Metadata Fast Message Queue (FMQ)
  using RequestMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
  std::unique_ptr<RequestMetadataQueue> mRequestMetadataQueue;

 private:  ////        interface/structure conversion helper
  virtual auto convertStreamConfigurationFromHidl(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration&
          srcStreams,
      mcam::android::StreamConfiguration& dstStreams)
      -> ::android::status_t;

  virtual auto convertHalStreamConfigurationToHidl(
      const mcam::android::HalStreamConfiguration& srcStreams,
      ::android::hardware::camera::device::V3_6::HalStreamConfiguration&
          dstStreams) -> ::android::status_t;

  virtual auto convertAllRequestFromHidl(
      const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
          inRequests,
      RequestMetadataQueue* pRequestFMQ,
      std::vector<mcam::android::CaptureRequest>& outRequests,
      std::vector<RequestBufferCache>* requestBufferCache)
      -> ::android::status_t;

  virtual auto convertToHalCaptureRequest(
      const ::android::hardware::camera::device::V3_4::CaptureRequest&
          hidl_request,
      RequestMetadataQueue* request_metadata_queue,
      mcam::android::CaptureRequest* hal_request,
      RequestBufferCache* requestBufferCache) -> ::android::status_t;

  virtual auto convertToHalMetadata(
      uint32_t message_queue_setting_size,
      RequestMetadataQueue* request_metadata_queue,
      const CameraMetadata& request_settings,
      camera_metadata** hal_metadata) -> ::android::status_t;

  virtual auto convertToHalStreamBuffer(
      const StreamBuffer& hidl_buffer,
      mcam::android::StreamBuffer* hal_buffer,
      const BufferHandle* importedBufferHandle) -> ::android::status_t;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace hidl
};      // namespace mcam
#endif  // MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERADEVICESESSION_H_
