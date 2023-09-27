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

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICESESSION_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICESESSION_H_

#include <CusCameraCommon.h>
#include <CusCameraDevice.h>
#include <CusCameraDeviceSessionCallback.h>
#include <IBufferHandleCacheMgr.h>
#include <IFeatureSession.h>

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <utils/Mutex.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// extended devicesession interface
#include <mtkcam3/main/mtkhal3plus/extension/device/IExtCameraDeviceSession.h>

using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraDeviceSession
    : public ::android::hardware::camera::device::V3_6::ICameraDeviceSession,
      public ::android::hardware::hidl_death_recipient {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~CusCameraDeviceSession();
  explicit CusCameraDeviceSession(
      const ::android::sp<ICameraDeviceSession>& session,
      int32_t InstanceId,
      camera_metadata* staticMetadata);
  static auto create(const ::android::sp<ICameraDeviceSession>& session,
                     int32_t InstanceId,
                     camera_metadata* staticMetadata)
      -> CusCameraDeviceSession*;

 public:  ////        Operations.
  virtual auto open(
      const ::android::sp<
          ::android::hardware::camera::device::V3_5::ICameraDeviceCallback>&
          callback) -> ::android::status_t;

  virtual auto createCustomSessionCallbacks()
      -> std::shared_ptr<custom_dev3::CustomCameraDeviceSessionCallback>;

  auto getInstanceId() const { return mInstanceId; }
  camera_metadata const* getStaticMetadata() const { return mStaticInfo; }

 private:  ////        Operations.
  virtual auto initialize() -> ::android::status_t;
  virtual auto getSafeBufferHandleCacheMgr() const
      -> ::android::sp<mcam::hidl::IBufferHandleCacheMgr>;
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
      const android::wp<android::hidl::base::V1_0::IBase>& who) override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:
  virtual auto processCaptureResult(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_4::CaptureResult>& results)
      -> void;

  virtual auto notify(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) -> void;

  virtual auto requestStreamBuffers(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_5::BufferRequest>&
          buffer_requests,
      ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_5::StreamBufferRet>**
          pvBufferReturns)
      -> ::android::hardware::camera::device::V3_5::BufferRequestStatus;

  virtual auto returnStreamBuffers(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::StreamBuffer>& buffers)
      -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:
  ::android::sp<ICameraDeviceSession> mSession = nullptr;
  ::android::sp<
      ::android::hardware::camera::device::V3_5::ICameraDeviceCallback>
      mCameraDeviceCallback = nullptr;

 private:
  std::shared_ptr<feature::IFeatureSession> mFeatureSession = nullptr;
  // determined in configuration time
  int32_t mDptzMode = 0;

 protected:
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;
  int32_t mInstanceId = -1;
  camera_metadata* mStaticInfo;

  // extended devicesession interface
  void* mLibHandle = NULL;
  const char* camDevice3HidlLib = "libmtkcam_hal_hidl_device.so";
  mutable android::Mutex mExtCameraDeviceSessionLock;
  ::android::sp<IExtCameraDeviceSession> mExtCameraDeviceSession;

  // linkToDeath
  ::android::hidl::base::V1_0::DebugInfo mLinkToDeathDebugInfo;

  mutable android::Mutex mBufferHandleCacheMgrLock;
  ::android::sp<mcam::hidl::IBufferHandleCacheMgr> mBufferHandleCacheMgr;

 protected:  ////                Metadata Fast Message Queue (FMQ)
  using RequestMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
  std::shared_ptr<RequestMetadataQueue> mRequestMetadataQueue;

  using ResultMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
  std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

  //  private:  ////
 private:
  auto convertAllRequestFromHidl(
      const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
          inRequests,
      std::vector<::android::hardware::camera::device::V3_4::CaptureRequest>&
          outRequests,
      RequestMetadataQueue* pRequestFMQ,
      CameraMetadata& metadata_queue_settings) -> ::android::status_t;

  auto convertRequestFromHidl(
      const ::android::hardware::camera::device::V3_4::CaptureRequest&
          hidl_request,
      ::android::hardware::camera::device::V3_4::CaptureRequest& outRequest,
      RequestMetadataQueue* request_metadata_queue,
      CameraMetadata& metadata_queue_settings) -> ::android::status_t;

  auto readMetadatafromFMQ(const uint64_t message_queue_setting_size,
                           RequestMetadataQueue* request_metadata_queue,
                           const CameraMetadata& request_setting,
                           CameraMetadata& out_request_setting,
                           CameraMetadata& metadata_queue_settings)
      -> ::android::status_t;
};

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace custom_dev3
};  // namespace NSCam

// the types of the class factories
typedef NSCam::IExtCameraDeviceSession* CreateExtCameraDeviceSessionInstance_t(
    int32_t);

#endif  // MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_CUSCAMERADEVICESESSION_H_
