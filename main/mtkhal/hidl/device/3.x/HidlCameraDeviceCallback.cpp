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

#include <main/mtkhal/hidl/device/3.x/HidlCameraDeviceCallback.h>
#include "MyUtils.h"

#include <TypesWrapper.h>
#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef MTKCAM_HAL3PLUS_CUSTOMIZATION_SUPPORT
// Size of result metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE = 0;
#else
// Size of result metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE = 16 << 20 /* 16MB */;
#endif

using ::android::BAD_VALUE;
using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;
using mcam::android::IACameraDeviceCallback;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;

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

namespace mcam {
namespace hidl {
/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDeviceCallback::~HidlCameraDeviceCallback() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDeviceCallback::HidlCameraDeviceCallback(const CreationInfo& info)
    : IACameraDeviceCallback(),
      mInstanceId(info.instanceId),
      mLogPrefix(std::to_string(mInstanceId) + "-hidl-callback"),
      mCameraDeviceCallback(info.pCallback),
      mResultMetadataQueue() {
  MY_LOGI("ctor");

  if (CC_UNLIKELY(mCameraDeviceCallback == nullptr)) {
    MY_LOGE("mCameraDeviceCallback does not exist");
  }
  mResultMetadataQueue = std::make_unique<ResultMetadataQueue>(
      CAMERA_RESULT_METADATA_QUEUE_SIZE, false /* non blocking */);
  if (!mResultMetadataQueue || !mResultMetadataQueue->isValid()) {
    MY_LOGE("invalid result fmq");
    // warning : IHal interface
    // return BAD_VALUE;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::create(const CreationInfo& info)
    -> HidlCameraDeviceCallback* {
  auto pInstance = new HidlCameraDeviceCallback(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::processCaptureResult(
    const std::vector<mcam::android::CaptureResult>& results) -> void {
  CAM_TRACE_CALL();
  std::vector<::android::hardware::camera::device::V3_4::CaptureResult>
      hidl_results;

  status_t res = convertToHidlCaptureResults(results, &hidl_results);
  if (res != OK) {
    MY_LOGE("Converting hal capture result failed: %s(%d)", strerror(-res),
            res);
  }

#ifndef MTKCAM_HAL3PLUS_CUSTOMIZATION_SUPPORT
  if (auto pResultMetadataQueue = mResultMetadataQueue.get()) {
    MY_LOGD("if HAL3+ enable, it should not callback meta by FMQ");
    // if ( 0 ) {
    //   auto pResultMetadataQueue = mResultMetadataQueue.get();
    // // if (auto pResultMetadataQueue = mResultMetadataQueue.get() ) {
    //   MY_LOGD("if HAL+ enable, it should not callback meta by FMQ");
    for (size_t i = 0; i < hidl_results.size(); i++) {
      auto& hidl_result = hidl_results[i];
      // write logical meta into FMQ
      if (hidl_result.v3_2.result.size() > 0) {
        if (CC_LIKELY(
                pResultMetadataQueue->write(hidl_result.v3_2.result.data(),
                                            hidl_result.v3_2.result.size()))) {
          hidl_result.v3_2.fmqResultSize = hidl_result.v3_2.result.size();
          hidl_result.v3_2.result = hidl_vec<uint8_t>();  // resize(0)
        } else {
          hidl_result.v3_2.fmqResultSize = 0;
          MY_LOGW(
              "fail to write meta to mResultMetadataQueue, data=%p, size=%zu",
              hidl_result.v3_2.result.data(), hidl_result.v3_2.result.size());
        }
      }

      // write physical meta into FMQ
      if (hidl_result.physicalCameraMetadata.size() > 0) {
        for (size_t i = 0; i < hidl_result.physicalCameraMetadata.size(); i++) {
          if (CC_LIKELY(pResultMetadataQueue->write(
                  hidl_result.physicalCameraMetadata[i].metadata.data(),
                  hidl_result.physicalCameraMetadata[i].metadata.size()))) {
            hidl_result.physicalCameraMetadata[i].fmqMetadataSize =
                hidl_result.physicalCameraMetadata[i].metadata.size();
            hidl_result.physicalCameraMetadata[i].metadata =
                hidl_vec<uint8_t>();  // resize(0)
          } else {
            MY_LOGW(
                "fail to write physical meta to mResultMetadataQueue, data=%p, "
                "size=%zu",
                hidl_result.physicalCameraMetadata[i].metadata.data(),
                hidl_result.physicalCameraMetadata[i].metadata.size());
          }
        }
      }
    }
  } else {
    MY_LOGD("callback meta by non-FMQ size(%zu)", hidl_results.size());
    for (size_t i = 0; i < hidl_results.size(); i++) {
      auto& hidl_result = hidl_results[i];
      // write logical meta into FMQ
      MY_LOGD("%zu: result.size(%zu)", i, hidl_result.v3_2.result.size());
      if (hidl_result.v3_2.result.size() > 0) {
        MY_LOGD("workaround: directly use CameraMetadata callback");
        // if (CC_LIKELY(
        //         pResultMetadataQueue->write(hidl_result.v3_2.result.data(),
        //                                     hidl_result.v3_2.result.size())))
        //                                     {
        //   hidl_result.v3_2.fmqResultSize = hidl_result.v3_2.result.size();
        //   hidl_result.v3_2.result = hidl_vec<uint8_t>();  // resize(0)
        // } else {
        //   hidl_result.v3_2.fmqResultSize = 0;
        //   MY_LOGW(
        //       "fail to write meta to mResultMetadataQueue, data=%p,
        //       size=%zu", hidl_result.v3_2.result.data(),
        //       hidl_result.v3_2.result.size());
        // }
      }

      // write physical meta into FMQ
      if (hidl_result.physicalCameraMetadata.size() > 0) {
        for (size_t i = 0; i < hidl_result.physicalCameraMetadata.size(); i++) {
          MY_LOGD(
              "workaround: directly use CameraMetadata callback physical "
              "result(%zu)",
              i);
          // if (CC_LIKELY(pResultMetadataQueue->write(
          //         hidl_result.physicalCameraMetadata[i].metadata.data(),
          //         hidl_result.physicalCameraMetadata[i].metadata.size()))) {
          //   hidl_result.physicalCameraMetadata[i].fmqMetadataSize =
          //       hidl_result.physicalCameraMetadata[i].metadata.size();
          //   hidl_result.physicalCameraMetadata[i].metadata =
          //       hidl_vec<uint8_t>();  // resize(0)
          // } else {
          //   MY_LOGW(
          //       "fail to write physical meta to mResultMetadataQueue,
          //       data=%p, " "size=%zu",
          //       hidl_result.physicalCameraMetadata[i].metadata.data(),
          //       hidl_result.physicalCameraMetadata[i].metadata.size());
          // }
        }
      }
    }
  }
#endif

  // call back
  hidl_vec<::android::hardware::camera::device::V3_4::CaptureResult>
      vecCaptureResult;
  vecCaptureResult.setToExternal(hidl_results.data(), hidl_results.size());

  for (auto& result : vecCaptureResult) {
    // MY_LOGD("%s", toString(result).c_str());
    for (auto& streambuffer : result.v3_2.outputBuffers)
      MY_LOGD("%s", toString(streambuffer).c_str());
  }

  MY_LOGD("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
  auto result =
      mCameraDeviceCallback->processCaptureResult_3_4(vecCaptureResult);
  if (CC_UNLIKELY(!result.isOk())) {
    MY_LOGE(
        "Transaction error in ICameraDeviceCallback::processCaptureResult_3_4: "
        "%s",
        result.description().c_str());
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::notify(
    const std::vector<mcam::android::NotifyMsg>& msgs) -> void {
  CAM_TRACE_CALL();
  if (!msgs.empty()) {
    // convert hal NotifyMsg to hidl
    std::vector<::android::hardware::camera::device::V3_2::NotifyMsg> hidl_msgs;
    auto res = convertToHidlNotifyMsgs(msgs, &hidl_msgs);
    if (OK != res) {
      MY_LOGE("Fail to convert hal NotifyMsg to hidl");
      return;
    }
    hidl_vec<NotifyMsg> vecNotifyMsg;
    vecNotifyMsg.setToExternal(hidl_msgs.data(), hidl_msgs.size());

    for (auto& msg : vecNotifyMsg) {
      MY_LOGD("%s", toString(msg).c_str());
    }
    MY_LOGD("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
    auto result = mCameraDeviceCallback->notify(vecNotifyMsg);
    if (CC_UNLIKELY(!result.isOk())) {
      MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
              result.description().c_str());
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::requestStreamBuffers(
    const std::vector<mcam::android::BufferRequest>& vBufferRequests,
    std::vector<mcam::android::StreamBufferRet>* pvBufferReturns)
    -> mcam::android::BufferRequestStatus {
  CAM_TRACE_CALL();
#warning "implement, and remember to cache buffer handle"
  // ::android::hardware::camera::device::V3_5::BufferRequestStatus status;
  // BufferRequestStatus hidl_status;
  // hidl_vec<StreamBufferRet> stream_buffer_returns;
  // auto cb_status = hidl_device_callback_->requestStreamBuffers(
  //     hidl_buffer_requests, [&hidl_status, &stream_buffer_returns](
  //                             BufferRequestStatus status_ret,
  //                             const hidl_vec<StreamBufferRet>& buffer_ret) {
  //     hidl_status = status_ret;
  //     stream_buffer_returns = std::move(buffer_ret);
  // });
  // if (!cb_status.isOk()) {
  //     ALOGE("%s: Transaction request stream buffers error: %s", __FUNCTION__,
  //           cb_status.description().c_str());
  //     return BufferRequestStatus::kFailedUnknown;
  // }
  return mcam::android::BufferRequestStatus::FAILED_UNKNOWN;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::returnStreamBuffers(
    const std::vector<mcam::android::StreamBuffer>& buffers) -> void {
  CAM_TRACE_CALL();
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::getCaptureResultMetadataQueueDesc()
    -> const ResultMetadataQueue::Descriptor* {
  return mResultMetadataQueue->getDesc();
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::getCameraDeviceCallback()
    -> ::android::sp<ICameraDeviceCallback> {
  return mCameraDeviceCallback;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::convertToHidlCaptureResults(
    const std::vector<mcam::android::CaptureResult>& hal_captureResults,
    std::vector<::android::hardware::camera::device::V3_4::CaptureResult>*
        hidl_captureResults) -> ::android::status_t {
  CAM_TRACE_CALL();

  for (auto& hal_result : hal_captureResults) {
    ::android::hardware::camera::device::V3_4::CaptureResult hidl_Result = {};
    convertToHidlCaptureResult(hal_result, &hidl_Result);
    hidl_captureResults->push_back(std::move(hidl_Result));
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::convertToHidlCaptureResult(
    const mcam::android::CaptureResult& hal_captureResult,
    ::android::hardware::camera::device::V3_4::CaptureResult*
        hidl_captureResult) -> ::android::status_t {
  CAM_TRACE_CALL();

  // Convert the result metadata, and add it to CaptureResult
  if (hal_captureResult.result != nullptr &&
      get_camera_metadata_size(hal_captureResult.result) != 0) {
    hidl_vec<uint8_t> hidl_meta;
    hidl_meta.setToExternal(
        reinterpret_cast<uint8_t*>(
            const_cast<camera_metadata*>(hal_captureResult.result)),
        get_camera_metadata_size(hal_captureResult.result));
    hidl_captureResult->v3_2.result = std::move(hidl_meta);
  }

  // 2. Convert outputBuffers to HIDL
  if (!hal_captureResult.outputBuffers.empty()) {
    hidl_vec<StreamBuffer> vOutBuffers;
    size_t Psize = hal_captureResult.outputBuffers.size();
    vOutBuffers.resize(Psize);

    for (size_t i = 0; i < Psize; i++) {
      status_t res = convertToHidlStreamBuffer(
          hal_captureResult.outputBuffers[i], &vOutBuffers[i]);
      if (res != OK) {
        MY_LOGE("Converting hidl stream buffer failed: %s(%d)", strerror(-res),
                res);
        return res;
      }
    }
    hidl_captureResult->v3_2.outputBuffers = std::move(vOutBuffers);
  } else {
    hidl_captureResult->v3_2.outputBuffers =
        hidl_vec<::android::hardware::camera::device::V3_2::StreamBuffer>();
  }
  // 3. Convert inputBuffer to HIDL
  if (hal_captureResult.inputBuffer.streamId !=
      mcam::android::NO_STREAM) {
    hidl_vec<StreamBuffer> vInputBuffer(1);

    status_t res = convertToHidlStreamBuffer(hal_captureResult.inputBuffer,
                                             &vInputBuffer[0]);
    if (res != OK) {
      MY_LOGE("Converting hidl stream buffer failed: %s(%d)", strerror(-res),
              res);
      return res;
    }
    hidl_captureResult->v3_2.inputBuffer = vInputBuffer[0];
  } else {
    hidl_captureResult->v3_2.inputBuffer = {
        .streamId = -1};  // force assign -1 indicating no input buffer
  }

  hidl_captureResult->v3_2.frameNumber = hal_captureResult.frameNumber;
  hidl_captureResult->v3_2.fmqResultSize = 0;
  hidl_captureResult->v3_2.partialResult = hal_captureResult.partialResult;
  MY_LOGD("partial result = %u", hidl_captureResult->v3_2.partialResult);

  // physical meta call back
  if (!hal_captureResult.physicalCameraMetadata.empty()) {
    // write physical meta into FMQ with the last partial logical meta
    hidl_captureResult->physicalCameraMetadata.resize(
        hal_captureResult.physicalCameraMetadata.size());
    for (size_t i = 0; i < hal_captureResult.physicalCameraMetadata.size();
         i++) {
      camera_metadata* metadata = const_cast<camera_metadata_t*>(
          hal_captureResult.physicalCameraMetadata[i].metadata);
      size_t metadata_size = get_camera_metadata_size(
          hal_captureResult.physicalCameraMetadata[i].metadata);

      hidl_vec<uint8_t> physicTemp;
      physicTemp.setToExternal(
          reinterpret_cast<uint8_t*>(const_cast<camera_metadata*>(metadata)),
          metadata_size);
      auto& physicalCameraId =
          hal_captureResult.physicalCameraMetadata[i].physicalCameraId;
      ::android::hardware::camera::device::V3_4::PhysicalCameraMetadata
          physicalCameraMetadata = {
              .fmqMetadataSize = 0,
              .metadata = std::move(physicTemp),
          };
      if (physicalCameraId == -1) {
        physicalCameraMetadata.physicalCameraId = "";
      } else {
        physicalCameraMetadata.physicalCameraId =
            std::to_string(physicalCameraId);
      }
      hidl_captureResult->physicalCameraMetadata[i] = physicalCameraMetadata;
    }
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::convertToHidlStreamBuffer(
    const mcam::android::StreamBuffer& hal_buffer,
    ::android::hardware::camera::device::V3_2::StreamBuffer* hidl_buffer)
    -> ::android::status_t {
  CAM_TRACE_CALL();

  auto createNativeHandle = [](int dup_fd) -> native_handle_t* {
    if (-1 != dup_fd) {
      auto handle = ::native_handle_create(/*numFds*/ 1, /*numInts*/ 0);
      if (CC_LIKELY(handle)) {
        handle->data[0] = dup_fd;
        return handle;
      }
    }
    return nullptr;
  };

  if (hidl_buffer == nullptr) {
    MY_LOGE("hidl_buffer is nullptr.");
    return BAD_VALUE;
  }

  hidl_buffer->streamId = hal_buffer.streamId;
  hidl_buffer->bufferId = hal_buffer.bufferId;
  hidl_buffer->buffer = nullptr;
  hidl_buffer->status =
      static_cast<::android::hardware::camera::device::V3_2::BufferStatus>(
          hal_buffer.status);
  hidl_buffer->acquireFence = createNativeHandle(hal_buffer.acquireFenceFd);
  hidl_buffer->releaseFence.setTo(createNativeHandle(hal_buffer.releaseFenceFd),
                                  true);

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::convertToHidlNotifyMsgs(
    const std::vector<mcam::android::NotifyMsg>& hal_messages,
    std::vector<::android::hardware::camera::device::V3_2::NotifyMsg>*
        hidl_messages) -> ::android::status_t {
  CAM_TRACE_CALL();
  if (hidl_messages == nullptr) {
    MY_LOGE("hidl_message is nullptr.");
    return BAD_VALUE;
  }

  for (auto& hal_msg : hal_messages) {
    ::android::hardware::camera::device::V3_2::NotifyMsg hidl_msg = {};

    switch (hal_msg.type) {
      case mcam::android::MsgType::ERROR: {
        hidl_msg.type =
            ::android::hardware::camera::device::V3_2::MsgType::ERROR;
        auto res_error =
            convertToHidlErrorMessage(hal_msg.msg.error, &hidl_msg.msg.error);
        if (res_error != OK) {
          MY_LOGE("Converting to HIDL error message failed: %s(%d)",
                  strerror(-res_error), res_error);
          return res_error;
        }
        break;
      }
      case mcam::android::MsgType::SHUTTER: {
        hidl_msg.type =
            ::android::hardware::camera::device::V3_2::MsgType::SHUTTER;
        auto res_shutter = convertToHidlShutterMessage(hal_msg.msg.shutter,
                                                       &hidl_msg.msg.shutter);
        if (res_shutter != OK) {
          MY_LOGE("Converting to HIDL shutter message failed: %s(%d)",
                  strerror(-res_shutter), res_shutter);
          return res_shutter;
        }
        break;
      }
      default: {
        MY_LOGE("Unknown message type: %u", hal_msg.type);
        return BAD_VALUE;
      }
    }

    hidl_messages->push_back(std::move(hidl_msg));
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::convertToHidlErrorMessage(
    const mcam::android::ErrorMsg& hal_error,
    ::android::hardware::camera::device::V3_2::ErrorMsg* hidl_error)
    -> ::android::status_t {
  CAM_TRACE_CALL();
  if (hidl_error == nullptr) {
    MY_LOGE("hidl_error is nullptr.");
    return BAD_VALUE;
  }

  hidl_error->frameNumber = hal_error.frameNumber;
  hidl_error->errorStreamId = hal_error.errorStreamId;

  switch (hal_error.errorCode) {
    case mcam::android::ErrorCode::ERROR_DEVICE:
      hidl_error->errorCode =
          ::android::hardware::camera::device::V3_2::ErrorCode::ERROR_DEVICE;
      break;
    case mcam::android::ErrorCode::ERROR_REQUEST:
      hidl_error->errorCode =
          ::android::hardware::camera::device::V3_2::ErrorCode::ERROR_REQUEST;
      break;
    case mcam::android::ErrorCode::ERROR_RESULT:
      hidl_error->errorCode =
          ::android::hardware::camera::device::V3_2::ErrorCode::ERROR_RESULT;
      break;
    case mcam::android::ErrorCode::ERROR_BUFFER:
      hidl_error->errorCode =
          ::android::hardware::camera::device::V3_2::ErrorCode::ERROR_BUFFER;
      break;
    default:
      MY_LOGE("Unknown error code: %u", hal_error.errorCode);
      return BAD_VALUE;
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceCallback::convertToHidlShutterMessage(
    const mcam::android::ShutterMsg& hal_shutter,
    ::android::hardware::camera::device::V3_2::ShutterMsg* hidl_shutter)
    -> ::android::status_t {
  CAM_TRACE_CALL();
  if (hidl_shutter == nullptr) {
    MY_LOGE("hidl_shutter is nullptr.");
    return BAD_VALUE;
  }

  hidl_shutter->frameNumber = hal_shutter.frameNumber;
  hidl_shutter->timestamp = hal_shutter.timestamp;
  return OK;
}
};  // namespace hidl
};  // namespace mcam
