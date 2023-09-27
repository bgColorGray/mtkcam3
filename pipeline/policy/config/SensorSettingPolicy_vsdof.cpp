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

#define LOG_TAG "mtkcam-SensorSettingPolicyVsdof"

#include <algorithm>

#include <mtkcam3/pipeline/policy/IConfigSensorSettingPolicy.h>
//
#include "MyUtils.h"

#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/hw/HwTransform.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>

#endif

#include <mtkcam/utils/std/ULog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCamHW;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)

#define CHECK_ERROR(_err_)                                \
    do {                                                  \
        MERROR const err = (_err_);                       \
        if( CC_UNLIKELY( err != OK ) ) {                                 \
            MY_LOGE("err:%d(%s)", err, ::strerror(-err)); \
            return err;                                   \
        }                                                 \
    } while(0)

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {

enum eMode_vsdof
{
    eNORMAL_PREVIEW = 0,
    eNORMAL_VIDEO,
    eNORMAL_CAPTURE,
    eSLIM_VIDEO1,
    eSLIM_VIDEO2,
    eCUSTOM1,
    eCUSTOM2,
    eCUSTOM3,
    eCUSTOM4,
    eCUSTOM5,
    eCUSTOM6,
    eCUSTOM7,
    eCUSTOM8,
    eCUSTOM9,
    eCUSTOM10,
    eCUSTOM11,
    eCUSTOM12,
    eCUSTOM13,
    eCUSTOM14,
    eCUSTOM15,
    eNUM_SENSOR_MODE,
};

const char* kModeNames_vsdof[eMode_vsdof::eNUM_SENSOR_MODE+1] =
{
    "PREVIEW",
    "VIDEO",
    "CAPTURE",
    "SLIM_VIDEO1",
    "SLIM_VIDEO2",
    "CUSTOM1",
    "CUSTOM2",
    "CUSTOM3",
    "CUSTOM4",
    "CUSTOM5",
    "CUSTOM6",
    "CUSTOM7",
    "CUSTOM8",
    "CUSTOM9",
    "CUSTOM10",
    "CUSTOM11",
    "CUSTOM12",
    "CUSTOM13",
    "CUSTOM14",
    "CUSTOM15",
    "UNDEFINED"
};

struct SensorParams
{
    std::unordered_map<eMode_vsdof, SensorSetting> mSetting;
    std::unordered_map<eMode_vsdof, eMode_vsdof>   mAltMode;
};

static auto
parseSensorParamsSetting(
    std::shared_ptr<SensorParams>& pParams,
    const HwInfoHelper& rHelper
) -> int
{
#define addStaticParams(idx, _scenarioId_, _item_)                                          \
    do {                                                                                    \
        int32_t fps;                                                                        \
        MSize   size;                                                                       \
        if ( CC_UNLIKELY( ! rHelper.getSensorFps( _scenarioId_, fps) ) ) {                  \
            MY_LOGW("getSensorFps fail"); break;                                            \
        }                                                                                   \
        if ( CC_UNLIKELY( ! rHelper.getSensorSize(_scenarioId_, size) ) ) {                 \
            MY_LOGW("getSensorSize fail"); break;                                           \
        }                                                                                   \
        _item_->mSetting[idx].sensorFps  = static_cast<uint32_t>(fps);                      \
        _item_->mSetting[idx].sensorSize = size;                                            \
        _item_->mSetting[idx].sensorMode = _scenarioId_;                                    \
        _item_->mAltMode[idx] = idx;                                                        \
        MY_LOGD("candidate mode %d, size(%d, %d)@%d", idx, _item_->mSetting[idx].sensorSize.w, \
                _item_->mSetting[idx].sensorSize.h, _item_->mSetting[idx].sensorFps);       \
    } while(0)

    addStaticParams(eNORMAL_PREVIEW, SENSOR_SCENARIO_ID_NORMAL_PREVIEW, pParams);
    addStaticParams(eNORMAL_CAPTURE, SENSOR_SCENARIO_ID_NORMAL_CAPTURE, pParams);
    addStaticParams(eNORMAL_VIDEO  , SENSOR_SCENARIO_ID_NORMAL_VIDEO  , pParams);
    addStaticParams(eSLIM_VIDEO1,    SENSOR_SCENARIO_ID_SLIM_VIDEO1,    pParams);
    addStaticParams(eSLIM_VIDEO2,    SENSOR_SCENARIO_ID_SLIM_VIDEO2,    pParams);
    addStaticParams(eCUSTOM1,        SENSOR_SCENARIO_ID_CUSTOM1,        pParams);
    addStaticParams(eCUSTOM2,        SENSOR_SCENARIO_ID_CUSTOM2,        pParams);
    addStaticParams(eCUSTOM3,        SENSOR_SCENARIO_ID_CUSTOM3,        pParams);
    addStaticParams(eCUSTOM4,        SENSOR_SCENARIO_ID_CUSTOM4,        pParams);
    addStaticParams(eCUSTOM5,        SENSOR_SCENARIO_ID_CUSTOM5,        pParams);
    addStaticParams(eCUSTOM6,        SENSOR_SCENARIO_ID_CUSTOM6,        pParams);
    addStaticParams(eCUSTOM7,        SENSOR_SCENARIO_ID_CUSTOM7,        pParams);
    addStaticParams(eCUSTOM8,        SENSOR_SCENARIO_ID_CUSTOM8,        pParams);
    addStaticParams(eCUSTOM9,        SENSOR_SCENARIO_ID_CUSTOM9,        pParams);
    addStaticParams(eCUSTOM10,       SENSOR_SCENARIO_ID_CUSTOM10,        pParams);
    addStaticParams(eCUSTOM11,       SENSOR_SCENARIO_ID_CUSTOM11,        pParams);
    addStaticParams(eCUSTOM12,       SENSOR_SCENARIO_ID_CUSTOM12,        pParams);
    addStaticParams(eCUSTOM13,       SENSOR_SCENARIO_ID_CUSTOM13,        pParams);
    addStaticParams(eCUSTOM14,       SENSOR_SCENARIO_ID_CUSTOM14,        pParams);
    addStaticParams(eCUSTOM15,       SENSOR_SCENARIO_ID_CUSTOM15,        pParams);

#undef addStaticParams
    //
    return OK;
}

static auto
determineScen(
    SensorSetting& res,
    std::shared_ptr<SensorParams>& pParams,
    std::shared_ptr<ParsedAppImageStreamInfo> const& pParsedAppImageInfo __attribute__((unused)),
    MUINT32 sensorIndex // first element will be main1
) -> int
{
    std::string strInfo = "cam" + std::to_string(sensorIndex);
    bool hasError = false;
    MUINT sensorScenario = ((MUINT)-1);
    switch(sensorIndex)
    {
        case 0:
            sensorScenario = StereoSettingProvider::getCurrentSensorScenario(StereoHAL::eSTEREO_SENSOR_MAIN1);
            break;
        case 1:
            sensorScenario = StereoSettingProvider::getCurrentSensorScenario(StereoHAL::eSTEREO_SENSOR_MAIN2);
            break;
        case 2:
            sensorScenario = StereoSettingProvider::getCurrentSensorScenario(StereoHAL::eSTEREO_SENSOR_MAIN3);
            break;
        default:
            hasError = true;
            MY_LOGA("cannot get sensor scenario, id(%" PRId32 ")", sensorIndex);
    }
    switch(sensorScenario)
    {
        case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
            res = pParams->mSetting[eNORMAL_PREVIEW];
            strInfo += " sensorScenario(Preview)";
            break;
        case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
            res = pParams->mSetting[eNORMAL_VIDEO];
            strInfo += " sensorScenario(Video)";
            break;
        case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
            res = pParams->mSetting[eNORMAL_CAPTURE];
            strInfo += " sensorScenario(Capture)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM1:
            res = pParams->mSetting[eCUSTOM1];
            strInfo += " sensorScenario(Custom1)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM2:
            res = pParams->mSetting[eCUSTOM2];
            strInfo += " sensorScenario(Custom2)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM3:
            res = pParams->mSetting[eCUSTOM3];
            strInfo += " sensorScenario(Custom3)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM4:
            res = pParams->mSetting[eCUSTOM4];
            strInfo += " sensorScenario(Custom4)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM5:
            res = pParams->mSetting[eCUSTOM5];
            strInfo += " sensorScenario(Custom5)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM6:
            res = pParams->mSetting[eCUSTOM6];
            strInfo += " sensorScenario(Custom6)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM7:
            res = pParams->mSetting[eCUSTOM7];
            strInfo += " sensorScenario(Custom7)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM8:
            res = pParams->mSetting[eCUSTOM8];
            strInfo += " sensorScenario(Custom8)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM9:
            res = pParams->mSetting[eCUSTOM9];
            strInfo += " sensorScenario(Custom9)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM10:
            res = pParams->mSetting[eCUSTOM10];
            strInfo += " sensorScenario(Custom10)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM11:
            res = pParams->mSetting[eCUSTOM11];
            strInfo += " sensorScenario(Custom11)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM12:
            res = pParams->mSetting[eCUSTOM12];
            strInfo += " sensorScenario(Custom12)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM13:
            res = pParams->mSetting[eCUSTOM13];
            strInfo += " sensorScenario(Custom13)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM14:
            res = pParams->mSetting[eCUSTOM14];
            strInfo += " sensorScenario(Custom14)";
            break;
        case SENSOR_SCENARIO_ID_CUSTOM15:
            res = pParams->mSetting[eCUSTOM15];
            strInfo += " sensorScenario(Custom15)";
            break;
        default:
            strInfo += std::string(" sensorScenario(Not support)");
            hasError = true;
    }

    MY_LOGI("%s", strInfo.c_str());
    if(hasError)
    {
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
        aee_system_exception(
            LOG_TAG,
            NULL,
            DB_OPT_DEFAULT,
            strInfo.c_str());
#endif
    }
    return OK;
}


/**
 * Make a function target - default version
 */
FunctionType_Configuration_SensorSettingPolicy makePolicy_Configuration_SensorSetting_Vsdof()
{
    return [](Configuration_SensorSetting_Params const& params) -> int {
        auto pvOut = params.pvOut;
        auto pStreamingFeatureSetting = params.pStreamingFeatureSetting;
        auto pSeamlessSwitchFeatureSetting = params.pSeamlessSwitchFeatureSetting;
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;
        auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;

        if ( CC_UNLIKELY( ! pvOut || ! pStreamingFeatureSetting ||
                          ! pPipelineStaticInfo || ! pPipelineUserConfiguration ||
                          ! pSeamlessSwitchFeatureSetting) ) {
            MY_LOGE("error input params");
            return -EINVAL;
        }

        auto const& pParsedAppImageInfo = pPipelineUserConfiguration->pParsedAppImageStreamInfo;

        std::map<uint32_t, std::shared_ptr<SensorParams> > mStatic;
        ssize_t index = 0;
        for ( auto const& id : pPipelineStaticInfo->sensorId ) {
            SensorSetting res;  // output
            HwInfoHelper  infoHelper = HwInfoHelper(id);
            infoHelper.updateInfos();
            HwTransHelper tranHelper = HwTransHelper(id);
            auto pStatic = std::make_shared<SensorParams>();
            if ( CC_UNLIKELY( pStatic==nullptr ) ) {
                MY_LOGE("initial sensor static fail");
                return -EINVAL;
            }

            CHECK_ERROR( parseSensorParamsSetting(pStatic, infoHelper) );

            // sensor mode decision policy:
            CHECK_ERROR( determineScen(res, pStatic, pParsedAppImageInfo, index) );

            //ADD VHDR query Logic with check Error function
            // USE orginal logic for now workaround for index
            if (pStreamingFeatureSetting->p1ConfigParam.at(id).hdrHalMode  & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER ) {
                IHalSensorList *halSensorList = MAKE_HalSensorList();
                IHalSensor *pHalSensor = nullptr;
                if(halSensorList != nullptr) {
                    pHalSensor = halSensorList->createSensor(LOG_TAG, 1, (MUINT32*)&id);
                    if(pHalSensor == nullptr) {
                      MY_LOGE("create Sensor(%d) Fail", id);
                      return -EINVAL;
                    }
                }
                else {
                  MY_LOGE("cannot get HalSensorList.");
                  return -EINVAL;
                }
                MUINT32 stagger_scenario = 0xff;//MUST SET TO 0xff
                MUINT32 stagger_type = HDR_NONE;
                if (pStreamingFeatureSetting->p1ConfigParam.at(id).hdrHalMode  == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP) {
                    stagger_type = HDR_RAW_STAGGER_2EXP;
                    MY_LOGD("Set HDR_RAW_STAGGER_2EXP");
                }
                else if (pStreamingFeatureSetting->p1ConfigParam.at(id).hdrHalMode  == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP) {
                    stagger_type = HDR_RAW_STAGGER_3EXP;
                    MY_LOGD("Set HDR_RAW_STAGGER_3EXP");
                }
                int sensorIndexDual = (index == 0)? DUAL_CAMERA_MAIN_SENSOR: DUAL_CAMERA_MAIN_2_SENSOR;
                int scenario = res.sensorMode;
                pHalSensor->sendCommand(sensorIndexDual, SENSOR_CMD_GET_STAGGER_TARGET_SCENARIO, (MUINTPTR)&scenario,(MUINTPTR)&stagger_type, (MUINTPTR)&stagger_scenario);

                if (stagger_scenario == 0xff) {
                    // workaround, force use available stagger sensor mode
                    for (int i = 0; i < eNUM_SENSOR_MODE; i++) {
                        int scenario = i;
                        pHalSensor->sendCommand(sensorIndexDual, SENSOR_CMD_GET_STAGGER_TARGET_SCENARIO, (MUINTPTR)&scenario,(MUINTPTR)&stagger_type, (MUINTPTR)&stagger_scenario);
                        if (stagger_scenario <= SENSOR_SCENARIO_ID_CUSTOM5) {
                            MY_LOGD("Replace original mode");
                            auto const& mode = static_cast<eMode_vsdof>(stagger_scenario);
                            res = pStatic->mSetting[mode];
                            break;
                        }
                    }

                    MY_LOGE("Stagger not supported for current sensor mode! Use %d for workaround", stagger_scenario);
                }
                else if (stagger_scenario <= SENSOR_SCENARIO_ID_CUSTOM5) {
                    auto const& mode = static_cast<eMode_vsdof>(stagger_scenario);
                    res = pStatic->mSetting[mode];
                }
                //select real stagger sensor mode
            }

            // Seamless Switch
            auto const& pParsedMultiCamInfo = pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
            auto isSAT = [pParsedMultiCamInfo]() -> bool {
                if (pParsedMultiCamInfo &&
                    pParsedMultiCamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM) {
                    return true;
                }
                return false;
            }();
            if (index == 0 && isSAT && pSeamlessSwitchFeatureSetting->isSeamlessEnable &&
                pSeamlessSwitchFeatureSetting->defaultScene >= 0) {
                // support in-sensor-zoom for main1 sensor
                const uint32_t defaultMode =
                        static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->defaultScene);
                const auto origSetting = res;
                const auto defaultSetting =
                        [&setting = pStatic->mSetting](const uint32_t mode) -> SensorSetting {
                            const auto iter = std::find_if(setting.begin(), setting.end(),
                                [mode](const auto settingPair) {
                                    const auto& sensorMode = settingPair.second.sensorMode;
                                    return (sensorMode == mode);
                                });
                            if (iter == setting.end()) {
                                MY_LOGE("cannot get default sensor mode, should not happen");
                                return SensorSetting{};
                            }
                            return iter->second;
                        }(defaultMode);
                MY_LOGI("Seamless Switch: [Config] seamless default scene[%u]={size = %dx%d, fps = %u}, "
                    "preview original scene={size = %ux%u, fps = %u}",
                    defaultMode, defaultSetting.sensorSize.w, defaultSetting.sensorSize.h,
                    defaultSetting.sensorFps, origSetting.sensorSize.w,
                    origSetting.sensorSize.h, origSetting.sensorFps);

                if (defaultSetting.sensorFps < origSetting.sensorFps ||
                        defaultSetting.sensorSize.w < origSetting.sensorSize.w ||
                        defaultSetting.sensorSize.h < origSetting.sensorSize.h) {
                    MY_LOGW("Seamless Switch: [Config] default scene does not meet preview requirement");
                }
                res = defaultSetting;
            }
//lbDone:
            MY_LOGD("select mode %d, size(%dx%d) @ %d hdrHalMode(%d) hdrSensor mode(%d)",
                    res.sensorMode, res.sensorSize.w, res.sensorSize.h,
                    res.sensorFps, pStreamingFeatureSetting->p1ConfigParam.at(id).hdrHalMode, pStreamingFeatureSetting->p1ConfigParam.at(id).hdrSensorMode);
            pvOut->emplace(id, res);
            index++;
        }

        return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

