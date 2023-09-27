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

#include "MultiFrameNode.h"

#define PIPE_CLASS_TAG "MultiFrameNode"
#define PIPE_TRACE TRACE_MULTIFRAME_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <sstream>
#include "../CaptureFeaturePlugin.h"
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAPTURE_MULTIFRAME);

using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::NSIoPipe::NSSImager;
using namespace NS3Av3;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

/******************************************************************************
*
******************************************************************************/
class MultiFrameInterface : public MultiFramePlugin::IInterface
{
public:
    virtual MERROR offer(MultiFramePlugin::Selection& sel)
    {
        sel.mRequestCount = 0;
        sel.mFrontDummy = 0;
        sel.mPostDummy = 0;

        sel.mIBufferFull
            .addSupportFormat(eImgFmt_NV12)
            .addSupportFormat(eImgFmt_YV12)
            .addSupportFormat(eImgFmt_YUY2)
            .addSupportFormat(eImgFmt_NV21)
            .addSupportFormat(eImgFmt_I420)
            .addSupportFormat(eImgFmt_I422)
            .addSupportFormat(eImgFmt_BAYER10)
            .addSupportFormat(eImgFmt_BAYER10_UNPAK)
            .addSupportFormat(eImgFmt_BAYER12)
            .addSupportFormat(eImgFmt_BAYER12_UNPAK)
            .addSupportFormat(eImgFmt_RAW16)
            .addSupportFormat(eImgFmt_MTK_YUV_P010)
            .addSupportFormat(eImgFmt_MTK_YUV_P012)
            .addSupportSize(eImgSize_Full)
            .addSupportSize(eImgSize_Specified);

        sel.mIBufferSpecified
            .addSupportFormat(eImgFmt_NV12)
            .addSupportFormat(eImgFmt_YV12)
            .addSupportFormat(eImgFmt_YUY2)
            .addSupportFormat(eImgFmt_NV21)
            .addSupportFormat(eImgFmt_I420)
            .addSupportFormat(eImgFmt_I422)
            .addSupportFormat(eImgFmt_MTK_YUV_P010)
            .addSupportFormat(eImgFmt_MTK_YUV_P012)
            .addSupportFormat(eImgFmt_Y8)
            .addSupportSize(eImgSize_Specified)
            .addSupportSize(eImgSize_Quarter);

        if(PlatformCapability::isSupport(PlatformCapability::HWCap_P1RRZO))
        {
            MY_LOGD("MultiFrameNode: support P1RRZO");
            sel.mIBufferResized
            .addSupportFormat(eImgFmt_FG_BAYER10)
            .addSupportSize(eImgSize_Resized);
        }
        else
        {
            MY_LOGD("MultiFrameNode: not support P1RRZO");
            sel.mIBufferResized
            .addSupportFormat(eImgFmt_NV12)
            .addSupportFormat(eImgFmt_YV12)
            .addSupportFormat(eImgFmt_YUY2)
            .addSupportFormat(eImgFmt_NV21)
            .addSupportFormat(eImgFmt_I420)
            .addSupportFormat(eImgFmt_I422)
            .addSupportFormat(eImgFmt_MTK_YUV_P010)
            .addSupportFormat(eImgFmt_MTK_YUV_P012)
            .addSupportFormat(eImgFmt_Y8)
            .addSupportSize(eImgSize_Specified)
            .addSupportSize(eImgSize_Quarter);
        }


        if (sel.mRequestIndex == 0) {
            sel.mOBufferFull
                .addSupportFormat(eImgFmt_NV12)
                .addSupportFormat(eImgFmt_YV12)
                .addSupportFormat(eImgFmt_YUY2)
                .addSupportFormat(eImgFmt_NV21)
                .addSupportFormat(eImgFmt_I420)
                .addSupportFormat(eImgFmt_I422)
                .addSupportFormat(eImgFmt_Y8)
                .addSupportFormat(eImgFmt_BAYER12)
                .addSupportFormat(eImgFmt_BAYER10)
                .addSupportFormat(eImgFmt_BAYER16_UNPAK)
                .addSupportFormat(eImgFmt_BAYER12_UNPAK)
                .addSupportFormat(eImgFmt_BAYER10_UNPAK)
                .addSupportFormat(eImgFmt_RAW16)
                .addSupportFormat(eImgFmt_MTK_YUV_P010)
                .addSupportSize(eImgSize_Full);

            sel.mOBufferThumbnail
                .addSupportFormat(eImgFmt_NV12)
                .addSupportFormat(eImgFmt_YV12)
                .addSupportFormat(eImgFmt_YUY2)
                .addSupportFormat(eImgFmt_NV21)
                .addSupportSize(eImgSize_Arbitrary);
        }
        return OK;
    };

    virtual ~MultiFrameInterface() {};
};

REGISTER_PLUGIN_INTERFACE(MultiFrame, MultiFrameInterface);

/******************************************************************************
*
******************************************************************************/
class MultiFrameCallback : public MultiFramePlugin::RequestCallback
{
public:
    MultiFrameCallback(MultiFrameNode* pNode)
        : mpNode(pNode)
    {
    }

    virtual void onAborted(MultiFramePlugin::Request::Ptr pPlgRequest)
    {
        MY_LOGD("onAborted request: %p", pPlgRequest.get());
        onCompleted(pPlgRequest, UNKNOWN_ERROR);
    }

    virtual void onCompleted(MultiFramePlugin::Request::Ptr pPlgRequest, MERROR result)
    {
        RequestPtr pRequest = mpNode->findRequest(pPlgRequest);

        if (pRequest == NULL) {
            MY_LOGE("unknown request happened: %p, result %d", pPlgRequest.get(), result);
            return;
        }

        *pPlgRequest = MultiFramePlugin::Request();
        MY_LOGD("onCompleted request: %p, result %d", pPlgRequest.get(), result);
        mpNode->onRequestFinish(pRequest);
    }

    virtual void onNextCapture(MultiFramePlugin::Request::Ptr pPlgRequest)
    {
        if (pPlgRequest == NULL) {
            MY_LOGE("unknown request happened: %p", pPlgRequest.get());
            return;
        }

        mpNode->onNextCapture(pPlgRequest);
    }

    virtual bool onConfigMainFrame(MultiFramePlugin::Request::Ptr pPlgRequest)
    {
        if (pPlgRequest == NULL) {
            MY_LOGE("unknown request happened: %p", pPlgRequest.get());
            return false;
        }
        return mpNode->onConfigMainFrame(pPlgRequest);
    }

    virtual ~MultiFrameCallback() { };
private:
    MultiFrameNode* mpNode;
};


/******************************************************************************
*
******************************************************************************/
MultiFrameNode::MultiFrameNode(NodeID_T nid, const char* name, MINT32 policy, MINT32 priority)
    : CamNodeULogHandler(Utils::ULog::MOD_CAPTURE_MULTIFRAME)
    , CaptureFeatureNode(nid, name, 0, policy, priority)
    , mAbortingRequestNo(-1)
{
    TRACE_FUNC_ENTER();
    MY_LOGI("(%p) ctor", this);
    this->addWaitQueue(&mRequests);
    mRawTransDump = property_get_int32("vendor.debug.camera.p2.dump", 0);
    TRACE_FUNC_EXIT();
}

MultiFrameNode::~MultiFrameNode()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("(%p) dtor", this);
    TRACE_FUNC_EXIT();
}

MVOID MultiFrameNode::setBufferPool(const android::sp<CaptureBufferPool> &pool)
{
    TRACE_FUNC_ENTER();
    mpBufferPool = pool;
    TRACE_FUNC_EXIT();
}

MBOOL MultiFrameNode::onData(DataID id, const RequestPtr& pRequest)
{
    TRACE_FUNC_ENTER();
    MY_LOGD_IF(mLogLevel, "R/F/S:%d/%d/%d I/C:%d/%d %s arrived",
                pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex(),
                pRequest->getActiveFrameIndex(), pRequest->getActiveFrameCount(),
                PathID2Name(id));

    NodeID_T srcNodeId;
    NodeID_T dstNodeId;
    MBOOL ret = GetPath(id, srcNodeId, dstNodeId);
    if (!ret) {
        MY_LOGD("Can not find the path: %d", id);
        return ret;
    }

    if (dstNodeId == NID_MULTIRAW)
        pRequest->addParameter(PID_MULTIFRAME_TYPE, 0);
    else if (dstNodeId == NID_MULTIYUV)
        pRequest->addParameter(PID_MULTIFRAME_TYPE, 1);

    if (pRequest->isReadyFor(dstNodeId))
        mRequests.enque(pRequest);

    TRACE_FUNC_EXIT();
    return ret;
}

MVOID MultiFrameNode::abortRequest(RequestPtr& pRequest, ProviderPtr pPlgProvider)
{
    // pick a pProvider
    ProviderPtr pProvider = pPlgProvider;
    if (pProvider == NULL) {
        for (size_t i = 0 ; i < mProviderMap.size() ; i++) {
            FeatureID_T featId = mProviderMap.keyAt(i);
            if (pRequest->hasFeature(featId)) {
                pProvider =  mProviderMap.valueFor(featId);
                break;
            }
        }
    }

    // Clear the selection repository
    if (mAbortingRequestNo != pRequest->getRequestNo()) {
        do
        {
            // The rest requests will not queue to capture pipe, while abort() happend
            // Remove the rest selections.
            auto pFrontSel = mPlugin->frontSelection(pProvider);
            if (pFrontSel == NULL || pFrontSel->mRequestIndex == 0) {
                break;
            }
            auto pPopSel = mPlugin->popSelection(pProvider);
            MY_LOGW("Erase Selection C/I: %u/%d",
                pPopSel->mRequestCount, pPopSel->mRequestIndex);
        } while(true);
    }
    mAbortingRequestNo = pRequest->getRequestNo();
}

MBOOL MultiFrameNode::onAbort(RequestPtr &pRequest)
{
    Mutex::Autolock _l(mOperLock);
    MBOOL found = MFALSE;

    if (!pRequest->isCancelled())
        return MTRUE;

    RequestPair p;
    {
        Mutex::Autolock _l(mPairLock);
        auto it = mRequestPairs.begin();
        for (; it != mRequestPairs.end(); it++) {
            RequestPair& rPair = *it;
            if (rPair.mpCapRequest == pRequest) {
                p = rPair;
                found = MTRUE;
                break;
            }
        }
    }

    auto pProvider = p.mpPlgProvider;

    abortRequest(pRequest, (found ? pProvider : NULL));

    if (found) {
        MY_LOGI("+, R/F/S Num: %d/%d/%d",
            pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
        std::vector<PluginRequestPtr> vpPlgRequests;
        vpPlgRequests.push_back(p.mpPlgRequest);

        pProvider->abort(vpPlgRequests);

         MY_LOGI("-, R/F/S Num: %d/%d/%d",
            pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex());
    }

    return MTRUE;
}

MBOOL MultiFrameNode::onInit()
{
    TRACE_FUNC_ENTER();
    CaptureFeatureNode::onInit();
    MY_LOGI("(%p) uniqueKey:%d", this, mUsageHint.mPluginUniqueKey);

    mPlugin = MultiFramePlugin::getInstance(mUsageHint.mPluginUniqueKey);

    FeatureID_T featId;
    auto& pProviders = mPlugin->getProviders();

    for (auto& pProvider : pProviders) {
        const MultiFramePlugin::Property& rProperty = pProvider->property();
        featId = NULL_FEATURE;

        if (rProperty.mFeatures & MTK_FEATURE_MFNR)
            featId = FID_MFNR;
        else if (rProperty.mFeatures & MTK_FEATURE_AINR)
            featId = FID_AINR;
        else if (rProperty.mFeatures & MTK_FEATURE_HDR)
            featId = FID_HDR;
        else if (rProperty.mFeatures & TP_FEATURE_MFNR)
            featId = FID_MFNR_3RD_PARTY;
        else if (rProperty.mFeatures & TP_FEATURE_HDR)
            featId = FID_HDR_3RD_PARTY;
        else if (rProperty.mFeatures & TP_FEATURE_HDR_DC)
            featId = FID_HDR2_3RD_PARTY;
        else if (rProperty.mFeatures & MTK_FEATURE_AINR_FOR_HDR)
            featId = FID_AINR_YHDR;
        else if (rProperty.mFeatures & MTK_FEATURE_AIHDR)
            featId = FID_AIHDR;
        else if (rProperty.mFeatures & TP_FEATURE_ZSDHDR)
            featId = FID_ZSDHDR_3RD_PARTY;

        if (featId != NULL_FEATURE) {
            MY_LOGD_IF(mLogLevel, "Find plugin: %s", FeatID2Name(featId));
            mProviderMap.add(featId, pProvider);

            if (rProperty.mInitPhase == ePhase_OnPipeInit && mInitMap.count(featId) <= 0)
            {
                std::function<void()> func = [featId, pProvider]() {
                    MY_LOGD("Init Plugin: %s on init phase +", FeatID2Name(featId));
                    pProvider->init();
                    MY_LOGD("Init Plugin: %s on init phase -", FeatID2Name(featId));
                };

                mInitMap[featId] = mpTaskQueue->addFutureTask(func);
            }
        }
    }

    mpCallback = make_shared<MultiFrameCallback>(this);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MultiFrameNode::onUninit()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("(%p) uniqueKey:%d", this, mUsageHint.mPluginUniqueKey);

    for (size_t i = 0 ; i < mProviderMap.size() ; i++) {
        ProviderPtr pProvider = mProviderMap.valueAt(i);
        FeatureID_T featId = mProviderMap.keyAt(i);

        if (mInitMap.count(featId) > 0) {
            if (!mInitFeatures.hasBit(featId)) {
                MY_LOGD("Wait for initilizing + Feature: %s", FeatID2Name(featId));
                mInitMap[featId].wait();
                MY_LOGD("Wait for initilizing - Feature: %s", FeatID2Name(featId));
                mInitFeatures.markBit(featId);
            }
        }
    }

    mProviderMap.clear();
    mInitMap.clear();
    mInitFeatures.clear();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MultiFrameNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL MultiFrameNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL MultiFrameNode::onThreadLoop()
{
    TRACE_FUNC_ENTER();
    if (!waitAllQueue())
    {
        TRACE_FUNC("Wait all queue exit");
        return MFALSE;
    }

    RequestPtr pRequest;
    if (!mRequests.deque(pRequest)) {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    } else if (pRequest == NULL) {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    onRequestProcess(pRequest);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID MultiFrameNode::onFlush()
{
    TRACE_FUNC_ENTER();
    CaptureFeatureNode::onFlush();
    TRACE_FUNC_EXIT();
}

MBOOL MultiFrameNode::onRequestProcess(RequestPtr& pRequest)
{
    Mutex::Autolock _l(mOperLock);
    incExtThreadDependency();

    MINT32 frameType = pRequest->getParameter(PID_MULTIFRAME_TYPE);
    NodeID_T nodeId = frameType == 0 ? NID_MULTIRAW : NID_MULTIYUV;

    MINT32 requestNo = pRequest->getRequestNo();
    MINT32 frameNo = pRequest->getFrameNo();
    MINT32 sensorId = pRequest->getMainSensorIndex();
    if (pRequest->isCancelled() || mAbortingRequestNo == requestNo) {
        MY_LOGD("Cancel, R/F/S/A Num: %d/%d/%d/%d", requestNo, frameNo, sensorId, mAbortingRequestNo);

        // onRequestProcess may execute before onAbort
        if (mAbortingRequestNo != requestNo && pRequest->hasParameter(PID_ABORTED)) {
            abortRequest(pRequest, NULL);
        }

        pRequest->decNodeReference(nodeId);
        onRequestFinish(pRequest);
        return MTRUE;
    }

    FeatureID_T featureId = NULL_FEATURE;
    CAM_TRACE_FMT_BEGIN("mf:process|r%df%ds%d", requestNo, frameNo, sensorId);

    sp<CaptureFeatureNodeRequest> pNodeReq =
        pRequest->getNodeRequest(nodeId);

    if (pNodeReq == NULL) {
        MY_LOGE("should not be here if no node request, type:%d", frameType);
        pRequest->addParameter(PID_FAILURE, 1);
        onRequestFinish(pRequest);
        return MFALSE;
    }

    // pick a pProvider
    ProviderPtr pProvider = NULL;
    for (size_t i = 0 ; i < mProviderMap.size() ; i++) {
        FeatureID_T featId = mProviderMap.keyAt(i);
        if (pRequest->hasFeature(featId)) {
            pProvider =  mProviderMap.valueFor(featId);
            featureId = featId;
            break;
        }
    }

    TypeID_T uFullType = frameType == 0 ? TID_MAN_FULL_RAW : TID_MAN_FULL_YUV;
    MBOOL hasInRszRaw = pNodeReq->mapBufferID(TID_MAN_RSZ_RAW, INPUT) != NULL_BUFFER;
    MY_LOGI("(%p)+, R/F/S Num: %d/%d/%d fulltype=%d hasInRszRaw:%d", this, requestNo, frameNo, sensorId, uFullType, hasInRszRaw);

    auto pPlgRequest = mPlugin->createRequest();

    pPlgRequest->mSensorId = sensorId;

    pPlgRequest->mIBufferFull       = PluginHelper::CreateBuffer(pNodeReq, uFullType, INPUT);
    // ISP 3.0: For R2Y MFNR
    if (nodeId == NID_MULTIYUV && pPlgRequest->mIBufferFull == NULL)
        pPlgRequest->mIBufferFull   = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_FULL_RAW, INPUT);

    pPlgRequest->mIBufferSpecified  = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_SPEC_YUV, INPUT);
    if(nodeId == NID_MULTIRAW)
    {
        if(hasInRszRaw)
            pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_RSZ_RAW, INPUT);
        else
            pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_RSZ_YUV, INPUT);
    }
    else if(nodeId == NID_MULTIYUV)
    {
        // TID_MAN_MSS_YUV/TID_MAN_RSZ_YUV/TID_MAN_IMGO_RSZ_YUV : one request can only exist one type in one time
        if(pNodeReq->mapBufferID(TID_MAN_MSS_YUV, INPUT) != NULL_BUFFER)
            pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_MSS_YUV, INPUT);
        else if(pNodeReq->mapBufferID(TID_MAN_IMGO_RSZ_YUV, INPUT) != NULL_BUFFER)
            pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_IMGO_RSZ_YUV, INPUT);
        else
            pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_RSZ_YUV, INPUT);
    }
    pPlgRequest->mIBufferLCS        = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_LCS, INPUT);
    pPlgRequest->mIBufferLCESHO     = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_LCESHO, INPUT);
    pPlgRequest->mIBufferDCES       = PluginHelper::CreateBuffer(pNodeReq, TID_MAN_DCES, INPUT);
    pPlgRequest->mOBufferFull       = PluginHelper::CreateBuffer(pNodeReq, uFullType, OUTPUT);

    pPlgRequest->mIMetadataDynamic  = PluginHelper::CreateMetadata(pNodeReq, MID_MAN_IN_P1_DYNAMIC);
    pPlgRequest->mIMetadataApp      = PluginHelper::CreateMetadata(pNodeReq, MID_MAN_IN_APP);
    pPlgRequest->mIMetadataHal      = PluginHelper::CreateMetadata(pNodeReq, MID_MAN_IN_HAL);
    pPlgRequest->mOMetadataApp      = PluginHelper::CreateMetadata(pNodeReq, MID_MAN_OUT_APP);
    pPlgRequest->mOMetadataHal      = PluginHelper::CreateMetadata(pNodeReq, MID_MAN_OUT_HAL);

    // pure raw to process raw
    // plugin should inform MultiFramenode by using sel.mProcessRaw to tell plugin need process raw
    // using sel.mIBufferFull.addAcceptedFormat, add the prefered format in the first place
    PluginSmartBufferHandlePtr iBufferFullHandle = NULL;
    const BufferID_T inputBufId = pNodeReq->mapBufferID(TID_MAN_FULL_RAW, INPUT);
    IImageBuffer* pInputImgBuf = pNodeReq->acquireBuffer(inputBufId);

    MINT inBufFmt = eImgFmt_UNKNOWN;
    MSize inImgSize = MSize(0, 0);
    MINT32 inColorArrangement = -1;
    if(pInputImgBuf != nullptr) {
        inBufFmt = pInputImgBuf->getImgFormat();
        inImgSize = pInputImgBuf->getImgSize();
        inColorArrangement = pInputImgBuf->getColorArrangement();
    }

    MBOOL bNeedRawFmtTrans = MFALSE;

    if(mPluginRawBufType.indexOfKey(featureId) >= 0)
    {
        const MINT pluginPreRawType = mPluginRawBufType.editValueFor(featureId).mPluginPreferRawType;
        bNeedRawFmtTrans = ((pluginPreRawType != eImgFmt_UNKNOWN) && (pluginPreRawType != inBufFmt))
                            || mPluginRawBufType.editValueFor(featureId).mPluginIBufProcessRaw;
    }

    if (uFullType == TID_MAN_FULL_RAW &&
        bNeedRawFmtTrans && P2_RAW_FORMAT_UTILITY)
    {
        const MINT Fmt = mPluginRawBufType.editValueFor(featureId).mPluginPreferRawType;
        MBOOL outProcessRaw = mPluginRawBufType.editValueFor(featureId).mPluginIBufProcessRaw;
        android::sp<IIBuffer> pWorkingBuffer = mpBufferPool->getImageBuffer(inImgSize, Fmt, HwStrideAlignment::queryFormatAlignment(Fmt));
        IImageBuffer* pOutputImgBuf = pWorkingBuffer->getImageBufferPtr();
        pOutputImgBuf->setColorArrangement(inColorArrangement);
        std::unique_ptr<IImageTransform, std::function<void(IImageTransform*)>> transform(
                IImageTransform::createInstance(PIPE_CLASS_TAG, sensorId, IImageTransform::OPMode_DisableWPE),
                [](IImageTransform *p)
                {
                    if (p)
                        p->destroyInstance();
                }
            );

        if (transform.get() == NULL) {
            MY_LOGE("IImageTransform is NULL, cannot generate output");
            return MFALSE;
        }

        IMetadata* pAppMeta = pRequest->getMetadata(MID_MAN_IN_APP)->native();
        IMetadata* pHalMeta = pRequest->getMetadata(MID_MAN_IN_HAL)->native();
        if(transform->pureRawFmtTrans(pInputImgBuf, pOutputImgBuf, pAppMeta, pHalMeta, outProcessRaw, mRawTransDump))
        {
            iBufferFullHandle = PluginSmartBufferHandle::createInstance(PIPE_CLASS_TAG, pWorkingBuffer);
            pPlgRequest->mIBufferFull = std::move(iBufferFullHandle);
            pNodeReq->releaseBuffer(inputBufId);
        }
        else
        {
            MY_LOGW("force pure raw to process raw fail, remain original pure raw as input");
        }
    }

    // set PQ user id into meta for plugin use
    MINT64 userId = generatePQUserID(requestNo, pRequest->getMainSensorIndex(), pRequest->isPhysicalStream());
    trySetMetadata<MINT64>(pRequest->getMetadata(MID_MAN_IN_HAL)->native(), MTK_FEATURE_CAP_PQ_USERID, userId);
    // update frame info
    pPlgRequest->mRequestIndex = pRequest->getPipelineFrameIndex();
    pPlgRequest->mRequestCount = pRequest->getPipelineFrameCount();
    pPlgRequest->mRequestBSSIndex = pRequest->getBSSFrameIndex();
    pPlgRequest->mRequestBSSCount = pRequest->getBSSFrameCount();



    MBOOL ret = MFALSE;
    if (pProvider != NULL)
    {
        if (!mInitFeatures.hasBit(featureId) && (mInitMap.count(featureId) > 0)) {
            MY_LOGD("Wait for initilizing + Feature: %s", FeatID2Name(featureId));
            mInitMap[featureId].wait();
            MY_LOGD("Wait for initilizing - Feature: %s", FeatID2Name(featureId));
            mInitFeatures.markBit(featureId);
        }

        {
            Mutex::Autolock _l(mPairLock);
            auto& rPair = mRequestPairs.editItemAt(mRequestPairs.add());
            rPair.mpCapRequest = pRequest;
            rPair.mpPlgRequest = pPlgRequest;
            rPair.mpPlgProvider = pProvider;
        }
        pProvider->process(pPlgRequest, mpCallback);
        ret = MTRUE;
    }
    else
    {
        MY_LOGE("do not execute a plugin");
        pRequest->addParameter(PID_FAILURE, 1);
        onRequestFinish(pRequest);
    }

    MY_LOGI("(%p)-, R/F/S Num: %d/%d/%d", this, requestNo, frameNo, sensorId);
    CAM_TRACE_FMT_END();
    return MTRUE;
}

RequestPtr MultiFrameNode::findRequest(PluginRequestPtr& pPlgRequest)
{
    Mutex::Autolock _l(mPairLock);
    for (const auto& rPair : mRequestPairs) {
        if (pPlgRequest == rPair.mpPlgRequest) {
            return rPair.mpCapRequest;
        }
    }

    return NULL;
}

RequestPtr MultiFrameNode::findRequest(MINT32 iReqNo, MINT32 iFrameIdx)
{
    Mutex::Autolock _l(mPairLock);
    for (const auto& rPair : mRequestPairs) {
        if(rPair.mpCapRequest->getRequestNo() == iReqNo &&
            rPair.mpCapRequest->getPipelineFrameIndex() == iFrameIdx )
            return rPair.mpCapRequest;
    }
    return NULL;
}

MVOID MultiFrameNode::onRequestFinish(RequestPtr& pRequest)
{
    MINT32 requestNo = pRequest->getRequestNo();
    MINT32 frameNo = pRequest->getFrameNo();
    MINT32 sensorId = pRequest->getMainSensorIndex();
    CAM_TRACE_FMT_BEGIN("mf:finish|r%df%ds%d", requestNo, frameNo, sensorId);
    MY_LOGI("(%p)R/F/S Num: %d/%d/%d", this, requestNo, frameNo, sensorId);

    {
        Mutex::Autolock _l(mPairLock);
        auto it = mRequestPairs.begin();
        for (; it != mRequestPairs.end(); it++) {
            if ((*it).mpCapRequest == pRequest) {
                mRequestPairs.erase(it);
                break;
            }
        }
    }

    if (pRequest->getParameter(PID_ENABLE_NEXT_CAPTURE) > 0)
    {
        if (pRequest->getParameter(PID_THUMBNAIL_TIMING) == NSPipelinePlugin::eTiming_MultiFrame)
        {
            MUINT32 frameCount = pRequest->getActiveFrameCount();
            MUINT32 frameIndex = pRequest->getActiveFrameIndex();

            if (frameCount == frameIndex + 1)
            {
                if (pRequest->mpCallback != NULL) {
                    MY_LOGD("Notify next capture at (%dI%d)", frameIndex, frameCount);
                    pRequest->mpCallback->onContinue(pRequest);
                } else {
                    MY_LOGW("have no request callback instance!");
                }
            }
        }
    }
    MINT32 frameType = pRequest->getParameter(PID_MULTIFRAME_TYPE);
    NodeID_T nodeId = frameType == 0 ? NID_MULTIRAW : NID_MULTIYUV;
    // decrease reference
    pRequest->decNodeReference(nodeId);
    dispatch(pRequest, nodeId);

    decExtThreadDependency();
    CAM_TRACE_FMT_END();
    return;
}

MVOID MultiFrameNode::onNextCapture(PluginRequestPtr& pPlgRequest)
{
    auto findRequestPair = [this](const PluginRequestPtr& pPlgRequest, RequestPair& rPair) -> MBOOL
    {
        if (&rPair == nullptr) {
            MY_LOGE("RequestPair is nullptr!");
            return MFALSE;
        }
        Mutex::Autolock _l(mPairLock);
        for (const auto& item : mRequestPairs) {
            if(item.mpPlgRequest == NULL) {
                continue;
            }
            if (pPlgRequest == item.mpPlgRequest) {
                rPair = item;
                return MTRUE;
            }
        }
        return MFALSE;
    };

    RequestPair reqPair;
    if (!findRequestPair(pPlgRequest, reqPair)) {
        MY_LOGE("cannot find the corresponding requestPair: %p", pPlgRequest.get());
    }

    const RequestPtr& pRequest = reqPair.mpCapRequest;
    const MultiFramePlugin::Property& rProperty = reqPair.mpPlgProvider->property();
    const MBOOL isEnableNextCapture = (pRequest->getParameter(PID_ENABLE_NEXT_CAPTURE) > 0);
    const MBOOL hasTiming = pRequest->hasParameter(PID_THUMBNAIL_TIMING);
    if (!isEnableNextCapture || !hasTiming)
    {
        MY_LOGD("failed to processor next capure, isEnableNextCapture:%d, hasTiming:%d", isEnableNextCapture, hasTiming);
        return;
    }

    const MBOOL timing = pRequest->getParameter(PID_THUMBNAIL_TIMING);
    if (rProperty.mThumbnailTiming == timing) {
        if (pRequest->mpCallback != NULL) {
            MY_LOGD("Notify next capture at (%dI%d), R/F/S Num: %d/%d/%d, feature:%#012" PRIx64,
                pRequest->getActiveFrameIndex(), pRequest->getActiveFrameCount(),
                pRequest->getRequestNo(), pRequest->getFrameNo(),
                pRequest->getMainSensorIndex(), rProperty.mFeatures);
            pRequest->mpCallback->onContinue(pRequest);
        } else {
            MY_LOGW("have no request callback instance!");
        }
    }
}

bool MultiFrameNode::onConfigMainFrame(PluginRequestPtr& pPlgRequest)
{

    RequestPtr pToBeMainRequest = this->findRequest(pPlgRequest);
    if(pToBeMainRequest == NULL)
    {
        MY_LOGE("Cannot find the request(I/C:%d/%d, BI/BC:%d/%d) in on-going request map, maybe already dispatch.",
                pPlgRequest->mRequestIndex, pPlgRequest->mRequestCount, pPlgRequest->mRequestBSSIndex, pPlgRequest->mRequestBSSIndex);
        return false;
    }

    MINT32 frameType = pToBeMainRequest->getParameter(PID_MULTIFRAME_TYPE);
    NodeID_T nodeId = frameType == 0 ? NID_MULTIRAW : NID_MULTIYUV;
    if(nodeId == NID_MULTIYUV)
    {
        MY_LOGE("Cannot support Multi-yuv node to configure main frame");
        return false;
    }

    // find the FrameNo=0 request
    RequestPtr pCurMainFrame = this->findRequest(pToBeMainRequest->getRequestNo(), 0);

    if(pCurMainFrame == NULL)
    {
        MY_LOGE("Cannot find the main request, R/F:%d/0, maybe already dispatch.", pToBeMainRequest->getRequestNo());
        return false;
    }
    else if(pCurMainFrame->getFrameNo() == pToBeMainRequest->getFrameNo())
    {
        MY_LOGW("Current MainFrame is already the input one, no need to do anything.");
        return true;
    }
    else if(pCurMainFrame->isUnderBSS() && !pCurMainFrame->isBSSFirstFrame())
    {
        MY_LOGE("Request no:%d already had bss main-frame from BSSCore, cannot configure another main-frame", pCurMainFrame->getRequestNo());
        return false;
    }
    MY_LOGD("R:%d config main frame to F:%d from Frame:%d",
            pToBeMainRequest->getRequestNo(), pToBeMainRequest->getFrameNo(), pCurMainFrame->getFrameNo());
    // swap the bss order
    auto cur_bss = pCurMainFrame->getParameter(PID_BSS_ORDER);
    auto to_be_bss = pToBeMainRequest->getParameter(PID_BSS_ORDER);
    pCurMainFrame->addParameter(PID_BSS_ORDER, to_be_bss);
    pToBeMainRequest->addParameter(PID_BSS_ORDER, cur_bss);
    // set cross request
    pCurMainFrame->setCrossRequest(pToBeMainRequest);
    pToBeMainRequest->setCrossRequest(pCurMainFrame);
    return true;
}

MVOID MultiFrameNode::
updateSelInfo(Selection& sel, CaptureFeatureInferenceData& rInfer)
{
    /* Set capture ISO into selection metadata */
    IMetadata* selHalMeta = sel.mIMetadataHal.getControl().get();
    IMetadata* inferDynamicMeta = rInfer.mpIMetadataDynamic.get();

    if (selHalMeta != nullptr && inferDynamicMeta != nullptr) {
        MINT32 iso = 0;
        if (IMetadata::getEntry<MINT32>(inferDynamicMeta, MTK_SENSOR_SENSITIVITY, iso)) {
            IMetadata::setEntry<MINT32>(selHalMeta, MTK_SENSOR_SENSITIVITY, iso);
        }
    }
}

MERROR MultiFrameNode::evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInfer)
{
    auto& rSrcData = rInfer.getSharedSrcData();
    auto& rDstData = rInfer.getSharedDstData();
    auto& rFeatures = rInfer.getSharedFeatures();
    auto& rMetadatas = rInfer.getSharedMetadatas();

    MINT32 iRequestNum = rInfer.getRequestNum();
    MUINT8 uRequestCount = rInfer.getRequestCount();
    MINT8 uRequestIndex = rInfer.getRequestIndex();
    MUINT8 uDroppedCount = rInfer.getDroppedCount();
    MUINT8 uBSSBypassCount = rInfer.getBSSBypassCount();
    MINT32 sensorId = rInfer.mSensorIndex;
    MBOOL isPhysical = rInfer.mIsPhysical;

    MBOOL isEvaluated = MFALSE;
    MBOOL isValid;
    MERROR status = OK;

    // Foreach all loaded plugin
    for (size_t i = 0 ; i < mProviderMap.size() ; i++) {

        FeatureID_T featId = mProviderMap.keyAt(i);
        if (!rInfer.hasFeature(featId)) {
            if ((uDroppedCount != 0) && (uRequestCount < 2)) {
                ProviderPtr pProvider = mProviderMap.valueAt(i);
                auto token = Selection::createToken(mUsageHint.mPluginUniqueKey, iRequestNum, uRequestIndex, sensorId, isPhysical);
                auto pPopSel = mPlugin->popSelection(pProvider, token);
                MY_LOGW_IF((pPopSel != nullptr), "DropToOne Erase Selection C/I: %u/%d", pPopSel->mRequestCount, pPopSel->mRequestIndex);
            }
            continue;
        } else if (isEvaluated) {
            MY_LOGE("has duplicated feature: %s", FeatID2Name(featId));
            continue;
        }

        MY_LOGD_IF(mLogLevel, "I/C: %d/%d dropCount=%d BSSBypass=%d isUnderBSS=%d", uRequestIndex, uRequestCount, uDroppedCount, uBSSBypassCount, rInfer.isUnderBSS());

        isValid = MTRUE;

        ProviderPtr pProvider = mProviderMap.valueAt(i);

        if (pProvider->property().mInitPhase == ePhase_OnRequest &&
            mInitMap.count(featId) <= 0)
        {
            std::function<void()> func = [featId, pProvider]() {
                MY_LOGD("Init Plugin: %s on evaluate phase +", FeatID2Name(featId));
                pProvider->init();
                MY_LOGD("Init Plugin: %s on evaluate phase -", FeatID2Name(featId));
            };

            mInitMap[featId] = mpTaskQueue->addFutureTask(func);
        }



        // Gate the evaluate for RAW-Domain or YUV-Domain
        {
            auto token = Selection::createToken(mUsageHint.mPluginUniqueKey, iRequestNum, uRequestIndex, sensorId, isPhysical);
            auto pFrontSel = mPlugin->frontSelection(pProvider, token);
            if (pFrontSel == NULL) {
                MY_LOGW("Have no a selection, feature:%s, count:%u, index:%u, could be used in another domain. nodeID=%s",
                        FeatID2Name(featId), uRequestCount ,uRequestIndex, NodeID2Name(nodeId));
                return OK;
            }
                const Selection& rFrontSel = *pFrontSel;

            if (rFrontSel.mIBufferFull.getRequired() && rFrontSel.mIBufferFull.isValid()) {
                MINT fmt = rFrontSel.mIBufferFull.getFormats()[0];

                // To determine RAW or YUV domain algo should consider the specified buffer format
                if (rFrontSel.mIBufferSpecified.getRequired() && rFrontSel.mIBufferSpecified.isValid()) {
                    fmt = rFrontSel.mIBufferSpecified.getFormats()[0];
                }

                if ((fmt & eImgFmt_RAW_START) == eImgFmt_RAW_START) {
                    rInfer.mMultiframeType = MULTIFRAME_TYPE_MULTIRAW;
                    if (nodeId != NID_MULTIRAW) {
                        MY_LOGD("ignore YUV-domain request in evaluation, nodeId:%d", nodeId);
                        return OK;
                    }
                } else {
                    rInfer.mMultiframeType = MULTIFRAME_TYPE_MULTIYUV;
                    if (nodeId != NID_MULTIYUV) {
                        MY_LOGD("ignore RAW-domain request in evaluation, nodeId:%d", nodeId);
                        return OK;
                    }
                }
            }
        }

        // Align the lastest set of selection while the first request
        if (uRequestIndex == 0)
        {
            auto token = Selection::createToken(mUsageHint.mPluginUniqueKey, iRequestNum, uRequestIndex, sensorId, isPhysical);
            auto pFrontSel = mPlugin->frontSelection(pProvider, token);
            if (pFrontSel == NULL)
                break;

            // check the starting selection index should start at 0, if not, there might exist last capture(abort) selection
            if(pFrontSel->mRequestIndex != 0)
            {
                size_t num = mPlugin->numOfSelection(pProvider);
                if(num < uRequestCount)
                {
                    MY_LOGE("Error! Should not happen! I/C: %d/%d dropCount=%d BSSBypass=%d isUnderBSS=%d",
                            uRequestIndex, uRequestCount, uDroppedCount, uBSSBypassCount, rInfer.isUnderBSS());
                    return BAD_VALUE;
                }
                // pop selection
                do
                {
                    auto pPopSel = mPlugin->popSelection(pProvider);
                    MY_LOGW("align selection queue: erased selection C/I: %u/%d num:%zu",
                    pPopSel->mRequestCount, pPopSel->mRequestIndex, num);
                    pFrontSel = mPlugin->frontSelection(pProvider);
                    num = mPlugin->numOfSelection(pProvider);
                    MY_LOGW("current front selection: C/I: %u/%d num:%zu",
                    pFrontSel->mRequestCount, pFrontSel->mRequestIndex, num);
                }while(pFrontSel->mRequestIndex != 0 && num > uRequestCount);

            }
        } else {
            auto token = Selection::createToken(mUsageHint.mPluginUniqueKey, iRequestNum, uRequestIndex, sensorId, isPhysical);
            auto pFrontSel = mPlugin->frontSelection(pProvider, token);
            if (pFrontSel == NULL) {
                MY_LOGE("have no selection to pop. C/I: %d/%d", uRequestCount, uRequestIndex);
                return OK;
            } else if (!rInfer.isBypassBSS() &&
                        !rInfer.isDropFrame() &&
                        pFrontSel->mRequestIndex != uRequestIndex)
            {
                MY_LOGE("Not Match Selection C/I: %u/%d; Request C/I :%u/%d",
                        pFrontSel->mRequestCount, pFrontSel->mRequestIndex,
                        uRequestCount ,uRequestIndex);
                return OK;
            }
        }

        auto token = Selection::createToken(mUsageHint.mPluginUniqueKey, iRequestNum, uRequestIndex, sensorId, isPhysical);
        auto pPopSel = mPlugin->popSelection(pProvider, token);
        if (pPopSel == NULL) {
            MY_LOGE("have no selection to pop. C/I: %d/%d", uRequestCount, uRequestIndex);
            return OK;
        }

        /* Do re-negotiate to update output buffer fmt, size...
         * This is designed for AINR 16bits mode because we need to decide
         * AINR output fmt depend on capture ISO.
         */
        updateSelInfo(*pPopSel, rInfer);
        pProvider->negotiateUpdate(*pPopSel);

        // Bypass BSS -> bypass Multiframe node
        if(rInfer.isBypassBSS() || rInfer.isDropFrame())
        {
            MY_LOGD("I/C: %d/%d , bypass MultiframeNode, BSS bypassed:%d dropFrame:%d",
                    uRequestIndex, uRequestCount, rInfer.isBypassBSS(), rInfer.isDropFrame());
            return OK;
        }
        else if(uRequestCount < 2)
        {
            MY_LOGD("I/C: %d/%d , Have not enough frames to blending. count: %d, dropped: %d",
                    uRequestIndex, uRequestCount, uRequestCount, uDroppedCount);
            return OK;
        }

        const Selection& rSel = *pPopSel;
        // should issue AEE if popped selection is not equal to request
        MY_LOGD("Pop Selection C/I: %u/%d; Request C/I :%u/%d",
                rSel.mRequestCount, rSel.mRequestIndex,
                uRequestCount ,uRequestIndex);

        // full size input
        if (rSel.mIBufferFull.getRequired()) {
            if (rSel.mIBufferFull.isValid()) {
                auto& src_0 = rSrcData.editItemAt(rSrcData.add());

                // Directly select the first format, using lazy strategy
                MINT fmt = rSel.mIBufferFull.getFormats()[0];
                // Check either RAW-Domain or YUV-Domain
                if ((fmt & eImgFmt_RAW_START) == eImgFmt_RAW_START) {
                    src_0.mTypeId = TID_MAN_FULL_RAW;
                    mPluginRawBufType.add(featId, {rSel.mProcessRaw, fmt});
                    src_0.setAllFmtSupport(MTRUE);

                    if (!rInfer.hasType(TID_MAN_FULL_RAW))
                        isValid = MFALSE;
                } else {
                    src_0.mTypeId = TID_MAN_FULL_YUV;
                    if (!rInfer.hasType(TID_MAN_FULL_YUV))
                        isValid = MFALSE;
                }

                if (isValid) {
                    src_0.mSizeId = rSel.mIBufferFull.getSizes()[0];
                    src_0.setFormat(fmt);
                    src_0.addSupportFormats(rSel.mIBufferFull.getFormats());

                    if (src_0.mSizeId == SID_SPECIFIED)
                        src_0.mSize = rSel.mIBufferFull.getSpecifiedSize();
                    else {
                        if ((fmt & eImgFmt_RAW_START) == eImgFmt_RAW_START)
                            src_0.mSize = rInfer.getSize(TID_MAN_FULL_RAW);
                        else
                            src_0.mSize = rInfer.getSize(TID_MAN_FULL_YUV);
                    }

                    // alignment for MFNR
                    rSel.mIBufferFull.getAlignment(src_0.mAlign);
                    if (src_0.mAlign.w | src_0.mAlign.h)
                        MY_LOGD("full buffer: align(%d,%d) size(%dx%d)",
                                src_0.mAlign.w, src_0.mAlign.h, src_0.mSize.w, src_0.mSize.h);
                }
            }
            else
            {
                MY_LOGE("mIBufferFull not valid");
                isValid = MFALSE;
            }
        }

        // specified size input
        if (isValid && rSel.mIBufferSpecified.getRequired()) {
            if (!rInfer.hasType(TID_MAN_SPEC_YUV))
                isValid = MFALSE;

            if (rSel.mIBufferSpecified.isValid()) {
                auto& src_1 = rSrcData.editItemAt(rSrcData.add());
                src_1.mTypeId = TID_MAN_SPEC_YUV;
                src_1.mSizeId = rSel.mIBufferSpecified.getSizes()[0];
                src_1.addSupportFormats(rSel.mIBufferSpecified.getFormats());
                if (src_1.mSizeId == SID_SPECIFIED)
                    src_1.mSize = rSel.mIBufferSpecified.getSpecifiedSize();
                else if (src_1.mSizeId == SID_QUARTER) {
                    MSize full = rInfer.getSize(TID_MAN_FULL_RAW);
                    src_1.mSize = MSize(full.w / 2, full.h / 2);
                }

                // alignment for MFNR
                rSel.mIBufferSpecified.getAlignment(src_1.mAlign);
                if (src_1.mAlign.w | src_1.mAlign.h)
                    MY_LOGD("specified buffer: align(%d,%d) size(%dx%d)",
                            src_1.mAlign.w, src_1.mAlign.h, src_1.mSize.w, src_1.mSize.h);

            }
            else
            {
                MY_LOGE("mIBufferSpecified not valid");
                isValid = MFALSE;
            }
        }

        // lcs
        if (isValid && rSel.mIBufferLCS.getRequired()) {
            if (!rInfer.hasType(TID_MAN_LCS))
            {
                MY_LOGE("No LCS type, not valid");
                isValid = MFALSE;
            }

            auto& src_2 = rSrcData.editItemAt(rSrcData.add());
            src_2.mTypeId = TID_MAN_LCS;
        }

        // lcesho
        if (isValid && rSel.mIBufferLCESHO.getRequired()) {
            if (!rInfer.hasType(TID_MAN_LCESHO))
            {
                MY_LOGE("No LCS type, not valid");
                isValid = MFALSE;
            }

            auto& src = rSrcData.editItemAt(rSrcData.add());
            src.mTypeId = TID_MAN_LCESHO;
        }

        // resized raw or yuv by platform
        if (isValid && rSel.mIBufferResized.getRequired()) {

            if (rSel.mIBufferResized.isValid()) {
                // find the buffer format
                MBOOL needRawFmt = MFALSE;
                auto fmtVec = rSel.mIBufferResized.getFormats();
                if(isHalRawFormat((EImageFormat)fmtVec[0]))
                    needRawFmt = MTRUE;
                else
                    needRawFmt = MFALSE;

                if (needRawFmt && !rInfer.hasType(TID_MAN_RSZ_RAW))
                {
                    MY_LOGE("mIBufferResized does not support TID_MAN_RSZ_RAW in this platform.");
                    isValid = MFALSE;
                }
                else if(!needRawFmt && (!rInfer.hasType(TID_MAN_RSZ_YUV) && !rInfer.hasType(TID_MAN_MSS_YUV) && !rInfer.hasType(TID_MAN_IMGO_RSZ_YUV)))
                {
                    MY_LOGE("mIBufferResized does not support TID_MAN_RSZ_YUV/MSS_YUV/IMGO_RSZ_YUV in this platform.");
                    isValid = MFALSE;
                }
                else
                {
                    auto& src_3 = rSrcData.editItemAt(rSrcData.add());
                    if(needRawFmt)
                        src_3.mTypeId = TID_MAN_RSZ_RAW;
                    else if(PlatformCapability::isSupport(PlatformCapability::HWCap_MSSMSF) &&
                        rSel.mIBufferResized.hasCapability(eBufCap_MSS))
                        src_3.mTypeId = TID_MAN_MSS_YUV;
                    else if(rSel.mIBufferResized.hasCapability(eBufCap_IMGO))
                        src_3.mTypeId = TID_MAN_IMGO_RSZ_YUV;
                    else
                        src_3.mTypeId = TID_MAN_RSZ_YUV;

                    src_3.mSizeId = rSel.mIBufferResized.getSizes()[0];
                    src_3.addSupportFormats(rSel.mIBufferResized.getFormats());
                    if (src_3.mSizeId == SID_SPECIFIED)
                        src_3.mSize = rSel.mIBufferResized.getSpecifiedSize();

                    rSel.mIBufferResized.getAlignment(src_3.mAlign);
                    if (src_3.mAlign.w | src_3.mAlign.h)
                        MY_LOGD("resize buffer: align(%d,%d) size(%dx%d)",
                                src_3.mAlign.w, src_3.mAlign.h, src_3.mSize.w, src_3.mSize.h);
                }
            }
            else
            {
                MY_LOGE("mIBufferResized not valid");
                isValid = MFALSE;
            }
        }
        // DCES
        if (isValid && rSel.mIBufferDCES.getRequired()) {
            if (!rInfer.hasType(TID_MAN_DCES))
            {
                MY_LOGE("no DCES type, not valid");
                isValid = MFALSE;
            }
            auto& src = rSrcData.editItemAt(rSrcData.add());
            src.mTypeId = TID_MAN_DCES;
        }
        // face data
        const MultiFramePlugin::Property& rProperty = pProvider->property();
        if (rProperty.mFaceData == eFD_Current) {
            auto& src_1 = rSrcData.editItemAt(rSrcData.add());
            src_1.mTypeId = TID_MAN_FD;
            src_1.mSizeId = NULL_SIZE;
            rInfer.markFaceData(eFD_Current);
        }
        else if (rProperty.mFaceData == eFD_Cache) {
            rInfer.markFaceData(eFD_Cache);
        }
        else if (rProperty.mFaceData == eFD_None) {
            rInfer.markFaceData(eFD_None);
        }
        else {
            MY_LOGW("Unknow faceDateType: %x", rInfer.mFaceDateType.value);
        }

        //
        if( rProperty.mBoost != 0 )
        {
            MY_LOGD("append boostType, value:%#09x", rProperty.mBoost);
            rInfer.appendBootType(rProperty.mBoost);
        }

        // full size output
        if (isValid && rSel.mOBufferFull.getRequired()) {
            if (rSel.mOBufferFull.isValid()) {
                auto& dst_0 = rDstData.editItemAt(rDstData.add());
                dst_0.mSizeId = rSel.mOBufferFull.getSizes()[0];
                dst_0.addSupportFormats(rSel.mOBufferFull.getFormats());
                if ((dst_0.getFormat()  & eImgFmt_RAW_START) == eImgFmt_RAW_START) {
                    dst_0.mSize = rInfer.getSize(TID_MAN_FULL_RAW);
                    dst_0.mTypeId = TID_MAN_FULL_RAW;
                } else {
                    dst_0.mSize = rInfer.getSize(TID_MAN_FULL_YUV);
                    dst_0.mTypeId = TID_MAN_FULL_YUV;
                }

                rSel.mOBufferFull.getAlignment(dst_0.mAlign);
                if (dst_0.mAlign.w | dst_0.mAlign.h) {
                    MY_LOGD("mOBufferFull: align(%d, %d) size(%d, %d)",
                        dst_0.mAlign.w, dst_0.mAlign.h, dst_0.mSize.w, dst_0.mSize.h);
                }
            }
            else
            {
                MY_LOGE("mOBufferFull not valid");
                isValid = MFALSE;
            }
        }

        if (isValid) {
            const MultiFramePlugin::Property& rProperty = pProvider->property();
            rInfer.markThumbnailTiming(rProperty.mThumbnailTiming);
            rInfer.setThumbnailDelay(rProperty.mThumbnailDelay);

            if (rSel.mIMetadataDynamic.getRequired())
                rMetadatas.push_back(MID_MAN_IN_P1_DYNAMIC);

            if (rSel.mIMetadataApp.getRequired())
                rMetadatas.push_back(MID_MAN_IN_APP);

            if (rSel.mIMetadataHal.getRequired())
                rMetadatas.push_back(MID_MAN_IN_HAL);

            if (uRequestIndex == 0 && rSel.mOMetadataApp.getRequired())
                rMetadatas.push_back(MID_MAN_OUT_APP);

            if (uRequestIndex == 0 && rSel.mOMetadataHal.getRequired())
                rMetadatas.push_back(MID_MAN_OUT_HAL);
        }

        if (isValid) {
            isEvaluated = MTRUE;
            rFeatures.push_back(featId);
            if(!rInfer.addNodeIO(nodeId, rSrcData, rDstData, rMetadatas, rFeatures, MTRUE))
            {
                status = BAD_VALUE;
            }
        } else
        {
            MY_LOGW("%s has an invalid evaluation: %s", NodeID2Name(nodeId), FeatID2Name(featId));
        }
    }

    return status;
}

std::string MultiFrameNode::getStatus(std::string& strDispatch)
{
    Mutex::Autolock _l(mPairLock);
    String8 str;
    if (mRequestPairs.size() > 0) {
        const MultiFramePlugin::Property& rProperty = mRequestPairs[0].mpPlgProvider->property();
        str = String8::format("(name:%s/algo:0x%016" PRIx64 ")", rProperty.mName, rProperty.mFeatures);
        if (strDispatch.size() == 0) {
            String8 strTmp = String8::format(" NOT Finish Provider: %s", rProperty.mName);
            strDispatch = strTmp.string() ? strTmp.string() : "";
        }
    }
    return (str.string() ? str.string() : "");
}

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
