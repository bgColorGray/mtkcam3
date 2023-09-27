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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_G_P2SM_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_G_P2SM_NODE_H_

#include "StreamingFeatureNode.h"
#include "DIPStreamBase.h"

#include <mtkcam3/feature/utils/p2/P2Util.h>
#include <mtkcam/drv/iopipe/PostProc/IHalPostProcPipe.h>
#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam3/feature/utils/p2/DIPStream.h>
#include "p2g/P2GMgr.h"

#define NEED_EXTRA_P2_TUNING 1

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

namespace G {

class P2SMEnqueData : public WatchDogData
{
public:
    RequestPtr mRequest;
    P2SWHolder mSW;
    P2HWHolder mHW;
    MUINT32 mRecordRunCount = 0;
    MUINT32 mDropCount = 0;
};

class P2SMNode : public StreamingFeatureNode
               , public DIPStreamBase<P2SMEnqueData>
               , public P2G::IP2GNode
{
private:
    using P2Pack = NSCam::Feature::P2Util::P2Pack;

    class MainData : public virtual android::RefBase
    {
    public:
        MainData()
        {
            mBatchData = P2G::IO::makeBatchData();
        }

        P2G::POD mP2GBasic;
        IAdvPQCtrl mPQCtrl;
        MUINT32 mReqCount = 1;
        MBOOL mIsRecord = MFALSE;
        MUINT32 mTimestampCode = 1;
        MRectF mRecordCrop;
        P2G::Feature mAddOn;

        P2G::IO::BatchData mBatchData;
        std::vector<MSize> mDSDNSizes;
        P2G::ILazy<DSDNInfo> mDSDNInfo;
        std::vector<MSize> mP2Sizes;
    };

    class SubData
    {
    public:
        SubData() {}
        SubData(const sp<MainData> &main, IImageBuffer *rrzo, IImageBuffer *lcso)
            : mMainData(main), mRRZO(rrzo), mLCSO(lcso)
        {}
    public:
        sp<MainData> mMainData;
        IImageBuffer *mRRZO = NULL;
        IImageBuffer *mLCSO = NULL;
    };

public:
    P2SMNode(const char *name);
    virtual ~P2SMNode();
    MVOID setNormalStream(Feature::P2Util::DIPStream *stream);
    MVOID flush();

public:
    virtual MBOOL onData(DataID id, const RequestPtr &data);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

protected:
    virtual MVOID onDIPStreamBaseCB(const Feature::P2Util::DIPParams &params, const P2SMEnqueData &request);

private:
    MVOID handleResultData(const RequestPtr &request, const P2SMEnqueData &data);
    MVOID handleP2GData(const RequestPtr &request, P2SMEnqueData &data, const P2G::Path &path);
    MBOOL initP2();
    MVOID uninitP2();
    MBOOL initPool(const MSize &fullsize);

    MBOOL processP2SM(const RequestPtr &request);
    MBOOL prepareMainData(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData);
    MBOOL prepareGroupIO(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData, std::vector<P2G::IO> &groupIO);
    MBOOL updateSMVRResult(const RequestPtr &request, const P2SMEnqueData &data);
    MBOOL processP2SW(const RequestPtr &request, const P2G::Path &path);
    MBOOL processP2HW(const RequestPtr &request, P2SMEnqueData &data, const P2G::Path &path);
    P2G::IO makeIO(const RequestPtr &request, const sp<MainData> &mainData, const P2G::Feature &smvrType, P2G::IO::LoopData &loopData);
    MBOOL prepareMainRun(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData, std::vector<P2G::IO> &groupIO);
    MBOOL prepareRecordRun(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData, std::vector<P2G::IO> &groupIO);

    //MBOOL processP2SM(const RequestPtr &request);
    MBOOL prepareDrop(const RequestPtr &request, P2SMEnqueData &data, const sp<MainData> &mainData);
    MUINT32 toRecordTimestampCode(const sp<MainData> &mainData);

    MVOID enqueFeatureStream(Feature::P2Util::DIPParams &params, P2SMEnqueData &data);
    MVOID tryAllocateTuningBuffer();

private:
    using IP2GNode::mP2GMgr;
    WaitQueue<RequestPtr> mRequests;
    Feature::P2Util::DIPStream *mDIPStream = NULL;

    MBOOL mOptimizeFirst = MTRUE;
    std::queue<SubData> mSubQueue;
    P2G::IO::LoopData mMainLoopData;
    P2G::IO::LoopData mRecordLoopData;

    MBOOL mOneP2G = MTRUE;
    MBOOL mNeedDreTun = MFALSE;
    SMVRHal mSMVRHal;
    MUINT32 mSMVRSpeed = 1;
    MUINT32 mExpectMS = 25;
};

} // namespace G

using G_P2SMNode = G::P2SMNode;

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_G_P2SM_NODE_H_
