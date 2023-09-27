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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_PIPE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_PIPE_H_

#include <list>
#include <utils/Mutex.h>

#include <featurePipe/core/include/CamPipe.h>
#include <mtkcam3/feature/featurePipe/util/FeaturePipeDebugee.h>
#include "StreamingFeatureNode.h"
#include "ImgBufferStore.h"
#include "StreamingFeaturePipeUsage.h"
#include "p2g/P2GMgr.h"

#include "RootNode.h"
#include "P2AMDPNode.h"
#include "G_P2ANode.h"
#include "G_P2SMNode.h"
#include "P2SWNode.h"
#include "MSSNode.h"
#include "MSFNode.h"
#include "VNRNode.h"
#include "WarpNode.h"
#include "EISNode.h"
#include "RSCNode.h"
#include "HelperNode.h"
#include "VendorMDPNode.h"
#include "DepthNode.h"
#include "BokehNode.h"
#include "XNode.h"
#include "TPINode.h"
#include "TPI_AsyncNode.h"
#include "TPI_DispNode.h"
#include "FrameControl.h"
#include "TrackingNode.h"

#include <mtkcam3/feature/utils/p2/DIPStream.h>
using Feature::P2Util::DipUsageHint;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class StreamingFeaturePipe : public CamPipe<StreamingFeatureNode>, public StreamingFeatureNode::Handler_T, public IStreamingFeaturePipe
{
public:
    StreamingFeaturePipe(MUINT32 sensorIndex, const UsageHint &usageHint);
    virtual ~StreamingFeaturePipe();

public:
    // IStreamingFeaturePipe Members
    virtual void setSensorIndex(MUINT32 sensorIndex);
    virtual MBOOL init(const char *name=NULL);
    virtual MBOOL uninit(const char *name=NULL);
    virtual MBOOL enque(const FeaturePipeParam &param);
    virtual MVOID notifyFlush();
    virtual MBOOL flush();
    virtual MBOOL setJpegParam(NSCam::NSIoPipe::NSPostProc::EJpgCmd cmd, int arg1, int arg2);
    virtual MBOOL setFps(MINT32 fps);
    virtual MUINT32 getRegTableSize();
    virtual MBOOL sendCommand(NSCam::NSIoPipe::NSPostProc::ESDCmd cmd, MINTPTR arg1=0, MINTPTR arg2=0, MINTPTR arg3=0);
    virtual MBOOL addMultiSensorID(MUINT32 sensorID);

public:
    virtual MVOID sync();

protected:
    typedef CamPipe<StreamingFeatureNode> PARENT_PIPE;
    virtual MBOOL onInit();
    virtual MVOID onUninit();

protected:
    virtual MBOOL onData(DataID id, const RequestPtr &data);

private:
    MBOOL earlyInit();
    MVOID lateUninit();
    MBOOL initNodes();
    MBOOL uninitNodes();
    MVOID initP2AGroup();
    MVOID uninitP2AGroup();
    MVOID initTPIGroup();
    MVOID uninitTPIGroup();

    MBOOL prepareDebugSetting();
    MBOOL prepareGeneralPipe();
    MBOOL prepareNodeSetting();
    MBOOL prepareNodeConnection();
    MBOOL prepareIOControl();
    MBOOL prepareBuffer();
    MBOOL prepareSyncCtrl();
    MVOID prepareFeatureRequest(const FeaturePipeParam &param);
    MVOID prepareEISQControl(const FeaturePipeParam &param);
    MVOID prepareIORequest(const RequestPtr &request);

    android::sp<IBufferPool> createFullImgPool(const char* name, MSize size);
    android::sp<IBufferPool> createImgPool(const char* name, MSize size, EImageFormat fmt);
    android::sp<IBufferPool> createWarpOutputPool(const char* name, MSize size);

    MVOID releaseGeneralPipe();
    MVOID releaseNodeSetting();
    MVOID releaseBuffer();

    MVOID applyMaskOverride(const RequestPtr &request);
    MVOID applyVarMapOverride(const RequestPtr &request);

    DipUsageHint toDipUsageHint(const StreamingFeaturePipeUsage &sfpUsage) const;
private:
    MUINT32 mForceOnMask;
    MUINT32 mForceOffMask;
    MUINT32 mSensorIndex;
    StreamingFeaturePipeUsage mPipeUsage;
    MUINT32 mCounter = 0;
    MUINT32 mRecordCounter;
    FPSCounter mDisplayFPSCounter;
    FPSCounter mFrameFPSCounter;

    MINT32 mDebugDump;
    MINT32 mDebugDumpCount;
    MBOOL mDebugDumpByRecordNo;
    MBOOL mForceIMG3O;
    MBOOL mForceWarpPass;
    MUINT32 mForceGpuOut;
    MBOOL mForceGpuRGBA;
    MBOOL mUsePerFrameSetting;
    MBOOL mForcePrintIO;
    MBOOL mEarlyInited;
    MBOOL mEnSyncCtrl;

    RootNode mRootNode;
    P2AMDPNode mP2AMDP;
    G_P2ANode mG_P2A;
    G_P2SMNode mG_P2SM;
    P2SWNode mP2SW;
    MSSNode mMSS;
    MSSNode mPMSS;
    MSFNode mMSF;
    VNRNode mVNR;
    TPI_AsyncNode mAsync;
    TPI_DispNode mDisp;
    WarpNode mWarpP;
    WarpNode mWarpR;
    DepthNode mDepth;
    BokehNode mBokeh;
    TrackingNode mTracking;

    EISNode mEIS;
    RSCNode mRSC;
    HelperNode mHelper;
    VendorMDPNode mVmdpP;
    VendorMDPNode mVmdpR;
    std::vector<TPINode*> mTPIs;
    XNode mXNode;

    android::sp<IBufferPool> mFullImgPool;
    android::sp<IBufferPool> mDepthYuvOutPool;
    android::sp<IBufferPool> mBokehOutPool;
    android::sp<IBufferPool> mEisFullImgPool;
    android::sp<IBufferPool> mWarpOutputPool;
    android::sp<IBufferPool> mTPIBufferPool;
    android::sp<SharedBufferPool> mTPISharedBufferPool;
    android::sp<IBufferPool> mVNRInputPool;
    android::sp<IBufferPool> mVNROutputPool;

    Feature::P2Util::DIPStream *mDIPStream = NULL;

    typedef std::list<StreamingFeatureNode*> NODE_LIST;
    NODE_LIST mNodes;

    NODE_LIST mDisplayPath;
    NODE_LIST mRecordPath;
    NODE_LIST mPhysicalPath;
    NODE_LIST mPreviewCallbackPath;
    NODE_LIST mAsyncPath;

    android::sp<NodeSignal> mNodeSignal;
    std::shared_ptr<FrameControl> mMSSSync;
    std::shared_ptr<FrameControl> mPMSSSync;
    std::shared_ptr<FrameControl> mMotionSync;
    std::shared_ptr<FrameControl> mP2L1Sync;
    std::shared_ptr<P2G::P2GMgr> mP2GMgr;
    TPIMgr *mTPIMgr = NULL;

    EISQControl mEISQControl;

    NextIO::Control mIOControl;
    std::vector<MUINT32> mAllSensorIDs;

    std::shared_ptr<FeaturePipeDebugee<StreamingFeaturePipe>> mFPDebugee;
    EnqueInfo   mLastEnqInfo;

    MBOOL mUseG = MFALSE;
};


} // NSFeaturePipe
} // NSCamFeature
} // NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_PIPE_H_
