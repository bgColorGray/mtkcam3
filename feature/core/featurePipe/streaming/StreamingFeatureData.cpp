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

#include "StreamingFeatureData.h"
#include "StreamingFeatureNode.h"
#include "StreamingFeature_Common.h"
#include <camera_custom_eis.h>
#include <camera_custom_dualzoom.h>

#include <utility>

#define PIPE_CLASS_TAG "Data"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_DATA
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam3/feature/DualCam/DualCam.Common.h>

#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);
using namespace NSCam::Utils::ULog;

using NSCam::NSIoPipe::QParams;
using NSCam::NSIoPipe::MCropRect;
using NSCam::Feature::P2Util::P2IO;
using NSCam::Feature::P2Util::P2Pack;
using NSCam::Feature::P2Util::P2SensorData;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

std::unordered_map<MUINT32, std::string> StreamingFeatureRequest::mFeatureMaskNameMap;

StreamingFeatureRequest::StreamingFeatureRequest(const StreamingFeaturePipeUsage &pipeUsage, const FeaturePipeParam &extParam, MUINT32 requestNo, MUINT32 recordNo, const EISQState &eisQ)
    : mExtParam(extParam)
    , mPipeUsage(pipeUsage)
    , mVarMap(mExtParam.mVarMap)
    , mP2Pack(mExtParam.mP2Pack)
    , mLog(mP2Pack.mLog)
    , mSFPIOManager(mExtParam.mSFPIOManager)
    , mMasterID(mExtParam.getVar<MUINT32>(SFP_VAR::DUALCAM_FOV_MASTER_ID, pipeUsage.getSensorIndex()))
    , mIORequestMap({{mMasterID, NextIO::Request()}})
    , mMasterIOReq(mIORequestMap[mMasterID])
    , mFeatureMask(extParam.mFeatureMask)
    , mRequestNo(requestNo)
    , mRecordNo(recordNo)
    , mMWFrameNo(extParam.mP2Pack.getFrameData().mMWFrameNo)
    , mAppMode(IStreamingFeaturePipe::APP_PHOTO_PREVIEW)
    , mDebugDump(0)
    , mP2DumpType(mExtParam.mDumpType)
    , mSlaveParamMap(mExtParam.mSlaveParamMap)
    , mDisplayFPSCounter(NULL)
    , mFrameFPSCounter(NULL)
    , mResult(MTRUE)
    , mNeedDump(MFALSE)
    , mForceIMG3O(MFALSE)
    , mForceWarpPass(MFALSE)
    , mForceGpuOut(NO_FORCE)
    , mForceGpuRGBA(MFALSE)
    , mForcePrintIO(MFALSE)
    , mIs4K2K(MFALSE)
    , mEISQState(eisQ)
    , mOutputControl(pipeUsage.getAllSensorIDs())
{
    mExtParam.mDIPParams.mDequeSuccess = MFALSE;
    // 3DNR + EIS1.2 in 4K2K record mode use CRZ to reduce throughput
    mIsP2ACRZMode = getVar<MBOOL>(SFP_VAR::NR3D_EIS_IS_CRZ_MODE, MFALSE);
    mAppMode = getVar<IStreamingFeaturePipe::eAppMode>(SFP_VAR::APP_MODE, IStreamingFeaturePipe::APP_PHOTO_PREVIEW);
    mBasicPQ = getVar<IPQCtrl_const>(SFP_VAR::PQ_INFO, NULL);
    mSlaveBasicPQ = getVar<IPQCtrl_const>(SFP_VAR::PQ_INFO_SLAVE, NULL);

    mMasterID = getVar<MUINT32>(SFP_VAR::DUALCAM_FOV_MASTER_ID, pipeUsage.getSensorIndex());
    mSlaveID = getVar<MUINT32>(SFP_VAR::DUALCAM_FOV_SLAVE_ID, INVALID_SENSOR); // TODO use sub sensor list id to replace it
    mFDSensorID = getVar<MUINT32>(SFP_VAR::FD_SOURCE_SENSOR_ID, mMasterID);
    if(mMasterID == mSlaveID || (mSlaveID != INVALID_SENSOR && !hasSlave(mSlaveID)))
    {
        MY_LOGE("Parse Request Sensor ID & slave FeatureParam FAIL! , master(%u), slave(%u), slaveParamExist(%d)",
                mMasterID, mSlaveID, hasSlave(mSlaveID));
    }
    appendSlaveMask();
    updateEnqueInfo();
    prepareOutputInfo();

    mTPILog = mPipeUsage.supportVendorLog();
    if( mPipeUsage.supportVendorDebug() )
    {
        mTPILog = mTPILog || property_get_int32(KEY_DEBUG_TPI_LOG, mTPILog);
        mTPIDump = property_get_int32(KEY_DEBUG_TPI_DUMP, mTPIDump);
        mTPIScan = property_get_int32(KEY_DEBUG_TPI_SCAN, mTPIScan);
        mTPIBypass = property_get_int32(KEY_DEBUG_TPI_BYPASS, mTPIBypass);
    }

    mHasGeneralOutput = hasDisplayOutput() || hasRecordOutput() || hasExtraOutput();
    mHasOutYuv = mHasGeneralOutput || hasFDOutput() || mSFPIOManager.countPhysical() || mSFPIOManager.countLarge();

    mAppFPS = std::max(mP2Pack.getFrameData().mMinFps, 30);
    mNodeFPS = mPipeUsage.supportSMP2() ? 30 : mAppFPS;
    mNodeCycleTimeMs = 1000 / mNodeFPS;

    mTimer.start();
}

StreamingFeatureRequest::~StreamingFeatureRequest()
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);

    double frameFPS = 0, displayFPS = 0;
    if( mDisplayFPSCounter )
    {
        mDisplayFPSCounter->update(mTimer.getDisplayMark());
        displayFPS = mDisplayFPSCounter->getFPS();
    }
    if( mFrameFPSCounter )
    {
        mFrameFPSCounter->update(mTimer.getFrameMark());
        frameFPS = mFrameFPSCounter->getFPS();
    }
    const Feature::P2Util::P2FrameData& data = mP2Pack.getFrameData();
    mTimer.print(mRequestNo, mRecordNo, displayFPS, frameFPS, data.mMinFps, data.mMaxFps);
}

MVOID StreamingFeatureRequest::prepareOutputInfo()
{
    IO_TYPE target[] = { IO_TYPE_DISPLAY, IO_TYPE_RECORD, IO_TYPE_FD };
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    for( IO_TYPE ioType : target)
    {
        P2IO output;
        if( getOutBuffer(generalIO, ioType, output) )
        {
            OutputInfo bufInfo;
            bufInfo.mCropRect = output.mCropRect;
            bufInfo.mOutSize = output.getImgSize();
            mOutputInfoMap[ioType] = bufInfo;
        }
    }
}

MVOID StreamingFeatureRequest::setDisplayFPSCounter(FPSCounter *counter)
{
    mDisplayFPSCounter = counter;
}

MVOID StreamingFeatureRequest::setFrameFPSCounter(FPSCounter *counter)
{
    mFrameFPSCounter = counter;
}

MVOID StreamingFeatureRequest::appendSlaveMask()
{
    if( mSlaveID != INVALID_SENSOR && mPipeUsage.supportSlaveNR() )
    {
        const MUINT32 slaveMask = getSlave(mSlaveID).mFeatureMask;
        if( HAS_3DNR(slaveMask) ) ENABLE_3DNR_S(mFeatureMask);
        if( HAS_DSDN20(slaveMask) ) ENABLE_DSDN20_S(mFeatureMask);
        if( HAS_DSDN25(slaveMask) ) ENABLE_DSDN25_S(mFeatureMask);
        if( HAS_DSDN30(slaveMask) ) ENABLE_DSDN30_S(mFeatureMask);
    }
}

MVOID StreamingFeatureRequest::calSizeInfo()
{
    SrcCropInfo cInfo;
    calNonLargeSrcCrop(cInfo, mMasterID);
    mNonLargeSrcCrops[mMasterID] = cInfo;

    mFullImgSize = cInfo.mSrcCrop.s;
    mIs4K2K = NSFeaturePipe::is4K2K(mFullImgSize);

    if(mSlaveID != INVALID_SENSOR)
    {
        calNonLargeSrcCrop(cInfo, mSlaveID);
        mNonLargeSrcCrops[mSlaveID] = cInfo;
    }

    if(mHasGeneralOutput)
    {
        calGeneralZoomROI();
    }
}

MVOID StreamingFeatureRequest::updateEnqueInfo()
{
    for( MUINT32 sID : {mMasterID, mSlaveID} )
    {
        if( sID != INVALID_SENSOR )
        {
            IImageBuffer* rrz = get(mSFPIOManager.getInput(sID).mRRZO);
            mCurEnqInfo[sID] = EnqueInfo::Data(rrz ? rrz->getImgSize() : MSize(0,0),
                                                mP2Pack.getSensorData(sID).mP1TS,
                                                getFeatureMask(sID));
        }
    }
}

MUINT32 StreamingFeatureRequest::getFeatureMask(MUINT32 sensorID)
{
    return (sensorID == mMasterID) ? mFeatureMask : getSlave(sensorID).mFeatureMask;
}

MVOID StreamingFeatureRequest::calGeneralZoomROI()
{
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL found = MFALSE;
    for( const SFPOutput &out : generalIO.mOutList)
    {
        if( isFDOutput(out) )
        {
            continue;
        }
        if( found )
        {
            accumulateUnion(mGeneralZoomROI, out.mCropRect);
        }
        else
        {
            mGeneralZoomROI = out.mCropRect;
            found = MTRUE;
        }
    }
    MY_S_LOGE_IF(!found, mLog, "Can not find general zoom ROI");
}

NextIO::ReqInfo StreamingFeatureRequest::extractReqInfo() const
{
    NextIO::ReqInfo info(mRequestNo, mFeatureMask, mMasterID, mSlaveID);

    std::list<NextIO::OutInfo> outInfo;
    for( const SFPOutput &out : mSFPIOManager.getFirstGeneralIO().mOutList )
    {
        NextIO::OutType type = isDisplayOutput(out) ? NextIO::OUT_PREVIEW :
                               isRecordOutput(out) ? NextIO::OUT_RECORD :
                               isExtraOutput(out) ? NextIO::OUT_EXTRA :
                               NextIO::OUT_UNKNOWN;
        outInfo.push_back(NextIO::OutInfo(type, out.mCropDstSize, out.mCropRect));
    }
    info.mOutInfoMap[mMasterID] = outInfo;

    for( MUINT32 sID : {mMasterID, mSlaveID} )
    {
        if( sID == INVALID_SENSOR ) continue;

        auto &inInfoMap = info.mInInfoMap[sID];
        const SFPSensorInput &sensorIn = mSFPIOManager.getInput(sID);
        if( sensorIn.mIMGO.size() && sensorIn.mIMGO[0] )
        {
            MSize imgoSize = sensorIn.mIMGO[0]->getImgSize();
            inInfoMap[NextIO::IN_IMGO] = NextIO::InInfo(NextIO::IN_IMGO, imgoSize, MRect(imgoSize.w, imgoSize.h));
        }
        if( sensorIn.mRRZO.size() && sensorIn.mRRZO[0] )
        {
            const P2SensorData &sensorData = mP2Pack.getSensorData(sID);
            MSize rrzSize = sensorIn.mRRZO[0]->getImgSize();
            inInfoMap[NextIO::IN_RRZO] = NextIO::InInfo(NextIO::IN_RRZO, rrzSize, sensorData.mP1Crop);
        }

        auto &metaMap = info.mMetaMap[sID];
        metaMap[NextIO::IN_META_APP] = sensorIn.mAppIn;
        metaMap[NextIO::IN_META_P1_HAL] = sensorIn.mHalIn;
        metaMap[NextIO::IN_META_P1_APP] = sensorIn.mAppDynamicIn;

        auto &outList = info.mOutInfoMap[sID];
        for( const SFPOutput &out : mSFPIOManager.getPhysicalIO(sID).mOutList )
        {
            outList.push_back(NextIO::OutInfo(NextIO::OUT_PHY, out.mCropDstSize, out.mCropRect));
        }
    }
    return info;
}

MBOOL StreamingFeatureRequest::updateResult(MBOOL result)
{
    mResult = (mResult && result);
    mExtParam.mDIPParams.mDequeSuccess = mResult;
    return mResult;
}

MBOOL StreamingFeatureRequest::doExtCallback(FeaturePipeParam::MSG_TYPE msg)
{
    MBOOL ret = MFALSE;
    if( msg == FeaturePipeParam::MSG_FRAME_DONE )
    {
        mTimer.stop();
    }
    if( mExtParam.mCallback )
    {
        if(msg == FeaturePipeParam::MSG_FRAME_DONE)
        {
            CAM_ULOG_EXIT(MOD_FPIPE_STREAMING, REQ_STR_FPIPE_REQUEST, mRequestNo);
            CAM_ULOG_EXIT(MOD_P2_STR_PROC, REQ_P2_STR_REQUEST, mExtParam.mP2Pack.mLog.getLogFrameID());
        }
        ret = mExtParam.mCallback(msg, mExtParam);
    }
    return ret;
}

IPQCtrl_const StreamingFeatureRequest::getBasicPQ(MUINT32 sensorID) const
{
    return (sensorID == mMasterID) ? mBasicPQ : mSlaveBasicPQ;
}

MVOID StreamingFeatureRequest::calNonLargeSrcCrop(SrcCropInfo &info, MUINT32 sensorID)
{
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    const SFPSensorInput &sensorIn = mSFPIOManager.getInput(sensorID);
    SFPSensorTuning tuning;

    if(generalIO.isValid() && generalIO.hasTuning(sensorID))
    {
        tuning = generalIO.getTuning(sensorID);
    }
    else if(mSFPIOManager.hasPhysicalIO(sensorID))
    {
        tuning = mSFPIOManager.getPhysicalIO(sensorID).getTuning(sensorID);
    }

    info.mIMGOSize = (sensorIn.mIMGO.size() && sensorIn.mIMGO[0]) ? sensorIn.mIMGO[0]->getImgSize() : MSize(0,0);
    info.mRRZOSize = (sensorIn.mRRZO.size() && sensorIn.mRRZO[0]) ? sensorIn.mRRZO[0]->getImgSize()
                      : mP2Pack.isValid() ? mP2Pack.getSensorData(sensorID).mP1OutSize
                      : getSensorVarMap(sensorID).get<MSize>(SFP_VAR::HAL1_P1_OUT_RRZ_SIZE, MSize(0,0));
    info.mIMGOin = (tuning.isIMGOin() && !tuning.isRRZOin());
    if( !info.mIMGOin )
    {
        info.mSrcCrop = MRect(MPoint(0,0), info.mRRZOSize);
    }
    info.mIsSrcCrop = MFALSE;

    if(info.mIMGOin)
    {
        info.mSrcCrop = (mP2Pack.isValid()) ? mP2Pack.getSensorData(sensorID).mP1Crop
                    : getSensorVarMap(sensorID).get<MRect>(SFP_VAR::IMGO_2IMGI_P1CROP, MRect(0,0));
        info.mIsSrcCrop = MTRUE;
        // HW limitation
        const MUINT32 alignMask = mP2Pack.getPlatInfo()->getImgoAlignMask();
        info.mSrcCrop.p.x &= alignMask;
    }

    info.mSrcSize = info.mIMGOin ? info.mIMGOSize : info.mRRZOSize;
    // NOTE : currently not consider CRZ mode
    MY_LOGD("sID(%d), imgoIn(%d), srcCrop(%d,%d,%dx%d), isSrcCrop(%d), mP2Pack Valid(%d), imgo(%dx%d),rrz(%dx%d)",
            sensorID, info.mIMGOin, info.mSrcCrop.p.x, info.mSrcCrop.p.y, info.mSrcCrop.s.w, info.mSrcCrop.s.h,
            info.mIsSrcCrop, mP2Pack.isValid(), info.mIMGOSize.w, info.mIMGOSize.h, info.mRRZOSize.w, info.mRRZOSize.h);
}

MVOID StreamingFeatureRequest::checkBufferHoldByReq() const
{
    for(auto& it : mIORequestMap)
    {
        if(it.second.isFullBufHold())
        {
            MY_LOGW("Request hold working buf in SensorID (%d)", it.first);
        }
    }
}

IImageBuffer* StreamingFeatureRequest::getMasterInputBuffer()
{
    IImageBuffer *buffer = NULL;
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    const SFPSensorInput &masterIn = mSFPIOManager.getInput(mMasterID);
    SFPSensorTuning tuning;

    if(generalIO.isValid() && generalIO.hasTuning(mMasterID))
    {
        tuning = generalIO.getTuning(mMasterID);
    }
    else if(mSFPIOManager.hasPhysicalIO(mMasterID))
    {
        tuning = mSFPIOManager.getPhysicalIO(mMasterID).getTuning(mMasterID);
    }
    buffer =  tuning.isRRZOin() ? get(masterIn.mRRZO)
        : tuning.isIMGOin() ? get(masterIn.mIMGO)
        : NULL;
    return buffer;
}

MBOOL StreamingFeatureRequest::popDisplayOutput(const StreamingFeatureNode *node, P2IO &output)
{
    TRACE_FUNC_ENTER();
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = needDisplayOutput(node) && getOutBuffer(generalIO, IO_TYPE_DISPLAY, output);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No display buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOut(OutputControl::GENERAL_DISPLAY, output);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popRecordOutput(const StreamingFeatureNode *node, P2IO &output)
{
    TRACE_FUNC_ENTER();
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = needRecordOutput(node) && getOutBuffer(generalIO, IO_TYPE_RECORD, output);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No record buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOut(OutputControl::GENERAL_RECORD, output);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popRecordOutputs(const StreamingFeatureNode *node, std::vector<P2IO> &outList)
{
    TRACE_FUNC_ENTER();
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = needRecordOutput(node) && getMultiOutBuffer(generalIO, IO_TYPE_RECORD, outList);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No record buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOuts(OutputControl::GENERAL_RECORD, outList);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popExtraOutputs(const StreamingFeatureNode *node, std::vector<P2IO> &outList)
{
    TRACE_FUNC_ENTER();
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = needExtraOutput(node) && getOutBuffer(generalIO, IO_TYPE_EXTRA, outList);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No extra buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOuts(OutputControl::GENERAL_EXTRA, outList);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popPhysicalOutput(const StreamingFeatureNode *node, MUINT32 sensorID, P2IO &output)
{
    TRACE_FUNC_ENTER();
    const SFPIOMap &phyIO = mSFPIOManager.getPhysicalIO(sensorID);
    MBOOL ret = needPhysicalOutput(node, sensorID) && getOutBuffer(phyIO, IO_TYPE_PHYSICAL, output);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No physical buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOut(OutputControl::SENSOR_PHYSICAL, sensorID, output);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popLargeOutputs(const StreamingFeatureNode *node, MUINT32 sensorID, std::vector<P2IO> &outList)
{
    TRACE_FUNC_ENTER();
    // NOTE currently Large output has no need to consider IOPath
    (void)node;
    MBOOL ret = MFALSE;
    const SFPIOMap &largeIO = mSFPIOManager.getLargeIO(sensorID);
    if(largeIO.isValid())
    {
        largeIO.getAllOutput(outList);
        ret = MTRUE;
    }
    if( !ret )
    {
        TRACE_FUNC("frame %d: No Large buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOuts(OutputControl::SENSOR_LARGE, sensorID, outList);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popFDOutput(const StreamingFeatureNode *node, P2IO &output)
{
    TRACE_FUNC_ENTER();
    // NOTE currently FD output has no need to consider IOPath
    (void)node;
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = getOutBuffer(generalIO, IO_TYPE_FD, output);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No FD buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOut(OutputControl::GENERAL_FD, output);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::popAppDepthOutput(const StreamingFeatureNode *node, P2IO &output)
{
    TRACE_FUNC_ENTER();
    // NOTE currently AppDepth output has no need to consider IOPath
    (void)node;
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = getOutBuffer(generalIO, IO_TYPE_APP_DEPTH, output);
    if( !ret )
    {
        TRACE_FUNC("frame %d: No AppDepth buffer", mRequestNo);
    }
    ret = ret && mOutputControl.registerFillOut(OutputControl::GENERAL_APP_DEPTH, output);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL StreamingFeatureRequest::getOutputInfo(IO_TYPE ioType, OutputInfo &bufInfo) const
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    auto search = mOutputInfoMap.find(ioType);
    if( search != mOutputInfoMap.end() )
    {
        bufInfo = search->second;
        ret = MTRUE;
    }
    else
    {
        ret = MFALSE;
    }
    return ret;
}

EISSourceDumpInfo StreamingFeatureRequest::getEISDumpInfo()
{
    return mEisDumpInfo;
}

IImageBuffer* StreamingFeatureRequest::getDisplayOutputBuffer()
{
    P2IO output;
    IImageBuffer *outputBuffer = NULL;
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = getOutBuffer(generalIO, IO_TYPE_DISPLAY, output);
    if( ret )
    {
        outputBuffer = output.mBuffer;
    }
    return outputBuffer;
}

IImageBuffer* StreamingFeatureRequest::getRecordOutputBuffer()
{
    P2IO output;
    IImageBuffer *outputBuffer = NULL;
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();
    MBOOL ret = getOutBuffer(generalIO, IO_TYPE_RECORD, output);
    if( ret )
    {
        outputBuffer = output.mBuffer;
    }
    return outputBuffer;
}

MSize StreamingFeatureRequest::getMasterInputSize()
{
    MSize size;
    IImageBuffer* pImgBuf = getMasterInputBuffer();
    if (pImgBuf != NULL)
    {
        size = pImgBuf->getImgSize();
    }
    return size;
}

MSize StreamingFeatureRequest::getDisplaySize() const
{
    OutputInfo disInfo;
    MSize size;
    if( getOutputInfo(IO_TYPE_DISPLAY, disInfo) )
    {
        size = disInfo.mOutSize;
    }

    return size;
}

MSize StreamingFeatureRequest::getFDSize() const
{
    OutputInfo fdInfo;
    MSize size;
    if( getOutputInfo(IO_TYPE_FD, fdInfo) )
    {
        size = fdInfo.mOutSize;
    }

    return size;
}

MRectF StreamingFeatureRequest::getRecordCrop() const
{
    OutputInfo recInfo;
    MRectF crop;
    if( getOutputInfo(IO_TYPE_RECORD, recInfo) )
    {
        crop = recInfo.mCropRect;
    }

    return crop;
}

MRectF StreamingFeatureRequest::getZoomROI() const
{
    return mGeneralZoomROI;
}

MUINT32 StreamingFeatureRequest::getExtraOutputCount() const
{
    return getOutBufferCount(mSFPIOManager.getFirstGeneralIO(), IO_TYPE_EXTRA);
}

ImgBuffer StreamingFeatureRequest::popAsyncImg(const StreamingFeatureNode *node)
{
    TRACE_FUNC_ENTER();
    NextIO::NextBuf async = mMasterIOReq.getAsyncImg(node);
    ImgBuffer buf = async.mBuffer;
    if( mOutputControl.registerFillOut(OutputControl::GENERAL_ASYNC, buf, MFALSE) &&
        buf != NULL )
    {
        MSize resize = isValid(async.mAttr.mResize) ? async.mAttr.mResize : getDisplaySize();
        buf->setExtParam(resize);
        if( mPipeUsage.supportVendorErase() )
        {
            buf->erase();
            buf->syncCache(eCACHECTRL_FLUSH);
        }
    }
    TRACE_FUNC_EXIT();
    return buf;
}

ImgBuffer StreamingFeatureRequest::getAsyncImg(const StreamingFeatureNode *node) const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_ENTER();
    return mMasterIOReq.getAsyncImg(node).mBuffer;
}

MVOID StreamingFeatureRequest::clearAsyncImg(const StreamingFeatureNode *node)
{
    mMasterIOReq.clearAsyncImg(node);
}

NextIO::NextReq StreamingFeatureRequest::requestNextFullImg(const StreamingFeatureNode *node, MUINT32 sensorID)
{
    TRACE_FUNC_ENTER();
    NextIO::NextReq nextReq;
    if(mIORequestMap.count(sensorID) > 0 && mIORequestMap.at(sensorID).needNextFull(node))
    {
        nextReq = mIORequestMap.at(sensorID).getNextFullImg(node);
    }

    TRACE_FUNC_EXIT();
    return nextReq;
}

MBOOL StreamingFeatureRequest::needDisplayOutput(const StreamingFeatureNode *node)
{
    return mMasterIOReq.needPreview(node);
}

MBOOL StreamingFeatureRequest::needRecordOutput(const StreamingFeatureNode *node)
{
    return mMasterIOReq.needRecord(node);
}

MBOOL StreamingFeatureRequest::needExtraOutput(const StreamingFeatureNode *node)
{
    return mMasterIOReq.needPreviewCallback(node);
}

MBOOL StreamingFeatureRequest::needFullImg(const StreamingFeatureNode *node, MUINT32 sensorID)
{
    return (mIORequestMap.count(sensorID) > 0)
            && (mIORequestMap.at(sensorID).needFull(node));
}

MBOOL StreamingFeatureRequest::needNextFullImg(const StreamingFeatureNode *node, MUINT32 sensorID)
{
    return (mIORequestMap.count(sensorID) > 0)
            && (mIORequestMap.at(sensorID).needNextFull(node));
}

MBOOL StreamingFeatureRequest::needPhysicalOutput(const StreamingFeatureNode *node, MUINT32 sensorID)
{
    return (mIORequestMap.count(sensorID) > 0)
            && (mIORequestMap.at(sensorID).needPhysicalOut(node));
}

MBOOL StreamingFeatureRequest::hasGeneralOutput() const
{
    return mHasGeneralOutput;
}

MBOOL StreamingFeatureRequest::hasDisplayOutput() const
{
    return existOutBuffer(mSFPIOManager.getFirstGeneralIO(), IO_TYPE_DISPLAY);
}

MBOOL StreamingFeatureRequest::hasFDOutput() const
{
    return existOutBuffer(mSFPIOManager.getFirstGeneralIO(), IO_TYPE_FD);
}

MBOOL StreamingFeatureRequest::hasRecordOutput() const
{
    return existOutBuffer(mSFPIOManager.getFirstGeneralIO(), IO_TYPE_RECORD);
}

MBOOL StreamingFeatureRequest::hasExtraOutput() const
{
    return existOutBuffer(mSFPIOManager.getFirstGeneralIO(), IO_TYPE_EXTRA);
}

MBOOL StreamingFeatureRequest::hasPhysicalOutput(MUINT32 sensorID) const
{
    return mSFPIOManager.getPhysicalIO(sensorID).isValid();
}

MBOOL StreamingFeatureRequest::hasLargeOutput(MUINT32 sensorID) const
{
    return mSFPIOManager.getLargeIO(sensorID).isValid();
}

MBOOL StreamingFeatureRequest::hasOutYuv() const
{
    return mHasOutYuv;
}

MSize StreamingFeatureRequest::getEISInputSize() const
{
    MSize size = mFullImgSize;
    return size;
}

SrcCropInfo StreamingFeatureRequest::getSrcCropInfo(MUINT32 sensorID)
{
    if(mNonLargeSrcCrops.count(sensorID) == 0)
    {
        MY_LOGW("sID(%d) srcCropInfo not exist!, create dummy.", sensorID);
        mNonLargeSrcCrops[sensorID] = SrcCropInfo();
    }
    return mNonLargeSrcCrops.at(sensorID);
}

EnqueInfo::Data StreamingFeatureRequest::getLastEnqData(MUINT32 sensorID) const
{
    return mLastEnqInfo.at(sensorID);
}

EnqueInfo::Data StreamingFeatureRequest::getCurEnqData(MUINT32 sensorID) const
{
    return mCurEnqInfo.at(sensorID);
}

EnqueInfo StreamingFeatureRequest::getCurEnqInfo() const
{
    return mCurEnqInfo;
}

MVOID StreamingFeatureRequest::setLastEnqInfo(const EnqueInfo& info)
{
    mLastEnqInfo = info;
}

MVOID StreamingFeatureRequest::setDumpProp(MINT32 start, MINT32 count, MBOOL byRecordNo)
{
    MINT32 debugDumpNo = byRecordNo ? mRecordNo : mRequestNo;
    mNeedDump = (start < 0) ||
                (((MINT32)debugDumpNo >= start) && (((MINT32)debugDumpNo - start) < count));
    mNeedDump = mNeedDump || mP2DumpType == Feature::P2Util::P2_DUMP_DEBUG;
}

MVOID StreamingFeatureRequest::setForceIMG3O(MBOOL forceIMG3O)
{
    mForceIMG3O = forceIMG3O;
}

MVOID StreamingFeatureRequest::setForceWarpPass(MBOOL forceWarpPass)
{
    mForceWarpPass = forceWarpPass;
}

MVOID StreamingFeatureRequest::setForceGpuOut(MUINT32 forceGpuOut)
{
    mForceGpuOut = forceGpuOut;
}

MVOID StreamingFeatureRequest::setForceGpuRGBA(MBOOL forceGpuRGBA)
{
    mForceGpuRGBA = forceGpuRGBA;
}

MVOID StreamingFeatureRequest::setForcePrintIO(MBOOL forcePrintIO)
{
    mForcePrintIO = forcePrintIO;
    mLog = makeRequestLogger(mLog, (mForcePrintIO ? 1 : 0), mRequestNo);
}

MVOID StreamingFeatureRequest::setEISDumpInfo(const EISSourceDumpInfo& info)
{
    mEisDumpInfo = info;
}

MBOOL StreamingFeatureRequest::isForceIMG3O() const
{
    return mForceIMG3O;
}

MBOOL StreamingFeatureRequest::hasSlave(MUINT32 sensorID) const
{
    return (mSlaveParamMap.count(sensorID) != 0);
}

FeaturePipeParam& StreamingFeatureRequest::getSlave(MUINT32 sensorID)
{
    if(hasSlave(sensorID))
    {
        return mSlaveParamMap.at(sensorID);
    }
    else
    {
        MY_LOGE("sensor(%d) has no slave FeaturePipeParam !! create Dummy", sensorID);
        mSlaveParamMap[sensorID] = FeaturePipeParam();
        return mSlaveParamMap.at(sensorID);
    }
}

const SFPSensorInput& StreamingFeatureRequest::getSensorInput() const
{
    return mSFPIOManager.getInput(mMasterID);
}

const SFPSensorInput& StreamingFeatureRequest::getSensorInput(MUINT32 sensorID) const
{
    return mSFPIOManager.getInput(sensorID);
}

VarMap<SFP_VAR>& StreamingFeatureRequest::getSensorVarMap(MUINT32 sensorID)
{
    if(sensorID == mMasterID)
    {
        return mVarMap;
    }
    else
    {
        return mSlaveParamMap[sensorID].mVarMap;
    }
}

BasicImg::SensorClipInfo StreamingFeatureRequest::getSensorClipInfo(MUINT32 sensorID) const
{
    const P2SensorData &sensorData = mP2Pack.getSensorData(sensorID);
    return BasicImg::SensorClipInfo(sensorID, sensorData.mP1Crop, sensorData.mSensorSize);
}

const char* StreamingFeatureRequest::getFeatureMaskName() const
{
    std::unordered_map<MUINT32, std::string>::const_iterator iter = mFeatureMaskNameMap.find(mFeatureMask);

    if( iter == mFeatureMaskNameMap.end() )
    {
        string str;

        appendVendorTag(str, mFeatureMask);
        append3DNRTag(str, mFeatureMask);
        appendRSCTag(str, mFeatureMask);
        appendDSDNTag(str, mFeatureMask);
        appendFSCTag(str, mFeatureMask);
        appendEisTag(str, mFeatureMask);
        appendNoneTag(str, mFeatureMask);
        appendDefaultTag(str, mFeatureMask);

        iter = mFeatureMaskNameMap.insert(std::make_pair(mFeatureMask, str)).first;
    }

    return iter->second.c_str();
}

MBOOL StreamingFeatureRequest::need3DNR(MUINT32 sensorID) const
{
    return (sensorID == mMasterID) ? HAS_3DNR(mFeatureMask) : HAS_3DNR_S(mFeatureMask);
}

MBOOL StreamingFeatureRequest::needVHDR() const
{
    return HAS_VHDR(mFeatureMask);
}

MBOOL StreamingFeatureRequest::needDSDN20(MUINT32 sensorID) const
{
    return (sensorID == mMasterID) ? HAS_DSDN20(mFeatureMask) : HAS_DSDN20_S(mFeatureMask);
}

MBOOL StreamingFeatureRequest::needDSDN25(MUINT32 sensorID) const
{
    return (sensorID == mMasterID) ? HAS_DSDN25(mFeatureMask) : HAS_DSDN25_S(mFeatureMask);
}

MBOOL StreamingFeatureRequest::needDSDN30(MUINT32 sensorID) const
{
    return (sensorID == mMasterID) ? HAS_DSDN30(mFeatureMask) : HAS_DSDN30_S(mFeatureMask);
}

MBOOL StreamingFeatureRequest::needEIS() const
{
    return HAS_EIS(mFeatureMask) && (hasRecordOutput() || needPreviewEIS());
}

MBOOL StreamingFeatureRequest::needPreviewEIS() const
{
    return mPipeUsage.supportPreviewEIS();
}

MBOOL StreamingFeatureRequest::needTPIYuv() const
{
    return HAS_TPI_YUV(mFeatureMask) && hasGeneralOutput();
}

MBOOL StreamingFeatureRequest::needTPIAsync() const
{
    return HAS_TPI_ASYNC(mFeatureMask) && hasGeneralOutput();
}

MBOOL StreamingFeatureRequest::needEarlyFSCVendorFullImg() const
{
    return mPipeUsage.supportVendorFSCFullImg() && needFSC();
}

MBOOL StreamingFeatureRequest::needWarp() const
{
    return needEIS();
}

MBOOL StreamingFeatureRequest::needEarlyDisplay() const
{
    return HAS_EIS(mFeatureMask) && !needPreviewEIS();
}

MBOOL StreamingFeatureRequest::needP2AEarlyDisplay() const
{
    return needEarlyDisplay() && !needTPIYuv();
}

MBOOL StreamingFeatureRequest::skipMDPDisplay() const
{
    return needEarlyDisplay();
}

MBOOL StreamingFeatureRequest::needRSC() const
{
    return mPipeUsage.supportRSCNode();
}

MBOOL StreamingFeatureRequest::needFSC() const
{
    return mPipeUsage.supportFSC() && HAS_FSC(mFeatureMask);
}

MBOOL StreamingFeatureRequest::needDump() const
{
    return mNeedDump;
}

MBOOL StreamingFeatureRequest::needNddDump() const
{
    return mP2DumpType == Feature::P2Util::P2_DUMP_NDD && mP2Pack.isValid();
}

MBOOL StreamingFeatureRequest::needRegDump() const
{
    return needDump() || needNddDump();
}

MBOOL StreamingFeatureRequest::is4K2K() const
{
    return mIs4K2K;
}

MUINT32 StreamingFeatureRequest::getMasterID() const
{
    return mMasterID;
}

MBOOL StreamingFeatureRequest::needTOF() const
{
    return mPipeUsage.supportTOF();
}

MUINT32 StreamingFeatureRequest::needTPILog() const
{
    return mTPILog;
}

MUINT32 StreamingFeatureRequest::needTPIDump() const
{
    return mTPIDump;
}

MUINT32 StreamingFeatureRequest::needTPIScan() const
{
    return mTPIScan;
}

MUINT32 StreamingFeatureRequest::needTPIBypass() const
{
    return mTPIBypass;
}

MUINT32 StreamingFeatureRequest::needTPIBypass(unsigned tpiNodeID) const
{
    return mTPIBypass || !mTPIFrame.needNode(tpiNodeID);
}

IMetadata* StreamingFeatureRequest::getAppMeta() const
{
    return getSensorInput().getValidAppIn();
}

MVOID StreamingFeatureRequest::getTPIMeta(TPIRes &res) const
{
    const SFPSensorInput &master = getSensorInput(mMasterID);
    const SFPIOMap &generalIO = mSFPIOManager.getFirstGeneralIO();

    res.setMeta(TPI_META_ID_MTK_IN_APP, master.getValidAppIn());
    res.setMeta(TPI_META_ID_MTK_IN_P1_HAL, master.mHalIn);
    res.setMeta(TPI_META_ID_MTK_IN_P1_APP, master.mAppDynamicIn);

    res.setMeta(TPI_META_ID_MTK_OUT_P2_APP, generalIO.mAppOut);
    res.setMeta(TPI_META_ID_MTK_OUT_P2_HAL, generalIO.mHalOut);

    if( mPipeUsage.supportDual() )
    {
        const SFPSensorInput &slave = getSensorInput(mSlaveID);
        res.setMeta(TPI_META_ID_MTK_IN_P1_HAL_2, slave.mHalIn);
        res.setMeta(TPI_META_ID_MTK_IN_P1_APP_2, slave.mAppDynamicIn);
    }
}

MBOOL StreamingFeatureRequest::isSlaveParamValid() const
{
    return mSlaveID != INVALID_SENSOR && hasSlave(mSlaveID);
}

MSizeF StreamingFeatureRequest::getEISMarginPixel() const
{
    MRectF region = this->getVar<MRectF>(SFP_VAR::EIS_RRZO_CROP, MRectF(0, 0));
    TRACE_FUNC("FSC(%d). RRZO crop(%f,%f)(%fx%f)", needFSC(),
               region.p.x, region.p.y, region.s.w, region.s.h);
    return MSizeF(region.p.x, region.p.y);
}

MRectF StreamingFeatureRequest::getEISCropRegion() const
{
    return this->getVar<MRectF>(SFP_VAR::EIS_RRZO_CROP, MRectF(0, 0));
}

MSize StreamingFeatureRequest::getFSCMaxMarginPixel() const
{
    MSize maxFSCMargin(0, 0);
    FSC::FSC_WARPING_DATA_STRUCT fscData;
    if( this->tryGetVar<FSC::FSC_WARPING_DATA_STRUCT>(SFP_VAR::FSC_RRZO_WARP_DATA, fscData) )
    {
        maxFSCMargin = MSize(FSC_CROP_INT(fscData.maxRRZOCropRegion.p.x), FSC_CROP_INT(fscData.maxRRZOCropRegion.p.y));
        TRACE_FUNC("Max FSC RRZO crop(%d,%d)(%dx%d)",
               FSC_CROP_INT(fscData.maxRRZOCropRegion.p.x), FSC_CROP_INT(fscData.maxRRZOCropRegion.p.y),
               FSC_CROP_INT(fscData.maxRRZOCropRegion.s.w), FSC_CROP_INT(fscData.maxRRZOCropRegion.s.h));
    }
    else
    {
        MY_LOGW("Cannot get FSC_WARPING_DATA_STRUCT. Use default (%dx%d)", maxFSCMargin.w, maxFSCMargin.h);
    }
    return maxFSCMargin;
}

MBOOL StreamingFeatureRequest::isP2ACRZMode() const
{
    return mIsP2ACRZMode;
}

EISQ_ACTION StreamingFeatureRequest::getEISQAction() const
{
    return mEISQState.mAction;
}

MUINT32 StreamingFeatureRequest::getEISQCounter() const
{
    return mEISQState.mCounter;
}

MBOOL StreamingFeatureRequest::useWarpPassThrough() const
{
    return mForceWarpPass;
}

MUINT32 StreamingFeatureRequest::getAppFPS() const
{
    return mAppFPS;
}

MUINT32 StreamingFeatureRequest::getNodeFPS() const
{
    return mNodeFPS;
}

MUINT32 StreamingFeatureRequest::getNodeCycleTimeMs() const
{
    return mNodeCycleTimeMs;
}

MBOOL StreamingFeatureRequest::useDirectGpuOut() const
{
    MBOOL val = MFALSE;
    if( mForceGpuRGBA == MFALSE )
    {
        if( mForceGpuOut )
        {
            val = (mForceGpuOut == FORCE_ON);
        }
        else
        {
            val = this->is4K2K() && !mPipeUsage.supportWPE() && !EISCustom::isEnabled4K2KMDP();
        }
    }
    return val;
}

MBOOL StreamingFeatureRequest::needPrintIO() const
{
    return mForcePrintIO;
}

void StreamingFeatureRequest::appendEisTag(string& str, MUINT32 mFeatureMask) const
{
    if( HAS_EIS(mFeatureMask) )
    {
        if(!str.empty())
        {
            str += "+";
        }
        str += TAG_EIS();

        if( mPipeUsage.supportEIS_Q() )
        {
            str += TAG_EIS_QUEUE();
        }
    }
}

#define APPEND_FEATURE_TAG(f, str, mask) \
{                               \
    if( HAS_##f(mask) )         \
    {                           \
        if(!str.empty())        \
        {                       \
            str += "+";         \
        }                       \
        str += TAG_##f();       \
    }                           \
}


void StreamingFeatureRequest::append3DNRTag(string& str, MUINT32 mFeatureMask) const
{
    APPEND_FEATURE_TAG(3DNR, str, mFeatureMask);
    APPEND_FEATURE_TAG(3DNR_S, str, mFeatureMask);
}

void StreamingFeatureRequest::appendRSCTag(string& str, MUINT32 mFeatureMask) const
{
    APPEND_FEATURE_TAG(RSC, str, mFeatureMask);
}

void StreamingFeatureRequest::appendDSDNTag(string& str, MUINT32 mFeatureMask) const
{
    APPEND_FEATURE_TAG(DSDN20, str, mFeatureMask);
    APPEND_FEATURE_TAG(DSDN20_S, str, mFeatureMask);
    APPEND_FEATURE_TAG(DSDN25, str, mFeatureMask);
    APPEND_FEATURE_TAG(DSDN25_S, str, mFeatureMask);
    APPEND_FEATURE_TAG(DSDN30, str, mFeatureMask);
    APPEND_FEATURE_TAG(DSDN30_S, str, mFeatureMask);
}

void StreamingFeatureRequest::appendFSCTag(string& str, MUINT32 mFeatureMask) const
{
    APPEND_FEATURE_TAG(FSC, str, mFeatureMask);
}

void StreamingFeatureRequest::appendVendorTag(string& str, MUINT32 mFeatureMask) const
{
    APPEND_FEATURE_TAG(TPI_YUV, str, mFeatureMask);
    APPEND_FEATURE_TAG(TPI_ASYNC, str, mFeatureMask);
}

#undef APPEND_FEATURE_TAG

void StreamingFeatureRequest::appendNoneTag(string& str, MUINT32 mFeatureMask) const
{
    if( mFeatureMask == 0 )
    {
        str += "NONE";
    }
}

void StreamingFeatureRequest::appendDefaultTag(string& str, MUINT32 mFeatureMask) const
{
    (void)(mFeatureMask);
    if(str.empty())
    {
        str += "UNKNOWN";
    }
}

BasicImgData toBasicImgData(const DualBasicImgData &data)
{
    return BasicImgData(data.mData.mMaster, data.mRequest);
}

VMDPReqData toVMDPReqData(const DualBasicImgData &data)
{
    VMDPReq vmdpReq;
    vmdpReq.mInputImg = data.mData;
    return VMDPReqData(vmdpReq, data.mRequest);
}

BasicImg P2GOut::getNext() const
{
    size_t count = mNextMap.size();
    if( count > 1 )
    {
        // MY_LOGW("unexpect nextMap size(%zu) > 1", count);
    }
    return count ? mNextMap.begin()->second.mImg : BasicImg();
}

} // NSFeaturePipe
} // NSCamFeature
} // NSCam
