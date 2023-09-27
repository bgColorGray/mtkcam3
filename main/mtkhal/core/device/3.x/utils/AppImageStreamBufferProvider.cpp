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

#include "AppImageStreamBufferProvider.h"

#include <mtkcam/utils/std/ULog.h>  // will include <log/log.h> if LOG_TAG was defined

#include <cutils/properties.h>
#include <sync/sync.h>
#include <map>
#include <memory>
#include <mutex>
//
#include "MyUtils.h"
//
using ::android::OK;
using mcam::core::Utils::AppImageStreamBuffer;
using mcam::core::Utils::AppImageStreamBufferProvider;
using mcam::core::Utils::IHalBufHandler;

#define ThisNamespace AppImageStreamBufferProviderImpl

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_DEBUG(level, fmt, arg...)                    \
  do {                                                  \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
  } while (0)

#define MY_WARN(level, fmt, arg...)                     \
  do {                                                  \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
  } while (0)

#define MY_ERROR_ULOG(level, fmt, arg...)               \
  do {                                                  \
    CAM_ULOGM##level("[%s] " fmt, __FUNCTION__, ##arg); \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                  \
  do {                                                \
    CAM_LOG##level("[%s] " fmt, __FUNCTION__, ##arg); \
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
namespace Utils {

class AppImageStreamBufferProviderImpl : public AppImageStreamBufferProvider {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // 1st layer map:
  //      key - requestNumber value
  //      value - 2nd layer map
  //                  key - streamId
  //                  value - ImageStreamBuffer sp
  typedef std::map<uint32_t,
                   std::map<StreamId_T, android::sp<AppImageStreamBuffer>>>
      ImageStreamBufferMap;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  explicit AppImageStreamBufferProviderImpl(
      android::sp<IHalBufHandler> pHalBufHandler);
  virtual ~AppImageStreamBufferProviderImpl() = default;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  virtual auto destroy() -> void;

  virtual auto storeStreamBuffers(
      const uint32_t requestNumber,
      const android::KeyedVector<StreamId_T,
                                 android::sp<AppImageStreamBuffer>>&
          vInputImageBuffers,
      const android::KeyedVector<StreamId_T,
                                 android::sp<AppImageStreamBuffer>>&
          vOutputImageBuffers)
      // const std::map<int32_t, android::sp<IImageStreamBuffer>>& buffers
      -> int;

  virtual auto waitUntilDrained(nsecs_t const timeout) -> int;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IImageStreamBufferProvider Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  virtual auto requestStreamBuffer(requestStreamBuffer_cb cb,
                                   RequestStreamBuffer const& in) -> int;

 private:
  virtual auto dumpL() const -> void;
  virtual auto dumpL(
      const std::map<StreamId_T, android::sp<AppImageStreamBuffer>>&
          buffers) const -> void;

 private:
  android::sp<IHalBufHandler> mHalBufHandler = nullptr;

  std::mutex mMutex;
  ImageStreamBufferMap mMap;

  int32_t mLogLevel;
  int32_t mHandleAcquireFence = 0;
};

/******************************************************************************
 *
 ******************************************************************************/
auto AppImageStreamBufferProvider::create(
    android::sp<IHalBufHandler> pHalBufHandler)
    -> std::shared_ptr<AppImageStreamBufferProvider> {
  auto pInstance =
      std::make_shared<AppImageStreamBufferProviderImpl>(pHalBufHandler);
  if (pInstance == nullptr) {
    return nullptr;
  }
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::AppImageStreamBufferProviderImpl(
    android::sp<IHalBufHandler> pHalBufHandler)
    : mHalBufHandler(pHalBufHandler), mLogLevel() {
  mHandleAcquireFence = ::property_get_int32(
      "vendor.debug.camera.appimagesbpvdr.acquirefence", 1);

  int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
  if (loglevel == 0) {
    loglevel = ::property_get_int32("vendor.debug.camera.log.AppStreamMgr", 0);
  }

  mLogLevel = ::property_get_int32("vendor.debug.camera.log", 0);
  if (mLogLevel == 0) {
    mLogLevel = ::property_get_int32(
        "vendor.debug.camera.log.deviceutils.appimagesbpvdr", 0);
  }

  MY_LOGD("HalBufHandler(%p) handle_acquire_fence(%d)", pHalBufHandler.get(),
          mHandleAcquireFence);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  MY_LOGW("might need to handle cached streambuffer");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::storeStreamBuffers(
    const uint32_t requestNumber,
    const android::KeyedVector<StreamId_T,
                               android::sp<AppImageStreamBuffer>>&
        vInputImageBuffers,
    const android::KeyedVector<StreamId_T,
                               android::sp<AppImageStreamBuffer>>&
        vOutputImageBuffers)
    // const std::map<int32_t, android::sp<IImageStreamBuffer>>& buffers
    -> int {
  // MY_LOGD("requestNumber(%u) store ImageStreamBuffer.size(%zu)",
  // requestNumber, buffers.size());
  MY_LOGD(
      "requestNumber(%u) store inputSBuffer.size(%zu) outputSBuffer.size(%zu)",
      requestNumber, vInputImageBuffers.size(), vOutputImageBuffers.size());
  {
    std::lock_guard<std::mutex> _l(mMutex);
    if (mMap.find(requestNumber) != mMap.end()) {
      MY_LOGE("should no be existed");
      dumpL(mMap[requestNumber]);
      return -1;
    }
    for (size_t i = 0; i < vInputImageBuffers.size(); ++i) {
      mMap[requestNumber][vInputImageBuffers.keyAt(i)] =
          vInputImageBuffers.valueAt(i);
    }
    for (size_t i = 0; i < vOutputImageBuffers.size(); ++i) {
      mMap[requestNumber][vOutputImageBuffers.keyAt(i)] =
          vOutputImageBuffers.valueAt(i);
    }
    // mMap[requestNumber] = buffers;
  }
  return 0;
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
  //
  int err = OK;
  std::lock_guard<std::mutex> _l(mMutex);

  err = mHalBufHandler->waitUntilDrained(timeoutToWait());

  if (OK != err) {
    MY_LOGW("timeout(ns):%" PRId64 " err:%d(%s)", timeout, -err,
            ::strerror(-err));
  }

  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::requestStreamBuffer(requestStreamBuffer_cb cb,
                                        RequestStreamBuffer const& in) -> int {
  auto waitAcquireFence = [&](auto& pStreamBuffer) -> int {
    int err = 0;

    MY_LOGD_IF(mLogLevel >= 1,
               "requestSB: request[%u] pStreamBuffer->getAcquireFence()=%d",
               in.requestNo, pStreamBuffer->getAcquireFence());
    if (pStreamBuffer->getAcquireFence() !=
        -1) {  // waiting acquire_fence before createImageStreamBuffer
      nsecs_t current = ::systemTime();
      //
      int acquire_fence = ::dup(pStreamBuffer->getAcquireFence());
      if (acquire_fence < 0) {
        MY_LOGE("request[%u] acquire_fence = %d < 0",
                in.requestNo, acquire_fence);
        return -1;
      }
      err = ::sync_wait(acquire_fence, 1000);  // wait fence for 1000 ms
      if (err < 0) {
        MY_LOGW("sync_wait() failed: %s", strerror(errno));
        ::close(acquire_fence);
        return err;
      }

      auto t = ((::systemTime() - current) / 1000000L);

      auto pStreamInfo = pStreamBuffer->getStreamInfo();
      const StreamId_T streamId =
          (pStreamInfo != nullptr) ? pStreamInfo->getStreamId() : -1;
      MY_LOGD_IF(mLogLevel >= 1 || t >= 10,
                 "requestSB: request[%u] StreamBuffer:%p streamId:(%" PRId64
                 ") bufferId:%" PRIu64
                 " "
                 "imageBufferHeap(%p), pre wait acquireFence %d, time %d ms",
                 in.requestNo, pStreamBuffer.get(), streamId,
                 pStreamBuffer->getBufferId(),
                 pStreamBuffer->getImageBufferHeap().get(), acquire_fence,
                 static_cast<int>(t));

      ::close(acquire_fence);
      acquire_fence = -1;
      pStreamBuffer->setAcquireFence(-1);
    }
    return err;
  };

  android::sp<AppImageStreamBuffer> pStreamBuffer = nullptr;
  int err = OK;
  if (mHalBufHandler) {
    std::lock_guard<std::mutex> _l(mMutex);
    if ( (err = mHalBufHandler->requestStreamBuffer(cb, in)) == 0 ) {
      const StreamId_T streamId = in.pStreamInfo->getStreamId();
      if (MY_UNLIKELY(mMap.find(in.requestNo) == mMap.end() ||
                      mMap[in.requestNo].find(streamId) ==
                          mMap[in.requestNo].end())) {
        MY_LOGD("not stored");
      } else {
        mMap[in.requestNo].erase(streamId);
        MY_LOGD("requestNumber(%u) streamId(%" PRId64 ") streamBuffer(%p)",
                in.requestNo, streamId, pStreamBuffer.get());
      }
    } else {
      MY_LOGE("should not happen");
      return -1;
    }
  } else {
    std::lock_guard<std::mutex> _l(mMutex);
    const StreamId_T streamId = in.pStreamInfo->getStreamId();
    if (MY_UNLIKELY(mMap.find(in.requestNo) == mMap.end() ||
                    mMap[in.requestNo].find(streamId) ==
                        mMap[in.requestNo].end())) {
      MY_LOGE("should not happen");
      return -1;
    }
    pStreamBuffer = mMap[in.requestNo][streamId];
    mMap[in.requestNo].erase(streamId);
    MY_LOGD("requestNumber(%u) streamId(%" PRId64 ") streamBuffer(%p)",
            in.requestNo, streamId, pStreamBuffer.get());
  }
  // handle acquire fence
  if (mHandleAcquireFence && (err = waitAcquireFence(pStreamBuffer)) != 0) {
    MY_LOGE("cannot wait acquire fence");
  }
  //cb(pStreamBuffer);
  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpL() const -> void {
  for (auto& it : mMap) {
    MY_LOGI("requestNumber(%u): ", it.first);
    dumpL(it.second);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpL(
    const std::map<StreamId_T, android::sp<AppImageStreamBuffer>>& buffers)
    const -> void {
  for (auto& it : buffers) {
    MY_LOGI("\tstreamId(%" PRId64 "), buffer(%p)", it.first, it.second.get());
  }
}
};  // namespace Utils
};  // namespace core
};  // namespace mcam
