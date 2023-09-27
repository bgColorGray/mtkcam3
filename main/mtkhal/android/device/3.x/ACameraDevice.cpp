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

#include <main/mtkhal/android/device/3.x/ACameraDevice.h>
#include <main/mtkhal/android/device/3.x/ACameraDeviceSession.h>
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <memory>
#include <string>
#include <vector>

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_LOGV("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_LOGD("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_LOGI("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_LOGW("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_LOGE("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("%d[ACameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
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
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
ACameraDevice::~ACameraDevice() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
ACameraDevice::ACameraDevice(const CreationInfo& info)
    : IACameraDevice(),
      mLogLevel(0),
      mInstanceId(info.instanceId),
      mDevice(info.pDevice) {
  MY_LOGI("ctor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::create(const CreationInfo& info) -> ACameraDevice* {
  auto pInstance = new ACameraDevice(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::getResourceCost(CameraResourceCost& resourceCost) -> int {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  mcam::CameraResourceCost mtkResourceCost;
  int status = mDevice->getResourceCost(mtkResourceCost);
  convertCameraResourceCost(mtkResourceCost, resourceCost);

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::getCameraCharacteristics(
    const camera_metadata*& cameraCharacteristics) -> int {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  IMetadata mtkCameraCharacteristics;
  auto status = mDevice->getCameraCharacteristics(mtkCameraCharacteristics);
  if (status != 0) {
    MY_LOGE("cannot get CameraCharacteristics");
  } else {
    if (CC_UNLIKELY(convertMetadata(mtkCameraCharacteristics,
                                    cameraCharacteristics))) {
      MY_LOGE("convert failed");
      return -EINVAL;
    }
  }

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::setTorchMode(TorchMode mode) -> int {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  int status = mDevice->setTorchMode(static_cast<mcam::TorchMode>(mode));

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::open(
    const std::shared_ptr<IACameraDeviceCallback>& callback)
    -> std::shared_ptr<IACameraDeviceSession> {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  // create ACameraDeviceSession
  ACameraDeviceSession::CreationInfo sessionCreationInfo = {
      .instanceId = getInstanceId(),
      .pCallback = callback,
  };
  auto aSession = std::shared_ptr<ACameraDeviceSession>(
      ACameraDeviceSession::create(sessionCreationInfo));
  if (aSession == nullptr) {
    MY_LOGE("cannot create ACameraDeviceSession");
    return nullptr;
  }

  // open MtkcamDevice
  auto halSession = mDevice->open(aSession);
  if (aSession == nullptr) {
    MY_LOGE("cannot get MtkcamDeviceSession");
    return nullptr;
  }

  // open ACameraDeviceSession
  auto status = aSession->open(halSession);
  if (status != 0) {
    MY_LOGE("cannot open ACameraDeviceSession");
    aSession = nullptr;
  }

  MY_LOGD("-");
  return aSession;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::dumpState(IPrinter& printer,
                              const std::vector<std::string>& options) -> void {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  mDevice->dumpState(printer, options);

  MY_LOGD("-");
  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::getPhysicalCameraCharacteristics(
    int physicalId,
    const camera_metadata*& physicalMetadata) -> int {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  IMetadata mtkPhysicalMetadata;
  int status = mDevice->getPhysicalCameraCharacteristics(physicalId,
                                                         mtkPhysicalMetadata);
  if (CC_UNLIKELY(convertMetadata(mtkPhysicalMetadata, physicalMetadata))) {
    MY_LOGE("convert failed");
    return -EINVAL;
  }

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDevice::isStreamCombinationSupported(
    const StreamConfiguration& streams,
    bool& queryStatus) -> int {
  MY_LOGD("+");
  if (CC_UNLIKELY(mDevice == nullptr)) {
    MY_LOGE("mDevice does not exist");
  }

  mcam::StreamConfiguration mtkStreams;
  convertStreamConfiguration(streams, mtkStreams);
  int status = mDevice->isStreamCombinationSupported(mtkStreams, queryStatus);

  MY_LOGD("-");
  return status;
}

};  // namespace android
};  // namespace mcam
