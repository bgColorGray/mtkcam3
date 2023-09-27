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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_DATA_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_DATA_H_

#include <unordered_map>
#include "MtkHeader.h"
//#include <mtkcam3/feature/featurePipe/IStreamingFeaturePipe.h>
//#include <mtkcam/common/faces.h>
//#include <mtkcam3/featureio/eis_type.h>
#include <camera_custom_eis.h>

#include <utils/RefBase.h>
#include <utils/Vector.h>

#include <featurePipe/core/include/CamNodeULog.h>
#include <featurePipe/core/include/WaitQueue.h>
#include <featurePipe/core/include/IIBuffer.h>
#include <featurePipe/core/include/TuningBufferPool.h>

#include "StreamingFeatureTimer.h"
#include "StreamingFeaturePipeUsage.h"

#include "OutputControl.h"
#include "EISQControl.h"
#include "MDPWrapper.h"
#include "SyncCtrl.h"

#include "tpi/TPIMgr.h"

#include <featurePipe/core/include/CamNodeULog.h>

#include <mtkcam3/feature/utils/p2/DIPStream.h>

#include "StreamingFeatureType.h"
#include "p2g/IP2GNode.h"

#define SFP_GET_REQ_LOGSTR(req) (req == NULL) ? "" : req->mLog.getLogStr()
#define P2_CAM_TRACE_REQ(lv,req) P2_CAM_TRACE_INFO(lv, SFP_GET_REQ_LOGSTR(req))
#define SFPWatchdog Utils::ULog::ULogTimeBombHandle

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class StreamingFeatureNode;

class StreamingFeatureRequest : public virtual android::RefBase
{
private:
    // must allocate extParam before everything else
    FeaturePipeParam mExtParam;
public:
    // alias members, do not change initialize order
    const StreamingFeaturePipeUsage &mPipeUsage;
    VarMap<SFP_VAR> &mVarMap;
    const Feature::P2Util::P2Pack &mP2Pack;
    Feature::ILog mLog;
    SFPIOManager &mSFPIOManager;
    MUINT32 mSlaveID = INVALID_SENSOR;
    MUINT32 mMasterID = INVALID_SENSOR;
    MUINT32 mFDSensorID = INVALID_SENSOR;
    std::unordered_map<MUINT32, NextIO::Request> mIORequestMap;
    NextIO::Request &mMasterIOReq;

    MUINT32 mFeatureMask;
    const MUINT32 mRequestNo;
    const MUINT32 mRecordNo;
    const MUINT32 mMWFrameNo;
    IStreamingFeaturePipe::eAppMode mAppMode;
    StreamingFeatureTimer mTimer;
    //NSCam::NSIoPipe::QParams mP2A_QParams;
    //NSCam::NSIoPipe::QParams mN3D_P2Node_QParams;
    MINT32 mDebugDump;
    Feature::P2Util::P2DumpType mP2DumpType = Feature::P2Util::P2_DUMP_NONE;;
    HelperRWData mHelperNodeData; // NOTE can only access by helper node thread
    MBOOL mDispNodeData = MTRUE;
    TPIOFrame mTPIFrame;
    std::unordered_map<IO_TYPE, OutputInfo> mOutputInfoMap;

public:
    StreamingFeatureRequest(const StreamingFeaturePipeUsage &pipeUsage, const FeaturePipeParam &extParam, MUINT32 requestNo, MUINT32 recordNo, const EISQState &eisQ);
    ~StreamingFeatureRequest();

    MVOID prepareOutputInfo();
    MVOID setDisplayFPSCounter(FPSCounter *counter);
    MVOID setFrameFPSCounter(FPSCounter *counter);
    MVOID calSizeInfo();
    NextIO::ReqInfo extractReqInfo() const;
    MVOID checkBufferHoldByReq() const;

    MBOOL updateResult(MBOOL result);
    MBOOL doExtCallback(FeaturePipeParam::MSG_TYPE msg);

    IPQCtrl_const getBasicPQ(MUINT32 sensorID) const;

    IImageBuffer* getMasterInputBuffer();
    IImageBuffer* getDisplayOutputBuffer();
    IImageBuffer* getRecordOutputBuffer();

    MSize getMasterInputSize();
    MSize getDisplaySize() const;
    MSize getFDSize() const;
    MRectF getRecordCrop() const;
    MRectF getZoomROI() const;

    MBOOL popDisplayOutput(const StreamingFeatureNode *node, NSCam::Feature::P2Util::P2IO &output);
    MBOOL popRecordOutput(const StreamingFeatureNode *node, NSCam::Feature::P2Util::P2IO &output);
    MBOOL popRecordOutputs(const StreamingFeatureNode *node, std::vector<NSCam::Feature::P2Util::P2IO> &outList);
    MBOOL popExtraOutputs(const StreamingFeatureNode *node, std::vector<NSCam::Feature::P2Util::P2IO> &outList);
    MBOOL popPhysicalOutput(const StreamingFeatureNode *node, MUINT32 sensorID, NSCam::Feature::P2Util::P2IO &output);
    MBOOL popLargeOutputs(const StreamingFeatureNode *node, MUINT32 sensorID, std::vector<NSCam::Feature::P2Util::P2IO> &outList);
    MBOOL popFDOutput(const StreamingFeatureNode *node, NSCam::Feature::P2Util::P2IO &output);
    MBOOL popAppDepthOutput(const StreamingFeatureNode *node, NSCam::Feature::P2Util::P2IO &output);
    MBOOL getOutputInfo(IO_TYPE ioType, OutputInfo &bufInfo) const;
    EISSourceDumpInfo getEISDumpInfo();

    ImgBuffer popAsyncImg(const StreamingFeatureNode *node);
    ImgBuffer getAsyncImg(const StreamingFeatureNode *node) const;
    MVOID     clearAsyncImg(const StreamingFeatureNode *node);
    NextIO::NextReq requestNextFullImg(const StreamingFeatureNode *node, MUINT32 sensorID);

    MBOOL needDisplayOutput(const StreamingFeatureNode *node);
    MBOOL needRecordOutput(const StreamingFeatureNode *node);
    MBOOL needExtraOutput(const StreamingFeatureNode *node);
    MBOOL needFullImg(const StreamingFeatureNode *node, MUINT32 sensorID);
    MBOOL needNextFullImg(const StreamingFeatureNode *node, MUINT32 sensorID);
    MBOOL needPhysicalOutput(const StreamingFeatureNode *node, MUINT32 sensorID);

    MBOOL hasGeneralOutput() const;
    MBOOL hasDisplayOutput() const;
    MBOOL hasFDOutput() const;
    MBOOL hasRecordOutput() const;
    MBOOL hasExtraOutput() const;
    MBOOL hasPhysicalOutput(MUINT32 sensorID) const;
    MBOOL hasLargeOutput(MUINT32 sensorID) const;
    MBOOL hasOutYuv() const;

    MSize getEISInputSize() const;
    SrcCropInfo getSrcCropInfo(MUINT32 sensorID);
    EnqueInfo::Data getLastEnqData(MUINT32 sensorID) const;
    EnqueInfo::Data getCurEnqData(MUINT32 sensorID) const;
    EnqueInfo getCurEnqInfo() const;
    MVOID setLastEnqInfo(const EnqueInfo& info);

    DECLARE_VAR_MAP_INTERFACE(mVarMap, SFP_VAR, setVar, getVar, tryGetVar, clearVar);
    template <typename T>
    T getSensorVar(MUINT32 sensorID, SFP_VAR key, const T &var);

    MVOID setDumpProp(MINT32 start, MINT32 count, MBOOL byRecordNo);
    MVOID setForceIMG3O(MBOOL forceIMG3O);
    MVOID setForceWarpPass(MBOOL forceWarpPass);
    MVOID setForceGpuOut(MUINT32 forceGpuOut);
    MVOID setForceGpuRGBA(MBOOL forceGpuRGBA);
    MVOID setForcePrintIO(MBOOL forcePrintIO);
    MVOID setEISDumpInfo(const EISSourceDumpInfo& info);

    MBOOL isForceIMG3O() const;
    MBOOL hasSlave(MUINT32 sensorID) const;
    MBOOL isSlaveParamValid() const;
    FeaturePipeParam& getSlave(MUINT32 sensorID);
    const SFPSensorInput& getSensorInput() const;
    const SFPSensorInput& getSensorInput(MUINT32 sensorID) const;
    VarMap<SFP_VAR>& getSensorVarMap(MUINT32 sensorID);

    BasicImg::SensorClipInfo getSensorClipInfo(MUINT32 sensorID) const;
    MUINT32 getExtraOutputCount() const;

    const char* getFeatureMaskName() const;
    MBOOL need3DNR(MUINT32 sensorID) const;
    MBOOL needVHDR() const;
    MBOOL needDSDN20(MUINT32 sensorID) const;
    MBOOL needDSDN25(MUINT32 sensorID) const;
    MBOOL needDSDN30(MUINT32 sensorID) const;
    MBOOL needEIS() const;
    MBOOL needPreviewEIS() const;
    MBOOL needTPIYuv() const;
    MBOOL needTPIAsync() const;
    MBOOL needEarlyFSCVendorFullImg() const;
    MBOOL needWarp() const;
    MBOOL needEarlyDisplay() const;
    MBOOL needP2AEarlyDisplay() const;
    MBOOL skipMDPDisplay() const;
    MBOOL needRSC() const;
    MBOOL needFSC() const;
    MBOOL needDump() const;
    MBOOL needNddDump() const;
    MBOOL needRegDump() const;
    MBOOL is4K2K() const;
    MBOOL isP2ACRZMode() const;
    EISQ_ACTION getEISQAction() const;
    MUINT32 getEISQCounter() const;
    MBOOL useWarpPassThrough() const;
    MBOOL useDirectGpuOut() const;
    MBOOL needPrintIO() const;
    MUINT32 getMasterID() const;
    MBOOL needTOF() const;

    MUINT32 needTPILog() const;
    MUINT32 needTPIDump() const;
    MUINT32 needTPIScan() const;
    MUINT32 needTPIBypass() const;
    MUINT32 needTPIBypass(unsigned tpiNodeID) const;

    MSize getFSCMaxMarginPixel() const;
    MSizeF getEISMarginPixel() const;
    MRectF getEISCropRegion() const;
    MUINT32 getAppFPS() const;
    MUINT32 getNodeFPS() const;
    MUINT32 getNodeCycleTimeMs() const;

    IMetadata* getAppMeta() const;
    MVOID getTPIMeta(TPIRes &res) const;

    static NSCam::Utils::ULog::RequestTypeId getULogRequestTypeId() {
        return NSCam::Utils::ULog::REQ_STR_FPIPE_REQUEST;
    }

    NSCam::Utils::ULog::RequestSerial getULogRequestSerial() const {
        return mRequestNo;
    }

private:
    MVOID checkEISQControl();
    MVOID appendSlaveMask();
    MUINT32 getFeatureMask(MUINT32 sensorID);

    MVOID calNonLargeSrcCrop(SrcCropInfo &info, MUINT32 sensorID);
    MVOID calGeneralZoomROI();
    MVOID updateEnqueInfo();

    void appendEisTag(string& str, MUINT32 mFeatureMask) const;
    void append3DNRTag(string& str, MUINT32 mFeatureMask) const;
    void appendRSCTag(string& str, MUINT32 mFeatureMask) const;
    void appendDSDNTag(string& str, MUINT32 mFeatureMask) const;
    void appendFSCTag(string& str, MUINT32 mFeatureMask) const;
    void appendVendorTag(string& str, MUINT32 mFeatureMask) const;
    void appendNoneTag(string& str, MUINT32 mFeatureMask) const;
    void appendDefaultTag(string& str, MUINT32 mFeatureMask) const;

private:
    IPQCtrl_const mBasicPQ;
    IPQCtrl_const mSlaveBasicPQ;
    std::unordered_map<MUINT32, FeaturePipeParam> &mSlaveParamMap;
    std::unordered_map<MUINT32, SrcCropInfo> mNonLargeSrcCrops;
    MRectF mGeneralZoomROI;
    MSize mFullImgSize;
    MBOOL mHasGeneralOutput = MFALSE;

    FPSCounter *mDisplayFPSCounter = NULL;
    FPSCounter *mFrameFPSCounter = NULL;

    static std::unordered_map<MUINT32, std::string> mFeatureMaskNameMap;

    MBOOL mResult;
    MBOOL mNeedDump;
    MBOOL mForceIMG3O;
    MBOOL mForceWarpPass;
    MUINT32 mForceGpuOut;
    MBOOL mForceGpuRGBA;
    MBOOL mForcePrintIO;
    MBOOL mIs4K2K;
    MBOOL mIsP2ACRZMode;
    EISQState mEISQState;
    EISSourceDumpInfo mEisDumpInfo;
    OutputControl mOutputControl;
    EnqueInfo mLastEnqInfo;
    EnqueInfo mCurEnqInfo;

    MUINT32 mTPILog = 0;
    MUINT32 mTPIDump = 0;
    MUINT32 mTPIScan = 0;
    MUINT32 mTPIBypass = 0;

    MUINT32 mAppFPS = 30;
    MUINT32 mNodeCycleTimeMs = 0;
    MUINT32 mNodeFPS = 30;

    MBOOL mHasOutYuv = MFALSE;
}; // class StreamingFeatureRequest end
typedef android::sp<StreamingFeatureRequest> RequestPtr;

class WatchDogData
{
public:
    MVOID setWatchDog(const SFPWatchdog &watchdog) const { mCBWatchDog = watchdog; }
private:
    mutable SFPWatchdog mCBWatchDog;
};

template <typename T>
class Data
{
public:
    T mData;
    RequestPtr mRequest;

    // lower value will be process first
    MUINT32 mPriority;

    Data()
        : mPriority(0)
    {}

    virtual ~Data() {}

    Data(const T &data, const RequestPtr &request, MINT32 nice = 0)
        : mData(data)
        , mRequest(request)
      , mPriority(request->mRequestNo)
    {
        if( nice > 0 )
        {
            // TODO: watch out for overflow
            mPriority += nice;
        }
    }

    T& operator->()
    {
        return mData;
    }

    const T& operator->() const
    {
        return mData;
    }

    class IndexConverter
    {
    public:
        IWaitQueue::Index operator()(const Data &data) const
        {
            return IWaitQueue::Index(data.mRequest->mRequestNo,
                                     data.mPriority);
        }
        static unsigned getID(const Data &data)
        {
            return data.mRequest->mRequestNo;
        }
        static unsigned getPriority(const Data &data)
        {
            return data.mPriority;
        }
    };

    static NSCam::Utils::ULog::RequestTypeId getULogRequestTypeId() {
        return NSCam::Utils::ULog::REQ_STR_FPIPE_REQUEST;
    }

    NSCam::Utils::ULog::RequestSerial getULogRequestSerial() const {
        return mRequest->mRequestNo;
    }
};

template <typename T>
T StreamingFeatureRequest::getSensorVar(MUINT32 sensorID, SFP_VAR key, const T &var)
{
    return getSensorVarMap(sensorID).get<T>(key, var);
}

class P2SWHolder
{
public:
  std::vector<P2G::P2SW> mP2SW;
  P2G::ILazy<NR3DMotion> mMotion;
  P2G::ILazy<NR3DMotion> mSlaveMotion;
  P2G::ILazy<P2G::GImg> mOMCMV;
  P2G::ILazy<P2G::GImg> mSlaveOMCMV;
  std::vector<MSize> mP2Sizes;
  std::vector<MSize> mSlaveP2Sizes;
  P2G::ILazy<DSDNInfo> mDSDNInfo;
  P2G::ILazy<DSDNInfo> mSlaveDSDNInfo;
};

class P2GOut
{
public:
  BasicImg mFull;
  BasicImg mDS;
  BasicImg mDN;
  NextImgMap mNextMap;

public:
  BasicImg getNext() const;
};

class P2HWHolder
{
public:
  std::vector<P2G::P2HW> mP2HW;
  std::vector<P2G::PMDP> mPMDP;
  P2GOut mOutM;
  P2GOut mOutS;
  MBOOL mEarlyDisplay = MFALSE;
};

typedef Data<ImgBuffer> ImgBufferData;
typedef Data<EIS_HAL_CONFIG_DATA> EisConfigData;
typedef Data<FeaturePipeParam::MSG_TYPE> CBMsgData;
typedef Data<HelpReq> HelperData;
typedef Data<RSCResult> RSCData;
typedef Data<DSDNImg> DSDNData;
typedef Data<BasicImg> BasicImgData;
typedef Data<DualBasicImg> DualBasicImgData;
typedef Data<DepthImg> DepthImgData;
typedef Data<PMDPReq> PMDPReqData;
typedef Data<TPIRes> TPIData;
typedef Data<VMDPReq> VMDPReqData;
typedef Data<WarpImg> WarpData;
typedef Data<std::vector<P2G::MSS>> MSSData;
typedef Data<P2SWHolder> P2GSWData;
typedef Data<P2HWHolder> P2GHWData;
typedef Data<NextResult> NextData;
typedef Data<ITrackingData> TrackingData;
BasicImgData toBasicImgData(const DualBasicImgData &data);
VMDPReqData toVMDPReqData(const DualBasicImgData &data);

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_DATA_H_
