/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_TYPES_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_TYPES_H_
//
#include <mtkcam3/pipeline/stream/IStreamBuffer.h>
#include <mtkcam3/pipeline/pipeline/IPipelineContext.h>
#include <mtkcam/def/common.h>  // for mtkcam3/feature/eis/EisInfo.h
#include <mtkcam/drv/def/ICam_type.h> // for isp quality enum
#include <mtkcam3/feature/eis/EisInfo.h>
#include <mtkcam/def/ImageBufferInfo.h>
#include <mtkcam3/feature/stereo/StereoCamEnum.h>
#include <mtkcam3/def/zslPolicy/types.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>

//
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <set>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {

////////////////////////////////////////////////////////////////////////////////

/**
 *  Pipeline static information.
 *
 *  The following information is static and unchanged forever, regardless of
 *  any operation (e.g. open or configure).
 */
struct PipelineStaticInfo
{
    /**
     *  Logical device open id
     */
    int32_t                                     openId = -1;

    /**
     *  Physical sensor id (0, 1, 2)
     */
    std::vector<int32_t>                        sensorId;

    int32_t getIndexFromSensorId(int Id) const
    {
        for (int32_t i = 0; i < (int32_t)sensorId.size(); i++)
        {
            if(Id == sensorId[i])
                return i;
        }
        return -1;
    };

    /**
     *  Sensor raw type.
     *
     *  SENSOR_RAW_xxx in mtkcam/include/mtkcam/drv/IHalSensor.h
     */
    std::vector<uint32_t>                       sensorRawType;

    /**
     *
     */
    bool                                        isDualDevice = false;

    /**
     *  Type3 PD sensor without PD hardware (ISP3.0)
     */
    bool                                        isType3PDSensorWithoutPDE = false;

    /**
     *  is 4-Cell sensor (only for main1 sensor or single cam now)
     */
    bool                                        is4CellSensor = false;
    NSCamHW::HwInfoHelper::e4CellSensorPattern  fcellSensorPattern = NSCamHW::HwInfoHelper::e4CellSensorPattern_Unknown;

    /**
     *  seamless switch for 4cell sensor supported
     *  Only for main1 sensor(or single cam) now.
     */
    bool                                        isSeamlessSwitchSupported = false;
    std::set<uint32_t>                          seamlessSwitchSensorModes;

    /**
     *  is VHDR sensor
     */
    bool                                        isVhdrSensor = false;
    /**
     *  is P1 direct output FD YUV (ISP6.0)
     */
    bool                                        isP1DirectFDYUV = false;

    /**
     *  support AIAWB
     */
    bool                                        isAIAWBSupported = false;

    /**
     *  support AI Shutter
     */
    bool                                        isAIShutterSupported = false;

    /**
     *  Must use P2 output process raw
     */
    bool                                        isNeedP2ProcessRaw = false;

    /**
     *  P1 direct output rectify YUV (ISP6.0)
     */
    int32_t                                     numsP1YUV = -1;
    /**
     *  is support burst capture or not
     */
    bool                                        isSupportBurstCap = false;

    /**
     *  isp tuning data stream size for raw
     */
    MSize                                       rawIspTuningDataStreamSize;

    /**
     *  isp tuning data stream size for yuv
     */
    MSize                                       yuvIspTuningDataStreamSize;


    /**
     *  need additional per-stream cropping, for example: for resolve PDC boundary quality issue in some sensor
     */
    bool                                        isAdditionalCroppingEnabled = false;

    bool                                        isAFSupported = true;
};


struct ParsedAppConfiguration;
struct ParsedAppImageStreamInfo;
struct ParsedMultiCamInfo;
struct ParsedSMVRBatchInfo;

/**
 *  Pipeline user configuration
 *
 *  The following information is given and set up at the configuration stage, and
 *  is never changed AFTER the configuration stage.
 */
struct PipelineUserConfiguration
{
    std::shared_ptr<ParsedAppConfiguration>     pParsedAppConfiguration;

    /**
     * Parsed App image stream info set
     *
     * It results from the raw data, i.e. vImageStreams.
     */
    std::shared_ptr<ParsedAppImageStreamInfo>   pParsedAppImageStreamInfo;

    /**************************************************************************
     * App image stream info set (raw data)
     **************************************************************************/

    /**
     * App image streams to configure.
     */
    std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>
                                                vImageStreams;

    /**
     * App meta streams to configure.
     */
    std::unordered_map<StreamId_T, android::sp<IMetaStreamInfo>>
                                                vMetaStreams;

    /**
     * App image streams min frame duration to configure.
     */
    std::unordered_map<StreamId_T, int64_t>     vMinFrameDuration;

    /**
     * App image streams stall frame duration to configure.
     */
    std::unordered_map<StreamId_T, int64_t>     vStallFrameDuration;

   /**
     * @param[in] physical camera id list
     */
    std::vector<int32_t>                        vPhysicCameras;

};

////////////////////////////////////////////////////////////////////////////////


/**
 * P1 DMA bitmask definitions
 *
 * Used in the following structures:
 *      IPipelineSettingPolicy.h: RequestResultParams::needP1Dma
 *      IIOMapPolicy.h: RequestInputParams::pRequest_NeedP1Dma
 *      IStreamInfoConfigurationPolicy.h: FunctionType_StreamInfoConfiguration_P1
 *
 */
enum : uint32_t
{
    P1_IMGO     = (0x01U << 0),
    P1_RRZO     = (0x01U << 1),
    P1_LCSO     = (0x01U << 2),
    P1_RSSO     = (0x01U << 3),
    // for ISP 60
    P1_FDYUV    = (0x01U << 4),
    P1_FULLYUV  = (0x01U << 5),
    P1_SCALEDYUV = (0x01U << 6),
    // for seamless switch
    P1_SEAMLESSIMGO = (0x01U << 7),
    //
    P1_MASK     = 0x0F,
    P1_ISP6_0_MASK = 0xFF,
};

/**
 * P2 node bitmask definitions
 *
 * Used in the following structures:
 *      IIOMapPolicy.h: RequestInputParams::pRequest_NeedP1Dma
 *
 */
enum : uint32_t
{
    P2_STREAMING     = (0x01U << 0),
    P2_CAPTURE       = (0x01U << 1),
};

/**
 * Reconfig category enum definitions
 * For pipelineModelSession processReconfiguration use
 */
enum class ReCfgCtg: uint8_t
{
    NO          = 0,
    STREAMING,
    CAPTURE,
    HIGH_RES_REMOSAIC,
    SEAMLESS_REMOSAIC,
    NUM
};

/**
 *  Sensor Setting
 */
struct SensorSetting
{
    uint32_t                                    sensorMode = 0;
    uint32_t                                    sensorFps = 0;
    MSize                                       sensorSize;
};

/**
 * BatchPolicy
 * size/frameType: batch size / frame type to be counted in BatchInfo
 *
 * 1/(ALL): for normal case.
 * { pre-dummy  pre-dummy  pre-sub  main sub post-dummy post-dummy }, BatchInfo will be
 *     0/1         0/1         0/1     0/1   0/1      0/1         0/1
 *
 * 2/(STREAMING): for M-Stream video HDR.
 * { pre-dummy pre-dummy pre-sub main post-dummy post-dummy }, BatchInfo will be
 *      0/0        0/0      0/2   1/2    0/0        0/0
 *
 * 4/(ALL): for 120fps MFLL capture
 * { pre-dummy pre-dummy main sub } { sub sub sub sub } { post-dummy post-dummy post-dummy post-dummy } BatchInfo will be
 *     0/4       1/4     2/4  3/4     0/4 1/4 2/4 3/4         0/4       1/4        2/4         3/4
 */
struct BatchPolicy
{
    enum : uint32_t
    {
        NONE       = 0,
        MAIN       = (0x01U << 0),
        PRE_SUB    = (0x01U << 1),
        SUB        = (0x01U << 2),
        PRE_DUMMY  = (0x01U << 3),
        POST_DUMMY = (0x01U << 4),
        //
        STREAMING  = (MAIN | PRE_SUB | SUB),
        ALL        = 0xFFFFFFFFU,
    };

    size_t size = 1;
    uint32_t frameType = ALL;
};


static inline android::String8 toString(const SensorSetting& o __unused)
{
    android::String8 os = android::String8::format("{ .sensorMode=%d .sensorFps=%d .sensorSize=%dx%d }", o.sensorMode, o.sensorFps, o.sensorSize.w, o.sensorSize.h);
    return os;
};


/**
 *  DMA settings
 */
struct DmaSetting
{
    /**
     * Image format.
     */
    int32_t                                     format = 0;

    /**
     * Image resolution in pixel.
     */
    MSize                                       imageSize;

};


static inline android::String8 toString(const DmaSetting& o __unused)
{
    android::String8 os;
    os += "{";
    os += android::String8::format(" format:%#x", o.format);
    os += android::String8::format(" %dx%d", o.imageSize.w, o.imageSize.h);
    os += " }";
    return os;
};


/**
 *  Pass1-specific HW settings
 */
struct P1HwSetting
{
    /**
     * @param imgoAlloc
     *        It is used for allcating buffer. If we need to runtime change
     *        this dma setting, a worst setting (e.g. eImgFmt_BAYER10_UNPAK)
     *        should be indicated in allocating buffer stage.
     *
     * @param imgoDefaultRequest
     *        It is used for default (streaming) request.
     *        It is used for configuring P1Node.
     *
     * @param imgoSeamlessRequest
     *        It is used for seamless capture, if seamless switch is supported requests.
     *        It is used for configuring P1Node also.
     */
    DmaSetting                                  imgoAlloc;
    DmaSetting                                  imgoDefaultRequest;
    DmaSetting                                  imgoSeamlessRequest;

    /**
     * @param rrzoDefaultRequest
     *        It is used for default (streaming) request.
     */
    DmaSetting                                  rrzoDefaultRequest;

    MSize                                       rssoSize;

    uint32_t                                    pixelMode = 0;
    bool                                        usingCamSV = false;
    // ISP6.0
    MSize                                       fdyuvSize;
    MSize                                       scaledYuvSize;
    MSize                                       AIAWBYuvSize;
    uint32_t                                    AIAWBPort = 0;
    bool                                        canSupportScaledYuv = false;

    // define in def/ICam_type.h
    E_CamIQLevel                                ispQuality = eCamIQ_MAX;
    E_INPUT                                     camTg = TG_CAM_MAX;
    CAM_RESCONFIG                               camResConfig;
};


static inline android::String8 toString(const P1HwSetting& o __unused)
{
    android::String8 os;
    os += "{";
    os += " .imgoAlloc=";
    os += toString(o.imgoAlloc);
    os += " .imgoDefaultRequest=";
    os += toString(o.imgoDefaultRequest);
    os += " .imgoSeamlessRequest=";
    os += toString(o.imgoSeamlessRequest);
    os += " .rrzoDefaultRequest=";
    os += toString(o.rrzoDefaultRequest);
    os += android::String8::format(" .rssoSize=%dx%d", o.rssoSize.w, o.rssoSize.h);
    os += android::String8::format(" .pixelMode=%u .usingCamSV=%d", o.pixelMode, o.usingCamSV);
    os += " }";
    return os;
};


/**
 *  Parsed metadata control request
 */
struct ParsedMetaControl
{
    bool                                        repeating = false;

    int32_t                                     control_aeTargetFpsRange[2]     = {0};//CONTROL_AE_TARGET_FPS_RANGE
    uint8_t                                     control_captureIntent           = static_cast< uint8_t>(-1L);//CONTROL_CAPTURE_INTENT
    uint8_t                                     control_enableZsl               = static_cast< uint8_t>(0);//CONTROL_ENABLE_ZSL
    uint8_t                                     control_mode                    = static_cast< uint8_t>(-1L);//CONTROL_MODE
    uint8_t                                     control_sceneMode               = static_cast< uint8_t>(-1L);//CONTROL_SCENE_MODE
    uint8_t                                     control_videoStabilizationMode  = static_cast< uint8_t>(-1L);//CONTROL_VIDEO_STABILIZATION_MODE
    uint8_t                                     edge_mode                       = static_cast< uint8_t>(-1L);//EDGE_MODE
    uint8_t                                     control_isp_tuning              = 0;//for mtk isp tuning
    MINT32                                      control_isp_frm_count           = 0;
    MINT32                                      control_isp_frm_index           = 0;
    MINT32                                      control_processRaw              = 0;
    MINT32                                      control_remosaicEn              = 0;//MTK_CONTROL_CAPTURE_REMOSAIC_EN
    MINT32                                      control_vsdofFeatureWarning     = -1;//MTK_VSDOF_FEATURE_CAPTURE_WARNING_MSG

};


static inline android::String8 toString(const ParsedMetaControl& o __unused)
{
    android::String8 os;
    os += "{";
    os += android::String8::format(" repeating:%d", o.repeating);
    os += android::String8::format(" control.aeTargetFpsRange:%d,%d", o.control_aeTargetFpsRange[0], o.control_aeTargetFpsRange[1]);
    os += android::String8::format(" control.captureIntent:%d", o.control_captureIntent);
    os += android::String8::format(" control.enableZsl:%d", o.control_enableZsl);
    os += android::String8::format(" control.processRawEn:%d", o.control_processRaw);
    if ( static_cast< uint8_t>(-1L) != o.control_mode ) {
        os += android::String8::format(" control.mode:%d", o.control_mode);
    }
    if ( static_cast< uint8_t>(-1L) != o.control_sceneMode ) {
        os += android::String8::format(" control.sceneMode:%d", o.control_sceneMode);
    }
    if ( static_cast< uint8_t>(-1L) != o.control_videoStabilizationMode ) {
        os += android::String8::format(" control.videoStabilizationMode:%d", o.control_videoStabilizationMode);
    }
    if ( static_cast< uint8_t>(-1L) != o.edge_mode ) {
        os += android::String8::format(" edge.mode:%d", o.edge_mode);
    }
    os += " }";
    return os;
};


/**
 *  Parsed App configuration
 */
struct ParsedAppConfiguration
{
    /**
     * The operation mode of pipeline.
     * The caller must promise its value.
     */
    uint32_t                                    operationMode = 0;

    /**
     * Session wide camera parameters.
     *
     * The session parameters contain the initial values of any request keys that were
     * made available via ANDROID_REQUEST_AVAILABLE_SESSION_KEYS. The Hal implementation
     * can advertise any settings that can potentially introduce unexpected delays when
     * their value changes during active process requests. Typical examples are
     * parameters that trigger time-consuming HW re-configurations or internal camera
     * pipeline updates. The field is optional, clients can choose to ignore it and avoid
     * including any initial settings. If parameters are present, then hal must examine
     * their values and configure the internal camera pipeline accordingly.
     */
    IMetadata                                   sessionParams;

    /**
     * operationMode = 1
     *
     * StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE = 1
     * Refer to https://developer.android.com/reference/android/hardware/camera2/params/SessionConfiguration#SESSION_HIGH_SPEED
     */
    bool                                        isConstrainedHighSpeedMode = false;

    /**
     * operationMode: "persist.vendor.mtkcam.operationMode.superNightMode"
     *
     * Super night mode.
     */
    bool                                        isSuperNightMode = false;

    /**
     * Check to see whether or not the camera is being used by a proprietary client.
     *
     * true: proprietary.
     * false: unknown. It could be a proprietary client or a 3rd-party client.
     */
    bool                                        isProprietaryClient = false;

    /**
     * Always-On Vision (AOV) mode.
     *
     * = 0: non-aov mode
     * = 1: aov mode - step 1: P1 -> Y8
     * = 2: aov mode - step 2: P1 -> RRZO -> P2 -> NV21
     */
    uint32_t                                    aovMode = 0;

    /**
     * Output stream for Always-On Vision usecase.
     */
    android::sp<IImageStreamInfo>               aovImageStreamInfo;

    /**
     * initRequest: parsing sessionparam to get init request is enable or not
     *
     * if enable, default set 4 to improve start preview time
     */
    int                                         initRequest = 0;

    /**
     * Dual cam related info.
     */
    std::shared_ptr<ParsedMultiCamInfo>         pParsedMultiCamInfo;

    /**
     * SMVRBatch related info.
     */
    std::shared_ptr<ParsedSMVRBatchInfo>        pParsedSMVRBatchInfo;

    int                                         useP1DirectFDYUV = false;
    bool                                        supportAIShutter = false;

    /**
     *  Type3 PD sensor without PD hardware (ISP3.0)
     */
    bool                                        isType3PDSensorWithoutPDE = false;

    bool                                        useP1DirectAppRaw = false;

    bool                                        hasTuningEnable = false;

    uint64_t                                    configTimestamp = 0;

    /**
     *  TrackingFocus mode
     */
    bool                                        hasTrackingFocus = false;
};


/**
 *  Parsed App image stream info
 */
struct ParsedAppImageStreamInfo
{
    /**************************************************************************
     *  App image stream info set
     **************************************************************************/

    /**
     * Output streams for any processed (but not-stalling) formats
     *
     * Reference:
     * https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics.html#REQUEST_MAX_NUM_OUTPUT_PROC
     */
    std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>
                                                vAppImage_Output_Proc;

    /**
     * Input stream for yuv reprocessing
     */
    android::sp<IImageStreamInfo>               pAppImage_Input_Yuv;

    /**
     * Output stream for private reprocessing
     */
    android::sp<IImageStreamInfo>               pAppImage_Output_Priv;

    /**
     * Input stream for private reprocessing
     */
    android::sp<IImageStreamInfo>               pAppImage_Input_Priv;

    /**
     * Output stream for RAW16/DNG capture.
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<int, android::sp<IImageStreamInfo>>
                                                vAppImage_Output_RAW16;

    /**
     * Output stream for RAW10 capture.
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<int, android::sp<IImageStreamInfo>>
                                                vAppImage_Output_RAW10;

    /**
     * Input stream for RAW16 reprocessing.
     */
    android::sp<IImageStreamInfo>               pAppImage_Input_RAW16;

    /**
     * Output stream for JPEG capture.
     */
    android::sp<IImageStreamInfo>               pAppImage_Jpeg;

    /**
     * Output stream for Depthmap.
     */
    android::sp<IImageStreamInfo>               pAppImage_Depth;

    /**
     * Output stream for physical yuv stream
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<uint32_t, std::vector<android::sp<IImageStreamInfo>>>
                                                vAppImage_Output_Proc_Physical;

    /**
     * Output stream for physical RAW16/DNG capture.
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<uint32_t, std::vector<android::sp<IImageStreamInfo>>>
                                                vAppImage_Output_RAW16_Physical;

    /**
     * Output stream for physical RAW10 capture.
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<uint32_t, std::vector<android::sp<IImageStreamInfo>>>
                                                vAppImage_Output_RAW10_Physical;

    /**
     * Output stream for isp tuning data stream (RAW)
     */
    android::sp<IImageStreamInfo>               pAppImage_Output_IspTuningData_Raw;

    /**
     * Output stream for isp tuning data stream (YUV)
     */
    android::sp<IImageStreamInfo>               pAppImage_Output_IspTuningData_Yuv;

    /**
     * Output stream for physical isp meta stream (Raw)
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<uint32_t, android::sp<IImageStreamInfo>>
                                                vAppImage_Output_IspTuningData_Raw_Physical;

    /**
     * Output stream for physical isp meta stream (Yuv)
     * map : <sensorId, sp<streamInfo>>
     */
    std::unordered_map<uint32_t, android::sp<IImageStreamInfo>>
                                                vAppImage_Output_IspTuningData_Yuv_Physical;

    /**************************************************************************
     *  Parsed info
     **************************************************************************/

    /**
     * One of consumer usages of App image streams contains BufferUsage::VIDEO_ENCODER.
     */
    bool                                        hasVideoConsumer = false;

    /**
     * 4K video recording
     */
    bool                                        hasVideo4K = false;

    /**
     * The image size of video recording, in pixels.
     */
    MSize                                       videoImageSize;

    /**
     * The image size of app yuv out, in pixels.
     */
    MSize                                       maxYuvSize;

    /**
     * The max. image size of App image streams, in pixels, regardless of stream formats.
     */
    MSize                                       maxImageSize;

    /**
     * Security Info
     */
    SecureInfo                                  secureInfo{.type=SecType::mem_normal,};

    bool                                        hasRawOut = false;

    // for TrackingFocus
    bool                                        hasTrackingFocus = false;

    /**
     * Store needs handling physical sensor id.
     */
    std::vector<uint32_t>                       outputPhysicalSensors;

    StreamId_T                                  previewStreamId = -1;
    MSize                                       previewSize;
};

enum class DualDevicePath{
    /* Using dual camera device, but it does not set feature mode in session parameter or
     * set camera id to specific stream.
     */
    Single,
    /* Using dual camera device, set physical camera id to specific stream.
     */
    MultiCamControl,
};

struct ParsedMultiCamInfo
{
    /**
     * Dual device pipeline path.
     */
    DualDevicePath                              mDualDevicePath = DualDevicePath::Single;

    /**
     * dual cam feature which get from session param.
     */
    int32_t                                     mDualFeatureMode = -1;

    /**
     * support dual cam pack image in hal.
     */
    int32_t                                     mStreamingFeatureMode = -1;

    /**
     * store capture feature mode. (like tk vsdof/ 3rd vsdof/ zoom,.etc)
     */
    int32_t                                     mCaptureFeatureMode = -1;


   /**
     * support dual cam pack image in hal.
     */
    bool                                        mSupportPackJpegImages = false;

    /**
     * support number of avtive camera in the same time.
     */
    uint32_t                                    mSupportPass1Number = 2;


    /**
     * support physical stream output
     */
    bool                                        mSupportPhysicalOutput = false;

};

#define MAX_PIP_CAMDEV_NUM 2
typedef struct ParsedPIPInfo
{
    bool    isValid = false;
    bool    isVendorPIP = false;
    int32_t currDevId = 0;
    int32_t openOrder = 0;
    bool    isMulticam =  false ;
    bool    isOtherMulticam = false;
    // feature combine
    int32_t PQIdx = 0;
    bool    isPreviewEisOn = false;
    bool    isRscOn = false;
    bool    isAISSOn = false;
    int32_t logLevel = 0;
} ParsedPIPInfo;

struct ParsedSMVRBatchInfo
{
    // meta: MTK_SMVR_FEATURE_SMVR_MODE
    int32_t              maxFps = 0;    // = meta:idx=0
    int32_t              p2BatchNum = 1; // = min(meta:idx=1, p2IspBatchNum)
    int32_t              imgW = 0;       // = StreamConfiguration.streams[videoIdx].width
    int32_t              imgH = 0;       // = StreamConfiguration.streams[videoIdx].height
    int32_t              p1BatchNum = 1; // = maxFps / 30
    int32_t              opMode = 0;     // = StreamConfiguration.operationMode
    int32_t              logLevel = 0;   // from property
};

/**
 *  (Non Pass1-specific) Parsed stream info
 */
struct ParsedStreamInfo_NonP1
{
    /******************************************
     *  app meta stream info
     ******************************************/
    android::sp<IMetaStreamInfo>                pAppMeta_Control;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicP2StreamNode;
    // <sensor id, IMetaStreamInfo>
    std::unordered_map<uint32_t, android::sp<IMetaStreamInfo>>
                                                vAppMeta_DynamicP2StreamNode_Physical;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicP2CaptureNode;
    // <sensor id, IMetaStreamInfo>
    std::unordered_map<uint32_t, android::sp<IMetaStreamInfo>>
                                                vAppMeta_DynamicP2CaptureNode_Physical;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicFD;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicJpeg;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicRAW16;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicP1ISPPack;
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicP2ISPPack;
    // <sensor id, IMetaStreamInfo>
    std::unordered_map<uint32_t, android::sp<IMetaStreamInfo>>
                                                vAppMeta_DynamicP1ISPPack_Physical;
    // <sensor id, IMetaStreamInfo>
    std::unordered_map<uint32_t, android::sp<IMetaStreamInfo>>
                                                vAppMeta_DynamicP2ISPPack_Physical;
    // <sensor id, IMetaStreamInfo>
    std::unordered_map<uint32_t, android::sp<IMetaStreamInfo>>
                                                vAppMeta_DynamicRAW16_Physical;


    /******************************************
     *  hal meta stream info
     ******************************************/
    android::sp<IMetaStreamInfo>                pHalMeta_DynamicP2StreamNode;
    android::sp<IMetaStreamInfo>                pHalMeta_DynamicP2CaptureNode;
    android::sp<IMetaStreamInfo>                pHalMeta_DynamicPDE;


    /******************************************
     *  hal image stream info
     ******************************************/

    /**
     *  Face detection.
     */
    android::sp<IImageStreamInfo>               pHalImage_FD_YUV = nullptr;

    /**
     *  The Jpeg orientation is passed to HAL at the request stage.
     *  Maybe we can create a stream set for every orientation at the configuration stage, but only
     *  one within that stream set can be passed to the configuration of pipeline context.
     */
    android::sp<IImageStreamInfo>               pHalImage_Jpeg_YUV;

    /**
     *  The thumbnail size is passed to HAL at the request stage.
     */
    android::sp<IImageStreamInfo>               pHalImage_Thumbnail_YUV;

    /**
     *  HAL-level Jpeg stream info.
     *
     *  HAL-level Jpeg is used for debug and tuning on YUV capture
     *  (while App-level Jpeg is not configured).
     */
    android::sp<IImageStreamInfo>               pHalImage_Jpeg;

    /**
     *  The jpeg yuv used to store in main jpeg.
     */
    android::sp<IImageStreamInfo>               pHalImage_Jpeg_Sub_YUV = nullptr;

    /**
     *  The depth yuv used to store in main jpeg.
     */
    android::sp<IImageStreamInfo>               pHalImage_Depth_YUV = nullptr;

};


/**
 *  (Pass1-specific) Parsed stream info
 */
struct ParsedStreamInfo_P1
{
    /******************************************
     *  app meta stream info
     ******************************************/
    /**
     *  Only one of P1Node can output this data.
     *  Why do we need more than one of this stream?
     */
    android::sp<IMetaStreamInfo>                pAppMeta_DynamicP1 = nullptr;


    /******************************************
     *  hal meta stream info
     ******************************************/
    android::sp<IMetaStreamInfo>                pHalMeta_Control = nullptr;
    android::sp<IMetaStreamInfo>                pHalMeta_DynamicP1 = nullptr;


    /******************************************
     *  hal image stream info
     ******************************************/
    android::sp<IImageStreamInfo>               pHalImage_P1_Imgo = nullptr;
    android::sp<IImageStreamInfo>               pHalImage_P1_SeamlessImgo = nullptr;
    android::sp<IImageStreamInfo>               pHalImage_P1_Rrzo = nullptr;
    android::sp<IImageStreamInfo>               pHalImage_P1_Lcso = nullptr;
    android::sp<IImageStreamInfo>               pHalImage_P1_Rsso = nullptr;
    /******************************************
     *  hal image stream info for ISP6.0
     ******************************************/
    android::sp<IImageStreamInfo>               pHalImage_P1_FDYuv = nullptr;
    android::sp<IImageStreamInfo>               pHalImage_P1_ScaledYuv = nullptr;
    /******************************************
     *  hal image stream info for RssoR2
     ******************************************/
    android::sp<IImageStreamInfo>               pHalImage_P1_RssoR2 = nullptr;
};


/**
 *  Pipeline nodes need.
 *  true indicates its corresponding pipeline node is needed.
 */
struct PipelineNodesNeed
{
    /**
     * [Note]
     * The SensorId is shared, for example:
     *      PipelineStaticInfo->sensorId[x]
     *      needP1Node.at(sensorId)
     *      vNeedLaggingConfig.at(sensorId)
     */
    SensorMap<bool>                             needP1Node;

    bool                                        needP2StreamNode = false;
    bool                                        needP2CaptureNode = false;

    bool                                        needFDNode = false;

    bool                                        needJpegNode = false;
    bool                                        needRaw16Node = false;
    bool                                        needPDENode = false;

    bool                                        needP1RawIspTuningDataPackNode = false;
    bool                                        needP2YuvIspTuningDataPackNode = false;
    bool                                        needP2RawIspTuningDataPackNode = false;

    SensorMap<bool>                             vNeedLaggingConfig;

};


static inline android::String8 toString(const PipelineNodesNeed& o __unused)
{
    android::String8 os;
    os += "{ ";
    for (const auto& [_sensorId, _needP1] : o.needP1Node) {
        if ( _needP1 )  { os += android::String8::format("P1Node[%u] ", _sensorId); }
    }
    if ( o.needP2StreamNode )   { os += "P2StreamNode "; }
    if ( o.needP2CaptureNode )  { os += "P2CaptureNode "; }
    if ( o.needFDNode )         { os += "FDNode "; }
    if ( o.needJpegNode )       { os += "JpegNode "; }
    if ( o.needRaw16Node )      { os += "Raw16Node "; }
    if ( o.needPDENode )        { os += "PDENode "; }
    if ( o.needP1RawIspTuningDataPackNode )        { os += "P1RawIspTuningDataPackNode "; }
    if ( o.needP2YuvIspTuningDataPackNode )        { os += "P2YuvIspTuningDataPackNode "; }
    os += "}";
    return os;
};


/**
 *  Pipeline topology
 */
struct PipelineTopology
{
    using NodeSet       = NSCam::v3::pipeline::NSPipelineContext::NodeSet;
    using NodeEdgeSet   = NSCam::v3::pipeline::NSPipelineContext::NodeEdgeSet;

    /**
     * The root nodes of a pipeline.
     */
    NodeSet                                     roots;

    /**
     * The edges to connect pipeline nodes.
     */
    NodeEdgeSet                                 edges;

};


static inline android::String8 toString(const PipelineTopology& o __unused)
{
    android::String8 os;
    os += "{ ";
    os += ".root={";
    for( size_t i = 0; i < o.roots.size(); i++ ) {
        os += android::String8::format(" %#" PRIxPTR " ", o.roots[i]);
    }
    os += "}";
    os += ", .edges={";
    for( size_t i = 0; i < o.edges.size(); i++ ) {
        os += android::String8::format("(%#" PRIxPTR " -> %#" PRIxPTR ")",
            o.edges[i].src, o.edges[i].dst);
    }
    os += "}";
    return os;
};


/**
 * The buffer numbers of Hal P1 output streams (for feature settings)
 */
struct HalP1OutputBufferNum
{
    uint8_t       imgo = 0;
    uint8_t       rrzo = 0;
    uint8_t       lcso = 0;
    uint8_t       rsso = 0;      // reserver for streaming features
    uint8_t       scaledYuv = 0; // reserver for streaming features

};


static inline android::String8 toString(const HalP1OutputBufferNum& o __unused)
{
    android::String8 os;
    os += "{ ";
    os += android::String8::format(".imgo=%u ", o.imgo);
    os += android::String8::format(".rrzo=%u ", o.rrzo);
    os += android::String8::format(".lcso=%u ", o.lcso);
    os += android::String8::format(".rsso=%u ", o.rsso);
    os += android::String8::format(".scaledYuv=%u ", o.scaledYuv);
    os += "}";
    return os;
};


/**
 * Streaming feature settings
 */
struct StreamingFeatureSetting
{
    struct AppInfo
    {
        int32_t recordState = -1;
        uint32_t appMode = 0;
        uint32_t eisOn = 0;
    };

    AppInfo                                     mLastAppInfo;
    uint32_t                                    nr3dMode = 0;
    uint32_t                                    fscMode = 0;
    uint32_t                                    fscMaxMargin = 0;

    struct P1ConfigParam
    {
        uint32_t                                hdrHalMode = 0;
        uint32_t                                hdrSensorMode = 0;
        uint32_t                                aeTargetMode = 0;
        uint32_t                                validExposureNum = 0;
        uint32_t                                fusNum = 0;
    };
    SensorMap<P1ConfigParam>                    p1ConfigParam;
    SensorMap<uint32_t>                         batchSize;

    bool                                        bNeedLMV = false;
    bool                                        bNeedRSS = false;
    bool                                        bForceIMGOPool = false;
    bool                                        bIsEIS   = false;
    bool                                        bPreviewEIS = false;
    bool                                        bNeedLargeRsso = false;
    bool                                        bDisableInitRequest = false;
    bool                                        bEnableGyroMV = false;
    NSCam::EIS::EisInfo                         eisInfo;
    uint32_t                                    eisExtraBufNum = 0;
    uint32_t                                    minRrzoEisW    = 0;
    // final rrzo = calculated rrz * targetRrzoRatio
    // This ratio contains fixed margin (ex EIS) + max dynamic margin (ex FOV)
    // Currently only support fixed margin
    float                                       targetRrzoRatio = 1.0f;
    // Input = output * totalMarginRatio
    float                                       totalMarginRatio = 1.0f;
    float                                       rrzoHeightToWidth = 0;
    bool                                        bEnableTSQ     = false;
    uint32_t                                    BWCScenario = -1;
    uint32_t                                    BWCFeatureFlag = 0;
    uint32_t                                    dsdnHint = 0;
    uint32_t                                    dsdn30GyroEnable = 0;
    uint32_t                                    reqFps = 30;
    //
    /**
     * The additional buffer numbers of Hal P1 output streams which may be kept
     * and used by streaming features during processing a streaming request.
     */
    HalP1OutputBufferNum                        additionalHalP1OutputBufferNum;
    // hint support feature for dedicated scenario for P2 node init
    int64_t                                     supportedScenarioFeatures = 0; /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    // PQ/CZ/DRE/HFG
    bool                                        bSupportPQ     = false;
    bool                                        bSupportCZ     = false;
    bool                                        bSupportDRE     = false;
    bool                                        bSupportHFG     = false;
    // ISP6.0
    bool                                        bNeedP1FDYUV = false;
    bool                                        bNeedP1ScaledYUV = false;
    // HDR10
    bool                                        bIsHdr10 = false;
    uint32_t                                    hdr10Spec = 0;
    // 4K video
    MSize                                       aspectRatioFor4K = MSize(0, 0); /* ratio */
    uint32_t                                    fpsFor4K = 0;
    //
    StreamId_T                                  dispStreamId = -1;
    ParsedPIPInfo                               pipInfo;
};


static inline android::String8 toString(const StreamingFeatureSetting& o __unused)
{
    android::String8 os;
    os += "{ ";
    os += android::String8::format(".additionalHalP1OutputBufferNum=%s ", toString(o.additionalHalP1OutputBufferNum).c_str());
    os += android::String8::format(".supportedScenarioFeatures=%#" PRIx64 " ", o.supportedScenarioFeatures);
    os += android::String8::format(".disableInitRequest=%d .bForceIMGOPool=%d ", o.bDisableInitRequest, o.bForceIMGOPool);
    os += android::String8::format(".RRZ=%f .LMV=%d .RSS=(%d/%d) .PQ=%d .CZ=%d .DRE=%d .HFG=%d .DSDN=%d "
                                   ".EIS=%d .3DNR=%d .FSC=0x%x(maxMargin=%d) .HDR10=(%d/%d) "
                                   ".EISInfo(mode=%d,factor=%d,videoConfig=%d,tsq=%d,extra=%d,preview=%d) ",
                                   o.targetRrzoRatio, o.bNeedLMV, o.bNeedRSS, o.bNeedLargeRsso,
                                   o.bSupportPQ, o.bSupportCZ, o.bSupportDRE, o.bSupportHFG, o.dsdnHint,
                                   o.bIsEIS, o.nr3dMode, o.fscMode, o.fscMaxMargin, o.bIsHdr10, o.hdr10Spec,
                                   o.eisInfo.mode, o.eisInfo.factor, o.eisInfo.videoConfig, o.bEnableTSQ, o.eisExtraBufNum, o.bPreviewEIS);
    for (const auto& [_sensorId, _batchSize] : o.batchSize) {
        os += android::String8::format(".batch[%d](%d) ", _sensorId, _batchSize);
    }
    for (const auto& [_sensorId, _p1ConfigParam] : o.p1ConfigParam) {
        os += android::String8::format(".p1Config[%d]"
                                       "(hdrhal=0x%x,hdrsensor=0x%x,aeTarget=%d,expNum=%d,fusNum=%d) ",
                                       _sensorId, _p1ConfigParam.hdrHalMode, _p1ConfigParam.hdrSensorMode,
                                       _p1ConfigParam.aeTargetMode, _p1ConfigParam.validExposureNum,
                                       _p1ConfigParam.fusNum);
    }
    os += "}";
    return os;
};


/**
 * capture feature settings
 */
struct CaptureFeatureSetting
{
    /**
     * The additional buffer numbers of Hal P1 output streams which may be kept
     * and used by capture features during processing a capture request.
     */
    HalP1OutputBufferNum                        additionalHalP1OutputBufferNum;

    /**
     * The max. buffer number of App RAW16 output image stream.
     *
     * App RAW16 output image stream could be used for RAW16 capture or reprocessing.
     *
     * This value is usually used for
     * ParsedAppImageStreamInfo::vAppImage_Output_RAW16::setMaxBufNum.
     */
    uint32_t                                    maxAppRaw16OutputBufferNum = 1;

    /**
     */
    uint32_t                                    maxAppJpegStreamNum     = 1;

    // hint support feature for dedicated scenario for P2 node init
    uint64_t                                    supportedScenarioFeatures = 0; /*eFeatureIndexMtk and eFeatureIndexCustomer*/

    uint32_t                                     pluginUniqueKey           = 0;

    /**
     *  indicate the dualcam feature mode
     *
     *  E_STEREO_FEATURE_xxx in mtkcam3/include/mtkcam3/feature/stereo/StereoCamEnum.h
     */
    uint64_t                                    dualFeatureMode = 0;
};


static inline android::String8 toString(const CaptureFeatureSetting& o __unused)
{
    android::String8 os;
    os += "{ ";
    os += android::String8::format(".additionalHalP1OutputBufferNum=%s ", toString(o.additionalHalP1OutputBufferNum).c_str());
    os += android::String8::format(".maxAppRaw16OutputBufferNum=%u ", o.maxAppRaw16OutputBufferNum);
    os += android::String8::format(".maxAppJpegStreamNum=%u ", o.maxAppJpegStreamNum);
    os += android::String8::format(".supportedScenarioFeatures=%#" PRIx64 " ", o.supportedScenarioFeatures);
    os += "}";
    return os;
};


/**
 * Seamless switch feature settings
 */
struct SeamlessSwitchFeatureSetting
{
    // static info : query from sensor driver
    bool isInitted = false;
    int32_t defaultScene = -1;
    int32_t cropScene = -1;
    int32_t fullScene = -1;
    float cropZoomRatio = 0.0f;

    // user setting
    bool isSeamlessEnable = false;
    bool isSeamlessCaptureEnable = false;
    bool isSeamlessPreviewEnable = false;
};

static inline std::string toString(SeamlessSwitchFeatureSetting const& o)
{
    std::string str = "";
    str += "isInitted=" + std::to_string(o.isInitted) +
           ", defaultScene=" + std::to_string(o.defaultScene) +
           ", cropScene=" + std::to_string(o.cropScene) +
           ", fullScene=" + std::to_string(o.fullScene) +
           ", cropZoomRatio=" + std::to_string(o.cropZoomRatio) +
           ", isSeamlessEnable=" + std::to_string(o.isSeamlessEnable) +
           ", isSeamlessCaptureEnable=" + std::to_string(o.isSeamlessCaptureEnable) +
           ", isSeamlessPreviewEnable=" + std::to_string(o.isSeamlessPreviewEnable);
    return str;
};


/**
 * boost scenario control
 */
struct BoostControl
{
    int32_t                                    boostScenario = -1;
    uint32_t                                   featureFlag   = 0;
};

/**
 * ZSL policy
 */
struct ZslPolicyParams
{
    std::shared_ptr<ZslPolicy::ZslPluginParams> pZslPluginParams = nullptr;  // TODO: remove
    std::shared_ptr<ZslPolicy::ISelector>       pSelector = nullptr;
    bool                                        needPDProcessedRaw = false;
    bool                                        needFrameSync = false;
};

static inline std::string toString(ZslPolicyParams const& o)
{
    std::ostringstream oss;
    oss << "{"
        << " pluginParams=" << ZslPolicy::toString(o.pZslPluginParams)
        << " Selector=" << ZslPolicy::toString(o.pSelector)
        << " needPDProcRaw=" << (o.needPDProcessedRaw ? "Y" : "N")
        << " needFrameSync=" << (o.needFrameSync ? "Y" : "N")
        << " }";
    return oss.str();
};


/**
 * Request-stage policy for multicam
 *  $ Need to flush specific NodeId.
 *  $ Need to config specific NodeId.
 */
struct MultiCamReqOutputParams
{
    /**
     * In reconfig stage, if size not equal to zero,
     * it means need flush specific sensor id.
     */
    std::vector<uint32_t>                       flushSensorIdList;
    /**
     * In reconfig stage, if size not equal to zero,
     * it means need config specific node id.
     */
    std::vector<uint32_t>                       configSensorIdList;
    /**
     * use to set sync2a in switch sensor scenario
     */
    std::vector<uint32_t>                       switchControl_Sync2ASensorList;
    /**
     * list needs mark error buffer sensor id list. (For physical stream)
     */
    std::vector<uint32_t>                       markErrorSensorList;
    // [sensor status]
    std::vector<uint32_t>                       goToStandbySensorList;
    std::vector<uint32_t>                       streamingSensorList;
    std::vector<uint32_t>                       standbySensorList;
    std::vector<uint32_t>                       resumeSensorList;
    // [crop]
    std::unordered_map<uint32_t, MRect>         tgCropRegionList;
    std::unordered_map<uint32_t, MRect>         tgCropRegionList_Cap;
    std::unordered_map<uint32_t, MRect>         tgCropRegionList_3A;
    std::unordered_map<uint32_t, MRect>         sensorScalerCropRegionList;
    std::unordered_map<uint32_t, MRect>         sensorScalerCropRegionList_Cap;
    std::unordered_map<uint32_t, std::vector<MINT32>>         sensorAfRegionList;
    std::unordered_map<uint32_t, std::vector<MINT32>>         sensorAeRegionList;
    std::unordered_map<uint32_t, std::vector<MINT32>>         sensorAwbRegionList;

    bool need3ASwitchDoneNotify = false;
    bool needSync2A = false;
    bool needSyncAf = false;
    bool needFramesync = false;
    bool needSynchelper_3AEnq = false;
    bool needSynchelper_Timestamp = false;
    MUINT32 qualityFlag = (MUINT32)-1;

    // master id: decided by sensor control decision.
    uint32_t                                    masterId = (uint32_t)-1;
    // preview master id: current master id in streaming flow.
    uint32_t                                    prvMasterId = (uint32_t)-1;
    // for capture request
    std::vector<uint32_t>                       prvStreamingSensorList;
    // is logical raw capture
    bool                                        needLogicalRawOutput = false;
    // is zoomControlUpdate
    bool                                        isZoomControlUpdate = false;
    // is need force off streaming NR in slave cam.
    bool                                        isForceOffStreamingNr = false;
    // if use switch flow, need preque flow or not for this request
    bool                                        bSwitchNoPreque = false;
};

struct AdditionalRawInfo
{
    MINT32 format = -1;
    MINT32 stride = -1;
    MSize imgSize;
};

enum IspSwitchState
{
    Idle,
    Switching,
    Done,
};

enum AfSearchState
{
    kTouchAfNotSearching,  // Can switch sensor
    kTouchAfSearching,     // Touch Af still searching -> can't switch
    kCanceled,             // switch sensor if trigger is cancel
};
/******************************************************************************
 *
 ******************************************************************************/
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_TYPES_H_

