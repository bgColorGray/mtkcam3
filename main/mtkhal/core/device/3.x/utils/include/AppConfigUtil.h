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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_APPCONFIGUTIL_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_APPCONFIGUTIL_H_
//
//#include <mtkcam-interfaces/utils/streamSettingsConverter/IStreamSettingsConverter.h>
//
#include <IAppConfigUtil.h>

#include <map>
#include <string>
#include <memory>
#include <vector>

using ::android::String8;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace Utils {

class AppConfigUtil : public IAppConfigUtil {
  friend class IAppConfigUtil;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Definitions.
  struct CommonInfo {
    int32_t mLogLevel = 0;
    int32_t mInstanceId = -1;
    std::vector<int32_t>  mSensorId;
    android::sp<IMetadataProvider> mMetadataProvider = nullptr;
    std::map<uint32_t, android::sp<IMetadataProvider>>
        mPhysicalMetadataProviders;
    android::sp<IMetadataConverter> mMetadataConverter = nullptr;
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

//  using ParsedSMVRBatchInfo = NSCam::v3::pipeline::model::ParsedSMVRBatchInfo;
//  using StreamSettingsInfoMap = NSCam::Utils::StreamSettingsConverter::
//                                IStreamSettingsConverter::StreamSettingsInfoMap;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                Data Members.
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
  std::string const mInstanceName;
  std::shared_ptr<IStreamInfoBuilderFactory> mStreamInfoBuilderFactory;

  MINT mDngFormat = 0;
  MINT32 mE2EHDROn = 0;
  std::shared_ptr<ParsedSMVRBatchInfo> mspParsedSMVRBatchInfo = nullptr;
//  std::shared_ptr<StreamSettingsInfoMap> mspStreamSettingsInfoMap = nullptr;

  IMetadata::IEntry mEntryMinDuration;
  IMetadata::IEntry mEntryStallDuration;

  mutable android::Mutex mImageConfigMapLock;
  ImageConfigMap mImageConfigMap;
  MetaConfigMap mMetaConfigMap;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto destroy() -> void;

  virtual auto beginConfigureStreams(
      const StreamConfiguration& requestedConfiguration,
      HalStreamConfiguration& halConfiguration,
      std::shared_ptr<device::policy::AppUserConfiguration>& rCfgParams)
      -> ::android::status_t;

  virtual auto getAppStreamInfoBuilderFactory() const
      -> std::shared_ptr<IStreamInfoBuilderFactory>;

  /**
   * set Config Map
   */
  virtual auto getConfigMap(ImageConfigMap& imageConfigMap,
                            MetaConfigMap& metaConfigMap,
                            bool isInt32Key) -> void;

//  virtual auto getParsedSMVRBatchInfo()
//    -> std::shared_ptr<ParsedSMVRBatchInfo> {return mspParsedSMVRBatchInfo;}


 private:
  virtual auto getConfigImageStream(StreamId_T streamId) const
      -> android::sp<AppImageStreamInfo>;

  virtual auto getConfigMetaStream(StreamId_T streamId) const
      -> android::sp<AppMetaStreamInfo>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  explicit AppConfigUtil(const CreationInfo& creationInfo);

  auto getLogLevel() const -> int32_t {
    return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0;
  }

  auto extractSMVRBatchInfo(const StreamConfiguration& requestedConfiguration,
                            const IMetadata& sessionParams)
      -> std::shared_ptr<ParsedSMVRBatchInfo>;

  auto extractE2EHDRInfo(const StreamConfiguration& requestedConfiguration,
                         const IMetadata& sessionParams)
      -> MINT32;

  auto extractDNGFormatInfo(const StreamConfiguration& requestedConfiguration)
      -> MINT;

//  auto extractStreamSettingsInfoMap(const IMetadata& sessionParams)
//      -> std::shared_ptr<StreamSettingsInfoMap>;

  auto createImageStreamInfo(const Stream& rStream,
                             HalStream& rOutStream,
                             StreamId_T streamId) const
      -> AppImageStreamInfo*;

  auto createMetaStreamInfo(char const* metaName,
                            StreamId_T suggestedStreamId,
                            MINT physicalId = 0) const
      -> AppMetaStreamInfo*;

  auto addConfigStream(AppImageStreamInfo* pStreamInfo)
                       // bool keepBufferCache
      -> void;

  auto addConfigStream(AppMetaStreamInfo* pStreamInfo) -> void;
};

};      // namespace Utils
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_APPCONFIGUTIL_H_
