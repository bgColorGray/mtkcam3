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

#include "CaptureFeatureRequest.h"
#include "CaptureFeature_Common.h"

#include <utility>
#include <algorithm>

#define PIPE_CLASS_TAG "Request"
#define PIPE_TRACE TRACE_CAPTURE_FEATURE_DATA
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam3/feature/DualCam/DualCam.Common.h>
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_CAPTURE);

using namespace NSCam::Utils::ULog;
using namespace NSCam::TuningUtils;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

using android::RWLock;

CaptureFeatureRequest::CaptureFeatureRequest()
: mEnableBoost(MFALSE)
, mSensorIndex(-1)
, mSensorIndex2(-1)
, mMasterIndex(-1)
, mIsPhysical(MFALSE)
{
    clear();
    mLogLevel = ::property_get_int32(CAPTURE_PIPE_LOGGING_PROPERTY, 0);
}

CaptureFeatureRequest::~CaptureFeatureRequest()
{
}

MVOID CaptureFeatureRequest::addBuffer(BufferID_T bufId, sp<BufferHandle> pBufHandle)
{
    BufferItem item;
    // acquire when addBuffer
#define BUFFER_DESC(buf, type, crossable)   \
    case buf:                               \
        pBufHandle->acquire();              \
        item.mTypeId    = type;             \
        item.mCrossable = crossable;        \
        item.mCreated   = MTRUE;            \
        item.mpHandle   = pBufHandle;       \
        break;

    switch (bufId) {
        BUFFER_DESC(BID_MAN_IN_FULL,        TID_MAN_FULL_RAW,   MTRUE);
        BUFFER_DESC(BID_MAN_IN_YUV,         TID_MAN_FULL_YUV,   MTRUE);
        BUFFER_DESC(BID_MAN_IN_LCS,         TID_MAN_LCS,        MTRUE);
        BUFFER_DESC(BID_MAN_IN_LCESHO,      TID_MAN_LCESHO,     MTRUE);
        BUFFER_DESC(BID_MAN_IN_RSZ,         TID_MAN_RSZ_RAW,    MTRUE);
        BUFFER_DESC(BID_MAN_OUT_YUV00,      TID_MAN_CROP1_YUV,  MFALSE);
        BUFFER_DESC(BID_MAN_OUT_YUV01,      TID_MAN_CROP2_YUV,  MFALSE);
        BUFFER_DESC(BID_MAN_OUT_YUV02,      TID_MAN_CROP3_YUV,  MFALSE);
        BUFFER_DESC(BID_MAN_OUT_JPEG,       TID_JPEG,           MFALSE);
        BUFFER_DESC(BID_MAN_OUT_THUMBNAIL,  TID_THUMBNAIL,      MFALSE);
        BUFFER_DESC(BID_MAN_OUT_POSTVIEW,   TID_POSTVIEW,       MFALSE);
        BUFFER_DESC(BID_MAN_OUT_CLEAN,      TID_MAN_CLEAN,      MFALSE);
        BUFFER_DESC(BID_MAN_OUT_DEPTH,      TID_MAN_DEPTH,      MFALSE);
        BUFFER_DESC(BID_MAN_OUT_BOKEH,      TID_MAN_BOKEH,      MFALSE);
        BUFFER_DESC(BID_MAN_OUT_RAW,        TID_MAN_FULL_RAW,   MFALSE);
        BUFFER_DESC(BID_SUB_IN_FULL,        TID_SUB_FULL_RAW,   MTRUE);
        BUFFER_DESC(BID_SUB_IN_LCS,         TID_SUB_LCS,        MTRUE);
        BUFFER_DESC(BID_SUB_IN_LCESHO,      TID_SUB_LCESHO,     MTRUE);
        BUFFER_DESC(BID_SUB_IN_RSZ,         TID_SUB_RSZ_RAW,    MTRUE);

        case BID_SUB_OUT_YUV00:
        case BID_SUB_OUT_YUV01:
            MY_LOGE("unsupport buffer id: %d", bufId);
            return;
        default:
            MY_LOGE("unknown buffer id: %d", bufId);
            return;
    }

#undef BUFFER_DESC

    mBufferItems.add(bufId, item);
}

sp<BufferHandle> CaptureFeatureRequest::getBuffer(BufferID_T bufId)
{
    sp<BufferHandle> pBuffer;

    // get input buffers from cross request
    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL) {
        pCrossRequest->mBufferMutex.lock();

        auto& rBufferItems = pCrossRequest->mBufferItems;
        if (rBufferItems.indexOfKey(bufId) >= 0 &&
            rBufferItems.valueFor(bufId).mCrossable)
        {
            auto& rItem = rBufferItems.editValueFor(bufId);
            pBuffer = rItem.mpHandle;
            pCrossRequest->mBufferMutex.unlock();

            return pBuffer;
        } else {
            pCrossRequest->mBufferMutex.unlock();
        }
    }

    {
        mBufferMutex.lock();
        if (mBufferItems.indexOfKey(bufId) < 0) {
            mBufferMutex.unlock();
            return NULL;
        }

        auto& rItem = mBufferItems.editValueFor(bufId);
        pBuffer = rItem.mpHandle;
        mBufferMutex.unlock();
    }

    return pBuffer;
}

MVOID CaptureFeatureRequest::addTuningBuffer(BufferID_T bufId, SmartTuningBuffer pTuningBufHandle)
{
    mTuningBufferMutex.lock();
    mTuningBufferItems.add(bufId, pTuningBufHandle);
    mTuningBufferMutex.unlock();
}

SmartTuningBuffer CaptureFeatureRequest::getTuningBuffer(BufferID_T bufId)
{
    SmartTuningBuffer pTuningBuffer;
    {
        mTuningBufferMutex.lock();
        if (mTuningBufferItems.indexOfKey(bufId) < 0) {
            mTuningBufferMutex.unlock();
            return NULL;
        }

        auto& rItem = mTuningBufferItems.editValueFor(bufId);
        pTuningBuffer = rItem;
        mTuningBufferMutex.unlock();
    }

    return pTuningBuffer;
}

MVOID CaptureFeatureRequest::setCrossRequest(sp<CaptureFeatureRequest> pRequest)
{
    mpCrossRequest = pRequest;
}

sp<CaptureFeatureRequest> CaptureFeatureRequest::getCrossRequest()
{
    return mpCrossRequest.promote();
}

MVOID CaptureFeatureRequest::setDelayInference(std::function<void()>& func)
{
    Mutex::Autolock lock(mInferMutex);
    mInferFunction = move(func);
}

MBOOL CaptureFeatureRequest::hasDelayInference()
{
    Mutex::Autolock lock(mInferMutex);
    return mInferFunction != NULL;
}

MVOID CaptureFeatureRequest::startInference()
{
    if (hasDelayInference()) {
        Mutex::Autolock lock(mInferMutex);
        mInferFunction();
        mInferFunction = NULL;
        mInferCond.broadcast();
    }
}

MVOID CaptureFeatureRequest::waitInference()
{
    Mutex::Autolock lock(mInferMutex);
    if (mInferFunction != NULL) {
        MY_LOGI("wait+");
        mInferCond.wait(mInferMutex);
        MY_LOGI("wait-");
    }
}

MVOID CaptureFeatureRequest::addBufferItem(BufferID_T bufId, TypeID_T typeId, MSize& size, Format_T fmt, MSize& align)
{
    BufferItem item;
    item.mTypeId = typeId;
    item.mCreated = MFALSE;
    item.mSize = size;
    item.mFormat = fmt;
    item.mAlign  = align;
    mBufferItems.add(bufId, item);
}

MVOID CaptureFeatureRequest::addMetadata(MetadataID_T metaId, sp<MetadataHandle> pMetaHandle)
{
    MetadataItem item;

    switch (metaId) {
        case MID_MAN_IN_P1_DYNAMIC:
        case MID_MAN_IN_APP:
        case MID_MAN_IN_HAL:
        case MID_SUB_IN_P1_DYNAMIC:
        case MID_SUB_IN_HAL:
            item.mCrossable = MTRUE;
            break;

        case MID_MAN_OUT_APP:
        case MID_MAN_OUT_HAL:
        case MID_SUB_OUT_APP:
        case MID_SUB_OUT_HAL:
            break;

        default:
           MY_LOGE("unknown metadata id: %d", metaId);
           return;
    }
    // acquire the meta
    pMetaHandle->acquire();
    item.mpHandle = pMetaHandle;
    mMetadataItems.add(metaId, item);
}

sp<MetadataHandle> CaptureFeatureRequest::getMetadata(MetadataID_T metaId)
{
    sp<MetadataHandle> pMetadata;

    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL) {
        Mutex::Autolock lock(pCrossRequest->mMetadataMutex);

        auto& rMetadataItems = pCrossRequest->mMetadataItems;
        if (rMetadataItems.indexOfKey(metaId) >= 0 &&
            rMetadataItems.valueFor(metaId).mCrossable)
        {
            auto& rItem = rMetadataItems.editValueFor(metaId);
            pMetadata = rItem.mpHandle;
            return pMetadata;
        }
    }

    {
        Mutex::Autolock lock(mMetadataMutex);
        if (mMetadataItems.indexOfKey(metaId) < 0)
            return NULL;

        auto& rItem = mMetadataItems.editValueFor(metaId);
        pMetadata = rItem.mpHandle;
    }

    return pMetadata;
}

MVOID CaptureFeatureRequest::addParameter(ParameterID_T paramId, MINT32 value)
{
    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL)
    {
        RWLock::AutoWLock lock(pCrossRequest->mParamRWLock);
        pCrossRequest->mParameter[paramId] = value;
    }
    else
    {
        RWLock::AutoWLock _l(mParamRWLock);
        mParameter[paramId] = value;
    }
}

MVOID CaptureFeatureRequest::deleteParameter(ParameterID_T paramId)
{
    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL)
    {
        RWLock::AutoWLock lock(pCrossRequest->mParamRWLock);
        pCrossRequest->mParameter[paramId] = -1;
    }
    else
    {
        RWLock::AutoWLock _l(mParamRWLock);
        mParameter[paramId] = -1;
    }
}

MINT32 CaptureFeatureRequest::getParameter(ParameterID_T paramId, MBOOL isGetCross)
{
    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL && isGetCross)
    {
        RWLock::AutoRLock lock(pCrossRequest->mParamRWLock);
        return pCrossRequest->mParameter[paramId];
    }
    else
    {
        RWLock::AutoRLock _l(mParamRWLock);
        return mParameter[paramId];
    }
}

MBOOL CaptureFeatureRequest::hasParameter(ParameterID_T paramId)
{
    return getParameter(paramId) >= 0;
}

MINT32 CaptureFeatureRequest::getRequestNo()
{
    return getParameter(PID_REQUEST_NUM);
}

MINT32 CaptureFeatureRequest::getFrameNo(MBOOL isGetCross)
{
    return getParameter(PID_FRAME_NUM, isGetCross);
}

MBOOL CaptureFeatureRequest::detachResourceItems()
{
    {
        Mutex::Autolock lock(mBufferMutex);
        for (size_t i = 0; i < mBufferItems.size(); i++) {
            auto& rLocalItem = mBufferItems.editValueAt(i);
            if (rLocalItem.mpHandle != NULL) {
                MY_LOGW("[R/F Num: %d/%d] The bufferId(%d) hasn't been released. mReference(%d)",
                          getRequestNo(), getFrameNo(), mBufferItems.keyAt(i), rLocalItem.mReference);

                sp<BufferHandle> pBuffer = rLocalItem.mpHandle;
                pBuffer->release();
                rLocalItem.mpHandle = NULL;
            }
        }
    }
    {
        Mutex::Autolock lock(mMetadataMutex);
        for (size_t i = 0; i < mMetadataItems.size(); i++) {
            auto& rLocalItem = mMetadataItems.editValueAt(i);
            if (rLocalItem.mpHandle != NULL) {
                MY_LOGW("[R/F Num: %d/%d] The MetadataId(%d) hasn't been released. mReference(%d)",
                          getRequestNo(), getFrameNo(), mMetadataItems.keyAt(i), rLocalItem.mReference);

                sp<MetadataHandle> pHandle = rLocalItem.mpHandle;
                pHandle->release();
                rLocalItem.mpHandle = NULL;
            }
        }
    }
    return MTRUE;
}

MBOOL CaptureFeatureRequest::isSingleFrame()
{
    if (getParameter(PID_FRAME_COUNT) < 2)
        return MTRUE;
    else if(this->isUnderBSS() && this->getBSSFrameCount() < 2)
        return MTRUE;

    return MFALSE;
}

MBOOL CaptureFeatureRequest::isPhysicalStream()
{
    return mIsPhysical;
}

MBOOL CaptureFeatureRequest::isMasterIndex(MINT32 sensorIndex)
{
    return (mMasterIndex == sensorIndex);
}

MBOOL CaptureFeatureRequest::isPipelineFirstFrame()
{
    return (getPipelineFrameIndex() == 0);
}

MBOOL CaptureFeatureRequest::isPipelineLastFrame()
{
    return (getPipelineFrameCount() == (getPipelineFrameIndex() + 1));
}

MUINT32 CaptureFeatureRequest::getPipelineFrameCount()
{
    auto count = getParameter(PID_FRAME_COUNT);
    if(count< 2)
        return 1;
    return count;
}

MINT32 CaptureFeatureRequest::getPipelineFrameIndex()
{
    auto index = getParameter(PID_FRAME_INDEX);;
    if(index<0)
        return 0;
    return index;
}

MINT32 CaptureFeatureRequest::getMainSensorIndex()
{
    return mSensorIndex;
}

MINT32 CaptureFeatureRequest::getSubSensorIndex()
{
    return mSensorIndex2;
}

MBOOL CaptureFeatureRequest::isUnderBSS()
{
    if(hasParameter(PID_BSS_REQ_STATE) &&
       getParameter(PID_BSS_REQ_STATE) != BSS_STATE_NOT_BSS_REQ
    )
        return MTRUE;
    else
        return MFALSE;
}

MBOOL CaptureFeatureRequest::isBypassBSS()
{
    if(hasParameter(PID_BSS_REQ_STATE) &&
       getParameter(PID_BSS_REQ_STATE) == BSS_STATE_BYPASS_BSS
    )
        return MTRUE;
    else
        return MFALSE;
}

MBOOL CaptureFeatureRequest::isBSSFirstFrame()
{
    return (getBSSFrameIndex() == 0);
}

MBOOL CaptureFeatureRequest::isBSSLastFrame()
{
    return (getBSSFrameCount() == (getBSSFrameIndex() + 1));
}

MUINT32 CaptureFeatureRequest::getBSSFrameCount()
{
    auto frameCount = getParameter(PID_FRAME_COUNT);

    if (frameCount< 2)
        return 1;

    auto count = getParameter(PID_BSS_BYPASSED_COUNT);
    MINT32 bypassBSS = (count>0) ? count : 0;
    count = getParameter(PID_DROPPED_COUNT);
    MINT32 dropCount = (count>0) ? count : 0;

    return frameCount - bypassBSS - dropCount;
}

MINT32 CaptureFeatureRequest::getBSSFrameIndex()
{
    auto index = getParameter(PID_BSS_ORDER);
    if (index >= 0)
        return index;
    else
        return -1;
}

MBOOL CaptureFeatureRequest::isActiveFirstFrame()
{
    if(!this->isUnderBSS())
        return this->isPipelineFirstFrame();
    else
        return this->isBSSFirstFrame();
}

MBOOL CaptureFeatureRequest::isActiveLastFrame()
{
    if(!this->isUnderBSS())
        return this->isPipelineLastFrame();
    else
        return this->isBSSLastFrame();
}

MUINT32 CaptureFeatureRequest::getActiveFrameCount()
{
    if(!this->isUnderBSS())
        return this->getPipelineFrameCount();
    else
        return this->getBSSFrameCount();
}

MINT32 CaptureFeatureRequest::getActiveFrameIndex()
{
    if(!this->isUnderBSS())
        return this->getPipelineFrameIndex();
    else
        return this->getBSSFrameIndex();
}

MBOOL CaptureFeatureRequest::isMainFrame()
{
    if(this->getActiveFrameIndex() == 0)
        return MTRUE;
    else
        return MFALSE;
}


MBOOL CaptureFeatureRequest::isDropFrame()
{
    if(this->hasParameter(PID_DROPPED_FRAME) &&
        this->getParameter(PID_DROPPED_FRAME) == 1)
        return MTRUE;
    else
        return MFALSE;
}

MVOID CaptureFeatureRequest::addPath(PathID_T pathId)
{
    MY_LOGD_IF(mLogLevel, "add Path=%d %s", pathId, PathID2Name(pathId));
    NodeID_T srcNodeId;
    NodeID_T dstNodeId;
    MBOOL ret = GetPath(pathId, srcNodeId, dstNodeId);
    if (!ret) {
        MY_LOGD("Can not find the path: %d", pathId);
        return;
    }

    MY_LOGD_IF(mLogLevel, "addPath src=%s  dst=%s", NodeID2Name(srcNodeId), NodeID2Name(dstNodeId));

    mNodePath[srcNodeId][dstNodeId] = pathId;

    mPathToDo.markBit(pathId);
    mNodeToDo.markBit(dstNodeId);
}

MVOID CaptureFeatureRequest::lock() {
    mRequestMutex.lock();
}

MVOID CaptureFeatureRequest::unlock() {
    mRequestMutex.unlock();
}

MVOID CaptureFeatureRequest::finishPath_Locked(PathID_T pathId)
{
    mPathToDo.clearBit(pathId);
}

MVOID CaptureFeatureRequest::finishNode_Locked(NodeID_T nodeId)
{
    mNodeToDo.clearBit(nodeId);
}

MBOOL CaptureFeatureRequest::isFinished_Locked()
{
    return mPathToDo.isEmpty() && mNodeToDo.isEmpty();
}

MBOOL CaptureFeatureRequest::isReadyFor(NodeID_T nodeId)
{
    for (NodeID_T i = 0; i < NUM_OF_NODE; i++) {
        if (mNodePath[i][nodeId] == NULL_PATH)
            continue;

        PathID_T pathId = FindPath(i, nodeId);
        if (pathId == NULL_PATH)
            continue;

        if (mPathToDo.hasBit(pathId))
            return MFALSE;
    }

    return MTRUE;
}

MBOOL CaptureFeatureRequest::isCancelled()
{
    return (hasParameter(PID_ABORTED) && !hasParameter(PID_RESTORED)) ||
           hasParameter(PID_FAILURE) ||
           hasParameter(PID_DROPPED_FRAME);
}

Vector<NodeID_T> CaptureFeatureRequest::getPreviousNodes(NodeID_T nodeId)
{
    Vector<NodeID_T> vec;
    for (int i = 0; i < NUM_OF_NODE; i++) {
        if (mNodePath[i][nodeId] != NULL_PATH) {
            vec.push_back(i);
        }
    }
    return vec;
}

Vector<NodeID_T> CaptureFeatureRequest::getNextNodes(NodeID_T nodeId)
{
    Vector<NodeID_T> vec;
    for (int i = 0; i < NUM_OF_NODE; i++) {
        if (mNodePath[nodeId][i] != NULL_PATH) {
            vec.push_back(i);
        }
    }
    return vec;
}

std::string CaptureFeatureRequest::getFeatureString()
{
    std::string strFeature = "";
    for (FeatureID_T featId = 0; featId < NUM_OF_FEATURE; featId++) {
        if (mFeatures.hasBit(featId)) {
            if (strFeature.length() > 0) {
                strFeature += "/";
            }
            strFeature += (FeatID2Name(featId) ? FeatID2Name(featId) : "");
        }
    }
    return strFeature;
}

MVOID CaptureFeatureRequest::addFeature(FeatureID_T fid)
{
    mFeatures.markBit(fid);
}

MVOID CaptureFeatureRequest::setFeatures(MUINT64 features)
{
    mFeatures.value = features;
}

MBOOL CaptureFeatureRequest::hasFeature(FeatureID_T fid)
{
    return mFeatures.hasBit(fid);
}

MBOOL CaptureFeatureRequest::isFeatureSupportSub(FeatureID_T fid)
{
    return mFeaturesSupportSub.hasBit(fid);
}

MVOID CaptureFeatureRequest::removeFeature(FeatureID_T fid)
{
    return mFeatures.clearBit(fid);
}

MVOID CaptureFeatureRequest::setSensorIndex(MINT32 masterIndex, MINT32 sensorIndex, MINT32 sensorIndex2)
{
    TRACE_FUNC_ENTER();
    this->mMasterIndex = masterIndex;
    this->mSensorIndex = sensorIndex;
    this->mSensorIndex2 = sensorIndex2;
    TRACE_FUNC_EXIT();
}

MVOID CaptureFeatureRequest::setIsPhysicalStream(MBOOL isPhysical)
{
    mIsPhysical = isPhysical;
}

MVOID CaptureFeatureRequest::clear()
{
    memset(mNodePath, NULL_PATH, sizeof(mNodePath));
    {
        RWLock::AutoWLock _l(mParamRWLock);
        std::fill_n(mParameter, NUM_OF_TOTAL_PARAMETER, -1);
    }
}

MVOID CaptureFeatureRequest::addNodeIO(
        NodeID_T nodeId, Vector<BufferID_T>& vInBufId, Vector<BufferID_T>& vOutBufId, Vector<MetadataID_T>& vMetaId)
{
    sp<CaptureFeatureNodeRequest> pNodeRequest = new CaptureFeatureNodeRequest(this);

    for (BufferID_T bufId : vInBufId) {
        if (mBufferItems.indexOfKey(bufId) < 0) {
            MY_LOGE("can not find buffer, id:%d", bufId);
            continue;
        }

        TypeID_T typeId = mBufferItems.valueFor(bufId).mTypeId;

        pNodeRequest->mITypeMap.add(typeId, bufId);
        mBufferItems.editValueFor(bufId).mReference++;

        MY_LOGI("[R/F Num: %d/%d] In, nodeId(%d)(%s), typeId(%d)(%s), bufferId(%d), reference(%d)",
                  getRequestNo(), getFrameNo(), nodeId, NodeID2Name(nodeId), typeId, TypeID2Name(typeId), bufId, mBufferItems.editValueFor(bufId).mReference);
    }

    for (BufferID_T bufId : vOutBufId) {
        if (mBufferItems.indexOfKey(bufId) < 0) {
            MY_LOGE("can not find buffer, id:%d", bufId);
            continue;
        }

        TypeID_T typeId = mBufferItems.valueFor(bufId).mTypeId;

        pNodeRequest->mOTypeMap.add(typeId, bufId);
        mBufferItems.editValueFor(bufId).mReference++;

        MY_LOGI("[R/F Num: %d/%d] Out nodeId(%d)(%s), typeId(%d)(%s), bufferId(%d), reference(%d)",
                  getRequestNo(), getFrameNo(), nodeId, NodeID2Name(nodeId), typeId, TypeID2Name(typeId), bufId, mBufferItems.editValueFor(bufId).mReference);

    }

    for (MetadataID_T metaId : vMetaId) {
        if (mMetadataItems.indexOfKey(metaId) < 0)
            continue;

        pNodeRequest->mMetadataSet.markBit(metaId);
        mMetadataItems.editValueFor(metaId).mReference++;

        MY_LOGI("[R/F Num: %d/%d] nodeId(%d)(%s), metaId(%d), reference(%d)",
                  getRequestNo(), getFrameNo(), nodeId, NodeID2Name(nodeId), metaId, mMetadataItems.editValueFor(metaId).mReference);

    }
    MY_LOGD("Add node requet, node=%s", NodeID2Name(nodeId));
    mNodeRequest.add(nodeId, pNodeRequest);

}

MVOID CaptureFeatureRequest::incBufferRef(BufferID_T bufId)
{
    Mutex::Autolock lock(mBufferMutex);
    if (mBufferItems.indexOfKey(bufId) >= 0) {
        MUINT32& ref = mBufferItems.editValueFor(bufId).mReference;

        if (ref <= 0) {
            MY_LOGD("Buffer was already released. frame:%d, buffer:%d, ref:%d",
                getFrameNo(), bufId, ref);
        } else {
            ref++;
        }
    }
}

MVOID CaptureFeatureRequest::decBufferRef(BufferID_T bufId)
{
    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL) {
        bool bCrossBuffer = false;
        {
            Mutex::Autolock lock(pCrossRequest->mBufferMutex);
            auto& rBufferItems = pCrossRequest->mBufferItems;
            ssize_t index = rBufferItems.indexOfKey(bufId);
            if(index >= 0)
                bCrossBuffer = rBufferItems.valueFor(bufId).mCrossable;
        }
        if (bCrossBuffer) {
            MUINT32 crossRef;
            {
                Mutex::Autolock lock(mBufferMutex);
                if (mBufferItems.indexOfKey(bufId) < 0) {
                    MY_LOGE("Local buffer(%d) is not existed.", bufId);
                    return;
                }
                // The cross request is to exchange the buffer handle, not to exchange reference count
                // Minus to the local reference count
                auto& rLocalItem = mBufferItems.editValueFor(bufId);

                MUINT32& ref = rLocalItem.mReference;
                crossRef = --ref;
            }
            TypeID_T typeId = NULL_TYPE;
            if (crossRef <= 0)
            {
                Mutex::Autolock lock(pCrossRequest->mBufferMutex);
                auto& rBufferItems = pCrossRequest->mBufferItems;
                auto& rCrossItem = rBufferItems.editValueFor(bufId);
                typeId = rCrossItem.mTypeId;
                if (rCrossItem.mpHandle != NULL) {
                    sp<BufferHandle> pBuffer = rCrossItem.mpHandle;
                    pBuffer->release();
                    rCrossItem.mpHandle = NULL;

                    MY_LOGD_IF(mLogLevel, "The cross buffer(%d)(%s) has been released.", bufId, TypeID2Name(typeId));

                } else {
                    MY_LOGW("The buffer(%d)(%s) had been released.", bufId, TypeID2Name(typeId));
                }
            }

            MY_LOGD_IF(mLogLevel, "[R/F Num: %d/%d] decRef(cross) bufId(%d), typeId(%s), reference(%d)",
                          getRequestNo(), getFrameNo(), bufId, TypeID2Name(typeId), crossRef);

            return;
        }
    }

    {
        Mutex::Autolock lock(mBufferMutex);
        if (mBufferItems.indexOfKey(bufId) >= 0) {
            auto& rLocalItem = mBufferItems.editValueFor(bufId);
            TypeID_T typeId = rLocalItem.mTypeId;
            MUINT32& ref = rLocalItem.mReference;
            if (--ref <= 0) {
                // BSS: no allocated buffer if dropped
                if (rLocalItem.mpHandle != NULL) {
                    sp<BufferHandle> pBuffer = rLocalItem.mpHandle;
                    pBuffer->release();
                    rLocalItem.mpHandle = NULL;

                    MY_LOGD_IF(mLogLevel, "The buffer(%d) has been released.", bufId);

                }
            }

            MY_LOGD_IF(mLogLevel, "[R/F Num: %d/%d] decRef bufId(%d), typeId(%s), reference(%d)",
                      getRequestNo(), getFrameNo(), bufId, TypeID2Name(typeId), rLocalItem.mReference);
        }
    }
}

MVOID CaptureFeatureRequest::decMetadataRef(MetadataID_T metaId)
{
    auto pCrossRequest = mpCrossRequest.promote();
    if (pCrossRequest != NULL) {
        bool bCrossMeta;
        {
            Mutex::Autolock lock(pCrossRequest->mMetadataMutex);
            auto& rMetadataItems = pCrossRequest->mMetadataItems;
            bCrossMeta = rMetadataItems.indexOfKey(metaId) >= 0 && rMetadataItems.valueFor(metaId).mCrossable;
        }
        if (bCrossMeta) {
            MUINT32 crossRef;
            {
                Mutex::Autolock lock(mMetadataMutex);
                if (mMetadataItems.indexOfKey(metaId) < 0) {
                    MY_LOGE("Local metadata(%d) is not existed.", metaId);
                    return;
                }

                // The cross request is to exchange the metadata handle, not to exchange reference count
                // Minus to the local reference count
                auto& rLocalItem = mMetadataItems.editValueFor(metaId);
                MUINT32& ref =  rLocalItem.mReference;
                crossRef = --ref;
            }
            if (crossRef <= 0)
            {
                Mutex::Autolock lock(pCrossRequest->mMetadataMutex);
                auto& rMetadataItems = pCrossRequest->mMetadataItems;
                auto& rCrossItem = rMetadataItems.editValueFor(metaId);
                if (rCrossItem.mpHandle != NULL) {
                    sp<MetadataHandle> pHandle = rCrossItem.mpHandle;
                    pHandle->release();
                    rCrossItem.mpHandle = NULL;
                } else {
                    MY_LOGW("The metadata(%d) had been released.", metaId);
                }
            }

            MY_LOGD_IF(mLogLevel, "[R/F Num: %d/%d] decRef(cross) metaId(%d), reference(%d)",
                        getRequestNo(), getFrameNo(), metaId, crossRef);

            return;
        }
    }

    {
        Mutex::Autolock lock(mMetadataMutex);
        if (mMetadataItems.indexOfKey(metaId) >= 0) {
            auto& rLocalItem = mMetadataItems.editValueFor(metaId);

            MUINT32& ref =  rLocalItem.mReference;
            if (--ref <= 0) {
                // BSS: no allocated buffer if dropped
                if (rLocalItem.mpHandle != NULL) {
                    sp<MetadataHandle> pHandle = rLocalItem.mpHandle;
                    pHandle->release();
                    rLocalItem.mpHandle = NULL;
                }
            }

            MY_LOGD_IF(mLogLevel, "[R/F Num: %d/%d] decRef metaId(%d), reference(%d)",
                    getRequestNo(), getFrameNo(), metaId, rLocalItem.mReference);
        }
    }
}

sp<CaptureFeatureNodeRequest> CaptureFeatureRequest::getNodeRequest(NodeID_T nodeId)
{
    if (mNodeRequest.indexOfKey(nodeId) >= 0)
        return mNodeRequest.valueFor(nodeId);

    return NULL;
}

MVOID CaptureFeatureRequest::decNodeReference(NodeID_T nodeId)
{

    MY_LOGI("[R/F Num: %d/%d] decNodeReference nodeId(%s)",
            getRequestNo(), getFrameNo(), NodeID2Name(nodeId));

    if (mNodeRequest.indexOfKey(nodeId) < 0)
        return;
    sp<CaptureFeatureNodeRequest> pNodeReq = mNodeRequest.valueFor(nodeId);
    auto& rITypeMap = pNodeReq->mITypeMap;
    auto& rOTypeMap = pNodeReq->mOTypeMap;
    auto& rITypeReleased = pNodeReq->mITypeReleased;
    auto& rOTypeReleased = pNodeReq->mOTypeReleased;
    auto& rOMetaReleased = pNodeReq->mMetaReleased;

    for (size_t i = 0; i < rITypeMap.size(); i++) {
        TypeID_T typeId = rITypeMap.keyAt(i);
        BufferID_T bufId = rITypeMap.valueAt(i);
        if (!rITypeReleased.hasBit(typeId)) {
            rITypeReleased.markBit(typeId);
            decBufferRef(bufId);
        }
    }

    for (size_t i = 0; i < rOTypeMap.size(); i++) {
        BufferID_T bufId = rOTypeMap.valueAt(i);
        TypeID_T typeId = rOTypeMap.keyAt(i);
        if (!rOTypeReleased.hasBit(typeId)) {
            rOTypeReleased.markBit(typeId);
            decBufferRef(bufId);
        }
    }

    for (MetadataID_T i = 0; i < NUM_OF_METADATA; i++) {
        if (pNodeReq->hasMetadata(i) && !rOMetaReleased.hasBit(i))
        {
            rOMetaReleased.markBit(i);
            decMetadataRef(i);
        }
    }
    return;
}

MVOID CaptureFeatureRequest::dump()
{
    ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG, "[dump]");
    dump(logPrinter);
}

MVOID CaptureFeatureRequest::dump(android::Printer& printer)
{
    String8 strFeature;
    for (FeatureID_T featId = 0; featId < NUM_OF_FEATURE; featId++) {
        if (mFeatures.hasBit(featId)) {
            if (strFeature.length() > 0) {
                strFeature += "/";
            }
            strFeature += FeatID2Name(featId);
        }
    }

    if (hasParameter(PID_FRAME_INDEX) && hasParameter(PID_FRAME_COUNT)) {
        printer.printFormatLine("  [R/F Num: %d/%d] [Multi-Frame Index/Count/BSSIndex/BSSCount: %d/%d/%d/%d Features: %s]",
                getRequestNo(),
                getFrameNo(),
                getPipelineFrameIndex(),
                getPipelineFrameCount(),
                getBSSFrameIndex(),
                getBSSFrameCount(),
                strFeature.string());
    } else {
        printer.printFormatLine("  [R/F Num: %d/%d] [Features: %s]",
                getRequestNo(),
                getFrameNo(),
                strFeature.string());
    }

    auto pTool = NSCam::Utils::LogTool::get();
    for (size_t i = 0; i < mNodeRequest.size(); i++)
    {
        NodeID_T nodeId = mNodeRequest.keyAt(i);
        const sp<CaptureFeatureNodeRequest> pNodeReq = mNodeRequest.valueAt(i);

        timespec& rBegin = mNodeBegin[nodeId];
        timespec& rEnd = mNodeEnd[nodeId];
        printer.printFormatLine("      NODE %-10s [%s ~ %s]",
                String8::format("[%s]", NodeID2Name(nodeId)).string(),
                (rBegin.tv_sec) ? pTool->convertToFormattedLogTime(&rBegin).c_str() : "",
                (rEnd.tv_sec)   ? pTool->convertToFormattedLogTime(&rEnd).c_str()   : "");


        auto& rITypeMap = pNodeReq->mITypeMap;
        for (size_t j = 0; j < rITypeMap.size(); j++) {

            TypeID_T typeId = rITypeMap.keyAt(j);
            BufferID_T bufId = rITypeMap.valueAt(j);
            const sp<BufferHandle> pBuffer = getBuffer(bufId);

            if (pBuffer == NULL) {
                printer.printFormatLine("          INPUT-%zu     %-18s Released",
                        j, TypeID2Name(typeId));
            } else {
                printer.printFormatLine("          INPUT-%zu     %-18s Handle:0x%p",
                        j, TypeID2Name(typeId), pBuffer.get());
            }
        }

        auto& rOTypeMap = pNodeReq->mOTypeMap;
        for (size_t j = 0; j < rOTypeMap.size(); j++) {

            TypeID_T typeId = rOTypeMap.keyAt(j);
            BufferID_T bufId = rOTypeMap.valueAt(j);
            const sp<BufferHandle> pBuffer = getBuffer(bufId);
            if (pBuffer == NULL) {
                printer.printFormatLine("          OUTPUT-%zu    %-18s Released",
                        j, TypeID2Name(typeId));
            } else {
                printer.printFormatLine("          OUTPUT-%zu    %-18s Handle:0x%p",
                        j, TypeID2Name(typeId), pBuffer.get());
            }
        }

    }
}

MVOID CaptureFeatureRequest::dump(const char* name, NodeID_T nodeId, TypeID_T typeId, Direction dir)
{
    sp<CaptureFeatureNodeRequest> nodeRequestPtr = getNodeRequest(nodeId);
    const BufferID_T bufId = nodeRequestPtr->mapBufferID(typeId, dir);

    if ( bufId == NULL_BUFFER ) {
        MY_LOGW("failed to get bufId, nodeId:%d(%s), typeId:%d(%s), dir:%d, name:%s",
            nodeId, NodeID2Name(nodeId), typeId, TypeID2Name(typeId), dir, name);
    }
    else
    {
        sp<BufferHandle> bufferHandle = getBuffer(bufId);
        IImageBuffer* pImgBuf = bufferHandle->native();

        if (pImgBuf) {
            dump(name, nodeId, pImgBuf);
        }
        else {
            MY_LOGW("failed to dump due to image buffer is null!");
        }
    }
}

MVOID CaptureFeatureRequest::dump(const char* name, NodeID_T nodeId, IImageBuffer* pImgBuf)
{
    char filename[256] = {0};
    sp<CaptureFeatureNodeRequest> nodeRequestPtr = getNodeRequest(nodeId);
    const IMetadata* pInHalMeta = nodeRequestPtr->acquireMetadata(MID_MAN_IN_HAL);
    MINT32 iUniqueKey = 0;
    tryGetMetadata<MINT32>(pInHalMeta, MTK_PIPELINE_UNIQUE_KEY, iUniqueKey);

    MINT32 sensorId = getMainSensorIndex();
    FILE_DUMP_NAMING_HINT hint;
    hint.UniqueKey  = iUniqueKey;
    hint.RequestNo  = getRequestNo();
    hint.FrameNo    = getFrameNo();
    extract_by_SensorOpenId(&hint, sensorId);

    extract(&hint, pImgBuf);
    genFileName_YUV(filename, sizeof(filename), &hint, YUV_PORT_UNDEFINED, name);
    pImgBuf->saveToFile(filename);
    MY_LOGD("Dump: %s", filename);
}

std::string CaptureFeatureRequest::getStatus(std::string& strDispatch)
{
    String8 ret;
    String8 strFeature;
    String8 strNode;
    for (FeatureID_T featId = 0; featId < NUM_OF_FEATURE; featId++) {
        if (mFeatures.hasBit(featId)) {
            if (strFeature.length() > 0) {
                strFeature += "/";
            }
            strFeature += FeatID2Name(featId);
        }
    }

    if (hasParameter(PID_FRAME_INDEX) && hasParameter(PID_FRAME_COUNT)) {
        ret = String8::format("[R/F Num: %d/%d] [Multi-Frame Index/Count/BSSIndex/BSSCount: %d/%d/%d/%d Features: %s]",
                              getRequestNo(),
                              getFrameNo(),
                              getPipelineFrameIndex(),
                              getPipelineFrameCount(),
                              getBSSFrameIndex(),
                              getBSSFrameCount(),
                              strFeature.string());
    } else {
        ret = String8::format("  [R/F Num: %d/%d] [Features: %s]",
                              getRequestNo(),
                              getFrameNo(),
                              strFeature.string());
    }
    MY_LOGI("%s", ret.string());

    auto pTool = NSCam::Utils::LogTool::get();

    ret += String8::format(" [NOT Finish Node:");
    for (size_t i = 0; i < mNodeRequest.size(); i++)
    {
        NodeID_T nodeId = mNodeRequest.keyAt(i);
        const sp<CaptureFeatureNodeRequest> pNodeReq = mNodeRequest.valueAt(i);

        timespec& rBegin = mNodeBegin[nodeId];
        timespec& rEnd = mNodeEnd[nodeId];

        MY_LOGI("      NODE %-10s [%s ~ %s]",
                String8::format("[%s]", NodeID2Name(nodeId)).string(),
                (rBegin.tv_sec) ? pTool->convertToFormattedLogTime(&rBegin).c_str() : "",
                (rEnd.tv_sec)   ? pTool->convertToFormattedLogTime(&rEnd).c_str()   : "");

        auto& rITypeMap = pNodeReq->mITypeMap;
        for (size_t j = 0; j < rITypeMap.size(); j++) {

            TypeID_T typeId = rITypeMap.keyAt(j);
            BufferID_T bufId = rITypeMap.valueAt(j);
            const sp<BufferHandle> pBuffer = getBuffer(bufId);

            if (pBuffer == NULL) {
                MY_LOGI("          INPUT-%zu     %-18s Released",
                        j, TypeID2Name(typeId));
            } else {
                MY_LOGI("          INPUT-%zu     %-18s Handle:0x%p",
                        j, TypeID2Name(typeId), pBuffer.get());
            }
        }

        auto& rOTypeMap = pNodeReq->mOTypeMap;
        for (size_t j = 0; j < rOTypeMap.size(); j++) {

            TypeID_T typeId = rOTypeMap.keyAt(j);
            BufferID_T bufId = rOTypeMap.valueAt(j);
            const sp<BufferHandle> pBuffer = getBuffer(bufId);
            if (pBuffer == NULL) {
                MY_LOGI("          OUTPUT-%zu    %-18s Released",
                        j, TypeID2Name(typeId));
            } else {
                MY_LOGI("          OUTPUT-%zu    %-18s Handle:0x%p",
                        j, TypeID2Name(typeId), pBuffer.get());
            }
        }

        if (!rEnd.tv_sec) {
            if (strNode.length() > 0) {
                strNode += "/";
            }

            strNode += String8::format("%s", NodeID2Name(nodeId));
            if (strDispatch.size() == 0) {
                String8 strTmp = String8::format(" NOT Finish Node: %s", NodeID2Name(nodeId));
                strDispatch = strTmp.string() ? strTmp.string() : "";
            }
        }
    }
    ret += String8::format("%s]", strNode.c_str());

    return (ret.string() ? ret.string() : "");
}

BufferID_T CaptureFeatureNodeRequest::mapBufferID(TypeID_T typeId, Direction dir)
{
    BufferID_T bufId = NULL_BUFFER;
    if (dir == INPUT) {
        if (mITypeMap.indexOfKey(typeId) >= 0) {
            bufId = mITypeMap.valueFor(typeId);
            mIBufferMap.add(bufId, typeId);
        }
    } else {
        if (mOTypeMap.indexOfKey(typeId) >= 0) {
            bufId = mOTypeMap.valueFor(typeId);
            mOBufferMap.add(bufId, typeId);
        }
    }

    return bufId;
}

MVOID CaptureFeatureNodeRequest::queryTypeId(BufferID_T bufId, TypeID_T& typeId, Direction& dir)
{
    if (bufId != NULL_BUFFER) {
        if (mIBufferMap.indexOfKey(bufId) >= 0) {
            typeId = mIBufferMap.valueFor(bufId);
            dir = INPUT;
        }
        else if (mOBufferMap.indexOfKey(bufId) >= 0) {
            typeId = mIBufferMap.valueFor(bufId);
            dir = OUTPUT;
        }
    }
}

MBOOL CaptureFeatureNodeRequest::hasMetadata(MetadataID_T metaId)
{
    return mMetadataSet.hasBit(metaId);
}

IImageBuffer* CaptureFeatureNodeRequest::acquireBuffer(BufferID_T bufId)
{
    if (bufId == NULL_BUFFER)
        return NULL;

    if (mIBufferMap.indexOfKey(bufId) >= 0)
    {
        TypeID_T typeId = mIBufferMap.valueFor(bufId);
        if (mITypeAcquired.hasBit(typeId))
        {
            MY_LOGW("Input:%s already acquired", TypeID2Name(typeId));
        }
        mITypeAcquired.markBit(typeId);

    }
    else if (mOBufferMap.indexOfKey(bufId) >= 0)
    {
        TypeID_T typeId = mOBufferMap.valueFor(bufId);
        if (mOTypeAcquired.hasBit(typeId))
        {
            MY_LOGW("Output:%s already acquired", TypeID2Name(typeId));
        }
        mOTypeAcquired.markBit(typeId);
    }

    auto pBufferHandle = mpRequest->getBuffer(bufId);
    if (pBufferHandle == NULL)
        return NULL;

    return pBufferHandle->native();
}

std::vector<IImageBuffer*> CaptureFeatureNodeRequest::acquireBufferAll(BufferID_T bufId)
{
    if (bufId == NULL_BUFFER)
        return {};

    if (mIBufferMap.indexOfKey(bufId) >= 0) {
        TypeID_T typeId = mIBufferMap.valueFor(bufId);
        if (mITypeAcquired.hasBit(typeId))
        {
            MY_LOGW("Input:%s already acquired", TypeID2Name(typeId));
        }
        mITypeAcquired.markBit(typeId);
    }
    else if (mOBufferMap.indexOfKey(bufId) >= 0)
    {
        TypeID_T typeId = mOBufferMap.valueFor(bufId);
        if (mOTypeAcquired.hasBit(typeId))
        {
            MY_LOGW("Output:%s already acquired", TypeID2Name(typeId));
        }
        mOTypeAcquired.markBit(typeId);
    }

    auto pBufferHandle = mpRequest->getBuffer(bufId);
    if (pBufferHandle == NULL)
        return {};

    return pBufferHandle->nativeAll();
}

MBOOL CaptureFeatureNodeRequest::isMultiBuffer(BufferID_T bufId)
{
    if (bufId == NULL_BUFFER)
      return MFALSE;

    auto pBufferHandle = mpRequest->getBuffer(bufId);
    if (pBufferHandle == NULL)
      return MFALSE;

    return pBufferHandle->isMultiBuffer();
}

MVOID CaptureFeatureNodeRequest::releaseBuffer(BufferID_T bufId)
{
    MBOOL bAlreadyRelease = MFALSE;
    if (mIBufferMap.indexOfKey(bufId) >= 0)
    {
        TypeID_T typeId = mIBufferMap.valueFor(bufId);
        if(mITypeReleased.hasBit(typeId))
            bAlreadyRelease = MTRUE;
        mITypeReleased.markBit(typeId);
    }
    else if (mOBufferMap.indexOfKey(bufId) >= 0)
    {
        TypeID_T typeId = mOBufferMap.valueFor(bufId);
        if(mOTypeReleased.hasBit(typeId))
            bAlreadyRelease = MTRUE;
        mOTypeReleased.markBit(typeId);
    }

    if(!bAlreadyRelease)
        mpRequest->decBufferRef(bufId);
}

IMetadata* CaptureFeatureNodeRequest::acquireMetadata(MetadataID_T metaId)
{
    if (metaId == NULL_METADATA)
        return NULL;

    if (!hasMetadata(metaId))
        return NULL;

    sp<MetadataHandle> pMetaHandle = mpRequest->getMetadata(metaId);
    if (pMetaHandle == NULL)
        return NULL;

    return pMetaHandle->native();
}

MVOID CaptureFeatureNodeRequest::releaseMetadata(MetadataID_T metaId)
{
    if (!hasMetadata(metaId))
        return;

    MBOOL bAlreadyRelease = MFALSE;
    if(mMetaReleased.hasBit(metaId))
        bAlreadyRelease = MTRUE;

    if(!bAlreadyRelease)
    {
        mpRequest->decMetadataRef(metaId);
        mMetaReleased.markBit(metaId);
    }
}

MUINT32 CaptureFeatureNodeRequest::getImageTransform(BufferID_T bufId) const
{
    auto pBuffer = mpRequest->getBuffer(bufId);
    if (pBuffer == NULL)
        return 0;

    return pBuffer->getTransform();
}

MINT CaptureFeatureRequest::getImageFormat(BufferID_T bufId)
{
    if (mBufferItems.indexOfKey(bufId) < 0) {
        MY_LOGE("can not find buffer ID:%d", bufId);
        return 0;
    }

    if (bufId & PIPE_BUFFER_STARTER) {
        return  mBufferItems.valueFor(bufId).mFormat;
    } else {
        auto pBufferHandle = this->getBuffer(bufId);
        if (pBufferHandle == NULL)
            return 0;
        return pBufferHandle->native()->getImgFormat();
    }
}

MSize CaptureFeatureRequest::getImageSize(BufferID_T bufId)
{
    if (mBufferItems.indexOfKey(bufId) < 0) {
        MY_LOGE("can not find buffer ID:%d", bufId);
        return MSize(0, 0);
    }

    if (bufId & PIPE_BUFFER_STARTER) {
        return mBufferItems.valueFor(bufId).mSize;
    } else {
        auto pBufferHandle = this->getBuffer(bufId);
        if (pBufferHandle == NULL)
            return MSize(0, 0);
        return pBufferHandle->native()->getImgSize();
    }
}

MBOOL CaptureFeatureRequest::isTargetBuffer(BufferID_T bufId)
{
    if (mBufferItems.indexOfKey(bufId) < 0) {
        MY_LOGW("can not find buffer ID:%d", bufId);
        return MFALSE;
    }

    return mBufferItems.valueFor(bufId).mIsTarget;
}

MVOID CaptureFeatureRequest::startTimer(NodeID_T nodeId)
{
    if (auto pTool = NSCam::Utils::LogTool::get()) {
        pTool->getCurrentLogTime(&mNodeBegin[nodeId]);
    }
}

MVOID CaptureFeatureRequest::stopTimer(NodeID_T nodeId)
{
    if (auto pTool = NSCam::Utils::LogTool::get()) {
        pTool->getCurrentLogTime(&mNodeEnd[nodeId]);
    }
}

MVOID CaptureFeatureRequest::setBooster(IBoosterPtr boosterPtr)
{
    mBoosterPtr = std::move(boosterPtr);
}

MVOID CaptureFeatureRequest::enableBoost()
{
    if( mBoosterPtr != NULL) {
        mBoosterPtr->enable();
        mEnableBoost = MTRUE;
    }
}

MVOID CaptureFeatureRequest::disableBoost()
{
    if( mBoosterPtr != NULL){
        mBoosterPtr->disable();
        mEnableBoost = MFALSE;
    }
}

MBOOL CaptureFeatureRequest::isBoostEnable()
{
    return mEnableBoost;
}


} // NSCapture
} // NSFeaturePipe
} // NSCamFeature
} // NSCam
