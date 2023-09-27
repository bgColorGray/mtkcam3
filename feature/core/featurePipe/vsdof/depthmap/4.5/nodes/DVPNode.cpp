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
#include "DVPNode.h"
#include "../DepthMapPipe_Common.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"
// Logging header
#define PIPE_CLASS_TAG "DVPNode"
#include <PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_DPE);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {
#define DPE_USER_NAME "DEPTHPIP_DVP"

DVPNode::
DVPNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
{
    this->addWaitQueue(&mJobQueue);
    this->addWaitQueue(&mJobQueue_MYS);
}
/*******************************************************************************
 *
 ********************************************************************************/
DVPNode::
~DVPNode()
{
    MY_LOGD("[Destructor]");
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVPNode::
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
DVPNode::
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
DVPNode::
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
DVPNode::
onThreadStart()
{
    VSDOF_INIT_LOG("+");
    CAM_ULOGM_TAGLIFE("DVPNode::onThreadStart");
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
DVPNode::
onThreadStop()
{
    VSDOF_LOGD("+");
    CAM_ULOGM_TAGLIFE("DVPNode::onThreadStop");

    cleanUp();
    VSDOF_LOGD("-");
    return MTRUE;
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVPNode::
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
DVPNode::
onData(DataID data, DepthMapRequestPtr& pRequest)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : reqId=%d id=%s", pRequest->getRequestNo(), ID2Name(data));

    switch(data)
    {
        case DVS_TO_DVP_NOC_CFM:
        case SLANT_TO_DVP_NOC_CFM:
            mJobQueue.enque(pRequest);
            break;
        case P2A_TO_DVP_MY_S:
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
/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVPNode::
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
    // timer
    pRequest->mTimer.startDVP();
    // mark on-going-request start
    this->incExtThreadDependency();
    MUINT32 iReqIdx = pRequest->getRequestNo();

    MY_LOGD("threadLoop start, reqID=%d", iReqIdx);
    CAM_ULOGM_TAGLIFE("DVPNode::onThreadLoop");

    // DPEParamV4L2 enqueParams;
    DPEParamV4L2 enqueParams;
    enqueParams.mpfnCallback = onDPEEnqueDone;

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
    // enque cookie instance
    EnqueCookieContainer *pCookieIns = new EnqueCookieContainer(pRequest, this);
    enqueParams.mpCookie = (void*) pCookieIns;
    // timer
    pRequest->mTimer.startDVPDrv();
    CAM_ULOGM_TAG_BEGIN("DPENODE::DPEStream::enque");
    bRet = mpDPEV4L2Stream->EGNenque(enqueParams);
    MY_LOGD("DVE Enque, reqID=%d", iReqIdx);
    CAM_ULOGM_TAG_END();
    if(!bRet)
    {
        MY_LOGE("DPE enque failed!!");
        goto lbExit;
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
DVPNode::
onDPEEnqueDone(DPEParamV4L2& rParams)
{
    EnqueCookieContainer* pEnqueCookie = reinterpret_cast<EnqueCookieContainer*>(rParams.mpCookie);
    DVPNode* pDPENode = reinterpret_cast<DVPNode*>(pEnqueCookie->mpNode);
    pDPENode->handleDPEEnqueDone(rParams, pEnqueCookie);
}
/*******************************************************************************
 *
 ********************************************************************************/
MVOID
DVPNode::
handleDPEEnqueDone(
    DPEParamV4L2& rParams,
    EnqueCookieContainer* pEnqueCookie
)
{
    CAM_ULOGM_TAGLIFE("DVPNode::handleDPEEnqueDone");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    MY_LOGD("reqID=%d +", pRequest->getRequestNo());
    // stop timer
    pRequest->mTimer.stopDVPDrv();
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer* pOutImgBuf = nullptr;
    MBOOL bRet = MFALSE;
    DepthMapBufferID outBID = pRequest->mbWMF_ON? BID_WMF_OUT_DMW : BID_ASF_OUT_HF;
    // check flush status
    if(mpNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH))
    {
        goto lbExit;
    }
    VSDOF_PRFTIME_LOG("+, reqID=%d, DPE exec-time=%d msec",
            pRequest->getRequestNo(), pRequest->mTimer.getElapsedDVPDrv());
    // get the output buffers and invalid
    bRet = pBufferHandler->getEnqueBuffer(getNodeId(), outBID, pOutImgBuf);
    if(!bRet)
    {
        MY_LOGE("Failed to get DPE output buffers");
        goto lbExit;
    }
    pOutImgBuf->syncCache(eCACHECTRL_INVALID);
    // invalidate
    if ( mpFlowOption->checkConnected(DVP_TO_GF_DMW_N_DEPTH) )
    {
        pBufferHandler->configOutBuffer(getNodeId(), outBID, eDPETHMAP_PIPE_NODEID_GF);
        pBufferHandler->configOutBuffer(getNodeId(),
                StereoSettingProvider::isSlantCameraModule()?
                    BID_OCC_OUT_CFM_M_NONSLANT : BID_OCC_OUT_CFM_M,
                eDPETHMAP_PIPE_NODEID_GF);
        pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_MY_S, eDPETHMAP_PIPE_NODEID_GF);
        // deliver to GF if debugInk need only
        if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_NOC)
            pBufferHandler->configOutBuffer(getNodeId(),
                                StereoSettingProvider::isSlantCameraModule()?
                                    BID_OCC_OUT_NOC_M_NONSLANT : BID_OCC_OUT_NOC_M,
                                eDPETHMAP_PIPE_NODEID_GF);
        handleDataAndDump(DVP_TO_GF_DMW_N_DEPTH, pRequest);
    }
    else if ( mpFlowOption->checkConnected(DVP_OUT_DEPTHMAP) )
    {
        bRet = onHandle3rdPartyFlowDone(pRequest);
    }
    else
    {
        MY_LOGE("undefined handle DVP done");
        bRet = MFALSE;
    }

lbExit:
    // launch onProcessDone
    pBufferHandler->onProcessDone(getNodeId());
    delete pEnqueCookie;
    // mark on-going-request end
    this->decExtThreadDependency();
    // stop timer
    pRequest->mTimer.stopDVP();
    if(mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_HW_HOLEFILLED_DEPTH)
        pRequest->mTimer.showTotalSummary(pRequest->getRequestNo(), pRequest->getRequestAttr(), mpPipeOption);
    VSDOF_LOGD("reqID=%d -", pRequest->getRequestNo());
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
DVPNode::
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
    rEnqParam.mDpeConfig.Dpe_engineSelect = MODE_DVP_ONLY;
    // not 16bit mode for vsdof
    rEnqParam.mDpeConfig.Dpe_is16BitMode = 0;
    // check use WMF submodule or not
    pRequest->mbWMF_ON = rEnqParam.mDpeConfig.Dpe_DVPSettings.SubModule_EN.wmf_hf_en
                    || rEnqParam.mDpeConfig.Dpe_DVPSettings.SubModule_EN.wmf_filt_en;
    // insert the dynamic data
    IImageBuffer *pImgBuf_NOC_M  = nullptr;
    IImageBuffer *pImgBuf_MY_S   = nullptr;
    IImageBuffer *pImgBuf_CRM = nullptr, *pImgBuf_RD = nullptr;
    IImageBuffer *pImgBuf_HF = nullptr;
    IImageBuffer *pImgBuf_WMF = nullptr;
    // DPE Config IImageBuffer*
    DPEConfigBuffer dpeBuffer;
    // input buffer
    MBOOL bRet = MTRUE;
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(),
                                StereoSettingProvider::isSlantCameraModule()?
                                    BID_OCC_OUT_NOC_M_NONSLANT : BID_OCC_OUT_NOC_M,
                                pImgBuf_NOC_M);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_MY_S, pImgBuf_MY_S);

    // output buffer
    pImgBuf_CRM = pBufferHandler->requestBuffer(getNodeId(), BID_ASF_OUT_CRM);
    pImgBuf_RD = pBufferHandler->requestBuffer(getNodeId(), BID_ASF_OUT_RD);
    pImgBuf_HF = pBufferHandler->requestBuffer(getNodeId(), BID_ASF_OUT_HF);
    if (!bRet || (pImgBuf_CRM == nullptr || pImgBuf_RD == nullptr ||
                  pImgBuf_HF == nullptr))
    {
        MY_LOGE("Cannot get buffers!!! "
               "NOC(%p), MY_S(%p), CRM(%p), RD(%p), HF(%p)",
               static_cast<void*>(pImgBuf_NOC_M),
               static_cast<void*>(pImgBuf_MY_S),
               static_cast<void*>(pImgBuf_CRM),
               static_cast<void*>(pImgBuf_RD),
               static_cast<void*>(pImgBuf_HF));
        return MFALSE;
    }
    if(pRequest->mbWMF_ON)
    {
        pImgBuf_WMF = pBufferHandler->requestBuffer(getNodeId(), BID_WMF_OUT_DMW);
        if (pImgBuf_WMF == nullptr)
        {
            MY_LOGE("Cannot get WMF! WMF=(%p)",
                        static_cast<void*>(pImgBuf_WMF));
            return MFALSE;
        }
    }

    pImgBuf_NOC_M->syncCache(eCACHECTRL_FLUSH);
    pImgBuf_MY_S->syncCache(eCACHECTRL_FLUSH);
    // Input
    dpeBuffer.Dpe_InBuf_SrcImg_C  = pImgBuf_MY_S;
    dpeBuffer.Dpe_InBuf_SrcImg_Y  = pImgBuf_MY_S;
    dpeBuffer.Dpe_InBuf_OCC       = pImgBuf_NOC_M;
    // output - DVP Buffers
    dpeBuffer.Dpe_OutBuf_CRM      = pImgBuf_CRM;
    dpeBuffer.Dpe_OutBuf_ASF_RD   = pImgBuf_RD;
    dpeBuffer.Dpe_OutBuf_ASF_HF   = pImgBuf_HF;
    if(pRequest->mbWMF_ON)
        dpeBuffer.Dpe_OutBuf_WMF_FILT = pImgBuf_WMF;
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
DVPNode::
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
DVPNode::
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
