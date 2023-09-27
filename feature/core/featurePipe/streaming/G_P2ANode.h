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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_G_P2A_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_G_P2A_NODE_H_

#include <unordered_map>
#include <mtkcam/aaa/IHal3A.h>

#include "StreamingFeatureNode.h"
#include "DIPStreamBase.h"
#include "p2g/P2GMgr.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

namespace G {

class P2AEnqueData : public WatchDogData
{
public:
    P2AEnqueData() {}
    P2AEnqueData(const RequestPtr &request) : mRequest(request) {}
public:
    RequestPtr        mRequest;
    P2SWHolder   mSW;
    P2HWHolder   mHW;
    std::vector<MSize> mDSDNSizes;
    std::vector<MSize> mSlaveDSDNSizes;
};

class P2ANode : public StreamingFeatureNode
              , public DIPStreamBase<P2AEnqueData>
              , public P2G::IP2GNode
{
public:
    using LazyImg = P2G::ILazy<P2G::GImg>;

    class IOOut
    {
    public:
        IAdvPQCtrl mPQCtrl;
        LazyImg mFull;
        std::vector<LazyImg> mNext;
        LazyImg mTiny;
        LazyImg mDS;
        BasicImg mDSBasicImg;
        LazyImg mDN;
        LazyImg mOMC;
    };

    class IOIn
    {
    public:
        MSize mConfSize;
        MSize mIdiSize;
    };

    class RunCache
    {
    public:
        MBOOL mRun = MFALSE;
        RunCache(){}
        RunCache(MBOOL run)
        : mRun(run) {}
    };

public:
    P2ANode(const char *name);
    virtual ~P2ANode();

    MVOID setDIPStream(Feature::P2Util::DIPStream *stream);

    MVOID setFullImgPool(const android::sp<IBufferPool> &pool, MUINT32 allocate = 0);

public:
    virtual MBOOL onData(DataID id, const RequestPtr &data);
    virtual MBOOL onData(DataID id, const RSCData &data);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

protected:
    virtual MVOID onDIPStreamBaseCB(const Feature::P2Util::DIPParams &dipParams, const P2AEnqueData &request);

private:
    MVOID handleResultData(const RequestPtr &request, const P2AEnqueData &data);
    MVOID handleP2GData(const RequestPtr &request, P2AEnqueData &data, const P2G::Path &path);

    MBOOL processP2A(const RequestPtr &request, const RSCResult &rsc);
    MVOID prepareFeatureData(const RequestPtr &request, P2AEnqueData &data, IOIn &inM, IOIn &inS, IOOut &outM, IOOut &outS, MBOOL runSlave);
    MVOID fillDsdnData(const P2G::P2GMgr::DsdnQueryOut &dsdnOut, MBOOL isMaster, P2AEnqueData &data, IOIn &ioIn, IOOut &ioOut);
    MBOOL prepareFull(const RequestPtr &request, MBOOL run, MUINT32 sensorID, P2GOut &p2gOut, IOOut &ioOut);
    MBOOL prepareDS(const RequestPtr &request, MUINT32 sensorID, const std::vector<MSize> &dsSize, P2GOut &p2gOut, IOOut &ioOut);
    MBOOL prepareGroupIO(const RequestPtr &request, const P2AEnqueData &data, const IOIn &ioInM, const IOIn &ioInS, const IOOut &outM, const IOOut &outS, std::vector<P2G::IO> &groupIO);

    MVOID updateMotion(const RequestPtr &request, const P2AEnqueData &data, const RSCResult &rsc, MBOOL runSlave = MFALSE);
    MBOOL runP2SW(const RequestPtr &request, const std::vector<P2G::P2SW> &p2sw);
    MBOOL runP2HW(const RequestPtr &request, P2AEnqueData &data, const std::vector<P2G::P2HW> &p2hw, const std::vector<P2G::PMDP> &pmdp);

    MVOID requestFull(const RequestPtr &request, MUINT32 sensorID, const IAdvPQCtrl &pqCtrl, P2GOut &p2gOut, IOOut &ioOut);
    MVOID updateDsInfo(const RequestPtr &request, MUINT32 sensorID, BasicImg &ds, const MRectF &crop, const MSize &size) const;
    MRectF getNextCrop(const RequestPtr &request, IOOut &ioOut) const;
    MUINT32 getNextDmaConstrain(const IOOut &ioOut) const;

    P2G::IO makeIO(const RequestPtr &request, const P2AEnqueData &data, P2G::Scene scene, MUINT32 sensorID, MUINT32 FDSensorID, const SFPIOMap &iomap, IMetadata *exAppOut, MUINT32 outMask, const IOIn &ioIn, const IOOut &ioOut, const TunBuffer &syncTun, const P2G::Feature &feature);
    MBOOL fillOut(const RequestPtr &request, MUINT32 sensorID, MUINT32 outMask, const IOOut &ioOut, P2G::IO::OutData &out);
    MUINT32 getExpectMS(const RequestPtr &request) const;

    MVOID enqueFeatureStream(Feature::P2Util::DIPParams &params, P2AEnqueData &data);

    MBOOL init3A();
    MVOID uninit3A();
    MBOOL needNormalYuv(MUINT32 sensorID, const RequestPtr &request);

private:
    WaitQueue<RequestPtr> mRequests;
    WaitQueue<RSCData> mRSCDatas;
    NS3Av3::IHal3A *mp3A = NULL;
    Feature::P2Util::DIPStream *mDIPStream = NULL;

    MUINT32   mEisMode = EIS_MODE_OFF;
    MUINT32   mForceExpectMS = 0;
    MBOOL     mOneP2G = MTRUE;
    MBOOL     mWaitRSC = MFALSE;
    MBOOL     mUseLMV = MFALSE;
    MBOOL     mNeedDreTun = MFALSE;
    MBOOL     mSupportPhyNR = MFALSE;
    MBOOL     mSupportPhyDSDN20 = MFALSE;

private:
    static constexpr P2G::Scene GM = P2G::Scene::GM;
    static constexpr P2G::Scene GS = P2G::Scene::GS;
    static constexpr P2G::Scene PM = P2G::Scene::PM;
    static constexpr P2G::Scene PS = P2G::Scene::PS;
    static constexpr P2G::Scene LM = P2G::Scene::LM;
    static constexpr P2G::Scene LS = P2G::Scene::LS;
    enum OutMask
    {
        O_DISP      = 1<<0,
        O_REC       = 1<<1,
        O_FD        = 1<<2,
        O_EXTRA     = 1<<3,
        O_PHY       = 1<<4,
        O_LARGE     = 1<<5,
        O_FULL      = 1<<6,
        O_DS        = 1<<7,
        O_OMC       = 1<<8,
    };

private:
    using IP2GNode::mP2GMgr;
    /* search loop data by mLoopData[LoopScene][sensorID] */
    std::unordered_map<P2G::LoopScene, std::unordered_map<MUINT32, P2G::IO::LoopData>> mLoopData;
    std::unordered_map<MUINT32, RunCache> mRunCache;
};

} // namespace G

using G_P2ANode = G::P2ANode;

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_NODE_H_
