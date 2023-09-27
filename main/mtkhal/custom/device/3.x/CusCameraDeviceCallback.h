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

#ifndef MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICECALLBACK_H_
#define MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICECALLBACK_H_

#include <mtkcam3/main/mtkhal/core/common/1.x/types.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// using mcam::BufferRequest;
// using mcam::BufferRequestStatus;
using mcam::CaptureResult;
using mcam::IMtkcamDeviceCallback;
using mcam::NotifyMsg;
using mcam::StreamBuffer;
// using mcam::StreamBufferRet;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace custom {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraDeviceCallback : public mcam::IMtkcamDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~CusCameraDeviceCallback();
  explicit CusCameraDeviceCallback(int32_t InstanceId);
  static auto create(int32_t InstanceId) -> CusCameraDeviceCallback*;

 public:  ////        Operations.
  virtual void setCustomSessionCallback(
      const std::shared_ptr<mcam::IMtkcamDeviceCallback> callback);

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }
  auto getInstanceId() const { return mInstanceId; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  void processCaptureResult(
      const std::vector<mcam::CaptureResult>& results) override;

  void notify(const std::vector<mcam::NotifyMsg>& msgs) override;
#if 0
  mcam::BufferRequestStatus requestStreamBuffers(
      const std::vector<mcam::BufferRequest>& vBufferRequests,
      std::vector<mcam::StreamBufferRet>* pvBufferReturns);

  void returnStreamBuffers(
      const std::vector<mcam::StreamBuffer>& buffers);
#endif
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  std::weak_ptr<mcam::IMtkcamDeviceCallback>
      mCustomCameraDeviceCallback;
  // std::mutex mInterfaceLock;
  std::string const mLogPrefix;
  int32_t mInstanceId = -1;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace custom
};      // namespace mcam
#endif  // MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERADEVICECALLBACK_H_
