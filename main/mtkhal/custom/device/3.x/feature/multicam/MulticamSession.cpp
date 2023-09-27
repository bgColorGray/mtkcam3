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
#define LOG_TAG "mtkcam_hal/custom"

#include "main/mtkhal/custom/device/3.x/feature/multicam/MulticamSession.h"

#include <mtkcam/utils/metadata/IMetadata.h>

#include <cutils/properties.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>

// #include <mtkcam/utils/metastore/IMetadataProvider.h>

#include <dlfcn.h>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <sstream>

#include "../../MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using mcam::core::EnableOption;

#define DUMP_FOLDER "/data/vendor/camera_dump/"
#define FUNC_START MY_LOGD("+");
#define FUNC_END MY_LOGD("-");
// #define PHYSICAL_FORMAT 8217  // ISP7 ise NSCam::eImgFmt_MTK_YUV_P010
#define PHYSICAL_FORMAT 17  // ISP6 use NSCam::eImgFmt_NV21
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
#define MY_LOGV_IF(c, ...) do { if (c) { MY_LOGV(__VA_ARGS__); } } while (0)
#define MY_LOGD_IF(c, ...) do { if (c) { MY_LOGD(__VA_ARGS__); } } while (0)
#define MY_LOGI_IF(c, ...) do { if (c) { MY_LOGI(__VA_ARGS__); } } while (0)
#define MY_LOGW_IF(c, ...) do { if (c) { MY_LOGW(__VA_ARGS__); } } while (0)
#define MY_LOGE_IF(c, ...) do { if (c) { MY_LOGE(__VA_ARGS__); } } while (0)
#define MY_LOGA_IF(c, ...) do { if (c) { MY_LOGW(__VA_ARGS__); } } while (0)
#define MY_LOGF_IF(c, ...) do { if (c) { MY_LOGE(__VA_ARGS__); } } while (0)

// using namespace NSCam;
// using namespace mcam;
// using namespace mcam::custom;
// using namespace mcam::custom::feature::multicam;
using mcam::custom::feature::CreationParams;
using mcam::custom::feature::IFeatureSession;

namespace mcam {
namespace custom {
namespace feature {
namespace multicam {

/******************************************************************************
 *
 ******************************************************************************/
auto MulticamSession::makeInstance(CreationParams const& rParams)
    -> std::shared_ptr<IFeatureSession> {
  auto pSession = std::make_shared<MulticamSession>(rParams);
  if (pSession == nullptr) {
    CAM_ULOGME("[%s] Bad pSession", __FUNCTION__);
    return nullptr;
  }
  return pSession;
}

/******************************************************************************
 *
 ******************************************************************************/
MulticamSession::MulticamSession(CreationParams const& rParams)
    : mSession(rParams.session),
      mCameraDeviceCallback(rParams.callback),
      mInstanceId(rParams.openId),
      mCameraCharacteristics(rParams.cameraCharateristics),
      mvSensorId(rParams.sensorId),
      mLogPrefix("multicam-session-" + std::to_string(rParams.openId)) {
  MY_LOGI("ctor");
  if (CC_UNLIKELY(mSession == nullptr || mCameraDeviceCallback == nullptr)) {
    MY_LOGE("session does not exist");
  }

  if (mvSensorId.size() != 0) {
    std::string str = "mvSensorId: ";
    for (auto const sensorId : mvSensorId) {
      str += std::to_string(sensorId);
    }
    MY_LOGD("%s", str.c_str());
  }

  if (mvPhysicalSensorList.size() != 0) {
    MY_LOGD("init mvCorePhysicalOutputStreams");
    for (auto const sensorId : mvPhysicalSensorList) {
      mvCorePhysicalOutputStreams.emplace(sensorId,
                                          std::vector<mcam::Stream>());
      MY_LOGD("config physical sensor(%d)", sensorId);
    }
  }

  IProviderCreationParams* params;
  // for build pass, must restore if postprocdevicesession is ready
  mPostProcProvider = getPostProcProviderInstance(params);
  if (mPostProcProvider == nullptr) {
    MY_LOGE("null mPostProcProvider");
  }
  // for streaming post-proc device
  auto ret =
      mPostProcProvider->getDeviceInterface(MTK_POSTPROCDEV_DEVICE_TYPE_VRP,
                                            mPostProcDevice);
  auto pPostProcListener = PostProcListener::create(this);
  mPostProcListener = std::shared_ptr<PostProcListener>(pPostProcListener);
  mPostProcDeviceSession = mPostProcDevice->open(mPostProcListener);
}

/******************************************************************************
 *
 ******************************************************************************/
MulticamSession::~MulticamSession() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
bool MulticamSession::getActiveRegionBySensorId(
        int32_t const sensorId,
        NSCam::MRect& activeArray) {
  // physical static metadata
  auto pMetaProvider = NSCam::IMetadataProvider::create(sensorId);
  if (pMetaProvider) {
    auto pMetadataProvider =
        std::shared_ptr<NSCam::IMetadataProvider>(pMetaProvider);
    auto staticMetadata =
        pMetadataProvider->getMtkStaticCharacteristics();
    //
    IMetadata::getEntry<NSCam::MRect>(&staticMetadata,
                                MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION,
                                activeArray);
    MY_LOGD("sensorId(%d), activeArray MRect(%d,%d), %dx%d",
            sensorId, activeArray.p.x, activeArray.p.y,
            activeArray.s.w, activeArray.s.h);
    return true;
  } else {
    MY_LOGE("null pMetadataProvider for sensorId(%d)", sensorId);
    return false;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
bool MulticamSession::dumpStreamBuffer(
      const StreamBuffer& streamBuf,
      std::string prefix_msg,
      uint32_t frameNumber) {
  FUNC_START
  MY_LOGE("pImgBufHeap is null! streamId:0x%X", streamBuf.streamId);
  auto streamId = streamBuf.streamId;
  auto pImgBufHeap = streamBuf.buffer;
  if (!pImgBufHeap) {
    MY_LOGE("pImgBufHeap is null! streamId:0x%X", streamBuf.streamId);
    FUNC_END
    return false;
  }

  // custom zone buffer
  auto const& corePhysicalOut_0 = mvCorePhysicalOutputStreams[cam_first][0];
  auto const& corePhysicalOut_1 = mvCorePhysicalOutputStreams[cam_second][0];

  std::string sensorStr;
  if (streamId == corePhysicalOut_0.id) {
    sensorStr = "phyCam" + std::to_string(cam_first);
  } else if (streamId == corePhysicalOut_1.id) {
    sensorStr = "phyCam" + std::to_string(cam_second);
  } else {
    sensorStr = "none";
  }

  auto imgBitsPerPixel = pImgBufHeap->getImgBitsPerPixel();
  auto imgSize = pImgBufHeap->getImgSize();
  auto imgFormat = pImgBufHeap->getImgFormat();
  auto imgStride = pImgBufHeap->getBufStridesInBytes(0);
  std::string extName;
  //
  if (imgFormat == NSCam::eImgFmt_JPEG) {
    extName = "jpg";
  } else if (imgFormat == NSCam::eImgFmt_RAW16 ||
             (imgFormat >= 0x2200 && imgFormat <= 0x2300)) {
    extName = "raw";
  } else {
    extName = "yuv";
  }
  //
  char szFileName[1024];
  snprintf(szFileName, sizeof(szFileName), "%s%s-PW_f%d_%s_%dx%d_%d_%d.%s",
           DUMP_FOLDER, prefix_msg.c_str(), frameNumber, sensorStr.c_str(),
           imgSize.w, imgSize.h,
           imgStride, streamId, extName.c_str());
  //
  auto imgBuf = pImgBufHeap->createImageBuffer();
  //
  MY_LOGD("dump image frameNum(%d) streamId(%d) to %s", frameNumber, streamId,
          szFileName);
  imgBuf->lockBuf(prefix_msg.c_str(), mcam::eBUFFER_USAGE_SW_READ_OFTEN);
  imgBuf->saveToFile(szFileName);
  imgBuf->unlockBuf(prefix_msg.c_str());
  return true;
  //
  FUNC_END
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
int MulticamSession::configureStreams(
    const mcam::StreamConfiguration& requestedConfiguration,
    mcam::HalStreamConfiguration& halConfiguration) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  // check if preview stream brings mcam::eBUFFER_USAGE_HW_COMPOSER (1<<8)
  auto isPreviewStream = [&](const auto& stream) -> bool {
    if (stream.streamType == StreamType::OUTPUT &&
        stream.format == NSCam::EImageFormat::eImgFmt_NV21 &&
        (((static_cast<uint64_t>(stream.usage) &
           mcam::eBUFFER_USAGE_HW_TEXTURE) ==
          static_cast<uint64_t>(mcam::eBUFFER_USAGE_HW_TEXTURE)) ||
         ((static_cast<uint64_t>(stream.usage) &
           mcam::eBUFFER_USAGE_HW_COMPOSER) ==
          static_cast<uint64_t>(mcam::eBUFFER_USAGE_HW_COMPOSER)))) {
      return true;
    }
    return false;
  };

  // step1. prepare input parameters for mtk zone
  mcam::StreamConfiguration customStreamConfiguration;
  mcam::StreamConfiguration postProcConfiguration;
  std::vector<mcam::Stream> cusStreams;  // customStreamConfiguration.streams;
  customStreamConfiguration.operationMode =
      requestedConfiguration.operationMode;
  customStreamConfiguration.streamConfigCounter =
      requestedConfiguration.streamConfigCounter;

  // check if device is multicam
  bool isMulticam = false;
  auto pMetadataProvider = std::shared_ptr<NSCam::IMetadataProvider>(
                            NSCam::IMetadataProvider::create(mInstanceId));
  if (pMetadataProvider) {
    IMetadata::IEntry const& capbilities =
        pMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_REQUEST_AVAILABLE_CAPABILITIES);
    if (capbilities.isEmpty()) {
      MY_LOGE(
          "No static MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA");
    } else {
      for (MUINT i = 0; i < capbilities.count(); i++) {
        if (capbilities.itemAt(i, NSCam::Type2Type<MUINT8>()) ==
            MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
          isMulticam = true;
          break;
        }
      }
    }
  }

  //  multicam -> pass sensorId, singlecam -> pass instanceId
  for (std::vector<int32_t>::const_iterator i = mvSensorId.begin();
       i != mvSensorId.end(); ++i) {
    MY_LOGD("MulticamSession mvSensorId: %d", *i);
  }
  MY_LOGD("MulticamSession mInstanceId: %d", mInstanceId);

  // step2. determine customization streams
  int32_t customStreamId = (requestedConfiguration.streams[0].id + 1) * 1000;
  int32_t customStreamIdStart = customStreamId;

  // step3. prepare image streams (example for preview stream)
  for (auto& inStream : requestedConfiguration.streams) {
    // preview stream: create working stream for mtk zone
    MY_LOGD("%s", toString(inStream).c_str());
    if (isPreviewStream(inStream)) {
      mPreviewStream = inStream;

      // multicam collect 2 physical streams from data collection
      // Create working stream for physical P1 YUV for MTK Zone
      // width/height, same as preview size
      // cam0
      {
        NSCam::MRect activeRegion_0;
        if (!getActiveRegionBySensorId(cam_first, activeRegion_0)) {
          MY_LOGE("fail to get sensor(%d) active array", cam_first);
          activeRegion_0 = NSCam::MRect(NSCam::MPoint(0,0),
                                        NSCam::MSize(4000, 3000));
        };
        mcam::Stream outputStream;
        outputStream.id = customStreamId++;
        outputStream.streamType = StreamType::OUTPUT;
        outputStream.width = inStream.width;  // 4000;
        outputStream.height = inStream.height;  // 3000;
        outputStream.format = (NSCam::EImageFormat) PHYSICAL_FORMAT;  // from P2
        outputStream.usage = inStream.usage;  // 6
        outputStream.dataSpace = inStream.dataSpace;
        outputStream.transform = NSCam::eTransform_None;
        outputStream.physicalCameraId = cam_first;
        outputStream.bufPlanes = inStream.bufPlanes;
        outputStream.bufferSize = inStream.bufferSize;
        // for (size_t i = 0; i < outputStream.bufPlanes.count; i++) {
        //   outputStream.bufferSize +=
        //     static_cast<uint32_t>(
        //       outputStream.bufPlanes.planes[i].sizeInBytes);
        // }
        outputStream.settings = inStream.settings;
        // ISP6 not support
        // IMetadata::setEntry<MINT32>(&outputStream.settings,
        //                             MTK_HALCORE_STREAM_SOURCE,
        //                             0x7001); // 0x7001(28673), R1
        // IMetadata::setEntry<NSCam::MRect>(&outputStream.settings,
        //                                   MTK_HALCORE_STREAM_CROP_REGION,
        //                                   activeRegion_0);
        cusStreams.push_back(outputStream);
        mvCorePhysicalOutputStreams[cam_first].push_back(outputStream);
      }

      // cam1
      {
        NSCam::MRect activeRegion_1;
        if (!getActiveRegionBySensorId(cam_second, activeRegion_1)) {
          MY_LOGE("fail to get sensor(%d) active array", cam_second);
          activeRegion_1 = NSCam::MRect(NSCam::MPoint(0,0),
                                        NSCam::MSize(4208, 3120));
        };
        mcam::Stream outputStream;
        outputStream.id = customStreamId++;
        outputStream.streamType = StreamType::OUTPUT;
        outputStream.width = inStream.width;  // 4208;
        outputStream.height = inStream.height;  // 3120;
        outputStream.format = (NSCam::EImageFormat) PHYSICAL_FORMAT;
        outputStream.usage = inStream.usage;  // 6
        outputStream.dataSpace = inStream.dataSpace;
        outputStream.transform = NSCam::eTransform_None;
        outputStream.physicalCameraId = cam_second;
        outputStream.bufPlanes = inStream.bufPlanes;
        outputStream.bufferSize = inStream.bufferSize;
        // for (size_t i = 0; i < outputStream.bufPlanes.count; i++) {
        //   outputStream.bufferSize +=
        //     static_cast<uint32_t>(
        //       outputStream.bufPlanes.planes[i].sizeInBytes);
        // }
        outputStream.settings = inStream.settings;
        // ISP6 not support
        // IMetadata::setEntry<MINT32>(&outputStream.settings,
        //                             MTK_HALCORE_STREAM_SOURCE,
        //                             0x7001); // 0x7001(28673), R1
        // IMetadata::setEntry<NSCam::MRect>(&outputStream.settings,
        //                                   MTK_HALCORE_STREAM_CROP_REGION,
        //                                   activeRegion_1);
        cusStreams.push_back(outputStream);
        mvCorePhysicalOutputStreams[cam_second].push_back(outputStream);
      }

      MY_LOGD("PreviewStreamId(%u),"
              "PhysicalOutStream_0(%u), PhysicalOutStream_1(%u)",
              inStream.id,
              mvCorePhysicalOutputStreams[cam_first][0].id,
              mvCorePhysicalOutputStreams[cam_second][0].id);

      // post proc input, use selected master in request time
      for (auto const& [sensorId, vCoreStreams] : mvCorePhysicalOutputStreams) {
        mcam::Stream postProcStreamIn = vCoreStreams[0];
        postProcStreamIn.streamType = StreamType::INPUT;
        mvPostProcInputStreams.emplace(sensorId, std::vector<mcam::Stream>({postProcStreamIn}));
        postProcConfiguration.streams.push_back(postProcStreamIn);
        MY_LOGD("postProcStreamIn(%u)", postProcStreamIn.id);
      }

      // post proc output, final preview
      mPostProcStreamOut.id = inStream.id;
      mPostProcStreamOut.streamType = StreamType::OUTPUT;
      mPostProcStreamOut.width = inStream.width;
      mPostProcStreamOut.height = inStream.height;
      mPostProcStreamOut.format = inStream.format;
      mPostProcStreamOut.usage = inStream.usage;
      mPostProcStreamOut.dataSpace = inStream.dataSpace;
      mPostProcStreamOut.transform = NSCam::eTransform_None;
      mPostProcStreamOut.physicalCameraId = inStream.physicalCameraId;
      mPostProcStreamOut.bufferSize = inStream.bufferSize;
      mPostProcStreamOut.bufPlanes = inStream.bufPlanes;
      mPostProcStreamOut.settings = inStream.settings;
      postProcConfiguration.streams.push_back(mPostProcStreamOut);
      MY_LOGD("mPostProcStreamOut(%u)", mPostProcStreamOut.id);

    } else {
      MY_LOGD("others streamId: %u", inStream.id);
      cusStreams.push_back(inStream);
    }
  }

  customStreamConfiguration.streams = cusStreams;

  // step4. prepare metadata
  NSCam::IMetadata cusSessionParams =
          requestedConfiguration.sessionParams;

  // add Custom Zone vendor tags
  // (1) set initial streaming sensor status (6s not support)
  // {
  //   NSCam::IMetadata::IEntry entry(MTK_HALCORE_PHYSICAL_SENSOR_STATUS);
  //   for (auto const& sensorId : mvSensorId) {
  //     int32_t status = -1;
  //     if (mPhysicalSensorSet.count(sensorId) != 0) {
  //       status = MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STREAMING;
  //     } else {
  //       status = MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STANDBY;
  //     }
  //     entry.push_back(sensorId, NSCam::Type2Type<MINT32>());  // sensorId
  //     entry.push_back(status, NSCam::Type2Type<MINT32>());  // status
  //   }
  //   int err = cusSessionParams.update(entry.tag(), entry);
  // }

  // (2) set callback type
  // eCORE_MTK_METAMERGE = 0x00000001U,
  // eCORE_MTK_METAPENDING = 0x00000002U,
  // eCORE_MTK_RULE_MASK = 0x0000000FU,
  // eCORE_AOSP_META_RULE = 0x00000010U,
  // eCORE_AOSP_RULE_MASK = 0x000000F0U,
  // eCORE_BYPASS = 0x10000000U,

  {
    NSCam::IMetadata::IEntry entry(MTK_HALCORE_CALLBACK_RULE);
    int64_t refineCbRule = EnableOption::eCORE_MTK_METAPENDING;
    entry.push_back(refineCbRule, NSCam::Type2Type<MINT64>());
    int err = cusSessionParams.update(entry.tag(), entry);
  }

  customStreamConfiguration.sessionParams = std::move(cusSessionParams);

  // dump
  MY_LOGD("======== dump all streams ========");
  for (auto& stream : requestedConfiguration.streams) {
    MY_LOGI("%s", toString(stream).c_str());
  }
  MY_LOGD("======== dump all custome streams ========");
  for (auto& stream : customStreamConfiguration.streams) {
    MY_LOGI("%s", toString(stream).c_str());
  }

  // step5. call MTK ICameraDeviceSession::configureStreams_3_6
  auto ret =
      mSession->configureStreams(customStreamConfiguration, halConfiguration);
  MY_LOGD("mSession configure result : %d", ret);

#warning "workaround sleep for async pipeline configuration"
  usleep(100 * 1000);

  // post proc
  NSCam::IMetadata postProcConfiureParams = cusSessionParams;
  postProcConfiguration.sessionParams = std::move(postProcConfiureParams);
  auto ret_postProc = mPostProcDeviceSession->configureStreams(
                        postProcConfiguration, halConfiguration);
  MY_LOGD("mPostProcDeviceSession configure result : %d", ret_postProc);


  // custom stream shouldn't return to ACameraDevice layer
  auto it = halConfiguration.streams.begin();
  for ( ; it != halConfiguration.streams.end(); ) {
    auto& stream = *it;
    if (stream.id >= customStreamIdStart) {
      MY_LOGD("custom stream id: %lld", stream.id);
      it = halConfiguration.streams.erase(it);
    } else {
      MY_LOGD("android stream id: %lld", stream.id);
      ++it;
    }
  }

  MY_LOGD("-");
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
int MulticamSession::processCaptureRequest(
    const std::vector<mcam::CaptureRequest>& requests,
    uint32_t& numRequestProcessed) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();

  // MY_LOGD("======== dump all requests ========");
  // for (auto& request : requests) {
  //   MY_LOGI("%s", toString(request).c_str());
  //   for (auto& streambuffer : request.outputBuffers) {
  //     MY_LOGI("%s", toString(streambuffer).c_str());
  //   }
  // }

  int status = 0;  // OK;
  {
    if (CC_UNLIKELY(mSession == nullptr)) {
      MY_LOGE("Bad CameraDevice3Session");
      return 7;  // Status::ILLEGAL_ARGUMENT;
    }
    std::vector<mcam::CaptureRequest> cusCaptureRequests;
    cusCaptureRequests.resize(requests.size());
    MY_LOGD("preserve cusCaptureRequests size(%zu) from requests size(%zu)",
            cusCaptureRequests.size(), requests.size());

    for (size_t i = 0; i < requests.size(); i++) {
      // step1. prepare input parameters for mtk zone
      auto& cusCaptureRequest = cusCaptureRequests[i];
      cusCaptureRequest.frameNumber = requests[i].frameNumber;
      cusCaptureRequest.settings = requests[i].settings;
      cusCaptureRequest.inputBuffers = requests[i].inputBuffers;
      cusCaptureRequest.halSettings = requests[i].halSettings;
      cusCaptureRequest.subRequests = requests[i].subRequests;
      cusCaptureRequest.physicalCameraSettings =
          requests[i].physicalCameraSettings;
      MY_LOGD("processCaptureRequest request(%d)",
               cusCaptureRequest.frameNumber);

      MY_LOGD("inputBuffers size:%d", cusCaptureRequest.inputBuffers.size());

      // Native ISP I/F preparation
      std::lock_guard<std::mutex> _l(mInflightMapLock);
      auto& inflightReq = mInflightRequests[requests[i].frameNumber];
      auto& postProc = inflightReq.mPostProc;
      inflightReq.frameNumber = requests[i].frameNumber;

      auto& cusStreamBuffers = cusCaptureRequest.outputBuffers;
      cusStreamBuffers.resize(requests[i].outputBuffers.size());
      MY_LOGD("outputBuffers size:%d", requests[i].outputBuffers.size());

      size_t imgCount = 0;
      for (auto& streamBuffer : requests[i].outputBuffers) {
        // preview stream: create working buffer for mtk zone
        // prepare image streams (example for preview stream)
        if (streamBuffer.streamId == mPreviewStream.id) {
          // 1. add working preview buffer (replace preview buffer)
          // 2. add mvCorePhysicalOutputStreams
          // cusStreamBuffer size: replace preview with 2 physical stream
          cusStreamBuffers.resize(requests[i].outputBuffers.size() - 1 + 2);
          MY_LOGD("cusStreamBuffers size:%d", cusStreamBuffers.size());

          // step4-1. prepare physical image stream 0
          {
            auto& corePhysicalOut = mvCorePhysicalOutputStreams[cam_first][0];
            auto& physicalOutStreamBuffer_0 = cusStreamBuffers[imgCount++];
            physicalOutStreamBuffer_0.streamId = corePhysicalOut.id;
            physicalOutStreamBuffer_0.status = mcam::BufferStatus::OK;
            physicalOutStreamBuffer_0.bufferSettings = streamBuffer.bufferSettings;
            physicalOutStreamBuffer_0.bufferId = mWorkingStreamBufferSerialNo++;

            // allocate buffer
            mcam::IImageBufferAllocator::ImgParam imgParam_physical_0(
                (NSCam::EImageFormat) PHYSICAL_FORMAT,
                NSCam::MSize(corePhysicalOut.width,
                             corePhysicalOut.height),
                             corePhysicalOut.usage);
            std::string str_phy_0 = "customPhysicalOutStream_0";
            auto pImageBufferHeap = mcam::IImageBufferHeapFactory::create(
                LOG_TAG, str_phy_0.c_str(), imgParam_physical_0, true);
            physicalOutStreamBuffer_0.buffer = pImageBufferHeap;
            inflightReq.mPhysicalBufferHandle_0 = physicalOutStreamBuffer_0.buffer;
            MY_LOGD("working buffer: physical_0");
          }

          // step4-2. prepare physical image stream 1
          {
            auto& corePhysicalOut = mvCorePhysicalOutputStreams[cam_second][0];
            auto& physicalOutStreamBuffer_1 = cusStreamBuffers[imgCount++];
            physicalOutStreamBuffer_1.streamId = corePhysicalOut.id;
            physicalOutStreamBuffer_1.status = mcam::BufferStatus::OK;
            physicalOutStreamBuffer_1.bufferSettings = streamBuffer.bufferSettings;
            physicalOutStreamBuffer_1.bufferId = mWorkingStreamBufferSerialNo++;

            // allocate buffer
            mcam::IImageBufferAllocator::ImgParam imgParam_physical_1(
                (NSCam::EImageFormat) PHYSICAL_FORMAT,
                NSCam::MSize(corePhysicalOut.width,
                             corePhysicalOut.height),
                             corePhysicalOut.usage);
            std::string str_phy_1 = "customPhysicalOutStream_1";
            auto pImageBufferHeap = mcam::IImageBufferHeapFactory::create(
                LOG_TAG, str_phy_1.c_str(), imgParam_physical_1, true);
            physicalOutStreamBuffer_1.buffer = pImageBufferHeap;
            inflightReq.mPhysicalBufferHandle_1 = physicalOutStreamBuffer_1.buffer;
            MY_LOGD("working buffer: physical_1");
          }

          // 2 physical stream
          postProc.numInputBuffers = 2;  // wait for 2 and select master
          postProc.numOutputBuffers = 1;
          postProc.readyOutputBuffers = 1;

          inflightReq.mFwkPreviewBufferId = streamBuffer.bufferId;
          inflightReq.mFwkPreviewBufferHd = streamBuffer.buffer;

        } else {
          cusStreamBuffers[imgCount++] = streamBuffer;
          MY_LOGD("others: %s",
                  toString(cusStreamBuffers[imgCount - 1]).c_str());
        }

        // step5. clone from original input settings from framework
        if (!(requests[i].settings.isEmpty())) {
          NSCam::IMetadata inputSettings = requests[i].settings;
          // read zoom ratio
          {
            auto iDebugZoomRatio =
                  property_get_int32("vendor.debug.camera.zoomRatio", 0);
            IMetadata::IEntry entry =
              inputSettings.entryFor(MTK_CONTROL_ZOOM_RATIO);
            if (iDebugZoomRatio != 0) {
              inflightReq.mZoomRatio = (iDebugZoomRatio * 1.0f) / 10;
              MY_LOGE("force zoom ratio");
            } else if (!entry.isEmpty()) {
              inflightReq.mZoomRatio =
                entry.itemAt(0, NSCam::Type2Type<MFLOAT>());
              MY_LOGD("use app meta zoom ratio");
            } else {
              MY_LOGE("MTK_CONTROL_ZOOM_RATIO not found.");
            }
          }
          MY_LOGD("req zoomRatio(%f)", inflightReq.mZoomRatio);

          // Request time: set Custom Zone tag
          // (1) set master id
          {
            int32_t masterId = cam_first;
            if (inflightReq.mZoomRatio >= 2.0) {
              masterId = cam_second;
            }
            masterId = property_get_int32(
                        "vendor.debug.camera.custzone.mastercam", masterId);
            MY_LOGD("custom zone: mastercam(%d)", masterId);
            NSCam::IMetadata::IEntry entry(MTK_HALCORE_PHYSICAL_MASTER_ID);
            entry.push_back(masterId, NSCam::Type2Type<MINT32>());  // preview
            inputSettings.update(entry.tag(), entry);
          }

          // (2) set sensor status
          {
            char tempbuf[1024];
            property_get(
              "vendor.debug.camera.custzone.forceStreamingIds", tempbuf, "-1");
            bool forceSensorStreaming = (0 != strcmp(tempbuf, "-1"));
            std::set<int> parsedSensors;
            if (forceSensorStreaming) {
              std::istringstream iss((std::string(tempbuf)));
              for (std::string token; std::getline(iss, token, ','); ) {
                parsedSensors.emplace(std::stoi(token));
              }
            }
            NSCam::IMetadata::IEntry entry(MTK_HALCORE_PHYSICAL_SENSOR_STATUS);
            for (auto const& sensorId : mvSensorId) {
              int32_t status = -1;
              std::string dumpStr;
              if (forceSensorStreaming) {
                // default set first 2 sensors as streaming
                if (parsedSensors.count(sensorId) != 0) {
                  status = MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STREAMING;
                  dumpStr += "streaming";
                } else {
                  status = MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STANDBY;
                  dumpStr += "standby";
                }
                dumpStr += "(debug)";
              } else {
                // default set first 2 sensors as streaming
                if (mPhysicalSensorSet.count(sensorId) != 0) {
                  status = MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STREAMING;
                  dumpStr += "streaming";
                } else {
                  status = MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STANDBY;
                  dumpStr += "standby";
                }
              }
              entry.push_back(sensorId, NSCam::Type2Type<MINT32>());
              entry.push_back(status, NSCam::Type2Type<MINT32>());
              MY_LOGD("custom zone: cam%d %s", sensorId, dumpStr.c_str());
            }
            inputSettings.update(entry.tag(), entry);
          }
          mLastSettings.clear();
          mLastSettings = inputSettings;

#warning \
  "implementor must repeat settings if break settings transferred by fwk"
          // user could append customized control metadata in cusSettings
          // however, no operation in this case
          cusCaptureRequests[i].settings = std::move(mLastSettings);
        }
        postProc.pAppCtrl = mLastSettings;
      }
    }
    MY_LOGD("======== dump all original requests (%zu)========",
            requests.size());
    MY_LOGD("======== dump all customized requests (%zu)========",
            cusCaptureRequests.size());
    std::lock_guard<std::mutex> _l(mInflightMapLock);
    for (auto& iter : mInflightRequests) {
      if (iter.second.bRemovable) {
        mInflightRequests.erase(iter.second.frameNumber);
      }
    }
    MY_LOGD("-");
    return mSession->processCaptureRequest(cusCaptureRequests,
                                           numRequestProcessed);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
int MulticamSession::flush() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return 7;  // Status::INTERNAL_ERROR
  }

  // flush/close HAL3+
  MY_LOGD("mSession device flush + ");
  auto status = mSession->flush();
  MY_LOGD("mSession device flush - ");
  MY_LOGD("MulticamSession PostProc flush + ");
  auto pstatus = mPostProcDeviceSession->flush();
  MY_LOGD("MulticamSession PostProc flush - ");

  for (auto& inflight : mInflightRequests) {
    inflight.second.dump();
    if (!inflight.second.bRemovable) {
      // notify error
      mcam::NotifyMsg hidl_msg = {};
      hidl_msg.type = mcam::MsgType::ERROR;
      hidl_msg.msg.error.frameNumber = inflight.second.frameNumber;
      hidl_msg.msg.error.errorStreamId = -1;
      hidl_msg.msg.error.errorCode = mcam::ErrorCode::ERROR_REQUEST;
      mCameraDeviceCallback->notify({hidl_msg});

      // processCaptureResult buffer
      mcam::CaptureResult imageResult;
      std::vector<mcam::StreamBuffer> inputBuf;
      inputBuf.push_back({.streamId = -1});
      imageResult.frameNumber = inflight.second.frameNumber;
      imageResult.result = IMetadata();
      imageResult.outputBuffers.resize(1);
      imageResult.inputBuffers = inputBuf;
      imageResult.bLastPartialResult = false;
      //
      auto& streamBuffer = imageResult.outputBuffers[0];
      streamBuffer.streamId = mPreviewStream.id;
      streamBuffer.bufferId = inflight.second.mFwkPreviewBufferId;
      streamBuffer.buffer = nullptr;
      streamBuffer.status = mcam::BufferStatus::ERROR;
      streamBuffer.acquireFenceFd = -1;
      streamBuffer.releaseFenceFd = -1;
      mCameraDeviceCallback->processCaptureResult({imageResult});
      inflight.second.bRemovable = true;
    }
  }
  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
int MulticamSession::close() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto result = mSession->close();
  if (CC_UNLIKELY(result != 0)) {
    MY_LOGE("MulticamSession close fail");
  }
  mSession = nullptr;

  if (mPostProcDeviceSession != nullptr) {
    auto ret = mPostProcDeviceSession->close();
    if (CC_UNLIKELY(ret != 0)) {
      MY_LOGE("PostProcDeviceSession close() fail");
    }
    MY_LOGD("PostProcDeviceSession close done");
  }

  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
void MulticamSession::notify(const std::vector<mcam::NotifyMsg>& msgs) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  if (msgs.size()) {
    mCameraDeviceCallback->notify(msgs);
    // for (auto& msg : msgs) {
    //   MY_LOGI("msgs: %s", toString(msg).c_str());
    // }
    std::vector<mcam::NotifyMsg> vMsgs;
    for (auto& msg : msgs) {
      bool skip = false;
      if (msg.type == mcam::MsgType::ERROR) {
        if (msg.msg.error.errorCode == mcam::ErrorCode::ERROR_BUFFER) {
          if (msg.msg.error.errorStreamId ==
                mvCorePhysicalOutputStreams[cam_first][0].id) {
            MY_LOGE("skip custom zone stream callback: physical_0");
          }
          else if (msg.msg.error.errorStreamId ==
                mvCorePhysicalOutputStreams[cam_second][0].id) {
            MY_LOGE("skip custom zone stream callback: physical_1");
          } else {
            vMsgs.push_back(msg);
          }
        } else if (msg.msg.error.errorCode ==
                   mcam::ErrorCode::ERROR_REQUEST) {
          mInflightRequests[msg.msg.error.frameNumber].bRemovable = true;
          vMsgs.push_back(msg);
        } else if (msg.msg.error.errorCode ==
                   mcam::ErrorCode::ERROR_RESULT) {
          mInflightRequests[msg.msg.error.frameNumber].isErrorResult = true;
        } else {
          vMsgs.push_back(msg);
        }
      } else {
        vMsgs.push_back(msg);
      }
    }
    std::vector<mcam::NotifyMsg> vHidlMsgs;
    vHidlMsgs = vMsgs;
    MY_LOGI("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
    mCameraDeviceCallback->notify(vHidlMsgs);
  } else {
    MY_LOGE("msgs size is zero, bypass the msgs");
  }

  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
auto MulticamSession::InflightRequest::dump() -> void {
  ALOGI("InflightRequest:%s: frameNumber(%u) mPostProc(%p) bRemovable(%d)",
        __FUNCTION__, frameNumber, &mPostProc, bRemovable);

  ALOGI("InflightRequest:%s: PostProc.pAppCtrl(%p) PostProc.pAppResult(%p)",
    __FUNCTION__, mPostProc.pAppCtrl.isEmpty(), mPostProc.pAppResult.isEmpty());

  ALOGI(
      "InflightRequest:%s: "
      "PostProc.numInputBuffers(%zu) PostProc.numOutputBuffers(%zu)",
      __FUNCTION__, mPostProc.numInputBuffers, mPostProc.numOutputBuffers);

  ALOGI(
      "InflightRequest:%s: "
      "PostProc.readyInputBuffers(%zu) PostProc.readyOutputBuffers(%zu)",
      __FUNCTION__, mPostProc.readyInputBuffers, mPostProc.readyOutputBuffers);
}


/******************************************************************************
 *
 ******************************************************************************/
bool MulticamSession::processMasterSelection(
        const mcam::CaptureResult& result,
        mcam::StreamBuffer& selectedBuffer) {
  MY_LOGD("+");
  auto& inflightRequest = mInflightRequests[result.frameNumber];
  // decide master by ratio
  int32_t targetPhysicalId = -1;
  if (inflightRequest.mZoomRatio >= 2.0) {
    targetPhysicalId = 2;
    selectedBuffer.streamId = mvCorePhysicalOutputStreams[cam_second][0].id;
    selectedBuffer.bufferId = inflightRequest.mPhysicalBufferId_1;
    selectedBuffer.buffer = inflightRequest.mPhysicalBufferHandle_1;
  } else {
    targetPhysicalId = 0;
    selectedBuffer.streamId = mvCorePhysicalOutputStreams[cam_first][0].id;
    selectedBuffer.bufferId = inflightRequest.mPhysicalBufferId_0;
    selectedBuffer.buffer = inflightRequest.mPhysicalBufferHandle_0;
  }
  MY_LOGD("select cam(%d) buffer.", targetPhysicalId);
  MY_LOGD("-");
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
void MulticamSession::processOneCaptureResult(
    const mcam::CaptureResult& result) {
  struct BufferInfo {
    int64_t streamId = -1;
    uint64_t bufferId = 0;
    bool bError = true;
    std::shared_ptr<mcam::IImageBufferHeap> buffer = nullptr;
  };

  auto isBufferOK = [](auto& streamBuffer) -> bool {
    return streamBuffer.status == mcam::BufferStatus::OK;
  };

  // dump physical here
  for (auto& streamBuf : result.outputBuffers) {
    dumpStreamBuffer(streamBuf, "Debug", result.frameNumber);
  }

  //
  {
    std::lock_guard<std::mutex> _l(mInflightMapLock);
    auto& inflightReq = mInflightRequests[result.frameNumber];
    auto& postProc = inflightReq.mPostProc;
    // callback for images
    std::vector<BufferInfo> vBufInfos;
    std::vector<mcam::StreamBuffer> vOutBuffers;

    // output streams
    if (result.outputBuffers.size()) {
      for (auto& streamBuffer : result.outputBuffers) {

        // if error preview buffer,
        // prepare error buffer for framework stream
        auto const& corePhysicalOut_0 = mvCorePhysicalOutputStreams[cam_first][0];
        auto const& corePhysicalOut_1 = mvCorePhysicalOutputStreams[cam_second][0];

        // physical stream doesn't need to return to framework
        if (streamBuffer.streamId == corePhysicalOut_0.id) {
          // cam0 physical buffer
          if (isBufferOK(streamBuffer)) {
            MY_LOGI("cam0: receive physical stream buffer id(%d)",
                    corePhysicalOut_0.id);
            postProc.readyInputBuffers++;
          }
          else {
            MY_LOGI("cam0: receive error physical stream buffer id(%d)",
                    corePhysicalOut_0.id);
          }
        } else if (streamBuffer.streamId == corePhysicalOut_1.id) {
          // cam1 physical buffer
          if (isBufferOK(streamBuffer)) {
            MY_LOGI("cam1: receive physical stream buffer id(%d)",
                    corePhysicalOut_1.id);
            postProc.readyInputBuffers++;
          }
          else {
            MY_LOGI("cam1: receive error physical stream buffer id(%d)",
                    corePhysicalOut_1.id);
          }
        } else {
          vBufInfos.push_back({.streamId = streamBuffer.streamId,
                               .bufferId = streamBuffer.bufferId,
                               .bError = (streamBuffer.status ==
                                          mcam::BufferStatus::ERROR),
                               .buffer = streamBuffer.buffer});
        }
      }

      // callback other image streams except 2 physical stream
      if (vBufInfos.size()) {
        std::vector<mcam::StreamBuffer> inputBuf;
        inputBuf.push_back({.streamId = -1});
        mcam::CaptureResult imageResult;
        imageResult.frameNumber = result.frameNumber;
        imageResult.result = NSCam::IMetadata();
        imageResult.outputBuffers.resize(vBufInfos.size());
        imageResult.inputBuffers = inputBuf;
        imageResult.bLastPartialResult = false;

        for (size_t i = 0; i < vBufInfos.size(); ++i) {
          auto& streamBuffer = imageResult.outputBuffers[i];
          streamBuffer.streamId = vBufInfos[i].streamId;
          streamBuffer.bufferId = vBufInfos[i].bufferId;
          streamBuffer.buffer = vBufInfos[i].buffer;
          streamBuffer.status = (vBufInfos[i].bError)
                                    ? mcam::BufferStatus::ERROR
                                    : mcam::BufferStatus::OK;
          streamBuffer.acquireFenceFd = -1;
          streamBuffer.releaseFenceFd = -1;
        }

        MY_LOGD("output stream id:%d", imageResult.outputBuffers[0].streamId);

        if (mCameraDeviceCallback.get()) {
          mCameraDeviceCallback->processCaptureResult({imageResult});
        }
      }
    }

    // input stream
    auto inputBuf = result.inputBuffers;
    if (inputBuf.size()) {
      if (inputBuf[0].streamId != -1) {
        // ALOGI("input stream: %s", toString(result).c_str());
        MY_LOGD("input stream id:%d", inputBuf[0].streamId);
        if (mCameraDeviceCallback.get()) {
          mCameraDeviceCallback->processCaptureResult({result});
        }
      }
    }
    // metadata
    append_Result += result.result;

    if (result.bLastPartialResult == true) {
      postProc.pAppResult = append_Result;
      MY_LOGD("PostProc.pAppResult(%p)", &(postProc.pAppResult));
      MY_LOGD("receive_last_partial");
    }
    //

    if (inflightReq.isErrorResult == false) {
      mcam::CaptureResult metaResult;
      metaResult.frameNumber = result.frameNumber;
      metaResult.result = result.result;
      metaResult.halResult = result.halResult;
      metaResult.physicalCameraMetadata = result.physicalCameraMetadata;
      metaResult.outputBuffers = {};
      // metaResult.inputBuffers = {{.streamId = -1}};
      metaResult.inputBuffers = {};
      metaResult.bLastPartialResult = false;

      ALOGD("lhs callback, bLastPartialResult: %d",
            metaResult.bLastPartialResult);

      if (mCameraDeviceCallback.get()) {
        mCameraDeviceCallback->processCaptureResult({metaResult});
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void MulticamSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  MY_LOGI("results(%zu)", results.size());
  uint32_t numPostProcRequestProcessed = 0;
  for (auto& result : results) {
    processOneCaptureResult(result);
    IMetadata halDumpMeta = result.halResult;
    MY_LOGD("halResult is empty(%d)", halDumpMeta.isEmpty());

    std::lock_guard<std::mutex> _l(mInflightMapLock);
    auto& inflightReq = mInflightRequests[result.frameNumber];
    auto& postProc = inflightReq.mPostProc;

    if (inflightReq.bRemovable == true) {
      ALOGI("already removed before, neglect callback for this frame");
      return;
    }

    // Native ISP processing when all images/metadata are prepared
    MY_LOGD("LHS result frame number: %d", result.frameNumber);
    MY_LOGD("LHS inflight request frame number: %d", inflightReq.frameNumber);
    inflightReq.dump();

    if (!(halDumpMeta.isEmpty())) {
      inflightReq.mPostProc.pHalResult = result.halResult;
    }

    // we currently has no post proc,
    // but we leverage this to receive all physical and do preview selection
    if (inflightReq.isReadyForPostProc()) {
      MY_LOGD("send post proc request +");
      inflightReq.mPreviewBufferId++;

      // dump

      mcam::CaptureRequest postProcRequest;
      mcam::StreamBuffer outputBuffer;
      outputBuffer.streamId = mPreviewStream.id;
      outputBuffer.buffer = inflightReq.mFwkPreviewBufferHd;
      outputBuffer.bufferId = inflightReq.mFwkPreviewBufferId;
      //
      mcam::StreamBuffer inputBuffer;
      processMasterSelection(result, inputBuffer);
      //
      postProcRequest.outputBuffers = {outputBuffer};
      postProcRequest.inputBuffers = {inputBuffer};
      postProcRequest.frameNumber = inflightReq.frameNumber;
      postProcRequest.settings = inflightReq.mPostProc.pAppCtrl;
      postProcRequest.halSettings = inflightReq.mPostProc.pHalResult;

      std::vector<mcam::CaptureRequest> postProcRequests = {postProcRequest};
      MY_LOGD("postProcRequest frame No: %d", postProcRequest.frameNumber);
      mPostProcDeviceSession->processCaptureRequest(
          postProcRequests, numPostProcRequestProcessed);
      MY_LOGD("send post proc request -");
    }
  }
  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
void MulticamSession::postProcProcessCompleted(
    const std::vector<mcam::CaptureResult>& results) {
  ALOGD("postProcProcessCompleted: %s +", __FUNCTION__);
  MY_LOGD("postProcProcessCompleted + ");

  std::vector<mcam::CaptureResult> imageResults;
  std::vector<mcam::CaptureResult> metadataResults;

  for (auto& result : results) {
    auto frameNumber = result.frameNumber;
    auto& inflightRequest = mInflightRequests[frameNumber];

    ALOGI("postProcProcessCompleted:%s: results.reqNumber(%u)", __FUNCTION__,
          result.frameNumber);

    if (inflightRequest.bRemovable == true) {
      ALOGI("already removed before, neglect callback for this frame");
      return;
    }

    // prepare buffer callback
    if (inflightRequest.mFwkPreviewBufferHd) {
      mcam::CaptureResult captureResult;
      captureResult.frameNumber = frameNumber;
      captureResult.outputBuffers.resize(1);

      auto& streamBuffer = captureResult.outputBuffers[0];
      streamBuffer.streamId = mPreviewStream.id;
      streamBuffer.bufferId = inflightRequest.mFwkPreviewBufferId;
      streamBuffer.buffer = result.outputBuffers[0].buffer;
      streamBuffer.status = mcam::BufferStatus::OK;
      imageResults = {captureResult};
      if (mCameraDeviceCallback.get()) {
        mCameraDeviceCallback->processCaptureResult(imageResults);
      }
    }

    if (inflightRequest.isErrorResult == false) {
      {
        mcam::CaptureResult captureResult;
        captureResult.frameNumber = frameNumber;
        std::vector<mcam::StreamBuffer> inputBuf;
        inputBuf.push_back({.streamId = -1});
        IMetadata final;
        IMetadata hidl_meta;
        if (!(result.result.isEmpty())) {
          captureResult.result = std::move(hidl_meta);
        } else {
          uint8_t bFrameEnd = 1;
          IMetadata::setEntry<uint8_t>(&final, MTK_HALCORE_REQUEST_END,
                                       bFrameEnd);
          hidl_meta = final;
          captureResult.result = std::move(hidl_meta);
        }
        captureResult.outputBuffers = {};
        captureResult.bLastPartialResult = true;

        ALOGD("NativeCallback_bLastPartialResult: %d",
              captureResult.bLastPartialResult);
        metadataResults = {captureResult};
        if (mCameraDeviceCallback.get()) {
          mCameraDeviceCallback->processCaptureResult(metadataResults);
        }
        if (!(final.isEmpty()))
          final.clear();
      }
    }
    inflightRequest.bRemovable = true;
  }
}

/******************************************************************************
 * postProcListener Interface
 ******************************************************************************/

MulticamSession::PostProcListener::PostProcListener(MulticamSession* pParent) {
  mParent = pParent;
}

/******************************************************************************
 * postProcListener Interface
 ******************************************************************************/
auto MulticamSession::PostProcListener::create(MulticamSession* pParent)
    -> PostProcListener* {
  auto pInstance = new PostProcListener(pParent);
  return pInstance;
}

/******************************************************************************
 * postProcListener Interface
 ******************************************************************************/
void MulticamSession::PostProcListener::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  mParent->postProcProcessCompleted(results);
}

/******************************************************************************
 * postProcListener Interface
 ******************************************************************************/
void MulticamSession::PostProcListener::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  ALOGE("MulticamSession::PostProcListener::notify: %s Not implemented ",
        __FUNCTION__);
}

};      // namespace multicam
};      // namespace feature
};      // namespace custom
};      // namespace mcam
