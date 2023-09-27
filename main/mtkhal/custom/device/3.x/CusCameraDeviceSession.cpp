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

#include <cutils/properties.h>
#include <dlfcn.h>
#include <main/mtkhal/custom/device/3.x/CusCameraDeviceSession.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>
//
#include <memory>
#include <string>
#include <utility>
#include <sstream>
#include <vector>
//
#include "MyUtils.h"
//
#include "feature/basic/BasicSession.h"
#include "feature/sample/SampleSession.h"
#include "feature/multicam/MulticamSession.h"
// #include "custzone/dptz/DptzSessionFactory.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

// using mcam::BufferRequest;
// using mcam::BufferRequestStatus;
using mcam::CameraOfflineSessionInfo;
using mcam::CaptureResult;
// using mcam::ExtConfigurationResults;
using mcam::HalStreamConfiguration;
using mcam::IMtkcamDeviceCallback;
using mcam::IMtkcamDeviceSession;
using mcam::NotifyMsg;
using mcam::RequestTemplate;
using mcam::StreamBuffer;
// using mcam::StreamBufferRet;
using mcam::StreamConfiguration;
using mcam::custom::CusCameraDeviceCallback;
using NSCam::IMetadata;
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

namespace mcam {
namespace custom {
/******************************************************************************
 *
 ******************************************************************************/
CusCameraDeviceSession::~CusCameraDeviceSession() {
  MY_LOGI("dtor");
  if (!mStaticInfo.isEmpty())
    mStaticInfo.clear();
}

/******************************************************************************
 *
 ******************************************************************************/
CusCameraDeviceSession::CusCameraDeviceSession(int32_t intanceId,
                                               IMetadata staticMetadata)
    : IMtkcamDeviceSession(),
      mCameraDeviceCallback(nullptr),
      mLogPrefix(std::to_string(intanceId) + "-custom-session"),
      mInstanceId(intanceId) {
  MY_LOGI("ctor");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("session does not exist");
  }
  mStaticInfo = staticMetadata;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CusCameraDeviceSession::create(int32_t intanceId, IMetadata staticMetadata)
    -> CusCameraDeviceSession* {
  auto pInstance = new CusCameraDeviceSession(intanceId, staticMetadata);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceSession::open(
    const std::shared_ptr<mcam::IMtkcamDeviceCallback>& callback) {
  auto customCallback =
      std::shared_ptr<mcam::custom::CusCameraDeviceCallback>(
          CusCameraDeviceCallback::create(
              getInstanceId()));  // create CusCameraDeviceCallback

  customCallback->setCustomSessionCallback(
      callback);  // set callback in CusCameraDeviceCallback

  mCameraDeviceCallback = customCallback;
  MY_LOGI("mCameraDeviceCallback(%p) callback(%p)", mCameraDeviceCallback.get(),
          callback.get());
  auto status = initialize();
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDeviceSession::initialize() {
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDeviceSession::constructDefaultRequestSettings(
    RequestTemplate type,
    IMetadata& requestTemplate) {
  return mSession->constructDefaultRequestSettings(type, requestTemplate);
}

/******************************************************************************
 *
 ******************************************************************************/

// V3.6 is identical to V3.5
int CusCameraDeviceSession::configureStreams(
    const mcam::StreamConfiguration& requestedConfiguration,
    mcam::HalStreamConfiguration& halConfiguration) {
  auto determineFeatureSession =
      [](const auto& streams,
         const auto& sessionParams) -> feature::FeatureSessionType {
#warning "TODO: should be determined by configured streams and sessionParams"
    if (property_get_int32("vendor.debug.camera.hal3plus.samplesession", 0) ==
        1)
      return feature::FeatureSessionType::SAMPLE;
    else if (
      property_get_int32("vendor.debug.camera.custzone.multicam", 1) == 1) {
      return feature::FeatureSessionType::MULTICAM;
    }
    // else if (property_get_int32("vendor.debug.camera.custzone.dptz", 0) == 1)
    //   return feature::FeatureSessionType::DPTZ;
    else
      return feature::FeatureSessionType::BASIC;
  };

  auto type = determineFeatureSession(requestedConfiguration.streams,
                                      requestedConfiguration.sessionParams);

  std::vector<int32_t> vPhysicalSensors;
  auto pMetadataProvider = std::shared_ptr<NSCam::IMetadataProvider>(
      NSCam::IMetadataProvider::create(mInstanceId));

  if (pMetadataProvider) {
    NSCam::IMetadata::IEntry const& entry =
        pMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
    MUINT32 count = entry.count();
    if (count == 0) {
      vPhysicalSensors.push_back(mInstanceId);
    } else {
      MY_LOGD("parse MTK_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS");
      std::string sensorListString;
      for (size_t i = 0; i < count; ++i) {
        auto c = entry.itemAt(i, NSCam::Type2Type<MUINT8>());
        sensorListString.push_back(c);
      }
      std::istringstream iss(sensorListString);
      for (std::string token; std::getline(iss, token, '\0'); ) {
        vPhysicalSensors.push_back(std::stoi(token));
        MY_LOGD("parse physId: %d", std::stoi(token));
      }
      // dump
      // for (auto const& id : vPhysicalSensors) {
      //   MY_LOGD("cus_physId: %d ", id);
      // }
    }
  }

  feature::CreationParams creationParams = {
      .openId = mInstanceId,                // TBD
      .cameraCharateristics = mStaticInfo,  // TBD
      .sensorId = vPhysicalSensors,         // TBD
      .session = mSession,
      .callback = mCameraDeviceCallback,
  };

  switch (type) {
    case feature::FeatureSessionType::SAMPLE:
      mFeatureSession =
          feature::sample::SampleSession::makeInstance(creationParams);
      break;
    case feature::FeatureSessionType::MULTICAM:
      mFeatureSession =
          feature::multicam::MulticamSession::makeInstance(creationParams);
      break;
    // case feature::FeatureSessionType::DPTZ:
    //   mFeatureSession =
    //       NSCam::CustZone::DptzSessionFactory::makeInstance(creationParams);
    //   break;
    default:
      mFeatureSession =
          feature::basic::BasicSession::makeInstance(creationParams);
      break;
  }

  return mFeatureSession->configureStreams(requestedConfiguration,
                                           halConfiguration);
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDeviceSession::processCaptureRequest(
    const std::vector<mcam::CaptureRequest>& requests,
    uint32_t& numRequestProcessed) {
  MY_LOGD("+");
  numRequestProcessed = 0;
  auto result =
      mFeatureSession->processCaptureRequest(requests, numRequestProcessed);
  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/

// V3.5
void CusCameraDeviceSession::signalStreamFlush(
    const std::vector<int32_t>& streamIds,
    uint32_t streamConfigCounter) {
  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  mSession->signalStreamFlush(streamIds, streamConfigCounter);
}

int CusCameraDeviceSession::isReconfigurationRequired(
    const NSCam::IMetadata& oldSessionParams,
    const NSCam::IMetadata& newSessionParams,
    bool& reconfigurationNeeded) const {
  auto ret = mSession->isReconfigurationRequired(
      oldSessionParams, newSessionParams, reconfigurationNeeded);
  return ret;
}

// V3.6
int CusCameraDeviceSession::switchToOffline(
    const std::vector<int64_t>& streamsToKeep,
    mcam::CameraOfflineSessionInfo& offlineSessionInfo,
    std::shared_ptr<IMtkcamOfflineSession>& offlineSession) {
  return mSession->switchToOffline(streamsToKeep, offlineSessionInfo,
                                   offlineSession);
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDeviceSession::flush() {
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return 7;  // Status::INTERNAL_ERROR
  }
  int status;
  if (mFeatureSession != nullptr)
    status = mFeatureSession->flush();

  MY_LOGD("-");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
int CusCameraDeviceSession::close() {
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto result = mFeatureSession->close();

  if (CC_UNLIKELY(result != 0)) {
    MY_LOGE("CusCameraDeviceSession close fail");
  }

  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceSession::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  if (msgs.size()) {
    mFeatureSession->notify(msgs);
  } else {
    MY_LOGE("msgs size is zero, bypass the megs");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  MY_LOGD("+");
  mFeatureSession->processCaptureResult(results);
  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
#if 0
mcam::BufferRequestStatus CusCameraDeviceSession::requestStreamBuffers(
    const std::vector<mcam::BufferRequest>& vBufferRequests,
    std::vector<mcam::StreamBufferRet>* pvBufferReturns) {
  return mCameraDeviceCallback->requestStreamBuffers(vBufferRequests,
                                                     pvBufferReturns);
}
#endif
/******************************************************************************
 *
 ******************************************************************************/
#if 0
void CusCameraDeviceSession::returnStreamBuffers(
    const std::vector<mcam::StreamBuffer>& buffers) {
  return mCameraDeviceCallback->returnStreamBuffers(buffers);
}
#endif

/******************************************************************************
 *
 ******************************************************************************/
void CusCameraDeviceSession::setCameraDeviceSession(
    const std::shared_ptr<mcam::IMtkcamDeviceSession>& session) {
  if (session == nullptr) {
    MY_LOGE("fail to setCameraDeviceSession due to session is nullptr");
  }
  mSession = session;
}

/******************************************************************************
 *
 ******************************************************************************/
#if 1
int CusCameraDeviceSession::getConfigurationResults(
    const uint32_t streamConfigCounter,
    ExtConfigurationResults& ConfigurationResults) {
  return mSession->getConfigurationResults(streamConfigCounter,
                                           ConfigurationResults);
}
#endif

/******************************************************************************
 *
 ******************************************************************************/

};  // namespace custom
};  // namespace mcam
