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

#include <main/mtkhal/custom/provider/CusCameraProvider.h>
//
//
#include <mtkcam/utils/debug/IPrinter.h>
#include <mtkcam/utils/metadata/IVendorTagDescriptor.h>
#include <mtkcam/utils/std/CallStackLogger.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/ULog.h>
//
#include <dlfcn.h>
#include <chrono>
#include <cstdlib>
#include <future>
#include <memory>
#include <string>
#include <vector>
//

#include "../device/3.x/CusCameraDevice.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

// using mcam::CameraMetadataType;
using mcam::DeviceState;
using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;
using mcam::Status;
using mcam::TorchModeStatus;
using mcam::custom::CusCameraDevice;
// using NSCam::ICameraDeviceManager;

// using NSCam::CameraDevice3Impl;
using NSCam::IPrinter;
using NSCam::Utils::ULog::DetailsType;
using NSCam::Utils::ULog::ULogPrinter;
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
namespace custom {

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraProvider::getSafeMtkCamProvider() const
    -> std::shared_ptr<IMtkcamProvider> {
  std::lock_guard<std::mutex> lock(mMtkcamProviderLock);
  return mMtkcamProvider;
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraProvider::~CusCameraProvider() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraProvider::CusCameraProvider(const CreationInfo& info)
    : IMtkcamProvider(), mProviderCallback(nullptr), mProviderCallbackLock() {
  mMtkcamProvider = info.pProvider;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraProvider::create(const CreationInfo& info)
    -> std::shared_ptr<IMtkcamProvider> {
  auto pInstance = std::make_shared<CusCameraProvider>(info);

  if (!pInstance) {
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraProvider::setCallback(
    const std::shared_ptr<mcam::IMtkcamProviderCallback>& callback) {
  {
    std::lock_guard<std::mutex> lock(mProviderCallbackLock);

    //  cleanup the previous CusCameraProviderCallback if existed.
    if (mProviderCallback != nullptr) {
      mProviderCallback = nullptr;
    }
    //  setup CusCameraProviderCallback
    mProviderCallback = callback;
  }
  // setup MtkCamProviderCallback
  auto pCameraProvider = getSafeMtkCamProvider();
  if (pCameraProvider != 0) {
    return pCameraProvider->setCallback(shared_from_this());
  }
  return 7;  // Status::INTERNAL_ERROR
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraProvider::getVendorTags(
    std::vector<mcam::VendorTagSection>& sections) const {
  auto pCameraProvider = getSafeMtkCamProvider();
  pCameraProvider->getVendorTags(sections);
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraProvider::getCameraIdList(
    std::vector<int32_t>& rCameraDeviceIds) const {
  MY_LOGD("+");
  auto pCameraProvider = getSafeMtkCamProvider();
  return pCameraProvider->getCameraIdList(rCameraDeviceIds);
}

/******************************************************************************
 *
 ******************************************************************************/
bool CusCameraProvider::isSetTorchModeSupported() const {
  MY_LOGD("+");
  auto pCameraProvider = getSafeMtkCamProvider();
  if (pCameraProvider != 0) {
    return pCameraProvider->isSetTorchModeSupported();
  }

  return false;
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraProvider::getDeviceInterface(
    const int32_t cameraDeviceId,
    std::shared_ptr<mcam::IMtkcamDevice>& rpDevice) const {
  MY_LOGD("+");

  auto pCameraProvider = getSafeMtkCamProvider();
  if (pCameraProvider != 0) {
    // std::string dName = deviceName;
    // std::size_t foundId = dName.find_last_of("/\\");
    // int intanceId = atoi((dName.substr(foundId + 1)).c_str());

    auto res = pCameraProvider->getDeviceInterface(
        cameraDeviceId, rpDevice);  // get mtkcamDevice

    auto pCusCameraDevice =
        std::shared_ptr<mcam::custom::CusCameraDevice>(
            CusCameraDevice::create(
                rpDevice,
                cameraDeviceId));  // pass mtkcamDevice to CusCameraDevice

    // auto pCusCameraDevice =
    // mcam::custom::CusCameraDevice::create(rpDevice, intanceId); //
    // pass mtkcamDevice to CusCameraDevice
    rpDevice = pCusCameraDevice;  // return CusCameraDevice
    return res;
  }
  return 7;  // Status::INTERNAL_ERROR
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraProvider::notifyDeviceStateChange(
    mcam::DeviceState newState) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeMtkCamProvider();
  if (pCameraProvider != 0) {
    return pCameraProvider->notifyDeviceStateChange(newState);
  }
  return 7;  // Status::INTERNAL_ERROR
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraProvider::getConcurrentStreamingCameraIds(
    std::vector<std::vector<int32_t>>& cameraIds) const {
  MY_LOGD("+");
  auto pCameraProvider = getSafeMtkCamProvider();
  if (pCameraProvider != 0) {
    return pCameraProvider->getConcurrentStreamingCameraIds(cameraIds);
  }
  return 7;  // Status::INTERNAL_ERROR
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraProvider::isConcurrentStreamCombinationSupported(
    std::vector<mcam::CameraIdAndStreamCombination>& configs,
    bool& queryStatus) const {
  MY_LOGD("+");
  auto pCameraProvider = getSafeMtkCamProvider();
  if (pCameraProvider != 0) {
    return pCameraProvider->isConcurrentStreamCombinationSupported(configs,
                                                                   queryStatus);
  }
  return 7;  // Status::INTERNAL_ERROR
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraProvider::debug(std::shared_ptr<NSCam::IPrinter> printer,
                              const std::vector<std::string>& options) {
  auto pCameraProvider = getSafeMtkCamProvider();
  pCameraProvider->debug(printer, options);
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraProvider::cameraDeviceStatusChange(int32_t cameraDeviceId,
                                                 CameraDeviceStatus newStatus) {
  MY_LOGD("+");
  std::shared_ptr<mcam::IMtkcamProviderCallback> callback;
  {
    std::lock_guard<std::mutex> lock(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %" PRId32 " newStatus:%" PRIu32,
            cameraDeviceId, static_cast<uint32_t>(newStatus));
  } else {
    callback->cameraDeviceStatusChange(cameraDeviceId, newStatus);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraProvider::torchModeStatusChange(int32_t cameraDeviceId,
                                              TorchModeStatus newStatus) {
  MY_LOGD("+");
  std::shared_ptr<mcam::IMtkcamProviderCallback>
      callback;  //  <IMTKCameraProviderCallback>  callback
  {
    std::lock_guard<std::mutex> lock(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %" PRId32 " newStatus:%" PRIu32,
            cameraDeviceId, static_cast<uint32_t>(newStatus));
  } else {
    return callback->torchModeStatusChange(cameraDeviceId, newStatus);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraProvider::physicalCameraDeviceStatusChange(
    int32_t cameraDeviceId,
    int32_t physicalDeviceId,
    CameraDeviceStatus newStatus) {
  MY_LOGD("+");
  std::shared_ptr<mcam::IMtkcamProviderCallback>
      callback;  //  <IMTKCameraProviderCallback>  callback
  {
    std::lock_guard<std::mutex> lock(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %" PRId32 " newStatus:%" PRIu32,
            cameraDeviceId, static_cast<uint32_t>(newStatus));
  } else {
    return callback->physicalCameraDeviceStatusChange(
        cameraDeviceId, physicalDeviceId, newStatus);
  }
}

};  // namespace custom
};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
using mcam::custom::CusCameraProvider;

std::shared_ptr<IMtkcamProvider> getCustomProviderInstance(
    IProviderCreationParams const* params) {
  IProviderCreationParams mtkParams = {
      .providerName = params->providerName,
  };
  std::shared_ptr<IMtkcamProvider> mtkProvider =
      getMtkcamProviderInstance(&mtkParams);
  if (mtkProvider == nullptr) {
    MY_LOGE("fail to get IMtkcamProvider %s", params->providerName.c_str());
    return nullptr;
  }

  CusCameraProvider::CreationInfo createionInfo = {
      .providerName = params->providerName,
      .pProvider = mtkProvider,
  };
  auto cusProvider = CusCameraProvider::create(createionInfo);
  if (!cusProvider) {
    MY_LOGE("cannot create CusCameraProvider %s", params->providerName.c_str());
    return nullptr;
  }

  return cusProvider;
}
