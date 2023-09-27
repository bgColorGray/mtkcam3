/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_DEVICE_INCLUDE_MTKCAMHAL_TYPES_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_DEVICE_INCLUDE_MTKCAMHAL_TYPES_H_

#include <cutils/native_handle.h>
#include <system/graphics-base-v1.0.h>
#include <system/graphics-base-v1.1.h>
#include <system/graphics-base-v1.2.h>

// #include <mtkcam/utils/metadata/IMetadata.h>
#include <system/camera_metadata.h>

#include <vector>
#include <functional>
#include <string>
#include <sstream>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {

struct  AppBufferHandleHolder
{
    buffer_handle_t bufferHandle = nullptr;
    bool freeByOthers = false;
    //
    AppBufferHandleHolder(buffer_handle_t);
    ~AppBufferHandleHolder();
};

static const int NO_STREAM = -1;
// directly reference HIDL I/F

// Base ::android::hardware::camera::device::V3_2::StreamType;
enum class StreamType : uint32_t {
    OUTPUT = 0,
    INPUT = 1,
};

// Base ::android::hardware::camera::device::V3_2::StreamRotation;
enum StreamRotation : uint32_t  {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3,
};

// Base ::android::hardware::camera::device::V3_2::StreamConfigurationMode;
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

// Base ::android::hardware::camera::device::V3_2::Stream;
struct Stream {
    int32_t id = NO_STREAM;
    StreamType streamType = StreamType::OUTPUT;
    uint32_t width = 0;
    uint32_t height = 0;
    android_pixel_format_t format = HAL_PIXEL_FORMAT_RGBA_8888;
    uint64_t usage = 0;
    android_dataspace_t dataSpace = HAL_DATASPACE_UNKNOWN;
    StreamRotation rotation = StreamRotation::ROTATION_0;
    std::string physicalCameraId;                               //V3_4
    uint32_t bufferSize = 0;                                    //V3_4
};

// Base ::android::hardware::camera::device::V3_2::StreamConfiguration;
struct StreamConfiguration {
    std::vector<Stream> streams;
    StreamConfigurationMode operationMode;
    const camera_metadata *sessionParams;
    uint32_t streamConfigCounter = 0;                           //V3_5
};

// Base ::android::hardware::camera::device::V3_6::OfflineRequest;
struct OfflineRequest {
    uint32_t frameNumber = 0;
    std::vector<int32_t> pendingStreams;
};

// Base ::android::hardware::camera::device::V3_6::OfflineStream;
struct OfflineStream {
    int32_t id = NO_STREAM;
    uint32_t numOutstandingBuffers = 0;
    std::vector<uint64_t> circulatingBufferIds;
};

// Base ::android::hardware::camera::device::V3_6::CameraOfflineSessionInfo;
struct CameraOfflineSessionInfo {
    std::vector<OfflineStream> offlineStreams;
    std::vector<OfflineRequest> offlineRequests;
};

// Base ::android::hardware::camera::device::V3_2::HalStream;
struct HalStream {
    int32_t id = NO_STREAM;
    android_pixel_format_t overrideFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    uint64_t producerUsage = 0;
    uint64_t consumerUsage = 0;
    uint32_t maxBuffers = 0;
    android_dataspace_t overrideDataSpace = HAL_DATASPACE_UNKNOWN;  //V3_3
    std::string physicalCameraId;                                   //V3_4
    bool supportOffline = false;                                    //V3_6
};

// Base ::android::hardware::camera::device::V3_2::HalStreamConfiguration;
struct HalStreamConfiguration {
    std::vector<HalStream> streams;
};

// Base ::android::hardware::camera::device::V3_4::PhysicalCameraSetting;
struct PhysicalCameraSetting {
    std::string physicalCameraId;
    const camera_metadata *settings;
};

// Base ::android::hardware::camera::device::V3_2::BufferStatus;
enum class BufferStatus : uint32_t {
    OK = 0,
    ERROR = 1,
};

enum class FenceType : uint32_t {
    FD = 0,
    HDL = 1,
};

struct FenceOp {
    FenceType type = FenceType::HDL;
    int fd = -1;
    const native_handle_t* hdl = nullptr;
};

// Base ::android::hardware::camera::device::V3_2::StreamBuffer;
struct StreamBuffer {
    int32_t streamId = NO_STREAM;
    uint64_t bufferId = 0;                   // SHOULD ONLY EXIST IN HIDL HAL
    buffer_handle_t buffer = nullptr;
    BufferStatus status = BufferStatus::OK;
    FenceOp acquireFenceOp{FenceType::HDL, -1, nullptr};
    FenceOp releaseFenceOp{FenceType::HDL, -1, nullptr};
    std::shared_ptr<AppBufferHandleHolder> appBufferHandleHolder = nullptr;
    bool freeByOthers = false;
};

// Base ::android::hardware::camera::device::V3_2::CameraBlobId;
enum class CameraBlobId : uint16_t {
    JPEG = 0x00FF,
    JPEG_APP_SEGMENTS = 0x100,                                  //V3_5
};

// Base ::android::hardware::camera::device::V3_2::CameraBlob;
struct CameraBlob {
    CameraBlobId blobId;
    uint32_t blobSize;
};

// Base ::android::hardware::camera::device::V3_2::MsgType;
enum class MsgType : uint32_t {
    ERROR = 1,
    SHUTTER = 2,
};

// Base ::android::hardware::camera::device::V3_2::ErrorCode;
enum class ErrorCode : uint32_t {
    ERROR_DEVICE = 1,
    ERROR_REQUEST = 2,
    ERROR_RESULT = 3,
    ERROR_BUFFER = 4,
};

// Base ::android::hardware::camera::device::V3_2::ErrorMsg;
struct ErrorMsg {
    uint32_t frameNumber = 0;
    int32_t errorStreamId = NO_STREAM;
    ErrorCode errorCode = ErrorCode::ERROR_DEVICE;
};

// Base ::android::hardware::camera::device::V3_2::ShutterMsg;
struct ShutterMsg {
    uint32_t frameNumber = 0;
    uint64_t timestamp = 0;
};

// Base ::android::hardware::camera::device::V3_2::NotifyMsg;
struct NotifyMsg {
    MsgType type = MsgType::ERROR;
    union Message {
        ErrorMsg error;
        ShutterMsg shutter;
    } msg;
};

// Base ::android::hardware::camera::device::V3_2::RequestTemplate;
enum RequestTemplate : uint32_t {
    PREVIEW = 1u,
    STILL_CAPTURE = 2u,
    VIDEO_RECORD = 3u,
    VIDEO_SNAPSHOT = 4u,
    ZERO_SHUTTER_LAG = 5u,
    MANUAL = 6u,
    VENDOR_TEMPLATE_START = 0x40000000u,
};

// Base ::android::hardware::camera::device::V3_2::CaptureRequest;
struct CaptureRequest {
    uint32_t frameNumber = 0;
    const camera_metadata *settings;
    StreamBuffer inputBuffer;
    std::vector<StreamBuffer> outputBuffers;
    std::vector<PhysicalCameraSetting> physicalCameraSettings;      //V3_4
};

// Base ::android::hardware::camera::device::V3_4::PhysicalCameraMetadata;
struct PhysicalCameraMetadata {
    size_t resultSize = 0;
    std::string physicalCameraId;
    const camera_metadata *metadata;
};

// Base ::android::hardware::camera::device::V3_2::CaptureResult;
struct CaptureResult {
    uint32_t frameNumber = 0;
    size_t resultSize = 0;
    const camera_metadata *result;
    std::vector<StreamBuffer> outputBuffers;
    StreamBuffer inputBuffer;
    uint32_t partialResult = 0;
    std::vector<PhysicalCameraMetadata> physicalCameraMetadata;     //V3_4
};

// Base ::android::hardware::camera::device::V3_2::BufferCache;
struct BufferCache {    // SHOULD ONLY EXIST IN HIDL HAL
    int32_t streamId;
    uint64_t bufferId;
};

// Base ::android::hardware::camera::device::V3_5::StreamBufferRequestError;
enum class StreamBufferRequestError : uint32_t {
    NO_ERROR            = 0,
    NO_BUFFER_AVAILABLE = 1,
    MAX_BUFFER_EXCEEDED = 2,
    STREAM_DISCONNECTED = 3,
    UNKNOWN_ERROR = 4,
};

// Base ::android::hardware::camera::device::V3_5::StreamBuffersVal;
struct StreamBuffersVal {
    StreamBufferRequestError error = StreamBufferRequestError::UNKNOWN_ERROR;
    std::vector<StreamBuffer> buffers;
};

// Base ::android::hardware::camera::device::V3_5::StreamBufferRet;
struct StreamBufferRet {
    int32_t streamId = NO_STREAM;
    StreamBuffersVal val;
};

// Base ::android::hardware::camera::device::V3_5::BufferRequestStatus;
enum BufferRequestStatus : uint32_t {
    STATUS_OK = 0,
    FAILED_PARTIAL = 1,
    FAILED_CONFIGURING = 2,
    FAILED_ILLEGAL_ARGUMENTS = 3,
    FAILED_UNKNOWN = 4,
};

// Base ::android::hardware::camera::device::V3_5::BufferRequest;
struct BufferRequest {
    int32_t streamId = NO_STREAM;
    uint32_t numBuffersRequested = 0;
};

using ProcessCaptureResultFunc =
    std::function<void(std::vector<CaptureResult>& /*results*/)>;

using NotifyFunc =
    std::function<void(const std::vector<NotifyMsg>& /*msgs*/)>;


using RequestStreamBuffersFunc =
    std::function<BufferRequestStatus(
        const std::vector<BufferRequest>& /*buffer_requests*/,
        std::vector<StreamBufferRet>* /*buffer_returns*/)>;

// Callback function invoked to return stream buffers.
using ReturnStreamBuffersFunc =
    std::function<void(const std::vector<StreamBuffer>& /*buffers*/)>;

struct CameraDevice3SessionCallback {
  ProcessCaptureResultFunc processCaptureResult;
  NotifyFunc notify;
  RequestStreamBuffersFunc requestStreamBuffers;
  ReturnStreamBuffersFunc returnStreamBuffers;
};

static inline std::string toString(const Stream& o) {
    std::stringstream os;
    os  << "streamId:" << o.id
        << " type:"<< (o.streamType == StreamType::OUTPUT ? "OUT" : "IN")
        << " (w,h) = (" << o.width << "," << o.height << ")"
        << " format:" << (int)o.format
        << " usage:0x" << std::hex << o.usage
        << " dataSpace:" << (int)o.dataSpace
        << " rotation:" << (int)o.rotation
        << " physicalCameraId:" << o.physicalCameraId
        << " bufferSize:" << o.bufferSize;
    return os.str();
}

static inline std::string toString(const ErrorCode& o)      {
    std::string os;
    switch (o)
    {
    case ErrorCode::ERROR_BUFFER :
        os = "ERROR_BUFFER";
        break;
    case ErrorCode::ERROR_DEVICE :
        os = "ERROR_DEVICE";
        break;
    case ErrorCode::ERROR_REQUEST :
        os = "ERROR_REQUEST";
        break;
    case ErrorCode::ERROR_RESULT :
        os = "ERROR_RESULT";
        break;
    default:
        os = "Unknown ErrorCode";
        break;
    }
    return os;
}

static inline std::string toString(const StreamBuffer& o)   {
    std::stringstream os;
    os << "streamId:" << o.streamId << " bufferId:" << o.bufferId
       << " buffer:" << (void*)o.buffer
       << " status:" << (o.status == BufferStatus::OK ? "OK" : "ERROR")
       << " acquireFenceOp.hdl:"
       << reinterpret_cast<void*>(
              const_cast<native_handle*>(o.acquireFenceOp.hdl))
       << " releaseFenceOp.hdl:"
       << reinterpret_cast<void*>(
              const_cast<native_handle*>(o.releaseFenceOp.hdl))
       << " acquireFenceOp.fd:" << o.acquireFenceOp.fd
       << " releaseFenceOp.fd:" << o.releaseFenceOp.fd
       << " appBufferHandleHolder:" << (void*)o.appBufferHandleHolder.get()
       << " freeByOthers:" << o.freeByOthers;
    return os.str();
}


/******************************************************************************
 * Provider
 ******************************************************************************/
enum class TorchMode : uint32_t {
    OFF = 0, // Turn off the flash
    ON  = 1  // Turn on the flash to torch mode
};

enum class CameraMetadataType : uint32_t {
    BYTE = 0,       // Unsigned 8-bit integer (uint8_t)
    INT32 = 1,      // Signed 32-bit integer (int32_t)
    FLOAT = 2,      // 32-bit float (float)
    INT64 = 3,      // Signed 64-bit integer (int64_t)
    DOUBLE = 4,     // 64-bit float (double)
    RATIONAL = 5    // A 64-bit fraction (camera_metadata_rational_t)
};

struct VendorTag {
    uint32_t tagId;         // Tag identifier, must be >= TagBoundaryId::VENDOR
    std::string tagName;    // Name of tag, not including section name
    CameraMetadataType tagType;
};

struct VendorTagSection {
    std::string sectionName;        // Section name; must be namespaced within vendor's name
    std::vector<VendorTag> tags;    // List of tags in this section
};

enum class TagBoundaryId : uint32_t {
    AOSP    = 0x0,         // First valid tag id for android-defined tags
    VENDOR  = 0x80000000u  // First valid tag id for vendor extension tags
};

enum class CameraDeviceStatus : uint32_t {
    NOT_PRESENT = 0,        //not currently connected
    PRESENT = 1,            //connected
    ENUMERATING = 2,        //undergoing enumeration
};

enum class TorchModeStatus : uint32_t {
    NOT_AVAILABLE = 0,      // the torch mode can not be turned on
    AVAILABLE_OFF = 1,      // A torch mode has become off and is available to be turned on
    AVAILABLE_ON = 2,       // A torch mode has become on and is available to be turned off
};


struct CameraResourceCost {
    uint32_t resourceCost = 0;
    std::vector<std::string> conflictingDevices;
    std::vector<std::string> conflictingDeviceIds;      // for legacy
};

// provider@2.5
enum class DeviceState : uint64_t {
    NORMAL = 0,
    BACK_COVERED = 1 << 0,
    FRONT_COVERED = 1 << 1,
    FOLDED = 1 << 2,
};

// provider@2.6
struct CameraIdAndStreamCombination {
    std::string cameraId;
    StreamConfiguration streamConfiguration;    // device@3.4
};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_DEVICE_INCLUDE_MTKCAMHAL_TYPES_H_

