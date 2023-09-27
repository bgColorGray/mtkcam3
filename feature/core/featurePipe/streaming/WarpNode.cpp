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

#include "WarpNode.h"

#include "GPUWarpStream.h"
#include "WPEWarpStream.h"

#define PIPE_CLASS_TAG "WarpNode"
#define PIPE_TRACE TRACE_WARP_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <utils/String8.h>
#include <sys/stat.h>
#include <mtkcam3/feature/eis/eis_hal.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_EIS_WARP);

using NSCam::NSIoPipe::Output;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

enum DumpMaskIndex
{
    MASK_REC_WARP_IN,
    MASK_REC_WARP_OUT,
    MASK_REC_WARP_MAP,
    MASK_REC_MDP_OUT,
    MASK_DISP_WARP_IN,
    MASK_DISP_WARP_OUT,
    MASK_DISP_WARP_MAP,
    MASK_DISP_MDP_OUT,

    MASK_UNKNOWN = 31,
};

const std::vector<DumpFilter> sFilterTable =
{
    DumpFilter( MASK_REC_WARP_IN,       "rec_warp_in",      MTRUE),
    DumpFilter( MASK_REC_WARP_OUT,      "rec_warp_out",     MTRUE),
    DumpFilter( MASK_REC_WARP_MAP,      "rec_warpmap",      MTRUE),
    DumpFilter( MASK_REC_MDP_OUT,       "rec_mdp_out",      MTRUE),
    DumpFilter( MASK_DISP_WARP_IN,      "disp_warp_in",     MTRUE),
    DumpFilter( MASK_DISP_WARP_OUT,     "disp_warp_out",    MTRUE),
    DumpFilter( MASK_DISP_WARP_MAP,     "disp_warpmap",     MTRUE),
    DumpFilter( MASK_DISP_MDP_OUT,      "disp_mdp_out",     MTRUE)
};

enum WARP_FILTER
{
    FILTER_WARP_IN,
    FILTER_WARP_OUT,
    FILTER_WARP_MAP,
    FILTER_MDP_OUT,
};

DumpMaskIndex getMaskIndex(WARP_FILTER myFilter, MBOOL isPreview)
{
    switch(myFilter)
    {
        case FILTER_WARP_IN:
            return (isPreview) ? MASK_DISP_WARP_IN : MASK_REC_WARP_IN;
        case FILTER_WARP_OUT:
            return (isPreview) ? MASK_DISP_WARP_OUT : MASK_REC_WARP_OUT;
        case FILTER_WARP_MAP:
            return (isPreview) ? MASK_DISP_WARP_MAP : MASK_REC_WARP_MAP;
        case FILTER_MDP_OUT:
            return (isPreview) ? MASK_DISP_MDP_OUT : MASK_REC_MDP_OUT;
        default:
            MY_LOGF("unsupported Dump Warp Filter %d", myFilter);
            return MASK_UNKNOWN;
    }
}

WarpNode::WarpNode(const char *name, SFN_HINT typeHint)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_EIS_WARP)
    , StreamingFeatureNode(name)
    , mSFNHint(typeHint)
    , mWarpStream(NULL)
    , mJpegEncodeThread(NULL)
    , mLog(name)
{
    TRACE_N_FUNC_ENTER(mLog);
    this->addWaitQueue(&mInputImgDatas);
    this->addWaitQueue(&mWarpData);
    setTimerIndex();
    TRACE_N_FUNC_EXIT(mLog);
}

WarpNode::~WarpNode()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::setInputBufferPool(const android::sp<IBufferPool> &pool)
{
    TRACE_N_FUNC_ENTER(mLog);
    mInputBufferPool = pool;
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::setOutputBufferPool(const android::sp<IBufferPool> &pool)
{
    TRACE_N_FUNC_ENTER(mLog);
    mOutputBufferPool = pool;
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::clearTSQ()
{
    TRACE_N_FUNC_ENTER(mLog);
    mWarpSrcInfoQueue.clear();
    TRACE_N_FUNC_EXIT(mLog);
}

MBOOL WarpNode::onData(DataID id, const BasicImgData &data)
{
    TRACE_N_FUNC(mLog, "Frame %d: %s arrived +", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_BOKEH_TO_WARP_P_FULLIMG ||
        id == ID_BOKEH_TO_WARP_R_FULLIMG ||
        id == ID_P2G_TO_WARP_P ||
        id == ID_P2G_TO_WARP_R ||
        id == ID_VMDP_TO_NEXT_P ||
        id == ID_VMDP_TO_NEXT_R ||
        id == ID_VMDP_TO_NEXT )
    {
        this->mInputImgDatas.enque(data);
        ret = MTRUE;
    }
    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MBOOL WarpNode::onData(DataID id, const DualBasicImgData &data)
{
    TRACE_N_FUNC(mLog, "Frame %d: %s arrived +", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_VNR_TO_NEXT_P ||
        id == ID_VNR_TO_NEXT_R )
    {
        this->mInputImgDatas.enque(toBasicImgData(data));
        ret = MTRUE;
    }
    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MBOOL WarpNode::onData(DataID id, const WarpData &warpData)
{
    TRACE_N_FUNC(mLog, "Frame %d: %s arrived +", warpData.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_EIS_TO_WARP_P || id == ID_EIS_TO_WARP_R )
    {
        this->mWarpData.enque(warpData);
        ret = MTRUE;
    }

    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MVOID WarpNode::setSyncCtrlWait(const std::shared_ptr<ISyncCtrl> &syncCtrl)
{
    TRACE_N_FUNC_ENTER(mLog);
    mSyncCtrlWait = syncCtrl;
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::setSyncCtrlSignal(const std::shared_ptr<ISyncCtrl> &syncCtrl)
{
    TRACE_N_FUNC_ENTER(mLog);
    mSyncCtrlSignal = syncCtrl;
    TRACE_N_FUNC_EXIT(mLog);
}

NextIO::Policy WarpNode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    NextIO::Policy policy;
    NextIO::NextAttr attr;
    attr.setAttrByPool(mInputBufferPool);
    attr.mReadOnly = mUseSharedInput;

    if( reqInfo.hasMaster() )
    {
        policy.enableAll(reqInfo.mMasterID);
        NextIO::InInfo rrzo = reqInfo.getInInfo(NextIO::IN_RRZO);

        if( rrzo )
        {
            attr.mResize = rrzo.mSize;
        }
        policy.add(reqInfo.mMasterID, NextIO::NORMAL, attr, mInputBufferPool);
    }

    return policy;
}

MVOID WarpNode::setTimerIndex()
{
    if     ( mSFNHint == SFN_HINT::PREVIEW )  mTimerIndex = 0;
    else if( mSFNHint == SFN_HINT::RECORD )   mTimerIndex = 1;
    else                                      mTimerIndex = 0;
    CAM_ULOGM_ASSERT(mTimerIndex < MAX_WARP_NODE_COUNT, "TimerIndex(%d) error", mTimerIndex);
}

SFN_TYPE::SFN_VALUE WarpNode::getSFNType()
{
    SFN_TYPE::SFN_VALUE type = SFN_TYPE::BAD;
    MBOOL twinWarp = mPipeUsage.supportWarpNode(SFN_HINT::TWIN);

    if( !twinWarp )                            type = SFN_TYPE::UNI;
    else if( mSFNHint == SFN_HINT::RECORD )    type = SFN_TYPE::TWIN_R;
    else if( mSFNHint == SFN_HINT::PREVIEW )   type = SFN_TYPE::TWIN_P;
    else                                       type = SFN_TYPE::BAD;
    return type;
}

MBOOL WarpNode::onInit()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC(mLog, "onInit +");
    MBOOL ret = MTRUE;
    StreamingFeatureNode::onInit();
    enableDumpMask(sFilterTable);
    mSFNType = SFN_TYPE(getSFNType());
    mLog = mSFNType.Type2Name();
    mEndTimeDecrease = getPropertyValue(KEY_DEBUG_WARP_ENDTIME_DEC, VAL_DEBUG_ENDTIME_DEC);
    mDoneMsg = ( mSFNType.isTwinP() && mPipeUsage.supportWarpNode(SFN_HINT::RECORD) ) ? FeaturePipeParam::MSG_DISPLAY_DONE : FeaturePipeParam::MSG_FRAME_DONE;
    mFPSBase = mPipeUsage.supportWarpNode(SFN_HINT::TWIN) ? 2 : 1;

    if( mPipeUsage.supportWPE() )
    {
        mWarpStream = WPEWarpStream::createInstance();
    }
    else
    {
        mWarpStream = GPUWarpStream::createInstance();
    }

    if( mWarpStream == NULL )
    {
        MY_N_LOGE(mLog, "OOM: failed to create warp module");
        ret = MFALSE;
    }
    if( !mSFNType.isValid() )
    {
        MY_N_LOGE(mLog, "Invalid node type %s", mSFNType.Type2Name());
        ret = MFALSE;
    }
    mForceExpectMS = getPropertyValue("vendor.debug.fpipe.warp.expect", 0);
    mUseSharedInput = mPipeUsage.supportSharedInput();
    mDumpEISDisplay = getPropertyValue("vendor.debug.eis.dumpdisplay", 0) || mUsePerFrameSetting;
    TRACE_N_FUNC(mLog, "onInit -");
    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MBOOL WarpNode::onUninit()
{
    TRACE_N_FUNC_ENTER(mLog);
    mInputBufferPool = NULL;
    mOutputBufferPool = NULL;
    mJpegEncodeThread = NULL;
    if( mWarpStream )
    {
        mWarpStream->destroyInstance();
        mWarpStream = NULL;
    }
    mExtraInBufferNeeded = 0;
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL WarpNode::onThreadStart()
{
    TRACE_N_FUNC_ENTER(mLog);
    MBOOL ret = MTRUE;
    if( mWarpStream != NULL &&
        mInputBufferPool != NULL &&
        mOutputBufferPool != NULL )
    {
        MUINT32 inputPoolSize = mPipeUsage.getNumWarpInBuffer() + mPipeUsage.getNumDSDN20Buffer();
        MUINT32 outputPoolSize = mPipeUsage.getNumWarpOutBuffer();
        mExtraInBufferNeeded = mSFNType.isPreview() ? mPipeUsage.getNumExtraWarpInBuffer() : 0;
        Timer timer(MTRUE);
        if (mSFNType.isPreview()) {
            mInputBufferPool->allocate(inputPoolSize);
        }
        if (!mPipeUsage.supportWPE()) {
            mOutputBufferPool->allocate(outputPoolSize);
        } else {
            MY_N_LOGD(mLog, "Do not allocate warpout buffers for WPE DL mode");
        }
        mNodeSignal->setSignal(NodeSignal::SIGNAL_GPU_READY);
        MY_N_LOGD(mLog, "WarpNode %s (%d+%d) buf in %d ms, input pool size=%d", STR_ALLOCATE, inputPoolSize, outputPoolSize, timer.getNow(), mInputBufferPool->peakPoolSize());

        MY_N_LOGI(mLog, "init WarpStream ++");
        P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "WarpStream Init");
        ret = mWarpStream->init(mSensorIndex, mTimerIndex, mInputBufferPool->getImageSize(), MAX_WARP_SIZE);
        P2_CAM_TRACE_END(TRACE_ADVANCED);
        MY_N_LOGI(mLog, "init WarpStream --");
    }
    TRACE_N_FUNC(mLog, "onThreadStart -");
    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MBOOL WarpNode::onThreadStop()
{
    TRACE_N_FUNC_ENTER(mLog);
    this->waitWarpStreamBaseDone();
    if( mWarpStream != NULL )
    {
        mWarpStream->uninit();
    }
    clearTSQ();
    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL WarpNode::onThreadLoop()
{
    TRACE_N_FUNC(mLog, "Waitloop");
    RequestPtr request;
    BasicImgData inBuffer;
    std::queue<BasicImgData> warpMapQueue;
    WarpData warpData;

    TRACE_N_FUNC(mLog, "waitAllQueue +");
    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    TRACE_N_FUNC(mLog, "waitAllQueue -");
    if( !mInputImgDatas.deque(inBuffer) )
    {
        MY_N_LOGE(mLog, "InputImgData deque out of sync");
        return MFALSE;
    }
    if ( !mWarpData.deque(warpData))
    {
        MY_N_LOGE(mLog, "warpMapData deque out of sync");
        return MFALSE;
    }
    if( warpData.mRequest != inBuffer.mRequest )
    {
        MY_N_LOGE(mLog, "Request out of sync");
        return MFALSE;
    }

    request = warpData.mRequest;
    MBOOL hasWarp = warpData.mData;

    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    TRACE_N_FUNC_ENTER(mLog);

    request->mTimer.startWarp(mTimerIndex);
    markNodeEnter(request);
    TRACE_N_FUNC(mLog, "Frame %d in WarpNode needWarp=%d hasWarp(%d) inBuffer(%p)",
        request->mRequestNo, request->needWarp(), hasWarp, inBuffer.mData.mBuffer.get());

    MUINT32 cycleMs = getMaxCycleTimeMs(request);
    MUINT32 fps = request->getAppFPS()*mFPSBase;

    WarpSrcInfo srcInfo = extractSrc(request, inBuffer.mData);

    MBOOL needRun = hasWarp && srcInfo.mNeedWarp;
    MBOOL ret = needRun && processWarp(request, warpData.mData, srcInfo, cycleMs, fps, warpData.mData.mSyncCtrlData);
    warpData.mData.mSyncCtrlData.notify(mSyncCtrlSignal, request->mRequestNo);
    if( !ret )
    {
        TRACE_N_FUNC(mLog, "Drop warp (req:%d hasWarp:%d needWarp:%d QAction=%d)", request->mRequestNo, hasWarp, srcInfo.mNeedWarp, request->getEISQAction());
        if( mSFNType.isRecord() && request->hasRecordOutput() )
        {
            request->setVar<MBOOL>(SFP_VAR::EIS_SKIP_RECORD, MTRUE);
        }
        handleResultData(request);
    }
    inBuffer.mData.mBuffer = NULL;

    if( request->needWarp() )
    {
        tryAllocateExtraInBuffer();
    }

    TRACE_N_FUNC_EXIT(mLog);
    return MTRUE;
}

MVOID WarpNode::tryAllocateExtraInBuffer()
{
    TRACE_N_FUNC_ENTER(mLog);
    TRACE_N_FUNC(mLog, "FullImgPool=(%d/%d) ExtraNeeded=%d", mInputBufferPool->peakAvailableSize(), mInputBufferPool->peakPoolSize(), mExtraInBufferNeeded);

    while( (mExtraInBufferNeeded > 0) &&
           (!this->peakAllQueue()) &&
           (!mNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH)) )
    {
        mInputBufferPool->allocate();
        mExtraInBufferNeeded--;
        MY_N_LOGD(mLog, "Extra in buffer allocation remain = %d", mExtraInBufferNeeded);
    }
    TRACE_N_FUNC_EXIT(mLog);
}

MSize WarpNode::toWarpOutSize(const MSizeF &inSize)
{
    TRACE_FUNC_ENTER();
    MSize outSize = MSize(static_cast<int>(inSize.w) & ~(0x1), static_cast<int>(inSize.h) & ~(0x1));
    TRACE_FUNC("inSize(%f,%f),outSize(%dx%d)", inSize.w, inSize.h, outSize.w, outSize.h);
    TRACE_FUNC_EXIT();
    return outSize;
}

ImgBuffer WarpNode::prepareOutBuffer(const WarpSrcInfo & /*src*/, const WarpDstInfo &dst, const MSizeF &warpOutSize)
{
    TRACE_N_FUNC_ENTER(mLog);
    ImgBuffer outBuffer;

    if( dst.mNeedTempOut )
    {
        if (!mPipeUsage.supportWPE()) {
            MSize size = toWarpOutSize(warpOutSize);
            outBuffer = mOutputBufferPool->requestIIBuffer();
            outBuffer->getImageBuffer()->setExtParam(size);
        }
    }
    else
    {
        if( dst.mOutputBuffer )
        {
            outBuffer = new IIBuffer_IImageBuffer(dst.mOutputBuffer);
        }
        else
        {
            MY_N_LOGE(mLog, "Missing record buffer");
        }
    }

    TRACE_N_FUNC_EXIT(mLog);
    return outBuffer;
}

MBOOL WarpNode::needTempOutBuffer(const RequestPtr &request)
{
    TRACE_N_FUNC_ENTER(mLog);
    MBOOL ret = MTRUE;
    if( mPipeUsage.supportWPE() )
    {
        ret = MTRUE;
    }
    else if( request->useDirectGpuOut() )
    {
        IImageBuffer *recordBuffer = request->getRecordOutputBuffer();
        if( recordBuffer && getGraphicBufferAddr(recordBuffer) )
        {
            ret = MFALSE;
        }
    }
    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

#if 0
MVOID WarpNode::applyDZ(const RequestPtr &request, WarpParam &param)
{
    MRectF cropRect;
    OutputInfo info;
    if( request->getOutputInfo(IO_TYPE_RECORD, info) ||
        request->getOutputInfo(IO_TYPE_DISPLAY, info) )
    {
        cropRect = info.mCropRect;
    }
    else
    {
        cropRect.s = MSizeF(param.mWarpOut->getImageBuffer()->getImgSize().w,
                            param.mWarpOut->getImageBuffer()->getImgSize().h);
    }

    TRACE_N_FUNC(mLog, "mIn=%dx%d, OutSize=(%dx%d), rect=(%d,%d)(%dx%d), offset=(%fx%f)",
        param.mInSize.w, param.mInSize.h,
        param.mOutSize.w, param.mOutSize.h,
        cropRect.p.x, cropRect.p.y,
        cropRect.s.w, cropRect.s.h,
        param.mWarpMap.mDomainOffset.x, param.mWarpMap.mDomainOffset.y );

    WarpBase::applyDZWarp(param.mWarpMap.mBuffer, param.mInSize, param.mOutSize,
                          cropRect, MSizeF(param.mWarpMap.mDomainOffset.x, param.mWarpMap.mDomainOffset.y)/*MPointF is better?*/);
}
#endif

MVOID WarpNode::waitSync(const RequestPtr &request, const SyncCtrlData &syncCtrlData)
{
    TRACE_N_FUNC_ENTER(mLog);
    request->mTimer.startWarpWait(mTimerIndex);
    mWaitFrame = syncCtrlData.getWaitFrame(mSyncCtrlWait, request->mRequestNo);
    syncCtrlData.wait(mSyncCtrlWait, request->mRequestNo);
    mWaitFrame = 0;
    request->mTimer.stopWarpWait(mTimerIndex);
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::enqueWarpStream(const WarpParam &param, const WarpEnqueData &data, const SyncCtrlData &syncCtrlData)
{
    TRACE_N_FUNC_ENTER(mLog);
    String8 str(" ");
    str += String8::format("in(%dx%d),out(%dx%d),warpout(%fx%f),mdp(size=%zu):",
                           param.mInSize.w, param.mInSize.h,
                           param.mOutSize.w, param.mOutSize.h,
                           param.mWarpOutSize.w, param.mWarpOutSize.h,
                           param.mMDPOut.size());
    for( auto &out : param.mMDPOut )
    {
        str += String8::format("crop(%f,%f)(%fx%f).",
            out.mCropRect.p.x, out.mCropRect.p.y, out.mCropRect.s.w, out.mCropRect.s.h);
    }
    waitSync(data.mRequest, syncCtrlData);
    MY_N_LOGD(mLog, "Sensor(%d) Frame(%d) sync wait done(%d ms), warp enque start: %s", mSensorIndex, data.mRequest->mRequestNo, data.mRequest->mTimer.getElapsedWarpWait(mTimerIndex), str.string());
    data.mRequest->mTimer.startEnqueWarp(mTimerIndex);

    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_WPE));
    this->incExtThreadDependency();
    this->enqueWarpStreamBase(mWarpStream, param, data);
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::onWarpStreamBaseCB(const WarpParam &param, const WarpEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing WarpNode class members
    TRACE_N_FUNC_ENTER(mLog);

    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_N_LOGE(mLog, "Missing request");
    }
    else
    {
        request->mTimer.stopEnqueWarp(mTimerIndex);
        MY_N_LOGD(mLog, "sensor(%d) Frame %d warp enque done in %d ms, isPreview=%d result=%d msg=%d support(p:%d r:%d)",
                         mSensorIndex, request->mRequestNo, request->mTimer.getElapsedEnqueWarp(mTimerIndex), mSFNType.isPreview(), param.mResult,
                         mDoneMsg, mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW), mPipeUsage.supportWarpNode(SFN_HINT::RECORD));
        CAM_ULOGM_ASSERT(param.mResult, "Frame %d warp enque result failed", request->mRequestNo);

        request->updateResult(param.mResult);

        if( data.mNeedDump || request->needDump() )
        {
            dump(data, param.mMDPOut, mSFNType.isPreview());
        }

        handleResultData(request);
        request->mTimer.stopWarp(mTimerIndex);
    }
    this->decExtThreadDependency();
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::handleResultData(const RequestPtr &request)
{
    TRACE_N_FUNC_ENTER(mLog);
    markNodeExit(request);
    handleData(ID_WARP_TO_HELPER, HelperData(mDoneMsg, request));
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::dump(const WarpEnqueData &data, const MDPWrapper::P2IO_OUTPUT_ARRAY &mdpout, MBOOL isPreview) const
{
    TRACE_N_FUNC_ENTER(mLog);
    if( allowDump(getMaskIndex(FILTER_WARP_MAP, isPreview)) )
    {
        dumpData(data.mRequest, data.mWarpMap.mBuffer, "warp_map-%s", mSFNType.getDumpPostfix());
    }
    if( allowDump(getMaskIndex(FILTER_WARP_IN, isPreview)) )
    {
        dumpData(data.mRequest, data.mSrc, "warp_in-%s", mSFNType.getDumpPostfix());
    }
    if( allowDump(getMaskIndex(FILTER_WARP_OUT, isPreview)) )
    {
        dumpData(data.mRequest, data.mDst, "warp_out-%s", mSFNType.getDumpPostfix());
    }
    if( allowDump(getMaskIndex(FILTER_MDP_OUT, isPreview)) )
    {
        unsigned i = 0;
        for( const auto &out : mdpout )
        {
            dumpData(data.mRequest, out.mBuffer, "warp_mdp_out_%d-%s", i, mSFNType.getDumpPostfix());
            ++i;
        }
    }
    TRACE_N_FUNC_EXIT(mLog);
}

MVOID WarpNode::dumpAdditionalInfo(android::Printer &printer) const
{
    TRACE_N_FUNC_ENTER(mLog);
    if( mSyncCtrlWait )
    {
        printer.printFormatLine("[FPipeStatus] %s syncCtrl waitFrame=%u", mLog, mWaitFrame);
    }
    TRACE_N_FUNC_EXIT(mLog);
}

WarpSrcInfo WarpNode::extractSrc(const RequestPtr &request, const BasicImg &inputImg)
{
    TRACE_N_FUNC_ENTER(mLog);
    WarpSrcInfo src;

    src.mID = request->mRequestNo;
    src.mValid = MTRUE;
    src.mNeedWarp = request->needWarp();
    src.mP2Pack = request->mP2Pack;

    if( src.mNeedWarp )
    {
        src.mInput = inputImg;
        OutputInfo info;

        if( (mSFNType.isRecord() && request->getOutputInfo(IO_TYPE_RECORD, info)) ||
            (mSFNType.isPreview() && request->getOutputInfo(IO_TYPE_DISPLAY, info)) )
        {
            src.mCrop = info.mCropRect;
        }
        else
        {
            MY_S_LOGW_IF(request->mLog.getLogLevel(), request->mLog, "%s Can not find crop !!", mLog);
        }

        if( mSFNType.isRecord() )
        {
            std::shared_ptr<AdvPQCtrl> recPQ;
            recPQ = AdvPQCtrl::clone(src.mInput.mPQCtrl, "s_wpe_rec");
            recPQ->setIs2ndDRE(MTRUE);
            src.mInput.mPQCtrl = recPQ;
            if( mPipeUsage.supportEIS_TSQ() )
            {
                EISQ_ACTION action = request->getEISQAction();
                src = processEISQAction<WarpSrcInfo>(action, src, mWarpSrcInfoQueue);
            }
            TRACE_N_FUNC(mLog, "Frame ReqNo:%d RecNo=%d: rqq=%d tsq=%d previewWarpNode=%d recordWarpNode=%d qAction=%d qCounter=%d WarpSrcInfoQueue=%zu src=%d", request->mRequestNo, request->mRecordNo, mPipeUsage.supportEIS_RQQ(), mPipeUsage.supportEIS_TSQ(), mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW), mPipeUsage.supportWarpNode(SFN_HINT::RECORD), request->getEISQAction(), request->getEISQCounter(), mWarpSrcInfoQueue.size(), src.mID);
        }
    }
    else if( mWarpSrcInfoQueue.size() )
    {
        MY_N_LOGD(mLog, "clearTSQ (ReqNo:%d RecNo=%d tsq=%d qAction=%d WarpSrcInfoQueue=%zu src=%d)", request->mRequestNo, request->mRecordNo, mPipeUsage.supportEIS_TSQ(), request->getEISQAction(), mWarpSrcInfoQueue.size(), src.mID);
        clearTSQ();
    }

    TRACE_N_FUNC_EXIT(mLog);
    return src;
}

WarpDstInfo WarpNode::prepareDst(const RequestPtr &request)
{
    TRACE_N_FUNC_ENTER(mLog);
    WarpDstInfo dst;
    dst.mID = request->mRequestNo;
    dst.mNeedTempOut = needTempOutBuffer(request);
    dst.mOutputBuffer = request->getRecordOutputBuffer();

    TRACE_N_FUNC_EXIT(mLog);
    return dst;
}

MDPWrapper::P2IO_OUTPUT_ARRAY WarpNode::prepareMDPParam(const RequestPtr &request, const WarpSrcInfo &src, const WarpDstInfo & /*dst*/, const WarpImg &warpMap, WarpEnqueData &data, const DomainTransform &transform)
{
    (void)data;
    TRACE_N_FUNC_ENTER(mLog);

    MBOOL skipDisplay = request->skipMDPDisplay();
    MBOOL useWarpRecord = request->useDirectGpuOut();
    MDPWrapper::P2IO_OUTPUT_ARRAY outList;
    P2IO out;
    std::vector<P2IO> extOuts;
    MRectF displayCrop;

    IAdvPQCtrl_const pqCtrl = src.mInput.mPQCtrl;

    if( !skipDisplay )
    {
        if (request->popDisplayOutput(this, out) && out.isValid())
        {
            MRectF orignalCrop = out.mCropRect;
            MRectF newCrop = transform.applyTo(orignalCrop);
            out.mCropRect = refineMDPCrop(newCrop, warpMap.mInputCrop.s, request);
            displayCrop = out.mCropRect;
            out.mPQCtrl = pqCtrl;
            out.mBuffer->setTimestamp(src.mInput.mBuffer->getImageBuffer()->getTimestamp());
            outList.push_back(out);

            TRACE_N_FUNC(mLog, "[Display]MDP Crop before transform=" MCropF_STR ",after=" MCropF_STR,
                       MCropF_ARG(orignalCrop), MCropF_ARG(out.mCropRect));
        }

        if (request->popExtraOutputs(this, extOuts))
        {
            for (size_t i = 0; i < extOuts.size(); i++)
            {
                P2IO extOut = extOuts[i];
                MRectF orignalCrop = extOut.mCropRect;
                MRectF newCrop = transform.applyTo(orignalCrop);
                extOut.mCropRect = refineMDPCrop(newCrop, warpMap.mInputCrop.s, request);
                extOut.mPQCtrl = pqCtrl;
                outList.push_back(extOut);
                if(extOut.isValid())
                {
                    extOut.mBuffer->setTimestamp(src.mInput.mBuffer->getImageBuffer()->getTimestamp());
                }
                else
                {
                    MY_LOGF("Ext output %zu is NULL!", i);
                }

                TRACE_N_FUNC(mLog, "[Extra]MDP Crop before transform=" MCropF_STR ",after=" MCropF_STR,
                           MCropF_ARG(orignalCrop), MCropF_ARG(extOut.mCropRect));
            }
        }
    }

    if( !useWarpRecord
        && request->popRecordOutput(this, out) && out.isValid())
    {
        MRectF newCrop = transform.applyTo(src.mCrop); // Get queued crop from srcInfo
        out.mCropRect = refineMDPCrop(newCrop, warpMap.mInputCrop.s, request);
        out.mPQCtrl = pqCtrl;
        out.mType = P2IO::TYPE_RECORD;
        out.mBuffer->setTimestamp(src.mInput.mBuffer->getImageBuffer()->getTimestamp());
        outList.push_back(out);

        TRACE_N_FUNC(mLog, "[Record]MDP Crop before transform=" MCropF_STR ",after=" MCropF_STR,
                   MCropF_ARG(src.mCrop), MCropF_ARG(out.mCropRect));
    }

    if( outList.empty() )
    {
        MY_N_LOGW(mLog, "No avalible MDP buffer");
    }

    TRACE_N_FUNC_EXIT(mLog);
    return outList;
}

WarpParam WarpNode::prepareWarpParam(const RequestPtr &request, const WarpSrcInfo &src, WarpDstInfo &dst,
    const WarpImg &warpMap, WarpEnqueData &data, MUINT32 cycleMs, MUINT32 scenarioFPS, MBOOL bypass)
{
    TRACE_N_FUNC_ENTER(mLog);
    if(MSizeF(src.mInput.mBuffer->getImgSize()) != warpMap.mInputSize)
    {
        MSize inSize = src.mInput.mBuffer->getImgSize();
        MY_S_LOGE(request->mLog, "WarpMap ERROR!, warpTargetInput(" MSizeF_STR ") != input(" MSize_STR "), warpCrop(" MCropF_STR ")",
                    MSizeF_ARG(warpMap.mInputSize), MSize_ARG(inSize), MCropF_ARG(warpMap.mInputCrop));
    }

    WarpParam param;
    param.mIn = src.mInput.mBuffer;
    param.mInSize = param.mIn->getImageBuffer()->getImgSize();

    param.mWarpOutSize = warpMap.mInputCrop.s;
    if (!mPipeUsage.supportWPE()) {
        param.mWarpOut = prepareOutBuffer(src, dst, param.mWarpOutSize);
        param.mOutSize = param.mWarpOut->getImageBuffer()->getImgSize();
    }

    param.mByPass = bypass;
    param.mWarpMap = warpMap;
    param.mEndTimeMs = cycleMs;
    param.mScenarioFPS = scenarioFPS;

    DomainTransform transform = src.mInput.mTransform;
    if(!bypass)
    {
        const char* str = mSFNType.isPreview() ? "warp-dis" : "warp-rec";
        transform.accumulate(str, request->mLog, warpMap.mInputSize, warpMap.mInputCrop, warpMap.mInputCrop.s);
    }

    param.mMDPOut = prepareMDPParam(request, src, dst, warpMap, data, transform);

    TRACE_N_FUNC(mLog, "In(%dx%d),Out(%dx%d),WarpOut(%fx%f),transform=" MTransF_STR,
               param.mInSize.w, param.mInSize.h,
               param.mOutSize.w, param.mOutSize.h,
               param.mWarpOutSize.w, param.mWarpOutSize.h,
               MTransF_ARG(transform.mOffset, transform.mScale));
    TRACE_N_FUNC_EXIT(mLog);
    return param;
}

MVOID WarpNode::processEISNDD(const RequestPtr &request, const WarpParam &param, const WarpSrcInfo &srcInfo)
{
    EISSourceDumpInfo info = request->getEISDumpInfo();
    if (request->getEISDumpInfo().dumpIdx == 1)
    {
        mJpegEncodeThread = NULL;
        mJpegEncodeThread = JpegEncodeThread::getInstance(param.mMDPOut[0].mCropRect.s, info.filePath.c_str());
    }
    if (info.markFrame)
    {
        mJpegEncodeThread->compressJpeg(srcInfo.mInput.mBuffer, true);
    }
    else
    {
        mJpegEncodeThread->compressJpeg(srcInfo.mInput.mBuffer, false);
    }
}

MBOOL WarpNode::processWarp(const RequestPtr &request, const WarpImg &warpMap, const WarpSrcInfo &srcInfo,
    MUINT32 cycleMs, MUINT32 scenarioFPS, const SyncCtrlData &syncCtrlData)
{
    TRACE_N_FUNC_ENTER(mLog);
    MBOOL ret = MTRUE;
    WarpEnqueData data;
    WarpParam param;

    TRACE_N_FUNC(mLog, "processWarp #%d src=%d isPreview=%d needWarp=%d valid=%d warpMap=%p cycleMs=%u fps=%u", request->mRequestNo, srcInfo.mID, mSFNType.isPreview(), srcInfo.mNeedWarp, srcInfo.mValid, warpMap.mBuffer.get(), cycleMs, scenarioFPS);

    // zzz warpDomain should be also queue
    WarpDstInfo dstInfo = prepareDst(request);
    param = prepareWarpParam(request, srcInfo, dstInfo, warpMap, data, cycleMs, scenarioFPS, request->useWarpPassThrough());
    param.mRequest = data.mRequest = request;
    data.mSrc = param.mIn;
    data.mDst = param.mWarpOut;
    data.mWarpMap = param.mWarpMap;
    data.mSyncCtrlData = syncCtrlData;
    MBOOL dumpEIS = mSFNType.isPreview() ? mDumpEISDisplay : !mDumpEISDisplay;

    if ( dumpEIS && request->getEISDumpInfo().dumpIdx >= 1 )
    {
        processEISNDD(request, param, srcInfo);
    }

    if( param.mByPass )
    {
        WarpBase::makePassThroughWarp(param.mWarpMap.mBuffer, param.mInSize, param.mOutSize);
    }

    if( request->useDirectGpuOut() )
    {
        //applyDZ(request, param);
        MY_N_LOGE(mLog, "Currently not support warp direct GPU out !!!!! ERROR!!!");
    }

    enqueWarpStream(param, data, syncCtrlData);

    TRACE_N_FUNC_EXIT(mLog);
    return ret;
}

MRectF WarpNode::refineMDPCrop(const MRectF &crop, const MSizeF &warpCropSize, const RequestPtr &request)
{
    MRectF outCrop = crop;
    MSize warpOutSize = toWarpOutSize(warpCropSize);

    // transform to warpOut domain
    {
        outCrop.s.w *= warpOutSize.w / warpCropSize.w;
        outCrop.s.h *= warpOutSize.h / warpCropSize.h;

    }
    if( outCrop.s.w != crop.s.w || outCrop.s.h != crop.s.h) {
        MY_LOGD_IF(request->needPrintIO(),
            "newCrop (%f,%f)(%fx%f) != srcCrop(%f,%f)(%fx%f)",
            outCrop.p.x, outCrop.p.y, outCrop.s.w, outCrop.s.h,
            crop.p.x, crop.p.y, crop.s.w, crop.s.h);
    }

    refineBoundaryF("WarpNode", warpOutSize, outCrop, request->needPrintIO());
    return outCrop;
}

MUINT32 WarpNode::getMaxCycleTimeMs(const RequestPtr &request)
{
    MUINT32 cycle = ( mPipeUsage.supportWarpNode(SFN_HINT::TWIN) ) ? 2 : 1;
    MUINT32 expectMs = (mForceExpectMS > 0) ? mForceExpectMS : request->getNodeCycleTimeMs();
    MUINT32 maxTimeMs = expectMs / std::max<MUINT32>(1, cycle);
    maxTimeMs = maxTimeMs - mEndTimeDecrease;
    MY_S_LOGD_IF(request->mLog.getLogLevel(), request->mLog, "expect(%d) cycle(%d) cycleMs(%d)", expectMs, cycle, maxTimeMs);
    return maxTimeMs;
}

// ToDo WarpMDP timer

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
