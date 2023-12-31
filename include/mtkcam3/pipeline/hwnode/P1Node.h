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

#ifndef _MTK_HARDWARE_INCLUDE_MTKCAM_V3_HWNODE_P1NODE_H_
#define _MTK_HARDWARE_INCLUDE_MTKCAM_V3_HWNODE_P1NODE_H_

#include <mtkcam3/pipeline/pipeline/IPipelineNode.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include <mtkcam3/pipeline/stream/IStreamBuffer.h>
#include <mtkcam3/pipeline/utils/streambuf/IStreamBufferPool.h>
#include <mtkcam3/pipeline/utils/SyncHelper/ISyncHelper.h>
#include <mtkcam/utils/hw/IResourceConcurrency.h>
#include <mtkcam/drv/def/ICam_type.h>
#include <mtkcam/utils/imgbuf/ISecureImageBufferHeap.h>
#include <vector>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {

/******************************************************************************
 *
 ******************************************************************************/
class P1Node
    : public virtual IPipelineNode
{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                            Definitions.
    typedef android::Vector<android::sp<IMetaStreamInfo> >  MetaStreamInfoSetT;
    typedef android::Vector<android::sp<IImageStreamInfo> > ImageStreamInfoSetT;
    typedef NSCam::v3::Utils::IStreamBufferPool<IImageStreamBuffer> IImageStreamBufferPoolT;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Definitions.

    typedef IPipelineNode::InitParams       InitParams;

    /**
     * The pipe mode for DRV normal pipe mode selection (EPipeSelect)
     */
    enum PIPE_MODE
    {
        PIPE_MODE_NORMAL           = 0,
        PIPE_MODE_NORMAL_SV,
    };

    /**
     * The receive mode for p1 receive the pipeline frame
     */
    enum REV_MODE
    {
        REV_MODE_NORMAL            = 0, // General case
        REV_MODE_CONSERVATIVE,          // SMVR, ...
        REV_MODE_AGGRESSIVE             // Reserved for development
    };

    /**
     * The resize quality setting
     */
    enum RESIZE_QUALITY
    {
        RESIZE_QUALITY_UNKNOWN      = 0,    // unknown/undefined
        RESIZE_QUALITY_L,                   // level low
        RESIZE_QUALITY_H,                   // level high
    };

    /**
     * The raw default type
     */
    enum RAW_DEF_TYPE
    {
        RAW_DEF_TYPE_PROCESSED_RAW  = 0x0000,   /*!< Processed raw */
        RAW_DEF_TYPE_PURE_RAW       = 0x0001,   /*!< Pure raw */
        RAW_DEF_TYPE_AUTO           = 0x000F    // if (rawProcessed ||
                                                //     post-proc-raw-unsupported)
                                                //     PROCESSED_RAW
                                                // else
                                                //     PURE_RAW
    };

    /**
     * The pre-config command
     */
    enum PRECONFIG_CMD : int32_t {
        PREQUEUE
    };

    /**
     * Sensor Parameters.
     */
    struct SensorParams
    {
        /* Refer to SENSOR_SCENARIO_ID_XXX in IHalSensor.h */
        MUINT       mode;
        MSize       size;
        MUINT       fps;
        MUINT32     pixelMode;
        /* Refer to SENSOR_VHDR_MODE_XXX in IHalSensor.h.
         * It is independant with SENSOR_SCENARIO_ID_XXX currently.
         * It depends on intersection of :
         * (1) app open hdr or not
         * (2) which vhdr mode sensor supported.
         */
        MUINT32     vhdrMode;

        SensorParams(): mode(0), size(), fps(0), pixelMode(0), vhdrMode(0)
        {}

        SensorParams(MUINT _mode, MSize _size, MUINT _fps, MUINT32 _pxlMode)
            : mode(_mode), size(_size), fps(_fps), pixelMode(_pxlMode), vhdrMode(0)
        {}

        SensorParams(MUINT _mode, MSize _size, MUINT _fps, MUINT32 _pxlMode, MUINT32 _vhdrMode)
            : mode(_mode), size(_size), fps(_fps), pixelMode(_pxlMode), vhdrMode(_vhdrMode)
        {}

    };


    /**
     * Configure Parameters.
     */
    struct  ConfigParams
    {
        ConfigParams()
            : pInAppMeta(NULL)
            , pInHalMeta(NULL)
            , pInImage_opaque(NULL)
            , pInImage_raw(NULL)
            , pInImage_yuv(NULL)
            , pOutAppMeta(NULL)
            , pOutHalMeta(NULL)
            , pOutImage_resizer(NULL)
            , pOutImage_lcso(NULL)
            , pOutImage_rsso(NULL)
            , pOutImage_mono(NULL)
            , pOutImage_full_seamless_HAL(NULL)
            , pOutImage_full_HAL(NULL)
            , pOutImage_full_APP(NULL)
            , pOutImage_opaque(NULL)
            , pOutImage_yuv_full(NULL)
            , pOutImage_yuv_resizer1(NULL)
            , pOutImage_yuv_resizer2(NULL)
            , pStreamPool_resizer(NULL)
            , pStreamPool_lcso(NULL)
            , pStreamPool_rsso(NULL)
            , pStreamPool_full(NULL)
            , pStreamPool_yuv_full(NULL)
            , pStreamPool_yuv_resizer1(NULL)
            , pStreamPool_yuv_resizer2(NULL)
            , pResourceConcurrency(NULL)
            , pSyncHelper(NULL)
            , pipeMode(PIPE_MODE_NORMAL)
            , skipSensorConfig(MFALSE)
            , pipeBit(NSCam::NSIoPipe::NSCamIOPipe::CAM_Pipeline_12BITS)
            , camResConfig()
            , resizeQuality(RESIZE_QUALITY_UNKNOWN)
            , tgNum(0)
            , batchNum(0)
            , burstNum(0)
            , groupNum(0)
            , staggerNum(0)
            , laggingLaunch(MFALSE)
            , initRequest(0)
            , receiveMode(REV_MODE_NORMAL)
            , rawProcessed(MFALSE)
            , rawDefType(RAW_DEF_TYPE_AUTO)
            , disableFrontalBinning(MFALSE)
            , disableDynamicTwin(MTRUE)
            , disableHLR(MFALSE)
            , enableUNI(MFALSE)
            , bImgoOnly(MFALSE)
            , enableEIS(MFALSE)
            , enableLCS(MFALSE)
            , enableRSS(MFALSE)
            , enableFSC(MFALSE)
            , enableDualPD(MFALSE)
            , enableQuadCode(MFALSE)
            , enableSecurity(MFALSE)
            , secType(SecType::mem_normal)
            , statusSecHeap(nullptr)
            , enableCaptureFlow(MFALSE)
            , enableFrameSync(MFALSE)
            , forceSetEIS(MFALSE)
            , stereoCamMode(MFALSE)
            , packedEisInfo(0)
            , bAospOpaque(MTRUE)
            , bEnableGyroMV(MFALSE)
            , openId(0)
            {
                sensorParams.mode = 0;
                sensorParams.size = MSize(0, 0);
                sensorParams.fps = 0;
                sensorParams.pixelMode = 0;
                //CAM_LOGI("ConfigParams() creation");
            };
        /**
         * A pointer to input meta stream info.
         */
        android::sp<IMetaStreamInfo> pInAppMeta;

        /**
         * A pointer to input meta stream info.
         */
        android::sp<IMetaStreamInfo> pInHalMeta;

        /**
         * A pointer to input image stream info. (full-zsl input port)
         */
        android::sp<IImageStreamInfo> pInImage_opaque;

        /**
         * A pointer to input image stream info. (raw-in input port)
         */
        android::sp<IImageStreamInfo> pInImage_raw;

        /**
         * A pointer to input image stream info. (yuv input port)
         */
        android::sp<IImageStreamInfo> pInImage_yuv;

        /**
         * A pointer to output meta stream info.
         */
        android::sp<IMetaStreamInfo> pOutAppMeta;

        /**
         * A pointer to output meta stream info.
         */
        android::sp<IMetaStreamInfo> pOutHalMeta;

        /**
         * A pointer to output image stream info. (resizer output port)
         */
        android::sp<IImageStreamInfo> pOutImage_resizer;

        /**
         * A pointer to output image stream info. (lcs output port)
         */
        android::sp<IImageStreamInfo> pOutImage_lcso;

        /**
         * A pointer to output image stream info. (rss output port)
         */
        android::sp<IImageStreamInfo> pOutImage_rsso;

        /**
         * A pointer to output image stream info. (mono output port)
         */
        android::sp<IImageStreamInfo> pOutImage_mono;

        /**
         * A pointer to output image stream info. (full ouput port: seamless imgo)
         */
        android::sp<IImageStreamInfo> pOutImage_full_seamless_HAL;

        /**
         * A pointer to output image stream info. (full ouput port)
         */
        android::sp<IImageStreamInfo> pOutImage_full_HAL;

        /**
         * A pointer to output image stream info. (full ouput port)
         */
        android::sp<IImageStreamInfo> pOutImage_full_APP;

        /**
         * A pointer to output image stream info. (full-zsl output port)
         */
        android::sp<IImageStreamInfo> pOutImage_opaque;

        /**
         * A pointer to output image stream info. (full-zsl output port)
         */
        android::sp<IImageStreamInfo> pOutImage_yuv_full;

        /**
         * A pointer to output image stream info. (resizer-yuv 1 output port)
         */
        android::sp<IImageStreamInfo> pOutImage_yuv_resizer1;

        /**
         * A pointer to output image stream info. (resizer-yuv 2 output port)
         */
        android::sp<IImageStreamInfo> pOutImage_yuv_resizer2;

        /**
         * A pointer to sensor parameter. Must Have.
         */
        SensorParams                 sensorParams;

        /**
         * The list of available target sensor modes for seamless switch.
         */
        std::vector<MUINT32>              seamlessSensorModes;

        /**
         * Max Resolution of sensor modes.
         */
        MSize                             maxResolution;

        /**
         * A pointer to full output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_resizer;

        /**
         * A pointer to lcs output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_lcso;

        /**
         * A pointer to rss output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_rsso;

        /**
         * A pointer to resizer output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_full;

        /**
         * A pointer to full output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_yuv_full;

        /**
         * A pointer to yuv resizer output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_yuv_resizer1;

        /**
         * A pointer to yuv resizer output image stream info.
         */
        android::sp<IImageStreamBufferPoolT> pStreamPool_yuv_resizer2;

        /**
         * A pointer to the resource concurrency control.
         */
        android::sp<IResourceConcurrency> pResourceConcurrency;

        /**
         * A pointer to the sync helper module.
         */
        android::sp<NSCam::v3::Utils::Imp::ISyncHelper> pSyncHelper;

        /**
         * The metadata pass to 3A as NS3Av3::ConfigInfo_T::CfgAppMeta.
         */
        NSCam::IMetadata cfgAppMeta;

        /**
         * The metadata pass to 3A as NS3Av3::ConfigInfo_T::CfgHalMeta.
         */
        NSCam::IMetadata cfgHalMeta;

        /**
         * The enum for DRV normal pipe mode selection (EPipeSelect).
         */
        PIPE_MODE pipeMode;

        /**
         * A boolean to skip sensor configure: default value is false.
         * This control is available while ENPipeQueryCmd_NEED_CFG_SENSOR_BY_MW is
         * supported in DRV
         */
        MBOOL skipSensorConfig;

        /**
         * The enum (E_CAM_PipelineBitDepth_SEL) for DRV pipeline raw bit depth.
         */
        MUINT32 pipeBit;

        /**
         * The definition (CAM_RESCONFIG) of DRV for Camera Resource Configure.
         */
        CAM_RESCONFIG camResConfig;

        /**
         * The resize quality level for DRV to control the frontal binning.
         * RESIZE_QUALITY_H : DRV will try to disable frontal binning
         * RESIZE_QUALITY_L : DRV will try to enable frontal binning
         * RESIZE_QUALITY_UNKNOWN, DRV will reference the disableFrontalBinning
         */
        RESIZE_QUALITY resizeQuality;

        /**
         * The number of TG as configure: default value is 0 for auto assign.
         */
        MUINT8 tgNum;

        /**
         * The number of batch processing: default value is 0.
         */
        MUINT8 batchNum;

        /**
         * The number of burst processing: default value is 0.
         */
        MUINT8 burstNum;

        /**
         * The number of group processing: default value is 0.
         * (groupNum <= 1): no grouping
         * (groupNum > 1): max grouping number; it use grouping but it may change per-frame.
         * For example, when groupNum = 2
         * We check grouping per-frame via MTK_3A_FEATURE_AE_EXPOSURE_LEVEL of control hal metadata
         * (empty or MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_NONE) : 1/1 (normal flow; bypass fusion)
         * (MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_SHORT): 1/2
         * (MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_NORMAL): 2/2
         */
        MUINT8 groupNum;

        /**
         * The number for stagger processing: default value is 0.
         * (staggerNum <= 1): not with stagger processing
         * (staggerNum > 1): max stagger number
         */
        MUINT8 staggerNum;

        /**
         * A boolean to enable the lagging launch flow,
         * default value is false.
         * If set it true, the related modules will really start while the first
         * frame/request received by  queue().
         * DO NOT set it true while (enableCaptureFlow == true) or
         * (initRequest > 0).
         *
         * If (laggingLaunch is true)
         * and (ENPipeQueryCmd_NEED_CFG_SENSOR_BY_MW is supported in DRV)
         * and (skipSensorConfig is false)
         * then it will execute sensor-configure while config()
         */
        MBOOL laggingLaunch;

        /**
         * The number of init request set : default value is 0.
         * (initRequest = 0) : disable the init request flow
         * (initRequest > 0) : enable the init request flow
         * and it need initRequest request-sets before DRV start
         */
        MUINT8 initRequest;

        /**
         * The receive mode while the pipeline frame arrival,
         * default value is REV_MODE_NORMAL.
         */
        REV_MODE receiveMode;

        /**
         * A boolean to enable the processed raw type of full path.
         * It will be ignored while the platform post-proc-raw-unsupported.
         */
        MBOOL rawProcessed;

        /**
         * The raw default type for a request did not set its raw type.
         * if (post-proc-raw-unsupported)
         *     it should be TYPE_PROCESSED_RAW/TYPE_AUTO
         * else if (rawProcessed)
         *     it should be TYPE_PROCESSED_RAW/TYPE_PURE_RAW/TYPE_AUTO
         * else
         *     it should be TYPE_PURE_RAW/TYPE_AUTO
         */
        RAW_DEF_TYPE rawDefType;

        /**
         * A boolean to force to disable the frontal binning.
         */
        MBOOL disableFrontalBinning;

        /**
         * A boolean to force to disable the dynamic twin mode.
         * True: forced-off while platform turn-off supported
         * False: auto, decide by the platform capability
         */
        MBOOL disableDynamicTwin;

        /**
         * A boolean to force to disable the HLR.
         * True:forced-off False:auto
         */
        MBOOL disableHLR;

        /**
         * A boolean to force to enable the UNI.
         * True:forced-on False:auto
         */
        MBOOL enableUNI;


        /**
         * A hint to set data pattern to eCAM_BYPASS.
         */
        MBOOL bImgoOnly;

        /**
         * A boolean to enable the EIS related function.
         */
        MBOOL enableEIS;

        /**
         * A boolean to enable the LCS related function.
         */
        MBOOL enableLCS;

        /**
         * A boolean to enable the RSS related function.
         */
        MBOOL enableRSS;

        /**
         * A boolean to enable the FSC related function.
         */
        MBOOL enableFSC;

        /**
         * A boolean to enable Dual PD.
         */
        MBOOL enableDualPD;

        /**
         * A boolean to enable Quad-Code.
         */
        MBOOL enableQuadCode;

        /**
         * A boolean to enable Security flow.
         */
        MBOOL enableSecurity;

        /**
         * A enum to identify Security flow.
         */
        SecType secType;

        /**
         * A Secure handle to identify data source
         */
        std::shared_ptr<IImageBufferHeap> statusSecHeap;

        /**
         * A boolean to enable optimized capture flow.
         */
        MBOOL enableCaptureFlow;

        /**
         * A boolean indicates whether this sensor needs to be synchronized with
         * another one. Set to true to turn on hwsync module
         */
        MBOOL enableFrameSync;

        /**
         * A boolean to force set the EIS related function.
         */
        MBOOL forceSetEIS;

        /**
         * A boolean indicates whether it is in stereo cam mode,
         * there may have some constraints needs to to applied
         */
        MBOOL stereoCamMode;

        /**
         An integer to store packed EIS info
         */
        MUINT64 packedEisInfo;

        /**
         A hint for opaque flow.
         User can choose not to use default opaque flow
         ex : ISP HIDL not use AOSP opaque flow
         */
        MBOOL bAospOpaque;

        /**
         Currently cannot combine SMVR due to exposure time accuracy concern
         */
        MBOOL bEnableGyroMV;

        /*
         An integer to store virtual device open ID
        */
        MINT32 openId;
    };


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Operations.
    static  android::sp<P1Node>     createInstance();

    virtual MERROR                  config(ConfigParams const& rParams) = 0;

    virtual MERROR                  init(InitParams const& rParams)= 0;

};

};
};

#endif
