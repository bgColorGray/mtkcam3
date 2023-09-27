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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_TYPES_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_TYPES_H_

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "mtkcam/def/ImageBufferInfo.h"
#include "mtkcam/def/ImageFormat.h"
#include "mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h"
#include "mtkcam/utils/metadata/IMetadata.h"

typedef uint64_t BufferUsage;
using NSCam::IMetadata;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {

static const int NO_STREAM = -1;

enum class Dataspace : int32_t {
  // V1_0
  UNKNOWN = 0x0,
  ARBITRARY = 0x1,

  STANDARD_SHIFT = 16,
  STANDARD_MASK = 63 << STANDARD_SHIFT,  // 0x3F
  STANDARD_UNSPECIFIED = 0 << STANDARD_SHIFT,
  STANDARD_BT709 = 1 << STANDARD_SHIFT,
  STANDARD_BT601_625 = 2 << STANDARD_SHIFT,
  STANDARD_BT601_625_UNADJUSTED = 3 << STANDARD_SHIFT,
  STANDARD_BT601_525 = 4 << STANDARD_SHIFT,
  STANDARD_BT601_525_UNADJUSTED = 5 << STANDARD_SHIFT,
  STANDARD_BT2020 = 6 << STANDARD_SHIFT,
  STANDARD_BT2020_CONSTANT_LUMINANCE = 7 << STANDARD_SHIFT,
  STANDARD_BT470M = 8 << STANDARD_SHIFT,
  STANDARD_FILM = 9 << STANDARD_SHIFT,
  STANDARD_DCI_P3 = 10 << STANDARD_SHIFT,
  STANDARD_ADOBE_RGB = 11 << STANDARD_SHIFT,

  TRANSFER_SHIFT = 22,
  TRANSFER_MASK = 31 << TRANSFER_SHIFT,  // 0x1F
  TRANSFER_UNSPECIFIED = 0 << TRANSFER_SHIFT,
  TRANSFER_LINEAR = 1 << TRANSFER_SHIFT,
  TRANSFER_SRGB = 2 << TRANSFER_SHIFT,
  TRANSFER_SMPTE_170M = 3 << TRANSFER_SHIFT,
  TRANSFER_GAMMA2_2 = 4 << TRANSFER_SHIFT,
  TRANSFER_GAMMA2_6 = 5 << TRANSFER_SHIFT,
  TRANSFER_GAMMA2_8 = 6 << TRANSFER_SHIFT,
  TRANSFER_ST2084 = 7 << TRANSFER_SHIFT,
  TRANSFER_HLG = 8 << TRANSFER_SHIFT,

  RANGE_SHIFT = 27,
  RANGE_MASK = 7 << RANGE_SHIFT,  // 0x7
  RANGE_UNSPECIFIED = 0 << RANGE_SHIFT,
  RANGE_FULL = 1 << RANGE_SHIFT,
  RANGE_LIMITED = 2 << RANGE_SHIFT,
  RANGE_EXTENDED = 3 << RANGE_SHIFT,

  SRGB_LINEAR = 0x200,  // deprecated, use V0_SRGB_LINEAR
  V0_SRGB_LINEAR = STANDARD_BT709 | TRANSFER_LINEAR | RANGE_FULL,
  SRGB = 0x201,  // deprecated, use V0_SRGB
  V0_SRGB = STANDARD_BT709 | TRANSFER_SRGB | RANGE_FULL,
  V0_SCRGB = STANDARD_BT709 | TRANSFER_SRGB | RANGE_EXTENDED,
  JFIF = 0x101,  // deprecated, use V0_JFIF
  V0_JFIF = STANDARD_BT601_625 | TRANSFER_SMPTE_170M | RANGE_FULL,
  BT601_625 = 0x102,  // deprecated, use V0_BT601_625
  V0_BT601_625 = STANDARD_BT601_625 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
  BT601_525 = 0x103,  // deprecated, use V0_BT601_525
  V0_BT601_525 = STANDARD_BT601_525 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
  BT709 = 0x104,  // deprecated, use V0_BT709
  V0_BT709 = STANDARD_BT709 | TRANSFER_SMPTE_170M | RANGE_LIMITED,

  DCI_P3_LINEAR = STANDARD_DCI_P3 | TRANSFER_LINEAR | RANGE_FULL,
  DCI_P3 = STANDARD_DCI_P3 | TRANSFER_GAMMA2_6 | RANGE_FULL,
  DISPLAY_P3_LINEAR = STANDARD_DCI_P3 | TRANSFER_LINEAR | RANGE_FULL,
  DISPLAY_P3 = STANDARD_DCI_P3 | TRANSFER_SRGB | RANGE_FULL,
  ADOBE_RGB = STANDARD_ADOBE_RGB | TRANSFER_GAMMA2_2 | RANGE_FULL,
  BT2020_LINEAR = STANDARD_BT2020 | TRANSFER_LINEAR | RANGE_FULL,
  BT2020 = STANDARD_BT2020 | TRANSFER_SMPTE_170M | RANGE_FULL,
  BT2020_PQ = STANDARD_BT2020 | TRANSFER_ST2084 | RANGE_FULL,

  DEPTH = 0x1000,
  SENSOR = 0x1001,

  // V1_1
  DEPTH_16 = 0x30,
  DEPTH_24 = 0x31,
  DEPTH_24_STENCIL_8 = 0x32,
  DEPTH_32F = 0x33,
  DEPTH_32F_STENCIL_8 = 0x34,
  STENCIL_8 = 0x35,
  YCBCR_P010 = 0x36,

  // V1_2
  DISPLAY_BT2020 = STANDARD_BT2020 | TRANSFER_SRGB | RANGE_FULL,
  DYNAMIC_DEPTH = 0x1002,
  JPEG_APP_SEGMENTS = 0x1003,
  HEIF = 0x1004,
};

enum class StreamType : uint32_t {
  OUTPUT = 0,
  INPUT = 1,
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
  NSCam::EImageFormat format = NSCam::eImgFmt_UNKNOWN;
  BufferUsage usage = 0;
  Dataspace dataSpace = Dataspace::UNKNOWN;
  uint32_t transform = NSCam::eTransform_None;
  int32_t physicalCameraId = -1;  // V3_4
  uint32_t bufferSize = 0;   // V3_4
  // MTK extension
  IMetadata settings;
  NSCam::BufPlanes bufPlanes;
};

struct StreamConfiguration {
  std::vector<Stream> streams;
  StreamConfigurationMode operationMode;
  IMetadata sessionParams;
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
  NSCam::EImageFormat overrideFormat = NSCam::eImgFmt_UNKNOWN;
  BufferUsage producerUsage = 0;
  BufferUsage consumerUsage = 0;
  uint32_t maxBuffers = 1;
  Dataspace overrideDataSpace = Dataspace::UNKNOWN;  // V3_3
  int32_t physicalCameraId = -1;                     // V3_4
  bool supportOffline = false;                       // V3_6
  // MTK extension
  IMetadata results;
};

struct HalStreamConfiguration  {
  std::vector<HalStream> streams;
  // MTK extension
  IMetadata sessionResults;
};

/*
struct PhysicalCameraSetting {
  std::string physicalCameraId;
  const camera_metadata* settings;
};
*/

enum class BufferStatus : uint32_t {
  OK = 0,
  ERROR = 1,
};

// use FD as fence type
// enum class FenceType : uint32_t {
//   FD = 0,
//   HDL = 1,
// };

// struct FenceOp {
//   FenceType type = FenceType::HDL;
//   int fd = -1;
//   const native_handle_t* hdl = nullptr;
// };

struct StreamBuffer {
  int32_t streamId = NO_STREAM;
  uint64_t bufferId = 0;
  std::shared_ptr<IImageBufferHeap> buffer = nullptr;
  BufferStatus status = BufferStatus::OK;
  int acquireFenceFd = -1;
  int releaseFenceFd = -1;
  // MTK extension
  IMetadata bufferSettings;  // control/result metadata
};

// should be handled in android adaptor
enum class CameraBlobId : uint16_t {
  JPEG = 0x00FF,
  JPEG_APP_SEGMENTS = 0x100,  // V3_5
};

struct CameraBlob {
  CameraBlobId blobId;
  uint32_t blobSize;
};

enum class MsgType : uint32_t {
  UNKNOWN = 0,
  ERROR = 1,
  SHUTTER = 2,
};

enum class ErrorCode : uint32_t {
  ERROR_NONE = 0,
  ERROR_DEVICE = 1,
  ERROR_REQUEST = 2,
  ERROR_RESULT = 3,
  ERROR_BUFFER = 4,
};

struct ErrorMsg {
  uint32_t frameNumber = 0;
  int64_t errorStreamId = NO_STREAM;
  ErrorCode errorCode = ErrorCode::ERROR_DEVICE;
};

struct ShutterMsg {
  uint32_t frameNumber = 0;
  uint64_t timestamp = 0;
};

struct NotifyMsg {
  MsgType type = MsgType::UNKNOWN;  //  MsgType::UNKNOWN -> assert
  union Message {
    ErrorMsg error;
    ShutterMsg shutter;
  } msg;
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

struct PhysicalCameraSetting {
  int32_t physicalCameraId = -1;
  IMetadata settings;
  IMetadata halSettings;
};

struct SubRequest {
  uint32_t subFrameIndex = 1;  // start from 1. reserve zero for main frame.
  std::vector<StreamBuffer> inputBuffers;  // logical or physical sensor inputs
  // std::vector<StreamBuffer> outputBuffers;  // sub-request have no output
  // bufs
  IMetadata settings;     // logical app ctrl
  IMetadata halSettings;  // logical hal ctrl
  std::vector<PhysicalCameraSetting>
      physicalCameraSettings;  // physical app & hal ctrl
};

struct CaptureRequest {
  // main-request for multiframe (including logical and physical devices)
  uint32_t frameNumber = 0;
  std::vector<StreamBuffer> inputBuffers;  // logical or physical sensor inputs
  std::vector<StreamBuffer>
      outputBuffers;      // logical or physical sensor outputs
  IMetadata settings;     // logical app ctrl
  IMetadata halSettings;  // logical hal ctrl
  std::vector<PhysicalCameraSetting>
      physicalCameraSettings;  // physical app & hal ctrl
  // sub-requests for multiframe
  std::vector<SubRequest> subRequests;
};

struct PhysicalCameraMetadata {
  int32_t physicalCameraId = -1;
  IMetadata metadata;
  IMetadata halMetadata;
};

struct CaptureResult {
  uint32_t frameNumber = 0;
  std::vector<StreamBuffer> outputBuffers;
  std::vector<StreamBuffer> inputBuffers;  // including multi-frame inputs
  bool bLastPartialResult;
  IMetadata result;
  IMetadata halResult;
  std::vector<PhysicalCameraMetadata> physicalCameraMetadata;
};

// struct BufferCache {  // SHOULD ONLY EXIST IN HIDL HAL
//   int32_t streamId;
//   uint64_t bufferId;
// };

#if 0  // not implement HAL Buffer Management
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
#endif

// struct DeviceInfo {
//   int32_t instanceId = -1;
//   int32_t virtualInstanceId = -1;
//   bool hasFlashUnit = false; // static: android.flash.info.available
//   bool isHidden = false;
//   int32_t facing = -1; // static: android.lens.facing
// };

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
     << " transform:" << o.transform
     << " physicalCameraId:" << o.physicalCameraId
     << " bufferSize:" << o.bufferSize;
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
  os << "streamId:" << o.streamId << " bufferId:" << o.bufferId
     << " buffer:" << reinterpret_cast<void*>(o.buffer.get())
     << " status:" << (o.status == BufferStatus::OK ? "OK" : "ERROR")
     << " acquireFenceFd:" << o.acquireFenceFd
     << " releaseFenceFd:" << o.releaseFenceFd;
  return os.str();
}

static inline std::string queryImageFormatName(int format) {
#define _ENUM_TO_NAME_(_prefix_, _format_) \
  case _prefix_##_format_: {               \
    static std::string name = #_format_;   \
    return name;                           \
  } break

  switch (format) {
    _ENUM_TO_NAME_(NSCam::eImgFmt_, IMPLEMENTATION_DEFINED);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, RAW16);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, RAW_OPAQUE);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, BLOB);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, YCBCR_420_888);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, RGBA8888);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, RGBX8888);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, RGB888);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, RGB565);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, BGRA8888);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, YUY2);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, NV16);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, NV21);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, NV12);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, YV12);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, Y8);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, Y16);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, CAMERA_OPAQUE);
    _ENUM_TO_NAME_(NSCam::eImgFmt_, YUV_P010);
    default: { return "UNKNOWN"; } break;
#undef _ENUM_TO_NAME_
  }

  return "";
}

static inline std::string queryBufferUsageName(int usage) {
#define _USAGE_TO_NAME_(_prefix_, _usage_) \
  case _prefix_##_usage_: {                \
    str += "|" #_usage_;                   \
  } break

  std::string str = "0";
  //
  switch ((usage & eBUFFER_USAGE_SW_READ_MASK)) {
    _USAGE_TO_NAME_(eBUFFER_USAGE_, SW_READ_RARELY);
    _USAGE_TO_NAME_(eBUFFER_USAGE_, SW_READ_OFTEN);
    default:
      break;
  }
  //
  switch ((usage & eBUFFER_USAGE_SW_WRITE_MASK)) {
    _USAGE_TO_NAME_(eBUFFER_USAGE_, SW_WRITE_RARELY);
    _USAGE_TO_NAME_(eBUFFER_USAGE_, SW_WRITE_OFTEN);
    default:
      break;
  }

  switch ((usage & eBUFFER_USAGE_HW_CAMERA_READWRITE)) {
    _USAGE_TO_NAME_(eBUFFER_USAGE_, HW_CAMERA_WRITE);
    _USAGE_TO_NAME_(eBUFFER_USAGE_, HW_CAMERA_READ);
    default:
      break;
  }
#undef _USAGE_TO_NAME_

  if ((usage & eBUFFER_USAGE_HW_MASK) != 0) {
#define _USAGE_TO_NAME_OR_(_prefix_, _usage_) \
  if ((usage & _prefix_##_usage_)) {          \
    str += "|" #_usage_;                      \
  }
    _USAGE_TO_NAME_OR_(eBUFFER_USAGE_, HW_TEXTURE)
    _USAGE_TO_NAME_OR_(eBUFFER_USAGE_, HW_RENDER)
    _USAGE_TO_NAME_OR_(eBUFFER_USAGE_, HW_COMPOSER)
    _USAGE_TO_NAME_OR_(eBUFFER_USAGE_, HW_VIDEO_ENCODER)
#undef _USAGE_TO_NAME_OR_
  }

#define _BIT_TO_NAME_(_bit_, _name_) \
  if ((usage & _bit_)) {          \
    str += "|" _name_;                      \
  }
  _BIT_TO_NAME_((1ULL << 28), "PRIV_0")
  _BIT_TO_NAME_((1ULL << 29), "PRIV_1(SMVR)")
  _BIT_TO_NAME_((1ULL << 30), "PRIV_2")
  _BIT_TO_NAME_((1ULL << 31), "PRIV_3")
#undef _BIT_TO_NAME_

  return str;
}

static inline std::string queryDataspaceName(int32_t dataspace) {
  std::string str;

  switch (dataspace) {
#define _DS_TO_NAME_(_ds_)         \
  case (int32_t)Dataspace::_ds_: { \
    str = #_ds_;                   \
  } break;

    // Legacy dataspaces
    _DS_TO_NAME_(SRGB_LINEAR);
    _DS_TO_NAME_(V0_SRGB_LINEAR);
    // _DS_TO_NAME_(V0_SCRGB_LINEAR);
    _DS_TO_NAME_(SRGB);
    _DS_TO_NAME_(V0_SRGB);
    _DS_TO_NAME_(V0_SCRGB);

    // YCbCr Colorspaces
    _DS_TO_NAME_(JFIF);
    _DS_TO_NAME_(V0_JFIF);
    _DS_TO_NAME_(HEIF);
    _DS_TO_NAME_(JPEG_APP_SEGMENTS);
    _DS_TO_NAME_(BT601_625);
    _DS_TO_NAME_(V0_BT601_625);
    _DS_TO_NAME_(BT601_525);
    _DS_TO_NAME_(V0_BT601_525);
    _DS_TO_NAME_(BT709);
    _DS_TO_NAME_(V0_BT709);
    _DS_TO_NAME_(DCI_P3_LINEAR);
    _DS_TO_NAME_(DCI_P3);
    // _DS_TO_NAME_(DISPLAY_P3_LINEAR);  // the same as DCI_P3_LINEAR
    _DS_TO_NAME_(DISPLAY_P3);
    _DS_TO_NAME_(ADOBE_RGB);
    _DS_TO_NAME_(BT2020_LINEAR);
    _DS_TO_NAME_(BT2020);
    _DS_TO_NAME_(BT2020_PQ);

    // Data spaces for non-color formats
    _DS_TO_NAME_(DEPTH);
    _DS_TO_NAME_(SENSOR);

    _DS_TO_NAME_(ARBITRARY);
    _DS_TO_NAME_(UNKNOWN);
    default: { str = std::to_string(dataspace); } break;

#undef _DS_TO_NAME_
  }

#define _DATASPACE_TO_NAME_(_ds_)  \
  case (int32_t)Dataspace::_ds_: { \
    str += "|" #_ds_;              \
  } break;
  //
  // STANDARD_MASK
  switch (dataspace & static_cast<int32_t>(Dataspace::STANDARD_MASK)) {
    _DATASPACE_TO_NAME_(STANDARD_BT709);
    _DATASPACE_TO_NAME_(STANDARD_BT601_625);
    _DATASPACE_TO_NAME_(STANDARD_BT601_625_UNADJUSTED);
    _DATASPACE_TO_NAME_(STANDARD_BT601_525);
    _DATASPACE_TO_NAME_(STANDARD_BT601_525_UNADJUSTED);
    _DATASPACE_TO_NAME_(STANDARD_BT2020);
    _DATASPACE_TO_NAME_(STANDARD_BT2020_CONSTANT_LUMINANCE);
    _DATASPACE_TO_NAME_(STANDARD_BT470M);
    _DATASPACE_TO_NAME_(STANDARD_FILM);
    _DATASPACE_TO_NAME_(STANDARD_DCI_P3);
    _DATASPACE_TO_NAME_(STANDARD_ADOBE_RGB);
    default:
      break;
  }
  //
  // TRANSFER_MASK
  switch (dataspace & static_cast<int32_t>(Dataspace::TRANSFER_MASK)) {
    _DATASPACE_TO_NAME_(TRANSFER_LINEAR);
    _DATASPACE_TO_NAME_(TRANSFER_SRGB);
    _DATASPACE_TO_NAME_(TRANSFER_SMPTE_170M);
    _DATASPACE_TO_NAME_(TRANSFER_GAMMA2_2);
    _DATASPACE_TO_NAME_(TRANSFER_GAMMA2_6);
    _DATASPACE_TO_NAME_(TRANSFER_GAMMA2_8);
    _DATASPACE_TO_NAME_(TRANSFER_ST2084);
    _DATASPACE_TO_NAME_(TRANSFER_HLG);
    default:
      break;
  }
  //
  // RANGE_MASK
  switch (dataspace & static_cast<int32_t>(Dataspace::RANGE_MASK)) {
    _DATASPACE_TO_NAME_(RANGE_FULL);
    _DATASPACE_TO_NAME_(RANGE_LIMITED);
    _DATASPACE_TO_NAME_(RANGE_EXTENDED);
    default:
      break;
  }
#undef _DATASPACE_TO_NAME_

  return str;
}

};      // namespace mcam
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_TYPES_H_
