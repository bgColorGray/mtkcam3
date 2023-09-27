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

#include <AppConfigUtil.h>

//#include <graphics_mtk_defs.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
//#include <mtkcam-interfaces/pipeline/utils/packutils/PackUtils_v2.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/std/TypeTraits.h>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include "MyUtils.h"

using ::android::Mutex;
using ::android::OK;
using ::android::String8;
using NSCam::DEVICE_FEATURE_SECURE_CAMERA;
using NSCam::eImgFmt_BAYER10;
using NSCam::eImgFmt_BAYER10_UNPAK;
using NSCam::eImgFmt_BAYER12;
using NSCam::eImgFmt_BAYER12_UNPAK;
using NSCam::eImgFmt_BAYER14_UNPAK;
using NSCam::eImgFmt_BAYER15_UNPAK;
using NSCam::eImgFmt_BAYER16_UNPAK;
using NSCam::eImgFmt_BAYER8_UNPAK;
using NSCam::eImgFmt_BLOB;
using NSCam::eImgFmt_ISP_TUING;
using NSCam::eImgFmt_JPEG;
using NSCam::eImgFmt_JPEG_APP_SEGMENT;
using NSCam::eImgFmt_MTK_YUV_P010;
using NSCam::eImgFmt_MTK_YUV_P012;
using NSCam::eImgFmt_MTK_YUV_P210;
using NSCam::eImgFmt_NV12;
using NSCam::eImgFmt_NV21;
using NSCam::eImgFmt_RAW16;
using NSCam::eImgFmt_RGB888;
using NSCam::eImgFmt_Y8;
using NSCam::eImgFmt_YUV_P010;
using NSCam::eImgFmt_YUV_P210_3PLANE;
using NSCam::eImgFmt_YUY2;
using NSCam::eImgFmt_YV12;
using NSCam::eSTREAMID_BEGIN_OF_PHYSIC_ID;
using NSCam::eSTREAMID_END_OF_FWK;
//using NSCam::StreamIdUtils;
using NSCam::Type2Type;
using NSCam::v3::eSTREAMTYPE_META_IN;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
                    CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
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

//static void updateStreamId(
//              // input
//              const mcam::Stream& stream,
//              const int32_t logicalSensorId,
//              // output
//              StreamId_T* Id) {
//  auto& setting = stream.settings;
//  auto dmaEntry = setting.entryFor(MTK_HALCORE_STREAM_SOURCE);
//  if (dmaEntry.isEmpty()) {
//    return;
//  }
//  int32_t dma = dmaEntry.itemAt(0, Type2Type<MINT32>());
//  auto phyId = stream.physicalCameraId;
//  if (phyId == -1) {
//    MY_LOGD("set phyId = logical id(%d)", logicalSensorId);
//    phyId = logicalSensorId;
//  }
//  *Id = StreamIdUtils::getP1StreamIdWithAppSId(
//          static_cast<uint32_t>(phyId),
//          static_cast<int32_t>(dma), static_cast<int32_t>(*Id));
//}
//
//static int32_t mapImageFormat(uint32_t const& format) {
//  switch (format) {
//    case MTK_HAL3PLUS_STREAM_FORMAT_BLOB:
//      return (int32_t)eImgFmt_BLOB;
//    case MTK_HAL3PLUS_STREAM_FORMAT_JPEG:
//      return (int32_t)eImgFmt_JPEG;
//    case MTK_HAL3PLUS_STREAM_FORMAT_RGB_888:
//      return (int32_t)eImgFmt_RGB888;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_3PLANE:
//      return (int32_t)eImgFmt_YUV_P210_3PLANE;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010:
//      return (int32_t)eImgFmt_YUV_P010;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010_PAK:
//      return (int32_t)eImgFmt_MTK_YUV_P010;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P012_PAK:
//      return (int32_t)eImgFmt_MTK_YUV_P012;
//    case MTK_HAL3PLUS_STREAM_FORMAT_NV21:
//      return (int32_t)eImgFmt_NV21;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YV12:
//      return (int32_t)eImgFmt_YV12;
//    case MTK_HAL3PLUS_STREAM_FORMAT_Y8:
//      return (int32_t)eImgFmt_Y8;
//    case MTK_HAL3PLUS_STREAM_FORMAT_NV12:
//      return (int32_t)eImgFmt_NV12;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_PAK:
//      return (int32_t)eImgFmt_MTK_YUV_P210;
//    case MTK_HAL3PLUS_STREAM_FORMAT_YUY2:
//      return (int32_t)eImgFmt_YUY2;
//    case MTK_HAL3PLUS_STREAM_FORMAT_RAW10_PAK:
//      return (int32_t)eImgFmt_BAYER10;
//    case MTK_HAL3PLUS_STREAM_FORMAT_RAW12_PAK:
//      return (int32_t)eImgFmt_BAYER12;
//    case MTK_HAL3PLUS_STREAM_FORMAT_RAW12:
//      return (int32_t)eImgFmt_BAYER12_UNPAK;
//    case MTK_HAL3PLUS_STREAM_FORMAT_RAW16:
//      return (int32_t)eImgFmt_BAYER16_UNPAK;
//    default:
//      MY_LOGW("unsupport format: (%d)", format);
//      return -1;
//  }
//}
//
//static bool updateImgBufferInfo(
//    const std::shared_ptr<AppConfigUtil::StreamSettingsInfoMap>
//        mspStreamSettingsInfoMap,
//    NSCam::ImageBufferInfo& imgBufferInfo,
//    const Stream& rStream) {
//  bool ret = true;
//  if (nullptr == mspStreamSettingsInfoMap) {
//    return false;
//  }
//  auto updateImgFormat = imgBufferInfo.imgFormat;
//  auto updateBufPlanes = imgBufferInfo.bufPlanes;
//  auto updateImgWidth = rStream.width;
//  auto updateImgHeight = rStream.height;
//
//  auto it = mspStreamSettingsInfoMap->find(rStream.id);
//  if (it != mspStreamSettingsInfoMap->end()) {
//    // update image format
//    auto pFormat = it->second->pFormat;
//    if (pFormat) {
//      updateImgFormat = mapImageFormat(*pFormat);
//      MY_LOGD("streamId(%d), update image format: (%d)->(%d)", rStream.id,
//              imgBufferInfo.imgFormat, updateImgFormat);
//    }
//
//    // update image width and height
//    auto pSize = it->second->pSize;
//    if (pSize) {
//      updateImgWidth = pSize->w;
//      updateImgHeight = pSize->h;
//      MY_LOGD("streamId(%d), update image size: (%d,%d)->(%d,%d)", rStream.id,
//              imgBufferInfo.imgWidth, imgBufferInfo.imgHeight, updateImgWidth,
//              updateImgHeight);
//    }
//
//    // update for tuning stream
//    auto pSource = it->second->pSource;
//    if (pSource != nullptr &&
//        *pSource == MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA) {
//      auto mspPackUtil =
//          NSCam::v3::PackUtilV2::IIspTuningDataPackUtil::getInstance();
//      updateBufPlanes.count = 1;
//      updateBufPlanes.planes[0].sizeInBytes =
//          updateBufPlanes.planes[0].rowStrideInBytes = updateImgWidth =
//              mspPackUtil->getPackBufferSize(0);  // input sensor id no used
//
//      updateImgFormat = eImgFmt_ISP_TUING;  // typo: TUNING
//      updateImgHeight = 1;
//
//      MY_LOGD(
//          "streamId(%d) is tuning stream, update image format: (%d)->(%d),"
//          " image (width,height)=(%d,%d)",
//          rStream.id, imgBufferInfo.imgFormat, updateImgFormat, updateImgWidth,
//          updateImgHeight);
//    } else if (updateImgFormat == eImgFmt_BAYER10) {
//      updateBufPlanes.count = 1;
//      //  updateBufPlanes.planes[0].rowStrideInBytes =
//      //      ((updateImgWidth * 10 / 8) + 31) & (~32);
//#define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//      updateBufPlanes.planes[0].rowStrideInBytes =
//          ALIGN_PIXEL(updateImgWidth, 64) * 1.25;
//#undef ALIGN_PIXEL
//
//      MY_LOGD("streamId(%d): aligned stride (by 64) : %d", rStream.id,
//              updateBufPlanes.planes[0].rowStrideInBytes);
//      updateBufPlanes.planes[0].sizeInBytes =
//          updateImgHeight * updateBufPlanes.planes[0].rowStrideInBytes;
//    } else if (updateImgFormat == NSCam::eImgFmt_MTK_YUV_P210 ||
//               updateImgFormat == NSCam::eImgFmt_MTK_YUV_P010) {
//      // update buffer planes info for 10bpp format
//      updateBufPlanes.count =
//          NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//      for (int i = 0; i < updateBufPlanes.count; ++i) {
//#define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//        updateBufPlanes.planes[i].rowStrideInBytes = ALIGN_PIXEL(
//            (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//            updateImgWidth) *
//            NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//            + 7) / 8, 80);
//#undef ALIGN_PIXEL
//        MY_LOGD("streamId(%d): buffer plane[%d] aligned stride (by 80) : %d",
//            rStream.id, i, updateBufPlanes.planes[i].rowStrideInBytes);
//
//        updateBufPlanes.planes[i].sizeInBytes =
//            updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes;
//      }
//    } else if (updateImgFormat == NSCam::eImgFmt_MTK_YUV_P012) {
//      // update buffer planes info for 12bpp format
//      updateBufPlanes.count =
//          NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//      for (int i = 0; i < updateBufPlanes.count; ++i) {
//#define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//        updateBufPlanes.planes[i].rowStrideInBytes = ALIGN_PIXEL(
//            (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//            updateImgWidth) *
//            NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//            + 7) / 8, 96);
//#undef ALIGN_PIXEL
//        MY_LOGD("streamId(%d): buffer plane[%d] aligned stride (by 96) : %d",
//            rStream.id, i, updateBufPlanes.planes[i].rowStrideInBytes);
//
//        updateBufPlanes.planes[i].sizeInBytes =
//            updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes;
//      }
//    } else {
//      // update buffer planes info for other format
//      updateBufPlanes.count =
//          NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//      for (int i = 0; i < updateBufPlanes.count; ++i) {
//        updateBufPlanes.planes[i].rowStrideInBytes =
//            (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//            updateImgWidth) *
//            NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//            + 7) / 8;
//        updateBufPlanes.planes[i].sizeInBytes =
//            updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes;
//      }
//    }
//  } else {
//    MY_LOGD("streamId(%d): no need to update ImgBufferInfo", rStream.id);
//    return false;
//  }
//
//  imgBufferInfo.bufPlanes = updateBufPlanes;
//  imgBufferInfo.imgFormat = updateImgFormat;
//  imgBufferInfo.imgWidth = updateImgWidth;
//  imgBufferInfo.imgHeight = updateImgHeight;
//
//  return ret;
//}

/******************************************************************************
 *
 ******************************************************************************/
auto IAppConfigUtil::create(const CreationInfo& creationInfo)
    -> std::shared_ptr<IAppConfigUtil> {
  return std::make_shared<AppConfigUtil>(creationInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
AppConfigUtil::AppConfigUtil(const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>()),
      mInstanceName{std::to_string(creationInfo.mInstanceId) +
                    ":AppConfigUtil"},
      mStreamInfoBuilderFactory(
          std::make_shared<Utils::Camera3StreamInfoBuilderFactory>()) {
  if (mCommonInfo != nullptr) {
    int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
    if (loglevel == 0) {
      loglevel =
          ::property_get_int32("vendor.debug.camera.log.AppConfigUtil", 0);
    }
    mCommonInfo->mLogLevel = loglevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    if (creationInfo.mPolicyStaticInfo != nullptr) {
      mCommonInfo->mSensorId = creationInfo.mPolicyStaticInfo->sensorId;
      MY_LOGD("mCommonInfo->mSensorId size = %zu",
              mCommonInfo->mSensorId.size());
    }
    if (mCommonInfo->mSensorId.size() == 0) {
      MY_LOGD("mCommonInfo->mSensorId.size() == 0, push instance id");
      mCommonInfo->mSensorId.push_back(creationInfo.mInstanceId);
    }
    mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
    mCommonInfo->mPhysicalMetadataProviders =
        creationInfo.mPhysicalMetadataProviders;
    mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
  }

  mEntryMinDuration =
      mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_SCALER_AVAILABLE_MIN_FRAME_DURATIONS);
  if (mEntryMinDuration.isEmpty()) {
    MY_LOGE("no static MTK_SCALER_AVAILABLE_MIN_FRAME_DURATIONS");
  }
  mEntryStallDuration =
      mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_SCALER_AVAILABLE_STALL_DURATIONS);
  if (mEntryStallDuration.isEmpty()) {
    MY_LOGE("no static MTK_SCALER_AVAILABLE_STALL_DURATIONS");
  }
  MY_LOGD("mImageConfigMap.size()=%lu, mMetaConfigMap.size()=%lu",
          mImageConfigMap.size(), mMetaConfigMap.size());
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::destroy() -> void {
  MY_LOGW("AppConfigUtil::destroy");
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::beginConfigureStreams(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration,
    std::shared_ptr<device::policy::AppUserConfiguration>& rCfgParams)
    -> ::android::status_t {
  auto addFrameDuration = [this](
      auto& rStreams, auto const pStreamInfo, auto const format) {
    for (size_t j = 0; j < mEntryMinDuration.count(); j += 4) {
      if (mEntryMinDuration.itemAt(j, Type2Type<MINT64>()) ==
              (MINT64)format &&
          mEntryMinDuration.itemAt(j + 1, Type2Type<MINT64>()) ==
              (MINT64)pStreamInfo->getLandscapeSize().w &&
          mEntryMinDuration.itemAt(j + 2, Type2Type<MINT64>()) ==
              (MINT64)pStreamInfo->getLandscapeSize().h) {
        rStreams->vMinFrameDuration.emplace(
            pStreamInfo->getStreamId(),
            mspParsedSMVRBatchInfo
                ? 1000000000 / mspParsedSMVRBatchInfo->maxFps
                : mEntryMinDuration.itemAt(j + 3, Type2Type<MINT64>()));

        rStreams->vStallFrameDuration.emplace(
            pStreamInfo->getStreamId(),
            mEntryStallDuration.itemAt(j + 3, Type2Type<MINT64>()));
        MY_LOGI("[addFrameDuration] format:%" PRId64 " size:%" PRId64
                "x%" PRId64 " min_duration:%" PRId64
                ", stall_duration:%" PRId64,
                mEntryMinDuration.itemAt(j, Type2Type<MINT64>()),
                mEntryMinDuration.itemAt(j + 1, Type2Type<MINT64>()),
                mEntryMinDuration.itemAt(j + 2, Type2Type<MINT64>()),
                mEntryMinDuration.itemAt(j + 3, Type2Type<MINT64>()),
                mEntryStallDuration.itemAt(j + 3, Type2Type<MINT64>()));
        return HAL_OK;
      }
    }
    return HAL_UNKNOWN_ERROR;
  };

  rCfgParams->sessionParams = requestedConfiguration.sessionParams;
  {
    StreamId_T const streamId = eSTREAMID_END_OF_FWK;
    std::string const streamName = "Meta:App:Control";
    auto pStreamInfo = createMetaStreamInfo(streamName.c_str(), streamId);
    addConfigStream(pStreamInfo);

    rCfgParams->vMetaStreams.emplace(streamId, pStreamInfo);
  }
  //
  rCfgParams->operationMode =
      static_cast<uint32_t>(requestedConfiguration.operationMode);
  //
  mspParsedSMVRBatchInfo =
      extractSMVRBatchInfo(requestedConfiguration, rCfgParams->sessionParams);
  //
  mE2EHDROn =
      extractE2EHDRInfo(requestedConfiguration, rCfgParams->sessionParams);
  //
//  mspStreamSettingsInfoMap =
//      extractStreamSettingsInfoMap(rCfgParams->sessionParams);

  mDngFormat = extractDNGFormatInfo(requestedConfiguration);
  //
  halConfiguration.streams.resize(requestedConfiguration.streams.size());
  // rCfgParams->vImageStreams.setCapacity(requestedConfiguration.streams.size());
  for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
    const auto& srcStream = requestedConfiguration.streams[i];
    auto& dstStream = halConfiguration.streams[i];
    StreamId_T streamId = srcStream.id;

//    updateStreamId(srcStream, mCommonInfo->mSensorId[0], &streamId);
//
//    // update streamId by add physical device id from streamSettingsInfo
//    if (mspStreamSettingsInfoMap) {
//      auto it = mspStreamSettingsInfoMap->find(streamId);
//      if (it != mspStreamSettingsInfoMap->end()) {
//        auto pPhysicalDeviceId = it->second->pPhysicalDeviceId;
//        auto pSource = it->second->pSource;
//        if ((nullptr != pPhysicalDeviceId) && (nullptr != pSource)) {
//          streamId = StreamIdUtils::getP1StreamIdWithAppSId(
//              static_cast<uint32_t>(*pPhysicalDeviceId),
//              static_cast<int32_t>(*pSource), static_cast<int32_t>(streamId));
//          MY_LOGD("append streamId(%#" PRIx64 ")->(%#" PRIx64 ")", srcStream.id,
//                  streamId);
//        }
//      }
//    }
    //
    android::sp<AppImageStreamInfo> pStreamInfo = getConfigImageStream(streamId);
    if (pStreamInfo == nullptr) {
      pStreamInfo = createImageStreamInfo(srcStream, dstStream, streamId);
      if (pStreamInfo == nullptr) {
        MY_LOGE("createImageStreamInfo failed - Stream=%s",
                toString(srcStream).c_str());
        return -ENODEV;
      }
      addConfigStream(pStreamInfo.get() /*, false*/ /*keepBufferCache*/);
    } else {
      auto generateLogString = [=]() {
        return String8::format(
            "streamId:%d type(%d:%d) "
            "size(%dx%d:%dx%d) format(%d:%d) dataSpace(%d:%d) "
            "usage(%#" PRIx64 ":%#" PRIx64 ")",
            srcStream.id, pStreamInfo->getStream().streamType,
            srcStream.streamType, pStreamInfo->getStream().width,
            pStreamInfo->getStream().height, srcStream.width, srcStream.height,
            pStreamInfo->getStream().format, srcStream.format,
            pStreamInfo->getStream().dataSpace, srcStream.dataSpace,
            pStreamInfo->getStream().usage, srcStream.usage);
      };

      MY_LOGI("stream re-configuration: %s", generateLogString().c_str());

      // refer to google default wrapper implementation:
      // width/height/format must not change, but usage/rotation might need to
      // change
      bool check1 =
          (srcStream.streamType == pStreamInfo->getStream().streamType &&
           srcStream.width == pStreamInfo->getStream().width &&
           srcStream.height == pStreamInfo->getStream().height);
      // && srcStream.dataSpace  == pStreamInfo->getStream().dataSpace
      bool check2 =
          true || (srcStream.format == pStreamInfo->getStream().format ||
                   (srcStream.format == pStreamInfo->getImgFormat() &&
                    NSCam::eImgFmt_IMPLEMENTATION_DEFINED ==
                        pStreamInfo->getStream().format));
      // ||(pStreamInfo->getStream().format == real format of
      // srcStream.format &&
      //   NSCam::eImgFmt_IMPLEMENTATION_DEFINED == srcStream.format)
      if (!check1 || !check2) {
        MY_LOGE("stream configuration changed! %s",
                generateLogString().c_str());
        return -ENODEV;
      }

      // If usage is chaged, it implies that
      // the real format (flexible yuv/implementation_defined)
      // and the buffer layout may change.
      // In this case, should HAL and Frameworks have to cleanup the buffer
      // handle cache?
      if (pStreamInfo->getStream().usage != srcStream.usage) {
        MY_LOGW("stream usage changed! %s", generateLogString().c_str());
        MY_LOGW("shall HAL and Frameworks have to clear buffer handle cache?");
      }

      // Create a new stream to override the old one, since usage/rotation might
      // need to change.
      pStreamInfo = createImageStreamInfo(srcStream, dstStream, streamId);
      if (pStreamInfo == nullptr) {
        MY_LOGE("createImageStreamInfo failed - Stream=%s",
                toString(srcStream).c_str());
        return -ENODEV;
      }
      addConfigStream(pStreamInfo.get() /*, true*/ /*keepBufferCache*/);
    }

    // check hidl_stream contain physic id or not.
    if (srcStream.physicalCameraId != -1) {
      auto& sensorList = rCfgParams->vPhysicCameras;
      auto& pcId = srcStream.physicalCameraId;
      MY_LOGD("pcid(%d)", pcId);
      auto iter = std::find(sensorList.begin(), sensorList.end(), pcId);
      if (iter == sensorList.end()) {
        sensorList.push_back(pcId);
        // physical settings
        String8 const streamName =
            String8::format("Meta:App:Physical_%d", pcId);
        StreamId_T const streamId =
            eSTREAMID_BEGIN_OF_PHYSIC_ID + (int64_t)pcId;
        auto pMetaStreamInfo =
            createMetaStreamInfo(streamName.c_str(), streamId, pcId);
        addConfigStream(pMetaStreamInfo);
        rCfgParams->vMetaStreams_physical.emplace(pcId, pMetaStreamInfo);
      }
    }

    rCfgParams->vImageStreams.emplace(streamId, pStreamInfo);
    if (addFrameDuration(rCfgParams,
                         pStreamInfo,
                         pStreamInfo->getOriImgFormat()) != HAL_OK) {
      // The image format is overrided previously,
      // use IMPLEMENTATION_DEFINED to search again.
      addFrameDuration(rCfgParams,
                       pStreamInfo,
                       NSCam::eImgFmt_IMPLEMENTATION_DEFINED);
    }

//    // update streamImageProcessingSettings
//    if (srcStream.streamType == StreamType::OUTPUT) {
//      uint8_t value;
//      MINT32 setting = MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_UNDEFINED;
//      IMetadata::getEntry<MINT32>(&srcStream.settings,
//              MTK_HALCORE_IMAGE_PROCESSING_SETTINGS, setting);
//      if (setting != MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_UNDEFINED) {
//        if (setting & MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY) {
//          value |= NSCam::v3::pipeline::model::HIGH_QUALITY;
//        }
//        if (setting & MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_FAST) {
//           value |= NSCam::v3::pipeline::model::FAST;
//        }
//      } else {
//        value = NSCam::v3::pipeline::model::FAST;
//      }
//      rCfgParams->streamImageProcessingSettings.emplace(streamId, value);
//    }

    MY_LOGD_IF(getLogLevel() >= 2,
               "Stream: id:%d streamType:%d %dx%d format:0x%x usage:0x%" PRIx64
               " dataSpace:0x%x transform:%d",
               srcStream.id, srcStream.streamType, srcStream.width,
               srcStream.height, srcStream.format, srcStream.usage,
               srcStream.dataSpace, srcStream.transform);
  }
  rCfgParams->pStreamInfoBuilderFactory = getAppStreamInfoBuilderFactory();
  //
  //  remove unused config stream
  //  Hal3+ modify : int32_t
  std::unordered_set<int32_t> usedStreamIds;
  usedStreamIds.reserve(halConfiguration.streams.size());
  for (size_t i = 0; i < halConfiguration.streams.size(); i++) {
    usedStreamIds.insert(halConfiguration.streams[i].id);
  }
  {
    Mutex::Autolock _l(mImageConfigMapLock);
    for (ssize_t i = mImageConfigMap.size() - 1; i >= 0; i--) {
      int32_t const streamId = mImageConfigMap.keyAt(i);
      auto const it = usedStreamIds.find(streamId);
      if (it == usedStreamIds.end()) {
        //  remove unsued stream
        MY_LOGD("remove unused ImageStreamInfo, streamId:%#" PRIx64 "",
                streamId);
        mImageConfigMap.removeItemsAt(i);
      }
    }
  }
  //
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::createImageStreamInfo(const Stream& rStream,
                                          HalStream& rOutStream,
                                          const StreamId_T rStreamId) const
    -> AppImageStreamInfo* {
  const bool isSMVRBatchStream =
      mspParsedSMVRBatchInfo != nullptr &&
      mspParsedSMVRBatchInfo->opMode != 1 &&
      rStream.usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER;
  const bool isSecureFlow =
      ((rStream.usage & NSCam::eBUFFER_USAGE_SW_READ_MASK) ==
       NSCam::eBUFFER_USAGE_SW_READ_NEVER) &&
      ((rStream.usage & NSCam::eBUFFER_USAGE_SW_WRITE_MASK) ==
       NSCam::eBUFFER_USAGE_SW_WRITE_NEVER);
  //  stream name = s<stream id>:d<device id>:App:<format>:<hal client usage>
  String8 s8StreamName =
      String8::format("s%d:d%d:App:", rStream.id, mCommonInfo->mInstanceId);
  String8 s8FormatAllocated = android::String8::format(
      "%s", queryImageFormatName(rStream.format).c_str());
  s8StreamName += s8FormatAllocated;
  MY_LOGD("rStream.format=%d", rStream.format);
  s8StreamName += ":";
  s8StreamName += android::String8::format(
      "%s", queryBufferUsageName(rStream.usage).c_str());

  auto& bufPlanes = rStream.bufPlanes;
  //
  rOutStream.id = rStream.id;
  rOutStream.physicalCameraId = rStream.physicalCameraId;
  rOutStream.overrideFormat = rStream.format;
  rOutStream.producerUsage =
      (rStream.streamType == StreamType::OUTPUT) ? rStream.usage : 0;
  rOutStream.consumerUsage =
      (rStream.streamType == StreamType::OUTPUT) ? 0 : rStream.usage;
  rOutStream.maxBuffers = 1;
  rOutStream.overrideDataSpace = rStream.dataSpace;

  MINT imgFormat = rStream.format;
  switch (rStream.format) {
  case NSCam::eImgFmt_BLOB:
    if (rStream.dataSpace == Dataspace::V0_JFIF) {
      imgFormat = eImgFmt_JPEG;
    } else if (rStream.dataSpace == Dataspace::JPEG_APP_SEGMENTS) {
      imgFormat = eImgFmt_JPEG_APP_SEGMENT;
    }
    break;
  case NSCam::eImgFmt_RAW16:
    imgFormat = mDngFormat;
    break;
  case NSCam::eImgFmt_RAW10:
    imgFormat = NSCam::eImgFmt_BAYER10_MIPI;
    break;
  default:
    // not to override
    break;
  }
  rOutStream.overrideFormat = static_cast<NSCam::EImageFormat>(imgFormat);

  MINT allocImgFormat = static_cast<MINT>(rStream.format);
  IImageStreamInfo::BufPlanes_t allocBufPlanes = bufPlanes;

  // SMVRBatch:: handle blob layout
  uint32_t oneImgTotalSizeInBytes_32align = 0;
  if (isSMVRBatchStream) {
    uint32_t oneImgTotalSizeInBytes = 0;
    uint32_t oneImgTotalStrideInBytes = 0;

    allocBufPlanes.count = 1;
    allocImgFormat = static_cast<MINT>(eImgFmt_BLOB);  // for smvr-batch mode

    for (size_t i = 0; i < bufPlanes.count; i++) {
      MY_LOGD("SMVRBatch: idx=%zu, (sizeInBytes, rowStrideInBytes)=(%zu,%zu)",
              i, bufPlanes.planes[i].sizeInBytes,
              bufPlanes.planes[i].rowStrideInBytes);
      oneImgTotalSizeInBytes += bufPlanes.planes[i].sizeInBytes;
      // oneImgTotalStrideInBytes += bufPlanes.planes[i].rowStrideInBytes;
      oneImgTotalStrideInBytes = oneImgTotalSizeInBytes;
    }
    oneImgTotalSizeInBytes_32align = (((oneImgTotalSizeInBytes - 1) >> 5) + 1)
                                     << 5;
    allocBufPlanes.planes[0].sizeInBytes =
        oneImgTotalSizeInBytes_32align * mspParsedSMVRBatchInfo->p2BatchNum;
    allocBufPlanes.planes[0].rowStrideInBytes =
        allocBufPlanes.planes[0].sizeInBytes;

    // debug message
    MY_LOGD_IF(
        mspParsedSMVRBatchInfo->logLevel >= 2,
        "SMVRBatch: %s: isVideo=%" PRIu64
        " \n"
        "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%#" PRIx64
        ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
        "\t HalStream-info(producerUsage= %#" PRIx64
        ",  consumerUsage= %#" PRIx64
        ", overrideFormat=0x%x ) \n"
        "\t Blob-info(imgFmt=0x%x, allocImgFmt=0x%x, vOutBurstNum=%d, "
        "oneImgTotalSizeInBytes(%d, 32align-%d ), "
        "oneImgTotalStrideInBytes=%d, allocBufPlanes(size=%zu, "
        "sizeInBytes=%zu, rowStrideInBytes=%zu)) \n",
        __FUNCTION__, (rStream.usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER),
        rStream.id, rStream.format, rStream.usage, StreamType::OUTPUT,
        rStream.streamType, rOutStream.producerUsage, rOutStream.consumerUsage,
        rOutStream.overrideFormat, imgFormat, allocImgFormat,
        mspParsedSMVRBatchInfo->p2BatchNum, oneImgTotalSizeInBytes,
        oneImgTotalSizeInBytes_32align, oneImgTotalStrideInBytes,
        allocBufPlanes.count, allocBufPlanes.planes[0].sizeInBytes,
        allocBufPlanes.planes[0].rowStrideInBytes);
  } else if (rStream.format == NSCam::eImgFmt_RAW16 ||
             rStream.format == NSCam::eImgFmt_RAW10) {
    // Alloc Format: Raw -> Blob
    allocBufPlanes.count = 1;
    allocImgFormat = static_cast<MINT>(eImgFmt_BLOB);
    allocBufPlanes.planes[0].sizeInBytes = bufPlanes.planes[0].sizeInBytes;
    allocBufPlanes.planes[0].rowStrideInBytes = bufPlanes.planes[0].sizeInBytes;
    MY_LOGD("Raw: (sizeInBytes, rowStrideInBytes)=(%zu,%zu)",
            allocBufPlanes.planes[0].sizeInBytes,
            allocBufPlanes.planes[0].rowStrideInBytes);
  }

  NSCam::ImageBufferInfo imgBufferInfo;
  imgBufferInfo.count = 1;
  imgBufferInfo.bufStep = 0;
  imgBufferInfo.startOffset = 0;
  // SMVRBatch:: buffer offset setting
  if (isSMVRBatchStream) {
    imgBufferInfo.count = mspParsedSMVRBatchInfo->p2BatchNum;
    imgBufferInfo.bufStep = oneImgTotalSizeInBytes_32align;
  }

  // !!NOTES: bufPlanes, imgFormat should be maintained as original format, ref:
  // Camera3ImageStreamInfo.cpp
  imgBufferInfo.bufPlanes = bufPlanes;
  imgBufferInfo.imgFormat = imgFormat;

  imgBufferInfo.imgWidth = rStream.width;
  imgBufferInfo.imgHeight = rStream.height;

//  // update imgBufferInfo by streamSettingsInfo => move to ACameraDeviceSession
//  if (/*mspStreamSettingsInfoMap*/ false) {
//    if (!updateImgBufferInfo(mspStreamSettingsInfoMap, imgBufferInfo,
//                             rStream)) {
//      MY_LOGD("streamId(%d), update imgBufferInfo failed", rStream.id);
//    } else {
//      allocBufPlanes.count = 1;
//      allocImgFormat = static_cast<MINT>(eImgFmt_BLOB);
//      allocBufPlanes.planes[0].rowStrideInBytes = 0;
//      allocBufPlanes.planes[0].sizeInBytes = 0;
//      for (size_t idx = 0; idx < imgBufferInfo.bufPlanes.count; idx++) {
//        allocBufPlanes.planes[0].sizeInBytes +=
//            imgBufferInfo.bufPlanes.planes[idx].sizeInBytes;
//      }
//      allocBufPlanes.planes[0].rowStrideInBytes =
//          allocBufPlanes.planes[0].sizeInBytes;
//      MY_LOGD("stride : %d, size : %d",
//              allocBufPlanes.planes[0].rowStrideInBytes,
//              allocBufPlanes.planes[0].sizeInBytes);
//    }
//  }

  auto const& pStreamInfo = getConfigImageStream(rStream.id);

  AppImageStreamInfo::CreationInfo creationInfo = {
      .mStreamName = s8StreamName,
      .mImgFormat = allocImgFormat,  // alloc stage, TBD if it's YUV format for
                                     // batch mode SMVR
      .mOriImgFormat = (pStreamInfo.get()) ? pStreamInfo->getOriImgFormat()
                                           : rStream.format,
      .mRealAllocFormat = rStream.format,
      .mStream = rStream,
      .mStreamId = rStreamId,
      .mHalStream = rOutStream,
      .mImageBufferInfo = imgBufferInfo,
      .mSensorId = static_cast<MINT>(rStream.physicalCameraId),
      .mSecureInfo = SecureInfo{isSecureFlow ? NSCam::SecType::mem_protected
                                             : NSCam::SecType::mem_normal}};
  /* alloc stage, TBD if it's YUV format for batch mode SMVR */
  creationInfo.mvbufPlanes = allocBufPlanes;

  // fill in the secure info if and only if it's a secure camera device
  // debug message
  MY_LOGD(
      "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%" PRIx64
      ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
      "\t HalStream-info(producerUsage= %#" PRIx64 ",  consumerUsage= %#" PRIx64
      ", overrideFormat=0x%x ) \n"
      "\t Blob-info(imgFmt=0x%x, allocImgFmt=0x%x, "
      "allocBufPlanes(size=%zu, sizeInBytes=%zu, rowStrideInBytes=%zu))\n"
      "\t creationInfo(secureInfo=0x%x)\n",
      rStream.id, rStream.format, rStream.usage,
      StreamType::OUTPUT, rStream.streamType, rOutStream.producerUsage,
      rOutStream.consumerUsage, rOutStream.overrideFormat, imgFormat,
      allocImgFormat, allocBufPlanes.count,
      allocBufPlanes.planes[0].sizeInBytes,
      allocBufPlanes.planes[0].rowStrideInBytes,
      toLiteral(creationInfo.mSecureInfo.type));

  return new AppImageStreamInfo(creationInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::createMetaStreamInfo(char const* metaName,
                                         StreamId_T suggestedStreamId,
                                         MINT physicalId) const
    -> AppMetaStreamInfo* {
    return new AppMetaStreamInfo(
        metaName,
        suggestedStreamId,
        eSTREAMTYPE_META_IN,
        0,
        0,
        physicalId
    );
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::extractSMVRBatchInfo(
    const StreamConfiguration& requestedConfiguration,
    const IMetadata& sessionParams)
    -> std::shared_ptr<AppConfigUtil::ParsedSMVRBatchInfo> {
  const MUINT32 operationMode =
      (const MUINT32)requestedConfiguration.operationMode;

  bool bEnableBatch = EANBLE_SMVR_BATCH == 1;
  MINT32 customP2BatchNum = 1;
  MINT32 p2IspBatchNum = 1;
  std::shared_ptr<ParsedSMVRBatchInfo> pParsedSMVRBatchInfo = nullptr;
  int isFromApMeta = 0;

  IMetadata::IEntry const& entry =
      sessionParams.entryFor(MTK_SMVR_FEATURE_SMVR_MODE);

  if (getLogLevel() >= 2) {
    MY_LOGD(
        "SMVRBatch: chk sessionParams.count()=%d, "
        "MTK_SMVR_FEATURE_SMVR_MODE: count: %d",
        sessionParams.count(), entry.count());
    const_cast<IMetadata&>(sessionParams).dump();
  }
  if (bEnableBatch && operationMode == 1) {
    pParsedSMVRBatchInfo = std::make_shared<ParsedSMVRBatchInfo>();
    if (MY_UNLIKELY(pParsedSMVRBatchInfo == nullptr)) {
      CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>", __FUNCTION__);
      return nullptr;
    }
    pParsedSMVRBatchInfo->opMode = 1;
    pParsedSMVRBatchInfo->logLevel =
        ::property_get_int32("vendor.debug.smvrb.loglevel", 0);

    // get image w/h
    for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
      const auto& srcStream = requestedConfiguration.streams[i];
      // if (usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER)
      if (pParsedSMVRBatchInfo->imgW == 0) {
        // !!NOTES: assume previewSize = videoOutSize found
        pParsedSMVRBatchInfo->imgW = srcStream.width;
        pParsedSMVRBatchInfo->imgH = srcStream.height;
        // break;
      }
      MY_LOGD("SMVR: vImageStreams[%zu]=%dx%d, isVideo=%" PRIu64 "", i,
              srcStream.width, srcStream.height,
              (srcStream.usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER));
    }

    auto entryHighSpeed =
        mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS);
    MINT32 iBurstNum = 0;
    if (!entryHighSpeed.isEmpty()) {
      auto& w = pParsedSMVRBatchInfo->imgW;
      auto& h = pParsedSMVRBatchInfo->imgH;
      for (size_t i = 0; i < entryHighSpeed.count(); i += 5) {
        if (w == entry.itemAt(i, Type2Type<int32_t>()) &&
            h == entry.itemAt(i + 1, Type2Type<int32_t>())) {
          iBurstNum = entry.itemAt(i + 4, Type2Type<MINT32>());
          break;
        }
      }
    }
    if (iBurstNum <= 0) {
      iBurstNum = 4;
    }
    pParsedSMVRBatchInfo->maxFps = iBurstNum * 30;
    pParsedSMVRBatchInfo->p2BatchNum = iBurstNum;
    pParsedSMVRBatchInfo->p1BatchNum = iBurstNum;
  } else if (!entry.isEmpty() && entry.count() >= 2) {
    isFromApMeta = 1;
    pParsedSMVRBatchInfo = std::make_shared<ParsedSMVRBatchInfo>();
    if (MY_UNLIKELY(pParsedSMVRBatchInfo == nullptr)) {
      CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>", __FUNCTION__);
      return nullptr;
    }
    // get image w/h
    for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
      const auto& srcStream = requestedConfiguration.streams[i];
      // if (usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER)
      if (pParsedSMVRBatchInfo->imgW == 0) {
        // !!NOTES: assume previewSize = videoOutSize found
        pParsedSMVRBatchInfo->imgW = srcStream.width;
        pParsedSMVRBatchInfo->imgH = srcStream.height;
        // break;
      }
      MY_LOGD("SMVRBatch: vImageStreams[%zu]=%dx%d, isVideo=%" PRIu64 "", i,
              srcStream.width, srcStream.height,
              (srcStream.usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER));
    }

    if (!entry.isEmpty() && entry.count() >= 2) {
      pParsedSMVRBatchInfo->maxFps =
          entry.itemAt(0, Type2Type<MINT32>());  // meta[0]: LmaxFps
      customP2BatchNum =
          entry.itemAt(1, Type2Type<MINT32>());  // meta[1]: customP2BatchNum
    }
    pParsedSMVRBatchInfo->maxFps = ::property_get_int32(
        "vendor.debug.smvrb.maxFps", pParsedSMVRBatchInfo->maxFps);
    // if (pParsedSMVRBatchInfo->maxFps <= 120) {
    //   MY_LOGE("SMVRBatch: !!err: only support slow motion more than "
    //       "120fps: curr-maxFps=%d", pParsedSMVRBatchInfo->maxFps);
    //   return nullptr;
    // }
    // determine final P2BatchNum
#define min(a, b) ((a) < (b) ? (a) : (b))
    MUINT32 vOutSize = pParsedSMVRBatchInfo->imgW * pParsedSMVRBatchInfo->imgH;
    if (vOutSize <= 640 * 480) {  // vga
      p2IspBatchNum = ::property_get_int32("ro.vendor.smvr.p2batch.vga", 1);
    } else if (vOutSize <= 1280 * 736) {  // hd
      p2IspBatchNum = ::property_get_int32("ro.vendor.smvr.p2batch.hd", 1);
    } else if (vOutSize <= 1920 * 1088) {  // fhd
      p2IspBatchNum = ::property_get_int32("ro.vendor.smvr.p2batch.fhd", 1);
    } else {
      p2IspBatchNum = 1;
    }
    // change p2IspBatchNum by debug adb if necessary
    p2IspBatchNum =
        ::property_get_int32("vendor.debug.smvrb.P2BatchNum", p2IspBatchNum);
    // final P2BatchNum
    pParsedSMVRBatchInfo->p2BatchNum = min(p2IspBatchNum, customP2BatchNum);
#undef min

    // P1BatchNum
    pParsedSMVRBatchInfo->p1BatchNum = pParsedSMVRBatchInfo->maxFps / 30;
    // operatioin mode
    pParsedSMVRBatchInfo->opMode = operationMode;

    // log level
    pParsedSMVRBatchInfo->logLevel =
        ::property_get_int32("vendor.debug.smvrb.loglevel", 0);
  } else {
    MINT32 propSmvrBatchEnable =
        ::property_get_int32("vendor.debug.smvrb.enable", 0);
    if (propSmvrBatchEnable) {
      pParsedSMVRBatchInfo = std::make_shared<ParsedSMVRBatchInfo>();
      if (MY_UNLIKELY(pParsedSMVRBatchInfo == nullptr)) {
        CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>",
                 __FUNCTION__);
        return nullptr;
      }

      // get image w/h
      for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
        const auto& srcStream = requestedConfiguration.streams[i];
        // if (usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER)
        if (pParsedSMVRBatchInfo->imgW == 0) {
          // !!NOTES: assume previewSize = videoOutSize found
          pParsedSMVRBatchInfo->imgW = srcStream.width;
          pParsedSMVRBatchInfo->imgH = srcStream.height;
          // break;
        }
        MY_LOGD("SMVRBatch: vImageStreams[%lu]=%dx%d, isVideo=%" PRIu64 "", i,
                srcStream.width, srcStream.height,
                (srcStream.usage & NSCam::eBUFFER_USAGE_HW_VIDEO_ENCODER));
      }

      pParsedSMVRBatchInfo->maxFps = ::property_get_int32(
          "vendor.debug.smvrb.maxFps", pParsedSMVRBatchInfo->maxFps);
      // if (pParsedSMVRBatchInfo->maxFps <= 120) {
      //   MY_LOGE("SMVRBatch: !!err: only support slow motion more than "
      //       "120fps: curr-maxFps=%d",
      //       pParsedSMVRBatchInfo->maxFps); return nullptr;
      // }

      pParsedSMVRBatchInfo->p2BatchNum =
          ::property_get_int32("vendor.debug.smvrb.P2BatchNum", 1);
      pParsedSMVRBatchInfo->p1BatchNum =
          ::property_get_int32("vendor.debug.smvrb.P1BatchNum", 1);
      pParsedSMVRBatchInfo->opMode = operationMode;
      pParsedSMVRBatchInfo->logLevel =
          ::property_get_int32("vendor.debug.smvrb.loglevel", 0);
    }
  }

  if (pParsedSMVRBatchInfo != nullptr) {
    MY_LOGD(
        "SMVRBatch: isFromApMeta=%d, vOutImg=%dx%d, meta-info(maxFps=%d, "
        "customP2BatchNum=%d), p2IspBatchNum=%d, final-P2BatchNum=%d, "
        "p1BatchNum=%d, opMode=%d, logLevel=%d",
        isFromApMeta, pParsedSMVRBatchInfo->imgW, pParsedSMVRBatchInfo->imgH,
        pParsedSMVRBatchInfo->maxFps, customP2BatchNum, p2IspBatchNum,
        pParsedSMVRBatchInfo->p2BatchNum, pParsedSMVRBatchInfo->p1BatchNum,
        pParsedSMVRBatchInfo->opMode, pParsedSMVRBatchInfo->logLevel);

  } else {
    MY_LOGD("SMVRBatch: no need");
  }

  return pParsedSMVRBatchInfo;
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::extractE2EHDRInfo(
    const StreamConfiguration& requestedConfiguration,
    const IMetadata& sessionParams) -> MINT32 {
  MINT32 iEnableE2EHDR = 0;
  IMetadata::IEntry const& entry =
      sessionParams.entryFor(MTK_STREAMING_FEATURE_HDR10);
  if (entry.isEmpty()) {
    MY_LOGW(
        "query tag MTK_STREAMING_FEATURE_HDR10 failed, E2E HDR is not applied");
  } else {
    iEnableE2EHDR = entry.itemAt(0, Type2Type<MINT32>());
  }
  return iEnableE2EHDR;
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::extractDNGFormatInfo(
    const StreamConfiguration& requestedConfiguration) -> MINT {
  IMetadata const& metaSessionParams = requestedConfiguration.sessionParams;
  MINT32 dngBit = 0;
  MINT imageFormat = eImgFmt_UNKNOWN;
  // todo: confirm these two metadata tags
  IMetadata::IEntry const& vendorTagEntry =
      metaSessionParams.entryFor(MTK_SENSOR_INFO_WHITE_LEVEL);
  IMetadata::IEntry const& staticMetaEntry =
      mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_SENSOR_INFO_WHITE_LEVEL);
  if (!vendorTagEntry.isEmpty()) {
    dngBit = vendorTagEntry.itemAt(0, Type2Type<MINT32>());
    MY_LOGD("found DNG bit size = %d in vendor tag", dngBit);
  } else if (!staticMetaEntry.isEmpty()) {
    dngBit = staticMetaEntry.itemAt(0, Type2Type<MINT32>());
    MY_LOGD("found DNG bit size = %d in static meta", dngBit);
  } else {
    MY_LOGD("no need to overwrite stream format for DNG");
  }
  //
#define ADD_CASE(bitDepth)                         \
  case ((1 << bitDepth) - 1):                      \
    imageFormat = eImgFmt_BAYER##bitDepth##_UNPAK; \
    break;
  //
  switch (dngBit) {
    ADD_CASE(8)
    ADD_CASE(10)
    ADD_CASE(12)
    ADD_CASE(14)
    ADD_CASE(15)
    ADD_CASE(16)
    default:
      MY_LOGE("unsupported DNG bit size = %d in metadata.", dngBit);
  }
#undef ADD_CASE
  return imageFormat;
}

/******************************************************************************
 *
 ******************************************************************************/
//auto AppConfigUtil::extractStreamSettingsInfoMap(const IMetadata& sessionParams)
//    -> std::shared_ptr<AppConfigUtil::StreamSettingsInfoMap> {
//  NSCam::Utils::StreamSettingsConverter::IStreamSettingsConverter::
//      StreamSettingsInfoMap result;
//  if (NSCam::Utils::StreamSettingsConverter::IStreamSettingsConverter::
//          trygetStreamSettingsInfoMap(&sessionParams,
//                                      MTK_HAL3PLUS_STREAM_SETTINGS, result)) {
//    // print out streamSettingsInfo
//    for (const auto& it : result) {
//      MY_LOGD("%s", NSCam::Utils::StreamSettingsConverter::
//                        IStreamSettingsConverter::toString(result, it.first)
//                            .c_str());
//    }
//  } else {
//    MY_LOGD("cannot get streamSettingsInfoMap");
//    return nullptr;
//  }
//  return std::make_shared<AppConfigUtil::StreamSettingsInfoMap>(result);
//}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::addConfigStream(
    AppImageStreamInfo* pStreamInfo) -> void {
  Mutex::Autolock _l(mImageConfigMapLock);
  ssize_t const index = mImageConfigMap.indexOfKey(pStreamInfo->getStreamId());
  if (index >= 0) {
    mImageConfigMap.replaceValueAt(index, pStreamInfo);
    // 2nd modification
    // auto& item = mImageConfigMap.editValueAt(index);
    // item.pStreamInfo = pStreamInfo;

    // 1st modification
    // auto& item = mImageConfigMap.editValueAt(index);
    // if  ( keepBufferCache ) {
    // item.pStreamInfo = pStreamInfo;
    // item.vItemFrameQueue.clear();
    // }
    // else {
    //     item.pStreamInfo = pStreamInfo;
    //     item.vItemFrameQueue.clear();

    //     // MY_LOGF_IF(item.pBufferHandleCache==nullptr, "streamId:%#" PRIx64
    //     " has no buffer handle cache", pStreamInfo->getStreamId());
    //     // item.pBufferHandleCache->clear();
    // }
  } else {
    mImageConfigMap.add(pStreamInfo->getStreamId(), pStreamInfo);
    // 2nd modification
    // mImageConfigMap.add(
    //     pStreamInfo->getStreamId(),
    //     ImageConfigItem{
    //         .pStreamInfo = pStreamInfo,
    //         // .pBufferHandleCache = std::make_shared<BufferHandleCache>(
    //         pStreamInfo->getStreamId() ),
    // });
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::addConfigStream(
    AppMetaStreamInfo* pStreamInfo) -> void {
  // MetaConfigItem item;
  // item.pStreamInfo = pStreamInfo;
  // mMetaConfigMap.add(pStreamInfo->getStreamId(), item);
  mMetaConfigMap.add(pStreamInfo->getStreamId(), pStreamInfo);
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::getConfigImageStream(StreamId_T streamId) const
    -> android::sp<AppImageStreamInfo> {
  Mutex::Autolock _l(mImageConfigMapLock);
  ssize_t const index = mImageConfigMap.indexOfKey(streamId);
  if (0 <= index) {
    return mImageConfigMap.valueAt(index);
    // return mImageConfigMap.valueAt(index).pStreamInfo;
  }
  return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::getConfigMetaStream(StreamId_T streamId) const
    -> android::sp<AppMetaStreamInfo> {
  ssize_t const index = mMetaConfigMap.indexOfKey(streamId);
  if (0 <= index) {
    return mMetaConfigMap.valueAt(index);
    // return mMetaConfigMap.valueAt(index).pStreamInfo;
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
auto AppConfigUtil::getAppStreamInfoBuilderFactory() const
    -> std::shared_ptr<IStreamInfoBuilderFactory> {
  return mStreamInfoBuilderFactory;
}

/******************************************************************************
 *
 ******************************************************************************/
auto AppConfigUtil::getConfigMap(ImageConfigMap& imageConfigMap,
                                 MetaConfigMap& metaConfigMap,
                                 bool isInt32Key) -> void {
  imageConfigMap.clear();
  metaConfigMap.clear();
  //  Hal3+ modify : isInt32Key
  if (isInt32Key) {
    for (size_t i = 0; i < mImageConfigMap.size(); ++i) {
      int32_t key = mImageConfigMap.keyAt(i);
      imageConfigMap.add(key, mImageConfigMap.valueAt(i));
    }
  } else {
    for (size_t i = 0; i < mImageConfigMap.size(); ++i) {
      imageConfigMap.add(mImageConfigMap.keyAt(i), mImageConfigMap.valueAt(i));
    }
  }
  for (size_t i = 0; i < mMetaConfigMap.size(); ++i) {
    metaConfigMap.add(mMetaConfigMap.keyAt(i), mMetaConfigMap.valueAt(i));
  }
}

};  // namespace Utils
};  // namespace core
};  // namespace mcam
