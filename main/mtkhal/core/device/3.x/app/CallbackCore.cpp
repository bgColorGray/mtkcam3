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

#include <main/mtkhal/core/device/3.x/app/CallbackCore.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
#include "MyUtils.h"

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
//
using ::android::Mutex;
using ::android::OK;
using ::android::String8;
using NSCam::IDebuggeeManager;
using NSCam::v3::StreamId_T;
using NSCam::Type2Type;
using NSCam::Utils::ULog::DetailsType;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::ULogPrinter;
using NSCam::v3::STREAM_BUFFER_STATUS;

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
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
const char* CallbackCore::MyDebuggee::mName{"NSCam::v3::ICallbackCore"};
auto CallbackCore::MyDebuggee::debug(::android::Printer& printer,
                                     const std::vector<std::string>& options)
    -> void {
  struct MyPrinter : public IPrinter {
    ::android::Printer* mPrinter;
    explicit MyPrinter(::android::Printer* printer)
      : mPrinter(printer) {}
    ~MyPrinter() {}
    virtual void printLine(const char* string) {
      mPrinter->printLine(string);
    }
    virtual void printFormatLine(const char* format, ...) {
      va_list arglist;
      va_start(arglist, format);
      char* formattedString;
      if (vasprintf(&formattedString, format, arglist) < 0) {
        CAM_ULOGME("Failed to format string");
        va_end(arglist);
        return;
      }
      va_end(arglist);
      printLine(formattedString);
      free(formattedString);
    }
    virtual std::shared_ptr<::android::FdPrinter> getFDPrinter() {
      return nullptr;
    }
  } myPrinter(&printer);
  auto p = mContext.promote();
  if (p != nullptr) {
    p->dumpState(myPrinter, options);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ICallbackCore::create(const CreationInfo& creationInfo)
    -> std::shared_ptr<ICallbackCore> {
  auto pInstance = std::make_shared<CallbackCore>(creationInfo);
  if (!pInstance || !pInstance->initialize()) {
    return nullptr;
  }
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
CallbackCore::CallbackCore(const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>()),
      mInstanceName{creationInfo.userName + "-" +
                    std::to_string(creationInfo.mInstanceId) +
                    ":CallbackCore"} {
  if (creationInfo.mpUser == nullptr || creationInfo.mpfnCallback == nullptr ||
      creationInfo.coreSetting == nullptr) {
    MY_LOGE("mpUser:%p, mpfnCallback:%p, coreSetting:%p", creationInfo.mpUser,
            creationInfo.mpfnCallback, creationInfo.coreSetting.get());
    mCommonInfo = nullptr;
    return;
  }

  if (mCommonInfo != nullptr) {
    mCommonInfo->mLogLevel = creationInfo.mLogLevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    mCommonInfo->mSupportBufferManagement =
        creationInfo.mSupportBufferManagement;
    mCommonInfo->mAtMostMetaStreamCount = creationInfo.mAtMostMetaStreamCount;
    mCommonInfo->mpfnCallback = creationInfo.mpfnCallback;
    mCommonInfo->mpUser = creationInfo.mpUser;
    mCommonInfo->userName = creationInfo.userName;
    mCommonInfo->coreSetting = creationInfo.coreSetting;
  }
  MY_LOGD("User:%s", mCommonInfo->userName.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
bool CallbackCore::initialize() {
  ::android::status_t status = OK;
  //
  if (mCommonInfo == nullptr) {
    MY_LOGE("Bad mCommonInfo");
    return false;
  }
  //
  {
    mBatchHandler = new BatchHandler(mCommonInfo /*, mCallbackHandler*/);
    if (mBatchHandler == nullptr) {
      MY_LOGE("Bad mBatchHandler");
      return false;
    }
    //
    mFrameHandler = new FrameHandler(mCommonInfo, mBatchHandler);
    if (mFrameHandler == nullptr) {
      MY_LOGE("Bad mFrameHandler");
      return false;
    }
    //
    mResultHandler = new ResultHandler(mCommonInfo, mFrameHandler);
    if (mResultHandler == nullptr) {
      MY_LOGE("Bad mResultHandler");
      return false;
    } else {
      const std::string threadName{std::to_string(mCommonInfo->mInstanceId) +
                                   ":CbCore-RstHdl"};
      status = mResultHandler->run(threadName.c_str());
      if (OK != status) {
        MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(),
                status, ::strerror(-status));
        return false;
      }
    }
  }
  //
  mDebuggee = std::make_shared<MyDebuggee>(this);
  if (auto pDbgMgr = IDebuggeeManager::get()) {
    mDebuggee->mCookie = pDbgMgr->attach(mDebuggee, 1);
  }
  //
  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::destroy() -> void {
  auto resLock = mInterfaceLock.timedLock(::ms2ns(1000));
  if (OK != resLock) {
    MY_LOGW("timedLock failed; still go on to destroy");
  }

  mFrameHandler->dumpIfHasInflightRequest();

  mResultHandler->destroy();
  mResultHandler = nullptr;

  mFrameHandler->destroy();
  mFrameHandler = nullptr;

  mBatchHandler->destroy();
  mBatchHandler = nullptr;

  if (mDebuggee != nullptr) {
    if (auto pDbgMgr = IDebuggeeManager::get()) {
      pDbgMgr->detach(mDebuggee->mCookie);
    }
    mDebuggee = nullptr;
  }

  if (OK == resLock) {
    mInterfaceLock.unlock();
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::dumpState(IPrinter& printer __unused,
                             const std::vector<std::string>& options __unused)
    -> void {
  printer.printFormatLine("## App Stream Manager  [%u]\n",
                          mCommonInfo->mInstanceId);

  if (OK == mInterfaceLock.timedLock(::ms2ns(500))) {
    dumpStateLocked(printer, options);

    mInterfaceLock.unlock();
  } else {
    printer.printLine("mInterfaceLock.timedLock timeout");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::dumpStateLocked(IPrinter& printer,
                                   const std::vector<std::string>& options)
    -> void {
  if (mFrameHandler != nullptr) {
    mFrameHandler->dumpState(printer, options);
  }
  if (mBatchHandler != nullptr) {
    printer.printLine(" ");
    mBatchHandler->dumpState(printer, options);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::beginConfiguration(StreamConfiguration const& rStreams)
    -> void {
  Mutex::Autolock _l(mInterfaceLock);
  mFrameHandler->setOperationMode(rStreams.operationMode);
  mFrameHandler->beginConfiguration(rStreams);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::endConfiguration(HalStreamConfiguration const& rHalStreams)
    -> void {
  Mutex::Autolock _l(mInterfaceLock);
  //
  mFrameHandler->endConfiguration(rHalStreams);
  // clear old BatchStreamId & remove Unused Config Stream
  mBatchHandler->resetBatchStreamId();
  for (auto const& item : rHalStreams.streams) {
    auto pStreamInfo = mFrameHandler->getConfigImageStream(item.id);
    if (pStreamInfo == nullptr) {
      MY_LOGE("no image stream info for stream id %" PRId64 " - size:%zu",
              item.id, rHalStreams.streams.size());
      return;
    }
    mBatchHandler->checkStreamUsageforBatchMode(pStreamInfo);
    // MY_LOGD("%s", toString(*pStreamInfo));
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::flushRequest(const std::vector<CaptureRequest>& requests)
    -> void {
  auto prepareBuffer =
      [this](android::Vector<CallbackParcel::ImageItem>& vImageItem,
             StreamBuffer const& buffer) -> void {
    if (-1 != buffer.streamId && 0 != buffer.bufferId) {
      std::shared_ptr<StreamBuffer> pBuffer =
          std::make_shared<StreamBuffer>(buffer);
      pBuffer->status = BufferStatus::ERROR;
      vImageItem.push_back(CallbackParcel::ImageItem{
          .buffer = pBuffer,
          .stream = mFrameHandler->getConfigImageStream(buffer.streamId),
      });
    }
  };

  Mutex::Autolock _l(mInterfaceLock);
  std::list<CallbackParcel> cbList;
  for (auto const& req : requests) {
    CallbackParcel cbParcel = {
        .shutter = nullptr,
        .frameNumber = req.frameNumber,
        .valid = MTRUE,
    };

    //  cbParcel.vError
    cbParcel.vError.push_back(CallbackParcel::Error{
        .errorCode = ErrorCode::ERROR_REQUEST,
    });

    //  cbParcel.vInputImageItem <- req.inputBuffers
    for (auto const& buffer : req.inputBuffers) {
      prepareBuffer(cbParcel.vInputImageItem, buffer);
    }

    //  cbParcel.vOutputImageItem <- req.outputBuffers
    for (auto const& buffer : req.outputBuffers) {
      prepareBuffer(cbParcel.vOutputImageItem, buffer);
    }
    cbList.push_back(std::move(cbParcel));
  }
  mBatchHandler->push(cbList);
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::abortRequest(const std::vector<uint32_t> frameNumbers)
    -> void {
  auto pHandler = mResultHandler;
  ResultQueue vResultQueue;
  String8 log = String8::format("Abort Request, frameNumber:");
  for (auto const frameNumber : frameNumbers) {
    log += String8::format(" %u", frameNumber);
    std::shared_ptr<ErrorMsg> pErrMsg = std::make_shared<ErrorMsg>();
    pErrMsg->frameNumber = frameNumber;
    pErrMsg->errorCode = ErrorCode::ERROR_REQUEST;
    vResultQueue.vMessageResult.insert(std::make_pair(frameNumber, pErrMsg));
  }
  pHandler->enqueResult(vResultQueue);
  MY_LOGW("%s", log.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::submitRequests(
    /*const vector<CaptureRequest>& requests,*/
    const std::vector<CaptureRequest>& rRequests) -> int {
  Mutex::Autolock _l(mInterfaceLock);
  CAM_ULOGM_FUNCLIFE();
  for (const auto& rRequest : rRequests) {
    int err = mFrameHandler->registerFrame(rRequest);
    if (err != OK)
      return err;
  }
  if (rRequests.size() > 1)
    mBatchHandler->registerBatch(rRequests);
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::updateResult(ResultQueue const& param) -> void {
  auto pHandler = mResultHandler;
  if (MY_LIKELY(pHandler != nullptr)) {
    pHandler->enqueResult(param);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::waitUntilDrained(nsecs_t const timeout) -> int {
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
  Mutex::Autolock _l(mInterfaceLock);

  (void)((OK == (err = mFrameHandler->waitUntilDrained(timeout))) &&
         (OK == (err = mBatchHandler->waitUntilDrained(timeoutToWait()))));

  if (OK != err) {
    MY_LOGW("timeout(ns):%" PRId64 " err:%d(%s)", timeout, -err,
            ::strerror(-err));
    struct MyPrinter : public IPrinter {
      std::shared_ptr<::android::FdPrinter> mFdPrinter;
      ULogPrinter mLogPrinter;
      explicit MyPrinter()
          : mFdPrinter(nullptr),
            mLogPrinter(__ULOG_MODULE_ID, LOG_TAG,
                        DetailsType::DETAILS_DEBUG, "[waitUntilDrained] ") {}
      ~MyPrinter() {}
      virtual void printLine(const char* string) {
        mLogPrinter.printLine(string);
      }
      virtual void printFormatLine(const char* format, ...) {
        va_list arglist;
        va_start(arglist, format);
        char* formattedString;
        if (vasprintf(&formattedString, format, arglist) < 0) {
          CAM_ULOGME("Failed to format string");
          va_end(arglist);
          return;
        }
        va_end(arglist);
        printLine(formattedString);
        free(formattedString);
      }
      virtual std::shared_ptr<::android::FdPrinter> getFDPrinter() {
        return mFdPrinter;
      }
    } printer;
    dumpStateLocked(printer, {});
  }

  return err;
}

/******************************************************************************
 *
 ******************************************************************************/
auto CallbackCore::markStreamError(uint32_t frameNumber, StreamId_T streamId)
    -> void {
  Mutex::Autolock _l(mInterfaceLock);
  // if (mFrameHandler)
  // mFrameHandler->updateStreamBuffer(frameNumber, streamId, nullptr);
}

/******************************************************************************
 * CoreSetting
 ******************************************************************************/
CoreSetting::CoreSetting(int64_t setting)
    : mSetting(setting) {
  validateSetting();
  CAM_ULOGMI("UserSetting:0x%X, %s", setting, toString().c_str());
}

void CoreSetting::validateSetting() {
  mSetting &= (eCORE_MTK_RULE_MASK | eCORE_AOSP_RULE_MASK | eCORE_BYPASS);
  if (!enableCallbackCore()) {
    CAM_ULOGMI("bypass mode is ON, change setting : 0x%X -> 0x%X",
               mSetting, eCORE_BYPASS);
    mSetting = eCORE_BYPASS;
  }
}

String8 CoreSetting::toString() {
  String8 log = String8::format("enable:");
  bool first = true;
  if (enableMetaMerge()) {
    log += (first ? "" : "|");
    log += String8::format("MetaMerge");
    first = false;
  }
  if (enableMetaPending()) {
    log += (first ? "" : "|");
    log += String8::format("MetaPending");
    first = false;
  }
  if (enableAOSPMetaRule()) {
    log += (first ? "" : "|");
    log += String8::format("AOSPMetaRule");
    first = false;
  }
  if (!enableCallbackCore()) {
    log += (first ? "" : "|");
    log += String8::format("BypassMode");
    first = false;
  }
  if (first) {
    log += String8::format("none");
  }
  return log;
}

};  // namespace core
};  // namespace mcam
