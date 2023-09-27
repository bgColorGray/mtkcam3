/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <main/mtkhal/custom/device/3.x/CusCameraOfflineSession.h>
#include "MyUtils.h"

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>  // will include <log/log.h> if LOG_TAG was defined
#include <mtkcam/utils/std/ULogDef.h>

#include <memory>
#include <vector>

using mcam::IMtkcamOfflineSession;
using mcam::custom::CusCameraDeviceCallback;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
// using NSCam::v3::CameraDevice3SessionCallback;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGV(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGI_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGI(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace mcam {
namespace custom {
/******************************************************************************
 *
 ******************************************************************************/
CusCameraOfflineSession::CusCameraOfflineSession(
    const std::shared_ptr<mcam::IMtkcamOfflineSession>& session)
    : IMtkcamOfflineSession(),
      mSession(session),
      mLogLevel(0),
      mCameraDeviceCallback() {
  MY_LOGW("Not implement");
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraOfflineSession::~CusCameraOfflineSession() {
  MY_LOGW("Not implement");
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraOfflineSession::create(
    const std::shared_ptr<mcam::IMtkcamOfflineSession>& session)
    -> CusCameraOfflineSession* {
  auto pInstance = new CusCameraOfflineSession(session);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraOfflineSession  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/******************************************************************************
 *
 ******************************************************************************/

void CusCameraOfflineSession::setCallback(
    const std::shared_ptr<mcam::IMtkcamDeviceCallback>& callback) {
  MY_LOGW("Not implement");

  auto customCallback =
      std::shared_ptr<mcam::custom::CusCameraDeviceCallback>(
          CusCameraDeviceCallback::create(
              getInstanceId()));  // create CusCameraDeviceCallback

  customCallback->setCustomSessionCallback(
      callback);  // set callback in CusCameraDeviceCallback
  mCameraDeviceCallback = customCallback;
  auto pSession = std::shared_ptr<IMtkcamDeviceCallback>(this);
  mSession->setCallback(pSession);
}

/******************************************************************************
 *
 ******************************************************************************/

void CusCameraOfflineSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  mCameraDeviceCallback->processCaptureResult(results);
}

void CusCameraOfflineSession::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  if (msgs.size()) {
    mCameraDeviceCallback->notify(msgs);
  } else {
    MY_LOGE("msgs size is zero, bypass the megs");
  }
}
#if 0
mcam::BufferRequestStatus CusCameraOfflineSession::requestStreamBuffers(
    const std::vector<mcam::BufferRequest>& vBufferRequests,
    std::vector<mcam::StreamBufferRet>* pvBufferReturns) {
  MY_LOGE("Not implement");
  return mCameraDeviceCallback->requestStreamBuffers(vBufferRequests,
                                                           pvBufferReturns);
}

void CusCameraOfflineSession::returnStreamBuffers(
    const std::vector<mcam::StreamBuffer>& buffers) {
  MY_LOGE("Not implement");

  return mCameraDeviceCallback->returnStreamBuffers(buffers);
}
#endif

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraOfflineSession::close() {
  return mSession->close();
}
};  // namespace custom
};  // namespace mcam
