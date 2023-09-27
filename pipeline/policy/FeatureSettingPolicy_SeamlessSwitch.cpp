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
 * MediaTek Inc. (C) 2022. All rights reserved.
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

#include "FeatureSettingPolicy.h"
//
#include <string>
#include <memory>
#include <vector>

// MTKCAM
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/hw/HwTransform.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/drv/IHalSensor.h>

// MTKCAM3
#include <mtkcam3/feature/utils/FeatureProfileHelper.h>

#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FEATURE_SETTING_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)


namespace NSCam::v3::pipeline::policy::featuresetting {

static constexpr char SEAMLESS_PROPERTY_SEAMLESS_ENABLE[] =
        "vendor.debug.camera.seamlessen";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_ENABLE_CAPTURE[] =
        "vendor.debug.camera.seamlessen.capture";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_ENABLE_CAPTURE_REQUEST[] =
        "vendor.debug.camera.seamlessen.capture.req";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_ENABLE_PREVIEW[] =
        "vendor.debug.camera.seamlessen.preview";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_ENABLE_PREVIEW_REQUEST[] =
        "vendor.debug.camera.seamlessen.preview.req";

static constexpr char SEAMLESS_PROPERTY_SEAMLESS_ZOOM_RATIO[] =
        "vendor.debug.camera.seamless.zoom.ratio";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_SCENE_DEFAULT[] =
        "vendor.debug.camera.seamless.scene.default";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_SCENE_CROP[] =
        "vendor.debug.camera.seamless.scene.crop";
static constexpr char SEAMLESS_PROPERTY_SEAMLESS_SCENE_FULL[] =
        "vendor.debug.camera.seamless.scene.full";

static constexpr int SEAMLESS_SCENARIO_COUNT = 3;

using NSCam::NSPipelinePlugin::MetadataPtr;

static auto
isDefault(const SensorCropWinInfo& cropInfo) -> bool {
    // 2 bin or 766
    // or 4 bin as default mode
    // or 9 cell,2 bin as default mode
    return ((cropInfo.w0_size >> 1) == cropInfo.scale_w
        ||  (cropInfo.h0_size >> 1) == cropInfo.scale_h)
        ||
            ((cropInfo.w0_size >> 2) == cropInfo.scale_w
        ||  (cropInfo.h0_size >> 2) == cropInfo.scale_h)
        ||
            (cropInfo.w0_size == (cropInfo.scale_w*3)
        ||  cropInfo.h0_size == (cropInfo.scale_h*3));
}

static auto
isFull(const SensorCropWinInfo& cropInfo,
       const SensorCropWinInfo& cropInfoCap) -> bool {
    // sensor full size == tg out
    // or full mode > capture mode size
    // or 766, 2 bin as full mode
    return (cropInfo.w2_tg_size == cropInfo.full_w
        &&  cropInfo.h2_tg_size == cropInfo.full_h)
        ||
            (cropInfo.w2_tg_size > cropInfoCap.w2_tg_size
        &&  cropInfo.h2_tg_size > cropInfoCap.h2_tg_size);
}

static auto
isCrop(const SensorCropWinInfo& cropInfo) {
    // crop at first stage
    // or 766, crop after 2 bin
    // or 9 cell
    return ((cropInfo.full_w >> 1) == cropInfo.w0_size
        ||  (cropInfo.full_h >> 1) == cropInfo.h0_size)
        ||
            ((cropInfo.scale_w >> 1) == cropInfo.w2_tg_size
        &&  (cropInfo.scale_h >> 1) == cropInfo.h2_tg_size)
        ||
            (cropInfo.full_w == (cropInfo.w0_size*3)
        ||  cropInfo.full_h == (cropInfo.h0_size*3));
}

static auto
decideCustomScene(SeamlessSwitchFeatureSetting& setting) -> bool {
    int32_t cDefaultScene = ::property_get_int32(SEAMLESS_PROPERTY_SEAMLESS_SCENE_DEFAULT, -1);
    int32_t cCropScene = ::property_get_int32(SEAMLESS_PROPERTY_SEAMLESS_SCENE_CROP, -1);
    int32_t cFullScene = ::property_get_int32(SEAMLESS_PROPERTY_SEAMLESS_SCENE_FULL, -1);
    bool ret = false;

    if (cDefaultScene >= 0) {
        setting.defaultScene = cDefaultScene;
        ret = true;
    }
    if (cCropScene >= 0) {
        setting.cropScene = cCropScene;
        ret = true;
    }
    if (cFullScene >= 0) {
        setting.fullScene = cFullScene;
        ret = true;
    }
    return ret;
}

static auto
decideCustomZoomRatio(SeamlessSwitchFeatureSetting& setting) -> bool {
    bool ret = false;
    int32_t cZoomRatio = ::property_get_int32(SEAMLESS_PROPERTY_SEAMLESS_ZOOM_RATIO, -1);

    if (cZoomRatio > 0) {
        setting.cropZoomRatio = static_cast<float>(cZoomRatio);
        ret = true;
    }
    return ret;
}

static auto
decideSeamlessEnable() -> bool {
    if (::property_get_bool(SEAMLESS_PROPERTY_SEAMLESS_ENABLE, 0)) {
        return true;
    }
    return false;
}

static auto
decideSeamlessCaptureEnable() -> bool {
    if (::property_get_bool(SEAMLESS_PROPERTY_SEAMLESS_ENABLE_CAPTURE, 1)) {
        return true;
    }
    return false;
}

static auto
decideSeamlessPreviewEnable() -> bool {
    if (::property_get_bool(SEAMLESS_PROPERTY_SEAMLESS_ENABLE_PREVIEW, 1)) {
        return true;
    }
    return false;
}

static auto
decideSeamlessCaptureRequest(IMetadata const* pAppMeta) -> bool {
    const IMetadata::IEntry entry =
            pAppMeta->entryFor(MTK_CONTROL_CAPTURE_SEAMLESS_REMOSAIC_EN);
    if (!entry.isEmpty() && (entry.itemAt(0, Type2Type<MINT32>()) != 0)) {
        return true;
    }
    static bool staticEnable =
        ::property_get_bool(SEAMLESS_PROPERTY_SEAMLESS_ENABLE_CAPTURE_REQUEST, 1);
    return staticEnable;
}

static auto
decideSeamlessPreviewRequest() -> bool {
    static bool staticEnable =
        ::property_get_bool(SEAMLESS_PROPERTY_SEAMLESS_ENABLE_PREVIEW_REQUEST, 1);
    return staticEnable;
}

static auto
updateSeamlessSwitchRelatedMetadata(
        MetadataPtr pOutMetaHal, const uint32_t sensorId, const uint32_t targetSensorMode,
        const uint8_t switchPolicy, const bool isSwitchToFullMode, const bool isLogEnable) -> void {
    MSize sensorSize = MSize();
    MSize sensorCropWinSize = MSize();
    uint32_t u4RawFmtType = 0;
    NSCamHW::HwInfoHelper hwInfoHelper(sensorId);

    hwInfoHelper.updateInfos();
    if (!hwInfoHelper.getSensorSize(targetSensorMode, sensorSize)) {
        MY_LOGE("Seamless Switch: hwInfoHelper getSensorSize failed!!");
        return;
    }
    if (!hwInfoHelper.getSensorCropWindowSize(targetSensorMode, sensorCropWinSize)) {
        MY_LOGE("Seamless Switch: hwInfoHelper getSensorCropWindowSize failed!!");
        return;
    }
    if (!hwInfoHelper.getSensorRawFmtType(u4RawFmtType)) {
        MY_LOGE("Seamless Switch: hwInfoHelper getSensorRawFmtType failed!!");
        return;
    }

    {
        // trigger switching
        IMetadata::IEntry entry(MTK_HAL_REQUEST_SEAMLESS_SWITCH);
        entry.push_back(1, Type2Type<MUINT8>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    {
        IMetadata::IEntry entry(MTK_P1NODE_SEAMLESS_TARGET_SENSOR_MODE);
        entry.push_back(static_cast<MINT32>(targetSensorMode), Type2Type<MINT32>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    if (sensorCropWinSize != MSize()) {
        IMetadata::IEntry entry(MTK_P1NODE_SEAMLESS_ACTIVE_SENSOR_SIZE);
        entry.push_back(sensorCropWinSize, Type2Type<MSize>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    if (sensorSize != MSize()) {
        IMetadata::IEntry entry(MTK_P1NODE_SEAMLESS_TARGET_SENSOR_SIZE);
        entry.push_back(sensorSize, Type2Type<MSize>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    {
        uint8_t sensorPattern = eCAM_NORMAL;
        if (u4RawFmtType == SENSOR_RAW_4CELL && isSwitchToFullMode) {
            sensorPattern = eCAM_4CELL;
        }
        IMetadata::IEntry entry(MTK_P1NODE_SEAMLESS_TARGET_SENSOR_PATTERN);
        entry.push_back(static_cast<MUINT8>(sensorPattern), Type2Type<MUINT8>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    {
        IMetadata::IEntry entry(MTK_P1NODE_SEAMLESS_SWITCH_POLICY);
        entry.push_back(static_cast<MUINT8>(switchPolicy), Type2Type<MUINT8>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    if (sensorSize != MSize()) {
        // update sensor size for p2
        IMetadata::IEntry entry(MTK_HAL_REQUEST_SENSOR_SIZE);
        entry.push_back(sensorSize, Type2Type<MSize>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    if (isSwitchToFullMode) {
        // Setting binning enable allows p1 driver to use bin when the
        // throughput is larger than hw limitation.
        IMetadata::IEntry entry(MTK_P1NODE_SEAMLESS_SWITCH_BINNING_ENABLE);
        entry.push_back(1, Type2Type<MBOOL>());
        pOutMetaHal->update(entry.tag(), entry);
    }

    MY_LOGD_IF(isLogEnable,
        "Seamless Switch: [Debug] sensorId = %u, sensorSize = %dx%d, sensorCropWinSize = %dx%d, "
        "switchPolicy = %u, targetSensorMode = %u, isSwitchToFullMode = %d",
        sensorId, sensorSize.w, sensorSize.h, sensorCropWinSize.w, sensorCropWinSize.h,
        switchPolicy, targetSensorMode, isSwitchToFullMode);
}

static auto
updateSeamlessSwitchCrop(IMetadata const* pAppMeta, MetadataPtr pOutMetaHal,
        const uint32_t sensorMode, const uint32_t sensorId,
        const bool isLogEnable) -> void {
    MSize sensorSize = MSize();
    MRect appCropRegion = MRect();
    NSCamHW::HwInfoHelper hwInfoHelper(sensorId);
    hwInfoHelper.updateInfos();
    if (!hwInfoHelper.getSensorSize(sensorMode, sensorSize)) {
        MY_LOGE("Seamless Switch: hwInfoHelper getSensorSize failed!!");
        return;
    }
    {
        const IMetadata::IEntry entry = pAppMeta->entryFor(MTK_SCALER_CROP_REGION);
        if (!entry.isEmpty()) {
            appCropRegion = entry.itemAt(0, Type2Type<MRect>());
        } else {
            MY_LOGE("Seamless Switch: get MTK_SCALER_CROP_REGION failed!!");
            return;
        }
    }

    // convert app crop region to sensor domain
    MRect resultCropRegion = MRect();
    {
        NSCamHW::HwTransHelper hwTransHelper(sensorId);
        NSCamHW::HwMatrix active2SensorMatrix;
        if (!hwTransHelper.getMatrixFromActive(sensorMode, active2SensorMatrix)) {
            MY_LOGE("Seamless Switch: getMatrixFromActive failed!!");
            return;
        }
        active2SensorMatrix.transform(appCropRegion, resultCropRegion);
    }

    {
        IMetadata::IEntry entry(MTK_P1NODE_SENSOR_CROP_REGION);
        entry.push_back(resultCropRegion, Type2Type<MRect>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    {
        IMetadata::IEntry entry(MTK_SENSOR_SCALER_CROP_REGION);
        entry.push_back(resultCropRegion, Type2Type<MRect>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    {
        IMetadata::IEntry entry(MTK_3A_PRV_CROP_REGION);
        entry.push_back(resultCropRegion, Type2Type<MRect>());
        pOutMetaHal->update(entry.tag(), entry);
    }
    {
        IMetadata::IEntry entry(MTK_P2NODE_SENSOR_CROP_REGION);
        entry.push_back(resultCropRegion, Type2Type<MRect>());
        pOutMetaHal->update(entry.tag(), entry);
    }

    MY_LOGD_IF(isLogEnable,
        "Seamless Switch: [Debug] sensorId = %u, sersorMode = %u, "
        "resultCropRegion = (%dx%d)@(%d,%d)",
        sensorId, sensorMode, resultCropRegion.s.w, resultCropRegion.s.h,
        resultCropRegion.p.x, resultCropRegion.p.y);
}

static auto
tryGetIspHidlHint(IMetadata const* pAppMeta, int32_t& ispFrameCnt, int32_t& ispFrameIdx) -> bool {
    IMetadata::IEntry const eIspFmtCnt =
        pAppMeta->entryFor(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_COUNT);
    IMetadata::IEntry const eIspFmtIdx =
        pAppMeta->entryFor(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX);
    if (!eIspFmtCnt.isEmpty() && !eIspFmtIdx.isEmpty()) {
        ispFrameCnt = eIspFmtCnt.itemAt(0, Type2Type<MINT32>());
        ispFrameIdx = eIspFmtIdx.itemAt(0, Type2Type<MINT32>());
        return true;
    }
    return false;
}

static auto
tryGetControlZoomRatio(IMetadata const* pAppMeta, float& controlZoomRatio) -> bool {
    const IMetadata::IEntry entry = pAppMeta->entryFor(MTK_CONTROL_ZOOM_RATIO);
    if (!entry.isEmpty()) {
        controlZoomRatio = entry.itemAt(0, Type2Type<MFLOAT>());
        return true;
    }
    return false;
}

static auto
trySwitchToFullMode(RequestOutputParams* out, const uint32_t sensorId,
        const uint32_t sourceSensorMode, uint32_t& targetSensorMode,
        const uint32_t defaultMode, const uint32_t fullMode, IMetadata const* pAppMeta,
        MetadataPtr pOutMetaHal, const bool isLogEnable) -> bool {
    if (sourceSensorMode != defaultMode) {
        MY_LOGW("Seamless Switch: switch from mode = %u is not supported!", sourceSensorMode);
        return false;
    }

    const auto sensorModeIter = out->sensorMode.find(sensorId);
    if (sensorModeIter == out->sensorMode.end()) {
        MY_LOGE("Seamless Switch: cannot get current sensor mode");
        return false;
    }
    targetSensorMode = sensorModeIter->second;
    if (fullMode != targetSensorMode) {
        MY_LOGE("Seamless Switch: remosaic sensor mode (%u)"
            "does not support seamless switch (fullMode = %u)",
            targetSensorMode, fullMode);
        return false;
    }

    auto isSeamlessCaptureRequest = decideSeamlessCaptureRequest(pAppMeta);
    if (!isSeamlessCaptureRequest) {
        MY_LOGD("Seamless Switch: support seamless switch capture, but disable by user");
        return false;
    }

    updateSeamlessSwitchRelatedMetadata(
        pOutMetaHal, sensorId, targetSensorMode,
        MTK_SEAMLESS_SWITCH_POLICY_ABANDON, true, isLogEnable);
    return true;
}

static auto
trySwitchInSensor(const uint32_t targetSensorMode, const uint32_t sensorId,
        MetadataPtr pOutMetaHal, const bool isLogEnable) -> bool {
    auto isSeamlessPreviewRequest = decideSeamlessPreviewRequest();
    if (!isSeamlessPreviewRequest) {
        MY_LOGD("Seamless Switch: support seamless switch preview, but disable by user");
        return false;
    }
    updateSeamlessSwitchRelatedMetadata(
        pOutMetaHal, sensorId, targetSensorMode,
        MTK_SEAMLESS_SWITCH_POLICY_NON_ABANDON, false, isLogEnable);
    return true;
}

static auto
checkMain1Streaming(RequestInputParams const* in, const uint32_t mainSensor) -> bool {
    if (auto pMulti = in->pMultiCamReqOutputParams) {
        const auto sensors = pMulti->streamingSensorList;
        const auto iter = std::find(sensors.begin(), sensors.end(), mainSensor);
        if (iter == sensors.end()) {
            return false;
        }
    }
    // 1. multi-cam case and main1 is streaming.
    // 2. single-cam case.
    return true;
}

auto
FeatureSettingPolicy::
evaluateSeamlessSwitchConfiguration(
        ConfigurationOutputParams* out,
        [[maybe_unused]] ConfigurationInputParams const* in) -> bool {
    CAM_ULOGM_APILIFE();

    auto& configParam = out->SeamlessSwitchParams;
    if (configParam.isInitted) {
        return true;
    }

    const auto& pStaticInfo = mPolicyParams.pPipelineStaticInfo;
    if (!pStaticInfo) {
        MY_LOGE("Seamless Switch: [Config] cannot get static info");
        return false;
    }
    if (!pStaticInfo->isSeamlessSwitchSupported) {
        MY_LOGD("Seamless Switch: [Config] platform not supported!");
        return true;
    }
    if (pStaticInfo->seamlessSwitchSensorModes.empty()) {
        MY_LOGE("Seamless Switch: [Config] cannot get supported sensor modes!");
        return false;
    }

    // query sensor crop window info
    struct SeamlessSensorCropInfo {
        uint32_t sensorMode = 0;
        SensorCropWinInfo cropInfo = {};
    };
    std::vector<SeamlessSensorCropInfo> seamlessSensorCropInfos;
    seamlessSensorCropInfos.reserve(SEAMLESS_SCENARIO_COUNT);
    SensorCropWinInfo captureCropInfo = {};
    {
        auto pHalDeviceList = MAKE_HalLogicalDeviceList();
        if (pHalDeviceList == nullptr) {
            MY_LOGE("Seamless Switch: [Config] MAKE_HalLogicalDeviceList failed!");
            return false;
        }
        int sensorDevIndex = pHalDeviceList->querySensorDevIdx(mReqMasterId);
        IHalSensorList* sensorList = MAKE_HalSensorList();
        if (sensorList == nullptr) {
            MY_LOGE("Seamless Switch: [Config] cannot get sensor list");
            return false;
        }
        IHalSensor* pIHalSensor = sensorList->createSensor(LOG_TAG, mReqMasterId);
        if (pIHalSensor == nullptr) {
            MY_LOGE("Seamless Switch: [Config] cannot get hal sensor");
            return false;
        }

        for (const auto mode : pStaticInfo->seamlessSwitchSensorModes) {
            SensorCropWinInfo rSensorCropInfo;
            int err = pIHalSensor->sendCommand(sensorDevIndex, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                                (MUINTPTR)&mode, (MUINTPTR)&rSensorCropInfo, 0);
            SeamlessSensorCropInfo sensorCropInfo = {.sensorMode = mode, .cropInfo = rSensorCropInfo};
            seamlessSensorCropInfos.emplace_back(sensorCropInfo);
        }
        {
            uint32_t mode = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
            int err = pIHalSensor->sendCommand(sensorDevIndex, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                                (MUINTPTR)&mode, (MUINTPTR)&captureCropInfo, 0);
        }
        pIHalSensor->destroyInstance(LOG_TAG);
    }

    // query sensor modes
    float cropTgWidth = 0.0f, fullTgWidth = 0.0f;
    {
        int count = SEAMLESS_SCENARIO_COUNT;
        for (const auto& info : seamlessSensorCropInfos) {
            if (count == 0) break;

            const int32_t scene = static_cast<int32_t>(info.sensorMode);
            const auto& cropInfo = info.cropInfo;

            if (configParam.defaultScene < 0 && isDefault(cropInfo)) {
                MY_LOGD("Seamless Switch: [Config] get defaultScene = %d", scene);
                configParam.defaultScene = scene;
                count--;
                continue;
            }
            if (configParam.cropScene < 0 && isCrop(cropInfo)) {
                MY_LOGD("Seamless Switch: [Config] get cropScene = %d", scene);
                configParam.cropScene = scene;
                cropTgWidth = static_cast<float>(cropInfo.w2_tg_size);
                count--;
                continue;
            }
            if (configParam.fullScene < 0 && isFull(cropInfo, captureCropInfo)) {
                MY_LOGD("Seamless Switch: [Config] get fullScene = %d", scene);
                configParam.fullScene = scene;
                fullTgWidth = static_cast<float>(cropInfo.w2_tg_size);
                count--;
                continue;
            }
        }

        if (decideCustomScene(configParam)) {
            MY_LOGD("Seamless Switch: [Config] get custom scenario setting: "
                "default = %d, crop = %d, full = %d",
                configParam.defaultScene, configParam.cropScene, configParam.fullScene);
        }
    }

    // crop zoom ratio
    if (configParam.cropScene >= 0 && configParam.fullScene >= 0) {
        configParam.cropZoomRatio = fullTgWidth / cropTgWidth;
        if (decideCustomZoomRatio(configParam)) {
            MY_LOGD("Seamless Switch: [Config] get custom crop ratio: %f",
                configParam.cropZoomRatio);
        }
    }

    // query user setting
    configParam.isSeamlessEnable = decideSeamlessEnable();
    if (configParam.isSeamlessEnable) {
        configParam.isSeamlessCaptureEnable = decideSeamlessCaptureEnable();
        configParam.isSeamlessPreviewEnable = decideSeamlessPreviewEnable();

        mCurrentSensorMode = static_cast<uint32_t>(configParam.defaultScene);
        MY_LOGD("Seamless Switch: [Config] isSeamlessEnable = 1 "
            "workaround: init mCurrentSensorMode = %u (with defaultScene)",
            mCurrentSensorMode);
    }

    configParam.isInitted = true;
    // copy a set for per-frame usage
    MY_LOGI("Seamless Switch: [Config] SeamlessSwitchFeatureSetting = {%s}",
        toString(configParam).c_str());
    return true;
}

auto
FeatureSettingPolicy::
evaluateSeamlessSwitchSetting(
        RequestOutputParams* out,
        RequestInputParams const* in) -> bool {
    CAM_ULOGM_APILIFE();

    const auto& pStaticInfo = mPolicyParams.pPipelineStaticInfo;
    const uint32_t reqNo = in->requestNo;
    const uint32_t main1SensorId = pStaticInfo->sensorId[0];
    const auto pAppMeta = in->pRequest_AppControl;
    const auto pConfigParam = in->pConfiguration_SeamlessSwitch;
    if (!pStaticInfo) {
        MY_LOGE("Seamless Switch: [R%u] cannot get static info", reqNo);
        return false;
    }
    if (!pAppMeta) {
        MY_LOGE("Seamless Switch: [R%u] cannot get app meta", reqNo);
        return false;
    }
    if (!pConfigParam) {
        MY_LOGD_IF(mbDebug > 0,
            "Seamless Switch: [R%u Debug] cannot get config param", reqNo);
        return true;
    }

    if (!in->bIsHWReady) {
        MY_LOGW("Seamless Switch: [R%u] hw not ready for seamless switch", reqNo);
        return true;
    }

    auto isMain1Streaming = checkMain1Streaming(in, main1SensorId);
    const auto& configParam = *pConfigParam;
    MY_LOGD_IF(mbDebug > 0,
        "Seamless Switch: [R%u Debug] main1SensorId = %u, isMain1Streaming = %d, "
        "SeamlessSwitchFeatureSetting = {%s}",
        reqNo, main1SensorId, isMain1Streaming, toString(configParam).c_str());

    if (!isMain1Streaming) {
        // return directly since seamless feature is only applied on following conditions:
        //      1. seamless remosaic full mode capture with single-cam device.
        //      2. seamless crop mode preview with single-cam device.
        //      3. seamless crop mode preview on main-cam with multi-cam device.
        return true;
    }

    const uint32_t configDefaultMode = static_cast<uint32_t>(configParam.defaultScene);
    const uint32_t configCropMode = static_cast<uint32_t>(configParam.cropScene);
    const uint32_t configFullMode = static_cast<uint32_t>(configParam.fullScene);
    if (!configParam.isSeamlessEnable) {
        return true;
    }

    MetadataPtr pOutMetaHal = std::make_shared<IMetadata>();
    bool needCaptureSwitching = false, needPreviewSwitching = false;
    const uint32_t sourceSensorMode = mCurrentSensorMode;
    uint32_t targetSensorMode = 0;

    // 1. handle capture switching
    if (configParam.isSeamlessCaptureEnable) {
        // 1.1 handle auto switch back
        if (mIsReturnFromSeamless) {
            // try switch back to default mode
            if (sourceSensorMode != configFullMode) {
                MY_LOGE("Seamless Switch: [R%u] switch from mode = %u is not supported!",
                    reqNo, sourceSensorMode);
                return false;
            }
            targetSensorMode = configDefaultMode;
            updateSeamlessSwitchRelatedMetadata(
                pOutMetaHal, main1SensorId, targetSensorMode,
                MTK_SEAMLESS_SWITCH_POLICY_NON_ABANDON, false, (mbDebug > 0));

            mIsReturnFromSeamless = false;
            needCaptureSwitching = true;
        }

        // 1.2 update reconfig related settings to seamless if supported
        if ((out->needReconfiguration && out->reconfigCategory == ReCfgCtg::HIGH_RES_REMOSAIC) ||
            (mRcfFrameCnt != 0)) {
            int32_t ispFrameCnt = 0, ispFrameIdx = 0;
            bool isAppManual = tryGetIspHidlHint(pAppMeta, ispFrameCnt, ispFrameIdx);
            MY_LOGD_IF(isAppManual,
                "Seamless Switch: [R%u IspHidl(%d/%d)] mRcfFrameCnt = %d",
                reqNo, ispFrameIdx, ispFrameCnt, mRcfFrameCnt);

            if (!isAppManual || ispFrameCnt == 0) {
                // single requst
                if (!trySwitchToFullMode(
                        out, main1SensorId, sourceSensorMode, targetSensorMode,
                        configDefaultMode, configFullMode, pAppMeta, pOutMetaHal,
                        (mbDebug > 0))) {
                    MY_LOGD("Seamless Switch: [R%u] trySwitchToFullMode failed. "
                        "workaround: disable remosaic reconfig and change to zsl capture",
                        reqNo);
                    out->needReconfiguration = false;
                    out->reconfigCategory = ReCfgCtg::NO;
                    out->needZslFlow = true;
                    out->sensorMode[main1SensorId] = sourceSensorMode;
                    return true;
                }

                mIsReturnFromSeamless = true;
                needCaptureSwitching = true;
            } else {
                // multi request
                if (mRcfFrameCnt != 0) {
                    if (ispFrameCnt != mRcfFrameCnt) {
                        MY_LOGE("Seamless Switch: [R%u IspHidl(%d/%d)] hint error, "
                            "mRcfFrameCnt = %d", reqNo, ispFrameIdx, ispFrameCnt, mRcfFrameCnt);
                        return false;
                    }
                    if (ispFrameCnt - 1 == ispFrameIdx) {
                        mRcfFrameCnt = 0;
                        mIsReturnFromSeamless = true;
                        MY_LOGD("Seamless Switch: [R%u] the last frame of this "
                            "sequence comes, reset frameCnt",
                            reqNo);
                    }
                } else {
                    MY_LOGD("Seamless Switch: [R%u IspHidl(%d/%d)] a new "
                        "Seamless reconfig sequence comes", reqNo, ispFrameIdx, ispFrameCnt);
                    mRcfFrameCnt = ispFrameCnt;

                    if (!trySwitchToFullMode(
                            out, main1SensorId, sourceSensorMode, targetSensorMode,
                            configDefaultMode, configFullMode, pAppMeta,
                            pOutMetaHal, (mbDebug > 0))) {
                        MY_LOGW("Seamless Switch: [R%u] trySwitchToFullMode failed. "
                            "workaround: disable remosaic reconfig", reqNo);
                        out->needReconfiguration = false;
                        out->reconfigCategory = ReCfgCtg::NO;
                        out->needZslFlow = true;
                        out->sensorMode[main1SensorId] = sourceSensorMode;
                        return true;
                    }

                    mIsReturnFromSeamless = true;
                    needCaptureSwitching = true;
                }
            }
        }

        if (needCaptureSwitching) {
            MY_LOGI("Seamless Switch: [R%u Capture Switching] sensor mode (%u -> %u)",
                reqNo, sourceSensorMode, targetSensorMode);
            mCurrentSensorMode = targetSensorMode;
            out->reconfigCategory = ReCfgCtg::SEAMLESS_REMOSAIC;
            out->isSeamlessCaptureRequest = true;
        }
    }

    // 2. handle preview switching
    if (configParam.isSeamlessPreviewEnable) {
        if (in->bIsMultiCamSeamless) {
            MY_LOGD_IF(mbDebug > 0,
                "Seamless Switch: [R%u Debug] get setting from sensor control: "
                "bIsSeamlessSwitching = %d, seamlessTargetSensorMode = %u, "
                "bIsLosslessZoom = %d, bLosslessZoomSensorMode = %u",
                reqNo, in->bIsSeamlessSwitching, in->seamlessTargetSensorMode,
                in->bIsLosslessZoom, in->losslessZoomSensorMode);
            if (in->bIsSeamlessSwitching) {
                updateSeamlessSwitchRelatedMetadata(
                    pOutMetaHal, main1SensorId, in->seamlessTargetSensorMode,
                    MTK_SEAMLESS_SWITCH_POLICY_NON_ABANDON, false, (mbDebug > 0));
            }
            updateRequestResultParams(out->mainFrame, nullptr, pOutMetaHal, 0, main1SensorId);
            if (in->bIsLosslessZoom) {
                out->sensorMode[main1SensorId] = in->losslessZoomSensorMode;
            }
            return true;
        }

        const float& zoomRatioSetting = configParam.cropZoomRatio;
        float controlZoomRatio = 1.0;
        if (!tryGetControlZoomRatio(pAppMeta, controlZoomRatio)) {
            MY_LOGE("Seamless Switch: [R%u] get MTK_CONTROL_ZOOM_RATIO failed!!", reqNo);
            return false;
        }
        MY_LOGD_IF(mbDebug > 0,
            "Seamless Switch: [R%u Debug] controlZoomRatio = %f, mCurrentSensorMode = %d",
            reqNo, controlZoomRatio, mCurrentSensorMode);

        if (controlZoomRatio >= zoomRatioSetting && mCurrentSensorMode == configDefaultMode) {
            targetSensorMode = configCropMode;
            if (!trySwitchInSensor(
                    targetSensorMode, main1SensorId, pOutMetaHal, (mbDebug > 0))) {
                MY_LOGW("Seamless Switch: [R%u] trySwitchInSensor failed", reqNo);
                return false;
            }
            needPreviewSwitching = true;
        }

        if (controlZoomRatio < zoomRatioSetting && mCurrentSensorMode == configCropMode) {
            targetSensorMode = configDefaultMode;
            if (!trySwitchInSensor(
                    targetSensorMode, main1SensorId, pOutMetaHal, (mbDebug > 0))) {
                MY_LOGW("Seamless Switch: [R%u] trySwitchInSensor failed", reqNo);
                return false;
            }
            needPreviewSwitching = true;
        }

        if (needPreviewSwitching) {
            MY_LOGI("Seamless Switch: [R%u Preview Switching] "
                "sensor mode (%u -> %u), zoomRatio = %f",
                reqNo, sourceSensorMode, targetSensorMode, controlZoomRatio);
            mCurrentSensorMode = targetSensorMode;
            out->isSeamlessPreviewRequest = true;
        }
    }

    // 3. update seamless settings
    bool needPerframeUpdateMeta = false;
    if (mCurrentSensorMode == configFullMode) {
        out->mainFrame->needSeamlessRemosaicCapture = true;
    }

    if (mCurrentSensorMode == configCropMode) {
        // per-frame update crop for in-sensor-zoom
        updateSeamlessSwitchCrop(pAppMeta, pOutMetaHal, configCropMode,
            main1SensorId, (mbDebug > 0));
        out->sensorMode[main1SensorId] = mCurrentSensorMode;
        needPerframeUpdateMeta = true;
    }

    if (needCaptureSwitching || needPreviewSwitching || needPerframeUpdateMeta) {
        updateRequestResultParams(out->mainFrame, nullptr, pOutMetaHal, 0, main1SensorId);
        // for (auto& reqResult : out->subFrames) {
        //     updateRequestResultParams(reqResult, nullptr, pOutMetaHal, 0, main1SensorId);
        // }
        // for (auto& reqResult : out->preSubFrames) {
        //     updateRequestResultParams(reqResult, nullptr, pOutMetaHal, 0, main1SensorId);
        // }
    }

    return true;
}

}  // namespace NSCam::v3::pipeline::policy::featuresetting
