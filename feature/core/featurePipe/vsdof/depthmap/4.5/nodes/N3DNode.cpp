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
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
// Module header file

// Local header file
#include "N3DNode.h"
#include "../DepthMapPipe_Common.h"
#include "../DepthMapPipeUtils.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"
#include "../StageExecutionTime.h"
// logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "N3DNode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_N3D);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {


#define SUPPORT_CAPTURE 0

const char* PHASE2Name(N3D_PVPHASE_ENUM phase)
{
#define MAKE_NAME_CASE(name) \
  case name: return #name;

  switch(phase)
  {
    MAKE_NAME_CASE(eN3D_PVPHASE_MAIN1_PADDING);
    MAKE_NAME_CASE(eN3D_PVPHASE_GPU_WARPING);
    MAKE_NAME_CASE(eN3D_PVPHASE_LEARNING);
  };
  return "UNKNOWN";
#undef MAKE_NAME_CASE
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

N3DNode::
N3DNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
, DataSequential(reqOrderGetter, "N3DSeq", "order")
, miSensorIdx_Main1(config.mpSetting->miSensorIdx_Main1)
, miSensorIdx_Main2(config.mpSetting->miSensorIdx_Main2)
{
    this->addWaitQueue(&mPriorityQueue);
    // set request order allow not-in-order ready
    DataSequential::allowNotInOrderReady(MTRUE);
}

N3DNode::
~N3DNode()
{
    MY_LOGD("[Destructor]");
}

MVOID
N3DNode::
cleanUp()
{
    VSDOF_LOGD("+");

    mpN3DHAL_CAP = nullptr;
    mpN3DHAL_VRPV = nullptr;

    if(mpTransformer != nullptr)
    {
        mpTransformer->destroyInstance();
        mpTransformer = NULL;
    }
    mPriorityQueue.clear();
    VSDOF_LOGD("-");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapPipeNode Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MBOOL
N3DNode::
onInit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
N3DNode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
N3DNode::
onThreadStart()
{
    VSDOF_INIT_LOG("+");
    CAM_ULOGM_TAGLIFE("N3DNode::onThreadStart");
    MBOOL bRet = MTRUE;
#if(SUPPORT_CAPTURE == 1)
    // create N3DHAL instance - Capture
    N3D_HAL_INIT_PARAM_WPE initParam_CAP;
    initParam_CAP.eScenario  = eSTEREO_SCENARIO_CAPTURE;
    initParam_CAP.fefmRound  = VSDOF_CONST_FE_EXEC_TIMES;

    bRet &= mpBuffPoolMgr->getAllPoolImageBuffer(BID_N3D_OUT_WARPMTX_MAIN2_X,
                                        eBUFFER_POOL_SCENARIO_CAPTURE,
                                        initParam_CAP.outputWarpMapMain2[0]);
    bRet &= mpBuffPoolMgr->getAllPoolImageBuffer(BID_N3D_OUT_WARPMTX_MAIN2_Y,
                                        eBUFFER_POOL_SCENARIO_CAPTURE,
                                        initParam_CAP.outputWarpMapMain2[1]);
    if(!bRet)
    {
        MY_LOGE("Failed to get all pool imagebuffer");
        return MFALSE;
    }
    mpN3DHAL_CAP = N3D_HAL::createInstance(initParam_CAP);
#endif
    // create N3DHAL instance - Preview/Record
    // init param is set by HW limitation
    //
    BufferPoolScenario eBufScenario = (mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW)?
                                        eBUFFER_POOL_SCENARIO_PREVIEW : eBUFFER_POOL_SCENARIO_RECORD;
    if (hasWPEHw()) {
        N3D_HAL_INIT_PARAM_WPE initParam_VRPV;
        initParam_VRPV.eScenario = mpPipeOption->mStereoScenario;
        initParam_VRPV.fefmRound = VSDOF_CONST_FE_EXEC_TIMES;
        bRet = mpBuffPoolMgr->getAllPoolImageBuffer(BID_N3D_OUT_WARPMTX_MAIN2_X,
                                            eBufScenario,
                                            initParam_VRPV.outputWarpMapMain2[0]);
        bRet &= mpBuffPoolMgr->getAllPoolImageBuffer(BID_N3D_OUT_WARPMTX_MAIN2_Y,
                                            eBufScenario,
                                            initParam_VRPV.outputWarpMapMain2[1]);
        mpN3DHAL_VRPV = N3D_HAL::createInstance(initParam_VRPV);
        MY_LOGD("create N3D_HAL instance for WPE");
    } else {
        N3D_HAL_INIT_PARAM initParam_VRPV;
        initParam_VRPV.eScenario = mpPipeOption->mStereoScenario;
        initParam_VRPV.fefmRound = VSDOF_CONST_FE_EXEC_TIMES;
        bRet = mpBuffPoolMgr->getAllPoolImageBuffer(BID_P2A_OUT_RECT_IN2,
                                            eBufScenario,
                                            initParam_VRPV.inputImageBuffers);
        bRet &= mpBuffPoolMgr->getAllPoolImageBuffer(BID_WPE_OUT_SV_Y,
                                            eBufScenario,
                                            initParam_VRPV.outputImageBuffers);
        bRet &= mpBuffPoolMgr->getAllPoolImageBuffer(BID_WPE_OUT_MASK_S,
                                            eBufScenario,
                                            initParam_VRPV.outputMaskBuffers);
        mpN3DHAL_VRPV = N3D_HAL::createInstance(initParam_VRPV);
        MY_LOGD("create N3D_HAL instance for GPU warping");
    }

    if(!bRet)
    {
        MY_LOGE("Failed to get all preview pool imagebuffer");
        return MFALSE;
    }
    // create ImageTransformer
    mpTransformer = IImageTransform::createInstance(PIPE_CLASS_TAG, miSensorIdx_Main1);
    //
    miMaxWorkingSize = CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS);
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
N3DNode::
onThreadStop()
{
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}


MBOOL
N3DNode::
onData(DataID data, DepthMapRequestPtr& pRequest)
{
    MBOOL ret = MTRUE;
    VSDOF_LOGD("+ : dataID=%s reqId=%d size=%lu", ID2Name(data), pRequest->getRequestNo(), mPriorityQueue.size());

    switch(data)
    {
        // first phase
        case P2A_TO_N3D_PADDING_MATRIX:
        // capture flow
        case P2A_TO_N3D_CAP_RECT2:
        // need FEFM - second phase
        case P2A_TO_N3D_RECT2_FEO:
        // NOFEFM - second phase
        case P2A_TO_N3D_NOFEFM_RECT2:
        // learning - third phase
        case P2A_TO_N3D_FEOFMO:
            mPriorityQueue.enque(pRequest);
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
N3DNode::
onThreadLoop()
{
    DepthMapRequestPtr pRequest;

    if( !waitAnyQueue() )
    {
        return MFALSE;
    }
    //
    if( !mPriorityQueue.deque(pRequest) )
    {
        MY_LOGE("mPriorityQueue.deque() failed");
        return MFALSE;
    }
    // not enough working set -> wait for latest
    if((pRequest->getRequestAttr().opState == eSTATE_NORMAL ||
        pRequest->getRequestAttr().opState == eSTATE_RECORD) &&
        DataSequential::getQueuedDataSize() >= miMaxWorkingSize)
    {
        DepthMapRequestPtr pOldestRequest = DataSequential::getOldestDataOnGoingSequence();
        VSDOF_LOGD("Enter not enough working set situation, deque oldest reqID=%d order=%lu",
                    pOldestRequest->getRequestNo(), pOldestRequest->getRequestOrder());
        // if not the oldest
        if(pOldestRequest->getRequestOrder() != pRequest->getRequestOrder())
        {
            MY_LOGW("Failed to get oldest request! oldest order=%lu   deque request order=%lu",
                        pOldestRequest->getRequestOrder(), pRequest->getRequestOrder());
            // push back
            mPriorityQueue.enque(pRequest);
            // and return
            return MTRUE;
        }
    }
    // mark on-going-request start
    this->incExtThreadDependency();
    VSDOF_LOGD("threadLoop start, reqID=%d order=%lu needFEFM=%d",
                pRequest->getRequestNo(), pRequest->getRequestOrder(), pRequest->getRequestAttr().needFEFM);
    CAM_ULOGM_TAGLIFE("N3DNode::onThreadLoop");
    //
    MBOOL ret;
    if(pRequest->getRequestAttr().opState == eSTATE_CAPTURE)
    {
        ret = performN3DALGO_CAP(pRequest);
    }
    else
    {
        // process done are launched inside it
        ret = performN3DALGO_VRPV(pRequest);
    }
    // error handling
    if(!ret)
    {
        #ifdef UNDER_DEVELOPMENT
        AEE_ASSERT("N3DRun fail, reqID=%d", pRequest->getRequestNo());
        #endif
        MY_LOGE("N3D operation failed: reqID=%d", pRequest->getRequestNo());
        // if error occur in the queued-flow, skip this operation and call queue-done
        if(pRequest->isQueuedDepthRequest(mpPipeOption))
            handleData(QUEUED_FLOW_DONE, pRequest);
        else
            handleData(ERROR_OCCUR_NOTIFY, pRequest);
    }
    // mark on-going-request end
    this->decExtThreadDependency();
    return ret;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  N3DNode Private Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MBOOL
N3DNode::
performN3DALGO_VRPV(DepthMapRequestPtr& pRequest)
{
    CAM_ULOGM_TAGLIFE("N3DNode::performN3DALGO_VRPV");
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // for a frame skip depth but need calibration(FEFM)
    // initialize N3D phase to learning since it do not need
    // DVS main1/main2 input
    if ( pRequest->getRequestAttr().needFEFM &&
         pRequest->isSkipDepth(mpPipeOption) ) {
        pRequest->mN3DPhase = eN3D_PVPHASE_LEARNING;
    }
    N3D_PVPHASE_ENUM phase = pRequest->mN3DPhase;
    int __moduleRot = StereoSettingProvider::getModuleRotation();

    VSDOF_LOGD("reqID=%d performN3DALGO_VRPV phase=%s moduleRot=%d",
                pRequest->getRequestNo(), PHASE2Name(phase), __moduleRot);
    MBOOL bRet = MTRUE;
    if(phase == eN3D_PVPHASE_MAIN1_PADDING)
    {
        // start timer
        pRequest->mTimer.startN3DPhase1();
        // mark on going
        this->markOnGoingData(pRequest);
        // input
        // generate Main1 input, for smaller depth size
        //      N3D_HAL - need resize P1_OUT_RSSOR2 to BID_P2A_OUT_RECT_IN2
        //      WPE - use warping grid
        IImageBuffer* pMain1Rssor2 = pBufferHandler->requestBuffer(getNodeId(), BID_P1_OUT_RECT_IN1);
        IImageBuffer* pRectIn1Buf = nullptr;
        pRequest->mTimer.startN3DMdp();
        if ( !StereoSettingProvider::isSlantCameraModule() ) {
            pRectIn1Buf = geneN3DMain1InputIfNeed(pRequest);
            if (pRectIn1Buf == nullptr) {
                MY_LOGE("nullptr pRectIn1Buf");
                return MFALSE;
            }
        } else {
            pRectIn1Buf = pMain1Rssor2;
        }
        pRequest->mTimer.stopN3DMdp();
        VSDOF_PRFTIME_LOG("+ :reqID=%d , pure mdp for RECT_IN1, exec-time=%d ms",
                        pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DMdp());
        // warp map
        IImageBuffer* pWarpMapX_Main1 = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN1_X);
        IImageBuffer* pWarpMapY_Main1 = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN1_Y);
        // output
        IImageBuffer* pImgBuf_MV_Y = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_MV_Y);
        IImageBuffer* pImgBuf_MASK_M = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_MASK_M);
        //
        CAM_ULOGM_TAG_BEGIN("N3DNode::N3DHALRun Main1 Warping");
        pRequest->mTimer.startN3DMain1Padding();
        if ( !StereoSettingProvider::isSlantCameraModule() ) {
            // generate MV_Y by N3D_HAL SW
            bRet = mpN3DHAL_VRPV->N3DHALWarpMain1(pRectIn1Buf, pImgBuf_MV_Y, pImgBuf_MASK_M);
        }
        else {
            // generate MV_Y by WPE HW
            bRet = executeImageTransform(pRectIn1Buf, pImgBuf_MV_Y, pWarpMapX_Main1, pWarpMapY_Main1);
        }
        pRequest->mTimer.stopN3DMain1Padding();
        CAM_ULOGM_TAG_END();
        //
        if(!bRet)
            MY_LOGE("reqID=%d, Generate MV_Y/MASK_M failed.", pRequest->getRequestNo());
        // generate warping mtx
        N3D_HAL_PARAM_WPE inParam;
        if(!prepareN3DInputMeta(pRequest, inParam))
        {
            MY_LOGE("reqID=%d, prepare input meta failed.", pRequest->getRequestNo());
            bRet = MFALSE;
        }
        N3D_HAL_OUTPUT_WPE outParam;
        outParam.warpMapMain2[0] = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_X);
        outParam.warpMapMain2[1] = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_Y);
        // get out RECT_IN2 for WPE
        if (__moduleRot == 90 || __moduleRot == 270) {
            inParam.rectifyImgMain2  = pBufferHandler->requestBuffer(getNodeId(), BID_P1_OUT_RECT_IN2);
            outParam.rectifyImgMain2 = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_OUT_RECT_IN2);
            VSDOF_LOGD("Needs MDP rotate in N3D Hal!!! P1_RectIn2=%p, P2A_RectIn2=%p",
                       static_cast<void*>(inParam.rectifyImgMain2),
                       static_cast<void*>(outParam.rectifyImgMain2));
        }
        debugN3DParams(outParam);
        //
        CAM_ULOGM_TAG_BEGIN("N3DNode::N3DHALRun get Main WarpingMatrix");
        pRequest->mTimer.startN3DMaskWarping();
        bRet &= mpN3DHAL_VRPV->getWarpMapMain2(inParam, outParam);
        pRequest->mTimer.stopN3DMaskWarping();
        CAM_ULOGM_TAG_END();
        if(!bRet)
            MY_LOGE("reqID=%d, Generate WarpingMatrix failed.", pRequest->getRequestNo());
        else
            writeN3DResultToMeta(outParam, pRequest);
        VSDOF_LOGD("start N3D(PV/VR) ALGO - Main1 Padding + Generate Main2 Mask, reqID=%d time=%d/%d ms",
                    pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DMain1Padding(), pRequest->mTimer.getElapsedN3DMaskWarping());
    }
    else if(phase == eN3D_PVPHASE_GPU_WARPING)
    {
        // start timer
        pRequest->mTimer.startN3DPhase1();
        // mark on going
        this->markOnGoingData(pRequest);
        pRequest->mTimer.startN3DMdp();
        // resize
        IImageBuffer* pMain1Rssor2 = geneN3DMain1InputIfNeed(pRequest);
        IImageBuffer* pMain2Rssor2 = geneN3DMain2Input(pRequest);
        pRequest->mTimer.stopN3DMdp();
        VSDOF_PRFTIME_LOG("+ :reqID=%d , pure mdp for RECT_IN1 & RECT_IN2, exec-time=%d ms",
                        pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DMdp());
        // input
        N3D_HAL_PARAM inParam;
        inParam.rectifyImgMain1 = pMain1Rssor2;
        inParam.rectifyImgMain2 = pMain2Rssor2;
        if(!prepareN3DInputMeta(pRequest, inParam))
        {
            MY_LOGE("reqID=%d, prepare input meta failed.", pRequest->getRequestNo());
            bRet = MFALSE;
        }
        // output
        N3D_HAL_OUTPUT outParam;
        outParam.rectifyImgMain1 = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_MV_Y);
        outParam.rectifyImgMain2 = pBufferHandler->requestBuffer(getNodeId(), BID_WPE_OUT_SV_Y);
        outParam.maskMain1 = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_MASK_M);
        outParam.maskMain2 = pBufferHandler->requestBuffer(getNodeId(), BID_WPE_OUT_MASK_S);
        //
        CAM_ULOGM_TAG_BEGIN("N3DNode::N3DHALRun");
        pRequest->mTimer.startN3DGpuWarping();
        bRet &= mpN3DHAL_VRPV->N3DHALRun(inParam, outParam);
        pRequest->mTimer.stopN3DGpuWarping();
        CAM_ULOGM_TAG_END();
        if(!bRet)
            MY_LOGE("reqID=%d, execute N3DHALRun failed.", pRequest->getRequestNo());
        else
            writeN3DResultToMeta(outParam, pRequest);
        VSDOF_LOGD("start N3D(PV/VR) ALGO - GPU warping, reqID=%d time=%d ms",
                    pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DGpuWarping());
    }
    else if(phase == eN3D_PVPHASE_LEARNING)
    {
        HWFEFM_DATA fefmData;
        if ( pRequest->isSkipDepth(mpPipeOption) ) {
            // mark on going - in skip depth case, no eN3D_PVPHASE_MAIN1_PADDING phase
            this->markOnGoingData(pRequest);
        }
        if(!prepareFEFMData(pBufferHandler, fefmData))
            return MFALSE;
        pRequest->mTimer.startN3DLearning();
        bRet = mpN3DHAL_VRPV->runN3DLearning(fefmData);
        pRequest->mTimer.stopN3DLearning();
        VSDOF_LOGD("N3D(PV/VR) Run learningstage. reqID=%d time=%d ms"
                        , pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DLearning());
    }
    //
    if(bRet)
    {
        if(phase == eN3D_PVPHASE_MAIN1_PADDING)
        {
            if (__moduleRot == 90 || __moduleRot == 270) {
                pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_RECT_IN2, eDPETHMAP_PIPE_NODEID_WPE);
            }
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_MV_Y, eDPETHMAP_PIPE_NODEID_DVS);
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_MASK_M, eDPETHMAP_PIPE_NODEID_DVS);
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_X, eDPETHMAP_PIPE_NODEID_WPE);
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_Y, eDPETHMAP_PIPE_NODEID_WPE);

            pRequest->mTimer.stopN3DPhase1();

            // remove N3DNode from input request buffer map
            if (!pRequest->unRegisterInputBufferUser(this)) {
                MY_LOGE("unRegisterInputBufferUser failed");
            }

            // Pass to WPE
            handleDataAndDump(N3D_TO_WPE_RECT2_WARPMTX, pRequest);
            if(!pRequest->getRequestAttr().needFEFM)
            {
                // sequential: mark finish
                this->markFinishNoInOder(pRequest);
                pRequest->getBufferHandler()->onProcessDone(getNodeId());
            }
            else
            {
                mvToDoLearningReqIDs.add(pRequest->getRequestNo(), MTRUE);
                pRequest->mN3DPhase = eN3D_PVPHASE_LEARNING;
            }
        }
        else if(phase == eN3D_PVPHASE_GPU_WARPING)
        {
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_MV_Y, eDPETHMAP_PIPE_NODEID_DVS);
            pBufferHandler->configOutBuffer(getNodeId(), BID_WPE_OUT_SV_Y, eDPETHMAP_PIPE_NODEID_DVS);
            pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_MASK_M, eDPETHMAP_PIPE_NODEID_DVS);
            pBufferHandler->configOutBuffer(getNodeId(), BID_WPE_OUT_MASK_S, eDPETHMAP_PIPE_NODEID_DVS);

            pRequest->mTimer.stopN3DPhase1();

            // remove N3DNode from input request buffer map
            if (!pRequest->unRegisterInputBufferUser(this)) {
                MY_LOGE("unRegisterInputBufferUser failed");
            }

            // Pass to DVS
            handleDataAndDump(N3D_TO_DVS_MVSV_MASK, pRequest);
            if(!pRequest->getRequestAttr().needFEFM)
            {
                // sequential: mark finish
                this->markFinishNoInOder(pRequest);
                pRequest->getBufferHandler()->onProcessDone(getNodeId());
            }
            else
            {
                mvToDoLearningReqIDs.add(pRequest->getRequestNo(), MTRUE);
                pRequest->mN3DPhase = eN3D_PVPHASE_LEARNING;
            }
        }
        else
        {
            // sequential: mark finish
            this->markFinishNoInOder(pRequest);
            // remove todo item
            mvToDoLearningReqIDs.removeItem(pRequest->getRequestNo());
            pBufferHandler->onProcessDone(getNodeId());
        }
    }

    return bRet;
}

IImageBuffer*
N3DNode::
geneN3DMain1InputIfNeed(DepthMapRequestPtr& pRequest)
{
    CAM_ULOGM_TAGLIFE("N3DNode::geneN3DMain1InputIfNeed");
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // query RECT_IN1 buffer size
    StereoSizeProvider* pSizePrvder = StereoSizeProvider::getInstance();
    IImageBuffer* pMain1Rssor2 = pBufferHandler->requestBuffer(getNodeId(), BID_P1_OUT_RECT_IN1);
    // check size
    const MSize RectIn1_size = pSizePrvder->getBufferSize(E_RECT_IN_M, mpPipeOption->mStereoScenario);
    const MSize Rssor2_size = pMain1Rssor2->getImgSize();
    // IF the size is equal, no need to use MDP & return Main RSSOR2
    if(RectIn1_size.w == Rssor2_size.w && RectIn1_size.h == Rssor2_size.h) {
        return pMain1Rssor2;
    }
    IImageBuffer* pRect_In1 = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_OUT_RECT_IN1);
    // enque
    if (!executeImageTransform(pMain1Rssor2, pRect_In1)) {
        MY_LOGE("failed to transform MAIN1 RSSOR2 to RECT_IN1");
        return nullptr;
    }
    return pRect_In1;
}

IImageBuffer* N3DNode::geneN3DMain2Input(const DepthMapRequestPtr& pRequest) {
    CAM_ULOGM_TAGLIFE("N3DNode::geneN3DMain2Input");
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer* pMain2Rssor2 = pBufferHandler->requestBuffer(getNodeId(), BID_P1_OUT_RECT_IN2);
    IImageBuffer* pRect_In2 = pBufferHandler->requestBuffer(getNodeId(), BID_P2A_OUT_RECT_IN2);
    // enque
    if (!executeImageTransform(pMain2Rssor2, pRect_In2)) {
        MY_LOGE("failed to transform MAIN2 RSSOR2 to RECT_IN2");
        return nullptr;
    }
    return pRect_In2;
}

MBOOL
N3DNode::
executeImageTransform(
    IImageBuffer* pSrcBuffer,
    IImageBuffer* pDstBuffer,
    IImageBuffer* pWarpMapX,
    IImageBuffer* pWarpMapY
)
{
    CAM_ULOGM_TAGLIFE("N3DNode::executeImageTransform");
    const MSize srcSize = pSrcBuffer->getImgSize();
    const MSize dstSize = pDstBuffer->getImgSize();
    const MRect cropRect = MRect(srcSize.w, srcSize.h);
    VSDOF_LOGD("srcSize:%dx%d dstSize:%dx%d", srcSize.w, srcSize.h, dstSize.w, dstSize.h);
    if(!mpTransformer->execute(
                            pSrcBuffer,
                            pWarpMapX,
                            pWarpMapY,
                            pDstBuffer,
                            0L,
                            cropRect,
                            0,
                            0,
                            0,
                            0xFFFFFFFF,
                            NULL,
                            MFALSE,
                            MFALSE) )
    {
        MY_LOGE("excuteMDP fail: Cannot perform MDP operation.");
        return MFALSE;
    }
    return MTRUE;
}

MBOOL
N3DNode::
prepareN3DInputMeta(
    DepthMapRequestPtr& pRequest,
    N3D_HAL_PARAM_WPE& rN3dParam
)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    //
    MINT32 magicNum1 = 0;
    MINT32 magicNum2 = 0;
    //
    IMetadata* pInHalMeta_Main1 = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN1);
    IMetadata* pInHalMeta_Main2 = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN2);
    if(!tryGetMetadata<MINT32>(pInHalMeta_Main1, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum1)) {
        MY_LOGE("Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta of Main1!");
        return MFALSE;
    }
    if(!tryGetMetadata<MINT32>(pInHalMeta_Main2, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum2)) {
        MY_LOGE("Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta of Main2!");
        return MFALSE;
    }
    //
    IMetadata* pInAppMeta = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_APP);
    MFLOAT zoomRatio = 1;
    if(!tryGetMetadata<MFLOAT>(pInAppMeta, MTK_CONTROL_ZOOM_RATIO, zoomRatio)) {
        MY_LOGW("Cannot find MTK_CONTROL_ZOOM_RATIO meta!");
    }
    // prepare params
    rN3dParam.magicNumber[0] = magicNum1;
    rN3dParam.magicNumber[1] = magicNum2;
    rN3dParam.requestNumber = pRequest->getRequestNo();
    rN3dParam.previewCropRatio = 1 / zoomRatio;
    //
    if(!checkToDump(N3D_TO_WPE_RECT2_WARPMTX, pRequest))
    {
        rN3dParam.dumpHint = nullptr;
    }
    else
    {
        extract(&mDumpHint_Main1, pInHalMeta_Main1);
        rN3dParam.dumpHint = &mDumpHint_Main1;
        rN3dParam.dumpLevel = miNDDLevel;
    }
    return MTRUE;
}


MBOOL
N3DNode::
prepareN3DInputMeta(
    DepthMapRequestPtr& pRequest,
    N3D_HAL_PARAM& rN3dParam
)
{
    VSDOF_LOGD("prepareN3DInputMeta");
    CAM_ULOGM_TAGLIFE("prepareN3DInputMeta");
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    //
    MINT32 depthAFON = 0;
    MINT32 disMeasureON = 0;
    MUINT8 isAFTrigger = 0;
    MINT32 magicNum1 = 0;
    MINT32 magicNum2 = 0;
    //
    IMetadata* pInAppMeta = pBufferHandler->requestMetadata(
                                getNodeId(), BID_META_IN_APP);
    //  AF ON
    if (!tryGetMetadata<MINT32>(pInAppMeta,
                               MTK_STEREO_FEATURE_DEPTH_AF_ON,
                               depthAFON)) {
        VSDOF_LOGD(
            "reqID=%d, Cannot find MTK_STEREO_FEATURE_DEPTH_AF_ON meta!",
            pRequest->getRequestNo());
    }
    //  Distance Measurement
    if (!tryGetMetadata<MINT32>(pInAppMeta,
                               MTK_STEREO_FEATURE_DISTANCE_MEASURE_ON,
                               disMeasureON)) {
        VSDOF_LOGD(
            "reqID=%d, Cannot find MTK_STEREO_FEATURE_DISTANCE_MEASURE_ON meta!",
            pRequest->getRequestNo());
    }
    //  AF Trigger
    if (!tryGetMetadata<MUINT8>(pInAppMeta,
                               MTK_CONTROL_AF_TRIGGER,
                               isAFTrigger)) {
        VSDOF_LOGD(
            "reqID=%d, Cannot find MTK_CONTROL_AF_TRIGGER meta!",
            pRequest->getRequestNo());
    }
    //  Magic number
    IMetadata* pInHalMeta_Main1 = pBufferHandler->requestMetadata(
                                    getNodeId(), BID_META_IN_HAL_MAIN1);
    if (!tryGetMetadata<MINT32>(pInHalMeta_Main1,
                               MTK_P1NODE_PROCESSOR_MAGICNUM,
                               magicNum1)) {
        VSDOF_LOGD(
            "reqID=%d, Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta! of Main1",
            pRequest->getRequestNo());
    }
    IMetadata* pInHalMeta_Main2 = pBufferHandler->requestMetadata(
                                    getNodeId(), BID_META_IN_HAL_MAIN2);
    if (!tryGetMetadata<MINT32>(pInHalMeta_Main2,
                               MTK_P1NODE_PROCESSOR_MAGICNUM,
                               magicNum2)) {
        VSDOF_LOGD(
            "reqID=%d, Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta! of Main2",
            pRequest->getRequestNo());
    }
    // prepare params
    rN3dParam.magicNumber[0] = magicNum1;
    rN3dParam.magicNumber[1] = magicNum2;
    rN3dParam.requestNumber = pRequest->getRequestNo();
    rN3dParam.isAFTrigger = isAFTrigger;
    rN3dParam.isDepthAFON = depthAFON;
    rN3dParam.isDistanceMeasurementON = disMeasureON;
    // prepare EIS data
    prepareEISData(pInAppMeta, pInHalMeta_Main1, rN3dParam.eisData);
    // dumpHint
    if(!checkToDump(N3D_TO_DVS_MVSV_MASK, pRequest)) {
        rN3dParam.dumpHint = nullptr;
    } else {
        extract(&mDumpHint_Main1, pInHalMeta_Main1);
        rN3dParam.dumpHint = &mDumpHint_Main1;
    }
    return MTRUE;
}

MBOOL
N3DNode::
writeN3DResultToMeta(
    const N3D_HAL_OUTPUT_COMMON& n3dOutput,
    DepthMapRequestPtr& pRequest
)
{
    IMetadata* pOutAppMeta = pRequest->getBufferHandler()->requestMetadata(getNodeId(), BID_META_OUT_APP);
    VSDOF_LOGD("reqID=%d output distance:%f", pRequest->getRequestNo(), n3dOutput.distance);
    trySetMetadata<MFLOAT>(pOutAppMeta, MTK_STEREO_FEATURE_RESULT_DISTANCE, n3dOutput.distance);
    // set outAppMeta ready
    pRequest->setOutputBufferReady(BID_META_OUT_APP);

    IMetadata* pOutHalMeta = pRequest->getBufferHandler()->requestMetadata(getNodeId(), BID_META_OUT_HAL);
    VSDOF_LOGD("reqID=%d output convOffset:%f", pRequest->getRequestNo(), n3dOutput.convOffset);
    trySetMetadata<MFLOAT>(pOutHalMeta, MTK_CONVERGENCE_DEPTH_OFFSET, n3dOutput.convOffset);
    // set outHalMeta ready
    pRequest->setOutputBufferReady(BID_META_OUT_HAL);

    // pass data finish
    if(!pRequest->isQueuedDepthRequest(mpPipeOption))
        this->handleData(DEPTHMAP_META_OUT, pRequest);
    return MTRUE;
}

MBOOL
N3DNode::
performN3DALGO_CAP(DepthMapRequestPtr& pRequest)
{
    CAM_ULOGM_TAGLIFE("N3DNode::performN3DALGO_CAP");
    // start timer
    pRequest->mTimer.startN3D();
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();

    MY_LOGD("performN3DALGO_CAP reqID=%d", pRequest->getRequestNo());
    // do learning first
    MBOOL bRet = MTRUE;
    N3D_HAL_PARAM_WPE inParam;
    if(!prepareN3DInputMeta(pRequest, inParam))
    {
        MY_LOGE("reqID=%d, prepare input meta failed.", pRequest->getRequestNo());
        bRet = MFALSE;
    }

    HWFEFM_DATA fefmData;
    fefmData.magicNumber[0] = inParam.magicNumber[0];
    fefmData.magicNumber[1] = inParam.magicNumber[1];
    fefmData.dumpHint = inParam.dumpHint;
    if(!prepareFEFMData(pBufferHandler, fefmData))
        return MFALSE;

    pRequest->mTimer.startN3DLearning();
    bRet = mpN3DHAL_CAP->runN3DLearning(fefmData);
    pRequest->mTimer.stopN3DLearning();
    VSDOF_PRFLOG("N3D(CAP) Run learningstage. reqID=%d time=%d ms"
                    , pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DLearning());
    // generate warping mtx next
    N3D_HAL_OUTPUT_WPE outParam;
    outParam.warpMapMain2[0] = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_X);
    outParam.warpMapMain2[1] = pBufferHandler->requestBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_Y);
    debugN3DParams(outParam);
    //
    pRequest->mTimer.startN3DMaskWarping();
    bRet &= mpN3DHAL_CAP->getWarpMapMain2(inParam, outParam);
    pRequest->mTimer.stopN3DMaskWarping();
    if(!bRet)
        MY_LOGE("reqID=%d, Generate capture WarpingMatrix failed.", pRequest->getRequestNo());
    else
    {
        VSDOF_PRFLOG("N3D(CAP) Run : Generate Main2 Mask, reqID=%d time=%d ms",
                    pRequest->getRequestNo(), pRequest->mTimer.getElapsedN3DMaskWarping());
        // write meta
        writeN3DResultToMeta(outParam, pRequest);
        // config output
        pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_X, eDPETHMAP_PIPE_NODEID_WPE);
        pBufferHandler->configOutBuffer(getNodeId(), BID_N3D_OUT_WARPMTX_MAIN2_Y, eDPETHMAP_PIPE_NODEID_WPE);
        pBufferHandler->configOutBuffer(getNodeId(), BID_P2A_OUT_RECT_IN2, eDPETHMAP_PIPE_NODEID_WPE);
        // pass to WPE
        this->handleDataAndDump(N3D_TO_WPE_RECT2_WARPMTX, pRequest);
    }
    // handle process done
    pBufferHandler->onProcessDone(getNodeId());

    return MTRUE;
}

MBOOL
N3DNode::
copyImageBuffer(sp<IImageBuffer> srcImgBuf, sp<IImageBuffer> dstImgBuf)
{
    if(srcImgBuf->getPlaneCount() != dstImgBuf->getPlaneCount())
    {
        MY_LOGE("source/destination image buffer has different plane count! cannot copy!");
        return MFALSE;
    }
    for(int index=0;index<srcImgBuf->getPlaneCount();++index)
    {
        if(srcImgBuf->getBufSizeInBytes(index) != dstImgBuf->getBufSizeInBytes(index))
        {
            MY_LOGE("The %d-st plane of source/destination image buffer has different buffer size! cannot copy!", index);
            return MFALSE;
        }
        // memory copy
        memcpy((MUINT8*)dstImgBuf->getBufVA(index), (MUINT8*)srcImgBuf->getBufVA(index), srcImgBuf->getBufSizeInBytes(index));
    }
    return MTRUE;
}

MVOID
N3DNode::
debugN3DParams(N3D_HAL_OUTPUT_WPE& param)
{
    if(DepthPipeLoggingSetup::miDebugLog<2)
        return;

    MY_LOGD("+");
    for(int i=0;i<WPE_PLANE_COUNT;i++)
    {
        MY_LOGD("param.warpMapMain2[%d]=%p", i, param.warpMapMain2[i]);
        MY_LOGD("param.warpMapMain1[%d]=%p", i, param.warpMapMain1[i]);
    }
    if(param.rectifyImgMain2)
    {
        MY_LOGD("param.rectifyImgMain2=%p", param.rectifyImgMain2);
    }
    MY_LOGD("-");
}

MVOID
N3DNode::
debugN3DParams(N3D_HAL_OUTPUT& param)
{
    if(DepthPipeLoggingSetup::miDebugLog<2)
        return;

    MY_LOGD("+");
    if(param.rectifyImgMain1)
        MY_LOGD("param.rectifyImgMain1=%p", param.rectifyImgMain1);
    if(param.rectifyImgMain2)
        MY_LOGD("param.rectifyImgMain2=%p", param.rectifyImgMain2);
    if(param.maskMain1)
        MY_LOGD("param.maskMain1=%p", param.maskMain1);
    if(param.maskMain2)
        MY_LOGD("param.maskMain2=%p", param.maskMain2);
    MY_LOGD("-");
}

MBOOL
N3DNode::
prepareFEFMData(sp<BaseBufferHandler>& pBufferHandler, HWFEFM_DATA& rFefmData)
{
    CAM_ULOGM_TAGLIFE("prepareFEFMData");
    VSDOF_LOGD("prepareFEFMData");
    // N3D input FEO/FMO data
    IImageBuffer *pFe1boBuf = nullptr, *pFe2boBuf = nullptr;
    IImageBuffer *pFe1coBuf = nullptr, *pFe2coBuf = nullptr;
    IImageBuffer *pFmboLRBuf = nullptr, *pFmboRLBuf = nullptr;
    IImageBuffer *pFmcoLRBuf = nullptr, *pFmcoRLBuf = nullptr;

    MBOOL bRet = MTRUE;
    // Get FEO
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FE1BO, pFe1boBuf);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FE2BO, pFe2boBuf);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FE1CO, pFe1coBuf);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FE2CO, pFe2coBuf);
    // syncCache
    pFe1boBuf->syncCache(eCACHECTRL_INVALID);
    pFe2boBuf->syncCache(eCACHECTRL_INVALID);
    pFe1coBuf->syncCache(eCACHECTRL_INVALID);
    pFe2coBuf->syncCache(eCACHECTRL_INVALID);

    // insert params
    rFefmData.geoDataMain1[0] = pFe1boBuf;
    rFefmData.geoDataMain1[1] = pFe1coBuf;
    rFefmData.geoDataMain1[2] = NULL;
    rFefmData.geoDataMain2[0] = pFe2boBuf;
    rFefmData.geoDataMain2[1] = pFe2coBuf;
    rFefmData.geoDataMain2[2] = NULL;

    // Get FMO
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FMBO_LR, pFmboLRBuf);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FMBO_RL, pFmboRLBuf);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FMCO_LR, pFmcoLRBuf);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_FMCO_RL, pFmcoRLBuf);
    // sync cache
    pFmboLRBuf->syncCache(eCACHECTRL_INVALID);
    pFmboRLBuf->syncCache(eCACHECTRL_INVALID);
    pFmcoLRBuf->syncCache(eCACHECTRL_INVALID);
    pFmcoRLBuf->syncCache(eCACHECTRL_INVALID);

    // insert params
    rFefmData.geoDataLeftToRight[0] = pFmboLRBuf;
    rFefmData.geoDataLeftToRight[1] = pFmcoLRBuf;
    rFefmData.geoDataLeftToRight[2] = NULL;
    rFefmData.geoDataRightToLeft[0] = pFmboRLBuf;
    rFefmData.geoDataRightToLeft[1] = pFmcoRLBuf;
    rFefmData.geoDataRightToLeft[2] = NULL;

    if(!bRet)
        MY_LOGE("Failed to get FEFM buffers: %p %p %p %p %p %p %p %p",
                pFe1boBuf, pFe1coBuf, pFe2boBuf, pFe2coBuf,
                pFmboLRBuf, pFmcoLRBuf, pFmboRLBuf, pFmcoRLBuf);

    return bRet;
}


MBOOL
N3DNode::
prepareEISData(
    IMetadata*& pInAppMeta,
    IMetadata*& pInHalMeta_Main1,
    EIS_DATA& rEISData
)
{
    if(isEISOn(pInAppMeta))
    {
        eis_region region;
        if(queryEisRegion(pInHalMeta_Main1, region))
        {
            rEISData.isON = true;
            rEISData.eisOffset.x = region.x_int;
            rEISData.eisOffset.y = region.y_int;
            rEISData.eisImgSize = region.s;
        }
        else
            return MFALSE;
    }
    else
    {
        rEISData.isON = false;
    }
    return MTRUE;
}

MVOID
N3DNode::
onFlush()
{
    MY_LOGD("+ extDep=%d", this->getExtThreadDependency());
    DepthMapRequestPtr pRequest;
    while( mPriorityQueue.deque(pRequest) )
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



