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

#define LOG_TAG "mtkcam-SensorSettingPolicySMVR"

#include <mtkcam3/pipeline/policy/IConfigSensorSettingPolicy.h>
//
#include "MyUtils.h"

#include <isp_tuning_sensor.h>
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

#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {

/**
 * Make a function target - SMVRBatch
 */

typedef struct SmvrBSensorInfo
{
    MUINT fps = 0;
    MUINT w = 0;
    MUINT h = 0;
    MINT  sensorScen = -1;
} SmvrBSensorInfo;

static void compareAndSelectClosestCustomSensor(
    const int32_t logLevel,
    SmvrBSensorInfo const &target, SmvrBSensorInfo const &custom,
    SmvrBSensorInfo& outSelect)
{
    MY_LOGD_IF(logLevel >= 1, "SMVRBatch:: as-is: outSelect(fps(%d), %dx%d, sensorScen(%d)), "
            "target(fps(%d), %dx%d, sensorScen(%d)), "
            "custom(fps(%d), %dx%d, sensorScen(%d)) ",
        outSelect.fps, outSelect.w, outSelect.h, outSelect.sensorScen,
        target.fps, target.w, target.h, target.sensorScen,
        custom.fps, custom.w, custom.h, custom.sensorScen);

    if (target.fps <= custom.fps &&
        target.w   <= custom.w &&
        target.h   <= custom.h)
    {
        if (outSelect.fps == 0)
        {
            outSelect.fps = custom.fps;
            outSelect.w = custom.w;
            outSelect.h = custom.h;
            outSelect.sensorScen = custom.sensorScen;
        }
        else
        {
            if (outSelect.fps >= custom.fps &&
                outSelect.w   >= custom.w &&
                outSelect.h   >= custom.h)
            {
                outSelect.fps = custom.fps;
                outSelect.w = custom.w;
                outSelect.h = custom.h;
                outSelect.sensorScen = custom.sensorScen;
            }
        }
    }
    MY_LOGD_IF(logLevel >= 1, "SMVRBatch:: to-be: outSelect: fps(%d), %dx%d, sensorScen(%d)",
        outSelect.fps, outSelect.w, outSelect.h, outSelect.sensorScen);
}

FunctionType_Configuration_SensorSettingPolicy makePolicy_Configuration_SensorSetting_SMVRBatch()
{
    return [](Configuration_SensorSetting_Params const& params) -> int {
        auto pvOut = params.pvOut;
        //auto pStreamingFeatureSetting = params.pStreamingFeatureSetting;
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;

        auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;
        if ( CC_UNLIKELY( ! pvOut || ! pPipelineStaticInfo ) ) {
            MY_LOGE("error input params");
            return -EINVAL;
        }

        auto pParsedAppConfiguration = (pPipelineUserConfiguration != nullptr)
            ? pPipelineUserConfiguration->pParsedAppConfiguration : nullptr;
        auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr)
            ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;
        if ( CC_UNLIKELY( ! pParsedSMVRBatchInfo ) ) {
            MY_LOGE("error input for pParsedSMVRBatchInfo");
            return -EINVAL;
        }

        IHalSensorList * const pIHalSensorList = MAKE_HalSensorList();
        if ( CC_UNLIKELY( pIHalSensorList == nullptr) ) {
            MY_LOGE("error when MAKE_HalSensorList");
            return -EINVAL;
        }


        SmvrBSensorInfo targetSenInfo = {
            (MUINT) pParsedSMVRBatchInfo->maxFps,
            (MUINT) pParsedSMVRBatchInfo->imgW,
            (MUINT) pParsedSMVRBatchInfo->imgH,
            (MINT)  -1
        };

        SmvrBSensorInfo selectSenInfo;
        if ( pPipelineStaticInfo->sensorId.size() == 0 )
        {
            MY_LOGE("error no sensorId available");
            return -EINVAL;
        }
        int32_t sensorId = pPipelineStaticInfo->sensorId[0];
        NSCam::SensorStaticInfo sensorStaticInfo;
        memset(&sensorStaticInfo, 0, sizeof(NSCam::SensorStaticInfo));
        NSIspTuning::ESensorDev_T sensorDev =
            (NSIspTuning::ESensorDev_T)pIHalSensorList->querySensorDevIdx(sensorId);
        pIHalSensorList->querySensorStaticInfo(sensorDev, &sensorStaticInfo);

        SmvrBSensorInfo customSenInfo;

        // check video1
        customSenInfo.fps = sensorStaticInfo.video1FrameRate/10;
        customSenInfo.w = sensorStaticInfo.video1Width;
        customSenInfo.h = sensorStaticInfo.video1Height;
        customSenInfo.sensorScen = SENSOR_SCENARIO_ID_SLIM_VIDEO1;
        compareAndSelectClosestCustomSensor(pParsedSMVRBatchInfo->logLevel,
           targetSenInfo, customSenInfo, selectSenInfo);

        // check custom1
        customSenInfo.fps = sensorStaticInfo.custom1FrameRate/10;
        customSenInfo.w = sensorStaticInfo.SensorCustom1Width;
        customSenInfo.h = sensorStaticInfo.SensorCustom1Height;
        customSenInfo.sensorScen = SENSOR_SCENARIO_ID_CUSTOM1;
        compareAndSelectClosestCustomSensor(pParsedSMVRBatchInfo->logLevel,
           targetSenInfo, customSenInfo, selectSenInfo);
        // check custom2
        customSenInfo.fps = sensorStaticInfo.custom2FrameRate/10;
        customSenInfo.w = sensorStaticInfo.SensorCustom2Width;
        customSenInfo.h = sensorStaticInfo.SensorCustom2Height;
        customSenInfo.sensorScen = SENSOR_SCENARIO_ID_CUSTOM2;
        compareAndSelectClosestCustomSensor(pParsedSMVRBatchInfo->logLevel,
           targetSenInfo, customSenInfo, selectSenInfo);
        // check custom3
        customSenInfo.fps = sensorStaticInfo.custom3FrameRate/10;
        customSenInfo.w = sensorStaticInfo.SensorCustom3Width;
        customSenInfo.h = sensorStaticInfo.SensorCustom3Height;
        customSenInfo.sensorScen = SENSOR_SCENARIO_ID_CUSTOM3;
        compareAndSelectClosestCustomSensor(pParsedSMVRBatchInfo->logLevel,
           targetSenInfo, customSenInfo, selectSenInfo);
        // check custom4
        customSenInfo.fps = sensorStaticInfo.custom4FrameRate/10;
        customSenInfo.w = sensorStaticInfo.SensorCustom4Width;
        customSenInfo.h = sensorStaticInfo.SensorCustom4Height;
        customSenInfo.sensorScen = SENSOR_SCENARIO_ID_CUSTOM4;
        compareAndSelectClosestCustomSensor(pParsedSMVRBatchInfo->logLevel,
           targetSenInfo, customSenInfo, selectSenInfo);
        // check custom5
        customSenInfo.fps = sensorStaticInfo.custom5FrameRate/10;
        customSenInfo.w = sensorStaticInfo.SensorCustom5Width;
        customSenInfo.h = sensorStaticInfo.SensorCustom5Height;
        customSenInfo.sensorScen = SENSOR_SCENARIO_ID_CUSTOM5;
        compareAndSelectClosestCustomSensor(pParsedSMVRBatchInfo->logLevel,
           targetSenInfo, customSenInfo, selectSenInfo);
        if (selectSenInfo.fps == 0)
        {
            MY_LOGE("SMVRBatch:: !!err: sensorDev(%d): no suitable sensorMode "
                "to selectfor %dx%d @ %d",
                sensorId, targetSenInfo.w, targetSenInfo.h, targetSenInfo.fps);
            return -EINVAL;
        }
        // !!NOTES: fps and  size are determined at sensor-meta and passed by App
        SensorSetting res;  // output
        res.sensorFps  = static_cast<uint32_t>(selectSenInfo.fps);;
        res.sensorSize = MSize(selectSenInfo.w, selectSenInfo.h);
        res.sensorMode = selectSenInfo.sensorScen;

        MY_LOGD("SMVRBatch:: sensor id/dev(%d/%d): select sensorMode: %d, size(%dx%d) @ %d fps",
            sensorId, sensorDev,
            res.sensorMode, res.sensorSize.w, res.sensorSize.h, res.sensorFps);
        (*pvOut).emplace(sensorId, res);

        return OK;
    };
}

};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
