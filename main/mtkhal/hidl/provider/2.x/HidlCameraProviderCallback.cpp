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

#include <HidlCameraDevice.h>
#include <main/mtkhal/hidl/provider/2.x/HidlCameraProviderCallback.h>
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>

#include <string>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::TorchModeStatus;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
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
namespace hidl {

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraProviderCallback::~HidlCameraProviderCallback() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraProviderCallback::HidlCameraProviderCallback(const CreationInfo& info)
    : IACameraProviderCallback(), mProviderCallback(info.pCallback) {
  MY_LOGI("ctor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraProviderCallback::create(const CreationInfo& info)
    -> HidlCameraProviderCallback* {
  auto pInstance = new HidlCameraProviderCallback(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
void HidlCameraProviderCallback::cameraDeviceStatusChange(
    int32_t cameraDeviceId,
    mcam::android::CameraDeviceStatus newStatus) {
  ::android::sp<ICameraProviderCallback> callback;
  {
    std::lock_guard<std::mutex> guard(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  auto deviceName = mapToHidlDeviceName(cameraDeviceId).c_str();
  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %s newStatus:%u", deviceName, newStatus);
    return;
  }

  auto const status = static_cast<CameraDeviceStatus>(newStatus);
  auto ret = callback->cameraDeviceStatusChange(deviceName, status);
  if (!ret.isOk()) {
    MY_LOGE(
        "Transaction error in "
        "ICameraProviderCallback::cameraDeviceStatusChange: %s",
        ret.description().c_str());
  }
  MY_LOGI("%s CameraDeviceStatus:%u", deviceName, status);
}

/******************************************************************************
 *
 ******************************************************************************/
void HidlCameraProviderCallback::torchModeStatusChange(
    int32_t cameraDeviceId,
    mcam::android::TorchModeStatus newStatus) {
  ::android::sp<ICameraProviderCallback> callback;
  {
    std::lock_guard<std::mutex> guard(mProviderCallbackLock);
    callback = mProviderCallback;
  }
  std::string strDeviceName = mapToHidlDeviceName(cameraDeviceId);
  auto deviceName = strDeviceName.c_str();
  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %s newStatus:%u", deviceName, newStatus);
    return;
  }

  TorchModeStatus const status = (TorchModeStatus)newStatus;
  auto ret = callback->torchModeStatusChange(deviceName, status);
  if (!ret.isOk()) {
    MY_LOGE(
        "Transaction error in ICameraProviderCallback::torchModeStatusChange: "
        "%s",
        ret.description().c_str());
  }
  MY_LOGI("%s TorchModeStatus:%u", deviceName, status);
}

/******************************************************************************
 *
 ******************************************************************************/
void HidlCameraProviderCallback::physicalCameraDeviceStatusChange(
    int32_t cameraDeviceId,
    int32_t physicalDeviceId,
    mcam::android::CameraDeviceStatus newStatus) {
  ::android::sp<ICameraProviderCallback> callback;
  {
    std::lock_guard<std::mutex> guard(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  auto deviceName = mapToHidlDeviceName(cameraDeviceId).c_str();
  auto physicalDeviceName = std::to_string(physicalDeviceId).c_str();
  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %s %s newStatus:%u", deviceName,
            physicalDeviceName, newStatus);
    return;
  }

  auto const status = static_cast<CameraDeviceStatus>(newStatus);
  ::android::sp<
      ::android::hardware::camera::provider::V2_6::ICameraProviderCallback>
      callback_2_6 = ::android::hardware::camera::provider::V2_6::
          ICameraProviderCallback::castFrom(callback);
  auto ret = callback_2_6->physicalCameraDeviceStatusChange(
      deviceName, physicalDeviceName, status);
  if (!ret.isOk()) {
    MY_LOGE(
        "Transaction error in "
        "ICameraProviderCallback::physicalCameraDeviceStatusChange: %s",
        ret.description().c_str());
  }
  MY_LOGI("%s %s CameraDeviceStatus:%u", deviceName, physicalDeviceName,
          status);
}

};  // namespace hidl
};  // namespace mcam
