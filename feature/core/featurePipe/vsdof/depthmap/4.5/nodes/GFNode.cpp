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
#include <../../util/vsdof_util.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
// Module header file

// Local header file
#include "GFNode.h"
#include "../DepthMapPipeUtils.h"
#include "../bokeh_buffer_packer.h"
// Logging header file
#define PIPE_CLASS_TAG "GFNode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_GF);

static const MINT64 tolerance = 30000000;   // 30ms
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
    // clear FD data vector
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
    GF_HAL_INIT_DATA gfInitData;
    if(!requireAlgoDataFromStaticInfo(gfInitData))
    {
        MY_LOGE("require data for init GF_HAL failed");
        cleanUp();
        return MFALSE;
    }
    else
    {
        mpGf_Hal = GF_HAL::createInstance(gfInitData);
        if(!mpGf_Hal)
        {
            MY_LOGE("Create GF_HAL fail.");
            cleanUp();
        }
    }

    mpFdReader = IFDContainer::createInstance(LOG_TAG, IFDContainer::eFDContainer_Opt_Read);
    if(!mpFdReader)
    {
        MY_LOGE("create FDContainer failed.");
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
        case DVP_TO_GF_DMW_N_DEPTH:
            mJobQueue.enque(pRequest);
            ret = MTRUE;
            break;
        case DLDEPTH_TO_GF_DEPTHMAP:
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
    DepthMapRequestPtr pRequest;
    DepthMapRequestPtr pRequest_MYS;
    //
    if( !waitAnyQueue() )
    {
        return MFALSE;
    }
    if( !mJobQueue.deque(pRequest) )
    {
        MY_LOGE("mJobQueue.deque() failed");
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
    pRequest->mTimer.showTotalSummary(pRequest->getRequestNo(), pRequest->getRequestAttr(), mpPipeOption);
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
    if(!mpGf_Hal->GFHALRun(sInData, sOutData))
    {
        #ifdef UNDER_DEVELOPMENT
        AEE_ASSERT("GFHalRun fail, reqID=%d", iReqIdx);
        #endif
        MY_LOGE("GFHalRun fail, reqID=%d", iReqIdx);
        return MFALSE;
    }
    pRequest->mTimer.stopGFALGO();
    // unlock FD data
    mpFdReader->queryUnlock(mvFdDatas);

    VSDOF_PRFTIME_LOG("[NormalPass]gf algo processing time(%d ms) reqID=%d",
                                pRequest->mTimer.getElapsedGFALGO(), iReqIdx);
    // Debug Ink
    MBOOL bRet = MTRUE;
    if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_SW_OPTIMIZED_DEPTH )
        bRet = onHandle3rdPartyFlowDone(pRequest);
    else
        bRet = onHandleTKFlowDone(pRequest);

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
    if(DepthPipeLoggingSetup::miDebugLog<2)
        return;

    MY_LOGD("Input GFParam: GF_HAL_IN_DATA");
    MY_LOGD("magicNumber=%d", inData.magicNumber);
    MY_LOGD("scenario=%d", inData.scenario);
    MY_LOGD("dofLevel=%d", inData.dofLevel);
    MY_LOGD("depthMap=%p", inData.depthMap);
    MY_LOGD("images.size()=%lu", inData.images.size());
    for(ssize_t idx=0;idx<inData.images.size();++idx)
    {
        MY_LOGD("images[%lu]=%p", idx, inData.images[idx]);
        if(inData.images[idx])
        {
            MY_LOGD("images[%lu], image size=%dx%d", idx,
                inData.images[idx]->getImgSize().w, inData.images[idx]->getImgSize().h);
        }
    }
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
GFNode::onHandleTKFlowDone(DepthMapRequestPtr pRequest)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    MUINT32 iReqIdx = pRequest->getRequestNo();
    VSDOF_LOGD("reqID=%d +", iReqIdx);
    DumpConfig config;
    DepthBufferInfo depthInfo;
    MBOOL bRet = MTRUE;
    // Debug Ink
    if(pRequest->isRequestBuffer(BID_GF_IN_DEBUG))
    {
        DepthMapBufferID debugBID;
        if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_DISPARITY)
        {
            debugBID = mpFlowOption->checkConnected(DLDEPTH_TO_GF_DEPTHMAP) ?
                        ( StereoSettingProvider::isSlantCameraModule()?
                            BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT :
                            BID_DLDEPTH_INTERNAL_DEPTHMAP ) :
                        ( pRequest->mbWMF_ON ?
                            BID_WMF_OUT_DMW :
                            BID_ASF_OUT_HF );
        }
        else if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_CONFIDENCE)
        {
            debugBID = StereoSettingProvider::isSlantCameraModule()?
                        BID_OCC_OUT_CFM_M_NONSLANT : BID_OCC_OUT_CFM_M;
        }
        else if(mpPipeOption->mDebugInk == eDEPTH_DEBUG_IMG_NOC)
        {
            debugBID = StereoSettingProvider::isSlantCameraModule()?
                        BID_OCC_OUT_NOC_M_NONSLANT : BID_OCC_OUT_NOC_M;
        }
        else
        {
            MY_LOGW("invalid debug ink");
            bRet = MFALSE;
        }
        //
        if ( bRet )
            bRet &= pBufferHandler->getEnquedSmartBuffer(
                                        getNodeId(),
                                        debugBID,
                                        depthInfo.mpDebugBuffer);
    }
    // handle output blur/depth map, use request or working buffer
    bRet &= pBufferHandler->getEnquedSmartBuffer(
                                    getNodeId(), BID_GF_INTERNAL_DMBG, depthInfo.mpDMBGBuffer);
    if(bRet)
    {
        depthInfo.mpDMBGBuffer->mImageBuffer->syncCache(eCACHECTRL_FLUSH);
        depthInfo.miReqIdx = iReqIdx;
    }
    else
    {
        MY_LOGE("Cannot find the internal DMBG or depthMap buffer!");
        return MFALSE;
    }
    // Case 1 : Queued flow or depth delay is set more than 0, stored the depthInfo
    if(pRequest->isQueuedDepthRequest(mpPipeOption))
    {
        if(miDepthDelay)
            mpDepthStorage->push_back(depthInfo);
        else
            mpDepthStorage->setStoredData(depthInfo);
        this->handleDump(GF_OUT_INTERNAL_DMBG, pRequest);
        // norify Queue flow done
        handleData(QUEUED_FLOW_DONE, pRequest);
    }
    // Case 2 : Not queued flow, directly output blur/depth map and set as latest data
    else
    {
        // handle blur map
        if(!_copyBufferIntoRequest(depthInfo.mpDMBGBuffer->mImageBuffer.get(),
                                    pRequest, BID_GF_OUT_DMBG))
        {
            AEE_ASSERT("[GFNode] Failed to copy BID_GF_OUT_DMBG");
            return MFALSE;
        }
        // handle debug buffer
        if(mpPipeOption->mDebugInk > eDEPTH_DEBUG_IMG_BLUR_MAP &&
            mpPipeOption->mDebugInk < eDEPTH_DEBUG_IMG_INVALID &&
            pRequest->isRequestBuffer(BID_GF_IN_DEBUG) &&
            !_copyBufferIntoRequest(depthInfo.mpDebugBuffer->mImageBuffer.get(),
                                    pRequest, BID_GF_IN_DEBUG))
        {
            MY_LOGW("[GFNode] Failed to copy BID_GF_IN_DEBUG");
        }
        pRequest->setOutputBufferReady(BID_GF_IN_DEBUG);
        // save result into storage
        mpDepthStorage->setLatestData(depthInfo);
        // notify DMBG done
        handleDataAndDump(GF_OUT_DMBG, pRequest, &config);
    }
    VSDOF_LOGD("reqID=%d -", iReqIdx);
    return MTRUE;
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

    IMetadata* pInAppMeta = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_APP);
    // config DOF level
    if(!tryGetMetadata<MINT32>(pInAppMeta, MTK_STEREO_FEATURE_DOF_LEVEL, inData.dofLevel))
    {
        MY_LOGE("reqID=%d Cannot find MTK_STEREO_FEATURE_DOF_LEVEL meta in AppMeta!", iReqIdx);
        return MFALSE;
    }
    // config crop ratio
    MFLOAT zoomRatio = 1;
    if(!tryGetMetadata<MFLOAT>(pInAppMeta, MTK_CONTROL_ZOOM_RATIO, zoomRatio)) {
        MY_LOGW("Cannot find MTK_CONTROL_ZOOM_RATIO meta!");
    } else {
        inData.previewCropRatio = 1 / zoomRatio;
    }

    // config touch info
    IMetadata::IEntry entry = pInAppMeta->entryFor(MTK_STEREO_FEATURE_TOUCH_POSITION);
    if( !entry.isEmpty() ) {
        inData.touchPosX = entry.itemAt(0, Type2Type<MINT32>());
        inData.touchPosY = entry.itemAt(1, Type2Type<MINT32>());
    }

    // InHalMeta
    IMetadata* pInHalMeta = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN1);
    if( !checkToDump(GF_OUT_INTERNAL_DMBG, pRequest) &&
        !checkToDump(GF_OUT_INTERNAL_DEPTH, pRequest) ) {
        inData.dumpHint = nullptr;
    } else {
        extract(&mDumpHint_Main1, pInHalMeta);
        inData.dumpHint = &mDumpHint_Main1;
    }

    // ndd_timestamp
    extract(&mDumpHint_Main1, pInHalMeta);
    inData.ndd_timestamp = mDumpHint_Main1.UniqueKey;

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
    MINT64 timestamp = 0;
    if(!tryGetMetadata<MINT64>(pInHalMeta, MTK_P1NODE_FRAME_START_TIMESTAMP, timestamp))
    {
        MY_LOGW("reqID=%d Cannot find MTK_P1NODE_FRAME_START_TIMESTAMP meta in HalMeta!", iReqIdx);
    }

    // config convergence offset
    IMetadata* pOutHalMeta = pRequest->getBufferHandler()->requestMetadata(getNodeId(), BID_META_OUT_HAL);
    if(!tryGetMetadata<MFLOAT>(pOutHalMeta, MTK_CONVERGENCE_DEPTH_OFFSET, inData.convOffset))
    {
        MY_LOGE("reqID=%d Cannot find MTK_CONVERGENCE_DEPTH_OFFSET meta in outHalMeta!", iReqIdx);
        return MFALSE;
    }

    // config focalLensFactor
    IMetadata* pOutAppMeta = pRequest->getBufferHandler()->requestMetadata(getNodeId(), BID_META_OUT_APP);
    if(!tryGetMetadata<MFLOAT>(pOutAppMeta, MTK_STEREO_FEATURE_RESULT_DISTANCE, inData.focalLensFactor))
    {
        MY_LOGE("reqID=%d Cannot find MTK_STEREO_FEATURE_RESULT_DISTANCE!", iReqIdx);
    }

    // config FD info
    if(!mpFdReader || timestamp != 0)
    {
        const MINT64 startTimestamp = timestamp - tolerance;
        const MINT64 endTimestamp   = timestamp;
        mvFdDatas = mpFdReader->queryLock(startTimestamp, endTimestamp);
        if(!mvFdDatas.empty())
        {
            inData.fdData = &(mvFdDatas.back()->facedata);
        }
    }
    return MTRUE;
}

//************************************************************************
//
//************************************************************************
MBOOL
GFNode::
requireAlgoDataFromStaticInfo(
    GF_HAL_INIT_DATA& initData
)
{
    initData.eScenario = mpPipeOption->mStereoScenario;

    initData.outputDepthmap = StereoSettingProvider::isMTKDepthmapMode();

    auto openId = StereoSettingProvider::getLogicalDeviceID();
    sp<IMetadataProvider> pMetadataProvider = NSMetadataProviderManager::valueForByDeviceId(openId);
    if(pMetadataProvider == nullptr)
    {
        MY_LOGE("Cannot get metadata provider for logical device %d", openId);
        return MFALSE;
    }
    const IMetadata& staticMetadata = pMetadataProvider->getMtkStaticCharacteristics();

    if(!IMetadata::getEntry<float>(&staticMetadata, MTK_LENS_INFO_AVAILABLE_APERTURES, initData.aperture))
    {
        MY_LOGE("Cannot get static info: MTK_LENS_INFO_AVAILABLE_APERTURES");
        return MFALSE;
    }

    if(!IMetadata::getEntry<float>(&staticMetadata, MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, initData.focalLenght))
    {
        MY_LOGE("Cannot get static info: MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS");
        return MFALSE;
    }

    if(!IMetadata::getEntry<MFLOAT>(&staticMetadata, MTK_SENSOR_INFO_PHYSICAL_SIZE, initData.sensorPhysicalWidth, 0))
    {
        MY_LOGE("Cannot get static info: MTK_SENSOR_INFO_PHYSICAL_SIZE (h) fail.");
        return MFALSE;
    }

    if(!IMetadata::getEntry<MFLOAT>(&staticMetadata, MTK_SENSOR_INFO_PHYSICAL_SIZE, initData.sensorPhysicalHeight, 1))
    {
        MY_LOGE("Cannot get static info: MTK_SENSOR_INFO_PHYSICAL_SIZE (v) fail.");
        return MFALSE;
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
    MINT32 iReqIdx = pRequest->getRequestNo();
    VSDOF_LOGD("reqID=%d", pRequest->getRequestNo());

    // require input meta
    if(!requireInputMetaFromRequest(pRequest, inData))
        return MFALSE;
    //
    MBOOL bRet = MTRUE;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // config MYS/DMW buffer
    IImageBuffer *pImgMYS = nullptr, *pImgDepth = nullptr, *pImgCFM;
    DepthMapBufferID inDepthBID = pRequest->getRequestAttr().opState == eSTATE_RECORD ?
                                ( StereoSettingProvider::isSlantCameraModule() ?
                                    BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT :
                                    BID_DLDEPTH_INTERNAL_DEPTHMAP ) :
                                ( pRequest->mbWMF_ON ?
                                    BID_WMF_OUT_DMW :
                                    BID_ASF_OUT_HF );
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), inDepthBID, pImgDepth);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(),
                                        StereoSettingProvider::isSlantCameraModule()?
                                            BID_OCC_OUT_CFM_M_NONSLANT:
                                            BID_OCC_OUT_CFM_M,
                                        pImgCFM);
    bRet &= pBufferHandler->getEnqueBuffer(getNodeId(), BID_P2A_OUT_MY_S, pImgMYS);
    if(bRet)
    {
        inData.hasFEFM = pRequest->getRequestAttr().needFEFM;
        inData.isCapture = (pRequest->getRequestAttr().opState == eSTATE_CAPTURE) ? true : false;
        // MY_S
        inData.images.push_back(pImgMYS);
        // Confidence
        inData.confidenceMap = pImgCFM;
        // In queue flow, the depth map (ASF) may be nullptr.
        if(pImgDepth == nullptr)
        {
            inData.depthMap = nullptr;
            if(!pRequest->isQueuedDepthRequest(mpPipeOption))
            {
                MY_LOGE("Input Depth buffer is null, please check!");
                return MFALSE;
            }
        }
        else
            inData.depthMap = pImgDepth;
    }
    else
    {
        MY_LOGE("Cannot get MYS or DMW/DLDEPTH buffers!!");
        return MFALSE;
    }
    // output
    if(mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_VSDOF)
    {
        outData.dmbg = pBufferHandler->requestBuffer(getNodeId(), BID_GF_INTERNAL_DMBG);
    }
    else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_SW_OPTIMIZED_DEPTH )
    {
        outData.depthMap = pBufferHandler->requestBuffer(getNodeId(), BID_GF_INTERNAL_DEPTHMAP);
    }
    else
    {
        MY_LOGE("undefined FeatureMode for GF output depth");
        return MFALSE;
    }
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
