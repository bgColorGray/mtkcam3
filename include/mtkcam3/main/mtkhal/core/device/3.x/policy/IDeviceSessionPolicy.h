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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_IDEVICESESSIONPOLICY_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_IDEVICESESSIONPOLICY_H_

#include <mtkcam3/main/mtkhal/core/device/3.x/device/types.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/utils/streambuffer/AppStreamBuffers.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/utils/streaminfo/AppImageStreamInfo.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/policy/types.h>
//#include <mtkcam-android/policy/Types.h>

#include <mtkcam3/pipeline/model/IPipelineModelManager.h>
#include <mtkcam3/pipeline/stream/IStreamBufferProvider.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
//#include <mtkcam-interfaces/utils/hw/IPowerOnOffController.h>

#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>

#include <utils/RefBase.h>

#include <set>
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>

#include "../../../../../../../../main/mtkhal/core/device/3.x/utils/include/ZoomRatioConverter.h"

//using NSCam::IPowerOnOffController;
//using NSCam::v3::pipeline::model::RequestSet;
//using NSCam::v3::pipeline::policy::ParsedStreamInfo_P1;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace device {
namespace policy {
// static info, immutable when bring-up.
struct DeviceSessionStaticInfo {
  int32_t mInstanceId = -1;
  std::vector<int32_t> mSensorId;
  android::sp<IMetadataProvider> mMetadataProvider = nullptr;
  std::map<uint32_t, android::sp<IMetadataProvider>> mPhysicalMetadataProviders;
  android::sp<IMetadataConverter> mMetadataConverter = nullptr;
//  std::weak_ptr<IPowerOnOffController> mPowerOnOffController;
  std::shared_ptr<ZoomRatioConverter> mZoomRatioConverter = nullptr;
  std::shared_ptr<IPrinter> mErrorPrinter;
  std::shared_ptr<IPrinter> mWarningPrinter;
  std::shared_ptr<IPrinter> mDebugPrinter;
  std::shared_ptr<PolicyStaticInfo> mPolicyStaticInfo;
};

// config info, changable only in configuration/reconfiguration stage.
struct DeviceSessionConfigInfo {
  ImageConfigMap mImageConfigMap;
  MetaConfigMap  mMetaConfigMap;
  bool isSecureCameraDevice = false;
  bool mSupportBufferManagement = false;
};

struct ConfigurationInputParams {
  // requested stream from user configuration
  StreamConfiguration const* requestedConfiguration;
  uint64_t mConfigTimestamp;
  // ImageConfigurationMap const* currentImageConfigMap;
};

struct ConfigurationSessionParams {
  // parsing from session params
  bool zslMode = false;
};

struct MetaStreamCreationParams {};

// will be moved to feature decision
struct ImageCreationParams {
  NSCam::ImageBufferInfo imgBufferInfo;
  bool isSecureFlow = false;
};

struct ConfigurationOutputParams {
  // output stream for user configuration
  HalStreamConfiguration* resultConfiguration;
  // pipeline model configuration input params
  std::shared_ptr<PipelineUserConfiguration>* pPipelineUserConfiguration;
  ImageConfigMap* imageConfigMap;
  MetaConfigMap*  metaConfigMap;
};

struct RequestInputParams {
  std::vector<CaptureRequest> const* requests;
};

struct RequestOutputParams {
  android::Vector<Request>*     requests;
//  RequestSet*  requestSet;  // for new design
//  std::set<StreamId_T>*         vskipStreamList;
//  std::vector<uint32_t>*        vskipSensorList;
//  SensorMap<AdditionalRawInfo>* vAdditionalRawInfo;
};

struct ResultInputParams {};

struct ResultOutputParams {};

struct ResultParams {
  UpdateResultParams* params;
  std::vector<android::sp<IMetaStreamBuffer>> halMeta;
};

class IDeviceSessionPolicy {
 public:
  virtual ~IDeviceSessionPolicy() = default;

  virtual auto evaluateConfiguration(ConfigurationInputParams const* in,
                                     ConfigurationOutputParams* out) -> int = 0;

  virtual auto evaluateRequest(RequestInputParams const* in,
                               RequestOutputParams* out) -> int = 0;

  virtual auto processResult(
      ResultParams* params) -> int = 0;

  virtual auto destroy() -> void = 0;

//  virtual auto setPipelineConfigurationData(
//      SensorMap<uint32_t> const& sensorModeMap,
//      SensorMap<ParsedStreamInfo_P1> const&
//        p1StreamInfoMap) -> void = 0;
//  virtual auto sendNotifyToPolicy(NotifyParams* params) -> int;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IStateCallback Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// public:  ////        Operations.
//  virtual void onConfigureFailed() = 0;
//
//  virtual void onConfigured() = 0;
//
//  virtual void onReady() = 0;
//
//  virtual void onSensorStateChanged(
//                      SensorStateChangedResult const& sensorState) = 0;
};

class IDeviceSessionPolicyFactory {
 public:  ////
  /**
   * A structure for creation parameters.
   */
  struct CreationParams {
    std::shared_ptr<DeviceSessionStaticInfo const> mStaticInfo;
    std::shared_ptr<DeviceSessionConfigInfo const> mConfigInfo;
  };

  static auto createDeviceSessionPolicyInstance(CreationParams const& params)
      -> std::shared_ptr<IDeviceSessionPolicy>;
};

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace policy
};  // namespace device
};  // namespace core
};  // namespace mcam
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_IDEVICESESSIONPOLICY_H_
