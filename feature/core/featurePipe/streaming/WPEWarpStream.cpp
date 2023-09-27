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

#include "WPEWarpStream.h"
#include "StreamingFeature_Common.h"
#include "DebugControl.h"
#define PIPE_CLASS_TAG "WPEWarpStream"
#define PIPE_TRACE TRACE_WPE_WARP_STREAM_BASE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam3/feature/utils/p2/P2Util.h>

#include <time.h>
#include <unistd.h>
#include <cstdlib>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

#define WPE_NORMAL_STREAM_NAME "WPEWarpStream"

#define PORTID_IN 0
#define PORTID_OUT 1

using NSCam::NSIoPipe::ExtraParam;
using NSCam::NSIoPipe::Input;
using NSCam::NSIoPipe::Output;
using NSCam::NSIoPipe::MCrpRsInfo;
using NSCam::NSIoPipe::MCropRect;
using NSCam::NSIoPipe::EPortType_Memory;
using NSCam::NSIoPipe::PortID;
using NSCam::NSIoPipe::EStreamPipeID_WarpEG;
using NSImageio::NSIspio::EPortIndex_WPEI;
using NSImageio::NSIspio::EPortIndex_WPEO;
using NSImageio::NSIspio::EPortIndex_WDMAO;
using NSImageio::NSIspio::EPortIndex_WROTO;
using NSCam::NSIoPipe::EPIPE_WPE_INFO_CMD;
using NSCam::NSIoPipe::NSWpe::WPE_MODE_WPEO;
using NSCam::NSIoPipe::NSWpe::WPE_MODE_MDP;
using NSCam::NSIoPipe::NSWpe::WPE_BUS_SIZE_32_BITS;

using NSCam::NSIoPipe::PORT_WDMAO;
using NSCam::NSIoPipe::PORT_WROTO;

using namespace ::NSCam::Feature::P2Util;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

WPEWarpStream* WPEWarpStream::createInstance()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return new WPEWarpStream();
}

WPEWarpStream::WPEWarpStream()
    : mDIPStream(NULL)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

WPEWarpStream::~WPEWarpStream()
{
    TRACE_FUNC_ENTER();
    this->uninit();
    TRACE_FUNC_EXIT();
}

MBOOL WPEWarpStream::init(const MUINT32 &sensorIdx, const MUINT32 &timerIdx, const MSize &maxImageSize, const MSize &maxWarpSize)
{
    (void)timerIdx;
    TRACE_FUNC_ENTER();
    (void)(maxImageSize);
    (void)(maxWarpSize);
    MBOOL ret = MFALSE;
    MY_LOGI("create & init DIPStream ++");
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "WpeWarpStream:init");
    if( mDIPStream == NULL )
    {
        mDIPStream = DIPStream::createInstance(sensorIdx);
        if( mDIPStream == NULL )
        {
            MY_LOGE("Cannot create DIPStream for WPE");
        }
        else
        {
            ret = mDIPStream->init(WPE_NORMAL_STREAM_NAME, EStreamPipeID_WarpEG);
        }
    }
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    MY_LOGI("create & init DIPStream --");

#if USE_WPE_STAND_ALONE
    if( mProcessThread == NULL )
    {
        mProcessThread = new ProcessThread(timerIdx);
        mProcessThread->run("Cam@fpipe.wpe.mdp");
    }
#endif

    TRACE_FUNC_EXIT();
    return ret;
}

MVOID WPEWarpStream::uninit()
{
    TRACE_FUNC_ENTER();
    if( mDIPStream )
    {
        this->waitDIPStreamBaseDone();
        mDIPStream->uninit(WPE_NORMAL_STREAM_NAME);
        mDIPStream->destroyInstance();
        mDIPStream = NULL;
    }

    if( mProcessThread != NULL )
    {
        mProcessThread->signalStop();
        mProcessThread->join();
        mProcessThread = NULL;
    }

    TRACE_FUNC_EXIT();
}

MBOOL WPEWarpStream::enque(WarpParam warpParam)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( mDIPStream )
    {
        if( mProcessThread != NULL && warpParam.mByPass )
        {
            warpParam.mResult = MTRUE;
            mProcessThread->threadEnque(warpParam);
        }
        else
        {
            DIPParams dipParam;
            WPEEnqueData data(warpParam);
            data.mWPEObj = std::make_shared<WPEObj>();
            prepareDIPParams(dipParam, data);
            this->enqueDIPStreamBase(mDIPStream, dipParam, data);
        }

        ret = MTRUE;
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MVOID WPEWarpStream::prepareDIPParams(DIPParams &param, WPEEnqueData &data)
{
    TRACE_FUNC_ENTER();
    checkMDPOut(data);

    DIPFrameParams fparam;
    setBasic(fparam, data.mWarpParam);
    setInBuffer(fparam, data.mWarpParam);
    setWPEDIPParams(fparam, data.mWPEObj->mWPEQParam, data.mWarpParam);

#if USE_WPE_STAND_ALONE
    setStandaloneOutput(fparam, data.mWarpParam);
    param.mvDIPFrameParams.push_back(fparam);
#else // USE_WPE_STAND_ALONE
    adjustCropSetting(data.mWarpParam);
    P2IOClassfier collect(data.mWarpParam.mMDPOut);
    while( collect.hasIO() )
    {
        DIPFrameParams f = fparam;
        P2IOClassfier::Pair pair = collect.popOnePair();
        std::shared_ptr<PQObj> pqObj = std::make_shared<PQObj>();
        pushIO(f, pair.mWDMAO, PORT_WDMAO, CROP_WDMAO);
        pushIO(f, pair.mWROTO, PORT_WROTO, CROP_WROTO);
        pushPQ(f, data, pqObj, pair.mWDMAO, pair.mWROTO);
        param.mvDIPFrameParams.push_back(f);
    }
#endif // USE_WPE_STAND_ALONE

    Feature::P2Util::updateExpectEndTime(param, data.mWarpParam.mEndTimeMs);
    Feature::P2Util::updateScenario(param, data.mWarpParam.mScenarioFPS, data.mWarpParam.mInSize.w);
    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::checkMDPOut(WPEEnqueData &data)
{
    // TODO: record buffer handling
#if !USE_WPE_STAND_ALONE
    // create dummy buffer
    if( !data.mWarpParam.mMDPOut.size() )
    {
        MSize dummySize = data.mWarpParam.mOutSize;
        data.mWarpParam.mWarpOut->getImageBuffer()->setExtParam(dummySize);

        Output output;
        output.mPortID = PortID(EPortType_Memory, NSImageio::NSIspio::EPortIndex_WDMAO, PORTID_OUT);
        output.mBuffer = data.mWarpParam.mWarpOut->getImageBufferPtr();

        MCrpRsInfo cropRect;
        cropRect.mCropRect = MCropRect(dummySize.w, dummySize.h);
        data.mWarpParam.mMDPOut.push_back(P2IO(output, cropRect));
        MY_LOGD("No record buffer, use dummy");
    }
#endif
}

MVOID WPEWarpStream::setBasic(DIPFrameParams &fparam, const WarpParam &warpParam)
{
    TRACE_FUNC_ENTER();
    const RequestPtr &request = warpParam.mRequest;
    fparam.UniqueKey = request->mRequestNo;
    fparam.mSensorIdx = request->getMasterID();
    fparam.mStreamTag = NSIoPipe::NSPostProc::ENormalStreamTag_Normal;

    fparam.FrameNo = request->mP2Pack.getFrameData().mMWFrameNo;
    fparam.RequestNo = request->mP2Pack.getFrameData().mMWFrameRequestNo;
    fparam.Timestamp = request->mP2Pack.getSensorData().mP1TS;
    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::setInBuffer(DIPFrameParams &fparam, const WarpParam &warpParam)
{
    TRACE_FUNC_ENTER();

    Input input;
    input.mPortID = PortID(EPortType_Memory, EPortIndex_WPEI, PORTID_IN);
    input.mBuffer = warpParam.mIn->getImageBufferPtr();
    fparam.mvIn.push_back(input);
    TRACE_FUNC("PortID index(%d), in/out(%d)", input.mPortID.index, input.mPortID.inout);

    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::setStandaloneOutput(DIPFrameParams &fparam, const WarpParam &warpParam)
{
    (void)fparam;
    (void)warpParam;
    TRACE_FUNC_ENTER();
#if USE_WPE_STAND_ALONE
    Output output;
    output.mPortID = PortID(EPortType_Memory, EPortIndex_WPEO, PORTID_OUT);
    output.mBuffer = warpParam.mWarpOut->getImageBufferPtr(); // WPEO
    fparam.mvOut.push_back(output);
    TRACE_FUNC("PortID index(%d), in/out(%d)", output.mPortID.index, output.mPortID.inout);
#endif // USE_WPE_STAND_ALONE
    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::adjustCropSetting(WarpParam &warpParam)
{
    MSize wpeOutSize = toWPEOutSize(warpParam);
    for( P2IO &io : warpParam.mMDPOut )
    {
        io.mCropRect = toWPEScaleCrop(wpeOutSize, io.mCropRect, warpParam.mRequest->needPrintIO());
        io.mDMAConstrain = Feature::P2Util::DMAConstrain::NONE;
    }
}

MBOOL WPEWarpStream::pushIO(DIPFrameParams &frame, const P2IO &io, const PortID &portID, MUINT32 cropID)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = io.isValid();
    if( ret )
    {
        Feature::P2Util::push_out(frame, portID, io);
        if( io.isCropValid() )
        {
            Feature::P2Util::push_crop(frame, cropID, io.mCropRect, io.mCropDstSize, io.mDMAConstrain);
        }
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL WPEWarpStream::pushPQ(DIPFrameParams &frame, WPEEnqueData &data, const std::shared_ptr<PQObj> &pqObj, const P2IO &wdmao, const P2IO &wroto)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    if( wdmao.isValid() )
    {
        pqObj->setWdmaPQ(makeDpPqParam(wdmao.mPQCtrl.get(), wdmao.mType, wdmao.mBuffer, PIPE_CLASS_TAG));
    }
    if( wroto.isValid() )
    {
        pqObj->setWrotPQ(makeDpPqParam(wroto.mPQCtrl.get(), wroto.mType, wroto.mBuffer, PIPE_CLASS_TAG));
    }
    Feature::P2Util::push_PQParam(frame, pqObj->getPQParamPtr());
    data.mPQObjList.push_back(pqObj);
    TRACE_FUNC_EXIT();
    return ret;
}

MVOID WPEWarpStream::setWPEDIPParams(DIPFrameParams &fparam, WPEQParams &wpeQParam, const WarpParam &warpParam)
{
    TRACE_FUNC_ENTER();

    setWPEMode(wpeQParam);
    setWPECrop(wpeQParam, warpParam);
    setWPEBuffer(wpeQParam, warpParam);

    ExtraParam extraParam;
    extraParam.CmdIdx = EPIPE_WPE_INFO_CMD;
    extraParam.moduleStruct = &wpeQParam;
    fparam.mvExtraParam.push_back(extraParam);

    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::setWPEMode(WPEQParams &wpeQParam)
{
    TRACE_FUNC_ENTER();
#if USE_WPE_STAND_ALONE
    wpeQParam.wpe_mode = NSCam::NSIoPipe::NSWpe::WPE_MODE_WPEO;
#else
    wpeQParam.wpe_mode = NSCam::NSIoPipe::NSWpe::WPE_MODE_MDP;
#endif
    wpeQParam.vgen_hmg_mode = 0; // no use z plane
    wpeQParam.tbl_sel_v = 24;
    wpeQParam.tbl_sel_h = 24;
    TRACE_FUNC("WPE Mode = (%d), z(%d)", wpeQParam.wpe_mode, wpeQParam.vgen_hmg_mode);

    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::setWPECrop(WPEQParams &wpeQParam, const WarpParam &warpParam)
{
    TRACE_FUNC_ENTER();

    // Source Crop
    MCrpRsInfo srcCrop;
    srcCrop.mCropRect = NSIoPipe::MCropRect(0, 0);
    wpeQParam.mwVgenCropInfo.push_back(srcCrop);

    // Target Crop
    MSize dstSize = toWPEOutSize(warpParam);
    wpeQParam.wpecropinfo.x_start_point = 0;
    wpeQParam.wpecropinfo.y_start_point = 0;
    wpeQParam.wpecropinfo.x_end_point = dstSize.w - 1;
    wpeQParam.wpecropinfo.y_end_point = dstSize.h - 1;
    TRACE_FUNC("WPE Target Crop = x_start(%d), y_start(%d), x_end(%d), y_end(%d)",
               wpeQParam.wpecropinfo.x_start_point,
               wpeQParam.wpecropinfo.y_start_point,
               wpeQParam.wpecropinfo.x_end_point,
               wpeQParam.wpecropinfo.y_end_point);

    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::setWPEBuffer(WPEQParams &wpeQParam, const WarpParam &warpParam)
{
    TRACE_FUNC_ENTER();

    warpParam.mWarpMap.mBuffer->getImageBuffer()->syncCache(eCACHECTRL_FLUSH);

    // x plane
    wpeQParam.warp_veci_info.width    = warpParam.mWarpMap.mBuffer->getImageBuffer()->getImgSize().w;
    wpeQParam.warp_veci_info.height   = warpParam.mWarpMap.mBuffer->getImageBuffer()->getImgSize().h;
    wpeQParam.warp_veci_info.stride   = warpParam.mWarpMap.mBuffer->getImageBuffer()->getBufStridesInBytes(0);
    wpeQParam.warp_veci_info.bus_size = NSCam::NSIoPipe::NSWpe::WPE_BUS_SIZE_32_BITS;
    wpeQParam.warp_veci_info.virtAddr = (MUINTPTR)((warpParam.mWarpMap.mBuffer->getImageBuffer())->getBufVA(0));
    wpeQParam.warp_veci_info.phyAddr  = (MUINTPTR)((warpParam.mWarpMap.mBuffer->getImageBuffer())->getBufPA(0));
    wpeQParam.warp_veci_info.fd = warpParam.mWarpMap.mBuffer->getImageBuffer()->getPlaneFD(0);
    wpeQParam.warp_veci_info.offset = warpParam.mWarpMap.mBuffer->getImageBuffer()->getPlaneOffsetInBytes(0);

    // y plane
    wpeQParam.warp_vec2i_info.width    = warpParam.mWarpMap.mBuffer->getImageBuffer()->getImgSize().w;
    wpeQParam.warp_vec2i_info.height   = warpParam.mWarpMap.mBuffer->getImageBuffer()->getImgSize().h;
    wpeQParam.warp_vec2i_info.stride   = warpParam.mWarpMap.mBuffer->getImageBuffer()->getBufStridesInBytes(1);
    wpeQParam.warp_vec2i_info.bus_size = NSCam::NSIoPipe::NSWpe::WPE_BUS_SIZE_32_BITS;
    wpeQParam.warp_vec2i_info.virtAddr = (MUINTPTR)((warpParam.mWarpMap.mBuffer->getImageBuffer())->getBufVA(1));
    wpeQParam.warp_vec2i_info.phyAddr  = (MUINTPTR)((warpParam.mWarpMap.mBuffer->getImageBuffer())->getBufPA(1));
    wpeQParam.warp_vec2i_info.fd = warpParam.mWarpMap.mBuffer->getImageBuffer()->getPlaneFD(1);
    wpeQParam.warp_vec2i_info.offset = warpParam.mWarpMap.mBuffer->getImageBuffer()->getPlaneOffsetInBytes(1);

    TRACE_FUNC("WPE Warp Info = Grid W (%d), H (%d), stride(X)=(%d), stride(Y)=(%d)",
               wpeQParam.warp_veci_info.width, wpeQParam.warp_veci_info.height,
               wpeQParam.warp_veci_info.stride, wpeQParam.warp_vec2i_info.stride);

    TRACE_FUNC_EXIT();
}

MSize WPEWarpStream::toWPEOutSize(const WarpParam &param)
{
    TRACE_FUNC_ENTER();
    //MSize size = USE_WPE_STAND_ALONE ? param.mInSize : param.mInSize;
    MSize size = MSize(param.mWarpOutSize.w, param.mWarpOutSize.h);
    MSize outSize = MSize(size.w & ~(0x1), size.h & ~(0x1));
    if( outSize.w != size.w || outSize.h != size.h ) {
        MY_LOGD_IF(param.mRequest->needPrintIO(),
                   "outSize(%dx%d) != size(%dx%d)",
                   outSize.w, outSize.h, size.w, size.h);
    }
    TRACE_FUNC_EXIT();
    return outSize;
}

MVOID WPEWarpStream::onDIPStreamBaseCB(const DIPParams &param, const WPEEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing WPEWarpStream class members
    TRACE_FUNC_ENTER();

    if( !param.mDequeSuccess )
    {
        MY_LOGW("Enque WPE NormalStream result failed");
    }

    WarpParam retData = data.mWarpParam;
    retData.mResult = param.mDequeSuccess;

    if( mProcessThread != NULL )
    {
        mProcessThread->threadEnque(retData);
    }
    else if( data.mWarpParam.mCallback )
    {
        data.mWarpParam.mCallback(retData);
    }
    else
    {
        MY_LOGE("Missing callback");
    }

    TRACE_FUNC_EXIT();
}

MRectF WPEWarpStream::toWPEScaleCrop(const MSize &wpeOutSize, const MRectF &srcCrop, MBOOL print)
{
    TRACE_FUNC_ENTER();

    MRectF newCrop = srcCrop;
    MSizeF dstSize = MSizeF(wpeOutSize.w, wpeOutSize.h);

    // Boundary check
    if( srcCrop.s.w > dstSize.w || srcCrop.s.h > dstSize.h )
    {
        newCrop.s.w = min(srcCrop.s.w, dstSize.w);
        newCrop.s.h = min(srcCrop.s.h, dstSize.h);

        MY_LOGD_IF(print,
            "WPEScaleCrop (%f,%f)(%fx%f) > WPEOutSize(%fx%f), force use new Crop = (%f,%f)(%fx%f)",
            srcCrop.p.x, srcCrop.p.y, srcCrop.s.w, srcCrop.s.h,
            dstSize.w, dstSize.h,
            newCrop.p.x, newCrop.p.y,
            newCrop.s.w, newCrop.s.h);
    }

    return newCrop;
#if 0
    MRectF wpeScaleCrop;
    wpeScaleCrop.p.x = srcCrop.p.x * param.mInSize.w / param.mWarpOutSize.w;
    wpeScaleCrop.p.y = srcCrop.p.y * param.mInSize.h / param.mWarpOutSize.h;
    wpeScaleCrop.s.w = srcCrop.s.w * param.mInSize.w / param.mWarpOutSize.w;
    wpeScaleCrop.s.h = srcCrop.s.h * param.mInSize.h / param.mWarpOutSize.h;

    // Boundary check
    MRectF newCrop = wpeScaleCrop;
    MSizeF dstSize = MSizeF(toWPEOutSize(param).w, toWPEOutSize(param).h);
    if( wpeScaleCrop.s.w > dstSize.w || wpeScaleCrop.s.h > dstSize.h )
    {
        newCrop.s.w = min(wpeScaleCrop.s.w, dstSize.w);
        newCrop.s.h = min(wpeScaleCrop.s.h, dstSize.h);
        MY_LOGD("WPEScaleCrop (%f,%f)(%fx%f) > WPEOutSize(%fx%f), force use new Crop = (%f,%f)(%fx%f)",
                wpeScaleCrop.p.x, wpeScaleCrop.p.y, wpeScaleCrop.s.w, wpeScaleCrop.s.h,
                dstSize.w, dstSize.h,
                newCrop.p.x, newCrop.p.y,
                newCrop.s.w, newCrop.s.h);
    }

    TRACE_FUNC("inSize(%dx%d),warpOutSize(%fx%f),WPEScaleCrop=(%f,%f)(%fx%f)=>(%f,%f)(%fx%f)",
               param.mInSize.w, param.mInSize.h,
               param.mWarpOutSize.w, param.mWarpOutSize.h,
               srcCrop.p.x, srcCrop.p.y,
               srcCrop.s.w, srcCrop.s.h,
               newCrop.p.x, newCrop.p.y,
               newCrop.s.w, newCrop.s.h);

    TRACE_FUNC_EXIT();
    return newCrop;
#endif
}

WPEWarpStream::ProcessThread::ProcessThread(const MUINT32 &timerIdx)
    : mStop(MFALSE)
    , mTimerIdx(timerIdx)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

WPEWarpStream::ProcessThread::~ProcessThread()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::ProcessThread::threadEnque(const WarpParam &param)
{
    TRACE_FUNC_ENTER();
    android::Mutex::Autolock lock(mThreadMutex);
    mQueue.push(param);
    mThreadCondition.broadcast();
    TRACE_FUNC_EXIT();
}

MVOID WPEWarpStream::ProcessThread::signalStop()
{
    TRACE_FUNC_ENTER();
    android::Mutex::Autolock lock(mThreadMutex);
    mStop = MTRUE;
    mThreadCondition.broadcast();
    TRACE_FUNC_EXIT();
}

android::status_t WPEWarpStream::ProcessThread::readyToRun()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return android::NO_ERROR;
}

bool WPEWarpStream::ProcessThread::threadLoop()
{
    TRACE_FUNC_ENTER();
    WarpParam param;

    while( waitParam(param) )
    {
        processParam(param);
        param = WarpParam();
    }

    TRACE_FUNC_EXIT();

    return false;
}

MBOOL WPEWarpStream::ProcessThread::waitParam(WarpParam &param)
{
    TRACE_FUNC_ENTER();
    android::Mutex::Autolock lock(mThreadMutex);
    MBOOL ret = MFALSE, done = MFALSE;

    do
    {
        if( mQueue.size() )
        {
            param = mQueue.front();
            mQueue.pop();

            ret = MTRUE;
            done = MTRUE;
        }
        else if( mStop )
        {
            ret = MFALSE;
            done = MTRUE;
        }
        else
        {
            mThreadCondition.wait(mThreadMutex);
        }
    }
    while( !done );

    TRACE_FUNC_EXIT();

    return ret;
}

MVOID WPEWarpStream::ProcessThread::processParam(WarpParam param)
{
    TRACE_FUNC_ENTER();

    if( param.mResult && param.mMDPOut.size() )
    {
        if( param.mRequest == NULL )
        {
            MY_LOGE("Missing request");
            param.mResult = MFALSE;
        }
        else
        {
            param.mRequest->mTimer.startWarpMDP(mTimerIdx);

            ImgBuffer srcBuffer = param.mByPass ? param.mIn : param.mWarpOut;
            if(USE_WPE_STAND_ALONE && !param.mByPass)
            {
                for( auto &out : param.mMDPOut )
                {
                    out.mCropRect = toWPEScaleCrop(toWPEOutSize(param), out.mCropRect, param.mRequest->needPrintIO());
                }
            }
            MUINT32 numMDPPort = std::max<MUINT32>(1, mMDP.getNumOutPort());
            MUINT32 cycle = (param.mMDPOut.size() + numMDPPort - 1) / numMDPPort;
            param.mResult = mMDP.process(param.mScenarioFPS, srcBuffer->getImageBufferPtr(), param.mMDPOut, param.mRequest->needPrintIO(),
                param.mEndTimeMs/cycle);

            param.mRequest->mTimer.stopWarpMDP(mTimerIdx);
        }
    }

    if( param.mCallback )
    {
        param.mCallback(param);
    }
    else
    {
        MY_LOGE("Missing callback");
    }

    TRACE_FUNC_EXIT();
}


} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
