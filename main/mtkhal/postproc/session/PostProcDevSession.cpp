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

#define LOG_TAG "MTKHAL/PostProcDevSessionImp"

#include "../include/PostProcDevSession.h"

#include <map>
#include <mutex>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "mtkcam/utils/metastore/IMetadataProvider.h"
#include "mtkcam/drv/IHalSensor.h"
#include "mtkcam3/pipeline/hwnode/StreamId.h"
#include "mtkcam3/main/mtkhal/coredev/ICoreCallback.h"
#include "mtkcam3/main/mtkhal/coredev/ICoreDev.h"
#include "mtkcam3/main/mtkhal/coredev/types.h"
#include "mtkcam3/pipeline/utils/streaminfo/ImageStreamInfo.h"
#include "mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h"
#include "mtkcam/def/ImageFormat.h"
#include "mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h"
#include "mtkcam/utils/metadata/IMetadata.h"
#include "mtkcam/utils/metadata/client/mtk_metadata_tag.h"
#include "mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h"
#include "mtkcam/utils/std/Log.h"
#include "mtkcam/utils/std/Trace.h"
#include "mtkcam/utils/std/ULog.h"


CAM_ULOG_DECLARE_MODULE_ID(MOD_POSTPROC_DEVICE);

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
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)

#if 1
#define FUNC_START MY_LOGD("+")
#define FUNC_END MY_LOGD("-")
#else
#define FUNC_START
#define FUNC_END
#endif

using NSCam::v3::CoreDev::RequestParams;
using NSCam::v3::CoreDev::ResultData;
using NSCam::v3::CoreDev::ICoreCallback;
using NSCam::v3::CoreDev::CoreDevType;
using NSCam::v3::CoreDev::ICoreDev;
using NSCam::v3::CoreDev::InternalStreamConfig;
using NSCam::v3::CoreDev::RequestData;
using NSCam::v3::CoreDev::SensorData;
using NSCam::v3::CoreDev::RegisteredBuf;
using NSCam::v3::CoreDev::RequestBuf;
using NSCam::v3::CoreDev::RegisteredData;
using NSCam::v3::IImageStreamInfo;
using mcam::IImageBufferHeap;

using MetaStreamInfo = NSCam::v3::Utils::MetaStreamInfo;


/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {

class PostProcDevSessionImp : public PostProcDevSession {
 public:  //// Definitions.
  explicit PostProcDevSessionImp(
              int32_t type,
              const std::shared_ptr<IMtkcamDeviceCallback>& callback);
  virtual ~PostProcDevSessionImp();

 public:
  /**
   * configureStreams
   */
  auto  configureStreams(
                  const StreamConfiguration& requestedConfiguration,
                  HalStreamConfiguration& halConfiguration)  // output
                  -> int override;

  /**
   * processCaptureRequest
   */
  auto  processCaptureRequest(
                  const std::vector<CaptureRequest>& requests,
                  uint32_t& numRequestProcessed)  // output
                  -> int override;

  auto  flush() -> int override;

  auto  close() -> int override;

 private:
  auto processSingleRequestLocked(const CaptureRequest& request) -> int;

 private:
  std::mutex mLock;
  int32_t mType;
  std::shared_ptr<ICoreCallback> mCallback = nullptr;
  std::shared_ptr<ICoreDev> mCoreDev = nullptr;
  std::map<int64_t, mcam::Stream> mCnfiguredStreams;
  std::vector<int32_t> mSensorIdSet;
};

class CallbackHandler : public ICoreCallback {
 public:
  explicit CallbackHandler(const std::shared_ptr<IMtkcamDeviceCallback>& cb);
  virtual ~CallbackHandler();
  auto onFrameUpdated(ResultData const& params) -> void override;

  auto registerReq(
                 uint32_t const req,
                 std::vector<RegisteredBuf> const& bufs) -> void override;

  auto registerMtkHalReq(uint32_t const req,
                           RegisteredData const& data) -> void override;

  auto waitForReqDrained(int32_t const operation) -> void override;

 private:
  std::mutex mLock;
  std::condition_variable mWaitCond;
  std::weak_ptr<IMtkcamDeviceCallback> mwpCallback;
  std::map<int32_t, RegisteredData> mRegisteredBufs;
};


/******************************************************************************
 *  PostProcDevSession
 ******************************************************************************/

static std::shared_ptr<ICoreDev> createCoreDevice(
  int32_t const& devType) {
  switch (devType) {
    case MTK_POSTPROCDEV_DEVICE_TYPE_CAPTURE:
      return ICoreDev::createCoreDev(CoreDevType::CAPTURE_DEV);
    case MTK_POSTPROCDEV_DEVICE_TYPE_VRP:
      return ICoreDev::createCoreDev(CoreDevType::STREAM_DEV);
    case MTK_POSTPROCDEV_DEVICE_TYPE_WPEPQ:
      return ICoreDev::createCoreDev(CoreDevType::WPE_DEV);
    case MTK_POSTPROCDEV_DEVICE_TYPE_FD:
      return ICoreDev::createCoreDev(CoreDevType::FD_DEV);
    default:
      return nullptr;
  }
}

static android::sp<IImageStreamInfo> createImageStreamInfo(
    const mcam::Stream& srcStream,
    const int32_t& devType,
    mcam::HalStream* outSetting) {
  FUNC_START;
  std::ostringstream os;
  os << "s:" << std::showbase << std::hex << srcStream.id
     << ":t:" << devType << ":PPDevSesiion:";

  std::string streamName = os.str();
  int32_t imgFormat = (int32_t)srcStream.format;
  NSCam::MSize imgSize = NSCam::MSize(srcStream.width, srcStream.height);
  IImageStreamInfo::BufPlanes_t bufPlanes = srcStream.bufPlanes;
  int32_t StreamType = srcStream.streamType == StreamType::OUTPUT ?
                       NSCam::v3::eSTREAMTYPE_IMAGE_OUT :
                       NSCam::v3::eSTREAMTYPE_IMAGE_IN;

  uint64_t const usageForHal = mcam::eBUFFER_USAGE_SW_READ_OFTEN |
                               mcam::eBUFFER_USAGE_SW_WRITE_OFTEN |
                               mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE;
  uint64_t usageForAllocator = usageForHal;
  //
  IImageStreamInfo::BufPlanes_t allocBufPlanes = bufPlanes;
  int32_t allocImgFormat = imgFormat;
  // Alloc Format: Jpeg -> Blob
  if (imgFormat == (int32_t)NSCam::eImgFmt_BLOB ||
      imgFormat == (int32_t)NSCam::eImgFmt_JPEG) {
    bufPlanes.count = 1;
    allocBufPlanes.count = 1;
    allocImgFormat = static_cast<int32_t>(NSCam::eImgFmt_BLOB);
    allocBufPlanes.planes[0].sizeInBytes = bufPlanes.planes[0].sizeInBytes;
    allocBufPlanes.planes[0].rowStrideInBytes = bufPlanes.planes[0].sizeInBytes;
    MY_LOGD("blob: (sizeInBytes, rowStrideInBytes)=(%zu,%zu)",
            bufPlanes.planes[0].sizeInBytes,
            allocBufPlanes.planes[0].rowStrideInBytes);
  }
  // Alloc Format: Raw16 -> Blob
  if (imgFormat == NSCam::eImgFmt_RAW16) {
    allocBufPlanes.count = 1;
    allocImgFormat = static_cast<int32_t>(NSCam::eImgFmt_BLOB);
    imgFormat = static_cast<int32_t>(NSCam::eImgFmt_BAYER12_UNPAK);
    allocBufPlanes.planes[0].sizeInBytes = bufPlanes.planes[0].sizeInBytes;
    allocBufPlanes.planes[0].rowStrideInBytes = bufPlanes.planes[0].sizeInBytes;
    MY_LOGD("Raw16-> 12bit raw: (sizeInBytes, rowStrideInBytes)=(%zu,%zu)",
             bufPlanes.planes[0].sizeInBytes,
             allocBufPlanes.planes[0].rowStrideInBytes);
  }

  if (StreamType == NSCam::v3::eSTREAMTYPE_IMAGE_IN) {
    streamName += "input";
  } else {
    streamName += "output";
  }

  auto pImageStreamInfo = NSCam::v3::Utils::ImageStreamInfoBuilder()
                          .setStreamName(streamName.c_str() == nullptr ? "unknown" : streamName.c_str())
                          .setStreamId(srcStream.id)
                          .setStreamType(StreamType)
                          .setMaxBufNum(10)
                          .setMinInitBufNum(1)
                          .setUsageForAllocator(usageForAllocator)
                          .setAllocImgFormat(allocImgFormat)
                          .setAllocBufPlanes(allocBufPlanes)
                          .setImgFormat(imgFormat)
                          .setBufPlanes(bufPlanes)
                          .setImgSize(imgSize)
                          .setBufCount(1)
                          .setBufStep(0)
                          .setStartOffset(0)
                          .setTransform(srcStream.transform)
                          .build();
  //
  if (outSetting != nullptr) {
    outSetting->id = srcStream.id;
    outSetting->overrideFormat = (NSCam::EImageFormat)imgFormat;
    outSetting->producerUsage = StreamType == NSCam::v3::eSTREAMTYPE_IMAGE_IN ?
                                0 : usageForHal;
    outSetting->consumerUsage = StreamType == NSCam::v3::eSTREAMTYPE_IMAGE_IN ?
                                usageForHal : 0;
    outSetting->maxBuffers = 4;  // currently, use hardcode = 4
  }
  FUNC_END;
  return pImageStreamInfo;
}


template <typename T>
inline MBOOL tryGetMetadata(IMetadata const* metadata,
                            MUINT32 const tag,
                            T& rVal) {
  if (metadata == NULL) {
    MY_LOGE("InMetadata == NULL");
    return MFALSE;
  }

  IMetadata::IEntry entry = metadata->entryFor(tag);
  if (!entry.isEmpty()) {
    rVal = entry.itemAt(0, NSCam::Type2Type<T>());
    return MTRUE;
  }
  return MFALSE;
}

static std::shared_ptr<NSCam::CameraInfoMapT> createCameraInfo(
    std::vector<int32_t> SensorIdSet) {

  return nullptr;
}

auto PostProcDevSession::createSession(
        int32_t type,
        const std::shared_ptr<IMtkcamDeviceCallback>& callback)
        -> std::shared_ptr<IMtkcamDeviceSession> {
  return std::make_shared<PostProcDevSessionImp>(type, callback);
}

PostProcDevSessionImp::PostProcDevSessionImp(
        int32_t type,
        const std::shared_ptr<IMtkcamDeviceCallback>& callback) {
  mType = type;
  mCallback = std::make_shared<CallbackHandler>(callback);
  mCoreDev = createCoreDevice(mType);
}

PostProcDevSessionImp::~PostProcDevSessionImp() {
  MY_LOGD("destroy PostProcDevSessionImp");
}

auto PostProcDevSessionImp::configureStreams(
        const StreamConfiguration& requestedConfiguration,
        HalStreamConfiguration& halConfiguration)
        -> int {
  InternalStreamConfig coreConfig;
  mSensorIdSet.clear();
  coreConfig.SensorIdSet.clear();
  for (ssize_t i = 0; i < requestedConfiguration.streams.size(); i++) {
    mcam::HalStream halSetting;
    auto pInfo = createImageStreamInfo(
                      requestedConfiguration.streams[i],
                      mType,
                      &halSetting);
    if (requestedConfiguration.streams[i].streamType == StreamType::INPUT) {
      coreConfig.vInputInfo.emplace(pInfo->getStreamId(), pInfo);
    } else {
      coreConfig.vOutputInfo.emplace(pInfo->getStreamId(), pInfo);
    }
    halConfiguration.streams.push_back(halSetting);
    auto stream = requestedConfiguration.streams[i];
    if (stream.physicalCameraId == -1) {
      MY_LOGW("stream (0x%" PRIx64 ") physicalCameraId is -1",
               pInfo->getStreamId());
      if (mSensorIdSet.size() == 0) {
        MY_LOGW("mSensorIdSet size = 0, id=0 by default");
        mSensorIdSet.push_back(0);
      }
      MY_LOGW("stream (0x%" PRIx64 ") physicalCameraId is -1",
                 pInfo->getStreamId());
      MY_LOGW("set physicalCameraId = %d", mSensorIdSet[0]);
      stream.physicalCameraId = mSensorIdSet[0];
    } else {
      bool found = false;
      for (auto&& id : mSensorIdSet) {
        if (id == stream.physicalCameraId) {
          found = true;
          break;
        }
      }
      if (!found) {
        mSensorIdSet.push_back(stream.physicalCameraId);
      }
    }
    mCnfiguredStreams.emplace(stream.id, stream);
    MY_LOGD("pushed in-img stream (0x%" PRIx64 ")", pInfo->getStreamId());
  }
  coreConfig.SensorIdSet = mSensorIdSet;
  coreConfig.pConfigureParam =
    std::make_shared<IMetadata>(requestedConfiguration.sessionParams);
  coreConfig.pCameraInfoMap = createCameraInfo(coreConfig.
SensorIdSet);
  //  input metadata
  #define SET_METASTREAM(_id_, _name_, _type_, _maxBuf_, _initBufNum_) \
  {                                                                    \
    auto pMetaStreamInfo = new MetaStreamInfo(                         \
                                _name_,                                \
                                _id_,                                  \
                                _type_,                                \
                                _maxBuf_,                              \
                                _initBufNum_                           \
                                );                                     \
    coreConfig.vTuningMetaInfo.emplace(_id_, pMetaStreamInfo);         \
  }
  SET_METASTREAM(NSCam::eSTREAMID_META_APP_CONTROL,
                 "Meta:App:Control",
                 NSCam::v3::eSTREAMTYPE_META_IN, 0, 0);
  SET_METASTREAM(NSCam::eSTREAMID_META_PIPE_DYNAMIC_01,
                 "Hal:Meta:P1:Dynamic",
                 NSCam::v3::eSTREAMTYPE_META_IN, 10, 1);
  SET_METASTREAM(NSCam::eSTREAMID_META_APP_DYNAMIC_01,
                 "App:Meta:DynamicP1",
                 NSCam::v3::eSTREAMTYPE_META_IN, 10, 1);

  auto ret = mCoreDev->configure(coreConfig);
  return ret;
}

auto PostProcDevSessionImp::processCaptureRequest(
        const std::vector<CaptureRequest>& requests,
        uint32_t& numRequestProcessed)
        -> int {
  int ret = 0;
  numRequestProcessed = 0;
  for (auto& req : requests) {
    IMetadata halDumpMeta = req.halSettings;
    // halDumpMeta.dump(1,1);
    MY_LOGD("postProcReq halmeta is empty(%d)", halDumpMeta.isEmpty());

    ret = processSingleRequestLocked(req);
    if (ret != 0) {
      MY_LOGE("process request(%d) error", req.frameNumber);
      break;
    }
    numRequestProcessed++;
  }

  return ret;
}

static int parseRequestToGetInput(
        const mcam::SubRequest& input,
        const std::map<int64_t, mcam::Stream>& streamMap,
        RegisteredData* regData,
        std::vector<std::vector<SensorData>>* inData) {
  std::vector<SensorData> vsensorData;
  std::map<int32_t, int32_t> sensorDataIndexMap;
  std::vector<RequestBuf> requestInBufs;
  for (auto& inBuf : input.inputBuffers) {
    SensorData sensorData;
    SensorData* psensorData = &sensorData;
    int32_t sensorId;
    if (streamMap.count(inBuf.streamId) == 0) {
      MY_LOGE("stream(0x%" PRIx64 ") is not configured", inBuf.streamId);
      return -1;
    }
    sensorId = streamMap.at(inBuf.streamId).physicalCameraId;
    if (sensorDataIndexMap.count(sensorId) == 0) {
      //  add sensor id to index map
      sensorDataIndexMap.emplace(sensorId, vsensorData.size());
      sensorData.sensorId = sensorId;
      for (auto& settings : input.physicalCameraSettings) {
        if (settings.physicalCameraId == sensorId) {
          sensorData.pHalCameraResult =
            std::make_shared<IMetadata>(settings.halSettings);
          break;
        }
      }
      if (sensorData.pHalCameraResult == nullptr) {
        sensorData.pHalCameraResult =
          std::make_shared<IMetadata>(input.halSettings);
      }
      /*
      if (input.physicalCameraSettings.count(sensorId) == 0) {
        sensorData.pHalCameraResult =
          std::make_shared<IMetadata>(input.halSettings);
      } else {
        sensorData.pHalCameraResult =
          std::make_shared<IMetadata>(
            input.physicalCameraSettings.at(sensorId).halSettings);
      }
      */

      /*IMetadata::IEntry appEntry =
        sensorData.pHalCameraResult->entryFor(MTK_POSTPROC_APP_RESULT);
      if (!appEntry.isEmpty()) {
        IMetadata appResult;
        appResult = appEntry.itemAt(0, NSCam::Type2Type<IMetadata>());
        sensorData.pAppCameraResult =
                  std::make_shared<IMetadata>(appResult);
      }*/
      if (input.subFrameIndex == 0) {
        regData->halResult = *(sensorData.pHalCameraResult);
      }
      vsensorData.push_back(sensorData);
    }
    RequestBuf buf;
    buf.streamId = inBuf.streamId;
    buf.pHeap = inBuf.buffer;
    buf.bufferId = inBuf.bufferId;
    requestInBufs.push_back(buf);
    psensorData = &(vsensorData[sensorDataIndexMap.at(sensorId)]);
    psensorData->inBufs.emplace(inBuf.streamId, buf.pHeap);
  }
  regData->inBufs.emplace(input.subFrameIndex, requestInBufs);
  inData->push_back(vsensorData);
  return 0;
}

auto PostProcDevSessionImp::processSingleRequestLocked(
        const CaptureRequest& request)
        -> int {
  int ret = 0;
  RegisteredData data;
  bool isFindHalMeta = false;
  std::lock_guard<std::mutex> _lock(mLock);
  std::shared_ptr<RequestParams> pOut = std::make_shared<RequestParams>();
  pOut->requestNo = request.frameNumber;
  pOut->pCallback = mCallback;
  pOut->pAppControl = std::make_shared<IMetadata>(request.settings);

  for (auto& outBuf : request.outputBuffers) {
    RequestBuf buf;
    buf.streamId = outBuf.streamId;
    buf.pHeap = outBuf.buffer;
    buf.bufferId = outBuf.bufferId;
    data.outBufs.push_back(buf);
    pOut->outBufs.emplace(outBuf.streamId, buf.pHeap);
  }
  mcam::SubRequest mainInput{
    .subFrameIndex = 0,
    .inputBuffers = request.inputBuffers,
    .settings = request.settings,
    .halSettings = request.halSettings,
    .physicalCameraSettings = request.physicalCameraSettings,
  };
  ret = parseRequestToGetInput(mainInput,
                               mCnfiguredStreams, &data, &(pOut->inData));
  if (ret) {
    MY_LOGE("parse frameindex(%d) failed", mainInput.subFrameIndex);
    return ret;
  }

  for (auto& subInput : request.subRequests) {
    ret = parseRequestToGetInput(subInput,
                                 mCnfiguredStreams, &data, &(pOut->inData));
    if (ret) {
      MY_LOGE("parse frameindex(%d) failed", subInput.subFrameIndex);
      return ret;
    }
  }


  mCallback->registerMtkHalReq(request.frameNumber,
                                       data);
  auto vreqData = mCoreDev->createReqData(pOut);
  ret = mCoreDev->processReq(pOut->requestNo, vreqData);

  return ret;
}

auto PostProcDevSessionImp::flush()-> int {
  int ret = 0;
  ret = mCoreDev->flush();
  if (ret) {
    MY_LOGE("flush core device failed");
    goto lEXIT;
  }
  mCallback->waitForReqDrained(0);

lEXIT:
  return ret;
}

auto PostProcDevSessionImp::close()-> int {
  int ret = 0;
  ret = mCoreDev->close();
  if (ret) {
    MY_LOGE("close core device failed");
    goto lEXIT;
  }
  mCallback->waitForReqDrained(0);

lEXIT:
  mCoreDev = nullptr;
  mCallback = nullptr;
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
static bool findBuffer(
  std::vector<std::shared_ptr<IImageBufferHeap>> completedBufs,
  std::shared_ptr<IImageBufferHeap> pHeap) {
  if (pHeap == nullptr) {
    return false;
  }
  for (auto& buf : completedBufs) {
    if (buf.get() == pHeap.get()) {
      MY_LOGD("debug: find buffer");
      return true;
    }
  }
  return false;
}

CallbackHandler::CallbackHandler(
    const std::shared_ptr<IMtkcamDeviceCallback>& cb) {
  mwpCallback = cb;
}

CallbackHandler::~CallbackHandler() {
  MY_LOGD("destroy CallbackHandler");
}

auto CallbackHandler::registerReq(
        uint32_t const req,
        std::vector<RegisteredBuf> const& bufs) -> void {
  MY_LOGW("unsupport this api, please check");
}

auto CallbackHandler::onFrameUpdated(
        ResultData const& params) -> void {
  FUNC_START;

  mcam::CaptureResult CBData;
  CBData.frameNumber = params.requestNo;

  if (params.pMetaRet != nullptr) {
    CBData.result = *(params.pMetaRet.get());
  }
  CBData.bLastPartialResult = false;
  if (params.bCompleted) {
    CBData.bLastPartialResult = true;
  }
  {
    std::unique_lock<std::mutex> _lock(mLock);
    if (mRegisteredBufs.count(params.requestNo)) {
      if (params.pMetaHal != nullptr) {
        mRegisteredBufs.at(params.requestNo).halResult +=
          *(params.pMetaHal.get());
      }
      if (params.bCompleted) {
        CBData.halResult = mRegisteredBufs.at(params.requestNo).halResult;
      }
      for (auto& buf : mRegisteredBufs.at(params.requestNo).outBufs) {
        if ((params.bCompleted && buf.pHeap != nullptr) ||
            findBuffer(params.completedBufs, buf.pHeap)) {
          StreamBuffer cbStreamBuf;
          cbStreamBuf.streamId = buf.streamId;
          cbStreamBuf.buffer = buf.pHeap;
          cbStreamBuf.status = (BufferStatus)params.status;
          cbStreamBuf.bufferId = buf.bufferId;
          CBData.outputBuffers.push_back(cbStreamBuf);
          buf.pHeap = nullptr;
        }
      }
      for (auto& pair : mRegisteredBufs.at(params.requestNo).inBufs) {
        std::vector<StreamBuffer> cbStreamBufs;
        for (auto& buf : pair.second) {
          if ((params.bCompleted && buf.pHeap != nullptr) ||
              findBuffer(params.completedBufs, buf.pHeap)) {
            StreamBuffer cbStreamBuf;
            cbStreamBuf.streamId = buf.streamId;
            cbStreamBuf.buffer = buf.pHeap;
            cbStreamBuf.status = (BufferStatus)params.status;
            cbStreamBuf.bufferId = buf.bufferId;
            cbStreamBufs.push_back(cbStreamBuf);
            buf.pHeap = nullptr;
          }
        }
        CBData.inputBuffers = cbStreamBufs;
      }
    }
  }
  auto cbHandler = mwpCallback.lock();
  if (cbHandler) {
    std::vector<mcam::CaptureResult> results;
    results.push_back(CBData);
    cbHandler->processCaptureResult(results);
  }
  {
    std::unique_lock<std::mutex> _lock(mLock);
    if (params.bCompleted) {
      mRegisteredBufs.erase(params.requestNo);
    }
    if (mRegisteredBufs.empty()) {
      mWaitCond.notify_all();
    }
  }
  FUNC_END;
}

auto CallbackHandler::registerMtkHalReq(
        uint32_t const req,
        RegisteredData const& data) -> void {
  std::lock_guard<std::mutex> _lock(mLock);
  mRegisteredBufs.emplace(req, data);
}

auto CallbackHandler::waitForReqDrained(
        int32_t const operation) -> void {
  FUNC_START;
  std::unique_lock<std::mutex> _lock(mLock);
  while (!mRegisteredBufs.empty()) {
    auto err = mWaitCond.wait_for(_lock, std::chrono::nanoseconds(10000000000));
    if (err == std::cv_status::timeout) {
      MY_LOGI("waitForReqDrained timeout");
      break;
    }
  }
  FUNC_END;
}

}  // namespace mcam
