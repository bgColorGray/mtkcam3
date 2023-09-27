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
 * @file NodeBufferPoolMgr_VSDOF.cpp
 * @brief BufferPoolMgr for VSDOF
*/

// Standard C header file
#include <future>
#include <thread>
// Android system/core header file
#include <ui/gralloc_extra.h>
// mtkcam custom header file

// mtkcam global header file
#include <camera_v4l2_dpe.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/def/IPostProcDef.h>
// Module header file
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include "NNRotate.h"
#include "MaskGen.h"
// Local header file
#include "NodeBufferPoolMgr_VSDOF.h"
#include "NodeBufferHandler.h"
#include "../DepthMapPipe_Common.h"
#include "../DepthMapPipeNode.h"
#include "../DepthMapPipeUtils.h"
#include "../StageExecutionTime.h"
#include "./bufferSize/NodeBufferSizeMgr.h"
// Logging header file
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "NodeBufferPoolMgr_VSDOF"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace NSCam::NSIoPipe;
using NSCam::NSIoPipe::NSPostProc::INormalStream;

#define SUPPORT_RECORD 1
#define SUPPORT_CAPTURE 0

#define MASK_DEFAULT_VALUE 255

/*******************************************************************************
* Global Define
********************************************************************************/
MUINT32 queryStrideInPixels(MUINT32 fmt, MUINT32 i, MUINT32 width)
{
    MUINT32 pixel;
    pixel = Utils::Format::queryPlaneWidthInPixels(fmt, i, width) * Utils::Format::queryPlaneBitsPerPixel(fmt, i) / 8;
    return pixel;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferPoolMgr_VSDOF class - Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferPoolMgr_VSDOF::
NodeBufferPoolMgr_VSDOF(
    PipeNodeBitSet& nodeBitSet,
    sp<DepthMapPipeSetting> pPipeSetting,
    sp<DepthMapPipeOption> pPipeOption
)
: mNodeBitSet(nodeBitSet)
, mpPipeOption(pPipeOption)
, mpPipeSetting(pPipeSetting)
{
    // decide size mgr
    mpBufferSizeMgr = new NodeBufferSizeMgr(pPipeOption);
    MBOOL bRet = this->initializeBufferPool();

    if(!bRet)
    {
        MY_LOGE("Failed to initialize buffer pool set! Cannot continue..");
        uninit();
        return;
    }

    this->buildImageBufferPoolMap();
    this->buildBufScenarioToTypeMap();

}

NodeBufferPoolMgr_VSDOF::
~NodeBufferPoolMgr_VSDOF()
{
    MY_LOGD("[Destructor] +");
    uninit();
    MY_LOGD("[Destructor] -");
}


MBOOL
NodeBufferPoolMgr_VSDOF::
uninit()
{
    // destroy buffer pools
    FatImageBufferPool::destroy(mpRectInBufPool_Main1);
    FatImageBufferPool::destroy(mpRectInBufPool_Main1_CAP);
    if (mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        FatImageBufferPool::destroy(mpInternalMain1YUVObufPool);
        FatImageBufferPool::destroy(mpInternalMain2YUVObufPool);
    }
    FatImageBufferPool::destroy(mpRectInBufPool_Main2);
    GraphicBufferPool::destroy(mpRectInGraBufPool_Main2);
    #if(SUPPORT_CAPTURE == 1)
    FatImageBufferPool::destroy(mpRectInBufPool_Main2_CAP);
    #endif
    FatImageBufferPool::destroy(mpFEOB_BufPool);
    FatImageBufferPool::destroy(mpFEOC_BufPool);
    FatImageBufferPool::destroy(mpFMOB_BufPool);
    FatImageBufferPool::destroy(mpFMOC_BufPool);
    FatImageBufferPool::destroy(mpFEBInBufPool_Main1);
    FatImageBufferPool::destroy(mpFEBInBufPool_Main1_NonSlant);
    FatImageBufferPool::destroy(mpFEBInBufPool_Main2);
    FatImageBufferPool::destroy(mpFECInBufPool_Main1);
    FatImageBufferPool::destroy(mpFECInBufPool_Main2);
    FatImageBufferPool::destroy(mpDefaultWarpMapXBufPool);
    FatImageBufferPool::destroy(mpDefaultWarpMapYBufPool);
    TuningBufferPool::destroy(mpTuningBufferPool);
    TuningBufferPool::destroy(mpPQTuningBufferPool);
    TuningBufferPool::destroy(mpDpPQParamTuningBufferPool);
    #ifdef GTEST
    FatImageBufferPool::destroy(mFEHWInput_StageB_Main1);
    FatImageBufferPool::destroy(mFEHWInput_StageB_Main2);
    FatImageBufferPool::destroy(mFEHWInput_StageC_Main1);
    FatImageBufferPool::destroy(mFEHWInput_StageC_Main2);
    #endif
    FatImageBufferPool::destroy(mMYSImgBufPool);
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        FatImageBufferPool::destroy(mpInternalMain2RRZObufPool);
        FatImageBufferPool::destroy(mIMG3OmgBufPool);
        FatImageBufferPool::destroy(mInternalFDImgBufPool);
    }
    //----------------------N3D section--------------------------------//
    GraphicBufferPool::destroy(mN3DWarpingMatrix_Main2_X);
    GraphicBufferPool::destroy(mN3DWarpingMatrix_Main2_Y);
    #if(SUPPORT_CAPTURE == 1)
        GraphicBufferPool::destroy(mN3DWarpingMatrix_Main2_X_CAP);
        GraphicBufferPool::destroy(mN3DWarpingMatrix_Main2_Y_CAP);
    #endif
    //----------------------WPE section--------------------------------//
    FatImageBufferPool::destroy(mDefaultMaskBufPool_Main2);
    FatImageBufferPool::destroy(mWarpImgBufPool_Main1);
    FatImageBufferPool::destroy(mWarpMaskBufPool_Main1);
    FatImageBufferPool::destroy(mWarpImgBufPool_Main2);
    FatImageBufferPool::destroy(mWarpMaskBufPool_Main2);
    GraphicBufferPool::destroy(mWarpGraBufPool_Main2);
    GraphicBufferPool::destroy(mWarpMaskGraBufPool_Main2);
    #if(SUPPORT_CAPTURE == 1)
        FatImageBufferPool::destroy(mWarpImgBufPool_Main1_CAP);
        FatImageBufferPool::destroy(mWarpMaskBufPool_Main1_CAP);
        FatImageBufferPool::destroy(mWarpImgBufPool_Main2_CAP);
        FatImageBufferPool::destroy(mWarpMaskBufPool_Main2_CAP);
    #endif
    //----------------------DPE section--------------------------------//
    FatImageBufferPool::destroy(mCFMImgBufPool);
    FatImageBufferPool::destroy(mNOCImgBufPool);
    FatImageBufferPool::destroy(mCFMImgBufPool_NonSlant);
    FatImageBufferPool::destroy(mNOCImgBufPool_NonSlant);
    //----------------------DVP section--------------------------------//
    if ( mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DVP) ) {
        FatImageBufferPool::destroy(mASFCRMImgBufPool);
        FatImageBufferPool::destroy(mASFRDImgBufPool);
        FatImageBufferPool::destroy(mASFHFImgBufPool);
        FatImageBufferPool::destroy(mDMWImgBufPool);
    }
    //----------------------GF section--------------------------------//
    if ( mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_GF) ) {
        FatImageBufferPool::destroy(mpInternalDMBGImgBufPool);
        FatImageBufferPool::destroy(mpInternalSwOptDepthImgBufPool);
    }
    //----------------------DLDEpth section---------------------------//
    if ( mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DLDEPTH) ) {
        FatImageBufferPool::destroy(mpInternalDLDepthImgBufPool);
        FatImageBufferPool::destroy(mpInternalDLDepthImgBufPool_NonSlant);
    }
    //----------------------Static section---------------------------//
    uninitStaticBuffers();

    if(mpBufferSizeMgr != nullptr)
        delete mpBufferSizeMgr;

    return MTRUE;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferPoolMgr Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
SmartFatImageBuffer
NodeBufferPoolMgr_VSDOF::
request(DepthMapBufferID id, BufferPoolScenario scen)
{
    ssize_t index;
    if((index=mBIDtoImgBufPoolMap_Default.indexOfKey(id)) >= 0)
    {
        TIME_THREHOLD(10, "request buffer id=%d %s", id, DepthMapPipeNode::onDumpBIDToName(id));
        sp<FatImageBufferPool> pBufferPool = mBIDtoImgBufPoolMap_Default.valueAt(index);
        return pBufferPool->request();
    }
    else if((index=mBIDtoImgBufPoolMap_Scenario.indexOfKey(id)) >= 0)
    {
        ScenarioToImgBufPoolMap ScenarioBufMap = mBIDtoImgBufPoolMap_Scenario.valueAt(index);
        if((index=ScenarioBufMap.indexOfKey(scen))>=0)
        {
            TIME_THREHOLD(10, "request buffer id=%d %s", id, DepthMapPipeNode::onDumpBIDToName(id));
            sp<FatImageBufferPool> pBufferPool = ScenarioBufMap.valueAt(index);
            return pBufferPool->request();
        }
    }
    return NULL;
}

IImageBuffer*
NodeBufferPoolMgr_VSDOF::
requestStaticBuffer(DepthMapBufferID id)
{
    if(mBIDtoStaticBufferMap.count(id) > 0)
    {
        return mBIDtoStaticBufferMap.at(id);
    }
    return NULL;
}

SmartGraphicBuffer
NodeBufferPoolMgr_VSDOF::
requestGB(DepthMapBufferID id, BufferPoolScenario scen)
{

    ssize_t index;
    if((index=mBIDtoGraBufPoolMap_Scenario.indexOfKey(id)) >= 0)
    {
        TIME_THREHOLD(10, "request GB buffer id=%d %s", id, DepthMapPipeNode::onDumpBIDToName(id));
        ScenarioToGraBufPoolMap ScenarioBufMap = mBIDtoGraBufPoolMap_Scenario.valueAt(index);
        if((index=ScenarioBufMap.indexOfKey(scen))>=0)
        {
            sp<GraphicBufferPool> pBufferPool = ScenarioBufMap.valueAt(index);
            VSDOF_LOGD("requestGB  bufferID=%d", id);
            SmartGraphicBuffer smGraBuf = pBufferPool->request();
            return smGraBuf;
        }
    }
    return NULL;
}

SmartTuningBuffer
NodeBufferPoolMgr_VSDOF::
requestTB(DepthMapBufferID id, BufferPoolScenario scen)
{
    TIME_THREHOLD(10, "request tuning buffer id=%d %s", id, DepthMapPipeNode::onDumpBIDToName(id));
    SmartTuningBuffer smTuningBuf = nullptr;
    if(id == BID_P2A_TUNING)
    {
        smTuningBuf = mpTuningBufferPool->request();
        memset(smTuningBuf->mpVA, 0, mpTuningBufferPool->getBufSize());
    }
    else if(id == BID_PQ_PARAM)
    {
        smTuningBuf = mpPQTuningBufferPool->request();
        memset(smTuningBuf->mpVA, 0, mpPQTuningBufferPool->getBufSize());
    }
    else if(id == BID_DP_PQ_PARAM)
    {
        smTuningBuf = mpDpPQParamTuningBufferPool->request();
        memset(smTuningBuf->mpVA, 0, mpDpPQParamTuningBufferPool->getBufSize());
    }

    if(smTuningBuf.get() == nullptr)
        MY_LOGE("Cannot find the TuningBufferPool with scenario:%d of buffer id:%d!!", scen, id);
    return smTuningBuf;
}

BufferPoolHandlerPtr
NodeBufferPoolMgr_VSDOF::
createBufferPoolHandler()
{
    BaseBufferHandler* pPtr = new NodeBufferHandler(this, mpPipeOption->mFlowType);
    return pPtr;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
queryBufferType(
    DepthMapBufferID bid,
    BufferPoolScenario scen,
    DepthBufferType& rOutBufType
)
{
    ssize_t index;

    if((index=mBIDToScenarioTypeMap.indexOfKey(bid))>=0)
    {
        BufScenarioToTypeMap scenMap = mBIDToScenarioTypeMap.valueAt(index);
        if((index=scenMap.indexOfKey(scen))>=0)
        {
            rOutBufType = scenMap.valueAt(index);
            return MTRUE;
        }
    }
    return MFALSE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferPoolMgr_VSDOF class - Private Functinos
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MVOID
NodeBufferPoolMgr_VSDOF::
initBT160ColorSpaceForYUVGB(sp<GraphicBufferPool> pool)
{
    GraphicBufferPool::CONTAINER_TYPE poolContents = pool->getPoolContents();
    for(sp<GraphicBufferHandle>& handle : poolContents)
    {
        // config
        android::sp<NativeBufferWrapper> pNativBuffer = handle->mGraphicBuffer;
        // config graphic buffer to BT601_FULL
        gralloc_extra_ion_sf_info_t info = {0};
        gralloc_extra_query(pNativBuffer->getHandle(), GRALLOC_EXTRA_GET_IOCTL_ION_SF_INFO, &info);
        gralloc_extra_sf_set_status(&info, GRALLOC_EXTRA_MASK_YUV_COLORSPACE, GRALLOC_EXTRA_BIT_YUV_BT601_FULL);
        gralloc_extra_perform(pNativBuffer->getHandle(), GRALLOC_EXTRA_SET_IOCTL_ION_SF_INFO, &info);
    }
}

MBOOL
NodeBufferPoolMgr_VSDOF::
initializeBufferPool()
{
    CAM_ULOGM_TAGLIFE("NodeBufferPoolMgr_VSDOF::initializeBufferPool");
    VSDOF_INIT_LOG("+");

    MBOOL bRet = MTRUE;
    // more +1 for static buffers
    std::future<MBOOL> vFutures[eDPETHMAP_PIPE_NODE_SIZE+1+1];
    int index = 0;

    // P2A buffer pool
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_P2A))
    {
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initP2ABufferPool, this);
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initFEFMBufferPool, this);
    }
    // N3D buffer pool
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_N3D))
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initN3DWPEBufferPool, this);
    // DVS buffer pool
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DVS))
    {
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initDVSBufferPool, this);
    }
    // DVP buffer pool
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DVP))
    {
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initDVPBufferPool, this);
    }
    // GF buffer pool
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_GF))
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initGFBufferPool, this);
    // DLDEPTH buffer pool
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DLDEPTH))
        vFutures[index++] = std::async(
                                std::launch::async,
                                &NodeBufferPoolMgr_VSDOF::initDLDepthBufferPool, this);
    // Static buffers
    vFutures[index++] = std::async(
                            std::launch::async,
                            &NodeBufferPoolMgr_VSDOF::initStaticBuffers, this);
    // wait all futures
    for(int idx = index-1; idx >=0; --idx)
    {
        bRet &= vFutures[idx].get();
    }

    VSDOF_INIT_LOG("-");
    return bRet;
}



MBOOL
NodeBufferPoolMgr_VSDOF::
initP2ABufferPool()
{
    VSDOF_INIT_LOG("+");
    const P2ABufferSize& rP2A_VR  = mpBufferSizeMgr->getP2A(eSTEREO_SCENARIO_RECORD);
    const P2ABufferSize& rP2A_PV  = mpBufferSizeMgr->getP2A(eSTEREO_SCENARIO_PREVIEW);
    const P2ABufferSize& rP2A_CAP = mpBufferSizeMgr->getP2A(eSTEREO_SCENARIO_CAPTURE);

    /**********************************************************************
     * Rectify_in/FD/Tuning/FE/FM buffer pools
     **********************************************************************/

    // Rect_in2 (main2)
    MSize RectIn2_size = (mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW)?
                        rP2A_PV.mRECT_IN_SIZE_MAIN2 : rP2A_VR.mRECT_IN_SIZE_MAIN2;
    if (hasWPEHw()) {
        CREATE_IMGBUF_POOL(mpRectInBufPool_Main2, "RectInBufPool_Main2",
                            RectIn2_size,
                            eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mpRectInBufPool_Main2, CALC_BUFFER_COUNT(P2A_RUN_MS+N3D_RUN_MS+WPE_RUN_MS));
    } else {
        CREATE_GRABUF_POOL(mpRectInGraBufPool_Main2, "RectInGraBufPool_Main2",
                            RectIn2_size,
                            eImgFmt_YV12, GraphicBufferPool::USAGE_HW_TEXTURE);
        ALLOCATE_BUFFER_POOL(mpRectInGraBufPool_Main2, CALC_BUFFER_COUNT(P2A_RUN_MS+N3D_RUN_MS+WPE_RUN_MS));
    }
    // Rect_in1 (main1)
    MSize RectIn1_size = (mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW)?
                       rP2A_PV.mRECT_OUT1_SIZE : rP2A_VR.mRECT_OUT1_SIZE;
    CREATE_IMGBUF_POOL(mpRectInBufPool_Main1, "RectInBufPool_Main1",
                       RectIn1_size, eImgFmt_Y8, eBUFFER_USAGE_SW_READ_OFTEN|FatImageBufferPool::USAGE_HW);
    ALLOCATE_BUFFER_POOL(mpRectInBufPool_Main1, CALC_BUFFER_COUNT(P2A_RUN_MS+N3D_RUN_MS));

    #if(SUPPORT_CAPTURE == 1)
        // Capture, Rect_in1
        CREATE_IMGBUF_POOL(mpRectInBufPool_Main1_CAP, "RectInBufPool_Main1_CAP",
                            rP2A_CAP.mRECT_IN_SIZE_MAIN1,
                            eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mpRectInBufPool_Main1_CAP, VSDOF_WORKING_BUF_SET);
        // Capture, Rect_in2
        CREATE_IMGBUF_POOL(mpRectInBufPool_Main2_CAP, "RectInGra_BufPool",
                            rP2A_CAP.mRECT_IN_SIZE_MAIN2,
                            eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mpRectInBufPool_Main2_CAP, VSDOF_WORKING_BUF_SET);
    #endif

    if (mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        // Internal Main1 YUVO
        MSize main1YUVOSize = mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW ?
                                    rP2A_PV.mMAIN1_YUVO_SIZE : rP2A_VR.mMAIN1_YUVO_SIZE;
        CREATE_IMGBUF_POOL(mpInternalMain1YUVObufPool, "mInternalMain1YUVO_BufPool",
                            main1YUVOSize, eImgFmt_MTK_YUV_P010,
                            eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mpInternalMain1YUVObufPool, CALC_BUFFER_COUNT(P2A_RUN_MS));

        // Internal Main2 YUVO
        MSize main2YUVOSize = mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW ?
                                    rP2A_PV.mMAIN2_YUVO_SIZE : rP2A_VR.mMAIN2_YUVO_SIZE;
        CREATE_IMGBUF_POOL(mpInternalMain2YUVObufPool, "mInternalMain2YUVO_BufPool",
                            main2YUVOSize, eImgFmt_MTK_YUV_P010,
                            eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mpInternalMain2YUVObufPool, CALC_BUFFER_COUNT(P2ABAYER_RUN_MS));
    }

    //
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        // Intenral Main2 RRZO
        CREATE_IMGBUF_POOL_USER_STRIDE(mpInternalMain2RRZObufPool, "mpInternalMain2RRZO_BufPool", rP2A_PV.mMAIN2_RRZO_SIZE,
                            eImgFmt_FG_BAYER10, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | FatImageBufferPool::USAGE_HW,
                            std::vector<MUINT32>(1, rP2A_PV.mMAIN2_RRZO_STRIDE));
        ALLOCATE_BUFFER_POOL(mpInternalMain2RRZObufPool, CALC_BUFFER_COUNT(P2ABAYER_RUN_MS));
        // Internal FD
        CREATE_IMGBUF_POOL(mInternalFDImgBufPool, "mInternalFDImgBufPool", rP2A_PV.mFD_IMG_SIZE,
                            eImgFmt_YUY2, eBUFFER_USAGE_SW_READ_OFTEN|FatImageBufferPool::USAGE_HW)
        ALLOCATE_BUFFER_POOL(mInternalFDImgBufPool, VSDOF_WORKING_BUF_SET*2);
        // IMG3O bufers
        CREATE_IMGBUF_POOL(mIMG3OmgBufPool, "IMG3OmgBufPool", mpPipeSetting->mszRRZO_Main1,
                            eImgFmt_YV12, FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mIMG3OmgBufPool, CALC_BUFFER_COUNT(P2A_RUN_MS)+QUEUE_DATA_BACKUP);
    }
    // TuningBufferPool creation
    mpTuningBufferPool = TuningBufferPool::create("VSDOF_TUNING_P2A", INormalStream::getRegTableSize());
    mpPQTuningBufferPool = TuningBufferPool::create("PQTuningPool", sizeof(PQParam));
    mpDpPQParamTuningBufferPool = TuningBufferPool::create("PQTuningPool", sizeof(DpPqParam));
    ALLOCATE_BUFFER_POOL(mpTuningBufferPool, VSDOF_DEPTH_P2FRAME_SIZE + 2);
    ALLOCATE_BUFFER_POOL(mpPQTuningBufferPool, VSDOF_DEPTH_P2FRAME_SIZE);
    ALLOCATE_BUFFER_POOL(mpDpPQParamTuningBufferPool, VSDOF_DEPTH_P2FRAME_SIZE);

    // init MY_S
    MSize MYS_size = (mpPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW)?
                       rP2A_PV.mMYS_SIZE.size : rP2A_VR.mMYS_SIZE.size;
    CREATE_IMGBUF_POOL(mMYSImgBufPool, "mMYSImgBufPool", MYS_size, eImgFmt_NV12,
                        eBUFFER_USAGE_SW_READ_OFTEN|FatImageBufferPool::USAGE_HW);
    // MY_S : from P2ABayer to GF
    ALLOCATE_BUFFER_POOL(mMYSImgBufPool, CALC_BUFFER_COUNT(TOTAL_RUN_MS));
    VSDOF_INIT_LOG("-");
    return MTRUE;
}


MBOOL
NodeBufferPoolMgr_VSDOF::
initFEFMBufferPool()
{
    VSDOF_INIT_LOG("+");
    const P2ABufferSize& rP2aSize = mpBufferSizeMgr->getP2A(eSTEREO_SCENARIO_CAPTURE);
    /**********************************************************************
     * FE/FM has 3 stage A,B,C, currently only apply 2 stage FEFM: stage=B(1),C(2)
     **********************************************************************/
    // FE/FM buffer pool - stage B
    MUINT32 iBlockSize = StereoSettingProvider::fefmBlockSize(1);
    // query the FEO buffer size from FE input buffer size
    MSize szFEBufSize = rP2aSize.mFEB_INPUT_SIZE_MAIN1.size;
    MSize szFEOBufferSize, szFMOBufferSize;
    queryFEOBufferSize(szFEBufSize, iBlockSize, szFEOBufferSize);
    // create buffer pool
    CREATE_IMGBUF_POOL(mpFEOB_BufPool, "FEB_BufPoll", szFEOBufferSize,
                        eImgFmt_STA_BYTE, FatImageBufferPool::USAGE_HW);
    // query the FMO buffer size from FEO size
    queryFMOBufferSize(szFEOBufferSize, szFMOBufferSize);
    // create buffer pool
    CREATE_IMGBUF_POOL(mpFMOB_BufPool, "FMB_BufPool", szFMOBufferSize,
                        eImgFmt_STA_BYTE, FatImageBufferPool::USAGE_HW);

    // FE/FM buffer pool - stage C
    iBlockSize = StereoSettingProvider::fefmBlockSize(2);
    // query FEO/FMO size and create pool
    szFEBufSize = rP2aSize.mFEC_INPUT_SIZE_MAIN1;
    queryFEOBufferSize(szFEBufSize, iBlockSize, szFEOBufferSize);
    CREATE_IMGBUF_POOL(mpFEOC_BufPool, "FEC_BufPool", szFEOBufferSize,
                        eImgFmt_STA_BYTE, FatImageBufferPool::USAGE_HW);
    queryFMOBufferSize(szFEOBufferSize, szFMOBufferSize);
    CREATE_IMGBUF_POOL(mpFMOC_BufPool, "FMC_BufPool", szFMOBufferSize,
                        eImgFmt_STA_BYTE, FatImageBufferPool::USAGE_HW);

    // create the FE input buffer pool - stage B (the seocond FE input buffer)
    //FEB Main1 input
    CREATE_IMGBUF_POOL(mpFEBInBufPool_Main1, "FE1BInputBufPool", rP2aSize.mFEB_INPUT_SIZE_MAIN1.size,
                    eImgFmt_YUY2, FatImageBufferPool::USAGE_HW);

    //FEB Main2 input
    CREATE_IMGBUF_POOL(mpFEBInBufPool_Main2, "FE2BInputBufPool", rP2aSize.mFEB_INPUT_SIZE_MAIN2.size,
                    eImgFmt_YUY2, FatImageBufferPool::USAGE_HW);

    //FEC Main1 input
    CREATE_IMGBUF_POOL(mpFECInBufPool_Main1, "FE1CInputBufPool", rP2aSize.mFEC_INPUT_SIZE_MAIN1,
                    eImgFmt_YUY2, FatImageBufferPool::USAGE_HW);

    //FEC Main2 input
    CREATE_IMGBUF_POOL(mpFECInBufPool_Main2, "FE2CInputBufPool", rP2aSize.mFEC_INPUT_SIZE_MAIN2,
                    eImgFmt_YUY2, FatImageBufferPool::USAGE_HW);

    // FEO/FMO buffer pool- ALLOCATE buffers : Main1+Main2 -> two working set
    int FEFMO_BUFFER_COUNT = 2 * CALC_BUFFER_COUNT(P2A_RUN_MS+P2ABAYER_RUN_MS+N3D_RUN_MS+N3D_LEARNING_RUN_MS);
    ALLOCATE_BUFFER_POOL(mpFEOB_BufPool, FEFMO_BUFFER_COUNT)
    ALLOCATE_BUFFER_POOL(mpFEOC_BufPool, FEFMO_BUFFER_COUNT)
    ALLOCATE_BUFFER_POOL(mpFMOB_BufPool, FEFMO_BUFFER_COUNT)
    ALLOCATE_BUFFER_POOL(mpFMOC_BufPool, FEFMO_BUFFER_COUNT)

    // FEB/FEC_Input buffer pool- ALLOCATE buffers : 2 (internal working buffer in Burst trigger)
    ALLOCATE_BUFFER_POOL(mpFEBInBufPool_Main1, 2)
    ALLOCATE_BUFFER_POOL(mpFEBInBufPool_Main2, 2)
    ALLOCATE_BUFFER_POOL(mpFECInBufPool_Main1, 2)
    ALLOCATE_BUFFER_POOL(mpFECInBufPool_Main2, 2)

    if ( StereoSettingProvider::isSlantCameraModule() )
    {
        //FEB Main1 NonSlant input
        CREATE_IMGBUF_POOL(mpFEBInBufPool_Main1_NonSlant,
                    "FE1BInputBufPool_NonSlant",
                    rP2aSize.mFEB_INPUT_SIZE_MAIN1_NONSLANT.size,
                    eImgFmt_YUY2, FatImageBufferPool::USAGE_HW);
        ALLOCATE_BUFFER_POOL(mpFEBInBufPool_Main1_NonSlant, 2)
        // Default WarpMap X
        CREATE_IMGBUF_POOL(mpDefaultWarpMapXBufPool,
                    "DefaultWarpMapXBufPool",
                    MSize(2,2), eImgFmt_STA_4BYTE,
                    FatImageBufferPool::USAGE_HW_AND_SW);
        ALLOCATE_BUFFER_POOL(mpDefaultWarpMapXBufPool,
                    CALC_BUFFER_COUNT(TOTAL_RUN_MS));
        // Default WarpMap Y
        CREATE_IMGBUF_POOL(mpDefaultWarpMapYBufPool,
                    "DefaultWarpMapYBufPool",
                    MSize(2,2), eImgFmt_STA_4BYTE,
                    FatImageBufferPool::USAGE_HW_AND_SW);
        ALLOCATE_BUFFER_POOL(mpDefaultWarpMapYBufPool,
                    CALC_BUFFER_COUNT(TOTAL_RUN_MS));
    }

    #ifdef GTEST
    // FE HW Input For UT
    CREATE_IMGBUF_POOL(mFEHWInput_StageB_Main1, "mFEHWInput_StageB_Main1", rP2aSize.mFEB_INPUT_SIZE_MAIN1.size, eImgFmt_YV12, FatImageBufferPool::USAGE_HW);
    CREATE_IMGBUF_POOL(mFEHWInput_StageB_Main2, "mFEHWInput_StageB_Main2", rP2aSize.mFEBO_AREA_MAIN2.size, eImgFmt_YV12, FatImageBufferPool::USAGE_HW);
    CREATE_IMGBUF_POOL(mFEHWInput_StageC_Main1, "mFEHWInput_StageC_Main1", rP2aSize.mFEC_INPUT_SIZE_MAIN1, eImgFmt_YV12, FatImageBufferPool::USAGE_HW);
    CREATE_IMGBUF_POOL(mFEHWInput_StageC_Main2, "mFEHWInput_StageC_Main2", rP2aSize.mFECO_AREA_MAIN2.size, eImgFmt_YV12, FatImageBufferPool::USAGE_HW);
    ALLOCATE_BUFFER_POOL(mFEHWInput_StageB_Main1, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mFEHWInput_StageB_Main2, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mFEHWInput_StageC_Main1, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mFEHWInput_StageC_Main2, VSDOF_WORKING_BUF_SET);
    #endif
    VSDOF_INIT_LOG("-");

    return MTRUE;
}


MBOOL
NodeBufferPoolMgr_VSDOF::
initN3DWPEBufferPool()
{
    VSDOF_INIT_LOG("+");
    // n3d size : no difference between scenarios
    const N3DBufferSize& rN3DSize = mpBufferSizeMgr->getN3D(mpPipeOption->mStereoScenario);
    const N3DBufferSize& rN3DSize_CAP = mpBufferSizeMgr->getN3D(eSTEREO_SCENARIO_CAPTURE);
    // SW read/write, hw read
    MUINT32 usage = eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ;

    // INIT N3D/WPE output buffer pool
    // MV_Y
    CREATE_IMGBUF_POOL(mWarpImgBufPool_Main1, "WarpImgBufPool_Main1", rN3DSize.mWARP_IMG_SIZE,
                        eImgFmt_Y8, usage);
    // MASK_M
    CREATE_IMGBUF_POOL(mWarpMaskBufPool_Main1, "N3DMaskBufPool", rN3DSize.mWARP_MASK_SIZE,
                        eImgFmt_Y8, usage);
    if (hasWPEHw()) {
        // SV_Y
        CREATE_IMGBUF_POOL(mWarpImgBufPool_Main2, "mWarpImgBufPool_Main2", rN3DSize.mWARP_IMG_SIZE,
                            eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
        // MASK_S
        CREATE_IMGBUF_POOL(mWarpMaskBufPool_Main2, "N3DMaskBufPool_Main2", rN3DSize.mWARP_MASK_SIZE,
                            eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
    } else {
        // SV_Y
        CREATE_GRABUF_POOL(mWarpGraBufPool_Main2, "mWarpGraBufPool_Main2", rN3DSize.mWARP_IMG_SIZE,
                            eImgFmt_YV12 , GraphicBufferPool::USAGE_HW_TEXTURE);
        // MASK_S
        CREATE_GRABUF_POOL(mWarpMaskGraBufPool_Main2, "mWarpMaskGraBufPool_Main2", rN3DSize.mWARP_MASK_SIZE,
                            eImgFmt_YV12 , usage);
    }
    // mN3DWarpingMatrix_Main2_X
    CREATE_GRABUF_POOL(mN3DWarpingMatrix_Main2_X, "N3DWarpingMatrix_Main2_X", rN3DSize.mWARP_MAP_SIZE_MAIN2,
                        HAL_PIXEL_FORMAT_RGBA_8888 , GraphicBufferPool::USAGE_HW_TEXTURE);
    // mN3DWarpingMatrix_Main2_Y
    CREATE_GRABUF_POOL(mN3DWarpingMatrix_Main2_Y, "N3DWarpingMatrix_Main2_Y", rN3DSize.mWARP_MAP_SIZE_MAIN2,
                        HAL_PIXEL_FORMAT_RGBA_8888 , GraphicBufferPool::USAGE_HW_TEXTURE);
    // WPE_IN_MASK
    CREATE_IMGBUF_POOL(mDefaultMaskBufPool_Main2, "DefaultMaskBufPool", rN3DSize.mWARP_IN_MASK_MAIN2,
                        eImgFmt_Y8, usage);
    #if(SUPPORT_CAPTURE == 1)
    CREATE_IMGBUF_POOL(mWarpImgBufPool_Main1_CAP, "mWarpImgBufPool_Main1_CAP", rN3DSize_CAP.mWARP_IMG_SIZE,
                        eImgFmt_Y8, usage);
    CREATE_IMGBUF_POOL(mWarpImgBufPool_Main2_CAP, "mWarpImgBufPool_Main2_CAP", rN3DSize_CAP.mWARP_IMG_SIZE,
                        eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
    CREATE_IMGBUF_POOL(mWarpMaskBufPool_Main1_CAP, "mWarpMaskBufPool_Main1_CAP", rN3DSize_CAP.mWARP_MASK_SIZE,
                        eImgFmt_Y8, usage);
    CREATE_IMGBUF_POOL(mWarpMaskBufPool_Main2_CAP, "mWarpMaskBufPool_Main2_CAP", rN3DSize_CAP.mWARP_MASK_SIZE,
                        eImgFmt_Y8, FatImageBufferPool::USAGE_HW);
    CREATE_GRABUF_POOL(mN3DWarpingMatrix_Main2_X_CAP, "N3DWarpingMatrix_Main2_X_CAP", rN3DSize_CAP.mWARP_MAP_SIZE_MAIN2,
                        HAL_PIXEL_FORMAT_RGBA_8888 , GraphicBufferPool::USAGE_HW_TEXTURE);
    CREATE_GRABUF_POOL(mN3DWarpingMatrix_Main2_Y_CAP, "N3DWarpingMatrix_Main2_Y_CAP", rN3DSize_CAP.mWARP_MAP_SIZE_MAIN2,
                        HAL_PIXEL_FORMAT_RGBA_8888 , GraphicBufferPool::USAGE_HW_TEXTURE);
    #endif
    // allocate
    ALLOCATE_BUFFER_POOL(mWarpImgBufPool_Main1, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS+DPE_RUN_MS));
    ALLOCATE_BUFFER_POOL(mWarpMaskBufPool_Main1, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS+DPE_RUN_MS));
    if (hasWPEHw()) {
        ALLOCATE_BUFFER_POOL(mWarpImgBufPool_Main2, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS+DPE_RUN_MS));
        ALLOCATE_BUFFER_POOL(mWarpMaskBufPool_Main2, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS+DPE_RUN_MS));
    } else {
        ALLOCATE_BUFFER_POOL(mWarpGraBufPool_Main2, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS+DPE_RUN_MS));
        ALLOCATE_BUFFER_POOL(mWarpMaskGraBufPool_Main2, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS+DPE_RUN_MS));
    }
    ALLOCATE_BUFFER_POOL(mN3DWarpingMatrix_Main2_X, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS));
    ALLOCATE_BUFFER_POOL(mN3DWarpingMatrix_Main2_Y, CALC_BUFFER_COUNT(N3D_RUN_MS+WPE_RUN_MS));
    #if(SUPPORT_CAPTURE == 1)
    ALLOCATE_BUFFER_POOL(mWarpImgBufPool_Main1_CAP, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mWarpImgBufPool_Main2_CAP, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mWarpMaskBufPool_Main1_CAP, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mWarpMaskBufPool_Main2_CAP, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mN3DWarpingMatrix_Main2_X_CAP, VSDOF_WORKING_BUF_SET);
    ALLOCATE_BUFFER_POOL(mN3DWarpingMatrix_Main2_Y_CAP, VSDOF_WORKING_BUF_SET);
    #endif
    ALLOCATE_BUFFER_POOL(mDefaultMaskBufPool_Main2, 2);
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
initDVSBufferPool()
{
    VSDOF_INIT_LOG("+");
    const DVSBufferSize& rDVSSize = mpBufferSizeMgr->getDVS(mpPipeOption->mStereoScenario);
    // allocate with the stride size as the width
    // CFM
    CREATE_IMGBUF_POOL(mCFMImgBufPool, "CFMImgBufPool", rDVSSize.mCFM_SIZE, eImgFmt_Y8,
                        FatImageBufferPool::USAGE_HW_AND_SW)
    // NOC
    CREATE_IMGBUF_POOL(mNOCImgBufPool, "NOCImgBufPool", rDVSSize.mNOC_SIZE, eImgFmt_Y8,
                        FatImageBufferPool::USAGE_HW_AND_SW)

    ALLOCATE_BUFFER_POOL(mCFMImgBufPool, CALC_BUFFER_COUNT(DPE_RUN_MS+GF_RUN_MS));
    MUINT32 NOCPoolSize = mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH?
                            CALC_BUFFER_COUNT(DPE_RUN_MS)+QUEUE_DATA_BACKUP :
                            CALC_BUFFER_COUNT(DPE_RUN_MS);
    ALLOCATE_BUFFER_POOL(mNOCImgBufPool, NOCPoolSize);
    // slant camera case
    if ( StereoSettingProvider::isSlantCameraModule() )
    {
        // non-slant CFM
        CREATE_IMGBUF_POOL(mCFMImgBufPool_NonSlant, "CFMImgBufPool_NonSlant",
                            rDVSSize.mCFM_NONSLANT_SIZE, eImgFmt_Y8,
                            FatImageBufferPool::USAGE_HW_AND_SW)
        // non-slant NOC
        CREATE_IMGBUF_POOL(mNOCImgBufPool_NonSlant, "NOCImgBufPool_NonSlant",
                            rDVSSize.mNOC_NONSLANT_SIZE, eImgFmt_Y8,
                            FatImageBufferPool::USAGE_HW_AND_SW)
        ALLOCATE_BUFFER_POOL(mCFMImgBufPool_NonSlant,
                            CALC_BUFFER_COUNT(DPE_RUN_MS+GF_RUN_MS));
        ALLOCATE_BUFFER_POOL(mNOCImgBufPool_NonSlant,
                            NOCPoolSize);

    }
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
initDVPBufferPool()
{
    VSDOF_INIT_LOG("+");
    const DVPBufferSize& rDVPSize = mpBufferSizeMgr->getDVP(mpPipeOption->mStereoScenario);
    // ASF_CRM
    CREATE_IMGBUF_POOL(mASFCRMImgBufPool, "ASFCRMImgBufPool", rDVPSize.mASF_CRM_SIZE, eImgFmt_Y8,
                        FatImageBufferPool::USAGE_HW)
    // ASF RD
    CREATE_IMGBUF_POOL(mASFRDImgBufPool, "ASFRDImgBufPool", rDVPSize.mASF_RD_SIZE, eImgFmt_Y8,
                        FatImageBufferPool::USAGE_HW)
    // ASF HF
    CREATE_IMGBUF_POOL(mASFHFImgBufPool, "ASFHFImgBufPool", rDVPSize.mASF_HF_SIZE, eImgFmt_Y8,
                        FatImageBufferPool::USAGE_HW)
    // DMW
    CREATE_IMGBUF_POOL(mDMWImgBufPool, "DMWImgBufPool", rDVPSize.mDMW_SIZE, eImgFmt_Y8,
                        FatImageBufferPool::USAGE_HW)
    // alloc
    ALLOCATE_BUFFER_POOL(mASFCRMImgBufPool, CALC_BUFFER_COUNT(DPE_RUN_MS));
    ALLOCATE_BUFFER_POOL(mASFRDImgBufPool, CALC_BUFFER_COUNT(DPE_RUN_MS));
    ALLOCATE_BUFFER_POOL(mASFHFImgBufPool, CALC_BUFFER_COUNT(DPE_RUN_MS));
    ALLOCATE_BUFFER_POOL(mDMWImgBufPool, CALC_BUFFER_COUNT(DPE_RUN_MS+GF_RUN_MS));
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
initGFBufferPool()
{
    VSDOF_INIT_LOG("+");
    MSize szDMBGSize = mpBufferSizeMgr->getGF(mpPipeOption->mStereoScenario).mDMBG_SIZE;
    MSize szDepthMapSize = mpBufferSizeMgr->getGF(mpPipeOption->mStereoScenario).mDEPTHMAP_SIZE;

    int bufferNumber = mpPipeOption->mFlowType == eDEPTH_FLOW_TYPE_QUEUED_DEPTH ?
                        CALC_BUFFER_COUNT(GF_RUN_MS)+QUEUE_DATA_BACKUP :
                        CALC_BUFFER_COUNT(GF_RUN_MS);
    if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_VSDOF )
    {
        CREATE_IMGBUF_POOL(mpInternalDMBGImgBufPool, "mpInternalDMBGImgBufPool", szDMBGSize
                            , eImgFmt_Y8, FatImageBufferPool::USAGE_SW);
        ALLOCATE_BUFFER_POOL(mpInternalDMBGImgBufPool, bufferNumber);
    }
    else
    {
        CREATE_IMGBUF_POOL(mpInternalSwOptDepthImgBufPool, "mpInternalSwOptDepthImgBufPool", szDepthMapSize
                            , eImgFmt_Y8, FatImageBufferPool::USAGE_SW);
        ALLOCATE_BUFFER_POOL(mpInternalSwOptDepthImgBufPool, bufferNumber);
    }
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
initDLDepthBufferPool()
{
    VSDOF_INIT_LOG("+");
    MSize szDLDepthSize = mpBufferSizeMgr->getDLDepth(eSTEREO_SCENARIO_RECORD).mDLDEPTH_SIZE;
    MSize szDLDepthSize_NonSlant = mpBufferSizeMgr->getDLDepth(eSTEREO_SCENARIO_RECORD).mDLDEPTH_SIZE_NONSLANT;
    CREATE_IMGBUF_POOL(mpInternalDLDepthImgBufPool, "mpInternalDLDepthImgBufPool", szDLDepthSize
                    , eImgFmt_Y8, FatImageBufferPool::USAGE_HW_AND_SW);
    ALLOCATE_BUFFER_POOL(mpInternalDLDepthImgBufPool, CALC_BUFFER_COUNT(DLDEPTH_RUN_MS+GF_RUN_MS));
    if ( StereoSettingProvider::isSlantCameraModule() ) {
        CREATE_IMGBUF_POOL(mpInternalDLDepthImgBufPool_NonSlant,
                           "mpInternalDLDepthImgBufPool_NonSlant",
                           szDLDepthSize_NonSlant, eImgFmt_Y8,
                           FatImageBufferPool::USAGE_HW_AND_SW);
        ALLOCATE_BUFFER_POOL(mpInternalDLDepthImgBufPool_NonSlant,
                            CALC_BUFFER_COUNT(DLDEPTH_RUN_MS+GF_RUN_MS));
    }
    VSDOF_INIT_LOG("-");
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
initStaticBuffers()
{
    VSDOF_INIT_LOG("+");
    // Warp map image params
    auto format = eImgFmt_STA_4BYTE;
    int plane = Utils::Format::queryPlaneCount(format);
    size_t stride[3] = {0, 0, 0};
    size_t boundary[3] = {0, 0, 0};
    MSize WarpMapSize(2, 2);
    for ( unsigned idx = 0; idx < plane; ++idx ) {
        stride[idx] = queryStrideInPixels(format, idx, WarpMapSize.w);
    }
    // buffer params
    int rotAngle = StereoSettingProvider::getModuleRotation();
    ENUM_STEREO_SCENARIO scenario = mpPipeOption->mStereoScenario;
    StereoSizeProvider* pSizePrvder = StereoSizeProvider::getInstance();
    IImageBufferAllocator::ImgParam imgParam(format, WarpMapSize, stride, boundary, plane);
    MUINT32 usage = eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN |
                    eBUFFER_USAGE_HW_CAMERA_READWRITE;
    // allocate & lockBuf
    #define ALLOCATE_AND_LOCK_BUF(buf)  \
        buf = IImageBufferAllocator::getInstance()->alloc(#buf, imgParam);  \
        if ( buf == nullptr ) { \
            MY_LOGE("failed to allocate %s", #buf); \
            return MFALSE;  \
        } \
        buf->lockBuf(#buf, usage); \
        VSDOF_MDEPTH_LOGD("Create Static Buffer, "#buf " size=%dx%d format=%x ", \
                buf->getImgSize().w, buf->getImgSize().h, buf->getImgFormat());
    //
    ALLOCATE_AND_LOCK_BUF(mpFE1BIn_WarpMapX);
    ALLOCATE_AND_LOCK_BUF(mpFE1BIn_WarpMapY);
    ALLOCATE_AND_LOCK_BUF(mpFE1BIn_WarpMapZ);
    ALLOCATE_AND_LOCK_BUF(mpFE2BIn_WarpMapX);
    ALLOCATE_AND_LOCK_BUF(mpFE2BIn_WarpMapY);
    ALLOCATE_AND_LOCK_BUF(mpFE2BIn_WarpMapZ);
    ALLOCATE_AND_LOCK_BUF(mpN3D_WarpMapX_Main1);
    ALLOCATE_AND_LOCK_BUF(mpN3D_WarpMapY_Main1);
    ALLOCATE_AND_LOCK_BUF(mpN3D_WarpMapZ_Main1);
    #undef ALLOCATE_AND_LOCK_BUF
    //
    const P2ABufferSize& rP2aSize = mpBufferSizeMgr->getP2A(scenario);
    // WarpMap for FE1BInput, nonslant -> slant
    {
        // input
        StereoArea areaFE1BIn_nonSlant = rP2aSize.mFEB_INPUT_SIZE_MAIN1_NONSLANT;
        // output
        StereoArea areaFE1BIn = rP2aSize.mFEB_INPUT_SIZE_MAIN1;
        MINT32* grid[3] = { (MINT32*)mpFE1BIn_WarpMapX->getBufVA(0),
                            (MINT32*)mpFE1BIn_WarpMapY->getBufVA(0),
                            (MINT32*)mpFE1BIn_WarpMapZ->getBufVA(0) };
        getGrid(grid,
                areaFE1BIn_nonSlant.size.w,
                areaFE1BIn_nonSlant.size.h,
                areaFE1BIn_nonSlant.size.w,
                areaFE1BIn.size.w,
                areaFE1BIn.size.h,
                areaFE1BIn.size.w,
                rotAngle);
        MY_LOGD("[getGrid] BID_P2A_WARPMTX_MAIN1, inWidth:%d, inHeight:%d, inStride:%d, outWidth:%d, outHeight:%d, outStride:%d rotAngle:%d",
                areaFE1BIn_nonSlant.size.w,
                areaFE1BIn_nonSlant.size.h,
                areaFE1BIn_nonSlant.size.w,
                areaFE1BIn.size.w,
                areaFE1BIn.size.h,
                areaFE1BIn.size.w,
                rotAngle);
    }
    // WarpMap for FE2BInput, Main2 YUVO -> slant FE2BInput
    {
        // input
        MSize sizeMain2Yuvo = rP2aSize.mMAIN2_YUVO_SIZE;
        MUINT32 strideMain2Yuvo = rP2aSize.mMAIN2_YUVO_STRIDE;
        // output
        StereoArea areaFE2BIn = rP2aSize.mFEB_INPUT_SIZE_MAIN2;
        MINT32* grid[3] = { (MINT32*)mpFE2BIn_WarpMapX->getBufVA(0),
                            (MINT32*)mpFE2BIn_WarpMapY->getBufVA(0),
                            (MINT32*)mpFE2BIn_WarpMapZ->getBufVA(0) };
        getGrid(grid,
                sizeMain2Yuvo.w,
                sizeMain2Yuvo.h,
                strideMain2Yuvo,
                areaFE2BIn.size.w,
                areaFE2BIn.size.h,
                areaFE2BIn.size.w,
                rotAngle);
        MY_LOGD("[getGrid] BID_P2A_WARPMTX_MAIN2, inWidth:%d, inHeight:%d, inStride:%d, outWidth:%d, outHeight:%d, outStride:%d, rotAngle:%d",
                sizeMain2Yuvo.w,
                sizeMain2Yuvo.h,
                strideMain2Yuvo,
                areaFE2BIn.size.w,
                areaFE2BIn.size.h,
                areaFE2BIn.size.w,
                rotAngle);
    }
    // WarpMap for MV_Y, Main1 RSSOR2 -> MV_Y
    {
        // input
        MSize sizeMain1Rssor2 = rP2aSize.mMAIN1_RSSOR2_SIZE;
        MUINT32 strideMain1Rssor2 = rP2aSize.mMAIN1_RSSOR2_STRIDE;
        // output
        StereoSizeProvider * pSizeProvder = StereoSizeProvider::getInstance();
        StereoArea areaMVY = pSizeProvder->getBufferSize(E_MV_Y, scenario);
        MINT32* grid[3] = { (MINT32*)mpN3D_WarpMapX_Main1->getBufVA(0),
                            (MINT32*)mpN3D_WarpMapY_Main1->getBufVA(0),
                            (MINT32*)mpN3D_WarpMapZ_Main1->getBufVA(0) };
        getGrid(grid,
                sizeMain1Rssor2.w,
                sizeMain1Rssor2.h,
                strideMain1Rssor2,
                areaMVY.size.w,
                areaMVY.size.h,
                areaMVY.size.w,
                rotAngle);
        MY_LOGD("[getGrid] BID_N3D_OUT_WARPMTX_MAIN1, inWidth:%d, inHeight:%d, inStride:%d, outWidth:%d, outHeight:%d, outStride:%d, rotAngle:%d",
                sizeMain1Rssor2.w,
                sizeMain1Rssor2.h,
                strideMain1Rssor2,
                areaMVY.size.w,
                areaMVY.size.h,
                areaMVY.size.w,
                rotAngle);
    }
    // Main1 MASK_M
    if ( !createMain1MASK() )
        return MFALSE;

    dumpStaticBuffers();

    VSDOF_INIT_LOG("-");
    return MTRUE;
}


MVOID
NodeBufferPoolMgr_VSDOF::
uninitStaticBuffers()
{
    #define UNLOCK_AND_DESTROY_BUF(buf) \
        if (buf) { \
            buf->unlockBuf(#buf); \
            IImageBufferAllocator::getInstance()->free(buf); }
    UNLOCK_AND_DESTROY_BUF(mpFE1BIn_WarpMapX);
    UNLOCK_AND_DESTROY_BUF(mpFE1BIn_WarpMapY);
    UNLOCK_AND_DESTROY_BUF(mpFE1BIn_WarpMapZ);
    UNLOCK_AND_DESTROY_BUF(mpFE2BIn_WarpMapX);
    UNLOCK_AND_DESTROY_BUF(mpFE2BIn_WarpMapY);
    UNLOCK_AND_DESTROY_BUF(mpFE2BIn_WarpMapZ);
    UNLOCK_AND_DESTROY_BUF(mpN3D_WarpMapX_Main1);
    UNLOCK_AND_DESTROY_BUF(mpN3D_WarpMapY_Main1);
    UNLOCK_AND_DESTROY_BUF(mpN3D_WarpMapZ_Main1);
    UNLOCK_AND_DESTROY_BUF(mpMain1_MASK_M);
    #undef UNLOCK_AND_DESTROY_BUF
}

MVOID
NodeBufferPoolMgr_VSDOF::
dumpStaticBuffers()
{
    MBOOL bDumpStaticBufffers = ::property_get_int32("vendor.debug.DepthPipe.TO_DUMP_STATIC_BUFFERS", 0);
    if (bDumpStaticBufffers == MFALSE)
        return;
    MY_LOGD("dumpStaticBuffers");
    const size_t PATH_SIZE = 1024;
    char filename[PATH_SIZE];
    #define DUMP_STATIC_BUF(BID, buf) \
        if (buf) {  \
            memset(filename, 0, sizeof(filename)); \
            int cx = 0; \
            cx = snprintf(filename, PATH_SIZE, "/data/vendor/camera_dump/000000000-0000-0000-main-undef-"#BID"__%dx%d", \
                        buf->getImgSize().w, buf->getImgSize().h); \
            buf->saveToFile(filename); \
        }
    DUMP_STATIC_BUF(BID_P2A_WARPMTX_X_MAIN1    , mpFE1BIn_WarpMapX);
    DUMP_STATIC_BUF(BID_P2A_WARPMTX_Y_MAIN1    , mpFE1BIn_WarpMapY);
    DUMP_STATIC_BUF(BID_P2A_WARPMTX_X_MAIN2    , mpFE2BIn_WarpMapX);
    DUMP_STATIC_BUF(BID_P2A_WARPMTX_Y_MAIN2    , mpFE2BIn_WarpMapY);
    DUMP_STATIC_BUF(BID_N3D_OUT_WARPMTX_MAIN1_X, mpN3D_WarpMapX_Main1);
    DUMP_STATIC_BUF(BID_N3D_OUT_WARPMTX_MAIN1_Y, mpN3D_WarpMapY_Main1);
    // DUMP_STATIC_BUF(BID_N3D_OUT_MASK_M_STATIC  , mpMain1_MASK_M);
    #undef DUMP_STATIC_BUF
}

// MBOOL
// NodeBufferPoolMgr_VSDOF::
// createMain1MASK()
// {
//     VSDOF_INIT_LOG("create MAIN1 MASK");
//     auto format = eImgFmt_Y8;
//     int plane = Utils::Format::queryPlaneCount(format);
//     size_t stride[3] = {0, 0, 0};
//     size_t stride_NonSlant[3] = {0, 0, 0};
//     size_t boundary[3] = {0, 0, 0};
//     MUINT32 usage = eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ;
//     // input
//     StereoSizeProvider * pSizeProvder = StereoSizeProvider::getInstance();
//     // output
//     MSize maskSize = pSizeProvder->getBufferSize(E_MASK_M_Y, mpPipeOption->mStereoScenario);
//     for ( unsigned idx = 0; idx < plane; ++idx ) {
//         stride[idx] = queryStrideInPixels(format, idx, maskSize.w);
//     }
//     IImageBufferAllocator::ImgParam imgParam(format, maskSize, stride, boundary, plane);
//     mpMain1_MASK_M = IImageBufferAllocator::getInstance()->alloc("mpMain1_MASK_M", imgParam);
//     if ( mpMain1_MASK_M == nullptr ) {
//         MY_LOGE("failed to allocate mpMain1_MASK_M");
//         return MFALSE;
//     }
//     mpMain1_MASK_M->lockBuf("mpMain1_MASK_M", usage);
//     // get N3D MASK_M buffer
//     ::memset((void*)mpMain1_MASK_M->getBufVA(0), 255, mpMain1_MASK_M->getBufSizeInBytes(0));
//     return MTRUE;
// }


MBOOL
NodeBufferPoolMgr_VSDOF::
createMain1MASK()
{
    VSDOF_INIT_LOG("create MAIN1 MASK");
    auto format = eImgFmt_Y8;
    int plane = Utils::Format::queryPlaneCount(format);
    size_t stride[3] = {0, 0, 0};
    size_t boundary[3] = {0, 0, 0};
    MUINT32 usage = eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ;
    // input
    StereoSizeProvider * pSizeProvder = StereoSizeProvider::getInstance();
    // pMain1_MASK_M_NonSlant->saveToFile("/data/vendor/camera_dump/source_MASK_M.y8");
    // output
    StereoArea areaMask_M = pSizeProvder->getBufferSize(E_MASK_M_Y, mpPipeOption->mStereoScenario);
    StereoArea areaMask_M_NONSLANT = pSizeProvder->getBufferSize(E_MASK_M_Y, mpPipeOption->mStereoScenario, false);
    for ( unsigned idx = 0; idx < plane; ++idx ) {
        stride[idx] = queryStrideInPixels(format, idx, areaMask_M.size.w);
    }
    IImageBufferAllocator::ImgParam imgParam(format, areaMask_M.size, stride, boundary, plane);
    mpMain1_MASK_M = IImageBufferAllocator::getInstance()->alloc("mpMain1_MASK_M", imgParam);
    if ( mpMain1_MASK_M == nullptr ) {
        MY_LOGE("failed to allocate mpMain1_MASK_M");
        return MFALSE;
    }
    mpMain1_MASK_M->lockBuf("mpMain1_MASK_M", usage);
    // get N3D MASK_M buffer
    int pad_L = areaMask_M.startPt.x;
    int pad_R = areaMask_M.padding.w - areaMask_M.startPt.x;
    int pad_U = areaMask_M.startPt.y;
    int pad_D = areaMask_M.padding.h - areaMask_M.startPt.y;
    VSDOF_LOGD("[MaskGen]in_w=%d, in_h=%d, out_w=%d, out_h=%d, pad_L=%d, pad_R=%d, pad_U=%d, pad_D=%d, rotAngle=%f, VA=%p",
            areaMask_M_NONSLANT.contentSize().w, areaMask_M_NONSLANT.contentSize().h, areaMask_M.contentSize().w, areaMask_M.contentSize().h,
            pad_L, pad_R, pad_U, pad_D,
            (float)StereoSettingProvider::getModuleRotation(), reinterpret_cast<MUINT8*>(mpMain1_MASK_M->getBufVA(0)));
    MaskGen(areaMask_M_NONSLANT.contentSize().w,         // in_w, nonslant contentSize()
            areaMask_M_NONSLANT.contentSize().h,         // in_h, nonslant contentSize()
            areaMask_M.contentSize().w,                  // out_w, slant contentSize()
            areaMask_M.contentSize().h,                  // out_h, slant contentSize()
            pad_L,               // pad_L
            pad_R,               // pad_R
            pad_U,               // pad_U
            pad_D,               // pad_D
            (float)StereoSettingProvider::getModuleRotation(),
            (unsigned char*)mpMain1_MASK_M->getBufVA(0));
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
buildImageBufferPoolMap()
{
    MY_LOGD("+");

    //---- build the buffer pool without specific scenario ----
    // P2A Part
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_P2A))
    {
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FE1BO, mpFEOB_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FE2BO, mpFEOB_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FE1CO, mpFEOC_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FE2CO, mpFEOC_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FMBO_RL, mpFMOB_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FMBO_LR, mpFMOB_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FMCO_LR, mpFMOC_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_OUT_FMCO_RL, mpFMOC_BufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_FE1B_INPUT, mpFEBInBufPool_Main1);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_FE1B_INPUT_NONSLANT, mpFEBInBufPool_Main1_NonSlant);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_FE2B_INPUT, mpFEBInBufPool_Main2);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_FE1C_INPUT, mpFECInBufPool_Main1);
        mBIDtoImgBufPoolMap_Default.add(BID_P2A_FE2C_INPUT, mpFECInBufPool_Main2);
        mBIDtoImgBufPoolMap_Default.add(BID_DEFAULT_WARPMAP_X, mpDefaultWarpMapXBufPool);
        mBIDtoImgBufPoolMap_Default.add(BID_DEFAULT_WARPMAP_Y, mpDefaultWarpMapYBufPool);
        #ifdef GTEST
        mBIDtoImgBufPoolMap_Default.add(BID_FE2_HWIN_MAIN1, mFEHWInput_StageB_Main1);
        mBIDtoImgBufPoolMap_Default.add(BID_FE2_HWIN_MAIN2, mFEHWInput_StageB_Main2);
        mBIDtoImgBufPoolMap_Default.add(BID_FE3_HWIN_MAIN1, mFEHWInput_StageC_Main1);
        mBIDtoImgBufPoolMap_Default.add(BID_FE3_HWIN_MAIN2, mFEHWInput_StageC_Main2);
        #endif

    }

    //---- build the buffer pool WITH specific scenario ----
    // P2A
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_P2A))
    {
        // ============ image buffer pool mapping =============
        // Rectify Main1 : BID_P2A_OUT_RECT_IN1
        ScenarioToImgBufPoolMap RectInImgBufMap_Main1;
        RectInImgBufMap_Main1.add(eBUFFER_POOL_SCENARIO_CAPTURE, mpRectInBufPool_Main1_CAP);
        RectInImgBufMap_Main1.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpRectInBufPool_Main1);
        RectInImgBufMap_Main1.add(eBUFFER_POOL_SCENARIO_RECORD, mpRectInBufPool_Main1);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_OUT_RECT_IN1, RectInImgBufMap_Main1);
        // MYS
        ScenarioToImgBufPoolMap MYSImgBufMap;
        MYSImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mMYSImgBufPool);
        MYSImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mMYSImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_OUT_MY_S, MYSImgBufMap);
        // IMG3O
        ScenarioToImgBufPoolMap IMG3OImgBufMap;
        IMG3OImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mIMG3OmgBufPool);
        IMG3OImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mIMG3OmgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_INTERNAL_IMG3O, IMG3OImgBufMap);
        // Internal FD
        ScenarioToImgBufPoolMap InternalFDImgBufMap;
        InternalFDImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mInternalFDImgBufPool);
        InternalFDImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mInternalFDImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_INTERNAL_FD, InternalFDImgBufMap);
        // Rectify Main2 : BID_P2A_OUT_RECT_IN2
        if (hasWPEHw()) {
            ScenarioToImgBufPoolMap RectInImgBufMap_Main2;
            RectInImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpRectInBufPool_Main2);
            RectInImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_RECORD, mpRectInBufPool_Main2);
            RectInImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_CAPTURE, mpRectInBufPool_Main2_CAP);
            mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_OUT_RECT_IN2, RectInImgBufMap_Main2);
        } else {
            ScenarioToGraBufPoolMap RectInGraBufMap_Main2;
            RectInGraBufMap_Main2.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpRectInGraBufPool_Main2);
            RectInGraBufMap_Main2.add(eBUFFER_POOL_SCENARIO_RECORD, mpRectInGraBufPool_Main2);
            mBIDtoGraBufPoolMap_Scenario.add(BID_P2A_OUT_RECT_IN2, RectInGraBufMap_Main2);
        }
        // Main1 YUVO internal
        ScenarioToImgBufPoolMap InternalMain1YUVOBufMap;
        InternalMain1YUVOBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalMain1YUVObufPool);
        InternalMain1YUVOBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalMain1YUVObufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_INTERNAL_IN_YUV1, InternalMain1YUVOBufMap);
        // Main2 YUVO Internal
        ScenarioToImgBufPoolMap InternalMain2YUVOBufMap;
        InternalMain2YUVOBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalMain2YUVObufPool);
        InternalMain2YUVOBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalMain2YUVObufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_INTERNAL_IN_YUV2, InternalMain2YUVOBufMap);
        // Main2 RRZO Internal
        ScenarioToImgBufPoolMap InternalMain2RRZOBufMap;
        InternalMain2RRZOBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalMain2RRZObufPool);
        InternalMain2RRZOBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalMain2RRZObufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_P2A_INTERNAL_IN_RRZ2, InternalMain2RRZOBufMap);
    }
    // N3D
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_N3D))
    {
        // WPE_IN_MASK
        ScenarioToImgBufPoolMap ImgBufMap_DefaultMask;
        ImgBufMap_DefaultMask.add(eBUFFER_POOL_SCENARIO_PREVIEW, mDefaultMaskBufPool_Main2);
        ImgBufMap_DefaultMask.add(eBUFFER_POOL_SCENARIO_RECORD, mDefaultMaskBufPool_Main2);
        ImgBufMap_DefaultMask.add(eBUFFER_POOL_SCENARIO_CAPTURE, mDefaultMaskBufPool_Main2);
        mBIDtoImgBufPoolMap_Scenario.add(BID_WPE_IN_MASK_S, ImgBufMap_DefaultMask);
        // MV_Y
        ScenarioToImgBufPoolMap MVY_ImgBufMap;
        MVY_ImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mWarpImgBufPool_Main1);
        MVY_ImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mWarpImgBufPool_Main1);
        MVY_ImgBufMap.add(eBUFFER_POOL_SCENARIO_CAPTURE, mWarpImgBufPool_Main1_CAP);
        mBIDtoImgBufPoolMap_Scenario.add(BID_N3D_OUT_MV_Y, MVY_ImgBufMap);
        // MASK_M
        ScenarioToImgBufPoolMap MASKImgBufMap;
        MASKImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mWarpMaskBufPool_Main1);
        MASKImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mWarpMaskBufPool_Main1);
        MASKImgBufMap.add(eBUFFER_POOL_SCENARIO_CAPTURE, mWarpMaskBufPool_Main1_CAP);
        mBIDtoImgBufPoolMap_Scenario.add(BID_N3D_OUT_MASK_M, MASKImgBufMap);
        // Main2 buffer need to be GraphicBuffer or Imagebuffer depend on HAS_WPE
        if (hasWPEHw()) {
            // SV_Y
            ScenarioToImgBufPoolMap WarpImgBufMap_Main2;
            WarpImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_PREVIEW, mWarpImgBufPool_Main2);
            WarpImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_RECORD, mWarpImgBufPool_Main2);
            WarpImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_CAPTURE, mWarpImgBufPool_Main2_CAP);
            mBIDtoImgBufPoolMap_Scenario.add(BID_WPE_OUT_SV_Y, WarpImgBufMap_Main2);
            // MASK_S
            ScenarioToImgBufPoolMap MaskImgBufMap_Main2;
            MaskImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_PREVIEW, mWarpMaskBufPool_Main2);
            MaskImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_RECORD, mWarpMaskBufPool_Main2);
            MaskImgBufMap_Main2.add(eBUFFER_POOL_SCENARIO_CAPTURE, mWarpMaskBufPool_Main2_CAP);
            mBIDtoImgBufPoolMap_Scenario.add(BID_WPE_OUT_MASK_S, MaskImgBufMap_Main2);
        } else {
            // SV_Y
            ScenarioToGraBufPoolMap WarpGraBufMap_Main2;
            WarpGraBufMap_Main2.add(eBUFFER_POOL_SCENARIO_PREVIEW, mWarpGraBufPool_Main2);
            WarpGraBufMap_Main2.add(eBUFFER_POOL_SCENARIO_RECORD, mWarpGraBufPool_Main2);
            mBIDtoGraBufPoolMap_Scenario.add(BID_WPE_OUT_SV_Y, WarpGraBufMap_Main2);
            // MASK_S
            ScenarioToGraBufPoolMap MaskGraBufMap_Main2;
            MaskGraBufMap_Main2.add(eBUFFER_POOL_SCENARIO_PREVIEW, mWarpMaskGraBufPool_Main2);
            MaskGraBufMap_Main2.add(eBUFFER_POOL_SCENARIO_RECORD, mWarpMaskGraBufPool_Main2);
            mBIDtoGraBufPoolMap_Scenario.add(BID_WPE_OUT_MASK_S, MaskGraBufMap_Main2);
        }
        // warping matrix (graphic)
        ScenarioToGraBufPoolMap GraBufMap_WarpMtx2_X;
        GraBufMap_WarpMtx2_X.add(eBUFFER_POOL_SCENARIO_PREVIEW, mN3DWarpingMatrix_Main2_X);
        GraBufMap_WarpMtx2_X.add(eBUFFER_POOL_SCENARIO_RECORD, mN3DWarpingMatrix_Main2_X);
        GraBufMap_WarpMtx2_X.add(eBUFFER_POOL_SCENARIO_CAPTURE, mN3DWarpingMatrix_Main2_X_CAP);
        mBIDtoGraBufPoolMap_Scenario.add(BID_N3D_OUT_WARPMTX_MAIN2_X, GraBufMap_WarpMtx2_X);

        ScenarioToGraBufPoolMap GraBufMap_WarpMtx2_Y;
        GraBufMap_WarpMtx2_Y.add(eBUFFER_POOL_SCENARIO_PREVIEW, mN3DWarpingMatrix_Main2_Y);
        GraBufMap_WarpMtx2_Y.add(eBUFFER_POOL_SCENARIO_RECORD, mN3DWarpingMatrix_Main2_Y);
        GraBufMap_WarpMtx2_Y.add(eBUFFER_POOL_SCENARIO_CAPTURE, mN3DWarpingMatrix_Main2_Y_CAP);
        mBIDtoGraBufPoolMap_Scenario.add(BID_N3D_OUT_WARPMTX_MAIN2_Y, GraBufMap_WarpMtx2_Y);
    }
    // DVS Part
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DVS))
    {
        // CFM
        ScenarioToImgBufPoolMap imgBufMap_CFM;
        imgBufMap_CFM.add(eBUFFER_POOL_SCENARIO_PREVIEW, mCFMImgBufPool);
        imgBufMap_CFM.add(eBUFFER_POOL_SCENARIO_RECORD, mCFMImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_OCC_OUT_CFM_M, imgBufMap_CFM);
        // NOC
        ScenarioToImgBufPoolMap imgBufMap_NOC;
        imgBufMap_NOC.add(eBUFFER_POOL_SCENARIO_PREVIEW, mNOCImgBufPool);
        imgBufMap_NOC.add(eBUFFER_POOL_SCENARIO_RECORD, mNOCImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_OCC_OUT_NOC_M, imgBufMap_NOC);
        if ( StereoSettingProvider::isSlantCameraModule() ) {
            // CFM
            ScenarioToImgBufPoolMap imgBufMap_CFM_NonSlant;
            imgBufMap_CFM_NonSlant.add(eBUFFER_POOL_SCENARIO_PREVIEW, mCFMImgBufPool_NonSlant);
            imgBufMap_CFM_NonSlant.add(eBUFFER_POOL_SCENARIO_RECORD, mCFMImgBufPool_NonSlant);
            mBIDtoImgBufPoolMap_Scenario.add(BID_OCC_OUT_CFM_M_NONSLANT, imgBufMap_CFM_NonSlant);
            // NOC
            ScenarioToImgBufPoolMap imgBufMap_NOC_NonSlant;
            imgBufMap_NOC_NonSlant.add(eBUFFER_POOL_SCENARIO_PREVIEW, mNOCImgBufPool_NonSlant);
            imgBufMap_NOC_NonSlant.add(eBUFFER_POOL_SCENARIO_RECORD, mNOCImgBufPool_NonSlant);
            mBIDtoImgBufPoolMap_Scenario.add(BID_OCC_OUT_NOC_M_NONSLANT, imgBufMap_NOC_NonSlant);
        }
    }
    // DVP Part
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DVP))
    {
        // ASF_CRM
        ScenarioToImgBufPoolMap imgBufMap_CRM;
        imgBufMap_CRM.add(eBUFFER_POOL_SCENARIO_PREVIEW, mASFCRMImgBufPool);
        imgBufMap_CRM.add(eBUFFER_POOL_SCENARIO_RECORD, mASFCRMImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_ASF_OUT_CRM, imgBufMap_CRM);
        // ASF_RD
        ScenarioToImgBufPoolMap imgBufMap_RD;
        imgBufMap_RD.add(eBUFFER_POOL_SCENARIO_PREVIEW, mASFRDImgBufPool);
        imgBufMap_RD.add(eBUFFER_POOL_SCENARIO_RECORD, mASFRDImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_ASF_OUT_RD, imgBufMap_RD);
        // ASF_HF
        ScenarioToImgBufPoolMap imgBufMap_HF;
        imgBufMap_HF.add(eBUFFER_POOL_SCENARIO_PREVIEW, mASFHFImgBufPool);
        imgBufMap_HF.add(eBUFFER_POOL_SCENARIO_RECORD, mASFHFImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_ASF_OUT_HF, imgBufMap_HF);
        // DMW
        ScenarioToImgBufPoolMap imgBufMap_DMW;
        imgBufMap_DMW.add(eBUFFER_POOL_SCENARIO_PREVIEW, mDMWImgBufPool);
        imgBufMap_DMW.add(eBUFFER_POOL_SCENARIO_RECORD, mDMWImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_WMF_OUT_DMW, imgBufMap_DMW);
    }
    // GF Part
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_GF))
    {
        // INTERNAL DMBG
        ScenarioToImgBufPoolMap DMBGImgBufMap;
        DMBGImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalDMBGImgBufPool);
        DMBGImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalDMBGImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_GF_INTERNAL_DMBG, DMBGImgBufMap);
        // INTERNAL DEPTHMAP
        ScenarioToImgBufPoolMap SwOptDepthMapImgBufMap;
        SwOptDepthMapImgBufMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalSwOptDepthImgBufPool);
        SwOptDepthMapImgBufMap.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalSwOptDepthImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_GF_INTERNAL_DEPTHMAP, SwOptDepthMapImgBufMap);
    }
    // DLDepth Part
    if(mNodeBitSet.test(eDPETHMAP_PIPE_NODEID_DLDEPTH))
    {
        ScenarioToImgBufPoolMap imgBufMap_DLDepth;
        imgBufMap_DLDepth.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalDLDepthImgBufPool);
        imgBufMap_DLDepth.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalDLDepthImgBufPool);
        mBIDtoImgBufPoolMap_Scenario.add(BID_DLDEPTH_INTERNAL_DEPTHMAP, imgBufMap_DLDepth);
        if ( StereoSettingProvider::isSlantCameraModule() ) {
            ScenarioToImgBufPoolMap imgBufMap_DLDepth_NonSlant;
            imgBufMap_DLDepth_NonSlant.add(eBUFFER_POOL_SCENARIO_PREVIEW, mpInternalDLDepthImgBufPool_NonSlant);
            imgBufMap_DLDepth_NonSlant.add(eBUFFER_POOL_SCENARIO_RECORD, mpInternalDLDepthImgBufPool_NonSlant);
            mBIDtoImgBufPoolMap_Scenario.add(BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT, imgBufMap_DLDepth_NonSlant);
        }
    }
    // Static part
    {
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_P2A_WARPMTX_X_MAIN1    , mpFE1BIn_WarpMapX));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_P2A_WARPMTX_Y_MAIN1    , mpFE1BIn_WarpMapY));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_P2A_WARPMTX_Z_MAIN1    , mpFE1BIn_WarpMapZ));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_P2A_WARPMTX_X_MAIN2    , mpFE2BIn_WarpMapX));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_P2A_WARPMTX_Y_MAIN2    , mpFE2BIn_WarpMapY));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_P2A_WARPMTX_Z_MAIN2    , mpFE2BIn_WarpMapZ));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_N3D_OUT_WARPMTX_MAIN1_X, mpN3D_WarpMapX_Main1));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_N3D_OUT_WARPMTX_MAIN1_Y, mpN3D_WarpMapY_Main1));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_N3D_OUT_WARPMTX_MAIN1_Z, mpN3D_WarpMapZ_Main1));
        mBIDtoStaticBufferMap.insert(std::make_pair(BID_N3D_OUT_MASK_M_STATIC  , mpMain1_MASK_M));
    }
    MY_LOGD("-");
    return MTRUE;
}


sp<FatImageBufferPool>
NodeBufferPoolMgr_VSDOF::
getImageBufferPool(
    DepthMapBufferID bufferID,
    BufferPoolScenario scenario
)
{
    ssize_t index=mBIDtoImgBufPoolMap_Default.indexOfKey(bufferID);
    if(index >= 0)
    {
        return mBIDtoImgBufPoolMap_Default.valueAt(index);
    }

    index = mBIDtoImgBufPoolMap_Scenario.indexOfKey(bufferID);
    if(index >= 0)
    {
        ScenarioToImgBufPoolMap bufMap;
        bufMap = mBIDtoImgBufPoolMap_Scenario.valueAt(index);
        if((index = bufMap.indexOfKey(scenario)) >= 0)
            return bufMap.valueAt(index);
    }

    MY_LOGW("Failed to get ImageBufferPool! bufferID=%d, scenario=%d", bufferID, scenario);
    return NULL;
}

sp<GraphicBufferPool>
NodeBufferPoolMgr_VSDOF::
getGraphicImageBufferPool(
    DepthMapBufferID bufferID,
    BufferPoolScenario scenario
)
{
    ssize_t index = mBIDtoGraBufPoolMap_Scenario.indexOfKey(bufferID);
    if(index >= 0)
    {
        ScenarioToGraBufPoolMap bufMap;
        bufMap = mBIDtoGraBufPoolMap_Scenario.valueAt(index);
        if((index = bufMap.indexOfKey(scenario)) >= 0)
            return bufMap.valueAt(index);
    }

    return NULL;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
buildBufScenarioToTypeMap()
{
    // buffer id in default imgBuf pool map -> all scenario are image buffer type
    for(ssize_t idx=0;idx<mBIDtoImgBufPoolMap_Default.size();++idx)
    {
        DepthMapBufferID bid = mBIDtoImgBufPoolMap_Default.keyAt(idx);
        BufScenarioToTypeMap typeMap;
        typeMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, eBUFFER_IMAGE);
        typeMap.add(eBUFFER_POOL_SCENARIO_RECORD, eBUFFER_IMAGE);
        typeMap.add(eBUFFER_POOL_SCENARIO_CAPTURE, eBUFFER_IMAGE);
        mBIDToScenarioTypeMap.add(bid, typeMap);
    }

    // buffer id in scenario imgBuf pool map -> the scenario is image buffer type
    for(ssize_t idx=0;idx<mBIDtoImgBufPoolMap_Scenario.size();++idx)
    {
        DepthMapBufferID bid = mBIDtoImgBufPoolMap_Scenario.keyAt(idx);
        ScenarioToImgBufPoolMap scenToBufMap = mBIDtoImgBufPoolMap_Scenario.valueAt(idx);

        BufScenarioToTypeMap typeMap;
        for(ssize_t idx2=0;idx2<scenToBufMap.size();++idx2)
        {
            BufferPoolScenario sce = scenToBufMap.keyAt(idx2);
            typeMap.add(sce, eBUFFER_IMAGE);
        }
        mBIDToScenarioTypeMap.add(bid, typeMap);
    }
    // graphic buffer section
    for(ssize_t idx=0;idx<mBIDtoGraBufPoolMap_Scenario.size();++idx)
    {
        DepthMapBufferID bid = mBIDtoGraBufPoolMap_Scenario.keyAt(idx);
        ScenarioToGraBufPoolMap scenToBufMap = mBIDtoGraBufPoolMap_Scenario.valueAt(idx);
        ssize_t idx2;
        if((idx2=mBIDToScenarioTypeMap.indexOfKey(bid))>=0)
        {
            BufScenarioToTypeMap& bufScenMap = mBIDToScenarioTypeMap.editValueAt(idx2);
            for(ssize_t idx2=0;idx2<scenToBufMap.size();++idx2)
            {
                BufferPoolScenario sce = scenToBufMap.keyAt(idx2);
                bufScenMap.add(sce, eBUFFER_GRAPHIC);
            }
        }
        else
        {
            BufScenarioToTypeMap typeMap;
            for(ssize_t idx2=0;idx2<scenToBufMap.size();++idx2)
            {
                BufferPoolScenario sce = scenToBufMap.keyAt(idx2);
                typeMap.add(sce, eBUFFER_GRAPHIC);
            }
            mBIDToScenarioTypeMap.add(bid, typeMap);
        }
    }
    // tuning buffer section
    BufScenarioToTypeMap typeMap;
    typeMap.add(eBUFFER_POOL_SCENARIO_PREVIEW, eBUFFER_TUNING);
    typeMap.add(eBUFFER_POOL_SCENARIO_RECORD, eBUFFER_TUNING);
    typeMap.add(eBUFFER_POOL_SCENARIO_CAPTURE, eBUFFER_TUNING);
    mBIDToScenarioTypeMap.add(BID_P2A_TUNING, typeMap);
    return MTRUE;
}

MBOOL
NodeBufferPoolMgr_VSDOF::
getAllPoolImageBuffer(
    DepthMapBufferID id,
    BufferPoolScenario scen,
    std::vector<IImageBuffer*>& rImgVec
)
{
    MBOOL bRet = MFALSE;
    if(mBIDtoImgBufPoolMap_Default.indexOfKey(id) >= 0)
    {
        bRet = MTRUE;
        sp<FatImageBufferPool> pool = mBIDtoImgBufPoolMap_Default.valueFor(id);
        FatImageBufferPool::CONTAINER_TYPE poolContents = pool->getPoolContents();
        for(sp<FatImageBufferHandle>& handle : poolContents)
        {
            rImgVec.push_back(handle->mImageBuffer.get());
        }
    }

    if(mBIDtoImgBufPoolMap_Scenario.indexOfKey(id) >= 0)
    {
        bRet = MTRUE;
        ScenarioToImgBufPoolMap bufMap = mBIDtoImgBufPoolMap_Scenario.valueFor(id);
        if(bufMap.indexOfKey(scen)>=0)
        {
            sp<FatImageBufferPool> pool = bufMap.valueFor(scen);
            FatImageBufferPool::CONTAINER_TYPE poolContents = pool->getPoolContents();
            for(sp<FatImageBufferHandle>& handle : poolContents)
            {
                // if not exist
                if(std::find(rImgVec.begin(), rImgVec.end(), handle->mImageBuffer.get())==rImgVec.end())
                    rImgVec.push_back(handle->mImageBuffer.get());
            }
        }
    }

    if(mBIDtoGraBufPoolMap_Scenario.indexOfKey(id) >= 0)
    {
        bRet = MTRUE;
        ScenarioToGraBufPoolMap bufMap = mBIDtoGraBufPoolMap_Scenario.valueFor(id);
        if(bufMap.indexOfKey(scen)>=0)
        {
            sp<GraphicBufferPool> pool = bufMap.valueFor(scen);
            GraphicBufferPool::CONTAINER_TYPE poolContents = pool->getPoolContents();
            for(sp<GraphicBufferHandle>& handle : poolContents)
            {
                // if not exist
                if(std::find(rImgVec.begin(), rImgVec.end(), handle->mImageBuffer.get())==rImgVec.end())
                    rImgVec.push_back(handle->mImageBuffer.get());
            }
        }
    }

    return bRet;
}

}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam

