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
 * MediaTek Inc. (C) 2017. All rights reserved.
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
#include <list>
// Android system/core header file

// mtkcam custom header file
#include <camera_custom_stereo.h>
// mtkcam global header file
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/aaa/aaa_hal_common.h>
// #include <libion_mtk/include/ion.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
// Module header file
#include <stereo_tuning_provider.h>
#include <../../util/vsdof_util.h>
// Local header file
#include "P2ANode.h"
#include "../DepthMapPipe_Common.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"

// Logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "P2A_Node"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_P2A);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace NSCam::NSIoPipe::NSPostProc;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
P2ANode::
P2ANode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
, DataSequential<DepthMapRequestPtr>(reqNoGetter, "P2ANode")
, miSensorIdx_Main1(config.mpSetting->miSensorIdx_Main1)
, miSensorIdx_Main2(config.mpSetting->miSensorIdx_Main2)
, NR3DCommon()
{
    this->addWaitQueue(&mRequestQue);
}

P2ANode::
~P2ANode()
{
    MY_LOGD("[Destructor]");
}

MVOID
P2ANode::
cleanUp()
{
    MY_LOGD("+");
    if(mpINormalStream != NULL)
    {
        mpINormalStream->uninit("VSDOF_P2A");
        mpINormalStream->destroyInstance();
        mpINormalStream = NULL;
    }

    if(mpIspHal_Main1)
    {
        mpIspHal_Main1->destroyInstance("VSDOF_3A_MAIN1");
        mpIspHal_Main1 = NULL;
    }

    if(mpIspHal_Main2)
    {
        mpIspHal_Main2->destroyInstance("VSDOF_3A_MAIN2");
        mpIspHal_Main2 = NULL;
    }

    if(mpTransformer != nullptr)
    {
        mpTransformer->destroyInstance();
        mpTransformer = NULL;
    }

    if(mpDpIspStream != nullptr)
    {
        delete mpDpIspStream;
        mpDpIspStream = nullptr;
    }

     // nr3d
    if(!NR3DCommon::uninit())
    {
        MY_LOGE("Failed to uninit NR3D.");
    }

    MY_LOGD("-");
}

MBOOL
P2ANode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ANode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ANode::
onThreadStart()
{
    CAM_ULOGM_TAGLIFE("P2ANode::onThreadStart");
    VSDOF_INIT_LOG("+");
    // Create NormalStream
    VSDOF_LOGD("NormalStream create instance: idx=%d", miSensorIdx_Main1);
    CAM_ULOGM_TAG_BEGIN("P2ANode::NormalStream::createInstance+init");
    mpINormalStream = NSCam::NSIoPipe::NSPostProc::INormalStream::createInstance(miSensorIdx_Main1);

    if (mpINormalStream == NULL)
    {
        MY_LOGE("mpINormalStream create instance for P2A Node failed!");
        cleanUp();
        return MFALSE;
    }
    // create NormalStream for P2 or WPE
    if ( StereoSettingProvider::isSlantCameraModule() ) {
        mpINormalStream->init("VSDOF_P2A", NSCam::NSIoPipe::EStreamPipeID_WarpEG);
    } else {
        mpINormalStream->init("VSDOF_P2A");
    }
    CAM_ULOGM_TAG_END();
    // 3A: create instance
    // UT does not test 3A
    CAM_ULOGM_TAG_BEGIN("P2ANode::create_3A_instance");
    #ifndef GTEST
    mpIspHal_Main1 = MAKE_HalISP(miSensorIdx_Main1, "VSDOF_3A_MAIN1");
    mpIspHal_Main2 = MAKE_HalISP(miSensorIdx_Main2, "VSDOF_3A_MAIN2");
    MY_LOGD("ISP Hal create instance, Main1: %p Main2: %p", mpIspHal_Main1, mpIspHal_Main2);
    #endif
    // nr3d
    if(!NR3DCommon::init("P2ABayer3DNR", miSensorIdx_Main1))
    {
        MY_LOGE("Failed to init NR3D.");
        return MFALSE;
    }
    // create ImageTransformer
    mpTransformer = IImageTransform::createInstance(PIPE_CLASS_TAG, miSensorIdx_Main1);
    // create MDP stream
    mpDpIspStream = new DpIspStream(DpIspStream::ISP_ZSD_STREAM);
    CAM_ULOGM_TAG_END();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ANode::
onThreadStop()
{
    CAM_ULOGM_TAGLIFE("P2ANode::onThreadStop");
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
P2ANode::
onData(DataID data, DepthMapRequestPtr &request)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : reqID=%d", request->getRequestNo());
    CAM_ULOGM_TAGLIFE("P2ANode::onData");

    switch(data)
    {
        case ROOT_ENQUE:
            VSDOF_PRFLOG("+ : reqID=%d size=%lu skipDepth=%d",
                request->getRequestNo(), mRequestQue.size(), request->isSkipDepth(mpPipeOption));
            mRequestQue.enque(request);
            // notify N3D to do padding & matrix
            if(  request->getRequestAttr().opState != eSTATE_STANDALONE &&
                !request->isSkipDepth(mpPipeOption) )
                this->handleData(P2A_TO_N3D_PADDING_MATRIX, request);
            // dump mapping
            this->dumpSensorMapping(request);
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
P2ANode::
perform3AIspTuning(
    DepthMapRequestPtr& pRequest,
    Stereo3ATuningRes& rOutTuningRes
)
{
    pRequest->mTimer.startP2ASetIsp();
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        rOutTuningRes.tuningRes_main1 = applyISPTuning(pRequest, eP2APATH_MAIN1);
    }
    else if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        MY_LOGE("this flow should not get here for this image combination");
    }
    else
    {
        MY_LOGE("Not supported input image combination");
    }
    pRequest->mTimer.stopP2ASetIsp();
    return MTRUE;
}

AAATuningResult
P2ANode::
applyISPTuning(
    DepthMapRequestPtr& pRequest,
    StereoP2Path p2aPath
)
{
    CAM_ULOGM_TAGLIFE("P2ANode::applyISPTuning");
    MBOOL bIsMain1Path = (p2aPath == eP2APATH_MAIN1 || p2aPath == eP2APATH_FE_MAIN1);
    VSDOF_PRFTIME_LOG("+, reqID=%d bIsMain1Path=%d", pRequest->getRequestNo(), bIsMain1Path);
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // get in/out APP/HAL meta
    DepthMapBufferID halMetaBID = bIsMain1Path ? BID_META_IN_HAL_MAIN1 : BID_META_IN_HAL_MAIN2;
    IMetadata* pMeta_InApp  = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_APP );
    IMetadata* pMeta_InHal  = pBufferHandler->requestMetadata(getNodeId(), halMetaBID      );
    IMetadata* pMeta_OutApp = pBufferHandler->requestMetadata(getNodeId(), BID_META_OUT_APP);
    IMetadata* pMeta_OutHal = pBufferHandler->requestMetadata(getNodeId(), BID_META_OUT_HAL);
    // get tuning buf
    IImageBuffer* pTuningImgBuf = nullptr;
    MVOID* pTuningBuf = pBufferHandler->requestWorkingTuningBuf(getNodeId(), BID_P2A_TUNING);
    // in/out meta set
    MetaSet_T inMetaSet(*pMeta_InApp, *pMeta_InHal);
    MetaSet_T outMetaSet(*pMeta_OutApp, *pMeta_OutHal);
    // check raw type
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2 &&
        mpFlowOption->getInputRawType(pRequest, p2aPath) == eRESIZE_RAW)
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_3A_PGN_ENABLE, 0);
    else
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_3A_PGN_ENABLE, 1);
    // manully tuning for current flow option
    EIspProfile_T profile = mpFlowOption->config3ATuningMeta(pRequest, p2aPath, inMetaSet);
    // Main1 path need LCE & DRE when preview
    MVOID* pLcsBuf = nullptr;
    MVOID* pDreBuf = nullptr;
    MVOID* pLcshBuf = nullptr;
    if(pRequest->isRequestBuffer(BID_P2A_OUT_MV_F))
    {
        if(bIsMain1Path)
        {
            pLcsBuf = (void*)pBufferHandler->requestBuffer(getNodeId(), PBID_IN_LCSO1);
            pLcshBuf = (void*)pBufferHandler->requestBuffer(getNodeId(), PBID_IN_LCSHO1);
            pDreBuf = pRequest->extraData.pDreBuf;
        }
        else
        {
            updateEntry<MINT32>(&inMetaSet.halMeta, MTK_3A_ISP_BYPASS_LCE, true);
        }
    }
    // config meta for FE path
    if(p2aPath == eP2APATH_FE_MAIN1 || p2aPath == eP2APATH_FE_MAIN2)
    {
        updateEntry<MUINT8>(&inMetaSet.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ISP_TUNING_FE_PASS_MODE);
    }
    // UT do not test setP2Isp
    AAATuningResult result(pTuningBuf, pLcsBuf, pDreBuf, profile);
    result.tuningResult.pLceshoBuf = pLcshBuf;
    #ifndef GTEST
    IHalISP* p3AHAL = bIsMain1Path ? mpIspHal_Main1 : mpIspHal_Main2;
    p3AHAL->setP2Isp(0, inMetaSet, &result.tuningResult, &outMetaSet);
    #endif
    VSDOF_PRFTIME_LOG("-, reqID=%d", pRequest->getRequestNo());
    return result;
}


MBOOL
P2ANode::
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

    CAM_ULOGM_TAGLIFE("P2ANode::onThreadLoop");
    // mark on-going-pRequest start
    this->incExtThreadDependency();

    const EffectRequestAttrs& attr = pRequest->getRequestAttr();
    VSDOF_LOGD("threadLoop start, reqID=%d eState=%d needFEFM=%d",
                    pRequest->getRequestNo(), attr.opState, attr.needFEFM);
    // overall P2ANode timer
    pRequest->mTimer.startP2A();
    // enque cookie instance
    EnqueCookieContainer *pCookieIns = new EnqueCookieContainer(pRequest, this);

    MBOOL bRet = MTRUE;
    // only preview needs sequential: mark on going
    if(pRequest->getRequestAttr().opState != eSTATE_CAPTURE)
        this->markOnGoingData(pRequest);
    // execute P2A Flow
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        // execute P2-DL-MDP -> convert RAW to YUV
        bRet = handleWithP2(pCookieIns);
        if(!bRet)
            MY_LOGE("Main2 P2-DL-MDP Process Failed");
    }
    else if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        // standalone -> skip depth flow
        if(attr.opState == eSTATE_STANDALONE)
        {
            onHandleP2AProcessDone(pCookieIns, MTRUE);
        }
        else
        {
            // for slant camera with FEFM, use WPE for rotated FE1BInput and non-rotate MY_S
            if (pRequest->getRequestAttr().needFEFM &&
                StereoSettingProvider::isSlantCameraModule()) {
                bRet = handleWithWPE(pCookieIns);
                if (!bRet) { delete pCookieIns; }
            } else {
                bRet = handleWithMDP(pCookieIns);
            }
        }
    }
    else
    {
        delete pCookieIns;
        bRet = MFALSE;
        MY_LOGE("Unsupported input type, setting MUST has errro");
    }
    return bRet;
}

MBOOL
P2ANode::
handleWithMDP(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2ABayerNode::handleWithMDP");
    MBOOL bRet = MTRUE;
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    MBOOL bIsNeedFEFM = pRequest->getRequestAttr().needFEFM;
    MBOOL bIsNeedMY_S = mpFlowOption->checkConnected(P2A_TO_DVP_MY_S)
                        || mpFlowOption->checkConnected(P2A_TO_DLDEPTH_MY_S);
    VSDOF_LOGD("reqID=%d, do handleWithMDP use Main1 YUVO to gene %s",
                pRequest->getRequestNo(),
                (bIsNeedMY_S && bIsNeedFEFM) ? "MY_S + FE1BInput"
                : bIsNeedMY_S ? "MY_S"
                : bIsNeedFEFM ? "FE1BInput"
                : "Bypass"
                );

    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer *pMain1YUV1 = nullptr;
    IImageBuffer *pMY_S = nullptr, *pFe1bBuf_in = nullptr;
    MRect cropRect;
    // No need MDP round when both FEFM and MY_S are not needed
    if (bIsNeedFEFM || bIsNeedMY_S) {
        pMain1YUV1 = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_IN_YUV1);
        cropRect = MRect(pMain1YUV1->getImgSize().w, pMain1YUV1->getImgSize().h);
        // output : FE1BInput if need FEFM
        if (bIsNeedFEFM) {
            pFe1bBuf_in = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_FE1B_INPUT);
        }
        // output: MY_S if DVS+DVP
        if (bIsNeedMY_S) {
            pMY_S = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_OUT_MY_S);
            StereoSizeProvider *pSizeProvder = StereoSizeProvider::getInstance();
            StereoArea MYS_SIZE = pSizeProvder->getBufferSize(E_MY_S, mpPipeOption->mStereoScenario);
            pMY_S->setExtParam(MYS_SIZE.size - MYS_SIZE.padding);
        }
        // execute pure mdp
        pRequest->mTimer.startP2AMdp();
        if (!mpTransformer->execute(
                                pMain1YUV1,
                                bIsNeedMY_S ? pMY_S      : 0L,
                                bIsNeedFEFM ? pFe1bBuf_in: 0L,
                                bIsNeedMY_S ? cropRect   : 0L,
                                bIsNeedFEFM ? cropRect   : 0L,
                                bIsNeedMY_S ? remapTransform(StereoSettingProvider::getModuleRotation()) : 0L,
                                bIsNeedFEFM ? remapTransform(StereoSettingProvider::getModuleRotation()) : 0L,
                                0xFFFFFFFF,
                                NULL,   // not use ISP HW, do not need this argument
                                MFALSE, // do not use util to dump
                                MTRUE)) { // use half input buffer size to config MDP part while using WPE
            MY_LOGE("excuteMDP fail: Cannot perform MDP operation.");
            bRet = MFALSE;
        } else {
            pRequest->mTimer.stopP2AMdp();
            VSDOF_PRFTIME_LOG("+ :reqID=%d , pure mdp exec-time=%d ms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedP2AMdp());
        }
    }

    this->onHandleP2AProcessDone(pEnqueCookie, bRet);
    VSDOF_LOGD("- :reqID=%d", pRequest->getRequestNo());
    return bRet;
}

MBOOL
P2ANode::
handleWithP2(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2A::handleWithP2");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    VSDOF_LOGD("reqID=%d, do handleWithP2, gene FE", pRequest->getRequestNo());
    // enque QParams
    QParams enqueParams;
    // apply 3A Isp tuning
    Stereo3ATuningRes tuningRes;
    //
    MBOOL bRet = MTRUE;
    // perform 3dnr
    if(pRequest->isRequestBuffer(BID_P2A_OUT_MV_F))
        bRet &= perform3dnr(pRequest, mpFlowOption, tuningRes);
    // apply ISP
    if(!perform3AIspTuning(pRequest, tuningRes))
        return MFALSE;

    // call flow option to build QParams
    bRet = mpFlowOption->buildQParam(pRequest, tuningRes, enqueParams, pEnqueCookie);
    // debug param
    debugQParams(enqueParams);
    if(!bRet)
    {
        AEE_ASSERT("[P2ANode]Failed to build P2 enque parameters.");
        return MFALSE;
    }
    // callback
    enqueParams.mpfnCallback = onP2Callback;
    enqueParams.mpCookie = (MVOID*) pEnqueCookie;
    // enque
    CAM_ULOGM_TAG_BEGIN("P2ANode::NormalStream::enque");
    VSDOF_LOGD("mpINormalStream enque start! reqID=%d", pRequest->getRequestNo());
    pRequest->mTimer.startP2ADrv();
    bRet = mpINormalStream->enque(enqueParams);
    CAM_ULOGM_TAG_END();
    VSDOF_LOGD("mpINormalStream enque end! reqID=%d", pRequest->getRequestNo());
    // error handling
    if(!bRet)
    {
        AEE_ASSERT("[P2ANode] NormalStream enque failed");
        return MFALSE;
    }
    VSDOF_LOGD("- :reqID=%d", pRequest->getRequestNo());
    return MTRUE;
}

MBOOL
P2ANode::
handleWithWPE(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2A::handleWithWPE");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    VSDOF_LOGD("reqID=%d, do handleWithWPE", pRequest->getRequestNo());
    // enque QParams
    QParams enqueParams;
    // apply 3A Isp tuning
    Stereo3ATuningRes tuningRes;
    //
    MBOOL bRet = MTRUE;
    // call flow option to build QParams
    bRet = mpFlowOption->buildQParam(pRequest, tuningRes, enqueParams, pEnqueCookie);
    // debug param
    debugQParams(enqueParams);
    if(!bRet)
    {
        AEE_ASSERT("[P2ANode]Failed to build P2 enque parameters.");
        return MFALSE;
    }
    // callback
    enqueParams.mpfnCallback = onP2Callback;
    enqueParams.mpCookie = (MVOID*) pEnqueCookie;
    // enque
    CAM_ULOGM_TAG_BEGIN("P2ANode::NormalStream::enque");
    VSDOF_LOGD("mpINormalStream enque start! reqID=%d", pRequest->getRequestNo());
    pRequest->mTimer.startP2ADrv();
    bRet = mpINormalStream->enque(enqueParams);
    CAM_ULOGM_TAG_END();
    VSDOF_LOGD("mpINormalStream enque end! reqID=%d", pRequest->getRequestNo());
    // error handling
    if(!bRet)
    {
        AEE_ASSERT("[P2ANode] NormalStream enque failed");
        return MFALSE;
    }
    VSDOF_LOGD("- :reqID=%d", pRequest->getRequestNo());
    return MTRUE;
}

MVOID
P2ANode::
onP2Callback(QParams& rParams)
{
    EnqueCookieContainer* pEnqueCookie = (EnqueCookieContainer*) (rParams.mpCookie);
    P2ANode* pP2ANode = (P2ANode*) (pEnqueCookie->mpNode);
    pP2ANode->handleP2Done(pEnqueCookie);
}

MVOID
P2ANode::
handleP2Done(EnqueCookieContainer* pEnqueCookie)
{
    CAM_ULOGM_TAGLIFE("P2ANode::handleP2Done");
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    // stop timer
    pRequest->mTimer.stopP2ADrv();
    VSDOF_LOGD("reqID=%d , p2 exec-time=%d ms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedP2ADrv());
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        // dump buffer
        this->handleDump(TO_DUMP_IMG3O, pRequest);
        // wait for former MYS resize
        if(mResizeFuture.valid())
        {
            VSDOF_LOGD("reqID=%d wait for former MYS resize", pRequest->getRequestNo());
            mResizeFuture.wait();
            VSDOF_LOGD("reqID=%d wait for former MYS resize, done!", pRequest->getRequestNo());
        }
        // if need FEFM, need to generate the MY_S using MDP
        if((pRequest->getRequestAttr().opState == eSTATE_NORMAL &&
            pRequest->getRequestAttr().needFEFM))
        {
            VSDOF_LOGD("reqID=%d create thread for MYS resize", pRequest->getRequestNo());
            mResizeFuture = std::async(
                            std::launch::async,
                            &P2ANode::resizeFDIntoMYS, this, pEnqueCookie);
            return;
        }
    }
lbExit:
    onHandleP2AProcessDone(pEnqueCookie, MTRUE);
}

MBOOL
P2ANode::
onHandleP2AProcessDone(
    EnqueCookieContainer* pEnqueCookie,
    MBOOL isNoError
)
{
    // ensure the request is done in order
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    waitForPreReqFinish(pRequest);
    CAM_ULOGM_TAGLIFE("P2ANode::onHandleP2AProcessDone");
    MBOOL bRet = isNoError;
    MBOOL isFlush = mpNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH);
    DumpConfig config;
    if(!bRet || isFlush)
        goto lbExit;
    // dump buffer first
    if(pRequest->isRequestBuffer(BID_P2A_IN_RSRAW1))
    {
        config = DumpConfig(NULL, ".raw", MTRUE);
        this->handleDump(TO_DUMP_RAWS, pRequest, &config);
        if(pRequest->getRequestAttr().needFEFM)
            this->handleDump(TO_DUMP_IMG3O, pRequest);
    }
    if(pRequest->isRequestBuffer(BID_P1_OUT_RECT_IN1))
    {
        config = DumpConfig(NULL, NULL, MTRUE);
        this->handleDump(TO_DUMP_YUVS, pRequest, &config);
    }
    mpFlowOption->onP2ProcessDone(this, pRequest);
    // remove P2ANode from input request buffer map
    if (!pRequest->unRegisterInputBufferUser(this)) {
        MY_LOGE("unRegisterInputBufferUser failed");
    }
    this->configureToNext(pRequest);
    // handle flow type task
    if(!onHandleFlowTypeP2Done(pRequest))
    {
        MY_LOGE("onHandleFlowTypeP2Done failed!");
        bRet = MFALSE;
    }
lbExit:
    if(!bRet)
        MY_LOGE("Error:Drop Frame, reqID=%d", pRequest->getRequestNo());
    // launch onProcessDone
    pRequest->getBufferHandler()->onProcessDone(getNodeId());
    if ( pRequest->extraData.pDreBuf ){
        pRequest->extraData.pDreBuf = nullptr;
    }
    delete pEnqueCookie;
    // only preview/record need sequential
    if(pRequest->getRequestAttr().opState != eSTATE_CAPTURE)
        this->onHandleOnGoingReqReady(pRequest->getRequestNo());
    // mark on-going-request end
    this->decExtThreadDependency();
    mP2ACondition.broadcast();
    return bRet;
}

MBOOL
P2ANode::
onHandleOnGoingReqReady(
    MUINT32 iReqID
)
{
    // sequential: mark finish
    Vector<DepthMapRequestPtr> popReqVec;
    this->markFinishAndPop(iReqID, popReqVec);
    // pass to P2ABayer node
    for(size_t idx = 0;idx<popReqVec.size();++idx)
    {
        DepthMapRequestPtr pReq = popReqVec.itemAt(idx);
        VSDOF_LOGD("Seq: free the reqID=%d", pReq->getRequestNo());
        this->handleData(BAYER_ENQUE, pReq);
    }
    mInternalCondition.broadcast();
    return MTRUE;
}

MBOOL
P2ANode::
onHandleFlowTypeP2Done(
    sp<DepthMapEffectRequest> pRequest
)
{
    auto markOutBufferAndStatus = [&](DepthMapBufferID bufferID, int val) {
                                IImageBuffer* pBuf = nullptr;
                                if(pRequest->getRequestImageBuffer({.bufferID=bufferID, .ioType=eBUFFER_IOTYPE_OUTPUT}, pBuf))
                                {
                                    ::memset((void*)pBuf->getBufVA(0), val, pBuf->getBufSizeInBytes(0));
                                    pBuf->syncCache(eCACHECTRL_FLUSH);
                                    // mark ready
                                    pRequest->setOutputBufferReady(bufferID);
                                }
                            };
    // default value
    const int DMBG_DEFAULT_VAL = 0;
    const int DEPTH_DEFAULT_VAL = 255;

    MUINT32 iReqID = pRequest->getRequestNo();
    VSDOF_LOGD("reqID=%d +", iReqID);
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    if(pRequest->isQueuedDepthRequest(mpPipeOption) ||
        pRequest->getRequestAttr().opState == eSTATE_STANDALONE ||
        pRequest->isSkipDepth(mpPipeOption) )
    {
        IMetadata* pOutHalMeta = nullptr;
        // get queue depth info
        DepthBufferInfo depthInfo;
        MBOOL bDepthReady = MFALSE;
        // check
        if(miDepthDelay)
        {
            if(pRequest->getRequestAttr().opState == eSTATE_STANDALONE &&
                mpDepthStorage->getLatestData(depthInfo))
            {
                bDepthReady = MTRUE;
            }
            else if(pRequest->getRequestAttr().opState == eSTATE_NORMAL)
            {
                bDepthReady = mpDepthStorage->wait_pop(depthInfo);
            }
        }
        else if ( pRequest->isSkipDepth(mpPipeOption) )
            bDepthReady = MFALSE;
        else    // queued depth flow
            bDepthReady = mpDepthStorage->getStoredData(depthInfo);
        //
        if(!bDepthReady)
        {
            VSDOF_LOGD("reqID=%d, depth not ready!", pRequest->getRequestNo());
            // clear all
            markOutBufferAndStatus(BID_GF_OUT_DMBG, DMBG_DEFAULT_VAL);
            markOutBufferAndStatus(BID_DVS_OUT_UNPROCESS_DEPTH, DEPTH_DEFAULT_VAL);
            markOutBufferAndStatus(BID_GF_IN_DEBUG, DMBG_DEFAULT_VAL);
            handleData(REQUEST_DEPTH_NOT_READY, pRequest);
            goto lbExit;
        }
        VSDOF_LOGD("[DepthPipe] reqID=%d use the reqID=%d's blur map, depth delay is %d frames!",
            iReqID, depthInfo.miReqIdx, iReqID - depthInfo.miReqIdx);
        // handle stereo warning
        IMetadata* pMeta_Ret = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_P1_RETURN);
        IMetadata* pMeta_OutApp = pBufferHandler->requestMetadata(getNodeId(), BID_META_OUT_APP);
        MINT32 iWarningMsg = 0;
        if(tryGetMetadata<MINT32>(pMeta_Ret, MTK_STEREO_FEATURE_WARNING, iWarningMsg))
        {
            VSDOF_LOGD("reqID=%d warning msg=%d", pRequest->getRequestNo(), iWarningMsg);
            if(iWarningMsg != 0)
            {
                // clear DMBG
                markOutBufferAndStatus(BID_GF_OUT_DMBG, DMBG_DEFAULT_VAL);
                // clear depth
                markOutBufferAndStatus(BID_DVS_OUT_UNPROCESS_DEPTH, DEPTH_DEFAULT_VAL);
                // clear Debug
                markOutBufferAndStatus(BID_GF_IN_DEBUG, DMBG_DEFAULT_VAL);
            }
            // set meta
            trySetMetadata<MINT32>(pMeta_OutApp, MTK_STEREO_FEATURE_WARNING, iWarningMsg);
        }
        // set distance
        trySetMetadata<MFLOAT>(pMeta_OutApp, MTK_STEREO_FEATURE_RESULT_DISTANCE, depthInfo.mfDistance);
        // copy queued DMBG into request
        if(iWarningMsg == 0 &&
            pRequest->isRequestBuffer(BID_GF_OUT_DMBG) &&
            !_copyBufferIntoRequest(depthInfo.mpDMBGBuffer->mImageBuffer.get(),
                                    pRequest, BID_GF_OUT_DMBG))
        {
            AEE_ASSERT("[P2ABayerNode] Failed to copy BID_GF_OUT_DMBG");
            return MFALSE;
        }
        // copy queued DepthMap into request
        if(iWarningMsg == 0 &&
            pRequest->isRequestBuffer(BID_DVS_OUT_UNPROCESS_DEPTH) &&
            !_copyBufferIntoRequestWithCrop(depthInfo.mpDepthBuffer->mImageBuffer.get(),
                                    pRequest, BID_DVS_OUT_UNPROCESS_DEPTH))
        {
            AEE_ASSERT("[P2ABayerNode] Failed to copy BID_DVS_OUT_UNPROCESS_DEPTH");
            return MFALSE;
        }
        // copy queued Debug buffer into request, bypass if debugImg not blur map
        if(iWarningMsg == 0 &&
            mpPipeOption->mDebugInk > eDEPTH_DEBUG_IMG_BLUR_MAP &&
            mpPipeOption->mDebugInk < eDEPTH_DEBUG_IMG_INVALID &&
            pRequest->isRequestBuffer(BID_GF_IN_DEBUG) &&
            !_copyBufferIntoRequestWithCrop(depthInfo.mpDebugBuffer->mImageBuffer.get(),
                                    pRequest, BID_GF_IN_DEBUG))
        {
            MY_LOGW("[P2ABayerNode] Failed to copy BID_GF_IN_DEBUG");
        }
        // set meta ready
        pRequest->setOutputBufferReady(BID_META_OUT_APP);
        pRequest->setOutputBufferReady(BID_META_OUT_HAL);
        // notify pipe
        handleDataAndDump(P2A_NORMAL_FRAME_DONE, pRequest);
    }
lbExit:
    VSDOF_LOGD("reqID=%d -", pRequest->getRequestNo());
    return MTRUE;
}

MBOOL
P2ANode::
resizeFDIntoMYS(EnqueCookieContainer* pEnqueCookie)
{
    MBOOL bRet = MTRUE;
    DepthMapRequestPtr pRequest = pEnqueCookie->mRequest;
    VSDOF_LOGD("reqID=%d, do resizeFDIntoMYS", pRequest->getRequestNo());
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer* pFdBuf = nullptr;
    IImageBuffer* pMY_S = nullptr;
    VSDOF::util::sDpIspConfig config;
    DpPqParam pqParam;
    const MINT32 mysFrameNo = 10;
    MBOOL bExistFD;
    // generate MY_S by resizing FD or internal fd
    bExistFD = pRequest->getRequestImageBuffer({.bufferID=BID_P2A_OUT_FDIMG,
                                                .ioType=eBUFFER_IOTYPE_OUTPUT}, pFdBuf);
    if(!bExistFD && !pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_INTERNAL_FD, pFdBuf))
    {
        MY_LOGE("Cannot get the fd/internal fd buffer, reqID=%d", pRequest->getRequestNo());
        bRet = MFALSE;
        goto lbExit;
    }
    // output MY_S
    pMY_S = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_OUT_MY_S);

    // fill config
    config.pDpIspStream = mpDpIspStream;
    config.pSrcBuffer = pFdBuf;
    config.pDstBuffer = pMY_S;
    config.rotAngle = (int)StereoSettingProvider::getModuleRotation();
    // PQParam
    if(!configureDpPQParam(getNodeId(), mysFrameNo, pRequest, miSensorIdx_Main1, pqParam))
    {
        MY_LOGE("PQ config failed!.");
        bRet = MFALSE;
        goto lbExit;
    }
    config.pDpPqParam = &pqParam;

    pRequest->mTimer.startP2AMYSResize();
    if(!excuteDpIspStream(config))
    {
        MY_LOGE("excuteMDP fail: Cannot perform MDP operation on target1.");
        bRet = MFALSE;
        goto lbExit;
    }
    pRequest->mTimer.stopP2AMYSResize();
    VSDOF_PRFTIME_LOG("+ :reqID=%d , pure mdp exec-time=%d ms", pRequest->getRequestNo(), pRequest->mTimer.getElapsedP2AMYSResize());
lbExit:
    this->onHandleP2AProcessDone(pEnqueCookie, bRet);
    VSDOF_LOGD("- :reqID=%d", pRequest->getRequestNo());
    return bRet;
}

MBOOL
P2ANode::
configureToNext(DepthMapRequestPtr pRequest)
{
    VSDOF_LOGD("+, reqID=%d", pRequest->getRequestNo());
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // stop overall timer
    pRequest->mTimer.stopP2A();
    // mark buffer ready and handle data
    if(eSTATE_NORMAL == pRequest->getRequestAttr().opState ||
        eSTATE_STANDALONE == pRequest->getRequestAttr().opState)
    {
        // FD
        if(pRequest->setOutputBufferReady(BID_P2A_OUT_FDIMG))
            this->handleDataAndDump(P2A_OUT_FD, pRequest);
        // MV_F
        if(pRequest->setOutputBufferReady(BID_P2A_OUT_MV_F))
            this->handleDataAndDump(P2A_OUT_MV_F, pRequest);
    }
    else if(pRequest->getRequestAttr().opState == eSTATE_CAPTURE)
    {
        // PostView
        pRequest->setOutputBufferReady(BID_P2A_OUT_POSTVIEW);
        // notify YUV done
        this->handleDataAndDump(P2A_OUT_YUV_DONE, pRequest);
    }
    // if do depth
    if(!pRequest->isSkipDepth(mpPipeOption))
    {
        if(pRequest->getRequestAttr().opState == eSTATE_CAPTURE)
        {
            // FEO
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE1BO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE2BO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE1CO, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FE2CO, eDPETHMAP_PIPE_NODEID_N3D);
            // FMO
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMBO_LR, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMBO_RL, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMCO_LR, eDPETHMAP_PIPE_NODEID_N3D);
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_FMCO_RL, eDPETHMAP_PIPE_NODEID_N3D);
            // Rect_in2
            pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_RECT_IN2, eDPETHMAP_PIPE_NODEID_N3D);
            // MV_Y
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_MV_Y, eDPETHMAP_PIPE_NODEID_DLDEPTH);
            // pass to N3D
            this->handleDataAndDump(P2A_TO_N3D_CAP_RECT2, pRequest);
        }
        else    // PV or VR
        {
            // MY_S
            if(mpPipeOption->mFeatureMode != eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH)
            {
                if ( mpFlowOption->checkConnected(P2A_TO_DVP_MY_S) ) {
                    pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_MY_S, eDPETHMAP_PIPE_NODEID_DVP);
                    handleDataAndDump(P2A_TO_DVP_MY_S, pRequest);
                }
                else if ( mpFlowOption->checkConnected(P2A_TO_DLDEPTH_MY_S) ) {
                    pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_MY_S, eDPETHMAP_PIPE_NODEID_DLDEPTH);
                    handleDataAndDump(P2A_TO_DLDEPTH_MY_S, pRequest);
                }
            }
        }
    }
    // FEFM - independent from skipDepth
    if(pRequest->getRequestAttr().needFEFM)
    {
        pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_FE1B_INPUT, eDPETHMAP_PIPE_NODEID_P2ABAYER);
        this->handleData(BAYER_ENQUE, pRequest);
    }
    // notify YUV done
    if(pRequest->isRequestBuffer(BID_P2A_OUT_MV_F))
        this->handleDataAndDump(P2A_OUT_YUV_DONE, pRequest);

    VSDOF_LOGD("-, reqID=%d", pRequest->getRequestNo());
    return MTRUE;
}

MBOOL
P2ANode::
waitForPreReqFinish(sp<DepthMapEffectRequest> pRequest)
{
    Mutex::Autolock _l(mP2ALock);
    VSDOF_LOGD("reqID=%d wait for P2A Finish +", pRequest->getRequestNo());
    ssize_t index = -1;
    Timer time;
    time.start();
    while(this->getOldestOnGoingSequence() < pRequest->getRequestNo())
    {
        VSDOF_LOGD("reqID=%d wait for P2A Finish waiting!", pRequest->getRequestNo());
        mP2ACondition.wait(mP2ALock);
    }
    time.stop();
    VSDOF_LOGD("reqID=%d wait for P2A Finish :%dms", pRequest->getRequestNo(), time.getElapsed());
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P2ANode::
onFlush()
{
    MY_LOGD("+ extDep=%d", this->getExtThreadDependency());
    //
    if(mResizeFuture.valid())
        mResizeFuture.wait();
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

