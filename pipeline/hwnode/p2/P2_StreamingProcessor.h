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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#ifndef _MTKCAM_HWNODE_P2_STREAMING_PROCESSOR_H_
#define _MTKCAM_HWNODE_P2_STREAMING_PROCESSOR_H_

#include <set>

#include <mtkcam3/feature/featurePipe/IStreamingFeaturePipe.h>
#include <mtkcam3/feature/3dnr/util_3dnr.h>

#include "P2_Processor.h"
#include "P2_Util.h"
#include "P2_SMVRQueue.h"

#include <mtkcam/utils/hw/IFDContainer.h>
#include <mtkcam3/feature/utils/FeatureCache.h>
using NSCam::IFDContainer;

using NSCam::NSCamFeature::NSFeaturePipe::FeaturePipeParam;
using NSCam::NSCamFeature::NSFeaturePipe::IStreamingFeaturePipe;

namespace P2
{

class RSSOMgr
{
public:
    enum Policy { POLICY_NO_CACHE, POLICY_COPY_CACHE };

public:
    class Cache
    {
    public:
        Cache();
        Cache(const sp<P2Img> &rsso);
        Cache(const sp<IImageBuffer> &cache);
        IImageBuffer* getIImageBufferPtr();
    public:
        sp<P2Img> mPrev;
        sp<IImageBuffer> mCache;
    };

public:
    RSSOMgr();
    ~RSSOMgr();
    MVOID init(const std::vector<MUINT32> &sensorIDs);
    MVOID uninit();
    Cache updateRSSO(const ILog &log, const sp<P2Img> &rsso, MUINT32 sensorID);
    MVOID releaseRSSO(const ILog &log, Policy policy);
    MVOID releaseUnusedRssoCache(const ILog &log, const std::set<MUINT32/*sID*/>& curr);

private:
    android::Mutex mMutex;
    MBOOL mReady = MFALSE;
    std::unordered_map<MUINT32, Cache> mHolder;
    std::unordered_map<MUINT32, Pool> mCachePool;
};

class StreamingProcessor : virtual public Processor<P2InitParam, P2ConfigParam, vector<sp<P2Request>>>
{
    enum class ERequestPath
    {
        eGeneral,
        ePhysic,
        eLarge
    };

    struct EisAeInfo {
      MINT32 expTime = 0;
      MINT32 longExpTime = 0;
      MINT32 gain = 0;
      MINT32 fusionBaseFrame = 0;
    };

    class Payload;
    class PartialPayload;
    class P2RequestPack;

public:
    StreamingProcessor();
    virtual ~StreamingProcessor();

protected:
    virtual MBOOL onInit(const P2InitParam &param);
    virtual MVOID onUninit();
    virtual MVOID onThreadStart();
    virtual MVOID onThreadStop();
    virtual MBOOL onConfig(const P2ConfigParam &param);
    virtual MBOOL onEnque(const vector<sp<P2Request>> &requests);
    virtual MVOID onNotifyFlush();
    virtual MVOID onWaitFlush();
    virtual MVOID onIdle();

private:
    IStreamingFeaturePipe::UsageHint getFeatureUsageHint(const P2ConfigInfo &config);
    MBOOL needReConfig(const P2ConfigInfo &oldConfig, const P2ConfigInfo &newConfig);
    MBOOL initFeaturePipe(const P2ConfigInfo &config);
    MVOID uninitFeaturePipe();
    MBOOL init3A();
    MVOID uninit3A();
    MVOID waitFeaturePipeDone();

    MVOID incPayloadCount(const ILog &log);
    MVOID decPayloadCount(const ILog &log);

    MVOID incPayload(const sp<Payload> &payload);
    MBOOL decPayload(FeaturePipeParam &param, const sp<Payload> &payload, MBOOL checkOrder);

    MBOOL prepareISPTuning(P2Util::SimpleIn& input, const ILog &log) const;
    MBOOL processP2(const sp<Payload>& payload);
    MBOOL checkFeaturePipeParamValid(const sp<Payload> &payload);
    MVOID onFPipeCB(FeaturePipeParam::MSG_TYPE msg, FeaturePipeParam &param, const sp<Payload> &payload);
    static MBOOL sFPipeCB(FeaturePipeParam::MSG_TYPE msg, FeaturePipeParam &param);
    MBOOL makeRequestPacks(const vector<sp<P2Request>> &requests, vector<sp<P2RequestPack>>& rReqPacks) const;
    sp<Payload> makePayLoad(const vector<sp<P2Request>> &requests, const vector<sp<P2RequestPack>>& reqPacks) const;
    MBOOL prepareInputs(const sp<Payload>& payload);
    MBOOL prepareOutputs(const sp<Payload>& payload) const;
    MBOOL prepareBasicPQ(const sp<Payload>& payload) const;
    MVOID releaseResource(const vector<sp<P2Request>> &requests, MUINT32 res) const;
    MVOID makeSFPIOOuts(const sp<Payload>& payload, const ERequestPath& path, FeaturePipeParam& featureParam) const;
    MBOOL makeSFPIOMgr(const sp<Payload>& payload, FeaturePipeParam& featureParam) const;

    MBOOL isRequestValid(const sp<P2Request>& request) const;
    MBOOL isRequestValid_VSDOF(const sp<P2Request>& request) const;

private:
    // Features
    MVOID prepareFeatureParam(P2Util::SimpleIn& input, const ILog &log);
    MBOOL prepareCommon(P2Util::SimpleIn& input, const ILog &log) const;
    // EIS
    MBOOL isEIS12() const;
    MBOOL isAdvEIS() const;
    MBOOL isAdvEIS(MUINT32 eisMode) const;
    MRectF getEISRRZOMargin(P2Util::SimpleIn& input) const;
    MBOOL prepareEISVar(P2Util::SimpleIn& input, FeaturePipeParam &featureParam, const sp<P2Request> &request,
        const sp<Cropper> &cropper, const MRectF &eisMargin) const;
    MBOOL getEISExpTime(MINT32 &expTime, MINT32 &longExpTime, const LMVInfo &lmvInfo, const sp<P2Meta> &inHal) const;
    MVOID setLMVOParam(FeaturePipeParam &featureParam, const sp<P2Request> &request) const;
    MBOOL prepareEISMask(FeaturePipeParam &featureParam) const;
    MBOOL prepareEIS(P2Util::SimpleIn& input, const ILog &log) const;
    MINT64 getVRtimestamp(const sp<Payload> &payload, const FeaturePipeParam &param);
    MVOID processTSQ(const sp<P2Request> &request, MINT64 ts, MBOOL skipRecord);
    MVOID processVRQ(const FeaturePipeParam &param, const sp<Payload> &payload);
    MVOID calcEISRatio(const MSize &streamingSize, const std::vector<P2AppStreamInfo> &streamInfo);
    MBOOL getEisAeInfo(const P2Info& p2Info, EisAeInfo& aeInfo, const sp<P2Meta>& inHal,
        const sp<P2Meta>& inApp) const;
    // 3DNR
    MVOID init3DNR();
    MVOID uninit3DNR();
    MINT32 get3DNRIsoThreshold(MUINT32 sensorID, MUINT8 ispProfile) const;
    MBOOL prepare3DNR_FeatureData(
        MBOOL en3DNRFlow, MBOOL isEIS4K, MBOOL isIMGO,
        P2Util::SimpleIn& input, MUINT8 ispProfile, const ILog &log,
        MBOOL isRscNeeded, MBOOL en3DNRDSDNFlow
        ) const;
    MVOID prepareRSC(P2Util::SimpleIn& input, const ILog &log) const;
    MBOOL prepare3DNR_FeatureData(P2Util::SimpleIn& input, const ILog &log) const;
    MBOOL getInputCrop3DNR(
        MBOOL &isEIS4K, MBOOL &isIMGO, MRect &inputCrop,
        P2Util::SimpleIn& input, const ILog &log) const;
    MBOOL is3DNRFlowEnabled(P2Util::SimpleIn& input, const ILog &log);
    MBOOL is3DNRDsdnFlowEnabled(P2Util::SimpleIn& input, const ILog &log);
    MBOOL prepare3DNR(P2Util::SimpleIn& input, const ILog &log);
    // FSC
    MBOOL prepareFSC(P2Util::SimpleIn& input, const ILog &log) const;
    MBOOL isSubPixelEnabled() const;
    MVOID drawCropRegion(const sp<Payload>& payload) const;
    MVOID drawLine(const sp<P2Img> &img) const;
    MVOID drawP2InputLine(const sp<Payload> &payload) const;
    MVOID drawP2OutputLine(const sp<Payload> &payload) const;
    // RSSO
    MBOOL isNeedRSSO(P2Util::SimpleIn& input) const;
    MBOOL prepareRSSO(const sp<Payload>& payload);
    // 3rd party
    MBOOL prepareTP(P2Util::SimpleIn& input, const ILog &log) const;

    MBOOL prepareSMVR(const sp<P2RequestPack> &reqPack, const ILog &log);
    MBOOL processSMVRQ(const FeaturePipeParam &param, const sp<Payload> &payload);
    MVOID flushSMVRQ();
    MBOOL isBatchSMVR() const;
    MUINT32 getSMVRCount(const ILog &log, const sp<P2RequestPack> &reqPack);
    MUINT32 getSMVRFPS(const ILog &log, const sp<P2Meta> &inHal) const;
    MVOID prepareSMVRQueue(const ILog &log, const P2Util::SimpleIn &in, MUINT32 reqCount);
    MVOID prepareSMVRVar(const ILog &log, FeaturePipeParam &featureParam, MUINT32 reqCount) const;
    MVOID releaseSMVRQueue(MUINT32 run, MUINT32 drop);
    MVOID updateSMVRTimestamp(const sp<Payload> &payload, MUINT32 run);
    MVOID updateSMVRMeta(const sp<Payload> &payload, MUINT32 queueCount);

    // DSDN
    MVOID initDSDN(const P2ConfigInfo &config);
    MVOID uninitDSDN();
    NSCam::NSCamFeature::DSDNParam queryDSDNParam(const P2ConfigInfo &config);
    MBOOL prepareDSDN(P2Util::SimpleIn &input, const ILog &log);
    NSCam::Feature::DSDNCache::Data prepareDSDNCache(const ILog &log, MBOOL targetRun, const P2PlatInfo::NVRamDSDN &nvram);
    MVOID readbackP2Input(const NSCam::TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
        const MINT32 &magic3A,
        const std::string &key, IImageBuffer *out_pImgBuf) const;

private:
    // P2Request that can be process together will be merged together
    class P2RequestPack : virtual public android::RefBase
    {
    public:
        P2RequestPack() = delete;
        P2RequestPack(const ILog &log, const sp<P2Request>& pReq, const vector<MUINT32>& sensorIDs);
        virtual ~P2RequestPack();

        MVOID addOutput(const sp<P2Request>& pReq, const MUINT32 outputIndex);
        MVOID updateBufferResult(MBOOL result);
        MVOID updateMetaResult(MBOOL result);
        MVOID dropRecord();
        MVOID updateVRTimestamp(MUINT32 run);
        MINT64 getVRTimestamp();
        MVOID earlyRelease(MUINT32 mask);
        MBOOL contains(const sp<P2Request>& pReq) const;
        P2Util::SimpleIn* getInput(MUINT32 sensorID);

        MUINT32 getNumDisplay() const;
        MUINT32 getNumRecord() const;
    public:
        ILog mLog;
        sp<P2Request> mMainRequest;
        set<sp<P2Request>> mRequests;
        unordered_map<MUINT32, MUINT32>  mSensorInputMap;
        vector<P2Util::SimpleIn>  mInputs;
        unordered_map<P2Request*, vector<P2Util::SimpleOut>> mOutputs;

    private:
        MUINT32 getFDSensorID(const ILog &log, const sp<P2Request> &pReq, const vector<MUINT32> &sensorIDs, MUINT32 defaultSID) const;
    };

    class PartialPayload : virtual public android::RefBase
    {
    public:
        PartialPayload(const ILog &mainLog, const sp<P2RequestPack> &pReqPack);
        virtual ~PartialPayload();
        MVOID print() const;

    public:
        sp<P2RequestPack> mRequestPack;
        ILog mLog;
    };

    class Payload : virtual public android::RefBase
    {
    public:
        Payload(StreamingProcessor* parent, const ILog &mainLog, MUINT32 masterSensorId, MBOOL fdPrint);
        MVOID addRequests(const vector<sp<P2Request>> &requests);
        MVOID addRequestPacks(const vector<sp<P2RequestPack>>& reqPacks);
        MBOOL prepareFdData(const P2Info &p2Info, IFDContainer *pFDContainer);

        MVOID print() const;
        sp<P2Request> getMainRequest();
        sp<P2Request> getAnyRequest();
        sp<P2Request> getPathRequest(ERequestPath path, const MUINT32& sensorID);
        sp<P2RequestPack> getRequestPack(const sp<P2Request> &pReq);

        // After all the operations of streaming processor,
        // chose one main FPP and create SFPIOMgr which will be used as SFP input.
        FeaturePipeParam* getMainFeaturePipeParam();
    private:
        virtual ~Payload();

    public:
        StreamingProcessor* mParent = nullptr;
        const ILog mLog;
        const MUINT32 mMasterID = 0;

        // 1 PartialPayload for 1 P2RequestPack
        vector<sp<PartialPayload>> mPartialPayloads;

        // path to <sensorID, P2Request> mapping, for SFPIO
        unordered_map<
            ERequestPath,
            unordered_map<MUINT32,sp<P2Request>>> mReqPaths;

        MBOOL mFDPrint = MFALSE;
        std::shared_ptr<FD_DATATYPE> mFDdata; // for PQParam
        std::set<MUINT32> mSensorIDs;
    };

private:
    ILog mLog;

    P2Info mP2Info;
    MBOOL mFDPrint = MFALSE;

    IStreamingFeaturePipe *mFeaturePipe = nullptr;
    IStreamingFeaturePipe::UsageHint mPipeUsageHint;

    std::unordered_map<MUINT32, IHal3A*> mHal3AMap; // sensorID -> IHal3A
    std::unordered_map<MUINT32, NS3Av3::IHalISP*> mHalISPMap; // sensorID -> IHal3A
private:
    std::atomic<MUINT32> mPayloadCount = {0};
    android::Mutex mPayloadMutex;
    android::Condition mPayloadCondition;
    std::list<sp<Payload>> mPayloadList;

    bool mBypssSFP = false;

    // for 3DNR
    MINT32 m3dnrEnable = 0;
    MINT32 m3dnrDebugEnable = 0;
    MINT32 m3dnrLogLevel = 0;
    MBOOL mIs3dnrEnabled_p2a = MFALSE;
    MBOOL mIs3dnrEnabled_dsdn = MFALSE;
    std::unordered_map<MUINT32, sp<Util3dnr>> mUtil3dnrMap; // sensorID -> Util3dnr
    MUINT8 mIspProfile = 0;
    MINT32 prevMasterID = -1;
    MINT32 prevSlaveID = -1;

    // for RSC
    RSSOMgr mRssoMgr;

    // for FSC
    MUINT32 mDebugDrawCropMask;

    // for draw line
    MUINT32 mDrawType = 0;
    MUINT32 mDrawDefaultPosition = 0;
    MUINT32 mDrawMoveSpeed = 0;
    MUINT32 mDrawLineWidth = 0;
    MUINT32 mDrawValue = 0;
    // for p2 readback
    MUINT32 mReadbackMask = 0;

    // for DRE
    sp<IFDContainer> mspFDContainer;

    SMVRQueue mSMVRQueue;
    MBOOL mSMVROverBurst = MFALSE;
    float mEisWidthRatio = 0;
    float mEisHeightRatio = 0;
    MINT64 mLastP1timestamp = 0;
    MBOOL mUseGrallocTSQ = MFALSE;
    MINT32 mFusionBaseFrame = 0;

    map<MUINT32, MBOOL> mDSDNTargetState;
    map<MUINT32, MBOOL> mDSDNState;
    MINT32 mDSDNDebugISO_H = 0;
    MINT32 mDSDNDebugP1Ctrl = 0;
    MINT32 mDSDNDebugP1DsRatio = 0;
    MINT32 mDSDNDebugSlave = 0;
    MUINT32 mDSDNDebugFlag = 0;
};

} // namespace P2

#endif // _MTKCAM_HWNODE_P2_STREAMING_PROCESSOR_H_
