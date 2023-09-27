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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#include "P2ANode.h"
#include "EnqueISPWorker.h"

#define PIPE_CLASS_TAG "P2ANode"
#define PIPE_TRACE TRACE_P2A_NODE
#include <featurePipe/core/include/PipeLog.h>

#include <mtkcam/aaa/IIspMgr.h>
#include <isp_tuning/isp_tuning.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

#include <mtkcam/drv/iopipe/SImager/IImageTransform.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <mtkcam/utils/exif/DebugExifUtils.h>
//
#include <mtkcam3/feature/lcenr/lcenr.h>
#include <mtkcam3/feature/msnr/IMsnr.h>
//
#include <cmath>
#include <camera_custom_nvram.h>
#include <debug_exif/cam/dbg_cam_param.h>
//
#include "../../thread/CaptureTaskQueue.h"
#include <MTKDngOp.h>
//
#include <mtkcam3/feature/utils/ISPProfileMapper.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAPTURE_P2A);

using namespace NSCam::NSCamFeature::NSFeaturePipe;
using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSMFB20;
using namespace NSCam::TuningUtils;
using namespace NSCam::NSIoPipe::NSSImager;
using namespace NSCam::NSPipelinePlugin;
using namespace NSIspTuning;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

enum DeubgMode {
    AUTO    = -1,
    OFF     = 0,
    ON      = 1,
};

MBOOL getIsSupportCroppedFSYUV(CaptureFeatureInferenceData& rInfer __unused)
{
    static const MBOOL isForceEnable = []()
    {
        const char* prop = "vendor.debug.camera.capture.enable.croppedfsyuv";
        MBOOL ret = property_get_int32(prop, 0);
        MY_LOGD("prop:%s, value:%d", prop, ret);
        return (ret != 0);
    }();

    MBOOL ret = MFALSE;
    // TODO: determine witch feature we need to do croping for FSRAW to dFSYUV in P2ANode
    // ex: const MBOOL isFeatureEnable = rInfer.hasFeature(FID_NR);
    const MBOOL isFeatureEnable = 0;
    if (isFeatureEnable || isForceEnable) {
        MY_LOGD("is need cropped FSYUV, isFeatureEnable:%d, isForceEnable:%d", isFeatureEnable, isForceEnable);
        ret = MTRUE;
    }
    return ret;
}

P2ANode::P2ANode(NodeID_T nid, const char* name, MINT32 policy, MINT32 priority)
    : CaptureFeatureNode(nid, name, 0, policy, priority)
    , CamNodeULogHandler(Utils::ULog::MOD_CAPTURE_P2A)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mRequestPacks);
    TRACE_FUNC_EXIT();
}

P2ANode::~P2ANode()
{
    TRACE_FUNC_ENTER();

    TRACE_FUNC_EXIT();
}

MVOID P2ANode::setBufferPool(const android::sp<CaptureBufferPool> &pool)
{
    TRACE_FUNC_ENTER();
    mpBufferPool = pool;
    TRACE_FUNC_EXIT();
}

MBOOL P2ANode::onInit()
{
    TRACE_FUNC_ENTER();
    CaptureFeatureNode::onInit();

    for (size_t i = 0; i < mSensorList.size(); i++)
    {
        MINT32 sensorId = mSensorList[i];
        std::string userName = "CFP" + to_string(sensorId);
        ISPOperator opt;
        opt.mIspHal = MAKE_HalISP(sensorId, userName.c_str());
        opt.mP2Opt = new P2Operator(LOG_TAG, sensorId);
        mISPOperators[sensorId] = opt;
    }

    // dual sync info size
    NS3Av3::Buffer_Info info;
    if (mISPOperators[mSensorList[0]].mIspHal->queryISPBufferInfo(info) && info.DCESO_Param.bSupport)
    {
        PlatCap::setSupport(PlatCap::HWCap_DCE, MTRUE);
    }

    mSizeConfig.configSize(info);
    mDualSharedData = std::make_shared<SharedData>();

    MY_LOGD("Support CZ(%d) DRE(%d) HFG(%d) DCE(%d)",
            PlatCap::isSupport(PlatCap::HWCap_ClearZoom),
            PlatCap::isSupport(PlatCap::HWCap_DRE),
            PlatCap::isSupport(PlatCap::HWCap_HFG),
            PlatCap::isSupport(PlatCap::HWCap_DCE));

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onUninit()
{
    TRACE_FUNC_ENTER();

    for (size_t i = 0; i < mSensorList.size(); i++) {
        std::string userName = "CFP" + to_string(mSensorList[i]);
        ISPOperator opt = mISPOperators[mSensorList[i]];
        if (opt.mIspHal) {
            opt.mIspHal->destroyInstance(userName.c_str());
            opt.mIspHal = NULL;
        }
    }
    mISPOperators.clear();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onThreadStart()
{
    TRACE_FUNC_ENTER();

    mpISPProfileMapper = ISPProfileMapper::getInstance();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onThreadStop()
{
    TRACE_FUNC_ENTER();

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL P2ANode::onData(DataID id, const RequestPtr& pRequest)
{
    TRACE_FUNC_ENTER();
    MY_LOGD_IF(mLogLevel, "Frame %d: %s arrived", pRequest->getRequestNo(), PathID2Name(id));

    NodeID_T srcNodeId;
    NodeID_T dstNodeId;
    MBOOL ret = GetPath(id, srcNodeId, dstNodeId);

    if (!ret) {
        MY_LOGD("Can not find the path: %d", id);
        return ret;
    }

    if (dstNodeId != NID_P2A && dstNodeId != NID_P2B) {
        MY_LOGE("Unexpected dstNode node: %s", NodeID2Name(dstNodeId));
        return MFALSE;
    }

    if (pRequest->isReadyFor(dstNodeId)) {
        RequestPack pack = {
            .mNodeId = dstNodeId,
            .mpRequest = pRequest
        };
        MY_LOGD_IF(mLogLevel, "Frame %d: %s enqued", pRequest->getRequestNo(), PathID2Name(id));
        mRequestPacks.enque(pack);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL P2ANode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPack pack;

    CAM_TRACE_CALL();

    if (!waitAllQueue()) {
        return MFALSE;
    }

    if (!mRequestPacks.deque(pack)) {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    } else if (pack.mpRequest == NULL) {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    RequestPtr pRequest = pack.mpRequest;

    TRACE_FUNC_ENTER();

    MBOOL ret = onRequestProcess(pack.mNodeId, pRequest);
    if (ret == MFALSE) {
        pRequest->addParameter(PID_FAILURE, 1);
    }

    // Default timing of next capture is P2 start
    if (pRequest->getParameter(PID_ENABLE_NEXT_CAPTURE) > 0)
    {
        std::lock_guard<std::mutex> guard(pRequest->mLockNextCapture);

        const MBOOL isTimingNotFound = !pRequest->hasParameter(PID_THUMBNAIL_TIMING);
        const MBOOL isP2BeginTiming = (pRequest->getParameter(PID_THUMBNAIL_TIMING) == NSPipelinePlugin::eTiming_P2_Begin);
        const MBOOL isNotCShot = !pRequest->hasParameter(PID_CSHOT_REQUEST);
        if ((isTimingNotFound || isP2BeginTiming) && isNotCShot  && !pRequest->mIsNextCaptureCallBacked)
        {
            const MINT32 frameCount = pRequest->getActiveFrameCount();
            const MINT32 frameIndex = pRequest->getActiveFrameIndex();

            if ((pRequest->isSingleFrame() && pRequest->isActiveFirstFrame()) || pRequest->isActiveLastFrame())
            {
                if (pRequest->mpCallback != NULL) {
                    MY_LOGI("Notify next capture at P2 beginning(%d|%d), isTimingNotFound:%d, isP2BeginTiming:%d, isNotCShot:%d",
                        frameIndex, frameCount, isTimingNotFound, isP2BeginTiming, isNotCShot);
                    pRequest->mpCallback->onContinue(pRequest);
                    pRequest->mIsNextCaptureCallBacked = MTRUE;
                } else {
                    MY_LOGW("have no request callback instance!, isTimingNotFound:%d, isP2BeginTiming:%d, isNotCShot:%d",
                        isTimingNotFound, isP2BeginTiming, isNotCShot);
                }
            }
        }
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

/*******************************************************************************
 *
 ********************************************************************************/
MBOOL
P2ANode::onRequestProcess(NodeID_T nodeId, RequestPtr& pRequest)
{
    // create enqueWorkerInfo that will be used through all EnqueISPWorker
    IEnqueISPWorker::EnqueWorkerInfo enqueWorkerInfo(this, nodeId, pRequest);

#ifdef GTEST
    MY_LOGD("run GTEST, return directly, request:%d", enqueWorkerInfo.mRequestNo);
    dispatch(pRequest);
    return MTRUE;
#endif

    CAM_TRACE_FMT_BEGIN("p2a:process|r%df%ds%d", enqueWorkerInfo.mRequestNo, enqueWorkerInfo.mFrameNo, enqueWorkerInfo.mSensorId);

    MY_LOGI("(%d) +, R/F/S Num: %d/%d/%d", nodeId, enqueWorkerInfo.mRequestNo, enqueWorkerInfo.mFrameNo, enqueWorkerInfo.mSensorId);
    // check drop frame
    if (pRequest->isCancelled())
    {
        MY_LOGD("Cancel, R/F/S Num: %d/%d/%d", enqueWorkerInfo.mRequestNo, enqueWorkerInfo.mFrameNo, enqueWorkerInfo.mSensorId);
        onRequestFinish(nodeId, pRequest);
        return MTRUE;
    }
    //
    MBOOL ret = MTRUE;

    incExtThreadDependency();
    // Create request holder
    shared_ptr<RequestHolder> pHolder(new RequestHolder(pRequest),
            [=](RequestHolder* p) mutable
            {
                if (p->mStatus != OK)
                    p->mpRequest->addParameter(PID_FAILURE, 1);

                p->mpHolders.clear();

                onRequestFinish(nodeId, p->mpRequest);
                decExtThreadDependency();
                delete p;
            }
    );
    // Make sure that requests are finished in order
    {
        shared_ptr<RequestHolder> pLastHolder = mpLastHolder.lock();
        if (pLastHolder != NULL)
            pLastHolder->mpPrecedeOver = pHolder;

        mpLastHolder = pHolder;
    }
    // create shareddata which is used to sync information between master sensor and sub sensor
    shared_ptr<SharedData> pSharedData;
    if (enqueWorkerInfo.isPhysical && (enqueWorkerInfo.mRequestNo == mLastRequestNo) && !pRequest->isMasterIndex(enqueWorkerInfo.mSensorId))
    {
        if (mDualSharedData == NULL)
        {
            pSharedData = std::make_shared<SharedData>();
            MY_LOGW("syncData req(%d) is not existed", enqueWorkerInfo.mRequestNo);
        }
        else
        {
            pSharedData = mDualSharedData;
            MY_LOGD("sync req(%d) physical tuning data from master to slave", enqueWorkerInfo.mRequestNo);
        }
    }
    else
        pSharedData = std::make_shared<SharedData>();

    enqueWorkerInfo.pSharedData = pSharedData;
    enqueWorkerInfo.pHolder     = pHolder;

    // do dualCamCrop when stereoFeatureMode is VSDOF
    if (enqueWorkerInfo.isVsdof && mpFOVCalculator->getIsEnable())
    {
        pRequest->addParameter(PID_IGNORE_CROP, 1);
        MY_LOGD("R/F/S Num: %d/%d/%d set ignore crop", enqueWorkerInfo.mRequestNo, enqueWorkerInfo.mFrameNo, enqueWorkerInfo.mSensorId);
    }

    if (!enqueWorkerInfo.mpIMetaHal->entryFor(MTK_P2NODE_CROP_REGION).isEmpty())
    {
        pRequest->addParameter(PID_ENABLE_P2_CROP, 1);
    }


    if (enqueWorkerInfo.mMsnrmode == MsnrMode_Disable || !enqueWorkerInfo.isRunMSNR)
    {
        pRequest->removeFeature(FID_NR);
        mpISPProfileMapper->removeFeature(MTK_FEATURE_NR, enqueWorkerInfo.mMappingKey);
    }

    // RSZ Main sensor (RAW and YUV)
    BufferID_T uOResizedYuv = enqueWorkerInfo.mpNodeRequest->mapBufferID(TID_MAN_RSZ_YUV, OUTPUT);
    if(uOResizedYuv != NULL_BUFFER)
    {
        MY_LOGD("RSZ Main process");
        EnqueISPWorker_R2Y pWorker_R2Y(&enqueWorkerInfo, IEnqueISPWorker::TASK_RSZ_MAIN);
        ret &= pWorker_R2Y.run();
        if(!ret)
        {
            CAM_ULOGM_FATAL("RSZ main process fail");
            return MFALSE;
        }
    }

    // RAW output
    BufferID_T uORawOut = enqueWorkerInfo.mpNodeRequest->mapBufferID(TID_MAN_FULL_RAW, OUTPUT);
    if(uORawOut != NULL_BUFFER)
    {
        MY_LOGD("output raw");
        EnqueISPWorker_R2R pWorker_R2R(&enqueWorkerInfo);
        ret &= pWorker_R2R.run();
        if(!ret)
        {
            CAM_ULOGM_FATAL("output raw fail");
            return MFALSE;
        }
    }

    // DCE process
    if(enqueWorkerInfo.isRunDCE && !enqueWorkerInfo.isNotApplyDCE)
    {
        {
            MY_LOGD("DCE process");
            EnqueISPWorker_R2Y pWorker_R2Y(&enqueWorkerInfo, IEnqueISPWorker::TASK_DCE_MAIN_1st);
            EnqueISPWorker_R2Y pWorker_R2Y_2round(&enqueWorkerInfo, IEnqueISPWorker::TASK_DCE_MAIN_2nd);
            pWorker_R2Y.addNextTask(&pWorker_R2Y_2round, EnqueISPWorker_R2Y::BEFORE_SETP2ISP);
            ret &= pWorker_R2Y.run();
            if(!ret)
            {
                CAM_ULOGM_FATAL("Main DCE fail");
                return MFALSE;
            }
        }
        if(enqueWorkerInfo.mSubSensorId >= 0 && enqueWorkerInfo.isZoom)
        {
            MY_LOGD("sub sensor DCE process");
            EnqueISPWorker_R2Y pWorker_R2Y(&enqueWorkerInfo, IEnqueISPWorker::TASK_DCE_SUB_1st);
            EnqueISPWorker_R2Y pWorker_R2Y_2round(&enqueWorkerInfo, IEnqueISPWorker::TASK_FULL_SUB);
            pWorker_R2Y.addNextTask(&pWorker_R2Y_2round, EnqueISPWorker_R2Y::BEFORE_SETP2ISP);
            ret &= pWorker_R2Y.run();
            if(!ret)
            {
                CAM_ULOGM_FATAL("Sub DCE fail");
                return MFALSE;
            }
        }
    }
    // Main Full size enque
    else if(enqueWorkerInfo.hasYUVOutput)
    {
        if(enqueWorkerInfo.isYuvRep)
        {
            MY_LOGD("Yuv repro");
            EnqueISPWorker_Y2Y pWorker_Y2Y(&enqueWorkerInfo);
            ret &= pWorker_Y2Y.run();
            if(!ret)
            {
                CAM_ULOGM_FATAL("Yuv repro fail");
                return MFALSE;
            }
        }
        else
        {
            MY_LOGD("Main Full size enque process");
            EnqueISPWorker_R2Y pWorker_R2Y_Full(&enqueWorkerInfo, IEnqueISPWorker::TASK_FULL_MAIN);
            ret &= pWorker_R2Y_Full.run();
            if(!ret)
            {
                CAM_ULOGM_FATAL("Main Full size enque process fail");
                return MFALSE;
            }
        }
    }

    if(enqueWorkerInfo.mSubSensorId >= 0)
    {
        BufferID_T uOBufId = enqueWorkerInfo.mpNodeRequest->mapBufferID(TID_SUB_FULL_YUV, OUTPUT);
        if(uOBufId != NULL_BUFFER)
        {
            MY_LOGD("Sub Full size enque process");
            EnqueISPWorker_R2Y pWorker_R2Y_Fullsub(&enqueWorkerInfo, IEnqueISPWorker::TASK_FULL_SUB);
            ret &= pWorker_R2Y_Fullsub.run();
            if(!ret)
            {
                CAM_ULOGM_FATAL("Sub Full size enque process fail");
                return MFALSE;
            }
        }
        BufferID_T uORszBufId = enqueWorkerInfo.mpNodeRequest->mapBufferID(TID_SUB_RSZ_YUV, OUTPUT);
        if(uORszBufId != NULL_BUFFER)
        {
            MY_LOGD("Sub Resized enque process");
            EnqueISPWorker_R2Y pWorker_R2Y_Fullsub(&enqueWorkerInfo, IEnqueISPWorker::TASK_RSZ_SUB);
            ret &= pWorker_R2Y_Fullsub.run();
            if(!ret)
            {
                CAM_ULOGM_FATAL("Sub Resized enque process fail");
                return MFALSE;
            }
        }
    }
    // main frame & master sensor update share info
    if (pRequest->isActiveFirstFrame() && pRequest->isMasterIndex(enqueWorkerInfo.mSensorId)) {
        mLastRequestNo  = enqueWorkerInfo.mRequestNo;
        mDualSharedData = enqueWorkerInfo.pSharedData;
    }

    MY_LOGI("(%d) -, R/F/S Num: %d/%d/%d", enqueWorkerInfo.mNodeId, enqueWorkerInfo.mRequestNo, enqueWorkerInfo.mFrameNo, enqueWorkerInfo.mSensorId);

    CAM_TRACE_FMT_END();
    return MTRUE;
}


/*******************************************************************************
 *
 ********************************************************************************/

MVOID P2ANode::onRequestFinish(NodeID_T nodeId, RequestPtr& pRequest)
{
    MINT32 requestNo = pRequest->getRequestNo();
    MINT32 frameNo = pRequest->getFrameNo();
    MINT32 sensorId = pRequest->getMainSensorIndex();
    MINT32 subSensorId = pRequest->getSubSensorIndex();
    CAM_TRACE_FMT_BEGIN("p2a:finish|r%df%d%d)", requestNo, frameNo, sensorId);
    MY_LOGI("(%d), R/F/S Num: %d/%d/%d", nodeId, requestNo, frameNo, sensorId);

    const MINT32 frameCount = pRequest->getActiveFrameCount();
    const MINT32 frameIndex = pRequest->getActiveFrameIndex();
    const MBOOL isUnderBSS = pRequest->isUnderBSS();
    const MBOOL isFirstFrame = pRequest->isActiveFirstFrame();

    if (pRequest->getParameter(PID_ENABLE_NEXT_CAPTURE) > 0)
    {
        std::lock_guard<std::mutex> guard(pRequest->mLockNextCapture);

        const MBOOL isTimingFound   = pRequest->hasParameter(PID_THUMBNAIL_TIMING);
        const MBOOL isP2BeginTiming = (pRequest->getParameter(PID_THUMBNAIL_TIMING) == NSPipelinePlugin::eTiming_P2_Begin);
        const MBOOL isP2Timing      = (pRequest->getParameter(PID_THUMBNAIL_TIMING) == NSPipelinePlugin::eTiming_P2);
        const MBOOL isCShot         = pRequest->hasParameter(PID_CSHOT_REQUEST);

        if ((isTimingFound && isP2Timing)
          || isCShot
          || (!pRequest->mIsNextCaptureCallBacked && (!isTimingFound || isP2BeginTiming)))
        {
            if (nodeId == NID_P2B && (pRequest->hasFeature(FID_AINR) || pRequest->hasFeature(FID_AINR_YHDR)) && pRequest->isMainFrame())
            {
                if (isFirstFrame)
                {
                    if (pRequest->mpCallback != NULL) {
                        MY_LOGI("[AINR] Notify next capture at P2 ending(%d|%d, isbss:%d)", frameIndex, frameCount, isUnderBSS);
                        pRequest->mpCallback->onContinue(pRequest);
                        pRequest->mIsNextCaptureCallBacked = MTRUE;
                    } else {
                        MY_LOGW("[AINR] have no request callback instance!");
                    }
                }
            }
            else if ((pRequest->isSingleFrame() && isFirstFrame) || pRequest->isActiveLastFrame())
            {
                if (pRequest->mpCallback != NULL) {
                    MY_LOGI("Notify next capture at P2 ending(%d|%d, isbss:%d)", frameIndex, frameCount, isUnderBSS);
                    pRequest->mpCallback->onContinue(pRequest);
                    pRequest->mIsNextCaptureCallBacked = MTRUE;
                } else {
                    MY_LOGW("have no request callback instance!");
                }
            }
        }
    }

    // Keep the delay request while user assigns a delay postview
    if (pRequest->hasParameter(PID_THUMBNAIL_DELAY))
    {
        MINT32 delay = pRequest->getParameter(PID_THUMBNAIL_DELAY);
        if (delay != 0)
        {
            // Assign to delay request, and increate postview refernce count to keep the handle
            if (isFirstFrame)
            {
                sp<CaptureFeatureNodeRequest> pNodeReq = pRequest->getNodeRequest(nodeId);
                BufferID_T uPostView = pNodeReq->mapBufferID(TID_POSTVIEW, OUTPUT);

                if (uPostView != NULL_BUFFER)
                {
                    mwpDelayRequest = pRequest;
                    pRequest->incBufferRef(uPostView);
                    MY_LOGD("Delay Release: Postview(%d|%d)", frameIndex, frameCount);
                }
            }

            // Decrease the reference count of delayed buffer
            if (frameIndex == delay || frameCount == frameIndex + 1)
            {
                RequestPtr r = mwpDelayRequest.promote();
                if (r != NULL)
                {
                    mwpDelayRequest = NULL;

                    sp<CaptureFeatureNodeRequest> pNodeReq = r->getNodeRequest(nodeId);
                    BufferID_T uPostView = pNodeReq->mapBufferID(TID_POSTVIEW, OUTPUT);

                    if (uPostView != NULL_BUFFER)
                    {
                        r->decBufferRef(uPostView);
                        MY_LOGD("Release: Postview(%d|%d)", frameIndex, frameCount);
                    }
                }
                else
                {
                    MY_LOGW("Release: Postview(%d|%d). But request released earlier", frameIndex, frameCount);
                }

            }
        }
    }

    if (isFirstFrame && mpFOVCalculator->getIsEnable()) {
        updateFovCropRegion(pRequest, sensorId, MID_MAN_IN_HAL);
        updateFovCropRegion(pRequest, subSensorId, MID_SUB_IN_HAL);
    }

    pRequest->decNodeReference(nodeId);

    dispatch(pRequest, nodeId);

    CAM_TRACE_FMT_END();
}

MVOID P2ANode::updateFovCropRegion(RequestPtr& pRequest, MINT32 sensorIndex, MetadataID_T metaId)
{
    const MINT32 requestNo = pRequest->getRequestNo();
    const MINT32 frameNo = pRequest->getFrameNo();
    sp<MetadataHandle> metaPtr = pRequest->getMetadata(metaId);
    IMetadata* pMetaPtr = (metaPtr != nullptr) ? metaPtr->native() : nullptr;
    if (pMetaPtr == nullptr)
    {
        MY_LOGW("failed to get meta, R/F/S Num: %d/%d/%d, metaId:%d", requestNo, frameNo, sensorIndex, metaId);
        return;
    }

    FovCalculator::FovInfo fovInfo;
    if (!mpFOVCalculator->getFovInfo(sensorIndex, fovInfo))
    {
        MY_LOGW("failed to get fovInfo, R/F Num: %d/%d, sensorIndex:%d", requestNo, frameNo, sensorIndex);
        return;
    }

    auto updateCropRegion = [&](IMetadata::IEntry& entry, IMetadata* meta)
    {
        entry.push_back(fovInfo.mFOVCropRegion.p.x, Type2Type<MINT32>());
        entry.push_back(fovInfo.mFOVCropRegion.p.y, Type2Type<MINT32>());
        entry.push_back(fovInfo.mFOVCropRegion.s.w, Type2Type<MINT32>());
        entry.push_back(fovInfo.mFOVCropRegion.s.h, Type2Type<MINT32>());
        meta->update(entry.tag(), entry);
    };

    IMetadata::IEntry fovCropRegionEntry(MTK_STEREO_FEATURE_FOV_CROP_REGION);
    updateCropRegion(fovCropRegionEntry, pMetaPtr);
    MY_LOGD("update fov crop region, R/F Num: %d/%d, sensorIndex:%d, metaId:%d, region:(%d, %d)x(%d, %d)",
        requestNo, frameNo,
        sensorIndex, metaId,
        fovInfo.mFOVCropRegion.p.x, fovInfo.mFOVCropRegion.p.y,
        fovInfo.mFOVCropRegion.s.w, fovInfo.mFOVCropRegion.s.h);
}

MERROR P2ANode::evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInfer)
{
    auto& rSrcData = rInfer.getSharedSrcData();
    auto& rDstData = rInfer.getSharedDstData();
    auto& rFeatures = rInfer.getSharedFeatures();
    auto& rMetadatas = rInfer.getSharedMetadatas();

    const MBOOL isVsdof = isVSDoFMode(mUsageHint);
    const MBOOL isZoom = isZoomMode(mUsageHint);

    // Debug
    if (mDebugItem.mDebugDRE == OFF || !PlatCap::isSupport(PlatCap::HWCap_DRE))
        rInfer.clearFeature(FID_DRE);
    else if (mDebugItem.mDebugDRE == ON)
        rInfer.markFeature(FID_DRE);

    if (mDebugItem.mDebugCZ == OFF || !PlatCap::isSupport(PlatCap::HWCap_ClearZoom))
        rInfer.clearFeature(FID_CZ);
    else if (mDebugItem.mDebugCZ == ON)
        rInfer.markFeature(FID_CZ);

    if (mDebugItem.mDebugHFG == OFF || !PlatCap::isSupport(PlatCap::HWCap_HFG))
        rInfer.clearFeature(FID_HFG);
    else if (mDebugItem.mDebugHFG == ON)
        rInfer.markFeature(FID_HFG);

    if (mDebugItem.mDebugDCE == OFF || !PlatCap::isSupport(PlatCap::HWCap_DCE))
        rInfer.clearFeature(FID_DCE);
    // DCE is default on if support
    else if (mDebugItem.mDebugDCE == ON || PlatCap::isSupport(PlatCap::HWCap_DCE))
        rInfer.markFeature(FID_DCE);

    MBOOL isP2A = nodeId == NID_P2A;
    MBOOL hasMain = MFALSE;
    MBOOL hasSub = MFALSE;

    const MUINT8 reqtIndex = rInfer.getRequestIndex();
    const MUINT8 reqCount = rInfer.getRequestCount();
    const MBOOL isMainFrame = rInfer.isMainFrame();
    const MBOOL isBSS = rInfer.isUnderBSS();
    const MINT32 sensorId = rInfer.mSensorIndex;
    const MINT32 subSensorId = rInfer.mSensorIndex2;

    MY_LOGD("RequestIndex:%d RequestCount:%d isMainFrame:%d isBSS:%d",
            reqtIndex, reqCount, isMainFrame, isBSS);

    auto getFSYUVSize = [](NodeID_T nodeId, CaptureFeatureInferenceData& data) -> MSize
    {
        MSize ret = data.getSize(TID_MAN_FULL_RAW);
        if (getIsSupportCroppedFSYUV(data))
        {
            if ((data.mCroppedYUVSize.w > 0) && (data.mCroppedYUVSize.h > 0))
            {
                ret = data.mCroppedYUVSize;
                data.markSupportCroppedFSYUV(nodeId);
                MY_LOGD("markSupportCroppedFSYUV and get the FSYUVSize:(%d, %d)", ret.w, ret.h);
            } else {
                MY_LOGW("not support CroppedFSYUV due to FSYUVSize:(%d, %d)", data.mCroppedYUVSize.w, data.mCroppedYUVSize.h);
            }
        }
        return ret;
    };
    /*** REFACTOR NOTE: try to shorten this function, ex. split differnt node's operatios, ... ***/
    // Reprocessing
    if (isP2A && rInfer.hasType(TID_MAN_FULL_YUV))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_MAN_FULL_YUV;
        src_0.mSizeId = SID_FULL;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_MAN_FULL_YUV;
        dst_0.mSizeId = SID_FULL;
        dst_0.mSize = rInfer.getSize(TID_MAN_FULL_YUV);
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);

        auto& dst_1 = rDstData.editItemAt(rDstData.add());
        dst_1.mTypeId = TID_MAN_CROP1_YUV;
        dst_1.setAllFmtSupport(MTRUE);

        auto& dst_2 = rDstData.editItemAt(rDstData.add());
        dst_2.mTypeId = TID_MAN_CROP2_YUV;
        dst_2.setAllFmtSupport(MTRUE);

        auto& dst_3 = rDstData.editItemAt(rDstData.add());
        dst_3.mTypeId = TID_MAN_CROP3_YUV;
        dst_3.setAllFmtSupport(MTRUE);

        auto& dst_4 = rDstData.editItemAt(rDstData.add());
        dst_4.mTypeId = TID_JPEG;
        dst_4.setAllFmtSupport(MTRUE);

        auto& dst_5 = rDstData.editItemAt(rDstData.add());
        dst_5.mTypeId = TID_THUMBNAIL;
        dst_5.setAllFmtSupport(MTRUE);

        auto& dst_6 = rDstData.editItemAt(rDstData.add());
        dst_6.mTypeId = TID_MAN_FD_YUV;
        dst_6.setAllFmtSupport(MTRUE);

        auto& dst_7 = rDstData.editItemAt(rDstData.add());
        dst_7.mTypeId = TID_POSTVIEW;
        dst_7.setAllFmtSupport(MTRUE);

        auto& dst_8 = rDstData.editItemAt(rDstData.add());
        dst_8.mTypeId = TID_MAN_MSS_YUV;
        dst_8.setFormat(eImgFmt_MTK_YUV_P012);
        dst_8.addSupportFormat(eImgFmt_MTK_YUV_P010);

        hasMain = MTRUE;
    }
    else if (isP2A && rInfer.hasType(TID_MAN_FULL_RAW))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_MAN_FULL_RAW;
        src_0.mSizeId = SID_FULL;

        auto& src_1 = rSrcData.editItemAt(rSrcData.add());
        src_1.mTypeId = TID_MAN_LCS;
        src_1.mSizeId = SID_ARBITRARY;

        auto& src_2 = rSrcData.editItemAt(rSrcData.add());
        src_2.mTypeId = TID_MAN_LCESHO;
        src_2.mSizeId = SID_ARBITRARY;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_MAN_FULL_YUV;
        dst_0.mSizeId = SID_FULL;
        // Use 10bit as default foramt for ISP 6.0
        if(PlatCap::isSupport(PlatCap::HWCap_Support10Bits))
            dst_0.setFormat(eImgFmt_MTK_YUV_P010);
        else
            dst_0.setFormat(eImgFmt_YV12);
        dst_0.addSupportFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);

        if (isVsdof && mpFOVCalculator->getIsEnable())
        {
            FovCalculator::FovInfo fovInfo;
            if (mpFOVCalculator->getFovInfo(sensorId, fovInfo))
            {
                dst_0.mSize = fovInfo.mDestinationSize;
            }
            else
            {
                dst_0.mSize = rInfer.getSize(TID_MAN_FULL_RAW);
            }
        }
        else
        {
            dst_0.mSize = getFSYUVSize(nodeId, rInfer);
        }

        // should only output working buffer if run DRE
        if (!(PlatCap::isSupport(PlatCap::HWCap_DRE) && rInfer.hasFeature(FID_DRE)))
        {
            auto& dst_1 = rDstData.editItemAt(rDstData.add());
            dst_1.mTypeId = TID_MAN_CROP1_YUV;
            dst_1.setAllFmtSupport(MTRUE);

            auto& dst_2 = rDstData.editItemAt(rDstData.add());
            dst_2.mTypeId = TID_MAN_CROP2_YUV;
            dst_2.setAllFmtSupport(MTRUE);

            auto& dst_3 = rDstData.editItemAt(rDstData.add());
            dst_3.mTypeId = TID_MAN_CROP3_YUV;
            dst_3.setAllFmtSupport(MTRUE);
        }

        auto& dst_4 = rDstData.editItemAt(rDstData.add());
        dst_4.mTypeId = TID_MAN_SPEC_YUV;
        dst_4.setAllFmtSupport(MTRUE);

        auto& dst_5 = rDstData.editItemAt(rDstData.add());
        dst_5.mTypeId = TID_JPEG;
        dst_5.setAllFmtSupport(MTRUE);

        auto& dst_6 = rDstData.editItemAt(rDstData.add());
        dst_6.mTypeId = TID_THUMBNAIL;
        dst_6.setAllFmtSupport(MTRUE);

        auto& dst_7 = rDstData.editItemAt(rDstData.add());
        dst_7.mTypeId = TID_MAN_FD_YUV;
        dst_7.setAllFmtSupport(MTRUE);

        {
            auto& dst_8 = rDstData.editItemAt(rDstData.add());
            dst_8.mTypeId = TID_POSTVIEW;
            dst_8.setAllFmtSupport(MTRUE);
        }

        if (subSensorId >= 0)
        {
            auto& dst_9 = rDstData.editItemAt(rDstData.add());
            dst_9.mTypeId = TID_MAN_CLEAN;
            dst_9.setAllFmtSupport(MTRUE);
        }

        if(PlatCap::isSupport(PlatCap::HWCap_DCE) && rInfer.hasFeature(FID_DCE))
        {
            auto& dst_10 = rDstData.editItemAt(rDstData.add());
            dst_10.mTypeId = TID_MAN_DCES;
            dst_10.mSize = mSizeConfig.mDCES_Size;
            dst_10.setFormat(mSizeConfig.mDCES_Format);
        }

        auto& dst_11 = rDstData.editItemAt(rDstData.add());
        dst_11.mTypeId = TID_MAN_MSS_YUV;
        dst_11.setFormat(eImgFmt_MTK_YUV_P012);
        dst_11.addSupportFormat(eImgFmt_MTK_YUV_P010);

        auto& dst_12 = rDstData.editItemAt(rDstData.add());
        dst_12.mTypeId = TID_MAN_FULL_RAW;
        dst_12.mSizeId = SID_FULL;
        dst_12.mSize = rInfer.getSize(TID_MAN_FULL_RAW);

        auto& dst_13 = rDstData.editItemAt(rDstData.add());
        dst_13.mTypeId = TID_MAN_IMGO_RSZ_YUV;
        dst_13.setFormat(eImgFmt_MTK_YUV_P010);

        hasMain = MTRUE;
    }
    else if (!isP2A && rInfer.hasType(TID_MAN_FULL_RAW))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_MAN_FULL_RAW;
        src_0.mSizeId = SID_FULL;

        auto& src_1 = rSrcData.editItemAt(rSrcData.add());
        src_1.mTypeId = TID_MAN_LCS;
        src_1.mSizeId = SID_ARBITRARY;

        auto& src_2 = rSrcData.editItemAt(rSrcData.add());
        src_2.mTypeId = TID_MAN_LCESHO;
        src_2.mSizeId = SID_ARBITRARY;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_MAN_FULL_PURE_YUV;
        dst_0.mSizeId = SID_FULL;
        if (isVsdof && mpFOVCalculator->getIsEnable())
        {
            FovCalculator::FovInfo fovInfo;
            if (mpFOVCalculator->getFovInfo(sensorId, fovInfo))
            {
                dst_0.mSize = fovInfo.mDestinationSize;
            }
            else
            {
                dst_0.mSize = rInfer.getSize(TID_MAN_FULL_RAW);
            }
        }
        else
        {
            dst_0.mSize = getFSYUVSize(nodeId, rInfer);
        }
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);

        hasMain = MTRUE;
    }

    if (isP2A && rInfer.hasType(TID_SUB_FULL_RAW))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_SUB_FULL_RAW;
        src_0.mSizeId = SID_FULL;

        auto& src_1 = rSrcData.editItemAt(rSrcData.add());
        src_1.mTypeId = TID_SUB_LCS;
        src_1.mSizeId = SID_ARBITRARY;

        auto& src_2 = rSrcData.editItemAt(rSrcData.add());
        src_2.mTypeId = TID_SUB_LCESHO;
        src_2.mSizeId = SID_ARBITRARY;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_SUB_FULL_YUV;
        dst_0.mSizeId = SID_FULL;
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);

        if (isVsdof && mpFOVCalculator->getIsEnable())
        {
            FovCalculator::FovInfo fovInfo;
            if(mpFOVCalculator->getFovInfo(subSensorId, fovInfo))
            {
                dst_0.mSize = fovInfo.mDestinationSize;
            }
            else
            {
                dst_0.mSize = rInfer.getSize(TID_SUB_FULL_RAW);
            }
        }
        else
        {
            dst_0.mSize = rInfer.getSize(TID_SUB_FULL_RAW);
        }

        if(isZoom)
        {
            if(PlatCap::isSupport(PlatCap::HWCap_DCE) && rInfer.hasFeature(FID_DCE))
            {
                auto& dst_1 = rDstData.editItemAt(rDstData.add());
                dst_1.mTypeId = TID_SUB_DCES;
                dst_1.mSize = mSizeConfig.mDCES_Size;
                dst_1.setFormat(mSizeConfig.mDCES_Format);
            }

            auto& dst_2 = rDstData.editItemAt(rDstData.add());
            dst_2.mTypeId = TID_SUB_MSS_YUV;
            dst_2.setFormat(eImgFmt_MTK_YUV_P010);
            dst_2.addSupportFormat(eImgFmt_MTK_YUV_P010);
        }

        hasSub = MTRUE;
    }

    if (isP2A && rInfer.hasType(TID_MAN_RSZ_RAW))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_MAN_RSZ_RAW;
        src_0.mSizeId = SID_RESIZED;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_MAN_RSZ_YUV;
        dst_0.mSizeId = SID_RESIZED;
        dst_0.mSize = rInfer.getSize(TID_MAN_RSZ_RAW);
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);
        hasMain = MTRUE;
    }

    if (isP2A && rInfer.hasType(TID_MAN_RSZ_YUV))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_MAN_RSZ_YUV;
        src_0.mSizeId = SID_RESIZED;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_MAN_RSZ_YUV;
        dst_0.mSizeId = SID_RESIZED;
        dst_0.mSize = rInfer.getSize(TID_MAN_RSZ_YUV);
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);
        hasMain = MTRUE;
    }

    if (isP2A && rInfer.hasType(TID_SUB_RSZ_RAW))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_SUB_RSZ_RAW;
        src_0.mSizeId = SID_RESIZED;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_SUB_RSZ_YUV;
        dst_0.mSizeId = SID_RESIZED;
        dst_0.mSize = rInfer.getSize(TID_SUB_RSZ_RAW);
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);

        hasSub = MTRUE;
    }

    if (isP2A && rInfer.hasType(TID_SUB_RSZ_YUV))
    {
        auto& src_0 = rSrcData.editItemAt(rSrcData.add());
        src_0.mTypeId = TID_SUB_RSZ_YUV;
        src_0.mSizeId = SID_RESIZED;

        auto& dst_0 = rDstData.editItemAt(rDstData.add());
        dst_0.mTypeId = TID_SUB_RSZ_YUV;
        dst_0.mSizeId = SID_RESIZED;
        dst_0.mSize = rInfer.getSize(TID_SUB_RSZ_YUV);
        dst_0.setFormat(eImgFmt_YV12);
        dst_0.setAllFmtSupport(MTRUE);

        hasSub = MTRUE;
    }

    if (hasMain)
    {
        rMetadatas.push_back(MID_MAN_IN_P1_DYNAMIC);
        rMetadatas.push_back(MID_MAN_IN_APP);
        rMetadatas.push_back(MID_MAN_IN_HAL);
        rMetadatas.push_back(MID_MAN_OUT_APP);
        rMetadatas.push_back(MID_MAN_OUT_HAL);

        if (isP2A  && !rInfer.hasFeature(FID_DRE)&& !rInfer.hasFeature(FID_DRE)) {
            if (rInfer.hasFeature(FID_CZ))
                rFeatures.push_back(FID_CZ);
            if (rInfer.hasFeature(FID_HFG))
                rFeatures.push_back(FID_HFG);
        }

    }

    if (isP2A && hasSub)
    {
        rMetadatas.push_back(MID_SUB_IN_P1_DYNAMIC);
        rMetadatas.push_back(MID_SUB_IN_HAL);
    }

    if(!rInfer.addNodeIO(nodeId, rSrcData, rDstData, rMetadatas, rFeatures))
        return BAD_VALUE;
    else
        return OK;
}

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
