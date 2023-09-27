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

using ::android::Condition;
using ::android::DEAD_OBJECT;
using ::android::Mutex;
using ::android::NO_ERROR;
using ::android::OK;
using ::android::status_t;
using ::android::String8;
using ::android::Vector;

#define ThisNamespace CustCallback::CallbackHandler

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg)
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
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace NSCam {
namespace custom_dev3 {
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
ThisNamespace::CallbackHandler(std::shared_ptr<CommonInfo> pCommonInfo)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) +
                    "-CallbackHandler"},
      mCommonInfo(pCommonInfo) {
  mResultMetadataQueue = mCommonInfo->mResultMetadataQueue;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  MY_LOGD("[destroy] mCallbackHandler->join +");
  this->requestExit();
  this->join();
  MY_LOGD("[destroy] mCallbackHandler->join -");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer __unused,
                              const std::vector<std::string>& options __unused)
    -> void {
  auto logQueue = [&](auto const& queue) {
    for (auto const& it : queue) {
      for (auto const& res : it.vResult) {
        MY_LOGD("%s", dumpCaptureResult(res).c_str());
      }
      for (auto const& msg : it.vMsg) {
        MY_LOGD("%s", dumpNotifyMsg(msg).c_str());
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
auto ThisNamespace::push(FrameParcel& item) -> void {
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
    mQueue2.insert(mQueue2.end(), mQueue1.begin(), mQueue1.end());
    mQueue1.clear();

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

  auto appendCbData = [](auto& src, auto& dst) -> void {
    if (dst.empty()) {
      dst = std::move(src);
    } else {
      dst.reserve(dst.size() + src.size());
      std::move(std::begin(src), std::end(src), std::back_inserter(dst));
      src.clear();
    }
  };

  std::vector<CaptureResult> vCaptureResult;
  std::vector<NotifyMsg> vNotifyMsg;

  {
    Mutex::Autolock _l2(mQueue2Lock);

    for (auto& parcel : mQueue2) {
      appendCbData(parcel.vMsg, vNotifyMsg);
      appendCbData(parcel.vResult, vCaptureResult);
    }

    if (CC_UNLIKELY(getLogLevel() >= 1)) {
      for (const auto& item : vCaptureResult)
        MY_LOGD("%s", dumpCaptureResult(item).c_str());
      for (const auto& item : vNotifyMsg)
        MY_LOGD("%s", dumpNotifyMsg(item).c_str());
    }
    //
    //  send notify callbacks
    if (!vNotifyMsg.empty()) {
      hidl_vec<NotifyMsg> vecNotifyMsg;
      vecNotifyMsg.setToExternal(vNotifyMsg.data(), vNotifyMsg.size());
      auto ret1 = mCommonInfo->mDeviceCallback->notify(vecNotifyMsg);
      if (CC_UNLIKELY(!ret1.isOk())) {
          MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s", ret1.description().c_str());
      }
    }
    //
    // send capture result callback
    if  (!vCaptureResult.empty()) {
      hidl_vec<CaptureResult> vecCaptureResult;
      vecCaptureResult.setToExternal(vCaptureResult.data(), vCaptureResult.size());
      auto ret2 = mCommonInfo->mDeviceCallback->processCaptureResult_3_4(vecCaptureResult);
      if (CC_UNLIKELY(!ret2.isOk())) {
        MY_LOGE("Transaction error in ICameraDeviceCallback::processCaptureResult: %s", ret2.description().c_str());
      }
    }

    // for (auto& result : vecCaptureResult) {
    //   MY_LOGI("%s", toString(result).c_str());
    //   for (auto& streambuffer : result.v3_2.outputBuffers)
    //     MY_LOGI("%s", toString(streambuffer).c_str());
    // }

    mQueue2.clear();
    mQueue2Cond.broadcast();  // inform anyone of empty mQueue2
    MY_LOGD_IF(getLogLevel() >= 1, "-");
  }
}

};  // namespace v3
};  // namespace NSCam
