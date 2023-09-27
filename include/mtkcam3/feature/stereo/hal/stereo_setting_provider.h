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
#ifndef _STEREO_SETTING_PROVIDER_H_
#define _STEREO_SETTING_PROVIDER_H_

#include <mtkcam/def/common.h>
#include <mtkcam/utils/std/common.h>                // For property
#include <string>
#include <camera_custom_stereo.h>
#include <mtkcam3/feature/stereo/StereoCamEnum.h>
#include "stereo_common.h"
#include <ctime>
#include <sstream>
#include <mtkcam3/pipeline/hwnode/P1Node.h>
#include <unordered_map>

//NOTICE: property has 31 characters limitation
#define PROPERTY_ENABLE_LOG         STEREO_PROPERTY_PREFIX"log"
#define PROPERTY_ENABLE_DUMP        STEREO_PROPERTY_PREFIX"dump"
#define PERPERTY_PASS1_LOG          PROPERTY_ENABLE_LOG".Pass1"
#define PERPERTY_DEPTHMAP_NODE_LOG  PROPERTY_ENABLE_LOG".DepthMapNode"
#define PERPERTY_BOKEH_NODE_LOG     PROPERTY_ENABLE_LOG".BokehNode"
#define PERPERTY_BMDENOISE_NODE_LOG PROPERTY_ENABLE_LOG".BMDenoiseNode"
#define PERPERTY_DCMF_NODE_LOG      PROPERTY_ENABLE_LOG".DualCamMFNode"
#define PERPERTY_DIT_NODE_LOG       PROPERTY_ENABLE_LOG".DITNode"
#define PERPERTY_JENC_NODE_LOG      PROPERTY_ENABLE_LOG".JpgEncNode"
#define PERPERTY_VENC_NODE_LOG      PROPERTY_ENABLE_LOG".VEncNode"
#define PROPERTY_ENABLE_PROFILE_LOG PROPERTY_ENABLE_LOG".Profile"

/**************************************************************************
 *                      D E F I N E S / M A C R O S                       *
 **************************************************************************/

/**************************************************************************
 *     E N U M / S T R U C T / T Y P E D E F    D E C L A R A T I O N     *
 **************************************************************************/
typedef MINT32 N3D_SCENEINFO_TYPE;

enum {
    PipelineMode_PREVIEW,
    PipelineMode_RECORDING,
    PipelineMode_ZSD
};

struct STEREO_PARAMS_T
{
    std::string jpsSize;          // stereo picture size
    std::string jpsSizesStr;      // supported stereo picture size
    std::string refocusSize;      // refocus picture size
    std::string refocusSizesStr;  // supported refocus picture size
    std::string postviewSizesStr; // supported postview size
    std::string depthmapSizeStr;  // supported depthmap size
    int n3dSizes;      // n3d size
    int extraSizes;    // extra size
};

enum ENUM_DEPTHMAP_REFINE_LEVEL
{
    E_DEPTHMAP_REFINE_NONE,         //0: DVS(OCC)
    E_DEPTHMAP_REFINE_SW_OPTIMIZED, //1: GF
    E_DEPTHMAP_REFINE_HOLE_FILLED,  //2: DVP
    E_DEPTHMAP_REFINE_AI_DEPTH,     //3: AI Depth
    //=== Add more enum before this line ===
    E_DEPTHMAP_REFINE_MAX,
    E_DEPTHMAP_REFINE_DEFAULT = E_DEPTHMAP_REFINE_NONE,
};

enum ENUM_FRAME_SYNC_TYPE
{
    E_VSYNC_ALIGN,
    E_READ_OUT_CENTER_ALIGN,
    //=== Add more enum before this line ===
    E_FRAME_SYNC_MAX,
    E_FRAME_SYNC_DEFAULT = E_VSYNC_ALIGN,
};

#if (1 == HAS_WPE)
//=====================
// WPE buffer
//=====================
class WARPING_BUFFER_CONFIG_T
{
public:
    static const size_t MAX_WARPING_BUFFER_PLANE = 3;

    NSCam::MSize  BUFFER_SIZE;   //max: 640x480
    MUINT32       PLANE_COUNT;   //2~3

    WARPING_BUFFER_CONFIG_T(NSCam::MSize s=NSCam::MSize(0, 0), MUINT32 p=0)
        : BUFFER_SIZE(s)
        , PLANE_COUNT(p)
    {
    }

    WARPING_BUFFER_CONFIG_T(const WARPING_BUFFER_CONFIG_T &config)
        : BUFFER_SIZE(config.BUFFER_SIZE)
        , PLANE_COUNT(config.PLANE_COUNT)
    {
    }

    inline size_t getPlaneSizeInBytes() const {
        return (BUFFER_SIZE.w * BUFFER_SIZE.h * sizeof(MUINT32));
    }
};

struct WARPING_BUFFER_T
{
    const WARPING_BUFFER_CONFIG_T CONFIG;
    android::sp<NSCam::IImageBuffer>    planeBuffer[3];

    WARPING_BUFFER_T(WARPING_BUFFER_CONFIG_T &config);
    ~WARPING_BUFFER_T();
};
#endif

struct MTK_DEPTHMAP_INFO_T
{
    NSCam::EImageFormat format = NSCam::eImgFmt_Y8;
    NSCam::MSize        size   = NSCam::MSize(1920, 1080);
    int                 stride = size.w;
};

struct MULTICAM_LOGICAL_DEVICE_PARAM
{
    int32_t logicalDeviceID = 0;
    MINT32  stereoMode      = 0;
    NSCam::MSize   previewSize;
    bool    isPortrait      = false;
    bool    isRecording     = false;
    MUINT32 eisFactor       = 100;

    // MULTICAM_LOGICAL_DEVICE_PARAM() {}

    MULTICAM_LOGICAL_DEVICE_PARAM(int32_t id,
                                  MINT32 mode,
                                  NSCam::MSize pvSize = NSCam::MSize(),
                                  bool portrait  = false,
                                  bool recording = false,
                                  MUINT32 eis    = 100)
    {
        logicalDeviceID = id;
        stereoMode      = mode;
        previewSize     = pvSize;
        isPortrait      = portrait;
        isRecording     = recording;
        eisFactor       = eis;
    }
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *        P U B L I C    F U N C T I O N    D E C L A R A T I O N         *
 **************************************************************************/

/**************************************************************************
 *                   C L A S S    D E C L A R A T I O N                   *
 **************************************************************************/

/**
 * \brief This class provides static stereo settings
 * \details Make sure you have turned on sensors before using any sensor related API
 *
 */
class StereoSettingProvider
{
public:
    /**
     * \brief Get stereo sensor index
     * \details In most of the time, main1 index is 0, main2 index is 2.
     *
     * \param  main1 sensor index
     * \param  main2 sensor index
     * \param  stereo profile
     *
     * \return true if successfully get both index; otherwise false
     */
    static bool getStereoSensorIndex(int32_t &main1Idx, int32_t &main2Idx, MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get sensor device index
     * \details In most of the time, main1 index is 1, main2 index is 4.
     *
     * \param  main1 Saves the device index of main stereo sensor
     * \param  main2 Saves the device index of another stereo sensor
     * \param  stereo profile
     *
     * \return true if successfully get both index; otherwise false
     */
    static bool getStereoSensorDevIndex(int32_t &main1DevIdx, int32_t &main2DevIdx, MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get image ratio
     *
     * \return Ratio of image
     */
    static const StereoHAL::STEREO_IMAGE_RATIO_T &imageRatio() { return __imageRatio; }

    /**
     * \brief Get De-Noise setting
     * \return True if running de-noise feature
     */
    static bool isDeNoise();

    /**
     * \brief Get 3rd-Party setting
     * \return True if running de-noise feature
     */
    static bool is3rdParty(StereoHAL::ENUM_STEREO_SCENARIO scenario=StereoHAL::eSTEREO_SCENARIO_PREVIEW, MINT32 featureMode=__stereoFeatureMode);

    /**
     * \brief Check if feature mode is to provide MTK depthmap
     * \return true if feature mode is MTK depthmap mode
     */
    static bool isMTKDepthmapMode(MINT32 featureMode=__stereoFeatureMode) { return (featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP); }

    /**
     * \brief Check if feature mode is active stereo
     * \return true if feature mode is active stereo
     */
    static bool isActiveStereo(MINT32 featureMode=__stereoFeatureMode) { return (featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_ACTIVE_STEREO); }

    /**
     * \brief Get stereo profile
     * \details There could be more than one set of stereo camera on the device,
     *          different profile has different sensor id, baseline, FOV, sizes, etc.
     * \return Stereo profile
     */
    static ENUM_STEREO_SENSOR_PROFILE stereoProfile(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Check if the device has hardware feature extraction component
     * \return true if the device has hardware feature extraction component
     */
    static bool hasHWFE();

    /**
     * \brief Get FE/FM block size
     * \details Block sizes are different in each FE/FM stage
     *
     * \param FE_MODE Block size of mode 0: 32, mode 1: 16, mode 2: 8
     * \return Block size
     */
    static MUINT32 fefmBlockSize(const int FE_MODE);

    /**
     * \brief Get LDC table of current profile
     * \return LDC table
     */
    static std::vector<float> &getLDCTable();

    /**
     * \brief Enable LDC or not
     * \return true if enable LDC
     */
    static bool LDCEnabled();

    /**
     * \brief Get FOV crop setting of current profile
     *
     * \param Sensor to query
     * \return FOV crop setting
     */
    static CUST_FOV_CROP_T getFOVCropSetting(StereoHAL::ENUM_STEREO_SENSOR sensor);

    /**
     * \brief Get calibration distance of sensor
     * \details Calibration distances came from two sources: calibration driver or custom setting
     *          User can customize this field in camera_custom_stereo_setting.h, e.g.
     *          "\"Calibration\": {"
     *              "\"Macro Distance\": 100,"
     *              "\"Infinite Distance\": 5000"
     *          "}"
     *
     * \param sensor Sensor to query
     * \param macroDistance Calibration distance of macro
     * \param infiniteDistance Calibration distance of infinite
     * \return true if success
     */
    static bool getCalibrationDistance(StereoHAL::ENUM_STEREO_SENSOR sensor, MUINT32 &macroDistance, MUINT32 &infiniteDistance);

    /**
     * \brief Get FOV(field of view) of stereo sensors
     *
     * \param mainFOV FOV of main1 sensor
     * \param main2FOV FOV of main2 sensor
     * \param profile Profile of stereo camera, can be rear-rear or front-front
     *
     * \return true if success
     * \see getStereoFOV
     */
    static bool getStereoCameraFOV(SensorFOV &mainFOV, SensorFOV &main2FOV, MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get FOV ratio of main1/main2
     *
     * \param previewCropRatio the preview crop ratio to query
     * \param logicalDeviceID logical device to query
     *
     * \return FOV ratio
     */
    static float getStereoCameraFOVRatio(float previewCropRatio, MUINT32 logicalDeviceID=getLogicalDeviceID());  //main2_fov / main1_fov

    /**
     * \brief Get module rotation of current stereo profile
     * \details User needs to assign stereo profile first
     *
     * \param profile Profile of stereo camera, can be rear-rear or front-front
     *
     * \return Module degree clockwised
     */
    static int getModuleRotation(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get sensor relative position
     * \return 0: main-main2 (main in L)
     *         1: main2-main (main in R)
     */
    static ENUM_STEREO_SENSOR_RELATIVE_POSITION getSensorRelativePosition(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Query if the sensor is AF or FF
     *
     * \param SENSOR_INDEX sensor index
     * \return true if the sensor is AF
     */
    static bool isSensorAF(const int SENSOR_INDEX);

    /**
     * \brief Enable stereo log, each node can decide to log or not
     * \details This equals setprop debug.STEREO.log 1
     * \return true if success
     * \see disableLog()
     */
    static bool enableLog();

    /**
     * \brief Enable log by a property
     * \details This equals setprop LOG_PROPERTY_NAME 1
     *
     * \param LOG_PROPERTY_NAME Log property to enable
     * \return true if success
     */
    static bool enableLog(const char *LOG_PROPERTY_NAME);

    /**
     * \brief Disable stereo log, each node can decide to log or not
     * \details This equals setprop debug.STEREO.log 1
     * \return true if success
     * \see enableLog()
     */
    static bool disableLog();   //Globally disable log

    /**
     * \brief Check if global log is enabled
     * \details This equals getprop debug.STEREO.log
     * \return true if log is enabled
     */
    static bool isLogEnabled();

    /**
     * \brief Check if a log property is enabled or not
     * \details This equals getprop LOG_PROPERTY_NAME
     *
     * \param LOG_PROPERTY_NAME The log property to check
     * \return true if log is enabled
     */
    static bool isLogEnabled(const char *LOG_PROPERTY_NAME);   //Check log status of each node, refers to global log switch

    /**
     * \brief Check is profile log is enabled or not
     * \details This equals getprop debug.STEREO.Profile
     * \return true if profile log is enabled
     */
    static bool isProfileLogEnabled(); //Check if global profile log is enabled

    /**
     * \brief Get maximum extra data buffer size
     * \details We provides this API for AP to pass extra data buffer to middleware,
     *          NOTICE: if we need to add new data to extra data, or change current data size,
     *                  we need to review the returned size again.
     * \return Estimated size of extra data
     */
    static MUINT32 getExtraDataBufferSizeInBytes();

    /**
     * \brief Get MAX size of warping matrix output by N3D HAL
     * \details This size is for each sensor
     * \return Maxium warping matrix in bytes
     */
    static MUINT32 getMaxWarpingMatrixBufferSizeInBytes();

    /**
     * \brief Get MAX size of Scene Info output by N3D HAL
     * \details This size is for each sensor
     * \return Maxium B+M High Resolution in bytes
     */
    static MUINT32 getMaxSceneInfoBufferSizeInBytes();

    /**
     * \brief Get baseline of current profile, unit: cm
     * \details Should set stereo profile before using this API
     *
     * \param profile profile to get baseline
     * \return baseline of current profile, unit: cm
     */
    static float getStereoBaseline(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get sensor output format in raw domain
     * \details User should power on the sensor before using this API
     * \param SENSOR_INDEX Sensor index, 0 for main sensor
     * \return enum
     *         {
     *             SENSOR_RAW_Bayer = 0x0,
     *             SENSOR_RAW_MONO,
     *             SENSOR_RAW_RWB,
     *             SENSOR_RAW_FMT_NONE = 0xFF,
     *         };
     * \see getStereoSensorIndex
     */
    static MUINT getSensorRawFormat(const int SENSOR_INDEX);

    /**
     * \brief Check if stereo camera is Bayer+Mono
     * \details Mono sensor will be main2, therefore we don't need to pass sensor index
     * \return true if stereo camera is Bayer+Mono
     */
    static bool isBayerPlusMono();

    /**
     * \brief Check if current feature is Bayer+Mono VSDoF
     * \details Condition: 1. B+M denoise is off 2. Main2 is mono sensor
     * \return true if Bayer+Mono VSDoF is enabled
     */
    static bool isBMVSDoF();

    /**
     * \brief Check if current feature use dual camera
     * \details Dual camera feature excludes PIP and includes zoom, vsdof, denoise, etc
     * \return true if it is dual cam
     */
    static bool isDualCamMode();

    /**
     * \brief Set stereo profile
     * \details There could be more than one set of stereo camera on the device,
     *          different profile has different sensor id, baseline, FOV, sizes, etc.
     *
     * \param profile Stereo profile
     */
    static void setStereoProfile(ENUM_STEREO_SENSOR_PROFILE profile) __attribute__ ((deprecated("[Dualcam]Use setLogicalDeviceID")));

    /**
     * \brief Set logical device ID
     * \details A logical device will contain 2 or more sensors.
     *          This will replace the call to setStereoProfile in HAL1
     *
     * \param logicalDeviceID Logical device ID
     * \return true if success for multicam
     */
    static bool setLogicalDeviceID(const MUINT32 logicalDeviceID);

    /**
     * \brief Set logical device ID & feature mode at the same time
     * \details Set those param at the same time to prevent race condition
     *
     * \param param params to set, includes logical device ID, feature mode, preview size, etc
     */
    static bool setLogicalDeviceParam(MULTICAM_LOGICAL_DEVICE_PARAM param);

    /**
     * \brief Get latest logical device ID
     * \details setLogicalDeviceID first
     * \return latest logical device ID set by setLogicalDeviceID
     * \see setLogicalDeviceID
     */
    static MUINT32 getLogicalDeviceID() { return __logicalDeviceID; }

    /**
     * \brief Set image ratio
     *
     * \param ratio Image ratio
     */
    static void setImageRatio(StereoHAL::STEREO_IMAGE_RATIO_T ratio);

    /**
     * \brief Set stereo feature mode and portrait mode
     * \details Stereo feature mode can be combination of ENUM_STEREO_FEATURE_MODE, default is -1
     *
     * \param stereoMode stereo feature mode
     * \param isPortrait true for portrait mode, default is false
     *                   NOTICE: only vsdof and mtkdepthmap support portrait mode now
     * \param isRecording true for recording flow
     * \param eisFactor EIS enlarge factor
     */
    static void setStereoFeatureMode(MINT32 stereoMode, bool isPortrait=false, bool isRecording=false, MUINT32 eisFactor=100);

    /**
     * \brief Get if current flow is record
     * \details This value will be assigned when calling setStereoFeatureMode
     * \return true if it is recording now
     * \refer setStereoFeatureMode
     */
    static bool isRecording() { return __isRecording; }

    /**
     * \brief Get current shot mode
     * \return Shot mode, currenly only eShotMode_ZsdShot is returned
     */
    static NSCam::EShotMode getShotMode();

    /**
     * \brief Set current sensor module type in stereo mode
     */
    static void setStereoModuleType(MINT32 moduleType);

    /**
     * \brief Get current sensor module type
     * \return return stereo module type
     */
    static int getStereoModuleType() {return __stereoModuleType;}

    /**
     * \brief Get current feature type
     *
     * \param openID the open ID to query, can be logical of physical
     * \return return stereo feature type
     */
    static int getStereoFeatureMode(MUINT32 openID=getLogicalDeviceID());

#if (1 == HAS_WPE)
    /**
     * \brief Get warping buffer for N3D
     *
     * \param sensor Sensor to get
     *
     * \return WARPING_BUFFER_CONFIG_T, if user want to get size of each plan in byte, please call WARPING_BUFFER_CONFIG_T::getPlaneSizeInBytes()
     */
    static WARPING_BUFFER_CONFIG_T getWarpingBufferConfig(StereoHAL::ENUM_STEREO_SENSOR sensor);
#endif

    /**
     * \brief get stereo shot mode
     *
     * \return return current stereo shot mode
     */
    static MUINT getStereoShotMode() {return __stereoShotMode;}

    /**
     * \brief set stereo shot mode
     */
    static void setStereoShotMode(MUINT32 stereoShotMode);

    /**
     * \brief Get DPE round for capture
     * \return DPE round for capture
     */
    static size_t getDPECaptureRound();

    /**
     * \brief Get max N3D capture debug buffer in bytes
     * \details N3D debug buffer will pack several buffers into one, this API help to get the total size in bytes
     * \return Max N3D debug buffer size in bytes
     */
    static size_t getMaxN3DDebugBufferSizeInBytes();

    /**
     * \brief Check if the sensor combination is Wide+Tele (by FOV diff)
     *
     * \param logicalDeviceID The stereo set to query
     * \return true if the sensor combination is Wide+Tele
     */
    static bool isWidePlusTele(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Check if Wide+Tele VSDoF
     *
     * \param profile The stereo set to query
     * \return true if Wide+Tele VSDoF
     */
    static bool isWideTeleVSDoF(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get sensor scenario
     * \details Use current parameters to query and update sensor scenarios
     *
     * \param stereoMode Stereo Mode, such as VSDoF, Denoise, etc
     * \param isRecording true for recording
     * \param updateToo Update sensor scenario after query at the same time
     *
     * \param sensorScenarioMain1 Updated sensor scenario of main1
     * \param sensorScenarioMain2 Updated sensor scenario of main2
     */
    static bool getSensorScenario(MINT32 stereoMode,
                                  bool isRecording,
                                  //Below are output
                                  MUINT &sensorScenarioMain1,
                                  MUINT &sensorScenarioMain2,
                                  bool updateToo=false);

    /**
     * \brief Update sensor scenario
     * \details Update result from getSensorScenario
     *
     * \param sensorScenarioMain1 Updated sensor scenario of main1
     * \param sensorScenarioMain2 Updated sensor scenario of main2
     */
    static void updateSensorScenario(MUINT sensorScenarioMain1,
                                     MUINT sensorScenarioMain2);

    /**
     * \brief Get sensor scenarios
     * \details Use current parameters to query and update sensor scenarios
     *
     * \param stereoMode Stereo Mode, such as VSDoF, Denoise, etc
     * \param isRecording true for recording
     * \param updateToo Update sensor scenario after query at the same time
     *
     * \param sensorScenarios Updated sensor scenarios
     */
    static bool getSensorScenarios(MINT32 stereoMode,
                                  bool isRecording,
                                  std::vector<MUINT> &sensorScenarios,
                                  bool updateToo=false);

    /**
     * \brief Update sensor scenario
     * \details Update result from getSensorScenario
     *
     * \param sensorScenarios Updated sensor scenarios
     */
    static void updateSensorScenarios(const std::vector<MUINT> &sensorScenarios);

    /**
     * \brief Get current sensor scenario of main1
     * \details Need to call updateSensorScenario to update before this
     *
     * \return current sensor scenario of main1
     */
    static MUINT getSensorScenarioMain1();

    /**
     * \brief Get current sensor scenario of main2
     * \details Need to call updateSensorScenario to update before this
     *
     * \return current sensor scenario of main2
     */
    static MUINT getSensorScenarioMain2();

    /**
     * \brief Get current sensor scenario of the sensor
     * \details Need to call updateSensorScenario to update before this
     *
     * \param sensor the sensor to query
     * \return current sensor scenario of the sensor
     */
    static MUINT getCurrentSensorScenario(StereoHAL::ENUM_STEREO_SENSOR sensor);

    /**
     * \brief Get hardware FEO size
     *
     * \param inputSize Center cropped input image size, can query from pass2 size in size providers
     * \param blockSize Get from fefmBlockSize
     * \param feoSize Output size of
     * \return size of buffer in byes
     */
    static size_t queryHWFEOBufferSize(NSCam::MSize inputSize, MUINT blockSize, NSCam::MSize &feoSize)
    {
        feoSize.w = inputSize.w/blockSize*40;
        feoSize.h = inputSize.h/blockSize;
        return feoSize.w * feoSize.h;
    }

    /**
     * \brief Get hardware FMO size
     *
     * \param feoSize get from queryHWFEOBufferSize
     * \param fmoSize fmo buffer size
     *
     * \return size of buffer in bytes
     */
    static size_t queryHWFMOBufferSize(NSCam::MSize feoSize, NSCam::MSize &fmoSize)
    {
        fmoSize.w = (feoSize.w/40) * 2;
        fmoSize.h = feoSize.h;

        return fmoSize.w * fmoSize.h;
    }

    /**
     * \brief Set current VSDoF scenario
     * \details Should be only set by middleware
     *
     * \param scenario Scenario to set, enum is defined in camera_custom_stereo.h
     */
    static void setVSDoFScenario(ENUM_STEREO_CAM_SCENARIO scenario)
    {
        __vsdofScenario = scenario;
    }

    /**
     * \brief Get current VSDoF scenario
     * \details Get the vsdof scenario set by middlware
     * \return Scenario to get, enum is defined in camera_custom_stereo.h
     */
    static ENUM_STEREO_CAM_SCENARIO getVSDoFScenario()
    {
        return __vsdofScenario;
    }

    /**
     * \brief Get callback buffers to AP in brief string
     * \details Buffers:
     *       ci: Clean Image
     *       bi: Bokeh Image
     *      rbi: Relighting Bokeh Image
     *      mbd: MTK Bokeh Depth
     *      mdb: MTK Debug Buffer
     *      mbm: MTK Bokeh Metadata
     * \return Callback buffer list in string, e.g. ci,bi,mbd,mdb,mbm
     */
    static std::string getCallbackBufferList();

    /**
     * \brief Get callback buffers to AP in bit combination of CallbackBufferType
     * \details Buffers:
     *      Clean Image
     *      Bokeh Image
     *      Relighting Bokeh Image
     *      MTK Bokeh Depth
     *      MTK Debug Buffer
     *      MTK Bokeh Metadata
     * \return Callback buffer list bitset, e.g. 001111
     */
    static std::bitset<NSCam::v1::Stereo::CallbackBufferType::E_DUALCAM_JPEG_ENUM_SIZE> getBokehJpegBufferList();

    /**
     * \brief Get Main1 FOV crop ratio
     * \details Must get after set stereo profile and feature mode
     *
     * \param previewCropRatio the preview crop ratio to query
     * \return FOV crop ratio, 1.0 if no crop
     */
    static float getMain1FOVCropRatio(float previewCropRatio=1.0f);

    /**
     * \brief Get Main2 FOV crop ratio
     * \details Must get after set stereo profile and feature mode
     *
     * \param previewCropRatio the preview crop ratio to query
     * \return FOV crop ratio, 1.0 if no crop
     */
    static float getMain2FOVCropRatio(float previewCropRatio=1.0f);

    /**
     * \brief Set preview size passed from AP
     * \details This will also decide image ratio in providers
     *
     * \param pvSize Current preview size
     */
    static void setPreviewSize(NSCam::MSize pvSize);

    /**
     * \brief Get calibration data of profile
     * \details Get calibration data from 3rd party, EEPROM or offline calibration data
     *
     * \param calibrationData Calibration data buffer address to fill
     * \param profile Profile to query, default is current profile set from middlware
     *
     * \return Calibration data size returned
     */
    static size_t getCalibrationData(void *calibrationData, MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get ISP Profile for P1 YUV
     * \details If not supprted, return EIspProfile_N3D_Preview
     *
     * \param sensorID Sensor index to query
     * \return ISP Profile for P1 YUV
     */
    static int getISPProfileForP1YUV(int sensorID);

    /**
     * \brief Check the ISP supports P1 YUV function
     * \return how many P1 YUV that ISP supports
     */
    static int getP1YUVSupported();

    /**
     * \brief Get P1 resize quality for VSDoF
     * \details For VSDoF on MT6799, if a sensor is > 16M, we should enable frontal bin,
     *          Can only get afer calling setLogicalDeviceID() and setStereoFeatureMode()
     * \return NSCam::v3::P1Node::RESIZE_QUALITY
     */
    static NSCam::v3::P1Node::RESIZE_QUALITY getVSDoFP1ResizeQuality();

    /**
     * \brief Get Sensor FOV info by sensor index
     *
     * \param sensorIndex Sensor index, like 0, 1, 2...
     * \return true if success and save result into fovHorizontal & fovVertical
     */
    static bool getSensorFOV(const int SENSOR_INDEX, float &fovHorizontal, float &fovVertical);

    /**
     * \brief * \brief Get depthmap info
     * \details Get depthmap size info from config
     *
     * \param ratio image ratio
     * \param eScenario preview or capture
     *
     * \return Depthmap infomation, includes size & format
     */
    static MTK_DEPTHMAP_INFO_T getDepthmapInfo(StereoHAL::STEREO_IMAGE_RATIO_T ratio=imageRatio(),
                                               StereoHAL::ENUM_STEREO_SCENARIO eScenario=StereoHAL::eSTEREO_SCENARIO_CAPTURE);

    /**
     * \brief For low-end ISP, we'll skip to output main2 images for every n frames
     * \details TK flow will always be 1(not to skip)
     *
     * \param scenario scanario to get
     * \return frequecy to output main2 image
     */
    static MUINT32 getMain2OutputFrequency(StereoHAL::ENUM_STEREO_SCENARIO scenario=StereoHAL::eSTEREO_SCENARIO_PREVIEW);

    /**
     * \brief set 3rd depth algo running state
     * \details set 3rd depth algo running state
     *
     * \param isDepthRunning 3rd depth algo is running or not (false:stop, true:running)
     * \return None
     */
    static void set3rdDepthAlgoRunning(bool isDepthRunning);

    /**
     * \brief set 3rd bokeh algo running state
     * \details set 3rd bokeh algo running state
     *
     * \param isBokehRunning 3rd bokeh algo is running or not (false:stop, true:running)
     * \return None
     */
    static void set3rdBokehAlgoRunning(bool isBokehRunning);

    /**
     * \brief get 3rd depth or bokeh algo running state
     * \details get 3rd depth or bokeh algo running state
     *
     * \return 3rd depth or bokeh algo running state (false:stop, true:running)
     */
    static bool get3rdDepthOrBokehAlgoRunning();

    /**
     * \brief Check if AF sync is needed, need to set logical device ID & feature mode first
     * \details Only AF+AF needs
     *
     * \param logicalDeviceID Logical device ID
     * \return True if sensor is AF+AF, so that AF sync is needed
     */
    static bool isAFSyncNeeded(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get depthmap output type for mtkdepthmap feature mode
     * \details There might be two possible node to output depthmap: OCC and GF.
     *          And their output can map to E_DEPTHMAP_OCC and E_DEPTHMAP_SW_OPTIMIZED
     *          respectively.
     *          This query is only valid when feature mode is set to mtkdepthmap
     * \return -1 if the value is not configured, otherwise returns the value configured
     * \refer ENUM_DEPTHMAP_REFINE_LEVEL
     */
    static int getDepthmapRefineLevel(StereoHAL::ENUM_STEREO_SCENARIO eScenario=StereoHAL::eSTEREO_SCENARIO_UNKNOWN);

    /**
     * \brief Get multicam zoom range of the logical device
     * \details If only returns one number A, means the range will be [A, A], which means no zoom support
     *          If returns two number [A, B], means the range will be [A, B]
     *          If returns with N numbers, get range from index 0 and index N-1.
     *          This API guarantees the ranges are in ascending order
     * \return Supported multicam zoom range in float
     */
    static std::vector<float> getMulticamZoomRange(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get multicam zoom step of the logical device
     * \details If only returns one number A, means the step will be [A, A], which means no zoom support
     *          If returns two number [A, B], means the step will be [A, B]
     *          If returns with N numbers, get step from index 0 and index N-1.
     *          This API guarantees the steps are in ascending order
     * \return Supported multicam zoom step in float
     */
    static std::vector<float> getMulticamZoomSteps(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Get FRZ Ratio of a sensor for a feature
     * \details FRZ Range will be [0, 1.0].
     *          Return 0.5 by default.
     *
     * \param sensorIndex Sensor index to query
     * \param feature Feature to query
     * \return FRZ Ratio
     */
    static float getFRZRatio(int32_t sensorIndex, int featureMode);

    /**
     * \brief Get physical sensor ID from steroe sensor enum
     * \details e.g. eSTEREO_SENSOR_MAIN1->0, eSTEREO_SENSOR_MAIN2->2
     *
     * \param sensor Sensor to query
     * \param logicalDeviceID to query, use current logical device id by default
     * \return Physical sensor index
     */
    static int32_t sensorEnumToId(StereoHAL::ENUM_STEREO_SENSOR sensor, int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Get stereo sensor enum from physical sensor index
     * \details e.g. 0->eSTEREO_SENSOR_MAIN1, 2->eSTEREO_SENSOR_MAIN2
     *
     * \param SENSOR_INDEX Physical sensor ID to query
     * \param logicalDeviceID to query, use current logical device id by default
     * \return Stereo sensor enum
     */
    static StereoHAL::ENUM_STEREO_SENSOR sensorIdToEnum(const int32_t SENSOR_INDEX, int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Get Lens pose translation of the sensor
     * \details This will be used for static metadata LEN_POSE_TRANSLATION
     *
     * \param sensorId Sensor ID
     *
     * \return Lens pose translation, length must be 3
     * \refer https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics.html#LENS_POSE_TRANSLATION
     */
    static std::vector<float> getLensPoseTranslation(int32_t sensorId);

    /**
     * \brief Return EIS factor for recording
     * \details This value is set by setStereoFeatureMode
     * \return EIS factor in recording mode, default is 100, which means 1.00x, no EIS
     */
    static MUINT32 getEISFactor();

    /**
     * \brief Check if recording is enabled in stereo mode
     * \details User can add "Enable Record": 1 in each logical device setting or global setting in the setting file
     *
     * \param logicalDeviceID The logical device to query
     * \return true if:
     *          1. Is dual cam zoom or "Enable Record" is set to 1
     *          3. SensorScenarioRecording is configured for sensors in the logical device
     */
    static bool isMulticamRecordEnabled(int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Get exclusive sensor group
     * \details The sensors are exlusive(cannot use the same RAW ISP) within the same group
     *
     * \param logicalDeviceID The logical device to query, default is the current logical device
     * \return The map of sensor index -> group ID
     */
    static std::unordered_map<int, int> getExclusiveGroup(int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Check if bokeh blending is enabled
     * \details Only query after setLogicalDeviceID is called
     *               User can turn on/off by SW_BLENDING tuning or property
     * \param scenario scenario to query, only support preview/record
     * \return true if bokeh blending is enabled
     */
    static bool isBokehBlendingEnabled(StereoHAL::ENUM_STEREO_SCENARIO scenario=StereoHAL::eSTEREO_SCENARIO_PREVIEW);

    /**
     * \brief Get MTK depth flow
     * \details Get customized depth flow of the logical device.
     *          The "Feature" of the logical device must be one of "vsdof", "mtkdepthmap" or "3rdparty".
     *          If the feature is set to "mtkdepthmap" or "3rdparty", this API will return 2 or 1 repectively.
     *          If the feature is set to "vsdof", we'll check the setting by the key "Depth Flow".
     *          The value should be one of "vsdof", "mtkdepthmap" or "3rdparty".
     *          e.g. "Depth Flow": "vsdof"
     *
     *          If not set, we'll return result of getStereoModeType() in camera_custom_stereo.cpp
     *
     * \param logicalDeviceID The logical device to query
     * \return The depth flow of the logical device
     */
    static int getMtkDepthFlow(int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Set sensor physical information for sensors
     * \details Set some physical information to setting provider, must set before calling setLogicalDevices()
     *
     * \param logicalDeviceID The logical device to set(includes physical sensor ids)
     * \param info The information to set
     * \return true if success
     */
    static bool setSensorPhysicalInfo(int logicalDeviceID, StereoHAL::SENSOR_PHYSICAL_INFO_T info);

    /**
     * \brief Get sensor physical information set by setSensorPhysicalInfo
     *
     * \param logicalDeviceID logical device to query
     * \param info Information returns
     *
     * \return true if success, false if the setting is not found
     */
    // static bool getSensorPhysicalInfo(int logicalDeviceID, StereoHAL::SENSOR_PHYSICAL_INFO_T &info);

    /**
     * \brief Check if depth delay is enabled, default is disabled.
     * \details User can enable by add "Depth Delay": 1 in global setting or individual logical device.
     *          Only works for turnkey VSDoF flow
     *
     * \param logicalDeviceID The logical device to query
     * \return true if depth delay is enabled
     */
    static bool isDepthDelayEnabled(int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Check if current module set is slant or not
     * \details Slant camera module means that minor camera has both x and y offset to main camera
     *
     * \param logicalDeviceID The logical device to query
     * \return true if the current camera module is slant camera
     */
    static bool isSlantCameraModule(int logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Get depth frequency
     * \details User can control depth frequency for logical device individually or in global.
     *          Which means the setting can be put into an logical device, or put it outside of the sensor combination setting
     *          Setting example 1:
     *            "Depth Freq": 15
     *          Setting example 2:
     *            "Depth Freq": {
     *                "Preview": 15,
     *                "Record": 1
     *            }
     *
     * \param scenario eSTEREO_SCENARIO_PREVIEW or eSTEREO_SCENARIO_RECORD
     * \param logicalDeviceID The logical device to query
     *
     * \return Depth frequency, means depth map comes out per n frame
     */
    static size_t getDepthFreq(StereoHAL::ENUM_STEREO_SCENARIO scenario, MUINT32 logicalDeviceID=StereoSettingProvider::getLogicalDeviceID());

    /**
     * \brief Set preview crop ratio for bokeh, this value will be <= 1.0f
     *
     * \param ratio The current preview crop ratio for bokeh
     */
    static void setPreviewCropRatio(float ratio);

    /**
     * \brief Get current preview crop ratio which was set by setPreviewCropRatio
     * \return Preview crop ratio in bokeh mode
     */
    static float getPreviewCropRatio() { return __previewCropRatio; }

    /**
     * \brief * \brief Get min pass1 crop ratio
     * \details Since pass1 cannot scale up, when we crop for zooming,
     *          it cannot be smaller than output size.
     *          This means P1 has a crop limit.
     *
     * \return the min crop ratio for Pass1, will <= 1.0f
     */
    static float getVSDoFMinPass1CropRatio();

    /**
     * \brief Get frame sync type of the logical device
     * \details Can be vsync align or read out center align
     *
     * \param logicalDeviceID The logical device to query
     * \return the frame sync type, one of the value of ENUM_FRAME_SYNC_TYPE
     */
    static int getFrameSyncType(MUINT32 logicalDeviceID=getLogicalDeviceID());

    /**
     * \brief Check if current flow is HIDL
     * \details User can return depthmap through HIDL
     *          User can turn it on by append "+hidl" to feature in setting,
     *          e.g. "Feature": "Mtkdepthmap+hidl"
     *
     * \param logicalDeviceID The logical device to query
     * \return True if the device is set to HIDL flow
     */
    static bool isHIDL(MUINT32 logicalDeviceID=getLogicalDeviceID());

private:
    static void _updateImageSettings();
    static bool _getOriginalSensorIndex(int32_t &main1Idx, int32_t &main2Idx, MUINT32 logicalDeviceID=getLogicalDeviceID());
    static std::string _getStereoFeatureModeString(int stereoMode);
    static bool __initTuningIfReady();

private:
    static MUINT32                      __logicalDeviceID;
    static StereoHAL::STEREO_IMAGE_RATIO_T            __imageRatio;
    static MINT32                       __stereoFeatureMode;
    static bool                         __isRecording;
    static MUINT32                      __eisFactor;
    static MINT32                       __stereoModuleType;
    static MUINT32                      __stereoShotMode;

    static std::vector<MUINT>           __sensorScenarios;

    static MUINT32                      __featureStartTime;

    static ENUM_STEREO_CAM_SCENARIO     __vsdofScenario;

    static bool                         __3rdCaptureDepthRunning;
    static bool                         __3rdCaptureBokehRunning;

    static float                        __previewCropRatio;
};

#endif  // _STEREO_SETTING_PROVIDER_H_

