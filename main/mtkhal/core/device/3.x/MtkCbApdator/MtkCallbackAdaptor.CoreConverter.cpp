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

#include "MtkCallbackAdaptor.h"
#include "MyUtils.h"

#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

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

#define ThisNamespace MtkCallbackAdaptor::CoreConverter

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
static inline IMetadata extractIMetaStreamBuffer(
    android::sp<IMetaStreamBuffer> const& pBuf,
    bool isHal,
    android::String8& log) {
  std::string instanceName = "ACbAdptor::CoreConverter";
  IMetadata out;
  //
  IMetadata* pSrc = pBuf->tryReadLock(instanceName.c_str());
  out = *pSrc;
  pBuf->unlock(instanceName.c_str(), pSrc);
  // debug log
  const bool isPhysical =
      pBuf->getStreamInfo()->getPhysicalCameraId() != -1;
  log += String8::format("[%s%sMeta:count=%u]",
                        isPhysical ? "Phy" : "",
                        isHal ? "Hal" : "",
                        out.count());
  return out;
}

static inline IMetadata extractIMetaStreamBuffers(
    android::Vector<android::sp<IMetaStreamBuffer>> const& pBufs,
    bool isHal,
    android::String8& log) {
  IMetadata out;
  for (auto const& pBuf : pBufs) {
    out += extractIMetaStreamBuffer(pBuf, isHal, log);
  }
  return out;
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
auto ThisNamespace::convertResult(UpdateResultParams const& params,
                                  ICallbackCore::ResultQueue& rResultQueue)
    -> void {
  uint32_t frameNumber = params.requestNo;
  String8 log = String8::format("User %#" PRIxPTR " of frameNumber %u ",
                                params.userId, frameNumber);
  // shutter timestamp
  convertAndFillShutter(params, rResultQueue.vShutterResult, log);
  // error meta
  convertAndFillErrorResult(params, rResultQueue.vMessageResult, log);
  // meta
  convertAndFillMetaResult(params, rResultQueue.vMetaResult, log);
  // hal meta
  convertAndFillHalMetaResult(params, rResultQueue.vHalMetaResult, log);
  // physical meta
  convertAndFillPhysicMetaResult(params, rResultQueue.vPhysicMetaResult, log);
  // input img
  convertAndFillImageResult(frameNumber, params.outputResultImages,
                            rResultQueue.vOutputImageResult, log);
  // output img
  convertAndFillImageResult(frameNumber, params.inputResultImages,
                            rResultQueue.vInputImageResult, log);
  //
  MY_LOGD("%s", log.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillShutter(
    UpdateResultParams const& params,
    std::map<uint32_t, std::shared_ptr<ICallbackCore::ShutterResult>>& rMap,
    String8& log) -> void {
  if (params.shutterTimestamp > 0) {
    std::shared_ptr<ICallbackCore::ShutterResult> pRes =
        std::make_shared<ICallbackCore::ShutterResult>();
    pRes->frameNumber = params.requestNo;
    pRes->timestamp = params.shutterTimestamp;
    pRes->startOfFrame = params.timestampStartOfFrame;
    rMap.insert(std::make_pair(params.requestNo, pRes));
    //
    log += String8::format("[ts:%" PRId64 "]", params.shutterTimestamp);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillErrorResult(
    UpdateResultParams const& params,
    std::map<uint32_t, std::shared_ptr<ErrorMsg>>& rMap,
    String8& log) -> void {
  if (params.hasResultMetaError) {
    std::shared_ptr<ErrorMsg> pRes = std::make_shared<ErrorMsg>();
    pRes->frameNumber = params.requestNo;
    pRes->errorCode = ErrorCode::ERROR_RESULT;
    rMap.insert(std::make_pair(params.requestNo, pRes));
    //
    log += String8::format("(Meta error)");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillMetaResult(
    UpdateResultParams const& params,
    std::map<uint32_t, ICallbackCore::VecMetaResult>& rMap,
    String8& log) -> void {
  uint32_t frameNumber = params.requestNo;
  ICallbackCore::VecMetaResult vRes;
  bool anyUpdate = false;
  //
  IMetadata metadata;
  if (params.resultMeta.size() != 0) {
    anyUpdate = true;
    // update metadata
    metadata =
        extractIMetaStreamBuffers(params.resultMeta, false, log);
    // update Real-Time Partial
    if (params.hasRealTimePartial) {
      vRes.isRealTimePartial = true;
      // add tag in metadata
      if (IMetadata::setEntry<MUINT8>(&vRes.back(),
                                      MTK_HALCORE_ISREALTIME_CALLBACK, 1)) {
        MY_LOGW("failed to setEntry, MTK_HALCORE_ISREALTIME_CALLBACK");
      }
      log += String8::format("(Real-Time)");
    }
  }
  // update Last Partial
  if (params.hasLastPartial) {
    anyUpdate = true;
    vRes.hasLastPartial = true;
    log += String8::format("(Last-Partial)");
  }
  //
  if (anyUpdate) {
    vRes.emplace_back(metadata);
    rMap.insert(std::make_pair(frameNumber, std::move(vRes)));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillHalMetaResult(
    UpdateResultParams const& params,
    std::map<uint32_t, ICallbackCore::VecMetaResult>& rMap,
    String8& log) -> void {
  uint32_t frameNumber = params.requestNo;
  ICallbackCore::VecMetaResult vRes;
  //
  if (params.resultHalMeta.size() != 0) {
    vRes.emplace_back(
        IMetadata(extractIMetaStreamBuffers(params.resultHalMeta, true, log)));
    rMap.insert(std::make_pair(frameNumber, std::move(vRes)));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillPhysicMetaResult(
    UpdateResultParams const& params,
    std::map<uint32_t, ICallbackCore::VecPhysicMetaResult>& rMap,
    String8& log) -> void {
  //
  auto extractPhysicalResult =
      [&](android::Vector<android::sp<PhysicalResult>> const& in,
          ICallbackCore::VecPhysicMetaResult& out, String8& log) -> void {
    for (auto const& res : in) {
      std::string camId = res->physicalCameraId;
      android::sp<IMetaStreamBuffer> pMetaBuf = res->physicalResultMeta;
      //
      std::shared_ptr<PhysicalCameraMetadata> pPhyRes =
          std::make_shared<PhysicalCameraMetadata>();
      pPhyRes->physicalCameraId = std::stoi(camId);
      pPhyRes->metadata = extractIMetaStreamBuffer(pMetaBuf, false, log);
      out.emplace_back(pPhyRes);
    }
  };
  //
  if (params.physicalResult.size() != 0) {
    ICallbackCore::VecPhysicMetaResult vRes;
    extractPhysicalResult(params.physicalResult, vRes, log);
    rMap.insert(std::make_pair(params.requestNo, std::move(vRes)));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertAndFillImageResult(
    uint32_t frameNumber,
    android::Vector<StreamBuffer> const& in,
    std::map<uint32_t, ICallbackCore::VecImageResult>& rMap,
    String8& log) -> void {
  if (in.size() != 0) {
    ICallbackCore::VecImageResult vRes;
    for (auto const& item : in) {
      vRes.emplace_back(std::make_shared<StreamBuffer>(item));
      log += String8::format("[Stream:%02" PRId32 "%s]", item.streamId,
                             item.status == BufferStatus::ERROR ? "(err)" : "");
      rMap.insert(std::make_pair(frameNumber, std::move(vRes)));
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertCallbackParcel(std::list<CallbackParcel>& cblist)
    -> MtkCallbackParcel {
  MtkCallbackParcel hildCbParcel;
  std::unordered_set<uint32_t> ErroFrameNumbers;
  //
  for (auto const& cbParcel : cblist) {
    for (size_t i = 0; i < cbParcel.vError.size(); i++) {
      if (cbParcel.vError[i].errorCode != ErrorCode::ERROR_BUFFER) {
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
      convertHalMetaResult(cbParcel, hildCbParcel.vCaptureResult);
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

    if (MY_UNLIKELY(getLogLevel() >= 1)) {
      Vector<String8> logs;
      convertToDebugString(cbParcel, logs);
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

      rvMsg.push_back(NotifyMsg{.type = MsgType::SHUTTER,
                                .msg = {.shutter = *(cbParcel.shutter)}});
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
    auto const errorStreamId =
        static_cast<int32_t>((rError.stream) ? rError.stream->id : -1);

    CAM_ULOGM_DTAG_BEGIN(
        ATRACE_ENABLED(),
        "Dev-%d:convertError|request:%d =errorCode:%d errorStreamId:%d",
        mCommonInfo->mInstanceId, cbParcel.frameNumber, rError.errorCode,
        errorStreamId);
    CAM_ULOGM_DTAG_END();

    rvMsg.push_back(NotifyMsg{.type = MsgType::ERROR,
                              .msg = {.error = {
                                          .frameNumber = cbParcel.frameNumber,
                                          .errorCode = rError.errorCode,
                                          .errorStreamId = errorStreamId,
                                      }}});
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
    for (size_t i = 0; i < cbParcel.vOutputMetaItem.size(); i++) {
      auto const& rCbMetaItem = cbParcel.vOutputMetaItem[i];
      IMetadata const& rSrcMeta = rCbMetaItem.metadata;
      if (rSrcMeta.count() == 0) {
        MY_LOGW("Src Meta is null");
      } else {
        if (resultBufNo < rCbMetaItem.bufferNo) {
          // update the bufferNo to the last meta item in cbParcel
          resultBufNo = rCbMetaItem.bufferNo;
        }
        resultMeta += rSrcMeta;
      }
    }
    if (!cbParcel.needIgnore) {
      CAM_ULOGM_DTAG_BEGIN(
          ATRACE_ENABLED(),
          "Dev-%d:convertMeta|request:%d =bufferNo:%d count:%u",
          mCommonInfo->mInstanceId, cbParcel.frameNumber, resultBufNo,
          resultMeta.count());

      CaptureResult res = CaptureResult{
          .frameNumber = cbParcel.frameNumber,
          .result = resultMeta,
          .bLastPartialResult =
              resultBufNo == mCommonInfo->mAtMostMetaStreamCount,
      };
      //
      if (res.bLastPartialResult) {
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
          "Dev-%d:convertMeta|request:%d =bufferNo:%d count:%u",
          mCommonInfo->mInstanceId, cbParcel.frameNumber, rCbMetaItem.bufferNo,
          rCbMetaItem.metadata.count());

      CaptureResult res = CaptureResult{
          .frameNumber = cbParcel.frameNumber,
          .result = rCbMetaItem.metadata,
          .bLastPartialResult =
              rCbMetaItem.bufferNo == mCommonInfo->mAtMostMetaStreamCount,
      };
      //
      if (res.bLastPartialResult) {
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
auto ThisNamespace::convertHalMetaResult(CallbackParcel const& cbParcel,
                                         std::vector<CaptureResult>& rvResult)
    -> void {
  CAM_ULOGM_FUNCLIFE();

  // CallbackParcel::vOutputHalMetaItem
  for (size_t i = 0; i < cbParcel.vOutputHalMetaItem.size(); i++) {
    auto const& rCbMetaItem = cbParcel.vOutputHalMetaItem[i];
    //
    if (!cbParcel.needIgnore) {
      CAM_ULOGM_DTAG_BEGIN(
          ATRACE_ENABLED(),
          "Dev-%d:convertMeta|request:%d =bufferNo:%d count:%u",
          mCommonInfo->mInstanceId, cbParcel.frameNumber, rCbMetaItem.bufferNo,
          rCbMetaItem.metadata.count());

      CaptureResult res = CaptureResult{
          .frameNumber = cbParcel.frameNumber,
          .halResult = rCbMetaItem.metadata,
          .bLastPartialResult = false,
      };
      //
      rvResult.push_back(res);
      CAM_ULOGM_DTAG_END();
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/

auto ThisNamespace::convertPhysicMetaResult(
    CallbackParcel const& cbParcel,
    CaptureResult& rvResult) -> void {
  rvResult.physicalCameraMetadata.resize(cbParcel.vOutputPhysicMetaItem.size());
  for (size_t i = 0; i < cbParcel.vOutputPhysicMetaItem.size(); i++) {
    auto const& rCbPhysicMetaItem = cbParcel.vOutputPhysicMetaItem[i];
    PhysicalCameraMetadata physicalCameraMetadata = {
        .physicalCameraId = rCbPhysicMetaItem.physicalCameraId,
        .metadata = rCbPhysicMetaItem.metadata,
    };
    rvResult.physicalCameraMetadata[i] = physicalCameraMetadata;
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
      CallbackParcel::ImageItem const& rCbImageItem =
          cbParcel.vOutputImageItem[i];
      vOutBuffers.push_back(*(rCbImageItem.buffer));
    }
    //
    // Input
    std::vector<StreamBuffer> vInBuffers;
    for (size_t i = 0; i < cbParcel.vInputImageItem.size(); i++) {
      CallbackParcel::ImageItem const& rCbImageItem =
          cbParcel.vInputImageItem[i];
      vInBuffers.push_back(*(rCbImageItem.buffer));
    }
    //
    //
    rvResult.push_back(CaptureResult{
        .frameNumber = cbParcel.frameNumber,
        // .fmqResultSize  = 0,
        // .result
        .outputBuffers = std::move(vOutBuffers),
        .inputBuffers = std::move(vInBuffers),
        .bLastPartialResult = false,
    });
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::convertResult(UpdateResultParams const& params,
                                  MtkCallbackParcel& rMtkCbParcel)
    -> bool {
  android::String8 log;
  uint32_t frameNumber = params.requestNo;
  bool anyUpdate = false;
  // timestamp
  if (params.shutterTimestamp > 0) {
    anyUpdate = true;
    rMtkCbParcel.vNotifyMsg.emplace_back(NotifyMsg{
        .type = MsgType::SHUTTER,
        .msg.shutter.frameNumber = frameNumber,
        .msg.shutter.timestamp = (uint64_t)params.shutterTimestamp,
    });
    log += String8::format("[ts:%" PRId64 "]", params.shutterTimestamp);
  }
  // error msg
  if (params.hasResultMetaError) {
    anyUpdate = true;
    rMtkCbParcel.vErrorMsg.emplace_back(NotifyMsg{
        .type = MsgType::ERROR,
        .msg.error.frameNumber = frameNumber,
        .msg.error.errorCode = ErrorCode::ERROR_RESULT,
    });
    log += String8::format("(Meta error)");
  }
  // hal meta
  if (params.resultHalMeta.size() > 0) {
    anyUpdate = true;
    CaptureResult result;
    result.frameNumber = frameNumber;
    result.halResult =
        extractIMetaStreamBuffers(params.resultHalMeta, true, log);
    rMtkCbParcel.vCaptureResult.emplace_back(result);
  }
  // meta
  if (params.resultMeta.size() > 0 || params.hasLastPartial) {
    anyUpdate = true;
    CaptureResult result;
    result.frameNumber = frameNumber;
    result.result = extractIMetaStreamBuffers(params.resultMeta, false, log);
    if (params.hasRealTimePartial) {
      if (!IMetadata::setEntry<MUINT8>(&result.result,
                                        MTK_HALCORE_ISREALTIME_CALLBACK, 1)) {
        MY_LOGW("failed to setEntry, MTK_HALCORE_ISREALTIME_CALLBACK");
      }
    }
    if (params.hasLastPartial) {
      result.bLastPartialResult = true;
    }
    rMtkCbParcel.vCaptureResult.emplace_back(result);
  }
  // physical meta
  if (params.physicalResult.size() > 0) {
    bool hasPhysicalMetaUpdate = false;
    CaptureResult result;
    result.frameNumber = frameNumber;
    for (auto const& pRes : params.physicalResult) {
      hasPhysicalMetaUpdate = true;
      PhysicalCameraMetadata phyMetadata;
      phyMetadata.physicalCameraId = std::stoi(pRes->physicalCameraId);
      phyMetadata.metadata =
          extractIMetaStreamBuffer(pRes->physicalResultMeta, false, log);
      result.physicalCameraMetadata.emplace_back(phyMetadata);
    }
    if (hasPhysicalMetaUpdate) {
      anyUpdate = true;
      rMtkCbParcel.vCaptureResult.emplace_back(result);
    }
  }
  // image
  if (params.outputResultImages.size() > 0 ||
      params.inputResultImages.size() > 0) {
    anyUpdate = true;
    CaptureResult result;
    result.frameNumber = frameNumber;
    for (auto const& buf : params.outputResultImages) {
      result.outputBuffers.emplace_back(buf);
      log += String8::format("[Stream:%02" PRId32 "%s]", buf.streamId,
                             buf.status == BufferStatus::ERROR ? "(err)" : "");
    }
    for (auto const& buf : params.inputResultImages) {
      result.inputBuffers.emplace_back(buf);
      log += String8::format("[Stream:%02" PRId32 "%s]", buf.streamId,
                             buf.status == BufferStatus::ERROR ? "(err)" : "");
    }
    rMtkCbParcel.vBufferResult.emplace_back(result);
  }
  // debug
  if (anyUpdate) {
    MY_LOGD_IF(getLogLevel(), "%s", log.c_str());
    rMtkCbParcel.vFrameNumber.insert(frameNumber);
  }
  return anyUpdate;
}
};  // namespace core
};  // namespace mcam
