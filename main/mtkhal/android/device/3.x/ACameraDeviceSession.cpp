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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#include <main/mtkhal/android/device/3.x/ACameraDeviceSession.h>

#include <graphics_mtk_defs.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/LogicalCam/Type.h>
#include <mtkcam/utils/gralloc/IGrallocHelper.h>
#include <mtkcam/utils/gralloc/mtk_buffer_usage_defs.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#include <mtkcam/drv/IHalSensor.h>
// #include <mtkcam-interfaces/pipeline/utils/packutils/PackUtils_v2.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using ::android::sp;
using ::android::String8;
using mcam::android::Utils::AImageStreamInfo;
using NSCam::DEVICE_FEATURE_SECURE_CAMERA;
using mcam::GrallocRequest;
using mcam::GrallocStaticInfo;
using NSCam::IGrallocHelper;
using mcam::IGraphicImageBufferHeap;
using NSCam::IHalLogicalDeviceList;
using mcam::IImageBufferAllocator;
using mcam::IImageBufferHeap;
using mcam::IIonDeviceProvider;
using NSCam::IMetadataProvider;
using NSCam::MSize;
using NSCam::Type2Type;
using NSCam::Utils::ULog::DetailsType;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::ULogPrinter;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
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
#define VALIDATE_STRING(string) (string != nullptr ? string : "UNKNOWN")

namespace mcam {
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
ACameraDeviceSession::~ACameraDeviceSession() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
ACameraDeviceSession::ACameraDeviceSession(const CreationInfo& info)
    : IACameraDeviceSession(),
      IMtkcamDeviceCallback(),
      mSession(nullptr),
      mCallback(nullptr),
      mACbAdaptor(nullptr),
      mInstanceId(info.instanceId),
      mLogPrefix(std::to_string(mInstanceId) + "-a-session"),
      mLatestSettings(),
      mLogLevel(0) {
  MY_LOGI("ctor");

  ACameraDeviceCallback::CreationInfo creationInfo = {
      .instanceId = mInstanceId,
      .pCallback = info.pCallback,
  };
  mCallback = std::shared_ptr<ACameraDeviceCallback>(
      ACameraDeviceCallback::create(creationInfo));
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("cannot create ACameraDeviceCallback");
  }

  mLogLevel = ::property_get_int32("vendor.debug.camera.log", 0);
  if (mLogLevel == 0) {
    mLogLevel =
        ::property_get_int32("vendor.debug.camera.log.ACameraDeviceSession", 0);
  }

  mMetadataProvider = IMetadataProvider::create(mInstanceId);
  // check if the virtual device backuped by multiple physical devices
  IMetadata::IEntry const& capbilities =
      mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_REQUEST_AVAILABLE_CAPABILITIES);
  if (capbilities.isEmpty()) {
    MY_LOGE(
        "no static "
        "MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA");
    // return -EINVAL;
  } else {
    IHalLogicalDeviceList* pHalDeviceList = MAKE_HalLogicalDeviceList();
    for (MUINT i = 0; i < capbilities.count(); i++) {
      if (capbilities.itemAt(i, Type2Type<MUINT8>()) ==
          MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
        auto physicSensorIDs = pHalDeviceList->getSensorId(mInstanceId);
        for (size_t phy_it = 0; phy_it < physicSensorIDs.size(); phy_it++) {
          int32_t sensorID = physicSensorIDs[phy_it];
          uint32_t virtualID = pHalDeviceList->getVirtualInstanceId(sensorID);
          sp<IMetadataProvider> metadataProvider =
              IMetadataProvider::create(sensorID);
          mPhysicalMetadataProviders[virtualID] = metadataProvider;
        }
      }
    }
  }

  mIonDevice =
      IIonDeviceProvider::get()->makeIonDevice("ACameraDeviceSession", 0);
  if (CC_UNLIKELY(mIonDevice == nullptr)) {
    MY_LOGE("fail to make IonDevice");
  }

  IMetadata::IEntry const& entry =
      mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_REQUEST_PARTIAL_RESULT_COUNT);
  if (entry.isEmpty()) {
    MY_LOGE("no static REQUEST_PARTIAL_RESULT_COUNT");
  } else {
    mAtMostMetaStreamCount = entry.itemAt(0, Type2Type<MINT32>());
  }

  //--------------------------------------------------------------------------
  IMetadata::IEntry const& halBufentry =
      mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION);

  IACallbackAdaptor::CreationInfo cbCreationInfo = {
      .mInstanceId = mInstanceId,
      .mCameraDeviceCallback = info.pCallback,
      .mMetadataProvider = mMetadataProvider,
      .mPhysicalMetadataProviders = mPhysicalMetadataProviders,
      .mSupportBufferManagement = !halBufentry.isEmpty(),
  };

  {
    std::lock_guard<std::mutex> guard(mACbAdaptorLock);
    if (mACbAdaptor != nullptr) {
      MY_LOGE("mACbAdaptor:%p != 0 while opening", mACbAdaptor.get());
      mACbAdaptor->destroy();
      mACbAdaptor = nullptr;
    }
    mACbAdaptor = IACallbackAdaptor::create(cbCreationInfo);
    if (mACbAdaptor == nullptr) {
      mACbAdaptor = nullptr;
      MY_LOGE("IACallbackAdaptor::create");
    }
  }
  //--------------------------------------------------------------------------
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::create(const CreationInfo& info)
    -> ACameraDeviceSession* {
  auto pInstance = new ACameraDeviceSession(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::open(
    const std::shared_ptr<IMtkcamDeviceSession>& session) -> int {
  mSession = session;
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("mSession does not exist");
  }
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::constructDefaultRequestSettings(
    RequestTemplate type,
    const camera_metadata*& requestTemplate) -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }

  // get IMetadata
  IMetadata mtkRequestTemplate;
  auto status = mSession->constructDefaultRequestSettings(
      static_cast<mcam::RequestTemplate>(type), mtkRequestTemplate);
  if (CC_UNLIKELY(status != 0)) {
    MY_LOGE("templateHalSettings is null");
  }
  if (CC_UNLIKELY(convertMetadata(mtkRequestTemplate, requestTemplate))) {
    MY_LOGE("convert failed");
    return -EINVAL;
  }

  return status;
}

/******************************************************************************
 * configuration
 ******************************************************************************/
auto ACameraDeviceSession::setStreamImageProcessing(
    mcam::StreamConfiguration& requestedConfiguration) -> void {
  bool bSMVR = mspParsedSMVRBatchInfo != nullptr ||
               static_cast<uint32_t>(requestedConfiguration.operationMode) == 1;

  bool bVideoConsumer = false;
  auto& streams = requestedConfiguration.streams;
  //
  for (size_t i = 0; i < streams.size(); i++) {
    if (streams[i].usage & mcam::eBUFFER_USAGE_HW_VIDEO_ENCODER) {
      bVideoConsumer = true;
      MY_LOGD("bVideoConsumer %d", bVideoConsumer);
      break;
    }
  }
  // get tunning stream size
  MUINT8 ispTuningDataEnable = 0;
  IMetadata::getEntry<MUINT8>(&requestedConfiguration.sessionParams,
                              MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_ENABLE,
                              ispTuningDataEnable);
  MY_LOGD("ispTuningDataEnable(%d)", ispTuningDataEnable);
  //
  MSize rawIspTuningDataStreamSize, yuvIspTuningDataStreamSize;
  auto& staticMeta = mMetadataProvider->getMtkStaticCharacteristics();
  auto rawEntry =
      staticMeta.entryFor(MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_SIZE_FOR_RAW);
  if (!rawEntry.isEmpty()) {
    rawIspTuningDataStreamSize = MSize(rawEntry.itemAt(0, Type2Type<MINT32>()),
                                       rawEntry.itemAt(1, Type2Type<MINT32>()));
    MY_LOGD("MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_STREAM_SIZE_FOR_RAW(%dx%d)",
            rawIspTuningDataStreamSize.w, rawIspTuningDataStreamSize.h);
  }
  auto yuvEntry =
      staticMeta.entryFor(MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_SIZE_FOR_YUV);
  if (!yuvEntry.isEmpty()) {
    yuvIspTuningDataStreamSize = MSize(yuvEntry.itemAt(0, Type2Type<MINT32>()),
                                       yuvEntry.itemAt(1, Type2Type<MINT32>()));
    MY_LOGD("MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_STREAM_SIZE_FOR_YUV(%d,%d)",
            yuvIspTuningDataStreamSize.w, yuvIspTuningDataStreamSize.h);
  }
  //
  for (size_t i = 0; i < streams.size(); i++) {
    auto& stream = streams[i];
    if (stream.streamType == mcam::StreamType::INPUT) {
      continue;
    }

    switch (stream.format) {
      case NSCam::eImgFmt_BLOB:
      case NSCam::eImgFmt_JPEG:
      case NSCam::eImgFmt_JPEG_APP_SEGMENT: {
        MINT32 setting =
            bVideoConsumer ? MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_FAST
                           : MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY;
        //
        mStreamImageProcessingSettings.emplace(stream.id, setting);
        //
        IMetadata::setEntry<MINT32>(
            &stream.settings, MTK_HALCORE_IMAGE_PROCESSING_SETTINGS, setting);

        MY_LOGD("config streamId:%#" PRIx64 ", imgProcSettings %d", stream.id,
                setting);
      } break;

      case NSCam::eImgFmt_Y16:
      case NSCam::eImgFmt_YV12:
      case NSCam::eImgFmt_NV21:
      case NSCam::eImgFmt_NV12:
      case NSCam::eImgFmt_YUY2:
      case NSCam::eImgFmt_RGB888:
      case NSCam::eImgFmt_RGBA8888:
      case NSCam::eImgFmt_YUV_P010: {
        bool isStreamingUsage =
            0 != (stream.usage & (mcam::eBUFFER_USAGE_HW_TEXTURE |
                                  mcam::eBUFFER_USAGE_HW_COMPOSER |
                                  mcam::eBUFFER_USAGE_HW_VIDEO_ENCODER));
        MSize imgSize = MSize(stream.width, stream.height);
        bool IS_YUV_ISP_TUNING_DATA_STREAM =
            // 0: disable tuning data
            // 1: send tuning data by APP Metadata
            // 2: send tuning data by YUV stream buffer
            (ispTuningDataEnable == 2 && !isStreamingUsage &&
             imgSize == yuvIspTuningDataStreamSize &&
             stream.format == NSCam::eImgFmt_YV12);
        //
        bool IS_RAW_ISP_TUNING_DATA_STREAM =
            // 0: disable tuning data
            // 1: send tuning data by APP Metadata
            // 2: send tuning data by YUV stream buffer
            (ispTuningDataEnable == 2 && !isStreamingUsage &&
             imgSize == rawIspTuningDataStreamSize &&
             stream.format == NSCam::eImgFmt_YV12);
        if (!IS_YUV_ISP_TUNING_DATA_STREAM && !IS_RAW_ISP_TUNING_DATA_STREAM) {
          MINT32 setting =
              bSMVR || isStreamingUsage
                  ? MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_FAST
                  : (MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_FAST |
                     MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY);
          //
          mStreamImageProcessingSettings.emplace(stream.id, setting);
          //
          IMetadata::setEntry<MINT32>(
              &stream.settings, MTK_HALCORE_IMAGE_PROCESSING_SETTINGS, setting);

          MY_LOGD("config streamId:%#" PRIx64 ", imgProcSettings %d", stream.id,
                  setting);
        }
      } break;

      default:
        break;
    }
  }
}

/******************************************************************************
 * request
 ******************************************************************************/
auto ACameraDeviceSession::setStreamImageProcessing(
    std::vector<mcam::CaptureRequest>& requests) -> void {
  for (size_t i = 0; i < requests.size(); i++) {
    auto& request = requests[i];
    uint8_t control_captureIntent = static_cast<uint8_t>(-1L);
    IMetadata::getEntry<uint8_t>(&request.settings, MTK_CONTROL_CAPTURE_INTENT,
                                 control_captureIntent);

    for (size_t j = 0; j < request.outputBuffers.size(); j++) {
      auto& streamBuffer = request.outputBuffers[j];
      auto it = mStreamImageProcessingSettings.find(streamBuffer.streamId);
      if (it == mStreamImageProcessingSettings.end()) {
        continue;
      }
      auto& setting = it->second;
      bool hasHighQuality =
          setting & MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY;
      bool hasFast = setting & MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_FAST;
      auto result = MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_FAST;
      if (hasHighQuality && hasFast) {
        if (control_captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE ||
            control_captureIntent ==
                MTK_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG ||
            control_captureIntent == MTK_CONTROL_CAPTURE_INTENT_MANUAL) {
          result = MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY;
        }
      } else if (hasHighQuality) {
        result = MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY;
      }
      IMetadata::setEntry<MINT32>(&streamBuffer.bufferSettings,
                                  MTK_HALCORE_IMAGE_PROCESSING_SETTINGS,
                                  result);
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::configureStreams(
    const StreamConfiguration& configuration,
    HalStreamConfiguration& halConfiguration) -> int {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }
  disableRequesting();

  mcam::StreamConfiguration mtkConfiguration;
  mcam::HalStreamConfiguration mtkHalConfiguration;
  StreamCreationMap creationInfos;
  int status = 0;

  // check StreamConfiguration
  // bool isCustomHalFlow = false;
  // {
  //   IMetadata sessionParams;
  //   if (CC_UNLIKELY(convertMetadata(
  //       configuration.sessionParams, sessionParams))) {
  //     MY_LOGE("convert failed");
  //     return -EINVAL;
  //   }
  //   IMetadata::IEntry const& e1 =
  //       sessionParams.entryFor(MTK_HAL3PLUS_STREAM_SETTINGS);
  //   if (!e1.isEmpty()) {
  //     MY_LOGI("find stream setting hal3+");
  //     isCustomHalFlow = true;
  //   }
  // }
  // if (!isCustomHalFlow) {
    status = checkConfigParams(configuration);
    if (status != 0) {
      MY_LOGE("checkConfigParams failed");
      return -EINVAL;
    }
  // }

  //
  auto pACbAdptor = getSafeACallbackAdaptor();
  if (pACbAdptor == 0) {
    MY_LOGE("Bad ACallbackAdaptor");
    return -EINVAL;
  }

  // convert StreamConfiguration
  status = parseStreamConfiguration(configuration,
                                    halConfiguration,
                                    mtkConfiguration,
                                    creationInfos);
  if (status != 0) {
    MY_LOGE("failed to parseStreamConfiguration");
    return -EINVAL;
  }
  // configure ACallbackAdaptor
  pACbAdptor->beginConfiguration(mtkConfiguration);

  //
  setStreamImageProcessing(mtkConfiguration);
  //
  status = mSession->configureStreams(mtkConfiguration, mtkHalConfiguration);
  if (status != 0) {
    MY_LOGE("failed to configureStreams");
    return -EINVAL;
  }
  // status = convertHalStreamConfiguration(
  //     mtkHalConfiguration, halConfiguration);
  // if (status != 0) {
  //   MY_LOGE("failed to convertHalStreamConfiguration");
  //   return -EINVAL;
  // }

  // update configuration information from HalStreamConfiguration
  status = updateConfiguration(mtkConfiguration,
                               mtkHalConfiguration,
                               halConfiguration,
                               creationInfos);
  if (status != 0) {
    MY_LOGE("failed to updateConfiguration");
    return -EINVAL;
  }
  //
  pACbAdptor->endConfiguration(mtkHalConfiguration);

  enableRequesting();
  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::checkConfigParams(
    const StreamConfiguration& requestedConfiguration) -> int {
  int status = 0;

  // check overall configuration      -> AppStreamMgr::checkStreams
  // check each stream configuration  -> AppStreamMgr::checkStream
  const std::vector<Stream>& vStreamConfigured = requestedConfiguration.streams;
  if (0 == vStreamConfigured.size()) {
    MY_LOGE("StreamConfiguration.streams.size() = 0");
    return -EINVAL;
  }
  //
  //
  std::map<StreamType, size_t> typeNum;
  typeNum.insert({StreamType::OUTPUT, 0});
  typeNum.insert({StreamType::INPUT, 0});
  //
  std::map<uint32_t, size_t> outRotationNum;

  outRotationNum.insert({StreamRotation::ROTATION_0, 0});
  outRotationNum.insert({StreamRotation::ROTATION_90, 0});
  outRotationNum.insert({StreamRotation::ROTATION_180, 0});
  outRotationNum.insert({StreamRotation::ROTATION_270, 0});
  //
  for (const auto& stream : vStreamConfigured) {
    //
    status = checkStream(stream);
    if (0 != status) {
      MY_LOGE("streams[id:%d] has a bad status: %d(%s)", stream.id, status,
              ::strerror(status));
      return status;
    }
    //
    typeNum[stream.streamType]++;
    if (StreamType::INPUT != stream.streamType)
      outRotationNum[stream.rotation]++;
  }

  /**
   * At most one input-capable stream may be defined (INPUT or BIDIRECTIONAL)
   * in a single configuration.
   *
   * At least one output-capable stream must be defined (OUTPUT or
   * BIDIRECTIONAL).
   */
  /*
   *
   * Return values:
   *
   *  0:      On successful stream configuration
   *
   *  -EINVAL: If the requested stream configuration is invalid. Some examples
   *          of invalid stream configurations include:
   *
   *          - Including more than 1 input-capable stream (INPUT or
   *            BIDIRECTIONAL)
   *
   *          - Not including any output-capable streams (OUTPUT or
   *            BIDIRECTIONAL)
   *
   *          - Including too many output streams of a certain format.
   *
   *          - Unsupported rotation configuration (only applies to
   *            devices with version >= CAMERA_DEVICE_API_VERSION_3_3)
   */
  if ((typeNum[StreamType::INPUT] > 1) || (typeNum[StreamType::OUTPUT] == 0)) {
    MY_LOGE("bad stream count: (out, in)=(%zu, %zu)",
            typeNum[StreamType::OUTPUT], typeNum[StreamType::INPUT]);
    return -EINVAL;
  }
  //
  size_t const num_rotation_not0 =
      outRotationNum[static_cast<int>(StreamRotation::ROTATION_90)] +
      outRotationNum[static_cast<int>(StreamRotation::ROTATION_180)] +
      outRotationNum[static_cast<int>(StreamRotation::ROTATION_270)];
  if (num_rotation_not0 > 1) {
    MY_LOGW(
        "more than one output streams need to rotation: (0, 90, 180, "
        "270)=(%zu,%zu,%zu,%zu)",
        outRotationNum[static_cast<int>(StreamRotation::ROTATION_0)],
        outRotationNum[static_cast<int>(StreamRotation::ROTATION_90)],
        outRotationNum[static_cast<int>(StreamRotation::ROTATION_180)],
        outRotationNum[static_cast<int>(StreamRotation::ROTATION_270)]);
    return -EINVAL;
  }
  //
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::checkStream(const Stream& rStream) -> int {
  /**
   * Return values:
   *
   *  0:      On successful stream configuration
   *
   *  -EINVAL: If the requested stream configuration is invalid. Some examples
   *          of invalid stream configurations include:
   *
   *          - Including streams with unsupported formats, or an unsupported
   *            size for that format.
   *
   *          ....
   *
   *          - Unsupported rotation configuration (only applies to
   *            devices with version >= CAMERA_DEVICE_API_VERSION_3_3)
   */
  if (HAL_DATASPACE_DEPTH == rStream.dataSpace) {
    MY_LOGE("Not support depth dataspace! %s", toString(rStream).c_str());
    return -EINVAL;
  } else if (HAL_DATASPACE_JPEG_APP_SEGMENTS == rStream.dataSpace) {
    IMetadata::IEntry const& entry =
        mMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_HEIC_INFO_SUPPORTED);
    if (entry.isEmpty()) {
      MY_LOGE("no static MTK_HEIC_INFO_SUPPORTED");
      return -EINVAL;
    } else {
      MBOOL heicSupport = entry.itemAt(0, Type2Type<MUINT8>());
      if (heicSupport)
        return 0;
      else
        return -EINVAL;
    }
  } else if (HAL_DATASPACE_UNKNOWN != rStream.dataSpace) {
    MY_LOGW("framework stream dataspace:0x%08x %s", rStream.dataSpace,
            toString(rStream).c_str());
  }
  //
  switch (rStream.rotation) {
    case StreamRotation::ROTATION_0:
      break;
    case StreamRotation::ROTATION_90:
    case StreamRotation::ROTATION_180:
    case StreamRotation::ROTATION_270:
      MY_LOGI("%s", toString(rStream).c_str());
      if (StreamType::INPUT == rStream.streamType) {
        MY_LOGE("input stream cannot support rotation");
        return -EINVAL;
      }

      switch (rStream.format) {
        case HAL_PIXEL_FORMAT_RAW16:
        case HAL_PIXEL_FORMAT_RAW10:
        case HAL_PIXEL_FORMAT_RAW_OPAQUE:
          MY_LOGE("raw stream cannot support rotation");
          return -EINVAL;
        default:
          break;
      }

      break;
    default:
      MY_LOGE("rotation:%d out of bounds - %s", rStream.rotation,
              toString(rStream).c_str());
      return -EINVAL;
  }
  //
  sp<IMetadataProvider> currentMetadataProvider = nullptr;
  if (rStream.physicalCameraId != -1) {
    MUINT32 vid = static_cast<MUINT32>(rStream.physicalCameraId);
    MY_LOGI(
        "Check the setting of physical camera %u instead of logical camera for "
        "streamId %d",
        vid, rStream.id);
    auto search = mPhysicalMetadataProviders.find(vid);
    if (search != mPhysicalMetadataProviders.end()) {
      currentMetadataProvider = search->second;
    } else {
      MY_LOGE("Couldn't find corresponding metadataprovider of physical cam %u",
              vid);
      return -EINVAL;
    }
  } else {
    currentMetadataProvider = mMetadataProvider;
  }
  IMetadata::IEntry const& entryScaler =
      currentMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
  if (entryScaler.isEmpty()) {
    MY_LOGE("no static MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS");
    return -EINVAL;
  }

  // android.scaler.availableStreamConfigurations
  // int32 x n x 4
  std::shared_ptr<AImageStreamInfo> pInfo = getImageStreamInfo(rStream.id);
  for (MUINT i = 0; i < entryScaler.count(); i += 4) {
    int32_t const format = entryScaler.itemAt(i, Type2Type<MINT32>());
    if ((int32_t)rStream.format == format ||
        (pInfo.get() && pInfo->getOriImgFormat() == format)) {
      MUINT32 scalerWidth = entryScaler.itemAt(i + 1, Type2Type<MINT32>());
      MUINT32 scalerHeight = entryScaler.itemAt(i + 2, Type2Type<MINT32>());

      if ((rStream.width == scalerWidth && rStream.height == scalerHeight) ||
          (rStream.rotation & StreamRotation::ROTATION_90 &&
           rStream.width == scalerHeight && rStream.height == scalerWidth)) {
        return 0;
      }
    }
  }
  MY_LOGE("unsupported size %dx%d for format 0x%x/rotation:%d - %s",
          rStream.width, rStream.height, rStream.format, rStream.rotation,
          toString(rStream).c_str());
  return -EINVAL;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::parseStreamConfiguration(
    const StreamConfiguration& configuration,
    HalStreamConfiguration& halConfiguration,
    mcam::StreamConfiguration& mtkConfiguration,
    StreamCreationMap& creationInfos)
    -> int {
  int res, status = 0;
  size_t configurationSize = configuration.streams.size();
  halConfiguration.streams.resize(configurationSize);
  mtkConfiguration.streams.resize(configurationSize);

  // dump input
  MY_LOGD("%s", toString(configuration).c_str());

  // StreamConfigurationMode operationMode;
  mtkConfiguration.operationMode =
      static_cast<mcam::StreamConfigurationMode>(
          configuration.operationMode);
  // const camera_metadata* sessionParams;
  if (CC_UNLIKELY(convertMetadata(configuration.sessionParams,
                                  mtkConfiguration.sessionParams))) {
    MY_LOGE("convert failed");
    return -EINVAL;
  }
  // uint32_t streamConfigCounter = 0;
  mtkConfiguration.streamConfigCounter = configuration.streamConfigCounter;

  //
  mspParsedSMVRBatchInfo =
      extractSMVRBatchInfo(configuration, mtkConfiguration.sessionParams);
  mE2EHDROn = extractE2EHDRInfo(configuration, mtkConfiguration.sessionParams);
  // mspStreamSettingsInfoMap =
  //     extractStreamSettingsInfoMap(mtkConfiguration.sessionParams);
  // mDngFormat =
  //     extractDNGFormatInfo(configuration, mtkConfiguration.sessionParams);

  for (size_t i = 0; i < configurationSize; i++) {
    auto& rStream = configuration.streams[i];
    auto& rHalStream = halConfiguration.streams[i];
    auto& rMtkStream = mtkConfiguration.streams[i];

    // convert Stream
    rMtkStream.id = rStream.id;
    rMtkStream.streamType =
        static_cast<mcam::StreamType>(rStream.streamType);
    rMtkStream.width = rStream.width;
    rMtkStream.height = rStream.height;
    rMtkStream.dataSpace =
        static_cast<decltype(rMtkStream.dataSpace)>(rStream.dataSpace);
    convertStreamRotation(rStream.rotation, rMtkStream.transform);
    rMtkStream.physicalCameraId = rStream.physicalCameraId;
    rMtkStream.bufferSize = rStream.bufferSize;
    // set format, usage and bufPlanes
    AImageStreamInfo::CreationInfo creationInfo;
    res = parseStream(rStream, rHalStream, rMtkStream, creationInfo);
    creationInfos.emplace(rStream.id, creationInfo);
    if (res != 0) {
      status = res;
      MY_LOGE("failed to parse stream %" PRId32, rStream.id);
    }
  }

  // dump output
  if (mLogLevel >= 0) {
    dumpMetadata(mtkConfiguration.sessionParams);
  }
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::extractSMVRBatchInfo(
    const StreamConfiguration& requestedConfiguration,
    const IMetadata& sessionParams) -> std::shared_ptr<ParsedSMVRBatchInfo> {
  const MUINT32 operationMode =
      (const MUINT32)requestedConfiguration.operationMode;

  bool bEnableBatch = EANBLE_SMVR_BATCH == 1;
  MINT32 customP2BatchNum = 1;
  MINT32 p2IspBatchNum = 1;
  std::shared_ptr<ParsedSMVRBatchInfo> pParsedSMVRBatchInfo = nullptr;
  int isFromApMeta = 0;

  IMetadata::IEntry const& entry =
      sessionParams.entryFor(MTK_SMVR_FEATURE_SMVR_MODE);

  if (mLogLevel >= 2) {
    MY_LOGD(
        "SMVRBatch: chk sessionParams.count()=%d, "
        "MTK_SMVR_FEATURE_SMVR_MODE: count: %d",
        sessionParams.count(), entry.count());
    const_cast<IMetadata&>(sessionParams).dump();
  }
  if (bEnableBatch && operationMode == 1) {
    pParsedSMVRBatchInfo = std::make_shared<ParsedSMVRBatchInfo>();
    if (CC_UNLIKELY(pParsedSMVRBatchInfo == nullptr)) {
      CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>", __FUNCTION__);
      return nullptr;
    }
    pParsedSMVRBatchInfo->opMode = 1;
    pParsedSMVRBatchInfo->logLevel =
        ::property_get_int32("vendor.debug.smvrb.loglevel", 0);

    // get image w/h
    for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
      const auto& srcStream = requestedConfiguration.streams[i];
      // if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
      if (pParsedSMVRBatchInfo->imgW == 0) {
        // !!NOTES: assume previewSize = videoOutSize found
        pParsedSMVRBatchInfo->imgW = srcStream.width;
        pParsedSMVRBatchInfo->imgH = srcStream.height;
        // break;
      }
      MY_LOGD("SMVR: vImageStreams[%zu]=%dx%d, isVideo=%" PRIu64 "", i,
              srcStream.width, srcStream.height,
              (srcStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER));
    }

    auto entryHighSpeed =
        mMetadataProvider->getMtkStaticCharacteristics().entryFor(
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
    if (CC_UNLIKELY(pParsedSMVRBatchInfo == nullptr)) {
      CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>", __FUNCTION__);
      return nullptr;
    }
    // get image w/h
    for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
      const auto& srcStream = requestedConfiguration.streams[i];
      // if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
      if (pParsedSMVRBatchInfo->imgW == 0) {
        // !!NOTES: assume previewSize = videoOutSize
        // found
        pParsedSMVRBatchInfo->imgW = srcStream.width;
        pParsedSMVRBatchInfo->imgH = srcStream.height;
        // break;
      }
      MY_LOGD("SMVRBatch: vImageStreams[%zu]=%dx%d, isVideo=%" PRIu64 "", i,
              srcStream.width, srcStream.height,
              (srcStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER));
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
      if (CC_UNLIKELY(pParsedSMVRBatchInfo == nullptr)) {
        CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>",
                 __FUNCTION__);
        return nullptr;
      }

      // get image w/h
      for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
        const auto& srcStream = requestedConfiguration.streams[i];
        // if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
        if (pParsedSMVRBatchInfo->imgW == 0) {
          // !!NOTES: assume previewSize = videoOutSize found
          pParsedSMVRBatchInfo->imgW = srcStream.width;
          pParsedSMVRBatchInfo->imgH = srcStream.height;
          // break;
        }
        MY_LOGD("SMVRBatch: vImageStreams[%lu]=%dx%d, isVideo=%" PRIu64 "", i,
                srcStream.width, srcStream.height,
                (srcStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER));
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
auto ACameraDeviceSession::extractE2EHDRInfo(
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
// auto ACameraDeviceSession::extractStreamSettingsInfoMap(
//     const IMetadata& sessionParams)
//     -> std::shared_ptr<ACameraDeviceSession::StreamSettingsInfoMap> {
//   NSCam::Utils::StreamSettingsConverter::IStreamSettingsConverter::
//       StreamSettingsInfoMap result;
//   if (NSCam::Utils::StreamSettingsConverter::IStreamSettingsConverter::
//           trygetStreamSettingsInfoMap(&sessionParams,
//                                       MTK_HAL3PLUS_STREAM_SETTINGS, result)) {
//     // print out streamSettingsInfo
//     for (const auto& it : result) {
//       MY_LOGD("%s", NSCam::Utils::StreamSettingsConverter::
//                         IStreamSettingsConverter::toString(result, it.first)
//                             .c_str());
//     }
//   } else {
//     MY_LOGD("cannot get streamSettingsInfoMap");
//     return nullptr;
//   }
//   return std::make_shared<ACameraDeviceSession::StreamSettingsInfoMap>(result);
// }

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::extractDNGFormatInfo(
    const IMetadata& sessionParams) -> MINT {
  MINT32 dngBit = 0;
  MINT imageFormat = NSCam::eImgFmt_BAYER10_UNPAK;
  IMetadata::IEntry const& staticMetaEntry =
      mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_SENSOR_INFO_WHITE_LEVEL);
  if (!staticMetaEntry.isEmpty()) {
    dngBit = staticMetaEntry.itemAt(0, Type2Type<MINT32>());
    MY_LOGD("found DNG bit size = %d in static meta", dngBit);
  } else {
    MY_LOGE("cannot found MTK_SENSOR_INFO_WHITE_LEVEL");
    return imageFormat;
  }
  //
#define ADD_CASE(bitDepth)                                \
  case ((1 << bitDepth) - 1):                             \
    imageFormat = NSCam::eImgFmt_BAYER##bitDepth##_UNPAK; \
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
auto ACameraDeviceSession::parseStream(const Stream& rStream,
                                       HalStream& rOutStream,
                                       mcam::Stream& rMtkStream,
                                       AImageStreamInfo::CreationInfo& creationInfo) -> int {
  int err = 0;
  const bool isSMVRBatchStream = mspParsedSMVRBatchInfo != nullptr &&
                                 mspParsedSMVRBatchInfo->opMode != 1 &&
                                 rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER;
  const bool isE2EHdrStream = mE2EHDROn &&
                              mspParsedSMVRBatchInfo == nullptr &&
                              rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER;
  const bool isSecureCameraDevice =
      ((MAKE_HalLogicalDeviceList()->getSupportedFeature(mInstanceId) &
        DEVICE_FEATURE_SECURE_CAMERA) &
       DEVICE_FEATURE_SECURE_CAMERA) > 0;
  const bool isSecureUsage =
      ((rStream.usage & GRALLOC_USAGE_PROTECTED) & GRALLOC_USAGE_PROTECTED) > 0;
  const bool isSecureFlow = isSecureCameraDevice && isSecureUsage;
  if (isSecureCameraDevice && !isSecureUsage) {
    MY_LOGE("Not support insecure stream for a secure camera device");
    return -EINVAL;
  }
  //
  // For secure camera device, the HAL client is expected to set
  // GRALLOC1_PRODUCER_USAGE_PROTECTED, meaning that the buffer is protected
  // from direct CPU access outside the isolated execution environment (e.g.
  // TrustZone or Hypervisor-based solution) or being read by non-secure
  // hardware.
  //
  // Moreover, this flag is incompatible with CPU read and write flags.
  MUINT64 secureUsage =
      (GRALLOC1_PRODUCER_USAGE_PROTECTED | GRALLOC_USAGE_SW_READ_NEVER |
       GRALLOC_USAGE_SW_WRITE_NEVER);
  MUINT64 const usageForHal =
      GRALLOC1_PRODUCER_USAGE_CAMERA |
      (isSecureFlow
           ? (secureUsage)
           : (GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN));
  MUINT64 const usageForHalClient = rStream.usage;
  MUINT64 usageForAllocator = usageForHal | usageForHalClient;
  MY_LOGD("isSecureCameraDevice=%d usageForHal=%#" PRIx64
          " usageForHalClient=%#" PRIx64 "",
          isSecureCameraDevice, usageForHal, usageForHalClient);
  MINT32 const formatToAllocate = static_cast<MINT32>(rStream.format);
  //
  bool isHal3Plus = false;
  //
  usageForAllocator = (rStream.streamType == StreamType::OUTPUT)
                          ? usageForAllocator
                          : usageForAllocator | GRALLOC_USAGE_HW_CAMERA_ZSL;

  // SMVRBatch: gralloc usage
  if (isSMVRBatchStream) {
    usageForAllocator |= to32Bits(NSCam::BufferUsage::SMVR);
  }
  // HDR10+: gralloc usage
  if (isE2EHdrStream) {
    usageForAllocator |= to32Bits(NSCam::BufferUsage::E2E_HDR);
  }
  //
  IGrallocHelper* pGrallocHelper = IGrallocHelper::singleton();
  GrallocStaticInfo grallocStaticInfo;
  GrallocRequest grallocRequest;
  grallocRequest.usage = usageForAllocator;
  grallocRequest.format = formatToAllocate;
  MY_LOGD("grallocRequest.format=%d, grallocRequest.usage = 0x%x ",
          grallocRequest.format, grallocRequest.usage);

  if (HAL_PIXEL_FORMAT_BLOB == formatToAllocate) {
    auto const dataspace = (android_dataspace_t)rStream.dataSpace;
    auto const bufferSz = rStream.bufferSize;
    // For BLOB format with dataSpace DEPTH, this must be zero and and HAL must
    // determine the buffer size based on ANDROID_DEPTH_MAX_DEPTH_SAMPLES.
    if (HAL_DATASPACE_DEPTH == dataspace) {
      // should be return in checkStream.
      MY_LOGE("Not support depth dataspace %s", toString(rStream).c_str());
      return -EINVAL;
    } else if (android_dataspace_t::HAL_DATASPACE_V0_JFIF == dataspace) {
      // For BLOB format with dataSpace JFIF, this must be non-zero and
      // represent the maximal size HAL can lock using
      // android.hardware.graphics.mapper lock API.
      if (CC_UNLIKELY(bufferSz == 0)) {
        MY_LOGW("HAL_DATASPACE_V0_JFIF with bufferSize(0)");
        IMetadata::IEntry const& entry =
            mMetadataProvider->getMtkStaticCharacteristics().entryFor(
                MTK_JPEG_MAX_SIZE);
        if (entry.isEmpty()) {
          MY_LOGW("no static JPEG_MAX_SIZE");
          grallocRequest.widthInPixels = rStream.width * rStream.height * 2;
        } else {
          grallocRequest.widthInPixels = entry.itemAt(0, Type2Type<MINT32>());
        }
      } else {
        grallocRequest.widthInPixels = bufferSz;
      }
      grallocRequest.heightInPixels = 1;
      MY_LOGI("BLOB with widthInPixels(%d), heightInPixels(%d), bufferSize(%u)",
              grallocRequest.widthInPixels, grallocRequest.heightInPixels,
              rStream.bufferSize);
    } else if (HAL_DATASPACE_JPEG_APP_SEGMENTS == dataspace) {
      if (CC_UNLIKELY(bufferSz == 0)) {
        MY_LOGW("HAL_DATASPACE_JPEG_APP_SEGMENTS with bufferSize(0)");
        IMetadata::IEntry const& entry =
            mMetadataProvider->getMtkStaticCharacteristics().entryFor(
                MTK_HEIC_INFO_MAX_JPEG_APP_SEGMENTS_COUNT);
        if (entry.isEmpty()) {
          MY_LOGW("no static JPEG_APP_SEGMENTS_COUNT");
          grallocRequest.widthInPixels = rStream.width * rStream.height;
        } else {
          size_t maxAppsSegment = entry.itemAt(0, Type2Type<MINT32>());
          grallocRequest.widthInPixels =
              maxAppsSegment * (2 + 0xFFFF) + sizeof(struct CameraBlob);
        }
      } else {
        grallocRequest.widthInPixels = bufferSz;
      }
      grallocRequest.heightInPixels = 1;
      MY_LOGI("BLOB with widthInPixels(%d), heightInPixels(%d), bufferSize(%u)",
              grallocRequest.widthInPixels, grallocRequest.heightInPixels,
              rStream.bufferSize);
    } else {
      if (bufferSz != 0)
        grallocRequest.widthInPixels = bufferSz;
      else
        grallocRequest.widthInPixels = rStream.width * rStream.height * 2;
      grallocRequest.heightInPixels = 1;
      MY_LOGW(
          "undefined dataspace(0x%x) with bufferSize(%u) in BLOB format -> "
          "%dx%d",
          static_cast<int>(dataspace), bufferSz, grallocRequest.widthInPixels,
          grallocRequest.heightInPixels);
    }
  } else {
    grallocRequest.widthInPixels = rStream.width;
    grallocRequest.heightInPixels = rStream.height;
  }
  //
  if ((grallocRequest.usage & to32Bits(NSCam::BufferUsage::E2E_HDR)) &&
      (grallocRequest.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)) {
    MY_LOGD("force to re-create stream");
    err = pGrallocHelper->query(&grallocRequest, &grallocStaticInfo, true);
  } else {
    err = pGrallocHelper->query(&grallocRequest, &grallocStaticInfo);
  }
  if (0 != err) {
    MY_LOGE("IGrallocHelper::query - err:%d(%s)", err, ::strerror(-err));
    return err;
  }
  //
  //  stream name = s<stream id>:d<device id>:App:<format>:<hal client usage>
  String8 s8StreamName =
      String8::format("s%d:d%d:App:", rStream.id, mInstanceId);
  String8 const s8FormatAllocated =
      pGrallocHelper->queryPixelFormatName(grallocStaticInfo.format);
  switch (grallocStaticInfo.format) {
    case HAL_PIXEL_FORMAT_BLOB:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCRCB_420_SP:
    case HAL_PIXEL_FORMAT_YCBCR_422_I:
    case HAL_PIXEL_FORMAT_NV12:
    case HAL_PIXEL_FORMAT_YCBCR_P010:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RAW16:
    case HAL_PIXEL_FORMAT_RAW10:
    case HAL_PIXEL_FORMAT_Y8:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case HAL_PIXEL_FORMAT_CAMERA_OPAQUE:
      s8StreamName += s8FormatAllocated;
      break;
    case HAL_PIXEL_FORMAT_Y16:
    default:
      MY_LOGE(
          "Unsupported format:0x%x(%s), grallocRequest.format=%d, "
          "grallocRequest.usage = %d ",
          grallocStaticInfo.format, s8FormatAllocated.c_str(),
          grallocRequest.format, grallocRequest.usage);
      return NULL;
  }
  MY_LOGD("grallocStaticInfo.format=%d", grallocStaticInfo.format);
  //
  s8StreamName += ":";
  s8StreamName += pGrallocHelper->queryGrallocUsageName(usageForHalClient);
  //
  IImageStreamInfo::BufPlanes_t bufPlanes;
  bufPlanes.count = grallocStaticInfo.planes.size();
  for (size_t i = 0; i < bufPlanes.count; i++) {
    IImageStreamInfo::BufPlane& plane = bufPlanes.planes[i];
    plane.sizeInBytes = grallocStaticInfo.planes[i].sizeInBytes;
    plane.rowStrideInBytes = grallocStaticInfo.planes[i].rowStrideInBytes;
  }
  //
  rOutStream.id = rStream.id;
  rOutStream.physicalCameraId = rStream.physicalCameraId;
  rOutStream.overrideFormat =
      (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ==
           static_cast<android_pixel_format_t>(rStream.format)
       // [ALPS03443045] Don't override it since there's a bug in API1 -> HAL3.
       // StreamingProcessor::recordingStreamNeedsUpdate always return true for
       // video stream.
       && (GRALLOC_USAGE_HW_VIDEO_ENCODER & rStream.usage) == 0
       // we don't have input stream's producer usage to determine the real
       // format.
       && StreamType::OUTPUT == rStream.streamType)
          ? static_cast<decltype(rOutStream.overrideFormat)>(
                grallocStaticInfo.format)
          : rStream.format;
  rOutStream.producerUsage =
      (rStream.streamType == StreamType::OUTPUT) ? usageForHal : 0;
  // SMVRBatch: producer usage
  if (isSMVRBatchStream) {
    rOutStream.producerUsage |= to32Bits(NSCam::BufferUsage::SMVR);
  }
  // HDR10+: producer usage
  if (isE2EHdrStream) {
    rOutStream.producerUsage |= to32Bits(NSCam::BufferUsage::E2E_HDR);
  }
  rOutStream.consumerUsage =
      (rStream.streamType == StreamType::OUTPUT) ? 0 : usageForHal;
  rOutStream.maxBuffers = 1;
  rOutStream.overrideDataSpace = rStream.dataSpace;

  auto const& pStreamInfo = getImageStreamInfo(rStream.id);

  MINT imgFormat = grallocStaticInfo.format;

  MINT allocImgFormat = static_cast<MINT>(grallocStaticInfo.format);
  IImageStreamInfo::BufPlanes_t allocBufPlanes = bufPlanes;

  // SMVRBatch:: handle blob layout
  uint32_t oneImgTotalSizeInBytes_32align = 0;
  if (isSMVRBatchStream) {
    uint32_t oneImgTotalSizeInBytes = 0;
    uint32_t oneImgTotalStrideInBytes = 0;

    allocBufPlanes.count = 1;
    // for smvr-batch mode
    allocImgFormat = static_cast<MINT>(NSCam::eImgFmt_BLOB);

    for (size_t i = 0; i < bufPlanes.count; i++) {
      MY_LOGD("SMVRBatch: idx=%zu, (sizeInBytes, rowStrideInBytes)=(%zu,%zu)",
              i, grallocStaticInfo.planes[i].sizeInBytes,
              grallocStaticInfo.planes[i].rowStrideInBytes);
      oneImgTotalSizeInBytes += grallocStaticInfo.planes[i].sizeInBytes;
      //                oneImgTotalStrideInBytes +=
      //                grallocStaticInfo.planes[i].rowStrideInBytes;
      oneImgTotalStrideInBytes = oneImgTotalSizeInBytes;
    }
    oneImgTotalSizeInBytes_32align = (((oneImgTotalSizeInBytes - 1) >> 5) + 1)
                                     << 5;
    allocBufPlanes.planes[0].sizeInBytes =
        oneImgTotalSizeInBytes_32align * mspParsedSMVRBatchInfo->p2BatchNum;
    allocBufPlanes.planes[0].rowStrideInBytes =
        allocBufPlanes.planes[0].sizeInBytes;

    // debug message
    MY_LOGD_IF(mspParsedSMVRBatchInfo->logLevel >= 2,
               "SMVRBatch: %s: isVideo=%" PRIu64
               " \n"
               "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%#" PRIx64
               ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
               "\t grallocStaticInfo(format=0x%x) \n"
               "\t HalStream-info(producerUsage= %#" PRIx64
               ",  consumerUsage= %#" PRIx64
               ", overrideFormat=0x%x ) \n"
               "\t Blob-info(imgFmt=0x%x, allocImgFmt=0x%x, vOutBurstNum=%d, "
               "oneImgTotalSizeInBytes(%d, 32align-%d ), "
               "oneImgTotalStrideInBytes=%d, allocBufPlanes(size=%zu, "
               "sizeInBytes=%zu, rowStrideInBytes=%zu) \n"
               "\t Misc-info(usageForAllocator=%#" PRIx64
               ", GRALLOC1_USAGE_SMVR=%#" PRIx64 ") \n",
               __FUNCTION__, (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER),
               rStream.id, rStream.format, rStream.usage, StreamType::OUTPUT,
               rStream.streamType,
               static_cast<android_pixel_format_t>(grallocStaticInfo.format),
               rOutStream.producerUsage, rOutStream.consumerUsage,
               rOutStream.overrideFormat, imgFormat, allocImgFormat,
               mspParsedSMVRBatchInfo->p2BatchNum, oneImgTotalSizeInBytes,
               oneImgTotalSizeInBytes_32align, oneImgTotalStrideInBytes,
               allocBufPlanes.count, allocBufPlanes.planes[0].sizeInBytes,
               allocBufPlanes.planes[0].rowStrideInBytes, usageForAllocator,
               NSCam::BufferUsage::SMVR);
  } else if (grallocStaticInfo.format == HAL_PIXEL_FORMAT_RAW16 ||
             grallocStaticInfo.format == HAL_PIXEL_FORMAT_RAW10) {
    // Alloc Format: Raw -> Blob
    allocBufPlanes.count = 1;
    allocImgFormat = static_cast<MINT>(NSCam::eImgFmt_BLOB);
    allocBufPlanes.planes[0].sizeInBytes =
        grallocStaticInfo.planes[0].sizeInBytes;
    allocBufPlanes.planes[0].rowStrideInBytes =
        grallocStaticInfo.planes[0].sizeInBytes;
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

  // update imgBufferInfo by streamSettingsInfo
  // if (mspStreamSettingsInfoMap) {
  //   if (!updateImgBufferInfo(mspStreamSettingsInfoMap, imgBufferInfo,
  //                            rStream)) {
  //     MY_LOGD("streamId(%d), update imgBufferInfo failed", rStream.id);
  //   } else {
  //     allocBufPlanes.count = 1;
  //     allocImgFormat = static_cast<MINT>(NSCam::eImgFmt_BLOB);
  //     allocBufPlanes.planes[0].rowStrideInBytes = 0;
  //     allocBufPlanes.planes[0].sizeInBytes = 0;
  //     for (size_t idx = 0; idx < imgBufferInfo.bufPlanes.count; idx++) {
  //       allocBufPlanes.planes[0].sizeInBytes +=
  //           imgBufferInfo.bufPlanes.planes[idx].sizeInBytes;
  //     }
  //     allocBufPlanes.planes[0].rowStrideInBytes =
  //         allocBufPlanes.planes[0].sizeInBytes;
  //     MY_LOGD("stride : %d, size : %d",
  //               allocBufPlanes.planes[0].rowStrideInBytes,
  //               allocBufPlanes.planes[0].sizeInBytes);
  //     isHal3Plus = true;
  //   }
  // }

  creationInfo.mStreamName = s8StreamName;
  // alloc stage, TBD if it's YUV format for batch mode SMVR
  creationInfo.mImgFormat = allocImgFormat;
  creationInfo.mOriImgFormat = pStreamInfo.get() ?
      pStreamInfo->getOriImgFormat() : formatToAllocate;
  creationInfo.mRealAllocFormat = grallocStaticInfo.format;
  creationInfo.mStream = rStream;
  creationInfo.mHalStream = rOutStream;
  creationInfo.mImageBufferInfo = imgBufferInfo;
  creationInfo.mSensorId = static_cast<MINT>(rStream.physicalCameraId);
  creationInfo.mSecureInfo = SecureInfo{isSecureFlow ?
      NSCam::SecType::mem_protected : NSCam::SecType::mem_normal};
  /* alloc stage, TBD if it's YUV format for batch mode SMVR */
  creationInfo.mvbufPlanes = allocBufPlanes;

  // fill in the secure info if and only if it's a secure camera device
  // debug message
  MY_LOGD("rStream.v_32.usage(0x%" PRIx64
          ") \n"
          "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%" PRIx64
          ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
          "\t grallocStaticInfo(format=0x%x) \n"
          "\t HalStream-info(producerUsage= %#" PRIx64
          ",  consumerUsage= %#" PRIx64
          ", overrideFormat=0x%x ) \n"
          "\t Blob-info(imgFmt=0x%x, allocImgFmt=0x%x, "
          "allocBufPlanes(size=%zu, sizeInBytes=%zu, rowStrideInBytes=%zu) \n"
          "\t Misc-info(usageForAllocator=%#" PRIx64
          ", NSCam::BufferUsage::PROT_CAMERA_BIDIRECTIONAL=%#" PRIx64
          ") NSCam::BufferUsage::E2E_HDR=%#" PRIx64
          ")\n"
          "\t creationInfo(secureInfo=0x%x)\n",
          rStream.usage, rStream.id, rStream.format, rStream.usage,
          StreamType::OUTPUT, rStream.streamType,
          static_cast<android_pixel_format_t>(grallocStaticInfo.format),
          rOutStream.producerUsage, rOutStream.consumerUsage,
          rOutStream.overrideFormat, imgFormat, allocImgFormat,
          allocBufPlanes.count, allocBufPlanes.planes[0].sizeInBytes,
          allocBufPlanes.planes[0].rowStrideInBytes, usageForAllocator,
          NSCam::BufferUsage::PROT_CAMERA_BIDIRECTIONAL,
          NSCam::BufferUsage::E2E_HDR,
          toLiteral(creationInfo.mSecureInfo.type));

  // set format, usage and bufPlanes
  if (isHal3Plus) {
    rMtkStream.format =
        static_cast<decltype(rMtkStream.format)>(imgBufferInfo.imgFormat);
    rMtkStream.width = imgBufferInfo.imgWidth;
    rMtkStream.height = imgBufferInfo.imgHeight;
    rMtkStream.usage =
        static_cast<decltype(rMtkStream.usage)>(usageForAllocator);
    rMtkStream.bufPlanes = imgBufferInfo.bufPlanes;
  } else {
    rMtkStream.format =
        static_cast<decltype(rMtkStream.format)>(grallocStaticInfo.format);
    rMtkStream.usage =
        static_cast<decltype(rMtkStream.usage)>(usageForAllocator);
    rMtkStream.bufPlanes.count = grallocStaticInfo.planes.size();
    for (size_t i = 0; i < rMtkStream.bufPlanes.count; i++) {
      auto& plane = rMtkStream.bufPlanes.planes[i];
      plane.sizeInBytes = grallocStaticInfo.planes[i].sizeInBytes;
      plane.rowStrideInBytes = grallocStaticInfo.planes[i].rowStrideInBytes;
    }
  }

  // fill in the secure info if and only if it's a secure camera device
  // debug message
  MY_LOGD("rMtkStream.v_32.usage(0x%" PRIx64
          ") \n"
          "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%" PRIx64
          ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
          "\t grallocStaticInfo(format=0x%x) \n"
          "\t Blob-info(allocBufPlanes(size=%zu, sizeInBytes=%zu, "
          "rowStrideInBytes=%zu) \n"
          "\t Misc-info(usageForAllocator=%#" PRIx64
          ", NSCam::BufferUsage::PROT_CAMERA_BIDIRECTIONAL=%#" PRIx64
          ") NSCam::BufferUsage::E2E_HDR=%#" PRIx64 ")\n",
          rMtkStream.usage, rMtkStream.id, rMtkStream.format, rMtkStream.usage,
          StreamType::OUTPUT, rMtkStream.streamType,
          static_cast<android_pixel_format_t>(grallocStaticInfo.format),
          bufPlanes.count, bufPlanes.planes[0].sizeInBytes,
          bufPlanes.planes[0].rowStrideInBytes, usageForAllocator,
          NSCam::BufferUsage::PROT_CAMERA_BIDIRECTIONAL,
          NSCam::BufferUsage::E2E_HDR);

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::updateConfiguration(
    const mcam::StreamConfiguration& mtkConfiguration,
    const mcam::HalStreamConfiguration& mtkHalConfiguration,
    HalStreamConfiguration& halConfiguration,
    StreamCreationMap& creationInfos) -> int {
  size_t configurationSize = halConfiguration.streams.size();

  for (size_t i = 0; i < configurationSize; i++) {
    auto& rMtkStream = mtkConfiguration.streams[i];
    auto& rMtkHalStream = mtkHalConfiguration.streams[i];

    // update max buffer size in HalStreamConfiguration
    auto& rHalStream = halConfiguration.streams[i];
    rHalStream.maxBuffers = rMtkHalStream.maxBuffers;

    // update image format in StreamInfo creation
    auto& rCreationInfo = creationInfos.find(rMtkHalStream.id)->second;
    auto& rImageFormat = rCreationInfo.mImageBufferInfo.imgFormat;
    rImageFormat = rMtkHalStream.overrideFormat;

    // handle AOSP RAW16 bit depth
    if (rImageFormat == NSCam::eImgFmt_RAW16) {
      rImageFormat = extractDNGFormatInfo(mtkConfiguration.sessionParams);
    }

    // create AImageStreamInfo after update
    {
      std::lock_guard<std::mutex> guard(mImageConfigMapLock);
      auto pStream = std::make_shared<AImageStreamInfo>(rCreationInfo);
      mImageConfigMap.emplace(rMtkHalStream.id, pStream);
    }
  }
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::processCaptureRequest(
    const std::vector<CaptureRequest>& requests,
    uint32_t& numRequestProcessed) -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }
  // auto test log print
  for (auto const& req : requests) {
    printAutoTestLog(req.settings, false);
  }
  std::vector<mcam::CaptureRequest> mtkRequests;
  // convert parameters
  convertCaptureRequests(requests, mtkRequests);
  // apply AOSP setting rule
  applySettingRule(mtkRequests);
  // convert buffers: buffer_handle_t -> shared_ptr<IImageBufferHeap>
  if (CC_UNLIKELY(convertImageStreamBuffers(requests, mtkRequests))) {
    MY_LOGE("failed to convertImageStreamBuffers");
  }
  //
  setStreamImageProcessing(mtkRequests);
  //
  auto pACbAdptor = getSafeACallbackAdaptor();
  if (pACbAdptor == 0) {
    MY_LOGE("Bad ACallbackAdaptor");
    return -EINVAL;
  }
  // 3. handle flush case
  if (0 == mRequestingAllowed.load(std::memory_order_relaxed)) {
    MY_LOGW("submitting requests during flushing - requestNo_1st:%u #:%zu",
            mtkRequests[0].frameNumber, numRequestProcessed);
    for (auto& r : mtkRequests) {
      CAM_ULOG_EXIT_GUARD(MOD_CAMERA_DEVICE,
                          NSCam::Utils::ULog::REQ_APP_REQUEST, r.frameNumber);
    }
    //
    // AppStreamMgr handle flushed requests
    numRequestProcessed = mtkRequests.size();
    pACbAdptor->flushRequest(mtkRequests);
    return 0;
  }
  pACbAdptor->submitRequests(mtkRequests);
  //
  auto status =
      mSession->processCaptureRequest(mtkRequests, numRequestProcessed);

  if (CC_UNLIKELY(status != 0)) {
    MY_LOGE("status: %d ,numRequestProcessed: %u", status, numRequestProcessed);
    MY_LOGE("======== dump all requests ========");
    for (auto& request : requests) {
      MY_LOGE("%s", toString(request).c_str());
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::flush() -> int {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }
  disableRequesting();

  auto status = mSession->flush();

  auto pACbAdptor = getSafeACallbackAdaptor();
  if (pACbAdptor != 0) {
    MY_LOGD("waitUntilDrained +");
    int err = pACbAdptor->waitUntilDrained(500000000);
    MY_LOGD("waitUntilDrained -");
    MY_LOGW_IF(0 != err, "ACallbackAdaptor::waitUntilDrained err:%d(%s)", -err,
               ::strerror(-err));
  }

  enableRequesting();
  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::close() -> int {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }

  disableRequesting();

  auto status = mSession->close();
  if (CC_UNLIKELY(status != 0)) {
    MY_LOGE("ACameraDeviceSession close fail");
  }
  mSession = nullptr;

  //
  {
    std::lock_guard<std::mutex> guard(mACbAdaptorLock);
    if (mACbAdaptor != nullptr) {
      mACbAdaptor->destroy();
      mACbAdaptor = nullptr;
      MY_LOGD("ACallbackAdaptor -");
    }
  }

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::signalStreamFlush(
    const std::vector<int32_t>& streamIds,
    uint32_t streamConfigCounter) -> void {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }

  mSession->signalStreamFlush(streamIds, streamConfigCounter);

  MY_LOGD("-");
  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::isReconfigurationRequired(
    const camera_metadata* const& oldSessionParams,
    const camera_metadata* const& newSessionParams,
    bool& reconfigurationNeeded) -> int {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }

  IMetadata oldHalMetadata;
  IMetadata newHalMetadata;
  if (CC_UNLIKELY(convertMetadata(oldSessionParams, oldHalMetadata))) {
    MY_LOGE("convert oldSessionParams failed");
    return -EINVAL;
  }
  if (CC_UNLIKELY(convertMetadata(newSessionParams, newHalMetadata))) {
    MY_LOGE("convert newSessionParams failed");
    return -EINVAL;
  }

  auto status = mSession->isReconfigurationRequired(
      oldHalMetadata, newHalMetadata, reconfigurationNeeded);

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::switchToOffline(
    const std::vector<int64_t>& streamsToKeep,
    CameraOfflineSessionInfo& offlineSessionInfo,
    std::shared_ptr<IACameraOfflineSession>& offlineSession) -> int {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }

  MY_LOGW("Not implement, return -EINVAL.");

  MY_LOGD("-");
  return -EINVAL;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::getConfigurationResults(
    const uint32_t streamConfigCounter,
    ExtConfigurationResults& configurationResults) -> int {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }

  mcam::ExtConfigurationResults mtkConfigurationResults;
  int status = mSession->getConfigurationResults(streamConfigCounter,
                                                 mtkConfigurationResults);
  convertExtConfigurationResults(mtkConfigurationResults, configurationResults);

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& mtkResults) -> void {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  auto pACbAdptor = getSafeACallbackAdaptor();
  if (pACbAdptor == 0) {
    MY_LOGE("Bad ACallbackAdaptor");
    return;
  }
  pACbAdptor->processCaptureResult(mtkResults);

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::notify(
    const std::vector<mcam::NotifyMsg>& mtkMsgs) -> void {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  // std::vector<NotifyMsg> msgs;
  // convertNotifyMsgs(mtkMsgs, msgs);
  // mCallback->notify(msgs);

  auto pACbAdptor = getSafeACallbackAdaptor();
  if (pACbAdptor == 0) {
    MY_LOGE("Bad ACallbackAdaptor");
    return;
  }
  pACbAdptor->notify(mtkMsgs);

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
#if 0  // not implement HAL Buffer Management
auto ACameraDeviceSession::requestStreamBuffers(
    const std::vector<mcam::BufferRequest>& vMtkBufferRequests,
    std::vector<mcam::StreamBufferRet>* pvMtkBufferReturns)
    -> mcam::BufferRequestStatus {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  std::vector<BufferRequest> vBufferRequests;
  std::vector<StreamBufferRet> pvBufferReturns;
  convertBufferRequests(vMtkBufferRequests, vBufferRequests);
  convertStreamBufferRets(*pvMtkBufferReturns, pvBufferReturns);
  BufferRequestStatus status =
      mCallback->requestStreamBuffers(vBufferRequests, &pvBufferReturns);

  return static_cast<mcam::BufferRequestStatus>(status);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::returnStreamBuffers(
    const std::vector<mcam::StreamBuffer>& mtkBuffers) -> void {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  std::vector<StreamBuffer> buffers;
  convertStreamBuffers(mtkBuffers, buffers);
  mCallback->returnStreamBuffers(buffers);

  return;
}
#endif

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::getImageStreamInfo(int64_t streamId)
    -> std::shared_ptr<AImageStreamInfo> {
  std::lock_guard<std::mutex> guard(mImageConfigMapLock);
  auto it = mImageConfigMap.find(streamId);
  if (it != mImageConfigMap.end()) {
    return it->second;
  }
  return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::convertImageStreamBuffers(
    const std::vector<CaptureRequest>& srcRequests,
    std::vector<mcam::CaptureRequest>& dstRequests) -> int {
  int status = 0;
  int res = 0;
  for (int i = 0; i < srcRequests.size(); i++) {
    if (srcRequests[i].inputBuffer.streamId != NO_STREAM) {
      auto pStreamInfo =
          mImageConfigMap.find(srcRequests[i].inputBuffer.streamId)->second;
      res = createImageBufferHeap(srcRequests[i].inputBuffer, pStreamInfo,
                                  dstRequests[i].inputBuffers[0].buffer);
      if (CC_UNLIKELY(res != 0)) {
        MY_LOGE("create failed");
        status = res;
      }
    }
    for (int j = 0; j < srcRequests[i].outputBuffers.size(); j++) {
      auto pStreamInfo =
          mImageConfigMap.find(srcRequests[i].outputBuffers[j].streamId)
              ->second;
      res = createImageBufferHeap(srcRequests[i].outputBuffers[j], pStreamInfo,
                                  dstRequests[i].outputBuffers[j].buffer);
      if (CC_UNLIKELY(res != 0)) {
        MY_LOGE("create failed");
        status = res;
      }
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::createImageBufferHeap(
    const StreamBuffer& rStreamBuffer,
    const std::shared_ptr<AImageStreamInfo>& pStreamInfo,
    std::shared_ptr<mcam::IImageBufferHeap>& pImageBufferHeap) -> int {
  if (pStreamInfo == nullptr) {
    MY_LOGE(
        "Null ImageStreamInfo, failed to allocate streamBuffer, return null");
    return -EINVAL;
  }

  int heapFormat = pStreamInfo->getAllocImgFormat();
  bool isJpeg =
      pStreamInfo->getImgFormat() == static_cast<int>(NSCam::eImgFmt_JPEG) ||
      pStreamInfo->getImgFormat() ==
          static_cast<int>(NSCam::eImgFmt_JPEG_APP_SEGMENT);
  std::string strName = VALIDATE_STRING(pStreamInfo->getStreamName());

  auto bufferHandle = const_cast<buffer_handle_t>(rStreamBuffer.buffer);
  auto& acquire_fence = rStreamBuffer.acquireFenceFd;
  auto& release_fence = rStreamBuffer.releaseFenceFd;
  auto bufferName =
      "halbuf:b" + std::to_string(rStreamBuffer.bufferId) + ":" + strName;

  MY_LOGD(
      "createFromBlob check: heapFormat(%d),"
      "eImgFmt_BLOB(%d),ImgFormat(%d),eImgFmt_BLOB_START(%d)",
      heapFormat, NSCam::eImgFmt_BLOB, pStreamInfo->getImgFormat(),
      NSCam::eImgFmt_BLOB_START);

  // if (heapFormat == static_cast<int>(NSCam::eImgFmt_BLOB)
  //     && mspStreamSettingsInfoMap) {
  //   MY_LOGD("hal3plus flow: createFromBlob");
  //   IImageBufferAllocator::ImgParam param(
  //     pStreamInfo->getImgFormat(),
  //     pStreamInfo->getImgSize(),
  //     pStreamInfo->getUsageForAllocator());

  //   for (int i = 0; i < pStreamInfo->getBufPlanes().count; ++i) {
  //     param.strideInByte[i] =
  //         pStreamInfo->getBufPlanes().planes[i].rowStrideInBytes;
  //     MY_LOGD("createFromBlob param: strideInByte[%d]=(%d)",
  //              i, param.strideInByte[i]);
  //   }
  //   MY_LOGD("createFromBlob param: format(%d), size(%d,%d)",
  //            param.format, param.size.w, param.size.h);

  //   pImageBufferHeap = IGraphicImageBufferHeap::createFromBlob(
  //       bufferName.c_str(), param, &bufferHandle, acquire_fence, release_fence,
  //       mIonDevice, rStreamBuffer.appBufferHandleHolder);

  //   MY_LOGD("createFromBlob info: imgFormat(%d),imgSize(%d,%d),planeCount(%d)",
  //           pImageBufferHeap->getImgFormat(), pImageBufferHeap->getImgSize().w,
  //           pImageBufferHeap->getImgSize().h,
  //           pImageBufferHeap->getPlaneCount());
  //   for (int i = 0; i < pImageBufferHeap->getPlaneCount(); ++i) {
  //     MY_LOGD("bufStridesInBytes[%d]=(%d)", i,
  //             pImageBufferHeap->getBufStridesInBytes(i));
  //   }
  //   //
  //   if (pImageBufferHeap == nullptr) {
  //     ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG,
  //                            DetailsType::DETAILS_DEBUG,
  //                            "createIGraphicImageBufferHeap-Fail");

  //     MY_LOGF(
  //         "IGraphicImageBufferHeap::createFromBlob \"%s:%s\","
  //         "handle: %p, fd: %d",
  //         bufferName.c_str(), pStreamInfo->getStreamName(), bufferHandle,
  //         bufferHandle->data[0]);
  //   }
  // } else {
    if (heapFormat == static_cast<int>(NSCam::eImgFmt_BLOB) && isJpeg) {
      heapFormat = pStreamInfo->getImgFormat();
      MY_LOGD("blob -> Jpeg");
    }
    MY_LOGD("original flow: create");
    int stride = 0;
    pImageBufferHeap = IGraphicImageBufferHeap::create(
        (bufferName + ":" + strName).c_str(),
        pStreamInfo->getUsageForAllocator(),
        pStreamInfo->getAllocImgFormat() == NSCam::eImgFmt_BLOB
            ? MSize(pStreamInfo->getAllocBufPlanes().planes[0].rowStrideInBytes,
                    1)
            : pStreamInfo->getImgSize(),
        heapFormat, &bufferHandle, acquire_fence, release_fence, stride,
        pStreamInfo->getSecureInfo().type, mIonDevice,
        rStreamBuffer.appBufferHandleHolder);

    if (pImageBufferHeap == nullptr) {
      ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG,
                              DetailsType::DETAILS_DEBUG,
                              "createIGraphicImageBufferHeap-Fail");
      // mFrameHandler->dumpState(logPrinter, std::vector<std::string>());
      MY_LOGF("IGraphicImageBufferHeap::create \"%s:%s\", handle: %p, fd: %d",
              bufferName.c_str(), pStreamInfo->getStreamName(), bufferHandle,
              bufferHandle->data[0]);
    }
  // }

  if (pStreamInfo->getAllocImgFormat() == HAL_PIXEL_FORMAT_RAW16 &&
      pStreamInfo->getStreamType() == NSCam::v3::eSTREAMTYPE_IMAGE_IN) {
    uint8_t colorFilterArrangement = 0;
    IMetadata const& staticMetadata =
        mMetadataProvider->getMtkStaticCharacteristics();
    bool ret = IMetadata::getEntry(&staticMetadata,
                                   MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
                                   colorFilterArrangement);
    MY_LOGF_IF(!ret,
               "no static info: MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT");
    auto sensorColorOrder = convertToSensorColorOrder(colorFilterArrangement);
    pImageBufferHeap->setColorArrangement(sensorColorOrder);
  }

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::convertToSensorColorOrder(
    uint8_t colorFilterArrangement) -> int32_t {
  switch (colorFilterArrangement) {
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
      return (int32_t)NSCam::SENSOR_FORMAT_ORDER_RAW_R;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
      return (int32_t)NSCam::SENSOR_FORMAT_ORDER_RAW_Gr;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
      return (int32_t)NSCam::SENSOR_FORMAT_ORDER_RAW_Gb;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
      return (int32_t)NSCam::SENSOR_FORMAT_ORDER_RAW_B;
    default:
      break;
  }
  MY_LOGE("Unsupported Color Filter Arrangement:%d", colorFilterArrangement);
  return -1;
}

/******************************************************************************
 *
 ******************************************************************************/
// auto ACameraDeviceSession::mapImageFormat(
//     uint32_t const& format) -> int32_t {
//   switch (format) {
//     case MTK_HAL3PLUS_STREAM_FORMAT_BLOB:
//       return (int32_t)NSCam::eImgFmt_BLOB;
//     case MTK_HAL3PLUS_STREAM_FORMAT_JPEG:
//       return (int32_t)NSCam::eImgFmt_JPEG;
//     case MTK_HAL3PLUS_STREAM_FORMAT_RGB_888:
//       return (int32_t)NSCam::eImgFmt_RGB888;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_3PLANE:
//       return (int32_t)NSCam::eImgFmt_YUV_P210_3PLANE;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010:
//       return (int32_t)NSCam::eImgFmt_YUV_P010;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010_PAK:
//       return (int32_t)NSCam::eImgFmt_MTK_YUV_P010;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P012_PAK:
//       return (int32_t)NSCam::eImgFmt_MTK_YUV_P012;
//     case MTK_HAL3PLUS_STREAM_FORMAT_NV21:
//       return (int32_t)NSCam::eImgFmt_NV21;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YV12:
//       return (int32_t)NSCam::eImgFmt_YV12;
//     case MTK_HAL3PLUS_STREAM_FORMAT_Y8:
//       return (int32_t)NSCam::eImgFmt_Y8;
//     case MTK_HAL3PLUS_STREAM_FORMAT_NV12:
//       return (int32_t)NSCam::eImgFmt_NV12;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_PAK:
//       return (int32_t)NSCam::eImgFmt_MTK_YUV_P210;
//     case MTK_HAL3PLUS_STREAM_FORMAT_YUY2:
//       return (int32_t)NSCam::eImgFmt_YUY2;
//     case MTK_HAL3PLUS_STREAM_FORMAT_RAW10_PAK:
//       return (int32_t)NSCam::eImgFmt_BAYER10;
//     case MTK_HAL3PLUS_STREAM_FORMAT_RAW12_PAK:
//       return (int32_t)NSCam::eImgFmt_BAYER12;
//     case MTK_HAL3PLUS_STREAM_FORMAT_RAW12:
//       return (int32_t)NSCam::eImgFmt_BAYER12_UNPAK;
//     case MTK_HAL3PLUS_STREAM_FORMAT_RAW16:
//       return (int32_t)NSCam::eImgFmt_BAYER16_UNPAK;
//     default:
//       MY_LOGW("unsupport format: (%d)", format);
//       return -1;
//   }
// }

/******************************************************************************
 *
 ******************************************************************************/
// auto ACameraDeviceSession::updateImgBufferInfo(
//     const std::shared_ptr<ACameraDeviceSession::StreamSettingsInfoMap>
//         mspStreamSettingsInfoMap,
//     NSCam::ImageBufferInfo& imgBufferInfo,
//     const Stream& rStream) -> bool {
//   bool ret = true;
//   if (nullptr == mspStreamSettingsInfoMap) {
//     return false;
//   }
//   auto updateImgFormat = imgBufferInfo.imgFormat;
//   auto updateBufPlanes = imgBufferInfo.bufPlanes;
//   auto updateImgWidth = rStream.width;
//   auto updateImgHeight = rStream.height;

//   auto it = mspStreamSettingsInfoMap->find(rStream.id);
//   if (it != mspStreamSettingsInfoMap->end()) {
//     // update image format
//     auto pFormat = it->second->pFormat;
//     if (pFormat) {
//       updateImgFormat = mapImageFormat(*pFormat);
//       MY_LOGD("streamId(%d): update image format: (%d)->(%d)", rStream.id,
//               imgBufferInfo.imgFormat, updateImgFormat);
//     }

//     // update image width and height
//     auto pSize = it->second->pSize;
//     if (pSize) {
//       updateImgWidth = pSize->w;
//       updateImgHeight = pSize->h;
//       MY_LOGD("streamId(%d): update image size: (%d,%d)->(%d,%d)", rStream.id,
//               imgBufferInfo.imgWidth, imgBufferInfo.imgHeight, updateImgWidth,
//               updateImgHeight);
//     }

//     // update for tuning stream
//     auto pSource = it->second->pSource;
//     if (pSource != nullptr &&
//         *pSource == MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA) {
//       auto mspPackUtil =
//           NSCam::v3::PackUtilV2::IIspTuningDataPackUtil::getInstance();
//       updateBufPlanes.count = 1;
//       updateBufPlanes.planes[0].sizeInBytes =
//           updateBufPlanes.planes[0].rowStrideInBytes = updateImgWidth =
//               mspPackUtil->getPackBufferSize(0);  // input sensor id no used

//       updateImgFormat = NSCam::eImgFmt_ISP_TUING;  // typo: TUNING
//       updateImgHeight = 1;

//       MY_LOGD(
//           "streamId(%d): is tuning stream, update image format: (%d)->(%d),"
//           " image (width,height)=(%d,%d)",
//           rStream.id, imgBufferInfo.imgFormat, updateImgFormat, updateImgWidth,
//           updateImgHeight);
//     } else if (updateImgFormat == NSCam::eImgFmt_BAYER10) {
//       updateBufPlanes.count = 1;
// #define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//       updateBufPlanes.planes[0].rowStrideInBytes =
//           ALIGN_PIXEL(updateImgWidth, 64) * 1.25;
// #undef ALIGN_PIXEL

//       MY_LOGD("streamId(%d): aligned stride (by 64) : %d", rStream.id,
//               updateBufPlanes.planes[0].rowStrideInBytes);
//       updateBufPlanes.planes[0].sizeInBytes =
//           updateImgHeight * updateBufPlanes.planes[0].rowStrideInBytes;
//     } else if (updateImgFormat == NSCam::eImgFmt_MTK_YUV_P210 ||
//                updateImgFormat == NSCam::eImgFmt_MTK_YUV_P010) {
//       // update buffer planes info for 10bpp format
//       updateBufPlanes.count =
//           NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//       for (int i = 0; i < updateBufPlanes.count; ++i) {
// #define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//         updateBufPlanes.planes[i].rowStrideInBytes = ALIGN_PIXEL(
//             (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//             updateImgWidth) *
//             NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//             + 7) / 8, 80);
// #undef ALIGN_PIXEL
//         MY_LOGD("streamId(%d): buffer plane[%d] aligned stride (by 80) : %d",
//             rStream.id, i, updateBufPlanes.planes[i].rowStrideInBytes);

//         updateBufPlanes.planes[i].sizeInBytes =
//             updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes;
//       }
//     } else if (updateImgFormat == NSCam::eImgFmt_MTK_YUV_P012) {
//       // update buffer planes info for 12bpp format
//       updateBufPlanes.count =
//           NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//       for (int i = 0; i < updateBufPlanes.count; ++i) {
// #define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//         updateBufPlanes.planes[i].rowStrideInBytes = ALIGN_PIXEL(
//             (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//             updateImgWidth) *
//             NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//             + 7) / 8, 96);
// #undef ALIGN_PIXEL
//         MY_LOGD("streamId(%d): buffer plane[%d] aligned stride (by 96) : %d",
//             rStream.id, i, updateBufPlanes.planes[i].rowStrideInBytes);

//         updateBufPlanes.planes[i].sizeInBytes =
//             updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes;
//       }
//     } else if (updateImgFormat == NSCam::eImgFmt_NV21) {
//       // update buffer planes info for 8bpp format
//       updateBufPlanes.count =
//           NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//       for (int i = 0; i < updateBufPlanes.count; ++i) {
// #define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
//         updateBufPlanes.planes[i].rowStrideInBytes = ALIGN_PIXEL(
//             (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//             updateImgWidth) *
//             NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//             + 7) / 8, 64);
// #undef ALIGN_PIXEL
//         MY_LOGD("streamId(%d): buffer plane[%d] aligned stride (by 64) : %d",
//             rStream.id, i, updateBufPlanes.planes[i].rowStrideInBytes);

//         updateBufPlanes.planes[i].sizeInBytes = (i == 0) ?
//             (updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes) :
//             (updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes / 2);
//       }
//     } else {
//       // update buffer planes info for other format
//       updateBufPlanes.count =
//           NSCam::Utils::Format::queryPlaneCount(updateImgFormat);
//       for (int i = 0; i < updateBufPlanes.count; ++i) {
//         updateBufPlanes.planes[i].rowStrideInBytes =
//             (NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, i,
//             updateImgWidth) *
//             NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, i)
//             + 7) / 8;
//         updateBufPlanes.planes[i].sizeInBytes =
//             updateImgHeight * updateBufPlanes.planes[i].rowStrideInBytes;
//       }
//     }
//   } else {
//     MY_LOGD("streamId(%d): no need to update ImgBufferInfo", rStream.id);
//     return false;
//   }

//   // debug:
//   MY_LOGD("queryPlaneWidthInPixels(updateImgFormat, 0, updateImgWidth) = %d,"
//           "queryPlaneBitsPerPixel(updateImgFormat, 0) = %d",
//             NSCam::Utils::Format::queryPlaneWidthInPixels(updateImgFormat, 0,
//             updateImgWidth),
//             NSCam::Utils::Format::queryPlaneBitsPerPixel(updateImgFormat, 0));

//   imgBufferInfo.bufPlanes = updateBufPlanes;
//   imgBufferInfo.imgFormat = updateImgFormat;
//   imgBufferInfo.imgWidth = updateImgWidth;
//   imgBufferInfo.imgHeight = updateImgHeight;

//   return ret;
// }

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::applySettingRule(
    std::vector<mcam::CaptureRequest>& rMktRequests) -> void {
  /**
   * As a special case, a NULL settings buffer indicates that the
   * settings are identical to the most-recently submitted capture request.
   * A NULL buffer cannot be used as the first submitted request after a
   * configure_streams() call.
   */
  for (auto& request : rMktRequests) {
    bool isRepeating = false;
    if (request.settings.count() > 0) {
      isRepeating = false;
      mLatestSettings.clear();
      mLatestSettings = request.settings;  // update Latest setting
      MY_LOGD_IF(
          mLogLevel >= 1,
          "frameNo:%u NULL settings -> most-recently submitted capture request",
          request.frameNumber);

    } else {
      isRepeating = true;
      request.settings = mLatestSettings;  // use Latest setting
    }
    // set vendor tag
    IMetadata::setEntry<MUINT8>(&request.settings,
                                MTK_HALCORE_ISREAPEATING_SETTING,
                                isRepeating ? 1 : 0);
    // debug
    if (mLogLevel >= 2) {
      dumpMetadata(request.settings, request.frameNumber);
    }
    if (!isRepeating) {
      //
      IMetadata::IEntry const& e1 =
          mLatestSettings.entryFor(MTK_CONTROL_AF_TRIGGER);
      if (!e1.isEmpty()) {
        MUINT8 af_trigger = e1.itemAt(0, Type2Type<MUINT8>());
        if (af_trigger == MTK_CONTROL_AF_TRIGGER_START) {
          CAM_ULOGM_DTAG_BEGIN(true, "AF_state: %d", af_trigger);
          MY_LOGD("AF_state: %d", af_trigger);
          CAM_ULOGM_DTAG_END();
        }
      }
      //
      IMetadata::IEntry const& e2 =
          mLatestSettings.entryFor(MTK_CONTROL_AE_PRECAPTURE_TRIGGER);
      if (!e2.isEmpty()) {
        MUINT8 ae_pretrigger = e2.itemAt(0, Type2Type<MUINT8>());
        if (ae_pretrigger == MTK_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
          CAM_ULOGM_DTAG_BEGIN(true, "ae precap: %d", ae_pretrigger);
          MY_LOGD("ae precap: %d", ae_pretrigger);
          CAM_ULOGM_DTAG_END();
        }
      }
      //
      IMetadata::IEntry const& e3 =
          mLatestSettings.entryFor(MTK_CONTROL_CAPTURE_INTENT);
      if (!e3.isEmpty()) {
        MUINT8 capture_intent = e3.itemAt(0, Type2Type<MUINT8>());
        CAM_ULOGM_DTAG_BEGIN(true, "capture intent: %d", capture_intent);
        MY_LOGD("capture intent: %d", capture_intent);
        CAM_ULOGM_DTAG_END();
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::getSafeACallbackAdaptor() const
    -> ::android::sp<IACallbackAdaptor> {
  //  Although mACbAdaptor is setup during opening camera,
  //  we're not sure any callback to this class will arrive
  //  between open and close calls.
  std::lock_guard<std::mutex> guard(mACbAdaptorLock);
  return mACbAdaptor;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::enableRequesting() -> void {
  std::lock_guard<std::mutex> _lRequesting(mRequestingLock);
  mRequestingAllowed.store(1, std::memory_order_relaxed);
  m1stRequestNotSent = true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraDeviceSession::disableRequesting() -> void {
  std::lock_guard<std::mutex> _lRequesting(mRequestingLock);
  mRequestingAllowed.store(0, std::memory_order_relaxed);
}

};  // namespace android
};  // namespace mcam
