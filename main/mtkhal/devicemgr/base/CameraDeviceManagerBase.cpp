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

#include <main/mtkhal/devicemgr/base/CameraDeviceManagerBase.h>

#include <memory>
#include <string>
#include <vector>

#include "../../core/device/3.x/device/MtkCameraDevice.h"
#include "MyUtils.h"

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

using ::android::BAD_VALUE;
using ::android::OK;
using ::android::RWLock;
using ::android::status_t;
using ::android::String8;
using mcam::IMtkcamDevice;
using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;
using mcam::core::MtkCameraDevice;

/******************************************************************************
 *
 ******************************************************************************/
namespace {
struct MyTraceAsyncBegin {
  String8 mName;
  int32_t mCookie;
  status_t& mrStatus;

  MyTraceAsyncBegin(String8 const& name, int32_t cookie, status_t& status)
      : mName(name), mCookie(cookie), mrStatus(status) {
    CAM_TRACE_ASYNC_BEGIN(mName.string(), mCookie);
  }

  ~MyTraceAsyncBegin() {
    if (OK != mrStatus) {
      CAM_TRACE_ASYNC_END(mName.string(), mCookie);
    }
  }
};
}  // namespace

namespace NSCam {
/******************************************************************************
 *
 ******************************************************************************/
CameraDeviceManagerBase::~CameraDeviceManagerBase() {
  for (auto& item : mVirtEnumDeviceFactoryMap) {
    if (item.second.mLibHandle) {
      ::dlclose(item.second.mLibHandle);
      item.second.mLibHandle = nullptr;
      item.second.mCreateVirtualCameraDevice = nullptr;
    }
  }
  mVirtEnumDeviceFactoryMap.clear();
  sem_destroy(&mSemPowerOn);
}

/******************************************************************************
 *
 ******************************************************************************/
CameraDeviceManagerBase::CameraDeviceManagerBase(char const* type)
    : ICameraDeviceManager(),
      IMtkcamProvider(),
      mType((type) ? type : ""),
      mCreationTimestamp(NSCam::Utils::LogTool::get()->getFormattedLogTime())
      //
      ,
      mDataRWLock(),
      mIsSetTorchModeSupported(true)  // From API Level 29, always return true
                                      // for isSetTorchModeSupported
      ,
      mPhysEnumDeviceMap(),
      mVirtEnumDeviceMap(),
      mOpenDeviceMap(),
      mCallback(nullptr)
      //
      ,
      mActiveOperation(),
      mActiveOperationCommandList() {
  char const cSupportAospHalVersion[] =
      "persist.vendor.mtkcam.aosp_hal_version";

  mSupportLegacyDevice = false;
  mSupportLatestDevice = false;
  mSecureStatus = ESecureModeStatus::SECURE_NONE;
  mIsCameraHiddenEnabled =
      ::property_get_bool("vendor.camprovider.enable_hidden", false);

  // Handle synchronization for multi-thread powerOn sensor
  sem_init(&mSemPowerOn, 0, 1);

  char overriddenVersions[PROPERTY_VALUE_MAX] = {0};
  ::property_get(cSupportAospHalVersion, overriddenVersions, nullptr);
  for (char* s = overriddenVersions; *s != 0 && ::isdigit(*s);) {
    char* end = nullptr;
    int v = static_cast<int>(::strtol(s, &end, 10));
    if (*end != 0 && *end != ',') {
      break;
    }
    s = end + 1;
    mOverriddenDeviceHalVersions.push_back(v);
  }

  MY_LOGI("\"%s\" this:%p %s:%s", mType.c_str(), this, cSupportAospHalVersion,
          overriddenVersions);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::initialize() -> bool {
  CAM_TRACE_NAME(LOG_TAG ":initialize");

  // global vendor tags should be setup before enumerating devices...
  auto pVendorTagDesc = NSCam::getVendorTagDescriptor();
  if (!pVendorTagDesc) {
    MY_LOGE("bad pVendorTagDesc");
    return false;
  }

  // loading libraries in charge of creating devices.
  auto loadDeviceFactory = [](char const* libname, char const* symbol) {
    VirtEnumDeviceFactory item;
    item.mLibHandle = ::dlopen(libname, RTLD_NOW);
    if (item.mLibHandle == nullptr) {
      char const* err_str = ::dlerror();
      CAM_ULOGME("[loadDeviceFactory] dlopen: %s error=%s", libname,
                 (err_str ? err_str : "unknown"));
      return item;
    }
    *reinterpret_cast<void**>(&item.mCreateVirtualCameraDevice) =
        ::dlsym(item.mLibHandle, symbol);
    if (item.mCreateVirtualCameraDevice == nullptr) {
      char const* err_str = ::dlerror();
      CAM_ULOGME("[loadDeviceFactory] dlsym: %s error=%s", symbol,
                 (err_str ? err_str : "unknown"));
      ::dlclose(item.mLibHandle);
      item.mLibHandle = nullptr;
      return item;
    }
    return item;
  };
  mVirtEnumDeviceFactoryMap[3] = loadDeviceFactory(
      "libmtkcam_hal_core_device.so", "createVirtualCameraDevice");

  // enumerating devices...
  status_t status = OK;
  MY_LOGI("+");
  RWLock::AutoWLock _l(mDataRWLock);
  { status = enumerateDevicesLocked(); }
  MY_LOGI("-");
  return (OK == status);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::debug(std::shared_ptr<IPrinter> printer,
                                    const std::vector<std::string>& options
                                        __unused) -> void {
  // if (0 == options.size())
  {
    //// Device Manager
    printer->printFormatLine("## Camera device manager (since %s) ##",
                             mCreationTimestamp.c_str());
    ::android::Vector<::android::String8> lines;
    {
      RWLock::AutoRLock _l(mDataRWLock);
      logLocked(&lines);
    }
    for (size_t i = 0; i < lines.size(); i++) {
      printer->printFormatLine("  %s", lines[i].c_str());
    }
    lines.clear();
  }

  //// Invoke debugging framework.
  if (auto pDbgMgr = IDebuggeeManager::get()) {
    pDbgMgr->debug(printer->getFDPrinter(), options);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::setCallback(
    const std::shared_ptr<IMtkcamProviderCallback>& callback)
    -> ::android::status_t {
  RWLock::AutoWLock _l(mDataRWLock);
  //
  mCallback = callback;
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::getCameraIdList(
    std::vector<int32_t>& rCameraDeviceIds) const -> int {
  for (size_t i = 0; i < mVirtEnumDeviceMap.size(); i++) {
    auto& pVirtEnumDevice = mVirtEnumDeviceMap.valueAt(i);
    if (!isHiddenCamera(pVirtEnumDevice->mInstanceId)) {
      auto& pVirtDevice = pVirtEnumDevice->mVirtDevice;
      rCameraDeviceIds.push_back(pVirtDevice->getVirtualInstanceId());
    }
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::getVendorTags(
    std::vector<mcam::VendorTagSection>& sections __unused) const
    -> void {
  MY_LOGW("Not implement");
}

/******************************************************************************
 *
 ******************************************************************************/
// auto CameraDeviceManagerBase::getDeviceInfoList(
//    std::vector<mcam::DeviceInfo>& rDeviceInfos) -> int {
//  RWLock::AutoRLock _l(mDataRWLock);
//  //
//  rDeviceInfos.reserve(mVirtEnumDeviceMap.size());
//  for (size_t i = 0; i < mVirtEnumDeviceMap.size(); i++) {
//    auto& pVirtEnumDevice = mVirtEnumDeviceMap.valueAt(i);
//    if (!isHiddenCamera(pVirtEnumDevice->mInstanceId)) {
//      auto const& name = mVirtEnumDeviceMap.keyAt(i);
//      auto& pVirtDevice = pVirtEnumDevice->mVirtDevice;
//      auto& virtDeviceInfo = pVirtDevice->getDeviceInfo();
//      mcam::DeviceInfo info = {
//        .instanceId = pVirtDevice->getInstanceId(),
//        .virtualInstanceId = pVirtDevice->getVirtualInstanceId(),
//        .hasFlashUnit = pVirtDevice->hasFlashUnit(),
//        .isHidden = pVirtDevice->isHidden(),
//        .facing = pVirtDevice->getFacing(),
//        .type = pVirtDevice->getType(),
//      };
//      rDeviceInfos.push_back(info);
//    } else {
//      MY_LOGD("Camera %d is hidden", pVirtEnumDevice->mInstanceId);
//    }
//  }
//  //
//  return OK;
//}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::getDeviceInterface(
    const int32_t cameraDeviceId,
    std::shared_ptr<IMtkcamDevice>& rpDevice) const -> int {
  RWLock::AutoRLock _l(mDataRWLock);
  //
  auto const& pInfo = mVirtEnumDeviceMap.valueFor(cameraDeviceId);
  if (pInfo == nullptr) {
    MY_LOGE("[%d] this device id is unknown", cameraDeviceId);
    logLocked();
    return BAD_VALUE;
  }
  //
  auto const& pDevice = pInfo->mVirtDevice;
  if (pDevice == nullptr) {
    MY_LOGE("[%d] bad virtual device", cameraDeviceId);
    logLocked();
    return BAD_VALUE;
  }
  //
  rpDevice = static_pointer_cast<IMtkcamDevice>(
      static_pointer_cast<MtkCameraDevice>(pDevice));
  if (rpDevice == nullptr) {
    MY_LOGE("[%d] cannot get IMtkcamDevice", cameraDeviceId);
    logLocked();
    return BAD_VALUE;
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::getType() const -> const std::string& {
  return mType;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::isSetTorchModeSupported() const -> bool {
  RWLock::AutoRLock _l(mDataRWLock);
  //
  return mIsSetTorchModeSupported;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::notifyDeviceStateChange(
    mcam::DeviceState newState) -> int {
  int status = OK;
  MY_LOGW("Not implement, newState=%" PRIu64 "", newState);
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::setTorchMode(const std::string& deviceName,
                                           uint32_t mode) -> int {
  CAM_TRACE_NAME(
      String8::format("%s:" LOG_TAG ":setTorchMode", deviceName.c_str())
          .string());

  int status = 0;
  std::shared_ptr<IVirtualDevice> pVirtualDevice = nullptr;
  //
  {
    RWLock::AutoRLock _l(mDataRWLock);
    status = getVirtualDeviceLocked(deviceName, nullptr, &pVirtualDevice);
    if (OK != status) {
      return status;
    }
  }
  //
  status = mActiveOperation.lock(pVirtualDevice, ActiveOperation::SET_TORCH);
  if (OK != status) {
    return status;
  }
  //
  // [1] set torch mode
  do {
    RWLock::AutoWLock _l(mDataRWLock);
    //
    status = isTorchModeControllableLocked(deviceName);
    if (OK != status) {
      break;
    }
    //
    status = onEnableTorchLocked(deviceName, mode);
    if (OK != status) {
      break;
    }
    //
    addTorchModeStatusChangeLocked(deviceName, mode);
  } while (0);
  //
  mActiveOperation.unlock(pVirtualDevice, ActiveOperation::SET_TORCH);
  pVirtualDevice = nullptr;
  if (OK != status) {
    return status;
  }
  //
  // [2] flush torch callback
  return flushTorchModeStatusChangeCallback();
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::getConcurrentStreamingCameraIds(
    std::vector<std::vector<int32_t>>& cameraIds) const -> int {
  MY_LOGW("Not implement, return OK, cameraIds.size=%zu", cameraIds.size());
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::isConcurrentStreamCombinationSupported(
    std::vector<mcam::CameraIdAndStreamCombination>& configs,
    bool& queryStatus) const -> int {
  MY_LOGW("Not implement, return -ENOSYS, configs.size=%zu, queryStatus=%d",
          configs.size(), queryStatus);
  return -ENOSYS;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::startOpenDevice(const std::string& deviceName)
    -> ::android::status_t {
  status_t status = OK;
  waitForPreviousPowerOnDone();
  std::shared_ptr<IVirtualDevice> pVirtualDevice = nullptr;
  //
  {
    RWLock::AutoRLock _l(mDataRWLock);
    status = getVirtualDeviceLocked(deviceName, nullptr, &pVirtualDevice);
    if (OK != status) {
      return status;
    }
  }
  //
  // CAM_TRACE_ASYNC_BEGIN
  MyTraceAsyncBegin _lAsyncTrace(
      String8::format("%s:" LOG_TAG ":OpenDevice",
                      pVirtualDevice->getInstanceName()),
      pVirtualDevice->getInstanceId(), status);
  CAM_TRACE_NAME(LOG_TAG ":startOpenDevice");
  //
  //  save the caller's information, which will be validated in future.
  status = mActiveOperation.lock(pVirtualDevice, ActiveOperation::OPEN);
  if (OK != status) {
    return status;
  }
  //
  do {
    RWLock::AutoWLock _l(mDataRWLock);
    //
    size_t const nOriginalOpenNum = mOpenDeviceMap.size();
    //
    MY_LOGD(
        "+ %s mActiveOperationCommandList.size:%zu mOpenDeviceMap.size:%zu "
        "mPhysEnumDeviceMap.size:%zu mVirtEnumDeviceMap.size:%zu",
        pVirtualDevice->getInstanceName(), mActiveOperationCommandList.size(),
        mOpenDeviceMap.size(), mPhysEnumDeviceMap.size(),
        mVirtEnumDeviceMap.size());
    //
    //  [1] check to see whether it's ready to open.
    status = validateOpenLocked(pVirtualDevice);
    if (OK != status) {
      break;
    }
    //
    //  [2] attach opened device
    status = attachOpenDeviceLocked(pVirtualDevice);
    if (OK != status) {
      break;
    }
    //
    //  [3] lock & turn off torch when first camera opened
    //  Note: torch must be turned off before opening camera...
    if (nOriginalOpenNum == 0 && mOpenDeviceMap.size() != 0) {
      for (size_t i = 0; i < mVirtEnumDeviceMap.size(); i++) {
        auto const& pInfo = mVirtEnumDeviceMap.valueAt(i);
        if (pInfo->mVirtDevice->getFacing() == pVirtualDevice->getFacing()) {
          ::android::sp<ICommand> pCommand = new SetTorchModeStatusCommand(
              this, pInfo->mVirtDevice, ETorchModeStatus::NOT_AVAILABLE);
          if (OK == pCommand->doExecute()) {
            mActiveOperationCommandList.push_back(pCommand);
          }
        }
      }
    }
  } while (0);
  //
  if (OK != status) {
    // get locked only when this function returns OK.
    mActiveOperation.unlock(pVirtualDevice, ActiveOperation::OPEN);
    MY_LOGE("%s status:%s(%d)", pVirtualDevice->getInstanceName(),
            ::strerror(-status), -status);
  }
  //
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::finishOpenDevice(const std::string& deviceName,
                                               bool cancel)
    -> ::android::status_t {
  status_t status = OK;
  std::shared_ptr<IVirtualDevice> pVirtualDevice = nullptr;
  //
  {
    RWLock::AutoRLock _l(mDataRWLock);
    status = getVirtualDeviceLocked(deviceName, nullptr, &pVirtualDevice);
    if (OK != status) {
      return status;
    }
  }
  //
  {
    CAM_TRACE_NAME(LOG_TAG ":finishOpenDevice");

    bool needToFlushTorchCallback = (!cancel);
    //
    //  check to see whether this is called by the same device and operation.
    status = mActiveOperation.validate(pVirtualDevice, ActiveOperation::OPEN);
    if (OK != status) {
      return status;
    }
    //
    {
      RWLock::AutoWLock _l(mDataRWLock);
      //
      //  restore all states when the operation is cancelled.
      if (cancel) {
        //  detach opened device
        updatePowerOnDone();
        detachOpenDeviceLocked(pVirtualDevice);
        //  undo commands
        for (auto const& pCommand : mActiveOperationCommandList) {
          pCommand->undoExecute();
        }
      }

      //  reset everything
      mActiveOperationCommandList.clear();
    }
    //
    if (OK == status) {
      // unlock only for the right owner.
      mActiveOperation.unlock(pVirtualDevice, ActiveOperation::OPEN);
    }
    //
    //  flush torch callback
    if (needToFlushTorchCallback) {
      flushTorchModeStatusChangeCallback();
    }
  }
  //
  if (OK == status) {
    CAM_TRACE_ASYNC_END(String8::format("%s:" LOG_TAG ":OpenDevice",
                                        pVirtualDevice->getInstanceName())
                            .string(),
                        pVirtualDevice->getInstanceId());
  }
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::startCloseDevice(const std::string& deviceName)
    -> ::android::status_t {
  status_t status = OK;
  std::shared_ptr<IVirtualDevice> pVirtualDevice = nullptr;
  //
  {
    RWLock::AutoRLock _l(mDataRWLock);
    status = getVirtualDeviceLocked(deviceName, nullptr, &pVirtualDevice);
    if (OK != status) {
      return status;
    }
  }
  //
  // CAM_TRACE_ASYNC_BEGIN
  MyTraceAsyncBegin _lAsyncTrace(
      String8::format("%s:" LOG_TAG ":CloseDevice",
                      pVirtualDevice->getInstanceName()),
      pVirtualDevice->getInstanceId(), status);
  CAM_TRACE_NAME(LOG_TAG ":startCloseDevice");
  //
  //  save the caller's information, which will be validated in future.
  status = mActiveOperation.lock(pVirtualDevice, ActiveOperation::CLOSE);
  if (OK != status) {
    return status;
  }
  //
#if 0  // Coverity 2357530 Logically dead code
    {
        // RWLock::AutoWLock _l(mDataRWLock);
        //  do something necessary here...
    }
    //
    if  ( OK != status ) {
        // get locked only when this function returns OK.
        mActiveOperation.unlock(pVirtualDevice, ActiveOperation::CLOSE);
        MY_LOGE("%s status:%s(%d)", pVirtualDevice->getInstanceName(),
                ::strerror(-status), -status);
    }
#endif
  //
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::finishCloseDevice(const std::string& deviceName)
    -> ::android::status_t {
  status_t status = OK;
  std::shared_ptr<IVirtualDevice> pVirtualDevice = nullptr;
  //
  {
    RWLock::AutoRLock _l(mDataRWLock);
    status = getVirtualDeviceLocked(deviceName, nullptr, &pVirtualDevice);
    if (OK != status) {
      return status;
    }
  }
  //
  {
    CAM_TRACE_NAME(LOG_TAG ":finishCloseDevice");
    //
    //  check to see whether this is called by the same device and operation.
    status = mActiveOperation.validate(pVirtualDevice, ActiveOperation::CLOSE);
    if (OK != status) {
      return status;
    }
    //
    {
      RWLock::AutoWLock _l(mDataRWLock);
      //
      //  [1] detach opened device
      detachOpenDeviceLocked(pVirtualDevice);
      //
      //  [2] unlock torch when all cameras closed
      if (mOpenDeviceMap.size() == 0) {
        for (size_t i = 0; i < mVirtEnumDeviceMap.size(); i++) {
          auto const& pInfo = mVirtEnumDeviceMap.valueAt(i);
          if (pInfo->mVirtDevice->getFacing() == pVirtualDevice->getFacing()) {
            ::android::sp<ICommand> pCommand = new SetTorchModeStatusCommand(
                this, pInfo->mVirtDevice, ETorchModeStatus::AVAILABLE_OFF);
            pCommand->doExecute();
          }
        }
      }
      //
      //  [3] reset everything
      mActiveOperationCommandList.clear();
    }
    //
    if (OK == status) {
      // unlock only for the right owner.
      mActiveOperation.unlock(pVirtualDevice, ActiveOperation::CLOSE);
    }
    //
    //  flush torch callback
    flushTorchModeStatusChangeCallback();
  }
  //
  if (OK == status) {
    CAM_TRACE_ASYNC_END(String8::format("%s:" LOG_TAG ":CloseDevice",
                                        pVirtualDevice->getInstanceName())
                            .string(),
                        pVirtualDevice->getInstanceId());
  }
  return status;
}
/******************************************************************************
 *
 ******************************************************************************/
auto CameraDeviceManagerBase::getOpenedCameraNum() -> uint32_t {
  return mOpenDeviceMap.size();
}
};  // namespace NSCam
