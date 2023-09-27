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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_PIPE_USAGE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_PIPE_USAGE_H_

//#include "MtkHeader.h"
#include <mtkcam3/feature/featurePipe/IStreamingFeaturePipe.h>
#include <mtkcam3/feature/3dnr/3dnr_defs.h>
#include "tpi/inc/tpi_type.h"
#include "TPIUsage.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

enum class SFN_HINT
{
    NONE = 0,
    PREVIEW,
    RECORD,
    TWIN,
};

class StreamingFeaturePipeUsage
{
public:
    enum eUsageMask
    {
        PIPE_USAGE_NONE           = 0,
        PIPE_USAGE_EIS            = 1 << 0,
        PIPE_USAGE_3DNR           = 1 << 1,
        PIPE_USAGE_VENDOR         = 1 << 2,
        PIPE_USAGE_EARLY_DISPLAY  = 1 << 3,
        PIPE_USAGE_SMVR           = 1 << 4,
        PIPE_USAGE_DSDN           = 1 << 5,
        PIPE_USAGE_RSC            = 1 << 6,
    };

public:
    StreamingFeaturePipeUsage();
    StreamingFeaturePipeUsage(IStreamingFeaturePipe::UsageHint hint, MUINT32 sensorIndex);

    MVOID config(const TPIMgr *mgr);

    MBOOL supportP2G() const;
    MBOOL supportP2AP2() const;
    MBOOL supportDepthP2() const;
    MBOOL supportSMP2() const;

    MBOOL supportLargeOut() const;
    MBOOL supportPhysicalOut() const;
    MBOOL supportIMG3O() const;

    MBOOL supportVNRNode() const;
    MBOOL supportEISNode() const;
    MBOOL supportWarpNode(SFN_HINT hint = SFN_HINT::NONE) const;
    MBOOL supportVMDPNode(SFN_HINT hint = SFN_HINT::NONE) const;
    MBOOL supportTrackingFocus() const;
    MBOOL supportRSCNode() const;
    MBOOL supportAISS() const;
    MBOOL supportTOFNode() const;
    MBOOL supportP2ALarge() const;

    MBOOL support4K2K() const;
    MBOOL support60FPS() const;

    MBOOL supportTimeSharing() const;
    MBOOL supportP2AFeature() const;
    MBOOL supportBypassP2A() const;
    MBOOL supportP1YUVIn() const;
    MBOOL supportPure() const;

    MBOOL encodePreviewEIS() const;
    MBOOL supportPreviewEIS() const;
    MBOOL supportEIS_Q() const;
    MBOOL supportEIS_TSQ() const;
    MBOOL supportEIS_RQQ() const;
    MBOOL supportTPI_TSQ() const;
    MBOOL supportTPI_RQQ() const;
    MBOOL supportEISRSC() const;
    MBOOL supportWPE() const;

    MBOOL supportDual() const;
    MBOOL supportDepth() const;
    MBOOL supportDPE() const;
    MBOOL supportBokeh() const;
    MBOOL supportTOF() const;
    MBOOL supportP2GWithDepth() const;

    MBOOL support3DNR() const;
    MBOOL support3DNRRSC() const;
    MBOOL support3DNRLMV() const;
    MBOOL supportDSDN20() const;
    MBOOL supportDSDN25() const;
    MBOOL supportDSDN30() const;
    MBOOL isGyroEnable() const;
    MBOOL supportDSDN() const;
    MBOOL is3DNRModeMaskEnable(NR3D::E3DNR_MODE_MASK mask) const;
    MBOOL supportFSC() const;
    MBOOL supportVendorFSCFullImg() const;
    MBOOL supportEnlargeRsso() const;

    MBOOL supportFull_YUY2() const;
    MBOOL supportFull_NV21() const;
    MBOOL supportGraphicBuffer() const;
    MBOOL supportSlaveNR() const;
    MBOOL supportPhysicalNR() const;
    MBOOL supportPhysicalDSDN20() const;
    EImageFormat getFullImgFormat() const;
    EImageFormat getVNRImgFormat() const;
    MINT32 getDMAConstrain() const;
    MBOOL printNextIO() const;

    std::vector<MUINT32> getAllSensorIDs() const;
    MUINT32 getMode() const;
    MUINT32 getEISMode() const;
    MUINT32 getDualMode() const;
    MUINT32 get3DNRMode() const;
    MUINT32 getAISSMode() const;
    MUINT32 getFSCMode() const;
    MUINT32 getP1Batch() const;
    MUINT32 getSMVRSpeed() const;
    TP_MASK_T getTPMask() const;
    MFLOAT getTPMarginRatio() const;
    IMetadata getAppSessionMeta() const;
    MSize   getStreamingSize() const;
    MSize   getFDSize() const;
    MSize   getRrzoSizeByIndex(MUINT32 index);
    MSize   getMaxOutSize() const;
    MSize   getVideoSize() const;
    MUINT32 getNumSensor() const;
    MUINT32 getNumScene() const;

    MUINT32 getNumP2GExtImg() const;
    MUINT32 getNumP2GExtPQ() const;
    MUINT32 getP2GLength() const;
    MUINT32 getPostProcLength() const;
    MUINT32 getDispPostProcLength() const;

    MUINT32 getNumP2ABuffer() const;
    MUINT32 getNumP2APureBuffer() const;
    MUINT32 getNumDCESOBuffer() const;
    MUINT32 getNumDepthImgBuffer() const;
    MUINT32 getNumBokehOutBuffer() const;
    MUINT32 getNumP2ATuning() const;
    MUINT32 getNumP2ASyncTuning() const;
    MUINT32 getNumWarpInBuffer() const;
    MUINT32 getNumExtraWarpInBuffer() const;
    MUINT32 getNumWarpOutBuffer() const;
    MUINT32 getNumDSDN20Buffer() const;
    MUINT32 getNumDSDN25Buffer() const;
    MUINT32 getMaxP2EnqueDepth() const;
    MUINT32 getSensorIndex() const;
    MUINT32 getDualSensorIndex_Wide() const;
    MUINT32 getDualSensorIndex_Tele() const;
    MUINT32 getEISFactor() const;
    MUINT32 getEISQueueSize() const;
    MUINT32 getEISStartFrame() const;
    MUINT32 getEISVideoConfig() const;
    MUINT32 getWarpPrecision() const;
    DSDNParam getDSDNParam() const;
    MUINT32 getSensorModule() const;
    MBOOL useScenarioRecorder() const;
    NSPipelinePlugin::TPICustomConfig getTPICustomConfig() const;

    TPIUsage getTPIUsage() const;
    MUINT32 getTPINodeCount(TPIOEntry entry) const;
    MBOOL supportTPI(TPIOEntry entry) const;
    MBOOL supportTPI(TPIOEntry entry, TPIOUse use) const;
    MBOOL supportVendorDebug() const;
    MBOOL supportVendorLog() const;
    MBOOL supportVendorErase() const;
    MBOOL supportSharedInput() const;

    MBOOL supportXNode() const;
    MBOOL isSecureP2() const;
    MBOOL isForceP2Zoom() const;
    NSCam::SecType getSecureType() const;

    MBOOL supportHDR10() const;
    MINT32 getHDR10Spec() const;
    MUINT32 getP2RBMask() const;
private:
    MBOOL getInitP2G() const;

private:
    enum P2A_MODE_ENUM
    {
        P2A_MODE_DISABLE,
        P2A_MODE_NORMAL,
        P2A_MODE_TIME_SHARING,
        P2A_MODE_FEATURE,
        P2A_MODE_BYPASS,
    };

    class BufferNumInfo
    {
    public:
        BufferNumInfo()
            : mBasic(0)
            , mExtra(0)
        {
        }

        BufferNumInfo(MUINT32 basic, MUINT32 extra=0)
            : mBasic(basic)
            , mExtra(extra)
        {
        }

        ~BufferNumInfo() {}

    public:
        MUINT32 mBasic;
        MUINT32 mExtra;
    };

    MVOID updateOutSize(const std::vector<MSize> &out);
    BufferNumInfo get3DNRBufferNum() const;
    BufferNumInfo getEISBufferNum() const;
    BufferNumInfo getVendorBufferNum() const;

    IStreamingFeaturePipe::UsageHint    mUsageHint;
    TPIUsage                            mTPIUsage;
    MUINT32                             mPipeFunc = 0;
    MUINT32                             mP2AMode = 0;

    MSize                               mStreamingSize;
    std::vector<MSize>                  mOutSizeVector;
    MSize                               mMaxOutArea;
    MBOOL                               mVendorDebug = MFALSE;
    MBOOL                               mVendorLog = MFALSE;
    MBOOL                               mVendorErase = MFALSE;
    MUINT32                             m3DNRMode = 0;
    MUINT32                             mAISSMode = 0;
    MUINT32                             mFSCMode = 0;
    MUINT32                             mDualMode = 0;
    MUINT32                             mSensorIndex = INVALID_SENSOR;
    IStreamingFeaturePipe::OutConfig    mOutCfg;
    MUINT32                             mNumSensor = 0;
    MBOOL                               mSupportPure = MFALSE;
    MBOOL                               mForceIMG3O = MFALSE;
    MBOOL                               mPrintNextIO = MFALSE;
    NSCam::SecType                      mSecType = NSCam::SecType::mem_normal;
    std::map<MUINT32, MSize>            mResizedRawSizeMap;
    MBOOL                               mUseP2G = MFALSE;
    MBOOL                               mSupportSharedInput = MFALSE;
    MBOOL                               mSupportSlaveNR = MFALSE;
    MBOOL                               mSupportPhysicalNR = MFALSE;
    MBOOL                               mSupportPhysicalDSDN20 = MFALSE;
    MBOOL                               mForceWarpP = MFALSE;
    MBOOL                               mForceWarpR = MFALSE;
    MBOOL                               mForceVmdpP = MFALSE;
    MBOOL                               mForceVmdpR = MFALSE;
    MBOOL                               mForceTpiEisTsq = MFALSE;
    MBOOL                               mEncodePreviewBuf = MFALSE;
    MBOOL                               mSupportXNode = MFALSE;
    MBOOL                               mUseScenarioRecorder = MFALSE;
};

} // NSFeaturePipe
} // NSCamFeature
} // NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_PIPE_USAGE_H_

