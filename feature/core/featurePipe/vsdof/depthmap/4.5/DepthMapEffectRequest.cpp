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
#include <utils/RefBase.h>
// mtkcam custom header file

// mtkcam global header file
#include <mtkcam/def/common.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
// Module header file

// Local header file
#include "DepthMapPipeUtils.h"
#include "DepthMapEffectRequest.h"
#include "DepthMapPipe_Common.h"
#include "DepthMapPipeNode.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);



namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

/*******************************************************************************
* Global Define
********************************************************************************/
/**
 * @brief RequestNo getter
 */
MUINT32 reqNoGetter(const DepthMapRequestPtr& pRequest)
{
    return pRequest->getRequestNo();
}

MUINT32 reqOrderGetter(const DepthMapRequestPtr& pRequest)
{
    return pRequest->getRequestOrder();
}


MUINT64 DepthMapEffectRequest::SerialNum = 0;
/*******************************************************************************
* External Function
********************************************************************************/

/*******************************************************************************
* Enum Define
********************************************************************************/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sp<IDepthMapEffectRequest>
IDepthMapEffectRequest::
createInstance(MUINT32 reqID, PFN_IREQ_FINISH_CALLBACK_T callback, MVOID* tag)
{
    return new DepthMapEffectRequest(reqID, callback, tag);
}

IDepthMapEffectRequest::
IDepthMapEffectRequest(MUINT32 reqID, PFN_IREQ_FINISH_CALLBACK_T callback, MVOID* tag)
: IDualFeatureRequest(reqID, callback, tag)
{

}

DepthMapEffectRequest::
DepthMapEffectRequest(MUINT32 reqID, PFN_IREQ_FINISH_CALLBACK_T callback, MVOID* tag)
: IDepthMapEffectRequest(reqID, callback, tag)
{
}

DepthMapEffectRequest::
~DepthMapEffectRequest()
{
    mpBufferHandler = NULL;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IDepthMapEffectRequest Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MVOID
DepthMapEffectRequest::
launchFinishCallback(ResultState state)
{
    VSDOF_LOGD("reqID=%d, request complete, state=%s",
                getRequestNo(), ResultState2Name(state));
    IDepthMapEffectRequest::launchFinishCallback(state);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapEffectRequest Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MBOOL
DepthMapEffectRequest::
init(
    sp<BaseBufferHandler> pHandler,
    const EffectRequestAttrs& reqAttrs
)
{
    mpBufferHandler = pHandler;
    mReqAttrs = reqAttrs;
    miOrder = SerialNum++;
    // config request to the handler
    MBOOL bRet = pHandler->configRequest(this);
    return bRet;
}

MBOOL
DepthMapEffectRequest::
handleProcessDone(eDepthMapPipeNodeID nodeID)
{
    MBOOL bRet = mpBufferHandler->onProcessDone(nodeID);
    return bRet;
}

MBOOL
DepthMapEffectRequest::
checkAllOutputReady()
{
    RWLock::AutoRLock _l(mFrameLock[eBUFFER_IOTYPE_OUTPUT]);
    size_t outBufSize = this->vOutputFrameInfo.size();
    // make sure all output frame are ready
    for(size_t index=0;index<outBufSize;++index)
    {
        if(!this->vOutputFrameInfo[index]->isFrameBufferReady())
        {
            VSDOF_LOGD("req_id=%d Data not ready!! buffer key=%d %s",
                this->getRequestNo(), (DepthMapBufferID)this->vOutputFrameInfo.keyAt(index),
                DepthMapPipeNode::onDumpBIDToName(this->vOutputFrameInfo.keyAt(index)));
            return MFALSE;
        }
    }
    VSDOF_LOGD("req_id=%d Data all ready!!", getRequestNo());
    return MTRUE;
}

MBOOL
DepthMapEffectRequest::
setOutputBufferReady(const DepthMapBufferID& bufferID)
{
    RWLock::AutoWLock _l(mFrameLock[eBUFFER_IOTYPE_OUTPUT]);
    ssize_t index = this->vOutputFrameInfo.indexOfKey(bufferID);
    if(index >= 0)
    {
        sp<EffectFrameInfo> pFrame = this->vOutputFrameInfo.valueAt(index);
        pFrame->setFrameReady(true);
        return MTRUE;
    }
    return MFALSE;
}

MBOOL DepthMapEffectRequest::registerInputBufferUser(sp<DepthMapPipeOption> pPipeOption) {
    std::list<DepthMapBufferID> bufferlist;
    auto &bufMap = this->mRequestBufferRegistry;
    InternalDepthMapBufferID bufID_main1 = BID_INVALID;
    InternalDepthMapBufferID bufID_main2 = BID_INVALID;
    // config request buffer map by input type
    if (pPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2) {
        // PBID_IN_YUV1
        bufID_main1 = BID_P2A_IN_YUV1;
        // PBID_IN_YUV2 (only need when FEFM is on)
        if (getRequestAttr().needFEFM) {
            bufID_main2 = BID_P2A_IN_YUV2;
        }
    } else if (pPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2) {
        // PBID_IN_RSRAW1
        bufID_main1 = BID_P2A_IN_RSRAW1;
        // PBID_IN_RSRAW2 (only need when FEFM is on)
        if (getRequestAttr().needFEFM) {
            bufID_main2 = BID_P2A_IN_RSRAW2;
        }
    } else {
        MY_LOGE("Something went wrong, input type is not supported");
        return MFALSE;
    }
    // P2A
    bufferlist.emplace_back(bufID_main1);
    bufMap.emplace(eDPETHMAP_PIPE_NODEID_P2A, bufferlist);
    bufferlist.clear();
    // P2ABayer
    if (bufID_main2 != BID_INVALID) {
        bufferlist.emplace_back(bufID_main2);
        bufMap.emplace(eDPETHMAP_PIPE_NODEID_P2ABAYER, bufferlist);
        bufferlist.clear();
    }
    // N3D
    if (!this->isSkipDepth(pPipeOption)) {
        bufferlist.emplace_back(BID_P1_OUT_RECT_IN1);
        bufferlist.emplace_back(BID_P1_OUT_RECT_IN2);
        // WPE (for platforms which have WPE)
        if (hasWPEHw()) {
            bufMap.emplace(eDPETHMAP_PIPE_NODEID_WPE, bufferlist);
        }
        bufMap.emplace(eDPETHMAP_PIPE_NODEID_N3D, bufferlist);
    }

    MY_LOGD("Input buffer map size:%zu", bufMap.size());
    bufferlist.clear();
    return MTRUE;
}

MBOOL DepthMapEffectRequest::unRegisterInputBufferUser(MVOID *tag) {
    DepthMapPipeNode *pNode = static_cast<DepthMapPipeNode*>(tag);
    const auto &nodeID = pNode->getNodeId();
    {
        const std::lock_guard<std::mutex> lock(this->mRequestBufMutex);
        auto &bufMap = this->mRequestBufferRegistry;
        auto &&found = bufMap.find(nodeID);
        if (found != bufMap.end()) {
            bufMap.erase(found);
            sp<DepthMapEffectRequest> pRequest = this;
            MY_LOGD("reqID=%d, nodeID=%d releases buffer", pRequest->getRequestNo(), nodeID);
            // handle data to depthmappipe to launch release callback flow
            // when no other nodes is using request buffer anymore
            if (bufMap.empty()) {
                pNode->handleData(DEPTHPIPE_BUF_RELEASED, pRequest);
                MY_LOGD("NodeID=%d handles data to DepthPipe", nodeID);
            }
        } else {
            MY_LOGE("nodeID=%d did not register to input buffer map", nodeID);
            return MFALSE;
        }
    }
    return MTRUE;
}

MBOOL
DepthMapEffectRequest::
copyInputDataIfNeed(sp<DepthMapPipeOption> pPipeOption)
{
    /**
     * To prevent from using released data, ensure critical flow always use external meta
     * and the others use internal one.
     *      The critical flow is :
     *          - QueuedDepth mode : Clean YUV flow
     *          - Standard mode : Depth flow
     */
    if ( getRequestAttr().opState != eSTATE_STANDALONE )
    {
        mInternalData.mbEnableMeta  = MTRUE;
    }
    VSDOF_LOGD("reqID=%u, Copy input data, enableMeta=%d",
                getRequestNo(), mInternalData.mbEnableMeta);
    sp<BaseBufferHandler> pBufferHandler = this->getBufferHandler();
    // copy MetaSet
    if ( mInternalData.mbEnableMeta )
    {
        IMetadata* pMeta_InApp       = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_IN_APP      , USE_EXTERNAL);
        IMetadata* pMeta_Main1_InHal = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_IN_HAL_MAIN1, USE_EXTERNAL);
        IMetadata* pMeta_Main2_InHal = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_IN_HAL_MAIN2, USE_EXTERNAL);
        IMetadata* pMeta_OutApp      = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_OUT_APP     , USE_EXTERNAL);
        IMetadata* pMeta_OutHal      = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_OUT_HAL     , USE_EXTERNAL);
        IMetadata* pMeta_P1_InRet    = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_IN_P1_RETURN, USE_EXTERNAL);
        mInternalData.mMetaSet.pMeta_InApp       = std::make_shared<IMetadata>(*pMeta_InApp);
        mInternalData.mMetaSet.pMeta_Main1_InHal = std::make_shared<IMetadata>(*pMeta_Main1_InHal);
        mInternalData.mMetaSet.pMeta_Main2_InHal = std::make_shared<IMetadata>(*pMeta_Main2_InHal);
        mInternalData.mMetaSet.pMeta_OutApp      = std::make_shared<IMetadata>(*pMeta_OutApp);
        mInternalData.mMetaSet.pMeta_OutHal      = std::make_shared<IMetadata>(*pMeta_OutHal);
        mInternalData.mMetaSet.pMeta_P1_InRet    = std::make_shared<IMetadata>(*pMeta_P1_InRet);
        // check copy result
        if(     mInternalData.mMetaSet.pMeta_InApp       == nullptr
            ||  mInternalData.mMetaSet.pMeta_Main1_InHal == nullptr
            ||  mInternalData.mMetaSet.pMeta_Main2_InHal == nullptr
            ||  mInternalData.mMetaSet.pMeta_OutApp      == nullptr
            ||  mInternalData.mMetaSet.pMeta_OutHal      == nullptr
            ||  mInternalData.mMetaSet.pMeta_P1_InRet    == nullptr)
        {
            MY_LOGE("Copy metadata Failed");
            return MFALSE;
        }
    }
    return MTRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapEffectRequest Private Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MBOOL
DepthMapEffectRequest::
isQueuedDepthRequest(sp<DepthMapPipeOption> pPipeOption)
{
    if(mReqAttrs.opState != eSTATE_CAPTURE && pPipeOption->mFlowType == eDEPTH_FLOW_TYPE_QUEUED_DEPTH)
        return MTRUE;
    else
        return MFALSE;
}

MBOOL
DepthMapEffectRequest::
isSkipDepth(sp<DepthMapPipeOption> pPipeOption)
{
    MINT32 reqNo = this->getRequestNo() > 0 ?
                    this->getRequestNo() - 1 : this->getRequestNo();
    if(pPipeOption->mbDepthGenControl
        && (reqNo % (pPipeOption->miDepthFrameGap) != 0 ))
        return MTRUE;
    else
        return MFALSE;
}
}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam