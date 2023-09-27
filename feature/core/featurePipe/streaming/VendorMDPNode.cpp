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

#include "VendorMDPNode.h"
#include "NextIOUtil.h"

#define PIPE_CLASS_TAG "VMDPNode"
#define PIPE_TRACE TRACE_VMDP_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam3/feature/eis/eis_ext.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_VENDOR_MDP);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

enum DumpMaskIndex
{
    MASK_ALL,
};

const std::vector<DumpFilter> sFilterTable =
{
    DumpFilter( MASK_ALL,      "all",       MFALSE )
};

VendorMDPNode::VendorMDPNode(const char *name, SFN_HINT typeHint)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_VENDOR_MDP)
    , StreamingFeatureNode(name)
    , mSFNHint(typeHint)
    , mMDP(PIPE_CLASS_TAG)
    , mLog(name)
{
    TRACE_N_FUNC_ENTER(mLog);
    this->addWaitQueue(&mData);
    mNumMDPPort = std::max<MUINT32>(1, mMDP.getNumOutPort());
    mForceExpectMS = getPropertyValue("vendor.debug.fpipe.vmdp.expect", 0);
    setTimerIndex();
    TRACE_N_FUNC_EXIT(mLog);
}

VendorMDPNode::~VendorMDPNode()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC_EXIT(mLog);
}

MBOOL VendorMDPNode::onData(DataID id, const VMDPReqData &data)
{
    TRACE_N_FUNC(mLog, "Frame %d: %s arrived +", data.mRequest->mRequestNo, ID2Name(id));
    if( id == ID_TPI_TO_NEXT ||
        id == ID_TPI_TO_NEXT_R )
    {
        this->mData.enque(data);
    }
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL VendorMDPNode::onData(DataID id, const DualBasicImgData &data)
{
    TRACE_N_FUNC(mLog, "Frame %d: %s arrived +", data.mRequest->mRequestNo, ID2Name(id));
    if( id == ID_VNR_TO_NEXT_FULLIMG )
    {
        this->mData.enque(toVMDPReqData(data));
    }
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

NextIO::Policy VendorMDPNode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    MBOOL dsdn20General = HAS_DSDN20(reqInfo.mFeatureMask) && (!HAS_EIS(reqInfo.mFeatureMask) || !mPipeUsage.supportPreviewEIS());
    MBOOL allowMaster = HAS_TPI_YUV(reqInfo.mFeatureMask) || dsdn20General;

    NextIO::Policy policy;
    if( reqInfo.hasMaster() && allowMaster )
    {
        StreamMask run;
        run.enableAll();
        if( !mSupportPhyDSDN20 || !HAS_DSDN20(reqInfo.mFeatureMask) ||  mSFNHint != SFN_HINT::PREVIEW)
        {
            run.disable(STREAMTYPE_PHYSICAL);
        }
        policy.setRun(run, reqInfo.mMasterID);
    }
    if( reqInfo.hasSlave() && mSupportPhyDSDN20 && HAS_DSDN20_S(reqInfo.mFeatureMask) && mSFNHint == SFN_HINT::PREVIEW )
    {
        StreamMask run;
        run.enable(STREAMTYPE_PHYSICAL);
        policy.setRun(run, reqInfo.mSlaveID);
    }

    return policy;
}

MVOID VendorMDPNode::setSyncCtrlWait(const std::shared_ptr<ISyncCtrl> &syncCtrl)
{
    TRACE_N_FUNC_ENTER(mLog);
    mSyncCtrlWait = syncCtrl;
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID VendorMDPNode::setSyncCtrlSignal(const std::shared_ptr<ISyncCtrl> &syncCtrl)
{
    TRACE_N_FUNC_ENTER(mLog);
    mSyncCtrlSignal = syncCtrl;
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID VendorMDPNode::setTimerIndex()
{
    if     ( mSFNHint == SFN_HINT::PREVIEW )  mTimerIndex = 0;
    else if( mSFNHint == SFN_HINT::RECORD )   mTimerIndex = 1;
    else                                      mTimerIndex = 0;
    CAM_ULOGM_ASSERT(mTimerIndex < MAX_VMDP_NODE_COUNT, "TimerIndex(%d) error", mTimerIndex);
}

SFN_TYPE::SFN_VALUE VendorMDPNode::getSFNType()
{
    SFN_TYPE::SFN_VALUE type = SFN_TYPE::BAD;
    MBOOL twinWarp = mPipeUsage.supportWarpNode(SFN_HINT::TWIN);
    MBOOL twinVmdp = mPipeUsage.supportVMDPNode(SFN_HINT::TWIN);

    if( !twinWarp && !twinVmdp )               type = SFN_TYPE::UNI;
    else if( !twinVmdp )                       type = SFN_TYPE::DIV;
    else if( mSFNHint == SFN_HINT::RECORD )    type = SFN_TYPE::TWIN_R;
    else if( mSFNHint == SFN_HINT::PREVIEW )   type = SFN_TYPE::TWIN_P;
    else                                       type = SFN_TYPE::BAD;
    return type;
}

MBOOL VendorMDPNode::onInit()
{
    TRACE_N_FUNC_ENTER(mLog);
    MBOOL ret = MTRUE;
    StreamingFeatureNode::onInit();
    enableDumpMask(sFilterTable);
    mSupportPhyDSDN20 = mPipeUsage.supportPhysicalDSDN20();
    mSFNType = SFN_TYPE(getSFNType());
    mLog = mSFNType.Type2Name();
    mDoneMsg = ( mSFNType.isTwinP() && mPipeUsage.supportVMDPNode(SFN_HINT::RECORD) ) ? FeaturePipeParam::MSG_DISPLAY_DONE : FeaturePipeParam::MSG_FRAME_DONE;
    if( !mSFNType.isValid() )
    {
        MY_N_LOGE(mLog, "Invalid node type %s", mSFNType.Type2Name());
        ret = MFALSE;
    }
    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MBOOL VendorMDPNode::onUninit()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL VendorMDPNode::onThreadStart()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL VendorMDPNode::onThreadStop()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL VendorMDPNode::onThreadLoop()
{
    TRACE_N_FUNC(mLog, "Waitloop");
    RequestPtr request;
    VMDPReqData data;
    HelperData dispVmdpHelperData;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mData.deque(data) )
    {
        MY_N_LOGE(mLog, "Data deque out of sync");
        return MFALSE;
    }

    if( data.mRequest == NULL)
    {
        MY_N_LOGE(mLog, "Request out of sync");
        return MFALSE;
    }
    TRACE_N_FUNC_ENTER(mLog);
    request = data.mRequest;
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    request->mTimer.startVmdp(mTimerIndex);
    markNodeEnter(request);
    MY_SN_LOGD(request->mLog, mLog, "Frame %d process VendorMDP start", request->mRequestNo);
    processVendorMDP(request, data.mData, data.mData.mSyncCtrlData);
    request->mTimer.stopVmdp(mTimerIndex);
    MY_SN_LOGD(request->mLog, mLog, "Frame %d process VendorMDP done in %d ms", request->mRequestNo, request->mTimer.getElapsedVmdp(mTimerIndex));
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL VendorMDPNode::processVendorMDP(const RequestPtr &request, const VMDPReq &mdpReq, const SyncCtrlData &syncCtrlData)
{
    TRACE_N_FUNC_ENTER(mLog);

    NextIO::NextReq nextReq;
    BasicImg nextFullImg;
    ImgBuffer asyncImg;
    MBOOL result = MTRUE;
    TRACE_N_FUNC(mLog, "mdp input img=%p buffer=%p slaveImg=%p slaveBuf=%p supportTPI(RQQ:%d, TSQ:%d)",
            mdpReq.mInputImg.mMaster.mBuffer.get(),
            mdpReq.mInputImg.mMaster.getImageBufferPtr(),
            mdpReq.mInputImg.mSlave.mBuffer.get(),
            mdpReq.mInputImg.mSlave.getImageBufferPtr(),
            mPipeUsage.supportTPI_RQQ(),
            mPipeUsage.supportTPI_TSQ());
    if( mdpReq.mTSQRecInfo.mNeedDrop || ( request->needRecordOutput(this) && !mdpReq.mInputImg ) )
    {
        dropRecord(request);
        MY_S_LOGD_IF(request->mLog.getLogLevel(), request->mLog, "record drop");
    }
    else
    {
        result = processGeneralMDP(request, mdpReq.mInputImg, nextFullImg, asyncImg, nextReq, mdpReq.mTSQRecInfo, syncCtrlData);
    }
    syncCtrlData.notify(mSyncCtrlSignal, request->mRequestNo);
    request->updateResult(result);

    markNodeExit(request);
    handleResultData(request, mdpReq, nextFullImg, nextReq);

    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MVOID VendorMDPNode::waitSync(const RequestPtr &request, const SyncCtrlData &syncCtrlData)
{
    TRACE_N_FUNC_ENTER(mLog);
    request->mTimer.startVmdpWait(mTimerIndex);
    mWaitFrame = syncCtrlData.getWaitFrame(mSyncCtrlWait, request->mRequestNo);
    syncCtrlData.wait(mSyncCtrlWait, request->mRequestNo);
    mWaitFrame = 0;
    request->mTimer.stopVmdpWait(mTimerIndex);
    TRACE_N_FUNC_EXIT(mLog);
}

MBOOL VendorMDPNode::processGeneralMDP(const RequestPtr &request, const DualBasicImg &dualImg, BasicImg &nextFullImg, ImgBuffer &asyncImg,  NextIO::NextReq &nextReq, const TSQRecInfo &tsqRecInfo, const SyncCtrlData &syncCtrlData)
{
    TRACE_N_FUNC_ENTER(mLog);

    MDPWrapper::P2IO_OUTPUT_ARRAY masterOuts = prepareMasterMDPOut(request, dualImg.mMaster, nextFullImg, asyncImg, nextReq, tsqRecInfo);
    MDPWrapper::P2IO_OUTPUT_ARRAY slaveOuts = prepareSlaveMDPOut(request, dualImg.mSlave);
    MUINT32 mdpCycleTimeMs = getMaxMDPCycleTimeMs(request, masterOuts.size(), slaveOuts.size());
    MY_S_LOGD_IF(request->mLog.getLogLevel(), request->mLog, "out(m/s)=(%zu/%zu), input(m/s)=(%d/%d) cycle=%d", masterOuts.size(), slaveOuts.size(),
                    dualImg.mMaster.isValid(), dualImg.mSlave.isValid(), mdpCycleTimeMs);
    MBOOL result = MTRUE;
    if( masterOuts.size() || slaveOuts.size() )
    {
        waitSync(request, syncCtrlData);
        MBOOL resultM = masterOuts.size() ? processMDP("master", request, dualImg.mMaster, masterOuts, mdpCycleTimeMs) : MTRUE;
        MBOOL resultS = slaveOuts.size()  ? processMDP("slave",  request, dualImg.mSlave,  slaveOuts,  mdpCycleTimeMs) : MTRUE;
        result = resultM && resultS;
    }
    TRACE_N_FUNC_EXIT(mLog);
    return result;
}

MBOOL VendorMDPNode::processMDP(const char* name, const RequestPtr &request, const BasicImg &fullImg, const MDPWrapper::P2IO_OUTPUT_ARRAY& outputs, MUINT32 cycleMs)
{
    TRACE_N_FUNC_ENTER(mLog);
    MBOOL result = MFALSE;
    if( fullImg.mBuffer == NULL )
    {
        MY_SN_LOGW(request->mLog, mLog, "sensor(%d) Frame %d fullImg is NULL in %s MDP run", mSensorIndex, request->mRequestNo, name);
    }
    else if(outputs.empty())
    {
        MY_SN_LOGW(request->mLog, mLog, "sensor(%d) Frame %d has no Output in %s MDP run", mSensorIndex, request->mRequestNo, name);
    }
    else
    {
        //If Dual pipe is needed, add condition below
        MBOOL isDualPipeNeed = mPipeUsage.supportVMDPNode(SFN_HINT::TWIN) &&
                               is4K2K(fullImg.mBuffer->getImgSize());
        result = mMDP.process(isDualPipeNeed? request->getAppFPS()*2 : request->getAppFPS(), fullImg.mBuffer->getImageBufferPtr(), outputs, request->needPrintIO(), isDualPipeNeed? cycleMs/2 : cycleMs);

        if( request->needDump() && allowDump(MASK_ALL))
        {
            for( unsigned i = 0; i < outputs.size(); ++i )
            {
                dumpData(request, outputs[i].mBuffer, "vmdp_%s_%s_%d", name, mSFNType.getDumpPostfix(), i);
            }
        }
    }
    TRACE_N_FUNC_EXIT(mLog);
    return result;
}

MVOID VendorMDPNode::handleResultData(const RequestPtr &request, const VMDPReq &mdpReq, const BasicImg &nextFullImg, const NextIO::NextReq &nextReq)
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC(mLog, "handleData frame %d doneMsg=%d needEarlyDisplay=%d", request->mRequestNo,
                        mDoneMsg, request->needEarlyDisplay());
    if( mPipeUsage.supportWarpNode() )
    {
        if( mSFNType.isDiv() )
        {
            handleData(ID_VMDP_TO_NEXT_P, BasicImgData(nextFullImg.mBuffer != NULL ? nextFullImg : mdpReq.mInputImg.mMaster, request));
            handleData(ID_VMDP_TO_NEXT_R, BasicImgData(nextFullImg.mBuffer != NULL ? nextFullImg : mdpReq.mInputImg.mMaster, request));
        }
        else
        {
            handleData(ID_VMDP_TO_NEXT, BasicImgData(nextFullImg.mBuffer != NULL ? nextFullImg : mdpReq.mInputImg.mMaster, request));
            if( request->needEarlyDisplay() )
            {
                handleData(ID_VMDP_TO_HELPER, HelperData(HelpReq(FeaturePipeParam::MSG_DISPLAY_DONE), request));
            }
        }
    }
    else if( mPipeUsage.supportXNode() )
    {
        NextResult nextResult;
        NextImgMap map = initNextMap(nextReq);
        updateFirst(map, nextReq, nextFullImg);
        addMaster(nextResult, request->mMasterID, map);
        handleData(ID_VMDP_TO_XNODE, NextData(nextResult, request));
    }
    else
    {   // last node
        handleData(ID_VMDP_TO_HELPER, HelperData(HelpReq(mDoneMsg), request));
    }
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID VendorMDPNode::dumpAdditionalInfo(android::Printer &printer) const
{
    TRACE_N_FUNC_ENTER(mLog);
    if( mSyncCtrlWait )
    {
        printer.printFormatLine("[FPipeStatus] %s syncCtrl waitFrame=%u", mLog, mWaitFrame);
    }
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID VendorMDPNode::dropRecord(const RequestPtr &request)
{
    TRACE_N_FUNC_ENTER(mLog);
    MY_N_LOGD(mLog, "frame %d need drop record", request->mRequestNo);
    request->setVar<MBOOL>(SFP_VAR::EIS_SKIP_RECORD, MTRUE);
    TRACE_N_FUNC_EXIT(mLog);
}

MDPWrapper::P2IO_OUTPUT_ARRAY VendorMDPNode::prepareMasterMDPOut(const RequestPtr &request, const BasicImg &fullImg, BasicImg &nextFullImg, ImgBuffer &asyncImg, NextIO::NextReq &nextReq, const TSQRecInfo &tsqRecInfo)
{
    TRACE_N_FUNC_ENTER(mLog);
    MDPWrapper::P2IO_OUTPUT_ARRAY outputs;
    P2IO out;
    IAdvPQCtrl_const pqCtrl = fullImg.mPQCtrl;
    std::shared_ptr<AdvPQCtrl> recPQ;
    if( mSFNType.isTwinR() )
    {
        recPQ = AdvPQCtrl::clone(fullImg.mPQCtrl, "s_vmdp_rec");
        recPQ->setIs2ndDRE(MTRUE);
        pqCtrl = recPQ;
    }

    // display
    if( request->popDisplayOutput(this, out) )
    {
        refineCrop(request, fullImg, out);
        out.mPQCtrl = pqCtrl;
        outputs.push_back(out);

        if( request->needTPIAsync() )
        {
            asyncImg = request->popAsyncImg(this);
            if( asyncImg != NULL )
            {
                P2IO async;
                async.mBuffer = asyncImg->getImageBufferPtr();
                async.mCropDstSize = asyncImg->getImgSize();
                async.mCropRect = out.mCropRect;
                outputs.push_back(async);
            }
        }
    }

    // record
    if( request->popRecordOutput(this, out) )
    {
        overrideTSQRecInfo(fullImg, tsqRecInfo, out);
        refineCrop(request, fullImg, out);
        out.mPQCtrl = pqCtrl;
        if( mSFNType.isTwinR() )
        {
            out.mType = P2IO::TYPE_RECORD;
        }
        outputs.push_back(out);
    }

    // Extra data
    std::vector<P2IO> extraOuts;
    if( request->popExtraOutputs(this, extraOuts))
    {
        for(P2IO out : extraOuts)
        {
            refineCrop(request, fullImg, out);
            out.mPQCtrl = pqCtrl;
            outputs.push_back(out);
        }
    }

    // next FullImg
    const MUINT32 masterID = request->mMasterID;
    if( request->needNextFullImg(this, masterID) )
    {
        MSize inSize = fullImg.mBuffer->getImageBuffer()->getImgSize();
        MSize outSize = inSize;
        nextReq = request->requestNextFullImg(this, masterID);
        NextIO::NextBuf firstNext = getFirst(nextReq);

        nextFullImg.mBuffer = firstNext.mBuffer;
        if( nextFullImg.mBuffer != NULL )
        {
            nextFullImg.setAllInfo(fullImg);
            if( firstNext.mAttr.mResize.w && firstNext.mAttr.mResize.h )
            {
                outSize = firstNext.mAttr.mResize;
            }

            P2IO out;
            out.mBuffer = nextFullImg.mBuffer->getImageBufferPtr();
            out.mBuffer->setExtParam(outSize);
            out.mCropRect = MRect(MPoint(0,0), fullImg.mBuffer->getImageBuffer()->getImgSize());
            out.mCropDstSize = outSize;
            if( mSFNType.isTwinR() )
            {
                out.mType = P2IO::TYPE_RECORD;
            }
            outputs.push_back(out);

            nextFullImg.accumulate("vmdpNFull", request->mLog, inSize, out.mCropRect, outSize);
        }
        else
        {
            MY_N_LOGW(mLog, "sensor(%d) Frame %d need next full but cannot get buffer",
                    mSensorIndex, request->mRequestNo);
        }
    }
    // physical
    if( request->popPhysicalOutput(this, masterID, out) )
    {
        refineCrop(request, fullImg, out);
        out.mPQCtrl = pqCtrl;
        outputs.push_back(out);
    }
    TRACE_N_FUNC(mLog, "outputs.size:%u -", outputs.size());
    return outputs;
}

MDPWrapper::P2IO_OUTPUT_ARRAY VendorMDPNode::prepareSlaveMDPOut(const RequestPtr &request, const BasicImg &fullImg)
{
    TRACE_N_FUNC_ENTER(mLog);
    MDPWrapper::P2IO_OUTPUT_ARRAY outputs;
    P2IO out;
    IAdvPQCtrl_const pqCtrl = fullImg.mPQCtrl;
    // physical
    if( request->popPhysicalOutput(this, request->mSlaveID, out) )
    {
        refineCrop(request, fullImg, out);
        out.mPQCtrl = pqCtrl;
        outputs.push_back(out);
    }
    TRACE_N_FUNC(mLog, "outputs.size:%u -", outputs.size());
    return outputs;
}


MVOID VendorMDPNode::refineCrop(const RequestPtr &request, const BasicImg &fullImg, P2IO &out)
{
    (void)request;
    MRectF cropRect = fullImg.mTransform.applyTo(out.mCropRect);

    TRACE_N_FUNC(mLog, "No(%d)," MTransF_STR ",source=" MCropF_STR "->result=" MCropF_STR,
            request->mRequestNo, MTransF_ARG(fullImg.mTransform.mOffset, fullImg.mTransform.mScale),
            MCropF_ARG(out.mCropRect), MCropF_ARG(cropRect));

    MSizeF sourceSize = fullImg.mBuffer->getImageBuffer()->getImgSize();
    refineBoundaryF("VendorMDPNode", sourceSize, cropRect, request->needPrintIO());
    out.mCropRect = cropRect;
}

MVOID VendorMDPNode::overrideTSQRecInfo(const BasicImg &fullImg, const TSQRecInfo &tsqRecInfo, P2IO &out)
{
    TRACE_N_FUNC(mLog, "tsqRecInfo.mOverride=%d", tsqRecInfo.mOverride);
    if( mPipeUsage.supportTPI_TSQ() && tsqRecInfo.mOverride )
    {
        out.mCropRect = tsqRecInfo.mOverrideCrop;
    }
    out.mBuffer->setTimestamp(fullImg.mBuffer->getImageBuffer()->getTimestamp());
}

MUINT32 VendorMDPNode::getMaxMDPCycleTimeMs(const RequestPtr &request, MUINT32 masterOuts, MUINT32 slaveOuts)
{
    MUINT32 cycle = (masterOuts+mNumMDPPort-1) / mNumMDPPort;

    if( slaveOuts || mPipeUsage.supportVMDPNode(SFN_HINT::TWIN) ) cycle += 1;
    MUINT32 expectMs = (mForceExpectMS > 0) ? mForceExpectMS : request->getNodeCycleTimeMs();
    MUINT32 maxTimeMs = expectMs / std::max<MUINT32>(1, cycle);
    MY_S_LOGD_IF(request->mLog.getLogLevel(), request->mLog, "mNumPort(%d) expect(%d) runOut M/S(%d/%d) cycle(%d) cycleMs(%d)", mNumMDPPort, expectMs, masterOuts, slaveOuts, cycle, maxTimeMs);
    return maxTimeMs;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
