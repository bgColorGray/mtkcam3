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

#include "XNode.h"

#include "NextIOUtil.h"

#define PIPE_CLASS_TAG "XNode"
#define PIPE_TRACE TRACE_X_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_TPI);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

const NextIO::NextID ID_FULL("full");
const NextIO::NextID ID_PREVIEW("preview");
const NextIO::NextID ID_RECORD("record");
const NextIO::NextID ID_TINY("tiny");

XNode::XNode(const char *name)
    : StreamingFeatureNode(name)
    , mMDP(PIPE_CLASS_TAG)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mNextDatas);
    TRACE_FUNC_EXIT();
}

XNode::~XNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL XNode::onData(DataID id, const NextData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_P2G_TO_XNODE ||
        id == ID_BOKEH_TO_XNODE ||
        id == ID_VMDP_TO_XNODE )
    {
        mNextDatas.enque(data);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

NextIO::NextID dummyID(MUINT32 index)
{
    switch(index)
    {
    case 0: return NextIO::NextID("dummy_0");
    case 1: return NextIO::NextID("dummy_1");
    case 2: return NextIO::NextID("dummy_2");
    case 3: return NextIO::NextID("dummy_3");
    default: return NextIO::NextID("dummy_x");
    };
}

MBOOL XNode::checkCropValid(const MRectF &crop, const MSizeF &inSize) const
{
    MBOOL ret = MTRUE;
    if( (crop.s.w > inSize.w) || (crop.s.h > inSize.h) || (crop.p.x > inSize.w) || (crop.p.y > inSize.h) )
    {
        MY_LOGF("Crop Not Valid! crop.p(%f,%f) .s(%fx%f), inSize(%fx%f)", crop.p.x, crop.p.y, crop.s.w, crop.s.h, inSize.w, inSize.h);
        ret = MFALSE;
    }
    return ret;
}


NextIO::Policy XNode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "XNode::getIOPolicy");
    NextIO::Policy policy;

    MBOOL run = property_get_int32("vendor.debug.fpipe.xnode.run", 1);
    MBOOL runSlave = property_get_int32("vendor.debug.fpipe.xnode.runSlave", 1);
    MBOOL tiny = mUseTiny && property_get_int32("vendor.debug.fpipe.xnode.tiny", mUseTiny);

    if( run )
    {
        policy.setUseAdvSetting(MTRUE);
        NextIO::OutInfo previewOut, recordOut;
        previewOut = reqInfo.getOutInfo(NextIO::OUT_PREVIEW);
        recordOut = reqInfo.getOutInfo(NextIO::OUT_RECORD);
        for( auto sensorID : reqInfo.mSensorIDs )
        {
            MBOOL isMaster = (sensorID == reqInfo.mMasterID);
            if( !isMaster && !runSlave )
                continue;
            policy.enableAll(sensorID);
            NextIO::InInfo inInfo = reqInfo.getInInfo(NextIO::IN_RRZO, sensorID);
            MSizeF inSize = inInfo.mSize.size() ? (MSizeF)inInfo.mSize : (MSizeF)reqInfo.getInInfo(NextIO::IN_IMGO, sensorID).mSize;

            if( mUseDivInput ) // do MDP/PQ at p2a
            {
                if( previewOut )
                {
                    NextIO::NextAttr attr;
                    attr.setAttrByPool(mPreviewPool);
                    attr.mPQ = mUseP2APQ ? NextIO::PQ_PREVIEW : NextIO::PQ_NONE;
                    attr.mCrop = isMaster ? previewOut.mCrop : MRectF();
                    attr.mResize = previewOut.mSize;
                    checkCropValid(attr.mCrop, inSize);
                    policy.add(sensorID, ID_PREVIEW, attr, mPreviewPool);
                }
                if( recordOut )
                {
                    NextIO::NextAttr attr;
                    attr.setAttrByPool(mFullPool);
                    attr.mPQ = mUseP2APQ ? NextIO::PQ_RECORD : NextIO::PQ_NONE;
                    attr.mCrop = isMaster ? recordOut.mCrop : MRectF();
                    attr.mResize = recordOut.mSize;
                    checkCropValid(attr.mCrop, inSize);
                    policy.add(sensorID, ID_RECORD, attr, mFullPool);
                }
            }
            else // do MDP/PQ at XNode
            {
                NextIO::NextAttr attr;
                attr.setAttrByPool(mFullPool);
                attr.mReadOnly = MTRUE; // hint img3o (readonly, no crop/resize)
                attr.mResize = MSize(0,0);
                attr.mCrop = MRectF(); // full roi
                policy.add(sensorID, ID_FULL, attr, mFullPool);
            }

            if( tiny )
            {
                NextIO::NextAttr attr;
                attr.setAttrByPool(mTinyPool);
                attr.mHighQuality = MFALSE; // hint img2o
                attr.mResize = MSize(768, 432);
                attr.mCrop = previewOut ? previewOut.mCrop :
                            recordOut ? recordOut.mCrop :
                            MRectF();
                attr.mCrop = isMaster ? attr.mCrop : MRectF();
                checkCropValid(attr.mCrop, inSize);
                policy.add(sensorID, ID_TINY, attr, mTinyPool);
            }

            if( mUseDummy )
            {
                NextIO::NextAttr attr;
                attr.setAttrByPool(mFullPool);
                attr.mResize = MSize(0,0);
                attr.mCrop = MRectF();
                for( MUINT32 i = 0; i < mUseDummy; ++i )
                {
                    policy.add(sensorID, dummyID(i), attr, mFullPool);
                }
            }
        }
    }

    if( mPrint )
    {
        print("xnode_req: ", reqInfo);
        MY_LOGD("run=%d: policy=%s", run, policy.toStr().c_str());
    }

    TRACE_FUNC_EXIT();
    return policy;
}

MBOOL XNode::onInit()
{
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();

    MSize fullSize = mPipeUsage.getStreamingSize();
    MSize previewSize = mPipeUsage.getStreamingSize();
    MSize tinySize(768, 432);

    mPrint = property_get_int32("vendor.debug.fpipe.xnode.print", 0);
    mUseDivInput = property_get_int32("vendor.debug.fpipe.xnode.div", 0);
    mUseP2APQ = property_get_int32("vendor.debug.fpipe.xnode.p2a_pq", 0);
    mUseP2APQ = (mUseDivInput && mPipeUsage.getNumSensor() == 1) ? mUseP2APQ : 0;
    mUseTiny = property_get_int32("vendor.debug.fpipe.xnode.tiny", 0);
    mUseDummy = property_get_int32("vendor.debug.fpipe.xnode.dummy", 0);
    mUseDummy = std::min<MUINT32>(mUseDummy, 4);

    if( mUseTiny )
    {
        MBOOL bokeh = mPipeUsage.supportBokeh();
        MBOOL tpi = mPipeUsage.supportTPI(TPIOEntry::YUV);
        if( bokeh || tpi )
        {
            MY_LOGD("disable tiny: bokeh(%d) TPI(%d) not support", bokeh, tpi);
            mUseTiny = MFALSE;
        }
    }

    MY_LOGD("buffer count(%d), full(%dx%d) preview(%dx%d) tiny(%dx%d) dummy=%d"
            "divIn(%d) p2a_pq(%d) print(%d)",
            mBufferCount, fullSize.w, fullSize.h, previewSize.w, previewSize.h,
            tinySize.w, tinySize.h, mUseDummy, mUseDivInput, mUseP2APQ, mPrint);

    mFullPool = createPool("fpipe.xnode.full", fullSize, eImgFmt_NV21);
    mPreviewPool = createPool("fpipe.xnode.preview", previewSize, eImgFmt_NV21);
    mTinyPool = createPool("fpipe.xnode.tiny", tinySize, eImgFmt_YUY2);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL XNode::onUninit()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL XNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    allocate("fpipe.xnode.full", mFullPool, mBufferCount + mUseDummy);
    allocate("fpipe.xnode.preview", mPreviewPool, mBufferCount);

    if( mUseTiny )
    {
        allocate("fpipe.xnode.tiny", mTinyPool, mBufferCount);
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL XNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    IBufferPool::destroy(mFullPool);
    IBufferPool::destroy(mPreviewPool);
    IBufferPool::destroy(mTinyPool);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL XNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr request;
    NextData data;

    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mNextDatas.deque(data) )
    {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    }
    request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    if( mPrint )
    {
        print("xnode_data", data.mData);
    }

    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    request->mTimer.startXNode();
    TRACE_FUNC("Frame %d in X", request->mRequestNo);
    MY_LOGD("sensor(%d) Frame %d process start",
            mSensorIndex, request->mRequestNo);
    process(request, data.mData);
    request->mTimer.stopXNode();
    MY_LOGD("sensor(%d) Frame %d process done in %d ms",
            mSensorIndex, request->mRequestNo,
            request->mTimer.getElapsedXNode());
    FeaturePipeParam::MSG_TYPE frameDone = FeaturePipeParam::MSG_FRAME_DONE;
    // handleData(ID_XNODE_TO_HELPER, HelperData(HelpReq(dispDone), request));
    handleData(ID_XNODE_TO_HELPER, HelperData(HelpReq(frameDone), request));
    TRACE_FUNC_EXIT();
    return MTRUE;
}

android::sp<IBufferPool> XNode::createPool(const char *name, const MSize &size, EImageFormat fmt) const
{
    TRACE_FUNC_ENTER();
    android::sp<IBufferPool> pool;
    pool = GraphicBufferPool::create(name, size, fmt, GraphicBufferPool::USAGE_HW_TEXTURE);
    TRACE_FUNC_EXIT();
    return pool;
}

MBOOL XNode::process(const RequestPtr &request, const NextResult &data)
{
    TRACE_FUNC_ENTER();
    Inputs in;
    Outputs out;

    getInput(request, data, in);
    getOutput(request, data, out);
    // in.mMasterFull.mImg->syncCache(eCACHECTRL_INVALID); // ready to  CPU touch
    // in.mMasterFull.mImg->syncCache(eCACHECTRL_FLUSH); // ready to HW touch
    simpleCopy(request, in, out);

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL XNode::getInput(const RequestPtr &request, const NextResult &data, Inputs &in)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;

    for( const auto &pair : data.getMasterMap() )
    {
        if( pair.first == ID_FULL )         in.mMasterFull = pair.second;
        else if( pair.first == ID_PREVIEW ) in.mMasterPreview = pair.second;
        else if( pair.first == ID_RECORD )  in.mMasterRecord = pair.second;
        else if( pair.first == ID_TINY )    in.mMasterTiny = pair.second;
        else
        {
            MY_LOGW("unknown buffer(%s)", pair.first.toStr().c_str());
            ret = MFALSE;
        }
    }

    if( !in.mMasterPreview && !in.mMasterRecord && !in.mMasterFull )
    {
        ret = MFALSE;
    }

    if( data.mMap.count(request->mSlaveID) )
    {
        for( const auto &pair : data.mMap.at(request->mSlaveID) )
        {
            if( pair.first == ID_FULL )         in.mSlaveFull = pair.second;
            else if( pair.first == ID_TINY )    in.mSlaveTiny = pair.second;
            else
            {
                MY_LOGW("unknown slavebuffer(%s)", pair.first.toStr().c_str());
                ret = MFALSE;
            }
        }
    }

    print("xnode_in", ID_FULL, in.mMasterFull);
    print("xnode_in", ID_PREVIEW, in.mMasterPreview);
    print("xnode_in", ID_RECORD, in.mMasterRecord);
    print("xnode_in", ID_TINY, in.mMasterTiny);
    print("xnode_in_s", ID_FULL, in.mSlaveFull);
    print("xnode_in_S", ID_TINY, in.mSlaveTiny);

    TRACE_FUNC_EXIT();
    return ret;
}

MVOID XNode::printOut(const Outputs &out) const
{
    print("xnode_out: preview", out.mPreview);
    print("xnode_out: record", out.mRecord);
    for( const auto &e : out.mExtra )
    {
        print("xnode_out: extra", e);
    }
    print("xnode_out: phy1", out.mPhy1);
    print("xnode_out: phy2", out.mPhy2);
}

MBOOL XNode::getOutput(const RequestPtr &request, const NextResult & /*data*/, Outputs &out)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;

    MBOOL hasPreview, hasRecord, hasPhy1, hasPhy2, hasExtra;

    hasPreview = request->popDisplayOutput(this, out.mPreview);
    hasRecord = request->popRecordOutput(this, out.mRecord);
    hasExtra = request->popExtraOutputs(this, out.mExtra);
    hasPhy1 = request->popPhysicalOutput(this, request->mMasterID, out.mPhy1);
    hasPhy2 = request->popPhysicalOutput(this, request->mSlaveID, out.mPhy2);

    if( !out.mPreview && !out.mRecord && !out.mPhy1 && !out.mPhy2 )
    {
        ret = MFALSE;
    }

    if( mPrint )
    {
        printOut(out);
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MVOID setPQCtrl(const NextImg &in, std::vector<P2IO> &out)
{
    for( auto &o : out ) o.mPQCtrl = in.mImg.mPQCtrl;
}

MBOOL XNode::simpleCopy(const RequestPtr & /*request*/, const Inputs &in, const Outputs &out)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;

    MUINT32 fps = 30;
    MUINT32 expectMS = 25;

    if( mUseDivInput )
    {
        if( in.mMasterPreview )
        {
            IImageBuffer *inBuffer = in.mMasterPreview.getImageBufferPtr();
            std::vector<P2IO> mdpOut;
            if( out.mPreview ) mdpOut.push_back(out.mPreview);
            mdpOut.insert(mdpOut.end(), out.mExtra.begin(), out.mExtra.end());
            if( !mUseP2APQ ) setPQCtrl(in.mMasterPreview, mdpOut);
            mMDP.process(fps, inBuffer, mdpOut, mPrint, expectMS/2);
        }

        if( in.mMasterRecord )
        {
            IImageBuffer *inBuffer = in.mMasterRecord.getImageBufferPtr();
            std::vector<P2IO> mdpOut;
            if( out.mRecord ) mdpOut.push_back(out.mRecord);
            if( !mUseP2APQ ) setPQCtrl(in.mMasterRecord, mdpOut);
            mMDP.process(fps, inBuffer, mdpOut, mPrint, expectMS/2);
        }
    }
    else
    {
        if( in.mMasterFull )
        {
            std::vector<P2IO> mdpOut;
            IImageBuffer *inBuffer = in.mMasterFull.getImageBufferPtr();

            if( out.mPreview ) mdpOut.push_back(out.mPreview);
            if( out.mRecord ) mdpOut.push_back(out.mRecord);
            mdpOut.insert(mdpOut.end(), out.mExtra.begin(), out.mExtra.end());

            setPQCtrl(in.mMasterFull, mdpOut);
            mMDP.process(fps, inBuffer, mdpOut, mPrint, expectMS);
        }
    }

    TRACE_FUNC_EXIT();
    return ret;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
