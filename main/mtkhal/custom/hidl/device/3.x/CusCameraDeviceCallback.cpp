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

#include "CusCameraDeviceCallback.h"
#include "MyUtils.h"

#include <TypesWrapper.h>
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

using ::android::BAD_VALUE;
using ::android::Mutex;
using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;
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

namespace NSCam {
namespace custom_dev3 {
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
    : ICameraDeviceCallback(),
      mInstanceId(InstanceId),
      mLogPrefix(std::to_string(InstanceId) + "-custom-session-callback"),
      mCustomCameraDeviceCallback(nullptr) {
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

auto CusCameraDeviceCallback::setCustomSessionCallback(
    const std::shared_ptr<CustomCameraDeviceSessionCallback> callback)
    -> ::android::status_t {
  if (callback != nullptr) {
    mCustomCameraDeviceCallback = callback;
    return OK;
  } else {
    MY_LOGE("callback is null!");
    return NO_INIT;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceCallback::processCaptureResult(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::CaptureResult>& results) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  MY_LOGE("Not implement");
  // mCustomCameraDeviceCallback->processCaptureResult(results);

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceCallback::notify(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  // MY_LOGD("+");
  mCustomCameraDeviceCallback->notify(msgs);
  // MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceCallback::processCaptureResult_3_4(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::CaptureResult>& results) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  // MY_LOGD("+");
  mCustomCameraDeviceCallback->processCaptureResult(results);
  // MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceCallback::requestStreamBuffers(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_5::BufferRequest>& bufReqs,
    requestStreamBuffers_cb _hidl_cb) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  MY_LOGE("Not implement");
  ::android::hardware::hidl_vec<
      ::android::hardware::camera::device::V3_5::StreamBufferRet>
      buffers;

  _hidl_cb(::android::hardware::camera::device::V3_5::BufferRequestStatus::
               FAILED_UNKNOWN,
           buffers);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceCallback::returnStreamBuffers(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::StreamBuffer>& buffers) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  MY_LOGE("Not implement");
  return Void();
}

};  // namespace custom_dev3
};  // namespace NSCam
