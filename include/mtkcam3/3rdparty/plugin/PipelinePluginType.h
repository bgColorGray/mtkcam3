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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_PLUGIN_PIPELINEPLUGINTYPE_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_PLUGIN_PIPELINEPLUGINTYPE_H_

#include <utils/BitSet.h>

#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/Reflection.h>

// feature index for pipeline plugin
#include <mtkcam3/3rdparty/mtk/mtk_feature_type.h>
#include <mtkcam3/3rdparty/customer/customer_feature_type.h>

// policy and feature related info
#include <mtkcam3/def/zslPolicy/types.h>
#include <mtkcam3/pipeline/policy/types.h>
#include <mtkcam/drv/IHalSensor.h> // sensopr mode define
#include <mtkcam/utils/hw/IScenarioControlV3.h> //boost control
#include "TPICustomInfo.h"


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace NSPipelinePlugin {


/******************************************************************************
 * Common
 ******************************************************************************/

enum ThumbnailTiming
{
    eTiming_P2_Begin,
    eTiming_P2,
    eTiming_Fusion,
    eTiming_Depth,
    eTiming_MultiFrame,
    eTiming_Bokeh,
    eTiming_MDP,
    // above enum item are node timing
    eTiming_Provider_01,
    eTiming_Provider_02,
    eTiming_Provider_03
};

enum Priority
{
    ePriority_Lowest    = 0x00,
    ePriority_Normal    = 0x10,
    ePriority_Default   = 0x80,
    ePriority_Highest   = 0xFF,
};

enum FaceData
{
    eFD_None,
    eFD_Cache,
    eFD_Current,
};

enum InitPhase
{
    ePhase_OnPipeInit,
    ePhase_OnRequest,
};

enum SelStage
{
    eSelStage_CFG,
    eSelStage_P1,
    eSelStage_P2,
};

enum JoinEntry
{
    eJoinEntry_S_YUV,
    eJoinEntry_S_RAW,
    eJoinEntry_S_ASYNC,
    eJoinEntry_S_DISP_ONLY,
    eJoinEntry_S_META_ONLY,
    eJoinEntry_S_DIV_2,
};

enum AsyncType
{
    eAsyncType_FIX_RATE,
    eAsyncType_WAITING,
    eAsyncType_POLLING,
};

enum JoinQueueCmd
{
    eJoinQueueCmd_NONE,
    eJoinQueueCmd_PUSH,
    eJoinQueueCmd_PUSH_POP,
    eJoinQueueCmd_POP_ALL,
};

enum JoinImageID
{
    eJoinImageID_DEFAULT,
    eJoinImageID_MAIN1,
    eJoinImageID_MAIN2,
};

typedef struct JoinImageInfo_t
{
    MUINT32         mISensorID = (MUINT32)-1;
    MSize           mISensorSize;
    MRectF          mISensorClip;
    MRectF          mISrcZoomROI;

    JoinImageID     mOSrcImageID = eJoinImageID_DEFAULT;
    MRectF          mOSrcImageClip;
    MRectF          mODstImageClip;
    MRectF          mODstZoomROI;

    MBOOL           mOUseSrcImageBuffer = false;
} JoinImageInfo;

typedef struct BufferInfo_t
{
    EImageFormat mFormat = NSCam::eImgFmt_UNKNOWN;
    MSize mSize;
    MUINT32 mStride = 0;
} BufferInfo;

enum PreProcess
{
    ePreProcess_None,
    ePreProcess_NR,
};

enum Boost
{
    eBoost_None = 0x0,
    eBoost_CPU  = 0x1 << 0
};

typedef enum IspHidlStage
{
    ISP_HIDL_STAGE_DISABLED = 0,
    ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA,
    ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL,
} IspHidlStage;

struct Policy {
    // Current camera state for plugin during negotiation
    struct State {
        MBOOL                   mZslPoolReady = MFALSE;
        MBOOL                   mZslRequest   = MFALSE;
        MBOOL                   mFlashFired   = MFALSE;
        MBOOL                   mAppManual3A  = MFALSE;
        MUINT32                 mExposureTime = -1;
        MUINT32                 mRealIso      = -1;
        MINT32                  mSensorMode   = SENSOR_SCENARIO_ID_UNNAMED_START;
        MSize                   mSensorSize   = MSize(0,0);

        // The App request image buffers type and purpose for feature plugins execute suitable behavior.
        NSCam::v3::pipeline::policy::ParsedAppImageStreamInfo const* pParsedAppImageStreamInfo = nullptr;

        // DualCam indicated Yuv Size Info (P2ANode will output this size for yuv plugin)
        MSize                   mDualCamDedicatedYuvSize = MSize(0,0);
        MRect                   mDualCamDedicatedFov     = MRect(MPoint(0, 0), MSize(0, 0));
        MINT32                  mMultiCamFeatureMode     = -1;

        // hdr mode
        MUINT32                 mHDRHalMode = 0;
        MINT32                  mValidExpNum = 0;

        // available system free memory in mega bytes
        MINT64                  mFreeMemoryMBytes = -1;
    };

    // Plugin retruns the decision of camera setting
    struct Decision {
        using ZslPluginParams = NSCam::ZslPolicy::ZslPluginParams;
        using BoostControl    = NSCam::v3::pipeline::policy::BoostControl;

        MINT32                  mSensorMode     = SENSOR_SCENARIO_ID_UNNAMED_START;
        MBOOL                   mZslEnabled     = MFALSE;
        ZslPluginParams         mZslPolicy;  // TODO: deprecated, replace with ISelector
        std::shared_ptr<ZslPolicy::ISelector> pZslSelector = nullptr;
        // only decision frames requirement and no need to execute plugin process if mProcess=MFALSE.
        MBOOL                   mProcess        = MTRUE;
        // boost config for BWC (IScenarioControlV3)
        BoostControl            mBoostControl;
        // Indicate whether to use unpack raw or not
        MBOOL                   mNeedUnpackRaw  = MFALSE;
        // Indicate whether to drop previous preview request for speed-up the request's exposure behavior
        // if the feature support & required.
        bool                    mDropPreviousPreviewRequest  = false;
    };

    // Current ISP state for plugin during negotiation
    struct StateIspHidl {
        uint32_t                           sensorId = 0;
        uint32_t                           requestNo = 0;

        // Request's input frame count(N) from App, and the input frame index(0 ~ N-1)
        int8_t                             frameCount = -1;
        int8_t                             frameIndex = -1;

        // Input YUV(ex:NV21) format stream for reprocessing.
        android::sp<NSCam::v3::IImageStreamInfo>      pAppImage_Input_Yuv = nullptr;
        // Input RAW16(ex:10bit, 12bit, ...) format stream for reprocessing.
        android::sp<NSCam::v3::IImageStreamInfo>      pAppImage_Input_RAW16 = nullptr;
        // Input private RAW format stream for reprocessing
        android::sp<NSCam::v3::IImageStreamInfo>      pAppImage_Input_Priv = nullptr;

        // Output image format stream.
        android::sp<NSCam::v3::IImageStreamInfo>      pAppImage_Output_RAW16 = nullptr;
        android::sp<NSCam::v3::IImageStreamInfo>      pAppImage_Output_Yuv = nullptr;
        android::sp<NSCam::v3::IImageStreamInfo>      pAppImage_Output_Jpeg = nullptr;

        // available system free memory in mega bytes
        MINT64                  mFreeMemoryMBytes = -1;
    };

    // Plugin retruns the decision of ISP processing setting
    struct DecisionIspHidl {
    };
};

struct StreamInfo {
    MSize mMaxOutSize;
    MSize mStreamingSize;
};

/******************************************************************************
 * RAW Interface
 ******************************************************************************/
class Raw {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures,
        (MUINT8)                mPriority,
        (MUINT8)                mInitPhase,
        (MBOOL)                 mInPlace)

        Property() : mName(0)
                    ,mFeatures(0)
                    ,mPriority(ePriority_Lowest)
                    ,mInitPhase(ePhase_OnRequest)
                    ,mInPlace(0)
        {
        }
    };

    struct Selection {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferSelection)       mIBufferFull,
        (BufferSelection)       mOBufferFull,
        (MetadataSelection)     mIMetadataDynamic,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal)
        IspHidlStage            mIspHidlStage = ISP_HIDL_STAGE_DISABLED;
        Policy::State           mState;
        Policy::Decision        mDecision;
        Policy::StateIspHidl    mStateIspHidl;
        Policy::DecisionIspHidl mDecisionIspHidl;
        bool                    mAlignInputFmt = false;
    };

    struct Request {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferHandle::Ptr)     mIBufferFull,
        (BufferHandle::Ptr)     mOBufferFull,
        (MetadataHandle::Ptr)   mIMetadataDynamic,
        (MetadataHandle::Ptr)   mIMetadataApp,
        (MetadataHandle::Ptr)   mIMetadataHal,
        (MetadataHandle::Ptr)   mOMetadataApp,
        (MetadataHandle::Ptr)   mOMetadataHal)
    };

    struct ConfigParam {};

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<Raw> RawPlugin;

/******************************************************************************
 * Multi-Frame Interface
 ******************************************************************************/
class MultiFrame {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures,
        (MUINT8)                mZsdBufferMaxNum,  // maximum full raw/yuv buffer requirement
        (MBOOL)                 mNeedRrzoBuffer,   // rrzo requirement
        (MUINT32)               mThumbnailDelay,
        (MUINT8)                mThumbnailTiming,
        (MUINT8)                mFaceData,
        (MUINT8)                mPriority,
        (MUINT8)                mInitPhase,
        (MUINT8)                mBoost)

        Property() : mName(0)
                    ,mFeatures(0)
                    ,mZsdBufferMaxNum(0)
                    ,mNeedRrzoBuffer(MTRUE) //TODO: modify default value to MFALSE after ZSL support partial P1 buffer request
                    ,mThumbnailDelay(0)
                    ,mThumbnailTiming(eTiming_P2)
                    ,mFaceData(0)
                    ,mPriority(ePriority_Lowest)
                    ,mInitPhase(ePhase_OnRequest)
                    ,mBoost(eBoost_None)
        {
        }
    };

    struct Selection {
        FIELDS(
        (MINT32)                mSensorId,
        (MUINT8)                mRequestIndex,      // [Caller]
        (MUINT8)                mRequestCount,      // [Provider] multiple frame count
        (BufferSelection)       mIBufferFull,
        (BufferSelection)       mIBufferSpecified,
        (BufferSelection)       mIBufferResized,
        (BufferSelection)       mIBufferLCS,
        (BufferSelection)       mIBufferLCESHO,
        (BufferSelection)       mIBufferDCES,
        (BufferSelection)       mOBufferFull,
        (BufferSelection)       mOBufferThumbnail,
        (MetadataSelection)     mIMetadataDynamic,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal,
        (MUINT8)                mFrontDummy,        // [Provider] the dummy frame count before this frame
        (MUINT8)                mPostDummy)         // [Provider] the dummy frame count after this frame
        IspHidlStage            mIspHidlStage = ISP_HIDL_STAGE_DISABLED;
        Policy::State           mState;
        Policy::Decision        mDecision;
        Policy::StateIspHidl    mStateIspHidl;
        Policy::DecisionIspHidl mDecisionIspHidl;
        MBOOL                   mProcessRaw = MFALSE;
    };

    struct Request {
        FIELDS(
        (MINT32)                mSensorId,
        (MUINT8)                mRequestIndex,
        (MUINT8)                mRequestCount,
        (MINT8)                 mRequestBSSIndex,
        (MINT8)                 mRequestBSSCount,
        (BufferHandle::Ptr)     mIBufferFull,
        (BufferHandle::Ptr)     mIBufferSpecified,
        (BufferHandle::Ptr)     mIBufferResized,
        (BufferHandle::Ptr)     mIBufferLCS,
        (BufferHandle::Ptr)     mIBufferLCESHO,
        (BufferHandle::Ptr)     mIBufferDCES,
        (BufferHandle::Ptr)     mOBufferFull,
        (MetadataHandle::Ptr)   mIMetadataDynamic,
        (MetadataHandle::Ptr)   mIMetadataApp,
        (MetadataHandle::Ptr)   mIMetadataHal,
        (MetadataHandle::Ptr)   mOMetadataApp,
        (MetadataHandle::Ptr)   mOMetadataHal)

        Request() : mSensorId(-1), mRequestIndex(0), mRequestCount(0), mRequestBSSIndex(-1), mRequestBSSCount(0) // when no bss needed, order is -1,
        {
        }
    };

    struct ConfigParam {};

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<MultiFrame> MultiFramePlugin;

/******************************************************************************
 * Fusion Interface
 ******************************************************************************/
class Fusion {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures,
        (MUINT8)                mPriority,
        (MUINT8)                mInitPhase,
        (MUINT8)                mThumbnailTiming,
        (MUINT8)                mFaceData,
        (MUINT8)                mBoost)

        Property() : mName(0)
                    ,mFeatures(0)
                    ,mPriority(ePriority_Lowest)
                    ,mInitPhase(ePhase_OnRequest)
                    ,mThumbnailTiming(eTiming_P2)
                    ,mFaceData(eFD_None)
                    ,mBoost(eBoost_None)
        {
        }
    };

    struct Selection {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferSelection)       mIBufferFull,
        (BufferSelection)       mIBufferResized,
        (BufferSelection)       mIBufferFull2,
        (BufferSelection)       mIBufferResized2,
        (BufferSelection)       mOBufferFull,
        (BufferSelection)       mOBufferDepth,
        (BufferSelection)       mOBufferThumbnail,
        (MetadataSelection)     mIMetadataDynamic,
        (MetadataSelection)     mIMetadataDynamic2,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal,
        (MetadataSelection)     mIMetadataHal2,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal)
    };

    struct Request {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferHandle::Ptr)     mIBufferFull,
        (BufferHandle::Ptr)     mIBufferResized,
        (BufferHandle::Ptr)     mIBufferFull2,
        (BufferHandle::Ptr)     mIBufferResized2,
        (BufferHandle::Ptr)     mOBufferFull,
        (BufferHandle::Ptr)     mOBufferDepth,
        (BufferHandle::Ptr)     mOBufferThumbnail,
        (MetadataHandle::Ptr)   mIMetadataDynamic,
        (MetadataHandle::Ptr)   mIMetadataDynamic2,
        (MetadataHandle::Ptr)   mIMetadataApp,
        (MetadataHandle::Ptr)   mIMetadataHal,
        (MetadataHandle::Ptr)   mIMetadataHal2,
        (MetadataHandle::Ptr)   mOMetadataApp,
        (MetadataHandle::Ptr)   mOMetadataHal)
    };

    struct ConfigParam {};

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<Fusion> FusionPlugin;

/******************************************************************************
 * Depth Interface
 ******************************************************************************/
class Depth {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures,
        (MUINT8)                mThumbnailTiming,
        (MUINT8)                mPriority,
        (MUINT8)                mInitPhase,
        (MUINT8)                mFaceData,
        (MUINT8)                mPreprocess,
        (MUINT8)                mBoost)

        Property() : mName(0)
                    ,mFeatures(0)
                    ,mThumbnailTiming(eTiming_P2)
                    ,mPriority(ePriority_Lowest)
                    ,mInitPhase(ePhase_OnRequest)
                    ,mFaceData(eFD_None)
                    ,mPreprocess(ePreProcess_None)
                    ,mBoost(eBoost_None)
        {
        }
    };

    struct Selection {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferSelection)       mIBufferFull,
        (BufferSelection)       mIBufferFull2,
        (BufferSelection)       mIBufferResized,
        (BufferSelection)       mIBufferResized2,
        (BufferSelection)       mIBufferLCS,
        (BufferSelection)       mIBufferLCESHO,
        (BufferSelection)       mIBufferLCS2,
        (BufferSelection)       mIBufferLCESHO2,
        (BufferSelection)       mOBufferDepth,
        (BufferSelection)       mOBufferThumbnail,
        (MetadataSelection)     mIMetadataDynamic,
        (MetadataSelection)     mIMetadataDynamic2,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal,
        (MetadataSelection)     mIMetadataHal2,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal)
    };

    struct Request {
        FIELDS(
        (MINT32)                  mSensorId,
        (BufferHandle::Ptr)       mIBufferFull,
        (BufferHandle::Ptr)       mIBufferFull2,
        (BufferHandle::Ptr)       mIBufferResized,
        (BufferHandle::Ptr)       mIBufferResized2,
        (BufferHandle::Ptr)       mIBufferLCS,
        (BufferHandle::Ptr)       mIBufferLCESHO,
        (BufferHandle::Ptr)       mIBufferLCS2,
        (BufferHandle::Ptr)       mIBufferLCESHO2,
        (BufferHandle::Ptr)       mOBufferDepth,
        (BufferHandle::Ptr)       mOBufferThumbnail,
        (MetadataHandle::Ptr)     mIMetadataDynamic,
        (MetadataHandle::Ptr)     mIMetadataDynamic2,
        (MetadataHandle::Ptr)     mIMetadataApp,
        (MetadataHandle::Ptr)     mIMetadataHal,
        (MetadataHandle::Ptr)     mIMetadataHal2,
        (MetadataHandle::Ptr)     mOMetadataApp,
        (MetadataHandle::Ptr)     mOMetadataHal)
    };

    struct ConfigParam {};

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<Depth> DepthPlugin;

/******************************************************************************
 * Bokeh Interface
 ******************************************************************************/
class Bokeh {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures,
        (MUINT8)                mThumbnailTiming,
        (MUINT8)                mPriority,
        (MUINT8)                mInitPhase,
        (MUINT8)                mFaceData,
        (MUINT8)                mBoost)

        Property() : mName(0)
                    ,mFeatures(0)
                    ,mThumbnailTiming(eTiming_P2)
                    ,mPriority(ePriority_Lowest)
                    ,mInitPhase(ePhase_OnRequest)
                    ,mFaceData(eFD_None)
                    ,mBoost(eBoost_None)
        {
        }
    };

    struct Selection {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferSelection)       mIBufferFull,
        (BufferSelection)       mIBufferDepth,
        (BufferSelection)       mOBufferFull,
        (BufferSelection)       mOBufferThumbnail,
        (MetadataSelection)     mIMetadataDynamic,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal)
    };

    struct Request {
        FIELDS(
        (MINT32)                mSensorId,
        (BufferHandle::Ptr)     mIBufferFull,
        (BufferHandle::Ptr)     mIBufferDepth,
        (BufferHandle::Ptr)     mOBufferFull,
        (BufferHandle::Ptr)     mOBufferThumbnail,
        (MetadataHandle::Ptr)   mIMetadataDynamic,
        (MetadataHandle::Ptr)   mIMetadataApp,
        (MetadataHandle::Ptr)   mIMetadataHal,
        (MetadataHandle::Ptr)   mOMetadataApp,
        (MetadataHandle::Ptr)   mOMetadataHal)
    };

    struct ConfigParam {};

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<Bokeh> BokehPlugin;


/******************************************************************************
 * YUV Interface
 ******************************************************************************/

class Yuv {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures,
        (MBOOL)                 mInPlace,
        (MUINT8)                mFaceData,
        (MUINT8)                mInitPhase,       // init at pipeline init
        (MUINT8)                mPriority,
        (MUINT8)                mPosition,        // YUV plugin point: 0->YUV, 1->YUV2
        (MBOOL)                 mMultiFrame
        )

        Property() : mName(0)
                    ,mFeatures(0)
                    ,mInPlace(MFALSE)
                    ,mFaceData(0)
                    ,mInitPhase(ePhase_OnRequest)
                    ,mPriority(ePriority_Lowest)
                    ,mPosition(0)
                    ,mMultiFrame(MFALSE)
        {
        }
    };

    struct Selection {
        FIELDS(
        (MINT32)                mSensorId,
        (MBOOL)                 mIsMultiCamVSDoFMode,    // [Caller]
        (MBOOL)                 mMainFrame,
        (MBOOL)                 mIsPhysical,
        (MBOOL)                 mIsDualMode,
        (MBOOL)                 mIsBayerBayer,
        (MBOOL)                 mIsBayerMono,
        (MBOOL)                 mIsMasterIndex,
        (BufferSelection)       mIBufferFull,
        (BufferSelection)       mIBufferClean,
        (BufferSelection)       mIBufferResized,
        (BufferSelection)       mIBufferDepth,
        (BufferSelection)       mIBufferDCES,
        (BufferSelection)       mIBufferLCS,
        (BufferSelection)       mIBufferLCESHO,
        (BufferSelection)       mOBufferFull,
        (MetadataSelection)     mIMetadataDynamic,
        (MetadataSelection)     mIMetadataDynamic2,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal,
        (MetadataSelection)     mIMetadataHal2,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal)
        MINT32                  mMultiCamFeatureMode = -1;
        MBOOL                   mIsAinr = MFALSE;
        MBOOL                   mIsRawInput = MTRUE;
        MSize                   mFovBufferSize = MSize(0, 0);
        IMetadata               mAppSessionMeta;
        MBOOL                   mIsHidlIsp = MFALSE;
        MINT32                  mMultiframeType = -1;
        MBOOL                   mIsFeatureSupportSub = MFALSE;
        MBOOL                   mInPlace = MFALSE;
        MSize                   mFullBufferSize = MSize(0, 0);
    };

    struct Request {
        FIELDS(
        (MINT32)                mSensorId,
        (MBOOL)                 mMainFrame,
        (MUINT32)               mRequestNo,
        (MUINT32)               mFrameNo,
        (MBOOL)                 mIsPhysical,
        (MBOOL)                 mIsDualMode,
        (MBOOL)                 mIsBayerBayer,
        (MBOOL)                 mIsBayerMono,
        (MBOOL)                 mIsMasterIndex,
        (MBOOL)                 mIsVSDoFMode,
        (MBOOL)                 mHasDCE,
        (MBOOL)                 mIsRawInput,
        (MINT32)                mMultiframeType,
        (BufferHandle::Ptr)     mIBufferFull,
        (BufferHandle::Ptr)     mIBufferClean,
        (BufferHandle::Ptr)     mIBufferResized,
        (BufferHandle::Ptr)     mIBufferDepth,
        (BufferHandle::Ptr)     mIBufferDCES,
        (BufferHandle::Ptr)     mIBufferLCS,
        (BufferHandle::Ptr)     mIBufferLCESHO,
        (BufferHandle::Ptr)     mOBufferFull,
        (MetadataHandle::Ptr)   mIMetadataDynamic,
        (MetadataHandle::Ptr)   mIMetadataDynamic2,
        (MetadataHandle::Ptr)   mIMetadataApp,
        (MetadataHandle::Ptr)   mIMetadataHal,
        (MetadataHandle::Ptr)   mIMetadataHal2,
        (MetadataHandle::Ptr)   mOMetadataApp,
        (MetadataHandle::Ptr)   mOMetadataHal);
        MVOID*                  mpBufLCE2CALTMVA = nullptr;
        MBOOL                   mNeedProcessSub = MFALSE;
        MSize                   mFovBufferSize = MSize(0, 0);
        MRect                   mFovCropRegion = MRect(0, 0);
    };

    struct ConfigParam {};

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<Yuv> YuvPlugin;

/******************************************************************************
 * Join Interface
 ******************************************************************************/
class Join {
public:
    struct Property {
        FIELDS(
        (const char*)           mName,
        (MUINT64)               mFeatures
        )

        Property() : mName(0)
                    ,mFeatures(0)
        {
        }
    };

    struct Selection {
        StreamInfo              mCfgInfo;
        TPICustomConfig         mTPICustomConfig;
        MUINT32                 mSelStage;
        MUINT32                 mCfgOrder;
        MUINT32                 mCfgJoinEntry;
        MBOOL                   mCfgSharedInput;
        MBOOL                   mCfgInplace;
        MBOOL                   mCfgCropInput;
        MBOOL                   mCfgEnableFD;
        MUINT32                 mCfgExpectMS;
        MUINT32                 mCfgAsyncType;
        MUINT32                 mCfgQueueCount;
        MUINT32                 mCfgMarginRatio; // out * (1 + margin%) = input
        MBOOL                   mCfgEnablePerFrameNegotiate;
        MBOOL                   mCfgRun;
        MBOOL                   mP2Run;
        std::vector<MUINT32>    mAllSensorIDs;
        FIELDS(
        (MINT32)                mSensorId,
        (BufferSelection)       mIBufferMain1,
        (BufferSelection)       mIBufferMain2,
        (BufferSelection)       mIBufferDownscale,
        (BufferSelection)       mIBufferDepth,
        (BufferSelection)       mIBufferLCS1,
        (BufferSelection)       mIBufferLCS2,
        (BufferSelection)       mIBufferRSS1,
        (BufferSelection)       mIBufferRSS2,
        (BufferSelection)       mOBufferMain1,
        (BufferSelection)       mOBufferMain2,
        (BufferSelection)       mOBufferDepth,
        (BufferSelection)       mOBufferDisplay,
        (BufferSelection)       mOBufferRecord,
        (MetadataSelection)     mIMetadataDynamic1,
        (MetadataSelection)     mIMetadataDynamic2,
        (MetadataSelection)     mIMetadataApp,
        (MetadataSelection)     mIMetadataHal1,
        (MetadataSelection)     mIMetadataHal2,
        (MetadataSelection)     mOMetadataApp,
        (MetadataSelection)     mOMetadataHal)
    };

    struct Request {
        JoinQueueCmd            mOQueueCmd;
        JoinImageInfo           mIBufferMain1_Info;
        JoinImageInfo           mIBufferMain2_Info;
        JoinImageInfo           mIBufferDepth_Info;
        JoinImageInfo           mOBufferMain1_Info;
        JoinImageInfo           mOBufferMain2_Info;
        JoinImageInfo           mOBufferDisplay_Info;
        JoinImageInfo           mOBufferRecord_Info;
        FIELDS(
        (BufferHandle::Ptr)     mIBufferMain1,
        (BufferHandle::Ptr)     mIBufferMain2,
        (BufferHandle::Ptr)     mIBufferDownscale,
        (BufferHandle::Ptr)     mIBufferDepth,
        (BufferHandle::Ptr)     mIBufferLCS1,
        (BufferHandle::Ptr)     mIBufferLCS2,
        (BufferHandle::Ptr)     mIBufferRSS1,
        (BufferHandle::Ptr)     mIBufferRSS2,
        (BufferHandle::Ptr)     mOBufferMain1,
        (BufferHandle::Ptr)     mOBufferMain2,
        (BufferHandle::Ptr)     mOBufferDepth,
        (BufferHandle::Ptr)     mOBufferDisplay,
        (BufferHandle::Ptr)     mOBufferRecord,
        (MetadataHandle::Ptr)   mIMetadataDynamic1,
        (MetadataHandle::Ptr)   mIMetadataDynamic2,
        (MetadataHandle::Ptr)   mIMetadataApp,
        (MetadataHandle::Ptr)   mIMetadataHal1,
        (MetadataHandle::Ptr)   mIMetadataHal2,
        (MetadataHandle::Ptr)   mOMetadataApp,
        (MetadataHandle::Ptr)   mOMetadataHal)
    };

    struct ConfigParam {
        BufferInfo mIBufferMain1;
    };

    struct RecommendedParam {
        // input
        IMetadata const* pAppMetadata = nullptr;
        // output
        std::vector<std::shared_ptr<IMetadata>> framesSetting;
    };
};

typedef PipelinePlugin<Join> JoinPlugin;

/******************************************************************************
*
******************************************************************************/
};  //namespace NSPipelinePlugin
};  //namespace NSCam
#endif //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_PLUGIN_PIPELINEPLUGINTYPE_H_
