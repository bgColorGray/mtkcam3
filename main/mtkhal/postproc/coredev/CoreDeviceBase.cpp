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
/* MediaTek Inc. (C) 2017. All rights reserved.
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

#define LOG_TAG "MtkCam/CoreDeviceBase"
#include "CoreDeviceBase.h"
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/ULog.h>
#include "CoreDeviceStreamImpl.h"
#include "CoreDeviceWPEImpl.h"
#include "CoreDeviceCaptureImpl.h"

#include <memory>
#include <unordered_map>
#include <vector>

CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_HAL_SERVER);

namespace NSCam {
namespace v3 {
namespace CoreDev {

#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)

#if 1
#define FUNC_START MY_LOGD("+")
#define FUNC_END MY_LOGD("-")
#else
#define FUNC_START
#define FUNC_END
#endif

using std::make_shared;

auto ICoreDev::createCoreDev(CoreDevType type) -> std::shared_ptr<ICoreDev> {
  if (type == STREAM_DEV) {
    int id = 0;
    auto pCorDevice = std::make_shared<model::CoreDeviceStreamImpl>(id);
    return pCorDevice;
  }
  if (type == WPE_DEV) {
    int id = 0;
    auto pCorDevice = std::make_shared<model::CoreDeviceWPEImpl>(id);
    return pCorDevice;
  }
  if (type == CAPTURE_DEV) {
    int id = 0;
    auto pCorDevice = std::make_shared<model::CoreDeviceCaptureImpl>(id);
    return pCorDevice;
  }
  MY_LOGD("unsupport type(0x%X)", (int32_t)type);
  return nullptr;
}

template <class T1, class T2>
void storeInfo(T1* pdst, T2 src) {
  for (auto& info : src) {
    if ((*pdst).find(info->getStreamId()) == (*pdst).end()) {
      (*pdst).emplace(info->getStreamId(), info);
    } else {
      MY_LOGW("the stream (%#" PRIx64 ") is existed in input configure",
              info->getStreamId());
    }
  }
}

CoreDeviceBase::CoreDeviceBase() {}

int32_t CoreDeviceBase::configure(InternalStreamConfig const& config) {
  InternalStreamConfig coreConfig = config;
  {
    std::lock_guard<std::mutex> _lock(mLock);
    for (auto& id : coreConfig.SensorIdSet) {
      std::vector<android::sp<IImageStreamInfo>> vImageInfo;
      std::vector<android::sp<IMetaStreamInfo>> vMetaInfo;
      storeInfo<std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>,
                std::vector<android::sp<IImageStreamInfo>>>(
          &(coreConfig.vInputInfo), vImageInfo);
      storeInfo<std::unordered_map<StreamId_T, android::sp<IMetaStreamInfo>>,
                std::vector<android::sp<IMetaStreamInfo>>>(
          &(coreConfig.vTuningMetaInfo), vMetaInfo);
    }
  }

  {
    std::lock_guard<std::mutex> _lock(mLock);
    mSensorIds = coreConfig.SensorIdSet;
    mpConfigureParam = make_shared<IMetadata>(*(coreConfig.pConfigureParam));
    mvInputInfo = coreConfig.vInputInfo;
    mvOutputInfo = coreConfig.vOutputInfo;
    mvTuningMetaInfo = coreConfig.vTuningMetaInfo;
    mCameraInfoMap = coreConfig.pCameraInfoMap;
  }

  // print debug info
  {
    for (auto it : mvInputInfo) {
      MY_LOGD("the in-img stream (%#" PRIx64 ") is configured", it.first);
    }
    for (auto it : mvOutputInfo) {
      MY_LOGD("the out-img stream (%#" PRIx64 ") is configured", it.first);
    }
    for (auto it : mvTuningMetaInfo) {
      MY_LOGD("the meta stream (%#" PRIx64 ") is configured", it.first);
    }
  }
  int32_t ret = configureCore(coreConfig);
  if (ret) {
    MY_LOGE("configure error!!");
    return -1;
  }
  return 0;
}

vector<std::shared_ptr<RequestData>> CoreDeviceBase::createReqData(
    shared_ptr<RequestParams> param) {
  std::lock_guard<std::mutex> _lock(mLock);
  vector<std::shared_ptr<RequestData>> vOut;
  int32_t i = 0;
  for (auto& inData : param->inData) {
    std::ostringstream oss;
    oss << "r:" << param->requestNo << "-i:" << i;
    std::shared_ptr<RequestData> Out = make_shared<RequestData>(oss.str());
    setRequestData(i, param, Out);
    vOut.push_back(Out);
    i++;
  }
  {
    std::lock_guard<std::mutex> _CBlock(mCBLock);
    mvInflightReqs.emplace(param->requestNo, param);
  }
  return vOut;
}

int32_t CoreDeviceBase::processReq(
    int32_t reqNo,
    vector<std::shared_ptr<RequestData>> reqs) {
  return processReqCore(reqNo, reqs);
}


int32_t CoreDeviceBase::performCallback(ResultData const& result) {
  std::lock_guard<std::mutex> _CBlock(mCBLock);
  if (mvInflightReqs.find(result.requestNo) != mvInflightReqs.end()) {
    shared_ptr<ICoreCallback> CB =
        mvInflightReqs[result.requestNo]->pCallback.lock();
    if (CB != nullptr) {
      CB->onFrameUpdated(result);
    }
    if (result.bCompleted) {
      mvInflightReqs.erase(result.requestNo);
    }
  } else {
    for (auto it : mvInflightReqs) {
      MY_LOGD("inflighting request(%d)", it.first);
    }
  }
  return 0;
}

int32_t CoreDeviceBase::close() {
  int ret = closeCore();
  return ret;
}

int32_t CoreDeviceBase::flush() {
  int ret = flushCore();
  return ret;
}

android::sp<IImageStreamInfo> CoreDeviceBase::getStreamInfo(
    StreamId_T id) const {
  if (mvInputInfo.find(id) != mvInputInfo.end()) {
    return mvInputInfo.at(id);
  }
  if (mvOutputInfo.find(id) != mvOutputInfo.end()) {
    return mvOutputInfo.at(id);
  }
  return nullptr;
}

int32_t CoreDeviceBase::setRequestData(int32_t idx,
                                       shared_ptr<RequestParams> param,
                                       std::shared_ptr<RequestData> outData) {
  outData->mRequestNo = param->requestNo;
  outData->mIndex = idx;
  if (idx == 0) {
    outData->mOutBufs = param->outBufs;
  }
  for (auto sensorData : param->inData[idx]) {
    if (sensorData.tuningBuf == nullptr) {
      if (idx == 0) {
        outData->mpAppControl = make_shared<IMetadata>(
                                      *(param->pAppControl.get()));
      }
      if (sensorData.pAppCameraResult != nullptr) {
        outData->mvAppCameraResult.emplace(
          sensorData.sensorId, sensorData.pAppCameraResult);
      }
      if (sensorData.pHalCameraResult != nullptr) {
        outData->mvPrivateMeta.emplace(
          sensorData.sensorId, sensorData.pHalCameraResult);
      }
      outData->mInBufs = sensorData.inBufs;
      continue;
    }
  }
  return 0;
}

}  // namespace CoreDev
}  // namespace v3
}  // namespace NSCam
