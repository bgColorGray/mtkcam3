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

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_INCLUDE_IFEATURESESSION_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_INCLUDE_IFEATURESESSION_H_

#include <CusCameraCommon.h>
#include <CusCameraDevice.h>
#include <IBufferHandleCacheMgr.h>
// extended devicesession interface
#include <mtkcam3/main/mtkhal3plus/extension/device/IExtCameraDeviceSession.h>

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include <memory>
#include <string>
#include <vector>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {
namespace feature {

enum class FeatureSessionType : uint32_t {
  BASIC = 0,
  SAMPLE,
  DPTZ,
  LOW_LATENCY,
  // DTPZ, LOW_LATENCY, ..., etc.
};

/******************************************************************************
 *
 ******************************************************************************/
class IFeatureSession {
  // TBD. should be moved to callback module
 public:
  using RequestMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;

  using ResultMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// Instantiation.
  virtual ~IFeatureSession() = default;

  // TBD. should be moved to callback module
  virtual auto setMetadataQueue(
      std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
      std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue) -> void = 0;

 public:  //// Operations.
  virtual Return<Status> flush() = 0;
  virtual Return<void> close() = 0;

  virtual Return<void> configureStreams(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration&
          requestedConfiguration,
      android::hardware::camera::device::V3_6::ICameraDeviceSession::
          configureStreams_3_6_cb _hidl_cb) = 0;

  virtual Return<void> processCaptureRequest(
      const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
          requests,
      const hidl_vec<BufferCache>& cachesToRemove,
      const std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
      android::hardware::camera::device::V3_4::ICameraDeviceSession::
          processCaptureRequest_3_4_cb _hidl_cb) = 0;

 public:  //// Callbacks.
  virtual Return<void> notify(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) = 0;

  virtual Return<void> processCaptureResult(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_4::CaptureResult>&
          results) = 0;

 public:
  virtual auto setExtCameraDeviceSession(
      const ::android::sp<IExtCameraDeviceSession>& session)
      -> ::android::status_t = 0;
};

/**
 * A structure for creation parameters.
 */
struct CreationParams {
  /**
   *  Logical device open id
   */
  int32_t openId = -1;

  /**
   *  Static metadata for this logical device
   */
  const camera_metadata* cameraCharateristics;

  /**
   *  Physical sensor id (0, 1, 2)
   */
  std::vector<int32_t> sensorId;

  /**
   *  ICameraDeviceSession instance of mtk zone.
   *  IFeatureSession should directly call ICameraDeviceSession w/ HIDL API.
   */
  const ::android::sp<
      ::android::hardware::camera::device::V3_6::ICameraDeviceSession>& session;

  /**
   *  ICameraDeviceCallback instance of camera framework.
   *  IFeatureSession should follow AOSP callback rules to handle framework's
   * streams, result metadata, and notifications.
   *
   *  Follow-ups: custom zone provide custom callback module to handle AOSP
   * rules and FMQ. However, IFeatureSession implementor must callback
   * frameworks's streams, last partial information, and notifications.
   */
  const ::android::sp<
      ::android::hardware::camera::device::V3_5::ICameraDeviceCallback>&
      callback;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace feature
};      // namespace custom_dev3
};      // namespace NSCam
#endif  // MAIN_MTKHAL_CUSTOM_HIDL_INCLUDE_IFEATURESESSION_H_
