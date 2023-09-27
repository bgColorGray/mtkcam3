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

// Module header file
#include <stereo_tuning_provider.h>
// Local header file
#include "WPENode.h"
#include "../DepthMapPipe_Common.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"

// Logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "WPENode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_WPE);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam::NSIoPipe;

#define WPENODE_USERNAME "VSDOF_WPE"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
WPENode::
WPENode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
, miSensorIdx_Main1(config.mpSetting->miSensorIdx_Main1)
{
    this->addWaitQueue(&mJobQueue);
}

WPENode::
~WPENode()
{
    MY_LOGD("[Destructor]");
}

MVOID
WPENode::
cleanUp()
{
    MY_LOGD("+");
    if(mpINormalStream != NULL)
    {
        mpINormalStream->uninit(WPENODE_USERNAME);
        mpINormalStream->destroyInstance();
        mpINormalStream = NULL;
    }

    MY_LOGD("-");
}

MBOOL
WPENode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
WPENode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
WPENode::
onThreadStart()
{
    CAM_ULOGM_TAGLIFE("WPENode::onThreadStart");
    VSDOF_INIT_LOG("+");
    // Create NormalStream, 0xFFF for WPE
    VSDOF_LOGD("NormalStream create instance");
    CAM_ULOGM_TAG_BEGIN("WPENode::NormalStream::createInstance+init");
    mpINormalStream = NSCam::NSIoPipe::NSPostProc::INormalStream::createInstance(miSensorIdx_Main1);

    if (mpINormalStream == NULL)
    {
        MY_LOGE("mpINormalStream create instance for WPE Node failed!");
        cleanUp();
        return MFALSE;
    }
    else
    {
        if(!mpINormalStream->init(WPENODE_USERNAME, NSCam::NSIoPipe::EStreamPipeID_WarpEG))
        {
            MY_LOGE("mpINormalStream init failed");
            cleanUp();
            return MFALSE;
        }
    }
    CAM_ULOGM_TAG_END();
    //
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
WPENode::
onThreadStop()
{
    CAM_ULOGM_TAGLIFE("WPENode::onThreadStop");
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
WPENode::
onData(DataID data, DepthMapRequestPtr &request)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : reqID=%d", request->getRequestNo());
    CAM_ULOGM_TAGLIFE("WPENode::onData");

    switch(data)
    {
        case N3D_TO_WPE_RECT2_WARPMTX:
            mJobQueue.enque(request);
            break;
        case N3D_TO_WPE_WARPMTX:
            mJobQueue.enque(request);
            break;
        default:
            MY_LOGW("Un-recognized data ID, id=%d reqID=%d", data, request->getRequestNo());
            ret = MFALSE;
            break;
    }

    VSDOF_LOGD("-");
    return ret;
}

MBOOL
WPENode::
onThreadLoop()
{
    DepthMapRequestPtr pRequest;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }

    if( !mJobQueue.deque(pRequest) )
    {
        MY_LOGE("mJobQueue.deque() failed");
        return MFALSE;
    }

    CAM_ULOGM_TAGLIFE("WPENode::onThreadLoop");
    pRequest->mTimer.startWPE();
    // mark on-going-pRequest start
    this->incExtThreadDependency();
    const EffectRequestAttrs& attr = pRequest->getRequestAttr();
    VSDOF_PRFLOG("threadLoop start, reqID=%d eState=%d needFEFM=%d",
                    pRequest->getRequestNo(), attr.opState, attr.needFEFM);
    //
    MBOOL bRet = MTRUE;
    EnqueCookieContainer *pCookieIns = new EnqueCookieContainer(pRequest, this);
    QParams enqueParams;
    bRet = mpFlowOption->buildQParam_Warp(pRequest, enqueParams, pCookieIns);
    // if(!prepareWPEEnqueParams(pRequest, enqueParams, pCookieIns))
        // goto lbExit;
    // callback
    enqueParams.mpfnCallback = onWPECallback;
    enqueParams.mpCookie = (MVOID*) pCookieIns;
    // enque
    CAM_ULOGM_TAG_BEGIN("WPENode::NormalStream::enque");
    VSDOF_LOGD("WPE enque start! reqID=%d", pRequest->getRequestNo());
    pRequest->mTimer.startWPEDrv();
    CAM_ULOGM_TAG_END();
    VSDOF_LOGD("WPE enque end! reqID=%d, result=%d", pRequest->getRequestNo(), bRet);
    bRet = mpINormalStream->enque(enqueParams);
    if(!bRet)
    {
        MY_LOGE("WPE enque failed! reqID=%d", pRequest->getRequestNo());
        goto lbExit;
    }
    return MTRUE;

lbExit:
    delete pCookieIns;
    // mark on-going-request end
    this->decExtThreadDependency();
    return MFALSE;

}

MVOID
WPENode::
onWPECallback(QParams& rParams)
{
    EnqueCookieContainer* pEnqueData = (EnqueCookieContainer*) (rParams.mpCookie);
    WPENode* pWPENode = (WPENode*) (pEnqueData->mpNode);
    pWPENode->handleWPEDone(rParams, pEnqueData);
}

MVOID
WPENode::
handleWPEDone(QParams& rParams, EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("WPENode::handleWPEDone");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    // stop timer
    pRequest->mTimer.stopWPEDrv();
    DumpConfig config;
    // check flush status
    if(mpNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH))
        goto lbExit;
    VSDOF_PRFTIME_LOG("+ :reqID=%d , WPE exec-time=%d ms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedWPEDrv());
    // remove WPENode from input request buffer map
    if (!pRequest->unRegisterInputBufferUser(this)) {
        MY_LOGE("unRegisterInputBufferUser failed");
    }
    this->configureToNext(pRequest);
lbExit:
    // launch onProcessDone
    pRequest->getBufferHandler()->onProcessDone(getNodeId());
    delete pEnqueCookie;
    VSDOF_LOGD("- :reqID=%d", pRequest->getRequestNo());
    // mark on-going-request end
    this->decExtThreadDependency();
    // stop overall timer
    pRequest->mTimer.stopWPE();
}

MBOOL
WPENode::
configureToNext(DepthMapRequestPtr pRequest)
{
    VSDOF_LOGD("+, reqID=%d", pRequest->getRequestNo());
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    //
    IImageBuffer *pImgBuf_SV_Y = nullptr;
    if(pBufferHandler->getEnqueBuffer(getNodeId(), BID_WPE_OUT_SV_Y, pImgBuf_SV_Y))
        pImgBuf_SV_Y->syncCache(eCACHECTRL_INVALID);
    // set output buffer
    if(pRequest->getRequestAttr().opState == eSTATE_CAPTURE)
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_WPE_OUT_SV_Y, eDPETHMAP_PIPE_NODEID_DLDEPTH);
        this->handleDataAndDump(WPE_TO_DLDEPTH_MV_SV, pRequest);
    }
    else
    {
        IImageBuffer *pImgBuf_MASK_S = nullptr;
        if(pBufferHandler->getEnqueBuffer(getNodeId(), BID_WPE_OUT_MASK_S, pImgBuf_MASK_S))
            pImgBuf_MASK_S->syncCache(eCACHECTRL_INVALID);
        pBufferHandler->configOutBuffer(getNodeId(), BID_WPE_OUT_SV_Y, eDPETHMAP_PIPE_NODEID_DVS);
        pBufferHandler->configOutBuffer(getNodeId(), BID_WPE_OUT_MASK_S, eDPETHMAP_PIPE_NODEID_DVS);
        this->handleDataAndDump(WPE_TO_DVS_WARP_IMG, pRequest);
    }

    VSDOF_LOGD("-, reqID=%d", pRequest->getRequestNo());
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
WPENode::
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

}; //NSFeaturePipe_DepthMap
}; //NSCamFeature
}; //NSCam

