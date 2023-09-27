
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

#include "BokehNode.h"

#include <mtkcam3/feature/stereo/hal/StereoArea.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <isp_tuning.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>

#include <stereo_tuning_provider.h>

#define PIPE_CLASS_TAG "BokehNode"
#define PIPE_TRACE TRACE_SFP_BOKEH_NODE
#include <featurePipe/core/include/PipeLog.h>

#include <mtkcam3/feature/utils/p2/P2Util.h>
#include "NextIOUtil.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_BOKEH);
#define ULOGMD_IF(cond, ...)       do { if ( (cond) ) { CAM_ULOGMD(__VA_ARGS__); } } while(0)

#define BOKEH_NORMAL_STREAM_NAME "SFP_Bokeh"
#define PIPE_LOG_TAG "BokehNode"

using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSIspTuning;

class DpBlitStream;
//=======================================================================================
#if SUPPORT_VSDOF
//=======================================================================================
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

using namespace StereoHAL;
using namespace NSFeaturePipe_DepthMap;

BokehNode::BokehNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_BOKEH)
    , StreamingFeatureNode(name)
    , mDIPStream(nullptr)
    , ColorizedDepthUtil()
{
    CAM_ULOGM_APILIFE();
    this->addWaitQueue(&mDepthDatas);
    this->addWaitQueue(&mCleanYuvDatas);
}

BokehNode::~BokehNode()
{
    CAM_ULOGM_APILIFE();
}

MBOOL BokehNode::onData(DataID id, const DepthImgData &data)
{
    CAM_ULOGM_APILIFE();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    switch (id) {
    case ID_DEPTH_TO_BOKEH:
        mDepthDatas.enque(data);
        ret = MTRUE;
        break;
    default:
        ret = MFALSE;
        break;
    }
    return ret;
}

MBOOL BokehNode::onData(DataID id, const BasicImgData &data)
{
    CAM_ULOGM_APILIFE();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;

    switch (id) {
    case ID_P2G_TO_BOKEH:
    case ID_DEPTH_P2_TO_BOKEH:
        mCleanYuvDatas.enque(data);
        ret = MTRUE;
        break;
    default:
        ret = MFALSE;
        break;
    }
    return ret;
}

NextIO::Policy BokehNode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    NextIO::Policy policy;
    if( reqInfo.hasMaster() )
    {
        policy.enableAll(reqInfo.mMasterID);
    }
    return policy;
}

MBOOL BokehNode::onInit()
{
    CAM_ULOGM_APILIFE();
    StreamingFeatureNode::onInit();
    mDynamicTuningPool = TuningBufferPool::create("bokeh.p2btuningBuf",
                                Feature::P2Util::DIPStream::getRegTableSize(),
                                TuningBufferPool::BUF_PROTECT_RUN);

    mWorkingScenario = (mPipeUsage.getVideoSize().w && mPipeUsage.getVideoSize().h) ?
                        eBOKEH_RECORD : eBOKEH_PREVIEW;
    // create instance
    mpDpIspStream = new DpIspStream(DpIspStream::ISP_ZSD_STREAM);
    mDIPStream    = Feature::P2Util::DIPStream::createInstance(mSensorIndex);
    if( mDIPStream == nullptr || !mDIPStream->init(BOKEH_NORMAL_STREAM_NAME))
    {
        CAM_ULOGME("BokehNode Init DIPStream Failed ! DIPStream(%p)", mDIPStream);
    }
    MY_LOGD("StreamingSize=(%dx%d), VideoSize=(%dx%d)",
            mPipeUsage.getStreamingSize().w, mPipeUsage.getStreamingSize().h,
            mPipeUsage.getVideoSize().w, mPipeUsage.getVideoSize().h);
    /**
     * @brief ShowDepthMap input
     *      - 1 : Blur/Depth map
     *      - 2 : Disparity
     *      - 3 : Confidence
     */
    // adb command
    mbShowDepthMap     = property_get_int32("vendor.debug.tkflow.bokeh.showdepth" , 0);
    mbDumpImgeBuf      = property_get_int32("vendor.debug.tkflow.bokeh.dump_img"  , 0);
    mbPipeLogEnable    = property_get_int32("vendor.debug.tkflow.bokeh.log"       , 0);
    mbDumpDIPParam     = property_get_int32("vendor.debug.tkflow.bokeh.dipparam"  , 0);
    mbDebugColor       = property_get_int32("vendor.debug.demo.color"             , 0);
    miDumpBufSize      = property_get_int32("vendor.dualfeature.dump.size"        , 0);
    miDumpStartIdx     = property_get_int32("vendor.dualfeature.dump.start"       , 0);
    // invalidate debugInk command
    if(mbShowDepthMap >= eDEPTH_DEBUG_IMG_INVALID)
    {
        MY_LOGW("set invalid Debug Ink value. Cancel the Debug Motion");
        mbShowDepthMap = 0;
    }
    // var by platform
#if(SUPPORT_ISP_VER == 65)
    if( mWorkingScenario == eBOKEH_PREVIEW ||
        (mWorkingScenario == eBOKEH_RECORD && !mPipeUsage.supportWarpNode()) )
        mbApplyMDPFeature = MTRUE;
    else
        mbApplyMDPFeature = MFALSE;
    mbApplyBlending = StereoSettingProvider::isBokehBlendingEnabled(mWorkingScenario == eBOKEH_PREVIEW?
                                                                    eSTEREO_SCENARIO_PREVIEW : eSTEREO_SCENARIO_RECORD);
#else
    mbApplyMDPFeature = MFALSE;
    mbApplyBlending   = MFALSE;
#endif

    extract_by_SensorOpenId(&mDumpHint, mSensorIndex);//Need to confirm with Jou-Feng or Ray

    // create Job Queue for handing SW blending (and MDP Feature if needed)
    mJobQueue = std::make_shared<NSCam::JobQueue<void()>>(LOG_TAG);

    CAM_ULOGMD("openid(%#x), Scenario(%d), EIS(%d), ShowDepthMap(%d), DumpImgBuf(%d), SwBlending(%d), MdpFeature(%d)",
                mSensorIndex, mWorkingScenario, mPipeUsage.supportWarpNode(), mbShowDepthMap, mbDumpImgeBuf, mbApplyBlending, mbApplyMDPFeature);

    return MTRUE;
}

MBOOL BokehNode::onUninit()
{
    CAM_ULOGM_APILIFE();
    if( mDIPStream )
    {
        mDIPStream->uninit(BOKEH_NORMAL_STREAM_NAME);
        mDIPStream->destroyInstance();
        mDIPStream = nullptr;
    }
    TuningBufferPool::destroy(mDynamicTuningPool);
    if(mpDepthMapBufPool)
        ImageBufferPool::destroy(mpDepthMapBufPool);
    if(mpBokehYUVBufPool)
        ImageBufferPool::destroy(mpBokehYUVBufPool);
    if(mpBlendingBufPool)
        ImageBufferPool::destroy(mpBlendingBufPool);
    //
    if (mpDpStream != nullptr)
        delete mpDpStream;
    if(mpDpIspStream != nullptr)
        delete mpDpIspStream;
    if(mpBlendingHal != nullptr)
        delete mpBlendingHal;
    // 3A
    if (mpIspHal_Main1) {
        mpIspHal_Main1->destroyInstance("BOKEH_3A_MAIN1");
        mpIspHal_Main1 = nullptr;
    }
    if (mp3AHal_Main1) {
        mp3AHal_Main1->destroyInstance("BOKEH_3A_MAIN1");
        mp3AHal_Main1 = nullptr;
    }
    // Job Queue
    if ( mJobQueue ) {
        mJobQueue->flush();
        mJobQueue->waitJobsDone();
    }

    return MTRUE;
}

MBOOL BokehNode::onThreadStart()
{
    CAM_ULOGM_APILIFE();
    if( mFullImgPoolAllocateNeed && mFullImgPool != nullptr )
    {
        Timer timer;
        timer.start();
        mFullImgPool->allocate(mFullImgPoolAllocateNeed);
        timer.stop();
        CAM_ULOGMD("Bokeh FullImg %s %d buf in %d ms", STR_ALLOCATE, mFullImgPoolAllocateNeed,
                timer.getElapsed());
    }
    //Set depi size
    ENUM_STEREO_SCENARIO scenario = (mWorkingScenario == eBOKEH_PREVIEW)?
                                    eSTEREO_SCENARIO_PREVIEW : eSTEREO_SCENARIO_RECORD;
    msDmgi = StereoSizeProvider::getInstance()->getBufferSize(
                                                E_DMG, scenario).size;
    // set streamingSize
    MSize mStreamingSize = mPipeUsage.getStreamingSize();
    CAM_ULOGMD("DMGI size(%dx%d) ", msDmgi.w, msDmgi.h);
    //
    if (mDynamicTuningPool != nullptr )
    {
        Timer timer(MTRUE);
        mDynamicTuningPool->allocate(mPipeUsage.getNumP2ATuning());
        timer.stop();
        CAM_ULOGMD("Dynamic Tuning %s %d bufs in %d ms",
                STR_ALLOCATE, mPipeUsage.getNumP2ATuning(), timer.getElapsed());
    }
    //
    if (mbShowDepthMap) {
        Timer timer(MTRUE);
        const MINT32 usage = ImageBufferPool::USAGE_HW;
        createBufferPool(mpDepthMapBufPool, 720, 408, eImgFmt_Y8, 2,
                        "BOKEH_P2B_DEPTHMAP_BUF", usage, MTRUE);
        mpDepthMapBufPool->allocate(2);
        timer.stop();
        CAM_ULOGMD("Debug Ink %s %d buf in %d ms", STR_ALLOCATE, 2,
                timer.getElapsed());
        mpDpStream = new DpBlitStream();
    }
    //
    if (mbApplyBlending){
        // create BLENDING_HAL instance
        mpBlendingHal = BLENDING_HAL::createInstance(mWorkingScenario == eBOKEH_PREVIEW?
                                                eSTEREO_SCENARIO_PREVIEW : eSTEREO_SCENARIO_RECORD);
        Timer timer(MTRUE);
        const MINT32 usage = ImageBufferPool::USAGE_HW_AND_SW;
        createBufferPool(mpBokehYUVBufPool,
                        mStreamingSize.w, mStreamingSize.h, mPipeUsage.getFullImgFormat(), 2,
                        "BOKEH_P2B_YUV_OUT_BUF", usage, MTRUE);
        mpBokehYUVBufPool->allocate(2);
        timer.stop();
        CAM_ULOGMD("Internal P2Out %s 2 buf in %d ms, format=%d", STR_ALLOCATE,
                timer.getElapsed(), mpBokehYUVBufPool->getImageFormat());
    }
    //
    if (mbApplyMDPFeature && mbApplyBlending) {
        Timer timer(MTRUE);
        const MINT32 usage = ImageBufferPool::USAGE_HW_AND_SW;
        createBufferPool(mpBlendingBufPool,
                        mStreamingSize.w, mStreamingSize.h, mPipeUsage.getFullImgFormat(), 2,
                        "BOKEH_BLENDING_YUV_OUT_BUF", usage, MTRUE);
        mpBlendingBufPool->allocate(2);
        timer.stop();
        CAM_ULOGMD("Internal Blending %s 2 buf in %d ms, format=%d", STR_ALLOCATE,
                timer.getElapsed(), mpBlendingBufPool->getImageFormat());
    }
    //
    #if (SUPPORT_ISP_VER >= 60)
        mpIspHal_Main1 = MAKE_HalISP(mSensorIndex, "BOKEH_3A_MAIN1");
    #else
        mp3AHal_Main1 = MAKE_Hal3A(mSensorIndex, "BOKEH_3A_MAIN1");
    #endif
    CAM_ULOGMD("3A crate Instance, Main1:%p", mp3AHal_Main1);


    return MTRUE;
}

MBOOL BokehNode::onThreadStop()
{
    CAM_ULOGM_APILIFE();
    this->waitDIPStreamBaseDone();

    return MTRUE;
}

MBOOL BokehNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    DepthImgData depthData;
    BasicImgData cleanYuvData;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mDepthDatas.deque(depthData) )
    {
        CAM_ULOGME("DepthImgData deque out of sync");
        return MFALSE;
    }
    if( !mCleanYuvDatas.deque(cleanYuvData) )
    {
        CAM_ULOGME("DSDNData deque out of sync");
        return MFALSE;
    }
    if(depthData.mRequest != cleanYuvData.mRequest)
    {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    CAM_ULOGM_APILIFE();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, depthData.mRequest);

    depthData.mRequest->mTimer.startBokeh();
    markNodeEnter(depthData.mRequest);
    BokehEnqueData enqueData;
    Feature::P2Util::DIPParams enqueParams;
    // Check Bokeh Input & hold strong reference
    CAM_ULOGMD("BokehNode: reqID=%d ", depthData.mRequest->mRequestNo);
    // query buffer result
    enqueData.mRequest  = cleanYuvData.mRequest;
    enqueData.mInYuvImg = cleanYuvData.mData;
    enqueData.mDMBG     = depthData.mData.mDMBGImg;
    enqueData.mDebugImg = depthData.mData.mDebugImg;
    // append DepthNode's output hal/app meta
    const SFPIOMap &generalIO = enqueData.mRequest->mSFPIOManager.getFirstGeneralIO();
    if(generalIO.mHalOut != nullptr && depthData.mData.mpCopyHalOut != nullptr)
    {
        *(generalIO.mHalOut) += *(depthData.mData.mpCopyHalOut);
    }
    else
        MY_LOGW("depth request, HAL OUT META is null, may lose some info");
    if(generalIO.mAppOut != nullptr && depthData.mData.mpCopyAppOut != nullptr)
    {
        *(generalIO.mAppOut) += *(depthData.mData.mpCopyAppOut);
    }
    else
        MY_LOGW("depth request, App OUT META is null, may lose some info");
    //
    if (enqueData.mRequest->hasGeneralOutput() &&
        depthData.mData.mDepthSucess == MTRUE)//NonmalCase
    {
        //Gather Input Buffer
        if (cleanYuvData.mData.mBuffer == nullptr) {
            CAM_ULOGME("Bokeh input Master YUV is nullptr!  Can not enque P2!");
            return MFALSE;
        }
        if(depthData.mData.mDMBGImg == nullptr) {
            CAM_ULOGME("Bokeh input DMBG is nullptr!  Can not enque P2!");
            return MFALSE;
        }
        // Prepare DIPParam for P2
        enqueParams.mvDIPFrameParams.push_back(Feature::P2Util::DIPFrameParams());
        enqueParams.mpCookie = (void*)&enqueData;

        Feature::P2Util::DIPFrameParams& mainFrame = enqueParams.mvDIPFrameParams.at(0);
        prepareBokehInput(mainFrame, enqueData.mRequest, enqueData);
        prepareBokehOutputs(mainFrame, enqueData.mRequest, enqueData);

        // set QoS for performance
#if (SUPPORT_ISP_VER == 40)
        struct timeval current;
        gettimeofday(&current, nullptr);
        for (size_t i = 0, n = enqueParams.mvDIPFrameParams.size(); i < n; ++i)
            enqueParams.mvDIPFrameParams.at(i).ExpectedEndTime = current;
#endif
        //
        if (mbDumpDIPParam) debugDIPParams(enqueParams);
        //
        enqueFeatureStream(enqueParams, enqueData);
    }
    else
    { // AbnormalCase
        enqueParams.mDequeSuccess = MFALSE;
        depthData.mRequest->updateResult(enqueParams.mDequeSuccess);
        //
        CAM_ULOGME("reqID=%d:Abnormal Return Buffer Result:%d",
                enqueData.mRequest->mRequestNo, enqueParams.mDequeSuccess);
        handleResultData(enqueData.mRequest, enqueData);
    }


    return MTRUE;
}

MVOID BokehNode::setOutputBufferPool(const android::sp<IBufferPool> &pool, MUINT32 allocate)
{
    TRACE_FUNC_ENTER();
    mFullImgPool = pool;
    mFullImgPoolAllocateNeed = allocate;

}

MBOOL BokehNode::prepareBokehInput(Feature::P2Util::DIPFrameParams &framePa, const RequestPtr &request,
                                                               BokehEnqueData &data)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    Feature::P2Util::push_in(framePa, NSCam::NSIoPipe::PORT_IMGI,
                                data.mInYuvImg.mBuffer->getImageBufferPtr());

    #if (SUPPORT_ISP_VER >= 60)
        Feature::P2Util::push_in(framePa, NSCam::NSIoPipe::PORT_CNR_BLURMAPI,
                                 data.mDMBG->getImageBufferPtr());
    #else
        Feature::P2Util::push_in(framePa, NSCam::NSIoPipe::PORT_DMGI,
                                 data.mDMBG->getImageBufferPtr());
    #endif
    MBOOL bRet = true;

    MSize imgInSize = data.mInYuvImg.mBuffer->getImageBufferPtr()->getImgSize();

    bRet &= setSRZInfo(framePa, EDipModule_SRZ3, msDmgi, imgInSize);
    if (!bRet)
        CAM_ULOGME("setSRZInfo SRZ3 fail");

    SmartTuningBuffer tuningBuf = mDynamicTuningPool->request();
    memset(tuningBuf->mpVA, 0, mDynamicTuningPool->getBufSize());
    if (!StereoTuningProvider::getBokehTuningInfo(tuningBuf->mpVA)) {
        CAM_ULOGME("set Tuning Parameter Fail!");
        return MFALSE;
    }

    ULOGMD_IF(mbPipeLogEnable, "applyIspTuning section + [ISpVer:%d]", SUPPORT_ISP_VER);
#if (SUPPORT_ISP_VER >= 50)
    const SFPSensorInput &masterIn = request->getSensorInput(request->mMasterID);
    const SFPIOMap &generalIO      = request->mSFPIOManager.getFirstGeneralIO();
    //
    _bokehNode_ispTuningConfig_ ispConfig = {
        .pInAppMeta      = masterIn.mAppIn,
        .pInHalMeta      = masterIn.mHalIn,
        .pOutApp         = generalIO.mAppOut,
        .pOutHal         = generalIO.mHalOut,
        .bInputResizeRaw = MTRUE,
        .reqNo           = static_cast<MINT32>(request->mRequestNo),
    };
    // TODO : check ISP ver condition
    #if (SUPPORT_ISP_VER >= 60)
        ispConfig.pHalIsp = mpIspHal_Main1;
    #else
        ispConfig.p3AHAL  = mp3AHal_Main1;
    #endif
    //
    trySetMetadata<MINT32>(masterIn.mHalIn, MTK_3A_ISP_BYPASS_LCE, true);
    #if (SUPPORT_ISP_VER > 60)
        trySetMetadata<MUINT8>(masterIn.mHalIn, MTK_3A_ISP_PROFILE,
                               NSIspTuning::EIspProfile_VSDOF_BOK);
    #else
        trySetMetadata<MUINT8>(masterIn.mHalIn, MTK_3A_ISP_PROFILE,
                               NSIspTuning::EIspProfile_Bokeh);
    #endif
    // TODO : check ISP ver condition
    #if (SUPPORT_ISP_VER >= 60)//0:Raw,1:Yuv
        trySetMetadata<MINT32>(masterIn.mHalIn, MTK_ISP_P2_IN_IMG_FMT, 1);
    #endif
    //
    applyISPTuning(PIPE_LOG_TAG, tuningBuf, ispConfig);
#endif
    ULOGMD_IF(mbPipeLogEnable,"applyIspTuning section - ");

    framePa.mStreamTag  = ENormalStreamTag_Bokeh;
    framePa.mTuningData = tuningBuf->mpVA;
    framePa.UniqueKey   = ENormalStreamTag_Bokeh;


    return MTRUE;
}

MBOOL BokehNode::prepareBokehOutputs(Feature::P2Util::DIPFrameParams &framePa, const RequestPtr &request,
                                     BokehEnqueData &bokeh_data)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();
    //
    const SFPSensorInput &masterIn = request->getSensorInput(request->mMasterID);
    MUINT32 sID = request->mMasterID;
    NSCam::NSIoPipe::Output previewOutput = {0}, videoOutput = {0};
    FrameInInfo inInfo;
    getFrameInInfo(inInfo, framePa);
    MSize imgiSize = inInfo.inSize;

    MCrpRsInfo mdp_crop_pv = {}, crz_crop_pv = {};
    MCrpRsInfo mdp_crop_vr = {}, crz_crop_vr = {};

    P2IO output;

    // SW Blending OFF
    if(!mbApplyBlending)
    {
        // case : P2+EIS
        if( request->needNextFullImg(this, sID) )
        {
            CAM_ULOGMD("Hook to plugin");
            P2IO out;
            prepareNextFullOut(out, request, bokeh_data, inInfo);
            imgiSize = bokeh_data.mInYuvImg.mBuffer->getImageBuffer()->getImgSize();
            previewOutput.mBuffer = out.mBuffer;
            previewOutput.mPortID = PORT_WDMAO;
            // MDP
            mdp_crop_pv.mGroupID   = 2;
            mdp_crop_pv.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
            mdp_crop_pv.mResizeDst = imgiSize;
            // CRZ
            crz_crop_pv.mGroupID   = 1;
            crz_crop_pv.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
            crz_crop_pv.mResizeDst = imgiSize;

            framePa.mvCropRsInfo.push_back(mdp_crop_pv);
            framePa.mvCropRsInfo.push_back(crz_crop_pv);
            framePa.mvOut.push_back(previewOutput);
        }
        else
        {
            // Display
            P2IO output_pv;
            if(request->popDisplayOutput(this, output_pv))
            {
                previewOutput.mBuffer = output_pv.mBuffer;
                // MDP
                mdp_crop_pv.mGroupID   = 2;
                mdp_crop_pv.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
                mdp_crop_pv.mResizeDst = imgiSize;
                // CRZ
                crz_crop_pv.mGroupID   = 1;
                crz_crop_pv.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
                crz_crop_pv.mResizeDst = imgiSize;
                // PORT
                if(!mbApplyMDPFeature) // case : P2
                {
                    // IMG3O not support output bokeh yuv before ISP6S
                    #if (SUPPORT_ISP_VER <= 60)
                        framePa.mvCropRsInfo.push_back(mdp_crop_pv);
                        previewOutput.mPortID = PORT_WDMAO;
                    #else
                        previewOutput.mPortID = PORT_IMG3O;
                    #endif
                }
                else // case : P2+MDP
                {
                    previewOutput.mPortID = PORT_WDMAO;
                    // add PQParam
                    DpPqParam *pDpPqParam = new DpPqParam();
                    *pDpPqParam = makeDpPqParam(bokeh_data.mInYuvImg.mPQCtrl.get(), output_pv.mType, output_pv.mBuffer, PIPE_CLASS_TAG);
                    PQParam *pPqParam = new PQParam();
                    pPqParam->WDMAPQParam = (void*)pDpPqParam;
                    ExtraParam extra;
                    extra.CmdIdx = EPIPE_MDP_PQPARAM_CMD;
                    extra.moduleStruct = (void*)pPqParam;
                    framePa.mvExtraParam.push_back(extra);
                    framePa.mvCropRsInfo.push_back(mdp_crop_pv);
                }
                framePa.mvCropRsInfo.push_back(crz_crop_pv);
                framePa.mvOut.push_back(previewOutput);
            }

            P2IO output_vr;
            if(request->popRecordOutput(this, output_vr))
            {
                videoOutput.mBuffer = output_vr.mBuffer;
                // MDP
                mdp_crop_vr.mGroupID   = 3;
                mdp_crop_vr.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
                mdp_crop_vr.mResizeDst = imgiSize;
                // CRZ
                crz_crop_vr.mGroupID   = 1;
                crz_crop_vr.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
                crz_crop_vr.mResizeDst = imgiSize;
                // PORT
                if(!mbApplyMDPFeature)  // case : P2
                {
                    videoOutput.mPortID = PORT_IMG2O;
                }
                else    // case : P2+MDP
                {
                    videoOutput.mPortID = PORT_WROTO;
                    // add PQParam
                    DpPqParam *pDpPqParam = new DpPqParam();
                    *pDpPqParam = makeDpPqParam(bokeh_data.mInYuvImg.mPQCtrl.get(), output_vr.mType, output_vr.mBuffer, PIPE_CLASS_TAG);
                    PQParam *pPqParam = new PQParam();
                    pPqParam->WROTPQParam = (void*)pDpPqParam;
                    ExtraParam extra;
                    extra.CmdIdx = EPIPE_MDP_PQPARAM_CMD;
                    extra.moduleStruct = (void*)pPqParam;
                    framePa.mvExtraParam.push_back(extra);
                    framePa.mvCropRsInfo.push_back(mdp_crop_vr);
                }
                framePa.mvCropRsInfo.push_back(crz_crop_vr);
                framePa.mvOut.push_back(videoOutput);
            }
        }
    }
    else
    {
        // case : P2+SW+EIS
        // query internal buffer from BufferPool
        SmartImageBuffer spBokehYuv = mpBokehYUVBufPool->request();
        MSize img3oSize = spBokehYuv->mImageBuffer->getImgSize();
        if(img3oSize.w != imgiSize.w || img3oSize.h != imgiSize.h)
        {
            // need change image size (IMGI size <= streamingSize)
            spBokehYuv->mImageBuffer->setExtParam(imgiSize);
        }
        previewOutput.mPortID = PORT_IMG3O;
        previewOutput.mBuffer = spBokehYuv->mImageBuffer.get();
        // CRZ
        crz_crop_pv.mGroupID   = 1;
        crz_crop_pv.mCropRect  = MCropRect(imgiSize.w, imgiSize.h);
        crz_crop_pv.mResizeDst = imgiSize;

        framePa.mvCropRsInfo.push_back(crz_crop_pv);
        framePa.mvOut.push_back(previewOutput);
    }

    if (mbPipeLogEnable)
    {
        if(request->needDisplayOutput(this) && previewOutput.mBuffer)
        {
            CAM_ULOGMD("IMGI size(%dx%d), %s(PV) size(%dx%d)"
                    "P2B PV CRZ setting: GroupId(%d) CropRect(%dx%d)",
                    imgiSize.w, imgiSize.h,
                    mbApplyMDPFeature? "WDMAO" : "IMG3O",
                    previewOutput.mBuffer->getImgSize().w, previewOutput.mBuffer->getImgSize().h,
                    crz_crop_pv.mGroupID, crz_crop_pv.mCropRect.s.w, crz_crop_pv.mCropRect.s.h);
        }
        if(request->needRecordOutput(this) && videoOutput.mBuffer)
        {
            CAM_ULOGMD("IMGI size(%dx%d), %s(VR) size(%dx%d)"
                    "P2B VR CRZ setting: GroupId(%d) CropRect(%dx%d)",
                    imgiSize.w, imgiSize.h,
                    mbApplyMDPFeature? "WROTO" : "IMG2O",
                    videoOutput.mBuffer->getImgSize().w, videoOutput.mBuffer->getImgSize().h,
                    crz_crop_vr.mGroupID, crz_crop_vr.mCropRect.s.w, crz_crop_vr.mCropRect.s.h);
        }
    }

    if (!bokeh_data.mRemainingOutputs.empty())
        CAM_ULOGME("Not Support Multiple output : RemainSize:%zu",
                bokeh_data.mRemainingOutputs.size());


    return MTRUE;
}


MBOOL BokehNode::needFullForExtraOut(std::vector<P2IO> &outs)
{
    static const MUINT32 maxMDPOut = 2;
    if(outs.size() > maxMDPOut)
        return MTRUE;
    MUINT32 rotCnt = 0;
    for (auto&& out : outs)
    {
        if (out.mTransform != 0)
        {
            ++rotCnt;
        }
    }
    return (rotCnt > 1);
}

MVOID BokehNode::prepareNextFullOut(P2IO &output, const RequestPtr &request,
                                        BokehEnqueData &data, const FrameInInfo &/*inInfo*/)
{
    TRACE_FUNC_ENTER();
    MSize outSize, resize;

    sp<IImageBuffer> pInBuffer = data.mInYuvImg.mBuffer->getImageBuffer();
    outSize = pInBuffer->getImgSize();
    data.mNextReq = request->requestNextFullImg(this, request->mMasterID);
    size_t nextsNum = data.mNextReq.mList.size();
    NextIO::NextBuf firstNext = getFirst(data.mNextReq);
    data.mOutNextFullImg.mBuffer = firstNext.mBuffer;
    resize = firstNext.mAttr.mResize;
    if( resize.w && resize.h )
    {
        outSize = resize;
    }
    if( nextsNum != 1 )
    {
        CAM_ULOGME("Not Support NextFull total(%zu), only support 1 buffer !!", nextsNum);
    }
    TRACE_FUNC(" Frame %d QFullImg %s", request->mRequestNo,
                data.mOutNextFullImg.mBuffer == nullptr ? "is null" : "" );

    data.mOutNextFullImg.setAllInfo(data.mInYuvImg);
    data.mOutNextFullImg.mBuffer->getImageBuffer()->setExtParam(outSize);

    output.mBuffer      = data.mOutNextFullImg.mBuffer->getImageBufferPtr();
    output.mTransform   = 0;
    output.mCropRect    = MRect(MPoint(0,0), pInBuffer->getImgSize());
    output.mCropDstSize = outSize;

    data.mOutNextFullImg.accumulate("bokehNFull", request->mLog, pInBuffer->getImgSize(), output.mCropRect, outSize);


}

MVOID BokehNode::prepareFullImg(Feature::P2Util::DIPFrameParams &frame, const RequestPtr &request,
                                BokehEnqueData &data, const FrameInInfo &/*inInfo*/)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d FullImgPool=(%d/%d)", request->mRequestNo,
                    mFullImgPool->peakAvailableSize(), mFullImgPool->peakPoolSize());

    data.mOutFullImg.mBuffer    = mFullImgPool->requestIIBuffer();
    sp<IImageBuffer> pOutBuffer = data.mOutFullImg.mBuffer->getImageBuffer();
    sp<IImageBuffer> pInBuffer  = data.mInYuvImg.mBuffer->getImageBuffer();

    data.mOutFullImg.setAllInfo(data.mInYuvImg);
    if (!pOutBuffer->setExtParam(pInBuffer->getImgSize()))
    {
        CAM_ULOGME("Full Img setExtParm Fail!, target size(%04dx%04d)",
                pInBuffer->getImgSize().w, pInBuffer->getImgSize().h);
    }

    Output output;
    output.mPortID = PortID(EPortType_Memory,  NSImageio::NSIspio::EPortIndex_IMG3O, PORTID_OUT);
    output.mBuffer = pOutBuffer.get();
    frame.mvOut.push_back(output);

}

MVOID BokehNode::enqueFeatureStream(Feature::P2Util::DIPParams &params, BokehEnqueData &data)
{
    TRACE_FUNC_ENTER();
    CAM_ULOGMD("sensor(%d) Frame %d enque start", mSensorIndex, data.mRequest->mRequestNo);
    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_P2));
    data.mRequest->mTimer.startEnqueBokeh();
    this->incExtThreadDependency();
    this->enqueDIPStreamBase(mDIPStream, params, data);

}

MVOID BokehNode::onDIPStreamBaseCB(const Feature::P2Util::DIPParams &params, const BokehEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing BokehNode class members
    TRACE_FUNC_ENTER();

    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if ( request == nullptr )
    {
        CAM_ULOGME("Missing request");
    }
    else
    {
        this->decExtThreadDependency();
        request->mTimer.stopEnqueBokeh();
        CAM_ULOGMD("sensor(%d) Frame %d enque done in %d ms, result = %d",
                mSensorIndex, request->mRequestNo, request->mTimer.getElapsedEnqueBokeh(),
                params.mDequeSuccess);

        if ( !params.mDequeSuccess )
            CAM_ULOGMW("Frame %d enque result failed", request->mRequestNo);

        // wake up background thread for handing post processing
        if ( mJobQueue )
        {
            mJobQueue->addJob(
                  std::bind([](intptr_t arg1,
                               const Feature::P2Util::DIPParams &params,
                               const BokehEnqueData &data)->void
                  {
                      if(arg1 == 0) return;
                      BokehNode* pObj = reinterpret_cast<BokehNode*>(arg1);
                      pObj->onHandlePostProcess(params, data);
                  },
                  reinterpret_cast<intptr_t>(this),
                  params,
                  data) );
        }
        else
        {
            MY_LOGE("Job queue is not existed, failed");
        }
        return;
    }
}

MBOOL
BokehNode::
onHandlePostProcess(const Feature::P2Util::DIPParams &params, const BokehEnqueData &data)
{
    CAM_TRACE_NAME("BokehNode::onHandlePostProcess");
    RequestPtr request = data.mRequest;
    BokehOutImgs bokehOutImgs;
    processOutputBuffer(params, data, bokehOutImgs);
    ULOGMD_IF(mbPipeLogEnable,
            "out level= %d, PV[P2:%p][Blending:%p][Mdp:%p], VR[P2:%p][Blending:%p][Mdp:%p]",
            bokehOutImgs.iOutLevel,
            (bokehOutImgs.pvBufs.pP2Out       == nullptr ? 0L : bokehOutImgs.pvBufs.pP2Out)      ,
            (bokehOutImgs.pvBufs.pBlendingOut == nullptr ? 0L : bokehOutImgs.pvBufs.pBlendingOut),
            (bokehOutImgs.pvBufs.pMdpOut      == nullptr ? 0L : bokehOutImgs.pvBufs.pMdpOut)     ,
            (bokehOutImgs.vrBufs.pP2Out       == nullptr ? 0L : bokehOutImgs.vrBufs.pP2Out)      ,
            (bokehOutImgs.vrBufs.pBlendingOut == nullptr ? 0L : bokehOutImgs.vrBufs.pBlendingOut),
            (bokehOutImgs.vrBufs.pMdpOut      == nullptr ? 0L : bokehOutImgs.vrBufs.pMdpOut) );
    // dump buffer
    if (mbDumpImgeBuf || mbShowDepthMap)
        dumpBuff(data, params, bokehOutImgs);
    // handle data
    request->updateResult(params.mDequeSuccess);
    handleResultData(request, data);
    // release malloc buffer
    for (const auto& frameParam : params.mvDIPFrameParams) {
        for (const auto& moduleInfo : frameParam.mvModuleData) {
            if ( moduleInfo.moduleTag == EDipModule_SRZ3 ||
                    moduleInfo.moduleTag == EDipModule_SRZ4)
            {
                if (moduleInfo.moduleStruct != nullptr)
                {
                    delete (_SRZ_SIZE_INFO_*)moduleInfo.moduleStruct;
                }
            }
        }
    }
    request->mTimer.stopBokeh();
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
BokehNode::
processOutputBuffer(
    const Feature::P2Util::DIPParams &params,
    const BokehEnqueData &data,
    BokehOutImgs &bokehOutImgs
)
{
    MY_LOGD("+");
    CAM_TRACE_NAME("BokehNode::processOutputBuffer");
    RequestPtr request = data.mRequest;
    P2IO output_pv, output_vr;
    MUINT32 sID = request->mMasterID;
    FrameInInfo inInfo;
    // obtain bokeh yuv from P2
    for (const auto& frameParam : params.mvDIPFrameParams) {
        getFrameInInfo(inInfo, frameParam);
        // get result buffer
        for (const auto& outParam : frameParam.mvOut) {
            NSCam::NSIoPipe::PortID portId = outParam.mPortID;
            MUINT32 bufDataType = mapToBufferID(portId, eSTEREO_SCENARIO_PREVIEW);
            if (bufDataType == BOKEH_ER_BUF_DISPLAY) {
                bokehOutImgs.pvBufs.pP2Out = outParam.mBuffer;
            }
            if (bufDataType == BOKEH_ER_BUF_RECORD) {
                bokehOutImgs.vrBufs.pP2Out = outParam.mBuffer;
            }
        }
        // delete PQParam
        for (const auto& extParam : frameParam.mvExtraParam) {
            if(extParam.CmdIdx == EPIPE_MDP_PQPARAM_CMD)
            {
                PQParam* pPqParam = (PQParam*)extParam.moduleStruct;
                if(pPqParam->WDMAPQParam)
                {
                    delete (DpPqParam*)pPqParam->WDMAPQParam;
                    pPqParam->WDMAPQParam = nullptr;
                }
                if(pPqParam->WROTPQParam)
                {
                    delete (DpPqParam*)pPqParam->WROTPQParam;
                    pPqParam->WROTPQParam = nullptr;
                }
                delete pPqParam;
                pPqParam = nullptr;
            }
        }
    }
    // output PVVR in this stage by P2 only
    if(mbApplyBlending)
    {
        bokehOutImgs.iOutLevel = 1;
        // output PV VR buffer in the next stage
        if(request->needNextFullImg(this, sID)) // case : P2+SW+EIS
        {
            P2IO out;
            prepareNextFullOut(out, request, const_cast<BokehEnqueData&>(data), inInfo);
            if(!executeSwBlending(data, bokehOutImgs.pvBufs.pP2Out, out.mBuffer))
            {
                MY_LOGE("executeSwBlending failed");
                return MFALSE;
            }
            bokehOutImgs.pvBufs.pBlendingOut = out.mBuffer;
        }
        // output PV VR buffer in this stage
        else
        {
            // pop display & record output buffer
            IImageBuffer *pOutPvBuf = nullptr, *pOutVrBuf = nullptr;
            if(!request->popDisplayOutput(this, output_pv))
            {
                MY_LOGE("query display output failed");
                return MFALSE;
            }
            pOutPvBuf = output_pv.mBuffer;
            if(request->popRecordOutput(this, output_vr))
            {
                pOutVrBuf = output_vr.mBuffer;
            }
            //
            if(mbApplyMDPFeature)       // case : P2+SW+MDP
            {
                bokehOutImgs.iOutLevel = 2;
                // set BokehNode output buffer
                bokehOutImgs.pvBufs.pMdpOut = pOutPvBuf;
                bokehOutImgs.vrBufs.pMdpOut = pOutVrBuf;
                // query blending output buffer from pool
                bokehOutImgs.pvBufs.pBlendingOut = mpBlendingBufPool->requestIIBuffer()->getImageBufferPtr();
                bokehOutImgs.pvBufs.pBlendingOut->setExtParam(bokehOutImgs.pvBufs.pP2Out->getImgSize());
                if(!executeSwBlending(data, bokehOutImgs.pvBufs.pP2Out, bokehOutImgs.pvBufs.pBlendingOut))
                {
                    MY_LOGE("executeSwBlending failed");
                    return MFALSE;
                }
                DpPqParam dpPqParam_pv, dpPqParam_vr;
                dpPqParam_pv = makeDpPqParam(data.mInYuvImg.mPQCtrl.get(), output_pv.mType, output_pv.mBuffer, PIPE_CLASS_TAG);
                if(pOutVrBuf)
                    dpPqParam_vr = makeDpPqParam(data.mInYuvImg.mPQCtrl.get(), output_vr.mType, output_vr.mBuffer, PIPE_CLASS_TAG);
                if( !executeMdp(bokehOutImgs.pvBufs.pBlendingOut,
                                bokehOutImgs.pvBufs.pMdpOut,
                                &dpPqParam_pv,
                                output_pv.mCropRect,
                                output_pv.mTransform,
                                bokehOutImgs.vrBufs.pMdpOut,
                                &dpPqParam_vr,
                                output_vr.mCropRect,
                                output_pv.mTransform) )
                {
                    MY_LOGE("failed to Apply MDP Feature on preview buffer");
                    return MFALSE;
                }
                // support VSS
                std::vector<P2IO> output_vss;
                if (request->needExtraOutput(this) && request->popExtraOutputs(this, output_vss)) {
                    for (size_t i = 0; i < output_vss.size(); ++i) {
                        DpPqParam dpPqParam_vss = makeDpPqParam(data.mInYuvImg.mPQCtrl.get(),
                                                                output_vss[i].mType,
                                                                output_vss[i].mBuffer,
                                                                PIPE_CLASS_TAG);
                        if ( !executeMdp(bokehOutImgs.pvBufs.pBlendingOut,
                                         output_vss[i].mBuffer,
                                         &dpPqParam_vss,
                                         output_pv.mCropRect,
                                         output_vss[i].mTransform)) {
                            MY_LOGE("failed to generate VSS from clear yuv");
                            return MFALSE;
                        }
                    }
                }
            }
            else    // case : P2+SW
            {
                // set BokehNode output buffer
                bokehOutImgs.pvBufs.pBlendingOut = pOutPvBuf;
                bokehOutImgs.vrBufs.pBlendingOut = pOutVrBuf;
                if(!executeSwBlending(data, bokehOutImgs.pvBufs.pP2Out, bokehOutImgs.pvBufs.pBlendingOut, bokehOutImgs.vrBufs.pBlendingOut))
                {
                    MY_LOGE("executeSwBlending failed");
                    return MFALSE;
                }
            }
        }
    }
    MY_LOGD("-");
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
BokehNode::
executeSwBlending(
    const BokehEnqueData &data,
    IImageBuffer *pBokehInBuf,
    IImageBuffer *pDstPvBuffer,
    IImageBuffer *pDstVrBuffer
)
{
    CAM_TRACE_NAME("BokehNode::executeSwBlending");
    MBOOL bRet = MTRUE;
    RequestPtr request = data.mRequest;
    MINT32 reqID = request->mRequestNo;
    const SFPSensorInput &masterIn = request->getSensorInput(request->mMasterID);
    // set Blending input data
    BLENDING_HAL_IN_DATA inParam;
    IMetadata* pInHalMeta = masterIn.mHalIn;
    if(!tryGetMetadata<MINT32>(pInHalMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, inParam.magicNumber))
    {
        MY_LOGE("reqID=%d Cannot find MTK_P1NODE_PROCESSOR_MAGICNUM meta in HalMeta!", reqID);
        return MFALSE;
    }
    inParam.requestNumber = reqID;
    inParam.bokehImage    = pBokehInBuf;
    inParam.cleanImage    = data.mInYuvImg.getImageBufferPtr();
    inParam.blurMap       = data.mDMBG->getImageBufferPtr();
    // set Blending output data
    BLENDING_HAL_OUT_DATA outParam;
    outParam.bokehImage = pDstPvBuffer;
    // execute blending
    Timer blendingTimer(MTRUE);
    ULOGMD_IF(mbPipeLogEnable, "execute Swblending start");
    if( !mpBlendingHal->BlendingHALRun(inParam, outParam) )
    {
        MY_LOGE("reqID=%d run blening hal failed", reqID);
        return MFALSE;
    }
    blendingTimer.stop();
    // copy buffer if need
    Timer copyTimer(MTRUE);
    if( pDstVrBuffer && !copyBuffer(pDstPvBuffer, pDstVrBuffer) )
    {
        MY_LOGE("reqID=%d copy blending buffer to video failed", reqID);
        return MFALSE;
    }
    copyTimer.stop();
    MY_LOGD("reqID=%d Apply SW blending %d ms, copy to video %d ms", request->mRequestNo, blendingTimer.getElapsed(), copyTimer.getElapsed());
    // sync cache
    pDstPvBuffer->syncCache(eCACHECTRL_FLUSH);
    if(pDstVrBuffer)
        pDstVrBuffer->syncCache(eCACHECTRL_FLUSH);
    return bRet;
}

/*******************************************************************************
 *
 ********************************************************************************/

MBOOL
BokehNode::
copyBuffer(
    IImageBuffer* pSrcBuffer,
    IImageBuffer* pDstBuffer
)
{
    MY_LOGD("copy buffer +");
    if(pDstBuffer->getPlaneCount() != pSrcBuffer->getPlaneCount())
    {
        MY_LOGE("buffer plane count is not consistent! dst_buffer:%lu, src_buffer:%lu",
                pDstBuffer->getPlaneCount(), pSrcBuffer->getPlaneCount());
        return MFALSE;
    }
    if(pDstBuffer->getBufSizeInBytes(0) != pSrcBuffer->getBufSizeInBytes(0))
    {
        MY_LOGE("buffer size is not consistent! dst_buffer size=%dx%d(%lu bytes) src_buffer size=%dx%d(%lu bytes)",
                    pDstBuffer->getImgSize().w, pDstBuffer->getImgSize().h, pDstBuffer->getBufSizeInBytes(0),
                    pSrcBuffer->getImgSize().w, pSrcBuffer->getImgSize().h, pSrcBuffer->getBufSizeInBytes(0));
        return MFALSE;
    }
    memcpy( (void*)pDstBuffer->getBufVA(0),
            (void*)pSrcBuffer->getBufVA(0),
            pDstBuffer->getBufSizeInBytes(0));
    pDstBuffer->syncCache(eCACHECTRL_FLUSH);
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID BokehNode::handleResultData(const RequestPtr &request, const BokehEnqueData &data)
{
    // This function is not thread safe,
    // because it is called by onDIPParamsCB,
    // avoid accessing BokehNode class members
    CAM_ULOGM_FUNCLIFE();
    BasicImg full;
    if ( data.mOutNextFullImg.mBuffer != nullptr )
    {
        full = data.mOutNextFullImg;
    }
    else
    {
        full = data.mOutFullImg;
    }

    markNodeExit(request);
    if( mPipeUsage.supportTPI(TPIOEntry::YUV) ) //ex:Face Beauty
    {
        handleData(ID_BOKEH_TO_VENDOR_FULLIMG, BasicImgData(full, request));
    }
    // support warpnode in record scenario
    else if( mPipeUsage.supportWarpNode() && mWorkingScenario == eBOKEH_RECORD)
    {
        if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )   handleData(ID_BOKEH_TO_WARP_P_FULLIMG, BasicImgData(full, request));
        if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )    handleData(ID_BOKEH_TO_WARP_R_FULLIMG, BasicImgData(full, request));
    }
    else if( mPipeUsage.supportXNode() )
    {
        NextResult nextResult;
        NextImgMap map = initNextMap(data.mNextReq);
        updateFirst(map, data.mNextReq, data.mOutNextFullImg);
        addMaster(nextResult, request->mMasterID, map);
        handleData(ID_BOKEH_TO_XNODE, NextData(nextResult, request));
    }
    else //To DISPLAY Preview
    {
        handleData(ID_BOKEH_TO_HELPER, HelperData(FeaturePipeParam::MSG_FRAME_DONE, request));
    }

    if( request->needDump() )
    {
        if( data.mOutFullImg.mBuffer != nullptr )
        {
            data.mOutFullImg.mBuffer->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
            dumpData(data.mRequest, data.mOutFullImg.mBuffer->getImageBufferPtr(),
                     "BokehNode_full");
        }
        if( data.mOutNextFullImg.mBuffer != nullptr )
        {
            data.mOutNextFullImg.mBuffer->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
            dumpData(data.mRequest, data.mOutNextFullImg.mBuffer->getImageBufferPtr(),
                     "BokehNode_nextfull");
        }
    }
}

/*******************************************************************************
 *
 ********************************************************************************/
MUINT32 BokehNode:: mapToBufferID( NSCam::NSIoPipe::PortID const portId,
                                    const MUINT32 scenarioID)
{
    if (portId == PORT_IMGI)
        return BOKEH_ER_BUF_MAIN1;
    if (portId == PORT_DMGI)
        return BOKEH_ER_BUF_DMG;
    if (portId == PORT_CNR_BLURMAPI)
        return BOKEH_ER_BUF_DMG;
    if (portId == PORT_DEPI)
        return BOKEH_ER_BUF_DMBG;
    if (portId == PORT_IMG3O)
        return BOKEH_ER_BUF_DISPLAY;
    if (portId == PORT_IMG2O)
        return BOKEH_ER_BUF_RECORD;

    if (portId == PORT_WROTO) {
        if(scenarioID == eSTEREO_SCENARIO_CAPTURE)
            return BOKEH_ER_BUF_VSDOF_IMG;
        else if(scenarioID == eSTEREO_SCENARIO_RECORD)
            return BOKEH_ER_BUF_RECORD;
    }
    if (portId == PORT_WDMAO) {
        if (scenarioID == eSTEREO_SCENARIO_CAPTURE)
            return BOKEH_ER_BUF_WDMAIMG;
        if (scenarioID == eSTEREO_SCENARIO_RECORD ||
            scenarioID == eSTEREO_SCENARIO_PREVIEW)
            return BOKEH_ER_BUF_DISPLAY;
    }
    return BOKEH_ER_BUF_START;
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL BokehNode::setSRZInfo(Feature::P2Util::DIPFrameParams& frameParam, MINT32 modulTag,
                            MSize inputSize, MSize outputSize)
{
    CAM_TRACE_NAME("BokehNode::setSRZInfo");
    // set SRZ 3
    _SRZ_SIZE_INFO_ *srzInfo = new _SRZ_SIZE_INFO_();
    if(srzInfo == nullptr)
    {
        MY_LOGE("failed to allocate srzInfo");
        delete srzInfo;
        return MFALSE;
    }
    srzInfo->in_w        = inputSize.w;
    srzInfo->in_h        = inputSize.h;
    srzInfo->crop_w      = inputSize.w;
    srzInfo->crop_h      = inputSize.h;
    srzInfo->crop_x      = 0;
    srzInfo->crop_y      = 0;
    srzInfo->out_w       = outputSize.w;
    srzInfo->out_h       = outputSize.h;
    srzInfo->crop_floatX = 0;
    srzInfo->crop_floatY = 0;
    //
    ModuleInfo moduleInfo;
    moduleInfo.moduleTag    = modulTag;
    moduleInfo.frameGroup   = 0;
    moduleInfo.moduleStruct = static_cast<void*>(srzInfo);
    //
    ULOGMD_IF(mbPipeLogEnable, "srz moduleTag (%d) in(%04dx%04d) out(%04dx%04d)",
               modulTag, srzInfo->in_w, srzInfo->in_h, srzInfo->out_w, srzInfo->out_h);

    frameParam.mvModuleData.push_back(moduleInfo);
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/

MBOOL
BokehNode::
executeMdp(
    IImageBuffer* input,
    IImageBuffer* output1,
    DpPqParam *dpPqParam1,
    MRectF cropRect1,
    MUINT32 transform1,
    IImageBuffer* output2,
    DpPqParam *dpPqParam2,
    MRectF cropRect2,
    MUINT32 transform2)
{
    CAM_TRACE_NAME("BokehNode::executeMdp");
    if(mpDpIspStream == nullptr)
    {
        MY_LOGE("null mpDpIspStream");
        return MFALSE;
    }
    if(input == nullptr || output1 == nullptr)
    {
        MY_LOGE("null I/O buffer");
        return MFALSE;
    }
    // map transform to rotation angle
    auto mapTransformToRotAngle = [&] (MUINT32 transform) {
        MINT32 rotAngle = 0;
        if (transform == 0x00) {
            rotAngle = 0;
        } else if (transform == 0x04) {
            rotAngle = 90;
        } else if (transform == 0x03) {
            rotAngle = 180;
        } else if (transform == 0x07) {
            rotAngle = 270;
        } else {
            MY_LOGE("Rotation angle not supported");
        }
        return rotAngle;
    };

    // set mdp config
    VSDOF::util::sDpIspConfigs config;
    VSDOF::util::sDpIspOut pvConfig, vrConfig;
    config.pDpIspStream = mpDpIspStream;
    config.pSrcBuffer   = input;
    // config pv part
    pvConfig.pDpPqParam = dpPqParam1;
    pvConfig.portIndex  = 0;
    pvConfig.rotAngle   = mapTransformToRotAngle(transform1);
    pvConfig.pDstBuffer = output1;
    pvConfig.cropRect   = MRect(MPoint(cropRect1.p.x, cropRect1.p.y),
                                MSize(cropRect1.s.w, cropRect1.s.h));
    config.vDpIspOut.push_back(pvConfig);
    // config vr part
    if(output2)
    {
        vrConfig.pDpPqParam = dpPqParam2;
        vrConfig.portIndex  = 1;
        vrConfig.rotAngle   = mapTransformToRotAngle(transform2);
        vrConfig.pDstBuffer = output2;
        vrConfig.cropRect   = MRect(MPoint(cropRect2.p.x, cropRect2.p.y),
                                    MSize(cropRect2.s.w, cropRect2.s.h));
        config.vDpIspOut.push_back(vrConfig);
    }
    ULOGMD_IF(mbPipeLogEnable, "execute pure mdp start");
    Timer timer(MTRUE);
    if(!executeDpIspStream(config))
    {
        MY_LOGE("excuteMDP fail: Cannot perform MDP operation on target1.");
        return MFALSE;
    }
    timer.stop();
    MY_LOGD("execute pure mdp end, %d ms", timer.getElapsed());
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID BokehNode::dumpBuff(
    const BokehEnqueData& data,
    const Feature::P2Util::DIPParams& rParams,
    BokehOutImgs &bokehOutImgs
)
{
    CAM_ULOGMD("+");
    RequestPtr reqPtr = data.mRequest;
    MINT32 reqID = reqPtr->mRequestNo;
    if (rParams.mvDIPFrameParams.size()==0) {
        CAM_ULOGME("rParams.mvDIPFrameParams is zero. should not happened");
        return;
    }
    //
    for (MUINT32 i = 0; (i < rParams.mvDIPFrameParams.size()) && (mbPipeLogEnable > 0) ; ++i) {
        CAM_ULOGMD("reqID(%d) in(%zu) out(%zu)", reqID,
                   rParams.mvDIPFrameParams[i].mvIn.size(),
                   rParams.mvDIPFrameParams[i].mvOut.size());
    }
    // get output preview buffer
    IImageBuffer *pOutPvBuffer = nullptr;
    if(bokehOutImgs.iOutLevel == 0)
        pOutPvBuffer = bokehOutImgs.pvBufs.pP2Out;
    else if(bokehOutImgs.iOutLevel == 1)
        pOutPvBuffer = bokehOutImgs.pvBufs.pBlendingOut;
    else if(bokehOutImgs.iOutLevel == 2)
        pOutPvBuffer = bokehOutImgs.pvBufs.pMdpOut;
    else
    {
        MY_LOGE("Undefined bokeh output level %d", bokehOutImgs.iOutLevel);
        return;
    }
    //
    if (mbShowDepthMap)
    {
        CAM_ULOGMD("ShowDepthMap (%d) +", reqID);
        IImageBuffer *input = nullptr;
        input = data.mDebugImg->getImageBufferPtr();
        if(input == nullptr)
        {
            MY_LOGE("debug buffer is nullptr, can not obtain correct debug buffer");
        }
        // check output buffer
        if(pOutPvBuffer == nullptr)
        {
            MY_LOGE("display buffer is null");
        }
        // mark cache to invalid, ensure get buffer from memory.
        if (input != nullptr) {
            if(mbDebugColor)
            {
                input->syncCache(eCACHECTRL_INVALID);
                IImageBuffer* resizedDepth = resizeIntoDebugSize(input);
                input->syncCache(eCACHECTRL_FLUSH);
                overlapDisplay(resizedDepth, pOutPvBuffer);
            }
            else
            {
                input->syncCache(eCACHECTRL_INVALID);
                shiftDepthMapValue(input, 2);
                input->syncCache(eCACHECTRL_FLUSH);
                outputDepthMap(input, pOutPvBuffer);
            }
        } else {
            CAM_ULOGME("Passing null pointer input to syncCache");
        }
        // }
        CAM_ULOGMD("ShowDepthMap -");
    }

    if (mbDumpImgeBuf && reqID >= miDumpStartIdx && reqID < miDumpStartIdx + miDumpBufSize) // dump out port
    {
        CAM_ULOGMD("Dump image(%d) +", reqID);
        const SFPSensorInput &masterSensorIn = reqPtr->getSensorInput(reqPtr->mMasterID);
        IMetadata* pMeta_InHal = masterSensorIn.mHalIn;
        //
        const size_t PATH_SIZE    = 1024;
        char writepath[PATH_SIZE] = {0};
        char filename[PATH_SIZE]  = {0};
        extract(&mDumpHint, pMeta_InHal);

        for (const auto& frameParam : rParams.mvDIPFrameParams)
        {
            // for (const auto& outParam : frameParam.mvOut)
            // {
            //     NSCam::NSIoPipe::PortID portId = outParam.mPortID;
            //     IImageBuffer* buf = outParam.mBuffer;
            //     if (buf != nullptr) {
            //         extract(&mDumpHint, buf);
            //         if (portId == PORT_WDMAO) {
            //             snprintf(filename, PATH_SIZE, "%s_%u_reqID_%d", "P2B_WDMAO",
            //                      buf->getBufStridesInBytes(0), reqID);
            //         } else if (portId == PORT_WROTO) {
            //             snprintf(filename, PATH_SIZE, "%s_%u_reqID_%d", "P2B_WROTO",
            //                      buf->getBufStridesInBytes(0), reqID);
            //         } else if (portId == PORT_IMG3O) {
            //             snprintf(filename, PATH_SIZE, "%s_%u_reqID_%d", "P2B_IMG3O",
            //                      buf->getBufStridesInBytes(0), reqID);
            //         } else if (portId == PORT_IMG2O) {
            //             snprintf(filename, PATH_SIZE, "%s_%u_reqID_%d", "P2B_IMG2O",
            //                      buf->getBufStridesInBytes(0), reqID);
            //         }
            //         genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
            //                 TuningUtils::YUV_PORT_WDMAO, filename);
            //         //
            //         buf->saveToFile(writepath);
            //     } else {
            //         CAM_ULOGME("outParam.mBuffer is nullptr, something wrong");
            //     }
            // }

            for (const auto& inParam : frameParam.mvIn)
            {
                NSCam::NSIoPipe::PortID portId = inParam.mPortID;
                if (portId == PORT_DMGI || portId == PORT_CNR_BLURMAPI) {
                    IImageBuffer* buf = inParam.mBuffer;
                    if (buf != nullptr) {
                        extract(&mDumpHint, buf);
                        //
                        snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                 (portId == PORT_DMGI ?  "P2B_DMBG" : "P2B_CNR_BLURMAP"),
                                 buf->getBufStridesInBytes(0), reqID);
                        //
                        genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                                        TuningUtils::YUV_PORT_UNDEFINED, filename);
                        //
                        buf->saveToFile(writepath);
                    } else {
                        CAM_ULOGME("inParam.mBuffer is nullptr, something wrong");
                    }
                }
                if(portId == PORT_IMGI) {
                    IImageBuffer* buf = inParam.mBuffer;
                    if (buf != nullptr) {
                        extract(&mDumpHint, buf);
                        //
                        snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d", "P2B_CLEAN_YUV",
                                buf->getBufStridesInBytes(0), reqID);
                        //
                        genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                                        TuningUtils::YUV_PORT_UNDEFINED, filename);
                        //
                        buf->saveToFile(writepath);
                    } else {
                        CAM_ULOGME("inParam.mBuffer is nullptr, something wrong");
                    }
                }
            }
        }
        // dump preview output buffer
        if(bokehOutImgs.pvBufs.pP2Out)
        {
            extract(&mDumpHint, bokehOutImgs.pvBufs.pP2Out);
            snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                bokehOutImgs.iOutLevel == 0 ? "P2B_DISPLAY" : "P2B_INTERNAL",
                                bokehOutImgs.pvBufs.pP2Out->getBufStridesInBytes(0), reqID);
            genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                            TuningUtils::YUV_PORT_UNDEFINED, filename);
            bokehOutImgs.pvBufs.pP2Out->saveToFile(writepath);
        }
        if(bokehOutImgs.pvBufs.pBlendingOut)
        {
            extract(&mDumpHint, bokehOutImgs.pvBufs.pBlendingOut);
            snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                bokehOutImgs.iOutLevel == 1 ? "SWBLENDING_DISPLAY" : "SWBLENDING_INTERNAL",
                                bokehOutImgs.pvBufs.pBlendingOut->getBufStridesInBytes(0), reqID);
            genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                            TuningUtils::YUV_PORT_UNDEFINED, filename);
            bokehOutImgs.pvBufs.pBlendingOut->saveToFile(writepath);
        }
        if(bokehOutImgs.pvBufs.pMdpOut)
        {
            extract(&mDumpHint, bokehOutImgs.pvBufs.pMdpOut);
            snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                bokehOutImgs.iOutLevel == 2 ? "MDP_DISPLAY" : "MDP_INTERNAL",
                                bokehOutImgs.pvBufs.pMdpOut->getBufStridesInBytes(0), reqID);
            genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                            TuningUtils::YUV_PORT_UNDEFINED, filename);
            bokehOutImgs.pvBufs.pMdpOut->saveToFile(writepath);
        }
        // dump video output buffer
        if(bokehOutImgs.vrBufs.pP2Out)
        {
            extract(&mDumpHint, bokehOutImgs.vrBufs.pP2Out);
            snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                bokehOutImgs.iOutLevel == 0 ? "P2B_RECORD" : "P2B_INTERNAL",
                                bokehOutImgs.vrBufs.pP2Out->getBufStridesInBytes(0), reqID);
            genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                            TuningUtils::YUV_PORT_UNDEFINED, filename);
            bokehOutImgs.vrBufs.pP2Out->saveToFile(writepath);
        }
        if(bokehOutImgs.vrBufs.pBlendingOut)
        {
            extract(&mDumpHint, bokehOutImgs.vrBufs.pBlendingOut);
            snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                bokehOutImgs.iOutLevel == 1 ? "SWBLENDING_RECORD" : "SWBLENDING_INTERNAL",
                                bokehOutImgs.vrBufs.pBlendingOut->getBufStridesInBytes(0), reqID);
            genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                            TuningUtils::YUV_PORT_UNDEFINED, filename);
            bokehOutImgs.vrBufs.pBlendingOut->saveToFile(writepath);
        }
        if(bokehOutImgs.vrBufs.pMdpOut)
        {
            extract(&mDumpHint, bokehOutImgs.vrBufs.pMdpOut);
            snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d",
                                bokehOutImgs.iOutLevel == 2 ? "MDP_RECORD" : "MDP_INTERNAL",
                                bokehOutImgs.vrBufs.pMdpOut->getBufStridesInBytes(0), reqID);
            genFileName_YUV(writepath, PATH_SIZE, &mDumpHint,
                            TuningUtils::YUV_PORT_UNDEFINED, filename);
            bokehOutImgs.vrBufs.pMdpOut->saveToFile(writepath);
        }
    }
    CAM_ULOGMD("-");
}

//************************************************************************
//
//************************************************************************
MVOID BokehNode:: outputDepthMap( IImageBuffer* depthMap, IImageBuffer* displayResult)
{
    if (displayResult == nullptr || displayResult->getPlaneCount() < 2)
        return;
    if (mpDpStream == nullptr)
        return;
    //
    SmartImageBuffer mdpBuffer = mpDepthMapBufPool->request();
    //
    VSDOF::util::sDpIspConfig config = {
        .pDpIspStream = mpDpIspStream,
        .pSrcBuffer   = depthMap,
        .pDstBuffer   = mdpBuffer->mImageBuffer.get(),
        .rotAngle     = 0,
    };
    if(!excuteDpIspStream(config)) {
        CAM_ULOGME("excuteMDP fail.");
        return;
    }

    MSize outImgSize = displayResult->getImgSize();
    MSize inImgSize  = mdpBuffer->mImageBuffer->getImgSize();
    if(displayResult->getImgFormat() == eImgFmt_YV12)
    {
        char* outAddr0   = (char*)displayResult->getBufVA(0);
        char* outAddr1   = (char*)displayResult->getBufVA(1);
        char* outAddr2   = (char*)displayResult->getBufVA(2);
        char* inAddr     = (char*)mdpBuffer->mImageBuffer->getBufVA(0);
        MINT32 halfInWidth  = inImgSize.w >> 1;
        MINT32 halfInHeight = inImgSize.h >> 1;
        MINT32 halfOutWidth = outImgSize.w  >> 1;
        //
        for (int i = 0; i < inImgSize.h; ++i) {
            memcpy(outAddr0, inAddr, inImgSize.w);
            outAddr0 += outImgSize.w;
            inAddr += inImgSize.w;
        }
        //
        for (int i = 0 ; i < halfInHeight; ++i) {
            memset(outAddr1, 128, halfInWidth);
            memset(outAddr2, 128, halfInWidth);
            outAddr1 += halfOutWidth;
            outAddr2 += halfOutWidth;
        }
    }
    else if(displayResult->getImgFormat() == eImgFmt_NV21)
    {
        char* outAddr0   = (char*)displayResult->getBufVA(0);
        char* outAddr1   = (char*)displayResult->getBufVA(1);
        char* inAddr     = (char*)mdpBuffer->mImageBuffer->getBufVA(0);
        MINT32 fullInWidth  = inImgSize.w;
        MINT32 halfInHeight = inImgSize.h >> 1;
        MINT32 fullOutWidth = outImgSize.w;
        //
        for (int i = 0; i < inImgSize.h; ++i) {
            memcpy(outAddr0, inAddr, inImgSize.w);
            outAddr0 += outImgSize.w;
            inAddr += inImgSize.w;
        }
        //
        for (int i = 0 ; i < halfInHeight; ++i) {
            memset(outAddr1, 128, fullInWidth);
            outAddr1 += fullOutWidth;
        }
    }
    mdpBuffer = nullptr;
    return;
}

//************************************************************************
//
//************************************************************************
IImageBuffer* BokehNode::resizeIntoDebugSize(IImageBuffer* depthMap)
{
    if(depthMap == nullptr)
        return nullptr;
    if (mpDpStream == nullptr)
        return nullptr;
    //
    SmartImageBuffer mdpBuffer = mpDepthMapBufPool->request();
    //
    VSDOF::util::sDpIspConfig config = {
        .pDpIspStream = mpDpIspStream,
        .pSrcBuffer   = depthMap,
        .pDstBuffer   = mdpBuffer->mImageBuffer.get(),
        .rotAngle     = 0,
    };
    if(!excuteDpIspStream(config)) {
        CAM_ULOGME("excuteMDP fail.");
    }
    return mdpBuffer->mImageBuffer.get();
}
//************************************************************************
//
//************************************************************************
MVOID BokehNode:: shiftDepthMapValue(IImageBuffer* depthMap, MUINT8 shiftValue)
{
    // offset value
    MUINT8* data = (MUINT8*)depthMap->getBufVA(0);
    MSize const size = depthMap->getImgSize();
    for (int i = 0; i < size.w*size.h ; ++i) {
        *data = *data << shiftValue;
        *data = (MUINT8)std::max(0, std::min((int)*data, 255));
        data++;
    }
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL BokehNode:: createBufferPool(android::sp<ImageBufferPool> &pPool,
    MUINT32 width, MUINT32 height, NSCam::EImageFormat format,
    MUINT32 bufCount, const char* caller, MUINT32 bufUsage, MBOOL continuesBuffer)
{
    CAM_ULOGM_FUNCLIFE();
    MBOOL ret = MFALSE;
    pPool = ImageBufferPool::create(caller, width, height, format, bufUsage, continuesBuffer);
    if(pPool == nullptr)
    {
        ret = MFALSE;
        CAM_ULOGME("Create [%s] failed.", caller);
        goto lbExit;
    }
    for(MUINT32 i=0;i<bufCount;++i)
    {
        if(!pPool->allocate())
        {
            ret = MFALSE;
            CAM_ULOGME("Allocate [%s] working buffer failed.", caller);
            goto lbExit;
        }
    }
    ret = MTRUE;
lbExit:
    return ret;
}

/*******************************************************************************
 *
 ********************************************************************************/
TuningParam BokehNode:: applyISPTuning(const char* name, SmartTuningBuffer& targetTuningBuf,
    const _bokehNode_ispTuningConfig_& ispConfig)
{
    CAM_ULOGMD("+, [%s] reqID=%d bIsResized=%d", name, ispConfig.reqNo, ispConfig.bInputResizeRaw);

    NS3Av3::TuningParam tuningParam = {nullptr};
    tuningParam.pRegBuf = reinterpret_cast<void*>(targetTuningBuf->mpVA);

    MetaSet_T inMetaSet(*ispConfig.pInAppMeta, *ispConfig.pInHalMeta);
    MetaSet_T outMetaSet(*ispConfig.pOutApp  , *ispConfig.pOutHal);
    MetaSet_T resultMeta;

    MUINT8 profile = -1;
    if (tryGetMetadata<MUINT8>(&inMetaSet.halMeta, MTK_3A_ISP_PROFILE, profile))
        CAM_ULOGMD("Profile:%d", profile);
    else
        CAM_ULOGMW("Failed getting profile!");

    // USE resize raw-->set PGN 0
    if (ispConfig.bInputResizeRaw)
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_3A_PGN_ENABLE, 0);
    else
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_3A_PGN_ENABLE, 1);

    #if (SUPPORT_ISP_VER >= 60)
        updateEntry<MUINT8>(&(inMetaSet.halMeta), MTK_ISP_P2_TUNING_UPDATE_MODE, 1);
        ispConfig.pHalIsp->setP2Isp(0, inMetaSet, &tuningParam, &outMetaSet);
    #else
        ispConfig.p3AHAL->setIsp(0, inMetaSet, &tuningParam, &outMetaSet);
    #endif

    { // write ISP resultMeta to input hal Meta
        *(ispConfig.pInHalMeta) += outMetaSet.halMeta;
    }
    CAM_ULOGMD("-, [%s] reqID=%d", name, ispConfig.reqNo);
    return tuningParam;
}
/*******************************************************************************
 *
 ********************************************************************************/
template <typename T>
inline MVOID trySetMetadata(IMetadata* pMetadata, MUINT32 const tag, T const& val)
{
    if( pMetadata == nullptr ) {
        CAM_ULOGMW("pMetadata == nullptr");
        return;
    }
    //
    IMetadata::IEntry entry(tag);
    entry.push_back(val, Type2Type<T>());
    pMetadata->update(tag, entry);
}

template <typename T>
inline MBOOL tryGetMetadata(IMetadata* pMetadata, MUINT32 const tag, T & rVal)
{
    if( pMetadata == nullptr ) {
        CAM_ULOGMW("pMetadata == nullptr");
        return MFALSE;
    }
    //
    IMetadata::IEntry entry = pMetadata->entryFor(tag);
    if( !entry.isEmpty() ) {
        rVal = entry.itemAt(0, Type2Type<T>());
        return MTRUE;
    }
    return MFALSE;
}

template <typename T>
inline MVOID updateEntry(IMetadata* pMetadata, MUINT32 const tag, T const& val)
{
    if( pMetadata == nullptr ) {
        CAM_ULOGMW("pMetadata == nullptr");
        return;
    }

    IMetadata::IEntry entry(tag);
    entry.push_back(val, Type2Type<T>());
    pMetadata->update(tag, entry);
}

MVOID debugDIPParams(const Feature::P2Util::DIPParams& rInputDIPParam)
{
    CAM_ULOGMD("Frame size = %zu", rInputDIPParam.mvDIPFrameParams.size());

    for (size_t index = 0; index < rInputDIPParam.mvDIPFrameParams.size(); ++index)
    {
        auto& frameParam = rInputDIPParam.mvDIPFrameParams.at(index);
        CAM_ULOGMD("=================================================\n"
                   "Frame index = %zu\n"
                   "mStreamTag=%d mSensorIdx=%d\n"
                   "Frame Expected End Time (tv_sec) = %ld\n"
                   "Frame Expected End Time (tv_usec) = %ld\n"
                   "=== mvIn section ==="
                   , index, frameParam.mStreamTag, frameParam.mSensorIdx
                   , frameParam.ExpectedEndTime.tv_sec
                   , frameParam.ExpectedEndTime.tv_usec);

        for (size_t index2=0;index2<frameParam.mvIn.size();++index2)
        {
            Input data = frameParam.mvIn[index2];
            CAM_ULOGMD("Index = %zu\n"
                       "mvIn.PortID.index = %d\n"
                       "mvIn.PortID.type = %d\n"
                       "mvIn.PortID.inout = %d\n"
                       "mvIn.mBuffer=%p "
                        , index2 , data.mPortID.index
                        , data.mPortID.type , data.mPortID.inout
                        , data.mBuffer);
            if (data.mBuffer !=nullptr)
            {
                CAM_ULOGMD("mvIn.mBuffer->getImgSize = %dx%d\n"
                           "mvIn.mBuffer->getImgFormat = %x\n"
                           "mvIn.mBuffer->getPlaneCount = %zu\n "
                            , data.mBuffer->getImgSize().w
                            , data.mBuffer->getImgSize().h
                            , data.mBuffer->getImgFormat()
                            , data.mBuffer->getPlaneCount());
                for (size_t j=0; j<data.mBuffer->getPlaneCount(); j++)
                {
                    CAM_ULOGMD("mvIn.mBuffer->getBufVA(%zu) = %lX\n"
                               "mvIn.mBuffer->getBufStridesInBytes(%zu) = %zu"
                                , j, data.mBuffer->getBufVA(j)
                                , j, data.mBuffer->getBufStridesInBytes(j));
                }
            }
            else
            {
                CAM_ULOGMD("mvIn.mBuffer is nullptr!!");
            }
            CAM_ULOGMD("mvIn.mTransform = %d", data.mTransform);
        }

        CAM_ULOGMD("=== mvOut section ===");
        for (size_t index2=0;index2<frameParam.mvOut.size(); index2++)
        {
            Output data = frameParam.mvOut[index2];
            CAM_ULOGMD("Index = %zu\n"
                       "mvOut.PortID.index = %d\n"
                       "mvOut.PortID.type = %d\n"
                       "mvOut.PortID.inout = %d\n"
                       "mvOut.mBuffer=%p "
                        , index2
                        , data.mPortID.index , data.mPortID.type , data.mPortID.inout
                        , data.mBuffer);
            if (data.mBuffer != nullptr)
            {
                CAM_ULOGMD("mvOut.mBuffer->getImgSize = %dx%d\n"
                           " mvOut.mBuffer->getImgFormat = %x\n"
                           " mvOut.mBuffer->getPlaneCount = %zu"
                            , data.mBuffer->getImgSize().w
                            , data.mBuffer->getImgSize().h
                            , data.mBuffer->getImgFormat()
                            , data.mBuffer->getPlaneCount());
                for (size_t j=0;j<data.mBuffer->getPlaneCount();j++)
                {
                    CAM_ULOGMD("mvOut.mBuffer->getBufVA(%zu) = %lX\n"
                               "mvOut.mBuffer->getBufStridesInBytes(%zu) = %zu "
                                , j, data.mBuffer->getBufVA(j)
                                , j, data.mBuffer->getBufStridesInBytes(j));
                }
            }
            else
            {
                CAM_ULOGMD("mvOut.mBuffer is nullptr!!");
            }
            CAM_ULOGMD("mvOut.mTransform = %d", data.mTransform);
        }

        CAM_ULOGMD("=== mvCropRsInfo section ===");
        for (size_t i=0;i<frameParam.mvCropRsInfo.size(); i++)
        {
            MCrpRsInfo data = frameParam.mvCropRsInfo[i];
            CAM_ULOGMD("Index = %zu\n"
                       "CropRsInfo.mGroupID=%d\n"
                       "CropRsInfo.mResizeDst=%dx%d\n"
                       "CropRsInfo.mCropRect.p_fractional=(%d,%d) \n"
                       "CropRsInfo.mCropRect.p_integral=(%d,%d) \n"
                       "CropRsInfo.mCropRect.s=%dx%d "
                        , i
                        , data.mGroupID
                        , data.mResizeDst.w, data.mResizeDst.h
                        , data.mCropRect.p_fractional.x, data.mCropRect.p_fractional.y
                        , data.mCropRect.p_integral.x, data.mCropRect.p_integral.y
                        , data.mCropRect.s.w, data.mCropRect.s.h);
        }

        CAM_ULOGMD("=== mvModuleData section ===");
        for (size_t i=0;i<frameParam.mvModuleData.size(); i++)
        {
            ModuleInfo data = frameParam.mvModuleData[i];
            CAM_ULOGMD("Index = %zu\nModuleData.moduleTag=%d", i, data.moduleTag);

            _SRZ_SIZE_INFO_ *SrzInfo = (_SRZ_SIZE_INFO_ *) data.moduleStruct;
            CAM_ULOGMD("SrzInfo->in_w=%d\n"
                       "SrzInfo->in_h=%d\n"
                       "SrzInfo->crop_w=%lu\n"
                       "SrzInfo->crop_h=%lu\n"
                       "SrzInfo->crop_x=%d\n"
                       "SrzInfo->crop_y=%d\n"
                       "SrzInfo->crop_floatX=%d\n"
                       "SrzInfo->crop_floatY=%d\n"
                       "SrzInfo->out_w=%d\n"
                       "SrzInfo->out_h=%d "
                        , SrzInfo->in_w
                        , SrzInfo->in_h
                        , SrzInfo->crop_w
                        , SrzInfo->crop_h
                        , SrzInfo->crop_x
                        , SrzInfo->crop_y
                        , SrzInfo->crop_floatX
                        , SrzInfo->crop_floatY
                        , SrzInfo->out_w
                        , SrzInfo->out_h);
        }
        CAM_ULOGMD("TuningData=%p\n=== mvExtraData section ===", frameParam.mTuningData);
        for (size_t i=0;i<frameParam.mvExtraParam.size(); i++)
        {
            auto extraParam = frameParam.mvExtraParam[i];
            if (extraParam.CmdIdx == EPIPE_FE_INFO_CMD)
            {
                FEInfo *feInfo = (FEInfo*) extraParam.moduleStruct;
                CAM_ULOGMD("mFEDSCR_SBIT=%d  mFETH_C=%d  mFETH_G=%d\n"
                           "mFEFLT_EN=%d  mFEPARAM=%d  mFEMODE=%d\n"
                           "mFEYIDX=%d  mFEXIDX=%d  mFESTART_X=%d\n"
                           "mFESTART_Y=%d  mFEIN_HT=%d  mFEIN_WD=%d"
                            , feInfo->mFEDSCR_SBIT, feInfo->mFETH_C , feInfo->mFETH_G
                            , feInfo->mFEFLT_EN   , feInfo->mFEPARAM, feInfo->mFEMODE
                            , feInfo->mFEYIDX     , feInfo->mFEXIDX , feInfo->mFESTART_X
                            , feInfo->mFESTART_Y  , feInfo->mFEIN_HT, feInfo->mFEIN_WD);

            }
            else if (extraParam.CmdIdx == EPIPE_FM_INFO_CMD)
            {
                FMInfo *fmInfo = (FMInfo*) extraParam.moduleStruct;
                CAM_ULOGMD("mFMHEIGHT=%d  mFMWIDTH=%d  mFMSR_TYPE=%d\n"
                           "mFMOFFSET_X=%d  mFMOFFSET_Y=%d  mFMRES_TH=%d\n"
                           "mFMSAD_TH=%d  mFMMIN_RATIO=%d"
                           , fmInfo->mFMHEIGHT, fmInfo->mFMWIDTH, fmInfo->mFMSR_TYPE
                           , fmInfo->mFMOFFSET_X, fmInfo->mFMOFFSET_Y, fmInfo->mFMRES_TH
                           , fmInfo->mFMSAD_TH, fmInfo->mFMMIN_RATIO);
            }
            else if(extraParam.CmdIdx == EPIPE_MDP_PQPARAM_CMD)
            {
                PQParam* param = reinterpret_cast<PQParam*>(extraParam.moduleStruct);
                if (param->WDMAPQParam != nullptr)
                {
                    DpPqParam* dpPqParam = (DpPqParam*)param->WDMAPQParam;
                    DpIspParam& ispParam = dpPqParam->u.isp;
                    VSDOFParam& vsdofParam = dpPqParam->u.isp.vsdofParam;
                    CAM_ULOGMD("WDMAPQParam %p enable = %d, scenario=%d\n"
                               "WDMAPQParam iso = %d, frameNo=%d requestNo=%d\n"
                               "WDMAPQParam lensId = %d, isRefocus=%d defaultUpTable=%d\n"
                               "WDMAPQParam defaultDownTable = %d, IBSEGain=%d"
                               , dpPqParam, dpPqParam->enable, dpPqParam->scenario
                               , ispParam.iso , ispParam.frameNo, ispParam.requestNo
                               , ispParam.lensId , vsdofParam.isRefocus, vsdofParam.defaultUpTable
                               , vsdofParam.defaultDownTable, vsdofParam.IBSEGain);
                }
                if (param->WROTPQParam != nullptr)
                {
                    DpPqParam* dpPqParam = (DpPqParam*)param->WROTPQParam;
                    DpIspParam&ispParam = dpPqParam->u.isp;
                    VSDOFParam& vsdofParam = dpPqParam->u.isp.vsdofParam;
                    CAM_ULOGMD("WROTPQParam %p enable = %d, scenario=%d\n"
                               "WROTPQParam iso = %d, frameNo=%d requestNo=%d\n"
                               "WROTPQParam lensId = %d, isRefocus=%d defaultUpTable=%d\n"
                               "WROTPQParam defaultDownTable = %d, IBSEGain=%d"
                               , dpPqParam, dpPqParam->enable, dpPqParam->scenario
                               , ispParam.iso , ispParam.frameNo, ispParam.requestNo
                               , ispParam.lensId , vsdofParam.isRefocus, vsdofParam.defaultUpTable
                               , vsdofParam.defaultDownTable, vsdofParam.IBSEGain);
                }
            }
        }
    }
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
//=======================================================================================
#else //SUPPORT_VSDOF
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
