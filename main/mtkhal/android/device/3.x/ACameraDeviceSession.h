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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef MAIN_MTKHAL_ANDROID_DEVICE_3_X_ACAMERADEVICESESSION_H_
#define MAIN_MTKHAL_ANDROID_DEVICE_3_X_ACAMERADEVICESESSION_H_

#include <mtkcam3/main/mtkhal/android/device/3.x/IACameraDeviceSession.h>
#include <main/mtkhal/android/device/3.x/ACameraDeviceCallback.h>
#include <mtkcam3/main/mtkhal/android/device/3.x/utils/AImageStreamInfo.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IIonDevice.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IMetadata.h>
// #include <mtkcam-interfaces/utils/streamSettingsConverter/IStreamSettingsConverter.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ACbAdatpor/IACallbackAdaptor.h"

using mcam::IMtkcamDeviceSession;
using mcam::android::Utils::AImageStreamInfo;
using mcam::IIonDevice;
using NSCam::IMetadata;

namespace mcam {
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
class ACameraDeviceSession : public IACameraDeviceSession,
                             public IMtkcamDeviceCallback {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  struct CreationInfo {
    int32_t instanceId;
    std::shared_ptr<IACameraDeviceCallback> pCallback = nullptr;
  };

  typedef struct ParsedSMVRBatchInfo
  {
      // meta: MTK_SMVR_FEATURE_SMVR_MODE
      MINT32               maxFps = 30;    // = meta:idx=0
      MINT32               p2BatchNum = 1; // = min(meta:idx=1, p2IspBatchNum)
      MINT32               imgW = 0;       // = StreamConfiguration.streams[videoIdx].width
      MINT32               imgH = 0;       // = StreamConfiguration.streams[videoIdx].height
      MINT32               p1BatchNum = 1; // = maxFps / 30
      MINT32               opMode = 0;     // = StreamConfiguration.operationMode
      MINT32               logLevel = 0;   // from property
  } ParsedSMVRBatchInfo;

  // using StreamSettingsInfoMap = NSCam::Utils::StreamSettingsConverter::
  //     IStreamSettingsConverter::StreamSettingsInfoMap;
  using StreamCreationMap =
      std::unordered_map<int32_t, AImageStreamInfo::CreationInfo>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  int32_t mInstanceId = -1;
  std::string const mLogPrefix;
  std::shared_ptr<IMtkcamDeviceSession> mSession = nullptr;
  std::shared_ptr<ACameraDeviceCallback> mCallback = nullptr;
  ::android::sp<IACallbackAdaptor> mACbAdaptor = nullptr;
  mutable std::mutex mACbAdaptorLock;
  ::android::sp<NSCam::IMetadataProvider> mMetadataProvider = nullptr;
  std::map<uint32_t, ::android::sp<NSCam::IMetadataProvider>>
      mPhysicalMetadataProviders;
  int32_t mLogLevel = 0;

  // ion device for allocate graphic buffer
  std::shared_ptr<IIonDevice> mIonDevice = nullptr;

  // Slow Motion Video Recording
  std::shared_ptr<ParsedSMVRBatchInfo> mspParsedSMVRBatchInfo = nullptr;
  // End To End HDR
  MINT32 mE2EHDROn = 0;
  // HAL3PLUS
  // std::shared_ptr<StreamSettingsInfoMap> mspStreamSettingsInfoMap = nullptr;
  // DNG
  // MINT mDngFormat = 0;

  // Preview Buffer Dejitter
  // bool mbIsDejitterEnabled = false;

  std::mutex mImageConfigMapLock;
  std::unordered_map<int64_t, std::shared_ptr<AImageStreamInfo>>
      mImageConfigMap;
  std::unordered_map<int64_t, MINT32> mStreamImageProcessingSettings;

  // TEMP IMPLEMENT : for partial result
  std::mutex mPartialResultMapLock;
  // frameNumber -> partialResult
  std::unordered_map<uint32_t, uint32_t> mPartialResultMap;

  MINT32 mAtMostMetaStreamCount = 1;

  // Hal should always keep the latesting logical setting. If a request
  // brings null setting, hal should use the stored one.
  IMetadata mLatestSettings;

  //  setup during submitting requests.
  mutable std::mutex mRequestingLock;
  std::atomic_int mRequestingAllowed = 0;
  bool m1stRequestNotSent = true;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~ACameraDeviceSession();
  explicit ACameraDeviceSession(const CreationInfo& info);
  static auto create(const CreationInfo& info) -> ACameraDeviceSession*;

 public:  ////        For ACameraDevice
  virtual auto open(const std::shared_ptr<IMtkcamDeviceSession>& session)
      -> int;

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }

  auto getParsedSMVRBatchInfo() -> std::shared_ptr<ParsedSMVRBatchInfo> {
    return mspParsedSMVRBatchInfo;
  }

  virtual auto checkConfigParams(
      const StreamConfiguration& requestedConfiguration) -> int;

  virtual auto checkStream(const Stream& rStream) -> int;

  virtual auto parseStreamConfiguration(
      const StreamConfiguration& configuration,
      HalStreamConfiguration& halConfiguration,
      mcam::StreamConfiguration& mtkConfiguration,
      StreamCreationMap& creationInfos) -> int;

  virtual auto parseStream(
      const Stream& rStream,
      HalStream& rOutStream,
      mcam::Stream& rMtkStream,
      AImageStreamInfo::CreationInfo& creationInfo) -> int;

  virtual auto updateConfiguration(
      const mcam::StreamConfiguration& mtkConfiguration,
      const mcam::HalStreamConfiguration& mtkHalConfiguration,
      HalStreamConfiguration& halConfiguration,
      StreamCreationMap& creationInfos) -> int;

  virtual auto extractSMVRBatchInfo(
      const StreamConfiguration& requestedConfiguration,
      const IMetadata& sessionParams) -> std::shared_ptr<ParsedSMVRBatchInfo>;

  virtual auto extractE2EHDRInfo(
      const StreamConfiguration& requestedConfiguration,
      const IMetadata& sessionParams) -> MINT32;

  // virtual auto extractStreamSettingsInfoMap(const IMetadata& sessionParams)
  //     -> std::shared_ptr<StreamSettingsInfoMap>;

  virtual auto extractDNGFormatInfo(const IMetadata& essionParams) -> MINT;

  virtual auto setStreamImageProcessing(
      mcam::StreamConfiguration& requestedConfiguration) -> void;

  virtual auto setStreamImageProcessing(
      std::vector<mcam::CaptureRequest>& requests) -> void;

  virtual auto getImageStreamInfo(int64_t streamId)
      -> std::shared_ptr<AImageStreamInfo>;

  virtual auto convertImageStreamBuffers(
      const std::vector<CaptureRequest>& srcRequests,
      std::vector<mcam::CaptureRequest>& dstRequests) -> int;

  virtual auto createImageBufferHeap(
      const StreamBuffer& rStreamBuffer,
      const std::shared_ptr<AImageStreamInfo>& pStreamInfo,
      std::shared_ptr<mcam::IImageBufferHeap>& pImageBufferHeap) -> int;

  virtual auto convertToSensorColorOrder(uint8_t colorFilterArrangement)
      -> int32_t;

  // virtual auto mapImageFormat(
  //     uint32_t const& format) -> int32_t;

  // virtual auto updateImgBufferInfo(
  //     const std::shared_ptr<StreamSettingsInfoMap> mspStreamSettingsInfoMap,
  //     NSCam::ImageBufferInfo& imgBufferInfo,
  //     const Stream& rStream) -> bool;

  virtual auto applySettingRule(
      std::vector<mcam::CaptureRequest>& rMktRequests) -> void;

  virtual auto getSafeACallbackAdaptor() const
      -> ::android::sp<IACallbackAdaptor>;

 protected:  ////        Operations.
  auto enableRequesting() -> void;
  auto disableRequesting() -> void;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICameraDeviceSession Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto constructDefaultRequestSettings(
      RequestTemplate type,
      const camera_metadata*& requestTemplate) -> int;

  virtual auto configureStreams(
      const StreamConfiguration& requestedConfiguration,
      HalStreamConfiguration& halConfiguration) -> int;

  virtual auto processCaptureRequest(
      const std::vector<CaptureRequest>& requests,
      uint32_t& numRequestProcessed) -> int;

  virtual auto flush() -> int;

  virtual auto close() -> int;

  virtual auto signalStreamFlush(const std::vector<int32_t>& streamIds,
                                 uint32_t streamConfigCounter) -> void;

  virtual auto isReconfigurationRequired(
      const camera_metadata* const& oldSessionParams,
      const camera_metadata* const& newSessionParams,
      bool& reconfigurationNeeded) -> int;

  virtual auto switchToOffline(
      const std::vector<int64_t>& streamsToKeep,
      CameraOfflineSessionInfo& offlineSessionInfo,
      std::shared_ptr<IACameraOfflineSession>& offlineSession) -> int;

  // WARNING: It should be removed after custom zone moved down.
  virtual auto getConfigurationResults(
      const uint32_t streamConfigCounter,
      ExtConfigurationResults& ConfigurationResults) -> int;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IMtkcamDeviceCallback Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto processCaptureResult(
      const std::vector<mcam::CaptureResult>& mtkResults) -> void;

  virtual auto notify(const std::vector<mcam::NotifyMsg>& mtkMsgs)
      -> void;

#if 0  // not implement HAL Buffer Management
  virtual auto requestStreamBuffers(
      const std::vector<mcam::BufferRequest>& vMtkBufferRequests,
      std::vector<mcam::StreamBufferRet>* pvMtkBufferReturns)
      -> mcam::BufferRequestStatus;

  virtual auto returnStreamBuffers(
      const std::vector<mcam::StreamBuffer>& mtkBuffers) -> void;
#endif
};

};  // namespace android
};  // namespace mcam

#endif  // MAIN_MTKHAL_ANDROID_DEVICE_3_X_ACAMERADEVICESESSION_H_
