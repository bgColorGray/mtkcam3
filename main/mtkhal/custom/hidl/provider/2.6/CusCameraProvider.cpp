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

#include "main/mtkhal/custom/hidl/provider/2.6/CusCameraProvider.h"
//
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/Printer.h>
#include <utils/Thread.h>
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

#include "../../device/3.x/CusCameraDevice3.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::CameraMetadataType;
using ::android::hardware::camera::common::V1_0::TorchModeStatus;
using ::android::hardware::camera::common::V1_0::VendorTag;
// using NSCam::ICameraDeviceManager;
using mcam::hidl::HidlCameraProvider;
using NSCam::IPrinter;
using NSCam::custom_dev3::CusCameraDevice3;
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

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_6 {

/******************************************************************************
 *
 ******************************************************************************/
extern "C" ICameraProvider* CreateLegacyCustomProviderInstance(
    const char* providerName) {
  MY_LOGI("+ %s", providerName);
  //
  auto provider = new CusCameraProvider(providerName);
  if (!provider) {
    MY_LOGE("cannot allocate camera provider %s", providerName);
    return nullptr;
  }

  MY_LOGI("- %s provider:%p", providerName, provider);
  return provider;
}

auto CusCameraProvider::getSafeHidlCameraProvider() const
    -> ::android::sp<HidlCameraProvider> {
  Mutex::Autolock _l(mHidlCameraProviderLock);
  return mHidlCameraProvider;
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraProvider::~CusCameraProvider() {
  MY_LOGI("dtor");
  if (mLibHandle != NULL) {
    dlclose(mLibHandle);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraProvider::CusCameraProvider(const char* providerName)
    : ICameraProvider(),
      mVendorTagSections(),
      mProviderCallback(nullptr),
      mProviderCallbackLock() {
  MY_LOGI("ctor");
  {
    MY_LOGI("open camera provider");
    mLibHandle = dlopen(camProviderLib, RTLD_NOW);
    if (mLibHandle == NULL) {
      MY_LOGE("dlopen fail: %s", dlerror());
    }
    // reset errors
    dlerror();
    CreateHidlProviderInstance_t* createHidlProvider =
        reinterpret_cast<CreateHidlProviderInstance_t*>(
            dlsym(mLibHandle, "CreateHidlProviderInstance"));
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
      MY_LOGE("Cannot load symbol CreateHidlProviderInstance: %s", dlerror());
    } else {
      mHidlCameraProvider = createHidlProvider(providerName);
    }
  }
  ::memset(&mProviderCallbackDebugInfo, 0, sizeof(mProviderCallbackDebugInfo));
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraProvider::serviceDied(uint64_t cookie __unused,
                                    const wp<hidl::base::V1_0::IBase>& who
                                        __unused) {
  if (cookie != (uint64_t)this) {
    MY_LOGW("Unexpected ICameraProviderCallback serviceDied cookie 0x%" PRIx64
            ", expected %p",
            cookie, this);
  }
  MY_LOGW("ICameraProviderCallback %s has died; removing it - cookie:0x%" PRIx64
          " this:%p",
          toString(mProviderCallbackDebugInfo).c_str(), cookie, this);

  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    pCameraProvider->serviceDied(cookie, who);
  }
  {
    Mutex::Autolock _l(mProviderCallbackLock);
    mProviderCallback = nullptr;
    ::memset(&mProviderCallbackDebugInfo, 0,
             sizeof(mProviderCallbackDebugInfo));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status> CusCameraProvider::setCallback(
    const ::android::sp<V2_4::ICameraProviderCallback>& callback) {
  {
    Mutex::Autolock _l(mProviderCallbackLock);

    //  cleanup the previous callback if existed.
    if (mProviderCallback != nullptr) {
      {  // unlinkToDeath
        hardware::Return<bool> ret = mProviderCallback->unlinkToDeath(this);
        if (!ret.isOk()) {
          MY_LOGE(
              "Transaction error in unlinking to ICameraProviderCallback "
              "death: %s",
              ret.description().c_str());
        } else if (!ret) {
          MY_LOGW(
              "Unable to unlink to ICameraProviderCallback death "
              "notifications");
        }
      }

      mProviderCallback = nullptr;
      ::memset(&mProviderCallbackDebugInfo, 0,
               sizeof(mProviderCallbackDebugInfo));
    }

    //  setup the new callback.
    mProviderCallback = callback;
    if (mProviderCallback != nullptr) {
      {  // linkToDeath
        hardware::Return<bool> ret =
            mProviderCallback->linkToDeath(this, (uintptr_t)this);
        if (!ret.isOk()) {
          MY_LOGE(
              "Transaction error in linking to ICameraProviderCallback death: "
              "%s",
              ret.description().c_str());
        } else if (!ret) {
          MY_LOGW(
              "Unable to link to ICameraProviderCallback death notifications");
        }
      }

      {  // getDebugInfo
        hardware::Return<void> ret = mProviderCallback->getDebugInfo(
            [this](const auto& info) { mProviderCallbackDebugInfo = info; });
        if (!ret.isOk()) {
          MY_LOGE(
              "Transaction error in ICameraProviderCallback::getDebugInfo: %s",
              ret.description().c_str());
        }
      }

      MY_LOGD("ICameraProviderCallback %s",
              toString(mProviderCallbackDebugInfo).c_str());
    }
  }
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    MY_LOGD("Legacy Custom SetCallback");
    return pCameraProvider->setCallback(this);
  }
  return Status::INTERNAL_ERROR;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::getVendorTags(getVendorTags_cb _hidl_cb) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    auto ret = pCameraProvider->getVendorTags(
        [&](auto status, const auto& vendorTagSecs) {
          MY_LOGD("getVendorTags from HAL, returns status:%d numSections %zu",
                  (int)status, vendorTagSecs.size());
          for (size_t i = 0; i < vendorTagSecs.size(); i++) {
            MY_LOGD("Vendor tag section %zu name %s", i,
                    vendorTagSecs[i].sectionName.c_str());

            // for (size_t j = 0; j < vendorTagSecs[i].tags.size(); j++) {
            //     const auto& tag = vendorTagSecs[i].tags[j];
            //     MY_LOGD("Vendor tag id %u name %s type %d", tag.tagId,
            //     tag.tagName.c_str(), (int)tag.tagType);
            // }
          }
          _hidl_cb(status, vendorTagSecs);
        });
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::getCameraIdList(getCameraIdList_cb _hidl_cb) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    auto ret =
        pCameraProvider->getCameraIdList([&](auto status, const auto& idList) {
          MY_LOGD("getCameraIdList from HAL,  returns status:%d", (int)status);
          for (size_t i = 0; i < idList.size(); i++) {
            MY_LOGD("Camera Id[%zu] is %s", i, idList[i].c_str());
          }
          _hidl_cb(status, idList);
        });
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::isSetTorchModeSupported(
    isSetTorchModeSupported_cb _hidl_cb) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    auto ret = pCameraProvider->isSetTorchModeSupported(
        [&](auto status, bool support) {
          MY_LOGD("isSetTorchModeSupported returns status:%d supported:%d",
                  (int)status, support);
          _hidl_cb(status, support);
        });
  }

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::getCameraDeviceInterface_V1_x(
    const hidl_string& cameraDeviceName,
    getCameraDeviceInterface_V1_x_cb _hidl_cb) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    auto ret = pCameraProvider->getCameraDeviceInterface_V1_x(
        cameraDeviceName, [&](auto status, auto& pDevice) {
          MY_LOGD("getCameraDeviceInterface_V1_x from HAL, returns status:%d",
                  (int)status);
          MY_LOGD("FIXME: should handle V1_0::ICameraDevice ");
          // auto pICameraDevice =
          // NSCam::custom_dev3::CusCameraDevice3::create(::android::hardware::camera::device::V1_0::ICameraDevice::castFrom(pDevice));
          //   if  ( pDevice == nullptr || pICameraDevice == nullptr || 0 !=
          //   (int)status ) {
          //     MY_LOGE(
          //       "DeviceName:%s pDevice:%p status:%d",
          //       cameraDeviceName.c_str(), pDevice.get(), status
          //       );
          //   }
          _hidl_cb(status, pDevice);
        });
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::getCameraDeviceInterface_V3_x(
    const hidl_string& cameraDeviceName,
    getCameraDeviceInterface_V3_x_cb _hidl_cb) {
  MY_LOGD("+");
  MY_LOGD("%s", cameraDeviceName.c_str());
  std::string deviceName = cameraDeviceName;
  std::size_t foundId = deviceName.find_last_of("/\\");
  int intanceId = atoi((deviceName.substr(foundId + 1)).c_str());
  MY_LOGD("intance id: %d", intanceId);

  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    auto ret = pCameraProvider->getCameraDeviceInterface_V3_x(
        cameraDeviceName, [&](auto status, auto& pDevice) {
          MY_LOGD("getCameraDeviceInterface_V3_x from HAL, returns status:%d",
                  (int)status);
          auto pICameraDevice = NSCam::custom_dev3::CusCameraDevice3::create(
              ::android::hardware::camera::device::V3_6::ICameraDevice::
                  castFrom(pDevice),
              intanceId);
          if (pDevice == nullptr || pICameraDevice == nullptr ||
              0 != static_cast<int>(status)) {
            MY_LOGE("DeviceName:%s pDevice:%p status:(%d)",
                    cameraDeviceName.c_str(), pDevice.get(), status);
          }
          _hidl_cb(status, pICameraDevice);
        });
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::notifyDeviceStateChange(
    hidl_bitfield<::android::hardware::camera::provider::V2_5::DeviceState>
        newState) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    pCameraProvider->notifyDeviceStateChange(newState);
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::getConcurrentStreamingCameraIds(
    getConcurrentStreamingCameraIds_cb _hidl_cb) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    hidl_vec<hidl_vec<hidl_string>> combinations;
    auto ret = pCameraProvider->getConcurrentStreamingCameraIds(
        [&](auto concurrentIdStatus, const auto& cameraDeviceIdCombinations) {
          MY_LOGD(
              "getConcurrentStreamingCameraIds from HAL, returns status:%d, "
              "cameraDeviceIdCombinations size:%zu",
              (int)concurrentIdStatus, cameraDeviceIdCombinations.size());
          _hidl_cb(concurrentIdStatus, cameraDeviceIdCombinations);
        });
  }

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::isConcurrentStreamCombinationSupported(
    const hidl_vec<CameraIdAndStreamCombination>& configs,
    isConcurrentStreamCombinationSupported_cb _hidl_cb) {
  MY_LOGD("+");
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    pCameraProvider->isConcurrentStreamCombinationSupported(
        configs, [&](auto status, auto queryStatus) {
          MY_LOGD(
              "isConcurrentStreamCombinationSupported from HAL, returns "
              "status:%d, queryStatus :%d",
              (int)status, queryStatus);
          _hidl_cb(status, queryStatus);
        });
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::debug(
    const ::android::hardware::hidl_handle& handle,
    const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
        options) {
  CAM_TRACE_CALL();
  auto pCameraProvider = getSafeHidlCameraProvider();
  if (pCameraProvider != 0) {
    return pCameraProvider->debug(handle, options);
  }

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::cameraDeviceStatusChange(
    const hidl_string& cameraDeviceName,
    ::android::hardware::camera::common::V1_0::CameraDeviceStatus newStatus) {
  MY_LOGD("+");
  ::android::sp<V2_4::ICameraProviderCallback> callback;
  {
    Mutex::Autolock _l(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %s newStatus:%u", cameraDeviceName.c_str(),
            newStatus);
    return Void();
  }

  auto ret = callback->cameraDeviceStatusChange(cameraDeviceName, newStatus);
  if (!ret.isOk()) {
    MY_LOGE(
        "Transaction error in "
        "ICameraProviderCallback::cameraDeviceStatusChange: %s",
        ret.description().c_str());
  }
  MY_LOGI("%s CameraDeviceStatus:%u", cameraDeviceName.c_str(), newStatus);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::torchModeStatusChange(
    const hardware::hidl_string& deviceName,
    ::android::hardware::camera::common::V1_0::TorchModeStatus newStatus) {
  MY_LOGD("+");
  ::android::sp<V2_4::ICameraProviderCallback> callback;
  {
    Mutex::Autolock _l(mProviderCallbackLock);
    callback = mProviderCallback;
  }

  if (callback == 0) {
    MY_LOGW("bad mProviderCallback - %s new_status:%u", deviceName.c_str(),
            newStatus);
    return Void();
  }

  auto ret = callback->torchModeStatusChange(deviceName, newStatus);
  if (!ret.isOk()) {
    MY_LOGE(
        "Transaction error in ICameraProviderCallback::torchModeStatusChange: "
        "%s",
        ret.description().c_str());
  }
  MY_LOGI("%s TorchModeStatus:%u", deviceName.c_str(), newStatus);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraProvider::physicalCameraDeviceStatusChange(
    const hidl_string& cameraDeviceName,
    const hidl_string& physicalCameraDeviceName,
    ::android::hardware::camera::common::V1_0::CameraDeviceStatus newStatus) {
  MY_LOGW("Not implement");
  return Void();
}

};  // namespace V2_6
};  // namespace provider
};  // namespace camera
};  // namespace hardware
};  // namespace android
