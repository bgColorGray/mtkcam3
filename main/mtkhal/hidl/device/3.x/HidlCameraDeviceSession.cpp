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

#include <main/mtkhal/hidl/device/3.x/HidlCameraDeviceSession.h>
#include "MyUtils.h"

#include <TypesWrapper.h>
#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef MTKCAM_HAL3PLUS_CUSTOMIZATION_SUPPORT
// Size of request metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_REQUEST_METADATA_QUEUE_SIZE = 0;
// Size of result metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE = 0;
#else
// Size of request metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_REQUEST_METADATA_QUEUE_SIZE = 2 << 20 /* 2MB */;
// Size of result metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE = 16 << 20 /* 16MB */;
#endif

using ::android::BAD_VALUE;
using ::android::Mutex;
using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;
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

// namespace {
//   std::map<int32_t, mcam::hidl::HidlCameraDeviceSession*> instanceMap;
// }

namespace mcam {
namespace hidl {
/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDeviceSession::~HidlCameraDeviceSession() {
  MY_LOGI("dtor");
  // auto res = instanceMap.erase(mInstanceId);
  // if (res != 1) {
  //   MY_LOGE("Not find instance:%d", mInstanceId);
  // }
}

/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDeviceSession::HidlCameraDeviceSession(const CreationInfo& info)
    : ICameraDeviceSession(),
      mSession(info.pSession),
      mCameraDeviceCallback(nullptr),
      mHidlCallback(info.pHidlCallback),
      mInstanceId(info.instanceId),
      mLogPrefix(std::to_string(mInstanceId) + "-hidl-session"),
      mBufferHandleCacheMgr(),
      mRequestMetadataQueue() {
  MY_LOGI("ctor");

  mConvertTimeDebug =
      property_get_int32("vendor.debug.camera.log.convertTime", 0);

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }
  ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::create(const CreationInfo& info)
    -> HidlCameraDeviceSession* {
  auto pInstance = new HidlCameraDeviceSession(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }
  // instanceMap[info.instanceId] = pInstance;
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::open(
    const ::android::sp<ICameraDeviceCallback>& callback)
    -> ::android::status_t {
  CAM_TRACE_CALL();
  // unlink to death notification for existed device callback
  if (mCameraDeviceCallback != nullptr) {
    mCameraDeviceCallback->unlinkToDeath(this);
    mCameraDeviceCallback = nullptr;
    ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));
  }

  // link to death notification for device callback
  if (callback != nullptr) {
    Return<bool> linked = callback->linkToDeath(this, (uint64_t)this);
    if (!linked.isOk()) {
      MY_LOGE("Transaction error in linking to mCameraDeviceCallback death: %s",
              linked.description().c_str());
    } else if (!linked) {
      MY_LOGW("Unable to link to mCameraDeviceCallback death notifications");
    }
    callback->getDebugInfo(
        [this](const auto& info) { mLinkToDeathDebugInfo = info; });
    MY_LOGD("Link death to ICameraDeviceCallback %s",
            toString(mLinkToDeathDebugInfo).c_str());
  }

  mCameraDeviceCallback = callback;
  MY_LOGI("mCameraDeviceCallback(%p) callback(%p)", mCameraDeviceCallback.get(),
          callback.get());
  auto status = initialize();
  if (CC_UNLIKELY(status != OK)) {
    MY_LOGE("HidlCameraDeviceSession initialization fail");
    return status;
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::initialize() -> ::android::status_t {
  CAM_TRACE_CALL();
  status_t status = OK;
  {
    MY_LOGI("FMQ request size : %zu, result size : %zu",
            CAMERA_REQUEST_METADATA_QUEUE_SIZE,
            CAMERA_RESULT_METADATA_QUEUE_SIZE);
    mRequestMetadataQueue = std::make_unique<RequestMetadataQueue>(
        CAMERA_REQUEST_METADATA_QUEUE_SIZE, false /* non blocking */);
    if (!mRequestMetadataQueue || !mRequestMetadataQueue->isValid()) {
      MY_LOGE("invalid request fmq");
      return BAD_VALUE;
    }
  }

  //--------------------------------------------------------------------------
  {
    Mutex::Autolock _l(mBufferHandleCacheMgrLock);

    if (mBufferHandleCacheMgr != nullptr) {
      MY_LOGE("mBufferHandleCacheMgr:%p != 0 while opening",
              mBufferHandleCacheMgr.get());
      mBufferHandleCacheMgr->destroy();
      mBufferHandleCacheMgr = nullptr;
    }
    mBufferHandleCacheMgr = IBufferHandleCacheMgr::create();
    if (mBufferHandleCacheMgr == nullptr) {
      MY_LOGE("IBufferHandleCacheMgr::create");
      return NO_INIT;
    }
  }

  // warning : IHal interface remove open()
  // status = mSession->open(createSessionCallbacks());

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::getSafeBufferHandleCacheMgr() const
    -> ::android::sp<IBufferHandleCacheMgr> {
  Mutex::Autolock _l(mBufferHandleCacheMgrLock);
  return mBufferHandleCacheMgr;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::constructDefaultRequestSettings(
    RequestTemplate type,
    constructDefaultRequestSettings_cb _hidl_cb) {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  CameraMetadata metadata;
  if (CC_UNLIKELY(mSession == nullptr)) {
    _hidl_cb(Status::INTERNAL_ERROR, metadata);
    return Void();
  }

  const camera_metadata* templateSettings;
  auto status = mSession->constructDefaultRequestSettings(
      static_cast<mcam::android::RequestTemplate>(type),
      templateSettings);
  if (CC_UNLIKELY(status != 0)) {
    _hidl_cb(Status::INTERNAL_ERROR, metadata);
    return Void();
  }

  // convert camera_metadata to CameraMetadata
  metadata.setToExternal(reinterpret_cast<uint8_t*>(
                             const_cast<camera_metadata*>(templateSettings)),
                         get_camera_metadata_size(templateSettings),
                         false /*shouldOwn*/);
  _hidl_cb(Status::OK, metadata);

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::configureStreams(
    const ::android::hardware::camera::device::V3_2::StreamConfiguration&
        requestedConfiguration,
    configureStreams_cb _hidl_cb) {
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::configureStreams_3_3(
    const ::android::hardware::camera::device::V3_2::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_3_cb _hidl_cb) {
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::configureStreams_3_4(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_4_cb _hidl_cb) {
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::configureStreams_3_5(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_5_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  WrappedHalStreamConfiguration halStreamConfiguration;
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    // ulog
    _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
  }

  mcam::android::StreamConfiguration input;
  mcam::android::HalStreamConfiguration output;

  // transfer from hidl to mtk
  convertStreamConfigurationFromHidl(requestedConfiguration, input);

  auto status = mSession->configureStreams(input, output);

  auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
  if (pBufferHandleCacheMgr == 0) {
    MY_LOGE("Bad BufferHandleCacheMgr");
    _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
  }
  pBufferHandleCacheMgr->configBufferCacheMap(requestedConfiguration);

  // transfer from mtk to hidl
  convertHalStreamConfigurationToHidl(output, halStreamConfiguration);
  _hidl_cb(mapToHidlCameraStatus(status), halStreamConfiguration);
  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
Return<void> HidlCameraDeviceSession::configureStreams_3_6(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_6_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  WrappedHalStreamConfiguration halStreamConfiguration;
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    // ulog
    _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
  }

  mcam::android::StreamConfiguration input;
  mcam::android::HalStreamConfiguration output;

  // transfer from hidl to mtk
  convertStreamConfigurationFromHidl(requestedConfiguration, input);

  auto status = mSession->configureStreams(input, output);

  auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
  if (pBufferHandleCacheMgr == 0) {
    MY_LOGE("Bad BufferHandleCacheMgr");
    _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
  }
  pBufferHandleCacheMgr->configBufferCacheMap(requestedConfiguration);

  // transfer from mtk to hidl
  convertHalStreamConfigurationToHidl(output, halStreamConfiguration);
  _hidl_cb(mapToHidlCameraStatus(status), halStreamConfiguration);
  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::getCaptureRequestMetadataQueue(
    getCaptureRequestMetadataQueue_cb _hidl_cb) {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  _hidl_cb(*mRequestMetadataQueue->getDesc());
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::getCaptureResultMetadataQueue(
    getCaptureResultMetadataQueue_cb _hidl_cb) {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  _hidl_cb(*(mHidlCallback->getCaptureResultMetadataQueueDesc()));
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::processCaptureRequest(
    const hidl_vec<::android::hardware::camera::device::V3_2::CaptureRequest>&
        requests,
    const hidl_vec<BufferCache>& cachesToRemove,
    processCaptureRequest_cb _hidl_cb) {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();
  uint32_t numRequestProcessed = 0;

  status_t status = OK;
  // halbufhandler->waithalbufcntavailable
  {
    auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
    if (pBufferHandleCacheMgr == 0) {
      MY_LOGE("Bad BufferHandleCacheMgr");
      _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
    }

    pBufferHandleCacheMgr->removeBufferCache(cachesToRemove);
    MY_LOGD("[debug] removeBufferCache done");

    int i = 0;
    hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>
        v34Requests(requests.size());
    for (auto& r : requests) {
      v34Requests[i++] =
          (::android::hardware::camera::device::V3_4::CaptureRequest&)
              WrappedCaptureRequest(r);
    }

    std::vector<RequestBufferCache> vBufferCache;
    pBufferHandleCacheMgr->registerBufferCache(v34Requests, vBufferCache);

    // pBufferHandleCacheMgr->dumpState();
    // MY_LOGD("[debug] registerBufferCache done, vBufferCache.size()=%u",
    // vBufferCache.size());

    if (CC_UNLIKELY(mSession == nullptr)) {
      MY_LOGE("Bad CameraDevice3Session");
      _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
    }

    // transfer from hidl to mtk structure

    std::vector<mcam::android::CaptureRequest> captureRequests;
    auto result =
        convertAllRequestFromHidl(v34Requests, mRequestMetadataQueue.get(),
                                  captureRequests, &vBufferCache);

    status = static_cast<status_t>(
        mSession->processCaptureRequest(captureRequests, numRequestProcessed));

    if (CC_UNLIKELY(result != OK)) {
      MY_LOGE("status: %d ,numRequestProcessed: %u", status,
              numRequestProcessed);
      MY_LOGE("======== dump all requests ========");
      for (auto& request : requests) {
        MY_LOGE("%s", toString(request).c_str());
      }
    }
  }

  _hidl_cb(mapToHidlCameraStatus(status), numRequestProcessed);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::processCaptureRequest_3_4(
    const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
        requests,
    const hidl_vec<BufferCache>& cachesToRemove,
    processCaptureRequest_3_4_cb _hidl_cb) {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();
  uint32_t numRequestProcessed = 0;

  status_t status = OK;
  // halbufhandler->waithalbufcntavailable
  {
    auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
    if (pBufferHandleCacheMgr == 0) {
      MY_LOGE("Bad BufferHandleCacheMgr");
      _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
    }

    pBufferHandleCacheMgr->removeBufferCache(cachesToRemove);
    MY_LOGD("[debug] removeBufferCache done");

    std::vector<RequestBufferCache> vBufferCache;
    pBufferHandleCacheMgr->registerBufferCache(requests, vBufferCache);

    // pBufferHandleCacheMgr->dumpState();
    // MY_LOGD("[debug] registerBufferCache done, vBufferCache.size()=%u",
    // vBufferCache.size());

    if (CC_UNLIKELY(mSession == nullptr)) {
      MY_LOGE("Bad CameraDevice3Session");
      _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
    }
    // transfer from hidl to mtk structure
    std::vector<mcam::android::CaptureRequest> captureRequests;
    auto result = convertAllRequestFromHidl(
        requests, mRequestMetadataQueue.get(), captureRequests, &vBufferCache);

    status = static_cast<status_t>(
        mSession->processCaptureRequest(captureRequests, numRequestProcessed));

    if (CC_UNLIKELY(result != OK)) {
      MY_LOGE("status: %d ,numRequestProcessed: %u", status,
              numRequestProcessed);
      MY_LOGE("======== dump all requests ========");
      for (auto& request : requests) {
        MY_LOGE("%s", toString(request).c_str());
      }
    }
  }

  _hidl_cb(mapToHidlCameraStatus(status), numRequestProcessed);
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.5
Return<void> HidlCameraDeviceSession::signalStreamFlush(
    const hidl_vec<int32_t>& streamIds,
    uint32_t streamConfigCounter) {
  CAM_TRACE_CALL();
  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");
  std::vector<int32_t> vStreamIds;
  for (auto& id : streamIds) {
    vStreamIds.push_back(id);
  }
  mSession->signalStreamFlush(vStreamIds, streamConfigCounter);

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::isReconfigurationRequired(
    const hidl_vec<uint8_t>& oldSessionParams,
    const hidl_vec<uint8_t>& newSessionParams,
    isReconfigurationRequired_cb _hidl_cb) {
  CAM_TRACE_CALL();
  const camera_metadata* oldMetadata;
  const camera_metadata* newMetadata;
  oldMetadata =
      reinterpret_cast<const camera_metadata_t*>(oldSessionParams.data());
  newMetadata =
      reinterpret_cast<const camera_metadata_t*>(newSessionParams.data());

  bool needReconfig = true;
  auto res = mSession->isReconfigurationRequired(oldMetadata, newMetadata,
                                                 needReconfig);

  _hidl_cb(mapToHidlCameraStatus(res), needReconfig);

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.6
Return<void> HidlCameraDeviceSession::switchToOffline(
    const hidl_vec<int32_t>& streamsToKeep,
    switchToOffline_cb _hidl_cb) {
  CAM_TRACE_CALL();
  MY_LOGW("Not implement, return ILLEGAL_ARGUMENT.");

  ::android::hardware::camera::device::V3_6::CameraOfflineSessionInfo info;
  ::android::sp<
      ::android::hardware::camera::device::V3_6::ICameraOfflineSession>
      session;

  // VTS will check halStreamConfig.streams[0].supportOffline
  // if it's not supported, offline session and info will not be accessed.
  _hidl_cb(Status::ILLEGAL_ARGUMENT, info, session);

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status> HidlCameraDeviceSession::flush() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return Status::INTERNAL_ERROR;
  }

  auto status = mSession->flush();

  // clear request metadata queue
  if (mRequestMetadataQueue->availableToRead()) {
    CameraMetadata dummySettingsFmq;
    dummySettingsFmq.resize(mRequestMetadataQueue->availableToRead());
    bool read = mRequestMetadataQueue->read(
        dummySettingsFmq.data(), mRequestMetadataQueue->availableToRead());
    if (!read) {
      MY_LOGW("cannot clear request fmq during flush!");
    }
  }

  return mapToHidlCameraStatus(status);
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> HidlCameraDeviceSession::close() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  // unlink to death notification for existed device callback
  if (mCameraDeviceCallback != nullptr) {
    mCameraDeviceCallback->unlinkToDeath(this);
    mCameraDeviceCallback = nullptr;
  }
  ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto status = mSession->close();
  if (CC_UNLIKELY(status != OK)) {
    MY_LOGE("HidlCameraDeviceSession close fail");
  }
  mSession = nullptr;

  //
  {
    Mutex::Autolock _l(mBufferHandleCacheMgrLock);
    if (mBufferHandleCacheMgr != nullptr) {
      mBufferHandleCacheMgr->destroy();
      mBufferHandleCacheMgr = nullptr;
    }
  }
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
void HidlCameraDeviceSession::serviceDied(
    uint64_t cookie __unused,
    const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused) {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  if (cookie != (uint64_t)this) {
    MY_LOGW("Unexpected ICameraDevice3Session serviceDied cookie 0x%" PRIx64
            ", expected %p",
            cookie, this);
  }

  MY_LOGW("ICameraDevice3Session has died - %s; removing it - cookie:0x%" PRIx64
          " this: %p",
          toString(mLinkToDeathDebugInfo).c_str(), cookie, this);

  // force to close camera
  MY_LOGW("CameraService has died, force to close camera device %d",
          mInstanceId);

  flush();
  close();
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::convertStreamConfigurationFromHidl(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        srcStreams,
    mcam::android::StreamConfiguration& dstStreams)
    -> ::android::status_t {
  CAM_TRACE_CALL();
  for (auto& stream : srcStreams.v3_4.streams) {
    MY_LOGD("%s", toString(stream).c_str());
  }
  MY_LOGD("operationMode=%u", srcStreams.v3_4.operationMode);

  int size = srcStreams.v3_4.streams.size();
  dstStreams.streams.resize(size);
  for (int i = 0; i < size; i++) {
    auto& srcStream = srcStreams.v3_4.streams[i];
    auto& dstStream = dstStreams.streams[i];
    //
    dstStream.id = srcStream.v3_2.id;
    dstStream.streamType =
        static_cast<mcam::android::StreamType>(srcStream.v3_2.streamType);
    dstStream.width = srcStream.v3_2.width;
    dstStream.height = srcStream.v3_2.height;
    dstStream.format =
        static_cast<android_pixel_format_t>(srcStream.v3_2.format);
    dstStream.usage = srcStream.v3_2.usage;
    dstStream.dataSpace =
        static_cast<android_dataspace_t>(srcStream.v3_2.dataSpace);
    dstStream.rotation = static_cast<mcam::android::StreamRotation>(
        srcStream.v3_2.rotation);
    //
    if (srcStream.physicalCameraId == "") {
      dstStream.physicalCameraId = -1;
    } else {
      dstStream.physicalCameraId = std::stoi(srcStream.physicalCameraId);
    }
    dstStream.bufferSize = srcStream.bufferSize;
  }

  dstStreams.operationMode =
      static_cast<mcam::android::StreamConfigurationMode>(
          srcStreams.v3_4.operationMode);
  dstStreams.sessionParams = reinterpret_cast<const camera_metadata_t*>(
      srcStreams.v3_4.sessionParams.data());
  dstStreams.streamConfigCounter =
      srcStreams
          .streamConfigCounter;  // ::android::hardware::camera::device::V3_5

  for (auto& stream : dstStreams.streams) {
    MY_LOGD("%s", toString(stream).c_str());
  }
  MY_LOGD("operationMode=%u", dstStreams.operationMode);
  if (dstStreams.sessionParams != nullptr) {
    MY_LOGD("sessionParams.size=%zu entryCount=%zu",
            get_camera_metadata_size(dstStreams.sessionParams),
            get_camera_metadata_entry_count(dstStreams.sessionParams));
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::convertHalStreamConfigurationToHidl(
    const mcam::android::HalStreamConfiguration& srcStreams,
    ::android::hardware::camera::device::V3_6::HalStreamConfiguration&
        dstStreams) -> ::android::status_t {
  CAM_TRACE_CALL();

  int size = srcStreams.streams.size();
  dstStreams.streams.resize(size);

  for (int i = 0; i < size; i++) {
    auto& srcStream = srcStreams.streams[i];
    auto& dstStream = dstStreams.streams[i];
    //
    dstStream.v3_4.v3_3.v3_2.id = srcStream.id;
    dstStream.v3_4.v3_3.v3_2.overrideFormat =
        static_cast<::android::hardware::graphics::common::V1_0::PixelFormat>(
            srcStream.overrideFormat);
    dstStream.v3_4.v3_3.v3_2.producerUsage = srcStream.producerUsage;
    dstStream.v3_4.v3_3.v3_2.consumerUsage = srcStream.consumerUsage;
    dstStream.v3_4.v3_3.v3_2.maxBuffers = srcStream.maxBuffers;
    dstStream.v3_4.v3_3.overrideDataSpace = srcStream.overrideDataSpace;
    if (srcStream.physicalCameraId == -1) {
      dstStream.v3_4.physicalCameraId = "";
    } else {
      dstStream.v3_4.physicalCameraId =
          std::to_string(srcStream.physicalCameraId);
    }
    dstStream.supportOffline = srcStream.supportOffline;
    MY_LOGD("HalStream.id(%d) HalStream.maxBuffers(%u)",
            dstStream.v3_4.v3_3.v3_2.id, dstStream.v3_4.v3_3.v3_2.maxBuffers);
    MY_LOGD("srcHalStream.id(%d) srcHalStream.maxBuffers(%u)", srcStream.id,
            srcStream.maxBuffers);
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::convertAllRequestFromHidl(
    const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
        inRequests,
    RequestMetadataQueue* pRequestFMQ,
    std::vector<mcam::android::CaptureRequest>& outRequests,
    std::vector<RequestBufferCache>* requestBufferCache)
    -> ::android::status_t {
  CAM_TRACE_CALL();

  status_t result = OK;

  for (auto& in : inRequests) {
    mcam::android::CaptureRequest out = {};
    RequestBufferCache* currentBufferCache = {};

    for (auto& bufferCache : *requestBufferCache) {
      if (bufferCache.frameNumber == in.v3_2.frameNumber) {
        currentBufferCache = &bufferCache;
        break;
      }
    }
    if (CC_LIKELY(OK == convertToHalCaptureRequest(in, pRequestFMQ, &out,
                                                   currentBufferCache))) {
      outRequests.push_back(std::move(out));
    } else {
      MY_LOGE("convert to Hal request failed, dump reqeust info : %s",
              toString(in).c_str());
      result = BAD_VALUE;
    }
  }

  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::convertToHalCaptureRequest(
    const ::android::hardware::camera::device::V3_4::CaptureRequest&
        hidl_request,
    RequestMetadataQueue* request_metadata_queue,
    mcam::android::CaptureRequest* hal_request,
    RequestBufferCache* requestBufferCache) -> ::android::status_t {
  CAM_TRACE_CALL();

  if (hal_request == nullptr) {
    MY_LOGE("hal_request is nullptr");
    return BAD_VALUE;
  }
  hal_request->frameNumber = hidl_request.v3_2.frameNumber;

  // 1. ConvertToHalMetadata: logical metadata
  status_t res = convertToHalMetadata(
      hidl_request.v3_2.fmqSettingsSize, request_metadata_queue,
      hidl_request.v3_2.settings,
      const_cast<camera_metadata**>(&(hal_request->settings)));
  if (res != OK) {
    MY_LOGE("Converting metadata failed: %s(%d)", strerror(-res), res);
    return res;
  }

  // 2. Convert inputBuffers from HIDL
  if (hidl_request.v3_2.inputBuffer.buffer != nullptr) {
    mcam::android::StreamBuffer hal_inputBuffer = {};
    const BufferHandle* importedBufferHandle =
        &(requestBufferCache->bufferHandleMap.valueFor(
            hidl_request.v3_2.inputBuffer.streamId));
    res = convertToHalStreamBuffer(hidl_request.v3_2.inputBuffer,
                                   &hal_inputBuffer, importedBufferHandle);
    if (res != OK) {
      MY_LOGE("Converting hal stream buffer failed: %s(%d)", strerror(-res),
              res);
      return res;
    }
    hal_request->inputBuffer = hal_inputBuffer;
  }

  // 3. Convert outputBuffers from HIDL
  for (auto& buffer : hidl_request.v3_2.outputBuffers) {
    mcam::android::StreamBuffer hal_outputBuffer = {};
    const BufferHandle* importedBufferHandle =
        &(requestBufferCache->bufferHandleMap.valueFor(buffer.streamId));
    status_t res = convertToHalStreamBuffer(buffer, &hal_outputBuffer,
                                            importedBufferHandle);
    if (res != OK) {
      MY_LOGE("Converting hal stream buffer failed: %s(%d)", strerror(-res),
              res);
      return res;
    }

    hal_request->outputBuffers.push_back(std::move(hal_outputBuffer));
  }

  // 4. ConvertToHalMetadata: physical metadata
  for (auto& hidl_physical_settings : hidl_request.physicalCameraSettings) {
    mcam::android::PhysicalCameraSetting hal_PhysicalCameraSetting;

    if (hidl_physical_settings.physicalCameraId == "") {
      hal_PhysicalCameraSetting.physicalCameraId = -1;
    } else {
      hal_PhysicalCameraSetting.physicalCameraId =
          std::stoi(hidl_physical_settings.physicalCameraId);
    }

    res = convertToHalMetadata(
        hidl_physical_settings.fmqSettingsSize, request_metadata_queue,
        hidl_physical_settings.settings,
        const_cast<camera_metadata**>(&(hal_PhysicalCameraSetting.settings)));
    if (res != OK) {
      MY_LOGE("Converting to HAL physical metadata failed: %s(%d)",
              strerror(-res), res);
      return res;
    }
    hal_request->physicalCameraSettings.push_back(
        std::move(hal_PhysicalCameraSetting));
  }

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::convertToHalMetadata(
    uint32_t message_queue_setting_size,
    RequestMetadataQueue* request_metadata_queue,
    const CameraMetadata& request_settings,
    camera_metadata** hal_metadata) -> ::android::status_t {
  CAM_TRACE_CALL();

  if (hal_metadata == nullptr) {
    MY_LOGE("hal_metadata is nullptr.");
    return BAD_VALUE;
  }

  const camera_metadata_t* metadata = nullptr;
  CameraMetadata metadata_queue_settings;

  if (message_queue_setting_size == 0) {
    // Use the settings in the request.
    if (request_settings.size() != 0) {
      metadata =
          reinterpret_cast<const camera_metadata_t*>(request_settings.data());
    }
  } else {
    // Read the settings from request metadata queue.if (request_metadata_queue
    // == nullptr)
    if (request_metadata_queue == nullptr) {
      MY_LOGE("request_metadata_queue is nullptr");
      return BAD_VALUE;
    }

    metadata_queue_settings.resize(message_queue_setting_size);
    bool success = request_metadata_queue->read(metadata_queue_settings.data(),
                                                message_queue_setting_size);
    if (!success) {
      MY_LOGE("Failed to read from request metadata queue.");
      return BAD_VALUE;
    }

    metadata = reinterpret_cast<const camera_metadata_t*>(
        metadata_queue_settings.data());
  }

  if (metadata == nullptr) {
    *hal_metadata = NULL;
    return OK;
  }

  *hal_metadata = clone_camera_metadata(metadata);

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto HidlCameraDeviceSession::convertToHalStreamBuffer(
    const ::android::hardware::camera::device::V3_2::StreamBuffer& hidl_buffer,
    mcam::android::StreamBuffer* hal_buffer,
    const BufferHandle* importedBufferHandle) -> ::android::status_t {
  CAM_TRACE_CALL();

  auto duplicateFenceFD = [=](buffer_handle_t handle, int& fd) {
    if (handle == nullptr || handle->numFds == 0) {
      fd = -1;
      return true;
    }
    if (handle->numFds != 1) {
      MY_LOGE("invalid fence handle with %d file descriptors", handle->numFds);
      fd = -1;
      return false;
    }
    if (handle->data[0] < 0) {
      fd = -1;
      return true;
    }
    fd = ::dup(handle->data[0]);
    if (fd < 0) {
      MY_LOGE("failed to dup fence fd %d", handle->data[0]);
      return false;
    }
    return true;
  };

  if (hal_buffer == nullptr) {
    MY_LOGE("hal_buffer is nullptr.");
    return BAD_VALUE;
  }

  hal_buffer->streamId = hidl_buffer.streamId;
  hal_buffer->bufferId = hidl_buffer.bufferId;
  hal_buffer->buffer = importedBufferHandle->bufferHandle;
  hal_buffer->appBufferHandleHolder =
      importedBufferHandle->appBufferHandleHolder;
  hal_buffer->status =
      static_cast<mcam::android::BufferStatus>(hidl_buffer.status);
  if (hidl_buffer.acquireFence.getNativeHandle() != nullptr) {
    duplicateFenceFD(hidl_buffer.acquireFence.getNativeHandle(),
                     hal_buffer->acquireFenceFd);
  }
  hal_buffer->releaseFenceFd = -1;

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
// auto HidlCameraDeviceSession::getConfigurationResults(
//     const uint32_t streamConfigCounter,  // input
//     getConfigurationResults_cb _ext_cb)   // output
//     -> void {
//   mcam::android::ExtConfigurationResults ConfigurationResults;
//   int pResult = mSession->getConfigurationResults(streamConfigCounter,
//                                                   ConfigurationResults);
//   camera_metadata* results = nullptr;
//   std::map<int32_t, camera_metadata*> streamResults;

//   if (pResult == OK) {
//     for (auto& result : ConfigurationResults.streamResults) {
//       // MY_LOGD("stream id: %d",result.first);
//       // for (auto i = 0; i < result.second.size(); i+=4)
//       //   MY_LOGD("plan id: %d, w: %d, h: %d, stride: %d",result.second[i],
//       //   result.second[i+1], result.second[i+2], result.second[i+3]);

//       camera_metadata* planInfo =
//           allocate_camera_metadata(1, result.second.size() * 8 * 4);
//       auto res = add_camera_metadata_entry(
//           planInfo, MTK_HAL3PLUS_STREAM_ALLOCATION_INFOS,
//           result.second.data(), result.second.size());
//       streamResults[result.first] = planInfo;
//     }

//     // for (auto &stream :streamResults)
//     // {
//     //   MY_LOGD("stream id: %d",stream.first);
//     //   camera_metadata_ro_entry entry;
//     //   auto res = find_camera_metadata_ro_entry(stream.second,
//     //   MTK_HAL3PLUS_STREAM_ALLOCATION_INFOS, &entry); MY_LOGD("entry count
//     //   :%d", entry.count); if ((0 != res)) {
//     //     MY_LOGD("GG .....");
//     //   }
//     //   MY_LOGD("plan id: %d, w: %d, h: %d, stride: %d",entry.data.i64[0],
//     //   entry.data.i64[1], entry.data.i64[2], entry.data.i64[3]);
//     // }
//   }

//   _ext_cb(pResult, results, streamResults);

//   for (auto& pStream : streamResults) {
//     free_camera_metadata(pStream.second);
//   }
// }

/******************************************************************************
 *
 ******************************************************************************/
// extern "C" {
// NSCam::IExtCameraDeviceSession* CreateExtCameraDeviceSessionInstance(
//     int32_t instanceId) {
//   auto iter = instanceMap.find(instanceId);
//   if (iter != instanceMap.end()) {
//     ALOGD("%s : find instance:%d from instanceMap", __FUNCTION__, instanceId);
//     return iter->second;
//   } else {
//     ALOGE("%s : Not find instance:%d from instanceMap", __FUNCTION__,
//           instanceId);
//     return nullptr;
//   }
// }
// }

};  // namespace hidl
};  // namespace mcam
