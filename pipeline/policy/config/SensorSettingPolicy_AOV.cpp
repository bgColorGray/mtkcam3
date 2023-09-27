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

#define LOG_TAG "mtkcam-SensorSettingPolicy_AOV"

#include <mtkcam3/pipeline/policy/IConfigSensorSettingPolicy.h>
//
#include "MyUtils.h"

#include <mtkcam/drv/IHalSensor.h>
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
#define MY_LOGD(fmt, arg...)  CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)  CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)  CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)  CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)  CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...) do { if (           (cond)) { MY_LOGD(__VA_ARGS__); } } while(0)
#define MY_LOGI_IF(cond, ...) do { if (           (cond)) { MY_LOGI(__VA_ARGS__); } } while(0)
#define MY_LOGW_IF(cond, ...) do { if (CC_UNLIKELY(cond)) { MY_LOGW(__VA_ARGS__); } } while(0)
#define MY_LOGE_IF(cond, ...) do { if (CC_UNLIKELY(cond)) { MY_LOGE(__VA_ARGS__); } } while(0)
#define MY_LOGF_IF(cond, ...) do { if (CC_UNLIKELY(cond)) { MY_LOGF(__VA_ARGS__); } } while(0)

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {

/**
 * Make a function target - AOV
 */
FunctionType_Configuration_SensorSettingPolicy makePolicy_Configuration_SensorSetting_AOV()
{
    return [](Configuration_SensorSetting_Params const& params) -> int {
        auto pvOut = params.pvOut;
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;
        auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;
        MY_LOGF_IF(!pvOut, "Bad arguments");
        MY_LOGF_IF(!pPipelineStaticInfo, "Bad arguments");
        MY_LOGF_IF(!pPipelineUserConfiguration, "Bad arguments");
        MY_LOGF_IF(pPipelineStaticInfo->sensorId.empty(), "Bad arguments");

        uint32_t sensorMode = -1;
        int32_t sensorFps = -1;
        MSize sensorSize;

        const int32_t sensorId = pPipelineStaticInfo->sensorId[0];
        const int32_t aovMode = pPipelineUserConfiguration->pParsedAppConfiguration->aovMode;

        // sensor mode
        sensorMode = [&]() {
            static const std::string prefix{"persist.vendor.camera.sensormode.aov"};
            // const std::string key = prefix + std::to_string(sensorId);
            const std::string key = prefix;
            int32_t prop = ::property_get_int32(key.c_str(), -1);
            MY_LOGI("property_get: %s=%d", key.c_str(), prop);

            int32_t sensorMode = 0;
            switch (aovMode) {
            case 1:   // AOV mode: step1
                sensorMode = (prop >= 0) ? prop : SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
                break;
            case 2:   // AOV mode: step2
            default:
                sensorMode = (prop >= 0) ? prop : SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
                break;
            }
            return sensorMode;
        }();

        // sensor resolution & fps
        {
            HwInfoHelper infoHelper(sensorId);
            infoHelper.updateInfos();
            bool ret = infoHelper.getSensorFps(sensorMode, sensorFps)
                    && infoHelper.getSensorSize(sensorMode, sensorSize);
            if (!ret) {
                MY_LOGE("Fail to get sensor fps:%d or size:%dx%d - sensorMode:%d",
                        sensorFps, sensorSize.w, sensorSize.h, sensorMode);
                return -1;
            }
        }

        (*pvOut)[sensorId] = SensorSetting{
            .sensorFps  = static_cast<uint32_t>(sensorFps),
            .sensorSize = sensorSize,
            .sensorMode = sensorMode,
            };

        MY_LOGI("Select sensorMode:%d sensorSize(%dx%d) @ %d",
                sensorMode, sensorSize.w, sensorSize.h, sensorFps);
        return 0;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

