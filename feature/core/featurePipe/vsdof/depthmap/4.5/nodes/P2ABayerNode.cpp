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
#include <cmath>
// Android system/core header file
#include <sync/sync.h>
// mtkcam custom header file
#include <camera_custom_stereo.h>
// mtkcam global header file
#include <isp_tuning.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/drv/iopipe/SImager/IImageTransform.h>
#include <mtkcam/aaa/aaa_hal_common.h>
// #include <libion_mtk/include/ion.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
// Module header file
#include <../../util/vsdof_util.h>

// Local header file
#include "P2ABayerNode.h"
#include "../DepthMapPipe_Common.h"
#include "../DepthMapPipeUtils.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"

// Logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "P2ABayerNode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_P2A_BAYER);
/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
P2ABayerNode::
P2ABayerNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
, DataSequential<DepthMapRequestPtr>(reqNoGetter, "P2ABayerNode")
, miSensorIdx_Main1(config.mpSetting->miSensorIdx_Main1)
, miSensorIdx_Main2(config.mpSetting->miSensorIdx_Main2)
{
    this->addWaitQueue(&mRequestQue);
}

P2ABayerNode::
~P2ABayerNode()
{
    MY_LOGD("[Destructor]");
}

MVOID
P2ABayerNode::
cleanUp()
{
    MY_LOGD("+");
    if(mpINormalStream != nullptr)
    {
        mpINormalStream->uninit(getName());
        mpINormalStream->destroyInstance();
        mpINormalStream = nullptr;
    }

    if(mpIspHal_Main1)
    {
        mpIspHal_Main1->destroyInstance(getName());
        mpIspHal_Main1 = nullptr;
    }

    if(mpTransformer != nullptr)
    {
        mpTransformer->destroyInstance();
        mpTransformer = NULL;
    }

    MY_LOGD("-");
}

MBOOL
P2ABayerNode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ABayerNode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ABayerNode::
onThreadStart()
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::onThreadStart");
    VSDOF_INIT_LOG("+");
    // Create NormalStream
    VSDOF_LOGD("NormalStream create instance: idx=%d", miSensorIdx_Main1);
    CAM_ULOGM_TAG_BEGIN("P2ABayerNode::NormalStream::createInstance+init");
    mpINormalStream = NSCam::NSIoPipe::NSPostProc::INormalStream::createInstance(miSensorIdx_Main1);

    if (mpINormalStream == nullptr)
    {
        MY_LOGE("mpINormalStream create instance for P2A Node failed!");
        cleanUp();
        return MFALSE;
    }
    mpINormalStream->init(getName());
    CAM_ULOGM_TAG_END();
    // 3A: create instance
    // UT does not test 3A
    CAM_ULOGM_TAG_BEGIN("P2ABayerNode::create_3A_instance");
    #ifndef GTEST
    mpIspHal_Main1 = MAKE_HalISP(miSensorIdx_Main1, getName());
    MY_LOGD("3A create instance, Main1: %p", mpIspHal_Main1);
    #endif
    CAM_ULOGM_TAG_END();
    // create ImageTransformer
    mpTransformer = IImageTransform::createInstance(PIPE_CLASS_TAG, miSensorIdx_Main1);
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ABayerNode::
onThreadStop()
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::onThreadStop");
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ABayerNode::
onData(DataID data, DepthMapRequestPtr &request)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : reqID=%d dataid=%s", request->getRequestNo(), ID2Name(data));
    CAM_ULOGM_TAGLIFE("P2ABayerNode::onData");

    switch(data)
    {
        case BAYER_ENQUE:   // from P2A
        {
            VSDOF_PRFLOG("+ : reqID=%d size=%lu", request->getRequestNo(), mRequestQue.size());
            mRequestQue.enque(request);
            break;
        }
        default:
            MY_LOGW("Un-recognized data ID, id=%d reqID=%d", data, request->getRequestNo());
            ret = MFALSE;
            break;
    }
    VSDOF_LOGD("-");
    return ret;
}

AAATuningResult
P2ABayerNode::
applyISPTuning(
    DepthMapRequestPtr& rpRequest,
    StereoP2Path p2aPath
)
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::applyISPTuning");
    MBOOL bIsMain1Path = (p2aPath == eP2APATH_MAIN1 || p2aPath == eP2APATH_FE_MAIN1);
    VSDOF_PRFTIME_LOG("+, reqID=%d bIsMain1Path=%d", rpRequest->getRequestNo(), bIsMain1Path);
    sp<BaseBufferHandler> pBufferHandler = rpRequest->getBufferHandler();
    // get in/out APP/HAL meta
    DepthMapBufferID halMetaBID = bIsMain1Path ? BID_META_IN_HAL_MAIN1 : BID_META_IN_HAL_MAIN2;
    IMetadata* pMeta_InApp  = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_APP );
    IMetadata* pMeta_InHal  = pBufferHandler->requestMetadata(getNodeId(), halMetaBID    );
    IMetadata* pMeta_OutApp = pBufferHandler->requestMetadata(getNodeId(), BID_META_OUT_APP);
    IMetadata* pMeta_OutHal = pBufferHandler->requestMetadata(getNodeId(), BID_META_OUT_HAL);
    // get tuning buf
    IImageBuffer* pTuningImgBuf = nullptr;
    MVOID* pTuningBuf = pBufferHandler->requestWorkingTuningBuf(getNodeId(), BID_P2A_TUNING);
    // in/out meta set
    MetaSet_T inMetaSet(*pMeta_InApp, *pMeta_InHal);
    MetaSet_T outMetaSet(*pMeta_OutApp, *pMeta_OutHal);
    // get raw type
    INPUT_RAW_TYPE rawType = mpFlowOption->getInputRawType(rpRequest, p2aPath);
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2 && rawType == eRESIZE_RAW)
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_3A_PGN_ENABLE, 0);
    else
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_3A_PGN_ENABLE, 1);    //// USE resize raw-->set PGN 0
    // config profile
    EIspProfile_T profile = mpFlowOption->config3ATuningMeta(rpRequest, p2aPath, inMetaSet);
    // FEPass
    if(p2aPath == eP2APATH_FE_MAIN1 || p2aPath == eP2APATH_FE_MAIN2)
    {
        updateEntry<MUINT8>(&inMetaSet.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ISP_TUNING_FE_PASS_MODE);
    }
    // UT do not test setP2Isp
    AAATuningResult result(pTuningBuf, nullptr, nullptr, profile);
    #ifndef GTEST
    //
    mpIspHal_Main1->setP2Isp(0, inMetaSet, &result.tuningResult, &outMetaSet);
    // only FULLRAW(capture) need to get exif result
    if(rpRequest->getRequestAttr().opState == eSTATE_CAPTURE && rawType == eFULLSIZE_RAW)
    {
        *pMeta_OutApp += outMetaSet.appMeta;
        *pMeta_OutHal += outMetaSet.halMeta;
        // Get standard EXIF info from input HAL metadata and set it to output HAL
        IMetadata exifMeta;
        if( tryGetMetadata<IMetadata>(pMeta_InHal, MTK_3A_EXIF_METADATA, exifMeta) ) {
            trySetMetadata<IMetadata>(pMeta_OutHal, MTK_3A_EXIF_METADATA, exifMeta);
        }
        else {
            MY_LOGW("no tag: MTK_3A_EXIF_METADATA");
        }
    }
    #endif
    VSDOF_PRFTIME_LOG("-, reqID=%d", rpRequest->getRequestNo());
    return result;
}

MBOOL
P2ABayerNode::
perform3AIspTuning(
    DepthMapRequestPtr& rpRequest,
    Stereo3ATuningRes& rOutTuningRes
)
{
// #ifdef GTEST
//     return MTRUE;
// #endif
    rpRequest->mTimer.startP2ABayerSetIsp();
    // only main1
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        rOutTuningRes.tuningRes_main2    = applyISPTuning(rpRequest, eP2APATH_MAIN2);
        rOutTuningRes.tuningRes_FE_main1 = applyISPTuning(rpRequest, eP2APATH_FE_MAIN1);
        rOutTuningRes.tuningRes_FE_main2 = applyISPTuning(rpRequest, eP2APATH_FE_MAIN2);
        rOutTuningRes.tuningRes_FM       = applyISPTuning(rpRequest, eP2APATH_FM);
    }
    else
    {
        rOutTuningRes.tuningRes_FE_main1 = applyISPTuning(rpRequest, eP2APATH_FE_MAIN1);
        rOutTuningRes.tuningRes_FE_main2 = applyISPTuning(rpRequest, eP2APATH_FE_MAIN2);
        rOutTuningRes.tuningRes_FM       = applyISPTuning(rpRequest, eP2APATH_FM);
    }
    rpRequest->mTimer.stopP2ABayerSetIsp();
    return MTRUE;
}

MBOOL
P2ABayerNode::
onThreadLoop()
{
    DepthMapRequestPtr pRequest;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }

    if( !mRequestQue.deque(pRequest) )
    {
        MY_LOGE("mRequestQue.deque() failed");
        return MFALSE;
    }
    CAM_ULOGM_TAGLIFE("P2ABayerNode::onThreadLoop");
    // mark on-going-request start
    this->incExtThreadDependency();

    VSDOF_LOGD("threadLoop start, reqID=%d eState=%d needFEFM:%d",
                    pRequest->getRequestNo(), pRequest->getRequestAttr().opState,
                    pRequest->getRequestAttr().needFEFM);
    // start P2A timer
    pRequest->mTimer.startP2ABayer();
    // enque cookie instance
    EnqueCookieContainer *pCookieIns = new EnqueCookieContainer(pRequest, this);
    //
    MBOOL bRet = MTRUE;
    //
    if(pRequest->getRequestAttr().opState != eSTATE_CAPTURE)
        this->markOnGoingData(pRequest);
    // process
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        bRet &= handleWithP2(pCookieIns);
        if(!bRet)
            MY_LOGE("Main1 P2-DL-MDP Process Failed");
    }
    else if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        // TODO add handle STANDALONE
        if(pRequest->getRequestAttr().opState == eSTATE_STANDALONE)
        {
            bRet &= this->onHandleP2ABayerProcessDone(pCookieIns, MTRUE);
        }
        else
        {
            if(pRequest->getRequestAttr().needFEFM)
            {
                bRet &= handleWithMDP(pCookieIns);
                if(!bRet){
                    MY_LOGE("Use Main2 YUVO to generate FE2BInput Failed");
                    delete pCookieIns;
                    return MFALSE;
                }
                bRet &= handleWithP2(pCookieIns);
                if(!bRet){
                    MY_LOGE("P2 FEFM Process Failed");
                }
            }
            else
            {
                VSDOF_LOGD("reqID=%d Bypass P2AbayerNode Flow", pRequest->getRequestNo());
                bRet &= this->onHandleP2ABayerProcessDone(pCookieIns, MTRUE);
            }
        }
    }
    else
    {
        delete pCookieIns;
        MY_LOGE("Unsupported input type, setting MUST has errro");
        return MFALSE;
    }

    return bRet;
}

MBOOL
P2ABayerNode::
handleWithMDP(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::handleWithMDP");
    MBOOL bRet = MTRUE;
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    MBOOL bIsNeedFEFM = pRequest->getRequestAttr().needFEFM;
    VSDOF_LOGD("reqID=%d, do handleWithMDP, use Main2YUVO to gene FE2BInput", pRequest->getRequestNo());
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer *pMain2YUVO = nullptr;
    IImageBuffer *pFe2bBuf_in = nullptr;
    IImageBuffer *pWarpMapX_Main2 = nullptr, *pWarpMapY_Main2 = nullptr;
    MSize imgSize;
    MRect cropRect;
    pMain2YUVO = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_IN_YUV2);
    if (!pMain2YUVO) {
        MY_LOGE("failed to get Main2 YUVO");
        return bRet;
    }
    imgSize = pMain2YUVO->getImgSize();
    cropRect = MRect(imgSize.w, imgSize.h);
    // output : FE2BInput
    pFe2bBuf_in = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_FE2B_INPUT);
    // slant camera case
    if (StereoSettingProvider::isSlantCameraModule()) {
        pWarpMapX_Main2 = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_WARPMTX_X_MAIN2);
        pWarpMapY_Main2 = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_WARPMTX_Y_MAIN2);
    }
    // normal case - execute pure mdp; slant case - execute wpe-dl-mdp
    pRequest->mTimer.startP2ABayerMdp();
    if(!mpTransformer->execute(
                        pMain2YUVO,
                        pWarpMapX_Main2,
                        pWarpMapY_Main2,
                        pFe2bBuf_in,
                        0L,
                        cropRect,
                        0L,
                        remapTransform(StereoSettingProvider::getModuleRotation()),
                        0L,
                        0xFFFFFFFF,
                        NULL,
                        MFALSE,
                        MFALSE) )
    {
        MY_LOGE("excuteMDP fail: Cannot perform MDP operation.");
        bRet = MFALSE;
        return bRet;
    }
    pRequest->mTimer.stopP2ABayerMdp();
    VSDOF_PRFTIME_LOG("+ :reqID=%d ,pure mdp exec-time=%d ms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedP2ABayerMdp());
    return bRet;
}

MBOOL
P2ABayerNode::
handleWithP2(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::handleWithP2");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    // enque QParams
    QParams enqueParams;
    // apply 3A Isp tuning
    Stereo3ATuningRes tuningRes;
    MBOOL bRet = MTRUE;

    bRet &= perform3AIspTuning(pRequest, tuningRes);
    if(!bRet)
        return MFALSE;

    // call flow option to build QParams
    bRet = mpFlowOption->buildQParam_Bayer(pRequest, tuningRes, enqueParams);
    //
    debugQParams(enqueParams);
    if(!bRet)
    {
        AEE_ASSERT("[P2ABayerNode]Failed to build P2 enque parametes.");
        return MFALSE;
    }
    // callback
    enqueParams.mpfnCallback = onP2Callback;
    enqueParams.mpCookie = (MVOID*) pEnqueCookie;
    // enque
    CAM_ULOGM_TAG_BEGIN("P2ABayerNode::NormalStream::enque");
    VSDOF_LOGD("mpINormalStream enque start! reqID=%d", pRequest->getRequestNo());
    pRequest->mTimer.startP2ABayerDrv();
    bRet = mpINormalStream->enque(enqueParams);
    CAM_ULOGM_TAG_END();
    VSDOF_LOGD("mpINormalStream enque end! reqID=%d", pRequest->getRequestNo());
    if(!bRet)
    {
        AEE_ASSERT("[P2ABayerNode] NormalStream enque failed");
        return MFALSE;
    }
    return MTRUE;
}

MVOID
P2ABayerNode::
onP2Callback(QParams& rParams)
{
    EnqueCookieContainer* pEnqueCookie = (EnqueCookieContainer*) (rParams.mpCookie);
    P2ABayerNode* pP2ABayerNode = (P2ABayerNode*) (pEnqueCookie->mpNode);
    pP2ABayerNode->handleP2Done(pEnqueCookie);
}

MVOID
P2ABayerNode::
handleP2Done(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::handleP2Done");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    // stop timer
    pRequest->mTimer.stopP2ABayerDrv();
    VSDOF_LOGD("reqID=%d , p2 exec-time=%d ms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedP2ABayerDrv());
    onHandleP2ABayerProcessDone(pEnqueCookie, MTRUE);
    VSDOF_PRFLOG("- :reqID=%d", pRequest->getRequestNo());
    return;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
P2ABayerNode::
onHandleP2ABayerProcessDone(
    EnqueCookieContainer* pEnqueCookie,
    MBOOL isNoError)
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::onHandleP2ABayerProcessDone");
    // wait former request process done
    Mutex::Autolock _l(mInternalLock);
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        VSDOF_LOGD("reqID=%d wait for previous request Finish +", pRequest->getRequestNo());
        pRequest->mTimer.startP2ABayerPrevReqWaiting();
        while(pRequest->getRequestNo() != this->getOldestDataOnGoingSequence()->getRequestNo())
        {
            VSDOF_LOGD("reqID=%d wait for previous request waiting!", pRequest->getRequestNo());
            mInternalCondition.wait(mInternalLock);
        }
        pRequest->mTimer.stopP2ABayerPrevReqWaiting();
    }
    VSDOF_LOGD("reqID=%d wait for previous request Finish :%dms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedP2ABayerPrevReqWaiting());
    // check flush status
    MBOOL bRet = isNoError;
    MBOOL isFlush = mpNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH);
    if(!bRet || isFlush)
        goto lbExit;
    // remove P2ABayerNode from input request buffer map
    if (!pRequest->unRegisterInputBufferUser(this)) {
        MY_LOGE("unRegisterInputBufferUser failed");
    }
    // config buffer to next node
    configureToNext(pRequest);
lbExit:
    // launch onProcessDone
    pRequest->getBufferHandler()->onProcessDone(getNodeId());
    delete pEnqueCookie;
    // sequential: mark finish
    Vector<DepthMapRequestPtr> popReqVec;
    this->markFinishAndPop(pRequest->getRequestNo(), popReqVec);
    mInternalCondition.broadcast();
    // mark on-going-request end
    this->decExtThreadDependency();
    return bRet;
}


MBOOL
P2ABayerNode::
configureToNext(DepthMapRequestPtr pRequest)
{
    // timer
    pRequest->mTimer.stopP2ABayer();
    // only capture frame goes here
    VSDOF_LOGD("+ reqID=%d", pRequest->getRequestNo());
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // MV_F_CAP
    if(pRequest->setOutputBufferReady(BID_P2A_OUT_MV_F_CAP))
        this->handleDataAndDump(P2A_OUT_MV_F_CAP, pRequest);

    if(pRequest->getRequestAttr().opState == eSTATE_NORMAL ||
        pRequest->getRequestAttr().opState == eSTATE_RECORD)
    {
        // Need FEFM -> output FEFM to N3D for learning
        if(pRequest->getRequestAttr().needFEFM)
        {
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE2BO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE2CO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE1BO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE1CO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMBO_LR, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMBO_RL, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMCO_LR, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMCO_RL, eDPETHMAP_PIPE_NODEID_N3D);
            this->handleDataAndDump(P2A_TO_N3D_FEOFMO, pRequest);
        }
    }

    VSDOF_LOGD("- reqID=%d", pRequest->getRequestNo());
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/

MVOID
P2ABayerNode::
onFlush()
{
    MY_LOGD("+ extDep=%d", this->getExtThreadDependency());
    DepthMapRequestPtr pRequest;
    while( mRequestQue.deque(pRequest) )
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

