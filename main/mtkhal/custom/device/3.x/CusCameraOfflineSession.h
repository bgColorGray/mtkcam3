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

#ifndef MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERAOFFLINESESSION_H_
#define MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERAOFFLINESESSION_H_
//

#include <CusCameraDeviceCallback.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamOfflineSession.h>

#include <memory>
#include <string>
#include <vector>

// using ::android::hardware::Return;
using mcam::IMtkcamOfflineSession;
using mcam::custom::CusCameraDeviceCallback;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace custom {

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraOfflineSession : public mcam::IMtkcamOfflineSession,
                                public mcam::IMtkcamDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Not implement

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  //  setup during constructor
  std::string const mLogPrefix;
  int32_t mInstanceId = -1;

  //  setup during opening camera
  int32_t mLogLevel = 0;
  std::shared_ptr<mcam::IMtkcamOfflineSession> mSession = nullptr;
  std::shared_ptr<mcam::custom::CusCameraDeviceCallback>
      mCameraDeviceCallback = nullptr;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~CusCameraOfflineSession();
  CusCameraOfflineSession(
      const std::shared_ptr<mcam::IMtkcamOfflineSession>& session);
  static auto create(
      const std::shared_ptr<mcam::IMtkcamOfflineSession>& session)
      -> CusCameraOfflineSession*;

 public:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }
  auto getInstanceId() const { return mInstanceId; }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraOfflineSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  void setCallback(
      const std::shared_ptr<mcam::IMtkcamDeviceCallback>& callback);
  int close();

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IMtkcamDeviceCallback Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  void processCaptureResult(
      const std::vector<mcam::CaptureResult>& results);

  void notify(const std::vector<mcam::NotifyMsg>& msgs);
#if 0
  mcam::BufferRequestStatus requestStreamBuffers(
      const std::vector<mcam::BufferRequest>& vBufferRequests,
      std::vector<mcam::StreamBufferRet>* pvBufferReturns);

  void returnStreamBuffers(
      const std::vector<mcam::StreamBuffer>& buffers);
#endif
};
};      // namespace custom
};      // namespace mcam
#endif  // MAIN_MTKHAL_CUSTOM_DEVICE_3_X_CUSCAMERAOFFLINESESSION_H_
