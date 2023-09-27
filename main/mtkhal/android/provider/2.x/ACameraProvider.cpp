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
 * MediaTek Inc. (C) 2021. All rights reserved.
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

#include <cutils/properties.h>
#include <main/mtkhal/android/device/3.x/ACameraDevice.h>
#include <main/mtkhal/android/provider/2.x/ACameraProvider.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam/utils/std/ULog.h>

#include <dlfcn.h>
#include <memory>
#include <string>
#include <vector>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;

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
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
ACameraProvider::~ACameraProvider() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
ACameraProvider::ACameraProvider(const CreationInfo& info)
    : IACameraProvider(),
      IMtkcamProviderCallback(),
      mName(info.providerName),
      mProvider(info.pProvider),
      mCallback(nullptr),
      mCallbackLock() {
  MY_LOGI("ctor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::create(const CreationInfo& info)
    -> std::shared_ptr<IACameraProvider> {
  auto pInstance = std::make_shared<ACameraProvider>(info);

  if (!pInstance) {
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::setCallback(
    const std::shared_ptr<IACameraProviderCallback>& callback) -> int {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  {
    std::lock_guard<std::mutex> guard(mCallbackLock);
    mCallback = callback;
  }
  auto status = mProvider->setCallback(shared_from_this());

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::getCameraIdList(
    std::vector<int32_t>& rCameraDeviceIds) const -> int {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  auto status = mProvider->getCameraIdList(rCameraDeviceIds);

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::isSetTorchModeSupported() const -> bool {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  return mProvider->isSetTorchModeSupported();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::getDeviceInterface(
    const int32_t cameraDeviceId,
    std::shared_ptr<IACameraDevice>& rpDevice) -> int {
  MY_LOGD("+");
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  ACameraDevice::CreationInfo creationInfo;
  creationInfo.instanceId = cameraDeviceId;
  auto status = mProvider->getDeviceInterface(creationInfo.instanceId,
                                              creationInfo.pDevice);
  if (creationInfo.pDevice == nullptr || status != 0) {
    MY_LOGE("cannot get MtkcamDevice");
  }

  rpDevice =
      std::shared_ptr<IACameraDevice>(ACameraDevice::create(creationInfo));
  if (rpDevice == nullptr) {
    MY_LOGE("cannot create ACameraDevice");
  }

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::notifyDeviceStateChange(DeviceState newState) -> int {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  auto status = mProvider->notifyDeviceStateChange(
      static_cast<mcam::DeviceState>(newState));

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::getConcurrentStreamingCameraIds(
    std::vector<std::vector<int32_t>>& cameraIds) const -> int {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  auto status = mProvider->getConcurrentStreamingCameraIds(cameraIds);

  MY_LOGW("Not implement, return 0");
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::isConcurrentStreamCombinationSupported(
    const std::vector<CameraIdAndStreamCombination>& configs,
    bool& queryStatus) -> int {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }

  std::vector<mcam::CameraIdAndStreamCombination> mtkConfigs;
  convertCameraIdAndStreamCombinations(configs, mtkConfigs);
  auto status = mProvider->isConcurrentStreamCombinationSupported(mtkConfigs,
                                                                  queryStatus);

  MY_LOGW("Not implement, return -ENOSYS");
  return -ENOSYS;
}

/******************************************************************************
 *  Warning: This should be removed after custom zone moved down.
 ******************************************************************************/
auto ACameraProvider::debug(std::shared_ptr<IPrinter> printer,
                            const std::vector<std::string>& options) -> void {
  if (CC_UNLIKELY(mProvider == nullptr)) {
    MY_LOGE("mProvider does not exist");
  }
  MY_LOGD("mProvider->debug(+)");
  mProvider->debug(printer, options);
  MY_LOGD("mProvider->debug(-)");
  // Unit Test (new version)
  {
    bool bIsMtkHalTest = false;
    for ( auto& s : options ) {
      if (s.find("--mtk_hal_test") != std::string::npos) {
        bIsMtkHalTest = true;
        break;
      }
    }
    //
    if (bIsMtkHalTest) {
      MY_LOGI("enter mtk_hal_test flow");
#define CAM_PATH_MTKHAL_TEST "libmtkcam_hal_ut.so"
#define FUNC_NAME_TEST "runMtkHalTest"
      //
      auto pUtLib = ::dlopen(CAM_PATH_MTKHAL_TEST, RTLD_NOW);
      if (!pUtLib) {
        char const* err_str = ::dlerror();
        MY_LOGE("dlopen: %s error=%s", CAM_PATH_MTKHAL_TEST,
                (err_str ? err_str : "unknown"));
        return;
      }
      typedef void (*pfnEntry_T)(const std::shared_ptr<IPrinter> printer,
                const std::vector<std::string>& options,
                const std::vector<int32_t>& cameraIdList);
      pfnEntry_T pfnEntry;
      pfnEntry = (pfnEntry_T)::dlsym(pUtLib, FUNC_NAME_TEST);
      if (!pfnEntry) {
        char const* err_str = ::dlerror();
        MY_LOGE("dlsym: %s error=%s getModuleLib:%p", FUNC_NAME_TEST,
                (err_str ? err_str : "unknown"), pUtLib);
      } else {
        std::vector<int32_t> cameraIdList;
        auto status = mProvider->getCameraIdList(cameraIdList);
        pfnEntry(printer, options, cameraIdList);
      }
      if (pUtLib) {
        dlclose(pUtLib);
      }
    } else {
      MY_LOGI("NOT mtk_hal_test flow!");
    }
  }
  // Unit Test (old version)
  {
    bool bIsMtkHalUt = false;
    for ( auto& s : options ) {
      if (s.find("--mtk_hal_ut") != std::string::npos) {
        bIsMtkHalUt = true;
        break;
      }
    }
    //
    if (bIsMtkHalUt) {
      MY_LOGI("enter mtk_hal_ut flow");
#define CAM_PATH_MTKHAL_UT "libmtkcam_hal_ut.so"
#define FUNC_NAME_UT "runMtkHalUt"
      //
      auto pUtLib = ::dlopen(CAM_PATH_MTKHAL_UT, RTLD_NOW);
      if (!pUtLib) {
        char const* err_str = ::dlerror();
        MY_LOGE("dlopen: %s error=%s", CAM_PATH_MTKHAL_UT,
                (err_str ? err_str : "unknown"));
        return;
      }
      typedef void (*pfnEntry_T)(const std::shared_ptr<IPrinter> printer,
                const std::vector<std::string>& options,
                const std::vector<int32_t>& cameraIdList);
      pfnEntry_T pfnEntry;
      pfnEntry = (pfnEntry_T)::dlsym(pUtLib, FUNC_NAME_UT);
      if (!pfnEntry) {
        char const* err_str = ::dlerror();
        MY_LOGE("dlsym: %s error=%s getModuleLib:%p", FUNC_NAME_UT,
                (err_str ? err_str : "unknown"), pUtLib);
      } else {
        std::vector<int32_t> cameraIdList;
        auto status = mProvider->getCameraIdList(cameraIdList);
        pfnEntry(printer, options, cameraIdList);
      }
      if (pUtLib) {
        dlclose(pUtLib);
      }
    } else {
      MY_LOGI("NOT mtk_hal_ut flow!");
    }
  }
  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::cameraDeviceStatusChange(
    int32_t cameraDeviceId,
    mcam::CameraDeviceStatus newStatus) -> void {
  {
    std::lock_guard<std::mutex> guard(mCallbackLock);
    if (CC_UNLIKELY(mCallback == nullptr)) {
      MY_LOGE("mCallback does not exist");
    }
    mCallback->cameraDeviceStatusChange(
        cameraDeviceId, static_cast<CameraDeviceStatus>(newStatus));
  }

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::torchModeStatusChange(
    int32_t cameraDeviceId,
    mcam::TorchModeStatus newStatus) -> void {
  {
    std::lock_guard<std::mutex> guard(mCallbackLock);
    if (CC_UNLIKELY(mCallback == nullptr)) {
      MY_LOGE("mCallback does not exist");
    }
    mCallback->torchModeStatusChange(cameraDeviceId,
                                     static_cast<TorchModeStatus>(newStatus));
  }

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraProvider::physicalCameraDeviceStatusChange(
    int32_t cameraDeviceId,
    int32_t physicalDeviceId,
    mcam::CameraDeviceStatus newStatus) -> void {
  {
    std::lock_guard<std::mutex> guard(mCallbackLock);
    if (CC_UNLIKELY(mCallback == nullptr)) {
      MY_LOGE("mCallback does not exist");
    }
    mCallback->physicalCameraDeviceStatusChange(
        cameraDeviceId, physicalDeviceId,
        static_cast<CameraDeviceStatus>(newStatus));
  }

  return;
}

};  // namespace android
};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
using mcam::android::IACameraProvider;
using mcam::android::ACameraProvider;

std::shared_ptr<IACameraProvider> getIACameraProviderInstance(
    IACameraProviderParams const* params) {
  IProviderCreationParams mtkParams = {
      .providerName = params->providerName,
  };
  std::shared_ptr<IMtkcamProvider> mtkProvider = nullptr;
  int32_t isSupportMTKHALCustom =
      property_get_int32("vendor.debug.camera.mtkhal.custom", 0);
  if (isSupportMTKHALCustom) {
    MY_LOGI("get custom provider, isSupportMTKHALCustom=%d",
            isSupportMTKHALCustom);
    mtkProvider = getCustomProviderInstance(&mtkParams);
  } else {
    MY_LOGI("get mtkcam provider, isSupportMTKHALCustom=%d",
            isSupportMTKHALCustom);
    mtkProvider = getMtkcamProviderInstance(&mtkParams);
  }
  if (!mtkProvider) {
    MY_LOGE("fail to get IMtkcamProvider %s", params->providerName.c_str());
    return nullptr;
  }

  ACameraProvider::CreationInfo createionInfo = {
      .providerName = params->providerName,
      .pProvider = mtkProvider,
  };
  auto aProvider = ACameraProvider::create(createionInfo);
  if (!aProvider) {
    MY_LOGE("cannot create ACameraProvider %s", params->providerName.c_str());
    return nullptr;
  }

  return aProvider;
}
