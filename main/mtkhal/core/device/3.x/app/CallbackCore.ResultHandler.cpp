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

#include "CallbackCore.h"
#include "MyUtils.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
//
using ::android::DEAD_OBJECT;
using ::android::Mutex;
using ::android::NOT_ENOUGH_DATA;
using ::android::OK;

#define ThisNamespace CallbackCore::ResultHandler

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
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::ResultHandler(std::shared_ptr<CommonInfo> pCommonInfo,
                             ::android::sp<FrameHandler> pFrameHandler)
    : mInstanceName{pCommonInfo->userName + "-" +
                    std::to_string(pCommonInfo->mInstanceId) +
                    "-ResultHandler"},
      mCommonInfo(pCommonInfo),
      mFrameHandler(pFrameHandler) {}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  MY_LOGI("[destroy] join +");
  this->requestExit();
  this->join();
  MY_LOGI("[destroy] join -");

  mFrameHandler = nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
// Good place to do one-time initializations
auto ThisNamespace::readyToRun() -> ::android::status_t {
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
  ResultQueue vResult;
  int err = dequeResult(vResult);
  if (OK == err && !vResult.empty()) {
    mFrameHandler->update(vResult);
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dequeResult(ResultQueue& rvResult) -> int {
  int err = OK;
  //
  Mutex::Autolock _l(mResultQueueLock);
  //
  while (!exitPending() && mResultQueue.empty()) {
    err = mResultQueueCond.wait(mResultQueueLock);
    MY_LOGW_IF(
        OK != err, "exitPending:%d ResultQueue#:",
        "[s:%zu, e:%zu, m:%zu, hm:%zu, pm:%zu, oi:%zu, ii:%zu] err:%d(%s)",
        exitPending(), mResultQueue.vShutterResult.size(),
        mResultQueue.vMessageResult.size(), mResultQueue.vMetaResult.size(),
        mResultQueue.vHalMetaResult.size(),
        mResultQueue.vPhysicMetaResult.size(),
        mResultQueue.vOutputImageResult.size(),
        mResultQueue.vInputImageResult.size(), err, ::strerror(-err));
  }
  //
  if (mResultQueue.empty()) {
    MY_LOGD_IF(getLogLevel() >= 1, "empty queue");
    rvResult.clear();
    err = NOT_ENOUGH_DATA;
  } else {
    //  If the queue is not empty, deque all items from the queue.
    rvResult = mResultQueue;
    mResultQueue.clear();
    err = OK;
  }
  //
  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::enqueResult(ResultQueue const& rQueue) -> int {
  NSCam::Utils::CamProfile profile(__FUNCTION__, "CallbackCore::ResultHandler");
  Mutex::Autolock _l(mResultQueueLock);
  //
  if (exitPending()) {
    MY_LOGW("Dead ResultQueue");
    return DEAD_OBJECT;
  }

#define APPEND_MAP(_map)    \
  if (!rQueue._map.empty()) \
    mResultQueue._map.insert(rQueue._map.begin(), rQueue._map.end());

  APPEND_MAP(vShutterResult);
  APPEND_MAP(vMessageResult);
#undef APPEND_MAP
  //

#define APPEND_VEC(_map)                                         \
  for (const auto& pair : rQueue._map) {                         \
    auto& srcVec = pair.second;                                  \
    if (mResultQueue._map.count(pair.first) > 0) {               \
      auto& srcVec = pair.second;                                \
      auto& dstVec = mResultQueue._map.at(pair.first);           \
      dstVec.reserve(dstVec.size() + srcVec.size());             \
      dstVec.insert(dstVec.end(), srcVec.begin(), srcVec.end()); \
    } else {                                                     \
      mResultQueue._map.insert(pair);                            \
    }                                                            \
  }

  APPEND_VEC(vMetaResult);
  APPEND_VEC(vHalMetaResult);
  APPEND_VEC(vPhysicMetaResult);
  APPEND_VEC(vOutputImageResult);
  APPEND_VEC(vInputImageResult);
#undef APPEND_VEC
  //
  for (auto const& pair : rQueue.vMetaResult) {
    uint32_t frameNumber = pair.first;
    if (pair.second.isRealTimePartial == true) {
      mResultQueue.vMetaResult.at(frameNumber).isRealTimePartial = true;
    }
    if (pair.second.hasLastPartial == true) {
      mResultQueue.vMetaResult.at(frameNumber).hasLastPartial = true;
    }
  }

  //
  mResultQueueCond.broadcast();
  //
  profile.print_overtime(
      1,
      "- enqueResult over time, enque size"
      "s:%zu e:%zu m:%zu hm:%zu pm:%zu oi:%zu ii:%zu",
      mResultQueue.vShutterResult.size(), mResultQueue.vMessageResult.size(),
      mResultQueue.vMetaResult.size(), mResultQueue.vHalMetaResult.size(),
      mResultQueue.vPhysicMetaResult.size(),
      mResultQueue.vOutputImageResult.size(),
      mResultQueue.vInputImageResult.size());
  return OK;
}

};  // namespace core
};  // namespace mcam
