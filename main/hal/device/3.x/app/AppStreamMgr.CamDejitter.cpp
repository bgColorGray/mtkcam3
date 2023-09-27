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

#include "AppStreamMgr.h"
#include "MyUtils.h"

#include <mtkcam/def/UITypes.h>
#include <memory>
#include <utility>
#include <chrono>
#include <string>
#include <sstream>
#include <iostream>

using NSCam::MSize;
using NSCam::Type2Type;
using namespace NSCam;
using namespace NSCam::v3;

#define ThisNamespace AppStreamMgr::CamDejitterImp

/******************************************************************************
 * log macro
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...)                                           \
  do {                                                                         \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg);                        \
  } while (0)

#define MY_WARN(level, fmt, arg...)                                            \
  do {                                                                         \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg);                        \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                                           \
  do {                                                                         \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg);                        \
  } while (0)

#define MY_LOGV(...) MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...) MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...) MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...) MY_WARN(W, __VA_ARGS__)
#define MY_LOGE(...) MY_ERROR(E, __VA_ARGS__)
#define MY_LOGA(...) MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...) MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGV(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)
#define MY_LOGD_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGD(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)
#define MY_LOGI_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGI(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)
#define MY_LOGW_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGW(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)
#define MY_LOGE_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGE(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)
#define MY_LOGA_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGA(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)
#define MY_LOGF_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      MY_LOGF(__VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)


/******************************************************************************
 *
 ******************************************************************************/

static inline std::pair<int32_t, MSize>
maxPair(const std::pair<int32_t, MSize> &lhs,
        const std::pair<int32_t, MSize> &rhs) {
  return (lhs.second.size() > rhs.second.size()) ? lhs : rhs;
}
static inline std::pair<int32_t, MSize>
minPair(const std::pair<int32_t, MSize> &lhs,
        const std::pair<int32_t, MSize> &rhs) {
  return (lhs.second.size() < rhs.second.size() && lhs.second.size() != 0)
             ? lhs
             : rhs;
}
static inline std::pair<int32_t, MSize>
minIdPair(const std::pair<int32_t, MSize> &lhs,
          const std::pair<int32_t, MSize> &rhs) {
  return (lhs.first < rhs.first && lhs.first != -1)
             ? lhs
             : rhs;
}
static inline std::pair<int32_t, MSize>
guessPreviewPair(const std::pair<int32_t, MSize> &lhs,
                 const std::pair<int32_t, MSize> &rhs) {
  return (rhs.second.h < 720 ||
         (lhs.second.size() < rhs.second.size() && lhs.second.size() != 0) ||
         (lhs.second.size() == rhs.second.size() && lhs.first < rhs.first)) ?
         lhs :
         rhs;
}
static inline std::pair<int32_t, MSize> toPair(const Stream &stream) {
  return std::make_pair(stream.id, MSize(stream.width, stream.height));
}

auto NSCam::v3::AppStreamMgr::CamDejitter::create(
    std::shared_ptr<CommonInfo> pCommonInfo)
    -> std::shared_ptr<AppStreamMgr::CamDejitter> {
  switch (::property_get_int32("vendor.debug.camera.hal.dejitter.enable", 2)) {
  case 1:
    // return std::make_shared<CameraDisplayHint>(pCommonInfo);
    // break;
  case 2:
    return std::make_shared<CamDejitterImp>(pCommonInfo);
    break;
  }
  return nullptr;
}

/**
 * Kalman Filter parameters
 * kfQk [float] covariance of the process noise
 * kfRk [float] covariance of the observation noise
 */
const float kfQk = 15000.0f;
const float kfRk = 20000000.0f;
// CamDeJitter parameters
/**
 * CamDeJitter parameters
 * gInitialLatency    [int64_t]  Initial Latency for the filter = 1ms
 * gResetThreashold   [int64_t]  SOF timestamp gap > 200ms, reset filter
 * gLatencyThreashold [int64_t]  Avg. Latency > 2000ms, SOF timestamp abnormal
 * gMaxCushionTime    [int64_t]  Max cushion time 33.5ms for Dejitter
 */
const int64_t gInitialLatency = 1000000;
const int64_t gResetThreashold = 200000000;
const int64_t gLatencyThreashold = 2000000000;
const int64_t gMaxCushionTime = 33500000;

/**
 * c'tor
 */
ThisNamespace::CamDejitterImp(std::shared_ptr<CommonInfo> pCommonInfo)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) + "-CamDejitter"}
    , mCommonInfo(pCommonInfo) {
  resetFilter();
  mDebugLogOn =
      ::property_get_int32("vendor.debug.camera.hal.dejitter.log", 0) != 0;
}

/**
 * d'tor
 */
ThisNamespace::~CamDejitterImp() {}

/**
 * [ThisNamespace::storeSofTimestamp description]
 * @param  framenumber [description]
 * @param  timestamp   [description]
 */
auto ThisNamespace::storeSofTimestamp(uint32_t framenumber, int64_t timestamp)
    -> void {
  if (mEnable) {
    std::lock_guard<std::mutex> lock(mTimestampMapLock);
    mSOFMap.insert(std::make_pair(framenumber, timestamp));
    if (mSOFMap.size() >= 30) {
      MY_LOGE("[CamDej] shutdown CamDejitter due to wrong stream(%d)",
              mDejitterStreamId);
      mSOFMap.clear();
      mEnable = false;
    }
  }
}

/**
 * [ThisNamespace::scanSrcQueue description]
 * @param  srcQueue [description]
 * @return          [description]
 */
auto ThisNamespace::scanSrcQueue(HidlCallbackQueue& srcQueue)
    -> void {
  mTargetTime = 0;
  mTargetFramenumber = 0;
  if (!mEnable || srcQueue.empty())
    return;

  bool hasNonTargetParcel = false;
  bool flushQueue = false;
  int64_t now = ::systemTime();
  for (auto&& parcel : srcQueue) {
    if (parcel.mDejitterInfo.hasDisplayBuf) {
      if (parcel.mDejitterInfo.targetTime == 0) {
        // find out the sof timestamp
        int64_t sofTimestamp = 0;
        {
          std::lock_guard<std::mutex> lock(mTimestampMapLock);
          if (CC_UNLIKELY(
                mSOFMap.count(parcel.mDejitterInfo.frameNumber) != 1)) {
            MY_LOGW("[CamDej] frameNumber %u missing timestamp",
              parcel.mDejitterInfo.frameNumber);
            flushQueue = true;
            break;
          }
          sofTimestamp = mSOFMap[parcel.mDejitterInfo.frameNumber];
        }
        if (CC_UNLIKELY(sofTimestamp > now)) {
          MY_LOGE("[CamDej] timestamp abnormal(%u/%" PRId64 "/%" PRId64 ")",
            parcel.mDejitterInfo.frameNumber, sofTimestamp, now);
          parcel.mDejitterInfo.targetTime = now;
          continue;
        }

        // sof gap too large or sof inverse, need to reset the filter
        if (CC_UNLIKELY(sofTimestamp - mLastSOF > gResetThreashold ||
                        sofTimestamp < mLastSOF)) {
          resetFilter();
          MY_LOGI("[CamDej] reset filter(%u/%" PRId64 "/%" PRId64 ")",
            parcel.mDejitterInfo.frameNumber, sofTimestamp, mLastSOF);
        }
        mLastSOF = sofTimestamp;

        // calculate the average latency and target time
        int64_t avgLatency = kalmanFilter(now - sofTimestamp);
        if (CC_UNLIKELY(avgLatency > gLatencyThreashold)) {
          MY_LOGE("[CamDej] latency abnormal("
            "%u/%" PRId64 "/%" PRId64 "/%" PRId64 ")",
            parcel.mDejitterInfo.frameNumber, sofTimestamp, now, avgLatency);
          parcel.mDejitterInfo.targetTime = now;
        } else {
          parcel.mDejitterInfo.targetTime =
            sofTimestamp + avgLatency + mCushionTime;
        }

        MY_LOGI_IF(mDebugLogOn,
                   "[CamDej] F:%u S:%" PRId64 " N:%" PRId64 " L:%" PRId64 "",
                   parcel.mDejitterInfo.frameNumber,
                   sofTimestamp, now, avgLatency);
      }
    } else {
      hasNonTargetParcel = true;
    }
  }

  // need to flush srcQueue ASAP
  if (CC_UNLIKELY(flushQueue)) {
    std::lock_guard<std::mutex> lock(mTimestampMapLock);
    std::ostringstream log;
    for (auto&& parcel : srcQueue) {
      if (parcel.mDejitterInfo.hasDisplayBuf) {
        parcel.mDejitterInfo.hasDisplayBuf = false;
        parcel.mDejitterInfo.targetTime = 0;
        if (mSOFMap.count(parcel.mDejitterInfo.frameNumber)) {
          mSOFMap.erase(parcel.mDejitterInfo.frameNumber);
        }
        log << parcel.mDejitterInfo.frameNumber << '/';
      }
    }
    MY_LOGW("[CamDej] flush (%s)", log.str().c_str());
    hasNonTargetParcel = true;
  }

  // determin the defer time for first target parcel
  if (!hasNonTargetParcel) {
    mTargetTime = srcQueue.front().mDejitterInfo.targetTime;
    mTargetFramenumber = srcQueue.front().mDejitterInfo.frameNumber;
  }

  {
    std::lock_guard<std::mutex> lock(mWakeupLock);
    mRefresh = false;
  }
}

/**
 * [ThisNamespace::splice description]
 * priority1: splice all of non-target parcels, and callback w/o delay
 * priority2: splice one target parcel, and calculate the delay time
 *            for this parcel
 * @param  dstQueue [description]
 * @param  srcQueue [description]
 */
auto ThisNamespace::splice(HidlCallbackQueue &dstQueue, HidlCallbackQueue &srcQueue)
    -> void {
  if (srcQueue.empty())
    return;
  if (!mEnable) {
    dstQueue.splice(dstQueue.end(), srcQueue);
    return;
  }

  // splice all of non-target parcels to dstQueue
  auto it = srcQueue.begin();
  auto itNext = srcQueue.end();
  while (it != srcQueue.end()) {
    if (it->mDejitterInfo.hasDisplayBuf) {
      it++;
    } else {
      itNext = it; itNext++;
      dstQueue.splice(dstQueue.end(), srcQueue, it);
      it = itNext;
    }
  }

  // there are no non-target parcel, splice first target parcel
  if (dstQueue.empty()) {
    dstQueue.splice(dstQueue.end(), srcQueue, srcQueue.begin());
    if (CC_UNLIKELY(!dstQueue.front().mDejitterInfo.hasDisplayBuf)) {
      MY_LOGE("[CamDej] callback queue abnormal (%zu/%zu)",
        dstQueue.size(), srcQueue.size());
      return;
    }
    MY_LOGI_IF(mDebugLogOn,
               "[CamDej] F:%u callback N:%" PRId64 "",
               mTargetFramenumber, ::systemTime());
  } else {
    MY_LOGI_IF(mDebugLogOn,
               "[CamDej] callback parcels (%zu/%zu)"
               , dstQueue.size(), srcQueue.size());
  }
}

/**
 * [ThisNamespace::waitForDejitter description]
 * wait until mTargetTime(ns) for dejitter
 * @return          true: interrupted, need rescan srcQueue, otherwise false
 */
auto ThisNamespace::waitForDejitter() -> bool {
  bool needRefresh = false;
  CAM_ULOGM_DTAG_BEGIN(mDebugLogOn && mTargetTime != 0 && ATRACE_ENABLED(),
                       "CamDejitter %u", mTargetFramenumber);
  int32_t deferTime = mTargetTime > 0 ?
                      mTargetTime - ::systemTime() : 0;
  if (deferTime > 0 && mEnable) {
    std::unique_lock<std::mutex> lock(mWakeupLock);
    mWakeupCond.wait_for(lock, std::chrono::nanoseconds(deferTime),
                         [&](){return (mEnable == false) ||
                                      (mRefresh == true);});
    needRefresh = mRefresh;
  }
  CAM_ULOGM_DTAG_END();

  mTargetTime = 0;
  return mEnable && needRefresh;
}

auto ThisNamespace::wakeupAndRefresh()
    -> void {
  std::lock_guard<std::mutex> lock(mWakeupLock);
  mRefresh = true;
  mWakeupCond.notify_all();
}

/**
 * [ThisNamespace::postCallback description]
 * After target img callback, remove SOF timestamp from the map
 */
auto ThisNamespace::postCallback() -> void {
  std::lock_guard<std::mutex> lock(mTimestampMapLock);
  if (mSOFMap.count(mTargetFramenumber)) {
    mSOFMap.erase(mTargetFramenumber);
  }
  mTargetFramenumber = 0;
}

/**
 * [ThisNamespace::flush description]
 * while flush, should stop dejitter immediately
 */
auto ThisNamespace::flush() -> void {
  std::lock_guard<std::mutex> lock(mWakeupLock);
  mEnable = false;
  mWakeupCond.notify_all();
}

/**
 * [ThisNamespace::configure description]
 * @param  requestedConfiguration [description]
 * @return                        [description]
 */
auto ThisNamespace::configure(
    const StreamConfiguration &requestedConfiguration) -> bool {
  if (!tryActivateHalDejitter(requestedConfiguration)) {
    MY_LOGI("condition to turn on camera hal dejitter is not satisfied");
    return false;
  }
  MY_LOGI("CamDejitter is activated : %p", this);
  return true;
}

/**
 * [ThisNamespace::kalmanFilter description]
 * kalman filter is a time domain Low-Pass-Filter
 * @param  measuredLatency The orginal latency from SOF to now
 * @return                 The average latency
 */
inline auto ThisNamespace::kalmanFilter(int64_t measuredLatency) -> int64_t {
  mLastCovariance += kfQk;
  float kalmanGain = mLastCovariance / (mLastCovariance + kfRk);
  mLastCovariance *= (1.0f - kalmanGain);
  mLastEstimateLatency += static_cast<int64_t>(
      kalmanGain * static_cast<float>(measuredLatency - mLastEstimateLatency));

  return mLastEstimateLatency;
}

/**
 * [ThisNamespace::resetFilter description]
 */
inline auto ThisNamespace::resetFilter() -> void {
  mLastEstimateLatency = gInitialLatency;
  mLastCovariance = kfRk;
}

/**
 * [ThisNamespace::tryActivateHalDejitter description]
 * Check the conditions for CamDejitter
 * @param  rStreams [description]
 * @return bool     enable CamDejitter
 */
auto ThisNamespace::tryActivateHalDejitter(
    const StreamConfiguration &rStreams) -> bool {
  //
  // 1. check if video preview
  bool isHasVideoStream = false;
  for (auto &stream : rStreams.streams) {
    if (stream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
      isHasVideoStream = true;
      break;
    }
  }
  //
  // 2. check high frame rate
  bool isMtkHgihFrameRate = false;
  mCushionTime = 8300000;
  IMetadata metaSessionParams;
  if ( rStreams.sessionParams != nullptr &&
      ! mCommonInfo->mMetadataConverter->convert(rStreams.sessionParams, metaSessionParams) ) {
    MY_LOGE("Bad Session parameters");
    return false;
  }
  if (metaSessionParams.count() > 0) {
    // 2.1 turnkey FPS tag in sessionParam
    IMetadata::IEntry const &entryFps =
        metaSessionParams.entryFor(MTK_STREAMING_FEATURE_HFPS_MODE);
    MINT32 streamHfpsMode = MTK_STREAMING_FEATURE_HFPS_MODE_NORMAL;
    if (!entryFps.isEmpty()) {
      streamHfpsMode = entryFps.itemAt(0, NSCam::Type2Type<MINT32>());
      if (streamHfpsMode == MTK_STREAMING_FEATURE_HFPS_MODE_60FPS) {
        isMtkHgihFrameRate = true;
        mCushionTime = 8300000;
      }
    }
  } else {
    MY_LOGW("[CamDej] No Session parameters");
  }
  int64_t cushionTime =
          ::property_get_int32("vendor.debug.camera.hal.dejitter.cushion", -1);
  mCushionTime = cushionTime == -1 ? mCushionTime :
                 cushionTime > gMaxCushionTime ? gMaxCushionTime : cushionTime;

  // 3. search target (preview) stream
  //    the code here need align FeatureSettingPolicy, streaming part
  std::pair<int32_t, MSize> pairPrefDisp(-1, MSize(0, 0));
  std::pair<int32_t, MSize> pairDisp(-1, MSize(0, 0));
  std::pair<int32_t, MSize> pairExtra(-1, MSize(0, 0));
  auto streamSize = rStreams.streams.size();
  int32_t videoStreamId = -1;
  MY_LOGI_IF(mDebugLogOn && streamSize == 0, "[CamDej] No streams");
  uint32_t index = 0;
  for (auto &stream : rStreams.streams) {
    MY_LOGI_IF(mDebugLogOn,
      "[CamDej] dump streams(%u/%zu):%d usage:%" PRIX64 "(%s), (%ux%u)",
      ++index, streamSize, stream.id, stream.usage,
      (stream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) != 0 ? "ENC" :
      (stream.usage & GRALLOC_USAGE_HW_COMPOSER)      != 0 ? "HWC" :
      (stream.usage & GRALLOC_USAGE_HW_TEXTURE)       != 0 ? "GPU" :
                                                             "CPU",
       stream.width, stream.height);
    if (stream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
      videoStreamId = stream.id;
      continue;
    } else if (stream.usage & GRALLOC_USAGE_HW_COMPOSER) {
      pairPrefDisp = guessPreviewPair(pairPrefDisp, toPair(stream));
    } else if (stream.usage & GRALLOC_USAGE_HW_TEXTURE) {
      pairDisp = guessPreviewPair(pairDisp, toPair(stream));
    } else {
      pairExtra = guessPreviewPair(pairExtra, toPair(stream));
    }
  }
  mDejitterStreamId =
      pairPrefDisp.second.size()
          ? pairPrefDisp.first
          : pairDisp.second.size()
                ? pairDisp.first
                : pairExtra.second.size() ? pairExtra.first : -1;

  bool platformSupport = false;
#if SUPPORT_DEJITTER == 1
  platformSupport = true;
#endif

  mEnable = (mDejitterStreamId != -1) &&
            (platformSupport);

  {
    std::lock_guard<std::mutex> lock(mTimestampMapLock);
    mSOFMap.clear();
  }

  MY_LOGI_IF(mDebugLogOn,
             "[CamDej] Enable:%s(plat:%s) StreamId:%d VideoStream:%d "
             "MtkHighFrameRate:%s Cushion:%" PRId64 "",
             mEnable ? "T" : "F", platformSupport ? "T" : "F",
             mDejitterStreamId, videoStreamId,
             isMtkHgihFrameRate ? "T" : "F", mCushionTime);
  return mEnable;
}
