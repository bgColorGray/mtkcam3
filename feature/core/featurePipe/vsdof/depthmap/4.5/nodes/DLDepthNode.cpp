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


// Standard C header file

// Android system/core header file

// mtkcam custom header file

// mtkcam global header file
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
// Module header file

// Local header file
#include "DLDepthNode.h"
#include "../DepthMapPipe_Common.h"
#include "../DepthMapPipeUtils.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"
// logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "DLDepthNode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_DLDEPTH);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DLDepthNode::
DLDepthNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
{
    this->addWaitQueue(&mJobQueue);
    this->addWaitQueue(&mJobQueue_MYS);
}

DLDepthNode::
~DLDepthNode()
{
}

MVOID
DLDepthNode::
cleanUp()
{
    VSDOF_LOGD("+");

    if(mpAIDepthHal != nullptr)
    {
        delete mpAIDepthHal;
        mpAIDepthHal = nullptr;
    }
    mJobQueue.clear();
    VSDOF_LOGD("-");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapPipeNode Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MBOOL
DLDepthNode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
DLDepthNode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
DLDepthNode::
onThreadStart()
{
    VSDOF_INIT_LOG("+");
    #if ( HAS_VAIDEPTH == 1 )
    mpAIDepthHal = VIDEO_AIDEPTH_HAL::createInstance();
    #endif
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
DLDepthNode::
onThreadStop()
{
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}


MBOOL
DLDepthNode::
onData(DataID data, DepthMapRequestPtr& pRequest)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : dataID=%s reqId=%d", ID2Name(data), pRequest->getRequestNo());

    switch(data)
    {
        // Capture
        case WPE_TO_DLDEPTH_MV_SV:
            mJobQueue.enque(pRequest);
            break;
        // Preview/Video
        case DVS_TO_DLDEPTH_MVSV_NOC:
            mJobQueue.enque(pRequest);
            break;
        case P2A_TO_DLDEPTH_MY_S:
            mJobQueue_MYS.enque(pRequest);
            break;
        default:
            MY_LOGW("Unrecongnized DataID=%d", data);
            ret = MFALSE;
            break;
    }

    VSDOF_LOGD("-");
    return ret;
}

MBOOL
DLDepthNode::
onThreadLoop()
{
    DepthMapRequestPtr pRequest;
    DepthMapRequestPtr pRequest_MYS;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mJobQueue.deque(pRequest) )
    {
        MY_LOGE("mJobQueue.deque() failed");
        return MFALSE;
    }
    if( !mJobQueue_MYS.deque(pRequest_MYS) )
    {
        MY_LOGE("mJobQueue.deque() failed");
        return MFALSE;
    }
    // sync frame
    if(pRequest->getRequestNo() != pRequest_MYS->getRequestNo())
    {
        MY_LOGE("Request number not consistent! reqID=%d   MYS reqID=%d",
                pRequest->getRequestNo(), pRequest_MYS->getRequestNo());
        return MFALSE;
    }

    // start timer
    pRequest->mTimer.startDLDepth();
    // mark on-going-request start
    this->incExtThreadDependency();
    VSDOF_PRFLOG("threadLoop start, reqID=%d", pRequest->getRequestNo());
    CAM_ULOGM_TAGLIFE("DLDepthNode::onThreadLoop");
    MBOOL bRet = this->runDLDepth(pRequest);;
    // timer
    pRequest->mTimer.stopDLDepth();
    // error handling
    if(!bRet)
    {
#ifdef UNDER_DEVELOPMENT
         AEE_ASSERT("[DLDepth] reqID=%d runDLDepth failed", pRequest->getRequestNo());
#endif
        MY_LOGE("DLDepth operation failed: reqID=%d", pRequest->getRequestNo());
        handleData(ERROR_OCCUR_NOTIFY, pRequest);
    }
    // mark on-going-request end
    this->decExtThreadDependency();
    return bRet;
}

MVOID
DLDepthNode::
onFlush()
{
    MY_LOGD("+ extDep=%d", this->getExtThreadDependency());
    DepthMapRequestPtr pRequest;
    while( mJobQueue.deque(pRequest) )
    {
        sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
        pBufferHandler->onProcessDone(getNodeId());
    }
    DepthMapPipeNode::onFlush();
    MY_LOGD("-");
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DLDepthNode Private Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MBOOL
DLDepthNode::
runDLDepth(DepthMapRequestPtr pRequest)
{
    VSDOF_LOGD("+ reqID=%d", pRequest->getRequestNo());
    //
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // prepare input parameter
    IImageBuffer *pImg_MYS = nullptr;
    VIDEO_AIDEPTH_HAL_PARAMS inParam;
    inParam.mainCamPos = StereoSettingProvider::getSensorRelativePosition();
    MBOOL bRet = MTRUE;
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_N3D_OUT_MV_Y, inParam.imageMain1);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_WPE_OUT_SV_Y, inParam.imageMain2);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_OCC_OUT_NOC_M, inParam.disparityMap);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_OCC_OUT_CFM_M, inParam.confidenceMap);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_MY_S, inParam.coloredImage);
    //
    IMetadata* inP1RetHal = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_P1_RETURN);
    if(!tryGetMetadata<MINT32>(inP1RetHal, MTK_SENSOR_SENSITIVITY, inParam.iso))
    {
        MY_LOGE("Failed to get ISO value.");
        return MFALSE;
    }
    IMetadata* inApp = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_APP);
    if(!tryGetMetadata<MINT64>(inApp, MTK_SENSOR_EXPOSURE_TIME, inParam.exposureTime))
    {
        MY_LOGE("Failed to get exposure value.");
        return MFALSE;
    }

    if(!checkToDump(DLDEPTH_TO_GF_DEPTHMAP, pRequest))
    {
        inParam.dumpHint = nullptr;
    }
    else
    {
        IMetadata* inHal_Main1 = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN1);
        extract(&mDumpHint_Main1, inHal_Main1);
        inParam.dumpHint = &mDumpHint_Main1;
        inParam.dumpLevel = miNDDLevel;
    }

    // output param
    VIDEO_AIDEPTH_HAL_OUTPUT outParam;
    outParam.depthMap = pBufferHandler->requestBuffer(getNodeId(), BID_DLDEPTH_INTERNAL_DEPTHMAP);
    if(outParam.depthMap == nullptr)
    {
        MY_LOGE("Failed to get depth buffer.");
        return MFALSE;
    }

    pRequest->mTimer.startDLDepthAlgo();
    bRet = mpAIDepthHal->VideoAIDepthHALRun(inParam, outParam);
    pRequest->mTimer.stopDLDepthAlgo();
    VSDOF_LOGD("reqID=%d finish the DLDepth execution, status=%d, exec-time=%dms",
                pRequest->getRequestNo(), bRet, pRequest->mTimer.getElapsedDLDepthAlgo());

    if ( mpFlowOption->checkConnected(DLDEPTH_TO_SLANT_DEPTH) ) {
        // DLDEPTH to SLANT
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M, eDPETHMAP_PIPE_NODEID_SLANT);
        pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_MY_S, eDPETHMAP_PIPE_NODEID_SLANT);
        pBufferHandler->configOutBuffer(getNodeId(), BID_DLDEPTH_INTERNAL_DEPTHMAP, eDPETHMAP_PIPE_NODEID_SLANT);
        // deliver to GF if debugInk need only
        if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_NOC)
            pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_NOC_M, eDPETHMAP_PIPE_NODEID_SLANT);
        handleDataAndDump(DLDEPTH_TO_SLANT_DEPTH, pRequest);
    }
    else if ( mpFlowOption->checkConnected(DLDEPTH_TO_GF_DEPTHMAP) )
    {
        // DLDEPTH TO GF
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M, eDPETHMAP_PIPE_NODEID_GF);
        pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_MY_S, eDPETHMAP_PIPE_NODEID_GF);
        pBufferHandler->configOutBuffer(getNodeId(), BID_DLDEPTH_INTERNAL_DEPTHMAP, eDPETHMAP_PIPE_NODEID_GF);
        // deliver to GF if debugInk need only
        if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_NOC)
            pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_NOC_M, eDPETHMAP_PIPE_NODEID_GF);
        handleDataAndDump(DLDEPTH_TO_GF_DEPTHMAP, pRequest);
    }
    else if( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_AI_DEPTH )
    {
        bRet &= onHandle3rdPartyFlowDone(pRequest);
    }
    else
    {
        MY_LOGE("undefined handle DLDEPTH done");
        bRet = MFALSE;
    }
    // handle process done
    pBufferHandler->onProcessDone(getNodeId());
    VSDOF_LOGD("- reqID=%d", pRequest->getRequestNo());
    return bRet;
}

};
};
};
