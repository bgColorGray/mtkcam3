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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_ANDROID_DEVICE_3_X_TYPES_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_ANDROID_DEVICE_3_X_TYPES_H_

#include <cutils/native_handle.h>
#include <system/camera_metadata.h>
#include <system/graphics-base-v1.0.h>
#include <system/graphics-base-v1.1.h>
#include <system/graphics-base-v1.2.h>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace android {

static const int NO_STREAM = -1;

enum class StreamType : uint32_t {
  OUTPUT = 0,
  INPUT = 1,
};

enum StreamRotation : uint32_t {
  ROTATION_0 = 0,
  ROTATION_90 = 1,
  ROTATION_180 = 2,
  ROTATION_270 = 3,
};

enum class StreamConfigurationMode : uint32_t {
  NORMAL_MODE = 0,
  CONSTRAINED_HIGH_SPEED_MODE = 1,
  VENDOR_MODE_0 = 0x8000,
  VENDOR_MODE_1,
  VENDOR_MODE_2,
  VENDOR_MODE_3,
  VENDOR_MODE_4,
  VENDOR_MODE_5,
  VENDOR_MODE_6,
  VENDOR_MODE_7,
};

struct Stream {
  int32_t id = NO_STREAM;
  StreamType streamType = StreamType::OUTPUT;
  uint32_t width = 0;
  uint32_t height = 0;
  android_pixel_format_t format = HAL_PIXEL_FORMAT_RGBA_8888;
  uint64_t usage = 0;
  android_dataspace_t dataSpace = HAL_DATASPACE_UNKNOWN;
  StreamRotation rotation = StreamRotation::ROTATION_0;
  int32_t physicalCameraId = -1;  // V3_4
  uint32_t bufferSize = 0;       // V3_4
};

struct StreamConfiguration {
  std::vector<Stream> streams;
  StreamConfigurationMode operationMode;
  const camera_metadata* sessionParams;
  uint32_t streamConfigCounter = 0;  // V3_5
};

struct OfflineRequest {
  uint32_t frameNumber = 0;
  std::vector<int32_t> pendingStreams;
};

struct OfflineStream {
  int32_t id = NO_STREAM;
  uint32_t numOutstandingBuffers = 0;
  std::vector<uint64_t> circulatingBufferIds;
};

struct CameraOfflineSessionInfo {
  std::vector<OfflineStream> offlineStreams;
  std::vector<OfflineRequest> offlineRequests;
};

struct HalStream {
  int32_t id = NO_STREAM;
  android_pixel_format_t overrideFormat = HAL_PIXEL_FORMAT_RGBA_8888;
  uint64_t producerUsage = 0;
  uint64_t consumerUsage = 0;
  uint32_t maxBuffers = 0;
  android_dataspace_t overrideDataSpace = HAL_DATASPACE_UNKNOWN;  // V3_3
  int32_t physicalCameraId = -1;                                  // V3_4
  bool supportOffline = false;                                    // V3_6
};

struct HalStreamConfiguration {
  std::vector<HalStream> streams;
};

struct PhysicalCameraSetting {
  int32_t physicalCameraId = -1;
  const camera_metadata* settings;
};

enum class BufferStatus : uint32_t {
  OK = 0,
  ERROR = 1,
};

struct AppBufferHandleHolder {
  buffer_handle_t bufferHandle = nullptr;
  bool freeByOthers = false;
  explicit AppBufferHandleHolder(buffer_handle_t);
  ~AppBufferHandleHolder();
};

struct StreamBuffer {
  int32_t streamId = NO_STREAM;
  uint64_t bufferId = 0;  // SHOULD ONLY EXIST IN HIDL HAL
  buffer_handle_t buffer = nullptr;
  BufferStatus status = BufferStatus::OK;
  int acquireFenceFd = -1;
  int releaseFenceFd = -1;
  std::shared_ptr<AppBufferHandleHolder> appBufferHandleHolder = nullptr;
};

enum class CameraBlobId : uint16_t {
  JPEG = 0x00FF,
  JPEG_APP_SEGMENTS = 0x100,  // V3_5
};

struct CameraBlob {
  CameraBlobId blobId;
  uint32_t blobSize;
};

enum class MsgType : uint32_t {
  ERROR = 1,
  SHUTTER = 2,
};

enum class ErrorCode : uint32_t {
  ERROR_DEVICE = 1,
  ERROR_REQUEST = 2,
  ERROR_RESULT = 3,
  ERROR_BUFFER = 4,
};

struct ErrorMsg {
  uint32_t frameNumber = 0;
  int32_t errorStreamId = NO_STREAM;
  ErrorCode errorCode = ErrorCode::ERROR_DEVICE;
};

struct ShutterMsg {
  uint32_t frameNumber = 0;
  uint64_t timestamp = 0;
};

struct NotifyMsg {
  MsgType type = MsgType::ERROR;
  union Message {
    ErrorMsg error;
    ShutterMsg shutter;
  } msg = {.error = ErrorMsg()};
};

enum RequestTemplate : uint32_t {
  PREVIEW = 1u,
  STILL_CAPTURE = 2u,
  VIDEO_RECORD = 3u,
  VIDEO_SNAPSHOT = 4u,
  ZERO_SHUTTER_LAG = 5u,
  MANUAL = 6u,
  VENDOR_TEMPLATE_START = 0x40000000u,
};

struct CaptureRequest {
  uint32_t frameNumber = 0;
  const camera_metadata* settings;
  StreamBuffer inputBuffer;
  std::vector<StreamBuffer> outputBuffers;
  std::vector<PhysicalCameraSetting> physicalCameraSettings;  // V3_4
};

struct PhysicalCameraMetadata {
  int32_t physicalCameraId = -1;
  const camera_metadata* metadata;
};

struct CaptureResult {
  uint32_t frameNumber = 0;
  const camera_metadata* result;
  std::vector<StreamBuffer> outputBuffers;
  StreamBuffer inputBuffer;
  uint32_t partialResult = 0;
  std::vector<PhysicalCameraMetadata> physicalCameraMetadata;  // V3_4
};

struct BufferCache {  // SHOULD ONLY EXIST IN HIDL HAL
  int32_t streamId;
  uint64_t bufferId;
};

enum class StreamBufferRequestError : uint32_t {
  NO_ERROR = 0,
  NO_BUFFER_AVAILABLE = 1,
  MAX_BUFFER_EXCEEDED = 2,
  STREAM_DISCONNECTED = 3,
  UNKNOWN_ERROR = 4,
};

struct StreamBuffersVal {
  StreamBufferRequestError error = StreamBufferRequestError::UNKNOWN_ERROR;
  std::vector<StreamBuffer> buffers;
};

struct StreamBufferRet {
  int32_t streamId = NO_STREAM;
  StreamBuffersVal val;
};

enum BufferRequestStatus : uint32_t {
  STATUS_OK = 0,
  FAILED_PARTIAL = 1,
  FAILED_CONFIGURING = 2,
  FAILED_ILLEGAL_ARGUMENTS = 3,
  FAILED_UNKNOWN = 4,
};

struct BufferRequest {
  int32_t streamId = NO_STREAM;
  uint32_t numBuffersRequested = 0;
};

enum DeviceType : uint32_t {
  NORMAL = 0,
  POSTPROC = 1,
};

struct DeviceInfo {
  int32_t instanceId = -1;
  int32_t virtualInstanceId = -1;
  bool hasFlashUnit = false;
  bool isHidden = false;
  int32_t facing = -1;
  DeviceType type = DeviceType::NORMAL;
};

// ExtCameraDeviceSession
struct ExtConfigurationResults {
  // int status,
  // camera_metadata* results;
  std::map<int32_t, std::vector<int64_t>> streamResults;
};

static inline std::string toString(const Stream& o) {
  std::stringstream os;
  os << "streamId:" << o.id
     << " type:" << (o.streamType == StreamType::OUTPUT ? "OUT" : "IN")
     << " (w,h) = (" << o.width << "," << o.height << ")"
     << " format:" << static_cast<int>(o.format) << " usage:0x" << std::hex
     << o.usage << " dataSpace:" << static_cast<int>(o.dataSpace)
     << " rotation:" << static_cast<int>(o.rotation)
     << " physicalCameraId:" << o.physicalCameraId
     << " bufferSize:" << o.bufferSize;
  return os.str();
}

static inline std::string toString(const StreamConfiguration& o) {
  std::stringstream os;
  os << "streams:";
  for (auto& stream : o.streams) {
    os << "{" << toString(stream) << "}";
  }
  os << " operationMode:" << static_cast<int>(o.operationMode)
     << " sessionParams.size:" << get_camera_metadata_size(o.sessionParams)
     << " streamConfigCounter:" << static_cast<int>(o.streamConfigCounter);
  return os.str();
}

static inline std::string toString(const ErrorCode& o) {
  std::string os;
  switch (o) {
    case ErrorCode::ERROR_BUFFER:
      os = "ERROR_BUFFER";
      break;
    case ErrorCode::ERROR_DEVICE:
      os = "ERROR_DEVICE";
      break;
    case ErrorCode::ERROR_REQUEST:
      os = "ERROR_REQUEST";
      break;
    case ErrorCode::ERROR_RESULT:
      os = "ERROR_RESULT";
      break;
    default:
      os = "Unknown ErrorCode";
      break;
  }
  return os;
}

static inline std::string toString(const StreamBuffer& o) {
  std::stringstream os;
  os << "streamId:" << o.streamId << " bufferId:" << o.bufferId << " buffer:"
     << reinterpret_cast<void*>(const_cast<native_handle*>(o.buffer))
     << " status:" << (o.status == BufferStatus::OK ? "OK" : "ERROR")
     << " acquireFence:" << o.acquireFenceFd
     << " releaseFence:" << o.releaseFenceFd
     << " appBufferHandleHolder:"
     << reinterpret_cast<void*>(o.appBufferHandleHolder.get());
  return os.str();
}

static inline std::string toString(const CaptureRequest& o) {
  std::stringstream os;
  os << "frameNumber:" << o.frameNumber
     << " settings.size:" << get_camera_metadata_size(o.settings)
     << " inputBuffer:" << toString(o.inputBuffer);
  for (auto buffer : o.outputBuffers) {
    os << " / outputBuffer:" << toString(buffer);
  }
  for (auto physicalSetting : o.physicalCameraSettings) {
    os << " / physicalCameraId:" << physicalSetting.physicalCameraId
       << " physicalSettings.size:"
       << get_camera_metadata_size(physicalSetting.settings);
  }
  return os.str();
}

};  // namespace android
};  // namespace mcam

#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_ANDROID_DEVICE_3_X_TYPES_H_
