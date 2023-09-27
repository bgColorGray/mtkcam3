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

#include "P2AMDPNode.h"

#define PIPE_CLASS_TAG "P2AMDPNode"
#define PIPE_TRACE TRACE_P2AMDP_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_P2A_MDP);


namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

const std::vector<DumpFilter> P2AMDPNode::sFilterTable =
{
    DumpFilter( P2AMDPNode::MASK_FULL_M,           "master_img3o",         MTRUE),
    DumpFilter( P2AMDPNode::MASK_NEXT_M,           "master_nextfull",      MTRUE),
    DumpFilter( P2AMDPNode::MASK_IMG2O_M,          "master_img2o",         MTRUE),
    DumpFilter( P2AMDPNode::MASK_FULL_S,           "slave_img3o",          MTRUE),
    DumpFilter( P2AMDPNode::MASK_NEXT_S,           "slave_nextfull",       MTRUE),
    DumpFilter( P2AMDPNode::MASK_IMG2O_S,          "slave_img2o",          MTRUE),
    DumpFilter( P2AMDPNode::MASK_TIMGO,            "timgo",                MTRUE),
    DumpFilter( P2AMDPNode::MASK_DSDN30_DSW_WEI,   "dsdn30_dswi_weight",   MTRUE),
    DumpFilter( P2AMDPNode::MASK_DSDN25_SMVR_SUB,  "subDSDN25orSMVR",      MFALSE),
    DumpFilter( P2AMDPNode::MASK_DSDN30_CONF_IDI,  "ConfandIdi",           MFALSE),
    DumpFilter( P2AMDPNode::MASK_DNO_M,            "master_dno",           MTRUE),
    DumpFilter( P2AMDPNode::MASK_DNO_S,            "slave_dno",            MTRUE),
    DumpFilter( P2AMDPNode::MASK_PMDP_ALL,         "pmdp_all",             MFALSE),
};


P2AMDPNode::P2AMDPNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_P2A_MDP)
    , StreamingFeatureNode(name)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mMDPRequests);
    mNumMDPPort = std::max<MUINT32>(1, mMDP.getNumOutPort());
    mForceExpectMS = getPropertyValue("vendor.debug.fpipe.pmdp.expect", 0);
    TRACE_FUNC_EXIT();
}

P2AMDPNode::~P2AMDPNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL P2AMDPNode::onData(DataID id, const P2GHWData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_P2G_TO_PMDP )
    {
        mMDPRequests.enque(data);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL P2AMDPNode::onInit()
{
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();
    enableDumpMask(sFilterTable);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2AMDPNode::onThreadLoop()
{
    TRACE_FUNC_ENTER();
    P2GHWData mdpRequest;
    if( !waitAllQueue() )
    {
        TRACE_FUNC("Wait all queue exit");
        return MFALSE;
    }
    if( !mMDPRequests.deque(mdpRequest) )
    {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    }
    if( mdpRequest.mRequest == NULL )
    {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }
    RequestPtr request = mdpRequest.mRequest;
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    request->mTimer.startP2AMDP();
    P2HWHolder &hw = mdpRequest.mData;
    handleRequest(P2G::makePMDPReq(hw.mPMDP), request);
    handleDump(request, hw);
    handleResultData(request, hw);
    request->mTimer.stopP2AMDP();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2AMDPNode::handleRequest(const PMDPReq &data, const RequestPtr &request)
{
    TRACE_FUNC_ENTER();
    markNodeEnter(request);
    MUINT32 maxMdpCycle = getMaxMDPCycleTimeMs(data, request);
    for( const PMDPReq::Data &run : data.mDatas )
    {
        processMDP(request, run.mMDPIn, run.mMDPOuts, run.mTuning, maxMdpCycle);
    }
    markNodeExit(request);
    TRACE_FUNC_EXIT();
}

MBOOL P2AMDPNode::processMDP(const RequestPtr &request, const BasicImg &mdpIn, std::vector<P2IO> mdpOuts, const TunBuffer &tuning, MUINT32 cycleTime)
{
    TRACE_FUNC_ENTER();
    if(mdpOuts.empty())
    {
        if(mdpIn.mBuffer == NULL)
        {
            return MTRUE;
        }
        else
        {
            MY_LOGE("Frame %d mdp in fullImg not NULL w/ mdp out list size(%zu)", request->mRequestNo, mdpOuts.size());
            return MFALSE;
        }
    }
    MBOOL result = MFALSE;

    if( mdpIn.mBuffer != NULL )
    {
        if(request->needPrintIO())
        {
            MY_LOGD("Frame %d in Helper doMDP, in(%p) , out size(%zu)", request->mRequestNo,
                mdpIn.mBuffer->getImageBufferPtr(), mdpOuts.size());
        }

        std::shared_ptr<AdvPQCtrl> pqCtrl;
        pqCtrl = AdvPQCtrl::clone(mdpIn.mPQCtrl, "s_pmdp");
        pqCtrl->setIspTuning(tuning);
        for( P2IO &out : mdpOuts )
        {
            out.mPQCtrl = pqCtrl;
        }
        result = mMDP.process(request->getAppFPS(), mdpIn.mBuffer->getImageBufferPtr(), mdpOuts, request->needPrintIO(), cycleTime);
        request->updateResult(result);

        if( request->needDump() && allowDump(MASK_PMDP_ALL))
        {
            const size_t size = mdpOuts.size();
            for( unsigned i = 0; i < size; ++i )
            {
                dumpData(request, mdpOuts[i].mBuffer, "p2amdp_%d", i);
            }
        }
    }
    else
    {
        MY_LOGE("Frame %d mdp in fullImg is NULL", request->mRequestNo);
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MUINT32 P2AMDPNode::getMaxMDPCycleTimeMs(const PMDPReq &data, const RequestPtr &request)
{
    MUINT32 expectMs = (mForceExpectMS > 0) ? mForceExpectMS : request->getNodeCycleTimeMs();
    MUINT32 cycle = 0;
    for( const PMDPReq::Data &run : data.mDatas )
    {
        cycle += (run.mMDPOuts.size() + mNumMDPPort - 1) / mNumMDPPort;
    }
    MUINT32 maxTimeMs = expectMs / std::max<MUINT32>(1, cycle);
    MY_S_LOGD_IF( (!data.mDatas.empty() && request->mLog.getLogLevel()), request->mLog, "mNumPort(%d) datas(%zu) expect(%d) cycle(%d) cycleMs(%d)", mNumMDPPort, data.mDatas.size(), expectMs, cycle, maxTimeMs);
    return maxTimeMs;
}

MVOID P2AMDPNode::dump(const RequestPtr &request, IImageBuffer *buffer, enum DumpMaskIndex mask, MBOOL isDump, const char *dumpStr, MBOOL isNDD, const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const TuningUtils::YUV_PORT &port, const char *nddStr) const
{
    TRACE_FUNC_ENTER();
    if( allowDump(mask) && buffer != NULL )
    {
        buffer->syncCache(eCACHECTRL_INVALID);
        if( isDump )
        {
            dumpData(request, buffer, dumpStr);
        }
        if ( isNDD )
        {
            dumpNddData(hint, buffer, port, nddStr);
        }
    }
    TRACE_FUNC_EXIT();
}

MVOID P2AMDPNode::handleDump(const RequestPtr &request, const P2HWHolder &data) const
{
    TRACE_FUNC_ENTER();
    MBOOL isDump = request->needDump();
    MBOOL isNDD = request->needNddDump();

    if( isDump || isNDD )
    {
        Timer timer(true);
        TuningUtils::FILE_DUMP_NAMING_HINT hintM, hintS;
        hintM = request->mP2Pack.getSensorData(request->mMasterID).mNDDHint;
        hintS = request->mP2Pack.getSensorData(request->mSlaveID).mNDDHint;

        dump(request, data.mOutM.mFull.getImageBufferPtr(), MASK_FULL_M,
            isDump, "full", isNDD, hintM, TuningUtils::YUV_PORT_IMG3O);
        dump(request, data.mOutM.getNext().getImageBufferPtr(), MASK_NEXT_M,
            isDump, "nextfull", isNDD, hintM, TuningUtils::YUV_PORT_WDMAO, "full");
        dump(request, data.mOutM.mDS.getImageBufferPtr(), MASK_IMG2O_M,
            isDump, "img2o", isNDD, hintM, TuningUtils::YUV_PORT_IMG2O);
        dump(request, data.mOutM.mDN.getImageBufferPtr(), MASK_DNO_M,
            isDump, "dno", isNDD, hintM, TuningUtils::YUV_PORT_IMG3O, "p2nr");

        dump(request, data.mOutS.mFull.getImageBufferPtr(), MASK_FULL_S,
            isDump, "slaveFull", false, hintS, TuningUtils::YUV_PORT_IMG3O, "slave");
        dump(request, data.mOutS.getNext().getImageBufferPtr(), MASK_NEXT_S,
            isDump, "slaveNextFull", false, hintS, TuningUtils::YUV_PORT_WDMAO, "slavefull");
        dump(request, data.mOutS.mDS.getImageBufferPtr(), MASK_IMG2O_S,
            isDump, "img2o", isNDD, hintS, TuningUtils::YUV_PORT_IMG2O);
        dump(request, data.mOutS.mDN.getImageBufferPtr(), MASK_DNO_S,
            isDump, "slaveDno", isNDD, hintS, TuningUtils::YUV_PORT_IMG3O, "p2nr");

        uint32_t smvrIndex = 0;
        for( const P2G::P2HW &p2hw : data.mP2HW )
        {
            if( p2hw.mOut.mTIMGO )
            {
                dump(request, p2hw.mOut.mTIMGO->getImageBufferPtr(),
                     MASK_TIMGO, false, "timgo", isNDD, hintM,
                     TuningUtils::YUV_PORT_TIMGO, mP2GMgr->getTimgoTypeStr());
            }

            if( p2hw.mIn.mDSDNIndex == 0 && p2hw.mFeature.has(P2G::Feature::SMVR_SUB) )
            {
                smvrIndex += 1;
            }
            MBOOL needDump = (p2hw.mIn.mDSDNIndex != 0 || p2hw.mFeature.has(P2G::Feature::SMVR_SUB));
            struct table
            {
                const P2G::ILazy<P2G::GImg>     &buf;
                MBOOL                           run;
                const char                      *ioStr;
                DumpMaskIndex                   mask;
            } myTable[] =
            {
                { p2hw.mOut.mIMG3O,     needDump,       "out",  MASK_DSDN25_SMVR_SUB},
                { p2hw.mIn.mDSWI,       MTRUE,          "mo",   MASK_DSDN30_DSW_WEI},
                { p2hw.mIn.mWEIGHTI,    MTRUE,          "wt",   MASK_DSDN30_DSW_WEI},
                { p2hw.mIn.mCONFI,      MTRUE,          "conf", MASK_DSDN30_CONF_IDI},
                { p2hw.mIn.mIDI,        MTRUE,          "idi",  MASK_DSDN30_CONF_IDI},
            };

            for(const table &t : myTable)
            {
                if( t.buf && t.run )
                {
                    char str[64];
                    int res = snprintf(str, sizeof(str), "dsdn25-msfp2-L%d-%s-s%d-sm%d", p2hw.mIn.mDSDNIndex, t.ioStr, p2hw.mIn.mBasic.mSensorIndex, smvrIndex);
                    MY_LOGE_IF( (res < 0), "snprintf failed!");
                    dump(request, t.buf->getImageBufferPtr(),
                        t.mask, isDump, str, isNDD,
                        (p2hw.mIn.mBasic.mSensorIndex == request->mMasterID) ? hintM : hintS,
                        TuningUtils::YUV_PORT_NULL, str);
                }
            }
        }
        timer.stop();
        MY_S_LOGD(request->mLog, "Dump Buffer done, cose (%d) ms", timer.getElapsed());
    }


    TRACE_FUNC_EXIT();
}

MVOID P2AMDPNode::handleResultData(const RequestPtr &request, const P2HWHolder &data)
{
    TRACE_FUNC_ENTER();
    BasicImg nextM = data.mOutM.getNext();
    BasicImg nextS = data.mOutS.getNext();
    BasicImg full = nextM ? nextM : data.mOutM.mFull;
    BasicImg slave = nextS ? nextS : data.mOutS.mFull;

    if( mPipeUsage.supportDSDN20() )
    {
        DSDNImg dsdn(full, slave);
        dsdn.mPackM.mDS1Img = data.mOutM.mDS;
        dsdn.mPackM.mDS2Img = data.mOutM.mDN.mBuffer;
        dsdn.mPackS.mDS1Img = data.mOutS.mDS;
        dsdn.mPackS.mDS2Img = data.mOutS.mDN.mBuffer;
        handleData(ID_P2G_TO_VNR, DSDNData(dsdn, request));
    }
    else if( mPipeUsage.supportBokeh() )
    {
        handleData(ID_P2G_TO_BOKEH, BasicImgData(full, request));
    }
    else if( mPipeUsage.supportTPI(TPIOEntry::YUV) )
    {
        TRACE_FUNC("to vendor");
        handleData(ID_P2G_TO_VENDOR_FULLIMG,
                   DualBasicImgData(DualBasicImg(full, slave), request));
    }
    else if( mPipeUsage.supportWarpNode() )
    {
        if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )   handleData(ID_P2G_TO_WARP_P, BasicImgData(full, request));
        if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )    handleData(ID_P2G_TO_WARP_R, BasicImgData(full, request));
    }
    else if( mPipeUsage.supportXNode() )
    {
        NextResult nextResult;
        nextResult.mSlaveID = request->mSlaveID;
        nextResult.mMasterID = request->mMasterID;
        if( data.mOutS.mNextMap.size() )
        {
            nextResult.mMap[request->mSlaveID] = data.mOutS.mNextMap;
        }
        nextResult.mMap[request->mMasterID] = data.mOutM.mNextMap;
        handleData(ID_P2G_TO_XNODE, NextData(nextResult, request));
    }
    else
    {
        handleData(ID_P2G_TO_HELPER, HelperData(HelpReq(FeaturePipeParam::MSG_FRAME_DONE), request));
    }

    if( data.mEarlyDisplay )
    {
        handleData(ID_P2G_TO_HELPER, HelperData(HelpReq(FeaturePipeParam::MSG_DISPLAY_DONE), request));
    }

    TRACE_FUNC_EXIT();
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
