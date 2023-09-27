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

#include "MtkCallbackAdaptor.h"
#include "MyUtils.h"

#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using ::android::Condition;
using ::android::DEAD_OBJECT;
using ::android::Mutex;
using ::android::NO_ERROR;
using ::android::OK;
using ::android::status_t;
using ::android::String8;
using ::android::Vector;
using NSCam::v3::STREAM_BUFFER_STATUS;

#define ThisNamespace MtkCallbackAdaptor::CallbackHandler

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...)                                       \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_WARN(level, fmt, arg...)                                        \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_ERROR_ULOG(level, fmt, arg...)                                  \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                                     \
  do {                                                                   \
    CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                   ##arg);                                               \
  } while (0)

#define MY_LOGV(...) MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...) MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...) MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...) MY_WARN(W, __VA_ARGS__)
#define MY_LOGE(...) MY_ERROR_ULOG(E, __VA_ARGS__)
#define MY_LOGA(...) MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...) MY_ERROR(F, __VA_ARGS__)
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
    if (MY_UNLIKELY(cond)) {  \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if (MY_UNLIKELY(cond)) {  \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if (MY_UNLIKELY(cond)) {  \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if (MY_UNLIKELY(cond)) {  \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace mcam {
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
static const int kDumpLockRetries = 50;
static const int kDumpLockSleep = 60000;

static bool tryLock(Mutex& mutex) {
  bool locked = false;
  for (int i = 0; i < kDumpLockRetries; ++i) {
    if (mutex.tryLock() == NO_ERROR) {
      locked = true;
      break;
    }
    usleep(kDumpLockSleep);
  }
  return locked;
}

/******************************************************************************
 *
 ******************************************************************************/
static void convertToDebugString(
    MtkCallbackAdaptor::MtkCallbackParcel const& cbParcel,
    android::Vector<android::String8>& out) {
  /*
      frameNumber 80 errors#:10 shutter:10 o:meta#:10 i:image#:10 o:image#:10
          {ERROR_DEVICE/ERROR_REQUEST/ERROR_RESULT}
          {ERROR_BUFFER streamId:123 bufferId:456}
          OUT Meta  -
              partial#:123 "xxxxxxxxxx"
          IN Image -
              streamId:01 bufferId:04 OK
          OUT Image -
              streamId:02 bufferId:04 ERROR with releaseFence
  */
  std::map<uint32_t, uint32_t> NotifyMsgIdMap;
  std::map<uint32_t, std::vector<uint32_t>> ErrorMsgIdMap;
  std::map<uint32_t, std::vector<uint32_t>> CaptureResultIdMap;
  std::map<uint32_t, std::vector<uint32_t>> BufferResultIdMap;

  // update notify msg id map
  for (size_t i = 0; i < cbParcel.vNotifyMsg.size(); ++i) {
    uint32_t frameNumber = cbParcel.vNotifyMsg[i].msg.shutter.frameNumber;
    NotifyMsgIdMap[frameNumber] = i;
  }
  // update error msg id map
  for (size_t i = 0; i < cbParcel.vErrorMsg.size(); ++i) {
    uint32_t frameNumber = cbParcel.vErrorMsg[i].msg.error.frameNumber;
    ErrorMsgIdMap[frameNumber].push_back(i);
  }
// update capture result & buffer result
#define UPDATE_CAPTURE_RESULT_ID(vData, map)     \
  for (size_t i = 0; i < vData.size(); ++i) {    \
    uint32_t frameNumber = vData[i].frameNumber; \
    map[frameNumber].push_back(i);               \
  }
  UPDATE_CAPTURE_RESULT_ID(cbParcel.vCaptureResult, CaptureResultIdMap);
  UPDATE_CAPTURE_RESULT_ID(cbParcel.vBufferResult, BufferResultIdMap);
#undef UPDATE_CAPTURE_RESULT_ID

  for (auto it = cbParcel.vFrameNumber.begin();
       it != cbParcel.vFrameNumber.end(); it++) {
    uint32_t frameNumber = *it;
    String8 log("");
    log += String8::format("frameNumber %u", frameNumber);
    if (!ErrorMsgIdMap[frameNumber].empty()) {
      log += String8::format(" errors#:%zu", ErrorMsgIdMap[frameNumber].size());
    }
    if (NotifyMsgIdMap.find(frameNumber) != NotifyMsgIdMap.end()) {
      log += String8::format(" shutter:%" PRId64 "",
                             cbParcel.vNotifyMsg[NotifyMsgIdMap[frameNumber]]
                                 .msg.shutter.timestamp);
    }
    if (!CaptureResultIdMap[frameNumber].empty()) {
      size_t phyMetaSize = 0;
      size_t metaSize = 0, halMetaSize = 0;
      for (auto const& id : CaptureResultIdMap[frameNumber]) {
        phyMetaSize +=
            cbParcel.vCaptureResult[id].physicalCameraMetadata.size();
        metaSize += cbParcel.vCaptureResult[id].result.count() > 0 ? 1 : 0;
        halMetaSize +=
            cbParcel.vCaptureResult[id].halResult.count() > 0 ? 1 : 0;
      }
      if (metaSize)
        log += String8::format(" o:meta#:%zu", metaSize);
      if (halMetaSize)
        log += String8::format(" o:hal meta#:%zu", halMetaSize);
      if (phyMetaSize)
        log += String8::format(" o:physic meta#:%zu", phyMetaSize);
    }
    if (!BufferResultIdMap[frameNumber].empty()) {
      size_t inImgSize = 0, outImgSize = 0;
      for (auto const& id : BufferResultIdMap[frameNumber]) {
        inImgSize += cbParcel.vBufferResult[id].inputBuffers.size();
        outImgSize += cbParcel.vBufferResult[id].outputBuffers.size();
      }
      if (inImgSize)
        log += String8::format(" i:image#:%zu", inImgSize);
      if (outImgSize)
        log += String8::format(" o:image#:%zu", outImgSize);
    }
    out.push_back(log);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::CallbackHandler(std::shared_ptr<CommonInfo> pCommonInfo)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) +
                    "-CallbackHandler"},
      mCommonInfo(pCommonInfo) {}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  MY_LOGI("[destroy] mCallbackHandler->join +");
  this->requestExit();
  this->join();
  MY_LOGI("[destroy] mCallbackHandler->join -");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer __unused,
                              const std::vector<std::string>& options __unused)
    -> void {
  auto logQueue = [&](auto const& queue) {
    for (auto const& item : queue) {
      Vector<String8> logs;
      convertToDebugString(item, logs);
      for (auto const& log : logs) {
        printer.printFormatLine("  %s", log.c_str());
      }
    }
  };

  printer.printLine(" *Pending callback results*");

  {
    Mutex::Autolock _l1(mQueue1Lock);
    logQueue(mQueue1);

    if (tryLock(mQueue2Lock)) {
      if (!mQueue1.empty()) {
        printer.printLine("  ***");
      }
      logQueue(mQueue2);
      mQueue2Lock.unlock();
    }
  }

  printer.printLine("");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::waitUntilDrained(nsecs_t const timeout) -> int {
  nsecs_t const startTime = ::systemTime();
  //
  auto timeoutToWait = [=]() {
    nsecs_t const elapsedInterval = (::systemTime() - startTime);
    nsecs_t const timeoutToWait =
        (timeout > elapsedInterval) ? (timeout - elapsedInterval) : 0;
    return timeoutToWait;
  };
  //
  auto waitEmpty = [=](Mutex& lock, Condition& cond, auto const& queue) -> int {
    int err = OK;
    Mutex::Autolock _l(lock);
    while (!exitPending() && !queue.empty()) {
      err = cond.waitRelative(lock, timeoutToWait());
      if (OK != err) {
        break;
      }
    }
    //
    if (queue.empty()) {
      return OK;
    }
    if (exitPending()) {
      return DEAD_OBJECT;
    }
    return err;
  };
  //
  //
  int err = OK;
  if (OK != (err = waitEmpty(mQueue1Lock, mQueue1Cond, mQueue1)) ||
      OK != (err = waitEmpty(mQueue2Lock, mQueue2Cond, mQueue2))) {
    MY_LOGW("mQueue1:#%zu mQueue2:#%zu exitPending:%d timeout(ns):%" PRId64
            " elapsed(ns):%" PRId64 " err:%d(%s)",
            mQueue1.size(), mQueue2.size(), exitPending(), timeout,
            (::systemTime() - startTime), err, ::strerror(-err));
  }
  //
  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::push(const MtkCallbackParcel& item) -> void {
  Mutex::Autolock _l(mQueue1Lock);

  // Transfers all elements from item to mQueue1.
  // After that, item is empty.
  mQueue1.push_back(item);

  mQueue1Cond.broadcast();
}

/******************************************************************************
 *
 ******************************************************************************/
// Good place to do one-time initializations
auto ThisNamespace::readyToRun() -> status_t {
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::requestExit() -> void {
  MY_LOGD("+ %s", mInstanceName.c_str());
  //
  {
    Mutex::Autolock _l1(mQueue1Lock);
    Mutex::Autolock _l2(mQueue2Lock);
    Thread::requestExit();
    mQueue1Cond.broadcast();
    mQueue2Cond.broadcast();
  }
  //
  MY_LOGD("- %s", mInstanceName.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::threadLoop() -> bool {
  if (!waitUntilQueue1NotEmpty()) {
    MY_LOGD_IF(1, "Queue1 is empty");
    return true;
  }
  //
  {
    Mutex::Autolock _l1(mQueue1Lock);
    Mutex::Autolock _l2(mQueue2Lock);

    // Transfers all elements from mQueue1 to mQueue2.
    // After that, mQueue1 is empty.
    mQueue2.splice(mQueue2.end(), mQueue1);
    mQueue1Cond.broadcast();
  }
  //
  performCallback();
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::waitUntilQueue1NotEmpty() -> bool {
  Mutex::Autolock _l(mQueue1Lock);

  while (!exitPending() && mQueue1.empty()) {
    int err = mQueue1Cond.wait(mQueue1Lock);
    MY_LOGW_IF(OK != err, "exitPending:%d mQueue1#:%zu err:%d(%s)",
               exitPending(), mQueue1.size(), err, ::strerror(-err));
  }

  return !mQueue1.empty();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::performCallback() -> void {
  CAM_ULOGM_FUNCLIFE();
  MtkCallbackParcel hidlCbParcel;

  auto appendCbData = [](auto& src, auto& dst) -> void {
    if (dst.empty()) {
      dst = std::move(src);
    } else {
      dst.reserve(dst.size() + src.size());
      std::move(std::begin(src), std::end(src), std::back_inserter(dst));
      src.clear();
    }
  };

  {
    Mutex::Autolock _l2(mQueue2Lock);

    for (auto& parcel : mQueue2) {
      hidlCbParcel.vFrameNumber.insert(parcel.vFrameNumber.begin(),
                                       parcel.vFrameNumber.end());
      appendCbData(parcel.vNotifyMsg, hidlCbParcel.vNotifyMsg);
      appendCbData(parcel.vErrorMsg, hidlCbParcel.vErrorMsg);
      appendCbData(parcel.vCaptureResult, hidlCbParcel.vCaptureResult);
      appendCbData(parcel.vBufferResult, hidlCbParcel.vBufferResult);
    }
    //
    if (MY_UNLIKELY(getLogLevel() >= 1)) {
      Vector<String8> logs;
      convertToDebugString(hidlCbParcel, logs);
      for (auto const& l : logs) {
        MY_LOGD("%s", l.c_str());
      }
    }
    //  send shutter callbacks
    if (!hidlCbParcel.vNotifyMsg.empty()) {
      if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
        cb->notify(hidlCbParcel.vNotifyMsg);
      } else {
        MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                mCommonInfo->mDeviceCallback.use_count());
      }
    }
    //  send metadata callbacks
    if (!hidlCbParcel.vCaptureResult.empty()) {
      if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
        cb->processCaptureResult(hidlCbParcel.vCaptureResult);
      } else {
        MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                mCommonInfo->mDeviceCallback.use_count());
      }
    }
    //  send error callback
    if (!hidlCbParcel.vErrorMsg.empty()) {
      if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
        cb->notify(hidlCbParcel.vErrorMsg);
      } else {
        MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                mCommonInfo->mDeviceCallback.use_count());
      }
    }
    //  send image callback
    if (!hidlCbParcel.vBufferResult.empty()) {
      if (auto cb = mCommonInfo->mDeviceCallback.lock()) {
        cb->processCaptureResult(hidlCbParcel.vBufferResult);
      } else {
        MY_LOGW("DeviceCallback is destructed, use_count=%ld",
                mCommonInfo->mDeviceCallback.use_count());
      }
    }

    mQueue2.clear();
    mQueue2Cond.broadcast();  // inform anyone of empty mQueue2
    MY_LOGD_IF(getLogLevel() >= 1, "-");
  }
}
};  // namespace core
};  // namespace mcam
