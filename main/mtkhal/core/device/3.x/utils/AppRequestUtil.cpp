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

#include "AppRequestUtil.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "AppStreamBufferBuilder.h"
#include "MyUtils.h"
//
//#include "mtkcam-interfaces/hw/camsys/cam_coordinator/ICamCoordinator.h"
#include "mtkcam/drv/IHalSensor.h"
#include "mtkcam3/pipeline/stream/IStreamInfo.h"
#include "mtkcam3/pipeline/stream/StreamId.h"
//

using android::Mutex;
using android::OK;
//using NSCam::EAppendStage;
using NSCam::eImgFmt_BAYER10;
using NSCam::eImgFmt_BAYER10_UNPAK;
using NSCam::eImgFmt_BAYER12_UNPAK;
using NSCam::eImgFmt_BAYER14_UNPAK;
using NSCam::eImgFmt_BAYER15_UNPAK;
using NSCam::eImgFmt_BAYER16_UNPAK;
using NSCam::eImgFmt_BAYER8_UNPAK;
using NSCam::eSTREAMID_BEGIN_OF_PHYSIC_ID;
using NSCam::eSTREAMID_END_OF_FWK;
//using NSCam::IIonDevice;
//using NSCam::IIonDeviceProvider;
using NSCam::ImageDescRawType;
//using NSCam::IMetadataUpdater;
using NSCam::RawBufferInfo;
using NSCam::Type2Type;
//using NSCam::camsys::camcoordinator::CamCoordinatorErrorCode;
//using NSCam::camsys::camcoordinator::DmaSizeInfoInputParams;
//using NSCam::camsys::camcoordinator::DmaSizeInfoMap;
//using NSCam::camsys::camcoordinator::DmaSizeInfoOutputParams;
//using NSCam::camsys::camcoordinator::ICamCoordinator;
//using NSCam::camsys::camcoordinator::SensorInfo;
//using NSCam::camsys::p1dma::FULL_RAW;
using NSCam::v3::STREAM_BUFFER_STATUS;
using NSCam::v3::pipeline::policy::SensorSetting;

#define ThisNamespace AppRequestUtil

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_DEBUG(level, fmt, arg...)                                          \
  do {                                                                        \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,    \
                     ##arg);                                                  \
    mCommonInfo->mDebugPrinter->printFormatLine(                              \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
  } while (0)

#define MY_WARN(level, fmt, arg...)                                           \
  do {                                                                        \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,    \
                     ##arg);                                                  \
    mCommonInfo->mWarningPrinter->printFormatLine(                            \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                                          \
  do {                                                                        \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,    \
                     ##arg);                                                  \
    mCommonInfo->mErrorPrinter->printFormatLine(                              \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
  } while (0)

#define MY_ASSERT(fmt, arg...)                                                \
  do {                                                                        \
    CAM_ULOGM_ASSERT(0, "[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                                  \
    mCommonInfo->mErrorPrinter->printFormatLine(                              \
        " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg);        \
  } while (0)

#define MY_FATAL(fmt, arg...)                                             \
  do {                                                                    \
    CAM_ULOGM_FATAL("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                    ##arg);                                               \
    mCommonInfo->mErrorPrinter->printFormatLine(                          \
        " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg);    \
  } while (0)

#define MY_LOGV(...) MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...) MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...) MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...) MY_WARN(W, __VA_ARGS__)
#define MY_LOGE(...) MY_ERROR(E, __VA_ARGS__)
#define MY_LOGA(...) MY_ASSERT(__VA_ARGS__)
#define MY_LOGF(...) MY_FATAL(__VA_ARGS__)

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
namespace Utils {

/******************************************************************************
 *
 ******************************************************************************/

static void findStreamCropSetting(
              const IMetadata& setting,
              const StreamId_T id,
              std::map<StreamId_T, NSCam::MRect>* outSettings) {
  auto cropEntry = setting.entryFor(MTK_HALCORE_STREAM_CROP_REGION);
  if (cropEntry.count() < 4) {
    return;
  }
  NSCam::MRect crop;
  crop.p.x = cropEntry.itemAt(0, Type2Type<MINT32>());
  crop.p.y = cropEntry.itemAt(1, Type2Type<MINT32>());
  crop.s.w = cropEntry.itemAt(2, Type2Type<MINT32>());
  crop.s.h = cropEntry.itemAt(3, Type2Type<MINT32>());
  outSettings->emplace(id, crop);
}


/******************************************************************************
 *
 ******************************************************************************/
auto IAppRequestUtil::create(const CreationInfo& creationInfo)
    -> std::shared_ptr<IAppRequestUtil> {
  return std::make_shared<AppRequestUtil>(creationInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::AppRequestUtil(const CreationInfo& creationInfo)
    : mInstanceName{std::to_string(creationInfo.mInstanceId) +
                    "-AppRequestUtil"},
      mCommonInfo(std::make_shared<CommonInfo>()) {
  if (creationInfo.mErrorPrinter == nullptr ||
      creationInfo.mWarningPrinter == nullptr ||
      creationInfo.mDebugPrinter == nullptr ||
      creationInfo.mMetadataProvider == nullptr ||
      creationInfo.mMetadataConverter == nullptr) {
    MY_LOGE(
        "mErrorPrinter:%p mWarningPrinter:%p mDebugPrinter:%p "
        "mMetadataProvider:%p mMetadataConverter:%p",
        creationInfo.mErrorPrinter.get(), creationInfo.mWarningPrinter.get(),
        creationInfo.mDebugPrinter.get(), creationInfo.mMetadataProvider.get(),
        creationInfo.mMetadataConverter.get());
    mCommonInfo = nullptr;
    return;
  }

  if (mCommonInfo != nullptr) {
    int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
    if (loglevel == 0) {
      loglevel =
          ::property_get_int32("vendor.debug.camera.log.AppStreamMgr", 0);
    }
    //
    //
    mCommonInfo->mLogLevel = loglevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    mCommonInfo->mErrorPrinter = creationInfo.mErrorPrinter;
    mCommonInfo->mWarningPrinter = creationInfo.mWarningPrinter;
    mCommonInfo->mDebugPrinter = creationInfo.mDebugPrinter;
    mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
    mCommonInfo->mPhysicalMetadataProviders =
        creationInfo.mPhysicalMetadataProviders;
    mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
  }

//  {
//    Mutex::Autolock _l(mMetadataUpdaterLock);
//    auto enableMetaUpdater =
//        ::property_get_int32("vendor.camera.util.enableMetaUpdater", 0);
//    if (enableMetaUpdater) {
//      MY_LOGI("enableMetaUpdater: %d, enable MetadataUpdater",
//              enableMetaUpdater);
//      mMetadataUpdater = IMetadataUpdater::create(IMetadataUpdater::StaticInfo{
//          .mInstanceId = creationInfo.mInstanceId,
//      });
//      if (mMetadataUpdater == nullptr) {
//        MY_LOGW("IMetadataUpdater::create(): nullptr, disable MetadataUpdater");
//      }
//    } else {
//      MY_LOGI("enableMetaUpdater: %d, disable MetadataUpdater",
//              enableMetaUpdater);
//      mMetadataUpdater = nullptr;
//    }
//  }

  MY_LOGD("mImageConfigMap.size()=%lu, mMetaConfigMap.size()=%lu",
          mImageConfigMap.size(), mMetaConfigMap.size());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  // mRequestMetadataQueue = nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::setConfigMap(ImageConfigMap& imageConfigMap,
                                 MetaConfigMap& metaConfigMap) -> void {
  Mutex::Autolock _l(mImageConfigMapLock);
  // reset first
  reset();
  // set Config Map
  mImageConfigMap.clear();
  mMetaConfigMap.clear();
  for (size_t i = 0; i < metaConfigMap.size(); i++) {
    mMetaConfigMap.add(metaConfigMap.keyAt(i), metaConfigMap.valueAt(i));
  }
  for (size_t i = 0; i < imageConfigMap.size(); i++) {
    mImageConfigMap.add(imageConfigMap.keyAt(i), imageConfigMap.valueAt(i));
  }
  MY_LOGD("[debug] mImageConfigMap.size()=%lu, mMetaConfigMap.size()=%lu",
          mImageConfigMap.size(), mMetaConfigMap.size());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::reset() -> void {}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::createRequests(const std::vector<CaptureRequest>& requests,
                                   android::Vector<Request>& rRequests,
                                   const ImageConfigMap* imgCfgMap,
                                   const MetaConfigMap* metaCfgMap) -> int {
  CAM_ULOGM_FUNCLIFE();

  if (requests.size() == 0) {
    MY_LOGE("empty requests list");
    return -EINVAL;
  }

  int err = OK;
  rRequests.resize(requests.size());
  size_t i = 0;
  for (; i < requests.size(); i++) {
    err = checkOneRequest(requests[i]);
    if (OK != err) {
      break;
    }

    // MY_LOGD("[debug] vBufferCache.size()=%zu", vBufferCache.size());
    err = createOneRequest(requests[i], rRequests.editItemAt(i), imgCfgMap,
                           metaCfgMap);
    if (OK != err) {
      break;
    }
  }

  if (OK != err) {
    MY_LOGE("something happened - frameNo:%u request:%zu/%zu",
            requests[i].frameNumber, i, requests.size());
    return err;
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::checkOneRequest(const CaptureRequest& request) const
    -> int {
  //  there are 0 output buffers
  if (request.outputBuffers.size() == 0) {
    MY_LOGE("[frameNo:%u] outputBuffers.size()==0", request.frameNumber);
    return -EINVAL;
  }
  //
  //  not allowed if the fmq size is zero and settings are NULL w/o the lastest
  //  setting.

  if (request.settings.isEmpty()) {
    MY_LOGE("[frameNo:%u] NULL request settings is not reasonable here",
            request.frameNumber);
    return -EINVAL;
  }
  //
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::createOneRequest(const CaptureRequest& request,
                                     Request& rRequest,
                                     const ImageConfigMap* imgCfgMap,
                                     const MetaConfigMap* metaCfgMap) -> int {
  auto queryBufferName = [=](auto const& buffer) -> std::string {
    return ("r" + std::to_string(request.frameNumber) + ":b" +
            std::to_string(buffer.bufferId));
  };

  auto replaceStreamInfo = [&](android::sp<AppImageStreamInfo>& pStreamInfo,
                               IMetadata setting) -> void {
#warning \
    "TODO: prepare customization streaminfo for future requestStreamBuffer call"
    int reqFormat = pStreamInfo->getAllocImgFormat();
    size_t stride = 0;
    if (pStreamInfo->getOriImgFormat() ==
        static_cast<MINT>(NSCam::eImgFmt_RAW16)) {
      RawBufferInfo rawInfo = {0, 0, 0};
      // query info from metadata
      {
        IMetadata::IEntry const& e1 =
            setting.entryFor(MTK_CONTROL_CAPTURE_PACKED_RAW_ENABLE);
        if (!e1.isEmpty()) {
          rawInfo.packedRaw = e1.itemAt(0, Type2Type<MINT32>());
          MY_LOGD("packedRaw: %d", rawInfo.packedRaw);
        }

        IMetadata::IEntry const& e2 =
            setting.entryFor(MTK_CONTROL_CAPTURE_RAW_BPP);
        if (!e2.isEmpty()) {
          rawInfo.bpp = e2.itemAt(0, Type2Type<MINT32>());
          MY_LOGD("bppOfRaw: %d", rawInfo.bpp);
        }

        IMetadata::IEntry const& e3 =
            setting.entryFor(MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE);
        if (!e3.isEmpty()) {
          rawInfo.processed = e3.itemAt(0, Type2Type<MINT32>());
          MY_LOGD("processedRaw: %d", rawInfo.processed);
        }
      }
      if (rawInfo.packedRaw || rawInfo.bpp != 0) {
        if (rawInfo.packedRaw) {
          reqFormat = eImgFmt_BAYER10;
          MSize imgoSize = pStreamInfo->getImgSize();
          std::shared_ptr<NSCamHW::HwInfoHelper> infohelper = std::make_shared<NSCamHW::HwInfoHelper>(0);
          if (OK != infohelper->alignPass1HwLimitation(reqFormat, MTRUE, imgoSize, stride)) {
              MY_LOGE("cannot align to hw limitation");
          }
        } else if (rawInfo.bpp != 0) {
#define ADD_CASE(bitDepth)                       \
  case (bitDepth):                               \
    reqFormat = eImgFmt_BAYER##bitDepth##_UNPAK; \
    break;
          switch (rawInfo.bpp) {
            ADD_CASE(8)
            ADD_CASE(10)
            ADD_CASE(12)
            ADD_CASE(14)
            ADD_CASE(15)
            ADD_CASE(16)
            default:
              MY_LOGE(
                  "unsupported MTK_CONTROL_CAPTURE_RAW_BPP = %d in request "
                  "setting.",
                  rawInfo.bpp);
          }
#undef ADD_CASE
        }
        auto pPackedRawStreamInfo = new AppImageStreamInfo(
            *pStreamInfo, reqFormat, stride);
        pStreamInfo = pPackedRawStreamInfo;  // replace request streamInfo
        MY_LOGD("change raw16 to format:%x, allocFormat:%x stride:%d",
                pStreamInfo->getImgFormat(), pStreamInfo->getAllocImgFormat(),
                stride);
      }
    }
  };

  if (imgCfgMap == nullptr) {
    MY_LOGE("Bad input params");
    return -ENODEV;
  }

  CAM_ULOGM_FUNCLIFE();
  //
  rRequest.requestNo = request.frameNumber;
  //
  //  vInputMetaBuffers <- request.settings
  IMetadata settings = request.settings;
  {
    android::sp<IMetaStreamInfo> pStreamInfo =
        getConfigMetaStream(eSTREAMID_END_OF_FWK, metaCfgMap);
    bool isRepeating = false;
    IMetadata::IEntry const& e1 =
        settings.entryFor(MTK_HALCORE_ISREAPEATING_SETTING);
    if (!e1.isEmpty()) {
      isRepeating = e1.itemAt(0, Type2Type<MUINT8>()) == 1 ? true : false;
    } else {
      MY_LOGE("no tag  MTK_HALCORE_ISREAPEATING_SETTING,");
    }
    //
//    if (!isRepeating) {
//      CAM_ULOGM_TAG_BEGIN("metadataUpdated");
//      // MetadataUpdater
//      if (mMetadataUpdater != nullptr) {
//        Mutex::Autolock _l(mMetadataUpdaterLock);
//        auto status = mMetadataUpdater->appendCustomerMeta(EAppendStage::Device,
//                                                           &settings);
//        if (CC_UNLIKELY(status != OK)) {
//          MY_LOGW("request %d appendCustomerMeta failed", request.frameNumber);
//        }
//      }
//      CAM_ULOGM_TAG_END();
//    }
    //
    android::sp<AppMetaStreamBuffer> pStreamBuffer =
        createMetaStreamBuffer(pStreamInfo, settings, isRepeating);
    if (CC_LIKELY(pStreamBuffer != nullptr &&
                  pStreamBuffer->getStreamInfo() != nullptr)) {
      rRequest.vInputMetaBuffers.add(
          pStreamBuffer->getStreamInfo()->getStreamId(), pStreamBuffer);
    }
  }
  //
  //  vOutputImageBuffers <- request.outputBuffers
  rRequest.vOutputImageBuffers.setCapacity(request.outputBuffers.size());
  for (const auto& buffer : request.outputBuffers) {
    auto pStreamInfo = getConfigImageStream(buffer.streamId, imgCfgMap);
    replaceStreamInfo(pStreamInfo, settings);

    if (buffer.bufferId == 0 && mCommonInfo->mSupportBufferManagement) {
      // android::sp<IImageStreamInfo> pStreamInfo =
      // getConfigImageStream(buffer.streamId);
      rRequest.vOutputImageStreams.add(
        pStreamInfo->getStreamId(), pStreamInfo.get());
    } else {
      android::sp<AppImageStreamBuffer> pStreamBuffer = nullptr;
      BuildImageStreamBufferInputParams const params = {
          .pStreamInfo = pStreamInfo.get(),
          // .bufferName         = "",
          .streamBuffer = buffer,
      };
      pStreamBuffer = makeAppImageStreamBuffer(params);

      if (MY_LIKELY(pStreamBuffer != nullptr &&
                    pStreamBuffer->getStreamInfo() != nullptr)) {
        rRequest.vOutputImageBuffers.add(
            pStreamBuffer->getStreamInfo()->getStreamId(), pStreamBuffer);
        rRequest.vOutputImageStreams.add(
            pStreamBuffer->getStreamInfo()->getStreamId(), pStreamInfo);
      }
    }

    findStreamCropSetting(
      buffer.bufferSettings,
      pStreamInfo->getStreamId(),
      &rRequest.streamCropSettings);

    // update streamImageProcessingSettings
//    uint8_t value;
//    MINT32 setting = MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_UNDEFINED;
//    IMetadata::getEntry<MINT32>(&buffer.bufferSettings,
//                                MTK_HALCORE_IMAGE_PROCESSING_SETTINGS, setting);
//    if (setting & MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY) {
//      value = NSCam::v3::pipeline::model::HIGH_QUALITY;
//    } else {
//      value = NSCam::v3::pipeline::model::FAST;
//    }
//    rRequest.streamImageProcessingSettings.emplace(buffer.streamId, value);
  }
  //
  //  vInputImageBuffers <- request.inputBuffers
  for (auto& inputBuffer : request.inputBuffers) {
    auto pStreamInfo = getConfigImageStream(inputBuffer.streamId, imgCfgMap);
    replaceStreamInfo(pStreamInfo, settings);

    if (inputBuffer.bufferId == 0 && mCommonInfo->mSupportBufferManagement) {
      rRequest.vInputImageStreams.add(inputBuffer.streamId, pStreamInfo.get());
    } else {
      android::sp<AppImageStreamBuffer> pStreamBuffer;
      BuildImageStreamBufferInputParams const params = {
          .pStreamInfo = pStreamInfo.get(),
          .streamBuffer = inputBuffer,
      };
      pStreamBuffer = makeAppImageStreamBuffer(params);
      if (pStreamBuffer == nullptr)
        return -ENODEV;

      if (MY_LIKELY(pStreamBuffer != nullptr &&
                    pStreamBuffer->getStreamInfo() != nullptr)) {
        rRequest.vInputImageBuffers.add(
            pStreamBuffer->getStreamInfo()->getStreamId(), pStreamBuffer);
        rRequest.vInputImageStreams.add(inputBuffer.streamId, pStreamInfo);
      }
    }
    findStreamCropSetting(
      inputBuffer.bufferSettings,
      pStreamInfo->getStreamId(),
      &rRequest.streamCropSettings);
  }
  //
  //  vInputMetaBuffers <- request.physicalCameraSettings
  {
    for (const auto& physicSetting : request.physicalCameraSettings) {
      if (physicSetting.settings.isEmpty()) {
        MY_LOGE("frameNo:%u invalid physical metadata settings!",
                request.frameNumber);
      }
      const IMetadata& physicMeta = physicSetting.settings;

      StreamId_T const streamId = eSTREAMID_BEGIN_OF_PHYSIC_ID +
                                  (int64_t)(physicSetting.physicalCameraId);
      android::sp<IMetaStreamInfo> pMetaStreamInfo =
          getConfigMetaStream(streamId, metaCfgMap);
      if (pMetaStreamInfo != nullptr) {
        android::sp<AppMetaStreamBuffer> pMetaStreamBuffer =
            createMetaStreamBuffer(pMetaStreamInfo, physicMeta,
                                   false /*isRepeating*/);
        rRequest.vInputMetaBuffers.add(pMetaStreamInfo->getStreamId(),
                                       pMetaStreamBuffer);
        //
        MY_LOGD(
            "Duck: create physical meta frameNo:%u physicId:%d "
            "streamId:%#" PRIx64 "",
            request.frameNumber, physicSetting.physicalCameraId, streamId);
      }
      //
      // to debug
      if (getLogLevel() >= 2) {
        MY_LOGI("Dump all physical metadata settings frameNo:%u",
                request.frameNumber);
        mCommonInfo->mMetadataConverter->dumpAll(physicMeta,
                                                 request.frameNumber);
      } else if (getLogLevel() >= 1) {
        MY_LOGI("Dump partial physical metadata settings frameNo:%u",
                request.frameNumber);
        mCommonInfo->mMetadataConverter->dump(physicMeta, request.frameNumber);
      }
    }
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getConfigMetaStream(StreamId_T streamId,
                                        const MetaConfigMap* metaCfgMap) const
    -> android::sp<AppMetaStreamInfo> {
  ssize_t const index = metaCfgMap->indexOfKey(streamId);
  if (0 <= index) {
    return metaCfgMap->valueAt(index);
  } else {
    MY_LOGE("Cannot find MetaStreamInfo for stream %#" PRIx64
            " in mMetaConfigMap",
            streamId);
    return nullptr;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getConfigImageStream(StreamId_T streamId,
                                         const ImageConfigMap* imgCfgMap) const
    -> android::sp<AppImageStreamInfo> {
  Mutex::Autolock _l(mImageConfigMapLock);
  ssize_t const index = imgCfgMap->indexOfKey(streamId);
  if (0 <= index) {
    return imgCfgMap->valueAt(index);
  }
  return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::createMetaStreamBuffer(
    android::sp<IMetaStreamInfo> pStreamInfo,
    IMetadata const& rSettings,
    bool const repeating) const -> AppMetaStreamBuffer* {
  AppMetaStreamBuffer* pStreamBuffer =
      AppMetaStreamBuffer::Allocator(pStreamInfo.get())(rSettings);
  //
  if (MY_LIKELY(pStreamBuffer != nullptr)) {
    pStreamBuffer->setRepeating(repeating);
  }
  //
  return pStreamBuffer;
}
};  // namespace Utils
};  // namespace core
};  // namespace mcam
