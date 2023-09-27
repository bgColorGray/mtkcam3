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

#include "main/mtkhal/android/device/ACbAdatpor/ACallbackAdaptor.h"
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include "MyUtils.h"

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#if 0
#include <aee.h>
#define AEE_ASSERT(fmt, arg...)                                   \
  do {                                                            \
    String8 const str = String8::format(fmt, ##arg);              \
    CAM_LOGE("ASSERT(%s) fail", str.string());                    \
    aee_system_exception("mtkcam/Metadata", NULL, DB_OPT_DEFAULT, \
                         str.string());                           \
  } while (0)
#else
#define AEE_ASSERT(fmt, arg...)
#endif
//
using ::android::Mutex;
using ::android::OK;
using ::android::String8;
using mcam::core::CoreSetting;
using mcam::core::EnableOption;
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
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
auto IACallbackAdaptor::create(const CreationInfo& creationInfo)
    -> IACallbackAdaptor* {
  auto pInstance = new ACallbackAdaptor(creationInfo);
  if (!pInstance || !pInstance->initialize()) {
    delete pInstance;
    return nullptr;
  }
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
ACallbackAdaptor::ACallbackAdaptor(const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>()),
      mInstanceName{std::to_string(creationInfo.mInstanceId) +
                    ":ACallbackAdaptor"} {
  bIsDejitterEnabled =
      (::property_get_int32("vendor.debug.camera.dejitter.enable", 1) > 0);
  MY_LOGI("bIsDejitterEnabled = %d", bIsDejitterEnabled);
  //
  if (creationInfo.mCameraDeviceCallback == nullptr ||
      creationInfo.mMetadataProvider == nullptr) {
    MY_LOGE("mCameraDeviceCallback:%p mMetadataProvider:%p",
            creationInfo.mCameraDeviceCallback.get(),
            creationInfo.mMetadataProvider.get());
    mCommonInfo = nullptr;
    return;
  }

  IGrallocHelper* pGrallocHelper = IGrallocHelper::singleton();
  if (pGrallocHelper == nullptr) {
    MY_LOGE("IGrallocHelper::singleton(): nullptr");
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
          ::property_get_int32("vendor.debug.camera.log.ACallbackAdaptor", 0);
    }

    mCommonInfo->mLogLevel = loglevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    mCommonInfo->mDeviceCallback = creationInfo.mCameraDeviceCallback;
    mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
    mCommonInfo->mPhysicalMetadataProviders =
        creationInfo.mPhysicalMetadataProviders;
    mCommonInfo->mGrallocHelper = pGrallocHelper;
    mCommonInfo->mAtMostMetaStreamCount = aAtMostMetaStreamCount;mCommonInfo->coreSetting =
        std::make_shared<CoreSetting>(EnableOption::eCORE_MTK_RULE_MASK |
                                      EnableOption::eCORE_AOSP_RULE_MASK);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
bool ACallbackAdaptor::initialize() {
  ::android::status_t status = OK;
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
    mCallbackHandler = new CallbackHandler(mCommonInfo);
    if (mCallbackHandler == nullptr) {
      MY_LOGE("Bad mCallbackHandler");
      return false;
    } else {
      const std::string threadName{std::to_string(mCommonInfo->mInstanceId) +
                                   ":ACbAdp-CbHdl"};
      status = mCallbackHandler->run(threadName.c_str());
      if (OK != status) {
        MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(),
                status, ::strerror(-status));
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
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::destroy() -> void {
  auto resLock = mInterfaceLock.timedLock(::ms2ns(1000));
  if (OK != resLock) {
    MY_LOGW("timedLock failed; still go on to destroy");
  }

  mCallbackCore->destroy();
  mCallbackCore = nullptr;

  mCallbackHandler->destroy();
  mCallbackHandler = nullptr;

  if (OK == resLock) {
    mInterfaceLock.unlock();
  }
}

/******************************************************************************
 *
 ******************************************************************************/
// auto ACallbackAdaptor::dumpState(IPrinter& printer __unused,
//                                    const std::vector<std::string>& options
//                                        __unused) -> void {
//   printer.printFormatLine("## Mtk Callback Adaptor  [%u]\n",
//                           mCommonInfo->mInstanceId);

//   if (OK == mInterfaceLock.timedLock(::ms2ns(500))) {
//     dumpStateLocked(printer, options);

//     mInterfaceLock.unlock();
//   } else {
//     printer.printLine("mInterfaceLock.timedLock timeout");
//   }
// }

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::dumpStateLocked(IPrinter& printer,
                                       const std::vector<std::string>& options)
    -> void {
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
auto ACallbackAdaptor::beginConfiguration(
    const mcam::StreamConfiguration& rStreams) -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  mCallbackCore->beginConfiguration(rStreams);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::endConfiguration(
    const mcam::HalStreamConfiguration& rHalStreams) -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  mCallbackCore->endConfiguration(rHalStreams);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::flushRequest(
    const std::vector<mcam::CaptureRequest>& requests) -> void {
  CAM_ULOGM_FUNCLIFE();
  Mutex::Autolock _l(mInterfaceLock);
  mCallbackCore->flushRequest(requests);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::submitRequests(
    /*const vector<CaptureRequest>& requests,*/
    const std::vector<mcam::CaptureRequest>& rRequests) -> int {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  return mCallbackCore->submitRequests(rRequests);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::waitUntilDrained(int64_t const timeout) -> int {
  int64_t const startTime = ::systemTime();
  //
  auto timeoutToWait = [=]() {
    int64_t const elapsedInterval = (::systemTime() - startTime);
    int64_t const timeoutToWait =
        (timeout > elapsedInterval) ? (timeout - elapsedInterval) : 0;
    return timeoutToWait;
  };
  //
  //
  int err = OK;
  Mutex::Autolock _l(mInterfaceLock);

  (void)((OK == (err = mCallbackCore->waitUntilDrained(timeout))) &&
         (OK == (err = mCallbackHandler->waitUntilDrained(timeoutToWait()))));

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
auto ACallbackAdaptor::processCaptureResult(
    const std::vector<mcam::CaptureResult>& mtkResults) -> void {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  ICallbackCore::ResultQueue results;
  storeSFDejitterSOF(mtkResults);
  mCoreConverter->convertResult(mtkResults, results);
  mCallbackCore->updateResult(results);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::notify(const std::vector<mcam::NotifyMsg>& mtkMsgs)
    -> void {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  ICallbackCore::ResultQueue results;
  mCoreConverter->convertResult(mtkMsgs, results);
  mCallbackCore->updateResult(results);
}

/******************************************************************************
 *
 ******************************************************************************/
void ACallbackAdaptor::onReceiveCbParcels(
    std::list<ICallbackCore::CallbackParcel>& cbList,
    void* pUser) {
  ACallbackAdaptor* pAdaptor = reinterpret_cast<ACallbackAdaptor*>(pUser);
  // apply android specific behavior
  for (auto& rParcel : cbList) {
    pAdaptor->applyGrallocBehavior(rParcel);
  }
  // callback
  auto pConverter = pAdaptor->mCoreConverter;
  auto pCallbackHandler = pAdaptor->mCallbackHandler;
  //
  pCallbackHandler->push(pConverter->convertCallbackParcel(cbList));
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::storeSFDejitterSOF(
    const std::vector<mcam::CaptureResult>& mtkResult) -> void {
  if (bIsDejitterEnabled) {
    Mutex::Autolock _l(mSOFMapLock);
    for (auto const& result : mtkResult) {
      if (result.halResult.count() > 0 &&
          mSOFMap.count(result.frameNumber) <= 0) {
        IMetadata::IEntry const& entry =
            result.halResult.entryFor(MTK_P1NODE_FRAME_START_TIMESTAMP);
        if (!entry.isEmpty()) {
          // store SOF from halResult
          int64_t timestamp = entry.itemAt(0, Type2Type<MINT64>());
          mSOFMap.insert(std::make_pair(result.frameNumber, timestamp));
        }
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACallbackAdaptor::applyGrallocBehavior(
    ICallbackCore::CallbackParcel& parcel) -> bool {
  for (auto const& rCbItem : parcel.vOutputImageItem) {
    uint32_t frameNumber = parcel.frameNumber;
    auto const dataspace = rCbItem.stream->dataSpace;
    auto buffer = rCbItem.buffer;
    auto pAppImageBufferHeap = buffer->buffer;
    mcam::IGraphicImageBufferHeap* pImageBufferHeap = nullptr;
    if (pAppImageBufferHeap != nullptr) {
      pImageBufferHeap =
          mcam::IGraphicImageBufferHeap::castFrom(pAppImageBufferHeap.get());
    }
    // apply surface dejitter
    if (bIsDejitterEnabled) {
      Mutex::Autolock _l(mSOFMapLock);
      bool checkUsage = (rCbItem.stream->usage & GRALLOC_USAGE_HW_TEXTURE) ||
                        (rCbItem.stream->usage & GRALLOC_USAGE_HW_COMPOSER);
      if (mSOFMap.count(frameNumber) > 0 && checkUsage) {
        int rc = IGrallocHelper::singleton()->setBufferSOF(
            *pImageBufferHeap->getBufferHandlePtr(),
            mSOFMap.count(frameNumber));
        if (OK != rc) {
          MY_LOGE("Failed to set SOF:%" PRId64 " for streamId %#" PRIx64
                  "of frameNumber %u",
                  mSOFMap.count(frameNumber), rCbItem.buffer->streamId,
                  frameNumber);
        }
      }
      if (parcel.needRemove)
        mSOFMap.erase(frameNumber);
    }
    // fill camera blob for exif
    bool const isCaptureBlob =
        static_cast<int>(dataspace) ==
            static_cast<int>(HAL_DATASPACE_V0_JFIF) ||
        static_cast<int>(dataspace) ==
            static_cast<int>(HAL_DATASPACE_JPEG_APP_SEGMENTS);
    if (HAL_PIXEL_FORMAT_BLOB ==
            static_cast<android_pixel_format_t>(rCbItem.stream->format) &&
        rCbItem.buffer->status == mcam::BufferStatus::OK &&
        isCaptureBlob) {
      auto buffer = rCbItem.buffer;
      auto pAppImageBufferHeap = buffer->buffer;
      mcam::IGraphicImageBufferHeap* pImageBufferHeap = nullptr;
      if (pAppImageBufferHeap != nullptr) {
        pImageBufferHeap =
            mcam::IGraphicImageBufferHeap::castFrom(pAppImageBufferHeap.get());
      }

      std::string name{std::to_string(mCommonInfo->mInstanceId) + ":ACbAdptor"};

      // [notice]
      // originally AppMgr fill camerablob for the non-BGService case
      // but in android layer the info is not available.
      // Therefore here always set camera blob. Need check if a new camera blob
      // can overwrite the former one.

      // if (pItem->buffer->getUsersManager()->getAllUsersStatus() !=
      //     IUsersManager::UserStatus::PRE_RELEASE) {

      mcam::GrallocStaticInfo staticInfo;
      buffer_handle_t pGraphicBufferHandle =
          pImageBufferHeap->getBufferHandlePtr()
              ? *pImageBufferHeap->getBufferHandlePtr()
              : nullptr;
      if (pGraphicBufferHandle == nullptr) {
        AEE_ASSERT("frameNumber %u, get null buffer handle",
                   parcel.frameNumber);
      }
      int rc = IGrallocHelper::singleton()->query(pGraphicBufferHandle,
                                                  &staticInfo, NULL);
      if (OK == rc && pImageBufferHeap->lockBuf(
                          name.c_str(), GRALLOC_USAGE_SW_WRITE_OFTEN |
                                            GRALLOC_USAGE_SW_READ_OFTEN)) {
        MINTPTR jpegBuf = pImageBufferHeap->getBufVA(0);
        size_t jpegDataSize = pImageBufferHeap->getBitstreamSize();
        size_t jpegBufSize = staticInfo.widthInPixels;
        CameraBlob* pTransport = reinterpret_cast<CameraBlob*>(
            jpegBuf + jpegBufSize - sizeof(CameraBlob));
        if (static_cast<int>(dataspace) ==
            static_cast<int>(HAL_DATASPACE_V0_JFIF))  // For Jpeg capture
          pTransport->blobId = CameraBlobId::JPEG;
        else if (static_cast<int>(dataspace) ==
                 static_cast<int>(
                     HAL_DATASPACE_JPEG_APP_SEGMENTS))  // For Heic capture
          pTransport->blobId = CameraBlobId::JPEG_APP_SEGMENTS;
        pTransport->blobSize = jpegDataSize;
        if (!pImageBufferHeap->unlockBuf(name.c_str())) {
          MY_LOGE("failed on pImageBufferHeap->unlockBuf");
        }
        MY_LOGD("CameraBlob added: bufferId:%" PRIu64 " jpegBuf:%#" PRIxPTR
                " bufsize:%zu datasize:%zu",
                buffer->bufferId, jpegBuf, jpegBufSize, jpegDataSize);
      } else {
        MY_LOGE("Fail to lock jpeg buffer - rc:%d", rc);
      }

      // } else {
      //   MY_LOGD("Pre-release CameraBlob added: bufferId:%" PRIu64 "",
      //           buffer->getBufferId());
      // }
    }
  }
  return true;
}

};  // namespace android
};  // namespace mcam
