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
 * MediaTek Inc. (C) 2020. All rights reserved.
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
#define LOG_TAG "mtkcam-FeatureSettingPolicy"

#include "FeatureSettingPolicy_Streaming_Custom.h"

#include <memory>
#include <vector>

#include "FeatureSettingPolicy.h"
#include "MyUtils.h"
#include "mtkcam/utils/std/Log.h"
#include "mtkcam/utils/std/Trace.h"
#include "mtkcam/utils/std/ULog.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FEATURE_SETTING_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
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

// #define FSP_CUSTOM_EXAMPLE

#ifdef FSP_CUSTOM_EXAMPLE
#include "isp_tuning/isp_tuning.h"
#endif

#define KEY_FSP_STREAMING_CUSTOM_DUMP "vendor.debug.featuresetting.custom.log"

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {

static void dumpMetadata(const CustomStreamData& o) {
  // note: should enable metadata loglevel first "vendor.debug.camera.metadata"
    for (size_t i = 0; i < o.preDummyFrame.size(); i++) {
    MY_LOGD("Start dump preDummy(%zu)...", i);
      for (const auto& pair : o.preDummyFrame[i]) {
      MY_LOGD("preDummy[%d]...", pair.first);
      if (pair.second.appMetadata != nullptr) {
        MY_LOGD("dump appMetadata...");
        pair.second.appMetadata->dump();
      }
      if (pair.second.halMetadata != nullptr) {
        MY_LOGD("dump halMetadata...");
        pair.second.halMetadata->dump();
      }
    }
  }
  for (const auto& pair : o.mainFrame) {
    MY_LOGD("start dump mainFrame[%d]...", pair.first);
    if (pair.second.appMetadata != nullptr) {
      MY_LOGD("dump appMetadata...");
      pair.second.appMetadata->dump();
    }
    if (pair.second.halMetadata != nullptr) {
      MY_LOGD("dump halMetadata...");
      pair.second.halMetadata->dump();
    }
  }
}

auto FeatureSettingPolicy::extractStreamConfiguration_Custom(
    ConfigurationInputParams const* in __unused) -> CustomStreamConfiguration {
  CAM_ULOGM_APILIFE();

  CustomStreamConfiguration outStreamConfig;
  /* set customized configuration here */
#ifdef FSP_CUSTOM_EXAMPLE
  outStreamConfig.proprietaryClient = 0;
  outStreamConfig.reqFps = 24;
  outStreamConfig.dsdnHint = 0;
  outStreamConfig.nr3dMode = 0;
  outStreamConfig.eisFactor = 120;
#endif

  static const bool enableLog = ::property_get_bool(KEY_FSP_STREAMING_CUSTOM_DUMP, 0);
  MY_LOGD_IF(enableLog, "CustomStreamConfiguration=%s",
             toString(outStreamConfig).c_str());
  return outStreamConfig;
}

auto FeatureSettingPolicy::extractStreamData_Custom(
    RequestInputParams const* in __unused) -> CustomStreamData {
  CAM_ULOGM_APILIFE();

  CustomStreamData outStreamData;
#ifdef FSP_CUSTOM_EXAMPLE
  /* customization example
   */
  IMetadata const* pAppMetaControl = in->pRequest_AppControl;
  MINT32 hfpsMode = MTK_STREAMING_FEATURE_HFPS_MODE_NORMAL;
  IMetadata::getEntry<MINT32>(pAppMetaControl,
      MTK_STREAMING_FEATURE_HFPS_MODE, hfpsMode);
  MUINT8 appEisMode = 0;
  IMetadata::getEntry<MUINT8>(pAppMetaControl,
      MTK_CONTROL_VIDEO_STABILIZATION_MODE, appEisMode);
  MINT32 appHDRMode = 0;
  IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_HDR_FEATURE_HDR_MODE, appHDRMode);

  if ((hfpsMode == MTK_STREAMING_FEATURE_HFPS_MODE_60FPS) &&
      (appEisMode == MTK_CONTROL_VIDEO_STABILIZATION_MODE_ON)) {
    outStreamData.cpuProfile = Cam3CPUCtrl::E_CAM3_CPU_CTRL_FPSGO_HIGH;
  }

  if (in->requestNo == 0 && appHDRMode == MTK_HDR_FEATURE_HDR_MODE_OFF ) {
    std::shared_ptr<NSCam::IMetadata> outMetaApp =
        std::make_shared<IMetadata>();
    std::shared_ptr<NSCam::IMetadata> outMetaHal =
        std::make_shared<IMetadata>();
    IMetadata::setEntry<MUINT8>(outMetaApp.get(), MTK_CONTROL_AE_LOCK, 1);
    IMetadata::setEntry<MUINT8>(outMetaHal.get(), MTK_3A_ISP_PROFILE,
                                NSIspTuning::EIspProfile_Video);
    FrameDescriptor FrameDesc {
        .appMetadata = outMetaApp,
        .halMetadata = outMetaHal,
    };

    SensorMap<FrameDescriptor> customDesc1;
    customDesc1.emplace(mReqMasterId, FrameDesc);
    outStreamData.preDummyFrame.push_back(customDesc1);

    SensorMap<FrameDescriptor> customDesc2;
    customDesc2.emplace(mReqMasterId, FrameDescriptor{}); // default descriptor
    const size_t numDefault = 3;
    for (size_t i = 0; i < numDefault; i++) {
        outStreamData.preDummyFrame.push_back(customDesc2);
    }
  }
  else {
    std::shared_ptr<NSCam::IMetadata> outMetaHal =
        std::make_shared<IMetadata>();
    if (appHDRMode == MTK_HDR_FEATURE_HDR_MODE_OFF) {
        IMetadata::setEntry<MUINT8>(outMetaHal.get(), MTK_3A_ISP_PROFILE,
                                    NSIspTuning::EIspProfile_Video);
    }
    FrameDescriptor customDesc {
        .appMetadata = nullptr,
        .halMetadata = outMetaHal,
    };

    // set metadata in mainframe to all streaming sensors
    outStreamData.mainFrame.emplace(mReqMasterId, customDesc);
    for (const auto& sensorId : mReqSubSensorIdList) {
      outStreamData.mainFrame.emplace(sensorId, customDesc);
    }
  }
#endif

  static const int32_t logLevel =
      ::property_get_int32(KEY_FSP_STREAMING_CUSTOM_DUMP, 0);
  MY_LOGD_IF((logLevel > 0), "request(%d)CustomStreamSetting=%s",
             in->requestNo, toString(outStreamData).c_str());
  if (logLevel >= 3) {
    dumpMetadata(outStreamData);
  }
  return outStreamData;
}

};  // namespace featuresetting
};  // namespace policy
};  // namespace pipeline
};  // namespace v3
};  // namespace NSCam
