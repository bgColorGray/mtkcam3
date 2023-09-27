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

#include "CustCallback.h"
#include "MyUtils.h"
// #include <utility>
//
using ::android::Mutex;
using ::android::OK;
using ::android::String8;
using NSCam::Utils::ULog::DetailsType;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::ULogPrinter;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
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

namespace NSCam {
namespace custom_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
String8 dumpCaptureResult(CaptureResult param) {
  String8 log = String8::format("[requestNo %u]", param.v3_2.frameNumber);
  // meta
  if (param.v3_2.result.size() > 0) {
    log += String8::format("meta : size=%zu, partial=%u ",
                           param.v3_2.result.size(),
                           param.v3_2.partialResult);
  }
  // physical meta
  for (const auto& item : param.physicalCameraMetadata) {
    log += String8::format("phy meta : camId=%s, size=%zu, ",
                           item.physicalCameraId.c_str(),
                           item.metadata.size());
  }
  // input buffer
  if (param.v3_2.inputBuffer.streamId != -1) {
    log += String8::format("img : input, streamId=%02" PRIx64 ", bufferId:%d, status:%s, ",
                           param.v3_2.inputBuffer.streamId,
                           param.v3_2.inputBuffer.bufferId,
                           param.v3_2.inputBuffer.status == BufferStatus::OK ? "OK" : "ERR");
  }
  // output buffer
  for (const auto& buf : param.v3_2.outputBuffers) {
    log += String8::format("img : output, streamId=%02" PRIx64 ", bufferId:%u, status:%s, ",
                           buf.streamId,
                           buf.bufferId,
                           buf.status == BufferStatus::OK ? "OK" : "ERR");
  }
  return log;
}

String8 dumpNotifyMsg(NotifyMsg param) {
  String8 log;
  if (param.type == MsgType::ERROR) {
    log += String8::format("[requestNo %u]", param.msg.error.frameNumber);
    log += String8::format("error : errorCode=%u",
                           param.msg.error.errorCode);
    if (param.msg.error.errorCode == ErrorCode::ERROR_BUFFER) {
      log += String8::format(", streamId=%02" PRIx64 "", param.msg.error.errorStreamId);
    }
  } else if (param.type == MsgType::SHUTTER) {
    log += String8::format("[requestNo %u]", param.msg.error.frameNumber);
    log += String8::format("shutter %" PRId64 "",
                           param.msg.shutter.timestamp);
  } else {
    log += String8::format("unknown");
  }
  return log;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ICustCallback::create(
    const CreationInfo& creationInfo)
    -> ICustCallback* {
  auto pInstance = new CustCallback(creationInfo);
  if (!pInstance || !pInstance->initialize()) {
    delete pInstance;
    return nullptr;
  }
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
CustCallback::CustCallback(
    const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>()),
      mInstanceName{std::to_string(creationInfo.mInstanceId) + ":CustCallback"} {
  //
  if (mCommonInfo != nullptr) {
    //
    int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
    if (loglevel == 0) {
      loglevel =
          ::property_get_int32("vendor.debug.camera.log.CustCallback", 0);
    }
    //
    mCommonInfo->mLogLevel = loglevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    mCommonInfo->mDeviceCallback = creationInfo.mCameraDeviceCallback;
    mCommonInfo->mAtMostMetaStreamCount = creationInfo.aAtMostMetaStreamCount;
    mCommonInfo->mResultMetadataQueueSize = creationInfo.mResultMetadataQueueSize;
    mCommonInfo->mResultMetadataQueue = creationInfo.mResultMetadataQueue;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
bool CustCallback::initialize() {
  android::status_t status = OK;
  //
  if (mCommonInfo == nullptr) {
    MY_LOGE("Bad mCommonInfo");
    return false;
  }
  //
  {
    mCallbackHandler = new CallbackHandler(mCommonInfo);
    if (mCallbackHandler == nullptr) {
      MY_LOGE("Bad mCallbackHandler");
      return false;
    } else {
      const std::string threadName{std::to_string(mCommonInfo->mInstanceId) +
                                   ":Cust-CbHdl"};
      status = mCallbackHandler->run(threadName.c_str());
      if (OK != status) {
        MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(),
                status, ::strerror(-status));
        return false;
      }
    }
    //
    mFrameHandler = new FrameHandler(mCommonInfo, mCallbackHandler);
    if (mFrameHandler == nullptr) {
      MY_LOGE("Bad mFrameHandler");
      return false;
    }
    //
    mResultHandler = new ResultHandler(mCommonInfo, mFrameHandler);
    if (mResultHandler == nullptr) {
      MY_LOGE("Bad mResultHandler");
      return false;
    } else {
      const std::string threadName{std::to_string(mCommonInfo->mInstanceId) +
                                   ":Cust-RstHdl"};
      status = mResultHandler->run(threadName.c_str());
      if (OK != status) {
        MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(),
                status, ::strerror(-status));
        return false;
      }
    }
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::destroy() -> void {
  auto resLock = mInterfaceLock.timedLock(::ms2ns(1000));
  if (OK != resLock) {
    MY_LOGW("timedLock failed; still go on to destroy");
  }

  // mFrameHandler->dumpIfHasInflightRequest();

  mResultHandler->destroy();
  mResultHandler = nullptr;

  mFrameHandler->destroy();
  mFrameHandler = nullptr;

  mCallbackHandler->destroy();
  mCallbackHandler = nullptr;

  if (OK == resLock) {
    mInterfaceLock.unlock();
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::configureStreams(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
      requestedConfiguration) -> int {
  CAM_ULOGM_FUNCLIFE();

  Mutex::Autolock _l(mInterfaceLock);

  ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG,
                         "[AppMgr-configureStreams] ");
  int status = OK;
  mFrameHandler->configureStreams(requestedConfiguration.streams);

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::submitRequests(
    const hidl_vec<CaptureRequest>& rRequests) -> int {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  for (const auto& rRequest : rRequests) {
    int err = mFrameHandler->registerFrame(rRequest);
    if (err != OK)
      return err;
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::processCaptureResult(
    const hidl_vec<CaptureResult>& param)  -> void {
  Mutex::Autolock _l(mInterfaceLock);
  auto pHandler = mResultHandler;
  if (pHandler) {
    if (getLogLevel() >= 1) {
      for (const auto& res : param)
        MY_LOGD("%s", dumpCaptureResult(res).c_str());
    }
    //
    pHandler->enqueResult(param, hidl_vec<NotifyMsg>(0));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::notify(
    const hidl_vec<NotifyMsg>& param) -> void {
  Mutex::Autolock _l(mInterfaceLock);
  auto pHandler = mResultHandler;
  if (pHandler) {  //
    if (getLogLevel() >= 1) {
      for (const auto& msg : param)
        MY_LOGD("%s", dumpNotifyMsg(msg).c_str());
    }
    //
    pHandler->enqueResult(hidl_vec<CaptureResult>(0), param);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::flushRequest(
    const hidl_vec<CaptureRequest>& requests) -> void {
  auto prepareBuffer = [this](
      StreamBuffer const& buffer,
      uint32_t frameNumber,
      bool isInput,
      auto& vCaptureResult) {
    if (-1 != buffer.streamId && 0 != buffer.bufferId) {
      // int acquire_fence = -1;
      // if (buffer.acquireFenceOp.type == FenceType::HDL) {
      //   if (buffer.acquireFenceOp.hdl != nullptr) {
      //     acquire_fence = ::dup(buffer.acquireFenceOp.hdl->data[0]);
      //   }
      // } else {
      //   acquire_fence = buffer.acquireFenceOp.fd;
      // }
      CaptureResult res {
        .v3_2.frameNumber = frameNumber,
      };
      if (isInput) {
        res.v3_2.inputBuffer = buffer;
      } else {
        res.v3_2.outputBuffers.resize(1);
        res.v3_2.outputBuffers[0] = buffer;
      }
      vCaptureResult.push_back(res);
    }
  };

  Mutex::Autolock _l(mInterfaceLock);
  CustCallback::FrameParcel cbList;
  for (auto const& req : requests) {
    uint32_t frameNumber =  req.v3_2.frameNumber;
    // error message
    NotifyMsg resMsg {
      .type = MsgType::ERROR,
      .msg.error.frameNumber = frameNumber,
      .msg.error.errorCode = ErrorCode::ERROR_REQUEST,
    };
    cbList.vMsg.push_back(std::move(resMsg));
    // input buffer
    prepareBuffer(req.v3_2.inputBuffer, frameNumber, true, cbList.vResult);
    // output buffer
    for (auto const& buf : req.v3_2.outputBuffers) {
      prepareBuffer(buf, frameNumber, false, cbList.vResult);
    }
  }
  mCallbackHandler->push(cbList);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::waitUntilDrained(nsecs_t const timeout) -> int {
  nsecs_t const startTime = ::systemTime();
  //
  auto timeoutToWait = [=]() {
    nsecs_t const elapsedInterval = (::systemTime() - startTime);
    nsecs_t const timeoutToWait =
        (timeout > elapsedInterval) ? (timeout - elapsedInterval) : 0;
    return timeoutToWait;
  };
  //
  //
  int err = OK;
  Mutex::Autolock _l(mInterfaceLock);

  (void)((OK == (err = mFrameHandler->waitUntilDrained(timeout))) &&
         (OK == (err = mCallbackHandler->waitUntilDrained(timeoutToWait()))));

  if (OK != err) {
    MY_LOGW("timeout(ns):%" PRId64 " err:%d(%s)", timeout, -err,
            ::strerror(-err));
    ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG,
                           DetailsType::DETAILS_DEBUG, "[waitUntilDrained] ");
    dumpStateLocked(logPrinter, {});
  }

  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CustCallback::dumpStateLocked(IPrinter& printer,
                                   const std::vector<std::string>& options)
    -> void {
  if (mFrameHandler != nullptr) {
    mFrameHandler->dumpState(printer, options);
  }
  if (mCallbackHandler != nullptr) {
    printer.printLine(" ");
    mCallbackHandler->dumpState(printer, options);
  }
}

};  // namespace v3
};  // namespace NSCam
