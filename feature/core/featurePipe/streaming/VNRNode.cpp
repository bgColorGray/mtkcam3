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

#include "VNRNode.h"

#include "NextIOUtil.h"

#define PIPE_CLASS_TAG "VNRNode"
#define PIPE_TRACE TRACE_VNR_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_VNR);

using NSImageio::NSIspio::EPortIndex_IMG3O;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

enum DumpMaskIndex
{
    MASK_VPUO,
};


const std::vector<DumpFilter> sFilterTable =
{
    DumpFilter( MASK_VPUO,      "vpuo",     MTRUE )
};

VNRNode::VNRNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_VNR)
    , StreamingFeatureNode(name)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mDSDNDatas);
    TRACE_FUNC_EXIT();
}

VNRNode::~VNRNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID VNRNode::setInputBufferPool(const android::sp<IBufferPool> &pool, MUINT32 allocate)
{
    (void)allocate;
    TRACE_FUNC_ENTER();
    mInputBufferPool = pool;
    TRACE_FUNC_EXIT();
}

MVOID VNRNode::setOutputBufferPool(const android::sp<IBufferPool> &pool, MUINT32 allocate)
{
    (void)allocate;
    TRACE_FUNC_ENTER();
    mOutputBufferPool = pool;
    TRACE_FUNC_EXIT();
}

MBOOL VNRNode::onData(DataID id, const DSDNData &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_P2G_TO_VNR )
    {
        mDSDNDatas.enque(data);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

NextIO::Policy VNRNode::getIOPolicy(const NextIO::ReqInfo &reqInfo) const
{
    NextIO::Policy policy;
    policy.setNoMDP(MTRUE);
    NextIO::NextAttr attr;
    attr.setAttrByPool(mInputBufferPool);
    attr.mResize = mCustomSize;
    attr.mReadOnly = mUseSharedInput;

    struct table
    {
        MUINT32                 sensorID;
        MBOOL                   run;
    } myTable[] =
    {
        { reqInfo.mMasterID, (reqInfo.hasMaster() && HAS_DSDN20(reqInfo.mFeatureMask)) },
        { reqInfo.mSlaveID,  (reqInfo.hasSlave() && HAS_DSDN20_S(reqInfo.mFeatureMask)) },
    };

    for( const table &t : myTable )
    {
        if( t.run )
        {
            policy.enableAll(t.sensorID);
            policy.add(t.sensorID, NextIO::NORMAL, attr, mInputBufferPool);
        }
    }

    return policy;
}

MBOOL VNRNode::onInit()
{
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();
    enableDumpMask(sFilterTable);
    MBOOL ret = MTRUE;
    mMaxSize = align(mPipeUsage.getStreamingSize(), BUF_ALLOC_ALIGNMENT_BIT);
    mVNR.init(mMaxSize, mPipeUsage.getDSDNParam().isDSDN20Bit10());
    mCustomSize = mPipeUsage.getTPIUsage().getCustomSize(TPIOEntry::YUV, MSize(0, 0));
    mUseSharedInput = mPipeUsage.supportSharedInput();
    mSimulate = VNRHal::isSimulate() ||
                getPropertyValue("vendor.debug.fpipe.vnr.simulate" , 0);
    ret = (mInputBufferPool != NULL && mOutputBufferPool != NULL);

    MY_LOGI("VNR custSize=(%dx%d) simulate=%d",
            mCustomSize.w, mCustomSize.h, mSimulate);
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL VNRNode::onUninit()
{
    TRACE_FUNC_ENTER();
    mVNR.uninit();
    mInputBufferPool = NULL;
    mOutputBufferPool = NULL;
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL VNRNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    MUINT32 base = 3;
    MBOOL isDual = mPipeUsage.getNumSensor() >= 2;

    MUINT32 in = isDual ? base*2*2 : base*2;
    MUINT32 out = isDual ? base*2 : base;
    allocate("fpipe.vnr.in", mInputBufferPool, in);
    allocate("fpipe.vnr.out", mOutputBufferPool, out);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL VNRNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL VNRNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    MBOOL needM = MFALSE, needS = MFALSE;
    MBOOL resultM = MFALSE, resultS = MFALSE;
    BasicImg outM, outS;
    DSDNData data;
    RequestPtr request;
    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mDSDNDatas.deque(data) )
    {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    }
    if( data.mRequest == NULL )
    {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    TRACE_FUNC_ENTER();
    request = data.mRequest;
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    request->mTimer.startVNR();
    markNodeEnter(request);
    MBOOL allowVNR = request->hasGeneralOutput() || mPipeUsage.supportPhysicalDSDN20();
    needM = allowVNR && request->needDSDN20(request->mMasterID);
    needS = allowVNR && request->needDSDN20(request->mSlaveID);
    TRACE_FUNC("Frame %d in VNR master needDSDN20 %d needDSDN20_S %d", request->mRequestNo, needM, needS);

    MUINT32 fps = (needM && needS) ? request->getAppFPS()*2 : request->getAppFPS();
    MY_S_LOGD(request->mLog, "sensor(%d) Frame %d VNR process start need(m/s)=(%d/%d) simulate=%d allow=%d", mSensorIndex, request->mRequestNo, needM, needS, mSimulate, allowVNR);
    struct table
    {
        MBOOL                   run;
        MUINT32                 sensorID;
        const DSDNImg::Pack     &imgs;
        MBOOL                   &res;
        BasicImg                &out;
    } myTable[] =
    {
        { needM, request->mMasterID, data.mData.mPackM, resultM, outM},
        { needS, request->mSlaveID,  data.mData.mPackS, resultS, outS},
    };

    for( const table &t : myTable )
    {
        if( t.run )
        {
            t.out = prepareOut(request, t.imgs, t.sensorID);
            t.res = processVNR(request, t.imgs, t.out, fps, t.sensorID);
            if( t.res )
            {
                dump(request, t.out, t.sensorID);
            }
        }
        if( !t.res )
        {
            t.out = t.imgs.mFullImg;
        }
    }
    MY_S_LOGD(request->mLog, "sensor(%d) Frame %d VNR process done in %d+%d ms, need(m/s)=(%d/%d) processed(m/s)=(%d/%d) fps=%d simulate=%d",
                mSensorIndex, request->mRequestNo, request->mTimer.getElapsedVNREnque(), request->mTimer.getElapsedVNREnque_S(), needM, needS, resultM, resultS, fps, mSimulate);
    markNodeExit(request);
    if( mPipeUsage.supportWarpNode() && !mPipeUsage.supportVMDPNode() )
    {
        if( mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) )  handleData(ID_VNR_TO_NEXT_P, DualBasicImgData(DualBasicImg(outM, outS), request));
        if( mPipeUsage.supportWarpNode(SFN_HINT::RECORD) )   handleData(ID_VNR_TO_NEXT_R, DualBasicImgData(DualBasicImg(outM, outS), request));
    }
    else
    {
        handleData(ID_VNR_TO_NEXT_FULLIMG, DualBasicImgData(DualBasicImg(outM, outS), request));
    }
    request->mTimer.stopVNR();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

BasicImg VNRNode::prepareOut(const RequestPtr &request, const DSDNImg::Pack &data, MUINT32 sensorID)
{
    TRACE_FUNC_ENTER();
    BasicImg out;
    NextIO::NextBuf firstNext;
    if( request->needNextFullImg(this, sensorID) )
    {
        NextIO::NextReq nexts = request->requestNextFullImg(this, sensorID);
        firstNext = getFirst(nexts);
        out.mBuffer = firstNext.mBuffer;
    }
    else
    {
        out.mBuffer = mOutputBufferPool->requestIIBuffer();
    }
    out.setAllInfo(data.mFullImg, firstNext.mAttr.mResize);
    TRACE_FUNC_EXIT();
    return out;
}

MBOOL VNRNode::processVNR(const RequestPtr &request, const DSDNImg::Pack &data, BasicImg &out, MUINT32 fps, MUINT32 sensorID)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( checkInput(request, data, sensorID) )
    {
        out.setAllInfo(data.mFullImg);
        MBOOL isMaster = (sensorID == request->mMasterID);
        isMaster ? request->mTimer.startVNREnque() : request->mTimer.startVNREnque_S();
        ImgBuffer inFull = data.mFullImg.mBuffer;
        ImgBuffer inDS1 = data.mDS1Img.mBuffer;
        ImgBuffer inDS2 = data.mDS2Img;
        ImgBuffer outFull = out.mBuffer;
        ret = mSimulate ? simulate(fps, inFull, inDS1, inDS2, outFull) :
                          mVNR.process(fps, inFull, inDS1, inDS2, outFull);
        isMaster ? request->mTimer.stopVNREnque() : request->mTimer.stopVNREnque_S();
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL VNRNode::checkInput(const RequestPtr &request, const DSDNImg::Pack &data, MUINT32 sensorID)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    if( !data.mFullImg.isValid() ||
        !data.mDS1Img.isValid() ||
        data.mDS2Img == NULL)
    {
        MY_LOGW("Frame %d: sID(%d) invalid input: full=%d ds1=%d ds2=%d",
                request->mRequestNo, sensorID,
                data.mFullImg.isValid(), data.mDS1Img.isValid(), data.mDS2Img != NULL);
        ret = MFALSE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL VNRNode::simulate(MUINT32 fps, const ImgBuffer &inFull, const ImgBuffer &inDS1, const ImgBuffer &inDS2, const ImgBuffer &outFull)
{
    (void)fps;
    (void)inDS1;
    (void)inDS2;
    IImageBuffer *src = inFull->getImageBufferPtr();
    IImageBuffer *dst = outFull->getImageBufferPtr();
    copyImageBuffer(src, dst);
    usleep(7000);
    return MTRUE;
}

MVOID VNRNode::dump(const RequestPtr &request, const BasicImg &out, MUINT32 sensorID) const
{
    TRACE_FUNC_ENTER();
    if( request->needNddDump() || request->needDump() )
    {
        syncCache(out, eCACHECTRL_INVALID);

        if( request->needNddDump() && allowDump(MASK_VPUO) )
        {
            TuningUtils::FILE_DUMP_NAMING_HINT hint = request->mP2Pack.getSensorData(sensorID).mNDDHint;
            dumpNddData(hint, out, TuningUtils::YUV_PORT_VNRO);
        }
        if( request->needDump() && allowDump(MASK_VPUO) )
        {
            dumpData(request, out, "%sdsdn_vnr_out", (sensorID == request->mMasterID) ? "" : "slave");
        }
    }
    TRACE_FUNC_EXIT();
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
