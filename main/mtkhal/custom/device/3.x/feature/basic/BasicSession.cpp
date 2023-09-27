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

#include "main/mtkhal/custom/device/3.x/feature/basic/BasicSession.h"

#include <cutils/properties.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

// using ::android::BAD_VALUE;
// using ::android::Mutex;
// using ::android::NAME_NOT_FOUND;
// using ::android::NO_INIT;
// using ::android::OK;
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
// using namespace mcam;
// using namespace mcam::custom;
// using namespace mcam::custom::feature::basic;
using mcam::custom::feature::CreationParams;
using mcam::custom::feature::IFeatureSession;

namespace mcam {
namespace custom {
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
  MY_LOGI("BasicSession_sensorId : %d ", mvSensorId[0]);

  if (CC_UNLIKELY(mSession == nullptr || mCameraDeviceCallback == nullptr)) {
    MY_LOGE("session does not exist");
  }
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
// auto BasicSession::setMetadataQueue(
//     std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
//     std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue) -> void {
//   mRequestMetadataQueue = pRequestMetadataQueue;
//   mResultMetadataQueue = pResultMetadataQueue;
// }

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
int BasicSession::configureStreams(
    const mcam::StreamConfiguration& requestedConfiguration,
    mcam::HalStreamConfiguration& halConfiguration) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  auto result =
      mSession->configureStreams(requestedConfiguration, halConfiguration);

  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
int BasicSession::processCaptureRequest(
    const std::vector<mcam::CaptureRequest>& requests,
    uint32_t& numRequestProcessed) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();

  auto result = mSession->processCaptureRequest(requests, numRequestProcessed);

  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
int BasicSession::flush() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return 7;  // Status::INTERNAL_ERROR
  }

  auto status = mSession->flush();

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
int BasicSession::close() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto result = mSession->close();
  if (CC_UNLIKELY(result != 0)) {
    MY_LOGE("BasicSession close fail");
  }
  mSession = nullptr;

  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
void BasicSession::notify(const std::vector<mcam::NotifyMsg>& msgs) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  if (msgs.size()) {
    mCameraDeviceCallback->notify(msgs);
  } else {
    MY_LOGE("msgs size is zero, bypass the megs");
  }

  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
void BasicSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  mCameraDeviceCallback->processCaptureResult(results);

  MY_LOGD("-");
}

};  // namespace basic
};  // namespace feature
};  // namespace custom
};  // namespace mcam
