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

#include <main/mtkhal/custom/device/3.x/CusCameraDeviceCallback.h>
#include "MyUtils.h"

#include <cutils/properties.h>

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <utils/String8.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// using mcam::BufferRequest;
// using mcam::BufferRequestStatus;
using mcam::CaptureResult;
using mcam::IMtkcamDeviceCallback;
using mcam::NotifyMsg;
using mcam::StreamBuffer;
// using mcam::StreamBufferRet;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;

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
CusCameraDeviceCallback::~CusCameraDeviceCallback() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraDeviceCallback::CusCameraDeviceCallback(int32_t InstanceId)
    : mInstanceId(InstanceId),
      mLogPrefix(std::to_string(InstanceId) + "-custom-session-callback"),
      mCustomCameraDeviceCallback() {
  MY_LOGI("ctor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceCallback::create(int32_t InstanceId)
    -> CusCameraDeviceCallback* {
  auto pInstance = new CusCameraDeviceCallback(InstanceId);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

void CusCameraDeviceCallback::setCustomSessionCallback(
    const std::shared_ptr<IMtkcamDeviceCallback> callback) {
  if (callback != nullptr) {
    mCustomCameraDeviceCallback = callback;
  } else {
    MY_LOGE("callback is null!");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceCallback::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  if (auto cb = mCustomCameraDeviceCallback.lock()) {
    cb->processCaptureResult(results);
  } else {
    MY_LOGW("mCustomCameraDeviceCallback does not exist, use_count=%ld",
            mCustomCameraDeviceCallback.use_count());
  }
  return;
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceCallback::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  if (auto cb = mCustomCameraDeviceCallback.lock()) {
    cb->notify(msgs);
  } else {
    MY_LOGW("mCustomCameraDeviceCallback does not exist, use_count=%ld",
            mCustomCameraDeviceCallback.use_count());
  }
  return;
}

/******************************************************************************
 *
 ******************************************************************************/
#if 0
mcam::BufferRequestStatus CusCameraDeviceCallback::requestStreamBuffers(
    const std::vector<mcam::BufferRequest>& vBufferRequests,
    std::vector<mcam::StreamBufferRet>* pvBufferReturns) {
  mcam::BufferRequestStatus status;
  if (auto cb = mCustomCameraDeviceCallback.lock()) {
    status = cb->requestStreamBuffers(vBufferRequests, pvBufferReturns);
  } else {
    MY_LOGW("mCustomCameraDeviceCallback does not exist, use_count=%ld",
            mCustomCameraDeviceCallback.use_count());
  }
  return status;
}
#endif
/******************************************************************************
 *
 ******************************************************************************/
#if 0
void CusCameraDeviceCallback::returnStreamBuffers(
    const std::vector<mcam::StreamBuffer>& buffers) {
  if (auto cb = mCustomCameraDeviceCallback.lock()) {
    status = cb->returnStreamBuffers(buffers);
  } else {
    MY_LOGW("mCustomCameraDeviceCallback does not exist, use_count=%ld",
            mCustomCameraDeviceCallback.use_count());
  }
  return;
}
#endif

};  // namespace custom
};  // namespace mcam
