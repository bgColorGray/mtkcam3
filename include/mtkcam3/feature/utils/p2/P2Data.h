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

#ifndef _MTKCAM_FEATURE_UTILS_P2_DATA_H_
#define _MTKCAM_FEATURE_UTILS_P2_DATA_H_

#include <map>
#include <vector>

#include <utils/RefBase.h>

#include <mtkcam/utils/metadata/IMetadata.h>

#include <mtkcam3/feature/utils/log/ILogger.h>
#include <mtkcam3/feature/utils/p2/Cropper.h>
#include <mtkcam3/feature/utils/p2/P2PlatInfo.h>

#include <mtkcam3/feature/eis/EisInfo.h>

#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>

#include <mtkcam/utils/imgbuf/ISecureImageBufferHeap.h>

#include <mtkcam3/3rdparty/plugin/TPICustomInfo.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>

namespace NSCam {
namespace Feature {
namespace P2Util {

#define INVALID_SENSOR_ID (MUINT32)(-1)

enum P2Type {
    P2_UNKNOWN,
    P2_PREVIEW,
    P2_PHOTO,
    P2_VIDEO,
    P2_HS_VIDEO,
    P2_BATCH_SMVR,
    P2_CAPTURE,
    P2_TIMESHARE_CAPTURE,
    P2_DUMMY
};

enum P2DumpType {
    P2_DUMP_NONE = 0,
    P2_DUMP_NDD = 1,
    P2_DUMP_DEBUG = 2,
    P2_DUMP_PER_FRAME = 3,
};

enum IMG_TYPE
{
    IMG_TYPE_UNKNOWN,
    IMG_TYPE_DISPLAY,
    IMG_TYPE_RECORD,
    IMG_TYPE_EXTRA,
    IMG_TYPE_FD,
    IMG_TYPE_PHYSICAL,
};

struct P2AppStreamInfo
{
    IMG_TYPE mImageType = IMG_TYPE_UNKNOWN;
    MSize mImageSize;
};

class P2UsageHint
{
public:
    class OutConfig
    {
    public:
        MUINT32 mMaxOutNum = 2;// max out buffer num in 1 pipeline frame for 1 sensor
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
    MSize               mStreamingSize;
    std::vector<P2AppStreamInfo> mAppStreamVector;
    std::vector<MSize>  mOutSizeVector;
    NSCam::EIS::EisInfo mEisInfo;
    MUINT32             mDsdnHint = 0;
    MUINT32             m3DNRMode = 0;
    MUINT32             mAISSMode = 0;
    MUINT32             mFSCMode = 0;
    MUINT32             mDualMode = 0;
    MUINT32             mSMVRSpeed = 1;
    MBOOL               mUseTSQ = MFALSE;
    MBOOL               mEnlargeRsso = MFALSE;
    MBOOL               mDynamicTuning = MFALSE;
    MUINT64             mTP = 0;
    // mTPMarginRatio: Fixed TPI margin ratio.
    // e.g.  1.2 means output * 1.2 = final rrzo, final rrz / 1.2 = available output area
    MFLOAT              mTPMarginRatio = 1.0f;
    MFLOAT              mTotalMarginRatio = 1.0f;
    IMetadata           mAppSessionMeta;
    MBOOL               mQParamValid = MTRUE; // Hal1 & Develop need
    OutConfig           mOutCfg;
    InConfig            mInCfg;
    NSPipelinePlugin::TPICustomConfig     mTPICustomConfig;
    // for multi-cam
    MUINT32             mSensorModule = 0;
    std::map<MUINT32, MSize> mResizedRawMap;
    NSCam::SecType mSecType = NSCam::SecType::mem_normal;
    //
    MBOOL               mBGPreRelease = MFALSE;
    MINT32              mPluginUniqueKey = 0;
    MBOOL               mIsHidlIsp = MFALSE;
    MBOOL               mHasVideo = MFALSE;
    MUINT32             mP2PQIndex = 0;
    MBOOL               mIsHdr10 = MFALSE;
    MINT32              mHdr10Spec = 0;
    MINT32              mPresetPQIdx = 0;
    MBOOL               mTrackingFocusOn = MFALSE;
};

class P2ConfigInfo
{
public:
    P2ConfigInfo();
    P2ConfigInfo(const ILog &log);
    ~P2ConfigInfo() {}
public:
    static const P2ConfigInfo Dummy;
public:
    ILog mLog;
    MUINT32 mLogLevel = 0;
    P2Type mP2Type = P2_UNKNOWN;
    P2UsageHint mUsageHint;
    MUINT32 mMainSensorID = -1;
    std::vector<MUINT32> mAllSensorID;
    std::vector<P2AppStreamInfo> mAppStreamInfo;
    MUINT32 mBurstNum = 0;
    MUINT32 mCustomOption = 0;
    MBOOL mSupportPQ = MFALSE;
    MBOOL mSupportClearZoom = MFALSE;
    MBOOL mSupportDRE = MFALSE;
    MBOOL mSupportHFG = MFALSE;
    MBOOL mUseScenarioRecorder = MFALSE;
    MBOOL mAutoTestLog = MFALSE;
};

class P2SensorInfo
{
public:
    P2SensorInfo();
    P2SensorInfo(const ILog &log, const MUINT32 &id);
    ~P2SensorInfo() {}
public:
    static const P2SensorInfo Dummy;
public:
    ILog mLog;
    MUINT32 mSensorID = INVALID_SENSOR_ID;
    const P2PlatInfo *mPlatInfo = NULL;
    MRect mActiveArray;
};

class P2FrameData
{
public:
    P2FrameData();
    P2FrameData(const ILog &log);
    ~P2FrameData() {}
public:
    static const P2FrameData Dummy;
public:
    ILog mLog;
    MUINT32 mP2FrameNo = 0;
    MINT32 mMWFrameNo = 0;
    MINT32 mMWFrameRequestNo = 0;
    MUINT32 mAppMode = 0;
    MBOOL mIsRecording = MFALSE;
    MUINT32 mMasterSensorID = INVALID_SENSOR_ID;
    MINT32 mMinFps = 0;
    MINT32 mMaxFps = 0;
};

class P2SensorData
{
public:
    P2SensorData();
    P2SensorData(const ILog &log);
    ~P2SensorData() {}
public:
    static const P2SensorData Dummy;
public:
    ILog mLog;
    MUINT32 mSensorID = INVALID_SENSOR_ID;
    MINT32 mMWUniqueKey = 0;
    MINT32 mMagic3A = 0;
    MUINT8 mIspProfile = 0;
    MINT64 mP1TS = 0;
    MINT64 FrameStartTime = 0;
    std::vector<MINT64> mP1TSVector;
    MINT32 mISO = 0;
    MINT32 mLV = 0;
    MINT32 mLensPosition = 0;
    MINT32 mFlickerResult = 0;

    MINT32 mSensorMode = 0;
    MSize mSensorSize;
    MRect mP1Crop;
    MRect mP1DMA;
    MSize mP1OutSize;
    MRect mP1BinCrop;
    MSize mP1BinSize;
    MRect mSensorTargetCrop;

    MBOOL mAppEISOn = MFALSE;
    MRect mAppCrop;
    MRect mSimulatedAppCrop;
    MRect mPhysicalAppCrop;

    android::sp<Cropper> mCropper;
    TuningUtils::FILE_DUMP_NAMING_HINT mNDDHint;
    P2PlatInfo::NVRamDSDN mNvramDsdn;
    P2PlatInfo::NVRamMSYUV_3DNR mNvramMsyuv_3DNR;

    MINT32 mAeBv = -1;
    MINT32 mAeCwv = -1;

    MINT32 mStreamingNR = MTK_DUALZOOM_STREAMING_NR_AUTO;
    MINT32 mHDRHalMode = MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
};

} // namespace P2Util
} // namespace Feature
} // namespace NSCam

#endif // _MTKCAM_FEATURE_UTILS_P2_DATA_H_
