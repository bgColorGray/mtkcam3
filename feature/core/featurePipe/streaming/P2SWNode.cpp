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

#include "P2SWNode.h"

#define PIPE_CLASS_TAG "P2SWNode"
#define PIPE_TRACE TRACE_P2SW_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam/aaa/IIspMgr.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_P2SW);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

P2SWNode::P2SWNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_P2SW)
    , StreamingFeatureNode(name)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mP2GSWDatas);

    TRACE_FUNC_EXIT();
}

P2SWNode::~P2SWNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL P2SWNode::onInit()
{
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "P2SW:onInit");
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();

    if( mPipeUsage.support3DNRRSC() )
    {
        mWaitRSC = MTRUE;
        this->addWaitQueue(&mRSCDatas);
        MY_LOGD("P2SW add RSC waitQueue for 3DNR");
    }

    if( mPipeUsage.support3DNRLMV() )
    {
        mUseLMV = MTRUE;
    }

    TRACE_FUNC_EXIT();
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    return (mP2GMgr != NULL);
}

MBOOL P2SWNode::onUninit()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SWNode::onThreadStart()
{
    P2_CAM_TRACE_CALL(TRACE_DEFAULT);
    TRACE_FUNC_ENTER();

    mP2GMgr->allocateP2SWBuffer();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SWNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2SWNode::onData(DataID id, const P2GSWData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    switch( id )
    {
    case ID_P2G_TO_P2SW:
        mP2GSWDatas.enque(data);
        ret = MTRUE;
        break;
    default:
        ret = MFALSE;
        break;
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL P2SWNode::onData(DataID id, const RSCData &data)
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

MBOOL P2SWNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    P2GSWData swData;
    RSCData rscData;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mP2GSWDatas.deque(swData) )
    {
        MY_LOGE("P2GSWData deque out of sync");
        return MFALSE;
    }
    else if( swData.mRequest == NULL )
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
        if( swData.mRequest != rscData.mRequest )
        {
            MY_LOGE("P2SW RSCData out of sync request(%d) rsc(%d)", swData.mRequest->mRequestNo, rscData.mRequest->mRequestNo);
            return MFALSE;
        }
    }

    TRACE_FUNC_ENTER();
    RequestPtr request = swData.mRequest;
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    request->mTimer.startP2SW();
    markNodeEnter(request);
    processP2SW(request, swData.mData, rscData.mData);
    request->mTimer.stopP2SW();
    MY_S_LOGD(request->mLog, "sensor(%d) Frame %d enque done in %d ms", mSensorIndex, request->mRequestNo, request->mTimer.getElapsedP2SW());

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID P2SWNode::processP2SW(const RequestPtr &request, const P2SWHolder &sw, const RSCResult &rsc)
{
    TRACE_FUNC_ENTER();
    updateMotion(request, sw, rsc);
    if ( request->isSlaveParamValid() )
        updateMotion(request, sw, rsc, MTRUE);
    runOMCSW(request, sw.mP2SW);

    if( mMotionNotify )
    {
        mMotionNotify->notify(request->mRequestNo);
    }

    runP2SW(request, sw.mP2SW);
    handleResultData(request);
    TRACE_FUNC_EXIT();
}

MVOID P2SWNode::updateMotion(const RequestPtr &request, const P2SWHolder &sw, const RSCResult &rsc, MBOOL runSlave)
{
    TRACE_FUNC_ENTER();
    if( (mWaitRSC || mUseLMV) && (sw.mMotion || sw.mSlaveMotion ) )
    {
        mP2GMgr->runUpdateMotion(request, sw, rsc, runSlave);
    }
    TRACE_FUNC_EXIT();
}

MVOID P2SWNode::runOMCSW(const RequestPtr & /*request*/, const std::vector<P2G::P2SW> &p2sw)
{
    TRACE_FUNC_ENTER();
    mP2GMgr->runP2SW(p2sw, P2G::P2SW::OMC);
    TRACE_FUNC_EXIT();
}

MVOID P2SWNode::runP2SW(const RequestPtr &request, const std::vector<P2G::P2SW> &p2sw)
{
    TRACE_FUNC_ENTER();
    mP2GMgr->waitFrameDepDone(request->mRequestNo);
    request->mTimer.startP2SWTuning();
    mP2GMgr->runP2SW(p2sw);
    request->mTimer.stopP2SWTuning();
    TRACE_FUNC_EXIT();
}

MVOID P2SWNode::handleResultData(const RequestPtr &request)
{
    TRACE_FUNC_ENTER();
    markNodeExit(request);
    handleData(ID_P2SW_TO_MSF, request);
    TRACE_FUNC_EXIT();
}

MVOID P2SWNode::setMotionNotifyCtrl(std::shared_ptr<FrameControl> &notify)
{
    mMotionNotify = notify;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

