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

#include "G_P2SMNode.h"
#include <mtkcam3/feature/smvr/SMVRData.h>

#define PIPE_CLASS_TAG "P2SMNode"
#define PIPE_TRACE TRACE_G_P2SM_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

using namespace NSCam::Feature::P2Util;
using namespace NSCam::NSIoPipe;
using NSCam::Feature::SMVR::SMVRResult;

#define NUM_BASIC_TUNING 3

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

namespace G {

P2SMNode::P2SMNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_P2SM)
    , StreamingFeatureNode(name)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mRequests);
    TRACE_FUNC_EXIT();
}

P2SMNode::~P2SMNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID P2SMNode::setNormalStream(Feature::P2Util::DIPStream *stream)
{
    TRACE_FUNC_ENTER();
    mDIPStream = stream;
    TRACE_FUNC_EXIT();
}

MVOID P2SMNode::flush()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("flush %zu data from SMVR queue", mSubQueue.size());
    while( mSubQueue.size() )
    {
        mSubQueue.pop();
    }
    TRACE_FUNC_EXIT();
}

MBOOL P2SMNode::onInit()
{
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();

    mOneP2G = mP2GMgr->isOneP2G();
    mSMVRHal = mP2GMgr->getSMVRHal();
    mNeedDreTun = mP2GMgr->isSupportDRETun();

    mSMVRSpeed = mSMVRHal.getP2Speed();

    mExpectMS = getPropertyValue("vendor.debug.fpipe.p2sm.expect", mExpectMS);

    P2G::Feature feature;
    if( mPipeUsage.support3DNR() ) feature += P2G::Feature::NR3D;
    if( mPipeUsage.supportDSDN25() ) feature += P2G::Feature::DSDN25;
    mMainLoopData = mP2GMgr->makeInitLoopData(feature);
    mRecordLoopData = mP2GMgr->makeInitLoopData(feature);

    TRACE_FUNC_EXIT();
    return (mDIPStream != NULL && mP2GMgr != NULL);
}

MBOOL P2SMNode::onUninit()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("SMVR Q=%zu", mSubQueue.size());
    mMainLoopData = NULL;
    mRecordLoopData = NULL;
    mDIPStream = NULL;
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::onThreadStart()
{
    TRACE_FUNC_ENTER();

    mP2GMgr->allocateP2GOutBuffer();
    if( mOneP2G )
    {
        mP2GMgr->allocateP2SWBuffer();
        mP2GMgr->allocateP2HWBuffer();
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    this->waitDIPStreamBaseDone();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::onData(DataID id, const RequestPtr &data)
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

NextIO::Policy P2SMNode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    NextIO::Policy policy;
    if( reqInfo.hasMaster() )
    {
        policy.enableAll(reqInfo.mMasterID);
    }
    return policy;
}

MBOOL P2SMNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr request;

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

    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);

    request->mTimer.startP2SM();
    processP2SM(request);
    request->mTimer.stopP2SM();
    tryAllocateTuningBuffer();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2SMNode::onDIPStreamBaseCB(const Feature::P2Util::DIPParams &params, const P2SMEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing P2SMNode class members
    TRACE_FUNC_ENTER();

    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Missing request");
    }
    else
    {
        request->mTimer.stopP2SMEnque();
        MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d enque done in %d ms, result = %d", mSensorIndex, request->mRequestNo, request->mTimer.getElapsedP2SMEnque(), params.mDequeSuccess);

        if( !params.mDequeSuccess )
        {
            MY_LOGW("Frame %d enque result failed", request->mRequestNo);
        }

        request->updateResult(params.mDequeSuccess);
        handleResultData(request, data);
        request->mTimer.stopP2SM();
    }

    this->decExtThreadDependency();
    TRACE_FUNC_EXIT();
}

MVOID P2SMNode::handleResultData(const RequestPtr &request, const P2SMEnqueData &data)
{
    // This function is not thread safe,
    // because it is called by onQParamsCB,
    // avoid accessing P2SMNode class members
    TRACE_FUNC_ENTER();
    mP2GMgr->notifyFrameDone(request->mRequestNo);
    handleData(ID_P2G_TO_PMDP, P2GHWData(data.mHW, request));

    TRACE_FUNC_EXIT();
}

MBOOL P2SMNode::processP2SM(const RequestPtr &request)
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);

    P2SMEnqueData data;
    sp<MainData> mainData = new MainData();
    std::vector<P2G::IO> groupIO;
    P2G::Path path;

    data.mRequest = request;
    prepareMainData(request, data, mainData);
    prepareGroupIO(request, data, mainData, groupIO);
    updateSMVRResult(request, data);
    MY_S_LOGD(request->mLog, "#req=%d, #out=%d isRec=%d #drop=%d #Q=%zu crop=(%fx%f)@(%f,%f)", mainData->mReqCount, data.mRecordRunCount, mainData->mIsRecord, data.mDropCount, mSubQueue.size(), mainData->mRecordCrop.s.w, mainData->mRecordCrop.s.h, mainData->mRecordCrop.p.x, mainData->mRecordCrop.p.y);

    path = mP2GMgr->calcPath(groupIO, request->mRequestNo);

    if( mOneP2G )
    {
        processP2SW(request, path);
        processP2HW(request, data, path);
    }
    else
    {
        data.mSW.mP2Sizes = mainData->mP2Sizes;
        handleP2GData(request, data, path);
    }

    P2_CAM_TRACE_END(TRACE_ADVANCED);
    TRACE_FUNC_EXIT();

    return MTRUE;
}

MBOOL P2SMNode::prepareMainData(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData)
{
    TRACE_FUNC_ENTER();
    MUINT32 sensorID = request->mMasterID;
    mainData->mReqCount = request->getVar<MUINT32>(SFP_VAR::SMVR_REQ_COUNT, 0);
    mainData->mIsRecord = request->mP2Pack.getFrameData().mIsRecording;
    mainData->mRecordCrop = request->getRecordCrop();
    mainData->mTimestampCode = mainData->mReqCount;
    mainData->mP2GBasic = mP2GMgr->makePOD(request, sensorID, P2G::Scene::GM);

    IAdvPQCtrl pqCtrl = AdvPQCtrl::clone(request->getBasicPQ(request->mMasterID), "s_p2sm");
    if( mP2GMgr->isSupportDCE() ) mainData->mAddOn += P2G::Feature::DCE;
    if( mNeedDreTun ) mainData->mAddOn += P2G::Feature::DRE_TUN;
    if( request->need3DNR(sensorID) ) mainData->mAddOn += P2G::Feature::NR3D;
    if( request->needDSDN25(sensorID) )
    {
        mainData->mAddOn += P2G::Feature::DSDN25;
        P2G::P2GMgr::DsdnQueryOut out = mP2GMgr->queryDSDNInfos(sensorID, request);
        mainData->mDSDNInfo = out.outInfo;
        mainData->mDSDNSizes = out.dsSizes;
        mainData->mP2Sizes = out.p2Sizes;
    }

    mainData->mPQCtrl = pqCtrl;
    prepareDrop(request, data, mainData);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::prepareGroupIO(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData, std::vector<P2G::IO> &groupIO)
{
    TRACE_FUNC_ENTER();

    prepareMainRun(request, data, mainData, groupIO);
    prepareRecordRun(request, data, mainData, groupIO);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::updateSMVRResult(const RequestPtr &request, const P2SMEnqueData &data)
{
    TRACE_FUNC_ENTER();
    SMVRResult result;
    result.mRunCount = data.mRecordRunCount;
    result.mDropCount = data.mDropCount;
    result.mQueueCount = mSubQueue.size();
    request->setVar<SMVRResult>(SFP_VAR::SMVR_RESULT, result);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::processP2SW(const RequestPtr &request, const P2G::Path &path)
{
    TRACE_FUNC_ENTER();
    mP2GMgr->waitFrameDepDone(request->mRequestNo);
    request->mTimer.startP2SMTuning();
    mP2GMgr->runP2SW(path.mP2SW);
    request->mTimer.stopP2SMTuning();
    if( request->needPrintIO() )
    {
        unsigned i = 0;
        for( const P2G::P2SW &p2sw : path.mP2SW )
        {
            if( p2sw.mOut.mTuningData )
            {
                printTuningParam(request->mLog, p2sw.mOut.mTuningData->mParam, i++);
            }
        }
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::processP2HW(const RequestPtr &request, P2SMEnqueData &data, const P2G::Path &path)
{
    TRACE_FUNC_ENTER();
    Feature::P2Util::DIPParams params;
    data.mHW.mP2HW = path.mP2HW;
    data.mHW.mPMDP = path.mPMDP;

    params = mP2GMgr->runP2HW(path.mP2HW);
    NSCam::Feature::P2Util::updateExpectEndTime(params, mExpectMS);
    NSCam::Feature::P2Util::updateScenario(params, request->getAppFPS(), mPipeUsage.getStreamingSize().w);

    if( request->needPrintIO() )
    {
        NSCam::Feature::P2Util::printDIPParams(request->mLog, params);
    }
    enqueFeatureStream(params, data);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

P2G::IO P2SMNode::makeIO(const RequestPtr &request, const sp<MainData> &mainData, const P2G::Feature &smvrType, P2G::IO::LoopData &loopData)
{
    (void)request;
    TRACE_FUNC_ENTER();
    P2G::IO io;

    io.mScene = P2G::Scene::GM;
    io.mFeature = smvrType + mainData->mAddOn;

    io.mLoopIn = loopData;
    io.mLoopOut = P2G::IO::makeLoopData();
    loopData = io.mLoopOut;

    io.mIn.mBasic = mainData->mP2GBasic;
    io.mIn.mPQCtrl = mainData->mPQCtrl;
    io.mIn.mIMGISize = mainData->mP2GBasic.mSrcCrop.s;
    io.mIn.mSrcCrop = mainData->mP2GBasic.mSrcCrop;
    io.mIn.mDCE_n_magic = mainData->mP2GBasic.mP2Pack.getSensorData().mMagic3A;

    if( io.mFeature.has(P2G::Feature::DSDN25) )
    {
      io.mIn.mDSDNSizes = mainData->mDSDNSizes;
      io.mIn.mDSDNInfo = mainData->mDSDNInfo;
    }

    TRACE_FUNC_EXIT();
    return io;
}

MBOOL P2SMNode::prepareMainRun(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData, std::vector<P2G::IO> &groupIO)
{
    TRACE_FUNC_ENTER();
    const SFPIOMap &generalIO = request->mSFPIOManager.getFirstGeneralIO();
    const SFPSensorInput &sensorIn = request->getSensorInput();
    P2G::IO io;
    P2IO p2io;
    std::vector<P2IO> p2ioList;

    io = makeIO(request, mainData, P2G::Feature::SMVR_MAIN, mMainLoopData);

    io.mBatchOut = mainData->mBatchData;

    io.mIn.mAppInMeta = sensorIn.mAppIn;
    io.mIn.mHalInMeta = sensorIn.mHalIn;
    io.mOut.mAppOutMeta = generalIO.mAppOut;
    io.mOut.mHalOutMeta = generalIO.mHalOut;

    io.mIn.mIMGI = P2G::makeGImg(get(sensorIn.mRRZO));
    io.mIn.mLCSO = P2G::makeGImg(get(sensorIn.mLCSO));
    io.mIn.mLCSHO = P2G::makeGImg(get(sensorIn.mLCSHO));

    IImageBuffer *imgi = get(sensorIn.mRRZO);
    io.mIn.mBasic.mIsYuvIn = imgi ? !NSCam::isHalRawFormat((EImageFormat)imgi->getImgFormat()) : MFALSE;

    if( request->popDisplayOutput(this, p2io) )
    {
        io.mOut.mDisplay = P2G::makeGImg(p2io);
    }
    if( request->popFDOutput(this, p2io) )
    {
        io.mOut.mFD = P2G::makeGImg(p2io);
    }
    if( request->popExtraOutputs(this, p2ioList) )
    {
        io.mOut.mExtra = P2G::makeGImg(p2ioList);
    }

    if( request->isForceIMG3O() )
    {
        MRect crop = mainData->mP2GBasic.mSrcCrop;
        ImgBuffer buffer = mP2GMgr->requestFullImgBuffer();
        data.mHW.mOutM.mFull = buffer;
        io.mOut.mFull = P2G::makeGImg(buffer, crop.s, toMRectF(crop));
    }

    groupIO.push_back(io);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::prepareRecordRun(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData, std::vector<P2G::IO> &groupIO)
{
    TRACE_FUNC_ENTER();
    const SFPSensorInput &sensorIn = request->getSensorInput();
    std::vector<P2IO> recordOut;
    size_t subRun = mSMVRSpeed;
    MBOOL optimize = mOptimizeFirst && mSubQueue.empty();

    for( unsigned i = 0; i < mainData->mReqCount; ++i )
    {
        mSubQueue.emplace(SubData(mainData, get(sensorIn.mRRZO, i),
                                            get(sensorIn.mLCSO, i)));
    }

    request->popRecordOutputs(this, recordOut);
    subRun = std::min(std::min(recordOut.size(), mSubQueue.size()), subRun);

    groupIO.reserve((optimize ? 0 : 1) + subRun);
    for( unsigned i = 0; i < subRun; ++i )
    {
        const SubData &sub = mSubQueue.front();
        recordOut[i].mTimestampCode = sub.mMainData->mTimestampCode;
        if( i == 0 && optimize )
        {
            groupIO[0].mOut.mRecord = P2G::makeGImg(recordOut[i]);
            mRecordLoopData = mMainLoopData;
        }
        else
        {
            P2G::IO io;
            io = makeIO(request, sub.mMainData,
                        P2G::Feature::SMVR_SUB, mRecordLoopData);
            io.mBatchIn = sub.mMainData->mBatchData;
            io.mIn.mIMGI = P2G::makeGImg(sub.mRRZO);
            io.mIn.mLCSO = P2G::makeGImg(sub.mLCSO);
            P2IO rec = recordOut[i];
            rec.mCropRect = sub.mMainData->mRecordCrop;
            io.mOut.mRecord = P2G::makeGImg(rec);
            groupIO.push_back(io);
        }
        mSubQueue.pop();
        ++data.mRecordRunCount;
    }

    if( subRun && mSubQueue.empty() )
    {
        mMainLoopData = mRecordLoopData;
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SMNode::prepareDrop(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData)
{
    TRACE_FUNC_ENTER();
    if( mainData != NULL && !mainData->mIsRecord )
    {
        if( !mSubQueue.empty() )
        {
            data.mDropCount += mSubQueue.size();
            flush();
        }
        if( mainData->mReqCount != 0 )
        {
            MY_S_LOGW(request->mLog, "SMVR mark drop but reqCount(%d) != 0, force drop", mainData->mReqCount);
            data.mDropCount += mainData->mReqCount;
            mainData->mReqCount = 0;
        }
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2SMNode::handleP2GData(const RequestPtr &request, P2SMEnqueData &data, const P2G::Path &path)
{
    TRACE_FUNC_ENTER();
    data.mSW.mP2SW = path.mP2SW;
    data.mHW.mP2HW = path.mP2HW;
    data.mHW.mPMDP = path.mPMDP;

    handleData(ID_P2G_TO_MSS, MSSData(path.mMSS, request));
    handleData(ID_P2G_TO_P2SW, P2GSWData(data.mSW, request));
    handleData(ID_P2G_TO_MSF, P2GHWData(data.mHW, request));
    TRACE_FUNC_EXIT();
}

MVOID P2SMNode::enqueFeatureStream(Feature::P2Util::DIPParams &params, P2SMEnqueData &data)
{
    TRACE_FUNC_ENTER();
    MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d enque start", mSensorIndex, data.mRequest->mRequestNo);
    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_P2));
    data.mRequest->mTimer.startP2SMEnque();
    this->incExtThreadDependency();
    this->enqueDIPStreamBase(mDIPStream, params, data);
    TRACE_FUNC_EXIT();
}

MVOID P2SMNode::tryAllocateTuningBuffer()
{
    TRACE_FUNC_ENTER();
    if( mOneP2G )
    {
        mP2GMgr->allocateP2GOutBuffer(MTRUE);
        mP2GMgr->allocateP2SWBuffer(MTRUE);
        mP2GMgr->allocateP2HWBuffer(MTRUE);
    }
    TRACE_FUNC_EXIT();
}

} // namespace G

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
