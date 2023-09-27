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

#include "YUVNode.h"

#define PIPE_CLASS_TAG "YUVNode"
#define PIPE_TRACE TRACE_YUV_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <sstream>
#include <tuple>
#include "../CaptureFeaturePlugin.h"
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/drv/iopipe/SImager/ISImager.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAPTURE_YUV);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {


/*******************************************************************************
* Type Alias.
********************************************************************************/
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::NSIoPipe::NSSImager;
//
using YuvPlugin = YUVNode::YuvPlugin;
using Selection = YuvPlugin::Selection;
using PluginBufferHandlePtr =  NSPipelinePlugin::BufferHandle::Ptr;


/*******************************************************************************
* Type Alias.
********************************************************************************/
static constexpr MINT INVALID_FORMAT = -1;


/*******************************************************************************
* Class Define
*******************************************************************************/
class YuvInterface : public YuvPlugin::IInterface
{
public:
    virtual MERROR offer(YuvPlugin::Selection& sel)
    {
        sel.mIBufferFull
            .addSupportFormat(eImgFmt_NV12)
            .addSupportFormat(eImgFmt_YV12)
            .addSupportFormat(eImgFmt_YUY2)
            .addSupportFormat(eImgFmt_NV21)
            .addSupportFormat(eImgFmt_I420)
            .addSupportFormat(eImgFmt_YUV_P010)
            .addSupportFormat(eImgFmt_MTK_YUV_P010)
            .addSupportFormat(eImgFmt_YUV_P010)
            .addSupportSize(eImgSize_Full);

        sel.mIBufferClean
            .addSupportFormat(eImgFmt_NV12)
            .addSupportFormat(eImgFmt_YV12)
            .addSupportFormat(eImgFmt_YUY2)
            .addSupportFormat(eImgFmt_NV21)
            .addSupportFormat(eImgFmt_I420)
            .addSupportFormat(eImgFmt_YUV_P010)
            .addSupportFormat(eImgFmt_MTK_YUV_P010)
            .addSupportFormat(eImgFmt_YUV_P010)
            .addSupportSize(eImgSize_Full);

        sel.mIBufferDepth
            .addSupportFormat(eImgFmt_Y8)
            .addSupportSize(eImgSize_Specified);

        sel.mOBufferFull
            .addSupportFormat(eImgFmt_NV12)
            .addSupportFormat(eImgFmt_YV12)
            .addSupportFormat(eImgFmt_YUY2)
            .addSupportFormat(eImgFmt_NV21)
            .addSupportFormat(eImgFmt_I420)
            .addSupportFormat(eImgFmt_YUV_P010)
            .addSupportFormat(eImgFmt_MTK_YUV_P010)
            .addSupportFormat(eImgFmt_YUV_P010)
            .addSupportSize(eImgSize_Full);

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

        return OK;
    };

    virtual ~YuvInterface() {};
};

REGISTER_PLUGIN_INTERFACE(Yuv, YuvInterface);

/******************************************************************************
*
******************************************************************************/
class YuvCallback : public YuvPlugin::RequestCallback
{
public:
    YuvCallback(YUVNode* pNode)
        : mpNode(pNode)
    {
    }

    virtual void onAborted(YuvPlugin::Request::Ptr pPlgRequest)
    {
        MY_LOGD("onAborted request: %p", pPlgRequest.get());
        onCompleted(pPlgRequest, UNKNOWN_ERROR);
    }

    virtual void onCompleted(YuvPlugin::Request::Ptr pPlgRequest, MERROR result)
    {
        RequestPtr pCapRequest = mpNode->findRequest(pPlgRequest);
        if (pCapRequest == NULL) {
            MY_LOGE("unknown request happened: %p, result %d", pPlgRequest.get(), result);
            return;
        }
        MY_LOGD("onCompleted plugin request:%p, result:%d, needProcessSub:%d, R/F/S Num: %d/%d/%d",
                    pPlgRequest.get(), result,
                    pPlgRequest->mNeedProcessSub,
                    pCapRequest->getRequestNo(), pCapRequest->getFrameNo(), pPlgRequest->mSensorId);
        if (result != OK) {
            pCapRequest->addParameter(PID_FAILURE, 1);
        }

        if (pPlgRequest->mNeedProcessSub) {
            mpNode->onRequestProcessSub(pCapRequest);
            return;
        }

        //
        MINT32 repeat = pCapRequest->getParameter(PID_REQUEST_REPEAT);
        NodeID_T nodeID = mpNode->getNodeID()+repeat;
        pCapRequest->decNodeReference(nodeID);
        //
        *pPlgRequest = YuvPlugin::Request();

        MBOOL bRepeat = mpNode->onRequestRepeat(pCapRequest);
        // if repeat
        if (bRepeat) {
            mpNode->onRequestProcess(pCapRequest);
        } else {
            mpNode->onRequestFinish(pCapRequest);
        }
    }

    virtual void onNextCapture(YuvPlugin::Request::Ptr pPlgRequest)
    {
        RequestPtr pRequest = mpNode->findRequest(pPlgRequest);

        if (pRequest == NULL) {
            MY_LOGE("unknown request happened: %p", pPlgRequest.get());
            return;
        }

        const MUINT32 frameCount = pRequest->getActiveFrameCount();
        const MUINT32 frameIndex = pRequest->getActiveFrameIndex();
        MY_LOGW("not support next capture at (%dI%d)", frameIndex, frameCount);
    }

    virtual ~YuvCallback() { };
private:
    YUVNode* mpNode;
};

/******************************************************************************
*
******************************************************************************/
/**
 * @brief the class, for caching the partial result of selection, that had Negotiated.
 *
 * the class provides the guarantee of thread-safe
 */
class YUVNode::NegotiatedCacher final
{
public:
    NegotiatedCacher();

public:
    MBOOL getIsFeatureIdExisting(FeatureID_T featureId) const;

    MVOID add(FeatureID_T featureId, const std::vector<MINT>& iFmts, const std::vector<MINT>& oFmts);

    MBOOL getIsSupportedInputBufferFmt(FeatureID_T featureId, MINT fmt) const;

    MBOOL getSupportedInputBufferFmt(FeatureID_T featureId, MINT& fmt) const;

    MBOOL getIsSupportedOutputBufferFmt(FeatureID_T featureId, MINT fmt) const;

    MBOOL getSupportedOutputBufferFmt(FeatureID_T featureId, MINT& fmt) const;

private:
    struct NegotiatedInfo
    {
    public:
        inline const std::string toString() const;

    public:
        std::vector<MINT> mIFmts;
        std::vector<MINT> mOFmts;
    };

private:
    MBOOL innerGetIsFeatureIdExisting(FeatureID_T featureId) const;

private:
    using NegotiatedInfoTable = std::map<FeatureID_T, const NegotiatedInfo>;

private:
    mutable std::mutex  mLocker;
    NegotiatedInfoTable mNegotiatedInfoTable;
};
/**
 * @brief helper class used by YuvNode
 */
class YuvNodeHelper final
{
public:
    YuvNodeHelper() = delete;

public:
    static MBOOL existNodeBuffer(const sp<CaptureFeatureNodeRequest>& pNodeReq, TypeID_T typeId, Direction dir);

    static MINT getImageFormat(const sp<CaptureFeatureNodeRequest>& pNodeReq, TypeID_T typeId, Direction dir);

    static MBOOL tryGetTheSameBufferFmt(const Selection& selection, MINT& iFmt, MINT& oFmt);

    static MBOOL getIsDumpImage(NodeID_T nodeId);

};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NegotiatedCacher Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
YUVNode::NegotiatedCacher::
NegotiatedCacher()
{

}

MBOOL
YUVNode::NegotiatedCacher::
getIsFeatureIdExisting(FeatureID_T featureId) const
{
    std::lock_guard<std::mutex> lock(mLocker);
    return innerGetIsFeatureIdExisting(featureId);
}

MVOID
YUVNode::NegotiatedCacher::
add(FeatureID_T featureId, const std::vector<MINT>& iFmts, const std::vector<MINT>& oFmts)
{
    std::lock_guard<std::mutex> lock(mLocker);
    if (innerGetIsFeatureIdExisting(featureId)) {
        MY_LOGW("invalid operator, featureId is existing, id:%s, value:%s",
            FeatID2Name(featureId), mNegotiatedInfoTable[featureId].toString().c_str());
        return;
    }
    //
    NegotiatedInfo info;
    info.mIFmts = iFmts;
    info.mOFmts = oFmts;
    mNegotiatedInfoTable.insert(std::pair<FeatureID_T, NegotiatedInfo>(featureId, info));
    MY_LOGD("insert new item into negotiated table, id:%s, value:%s", FeatID2Name(featureId), mNegotiatedInfoTable[featureId].toString().c_str());
}

MBOOL
YUVNode::NegotiatedCacher::
getIsSupportedInputBufferFmt(FeatureID_T featureId, MINT fmt) const
{
    std::lock_guard<std::mutex> lock(mLocker);
    MBOOL ret = MFALSE;
    if (!innerGetIsFeatureIdExisting(featureId)) {
        MY_LOGW("the feature(%s) is not existing", FeatID2Name(featureId));
        return ret;
    }
    //
    const std::vector<MINT>& fmts = mNegotiatedInfoTable.at(featureId).mIFmts;
    if (std::find(fmts.begin(), fmts.end(), fmt) != fmts.end()) {
        ret = MTRUE;
    }
    return ret;
}

MBOOL
YUVNode::NegotiatedCacher::
getSupportedInputBufferFmt(FeatureID_T featureId, MINT& fmt) const
{
    std::lock_guard<std::mutex> lock(mLocker);
    MBOOL ret = MFALSE;
    if (!innerGetIsFeatureIdExisting(featureId)) {
        MY_LOGW("the feature(%s) is not existing", FeatID2Name(featureId));
        return ret;
    }
    //
    const std::vector<MINT>& fmts = mNegotiatedInfoTable.at(featureId).mIFmts;
    if (!fmts.empty()) {
        fmt = fmts.front();
        ret = MTRUE;
    }
    return ret;
}

MBOOL
YUVNode::NegotiatedCacher::
getIsSupportedOutputBufferFmt(FeatureID_T featureId, MINT fmt) const
{
    std::lock_guard<std::mutex> lock(mLocker);
    MBOOL ret = MFALSE;
    if (!innerGetIsFeatureIdExisting(featureId)) {
        MY_LOGW("the featureId:%s is not existing", FeatID2Name(featureId));
        return ret;
    }
    //
    const std::vector<MINT>& fmts = mNegotiatedInfoTable.at(featureId).mOFmts;
    if(std::find(fmts.begin(), fmts.end(), fmt) != fmts.end()) {
        ret = MTRUE;
    }
    return ret;
}

MBOOL
YUVNode::NegotiatedCacher::
getSupportedOutputBufferFmt(FeatureID_T featureId, MINT& fmt) const
{
    std::lock_guard<std::mutex> lock(mLocker);
    MBOOL ret = MFALSE;
    if (!innerGetIsFeatureIdExisting(featureId)) {
        MY_LOGW("the feature(%s) is not existing", FeatID2Name(featureId));
        return ret;
    }
    //
    const std::vector<MINT>& fmts = mNegotiatedInfoTable.at(featureId).mOFmts;
    if (!fmts.empty()) {
        fmt = fmts.front();
        ret = MTRUE;
    }
    return ret;
}

const std::string
YUVNode::NegotiatedCacher::NegotiatedInfo::
toString() const
{
    auto toString = [](const std::string& name, const std::vector<MINT>& items)
    {
        std::ostringstream out;

        out << "{"
            << name << ":";

        const int last = items.size() - 1;
        for(int i = 0; i <= last; i++) {
            out << std::hex << std::showbase <<  Utils::Format::queryImageFormatName(items[i])
                                             << "("
                                             << items[i]
                                             << ")";
            if(i != last)
                out << ", ";
        }
        out << "}";
        return out.str();
    };
    //
    std::ostringstream ret;
    ret << "{";
    ret << toString("iFmts", mIFmts);
    ret << ", ";
    ret << toString("oFmts", mOFmts);
    ret << "}";
    return ret.str();
}

MBOOL
YUVNode::NegotiatedCacher::
innerGetIsFeatureIdExisting(FeatureID_T featureId) const
{
    return (mNegotiatedInfoTable.count(featureId) > 0);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  YuvNodeHelper Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MBOOL
YuvNodeHelper::
existNodeBuffer(const sp<CaptureFeatureNodeRequest>& pNodeReq, TypeID_T typeId, Direction dir)
{
    BufferID_T bufId = pNodeReq->mapBufferID(typeId, dir);
    IImageBuffer* pImgBuffer = pNodeReq->acquireBuffer(bufId);
    return (pImgBuffer != nullptr);
}

MINT
YuvNodeHelper::
getImageFormat(const sp<CaptureFeatureNodeRequest>& pNodeReq, TypeID_T typeId, Direction dir)
{
    // NOTE: this function DOESN'T call releaseBuffer to decrease the buffer reference count
    BufferID_T bufId = pNodeReq->mapBufferID(typeId, dir);
    IImageBuffer* pImgBuffer = pNodeReq->acquireBuffer(bufId);
    if(pImgBuffer == NULL) {
        MY_LOGE("buffer(%s) is null", TypeID2Name(typeId));
        return eImgFmt_UNKNOWN;
    }
    return pImgBuffer->getImgFormat();
}

MBOOL
YuvNodeHelper::
tryGetTheSameBufferFmt(const Selection& selection, MINT& iFmt, MINT& oFmt)
{
    const std::vector<MINT>& iFmts = selection.mIBufferFull.getFormats();
    const std::vector<MINT>& oFmts = selection.mOBufferFull.getFormats();
    for(MINT fmt : iFmts) {
        if(std::find(oFmts.begin(), oFmts.end(), fmt) != oFmts.end()) {
            iFmt = fmt;
            oFmt = fmt;
            return MTRUE;
        }
    }
    return MFALSE;
}

MBOOL
YuvNodeHelper::
getIsDumpImage(NodeID_T nodeId)
{
    if (nodeId == NID_YUV) {
        return NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV>();
    }
    else if (nodeId == NID_YUV_R1) {
        return NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV_R1>();
    }
    else if (nodeId == NID_YUV_R2) {
        return NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV_R2>();
    }
    else if (nodeId == NID_YUV_R3) {
        return NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV_R3>();
    }
    else if (nodeId == NID_YUV_R4) {
        return NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV_R4>();
    }
    else if (nodeId == NID_YUV_R5) {
        return NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV_R5>();
    }
    else if (nodeId == NID_YUV2) {
        return  NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV2>();
    }
    else if (nodeId == NID_YUV2_R1) {
        return  NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV2_R1>();
    }
    else if (nodeId == NID_YUV2_R2) {
        return  NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV2_R2>();
    }
     else if (nodeId == NID_YUV2_R3) {
        return  NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV2_R3>();
    }
    else if (nodeId == NID_YUV2_R4) {
        return  NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV2_R4>();
    }
     else if (nodeId == NID_YUV2_R5) {
        return  NSCam::NSCamFeature::NSFeaturePipe::NSCapture::getIsDumpImage<NID_YUV2_R5>();
    }
    else {
        MY_LOGW("not supported nodeId, node:%d(%s)", nodeId, NodeID2Name(nodeId));
    }
    return MFALSE;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  YUVNode Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
YUVNode::YUVNode(NodeID_T nid, const char *name, MINT32 policy, MINT32 priority, MBOOL hasTwinNodes)
    : CamNodeULogHandler(nid == NID_YUV ? Utils::ULog::MOD_CAPTURE_YUV : Utils::ULog::MOD_CAPTURE_YUV_1)
    , CaptureFeatureNode(nid, name, policy, priority)
    , mpNegotiatedCacher(new NegotiatedCacher())
    , mHasTwinNodes(hasTwinNodes)
{
    TRACE_FUNC_ENTER();
    MY_LOGI("(%p) ctor", this);
    this->addWaitQueue(&mRequests);
    TRACE_FUNC_EXIT();
}

YUVNode::~YUVNode()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("(%p) dtor", this);
    TRACE_FUNC_EXIT();
}

MVOID YUVNode::setBufferPool(const android::sp<CaptureBufferPool> &pool)
{
    TRACE_FUNC_ENTER();
    mpBufferPool = pool;
    TRACE_FUNC_EXIT();
}

MBOOL YUVNode::onData(DataID id, const RequestPtr& pRequest)
{
    TRACE_FUNC_ENTER();
    MY_LOGD_IF(mLogLevel, "R/F/S:%d/%d/%d I/C:%d/%d %s arrived",
                pRequest->getRequestNo(), pRequest->getFrameNo(), pRequest->getMainSensorIndex(),
                pRequest->getActiveFrameIndex(), pRequest->getActiveFrameCount(),
                PathID2Name(id));
    MBOOL ret = MTRUE;

    if (pRequest->isReadyFor(mNodeId)) {
        pRequest->addParameter(PID_REQUEST_REPEAT, 0);
        mRequests.enque(pRequest);
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MVOID YUVNode::getSensorFmt(MINT32& sensorIndex, MINT32& sensorIndex2, MUINT* sensorFmt, MUINT* sensorFmt2)
{
    *sensorFmt = (sensorIndex >= 0) ? getSensorRawFmt(sensorIndex) : SENSOR_RAW_FMT_NONE;
    *sensorFmt2 = (sensorIndex2 >= 0) ? getSensorRawFmt(sensorIndex2) : SENSOR_RAW_FMT_NONE;
    MY_LOGD("sensorIndex:%d -> sensorFmt:%#09x, sensorIndex2:%d -> sensorFmt2:%#09x",
        sensorIndex, *sensorFmt, sensorIndex2, *sensorFmt2);
}

MBOOL YUVNode::onInit()
{
    TRACE_FUNC_ENTER();
    CaptureFeatureNode::onInit();
    MY_LOGI("(%p)(%d) uniqueKey:%d", this, mNodeId, mUsageHint.mPluginUniqueKey);

    mPlugin = YuvPlugin::getInstance(mUsageHint.mPluginUniqueKey);

    FeatureID_T featId;
    auto& vpProviders = mPlugin->getProviders();
    mpInterface = mPlugin->getInterface();

    std::vector<ProviderPtr> vSortedProvider = vpProviders;

    std::sort(vSortedProvider.begin(), vSortedProvider.end(),
            [] (const ProviderPtr& p1, const ProviderPtr& p2) {
                return p1->property().mPosition < p2->property().mPosition ||
                       p1->property().mPriority > p2->property().mPriority;
            });

    for (auto& pProvider : vSortedProvider) {
        const YuvPlugin::Property& rProperty =  pProvider->property();
        featId = NULL_FEATURE;

        if (mHasTwinNodes) {
            if (mNodeId == NID_YUV && rProperty.mPosition != 0)
                continue;
            if (mNodeId == NID_YUV2 && rProperty.mPosition != 1)
                continue;
        }

        if (rProperty.mFeatures & MTK_FEATURE_NR)
            featId = FID_NR;
        else if (rProperty.mFeatures & MTK_FEATURE_FB)
            featId = FID_FB;
        else if (rProperty.mFeatures & MTK_FEATURE_ABF)
            featId = FID_ABF;
        else if (rProperty.mFeatures & TP_FEATURE_FB)
            featId = FID_FB_3RD_PARTY;
        else if (rProperty.mFeatures & MTK_FEATURE_AINR_YUV)
            featId = FID_AINR_YUV;
        else if (rProperty.mFeatures & TP_FEATURE_RELIGHTING)
            featId = FID_RELIGHTING_3RD_PARTY;
        else if(rProperty.mFeatures & MTK_FEATURE_YHDR_FOR_AINR)
            featId = FID_AINR_YHDR;

        if (featId != NULL_FEATURE) {
            MY_LOGD_IF(mLogLevel, "%s finds plugin:%s, priority:%d",
                    NodeID2Name(mNodeId), FeatID2Name(featId), rProperty.mPriority);
            auto& item = mProviderPairs.editItemAt(mProviderPairs.add());
            item.mFeatureId = featId;
            item.mpProvider = pProvider;

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

    mpCallback = make_shared<YuvCallback>(this);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL YUVNode::onUninit()
{
    TRACE_FUNC_ENTER();
    MY_LOGI("(%p)(%d) uniqueKey:%d", this, mNodeId, mUsageHint.mPluginUniqueKey);

    for (ProviderPair& p : mProviderPairs) {
        ProviderPtr pProvider = p.mpProvider;
        FeatureID_T featId = p.mFeatureId;

        if (mInitMap.count(featId) > 0) {
            if (!mInitFeatures.hasBit(featId)) {
                MY_LOGD("Wait for initilizing + Feature: %s", FeatID2Name(featId));
                mInitMap[featId].wait();
                MY_LOGD("Wait for initilizing - Feature: %s", FeatID2Name(featId));
                mInitFeatures.markBit(featId);
            }
        }
    }

    mProviderPairs.clear();
    mInitMap.clear();
    mInitFeatures.clear();
    mpCurProvider = NULL;

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL YUVNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL YUVNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}


MBOOL YUVNode::onThreadLoop()
{
    TRACE_FUNC_ENTER();
    if (!waitAllQueue()) {
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

MBOOL YUVNode::onRequestRepeat(RequestPtr& pRequest)
{
    MINT32 repeat = pRequest->getParameter(PID_REQUEST_REPEAT);
    // move to next repeat YUV node
    repeat++;

    sp<CaptureFeatureNodeRequest> pNodeReq = pRequest->getNodeRequest(mNodeId + repeat);

    // no more repeating
    if (pNodeReq == NULL)
        return MFALSE;

    MY_LOGD("onRequestRepeat request:%d, repeat:%d",
            pRequest->getRequestNo(), repeat);

    {
        Mutex::Autolock _l(mPairLock);
        auto it = mRequestPairs.begin();
        for (; it != mRequestPairs.end(); it++) {
            if ((*it).mPipe == pRequest) {
                mRequestPairs.erase(it);
                break;
            }
        }
        if (mRequestPairs.empty()) {
            mpCurProvider = NULL;
        }
    }

    pRequest->addParameter(PID_REQUEST_REPEAT, repeat);

    return MTRUE;
}

MVOID YUVNode::onRequestProcessSub(RequestPtr& pRequest)
{
    MY_LOGD("onRequestProcessSub request:%d", pRequest->getRequestNo());

    {
        Mutex::Autolock _l(mPairLock);
        auto it = mRequestPairs.begin();
        for (; it != mRequestPairs.end(); it++) {
            if ((*it).mPipe == pRequest) {
                mRequestPairs.erase(it);
                break;
            }
        }
        if (mRequestPairs.empty()) {
            mpCurProvider = NULL;
        }
    }
    return;
}

MVOID YUVNode::processRequest(RequestPtr& pRequest, sp<CaptureFeatureNodeRequest> pNodeReq, FeatureID_T featureId, ProviderPtr pProvider, SensorAliasName sensorType)
{
    YuvNodeDataType dataType(sensorType);

    MINT32 requestNo = pRequest->getRequestNo();
    MINT32 frameNo = pRequest->getFrameNo();
    MINT32 sensorId = pRequest->getMainSensorIndex();
    MINT32 subSensorId = pRequest->getSubSensorIndex();
    MINT32 curSensorId = (sensorType == eSAN_Master)? sensorId : subSensorId;
    MINT32 repeat = pRequest->getParameter(PID_REQUEST_REPEAT);
    MBOOL isMasterIndex = pRequest->isMasterIndex(curSensorId);

    // create ISP profile mapping key information
    MUINT sensorFmt = -1, sensorFmt2 = -1;
    getSensorFmt(sensorId, subSensorId, &sensorFmt, &sensorFmt2);
    const MBOOL isBayerBayer = (sensorFmt != SENSOR_RAW_MONO) && (sensorFmt2 != SENSOR_RAW_MONO);
    const MBOOL isBayerMono = (sensorFmt != SENSOR_RAW_MONO) && (sensorFmt2 == SENSOR_RAW_MONO);

    // get tuning buffer that allocated in P2ANode for MSNR to fill the data
    SmartTuningBuffer pBufLCE2CALTM = nullptr;
    if (sensorType == eSAN_Master) {
        pBufLCE2CALTM = pRequest->getTuningBuffer(BID_LCE2CALTM_TUNING);
    }

    FovCalculator::FovInfo fovInfo;
    if(isVSDoFMode(mUsageHint)) {
        if (mpFOVCalculator->getIsEnable()) {
            if(!mpFOVCalculator->getFovInfo(curSensorId, fovInfo)){
                MY_LOGE("failed to getFovInfo");
            }
        }
    }

    string fullYuvBufferName;
    if(sensorType == eSAN_Master) {
        fullYuvBufferName = "INPUT_TID_MAN_FULL_YUV";
    } else {
        fullYuvBufferName = "INPUT_TID_SUB_FULL_YUV";
    }

    PluginSmartBufferHandlePtr iBufferFullHandle = NULL;
    PluginSmartBufferHandlePtr oBufferFullHandle = NULL;
    const MBOOL bExistIBuf = YuvNodeHelper::existNodeBuffer(pNodeReq, dataType.tid_full_yuv, INPUT);
    const MBOOL bExistOBuf = YuvNodeHelper::existNodeBuffer(pNodeReq, dataType.tid_full_yuv, OUTPUT);
    MBOOL bIsIOBufIDEqual = MFALSE;
    if (bExistIBuf && bExistOBuf) {
        const BufferID_T IBufId = pNodeReq->mapBufferID(dataType.tid_full_yuv, INPUT);
        const BufferID_T OBufId = pNodeReq->mapBufferID(dataType.tid_full_yuv, OUTPUT);
        bIsIOBufIDEqual = (IBufId == OBufId) ? MTRUE : MFALSE;
    }
    const MBOOL isInPlace = pProvider->property().mInPlace || bIsIOBufIDEqual;
    const MINT iFmt = bExistIBuf ? YuvNodeHelper::getImageFormat(pNodeReq, dataType.tid_full_yuv, INPUT) : -1;
    const MINT oFmt = bExistOBuf ? YuvNodeHelper::getImageFormat(pNodeReq, dataType.tid_full_yuv, OUTPUT) : -1;
    MY_LOGD("get in/out full buffer format, R/F/S Num: %d/%d/%d, feature:%s, iFmt:%s, oFmt:%s, isInPlace:%d",
        requestNo, frameNo, curSensorId,
        FeatID2Name(featureId),
        bExistIBuf ? ImgFmt2Name(iFmt) : "no-buffer-exist",
        bExistOBuf ? ImgFmt2Name(oFmt) : "no-buffer-exist",
        isInPlace);
    // get compatible input/output BufferHandle
    {
        // input buffer
        MINT workingIFmt = iFmt;
        if (bExistIBuf && !mpNegotiatedCacher->getIsSupportedInputBufferFmt(featureId, iFmt)
                && mpNegotiatedCacher->getSupportedInputBufferFmt(featureId, workingIFmt)) {
            MY_LOGD("input buffer is not support, iFmt:%s, workingIFmt:%s", ImgFmt2Name(iFmt), ImgFmt2Name(workingIFmt));

            const BufferID_T inputBufId = pNodeReq->mapBufferID(dataType.tid_full_yuv, INPUT);
            {
                const IImageBuffer* pInputImgBuf = pNodeReq->acquireBuffer(inputBufId);
                if(pInputImgBuf) {
                    iBufferFullHandle = PluginSmartBufferHandle::createInstance(fullYuvBufferName.c_str(), workingIFmt, curSensorId, pInputImgBuf, mpBufferPool);
                } else {
                    MY_LOGE("failed to acquire input buffer");
                }
            }
            pNodeReq->releaseBuffer(inputBufId);

            if (YuvNodeHelper::getIsDumpImage(mNodeId)) {
                std::ostringstream oss;
                oss << "inRealBuffer_" << NodeID2Name(mNodeId) << "_" << FeatID2Name(featureId);
                pRequest->dump(oss.str().c_str(), mNodeId, dataType.tid_full_yuv, INPUT);

                oss.str("");
                oss << "inWorkingBuffer_" << NodeID2Name(mNodeId) << "_" << FeatID2Name(featureId);
                pRequest->dump(oss.str().c_str(), mNodeId, iBufferFullHandle->getSmartImageBuffer()->getImageBufferPtr());
            }

            if (isInPlace) {
                if(iBufferFullHandle) {
                    oBufferFullHandle = PluginSmartBufferHandle::createInstance(fullYuvBufferName.c_str(), iBufferFullHandle->getSmartImageBuffer());
                } else {
                    MY_LOGE("iBufferFullHandle is NULL");
                    return;
                }
            }
        }
        //
        MINT workingOFmt = oFmt;
        if (bExistOBuf && !isInPlace) {
            if (!mpNegotiatedCacher->getIsSupportedOutputBufferFmt(featureId, oFmt)) {
                MY_LOGD("output buffer is not support, oFmt:%s", ImgFmt2Name(oFmt));
                if (mpNegotiatedCacher->getIsSupportedOutputBufferFmt(featureId, workingIFmt)) {
                    MY_LOGD("align the output buffer format to input, oFmt:%s -> %s",
                        ImgFmt2Name(oFmt), ImgFmt2Name(workingOFmt));
                    workingOFmt = workingIFmt;
                }
                else if (!mpNegotiatedCacher->getSupportedOutputBufferFmt(featureId, workingOFmt)) {
                    workingOFmt = INVALID_FORMAT;
                }

                if (workingOFmt != INVALID_FORMAT) {
                    const BufferID_T outputBufId = pNodeReq->mapBufferID(dataType.tid_full_yuv, OUTPUT);
                    {
                        IImageBuffer* pOutputImgBuf = pNodeReq->acquireBuffer(outputBufId);
                        if(pOutputImgBuf) {
                            oBufferFullHandle = PluginSmartBufferHandle::createInstance(fullYuvBufferName.c_str(), workingOFmt, pOutputImgBuf->getImgSize(), mpBufferPool);
                        } else {
                            MY_LOGE("failed to acquire output buffer");
                        }
                    }
                    pNodeReq->releaseBuffer(outputBufId);
                }
            }
        }
        // last, set the callback for copy the result to real output image buffer when handle is releasing
        if (oBufferFullHandle != NULL) {
            PluginBufferHandlePtr realOBufferFullHandle = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_full_yuv, OUTPUT);
            auto params = std::make_tuple(pRequest, curSensorId, mNodeId, featureId, repeat);
            auto onReleasingCB = [realOBufferFullHandle, params](IImageBuffer* pImgBuffer)
            {
                const MINT32 sensorIdx = std::get<1>(params);
                const NodeID_T nodeId = std::get<2>(params);
                const FeatureID_T featureId = std::get<3>(params);
                const MINT32 repeat = std::get<4>(params);
                RequestPtr pRequest = std::get<0>(params);

                IImageBuffer* dstImgBuf = realOBufferFullHandle->acquire();
                if (dstImgBuf != NULL) {
                    formatConverter(sensorIdx, pImgBuffer, dstImgBuf);
                    //
                    if (YuvNodeHelper::getIsDumpImage(nodeId)) {
                        std::ostringstream oss;
                        oss << "outWorkingBuffer_" << NodeID2Name(nodeId) << "_" << FeatID2Name(featureId);
                        pRequest->dump(oss.str().c_str(), nodeId, pImgBuffer);
                        oss.str("");
                        oss << "outRealBuffer_" << NodeID2Name(nodeId) << "_" << FeatID2Name(featureId);
                        pRequest->dump(oss.str().c_str(), nodeId, dstImgBuf);
                    }
                    //
                    realOBufferFullHandle->release();
                }
                else {
                    const MINT32 requestNo = pRequest->getRequestNo();
                    const MINT32 frameNo = pRequest->getFrameNo();
                    MY_LOGE("failed to acquire buffer from output buffer handle, R/F/S Num:%d/%d/%d node:%s, feature:%s, repret:%d",
                        requestNo, frameNo, sensorIdx, NodeID2Name(nodeId), FeatID2Name(featureId), repeat);
                }
            };
            oBufferFullHandle->setOnReleasingCallback(onReleasingCB);
        }
    }
    // hint for msnr, if it equals to true then raw msnr needs to add dce.
    MBOOL hasDCE = pRequest->hasFeature(FID_DCE) && !(pRequest->hasParameter(PID_REPROCESSING));
    MBOOL isRawInput = pRequest->hasParameter(PID_REPROCESSING) ? MFALSE:MTRUE;

    auto pPlgRequest = mPlugin->createRequest();

    pPlgRequest->mSensorId = curSensorId;
    pPlgRequest->mMainFrame = pRequest->isMainFrame();
    pPlgRequest->mRequestNo = pRequest->getRequestNo();
    pPlgRequest->mFrameNo = pRequest->getFrameNo();
    pPlgRequest->mIsPhysical     = pRequest->isPhysicalStream();
    pPlgRequest->mIsDualMode     = (mUsageHint.mDualMode!=0);
    pPlgRequest->mIsBayerBayer   = isBayerBayer;
    pPlgRequest->mIsBayerMono    = isBayerMono;
    pPlgRequest->mIsMasterIndex  = isMasterIndex;
    pPlgRequest->mIsVSDoFMode    = isVSDoFMode(mUsageHint);
    pPlgRequest->mHasDCE         = hasDCE;
    pPlgRequest->mNeedProcessSub = (sensorType == eSAN_Master && pRequest->isFeatureSupportSub(featureId));
    pPlgRequest->mIsRawInput     = isRawInput;
    pPlgRequest->mMultiframeType = pRequest->getParameter(PID_MULTIFRAME_TYPE);
    pPlgRequest->mFovBufferSize  = fovInfo.mDestinationSize;
    pPlgRequest->mFovCropRegion  = fovInfo.mFOVCropRegion;
    pPlgRequest->mIBufferFull    = (iBufferFullHandle == NULL) ? PluginHelper::CreateBuffer(pNodeReq, dataType.tid_full_yuv, INPUT) : std::move(iBufferFullHandle);
    pPlgRequest->mIBufferClean   = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_full_pure_yuv, INPUT);
    pPlgRequest->mIBufferDepth   = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_depth, INPUT);
    pPlgRequest->mIBufferDCES    = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_dces, INPUT);
    pPlgRequest->mIBufferLCS     = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_lcs, INPUT);
    pPlgRequest->mIBufferLCESHO  = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_lcesho, INPUT);
    pPlgRequest->mOBufferFull    = (oBufferFullHandle == NULL) ? PluginHelper::CreateBuffer(pNodeReq, dataType.tid_full_yuv, OUTPUT) : std::move(oBufferFullHandle);
    if(pBufLCE2CALTM != nullptr){
        pPlgRequest->mpBufLCE2CALTMVA = reinterpret_cast<void*>(pBufLCE2CALTM->mpVA);
        MY_LOGD("pBufLCE2CALTM VA: 0x%p", pPlgRequest->mpBufLCE2CALTMVA);
    }
    // TID_MAN_MSS_YUV/TID_MAN_RSZ_YUV/TID_MAN_IMGO_RSZ_YUV : one request can only exist one type in one time
    if(pNodeReq->mapBufferID(dataType.tid_mss_yuv, INPUT) != NULL_BUFFER)
        pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_mss_yuv, INPUT);
    else if(pNodeReq->mapBufferID(dataType.tid_imgo_rsz_yuv, INPUT) != NULL_BUFFER)
        pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_imgo_rsz_yuv, INPUT);
    else
        pPlgRequest->mIBufferResized    = PluginHelper::CreateBuffer(pNodeReq, dataType.tid_rsz_yuv, INPUT);

    pPlgRequest->mIMetadataDynamic = PluginHelper::CreateMetadata(pNodeReq, dataType.mid_in_p1_dynamic);
    pPlgRequest->mIMetadataApp = PluginHelper::CreateMetadata(pNodeReq, dataType.mid_in_app);
    pPlgRequest->mIMetadataHal = PluginHelper::CreateMetadata(pNodeReq, dataType.mid_in_hal);
    pPlgRequest->mOMetadataApp = PluginHelper::CreateMetadata(pNodeReq, dataType.mid_out_app);
    pPlgRequest->mOMetadataHal = PluginHelper::CreateMetadata(pNodeReq, dataType.mid_out_hal);

    // set PQ user id into meta for plugin use
    MINT64 userId = generatePQUserID(requestNo, pRequest->getMainSensorIndex(), pRequest->isPhysicalStream());
    trySetMetadata<MINT64>(pRequest->getMetadata(dataType.mid_in_hal)->native(), MTK_FEATURE_CAP_PQ_USERID, userId);

    //
    {
        Mutex::Autolock _l(mPairLock);
        auto& rPair = mRequestPairs.editItemAt(mRequestPairs.add());
        rPair.mPipe = pRequest;
        rPair.mPlugin = pPlgRequest;
        mpCurProvider = pProvider;
    }

    pProvider->process(pPlgRequest, mpCallback);

    MY_LOGI("(%p)(%d) -, R/F/S Num: %d/%d/%d", this, mNodeId, requestNo, frameNo, curSensorId);
    return;
}

MBOOL YUVNode::onRequestProcess(RequestPtr& pRequest)
{
    MINT32 repeat = pRequest->getParameter(PID_REQUEST_REPEAT);
    if (repeat == 0)
        incExtThreadDependency();

    MINT32 requestNo = pRequest->getRequestNo();
    MINT32 frameNo = pRequest->getFrameNo();
    MINT32 sensorId = pRequest->getMainSensorIndex();
    MINT32 subSensorId = pRequest->getSubSensorIndex();
    NodeID_T nodeId = mNodeId + repeat;
    // MBOOL isMasterIndex = pRequest->isMasterIndex(sensorId);
    //
    if (pRequest->isCancelled()){
        MY_LOGD("Cancel, R/F/S Num: %d/%d/%d", requestNo, frameNo, sensorId);
        onRequestFinish(pRequest);
        return MFALSE;
    }
    FeatureID_T featureId = NULL_FEATURE;
    CAM_TRACE_FMT_BEGIN("yuv(%d):process|r%df%ds%d",mNodeId, requestNo, frameNo, sensorId);
    MY_LOGI("(%p)(%d) +, R/F/S Num: %d/%d/%d", this, mNodeId, requestNo, frameNo, sensorId);

    sp<CaptureFeatureNodeRequest> pNodeReq = pRequest->getNodeRequest(nodeId);
    if (pNodeReq == NULL) {
        MY_LOGE("should not be here if no node request");
        pRequest->addParameter(PID_FAILURE, 1);
        onRequestFinish(pRequest);
        return MFALSE;
    }

    // pick a provider
    ProviderPtr pProvider = NULL;
    for (ProviderPair& p : mProviderPairs) {
        FeatureID_T featId = p.mFeatureId;
        MBOOL bSupportMultiFrame = p.mpProvider->property().mMultiFrame;
        MY_LOGD_IF(mLogLevel, "hasFeature:%d MultiFrame:%d, isMainFrame:%d R/F/S:%d/%d/%d",
                    pRequest->hasFeature(featId), bSupportMultiFrame, pRequest->isMainFrame(),
                    pRequest->getRequestNo(), pRequest->getFrameNo(), sensorId);
        if (pRequest->hasFeature(featId)
            // check the plugin's allow multiframe config
            && (bSupportMultiFrame || pRequest->isMainFrame())
        )
        {
            if (repeat > 0) {
                repeat--;
                continue;
            }
            pProvider = p.mpProvider;
            featureId = featId;
            break;
        }
    }

    if (pProvider == NULL) {
        MY_LOGE("do not execute a plugin");
        onRequestFinish(pRequest);
        return MFALSE;
    }

    if (!mInitFeatures.hasBit(featureId) && (mInitMap.count(featureId) > 0)) {
        MY_LOGD("Wait for initilizing + Feature: %s", FeatID2Name(featureId));
        mInitMap[featureId].wait();
        MY_LOGD("Wait for initilizing - Feature: %s", FeatID2Name(featureId));
        mInitFeatures.markBit(featureId);
    }

    MBOOL ret = MFALSE;
    processRequest(pRequest, pNodeReq, featureId, pProvider, eSAN_Master);

    if(subSensorId != -1 && pRequest->isFeatureSupportSub(featureId)) {
        MY_LOGD("%s supports SUB, perform process for SUB", FeatID2Name(featureId));
        processRequest(pRequest, pNodeReq, featureId, pProvider, eSAN_Sub_01);
    }
    ret = MTRUE;

    MY_LOGI("(%p)(%d) -, R/F/S Num: %d/%d/%d", this, mNodeId, requestNo, frameNo, sensorId);
    CAM_TRACE_FMT_END();
    return ret;
}

RequestPtr YUVNode::findRequest(PluginRequestPtr& pPlgRequest)
{
    Mutex::Autolock _l(mPairLock);
    for (const auto& rPair : mRequestPairs) {
        if (pPlgRequest == rPair.mPlugin) {
            return rPair.mPipe;
        }
    }

    return NULL;
}

MVOID YUVNode::onRequestFinish(RequestPtr& pRequest)
{
    MINT32 repeat = pRequest->getParameter(PID_REQUEST_REPEAT);
    NodeID_T nodeId = mNodeId + repeat;
    MINT32 requestNo = pRequest->getRequestNo();
    MINT32 frameNo = pRequest->getFrameNo();
    MINT32 sensorId = pRequest->getMainSensorIndex();
    CAM_TRACE_FMT_BEGIN("yuv(%s):finish|r%df%ds%d",NodeID2Name(nodeId), requestNo, frameNo, sensorId);
    MY_LOGI("(%p)(%s), R/F/S Num: %d/%d/%d", this, NodeID2Name(nodeId), requestNo, frameNo, sensorId);

    {
        Mutex::Autolock _l(mPairLock);
        auto it = mRequestPairs.begin();
        for (; it != mRequestPairs.end(); it++) {
            if ((*it).mPipe == pRequest) {
                mRequestPairs.erase(it);
                break;
            }
        }
        if (mRequestPairs.empty()) {
            mpCurProvider = NULL;
        }
    }

    pRequest->decNodeReference(nodeId);
    // if cancel request, decNodeRef all the other repeat yuv node
    repeat++;
    auto pRepeatReq = pRequest->getNodeRequest(mNodeId + repeat);
    while(repeat<MAX_YUV_REPEAT_NUM && pRepeatReq != NULL)
    {
        MY_LOGD("Call another yuv node repeat:%d decNodeReference", repeat);
        pRequest->decNodeReference(mNodeId+repeat);
        repeat++;
        pRepeatReq = pRequest->getNodeRequest(mNodeId + repeat);
    }
    dispatch(pRequest);

    decExtThreadDependency();
    CAM_TRACE_FMT_END();
    return;
}

MBOOL YUVNode::evaluateSelection(
    ProviderPtr pProvider,
    CaptureFeatureInferenceData& rInfer,
    Vector<CaptureFeatureInferenceData::SrcData>& rSrcData,
    Vector<CaptureFeatureInferenceData::DstData>& rDstData,
    Vector<MetadataID_T>& rMetadatas,
    Selection& sel,
    SensorAliasName sensorType)
{

    MBOOL isValid = MTRUE;
    YuvNodeDataType dataType(sensorType);

    const YuvPlugin::Property& rProperty =  pProvider->property();
    // YUVNode can support all kinds fmt's input/output
    // full size input
    if (sel.mIBufferFull.getRequired()) {
        if (sel.mIBufferFull.isValid()) {
            auto& src_0 = rSrcData.editItemAt(rSrcData.add());

            src_0.mTypeId = dataType.tid_full_yuv;
            if (!rInfer.hasType(dataType.tid_full_yuv))
                isValid = MFALSE;

            src_0.mSizeId = sel.mIBufferFull.getSizes()[0];
            src_0.setAllFmtSupport(MTRUE);
            // although can support all fmt, but still if same format the better
            src_0.addSupportFormats(sel.mIBufferFull.getFormats());

            // In-place processing must add a output
            if (rProperty.mInPlace || sel.mInPlace) {
                auto& dst_0 = rDstData.editItemAt(rDstData.add());
                dst_0.mTypeId = dataType.tid_full_yuv;
                dst_0.mSizeId = src_0.mSizeId;
                dst_0.mSize = rInfer.getSize(dataType.tid_full_yuv);
                dst_0.setAllFmtSupport(MTRUE);
                // inplace no need to set dst formats
                dst_0.mInPlace = true;
            }
        } else
            isValid = MFALSE;
    }
    // clean buffer
    if (sel.mIBufferClean.getRequired()) {
        if (sel.mIBufferClean.isValid()) {
            auto& src_1 = rSrcData.editItemAt(rSrcData.add());

            src_1.mTypeId = dataType.tid_full_pure_yuv;
            if (!rInfer.hasType(dataType.tid_full_pure_yuv))
                isValid = MFALSE;

            src_1.mSizeId = sel.mIBufferClean.getSizes()[0];
            src_1.addSupportFormats(sel.mIBufferClean.getFormats());

        } else
            isValid = MFALSE;
    }
    // depth
    if (sel.mIBufferDepth.getRequired()) {
        if (sel.mIBufferDepth.isValid()) {
            auto& src_1 = rSrcData.editItemAt(rSrcData.add());

            src_1.mTypeId = dataType.tid_depth;
            if (!rInfer.hasType(dataType.tid_depth))
                isValid = MFALSE;

            src_1.mSizeId = sel.mIBufferDepth.getSizes()[0];

        } else
            isValid = MFALSE;
    }
    // resize yuv
    if (sel.mIBufferResized.getRequired()) {
        if (sel.mIBufferResized.isValid()) {
            auto& src_1 = rSrcData.editItemAt(rSrcData.add());

            if(PlatformCapability::isSupport(PlatformCapability::HWCap_MSSMSF)
                && sel.mIBufferResized.hasCapability(eBufCap_MSS)) {
                src_1.mTypeId = dataType.tid_mss_yuv;
                // if there is mss output, can only support 10 bits full input
                for (size_t idx = 0; idx < rSrcData.size(); idx++) {
                    if (rSrcData.editItemAt(idx).mTypeId == dataType.tid_full_yuv) {
                        auto& srcFull = rSrcData.editItemAt(idx);
                        srcFull.setAllFmtSupport(MFALSE);
                    }
                }
            }
            else if(sel.mIBufferResized.hasCapability(eBufCap_IMGO))
                src_1.mTypeId = dataType.tid_imgo_rsz_yuv;
            else
                src_1.mTypeId = dataType.tid_rsz_yuv;

            if (!rInfer.hasType(src_1.mTypeId))
            {
                MY_LOGE("No %s output!", TypeID2Name(src_1.mTypeId));
                isValid = MFALSE;
            }

            src_1.mSizeId = sel.mIBufferResized.getSizes()[0];
            src_1.addSupportFormats(sel.mIBufferResized.getFormats());
            if (src_1.mSizeId == SID_SPECIFIED)
                src_1.mSize = sel.mIBufferResized.getSpecifiedSize();
        } else
            isValid = MFALSE;
    }
    // face data
    if (rProperty.mFaceData == eFD_Current) {
        auto& src_2 = rSrcData.editItemAt(rSrcData.add());
        src_2.mTypeId = dataType.tid_fd;
        src_2.mSizeId = NULL_SIZE;
        rInfer.markFaceData(eFD_Current);
    }
    else if (rProperty.mFaceData == eFD_Cache) {
        rInfer.markFaceData(eFD_Cache);
    }
    else if (rProperty.mFaceData == eFD_None) {
        rInfer.markFaceData(eFD_None);
    }
    else {
        MY_LOGW("unknow faceDateType:%x", rInfer.mFaceDateType.value);
    }
    // DCES
    if (sel.mIBufferDCES.getRequired()) {
        if (!rInfer.hasType(dataType.tid_dces))
            isValid = MFALSE;

        auto& src = rSrcData.editItemAt(rSrcData.add());
        src.mTypeId = dataType.tid_dces;
    }
    // LCSO
    if (sel.mIBufferLCS.getRequired()) {
        if (!rInfer.hasType(dataType.tid_lcs))
            isValid = MFALSE;

        auto& src = rSrcData.editItemAt(rSrcData.add());
        src.mTypeId = dataType.tid_lcs;
        src.mSizeId = SID_ARBITRARY;
    }
    // LCESHO
    if (sel.mIBufferLCESHO.getRequired()) {
        if (!rInfer.hasType(dataType.tid_lcesho))
            isValid = MFALSE;

        auto& src = rSrcData.editItemAt(rSrcData.add());
        src.mTypeId = dataType.tid_lcesho;
        src.mSizeId = SID_ARBITRARY;
    }

    // full size output
    if (!(rProperty.mInPlace || sel.mInPlace) && sel.mOBufferFull.getRequired()) {
        if (sel.mOBufferFull.isValid()) {
            auto& dst_0 = rDstData.editItemAt(rDstData.add());
            dst_0.mTypeId = dataType.tid_full_yuv;
            dst_0.mSizeId = sel.mOBufferFull.getSizes()[0];
            dst_0.setAllFmtSupport(MTRUE);
            dst_0.addSupportFormats(sel.mOBufferFull.getFormats());
            dst_0.mSize = rInfer.getSize(dataType.tid_full_yuv);
        } else
            isValid = MFALSE;
    }

    if (sel.mIMetadataDynamic.getRequired())
        rMetadatas.push_back(dataType.mid_in_p1_dynamic);

    if (sel.mIMetadataApp.getRequired() && sensorType == eSAN_Master)
        rMetadatas.push_back(dataType.mid_in_app);

    if (sel.mIMetadataHal.getRequired())
        rMetadatas.push_back(dataType.mid_in_hal);

    if (sel.mOMetadataApp.getRequired())
        rMetadatas.push_back(dataType.mid_out_app);

    if (sel.mOMetadataHal.getRequired() && sensorType == eSAN_Master)
        rMetadatas.push_back(dataType.mid_out_hal);

    return isValid;
}

MERROR YUVNode::evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInfer)
{
    (void) nodeId;

    // skip not main frame and not yuv process
    if(!rInfer.isMainFrame() && !rInfer.isYUVProcess())
        return OK;

    MBOOL isValid;
    MUINT8 uRepeatCount = 0;
    MERROR status = OK;

    MINT32 sensorId = rInfer.mSensorIndex;
    MINT32 subSensorId = rInfer.mSensorIndex2;
    MUINT sensorFmt = -1, sensorFmt2 = -1;
    getSensorFmt(sensorId, subSensorId, &sensorFmt, &sensorFmt2);
    const MBOOL isBayerBayer = (sensorFmt != SENSOR_RAW_MONO) && (sensorFmt2 != SENSOR_RAW_MONO);
    const MBOOL isBayerMono = (sensorFmt != SENSOR_RAW_MONO) && (sensorFmt2 == SENSOR_RAW_MONO);

    // Foreach all loaded plugin
    for (ProviderPair& p : mProviderPairs) {
        FeatureID_T featId = p.mFeatureId;

        if (!rInfer.hasFeature(featId)) {
            continue;
        } else if (uRepeatCount >= MAX_YUV_REPEAT_NUM) {
            MY_LOGE("over max repeating count(%d), ignore feature: %s", MAX_YUV_REPEAT_NUM, FeatID2Name(featId));
            continue;
        }

        auto& rSrcData = rInfer.getSharedSrcData();
        auto& rDstData = rInfer.getSharedDstData();
        auto& rFeatures = rInfer.getSharedFeatures();
        auto& rMetadatas = rInfer.getSharedMetadatas();

        isValid = MTRUE;

        ProviderPtr pProvider = p.mpProvider;
        const YuvPlugin::Property& rProperty =  pProvider->property();
        // only allow provider with the MultiFrame property = true
        if(!rProperty.mMultiFrame && !rInfer.isMainFrame())
            continue;

        Selection sel;
        sel.mSensorId = rInfer.mSensorIndex;
        sel.mIsMultiCamVSDoFMode = (HasFeatureVSDoF(rInfer) || isVSDoFMode(mUsageHint));
        mpInterface->offer(sel);
        sel.mMainFrame = rInfer.isMainFrame();
        sel.mIsRawInput = rInfer.isRawInput();
        sel.mAppSessionMeta = mUsageHint.mAppSessionMeta;
        sel.mIsHidlIsp = mUsageHint.mIsHidlIsp;
        sel.mIsPhysical = rInfer.mIsPhysical;
        sel.mIsDualMode = (mUsageHint.mDualMode!=0);
        sel.mIsBayerBayer = isBayerBayer;
        sel.mIsBayerMono = isBayerMono;
        sel.mIsMasterIndex = rInfer.isMasterIndex();

        {
            YuvNodeDataType masterDataType(eSAN_Master);
            sel.mFullBufferSize = rInfer.getSize(masterDataType.tid_full_yuv);
        }

        if(sel.mIsMultiCamVSDoFMode) {
            sel.mMultiCamFeatureMode = MTK_MULTI_CAM_FEATURE_MODE_VSDOF;

            FovCalculator::FovInfo fovInfo;
            if (mpFOVCalculator->getIsEnable() && mpFOVCalculator->getFovInfo(rInfer.mSensorIndex, fovInfo)) {
                sel.mFovBufferSize = fovInfo.mDestinationSize;
            }
            MY_LOGD_IF(mLogLevel, "hasVSDoF feature %d, is VSDoF mode %d", HasFeatureVSDoF(rInfer), isVSDoFMode(mUsageHint));
        }

        auto HasFeatureAinr = [] (CaptureFeatureInferenceData& t) -> bool
        {
            return t.hasFeature(FID_AINR) || t.hasFeature(FID_AINR_YHDR);
        };

        if(HasFeatureAinr(rInfer)) {
            sel.mIsAinr = MTRUE;
        }
        // precondition: yuv node is evaluated after multiframe node
        sel.mMultiframeType = rInfer.getMultiframeType();

        sel.mIMetadataApp.setControl(rInfer.mpIMetadataApp);

        sel.mIMetadataHal.setControl(rInfer.mpIMetadataHal);
        sel.mIMetadataDynamic.setControl(rInfer.mpIMetadataDynamic);

        if (pProvider->negotiate(sel) != OK) {
            MY_LOGD("bypass %s after negotiation", FeatID2Name(featId));
            rInfer.clearFeature(featId);
            continue;
        }

        MY_LOGD("evaluate featureid:%s", FeatID2Name(featId));

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

        MINT iFmt;
        MINT oFmt;
        if (!YuvNodeHelper::tryGetTheSameBufferFmt(sel, iFmt, oFmt))
        {
            iFmt = INVALID_FORMAT;
            oFmt = INVALID_FORMAT;
        }
        MY_LOGD("input and outout format align: iFmt:%s, Fmt:%s",
            iFmt == INVALID_FORMAT ? "invaild" : ImgFmt2Name(iFmt),
            oFmt == INVALID_FORMAT ? "invaild" : ImgFmt2Name(oFmt));


        isValid = evaluateSelection(pProvider, rInfer, rSrcData, rDstData, rMetadatas, sel, eSAN_Master);

        // negotiation for SUB if supported by feature
        if(subSensorId != -1 && sel.mIsFeatureSupportSub) {
            rInfer.mFeaturesSupportSub.markBit(featId);
            MY_LOGD("%s supports SUB, perform negotiation for SUB", FeatID2Name(featId));
            // update selection for SUB
            sel.mSensorId = subSensorId;
            sel.mIsMasterIndex = MFALSE;
            sel.mIMetadataHal.setControl(rInfer.mpIMetadataHal2);
            sel.mIMetadataDynamic.setControl(rInfer.mpIMetadataDynamic2);

            {
                YuvNodeDataType subDataType(eSAN_Sub_01);
                sel.mFullBufferSize = rInfer.getSize(subDataType.tid_full_yuv);
            }

            if (pProvider->negotiate(sel) != OK) {
                MY_LOGD("bypass %s after negotiation", FeatID2Name(featId));
                continue;
            }

            isValid = evaluateSelection(pProvider, rInfer, rSrcData, rDstData, rMetadatas, sel, eSAN_Sub_01);
        }

        if (isValid) {
            // we need to know what are the provider supported input/output buffer form section, that had negotiated,
            // so we cache the "first" mentioned negotiate result of each provider.
            // please note, cache the "first one" means we ASSERT it will not change in next negotiation.
            if (!mpNegotiatedCacher->getIsFeatureIdExisting(featId)) {
                mpNegotiatedCacher->add(featId, sel.mIBufferFull.getFormats(), sel.mOBufferFull.getFormats());
            }

            rFeatures.push_back(featId);
            // check force yuv
            MBOOL bForceRunYUV = MFALSE;
            if(rInfer.isYUVProcess())
            {
                MY_LOGD("YUV Process request found, force run YUVNode.");
                bForceRunYUV = MTRUE;
            }

            if(!rInfer.addNodeIO(mNodeId + uRepeatCount, rSrcData, rDstData, rMetadatas, rFeatures, bForceRunYUV))
            {
                status = BAD_VALUE;
                break;
            }
            uRepeatCount++;
        } else
            MY_LOGW("%s has an invalid evaluation: %s", NodeID2Name(nodeId), FeatID2Name(featId));
    }
    return status;
}

std::string YUVNode::getStatus(std::string& strDispatch)
{
    Mutex::Autolock _l(mPairLock);
    String8 str;
    if (mRequestPairs.size() > 0 && mpCurProvider) {
        const YuvPlugin::Property& rProperty = mpCurProvider->property();
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
