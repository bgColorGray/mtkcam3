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

#include <memory>
//
using ::android::Mutex;
using ::android::OK;
using ::android::DEAD_OBJECT;
using ::android::NOT_ENOUGH_DATA;

#define ThisNamespace CustCallback::ResultHandler

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
ThisNamespace::ResultHandler(std::shared_ptr<CommonInfo> pCommonInfo,
                             android::sp<FrameHandler> pFrameHandler)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) +
                    "-ResultHandler"},
      mCommonInfo(pCommonInfo),
      mFrameHandler(pFrameHandler) {}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  MY_LOGD("[destroy] join +");
  this->requestExit();
  this->join();
  MY_LOGD("[destroy] join -");

  mFrameHandler = nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
// Good place to do one-time initializations
auto ThisNamespace::readyToRun() -> android::status_t {
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::requestExit() -> void {
  MY_LOGD("+ %s", mInstanceName.c_str());
  //
  {
    Mutex::Autolock _l(mResultQueueLock);
    Thread::requestExit();
    mResultQueueCond.broadcast();
  }
  //
  MY_LOGD("- %s", mInstanceName.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::threadLoop() -> bool {
  std::vector<CaptureResult> vResult;
  std::vector<NotifyMsg> vMsg;
  int err = dequeResult(vResult, vMsg);
  if (OK == err && (!vResult.empty() || !vMsg.empty())) {
    mFrameHandler->update(vResult, vMsg);
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dequeResult(std::vector<CaptureResult>& rvResult,
                                std::vector<NotifyMsg>& rvMsg) -> int {
  int err = OK;
  //
  Mutex::Autolock _l(mResultQueueLock);
  //
  while (!exitPending() && mResultQueue.empty() && mMsgQueue.empty()) {
    err = mResultQueueCond.wait(mResultQueueLock);
    MY_LOGW_IF(OK != err,
              "exitPending:%d ResultQueue#:%zu MsgQueue#:%zu err:%d(%s)",
               exitPending(), mResultQueue.size(), mMsgQueue.size(),
               err, ::strerror(-err));
  }
  //
  bool anyUpdate = false;
  if (!mResultQueue.empty()) {
    rvResult = mResultQueue;
    anyUpdate = true;
  }
  //
  if (!mMsgQueue.empty()) {
    rvMsg = mMsgQueue;
    anyUpdate = true;
  }
  //
  if (!anyUpdate) {
    MY_LOGD_IF(getLogLevel() >= 1, "empty ResultQueue & MsgQueue",
               rvResult.size(), rvMsg.size());
    err = NOT_ENOUGH_DATA;
  } else {
    mResultQueue.clear();
    mMsgQueue.clear();
    err = OK;
  }
  //
  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::enqueResult(const hidl_vec<CaptureResult>& rvResult,
                                const hidl_vec<NotifyMsg>& rvMsg) -> int {
  auto appendCbData = [](auto& src, auto& dst) -> void {
    if (dst.empty()) {
      dst = std::move(src);
    } else {
      dst.reserve(dst.size() + src.size());
      std::move(std::begin(src), std::end(src), std::back_inserter(dst));
    }
  };

  NSCam::Utils::CamProfile profile(__FUNCTION__, "CustCallback::ResultHandler");
  Mutex::Autolock _l(mResultQueueLock);
  //
  if (exitPending()) {
    MY_LOGW("Dead ResultQueue");
    return DEAD_OBJECT;
  }
  //
  profile.print_overtime(1,
                         "ResultQueue#:%zu, mMsgQueue#:%zu",
                         rvResult.size(), rvMsg.size());
  // convert from hidl_vec to std::vector
  appendCbData(rvResult, mResultQueue);
  appendCbData(rvMsg, mMsgQueue);
  //
  mResultQueueCond.broadcast();
  //
  profile.print_overtime(1, "-");
  return OK;
}

};  // namespace v3
};  // namespace NSCam
