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
/* MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERAOFFLINESESSION_H_
#define MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERAOFFLINESESSION_H_

#include <mtkcam3/main/mtkhal/android/device/3.x/IACameraOfflineSession.h>

#include <fmq/MessageQueue.h>

#include <memory>
#include <string>

#include <HidlCameraDevice.h>

using ::android::hardware::Return;
using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_6::ICameraOfflineSession;
using mcam::android::IACameraDeviceCallback;
using mcam::android::IACameraOfflineSession;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace hidl {

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraOfflineSession : public ICameraOfflineSession {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  struct CreationInfo {
    int32_t instanceId;
    std::shared_ptr<IACameraOfflineSession> pSession = nullptr;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  //  setup during constructor
  int32_t mInstanceId = -1;
  std::string const mLogPrefix;

  //  setup during opening camera
  int32_t mLogLevel = 0;
  std::shared_ptr<IACameraOfflineSession> mSession = nullptr;
  ::android::sp<ICameraDeviceCallback> mCameraDeviceCallback = nullptr;

  using ResultMetadataQueue = ::android::hardware::
      MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
  std::unique_ptr<ResultMetadataQueue> mResultMetadataQueue;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~HidlCameraOfflineSession();
  explicit HidlCameraOfflineSession(const CreationInfo& info);
  static auto create(const CreationInfo& info) -> HidlCameraOfflineSession*;

 public:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraOfflineSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  Return<void> setCallback(
      const ::android::sp<ICameraDeviceCallback>& callback) override;
  Return<void> getCaptureResultMetadataQueue(
      getCaptureResultMetadataQueue_cb _hidl_cb) override;
  Return<void> close() override;
};

};      // namespace hidl
};      // namespace mcam
#endif  // MAIN_MTKHAL_HIDL_DEVICE_3_X_HIDLCAMERAOFFLINESESSION_H_
