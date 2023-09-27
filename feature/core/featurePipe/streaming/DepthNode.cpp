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

#include <bitset>
#include "DepthNode.h"
#include <mtkcam/drv/iopipe/CamIO/IHalCamIO.h>
#include <mtkcam3/feature/eis/eis_hal.h>
#include <mtkcam3/feature/DualCam/FOVHal.h>

#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapEffectRequest.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>

#include "NextIOUtil.h"

#define PIPE_CLASS_TAG "DepthNode"
#define PIPE_TRACE TRACE_SFP_DEPTH_NODE
#include <featurePipe/core/include/PipeLog.h>
//=======================================================================================
#if SUPPORT_VSDOF
//=======================================================================================
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_DEPTH);

#define ULOGMD_IF(cond, ...)       do { if ( (cond) ) { CAM_ULOGMD(__VA_ARGS__); } } while(0)
// using namespace NSFeaturePipe_DepthMap::IDepthMapEffectRequest;
using namespace NSCam::NSIoPipe::NSPostProc;
using NSCam::Feature::P2Util::P2SensorData;
using NSFeaturePipe_DepthMap::IDepthMapEffectRequest;
using NSCam::NSCamFeature::NSFeaturePipe::FeaturePipeParam;

DepthNode::DepthNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_DEPTH)
    , StreamingFeatureNode(name)
{
    CAM_ULOGM_APILIFE();
    this->addWaitQueue(&mRequests);
}

DepthNode::~DepthNode()
{
    CAM_ULOGM_APILIFE();
}

MBOOL DepthNode::onData(DataID id, const RequestPtr &data)
{
    CAM_ULOGM_APILIFE();
    TRACE_FUNC("Frame %d: %s arrived", data->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    switch ( id ) {
    case ID_ROOT_TO_DEPTH:
        mRequests.enque(data);
        ret = MTRUE;
        break;
    default:
        ret = MFALSE;
        break;
    }
    return ret;
}

NextIO::Policy DepthNode::getIOPolicy(const NextIO::ReqInfo& reqInfo) const
{
    NextIO::Policy policy;
    for(MUINT32 sensorID : reqInfo.mSensorIDs )
    {
        policy.enableAll(sensorID);
    }
    return policy;
}

MBOOL DepthNode::onInit()
{
    CAM_ULOGM_APILIFE();
    StreamingFeatureNode::onInit();
    //Check ISP_VER
    #if (SUPPORT_ISP_VER == 0)
        CAM_ULOGME("ISP_VER is unknown!! Abnormal!!");
        return MFALSE;
    #endif
    //
    /**
     * @brief DebugInk input, miShowDepthMap
     *      - 1 : Blur/Depth map
     *      - 2 : Disparity
     *      - 3 : Confidence
     */
    miShowDepthMap = property_get_int32("vendor.debug.tkflow.bokeh.showdepth", 0);
    miLogEnable    = property_get_int32("vendor.debug.vsdof.tkflow.depthnode", 0);
    if(miShowDepthMap >= eDEPTH_DEBUG_IMG_INVALID)
    {
        MY_LOGW("set invalid Debug Ink value. Cancel the Debug Motion");
        miShowDepthMap = 0;
    }
    // Depth output clear yuv + depth in ISP6S, need combine DRE
    #if (SUPPORT_ISP_VER == 65 && !SUPPORT_P2GP2_WITH_DEPTH)
        mbApplyDREFeature = initDreHal();
    #endif
    //depthmap pipe section
    std::vector<MUINT32> ids = mPipeUsage.getAllSensorIDs();
    sp<DepthMapPipeSetting> pPipeSetting = new DepthMapPipeSetting();
    pPipeSetting->miSensorIdx_Main1 = ids.at(MAINCAM);
    pPipeSetting->miSensorIdx_Main2 = ids.at(SUBCAM);
    //
    mSensorID.assign(ids.begin(), ids.end());
    // get main1 size by index.
    // main1 size usually put in first.
    pPipeSetting->mszRRZO_Main1 = mPipeUsage.getRrzoSizeByIndex(ids.at(MAINCAM));
    CAM_ULOGMD("SensorID:Main1(%d), Main2(%d)|"
               " RRZO_Main1 Create Size(%04dx%04d)|"
               " Assign:(Main1,Main2)(%d,%d)",
               ids.at(MAINCAM), ids.at(SUBCAM),
               pPipeSetting->mszRRZO_Main1.w, pPipeSetting->mszRRZO_Main1.h,
               mSensorID.at(MAINCAM), mSensorID.at(SUBCAM));
    //pipe option
    sp<DepthMapPipeOption> pPipeOption = new DepthMapPipeOption();
    pPipeOption->mSensorType     = (SeneorModuleType)mPipeUsage.getSensorModule();
    pPipeOption->mStereoScenario = (mPipeUsage.getVideoSize().w && mPipeUsage.getVideoSize().h)?
                                    eSTEREO_SCENARIO_RECORD : eSTEREO_SCENARIO_PREVIEW;
    pPipeOption->mDebugInk       = (DepthDebugImg)miShowDepthMap;
    // FlowType
    #if (SUPPORT_P2GP2_WITH_DEPTH)
        pPipeOption->mFlowType = StereoSettingProvider::isDepthDelayEnabled() ?
                                eDEPTH_FLOW_TYPE_QUEUED_DEPTH : eDEPTH_FLOW_TYPE_STANDARD;
    #else
        pPipeOption->mFlowType = eDEPTH_FLOW_TYPE_QUEUED_DEPTH;
    #endif
    // InputImgType
    #if (SUPPORT_ISP_VER < 60)
        pPipeOption->mInputImg = eDEPTH_INPUT_IMG_2RAW;
    #elif (SUPPORT_ISP_VER == 60)
        pPipeOption->mInputImg = eDEPTH_INPUT_IMG_2RAW1RECT;
    #elif (SUPPORT_ISP_VER == 65)
        #if (SUPPORT_P2GP2_WITH_DEPTH)
            pPipeOption->mInputImg = eDEPTH_INPUT_IMG_2YUVO_2RSSOR2;
        #else
            pPipeOption->mInputImg = eDEPTH_INPUT_IMG_2RRZO_2RSSOR2;
        #endif
    #else
        pPipeOption->mInputImg = eDEPTH_INPUT_IMG_UNDEFINED;
        MY_LOGE("not supported input image comination, check ISP version");
        return MFALSE;
    #endif
    if(mPipeUsage.supportDPE() && mPipeUsage.supportBokeh())
    {
        pPipeOption->mFeatureMode = eDEPTHNODE_MODE_VSDOF;
    }
    else if(mPipeUsage.supportDPE())
    {
        if(StereoSettingProvider::getDepthmapRefineLevel() == E_DEPTHMAP_REFINE_NONE)
            pPipeOption->mFeatureMode = eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH;
        else if(StereoSettingProvider::getDepthmapRefineLevel() == E_DEPTHMAP_REFINE_SW_OPTIMIZED)
            pPipeOption->mFeatureMode = eDEPTHNODE_MODE_MTK_SW_OPTIMIZED_DEPTH;
        else if(StereoSettingProvider::getDepthmapRefineLevel() == E_DEPTHMAP_REFINE_HOLE_FILLED)
            pPipeOption->mFeatureMode = eDEPTHNODE_MODE_MTK_HW_HOLEFILLED_DEPTH;
        else if((StereoSettingProvider::getDepthmapRefineLevel() == E_DEPTHMAP_REFINE_AI_DEPTH))
            pPipeOption->mFeatureMode = eDEPTHNODE_MODE_MTK_AI_DEPTH;
        else {
            MY_LOGE("undefined depth refine level (%d)",
                (int)StereoSettingProvider::getDepthmapRefineLevel());
        }
    }
    else
    {
        CAM_ULOGME("not support");
        return MFALSE;
    }
    // Depth FPS control
    int depthFPS = StereoSettingProvider::getDepthFreq(pPipeOption->mStereoScenario);
    if ( depthFPS )
        pPipeOption->setEnableDepthGenControl(MTRUE, depthFPS);
    else
        pPipeOption->setEnableDepthGenControl(MFALSE, 0);
    //
    #if (SUPPORT_ISP_VER >= 60)
        mpDepthMapPipe = DepthPipeHolder::createPipe(pPipeSetting, pPipeOption);
    #else
        mpDepthMapPipe = IDepthMapPipe::createInstance(pPipeSetting, pPipeOption);
        MBOOL bRet = mpDepthMapPipe->init();
        if (!bRet) {
            CAM_ULOGME("onInit Failure, ret=%d", bRet);
            return MFALSE;
        }
    #endif

    // Create Depth or DMBG buffer pool
    ENUM_STEREO_SCENARIO bufScenario = pPipeOption->mStereoScenario;
    if(mPipeUsage.supportBokeh())
    {
        mDMBGImgPoolAllocateNeed = 3;
        MSize dmbgSize = StereoSizeProvider::getInstance()->getBufferSize(E_DMBG, bufScenario);
        mDMBGImgPool = ImageBufferPool::create("fpipe.DMBGImg", dmbgSize.w, dmbgSize.h,
                                        eImgFmt_STA_BYTE, ImageBufferPool::USAGE_HW_AND_SW);
    }
    else
    {
        mDepthMapImgPoolAllocateNeed = 3;
        ENUM_BUFFER_NAME bufName;
        if ( pPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_AI_DEPTH )
            bufName = E_VAIDEPTH_DEPTHMAP;
        else
            bufName = E_DEPTH_MAP;
        MSize depthSize = StereoSizeProvider::getInstance()->getBufferSize(bufName, bufScenario);
        mDepthMapImgPool = ImageBufferPool::create("fpipe.DepthMapImg", depthSize.w, depthSize.h,
                                           eImgFmt_Y8, ImageBufferPool::USAGE_HW_AND_SW);
    }
    // Debug Image is not depth or blur map, need allocate extra buffer
    if(miShowDepthMap > eDEPTH_DEBUG_IMG_BLUR_MAP)
    {
        mDebugImgPoolAllocateNeed = 3;
        ENUM_BUFFER_NAME bufName;
        if(miShowDepthMap == eDEPTH_DEBUG_IMG_DISPARITY)
            bufName = (bufScenario == eSTEREO_SCENARIO_PREVIEW)?
                      E_DMW : E_VAIDEPTH_DEPTHMAP;
        else if(miShowDepthMap == eDEPTH_DEBUG_IMG_CONFIDENCE)
            bufName = E_CFM_M;
        else if(miShowDepthMap == eDEPTH_DEBUG_IMG_NOC)
            bufName = E_NOC;
        MSize debugSize = StereoSizeProvider::getInstance()->getBufferSize(bufName, bufScenario);
        mDebugImgPool = ImageBufferPool::create("fpipe.DebugImg", debugSize.w, debugSize.h,
                                            eImgFmt_Y8, ImageBufferPool::USAGE_HW_AND_SW);
    }
    // create Dre bufferPool
    if ( mbApplyDREFeature )
    {
        mDreTuningPoolAllocateNeed = 3;
        MUINT32 size = mDreTuningByteSize ? mDreTuningByteSize : 256;
        mDreTuningPool = TuningBufferPool::create("fpipe.depth.dreTuning", size,
                                                TuningBufferPool::BUF_PROTECT_RUN);
        MY_LOGD("DreTuningSize = %u bytes", size);
    }

    MY_LOGI("RunningMode:%s, IspVer:%d, ShowDepthMap:%d, StereoScenario:%s InputImgType:%d mFlowType:%d mbApplyDREFeature:%d",
            pPipeOption->mFeatureMode == eDEPTHNODE_MODE_VSDOF ? "ALL TK Flow" : "TK Depth+TP Bokeh",
            SUPPORT_ISP_VER, miShowDepthMap,
            pPipeOption->mStereoScenario == eSTEREO_SCENARIO_PREVIEW ? "PV" : "VR",
            pPipeOption->mInputImg,
            pPipeOption->mFlowType,
            mbApplyDREFeature);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL DepthNode::onUninit()
{
    CAM_ULOGM_APILIFE();
    //depthmap pipe section
    if (mpDepthMapPipe != nullptr) {
        if (0 != mvDepthNodePack.size()) {
            CAM_ULOGME("DepthNodePack should be release before uninit, size:%zu",
                    mvDepthNodePack.size());
            mvDepthNodePack.clear();
        }
        //
        #if (SUPPORT_ISP_VER < 60)
        {
            mpDepthMapPipe->uninit();
            delete mpDepthMapPipe;
            mpDepthMapPipe = nullptr;
        }
        #endif
    }
    //
    IBufferPool::destroy(mDMBGImgPool);
    IBufferPool::destroy(mDepthMapImgPool);
    if(mDebugImgPool)
        IBufferPool::destroy(mDebugImgPool);
    if(mDreTuningPool)
        TuningBufferPool::destroy(mDreTuningPool);
    //
    uninitDreHal();
    mpDepthMapPipe = nullptr;
    return MTRUE;
}

MVOID DepthNode::onFlush()
{
    CAM_ULOGM_APILIFE();
    mpDepthMapPipe->sync();

}

MBOOL DepthNode::onThreadStart()
{
    CAM_ULOGM_APILIFE();
    if( mYuvImgPoolAllocateNeed && mYuvImgPool != NULL )
    {
        Timer timer;
        timer.start();
        mYuvImgPool->allocate(mYuvImgPoolAllocateNeed);
        timer.stop();
        CAM_ULOGMD("mDepthYUVImg %s %d buf in %d ms", STR_ALLOCATE, mYuvImgPoolAllocateNeed,
                                                   timer.getElapsed());
    }

    if( mDMBGImgPoolAllocateNeed && mDMBGImgPool != NULL )
    {
        Timer timer;
        timer.start();
        mDMBGImgPool->allocate(mDMBGImgPoolAllocateNeed);
        timer.stop();
        CAM_ULOGMD("mDMBGImg %s %d buf in %d ms", STR_ALLOCATE, mDMBGImgPoolAllocateNeed,
                                               timer.getElapsed());
    }

    if( mDepthMapImgPoolAllocateNeed && mDepthMapImgPool != NULL )
    {
        Timer timer;
        timer.start();
        mDepthMapImgPool->allocate(mDepthMapImgPoolAllocateNeed);
        timer.stop();
        CAM_ULOGMD("mDepthMapImg %s %d buf in %d ms", STR_ALLOCATE, mDepthMapImgPoolAllocateNeed,
                                                   timer.getElapsed());
    }

    if(mDebugImgPoolAllocateNeed && mDebugImgPool != NULL)
    {
        Timer timer;
        timer.start();
        mDebugImgPool->allocate(mDebugImgPoolAllocateNeed);
        timer.stop();
        MY_LOGD("mDebugMapImg %s %d buf in %d ms", STR_ALLOCATE, mDebugImgPoolAllocateNeed,
                                                   timer.getElapsed());
    }

    if(mDreTuningPoolAllocateNeed && mDreTuningPool != NULL)
    {
        Timer timer;
        timer.start();
        mDreTuningPool->allocate(mDreTuningPoolAllocateNeed);
        timer.stop();
        MY_LOGD("mDreTuningPool %s %d buf in %d ms", STR_ALLOCATE, mDreTuningPoolAllocateNeed,
                                                   timer.getElapsed());
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL DepthNode::onThreadStop()
{
    CAM_ULOGM_APILIFE();
    return MTRUE;
}

MBOOL DepthNode::onThreadLoop()
{
    CAM_ULOGMD("Waitloop");
    RequestPtr request;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mRequests.deque(request) )
    {
        CAM_ULOGME("Request deque out of sync");
        return MFALSE;
    }
    else if( request == NULL )
    {
        CAM_ULOGME("Request out of sync");
        return MFALSE;
    }

    CAM_ULOGM_APILIFE();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    //
    MUINT32 decision = 0;
    request->mTimer.startDepth();
    markNodeEnter(request);
    DepthEnqueData enqueData;
    MINT64 timestamp = 0;
    enqueData.mRequest = request;
    enqueData.mTargetBitSet.reset();
    // Output : CleanYUV, FD, DMBG or DepthMap, App/Hal OutMeta
    SFPIOManager &ioMgr       = request->mSFPIOManager;
    const SFPIOMap &generalIO = ioMgr.getFirstGeneralIO();
    P2IO out, phyOut, appDepthOut;

    // create Request
    sp<IDepthMapEffectRequest> pDepMapReq = IDepthMapEffectRequest::createInstance(
                                                request->mRequestNo, onPipeReady, this);
    // InputSection :: RRZO, LCSO, HalIn, AppIn Meta
    {
        const SFPSensorInput &masterSensorIn = request->getSensorInput(request->mMasterID);
        const SFPSensorInput &slaveSensorIn  = request->getSensorInput(request->mSlaveID);
        if(get(masterSensorIn.mRRZO) != NULL)
        {
            timestamp = get(masterSensorIn.mRRZO)->getTimestamp();
        }
        //
        std::vector<MUINT32> ids = mPipeUsage.getAllSensorIDs();
        // DRE
        if ( mbApplyDREFeature ){
            TunBuffer dreTuning = mDreTuningPool->request();
            IAdvPQCtrl pqCtrl = AdvPQCtrl::clone(request->getBasicPQ(ids.at(MAINCAM)), "s_depth");
            pqCtrl->setDreTuning(dreTuning);
            enqueData.mYuvOut.mData.mPQCtrl = pqCtrl;
            pDepMapReq->extraData.pDreBuf   = pqCtrl->getDreTuning();
        }
        //Gather Main1 necessary input data
        decision = setInputData(ids.at(MAINCAM), pDepMapReq, masterSensorIn);
        if (decision != 0) {
            CAM_ULOGME("reqID=%d:In Main1  with ERROR index(%#x),Abnormal",
                      enqueData.mRequest->mRequestNo, decision);
            goto ABNORMAL;
        }
        //
        decision = setInputData(ids.at(SUBCAM), pDepMapReq, slaveSensorIn);
        if (decision != 0) {//Gather Main2 necessary data
            CAM_ULOGMW("reqID=%d:In Main2 with missing data, Abnormal",
                    enqueData.mRequest->mRequestNo);//Misin main2 data still can enque(stanalone)
            if (SUPPORT_ISP_VER < 50) {
                CAM_ULOGMW("VSDOF Version doesn't support Stanalone Mode, !!Not Enque!!");
                goto ABNORMAL;
            } else {
                decision = 0;//Still enque if missing Main2. Stanalone.
            }
        }
    }
    // OutputSection :: FD
#if (!SUPPORT_P2GP2_WITH_DEPTH)
    if (request->popFDOutput(this, out)) {
        pDepMapReq->pushRequestImageBuffer(
                    {PBID_OUT_FDIMG , eBUFFER_IOTYPE_OUTPUT}, out.mBuffer);
    }
#endif

    // Assume clean yuv no crop & scale
    enqueData.mYuvOut.mData.mSensorClipInfo = request->getSensorClipInfo(request->mMasterID);

    // set clear yuv output
    if (request->needPhysicalOutput(this, request->mMasterID))
    {
        if (request->popPhysicalOutput(this, request->mMasterID, phyOut)) {
            enqueData.mYuvOut.mData.mBuffer = new IIBuffer_IImageBuffer(phyOut.mBuffer);
        }
        else {
            MY_LOGE("failed to pop phiysical output");
        }
    }
    else if (request->needFullImg(this, request->mMasterID)) {
        enqueData.mYuvOut.mData.mBuffer = mYuvImgPool->requestIIBuffer(); //CleanYUV
    }
    else if (request->needNextFullImg(this, request->mMasterID))//customize input yuv for next node
    {

        MSize rrzo_size = get(request->getSensorInput(request->mMasterID).mRRZO)->getImgSize();
        NextIO::NextReq nexts = request->requestNextFullImg(this, request->mMasterID);
        size_t nextsNum = nexts.mList.size();
        NextIO::NextBuf firstNext = getFirst(nexts);
        enqueData.mYuvOut.mData.mBuffer = firstNext.mBuffer;//CleanYUV
        MSize &resize = firstNext.mAttr.mResize;
        if (resize.w && resize.h) {// if assigned
            enqueData.mYuvOut.mData.mBuffer->getImageBuffer()->setExtParam(resize);
            MRectF crop((float)rrzo_size.w, (float)rrzo_size.h);
            enqueData.mYuvOut.mData.accumulate(
                "depthNFull", request->mLog, rrzo_size, crop, resize);
        } else {//not assign. set output size as RRZO size
            if (rrzo_size.w && rrzo_size.h)
                enqueData.mYuvOut.mData.mBuffer->getImageBuffer()->setExtParam(rrzo_size);
        }
        CAM_ULOGMD("Prepare Size as %s:WxH(%04dx%04d)",
                   (resize.w && resize.h) ? "customize": "RRZO",
                   (resize.w && resize.h) ? resize.w: rrzo_size.w,
                   (resize.w && resize.h) ? resize.h: rrzo_size.h);
    }
    else if (request->needDisplayOutput(this) && request->popDisplayOutput(this, out))
    {
        enqueData.mYuvOut.mData.mBuffer = new IIBuffer_IImageBuffer(out.mBuffer);
    }
    else
    {
        #if (SUPPORT_ISP_VER < 65)
            CAM_ULOGME("**Exception Case** For CleanYuv");
        #endif
    }

    if(enqueData.mYuvOut.mData.mBuffer != NULL)
    {
        enqueData.mYuvOut.mData.mBuffer->getImageBufferPtr()->setTimestamp(timestamp);
    }
    // set debug output
    if(miShowDepthMap > eDEPTH_DEBUG_IMG_BLUR_MAP)
        enqueData.mDepthOut.mDebugImg = mDebugImgPool->requestIIBuffer();
    // set depth/DMBG output
    if (mPipeUsage.supportBokeh()) {
        enqueData.mDepthOut.mDMBGImg = mDMBGImgPool->requestIIBuffer();//DMBG)
    }
    else {
        if (request->popAppDepthOutput(this, appDepthOut)) {
            enqueData.mDepthOut.mDepthMapImg = new IIBuffer_IImageBuffer(appDepthOut.mBuffer);
        }
        else {
            enqueData.mDepthOut.mDepthMapImg = mDepthMapImgPool->requestIIBuffer();
        }
    }
    //
    if (request->hasGeneralOutput() || phyOut.isValid() || appDepthOut.isValid())//Gather Output
    {
        // TODO need put App Depth & physical yuv to depth pipe if needed
        decision = 0;
        decision = setOutputData(pDepMapReq, enqueData, generalIO);
        if (decision != 0) {
            CAM_ULOGME("reqID=%d:Output with ERROR index(%#x),Abnormal, !!Not Enque!!",
                    enqueData.mRequest->mRequestNo, decision);
            goto ABNORMAL;
        }
    } else {
        CAM_ULOGME("NOT GeneralOut,Error State(%#x)",(decision |= (1<<14)));//0x4000
        goto ABNORMAL;
    }
    //
    if(decision != 0)
        CAM_ULOGMD("reqID=%d,decision=%#x", enqueData.mRequest->mRequestNo, decision);
    //
ABNORMAL:
    if ( (request->hasGeneralOutput() || phyOut.isValid() || appDepthOut.isValid()) && decision == 0) {
        if (enqueData.mRequest->mRequestNo <= 0) {
            CAM_ULOGME("[DepthNode]reqID=%d released, something wrong !!",
                                        enqueData.mRequest->mRequestNo);
            return MFALSE;
        }
        else
        {
            //hold enqueData members,
            //because its member hold strong pointer of CleanYUV / DMBG / Depth buffers.
            this->incExtThreadDependency(); // record one request enque to depth pipe.

            android::Mutex::Autolock lock(mLock);
            CAM_ULOGMD("thread_loop reqID=%d timestamp(%ld)", enqueData.mRequest->mRequestNo, (long)timestamp);
            //For DepthNodePack, it consolidates DepthEnqueData & IDepthMapEffectRequest
            DepthNodePackage pack = {
                .depEnquePack  = make_shared<DepthEnqueData>(enqueData),
                .depEffectPack = pDepMapReq,
            };
            mvDepthNodePack.add(enqueData.mRequest->mRequestNo, pack);
        }
        enqueData.mRequest->mTimer.startEnqueDepth();
        mpDepthMapPipe->enque(pDepMapReq);
    } else { //AbnormalCase
        CAM_ULOGME("!!!Bypass DepthMapPipe!!! Something Wrong!");
        enqueData.mDepthOut.mDepthSucess = MFALSE;

        handleResultData(enqueData.mRequest, enqueData);// send data to next nodes
        //
        enqueData.mRequest->mTimer.stopEnqueDepth();
        //Not need to decExtThreadDependency due to this case is exception
        enqueData.mRequest->mTimer.stopDepth();
    }

    return MTRUE;
}

MUINT32 DepthNode::setOutputData(sp<IDepthMapEffectRequest> pDepMapReq,
                                 DepthEnqueData& enqueData, const SFPIOMap &generalIO)
{
    if(miLogEnable > 0)
        CAM_ULOGM_FUNCLIFE();
    std::bitset<14> decision;
    if(mPipeUsage.supportDepthP2())
    {
        if (enqueData.mYuvOut.mData.mBuffer.get() != nullptr) {
            sp<IImageBuffer> spCleanYuvImg = enqueData.mYuvOut.mData.mBuffer->getImageBuffer();
            if (!pDepMapReq->pushRequestImageBuffer(
                        {PBID_OUT_MV_F, eBUFFER_IOTYPE_OUTPUT}, spCleanYuvImg))
                decision.set(8); //0x100, 256
        }
        else
            decision.set(9);     //0x200, 512
    }

    if (mPipeUsage.supportBokeh())
    {
        if (enqueData.mDepthOut.mDMBGImg.get() != nullptr) {
            sp<IImageBuffer> spDMBGImg = enqueData.mDepthOut.mDMBGImg->getImageBuffer();
            if (!pDepMapReq->pushRequestImageBuffer(
                        {PBID_OUT_DMBG , eBUFFER_IOTYPE_OUTPUT}, spDMBGImg)) {
                decision.set(10);//0x400, 1024
            }
        }
    }
    else
    {
        if (enqueData.mDepthOut.mDepthMapImg.get() != nullptr) {
            sp<IImageBuffer> spDepthImg = enqueData.mDepthOut.mDepthMapImg->getImageBuffer();
            if (!pDepMapReq->pushRequestImageBuffer(
                        {PBID_OUT_DEPTHMAP, eBUFFER_IOTYPE_OUTPUT}, spDepthImg)) {
                decision.set(11);//0x800, 2048
            }
        }
    }
    //MetaData Output : use copy metadata to prevent conflicting from G_P2A overwriting
    //
    std::shared_ptr<IMetadata> pCopyHalOutMeta = std::make_shared<IMetadata>();
    if (!pDepMapReq->pushRequestMetadata(
                     {PBID_OUT_HAL_META, eBUFFER_IOTYPE_OUTPUT}, pCopyHalOutMeta.get()))
    {
        decision.set(12);//0x1000, 4096
    }
    enqueData.mDepthOut.mpCopyHalOut = pCopyHalOutMeta;
    //
    std::shared_ptr<IMetadata> pCopyAppOutMeta = std::make_shared<IMetadata>();
    if (!pDepMapReq->pushRequestMetadata(
                     {PBID_OUT_APP_META, eBUFFER_IOTYPE_OUTPUT}, pCopyAppOutMeta.get()))
    {
        decision.set(13);//0x2000, 8096
    }
    enqueData.mDepthOut.mpCopyAppOut = pCopyAppOutMeta;
    // TODO : Debug Output
    if(miShowDepthMap == eDEPTH_DEBUG_IMG_NONE || miShowDepthMap >= eDEPTH_DEBUG_IMG_INVALID)
        enqueData.mDepthOut.mDebugImg = nullptr;
    if(miShowDepthMap == eDEPTH_DEBUG_IMG_BLUR_MAP)
        enqueData.mDepthOut.mDebugImg = enqueData.mDepthOut.mDMBGImg;
    else if(miShowDepthMap > eDEPTH_DEBUG_IMG_BLUR_MAP)
    {
        sp<IImageBuffer> spDebugBuffer = nullptr;
        spDebugBuffer = enqueData.mDepthOut.mDebugImg->getImageBuffer();
        if(!pDepMapReq->pushRequestImageBuffer(
                {PBID_OUT_DEBUG, eBUFFER_IOTYPE_OUTPUT}, spDebugBuffer))
        {
            decision.set(14);//0x800, 2048
        }
    }

    if (true == decision.any())
        CAM_ULOGME("assign outout Data Error, index(%#lx)", decision.to_ulong());

    return decision.to_ulong();
}

MUINT16 DepthNode::fillIntoDepthMapPipe(sp<IDepthMapEffectRequest> pDepMapReq,
                            vector<inputImgData>& vImgs, vector<inputMetaData>& vMetas)
{
    MUINT16 i, decision = 0;
    if (miLogEnable > 0) CAM_ULOGM_FUNCLIFE();

    #define insertImg2DepthMapPipe(pDepMapReq, i) \
    if (!pDepMapReq->pushRequestImageBuffer( \
         {vImgs[i].param.bufferID, vImgs[i].param.ioType}, vImgs[i].buf)) { \
        decision |= (1<<(2*i)); \
    } else { \
        ULOGMD_IF(miLogEnable > 0, "InSertBuf_OK_bID:%u,ioType:%u", \
                vImgs[i].param.bufferID, vImgs[i].param.ioType);     \
    }
    #define insertMeta2DepthMapPipe(pDepMapReq, i) \
    if (!pDepMapReq->pushRequestMetadata( \
         {vMetas[i].param.bufferID, vMetas[i].param.ioType}, vMetas[i].meta)) { \
        decision |= (1<<(2*i+1)); \
    } else { \
        ULOGMD_IF(miLogEnable > 0, "InSertMeta_OK_bID:%u,ioType:%u", \
                vMetas[i].param.bufferID, vMetas[i].param.ioType);    \
    }
    //img buffer
    for (i = 0; i < vImgs.size(); i++)
        insertImg2DepthMapPipe(pDepMapReq, i);
    //Metadata
    for (i = 0; i < vMetas.size(); i++)
        insertMeta2DepthMapPipe(pDepMapReq, i);
    //
    #undef insertImg2DepthMapPipe
    #undef insertMeta2DepthMapPipe

    return decision;
}

MUINT32 DepthNode::setInputData(MUINT32 sensorID,
        sp<IDepthMapEffectRequest> pDepMapReq, const SFPSensorInput &data)
{
    if (miLogEnable > 0) {
        CAM_ULOGMD("%s ispVer:%d",
               sensorID == mSensorID.at(MAINCAM) ? "main1" : "main2", SUPPORT_ISP_VER);
    }

    MUINT32 decision = 0;

    vector<inputImgData>  vImgs_main1 {
    #if (SUPPORT_ISP_VER < 65)
        {{PBID_IN_RSRAW1, eBUFFER_IOTYPE_INPUT}, get(data.mRRZO)},  // MAIN1 RSRAW
        {{PBID_IN_LCSO1 , eBUFFER_IOTYPE_INPUT}, get(data.mLCSO)},
    #else
        #if (SUPPORT_P2GP2_WITH_DEPTH)
            {{PBID_IN_YUV1, eBUFFER_IOTYPE_INPUT}, get(data.mRRZO)},  // MAIN1 YUV
        #else
            {{PBID_IN_RSRAW1, eBUFFER_IOTYPE_INPUT}, get(data.mRRZO)},  // MAIN1 RSRAW
            {{PBID_IN_LCSO1 , eBUFFER_IOTYPE_INPUT}, get(data.mLCSO)},
            {{PBID_IN_LCSHO1, eBUFFER_IOTYPE_INPUT}, get(data.mLCSHO)},
        #endif
    #endif
    // ISP60 & ISP6S : RECT_IN1
    #if (SUPPORT_ISP_VER == 60)
        {{PBID_IN_P1YUV1, eBUFFER_IOTYPE_INPUT}, get(data.mRrzYuv2)},
    #elif (SUPPORT_ISP_VER == 65)
        {{PBID_IN_P1YUV1, eBUFFER_IOTYPE_INPUT}, get(data.mRSSOR2)},
    #endif
    };
    vector<inputMetaData> vMetas_main1 {
        {{PBID_IN_APP_META      , eBUFFER_IOTYPE_INPUT}, data.mAppIn},
        {{PBID_IN_HAL_META_MAIN1, eBUFFER_IOTYPE_INPUT}, data.mHalIn},
        {{PBID_IN_P1_RETURN_META, eBUFFER_IOTYPE_INPUT}, data.mAppDynamicIn},
    };
    vector<inputImgData>  vImgs_main2 {
    #if (SUPPORT_ISP_VER < 65)
        {{PBID_IN_RSRAW2, eBUFFER_IOTYPE_INPUT}, get(data.mRRZO)},  // MAIN2 RSRAW
        {{PBID_IN_LCSO2 , eBUFFER_IOTYPE_INPUT}, get(data.mLCSO)},
    #else
        #if (SUPPORT_P2GP2_WITH_DEPTH)
            {{PBID_IN_YUV2, eBUFFER_IOTYPE_INPUT}, get(data.mRRZO)},  // MAIN2 YUV
        #else
            {{PBID_IN_RSRAW2, eBUFFER_IOTYPE_INPUT}, get(data.mRRZO)},  // MAIN2 RSRAW
            {{PBID_IN_LCSO2 , eBUFFER_IOTYPE_INPUT}, get(data.mLCSO)},
        #endif
    #endif
    // ISP6S : RECT_IN2
    #if (SUPPORT_ISP_VER == 65)
        {{PBID_IN_P1YUV2, eBUFFER_IOTYPE_INPUT}, get(data.mRSSOR2)},
    #endif
    };
    vector<inputMetaData> vMetas_main2 {
        {{PBID_IN_HAL_META_MAIN2, eBUFFER_IOTYPE_INPUT}, data.mHalIn},
    };

    if (sensorID == mSensorID.at(MAINCAM)) {    //main1
        decision = prepareMain1Data(data);
    } else {                //main2
        decision = prepareMain2Data(data);
    }
    if (decision != 0) {
        CAM_ULOGME("ispver:%d, CAM[%d]miss input index(%#x)", SUPPORT_ISP_VER, sensorID, decision);
        return decision;
    }

    if(miLogEnable > 0)
        CAM_ULOGMD("%s:img_Size:%zu, meta_Size:%zu",
           (sensorID == mSensorID.at(MAINCAM) ? "Main1" : "Main2"),
           (sensorID == mSensorID.at(MAINCAM) ? vImgs_main1.size()  : vImgs_main2.size()),
           (sensorID == mSensorID.at(MAINCAM) ? vMetas_main1.size() : vMetas_main2.size()));

    decision = fillIntoDepthMapPipe(pDepMapReq,
                        (sensorID == mSensorID.at(MAINCAM)) ? vImgs_main1 : vImgs_main2,
                        (sensorID == mSensorID.at(MAINCAM)) ? vMetas_main1: vMetas_main2);
    if (decision > 0) {
        CAM_ULOGME("Main%d fillInDepthMap Data Error, index(%#08x)",
               (sensorID == mSensorID.at(MAINCAM) ? 1: 2), decision);
    }

    return decision;
}

MUINT32 DepthNode::prepareMain1Data(const SFPSensorInput &data)
{
    std::bitset<7> decision;

    #define assertInput(input, idx) \
        if (input == nullptr) {decision.set(idx);}

    assertInput(get(data.mRRZO)   , 0);//0x01
    #if(SUPPORT_ISP_VER < 65)
        assertInput(get(data.mLCSO)   , 1);//0x02
    #endif
    assertInput(data.mAppIn       , 2);//0x04
    assertInput(data.mHalIn       , 3);//0x08
    assertInput(data.mAppDynamicIn, 4);//0x10

    #if (SUPPORT_ISP_VER == 60)
        assertInput(get(data.mRrzYuv2), 5);//0x20
    #elif (SUPPORT_ISP_VER == 65)
        assertInput(get(data.mRSSOR2), 5);//0x20
        #if (!SUPPORT_P2GP2_WITH_DEPTH)
            assertInput(get(data.mLCSO)   , 1);//0x02
            assertInput(get(data.mLCSHO)  , 6);//0x40
        #endif
        /*
         * RrzYuv1 stand for P1 CRZO1
         * RrzYuv2 stand for P1 CRZO2
         * P1 has 2 sensor, with CRZO1 & CRZO2 respectly
         */
    #endif
    #undef assertInput
    //
    if (decision.test(1)) {//LCSO missing is acceptable
        decision.reset(1);
        CAM_ULOGMW("%s,decision:%lx, input LCSO might miss, but still enque", __func__,
                decision.to_ulong());
    }
    return decision.to_ulong();
}

MUINT32 DepthNode::prepareMain2Data(const SFPSensorInput &data)
{
    std::bitset<4> decision;

    #define assertInput(input, idx) \
        if (input == nullptr) {decision.set(idx);}

    assertInput(get(data.mRRZO), 0);    //0x01
    #if(SUPPORT_ISP_VER < 65)
        assertInput(get(data.mLCSO), 1);    //0x02
    #endif
    assertInput(data.mHalIn, 2);    //0x04
    #if (SUPPORT_ISP_VER == 65)
        assertInput(get(data.mRSSOR2), 3); //0x08
    #endif
    #undef assertInput

    return decision.to_ulong();
}

IImageBuffer* DepthNode::createImageBufferFromFile(
    const IImageBufferAllocator::ImgParam imgParam,
    const char* path,
    const char* name,
    MINT usage
)
{
    IImageBufferAllocator *allocator = IImageBufferAllocator::getInstance();
    IImageBuffer* pImgBuf = allocator->alloc(name, imgParam);

    if(pImgBuf == nullptr)
    {
        return nullptr;
    }

    if(!pImgBuf->loadFromFile(path))
    {
        MY_LOGW("Failed to load image from file");
    }
    pImgBuf->lockBuf(name, usage);
    return pImgBuf;
}

MVOID DepthNode::setOutputBufferPool(const android::sp<IBufferPool> &pool, MUINT32 allocate)
{
    CAM_ULOGM_APILIFE();
    mYuvImgPool = pool;
    mYuvImgPoolAllocateNeed = allocate;
}

void DepthNode::updateMetadata(const RequestPtr &request,
                               const SFPSensorInput& input, const SFPIOMap& output)
{
    if (input.mHalIn == nullptr)
        return ;

    *(output.mHalOut)  += *(input.mHalIn);

    if (request->hasFDOutput()) {
        MRect fdCrop{0};
        VarMap<SFP_VAR> &varMap = request->getSensorVarMap(mSensorID.at(MAINCAM));
        if (varMap.tryGet<MRect>(SFP_VAR::FD_CROP_ACTIVE_REGION, fdCrop)) {
            ULOGMD_IF(miLogEnable > 0, "fd info: x:%d,y:%d, w:%d, h:%d",
                    fdCrop.p.x, fdCrop.p.y, fdCrop.s.w,fdCrop.s.h);
            IMetadata::setEntry<MRect>(output.mHalOut, MTK_P2NODE_FD_CROP_REGION, fdCrop);
        } else {
            CAM_ULOGMW("get VAR_FD_CROP_ACTIVE_REGION with problem!");
        }
    }
    if(miLogEnable > 0)
        CAM_ULOGMD("_inHal:%d, _outHal:%d", input.mHalIn->count(), output.mHalOut->count());
}

MVOID DepthNode::handleResultData(const RequestPtr &request, const DepthEnqueData &data)
{
    CAM_ULOGM_FUNCLIFE();

    markNodeExit(request);
    if(mPipeUsage.supportBokeh())
    {
        updateMetadata(request, request->getSensorInput(request->mMasterID),
                        request->mSFPIOManager.getFirstGeneralIO());

        handleData(ID_DEPTH_TO_BOKEH, DepthImgData(data.mDepthOut, request));
        if(mPipeUsage.supportDepthP2())
            handleData(ID_DEPTH_P2_TO_BOKEH, BasicImgData(data.mYuvOut.mData, request));
    }
    else
    {
        updateMetadata(request, request->getSensorInput(request->mMasterID),
                        request->mSFPIOManager.getFirstGeneralIO());

        handleData(ID_DEPTH_TO_VENDOR, DepthImgData(data.mDepthOut, request));
        if(mPipeUsage.supportDepthP2())
            handleData(ID_DEPTH_P2_TO_VENDOR, BasicImgData(data.mYuvOut.mData, request));
    }
    //
    if (request->needDump()) {
        if (data.mYuvOut.mData.mBuffer != nullptr) {
            data.mYuvOut.mData.mBuffer->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
            dumpData(data.mRequest, data.mYuvOut.mData.mBuffer->getImageBufferPtr(),
                    "DepthNode_yuv");
        }
        if (mPipeUsage.supportDPE()) {
            if (data.mDepthOut.mDepthMapImg != nullptr) {
                data.mDepthOut.mDepthMapImg->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mDepthOut.mDepthMapImg->getImageBufferPtr(),
                        "DepthNode_depthmap");
            }
        } else if(mPipeUsage.supportBokeh()) {
            if (data.mDepthOut.mDMBGImg != nullptr) {
                data.mDepthOut.mDMBGImg->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mDepthOut.mDMBGImg->getImageBufferPtr(),
                        "DepthNode_blurmap");
            }
        }
    }
    //
    if (request->needNddDump()) {
        char filename[1024] = {0};
        TuningUtils::FILE_DUMP_NAMING_HINT hint =
                            request->mP2Pack.getSensorData(request->mMasterID).mNDDHint;
        if (data.mYuvOut.mData.mBuffer != nullptr) {
            int err = snprintf(filename, 1024, "%s_%lu_reqID_%d", "BID_P2A_OUT_MV_F",
                    data.mYuvOut.mData.mBuffer->getImageBufferPtr()->getBufStridesInBytes(0),
                    request->mRequestNo);
            data.mYuvOut.mData.mBuffer->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
            dumpNddData(hint, data.mYuvOut.mData.mBuffer->getImageBufferPtr(),
                        TuningUtils::YUV_PORT_UNDEFINED, filename);
        }
    }
    //
    // if (request->needDisplayOutput(this))
    // {
        request->updateResult(true);
        // CAM_ULOGMD("DepthNode w/o cascade anything, buffer to preview directly");
    // }
    //
}

MVOID DepthNode::onPipeReady(MVOID *tag,
                             NSDualFeature::ResultState state,
                             sp<IDualFeatureRequest> &request) {
    CAM_ULOGM_FUNCLIFE();
    std::shared_ptr<DepthEnqueData> pEnqueData;
    DepthNodePackage depthNodePack;
    DepthNode       *pDepthNode = (DepthNode*)tag;
    sp<IDepthMapEffectRequest> pDepReq = (IDepthMapEffectRequest*) request.get();

    TRACE_FUNC("DepthPipeItem reqID=%d state=%d:%s",
               pDepReq->getRequestNo(), state, ResultState2Name(state));
    ssize_t idx = -1;
    {
        android::Mutex::Autolock lock(pDepthNode->mLock);
        idx = pDepthNode->mvDepthNodePack.indexOfKey(pDepReq->getRequestNo());
        if (idx < 0) {
            CAM_ULOGME("[DepthNode]idx=%zu,reqID=%u is missing, might released!!",
                    idx, pDepReq->getRequestNo());
            return;
        }
        depthNodePack = pDepthNode->mvDepthNodePack.valueAt(idx);
        pEnqueData    = depthNodePack.depEnquePack;

        // if complete/not_ready -> enque to next item
        if (state == eRESULT_COMPLETE ||
            state == eRESULT_DEPTH_NOT_READY ||
            state == eRESULT_FLUSH ||
            state == eRESULT_SKIP_DEPTH) {
            pEnqueData->mDepthOut.mDepthSucess = MTRUE;
            // for 3rd party, set depth img to null
            if (state == eRESULT_SKIP_DEPTH)
                pEnqueData->mDepthOut.mDepthMapImg = nullptr;
            // send data to next node
            pDepthNode->handleResultData(pEnqueData->mRequest, *(pEnqueData.get()));

            pEnqueData->mRequest->mTimer.stopEnqueDepth();
            pDepthNode->decExtThreadDependency(); // tell one request call back
            pEnqueData->mTargetBitSet.set(eTARGET_DEPTH_OUT); // set depth out flag to true

            // additional IspVer check to prevent side effect from
            // Release Hal Buffer flow (Supported IspVer >= 65)
            if (SUPPORT_ISP_VER < 65) {
                CAM_ULOGMD("reqID(%u), IspVer(%d) not support Release Hal Buffer Flow!",
                           pDepReq->getRequestNo(), SUPPORT_ISP_VER);
                pDepthNode->handleData(ID_DEPTH_TO_HELPER,
                                HelperData(HelperMsg::HMSG_YUVO_DONE, pEnqueData->mRequest));
                pEnqueData->mTargetBitSet.set(eTARGET_BUFFER_RELEASE);
            }
        } else if (state == eRESULT_BUF_RELEASED) {
            pDepthNode->handleData(ID_DEPTH_TO_HELPER,
                                HelperData(HelperMsg::HMSG_YUVO_DONE, pEnqueData->mRequest));
            pEnqueData->mTargetBitSet.set(eTARGET_BUFFER_RELEASE); // set buffer release flag to true
        }
        CAM_ULOGMD("reqID=%d, state=%s, targetBitSet=%lx", pDepReq->getRequestNo(),
                   ResultState2Name(state), pEnqueData->mTargetBitSet.to_ulong());
        // remove the package when depth out flag and buffer release flag are set
        if (pEnqueData->mTargetBitSet.all()) {
            CAM_ULOGMD("Remove reqID=%d, mvDepEffReq size=%zu",
                    pDepReq->getRequestNo(), pDepthNode->mvDepthNodePack.size());
            pDepthNode->mvDepthNodePack.removeItem(pDepReq->getRequestNo());
            pEnqueData->mRequest->mTimer.stopDepth();
        }
    } // end lock
}

MBOOL
DepthNode::initDreHal()
{
    TRACE_FUNC_ENTER();
    MBOOL hasIspBufferInfo = MFALSE;
    P2G::SensorContextPtr ctx;
    NS3Av3::Buffer_Info ISPBufferInfo;
    mSensorContextMap = make_shared<P2G::SensorContextMap>();
    mSensorContextMap->init(mPipeUsage.getAllSensorIDs());
    ctx = mSensorContextMap->find(mPipeUsage.getSensorIndex());

    if( ctx && ctx->mHalISP )
    {
        hasIspBufferInfo = ctx->mHalISP->queryISPBufferInfo(ISPBufferInfo);
    }
    // init DreHal
    mDreHal.init(hasIspBufferInfo, ISPBufferInfo);
    if( mDreHal.isSupport() )
    {
        mDreTuningByteSize = mDreHal.getTuningByteSize();
    }
    if ( mDreTuningByteSize == 0) {
        MY_LOGE("failed to query DreTuning size, init DreHal failed");
        return MFALSE;
    }
    return MTRUE;
    TRACE_FUNC_EXIT();
}

MVOID
DepthNode::uninitDreHal()
{
    TRACE_FUNC_ENTER();
    mDreHal.uninit();
    if ( mSensorContextMap )
    {
        mSensorContextMap->uninit();
        mSensorContextMap = nullptr;
    }
    TRACE_FUNC_EXIT();
}


} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

//=======================================================================================
#else //SUPPORT_VSDO
//=======================================================================================

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
//=======================================================================================
#endif //SUPPORT_VSDOF
//=======================================================================================

