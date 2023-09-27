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

#include "main/mtkhal/custom/hidl/device/3.x/feature/basic/BasicSession.h"

#include <cutils/properties.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../MyUtils.h"
// #include "../../cuscallback/ICustCallback.h"

using ::android::BAD_VALUE;
using ::android::Mutex;
using ::android::NAME_NOT_FOUND;
using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;
using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_6::HalStream;
using ::android::hardware::camera::device::V3_6::ICameraDeviceSession;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using std::vector;

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

// using namespace NSCam;
// using namespace NSCam::custom_dev3;
// using namespace NSCam::custom_dev3::feature::basic;

namespace NSCam {
namespace custom_dev3 {
namespace feature {
namespace basic {

/******************************************************************************
 *
 ******************************************************************************/
auto BasicSession::makeInstance(CreationParams const& rParams)
    -> std::shared_ptr<IFeatureSession> {
  auto pSession = std::make_shared<BasicSession>(rParams);
  if (pSession == nullptr) {
    CAM_ULOGME("[%s] Bad pSession", __FUNCTION__);
    return nullptr;
  }
  return pSession;
}

/******************************************************************************
 *
 ******************************************************************************/
BasicSession::BasicSession(CreationParams const& rParams)
    : mSession(rParams.session),
      mCameraDeviceCallback(rParams.callback),
      mInstanceId(rParams.openId),
      mCameraCharacteristics(rParams.cameraCharateristics),
      mvSensorId(rParams.sensorId),
      mLogPrefix("basic-session-" + std::to_string(rParams.openId)) {
  MY_LOGI("ctor");
  if (!mvSensorId.empty()) {
    MY_LOGI("BasicSession_sensorId : %d ", mvSensorId[0]);
  }

  if (CC_UNLIKELY(mSession == nullptr || mCameraDeviceCallback == nullptr)) {
    MY_LOGE("session does not exist");
  }

  bEnableCustCallback =
      property_get_int32("vendor.debug.camera.hal3plus.enableCustCallback", 0);
}

/******************************************************************************
 *
 ******************************************************************************/
BasicSession::~BasicSession() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto BasicSession::setMetadataQueue(
    std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
    std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue) -> void {
  mRequestMetadataQueue = pRequestMetadataQueue;
  mResultMetadataQueue = pResultMetadataQueue;
  // create CustCallback
  // if (bEnableCustCallback) {
  //   Mutex::Autolock _l(mCustCallbackLock);
  //   ICustCallback::CreationInfo creationInfo{
  //       .mInstanceId = mInstanceId,
  //       .mCameraDeviceCallback = mCameraDeviceCallback,
  //       .mResultMetadataQueue = mResultMetadataQueue,
  //       .mResultMetadataQueueSize = size_t(16 << 20),
  //       .aAtMostMetaStreamCount = 10,
  //   };
  //   if (mCustCallback != nullptr) {
  //     MY_LOGE("mCustCallback:%p != 0 while opening", mCustCallback.get());
  //     mCustCallback->destroy();
  //     mCustCallback = nullptr;
  //   }
  //   mCustCallback = ICustCallback::create(creationInfo);
  //   if (mCustCallback == nullptr) {
  //     MY_LOGE("IAppStreamManager::create");
  //     return;
  //   }
  // }
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
Return<void> BasicSession::configureStreams(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        requestedConfiguration,
    ::android::hardware::camera::device::V3_6::ICameraDeviceSession::
        configureStreams_3_6_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  // if (bEnableCustCallback) {
  //   auto pCustCallback = getSafeCustCallback();
  //   if (pCustCallback == 0) {
  //     MY_LOGE("Bad CustCallback");
  //     return Void();
  //   }
  //   pCustCallback->configureStreams(requestedConfiguration.v3_4);
  // }

  auto result = mSession->configureStreams_3_6(
      requestedConfiguration, [&](auto status, auto& HalStreamConfiguration) {
        for (auto& halstream : HalStreamConfiguration.streams)
          MY_LOGI("%s", toString(halstream).c_str());
        _hidl_cb(status, HalStreamConfiguration);
      });

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> BasicSession::processCaptureRequest(
    const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
        requests,
    const hidl_vec<BufferCache>& cachesToRemove,
    const std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
    android::hardware::camera::device::V3_4::ICameraDeviceSession::
        processCaptureRequest_3_4_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();

  // MY_LOGD("======== dump all requests ========");
  // for (auto& request : requests) {
  //   MY_LOGI("%s", toString(request).c_str());
  //   for (auto& streambuffer : request.v3_2.outputBuffers)
  //       MY_LOGI("%s", toString(streambuffer).c_str());
  // }

  // if (bEnableCustCallback) {
  //   auto pCustCallback = getSafeCustCallback();
  //   if (pCustCallback == 0) {
  //     MY_LOGE("Bad CustCallback");
  //     return Void();
  //   }
  //   pCustCallback->submitRequests(requests);
  // }

  auto result = mSession->processCaptureRequest_3_4(
      requests, cachesToRemove, [&](auto status, auto numRequestProcess) {
        _hidl_cb(status, numRequestProcess);
      });

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status> BasicSession::flush() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return Status::INTERNAL_ERROR;
  }

  auto status = mSession->flush();

  // if (bEnableCustCallback) {
  //   auto pCustCallback = getSafeCustCallback();
  //   if (pCustCallback == 0) {
  //     MY_LOGE("Bad CustCallback");
  //     return status;
  //   }
  //   MY_LOGD("pCustCallback->waitUntilDrained +");
  //   pCustCallback->waitUntilDrained(500000000);
  //   MY_LOGD("pCustCallback->waitUntilDrained -");
  // }

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> BasicSession::close() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto result = mSession->close();
  if (CC_UNLIKELY(result.isOk())) {
    MY_LOGE("CusCameraDeviceSession close fail");
  }

  // if (bEnableCustCallback) {
  //   auto pCustCallback = getSafeCustCallback();
  //   if (pCustCallback == 0) {
  //     MY_LOGE("Bad CustCallback");
  //     return Void();
  //   }
  //   pCustCallback->destroy();
  // }

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> BasicSession::notify(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  if (bEnableCustCallback) {
    // auto pCustCallback = getSafeCustCallback();
    // if (pCustCallback == 0) {
    //   MY_LOGE("Bad CustCallback");
    //   return Void();
    // }
    // pCustCallback->notify(msgs);
  } else {
    if (msgs.size()) {
      auto result = mCameraDeviceCallback->notify(msgs);
      if (CC_UNLIKELY(!result.isOk())) {
        MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
                result.description().c_str());
      }
    } else {
      MY_LOGE("msgs size is zero, bypass the megs");
    }
  }

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> BasicSession::processCaptureResult(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::CaptureResult>& results) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  if (bEnableCustCallback) {
    // auto pCustCallback = getSafeCustCallback();
    // if (pCustCallback == 0) {
    //   MY_LOGE("Bad CustCallback");
    //   return Void();
    // }
    // pCustCallback->processCaptureResult(results);
  } else {
    // write metadata to fmq
    if (auto pResultMetadataQueue = mResultMetadataQueue.get()) {
      for (size_t i = 0; i < results.size(); i++) {
        auto& hidl_result = results[i];
        // write logical meta into FMQ
        if (hidl_result.v3_2.result.size() > 0) {
          if (CC_LIKELY(pResultMetadataQueue->write(
                  hidl_result.v3_2.result.data(),
                  hidl_result.v3_2.result.size()))) {
            const_cast<uint64_t&>(hidl_result.v3_2.fmqResultSize) =
                hidl_result.v3_2.result.size();
            const_cast<hidl_vec<uint8_t>&>(hidl_result.v3_2.result) =
                hidl_vec<uint8_t>();  // resize(0)
          } else {
            const_cast<hidl_vec<uint8_t>&>(hidl_result.v3_2.result) = 0;
            MY_LOGW(
                "fail to write meta to mResultMetadataQueue, data=%p, size=%zu",
                hidl_result.v3_2.result.data(), hidl_result.v3_2.result.size());
          }
        }

        // write physical meta into FMQ
        if (hidl_result.physicalCameraMetadata.size() > 0) {
          for (size_t i = 0; i < hidl_result.physicalCameraMetadata.size();
               i++) {
            if (CC_LIKELY(pResultMetadataQueue->write(
                    hidl_result.physicalCameraMetadata[i].metadata.data(),
                    hidl_result.physicalCameraMetadata[i].metadata.size()))) {
              const_cast<uint64_t&>(
                  hidl_result.physicalCameraMetadata[i].fmqMetadataSize) =
                  hidl_result.physicalCameraMetadata[i].metadata.size();
              const_cast<hidl_vec<uint8_t>&>(
                  hidl_result.physicalCameraMetadata[i].metadata) =
                  hidl_vec<uint8_t>();  // resize(0)
            } else {
              MY_LOGW(
                  "fail to write physical meta to mResultMetadataQueue, "
                  "data=%p, "
                  "size=%zu",
                  hidl_result.physicalCameraMetadata[i].metadata.data(),
                  hidl_result.physicalCameraMetadata[i].metadata.size());
            }
          }
        }
      }
    }
    //
    // for (auto& result : results) {
    //   MY_LOGI("%s", toString(result).c_str());
    //   for (auto& streambuffer : result.v3_2.outputBuffers)
    //     MY_LOGI("%s", toString(streambuffer).c_str());
    // }
    auto result = mCameraDeviceCallback->processCaptureResult_3_4(results);
    if (CC_UNLIKELY(!result.isOk())) {
      MY_LOGE(
          "Transaction error in "
          "ICameraDeviceCallback::processCaptureResult_3_4: "
          "%s",
          result.description().c_str());
    }
  }

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
// auto BasicSession::getSafeCustCallback() const ->
// ::android::sp<ICustCallback> {
//   //  Although mCustCallback is setup during opening camera,
//   //  we're not sure any callback to this class will arrive
//   //  between open and close calls.
//   Mutex::Autolock _l(mCustCallbackLock);
//   return mCustCallback;
// }

};  // namespace basic
};  // namespace feature
};  // namespace custom_dev3
};  // namespace NSCam
