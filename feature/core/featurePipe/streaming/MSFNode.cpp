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

#include "MSFNode.h"
#include <mtkcam/drv/iopipe/CamIO/IHalCamIO.h>

#define PIPE_CLASS_TAG "MSFNode"
#define PIPE_TRACE TRACE_MSF_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_MSF);

using namespace NSCam::NSIoPipe;
using namespace NSCam::Feature::P2Util;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

MSFNode::MSFNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_MSF)
    , StreamingFeatureNode(name)
    , mDIPStream(NULL)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mMSSReqs);
    this->addWaitQueue(&mP2SWReqs);
    this->addWaitQueue(&mP2GHWDatas);

    mForceExpectMS = getPropertyValue("vendor.debug.fpipe.p2a_g.expect", 0);
    mForceSmvrHighClk = getPropertyValue("vendor.debug.smvrb.highclk", 1);
    mForceDSDN30HighClk = getPropertyValue("vendor.debug.dsdn30.highclk", 1);

    TRACE_FUNC_EXIT();
}

MSFNode::~MSFNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID MSFNode::setDIPStream(Feature::P2Util::DIPStream *stream)
{
    TRACE_FUNC_ENTER();
    mDIPStream = stream;
    TRACE_FUNC_EXIT();
}

MBOOL MSFNode::onInit()
{
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "MSF:onInit");
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();

    mNeedPMSS = mPipeUsage.supportDSDN30();
    if( mNeedPMSS )
    {
        this->addWaitQueue(&mPMSSReqs);
    }

    TRACE_FUNC_EXIT();
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    return (mP2GMgr != NULL);
}

MBOOL MSFNode::onUninit()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSFNode::onThreadStart()
{
    P2_CAM_TRACE_CALL(TRACE_DEFAULT);
    TRACE_FUNC_ENTER();
    mP2GMgr->allocateP2HWBuffer();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSFNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    this->waitDIPStreamBaseDone();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MSFNode::onData(DataID id, const RequestPtr &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    switch( id )
    {
    case ID_MSS_TO_MSF:
        mMSSReqs.enque(data);
        ret = MTRUE;
        break;
    case ID_PMSS_TO_MSF:
        mPMSSReqs.enque(data);
        ret = MTRUE;
        break;
    case ID_P2SW_TO_MSF:
        mP2SWReqs.enque(data);
        ret = MTRUE;
        break;
    default:
        ret = MFALSE;
        break;
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL MSFNode::onData(DataID id, const P2GHWData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s  rsc_data arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    if( id == ID_P2G_TO_MSF )
    {
        mP2GHWDatas.enque(data);
        ret = MTRUE;
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL MSFNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr mssReq, p2swReq, pmssReq;
    P2GHWData enqData;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mMSSReqs.deque(mssReq) || !mP2SWReqs.deque(p2swReq) || !mP2GHWDatas.deque(enqData) || (mNeedPMSS && !mPMSSReqs.deque(pmssReq)) )
    {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    }
    else if( mssReq == NULL || p2swReq == NULL || enqData.mRequest == NULL || (mNeedPMSS && pmssReq == NULL))
    {
        MY_LOGE("Dequed Request is NULL!! needPMSS(%d), mss/p2sw/p2a/pmss exist = (%d/%d/%d/%d)", mNeedPMSS, (mssReq != NULL), (p2swReq != NULL), (enqData.mRequest != NULL), (pmssReq != NULL));
        return MFALSE;
    }

    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, enqData.mRequest);

    enqData.mRequest->mTimer.startMSF();
    markNodeEnter(enqData.mRequest);
    processMSF(enqData.mRequest, enqData);
    enqData.mRequest->mTimer.stopMSF();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MUINT32 MSFNode::getExpectMS(const RequestPtr &request) const
{
    TRACE_FUNC_ENTER();

    MUINT32 ret = request->getNodeCycleTimeMs();
    MBOOL isSmvrNeedHigh = (mForceSmvrHighClk && mPipeUsage.getSMVRSpeed() > 4 && request->needDSDN25(request->mMasterID));
    MBOOL isDsdn30NeedHigh = (mForceDSDN30HighClk && mPipeUsage.support4K2K() /*&& (ret < 30)*/
                            && request->needDSDN30(request->mMasterID));
    if (isSmvrNeedHigh || isDsdn30NeedHigh)
    {
        ret = 1;
    }

    TRACE_FUNC_EXIT();
    return mForceExpectMS ? mForceExpectMS : ret;
}

MBOOL MSFNode::processMSF(const RequestPtr &request, const P2GHWData &data)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    Feature::P2Util::DIPParams params;

    params = mP2GMgr->runP2HW(data.mData.mP2HW);

    MUINT32 expectMS = getExpectMS(request);
    NSCam::Feature::P2Util::updateExpectEndTime(params, expectMS);
    NSCam::Feature::P2Util::updateScenario(params, request->getAppFPS(), mPipeUsage.getStreamingSize().w);

    static unsigned sCount = 0;

    if( request->needPrintIO() &&
        (data.mData.mP2HW.size() < 8 || ++sCount < 4) )
    {
        NSCam::Feature::P2Util::printDIPParams(request->mLog, params);
        for( const P2G::P2HW &p2hw : data.mData.mP2HW )
        {
            if( p2hw.mIn.mTuningData )
            {
                printTuningParam(request->mLog, p2hw.mIn.mTuningData->mParam);
            }
        }
    }
    MSFEnqueData enqueData(data);
    enqueFeatureStream(params, enqueData);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID MSFNode::enqueFeatureStream(Feature::P2Util::DIPParams &params, const MSFEnqueData &data)
{
    TRACE_FUNC_ENTER();
    MUINT32 idM = data.mRequest->mMasterID;
    MUINT32 idS = data.mRequest->mSlaveID;
    MUINT32 dsdn = data.mRequest->needDSDN30(idM) ? 30 :
                   data.mRequest->needDSDN25(idM) ? 25 :
                   data.mRequest->needDSDN20(idM) ? 20 : 0;
    MUINT32 dsdnS = data.mRequest->needDSDN30(idS) ? 30 :
                   data.mRequest->needDSDN25(idS) ? 25 :
                   data.mRequest->needDSDN20(idS) ? 20 : 0;

    MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d enque start: size=%zu dsdn(%d) dsdn_s(%d), endTime(%d), earlyCB(%d)",
              mSensorIndex, data.mRequest->mRequestNo, params.mvDIPFrameParams.size(), dsdn, dsdnS, getExpectMS(data.mRequest), params.mEarlyCBIndex);
    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_P2));
    data.mRequest->mTimer.startEnqueMSF();
    this->incExtThreadDependency();
    this->enqueDIPStreamBase(mDIPStream, params, data);
    TRACE_FUNC_EXIT();
}

MVOID MSFNode::onDIPStreamBaseCB(const Feature::P2Util::DIPParams &params, const MSFEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing MSFNode class members
    TRACE_FUNC_ENTER();
    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Missing request");
    }
    else
    {
        request->mTimer.stopEnqueMSF();
        MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d enque done in %d ms, result = %d", mSensorIndex, request->mRequestNo, request->mTimer.getElapsedEnqueMSF(), params.mDequeSuccess);
        CAM_ULOGM_ASSERT(params.mDequeSuccess, "Frame %d enque result failed", request->mRequestNo);

        request->updateResult(params.mDequeSuccess);
        handleResultData(data.mP2GHWData);
        request->mTimer.stopMSF();
    }

    this->decExtThreadDependency();
    TRACE_FUNC_EXIT();
}

MVOID MSFNode::onDIPStreamBaseEarlyCB(const MSFEnqueData &data)
{
    TRACE_FUNC_ENTER();
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Missing request");
    }
    else
    {
        MY_S_LOGD(data.mRequest->mLog, "sensor(%d) Frame %d EARLY Callback", mSensorIndex, request->mRequestNo);
        if( mLastP2L1Nofiy ) mLastP2L1Nofiy->notify(request->mRequestNo);
    }
    TRACE_FUNC_EXIT();
}

MVOID MSFNode::handleResultData(const P2GHWData &data)
{
    // This function is not thread safe,
    // because it is called by onDIPParamsCB,
    // avoid accessing MSFNode class members
    TRACE_FUNC_ENTER();
    RequestPtr req = data.mRequest;
    if( mLastP2L1Nofiy ) mLastP2L1Nofiy->notify(req->mRequestNo);
    mP2GMgr->notifyFrameDone(req->mRequestNo);
    markNodeExit(req);

    handleData(ID_P2G_TO_PMDP, data);
    TRACE_FUNC_EXIT();
}

MVOID MSFNode::setLastP2L1NotifyCtrl(std::shared_ptr<FrameControl> &notify)
{
    mLastP2L1Nofiy = notify;
}


} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

