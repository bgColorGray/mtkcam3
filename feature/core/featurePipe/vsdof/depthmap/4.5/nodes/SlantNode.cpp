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
#include "SlantNode.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"

// Logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "SlantNode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_WPE);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {


#define WPENODE_USERNAME "VSDOF_SLANTWARP"
#define NOC_DEFAULT_VALUE 255
#define DEPTH_DEFAULT_VALUE 255
#define CFM_DEFAULT_VALUE 0

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
SlantNode::
SlantNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
{
    this->addWaitQueue(&mJobQueue);
}

//************************************************************************
//
//************************************************************************
SlantNode::
~SlantNode()
{
    MY_LOGD("[Destructor]");
}

//************************************************************************
//
//************************************************************************
MVOID
SlantNode::
cleanUp()
{
    MY_LOGD("+");
    MY_LOGD("-");
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
onThreadStart()
{
    CAM_ULOGM_TAGLIFE("SlantNode::onThreadStart");
    VSDOF_INIT_LOG("+");
    //
    queryImgSizeInfo();
    //
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MVOID
SlantNode::
queryImgSizeInfo()
{
    StereoSizeProvider * pSizePrvder = StereoSizeProvider::getInstance();
    mNOC_SIZE              = pSizePrvder->getBufferSize(E_NOC  , mpPipeOption->mStereoScenario);
    mNOC_NONSLANT_SIZE     = pSizePrvder->getBufferSize(E_NOC  , mpPipeOption->mStereoScenario, false);
    mCFM_SIZE              = pSizePrvder->getBufferSize(E_CFM_M, mpPipeOption->mStereoScenario);
    mCFM_NONSLANT_SIZE     = pSizePrvder->getBufferSize(E_CFM_M, mpPipeOption->mStereoScenario, false);
    mDLDEPTH_SIZE          = pSizePrvder->getBufferSize(E_VAIDEPTH_DEPTHMAP, mpPipeOption->mStereoScenario);
    mDLDEPTH_NONSLANT_SIZE = pSizePrvder->getBufferSize(E_VAIDEPTH_DEPTHMAP, mpPipeOption->mStereoScenario, false);
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
onThreadStop()
{
    CAM_ULOGM_TAGLIFE("SlantNode::onThreadStop");
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
onData(DataID data, DepthMapRequestPtr &request)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : reqID=%d", request->getRequestNo());
    CAM_ULOGM_TAGLIFE("SlantNode::onData");

    switch(data)
    {
        case DVS_TO_SLANT_NOC_CFM:
        case DLDEPTH_TO_SLANT_DEPTH:
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

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
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
    CAM_ULOGM_TAGLIFE("SlantNode::onThreadLoop");
    //
    VSDOF_LOGD("reqID=%d threadLoop", pRequest->getRequestNo());
    MBOOL bRet = MTRUE;
    this->incExtThreadDependency();
    pRequest->mTimer.startSlant();
    bRet = runSwRotate(pRequest);
    pRequest->mTimer.stopSlant();
lbExit:
    if(!handleSlantWarpingDone(pRequest))
    {
        MY_LOGE("handleSlantWarpingDone failed!");
        bRet = MFALSE;
    }
    // timer
    return bRet;
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
runSwRotate(DepthMapRequestPtr pRequest)
{
    MBOOL bRet = MTRUE;
    if ( mpFlowOption->checkConnected(DVS_TO_SLANT_NOC_CFM) ) {
        bRet &= rotateNOC(pRequest);
        bRet &= rotateCFM(pRequest);
    }
    else if ( mpFlowOption->checkConnected(DLDEPTH_TO_SLANT_DEPTH) ) {
        bRet &= rotateDepth(pRequest);
        bRet &= rotateCFM(pRequest);
        if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_NOC)
            bRet &= rotateNOC(pRequest);
    }
    else {
        MY_LOGE("undefined flow in slant node");
        bRet = MFALSE;
    }
    return bRet;
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
rotateNOC(DepthMapRequestPtr pRequest)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    MBOOL bRet = MTRUE;
    IImageBuffer *pImgBuf_NOC = nullptr;
    IImageBuffer *pImgBuf_NOC_NonSlant = nullptr;
    // input
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(),
                            BID_OCC_OUT_NOC_M,
                            pImgBuf_NOC);
    if ( !bRet ) {
        MY_LOGE("failed to get buffer BID_OCC_OUT_NOC_M");
        return MFALSE;
    }
    pImgBuf_NOC->syncCache(eCACHECTRL_INVALID);
    // output
    pImgBuf_NOC_NonSlant = pBufferHandler->requestBuffer(getNodeId(),
                        BID_OCC_OUT_NOC_M_NONSLANT);
    pRequest->mTimer.startSlantAlgoNOC();
    VSDOF_LOGD2("inImg:%p inWidth:%d, inHeight:%d, inStride:%d, "
            "outWidth:%d, outHeight:%d, outStride:%d, "
            "rot:%d, outBoundval:%d, outImg:%p,",
            reinterpret_cast<MUINT8*>(pImgBuf_NOC->getBufVA(0)),
            mNOC_SIZE.contentSize().w,
            mNOC_SIZE.contentSize().h,
            mNOC_SIZE.size.w,
            mNOC_NONSLANT_SIZE.contentSize().w,
            mNOC_NONSLANT_SIZE.contentSize().h,
            mNOC_NONSLANT_SIZE.size.w,
            360 - StereoSettingProvider::getModuleRotation(),
            NOC_DEFAULT_VALUE,
            reinterpret_cast<MUINT8*>(pImgBuf_NOC_NonSlant->getBufVA(0)));
    NNRotate(reinterpret_cast<MUINT8*>(pImgBuf_NOC->getBufVA(0)),
            mNOC_SIZE.contentSize().w,
            mNOC_SIZE.contentSize().h,
            mNOC_SIZE.size.w,
            mNOC_NONSLANT_SIZE.contentSize().w,
            mNOC_NONSLANT_SIZE.contentSize().h,
            mNOC_NONSLANT_SIZE.size.w,
            360 - StereoSettingProvider::getModuleRotation(),
            NOC_DEFAULT_VALUE,
            reinterpret_cast<MUINT8*>(pImgBuf_NOC_NonSlant->getBufVA(0))
            );
    pRequest->mTimer.stopSlantAlgoNOC();
    return bRet;
}

MBOOL
SlantNode::
rotateCFM(DepthMapRequestPtr pRequest)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    MBOOL bRet = MTRUE;
    IImageBuffer *pImgBuf_CFM = nullptr;
    IImageBuffer *pImgBuf_CFM_NonSlant = nullptr;
    // input
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(),
                            BID_OCC_OUT_CFM_M, pImgBuf_CFM);
    if ( !bRet ) {
        MY_LOGE("failed to get buffer BID_OCC_OUT_CFM_M");
        return MFALSE;
    }
    pImgBuf_CFM->syncCache(eCACHECTRL_INVALID);
    // ouptut
    pImgBuf_CFM_NonSlant = pBufferHandler->requestBuffer(getNodeId(),
                        BID_OCC_OUT_CFM_M_NONSLANT);
    pRequest->mTimer.startSlantAlgoCFM();
    VSDOF_LOGD2("inImg:%p inWidth:%d, inHeight:%d, inStride:%d, "
            "outWidth:%d, outHeight:%d, outStride:%d, "
            "rot:%d, outBoundval:%d, outImg:%p,",
            reinterpret_cast<MUINT8*>(pImgBuf_CFM->getBufVA(0)),
            mCFM_SIZE.contentSize().w,
            mCFM_SIZE.contentSize().h,
            mCFM_SIZE.size.w,
            mCFM_NONSLANT_SIZE.contentSize().w,
            mCFM_NONSLANT_SIZE.contentSize().h,
            mCFM_NONSLANT_SIZE.size.w,
            360 - StereoSettingProvider::getModuleRotation(),
            CFM_DEFAULT_VALUE,
            reinterpret_cast<MUINT8*>(pImgBuf_CFM_NonSlant->getBufVA(0)));
    NNRotate(reinterpret_cast<MUINT8*>(pImgBuf_CFM->getBufVA(0)),
            mCFM_SIZE.contentSize().w,
            mCFM_SIZE.contentSize().h,
            mCFM_SIZE.size.w,
            mCFM_NONSLANT_SIZE.contentSize().w,
            mCFM_NONSLANT_SIZE.contentSize().h,
            mCFM_NONSLANT_SIZE.size.w,
            360 - StereoSettingProvider::getModuleRotation(),
            CFM_DEFAULT_VALUE,
            reinterpret_cast<MUINT8*>(pImgBuf_CFM_NonSlant->getBufVA(0))
            );
    pRequest->mTimer.stopSlantAlgoCFM();
    return bRet;
}

MBOOL
SlantNode::
rotateDepth(DepthMapRequestPtr pRequest)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    MBOOL bRet = MTRUE;
    IImageBuffer *pImgBuf_DEPTH = nullptr;
    IImageBuffer *pImgBuf_DEPTH_NonSlant = nullptr;
    // input
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(),
                                BID_DLDEPTH_INTERNAL_DEPTHMAP,
                                pImgBuf_DEPTH);
    if ( !bRet ) {
        MY_LOGE("failed to get buffer BID_DLDEPTH_INTERNAL_DEPTHMAP");
        return MFALSE;
    }
    pImgBuf_DEPTH->syncCache(eCACHECTRL_INVALID);
    // output
    pImgBuf_DEPTH_NonSlant = pBufferHandler->requestBuffer(getNodeId(),
                        BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT);
    pRequest->mTimer.startSlantAlgoDepth();
    VSDOF_LOGD2("inImg:%p inWidth:%d, inHeight:%d, inStride:%d, "
            "outWidth:%d, outHeight:%d, outStride:%d, "
            "rot:%d, outBoundval:%d, outImg:%p,",
            reinterpret_cast<MUINT8*>(pImgBuf_DEPTH->getBufVA(0)),
            mDLDEPTH_SIZE.contentSize().w,
            mDLDEPTH_SIZE.contentSize().h,
            mDLDEPTH_SIZE.size.w,
            mDLDEPTH_NONSLANT_SIZE.contentSize().w,
            mDLDEPTH_NONSLANT_SIZE.contentSize().h,
            mDLDEPTH_NONSLANT_SIZE.size.w,
            360 - StereoSettingProvider::getModuleRotation(),
            DEPTH_DEFAULT_VALUE,
            reinterpret_cast<MUINT8*>(pImgBuf_DEPTH_NonSlant->getBufVA(0)));
    NNRotate(reinterpret_cast<MUINT8*>(pImgBuf_DEPTH->getBufVA(0)),
            mDLDEPTH_SIZE.contentSize().w,
            mDLDEPTH_SIZE.contentSize().h,
            mDLDEPTH_SIZE.size.w,
            mDLDEPTH_NONSLANT_SIZE.contentSize().w,
            mDLDEPTH_NONSLANT_SIZE.contentSize().h,
            mDLDEPTH_NONSLANT_SIZE.size.w,
            360 - StereoSettingProvider::getModuleRotation(),
            DEPTH_DEFAULT_VALUE,
            reinterpret_cast<MUINT8*>(pImgBuf_DEPTH_NonSlant->getBufVA(0))
            );
    pRequest->mTimer.stopSlantAlgoDepth();
    return bRet;
}

//************************************************************************
//
//************************************************************************
MBOOL
SlantNode::
handleSlantWarpingDone(DepthMapRequestPtr pRequest)
{
    CAM_ULOGM_TAGLIFE("SlantNode::handleDPEEnqueDone");
    VSDOF_LOGD("reqID=%d +", pRequest->getRequestNo());
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    MBOOL bRet = MTRUE;

    if ( mpFlowOption->checkConnected(SLANT_TO_DVP_NOC_CFM) )
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_NOC_M_NONSLANT, eDPETHMAP_PIPE_NODEID_DVP);
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M_NONSLANT, eDPETHMAP_PIPE_NODEID_DVP);
        this->handleDataAndDump(SLANT_TO_DVP_NOC_CFM, pRequest);
    }
    else if ( mpFlowOption->checkConnected(SLANT_TO_GF_DEPTH_CFM) )
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT, eDPETHMAP_PIPE_NODEID_GF);
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M_NONSLANT, eDPETHMAP_PIPE_NODEID_GF);
        pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_MY_S, eDPETHMAP_PIPE_NODEID_GF);
        this->handleDataAndDump(SLANT_TO_GF_DEPTH_CFM, pRequest);
    }
    else if ( mpFlowOption->checkConnected(SLANT_OUT_DEPTHMAP) )
    {
        bRet &= onHandle3rdPartyFlowDone(pRequest);
    }
    else
    {
        MY_LOGE("undefined handle Slant done");
        bRet = MFALSE;
    }
    // launch onProcessDone
    pBufferHandler->onProcessDone(getNodeId());
    // mark on-going-request end
    this->decExtThreadDependency();
    // stop timer
    pRequest->mTimer.stopSlant();
    if(mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH)
        pRequest->mTimer.showTotalSummary(pRequest->getRequestNo(), pRequest->getRequestAttr(), mpPipeOption);
    VSDOF_LOGD("reqID=%d -", pRequest->getRequestNo());
    return bRet;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
SlantNode::
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
