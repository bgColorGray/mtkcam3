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

#include <main/mtkhal/core/device/3.x/MtkCbApdator/MtkCallbackAdaptor.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
#include "MyUtils.h"

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
//
using ::android::Mutex;
using ::android::OK;
using ::android::String8;
using NSCam::IDebuggeeManager;
using NSCam::v3::StreamId_T;
using NSCam::Type2Type;
using NSCam::Utils::ULog::DetailsType;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::ULogPrinter;
using NSCam::v3::STREAM_BUFFER_STATUS;
// using mcam::core::Utils::AppErrorImageStreamBuffer;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...)                    \
  do {                                                  \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
  } while (0)

#define MY_WARN(level, fmt, arg...)                     \
  do {                                                  \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                    \
  do {                                                  \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
  } while (0)

#define MY_LOGV(...) MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...) MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...) MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...) MY_WARN(W, __VA_ARGS__)
#define MY_LOGE(...) MY_ERROR(E, __VA_ARGS__)
#define MY_LOGA(...) MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...) MY_ERROR(F, __VA_ARGS__)
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
auto IAppStreamManager::create(const CreationInfo& creationInfo)
    -> IAppStreamManager* {
  auto pInstance = new MtkCallbackAdaptor(creationInfo);
  if (!pInstance || !pInstance->initialize()) {
    delete pInstance;
    return nullptr;
  }
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
MtkCallbackAdaptor::MtkCallbackAdaptor(const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>()),
      mInstanceName{std::to_string(creationInfo.mInstanceId) +
                    ":MtkCallbackAdaptor"} {
  if (creationInfo.mCameraDeviceCallback == nullptr ||
      creationInfo.mMetadataProvider == nullptr ||
      creationInfo.mMetadataConverter == nullptr) {
    MY_LOGE(
        "mCameraDeviceCallback:%p mMetadataProvider:%p mMetadataConverter:%p",
        creationInfo.mCameraDeviceCallback.get(),
        creationInfo.mMetadataProvider.get(),
        creationInfo.mMetadataConverter.get());
    mCommonInfo = nullptr;
    return;
  }

  if (mCommonInfo != nullptr) {
    size_t aAtMostMetaStreamCount = 1;
    IMetadata::IEntry const& entry =
        creationInfo.mMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_REQUEST_PARTIAL_RESULT_COUNT);
    if (entry.isEmpty()) {
      MY_LOGE("no static REQUEST_PARTIAL_RESULT_COUNT");
      aAtMostMetaStreamCount = 1;
    } else {
      aAtMostMetaStreamCount = entry.itemAt(0, Type2Type<MINT32>());
    }
    //
    int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
    if (loglevel == 0) {
      loglevel =
          ::property_get_int32("vendor.debug.camera.log.MtkCallbackAdaptor", 0);
    }
    mCommonInfo->mLogLevel = loglevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    mCommonInfo->mDeviceCallback = creationInfo.mCameraDeviceCallback;
    mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
    mCommonInfo->mPhysicalMetadataProviders =
        creationInfo.mPhysicalMetadataProviders;
    mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
    mCommonInfo->mAtMostMetaStreamCount = aAtMostMetaStreamCount;
    //
    // create CoreSetting
    MINT64 iRefineCbRule = creationInfo.iRefineCbRule;
    if (iRefineCbRule == -1) {
      MY_LOGI("user not set command to change mtk callback rule refinement, "
              "set to bypass mode, 0x%X",
              EnableOption::eCORE_BYPASS);
      iRefineCbRule = EnableOption::eCORE_BYPASS;
    }
    MINT64 iDebugRefineCBRule =
        ::property_get_int64("vendor.debug.camera.log.MtkCbRule", -1);
    if (iDebugRefineCBRule >= 0) {
      MY_LOGI("debug MtkCbRule ON, set rule refinement value = 0x%X",
              iDebugRefineCBRule);
      iRefineCbRule = iDebugRefineCBRule;
    }
    mCommonInfo->coreSetting =
        std::make_shared<CoreSetting>(iRefineCbRule);
  }
  // custom zone position : 0 = up, 1 = down
  bNeedCallbackCore = mCommonInfo->coreSetting->enableCallbackCore();
}

/******************************************************************************
 *
 ******************************************************************************/
bool MtkCallbackAdaptor::initialize() {
  android::status_t status = OK;
  //
  if (mCommonInfo == nullptr) {
    MY_LOGE("Bad mCommonInfo");
    return false;
  }
  //
  {
    mCoreConverter = std::make_shared<CoreConverter>(mCommonInfo);
    if (mCoreConverter == nullptr) {
      MY_LOGE("Bad mCoreConverter");
      return false;
    }
    //
    if (bNeedCallbackCore) {
      mCallbackHandler = new CallbackHandler(mCommonInfo);
      if (mCallbackHandler == nullptr) {
        MY_LOGE("Bad mCallbackHandler");
        return false;
      } else {
        const std::string threadName{std::to_string(mCommonInfo->mInstanceId) +
                                     ":ACbAdp-CbHdl"};
        status = mCallbackHandler->run(threadName.c_str());
        if (OK != status) {
          MY_LOGE("Fail to run the thread %s - status:%d(%s)",
                  threadName.c_str(), status, ::strerror(-status));
          return false;
        }
      }
      //
      ICallbackCore::CreationInfo cbCore_CreationInfo{
          .mInstanceId = mCommonInfo->mInstanceId,
          .mLogLevel = mCommonInfo->mLogLevel,
          .mSupportBufferManagement = mCommonInfo->mSupportBufferManagement,
          .mAtMostMetaStreamCount = mCommonInfo->mAtMostMetaStreamCount,
          .mpfnCallback = onReceiveCbParcels,
          .mpUser = this,
          .userName = LOG_TAG,
          .coreSetting = mCommonInfo->coreSetting,
      };
      mCallbackCore = ICallbackCore::create(cbCore_CreationInfo);
      if (mCallbackCore == nullptr) {
        MY_LOGE("Bad mCallbackCore");
        return false;
      }
    }
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::destroy() -> void {
  auto resLock = mInterfaceLock.timedLock(::ms2ns(1000));
  if (OK != resLock) {
    MY_LOGW("timedLock failed; still go on to destroy");
  }

  if (bNeedCallbackCore) {
    mCallbackCore->destroy();
    mCallbackCore = nullptr;

    mCallbackHandler->destroy();
    mCallbackHandler = nullptr;
  }

  if (OK == resLock) {
    mInterfaceLock.unlock();
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::dumpState(IPrinter& printer __unused,
                                   const std::vector<std::string>& options
                                       __unused) -> void {
  printer.printFormatLine("## Mtk Callback Adaptor  [%u]\n",
                          mCommonInfo->mInstanceId);

  if (OK == mInterfaceLock.timedLock(::ms2ns(500))) {
    dumpStateLocked(printer, options);

    mInterfaceLock.unlock();
  } else {
    printer.printLine("mInterfaceLock.timedLock timeout");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::dumpStateLocked(
    IPrinter& printer,
    const std::vector<std::string>& options) -> void {
  if (mCallbackCore != nullptr) {
    printer.printLine(" ");
    mCallbackCore->dumpState(printer, options);
  }

  if (mCallbackHandler != nullptr) {
    printer.printLine(" ");
    mCallbackHandler->dumpState(printer, options);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
// copy config image/meta map into AppStreamMgr.FrameHandler
auto MtkCallbackAdaptor::beginConfiguration(StreamConfiguration const& rStreams)
    -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  if (bNeedCallbackCore)
    mCallbackCore->beginConfiguration(rStreams);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::endConfiguration(
    HalStreamConfiguration const& rHalStreams) -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  if (bNeedCallbackCore)
    mCallbackCore->endConfiguration(rHalStreams);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::flushRequest(
    const std::vector<CaptureRequest>& requests) -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  if (bNeedCallbackCore)
    mCallbackCore->flushRequest(requests);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::abortRequest(const android::Vector<Request>& requests)
    -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  std::vector<uint32_t> frameNumbers;
  for (auto const& req : requests) {
    frameNumbers.push_back(req.requestNo);
  }
  if (bNeedCallbackCore)
    mCallbackCore->abortRequest(frameNumbers);
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::submitRequests(
    /*const vector<CaptureRequest>& requests,*/
    const std::vector<CaptureRequest>& rRequests) -> int {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  if (bNeedCallbackCore)
    mCallbackCore->submitRequests(rRequests);
  return 0;
}

/******************************************************************************
 *  need fix
 ******************************************************************************/
auto MtkCallbackAdaptor::updateStreamBuffer(
    uint32_t frameNo,
    StreamId_T streamId,
    std::shared_ptr<IImageStreamBuffer> const pBuffer) -> int {
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::updateResult(UpdateResultParams const& params)
    -> void {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  if (bNeedCallbackCore) {
    ICallbackCore::ResultQueue results;
    mCoreConverter->convertResult(params, results);
    mCallbackCore->updateResult(results);
  } else {
    MtkCallbackParcel mtkCbParcel;
    if (mCoreConverter->convertResult(params, mtkCbParcel)) {
      //  send shutter callbacks
      if (!mtkCbParcel.vNotifyMsg.empty()) {
        if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
            cb->notify(mtkCbParcel.vNotifyMsg);
        } else {
            MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                    mCommonInfo->mDeviceCallback.use_count());
        }
      }
      //  send metadata callbacks
      if (!mtkCbParcel.vCaptureResult.empty()) {
        if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
          cb->processCaptureResult(mtkCbParcel.vCaptureResult);
        } else {
          MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                  mCommonInfo->mDeviceCallback.use_count());
        }
      }
      //  send error callback
      if (!mtkCbParcel.vErrorMsg.empty()) {
        if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
          cb->notify(mtkCbParcel.vErrorMsg);
        } else {
          MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                  mCommonInfo->mDeviceCallback.use_count());
        }
      }
      //  send image callback
      if (!mtkCbParcel.vBufferResult.empty()) {
        if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
          cb->processCaptureResult(mtkCbParcel.vBufferResult);
        } else {
          MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                  mCommonInfo->mDeviceCallback.use_count());
        }
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::waitUntilDrained(nsecs_t const timeout) -> int {
  nsecs_t const startTime = ::systemTime();
  //
  auto timeoutToWait = [=]() {
    nsecs_t const elapsedInterval = (::systemTime() - startTime);
    nsecs_t const timeoutToWait =
        (timeout > elapsedInterval) ? (timeout - elapsedInterval) : 0;
    return timeoutToWait;
  };
  //
  //
  int err = OK;
  Mutex::Autolock _l(mInterfaceLock);

  if (bNeedCallbackCore) {
    (void)((OK == (err = mCallbackCore->waitUntilDrained(timeout))) &&
           (OK == (err = mCallbackHandler->waitUntilDrained(timeoutToWait()))));
  }

  if (OK != err) {
    MY_LOGW("timeout(ns):%" PRId64 " err:%d(%s)", timeout, -err,
            ::strerror(-err));
    struct MyPrinter : public IPrinter {
      std::shared_ptr<::android::FdPrinter> mFdPrinter;
      ULogPrinter mLogPrinter;
      explicit MyPrinter()
          : mFdPrinter(nullptr),
            mLogPrinter(__ULOG_MODULE_ID, LOG_TAG,
                        DetailsType::DETAILS_DEBUG, "[waitUntilDrained] ") {}
      ~MyPrinter() {}
      virtual void printLine(const char* string) {
        mLogPrinter.printLine(string);
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
        return mFdPrinter;
      }
    } printer;
    dumpStateLocked(printer, {});
  }

  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkCallbackAdaptor::markStreamError(uint32_t requestNo,
                                         StreamId_T streamId) -> void {
  //    Mutex::Autolock _l(mInterfaceLock);
  // if (mFrameHandler)
  //   mFrameHandler->updateStreamBuffer(requestNo, streamId, nullptr);
}

/******************************************************************************
 *
 ******************************************************************************/
void MtkCallbackAdaptor::onReceiveCbParcels(
    std::list<ICallbackCore::CallbackParcel>& cbList,
    void* pUser) {
  MtkCallbackAdaptor* pAdaptor = reinterpret_cast<MtkCallbackAdaptor*>(pUser);
  // callback
  auto pConverter = pAdaptor->mCoreConverter;
  auto pCallbackHandler = pAdaptor->mCallbackHandler;
  if (pCallbackHandler)
    pCallbackHandler->push(pConverter->convertCallbackParcel(cbList));
}

};  // namespace core
};  // namespace mcam
