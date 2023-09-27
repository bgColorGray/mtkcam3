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

#include <main/mtkhal/custom/device/3.x/CusCameraDevice.h>
#include "CusCameraDeviceSession.h"
#include "MyUtils.h"

#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <memory>
#include <string>
#include <vector>

// using ::android::FdPrinter;
// using ::android::OK;

using mcam::CameraResourceCost;
using mcam::IMtkcamDevice;
using mcam::IMtkcamDeviceSession;
using mcam::StreamConfiguration;
using mcam::TorchMode;
using mcam::custom::CusCameraDeviceCallback;
using mcam::custom::CusCameraDeviceSession;
using NSCam::IMetadata;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_LOGV("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_LOGD("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_LOGI("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_LOGW("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_LOGE("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("%d[CusCameraDevice::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
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
CusCameraDevice::~CusCameraDevice() {
  MY_LOGI("dtor");
  if (!mStaticInfo.isEmpty())
    mStaticInfo.clear();
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraDevice::CusCameraDevice(
    std::shared_ptr<mcam::IMtkcamDevice>& device,
    int32_t instanceId)
    : mcam::IMtkcamDevice(),
      mLogLevel(0),
      mInstanceId(instanceId),
      mDevice(device) {
  MY_LOGI("ctor");
  // get static metadata
  getCameraCharacteristics(mStaticInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDevice::create(std::shared_ptr<mcam::IMtkcamDevice>& device,
                             int32_t instanceId) -> CusCameraDevice* {
  auto pInstance = new CusCameraDevice(device, instanceId);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDevice::getResourceCost(
    mcam::CameraResourceCost& mtkCost) const {
  return mDevice->getResourceCost(mtkCost);
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDevice::getCameraCharacteristics(
    NSCam::IMetadata& cameraCharacteristics) const {
  return mDevice->getCameraCharacteristics(cameraCharacteristics);
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDevice::setTorchMode(mcam::TorchMode mode) {
  auto status = mDevice->setTorchMode(mode);
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<mcam::IMtkcamDeviceSession> CusCameraDevice::open(
    const std::shared_ptr<mcam::IMtkcamDeviceCallback>& callback) {
  auto customSession =
      std::shared_ptr<mcam::custom::CusCameraDeviceSession>(
          CusCameraDeviceSession::create(
              getInstanceId(), mStaticInfo));  // create CusCameraDeviceSession

  auto pMtkDeviceSession =
      mDevice->open(customSession);  // get MtkCameraDeviceSession (link to
                                     // CusCameraDeviceSession)

  customSession->setCameraDeviceSession(
      pMtkDeviceSession);  // pass MtkCameraDeviceSession to
                           // CusCameraDeviceSession

  customSession->open(
      callback);  // pass CusCameraDeviceCallback to CusCameraDeviceSession
  return customSession;
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDevice::dumpState(NSCam::IPrinter& printer,
                                const std::vector<std::string>& options) {
  return mDevice->dumpState(printer, options);
}

/******************************************************************************
 *
 ******************************************************************************/

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDevice::getPhysicalCameraCharacteristics(
    int32_t physicalId,
    NSCam::IMetadata& physicalMetadata) const {
  return mDevice->getPhysicalCameraCharacteristics(physicalId,
                                                   physicalMetadata);
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDevice::isStreamCombinationSupported(
    const mcam::StreamConfiguration& streams,
    bool& isSupported) const {
  return mDevice->isStreamCombinationSupported(streams, isSupported);
}

};  // namespace custom
};  // namespace mcam
