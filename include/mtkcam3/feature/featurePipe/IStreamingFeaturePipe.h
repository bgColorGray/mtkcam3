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

#ifndef _MTK_CAMERA_FEATURE_PIPE_I_STREAMING_FEATURE_PIPE_H_
#define _MTK_CAMERA_FEATURE_PIPE_I_STREAMING_FEATURE_PIPE_H_

//#include <effectHal/EffectRequest.h>
#include <unordered_map>
#include <vector>
#include <mtkcam/drv/iopipe/PostProc/IHalPostProcPipe.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>



#include <mtkcam3/feature/featurePipe/util/VarMap.h>
#include <mtkcam3/feature/3dnr/3dnr_defs.h>
#include "./IStreamingFeaturePipe_var.h"
#include <mtkcam3/feature/dsdn/dsdn_type.h>
#include <mtkcam3/feature/eis/EisInfo.h>
#include <mtkcam3/feature/utils/p2/P2Pack.h>
#include <mtkcam3/feature/featurePipe/SFPIO.h>

// for secure cam
#include <mtkcam/utils/imgbuf/ISecureImageBufferHeap.h>

#include <mtkcam3/feature/utils/p2/DIPStream.h>

#ifdef FEATURE_MASK
#error FEATURE_MASK macro redefine
#endif

#define FEATURE_MASK(name) (1 << OFFSET_##name)

#define MAKE_FEATURE_MASK_FUNC(name, tag)             \
    const MUINT32 MASK_##name = (1 << OFFSET_##name); \
    inline MBOOL HAS_##name(MUINT32 feature)          \
    {                                                 \
        return !!(feature & FEATURE_MASK(name));      \
    }                                                 \
    inline MVOID ENABLE_##name(MUINT32 &feature)      \
    {                                                 \
        feature |= FEATURE_MASK(name);                \
    }                                                 \
    inline MVOID DISABLE_##name(MUINT32 &feature)     \
    {                                                 \
        feature &= ~FEATURE_MASK(name);               \
    }                                                 \
    inline const char* TAG_##name()                   \
    {                                                 \
        return tag;                                   \
    }

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

const MUINT32 INVALID_SENSOR = (MUINT32)(-1);

enum FEATURE_MASK_OFFSET
{
    OFFSET_EIS,
    OFFSET_EIS_30,
    OFFSET_VHDR,
    OFFSET_3DNR,
    OFFSET_3DNR_S, // 3DNR Slave, controlled by SFP
    OFFSET_DSDN20,
    OFFSET_DSDN20_S, // DSDN20 Slave, controlled by SFP
    OFFSET_DSDN25,
    OFFSET_DSDN25_S, // DSDN25 Slave, controlled by SFP
    OFFSET_DSDN30,
    OFFSET_DSDN30_S, // DSDN25 Slave, controlled by SFP
    OFFSET_EIS_QUEUE,
    OFFSET_TPI_YUV,
    OFFSET_TPI_ASYNC,
    OFFSET_RSC,
    OFFSET_FSC,
};

MAKE_FEATURE_MASK_FUNC(EIS, "EIS22");
MAKE_FEATURE_MASK_FUNC(EIS_30, "EIS30");
MAKE_FEATURE_MASK_FUNC(VHDR, "");
MAKE_FEATURE_MASK_FUNC(3DNR, "3DNR");
MAKE_FEATURE_MASK_FUNC(3DNR_S, "3DNR_S");
MAKE_FEATURE_MASK_FUNC(DSDN20, "DSDN20");
MAKE_FEATURE_MASK_FUNC(DSDN20_S, "DSDN20_S");
MAKE_FEATURE_MASK_FUNC(DSDN25, "DSDN25");
MAKE_FEATURE_MASK_FUNC(DSDN25_S, "DSDN25_S");
MAKE_FEATURE_MASK_FUNC(DSDN30, "DSDN30");
MAKE_FEATURE_MASK_FUNC(DSDN30_S, "DSDN30_S");
MAKE_FEATURE_MASK_FUNC(EIS_QUEUE, "Q");
MAKE_FEATURE_MASK_FUNC(TPI_YUV, "TPI_YUV");
MAKE_FEATURE_MASK_FUNC(TPI_ASYNC, "TPI_ASYNC");
MAKE_FEATURE_MASK_FUNC(RSC, "RSC");
MAKE_FEATURE_MASK_FUNC(FSC, "FSC");


class FeaturePipeParam;

class IStreamingFeaturePipe
{
public:
    enum eAppMode
    {
        APP_PHOTO_PREVIEW = 0,
        APP_VIDEO_PREVIEW = 1,
        APP_VIDEO_RECORD  = 2,
        APP_VIDEO_STOP    = 3,
    };

    enum eUsageMode
    {
        USAGE_DEFAULT,
        USAGE_P2A_PASS_THROUGH,
        USAGE_P2A_FEATURE,
        USAGE_STEREO_EIS,
        USAGE_FULL,
        USAGE_BATCH_SMVR,
        USAGE_DUMMY,
    };

    class OutConfig
    {
    public:
        MUINT32 mMaxOutNum = 2;// max out buffer num in 1 frame for 1 sensor
        MBOOL mHasPhysical = MFALSE;
        MBOOL mHasLarge = MFALSE;
        MSize mFDSize;
        MSize mVideoSize;
    };
    class InConfig
    {
    public:
        enum Type
        {
            RAW_IN,
            P1YUV_IN,
        };
        Type    mType = RAW_IN;
        MUINT32 mReqFps = 30;
        MUINT32 mP1Batch = 1;
    };

    class UsageHint
    {
    public:
        eUsageMode mMode = USAGE_FULL;
        MSize mStreamingSize;
        std::vector<MSize> mOutSizeVector;
        NSCam::EIS::EisInfo mEISInfo;
        DSDNParam mDSDNParam;
        MUINT32 m3DNRMode = 0;
        MUINT32 mAISSMode = 0;
        MUINT32 mDSDNMode = 0;
        MUINT32 mFSCMode = 0;
        MUINT32 mDualMode = 0;
        MUINT32 mSMVRSpeed = 1;
        MBOOL   mUseTSQ = MFALSE;
        MBOOL   mEnlargeRsso = MFALSE;
        MUINT64 mTP = 0;
        MFLOAT mTPMarginRatio = 1.0;
        IMetadata mAppSessionMeta;
        std::vector<MUINT32> mAllSensorIDs;
        OutConfig mOutCfg;
        InConfig  mInCfg;
        NSPipelinePlugin::TPICustomConfig mTPICustomConfig;
        // for multi-cam
        std::map<MUINT32, MSize> mResizedRawMap;
        MUINT32 mSensorModule = 0;
        // for secure cam
        SecType mSecType = SecType::mem_normal;
        MBOOL  mIsHdr10 = MFALSE;
        MINT32 mHdr10Spec = 0;
        MUINT32 mP2RBMask = 0;
        MBOOL mUseScenarioRecorder = MFALSE;

        // for TrackingFocus
        MBOOL mTrackingFocusOn = MFALSE;

        UsageHint();
        UsageHint(eUsageMode mode, const MSize &streamingSize);
        ~UsageHint() {}

        MVOID enable3DNRModeMask(NSCam::NR3D::E3DNR_MODE_MASK mask)
        {
            m3DNRMode |= mask;
        }
    };

public:
    virtual ~IStreamingFeaturePipe() {}

public:
    // interface for PipelineNode
    static IStreamingFeaturePipe* createInstance(MUINT32 openSensorIndex, const UsageHint &usageHint);
    MBOOL destroyInstance(const char *name=NULL);

    virtual MBOOL init(const char *name=NULL) = 0;
    virtual MBOOL uninit(const char *name=NULL) = 0;
    virtual MBOOL enque(const FeaturePipeParam &param) = 0;
    virtual MVOID notifyFlush() = 0;
    virtual MBOOL flush() = 0;
    virtual MBOOL setJpegParam(NSCam::NSIoPipe::NSPostProc::EJpgCmd cmd, int arg1, int arg2) = 0;
    virtual MBOOL setFps(MINT32 fps) = 0;
    virtual MUINT32 getRegTableSize() = 0;
    virtual MBOOL sendCommand(NSCam::NSIoPipe::NSPostProc::ESDCmd cmd, MINTPTR arg1=0, MINTPTR arg2=0, MINTPTR arg3=0) = 0;
    virtual MBOOL addMultiSensorID(MUINT32 sensorID) = 0;

public:
    // sync will block until all data are processed,
    // use with caution and avoid deadlock !!
    virtual MVOID sync() = 0;

protected:
    IStreamingFeaturePipe() {}
};

class FeaturePipeParam
{
public:
    enum MSG_TYPE { MSG_FRAME_DONE, MSG_DISPLAY_DONE, MSG_RSSO_DONE, MSG_FD_DONE, MSG_P2B_SET_3A, MSG_N3D_SET_SHOTMODE, MSG_TRACKING_DONE, MSG_INVALID };
    typedef MBOOL (*CALLBACK_T)(MSG_TYPE, FeaturePipeParam&);

    VarMap<SFP_VAR> mVarMap;
    MUINT32 mFeatureMask;
    CALLBACK_T mCallback;
    Feature::P2Util::DIPParams mDIPParams;

    std::unordered_map<MUINT32, FeaturePipeParam> mSlaveParamMap; // Only Valid in Master FeaturePipeParam

    SFPIOManager mSFPIOManager; // Only Valid in Master FeaturePipeParam
    Feature::P2Util::P2Pack mP2Pack; // All Sensor has its own pack, including all sensors' data
    Feature::P2Util::P2DumpType mDumpType = Feature::P2Util::P2_DUMP_NONE;

    FeaturePipeParam()
        : mFeatureMask(0)
        , mCallback(NULL)
    {
    }

    FeaturePipeParam(CALLBACK_T callback)
        : mFeatureMask(0)
        , mCallback(callback)
    {
    }

    FeaturePipeParam(CALLBACK_T callback, const Feature::P2Util::P2Pack& p2Pack)
        : mFeatureMask(0)
        , mCallback(callback)
        , mP2Pack(p2Pack)
    {
    }

    MVOID setFeatureMask(MUINT32 mask, MBOOL enable)
    {
        if( enable )
        {
            mFeatureMask |= mask;
        }
        else
        {
            mFeatureMask &= (~mask);
        }
    }

    MVOID setDIPParams(Feature::P2Util::DIPParams &dipParams)
    {
        mDIPParams = dipParams;
    }

    Feature::P2Util::DIPParams getDIPParams() const
    {
        return mDIPParams;
    }

    MVOID addSlaveParam(MUINT32 sensorID, FeaturePipeParam &param)
    {
        mSlaveParamMap[sensorID] = param;
        if(mFirstSlaveID == INVALID_SENSOR)
            mFirstSlaveID = sensorID;
    }

    MBOOL getFirstSlaveParam(FeaturePipeParam &out) const
    {
        if(existSlaveParam())
        {
            out = mSlaveParamMap.at(mFirstSlaveID);
            return MTRUE;
        }
        return MFALSE;
    }

    MBOOL existSlaveParam() const
    {
        return (mFirstSlaveID != INVALID_SENSOR);
    }

    MBOOL addSFPIOMap(SFPIOMap &ioMap)
    {
        if(ioMap.mPathType == PATH_GENERAL)
            return mSFPIOManager.addGeneral(ioMap);
        else if(ioMap.mPathType == PATH_PHYSICAL)
            return mSFPIOManager.addPhysical(ioMap.getFirstSensorID(), ioMap);
        else if(ioMap.mPathType == PATH_LARGE)
            return mSFPIOManager.addLarge(ioMap.getFirstSensorID(), ioMap);
        else
            return MFALSE;
    }


    DECLARE_VAR_MAP_INTERFACE(mVarMap, SFP_VAR, setVar, getVar, tryGetVar, clearVar);
private:
    MUINT32 mFirstSlaveID = INVALID_SENSOR;
};

}; // NSFeaturePipe
}; // NSCamFeature
}; // NSCam

#undef MAKE_FEATURE_MASK_FUNC

#endif // _MTK_CAMERA_FEATURE_PIPE_I_STREAMING_FEATURE_PIPE_H_
