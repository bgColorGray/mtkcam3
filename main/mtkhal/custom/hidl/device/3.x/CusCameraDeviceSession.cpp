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

#include "main/mtkhal/custom/hidl/device/3.x/CusCameraDeviceSession.h"

#include <TypesWrapper.h>
#include <cutils/properties.h>
// #include <mtkcam-core/feature/utils/p2/DebugTimer.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>

#include <dlfcn.h>
#include <utils/String8.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "MyUtils.h"
#include "dptz/DptzSessionFactory.h"
#include "feature/basic/BasicSession.h"
#include "feature/sample/SampleSession.h"
#include "lowLatency/LowLatencySessionFactory.h"
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

// Size of request metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_REQUEST_METADATA_QUEUE_SIZE = 2 << 20 /* 2MB */;
// Size of result metadata fast message queue. Change to 0 to always use
// hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE = 16 << 20 /* 16MB */;

using ::android::BAD_VALUE;
using ::android::Mutex;
using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;
using mcam::hidl::BufferHandle;
using mcam::hidl::IBufferHandleCacheMgr;
using mcam::hidl::RequestBufferCache;
using NSCam::custom_dev3::feature::FeatureSessionType;
using NSCam::custom_dev3::feature::IFeatureSession;
using NSCam::custom_dev3::feature::basic::BasicSession;
using NSCam::custom_dev3::feature::sample::SampleSession;
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

namespace NSCam {
namespace custom_dev3 {
/******************************************************************************
 *
 ******************************************************************************/
CusCameraDeviceSession::~CusCameraDeviceSession() {
  MY_LOGI("dtor");
  if (mStaticInfo)
    free_camera_metadata(mStaticInfo);
  if (mLibHandle != NULL) {
    dlclose(mLibHandle);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraDeviceSession::CusCameraDeviceSession(
    const ::android::sp<ICameraDeviceSession>& session,
    int32_t intanceId,
    camera_metadata* staticMetadata)
    : ICameraDeviceSession(),
      mSession(session),
      mCameraDeviceCallback(nullptr),
      mLogPrefix(std::to_string(intanceId) + "-custom-session"),
      mInstanceId(intanceId),
      mBufferHandleCacheMgr(),
      mRequestMetadataQueue(),
      mResultMetadataQueue() {
  MY_LOGI("ctor");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }
  mStaticInfo = clone_camera_metadata(staticMetadata);
  ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));

  {
    MY_LOGI("open hidl camera device session");
    mLibHandle = dlopen(camDevice3HidlLib, RTLD_NOW);
    if (mLibHandle == NULL) {
      MY_LOGE("dlopen fail: %s", dlerror());
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::create(
    const ::android::sp<ICameraDeviceSession>& session,
    int32_t intanceId,
    camera_metadata* staticMetadata) -> CusCameraDeviceSession* {
  auto pInstance =
      new CusCameraDeviceSession(session, intanceId, staticMetadata);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::open(
    const ::android::sp<ICameraDeviceCallback>& callback)
    -> ::android::status_t {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
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
    MY_LOGE("CusCameraDeviceSession initialization fail");
    return status;
  }

  if (mLibHandle != NULL) {
    CreateExtCameraDeviceSessionInstance_t*
        CreateExtCameraDeviceSessionInstance =
            reinterpret_cast<CreateExtCameraDeviceSessionInstance_t*>(
                dlsym(mLibHandle, "CreateExtCameraDeviceSessionInstance"));
    const char* dlsym_error = dlerror();
    if (!CreateExtCameraDeviceSessionInstance) {
      MY_LOGE("Cannot load symbol: %s", dlerror());
    } else {
      mExtCameraDeviceSession =
          CreateExtCameraDeviceSessionInstance(getInstanceId());
    }
  }
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::initialize() -> ::android::status_t {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
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
    mResultMetadataQueue = std::make_unique<ResultMetadataQueue>(
        CAMERA_RESULT_METADATA_QUEUE_SIZE, false /* non blocking */);
    if (!mResultMetadataQueue || !mResultMetadataQueue->isValid()) {
      MY_LOGE("invalid result fmq");
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

  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::getSafeBufferHandleCacheMgr() const
    -> ::android::sp<IBufferHandleCacheMgr> {
  Mutex::Autolock _l(mBufferHandleCacheMgrLock);
  return mBufferHandleCacheMgr;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::constructDefaultRequestSettings(
    RequestTemplate type,
    constructDefaultRequestSettings_cb _hidl_cb) {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  CameraMetadata metadata;
  if (CC_UNLIKELY(mSession == nullptr)) {
    _hidl_cb(Status::INTERNAL_ERROR, metadata);
    return Void();
  }
  mSession->constructDefaultRequestSettings(type, [&](auto status,
                                                      const auto& metadata) {
    MY_LOGD("constructDefaultRequestSettings returns status:%d", (int)status);
    _hidl_cb(status, metadata);
  });

  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::configureStreams(
    const ::android::hardware::camera::device::V3_2::StreamConfiguration&
        requestedConfiguration,
    configureStreams_cb _hidl_cb) {
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::configureStreams_3_3(
    const ::android::hardware::camera::device::V3_2::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_3_cb _hidl_cb) {
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::configureStreams_3_4(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_4_cb _hidl_cb) {
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::configureStreams_3_5(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_5_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  //
  auto ret = mSession->configureStreams_3_5(
      requestedConfiguration, [&](auto status, auto& HalStreamConfiguration) {
        _hidl_cb(status, HalStreamConfiguration);
      });

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
Return<void> CusCameraDeviceSession::configureStreams_3_6(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        requestedConfiguration,
    configureStreams_3_6_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  // auto result = mSession->configureStreams_3_6(requestedConfiguration,
  //         [&](auto status, auto& HalStreamConfiguration) {
  //           _hidl_cb(status, HalStreamConfiguration);
  //         });
  // step1. determine feature mode
  auto determineFeatureSession =
      [](const auto& streams,
         const auto& sessionParams) -> feature::FeatureSessionType {
#warning "TODO: should be determined by configured streams and sessionParams"
    if (property_get_int32("vendor.debug.camera.hal3plus.samplesession", 0) ==
        1)
      return FeatureSessionType::SAMPLE;
    else if (property_get_int32("vendor.debug.camera.hal3plus.dptz", 0) == 1)
      return FeatureSessionType::DPTZ;
    else if (property_get_int32("vendor.debug.camera.hal3plus.lowlatency", 0)
        == 1)
      return FeatureSessionType::LOW_LATENCY;
    else if (property_get_int32("vendor.debug.camera.hal3plus.basic", 0)
        == 1)
      return FeatureSessionType::BASIC;

    if (sessionParams.size() > 0) {
      auto metadata =
          reinterpret_cast<const camera_metadata_t*>(sessionParams.data());
      camera_metadata_ro_entry_t entry;
      {
        auto ret = find_camera_metadata_ro_entry(
            metadata, MTK_DPTZ_FEATURE_DPTZ_MODE, &entry);
        if (ret == OK && entry.data.i32[0] ==
            MTK_DPTZ_FEATURE_AVAILABLE_DPTZ_MODE_ON) {
          return FeatureSessionType::DPTZ;
        }
      }
      {
        auto ret = find_camera_metadata_ro_entry(
            metadata, MTK_LOW_LATENCY_FEATURE_LOW_LATENCY_MODE, &entry);
        if (ret == OK && entry.data.i32[0] ==
            MTK_LOW_LATENCY_FEATURE_AVAILABLE_LOW_LATENCY_MODE_ON) {
          return FeatureSessionType::LOW_LATENCY;
        }
      }
    }

    return FeatureSessionType::BASIC;
  };

  auto type =
      determineFeatureSession(requestedConfiguration.v3_4.streams,
                              requestedConfiguration.v3_4.sessionParams);

  //  extract sensor ID from metadata and write into creationParams
  int result;
  camera_metadata_ro_entry_t entry;
  std::vector<int32_t> physId;
  result = find_camera_metadata_ro_entry(
      mStaticInfo, ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS, &entry);

  MY_LOGD("entry_count: %d ", entry.count);

  if (entry.count == 0) {
    physId.push_back(mInstanceId);
  } else {
    for (int i = 0; i < entry.count; i++) {
      physId.push_back(entry.data.u8[i]);
    }

    for (std::vector<int32_t>::const_iterator i = physId.begin();
         i != physId.end(); ++i) {
      MY_LOGD("cus_physId: %d ", *i);
    }
  }

  //

  {
    feature::CreationParams creationParams = {
        .openId = mInstanceId,                // TBD
        .cameraCharateristics = mStaticInfo,  // TBD
        .sensorId = physId,                   // TBD
        .session = mSession,
        .callback = mCameraDeviceCallback,
    };
    switch (type) {
      case FeatureSessionType::SAMPLE:
        mFeatureSession =
            feature::sample::SampleSession::makeInstance(creationParams);
        break;
      case FeatureSessionType::DPTZ:
        mFeatureSession =
            NSCam::Hal3Plus::DptzSessionFactory::makeInstance(creationParams);
        break;
      case FeatureSessionType::LOW_LATENCY:
        mFeatureSession =
            NSCam::Hal3Plus::LowLatencyFactory::makeInstance(creationParams);
        break;
      default:
        mFeatureSession =
            feature::basic::BasicSession::makeInstance(creationParams);
        break;
    }
    //
    auto res =
        mFeatureSession->setExtCameraDeviceSession(mExtCameraDeviceSession);
    if (res != OK) {
      MY_LOGE("Bad mExtCameraDeviceSession");
    }

    mFeatureSession->setMetadataQueue(mRequestMetadataQueue,
                                      mResultMetadataQueue);
    auto result = mFeatureSession->configureStreams(
        requestedConfiguration, [&](auto status, auto& HalStreamConfiguration) {
          for (auto& halstream : HalStreamConfiguration.streams)
            MY_LOGI("%s", toString(halstream).c_str());

          auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
          if (pBufferHandleCacheMgr == 0) {
            MY_LOGE("Bad BufferHandleCacheMgr");
            _hidl_cb(Status::ILLEGAL_ARGUMENT, HalStreamConfiguration);
          }
          pBufferHandleCacheMgr->configBufferCacheMap(requestedConfiguration);

          _hidl_cb(status, HalStreamConfiguration);
        });
  }

  // if ( mDptzMode==0 ) {
  //   auto result = mSession->configureStreams_3_6(requestedConfiguration,
  //           [&](auto status, auto& HalStreamConfiguration) {
  //             for ( auto& halstream : HalStreamConfiguration.streams )
  //               MY_LOGI("%s", toString(halstream).c_str());
  //             _hidl_cb(status, HalStreamConfiguration);
  //           });
  // } else {
  //   // step2. create DPTZ implementation and configure
  //   feature::CreationParams creationParams = {
  //     .openId = 0,  // TBD
  //     .cameraCharateristics = nullptr,  // TBD
  //     .sensorId = {}, // TBD
  //     .session = mSession,
  //     .callback = mCameraDeviceCallback,
  //   };

  //   mFeatureSession =
  //   feature::sample::SampleSession::makeInstance(creationParams);
  //   mFeatureSession->setMetadataQueue(mRequestMetadataQueue,
  //   mResultMetadataQueue); auto result =
  //   mFeatureSession->configureStreams(requestedConfiguration,
  //           [&](auto status, auto& HalStreamConfiguration) {
  //             for ( auto& halstream : HalStreamConfiguration.streams )
  //               MY_LOGI("%s", toString(halstream).c_str());

  //               auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
  //               if (pBufferHandleCacheMgr == 0) {
  //                 MY_LOGE("Bad BufferHandleCacheMgr");
  //                 _hidl_cb(Status::ILLEGAL_ARGUMENT, HalStreamConfiguration);
  //               }
  //               pBufferHandleCacheMgr->configBufferCacheMap(requestedConfiguration);

  //             _hidl_cb(status, HalStreamConfiguration);
  //           });
  // }
  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::getCaptureRequestMetadataQueue(
    getCaptureRequestMetadataQueue_cb _hidl_cb) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  _hidl_cb(*mRequestMetadataQueue->getDesc());
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::getCaptureResultMetadataQueue(
    getCaptureResultMetadataQueue_cb _hidl_cb) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  _hidl_cb(*mResultMetadataQueue->getDesc());
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::processCaptureRequest(
    const hidl_vec<::android::hardware::camera::device::V3_2::CaptureRequest>&
        requests,
    const hidl_vec<BufferCache>& cachesToRemove,
    processCaptureRequest_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

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
  }

  auto result = mSession->processCaptureRequest(
      requests, cachesToRemove, [&](auto status, auto numRequestProcess) {
        _hidl_cb(status, numRequestProcess);
      });

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::processCaptureRequest_3_4(
    const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
        requests,
    const hidl_vec<BufferCache>& cachesToRemove,
    processCaptureRequest_3_4_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  uint32_t numRequestProcessed = 0;
  CameraMetadata metadata_queue_settings;
  std::vector<::android::hardware::camera::device::V3_4::CaptureRequest>
      outRequests;

  MY_LOGD("======== dump all requests ========");
  for (auto& request : requests) {
    MY_LOGI("%s", toString(request).c_str());
  }

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
    // read metadta from fmq
    auto result = convertAllRequestFromHidl(requests, outRequests,
                                            mRequestMetadataQueue.get(),
                                            metadata_queue_settings);

    // MY_LOGD("======== dump all output Request ========");
    // for (auto& outRequest : outRequests) {
    //   MY_LOGD("%s", toString(outRequest).c_str());
    // }
    auto ret = mFeatureSession->processCaptureRequest(
        outRequests, {}, vBufferCache,
        [&](auto status, auto numRequestProcess) {
          _hidl_cb(status, numRequestProcess);
        });

    // if ( mDptzMode==0 ) {
    //   auto result = mSession->processCaptureRequest_3_4(outRequests,
    //   cachesToRemove,
    //           [&](auto status, auto numRequestProcess) {
    //             _hidl_cb(status, numRequestProcess);
    //           });
    // } else {
    //   auto result = mFeatureSession->processCaptureRequest(outRequests, {},
    //   vBufferCache,
    //             [&](auto status, auto numRequestProcess) {
    //             _hidl_cb(status, numRequestProcess);
    //           });
    // }
  }

  // auto result = mSession->processCaptureRequest_3_4(outRequests,
  // cachesToRemove,
  //         [&](auto status, auto numRequestProcess) {
  //           _hidl_cb(status, numRequestProcess);
  //         });

  MY_LOGD("-");
  return Void();
}

// V3.5
Return<void> CusCameraDeviceSession::signalStreamFlush(
    const hidl_vec<int32_t>& streamIds,
    uint32_t streamConfigCounter) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");
  // std::vector<int32_t> vStreamIds;
  // for (auto& id : streamIds) {
  //   vStreamIds.push_back(id);
  // }
  mSession->signalStreamFlush(streamIds, streamConfigCounter);

  return Void();
}

Return<void> CusCameraDeviceSession::isReconfigurationRequired(
    const hidl_vec<uint8_t>& oldSessionParams,
    const hidl_vec<uint8_t>& newSessionParams,
    isReconfigurationRequired_cb _hidl_cb) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  auto ret = mSession->isReconfigurationRequired(
      oldSessionParams, newSessionParams,
      [&](auto status, auto needReconfig) { _hidl_cb(status, needReconfig); });

  return Void();
}

// V3.6
Return<void> CusCameraDeviceSession::switchToOffline(
    const hidl_vec<int32_t>& streamsToKeep,
    switchToOffline_cb _hidl_cb) {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
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
Return<Status> CusCameraDeviceSession::flush() {
  MY_LOGD("+");
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return Status::INTERNAL_ERROR;
  }
  Status status;
  // if ( mDptzMode==0 ) {
  //   status = mSession->flush();
  // } else {
  //   status = mFeatureSession->flush();
  // }
  if (mFeatureSession) {
    status = mFeatureSession->flush();
  } else {
    status = mSession->flush();
  }

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
  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> CusCameraDeviceSession::close() {
  MY_LOGD("+");
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  // unlink to death notification for existed device callback
  if (mCameraDeviceCallback != nullptr) {
    mCameraDeviceCallback->unlinkToDeath(this);
    mCameraDeviceCallback = nullptr;
  }
  ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  // if ( mDptzMode==0 ) {
  //   auto result = mSession->close();
  //   if (CC_UNLIKELY(!result.isOk())) {
  //     MY_LOGE("CusCameraDeviceSession close fail");
  //   }
  // } else {
  //   auto result  = mFeatureSession->close();
  //   if (CC_UNLIKELY(!result.isOk())) {
  //     MY_LOGE("CusCameraDeviceSession close fail");
  //   }
  // }
  if (mFeatureSession) {
    auto result = mFeatureSession->close();
    if (CC_UNLIKELY(!result.isOk())) {
      MY_LOGE("CusCameraDeviceSession close fail");
    }
  } else {
    auto result = mSession->close();
    if (CC_UNLIKELY(!result.isOk())) {
      MY_LOGE("CusCameraDeviceSession close fail");
    }
  }

  //
  {
    Mutex::Autolock _l(mBufferHandleCacheMgrLock);
    if (mBufferHandleCacheMgr != nullptr) {
      mBufferHandleCacheMgr->destroy();
      mBufferHandleCacheMgr = nullptr;
    }
  }
  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceSession::serviceDied(
    uint64_t cookie __unused,
    const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused) {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  if (cookie != (uint64_t)this) {
    MY_LOGW("Unexpected ICameraDeviceSession serviceDied cookie 0x%" PRIx64
            ", expected %p",
            cookie, this);
  }

  MY_LOGW("ICameraDeviceSession has died - %s; removing it - cookie:0x%" PRIx64
          " this: %p",
          toString(mLinkToDeathDebugInfo).c_str(), cookie, this);

  // force to close camera
  MY_LOGW("CameraService has died, force to close camera device %d:",
          getInstanceId());
  // mSession->getInstanceId());

  flush();
  close();
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::createCustomSessionCallbacks()
    -> std::shared_ptr<custom_dev3::CustomCameraDeviceSessionCallback> {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  auto sessionCallbacks =
      std::make_shared<custom_dev3::CustomCameraDeviceSessionCallback>();
  if (sessionCallbacks != nullptr) {
    sessionCallbacks->processCaptureResult =
        custom_dev3::ProcessCaptureResultFunc(
            [this](const ::android::hardware::hidl_vec<
                   ::android::hardware::camera::device::V3_4::CaptureResult>&
                       results) { processCaptureResult(results); });
    sessionCallbacks->notify = NotifyFunc(
        [this](const ::android::hardware::hidl_vec<
               ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) {
          notify(msgs);
        });
    sessionCallbacks
        ->requestStreamBuffers = custom_dev3::RequestStreamBuffersFunc(
        [this](const ::android::hardware::hidl_vec<
                   ::android::hardware::camera::device::V3_5::BufferRequest>&
                   vBufferRequests,
               ::android::hardware::hidl_vec<
                   ::android::hardware::camera::device::V3_5::StreamBufferRet>**
                   pvBufferReturns) {
          return requestStreamBuffers(vBufferRequests, pvBufferReturns);
        });
    sessionCallbacks->returnStreamBuffers =
        custom_dev3::ReturnStreamBuffersFunc(
            [this](const ::android::hardware::hidl_vec<
                   ::android::hardware::camera::device::V3_2::StreamBuffer>&
                       buffers) { returnStreamBuffers(buffers); });
    return sessionCallbacks;
  }
  MY_LOGE("fail to create CustomCameraDevice3SessionCallback");
  return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::processCaptureResult(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::CaptureResult>& results)
    -> void {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  MY_LOGD("+");

  // for (auto& result : results) {
  //   MY_LOGI("%s", toString(result).c_str());
  //   for (auto& streambuffer : result.v3_2.outputBuffers)
  //     MY_LOGI("%s", toString(streambuffer).c_str());
  // }

  auto result = mFeatureSession->processCaptureResult(results);
  if (CC_UNLIKELY(!result.isOk())) {
    MY_LOGE(
        "Transaction error in ICameraDeviceCallback::processCaptureResult_3_4: "
        "%s",
        result.description().c_str());
  }

  // if ( mDptzMode ) {
  //   // dptz session should remove customized working streambuffer
  //   auto result = mFeatureSession->processCaptureResult(results);
  //   if (CC_UNLIKELY(!result.isOk())) {
  //     MY_LOGE(
  //         "Transaction error in
  //         ICameraDeviceCallback::processCaptureResult_3_4: "
  //         "%s",
  //         result.description().c_str());
  //   }
  // } else {
  //   // write metadata to fmq
  //   if (auto pResultMetadataQueue = mResultMetadataQueue.get()) {
  //     for (size_t i = 0; i < results.size(); i++) {
  //       auto& hidl_result = results[i];
  //       // write logical meta into FMQ
  //       if (hidl_result.v3_2.result.size() > 0) {
  //         if (CC_LIKELY(
  //                 pResultMetadataQueue->write(hidl_result.v3_2.result.data(),
  //                                             hidl_result.v3_2.result.size())))
  //                                             {
  //           const_cast<uint64_t&>(hidl_result.v3_2.fmqResultSize) =
  //           hidl_result.v3_2.result.size(); const_cast<hidl_vec<uint8_t>&>
  //           (hidl_result.v3_2.result) = hidl_vec<uint8_t>();  // resize(0)
  //         } else {
  //           const_cast<hidl_vec<uint8_t>&>(hidl_result.v3_2.result) = 0;
  //           MY_LOGW(
  //               "fail to write meta to mResultMetadataQueue, data=%p,
  //               size=%zu", hidl_result.v3_2.result.data(),
  //               hidl_result.v3_2.result.size());
  //         }
  //       }

  //       //write physical meta into FMQ
  //       if (hidl_result.physicalCameraMetadata.size() > 0) {
  //         for (size_t i = 0; i < hidl_result.physicalCameraMetadata.size();
  //         i++) {
  //           if (CC_LIKELY(pResultMetadataQueue->write(
  //                   hidl_result.physicalCameraMetadata[i].metadata.data(),
  //                   hidl_result.physicalCameraMetadata[i].metadata.size())))
  //                   {
  //             const_cast<uint64_t&>(hidl_result.physicalCameraMetadata[i].fmqMetadataSize)
  //             =
  //                 hidl_result.physicalCameraMetadata[i].metadata.size();
  //             const_cast<hidl_vec<uint8_t>&>
  //             (hidl_result.physicalCameraMetadata[i].metadata) =
  //                 hidl_vec<uint8_t>();  // resize(0)
  //           } else {
  //             MY_LOGW(
  //                 "fail to write physical meta to mResultMetadataQueue,
  //                 data=%p, " "size=%zu",
  //                 hidl_result.physicalCameraMetadata[i].metadata.data(),
  //                 hidl_result.physicalCameraMetadata[i].metadata.size());
  //           }
  //         }
  //       }
  //     }
  //   }
  //   //
  //   for (auto& result : results) {
  //     MY_LOGI("%s", toString(result).c_str());
  //     for (auto& streambuffer : result.v3_2.outputBuffers)
  //       MY_LOGI("%s", toString(streambuffer).c_str());
  //   }
  //   auto result = mCameraDeviceCallback->processCaptureResult_3_4(results);
  //   if (CC_UNLIKELY(!result.isOk())) {
  //     MY_LOGE(
  //         "Transaction error in
  //         ICameraDeviceCallback::processCaptureResult_3_4: "
  //         "%s",
  //         result.description().c_str());
  //   }
  // }
#if 0


  // MY_LOGI("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
  // auto ret = mCameraDeviceCallback->processCaptureResult_3_4(results);
  // if (CC_UNLIKELY(!ret.isOk())) {
  //  MY_LOGE(
  //   "Transaction error in ICameraDeviceCallback::processCaptureResult_3_4: "
  //   "%s",
  //   ret.description().c_str());
  // }

  for (auto& result : results) {
    MY_LOGI("%s", toString(result).c_str());
    for (auto& streambuffer : result.v3_2.outputBuffers)
      MY_LOGI("%s", toString(streambuffer).c_str());

    if ( result.v3_2.outputBuffers.size() ||
         result.v3_2.inputBuffer.streamId != -1 ||
         result.v3_2.fmqResultSize ||
         result.v3_2.result.size() ) {
      MY_LOGI("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
      // std::vector<::android::hardware::
      //             camera::device::V3_4::CaptureResult> vResults;
      // vResults.push_back(std::move(result));

      // hidl_vec<::android::hardware::
      // camera::device::V3_4::CaptureResult> hidlResults;
      // hidlResults.setToExternal(vResults.data(), vResults.size());

      MY_LOGI("%s", toString(result).c_str());
      for (auto& streambuffer : result.v3_2.outputBuffers)
        MY_LOGI("%s", toString(streambuffer).c_str());

      auto ret = mCameraDeviceCallback->processCaptureResult_3_4({result});
      if (CC_UNLIKELY(!ret.isOk())) {
        MY_LOGE(
            "Transaction error in"
            "ICameraDeviceCallback::processCaptureResult_3_4: "
            "%s",
            ret.description().c_str());
      }
    }
  }

  // for (auto& result : results ) {
  //   if ( result.v3_2.outputBuffers.size() ||
  //        result.v3_2.inputBuffer.streamId!=-1 ||
  //        result.v3_2.fmqResultSize || result.v3_2.result.size() ) {
  // for (size_t i=0; i<results.size(); ++i ) {
  //   if ( results[i].v3_2.outputBuffers.size() ||
  //        results[i].v3_2.inputBuffer.streamId!=-1 ||
  //        results[i].v3_2.fmqResultSize || results[i].v3_2.result.size() ) {

  //     MY_LOGI("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
  //     hidl_vec<::android::hardware::camera::
  //              device::V3_4::CaptureResult> hidlResults;
  //     hidlResults = {results[i]};

  //     MY_LOGI("%s", toString(results[i]).c_str());
  //     for (auto& streambuffer : results[i].v3_2.outputBuffers)
  //       MY_LOGI("%s", toString(streambuffer).c_str());

  //     auto ret = mCameraDeviceCallback
  //                ->processCaptureResult_3_4(hidlResults);
  //     if (CC_UNLIKELY(!ret.isOk())) {
  //       MY_LOGE(
  //           "Transaction error in"
  //           "ICameraDeviceCallback::processCaptureResult_3_4: "
  //           "%s",
  //           ret.description().c_str());
  //     }
  //   }
  // }
#endif
  MY_LOGD("-");
  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::notify(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) -> void {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  if (msgs.size()) {
    // if ( mDptzMode ) {
    //   auto result = mFeatureSession->notify(msgs);
    //   if (CC_UNLIKELY(!result.isOk())) {
    //     MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
    //             result.description().c_str());
    //   }
    // } else {
    //   auto result = mCameraDeviceCallback->notify(msgs);
    //   if (CC_UNLIKELY(!result.isOk())) {
    //     MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
    //             result.description().c_str());
    //   }
    // }
    auto result = mFeatureSession->notify(msgs);
    if (CC_UNLIKELY(!result.isOk())) {
      MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
              result.description().c_str());
    }
  } else {
    MY_LOGE("msgs size is zero, bypass the megs");
  }
}

/******************************************************************************
 *
 ******************************************************************************/

auto CusCameraDeviceSession::requestStreamBuffers(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_5::BufferRequest>&
        buffer_requests,
    ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_5::StreamBufferRet>**
        pvBufferReturns)
    -> ::android::hardware::camera::device::V3_5::BufferRequestStatus {
  MY_LOGE("Not implement");
  return ::android::hardware::camera::device::V3_5::BufferRequestStatus::
      FAILED_UNKNOWN;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::returnStreamBuffers(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::StreamBuffer>& buffers)
    -> void {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());
  MY_LOGE("Not implement");
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::convertAllRequestFromHidl(
    const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
        inRequests,
    std::vector<::android::hardware::camera::device::V3_4::CaptureRequest>&
        outRequests,
    RequestMetadataQueue* pRequestFMQ,
    CameraMetadata& metadata_queue_settings) -> ::android::status_t {
  status_t result = OK;

  for (auto& inRequest : inRequests) {
    ::android::hardware::camera::device::V3_4::CaptureRequest outRequest;
    RequestBufferCache* currentBufferCache = {};

    if (CC_LIKELY(OK == convertRequestFromHidl(inRequest, outRequest,
                                               pRequestFMQ,
                                               metadata_queue_settings))) {
      outRequests.push_back(std::move(outRequest));

    } else {
      MY_LOGE("convert to Hal request failed, dump reqeust info : %s",
              toString(inRequest).c_str());
      result = BAD_VALUE;
    }
  }

  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::convertRequestFromHidl(
    const ::android::hardware::camera::device::V3_4::CaptureRequest&
        hidl_request,
    ::android::hardware::camera::device::V3_4::CaptureRequest& outRequest,
    RequestMetadataQueue* request_metadata_queue,
    CameraMetadata& metadata_queue_settings) -> ::android::status_t {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  outRequest.v3_2.frameNumber = hidl_request.v3_2.frameNumber;
  outRequest.v3_2.inputBuffer = hidl_request.v3_2.inputBuffer;
  outRequest.v3_2.outputBuffers = hidl_request.v3_2.outputBuffers;
  outRequest.v3_2.fmqSettingsSize = 0;

  // 1. read Metadata: logical metadata
  status_t res =
      readMetadatafromFMQ(hidl_request.v3_2.fmqSettingsSize,
                          request_metadata_queue, hidl_request.v3_2.settings,
                          outRequest.v3_2.settings, metadata_queue_settings);
  if (res != OK) {
    MY_LOGE("read metadata failed: %s(%d)", strerror(-res), res);
    return res;
  }

  // 2. read Metadata: physical metadata
  for (auto& hidl_physical_settings : hidl_request.physicalCameraSettings) {
    // NSCam::v3::PhysicalCameraSetting hal_PhysicalCameraSetting;

    // status_t res = readMetadatafromFMQ(
    //     hidl_physical_settings.fmqSettingsSize,
    //     request_metadata_queue,
    //     hidl_physical_settings.settings,
    //     metadata_queue_settings);
    // if (res != OK) {
    //   MY_LOGE("Converting to HAL physical metadata failed: %s(%d)",
    //           strerror(-res), res);
    //   return res;
    // }
  }

  // MY_LOGD("======== dump outRequest ========");
  // MY_LOGE("%s", toString(outRequest).c_str());

  return OK;
}

auto CusCameraDeviceSession::readMetadatafromFMQ(
    const uint64_t message_queue_setting_size,
    RequestMetadataQueue* request_metadata_queue,
    const CameraMetadata& request_setting,
    CameraMetadata& out_request_setting,
    CameraMetadata& metadata_queue_settings) -> ::android::status_t {
  CAM_TRACE_NAME(
      android::String8::format("%s: %s", getLogPrefix().c_str(), __FUNCTION__)
          .string());

  // const camera_metadata_t* metadata = nullptr;
  // CameraMetadata metadata_queue_settings;

  if (message_queue_setting_size == 0) {
    // Use the settings in the request.
    if (request_setting.size() != 0) {
      out_request_setting.setToExternal(
          const_cast<uint8_t*>(request_setting.data()), request_setting.size());
    }
  } else {
    // Read the settings from request metadata queue.
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
  }

  out_request_setting.setToExternal(metadata_queue_settings.data(),
                                    metadata_queue_settings.size());

  return OK;
}

};  // namespace custom_dev3
};  // namespace NSCam
