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

#include <main/mtkhal/hidl/device/3.x/HidlCameraDevice3.h>
#include <main/mtkhal/hidl/device/3.x/HidlCameraDeviceCallback.h>
#include <main/mtkhal/hidl/device/3.x/HidlCameraDeviceSession.h>
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
using NSCam::IDebuggeeManager;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)                                                \
  CAM_LOGV("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGD(fmt, arg...)                                                \
  CAM_LOGD("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGI(fmt, arg...)                                                \
  CAM_LOGI("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGW(fmt, arg...)                                                \
  CAM_LOGW("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGE(fmt, arg...)                                                \
  CAM_LOGE("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGA(fmt, arg...)                                                \
  CAM_LOGA("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
           ##arg)
#define MY_LOGF(fmt, arg...)                                                \
  CAM_LOGF("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, \
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

namespace mcam {
namespace hidl {
/******************************************************************************
 *
 ******************************************************************************/
const char* HidlCameraDevice3::MyDebuggee::mName{ICameraDevice::descriptor};
auto HidlCameraDevice3::MyDebuggee::debug(
    ::android::Printer& printer,
    const std::vector<std::string>& options) -> void {
  struct MyPrinter : public IPrinter {
    ::android::Printer* mPrinter;
    explicit MyPrinter(::android::Printer* printer)
      : mPrinter(printer) {}
    ~MyPrinter() {}
    virtual void printLine(const char* string) {
      mPrinter->printLine(string);
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
    virtual std::shared_ptr<::android::FdPrinter> getFDPrinter() {
      return nullptr;
    }
  } myPrinter(&printer);
  auto p = mContext.promote();
  if (p != nullptr) {
    p->debug(myPrinter, options);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDevice3::~HidlCameraDevice3() {
  MY_LOGI("dtor");
  if (mDebuggee != nullptr) {
    if (auto pDbgMgr = IDebuggeeManager::get()) {
      pDbgMgr->detach(mDebuggee->mCookie);
    }
    mDebuggee = nullptr;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDevice3::HidlCameraDevice3(const CreationInfo& info)
    : ICameraDevice(),
      mLogLevel(0),
      mInstanceId(info.instanceId),
      mDevice(info.pDevice) {
  MY_LOGI("ctor");
  mDebuggee = std::make_shared<MyDebuggee>(this);
  if (auto pDbgMgr = IDebuggeeManager::get()) {
    mDebuggee->mCookie = pDbgMgr->attach(mDebuggee, 1);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDevice3::create(const CreationInfo& info)
    -> ::android::hidl::base::V1_0::IBase* {
  auto pInstance = new HidlCameraDevice3(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::getResourceCost(getResourceCost_cb _hidl_cb) {
  CameraResourceCost hidlCost;
  mcam::android::CameraResourceCost mtkCost;

  int status = mDevice->getResourceCost(mtkCost);

  hidlCost.resourceCost = mtkCost.resourceCost;
  hidlCost.conflictingDevices.resize(mtkCost.conflictingDevices.size());
  for (int i = 0; i < mtkCost.conflictingDevices.size(); ++i) {
    hidlCost.conflictingDevices[i] =
        mapToHidlDeviceName(mtkCost.conflictingDevices[i]);
  }

  _hidl_cb(mapToHidlCameraStatus(status), hidlCost);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::getCameraCharacteristics(
    getCameraCharacteristics_cb _hidl_cb) {
  CameraMetadata cameraCharacteristics;
  {
    const camera_metadata* p_camera_metadata;
    mDevice->getCameraCharacteristics(p_camera_metadata);
    if (p_camera_metadata == nullptr) {
      MY_LOGE("fail to getCameraCharacteristics");
    }

    cameraCharacteristics.setToExternal(
        reinterpret_cast<uint8_t*>(
            const_cast<camera_metadata*>(p_camera_metadata)),
        get_camera_metadata_size(p_camera_metadata), false /*shouldOwn*/);
  }

  _hidl_cb(Status::OK, cameraCharacteristics);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status> HidlCameraDevice3::setTorchMode(TorchMode mode) {
  int status =
      mDevice->setTorchMode(static_cast<mcam::android::TorchMode>(mode));
  return mapToHidlCameraStatus(status);
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::open(
    const ::android::sp<
        ::android::hardware::camera::device::V3_2::ICameraDeviceCallback>&
        callback_3_2,
    open_cb _hidl_cb) {
  auto callback = ::android::hardware::camera::device::V3_5::
      ICameraDeviceCallback::castFrom(callback_3_2);
  // 1. create hidl callback
  HidlCameraDeviceCallback::CreationInfo hidlCallbackCreationInfo = {
      .instanceId = getInstanceId(),
      .pCallback = callback,
  };
  auto hidlCallback = std::shared_ptr<HidlCameraDeviceCallback>(
      HidlCameraDeviceCallback::create(hidlCallbackCreationInfo));
  if (hidlCallback == nullptr) {
    _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
    return Void();
  }

  // 2. open
  HidlCameraDeviceSession::CreationInfo hidlSessionCreationInfo = {
      .instanceId = getInstanceId(),
      .pSession = mDevice->open(hidlCallback),
      .pHidlCallback = hidlCallback,
  };
  if (hidlSessionCreationInfo.pSession == nullptr) {
    _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
    return Void();
  }
  auto hidlSession = ::android::sp<HidlCameraDeviceSession>(
      HidlCameraDeviceSession::create(hidlSessionCreationInfo));
  if (hidlSession == nullptr) {
    _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
    return Void();
  }
  auto status = hidlSession->open(callback);
  //
  if (0 != status) {
    _hidl_cb(mapToHidlCameraStatus(status), nullptr);
  } else {
    _hidl_cb(mapToHidlCameraStatus(status), hidlSession);
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::dumpState(
    const ::android::hardware::hidl_handle& handle) {
  ::android::hardware::hidl_vec<::android::hardware::hidl_string> options;
  debug(handle, options);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::debug(
    const ::android::hardware::hidl_handle& handle __unused,
    const ::android::hardware::hidl_vec<::android::hardware::hidl_string>&
        options __unused) {
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

  struct MyPrinter : public FdPrinter, public NSCam::IPrinter {
    explicit MyPrinter(int fd) : FdPrinter(fd) {}
    void printLine(const char* string) { FdPrinter::printLine(string); }
    void printFormatLine(const char* format, ...) {
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
  } printer(handle->data[0]);

  debug(printer, std::vector<std::string>{options.begin(), options.end()});

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDevice3::debug(IPrinter& printer,
                              const std::vector<std::string>& options) -> void {
  printer.printFormatLine("## Camera device [%s]",
                          std::to_string(getInstanceId()).c_str());
  mDevice->dumpState(printer, options);
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::getPhysicalCameraCharacteristics(
    const ::android::hardware::hidl_string& physicalCameraId,
    getPhysicalCameraCharacteristics_cb _hidl_cb) {
  CameraMetadata cameraCharacteristics;
  int physicalId = strtol(physicalCameraId.c_str(), NULL, 10);

  const camera_metadata* p_camera_metadata;
  int status = mDevice->getPhysicalCameraCharacteristics(physicalId,
                                                         p_camera_metadata);

  if (status == OK) {
    cameraCharacteristics.setToExternal(
        reinterpret_cast<uint8_t*>(
            const_cast<camera_metadata*>(p_camera_metadata)),
        get_camera_metadata_size(p_camera_metadata),
        false /*shouldOwn*/);
    _hidl_cb(Status::OK, cameraCharacteristics);
  } else {
    MY_LOGD("Physical camera ID %d is visible for logical ID %d", physicalId,
            getInstanceId());
    _hidl_cb(Status::ILLEGAL_ARGUMENT, cameraCharacteristics);
  }

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDevice3::isStreamCombinationSupported(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
        streams,
    isStreamCombinationSupported_cb _hidl_cb) {
  bool isSupported = true;
  mcam::android::StreamConfiguration mtkStreams;
  convertFromHidl(streams, mtkStreams);
  int status = mDevice->isStreamCombinationSupported(mtkStreams, isSupported);

  if (status == OK) {
    _hidl_cb(Status::OK, isSupported);
  } else {
    _hidl_cb(Status::ILLEGAL_ARGUMENT, isSupported);
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDevice3::convertFromHidl(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
        srcStreams,
    mcam::android::StreamConfiguration& dstStreams) -> void {
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
  dstStreams.streamConfigCounter = 0;  // V3_5

  return;
}
};  // namespace hidl
};  // namespace mcam
