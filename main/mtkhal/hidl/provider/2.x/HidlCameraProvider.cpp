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

#include <HidlCameraCommon.h>
#include <main/mtkhal/hidl/device/3.x/HidlCameraDevice3.h>
#include <main/mtkhal/hidl/provider/2.x/HidlCameraProvider.h>
#include <main/mtkhal/hidl/provider/2.x/HidlCameraProviderCallback.h>
//
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/Printer.h>
#include <utils/Thread.h>
//
#include <mtkcam3/main/mtkhal/android/provider/2.x/IACameraProvider.h>
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
#include <future>
#include <memory>
#include <string>
#include <vector>
//
// #include "test/test.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using ::android::FdPrinter;
using ::android::Mutex;
using ::android::OK;
using ::android::hardware::Return;
using ::android::hardware::camera::common::V1_0::CameraMetadataType;
using ::android::hardware::camera::common::V1_0::VendorTag;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::camera::provider::V2_6::ICameraProvider;
using mcam::android::IACameraDevice;
using mcam::android::IACameraProvider;
using mcam::hidl::HidlCameraDevice3;
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
namespace hidl {

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraProvider::~HidlCameraProvider() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraProvider::HidlCameraProvider(const CreationInfo& info)
    : ICameraProvider(),
      mProvider(info.pProvider),
      mVendorTagSections(),
      mProviderCallback(nullptr),
      mProviderCallbackLock(),
      mHidlProviderCallback(nullptr) {
  ::memset(&mProviderCallbackDebugInfo, 0, sizeof(mProviderCallbackDebugInfo));
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraProvider::create(const CreationInfo& info)
    -> HidlCameraProvider* {
  auto pInstance = new HidlCameraProvider(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
bool HidlCameraProvider::initialize() {
  MY_LOGI("+");
  //
  //  (1) setup vendor tags (and global vendor tags)
  if (!setupVendorTags()) {
    MY_LOGE("setupVendorTags() fail");
    return false;
  }
  //  (2) check device IACameraProvider
  if (!mProvider) {
    MY_LOGE("bad IACameraProvider");
    return false;
  }
  //
  MY_LOGI("-");
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
bool HidlCameraProvider::setupVendorTags() {
  auto pVendorTagDesc = NSCam::getVendorTagDescriptor();
  if (!pVendorTagDesc) {
    MY_LOGE("bad pVendorTagDesc");
    return false;
  }
  //
  //  setup mVendorTagSections
  auto vSection = pVendorTagDesc->getSections();
  size_t const numSections = vSection.size();
  mVendorTagSections.resize(numSections);
  for (size_t i = 0; i < numSections; i++) {
    auto const& s = vSection[i];
    //
    // s.tags; -> tags;
    std::vector<VendorTag> tags;
    tags.reserve(s.tags.size());
    for (auto const& t : s.tags) {
      VendorTag vt;
      vt.tagId = t.second.tagId;
      vt.tagName = t.second.tagName;
      vt.tagType = (CameraMetadataType)t.second.tagType;
      tags.push_back(vt);
    }
    //
    mVendorTagSections[i].tags = tags;
    mVendorTagSections[i].sectionName = s.sectionName;
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
void HidlCameraProvider::serviceDied(
    uint64_t cookie __unused,
    const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused) {
  if (cookie != (uint64_t)this) {
    MY_LOGW("Unexpected ICameraProviderCallback serviceDied cookie 0x%" PRIx64
            ", expected %p",
            cookie, this);
  }
  MY_LOGW("ICameraProviderCallback %s has died; removing it - cookie:0x%" PRIx64
          " this:%p",
          toString(mProviderCallbackDebugInfo).c_str(), cookie, this);
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
Return<Status> HidlCameraProvider::setCallback(
    const ::android::sp<ICameraProviderCallback>& callback) {
  {
    Mutex::Autolock _l(mProviderCallbackLock);

    //  cleanup the previous callback if existed.
    if (mProviderCallback != nullptr) {
      {  // unlinkToDeath
        Return<bool> ret = mProviderCallback->unlinkToDeath(this);
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
        Return<bool> ret =
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
        Return<void> ret = mProviderCallback->getDebugInfo(
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

    HidlCameraProviderCallback::CreationInfo creationInfo = {
        .pCallback = mProviderCallback,
    };
    mHidlProviderCallback = std::shared_ptr<HidlCameraProviderCallback>(
        HidlCameraProviderCallback::create(creationInfo));
    if (mHidlProviderCallback == nullptr) {
      return Status::ILLEGAL_ARGUMENT;
    }
  }

  return mapToHidlCameraStatus(mProvider->setCallback(mHidlProviderCallback));
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::getVendorTags(getVendorTags_cb _hidl_cb) {
  _hidl_cb(Status::OK, mVendorTagSections);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::getCameraIdList(getCameraIdList_cb _hidl_cb) {
  //  deviceInfoList from device manager
  std::vector<int32_t> cameraIdList;
  auto status = mProvider->getCameraIdList(cameraIdList);

  //  deviceInfoList -> hidlDeviceNameList
  hidl_vec<hidl_string> hidlDeviceNameList;
  hidlDeviceNameList.resize(cameraIdList.size());
  for (size_t i = 0; i < cameraIdList.size(); i++) {
    /**
     * The device instance names must be of the form
     *      "device@<major>.<minor>/<type>/<id>"
     */
    hidlDeviceNameList[i] = mapToHidlDeviceName(cameraIdList[i]);
  }
  _hidl_cb(mapToHidlCameraStatus(status), hidlDeviceNameList);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::isSetTorchModeSupported(
    isSetTorchModeSupported_cb _hidl_cb) {
  _hidl_cb(Status::OK, mProvider->isSetTorchModeSupported());
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
template <class InterfaceT, class InterfaceCallbackT>
void HidlCameraProvider::getCameraDeviceInterface(
    const hidl_string& cameraDeviceName,
    InterfaceCallbackT _hidl_cb) {
  HidlCameraDevice3::CreationInfo creationInfo;
  std::string deviceName = cameraDeviceName;
  std::size_t foundId = deviceName.find_last_of("/\\");
  creationInfo.instanceId =
      static_cast<int32_t>(atoi((deviceName.substr(foundId + 1)).c_str()));
  auto status = mProvider->getDeviceInterface(creationInfo.instanceId,
                                              creationInfo.pDevice);
  //
  auto pICameraDevice = ::android::sp<InterfaceT>(
      static_cast<InterfaceT*>(HidlCameraDevice3::create(creationInfo)));
  if (creationInfo.pDevice == nullptr || pICameraDevice == nullptr ||
      0 != status) {
    MY_LOGE("DeviceName:%s pDevice:%p pICameraDevice:%p status:%s(%d)",
            cameraDeviceName.c_str(), creationInfo.pDevice.get(),
            pICameraDevice.get(), ::strerror(-status), -status);
  }
  _hidl_cb(mapToHidlCameraStatus(status), pICameraDevice);
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::getCameraDeviceInterface_V1_x(
    const hidl_string& cameraDeviceName,
    getCameraDeviceInterface_V1_x_cb _hidl_cb) {
  getCameraDeviceInterface<
      ::android::hardware::camera::device::V1_0::ICameraDevice>(
      cameraDeviceName, _hidl_cb);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::getCameraDeviceInterface_V3_x(
    const hidl_string& cameraDeviceName,
    getCameraDeviceInterface_V3_x_cb _hidl_cb) {
  getCameraDeviceInterface<
      ::android::hardware::camera::device::V3_2::ICameraDevice>(
      cameraDeviceName, _hidl_cb);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::notifyDeviceStateChange(
    ::android::hardware::hidl_bitfield<
        ::android::hardware::camera::provider::V2_5::DeviceState> newState) {
  // uint64_t state = static_cast<uint64_t>(newState); in emulator
  mcam::android::DeviceState state =
      static_cast<mcam::android::DeviceState>(newState);
  mProvider->notifyDeviceStateChange(state);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::getConcurrentStreamingCameraIds(
    getConcurrentStreamingCameraIds_cb _hidl_cb) {
  MY_LOGW("Not implement, return OK");
  hidl_vec<hidl_vec<hidl_string>> hidlDeviceNameList;
  std::vector<std::vector<int32_t>> deviceIdList;
  auto status = mProvider->getConcurrentStreamingCameraIds(deviceIdList);
  if (status == 0) {
    hidlDeviceNameList.resize(deviceIdList.size());
    for (size_t i = 0; i < deviceIdList.size(); i++) {
      hidlDeviceNameList[i].resize(deviceIdList[i].size());
      for (size_t j = 0; j < deviceIdList[i].size(); j++) {
        hidlDeviceNameList[i][j] = mapToHidlDeviceName(deviceIdList[i][j]);
      }
    }
  }
  _hidl_cb(mapToHidlCameraStatus(status), hidlDeviceNameList);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::isConcurrentStreamCombinationSupported(
    const hidl_vec<CameraIdAndStreamCombination>& configs,
    isConcurrentStreamCombinationSupported_cb _hidl_cb) {
  MY_LOGW("Not implement, return METHOD_NOT_SUPPORTED");
  //  * @return status Status code for the operation, one of:
  //  *     METHOD_NOT_SUPPORTED:
  //  *          The camera provider does not support stream combination query.
  //  * @return true in case the stream combination is supported, false
  //  otherwise.
  bool queryStatus = false;
  std::vector<mcam::android::CameraIdAndStreamCombination> halConfigs;
  halConfigs.resize(configs.size());
  for (size_t i = 0; i < configs.size(); i++) {
    halConfigs[i].cameraId = std::stoi(configs[i].cameraId);
    convertStreamConfigurationFromHidl(configs[i].streamConfiguration,
                                       halConfigs[i].streamConfiguration);
  }
  auto status = mProvider->isConcurrentStreamCombinationSupported(halConfigs,
                                                                  queryStatus);
  _hidl_cb(mapToHidlCameraStatus(status), queryStatus);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraProvider::convertStreamConfigurationFromHidl(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
        srcStreams,
    mcam::android::StreamConfiguration& dstStreams) -> int {
  CAM_TRACE_CALL();
  for (auto& stream : srcStreams.streams) {
    MY_LOGD("%s", toString(stream).c_str());
  }
  MY_LOGD("operationMode=%u", srcStreams.operationMode);

  int size = srcStreams.streams.size();
  dstStreams.streams.resize(size);
  for (int i = 0; i < size; i++) {
    auto& srcStream = srcStreams.streams[i];
    auto& dstStream = dstStreams.streams[i];
    //
    dstStream.id = srcStream.v3_2.id;
    dstStream.streamType =
        static_cast<mcam::android::StreamType>(srcStream.v3_2.streamType);
    dstStream.width = srcStream.v3_2.width;
    dstStream.height = srcStream.v3_2.height;
    dstStream.format =
        static_cast<android_pixel_format_t>(srcStream.v3_2.format);
    dstStream.usage = srcStream.v3_2.usage;
    dstStream.dataSpace =
        static_cast<android_dataspace_t>(srcStream.v3_2.dataSpace);
    dstStream.rotation = static_cast<mcam::android::StreamRotation>(
        srcStream.v3_2.rotation);
    //
    if (srcStream.physicalCameraId == "") {
      dstStream.physicalCameraId = -1;
    } else {
      dstStream.physicalCameraId =
          std::stoi(static_cast<std::string>(srcStream.physicalCameraId));
    }
    dstStream.bufferSize = srcStream.bufferSize;
  }

  dstStreams.operationMode =
      static_cast<mcam::android::StreamConfigurationMode>(
          srcStreams.operationMode);
  dstStreams.sessionParams = reinterpret_cast<const camera_metadata_t*>(
      srcStreams.sessionParams.data());

  for (auto& stream : dstStreams.streams) {
    MY_LOGD("%s", toString(stream).c_str());
  }
  MY_LOGD("operationMode=%u", dstStreams.operationMode);
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraProvider::debug(
    const ::android::hardware::hidl_handle& handle,
    const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
        options) {
  CAM_TRACE_CALL();
  //  validate handle
  if (handle == nullptr) {
    MY_LOGE("bad handle:%p", handle.getNativeHandle());
    return Void();
  } else if (handle->numFds != 1) {
    MY_LOGE("bad handle:%p numFds:%d", handle.getNativeHandle(),
            handle->numFds);
    return Void();
  } else if (handle->data[0] < 0) {
    MY_LOGE("bad handle:%p numFds:%d fd:%d < 0", handle.getNativeHandle(),
            handle->numFds, handle->data[0]);
    return Void();
  }
  const int fd = handle->data[0];
#if MTKCAM_TARGET_BUILD_VARIANT == 'u'
  const char targetBuild[] = "user-build";
#elif MTKCAM_TARGET_BUILD_VARIANT == 'd'
  const char targetBuild[] = "userdebug-build";
#else
  const char targetBuild[] = "eng-build";
#endif
  struct MyPrinter : public IPrinter {
    int mDupFd = -1;
    std::shared_ptr<FdPrinter> mFdPrinter;
    ULogPrinter mLogPrinter;
    explicit MyPrinter(int fd)
        : mDupFd(::dup(fd)),
          mFdPrinter(std::make_shared<FdPrinter>(mDupFd)),
          mLogPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_INFO) {}
    ~MyPrinter() {
      if (mDupFd >= 0) {
        close(mDupFd);
        mDupFd = -1;
      }
    }
    virtual void printLine(const char* string) {
      mFdPrinter->printLine(string);
#if MTKCAM_TARGET_BUILD_VARIANT == 'u'
      // in user-build: dump it to the log, too.
      mLogPrinter.printLine(string);
#endif
    }
    virtual void printFormatLine(const char* format, ...) {
      va_list arglist;
      va_start(arglist, format);
      char* formattedString;
      if (vasprintf(&formattedString, format, arglist) < 0) {
        CAM_ULOGME("Failed to format string");
        va_end(arglist);
        return;
      }
      va_end(arglist);
      printLine(formattedString);
      free(formattedString);
    }
    virtual std::shared_ptr<FdPrinter> getFDPrinter() {
      return mFdPrinter;
    }
  };
  auto printer = std::make_shared<MyPrinter>(fd);
  {
    std::string log{" ="};
    for (auto const& option : options) {
      log += option;
      log += " ";
    }
    MY_LOGI("fd:%d options(#%zu)%s", fd, options.size(), log.c_str());
    printer->printFormatLine(
        "## Reporting + (%s)",
        NSCam::Utils::LogTool::get()->getFormattedLogTime().c_str());
    printer->printFormatLine(
        "## Camera Hal Service (camerahalserver pid:%d %zuBIT) %s ## fd:%d "
        "options(#%zu)%s",
        ::getpid(), (sizeof(intptr_t) << 3), targetBuild, fd, options.size(),
        log.c_str());
    // if (log.find("hal3plus_ut") != std::string::npos) {
    //   MY_LOGI("enter hal3plus_ut");
    //   Hal3Plus* pHal3Plus = new Hal3Plus();
    //   std::vector<int32_t> cameraIdList;
    //   auto status = mProvider->getCameraIdList(cameraIdList);
    //   std::vector<std::string> deviceNameList;
    //   deviceNameList.resize(cameraIdList.size());
    //   for (size_t i = 0; i < cameraIdList.size(); i++) {
    //     deviceNameList[i] =
    //         mapToHidlDeviceName(cameraIdList[i]);
    //   }
    //   pHal3Plus->initial(log, deviceNameList);
    //   auto Id = pHal3Plus->getOpenId();
    //   std::shared_ptr<IACameraDevice> pDevice;
    //   mProvider->getDeviceInterface(static_cast<int32_t>(Id), pDevice);
    //   HidlCameraDevice3::CreationInfo creationInfo = {
    //       .instanceId = static_cast<int32_t>(Id),
    //       .pDevice = pDevice,
    //   };
    //   auto pICameraDevice = ::android::sp<
    //       ::android::hardware::camera::device::V3_2::ICameraDevice>(
    //       static_cast<
    //           ::android::hardware::camera::device::V3_2::ICameraDevice*>(
    //           HidlCameraDevice3::create(creationInfo)));
    //   pHal3Plus->setDevice(pICameraDevice);
    //   pHal3Plus->setVendorTag(mVendorTagSections);
    //   pHal3Plus->start();
    // }
  }
  {
    Mutex::Autolock _l(mProviderCallbackLock);
    printer->printFormatLine("  ICameraProviderCallback %p %s",
                             mProviderCallback.get(),
                             toString(mProviderCallbackDebugInfo).c_str());
  }
  //// IACameraProvider
  {
    mProvider->debug(printer,
                     std::vector<std::string>{options.begin(), options.end()});
    printer->printFormatLine(
        "## Reporting done - (%s)",
        NSCam::Utils::LogTool::get()->getFormattedLogTime().c_str());
  }
  MY_LOGI("-");
  return ICameraProvider::debug(handle, options);
}

};  // namespace hidl
};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
using mcam::hidl::HidlCameraProvider;

extern "C" {
HidlCameraProvider* CreateHidlProviderInstance(const char* providerName) {
  IACameraProviderParams params = {
      .providerName = std::string(providerName),
  };
  std::shared_ptr<IACameraProvider> aProvider =
      getIACameraProviderInstance(&params);
  if (!aProvider) {
    MY_LOGE("fail to get IACameraProvider %s", providerName);
    return nullptr;
  }

  HidlCameraProvider::CreationInfo creationInfo = {
      .providerName = providerName,
      .pProvider = aProvider,
  };
  HidlCameraProvider* hidlProvider = HidlCameraProvider::create(creationInfo);
  if (!hidlProvider) {
    MY_LOGE("cannot create HidlCameraProvider %s", providerName);
    return nullptr;
  } else {
    hidlProvider->initialize();
  }

  return hidlProvider;
}
}
