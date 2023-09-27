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

#define LOG_TAG "mtkcam_hal/custom"

#include "main/mtkhal/custom/device/3.x/feature/sample/SampleSession.h"

#include <mtkcam/utils/metadata/IMetadata.h>

#include <cutils/properties.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <dlfcn.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../MyUtils.h"
// using ::android::BAD_VALUE;
// using ::android::Mutex;
// using ::android::NAME_NOT_FOUND;
// using ::android::NO_INIT;
// using ::android::OK;
// using ::android::status_t;
// using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
// using ::android::hardware::camera::device::V3_6::HalStream;
// using ::android::hardware::camera::device::V3_6::ICameraDeviceSession;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
// using NSCam::v3::NativeDev::ISP_VIDEO;
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
// using namespace mcam::custom::feature::sample;

using mcam::custom::feature::CreationParams;
using mcam::custom::feature::IFeatureSession;

/******************************************************************************
 *
 ******************************************************************************/

#if (PLATFORM_SDK_VERSION >= 21)
#define CAM_PATH "/libmtkcam_postprocprovider.so"
#else
#ifdef __LP64__
#define CAM_PATH "/vendor/lib64/libmtkcam_postprocprovider.so"
#else
#define CAM_PATH "/vendor/lib/libmtkcam_postprocprovider.so"
#endif
#endif

/******************************************************************************
 *
 ******************************************************************************/

namespace mcam {
namespace custom {
namespace feature {
namespace sample {

/******************************************************************************
 *
 ******************************************************************************/
auto SampleSession::makeInstance(CreationParams const& rParams)
    -> std::shared_ptr<IFeatureSession> {
  auto pSession = std::make_shared<SampleSession>(rParams);
  if (pSession == nullptr) {
    CAM_ULOGME("[%s] Bad pSession", __FUNCTION__);
    return nullptr;
  }
  return pSession;
}
/******************************************************************************
 *
 ******************************************************************************/
/*
auto SampleSession::getPostProcProviderInstance(IProviderCreationParams const*
params)
    -> std::shared_ptr<mcam::IMtkcamProvider> {
  static void* gCamLib = NULL;

  //if (!gCamLib) {
    gCamLib = ::dlopen(CAM_PATH, RTLD_NOW);
    if (!gCamLib) {
      char const* err_str = ::dlerror();
      MY_LOGE("dlopen: %s error=%s", CAM_PATH, (err_str ? err_str : "unknown"));
    }
  //}

  typedef IMtkcamProvider* (*pfnEntry_T)(IProviderCreationParams const*);
  pfnEntry_T pfnEntry =
      (pfnEntry_T)::dlsym(gCamLib, "getPostProcProviderInstance");
  if (!pfnEntry) {
    char const* err_str = ::dlerror();
    MY_LOGE("dlsym: %s error=%s getModuleLib:%p", "getPostProcProviderInstance",
            (err_str ? err_str : "unknown"), gCamLib);
  }
  IProviderCreationParams postprocParams = {
      .providerName = params->providerName,
  };
  auto postprocProvider =
std::shared_ptr<IMtkcamProvider>(pfnEntry(&postprocParams));

  return postprocProvider;
}
*/

/******************************************************************************
 *
 ******************************************************************************/
SampleSession::SampleSession(CreationParams const& rParams)
    :  // mNativeCallback(std::make_shared<NativeCallback>()),
      mSession(rParams.session),
      mCameraDeviceCallback(rParams.callback),
      mInstanceId(rParams.openId),
      mCameraCharacteristics(rParams.cameraCharateristics),
      mvSensorId(rParams.sensorId),
      mLogPrefix("sample-session-" + std::to_string(rParams.openId)) {
  MY_LOGI("ctor");

  if (CC_UNLIKELY(mSession == nullptr || mCameraDeviceCallback == nullptr)) {
    MY_LOGE("session does not exist");
  }

  auto pMetadataProvider = std::shared_ptr<NSCam::IMetadataProvider>(
      NSCam::IMetadataProvider::create(mInstanceId));
  if (pMetadataProvider) {
    IMetadata::IEntry const& result_count =
        pMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_REQUEST_PARTIAL_RESULT_COUNT);
    if (result_count.isEmpty()) {
      MY_LOGE("No static MTK_REQUEST_PARTIAL_RESULT_COUNT");
      mAtMostMetaStreamCount = 1;
    } else {
      mAtMostMetaStreamCount =
          result_count.itemAt(0, NSCam::Type2Type<MINT32>());
    }
  }

  IProviderCreationParams* params;
  // for build pass, must restore if postprocdevicesession is ready
  mPostProcProvider = getPostProcProviderInstance(params);
  if (mPostProcProvider == nullptr) {
    MY_LOGE("null mPostProcProvider");
  }
  auto ret =
      mPostProcProvider->getDeviceInterface(mInstanceId, mPostProcDevice);
  auto pPostProcListener = PostProcListener::create(this);
  mPostProcListener = std::shared_ptr<PostProcListener>(pPostProcListener);
  mPostProcDeviceSession = mPostProcDevice->open(mPostProcListener);
  // preview by-pass mode
  // mNativeDevice = mNativeModule.createDevice(ISP_VIDEO);
}

/******************************************************************************
 *
 ******************************************************************************/
SampleSession::~SampleSession() {
  MY_LOGI("dtor");
  // auto ret = mNativeModule.closeDevice(mNativeDevice);
}

/******************************************************************************
 *
 ******************************************************************************/
// auto
// SampleSession::setCallbacks(
//   const
//   ::android::sp<::android::hardware::camera::device::V3_5::ICameraDeviceCallback>&
//   callback
// ) -> void
// {
//   mCameraDeviceCallback = callback;
//   return;
// }

/******************************************************************************
 *
 ******************************************************************************/
// auto SampleSession::setMetadataQueue(
//     std::shared_ptr<RequestMetadataQueue> &pRequestMetadataQueue,
//     std::shared_ptr<ResultMetadataQueue> &pResultMetadataQueue) -> void
// {
//   mRequestMetadataQueue = pRequestMetadataQueue;
//   mResultMetadataQueue = pResultMetadataQueue;
// }

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
int SampleSession::configureStreams(
    const mcam::StreamConfiguration& requestedConfiguration,
    mcam::HalStreamConfiguration& halConfiguration) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  // check if preview stream brings eBUFFER_USAGE_HW_COMPOSER (1<<8)
  auto isPreviewStream = [&](const auto& stream) -> bool {
    if (stream.streamType == StreamType::OUTPUT &&
        stream.format == NSCam::EImageFormat::eImgFmt_NV21 &&
        (((static_cast<uint64_t>(stream.usage) &
           mcam::eBUFFER_USAGE_HW_TEXTURE) ==
          static_cast<uint64_t>(mcam::eBUFFER_USAGE_HW_TEXTURE)) ||
         ((static_cast<uint64_t>(stream.usage) &
           mcam::eBUFFER_USAGE_HW_COMPOSER) ==
          static_cast<uint64_t>(mcam::eBUFFER_USAGE_HW_COMPOSER)))) {
      return true;
    }
    return false;
  };

  // step1. prepare input parameters for mtk zone
  mcam::StreamConfiguration customStreamConfiguration;
  std::vector<mcam::Stream>
      cusStreams;  // customStreamConfiguration.streams;
  customStreamConfiguration.operationMode =
      requestedConfiguration.operationMode;
  // customStreamConfiguration.sessionParams =
  // requestedConfiguration.sessionParams;
  customStreamConfiguration.streamConfigCounter =
      requestedConfiguration.streamConfigCounter;

  mStreamConfigCounter = requestedConfiguration.streamConfigCounter;

  // check if device is multicam
  std::shared_ptr<NSCam::IMetadataProvider> pMetadataProvider;
  MBOOL is_MultiCam = MFALSE;
  pMetadataProvider = std::shared_ptr<NSCam::IMetadataProvider>(
      NSCam::IMetadataProvider::create(mInstanceId));
  if (pMetadataProvider) {
    IMetadata::IEntry const& capbilities =
        pMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_REQUEST_AVAILABLE_CAPABILITIES);
    if (capbilities.isEmpty()) {
      MY_LOGE(
          "No static MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA");
    } else {
      for (MUINT i = 0; i < capbilities.count(); i++) {
        if (capbilities.itemAt(i, NSCam::Type2Type<MUINT8>()) ==
            MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
          is_MultiCam = MTRUE;
        }
      }
    }
  }

  //  multicam -> pass sensorId    singlecam -> pass instanceId
  for (std::vector<int32_t>::const_iterator i = mvSensorId.begin();
       i != mvSensorId.end(); ++i) {
    MY_LOGD("mvSensorId_in_Sample_Session:%d", *i);
  }
  MY_LOGD("mInstanceId_in_Sample_Session:%d", mInstanceId);

  // NSCam::v3::NativeDev::StreamConfiguration nativeStreamConfiguration;
  /*
  if (is_MultiCam == MFALSE) {
    nativeStreamConfiguration.SensorIdSet = {mInstanceId};
  } else {
    nativeStreamConfiguration.SensorIdSet = {mvSensorId};
  }
  */

  mcam::StreamConfiguration postProcConfiguration;
  /*
  if (is_MultiCam == MFALSE) {
    postProcConfiguration.sensorIdSet = {mInstanceId};
  } else {
    postProcConfiguration.sensorIdSet = {mvSensorId};
  }
  */
  postProcConfiguration.operationMode = requestedConfiguration.operationMode;
  postProcConfiguration.streamConfigCounter =
      requestedConfiguration.streamConfigCounter;

  // determine customization streams
  // key: stream.id, value: settings for this stream
  std::map<int32_t, std::vector<int64_t>> streamSettingMap;

  // step2. prepare tuning stream
  // 2.1    tuning stream for HAL3+ Stream of HIDL structure
  int32_t customStreamId = (requestedConfiguration.streams[0].id + 1) * 1000;
  /*
    mTuningStream.id = customStreamId++;  // cannot be the same with this and
                                               // previous configured streams
    mTuningStream.streamType = StreamType::OUTPUT;
    mTuningStream.width = 1280;  // hard code
    mTuningStream.height = 720;  // hard code
  #if 1
    mTuningStream.format = NSCam::eImgFmt_YV12;  // HAL_PIXEL_FORMAT_YV12
    mTuningStream.usage =
  static_cast<NSCam::BufferUsage>(NSCam::BufferUsage::eBUFFER_USAGE_SW_READ_OFTEN
  | NSCam::BufferUsage::eBUFFER_USAGE_SW_READ_RARELY); mTuningStream.dataSpace =
  mcam::Dataspace::UNKNOWN; #else mTuningStream.format =
  NSCam::eImgFmt_BLOB;
    // mTuningStream.v3_2.usage  =
    //
  static_cast<BufferUsageFlags>(::android::hardware::graphics::common::V1_0::BufferUsage::CPU_READ_OFTEN);
    // mTuningStream.v3_2.dataSpace =
    //
  static_cast<DataspaceFlags>(::android::hardware::graphics::common::V1_0::Dataspace::UNKNOWN);
  #endif
    mTuningStream.transform = NSCam::eTransform_None;
    mTuningStream.physicalCameraId = {};
    mTuningStream.bufferSize = 0;
    // 2.2    tuning stream information defined by vendortags
    streamSettingMap[1000].push_back(mTuningStream.id);  // STREAM_ID
    streamSettingMap[1000].push_back(0x90000010);             // STREAM_DESC
    streamSettingMap[1000].push_back(1);                      // PARAM_COUNT
    streamSettingMap[1000].push_back(
        MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT);                          //
  PARAM#1 streamSettingMap[1000].push_back(MTK_HAL3PLUS_STREAM_FORMAT_BLOB);  //
  VALUE#1 cusStreams.push_back(mTuningStream);
    // 2.3    tuning stream for native isp i/f (configuration does not need
  tuning
    // stream)
    mTuningStream_native.id = mTuningStream.id;
    mTuningStream_native.size = 1280 * 720;
    mTuningStream_native.stride = {1280 * 720};
    mTuningStream_native.width = 1280 * 720;
    mTuningStream_native.height = 1;
    mTuningStream_native.format =
        static_cast<NSCam::v3::NativeDev::MtkImgFormat>(MTK_HAL3PLUS_STREAM_FORMAT_BLOB);
    mTuningStream_native.transform = 0;
    mTuningStream_native.batchsize = 1;
  #warning "Native ISP does not need tuning stream configuration"
    // nativeStreamConfiguration.InputStreams.push_back(mTuningStream_native);

  */

  // step3. prepare image streams (example for preview stream)
  for (auto& inStream : requestedConfiguration.streams) {
    // preview stream: create working stream for mtk zone
    MY_LOGD("%s", toString(inStream).c_str());
    if (isPreviewStream(inStream)) {
      MY_LOGD("preview streamId: %u", inStream.id);
      mPreviewWorkingStream.id = customStreamId++;
      mPreviewWorkingStream.streamType = inStream.streamType;
      mPreviewWorkingStream.width = inStream.width;
      mPreviewWorkingStream.height = inStream.height;
      mPreviewWorkingStream.format = inStream.format;
      mPreviewWorkingStream.usage = inStream.usage;
      mPreviewWorkingStream.dataSpace = inStream.dataSpace;
      mPreviewWorkingStream.transform = inStream.transform;
      mPreviewWorkingStream.physicalCameraId = inStream.physicalCameraId;
      mPreviewWorkingStream.bufferSize = inStream.bufferSize;
      mPreviewWorkingStream.settings = inStream.settings;
      mPreviewWorkingStream.bufPlanes = inStream.bufPlanes;
      // mPreviewWorkingStream.batchSize = inStream.batchSize;
      // mPreviewWorkingStream.imgProcSettings = inStream.imgProcSettings;
      mPreviewStream = inStream;
      cusStreams.push_back(mPreviewWorkingStream);

      // preview stream for native isp i/f (hard code w/ YV12)
      // mPreviewInStream_native
      /*
      mPreviewInStream_native.id = mPreviewWorkingStream.id;
      mPreviewInStream_native.size =
          inStream.width * inStream.height * 3 / 2;
      mPreviewInStream_native.stride = {inStream.width,
                                        inStream.width / 2,
                                        inStream.width / 2};
      mPreviewInStream_native.width = inStream.width;
      mPreviewInStream_native.height = inStream.height;
      mPreviewInStream_native.format =
      static_cast<NSCam::v3::NativeDev::MtkImgFormat>(
          MTK_HAL3PLUS_STREAM_FORMAT_NV21);
      mPreviewInStream_native.transform = 0;
      mPreviewInStream_native.batchsize = 1;
      */

      // mPreviewOutStream_native
      /*
      mPreviewOutStream_native.id = inStream.id;
      mPreviewOutStream_native.size =
          inStream.width * inStream.height * 3 / 2;
      mPreviewOutStream_native.stride = {inStream.width,
                                         inStream.width / 2,
                                         inStream.width / 2};
      mPreviewOutStream_native.width = inStream.width;
      mPreviewOutStream_native.height = inStream.height;
      mPreviewOutStream_native.format =
          static_cast<NSCam::v3::NativeDev::MtkImgFormat>(
              MTK_HAL3PLUS_STREAM_FORMAT_NV21);
      mPreviewOutStream_native.transform = 0;
      mPreviewOutStream_native.batchsize = 1;
      nativeStreamConfiguration.InputStreams.push_back(mPreviewInStream_native);
      nativeStreamConfiguration.OutputStreams.push_back(
          mPreviewOutStream_native);
      */

      mPreviewOutStream_PostProc.id = inStream.id;
      mPreviewOutStream_PostProc.streamType = StreamType::OUTPUT;
      mPreviewOutStream_PostProc.width = mPreviewWorkingStream.width;
      mPreviewOutStream_PostProc.height = mPreviewWorkingStream.height;
      mPreviewOutStream_PostProc.format = NSCam::eImgFmt_NV21;
      mPreviewOutStream_PostProc.usage = mPreviewWorkingStream.usage;
      mPreviewOutStream_PostProc.dataSpace = mPreviewWorkingStream.dataSpace;
      mPreviewOutStream_PostProc.transform = NSCam::eTransform_None;
      mPreviewOutStream_PostProc.physicalCameraId =
          mPreviewWorkingStream.physicalCameraId;
      mPreviewOutStream_PostProc.bufferSize = mPreviewWorkingStream.bufferSize;
      mPreviewOutStream_PostProc.bufPlanes = mPreviewWorkingStream.bufPlanes;
      // mPreviewOutStream_PostProc.batchSize = mPreviewWorkingStream.batchSize;
      // mPreviewOutStream_PostProc.imgProcSettings =
      // mPreviewWorkingStream.imgProcSettings;
      mPreviewOutStream_PostProc.settings = mPreviewWorkingStream.settings;
      postProcConfiguration.streams.push_back(mPreviewOutStream_PostProc);

      mPreviewInStream_PostProc.id = mPreviewWorkingStream.id;
      mPreviewInStream_PostProc.streamType = StreamType::INPUT;
      mPreviewInStream_PostProc.width = mPreviewWorkingStream.width;
      mPreviewInStream_PostProc.height = mPreviewWorkingStream.height;
      mPreviewInStream_PostProc.format = NSCam::eImgFmt_NV21;
      mPreviewInStream_PostProc.usage = mPreviewWorkingStream.usage;
      mPreviewInStream_PostProc.dataSpace = mPreviewWorkingStream.dataSpace;
      mPreviewInStream_PostProc.transform = NSCam::eTransform_None;
      mPreviewInStream_PostProc.physicalCameraId =
          mPreviewWorkingStream.physicalCameraId;
      mPreviewInStream_PostProc.bufferSize = mPreviewWorkingStream.bufferSize;
      mPreviewInStream_PostProc.bufPlanes = mPreviewWorkingStream.bufPlanes;
      // mPreviewInStream_PostProc.batchSize = mPreviewWorkingStream.batchSize;
      // mPreviewInStream_PostProc.imgProcSettings =
      // mPreviewWorkingStream.imgProcSettings;
      mPreviewInStream_PostProc.settings = mPreviewWorkingStream.settings;
      postProcConfiguration.streams.push_back(mPreviewInStream_PostProc);
    } else {
      MY_LOGD("others streamId: %u", inStream.id);
      cusStreams.push_back(inStream);
    }
  }

  /*
      //  sort InputStreams
    sort(postProcConfiguration.streams.begin(),
    postProcConfiguration.streams.end(),
             [](mcam::Stream s1, mcam::Stream s2)->bool{return
    (s1.size > s2.size);});

    for (std::vector<v3::NativeDev::Stream>::const_iterator i =
    nativeStreamConfiguration.InputStreams.begin(); i !=
    nativeStreamConfiguration.InputStreams.end(); ++i){ MY_LOGD("input native
    stream sorted by size id: %d size: %d", i->id, i->size);
    }

    //  sort InputStreams
  */
  customStreamConfiguration.streams = cusStreams;

  // step4. prepare metadata
  // 4.1    transter from streamSettingMap to one vector (consecutive memory)

  /*
  std::vector<int64_t> streamSettings;
  streamSettings.push_back(streamSettingMap.size());
  for (const auto& onStream : streamSettingMap) {
    std::copy(onStream.second.begin(), onStream.second.end(),
              back_inserter(streamSettings));
  }
  */

  // 4.2    clone from original input sessionParams from framework
  const NSCam::IMetadata inputSessionParams =
      requestedConfiguration.sessionParams;
  auto cusSessionParams = inputSessionParams;

  // 4.3    add hal3plus vendortags
  /*
  NSCam::IMetadata::IEntry entry(MTK_HAL3PLUS_STREAM_SETTINGS);
  for (auto& val : streamSettings) {
    entry.push_back(val, NSCam::Type2Type<MINT64>());
  }
  int err = cusSessionParams.update(MTK_HAL3PLUS_STREAM_SETTINGS, entry);
  */

  // Tuning node not enabled
  /*
   uint8_t ispTuningDataEnable = 2;
   IMetadata::setEntry<uint8_t>(&cusSessionParams,
                                 MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_ENABLE,
                                 ispTuningDataEnable);
  */

  // set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP
  int64_t streamUsageMap[2];
  MY_LOGD("postProcConfiguration_streams:%d",
          postProcConfiguration.streams[1].id);
  streamUsageMap[0] = postProcConfiguration.streams[1].id;  // stream ID of f0
  // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
  streamUsageMap[1] = MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F0;

  NSCam::IMetadata::IEntry entry(MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP);
  for (auto& val : streamUsageMap) {
    entry.push_back(val, NSCam::Type2Type<MINT64>());
  }
  int err =
      cusSessionParams.update(MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP, entry);

  MY_LOGD("samp_err:%d", err);

  // MY_LOGE_IF(result != OK,
  // "add_camera_metadata_entry MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP fail,
  // check entry/data capacity");

  // set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP

  NSCam::IMetadata postProcConfiureParams = cusSessionParams;
  NSCam::IMetadata hidl_meta = cusSessionParams;
  customStreamConfiguration.sessionParams = std::move(hidl_meta);
  // nativeStreamConfiguration.pConfigureParams = nativeConfiureParams;
  postProcConfiguration.sessionParams = std::move(postProcConfiureParams);

  MY_LOGD("======== dump all streams ========");
  for (auto& stream : requestedConfiguration.streams)
    MY_LOGI("%s", toString(stream).c_str());

  MY_LOGD("======== dump all customed streams ========");
  for (auto& stream : customStreamConfiguration.streams)
    MY_LOGI("%s", toString(stream).c_str());

  // step5. call MTK ICameraDeviceSession::configureStreams_3_6
  auto ret =
      mSession->configureStreams(customStreamConfiguration, halConfiguration);
  MY_LOGD("msession configure result : %d", ret);

#warning "workaround sleep for async pipeline configuration"
  usleep(100 * 1000);

  // step6. configure native device (remove temporary for MTKHAL bypass)
  // mNativeCallback->mParent = this;
  // auto retNative =
  // mNativeDevice->configure(nativeStreamConfiguration, mNativeCallback);
  // if (retNative != 0) {
  //   MY_LOGE("configure native device fail");
  // }

  auto ret_postProc = mPostProcDeviceSession->configureStreams(
      postProcConfiguration, halConfiguration);
  MY_LOGD("mPostProcDeviceSession configure result : %d", ret_postProc);

  MY_LOGD("-");
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
int SampleSession::processCaptureRequest(
    const std::vector<mcam::CaptureRequest>& requests,
    uint32_t& numRequestProcessed) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();
  // MY_LOGD("======== dump all requests ========");
  // for (auto& request : requests) {
  //   MY_LOGI("%s", toString(request).c_str());
  //   for (auto& streambuffer : request.outputBuffers)
  //     MY_LOGI("%s", toString(streambuffer).c_str());
  // }

  int status = 0;  // OK;
  {
    if (CC_UNLIKELY(mSession == nullptr)) {
      MY_LOGE("Bad CameraDevice3Session");
      return 7;  // Status::ILLEGAL_ARGUMENT;
    }

    std::vector<mcam::CaptureRequest> cusCaptureRequests;
    cusCaptureRequests.resize(requests.size());
    MY_LOGD("preserve cusCaptureRequests size(%zu) from requests size(%zu)",
            cusCaptureRequests.size(), requests.size());

    for (size_t i = 0; i < requests.size(); i++) {
      // step1. prepare input parameters for mtk zone
      auto& cusCaptureRequest = cusCaptureRequests[i];
      cusCaptureRequest.frameNumber = requests[i].frameNumber;
      cusCaptureRequest.settings = requests[i].settings;
      cusCaptureRequest.inputBuffers = requests[i].inputBuffers;
      cusCaptureRequest.halSettings = requests[i].halSettings;
      cusCaptureRequest.subRequests = requests[i].subRequests;
      // cusCaptureRequest.v3_2.outputBuffers.resize(requests[i].v3_2.outputBuffers.size()+1);
      cusCaptureRequest.physicalCameraSettings =
          requests[i].physicalCameraSettings;

      MY_LOGD("processCaptureRequest request(%d)",
              cusCaptureRequest.frameNumber);

      // Native ISP I/F preparation
      std::lock_guard<std::mutex> _l(mInflightMapLock);

      auto& inflightReq = mInflightRequests[requests[i].frameNumber];
      auto& postProc = inflightReq.mPostProc;
      inflightReq.frameNumber = requests[i].frameNumber;

      auto& cusStreamBuffers = cusCaptureRequest.outputBuffers;
      cusStreamBuffers.resize(requests[i].outputBuffers.size());
      // MY_LOGD("cusStreamBuffers input size:%d", requests[i].inputBuffers.size());
      size_t imgCount = 0;

      for (auto& streamBuffer : requests[i].outputBuffers) {
        // preview stream: create working buffer for mtk zone
        // prepare image streams (example for preview stream)
        if (streamBuffer.streamId == mPreviewStream.id) {
          cusStreamBuffers.resize(requests[i].outputBuffers.size());  // +1

          MY_LOGD("cusStreamBuffers size:%d",
                  requests[i].outputBuffers.size() + 1);
          // step2. prepare tuning buffer
          // 2.1    streambuffer
          // auto& tuningStreamBuffer = cusStreamBuffers[imgCount++];
          // tuningStreamBuffer.streamId = mTuningStream.id;
          // tuningStreamBuffer.bufferId = mTuningStreamBufferSerialNo++;
          // tuningStreamBuffer.status = mcam::BufferStatus::OK;
          // tuningStreamBuffer.acquireFence = hidl_handle();
          // tuningStreamBuffer.releaseFence = hidl_handle();

          // 2.2    allocate buffer
          // IImageBufferAllocator::ImgParam
          // imgParam_tuning(mTuningStream.format,
          // NSCam::MSize(mTuningStream.width,
          // mTuningStream.height),
          // mTuningStream.usage);
          // std::string str_tuning = "custTuningStream";
          // std::shared_ptr<IImageBufferHeap> pImageBufferHeap =
          // IImageBufferHeapFactory::create(LOG_TAG, str_tuning.c_str(),
          // imgParam_tuning, true); tuningStreamBuffer.buffer =
          // pImageBufferHeap; inflightReq.mTuningBufferHd =
          // tuningStreamBuffer.buffer;

          // step3. prepare preview image streams
          // StreamBuffer previewWorkingStreamBuffer;
          auto& previewWorkingStreamBuffer = cusStreamBuffers[imgCount++];
          // 3.1 streambuffer
          previewWorkingStreamBuffer.streamId = mPreviewWorkingStream.id;
          previewWorkingStreamBuffer.bufferId =
              mPreviewWorkingStreamBufferSerialNo++;
          previewWorkingStreamBuffer.status = mcam::BufferStatus::OK;
          previewWorkingStreamBuffer.bufferSettings =
              streamBuffer.bufferSettings;
          // previewWorkingStreamBuffer.imgProcSettings =
          // streamBuffer.imgProcSettings;
          // previewWorkingStreamBuffer.acquireFence = hidl_handle();
          // previewWorkingStreamBuffer.releaseFence = hidl_handle();

          // 3.2 allocate buffer

          MINT bufBoundaryInBytes[2] = {0};
          MUINT32 bufStridesInBytes[2] = {mPreviewWorkingStream.width,
                                          mPreviewWorkingStream.width};

          size_t planeCount =
              NSCam::Utils::Format::queryPlaneCount(NSCam::eImgFmt_NV21);

          mcam::IImageBufferAllocator::ImgParam imgParam_preview(
              NSCam::eImgFmt_NV21,
              NSCam::MSize(mPreviewWorkingStream.width,
                           mPreviewWorkingStream.height),
              mPreviewWorkingStream.usage);
          std::string str_preview = "custPreviewStream";
          auto pImageBufferHeap = mcam::IImageBufferHeapFactory::create(
              LOG_TAG, str_preview.c_str(), imgParam_preview, true);
          previewWorkingStreamBuffer.buffer = pImageBufferHeap;
          inflightReq.mPreviewBufferHd = previewWorkingStreamBuffer.buffer;

          MY_LOGD("preview working buffer");
          // 3.3 prepare destination preview buffer for Native ISP

          // remove temporary for MTKHAL bypass
          // v3::NativeDev::NativeBuffer nativeBuf_preview = {
          //     .pBuffer = reinterpret_cast<void*>(streamBuffer.buffer),
          //     .type = v3::NativeDev::BufferType::GRAPHIC_HANDLE,
          //     .offset = 0,
          //     .bufferSize = mPreviewWorkingStream.width *
          //                   mPreviewWorkingStream.height * 3 / 2,
          // };
          // enable this flow for preview request

          postProc.numInputBuffers = 1;
          postProc.numOutputBuffers = 1;
          postProc.numTuningBuffers = 1;
          postProc.numManualTunings = 0;
          postProc.readyOutputBuffers = 1;
          inflightReq.mFwkPreviewBufferId = streamBuffer.bufferId;
          inflightReq.mFwkPreviewBufferHd = streamBuffer.buffer;
        } else {
          cusStreamBuffers[imgCount++] = streamBuffer;
          MY_LOGE("others: %s",
                  toString(cusStreamBuffers[imgCount - 1]).c_str());
        }

        // step4. clone from original input settings from framework
        if (!(requests[i].settings.isEmpty())) {
          const NSCam::IMetadata inputSettings = requests[i].settings;
          if (!(mLastSettings.isEmpty())) {
            mLastSettings.clear();
          }
#if 0  // do nothing
       // mLastSettings = allocate_camera_metadata(
       //   get_camera_metadata_entry_capacity(inputSettings),
       //   get_camera_metadata_data_capacity(inputSettings));
       // auto app_ret = append_camera_metadata(mLastSettings, inputSettings);
          mLastSettings = inputSettings;

#else

          //

          mLastSettings = inputSettings;
          /*
          mLastSettings = inputSettings;
          uint8_t ispTuningReqValue = 2;
          IMetadata::setEntry<uint8_t>(&mLastSettings,
          MTK_CONTROL_CAPTURE_ISP_TUNING_REQUEST, ispTuningReqValue);
          */

          /*
          int32_t forceCap = property_get_int32(
              "vendor.debug.camera.hal3plus.sample.capture", 0);
          if (forceCap) {
            uint8_t captureIntent = MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
            uint8_t capIntentval;
            IMetadata::setEntry<uint8_t>(&mLastSettings,
          MTK_CONTROL_CAPTURE_INTENT, captureIntent);
          }
          */

#endif
#warning "implementor must repeat settings if break settings transferred by fwk"
          // user could append customized control metadata in cusSettings
          // however, no operation in this case
          cusCaptureRequests[i].settings = std::move(mLastSettings);
        }
        postProc.pAppCtrl = mLastSettings;
      }
    }
    MY_LOGD("======== dump all original requests (%zu)========",
            requests.size());

    MY_LOGD("======== dump all customized requests (%zu)========",
            cusCaptureRequests.size());

    std::lock_guard<std::mutex> _l(mInflightMapLock);
    for (auto& iter : mInflightRequests) {
      if (iter.second.bRemovable) {
        mInflightRequests.erase(iter.second.frameNumber);
      }
    }
    MY_LOGD("-");
    return mSession->processCaptureRequest(cusCaptureRequests,
                                           numRequestProcessed);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
int SampleSession::flush() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return 7;  // Status::INTERNAL_ERROR;
  }
  // flush/close HAL3+
  MY_LOGD("sample_session device flush + ");
  auto status = mSession->flush();
  MY_LOGD("sample_session device flush - ");
  MY_LOGD("sample_session PostProc flush + ");
  auto pstatus = mPostProcDeviceSession->flush();
  MY_LOGD("sample_session PostProc flush - ");
  // flush/close Native ISP
  /*
  auto ret = mNativeModule.closeDevice(mNativeDevice);
  if (CC_UNLIKELY(ret != 0)) {
    MY_LOGE("NativeModule.closeDevice() fail");
  }
  mNativeDevice = nullptr;
  MY_LOGD("Native module close dev done");
  */
  //
  // std::vector<::android::hardware::camera::device::V3_2::NotifyMsg> msgs;
  for (auto& inflight : mInflightRequests) {
    inflight.second.dump();
    if (!inflight.second.bRemovable) {
      // notify error
      mcam::NotifyMsg hidl_msg = {};
      hidl_msg.type = mcam::MsgType::ERROR;
      hidl_msg.msg.error.frameNumber = inflight.second.frameNumber;
      hidl_msg.msg.error.errorStreamId = -1;
      hidl_msg.msg.error.errorCode = mcam::ErrorCode::ERROR_REQUEST;
      // msgs.push_back(std::move(hidl_msg));
      mCameraDeviceCallback->notify({hidl_msg});
      // if (CC_UNLIKELY(!notifyResult.isOk())) {
      //   MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
      //           notifyResult.description().c_str());
      // }

      // processCaptureResult buffer
      mcam::CaptureResult imageResult;
      std::vector<mcam::StreamBuffer> inputBuf;
      inputBuf.push_back({.streamId = -1});
      imageResult.frameNumber = inflight.second.frameNumber;
      // imageResult.fmqResultSize = 0;
      imageResult.result = IMetadata();
      // imageResult.resultSize = 0;
      imageResult.outputBuffers.resize(1);
      // imageResult.inputBuffers = inputBuf;
      imageResult.bLastPartialResult = false;
      //
      auto& streamBuffer = imageResult.outputBuffers[0];
      streamBuffer.streamId = mPreviewStream.id;
      streamBuffer.bufferId = inflight.second.mFwkPreviewBufferId;
      streamBuffer.buffer = nullptr;
      streamBuffer.status = mcam::BufferStatus::ERROR;
      streamBuffer.acquireFenceFd = -1;
      streamBuffer.releaseFenceFd = -1;

      mCameraDeviceCallback->processCaptureResult({imageResult});
      // if (CC_UNLIKELY(!processCapResult.isOk())) {
      //   MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
      //           processCapResult.description().c_str());
      // }
      //
      inflight.second.bRemovable = true;
    }
  }

  MY_LOGD("-");
  return status;
  // return Status::OK;
}

/******************************************************************************
 *
 ******************************************************************************/
int SampleSession::close() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto result = mSession->close();
  if (CC_UNLIKELY(result != 0)) {
    MY_LOGE("SampleSession close fail");
  }
  mSession = nullptr;

  if (mPostProcDeviceSession != nullptr) {
    auto ret = mPostProcDeviceSession->close();
    if (CC_UNLIKELY(ret != 0)) {
      MY_LOGE("PostProcDeviceSession close() fail");
    }
    // mNativeDevice = nullptr;
    MY_LOGD("PostProcDeviceSession close done");
  }

  MY_LOGD("-");
  return result;
}

/******************************************************************************
 *
 ******************************************************************************/
void SampleSession::notify(const std::vector<mcam::NotifyMsg>& msgs) {
  MY_LOGD("+");
  CAM_TRACE_CALL();
  if (msgs.size()) {
    // for (auto& msg : msgs) {
    //   MY_LOGI("msgs: %s", toString(msg).c_str());
    // }
    std::vector<mcam::NotifyMsg> vMsgs;
    for (auto& msg : msgs) {
      bool skip = false;
      if (msg.type == mcam::MsgType::ERROR) {
        if (msg.msg.error.errorCode == mcam::ErrorCode::ERROR_BUFFER) {
          if (msg.msg.error.errorStreamId == mTuningStream.id) {
            // do not callback (custom zone's stream)
          } else if (msg.msg.error.errorStreamId == mPreviewWorkingStream.id) {
            mcam::NotifyMsg hidl_msg = {};
            hidl_msg.type = msg.type;
            hidl_msg.msg.error.frameNumber = msg.msg.error.frameNumber;
            hidl_msg.msg.error.errorStreamId = mPreviewStream.id;
            hidl_msg.msg.error.errorCode = msg.msg.error.errorCode;
            vMsgs.push_back(std::move(hidl_msg));
          } else {
            vMsgs.push_back(msg);
          }
        } else if (msg.msg.error.errorCode ==
                   mcam::ErrorCode::ERROR_REQUEST) {
          mInflightRequests[msg.msg.error.frameNumber].bRemovable = true;
          vMsgs.push_back(msg);
        } else if (msg.msg.error.errorCode ==
                   mcam::ErrorCode::ERROR_RESULT) {
          mInflightRequests[msg.msg.error.frameNumber].isErrorResult = true;
        } else {
          vMsgs.push_back(msg);
        }
      } else {
        vMsgs.push_back(msg);
      }
    }
    std::vector<mcam::NotifyMsg> vHidlMsgs;
    vHidlMsgs = vMsgs;
    // for (auto& msg : vHidlMsgs) {
    //   MY_LOGI("sampleMsgs: %s", toString(msg).c_str());
    // }
    MY_LOGI("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
    mCameraDeviceCallback->notify(vHidlMsgs);
  }

  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
auto SampleSession::InflightRequest::dump() -> void {
  ALOGI("InflightRequest:%s: frameNumber(%u) mPostProc(%p) bRemovable(%d)",
        __FUNCTION__, frameNumber, &mPostProc, bRemovable);

  ALOGI("InflightRequest:%s: PostProc.pAppCtrl(%d) PostProc.pAppResult(%d)",
        __FUNCTION__, mPostProc.pAppCtrl.isEmpty(), mPostProc.pAppResult.isEmpty());

  ALOGI(
      "InflightRequest:%s: PostProc.readyInputBuffers(%zu) "
      "PostProc.readyOutputBuffers(%zu) PostProc.readyTuningBuffers(%zu) "
      "PostProc.readyManualTunings(%zu)",
      __FUNCTION__, mPostProc.readyInputBuffers, mPostProc.readyOutputBuffers,
      mPostProc.readyTuningBuffers, mPostProc.readyManualTunings);
  /*
    ALOGI(
        "InflightRequest:%s: PostProc.readyInputBuffers(%zu) "
        "PostProc.readyOutputBuffers(%zu) PostProc.readyTuningBuffers(%zu) "
        "PostProc.readyManualTunings(%zu)",
        __FUNCTION__, mPostProc.readyInputBuffers, mPostProc.readyOutputBuffers,
        mPostProc.readyTuningBuffers, mPostProc.readyManualTunings);
  */
  /*
    ALOGI("InflightRequest:%s: mFwkPreviewBufferHd(%p)
    mFwkPreviewBufferId(%llu)",
          __FUNCTION__, mFwkPreviewBufferHd, mFwkPreviewBufferId);
  */
  // ALOGI("InflightRequest:%s: mPreviewBufferHd(%s) mTuningBufferHd(%s)",
  //       __FUNCTION__, toString(mPreviewBufferHd).c_str(),
  //       toString(mTuningBufferHd).c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
// static void dumpProcRequest(
//     std::shared_ptr<NSCam::v3::NativeDev::ProcRequest>& procRequest) {
//   if (procRequest.get()) {
//     ALOGI("procRequest(%p) reqNumber(%u) pAppCtrl(%p)", procRequest.get(),
//           procRequest->reqNumber, procRequest->pAppCtrl);
//     ALOGI("procRequest Inputs(%p) InputRequest.size(%zu)",
//     &procRequest->Inputs,
//           procRequest->Inputs.in_data.size());
//     for (size_t i = 0; i < procRequest->Inputs.in_data.size(); i++) {
//       auto& inputRequest = procRequest->Inputs.in_data[i];
//       ALOGI("\tInputRequest[%zu].buffers.size(%zu)", i,
//             inputRequest.buffers.size());
//       for (size_t j = 0; j < inputRequest.buffers.size(); j++) {
//         auto& inputBufferSet = inputRequest.buffers[j];
//         ALOGI("\t\tInputRequest[%zu].buffers[%zu].buffers.size(%zu)", i, j,
//               inputBufferSet.buffers.size());
//         if (inputBufferSet.buffers.size()) {
//           auto& buffer = inputBufferSet.buffers[0];
//           ALOGI("\t\t\tBuffer(%p) streamId(%llu) buffer(%p).size(%zu)",
//           &buffer,
//                 buffer.streamId, buffer.buffer,
//                 buffer.buffer->planeBuf.size());
//           for (auto& nativeBuffer : buffer.buffer->planeBuf)
//             ALOGI(
//                 "\t\t\tNativeBuffer.pBuffer(%p) type(%s) bufferSize(%u)",
//                 nativeBuffer.pBuffer,
//                 (nativeBuffer.type ==
//                  NSCam::v3::NativeDev::BufferType::GRAPHIC_HANDLE)
//                     ? "GRAPHIC_HANDLE"
//                     : "UNKNOWN",
//                 nativeBuffer.bufferSize);
//         }
//         ALOGI("\t\t\ttuningBuffer(%p).size(%zu)",
//         inputBufferSet.tuningBuffer,
//               inputBufferSet.tuningBuffer->planeBuf.size());
//         ALOGI("\t\t\tNativeBuffer.pBuffer(%p) type(%s) bufferSize(%u)",
//               inputBufferSet.tuningBuffer->planeBuf[0].pBuffer,
//               (inputBufferSet.tuningBuffer->planeBuf[0].type ==
//                NSCam::v3::NativeDev::BufferType::GRAPHIC_HANDLE)
//                   ? "GRAPHIC_HANDLE"
//                   : "UNKNOWN",
//               inputBufferSet.tuningBuffer->planeBuf[0].bufferSize);
//         // ALOGI("\t\t\tmanualTuning(%p).size(%zu)",
//         //   inputBufferSet.manualTuning,
//         //   inputBufferSet.manualTuning->planeBuf.size());
//         ALOGI("\t\t\tptuningMeta(%p)", inputBufferSet.ptuningMeta);
//       }
//     }

//     ALOGI("procRequest Outputs(%p) OutputBuffer.size(%zu)",
//           &procRequest->Outputs, procRequest->Outputs.buffers.size());
//     for (size_t i = 0; i < procRequest->Outputs.buffers.size(); i++) {
//       auto& OutputBuffer = procRequest->Outputs.buffers[i];
//       ALOGI(
//           "\tOutputBuffer[%zu].buffer(%p) streamId(%llu)
//           buffer(%p).size(%zu)", i, &OutputBuffer.buffer,
//           OutputBuffer.buffer.streamId, OutputBuffer.buffer.buffer,
//           OutputBuffer.buffer.buffer->planeBuf.size());

//       for (auto& nativeBuffer : OutputBuffer.buffer.buffer->planeBuf)
//         ALOGI("\tNativeBuffer.pBuffer(%p) type(%s) bufferSize(%u)",
//               nativeBuffer.pBuffer,
//               (nativeBuffer.type ==
//                NSCam::v3::NativeDev::BufferType::GRAPHIC_HANDLE)
//                   ? "GRAPHIC_HANDLE"
//                   : "UNKNOWN",
//               nativeBuffer.bufferSize);
//     }

//   } else {
//     ALOGE("invalid procRequest");
//   }
// }

/******************************************************************************
 *
 ******************************************************************************/
void SampleSession::processOneCaptureResult(
    const mcam::CaptureResult& result) {
  struct BufferInfo {
    int64_t streamId = -1;
    uint64_t bufferId = 0;
    bool bError = true;
    std::shared_ptr<mcam::IImageBufferHeap> buffer = nullptr;
  };

  auto isBufferOK = [](auto& streamBuffer) -> bool {
    return streamBuffer.status == mcam::BufferStatus::OK;
  };
  //
  {
    std::lock_guard<std::mutex> _l(mInflightMapLock);
    auto& inflightReq = mInflightRequests[result.frameNumber];
    auto& postProc = inflightReq.mPostProc;
    // callback for images
    std::vector<BufferInfo> vBufInfos;
    std::vector<mcam::StreamBuffer> vOutBuffers;

    // output streams
    if (result.outputBuffers.size()) {
      for (auto& streamBuffer : result.outputBuffers) {
        if (streamBuffer.streamId == mPreviewWorkingStream.id) {
          // preview buffer
          if (isBufferOK(streamBuffer)) {
            postProc.readyInputBuffers++;
          } else {
            // prepare error buffer for framework stream
            vBufInfos.push_back({.streamId = mPreviewStream.id,
                                 .bufferId = inflightReq.mFwkPreviewBufferId,
                                 .bError = true,
                                 .buffer = streamBuffer.buffer});
            MY_LOGI("replace error streambuffer id(%d) to id(%d)",
                    mPreviewWorkingStream.id, mPreviewStream.id);
          }
        } else if (streamBuffer.streamId == mTuningStream.id) {
          // tuning buffer
          /*
          if (isBufferOK(streamBuffer)) {
            postProc.readyTuningBuffers++;
          }
          */
        } else {
          vBufInfos.push_back({.streamId = streamBuffer.streamId,
                               .bufferId = streamBuffer.bufferId,
                               .bError = (streamBuffer.status ==
                                          mcam::BufferStatus::ERROR),
                               .buffer = streamBuffer.buffer});
        }
      }
      if (vBufInfos.size()) {
        std::vector<mcam::StreamBuffer> inputBuf;
        inputBuf.push_back({.streamId = -1});
        mcam::CaptureResult imageResult;
        imageResult.frameNumber = result.frameNumber;
        imageResult.result = NSCam::IMetadata();
        imageResult.outputBuffers.resize(vBufInfos.size());
        // imageResult.inputBuffers = inputBuf;
        imageResult.bLastPartialResult = false;

        for (size_t i = 0; i < vBufInfos.size(); ++i) {
          auto& streamBuffer = imageResult.outputBuffers[i];
          streamBuffer.streamId = vBufInfos[i].streamId;
          streamBuffer.bufferId = vBufInfos[i].bufferId;
          streamBuffer.buffer = vBufInfos[i].buffer;
          streamBuffer.status = (vBufInfos[i].bError)
                                    ? mcam::BufferStatus::ERROR
                                    : mcam::BufferStatus::OK;
          streamBuffer.acquireFenceFd = -1;
          streamBuffer.releaseFenceFd = -1;
        }

        // ALOGI("output streams: %s", toString(imageResult).c_str());
        MY_LOGD("output stream id:%d", imageResult.outputBuffers[0].streamId);

        if (mCameraDeviceCallback.get()) {
          mCameraDeviceCallback->processCaptureResult({imageResult});
        }
      }
      /*
      if (mCameraDeviceCallback.get()) {
        mCameraDeviceCallback->processCaptureResult({result});

      }
      */
    }

    // input stream
    auto inputBuf = result.inputBuffers;
    if (inputBuf.size()) {
      if (inputBuf[0].streamId != -1) {
        // ALOGI("input stream: %s", toString(result).c_str());
        MY_LOGD("input stream id:%d", inputBuf[0].streamId);
        if (mCameraDeviceCallback.get()) {
          mCameraDeviceCallback->processCaptureResult({result});
        }
      }
    }
    // metadata
    append_Result += result.result;

    if (result.bLastPartialResult == true) {
      postProc.pAppResult = append_Result;
      MY_LOGD("PostProc.pAppResult(%p)", &(postProc.pAppResult));
      MY_LOGD("receive_last_partial");
    }
    //

    if (inflightReq.isErrorResult == false) {
      mcam::CaptureResult metaResult;
      metaResult.frameNumber = result.frameNumber;
      metaResult.result = result.result;
      metaResult.halResult = result.halResult;
      metaResult.physicalCameraMetadata = result.physicalCameraMetadata;
      metaResult.outputBuffers = {};
      // metaResult.inputBuffers = {{.streamId = -1}};
      metaResult.inputBuffers = {};
      metaResult.bLastPartialResult = false;

      if (mCameraDeviceCallback.get()) {
        mCameraDeviceCallback->processCaptureResult({metaResult});
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void SampleSession::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  // directly callback w/o FMQ
  MY_LOGI("results(%zu)", results.size());
  // std::vector<mcam::CaptureRequest> postProcRequests;
  uint32_t numPostProcRequestProcessed = 0;
  for (auto& result : results) {
    processOneCaptureResult(result);
    IMetadata halDumpMeta = result.halResult;
    MY_LOGD("halResult is empty(%d)", halDumpMeta.isEmpty());

    std::lock_guard<std::mutex> _l(mInflightMapLock);
    auto& inflightReq = mInflightRequests[result.frameNumber];
    auto& postProc = inflightReq.mPostProc;
    // Native ISP processing when all images/metadata are prepared
    MY_LOGD("LHS result frame number: %d", result.frameNumber);
    MY_LOGD("LHS inflight request frame number: %d", inflightReq.frameNumber);
    inflightReq.dump();

    if (!(halDumpMeta.isEmpty())) {
      inflightReq.mPostProc.pHalResult = result.halResult;
    }

    // inflightReq.mPostProc.pHalResult.dump(1,1);

    if (inflightReq.isReadyForPostProc()) {
      MY_LOGD("send post proc request +");

      inflightReq.mPreviewBufferId++;
      mcam::CaptureRequest postProcRequest;
      mcam::StreamBuffer outbuf;
      mcam::StreamBuffer inputbuf;
      postProcRequest.frameNumber = inflightReq.frameNumber;
      postProcRequest.settings = inflightReq.mPostProc.pAppCtrl;
      postProcRequest.halSettings = inflightReq.mPostProc.pHalResult;
      outbuf.streamId = mPreviewStream.id;
      outbuf.buffer = inflightReq.mFwkPreviewBufferHd;
      outbuf.bufferId = inflightReq.mFwkPreviewBufferId;
      inputbuf.streamId = mPreviewWorkingStream.id;

      inputbuf.buffer = inflightReq.mPreviewBufferHd;
      inputbuf.bufferId = inflightReq.mPreviewBufferId;
      postProcRequest.outputBuffers = {outbuf};
      postProcRequest.inputBuffers = {inputbuf};
      // postProcRequests.push_back(postProcRequest);

      std::vector<mcam::CaptureRequest> postProcRequests = {
          postProcRequest};

      MY_LOGD("postProcRequest frame No: %d", postProcRequest.frameNumber);

      mPostProcDeviceSession->processCaptureRequest(
          postProcRequests, numPostProcRequestProcessed);
      MY_LOGD("send post proc request -");
    }
  }
  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
// auto SampleSession::setExtCameraDeviceSession(
//     const ::android::sp<IExtCameraDeviceSession>& session)
//     -> int {
//   if (session == nullptr)
//   {
//     MY_LOGE("fail to setCameraDeviceSession due to session is nullptr");
//     return BAD_VALUE;
//   }
//   mExtCameraDeviceSession = session;
//   return OK;
// }

// /******************************************************************************
//  *
//  ******************************************************************************/
// Return<void> SampleSession::requestStreamBuffers(
//     const
//     ::android::hardware::hidl_vec<::android::hardware::camera::device::V3_5::BufferRequest>&
//     bufReqs,
//       android::hardware::camera::device::V3_5::ICameraDeviceCallback::requestStreamBuffers_cb
//       _hidl_cb) {
//   MY_LOGE("Not implement");
//   _hidl_cb(::android::hardware::camera::device::V3_5::BufferRequestStatus::FAILED_UNKNOWN,
//   {}); return Void();
// }

// /******************************************************************************
//  *
//  ******************************************************************************/
// Return<void> SampleSession::returnStreamBuffers(
//     const
//     ::android::hardware::hidl_vec<::android::hardware::camera::device::V3_2::StreamBuffer>&
//     buffers) {
//   CAM_TRACE_CALL();
//   MY_LOGE("Not implement");
//   return Void();
// }

/******************************************************************************
 *
 ******************************************************************************/

// int32_t SampleSession::NativeCallback::processCompleted(
//     int32_t const& status,
//     NSCam::v3::NativeDev::Result const& result) {
//   ALOGE("NativeCallback not Implemented %s -", __FUNCTION__);
//   return 0;
// }

/******************************************************************************
 *
 ******************************************************************************/

void SampleSession::postProcProcessCompleted(
    const std::vector<mcam::CaptureResult>& results) {
  ALOGD("postProcProcessCompleted: %s +", __FUNCTION__);
  MY_LOGD("postProcProcessCompleted + ");
  // std::lock_guard<std::mutex> _l(mParent->mInflightMapLock);

  std::vector<mcam::CaptureResult> imageResults;
  std::vector<mcam::CaptureResult> metadataResults;

  for (auto& result : results) {
    auto frameNumber = result.frameNumber;
    auto& inflightRequest = mInflightRequests[frameNumber];

    ALOGI("postProcProcessCompleted:%s: results.reqNumber(%u)", __FUNCTION__,
          result.frameNumber);

    if (inflightRequest.bRemovable == true) {
      ALOGI("already removed before, neglect callback for this frame");
      return;
    }

    // prepare buffer callback
    if (inflightRequest.mFwkPreviewBufferHd) {
      // std::vector<mcam::StreamBuffer> inputBuf;
      // inputBuf.push_back({.streamId = -1});
      mcam::CaptureResult captureResult;
      captureResult.frameNumber = frameNumber;
      // captureResult.result = NSCam::IMetadata();
      captureResult.outputBuffers.resize(1);
      // captureResult.inputBuffers = inputBuf;
      // captureResult.v3_2.partialResult;
      // captureResult.physicalCameraMetadata;

      auto& streamBuffer = captureResult.outputBuffers[0];
      streamBuffer.streamId = mPreviewStream.id;
      streamBuffer.bufferId = inflightRequest.mFwkPreviewBufferId;
      streamBuffer.buffer = result.outputBuffers[0].buffer;
      // streamBuffer.buffer = nullptr;
      streamBuffer.status = mcam::BufferStatus::OK;
      // streamBuffer.acquireFenceFd = -1;
      // streamBuffer.releaseFenceFd = -1;

      imageResults = {captureResult};
      // ALOGI("NativeCallback: %s", toString(captureResult).c_str());
      if (mCameraDeviceCallback.get()) {
        mCameraDeviceCallback->processCaptureResult(imageResults);
      }
    }

    // prepare metadata callback
    // if ( result.presult ) {

    if (inflightRequest.isErrorResult == false) {
      {
        mcam::CaptureResult captureResult;
        captureResult.frameNumber = frameNumber;
        std::vector<mcam::StreamBuffer> inputBuf;
        inputBuf.push_back({.streamId = -1});
        // maybe use FMQ
        IMetadata final;
        IMetadata hidl_meta;
        if (!(result.result.isEmpty())) {
          // hidl_meta = result.presult;
          captureResult.result = std::move(hidl_meta);
        } else {
          uint8_t bFrameEnd = 1;
          IMetadata::setEntry<uint8_t>(&final, MTK_HAL3PLUS_REQUEST_END,
                                       bFrameEnd);
          hidl_meta = final;
          captureResult.result = std::move(hidl_meta);
        }
        captureResult.outputBuffers = {};
        // captureResult.inputBuffers = inputBuf;
        captureResult.bLastPartialResult = true;

        ALOGD("NativeCallback_bLastPartialResult: %d",
              captureResult.bLastPartialResult);
        // captureResult.physicalCameraMetadata;
        metadataResults = {captureResult};
        // ALOGI("NativeCallback: %s", toString(captureResult).c_str());
        if (mCameraDeviceCallback.get()) {
          mCameraDeviceCallback->processCaptureResult(metadataResults);
        }
        if (!(final.isEmpty()))
          final.clear();
      }
    }
    {
      // free working buffer
      // mParent->mMapper.freeBuffer(inflightRequest.mPreviewBufferHd.getNativeHandle());
      // mParent->mMapper.freeBuffer(inflightRequest.mTuningBufferHd.getNativeHandle());
      // remove inflightrequest
      inflightRequest.bRemovable = true;
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/

// int32_t SampleSession::NativeCallback::processPartial(
//     int32_t const& status,
//     NSCam::v3::NativeDev::Result const& result,
//     std::vector<NSCam::v3::NativeDev::BufHandle*> const& completedBuf) {
//   ALOGE("NativeCallback: %s Not implemented by NativeDevice", __FUNCTION__);
//   return 0;
// }

// postProcListener Interface
/******************************************************************************
 *
 ******************************************************************************/

SampleSession::PostProcListener::PostProcListener(SampleSession* pParent) {
  mParent = pParent;
}

/******************************************************************************
 *
 ******************************************************************************/

auto SampleSession::PostProcListener::create(SampleSession* pParent)
    -> PostProcListener* {
  auto pInstance = new PostProcListener(pParent);
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/

void SampleSession::PostProcListener::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  mParent->postProcProcessCompleted(results);
}

/******************************************************************************
 *
 ******************************************************************************/

void SampleSession::PostProcListener::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  ALOGE("SampleSession::PostProcListener::notify: %s Not implemented ",
        __FUNCTION__);
}

};  // namespace sample
};  // namespace feature
};  // namespace custom
};  // namespace mcam
