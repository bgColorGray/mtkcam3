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

#include "StreamingFeaturePipeUsage.h"
#include "StreamingFeaturePipe.h"
#include <mtkcam/drv/iopipe/PostProc/WpeUtility.h>
#include <mtkcam3/feature/eis/eis_ext.h>
#include <mtkcam3/feature/stereo/StereoCamEnum.h>
#include <mtkcam3/feature/3dnr/3dnr_defs.h>
#include <mtkcam3/feature/fsc/fsc_defs.h>
#include <mtkcam3/feature/fsc/fsc_util.h>
#include <camera_custom_eis.h>
#include <camera_custom_dualzoom.h>

// draft, just for offline fov debug
#include <cutils/properties.h>

#define PIPE_CLASS_TAG "PipeUsage"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_USAGE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

typedef IStreamingFeaturePipe::UsageHint UsageHint;
#define ADD_3DNR_RSC_SUPPORT 1
#define P2A_TOTAL_PATH_NUM 3 // general, physical, large
#define MIN_P2A_TUNING_BUF_NUM 4

StreamingFeaturePipeUsage::StreamingFeaturePipeUsage()
{

}


StreamingFeaturePipeUsage::StreamingFeaturePipeUsage(UsageHint hint, MUINT32 sensorIndex)
    : mUsageHint(hint)
    , mStreamingSize(mUsageHint.mStreamingSize)
    , m3DNRMode(mUsageHint.m3DNRMode)
    , mAISSMode(mUsageHint.mAISSMode)
    , mFSCMode(mUsageHint.mFSCMode)
    , mDualMode(mUsageHint.mDualMode)
    , mSensorIndex(sensorIndex)
    , mOutCfg(mUsageHint.mOutCfg)
    , mNumSensor(mUsageHint.mAllSensorIDs.size())
    , mSecType(hint.mSecType)
    , mResizedRawSizeMap(mUsageHint.mResizedRawMap)
    , mUseScenarioRecorder(mUsageHint.mUseScenarioRecorder)
{
    if (mUsageHint.mMode == IStreamingFeaturePipe::USAGE_DEFAULT)
    {
        mUsageHint.mMode = IStreamingFeaturePipe::USAGE_FULL;
    }

    //remove future
    mUseP2G = getInitP2G();
    mVendorDebug = getPropertyValue(KEY_DEBUG_TPI, 0);
    mVendorLog = getPropertyValue(KEY_DEBUG_TPI_LOG, 0);
    mVendorErase = getPropertyValue(KEY_DEBUG_TPI_ERASE, 0);
    mSupportPure = (getPropertyValue(KEY_ENABLE_PURE_YUV, SUPPORT_PURE_YUV) == 1 ); // TODO test, remove later
    mPrintNextIO = getPropertyValue(KEY_FORCE_PRINT_NEXT_IO, VAL_FORCE_PRINT_NEXT_IO);
    mForceIMG3O = getPropertyValue(KEY_FORCE_IMG3O, VAL_FORCE_IMG3O);
    mSupportSlaveNR = getPropertyValue(KEY_ENABLE_SLAVENR, VAL_ENABLE_SLAVE_NR);
    mForceWarpP = getPropertyValue(KEY_FORCE_WARP_P, VAL_FORCE_WARP_P);
    mForceWarpR = getPropertyValue(KEY_FORCE_WARP_R, VAL_FORCE_WARP_R);
    mForceVmdpP = getPropertyValue(KEY_FORCE_VMDP_P, VAL_FORCE_VMDP_R);
    mForceVmdpR = getPropertyValue(KEY_FORCE_VMDP_R, VAL_FORCE_VMDP_R);
    mSupportPhysicalNR = getPropertyValue(KEY_ENABLE_PHYSICAL_NR, VAL_ENABLE_PHYSICAL_NR);
    mEncodePreviewBuf = getPropertyValue(KEY_DEBUG_ENCODE_PREVIEW, VAL_DEBUG_ENCODE_PREVIEW);
    mSupportXNode = getPropertyValue(KEY_FORCE_XNODE, VAL_FORCE_XNODE);

    switch (mUsageHint.mMode)
    {
    case IStreamingFeaturePipe::USAGE_P2A_FEATURE:
        mPipeFunc = PIPE_USAGE_RSC | PIPE_USAGE_EIS | PIPE_USAGE_3DNR | PIPE_USAGE_VENDOR | PIPE_USAGE_DSDN;
        mP2AMode = P2A_MODE_FEATURE;
        break;
    case IStreamingFeaturePipe::USAGE_FULL:
        mPipeFunc = PIPE_USAGE_RSC | PIPE_USAGE_EIS | PIPE_USAGE_3DNR | PIPE_USAGE_EARLY_DISPLAY | PIPE_USAGE_VENDOR | PIPE_USAGE_DSDN;
        mP2AMode = P2A_MODE_FEATURE;
        break;
    case IStreamingFeaturePipe::USAGE_STEREO_EIS:
        mPipeFunc = PIPE_USAGE_RSC |PIPE_USAGE_EIS;
        mP2AMode = P2A_MODE_BYPASS;
        break;
    case IStreamingFeaturePipe::USAGE_BATCH_SMVR:
        mPipeFunc = mUseP2G ? (PIPE_USAGE_SMVR | PIPE_USAGE_3DNR) : PIPE_USAGE_SMVR;
        mP2AMode = mUseP2G ? P2A_MODE_FEATURE : P2A_MODE_DISABLE;
        break;
    case IStreamingFeaturePipe::USAGE_P2A_PASS_THROUGH:
    default:
        mPipeFunc = PIPE_USAGE_VENDOR;
        mP2AMode = P2A_MODE_NORMAL;
        break;
    }

    if(m3DNRMode && !support3DNR())
    {
        MY_LOGE("3DNR not supportted! But UsageHint 3DNR Mode(%d) enabled!!", m3DNRMode);
    }

    updateOutSize(hint.mOutSizeVector);
}

MVOID StreamingFeaturePipeUsage::config(const TPIMgr *mgr)
{
    mTPIUsage.config(mgr);
    mSupportSharedInput = supportP2AP2() && (!supportTPI(TPIOEntry::YUV) ||
        mTPIUsage.use(TPIOEntry::YUV, TPIOUse::SHARED_INPUT));
    MY_LOGI("config usage : mSuppoRtSharedInput(%d),supportSlaveNR(%d), supportPhysicalNR(%d)",
            mSupportSharedInput, supportSlaveNR(), supportPhysicalNR());

    if( mSupportXNode )
    {
        MBOOL isDIV = mTPIUsage.use(TPIOEntry::YUV, TPIOUse::DIV);
        if( supportEISNode() || isDIV )
        {
            MY_LOGE("XNode disable: not combine with eis(%d)/tpi_div(%d)",
                    supportEISNode(), isDIV);
            mSupportXNode = MFALSE;
        }
    }
}

MBOOL StreamingFeaturePipeUsage::supportP2G() const
{
    return mUseP2G;
}

MBOOL StreamingFeaturePipeUsage::supportP2AP2() const
{
    return !supportDepthP2() && !supportSMP2();
}

MBOOL StreamingFeaturePipeUsage::supportDepthP2() const
{
    #if(SUPPORT_P2GP2_WITH_DEPTH)
        return MFALSE;
    #else
        return supportDPE();
    #endif
}

MBOOL StreamingFeaturePipeUsage::supportSMP2() const
{
    return !!(mPipeFunc & PIPE_USAGE_SMVR);
}

MBOOL StreamingFeaturePipeUsage::supportLargeOut() const
{
    return mOutCfg.mHasLarge;
}
MBOOL StreamingFeaturePipeUsage::supportPhysicalOut() const
{
    return mOutCfg.mHasPhysical;
}

MBOOL StreamingFeaturePipeUsage::supportIMG3O() const
{
#ifdef SUPPORT_IMG3O
    return MTRUE;
#else
    return MFALSE;
#endif
}

MBOOL StreamingFeaturePipeUsage::supportVNRNode() const
{
    return (mPipeFunc & PIPE_USAGE_DSDN) &&
           supportDSDN20();
}

MBOOL StreamingFeaturePipeUsage::supportEISNode() const
{
    return (mPipeFunc & PIPE_USAGE_EIS) &&
           ( EIS_MODE_IS_EIS_30_ENABLED(mUsageHint.mEISInfo.mode) ||
             EIS_MODE_IS_EIS_22_ENABLED(mUsageHint.mEISInfo.mode) );
}

MBOOL StreamingFeaturePipeUsage::supportWarpNode(SFN_HINT hint) const
{
    MBOOL support = MFALSE;
    switch (hint)
    {
    case SFN_HINT::PREVIEW:
        support = mForceWarpP && supportPreviewEIS();
        break;
    case SFN_HINT::RECORD:
        support = mForceWarpR && supportEISNode() && !!(getVideoSize().size());
        break;
    case SFN_HINT::TWIN:
        support = supportWarpNode(SFN_HINT::PREVIEW) && supportWarpNode(SFN_HINT::RECORD);
        break;
    case SFN_HINT::NONE:
    default:
        support = supportWarpNode(SFN_HINT::PREVIEW) || supportWarpNode(SFN_HINT::RECORD);
        break;
    }

    return support;
}

MBOOL StreamingFeaturePipeUsage::supportTrackingFocus() const
{
    return mUsageHint.mTrackingFocusOn;
}

MBOOL StreamingFeaturePipeUsage::supportVMDPNode(SFN_HINT hint) const
{
    MBOOL support = MFALSE;
    switch (hint)
    {
    case SFN_HINT::PREVIEW:
        support = mForceVmdpP && (supportTPI(TPIOEntry::YUV) || ( supportDSDN20() && ( !supportWarpNode() || !supportPreviewEIS()) ));
        break;
    case SFN_HINT::RECORD:
        support = mForceVmdpR && mTPIUsage.use(TPIOEntry::YUV, TPIOUse::DIV);
        break;
    case SFN_HINT::TWIN:
        support = supportVMDPNode(SFN_HINT::PREVIEW) && supportVMDPNode(SFN_HINT::RECORD);
        break;
    case SFN_HINT::NONE:
    default:
        support = supportVMDPNode(SFN_HINT::PREVIEW) || supportVMDPNode(SFN_HINT::RECORD);
        break;
    }

    return support;
}

MBOOL StreamingFeaturePipeUsage::supportRSCNode() const
{
    return (mPipeFunc & PIPE_USAGE_RSC) &&
            ( supportEISRSC() ||
              support3DNRRSC() ||
              supportAISS() );
}

MBOOL StreamingFeaturePipeUsage::supportAISS() const
{
    return mAISSMode;
}

MBOOL StreamingFeaturePipeUsage::supportTOFNode() const
{
    return supportTOF();
}

MBOOL StreamingFeaturePipeUsage::supportP2ALarge() const
{
    return mOutCfg.mHasLarge;
}

MBOOL StreamingFeaturePipeUsage::support4K2K() const
{
    return is4K2K(mStreamingSize);
}

MBOOL StreamingFeaturePipeUsage::support60FPS() const
{
    return mUsageHint.mInCfg.mReqFps == 60;
}

MBOOL StreamingFeaturePipeUsage::supportTimeSharing() const
{
    return mP2AMode == P2A_MODE_TIME_SHARING;
}

MBOOL StreamingFeaturePipeUsage::supportP2AFeature() const
{
    return mP2AMode == P2A_MODE_FEATURE;
}

MBOOL StreamingFeaturePipeUsage::supportBypassP2A() const
{
    return mP2AMode == P2A_MODE_BYPASS;
}

MBOOL StreamingFeaturePipeUsage::supportP1YUVIn() const
{
    return mUsageHint.mInCfg.mType == IStreamingFeaturePipe::InConfig::P1YUV_IN;
}

MBOOL StreamingFeaturePipeUsage::supportPure() const
{
    return mSupportPure;
}

MBOOL StreamingFeaturePipeUsage::encodePreviewEIS() const
{
    return mEncodePreviewBuf && supportPreviewEIS();
}

MBOOL StreamingFeaturePipeUsage::supportPreviewEIS() const
{
    return mUsageHint.mEISInfo.previewEIS && supportEISNode();
}

MBOOL StreamingFeaturePipeUsage::supportEIS_Q() const
{
    return mUsageHint.mEISInfo.supportQ && !encodePreviewEIS();
}

MBOOL StreamingFeaturePipeUsage::supportEIS_TSQ() const
{
    return supportEIS_Q() && mUsageHint.mUseTSQ;
}

MBOOL StreamingFeaturePipeUsage::supportEIS_RQQ() const
{
    return supportEIS_Q() && !mUsageHint.mUseTSQ;
}

MBOOL StreamingFeaturePipeUsage::supportTPI_TSQ() const
{
    return supportTPI(TPIOEntry::YUV) && mUsageHint.mUseTSQ;
}

MBOOL StreamingFeaturePipeUsage::supportTPI_RQQ() const
{
    return supportTPI(TPIOEntry::YUV) && !mUsageHint.mUseTSQ;
}

MBOOL StreamingFeaturePipeUsage::supportEISRSC() const
{
    return mUsageHint.mEISInfo.supportRSC;
}

MBOOL StreamingFeaturePipeUsage::supportWPE() const
{
    return EIS_MODE_IS_EIS_30_ENABLED(mUsageHint.mEISInfo.mode) && NSCam::NSIoPipe::WPEQuerySupport();
}

MBOOL StreamingFeaturePipeUsage::supportDual() const
{
    return  getNumSensor() >= 2;
}

MBOOL StreamingFeaturePipeUsage::supportDepth() const
{
    return supportDPE() || supportTOF();
}

MBOOL StreamingFeaturePipeUsage::supportDPE() const
{
    return SUPPORT_VSDOF &&
            ( (mDualMode & v1::Stereo::E_STEREO_FEATURE_VSDOF) ||
              (mDualMode & v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP) );
}

MBOOL StreamingFeaturePipeUsage::supportBokeh() const
{
    return SUPPORT_VSDOF &&
            (mDualMode & v1::Stereo::E_STEREO_FEATURE_VSDOF);
}

MBOOL StreamingFeaturePipeUsage::supportP2GWithDepth() const
{
    return supportDPE() && supportP2AP2();
}

MBOOL StreamingFeaturePipeUsage::supportTOF() const
{
    //return mDualMode == v1::Stereo::E_STEREO_FEATURE_TOF;
    return MFALSE;
}

MBOOL StreamingFeaturePipeUsage::support3DNR() const
{
    MBOOL ret = MFALSE;
    if(!supportIMG3O())
    {
        return MFALSE;
    }
    // could use property to enalbe 3DNR
    // assign value at getPipeUsageHint
    if (this->is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK_HAL_FORCE_SUPPORT) ||
        this->is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK_UI_SUPPORT))
    {
        ret = this->supportP2AFeature();
    }
    return ret;
}

MBOOL StreamingFeaturePipeUsage::support3DNRRSC() const
{
    MBOOL ret = MFALSE;
#if ADD_3DNR_RSC_SUPPORT
    if (support3DNR() && is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK_RSC_EN)
        && !is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK_SMVR_EN) )
    {
        ret = MTRUE;
    }
#endif // ADD_3DNR_RSC_SUPPORT
    return ret;
}

MBOOL StreamingFeaturePipeUsage::support3DNRLMV() const
{
    MBOOL ret = MFALSE;
    if (support3DNR() && !is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK_SMVR_EN) )
    {
        ret = MTRUE;
    }
    return ret;
}

MBOOL StreamingFeaturePipeUsage::is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK mask) const
{
    return (m3DNRMode & mask);
}

MBOOL StreamingFeaturePipeUsage::supportDSDN20() const
{
    return SUPPORT_VNR_DSDN && mUsageHint.mDSDNParam.isDSDN20();
}

MBOOL StreamingFeaturePipeUsage::supportDSDN25() const
{
    return mUseP2G && mUsageHint.mDSDNParam.isDSDN25();
}

MBOOL StreamingFeaturePipeUsage::supportDSDN30() const
{
    return mUseP2G && mUsageHint.mDSDNParam.isDSDN30() && !supportSMP2();
}

MBOOL StreamingFeaturePipeUsage::isGyroEnable() const
{
    return mUsageHint.mDSDNParam.isGyroEnable();
}

MBOOL StreamingFeaturePipeUsage::supportDSDN() const
{
    return supportDSDN20() || supportDSDN25() || supportDSDN30();
}

MBOOL StreamingFeaturePipeUsage::supportFSC() const
{
    return (mUsageHint.mFSCMode & NSCam::FSC::EFSC_MODE_MASK_FSC_EN);
}

MBOOL StreamingFeaturePipeUsage::supportVendorFSCFullImg() const
{
    return supportTPI(TPIOEntry::YUV) && supportFSC() && !supportEISNode();
}

MBOOL StreamingFeaturePipeUsage::supportEnlargeRsso() const
{
    return mUsageHint.mEnlargeRsso;
}

MBOOL StreamingFeaturePipeUsage::supportFull_NV21() const
{
    return USE_NV21_FULL_IMG && supportWPE() && !USE_WPE_STAND_ALONE &&
           NSCam::NSIoPipe::WPEQuerySupportFormat(NSCam::NSIoPipe::WPEFORMAT_420_2P_1ROUND_DL);
}

MBOOL StreamingFeaturePipeUsage::supportFull_YUY2() const
{
    return USE_YUY2_FULL_IMG && supportWPE();
}

MBOOL StreamingFeaturePipeUsage::supportGraphicBuffer() const
{
    return SUPPORT_GRAPHIC_BUFFER;
}

MBOOL StreamingFeaturePipeUsage::supportSlaveNR() const
{
    return mSupportSlaveNR && getNumSensor() > 1;
}

MBOOL StreamingFeaturePipeUsage::supportPhysicalNR() const
{
    return mSupportPhysicalNR && getNumSensor() > 1;
}

MBOOL StreamingFeaturePipeUsage::supportPhysicalDSDN20() const
{
    return supportPhysicalNR() && supportDSDN20() && !supportTPI(TPIOEntry::YUV) && !supportWarpNode();
}

EImageFormat StreamingFeaturePipeUsage::getFullImgFormat() const
{
    return supportFull_NV21() ? eImgFmt_NV21:
           supportFull_YUY2() ? eImgFmt_YUY2:
                                eImgFmt_YV12;
}

EImageFormat StreamingFeaturePipeUsage::getVNRImgFormat() const
{
    return eImgFmt_NV21;
}

MINT32 StreamingFeaturePipeUsage::getDMAConstrain() const
{
    return FSCUtil::isFSCSubpixelEnabled(mFSCMode) ?
           NSCam::Feature::P2Util::DMAConstrain::NONE :
           NSCam::Feature::P2Util::DMAConstrain::NOSUBPIXEL;
}

MBOOL StreamingFeaturePipeUsage::printNextIO() const
{
    return mPrintNextIO;
}

std::vector<MUINT32> StreamingFeaturePipeUsage::getAllSensorIDs() const
{
    return mUsageHint.mAllSensorIDs;
}

MUINT32 StreamingFeaturePipeUsage::getMode() const
{
    return mUsageHint.mMode;
}

MUINT32 StreamingFeaturePipeUsage::getEISMode() const
{
    return mUsageHint.mEISInfo.mode;
}

NSPipelinePlugin::TPICustomConfig StreamingFeaturePipeUsage::getTPICustomConfig() const
{
    return mUsageHint.mTPICustomConfig;
}

MUINT32 StreamingFeaturePipeUsage::getEISFactor() const
{
    return mUsageHint.mEISInfo.factor ? mUsageHint.mEISInfo.factor : 100;
}

DSDNParam StreamingFeaturePipeUsage::getDSDNParam() const
{
    return mUsageHint.mDSDNParam;
}

MUINT32 StreamingFeaturePipeUsage::getSensorModule() const
{
    return mUsageHint.mSensorModule;
}

MBOOL StreamingFeaturePipeUsage::useScenarioRecorder() const
{
    return mUseScenarioRecorder;
}

MUINT32 StreamingFeaturePipeUsage::getDualMode() const
{
    return mDualMode;
}

MUINT32 StreamingFeaturePipeUsage::get3DNRMode() const
{
    return m3DNRMode;
}

MUINT32 StreamingFeaturePipeUsage::getAISSMode() const
{
    return mAISSMode;
}

MUINT32 StreamingFeaturePipeUsage::getFSCMode() const
{
    return mFSCMode;
}

MUINT32 StreamingFeaturePipeUsage::getP1Batch() const
{
    return mUsageHint.mInCfg.mP1Batch;
}

MUINT32 StreamingFeaturePipeUsage::getSMVRSpeed() const
{
    return mUsageHint.mSMVRSpeed;
}

TP_MASK_T StreamingFeaturePipeUsage::getTPMask() const
{
    return mUsageHint.mTP;
}

MFLOAT StreamingFeaturePipeUsage::getTPMarginRatio() const
{
    return mUsageHint.mTPMarginRatio;
}

IMetadata StreamingFeaturePipeUsage::getAppSessionMeta() const
{
    return mUsageHint.mAppSessionMeta;
}

MSize StreamingFeaturePipeUsage::getStreamingSize() const
{
    return mStreamingSize;
}

MSize StreamingFeaturePipeUsage::getFDSize() const
{
    return mOutCfg.mFDSize;
}

MSize StreamingFeaturePipeUsage::getRrzoSizeByIndex(MUINT32 sID)
{
    MSize result= {0, 0};
    if (0 == mResizedRawSizeMap.count(sID)) {
        MY_LOGE("sensor ID(%d) MapCount(%zu)", sID, mResizedRawSizeMap.count(sID));
        return result;
    }
    result = mResizedRawSizeMap[sID];
    return result;
}

MSize StreamingFeaturePipeUsage::getMaxOutSize() const
{
    return mMaxOutArea;
}

MSize StreamingFeaturePipeUsage::getVideoSize() const
{
    return mOutCfg.mVideoSize;
}

MUINT32 StreamingFeaturePipeUsage::getNumSensor() const
{
    return mNumSensor;
}

MUINT32 StreamingFeaturePipeUsage::getNumScene() const
{
    MUINT32 numScene = 1;
    if( mOutCfg.mHasLarge ) ++numScene;
    if( mOutCfg.mHasPhysical ) ++numScene;
    numScene *= mNumSensor;
    return numScene;
}

MUINT32 StreamingFeaturePipeUsage::getNumP2GExtImg() const
{
    MUINT32 num = ((mOutCfg.mMaxOutNum > 2) || mForceIMG3O) ? 3 : 0;
    num = max(num, getVendorBufferNum().mBasic);
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumP2GExtPQ() const
{
    MUINT32 num = 0;
    num += mTPIUsage.getTPILength(TPIOEntry::YUV);
    num += supportTPI(TPIOEntry::ASYNC) ? 3 : 0;
    num += supportEISNode() ? 2 + getEISQueueSize() : 0;
    num += supportVNRNode() ? 2 : 0;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getP2GLength() const
{
    MUINT32 num = 1;
    num += support3DNRRSC() ? 1 : 0;
    num += supportDSDN() ? 1 : 0;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getPostProcLength() const
{
    MUINT32 num = 0;
    num += supportBokeh() ? 1 : 0;
    num += mTPIUsage.getTPILength(TPIOEntry::YUV) + 1;
    num += supportWarpNode() ? 1 : 0;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getDispPostProcLength() const
{
    MUINT32 num = 0;
    num += supportBokeh() ? 1 : 0;
    num += mTPIUsage.getTPILength(TPIOEntry::YUV, true) + 1; // skip div queue count
    num += supportWarpNode() ? 1 : 0;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumP2ABuffer() const
{
    MUINT32 num = ((mOutCfg.mMaxOutNum > 2) || mForceIMG3O) ? 3 : 0;
    num = max(num, get3DNRBufferNum().mBasic);
    num = max(num, getVendorBufferNum().mBasic);
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumP2APureBuffer() const
{
    MUINT32 num = max((MUINT32)3, getVendorBufferNum().mBasic);
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumDCESOBuffer() const
{
    return getNumSensor() * DCESO_DELAY_COUNT * P2A_TOTAL_PATH_NUM;
}

MUINT32 StreamingFeaturePipeUsage::getNumDepthImgBuffer() const
{
    // TODO consider DepthPipe buffer depth
    return 3;
}

MUINT32 StreamingFeaturePipeUsage::getNumBokehOutBuffer() const
{
    // TODO need confirm by VSDOF owner
    return 3;
}

MUINT32 StreamingFeaturePipeUsage::getNumP2ATuning() const
{
    MUINT32 num = MIN_P2A_TUNING_BUF_NUM;
    if(mOutCfg.mHasPhysical)
    {
        num *= 2;
    }
    if(supportP2ALarge())
    {
        num *= 2;
    }
    num = max(num, getNumP2ABuffer());
    if( supportDual() )
    {
        num *= 2;
    }

    if( mUsageHint.mDSDNParam.mMaxDSLayer > 1 )
    {
        num += 3 * (mUsageHint.mDSDNParam.mMaxDSLayer - 1);
    }

    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumP2ASyncTuning() const
{
    MUINT32 num = MIN_P2A_TUNING_BUF_NUM;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumWarpInBuffer() const
{
    MUINT32 num = 0;
    num = max(num, getEISBufferNum().mBasic);
    num += getVendorBufferNum().mBasic;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumExtraWarpInBuffer() const
{
    MUINT32 num = 0;
    num = max(num, getEISBufferNum().mExtra);
    num += getVendorBufferNum().mExtra;
    return num;
}

MUINT32 StreamingFeaturePipeUsage::getNumWarpOutBuffer() const
{
    return supportWarpNode() ? 3 : 0;
}

MUINT32 StreamingFeaturePipeUsage::getNumDSDN20Buffer() const
{
    return supportDSDN20() ? 4 : 0;
}

MUINT32 StreamingFeaturePipeUsage::getNumDSDN25Buffer() const
{
    return supportDSDN25() ? (3 * std::max<MUINT32>(getSMVRSpeed(), 1)) : 0;
}

MUINT32 StreamingFeaturePipeUsage::getMaxP2EnqueDepth() const
{
    return 3;
}

MVOID StreamingFeaturePipeUsage::updateOutSize(const std::vector<MSize> &out)
{
    mOutSizeVector = out;
    std::sort(mOutSizeVector.begin(), mOutSizeVector.end(),
              [](const MSize &lhs, const MSize &rhs)
                { return lhs.w*lhs.h > rhs.w*rhs.h; });

    for( const MSize &size : mOutSizeVector )
    {
        if( size.w > mStreamingSize.w || size.h > mStreamingSize.h )
        {
            MY_LOGI("large out size detected(%dx%d)", size.w, size.h);
            continue;
        }
        if( size.w > mMaxOutArea.w )
        {
            mMaxOutArea.w = size.w;
        }
        if( size.h > mMaxOutArea.h )
        {
            mMaxOutArea.h = size.h;
        }
    }

    if( supportVendorDebug() )
    {
        for( const MSize &s : out )
        {
            MY_LOGI("Out size=(%dx%d)", s.w, s.h);
        }
        for( const MSize &s : mOutSizeVector )
        {
            MY_LOGI("Out size=(%dx%d)", s.w, s.h);
        }
        MY_LOGI("mMaxOutArea=(%dx%d)", mMaxOutArea.w, mMaxOutArea.h);
    }
}

StreamingFeaturePipeUsage::BufferNumInfo StreamingFeaturePipeUsage::get3DNRBufferNum() const
{
    MUINT32 num = 0;
    if( support3DNR() )
    {
        num = 3;
    }

    return BufferNumInfo(num);
}

StreamingFeaturePipeUsage::BufferNumInfo StreamingFeaturePipeUsage::getEISBufferNum() const
{
    MUINT32 basic = 0, extra = 0;
    if( supportEISNode() )
    {
        basic = 5;
        if( supportEIS_Q() )
        {
            extra = getEISQueueSize();
        }
    }
    return BufferNumInfo(basic, extra);
}

StreamingFeaturePipeUsage::BufferNumInfo StreamingFeaturePipeUsage::getVendorBufferNum() const
{
    return supportTPI(TPIOEntry::YUV) ? BufferNumInfo(3+1) : BufferNumInfo(0);
}

MUINT32 StreamingFeaturePipeUsage::getSensorIndex() const
{
    return mSensorIndex;
}

MUINT32 StreamingFeaturePipeUsage::getEISQueueSize() const
{
    return mUsageHint.mEISInfo.queueSize;
}

MUINT32 StreamingFeaturePipeUsage::getEISStartFrame() const
{
    return mUsageHint.mEISInfo.startFrame;
}

MUINT32 StreamingFeaturePipeUsage::getEISVideoConfig() const
{
    return mUsageHint.mEISInfo.videoConfig;
}

MUINT32 StreamingFeaturePipeUsage::getWarpPrecision() const
{
    return supportWPE() ? 5 : WARP_MAP_PRECISION_BIT;
}

TPIUsage StreamingFeaturePipeUsage::getTPIUsage() const
{
    return mTPIUsage;
}

MUINT32 StreamingFeaturePipeUsage::getTPINodeCount(TPIOEntry entry) const
{
    return mTPIUsage.getNodeCount(entry);
}

MBOOL StreamingFeaturePipeUsage::supportTPI(TPIOEntry entry) const
{
    return mTPIUsage.useEntry(entry);
}

MBOOL StreamingFeaturePipeUsage::supportTPI(TPIOEntry entry, TPIOUse use) const
{
    return mTPIUsage.use(entry, use);
}

MBOOL StreamingFeaturePipeUsage::supportVendorDebug() const
{
    return mVendorDebug;
}

MBOOL StreamingFeaturePipeUsage::supportVendorLog() const
{
    return mVendorLog;
}

MBOOL StreamingFeaturePipeUsage::supportVendorErase() const
{
    return mVendorErase;
}

MBOOL StreamingFeaturePipeUsage::supportSharedInput() const
{
    return mSupportSharedInput;
}

MBOOL StreamingFeaturePipeUsage::supportXNode() const
{
    return mSupportXNode && !supportVMDPNode(SFN_HINT::TWIN);
}

MBOOL StreamingFeaturePipeUsage::isSecureP2() const
{
    return (mSecType != NSCam::SecType::mem_normal);
}

MBOOL StreamingFeaturePipeUsage::isForceP2Zoom() const
{
    return FORCE_ZOOM_BEFORE_TPI && (mUsageHint.mAllSensorIDs.size() == 1);
}


NSCam::SecType StreamingFeaturePipeUsage::getSecureType() const
{
    return mSecType;
}

MBOOL StreamingFeaturePipeUsage::getInitP2G() const
{
    return MTRUE;
}

MBOOL StreamingFeaturePipeUsage::supportHDR10() const
{
    return mUsageHint.mIsHdr10;
}

MINT32 StreamingFeaturePipeUsage::getHDR10Spec() const
{
    return mUsageHint.mHdr10Spec;
}

MUINT32 StreamingFeaturePipeUsage::getP2RBMask() const
{
    return mUsageHint.mP2RBMask;
}

} // NSFeaturePipe
} // NSCamFeature
} // NSCam
