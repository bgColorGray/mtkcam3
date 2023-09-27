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
 * @file DepthMapEffectRequest.h
 * @brief Effect Request inside the DepthMapPipe
 */

#ifndef _MTK_CAMERA_FEATURE_PIPE_DEPTH_MAP_EFFECT_REQUEST_H_
#define _MTK_CAMERA_FEATURE_PIPE_DEPTH_MAP_EFFECT_REQUEST_H_

// Standard C header file
#include <list>
#include <unordered_map>
#include <mutex>
// Android system/core header file
#include <utils/RWLock.h>
// mtkcam custom header file

// mtkcam global header file
#include <mtkcam3/feature/effectHalBase/EffectRequest.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapEffectRequest.h>
// Module header file
#include <WaitQueue.h>
#include <CamNodeULog.h>
// Local header file
#include "DepthMapPipeTimer.h"
#include "DepthMapPipe_Common.h"
#include "./bufferPoolMgr/BaseBufferPoolMgr.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"


/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {
/*******************************************************************************
* Structure Define
********************************************************************************/

/**
 * @struct EffectRequestAttrs
 * @brief EffectRequest attributes
 */
struct EffectRequestAttrs
{
    MBOOL isEISOn                      = MFALSE;
    MBOOL needFEFM                     = MFALSE;
    DepthMapPipeOpState opState        = eSTATE_NORMAL;
    BufferPoolScenario bufferScenario  = eBUFFER_POOL_SCENARIO_PREVIEW;
};

struct InternalData
{
    struct InternalMetaSet {
        std::shared_ptr<IMetadata> pMeta_InApp       = nullptr;
        std::shared_ptr<IMetadata> pMeta_Main1_InHal = nullptr;
        std::shared_ptr<IMetadata> pMeta_Main2_InHal = nullptr;
        std::shared_ptr<IMetadata> pMeta_OutApp      = nullptr;
        std::shared_ptr<IMetadata> pMeta_OutHal      = nullptr;
        std::shared_ptr<IMetadata> pMeta_P1_InRet    = nullptr;
    };

    struct InternalImageSet {
        IImageBuffer* pInternal_Main1_YUVO = nullptr;
        IImageBuffer* pInternal_Main2_YUVO = nullptr;
        IImageBuffer* pInternal_Main2_RRZO = nullptr;
    };

    MBOOL               mbEnableMeta  = MFALSE;
    MBOOL               mbEnableImage = MFALSE;
    InternalMetaSet     mMetaSet;
    InternalImageSet    mImageSet;
};


/*******************************************************************************
* Class Definition
********************************************************************************/
class BaseBufferHandler;
class DepthMapFlowOption;
/**
 * @class DepthMapEffectRequest
 * @brief Effect request used in DepthMapNode/Pipe
 */

class DepthMapEffectRequest
        : public IDepthMapEffectRequest
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    DepthMapEffectRequest(
                        MUINT32 reqNo = 0,
                        PFN_IREQ_FINISH_CALLBACK_T callback = NULL,
                        MVOID* tag = NULL);

    virtual ~DepthMapEffectRequest();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IDepthMapEffectRequest Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    MVOID launchFinishCallback(ResultState state);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapEffectRequest Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
     /**
     * @brief init the effect request
     * @param [in] pHandler current buffer handler address
     * @param [in] reqAttrs request attributes
     * @return
     * - MTRUE indicates success.
     * - MFALSE indicates failure.
     */
    MBOOL init(
            sp<BaseBufferHandler> pHandler,
            const EffectRequestAttrs& reqAttrs);
    /**
     * @brief get BufferHandler
     * @return
     * - buffer handler pointer
     */
    sp<BaseBufferHandler> getBufferHandler() {return mpBufferHandler;}
    /**
     * @brief notify the node operation done
     * @param [in] nodeID DepthMapPipe node id
     * @return
     * - MTRUE indicates success.
     * - MFALSE indicates failure.
     */
    MBOOL handleProcessDone(DepthMapPipeNodeID nodeID);

    /**
     * @brief check the output buffer inside the
     *        request is ready or not
     * @return
     * - MTRUE indicates ready
     * - MFALSE indicates not ready
     */
    MBOOL checkAllOutputReady();
    /**
     * @brief set the buffer ready for use
     * @param [in] bufferID buffer id
     * @return
     * - MTRUE indicates success
     * - MFALSE indicates the buffer is not Request buffer.
     */
    MBOOL setOutputBufferReady(const DepthMapBufferID& bufferID);

    /**
     * @brief add input buffer user to map
     * map<DepthMapPipeNodeID, std::list<DepthMapBufferID>
     */
    MBOOL registerInputBufferUser(sp<DepthMapPipeOption> pPipeOption);

    /**
     * @brief remove input buffer user from map, and if map is empty
     *        launch callback to DepthNode to release buffer
     * @param [in] tag: a pointer to the user
     */
    MBOOL unRegisterInputBufferUser(MVOID *tag);

    /**
     * @brief copy meta into internal one
     */
    MBOOL copyInputDataIfNeed(sp<DepthMapPipeOption> pPipeOption);

    const EffectRequestAttrs& getRequestAttr() {return mReqAttrs;}
    /**
     * @brief check the request is QUEUED-DEPTH or not
     * @param [in] pPipeOption
     * @return
     * - MTRUE indicates the request is QUEUED-DEPTH.
     * - MFALSE indicates the request is STANDARD.
     */
    virtual MBOOL isQueuedDepthRequest(sp<DepthMapPipeOption> pPipeOption);

    /**
     * @brief check the request needs to skip depth calculation or not
     * @param [in] pPipeOption
     * @return
     * - MTRUE indicates the request will not calculate depth
     * - MFALSE indicates the request will calculate depth
     */
    virtual MBOOL isSkipDepth(sp<DepthMapPipeOption> pPipeOption);
    /**
     * @brief request enque order
     */
    MUINT64 getRequestOrder() {return miOrder;}
    /**
     * @brief check if need use internal meta
     */
    MBOOL isInternalMetaAvailable() {return mInternalData.mbEnableMeta;};
    /**
     * @brief check if need use internal meta
     */
    MBOOL isInternalImageAvailable() {return mInternalData.mbEnableImage;};

    /**
     * @brief ULog typeID
     */
    static NSCam::Utils::ULog::RequestTypeId getULogRequestTypeId() {
        return NSCam::Utils::ULog::REQ_DEPTH_REQUEST;
    }

    NSCam::Utils::ULog::RequestSerial getULogRequestSerial() const {
        return getRequestNo();
    }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapEffectRequest Private Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapEffectRequest Public Data Member
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    // timer to record the entire process
    DepthMapPipeTimer mTimer;
    // use for any temporay data
    MVOID* mpData = nullptr;
    //
    MUINT64 miOrder = 0;
    // N3D preview phase
    N3D_PVPHASE_ENUM mN3DPhase = eN3D_PVPHASE_MAIN1_PADDING;
    // Internal Meta Buffer
    InternalData mInternalData;
    MBOOL mbNeedUseInternalData = MFALSE;
    // decide GF input by tuning data
    MBOOL mbWMF_ON = MFALSE;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  DepthMapEffectRequest Private Data Member
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    // buffer handler
    sp<BaseBufferHandler> mpBufferHandler = nullptr;
    EffectRequestAttrs mReqAttrs;
    static MUINT64 SerialNum;
    // maintain which nodes will use the request buffer
    std::mutex mRequestBufMutex;
    std::unordered_map<DepthMapPipeNodeID,
                       std::list<DepthMapBufferID> > mRequestBufferRegistry;
    //
    friend class DepthMapPipe;
};

typedef sp<DepthMapEffectRequest> DepthMapRequestPtr;

/**
 * @brief RequestNo getter
 */
MUINT32 reqNoGetter(const DepthMapRequestPtr& pRequest);

/**
 * @brief Request order getter - invariant to reversed req id
 */
MUINT32 reqOrderGetter(const DepthMapRequestPtr& pRequest);

/**
 * @brief DepthRequest priority indexer for PriorityWaitQueue
 */
class DepthRequestOrderConverter
{
public:
    IWaitQueue::Index operator()(const DepthMapRequestPtr &request) const
    {
        return IWaitQueue::Index(getID(request), getPriority(request));
    }
    static unsigned getID(const DepthMapRequestPtr &request)
    {
        return request->getRequestOrder();
    }
    static unsigned getPriority(const DepthMapRequestPtr &request)
    {
        return request->getRequestOrder();
    }
};

}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam

#endif