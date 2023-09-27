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
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.set
 ************************************************************************************************/
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "StereoSettingProvider"

#include <algorithm>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <stereo_tuning_provider.h>

#include <sstream>  //For ostringstream

#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/def/ImageFormat.h>
#include <camera_custom_stereo.h>       // For CUST_STEREO_* definitions.
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
CAM_ULOG_DECLARE_MODULE_ID(CAM_ULOG_MODULE_ID);
#include "../tuning-provider/stereo_tuning_provider_kernel.h"
#include <math.h>

#include <cutils/properties.h>
// #include "../inc/stereo_dp_util.h"
#include <mtkcam/aaa/IHal3A.h>

#include "stereo_setting_provider_kernel.h"
#if (1==VSDOF_SUPPORTED)
#include <fefm_setting_provider.h>
#endif

#include <ctime>
#include <mtkcam/drv/mem/cam_cal_drv.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include "stereo_setting_def.h"
#include <isp_tuning.h>

#if defined(__func__)
#undef __func__
#endif
#define __func__ __FUNCTION__

#define MY_LOGD(fmt, arg...)    CAM_ULOGMD("[%s]" fmt, __func__, ##arg)
#define MY_LOGI(fmt, arg...)    CAM_ULOGMI("[%s]" fmt, __func__, ##arg)
#define MY_LOGW(fmt, arg...)    CAM_ULOGMW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
#define MY_LOGE(fmt, arg...)    CAM_ULOGME("[%s] %s ERROR(%5d):" fmt, __func__,__FILE__, __LINE__, ##arg)

#define MY_LOGV_IF(cond, arg...)    if (cond) { MY_LOGV(arg); }
#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }

#define FUNC_START          MY_LOGD_IF(LOG_ENABLED, "+")
#define FUNC_END            MY_LOGD_IF(LOG_ENABLED, "-")

using namespace NSCam;
using namespace android;
using namespace NSCam::v1::Stereo;
using namespace StereoHAL;
using namespace NS3Av3;

std::mutex FOVCropUtil::__lock;

MUINT32 StereoSettingProvider::__logicalDeviceID = 0;
STEREO_IMAGE_RATIO_T StereoSettingProvider::__imageRatio = eStereoRatio_16_9;
MINT32 StereoSettingProvider::__stereoFeatureMode = 0;
bool StereoSettingProvider::__isRecording = false;
MUINT32 StereoSettingProvider::__eisFactor = 100;
MINT32 StereoSettingProvider::__stereoModuleType = -1;
MUINT32 StereoSettingProvider::__stereoShotMode = 0;
std::vector<MUINT> StereoSettingProvider::__sensorScenarios;
MUINT32 StereoSettingProvider::__featureStartTime = 0;
ENUM_STEREO_CAM_SCENARIO StereoSettingProvider::__vsdofScenario = eStereoCamPreview;
bool StereoSettingProvider::__3rdCaptureDepthRunning = false;
bool StereoSettingProvider::__3rdCaptureBokehRunning = false;
float StereoSettingProvider::__previewCropRatio = 0.0f;

std::vector<StereoSensorSetting_T *> g_sensorSettings;
StereoSensorConbinationSetting_T *g_currentSensorCombination = NULL;

std::unordered_map<MINT32, MULTICAM_LOGICAL_DEVICE_PARAM> g_logicalParamMap;

StereoSensorConbinationSetting_T *
__getSensorCombinationSetting(MUINT32 logicalDeviceID)
{
    return StereoSettingProviderKernel::getInstance()->getSensorCombinationSetting(logicalDeviceID);
}

bool __getSensorSettings(MUINT32 logicalDeviceID,
                         std::vector<StereoSensorSetting_T *> &settings)
{
    bool result = true;
    StereoSensorConbinationSetting_T *sensorCombination =
        StereoSettingProviderKernel::getInstance()->getSensorCombinationSetting(logicalDeviceID);

    if(sensorCombination &&
       sensorCombination->sensorSettings.size() > 0)
    {
        settings = sensorCombination->sensorSettings;
    } else {
        MY_LOGW("Cannot get sensor settings");
        result = false;
    }

    return result;
}

bool __getSensorSettingsAndCombination(MUINT32 logicalDeviceID,
                                       std::vector<StereoSensorSetting_T *> &sensorSettings,
                                       StereoSensorConbinationSetting_T *&sensorCombination)
{
    bool result = true;
    sensorCombination = StereoSettingProviderKernel::getInstance()->getSensorCombinationSetting(logicalDeviceID);

    if(sensorCombination)
    {
        sensorSettings = sensorCombination->sensorSettings;
    } else {
        MY_LOGW("Cannot get sensor settings");
        result = false;
    }

    return result;
}

ENUM_STEREO_SENSOR_PROFILE
StereoSettingProvider::stereoProfile(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination = StereoSettingProviderKernel::getInstance()->getSensorCombinationSetting(logicalDeviceID);
    if(sensorCombination &&
       sensorCombination->sensorSettings.size() > 1 &&
       sensorCombination->sensorSettings[0] &&
       sensorCombination->sensorSettings[1])
    {
        if(0 == sensorCombination->sensorSettings[0]->staticInfo.facingDirection) {
            if(0 == sensorCombination->sensorSettings[1]->staticInfo.facingDirection) {
                return STEREO_SENSOR_PROFILE_REAR_REAR;
            } else {
                return STEREO_SENSOR_PROFILE_REAR_FRONT;
            }
        } else {
            if(1 == sensorCombination->sensorSettings[1]->staticInfo.facingDirection) {
                return STEREO_SENSOR_PROFILE_FRONT_FRONT;
            } else {
                return STEREO_SENSOR_PROFILE_REAR_FRONT;
            }
        }
    }

    return STEREO_SENSOR_PROFILE_UNKNOWN;
}

bool
StereoSettingProvider::getStereoSensorIndex(int32_t &main1Idx, int32_t &main2Idx, MUINT32 logicalDeviceID)
{
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    vector<MINT32> sensorIDs;
    if(pHalDeviceList) {
        sensorIDs = pHalDeviceList->getSensorId(logicalDeviceID);
    }

    if(sensorIDs.size() > 1)
    {
        main1Idx = sensorIDs[0];
        main2Idx = sensorIDs[1];
    }
    else
    {
        auto pHalSensorList = MAKE_HalSensorList();
        const int SENSOR_COUNT = (pHalSensorList) ? pHalSensorList->queryNumberOfSensors() : 3;
        if(2 == SENSOR_COUNT) {
            // MY_LOGW("Only two sensors were found on the device");
            main1Idx = 0;
            main2Idx = 1;
        } else {
            main1Idx = 0;
            main2Idx = 2;
        }

        MY_LOGW("Cannot get sensor index by logical device ID %d, return %d, %d",
                logicalDeviceID, main1Idx, main2Idx);

        return false;
    }

    return true;
}

bool
StereoSettingProvider::getStereoSensorDevIndex(int32_t &main1DevIdx, int32_t &main2DevIdx, MUINT32 logicalDeviceID)
{
    int32_t main1Idx = 0;
    int32_t main2Idx = 0;
    if (!getStereoSensorIndex(main1Idx, main2Idx, logicalDeviceID)) {
        return false;
    }

    IHalSensorList *sensorList = MAKE_HalSensorList();
    if (NULL == sensorList) {
        return false;
    }

    main1DevIdx = sensorList->querySensorDevIdx(main1Idx);
    main2DevIdx = sensorList->querySensorDevIdx(main2Idx);
    // MY_LOGD_IF(isLogEnabled(), "Main sensor DEV idx %d, Main2 sensor DEV idx %d", main1DevIdx, main2DevIdx);

    return true;
}

void
StereoSettingProvider::setStereoProfile( ENUM_STEREO_SENSOR_PROFILE profile __attribute__((unused)) )
{
// #warning "[Dualcam]This function is only for debug now, will call setLogicalDeviceID(sensor count)"
    IHalSensorList *sensorList = MAKE_HalSensorList();
    if(sensorList) {
        setLogicalDeviceID(sensorList->queryNumberOfSensors());
    }
}

bool
StereoSettingProvider::setLogicalDeviceParam(MULTICAM_LOGICAL_DEVICE_PARAM param)
{
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(!pHalDeviceList) {
        MY_LOGE("Cannot get logical device list");
        return false;
    }

    auto deviceCount = pHalDeviceList->queryNumberOfDevices();
    if( param.logicalDeviceID < 0 ||
        param.logicalDeviceID >= deviceCount )
    {
        MY_LOGE("Invalid logical device ID: %d", param.logicalDeviceID);
        return false;
    }

    if(setLogicalDeviceID(param.logicalDeviceID)) {
        auto it = g_logicalParamMap.find(param.logicalDeviceID);
        if(it != g_logicalParamMap.end()) {
            it->second = param;
        } else {
            g_logicalParamMap.emplace(param.logicalDeviceID, param);
        }

        setStereoFeatureMode(param.stereoMode, param.isPortrait, param.isRecording, param.eisFactor);
        setPreviewSize(param.previewSize);
        return true;
    }

    return false;
}

bool
StereoSettingProvider::setLogicalDeviceID(const MUINT32 logicalDeviceID)
{
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(pHalDeviceList) {
        auto sensorIDs = pHalDeviceList->getSensorId(logicalDeviceID);
        if(sensorIDs.size() < 2) {
            MY_LOGD("Ignore single camera");
            return false;
        }

        auto n = pHalDeviceList->queryDriverName(logicalDeviceID);
        std::string s((n)?n:"");
        size_t prefixPos = s.find(SENSOR_DRVNAME_PREFIX);
        if(prefixPos != std::string::npos) {
            s.erase(prefixPos, SENSOR_DRVNAME_PREFIX.length());
        }

        MY_LOGD("Set logical device ID %d(%s)", logicalDeviceID, s.c_str());
    }
    else
    {
        MY_LOGE("Cannot get logical device list");
        return false;
    }

#if (1==VSDOF_SUPPORTED)
        FEFMSettingProvider::destroyInstance();
#endif
    StereoSettingProviderKernel::getInstance()->init();
    // StereoSettingProviderKernel::getInstance()->logSettings();

    __getSensorSettingsAndCombination(logicalDeviceID, g_sensorSettings, g_currentSensorCombination);

    if(g_currentSensorCombination == nullptr)
    {
        StereoSizeProvider::destroyInstance();
    }
    else
    {
        StereoSizeProvider::getInstance()->reset();
    }
    __logicalDeviceID = logicalDeviceID;
    __isRecording = false;
    __previewCropRatio = 0.0f;

    const size_t SETTING_SIZE = g_sensorSettings.size();
    if(SETTING_SIZE > 0) {
        std::ostringstream oss;
        oss << "Sensor settings(" << SETTING_SIZE << "): ";
        std::copy(g_sensorSettings.begin(), g_sensorSettings.end()-1, std::ostream_iterator<void*>(oss, " "));
        oss << g_sensorSettings.back() << ", combination " << g_currentSensorCombination;
        MY_LOGD("%s", oss.str().c_str());
    }
    else
    {
        MY_LOGD("Cannot find any sensor setting for logical device ID %d", logicalDeviceID);
    }

    return true;
}

void
StereoSettingProvider::setImageRatio(STEREO_IMAGE_RATIO_T ratio)
{
    char value[PROP_VALUE_MAX];
    size_t len = ::property_get("vendor.STEREO.ratio", value, NULL);
    if(len > 0) {
        ratio = STEREO_IMAGE_RATIO_T(value);
    }

    __imageRatio = std::move(ratio);
    MY_LOGD("Set image ratio to %s", (const char *)__imageRatio);

    StereoSizeProvider::getInstance()->reset();
    _updateImageSettings();
    __initTuningIfReady();
}

void
StereoSettingProvider::setStereoFeatureMode(MINT32 stereoMode, bool isPortrait, bool isRecording, MUINT32 eisFactor)
{
    if(g_currentSensorCombination == nullptr) {
        return;
    }

    MY_LOGD("Set stereo feature mode: %s -> %s, portrait: %d, record: %d, eisFactor: %u",
            _getStereoFeatureModeString(__stereoFeatureMode).c_str(),
            _getStereoFeatureModeString(stereoMode).c_str(), isPortrait, isRecording, eisFactor);

    __isRecording = isRecording;
    __eisFactor = (eisFactor > 100) ? eisFactor : 100;

    auto it = g_logicalParamMap.find(getLogicalDeviceID());
    if(it != g_logicalParamMap.end()) {
        it->second.stereoMode  = stereoMode;
        it->second.isPortrait  = isPortrait;
        it->second.isRecording = isRecording;
        it->second.eisFactor   = eisFactor;
    } else {
        MULTICAM_LOGICAL_DEVICE_PARAM param(getLogicalDeviceID(), stereoMode);
        param.isPortrait      = isPortrait;
        param.isRecording     = isRecording;
        param.eisFactor       = eisFactor;
        g_logicalParamMap.emplace(param.logicalDeviceID, param);
    }

    if((__stereoFeatureMode & ~E_STEREO_FEATURE_PORTRAIT_FLAG) == stereoMode) {
        return;
    }

    //Update sensor scrnario
    __stereoFeatureMode = stereoMode;
    __previewCropRatio = 0.0f;
    if(stereoMode == 0)
    {
        StereoSizeProvider::destroyInstance();
    }
    else
    {
        StereoSizeProvider::getInstance()->reset();

        if(isPortrait) {
            __stereoFeatureMode |= E_STEREO_FEATURE_PORTRAIT_FLAG;
        }
        getSensorScenarios(__stereoFeatureMode, isRecording, __sensorScenarios, true);
        _updateImageSettings();
    }

    if(!IS_STEREO_MODE(stereoMode)) {
        StereoTuningProviderKernel::destroyInstance();

        // MY_LOGD("Reset stereo setting");
        // StereoSettingProviderKernel::destroyInstance();
#if (1==VSDOF_SUPPORTED)
        FEFMSettingProvider::destroyInstance();
#endif
    }

    //Update FOV of sensor output
    if(g_currentSensorCombination &&
       g_sensorSettings.size() > 1)
    {
        MINT32 pipelineMode = (__isRecording)?PipelineMode_RECORDING:PipelineMode_ZSD;
        const bool needToUpdateVar = IS_TK_VSDOF_MODE(__stereoFeatureMode);
        int idx = 0;
        for(auto pSensorSetting : g_sensorSettings) {
            idx++;
            if(!pSensorSetting) {
                MY_LOGE("Main%d sensor setting is NULL", idx);
                continue;
            }

            pSensorSetting->updateSensorOutputFOV(__stereoFeatureMode, pipelineMode);
            if(needToUpdateVar) {
                pSensorSetting->updateModuleVariation(g_currentSensorCombination->moduleVariation,
                                                      StereoHAL::isVertical(StereoSettingProvider::getModuleRotation()));
                MY_LOGD("Main%d Sensor Output FOV: H/V: %.2f/%.2f(spec: %.2f/%.2f) Degrees, variation %f->%f",
                    idx,
                    radiansToDegree(pSensorSetting->fovHSensorOutput),
                    radiansToDegree(pSensorSetting->fovVSensorOutput),
                    radiansToDegree(pSensorSetting->fovHorizontal),
                    radiansToDegree(pSensorSetting->fovVertical),
                    radiansToDegree(g_currentSensorCombination->moduleVariation),
                    radiansToDegree(pSensorSetting->fovVariationSensorOutput));
            }
            else
            {
                MY_LOGD("Main%d Sensor Output FOV: H/V: %.2f/%.2f(spec: %.2f/%.2f) Degrees",
                    idx,
                    radiansToDegree(pSensorSetting->fovHSensorOutput),
                    radiansToDegree(pSensorSetting->fovVSensorOutput),
                    radiansToDegree(pSensorSetting->fovHorizontal),
                    radiansToDegree(pSensorSetting->fovVertical));
            }

        }

        float zoomRatio = property_get_int32("vendor.debug.camera.zoomRatio", 1);
        if(zoomRatio != 1) {
            zoomRatio = 10.0f/zoomRatio;
        }
        setPreviewCropRatio(zoomRatio);
    } else {
        MY_LOGE("Sensor combination is NULL, please call setLogicalDeviceID first");
    }
}

std::string
StereoSettingProvider::_getStereoFeatureModeString(int stereoMode)
{
    auto features = featureModeToStrings(stereoMode);
    if(features.size() > 0) {
        std::ostringstream oss;
        std::copy(features.begin(), features.end()-1, std::ostream_iterator<string>(oss, "+"));
        oss << features.back();

        if(stereoMode & E_STEREO_FEATURE_PORTRAIT_FLAG) {
            oss << "(portrait mode)";
        }

        return oss.str();
    }

    return "none";
}

bool
StereoSettingProvider::hasHWFE()
{
    static bool _hasHWFE = false;
    return _hasHWFE;
}

MUINT32
StereoSettingProvider::fefmBlockSize(const int FE_MODE)
{
    switch(FE_MODE)
    {
        case 0:
           return 32;
            break;
        case 1:
           return 16;
            break;
        case 2:
           return 8;
            break;
        default:
            break;
    }

    return 0;
}

bool
StereoSettingProvider::getStereoCameraFOV(SensorFOV &mainFOV, SensorFOV &main2FOV, MUINT32 logicalDeviceID)
{
    std::vector<StereoSensorSetting_T *> sensorSettings;
    if(logicalDeviceID == __logicalDeviceID) {
        sensorSettings = g_sensorSettings;
    } else {
        __getSensorSettings(logicalDeviceID, sensorSettings);
    }

    bool result = true;
    if(sensorSettings.size() > 1 &&
       NULL != sensorSettings[0] &&
       NULL != sensorSettings[1])
    {
        mainFOV.fov_horizontal  = radiansToDegree(sensorSettings[0]->fovHorizontal);
        mainFOV.fov_vertical    = radiansToDegree(sensorSettings[0]->fovVertical);
        main2FOV.fov_horizontal = radiansToDegree(sensorSettings[1]->fovHorizontal);
        main2FOV.fov_vertical   = radiansToDegree(sensorSettings[1]->fovVertical);
    }

    return result;
}

int
StereoSettingProvider::getModuleRotation(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination = g_currentSensorCombination;

    if(logicalDeviceID != __logicalDeviceID) {
        sensorCombination = __getSensorCombinationSetting(logicalDeviceID);
    }

    if(sensorCombination) {
        return sensorCombination->moduleRotation;
    }

    return 0;
}

ENUM_STEREO_SENSOR_RELATIVE_POSITION
StereoSettingProvider::getSensorRelativePosition(MUINT32 logicalDeviceID)
{
    int sensorRalation = 0;
    StereoSensorConbinationSetting_T *sensorCombination = g_currentSensorCombination;

    if(logicalDeviceID != __logicalDeviceID) {
        sensorCombination = __getSensorCombinationSetting(logicalDeviceID);
    }

    if(sensorCombination &&
       NULL != sensorCombination->sensorSettings[0])
    {
        if(1 == sensorCombination->sensorSettings[0]->staticInfo.facingDirection) {
            if(3 == sensorCombination->moduleType)
            {
                sensorRalation = 1;
            }
        } else {
            if(3 == sensorCombination->moduleType ||
               4 == sensorCombination->moduleType)
            {
                sensorRalation = 1;
            }
        }
    }

    return static_cast<ENUM_STEREO_SENSOR_RELATIVE_POSITION>(sensorRalation);
}

bool
StereoSettingProvider::isSensorAF(const int SENSOR_INDEX)
{
    bool isAF = false;
    IHal3A *pHal3A = MAKE_Hal3A(SENSOR_INDEX, LOG_TAG);
    if(NULL == pHal3A) {
        MY_LOGE("Cannot get 3A HAL of sensor %d", SENSOR_INDEX);
    } else {
        FeatureParam_T rFeatureParam;
        if(pHal3A->send3ACtrl(NS3Av3::E3ACtrl_GetSupportedInfo, (MUINTPTR)&rFeatureParam, 0)) {
            isAF = (rFeatureParam.u4MaxFocusAreaNum > 0);
        } else {
            MY_LOGW("Cannot query AF ability from 3A of sensor %d", SENSOR_INDEX);
        }

        pHal3A->destroyInstance(LOG_TAG);
        pHal3A = NULL;
    }

    MY_LOGD_IF(isLogEnabled(), "Sensor: %d, AF: %d", SENSOR_INDEX, isAF);

    return isAF;
}

bool
StereoSettingProvider::enableLog()
{
    return setProperty(PROPERTY_ENABLE_LOG, 1);
}

bool
StereoSettingProvider::enableLog(const char *LOG_PROPERTY_NAME)
{
    return setProperty(PROPERTY_ENABLE_LOG, 1) &&
           setProperty(LOG_PROPERTY_NAME, 1);
}

bool
StereoSettingProvider::disableLog()
{
    return setProperty(PROPERTY_ENABLE_LOG, 0);
}

bool
StereoSettingProvider::isLogEnabled()
{
    return (checkStereoProperty(PROPERTY_ENABLE_LOG) != 0);
}

bool
StereoSettingProvider::isLogEnabled(const char *LOG_PROPERTY_NAME)
{
    return isLogEnabled() && (checkStereoProperty(LOG_PROPERTY_NAME) != 0);
}

bool
StereoSettingProvider::isProfileLogEnabled()
{
    return isLogEnabled() || (checkStereoProperty(PROPERTY_ENABLE_PROFILE_LOG) != 0);
}

MUINT32
StereoSettingProvider::getExtraDataBufferSizeInBytes()
{
    return 8192;
}

MUINT32
StereoSettingProvider::getMaxWarpingMatrixBufferSizeInBytes()
{
    return 100 * sizeof(MFLOAT);
}

MUINT32
StereoSettingProvider::getMaxSceneInfoBufferSizeInBytes()
{
    return 15 * sizeof(MINT32);
}

float
StereoSettingProvider::getStereoBaseline(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination = g_currentSensorCombination;
    if(logicalDeviceID != __logicalDeviceID) {
        sensorCombination = __getSensorCombinationSetting(logicalDeviceID);
    }

    float baseline = 1.0f;
    if(sensorCombination) {
        baseline = sensorCombination->baseline;
    }

    return baseline;
}

MUINT
StereoSettingProvider::getSensorRawFormat(const int SENSOR_INDEX)
{
    // for dev
    if( SENSOR_INDEX != SENSOR_DEV_MAIN &&
        SENSOR_INDEX != SENSOR_DEV_SUB &&
        checkStereoProperty("vendor.STEREO.debug.main2Mono") == 1 )
    {
        MY_LOGD("force main2 to be MONO sensor");
        return SENSOR_RAW_MONO;
    }

    //Use cached result if ready
    StereoSensorSetting_T *sensorSetting = StereoSettingProviderKernel::getInstance()->getSensorSetting(SENSOR_INDEX);
    if(sensorSetting) {
        return sensorSetting->staticInfo.rawFmtType;
    }

    IHalSensorList *sensorList = MAKE_HalSensorList();
    if (NULL == sensorList) {
        MY_LOGE("Cannot get sensor list");
        return SENSOR_RAW_Bayer;
    }

    SensorStaticInfo sensorStaticInfo;
    ::memset(&sensorStaticInfo, 0, sizeof(SensorStaticInfo));
    int sensorDevIndex = sensorList->querySensorDevIdx(SENSOR_INDEX);
    sensorList->querySensorStaticInfo(sensorDevIndex, &sensorStaticInfo);

    return sensorStaticInfo.rawFmtType;
}

bool
StereoSettingProvider::isDeNoise()
{
#if (1 == STEREO_DENOISE_SUPPORTED)
    return (__stereoFeatureMode & E_STEREO_FEATURE_DENOISE);
#else
    return false;
#endif
}

bool
StereoSettingProvider::is3rdParty(ENUM_STEREO_SCENARIO scenario, MINT32 featureMode)
{
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY)
    {
        return true;
    }

    if(eSTEREO_SCENARIO_CAPTURE == scenario)
    {
         return (   (featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY)
                 || (featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_MULTI_CAM)
                );
    }

    return !IS_TK_VSDOF_MODE(featureMode);
}

bool
StereoSettingProvider::isBayerPlusMono()
{
    int32_t main1Idx, main2Idx;
    getStereoSensorIndex(main1Idx, main2Idx, __logicalDeviceID);
    return (SENSOR_RAW_MONO == getSensorRawFormat(main2Idx));
}

bool
StereoSettingProvider::isBMVSDoF()
{
    return (!isDeNoise() && isBayerPlusMono());
}

bool
StereoSettingProvider::isDualCamMode()
{
#if (1 == STEREO_CAMERA_SUPPORTED)
    MUINT32 supportedMode = (E_STEREO_FEATURE_CAPTURE|E_STEREO_FEATURE_THIRD_PARTY|E_STEREO_FEATURE_MULTI_CAM);
    #if (1 == STEREO_DENOISE_SUPPORTED)
        supportedMode |= E_STEREO_FEATURE_DENOISE;
    #endif
    #if (1 == VSDOF_SUPPORTED)
        supportedMode |= E_STEREO_FEATURE_VSDOF;
        supportedMode |= E_STEREO_FEATURE_MTK_DEPTHMAP;
    #endif
    #if (1 == DUAL_ZOOM_SUPPORTED)
        supportedMode |= E_DUALCAM_FEATURE_ZOOM;
    #endif

    return (__stereoFeatureMode & supportedMode);
#endif

    return false;
}

void
StereoSettingProvider::_updateImageSettings()
{
    for(auto &sensorSetting : g_sensorSettings)
    {
        auto imgoIter = sensorSetting->imgoYuvSize.find(__imageRatio);
        if(imgoIter != sensorSetting->imgoYuvSize.end()) {
            StereoSizeProvider::getInstance()->setIMGOYUVSize(sensorSetting->index, imgoIter->second);
        }

        auto rrzoIter = sensorSetting->rrzoYuvSize.find(__imageRatio);
        if(rrzoIter != sensorSetting->rrzoYuvSize.end()) {
            StereoSizeProvider::getInstance()->setRRZOYUVSize(sensorSetting->index, rrzoIter->second);
        }
    }

    if(NULL != g_currentSensorCombination &&
       g_currentSensorCombination->hasSizeConfig)
    {
        for(auto &m1 : g_currentSensorCombination->baseSize) {
            StereoSizeProvider::getInstance()->setCustomizedSize(m1.first, m1.second);
        }
    }
}

vector<float> EMPTY_LDC;
vector<float> &
StereoSettingProvider::getLDCTable()
{
    if(g_currentSensorCombination) {
        return g_currentSensorCombination->LDC;
    }

    return EMPTY_LDC;
}

bool
StereoSettingProvider::LDCEnabled()
{
    if(g_currentSensorCombination) {
        return g_currentSensorCombination->enableLDC;
    }

    return false;
}

CUST_FOV_CROP_T
StereoSettingProvider::getFOVCropSetting(ENUM_STEREO_SENSOR sensor)
{
    if(g_sensorSettings.size() < 2 ||
       NULL == g_sensorSettings[0] ||
       NULL == g_sensorSettings[1])
    {
        MY_LOGE("Sensor settings not found");
        return CUST_FOV_CROP_T();
    }

    if(NULL != g_currentSensorCombination &&
       g_currentSensorCombination->disableCrop)
    {
        return CUST_FOV_CROP_T();
    }

    auto &sensorSetting = (eSTEREO_SENSOR_MAIN1 == sensor)
                          ? g_sensorSettings[0]
                          : g_sensorSettings[1];

    auto it = sensorSetting->cropSettingMap.find(__previewCropRatio);
    if(it != sensorSetting->cropSettingMap.end() &&
       it->second.keepRatio < 1.0f)
    {
        return it->second.toCusCrop();
    }

    return CUST_FOV_CROP_T();
}

float
StereoSettingProvider::getStereoCameraFOVRatio(float previewCropRatio, MUINT32 logicalDeviceID)
{
    if(!IS_TK_VSDOF_MODE(__stereoFeatureMode)) {
        return 1.0f;
    }

    float fovRatio = 1.0f;
    std::vector<StereoSensorSetting_T *> sensorSettings = g_sensorSettings;
    StereoSensorConbinationSetting_T *sensorCombination = g_currentSensorCombination;
    if(logicalDeviceID == __logicalDeviceID) {
        sensorSettings = g_sensorSettings;
    } else {
        __getSensorSettingsAndCombination(logicalDeviceID, sensorSettings, sensorCombination);
    }

    if(sensorSettings.size() < 2 ||
       NULL == sensorSettings[0] ||
       NULL == sensorSettings[1] ||
       NULL == sensorCombination)
    {
        MY_LOGE("Sensor settings of logical device ID %d not found: %p %p, combination: %p",
                logicalDeviceID, sensorSettings[0], sensorSettings[1], sensorCombination);
        return 1.0f;
    }

    auto it = sensorSettings[0]->cropSettingMap.find(previewCropRatio);
    if( it == sensorSettings[0]->cropSettingMap.end() ) {
        FOVCropUtil::updateFOVCropRatios(*sensorCombination, previewCropRatio);
    }

    float main1FOV;
    float main2FOV;
    if(StereoHAL::isVertical(StereoSettingProvider::getModuleRotation())) {
        auto it = sensorSettings[0]->cropSettingMap.find(previewCropRatio);
        main1FOV = it->second.fovVRuntime;
        it = sensorSettings[1]->cropSettingMap.find(previewCropRatio);
        main2FOV = it->second.fovVRuntime;
    } else {
        it = sensorSettings[0]->cropSettingMap.find(previewCropRatio);
        main1FOV = it->second.fovHRuntime;
        it = sensorSettings[1]->cropSettingMap.find(previewCropRatio);
        main2FOV = it->second.fovHRuntime;
    }

    fovRatio = radiansRatio(main2FOV, main1FOV);

    return fovRatio;
}

float
StereoSettingProvider::getMain1FOVCropRatio(float previewCropRatio)
{
    if( !IS_TK_VSDOF_MODE(__stereoFeatureMode) )
    {
        return previewCropRatio;
    }

    if(NULL == g_currentSensorCombination ||
       (g_currentSensorCombination->disableCrop &&
        previewCropRatio == 1.0f))
    {
        return 1.0f;
    }

    if(g_sensorSettings.size() > 0 &&
       NULL != g_sensorSettings[0])
    {
        auto it = g_sensorSettings[0]->cropSettingMap.find(previewCropRatio);
        if(it == g_sensorSettings[0]->cropSettingMap.end()) {
            FOVCropUtil::updateFOVCropRatios(*g_currentSensorCombination, previewCropRatio);
        }

        it = g_sensorSettings[0]->cropSettingMap.find(previewCropRatio);
        return it->second.keepRatio;
    } else {
        MY_LOGE("Sensor setting of main1 not found");
    }

    return 1.0f;
}

float
StereoSettingProvider::getMain2FOVCropRatio(float previewCropRatio)
{
    if( !IS_TK_VSDOF_MODE(__stereoFeatureMode) )
    {
        return previewCropRatio;
    }

    if(NULL == g_currentSensorCombination ||
       (g_currentSensorCombination->disableCrop &&
        previewCropRatio == 1.0f))
    {
        return 1.0f;
    }

    if(g_sensorSettings.size() > 1 &&
       NULL != g_sensorSettings[1])
    {
        auto it = g_sensorSettings[1]->cropSettingMap.find(previewCropRatio);
        if(it == g_sensorSettings[1]->cropSettingMap.end()) {
            FOVCropUtil::updateFOVCropRatios(*g_currentSensorCombination, previewCropRatio);
        }

        it = g_sensorSettings[1]->cropSettingMap.find(previewCropRatio);
        return it->second.keepRatio;
    } else {
        MY_LOGE("Sensor setting of main1 not found");
    }

    return 1.0f;
}

EShotMode
StereoSettingProvider::getShotMode()
{
    return eShotMode_ZsdShot;
}

void
StereoSettingProvider::setStereoModuleType(MINT32 moduleType)
{
    __stereoModuleType = moduleType;
}

int
StereoSettingProvider::getStereoFeatureMode(MUINT32 openID)
{
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(!pHalDeviceList) {
        MY_LOGE("Cannot get logical device list");
        return 0;
    }

    int featureMode = 0;

    auto __isSensorOfTheLogicalDevice = [&](MUINT32 openId, MUINT32 logicalDeviceID)->bool
    {
        auto sensorIDs = pHalDeviceList->getSensorId(logicalDeviceID);
        if(std::find(sensorIDs.begin(), sensorIDs.end(), openId) != sensorIDs.end()) {
            return true;
        }

        return false;
    };

    if(openID == getLogicalDeviceID() ||
       __isSensorOfTheLogicalDevice(openID, getLogicalDeviceID()))
    {
        featureMode = __stereoFeatureMode;
    }

    // MY_LOGD("Feature mode for openID %d: %d", openID, featureMode);
    return featureMode;
}

size_t
StereoSettingProvider::getDPECaptureRound()
{
    int dpeRound = checkStereoProperty("vendor.STEREO.tuning.dpe_round", DPE_CAPTURE_ROUND);
    if(dpeRound < 0) {
        dpeRound = 0;
    }

    return (size_t)dpeRound;
}

void
StereoSettingProvider::setStereoShotMode(MUINT32 stereoShotMode)
{
    __stereoShotMode = stereoShotMode;
}

size_t
StereoSettingProvider::getMaxN3DDebugBufferSizeInBytes()
{
    return 1024*1024*3;
}

bool
StereoSettingProvider::isWidePlusTele(MUINT32 logicalDeviceID)
{
    std::vector<StereoSensorSetting_T *> sensorSettings;
    if(logicalDeviceID == __logicalDeviceID) {
        sensorSettings = g_sensorSettings;
    } else {
        __getSensorSettings(logicalDeviceID, sensorSettings);
    }

    bool result = false;
    if(sensorSettings.size() > 0 &&
       NULL != sensorSettings[0])
    {
        //W+T will exchange sensor index
        if(sensorSettings[0]->index != SENSOR_DEV_MAIN &&
           sensorSettings[0]->index != SENSOR_DEV_SUB)
        {
            return true;
        }
    }

    return result;
}

bool
StereoSettingProvider::isWideTeleVSDoF(MUINT32 logicalDeviceID)
{
    if(isWidePlusTele(logicalDeviceID) &&
       IS_STEREO_MODE(__stereoFeatureMode))
    {
        return true;
    }

    return false;
}

bool
StereoSettingProvider::getSensorScenario(MINT32 stereoMode,
                                         bool isRecording,
                                         MUINT &sensorScenarioMain1,
                                         MUINT &sensorScenarioMain2,
                                         bool updateToo)
{
    std::vector<MUINT> scenarios;
    bool isOK = getSensorScenarios(stereoMode, isRecording, scenarios, updateToo);
    if(isOK &&
       scenarios.size() > 1)
    {
        sensorScenarioMain1 = scenarios[0];
        sensorScenarioMain2 = scenarios[1];
    }
    else
    {
        sensorScenarioMain1 = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
        sensorScenarioMain2 = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
    }

    return isOK;
}

void
StereoSettingProvider::updateSensorScenario(MUINT sensorScenarioMain1,
                                            MUINT sensorScenarioMain2)
{
    __sensorScenarios.clear();
    __sensorScenarios.push_back(sensorScenarioMain1);
    __sensorScenarios.push_back(sensorScenarioMain2);
    MY_LOGD("Update sensor scenario: %d(%s) %d(%s)",
            __sensorScenarios[0], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[0]),
            __sensorScenarios[1], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[1]));
}

bool
StereoSettingProvider::getSensorScenarios(MINT32 stereoMode,
                                          bool isRecording,
                                          std::vector<MUINT> &sensorScenarios,
                                          bool updateToo)
{
    bool isOK = (g_sensorSettings.size() > 0);
    auto &src = (!isRecording) ? g_currentSensorCombination->sensorScenariosZSD
                               : g_currentSensorCombination->sensorScenariosRecord;
    if(src.size() == g_sensorSettings.size())
    {
        sensorScenarios.assign(src.begin(), src.end());
    }
    else
    {
        sensorScenarios.clear();
        MINT32 pipelineMode = (!isRecording) ? PipelineMode_ZSD : PipelineMode_RECORDING;
        const MUINT DEFAULT_SCENARIO = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
        for(auto &setting : g_sensorSettings) {
            if(setting) {
                MUINT sensorScenario = setting->getSensorScenario(stereoMode, pipelineMode);
                if(SENSOR_SCENARIO_ID_UNNAMED_START == sensorScenario) {
                    sensorScenario = DEFAULT_SCENARIO;
                    if(stereoMode)
                    {
                        isOK = false;
                        MY_LOGW("Cannot get sensor setting of main%zu, use %s as sensor scenario",
                                sensorScenarios.size()+1,
                                SENSOR_SCENARIO_TO_STRING_MAP.at(DEFAULT_SCENARIO));
                    }
                }
                sensorScenarios.push_back(sensorScenario);
            }
        }
    }

    if(isOK &&
       updateToo &&
       stereoMode != 0)
    {
        updateSensorScenarios(sensorScenarios);
        return true;
    }

    return isOK;
}

void
StereoSettingProvider::updateSensorScenarios(const std::vector<MUINT> &sensorScenarios)
{
    __sensorScenarios = sensorScenarios;

    const size_t SIZE = __sensorScenarios.size();
    if(SIZE > 2) {
        MY_LOGD("Update sensor scenarios(%zu): %d(%s) %d(%s) %d(%s)", SIZE,
                __sensorScenarios[0], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[0]),
                __sensorScenarios[1], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[1]),
                __sensorScenarios[2], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[2]));
    } else if(SIZE > 1) {
        MY_LOGD("Update sensor scenarios(2): %d(%s) %d(%s)",
                __sensorScenarios[0], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[0]),
                __sensorScenarios[1], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[1]));
    } else if(SIZE > 0) {
        MY_LOGD("Update sensor scenario(1): %d(%s)",
                __sensorScenarios[0], SENSOR_SCENARIO_TO_STRING_MAP.at(__sensorScenarios[0]));
    } else {
        MY_LOGD("Cannot find sensor scenario");
    }
}

MUINT
StereoSettingProvider::getSensorScenarioMain1()
{
    if(__sensorScenarios.size() > 0) {
        return __sensorScenarios[0];
    }

    MY_LOGW("Cannot get sensor scenario of main1, return CAPTURE as default");
    return SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
}

MUINT
StereoSettingProvider::getSensorScenarioMain2()
{
    if(__sensorScenarios.size() > 1) {
        return __sensorScenarios[1];
    }

    MY_LOGW("Cannot get sensor scenario of main2, return CAPTURE as default");
    return SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
}

MUINT
StereoSettingProvider::getCurrentSensorScenario(StereoHAL::ENUM_STEREO_SENSOR sensor)
{
    if(sensor > 0 &&
       sensor <= __sensorScenarios.size())
    {
        return __sensorScenarios[sensor-1];
    }

    MY_LOGE("Cannot get current sensor scenario of main%d, return CAPTURE as default", sensor);
    return SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
}

bool
StereoSettingProvider::getCalibrationDistance(ENUM_STEREO_SENSOR sensor, MUINT32 &macroDistance, MUINT32 &infiniteDistance)
{
    bool result = true;
    if(eSTEREO_SENSOR_MAIN1 == sensor) {
        if(g_sensorSettings.size() > 0 &&
           g_sensorSettings[0])
        {
            macroDistance    = g_sensorSettings[0]->distanceMacro;
            infiniteDistance = g_sensorSettings[0]->distanceInfinite;
            result = (macroDistance > 0 && infiniteDistance > 0);
        } else {
            macroDistance    = 100;
            infiniteDistance = 5000;
            result = false;
            MY_LOGW("Cannot get sensor setting of main1, use %d & %d for calibration distance", macroDistance, infiniteDistance);
        }
    } else {
        if(g_sensorSettings.size() > 1 &&
           g_sensorSettings[1])
        {
            macroDistance    = g_sensorSettings[1]->distanceMacro;
            infiniteDistance = g_sensorSettings[1]->distanceInfinite;
            result = (macroDistance > 0 && infiniteDistance > 0);
        } else {
            macroDistance    = 100;
            infiniteDistance = 5000;
            result = false;
            MY_LOGW("Cannot get sensor setting of main2, use %d & %d for calibration distance", macroDistance, infiniteDistance);
        }
    }

    return result;
}

std::string
StereoSettingProvider::getCallbackBufferList()
{
    char value[PROP_VALUE_MAX];
    std::string result = StereoSettingProviderKernel::getInstance()->getCallbackBufferListString();
    size_t len = ::property_get("vendor.STEREO.callback_list", value, NULL);
    if(len > 0) {
        result.assign(value);
    }

    if(0 == result.size()) {
        // ci: Clean Image, bi:Bokeh Image, mbd: MTK Bokeh Depth, mdb: MTK Debug Buffer, mbm: MTK Bokeh Metadata
        const char *DEFAULT_LIST = "bi";
        MY_LOGD_IF(__stereoFeatureMode != 0 && isLogEnabled(), "Callback list are not set, use default(%s)", DEFAULT_LIST);
        result.assign(DEFAULT_LIST);
    }

    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    MY_LOGD_IF((__stereoFeatureMode != 0 && isLogEnabled())||(len>0), "Callback list: %s", result.c_str());

    return result;
}

std::bitset<NSCam::v1::Stereo::CallbackBufferType::E_DUALCAM_JPEG_ENUM_SIZE>
StereoSettingProvider::getBokehJpegBufferList()
{
    std::string value = getCallbackBufferList();
    std::bitset<CallbackBufferType::E_DUALCAM_JPEG_ENUM_SIZE> result;
    result.reset();
    std::stringstream ss(value);
    std::string token;

    while(std::getline(ss, token, ','))
    {
        if(token.compare("ci") == 0)
        {
            result.set(E_DUALCAM_JPEG_CLEAN_IMAGE);
        }
        else if(token.compare("bi") == 0)
        {
            result.set(E_DUALCAM_JPEG_BOKEH_IMAGE);
        }
        else if(token.compare("mbd") == 0)
        {
            result.set(E_DUALCAM_JPEG_MTK_BOKEH_DEPTH);
        }
        else if(token.compare("mdb") == 0)
        {
            result.set(E_DUALCAM_JPEG_MTK_DEBUG_BUFFER);
        }
        else if(token.compare("mdm") == 0)
        {
            result.set(E_DUALCAM_JPEG_MTK_BOKEH_METADATA);
        }
        else if(token.compare("rbi") == 0)
        {
            result.set(E_DUALCAM_JPEG_RELIGHTING_BOKEH_IMAGE);
        }
        else
        {
            MY_LOGE("not support please check code consist or not. check target: stereo_setting, StereoCamEnum.");
        }
    }

    if(isLogEnabled())
    {
        string s = result.to_string();
        MY_LOGD("Callback list: %s(%s)", s.c_str(), value.c_str());
    }

    return result;
}

void
StereoSettingProvider::setPreviewSize(NSCam::MSize pvSize)
{
    if(pvSize.w == 0 || pvSize.h == 0) {
        // MY_LOGE("Invalid preview size: %dx%d", pvSize.w, pvSize.h);
        return;
    }

    MY_LOGD("Set preview size %dx%d", pvSize.w, pvSize.h);
    StereoSizeProvider::getInstance()->setPreviewSize(pvSize);
    setImageRatio(STEREO_IMAGE_RATIO_T(pvSize));

    auto it = g_logicalParamMap.find(getLogicalDeviceID());
    if(it != g_logicalParamMap.end()) {
        it->second.previewSize = pvSize;
    } else {
        MY_LOGW("Should set logical device ID & feature mofe before setPreviewSize");
    }
}

size_t
StereoSettingProvider::getCalibrationData(void *calibrationData, MUINT32 logicalDeviceID)
{
    if(NULL == calibrationData) {
        MY_LOGE("Invalid input calibration data address");
        return 0;
    }

    StereoSensorConbinationSetting_T *pSC = __getSensorCombinationSetting(logicalDeviceID);
    if(NULL == pSC ||
       0 == pSC->calibrationData.size() ||
       pSC->calibrationData.size() > CAM_CAL_Stereo_Data_SIZE)
    {
        MY_LOGD("Invalid offline calibration data");
        return 0;
    }

    ::memcpy(calibrationData, &(pSC->calibrationData[0]), pSC->calibrationData.size());
    MY_LOGD("return calibration data in %zu bytes", pSC->calibrationData.size());
    return pSC->calibrationData.size();
}

int
StereoSettingProvider::getISPProfileForP1YUV(int sensorId)
{
    int profile = StereoSettingProviderKernel::getInstance()->getISProfile(
                    sensorId,
                    (__isRecording) ? eSTEREO_SCENARIO_RECORD : eSTEREO_SCENARIO_PREVIEW);

    if(profile == -1)
    {
#if (1==HAS_P1YUV)
        profile = NSIspTuning::EIspProfile_P1_YUV_Depth;
#elif (2==HAS_P1YUV || 6==HAS_P1YUV)
        profile = (__isRecording)
                  ? NSIspTuning::EIspProfile_VSDOF_Video_Main_toW
                  : NSIspTuning::EIspProfile_VSDOF_PV_Main_toW;
#else
        profile = NSIspTuning::EIspProfile_N3D_Preview;
#endif
    }

    profile = ::property_get_int32("vendor.p1yuv.profile", profile);
    return profile;
}

int
StereoSettingProvider::getP1YUVSupported()
{
    int result = DirectYUVType::NO_P1_YUV;
#if (1==HAS_P1YUV)
    result = DirectYUVType::HAS_P1_YUV_CRZO;
#elif (2==HAS_P1YUV)
    result = DirectYUVType::HAS_P1_YUV_RSSO_R2;
#elif (6==HAS_P1YUV)
    result = DirectYUVType::HAS_P1_YUV_YUVO_RSSO_R2;
#endif
    return ::property_get_int32("ro.vendor.camera.directscaledyuv.support", result);
}

NSCam::v3::P1Node::RESIZE_QUALITY
StereoSettingProvider::getVSDoFP1ResizeQuality()
{
    NSCam::v3::P1Node::RESIZE_QUALITY quality = NSCam::v3::P1Node::RESIZE_QUALITY_H;
// #if (1==HAS_P1_RESIZE_QUALITY)
//     if(IS_STEREO_MODE(__stereoFeatureMode) &&
//        NULL != g_currentSensorCombination &&
//        NULL != g_currentSensorCombination->sensorSettings[0])
//     {
//         NSCam::SensorStaticInfo &staticInfo =g_currentSensorCombination->sensorSettings[0]->staticInfo;
//         MY_LOGD("Feature mode: %s, sensor %dx%d",
//                 _getStereoFeatureModeString(__stereoFeatureMode).c_str(),
//                 staticInfo.captureWidth, staticInfo.captureHeight);
//         if(staticInfo.captureWidth * staticInfo.captureHeight > 17000000)
//         {
//             quality = NSCam::v3::P1Node::RESIZE_QUALITY_L;
//         }
//     }
// #endif

    quality = static_cast<NSCam::v3::P1Node::RESIZE_QUALITY>(::property_get_int32("vendor.STEREO.p1resizequality", quality));
    MY_LOGD("P1 resize quality: %s", (NSCam::v3::P1Node::RESIZE_QUALITY_H==quality)?"High":"Low");
    return quality;
}

bool
StereoSettingProvider::getSensorFOV(const int SENSOR_INDEX, float &fovHorizontal, float &fovVertical)
{
    StereoSensorSetting_T *setting = StereoSettingProviderKernel::getInstance()->getSensorSetting(SENSOR_INDEX);
    if(setting)
    {
        fovHorizontal = radiansToDegree(setting->fovHorizontal);
        fovVertical   = radiansToDegree(setting->fovVertical);
        return true;
    }

    return false;
}

MTK_DEPTHMAP_INFO_T
StereoSettingProvider::getDepthmapInfo(STEREO_IMAGE_RATIO_T ratio, ENUM_STEREO_SCENARIO eScenario)
{
    MTK_DEPTHMAP_INFO_T result;
    result.size = StereoSizeProvider::getInstance()->thirdPartyDepthmapSize(ratio, eScenario);
    result.stride = result.size.w;
    auto pKernel = StereoSettingProviderKernel::getInstance();
    if(pKernel) {
        result.format = pKernel->getDepthmapFormat();
    }

    /**
     * Y8 is a YUV planar format comprised of a WxH Y plane, with each pixel
     * being represented by 8 bits. It is equivalent to just the Y plane from
     * YV12.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     */

    /**
     * Y16 is a YUV planar format comprised of a WxH Y plane, with each pixel
     * being represented by 16 bits. It is just like Y8, but has double the
     * bits per pixel (little endian).
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     * - strides are specified in pixels, not in bytes
     */
    StereoHAL::applyNAlign(2,  result.size.w);
    StereoHAL::applyNAlign(2,  result.size.h);
    StereoHAL::applyNAlign(16, result.stride);

    return result;
}

MUINT32
StereoSettingProvider::getMain2OutputFrequency(ENUM_STEREO_SCENARIO scenario)
{
    MUINT32 freq = 1;
    if(is3rdParty()) {
        auto pKernel = StereoSettingProviderKernel::getInstance();
        if(pKernel) {
            freq = pKernel->getMain2OutputFrequecy(scenario);
        }
        MY_LOGD("Main2 output frequency: %u(%s)", freq,
                (scenario==eSTEREO_SCENARIO_CAPTURE) ? "Capture" : "Preview");
    }

    return freq;
}

void
StereoSettingProvider::set3rdDepthAlgoRunning(bool isDepthRunning)
{
    __3rdCaptureDepthRunning = isDepthRunning;
}

void
StereoSettingProvider::set3rdBokehAlgoRunning(bool isBokehRunning)
{
    __3rdCaptureBokehRunning = isBokehRunning;
}

bool
StereoSettingProvider::get3rdDepthOrBokehAlgoRunning()
{
    return (__3rdCaptureDepthRunning || __3rdCaptureBokehRunning);
}

bool
StereoSettingProvider::isAFSyncNeeded(MUINT32 logicalDeviceID)
{
    bool isNeeded = false;
    // if(IS_STEREO_MODE(__stereoFeatureMode))
    {
        int sensorIndex[2];
        getStereoSensorIndex(sensorIndex[0], sensorIndex[1], logicalDeviceID);
        //For most of back dual cam, main2 sensor is usually FF, so we check main2 first
        isNeeded = (isSensorAF(sensorIndex[1]) && isSensorAF(sensorIndex[0]));
        MY_LOGD_IF(isLogEnabled(), "Logical device %d, Feature %s, need AF sync %d",
                    logicalDeviceID, _getStereoFeatureModeString(__stereoFeatureMode).c_str(),
                    isNeeded)
    }
    return isNeeded;
}

int // ENUM_DEPTHMAP_REFINE_LEVEL
StereoSettingProvider::getDepthmapRefineLevel(ENUM_STEREO_SCENARIO eScenario)
{
    if(StereoSettingProvider::isMTKDepthmapMode()) {
        if(g_currentSensorCombination) {
            if(eScenario == eSTEREO_SCENARIO_UNKNOWN) {
                eScenario = (isRecording()) ? eSTEREO_SCENARIO_RECORD : eSTEREO_SCENARIO_PREVIEW;
            }

            auto it = g_currentSensorCombination->depthmapRefineLevel.find(eScenario);
            if (it != g_currentSensorCombination->depthmapRefineLevel.end()) {
                return it->second;
            }
        }
    }

    return E_DEPTHMAP_REFINE_DEFAULT;
}

std::vector<float>
StereoSettingProvider::getMulticamZoomRange(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination =
        (logicalDeviceID == __logicalDeviceID)
        ? g_currentSensorCombination
        : __getSensorCombinationSetting(logicalDeviceID);

    if(sensorCombination) {
        return sensorCombination->multicamZoomRanges;
    }

    return {1.0f};
}

std::vector<float>
StereoSettingProvider::getMulticamZoomSteps(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination =
        (logicalDeviceID == __logicalDeviceID)
        ? g_currentSensorCombination
        : __getSensorCombinationSetting(logicalDeviceID);

    if(sensorCombination) {
        return sensorCombination->multicamZoomSteps;
    }

    return {1.0f};
}

float
StereoSettingProvider::getFRZRatio(int32_t sensorIndex, int featureMode)
{
    float frzRatio = -1;

    StereoSensorSetting_T *sensorSetting = StereoSettingProviderKernel::getInstance()->getSensorSetting(sensorIndex);
    if(sensorSetting)
    {
        auto it = sensorSetting->frzRatioSetting.find(featureMode);
        if(it != sensorSetting->frzRatioSetting.end())
        {
            frzRatio = it->second;
        }
    }

    if(frzRatio < 0 ||
       frzRatio > 1.0f)
    {
        frzRatio = 0.5f;
        MY_LOGD("Unknown FRZ ratio in setting for sensor %d, feature %s, use %.1f",
                sensorIndex, _getStereoFeatureModeString(__stereoFeatureMode).c_str(), frzRatio);
    }

    return frzRatio;
}

int32_t
StereoSettingProvider::sensorEnumToId(ENUM_STEREO_SENSOR sensor, int logicalDeviceID)
{
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(NULL == pHalDeviceList) {
        MY_LOGE("Cannot get sensor list");
        return -1;
    }

    auto sensorIDs = pHalDeviceList->getSensorId(logicalDeviceID);
    if(sensor <= sensorIDs.size()) {
        return sensorIDs[sensor-1];
    }
    else
    {
        MY_LOGE("Invalid stereo sensor index %d, sensor count of logical device %d: %zu",
                sensor, logicalDeviceID, sensorIDs.size());
        return -1;
    }
}

ENUM_STEREO_SENSOR
StereoSettingProvider::sensorIdToEnum(const int32_t SENSOR_INDEX, int logicalDeviceID)
{
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(NULL == pHalDeviceList) {
        MY_LOGE("Cannot get sensor list");
        return eSTEREO_SENSOR_UNKNOWN;
    }

    auto sensorIDs = pHalDeviceList->getSensorId(logicalDeviceID);
    int index = 1;
    for(auto &id : sensorIDs) {
        if(id == SENSOR_INDEX) {
            return static_cast<ENUM_STEREO_SENSOR>(index);
        }
        ++index;
    }

    MY_LOGE("Sensor index %d is not found in logical device %d",
            SENSOR_INDEX, logicalDeviceID);
    return eSTEREO_SENSOR_UNKNOWN;
}

std::vector<float>
StereoSettingProvider::getLensPoseTranslation(int32_t sensorId)
{
    return StereoSettingProviderKernel::getInstance()->getLensPoseTranslation(sensorId);
}

MUINT32
StereoSettingProvider::getEISFactor()
{
    return __eisFactor;
}

bool
StereoSettingProvider::isMulticamRecordEnabled(int logicalDeviceID)
{
    return StereoSettingProviderKernel::getInstance()->isMulticamRecordEnabled(logicalDeviceID);
}

std::unordered_map<int, int>
StereoSettingProvider::getExclusiveGroup(int logicalDeviceID)
{
    return StereoSettingProviderKernel::getInstance()->getExclusiveGroup(logicalDeviceID);
}

bool
StereoSettingProvider::isBokehBlendingEnabled(StereoHAL::ENUM_STEREO_SCENARIO scenario)
{
    bool result = false;
    MUINT32 coreNumber;
    std::vector<MUINT8> alphaTable;
    TUNING_PAIR_LIST_T tuningParams;
    StereoTuningProvider::getBlendingTuningInfo(coreNumber, alphaTable, tuningParams, scenario);
    for(auto &param : tuningParams)
    {
        if(param.first == "blending.en")
        {
            result = param.second;
            break;
        }
    }

    return property_get_int32("vendor.tkflow.bokeh.swblending", result);
}

int
StereoSettingProvider::getMtkDepthFlow(int logicalDeviceID)
{
    // VR is always TK flow for now
    if (isRecording()) {
        return 0;
    }
    int depthFlow;

    StereoSensorConbinationSetting_T *sensorCombination =
        (logicalDeviceID == __logicalDeviceID)
        ? g_currentSensorCombination
        : __getSensorCombinationSetting(logicalDeviceID);

    if(sensorCombination) {
        depthFlow = sensorCombination->depthFlow;
    } else {
        depthFlow = getStereoModeType();
    }
    // override with adb command
    depthFlow = property_get_int32("vendor.camera.stereo.mode", depthFlow);
    if (depthFlow != 1 && (getStereoModeType() == 1)) {
      MY_LOGW("Due to platform only support pure 3rd party force depthFlow to 1");
      depthFlow = 1;
    }
    MY_LOGD("Depth Flow: %d(logical device %d", depthFlow, logicalDeviceID);
    return depthFlow;
}

bool
StereoSettingProvider::setSensorPhysicalInfo(int logicalDeviceID, SENSOR_PHYSICAL_INFO_T info)
{
    return StereoSettingProviderKernel::getInstance()->setSensorPhysicalInfo(logicalDeviceID, info);
}

// bool
// StereoSettingProvider::getSensorPhysicalInfo(int logicalDeviceID, StereoHAL::SENSOR_PHYSICAL_INFO_T &info)
// {
//     return StereoSettingProviderKernel::getInstance()->getSensorPhysicalInfo(logicalDeviceID, info);
// }

bool
StereoSettingProvider::isDepthDelayEnabled(int logicalDeviceID)
{
    int flow  = getMtkDepthFlow(logicalDeviceID);
    int delay = StereoSettingProviderKernel::getInstance()->getDepthDelay(logicalDeviceID);
    bool result = property_get_int32("vendor.depthmap.pipe.enableDepthDelay", (flow==0) && (delay>0));
    MY_LOGD("Depth delay for cam %d: %s", logicalDeviceID, (result)?"Enabled":"Disabled");
    return result;
}

bool
StereoSettingProvider::isSlantCameraModule(int logicalDeviceID)
{
    return StereoHAL::isSlant(getModuleRotation(logicalDeviceID));
}

size_t
StereoSettingProvider::getDepthFreq(ENUM_STEREO_SCENARIO scenario, MUINT32 logicalDeviceID)
{
    size_t freq = StereoSettingProviderKernel::getInstance()->getDepthFreq(scenario, logicalDeviceID);
    freq = static_cast<size_t>(property_get_int32("vendor.STEREO.depth_freq", freq));
    MY_LOGD("Depth freq for cam %d for %s: %zu", logicalDeviceID, (scenario==eSTEREO_SCENARIO_PREVIEW)?"Preview":"Record", freq);
    return freq;
}

void
StereoSettingProvider::setPreviewCropRatio(float ratio)
{
    if(__previewCropRatio == ratio ||
       ratio > 1.0f ||
       ratio <= 0.0f) {
        return;
    }

    MY_LOGD("Set preview crop ratio to %f", ratio);
    __previewCropRatio = ratio;
    if(g_currentSensorCombination &&
       g_sensorSettings.size() > 1 &&
       IS_TK_VSDOF_MODE(__stereoFeatureMode) &&
       FOVCropUtil::updateFOVCropRatios(*g_currentSensorCombination, ratio))
    {
        for(int idx = 0; idx < 2; ++idx) {
            if(!g_sensorSettings[idx]) {
                MY_LOGE("Main%d is null", idx+1);
                continue;
            }

            auto it = g_sensorSettings[idx]->cropSettingMap.find(ratio);
            if(it != g_sensorSettings[idx]->cropSettingMap.end())
            {
                MY_LOGD("Main%d FOV Crop: Keep ratio %.4f, Runtime FOV(H/V): %.2f/%.2f Degrees, Baseline Ratio %.4f",
                        idx+1,
                        it->second.keepRatio,
                        radiansToDegree(it->second.fovHRuntime),
                        radiansToDegree(it->second.fovVRuntime),
                        it->second.baselineRatio);
            } else {
                MY_LOGE("Crop setting is not found for main%d of ratio %f", idx+1, ratio);
            }
        }
    }
}

float
StereoSettingProvider::getVSDoFMinPass1CropRatio()
{
    ENUM_STEREO_SCENARIO scenario = (__isRecording)
                                    ? eSTEREO_SCENARIO_RECORD
                                    : eSTEREO_SCENARIO_PREVIEW;
    // Main1
    MUINT32 junkStride;
    MSize   szRRZO;
    MRect   tgCropRect;
    StereoSizeProvider::getInstance()->getPass1Size( {  eSTEREO_SENSOR_MAIN1,
                                                        eImgFmt_FG_BAYER10,
                                                        EPortIndex_RRZO,
                                                        scenario,
                                                        1.0f,
                                                     },
                                                     //below are outputs
                                                     tgCropRect,
                                                     szRRZO,
                                                     junkStride);
    float main1Ratio = std::max(static_cast<float>(szRRZO.w)/tgCropRect.s.w,
                                static_cast<float>(szRRZO.h)/tgCropRect.s.h);

    // Main2
    StereoSizeProvider::getInstance()->getPass1Size( {  eSTEREO_SENSOR_MAIN2,
                                                        eImgFmt_FG_BAYER10,
                                                        EPortIndex_RRZO,
                                                        scenario,
                                                        1.0f,
                                                     },
                                                     //below are outputs
                                                     tgCropRect,
                                                     szRRZO,
                                                     junkStride);
    float main2Ratio = std::max(static_cast<float>(szRRZO.w)/tgCropRect.s.w,
                                static_cast<float>(szRRZO.h)/tgCropRect.s.h);

    return std::max(main1Ratio, main2Ratio);
}

int
StereoSettingProvider::getFrameSyncType(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination =
        (logicalDeviceID == __logicalDeviceID)
        ? g_currentSensorCombination
        : __getSensorCombinationSetting(logicalDeviceID);

    int frameSyncType = E_FRAME_SYNC_DEFAULT;
    if(sensorCombination) {
        frameSyncType = sensorCombination->frameSyncType;
    }

    MY_LOGD("Device %d frame sync type: %d(%s)",
            logicalDeviceID,
            frameSyncType,
            (frameSyncType==0)?"Vsync align":"Read out center align");

    return frameSyncType;
}

bool
StereoSettingProvider::isHIDL(MUINT32 logicalDeviceID)
{
    StereoSensorConbinationSetting_T *sensorCombination =
        (logicalDeviceID == __logicalDeviceID)
        ? g_currentSensorCombination
        : __getSensorCombinationSetting(logicalDeviceID);

    if(sensorCombination) {
        return sensorCombination->logicalDeviceFeatureSet & E_MULTICAM_FEATURE_HIDL_FLAG;
    }

    return false;
}

bool
StereoSettingProvider::__initTuningIfReady()
{
    IHalSensorList *sensorList = MAKE_HalSensorList();
    const MUINT32 SENSOR_COUNT = (sensorList) ? sensorList->queryNumberOfSensors() : 3;
    if(__logicalDeviceID < SENSOR_COUNT) {
        MY_LOGD("Wait to set logical device, current %d", __logicalDeviceID);
        return false;
    }

    if(0 == __imageRatio) {
        MY_LOGD("Wait to set image ratio");
        return false;
    }

    if(!IS_STEREO_MODE(__stereoFeatureMode))
    {
        MY_LOGD("Wait to set stereo feature mode, current: %s",
                _getStereoFeatureModeString(__stereoFeatureMode).c_str());
        return false;
    }

    StereoTuningProviderKernel::getInstance()->init();  //Do init and loading
    return true;
}
