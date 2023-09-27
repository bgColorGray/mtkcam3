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

#include "StreamingFeaturePipe.h"

#define PIPE_CLASS_TAG "Pipe"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_PIPE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
#include <mtkcam3/feature/utils/p2/LogReport.h>

#define NORMAL_STREAM_NAME "StreamingFeature"

#include <mtkcam/utils/std/ULog.h>

#include "p2g/P2GMgr.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);
using namespace NSCam::Utils::ULog;

using namespace NSCam::NSIoPipe::NSPostProc;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

#define TO_P2A()  mG_P2A
#define TO_P2SM() mG_P2SM

StreamingFeaturePipe::StreamingFeaturePipe(MUINT32 sensorIndex, const UsageHint &usageHint)
    : CamPipe<StreamingFeatureNode>("StreamingFeaturePipe")
    , mForceOnMask(0)
    , mForceOffMask(~0)
    , mSensorIndex(sensorIndex)
    , mPipeUsage(usageHint, sensorIndex)
    , mCounter(0)
    , mRecordCounter(0)
    , mDebugDump(0)
    , mDebugDumpCount(1)
    , mDebugDumpByRecordNo(MFALSE)
    , mForceIMG3O(MFALSE)
    , mForceWarpPass(MFALSE)
    , mForceGpuOut(NO_FORCE)
    , mForceGpuRGBA(MFALSE)
    , mUsePerFrameSetting(MFALSE)
    , mForcePrintIO(MFALSE)
    , mEarlyInited(MFALSE)
    , mEnSyncCtrl(MTRUE)
    , mRootNode("fpipe.root")
    , mP2AMDP("fpipe.p2amdp")
    , mG_P2A("fpipe.g_p2a")
    , mG_P2SM("fpipe.g_p2sm")
    , mP2SW("fpipe.p2ispsw")
    , mMSS("fpipe.mss", MSSNode::T_CURRENT)
    , mPMSS("fpipe.pmss", MSSNode::T_PREVIOUS)
    , mMSF("fpipe.msf")
    , mVNR("fpipe.vnr")
    , mAsync("fpipe.async")
    , mDisp("fpipe.disp")
    , mWarpP("fpipe.warp_p", SFN_HINT::PREVIEW)
    , mWarpR("fpipe.warp_r", SFN_HINT::RECORD)
    , mDepth("fpipe.depth")
    , mBokeh("fpipe.bokeh")
    , mEIS("fpipe.eis")
    , mRSC("fpipe.rsc")
    , mHelper("fpipe.helper")
    , mVmdpP("fpipe.vmdp_p", SFN_HINT::PREVIEW)
    , mVmdpR("fpipe.vmdp_r", SFN_HINT::RECORD)
    , mXNode("fpipe.xnode")
    , mTracking("fpipe.tracking")
{
    TRACE_FUNC_ENTER();
    mUseG = mPipeUsage.supportP2G();

    MY_LOGI("create pipe(%p): SensorIndex=%d sensorNum(%d) dualMode(%d),UsageMode=%d EisMode=0x%x 3DNRMode=%d AISSMode=%d DSDN(mode/m/d/layer/p1ctrl/10bit)=(%d/%d/%d/%d/%d/%d) tsq=%d StreamingSize=%dx%d Out(max/phy/large/fd/video)=(%d/%d/%d/%dx%d/%dx%d) In(fps/p1Batch)=(%d/%d), secType=%d HDR10=(%d/%d)", this, mSensorIndex, mPipeUsage.getNumSensor(),
        mPipeUsage.getDualMode(), mPipeUsage.getMode(), mPipeUsage.getEISMode(), mPipeUsage.get3DNRMode(), mPipeUsage.getAISSMode(),
        usageHint.mDSDNParam.mMode, usageHint.mDSDNParam.mMaxDsRatio.mMul, usageHint.mDSDNParam.mMaxDsRatio.mDiv, usageHint.mDSDNParam.mMaxDSLayer, usageHint.mDSDNParam.mDynamicP1Ctrl, usageHint.mDSDNParam.isDSDN20Bit10(), usageHint.mUseTSQ,
        usageHint.mStreamingSize.w, usageHint.mStreamingSize.h,
        usageHint.mOutCfg.mMaxOutNum, usageHint.mOutCfg.mHasPhysical, usageHint.mOutCfg.mHasLarge, usageHint.mOutCfg.mFDSize.w, usageHint.mOutCfg.mFDSize.h,
        usageHint.mOutCfg.mVideoSize.w, usageHint.mOutCfg.mVideoSize.h,
        usageHint.mInCfg.mReqFps, usageHint.mInCfg.mP1Batch,
        usageHint.mSecType, usageHint.mIsHdr10, usageHint.mHdr10Spec);

    mAllSensorIDs = mPipeUsage.getAllSensorIDs();
    mNodeSignal = new NodeSignal();
    if( mNodeSignal == NULL )
    {
        MY_LOGE("OOM: cannot create NodeSignal");
    }

    mEarlyInited = earlyInit();
    TRACE_FUNC_EXIT();
}

StreamingFeaturePipe::~StreamingFeaturePipe()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("destroy pipe(%p): SensorIndex=%d", this, mSensorIndex);
    lateUninit();
    // must call dispose to free CamGraph
    this->dispose();
    TRACE_FUNC_EXIT();
}

void StreamingFeaturePipe::setSensorIndex(MUINT32 sensorIndex)
{
    TRACE_FUNC_ENTER();
    this->mSensorIndex = sensorIndex;
    TRACE_FUNC_EXIT();
}

MBOOL StreamingFeaturePipe::init(const char *name)
{
    TRACE_FUNC_ENTER();
    (void)name;
    MBOOL ret = MFALSE;

    initTPIGroup();
    initP2AGroup();
    initNodes();
    mEISQControl.init(mPipeUsage);

    ret = PARENT_PIPE::init();

    mFPDebugee = std::make_shared<FeaturePipeDebugee<StreamingFeaturePipe>>(this);

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::uninit(const char *name)
{
    TRACE_FUNC_ENTER();
    (void)name;
    MBOOL ret;
    mFPDebugee = NULL;
    ret = PARENT_PIPE::uninit();

    uninitNodes();
    uninitP2AGroup();
    uninitTPIGroup();

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::enque(const FeaturePipeParam &param)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( !param.mP2Pack.isValid() )
    {
        MY_LOGE("Invalid P2Pack, directly assert.");
        return MFALSE;
    }
    this->prepareFeatureRequest(param);
    RequestPtr request;
    CAM_ULOG_SUBREQS(MOD_P2_STR_PROC, REQ_P2_STR_REQUEST, param.mP2Pack.mLog.getLogFrameID(), REQ_STR_FPIPE_REQUEST, mCounter);
    CAM_ULOG_ENTER(MOD_FPIPE_STREAMING, REQ_STR_FPIPE_REQUEST, mCounter);
    request = new StreamingFeatureRequest(mPipeUsage, param, mCounter, mRecordCounter, mEISQControl.getCurrentState());
    if(request == NULL)
    {
        MY_LOGE("OOM: Cannot allocate StreamingFeatureRequest");
        CAM_ULOG_DISCARD(MOD_FPIPE_STREAMING, REQ_STR_FPIPE_REQUEST, mCounter);
    }
    else
    {
        request->calSizeInfo();
        request->setLastEnqInfo(mLastEnqInfo);
        mLastEnqInfo = request->getCurEnqInfo();
        request->setDisplayFPSCounter(&mDisplayFPSCounter);
        request->setFrameFPSCounter(&mFrameFPSCounter);
        if( mTPIMgr != NULL &&
            (mPipeUsage.supportTPI(TPIOEntry::YUV) ||
             mPipeUsage.supportTPI(TPIOEntry::ASYNC) ) &&
            request->hasGeneralOutput() )
        {
            mTPIMgr->genFrame(request->mTPIFrame, request->getAppMeta());
        }
        if( mPipeUsage.supportTPI(TPIOEntry::YUV) &&
            request->mTPIFrame.needEntry(TPIOEntry::YUV) )
        {
            ENABLE_TPI_YUV(request->mFeatureMask);
        }
        if( mPipeUsage.supportTPI(TPIOEntry::ASYNC) &&
            request->hasDisplayOutput() &&
            request->mTPIFrame.needEntry(TPIOEntry::ASYNC) &&
            mAsync.queryFrameEnable() )
        {
            ENABLE_TPI_ASYNC(request->mFeatureMask);
        }
        if( mUsePerFrameSetting )
        {
            this->prepareDebugSetting();
        }
        this->applyMaskOverride(request);
        this->applyVarMapOverride(request);
        mNodeSignal->notifyEnque();
        prepareIORequest(request);
        ret = CamPipe::enque(ID_ROOT_ENQUE, request);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MVOID StreamingFeaturePipe::notifyFlush()
{
    TRACE_FUNC_ENTER();
    MY_LOGD("Notify flush");
    TRACE_FUNC_EXIT();
}

MBOOL StreamingFeaturePipe::flush()
{
    TRACE_FUNC_ENTER();
    MY_LOGD("Trigger flush");
    mNodeSignal->setStatus(NodeSignal::STATUS_IN_FLUSH);
    if( mPipeUsage.supportEIS_Q() )
    {
        MY_LOGD("Notify EIS: flush begin");
        mEIS.triggerDryRun();
    }
    if( mPipeUsage.supportTPI_RQQ() )
    {
        MY_LOGD("Notify TPI RQQ: flush begin");
        for( auto& pTPI : mTPIs )
        {
            pTPI->triggerDryRun();
        }
    }
    CamPipe::sync();
    if (mPipeUsage.supportDPE())
    {
        MY_LOGD("Notify DepthNodeL: Flush");
        mDepth.onFlush();
    }
    mEISQControl.reset();
    mWarpP.clearTSQ();
    mWarpR.clearTSQ();
    mNodeSignal->clearStatus(NodeSignal::STATUS_IN_FLUSH);
    if( mPipeUsage.supportEIS_Q() )
    {
        MY_LOGD("Notify EIS: flush end");
        mEIS.triggerDryRun();
    }
    if( mPipeUsage.supportSMP2() )
    {
        mG_P2SM.flush();
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL StreamingFeaturePipe::setJpegParam(NSCam::NSIoPipe::NSPostProc::EJpgCmd cmd, int arg1, int arg2)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( mDIPStream != NULL )
    {
        ret = mDIPStream->setJpegParam(cmd, arg1, arg2);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::setFps(MINT32 fps)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( mDIPStream != NULL )
    {
        ret = mDIPStream->setFps(fps);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MUINT32 StreamingFeaturePipe::getRegTableSize()
{
    TRACE_FUNC_ENTER();
    MUINT32 ret = 0;
    if( mDIPStream != NULL )
    {
        ret = mDIPStream->getRegTableSize();
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::sendCommand(NSCam::NSIoPipe::NSPostProc::ESDCmd cmd, MINTPTR arg1, MINTPTR arg2, MINTPTR arg3)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( mDIPStream != NULL )
    {
        ret = mDIPStream->sendCommand(cmd, arg1, arg2, arg3);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::addMultiSensorID(MUINT32 sensorID)
{
    (void)sensorID;
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID StreamingFeaturePipe::sync()
{
    TRACE_FUNC_ENTER();
    MY_LOGD("Sync start");
    CamPipe::sync();
    MY_LOGD("Sync finish");
    TRACE_FUNC_EXIT();
}

MBOOL StreamingFeaturePipe::onInit()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    MY_LOGI("+");
    MBOOL ret;
    ret = mEarlyInited &&
          this->prepareDebugSetting() &&
          this->prepareNodeSetting() &&
          this->prepareNodeConnection() &&
          this->prepareSyncCtrl() &&
          this->prepareIOControl() &&
          this->prepareBuffer();

    MY_LOGI("-");
    TRACE_FUNC_EXIT();
    return ret;
}

MVOID StreamingFeaturePipe::onUninit()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("+");
    this->releaseBuffer();
    this->releaseNodeSetting();
    MY_LOGI("-");
    TRACE_FUNC_EXIT();
}

MBOOL StreamingFeaturePipe::onData(DataID, const RequestPtr &)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::earlyInit()
{
    return this->prepareGeneralPipe();
}

MVOID StreamingFeaturePipe::lateUninit()
{
    this->releaseGeneralPipe();
}

MBOOL StreamingFeaturePipe::initNodes()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    mNodes.push_back(&mRootNode);

    if( mPipeUsage.supportDepthP2() )
    {
        mNodes.push_back(&mDepth);
        if( mPipeUsage.supportBokeh() )     mNodes.push_back(&mBokeh);
    }
    else if( mPipeUsage.supportSMP2() )
    {
        mNodes.push_back(&TO_P2SM());
        mNodes.push_back(&mP2AMDP);
    }
    else
    {
        mNodes.push_back(&TO_P2A());
        mNodes.push_back(&mP2AMDP);
    }

    if( mPipeUsage.supportP2GWithDepth() )
    {
        mNodes.push_back(&mDepth);
        if( mPipeUsage.supportBokeh() )     mNodes.push_back(&mBokeh);
    }

    if( mPipeUsage.supportDSDN20() )
    {
        mNodes.push_back(&mVNR);
    }
    else if( mPipeUsage.supportDSDN25() || mPipeUsage.supportDSDN30() )
    {
        mNodes.push_back(&mMSS);
        mNodes.push_back(&mMSF);
        mNodes.push_back(&mP2SW);
        if( mPipeUsage.supportDSDN30() )
        {
            mNodes.push_back(&mPMSS);
        }
    }

    if( mPipeUsage.supportTrackingFocus() ) mNodes.push_back(&mTracking);
    if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  mNodes.push_back(&mWarpP);
    if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   mNodes.push_back(&mWarpR);
    if( mPipeUsage.supportRSCNode() )           mNodes.push_back(&mRSC);
    if( mPipeUsage.supportEISNode() )           mNodes.push_back(&mEIS);
    if( mPipeUsage.supportTPI(TPIOEntry::YUV) )
    {
        MUINT32 count = mPipeUsage.getTPINodeCount(TPIOEntry::YUV);
        mTPIs.clear();
        mTPIs.reserve(count);
        char name[32];
        TPIUsage tpiUsage = mPipeUsage.getTPIUsage();
        for( MUINT32 i = 0; i < count; ++i )
        {
            TPI_NodeInfo nodeInfo = tpiUsage.getTPIO(TPIOEntry::YUV, i).mNodeInfo;
            int res = snprintf(name, sizeof(name), "fpipe.tpi.%d.%s.%d", i, nodeInfo.mName.c_str(), nodeInfo.mNodeID);
            MY_LOGE_IF((res < 0), "create TPI snprintf FAIL!");
            TPINode *tpi = new TPINode(name, i);
            tpi->setTPIMgr(mTPIMgr);
            mTPIs.push_back(tpi);
            mNodes.push_back(tpi);
        }
    }
    if( mPipeUsage.supportTPI(TPIOEntry::DISP) )
    {
        mDisp.setTPIMgr(mTPIMgr);
        mNodes.push_back(&mDisp);
    }
    if( mPipeUsage.supportTPI(TPIOEntry::ASYNC) )
    {
        mAsync.setTPIMgr(mTPIMgr);
        mNodes.push_back(&mAsync);
    }
    if( mPipeUsage.supportTPI(TPIOEntry::META) )
    {
        mHelper.setTPIMgr(mTPIMgr);
    }

    if( mPipeUsage.supportVMDPNode(SFN_HINT::PREVIEW) )  mNodes.push_back(&mVmdpP);
    if( mPipeUsage.supportVMDPNode(SFN_HINT::RECORD) )  mNodes.push_back(&mVmdpR);

    if( mPipeUsage.supportXNode() )
    {
        mNodes.push_back(&mXNode);
    }

    mNodes.push_back(&mHelper);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL StreamingFeaturePipe::uninitNodes()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    for( unsigned i = 0, n = mTPIs.size(); i < n; ++i )
    {
        delete mTPIs[i];
        mTPIs[i] = NULL;
    }
    mTPIs.clear();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID StreamingFeaturePipe::initP2AGroup()
{
    TRACE_FUNC_ENTER();
    if( !mP2GMgr )
    {
        mP2GMgr = std::make_shared<P2G::P2GMgr>();
        mP2GMgr->init(mPipeUsage);
        mG_P2A.setP2GMgr(mP2GMgr);
        mG_P2SM.setP2GMgr(mP2GMgr);
        mP2SW.setP2GMgr(mP2GMgr);
        mMSS.setP2GMgr(mP2GMgr);
        mPMSS.setP2GMgr(mP2GMgr);
        mMSF.setP2GMgr(mP2GMgr);
        mP2AMDP.setP2GMgr(mP2GMgr);
    }
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::uninitP2AGroup()
{
    TRACE_FUNC_ENTER();
    if( mP2GMgr )
    {
        mP2GMgr->uninit();
        mP2GMgr = NULL;
    }
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::initTPIGroup()
{
    TRACE_FUNC_ENTER();
    mTPIMgr = TPIMgr::createInstance();
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TPI init_config");
    mTPIMgr->createSession(mSensorIndex, mAllSensorIDs, mPipeUsage.getNumSensor(), mPipeUsage.getTPMask(), mPipeUsage.getTPMarginRatio(), mPipeUsage.getStreamingSize(), mPipeUsage.getMaxOutSize(), mPipeUsage.getAppSessionMeta(), mPipeUsage.getTPICustomConfig(), mPipeUsage.supportEISNode());
    mTPIMgr->initSession();
    mPipeUsage.config(mTPIMgr);
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::uninitTPIGroup()
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TPI uninit");
    mTPIMgr->uninitSession();
    mTPIMgr->destroySession();
    TPIMgr::destroyInstance(mTPIMgr);
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    TRACE_FUNC_EXIT();
}

MBOOL StreamingFeaturePipe::prepareDebugSetting()
{
    TRACE_FUNC_ENTER();

    mForceOnMask = 0;
    mForceOffMask = ~0;

    #define CHECK_DEBUG_MASK(name)                                          \
    {                                                                       \
        MINT32 prop = getPropertyValue(KEY_FORCE_##name, VAL_FORCE_##name); \
        if( prop == FORCE_ON )    ENABLE_##name(mForceOnMask);              \
        if( prop == FORCE_OFF )   DISABLE_##name(mForceOffMask);            \
    }
    CHECK_DEBUG_MASK(EIS);
    CHECK_DEBUG_MASK(EIS_30);
    CHECK_DEBUG_MASK(EIS_QUEUE);
    CHECK_DEBUG_MASK(3DNR);
    CHECK_DEBUG_MASK(3DNR_S);
    CHECK_DEBUG_MASK(VHDR);
    CHECK_DEBUG_MASK(DSDN20);
    CHECK_DEBUG_MASK(DSDN20_S);
    CHECK_DEBUG_MASK(DSDN25);
    CHECK_DEBUG_MASK(DSDN25_S);
    CHECK_DEBUG_MASK(DSDN30);
    CHECK_DEBUG_MASK(DSDN30_S);
    CHECK_DEBUG_MASK(TPI_YUV);
    CHECK_DEBUG_MASK(TPI_ASYNC);
    #undef CHECK_DEBUG_SETTING

    mDebugDump = getPropertyValue(KEY_DEBUG_DUMP, VAL_DEBUG_DUMP);
    mDebugDumpCount = getPropertyValue(KEY_DEBUG_DUMP_COUNT, VAL_DEBUG_DUMP_COUNT);
    mDebugDumpByRecordNo = getPropertyValue(KEY_DEBUG_DUMP_BY_RECORDNO, VAL_DEBUG_DUMP_BY_RECORDNO);
    mForceIMG3O = getPropertyValue(KEY_FORCE_IMG3O, VAL_FORCE_IMG3O);
    mForceWarpPass = getPropertyValue(KEY_FORCE_WARP_PASS, VAL_FORCE_WARP_PASS);
    mForceGpuOut = getPropertyValue(KEY_FORCE_GPU_OUT, VAL_FORCE_GPU_OUT);
    mForceGpuRGBA = getPropertyValue(KEY_FORCE_GPU_RGBA, VAL_FORCE_GPU_RGBA);
    mUsePerFrameSetting = getPropertyValue(KEY_USE_PER_FRAME_SETTING, VAL_USE_PER_FRAME_SETTING);
    mForcePrintIO = getPropertyValue(KEY_FORCE_PRINT_IO, VAL_FORCE_PRINT_IO);
    mEnSyncCtrl = getPropertyValue(KEY_ENABLE_SYNC_CTRL, VAL_ENABLE_SYNC_CTRL);

    if( !mPipeUsage.support3DNR() )
    {
        DISABLE_3DNR(mForceOffMask);
        DISABLE_3DNR_S(mForceOffMask);
    }
    if( !mPipeUsage.supportDSDN20() )
    {
        DISABLE_DSDN20(mForceOffMask);
        DISABLE_DSDN20_S(mForceOffMask);
    }
    if( !mPipeUsage.supportDSDN25() )
    {
        DISABLE_DSDN25(mForceOffMask);
        DISABLE_DSDN25_S(mForceOffMask);
    }
    if( !mPipeUsage.supportDSDN30() )
    {
        DISABLE_DSDN30(mForceOffMask);
        DISABLE_DSDN30_S(mForceOffMask);
    }
    if( !mPipeUsage.supportEISNode() )
    {
        DISABLE_EIS(mForceOffMask);
    }
    if( !mPipeUsage.supportEISNode() || !mPipeUsage.supportEIS_Q() )
    {
        DISABLE_EIS_QUEUE(mForceOffMask);
    }
    if( !mPipeUsage.supportTPI(TPIOEntry::YUV) )
    {
        DISABLE_TPI_YUV(mForceOffMask);
    }
    if( !mPipeUsage.supportTPI(TPIOEntry::ASYNC) )
    {
        DISABLE_TPI_ASYNC(mForceOffMask);
    }

    MY_LOGD("forceOnMask=0x%04x, forceOffMask=0x%04x", mForceOnMask, ~mForceOffMask);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

DipUsageHint StreamingFeaturePipe::toDipUsageHint(const StreamingFeaturePipeUsage &sfpUsage) const
{
    DipUsageHint dipUsageHint;

    dipUsageHint.mSecTag = sfpUsage.isSecureP2();
    dipUsageHint.mMaxEnqueDepth = sfpUsage.getMaxP2EnqueDepth();
    if (sfpUsage.supportDSDN25() )
    {
        dipUsageHint.mMaxSubFrameNum =
            sfpUsage.getDSDNParam().mMaxDSLayer + sfpUsage.getNumScene();
    }
    else
    {
        dipUsageHint.mMaxSubFrameNum = sfpUsage.getNumScene();
    }

    if (sfpUsage.supportSMP2() )
    {
        dipUsageHint.mMaxSMVRBatchNum = sfpUsage.getSMVRSpeed() + 1;
    }

    return dipUsageHint;
}

MBOOL StreamingFeaturePipe::prepareGeneralPipe()
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    P2_CAM_TRACE_CALL(TRACE_DEFAULT);

    MBOOL needNormalStream = mPipeUsage.supportP2AP2() || mPipeUsage.supportSMP2() || mPipeUsage.supportDSDN25();

    if( needNormalStream )
    {
        MY_LOGI("create & init DIPStream ++");
        mDIPStream = Feature::P2Util::DIPStream::createInstance(mSensorIndex);
        DipUsageHint dipUsageHint = toDipUsageHint(mPipeUsage);
        if( mDIPStream != NULL )
        {
            ret = mDIPStream->init(NORMAL_STREAM_NAME, NSIoPipe::EStreamPipeID_Normal, dipUsageHint);
        }
        else
        {
            ret = MFALSE;
        }
        MY_LOGI("create & init DIPStream(%p) -- "
            "secTag(%d), #P2EnqueDepth(%d), #subFrame(%d), #SMVRBatchNum(%d)",
            mDIPStream,
            dipUsageHint.mSecTag,
            dipUsageHint.mMaxEnqueDepth,
            dipUsageHint.mMaxSubFrameNum,
            dipUsageHint.mMaxSMVRBatchNum);
    }

    if( mPipeUsage.supportP2AP2() )
    {
        mG_P2A.setDIPStream(mDIPStream);
    }
    else if( mPipeUsage.supportSMP2() )
    {
        mG_P2SM.setNormalStream(mDIPStream);
    }
    if( mPipeUsage.supportDSDN25() || mPipeUsage.supportDSDN30() )
    {
        mMSF.setDIPStream(mDIPStream);
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeaturePipe::prepareNodeSetting()
{
    TRACE_FUNC_ENTER();
    NODE_LIST::iterator it, end;
    for( it = mNodes.begin(), end = mNodes.end(); it != end; ++it )
    {
        (*it)->setSensorIndex(mSensorIndex);
        (*it)->setPipeUsage(mPipeUsage);
        (*it)->setNodeSignal(mNodeSignal);
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL StreamingFeaturePipe::prepareNodeConnection()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();

    StreamingFeatureNode *dispHelper = &mHelper;
    StreamingFeatureNode *p2gIn = NULL;
    StreamingFeatureNode *p2gSW = NULL;
    StreamingFeatureNode *p2Out = NULL;
    StreamingFeatureNode *p2gOut = NULL;

    if( mPipeUsage.supportTPI(TPIOEntry::DISP) &&
        !mPipeUsage.supportSMP2() )
    {
        dispHelper = &mDisp;
        this->connectData(ID_DISP_TO_HELPER, mDisp, mHelper);
    }

    if( mPipeUsage.supportDepthP2() )
    {
        this->connectData(ID_ROOT_TO_DEPTH, mRootNode, mDepth);
        this->connectData(ID_DEPTH_TO_HELPER, mDepth, mHelper);
        if( mPipeUsage.supportBokeh() )
        {
            this->connectData(ID_DEPTH_TO_BOKEH, mDepth, mBokeh, CONNECTION_SEQUENTIAL);
            this->connectData(ID_DEPTH_P2_TO_BOKEH, mDepth, mBokeh, CONNECTION_SEQUENTIAL);
            this->connectData(ID_BOKEH_TO_HELPER, mBokeh, *dispHelper);
        }
    }
    else if( mPipeUsage.supportSMP2() )
    {
        p2gIn = p2gSW = p2Out = &TO_P2SM();
        this->connectData(ID_ROOT_TO_P2G, mRootNode, *p2gIn);
    }
    else
    {
        p2gIn = p2gSW = p2Out = &TO_P2A();
        this->connectData(ID_ROOT_TO_P2G, mRootNode, *p2gIn);
    }

    if( (mPipeUsage.supportDSDN25() || mPipeUsage.supportDSDN30()) && p2gIn )
    {
        p2gSW = &mP2SW;
        p2Out = &mMSF;
        this->connectData(ID_P2G_TO_MSS, *p2gIn, mMSS);
        this->connectData(ID_P2G_TO_MSF, *p2gIn, mMSF);
        this->connectData(ID_P2G_TO_P2SW, *p2gIn, mP2SW);
        this->connectData(ID_MSS_TO_MSF, mMSS, mMSF, CONNECTION_SEQUENTIAL);
        this->connectData(ID_P2SW_TO_MSF, mP2SW, mMSF);
        if( mPipeUsage.supportDSDN30() )
        {
            this->connectData(ID_P2G_TO_PMSS, *p2gIn, mPMSS);
            this->connectData(ID_PMSS_TO_MSF, mPMSS, mMSF, CONNECTION_SEQUENTIAL);
        }
    }

    if( p2Out )
    {
        p2gOut = &mP2AMDP;
        this->connectData(ID_P2G_TO_PMDP, *p2Out, *p2gOut);
        this->connectData(ID_P2G_TO_HELPER, *p2gOut, *dispHelper);
        //this->connectData(ID_PMDP_TO_HELPER, mP2AMDP, *dispHelper);
    }

    if( mPipeUsage.supportP2GWithDepth() )
    {
        this->connectData(ID_ROOT_TO_DEPTH, mRootNode, mDepth);
        this->connectData(ID_DEPTH_TO_HELPER, mDepth, mHelper);
        if( mPipeUsage.supportBokeh() && p2gOut )
        {
            this->connectData(ID_P2G_TO_BOKEH, *p2gOut, mBokeh);
            this->connectData(ID_DEPTH_TO_BOKEH, mDepth, mBokeh, CONNECTION_SEQUENTIAL);
            this->connectData(ID_BOKEH_TO_HELPER, mBokeh, *dispHelper);
        }
    }

    if( mPipeUsage.supportDSDN20() && p2gOut )
    {
        this->connectData(ID_P2G_TO_VNR, *p2gOut, mVNR, CONNECTION_SEQUENTIAL);
        if( mPipeUsage.supportTPI(TPIOEntry::YUV) && mTPIs.size() )
        {
            this->connectData(ID_VNR_TO_NEXT_FULLIMG, mVNR, mTPIs[0]);
        }
        else if( mPipeUsage.supportWarpNode() && !mPipeUsage.supportVMDPNode() )
        {
            if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  this->connectData(ID_VNR_TO_NEXT_P, mVNR, mWarpP);
            if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   this->connectData(ID_VNR_TO_NEXT_R, mVNR, mWarpR);
        }
        else
        {
            this->connectData(ID_VNR_TO_NEXT_FULLIMG, mVNR, mVmdpP);
        }
    }

    if( mPipeUsage.supportWarpNode() && p2gOut )
    {
        if( mPipeUsage.supportBokeh() )
        {
            if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  this->connectData(ID_BOKEH_TO_WARP_P_FULLIMG, mBokeh, mWarpP);
            if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   this->connectData(ID_BOKEH_TO_WARP_R_FULLIMG, mBokeh, mWarpR);
        }
        else
        {
            if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  this->connectData(ID_P2G_TO_WARP_P, *p2gOut, mWarpP);
            if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   this->connectData(ID_P2G_TO_WARP_R, *p2gOut, mWarpR);
        }
        if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )
        {
            this->connectData(ID_WARP_TO_HELPER, mWarpP, *dispHelper);
        }
        if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )
        {
            this->connectData(ID_WARP_TO_HELPER, mWarpR, *dispHelper, CONNECTION_SEQUENTIAL);
        }
    }

    // EIS nodes
    if (mPipeUsage.supportEISNode())
    {
        this->connectData(ID_ROOT_TO_EIS, mRootNode, mEIS);
        if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )
        {
            this->connectData(ID_EIS_TO_WARP_P, mEIS, mWarpP, CONNECTION_SEQUENTIAL);
        }
        if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )
        {
            this->connectData(ID_EIS_TO_WARP_R, mEIS, mWarpR, CONNECTION_SEQUENTIAL);
        }
        if( mPipeUsage.supportRSCNode() )
        {
            this->connectData(ID_RSC_TO_EIS, mRSC, mEIS, CONNECTION_SEQUENTIAL);
        }
    }

    if( mPipeUsage.supportRSCNode() )
    {
        this->connectData(ID_ROOT_TO_RSC, mRootNode, mRSC);
        this->connectData(ID_RSC_TO_HELPER, mRSC, mHelper, CONNECTION_SEQUENTIAL);
        if( p2gSW )
        {
            this->connectData(ID_RSC_TO_P2G, mRSC, *p2gSW, CONNECTION_SEQUENTIAL);
        }
    }

    if( mPipeUsage.supportTrackingFocus() )
    {
        this->connectData(ID_ROOT_TO_TRACKING, mRootNode, mTracking);
        this->connectData(ID_TRACKING_TO_HELPER, mTracking, mHelper, CONNECTION_SEQUENTIAL);
    }

    if( mPipeUsage.supportTPI(TPIOEntry::YUV) && mTPIs.size() )
    {
        StreamingFeatureNode *tpi = mTPIs[0];
        if( mPipeUsage.supportDepthP2() )
        {
            if( mPipeUsage.supportBokeh() )
            {
                this->connectData(ID_BOKEH_TO_VENDOR_FULLIMG, mBokeh, *tpi);
            }
            else
            {
                this->connectData(ID_DEPTH_TO_VENDOR, mDepth, *tpi, CONNECTION_SEQUENTIAL);
                this->connectData(ID_DEPTH_P2_TO_VENDOR, mDepth, *tpi, CONNECTION_SEQUENTIAL);
            }
        }
        else if( p2gOut )
        {
            if( mPipeUsage.supportBokeh() )
            {
                this->connectData(ID_BOKEH_TO_VENDOR_FULLIMG, mBokeh, *tpi);
            }
            else
            {
                this->connectData(ID_P2G_TO_VENDOR_FULLIMG, *p2gOut, *tpi);
            }
        }

        if( mPipeUsage.supportP2GWithDepth() )
        {
            this->connectData(ID_DEPTH_TO_VENDOR, mDepth, *tpi, CONNECTION_SEQUENTIAL);
        }

        unsigned n = mTPIs.size();
        for( unsigned i = 1; i < n; ++i )
        {
            this->connectData(ID_TPI_TO_NEXT, *mTPIs[i-1], *mTPIs[i]);
        }
        if( mPipeUsage.supportVMDPNode(SFN_HINT::PREVIEW) )
        {
            this->connectData(ID_TPI_TO_NEXT, *mTPIs[n-1], mVmdpP, CONNECTION_SEQUENTIAL);
        }
        if( mPipeUsage.supportVMDPNode(SFN_HINT::RECORD) )
        {
            this->connectData(ID_TPI_TO_NEXT_R, *mTPIs[n-1], mVmdpR, CONNECTION_SEQUENTIAL);
        }
    }

    if( mPipeUsage.supportXNode() )
    {
        if( mPipeUsage.supportBokeh() )
        {
            this->connectData(ID_BOKEH_TO_XNODE, mBokeh, mXNode);
        }
        else if( p2gOut )
        {
            this->connectData(ID_P2G_TO_XNODE, *p2gOut, mXNode);
        }
        this->connectData(ID_XNODE_TO_HELPER, mXNode, mHelper);
    }

    if( mPipeUsage.supportVMDPNode() )
    {
        if( mPipeUsage.supportVMDPNode(SFN_HINT::PREVIEW) )
        {
            this->connectData(ID_VMDP_TO_HELPER, mVmdpP, *dispHelper);
            // for TWIN mode
            if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  this->connectData(ID_VMDP_TO_NEXT_P, mVmdpP, mWarpP);
            if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   this->connectData(ID_VMDP_TO_NEXT_R, mVmdpP, mWarpR);
            // for warp UNI : preview (photo preview)
            if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  this->connectData(ID_VMDP_TO_NEXT, mVmdpP, mWarpP);
            // for warp UNI : record (early display)
            if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   this->connectData(ID_VMDP_TO_NEXT, mVmdpP, mWarpR);
            // XNode
            if( mPipeUsage.supportXNode() )                      this->connectData(ID_VMDP_TO_XNODE, mVmdpP, mXNode);
        }
        if( mPipeUsage.supportVMDPNode(SFN_HINT::RECORD) )
        {
            this->connectData(ID_VMDP_TO_HELPER, mVmdpR, *dispHelper);
        }
    }

    if( mPipeUsage.supportTPI(TPIOEntry::ASYNC) )
    {
        this->connectData(ID_HELPER_TO_ASYNC, mHelper, mAsync);
        this->connectData(ID_ASYNC_TO_HELPER, mAsync, mHelper);
    }

    this->setRootNode(&mRootNode);
    mRootNode.registerInputDataID(ID_ROOT_ENQUE);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL StreamingFeaturePipe::prepareIOControl()
{
    TRACE_FUNC_ENTER();

    StreamingFeatureNode *rootN = &TO_P2A();
    if( mPipeUsage.supportDepthP2() )
    {
        rootN = &mDepth;
    }
    else if( mPipeUsage.supportSMP2() )
    {
        rootN = &TO_P2SM();
    }

    mRecordPath.push_back(rootN);
    mDisplayPath.push_back(rootN);
    mPhysicalPath.push_back(rootN);
    mPreviewCallbackPath.push_back(rootN);

    if( mPipeUsage.supportBokeh() )
    {
        mRecordPath.push_back(&mBokeh);
        mDisplayPath.push_back(&mBokeh);
        mPreviewCallbackPath.push_back(&mBokeh);
    }
    if( mPipeUsage.supportDSDN20() )
    {
        mRecordPath.push_back(&mVNR);
        mDisplayPath.push_back(&mVNR);
        mPreviewCallbackPath.push_back(&mVNR);
        if( mPipeUsage.supportPhysicalDSDN20() )
        {
            mPhysicalPath.push_back(&mVNR);
        }
    }
    if( mPipeUsage.supportTPI(TPIOEntry::YUV) && mTPIs.size() )
    {
        mRecordPath.push_back(mTPIs[0]);
        mDisplayPath.push_back(mTPIs[0]);
        mPreviewCallbackPath.push_back(mTPIs[0]);
    }
    if( mPipeUsage.supportVMDPNode() )
    {
        mDisplayPath.push_back(&mVmdpP);
        mPreviewCallbackPath.push_back(&mVmdpP);
        mRecordPath.push_back(mPipeUsage.supportVMDPNode(SFN_HINT::RECORD) ? &mVmdpR : &mVmdpP);
        if( mPipeUsage.supportPhysicalDSDN20() )
        {
            mPhysicalPath.push_back(&mVmdpP);
        }
    }
    if( mPipeUsage.supportWarpNode() )
    {
        mRecordPath.push_back(&mWarpR);
        if (mPipeUsage.supportPreviewEIS())
        {
            mDisplayPath.push_back(&mWarpP);
            mPreviewCallbackPath.push_back(&mWarpP);
        }
    }
    if( mPipeUsage.supportTPI(TPIOEntry::ASYNC) )
    {
        mAsyncPath.push_back(&mAsync);
    }
    if( mPipeUsage.supportXNode() )
    {
        mDisplayPath.push_back(&mXNode);
        mRecordPath.push_back(&mXNode);
        mPreviewCallbackPath.push_back(&mXNode);
        mPhysicalPath.push_back(&mXNode);
    }

    mIOControl.setRoot(rootN);
    mIOControl.addStream(STREAMTYPE_PREVIEW, mDisplayPath);
    mIOControl.addStream(STREAMTYPE_RECORD, mRecordPath);
    mIOControl.addStream(STREAMTYPE_PREVIEW_CALLBACK, mPreviewCallbackPath);
    mIOControl.addStream(STREAMTYPE_PHYSICAL, mPhysicalPath);
    mIOControl.addStream(STREAMTYPE_ASYNC, mAsyncPath);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL StreamingFeaturePipe::prepareBuffer()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();

    MSize fullSize(MAX_FULL_WIDTH, MAX_FULL_HEIGHT);
    MSize streamingSize = mPipeUsage.getStreamingSize();

    if( streamingSize.w > 0 && streamingSize.h > 0 )
    {
        fullSize.w = align(streamingSize.w, BUF_ALLOC_ALIGNMENT_BIT);
        fullSize.h = align(streamingSize.h, BUF_ALLOC_ALIGNMENT_BIT);
    }

    MY_LOGD("sensor(%d) StreamingSize=(%dx%d) align64=(%dx%d)", mSensorIndex, streamingSize.w, streamingSize.h, fullSize.w, fullSize.h);

    if( mPipeUsage.supportP2AFeature() || mPipeUsage.supportDepthP2() )
    {
        if( mPipeUsage.supportDepthP2() )
        {
            mDepthYuvOutPool = createFullImgPool("fpipe.depthOutImg", fullSize);
        }
        else
        {
            mFullImgPool = createFullImgPool("fpipe.fullImg", fullSize);
        }

        if( mPipeUsage.supportBokeh() )
        {
            mBokehOutPool = createFullImgPool("fpipe.bokehOutImg", fullSize);
        }
    }

    if( mPipeUsage.supportWarpNode() )
    {
        MUINT32 eis_factor = mPipeUsage.getEISFactor();

        MUINT32 modifyW = fullSize.w;
        MUINT32 modifyH = fullSize.h;

        {
            modifyW = fullSize.w*100.0f/eis_factor;
            modifyH = fullSize.h*100.0f/eis_factor;
        }

        modifyW = align(modifyW, BUF_ALLOC_ALIGNMENT_BIT);
        modifyH = align(modifyH, BUF_ALLOC_ALIGNMENT_BIT);

        mWarpOutputPool = createWarpOutputPool("fpipe.warpOut", MSize(modifyW, modifyH));
        mEisFullImgPool = createFullImgPool("fpipe.eisFull", fullSize);
    }
    if( mPipeUsage.supportDepthP2() )
    {
        mDepth.setOutputBufferPool(mDepthYuvOutPool, mPipeUsage.getNumDepthImgBuffer());
        if(mPipeUsage.supportBokeh())
        {
            mBokeh.setOutputBufferPool(mBokehOutPool, mPipeUsage.getNumBokehOutBuffer());
        }
    }
    else
    {
        mG_P2A.setFullImgPool(mFullImgPool, mPipeUsage.getNumP2ABuffer());
    }

    if( mPipeUsage.supportP2GWithDepth() )
    {
        mDepth.setOutputBufferPool(mDepthYuvOutPool, mPipeUsage.getNumDepthImgBuffer());
        if(mPipeUsage.supportBokeh())
        {
            mBokeh.setOutputBufferPool(mBokehOutPool, mPipeUsage.getNumBokehOutBuffer());
        }
    }

    if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )
    {
        mWarpP.setInputBufferPool(mEisFullImgPool);
        mWarpP.setOutputBufferPool(mWarpOutputPool);
    }
    if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )
    {
        mWarpR.setInputBufferPool(mEisFullImgPool);
        mWarpR.setOutputBufferPool(mWarpOutputPool);
    }

    if( mPipeUsage.supportTPI(TPIOEntry::YUV) )
    {
        TPIUsage tpiUsage = mPipeUsage.getTPIUsage();
        EImageFormat fmt = tpiUsage.getCustomFormat(TPIOEntry::YUV, mPipeUsage.getFullImgFormat());
        MSize size = tpiUsage.getCustomSize(TPIOEntry::YUV, mPipeUsage.getStreamingSize());
        size = align(size, BUF_ALLOC_ALIGNMENT_BIT);
        mTPIBufferPool = GraphicBufferPool::create("fpipe.tpi", size, fmt, GraphicBufferPool::USAGE_HW_TEXTURE);
        mTPISharedBufferPool = SharedBufferPool::create("fpipe.tpi.shared", mTPIBufferPool);
        for( TPINode *tpi : mTPIs )
        {
            if( tpi )
            {
                tpi->setSharedBufferPool(mTPISharedBufferPool);
            }
        }
    }

    if( mPipeUsage.supportVNRNode() )
    {
        EImageFormat format = mPipeUsage.getVNRImgFormat();
        MSize size = mPipeUsage.getStreamingSize();

        if( mPipeUsage.supportTPI(TPIOEntry::YUV) && mTPIBufferPool != NULL )
        {
            format = mTPIBufferPool->getImageFormat();
            size = mTPIBufferPool->getImageSize();
        }
        else if( mPipeUsage.supportWarpNode() && mEisFullImgPool != NULL )
        {
            format = mEisFullImgPool->getImageFormat();
            size = mEisFullImgPool->getImageSize();
        }
        size = align(size, BUF_ALLOC_ALIGNMENT_BIT);

        mVNRInputPool = GraphicBufferPool::create("fpipe.vnr.in", size, format, ImageBufferPool::USAGE_HW);
        mVNROutputPool = GraphicBufferPool::create("fpipe.vnr.out", size, format, ImageBufferPool::USAGE_HW);

        mVNR.setInputBufferPool(mVNRInputPool);
        mVNR.setOutputBufferPool(mVNROutputPool);
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL StreamingFeaturePipe::prepareSyncCtrl()
{
    TRACE_FUNC_ENTER();
    if( mEnSyncCtrl && mPipeUsage.supportWarpNode(SFN_HINT::TWIN) )
    {
        std::shared_ptr<ISyncCtrl> warpSyncCtrlRwaitP = std::make_shared<SyncCtrlRwaitP>();
        mWarpR.setSyncCtrlWait(warpSyncCtrlRwaitP);
        mWarpP.setSyncCtrlSignal(warpSyncCtrlRwaitP);
        std::shared_ptr<ISyncCtrl> warpSyncCtrlPwaitR = std::make_shared<SyncCtrlPwaitR>();
        mWarpP.setSyncCtrlWait(warpSyncCtrlPwaitR);
        mWarpR.setSyncCtrlSignal(warpSyncCtrlPwaitR);
    }
    if( mEnSyncCtrl && mPipeUsage.supportVMDPNode(SFN_HINT::TWIN) )
    {
        std::shared_ptr<ISyncCtrl> vmdpSyncCtrlRwaitP = std::make_shared<SyncCtrlRwaitP>();
        mVmdpR.setSyncCtrlWait(vmdpSyncCtrlRwaitP);
        mVmdpP.setSyncCtrlSignal(vmdpSyncCtrlRwaitP);
        std::shared_ptr<ISyncCtrl> vmdpSyncCtrlPwaitR = std::make_shared<SyncCtrlPwaitR>();
        mVmdpP.setSyncCtrlWait(vmdpSyncCtrlPwaitR);
        mVmdpR.setSyncCtrlSignal(vmdpSyncCtrlPwaitR);
    }
    if( mPipeUsage.supportDSDN30() )
    {
        mMSSSync = std::make_shared<FrameControl>();
        mMSS.setMSSNotifyCtrl(mMSSSync);
        mPMSS.setMSSWaitCtrl(mMSSSync);
        mPMSSSync = std::make_shared<FrameControl>();
        mPMSS.setMSSNotifyCtrl(mPMSSSync);
        mMSS.setMSSWaitCtrl(mPMSSSync);

        mMotionSync = std::make_shared<FrameControl>();
        mPMSS.setMotionWaitCtrl(mMotionSync);
        mP2SW.setMotionNotifyCtrl(mMotionSync);
        mP2L1Sync = std::make_shared<FrameControl>();
        mPMSS.setLastP2L1WaitCtrl(mP2L1Sync);
        mMSF.setLastP2L1NotifyCtrl(mP2L1Sync);
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

android::sp<IBufferPool> StreamingFeaturePipe::createFullImgPool(const char* name, MSize size)
{
    TRACE_FUNC_ENTER();

    android::sp<IBufferPool> fullImgPool;
    EImageFormat format = mPipeUsage.getFullImgFormat();

    if (mPipeUsage.isSecureP2())
    {
        fullImgPool =  SecureBufferPool::create(name, size, format, ImageBufferPool::USAGE_HW);
    }
    else if( mPipeUsage.supportGraphicBuffer() && isGrallocFormat(format) )
    {
        NativeBufferWrapper::ColorSpace color;
        color = mPipeUsage.supportEISNode() ?
                NativeBufferWrapper::YUV_BT601_FULL :
                NativeBufferWrapper::NOT_SET;

        fullImgPool = GraphicBufferPool::create(name, size, format, GraphicBufferPool::USAGE_HW_TEXTURE, color);
    }
    else
    {
        fullImgPool = ImageBufferPool::create(name, size, format, ImageBufferPool::USAGE_HW);
    }
    MY_LOGD("sensor(%d) size=(%dx%d) format(%d) %s", mSensorIndex , size.w, size.h, format,
            mPipeUsage.isSecureP2() ? "SecureBuffer" :
                (mPipeUsage.supportGraphicBuffer() ? "GraphicBuffer" : "ImageBuffer") );

    TRACE_FUNC_EXIT();

    return fullImgPool;
}

android::sp<IBufferPool> StreamingFeaturePipe::createImgPool(const char* name, MSize size, EImageFormat fmt)
{
    TRACE_FUNC_ENTER();

    android::sp<IBufferPool> pool;
    if( mPipeUsage.supportGraphicBuffer() )
    {
        pool = GraphicBufferPool::create(name, size, fmt, GraphicBufferPool::USAGE_HW_TEXTURE);
    }
    else
    {
        pool = ImageBufferPool::create(name, size, fmt, ImageBufferPool::USAGE_HW );
    }

    TRACE_FUNC_EXIT();
    return pool;
}

android::sp<IBufferPool> StreamingFeaturePipe::createWarpOutputPool(const char* name, MSize size)
{
    TRACE_FUNC_ENTER();

    android::sp<IBufferPool> warpOutputPool;

    EImageFormat format = mPipeUsage.getFullImgFormat();

    if( mPipeUsage.supportGraphicBuffer() )
    {
        EImageFormat gbFmt = (!mPipeUsage.supportWPE() && mForceGpuRGBA ) ? eImgFmt_RGBA8888 : format;
        warpOutputPool = GraphicBufferPool::create(name, size, gbFmt, GraphicBufferPool::USAGE_HW_RENDER);
    }
    else
    {
        warpOutputPool = ImageBufferPool::create(name, size, format, ImageBufferPool::USAGE_HW);
    }

    MY_LOGD("sensor(%d) %s size=(%dx%d) format(%d) %s", mSensorIndex, mPipeUsage.supportWPE() ? "WPE" : "GPU" ,
            size.w, size.h, format, mPipeUsage.supportGraphicBuffer() ? "GraphicBuffer" : "ImageBuffer");

    TRACE_FUNC_EXIT();

    return warpOutputPool;
}

MVOID StreamingFeaturePipe::releaseNodeSetting()
{
    TRACE_FUNC_ENTER();
    this->disconnect();
    mDisplayPath.clear();
    mRecordPath.clear();
    mPhysicalPath.clear();
    mPreviewCallbackPath.clear();
    mAsyncPath.clear();
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::releaseGeneralPipe()
{
    P2_CAM_TRACE_CALL(TRACE_DEFAULT);
    TRACE_FUNC_ENTER();
    mG_P2A.setDIPStream(NULL);
    mG_P2SM.setNormalStream(NULL);
    if( mDIPStream )
    {
        mDIPStream->uninit(NORMAL_STREAM_NAME);
        mDIPStream->destroyInstance();
        mDIPStream = NULL;
    }
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::releaseBuffer()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();

    mG_P2A.setFullImgPool(NULL);
    mWarpP.setInputBufferPool(NULL);
    mWarpP.setOutputBufferPool(NULL);
    mWarpR.setInputBufferPool(NULL);
    mWarpR.setOutputBufferPool(NULL);

    IBufferPool::destroy(mFullImgPool);
    IBufferPool::destroy(mDepthYuvOutPool);
    IBufferPool::destroy(mBokehOutPool);
    IBufferPool::destroy(mEisFullImgPool);
    IBufferPool::destroy(mWarpOutputPool);
    SharedBufferPool::destroy(mTPISharedBufferPool);
    IBufferPool::destroy(mTPIBufferPool);
    IBufferPool::destroy(mVNRInputPool);
    IBufferPool::destroy(mVNROutputPool);

    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::applyMaskOverride(const RequestPtr &request)
{
    TRACE_FUNC_ENTER();
    request->mFeatureMask |= mForceOnMask;
    request->mFeatureMask &= mForceOffMask;
    request->setDumpProp(mDebugDump, mDebugDumpCount, mDebugDumpByRecordNo);
    request->setForceIMG3O(mForceIMG3O);
    request->setForceWarpPass(mForceWarpPass);
    request->setForceGpuOut(mForceGpuOut);
    request->setForceGpuRGBA(mForceGpuRGBA);
    request->setForcePrintIO(mForcePrintIO);
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::applyVarMapOverride(const RequestPtr &request)
{
    TRACE_FUNC_ENTER();
    (void)(request);
    TRACE_FUNC_EXIT();
}

MVOID StreamingFeaturePipe::prepareFeatureRequest(const FeaturePipeParam &param)
{
    ++mCounter;
    eAppMode appMode = param.getVar<eAppMode>(SFP_VAR::APP_MODE, APP_PHOTO_PREVIEW);
    if( appMode == APP_VIDEO_RECORD ||
        appMode == APP_VIDEO_STOP )
    {
        ++mRecordCounter;
    }
    else if( mRecordCounter )
    {
        MY_LOGI("Set Record Counter %d=>0. AppMode=%d", mRecordCounter, appMode);
        mRecordCounter = 0;
    }
    this->prepareEISQControl(param);
    TRACE_FUNC("Request=%d, Record=%d, AppMode=%d", mCounter, mRecordCounter, appMode);
}

MVOID StreamingFeaturePipe::prepareEISQControl(const FeaturePipeParam &param)
{
    EISQActionInfo info;
    info.mAppMode = param.getVar<eAppMode>(SFP_VAR::APP_MODE, APP_PHOTO_PREVIEW);
    info.mRecordCount = mRecordCounter;
    info.mIsAppEIS = HAS_EIS(param.mFeatureMask);
    info.mIsReady = existOutBuffer(param.mSFPIOManager.getGeneralIOs(), IO_TYPE_RECORD);
    mEISQControl.update(info);

    TRACE_FUNC("AppMode=%d, Record=%d, AppEIS=%d, IsRecordBuffer=%d",
               info.mAppMode, info.mRecordCount, info.mIsAppEIS, info.mIsReady);
}

MVOID StreamingFeaturePipe::prepareIORequest(const RequestPtr &request)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    {
        LogString dumpStr(2048);
        dumpStr.append("%s master/slave/fd(%d/%d/%d) ReqNo(%d), feature=0x%04x(%s), cycle(%d), fps(n/a)=(%d/%d), ZoomROI(" MCropF_STR ") SFPIOMgr:",
                request->mLog.getLogStr(), request->getMasterID(), request->mSlaveID, request->mFDSensorID, request->mRequestNo,
                request->mFeatureMask, request->getFeatureMaskName(), request->getNodeCycleTimeMs(), request->getNodeFPS(), request->getAppFPS(), MCropF_ARG(request->getZoomROI()));
        request->mSFPIOManager.appendDumpInfo(dumpStr);
        MY_SAFE_LOGD(dumpStr.c_str(), dumpStr.length());
    }

    if( mPipeUsage.useScenarioRecorder() )
    {
        Feature::P2Util::RecorderWrapper::print(request->mP2Pack, MOD_FPIPE_STREAMING, true,
            "Trigger StreamingFeature(%s)", request->getFeatureMaskName());
    }
    std::set<StreamType> generalStreams;
    if( request->hasDisplayOutput() )
    {
        generalStreams.insert(STREAMTYPE_PREVIEW);
    }
    if( request->hasRecordOutput() )
    {
        generalStreams.insert(STREAMTYPE_RECORD);
    }
    if( request->hasExtraOutput() )
    {
        generalStreams.insert(STREAMTYPE_PREVIEW_CALLBACK);
    }
    if( request->needTPIAsync() )
    {
        generalStreams.insert(STREAMTYPE_ASYNC);
    }

    std::map<MUINT32, NextIO::Control::StreamSet> streamsMap;
    streamsMap[request->mMasterID] = generalStreams;
    if( request->hasPhysicalOutput(request->mMasterID) )
    {
        streamsMap[request->mMasterID].insert(STREAMTYPE_PHYSICAL);
    }
    // Slave
    if(request->hasSlave(request->mSlaveID))
    {
        streamsMap[request->mSlaveID] = generalStreams;
        if( request->hasPhysicalOutput(request->mSlaveID) )
        {
            streamsMap[request->mSlaveID].insert(STREAMTYPE_PHYSICAL);
        }
    }

    request->mTimer.startPrepareIO();
    NextIO::ReqInfo reqInfo = request->extractReqInfo();
    mIOControl.prepareMap(streamsMap, reqInfo, request->mIORequestMap);
    if( request->needPrintIO() )
    {
        NextIO::print("IOUtil ReqInfo: ", reqInfo);
        mIOControl.dump(request->mIORequestMap);
    }
    request->mTimer.stopPrepareIO();

    TRACE_FUNC_EXIT();
}

} // NSFeaturePipe
} // NSCamFeature
} // NSCam
