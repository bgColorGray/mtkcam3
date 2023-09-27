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

#include "main/mtkhal/custom/hidl/device/3.x/feature/sample/SampleSession.h"

#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/ULogDef.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <cutils/properties.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../MyUtils.h"

using ::android::BAD_VALUE;
using ::android::Mutex;
using ::android::NAME_NOT_FOUND;
using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;
using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_6::HalStream;
using ::android::hardware::camera::device::V3_6::ICameraDeviceSession;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::v3::NativeDev::ISP_VIDEO;
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
// using namespace NSCam::custom_dev3;
// using namespace NSCam::custom_dev3::feature::sample;

namespace NSCam {
namespace custom_dev3 {
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
SampleSession::SampleSession(CreationParams const& rParams)
    : mNativeCallback(std::make_shared<NativeCallback>()),
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
  int meta_find_result;
  camera_metadata_ro_entry_t entry;
  meta_find_result = find_camera_metadata_ro_entry(
      mCameraCharacteristics, ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry);
  mAtMostMetaStreamCount = entry.data.i32[0];

  // preview by-pass mode
  mNativeDevice = mNativeModule.createDevice(ISP_VIDEO);
}

/******************************************************************************
 *
 ******************************************************************************/
SampleSession::~SampleSession() {
  MY_LOGI("dtor");
  auto ret = mNativeModule.closeDevice(mNativeDevice);
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
auto SampleSession::setMetadataQueue(
    std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
    std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue) -> void {
  mRequestMetadataQueue = pRequestMetadataQueue;
  mResultMetadataQueue = pResultMetadataQueue;
}

/******************************************************************************
 *
 ******************************************************************************/
// V3.6 is identical to V3.5
Return<void> SampleSession::configureStreams(
    const ::android::hardware::camera::device::V3_5::StreamConfiguration&
        requestedConfiguration,
    ::android::hardware::camera::device::V3_6::ICameraDeviceSession::
        configureStreams_3_6_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  auto isPreviewStream = [&](const auto& stream) -> bool {
    if (stream.v3_2.streamType == StreamType::OUTPUT &&
        stream.v3_2.format == ::android::hardware::graphics::common::V1_0::
                                  PixelFormat::IMPLEMENTATION_DEFINED &&
        (((static_cast<uint64_t>(stream.v3_2.usage) &
           ::android::hardware::graphics::common::V1_0::BufferUsage::
               GPU_TEXTURE) ==
          static_cast<uint64_t>(::android::hardware::graphics::common::V1_0::
                                    BufferUsage::GPU_TEXTURE)) ||
         ((static_cast<uint64_t>(stream.v3_2.usage) &
           ::android::hardware::graphics::common::V1_0::BufferUsage::
               COMPOSER_CLIENT_TARGET) ==
          static_cast<uint64_t>(::android::hardware::graphics::common::V1_0::
                                    BufferUsage::COMPOSER_CLIENT_TARGET)))) {
      return true;
    }
    return false;
  };

  // step1. prepare input parameters for mtk zone
  ::android::hardware::camera::device::V3_5::StreamConfiguration
      customStreamConfiguration;
  std::vector<android::hardware::camera::device::V3_4::Stream>
      cusStreams;  // customStreamConfiguration.streams;
  customStreamConfiguration.v3_4.operationMode =
      requestedConfiguration.v3_4.operationMode;
  // customStreamConfiguration.sessionParams =
  // requestedConfiguration.sessionParams;
  customStreamConfiguration.streamConfigCounter =
      requestedConfiguration.streamConfigCounter;

  mStreamConfigCounter = requestedConfiguration.streamConfigCounter;

  //  Determine if instance is multicam
  android::sp<IMetadataProvider> pMetadataProvider;
  MBOOL is_MultiCam = MFALSE;

  pMetadataProvider = IMetadataProvider::create(mInstanceId);
  if (pMetadataProvider) {
    IMetadata::IEntry const& capbilities =
        pMetadataProvider->getMtkStaticCharacteristics().entryFor(
            MTK_REQUEST_AVAILABLE_CAPABILITIES);
    if (capbilities.isEmpty()) {
      MY_LOGE(
          "No static MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA");
    } else {
      for (MUINT i = 0; i < capbilities.count(); i++) {
        if (capbilities.itemAt(i, Type2Type<MUINT8>()) ==
            MTK_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
          is_MultiCam = MTRUE;
        }
      }
    }
  }

  //  multicam -> pass sensorId    singlecam -> pass instanceId

  MY_LOGD("is_MultiCam:%d", is_MultiCam);

  for (std::vector<int32_t>::const_iterator i = mvSensorId.begin();
       i != mvSensorId.end(); ++i) {
    MY_LOGD("mvSensorId_in_Sample_Session:%d", *i);
  }
  MY_LOGD("mInstanceId_in_Sample_Session:%d", mInstanceId);

  v3::NativeDev::StreamConfiguration nativeStreamConfiguration;
  if (is_MultiCam == MFALSE) {
    nativeStreamConfiguration.SensorIdSet = {mInstanceId};
  } else {
    nativeStreamConfiguration.SensorIdSet = {mvSensorId};
  }

  // determine customization streams
  // key: stream.id, value: settings for this stream
  std::map<int32_t, std::vector<int64_t>> streamSettingMap;

  // step2. prepare tuning stream
  // 2.1    tuning stream for HAL3+ Stream of HIDL structure
  int32_t customStreamId =
      (requestedConfiguration.v3_4.streams[0].v3_2.id + 1) * 1000;
  mTuningStream.v3_2.id = customStreamId++;  // cannot be the same with this and
                                             // previous configured streams
  mTuningStream.v3_2.streamType = StreamType::OUTPUT;
  mTuningStream.v3_2.width = 1280;  // hard code
  mTuningStream.v3_2.height = 720;  // hard code
#if 1
  mTuningStream.v3_2.format = ::android::hardware::graphics::common::V1_0::
      PixelFormat::YV12;  // HAL_PIXEL_FORMAT_YV12
  mTuningStream.v3_2.usage = ::android::hardware::hidl_bitfield<
      ::android::hardware::graphics::common::V1_0::BufferUsage>(
      ::android::hardware::graphics::common::V1_0::BufferUsage::CPU_READ_OFTEN |
      ::android::hardware::graphics::common::V1_0::BufferUsage::
          CPU_READ_RARELY);
  mTuningStream.v3_2.dataSpace = ::android::hardware::hidl_bitfield<
      ::android::hardware::graphics::common::V1_0::Dataspace>(
      ::android::hardware::graphics::common::V1_0::Dataspace::UNKNOWN);
#else
  mTuningStream.v3_2.format =
      ::android::hardware::graphics::common::V1_0::PixelFormat::BLOB;
  // mTuningStream.v3_2.usage  =
  // static_cast<BufferUsageFlags>(::android::hardware::graphics::common::V1_0::BufferUsage::CPU_READ_OFTEN);
  // mTuningStream.v3_2.dataSpace =
  // static_cast<DataspaceFlags>(::android::hardware::graphics::common::V1_0::Dataspace::UNKNOWN);
#endif
  mTuningStream.v3_2.rotation = StreamRotation::ROTATION_0;
  mTuningStream.physicalCameraId = {};
  mTuningStream.bufferSize = 0;
  // 2.2    tuning stream information defined by vendortags
  streamSettingMap[1000].push_back(mTuningStream.v3_2.id);  // STREAM_ID
  streamSettingMap[1000].push_back(0x90000010);             // STREAM_DESC
  streamSettingMap[1000].push_back(1);                      // PARAM_COUNT
  streamSettingMap[1000].push_back(
      MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT);                          // PARAM#1
  streamSettingMap[1000].push_back(MTK_HAL3PLUS_STREAM_FORMAT_BLOB);  // VALUE#1
  cusStreams.push_back(mTuningStream);
  // 2.3    tuning stream for native isp i/f (configuration does not need tuning
  // stream)
  mTuningStream_native.id = mTuningStream.v3_2.id;
  mTuningStream_native.size = 1280 * 720;
  mTuningStream_native.stride = {1280 * 720};
  mTuningStream_native.width = 1280 * 720;
  mTuningStream_native.height = 1;
  mTuningStream_native.format =
      static_cast<uint32_t>(MTK_HAL3PLUS_STREAM_FORMAT_BLOB);
  mTuningStream_native.transform = 0;
  mTuningStream_native.batchsize = 1;
#warning "Native ISP does not need tuning stream configuration"
  // nativeStreamConfiguration.InputStreams.push_back(mTuningStream_native);

  // step3. prepare image streams (example for preview stream)
  for (auto& inStream : requestedConfiguration.v3_4.streams) {
    // preview stream: create working stream for mtk zone
    MY_LOGD("%s", toString(inStream).c_str());
    if (isPreviewStream(inStream)) {
      MY_LOGD("preview streamId: %u", inStream.v3_2.id);
      mPreviewWorkingStream.v3_2.id = customStreamId++;
      mPreviewWorkingStream.v3_2.streamType = inStream.v3_2.streamType;
      mPreviewWorkingStream.v3_2.width = inStream.v3_2.width;
      mPreviewWorkingStream.v3_2.height = inStream.v3_2.height;
      mPreviewWorkingStream.v3_2.format = inStream.v3_2.format;
      mPreviewWorkingStream.v3_2.usage = inStream.v3_2.usage;
      mPreviewWorkingStream.v3_2.dataSpace = inStream.v3_2.dataSpace;
      mPreviewWorkingStream.v3_2.rotation = inStream.v3_2.rotation;
      mPreviewWorkingStream.physicalCameraId = inStream.physicalCameraId;
      mPreviewWorkingStream.bufferSize = inStream.bufferSize;
      cusStreams.push_back(mPreviewWorkingStream);
      mPreviewStream = inStream;

      // preview stream for native isp i/f (hard code w/ YV12)
      // mPreviewInStream_native
      mPreviewInStream_native.id = mPreviewWorkingStream.v3_2.id;
      mPreviewInStream_native.size =
          inStream.v3_2.width * inStream.v3_2.height * 3 / 2;
      mPreviewInStream_native.stride = {inStream.v3_2.width,
                                        inStream.v3_2.width / 2,
                                        inStream.v3_2.width / 2};
      mPreviewInStream_native.width = inStream.v3_2.width;
      mPreviewInStream_native.height = inStream.v3_2.height;
      mPreviewInStream_native.format =
          static_cast<uint32_t>(MTK_HAL3PLUS_STREAM_FORMAT_NV21);
      mPreviewInStream_native.transform = 0;
      mPreviewInStream_native.batchsize = 1;

      // mPreviewOutStream_native
      mPreviewOutStream_native.id = inStream.v3_2.id;
      mPreviewOutStream_native.size =
          inStream.v3_2.width * inStream.v3_2.height * 3 / 2;
      mPreviewOutStream_native.stride = {inStream.v3_2.width,
                                         inStream.v3_2.width / 2,
                                         inStream.v3_2.width / 2};
      mPreviewOutStream_native.width = inStream.v3_2.width;
      mPreviewOutStream_native.height = inStream.v3_2.height;
      mPreviewOutStream_native.format =
          static_cast<uint32_t>(MTK_HAL3PLUS_STREAM_FORMAT_NV21);
      mPreviewOutStream_native.transform = 0;
      mPreviewOutStream_native.batchsize = 1;
      nativeStreamConfiguration.InputStreams.push_back(mPreviewInStream_native);
      nativeStreamConfiguration.OutputStreams.push_back(
          mPreviewOutStream_native);
    } else {  // other streams: directly pass to mtk zone
      MY_LOGD("others streamId: %u", inStream.v3_2.id);
      cusStreams.push_back(inStream);
    }
  }
  customStreamConfiguration.v3_4.streams.setToExternal(cusStreams.data(),
                                                       cusStreams.size());

  // step4. prepare metadata
  // 4.1    transter from streamSettingMap to one vector (consecutive memory)
  std::vector<int64_t> streamSettings;
  streamSettings.push_back(streamSettingMap.size());
  for (const auto& onStream : streamSettingMap) {
    std::copy(onStream.second.begin(), onStream.second.end(),
              back_inserter(streamSettings));
  }

  // 4.2    clone from original input sessionParams from framework
  const camera_metadata* inputSessionParams =
      reinterpret_cast<const camera_metadata_t*>(
          requestedConfiguration.v3_4.sessionParams.data());
  MY_LOGD(
      "inputSessionParams size(%zu)/entry count(%zu)/capacity(%zu); data "
      "count(%zu)/capacity(%zu)",
      get_camera_metadata_size(inputSessionParams),
      get_camera_metadata_entry_count(inputSessionParams),
      get_camera_metadata_entry_capacity(inputSessionParams),
      get_camera_metadata_data_count(inputSessionParams),
      get_camera_metadata_data_capacity(inputSessionParams));

  // camera_metadata* cusSessionParams =
  // clone_camera_metadata(inputSessionParams);
  camera_metadata* cusSessionParams = allocate_camera_metadata(
      get_camera_metadata_entry_capacity(inputSessionParams) + 10,
      get_camera_metadata_data_capacity(inputSessionParams) + 8 * 10);
  MY_LOGD(
      "cusSessionParams size(%zu)/entry count(%zu)/capacity(%zu); data "
      "count(%zu)/capacity(%zu)",
      get_camera_metadata_size(cusSessionParams),
      get_camera_metadata_entry_count(cusSessionParams),
      get_camera_metadata_entry_capacity(cusSessionParams),
      get_camera_metadata_data_count(cusSessionParams),
      get_camera_metadata_data_capacity(cusSessionParams));
  auto app_ret = append_camera_metadata(cusSessionParams, inputSessionParams);
  MY_LOGD(
      "appended cusSessionParams size(%zu)/entry count(%zu)/capacity(%zu); "
      "data count(%zu)/capacity(%zu)",
      get_camera_metadata_size(cusSessionParams),
      get_camera_metadata_entry_count(cusSessionParams),
      get_camera_metadata_entry_capacity(cusSessionParams),
      get_camera_metadata_data_count(cusSessionParams),
      get_camera_metadata_data_capacity(cusSessionParams));
  MY_LOGE_IF(app_ret != OK, "append_camera_metadata fail");

  // 4.3    add hal3plus vendortags
  auto result =
      add_camera_metadata_entry(cusSessionParams, MTK_HAL3PLUS_STREAM_SETTINGS,
                                streamSettings.data(), streamSettings.size());
  MY_LOGE_IF(result != OK,
             "add_camera_metadata_entry fail, check entry/data capacity");

  uint8_t ispTuningDataEnable = 2;
  result = add_camera_metadata_entry(cusSessionParams,
                                     MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_ENABLE,
                                     &ispTuningDataEnable, 1);
  MY_LOGE_IF(result != OK,
             "add_camera_metadata_entry fail, check entry/data capacity");
  camera_metadata* nativeConfiureParams =
      clone_camera_metadata(cusSessionParams);

  hidl_vec<uint8_t> hidl_meta;
  hidl_meta.setToExternal(reinterpret_cast<uint8_t*>(
                              const_cast<camera_metadata*>(cusSessionParams)),
                          get_camera_metadata_size(cusSessionParams));
  customStreamConfiguration.v3_4.sessionParams = std::move(hidl_meta);
  nativeStreamConfiguration.pConfigureParams = nativeConfiureParams;

  MY_LOGD("======== dump all streams ========");
  for (auto& stream : requestedConfiguration.v3_4.streams)
    MY_LOGI("%s", toString(stream).c_str());

  MY_LOGD("======== dump all customed streams ========");
  for (auto& stream : customStreamConfiguration.v3_4.streams)
    MY_LOGI("%s", toString(stream).c_str());

  // step5. call MTK ICameraDeviceSession::configureStreams_3_6
  auto ret = mSession->configureStreams_3_6(
      customStreamConfiguration,
      [&](auto status, auto& halStreamConfiguration) {
        // receive HAL3-like configuration results
        for (auto& halstream : halStreamConfiguration.streams)
          MY_LOGI("%s", toString(halstream).c_str());
        //
        ::android::hardware::camera::device::V3_6::HalStreamConfiguration
            customHalStreamConfiguration;
        std::vector<::android::hardware::camera::device::V3_6::HalStream>
            halStreams;
        for (auto& outStream : halStreamConfiguration.streams) {
          if (outStream.v3_4.v3_3.v3_2.id == mPreviewWorkingStream.v3_2.id) {
            // assume output will be the same with working stream
            ::android::hardware::camera::device::V3_6::HalStream
                outPreviewStream;
            outPreviewStream.v3_4.v3_3.v3_2.id = mPreviewStream.v3_2.id;
            outPreviewStream.v3_4.v3_3.v3_2.overrideFormat =
                outStream.v3_4.v3_3.v3_2.overrideFormat;
            outPreviewStream.v3_4.v3_3.v3_2.producerUsage =
                outStream.v3_4.v3_3.v3_2.producerUsage;
            outPreviewStream.v3_4.v3_3.v3_2.consumerUsage =
                outStream.v3_4.v3_3.v3_2.consumerUsage;
            outPreviewStream.v3_4.v3_3.v3_2.maxBuffers =
                outStream.v3_4.v3_3.v3_2.maxBuffers;
            outPreviewStream.v3_4.v3_3.overrideDataSpace =
                outStream.v3_4.v3_3.overrideDataSpace;
            outPreviewStream.v3_4.physicalCameraId =
                outStream.v3_4.physicalCameraId;
            outPreviewStream.supportOffline = outStream.supportOffline;
            halStreams.push_back(outPreviewStream);
          } else {
            halStreams.push_back(outStream);
          }
        }
        customHalStreamConfiguration.streams.setToExternal(halStreams.data(),
                                                           halStreams.size());
        for (auto& halstream : customHalStreamConfiguration.streams)
          MY_LOGI("%s", toString(halstream).c_str());
        _hidl_cb(status, customHalStreamConfiguration);
      });

#warning "workaround sleep for async pipeline configuration"
  usleep(100 * 1000);

  // step6. configure native device
  mNativeCallback->mParent = this;
  auto retNative =
      mNativeDevice->configure(nativeStreamConfiguration, mNativeCallback);
  if (retNative != 0) {
    MY_LOGE("configure native device fail");
  }
  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> SampleSession::processCaptureRequest(
    const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
        requests,
    const hidl_vec<BufferCache>& cachesToRemove,
    const std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
    android::hardware::camera::device::V3_4::ICameraDeviceSession::
        processCaptureRequest_3_4_cb _hidl_cb) {
  MY_LOGD("+");
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();
  uint32_t numRequestProcessed = 0;

  MY_LOGD("======== dump all requests ========");
  for (auto& request : requests) {
    MY_LOGI("%s", toString(request).c_str());
    for (auto& streambuffer : request.v3_2.outputBuffers)
      MY_LOGI("%s", toString(streambuffer).c_str());
  }

  status_t status = OK;
  {
    if (CC_UNLIKELY(mSession == nullptr)) {
      MY_LOGE("Bad CameraDevice3Session");
      _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
    }

    hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>
        cusCaptureRequests;
    cusCaptureRequests.resize(requests.size());
    MY_LOGD("preserve cusCaptureRequests size(%zu) from requests size(%zu)",
            cusCaptureRequests.size(), requests.size());

    for (size_t i = 0; i < requests.size(); i++) {
      // step1. prepare input parameters for mtk zone
      auto& cusCaptureRequest = cusCaptureRequests[i];
      cusCaptureRequest.v3_2.frameNumber = requests[i].v3_2.frameNumber;
      cusCaptureRequest.v3_2.fmqSettingsSize =
          requests[i].v3_2.fmqSettingsSize;  // MUST be zero, use settings
      cusCaptureRequest.v3_2.settings = requests[i].v3_2.settings;
      cusCaptureRequest.v3_2.inputBuffer = requests[i].v3_2.inputBuffer;
      // cusCaptureRequest.v3_2.outputBuffers.resize(requests[i].v3_2.outputBuffers.size()+1);
      cusCaptureRequest.physicalCameraSettings =
          requests[i].physicalCameraSettings;

      // Native ISP I/F preparation
      std::lock_guard<std::mutex> _l(mInflightMapLock);

      auto& inflightReq = mInflightRequests[requests[i].v3_2.frameNumber];
      auto& postProc = inflightReq.mPostProc;
      inflightReq.frameNumber = requests[i].v3_2.frameNumber;

      auto& cusStreamBuffers = cusCaptureRequest.v3_2.outputBuffers;
      cusStreamBuffers.resize(requests[i].v3_2.outputBuffers.size());
      size_t imgCount = 0;

      for (auto& streamBuffer : requests[i].v3_2.outputBuffers) {
        // preview stream: create working buffer for mtk zone
        // prepare image streams (example for preview stream)
        if (streamBuffer.streamId == mPreviewStream.v3_2.id) {
          cusStreamBuffers.resize(requests[i].v3_2.outputBuffers.size() + 1);
          // step2. prepare tuning buffer
          // 2.1    streambuffer
          auto& tuningStreamBuffer = cusStreamBuffers[imgCount++];
          tuningStreamBuffer.streamId = mTuningStream.v3_2.id;
          tuningStreamBuffer.bufferId = mTuningStreamBufferSerialNo++;
          tuningStreamBuffer.status =
              ::android::hardware::camera::device::V3_2::BufferStatus::OK;
          tuningStreamBuffer.acquireFence = hidl_handle();
          tuningStreamBuffer.releaseFence = hidl_handle();

          // 2.2    allocate buffer
          mAllocator.allocateGraphicBuffer(
              mTuningStream.v3_2.width, mTuningStream.v3_2.height,
              mTuningStream.v3_2.usage,
              static_cast<
                  ::android::hardware::graphics::common::V1_2::PixelFormat>(
                  mTuningStream.v3_2.format),
              &tuningStreamBuffer.buffer);
          MY_LOGI("tuning streambuffer: %s",
                  toString(tuningStreamBuffer).c_str());
          inflightReq.mTuningBufferHd = tuningStreamBuffer.buffer;

          // step3. prepare preview image streams
          // StreamBuffer previewWorkingStreamBuffer;
          auto& previewWorkingStreamBuffer = cusStreamBuffers[imgCount++];
          // 3.1 streambuffer
          previewWorkingStreamBuffer.streamId = mPreviewWorkingStream.v3_2.id;
          previewWorkingStreamBuffer.bufferId =
              mPreviewWorkingStreamBufferSerialNo++;
          previewWorkingStreamBuffer.status =
              ::android::hardware::camera::device::V3_2::BufferStatus::OK;
          previewWorkingStreamBuffer.acquireFence = hidl_handle();
          previewWorkingStreamBuffer.releaseFence = hidl_handle();

          // 3.2 allocate buffer
          mAllocator.allocateGraphicBuffer(
              mPreviewWorkingStream.v3_2.width,
              mPreviewWorkingStream.v3_2.height,
              mPreviewWorkingStream.v3_2.usage,
              static_cast<
                  ::android::hardware::graphics::common::V1_2::PixelFormat>(
                  ::android::hardware::graphics::common::V1_0::PixelFormat::
                      YCRCB_420_SP),
              &previewWorkingStreamBuffer.buffer);
          MY_LOGI("preview working streambuffer: %s",
                  toString(previewWorkingStreamBuffer).c_str());
          inflightReq.mPreviewBufferHd = previewWorkingStreamBuffer.buffer;

          // MY_LOGD("preview working buffer");
          // 3.3 prepare destination preview buffer for Native ISP
          v3::NativeDev::NativeBuffer nativeBuf_preview = {
              .pBuffer = reinterpret_cast<void*>(const_cast<native_handle_t*>(
                  streamBuffer.buffer.getNativeHandle())),
              .type = v3::NativeDev::BufferType::GRAPHIC_HANDLE,
              .offset = 0,
              .bufferSize = mPreviewWorkingStream.v3_2.width *
                            mPreviewWorkingStream.v3_2.height * 3 / 2,
          };
          // enable this flow for preview request
          postProc.numInputBuffers = 1;
          postProc.numOutputBuffers = 1;
          postProc.numTuningBuffers = 1;
          postProc.numManualTunings = 0;

          postProc.readyOutputBuffers = 1;
          for (auto& bufferCache : vBufferCache) {
            if (bufferCache.frameNumber == cusCaptureRequest.v3_2.frameNumber) {
              for (size_t i = 0; i < bufferCache.bufferHandleMap.size(); i++) {
                if (bufferCache.bufferHandleMap.keyAt(i) ==
                    streamBuffer.streamId) {
                  inflightReq.mFwkPreviewBufferHd =
                      bufferCache.bufferHandleMap.valueAt(i).bufferHandle;
                }
              }
            }
          }
          inflightReq.mFwkPreviewBufferId = streamBuffer.bufferId;
          MY_LOGI("preview id(%llu) streamBuffer(%s) native_handle(%p)",
                  streamBuffer.bufferId, toString(streamBuffer.buffer).c_str(),
                  streamBuffer.buffer.getNativeHandle());
          MY_LOGI("inflightReq.mFwkPreviewBufferHd(%p)",
                  inflightReq.mFwkPreviewBufferHd);
          MY_LOGI("inflightReq.mPreviewBufferHd(%s)",
                  toString(inflightReq.mPreviewBufferHd).c_str());
          MY_LOGI("inflightReq.mTuningBufferHd(%s)",
                  toString(inflightReq.mTuningBufferHd).c_str());
        } else {  // other streams: directly pass to mtk zone
          cusStreamBuffers[imgCount++] = streamBuffer;
          MY_LOGE("others: %s",
                  toString(cusStreamBuffers[imgCount - 1]).c_str());
        }

        // step4. clone from original input settings from framework
        if (requests[i].v3_2.fmqSettingsSize) {
          MY_LOGE("unexpected metadata transferred by FMQ");
        }

        if (requests[i].v3_2.settings.size()) {
          const camera_metadata* inputSettings =
              reinterpret_cast<const camera_metadata_t*>(
                  requests[i].v3_2.settings.data());
          if (mLastSettings != nullptr) {
            free_camera_metadata(mLastSettings);
            mLastSettings = nullptr;
          }
          // MY_LOGD("get_camera_metadata_size input(%zu) last(%zu)",
          //   get_camera_metadata_size(inputSettings),
          //   get_camera_metadata_size(mLastSettings));
#if 0  // do nothing
          mLastSettings = allocate_camera_metadata(
            get_camera_metadata_entry_capacity(inputSettings),
            get_camera_metadata_data_capacity(inputSettings));
          auto app_ret = append_camera_metadata(mLastSettings, inputSettings);
#else
          mLastSettings = allocate_camera_metadata(
              get_camera_metadata_entry_capacity(inputSettings) + 10,
              get_camera_metadata_data_capacity(inputSettings) + 8 * 10);
          // mLastSettings = clone_camera_metadata(inputSettings);
          // MY_LOGD("inputSettings size(%zu)/entry count(%zu)/capacity(%zu);
          // data count(%zu)/capacity(%zu)",
          //         get_camera_metadata_size(inputSettings),
          //         get_camera_metadata_entry_count(inputSettings),
          //         get_camera_metadata_entry_capacity(inputSettings),
          //         get_camera_metadata_data_count(inputSettings),
          //         get_camera_metadata_data_capacity(inputSettings) );
          auto app_ret = append_camera_metadata(mLastSettings, inputSettings);
          MY_LOGE_IF(app_ret != OK, "append_camera_metadata fail");

          uint8_t ispTuningReqValue = 2;
          auto res = add_camera_metadata_entry(
              mLastSettings, MTK_CONTROL_CAPTURE_ISP_TUNING_REQUEST,
              &ispTuningReqValue, 1);
          MY_LOGE_IF(res != OK,
                     "add MTK_CONTROL_CAPTURE_ISP_TUNING_REQUEST fail, check "
                     "entry/data capacity");

          int32_t forceCap = property_get_int32(
              "vendor.debug.camera.hal3plus.sample.capture", 0);
          if (forceCap) {
            uint8_t captureIntent = MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
            camera_metadata_entry_t entry;
            res = find_camera_metadata_entry(
                mLastSettings, MTK_CONTROL_CAPTURE_INTENT, &entry);
            if (res == NAME_NOT_FOUND) {
              MY_LOGI("add_camera_metadata_entry");
              res = add_camera_metadata_entry(
                  mLastSettings, MTK_CONTROL_CAPTURE_INTENT, &captureIntent, 1);
            } else if (res == OK) {
              MY_LOGI("update_camera_metadata_entry");
              res = update_camera_metadata_entry(mLastSettings, entry.index,
                                                 &captureIntent, 1, &entry);
            }
            MY_LOGE_IF(
                res != OK,
                "MTK_CONTROL_CAPTURE_INTENT fail, check entry/data capacity");
          }
          // MY_LOGD("mLastSettings size(%zu)/entry count(%zu)/capacity(%zu);
          // data count(%zu)/capacity(%zu)",
          //         get_camera_metadata_size(mLastSettings),
          //         get_camera_metadata_entry_count(mLastSettings),
          //         get_camera_metadata_entry_capacity(mLastSettings),
          //         get_camera_metadata_data_count(mLastSettings),
          //         get_camera_metadata_data_capacity(mLastSettings) );

#endif
#warning \
    "implementor must repeat settings if breaking settings transferred by fwk"
          // user could append customized control metadata in cusSettings
          // however, no operation in this case
          hidl_vec<uint8_t> hidl_meta;
          hidl_meta.setToExternal(reinterpret_cast<uint8_t*>(mLastSettings),
                                  get_camera_metadata_size(mLastSettings));
          cusCaptureRequests[i].v3_2.settings = std::move(hidl_meta);
        }
        postProc.pAppCtrl = clone_camera_metadata(mLastSettings);
      }
    }
    MY_LOGD("======== dump all original requests (%zu)========",
            requests.size());
    for (auto& request : requests) {
      MY_LOGI("%s", toString(request).c_str());
      for (auto& streambuffer : request.v3_2.outputBuffers)
        MY_LOGI("%s", toString(streambuffer).c_str());
    }

    MY_LOGD("======== dump all customized requests (%zu)========",
            cusCaptureRequests.size());
    for (auto& request : cusCaptureRequests) {
      MY_LOGI("%s", toString(request).c_str());
      for (auto& streambuffer : request.v3_2.outputBuffers)
        MY_LOGI("%s", toString(streambuffer).c_str());
    }

    auto ret = mSession->processCaptureRequest_3_4(
        cusCaptureRequests, {} /*cachesToRemove*/,
        [&](auto status, auto numRequestProcessed) {
          _hidl_cb(status, numRequestProcessed);
        });

    std::lock_guard<std::mutex> _l(mInflightMapLock);
    for (auto& iter : mInflightRequests) {
      if (iter.second.bRemovable) {
        mInflightRequests.erase(iter.second.frameNumber);
      }
    }
  }

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status> SampleSession::flush() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr)) {
    MY_LOGE("Bad CameraDevice3Session");
    return Status::INTERNAL_ERROR;
  }
  // flush/close HAL3+
  auto status = mSession->flush();
  // flush/close Native ISP
  auto ret = mNativeModule.closeDevice(mNativeDevice);
  if (CC_UNLIKELY(ret != OK)) {
    MY_LOGE("NativeModule.closeDevice() fail");
  }
  mNativeDevice = nullptr;
  MY_LOGD("Native module close dev done");
  //
  // std::vector<::android::hardware::camera::device::V3_2::NotifyMsg> msgs;
  for (auto& inflight : mInflightRequests) {
    inflight.second.dump();
    if (!inflight.second.bRemovable) {
      // notify error
      ::android::hardware::camera::device::V3_2::NotifyMsg hidl_msg = {};
      hidl_msg.type = MsgType::ERROR;
      hidl_msg.msg.error.frameNumber = inflight.second.frameNumber;
      hidl_msg.msg.error.errorStreamId = -1;
      hidl_msg.msg.error.errorCode = ErrorCode::ERROR_REQUEST;
      // msgs.push_back(std::move(hidl_msg));
      auto notifyResult = mCameraDeviceCallback->notify({hidl_msg});
      if (CC_UNLIKELY(!notifyResult.isOk())) {
        MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
                notifyResult.description().c_str());
      }

      // processCaptureResult buffer
      ::android::hardware::camera::device::V3_4::CaptureResult imageResult;
      imageResult.v3_2.frameNumber = inflight.second.frameNumber;
      imageResult.v3_2.fmqResultSize = 0;
      imageResult.v3_2.result = hidl_vec<uint8_t>();
      imageResult.v3_2.outputBuffers.resize(1);
      imageResult.v3_2.inputBuffer = {.streamId = -1};
      imageResult.v3_2.partialResult = 0;
      //
      auto& streamBuffer = imageResult.v3_2.outputBuffers[0];
      streamBuffer.streamId = mPreviewStream.v3_2.id;
      streamBuffer.bufferId = inflight.second.mFwkPreviewBufferId;
      streamBuffer.buffer = nullptr;
      streamBuffer.status =
          ::android::hardware::camera::device::V3_2::BufferStatus::ERROR;
      streamBuffer.acquireFence = nullptr;
      streamBuffer.releaseFence = nullptr;

      auto processCapResult =
          mCameraDeviceCallback->processCaptureResult_3_4({imageResult});
      if (CC_UNLIKELY(!processCapResult.isOk())) {
        MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
                processCapResult.description().c_str());
      }
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
Return<void> SampleSession::close() {
  CAM_TRACE_CALL();
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("+");

  if (CC_UNLIKELY(mSession == nullptr))
    MY_LOGE("Bad CameraDevice3Session");

  auto result = mSession->close();
  if (CC_UNLIKELY(result.isOk())) {
    MY_LOGE("CusCameraDeviceSession close fail");
  }

  if (mNativeDevice != nullptr) {
    auto ret = mNativeModule.closeDevice(mNativeDevice);
    if (CC_UNLIKELY(ret != OK)) {
      MY_LOGE("NativeModule.closeDevice() fail");
    }
    mNativeDevice = nullptr;
    MY_LOGD("Native module close dev done");
  }

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> SampleSession::notify(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) {
  MY_LOGD("+");
  CAM_TRACE_CALL();
  if (msgs.size()) {
    for (auto& msg : msgs) {
      MY_LOGI("msgs: %s", toString(msg).c_str());
    }
    std::vector<::android::hardware::camera::device::V3_2::NotifyMsg> vMsgs;
    for (auto& msg : msgs) {
      bool skip = false;
      if (msg.type ==
          ::android::hardware::camera::device::V3_2::MsgType::ERROR) {
        if (msg.msg.error.errorCode == ::android::hardware::camera::device::
                                           V3_2::ErrorCode::ERROR_BUFFER) {
          if (msg.msg.error.errorStreamId == mTuningStream.v3_2.id) {
            // do not callback (custom zone's stream)
          } else if (msg.msg.error.errorStreamId ==
                     mPreviewWorkingStream.v3_2.id) {
            ::android::hardware::camera::device::V3_2::NotifyMsg hidl_msg = {};
            hidl_msg.type = msg.type;
            hidl_msg.msg.error.frameNumber = msg.msg.error.frameNumber;
            hidl_msg.msg.error.errorStreamId = mPreviewStream.v3_2.id;
            hidl_msg.msg.error.errorCode = msg.msg.error.errorCode;
            vMsgs.push_back(std::move(hidl_msg));
          } else {
            vMsgs.push_back(msg);
          }
        } else if (msg.msg.error.errorCode ==
                   ::android::hardware::camera::device::V3_2::ErrorCode::
                       ERROR_REQUEST) {
          mInflightRequests[msg.msg.error.frameNumber].bRemovable = true;
          vMsgs.push_back(msg);
        } else {
          vMsgs.push_back(msg);
        }
      } else {
        vMsgs.push_back(msg);
      }
    }
    hidl_vec<::android::hardware::camera::device::V3_2::NotifyMsg> vHidlMsgs;
    vHidlMsgs.setToExternal(vMsgs.data(), vMsgs.size());
    for (auto& msg : vHidlMsgs) {
      MY_LOGI("sampleMsgs: %s", toString(msg).c_str());
    }
    MY_LOGI("mCameraDeviceCallback(%p)", mCameraDeviceCallback.get());
    auto result = mCameraDeviceCallback->notify(vHidlMsgs);
    if (CC_UNLIKELY(!result.isOk())) {
      MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s",
              result.description().c_str());
    }
  }

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
auto SampleSession::InflightRequest::dump() -> void {
  ALOGI("InflightRequest:%s: frameNumber(%u) mPostProc(%p) bRemovable(%d)",
        __FUNCTION__, frameNumber, &mPostProc, bRemovable);

  ALOGI("InflightRequest:%s: PostProc.pAppCtrl(%p) PostProc.pAppResult(%p)",
        __FUNCTION__, mPostProc.pAppCtrl, mPostProc.pAppResult);

  ALOGI(
      "InflightRequest:%s: PostProc.numInputBuffers(%zu) "
      "PostProc.numOutputBuffers(%zu) PostProc.numTuningBuffers(%zu) "
      "PostProc.numManualTunings(%zu)",
      __FUNCTION__, mPostProc.numInputBuffers, mPostProc.numOutputBuffers,
      mPostProc.numTuningBuffers, mPostProc.numManualTunings);

  ALOGI(
      "InflightRequest:%s: PostProc.readyInputBuffers(%zu) "
      "PostProc.readyOutputBuffers(%zu) PostProc.readyTuningBuffers(%zu) "
      "PostProc.readyManualTunings(%zu)",
      __FUNCTION__, mPostProc.readyInputBuffers, mPostProc.readyOutputBuffers,
      mPostProc.readyTuningBuffers, mPostProc.readyManualTunings);

  ALOGI("InflightRequest:%s: mFwkPreviewBufferHd(%p) mFwkPreviewBufferId(%llu)",
        __FUNCTION__, mFwkPreviewBufferHd, mFwkPreviewBufferId);

  ALOGI("InflightRequest:%s: mPreviewBufferHd(%s) mTuningBufferHd(%s)",
        __FUNCTION__, toString(mPreviewBufferHd).c_str(),
        toString(mTuningBufferHd).c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
static void dumpProcRequest(
    std::shared_ptr<NSCam::v3::NativeDev::ProcRequest>& procRequest) {
  if (procRequest.get()) {
    ALOGI("procRequest(%p) reqNumber(%u) pAppCtrl(%p)", procRequest.get(),
          procRequest->reqNumber, procRequest->pAppCtrl);
    ALOGI("procRequest Inputs(%p) InputRequest.size(%zu)", &procRequest->Inputs,
          procRequest->Inputs.in_data.size());
    for (size_t i = 0; i < procRequest->Inputs.in_data.size(); i++) {
      auto& inputRequest = procRequest->Inputs.in_data[i];
      ALOGI("\tInputRequest[%zu].buffers.size(%zu)", i,
            inputRequest.buffers.size());
      for (size_t j = 0; j < inputRequest.buffers.size(); j++) {
        auto& inputBufferSet = inputRequest.buffers[j];
        ALOGI("\t\tInputRequest[%zu].buffers[%zu].buffers.size(%zu)", i, j,
              inputBufferSet.buffers.size());
        if (inputBufferSet.buffers.size()) {
          auto& buffer = inputBufferSet.buffers[0];
          ALOGI("\t\t\tBuffer(%p) streamId(%llu) buffer(%p).size(%zu)", &buffer,
                buffer.streamId, buffer.buffer, buffer.buffer->planeBuf.size());
          for (auto& nativeBuffer : buffer.buffer->planeBuf)
            ALOGI("\t\t\tNativeBuffer.pBuffer(%p) type(%s) bufferSize(%u)",
                  nativeBuffer.pBuffer,
                  (nativeBuffer.type ==
                   NSCam::v3::NativeDev::BufferType::GRAPHIC_HANDLE)
                      ? "GRAPHIC_HANDLE"
                      : "UNKNOWN",
                  nativeBuffer.bufferSize);
        }
        ALOGI("\t\t\ttuningBuffer(%p).size(%zu)", inputBufferSet.tuningBuffer,
              inputBufferSet.tuningBuffer->planeBuf.size());
        ALOGI("\t\t\tNativeBuffer.pBuffer(%p) type(%s) bufferSize(%u)",
              inputBufferSet.tuningBuffer->planeBuf[0].pBuffer,
              (inputBufferSet.tuningBuffer->planeBuf[0].type ==
               NSCam::v3::NativeDev::BufferType::GRAPHIC_HANDLE)
                  ? "GRAPHIC_HANDLE"
                  : "UNKNOWN",
              inputBufferSet.tuningBuffer->planeBuf[0].bufferSize);
        // ALOGI("\t\t\tmanualTuning(%p).size(%zu)",
        //   inputBufferSet.manualTuning,
        //   inputBufferSet.manualTuning->planeBuf.size());
        ALOGI("\t\t\tptuningMeta(%p)", inputBufferSet.ptuningMeta);
      }
    }

    ALOGI("procRequest Outputs(%p) OutputBuffer.size(%zu)",
          &procRequest->Outputs, procRequest->Outputs.buffers.size());
    for (size_t i = 0; i < procRequest->Outputs.buffers.size(); i++) {
      auto& OutputBuffer = procRequest->Outputs.buffers[i];
      ALOGI(
          "\tOutputBuffer[%zu].buffer(%p) streamId(%llu) buffer(%p).size(%zu)",
          i, &OutputBuffer.buffer, OutputBuffer.buffer.streamId,
          OutputBuffer.buffer.buffer,
          OutputBuffer.buffer.buffer->planeBuf.size());

      for (auto& nativeBuffer : OutputBuffer.buffer.buffer->planeBuf)
        ALOGI("\tNativeBuffer.pBuffer(%p) type(%s) bufferSize(%u)",
              nativeBuffer.pBuffer,
              (nativeBuffer.type ==
               NSCam::v3::NativeDev::BufferType::GRAPHIC_HANDLE)
                  ? "GRAPHIC_HANDLE"
                  : "UNKNOWN",
              nativeBuffer.bufferSize);
    }

  } else {
    ALOGE("invalid procRequest");
  }
}

/******************************************************************************
 *
 ******************************************************************************/
void SampleSession::processOneCaptureResult(
    const ::android::hardware::camera::device::V3_4::CaptureResult& result) {
  struct BufferInfo {
    int32_t streamId = -1;
    uint64_t bufferId = 0;
    bool bError = true;
  };

  auto isBufferOK = [](auto& streamBuffer) -> bool {
    return streamBuffer.status ==
           ::android::hardware::camera::device::V3_2::BufferStatus::OK;
  };
  //
  {
    std::lock_guard<std::mutex> _l(mInflightMapLock);
    auto& inflightReq = mInflightRequests[result.v3_2.frameNumber];
    auto& postProc = inflightReq.mPostProc;
    // callback for images
    std::vector<BufferInfo> vBufInfos;
    std::vector<::android::hardware::camera::device::V3_2::StreamBuffer>
        vOutBuffers;

    // output streams
    if (result.v3_2.outputBuffers.size()) {
      for (auto& streamBuffer : result.v3_2.outputBuffers) {
        if (streamBuffer.streamId == mPreviewWorkingStream.v3_2.id) {
          // preview buffer
          if (isBufferOK(streamBuffer)) {
            postProc.readyInputBuffers++;
          } else {
            // prepare error buffer for framework stream
            vBufInfos.push_back({.streamId = mPreviewStream.v3_2.id,
                                 .bufferId = inflightReq.mFwkPreviewBufferId,
                                 .bError = true});
            MY_LOGI("replace error streambuffer id(%d) to id(%d)",
                    mPreviewWorkingStream.v3_2.id, mPreviewStream.v3_2.id);
          }
        } else if (streamBuffer.streamId == mTuningStream.v3_2.id) {
          // tuning buffer
          if (isBufferOK(streamBuffer)) {
            postProc.readyTuningBuffers++;
          }
        } else {
          vBufInfos.push_back({.streamId = streamBuffer.streamId,
                               .bufferId = streamBuffer.bufferId,
                               .bError = (streamBuffer.status ==
                                          ::android::hardware::camera::device::
                                              V3_2::BufferStatus::ERROR)});
        }
      }
      if (vBufInfos.size()) {
        ::android::hardware::camera::device::V3_4::CaptureResult imageResult;
        imageResult.v3_2.frameNumber = result.v3_2.frameNumber;
        imageResult.v3_2.fmqResultSize = 0;
        imageResult.v3_2.result = hidl_vec<uint8_t>();
        imageResult.v3_2.outputBuffers.resize(vBufInfos.size());
        imageResult.v3_2.inputBuffer = {.streamId = -1};
        imageResult.v3_2.partialResult = 0;

        for (size_t i = 0; i < vBufInfos.size(); ++i) {
          auto& streamBuffer = imageResult.v3_2.outputBuffers[i];
          streamBuffer.streamId = vBufInfos[i].streamId;
          streamBuffer.bufferId = vBufInfos[i].bufferId;
          streamBuffer.buffer = nullptr;
          streamBuffer.status =
              (vBufInfos[i].bError)
                  ? ::android::hardware::camera::device::V3_2::BufferStatus::
                        ERROR
                  : ::android::hardware::camera::device::V3_2::BufferStatus::OK;
          streamBuffer.acquireFence = nullptr;
          streamBuffer.releaseFence = nullptr;
        }

        ALOGI("output streams: %s", toString(imageResult).c_str());
        if (mCameraDeviceCallback.get()) {
          auto ret =
              mCameraDeviceCallback->processCaptureResult_3_4({imageResult});
          if (CC_UNLIKELY(!ret.isOk())) {
            ALOGE(
                "Transaction error in "
                "ICameraDeviceCallback::processCaptureResult_3_4: "
                "%s",
                ret.description().c_str());
          }
        }
      }
    }

    // input stream
    if (result.v3_2.inputBuffer.streamId != -1) {
      ALOGI("input stream: %s", toString(result).c_str());
      if (mCameraDeviceCallback.get()) {
        auto ret = mCameraDeviceCallback->processCaptureResult_3_4({result});
        if (CC_UNLIKELY(!ret.isOk())) {
          ALOGE(
              "Transaction error in "
              "ICameraDeviceCallback::processCaptureResult_3_4: "
              "%s",
              ret.description().c_str());
        }
      }
    }

    // metadata
    if (result.v3_2.fmqResultSize > 0) {
      MY_LOGE("unexpected callback using FMQ");
    } else if (result.v3_2.result.size()) {
      const camera_metadata* resultMetadata =
          reinterpret_cast<const camera_metadata*>(result.v3_2.result.data());
      int append_result = -1;
      if (append_Result == nullptr) {
        append_Result = clone_camera_metadata(resultMetadata);
      } else {
        size_t total_entry = get_camera_metadata_entry_count(append_Result) +
                             get_camera_metadata_entry_count(resultMetadata);

        size_t total_data = get_camera_metadata_data_count(append_Result) +
                            get_camera_metadata_data_count(resultMetadata);

        size_t entry_cap = get_camera_metadata_entry_capacity(append_Result);

        size_t data_cap = get_camera_metadata_data_capacity(append_Result);

        if (entry_cap < total_entry || data_cap < total_data) {
          MY_LOGD("append_Result space not enough");
          camera_metadata* alloc_AppCtrl =
              allocate_camera_metadata(total_entry, total_data);
          append_result = append_camera_metadata(alloc_AppCtrl, append_Result);
          append_result = append_camera_metadata(alloc_AppCtrl, resultMetadata);
          append_Result = alloc_AppCtrl;
        } else {
          MY_LOGD("append_Result space sufficient");
          append_result = append_camera_metadata(append_Result, resultMetadata);
        }
      }

      if (result.v3_2.partialResult == mAtMostMetaStreamCount) {
        postProc.pAppResult = clone_camera_metadata(append_Result);
        MY_LOGD("PostProc.pAppResult(%p)", postProc.pAppResult);
        MY_LOGD("receive_last_partial");
      }

      // write metadata to fmq
      if (auto pResultMetadataQueue = mResultMetadataQueue.get()) {
        // for (size_t i = 0; i < results.size(); i++) {
        //   auto& hidl_result = results[i];
        // write logical meta into FMQ
        // if (result.v3_2.result.size() > 0) {
        ::android::hardware::camera::device::V3_4::CaptureResult metaResult;
        if (CC_LIKELY(pResultMetadataQueue->write(result.v3_2.result.data(),
                                                  result.v3_2.result.size()))) {
          metaResult.v3_2.frameNumber = result.v3_2.frameNumber;
          metaResult.v3_2.fmqResultSize = result.v3_2.result.size();
          metaResult.v3_2.result = hidl_vec<uint8_t>();
          metaResult.v3_2.outputBuffers = {};
          metaResult.v3_2.inputBuffer = {.streamId = -1};
          metaResult.v3_2.partialResult = 1;
          // metaResult.physicalCameraMetadata;
        } else {
          MY_LOGE(
              "fail to write meta to mResultMetadataQueue, data=%p, size=%zu",
              result.v3_2.result.data(), result.v3_2.result.size());
        }

        if (result.physicalCameraMetadata.size() > 0) {
          for (size_t i = 0; i < result.physicalCameraMetadata.size(); i++) {
            if (CC_LIKELY(pResultMetadataQueue->write(
                    result.physicalCameraMetadata[i].metadata.data(),
                    result.physicalCameraMetadata[i].metadata.size()))) {
              metaResult.physicalCameraMetadata[i].fmqMetadataSize =
                  result.physicalCameraMetadata[i].metadata.size();
              metaResult.physicalCameraMetadata[i].metadata =
                  hidl_vec<uint8_t>();
            } else {
              MY_LOGE(
                  "fail to write physical meta to mResultMetadataQueue, "
                  "data=%p, "
                  "size=%zu",
                  result.physicalCameraMetadata[i].metadata.data(),
                  result.physicalCameraMetadata[i].metadata.size());
            }
          }
        }

        //
        ALOGI("metadata: %s", toString(metaResult).c_str());
        if (mCameraDeviceCallback.get()) {
          auto ret =
              mCameraDeviceCallback->processCaptureResult_3_4({metaResult});
          if (CC_UNLIKELY(!ret.isOk())) {
            ALOGE(
                "Transaction error in "
                "ICameraDeviceCallback::processCaptureResult_3_4: "
                "%s",
                ret.description().c_str());
          }
        }
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void> SampleSession::processCaptureResult(
    const ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::CaptureResult>& results) {
  MY_LOGD("+");
  CAM_TRACE_CALL();

  // directly callback w/o FMQ
  MY_LOGI("results(%zu)", results.size());
  for (auto& result : results) {
    processOneCaptureResult(result);

    std::lock_guard<std::mutex> _l(mInflightMapLock);
    auto& inflightReq = mInflightRequests[result.v3_2.frameNumber];
    auto& postProc = inflightReq.mPostProc;
    // Native ISP processing when all images/metadata are prepared
    inflightReq.dump();
    if (inflightReq.isReadyForPostProc()) {
      auto procRequest = std::make_shared<v3::NativeDev::ProcRequest>();
      procRequest->reqNumber = inflightReq.frameNumber;
      procRequest->pAppCtrl = postProc.pAppCtrl;

      // input
      struct v3::NativeDev::InputRequestSet inputRequestSet;
      struct v3::NativeDev::InputRequest inputReq;
      struct v3::NativeDev::InputBufferSet inputStream;

      struct v3::NativeDev::NativeBuffer nativeBuf;
      nativeBuf.pBuffer = reinterpret_cast<void*>(const_cast<native_handle_t*>(
          inflightReq.mPreviewBufferHd.getNativeHandle()));
      nativeBuf.type = v3::NativeDev::BufferType::GRAPHIC_HANDLE;
      nativeBuf.offset = 0;
      // nativeBuf.bufferSize =
      // mPreviewStream.v3_2.width*mPreviewStream.v3_2.height*3/2;
      nativeBuf.bufferSize = 3136320;  // hard code

      struct v3::NativeDev::BufHandle bufHandle;
      bufHandle.planeBuf = {nativeBuf};

      struct v3::NativeDev::Buffer inputBuffer;
      inputBuffer.streamId = mPreviewWorkingStream.v3_2.id;
      inputBuffer.buffer = &bufHandle;  // hxd
      inputBuffer.pinfo = nullptr;

      inputStream.buffers.push_back(inputBuffer);

      struct v3::NativeDev::NativeBuffer nativeBuf_tuning;
      nativeBuf_tuning.pBuffer =
          reinterpret_cast<void*>(const_cast<native_handle_t*>(
              inflightReq.mTuningBufferHd.getNativeHandle()));
      nativeBuf_tuning.type = v3::NativeDev::BufferType::GRAPHIC_HANDLE;
      nativeBuf_tuning.offset = 0;
      nativeBuf_tuning.bufferSize = 1415040;

      struct v3::NativeDev::BufHandle bufHandle_tuning;
      bufHandle_tuning.planeBuf = {nativeBuf_tuning};

      inputStream.tuningBuffer = &bufHandle_tuning;

      // ptuningMeta
      inputStream.ptuningMeta = postProc.pAppResult;

      inputReq.buffers = {inputStream};
      inputRequestSet.in_data = {inputReq};

      // output buffer part
      struct v3::NativeDev::NativeBuffer nativeBuf_out;
      nativeBuf_out.pBuffer = reinterpret_cast<void*>(
          const_cast<native_handle_t*>(inflightReq.mFwkPreviewBufferHd));
      nativeBuf_out.type = v3::NativeDev::GRAPHIC_HANDLE;
      nativeBuf_out.offset = 0;
      nativeBuf_out.bufferSize = 3136320;  // 461760;

      struct v3::NativeDev::BufHandle bufHandle_out;
      bufHandle_out.planeBuf = {nativeBuf_out};

      struct v3::NativeDev::Buffer outputBuffer;
      outputBuffer.streamId = mPreviewStream.v3_2.id;
      outputBuffer.buffer = &bufHandle_out;
      outputBuffer.pinfo = nullptr;

      struct v3::NativeDev::OutputBuffer nativeOutputBuf;
      nativeOutputBuf.buffer = outputBuffer;
      // nativeOutputBuf.crop  //TODO

      struct v3::NativeDev::OutputBufferSet nativeOutSet;
      nativeOutSet.buffers = {nativeOutputBuf};

      procRequest->Inputs = inputRequestSet;
      procRequest->Outputs = nativeOutSet;

      dumpProcRequest(procRequest);
      //
      std::vector<std::shared_ptr<v3::NativeDev::ProcRequest>> procRequests = {
          procRequest};
      auto ret = mNativeDevice->processRequest(procRequests);
      if (ret != 0)
        MY_LOGE("NativeDevice->processRequest fail...");
    }
  }
  //
  // auto ret = mNativeDevice->processRequest(procRequests);
  // if ( ret!=0 )
  //   MY_LOGE("NativeDevice->processRequest fail...");

  MY_LOGD("-");
  return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
auto SampleSession::setExtCameraDeviceSession(
    const ::android::sp<IExtCameraDeviceSession>& session)
    -> ::android::status_t {
  if (session == nullptr) {
    MY_LOGE("fail to setCameraDeviceSession due to session is nullptr");
    return BAD_VALUE;
  }
  mExtCameraDeviceSession = session;
  return OK;
}

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
int32_t SampleSession::NativeCallback::processCompleted(
    int32_t const& status,
    v3::NativeDev::Result const& result) {
  ALOGD("NativeCallback: %s +", __FUNCTION__);
  if (!mParent)
    ALOGE("NativeCallback:%s: invalid session", __FUNCTION__);

  // std::lock_guard<std::mutex> _l(mParent->mInflightMapLock);
  ALOGI("NativeCallback:%s: Result.reqNumber(%u) presult(%p)", __FUNCTION__,
        result.reqNumber, result.presult);
  if (status == 0) {
    hidl_vec<::android::hardware::camera::device::V3_4::CaptureResult>
        imageHidlResults;
    hidl_vec<::android::hardware::camera::device::V3_4::CaptureResult>
        metadataHidlResults;

    auto frameNumber = result.reqNumber;
    auto& inflightRequest = mParent->mInflightRequests[frameNumber];
    if (inflightRequest.bRemovable == true) {
      ALOGI("already removed before, neglect callback for this frame");
      return 0;
    }

    // prepare buffer callback
    if (inflightRequest.mFwkPreviewBufferHd) {
      ::android::hardware::camera::device::V3_4::CaptureResult captureResult;
      captureResult.v3_2.frameNumber = frameNumber;
      captureResult.v3_2.fmqResultSize = 0;
      captureResult.v3_2.result = hidl_vec<uint8_t>();
      captureResult.v3_2.outputBuffers.resize(1);
      captureResult.v3_2.inputBuffer = {.streamId = -1};
      // captureResult.v3_2.partialResult;
      // captureResult.physicalCameraMetadata;

      auto& streamBuffer = captureResult.v3_2.outputBuffers[0];
      streamBuffer.streamId = mParent->mPreviewStream.v3_2.id;
      streamBuffer.bufferId = inflightRequest.mFwkPreviewBufferId;
      streamBuffer.buffer = nullptr;
      streamBuffer.status =
          ::android::hardware::camera::device::V3_2::BufferStatus::OK;
      streamBuffer.acquireFence = nullptr;
      streamBuffer.releaseFence = nullptr;

      imageHidlResults = {captureResult};
      ALOGI("NativeCallback: %s", toString(captureResult).c_str());
      if (mParent->mCameraDeviceCallback.get()) {
        auto ret = mParent->mCameraDeviceCallback->processCaptureResult_3_4(
            imageHidlResults);
        if (CC_UNLIKELY(!ret.isOk())) {
          ALOGE(
              "Transaction error in "
              "ICameraDeviceCallback::processCaptureResult_3_4: "
              "%s",
              ret.description().c_str());
        }
      }
    }

    // prepare metadata callback
    // if ( result.presult ) {
    {
      ::android::hardware::camera::device::V3_4::CaptureResult captureResult;
      captureResult.v3_2.frameNumber = frameNumber;
      captureResult.v3_2.fmqResultSize = 0;
      // maybe use FMQ
      camera_metadata_t* final = nullptr;
      hidl_vec<uint8_t> hidl_meta;
      if (result.presult) {
        hidl_meta.setToExternal(
            reinterpret_cast<uint8_t*>(
                const_cast<camera_metadata*>(result.presult)),
            get_camera_metadata_size(result.presult));
        captureResult.v3_2.result = std::move(hidl_meta);
      } else {
        final = allocate_camera_metadata(1, 8);
        uint8_t bFrameEnd = 1;
        auto res = add_camera_metadata_entry(final, MTK_HAL3PLUS_REQUEST_END,
                                             &bFrameEnd, 1);
        if (res == OK) {
          hidl_meta.setToExternal(
              reinterpret_cast<uint8_t*>(const_cast<camera_metadata*>(final)),
              get_camera_metadata_size(final));
          captureResult.v3_2.result = std::move(hidl_meta);
        } else {
          ALOGE("%s: fail to prepare final result", __FUNCTION__);
        }
      }
      captureResult.v3_2.outputBuffers = {};
      captureResult.v3_2.inputBuffer = {.streamId = -1};
      captureResult.v3_2.partialResult = mParent->mAtMostMetaStreamCount;

      ALOGD("NativeCallback_mAtMostMetaStreamCount: %d",
            captureResult.v3_2.partialResult);
      // captureResult.physicalCameraMetadata;
      metadataHidlResults = {captureResult};
      ALOGI("NativeCallback: %s", toString(captureResult).c_str());
      if (mParent->mCameraDeviceCallback.get()) {
        auto ret = mParent->mCameraDeviceCallback->processCaptureResult_3_4(
            metadataHidlResults);
        if (CC_UNLIKELY(!ret.isOk())) {
          ALOGE(
              "Transaction error in "
              "ICameraDeviceCallback::processCaptureResult_3_4: "
              "%s",
              ret.description().c_str());
        }
      }
      if (final)
        free_camera_metadata(final);
    }
    {
      // free working buffer
      // mParent->mMapper.freeBuffer(inflightRequest.mPreviewBufferHd.getNativeHandle());
      // mParent->mMapper.freeBuffer(inflightRequest.mTuningBufferHd.getNativeHandle());
      // remove inflightrequest
      inflightRequest.bRemovable = true;
    }
  } else {
    ALOGE("not implemented yet");
  }

  ALOGD("NativeCallback: %s -", __FUNCTION__);
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
int32_t SampleSession::NativeCallback::processPartial(
    int32_t const& status,
    v3::NativeDev::Result const& result,
    std::vector<v3::NativeDev::BufHandle*> const& completedBuf) {
  ALOGE("NativeCallback: %s Not implemented by NativeDevice", __FUNCTION__);
  return 0;
}

};  // namespace sample
};  // namespace feature
};  // namespace custom_dev3
};  // namespace NSCam
