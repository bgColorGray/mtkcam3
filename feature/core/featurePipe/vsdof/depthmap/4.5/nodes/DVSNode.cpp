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
#include <camera_custom_stereo.h>
// mtkcam global header file
// Module header file
#include <stereo_tuning_provider.h>
// Local header file
#include "DVSNode.h"
#include "../DepthMapPipe_Common.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"
// Logging header
#define PIPE_CLASS_TAG "DVSNode"
#include <PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_DPE);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {
#define DPE_USER_NAME "DEPTHPIP_DVS"

DVSNode::
DVSNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
{
    this->addWaitQueue(&mJobQueue);
}
/*******************************************************************************
 *
 ********************************************************************************/
DVSNode::
~DVSNode()
{
    MY_LOGD("[Destructor]");
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVSNode::
cleanUp()
{
    VSDOF_LOGD("+");
    //
    if(mpDPEV4L2Stream != nullptr)
    {
        mpDPEV4L2Stream->uninit();
        mpDPEV4L2Stream->destroyInstance();
        mpDPEV4L2Stream = nullptr;
    }
    mJobQueue.clear();
    //
    VSDOF_LOGD("-");
}
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
onThreadStart()
{
    VSDOF_INIT_LOG("+");
    CAM_ULOGM_TAGLIFE("DVSNode::onThreadStart");
    // init DPEStream
    mpDPEV4L2Stream = DPEV4L2Stream::createInstance();
    if(mpDPEV4L2Stream == NULL || !mpDPEV4L2Stream->init())
    {
        MY_LOGE("DPE Stream create instance failed!");
        return MFALSE;
    }
    VSDOF_INIT_LOG("-");
    return MTRUE;
}
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
onThreadStop()
{
    VSDOF_LOGD("+");
    CAM_ULOGM_TAGLIFE("DVSNode::onThreadStop");

    cleanUp();
    VSDOF_LOGD("-");
    return MTRUE;
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVSNode::
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
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
onData(DataID data, DepthMapRequestPtr& pRequest)
{
    MBOOL ret = MTRUE;
    MY_LOGD("+ : reqId=%d id=%s", pRequest->getRequestNo(), ID2Name(data));

    switch(data)
    {
        case N3D_TO_DVS_MVSV_MASK:
        case WPE_TO_DVS_WARP_IMG:
            mJobQueue.enque(pRequest);
            break;
        default:
            MY_LOGW("Unrecongnized DataID=%d", data);
            ret = MFALSE;
            break;
    }

    VSDOF_LOGD("-");
    return ret;
}
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
onThreadLoop()
{
    DepthMapRequestPtr pRequest;

    MY_LOGD("onThreadLoop");
    if( !waitAnyQueue() )
    {
        return MFALSE;
    }
    if( !mJobQueue.deque(pRequest) )
    {
        MY_LOGE("mJobQueue.deque() failed");
        return MFALSE;
    }
    MY_LOGD("deque from waitQueue");
    if (pRequest == nullptr) {
        MY_LOGE("null pRequest");
    }
    // timer
    pRequest->mTimer.startDVS();
    // mark on-going-request start
    this->incExtThreadDependency();
    MUINT32 iReqIdx = pRequest->getRequestNo();

    MY_LOGD("threadLoop start, reqID=%d", iReqIdx);
    CAM_ULOGM_TAGLIFE("DVSNode::onThreadLoop");

    // DPEParamV4L2 enqueParams;
    DPEParamV4L2 enqueParams;
    enqueParams.mpfnCallback = onDPEEnqueDone;

    // check if the P2Cookie is valid or not.
    if (__builtin_expect( !isValidCookie(enqueParams), false )) {
        MY_LOGE("%s: DPEParamV4L2 is invalid", __FUNCTION__);
        checkSumDump(enqueParams);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        return MTRUE;
    }

    MBOOL bRet = MFALSE;
    if(pRequest->getRequestAttr().opState == eSTATE_NORMAL ||
        pRequest->getRequestAttr().opState == eSTATE_RECORD)
    {
        bRet = prepareDPEEnqueConfig_PVVR(pRequest, enqueParams);
    }
    if(!bRet)
    {
        MY_LOGE("Failed to prepare DPE enque paramters! isCap: %d",
                pRequest->getRequestAttr().opState == eSTATE_CAPTURE);
        return MFALSE;
    }

    // check if the P2Cookie is valid or not.
    if (__builtin_expect( !isValidCookie(enqueParams), false )) {
        MY_LOGE("%s: prepareDPEEnqueConfig, DPEParamV4L2 is invalid", __FUNCTION__);
        checkSumDump(enqueParams);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        return MTRUE;
    }
    // enque cookie instance
    EnqueCookieContainer *pCookieIns = new EnqueCookieContainer(pRequest, this);
    enqueParams.mpCookie = (void*) pCookieIns;
    // timer
    pRequest->mTimer.startDVSDrv();
    CAM_ULOGM_TAG_BEGIN("DVSNODE::DPEStream::enque");
    bRet = mpDPEV4L2Stream->EGNenque(enqueParams);
    MY_LOGD("DVE Enque, reqID=%d", iReqIdx);
    CAM_ULOGM_TAG_END();
    if(!bRet)
    {
        MY_LOGE("DPE enque failed!!");
        goto lbExit;
    }

    // check if the P2Cookie is valid or not.
    if (__builtin_expect( !isValidCookie(enqueParams), false )) {
        MY_LOGE("%s: EGNenque, DPEParamV4L2 is invalid", __FUNCTION__);
        checkSumDump(enqueParams);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        return MTRUE;
    }
    return MTRUE;

lbExit:
    delete pCookieIns;
    // mark on-going-request end
    this->decExtThreadDependency();
    return MFALSE;
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVSNode::
onDPEEnqueDone(DPEParamV4L2& rParams)
{
    EnqueCookieContainer* pEnqueCookie = reinterpret_cast<EnqueCookieContainer*>(rParams.mpCookie);
    DVSNode* pDVSNode = reinterpret_cast<DVSNode*>(pEnqueCookie->mpNode);
    pDVSNode->handleDPEEnqueDone(rParams, pEnqueCookie);
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVSNode::
handleDPEEnqueDone(
    DPEParamV4L2& rParams,
    EnqueCookieContainer* pEnqueCookie
)
{
    CAM_ULOGM_TAGLIFE("DVSNode::handleDPEEnqueDone");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    MY_LOGD("reqID=%d +", pRequest->getRequestNo());
    // check if the P2Cookie is valid or not.
    if (__builtin_expect( !isValidCookie(rParams), false )) {
        MY_LOGE("%s: DPEParamV4L2 is invalid", __FUNCTION__);
        checkSumDump(rParams);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        return;
    }
    // stop timer
    pRequest->mTimer.stopDVSDrv();
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer* pOutImgBuf = nullptr;
    MBOOL bRet = MFALSE;
    // check flush status
    if(mpNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH))
    {
        goto lbExit;
    }
    VSDOF_PRFTIME_LOG("+, reqID=%d, DPE exec-time=%d msec",
            pRequest->getRequestNo(), pRequest->mTimer.getElapsedDVSDrv());

    // get the output buffers and invalid
    bRet = pBufferHandler->getEnqueBuffer(getNodeId(), BID_OCC_OUT_NOC_M, pOutImgBuf);
    if(!bRet)
    {
        MY_LOGE("Failed to get DPE output buffers");
        goto lbExit;
    }
    // invalidate
    pOutImgBuf->syncCache(eCACHECTRL_INVALID);
    // config to next stage
    if(mpFlowOption->checkConnected(DVS_TO_SLANT_NOC_CFM))
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_NOC_M, eDPETHMAP_PIPE_NODEID_SLANT);
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M, eDPETHMAP_PIPE_NODEID_SLANT);
        handleDataAndDump(DVS_TO_SLANT_NOC_CFM, pRequest);
    }
    else if( mpFlowOption->checkConnected(DVS_TO_DVP_NOC_CFM) )
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_NOC_M, eDPETHMAP_PIPE_NODEID_DVP);
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M, eDPETHMAP_PIPE_NODEID_DVP);
        handleDataAndDump(DVS_TO_DVP_NOC_CFM, pRequest);
    }
    else if( mpFlowOption->checkConnected(DVS_TO_DLDEPTH_MVSV_NOC) )
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_NOC_M, eDPETHMAP_PIPE_NODEID_DLDEPTH);
        pBufferHandler->configOutBuffer(getNodeId(), BID_OCC_OUT_CFM_M, eDPETHMAP_PIPE_NODEID_DLDEPTH);
        pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_MV_Y, eDPETHMAP_PIPE_NODEID_DLDEPTH);
        pBufferHandler->configOutBuffer(getNodeId(), BID_WPE_OUT_SV_Y, eDPETHMAP_PIPE_NODEID_DLDEPTH);
        handleDataAndDump(DVS_TO_DLDEPTH_MVSV_NOC, pRequest);
    }
    else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH )
    {
        bRet &= onHandle3rdPartyFlowDone(pRequest);
    }
    else
    {
        MY_LOGE("undefined handle DVS done");
        bRet = MFALSE;
    }

lbExit:
    // launch onProcessDone
    pBufferHandler->onProcessDone(getNodeId());
    delete pEnqueCookie;
    // mark on-going-request end
    this->decExtThreadDependency();
    // stop timer
    pRequest->mTimer.stopDVS();
    if(mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH)
        pRequest->mTimer.showTotalSummary(pRequest->getRequestNo(), pRequest->getRequestAttr(), mpPipeOption);
    VSDOF_LOGD("reqID=%d -", pRequest->getRequestNo());
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVSNode::
prepareDPEEnqueConfig_PVVR(
    DepthMapRequestPtr pRequest,
    DPEParamV4L2& rEnqParam
)
{
    VSDOF_LOGD("+");

    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // query tuning provider
    StereoTuningProvider::getDPETuningInfo(&(rEnqParam.mDpeConfig), mpPipeOption->mStereoScenario);
    // insert DPE engine mode
    rEnqParam.mDpeConfig.Dpe_engineSelect = MODE_DVS_ONLY;
    // not 16bit mode for vsdof
    rEnqParam.mDpeConfig.Dpe_is16BitMode = 0;
    // insert the dynamic data
    IImageBuffer *pImgBuf_MV_Y   = nullptr, *pImgBuf_SV_Y   = nullptr;
    IImageBuffer *pImgBuf_MASK_M = nullptr, *pImgBuf_MASK_S = nullptr;
    // output buffer
    IImageBuffer *pImgBuf_NOC = nullptr, *pImgBuf_CFM = nullptr;
    // DPE Config IImageBuffer*
    DPEConfigBuffer dpeBuffer;
    // input buffer
    MBOOL bRet = MTRUE;
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_N3D_OUT_MV_Y, pImgBuf_MV_Y);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_WPE_OUT_SV_Y, pImgBuf_SV_Y);
    pImgBuf_MASK_M = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_MASK_M_STATIC);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_WPE_OUT_MASK_S, pImgBuf_MASK_S);
    // output buffer
    pImgBuf_NOC = pBufferHandler->requestBuffer(getNodeId(), BID_OCC_OUT_NOC_M);
    pImgBuf_CFM = pBufferHandler->requestBuffer(getNodeId(), BID_OCC_OUT_CFM_M);
    if(!bRet || (pImgBuf_NOC == nullptr || pImgBuf_CFM == nullptr))
    {
        MY_LOGE("Cannot get buffers!!! "
               "MV_Y(%p), SV_Y(%p), MASK_M(%p), MASK_S(%p), NOC(%p), CFM(%p)",
               static_cast<void*>(pImgBuf_MV_Y),
               static_cast<void*>(pImgBuf_SV_Y),
               static_cast<void*>(pImgBuf_MASK_M),
               static_cast<void*>(pImgBuf_MASK_S),
               static_cast<void*>(pImgBuf_NOC),
               static_cast<void*>(pImgBuf_CFM));
        return MFALSE;
    }
    pImgBuf_SV_Y->syncCache(eCACHECTRL_FLUSH);
    pImgBuf_MV_Y->syncCache(eCACHECTRL_FLUSH);
    pImgBuf_MASK_M->syncCache(eCACHECTRL_FLUSH);
    pImgBuf_MASK_S->syncCache(eCACHECTRL_FLUSH);
    // check main eye loc
    if(STEREO_SENSOR_REAR_MAIN_TOP == StereoSettingProvider::getSensorRelativePosition())
    {
        // Main1/2 is L/R
        dpeBuffer.Dpe_InBuf_SrcImg_Y_L = pImgBuf_MV_Y;
        dpeBuffer.Dpe_InBuf_SrcImg_Y_R = pImgBuf_SV_Y;
        dpeBuffer.Dpe_InBuf_ValidMap_L = pImgBuf_MASK_M;
        dpeBuffer.Dpe_InBuf_ValidMap_R = pImgBuf_MASK_S;
    }
    else
    {
        // Main1/2 is R/L
        dpeBuffer.Dpe_InBuf_SrcImg_Y_L = pImgBuf_SV_Y;
        dpeBuffer.Dpe_InBuf_SrcImg_Y_R = pImgBuf_MV_Y;
        dpeBuffer.Dpe_InBuf_ValidMap_L = pImgBuf_MASK_S;
        dpeBuffer.Dpe_InBuf_ValidMap_R = pImgBuf_MASK_M;
    }
    // output - DVS Buffers
    dpeBuffer.Dpe_OutBuf_CONF = pImgBuf_CFM;
    dpeBuffer.Dpe_OutBuf_OCC  = pImgBuf_NOC;
    dpeBuffer.Dpe_InBuf_OCC   = dpeBuffer.Dpe_OutBuf_OCC;
    // debug
    debugDPEConfig(dpeBuffer);
    // set DPE config bufPA
    fillDpeBufferPA(rEnqParam.mDpeConfig, dpeBuffer);

    VSDOF_LOGD("-");
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVSNode::
fillDpeBufferPA(
    DPE_Config &config,
    DPEConfigBuffer &buffer
)
{
    #define FILL_BUFFER_PA(bufname) \
        if(buffer.bufname != nullptr) \
            config.bufname = (MUINT32)buffer.bufname->getBufPA(0);

    FILL_BUFFER_PA(Dpe_InBuf_SrcImg_Y_L);
    FILL_BUFFER_PA(Dpe_InBuf_SrcImg_Y_R);
    FILL_BUFFER_PA(Dpe_InBuf_SrcImg_C);
    FILL_BUFFER_PA(Dpe_InBuf_SrcImg_Y);
    FILL_BUFFER_PA(Dpe_InBuf_ValidMap_L);
    FILL_BUFFER_PA(Dpe_InBuf_ValidMap_R);
    FILL_BUFFER_PA(Dpe_OutBuf_CONF);
    FILL_BUFFER_PA(Dpe_OutBuf_OCC);
    FILL_BUFFER_PA(Dpe_OutBuf_OCC_Ext);
    FILL_BUFFER_PA(Dpe_InBuf_OCC);
    FILL_BUFFER_PA(Dpe_InBuf_OCC_Ext);
    FILL_BUFFER_PA(Dpe_OutBuf_CRM);
    FILL_BUFFER_PA(Dpe_OutBuf_ASF_RD);
    FILL_BUFFER_PA(Dpe_OutBuf_ASF_RD_Ext);
    FILL_BUFFER_PA(Dpe_OutBuf_ASF_HF);
    FILL_BUFFER_PA(Dpe_OutBuf_ASF_HF_Ext);
    FILL_BUFFER_PA(Dpe_OutBuf_WMF_FILT);

    // reconfig Dpe_InBuf_SrcImg_C, need UV plane
    if(buffer.Dpe_InBuf_SrcImg_C)
        config.Dpe_InBuf_SrcImg_C = (MUINT32)buffer.Dpe_InBuf_SrcImg_C->getBufPA(1);

    #undef FILL_BUFFER_PA
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVSNode::
debugDPEConfig(DPEConfigBuffer& dpeBuffer)
{
    if(DepthPipeLoggingSetup::miDebugLog<2)
        return;

    #define DEBUG_BUFFER_SETUP(buf) \
            MY_LOGD("DPE buf:" # buf);\
            if(buf != nullptr){\
                MY_LOGD("Image buffer size=%dx%d:", buf->getImgSize().w, buf->getImgSize().h);\
                MY_LOGD("Image buffer format=%x planeCount=%lu, phyAddr(0)=%lX", buf->getImgFormat(), buf->getPlaneCount(), buf->getBufPA(0));\
                for(int i=0;i<buf->getPlaneCount();++i)\
                {\
                    MY_LOGD("bufferSize in bytes of plane(%d): %lu, phyAddr(%d):%lX",\
                     i, buf->getBufSizeInBytes(i), i, buf->getBufPA(0));\
                }}\
            else\
                MY_LOGD("null buffer: " # buf);

    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_SrcImg_Y_L);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_SrcImg_Y_R);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_SrcImg_C);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_SrcImg_Y);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_ValidMap_L);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_ValidMap_R);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_CONF);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_OCC);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_OCC_Ext);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_OCC);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_InBuf_OCC_Ext);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_CRM);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_ASF_RD);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_ASF_RD_Ext);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_ASF_HF);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_ASF_HF_Ext);
    DEBUG_BUFFER_SETUP(dpeBuffer.Dpe_OutBuf_WMF_FILT);

    #undef DEBUG_BUFFER_SETUP
}


}; //NSFeaturePipe_DepthMap
}; //NSCamFeature
}; //NSCam
