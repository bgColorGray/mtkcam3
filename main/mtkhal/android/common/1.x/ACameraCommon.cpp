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

#include <main/mtkhal/android/common/1.x/ACameraCommon.h>

#include <cutils/properties.h>
#include <utils/RefBase.h>

#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metadata/IMetadataTagSet.h>
#include <mtkcam/utils/cat/CamAutoTestLogger.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>

#include <utility>
#include <vector>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using mcam::IGraphicImageBufferHeap;
using NSCam::IMetadata;

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
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
static const ::android::sp<NSCam::IMetadataConverter> sMetadataConverter =
    NSCam::IMetadataConverter::createInstance(
        NSCam::IDefaultMetadataTagSet::singleton()->getTagSet());

/******************************************************************************
 *
 ******************************************************************************/
auto convertStatus(mcam::Status status) -> Status {
  int res = -static_cast<int>(status);
  MY_LOGI_IF(res != 0, "convertStatus [%d %s]", res, ::strerror(res));

  return static_cast<Status>(status);
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamConfiguration(const StreamConfiguration& srcStreams,
                                mcam::StreamConfiguration& dstStreams)
    -> int {
  int status = 0;
  int res = 0;
  // streams
  res = convertStreams(srcStreams.streams, dstStreams.streams);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // operationMode
  dstStreams.operationMode = static_cast<mcam::StreamConfigurationMode>(
      srcStreams.operationMode);
  // sessionParams
  res = convertMetadata(srcStreams.sessionParams, dstStreams.sessionParams);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // streamConfigCounter
  dstStreams.streamConfigCounter = srcStreams.streamConfigCounter;
  // sensorIdSet: todo

  // dump
  MY_LOGD("%s", toString(srcStreams).c_str());
  sMetadataConverter->dumpAll(dstStreams.sessionParams);

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreams(const std::vector<Stream>& srcStreams,
                    std::vector<mcam::Stream>& dstStreams) -> int {
  int status = 0;
  int res = 0;
  dstStreams.resize(srcStreams.size());
  for (int i = 0; i < srcStreams.size(); i++) {
    res = convertStream(srcStreams[i], dstStreams[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStream(const Stream& srcStream, mcam::Stream& dstStream)
    -> int {
  int status = 0;
  int res = 0;
  // id
  dstStream.id = srcStream.id;
  // streamType
  dstStream.streamType =
      static_cast<mcam::StreamType>(srcStream.streamType);
  // width
  dstStream.width = srcStream.width;
  // height
  dstStream.height = srcStream.height;
  // format
  dstStream.format = static_cast<NSCam::EImageFormat>(srcStream.format);
  // usage
  dstStream.usage = srcStream.usage;
  // dataSpace
  dstStream.dataSpace = static_cast<mcam::Dataspace>(srcStream.dataSpace);
  // transform
  res = convertStreamRotation(srcStream.rotation, dstStream.transform);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // physicalCameraId
  dstStream.physicalCameraId = srcStream.physicalCameraId;
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // bufferSize
  dstStream.bufferSize = srcStream.bufferSize;
  // settings: todo
  // imgProcSettings: todo
  // bufPlanes: todo
  // batchSize: todo

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamRotation(const StreamRotation& srcStreamRotation,
                           uint32_t& dstTransform) -> int {
  // Note: StreamRotation is applied counterclockwise,
  //       and Transform is applied clockwise.
  switch (srcStreamRotation) {
    case StreamRotation::ROTATION_0:
      dstTransform = NSCam::eTransform_None;
      break;
    case StreamRotation::ROTATION_90:
      dstTransform = NSCam::eTransform_ROT_270;
      break;
    case StreamRotation::ROTATION_180:
      dstTransform = NSCam::eTransform_ROT_180;
      break;
    case StreamRotation::ROTATION_270:
      dstTransform = NSCam::eTransform_ROT_90;
      break;
    default:
      MY_LOGE("Unknown stream rotation: %u", srcStreamRotation);
      return -EINVAL;
  }

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertHalStreamConfiguration(
    const mcam::HalStreamConfiguration& srcStreams,
    HalStreamConfiguration& dstStreams) -> int {
  int status = 0;
  int res = 0;
  // streams
  res = convertHalStreams(srcStreams.streams, dstStreams.streams);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }

  // dump
  for (auto& stream : dstStreams.streams) {
    MY_LOGD("stream.id(%d) stream.maxBuffers(%u)", stream.id,
            stream.maxBuffers);
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertHalStreams(const std::vector<mcam::HalStream>& srcStreams,
                       std::vector<HalStream>& dstStreams) -> int {
  int status = 0;
  int res = 0;
  dstStreams.resize(srcStreams.size());
  for (int i = 0; i < srcStreams.size(); i++) {
    res = convertHalStream(srcStreams[i], dstStreams[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertHalStream(const mcam::HalStream& srcStream,
                      HalStream& dstStream) -> int {
  // id
  dstStream.id = srcStream.id;
  // overrideFormat
  dstStream.overrideFormat =
      static_cast<android_pixel_format_t>(srcStream.overrideFormat);
  // producerUsage
  dstStream.producerUsage = static_cast<uint64_t>(srcStream.producerUsage);
  // consumerUsage
  dstStream.consumerUsage = static_cast<uint64_t>(srcStream.consumerUsage);
  // maxBuffers
  dstStream.maxBuffers = srcStream.maxBuffers;
  // overrideDataSpace
  dstStream.overrideDataSpace =
      static_cast<android_dataspace_t>(srcStream.overrideDataSpace);
  // physicalCameraId
  dstStream.physicalCameraId = srcStream.physicalCameraId;
  // supportOffline
  dstStream.supportOffline = srcStream.supportOffline;

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertCaptureRequests(
    const std::vector<CaptureRequest>& srcRequests,
    std::vector<mcam::CaptureRequest>& dstRequests) -> int {
  int status = 0;
  int res = 0;
  dstRequests.resize(srcRequests.size());
  for (int i = 0; i < srcRequests.size(); i++) {
    printAutoTestLog(srcRequests[i].settings, false);
    res = convertCaptureRequest(srcRequests[i], dstRequests[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertCaptureRequest(const CaptureRequest& srcRequest,
                           mcam::CaptureRequest& dstRequest) -> int {
  int status = 0;
  int res = 0;
  // frameNumber
  dstRequest.frameNumber = srcRequest.frameNumber;
  // settings
  if (srcRequest.settings) {
    res = convertMetadata(srcRequest.settings, dstRequest.settings);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }
  // inputBuffers
  if (srcRequest.inputBuffer.streamId != NO_STREAM) {
    dstRequest.inputBuffers.resize(1);
    res =
        convertStreamBuffer(srcRequest.inputBuffer, dstRequest.inputBuffers[0]);
  }
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // outputBuffers
  res =
      convertStreamBuffers(srcRequest.outputBuffers, dstRequest.outputBuffers);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // physicalCameraSettings
  dstRequest.physicalCameraSettings.resize(
      srcRequest.physicalCameraSettings.size());
  for (int i = 0; i < srcRequest.physicalCameraSettings.size(); i++) {
    auto& srcSetting = srcRequest.physicalCameraSettings[i];
    auto& dstSetting = dstRequest.physicalCameraSettings[i];
    // physicalCameraId
    dstSetting.physicalCameraId = srcSetting.physicalCameraId;
    // settings
    res = convertMetadata(srcSetting.settings, dstSetting.settings);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }
  // halSettings: todo

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertMetadata(const camera_metadata* const& srcMetadata,
                     IMetadata& dstMetadata) -> int {
  if (srcMetadata == nullptr ||
      get_camera_metadata_entry_count(srcMetadata) == 0) {
    MY_LOGD("null srcMetadata");
    return 0;
  }
  if (CC_UNLIKELY(!sMetadataConverter->convert(srcMetadata, dstMetadata))) {
    MY_LOGE("srcEntryCount=%zu, dstEntryCount=%zu",
            get_camera_metadata_entry_count(srcMetadata), dstMetadata.count());
    MY_LOGE("convert failed: camera_metadata(%p) -> IMetadata", srcMetadata);
    return EINVAL;
  }

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertMetadata(const IMetadata& srcMetadata,
                     const camera_metadata*& dstMetadata) -> int {
  size_t size = 0;
  camera_metadata* nonConstDstMetadata = nullptr;
  if (srcMetadata.count() > 0) {
    if (CC_UNLIKELY(!sMetadataConverter->convert(srcMetadata,
                                                 nonConstDstMetadata, &size) ||
                    !nonConstDstMetadata || size == 0)) {
      MY_LOGE("srcEntryCount=%zu, dstEntryCount=%zu", srcMetadata.count(),
              get_camera_metadata_entry_count(nonConstDstMetadata));
      MY_LOGE("convert failed: IMetadata -> camera_metadata(%p) size:%zu",
              nonConstDstMetadata, size);
    }
  }
  dstMetadata = nonConstDstMetadata;

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBuffers(const std::vector<StreamBuffer>& srcBuffers,
                          std::vector<mcam::StreamBuffer>& dstBuffers)
    -> int {
  int status = 0;
  int res = 0;
  dstBuffers.resize(srcBuffers.size());
  for (int i = 0; i < srcBuffers.size(); i++) {
    res = convertStreamBuffer(srcBuffers[i], dstBuffers[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBuffers(
    const std::vector<mcam::StreamBuffer>& srcBuffers,
    std::vector<StreamBuffer>& dstBuffers) -> int {
  int status = 0;
  int res = 0;
  dstBuffers.resize(srcBuffers.size());
  for (int i = 0; i < srcBuffers.size(); i++) {
    res = convertStreamBuffer(srcBuffers[i], dstBuffers[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBuffer(const StreamBuffer& srcBuffer,
                         mcam::StreamBuffer& dstBuffer) -> int {
  int status = 0;
  int res = 0;
  // streamId
  dstBuffer.streamId = srcBuffer.streamId;
  // bufferId
  dstBuffer.bufferId = srcBuffer.bufferId;
  // buffer: user should create ImageBufferHeap
  // status
  dstBuffer.status = static_cast<mcam::BufferStatus>(srcBuffer.status);
  // acquireFenceFd
  dstBuffer.acquireFenceFd = srcBuffer.acquireFenceFd;
  // releaseFenceFd
  dstBuffer.releaseFenceFd = srcBuffer.releaseFenceFd;
  // bufferSettings: todo
  // imgProcSettings: todo

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBuffer(const mcam::StreamBuffer& srcBuffer,
                         StreamBuffer& dstBuffer) -> int {
  int status = 0;
  int res = 0;
  // streamId
  dstBuffer.streamId = srcBuffer.streamId;
  // bufferId
  dstBuffer.bufferId = srcBuffer.bufferId;
  // buffer

  if (srcBuffer.buffer.get() == nullptr) {
    dstBuffer.buffer = nullptr;
    MY_LOGD("null srcBuffer");
  } else {
    if (IGraphicImageBufferHeap::castFrom(srcBuffer.buffer.get()) == nullptr) {
      dstBuffer.buffer = nullptr;
      MY_LOGD("null IGraphicImageBufferHeap");
    } else {
      dstBuffer.buffer =
          *(IGraphicImageBufferHeap::castFrom(srcBuffer.buffer.get())
                ->getBufferHandlePtr());
    }
  }
  // status
  dstBuffer.status = static_cast<BufferStatus>(srcBuffer.status);
  // acquireFenceFd
  // dstBuffer.acquireFenceFd = srcBuffer.buffer->getAcquireFence();
  // releaseFenceFd
  // dstBuffer.releaseFenceFd = srcBuffer.buffer->getReleaseFence();
  // appBufferHandleHolder: no needs to set

  if (srcBuffer.buffer.get() == nullptr) {
    dstBuffer.acquireFenceFd = -1;
    dstBuffer.releaseFenceFd = -1;
    MY_LOGD("set fence but null srcBuffer");
  } else {
    dstBuffer.acquireFenceFd = srcBuffer.buffer->getAcquireFence();
    // releaseFenceFd
    dstBuffer.releaseFenceFd = srcBuffer.buffer->getReleaseFence();
    // appBufferHandleHolder: todo: get from buffer
  }
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertCameraResourceCost(const mcam::CameraResourceCost& srcCost,
                               CameraResourceCost& dstCost) -> int {
  // resourceCost
  dstCost.resourceCost = srcCost.resourceCost;
  // conflictingDevices
  dstCost.conflictingDevices = srcCost.conflictingDevices;

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertExtConfigurationResults(
    const mcam::ExtConfigurationResults& srcResults,
    ExtConfigurationResults& dstResults) -> int {
  // streamResults
  dstResults.streamResults = srcResults.streamResults;

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertCaptureResults(
    const std::vector<mcam::CaptureResult>& srcResults,
    std::vector<CaptureResult>& dstResults,
    int32_t maxMetaCount) -> int {
  int status = 0;
  int res = 0;
  for (int i = 0; i < srcResults.size(); i++) {
    if (srcResults[i].halResult.count() > 0) {
      continue;
    }

    CaptureResult dstResult;
    res = convertCaptureResult(srcResults[i], dstResult);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
    // set max meta count
    if (srcResults[i].bLastPartialResult) {
      dstResult.partialResult = maxMetaCount;
    }
    //
    dstResults.push_back(std::move(dstResult));
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertCaptureResult(const mcam::CaptureResult& srcResult,
                          CaptureResult& dstResult) -> int {
  int status = 0;
  int res = 0;
  // frameNumber
  dstResult.frameNumber = srcResult.frameNumber;
  // result
  res = convertMetadata(srcResult.result, dstResult.result);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  printAutoTestLog(dstResult.result, true);
  // outputBuffers
  MY_LOGE("output buffer %d", srcResult.frameNumber);
  res = convertStreamBuffers(srcResult.outputBuffers, dstResult.outputBuffers);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }
  // inputBuffer
  if (srcResult.inputBuffers.size() != 0 &&
      srcResult.inputBuffers[0].streamId != -1) {
    MY_LOGE("input buffer %d %d", srcResult.frameNumber,
            srcResult.inputBuffers[0].streamId);
    res = convertStreamBuffer(srcResult.inputBuffers[0], dstResult.inputBuffer);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }
  // partialResult:  user should handle this:
  //     if bLastPartialResult then partialResult = maxcount
  // physicalCameraMetadata
  dstResult.physicalCameraMetadata.resize(
      srcResult.physicalCameraMetadata.size());
  for (int i = 0; i < srcResult.physicalCameraMetadata.size(); i++) {
    res = convertPhysicalCameraMetadata(srcResult.physicalCameraMetadata[i],
                                        dstResult.physicalCameraMetadata[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertPhysicalCameraMetadata(
    const mcam::PhysicalCameraMetadata& srcResult,
    PhysicalCameraMetadata& dstResult) -> int {
  int status = 0;
  int res = 0;
  // physicalCameraId
  dstResult.physicalCameraId = srcResult.physicalCameraId;
  // metadata
  res = convertMetadata(srcResult.metadata, dstResult.metadata);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertNotifyMsgs(const std::vector<mcam::NotifyMsg>& srcMsgs,
                       std::vector<NotifyMsg>& dstMsgs) -> int {
  int status = 0;
  int res = 0;
  dstMsgs.resize(srcMsgs.size());
  for (int i = 0; i < srcMsgs.size(); i++) {
    res = convertNotifyMsg(srcMsgs[i], dstMsgs[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertNotifyMsg(const mcam::NotifyMsg& srcMsg, NotifyMsg& dstMsg)
    -> int {
  // type
  dstMsg.type = static_cast<MsgType>(srcMsg.type);
  // msg
  switch (dstMsg.type) {
    case MsgType::ERROR: {
      convertErrorMsg(srcMsg.msg.error, dstMsg);
    }
    case MsgType::SHUTTER: {
      convertShutterMsg(srcMsg.msg.shutter, dstMsg);
      break;
    }
    default: {
      MY_LOGE("Unknown message type: %u", dstMsg.type);
      return -EINVAL;
    }
  }
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertShutterMsg(const mcam::ShutterMsg& srcMsg, NotifyMsg& dstMsg)
    -> void {
  dstMsg.type = MsgType::SHUTTER;
  // frameNumber
  dstMsg.msg.shutter.frameNumber = srcMsg.frameNumber;
  // timestamp
  dstMsg.msg.shutter.timestamp = srcMsg.timestamp;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertErrorMsg(const mcam::ErrorMsg& srcMsg, NotifyMsg& dstMsg)
    -> void {
  dstMsg.type = MsgType::ERROR;
  // frameNumber
  dstMsg.msg.error.frameNumber = srcMsg.frameNumber;
  // errorStreamId
  dstMsg.msg.error.errorStreamId = srcMsg.errorStreamId;
  // errorCode
  dstMsg.msg.error.errorCode = static_cast<ErrorCode>(srcMsg.errorCode);
}

/******************************************************************************
 *
 ******************************************************************************/
#if 0  // not implement HAL Buffer Management
auto convertBufferRequests(
    const std::vector<mcam::BufferRequest>& srcRequests,
    std::vector<BufferRequest>& dstRequests) -> int {
  int status = 0;
  int res = 0;
  dstRequests.resize(srcRequests.size());
  for (int i = 0; i < srcRequests.size(); i++) {
    res = convertBufferRequest(srcRequests[i], dstRequests[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertBufferRequest(const mcam::BufferRequest& srcRequest,
                          BufferRequest& dstRequest) -> int {
  // streamId
  dstRequest.streamId = srcRequest.streamId;
  // numBuffersRequested
  dstRequest.numBuffersRequested = srcRequest.numBuffersRequested;

  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBufferRets(
    const std::vector<mcam::StreamBufferRet>& srcRets,
    std::vector<StreamBufferRet>& dstRets) -> int {
  int status = 0;
  int res = 0;
  dstRets.resize(srcRets.size());
  for (int i = 0; i < srcRets.size(); i++) {
    res = convertStreamBufferRet(srcRets[i], dstRets[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBufferRet(const mcam::StreamBufferRet& srcBufferRet,
                            StreamBufferRet& dstBufferRet) -> int {
  int status = 0;
  int res = 0;
  // streamId
  dstBufferRet.streamId = srcBufferRet.streamId;
  // val
  res = convertStreamBuffersVal(srcBufferRet.val, dstBufferRet.val);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertStreamBuffersVal(const mcam::StreamBuffersVal& srcBuffersVal,
                             StreamBuffersVal& dstBuffersVal) -> int {
  int status = 0;
  int res = 0;
  // error
  dstBuffersVal.error =
      static_cast<StreamBufferRequestError>(srcBuffersVal.error);
  // buffers
  res = convertStreamBuffers(srcBuffersVal.buffers, dstBuffersVal.buffers);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }

  return status;
}
#endif

/******************************************************************************
 *
 ******************************************************************************/
auto convertCameraIdAndStreamCombinations(
    const std::vector<CameraIdAndStreamCombination>& srcConfigs,
    std::vector<mcam::CameraIdAndStreamCombination>& dstConfigs) -> int {
  int status = 0;
  int res = 0;
  dstConfigs.resize(srcConfigs.size());
  for (int i = 0; i < srcConfigs.size(); i++) {
    res = convertCameraIdAndStreamCombination(srcConfigs[i], dstConfigs[i]);
    if (CC_UNLIKELY(res != 0)) {
      MY_LOGE("convert failed");
      status = res;
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto convertCameraIdAndStreamCombination(
    const CameraIdAndStreamCombination& srcConfigs,
    mcam::CameraIdAndStreamCombination& dstConfigs) -> int {
  int status = 0;
  int res = 0;
  // cameraId
  dstConfigs.cameraId = srcConfigs.cameraId;
  // streamConfiguration
  res = convertStreamConfiguration(srcConfigs.streamConfiguration,
                                   dstConfigs.streamConfiguration);
  if (CC_UNLIKELY(res != 0)) {
    MY_LOGE("convert failed");
    status = res;
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto dumpMetadata(const IMetadata& srcMetadata, int frameNumber) -> int {
  sMetadataConverter->dumpAll(srcMetadata, frameNumber);
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto freeCameraMetadata(const std::vector<CaptureResult>& captureResults)
    -> void {
  for (auto& result : captureResults) {
    // free result
    if (result.result != nullptr) {
      sMetadataConverter->freeCameraMetadata(
          const_cast<camera_metadata*>(result.result));
    }
    // free physicalCameraMetadata
    for (auto& phyResult : result.physicalCameraMetadata) {
      sMetadataConverter->freeCameraMetadata(
          const_cast<camera_metadata*>(phyResult.metadata));
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto printAutoTestLog(const camera_metadata_t* pMetadata, bool bIsOutput)
    -> void {
  if (!CamAutoTestLogger::checkEnable() || pMetadata == nullptr)
    return;
  int result;
  camera_metadata_ro_entry_t entry;
  // AF Trigger
  result = find_camera_metadata_ro_entry(pMetadata, ANDROID_CONTROL_AF_TRIGGER,
                                         &entry);
  if (result != -ENOENT) {
    uint8_t afTrigger = entry.data.u8[0];
    if (afTrigger == ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN ||
        afTrigger == ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED ||
        afTrigger == ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
      CAT_LOGD("[CAT][AF] ANDROID_CONTROL_AF_TRIGGER:%u IOType:%d", afTrigger,
               bIsOutput);
    }
  }
  // Streaming vHDR
  result = find_camera_metadata_ro_entry(pMetadata, MTK_HDR_FEATURE_HDR_MODE,
                                         &entry);
  if (result != -ENOENT && !bIsOutput) {
    uint8_t vHDR = entry.data.u8[0];
    if (vHDR == MTK_HDR_FEATURE_HDR_MODE_OFF ||
        vHDR == MTK_HDR_FEATURE_HDR_MODE_VIDEO_ON ||
        vHDR == MTK_HDR_FEATURE_HDR_MODE_VIDEO_AUTO) {
      CAT_LOGD("[CAT][Event] vhdr:%s IOType:%d",
               vHDR == MTK_HDR_FEATURE_HDR_MODE_OFF
                   ? "Off"
                   : vHDR == MTK_HDR_FEATURE_HDR_MODE_VIDEO_ON ? "On" : "Auto",
               bIsOutput);
    }
  }
  // Capture HDR
  result = find_camera_metadata_ro_entry(pMetadata, ANDROID_CONTROL_SCENE_MODE,
                                         &entry);
  if (result != -ENOENT && !bIsOutput) {
    uint8_t captureHDR = entry.data.u8[0];
    CAT_LOGD("[CAT][Event] hdr:%s IOType:%d",
             captureHDR == ANDROID_CONTROL_SCENE_MODE_HDR ? "On" : "Off",
             bIsOutput);
  }
}

};  // namespace android
};  // namespace mcam
