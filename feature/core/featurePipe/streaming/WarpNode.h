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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_WARP_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_WARP_NODE_H_

#include "StreamingFeatureNode.h"
#include "WarpStreamBase.h"
#include "MtkHeader.h"
#include <featurePipe/core/include/JpegEncodeThread.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class WarpEnqueData : public WatchDogData
{
public:
    RequestPtr mRequest;
    ImgBuffer mSrc;
    ImgBuffer mDst;
    ImgBuffer mWarp;
    WarpImg mWarpMap;
    MBOOL mNeedDump = MFALSE;
    SyncCtrlData mSyncCtrlData;
};

/* For Queue record info used */
class WarpSrcInfo
{
public:
    MUINT32 mID = 0;
    MBOOL mValid = MFALSE;
    MBOOL mNeedWarp = MFALSE;
    BasicImg mInput;
    MRectF mCrop;
    Feature::P2Util::P2Pack mP2Pack;
};

class WarpDstInfo
{
public:
    MUINT32 mID = 0;
    MBOOL mNeedTempOut = MFALSE;
    IImageBuffer *mOutputBuffer = NULL;
};

class WarpNode : public virtual StreamingFeatureNode, public virtual WarpStreamBase<WarpEnqueData>
{
public:
    WarpNode(const char *name, SFN_HINT typeHint);
    virtual ~WarpNode();

    MVOID setInputBufferPool(const android::sp<IBufferPool> &pool);
    MVOID setOutputBufferPool(const android::sp<IBufferPool> &pool);
    MVOID clearTSQ();

public:
    virtual MBOOL onData(DataID id, const BasicImgData &data);
    virtual MBOOL onData(DataID id, const DualBasicImgData &data);
    virtual MBOOL onData(DataID id, const WarpData &warpData);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;
    MVOID setSyncCtrlWait(const std::shared_ptr<ISyncCtrl> &syncCtrl);
    MVOID setSyncCtrlSignal(const std::shared_ptr<ISyncCtrl> &syncCtrl);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:
    MVOID setTimerIndex();
    SFN_TYPE::SFN_VALUE getSFNType();
    MVOID tryAllocateExtraInBuffer();
    MVOID processEISNDD(const RequestPtr &request, const WarpParam &param, const WarpSrcInfo &srcInfo);
    MBOOL processWarp(const RequestPtr &request, const WarpImg &warpMap, const WarpSrcInfo &srcInfo, MUINT32 cycleMs, MUINT32 scenarioFPS, const SyncCtrlData &syncCtrlData);
    ImgBuffer prepareOutBuffer(const RequestPtr &request, WarpParam &param);
    MBOOL needTempOutBuffer(const RequestPtr &request);
    //MVOID applyDZ(const RequestPtr &request, WarpParam &param);
    MVOID waitSync(const RequestPtr &request, const SyncCtrlData &syncCtrlData);
    MVOID enqueWarpStream(const WarpParam &param, const WarpEnqueData &data, const SyncCtrlData &syncCtrlData);
    virtual MVOID onWarpStreamBaseCB(const WarpParam &param, const WarpEnqueData &data);
    MVOID handleResultData(const RequestPtr &request);
    MVOID dump(const WarpEnqueData &data, const MDPWrapper::P2IO_OUTPUT_ARRAY &out, MBOOL isPreview) const;
    MVOID dumpAdditionalInfo(android::Printer &printer) const override;

    ImgBuffer prepareOutBuffer(const WarpSrcInfo &src, const WarpDstInfo &dst, const MSizeF &warpOutSize);
    WarpSrcInfo extractSrc(const RequestPtr &request, const BasicImg &inputImg);
    WarpDstInfo prepareDst(const RequestPtr &request);
    WarpParam prepareWarpParam(const RequestPtr &request, const WarpSrcInfo &src, WarpDstInfo &dst,
        const WarpImg &warpMap, WarpEnqueData &data, MUINT32 cycleMs, MUINT32 scenarioFPS, MBOOL bypass = MFALSE);
    MDPWrapper::P2IO_OUTPUT_ARRAY prepareMDPParam(const RequestPtr &request, const WarpSrcInfo &src,
        const WarpDstInfo &dst, const WarpImg &warpMap, WarpEnqueData &data, const DomainTransform &transform);

    MRectF refineMDPCrop(const MRectF &crop, const MSizeF &warpCropSize, const RequestPtr &request);

    static MSize toWarpOutSize(const MSizeF &inSize);
    MUINT32 getMaxCycleTimeMs(const RequestPtr &request);

private:
    SFN_HINT mSFNHint = SFN_HINT::NONE;
    SFN_TYPE mSFNType = SFN_TYPE::BAD;
    MUINT32 mTimerIndex = 0;
    WarpStream *mWarpStream = NULL;
    WaitQueue<BasicImgData> mInputImgDatas;
    WaitQueue<WarpData> mWarpData;
    android::sp<IBufferPool> mInputBufferPool;
    android::sp<IBufferPool> mOutputBufferPool;
    android::sp<JpegEncodeThread> mJpegEncodeThread;

    FeaturePipeParam::MSG_TYPE mDoneMsg = FeaturePipeParam::MSG_INVALID;
    MUINT32 mExtraInBufferNeeded = 0;
    MUINT32 mForceExpectMS = 0;
    MUINT32 mEndTimeDecrease = 0;
    MBOOL mUseSharedInput = MFALSE;
    MUINT32 mDumpEISDisplay = 0;

    std::deque<WarpSrcInfo> mWarpSrcInfoQueue;
    MDPWrapper mMDPWrap;
    const char* mLog = "";
    MUINT32 mFPSBase = 0;
    std::shared_ptr<ISyncCtrl> mSyncCtrlWait;
    std::shared_ptr<ISyncCtrl> mSyncCtrlSignal;
    MUINT32 mWaitFrame = 0;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_WARP_NODE_H_
