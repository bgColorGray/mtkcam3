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

#include "MyUtils.h"

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

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

using ::android::NAME_NOT_FOUND;
using ::android::OK;
using ::android::RWLock;
using ::android::sp;
using ::android::status_t;

namespace NSCam {
/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::isTorchModeSupportedLocked(
    const std::string& deviceName) const -> ::android::status_t {
  ::android::sp<VirtEnumDevice> pInfo = nullptr;
  std::shared_ptr<IVirtualDevice> pDevice = nullptr;
  auto status = getVirtualDeviceLocked(deviceName, &pInfo, &pDevice);
  if (OK != status) {
    return status;
  }
  //
  if (!mIsSetTorchModeSupported) {
    MY_LOGW(
        "[%s] This does not support direct operation of flashlight torch mode. "
        "The framework must open the camera device and turn the torch on "
        "through the device interface.",
        deviceName.c_str());
    return -ENOSYS;  // METHOD_NOT_SUPPORTED
  }
  //
  if (!pDevice->hasFlashUnit()) {
    MY_LOGW("[%s] this camera device does not have a flash unit",
            deviceName.c_str());
    logLocked();
    return -EOPNOTSUPP;  // OPERATION_NOT_SUPPORTED
  }
  //
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::isTorchModeControllableLocked(
    const std::string& deviceName) const -> int {
  status_t status = isTorchModeSupportedLocked(deviceName);
  if (OK != status) {
    return status;
  }

  auto const& pInfo = mVirtEnumDeviceMap.valueFor(atoi(deviceName.c_str()));
  bool isTorchUnavailable =
      (pInfo == nullptr ||
       pInfo->mTorchModeStatus == (uint32_t)ETorchModeStatus::NOT_AVAILABLE);
  if (mOpenDeviceMap.size() != 0 && isTorchUnavailable) {
    MY_LOGE(
        "[%s] Due to other camera devices (# %zu) being open, the torch cannot "
        "be controlled currently.",
        deviceName.c_str(), mOpenDeviceMap.size());
    logLocked();
    return -EUSERS;  // MAX_CAMERAS_IN_USE
  }

  /**
   *  [TODO]
   *     CAMERA_IN_USE:
   *         This camera device has been opened, so the torch cannot be
   *         controlled until it is closed.
   */

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::flushTorchModeStatusChangeCallback() -> int {
  CAM_TRACE_NAME(LOG_TAG ":flushTorchModeStatusChangeCallback");

  // (1) move callback list to local
  std::list<std::pair<uint32_t, uint32_t>> callback_list;

  RWLock::AutoWLock _l(mDataRWLock);
  //
  for (size_t i = 0; i < mVirtEnumDeviceMap.size(); i++) {
    auto const& id = mVirtEnumDeviceMap.keyAt(i);
    auto const& pInfo = mVirtEnumDeviceMap.valueAt(i);
    if (pInfo->mTorchModeStatusChanging) {
      pInfo->mTorchModeStatusChanging = false;
      callback_list.push_back({id, pInfo->mTorchModeStatus});
    }
  }
  //
  if (mCallback == nullptr) {
    MY_LOGW("cannot promote callback - torch mode status callback list:# %zu",
            callback_list.size());
    return -ENODEV;  // INTERNAL_ERROR
  }
  //
  // (2) perform local callback outside locking
  for (auto const& item : callback_list) {
    auto& cameraDeviceId = item.first;
    mCallback->torchModeStatusChange(
        cameraDeviceId,
        static_cast<mcam::TorchModeStatus>(item.second));
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::addTorchModeStatusChangeLocked(
    const std::string& deviceName,
    bool enable) -> void {
  ETorchModeStatus torchModeStatus = (!enable) ? ETorchModeStatus::AVAILABLE_OFF
                                               : ETorchModeStatus::AVAILABLE_ON;

  auto const& pInfo = mVirtEnumDeviceMap.valueFor(atoi(deviceName.c_str()));
  if (pInfo != nullptr) {
    pInfo->mTorchModeStatus = (uint32_t)torchModeStatus;
    pInfo->mTorchModeStatusChanging = true;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::setTorchModeStatusWhenOpenCloseCameraLocked(
    const std::string& deviceName,
    uint32_t newTorchModeStatus,
    bool markAsChanging) -> ::android::status_t {
  status_t status = OK;
  ::android::sp<VirtEnumDevice> pInfo = nullptr;
  std::shared_ptr<IVirtualDevice> pDevice = nullptr;
  status = getVirtualDeviceLocked(deviceName, &pInfo, &pDevice);
  if (OK != status) {
    return status;
  }
  //
  if (!mIsSetTorchModeSupported) {
    return OK;
  }
  //
  if (!pDevice->hasFlashUnit()) {
    return OK;
  }
  //
  if (pInfo->mTorchModeStatus == newTorchModeStatus) {
    MY_LOGD("the same torch mode status transition: %u", newTorchModeStatus);
    return OK;
  }
  //
  status = onEnableTorchLocked(deviceName, false);
  if (OK != status) {
    MY_LOGW("fail to turn off torch while newTorchModeStatus:%u",
            newTorchModeStatus);
    return status;
  }
  //
  //  change torch mode status
  pInfo->mTorchModeStatus = newTorchModeStatus;
  pInfo->mTorchModeStatusChanging = markAsChanging;
  MY_LOGD("%s newTorchModeStatus:%u", deviceName.c_str(), newTorchModeStatus);
  //
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
CameraDeviceManagerBase::SetTorchModeStatusCommand::SetTorchModeStatusCommand(
    CameraDeviceManagerBase* pManager,
    const std::shared_ptr<IVirtualDevice>& pVirtualDevice,
    ETorchModeStatus newTorchModeStatus)
    : ICommand(),
      mpManager(pManager),
      mpVirtualDevice(pVirtualDevice),
      mOldStatus(ETorchModeStatus::NOT_AVAILABLE),
      mNewStatus(newTorchModeStatus) {}

//  torch mode status transition
static auto const& getTorchTransSet() {
  typedef CameraDeviceManagerBase::ETorchModeStatus ETorchModeStatusT;

  //  torch mode status transition: old status -> new status
  static std::set<std::pair<ETorchModeStatusT, ETorchModeStatusT>> const
      inst = {
          // open camera
          {ETorchModeStatusT::AVAILABLE_OFF, ETorchModeStatusT::NOT_AVAILABLE},
          {ETorchModeStatusT::AVAILABLE_ON, ETorchModeStatusT::NOT_AVAILABLE},
          // close camera
          {ETorchModeStatusT::NOT_AVAILABLE, ETorchModeStatusT::AVAILABLE_OFF},
          // unchanged
          {ETorchModeStatusT::AVAILABLE_ON, ETorchModeStatusT::AVAILABLE_ON},
          {ETorchModeStatusT::AVAILABLE_OFF, ETorchModeStatusT::AVAILABLE_OFF},
          {ETorchModeStatusT::NOT_AVAILABLE, ETorchModeStatusT::NOT_AVAILABLE},
      };

  return inst;
}

/******************************************************************************
 *
 ******************************************************************************/
::android::status_t
CameraDeviceManagerBase::SetTorchModeStatusCommand::doExecute() {
  status_t status = OK;
  const char* deviceName = mpVirtualDevice->getInstanceName();
  ::android::sp<VirtEnumDevice> pInfo = nullptr;

  // [1] get current (old) torch mode status and backup it.
  status = (deviceName)
               ? mpManager->getVirtualDeviceLocked(deviceName, &pInfo, nullptr)
               : NAME_NOT_FOUND;
  if (OK != status) {
    return status;
  }
  //
  mOldStatus = static_cast<ETorchModeStatus>(pInfo->mTorchModeStatus);
  //
  // [2] find the mapping: old status -> new status
  auto find = getTorchTransSet().find({mOldStatus, mNewStatus});
  if (find == getTorchTransSet().end()) {
    MY_LOGE("unsupported torch mode status transition: %u -> %u", mOldStatus,
            mNewStatus);
    mpManager->logLocked();
    return -ENODEV;
  }
  //
  // [3] change torch mode status
  return mpManager->setTorchModeStatusWhenOpenCloseCameraLocked(
      deviceName, static_cast<uint32_t>(mNewStatus), true);
}

/******************************************************************************
 *
 ******************************************************************************/
::android::status_t
CameraDeviceManagerBase::SetTorchModeStatusCommand::undoExecute() {
  const char* deviceName = mpVirtualDevice->getInstanceName();
  if (deviceName == nullptr) {
    MY_LOGE("Cannot get device name");
    return -ENODEV;
  }
  //
  // [1] find the mapping: old status -> new status
  auto find = getTorchTransSet().find({mOldStatus, mNewStatus});
  if (find == getTorchTransSet().end()) {
    MY_LOGE("unsupported torch mode status transition: %u <- %u", mOldStatus,
            mNewStatus);
    mpManager->logLocked();
    return -ENODEV;
  }
  //
  // [2] change torch mode status
  return mpManager->setTorchModeStatusWhenOpenCloseCameraLocked(
      deviceName, static_cast<uint32_t>(mOldStatus), false);
}
};  // namespace NSCam
