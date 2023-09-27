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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef MAIN_MTKHAL_ANDROID_DEVICE_3_X_ACAMERADEVICECALLBACK_H_
#define MAIN_MTKHAL_ANDROID_DEVICE_3_X_ACAMERADEVICECALLBACK_H_

#include <mtkcam3/main/mtkhal/android/device/3.x/IACameraDeviceCallback.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h>

#include <memory>
#include <string>
#include <vector>

namespace mcam {
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
class ACameraDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  struct CreationInfo {
    int32_t instanceId;
    std::shared_ptr<IACameraDeviceCallback> pCallback = nullptr;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  int32_t mInstanceId = -1;
  std::string const mLogPrefix;
  std::weak_ptr<IACameraDeviceCallback> mCallback;

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~ACameraDeviceCallback();
  explicit ACameraDeviceCallback(const CreationInfo& info);
  static auto create(const CreationInfo& info) -> ACameraDeviceCallback*;

 public:  ////        For ACameraDeviceSession.
  virtual auto processCaptureResult(const std::vector<CaptureResult>& results)
      -> void;

  virtual auto notify(const std::vector<NotifyMsg>& msgs) -> void;

#if 0  // not implement HAL Buffer Management
  virtual auto requestStreamBuffers(
      const std::vector<BufferRequest>& vBufferRequests,
      std::vector<StreamBufferRet>* pvBufferReturns)
      -> BufferRequestStatus;

  virtual auto returnStreamBuffers(const std::vector<StreamBuffer>& buffers)
      -> void;
#endif
};

};      // namespace android
};      // namespace mcam

#endif  // MAIN_MTKHAL_ANDROID_DEVICE_3_X_ACAMERADEVICECALLBACK_H_
