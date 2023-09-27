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

#include <mtkcam3/main/mtkhal/core/common/1.x/types.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h>
#include <mtkcam/utils/std/ULog.h>
//
#include <memory>
#include <mutex>
#include <string>
#include <vector>
//
#include "../../../devicemgr/depend/CameraDeviceManagerImpl.h"
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_ULOGM_ASSERT(0, "[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_ULOGM_FATAL("[MtkCameraProvider::%s] " fmt, __FUNCTION__, ##arg)
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
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
class MtkCameraProvider : public IMtkcamProvider {
 public:
  explicit MtkCameraProvider(std::string providerName);
  virtual ~MtkCameraProvider();

 public:
  virtual auto setCallback(
      const std::shared_ptr<IMtkcamProviderCallback>& callback) -> int;
  virtual auto getVendorTags(std::vector<VendorTagSection>& sections) const
      -> void;
  virtual auto getCameraIdList(std::vector<int32_t>& rCameraDeviceIds) const
      -> int;
  virtual auto isSetTorchModeSupported() const -> bool;
  virtual auto getDeviceInterface(
      const int32_t cameraDeviceId,
      std::shared_ptr<IMtkcamDevice>& rpDevice) const -> int;
  virtual auto notifyDeviceStateChange(DeviceState newState) -> int;
  virtual auto getConcurrentStreamingCameraIds(
      std::vector<std::vector<int32_t>>& cameraIds) const -> int;
  virtual auto isConcurrentStreamCombinationSupported(
      std::vector<CameraIdAndStreamCombination>& configs,
      bool& queryStatus) const -> int;
  virtual auto debug(std::shared_ptr<IPrinter> printer,
                     const std::vector<std::string>& options) -> void;

 private:
  mutable std::mutex mLock;
  std::shared_ptr<IMtkcamProvider> mpDeviceManager;
  std::shared_ptr<IMtkcamProviderCallback> mpProviderCallback;
};

/******************************************************************************
 *
 ******************************************************************************/
MtkCameraProvider::~MtkCameraProvider() {
  MY_LOGD("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
MtkCameraProvider::MtkCameraProvider(std::string providerName) {
  MY_LOGD("ctor");
  std::size_t foundId = providerName.find("/");
  const char* providerType = "";
  if (foundId != std::string::npos) {
    providerType = providerName.substr(0, foundId).c_str();
  }
  auto pDeviceManager =
      std::make_shared<NSCam::CameraDeviceManagerImpl>(providerType);
  if (pDeviceManager) {
    auto init = pDeviceManager->initialize();
    if (!init) {
      MY_LOGE("CameraDeviceManagerImpl::initialize fail");
    } else {
      mpDeviceManager = pDeviceManager;
    }
  } else {
    MY_LOGE("cannot create CameraDeviceManagerImpl");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::setCallback(
    const std::shared_ptr<IMtkcamProviderCallback>& callback) -> int {
  std::lock_guard<std::mutex> _l(mLock);
  mpProviderCallback = callback;
  if (mpDeviceManager) {
    mpDeviceManager->setCallback(callback);
  }
  return static_cast<int>(mcam::Status::OK);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::getVendorTags(
    std::vector<VendorTagSection>& sections __unused) const -> void {
  MY_LOGW("Not implement");
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::getCameraIdList(
    std::vector<int32_t>& rCameraDeviceIds __unused) const -> int {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    return mpDeviceManager->getCameraIdList(rCameraDeviceIds);
  }
  return static_cast<int>(mcam::Status::METHOD_NOT_SUPPORTED);
}

/******************************************************************************
 *
 ******************************************************************************/
// auto MtkCameraProvider::getDeviceInfoList(
//     std::vector<DeviceInfo>& rDeviceInfo)
//     -> int {
//   std::lock_guard <std::mutex> _l(mLock);
//   if (mpDeviceManager) {
//     mpDeviceManager->getDeviceInfoList(rDeviceInfo);
//   }
//   return static_cast<int>(mcam::Status::OK);
// }

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::isSetTorchModeSupported() const -> bool {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    return mpDeviceManager->isSetTorchModeSupported();
  }
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::getDeviceInterface(
    const int32_t cameraDeviceId,
    std::shared_ptr<IMtkcamDevice>& rpDevice) const -> int {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    return mpDeviceManager->getDeviceInterface(cameraDeviceId, rpDevice);
  }
  return static_cast<int>(mcam::Status::METHOD_NOT_SUPPORTED);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::notifyDeviceStateChange(DeviceState newState) -> int {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    return mpDeviceManager->notifyDeviceStateChange(newState);
  }
  return static_cast<int>(mcam::Status::METHOD_NOT_SUPPORTED);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::getConcurrentStreamingCameraIds(
    std::vector<std::vector<int32_t>>& cameraIds) const -> int {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    return mpDeviceManager->getConcurrentStreamingCameraIds(cameraIds);
  }
  return static_cast<int>(mcam::Status::METHOD_NOT_SUPPORTED);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::isConcurrentStreamCombinationSupported(
    std::vector<CameraIdAndStreamCombination>& configs,
    bool& queryStatus) const -> int {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    return mpDeviceManager->isConcurrentStreamCombinationSupported(configs,
                                                                   queryStatus);
  }
  return static_cast<int>(mcam::Status::METHOD_NOT_SUPPORTED);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCameraProvider::debug(std::shared_ptr<IPrinter> printer,
                              const std::vector<std::string>& options) -> void {
  std::lock_guard<std::mutex> _l(mLock);
  if (mpDeviceManager) {
    mpDeviceManager->debug(printer, options);
  }
}

};  // namespace core
};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
using mcam::core::MtkCameraProvider;

std::shared_ptr<IMtkcamProvider> getMtkcamProviderInstance(
    IProviderCreationParams const* params) {
  static auto mtkProvider =
      std::make_shared<MtkCameraProvider>(params->providerName);
  if (!mtkProvider) {
    MY_LOGE("cannot create MtkCameraProvider %s", params->providerName.c_str());
    return nullptr;
  }

  return mtkProvider;
}
