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

#include "ACallbackAdaptor.h"
#include "MyUtils.h"

#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using ::android::Condition;
using ::android::DEAD_OBJECT;
using ::android::Mutex;
using ::android::NO_ERROR;
using ::android::OK;
using ::android::status_t;
using ::android::String8;
using ::android::Vector;
using NSCam::v3::eSTREAMTYPE_IMAGE_OUT;
using NSCam::v3::eSTREAMTYPE_META_IN;
using NSCam::v3::eSTREAMTYPE_META_OUT;

#define ThisNamespace ACallbackAdaptor::CoreConverter

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
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace mcam {
namespace android {
/******************************************************************************
 *
 ******************************************************************************/
static const int kDumpLockRetries = 50;
static const int kDumpLockSleep = 60000;

static bool tryLock(Mutex& mutex) {
  bool locked = false;
  for (int i = 0; i < kDumpLockRetries; ++i) {
    if (mutex.tryLock() == NO_ERROR) {
      locked = true;
      break;
    }
    usleep(kDumpLockSleep);
  }
  return locked;
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::CoreConverter(std::shared_ptr<CommonInfo> pCommonInfo)
    : mInstanceName{std::to_string(pCommonInfo->mInstanceId) +
                    "-CoreConverter"},
      mCommonInfo(pCommonInfo) {
  bEnableMetaMerge = mCommonInfo->coreSetting->enableMetaMerge();
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertResult(
    std::vector<mcam::CaptureResult> const& params,
    ICallbackCore::ResultQueue& resultQueue) -> void {
#define APPEND_MAP(_map, _vec, frameNumber)                     \
  if (resultQueue._map.count(frameNumber) > 0) {                \
    auto& dstVec = resultQueue._map.at(frameNumber);            \
    dstVec.reserve(dstVec.size() + _vec.size());                \
    dstVec.insert(dstVec.end(), _vec.begin(), _vec.end());      \
  } else {                                                      \
    resultQueue._map.insert(std::make_pair(frameNumber, _vec)); \
  }

  for (auto const& result : params) {
    bool anyUpdate = false;
    uint32_t frameNumber = result.frameNumber;
    String8 log = String8::format("frameNumber %u ", frameNumber);
    // CaptureResult.result
    if (result.result.count() > 0) {
      anyUpdate = true;
      ICallbackCore::VecMetaResult vRes;
      vRes.push_back(result.result);
      log += String8::format("[Meta:count=%u]", result.result.count());
      APPEND_MAP(vMetaResult, vRes, frameNumber);
      // last partial
      if (result.bLastPartialResult) {
        resultQueue.vMetaResult.at(frameNumber).hasLastPartial = true;
        log += String8::format("(Last-partial)");
      }
      // real time
      MUINT8 isRealTime = 0;
      IMetadata::getEntry<MUINT8>(&result.result,
                                  MTK_HALCORE_ISREALTIME_CALLBACK, isRealTime);
      if (isRealTime) {
        // set real-time on I/F
        resultQueue.vMetaResult.at(frameNumber).isRealTimePartial = true;
        log += String8::format("(Real-Time)");
      }
    }
    // CaptureResult.physicalCameraMetadata
    if (result.physicalCameraMetadata.size() > 0) {
      anyUpdate = true;
      ICallbackCore::VecPhysicMetaResult vRes;
      for (auto const& phyRes : result.physicalCameraMetadata) {
        vRes.emplace_back(
            std::make_shared<mcam::PhysicalCameraMetadata>(phyRes));
        log +=
            String8::format("[phyMeta(%d):count=%u]", phyRes.physicalCameraId,
                            phyRes.metadata.count());
      }
      APPEND_MAP(vPhysicMetaResult, vRes, frameNumber);
    }
    // CaptureResult.outputBuffers
    if (result.outputBuffers.size() > 0) {
      anyUpdate = true;
      ICallbackCore::VecImageResult vRes;
      for (auto const& buf : result.outputBuffers) {
        vRes.emplace_back(std::make_shared<mcam::StreamBuffer>(buf));
        log += String8::format(
            "[Stream:%02" PRIx64 "%s]", buf.streamId,
            buf.status == mcam::BufferStatus::ERROR ? "(err)" : "");
      }
      APPEND_MAP(vOutputImageResult, vRes, frameNumber);
    }
    // CaptureResult.inputBuffers
    if (result.inputBuffers.size() > 0) {
      anyUpdate = true;
      ICallbackCore::VecImageResult vRes;
      for (auto const& buf : result.inputBuffers) {
        vRes.emplace_back(std::make_shared<mcam::StreamBuffer>(buf));
        log += String8::format(
            "[Stream:%02" PRIx64 "%s]", buf.streamId,
            buf.status == mcam::BufferStatus::ERROR ? "(err)" : "");
      }
      APPEND_MAP(vInputImageResult, vRes, frameNumber);
    }
    //
    MY_LOGD_IF(anyUpdate, "%s", log.c_str());
  }
#undef APPEND_VEC
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertResult(
    std::vector<mcam::NotifyMsg> const& params,
    ICallbackCore::ResultQueue& resultQueue) -> void {
  ::android::String8 log;
  for (auto const& item : params) {
    if (item.type == mcam::MsgType::SHUTTER) {
      convertAndFillShutter(item.msg.shutter, resultQueue.vShutterResult, log);
      appendSensorTimestampMeta(item.msg.shutter, resultQueue.vMetaResult, log);
    } else if (item.type == mcam::MsgType::ERROR) {
      convertAndFillErrorResult(item.msg.error, resultQueue.vMessageResult,
                                log);
    }
  }
  MY_LOGD("%s", log.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillShutter(
    mcam::ShutterMsg const& params,
    std::map<uint32_t, std::shared_ptr<ICallbackCore::ShutterResult>>& rMap,
    ::android::String8& log) -> void {
  uint32_t frameNumber = params.frameNumber;
  std::shared_ptr<ICallbackCore::ShutterResult> pRes =
      std::make_shared<ICallbackCore::ShutterResult>();
  pRes->frameNumber = frameNumber;
  pRes->timestamp = params.timestamp;
  rMap.insert(std::make_pair(frameNumber, pRes));
  log += String8::format("frameNumber %u ", frameNumber);
  log += String8::format("[ts:%" PRId64 "]", pRes->timestamp);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillErrorResult(
    mcam::ErrorMsg const& params,
    std::map<uint32_t, std::shared_ptr<mcam::ErrorMsg>>& rMap,
    ::android::String8& log) -> void {
  if (params.errorCode == mcam::ErrorCode::ERROR_BUFFER) {
    return;
  }
  uint32_t frameNumber = params.frameNumber;
  std::shared_ptr<mcam::ErrorMsg> pRes =
      std::make_shared<mcam::ErrorMsg>(params);
  rMap.insert(std::make_pair(frameNumber, pRes));
  log += String8::format("frameNumber %u ", frameNumber);
  log += String8::format("[err:%s]", toString(pRes->errorCode).c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::appendSensorTimestampMeta(
    mcam::ShutterMsg const& params,
    std::map<uint32_t, ICallbackCore::VecMetaResult>& rMap,
    ::android::String8& log) -> void {
  // append sensor timestamp
  uint32_t frameNumber = params.frameNumber;
  IMetadata metadata_sensorTimestamp;
  IMetadata::IEntry entry(MTK_SENSOR_TIMESTAMP);
  entry.push_back(params.timestamp, NSCam::Type2Type<MINT64>());
  metadata_sensorTimestamp.update(MTK_SENSOR_TIMESTAMP, entry);
  if (rMap.count(frameNumber) > 0) {
    rMap.at(frameNumber).emplace_back(metadata_sensorTimestamp);
  } else {
    ICallbackCore::VecMetaResult vMeta;
    vMeta.emplace_back(metadata_sensorTimestamp);
    rMap.insert(std::make_pair(frameNumber, std::move(vMeta)));
  }
  log += String8::format("[Meta:count=%u]", metadata_sensorTimestamp.count());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertCallbackParcel(std::list<CallbackParcel>& cblist)
    -> ACallbackParcel {
  ACallbackParcel hildCbParcel;
  std::unordered_set<uint32_t> ErroFrameNumbers;
  //
  for (auto const& cbParcel : cblist) {
    for (size_t i = 0; i < cbParcel.vError.size(); i++) {
      if (cbParcel.vError[i].errorCode != mcam::ErrorCode::ERROR_BUFFER) {
        ErroFrameNumbers.insert(cbParcel.frameNumber);
      }
    }
  }
  //
  for (auto const& cbParcel : cblist) {
    CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(), "Dev-%d:convert|request:%d",
                         mCommonInfo->mInstanceId, cbParcel.frameNumber);
    hildCbParcel.vFrameNumber.insert(cbParcel.frameNumber);
    // CallbackParcel::shutter
    convertShutterResult(cbParcel, hildCbParcel.vNotifyMsg);
    //
    // CallbackParcel::vError
    convertErrorResult(cbParcel, hildCbParcel.vErrorMsg);
    //
    // CallbackParcel::vOutputMetaItem
    // block the metadata callbacks which follow the error result
    if (ErroFrameNumbers.count(cbParcel.frameNumber) == 0) {
      //
      if (bEnableMetaMerge == 1) {
        convertMetaResultWithMergeEnabled(cbParcel,
                                          hildCbParcel.vCaptureResult);
      } else {
        convertMetaResult(cbParcel, hildCbParcel.vCaptureResult);
      }
    } else {
      for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
        MY_LOGI(
            "Ignore Metadata callback because error already occured, "
            "frameNumber(%d)/bufferNo(%u)",
            cbParcel.frameNumber, cbParcel.vOutputMetaItem[i].bufferNo);
      }
    }
    //
    // CallbackParcel::vOutputImageItem
    // CallbackParcel::vInputImageItem
    convertImageResult(cbParcel, hildCbParcel.vBufferResult);

    if (CC_UNLIKELY(getLogLevel() >= 1)) {
      Vector<String8> logs;
      mcam::core::convertToDebugString(cbParcel, logs);
      for (auto const& l : logs) {
        MY_LOGD("%s", l.c_str());
      }
    }

    CAM_ULOGM_DTAG_END()
  }

  return hildCbParcel;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertShutterResult(CallbackParcel const& cbParcel,
                                         std::vector<NotifyMsg>& rvMsg)
    -> void {
  CAM_ULOGM_FUNCLIFE();

  // CallbackParcel::shutter
  if (cbParcel.shutter != 0) {
    MY_LOGI_IF(cbParcel.needIgnore,
               "Ignore Shutter callback because error already occured, "
               "frameNumber(%d)/timestamp(%lu)",
               cbParcel.frameNumber, cbParcel.shutter->timestamp);
    if (!cbParcel.needIgnore) {
      CAM_ULOGM_DTAG_BEGIN(
          ATRACE_ENABLED(),
          "Dev-%d:convertShutter|request:%d =timestamp(ns):%" PRId64
          " needIgnore:%d",
          mCommonInfo->mInstanceId, cbParcel.frameNumber,
          cbParcel.shutter->timestamp, cbParcel.needIgnore);
      CAM_ULOGM_DTAG_END();

      NotifyMsg dstMsg;
      mcam::ShutterMsg pMsg = *(cbParcel.shutter);
      convertShutterMsg(pMsg, dstMsg);
      rvMsg.push_back(std::move(dstMsg));
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertErrorResult(CallbackParcel const& cbParcel,
                                       std::vector<NotifyMsg>& rvMsg) -> void {
  CAM_ULOGM_FUNCLIFE();

  // CallbackParcel::vError
  for (size_t i = 0; i < cbParcel.vError.size(); i++) {
    CallbackParcel::Error const& rError = cbParcel.vError[i];
    // CallbackParcel::ImageItem const& rImage = cbParcel.vOutputImageItem[i];
    auto const errorStreamId =
        static_cast<int32_t>((rError.stream) ? rError.stream->id : -1);
    CAM_ULOGM_DTAG_BEGIN(
        ATRACE_ENABLED(),
        "Dev-%d:convertError|request:%d =errorCode:%d errorStreamId:%d",
        mCommonInfo->mInstanceId, cbParcel.frameNumber, rError.errorCode,
        errorStreamId);
    CAM_ULOGM_DTAG_END();

    ErrorCode errorCode = (ErrorCode)(rError.errorCode);
    rvMsg.push_back(NotifyMsg{.type = MsgType::ERROR,
                              .msg = {.error = {
                                          .frameNumber = cbParcel.frameNumber,
                                          .errorCode = errorCode,
                                          .errorStreamId = errorStreamId,
                                      }}});
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertMetaResult(CallbackParcel const& cbParcel,
                                      std::vector<CaptureResult>& rvResult)
    -> void {
  CAM_ULOGM_FUNCLIFE();

  // CallbackParcel::vOutputMetaItem
  for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
    auto const& rCbMetaItem = cbParcel.vOutputMetaItem[i];
    //
    if (!cbParcel.needIgnore) {
      CAM_ULOGM_DTAG_BEGIN(
          ATRACE_ENABLED(),
          "Dev-%d:convertMeta|request:%d =bufferNo:%d count:%zu",
          mCommonInfo->mInstanceId, cbParcel.frameNumber, rCbMetaItem.bufferNo,
          rCbMetaItem.metadata.count());

      CaptureResult res = CaptureResult{
          .frameNumber = cbParcel.frameNumber,
          // .result = rCbMetaItem.metadata,
          .partialResult = rCbMetaItem.bufferNo,
      };
      convertMetadata(rCbMetaItem.metadata, res.result);
      if (CC_UNLIKELY(getLogLevel() >= 3)) {
        MY_LOGI("dump logical metadata of request %" PRIu32,
                cbParcel.frameNumber);
        dumpMetadata(rCbMetaItem.metadata, cbParcel.frameNumber);
      }
      //
      if (res.partialResult) {
        convertPhysicMetaResult(cbParcel, res);
      }
      //
      rvResult.push_back(res);
      CAM_ULOGM_DTAG_END();
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertMetaResultWithMergeEnabled(
    CallbackParcel const& cbParcel,
    std::vector<CaptureResult>& rvResult) -> void {
  CAM_ULOGM_FUNCLIFE();
  if (cbParcel.vOutputMetaItem.size() > 0) {
    IMetadata resultMeta = IMetadata();
    size_t resultBufNo = 0;
    String8 log =
        String8::format("frameNumber %u, bufferNo:", cbParcel.frameNumber);
    // Merge the metadata items
    {
      for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
        auto const& SrcMetaItem = cbParcel.vOutputMetaItem[i];
        MY_LOGI_IF(cbParcel.needIgnore,
                   "Ignore Metadata callback because error already occured, "
                   "frameNumber(%d)/bufferNo(%u)",
                   cbParcel.frameNumber, SrcMetaItem.bufferNo);

        IMetadata const& rSrcMeta = SrcMetaItem.metadata;
        if (rSrcMeta.count() == 0) {
          MY_LOGW("Src Meta is null");
        } else {
          if (resultBufNo < SrcMetaItem.bufferNo) {
            // update the bufferNo to the last meta item in cbParcel
            resultBufNo = SrcMetaItem.bufferNo;
          }
          resultMeta += rSrcMeta;
        }
        log += String8::format(" %d", SrcMetaItem.bufferNo);
      }
    }
    //
    if (!cbParcel.needIgnore) {
      CAM_ULOGM_DTAG_BEGIN(
          ATRACE_ENABLED(), "Dev-%d:convertMeta|request:%d =bufferNo:%lu",
          mCommonInfo->mInstanceId, cbParcel.frameNumber, resultBufNo);
      std::vector<uint8_t> temp;

      CaptureResult res = CaptureResult{
          .frameNumber = cbParcel.frameNumber,
          // .result = resultMeta,
          // .inputBuffers = {.streamId = -1},
          // force assign -1 indicating no input buffer
          .partialResult = (uint32_t)resultBufNo,
      };
      convertMetadata(resultMeta, res.result);
      if (CC_UNLIKELY(getLogLevel() >= 3)) {
        MY_LOGI("dump logical metadata of request %" PRIu32,
                cbParcel.frameNumber);
        dumpMetadata(resultMeta, cbParcel.frameNumber);
      }
      //
      if (res.partialResult) {
        convertPhysicMetaResult(cbParcel, res);
      }
      //
      if (CC_UNLIKELY(getLogLevel() >= 1)) {
        MY_LOGD("%s", log.c_str());
      }
      rvResult.push_back(res);
      CAM_ULOGM_DTAG_END();
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/

auto ThisNamespace::convertPhysicMetaResult(CallbackParcel const& cbParcel,
                                            CaptureResult& rvResult) -> void {
  rvResult.physicalCameraMetadata.resize(cbParcel.vOutputPhysicMetaItem.size());
  for (size_t i = 0; i < cbParcel.vOutputPhysicMetaItem.size(); i++) {
    auto const& rCbPhysicMetaItem = cbParcel.vOutputPhysicMetaItem[i];
    PhysicalCameraMetadata phyMetadata{
        .physicalCameraId = rCbPhysicMetaItem.physicalCameraId,
    };
    convertMetadata(rCbPhysicMetaItem.metadata, phyMetadata.metadata);
    if (CC_UNLIKELY(getLogLevel() >= 3)) {
      MY_LOGI("dump physical metadata of request %" PRIu32,
              cbParcel.frameNumber);
      dumpMetadata(rCbPhysicMetaItem.metadata, cbParcel.frameNumber);
    }
    rvResult.physicalCameraMetadata[i] = phyMetadata;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertImageResult(CallbackParcel const& cbParcel,
                                       std::vector<CaptureResult>& rvResult)
    -> void {
  CAM_ULOGM_FUNCLIFE();

  // CallbackParcel::vOutputImageItem
  // CallbackParcel::vInputImageItem
  //
  if (!cbParcel.vOutputImageItem.isEmpty() ||
      !cbParcel.vInputImageItem.isEmpty()) {
    //
    // Output
    std::vector<StreamBuffer> vOutBuffers;
    for (size_t i = 0; i < cbParcel.vOutputImageItem.size(); i++) {
      StreamBuffer dstBuffer;
      handleAOSPFenceRule(cbParcel.vOutputImageItem[i].buffer);
      convertStreamBuffer(*(cbParcel.vOutputImageItem[i].buffer), dstBuffer);
      vOutBuffers.push_back(std::move(dstBuffer));
    }
    //
    // Input
    StreamBuffer inputBuffer;
    if (cbParcel.vInputImageItem.size() > 0) {
      handleAOSPFenceRule(cbParcel.vInputImageItem[0].buffer);
      convertStreamBuffer(*(cbParcel.vInputImageItem[0].buffer), inputBuffer);
    }
    //
    //
    rvResult.push_back(CaptureResult{
        .frameNumber = cbParcel.frameNumber,
        // .fmqResultSize  = 0,
        .outputBuffers = std::move(vOutBuffers),
        .inputBuffer = std::move(inputBuffer),
        .result = nullptr,
        .partialResult = 0,
    });
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::handleAOSPFenceRule(
    std::shared_ptr<mcam::StreamBuffer> pBuffer) -> void {
  auto& pImgBufHeap = pBuffer->buffer;
  if (pImgBufHeap == nullptr) {
    MY_LOGE("nullptr IImageBuffer pointer");
  }
  if (pBuffer->status == mcam::BufferStatus::ERROR) {
    MINT AF = pImgBufHeap->getAcquireFence();
    MINT dupAF = -1;
    if (AF != -1) {
      dupAF = ::dup(AF);
    }
    pImgBufHeap->setReleaseFence(dupAF);
    pImgBufHeap->setAcquireFence(-1);
  } else {
    pImgBufHeap->setReleaseFence(-1);
    pImgBufHeap->setAcquireFence(-1);
  }
}

};  // namespace android
};  // namespace mcam
