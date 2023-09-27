/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "StereoSettingProviderKernel"

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
CAM_ULOG_DECLARE_MODULE_ID(CAM_ULOG_MODULE_ID);
#include "stereo_setting_provider_kernel.h"
#include <string.h>

#include <camera_custom_stereo.h>   //For DEFAULT_STEREO_SETTING
#include <vsdof/hal/ProfileUtil.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <cctype>   //isspace
#include <cmath>    //pow, tan, atan, sqrt
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <isp_tuning.h>
#include <set>

using namespace StereoHAL;

Mutex StereoSettingProviderKernel::__instanceLock;
StereoSettingProviderKernel * StereoSettingProviderKernel::__instance = NULL;

#define SETTING_HEADER_SDCARD_PATH      "/sdcard/camera_custom_stereo_setting.h"
#define SETTING_HEADER_VENDOR_PATH      "/data/vendor/camera_dump/camera_custom_stereo_setting.h"

#if defined(__func__)
#undef __func__
#endif
#define __func__ __FUNCTION__

#define MY_LOGD(fmt, arg...)    CAM_ULOGMD("[%s]" fmt, __func__, ##arg)
#define MY_LOGI(fmt, arg...)    CAM_ULOGMI("[%s]" fmt, __func__, ##arg)
#define MY_LOGW(fmt, arg...)    CAM_ULOGMW("[%s]" fmt"\n", __func__, ##arg)
#define MY_LOGE(fmt, arg...)    CAM_ULOGME("[%s]" fmt"\n", __func__, ##arg)

#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }

using namespace std;
using namespace NSCam;
using namespace StereoHAL;
using namespace NSCam::v1::Stereo;

//Used by stereo_setting_def.h
std::unordered_map<int, std::string> ISP_PROFILE_MAP =
{
    { NSIspTuning::EIspProfile_N3D_Preview,          "EIspProfile_N3D_Preview"          },

#if (1==HAS_P1YUV)
    { NSIspTuning::EIspProfile_P1_YUV_Depth,         "EIspProfile_P1_YUV_Depth"         },
#endif

#if (2==HAS_P1YUV || 6==HAS_P1YUV)
    { NSIspTuning::EIspProfile_VSDOF_PV_Main_toW,    "EIspProfile_VSDOF_PV_Main_toW"    },
    { NSIspTuning::EIspProfile_VSDOF_PV_Aux,         "EIspProfile_VSDOF_PV_Aux"         },
    { NSIspTuning::EIspProfile_VSDOF_PV_Main,        "EIspProfile_VSDOF_PV_Main"        },
    { NSIspTuning::EIspProfile_VSDOF_Video_Main_toW, "EIspProfile_VSDOF_Video_Main_toW" },
    { NSIspTuning::EIspProfile_VSDOF_Video_Aux,      "EIspProfile_VSDOF_Video_Aux"      },
    { NSIspTuning::EIspProfile_VSDOF_Video_Main,     "EIspProfile_VSDOF_Video_Main"     },
#endif
};

/**
 * \brief Get internal used sensor name
 * \details The name in the setting may contains driver prefix and sensor index
 *          e.g.1 For given name "SENSOR_DRVNAME_imx386_mipi_raw:3"
 *                we'll return "IMX386_MIPI_RAW" and return 3
 *          e.g.2 For given name "imx386_mipi_raw"
 *                we'll return "IMX386_MIPI_RAW" and return -1
 *
 * \param name name to input and return
 * \return specified sensor index, if not specified, return -1
 */
int32_t __parseAndRefineSensorName(std::string &name)
{
    // Remove prefix
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    size_t prefixPos = name.find(SENSOR_DRVNAME_PREFIX);
    if(prefixPos != std::string::npos) {
        name.erase(prefixPos, SENSOR_DRVNAME_PREFIX.length());
    }

    // Handle specified sensor ID
    size_t deliPos = name.find(DELIMITER);
    int customizeId = -1;
    if(deliPos != std::string::npos) {
        auto idString = name.substr(deliPos+DELIMITER.length());
        customizeId = std::stoi(idString);
        name = name.substr(0, deliPos);
    }

    return customizeId;
}

void from_json(const _JSON_TYPE &settingJson, StereoSensorSetting_T& setting)
{
    //Get Name(MUST)
    //  Name must copy from kernel-4.9\drivers\misc\mediatek\imgsensor\inc\kd_imgsensor.h,
    //  definition of SENSOR_DRVNAME_<sensor name>
    std::string name = settingJson[CUSTOM_KEY_NAME].get<std::string>();
    int customizeId = __parseAndRefineSensorName(name);
    setting.name = name;
    if(customizeId >= 0)
    {
        setting.index = customizeId;
    }

    MSize size;
    STEREO_IMAGE_RATIO_T ratio;

    //Get cusomized IMGO YUV size(optional, used by pure 3rd party flow)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_IMGOYUV_SIZE))
    {
        const _JSON_TYPE &sizeConfig = settingJson[CUSTOM_KEY_IMGOYUV_SIZE];
        for(_JSON_TYPE::const_iterator it = sizeConfig.begin(); it != sizeConfig.end(); ++it)
        {
            ratio = STEREO_IMAGE_RATIO_T(it.key(), true);
            size = stringToSize(it.value().get<std::string>());
            setting.imgoYuvSize[ratio] = size;
        }
    }

    //Get cusomized RRZO YUV size(optional, used by pure 3rd party flow)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_RRZOYUV_SIZE))
    {
        const _JSON_TYPE &sizeConfig = settingJson[CUSTOM_KEY_RRZOYUV_SIZE];
        for(_JSON_TYPE::const_iterator it = sizeConfig.begin(); it != sizeConfig.end(); ++it)
        {
            ratio = STEREO_IMAGE_RATIO_T(it.key(), true);
            size = stringToSize(it.value().get<std::string>());
            setting.rrzoYuvSize[ratio] = size;
        }
    }

    //Get sensor scenario of ZSD mode(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_SENSOR_SCENARIO_ZSD))
    {
        int feature;
        const _JSON_TYPE &config = settingJson[CUSTOM_KEY_SENSOR_SCENARIO_ZSD];
        for(_JSON_TYPE::const_iterator it = config.begin(); it != config.end(); ++it)
        {
            feature = stringToFeatureMode(it.key());
            setting.sensorScenarioMapZSD[feature] = stringToSensorScenario(it.value().get<std::string>());
        }
    }

    //Get sensor scenario of Recording mode(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_SENSOR_SCENARIO_REC))
    {
        int feature;
        const _JSON_TYPE &config = settingJson[CUSTOM_KEY_SENSOR_SCENARIO_REC];
        for(_JSON_TYPE::const_iterator it = config.begin(); it != config.end(); ++it)
        {
            feature = stringToFeatureMode(it.key());
            setting.sensorScenarioMapRecord[feature] = stringToSensorScenario(it.value().get<std::string>());
        }
    }

    //Get FOV(optional, use static info(integer) if not set)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_FOV)) {
        const _JSON_TYPE &fovValue = settingJson[CUSTOM_KEY_FOV];
        if(LogicalCamJSONUtil::HasMember(fovValue, CUSTOM_KEY_FOV_D)) {
            setting.fovDiagonal   = degreeToRadians(fovValue[CUSTOM_KEY_FOV_D].get<float>());
        } else {
            setting.fovHorizontal = degreeToRadians(fovValue[CUSTOM_KEY_FOV_H].get<float>());
            setting.fovVertical   = degreeToRadians(fovValue[CUSTOM_KEY_FOV_V].get<float>());
        }
    }

    //Get calibration data(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_CALIBRATION)) {
        const _JSON_TYPE &calibrationValue = settingJson[CUSTOM_KEY_CALIBRATION];
        setting.distanceMacro    = calibrationValue[CUSTOM_KEY_MACRO_DISTANCE].get<int>();
        setting.distanceInfinite = calibrationValue[CUSTOM_KEY_INFINITE_DISTANCE].get<int>();
    }

    //Get FRZ Ratio(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_FRZ_RATIO))
    {
        int feature;
        const _JSON_TYPE &config = settingJson[CUSTOM_KEY_FRZ_RATIO];
        for(_JSON_TYPE::const_iterator it = config.begin(); it != config.end(); ++it)
        {
            feature = stringToFeatureMode(it.key());
            setting.frzRatioSetting[feature] = it.value().get<float>();
        }
    }
}

void from_json(const _JSON_TYPE &settingJson, StereoSensorConbinationSetting_T& setting)
{
    int m, n;
    MSize size;
    STEREO_IMAGE_RATIO_T ratio;

    //Get logical device(MUST for HAL3)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_LOGICAL_DEVICE)) {
        //Name(MUST)
        const _JSON_TYPE &logicalDeviceJson = settingJson[CUSTOM_KEY_LOGICAL_DEVICE];
        if(LogicalCamJSONUtil::HasMember(logicalDeviceJson, CUSTOM_KEY_NAME)) {
            setting.logicalDeviceName = logicalDeviceJson[CUSTOM_KEY_NAME].get<std::string>();
        } else {
            MY_LOGE("Name is undefined for logical device");
            return;
        }

        //Feature list(MUST)
        if(LogicalCamJSONUtil::HasMember(logicalDeviceJson, CUSTOM_KEY_FEATURES)) {
            for(auto &feature : logicalDeviceJson[CUSTOM_KEY_FEATURES]) {
                setting.logicalDeviceFeatureSet |= stringToFeatureMode(feature.get<std::string>());
            }
        } else {
            MY_LOGE("Features is undefined for logical device");
            return;
        }
    }

    // Get sensor offset
    // If this setting has been set,
    // we'll ignore module type, baseline, module degree setting
    if (LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_SENSOR_OFFSET)) {
        //=========================== UT CASES ===========================
        //     Offset        Module Type  Beseline  Module Rotation
        //  1. [0, 2]             4           2             0
        //  2. [2, 2]             4         2.828          45
        //  3. [1.732, 1]         4           2            60
        //  4. [2, 0]             3           2            90
        //  5. [1.732, -1]        2           2           300 (-60)
        //  6. [2, -2]            2         2.828         315 (-45)
        //  7. [0, -2]            2           2             0
        //  8. [-2, -2]           2         2.828          45
        //  9. [-1.732, -1]       2           2            60
        // 10. [-2, 0]            1           2            90
        // 11. [-1.732, 1]        4           2           300 (-60)
        // 12. [-2, 2]            4         2.828         315 (-45)
        //================================================================
        const _JSON_TYPE& offsetConfig = settingJson[CUSTOM_KEY_SENSOR_OFFSET];
        float x;
        float y;
        for (const auto &offset : offsetConfig) {
            if (offset.type() != _JSON_TYPE::value_t::array) {
                auto coord = offsetConfig.get<std::vector<float>>();
                x = coord[0];
                y = coord[1];
                if (x != 0 || y != 0) {
                    setting.sensorOffset.push_back({x, y});
                }
                break;
            } else {
                auto coord = offset.get<std::vector<float>>();
                x = coord[0];
                y = coord[1];
                if (x != 0 || y != 0) {
                    setting.sensorOffset.push_back({x, y});
                }
            }
        }

        if (setting.sensorOffset.size() > 1) {
            x = setting.sensorOffset[1].first;
            y = setting.sensorOffset[1].second;

            setting.baseline = std::sqrt(x*x + y*y);

            if (y == 0) {
                setting.moduleType = (x > 0) ? 3 : 1;
                // We'll decide moduleRotation later for front camera
            } else {
                setting.moduleType = (y > 0) ? 4 : 2;
                if (x == 0) {
                    setting.moduleRotation = 0;
                } else {
                    setting.moduleRotation = static_cast<int>(
                        std::round(StereoHAL::radiansToDegree(
                                    std::fabs(
                                        std::atan(static_cast<float>(x)/y)))));
                    if ((y < 0 && x > 0) ||
                        (x < 0 && y > 0)) {
                        setting.moduleRotation = 360 - setting.moduleRotation;
                    }
                }
            }
        }
    } else {
        //Get Baseline(optional for pure 3rd party & slant camera)
        if (LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_BASELINE)) {
            setting.baseline = settingJson[CUSTOM_KEY_BASELINE].get<float>();
        }

        //Get module type(MUST for stereo, will check later & slant camera
        if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_MODULE_TYPE)) {
            setting.moduleType = settingJson[CUSTOM_KEY_MODULE_TYPE].get<int>();
        }

        // Get module rotation, this setting must work with baseline
        if (LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_MODULE_DEGREE)) {
            setting.moduleRotation = settingJson[CUSTOM_KEY_MODULE_DEGREE].get<float>();
        }
    }

    //Get module variation(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_MODULE_VARIATION)) {
        setting.moduleVariation = settingJson[CUSTOM_KEY_MODULE_VARIATION].get<float>();
        if(setting.moduleVariation < 0.0f) {
            setting.moduleVariation = 0.0f;
        } else {
            setting.moduleVariation = degreeToRadians(setting.moduleVariation);
        }
    }

    //Get working range(optional, should adjust for W+T VSDoF)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_WORKING_RANGE)) {
        setting.workingRange = settingJson[CUSTOM_KEY_WORKING_RANGE].get<float>();
        if(setting.workingRange <= 0) {
            setting.workingRange = DEFAULT_WORKING_RANGE;
        }
    }

    //Get FOV crop(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_FOV_CROP)) {
        const _JSON_TYPE &fovCropValue = settingJson[CUSTOM_KEY_FOV_CROP];
        if(LogicalCamJSONUtil::HasMember(fovCropValue, CUSTOM_KEY_CENTER_CROP)) {
            setting.isCenterCrop = !!(fovCropValue[CUSTOM_KEY_CENTER_CROP].get<int>());
        }

        if(LogicalCamJSONUtil::HasMember(fovCropValue, CUSTOM_KEY_DISABLE_CROP)) {
            setting.disableCrop = !!(fovCropValue[CUSTOM_KEY_DISABLE_CROP].get<int>());
        }
    }

    //Get customized depthmap size(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_DEPTHMAP_SIZE)) {
        const _JSON_TYPE &sizeConfig = settingJson[CUSTOM_KEY_DEPTHMAP_SIZE];
        for(_JSON_TYPE::const_iterator it = sizeConfig.begin(); it != sizeConfig.end(); ++it)
        {
            ratio = STEREO_IMAGE_RATIO_T(it.key(), true);
            size = stringToSize(it.value().get<std::string>());
            setting.depthmapSize[ratio] = size;
        }
    }

    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_DEPTHMAP_CAPTURE_SIZE)) {
        const _JSON_TYPE &sizeConfig = settingJson[CUSTOM_KEY_DEPTHMAP_CAPTURE_SIZE];
        for(_JSON_TYPE::const_iterator it = sizeConfig.begin(); it != sizeConfig.end(); ++it)
        {
            ratio = STEREO_IMAGE_RATIO_T(it.key(), true);
            size = stringToSize(it.value().get<std::string>());
            setting.depthmapSizeCapture[ratio] = size;
        }
    }

    //Get LDC(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_LDC)) {
        setting.LDC.clear();

        stringstream ss;
        float number;
        for(auto &line : settingJson[CUSTOM_KEY_LDC]) {
            std::string strLDC = line.get<std::string>();
            char *start = (char *)strLDC.c_str();
            if(start) {
                char *end = NULL;
                do {
                    number = ::strtof(start, &end);
                    if  ( start == end ) {
                        // MY_LOGD("No LDC data: %s", start);
                        break;
                    }

                    if(number < HUGE_VALF &&
                       number > -HUGE_VALF)
                    {
                        setting.LDC.push_back(number);
                    }
                    start = end + 1;
                } while ( end && *end );
            }
        }

        //Workaround for N3D crash issue
        if(setting.LDC.size() == 0) {
            setting.LDC.push_back(0);
        }

        setting.enableLDC = (setting.LDC.size() > 0);
    }

    //Get offline calibration(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_CALIBRATION)) {
        setting.calibrationData.clear();
        base64Decode(settingJson[CUSTOM_KEY_CALIBRATION].get<std::string>().c_str(), setting.calibrationData);
    }

    //Get customized base size(optional)
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_SIZE_CONFIG)) {
        const _JSON_TYPE &sizeValue = settingJson[CUSTOM_KEY_SIZE_CONFIG];

        StereoArea area;
        MSize contentSize;

        for(_JSON_TYPE::const_iterator it = sizeValue.begin(); it != sizeValue.end(); ++it)
        {
            ratio = STEREO_IMAGE_RATIO_T(it.key());
            const _JSON_TYPE &config = it.value();
            area.padding = stringToSize(config[CUSTOM_KEY_PADDING].get<std::string>());
            area.startPt.x = area.padding.w/2;
            area.startPt.y = area.padding.h/2;
            contentSize = stringToSize(config[CUSTOM_KEY_CONTENT_SIZE].get<std::string>());
            if(0 != contentSize.w % 16) {
                MSize newSize = contentSize;
                //Width must be 16-align
                applyNAlign(16, newSize.w);

                ratio.MToN(m, n);

                //Height must be even
                if(1 == setting.moduleType ||
                   3 == setting.moduleType)
                {
                    newSize.h = newSize.w * m / n;
                } else {
                    newSize.h = newSize.w * n / m;
                }
                applyNAlign(2, newSize.h);

                MY_LOGW("Content width must be 16-aligned, adjust size for 16:9 from %dx%d to %dx%d",
                        contentSize.w, contentSize.h, newSize.w, newSize.h);
                contentSize = newSize;
            }

            area.size = contentSize + area.padding;
            setting.baseSize[ratio] = area;
            setting.hasSizeConfig = true;
        }
    }

    // Get customized depthmap refine level
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_REFINE_LEVEL))
    {
        auto checkLevel = [&](int level) -> ENUM_DEPTHMAP_REFINE_LEVEL
        {
            if(level < E_DEPTHMAP_REFINE_NONE ||
               level >= E_DEPTHMAP_REFINE_MAX)
            {
                level = E_DEPTHMAP_REFINE_DEFAULT;
            }

            return static_cast<ENUM_DEPTHMAP_REFINE_LEVEL>(level);
        };

        const _JSON_TYPE &config = settingJson[CUSTOM_KEY_REFINE_LEVEL];
        ENUM_DEPTHMAP_REFINE_LEVEL level = E_DEPTHMAP_REFINE_DEFAULT;
        if(config.type() != _JSON_TYPE::value_t::object) {
            level = checkLevel(config.get<int>());
            setting.depthmapRefineLevel.emplace(eSTEREO_SCENARIO_PREVIEW, level);
            setting.depthmapRefineLevel.emplace(eSTEREO_SCENARIO_RECORD, level);
        } else {
            if(LogicalCamJSONUtil::HasMember(config, "Preview")) {
                level = checkLevel(config["Preview"].get<int>());
                setting.depthmapRefineLevel.emplace(eSTEREO_SCENARIO_PREVIEW, level);
            }

            if(LogicalCamJSONUtil::HasMember(config, "Record")) {
                level = checkLevel(config["Record"].get<int>());
                setting.depthmapRefineLevel.emplace(eSTEREO_SCENARIO_RECORD, level);
            }
        }
    }

    // Get customized zoom range
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_MULTICAM_ZOOM_RANGE)) {
        setting.multicamZoomRanges = settingJson[CUSTOM_KEY_MULTICAM_ZOOM_RANGE].get<std::vector<float>>();
        if(setting.multicamZoomRanges.size() == 0) {
            setting.multicamZoomRanges.push_back(1.0f);
        } else {
            std::sort(setting.multicamZoomRanges.begin(), setting.multicamZoomRanges.end());
        }
    }

    // Get customized zoom steps
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_MULTICAM_ZOOM_STEPS)) {
        setting.multicamZoomSteps = settingJson[CUSTOM_KEY_MULTICAM_ZOOM_STEPS].get<std::vector<float>>();
        if(setting.multicamZoomSteps.size() == 0) {
            setting.multicamZoomSteps.push_back(1.0f);
        } else {
            std::sort(setting.multicamZoomSteps.begin(), setting.multicamZoomSteps.end());
        }
    }

    // Get depth flow(optional)
    if(IS_STEREO_MODE(setting.logicalDeviceFeatureSet))
    {
        if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_DEPTH_FLOW))
        {
            if(settingJson[CUSTOM_KEY_DEPTH_FLOW].type() == _JSON_TYPE::value_t::number_integer ||
               settingJson[CUSTOM_KEY_DEPTH_FLOW].type() == _JSON_TYPE::value_t::number_unsigned)
            {
                setting.depthFlow = settingJson[CUSTOM_KEY_DEPTH_FLOW].get<int>();
            }
            else if(settingJson[CUSTOM_KEY_DEPTH_FLOW].type() == _JSON_TYPE::value_t::string)
            {
                std::string flow = settingJson[CUSTOM_KEY_DEPTH_FLOW].get<std::string>();
                std::transform(flow.begin(), flow.end(), flow.begin(), ::tolower);
                int flowValue = stringToFeatureMode(flow);

                if(flowValue & NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY)
                {
                    setting.depthFlow = 1;
                }
                else if(flowValue & NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP)
                {
                    setting.depthFlow = 2;
                }
                else
                {
                    setting.depthFlow = 0;
                }

                setting.logicalDeviceFeatureSet |= (flowValue & NSCam::v1::Stereo::E_MULTICAM_FEATURE_HIDL_FLAG);
            }
        }

        if(setting.depthFlow <= 0) {
            if(setting.logicalDeviceFeatureSet & NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY) {
                setting.depthFlow = 1;
            } else if(setting.logicalDeviceFeatureSet & NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP) {
                setting.depthFlow = 2;
            } else {
                setting.depthFlow = getStereoModeType();
            }
        }

        //Get depth delay
        if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_DEPTH_DELAY))
        {
            setting.depthDelay = settingJson[CUSTOM_KEY_DEPTH_DELAY].get<int>();
        }

        //Get depth freq setting
        if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_DEPTH_FREQ)) {
            const _JSON_TYPE &freqSetting = settingJson[CUSTOM_KEY_DEPTH_FREQ];
            if(freqSetting.type() == _JSON_TYPE::value_t::object) {
                for(auto &s : freqSetting.items()) {
                    std::string key = s.key();
                    int freq = s.value().get<int>();
                    if(freq > 0) {
                        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                        if(key.find("pre") == 0) {
                            setting.depthFreq.emplace(eSTEREO_SCENARIO_PREVIEW, freq);
                        } else if(key.find("rec") == 0 ||
                                  key.find("vid") == 0) {
                            setting.depthFreq.emplace(eSTEREO_SCENARIO_RECORD, freq);
                        }
                    }
                }
            } else {
                int freq = freqSetting.get<int>();
                if(freq > 0) {
                    setting.depthFreq.emplace(eSTEREO_SCENARIO_PREVIEW, freq);
                    setting.depthFreq.emplace(eSTEREO_SCENARIO_RECORD, freq);
                }
            }
        }
    }

    // Get customized frame sync type
    if(LogicalCamJSONUtil::HasMember(settingJson, CUSTOM_KEY_FRAME_SYNC)) {
        const _JSON_TYPE &frameSyncSetting = settingJson[CUSTOM_KEY_FRAME_SYNC];
        if(frameSyncSetting.is_string()) {
            auto val = frameSyncSetting.get<string>();
            std::transform(val.begin(), val.end(), val.begin(), ::tolower);
            if(val.find("vsync") == 0) {
                setting.frameSyncType = E_VSYNC_ALIGN;
            } else if(val.find("roc") == 0 ||
                      val.find("read out center") == 0) {
                setting.frameSyncType = E_READ_OUT_CENTER_ALIGN;
            }
        } else {
            setting.frameSyncType = frameSyncSetting.get<int>();
            if(setting.frameSyncType < 0 ||
               setting.frameSyncType >= E_FRAME_SYNC_MAX)
            {
                setting.frameSyncType = E_FRAME_SYNC_DEFAULT;
            }
        }
    }
}

StereoSettingProviderKernel *
StereoSettingProviderKernel::getInstance()
{
    Mutex::Autolock lock(__instanceLock);

    if(NULL == __instance) {
        __instance = new StereoSettingProviderKernel();
    }

    return __instance;
}

void
StereoSettingProviderKernel::destroyInstance()
{
    Mutex::Autolock lock(__instanceLock);

    if(__instance) {
        delete __instance;
        __instance = NULL;
    }
}

StereoSensorSetting_T *
StereoSettingProviderKernel::getSensorSetting(int sensorIndex)
{
    if(0 == __sensorSettings.size()) {
        init();
    }

    for(auto &s : __sensorSettings) {
        if(s.second.index == sensorIndex) {
            return &(s.second);
        }
    }

    MY_LOGD("Cannot get sensor setting of sensor %d", sensorIndex);
    return NULL;
}

StereoSensorConbinationSetting_T *
StereoSettingProviderKernel::getSensorCombinationSetting(MUINT32 logicalDeviceID)
{
    if(0 == __sensorCombinationSettings.size()) {
        init();
    }

    auto iter = __sensorCombinationSettings.find(logicalDeviceID);
    if(iter != __sensorCombinationSettings.end()) {
        return &(iter->second);
    }

    MY_LOGD("Sensor combination not found for logical device ID: %d", logicalDeviceID);
    return NULL;
}

StereoSettingProviderKernel::StereoSettingProviderKernel()
    : __logger(LOG_TAG, PROPERTY_SETTING_LOG)
{
    __logger.setSingleLineMode(0);
}

StereoSettingProviderKernel::~StereoSettingProviderKernel()
{
    __reset();
}

void
StereoSettingProviderKernel::init()
{
    AutoProfileUtil profile(LOG_TAG, "init");
    Mutex::Autolock lock(__instanceLock);

    if(__sensorCount == 0)
    {
        IHalSensorList *pSensorList = MAKE_HalSensorList();
        if (NULL == pSensorList) {
            MY_LOGE("Cannot get sensor list");
            return;
        }

        __sensorCount = pSensorList->queryNumberOfSensors();
        for(size_t index = 0; index < __sensorCount; ++index)
        {
            auto pName = pSensorList->queryDriverName(index);
            std::string s = (pName)?pName:"";
            size_t prefixPos = s.find(SENSOR_DRVNAME_PREFIX);
            if(prefixPos != std::string::npos) {
                s.erase(prefixPos, SENSOR_DRVNAME_PREFIX.length());
            }

            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            auto iter = __sensorNameMap.find(s);
            if(iter == __sensorNameMap.end())
            {
                std::vector<int32_t> v;
                v.push_back(index);
                __sensorNameMap.emplace(s, v);
            }
            else
            {
                iter->second.push_back(index);
            }
        }
    }

    // We'll Parse and save _JSON_TYPE in parsed order for saving to files
    // However, parsing in order will be 2~3 times slower, we only do it when customization is enabled
    __IS_CUSTOM_SETTING_ENABLED = (1 == checkStereoProperty(PROPERTY_ENABLE_CUSTOM_SETTING));
    __IS_EXPORT_ENABLED         = (1 == checkStereoProperty(PROPERTY_EXPORT_SETTING));

    _JSON_TYPE loadedJson;
    int status = 0;
    if(!__IS_CUSTOM_SETTING_ENABLED && !__IS_EXPORT_ENABLED) {
        AutoProfileUtil profile(LOG_TAG, "Parse logical cam setting");
        status = LogicalCamJSONUtil::parseLogicalCamCustomSetting(loadedJson);
    } else {
        KeepOrderJSON readJson;
        {
            AutoProfileUtil profile(LOG_TAG, "Parse logical cam setting");
            status = LogicalCamJSONUtil::parseLogicalCamCustomSetting(readJson);
        }

        if(__IS_EXPORT_ENABLED ||
           IS_JSON_FROM_DEFAULT(status))
        {
            __saveSettingToFile(readJson);
        }
        loadedJson = std::move(readJson);
    }

    if(status)
    {
        __reset();

        __parseDocument(loadedJson);
        // __loadSettingsFromCalibration();
    }

    for(auto &item : __sensorSettings)
    {
        __updateFOV(item.second);
        // Reset crop setting
        item.second.cropSettingMap.clear();
    }

    logSettings();
}

std::vector<float>
StereoSettingProviderKernel::getLensPoseTranslation(int32_t sensorId)
{
    std::vector<float> result(3, 0.0f);
    auto sensorSetting = getSensorSetting(sensorId);
    if(sensorSetting)
    {
        result = sensorSetting->lensPoseTranslation;
    }

    return result;
}

bool
StereoSettingProviderKernel::isMulticamRecordEnabled(int logicalDeviceID)
{
    auto sc = getSensorCombinationSetting(logicalDeviceID);
    return (sc && sc->isRecordEnabled);
}

EXCLUDEMAP_T
StereoSettingProviderKernel::getExclusiveGroup(int logicalDeviceID)
{
    EXCLUDEMAP_T result;
    auto sc = getSensorCombinationSetting(logicalDeviceID);
    if(sc)
    {
        for(int groupId = 0; groupId < sc->exclusiveGroups.size(); ++groupId) {
            for(auto &sensor : sc->exclusiveGroups[groupId]) {
                result.emplace(sensor->index, groupId);
            }
        }
    }
    else
    {
        auto pHalDeviceList = MAKE_HalLogicalDeviceList();
        if(pHalDeviceList) {
            auto sensorIDs = pHalDeviceList->getSensorId(logicalDeviceID);
            for(auto id : sensorIDs) {
                result.emplace(id, id);
            }
        }
    }

    return result;
}

int
StereoSettingProviderKernel::getISProfile(int sensorId, ENUM_STEREO_SCENARIO scenario, int logicalDeviceID)
{
    int ispProfile = -1;
    auto sc = getSensorCombinationSetting(logicalDeviceID);
    if(sc)
    {
        auto it = sc->ispProfiles.find(scenario);
        if(it != sc->ispProfiles.end()) {
            for(size_t i = 0; i < sc->sensorSettings.size(); ++i) {
                if(sensorId == sc->sensorSettings[i]->index) {
                    ispProfile = it->second[i];
                    break;
                }
            }
        }
        else
        {
            MY_LOGD_IF(LOG_ENABLED, "ISP Profile not set for sensor %d, scenario %d", sensorId, scenario);
        }
    }

    return ispProfile;
}

bool
StereoSettingProviderKernel::
setSensorPhysicalInfo(int logicalDeviceID, SENSOR_PHYSICAL_INFO_T &info)
{
    MY_LOGD("Set sensor physical info of sensor %d: orientation %d, focal length %f mm, physical size %f x %f mm",
            logicalDeviceID, info.orientation, info.focalLength, info.physicalWidth, info.physicalHeight);
    __physicalInfoMap.emplace(logicalDeviceID, info);

    return true;
}

bool
StereoSettingProviderKernel::
getSensorPhysicalInfo(int logicalDeviceID, SENSOR_PHYSICAL_INFO_T &info) const
{
    auto iter = __physicalInfoMap.find(logicalDeviceID);
    if(iter == __physicalInfoMap.end())
    {
        MY_LOGD("Physical information of sensor %d is not found", logicalDeviceID);
        return false;
    }

    info = iter->second;
    return true;
}

int
StereoSettingProviderKernel::getDepthDelay(int logicalDeviceID)
{
    auto sc = getSensorCombinationSetting(logicalDeviceID);
    if(sc && sc->depthDelay >= 0)
    {
        return sc->depthDelay;
    }

    if(__depthDelay > 0)
    {
        return __depthDelay;
    }

    return 0;
}

size_t
StereoSettingProviderKernel::getDepthFreq(ENUM_STEREO_SCENARIO scenario,
                                          MUINT32 logicalDeviceID)
{
    auto sc = getSensorCombinationSetting(logicalDeviceID);
    if(sc) {
        auto it = sc->depthFreq.find(scenario);
        if(it != sc->depthFreq.end()) {
            return it->second;
        }
    }

    auto it = __depthFreq.find(scenario);
    if(it != __depthFreq.end()) {
        return it->second;
    }

    return 0;
}

void
StereoSettingProviderKernel::__saveSettingToFile(KeepOrderJSON &jsonObj)
{
    AutoProfileUtil profile(LOG_TAG, "saveSettingToFile");
    //Save to json
    std::string s = jsonObj.dump(4);   //set indent of space
    char *data = (char *)s.c_str();

    // Only save to JSON when it does not exist
    if(__IS_CUSTOM_SETTING_ENABLED)
    {
        struct stat st;
        bool isSdcardFileExist = (stat(STEREO_SETTING_FILE_SDCARD_PATH, &st) == 0);
        bool isVendorFileExist = (stat(STEREO_SETTING_FILE_VENDOR_PATH, &st) == 0);
        if(!isSdcardFileExist &&
           !isVendorFileExist)
        {
            FILE *fp = fopen(STEREO_SETTING_FILE_SDCARD_PATH, "w");
            if(fp) {
                MY_LOGE_IF(fwrite(data, 1, strlen(data), fp) == 0, "Write failed");

                MY_LOGE_IF(fflush(fp), "Flush failed");
                MY_LOGE_IF(fclose(fp), "Close failed");
            }
            else
            {
                fp = fopen(STEREO_SETTING_FILE_VENDOR_PATH, "w");
                if(fp) {
                    MY_LOGE_IF(fwrite(data, 1, strlen(data), fp) == 0, "Write failed");

                    MY_LOGE_IF(fflush(fp), "Flush failed");
                    MY_LOGE_IF(fclose(fp), "Close failed");
                }
            }
        }
    }

    if(__IS_EXPORT_ENABLED) {
        FILE *fp = fopen(SETTING_HEADER_SDCARD_PATH, "w");
        if(NULL == fp)
        {
            fp = fopen(SETTING_HEADER_VENDOR_PATH, "w");
        }

        if(fp) {
            //Write copy right
            const char *COPY_RIGHT =
            "/* Copyright Statement:\n"
            " *\n"
            " * This software/firmware and related documentation (\"MediaTek Software\") are\n"
            " * protected under relevant copyright laws. The information contained herein\n"
            " * is confidential and proprietary to MediaTek Inc. and/or its licensors.\n"
            " * Without the prior written permission of MediaTek inc. and/or its licensors,\n"
            " * any reproduction, modification, use or disclosure of MediaTek Software,\n"
            " * and information contained herein, in whole or in part, shall be strictly prohibited.\n"
            " */\n"
            "/* MediaTek Inc. (C) 2019. All rights reserved.\n"
            " *\n"
            " * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES\n"
            " * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS (\"MEDIATEK SOFTWARE\")\n"
            " * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON\n"
            " * AN \"AS-IS\" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,\n"
            " * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF\n"
            " * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.\n"
            " * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE\n"
            " * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR\n"
            " * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH\n"
            " * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES\n"
            " * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES\n"
            " * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK\n"
            " * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR\n"
            " * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND\n"
            " * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,\n"
            " * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,\n"
            " * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO\n"
            " * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.\n"
            " *\n"
            " * The following software/firmware and/or related documentation (\"MediaTek Software\")\n"
            " * have been modified by MediaTek Inc. All revisions are subject to any receiver's\n"
            " * applicable license agreements with MediaTek Inc.\n"
            " */\n\n"
            "/********************************************************************************************\n"
            " *     LEGAL DISCLAIMER\n"
            " *\n"
            " *     (Header of MediaTek Software/Firmware Release or Documentation)\n"
            " *\n"
            " *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES\n"
            " *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS (\"MEDIATEK SOFTWARE\") RECEIVED\n"
            " *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN \"AS-IS\" BASIS\n"
            " *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,\n"
            " *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR\n"
            " *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY\n"
            " *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,\n"
            " *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK\n"
            " *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO\n"
            " *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION\n"
            " *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.\n"
            " *\n"
            " *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH\n"
            " *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,\n"
            " *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE\n"
            " *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.\n"
            " *\n"
            " *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS\n"
            " *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.\n"
            " ************************************************************************************************/\n"
            "#ifndef CAMERA_CUSTOM_STEREO_SETTING_H_\n"
            "#define CAMERA_CUSTOM_STEREO_SETTING_H_\n"
            "const char *DEFAULT_STEREO_SETTING =\n";
            MY_LOGE_IF(fwrite(COPY_RIGHT, 1, strlen(COPY_RIGHT), fp) == 0, "Write failed");

            //Write JSON line-by-line
            char lineBuffer[1024];
            char *line = strtok(data, "\n");
            int lineSize;
            int i, j;
            bool isPreSpace = true;
            while(line) {
                lineSize = strlen(line);
                isPreSpace = true;
                // lineBuffer[0] = '\"';
                for(i = 0, j = 0; i < lineSize; ++i, ++j) {
                    if(isPreSpace &&
                       !isspace(line[i]))
                    {
                        isPreSpace = false;
                        lineBuffer[j++] = '"';
                    }

                    if(line[i] == '\"') {
                        lineBuffer[j++] = '\\';
                    }

                    lineBuffer[j] = line[i];
                }
                line  = strtok(NULL, "\n");
                lineBuffer[j++] = '\"';

                if(NULL == line) {
                    lineBuffer[j++] = ';';
                }
                lineBuffer[j++] = '\n';

                MY_LOGE_IF(fwrite(lineBuffer, 1, j, fp) == 0, "Write failed");
            }

            const char *LAST_LINE = "#endif\n";
            size_t LAST_LINE_SIZE = strlen(LAST_LINE);
            ::memcpy(lineBuffer, LAST_LINE, LAST_LINE_SIZE);
            MY_LOGE_IF(fwrite(lineBuffer, 1, LAST_LINE_SIZE, fp) == 0, "Write failed");

            MY_LOGE_IF(fflush(fp), "Flush failed");
            MY_LOGE_IF(fclose(fp), "Close failed");
        }
    }
}

void
StereoSettingProviderKernel::__parseDocument(_JSON_TYPE &jsonObj)
{
    AutoProfileUtil profile(LOG_TAG, "  Parse docuemnt");

    __parseGlobalSettings(jsonObj);

    const _JSON_TYPE& sensorValues = jsonObj[CUSTOM_KEY_SENSORS];
    __parseSensorSettings(sensorValues);
    __loadSettingsFromSensorHAL();

    // Get settings of sensor combinations
    const _JSON_TYPE& combinationValues = jsonObj[CUSTOM_KEY_SENSOR_COMBINATIONS];
    __parseCombinationSettings(combinationValues);
}

void
StereoSettingProviderKernel::__parseGlobalSettings(const _JSON_TYPE &jsonObj)
{
    // Get platform record setting
    __isPlatformRecordEnabled = false;
    if(LogicalCamJSONUtil::HasMember(jsonObj, CUSTOM_KEY_ENABLE_RECORD)) {
        __isPlatformRecordEnabled = jsonObj[CUSTOM_KEY_ENABLE_RECORD].get<int>();
    }

    //Get callback buffer list(optional)
    if(LogicalCamJSONUtil::HasMember(jsonObj, CUSTOM_KEY_CALLBACK_BUFFER_LIST)) {
        const _JSON_TYPE &callbackSetting = jsonObj[CUSTOM_KEY_CALLBACK_BUFFER_LIST];
        if(LogicalCamJSONUtil::HasMember(callbackSetting, CUSTOM_KEY_VALUE)) {
            __callbackBufferListString = callbackSetting[CUSTOM_KEY_VALUE].get<std::string>();
        }
    }

    //Get depthmap format(optional, default is Y8)
    if(LogicalCamJSONUtil::HasMember(jsonObj, CUSTOM_KEY_DEPTHMAP_FORMAT)) {
        std::string formatStr = jsonObj[CUSTOM_KEY_DEPTHMAP_FORMAT].get<std::string>();
        if(formatStr == "Y16" ||
           formatStr == "y16")
        {
            __depthmapFormat = NSCam::eImgFmt_Y16;
        }
    }

    //Get Main2 output frequency
    if(LogicalCamJSONUtil::HasMember(jsonObj, CUSTOM_KEY_MAIN2_OUTPUT_FREQ)) {
        const _JSON_TYPE &main2FreqSetting = jsonObj[CUSTOM_KEY_MAIN2_OUTPUT_FREQ];
        for(auto &s : main2FreqSetting.items()) {
            std::string key = s.key();
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            if(key.find("pre") == 0) {
                __main2OutputFrequency[0] = s.value().get<int>();
            } else if(key.find("cap") == 0) {
                __main2OutputFrequency[1] = s.value().get<int>();
            }
        }
    }

    //Get depth delay setting
    if(LogicalCamJSONUtil::HasMember(jsonObj, CUSTOM_KEY_DEPTH_DELAY)) {
        __depthDelay = jsonObj[CUSTOM_KEY_DEPTH_DELAY].get<int>();
    }

    //Get depth freq setting
    if(LogicalCamJSONUtil::HasMember(jsonObj, CUSTOM_KEY_DEPTH_FREQ)) {
        const _JSON_TYPE &freqSetting = jsonObj[CUSTOM_KEY_DEPTH_FREQ];
        if(freqSetting.type() == _JSON_TYPE::value_t::object) {
            for(auto &s : freqSetting.items()) {
                std::string key = s.key();
                int freq = s.value().get<int>();
                if(freq > 0) {
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    if(key.find("pre") == 0) {
                        __depthFreq.emplace(eSTEREO_SCENARIO_PREVIEW, freq);
                    } else if(key.find("rec") == 0 ||
                              key.find("vid") == 0) {
                        __depthFreq.emplace(eSTEREO_SCENARIO_RECORD, freq);
                    }
                }
            }
        } else {
            int freq = freqSetting.get<int>();
            if(freq > 0) {
                __depthFreq.emplace(eSTEREO_SCENARIO_PREVIEW, freq);
                __depthFreq.emplace(eSTEREO_SCENARIO_RECORD, freq);
            }
        }
    }
}

void
StereoSettingProviderKernel::__parseSensorSettings(const _JSON_TYPE &sensorValues)
{
    std::string name;
    for(auto &settingValue : sensorValues) {
        //Check if the sensor is installed
        StereoSensorSetting_T setting = settingValue;
        name = setting.name;
        auto iter = __sensorNameMap.find(name);
        if(iter == __sensorNameMap.end()) {
            MY_LOGD_IF(LOG_ENABLED, "[Parse Sensor Setting]Sensor %s is not installed",
                       name.c_str());
            continue;
        }
        auto &nameIds = iter->second;

        auto tryToAddSetting = [&](string &name, StereoSensorSetting_T &setting)
        {
            // For printing
            #ifdef __func__
            #undef __func__
            #endif
            #define __func__ "__parseSensorSettings"

            auto settingIt = __sensorSettings.find(name);
            if(settingIt == __sensorSettings.end())
            {
                __sensorSettings.emplace(name, std::move(setting));
                MY_LOGD_IF(LOG_ENABLED, "[Parse Sensor Setting]Sensor %s(%d) is installed", setting.name.c_str(), setting.index);
            }
            else
            {
                MY_LOGE("[Parse Sensor Setting]Sensor setting of %s(%d) exists, there is duplcated settings\n"
                        "If you want to sepecify sensor IDs for duplicated sensors, you'll need to sepcify for all of them",
                        setting.name.c_str(), setting.index);
            }

            #undef __func__
        };

        //Use from_json(const _JSON_TYPE &settingJson, StereoSensorSetting_T& setting)
        if(setting.index >= 0)
        {
            //Check if specified sensor ID is correct
            auto idIter = std::find(nameIds.begin(), nameIds.end(), setting.index);
            if(idIter == nameIds.end())
            {
                std::ostringstream oss;
                oss << " Need to correct sensor id for sensor setting: \""
                    << name << DELIMITER << setting.index << "\"" << endl
                    << " Available settings:" << endl;
                for(auto &id : nameIds) {
                    oss << "    \"" << name << DELIMITER << id << "\"" << endl;
                }
                MY_LOGE("%s", oss.str().c_str());
                continue;
            }

            // User should config lens post translation like this:
            // "Lens Pose Translation": [-0.05, 0, 0] and must not be list of array
            if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_LENS_POSE_TRANSLATION))
            {
                const _JSON_TYPE &config = settingValue[CUSTOM_KEY_LENS_POSE_TRANSLATION];
                if(config.size() > 0)
                {
                    if(config[0].type() != _JSON_TYPE::value_t::array)
                    {
                        //Config by "Lens Pose Translation": [-0.05, 0, 0],
                        setting.lensPoseTranslation = config.get<std::vector<float>>();
                    }
                    else
                    {
                        MY_LOGE("Please check \"%s\" for %s, the list must be 3 numbers",
                                CUSTOM_KEY_LENS_POSE_TRANSLATION, name.c_str());
                    }
                }
            }

            // If there are duplicated sensors, we'll append sensor index to senso name for key
            name = setting.name + DELIMITER + std::to_string(setting.index);
            tryToAddSetting(name, setting);
        }
        else
        {
            //Get lens pose translation(optional)
            //If not set, we'll update later when logical device is parsed
            // User should config lens post translation like this:
            // "Lens Pose Translation": [[-1.05, 0, 0], [-2.0, 0, 0]] and must be list of array
            // And the list size must match duplicated sensor size
            std::vector<std::vector<float>> lensPoseTranslations;
            if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_LENS_POSE_TRANSLATION))
            {
                const _JSON_TYPE &config = settingValue[CUSTOM_KEY_LENS_POSE_TRANSLATION];
                if(config.size() > 0)
                {
                    if(config.size() == nameIds.size() &&
                       config[0].type() == _JSON_TYPE::value_t::array)
                    {
                        //Config by "Lens Pose Translation": [[-1.05, 0, 0], [-2.0, 0, 0]], for duplicated sensor
                        for(auto &n : config)
                        {
                            lensPoseTranslations.push_back(n.get<std::vector<float>>());
                        }
                    }
                    else
                    {
                        MY_LOGE("Please check \"%s\" for %s, the list size must be %zu and each element must be array with 3 elements",
                                CUSTOM_KEY_LENS_POSE_TRANSLATION, name.c_str(), nameIds.size());
                    }
                }
            }

            int idx = 0;
            for(auto &id : nameIds)
            {
                setting.index = id;
                if(lensPoseTranslations.size() > 0)
                {
                    setting.lensPoseTranslation = lensPoseTranslations[idx];
                    ++idx;
                }

                name = setting.name + DELIMITER + std::to_string(setting.index);
                tryToAddSetting(name, setting);
            }
        }
    }
}

void
StereoSettingProviderKernel::__parseCombinationSettings(const _JSON_TYPE &combinationValues)
{
    //Get settings of sensor combinations
    int logicalDeviceID = __sensorCount;
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(!pHalDeviceList) {
        MY_LOGE("Cannot get logical device list");
        return;
    }

    const int DEVICE_COUNT = pHalDeviceList->queryNumberOfDevices();
    std::unordered_map<std::string, int> nameToIDMap;
    for(int idx = __sensorCount; idx < DEVICE_COUNT; ++idx)
    {
        auto pName = pHalDeviceList->queryDriverName(idx);
        std::string name = (pName)?pName:"";
        // Remove prefix
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
        size_t prefixPos = name.find(SENSOR_DRVNAME_PREFIX);
        if(prefixPos != std::string::npos) {
            name.erase(prefixPos, SENSOR_DRVNAME_PREFIX.length());
        }

        nameToIDMap.emplace(name, idx);
    }

    for(auto &settingValue : combinationValues)
    {
        std::vector<StereoSensorSetting_T *> sensorSettings;
        //Check if the sensors are installed
        bool hasError = false;
        std::vector<std::string> namesInSetting;
        if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_SENSORS))
        {
            //Use "Sensors"
            const _JSON_TYPE &sensorsSetting = settingValue[CUSTOM_KEY_SENSORS];
            for(const auto &sensorSetting : sensorsSetting) {
                if(sensorSetting.is_array()) {
                    // Check if any of the sensor has been installed in the list
                    bool hasSensorInstalled = false;
                    for(const auto &nameSetting : sensorSetting) {
                        auto name = nameSetting.get<std::string>();
                        __parseAndRefineSensorName(name);
                        auto nameIt = __sensorNameMap.find(name);
                        if(nameIt != __sensorNameMap.end()) {
                            hasSensorInstalled = true;
                            namesInSetting.push_back(nameSetting.get<std::string>());
                            break;
                        }
                    }

                    if(!hasSensorInstalled) {
                        hasError = true;
                        break;
                    }
                } else if(sensorSetting.is_string()) {
                    namesInSetting.push_back(sensorSetting.get<std::string>());
                } else {
                    hasError = true;
                    break;
                }
            }

            hasError = hasError ||
                       !__validateSensorSetting(namesInSetting, sensorSettings);

            if (hasError) {
                continue;
            }
        }
        else
        {
            MY_LOGE("No sensor is defined");
            continue;
        }

        StereoSensorConbinationSetting_T setting;

        //Use from_json(const _JSON_TYPE &settingJson, StereoSensorConbinationSetting_T& setting)
        setting = settingValue;

        //Check if sensor combination is in logical device list, it may be hidden
        std::string logicalDeviceName = sensorSettings[0]->name + "_" + setting.logicalDeviceName;
        std::transform(logicalDeviceName.begin(), logicalDeviceName.end(), logicalDeviceName.begin(), ::toupper);
        auto nameIt = nameToIDMap.find(logicalDeviceName);
        if(nameIt == nameToIDMap.end())
        {
            MY_LOGD("Skip %s", logicalDeviceName.c_str());
            continue;
        }
        logicalDeviceID = nameIt->second;

        if(setting.moduleType == 0 &&
           sensorSettings.size() == 2 &&
           (    (setting.logicalDeviceFeatureSet & E_STEREO_FEATURE_CAPTURE)
             || (setting.logicalDeviceFeatureSet & E_STEREO_FEATURE_VSDOF)
             || (setting.logicalDeviceFeatureSet & E_STEREO_FEATURE_THIRD_PARTY)
             || (setting.logicalDeviceFeatureSet & E_STEREO_FEATURE_MTK_DEPTHMAP)
           ))
        {
            MY_LOGE("Module type must be set for stereo features");
        }

        // Automatically fill lens pose translation if possible
        if(sensorSettings.size() == 1)
        {
            if(sensorSettings[0]->lensPoseTranslation.size() == 0) {
                sensorSettings[0]->lensPoseTranslation.resize(3, 0.0f);
            }
        }
        else if(sensorSettings.size() != 2) //For sensors > 2, we only check if the data is filled
        {
            for(auto &sensorSetting: sensorSettings)
            {
                if(sensorSetting->lensPoseTranslation.size() == 0)
                {
                    MY_LOGE("%s of sensor %s(%d) is not set in multicam setting, please add for it, sample:\n"
                            "  \"%s\": [-0.05, 0, 0] (If a sensor ID is specified) or\n"
                            "  \"%s\": [[-0.05, 0, 0], [-1.05, 0, 0]] (For duplicated sensors share the same sensor setting)\n"
                            " Please fill the values in Google format, you can refer to:\n"
                            " https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics.html#LENS_POSE_TRANSLATION",
                            CUSTOM_KEY_LENS_POSE_TRANSLATION,
                            sensorSetting->name.c_str(), sensorSetting->index,
                            CUSTOM_KEY_LENS_POSE_TRANSLATION, CUSTOM_KEY_LENS_POSE_TRANSLATION);
                }
            }
        }
        else if(setting.moduleType > 0 &&
                setting.baseline > 0.0f)
        {
            auto minSensorId = std::min(sensorSettings[0]->index, sensorSettings[1]->index);
            for(auto &sensorSetting: sensorSettings)
            {
                if(sensorSetting->lensPoseTranslation.size() == LENS_POSE_TRANSLATION_SIZE)
                {
                    continue;
                }

                if(sensorSetting->index == minSensorId)
                {
                    sensorSetting->lensPoseTranslation.resize(3, 0.0f);
                }
                else if(setting.sensorOffset.size() > 1)
                {
                    float x = -setting.sensorOffset[1].first/100.0f;
                    float y = setting.sensorOffset[1].second/100.0f;
                    sensorSetting->lensPoseTranslation = {x, y, 0};
                }
                else
                {
                    float baseline = setting.baseline/100.0f;  //in meter
                    int   position = [&] {
                        if(1 == sensorSettings[0]->staticInfo.facingDirection) {
                            if(3 == setting.moduleType)
                            {
                                return 1;
                            }
                        } else {
                            if(3 == setting.moduleType ||
                               4 == setting.moduleType)
                            {
                                return 1;
                            }
                        }

                        return 0;
                    }();

                    if(1 == position ||
                       sensorSetting->index != sensorSettings[0]->index) // Main1 of the logical device is not main cam
                    {
                        baseline = -baseline;
                    }

                    int rotation = [&] {
                        if(1 == setting.moduleType ||
                           3 == setting.moduleType)
                        {
                            if(0 == sensorSetting->staticInfo.facingDirection) {
                                return 90;
                            } else {
                                return 270;
                            }
                        }

                        return 0;
                    }();

                    switch(rotation)
                    {
                        case 0:
                        default:
                            sensorSetting->lensPoseTranslation = {baseline, 0, 0};
                            break;
                        case 90:
                            sensorSetting->lensPoseTranslation = {0, -baseline, 0};
                            break;
                        case 270:
                            // Front cam
                            sensorSetting->lensPoseTranslation = {0, baseline, 0};
                            break;
                    }
                }

                MY_LOGD("%s of sensor %s(%d) init to [%.5f %.5f %.5f]",
                        CUSTOM_KEY_LENS_POSE_TRANSLATION,
                        sensorSetting->name.c_str(), sensorSetting->index,
                        sensorSetting->lensPoseTranslation[0],
                        sensorSetting->lensPoseTranslation[1],
                        sensorSetting->lensPoseTranslation[2]);
            }
        }

        setting.sensorSettings  = std::move(sensorSettings);
        setting.logicalDeviceID = logicalDeviceID;

        // Set module type if not configured
        if(setting.moduleRotation == -1) {
            if(1 == setting.moduleType ||
               3 == setting.moduleType)
            {
                if(1 == setting.sensorSettings[0]->staticInfo.facingDirection) {
                    setting.moduleRotation = setting.sensorSettings[0]->staticInfo.orientationAngle;
                } else {
                    setting.moduleRotation = 90;
                }
            } else {
                setting.moduleRotation = 0;
            }
        }

        setting.moduleRotation = std::fmod(setting.moduleRotation, 360.0f);
        if(setting.moduleRotation < 0) {
            setting.moduleRotation += 360.0f;
        }

        //Parse overwrite sensor scenario
        if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_SENSOR_SCENARIO_ZSD))
        {
            const _JSON_TYPE &config = settingValue[CUSTOM_KEY_SENSOR_SCENARIO_ZSD];
            if(config.size() == setting.sensorSettings.size())
            {
                for(_JSON_TYPE::const_iterator it = config.begin(); it != config.end(); ++it)
                {
                    setting.sensorScenariosZSD.push_back(stringToSensorScenario(it.value().get<std::string>()));
                }
            }
            else
            {
                MY_LOGE("ZSD sensor scenario size(%zu) != sensor size(%zu), ignore setting", config.size(), setting.sensorSettings.size());
            }
        }

        if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_SENSOR_SCENARIO_REC))
        {
            const _JSON_TYPE &config = settingValue[CUSTOM_KEY_SENSOR_SCENARIO_REC];
            if(config.size() == setting.sensorSettings.size())
            {
                for(_JSON_TYPE::const_iterator it = config.begin(); it != config.end(); ++it)
                {
                    setting.sensorScenariosRecord.push_back(stringToSensorScenario(it.value().get<std::string>()));
                }
            }
            else
            {
                MY_LOGE("Record sensor scenario size(%zu) != sensor size(%zu), ignore setting", config.size(), setting.sensorSettings.size());
            }
        }

        //Get ISP Profiles
        if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_ISP_PROFILE))
        {
            const _JSON_TYPE &config = settingValue[CUSTOM_KEY_ISP_PROFILE];

            std::unordered_map<std::string, int> profileMap =
            {
                { "EIspProfile_N3D_Preview",          NSIspTuning::EIspProfile_N3D_Preview          },

#if (1==HAS_P1YUV)
                { "EIspProfile_P1_YUV_Depth",         NSIspTuning::EIspProfile_P1_YUV_Depth         },
#endif

#if (2==HAS_P1YUV || 6 ==HAS_P1YUV)
                { "EIspProfile_VSDOF_PV_Main_toW",    NSIspTuning::EIspProfile_VSDOF_PV_Main_toW    },
                { "EIspProfile_VSDOF_PV_Aux",         NSIspTuning::EIspProfile_VSDOF_PV_Aux         },
                { "EIspProfile_VSDOF_PV_Main",        NSIspTuning::EIspProfile_VSDOF_PV_Main        },
                { "EIspProfile_VSDOF_Video_Main_toW", NSIspTuning::EIspProfile_VSDOF_Video_Main_toW },
                { "EIspProfile_VSDOF_Video_Aux",      NSIspTuning::EIspProfile_VSDOF_Video_Aux      },
                { "EIspProfile_VSDOF_Video_Main",     NSIspTuning::EIspProfile_VSDOF_Video_Main     },
#endif
            };

            auto getISPProfiles = [&](std::vector<std::string> &names) -> std::vector<int>
            {
                std::vector<int> profiles;
                for(auto &n : names) {
                    auto it = profileMap.find(n);
                    if(it != profileMap.end())
                    {
                        profiles.push_back(it->second);
                    }
                    else
                    {
                        MY_LOGE("Unsupported ISP Profile: %s", n.c_str());
                        break;
                    }
                }

                return profiles;
            };

            if(config.type() == _JSON_TYPE::value_t::array)
            {
                //All scenarios shares the same ISP profile
                auto profileNames = config.get<std::vector<std::string>>();
                std::vector<int> profiles = getISPProfiles(profileNames);
                if(profiles.size() == setting.sensorSettings.size())
                {
                    setting.ispProfiles.emplace(eSTEREO_SCENARIO_PREVIEW, profiles);
                    setting.ispProfiles.emplace(eSTEREO_SCENARIO_RECORD,  profiles);
                    // setting.ispProfiles.emplace(eSTEREO_SCENARIO_CAPTURE, profiles);
                }
            }
            else
            {
                for(_JSON_TYPE::const_iterator it = config.begin(); it != config.end(); ++it)
                {
                    std::string scenarioName = it.key();
                    std::transform(scenarioName.begin(), scenarioName.end(), scenarioName.begin(), ::tolower);
                    ENUM_STEREO_SCENARIO scenario = eSTEREO_SCENARIO_UNKNOWN;
                    if(scenarioName == "preview")
                    {
                        scenario = eSTEREO_SCENARIO_PREVIEW;
                    }
                    else if(scenarioName == "record")
                    {
                        scenario = eSTEREO_SCENARIO_RECORD;
                    }
                    // else if(scenarioName == "capture")
                    // {
                    //     scenario = eSTEREO_SCENARIO_CAPTURE;
                    // }

                    if(eSTEREO_SCENARIO_UNKNOWN == scenario)
                    {
                        MY_LOGE("Incorrect scenario key: \"%s\"\n"
                                "Should be one of \"Preview\" or \"Record\"",  it.key().c_str());
                                // "Should be one of \"Preview\", \"Record\" or \"Capture\"",  it.key().c_str());
                        continue;
                    }

                    auto profileNames = it.value().get<std::vector<std::string>>();
                    std::vector<int> profiles = getISPProfiles(profileNames);
                    if(profiles.size() == setting.sensorSettings.size())
                    {
                        setting.ispProfiles.emplace(scenario, std::move(profiles));
                    }
                }
            }
        }

        //Update record setting
        // Record is enabled if
        // 1. Sensor scenario for recording is configured and (one of 2~4 below)
        // 2. Feature mode of the logical device is Zoom or
        // 3. "Enable Record" is set to 1 outside of sensor combination setting or
        // 4. "Enable Record" is set to 1 in the sensor combination setting
        setting.isRecordEnabled = [&]
        {
            bool areSensorScenariosSet = true;
            // Check sensor scanerio
            if(setting.sensorScenariosRecord.size() == 0)
            {
                for(auto &s : setting.sensorSettings)
                {
                    areSensorScenariosSet = areSensorScenariosSet && (s->sensorScenarioMapRecord.size() > 0);
                }
            }

            return ( // Sensor scenario for recording is configured
                     areSensorScenariosSet &&
                     // Feature mode of the logical device is Zoom
                     ( (setting.logicalDeviceFeatureSet & NSCam::v1::Stereo::E_DUALCAM_FEATURE_ZOOM) ||
                     // "Enable Record" is set to 1 outside of sensor combination setting
                       __isPlatformRecordEnabled ||
                     // "Enable Record" is set to 1 in the sensor combination setting
                       ( LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_ENABLE_RECORD) &&
                         settingValue[CUSTOM_KEY_ENABLE_RECORD].get<int>() )
                     )
                   );
        }();

        //Get exlusive group
        std::set<int32_t> groupedSensorIds;
        if(LogicalCamJSONUtil::HasMember(settingValue, CUSTOM_KEY_EXCLUSIVES))
        {
            auto nameGroups = settingValue[CUSTOM_KEY_EXCLUSIVES].get<std::vector<std::vector<std::string>>>();
            for(auto &group : nameGroups) {
                bool hasExclusiveError = false;
                std::vector<StereoSensorSetting_T *> groupSetting;
                for(auto &name : group) {
                    int index = 0;
                    for(auto &n : namesInSetting) {
                        if(name == n) {
                            if(groupedSensorIds.find(setting.sensorSettings[index]->index) == groupedSensorIds.end())
                            {
                                groupSetting.push_back(setting.sensorSettings[index]);
                                groupedSensorIds.insert(setting.sensorSettings[index]->index);
                            }
                            else
                            {
                                hasExclusiveError = true;
                                MY_LOGW("Sensor %s(%d) belongs to another exlusive group, please correct settings",
                                        setting.sensorSettings[index]->name.c_str(), setting.sensorSettings[index]->index);
                            }
                            break;
                        }
                        index++;
                    }

                    if(hasExclusiveError) {
                        break;
                    }
                }

                if(!hasExclusiveError) {
                    setting.exclusiveGroups.push_back(groupSetting);
                }
            }
        }

        int settingSize = setting.sensorSettings.size();
        if(groupedSensorIds.size() < settingSize) {
            for(int index = 0; index < settingSize; ++index) {
                if(groupedSensorIds.find(setting.sensorSettings[index]->index) == groupedSensorIds.end())
                {
                    // std::vector<StereoSensorSetting_T *> groupSetting = {setting.sensorSettings[index]};
                    setting.exclusiveGroups.push_back({setting.sensorSettings[index]});
                }
            }
        }

        __sensorCombinationSettings[logicalDeviceID] = std::move(setting);
        logicalDeviceID++;
    }
}

bool
StereoSettingProviderKernel::__validateSensorSetting(
    const std::vector<std::string> &namesInSetting,
    std::vector<StereoSensorSetting_T *> &sensorSettings)
{
    //Preprocess names
    //We can support duplicated sensors in many ways
    //e.g. If there are two sensors with the same name imx456 and sensor IDs are 3 & 4,
    //     we can config in following ways:
    //     1. Specify all sensor id for every dup sensors
    //        "Sensors": ["IMX123_MIPI_RAW", "IMX456_MIPI_RAW:3", "IMX456_MIPI_RAW:4"]
    //     2. Specify sensor id for dup sensor used
    //        "Sensors": ["IMX123_MIPI_RAW", "IMX456_MIPI_RAW:3"]
    //     3. Only left one sensor ID not specified, need to put all dup sensors in the list
    //        "Sensors": ["IMX123_MIPI_RAW", "IMX456_MIPI_RAW", "IMX456_MIPI_RAW:4"]
    //        "Sensors": ["IMX123_MIPI_RAW", "IMX456_MIPI_RAW:3", "IMX456_MIPI_RAW"]
    //     4. Not specify sensor ID, need to put all dup sensors in the list,
    //        we'll assume you want to use them in order of sensor ID
    //        "Sensors": ["IMX123_MIPI_RAW", "IMX456_MIPI_RAW" "IMX456_MIPI_RAW"]

    std::unordered_map<string, vector<int32_t>> leftIdsMap;
    std::vector<int32_t> sensorIds(namesInSetting.size());
    std::vector<std::string> sensorNames(namesInSetting.size());
    std::unordered_map<string, size_t> nameCount;
    for(int settingIndex = 0; settingIndex < namesInSetting.size(); ++settingIndex)
    {
        auto name = namesInSetting[settingIndex];
        int customizedId = __parseAndRefineSensorName(name);
        sensorNames[settingIndex] = name;
        auto nameIt = __sensorNameMap.find(name);
        if(nameIt == __sensorNameMap.end())
        {
            return false;
        }
        else
        {
            auto leftIdIt = leftIdsMap.find(name);
            if(leftIdIt == leftIdsMap.end())
            {
                auto ret = leftIdsMap.emplace(name, nameIt->second);
                leftIdIt = ret.first;
            }

            {
                auto countIt = nameCount.find(name);
                if(countIt == nameCount.end())
                {
                    nameCount.emplace(name, 1);
                }
                else
                {
                    countIt->second++;
                }
            }

            auto &ids = nameIt->second;
            auto &leftIds = leftIdIt->second;
            if(ids.size() == 1)
            {
                if(leftIds.size() < 1)
                {
                    MY_LOGE("[Parse Sensor Combination]There are more than one \"%s\" in the setting, only one installed on the device",
                            name.c_str());
                    return false;
                }

                sensorIds[settingIndex] = ids[0];
                leftIds.clear();
            }
            else
            {
                if(customizedId < 0)
                {
                    // This is the first step checking, need to check later since the ID may be used later
                    auto idIt = leftIdsMap.find(name);
                    if(idIt != leftIdsMap.end()) {
                        if(idIt->second.size() == 1) {
                            sensorIds[settingIndex] = idIt->second[0];
                            idIt->second.clear();
                        } else if(idIt->second.size() == 0) {
                            MY_LOGE("[Parse Sensor Combination]There are more than one \"%s\" in the setting, please correct setting",
                                    namesInSetting[settingIndex].c_str());
                            return false;
                        } else {
                            sensorIds[settingIndex] = -1;
                        }
                    }
                }
                else
                {
                    // Sensor ID is specified, must be in installed list
                    if(leftIds.size() == 0)
                    {
                        MY_LOGE("[Parse Sensor Combination]Please remove duplicated sensor setting of \"%s\"",
                                namesInSetting[settingIndex].c_str());
                        return false;
                    }
                    else
                    {
                        auto cusIdIt = std::find(leftIds.begin(), leftIds.end(), customizedId);
                        if(cusIdIt != leftIds.end())
                        {
                            sensorIds[settingIndex] = customizedId;
                            leftIds.erase(cusIdIt);
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << "[Parse Sensor Combination] Need to correct sensor id for sensor setting: \""
                                << name << DELIMITER << customizedId << "\"" << endl
                                << " Available settings:" << endl;
                            for(auto &id : ids) {
                                oss << "    \"" << name << DELIMITER << id << "\"" << endl;
                            }
                            MY_LOGE("%s", oss.str().c_str());
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Scan if there are ID not given or other errors
    for(int idIndex = 0; idIndex < sensorIds.size(); ++idIndex)
    {
        auto &name = sensorNames[idIndex];
        if(sensorIds[idIndex] < 0)
        {
            auto idIt = leftIdsMap.find(name);
            if(idIt == leftIdsMap.end() ||
               idIt->second.size() == 0)
            {
                MY_LOGE("[Parse Sensor Combination]Please remove the \"%s\" without ID specified", sensorNames[idIndex].c_str());
                return false;
            }
            else if(idIt->second.size() == 1)
            {
                sensorIds[idIndex] = idIt->second[0];
                idIt->second.clear();
            }
            else
            {
                auto nameIt = __sensorNameMap.find(name);
                if(nameIt->second.size() == idIt->second.size() &&
                   nameCount[name] == nameIt->second.size())
                {
                    //Give sensor IDs to all dup sensor without id-specified in sensor index order
                    auto &ids = nameIt->second;
                    int idx = 1;
                    sensorIds[idIndex] = ids[0];
                    for(int dupIdx = idIndex+1; dupIdx < sensorIds.size(); ++dupIdx)
                    {
                        if(sensorNames[dupIdx] == name)
                        {
                            sensorIds[dupIdx] = ids[idx++];
                        }
                    }
                    idIt->second.clear();
                }
                else
                {
                    MY_LOGE("[Parse Sensor Combination]Need to correct setting for %s,\n"
                            "  Avaliable solutions:\n"
                            "  1. Specify sensor ID if only one of the duplicated sensors are used\n"
                            "  2. Remove all ID specified, we'll use them in sensor order\n"
                            "  3. Left only one without specified and specify for the others",
                            name.c_str());
                    return false;
                }
            }
        }
    }

    for(int s = 0; s < sensorIds.size(); ++s)
    {
        auto sensorId = sensorIds[s];
        auto name = sensorNames[s] + DELIMITER + std::to_string(sensorId);
        auto sensorSettingIter = __sensorSettings.find(name);
        if(sensorSettingIter != __sensorSettings.end())
        {
            sensorSettings.push_back(&(sensorSettingIter->second));
        }
        else
        {
            MY_LOGD_IF(LOG_ENABLED, "[Parse Sensor Combination]Sensor %s is not installed",
                                    sensorNames[s].c_str());
            return false;
        }
    }

    if(sensorSettings.size() != namesInSetting.size())
    {
        return false;
    }

    return true;
}

void
StereoSettingProviderKernel::__loadSettingsFromSensorHAL()
{
    AutoProfileUtil profile(LOG_TAG, "  Load settings from Sensor HAL");

    //Update sensor ID & FOV
    IHalSensorList *sensorList = MAKE_HalSensorList();
    if (NULL == sensorList) {
        MY_LOGE("Cannot get sensor list");
        return;
    }

    SensorStaticInfo sensorStaticInfo;
    for(auto &item : __sensorSettings) {
        auto &s = item.second;
        memset(&sensorStaticInfo, 0, sizeof(SensorStaticInfo));
        s.devIndex = sensorList->querySensorDevIdx(s.index);
        sensorList->querySensorStaticInfo(s.devIndex, &sensorStaticInfo);
        s.staticInfo = sensorStaticInfo;
    }
}

void
StereoSettingProviderKernel::__loadSettingsFromCalibration()
{
    AutoProfileUtil profile(LOG_TAG, "  Load calibration data");

    //Update calibration distance
    CAM_CAL_DATA_STRUCT calibrationData;
    CamCalDrvBase *pCamCalDrvObj = CamCalDrvBase::createInstance();
    MUINT32 queryResult;
    for(auto &item : __sensorSettings) {
        auto &s = item.second;
        if(0 == s.distanceMacro ||
           0 == s.distanceInfinite)
        {
            queryResult = pCamCalDrvObj->GetCamCalCalData(s.devIndex, CAMERA_CAM_CAL_DATA_3A_GAIN, (void *)&calibrationData);
            s.distanceMacro    = calibrationData.Single2A.S2aAF_t.AF_Macro_pattern_distance;    //unit: mm
            s.distanceInfinite = calibrationData.Single2A.S2aAF_t.AF_infinite_pattern_distance; //unit: mm
        }
    }
    pCamCalDrvObj->destroyInstance();
}

void
StereoSettingProviderKernel::__updateFOV(StereoSensorSetting_T &setting)
{
    // If FOV is not set, calculate by sensor physical information
    if(setting.fovDiagonal == 0.0f &&
       setting.fovHorizontal == 0.0f &&
       setting.fovVertical == 0.0f)
    {
        SENSOR_PHYSICAL_INFO_T info;
        if(getSensorPhysicalInfo(setting.index, info))
        {
            float physicalDiagonal = sqrt(info.physicalWidth*info.physicalWidth+info.physicalHeight*info.physicalHeight);
            setting.fovDiagonal = 2.0f*atan(physicalDiagonal/(2.0f*info.focalLength));
            MY_LOGD("Update FOV D:%.2f for sensor %d, physical size %.2fx%.2f mm, focalLength %f mm",
                     radiansToDegree(setting.fovDiagonal), setting.index, info.physicalWidth, info.physicalHeight, info.focalLength);
        }
    }

    // Update H/V FOV by diagonal FOV
    if(setting.fovDiagonal > 0.0f &&
       setting.fovHorizontal == 0.0f &&
       setting.fovVertical == 0.0f)
    {
        float w = setting.staticInfo.captureWidth;
        float h = setting.staticInfo.captureHeight;
        setting.fovVertical = 2.0f*atan(sqrt(pow(tan(setting.fovDiagonal/2.0f), 2)/(1+pow(w/h, 2))));
        setting.fovHorizontal = 2.0f*atan(w/h * tan(setting.fovVertical/2.0f));

        MY_LOGD("Update FOV H:%.2f/V:%.2f(D:%.2f) for sensor %d",
                radiansToDegree(setting.fovHorizontal),
                radiansToDegree(setting.fovVertical),
                radiansToDegree(setting.fovDiagonal),
                setting.index);
    }
}

void
StereoSettingProviderKernel::logSettings()
{
    __logger
    .updateLogStatus()
    .FastLogD("%s", "=======================")
    .FastLogD("%s", "    Sensor Settings")
    .FastLogD("%s", "=======================");

    size_t size = __sensorCount;
    for(auto &item : __sensorSettings) {
        item.second.log(__logger);
        if(size-- > 1) {
            __logger.FastLogD("%s", "----------------------------------------------");
        }
    }

    __logger
    .FastLogD("%s", "=======================")
    .FastLogD("%s", "  Sensor Combinations")
    .FastLogD("%s", "=======================");
    for(auto &item : __sensorCombinationSettings) {
        item.second.log(__logger);
        __logger .FastLogD("%s", "----------------------------------------------");
    }

    if(__depthDelay >= 0)
    {
        __logger
        .FastLogD("%s(Global):  %s", CUSTOM_KEY_DEPTH_DELAY, (__depthDelay>0)?"Enable":"Disable");
    }

    if(__depthFreq.size() > 0)
    {
        __logger.FastLogD("%s(Global):", CUSTOM_KEY_DEPTH_FREQ);
        auto it = __depthFreq.find(eSTEREO_SCENARIO_PREVIEW);
        if(it != __depthFreq.end()) {
            __logger.FastLogD("    Preview: %zu", it->second);
        }

        it = __depthFreq.find(eSTEREO_SCENARIO_RECORD);
        if(it != __depthFreq.end()) {
            __logger.FastLogD("    Record : %zu", it->second);
        }
    }

    __logger
    .FastLogD("Callback Buffer List: %s", __callbackBufferListString.c_str())
    .FastLogD("%s", "----------------------------------------------");

    __logger.print();
}

void
StereoSettingProviderKernel::__reset()
{
    __sensorSettings.clear();
    __sensorCombinationSettings.clear();
}
