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
#ifndef STEREO_SETTING_DEF_H_
#define STEREO_SETTING_DEF_H_

#include <vector>   //for std::vector
#include <utility>  //for std::pair

#include <camera_custom_stereo.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <mtkcam3/feature/stereo/hal/FastLogger.h>
#include <mtkcam/drv/IHalSensor.h>

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/aaa/IHal3A.h>

#include <mtkcam3/feature/stereo/hal/StereoArea.h>
#include <stereo_crop_util.h>

#include <mtkcam3/feature/stereo/hal/stereo_setting_keys.h>
#include <sstream>  //For ostringstream

#define IS_TK_VSDOF_MODE(mode) (   (mode & E_STEREO_FEATURE_VSDOF) \
                                || (mode & E_STEREO_FEATURE_MTK_DEPTHMAP) \
                                || (mode & E_STEREO_FEATURE_CAPTURE) \
                               )

#define IS_STEREO_MODE(mode) (   IS_TK_VSDOF_MODE(mode) \
                              || (mode & E_STEREO_FEATURE_MULTI_CAM) \
                              || (mode & E_STEREO_FEATURE_THIRD_PARTY) \
                              || (mode & E_STEREO_FEATURE_DENOISE) \
                             )

using namespace NSCam;
using namespace NS3Av3;
using namespace StereoHAL;
using namespace NSCam::v1::Stereo;

typedef std::unordered_map<int, int> SensorScenarioMap_T;

struct StereoCropSetting_T
{
    float keepRatio     = 1.0f; //keep ratio, i.e., 1.0 means no crop
    float baselineRatio = 0.5f; //min ratio of cropped edge
    float cropDegreeH   = 0.0f;
    float cropDegreeV   = 0.0f;

    // Runtime FOV after zoom, in radians
    float fovHRuntime;
    float fovVRuntime;

    CUST_FOV_CROP_T toCusCrop()
    {
        return CUST_FOV_CROP_T(cropDegreeH, baselineRatio);
    }
};

const std::unordered_map<int, const char *> SENSOR_SCENARIO_TO_STRING_MAP =
{
    { SENSOR_SCENARIO_ID_NORMAL_PREVIEW, "SENSOR_SCENARIO_ID_NORMAL_PREVIEW"   },
    { SENSOR_SCENARIO_ID_NORMAL_CAPTURE, "SENSOR_SCENARIO_ID_NORMAL_CAPTURE"   },
    { SENSOR_SCENARIO_ID_NORMAL_VIDEO,   "SENSOR_SCENARIO_ID_NORMAL_VIDEO"     },
    { SENSOR_SCENARIO_ID_SLIM_VIDEO1,    "SENSOR_SCENARIO_ID_HIGH_SPEED_VIDEO" },
    { SENSOR_SCENARIO_ID_SLIM_VIDEO2,    "SENSOR_SCENARIO_ID_SLIM_VIDEO"       },
    { SENSOR_SCENARIO_ID_CUSTOM1,        "SENSOR_SCENARIO_ID_CUSTOM1"          },
    { SENSOR_SCENARIO_ID_CUSTOM2,        "SENSOR_SCENARIO_ID_CUSTOM2"          },
    { SENSOR_SCENARIO_ID_CUSTOM3,        "SENSOR_SCENARIO_ID_CUSTOM3"          },
    { SENSOR_SCENARIO_ID_CUSTOM4,        "SENSOR_SCENARIO_ID_CUSTOM4"          },
    { SENSOR_SCENARIO_ID_CUSTOM5,        "SENSOR_SCENARIO_ID_CUSTOM5"          },
    { SENSOR_SCENARIO_ID_CUSTOM6,        "SENSOR_SCENARIO_ID_CUSTOM6"          },
    { SENSOR_SCENARIO_ID_CUSTOM7,        "SENSOR_SCENARIO_ID_CUSTOM7"          },
    { SENSOR_SCENARIO_ID_CUSTOM8,        "SENSOR_SCENARIO_ID_CUSTOM8"          },
    { SENSOR_SCENARIO_ID_CUSTOM9,        "SENSOR_SCENARIO_ID_CUSTOM9"          },
    { SENSOR_SCENARIO_ID_CUSTOM10,       "SENSOR_SCENARIO_ID_CUSTOM10"         },
    { SENSOR_SCENARIO_ID_CUSTOM11,       "SENSOR_SCENARIO_ID_CUSTOM11"         },
    { SENSOR_SCENARIO_ID_CUSTOM12,       "SENSOR_SCENARIO_ID_CUSTOM12"         },
    { SENSOR_SCENARIO_ID_CUSTOM13,       "SENSOR_SCENARIO_ID_CUSTOM13"         },
    { SENSOR_SCENARIO_ID_CUSTOM14,       "SENSOR_SCENARIO_ID_CUSTOM14"         },
    { SENSOR_SCENARIO_ID_CUSTOM15,       "SENSOR_SCENARIO_ID_CUSTOM15"         },
    { SENSOR_SCENARIO_ID_UNNAMED_START,  "Unknown"                             },
};

typedef const std::unordered_map<int, const char *> FEATURE_STRING_MAP_T;
FEATURE_STRING_MAP_T FEATURE_MODE_TO_STRING_MAP =
{
    { E_STEREO_FEATURE_CAPTURE,      CUSTOM_KEY_STEREO_CAPTURE },
    { E_STEREO_FEATURE_VSDOF,        CUSTOM_KEY_VSDOF          },
    { E_STEREO_FEATURE_DENOISE,      CUSTOM_KEY_DENOISE        },
    { E_STEREO_FEATURE_THIRD_PARTY,  CUSTOM_KEY_3RD_Party      },
    { E_DUALCAM_FEATURE_ZOOM,        CUSTOM_KEY_ZOOM           },
    { E_STEREO_FEATURE_MTK_DEPTHMAP, CUSTOM_KEY_MTK_DEPTHMAP   },
    { E_STEREO_FEATURE_MULTI_CAM,    CUSTOM_KEY_MULTI_CAM      },
    { E_SECURE_CAMERA,               CUSTOM_KEY_SECURE_CAMERA  },
};

#define LENS_POSE_TRANSLATION_SIZE (3)
struct StereoSensorSetting_T
{
    int             index             = -1; //0, 1, 2, 3
    int             devIndex          = 0;  //1, 2, 4, 8
    std::string     name;

    // FOV spec of sensor, in radians
    float           fovHorizontal     = 0.0f;
    float           fovVertical       = 0.0f;
    float           fovDiagonal       = 0.0f;

    // FOV after sensor crop, so they are related to sensor mode, in radians
    float           fovHSensorOutput  = 0.0f;
    float           fovVSensorOutput  = 0.0f;

    // Module variation after sensor crop for this sensor, in radians
    float           fovVariationSensorOutput = 0.0f;

    std::unordered_map<STEREO_IMAGE_RATIO_T, NSCam::MSize> imgoYuvSize;
    std::unordered_map<STEREO_IMAGE_RATIO_T, NSCam::MSize> rrzoYuvSize;

    int             distanceMacro     = 0;  //calibration distance of macro, unit: mm
    int             distanceInfinite  = 0;  //calibration distance of inf, unit: mm

    SensorScenarioMap_T sensorScenarioMapZSD;           //Feature -> sensor scenario(pipeline: ZSD)
    SensorScenarioMap_T sensorScenarioMapRecord;        //Feature -> sensor scenario(pipeline: RECORDING)

    NSCam::SensorStaticInfo staticInfo;

    //Calculated by FOVCropUtil, preview crop ratio -> crop setting
    unordered_map<float, StereoCropSetting_T> cropSettingMap;

    std::unordered_map<int, float>    frzRatioSetting;    //feature mode -> FRZ Ratio

    std::vector<float> lensPoseTranslation; //for static metadata LENS_POSE_TRANSLATION

    ~StereoSensorSetting_T()
    {
    }

    std::string stereoFeatureModeToString(int featureMode) const
    {
        std::string name;
        int featureOnly = featureMode & ~E_STEREO_FEATURE_PORTRAIT_FLAG;
        if(FEATURE_MODE_TO_STRING_MAP.find(featureOnly) != FEATURE_MODE_TO_STRING_MAP.end()) {
            name = FEATURE_MODE_TO_STRING_MAP.at(featureOnly);

            if(featureMode & E_STEREO_FEATURE_PORTRAIT_FLAG) {
                name += "(Portrait)";
            }
        }

        return name;
    }

    void log(FastLogger &logger) const
    {
        logger
        .FastLogD("Address:  %p", this)
        .FastLogD("Index:    %d", index)
        .FastLogD("DevIndex: %d", devIndex)
        .FastLogD("UID:      %#X", staticInfo.sensorDevID)
        .FastLogD("Name:     %s", name.c_str())
        .FastLogD("Facing:   %s", (staticInfo.facingDirection==0) ? "Rear" : "Front");

        if(fovDiagonal > 0.0f)
        {
            logger.FastLogD("FOV       D: %.2f(H: %.2f / V: %.2f)",
                            radiansToDegree(fovDiagonal),
                            radiansToDegree(fovHorizontal),
                            radiansToDegree(fovVertical));
        }
        else if(fovHorizontal > 0.0f || fovVertical > 0.0f)
        {
            logger.FastLogD("FOV       H: %.2f / V: %.2f",
                            radiansToDegree(fovHorizontal),
                            radiansToDegree(fovVertical));
        }

        if(lensPoseTranslation.size() > 0) {
            logger
            .FastLogD("Lens Pose Translation(in meter): [%.5f, %.5f %.5f]", lensPoseTranslation[0], lensPoseTranslation[1], lensPoseTranslation[2]);
        }

        if(distanceMacro > 0 && distanceInfinite > 0) {
            logger
            .FastLogD("Calibration Distance(in mm): Macro: %d, Infinite: %d", distanceMacro, distanceInfinite);
        }

        //IMGO
        for(auto &m1 : imgoYuvSize) {
            logger.FastLogD("IMGO Yuv Size of %s: %dx%d", (const char *)m1.first, m1.second.w, m1.second.h);
        }

        //RRZO
        for(auto &m1 : rrzoYuvSize) {
            logger.FastLogD("RRZO YUV Size of %s: %dx%d", (const char *)m1.first, m1.second.w, m1.second.h);
        }

        if(sensorScenarioMapZSD.size() > 0) {
            logger.FastLogD("%s", "---- Sensor Scenario for ZSD ----");
            for(auto &s : sensorScenarioMapZSD) {
                std::string featureName = stereoFeatureModeToString(s.first);
                logger.FastLogD("  %-20s: %-3d(%s)", featureName.c_str(), s.second, SENSOR_SCENARIO_TO_STRING_MAP.at(s.second));
            }
        }

        if(sensorScenarioMapRecord.size() > 0) {
            logger.FastLogD("%s", "---- Sensor Scenario for Record ----");
            for(auto &s : sensorScenarioMapRecord) {
                std::string featureName = stereoFeatureModeToString(s.first);
                logger.FastLogD("  %-20s: %-3d(%s)", featureName.c_str(), s.second, SENSOR_SCENARIO_TO_STRING_MAP.at(s.second));
            }
        }

        if(frzRatioSetting.size() > 0) {
            logger.FastLogD("%s", "---- FRZ Ratio ----");
            for(auto &s : frzRatioSetting) {
                std::string featureName = stereoFeatureModeToString(s.first);
                logger.FastLogD("  %-20s: %f", featureName.c_str(), s.second);
            }
        }
    }

    MINT32 getSensorScenario(MINT32 featureMode, MINT32 pipelineMode=PipelineMode_ZSD)
    {
        if(0 == featureMode) {
            return SENSOR_SCENARIO_ID_UNNAMED_START;
        }

        const int FEATURE_ID[] = //Upper feature has higher priority
        {
            NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP,
            NSCam::v1::Stereo::E_STEREO_FEATURE_VSDOF,
            NSCam::v1::Stereo::E_STEREO_FEATURE_MULTI_CAM,
            NSCam::v1::Stereo::E_STEREO_FEATURE_CAPTURE,
            NSCam::v1::Stereo::E_STEREO_FEATURE_DENOISE,
            NSCam::v1::Stereo::E_DUALCAM_FEATURE_ZOOM,
            NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY,
        };

        SensorScenarioMap_T *scenarioMap = NULL;
        if(pipelineMode == PipelineMode_ZSD) {
            scenarioMap = &sensorScenarioMapZSD;
        } else if(pipelineMode == PipelineMode_RECORDING) {
            scenarioMap = &sensorScenarioMapRecord;
        } else {
            ALOGE("Only support pipeline mode: PipelineMode_ZSD or PipelineMode_RECORDING");
        }

        if(scenarioMap) {
            const bool IS_PORTRAIT = featureMode & E_STEREO_FEATURE_PORTRAIT_FLAG;
            for(int searchFeature : FEATURE_ID) {
                if(IS_PORTRAIT) {
                    searchFeature |= E_STEREO_FEATURE_PORTRAIT_FLAG;
                }

                if((featureMode & searchFeature) &&
                   scenarioMap->find(searchFeature) != scenarioMap->end())
                {
                    return (*scenarioMap)[searchFeature];
                }

                //If feature mode not found, try to load with/without portrait mode
                searchFeature ^= E_STEREO_FEATURE_PORTRAIT_FLAG;
                if((featureMode & searchFeature) &&
                   scenarioMap->find(searchFeature) != scenarioMap->end())
                {
                    std::string featureName = stereoFeatureModeToString(searchFeature);
                    ALOGD("Use sensor scenario %s of feature %s %s portrait mode, sensor %d, pipeline mode %d",
                          SENSOR_SCENARIO_TO_STRING_MAP.at((*scenarioMap)[searchFeature]), featureName.c_str(),
                          (IS_PORTRAIT)?"without":"with", index, pipelineMode);
                    return (*scenarioMap)[searchFeature];
                }
            }
        }

        ALOGW("Cannot find sensor scenario for sensor %d, featureMode %#X, pipeline mode %d",
              index, featureMode, pipelineMode);

        return SENSOR_SCENARIO_ID_UNNAMED_START;
    }

    bool updateSensorOutputFOV(int featureMode, MINT32 pipelineMode=PipelineMode_ZSD)
    {
        int sensorScenario = getSensorScenario(featureMode, pipelineMode);
        if(sensorScenario == SENSOR_SCENARIO_ID_UNNAMED_START) {
            return false;
        }

        IHalSensorList* sensorList = nullptr;
        sensorList = MAKE_HalSensorList();
        if(NULL == sensorList) {
            ALOGE("Cannot get sensor list");
            return false;
        }
        IHalSensor* pIHalSensor = sensorList->createSensor(LOG_TAG, index);
        if(NULL == pIHalSensor) {
            ALOGE("Cannot get hal sensor");
            return false;
        }

        //Get sensor crop win info
        SensorCropWinInfo sensorCropInfo;
        MINT32 err = pIHalSensor->sendCommand(devIndex, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                       (MUINTPTR)&sensorScenario, (MUINTPTR)&sensorCropInfo, 0);
        if(err) {
            ALOGE("Cannot get sensor crop win info");
            return false;
        }

        //We only support CENTER CROP by sensor
        float ratio = 1.0f;
        if(sensorCropInfo.w2_tg_size != sensorCropInfo.scale_w ||
           sensorCropInfo.w0_size != sensorCropInfo.full_w)
        {
            ratio = (float)sensorCropInfo.w2_tg_size/(float)sensorCropInfo.scale_w *
                    (float)sensorCropInfo.w0_size/(float)sensorCropInfo.full_w;
            fovHSensorOutput = 2.0f*atan(ratio * tan(fovHorizontal/2.0f));
        } else {
            fovHSensorOutput = fovHorizontal;
        }

        if(sensorCropInfo.h2_tg_size != sensorCropInfo.scale_h ||
           sensorCropInfo.h0_size != sensorCropInfo.full_h)
        {
            ratio = (float)sensorCropInfo.h2_tg_size/(float)sensorCropInfo.scale_h *
                    (float)sensorCropInfo.h0_size/(float)sensorCropInfo.full_h;
            fovVSensorOutput = 2.0f*atan(ratio * tan(fovVertical/2.0f));
        } else {
            fovVSensorOutput = fovVertical;
        }

        return true;
    }

    void updateModuleVariation(const float MODULE_VARIATION, const bool isVertical)
    {
        if(isVertical) {
            float sensorOutputRatio = radiansRatio(fovVSensorOutput, fovVertical);
            fovVariationSensorOutput = std::max(
                atan(tan((fovVertical+MODULE_VARIATION)/2.0f)*sensorOutputRatio)*2.0f-fovVSensorOutput,
                fovVSensorOutput-atan(tan((fovVertical-MODULE_VARIATION)/2.0f)*sensorOutputRatio)*2.0f);
        } else {
            float sensorOutputRatio = radiansRatio(fovHSensorOutput, fovHorizontal);
            fovVariationSensorOutput = std::max(
                atan(tan((fovHorizontal+MODULE_VARIATION)/2.0f)*sensorOutputRatio)*2.0f-fovHSensorOutput,
                fovHSensorOutput-atan(tan((fovHorizontal-MODULE_VARIATION)/2.0f)*sensorOutputRatio)*2.0f);
        }
    }
};

#define DEFAULT_MODULE_VARIATION (1.0f)     //degree
#define DEFAULT_WORKING_RANGE    (20.0f)    //cm

extern std::unordered_map<int, std::string> ISP_PROFILE_MAP;

struct StereoSensorConbinationSetting_T
{
    MUINT32         logicalDeviceID = 0;
    std::string     logicalDeviceName;
    MUINT32         logicalDeviceFeatureSet = 0;

    std::vector<std::string> sensorNames;
    std::vector<StereoSensorSetting_T *> sensorSettings;
    MUINT32         moduleType      = 0;     //1-4
    float           baseline        = 0.95f;  //cm
    // If sensor offset is set, we'll ignore baseline/module type setting
    // Main cam will be (0, 0), so user only needs to config for the rest cameras
    std::vector<std::pair<float, float>> sensorOffset = {{0, 0}};
    int             moduleRotation = -1;  // 0-359
    bool            isSlant        = false;

    bool            enableLDC       = true;
    std::vector<float> LDC          = {0};

    bool            isCenterCrop    = true;
    bool            disableCrop     = false;
    float           moduleVariation = degreeToRadians(DEFAULT_MODULE_VARIATION);
    float           workingRange    = DEFAULT_WORKING_RANGE;

    //For 3rd party
    std::unordered_map<STEREO_IMAGE_RATIO_T, NSCam::MSize> depthmapSize;
    std::unordered_map<STEREO_IMAGE_RATIO_T, NSCam::MSize> depthmapSizeCapture;

    //Customized VSDoF size
    bool            hasSizeConfig = false;
    std::unordered_map<STEREO_IMAGE_RATIO_T, StereoHAL::StereoArea> baseSize;

    //Offline calibration
    std::vector<MUINT8> calibrationData;

    // Support to define different refine level for different stereo scenario
    std::unordered_map<ENUM_STEREO_SCENARIO, ENUM_DEPTHMAP_REFINE_LEVEL> depthmapRefineLevel;

    //Multicam zoom range
    std::vector<float> multicamZoomRanges = {1.0f};

    //Multicam zoom steps
    std::vector<float> multicamZoomSteps = {1.0f};

    //Overwrite sensor scenario in sensor setting if set
    //Since each logical device can have only one feature,
    //we do not need to map from feature like sensor setting
    std::vector<int> sensorScenariosZSD;
    std::vector<int> sensorScenariosRecord;

    bool isRecordEnabled = false;

    std::vector<std::vector<StereoSensorSetting_T *>> exclusiveGroups;

    std::unordered_map<int, std::vector<int>> ispProfiles;  //scenario -> isp profile list for each sensor

    int depthFlow = -1; //If set, it will be one of the vsdof value in NSCam::v1::Stereo

    int depthDelay = -1;    //-1: use global setting, 0: disable, 1: enable

    std::unordered_map<int, size_t> depthFreq;  //0: use default frequency, 1-30: output depthmap for every n frame

    int frameSyncType = E_FRAME_SYNC_DEFAULT;

    void log(FastLogger &logger)
    {
        logger
        .FastLogD("Address:         %p", this)
        .FastLogD("%s", "Logical Device:")
        .FastLogD("    ID:          %d", logicalDeviceID)
        .FastLogD("    Name:        %s", logicalDeviceName.c_str());

        std::ostringstream ossFeatureSet;
        if(logicalDeviceFeatureSet)
        {
            std::vector<string> features;
            FEATURE_STRING_MAP_T::const_iterator it = FEATURE_MODE_TO_STRING_MAP.begin();
            for(; it != FEATURE_MODE_TO_STRING_MAP.end(); ++it) {
                if(logicalDeviceFeatureSet & it->first) {
                    if(it->second)
                    {
                        features.push_back(it->second);
                    }
                }
            }
            std::copy(features.begin(), features.end()-1, std::ostream_iterator<string>(ossFeatureSet, ", "));
            ossFeatureSet << features.back();
        }
        else
        {
            ossFeatureSet << "N/A";
        }

        logger
        .FastLogD("    Feature Set: %s", ossFeatureSet.str().c_str())
        .FastLogD("    Record:      %d", isRecordEnabled)
        .FastLogD("%s", "Sensors:");
        for(auto &setting : sensorSettings) {
            logger.FastLogD("    %s(%d), address %p", setting->name.c_str(), setting->index, setting);
        }

        //Log exclusive groups
        std::vector<std::pair<int, std::string>> exclusiveList;
        for(int groupId = 0; groupId < exclusiveGroups.size(); ++groupId)
        {
            auto &group = exclusiveGroups[groupId];
            if(group.size() > 1)
            {
                std::string groupSensorName = group[0]->name + "(" + std::to_string(group[0]->index) + ")";
                for(int s = 1; s < group.size(); ++s) {
                    groupSensorName = groupSensorName + ", " + group[s]->name + "(" + std::to_string(group[s]->index) + ")";
                }
                exclusiveList.push_back({groupId, groupSensorName});
            }
        }

        if(exclusiveList.size() > 0)
        {
            logger
            .FastLogD("%s", "Exclusive Groups(sensors are exclusive within a group):");
            for(auto &ex : exclusiveList) {
                logger.FastLogD("    [%d] Sensors: %s", ex.first, ex.second.c_str());
            }
        }

        if(sensorScenariosZSD.size() > 0 &&
           sensorScenariosZSD.size() == sensorSettings.size())
        {
            std::ostringstream oss;
            for(auto iter = sensorScenariosZSD.begin(); iter < sensorScenariosZSD.end()-1; iter++) {
                oss << SENSOR_SCENARIO_TO_STRING_MAP.at(*iter) << ", ";
            }
            oss << SENSOR_SCENARIO_TO_STRING_MAP.at(sensorScenariosZSD.back());
            logger.FastLogD("Sensor Scenarios for ZSD: [%s]", oss.str().c_str());
        }

        if(sensorScenariosRecord.size() > 0 &&
           sensorScenariosRecord.size() == sensorSettings.size())
        {
            std::ostringstream oss;
            for(auto iter = sensorScenariosRecord.begin(); iter < sensorScenariosRecord.end()-1; iter++) {
                oss << SENSOR_SCENARIO_TO_STRING_MAP.at(*iter) << ", ";
            }
            oss << SENSOR_SCENARIO_TO_STRING_MAP.at(sensorScenariosRecord.back());
            logger.FastLogD("Sensor Scenarios for Record: [%s]", oss.str().c_str());
        }

        if(moduleType > 0)
        {
            logger
            .FastLogD("Module Type:      %d(%s, %s)", moduleType,
                                                    (2==moduleType || 4==moduleType) ? "Horizontal" : "Vertical",
                                                    (1==moduleType || 2==moduleType) ? "Main on left" : "Main on right");
        }

        logger.FastLogD("Frame Sync:       %d(%s)", frameSyncType, (frameSyncType==0)?"VSync align":"Read out center align");

        if(IS_STEREO_MODE(logicalDeviceFeatureSet))
        {
            logger
            .FastLogD("Baseline:         %.3f cm", baseline)
            .FastLogD("DisableCrop:      %s", (disableCrop)?"Yes":"No");

            if(!disableCrop) {
                logger.FastLogD("CenterCrop:       %s", (isCenterCrop)?"Yes":"No");
            }

            if(IS_TK_VSDOF_MODE(logicalDeviceFeatureSet))
            {
                logger
                .FastLogD("Module Variation: %.1f degrees", radiansToDegree(moduleVariation))
                .FastLogD("Working Range:    %.1f cm", workingRange)
                .FastLogD("Module Rotation:  % 3d degrees", moduleRotation);

                if (sensorOffset.size() > 1) {
                    logger.FastLogD("%s", "Sensor Offset:");
                    for(auto &offset : sensorOffset) {
                        logger.FastLogD("    (% .3f, % .3f)", offset.first, offset.second);
                    }
                }

                if(depthmapRefineLevel.size() > 0) {
                    logger.FastLogD("%s", "Refine Level:");
                    std::string refineScenaio;
                    std::string refineLevel;
                    for (auto &refineValue : depthmapRefineLevel)
                    {
                        refineScenaio = (refineValue.first == eSTEREO_SCENARIO_PREVIEW)
                                        ? "Preview" : "Record ";
                        switch(refineValue.second) {
                            case E_DEPTHMAP_REFINE_NONE:
                            default:
                                refineLevel = "None";
                                break;
                            case E_DEPTHMAP_REFINE_SW_OPTIMIZED:
                                refineLevel = "SW Optimized";
                                break;
                            case E_DEPTHMAP_REFINE_HOLE_FILLED:
                                refineLevel = "Hole filled";
                                break;
                            case E_DEPTHMAP_REFINE_AI_DEPTH:
                                refineLevel = "AI Depth";
                                break;
                        }
                        logger.FastLogD("    %s: %s", refineScenaio.c_str(), refineLevel.c_str());
                    }
                }
            }

            for(auto &m1 : depthmapSize) {
                logger.FastLogD("Depthmap Size of %s: %dx%d", (const char *)m1.first, m1.second.w, m1.second.h);
            }

            for(auto &m1 : depthmapSizeCapture) {
                logger.FastLogD("Capture Depthmap Size of %s: %dx%d", (const char *)m1.first, m1.second.w, m1.second.h);
            }

            if(hasSizeConfig) {
                for(auto &m1 : baseSize) {
                    StereoArea area = m1.second;
                    logger.FastLogD("Size config for %s:   Content %dx%d, Padding %dx%d", (const char *)m1.first,
                                    area.contentSize().w, area.contentSize().h,
                                    area.padding.w,area.padding.h);
                }
            }

            if(LDC.size() == 0) {
                logger.FastLogD("%s", "LDC disabled");
            } else {
                logger.FastLogD("LDC enabled, table size %zu:", LDC.size());

                if(LDC.size() < 16) {
                    for(size_t i = 0; i < LDC.size(); i++) {
                        logger.FastLogD(" % *.10f", 2, LDC[i]);
                    }
                } else {
                    const float *element = &LDC[0];
                    for(int k = 0; k < 2; k++) {
                        int line = (int)*element++;
                        int sizePerLine = (int)*element++;
                        logger.FastLogD(" %d %d\n", line, sizePerLine);
                        for(int i = 1; i <= line; i++) {
                            logger.FastLogD("[%02d]% *.10f % *.10f ... % *.10f % *.10f %*d %*d\n",
                                            i,
                                            2, *element, 2, *(element+1),
                                            2, *(element+sizePerLine-2), 2, *(element+sizePerLine-1),
                                            4, (int)*(element+sizePerLine),
                                            2, (int)*(element+sizePerLine+1));
                            element += (sizePerLine+2);
                        }
                    }
                }
            }

            if(calibrationData.size() > 0) {
                logger.FastLogD("Offline calibration size %d", calibrationData.size());
            }

            if(ispProfiles.size() > 0) {
                logger.FastLogD("%s", "ISP Profile:");
                for(auto &elm : ispProfiles) {
                    std::ostringstream oss;
                    oss << "    ";
                    std::string scenarioName;
                    switch(elm.first)
                    {
                    case eSTEREO_SCENARIO_PREVIEW:
                    default:
                        oss << "Preview: [";
                        break;
                    case eSTEREO_SCENARIO_RECORD:
                        oss << "Record:  [";
                        break;
                    // case eSTEREO_SCENARIO_CAPTURE:
                    //     oss << "Capture: [";
                    //     break;
                    }

                    auto profileIter = elm.second.begin();
                    while(profileIter != elm.second.end())
                    {
                        auto iter = ISP_PROFILE_MAP.find(*profileIter);
                        if(iter != ISP_PROFILE_MAP.end())
                        {
                            oss << iter->second << "(" << iter->first << ")";
                        }
                        else
                        {
                            oss << *profileIter;
                        }

                        if(profileIter != elm.second.end()-1) {
                            oss << ", ";
                        }

                        profileIter++;
                    }
                    oss << "]";
                    logger.FastLogD("%s", oss.str().c_str());
                }
            }

            if(depthFlow >= 0)
            {
                std::string flowName = "undefined";
                if(0 == depthFlow)
                {
                    flowName = CUSTOM_KEY_VSDOF;
                }
                else if(1 == depthFlow)
                {
                    flowName = CUSTOM_KEY_3RD_Party;
                }
                else if(2 == depthFlow)
                {
                    flowName = CUSTOM_KEY_MTK_DEPTHMAP;
                }

                if(logicalDeviceFeatureSet & NSCam::v1::Stereo::E_MULTICAM_FEATURE_HIDL_FLAG) {
                    flowName += "+hidl";
                }

                logger
                .FastLogD("%s:       %d(%s)", CUSTOM_KEY_DEPTH_FLOW, depthFlow, flowName.c_str());
            }

            if(depthDelay >= 0)
            {
                logger
                .FastLogD("%s:      %s", CUSTOM_KEY_DEPTH_DELAY, (depthDelay>0)?"Enable":"Disable");
            }

            if(depthFreq.size() > 0)
            {
                logger.FastLogD("%s:", CUSTOM_KEY_DEPTH_FREQ);
                auto it = depthFreq.find(eSTEREO_SCENARIO_PREVIEW);
                if(it != depthFreq.end()) {
                    logger.FastLogD("    Preview: %zu", it->second);
                }

                it = depthFreq.find(eSTEREO_SCENARIO_RECORD);
                if(it != depthFreq.end()) {
                    logger.FastLogD("    Record : %zu", it->second);
                }
            }
        }

        if(logicalDeviceFeatureSet & E_DUALCAM_FEATURE_ZOOM)
        {
            size_t multicamZoomRangeSize = multicamZoomRanges.size();
            if(multicamZoomRangeSize > 0)
            {
                logger.FastLogD("Multicam Zoom Range [%.2f %.2f]", multicamZoomRanges[0], multicamZoomRanges[multicamZoomRangeSize-1]);
            }

            if(!multicamZoomSteps.empty())
            {
                std::ostringstream oss;
                oss.precision(2);
                std::copy(multicamZoomSteps.begin(), multicamZoomSteps.end()-1, std::ostream_iterator<float>(oss, ", "));
                oss << multicamZoomSteps.back();
                logger.FastLogD("Multicam Zoom Steps [%s]", oss.str().c_str());
            }
        }
    }
};

#endif