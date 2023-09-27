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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#include "G_P2ANode.h"
#include <mtkcam/drv/iopipe/CamIO/IHalCamIO.h>
#include <mtkcam3/feature/eis/eis_hal.h>
#include "NextIOUtil.h"

#define PIPE_CLASS_TAG "P2ANode"
#define PIPE_TRACE TRACE_G_P2A_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_P2A);

using namespace NSCam::NSIoPipe;
using namespace NSCam::Feature::P2Util;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

namespace G {

P2ANode::P2ANode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_P2A)
    , StreamingFeatureNode(name)
    , mp3A(NULL)
    , mDIPStream(NULL)
    , mEisMode(0)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mRequests);
    mForceExpectMS = getPropertyValue("vendor.debug.fpipe.p2a.expect", 0);
    TRACE_FUNC_EXIT();
}

P2ANode::~P2ANode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::setDIPStream(Feature::P2Util::DIPStream *stream)
{
    TRACE_FUNC_ENTER();
    mDIPStream = stream;
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::setFullImgPool(const android::sp<IBufferPool> &pool, MUINT32 allocate)
{
    (void)pool;
    (void)allocate;
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL P2ANode::onInit()
{
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "P2A:onInit");
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();

    mOneP2G = mP2GMgr->isOneP2G();
    mNeedDreTun = mP2GMgr->isSupportDRETun();
    mEisMode = mPipeUsage.getEISMode();
    mSupportPhyNR = mPipeUsage.supportPhysicalNR();
    mSupportPhyDSDN20 = mPipeUsage.supportPhysicalDSDN20();

    if( mOneP2G && mPipeUsage.support3DNRRSC() )
    {
        mWaitRSC = MTRUE;
        this->addWaitQueue(&mRSCDatas);
        MY_LOGD("P2A add RSC waitQueue for 3DNR");
    }

    if( mOneP2G && mPipeUsage.support3DNRLMV() )
    {
        mUseLMV = MTRUE;
    }

    std::vector<MUINT32> sIDs = mPipeUsage.getAllSensorIDs();
    for( P2G::LoopScene scene : P2G::sAllLoopScene )
    {
        P2G::Feature feature;
        if( mPipeUsage.support3DNR() )
        {
            feature += P2G::Feature::NR3D;
        }
        if( scene == P2G::LoopScene::GEN || scene == P2G::LoopScene::PHY )
        {
            if( mPipeUsage.supportDSDN30() ) feature += P2G::Feature::DSDN30;
            if( mPipeUsage.supportDSDN25() ) feature += P2G::Feature::DSDN25;
            if( mPipeUsage.supportDSDN20() ) feature += P2G::Feature::DSDN20;
            if( mNeedDreTun ) feature += P2G::Feature::DRE_TUN;
        }
        for( MUINT32 sID : sIDs )
        {
            mLoopData[scene][sID] = mP2GMgr->makeInitLoopData(feature);
        }
    }

    TRACE_FUNC_EXIT();
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    return (mP2GMgr != NULL);
}

MBOOL P2ANode::onUninit()
{
    TRACE_FUNC_ENTER();
    mLoopData.clear();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onThreadStart()
{
    P2_CAM_TRACE_CALL(TRACE_DEFAULT);
    TRACE_FUNC_ENTER();

    mP2GMgr->allocateP2GInBuffer();
    mP2GMgr->allocateP2GOutBuffer();
    if( mOneP2G )
    {
        mP2GMgr->allocateP2SWBuffer();
        mP2GMgr->allocateP2HWBuffer();
        mP2GMgr->allocateDsBuffer();
    }

    init3A();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    this->waitDIPStreamBaseDone();
    uninit3A();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onData(DataID id, const RequestPtr &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    switch( id )
    {
    case ID_ROOT_TO_P2G:
        mRequests.enque(data);
        ret = MTRUE;
        break;
    default:
        ret = MFALSE;
        break;
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL P2ANode::onData(DataID id, const RSCData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s  rsc_data arrived, waitRSC(%d)", data.mRequest->mRequestNo, ID2Name(id), mWaitRSC);
    MBOOL ret = MFALSE;

    if( mWaitRSC && (id == ID_RSC_TO_P2G) )
    {
        mRSCDatas.enque(data);
        ret = MTRUE;
    }

    TRACE_FUNC_EXIT();
    return ret;
}

NextIO::Policy P2ANode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    NextIO::Policy policy;

    struct table
    {
        MUINT32                 sensorID;
        MBOOL                   run;
        MBOOL                   loop;
    } myTable[] =
    {
        { reqInfo.mMasterID, reqInfo.hasMaster(), HAS_3DNR(reqInfo.mFeatureMask) },
        { reqInfo.mSlaveID,  reqInfo.hasSlave(),  HAS_3DNR_S(reqInfo.mFeatureMask) },
    };

    for( const table &t : myTable )
    {
        if( t.run )
        {
            policy.enableAll(t.sensorID);
            if( t.loop )
            {
                policy.setLoopBack(MTRUE, t.sensorID);
            }
        }
    }
    return policy;
}

MBOOL P2ANode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr request;
    RSCData rscData;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mRequests.deque(request) )
    {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    }
    else if( request == NULL )
    {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    if( mWaitRSC )
    {
        if( !mRSCDatas.deque(rscData) )
        {
            MY_LOGE("RSCData deque out of sync");
            return MFALSE;
        }
        if( request != rscData.mRequest )
        {
            MY_LOGE("P2A_RSCData out of sync request(%d) rsc(%d)", request->mRequestNo, rscData.mRequest->mRequestNo);
            return MFALSE;
        }
    }

    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);

    request->mTimer.startP2A();
    markNodeEnter(request);
    processP2A(request, rscData.mData);
    request->mTimer.stopP2A();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

static MRectF applyViewRatio(const MRectF &src, const MSize &size)
{
    TRACE_FUNC_ENTER();
    MRectF view = src;
    if( size.w && size.h )
    {
        if( src.s.w * size.h > size.w * src.s.h )
        {
            view.s.w = (src.s.h * size.w / size.h);
            view.p.x += (src.s.w - view.s.w) / 2;
        }
        else
        {
            view.s.h = (src.s.w * size.h / size.w);
            view.p.y += (src.s.h - view.s.h) / 2;
        }
    }
    TRACE_FUNC_EXIT();
    return view;
}

MBOOL P2ANode::processP2A(const RequestPtr &request, const RSCResult &rsc)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);

    TRACE_FUNC_ENTER();
    IOOut outM, outS;
    IOIn inM, inS;
    std::vector<P2G::IO> groupIO;
    P2G::Path path;

    MBOOL runSlave = request->isSlaveParamValid();
    P2AEnqueData data(request);
    data.mHW.mEarlyDisplay = request->needP2AEarlyDisplay() && request->needDisplayOutput(this);

    prepareFull(request, MTRUE, request->mMasterID, data.mHW.mOutM, outM);
    prepareFull(request, runSlave, request->mSlaveID, data.mHW.mOutS, outS);
    prepareFeatureData(request, data, inM, inS, outM, outS, runSlave);
    prepareDS(request, request->mMasterID, data.mDSDNSizes, data.mHW.mOutM, outM);
    prepareDS(request, request->mSlaveID, data.mSlaveDSDNSizes, data.mHW.mOutS, outS);
    prepareGroupIO(request, data, inM, inS, outM, outS, groupIO);
    path = mP2GMgr->calcPath(groupIO, request->mRequestNo);

    if( mOneP2G )
    {
        updateMotion(request, data, rsc);
        if ( runSlave )
            updateMotion(request, data, rsc, MTRUE);

        runP2SW(request, path.mP2SW);
        runP2HW(request, data, path.mP2HW, path.mPMDP);
    }
    else
    {
        handleP2GData(request, data, path);
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2ANode::prepareFeatureData(const RequestPtr &request, P2AEnqueData &data, IOIn &inM, IOIn &inS, IOOut &outM, IOOut &outS, MBOOL runSlave)
{
    TRACE_FUNC_ENTER();
    if( request->need3DNR(request->mMasterID) )
    {
        data.mSW.mMotion = mP2GMgr->makeMotion();
    }
    if ( request->need3DNR(request->mSlaveID) )
    {
        data.mSW.mSlaveMotion = mP2GMgr->makeMotion();
    }

    MSize nextSize(0, 0);
    BasicImg nextM = data.mHW.mOutM.getNext();
    if( nextM.isValid() )
    {
        nextSize = nextM.getImgSize();
    }

    P2G::P2GMgr::DsdnQueryOut out = mP2GMgr->queryDSDNInfos(request->mMasterID, request, nextSize, mRunCache[request->mMasterID].mRun);
    MBOOL isMaster = MTRUE;
    fillDsdnData(out, isMaster, data, inM, outM);

    if( runSlave )
    {
        MSize next(0, 0);
        BasicImg nextS = data.mHW.mOutS.getNext();
        if( nextS.isValid() )
        {
            next = nextS.getImgSize();
        }
        P2G::P2GMgr::DsdnQueryOut out = mP2GMgr->queryDSDNInfos(request->mSlaveID, request, next, mRunCache[request->mSlaveID].mRun);
        isMaster = MFALSE;
        fillDsdnData(out, isMaster, data, inS, outS);
    }
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::fillDsdnData(const P2G::P2GMgr::DsdnQueryOut &dsdnOut, MBOOL isMaster, P2AEnqueData &data, IOIn &ioIn, IOOut &ioOut)
{
    ioIn.mConfSize = dsdnOut.confSize;
    ioIn.mIdiSize = dsdnOut.idiSize;
    ioOut.mOMC = dsdnOut.omc;

    if( isMaster )
    {
        data.mDSDNSizes = dsdnOut.dsSizes;
        data.mSW.mP2Sizes = dsdnOut.p2Sizes;
        data.mSW.mDSDNInfo = dsdnOut.outInfo;
        data.mSW.mOMCMV = dsdnOut.omc;
    }
    else
    {
        data.mSlaveDSDNSizes = dsdnOut.dsSizes;
        data.mSW.mSlaveP2Sizes = dsdnOut.p2Sizes;
        data.mSW.mSlaveDSDNInfo = dsdnOut.outInfo;
        data.mSW.mSlaveOMCMV = dsdnOut.omc;
    }
}

MBOOL P2ANode::prepareFull(const RequestPtr &request, MBOOL run, MUINT32 sensorID, P2GOut &p2gOut, IOOut &ioOut)
{
    TRACE_FUNC_ENTER();
    if( run )
    {
        IAdvPQCtrl pqCtrl;
        pqCtrl = AdvPQCtrl::clone(request->getBasicPQ(sensorID), "s_p2a");
        requestFull(request, sensorID, pqCtrl, p2gOut, ioOut);

        ioOut.mPQCtrl = pqCtrl;
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::prepareDS(const RequestPtr &request, MUINT32 sensorID, const std::vector<MSize> &dsSize, P2GOut &p2gOut, IOOut &ioOut)
{
    TRACE_FUNC_ENTER();
    if( request->needDSDN20(sensorID) && dsSize.size() )
    {
        MSize size = dsSize[0];
        MRectF nextCrop = getNextCrop(request, ioOut);
        MUINT32 nextDmaConstrain = getNextDmaConstrain(ioOut);
        p2gOut.mDS.mBuffer = mP2GMgr->requestDsBuffer(0);
        p2gOut.mDN.mBuffer = mP2GMgr->requestDnBuffer(0);
        ioOut.mDS = P2G::makeGImg(p2gOut.mDS.mBuffer, size, nextCrop, nextDmaConstrain);
        ioOut.mDN = P2G::makeGImg(p2gOut.mDN.mBuffer, size, toMRectF(size));
        updateDsInfo(request, sensorID, p2gOut.mDS, nextCrop, size);
        ioOut.mDSBasicImg = p2gOut.mDS;
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::prepareGroupIO(const RequestPtr &request, const P2AEnqueData &data, const IOIn &ioInM, const IOIn &ioInS, const IOOut &ioOutM, const IOOut &ioOutS, std::vector<P2G::IO> &groupIO)
{
    TRACE_FUNC_ENTER();
    IMetadata *appOutPM = NULL, *appOutPS = NULL;
    P2G::Feature gmFeat, gsFeat, pmFeat, psFeat, stdFeat;
    IOIn dummyIn, pmIn, psIn;
    MBOOL dsdn30LoopM = data.mSW.mDSDNInfo ? data.mSW.mDSDNInfo->mLoopValid : MTRUE;
    MBOOL dsdn30LoopS = data.mSW.mSlaveDSDNInfo ? data.mSW.mSlaveDSDNInfo->mLoopValid : MTRUE;

    SFPIOManager &ioMgr = request->mSFPIOManager;
    const MUINT32 idM = request->mMasterID;
    const MUINT32 idS = request->mSlaveID;
    MUINT32 idF = request->mFDSensorID;
    const SFPIOMap &ioGen = ioMgr.getFirstGeneralIO();
    const SFPIOMap &ioPM = ioMgr.getPhysicalIO(idM);
    const SFPIOMap &ioPS = ioMgr.getPhysicalIO(idS);
    const SFPIOMap &ioLM = ioMgr.getLargeIO(idM);
    const SFPIOMap &ioLS = ioMgr.getLargeIO(idS);

    MBOOL slave = request->isSlaveParamValid();
    MBOOL runGM = ioGen.isValid() && needNormalYuv(idM, request);
    MBOOL runGS = slave && ioGen.isValid() && needNormalYuv(idS, request);
    MBOOL runPM = ioPM.isValid();
    MBOOL runPS = slave && ioPS.isValid();
    MBOOL runLM = ioLM.isValid();
    MBOOL runLS = ioLS.isValid();

    MUINT32 outGM = O_FULL|O_DISP|O_REC|O_EXTRA|O_DS|O_OMC;
    MUINT32 outGS = O_FULL|O_DS|O_OMC;
    MUINT32 outP = O_PHY|O_OMC;
    MUINT32 outL = O_LARGE;

    // check slave sensor available when idF want use slave
    if ( runGS && idS == idF)
    {
        outGS |= O_FD;
    }
    else // change FD sensor to use master
    {
        outGM |= O_FD;
        idF = idM;
    }

    if( runGM && runPM && SFPIOMap::isSameTuning(ioGen, ioPM, idM) )
    {
        runPM = MFALSE;
        outGM |= O_PHY;
        appOutPM = ioPM.mAppOut;
    }
    if( runGS && runPS && SFPIOMap::isSameTuning(ioGen, ioPS, idS) )
    {
        runPS = MFALSE;
        outGS |= O_PHY;
        appOutPS = ioPS.mAppOut;
    }

    if( mP2GMgr->isSupportDCE() ) stdFeat += P2G::Feature::DCE;
    if( mP2GMgr->isSupportDCE() ) gmFeat += P2G::Feature::DCE;
    if( mP2GMgr->isSupportDCE() ) gsFeat += P2G::Feature::DCE;
    pmFeat = psFeat = stdFeat;

    if( mP2GMgr->isSupportDRETun() ) gmFeat += P2G::Feature::DRE_TUN;
    if( mP2GMgr->isSupportDRETun() && mPipeUsage.supportSlaveNR() ) gsFeat += P2G::Feature::DRE_TUN;
    if( request->need3DNR(idM) )   gmFeat += P2G::Feature::NR3D;
    if( request->need3DNR(idS) )   gsFeat += P2G::Feature::NR3D;
    if( request->needDSDN25(idM) ) gmFeat += P2G::Feature::DSDN25;
    if( request->needDSDN25(idS) ) gsFeat += P2G::Feature::DSDN25;
    if( request->needDSDN30(idM) ) gmFeat += dsdn30LoopM ? P2G::Feature::DSDN30 : P2G::Feature::DSDN30_INIT;
    if( request->needDSDN30(idS) ) gsFeat += dsdn30LoopS ? P2G::Feature::DSDN30 : P2G::Feature::DSDN30_INIT;
    if( mSupportPhyNR )
    {
        pmFeat = gmFeat;
        psFeat = gsFeat;
        pmIn = ioInM;
        psIn = ioInS;
        outP |= O_FULL;
    }
    if( request->needDSDN20(idM) ) gmFeat += P2G::Feature::DSDN20;
    if( request->needDSDN20(idS) ) gsFeat += P2G::Feature::DSDN20;
    if( mSupportPhyNR && mSupportPhyDSDN20 )
    {
        pmFeat = gmFeat;
        psFeat = gsFeat;
        outP |= O_DS;
    }

    if( request->needEarlyDisplay() ) gmFeat += P2G::Feature::EARLY_DISP;
    if( mP2GMgr->isSupportTIMGO() ) gmFeat += P2G::Feature::TIMGO;

    TunBuffer syncG = runGS ? mP2GMgr->requestSyncTuningBuffer() : NULL;
    TunBuffer syncP = runPS ? mP2GMgr->requestSyncTuningBuffer() : NULL;
    TunBuffer syncL = runLS ? mP2GMgr->requestSyncTuningBuffer() : NULL;

    IOOut pqPM, pqPS, pqLM, pqLS;
    if( runPM ) pqPM.mPQCtrl = AdvPQCtrl::clone(request->getBasicPQ(idM), "s_pm");
    if( runPS ) pqPS.mPQCtrl = AdvPQCtrl::clone(request->getBasicPQ(idS), "s_ps");
    if( runLM ) pqLM.mPQCtrl = AdvPQCtrl::clone(request->getBasicPQ(idM), "s_lm");
    if( runLS ) pqLS.mPQCtrl = AdvPQCtrl::clone(request->getBasicPQ(idS), "s_ls");
    pqPM = mSupportPhyNR ? ioOutM : pqPM;
    pqPS = mSupportPhyNR ? ioOutS : pqPS;

    struct table
    {
        P2G::Scene        scene;
        MBOOL             run;
        MUINT32           sensorID;
        const SFPIOMap    &iomap;
        IMetadata         *exAppOut;
        MUINT32           outMask;
        IOIn              ioIn;
        IOOut             ioOut;
        TunBuffer         syncTun;
        P2G::Feature      feature;
    } myTable[] =
    {
        { GM, runGM, idM, ioGen, appOutPM, outGM, ioInM,   ioOutM, syncG, gmFeat},
        { GS, runGS, idS, ioGen, appOutPS, outGS, ioInS,   ioOutS, syncG, gsFeat},
        { PM, runPM, idM, ioPM,  NULL,     outP,  pmIn,    pqPM,   syncP, pmFeat},
        { PS, runPS, idS, ioPS,  NULL,     outP,  psIn,    pqPS,   syncP, psFeat},
        { LM, runLM, idM, ioLM,  NULL,     outL,  dummyIn, pqLM,   syncL, stdFeat},
        { LS, runLS, idS, ioLS,  NULL,     outL,  dummyIn, pqLS,   syncL, stdFeat},
    };

    for( const table &t : myTable )
    {
        if( t.run )
        {
            groupIO.emplace_back(makeIO(request, data, t.scene, t.sensorID, idF, t.iomap, t.exAppOut, t.outMask, t.ioIn, t.ioOut, t.syncTun, t.feature));
        }
    }

    mRunCache[request->mMasterID] = RunCache(runGM || runPM);
    mRunCache[request->mSlaveID] =  RunCache(runGS || runPS);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2ANode::updateMotion(const RequestPtr &request, const P2AEnqueData &data, const RSCResult &rsc, MBOOL runSlave)
{
    TRACE_FUNC_ENTER();
    if( (mWaitRSC || mUseLMV) && (data.mSW.mMotion || data.mSW.mSlaveMotion) )
    {
        mP2GMgr->runUpdateMotion(request, data.mSW, rsc, runSlave);
    }
    TRACE_FUNC_EXIT();
}

MBOOL P2ANode::runP2SW(const RequestPtr &request, const std::vector<P2G::P2SW> &p2sw)
{
    TRACE_FUNC_ENTER();
    mP2GMgr->waitFrameDepDone(request->mRequestNo);
    request->mTimer.startP2ATuning();
    mP2GMgr->runP2SW(p2sw);
    request->mTimer.stopP2ATuning();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::runP2HW(const RequestPtr &request, P2AEnqueData &data, const std::vector<P2G::P2HW> &p2hw, const std::vector<P2G::PMDP> &pmdp)
{
    TRACE_FUNC_ENTER();
    Feature::P2Util::DIPParams params;
    data.mHW.mP2HW = p2hw;
    data.mHW.mPMDP = pmdp;

    params = mP2GMgr->runP2HW(p2hw);

    MUINT32 expectMS = getExpectMS(request);
    NSCam::Feature::P2Util::updateExpectEndTime(params, expectMS);
    NSCam::Feature::P2Util::updateScenario(params, request->getAppFPS(), mPipeUsage.getStreamingSize().w);

    if( request->needPrintIO() )
    {
        NSCam::Feature::P2Util::printDIPParams(request->mLog, params);
        for( const P2G::P2HW &p2hw : p2hw )
        {
            if( p2hw.mIn.mTuningData )
            {
                printTuningParam(request->mLog, p2hw.mIn.mTuningData->mParam);
            }
        }
    }
    enqueFeatureStream(params, data);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL canShare(const NextIO::NextAttr &attr, const MSize &fullSize)
{
    return attr.mReadOnly && attr.mHighQuality && (!attr.mResize || (attr.mResize == fullSize)) &&
           !attr.mRatioCrop && !isValid(attr.mCrop) && attr.mPQ == NextIO::PQ_NONE;
}

P2IO::TYPE toP2IOType(const NextIO::NextAttr &attr)
{
    switch(attr.mPQ)
    {
    case NextIO::PQ_PREVIEW:  return P2IO::TYPE_DISPLAY;
    case NextIO::PQ_RECORD:   return P2IO::TYPE_RECORD;
    default:                  return P2IO::TYPE_UNKNOWN;
    }
}

P2ANode::LazyImg makeGImg(const ImgBuffer &buffer, const MRectF &srcCrop, const MSize &fullSize, MUINT32 dmaMode, const NextIO::NextAttr &attr = NextIO::NextAttr())
{
    MRectF crop = isValid(attr.mCrop) ? attr.mCrop : srcCrop;
    MSize size = isValid(attr.mResize) ? attr.mResize : fullSize;
    if( attr.mRatioCrop )
    {
        crop = applyViewRatio(crop, size);
    }
    return P2G::makeGImg(buffer, size, crop, dmaMode, toP2IOType(attr));
}

BasicImg toBasicImg(const RequestPtr &request, const P2ANode::LazyImg &img, const IAdvPQCtrl &pqCtrl, const BasicImg::SensorClipInfo &clip, const MSize &srcSize)
{
    BasicImg basic;
    if( img )
    {
        basic.mBuffer = img->getImgBuffer();
        basic.mSensorClipInfo = clip;
        basic.accumulate("p2anext", request->mLog, srcSize, img->getCrop(), img->getImgSize());
        basic.mPQCtrl = pqCtrl;
    }
    return basic;
}

MVOID P2ANode::requestFull(const RequestPtr &request, MUINT32 sensorID, const IAdvPQCtrl &pqCtrl, P2GOut &p2gOut, IOOut &ioOut)
{
    TRACE_FUNC_ENTER();
    MBOOL fsc = request->needEarlyFSCVendorFullImg();
    BasicImg::SensorClipInfo clip = request->getSensorClipInfo(sensorID);
    SrcCropInfo roi = request->getSrcCropInfo(sensorID);
    MRectF fullCrop = toMRectF(roi.mSrcCrop);
    MRectF crop = fsc ? request->getSensorVar<MRectF>(sensorID, SFP_VAR::FSC_RRZO_CROP_REGION, fullCrop) :
                  mPipeUsage.isForceP2Zoom() ? request->getZoomROI() : fullCrop;
    MUINT32 dmaMode = fsc ? mPipeUsage.getDMAConstrain() : DMAConstrain::DEFAULT;
    MSize srcSize = roi.mSrcSize;
    MSize fullSize = roi.mSrcCrop.s;

    LazyImg full;

    if( request->needNextFullImg(this, sensorID) )
    {
        NextIO::NextReq req = request->requestNextFullImg(this, sensorID);
        p2gOut.mNextMap = initNextMap(req);
        ioOut.mNext.reserve(req.mList.size());
        for( const auto &next : req.mList )
        {
            LazyImg img;
            if( !ioOut.mFull && !fsc && canShare(next.mImg.mAttr, fullSize) )
            {
                img = makeGImg(next.mImg.mBuffer, fullCrop, fullSize, DMAConstrain::DEFAULT);
                ioOut.mFull = img;
                p2gOut.mFull = toBasicImg(request, ioOut.mFull, pqCtrl, clip, srcSize);
            }
            else
            {
                img = makeGImg(next.mImg.mBuffer, crop, fullSize, dmaMode, next.mImg.mAttr);
                if( !next.mImg.mAttr.mHighQuality && !ioOut.mTiny )
                {
                    ioOut.mTiny = img;
                }
                else
                {
                    ioOut.mNext.push_back(img);
                }
            }
            BasicImg basic = toBasicImg(request, img, pqCtrl, clip, srcSize);
            updateNextMap(p2gOut.mNextMap, basic, next);
        }
        if( mPipeUsage.printNextIO() )
        {
            print("p2a_req", req);
            print("p2a_cfg", p2gOut.mNextMap);
        }
    }
    MBOOL needDumyFull = (sensorID == request->mMasterID) && !request->hasOutYuv();
    if( !ioOut.mFull &&
        (request->needFullImg(this, sensorID) || request->isForceIMG3O() || needDumyFull) )
    {
        ImgBuffer buffer = mP2GMgr->requestFullImgBuffer();
        ioOut.mFull = makeGImg(buffer, fullCrop, fullSize, DMAConstrain::DEFAULT);
        p2gOut.mFull = toBasicImg(request, ioOut.mFull, pqCtrl, clip, srcSize);
    }
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::updateDsInfo(const RequestPtr &request, MUINT32 sensorID, BasicImg &ds, const MRectF &crop, const MSize &size) const
{
    TRACE_FUNC_ENTER();
    SrcCropInfo roi = request->getSrcCropInfo(sensorID);
    ds.mSensorClipInfo = request->getSensorClipInfo(sensorID);
    ds.accumulate("p2aDs", request->mLog, roi.mSrcSize, crop, size);
    TRACE_FUNC_EXIT();
}

MRectF P2ANode::getNextCrop(const RequestPtr &request, IOOut &ioOut) const
{
    TRACE_FUNC_ENTER();
    MRectF crop;
    SrcCropInfo roi = request->getSrcCropInfo(request->mMasterID);
    crop = ioOut.mNext.size() && ioOut.mNext[0] ? ioOut.mNext[0]->getCrop() :
           ioOut.mFull ? ioOut.mFull->getCrop() :
           toMRectF(roi.mSrcCrop);
    TRACE_FUNC_EXIT();
    return crop;
}

MUINT32 P2ANode::getNextDmaConstrain(const IOOut &ioOut) const
{
    TRACE_FUNC_ENTER();
    MUINT32 dmaMode = DMAConstrain::DEFAULT;
    dmaMode = (ioOut.mNext.size() && ioOut.mNext[0]) ? ioOut.mNext[0]->getDmaConstrain() : ioOut.mFull ? ioOut.mFull->getDmaConstrain() :
              DMAConstrain::DEFAULT;
    TRACE_FUNC_EXIT();
    return dmaMode;
}

P2G::IO P2ANode::makeIO(const RequestPtr &request, const P2AEnqueData &data, P2G::Scene scene, MUINT32 sensorID, MUINT32 FDSensorID, const SFPIOMap &iomap, IMetadata *exAppOut, MUINT32 outMask, const IOIn &ioIn, const IOOut &ioOut, const TunBuffer &syncTun, const P2G::Feature &feature)
{
    TRACE_FUNC_ENTER();

    const SFPSensorInput &sensorIn = request->getSensorInput(sensorID);
    const SFPSensorTuning &sensorCfg = iomap.getTuning(sensorID);

    P2G::IO io;
    io.mScene = scene;
    io.mFeature = feature;

    io.mIn.mBasic = mP2GMgr->makePOD(request, sensorID, scene);
    if(io.mIn.mBasic.mFDSensorID != FDSensorID)
    {
        MY_LOGW("change io.mIn.mBasic.mFDSensorID from %d to %d", io.mIn.mBasic.mFDSensorID, FDSensorID);
        io.mIn.mBasic.mFDSensorID = FDSensorID;
    }

    P2G::LoopScene ls = P2G::toLoopScene(scene);
    io.mLoopIn = mLoopData[ls][sensorID];
    io.mLoopOut = P2G::IO::makeLoopData();
    mLoopData[ls][sensorID] = io.mLoopOut;

    io.mIn.mPQCtrl = ioOut.mPQCtrl;
    io.mIn.mDCE_n_magic = io.mIn.mBasic.mP2Pack.getSensorData().mMagic3A;
    io.mIn.mIMGISize = io.mIn.mBasic.mSrcCrop.s;
    io.mIn.mSrcCrop = io.mIn.mBasic.mSrcCrop;
    io.mIn.mSyncTun = syncTun;

    io.mIn.mAppInMeta = sensorIn.mAppInOverride ? sensorIn.mAppInOverride
                                                : sensorIn.mAppIn;
    io.mIn.mHalInMeta = sensorIn.mHalIn;
    if( scene != GS )
    {
        io.mOut.mAppOutMeta = iomap.mAppOut;
        io.mOut.mHalOutMeta = iomap.mHalOut;
    }
    io.mOut.mExAppOutMeta = exAppOut;

    MBOOL isRRZO = sensorCfg.isRRZOin();
    IImageBuffer *imgi = get(isRRZO ? sensorIn.mRRZO : sensorIn.mIMGO);
    io.mIn.mIMGI = P2G::makeGImg(imgi);
    io.mIn.mLCSO = P2G::makeGImg(sensorCfg.isLCSOin() ? get(sensorIn.mLCSO) : NULL);
    io.mIn.mLCSHO = P2G::makeGImg(sensorCfg.isLCSHOin() ? get(sensorIn.mLCSHO)
                                                        : NULL);
    io.mIn.mBasic.mResized = isRRZO;
    io.mIn.mBasic.mIsYuvIn = imgi ? !NSCam::isHalRawFormat((EImageFormat)imgi->getImgFormat()) : MFALSE;

    io.mIn.mNR3DMotion = (sensorID == request->mMasterID) ? data.mSW.mMotion : data.mSW.mSlaveMotion;
    io.mIn.mDSDNInfo = (sensorID == request->mMasterID) ? data.mSW.mDSDNInfo : data.mSW.mSlaveDSDNInfo;
    io.mIn.mDSDNSizes = (sensorID == request->mMasterID) ? data.mDSDNSizes : data.mSlaveDSDNSizes;
    io.mIn.mConfSize = ioIn.mConfSize;
    io.mIn.mIdiSize = ioIn.mIdiSize;

    fillOut(request, sensorID, outMask, ioOut, io.mOut);

    TRACE_FUNC_EXIT();
    return io;
}

MBOOL P2ANode::fillOut(const RequestPtr &request, MUINT32 sensorID, MUINT32 outMask, const IOOut &ioOut, P2G::IO::OutData &out)
{
    TRACE_FUNC_ENTER();
    P2IO io;
    std::vector<P2IO> ioList;
    if( (outMask & O_DS) )
    {
        out.mDS1 = ioOut.mDS;
        out.mDN1 = ioOut.mDN;
    }

    if( (outMask & O_OMC) )
    {
        out.mOMCMV = ioOut.mOMC;
    }

    if( (outMask & O_DISP) && request->popDisplayOutput(this, io) )
    {
        out.mDisplay = P2G::makeGImg(io);
    }
    if( (outMask & O_REC) && request->popRecordOutput(this, io) )
    {
        out.mRecord = P2G::makeGImg(io);
    }
    if( (outMask & O_FD) && request->popFDOutput(this, io) )
    {
        if( ioOut.mDSBasicImg.isValid() )
        {
            MRectF cropRect = ioOut.mDSBasicImg.mTransform.applyTo(io.mCropRect);
            MSizeF sourceSize = ioOut.mDSBasicImg.mBuffer->getImageBuffer()->getImgSize();
            refineBoundaryF("DsFD", sourceSize, cropRect, request->needPrintIO());
            io.mCropRect = cropRect;
        }
        out.mFD = P2G::makeGImg(io);
    }
    if( (outMask & O_EXTRA) && request->popExtraOutputs(this, ioList) )
    {
        out.mExtra = P2G::makeGImg(ioList);
    }
    if( outMask & O_FULL )
    {
        out.mFull = ioOut.mFull;
        out.mNext = ioOut.mNext;
    }
    if( ioOut.mTiny )
    {
        if( !out.mFD )
        {
            out.mFD = ioOut.mTiny;
        }
        else
        {
            MY_LOGW("img2o occupied by FD, use mdp for tiny");
            out.mNext.push_back(ioOut.mTiny);
        }
    }
    if( (outMask & O_PHY) && request->popPhysicalOutput(this, sensorID, io) )
    {
        out.mPhy = P2G::makeGImg(io);
    }
    if( (outMask & O_LARGE) && request->popLargeOutputs(this, sensorID, ioList) )
    {
        out.mLarge = P2G::makeGImg(ioList);
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MUINT32 P2ANode::getExpectMS(const RequestPtr &request) const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return mForceExpectMS ? mForceExpectMS : request->getNodeCycleTimeMs();
}

MVOID P2ANode::onDIPStreamBaseCB(const Feature::P2Util::DIPParams &params, const P2AEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing P2ANode class members
    TRACE_FUNC_ENTER();
    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Missing request");
    }
    else
    {
        request->mTimer.stopEnqueP2A();
        MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d enque done in %d ms, result = %d", mSensorIndex, request->mRequestNo, request->mTimer.getElapsedEnqueP2A(), params.mDequeSuccess);
        CAM_ULOGM_ASSERT(params.mDequeSuccess, "Frame %d enque result failed", request->mRequestNo);

        request->updateResult(params.mDequeSuccess);
        handleResultData(request, data);
        request->mTimer.stopP2A();
    }

    this->decExtThreadDependency();
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::handleResultData(const RequestPtr &request, const P2AEnqueData &data)
{
    // This function is not thread safe,
    // because it is called by onDIPParamsCB,
    // avoid accessing P2ANode class members
    TRACE_FUNC_ENTER();
    mP2GMgr->notifyFrameDone(request->mRequestNo);
    markNodeExit(request);
    handleData(ID_P2G_TO_PMDP, P2GHWData(data.mHW, request));
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::handleP2GData(const RequestPtr &request, P2AEnqueData &data, const P2G::Path &path)
{
    TRACE_FUNC_ENTER();
    markNodeExit(request);
    data.mSW.mP2SW = path.mP2SW;
    data.mHW.mP2HW = path.mP2HW;
    data.mHW.mPMDP = path.mPMDP;

    handleData(ID_P2G_TO_MSS, MSSData(path.mMSS, request));
    handleData(ID_P2G_TO_P2SW, P2GSWData(data.mSW, request));
    handleData(ID_P2G_TO_MSF, P2GHWData(data.mHW, request));
    if( mPipeUsage.supportDSDN30() )
    {
        handleData(ID_P2G_TO_PMSS, MSSData(path.mPMSS, request));
    }
    TRACE_FUNC_EXIT();
}

MVOID P2ANode::enqueFeatureStream(Feature::P2Util::DIPParams &params, P2AEnqueData &data)
{
    TRACE_FUNC_ENTER();
    MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d enque start", mSensorIndex, data.mRequest->mRequestNo);
    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_P2));
    data.mRequest->mTimer.startEnqueP2A();
    this->incExtThreadDependency();
    this->enqueDIPStreamBase(mDIPStream, params, data);
    TRACE_FUNC_EXIT();
}

MBOOL P2ANode::init3A()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();

    MY_LOGD("mSensorIndex=%d", mSensorIndex);
    if( mp3A == NULL && SUPPORT_3A_HAL )
    {
        mp3A = MAKE_Hal3A(mSensorIndex, PIPE_CLASS_TAG);
    }

    if( EIS_MODE_IS_EIS_22_ENABLED(mEisMode) ||
        EIS_MODE_IS_EIS_30_ENABLED(mEisMode) )
    {
        //Disable OIS
        MY_LOGD("mEisMode: 0x%x => Disable OIS \n", mEisMode);
        if( mp3A )
        {
            mp3A->send3ACtrl(NS3Av3::E3ACtrl_SetEnableOIS, 0, 0);
        }
        else
        {
            MY_LOGE("mp3A is NULL\n");
        }
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2ANode::uninit3A()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();

    if( mp3A )
    {
        //Enable OIS
        if( EIS_MODE_IS_EIS_22_ENABLED(mEisMode) ||
            EIS_MODE_IS_EIS_30_ENABLED(mEisMode) )
        {
            MY_LOGD("mEisMode: 0x%x => Enable OIS \n", mEisMode);
            mp3A->send3ACtrl(NS3Av3::E3ACtrl_SetEnableOIS, 1, 0);
            mEisMode = EIS_MODE_OFF;
        }

        // turn OFF 'pull up ISO value to gain FPS'
        NS3Av3::AE_Pline_Limitation_T params;
        params.bEnable = MFALSE; // disable
        params.bEquivalent= MTRUE;
        params.u4IncreaseISO_x100= 100;
        params.u4IncreaseShutter_x100= 100;
        mp3A->send3ACtrl(NS3Av3::E3ACtrl_SetAEPlineLimitation, (MINTPTR)&params, 0);

        // destroy the instance
        mp3A->destroyInstance(PIPE_CLASS_TAG);
        mp3A = NULL;

    }

    TRACE_FUNC_EXIT();
}

MBOOL P2ANode::needNormalYuv(MUINT32 sensorID, const RequestPtr &request)
{
    return ( sensorID == request->mMasterID && request->hasGeneralOutput() ) ||
           request->needFullImg(this, sensorID) ||
           request->needNextFullImg(this, sensorID) ||
           ( request->mFDSensorID == sensorID && request->hasFDOutput() );
}

} // namespace G
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
