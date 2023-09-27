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
/* MediaTek Inc. (C) 2019. All rights reserved.
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

/*******************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES
 *AND AGREES THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK
 *SOFTWARE") RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO
 *BUYER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 *WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 *WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 *NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 *RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED
 *IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO
 *SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
 *SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 *LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT
 *MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR
 *REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK
 *FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS
 *PRINCIPLES.
 ******************************************************************************/
#ifndef FEATURE_COMMON_VSDOF_PROVIDERS_SETTING_PROVIDER_FOV_CROP_UTIL_H_
#define FEATURE_COMMON_VSDOF_PROVIDERS_SETTING_PROVIDER_FOV_CROP_UTIL_H_

#include <camera_custom_stereo.h>   // For DEFAULT_STEREO_SETTING
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>

#include <utility>
#include <algorithm>

#include "./stereo_setting_def.h"

using namespace std;
using namespace NSCam;
using namespace StereoHAL;

/**
 * \brief Get "keeped degrees" for a ratio to a degree
 * \details E.g. 60 degrees, ratio 0.95, return 57.488
 *
 * \param degree Degree before crop
 * \param ratio Ratio to cropped degree
 *
 * \return Degrees keeped from input degree
 */
inline float cropRatioToKeepDegree(float &degree, float ratio) {
    if (ratio < 0.0f ||
        ratio > 1.0f) {
        return degree;
    }

    degree = 2.0f*atan(ratio*tan(degree/2.0f));
    return degree;
}

/**
 * \brief Get "cropped degrees" for a ratio to a degree
 * \details E.g. 60 degrees, ratio 0.95, return 2.51198239
 *
 * \param degree Degree before crop
 * \param ratio Ratio to cropped degree
 *
 * \return Degrees to crop from input degree
 */
inline float cropRatioToDegree(float &degree, float ratio) {
    degree -= cropRatioToKeepDegree(degree, ratio);
    return degree;
}

class FOVCropUtil {
 public:
    static bool updateFOVCropRatios(StereoSensorConbinationSetting_T &sensorCombinatioSetting, float previewCropRatio) {
        std::lock_guard<std::mutex> lock(__lock);
        bool result = true;

        auto &sensorSettings = sensorCombinatioSetting.sensorSettings;
        if(sensorSettings.size() < 2) {
            return false;
        }

        auto it = sensorSettings[0]->cropSettingMap.find(previewCropRatio);
        if(it != sensorSettings[0]->cropSettingMap.end()) {
            return true;
        }

        if (sensorCombinatioSetting.disableCrop) {
            __updateCropSetting(sensorSettings[0], previewCropRatio, previewCropRatio);
            __updateCropSetting(sensorSettings[1], previewCropRatio, previewCropRatio);
            return true;
        }

        const bool IS_VERTICAL = (1 == sensorCombinatioSetting.moduleType ||
                                  3 == sensorCombinatioSetting.moduleType);
        const bool IS_BACK_CAM = (0 == sensorCombinatioSetting.sensorSettings[0]->staticInfo.facingDirection);
        bool IS_MAIN_ON_LEFT = true;
        if (IS_BACK_CAM) {
            if (3 == sensorCombinatioSetting.moduleType ||
                4 == sensorCombinatioSetting.moduleType) {
                IS_MAIN_ON_LEFT = false;
            }
        } else {
            if (3 == sensorCombinatioSetting.moduleType) {
                IS_MAIN_ON_LEFT = false;
            }
        }

        float main1FOV;
        float main2FOV;
        float moduleVarMain1 = sensorSettings[0]->fovVariationSensorOutput;
        float fovHRuntime;
        float fovVRuntime;
        if (IS_VERTICAL) {
            __getRuntimeFOVByRatio(sensorSettings[0], previewCropRatio, fovHRuntime, fovVRuntime);
            main1FOV = fovVRuntime;
            __getRuntimeFOVByRatio(sensorSettings[1], 1.0f, fovHRuntime, fovVRuntime);
            main2FOV = fovVRuntime;

            if (previewCropRatio < 1.0f) {
                moduleVarMain1 = atan(tan((sensorCombinatioSetting.sensorSettings[0]->fovVSensorOutput+moduleVarMain1)/2.0f)*previewCropRatio)*2.0f - main1FOV;
            }
        } else {
            __getRuntimeFOVByRatio(sensorSettings[0], previewCropRatio, fovHRuntime, fovVRuntime);
            main1FOV = fovHRuntime;
            __getRuntimeFOVByRatio(sensorSettings[1], 1.0f, fovHRuntime, fovVRuntime);
            main2FOV = fovHRuntime;

            if (previewCropRatio < 1.0f) {
                moduleVarMain1 = atan(tan((sensorCombinatioSetting.sensorSettings[0]->fovHSensorOutput+moduleVarMain1)/2.0f)*previewCropRatio)*2.0f - main1FOV;
            }
        }

        if (0 == main1FOV || 0 == main2FOV) {
            return false;
        }

        // Consider inf side FOV coverage
        bool cropMain1 = (main1FOV+2.0f*moduleVarMain1 >= main2FOV);

        // Consider near side FOV coverage
        if (!cropMain1) {
            // working range should be >= baseline(tan(Main2_FOV)-tan(Main1_FOV)) and consider module variation
            float fovFactor = std::min(tan(main2FOV/2.0f-sensorSettings[1]->fovVariationSensorOutput)-tan(main1FOV/2.0f),
                                       tan(main2FOV/2.0f)-tan((main1FOV+moduleVarMain1)/2.0f));
            cropMain1 = (sensorCombinatioSetting.workingRange - sensorCombinatioSetting.baseline/fovFactor < 0);
        }

        // Set croppedSensor to main1
        StereoSensorSetting_T *nonCropSensor = sensorSettings[0];
        StereoSensorSetting_T *croppedSensor = sensorSettings[1];
        if (cropMain1) {
            std::swap(nonCropSensor, croppedSensor);
        }

        float INNER_FOV;
        float OUTER_FOV;
        float INNER_FOV_VAR;
        float OUTER_FOV_VAR;
        if(main1FOV < main2FOV) {
            INNER_FOV = main1FOV;
            OUTER_FOV = main2FOV;
            INNER_FOV_VAR = moduleVarMain1;
            OUTER_FOV_VAR = sensorSettings[1]->fovVariationSensorOutput;
        } else {
            INNER_FOV = main2FOV;
            OUTER_FOV = main1FOV;
            INNER_FOV_VAR = sensorSettings[1]->fovVariationSensorOutput;
            OUTER_FOV_VAR = moduleVarMain1;
        }

        const float HALF_INNER_FOV = INNER_FOV/2.0f;
        const float HALF_OUTER_FOV = OUTER_FOV/2.0f;
        const float BASELINE_RATIO = sensorCombinatioSetting.baseline / sensorCombinatioSetting.workingRange;

        // Calculate inf & near side
        float INF_RATIO  = 1.0f;
        float NEAR_RATIO = 1.0f;
        if (cropMain1) {
            // Crop main1 as mush as posible
            INF_RATIO = radiansRatio(INNER_FOV - 2.0f*INNER_FOV_VAR, OUTER_FOV);
            NEAR_RATIO = std::min((tan(HALF_INNER_FOV - INNER_FOV_VAR) - BASELINE_RATIO) / tan(HALF_OUTER_FOV),
                                  (tan(HALF_INNER_FOV) - BASELINE_RATIO) / tan(HALF_OUTER_FOV + OUTER_FOV_VAR));
        } else {
            // Keep main2 as mush as posible
            INF_RATIO = radiansRatio(INNER_FOV + 2.0f*INNER_FOV_VAR, OUTER_FOV);
            NEAR_RATIO = std::max((tan(HALF_INNER_FOV + INNER_FOV_VAR) + BASELINE_RATIO) / tan(HALF_OUTER_FOV),
                                  (tan(HALF_INNER_FOV) + BASELINE_RATIO) / tan(HALF_OUTER_FOV-OUTER_FOV_VAR));
        }

        INF_RATIO  = std::min(INF_RATIO,  1.0f);
        NEAR_RATIO = std::min(NEAR_RATIO, 1.0f);

        if (sensorCombinatioSetting.isCenterCrop) {
            if (cropMain1) {
                __updateCropSetting(croppedSensor, previewCropRatio, std::min(INF_RATIO, NEAR_RATIO));
            } else {
                __updateCropSetting(croppedSensor, previewCropRatio, std::max(INF_RATIO, NEAR_RATIO));
            }
        } else {
            // Crop direction is from cropped sensor to non-cropped sensor
            // If crop main1, main1 inf side is toward main2
            // If crop main2, main2 near side is toward main1,
            // however CropUtil is refer to main1 direction, so we should apply inf side ratio
            __updateCropSetting(croppedSensor, previewCropRatio,
                                (INF_RATIO + NEAR_RATIO)/2.0f,
                                (1.0f-INF_RATIO)/(1.0f-NEAR_RATIO + 1.0f-INF_RATIO));
        }

        __updateCropSetting(nonCropSensor, previewCropRatio, (cropMain1)?1.0f:previewCropRatio);

        return result;
    }

private:
    static void __updateCropSetting(StereoSensorSetting_T *sensorSetting,
                                    float previewCropRatio,
                                    float keepRatio,
                                    float baselineRatio=0.5f)
    {
        auto it = sensorSetting->cropSettingMap.find(previewCropRatio);
        if(it == sensorSetting->cropSettingMap.end()) {
            StereoCropSetting_T cropSetting;
            cropSetting.keepRatio = keepRatio;
            cropSetting.baselineRatio = baselineRatio;
            __getRuntimeFOVByRatio(sensorSetting, keepRatio,
                                   cropSetting.fovHRuntime, cropSetting.fovVRuntime);
            cropSetting.cropDegreeH = sensorSetting->fovHSensorOutput - cropSetting.fovHRuntime;
            cropSetting.cropDegreeV = sensorSetting->fovVSensorOutput - cropSetting.fovVRuntime;
            sensorSetting->cropSettingMap.emplace(previewCropRatio, cropSetting);
        }
    }

    static void __getRuntimeFOVByRatio(StereoSensorSetting_T *sensorSetting,
                                       float ratio,
                                       float &fovHRuntim, float &fovVRuntime)
    {
        fovHRuntim  = 2.0f*atan(ratio*tan(sensorSetting->fovHSensorOutput/2.0f));
        fovVRuntime = 2.0f*atan(ratio*tan(sensorSetting->fovVSensorOutput/2.0f));
    }

private:
    static std::mutex __lock;
};

#endif  // FEATURE_COMMON_VSDOF_PROVIDERS_SETTING_PROVIDER_FOV_CROP_UTIL_H_
