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
 * MediaTek Inc. (C) 2021. All rights reserved.
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

#include <main/mtkhal/android/device/3.x/ACameraOfflineSession.h>
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <memory>
#include <vector>

using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using mcam::IMtkcamDeviceCallback;

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
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
ACameraOfflineSession::~ACameraOfflineSession() {
  MY_LOGI("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
ACameraOfflineSession::ACameraOfflineSession(const CreationInfo& info)
    : IACameraOfflineSession(),
      mInstanceId(info.instanceId),
      mLogPrefix(std::to_string(mInstanceId) + "-a-offSess"),
      mLogLevel(0),
      mSession(info.pSession),
      mCallback(nullptr) {
  MY_LOGI("ctor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraOfflineSession::create(const CreationInfo& info)
    -> ACameraOfflineSession* {
  auto pInstance = new ACameraOfflineSession(info);

  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }

  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraOfflineSession::setCallback(
    const std::shared_ptr<IACameraDeviceCallback>& callback) -> void {
  MY_LOGW("Not implement");

  ACameraDeviceCallback::CreationInfo creationInfo = {
      .instanceId = mInstanceId,
      .pCallback = callback,
  };
  auto aCallback = std::shared_ptr<ACameraDeviceCallback>(
      ACameraDeviceCallback::create(creationInfo));
  if (aCallback == nullptr) {
    MY_LOGE("cannot create ACameraDeviceCallback");
  }
  mCallback = aCallback;

  auto pSession = std::shared_ptr<IMtkcamDeviceCallback>(this);
  mSession->setCallback(pSession);

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraOfflineSession::close() -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGW("Not implement, return -EINVAL");
  return -EINVAL;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraOfflineSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& mtkResults) -> void {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  std::vector<CaptureResult> results;
  convertCaptureResults(mtkResults, results, 10);   // TODO(MTK): Stuff.
  mCallback->processCaptureResult(results);

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraOfflineSession::notify(
    const std::vector<mcam::NotifyMsg>& mtkMsgs) -> void {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  std::vector<NotifyMsg> msgs;
  convertNotifyMsgs(mtkMsgs, msgs);
  mCallback->notify(msgs);

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
#if 0  // not implement HAL Buffer Management
auto ACameraOfflineSession::requestStreamBuffers(
    const std::vector<mcam::BufferRequest>& vMtkBufferRequests,
    std::vector<mcam::StreamBufferRet>* pvMtkBufferReturns)
    -> mcam::BufferRequestStatus {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  std::vector<BufferRequest> vBufferRequests;
  std::vector<StreamBufferRet> pvBufferReturns;
  convertBufferRequests(vMtkBufferRequests, vBufferRequests);
  convertStreamBufferRets(*pvMtkBufferReturns, pvBufferReturns);
  BufferRequestStatus status =
      mCallback->requestStreamBuffers(vBufferRequests, &pvBufferReturns);

  return static_cast<mcam::BufferRequestStatus>(status);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ACameraOfflineSession::returnStreamBuffers(
    const std::vector<mcam::StreamBuffer>& mtkBuffers) -> void {
  if (CC_UNLIKELY(mCallback == nullptr)) {
    MY_LOGE("mCallback does not exist");
  }

  std::vector<StreamBuffer> buffers;
  convertStreamBuffers(mtkBuffers, buffers);
  mCallback->returnStreamBuffers(buffers);

  return;
}
#endif

};  // namespace android
};  // namespace mcam
