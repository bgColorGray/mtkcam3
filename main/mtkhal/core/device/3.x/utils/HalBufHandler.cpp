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

#include "HalBufHandler.h"
#include "AppStreamBufferBuilder.h"
#include "MyUtils.h"

#include <utils/Timers.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using ::android::OK;
using ::android::Mutex;
using ::android::UNKNOWN_ERROR;
using ::android::status_t;
using std::vector;
using NSCam::IIonDeviceProvider;

#define ThisNamespace HalBufHandler

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...)                                          \
  do {                                                                        \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,    \
                     ##arg);                                                  \
    mCommonInfo->mDebugPrinter->printFormatLine(                              \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
  } while (0)

#define MY_WARN(level, fmt, arg...)                                           \
  do {                                                                        \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,    \
                     ##arg);                                                  \
    mCommonInfo->mWarningPrinter->printFormatLine(                            \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
  } while (0)

#define MY_ERROR_ULOG(level, fmt, arg...)                                     \
  do {                                                                        \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,    \
                     ##arg);                                                  \
    mCommonInfo->mErrorPrinter->printFormatLine(                              \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                                          \
  do {                                                                        \
    CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__,      \
                   ##arg);                                                    \
    mCommonInfo->mErrorPrinter->printFormatLine(                              \
        #level " [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
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
namespace Utils {
/******************************************************************************
 *
 ******************************************************************************/
auto IHalBufHandler::create(const CreationInfo& creationInfo,
                            android::sp<IAppStreamManager> pAppStreamMgr)
    // android::sp<IAppRequestUtil> pAppRequestUtil
    -> IHalBufHandler* {
  auto pInstance =
      new HalBufHandler(creationInfo, pAppStreamMgr /*, pAppRequestUtil*/);
  if (!pInstance) {
    delete pInstance;
    return nullptr;
  }
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::HalBufHandler(const CreationInfo& creationInfo,
                             android::sp<IAppStreamManager> pAppStreamMgr)
    // android::sp<IAppRequestUtil> pAppRequestUtil
    : mInstanceName{std::to_string(creationInfo.mInstanceId) +
                    "-HalBufHandler"},
      mCommonInfo(std::make_shared<CommonInfo>()),
      mAppStreamMgr(pAppStreamMgr) {
  // , mAppRequestUtil(pAppRequestUtil)
  mBufMinNum =
      ::property_get_int32("vendor.debug.camera.halbuf.num", HAL_BUF_MIN_NUM);
  MY_LOGD("pre-allocate buffer numbers=%d", mBufMinNum);

  if (creationInfo.mErrorPrinter == nullptr ||
      creationInfo.mWarningPrinter == nullptr ||
      creationInfo.mDebugPrinter == nullptr ||
      creationInfo.mCameraDeviceCallback == nullptr ||
      creationInfo.mMetadataProvider == nullptr ||
      creationInfo.mMetadataConverter == nullptr) {
    MY_LOGE(
        "mErrorPrinter:%p mWarningPrinter:%p mDebugPrinter:%p "
        "mCameraDeviceCallback:%p mMetadataProvider:%p mMetadataConverter:%p",
        creationInfo.mErrorPrinter.get(), creationInfo.mWarningPrinter.get(),
        creationInfo.mDebugPrinter.get(),
        creationInfo.mCameraDeviceCallback.get(),
        creationInfo.mMetadataProvider.get(),
        creationInfo.mMetadataConverter.get());
    mCommonInfo = nullptr;
    return;
  }

  if (mCommonInfo != nullptr) {
    int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
    if (loglevel == 0) {
      loglevel =
          ::property_get_int32("vendor.debug.camera.log.AppStreamMgr", 0);
    }
    //
    //
    mCommonInfo->mLogLevel = loglevel;
    mCommonInfo->mInstanceId = creationInfo.mInstanceId;
    mCommonInfo->mErrorPrinter = creationInfo.mErrorPrinter;
    mCommonInfo->mWarningPrinter = creationInfo.mWarningPrinter;
    mCommonInfo->mDebugPrinter = creationInfo.mDebugPrinter;
    mCommonInfo->mDeviceCallback = creationInfo.mCameraDeviceCallback;
    mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
    mCommonInfo->mPhysicalMetadataProviders =
        creationInfo.mPhysicalMetadataProviders;
    mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
  }

  // threadloop
  ::android::status_t status = OK;
  const std::string threadName{std::to_string(mCommonInfo->mInstanceId) +
                               ":HalBufHdl"};
  status = this->run(threadName.c_str());
  if (OK != status) {
    MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(),
            -status, ::strerror(-status));
    return;
  }

  mIonDevice = IIonDeviceProvider::get()->makeIonDevice("HalBufHandler", 0);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {
  mCommonInfo->mDebugPrinter->printLine("[destroy] mHalBufHandler->join +");

  mAppStreamMgr = nullptr;
  // mAppRequestUtil = nullptr;

  this->requestExit();
  this->join();
  mCommonInfo->mDebugPrinter->printLine("[destroy] mHalBufHandler->join -");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer __unused,
                              const std::vector<std::string>& options __unused)
    -> void {
  // TBD
}

/******************************************************************************
 *
 ******************************************************************************/
// Good place to do one-time initializations
auto ThisNamespace::readyToRun() -> status_t {
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::requestExit() -> void {
  MY_LOGD("+ %s", mInstanceName.c_str());
  //
  {
    Mutex::Autolock _l(mBufLock);
    Thread::requestExit();
    mBufCond.broadcast();
  }
  //
  MY_LOGD("- %s", mInstanceName.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
// auto
// ThisNamespace::
// getHalBufManagerStreamProvider() ->
// std::shared_ptr<IImageStreamBufferProvider>
// {
//     class ImageStreamBufferProvider: public IImageStreamBufferProvider
//     {
//         ::android::wp<IAppStreamManager> mwpAppStreamManager;
//         ::android::wp<IAppRequestUtil> mwpAppRequestUtil;
//     public:
//         ImageStreamBufferProvider(
//             ::android::sp<IAppStreamManager> pAppStreamManager,
//             ::android::sp<IAppRequestUtil> pAppRequestUtil
//         )
//         {
//             mwpAppStreamManager = pAppStreamManager;
//             mwpAppRequestUtil   = pAppRequestUtil;
//         }
//         auto requestStreamBuffer( android::sp<IImageStreamBuffer>&
//         rpImageStreamBuffer, IImageStreamBufferProvider::RequestStreamBuffer
//         const& in) -> int
//         {
//             if( auto spAppStreamManager = mwpAppStreamManager.promote() )
//             {
//                 if ( auto spAppRequestUtil = mwpAppRequestUtil.promote() )
//                 {
//                     return requestStreamBuffer(rpImageStreamBuffer, in);
//                 }
//             }
//             return UNKNOWN_ERROR;
//         }
//     };

//     Mutex::Autolock _l(mInterfaceLock);
//     auto provider =
//     std::make_shared<ImageStreamBufferProvider>(mAppStreamMgr,
//     mAppRequestUtil); return provider;
// }

/******************************************************************************
 *
 ******************************************************************************/
// auto
// ThisNamespace::
// waitHalBufCntAvailable(const vector<CaptureRequest>& requests) -> void
// {
//     //    Mutex::Autolock _l(mInterfaceLock);
//     // if(mHalBufHandler)
//         updateHalBufCnt(requests); // update cnt & trigger pre-allocate
// }

/******************************************************************************
 *
 ******************************************************************************/
#warning "not finished yet, different i/f"
auto ThisNamespace::requestStreamBuffer(
    IImageStreamBufferProvider::requestStreamBuffer_cb cb
    __attribute__((unused)),
    IImageStreamBufferProvider::RequestStreamBuffer const& in
    __attribute__((unused))) -> int {
  //    Mutex::Autolock _l(mInterfaceLock);

  // MY_LOGD_IF(mCommonInfo->mLogLevel>=1,
  //         "receive requestNo %d, streamName %s, streamId %" PRIu64,
  //         in.requestNo,
  //         in.pStreamInfo->getStreamName(),
  //         in.pStreamInfo->getStreamId()
  //         );

  // int err = UNKNOWN_ERROR;
  // // rpImageStreamBuffer = nullptr;

  // StreamBuffer rpStreamBuffer;

  // if((err = allocHalBuf(rpStreamBuffer,in)) == OK) {
  //     BuildImageStreamBufferInputParams const params = {
  //         .pStreamInfo        = in.pStreamInfo,
  //         .pIonDevice         = mIonDevice,
  //         .staticMetadata     =
  //         mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics(),
  //         // .bufferName         = "",
  //         .streamBuffer       = rpStreamBuffer,
  //     };
  //     android::sp<AppImageStreamBuffer> appImageStreamBuffer =
  //     makeAppImageStreamBuffer(params);
  //     // IAppRequestUtil::convertStreamBuffer(in.pStreamInfo, mIonDevice,
  //     //     mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics(),
  //     //     rpStreamBuffer, appImageStreamBuffer);
  //     // mAppRequestUtil->convertStreamBuffer(rpStreamBuffer,
  //     appImageStreamBuffer);

  //     //  Wait acquire_fence.
  //     {
  //         auto fence = appImageStreamBuffer->getAcquireFence();
  //         if(fence >=0) {
  //             MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
  //                 "start wait acquire fence"); //TBD: calculate time

  //             sp<IFence> acquire_fence = IFence::create(fence);
  //             MERROR const err = acquire_fence->wait(in.timeout);
  //             MY_LOGD_IF(mCommonInfo->mLogLevel>=2,
  //                 "end wait acquire fence. err %d", (int)err); //TBD:
  //                 calculate time
  //             appImageStreamBuffer->setAcquireFence(-1);
  //         }
  //     }

  //     // rpImageStreamBuffer = appImageStreamBuffer;

  // }
  // else {
  //     //auto pStreamInfo = (IImageStreamInfo*)(in.pStreamInfo);
  //     //rpImageStreamBuffer = AppErrorImageStreamBuffer::Allocator()(
  //      //                           0, pStreamInfo, -1);
  //     MY_LOGW("return AppErrorImageStreamBuffer");
  // }
  // // TODO : need interface
  // mAppStreamMgr->updateStreamBuffer(in.requestNo,
  // in.pStreamInfo->getStreamId(), rpImageStreamBuffer);

  // // MY_LOGD_IF(mCommonInfo->mLogLevel>=1,
  // //     "err %d,  rpImageStreamBuffer %p", err, rpImageStreamBuffer.get());

  // return err;
  return OK;
}

/******************************************************************************
 * _allocHalBuf - Internal function
 *      to allocate buffers from HIDL interface and push_back to poolSP
 *
 * Return:
 *      >0 numbers of buffers be allocated
 *      =0 no buffers need retry later
 *      <0 some error happen user should mark this request as FAIL
 * MUST LOCK: mBufLock
 ******************************************************************************/
#warning "not finished yet"
auto ThisNamespace::_allocHalBuf(StreamId_T streamId __unused,
                                 int num __unused,
                                 HalBufPoolSP poolSP __unused) -> int {
    return 0;
//  Mutex::Autolock _l1(mCntLock);
//
//  MY_LOGD_IF(getLogLevel() >= 1, "streamId %d, num %d", (int)streamId, num);
//
//  // wait buffer usage < max buffer
//  while (true) {
//    auto maxCnt = _getHalBufMaxCnt(streamId);
//    if (maxCnt == 0) {
//      // ERROR : no max info, return error
//      MY_LOGW("can't find the max buf num of streamId %d", (int)streamId);
//      return -1;
//    }
//
//    auto usedCnt = _getHalBufAllocCnt(streamId);
//    auto freeCnt = maxCnt - usedCnt;
//    if (freeCnt > 0) {               // still have available count
//      num = std::min(freeCnt, num);  // alloc num must <= freeCnt
//      break;
//    }
//
//    MY_LOGW(
//        "Allocate failed. Reach max numers. streamId %d, usedCnt %d, maxCnt %d",
//        (int)streamId, usedCnt, maxCnt);
//    return -1;
//  }
//
//  // prepare HIDL call requestStreamBuffers
//  BufferRequestStatus status;
//  // only implement 1 reuqest for 1 streamId with many buffers
//  vector<BufferRequest> mHalBufferReqs(1);
//  vector<StreamBufferRet> bufRets;
//
//  mHalBufferReqs[0].streamId = streamId;
//  mHalBufferReqs[0].numBuffersRequested = num;
//
//  status = mCommonInfo->mDeviceCallback->requestStreamBuffers(mHalBufferReqs,
//                                                              &bufRets);
//
//  // auto err =
//  // mCommonInfo->mDeviceCallback->requestStreamBuffers(mHalBufferReqs,
//  //     [&status, &bufRets] (
//  //         BufferRequestStatus s,
//  //         const vector<StreamBufferRet>& rets) {
//  //             status = s;
//  //             bufRets = std::move(rets);
//  //         });
//
//  // if (!err.isOk()) {
//  //     MY_LOGE("Transaction error: %s", err.description().c_str());
//  //     return -1;
//  // }
//
//  switch (status) {
//    case BufferRequestStatus::FAILED_CONFIGURING:
//      /**
//       * Method call failed for all streams and no buffers are returned at all.
//       * Camera service is about to or is performing configureStreams. HAL must
//       * wait until next configureStreams call is finished before requesting
//       * buffers again.
//       */
//      MY_LOGE("FAILED_CONFIGURING");
//      return -1;
//    case BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS:
//      /**
//       * Method call failed for all streams and no buffers are returned at all.
//       * Failure due to bad BufferRequest input, eg: unknown streamId or
//       * repeated streamId.
//       */
//      MY_LOGE("FAILED_ILLEGAL_ARGUMENTS");
//      return -1;
//    default:
//      break;  // Other status Handled by following code
//  }
//
//  if (status != BufferRequestStatus::STATUS_OK &&
//      status != BufferRequestStatus::FAILED_PARTIAL &&
//      status != BufferRequestStatus::FAILED_UNKNOWN) {
//    MY_LOGE("unknown buffer request error code %d", status);
//    return -1;
//  }
//
//  // Only OK, FAILED_PARTIAL and FAILED_UNKNOWN reaches here
//  if (bufRets.size() != mHalBufferReqs.size()) {
//    MY_LOGE("expect %zu buffer requests returned, only got %zu",
//            mHalBufferReqs.size(), bufRets.size());
//    return -1;
//  }
//
//  // Handle failed streams
//  size_t i = 0;
//  // for (size_t i = 0; i < bufRets.size(); i++)  // assume here only 1 request
//  {
//    if (bufRets[i].val.error != StreamBufferRequestError::NO_ERROR) {
//      switch (bufRets[i].val.error) {
//        case StreamBufferRequestError::NO_BUFFER_AVAILABLE:
//          MY_LOGE("NO_BUFFER_AVAILABLE");
//          return -1;
//        case StreamBufferRequestError::MAX_BUFFER_EXCEEDED:
//          MY_LOGE("MAX_BUFFER_EXCEEDED");
//          return 0;  // for notify retry
//        case StreamBufferRequestError::STREAM_DISCONNECTED:
//          MY_LOGE("STREAM_DISCONNECTED");
//          return -1;
//        case StreamBufferRequestError::UNKNOWN_ERROR:
//          MY_LOGE("UNKNOWN_ERROR");
//          return -1;
//        default:
//          MY_LOGE("Unknown StreamBufferRequestError %d", bufRets[i].val.error);
//          return -1;
//      }
//    }
//  }
//
//  if (status == BufferRequestStatus::FAILED_UNKNOWN) {
//    MY_LOGE("FAILED_UNKNOWN");
//    return -1;
//  }
//
//  // Only BufferRequestStatus::STATUS_OK and BufferRequestStatus::FAILED_PARTIAL
//  // reaches here
//  const vector<StreamBuffer>& hBufs = std::move(bufRets[0].val.buffers);
//
//  if (hBufs.size() < 1) {
//    MY_LOGE("expect 1 buffer returned, got %zu!", hBufs.size());
//    return UNKNOWN_ERROR;
//  }
//
//  auto bufSize = static_cast<int>(hBufs.size());
//  for (int i = 0; i < bufSize; i++) {
//    MY_LOGD_IF(getLogLevel() >= 1,
//               "push_back bufferId: %d, buffer %p, acquireFence %p",
//               static_cast<int>(hBufs[i].bufferId), hBufs[i].buffer,
//               hBufs[i].acquireFenceOp.hdl);
//    poolSP->push_back(std::make_shared<HalBuf>(hBufs[i]));
//  }
//
//  // increase alloc counter
//  _incHalBufAllocCnt(streamId, bufSize);
//
//  return bufSize;
}

/******************************************************************************
 * getHalBufMaxCnt - internal function
 *      return the steam of max buf num
 ******************************************************************************/
auto ThisNamespace::_getHalBufMaxCnt(StreamId_T streamId) -> int {
  auto it = mCntMaxMap.find(streamId);
  if (it == mCntMaxMap.end())
    return 0;
  else
    return it->second;
}
/******************************************************************************
 * setHalBufMaxCnt
 *      record max buf num of each stream
 ******************************************************************************/
auto ThisNamespace::setHalBufMaxCnt(
    const HalStreamConfiguration& halConfiguration) -> void {
  Mutex::Autolock _l1(mCntLock);
  for (size_t i = 0; i < halConfiguration.streams.size(); i++) {
    auto& halStream = halConfiguration.streams[i];
    auto streamId = halStream.id;
    auto maxBufNum = halStream.maxBuffers;

    // create max map if need
    {
      auto it = mCntMaxMap.find(streamId);
      if (it == mCntMaxMap.end())
        mCntMaxMap.insert({streamId, maxBufNum});
      else
        it->second = maxBufNum;
    }
    MY_LOGD("streamId %d, maxBufNum %d", (int)streamId, (int)maxBufNum);
  }
}
/******************************************************************************
 * incHalBufAllocCnt
 *      increase counter of real allocate from HIDL
 *      call when allocate one hal buffer from HIDL
 ******************************************************************************/
auto ThisNamespace::_incHalBufAllocCnt(StreamId_T streamId, int cnt) -> int {
  auto it = mCntAllocMap.find(streamId);
  if (it == mCntAllocMap.end()) {
    mCntAllocMap.insert({streamId, cnt});
    MY_LOGD_IF(getLogLevel() >= 1, "streamId %d, buf %d+%d", (int)streamId,
               (int)0, (int)cnt);
    return cnt;
  } else {
    MY_LOGD_IF(getLogLevel() >= 1, "streamId %d, buf %d+%d", (int)streamId,
               (int)it->second, (int)cnt);
    it->second += cnt;
    return it->second;
  }
}
/******************************************************************************
 * _decHalBufAllocCnt
 *      decrease alloc counter.
 * return remind alloc counter
 ******************************************************************************/
auto ThisNamespace::_decHalBufAllocCnt(StreamId_T streamId, int cnt) -> int {
  auto it = mCntAllocMap.find(streamId);
  if (it == mCntAllocMap.end()) {
    MY_LOGE("stream %d not exists in mCntAllocMap", (int)streamId);
    return 0;
  }
  if (it->second <= 0) {
    MY_LOGE("stream %d in mCntAllocMap, or count %d <=0 , decCnt %d!",
            (int)streamId, it->second, cnt);
    return 0;
  }

  it->second -= cnt;
  return it->second;
}
/******************************************************************************
 * _getHalBufAllocCnt - internal function
 *      return the number of allocated counter
 ******************************************************************************/
inline auto ThisNamespace::_getHalBufAllocCnt(StreamId_T streamId) -> int {
  auto it = mCntAllocMap.find(streamId);
  if (it == mCntAllocMap.end())
    return 0;
  else
    return it->second;
}

/******************************************************************************
 * _incHalBufInflightCnt
 *      increase inflight count for this streamId
 ******************************************************************************/
auto ThisNamespace::_incHalBufInflightCnt(StreamId_T streamId, int cnt) -> int {
  auto it = mCntInflightMap.find(streamId);
  if (it == mCntInflightMap.end()) {
    mCntInflightMap.insert({streamId, cnt});
    MY_LOGD_IF(getLogLevel() >= 1, "streamId %d, buf %d+%d", (int)streamId,
               (int)0, (int)cnt);
    return cnt;
  } else {
    MY_LOGD_IF(getLogLevel() >= 1, "streamId %d, buf %d+%d", (int)streamId,
               (int)it->second, (int)cnt);
    it->second += cnt;
    return it->second;
  }
}
/******************************************************************************
 * _decHalBufInflightCnt
 *      decrease Inflight counter.
 * return remind Inflight counter
 ******************************************************************************/
auto ThisNamespace::_decHalBufInflightCnt(StreamId_T streamId, int cnt) -> int {
  auto it = mCntInflightMap.find(streamId);
  if (it == mCntInflightMap.end()) {
    MY_LOGE("stream %d not exists in mCntInflightMap", (int)streamId);
    return 0;
  }
  if (it->second <= 0) {
    MY_LOGE("stream %d in mCntInflightMap, or count %d <=0 , decCnt %d!",
            (int)streamId, it->second, cnt);
    return 0;
  }

  MY_LOGD_IF(getLogLevel() >= 1, "streamId %d, buf %d-%d", (int)streamId,
             (int)it->second, (int)cnt);
  it->second -= cnt;
  return it->second;
}
/******************************************************************************
 * _getHalBufInflightCnt - internal function
 *      return the number of Inflightated counter
 ******************************************************************************/
inline auto ThisNamespace::_getHalBufInflightCnt(StreamId_T streamId) -> int {
  auto it = mCntInflightMap.find(streamId);
  if (it == mCntInflightMap.end())
    return 0;
  else
    return it->second;
}

/******************************************************************************
 * updateHalBufCnt
 *      record the request hal buffer expect numbers
 *      Accumlative buffer numbers should <= max of each stream
 ******************************************************************************/
auto ThisNamespace::updateHalBufCnt(const vector<CaptureRequest>& requests)
    -> void {
  bool isHalBufRequest = false;

  // update counting
  {
    Mutex::Autolock _l1(mCntLock);

    for (int i = 0; i < requests.size(); i++) {
      // auto frameNo = requests[i].frameNumber;
      for (const auto& buffer : requests[i].outputBuffers) {
        auto streamId = buffer.streamId;
        auto bufferId = buffer.bufferId;

        if (bufferId == 0) {  // bufferId means it use hal buf feature
          if (mBufPoolMap.find(streamId) == mBufPoolMap.end()) {
            MY_LOGD_IF(getLogLevel() >= 1, "register stream %d to mBufPoolMap",
                       (int)streamId);
            // create empty pool
            mBufPoolMap.insert({streamId, std::make_shared<HalBufPool>()});
          }
          isHalBufRequest = true;
        }

        // increase inflight count no matter halbuf used or not
        _incHalBufInflightCnt(streamId, 1);
      }
    }
  }

  // trigger pre-allocate hal buf when receive request
  if (isHalBufRequest) {
    MY_LOGD("trigger allocate");
    mBufCond.broadcast();
  }
}
/******************************************************************************
 * updateHalBufCnt
 *      according performCallback results to
 *      1. decrease request counter.
 *      2. decrease alloc counter
 ******************************************************************************/
auto ThisNamespace::updateHalBufCnt(const vector<CaptureResult>& vBufferResult)
    -> void {
  bool isNoInflight = false;

  // update counting
  {
    Mutex::Autolock _l1(mCntLock);

    for (auto& result : vBufferResult) {
      for (auto& buf : result.outputBuffers) {
        auto streamId = buf.streamId;
        auto bufferId = buf.bufferId;
        auto status = buf.status;

        MY_LOGD_IF(getLogLevel() >= 1,
                   "Result requestNo %d, streamId %d, bufferId %d, status %d",
                   result.frameNumber, (int)streamId, (int)bufferId, status);

        if (bufferId > 0) {
          _decHalBufAllocCnt(streamId, 1);
        }

        // decrease inflight count no matter halbuf used or not
        if (_decHalBufInflightCnt(streamId, 1) == 0) {
          isNoInflight = true;
        }
      }
    }
  }

  // trigger release hal buf when no inflight request
  if (isNoInflight) {
    MY_LOGD("trigger release");
    mBufCond.broadcast();
  }
}

/******************************************************************************
 * _dequeueHalBufPool - internal function
 *      return HalBuf from Pool if exists
 *      Or blocking & allocate HALBuffer directly
 *      It will wakeup pre-allocate thread if buffer not enough
 ******************************************************************************/
#warning "not finished yet"
auto ThisNamespace::_dequeueHalBufPool(StreamId_T streamId __unused,
                                       HalBufSP& rpStreamBufferSP __unused)
    -> bool {
    return true;
//  Mutex::Autolock _l1(mBufLock);
//  auto it = mBufPoolMap.find(streamId);
//  HalBufPoolSP poolSP;
//  if (it == mBufPoolMap.end()) {
//    // create empty pool
//    poolSP = std::make_shared<HalBufPool>();
//    mBufPoolMap.insert({streamId, poolSP});
//  } else {
//    poolSP = it->second;
//  }
//
//  // Immidate allocate buffer from HIDL
//  if (poolSP->size() == 0) {
//    MY_LOGD_IF(getLogLevel() >= 1,
//               "direct allocate due to pool already empty. streamId %d",
//               (int)streamId);
//
//    auto num = _allocHalBuf(streamId, mBufMinNum + 1, poolSP);
//    MY_LOGD_IF(getLogLevel() >= 1, "_allocHalBuf allocated buf num %d", num);
//    if (num <= 0) {
//      return false;  // can't allocate buffer from HIDL
//    }
//  }
//
//  // Get buffer from pool front
//  rpStreamBufferSP = poolSP->front();
//  MY_LOGD_IF(getLogLevel() >= 1, "pop_front bufferId: %d, buffer %p",
//             (int)rpStreamBufferSP->bufferId, rpStreamBufferSP->buffer);
//
//  poolSP->pop_front();
//
//  // trigger thread to prepare next buffer
//  if (poolSP->size() < mBufMinNum) {
//    MY_LOGD_IF(getLogLevel() >= 1,
//               "Trigger pre-allocate pool size() %d, mBufMinNum %d",
//               (int)poolSP->size(), (int)mBufMinNum);
//    mBufCond.broadcast();
//  }
//  return true;
}

/******************************************************************************
 * allocHalBuf
 *      dequeue HalBuf from Pool
 ******************************************************************************/
#warning "not finished yet"
auto ThisNamespace::allocHalBuf(
    HalBufSP& rpStreamBufferSP __unused,
    IImageStreamBufferProvider::RequestStreamBuffer const& in __unused)
    -> int {
    return 0;
//  MY_LOGD_IF(
//      getLogLevel() >= 1,
//      "requestNo %d, timeout %d, streamName %s, streamId %d, streamType %d +",
//      in.requestNo, (int)in.timeout, in.pStreamInfo->getStreamName(),
//      (int)in.pStreamInfo->getStreamId(), in.pStreamInfo->getStreamType());
//
//  nsecs_t current = ::systemTime();
//  if (false ==
//      _dequeueHalBufPool(in.pStreamInfo->getStreamId(), rpStreamBufferSP)) {
//    MY_LOGE("_dequeueHalBufPool fail");
//    return UNKNOWN_ERROR;
//  }
//
//  if (rpStreamBufferSP->buffer == nullptr) {
//    auto it = mHalBufImportMap.find(
//        std::make_pair(rpStreamBufferSP->streamId, rpStreamBufferSP->bufferId));
//    if (it != mHalBufImportMap.end()) {
//      rpStreamBufferSP = it->second;
//      MY_LOGD("replace with mHalBufImportMap streamId %d, bufferId %d, buf %p",
//              (int)rpStreamBufferSP->streamId, (int)rpStreamBufferSP->bufferId,
//              rpStreamBufferSP->buffer);
//      mHalBufImportMap.erase(it);
//    }
//  }
//  auto t = ((::systemTime() - current) / 1000000L);
//  MY_LOGD_IF(getLogLevel() >= 1 || t >= 10,
//             "requestNo %d, timeout %d, streamName %s, streamId %d, streamType "
//             "%d time %d ms",
//             in.requestNo, static_cast<int>(in.timeout),
//             in.pStreamInfo->getStreamName(),
//             static_cast<int>(in.pStreamInfo->getStreamId()),
//             in.pStreamInfo->getStreamType(), static_cast<int>(t));
//
//  return OK;
}
/******************************************************************************
 * _releaseAllHalBuf
 *      release all pre-allocate buffer
 *
 * MUST LOCK: mBufLock
 ******************************************************************************/
#warning "not finished yet"
auto ThisNamespace::_releaseAllHalBuf(StreamId_T streamId __unused,
                                      HalBufPoolSP poolSP __unused)
    -> void {
//  // Pre-allocate bufferId & handle will be reuse between different
//  // configuration. we can't release those buffer during flush/waitUntilDrain
//  // stage i.e. bufferId will assume we keep last handle in previous session
//  //      this session allocation will only get bufferId with null handle
//  //
//  // Another race condition will be pre-allocate thread may run after
//  // waitUntilDrain and cause some buffer not return.
//  //
//  //
//
//  // total number of buffers to free
//  uint32_t numBuffers = (uint32_t)poolSP->size();
//
//  MY_LOGD("releaseStreamBuffers streamId %d, %d buffers +", (int)streamId,
//          (int)numBuffers);
//  if (numBuffers > 0) {
//    // reduce allocate count
//    {
//      Mutex::Autolock _l1(mCntLock);
//      _decHalBufAllocCnt(streamId, static_cast<int>(numBuffers));
//    }
//
//    // collect all buffers
//    std::vector<StreamBuffer> hBufs(numBuffers);
//    size_t i = 0;
//    for (auto bufSP : *poolSP) {
//      hBufs[i].streamId = bufSP->streamId;
//      hBufs[i].buffer = nullptr;  // use bufferId
//      hBufs[i].bufferId = bufSP->bufferId;
//      if (hBufs[i].bufferId == 0) {
//        MY_LOGE("bufferId is 0 in stream %d", hBufs[i].streamId);
//      }
//      // ERROR since the buffer is not for application to consume
//      hBufs[i].status = BufferStatus::ERROR;
//      // FIXME
//      if (bufSP->releaseFenceOp.hdl) {
//        MY_LOGW("streamId %d, bufferId %d, releaseFence %p exists!",
//                (int)bufSP->streamId, (int)bufSP->bufferId,
//                bufSP->releaseFenceOp.hdl);
//      }
//      i++;
//
//      //
//      if (bufSP->buffer) {
//        MY_LOGD("mHalBufImportMap insert streamId %d, bufferId %d, buf %p",
//                (int)bufSP->streamId, (int)bufSP->bufferId, bufSP->buffer);
//        mHalBufImportMap.insert_or_assign(
//            std::make_pair(bufSP->streamId, bufSP->bufferId), bufSP);
//      }
//    }
//
//    mCommonInfo->mDeviceCallback->returnStreamBuffers(hBufs);
//    // if (!ret.isOk()) {
//    //     MY_LOGE("Transaction error in mDeviceCallback->returnStreamBuffers:
//    //     %s", ret.description().c_str());
//    // }
//    poolSP->clear();
//  }
//
//  MY_LOGD("releaseStreamBuffers -");
}
/******************************************************************************
 * waitUntilDrained
 *      return all ununsed allocaed Hal Buffers back to Android
 ******************************************************************************/
auto ThisNamespace::waitUntilDrained(nsecs_t const timeout) -> int {
  (void)timeout;  // UNUSED parameters

  MY_LOGD("+");
  {  // release all pre-allocate buffers
    Mutex::Autolock _l1(mBufLock);

    for (auto it = mBufPoolMap.begin(); it != mBufPoolMap.end(); it++) {
      auto streamId = it->first;
      auto poolSP = it->second;
      _releaseAllHalBuf(streamId, poolSP);
    }
    mBufPoolMap.clear();
  }
  MY_LOGD("-");
  return OK;
}

/******************************************************************************
 * threadLoop
 *      wait signal and try to keep buffer numbers >= mBufMinNum
 ******************************************************************************/
auto ThisNamespace::threadLoop() -> bool {
  Mutex::Autolock _l1(mBufLock);

  MY_LOGD("+");
  while (!exitPending()) {
    mBufCond.wait(mBufLock);

    // check pre-allocate buffer numbers
    for (auto it = mBufPoolMap.begin(); it != mBufPoolMap.end(); it++) {
      auto streamId = it->first;
      auto poolSP = it->second;
      int inflightCnt = 0;
      {
        Mutex::Autolock _l1(mCntLock);
        inflightCnt = _getHalBufInflightCnt(streamId);
      }

      auto poolSize = poolSP->size();
      if (inflightCnt > 0 && poolSize < mBufMinNum) {
        // pre-allocate only when inflight request exists
        auto ret = _allocHalBuf(streamId, mBufMinNum, poolSP);
        MY_LOGD_IF(
            getLogLevel() >= 1,
            "_allocHalBuf(streamId %d, mBufMinNum %d, inflightCnt %d) ret %d",
            (int)streamId, (int)mBufMinNum, inflightCnt, (int)ret);
      } else if (inflightCnt == 0 && poolSize > 0) {
        // release pre-allocate buffer if no inflight request needed
        MY_LOGD_IF(
            getLogLevel() >= 1,
            "_releaseAllHalBuf(streamId %d, buff num %d, inflightCnt %d)",
            (int)streamId, (int)poolSize, inflightCnt);
        _releaseAllHalBuf(streamId, poolSP);
      }
    }
  }
  MY_LOGD("-");
  return true;
}
};  // namespace Utils
};  // namespace core
};  // namespace mcam
