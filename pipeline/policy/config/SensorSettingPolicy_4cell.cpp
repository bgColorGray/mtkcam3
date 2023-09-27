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

#define LOG_TAG "mtkcam-SensorSettingPolicy4cell"

#include <string>

#include <mtkcam3/pipeline/policy/IConfigSensorSettingPolicy.h>
//
#include "MyUtils.h"

#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/hw/HwTransform.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
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

#define parseSensorSetting(res, _scenarioId_, infoHelper)                        \
    do {                                                                       \
        int32_t fps;                                                           \
        MSize   size;                                                          \
        if ( CC_UNLIKELY( ! infoHelper.getSensorFps( _scenarioId_, fps) ) ) {    \
            MY_LOGW("getSensorFps fail"); break;                               \
        }                                                                      \
        if ( CC_UNLIKELY( ! infoHelper.getSensorSize(_scenarioId_, size) ) ) {   \
            MY_LOGW("getSensorSize fail"); break;                              \
        }                                                                      \
        res.sensorFps  = static_cast<uint32_t>(fps);                           \
        res.sensorSize = size;                                                 \
        res.sensorMode = _scenarioId_;                                           \
    } while(0)
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {

enum eMode_4Cell
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

const char* kModeNames_4Cell[eMode_4Cell::eNUM_SENSOR_MODE+1] =
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

static auto isSensorSupport(
    SensorSetting& res,
    const uint32_t hdrHalMode
) -> bool
{
    if ( hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW ){
        const uint32_t MSTREAM_SUPPORT_MIN_FPS = 60;
        if ( res.sensorFps >= MSTREAM_SUPPORT_MIN_FPS ) {
            return true;
        }
    }
    else
    {
        return true;
    }
    return false;
}

static auto
checkVhdrSensor(
    SensorSetting& res,
    const uint32_t hdrSensorMode,
    const uint32_t hdrHalMode,
    const HwInfoHelper& rHelper
) -> int
{
    uint32_t supportHDRMode = 0;
    char forceSensorMode[PROPERTY_VALUE_MAX] = {0};
    property_get("vendor.debug.force.vhdr.sensormode", forceSensorMode, "0");
    switch( forceSensorMode[0] )
    {
        case '0':
            break;
        case 'P':
        case 'p':
            parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_PREVIEW,rHelper);
            MY_LOGD("set sensor mode to NORMAL_PREVIEW(%d)",SENSOR_SCENARIO_ID_NORMAL_PREVIEW);
            return OK;
        case 'V':
        case 'v':
            parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_VIDEO,rHelper);
            MY_LOGD("set sensor mode to NORMAL_VIDEO(%d)",SENSOR_SCENARIO_ID_NORMAL_VIDEO);
            return OK;
        case 'C':
        case 'c':
            parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_CAPTURE,rHelper);
            MY_LOGD("set sensor mode to NORMAL_CAPTURE(%d)",SENSOR_SCENARIO_ID_NORMAL_CAPTURE);
            return OK;
        default:
            MY_LOGW("unknown force sensor mode(%s), not used", forceSensorMode);
            MY_LOGW("usage : setprop debug.force.vhdr.sensormode P/V/C");
            break;
        }

    // 1. Current sensor mode is VHDR support, use it.
    if ( CC_UNLIKELY( ! rHelper.querySupportVHDRMode( res.sensorMode,
                                                    supportHDRMode) ) )
    {
        MY_LOGE("[vhdrhal] HwInfoHelper querySupportVHDRMode fail");
        return -EINVAL;
    }
    if (hdrSensorMode == supportHDRMode)
    {
        if ( isSensorSupport(res, hdrHalMode) ) {
            return OK;
        }
    }

    // 2. Check sensor mode in order: preview -> video -> capture
    // Find acceptable sensor mode for this hdrSensorMode
    /*
           SENSOR_SCENARIO_ID_NORMAL_PREVIEW
           SENSOR_SCENARIO_ID_NORMAL_CAPTURE
           SENSOR_SCENARIO_ID_NORMAL_VIDEO
           SENSOR_SCENARIO_ID_SLIM_VIDEO1
           SENSOR_SCENARIO_ID_SLIM_VIDEO2
      */
#define CHECK_SENSOR_MODE_VHDR_SUPPORT(senMode, scenariomode)                \
do {                                                                         \
       if ( ! rHelper.querySupportVHDRMode(senMode, supportHDRMode) )        \
       {                                                                     \
           return -EINVAL;                                                   \
       }                                                                     \
       if ( hdrSensorMode == supportHDRMode )                                \
       {                                                                     \
           parseSensorSetting(res ,senMode ,rHelper);                        \
           if ( isSensorSupport(res, hdrHalMode) ) {                         \
               return OK;                                                    \
           }                                                                 \
       }                                                                     \
} while (0)
       CHECK_SENSOR_MODE_VHDR_SUPPORT(SENSOR_SCENARIO_ID_NORMAL_PREVIEW,eNORMAL_PREVIEW);
       CHECK_SENSOR_MODE_VHDR_SUPPORT(SENSOR_SCENARIO_ID_NORMAL_VIDEO,eNORMAL_VIDEO);
       CHECK_SENSOR_MODE_VHDR_SUPPORT(SENSOR_SCENARIO_ID_NORMAL_CAPTURE, eNORMAL_CAPTURE);
#undef CHECK_SENSOR_MODE_VHDR_SUPPORT

    //3.  PREVIEW & VIDEO & CAPTURE mode are all not acceptable
    MY_LOGE("[vhdrhal] VHDR not support preview & video & capture mode.");
    return -EINVAL;
}
/**
 * Make a function target - 4cell version
 */
FunctionType_Configuration_SensorSettingPolicy makePolicy_Configuration_SensorSetting_4Cell()
{
    return [](Configuration_SensorSetting_Params const& params) -> int {
        auto pvOut = params.pvOut;
        auto pStreamingFeatureSetting = params.pStreamingFeatureSetting;
        auto pSeamlessSwitchFeatureSetting = params.pSeamlessSwitchFeatureSetting;
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;
        auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;

        if ( CC_UNLIKELY( ! pvOut || ! pStreamingFeatureSetting || ! pSeamlessSwitchFeatureSetting ||
                          ! pPipelineStaticInfo || ! pPipelineUserConfiguration ) ) {
            MY_LOGE("error input params");
            return -EINVAL;
        }

#if MTKCAM_SENSOR_PREVIEW_30FPS
        MUINT32 defaultPreviewFps = 30;
#else
        MUINT32 defaultPreviewFps = 60;
#endif

        auto const& pParsedAppCfg       = pPipelineUserConfiguration->pParsedAppConfiguration;
        auto const& pParsedAppImageInfo = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
        MINT32 streamHfpsMode = MTK_STREAMING_FEATURE_HFPS_MODE_NORMAL;
        IMetadata::getEntry<MINT32>(&pParsedAppCfg->sessionParams, MTK_STREAMING_FEATURE_HFPS_MODE, streamHfpsMode);

        for ( const auto& sensorId : pPipelineStaticInfo->sensorId ) {
            SensorSetting res;  // output
            HwInfoHelper  infoHelper = HwInfoHelper(sensorId);
            infoHelper.updateInfos();

            // sensor mode decision policy:
            // 1. has video consumer
            if ( pParsedAppImageInfo->hasVideoConsumer ) {
                if ( (streamHfpsMode == MTK_STREAMING_FEATURE_HFPS_MODE_60FPS) ||
                      (pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW) )
                {
                    SensorSetting tmpRes;
                    MUINT32 minW = 0, minH = 0;
                    int minDis = INT_MAX;
                    for (int i = 0; i < eNUM_SENSOR_MODE; i++)
                    {
                        parseSensorSetting(tmpRes, i, infoHelper);
                        int dis = abs(tmpRes.sensorSize.w * pParsedAppImageInfo->videoImageSize.h - tmpRes.sensorSize.h * pParsedAppImageInfo->videoImageSize.w);
                        MBOOL matched = (tmpRes.sensorFps == 60) &&
                                        ((minW == 0 && minH == 0) || (tmpRes.sensorSize.w < minW || tmpRes.sensorSize.h < minH));
                        if ((tmpRes.sensorFps == 60 && dis < minDis) || (matched && dis <= minDis))
                        {
                            res = tmpRes;
                            minW = tmpRes.sensorSize.w;
                            minH = tmpRes.sensorSize.h;
                            minDis = dis;
                        }
                    }

                    if (res.sensorFps != 0)
                    {
                        goto lbDone;
                    }
                    else
                    {
                        MY_LOGW("Cannot find HFPS sensor mode!");
                    }
                } else if ( pParsedAppImageInfo->hasVideo4K ) {
                    // 2-2. 4K record
                    SensorSetting tmpRes;
                    parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_VIDEO,infoHelper);
                    if (pStreamingFeatureSetting->fpsFor4K > 0) {
                        int minDis = INT_MAX;
                        for ( int i=0; i<eNUM_SENSOR_MODE; ++i ) {
                            parseSensorSetting(tmpRes, i, infoHelper);
                            if (pStreamingFeatureSetting->fpsFor4K == tmpRes.sensorFps &&
                                tmpRes.sensorSize.w >= pParsedAppImageInfo->videoImageSize.w &&
                                tmpRes.sensorSize.h >= pParsedAppImageInfo->videoImageSize.h)
                            {
                                int dis = abs(tmpRes.sensorSize.w * pStreamingFeatureSetting->aspectRatioFor4K.h - tmpRes.sensorSize.h * pStreamingFeatureSetting->aspectRatioFor4K.w);
                                if (dis < minDis) {
                                    minDis = dis;
                                    res = tmpRes;
                                }
                            }
                        }
                    }
                    goto lbDone;
                }
                else {
                    parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_VIDEO,infoHelper);
                    float imageWSensorH = pParsedAppImageInfo->videoImageSize.w * res.sensorSize.h * 1.05f /* 5% margin */;
                    float imageHSensorW = pParsedAppImageInfo->videoImageSize.h * res.sensorSize.w;
                    if (imageWSensorH < imageHSensorW) {
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_PREVIEW,infoHelper);
                    }
                }
                goto lbDone;
            }
            // 2. default preview mode
            {
                parseSensorSetting(res, SENSOR_SCENARIO_ID_NORMAL_PREVIEW, infoHelper);
                MUINT32 BinW = res.sensorSize.w, BinH = res.sensorSize.h;
                for (int i = 0; i < eNUM_SENSOR_MODE; i++)
                {
                    SensorSetting parsedRes;
                    parseSensorSetting(parsedRes, i, infoHelper);
                    MBOOL matched = (parsedRes.sensorFps == defaultPreviewFps) &&
                                    (parsedRes.sensorSize.w == BinW && parsedRes.sensorSize.h == BinH);
                    if (matched)
                    {
                        res = parsedRes;
                        break;
                    }
                }

                if (pSeamlessSwitchFeatureSetting->isSeamlessEnable &&
                    pSeamlessSwitchFeatureSetting->defaultScene >= 0) {
                    const uint32_t defaultMode =
                            static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->defaultScene);

                    // set & check seamless default scene has meet preview requirement
                    parseSensorSetting(res, defaultMode, infoHelper);
                    MY_LOGI("Seamless Switch: [Config] default scene[%u]={size = %dx%d, fps = %u}, "
                        "preview original scene={size = %ux%u, fps = %u}",
                        defaultMode, res.sensorSize.w, res.sensorSize.h, res.sensorFps,
                        BinW, BinH, defaultPreviewFps);

                    if (res.sensorFps < defaultPreviewFps ||
                        res.sensorSize.w < BinW || res.sensorSize.h < BinH) {
                        MY_LOGW("Seamless Switch: [Config] default scene does not meet preview requirement");
                    }
                }

                MY_LOGI("default preview mode(%d): width(%d), height(%d), fps(%d)", res.sensorMode, res.sensorSize.w, res.sensorSize.h, res.sensorFps);
            }
lbDone:
            if (pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode != MTK_HDR_FEATURE_HDR_HAL_MODE_OFF){
                CHECK_ERROR( checkVhdrSensor(res, pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrSensorMode, pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode, infoHelper) );

                if (pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
                    IHalSensorList *halSensorList = MAKE_HalSensorList();
                    if (halSensorList == nullptr) {
                        MY_LOGE("halSensorList == nullptr");
                        return -EINVAL;
                    }
                    IHalSensor *pHalSensor = halSensorList->createSensor(LOG_TAG, 1, (MUINT32*)&sensorId);
                    MUINT32 stagger_scenario = 0xff;//MUST SET TO 0xff
                    MUINT32 stagger_type = HDR_NONE;
                    if (pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP) {
                        stagger_type = HDR_RAW_STAGGER_2EXP;
                    }
                    else if (pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP) {
                        stagger_type = HDR_RAW_STAGGER_3EXP;
                    }
                    int sensorIndexDual = DUAL_CAMERA_MAIN_SENSOR;
                    int scenario = res.sensorMode;
                    if(pHalSensor != nullptr)
                    {
                        pHalSensor->sendCommand(sensorIndexDual, SENSOR_CMD_GET_STAGGER_TARGET_SCENARIO, (MUINTPTR)&scenario,(MUINTPTR)&stagger_type, (MUINTPTR)&stagger_scenario);
                    }
                    if (stagger_scenario == 0xff) {
                        //sensor dot not support HDR_RAW_STAGGER_3EXP for preview mode
                        MY_LOGE("Stagger not supported for current sensor mode!");
                    }
                    else if (stagger_scenario >= SENSOR_SCENARIO_ID_NORMAL_PREVIEW && stagger_scenario <=SENSOR_SCENARIO_ID_CUSTOM5) {
                        parseSensorSetting(res, stagger_scenario, infoHelper);
                    }
                }
            }
            {
                MINT32 forceSensorMode = ::property_get_int32("vendor.debug.cameng.force_sensormode", -1);
                if( forceSensorMode != -1 )
                {
                    switch( forceSensorMode )
                    {
                    case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
                         parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_PREVIEW,infoHelper);
                         break;
                    case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
                         parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_CAPTURE,infoHelper);
                         break;
                    case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
                         parseSensorSetting(res,SENSOR_SCENARIO_ID_NORMAL_VIDEO,infoHelper);
                         break;
                    case SENSOR_SCENARIO_ID_SLIM_VIDEO1:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_SLIM_VIDEO1,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_SLIM_VIDEO2:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_SLIM_VIDEO2,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM1:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM1,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM2:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM2,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM3:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM3,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM4:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM4,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM5:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM5,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM6:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM6,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM7:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM7,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM8:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM8,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM9:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM9,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM10:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM10,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM11:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM11,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM12:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM12,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM13:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM13,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM14:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM14,infoHelper);
                        break;
                    case SENSOR_SCENARIO_ID_CUSTOM15:
                        parseSensorSetting(res,SENSOR_SCENARIO_ID_CUSTOM15,infoHelper);
                        break;
                    default:
                        MY_LOGW("Unknown sensorMode: %d", forceSensorMode);
                        break;
                    }
                    CAM_ULOGMI("Force set sensorMode: %d. Selected sensorMode: %d", forceSensorMode, res.sensorMode);
                }
            }
            CAM_ULOGMI("select size(%4dx%-4d)@%-3d sensorMode:%d hdrSensorMode:%d hdrHalMode:%d HfpsMode:%d",
                res.sensorSize.w, res.sensorSize.h, res.sensorFps,
                res.sensorMode,
                pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrSensorMode, pStreamingFeatureSetting->p1ConfigParam.at(sensorId).hdrHalMode, streamHfpsMode);
            (*pvOut).emplace(sensorId, res);
        }
        return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

