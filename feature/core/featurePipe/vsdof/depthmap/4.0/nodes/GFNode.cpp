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
#include <DpBlitStream.h>
#include <vsdof/util/vsdof_util.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
// Module header file

// Local header file
#include "GFNode.h"
#include "../DepthMapPipeUtils.h"
#include "../bokeh_buffer_packer.h"
// Logging header file
#define PIPE_CLASS_TAG "GFNode"
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_GF);
/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam{
namespace NSCamFeature{
namespace NSFeaturePipe_DepthMap{

using namespace VSDOF::util;

//************************************************************************
//
//************************************************************************
GFNode::
GFNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: DepthMapPipeNode(name, nodeID, config)
{
    mbIsWaitDepth = ::property_get_bool("vendor.depthmap.pipe.enableWaitDepth", 0);
    this->addWaitQueue(&mJobQueue);
}

//************************************************************************
//
//************************************************************************
GFNode::
~GFNode()
{
    MY_LOGD("[Destructor]");
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
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
GFNode::
onUninit()
{
    VSDOF_INIT_LOG("+");
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MVOID
GFNode::
cleanUp()
{
    MY_LOGD("+");
    mJobQueue.clear();
    // release gf_hal
    mpGf_Hal = nullptr;
    MY_LOGD("-");
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
onThreadStart()
{
    CAM_ULOGM_TAGLIFE("GFNode::onThreadStart");
    VSDOF_INIT_LOG("+");
    // create gf_hal for PV/VR
    MBOOL bIsMTKDepthMap = StereoSettingProvider::isMTKDepthmapMode();
    mpGf_Hal = GF_HAL::createInstance(
                            eSTEREO_SCENARIO_PREVIEW,
                            bIsMTKDepthMap
                            );
    if(!mpGf_Hal)
    {
        MY_LOGE("Create GF_HAL fail.");
        cleanUp();
    }

    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
onThreadStop()
{
    VSDOF_INIT_LOG("+");
    cleanUp();
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
onData(DataID data, DepthMapRequestPtr& pRequest)
{
    VSDOF_LOGD("reqID=%d + data=%d", pRequest->getRequestNo(), data);
    MBOOL ret = MFALSE;
    //
    switch(data)
    {
        case DPE_TO_GF_DMW_N_DEPTH:
            mJobQueue.enque(pRequest);
            ret = MTRUE;
            break;
        default:
            ret = MFALSE;
            break;
    }
    //
    VSDOF_LOGD("-");
    return ret;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
onThreadLoop()
{
    DepthMapRequestPtr pRequest;;
    //
    if( !waitAnyQueue() )
    {
        return MFALSE;
    }
    if( !mJobQueue.deque(pRequest) )
    {
    return MFALSE;
    }
    CAM_ULOGM_TAGLIFE("GFNode::onThreadLoop");

    VSDOF_LOGD("reqID=%d threadLoop", pRequest->getRequestNo());
    pRequest->mTimer.startGF();
    MBOOL bRet = MTRUE;
    {
        if(!executeAlgo(pRequest))
        {
            MY_LOGE("reqID=%d, GF executeAlgo failed.", pRequest->getRequestNo());
            handleData(ERROR_OCCUR_NOTIFY, pRequest);
            bRet = MFALSE;
        }
    }
    // launch onProcessDone
    pRequest->getBufferHandler()->onProcessDone(getNodeId());
    // timer
    pRequest->mTimer.stopGF();
    pRequest->mTimer.showTotalSummary(pRequest->getRequestNo(), pRequest->getRequestAttr());
    return bRet;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
executeAlgo(
    DepthMapRequestPtr pRequest)
{
    MBOOL ret = MFALSE;
    VSDOF_LOGD("+ reqId=%d", pRequest->getRequestNo());
    // Normal pass
    if(!runNormalPass(pRequest))
    {
        MY_LOGE("GF NormalPass failed!");
        goto lbExit;
    }
    // handle
    handleData(QUEUED_FLOW_DONE, pRequest);
    ret = MTRUE;
    VSDOF_LOGD("- reqId=%d", pRequest->getRequestNo());
lbExit:
    return ret;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
runNormalPass(
    DepthMapRequestPtr pRequest)
{
    CAM_ULOGM_TAGLIFE("GFNode::runNormalPass");

    GF_HAL_IN_DATA sInData;
    GF_HAL_OUT_DATA sOutData;
    MINT32 iReqIdx = pRequest->getRequestNo();

    if(!requireAlgoDataFromRequest(pRequest, sInData, sOutData))
    {
        MY_LOGE("get algo buffer fail, reqID=%d", iReqIdx);
        return MFALSE;
    }
    //
    debugGFParams(sInData, sOutData);
    //
    pRequest->mTimer.startGFALGO();
    //
    if(!mpGf_Hal->GFHALRun(sInData, sOutData))
    {
        #ifdef UNDER_DEVELOPMENT
        AEE_ASSERT("GFHalRun fail, reqID=%d", iReqIdx);
        #endif
        MY_LOGE("GFHalRun fail, reqID=%d", iReqIdx);
        return MFALSE;
    }
    //
    pRequest->mTimer.stopGFALGO();

    VSDOF_PRFTIME_LOG("[NormalPass]gf algo processing time(%d ms) reqID=%d",
                                pRequest->mTimer.getElapsedGFALGO(), iReqIdx);
    MBOOL bRet = MTRUE;
    if(pRequest->isQueuedDepthRequest(mpPipeOption))
    {
        DepthBufferInfo depthInfo;
        // get internal dmbg buffer
        MBOOL bRet = pRequest->getBufferHandler()->getEnquedSmartBuffer(
                                    getNodeId(), BID_GF_INTERNAL_DMBG, depthInfo.mpDMBGBuffer);
        if(bRet)
        {
            depthInfo.mpDMBGBuffer->mImageBuffer->syncCache(eCACHECTRL_FLUSH);
            depthInfo.miReqIdx = iReqIdx;
            if(mbIsWaitDepth)
                mpDepthStorage->push_back(depthInfo);
            else
                mpDepthStorage->setStoredData(depthInfo);
            this->handleDump(GF_OUT_INTERNAL_DMBG, pRequest);
        }
        else
            MY_LOGE("Cannot find the internal DMBG or depthMap buffer!");
    }
    else
    {
        // flush DMBG
        IImageBuffer* pImgBuf = nullptr;
        if(pRequest->getBufferHandler()->getEnqueBuffer(getNodeId(), BID_GF_OUT_DMBG, pImgBuf))
        {
            pImgBuf->syncCache(eCACHECTRL_FLUSH);
        }
        // mark DMBG ready
        pRequest->setOutputBufferReady(BID_GF_OUT_DMBG);
        // notify DMBG done
        handleDataAndDump(GF_OUT_DMBG, pRequest);
    }

    return bRet;
}
//************************************************************************
//
//************************************************************************
MVOID
GFNode::
debugGFParams(
    const GF_HAL_IN_DATA& inData,
    const GF_HAL_OUT_DATA& outData
)
{
    if(DepthPipeLoggingSetup::mbDebugLog<2)
        return;

    MY_LOGD("Input GFParam: GF_HAL_IN_DATA");
    MY_LOGD("magicNumber=%d", inData.magicNumber);
    MY_LOGD("scenario=%d", inData.scenario);
    MY_LOGD("dofLevel=%d", inData.dofLevel);
    MY_LOGD("depthMap=%p", inData.depthMap);
    MY_LOGD("images.size()=%lu", inData.images.size());
    for(ssize_t idx=0;idx<inData.images.size();++idx)
    {
        MY_LOGD("images[%zd]=%p", idx, inData.images[idx]);
        if(inData.images[idx])
        {
            MY_LOGD("images[%zd], image size=%dx%d", idx,
                inData.images[idx]->getImgSize().w, inData.images[idx]->getImgSize().h);
        }
    }
    MY_LOGD("confidenceMap size=%dx%d", inData.confidenceMap->getImgSize().w,
                                        inData.confidenceMap->getImgSize().h);
    if(outData.dmbg != nullptr)
        MY_LOGD("Output dmbg size =%dx%d", outData.dmbg->getImgSize().w
                                         , outData.dmbg->getImgSize().h);
    else
        MY_LOGD("Output dmbg null!");

    if(outData.depthMap != nullptr)
        MY_LOGD("Output depthMap size=%dx%d", outData.depthMap->getImgSize().w
                                         , outData.depthMap->getImgSize().h);
    else
        MY_LOGD("Output depthMap null!");

}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
requireInputMetaFromRequest(
    DepthMapRequestPtr pRequest,
    GF_HAL_IN_DATA& inData
)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    MINT32 iReqIdx = pRequest->getRequestNo();
    // config request number
    inData.requestNumber = iReqIdx;
    // InAppMeta
    DepthMapBufferID inAppMetaID = mapQueuedBufferID(pRequest, mpPipeOption, BID_META_IN_APP);
    IMetadata* pInAppMeta = pBufferHandler->requestMetadata(getNodeId(), inAppMetaID);
    // config DOF level
    if(!tryGetMetadata<MINT32>(pInAppMeta, MTK_STEREO_FEATURE_DOF_LEVEL, inData.dofLevel))
    {
        MY_LOGE("reqID=%d Cannot find MTK_STEREO_FEATURE_DOF_LEVEL meta in AppMeta!", iReqIdx);
        return MFALSE;
    }

    // config touch info
    IMetadata::IEntry entry = pInAppMeta->entryFor(MTK_STEREO_FEATURE_TOUCH_POSITION);
    if( !entry.isEmpty() ) {
        inData.touchPosX = entry.itemAt(0, Type2Type<MINT32>());
        inData.touchPosY = entry.itemAt(1, Type2Type<MINT32>());
    }

    // InHalMeta
    DepthMapBufferID inHalMetaID = mapQueuedBufferID(pRequest, mpPipeOption, BID_META_IN_HAL_MAIN1);
    IMetadata* pInHalMeta = pBufferHandler->requestMetadata(getNodeId(), inHalMetaID);
    if(!checkToDump(GF_OUT_INTERNAL_DMBG, pRequest)) {
        inData.dumpHint = nullptr;
    } else {
        extract(&mDumpHint_Main1, pInHalMeta);
        inData.dumpHint = &mDumpHint_Main1;
    }

    MINT32 iSensorMode;
    // config scenario
    if(!tryGetMetadata<MINT32>(pInHalMeta, MTK_P1NODE_SENSOR_MODE, iSensorMode))
    {
        MY_LOGE("reqID=%d Cannot find MTK_P1NODE_SENSOR_MODE meta in HalMeta!", iReqIdx);
        return MFALSE;
    }
    inData.scenario = getStereoSenario(iSensorMode);

    // config magic number
    if(!tryGetMetadata<MINT32>(pInHalMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, inData.magicNumber))
    {
        MY_LOGE("reqID=%d Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta in HalMeta!", iReqIdx);
        return MFALSE;
    }

    DepthMapBufferID outHalMetaID = mapQueuedBufferID(pRequest, mpPipeOption, BID_META_OUT_HAL);
    IMetadata* pOutHalMeta = pRequest->getBufferHandler()->requestMetadata(getNodeId(), outHalMetaID);
    // config convergence offset
    if(!tryGetMetadata<MFLOAT>(pOutHalMeta, MTK_CONVERGENCE_DEPTH_OFFSET, inData.convOffset))
    {
        MY_LOGE("reqID=%d Cannot find MTK_CONVERGENCE_DEPTH_OFFSET meta in outHalMeta!", iReqIdx);
        return MFALSE;
    }

    // config focalLensFactor
    DepthMapBufferID outAppBID = mapQueuedBufferID(pRequest, mpPipeOption, BID_META_OUT_APP);
    IMetadata* pOutAppMeta = pRequest->getBufferHandler()->requestMetadata(getNodeId(), outAppBID);
    if(!tryGetMetadata<MFLOAT>(pOutAppMeta, MTK_STEREO_FEATURE_RESULT_DISTANCE, inData.focalLensFactor))
    {
        MY_LOGW("reqID=%d, Failed to get MTK_STEREO_FEATURE_RESULT_DISTANCE!", pRequest->getRequestNo());
    }

    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
requireAlgoDataFromRequest(
    DepthMapRequestPtr pRequest,
    GF_HAL_IN_DATA& inData,
    GF_HAL_OUT_DATA& outData,
    bool bIsCapture)
{
    IImageBuffer *pImgBuf_DMBG = nullptr, *pImgBuf_DepthMap = nullptr;

    MINT32 iReqIdx = pRequest->getRequestNo();
    VSDOF_LOGD("reqID=%d", pRequest->getRequestNo());

    // require input meta
    if(!requireInputMetaFromRequest(pRequest, inData))
        return MFALSE;
    //
    MBOOL bRet = MTRUE;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // config MYS/DMW buffer
    IImageBuffer *pImgMYS = nullptr, *pImgDMW = nullptr;
    IImageBuffer *pImgCFM = nullptr;
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_MY_S, pImgMYS);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_WMF_OUT_DMW, pImgDMW);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_OCC_OUT_CFM_M, pImgCFM);
    if(bRet)
    {
        inData.hasFEFM = pRequest->getRequestAttr().needFEFM;
        inData.isCapture = (pRequest->getRequestAttr().opState == eSTATE_CAPTURE) ? true : false;
        // MY_S
        inData.images.push_back(pImgMYS);
        // confidence
        inData.confidenceMap = pImgCFM;
        // DMW : In queue flow, the depth map may be nullptr.
        if(pImgDMW == nullptr)
        {
            inData.depthMap = nullptr;
            if(!pRequest->isQueuedDepthRequest(mpPipeOption))
            {
                MY_LOGE("DMW buffer is null, please check!");
                return MFALSE;
            }
        }
        else
            inData.depthMap = pImgDMW;
    }
    else
    {
        MY_LOGE("Cannot get MYS/DMW buffers!!");
        return MFALSE;
    }
    // output
    if(pRequest->isQueuedDepthRequest(mpPipeOption))
        pImgBuf_DMBG = pBufferHandler->requestBuffer(getNodeId(), BID_GF_INTERNAL_DMBG);
    else
        pImgBuf_DMBG = pBufferHandler->requestBuffer(getNodeId(), BID_GF_OUT_DMBG);
    outData.dmbg = pImgBuf_DMBG;
    return MTRUE;
}

MVOID
GFNode::
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

};
};
};
