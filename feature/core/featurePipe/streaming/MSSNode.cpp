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

#include "MSSNode.h"

#if SUPPORT_MSS || SIMULATE_MSS

#define PIPE_CLASS_TAG "MSSNode"
#define PIPE_TRACE TRACE_MSS_NODE
#include <featurePipe/core/include/PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_MSS);

#include <mtkcam/drv/def/mfbcommon_v20.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

#define MSS_STREAM_NAME "FeaturePipe_MSS"

using namespace NSCam::NSIoPipe::NSMFB20;

const std::vector<DumpFilter> MSSNode::sFilterTable =
{
    DumpFilter( MSSNode::MASK_MSSO,          "msso" ,       MTRUE),
    DumpFilter( MSSNode::MASK_OMC,           "omcmv",       MTRUE),
};

static MVOID wait(std::shared_ptr<FrameControl> &ctrl, MUINT32 reqNo, MUINT32 diff = 0)
{
    if( ctrl )
    {
        diff > 0 ? ctrl->wait(reqNo, diff) : ctrl->wait(reqNo);
    }
}

static MVOID notify(std::shared_ptr<FrameControl> &ctrl, MUINT32 reqNo)
{
    if( ctrl )
    {
        ctrl->notify(reqNo);
    }
}

MSSNode::MSSNode(const char *name, MSSNode::Type type)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_MSS)
    , StreamingFeatureNode(name)
    , mType(type)
    , mIsPMSS(isPMSS())
    , mTypeName(mIsPMSS ? "PMSS" : "MSS")
    , mMSSStream(NULL)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mDatas);
    TRACE_FUNC_EXIT();
}

MSSNode::~MSSNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL MSSNode::isPMSS() const
{
    return mType == T_PREVIOUS;
}

MBOOL MSSNode::onData(DataID id, const MSSData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_P2G_TO_MSS || id == ID_P2G_TO_PMSS )
    {
        mDatas.enque(data);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL MSSNode::onInit()
{
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();
    enableDumpMask(sFilterTable);
    mReadBack = (mPipeUsage.getP2RBMask() & P2RB_BIT_ENABLE) && (mPipeUsage.getP2RBMask() & P2RB_BIT_READ_P2INBUF);

    MBOOL ret = MFALSE;
    MY_LOGI("create & init MSSStream ++");
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "MSS:init stream");
#if SIMULATE_MSS
    ret = MTRUE;
#else // SIMULATE_MSS
    mMSSStream = NSCam::NSIoPipe::NSEgn::IEgnStream<MSSConfig>::createInstance(MSS_STREAM_NAME);

    if( !mMSSStream )
    {
        MY_LOGE("Failed to create mss module");
        return ret;
    }

    if( mMSSStream )
    {
        ret = mMSSStream->init();
    }
#endif // SIMULATE_MSS

    P2_CAM_TRACE_END(TRACE_ADVANCED);
    MY_LOGI("create & init MSSStream --");

    ret = ret && (mP2GMgr != NULL);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL MSSNode::onUninit()
{
    TRACE_FUNC_ENTER();
    if( mMSSStream )
    {
        mMSSStream->uninit();
        mMSSStream->destroyInstance(MSS_STREAM_NAME);
        mMSSStream = NULL;
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSSNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    if( mIsPMSS )
    {
        mP2GMgr->allocatePDsBuffer();
    }
    else
    {
        mP2GMgr->allocateDsBuffer();
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSSNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    this->waitMSSStreamBaseDone();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSSNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr request;
    MSSData data;
    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mDatas.deque(data) )
    {
        MY_LOGE("%s Request deque out of sync", mTypeName.c_str());
        return MFALSE;
    }
    if( data.mRequest == NULL )
    {
        MY_LOGE("%s Request out of sync", mTypeName.c_str());
        return MFALSE;
    }

    TRACE_FUNC_ENTER();
    request = data.mRequest;
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    MBOOL needMSS = !data.mData.empty();
    startMSSTimer(request);
    markNodeEnter(request);
    TRACE_FUNC("%s Frame %d in MSS needMSS %d", mTypeName.c_str(), request->mRequestNo, needMSS);

    if( needMSS )
    {
        processMSS(data);
    }
    else
    {
        handleResultData(request);
    }

    stopMSSTimer(request);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSSNode::processMSS(MSSData &data)
{
    TRACE_FUNC_ENTER();
    waitBufferReady(data);

    MSSParam param;
    param.mpEngineID = NSIoPipe::eMSS;

    for( P2G::MSS &mssPath : data.mData )
    {
        param.mEGNConfigVec.push_back( prepareMSSConfig(data.mRequest, mssPath) );
    }

    MSSEnqueData enqueData(data);
    enqueMSSStream(param, enqueData);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID MSSNode::waitBufferReady(const MSSData &data)
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    if( mIsPMSS )
    {
        wait(mLastP2L1Wait, data.mRequest->mRequestNo, 1);
        wait(mMSSWait, data.mRequest->mRequestNo);
        wait(mMotionWait, data.mRequest->mRequestNo);
    }
    else
    {
        wait(mMSSWait, data.mRequest->mRequestNo, 1);
    }
    TRACE_FUNC_EXIT();
}

MSSConfig MSSNode::prepareMSSConfig(const RequestPtr &request, P2G::MSS &path)
{
    MSSConfig out;
    out.tag = EStreamTag_NORM;
    out.fps = request->getAppFPS();
    out.dump_en = (request->needNddDump() || request->needDump()) ? 1 : 0;
    if( out.dump_en )
    {
        out.dumphint = (&request->mP2Pack.getSensorData(path.mIn.mSensorID).mNDDHint);
    }
    if( path.mIn.mMSSOSizes.size() != path.mOut.mMSSOs.size() )
    {
        MY_LOGF("%s mMSSOSize(%zu) != mMSSO(%zu)", mTypeName.c_str(), path.mIn.mMSSOSizes.size(), path.mOut.mMSSOs.size());
    }
    //out.scale_Total = path.mOut.mMSSOs.size() + 1;

    if( mIsPMSS )
    {
        out.fps *= 3;
        out.mss_scenario = MSS_REFE_S1_MODE; // not support SMVR now
        out.omc_tuningBuf = (unsigned int*)(P2G::peak(path.mIn.mOMCTuning));
        out.mss_mvMap = P2G::peak(path.mIn.mOMCMV);
        out.mss_omcFrame = P2G::peak(path.mIn.mMSSI);
        out.scale_Total = path.mOut.mMSSOs.size();

        if( mReadBack )
        {
            readBack(request, path.mIn.mSensorID, ReadBack::TARGET_PMSSI, out.mss_omcFrame);
            readBack(request, path.mIn.mSensorID, ReadBack::TARGET_OMCMV, out.mss_mvMap);
        }
    }
    else
    {
        out.mss_scenario = path.mFeature.has(P2G::Feature::SMVR_MAIN | P2G::Feature::SMVR_SUB)
                        ? MSS_BASE_S1_SMVR_MODE : MSS_BASE_S1_MODE;
        out.mss_scaleFrame.push_back(P2G::peak(path.mIn.mMSSI));
        out.scale_Total = path.mOut.mMSSOs.size() + 1;
    }

    out.mss_scaleFrame.reserve(out.scale_Total);
    const size_t size = path.mOut.mMSSOs.size();
    for( size_t i = 0; i < size; ++i )
    {
        IImageBuffer *buf = NULL;
        if( path.mOut.mMSSOs[i] )
        {
            path.mOut.mMSSOs[i]->acquire();
            buf = path.mOut.mMSSOs[i]->getImageBufferPtr();
        }
        if( buf != NULL && buf->setExtParam(path.mIn.mMSSOSizes[i]) )
        {
            out.mss_scaleFrame.push_back(buf);
        }
        else
        {
            MY_LOGF("%s get MSSO buf(%p) failed ! or setExtParam failed!", mTypeName.c_str(), buf);
        }
    }

    return out;
}

MVOID MSSNode::enqueMSSStream(const MSSParam &param, const MSSEnqueData &data)
{
    P2_CAM_TRACE_ASYNC_BEGIN(TRACE_ADVANCED, mIsPMSS ? "PMSS:enqueMSSStream" : "MSS:enqueMSSStream", data.mRequest->mRequestNo);
    TRACE_FUNC_ENTER();

    MUINT32 run = data.mMSSData.mData.size();
    MUINT32 lv = run ? data.mMSSData.mData[0].mIn.mMSSOSizes.size() : 0;
    MSize max = lv ? data.mMSSData.mData[0].mIn.mMSSOSizes[0] : MSize(0,0);
    MSize min = lv ? data.mMSSData.mData[0].mIn.mMSSOSizes[lv-1] : MSize(0,0);

    MY_S_LOGD(data.mRequest->mLog,
        "%s sensor(%d) Frame %d mss enque start: "
        "run=%d lv=%d min=%dx%d max=%dx%d",
        mTypeName.c_str(), mSensorIndex, data.mRequest->mRequestNo,
        run, lv, min.w, min.h, max.w, max.h);

    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_MSS));
    startMSSEnqTimer(data.mRequest);
    this->incExtThreadDependency();
#if SIMULATE_MSS
    MY_LOGW("MSS Not Ready Yet!! using empty buffer as MSS output");
    onMSSStreamBaseCB(param, data);
#else // SIMULATE_MSS
    this->enqueMSSStreamBase(mMSSStream, param, data);
#endif // SIMULATE_MSS

    TRACE_FUNC_EXIT();
}

MVOID MSSNode::onMSSStreamBaseCB(const MSSParam &param, const MSSEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing MSSNode class memebers
    TRACE_FUNC_ENTER();
    (void)(param);
    this->decExtThreadDependency();

    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Missing request");
    }
    else
    {
        stopMSSEnqTimer(request);

        MY_S_LOGD(request->mLog, "%s sensor(%d) Frame %d mss enque done in %d ms", mTypeName.c_str(), mSensorIndex, request->mRequestNo, request->mTimer.getElapsedEnqueMSS());

        handleDump(request, data.mMSSData);

        handleResultData(request);

        P2_CAM_TRACE_ASYNC_END(TRACE_ADVANCED, mIsPMSS ? "PMSS:enqueMSSStream" : "MSS:enqueMSSStream", request->mRequestNo);
        stopMSSTimer(request);
    }
    TRACE_FUNC_EXIT();
}

MVOID MSSNode::dump(const RequestPtr &request, IImageBuffer *buffer, enum DumpMaskIndex mask, MBOOL isDump, const char *dumpStr, MBOOL isNDD, const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const TuningUtils::YUV_PORT &port, const char *nddStr) const
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

MVOID MSSNode::readBack(const RequestPtr &request, MUINT32 sensorID, ReadBack target, IImageBuffer *buffer)
{
    const Feature::P2Util::P2SensorData &sensorData = request->mP2Pack.getSensorData(sensorID);
    const MINT magic3A = sensorData.mMagic3A;

    const int32_t MAX_KEY_LEN = 128;
    const char *key = NULL;
    switch(target)
    {
        case ReadBack::TARGET_PMSSI:  key = "mss_1_omcframe"; break;
        case ReadBack::TARGET_OMCMV:  key = "mss_1_omcmv";    break;
        default: break;
    }
    if( !key || !buffer )
    {
        MY_LOGE("Set ReadBack key or buf failed !  target(%d), buf(%p)", target, buffer);
    }
    else
    {
        Feature::P2Util::readbackBuf(PIPE_CLASS_TAG, sensorData.mNDDHint, mReadBack,
            magic3A, key, buffer);
    }
}

MVOID MSSNode::handleDump(const RequestPtr &request, const MSSData &data) const
{
    TRACE_FUNC_ENTER();
    MBOOL isDump = request->needDump();
    MBOOL isNDD = request->needNddDump();

    if( isDump || isNDD )
    {
        TuningUtils::FILE_DUMP_NAMING_HINT hintM, hintS;
        hintM = request->mP2Pack.getSensorData(request->mMasterID).mNDDHint;
        hintS = request->mP2Pack.getSensorData(request->mSlaveID).mNDDHint;

        uint32_t smvrIndex = 0;
        for( const P2G::MSS &mssPath : data.mData )
        {

            if( mssPath.mFeature.has(P2G::Feature::SMVR_SUB) )
            {
                smvrIndex += 1;
            }

            uint32_t dsdnIndex = 1;
            for( const P2G::ILazy<P2G::GImg> &buf : mssPath.mOut.mMSSOs )
            {
                char str[64];
                int res = snprintf(str, sizeof(str), "dsdn25-msfp2-L%d-%s-s%d-sm%d", dsdnIndex, mIsPMSS ? "ref" : "in", mssPath.mIn.mSensorID, smvrIndex);
                MY_LOGE_IF( (res < 0), "snprintf failed!");
                if( buf )
                {
                    dump(request, buf->getImageBufferPtr(), MASK_MSSO, isDump,
                        str, isNDD,
                        (mssPath.mIn.mSensorID == request->mMasterID) ? hintM : hintS,
                        TuningUtils::YUV_PORT_NULL, str);
                }
                dsdnIndex++;
            }

            if( mssPath.mIn.mOMCMV )
            {
                char str[64];
                int res = snprintf(str, sizeof(str), "dsdn25-omcmv-s%d-sm%d", mssPath.mIn.mSensorID, smvrIndex);
                MY_LOGE_IF( (res < 0), "snprintf failed!");
                dump(request, mssPath.mIn.mOMCMV->getImageBufferPtr(), MASK_OMC, isDump,
                    str, isNDD,
                    (mssPath.mIn.mSensorID == request->mMasterID) ? hintM : hintS,
                    TuningUtils::YUV_PORT_NULL, str);

            }
        }
    }

    TRACE_FUNC_EXIT();
}

MVOID MSSNode::handleResultData(const RequestPtr &request)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    markNodeExit(request);
    handleData(mIsPMSS ? ID_PMSS_TO_MSF : ID_MSS_TO_MSF, request);
    notify(mMSSNotify, request->mRequestNo);
}

MVOID MSSNode::startMSSTimer(const RequestPtr &request) const
{
    mIsPMSS ? request->mTimer.startPMSS() : request->mTimer.startMSS();
}
MVOID MSSNode::stopMSSTimer(const RequestPtr &request) const
{
    mIsPMSS ? request->mTimer.stopPMSS() : request->mTimer.stopMSS();
}
MVOID MSSNode::startMSSEnqTimer(const RequestPtr &request) const
{
    mIsPMSS ? request->mTimer.startEnquePMSS() : request->mTimer.startEnqueMSS();
}
MVOID MSSNode::stopMSSEnqTimer(const RequestPtr &request) const
{
    mIsPMSS ? request->mTimer.stopEnquePMSS() : request->mTimer.stopEnqueMSS();
}

MVOID MSSNode::setMSSWaitCtrl(std::shared_ptr<FrameControl> &wait)
{
    mMSSWait = wait;
}

MVOID MSSNode::setMSSNotifyCtrl(std::shared_ptr<FrameControl> &notify)
{
    mMSSNotify = notify;
}

MVOID MSSNode::setMotionWaitCtrl(std::shared_ptr<FrameControl> &wait)
{
    mMotionWait = wait;
}

MVOID MSSNode::setLastP2L1WaitCtrl(std::shared_ptr<FrameControl> &wait)
{
    mLastP2L1Wait = wait;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // SUPPORT_MSS
