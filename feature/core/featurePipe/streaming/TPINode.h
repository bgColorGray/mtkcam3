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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_TPI_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_TPI_NODE_H_

#include "StreamingFeatureNode.h"
#include <queue>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class TPINode : public StreamingFeatureNode
{
public:
    TPINode(const char* name, MUINT32 index);
    virtual ~TPINode();
    MVOID setTPIMgr(TPIMgr *mgr);
    MVOID setSharedBufferPool(const sp<SharedBufferPool> &pool);

public:
    virtual MBOOL onData(DataID id, const BasicImgData&);
    virtual MBOOL onData(DataID id, const DualBasicImgData&);
    virtual MBOOL onData(DataID id, const DepthImgData&);
    virtual MBOOL onData(DataID id, const TPIData&);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:

class TPIQData
{
public:
    MBOOL                   mIsValid = MFALSE;
    MBOOL                   mPushOnly = MFALSE;

    RequestPtr              mRequest;
    BasicImg                mInput;
    MRectF                  mRecordCrop;

public:
    TPIQData()
    {}
    TPIQData(const RequestPtr &request, const BasicImg &in, const MRectF &crop)
    : mIsValid(MTRUE)
    , mRequest(request)
    , mInput(in)
    , mRecordCrop(crop)
    {}
    ~TPIQData()
    {}
};

private:
    MBOOL initSettings();
    MBOOL initPlugin();
    TPI_ConfigInfo getTPIConfig() const;
    MBOOL prepareBufferPool();
    MBOOL uninitPlugin();

    MBOOL initInRes(const RequestPtr &request, TPIRes &in);
    MBOOL prepareInRes(const RequestPtr &request, TPIRes &in);
    MBOOL prepareOutRes(const RequestPtr &request, const TPIRes &in, TPIRes &out);
    MBOOL updateBufferOwner(const RequestPtr &request, TPIRes::BasicImgMap &map);
    MBOOL process(const RequestPtr &request, const TPIRes &in, TPIRes &out, TPIQData &outQData);
    MVOID handleBypass(const RequestPtr &request, const TPIRes &in, TPIRes &out);
    MVOID dumpLog(const RequestPtr &request, const TPIRes &in, const TPIRes &out, TPI_Meta meta[], unsigned metaCount, TPI_Image img[], unsigned imgCount);
    MVOID dumpInBuffer(const RequestPtr &request, const TPIRes &in);
    MVOID dumpOutBuffer(const RequestPtr &request, const TPIRes &out);
    MVOID flushQueue();
    MVOID handleFlushDummyData(const RequestPtr &recReq);
    MVOID handleFlushQueueData(const TPIQData &qData);
    MVOID handleResultData(const RequestPtr &gnlReq, const TPIRes &out, const TPIQData &outQData);
    MVOID appendDepthMetaResult(TPIData &data, DepthImgData depthData);

    MVOID filterResult(const RequestPtr &request, TPI_Image img[], unsigned imgCount, TPI_Data &result);
    MVOID processRecData(const RequestPtr &request, const BasicImg &input, const TPI_Data &tpiData, TPIQData &outQData, TPIRes &out);
    MVOID processImgSrc(const RequestPtr &request, TPI_Image img[], unsigned imgCount, TPIRes &out);
    MBOOL needOutputRecord(const RequestPtr &request);
    MBOOL toRecReq(VMDPReqData &outRecReq, RequestPtr &recReq, const BasicImg &img, const TPIQData &recData, const RequestPtr &request);
    MVOID updateTSQInfo(TSQRecInfo &recInfo, const TPIQData &qData);
    MVOID updateRQQInfo(RequestPtr &recReq,  const TPIQData &qData);

private:
    MUINT32 mTPINodeIndex = 0;
    MUINT32 mPoolID = 0;
    TPIMgr *mTPIMgr = NULL;
    TPIUsage mTPIUsage;
    TPIO mTPINodeIO;

    MBOOL mIsFirstTPI = MFALSE;
    MBOOL mIsLastTPI = MFALSE;
    MBOOL mIsInplace = MFALSE;
    MBOOL mIsInputCrop = MFALSE;
    MBOOL mIsDivNode = MFALSE;
    MBOOL mIsDivPath = MFALSE;
    MBOOL mUseDivDisplay = MFALSE;
    MUINT32 mQueueCount = 0;
    MUINT32 mDelayCount = 0;
    MBOOL mWaitDepthData = MFALSE;
    MBOOL mForceErase = MFALSE;
    TPI_BufferInfo mInputInfo;
    MSize mCustomSize;
    MBOOL mUseSharedInput = MFALSE;

    WaitQueue<TPIData> mData;
    WaitQueue<DepthImgData> mDepthDatas;
    WaitQueue<BasicImgData> mCleanYuv;

    sp<SharedBufferPool> mSharedBufferPool;
    queue<TPIQData> mTPIQueue;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_TPI_NODE_H_
