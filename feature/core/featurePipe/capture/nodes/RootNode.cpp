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

#include "RootNode.h"

#define PIPE_CLASS_TAG "RootNode"
#define PIPE_TRACE TRACE_ROOT_NODE
#include <featurePipe/core/include/PipeLog.h>

#include <mtkcam3/feature/lmv/lmv_ext.h>
#include <custom/feature/mfnr/camera_custom_mfll.h>
#include <custom/debug_exif/dbg_exif_param.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/hw/IFDContainer.h>
#include <mtkcam/utils/hw/HwTransform.h>

#include <mtkcam/utils/TuningUtils/FileReadRule.h>

#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/aaa/INvBufUtil.h>

#include <mtkcam3/feature/bsscore/IBssCore.h>

#ifdef SUPPORT_MFNR
#include <mtkcam/utils/hw/IBssContainer.h>
#include <mtkcam3/feature/mfnr/IMfllNvram.h>
#include <mtkcam3/feature/mfnr/MfllProperty.h>

#include <custom/feature/mfnr/camera_custom_mfll.h>
#include <custom/debug_exif/dbg_exif_param.h>
#if (MFLL_MF_TAG_VERSION > 0)
using namespace __namespace_mf(MFLL_MF_TAG_VERSION);
#include <tuple>
#include <string>
#endif
#include <fstream>
#include <sstream>

// CUSTOM (platform)
#if MTK_CAM_NEW_NVRAM_SUPPORT
#include <mtkcam/utils/mapping_mgr/cam_idx_mgr.h>
#endif
#include <camera_custom_nvram.h>
#include <featurePipe/capture/exif/ExifWriter.h>
//
using std::vector;
using namespace mfll;

#endif //SUPPORT_MFNR

#define __DEBUG // enable debug

#ifdef __DEBUG
#include <memory>
#define FUNCTION_SCOPE \
auto __scope_logger__ = [](char const* f)->std::shared_ptr<const char>{ \
    CAM_ULOGMD("(%d)[%s] + ", ::gettid(), f); \
    return std::shared_ptr<const char>(f, [](char const* p){CAM_ULOGMD("(%d)[%s] -", ::gettid(), p);}); \
}(__FUNCTION__)
#else
#define FUNCTION_SCOPE
#endif //__DEBUG

#define MY_DBG_COND(level)          __builtin_expect( mDebugLevel >= level, false )
#define MY_LOGD3(...)               do { if ( MY_DBG_COND(3) ) MY_LOGD(__VA_ARGS__); } while(0)

#undef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(param) (param)

#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAPTURE_ROOT);

#define CHECK_OBJECT(x)  do{                                            \
    if (x == nullptr) { MY_LOGE("Null %s Object", #x); return -MFALSE;} \
} while(0)

using namespace NSCam::TuningUtils;
using namespace BSScore;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

class RootNode::FDContainerWraper final
{
public:
    using Ptr = UniquePtr<FDContainerWraper>;

public:
    static Ptr createInstancePtr(const RequestPtr& pRequest, MINT32 sensorIndex);

public:
    MBOOL updateFaceData(const RequestPtr& pRequest, function<MVOID(const MINT32 sensorId, const MPoint& in, MPoint& out)> transferTG2Pipe);

private:
    FDContainerWraper(const NSCamHW::HwMatrix& active2SensorMatrix, sp<IFDContainer>& fdReader);

    ~FDContainerWraper();

private:
    NSCamHW::HwMatrix   mActive2TGMatrix;
    sp<IFDContainer>    mFDReader;
};

auto
RootNode::FDContainerWraper::
createInstancePtr(const RequestPtr& pRequest, MINT32 sensorIndex) -> Ptr
{
    MY_LOGD("create FDContainerWraper ptr, reqNo:%d", pRequest->getRequestNo());

    auto pInMetaHal = pRequest->getMetadata(MID_MAN_IN_HAL);
    IMetadata* pHalMeta = (pInMetaHal != nullptr) ? pInMetaHal->native() : nullptr;
    if(pHalMeta == nullptr) {
        MY_LOGW("failed to create instance, can not get inHalMetadata");
        return MFALSE;
    }
    //
    MINT32 sensorMode;
    if (!tryGetMetadata<MINT32>(pHalMeta, MTK_P1NODE_SENSOR_MODE, sensorMode)) {
        MY_LOGW("failed to create instance, can not get tag MTK_P1NODE_SENSOR_MODE from metadata, addr:%p", pHalMeta);
        return nullptr;
    }
    //
    NSCamHW::HwTransHelper hwTransHelper(sensorIndex);
    NSCamHW::HwMatrix active2TGMatrix;
    if(!hwTransHelper.getMatrixFromActive(sensorMode, active2TGMatrix)) {
        MY_LOGW("failed to create instance, can not get active2TGMatrix, sensorMode:%d", sensorMode);
        return nullptr;
    }
    //
    sp<IFDContainer> fdReader = IFDContainer::createInstance(PIPE_CLASS_TAG, IFDContainer::eFDContainer_Opt_Read);
    if(fdReader == nullptr) {
        MY_LOGW("failed to create instance, can not get fdContainer");
        return MFALSE;
    }

    return Ptr(new FDContainerWraper(active2TGMatrix, fdReader), [](FDContainerWraper* p)
    {
        delete p;
    });
}

auto
RootNode::FDContainerWraper::
updateFaceData(const RequestPtr& pRequest, function<MVOID(const MINT32 sensorId, const MPoint& in, MPoint& out)> transferTG2Pipe) -> MBOOL
{
    const MINT32 reqNo = pRequest->getRequestNo();
    const MINT32 sensorId = pRequest->getMainSensorIndex();
    MY_LOGD("update default fd info for request, reqNo:%d", reqNo);
    //
    auto pInMetaHal = pRequest->getMetadata(MID_MAN_IN_HAL);
    IMetadata* pHalMeta = (pInMetaHal != nullptr) ? pInMetaHal->native() : nullptr;
    if(pHalMeta == nullptr) {
        MY_LOGW("failed to get inHalMetadata, reqNo:%d", reqNo);
        return MFALSE;
    }
    //
    //
    MINT64 frameStartTimestamp = 0;
    if(!tryGetMetadata<MINT64>(pHalMeta, MTK_P1NODE_FRAME_START_TIMESTAMP, frameStartTimestamp)) {
        MY_LOGW("failed to get p1 node frame start timestamp, reqNo:%d", reqNo);
        return MFALSE;
    }
    //
    auto createFaceDataPtr = [&fdReader=mFDReader](MINT64 tsStart, MINT64 tsEnd)
    {
        using FaceData = vector<FD_DATATYPE*>;
        FaceData* pFaceData = new FaceData();
        (*pFaceData) = fdReader->queryLock(tsStart, tsEnd);
        using FaceDataPtr = std::unique_ptr<FaceData, std::function<MVOID(FaceData*)>>;
        return FaceDataPtr(pFaceData, [&fdReader](FaceData* p)
        {
            fdReader->queryUnlock(*p);
            delete p;
        });

    };
    // query fd info by timestamps, fdData must be return after use
    static const MINT64 tolerence = 600000000; // 600ms
    const MINT64 tsStart = frameStartTimestamp - tolerence;
    const MINT64 tsEnd = frameStartTimestamp;
    auto faceDataPtr = createFaceDataPtr(tsStart, tsEnd);
    // back element is the closest to the timestamp we assigned
    auto foundItem = std::find_if(faceDataPtr->rbegin(), faceDataPtr->rend(), [] (const FD_DATATYPE* e)
    {
        return e != nullptr;
    });
    //
    if(foundItem == faceDataPtr->rend())
    {
        MY_LOGW("no faceData, reqNo:%d, timestampRange:(%" PRId64 ", %" PRId64 ")", reqNo, tsStart, tsEnd);
        return MFALSE;
    }
    //
    MtkCameraFaceMetadata faceData = (*foundItem)->facedata;
    const MINT32 faceCount = faceData.number_of_faces;
    if(faceCount == 0)
    {
        MY_LOGD("no faceData, reqNo:%d, count:%d", reqNo, faceCount);
        return MTRUE;
    }

    IMetadata::IEntry entryFaceRects(MTK_FEATURE_FACE_RECTANGLES);
    IMetadata::IEntry entryPoseOriens(MTK_FEATURE_FACE_POSE_ORIENTATIONS);
    for(MINT32 index = 0; index < faceCount; ++index) {
        MtkCameraFace& faceInfo = faceData.faces[index];
        // Face Rectangle
        // the source face data use point left-top and right-bottom to show the fd rectangle
        const MPoint actLeftTop(faceInfo.rect[0], faceInfo.rect[1]);
        const MPoint actRightBottom(faceInfo.rect[2], faceInfo.rect[3]);
        // active to tg domain
        MPoint tgLeftTop;
        MPoint tgRightBottom;
        mActive2TGMatrix.transform(actLeftTop, tgLeftTop);
        mActive2TGMatrix.transform(actRightBottom, tgRightBottom);
        // tg to pipe domain
        MPoint pipeLeftTop;
        MPoint pipeRightBottom;
        transferTG2Pipe(sensorId, tgLeftTop, pipeLeftTop);
        transferTG2Pipe(sensorId, tgRightBottom, pipeRightBottom);
        MY_LOGD("detected face rectangle, reqNo:%d, faceNum:%d/%d, act:(%d, %d)x(%d, %d), tg:(%d, %d)x(%d, %d), pipe:(%d, %d)x(%d, %d)",
            reqNo, index, faceCount,
            actLeftTop.x, actLeftTop.y, actRightBottom.x, actRightBottom.y,
            tgLeftTop.x, tgLeftTop.y, tgRightBottom.x, tgRightBottom.y,
            pipeLeftTop.x, pipeLeftTop.y, pipeRightBottom.x, pipeRightBottom.y);
        // note: we use the MRect to pass points left-top and right-bottom
        entryFaceRects.push_back(MRect(pipeLeftTop, MSize(pipeRightBottom.x, pipeRightBottom.y)), Type2Type<MRect>());
        // Face Pose
        const MINT32 poseX = 0;
        const MINT32 poseY = faceData.fld_rop[index];
        const MINT32 poseZ = faceData.fld_rip[index];
        MY_LOGD("detected face pose orientation, reqNo:%d, faceNum:%d/%d, (x, y, z):(%d, %d, %d)",
            reqNo, index, faceCount, poseX, poseY, poseZ);
        entryPoseOriens.push_back(poseX, Type2Type<MINT32>());    // pose X asix
        entryPoseOriens.push_back(poseY, Type2Type<MINT32>());    // pose Y asix
        entryPoseOriens.push_back(poseZ, Type2Type<MINT32>());    // pose Z asix
    }
    // update to appMeta
    pHalMeta->update(MTK_FEATURE_FACE_RECTANGLES, entryFaceRects);
    pHalMeta->update(MTK_FEATURE_FACE_POSE_ORIENTATIONS, entryPoseOriens);

    return MTRUE;
}

RootNode::FDContainerWraper::
FDContainerWraper(const NSCamHW::HwMatrix& active2SensorMatrix, sp<IFDContainer>& fdReader)
: mActive2TGMatrix(active2SensorMatrix)
, mFDReader(fdReader)
{
    MY_LOGD("ctor:%p", this);
}

RootNode::FDContainerWraper::
~FDContainerWraper()
{
    MY_LOGD("dtor:%p", this);
}

RootNode::RootNode(NodeID_T nid, const char* name, MINT32 policy, MINT32 priority)
    : CamNodeULogHandler(Utils::ULog::MOD_CAPTURE_ROOT)
    , CaptureFeatureNode(nid, name, 0, policy, priority)
    , mFDContainerWraperPtr(nullptr)
    , mDebugLevel(0)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mRequests);
    mDebugDump = property_get_int32("vendor.debug.camera.bss.dump", 0);
    mEnableBSSOrdering = property_get_int32("vendor.debug.camera.bss.enable", 1);
    mDebugDrop = property_get_int32("vendor.debug.camera.bss.drop", -1);
    mDebugLoadIn = property_get_int32("vendor.debug.camera.dumpin.en", -1);

    MY_LOGD("mDebugDump=%d mEnableBSSOrdering=%d mDebugDrop=%d mDebugLoadIn=%d", mDebugDump, mEnableBSSOrdering, mDebugDrop, mDebugLoadIn);
    TRACE_FUNC_EXIT();
}

RootNode::~RootNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL RootNode::onData(DataID id, const RequestPtr& pRequest)
{
    TRACE_FUNC_ENTER();
    MY_LOGD_IF(mLogLevel, "Frame %d: %s arrived", pRequest->getFrameNo(), PathID2Name(id));

    MBOOL ret = MFALSE;
    switch(id)
    {
        case PID_ENQUE:
        {
            const MINT32 frameIndex = pRequest->getPipelineFrameIndex();
            const MBOOL isFirstFrame = pRequest->isPipelineFirstFrame();
            // always update cache face data when delay inference
            const MBOOL isNeededCachedFD = (pRequest->hasDelayInference()) ? MTRUE : pRequest->getParameter(PID_FD_CACHED_DATA);
            const MBOOL sensorId = pRequest->getMainSensorIndex();

            MY_LOGD_IF(mLogLevel, "updateFaceData info., reqNo:%d, frameIndex:%d, isFirstFrame:%d, isNeededCachedFD:%d",
                pRequest->getRequestNo(), frameIndex, isFirstFrame, isNeededCachedFD);

            if (isFirstFrame && isNeededCachedFD) {
                // note: not tread-saft, in this function, we assert that is just only one thread call it
                if(mFDContainerWraperPtr == nullptr) {
                    mFDContainerWraperPtr = FDContainerWraper::createInstancePtr(pRequest, sensorId);
                }
                //
                auto transferTG2Pipe = [this](const MINT32 sensorId, const MPoint& in, MPoint& out) -> MVOID
                {
                    if(!mpFOVCalculator->getIsEnable() || !mpFOVCalculator->transform(sensorId, in, out)) {
                        out = in;
                    }
                };
                mFDContainerWraperPtr->updateFaceData(pRequest, transferTG2Pipe);
            }

#ifdef SUPPORT_MFNR
            auto isManualBypassBSS = [this](const RequestPtr& pRequest) -> bool {
                auto pInMetaHal = pRequest->getMetadata(MID_MAN_IN_HAL);
                IMetadata* pHalMeta = pInMetaHal->native();
                MUINT8 iBypassBSS = 0;
                if (!IMetadata::getEntry<MUINT8>(pHalMeta, MTK_FEATURE_MULTIFRAMENODE_BYPASSED, iBypassBSS)) {
                    MY_LOGW("get MTK_FEATURE_MULTIFRAMENODE_BYPASSED failed, set to 0");
                }

                if(iBypassBSS == MTK_FEATURE_MULTIFRAMENODE_TO_BE_BYPASSED)
                {
                    MY_LOGD("R/F/S: %d/%d/%d is BSS-Bypassed.", pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
                    return true;
                }
                return false;
            };
            auto isApplyBss = [this](const RequestPtr& pRequest) -> bool {
                if (pRequest->getPipelineFrameCount() <= 1)
                    return false;

                MY_LOGD3("pRequest->getPipelineFrameCount() = %d", pRequest->getPipelineFrameCount());

                auto pInMetaHal = pRequest->getMetadata(MID_MAN_IN_HAL);
                IMetadata* pHalMeta = pInMetaHal->native();
                MINT32 selectedBssCount = 0;
                if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_FEATURE_BSS_SELECTED_FRAME_COUNT, selectedBssCount)) {
                    MY_LOGW("get MTK_FEATURE_BSS_SELECTED_FRAME_COUNT failed, set to 0");
                }

                if (selectedBssCount < 1)
                    return false;

                if (CC_UNLIKELY(mfll::MfllProperty::getBss() <= 0)) {
                    MY_LOGD("%s: Bypass bss due to force disable by property", __FUNCTION__);
                    return false;
                }

                return true;
            };

            MBOOL isBSSReq = isApplyBss(pRequest);
            MBOOL isBypassBSS = isManualBypassBSS(pRequest);
            MBOOL isPhysical = pRequest->isPhysicalStream();
            // update BSS REQ STATE
            if(!isBSSReq || isPhysical)
                pRequest->addParameter(PID_BSS_REQ_STATE, BSS_STATE_NOT_BSS_REQ);
            else if(isBypassBSS)
                pRequest->addParameter(PID_BSS_REQ_STATE, BSS_STATE_BYPASS_BSS);
            else
                pRequest->addParameter(PID_BSS_REQ_STATE, BSS_STATE_DO_BSS);
            //
            const MINT32 frameCount = pRequest->getActiveFrameCount();
            if (isBSSReq && !isPhysical)
            {
                mRequests.enque(pRequest);
            } else if (isPhysical && (frameCount > 1)) {
                BssWaitingList& waitingList = mPhysicWaitingMap[sensorId];
                waitingList.mPhysicPendingRequests.push_back(pRequest);
                MY_LOGD("Physical sensorId(%d) Keep waiting, Index:%d Pending/Expected: %zu/%d",
                   sensorId, frameIndex, waitingList.mPhysicPendingRequests.size(), frameCount);

                auto func = [&, sensorId]() {
                    if (waitingList.mPhysicPendingRequests.empty()) {
                        MY_LOGW("Physical pending request is empty");
                        return;
                    }
                    if (waitingList.mWaitForBss != nullptr) {
                        MY_LOGD("Physical sensorId(%d) wait BSS done", sensorId);
                        sem_wait(waitingList.mWaitForBss.get());
                        sem_destroy(waitingList.mWaitForBss.get());
                        waitingList.mWaitForBss = nullptr;
                    }

                    MY_LOGD("physical sensorId %d get golden index/frame: %d/%d", sensorId, mGoldenInfo.first, mGoldenInfo.second);
                    RequestPtr mainRequest = waitingList.mPhysicPendingRequests.at(0);
                    RequestPtr goldenRequest = waitingList.mPhysicPendingRequests.at(mGoldenInfo.first);
                    if (waitingList.mPhysicPendingRequests.size() <= mGoldenInfo.first) {
                        MY_LOGW("Physical sensorId(%d) request cannot get gloden frame index %d, req nums %zu",
                                sensorId, mGoldenInfo.first, waitingList.mPhysicPendingRequests.size());
                        goldenRequest = mainRequest;
                    }
                    if (mainRequest != goldenRequest) {
                        // Append Hal metadata
                        auto mainHalMetaHandle = mainRequest->getMetadata(MID_MAN_IN_HAL);
                        auto goldenHalMetaHandle = goldenRequest->getMetadata(MID_MAN_IN_HAL);
                        if(mainHalMetaHandle != NULL && goldenHalMetaHandle != NULL)
                        {
                            IMetadata* pMainHalMeta = mainHalMetaHandle->native();
                            IMetadata* pGoldenHalMeta = goldenHalMetaHandle->native();
                            (*pGoldenHalMeta) = (*pMainHalMeta) + (*pGoldenHalMeta);
                        }
                        MY_LOGD_IF(mLogLevel, "Physical Set cross   %d   %d",
                            mainRequest->getFrameNo(),
                            goldenRequest->getFrameNo());
                        mainRequest->setCrossRequest(goldenRequest);
                        goldenRequest->setCrossRequest(mainRequest);
                    }
                    for (auto req : waitingList.mPhysicPendingRequests) {
                        if (req->hasDelayInference())
                            req->startInference();

                        dispatch(req);
                    }
                };

                if (pRequest->isActiveFirstFrame() && !waitingList.mGetPhysicalBss.valid()) {
                    waitingList.mWaitForBss = std::make_shared<sem_t>();
                    sem_init(waitingList.mWaitForBss.get(), 0, 0);
                    waitingList.mGetPhysicalBss = std::async(std::launch::deferred, func);
                }

                if (pRequest->isActiveLastFrame() && waitingList.mGetPhysicalBss.valid()) {
                    waitingList.mGetPhysicalBss.get();
                    waitingList.mPhysicPendingRequests.clear();
                }
            } else
#endif

            {
                if (pRequest->hasDelayInference())
                    pRequest->startInference();

                dispatch(pRequest);
            }
            ret = MTRUE;
            break;
        }
        default:
        {
            ret = MFALSE;
            MY_LOGE("unknown data id :%d", id);
            break;
        }
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL RootNode::onInit()
{
    TRACE_FUNC_ENTER();
    CaptureFeatureNode::onInit();
#ifdef SUPPORT_MFNR
    mDebugLevel = mfll::MfllProperty::readProperty(mfll::Property_LogLevel);
    mDebugLevel = max(property_get_int32("vendor.debug.camera.bss.log", mDebugLevel), mDebugLevel);
#endif
    UNREFERENCED_PARAMETER(mDebugLevel);
    TRACE_FUNC_EXIT();
    return MTRUE;
}


MBOOL RootNode::onThreadStart()
{
    return MTRUE;
}

MBOOL RootNode::onThreadStop()
{
    return MTRUE;
}

MERROR RootNode::evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInfer)
{
    (void) rInfer;
    (void) nodeId;
    return OK;
}

MBOOL RootNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr pRequest;

    CAM_TRACE_CALL();

    if (!waitAllQueue()) {
        return MFALSE;
    }

    {
        Mutex::Autolock _l(mLock);
        if (!mRequests.deque(pRequest)) {
            return MFALSE;
        } else if (pRequest == NULL) {
            return MFALSE;
        }

        onRequestProcess(pRequest);

        if (mbWait && mRequests.size() == 0) {
            mbWait = false;
            mWaitCondition.broadcast();
        }
    }

    return MTRUE;
}

MBOOL RootNode::onAbort(RequestPtr& pRequest)
{
    Mutex::Autolock _l(mLock);

    MBOOL bRestored = MFALSE;

    if (mRequests.size() > 0) {
        mbWait = true;
        MY_LOGI("Wait+:deque R/F/S Num: %d/%d/%d, request size %zu",
                    pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex(), mRequests.size());
        auto ret = mWaitCondition.waitRelative(mLock, 100000000); // 100msec
        if (ret != OK) {
            MY_LOGW("wait timeout!!");
        }
        MY_LOGI("Wait-");
    }

    MINT32 bypassBSSCount = mvBypassBSSRequest.size();
    auto aborthRequestInsideContatiner = [&] (Vector<RequestPtr>& container)
    {
        for (auto it = container.begin(); it != container.end(); it++) {
            if (it == nullptr) {
                MY_LOGE("%s: iterator is nullptr, should not happen", __FUNCTION__);
                break;
            }
            if ((*it) != pRequest)
                continue;

            container.erase(it);
#ifdef SUPPORT_MFNR
            if (pRequest->hasFeature(FID_MFNR)) {
                pRequest->addParameter(PID_RESTORED, 1);
                pRequest->mpCallback->onRestored(pRequest);
                bRestored = MTRUE;
                mvRestoredRequests.push_back(pRequest);
                MY_LOGI("Restore R/F/S Num: %d/%d/%d", pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
            } else
#endif
            {
                MY_LOGI("Abort R/F/S Num: %d/%d/%d", pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
                onRequestFinish(pRequest);
            }
            UNREFERENCED_PARAMETER(bypassBSSCount);
            break;
        }
    };

    aborthRequestInsideContatiner(mvPendingRequests);
    aborthRequestInsideContatiner(mvBypassBSSRequest);

#ifdef SUPPORT_MFNR
    MINT32 i4RestoredCount = mvRestoredRequests.size();
    //
    if (mvPendingRequests.size() == 0 && mvBypassBSSRequest.size() == 0 && i4RestoredCount != 0) {
        MY_LOGI("Restored requests[%zu], R/F/S Num: %d/%d/%d",
                mvRestoredRequests.size(), pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());

        for (size_t idx = 0; idx < mvRestoredRequests.size(); idx++) {
            auto pRestored = mvRestoredRequests.editItemAt(idx);
            if (pRestored == nullptr) {
                MY_LOGE("request in mvRestoredRequests in nullptr!!");
                continue;
            }
            pRestored->addParameter(PID_FRAME_COUNT, i4RestoredCount);
            pRestored->addParameter(PID_BSS_BYPASSED_COUNT, bypassBSSCount);
        }
        doBss(mvRestoredRequests);
        mvRestoredRequests.clear();
    }

    if (pRequest->isPhysicalStream()) {
        BssWaitingList& waitingList = mPhysicWaitingMap[pRequest->getMainSensorIndex()];
        for (auto it = waitingList.mPhysicPendingRequests.begin(); it != waitingList.mPhysicPendingRequests.end(); it++) {
            if ((*it) != pRequest)
                continue;
            pRequest->addParameter(PID_RESTORED, 1);
            pRequest->mpCallback->onRestored(pRequest);
            bRestored = MTRUE;
            MY_LOGI("Restore physical R/F/S Num: %d/%d/%d", pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
        }
        if (waitingList.mGetPhysicalBss.valid() && (pRequest == waitingList.mPhysicPendingRequests.back())) {
            waitingList.mGetPhysicalBss.get();
            waitingList.mPhysicPendingRequests.clear();
        }
    }
#endif

    return !bRestored;
}

MBOOL RootNode::onRequestProcess(RequestPtr& pRequest)
{
    this->incExtThreadDependency();

    if (pRequest->isCancelled() && !mbWait) {
        MY_LOGD("Cancel, R/F/S Num: %d/%d/%d", pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
        onRequestFinish(pRequest);
        return MFALSE;
    }

    if (!pRequest->isBypassBSS())
        mvPendingRequests.push_back(pRequest);
    else
        mvBypassBSSRequest.push_back(pRequest);

    auto frameIndex = pRequest->getPipelineFrameIndex();
    auto frameCount = pRequest->getPipelineFrameCount();
    // wait request
    if (mvPendingRequests.size() + mvBypassBSSRequest.size() < (size_t)frameCount) {
        MY_LOGD("BSS: Keep waiting, Index:%d Pending/ByPassBSS/Expected: %zu/%zu/%d",
                frameIndex, mvPendingRequests.size(), mvBypassBSSRequest.size(), frameCount);
    } else {
        MY_LOGD("BSS: Do re-order, Index:%d Pending/ByPassBSS/Expected: %zu/%zu/%d",
                frameIndex, mvPendingRequests.size(), mvBypassBSSRequest.size(), frameCount);

        Vector<RequestPtr> vReadyRequests = mvPendingRequests;
        // add BSS-bypass count parameters
        for(size_t index = 0;index < vReadyRequests.size();++index)
        {
            auto& pRequest = vReadyRequests.editItemAt(index);
            pRequest->addParameter(PID_BSS_BYPASSED_COUNT, mvBypassBSSRequest.size());
        }
        for(size_t index = 0;index < mvBypassBSSRequest.size();++index)
        {
            auto& pRequest = mvBypassBSSRequest.editItemAt(index);
            pRequest->addParameter(PID_BSS_BYPASSED_COUNT, mvBypassBSSRequest.size());
        }
        mvPendingRequests.clear();
#ifdef SUPPORT_MFNR

        doBss(vReadyRequests);

        // dispatch the bypass-bss request
        for(size_t index = 0;index < mvBypassBSSRequest.size();++index)
        {
            auto& pRequest = mvBypassBSSRequest.editItemAt(index);
            MY_LOGD("dispatch bypass requests: R/F/S: %d/%d/%d", pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
            onRequestFinish(pRequest);
        }
        mvBypassBSSRequest.clear();
#else
        MY_LOGE("Not support BSS: dispatch");
        return MFALSE;
#endif
    }

    return MTRUE;
}

MVOID RootNode::onRequestFinish(const RequestPtr& pRequest)
{
    if (pRequest->hasDelayInference())
        pRequest->startInference();

    if (mDebugDump && this->needToDump(pRequest)) {
        MINT32 uniqueKey = 0;
        MINT32 requestNo = pRequest->getRequestNo();
        MINT32 frameNo = pRequest->getFrameNo();
        MINT32 sensorId = pRequest->getMainSensorIndex();
        MINT32 iso = 0;
        MINT64 exp = 0;

        auto pInMetaHalHnd = pRequest->getMetadata(MID_MAN_IN_HAL);
        auto pInMetaDynamicHnd = pRequest->getMetadata(MID_MAN_IN_P1_DYNAMIC);

        IMetadata* pInMetaHal = pInMetaHalHnd->native();
        IMetadata* pInMetaDynamic = pInMetaDynamicHnd->native();

        tryGetMetadata<MINT32>(pInMetaHal, MTK_PIPELINE_UNIQUE_KEY, uniqueKey);
        tryGetMetadata<MINT32>(pInMetaDynamic, MTK_SENSOR_SENSITIVITY, iso);
        tryGetMetadata<MINT64>(pInMetaDynamic, MTK_SENSOR_EXPOSURE_TIME, exp);
        // convert ns into us
        exp /= 1000;

        char filename[256] = {0};
        FILE_DUMP_NAMING_HINT hint;
        hint.UniqueKey          = uniqueKey;
        hint.RequestNo          = requestNo;
        hint.FrameNo            = frameNo;

        extract_by_SensorOpenId(&hint, sensorId);
        auto DumpYuvBuffer = [&](IImageBuffer* pImgBuf, YUV_PORT port, const char* pStr) -> MVOID {
            if (pImgBuf == NULL)
                return;

            extract(&hint, pImgBuf);
            genFileName_YUV(filename, sizeof(filename), &hint, port, pStr);
            pImgBuf->saveToFile(filename);
            MY_LOGD("Dump YUV: %s", filename);
        };
        auto DumpRawBuffer = [&](IImageBuffer* pImgBuf, RAW_PORT port, const char* pStr) -> MVOID {
            if (pImgBuf == NULL)
                return;

            extract(&hint, pImgBuf);
            genFileName_RAW(filename, sizeof(filename), &hint, port, pStr);
            pImgBuf->saveToFile(filename);
            MY_LOGD("Dump RAW: %s", filename);
        };
        auto DumpLcsBuffer = [&](IImageBuffer* pImgBuf, const char* pStr) -> MVOID {
            if (pImgBuf == NULL)
                return;

            extract(&hint, pImgBuf);
            genFileName_LCSO(filename, sizeof(filename), &hint, pStr);
            pImgBuf->saveToFile(filename);
            MY_LOGD("Dump LCEI: %s", filename);
        };

        auto pInBufRszHnd = pRequest->getBuffer(BID_MAN_IN_RSZ);
        if (pInBufRszHnd != NULL) {
            IImageBuffer* pInBufRsz = pInBufRszHnd->native();
            if(NSCam::isHalRawFormat(static_cast<EImageFormat>(pInBufRsz->getImgFormat())))
            {
                String8 str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-rrzo", iso, exp);
                DumpRawBuffer(pInBufRsz, RAW_PORT_NULL, str.string());
            }
            else
            {
                String8 str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-yuvo", iso, exp);
                DumpYuvBuffer(pInBufRsz, YUV_PORT_NULL, str.string());
            }
        }

        // Dump imgo and lcso for skip frame
        if (pRequest->hasParameter(PID_DROPPED_FRAME)) {
            String8 str;
            hint.SensorDev = -1;
            MY_LOGD("== dump drop frame(R/F/S Num: %d/%d/%d) for MFNR == ", requestNo, frameNo, sensorId);
            auto pInBufLcsHnd = pRequest->getBuffer(BID_MAN_IN_LCS);
            if (pInBufLcsHnd != NULL) {
                IImageBuffer* pInBufLcs = pInBufLcsHnd->native();
                str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-lcso", iso, exp);
                    DumpLcsBuffer(pInBufLcs, str.string());
            }

            auto pInBufFullHnd = pRequest->getBuffer(BID_MAN_IN_FULL);
            if (pInBufFullHnd != NULL) {
                IImageBuffer* pInBufFull = pInBufFullHnd->native();
                String8 str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-raw", iso, exp);
                DumpRawBuffer(pInBufFull, RAW_PORT_NULL, str.string());
            }
            MY_LOGD("==============================================");
        }
    }
    MY_LOGD_IF(mLogLevel, "dispatch  I/C:%d/%d R/F/S:%d/%d/%d isCross:%d",
                pRequest->getActiveFrameIndex(), pRequest->getActiveFrameCount(),
                pRequest->getRequestNo(), pRequest->getFrameNo(),
                pRequest->getMainSensorIndex(), pRequest->isCross());
    dispatch(pRequest);
    this->decExtThreadDependency();
}

#ifdef SUPPORT_MFNR
// /*******************************************************************************
//  *
//  ********************************************************************************/

MBOOL
RootNode::
doBss(Vector<RequestPtr>& rvReadyRequests)
{
    FUNCTION_SCOPE;

    MBOOL ret = MFALSE;
    MINT32 frameNum = rvReadyRequests.size();

    if (frameNum == 0)
        return MTRUE;
    else if (frameNum == 1) {
        // won't do bss, set not bss request
        rvReadyRequests[0]->addParameter(PID_BSS_REQ_STATE, BSS_STATE_NOT_BSS_REQ);
        onRequestFinish(rvReadyRequests[0]);
        mGoldenInfo = std::make_pair<int, int>(rvReadyRequests[0]->getPipelineFrameIndex(), rvReadyRequests[0]->getFrameNo());
        for (const auto &p : mPhysicWaitingMap) {
            const BssWaitingList& list = p.second;
            if (list.mWaitForBss != nullptr) {
                sem_post(list.mWaitForBss.get());
            }
        }
        return MTRUE;
    }

    // Check each frame's required buffer
    Vector<RequestPtr> vInvalidRequests;
    for (size_t idx = 0; idx < rvReadyRequests.size();) {
        auto pRequest = rvReadyRequests.editItemAt(idx);
        if (pRequest == nullptr) {
            MY_LOGE("req in rvReadyRequests[%zu] is nullptr!!", idx);
            idx++;
            continue;
        }

        if (pRequest->getMetadata(MID_MAN_IN_HAL) == NULL)
        {
            vInvalidRequests.push_back(pRequest);
            rvReadyRequests.removeItemsAt(idx);
            MY_LOGW("Have no required HAL_META(%d), R/F/S Num: %d/%d/%d",
                    pRequest->getMetadata(MID_MAN_IN_HAL) == NULL,
                    pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
        } else
            idx++;
    }

    auto& pMainRequest = rvReadyRequests.editItemAt(0);
    CAM_TRACE_FMT_BEGIN("doBss req(%d)", pMainRequest->getRequestNo());

    ExifWriter writer(PIPE_CLASS_TAG);
    // set all sub requests belong to main request
    set<MINT32> mappingSet;
    MINT32 reqId = rvReadyRequests[0]->getRequestNo();
    const RequestPtr& reqMain = rvReadyRequests[0];
    for(size_t idx = 0; idx < rvReadyRequests.size(); idx++){
        auto req = rvReadyRequests.editItemAt(idx);
        if (req == nullptr) {
            MY_LOGE("request in rvReadyRequests in nullptr!!");
            continue;
        }
        mappingSet.insert(req->getRequestNo());
    }
    writer.addReqMapping(reqId, mappingSet);

    std::shared_ptr<IBssCore> spBssCore = IBssCore::createInstance();
    if (spBssCore.get() == NULL) {
        MY_LOGE("create IBssCore instance failed");
        return MFALSE;
    }

    Vector<MUINT32> BSSOrder;

    Vector<RequestPtr> vOrderedRequests;
    int frameNumToSkip = 0;
    spBssCore->Init(reqMain->getMainSensorIndex());

    Vector<std::shared_ptr<BssRequest>> myReadyRequests;
    for(int i=0; i<rvReadyRequests.size(); i++) {
        auto pInMetaHal = rvReadyRequests[i]->getMetadata(MID_MAN_IN_HAL);
        auto pInMetaApp = rvReadyRequests[i]->getMetadata(MID_MAN_IN_APP);
        auto pOutMetaApp = rvReadyRequests[i]->getMetadata(MID_MAN_IN_P1_DYNAMIC);
        auto pInBufRszHnd = rvReadyRequests[i]->getBuffer(BID_MAN_IN_RSZ);
        IMetadata* pHalMeta = pInMetaHal ? pInMetaHal->native() : NULL;
        IMetadata* pAppMeta = pInMetaApp ? pInMetaApp->native() : NULL;
        IMetadata* pAppMetaOut = pOutMetaApp ? pOutMetaApp->native() : NULL;
        IImageBuffer* pInBufRsz = pInBufRszHnd ? pInBufRszHnd->native() : NULL;
        MINT32 requestNo = rvReadyRequests[i]->getRequestNo();
        MINT32 frameNo = rvReadyRequests[i]->getFrameNo();

        std::shared_ptr<BssRequest> bssrequest = std::make_shared<BssRequest>(pHalMeta, pAppMeta, pAppMetaOut, pInBufRsz, requestNo, frameNo);
        myReadyRequests.push_back(bssrequest);
    }
    ret = spBssCore->doBss(myReadyRequests, BSSOrder, frameNumToSkip);

    if (mEnableBSSOrdering == 0 || !ret) {
        BSSOrder.clear();
        for (size_t i = 0; i < rvReadyRequests.size(); i++)
            BSSOrder.push_back(i);
        MY_LOGD("Disable reorder due to BSSOrdering(%d), doBss(%d)", mEnableBSSOrdering, ret);
    }

    //save re-ordered requestes vOrderedRequests
    for (size_t i = 0; i < rvReadyRequests.size(); i++) {
        vOrderedRequests.push_back(rvReadyRequests[BSSOrder[i]]);
        rvReadyRequests[BSSOrder[i]]->addParameter(PID_BSS_ORDER, i);
        MY_LOGD3("BSSOrder: %d", BSSOrder[i]);
    }

    const map<MINT32, MINT32>& data = spBssCore->getExifDataMap();
    MY_LOGD3("Exif data size: %zu", data.size());
    for(auto it = data.begin(); it != data.end(); it++) {
        writer.sendData(reqId, it->first, it->second);
    }
    writer.makeExifFromCollectedData(reqMain);

    // write golden frame index
    auto& pBestRequest = vOrderedRequests.editItemAt(0);
    auto pOutMetaApp = reqMain->getMetadata(MID_MAN_OUT_APP);
    IMetadata* pOAppMeta = (pOutMetaApp != nullptr) ? pOutMetaApp->native() : nullptr;
    IMetadata::setEntry<MINT32>(pOAppMeta, MTK_MFNR_FEATURE_BSS_GOLDEN_INDEX, pBestRequest->getPipelineFrameIndex());

    // Ignore the invalid request to BSS algo, and treat them as dropped frames
    if (vInvalidRequests.size() > 0) {
        rvReadyRequests.appendVector(vInvalidRequests);
        vOrderedRequests.appendVector(vInvalidRequests);
        frameNumToSkip += vInvalidRequests.size();
    }

    if (mDebugDrop > 0)
        frameNumToSkip = mDebugDrop;

    reorder(rvReadyRequests, vOrderedRequests, frameNumToSkip);
    CAM_TRACE_FMT_END();

    return ret;
}

// /*******************************************************************************
//  *
//  ********************************************************************************/

MVOID RootNode::syncGoldenFrameMeta(Vector<RequestPtr>& rvOrderedRequests, size_t i4SkipFrmCnt, MetadataID_T metaId)
{
    if (rvOrderedRequests.empty() || i4SkipFrmCnt >= rvOrderedRequests.size()) {
        MY_LOGE("%s: Cannot update metadata due to no avaliable request, metaId:%d", __FUNCTION__, metaId);
        return;
    }
    sp<MetadataHandle> pInMetaHal = rvOrderedRequests.editItemAt(0)->getMetadata(metaId);
    if (pInMetaHal == NULL) {
        MY_LOGD("%s: no neet to update, cannot get the halMetaHandle, metaId:%d", __FUNCTION__, metaId);
        return;
    }

    IMetadata* pHalMeta = pInMetaHal->native();
    if (pHalMeta == NULL) {
        MY_LOGD("%s: no neet to update, cannot get the halMetaPtr, metaId:%d", __FUNCTION__, metaId);
        return;
    }

    // Fix LSC
    {
        MUINT8 fixLsc = 0;
        if (CC_UNLIKELY( !IMetadata::getEntry<MUINT8>(pHalMeta, MTK_FEATURE_BSS_FIXED_LSC_TBL_DATA, fixLsc) )) {
            MY_LOGD("%s: cannot get MTK_FEATURE_BSS_FIXED_LSC_TBL_DATA, metaId:%d", __FUNCTION__, metaId);
        }
        if (fixLsc) {
            MY_LOGD3("%s: Fixed LSC due to MTK_FEATURE_BSS_FIXED_LSC_TBL_DATA is set, metaId:%d", __FUNCTION__, metaId);
            auto pBestMetaHal = rvOrderedRequests.editItemAt(0)->getMetadata(metaId);
            IMetadata* pBestHalMeta = pBestMetaHal->native();
            IMetadata::Memory prLscData;
            if (CC_LIKELY( IMetadata::getEntry<IMetadata::Memory>(pBestHalMeta, MTK_LSC_TBL_DATA, prLscData) )) {
                for (size_t i = 1 ; i < rvOrderedRequests.size(); i++) {
                    auto pInRefMetaHal = rvOrderedRequests.editItemAt(i)->getMetadata(metaId);
                    IMetadata* pRefHalMeta = pInRefMetaHal->native();
                    IMetadata::setEntry<IMetadata::Memory>(pRefHalMeta, MTK_LSC_TBL_DATA, prLscData);
                }
            } else {
                MY_LOGW("%s: cannot get MTK_LSC_TBL_DATA at idx, metaId:%d", __FUNCTION__, metaId);
            }
        }
    }
    // metadata keep in first
    {
        MUINT8 keepIsp = -1;
        if (CC_UNLIKELY( !IMetadata::getEntry<MUINT8>(pHalMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, keepIsp) )) {
            MY_LOGD("%s: cannot get MTK_ISP_P2_TUNING_UPDATE_MODE, metaId:%d", __FUNCTION__, metaId);
        } else {
            MY_LOGD3("%s: Update keepIsp due to MTK_ISP_P2_TUNING_UPDATE_MODE is set, metaId:%d, tuning mode before: %d", __FUNCTION__, metaId, keepIsp);
            for (size_t i = 0 ; i < rvOrderedRequests.size(); i++) {
                auto pInRefMetaHal = rvOrderedRequests.editItemAt(i)->getMetadata(metaId);
                IMetadata* pRefHalMeta = pInRefMetaHal->native();
                if (metaId == MID_MAN_IN_HAL)
                    IMetadata::setEntry<MUINT8>(pRefHalMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, i?2:0);
            }
        }
    }
    // metadata keep in last
    {
        MUINT8 focusPause = -1;
        if (CC_UNLIKELY( !IMetadata::getEntry<MUINT8>(pHalMeta, MTK_FOCUS_PAUSE, focusPause) )) {
            MY_LOGD("%s: cannot get MTK_FOCUS_PAUSE, metaId:%d", __FUNCTION__, metaId);
        } else {
            MY_LOGD3("%s: Update focusPause due to MTK_FOCUS_PAUSE is set, metaId:%d", __FUNCTION__, metaId);
            size_t resumeIdx = rvOrderedRequests.size() - i4SkipFrmCnt - 1; //i4SkipFrmCnt < rvOrderedRequests.size()
            for (size_t i = 0 ; i < rvOrderedRequests.size(); i++) {
                auto pInRefMetaHal = rvOrderedRequests.editItemAt(i)->getMetadata(metaId);
                IMetadata* pRefHalMeta = pInRefMetaHal->native();
                IMetadata::setEntry<MUINT8>(pRefHalMeta, MTK_FOCUS_PAUSE, (i<resumeIdx)?0:1);
            }
        }
    }
}

MVOID RootNode::updateInOrderMetadata(RequestPtr pGoldenRequest, RequestPtr pFirstPipelineRequest)
{
    if(pGoldenRequest == pFirstPipelineRequest)
        return;
    sp<MetadataHandle> pInMetaHalGolden = pGoldenRequest->getMetadata(MID_MAN_IN_HAL);
    sp<MetadataHandle> pInMetaHalFirst = pFirstPipelineRequest->getMetadata(MID_MAN_IN_HAL);

    MUINT8 dceControl_golden = MTK_FEATURE_CAP_PIPE_DCE_DEFAULT_APPLY;
    MUINT8 dceControl_first = MTK_FEATURE_CAP_PIPE_DCE_DEFAULT_APPLY;

    MBOOL bRet = tryGetMetadata<MUINT8>(pInMetaHalGolden->native(), MTK_FEATURE_CAP_PIPE_DCE_CONTROL, dceControl_golden);
    bRet |= tryGetMetadata<MUINT8>(pInMetaHalFirst->native(), MTK_FEATURE_CAP_PIPE_DCE_CONTROL, dceControl_first);

    // switch the need-in-order value
    if(bRet)
    {
        IMetadata::setEntry<MUINT8>(pInMetaHalGolden->native(), MTK_FEATURE_CAP_PIPE_DCE_CONTROL, dceControl_first);
        IMetadata::setEntry<MUINT8>(pInMetaHalFirst->native(), MTK_FEATURE_CAP_PIPE_DCE_CONTROL, dceControl_golden);
    }
}

MVOID RootNode::updateFaceData(RequestPtr pGoldenRequest, RequestPtr pFirstPipelineRequest)
{
    if(pGoldenRequest == pFirstPipelineRequest)
        return;
    sp<MetadataHandle> pInMetaHalGolden = pGoldenRequest->getMetadata(MID_MAN_IN_HAL);
    sp<MetadataHandle> pInMetaHalFirst = pFirstPipelineRequest->getMetadata(MID_MAN_IN_HAL);

    IMetadata* pInHalGolden = (pInMetaHalGolden != nullptr) ? pInMetaHalGolden->native() : nullptr;
    IMetadata* pInHalFirst = (pInMetaHalFirst != nullptr) ? pInMetaHalFirst->native() : nullptr;

    if ((pInHalFirst != nullptr) && (pInHalGolden != nullptr)) {
        IMetadata::IEntry entryFaceRects = pInHalFirst->entryFor(MTK_FEATURE_FACE_RECTANGLES);
        pInHalGolden->update(MTK_FEATURE_FACE_RECTANGLES, entryFaceRects);
        MY_LOGD("update faceData from main frame to golden frame");
    } else {
        MY_LOGW("cannot get the golden(%d)/first(%d) frame hal meta", (pInHalGolden == nullptr), (pInHalFirst == nullptr));
    }
}
#endif //SUPPORT_MFNR

// /*******************************************************************************
//  *
//  ********************************************************************************/
MVOID RootNode::reorder(
    Vector<RequestPtr>& rvRequests, Vector<RequestPtr>& rvOrderedRequests, size_t skipCount)
{
    FUNCTION_SCOPE;
    MY_LOGD("skip count=%zu", skipCount);
    if (rvRequests.size() != rvOrderedRequests.size()) {
        MY_LOGE("input(%zu) != result(%zu)", rvRequests.size(), rvOrderedRequests.size());
        return;
    }
    size_t frameCount = rvOrderedRequests.size();
    MY_LOGD("input size (%zu), result size (%zu)", rvRequests.size(), rvOrderedRequests.size());

    if (skipCount >= frameCount) {
        skipCount = frameCount - 1;
    }

    MY_LOGI("BSS: skip frame count: %zu", skipCount);

#ifdef SUPPORT_MFNR
    syncGoldenFrameMeta(rvOrderedRequests, skipCount, MID_MAN_IN_HAL);
    syncGoldenFrameMeta(rvOrderedRequests, skipCount, MID_SUB_IN_HAL);
    updateInOrderMetadata(rvOrderedRequests[0], rvRequests[0]);
    updateFaceData(rvOrderedRequests[0], rvRequests[0]);
#endif
    mGoldenInfo = std::make_pair<int, int>(rvOrderedRequests[0]->getPipelineFrameIndex(), rvOrderedRequests[0]->getFrameNo());

    // Switch input buffers with each other. To keep the first request's data path
    // Bind the life cycle of the request with bss input buffers to main request
    if (rvOrderedRequests[0] != rvRequests[0]) {
        MY_LOGD_IF(mLogLevel, "Set cross   %d   %d", rvRequests[0]->getFrameNo(), rvOrderedRequests[0]->getFrameNo());
        rvRequests[0]->setCrossRequest(rvOrderedRequests[0]);
        rvOrderedRequests[0]->setCrossRequest(rvRequests[0]);
    }

#ifdef SUPPORT_MFNR
   // write BSS-related EXIF
   //ExifWriter writer(PIPE_CLASS_TAG);
   //writer.makeExifFromCollectedData(rvRequests[0]);
#endif

    vector<int> bssForceOrder;
    if (mDebugLoadIn == 2) {
#ifdef SUPPORT_MFNR
        for (size_t i = 0; i < rvRequests.size(); i++) {
            bssForceOrder.push_back(i);
        }
#endif
    }

    const RequestPtr& rpPrimaryRequest = rvRequests.itemAt(0);

    MBOOL bDropToOne = (frameCount - skipCount) < 2 ? MTRUE : MFALSE;

    Vector<RequestPtr> vDispatchRequest;
    for (size_t i = 0; i < rvOrderedRequests.size(); i++) {
        const RequestPtr& rpOrderedRequest = rvOrderedRequests.itemAt(i);
        RequestPtr pDispatchRequest;
        if (i == 0)
            pDispatchRequest = rpPrimaryRequest;
        else if (rpOrderedRequest == rpPrimaryRequest)
            pDispatchRequest = rvOrderedRequests.editItemAt(0);
        else
            pDispatchRequest = rpOrderedRequest;

        auto pInMetaHalHnd = pDispatchRequest->getMetadata(MID_MAN_IN_HAL);
        IMetadata* pInMetaHal = pInMetaHalHnd->native();

        pDispatchRequest->addParameter(PID_DROPPED_COUNT, skipCount);
        if (i + skipCount >= frameCount)
            pDispatchRequest->addParameter(PID_DROPPED_FRAME, 1);

        if (mDebugLoadIn == 2 && bssForceOrder.size() > i) {
            pDispatchRequest->addParameter(PID_FRAME_INDEX_FORCE_BSS, bssForceOrder[i]);
            trySetMetadata<MINT32>(pInMetaHal, MTK_HAL_REQUEST_INDEX_BSS, bssForceOrder[i]);
        }

        if (bDropToOne) {
            auto updateDropTo1Meta = [&pDispatchRequest](MetadataID_T metaId) {
                auto pInMetaHalHnd = pDispatchRequest->getMetadata(metaId);
                 if (pInMetaHalHnd == NULL) {
                    MY_LOGD("no neet to update, cannot get the halMetaHandle, metaId:%d", metaId);
                    return;
                }
                IMetadata* pInMetaHal = pInMetaHalHnd->native();
                if (pInMetaHal == NULL) {
                    MY_LOGD("no neet to update, cannot get the halMetaPtr, metaId:%d", metaId);
                    return;
                }
                MINT64 feature = 0;
                // remove MFNR from metadata and parameter while dropping to one
                if (tryGetMetadata<MINT64>(pInMetaHal, MTK_FEATURE_CAPTURE, feature)) {
                    feature &= ~NSCam::NSPipelinePlugin::MTK_FEATURE_MFNR;
                    trySetMetadata<MINT64>(pInMetaHal, MTK_FEATURE_CAPTURE, feature);
                }
                trySetMetadata<MUINT8>(pInMetaHal, MTK_FEATURE_CAP_PIPE_DCE_CONTROL, MTK_FEATURE_CAP_PIPE_DCE_DEFAULT_APPLY);
                MY_LOGD("set DCE to %d due to drop to 1", MTK_FEATURE_CAP_PIPE_DCE_DEFAULT_APPLY);
            };
            updateDropTo1Meta(MID_MAN_IN_HAL);
            updateDropTo1Meta(MID_SUB_IN_HAL);
            pDispatchRequest->removeFeature(FID_MFNR);
        }

        if (pDispatchRequest->hasDelayInference()) {
            pDispatchRequest->startInference();
        }
        vDispatchRequest.push_back(pDispatchRequest);
    }

    // early callback golden frame index
    auto& pBestRequest = rvOrderedRequests[0];
    auto pOutMetaApp = rvRequests[0]->getMetadata(MID_MAN_OUT_APP);
    IMetadata* pOAppMeta =
        (pOutMetaApp != nullptr) ? pOutMetaApp->native() : nullptr;
    IMetadata::setEntry<MINT32>(pOAppMeta, MTK_MFNR_FEATURE_BSS_GOLDEN_INDEX,
                                pBestRequest->getPipelineFrameIndex());
    rvRequests[0]->mpCallback->onMetaResultAvailable(rvRequests[0], pOAppMeta);

    for (size_t i = 0; i < vDispatchRequest.size(); i++) {
        RequestPtr& pRequest = vDispatchRequest.editItemAt(i);

        MY_LOGD_IF(mLogLevel, "dispatch BSS-reorder request: order:%zu R/F/S:%d/%d/%d isCross:%d",
                    i, pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex(), pRequest->isCross());
        onRequestFinish(vDispatchRequest.editItemAt(i));
    }

    for (const auto &p : mPhysicWaitingMap) {
        const BssWaitingList& list = p.second;
        if (list.mWaitForBss != nullptr)
            sem_post(list.mWaitForBss.get());
    }
}

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
