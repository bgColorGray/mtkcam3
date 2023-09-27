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

/**
 * @file DepthMapFlowOption_VSDOF.cpp
 * @brief DepthMapFlowOption for VSDOF
 */

// Standard C header file

// Android system/core header file

// mtkcam custom header file
#include <camera_custom_stereo.h>
// mtkcam global header file
#include <mtkcam/def/common.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/iopipe/PostProc/IHalPostProcPipe.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <isp_tuning.h>
// Module header file
#include <stereo_tuning_provider.h>
#include <fefm_setting_provider.h>
// Local header file
#include "DepthMapFlowOption_VSDOF.h"
#include "../DepthMapPipe.h"
#include "../DepthMapPipeNode.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"

#define PIPE_MODULE_TAG "DepthMapPipe"
#define PIPE_CLASS_TAG "DepthMapFlowOption_VSDOF"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace NSIspTuning;
typedef NodeBufferSizeMgr::P2ABufferSize P2ABufferSize;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DepthMapFlowOption_VSDOF::
DepthMapFlowOption_VSDOF(
    sp<DepthMapPipeSetting> pSetting,
    sp<DepthMapPipeOption> pOption,
    sp<DepthInfoStorage> pStorage
)
: DepthQTemplateProvider(pSetting, pOption, this)
, mpPipeOption(pOption)
{
    mpDepthStorage = pStorage;
    //
    mpSizeMgr = new NodeBufferSizeMgr(mpPipeOption);
    // flow option config
    const P2ABufferSize& P2ASize = mpSizeMgr->getP2A(eSTEREO_SCENARIO_RECORD);
    mConfig.mbCaptureFDEnable = MTRUE;
    mConfig.mFDSize = P2ASize.mFD_IMG_SIZE;
}

DepthMapFlowOption_VSDOF::
~DepthMapFlowOption_VSDOF()
{
    MY_LOGD("[Destructor] +");
    if(mpSizeMgr != nullptr)
        delete mpSizeMgr;
    MY_LOGD("[Destructor] -");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  P2AFlowOption Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


MBOOL
DepthMapFlowOption_VSDOF::
buildQParam(
    DepthMapRequestPtr pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam,
    EnqueCookieContainer*& rpWpeQParamHolder
)
{
    MBOOL bRet                        = MFALSE;
    DepthInputImgCombination inputImg = mpPipeOption->mInputImg;
    DepthMapPipeOpState opState       = pRequest->getRequestAttr().opState;
    MBOOL bNeedFEFM                   = pRequest->getRequestAttr().needFEFM;
    if(inputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2) // original P2 DL MDP path
    {
        if(opState == eSTATE_NORMAL)
        {
            if(bNeedFEFM)
                bRet = TemplateProvider::buildQParams_NORMAL(pRequest, tuningResult, rOutParam);
            else
                bRet = TemplateProvider::buildQParams_NORMAL_NOFEFM(pRequest, tuningResult, rOutParam);
        }
        else if(opState == eSTATE_STANDALONE)
        {
            bRet = TemplateProvider::buildQParams_STANDALONE(pRequest, tuningResult, rOutParam);
        }
        else
        {
            MY_LOGE("Invalid request");
            bRet = MFALSE;
        }
    }
    else if(inputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        if ( bNeedFEFM && StereoSettingProvider::isSlantCameraModule() )
            bRet = TemplateProvider::buildQParams_NORMAL_SLANT(pRequest, rOutParam, rpWpeQParamHolder);
        else
            MY_LOGE("this scenario should not need QParams");
    }
    else
    {
        MY_LOGE("This input image combination is unknown, %d", inputImg);
        bRet = MFALSE;
    }
    return bRet;
}

MBOOL
DepthMapFlowOption_VSDOF::
onP2ProcessDone(
    P2ANode* pNode,
    sp<DepthMapEffectRequest> pRequest
)
{
    VSDOF_LOGD("+, reqID=%d", pRequest->getRequestNo());
    // notify template provider p2 done
    TemplateProvider::onHandleP2Done(eDPETHMAP_PIPE_NODEID_P2A, pRequest);
    VSDOF_LOGD("-, reqID=%d", pRequest->getRequestNo());
    return MTRUE;
}

INPUT_RAW_TYPE
DepthMapFlowOption_VSDOF::
getInputRawType(
    sp<DepthMapEffectRequest> pReq,
    StereoP2Path path
)
{
    INPUT_RAW_TYPE rawType;
    if(pReq->getRequestAttr().opState == eSTATE_CAPTURE &&
        path == eP2APATH_MAIN1_BAYER)
    {
        rawType = eFULLSIZE_RAW;
    }
    else
    {
        rawType = eRESIZE_RAW;
    }
    return rawType;
}


MBOOL
DepthMapFlowOption_VSDOF::
buildQParam_Bayer(
    DepthMapRequestPtr pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    // only Preview_NOFEFM/Capture can go here.
    MBOOL bRet                        = MFALSE;
    DepthInputImgCombination inputImg = mpPipeOption->mInputImg;
    DepthMapPipeOpState opState       = pRequest->getRequestAttr().opState;
    MBOOL bNeedFEFM                   = pRequest->getRequestAttr().needFEFM;
    if(inputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        if(opState == eSTATE_NORMAL && bNeedFEFM)
            bRet = TemplateProvider::buildQParams_BAYER_NORMAL(pRequest, tuningResult, rOutParam);
        else if(opState == eSTATE_NORMAL && !bNeedFEFM)
            bRet = TemplateProvider::buildQParams_BAYER_NORMAL_NOFEFM(pRequest, tuningResult, rOutParam);
        else if(opState == eSTATE_STANDALONE)
            bRet = TemplateProvider::buildQParams_STANDALONE(pRequest, tuningResult, rOutParam);
        else
            MY_LOGE("Not valid request!!bNeedFEFM :%d", bNeedFEFM);
    }
    else if(inputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        if((opState == eSTATE_NORMAL || opState == eSTATE_RECORD) && bNeedFEFM)
            bRet = TemplateProvider::buildQParams_BAYER_NORMAL_ONLYFEFM(pRequest, tuningResult, rOutParam);
        else
            MY_LOGE("Not valid request!!bNeedFEFM :%d", bNeedFEFM);
    }
    else
    {
        MY_LOGE("This input image combination is unknown, %d", inputImg);
        bRet = MFALSE;
    }
    return bRet;
}

MBOOL
DepthMapFlowOption_VSDOF::
onP2ProcessDone_Bayer(
    P2ABayerNode* pNode,
    sp<DepthMapEffectRequest> pRequest
)
{
    VSDOF_LOGD("+ reqID=%d", pRequest->getRequestNo());
    // notify template provider p2 done
    TemplateProvider::onHandleP2Done(eDPETHMAP_PIPE_NODEID_P2ABAYER, pRequest);
    VSDOF_LOGD("- reqID=%d", pRequest->getRequestNo());

    return MTRUE;
}

MBOOL
DepthMapFlowOption_VSDOF::
buildQParam_Warp(
    DepthMapRequestPtr pRequest,
    QParams& rOutParam,
    EnqueCookieContainer* &WPEQparamHolder
)
{
    MBOOL bRet = MFALSE;
    bRet = TemplateProvider::buildQParams_WARP_NORMAL(pRequest, rOutParam, WPEQparamHolder);
    return bRet;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapFlowOption_VSDOF private function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapFlowOption Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MBOOL
DepthMapFlowOption_VSDOF::
init()
{
    // prepare template
    MBOOL bRet = TemplateProvider::init(mpSizeMgr);
    return bRet;
}

MBOOL
DepthMapFlowOption_VSDOF::
queryReqAttrs(
    sp<DepthMapEffectRequest> pRequest,
    EffectRequestAttrs& rReqAttrs
)
{
    // deccide EIS on/off
    IMetadata* pInAppMeta = nullptr;
    pRequest->getRequestMetadata({.bufferID=BID_META_IN_APP, .ioType=eBUFFER_IOTYPE_INPUT}
                                   , pInAppMeta);

    rReqAttrs.isEISOn = isEISOn(pInAppMeta);
    rReqAttrs.opState = eSTATE_NORMAL;

    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        if(pRequest->isRequestBuffer(BID_P2A_IN_YUV1) &&
            !pRequest->isRequestBuffer(BID_P2A_IN_RSRAW1))
        {
            rReqAttrs.opState = eSTATE_CAPTURE;
            rReqAttrs.bufferScenario = eBUFFER_POOL_SCENARIO_CAPTURE;
        }
        else
        {
            // check standalone request
            if(!pRequest->isRequestBuffer(BID_P2A_IN_RSRAW2))
            {
                rReqAttrs.opState = eSTATE_STANDALONE;
            }
            else
            {
                rReqAttrs.opState = eSTATE_NORMAL;
                if(rReqAttrs.isEISOn)
                    rReqAttrs.bufferScenario = eBUFFER_POOL_SCENARIO_RECORD;
                else
                    rReqAttrs.bufferScenario = eBUFFER_POOL_SCENARIO_PREVIEW;
            }
        }
    }
    else
    {
        // check standalone request
        if(!pRequest->isRequestBuffer(BID_P2A_IN_YUV1)
        || !pRequest->isRequestBuffer(BID_P2A_IN_YUV2)
        || !pRequest->isRequestBuffer(BID_P1_OUT_RECT_IN1)
        || !pRequest->isRequestBuffer(BID_P1_OUT_RECT_IN2))
        {
            rReqAttrs.opState = eSTATE_STANDALONE;
        }
        else
        {
            if(mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW)
            {
                rReqAttrs.opState = eSTATE_NORMAL;
                rReqAttrs.bufferScenario = eBUFFER_POOL_SCENARIO_PREVIEW;
            }
            else
            {
                rReqAttrs.opState = eSTATE_RECORD;
                rReqAttrs.bufferScenario = eBUFFER_POOL_SCENARIO_RECORD;
            }
        }
    }

    if(rReqAttrs.opState != eSTATE_STANDALONE)
    {
        //
        MINT32 magicNum1 = 0, magicNum2 = 0;
        IMetadata* pInHalMeta_Main1 = nullptr, *pInHalMeta_Main2 = nullptr;
        MBOOL bRet = pRequest->getRequestMetadata({.bufferID=BID_META_IN_HAL_MAIN1,
                                        .ioType=eBUFFER_IOTYPE_INPUT}, pInHalMeta_Main1);
        bRet &= pRequest->getRequestMetadata({.bufferID=BID_META_IN_HAL_MAIN2,
                                        .ioType=eBUFFER_IOTYPE_INPUT}, pInHalMeta_Main2);
        if(!bRet)
        {
            MY_LOGE("Cannot get the input metadata!");
            return MFALSE;
        }
        if(!tryGetMetadata<MINT32>(pInHalMeta_Main1, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum1)) {
            MY_LOGE("Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta of Main1!");
        }
        if(!tryGetMetadata<MINT32>(pInHalMeta_Main2, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum2)) {
            MY_LOGE("Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta of Main2!");
        }
    #ifndef GTEST
        rReqAttrs.needFEFM = FEFMSettingProvider::getInstance()->needToRunFEFM(
                                    magicNum1, magicNum2, (rReqAttrs.opState==eSTATE_CAPTURE));
        // Bit True flow, FEFM ON
        // if(pRequest->getRequestNo() == miBitTrueFrame)
        //     rReqAttrs.needFEFM = 1;
    #else
        rReqAttrs.needFEFM = 1;
    #endif
    }
    VSDOF_LOGD("reqID=%d, queryReqAttrs: opState=%d, bufScenario=%d, needFEFM=%d",
                pRequest->getRequestNo(), rReqAttrs.opState, rReqAttrs.bufferScenario, rReqAttrs.needFEFM);
    return MTRUE;
}


MBOOL
DepthMapFlowOption_VSDOF::
queryPipeNodeBitSet(PipeNodeBitSet& nodeBitSet)
{
    nodeBitSet.reset();
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_P2A     , 1);
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_P2ABAYER, 1);
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_N3D     , 1);
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DVS     , 1);

    // check support WPE hareware
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_WPE     , hasWPEHw());

    // check mode
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DVP     , 0);
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_GF      , 0);
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DLDEPTH , 0);
    nodeBitSet.set(eDPETHMAP_PIPE_NODEID_SLANT   , 0);

    if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_VSDOF ) {
        nodeBitSet.set(eDPETHMAP_PIPE_NODEID_GF, 1);
        if ( mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW )
            nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DVP, 1);
        else if ( mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_RECORD )
            nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DLDEPTH, 1);
    }
    else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_SW_OPTIMIZED_DEPTH ) {
        nodeBitSet.set(eDPETHMAP_PIPE_NODEID_GF, 1);
        if ( mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW )
            nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DVP, 1);
        else if ( mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_RECORD )
            nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DLDEPTH, 1);
    }
    else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_HW_HOLEFILLED_DEPTH ) {
        nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DVP, 1);
    }
    else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_AI_DEPTH ) {
#ifdef SUPPORT_VIDEO_AIDEPTH
        nodeBitSet.set(eDPETHMAP_PIPE_NODEID_DLDEPTH, 1);
#else
        MY_LOGE("This platform not support AIDEPTH, check custom setting");
#endif
    }
    else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH ) {
    }
    else {
        MY_LOGE("invalid FeatureMode");
    }

    if ( StereoSettingProvider::isSlantCameraModule() )
        nodeBitSet.set(eDPETHMAP_PIPE_NODEID_SLANT, 1);

    return MTRUE;
}

MBOOL
DepthMapFlowOption_VSDOF::
buildPipeGraph(DepthMapPipe* pPipe, const DepthMapPipeNodeMap& nodeMap)
{
    DepthMapPipeNode* pP2ANode      = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_P2A);
    DepthMapPipeNode* pN3DNode      = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_N3D);
    DepthMapPipeNode* pDVSNode      = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_DVS);
    DepthMapPipeNode* pP2ABayerNode = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_P2ABAYER);
    DepthMapPipeNode* pWPENode      = nullptr;
    DepthMapPipeNode* pDVPNode      = nullptr;
    DepthMapPipeNode* pDLDepthNode  = nullptr;
    DepthMapPipeNode* pGFNode       = nullptr;
    DepthMapPipeNode* pSlantNode    = nullptr;

    MBOOL supportWPE     = ( nodeMap.indexOfKey(eDPETHMAP_PIPE_NODEID_WPE) >= 0);
    MBOOL supportDVP     = ( nodeMap.indexOfKey(eDPETHMAP_PIPE_NODEID_DVP) >= 0);
    MBOOL supportDLDEPTH = ( nodeMap.indexOfKey(eDPETHMAP_PIPE_NODEID_DLDEPTH) >=0 );
    MBOOL supportGF      = ( nodeMap.indexOfKey(eDPETHMAP_PIPE_NODEID_GF) >=0 );
    MBOOL supportSLANT   = ( nodeMap.indexOfKey(eDPETHMAP_PIPE_NODEID_SLANT) >= 0);

    if ( supportWPE )     pWPENode     = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_WPE);
    if ( supportDVP )     pDVPNode     = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_DVP);
    if ( supportDLDEPTH ) pDLDepthNode = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_DLDEPTH);
    if ( supportGF )      pGFNode      = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_GF);
    if ( supportSLANT )   pSlantNode   = nodeMap.valueFor(eDPETHMAP_PIPE_NODEID_SLANT);

    #define CONNECT_DATA(DataID, src, dst)\
        pPipe->connectData(DataID, DataID, src, dst);\
        mvAllowDataIDMap.add(DataID, MTRUE);

    // P2A to N3D + P2A_OUTput
    CONNECT_DATA(P2A_TO_N3D_PADDING_MATRIX, *pP2ANode, *pN3DNode);
    CONNECT_DATA(P2A_TO_N3D_RECT2_FEO, *pP2ANode, *pN3DNode);
    CONNECT_DATA(P2A_TO_N3D_NOFEFM_RECT2, *pP2ANode, *pN3DNode);
    CONNECT_DATA(P2A_TO_N3D_CAP_RECT2, *pP2ANode, *pN3DNode);
    CONNECT_DATA(TO_DUMP_IMG3O, *pP2ANode, pPipe);
    CONNECT_DATA(TO_DUMP_RAWS, *pP2ANode, pPipe);
    CONNECT_DATA(TO_DUMP_YUVS, *pP2ANode, pPipe);
    CONNECT_DATA(REQUEST_DEPTH_NOT_READY, *pP2ANode, pPipe);
    CONNECT_DATA(P2A_OUT_MV_F, *pP2ANode, pPipe);
    CONNECT_DATA(P2A_OUT_MV_F_CAP, *pP2ANode, pPipe);
    CONNECT_DATA(P2A_OUT_YUV_DONE, *pP2ANode, pPipe);
    CONNECT_DATA(TO_DUMP_IMG3O, *pP2ANode, pPipe);
    // used when need fefm
    CONNECT_DATA(P2A_OUT_MV_F, *pP2ANode, pPipe);
    CONNECT_DATA(P2A_OUT_FD, *pP2ANode, pPipe);
    // P2ABayer
    CONNECT_DATA(BAYER_ENQUE, *pP2ANode, *pP2ABayerNode);
    CONNECT_DATA(P2A_TO_N3D_FEOFMO, *pP2ABayerNode, *pN3DNode);
    CONNECT_DATA(P2A_OUT_FD, *pP2ABayerNode, pPipe);
    // N3DNode
    CONNECT_DATA(N3D_TO_DVS_MVSV_MASK, *pN3DNode, *pDVSNode);

    // depth output node (only one node can output depth)
    if ( supportGF ) {
        CONNECT_DATA(GF_OUT_DMBG, *pGFNode, pPipe);
        CONNECT_DATA(GF_OUT_INTERNAL_DMBG, *pGFNode, pPipe);
        CONNECT_DATA(GF_OUT_DEPTHMAP, *pGFNode, pPipe);
        CONNECT_DATA(GF_OUT_INTERNAL_DEPTH, *pGFNode, pPipe);
    }
    else if ( supportDVP ) {
        CONNECT_DATA(DVP_OUT_DEPTHMAP, *pDVPNode, pPipe);
        CONNECT_DATA(DVP_OUT_INTERNAL_DEPTH, *pDVPNode, pPipe);
    }
    else if ( supportSLANT ) {
        CONNECT_DATA(SLANT_OUT_DEPTHMAP, *pSlantNode, pPipe);
        CONNECT_DATA(SLANT_OUT_INTERNAL_DEPTH, *pSlantNode, pPipe);
    }
    else if ( supportDLDEPTH ) {
        CONNECT_DATA(DLDEPTH_OUT_DEPTHMAP, *pDLDepthNode, pPipe);
        CONNECT_DATA(DLDEPTH_OUT_INTERNAL_DEPTH, *pDLDepthNode, pPipe);
    }
    else {
        CONNECT_DATA(DVS_OUT_DEPTHMAP, *pDVSNode, pPipe);
        CONNECT_DATA(DVS_OUT_INTERNAL_DEPTH, *pDVSNode, pPipe);
    }

    // WPENode
    if ( supportWPE ) {
        // N3DNode
        CONNECT_DATA(N3D_TO_WPE_RECT2_WARPMTX, *pN3DNode, *pWPENode);
        CONNECT_DATA(N3D_TO_WPE_WARPMTX, *pN3DNode, *pWPENode);
        // DVSNode
        CONNECT_DATA(WPE_TO_DVS_WARP_IMG, *pWPENode, *pDVSNode);
    }

    // depth flow connection
    if ( supportDVP ) {
        // P2ANode
        CONNECT_DATA(P2A_TO_DVP_MY_S, *pP2ANode, *pDVPNode);
        // DVSNode
        CONNECT_DATA(DVS_TO_DVP_NOC_CFM, *pDVSNode, *pDVPNode);
        // SlantNode
        if ( supportSLANT )
        {
            CONNECT_DATA(SLANT_TO_DVP_NOC_CFM, *pSlantNode, *pDVPNode);
        }
    }
    if ( supportDLDEPTH ) {
        // P2ANode
        CONNECT_DATA(P2A_TO_DLDEPTH_MY_S, *pP2ANode, pDLDepthNode);
        // WPENode
        if (supportWPE) {
          CONNECT_DATA(WPE_TO_DLDEPTH_MV_SV, *pWPENode, *pDLDepthNode);
        }
        // DVSNode
        CONNECT_DATA(DVS_TO_DLDEPTH_MVSV_NOC, *pDVSNode, *pDLDepthNode);
    }
    if(supportGF)
    {
        if(supportDVP)
        {
            // DVPNode
            CONNECT_DATA(DVP_TO_GF_DMW_N_DEPTH, *pDVPNode, *pGFNode);
        }
        if(supportDLDEPTH)
        {
            // DLDepthNode
            CONNECT_DATA(DLDEPTH_TO_GF_DEPTHMAP, *pDLDepthNode, *pGFNode);
        }
        if ( supportSLANT )
        {
            // SlantNode
            CONNECT_DATA(SLANT_TO_GF_DEPTH_CFM, *pSlantNode, *pGFNode);
        }
    }
    // slant camera flow connection
    if ( supportSLANT )
    {
        // DVSNode
        CONNECT_DATA(DVS_TO_SLANT_NOC_CFM, *pDVSNode, *pSlantNode);
        // DLDepthNode
        if ( supportDLDEPTH )
        {
            CONNECT_DATA(DLDEPTH_TO_SLANT_DEPTH, *pDLDepthNode, *pSlantNode);
        }
    }

    // Hal Meta frame output
    CONNECT_DATA(DEPTHMAP_META_OUT, *pN3DNode, pPipe);

    // QUEUED DEPTH specific
    if(mpPipeOption->mFlowType == eDEPTH_FLOW_TYPE_QUEUED_DEPTH)
    {
        CONNECT_DATA(P2A_NORMAL_FRAME_DONE, *pP2ANode, pPipe);
        // queue flow done
        for(size_t index=0;index<nodeMap.size();++index)
        {
            CONNECT_DATA(QUEUED_FLOW_DONE, *nodeMap.valueAt(index), pPipe);
        }
    }
    // default node graph - Error handling
    for(size_t index=0;index<nodeMap.size();++index)
    {
        CONNECT_DATA(ERROR_OCCUR_NOTIFY, *nodeMap.valueAt(index), pPipe);
        CONNECT_DATA(DEPTHPIPE_BUF_RELEASED, *nodeMap.valueAt(index), pPipe);
    }
    pPipe->setRootNode(pP2ANode);

    return MTRUE;
}

MBOOL
DepthMapFlowOption_VSDOF::
checkConnected(
    DepthMapDataID dataID
)
{
    // check data id is allowed to handledata or not.
    if(mvAllowDataIDMap.indexOfKey(dataID)>=0)
    {
        return MTRUE;
    }
    return MFALSE;
}

NSIspTuning::EIspProfile_T
DepthMapFlowOption_VSDOF::
config3ATuningMeta(
    sp<DepthMapEffectRequest> pRequest,
    StereoP2Path path,
    MetaSet_T& rMetaSet
)
{
    // YUV input when capture scenaroi -> no need setIsp
    NSIspTuning::EIspProfile_T profile = EIspProfile_Preview;
    DepthMapPipeOpState opState = pRequest->getRequestAttr().opState;
    if(path == eP2APATH_MAIN1)
    {
        if(opState == eSTATE_NORMAL || opState == eSTATE_STANDALONE)
        {
            if(mpPipeOption->mSensorType == v1::Stereo::BAYER_AND_BAYER)
                profile = EIspProfile_VSDOF_PV_Main;
            else if(mpPipeOption->mSensorType == v1::Stereo::BAYER_AND_MONO)
                profile = EIspProfile_VSDOF_PV_Main_toW;
            else
                MY_LOGE("Unsupport sensor type, No IspProfile");
        }
        else if(opState == eSTATE_RECORD)
        {
            if(mpPipeOption->mSensorType == v1::Stereo::BAYER_AND_BAYER)
                profile = EIspProfile_VSDOF_Video_Main;
            else if(mpPipeOption->mSensorType == v1::Stereo::BAYER_AND_MONO)
                profile = EIspProfile_VSDOF_Video_Main_toW;
            else
                MY_LOGE("Unsupport sensor type, No IspProfile");
        }
        else
            MY_LOGE("Unsupport scenario, No IspProfile");
    }
    else if(path == eP2APATH_MAIN2)
    {
        if(opState == eSTATE_NORMAL)
            profile = EIspProfile_VSDOF_PV_Aux;
        else if (opState == eSTATE_RECORD)
            profile = EIspProfile_VSDOF_Video_Aux;
    }
    else if(path == eP2APATH_FE_MAIN1)
        profile = EIspProfile_VSDOF_FE_Main;
    else if(path == eP2APATH_FE_MAIN2)
        profile = EIspProfile_VSDOF_FE_Aux;
    else if(path == eP2APATH_FM)
        profile = EIspProfile_VSDOF_FM;
    else
    {
        MY_LOGE("Unsupport P2APath, No IspProfile");
    }
    trySetMetadata<MUINT8>(&rMetaSet.halMeta, MTK_3A_ISP_PROFILE, profile);
    return profile;
}

EIspProfile_T
DepthMapFlowOption_VSDOF::
getIspProfile_Bayer(sp<DepthMapEffectRequest> pRequest)
{
    // only for preview scenaios, no capture
    if(mpPipeOption->mSensorType == v1::Stereo::BAYER_AND_BAYER)
        return EIspProfile_N3D_Preview;
    else if(mpPipeOption->mSensorType == v1::Stereo::BAYER_AND_MONO)
        return EIspProfile_N3D_Preview_toW;
    else
    {
        MY_LOGE("Unsupport sensor type = %d", mpPipeOption->mSensorType);
        return EIspProfile_N3D_Preview;
    }
}

DepthMapBufferID
DepthMapFlowOption_VSDOF::
reMapBufferID(
    const EffectRequestAttrs& reqAttr,
    DepthMapBufferID bufferID
)
{
    return bufferID;
}

IImageBuffer*
DepthMapFlowOption_VSDOF::
get3DNRVIPIBuffer()
{
    return DepthQTemplateProvider::get3DNRVIPIBuffer();
}

}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam
