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

#ifndef MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICESESSION_H_
#define MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICESESSION_H_

#include <CusCameraDeviceCallback.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam3/main/mtkhal/core/common/1.x/types.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
// #include <mtkcam/utils/metadata/IMetadata.h>
// #include <system/camera_metadata.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "feature/IFeatureSession.h"

// extended devicesession interface
// #include
// <mtkcam3/main/mtkhal3plus/extension/device/IExtCameraDeviceSession.h>
// //needed?

// using mcam::BufferRequest;
// using mcam::BufferRequestStatus;
using mcam::CameraOfflineSessionInfo;
using mcam::CaptureResult;
// using mcam::ExtConfigurationResults;
using mcam::HalStreamConfiguration;
using mcam::IMtkcamDeviceCallback;
using mcam::IMtkcamDeviceSession;
using mcam::NotifyMsg;
using mcam::RequestTemplate;
using mcam::StreamBuffer;
// using mcam::StreamBufferRet;
using mcam::StreamConfiguration;
using mcam::custom::CusCameraDeviceCallback;
using NSCam::IMetadata;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace custom {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraDeviceSession : public mcam::IMtkcamDeviceSession,
                               public mcam::IMtkcamDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~CusCameraDeviceSession();
  explicit CusCameraDeviceSession(int32_t intanceId,
                                  NSCam::IMetadata staticMetadata);
  static auto create(int32_t intanceId, NSCam::IMetadata staticMetadata)
      -> CusCameraDeviceSession*;

 public:  ////        Operations.
  virtual void open(
      const std::shared_ptr<mcam::IMtkcamDeviceCallback>& callback);

  //   virtual auto createCustomSessionCallbacks()
  //       -> std::shared_ptr<custom_dev3::CustomCameraDeviceSessionCallback>;

  auto getInstanceId() const { return mInstanceId; }
  NSCam::IMetadata const getStaticMetadata() const { return mStaticInfo; }

  void setCameraDeviceSession(
      const std::shared_ptr<mcam::IMtkcamDeviceSession>& session);

 private:  ////        Operations.
  virtual int initialize();
  auto const& getLogPrefix() const { return mLogPrefix; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraDeviceSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  int constructDefaultRequestSettings(RequestTemplate type,
                                      IMetadata& requestTemplate) override;
  int configureStreams(
      const mcam::StreamConfiguration& requestedConfiguration,
      mcam::HalStreamConfiguration& halConfiguration) override;
  int processCaptureRequest(
      const std::vector<mcam::CaptureRequest>& requests,
      uint32_t& numRequestProcessed) override;
  int flush() override;
  int close() override;
  // ::android::hardware::camera::device::V3_5
  void signalStreamFlush(const std::vector<int32_t>& streamIds,
                         uint32_t streamConfigCounter) override;
  int isReconfigurationRequired(const NSCam::IMetadata& oldSessionParams,
                                const NSCam::IMetadata& newSessionParams,
                                bool& reconfigurationNeeded) const override;

  // ::android::hardware::camera::device::V3_6
  int switchToOffline(
      const std::vector<int64_t>& streamsToKeep,
      mcam::CameraOfflineSessionInfo& offlineSessionInfo,
      std::shared_ptr<IMtkcamOfflineSession>& offlineSession) override;
#if 1
  int getConfigurationResults(
      const uint32_t streamConfigCounter,
      ExtConfigurationResults& ConfigurationResults) override;
#endif

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  void notify(const std::vector<mcam::NotifyMsg>& msgs) override;

  void processCaptureResult(
      const std::vector<mcam::CaptureResult>& results) override;
#if 0
  mcam::BufferRequestStatus requestStreamBuffers(
      const std::vector<mcam::BufferRequest>& vBufferRequests,
      std::vector<mcam::StreamBufferRet>* pvBufferReturns) override;

  void returnStreamBuffers(
      const std::vector<mcam::StreamBuffer>& buffers) override;
#endif
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:
  std::shared_ptr<mcam::IMtkcamDeviceSession> mSession = nullptr;
  std::shared_ptr<CusCameraDeviceCallback> mCameraDeviceCallback = nullptr;
  std::shared_ptr<mcam::custom::feature::IFeatureSession>
      mFeatureSession = nullptr;

 protected:
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;
  int32_t mInstanceId = -1;
  NSCam::IMetadata mStaticInfo;
};

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace custom
};  // namespace mcam

#endif  // MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICESESSION_H_
