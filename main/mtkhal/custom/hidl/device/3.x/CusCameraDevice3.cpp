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

#include "CusCameraDevice3.h"
#include "CusCameraDeviceCallback.h"
#include "CusCameraDeviceSession.h"
#include "MyUtils.h"

#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <memory>
#include <string>
#include <vector>

using ::android::FdPrinter;
using ::android::OK;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)                                               \
  CAM_LOGV("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGD(fmt, arg...)                                               \
  CAM_LOGD("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGI(fmt, arg...)                                               \
  CAM_LOGI("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGW(fmt, arg...)                                               \
  CAM_LOGW("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGE(fmt, arg...)                                               \
  CAM_LOGE("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGA(fmt, arg...)                                               \
  CAM_LOGA("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGF(fmt, arg...)                                               \
  CAM_LOGF("%d[CusCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
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
CusCameraDevice3::~CusCameraDevice3() {
  MY_LOGI("dtor");
  if (mStaticInfo)
    free_camera_metadata(mStaticInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraDevice3::CusCameraDevice3(
    ::android::sp<::android::hardware::camera::device::V3_6::ICameraDevice>
        device,
    int32_t instanceId)
    : ICameraDevice(), mLogLevel(0), mInstanceId(instanceId), mDevice(device) {
  MY_LOGI("ctor");
  // get static metadata
  CameraMetadata staticInfo;
  auto result = mDevice->getCameraCharacteristics(
      [&](auto status, const auto& cameraCharacteristics) {
        staticInfo = cameraCharacteristics;
      });
  camera_metadata* staticMetadata =
      reinterpret_cast<camera_metadata*>(staticInfo.data());
  mStaticInfo = clone_camera_metadata(staticMetadata);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDevice3::create(
    ::android::sp<::android::hardware::camera::device::V3_6::ICameraDevice>
        device,
    int32_t instanceId)
    -> ::android::hardware::camera::device::V3_2::ICameraDevice* {
  auto pInstance = new CusCameraDevice3(device, instanceId);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::getResourceCost(getResourceCost_cb _hidl_cb) {
  // CameraResourceCost hidlCost;
  // NSCam::v3::CameraResourceCost mtkCost;

  // int status = mDevice->getResourceCost(mtkCost);
  auto status =
      mDevice->getResourceCost([&](auto status, const auto& resourceCost) {
        MY_LOGD("getResourceCost returns status:%d , Resource cost is %d",
                (int)status, resourceCost.resourceCost);
        for (const auto& name : resourceCost.conflictingDevices) {
          MY_LOGD("    Conflicting device: %s", name.c_str());
        }
        _hidl_cb(status, resourceCost);
      });
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::getCameraCharacteristics(
    getCameraCharacteristics_cb _hidl_cb) {
  auto result = mDevice->getCameraCharacteristics(
      [&](auto status, const auto& cameraCharacteristics) {
        _hidl_cb(status, cameraCharacteristics);
      });

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status> CusCameraDevice3::setTorchMode(TorchMode mode) {
  auto status = mDevice->setTorchMode(mode);
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::open(
    const ::android::sp<
        ::android::hardware::camera::device::V3_2::ICameraDeviceCallback>&
        callback,
    open_cb _hidl_cb) {
  MY_LOGD("intance id: %d", getInstanceId());
  ::android::sp<CusCameraDeviceCallback> cusCallback =
      CusCameraDeviceCallback::create(getInstanceId());

  auto ret =
      mDevice->open(cusCallback, [&](auto status, const auto& hidlSession) {
        MY_LOGI("device::open returns status:%d", (int)status);
        auto customSession = ::android::sp<CusCameraDeviceSession>(
            CusCameraDeviceSession::create(
                ::android::hardware::camera::device::V3_6::
                    ICameraDeviceSession::castFrom(hidlSession),
                getInstanceId(), mStaticInfo));
        cusCallback->setCustomSessionCallback(
            customSession->createCustomSessionCallbacks());

        auto result =
            customSession->open(::android::hardware::camera::device::V3_5::
                                    ICameraDeviceCallback::castFrom(callback));
        _hidl_cb(status, customSession);
      });

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::dumpState(
    const ::android::hardware::hidl_handle& handle) {
  // ::android::hardware::hidl_vec<::android::hardware::hidl_string> options;
  // debug(handle, options);

  auto status = mDevice->dumpState(handle);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::debug(
    const ::android::hardware::hidl_handle& handle __unused,
    const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
        options __unused) {
  auto status = mDevice->debug(handle, options);

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::getPhysicalCameraCharacteristics(
    const ::android::hardware::hidl_string& physicalCameraId,
    getPhysicalCameraCharacteristics_cb _hidl_cb) {
  auto ret = mDevice->getPhysicalCameraCharacteristics(
      physicalCameraId, [&](auto status, const auto& cameraCharacteristics) {
        _hidl_cb(status, cameraCharacteristics);
      });
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDevice3::isStreamCombinationSupported(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
        streams,
    isStreamCombinationSupported_cb _hidl_cb) {
  auto ret = mDevice->isStreamCombinationSupported(
      streams,
      [&](auto status, auto combStatus) { _hidl_cb(status, combStatus); });
  return ret;
}

};  // namespace custom_dev3
};  // namespace NSCam
