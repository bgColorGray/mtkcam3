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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_TYPE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_TYPE_H_

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
#include "StreamingFeatureIO.h"

#include "OutputControl.h"
#include "EISQControl.h"
#include "MDPWrapper.h"

#include "tpi/TPIMgr.h"

#include <featurePipe/core/include/CamNodeULog.h>

#include <mtkcam3/feature/utils/p2/DIPStream.h>

using namespace NSCam::NR3D;
#include "AdvPQCtrl.h"
#include "SyncCtrl.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

enum Domain { RRZO_DOMAIN, WARP_DOMAIN };

enum IO_TYPE
{
    IO_TYPE_DISPLAY,
    IO_TYPE_RECORD,
    IO_TYPE_EXTRA,
    IO_TYPE_FD,
    IO_TYPE_PHYSICAL,
    IO_TYPE_APP_DEPTH,
};

class StreamingFeatureNode;

class IMRect
{
public:
    IMRect();
    IMRect(const IMRect &src);
    IMRect(const MSize &size, const MPoint &point = MPoint(0,0), MBOOL valid = MTRUE);
    IMRect(const MRect &rect, MBOOL valid = MTRUE);
    IMRect& operator=(const IMRect &src);
    operator bool() const;
    MRect toMRect() const;
    MRectF toMRectF() const;

public:
    MBOOL   mIsValid = MFALSE;
    MRect   mRect;
    MPoint  &p = mRect.p;
    MSize   &s = mRect.s;
};

class IMRectF
{
public:
    IMRectF();
    IMRectF(const IMRectF &src);
    IMRectF(const MSizeF &size, const MPointF &point = MPointF(0,0), MBOOL valid = MTRUE);
    IMRectF(const MRectF &rect, MBOOL valid = MTRUE);
    IMRectF& operator=(const IMRectF &src);
    operator bool() const;
    MRectF toMRectF() const;

public:
    MBOOL   mIsValid = MFALSE;
    MRectF  mRect;
    MPointF &p = mRect.p;
    MSizeF  &s = mRect.s;
};

class ITrackingData
{
public:
    ITrackingData();
    ITrackingData(const MSizeF &size, const MPointF &point = MPointF(0,0));
    ITrackingData(const MRectF &rect);

public:
    MRectF              mInitROI;
    MRectF              mTrackingROI;
    MFLOAT              mTrustValue      = 0.0;
    std::vector<MRectF> mCandiTracking;
    std::vector<MFLOAT> mTrustValueList;
    MINT32              mCandiNum        = 0;
    MINT32              mTouchTrigger    = 0;
    MBOOL               mResetTracking   = MFALSE;
};

class SrcCropInfo
{
public:
    MSize mIMGOSize   = MSize(0, 0);
    MSize mRRZOSize   = MSize(0, 0);
    MBOOL mIMGOin     = MFALSE;
    MBOOL mIsSrcCrop  = MFALSE;
    MSize mSrcSize    = MSize(0, 0);
    MRect mSrcCrop    = MRect(0, 0);
};

class EnqueInfo
{
public:
    class Data
    {
    public:
        MSize mRRZSize = MSize(0, 0);
        MINT64 mTime = 0;
        MUINT32 mFeatureMask = 0;
        Data(const MSize &rrz, MINT64 time, MUINT32 mask)
        : mRRZSize(rrz)
        , mTime(time)
        , mFeatureMask(mask)
        {}
        Data() {}
    };
    Data& operator [] (MUINT32 sensorID) { return mDataMap[sensorID]; }
    Data at(MUINT32 sensorID) const { return mDataMap.count(sensorID) ? mDataMap.at(sensorID) : Data(); }
private:
    std::unordered_map<MUINT32, Data> mDataMap;
};

class DomainTransform
{
public:
    MPointF   mOffset;
    MSizeF    mScale = MSizeF(1.0f, 1.0f);
    MVOID accumulate(const char* name, const Feature::ILog &log, const MSize &inSize, const MRectF &crop, const MSize &outSize);
    MVOID accumulate(const char* name, const Feature::ILog &log, const MSizeF &inSize, const MRectF &crop, const MSizeF &outSize);
    MVOID accumulate(const char* name, const Feature::ILog &log, const MRectF &inZoomROI, const MRectF &outZoomROI, const MSizeF &outSize);
    MRectF applyTo(const MRectF &crop) const;
};

class OutputInfo
{
public:
    MRectF mCropRect;
    MSize  mOutSize;
};

enum HelperMsg
{
    HMSG_UNKNOWN,
    HMSG_FRAME_DONE,
    HMSG_DISPLAY_DONE,
    HMSG_ASYNC_DONE,
    HMSG_META_DONE,
    HMSG_RSSO_DONE,
    HMSG_YUVO_DONE,
    HMSG_TRACKING_DONE,
    HMSG_COUNT,          //  must be the last entry
};

FeaturePipeParam::MSG_TYPE toFPPMsg(HelperMsg msg);


class HelperRWData
{
public:
    MVOID markReady(HelperMsg msg);
    MVOID markDone(HelperMsg msg);
    MBOOL isReady(HelperMsg msg) const;
    MBOOL isReadyOrDone(HelperMsg msg) const;
    MBOOL isReadyOrDone(const std::set<HelperMsg> &msgs) const;

private:
    enum
    {
        HMSG_STATE_EMPTY  = 0,
        HMSG_STATE_READY  = 1 << 0,
        HMSG_STATE_DONE   = 1 << 1,
    };

private:
    MUINT32 mMsg[HMSG_COUNT] = { HMSG_STATE_EMPTY };
};

class BasicImg
{
public:
    class SensorClipInfo
    {
    public:
        MUINT32 mSensorID = INVALID_SENSOR_ID;
        MRectF  mSensorCrop;
        MSize   mSensorSize;
        SensorClipInfo();
        SensorClipInfo(MUINT32 sensorID, const MRectF &crop, const MSize &sensorSize);
        SensorClipInfo(MUINT32 sensorID, const MRect &crop, const MSize &sensorSize);
        ~SensorClipInfo() {}
        MVOID accumulate(const char* name, const Feature::ILog &log, const MSize &inputSize, const MRectF &cropInInput);
        MVOID accumulate(const char* name, const Feature::ILog &log, const MSizeF &inputSize, const MRectF &cropInInput);
    };

public:
    android::sp<IIBuffer> mBuffer;
    IAdvPQCtrl_const      mPQCtrl;
    DomainTransform       mTransform;
    SensorClipInfo        mSensorClipInfo;
    MBOOL                 mIsReady = MTRUE;

public:
    BasicImg();
    BasicImg(const android::sp<IIBuffer> &img, MBOOL isReady = MTRUE);
    BasicImg(const android::sp<IIBuffer> &img, MUINT32 sensorID, const MRectF &crop, const MSize &sensorSize, MBOOL isReady = MTRUE);
    ~BasicImg() {}

    operator bool() const;
    MBOOL operator==(const BasicImg &rhs) const;
    MBOOL operator!=(const BasicImg &rhs) const;

    MBOOL isValid() const;
    IImageBuffer* getImageBufferPtr() const;
    MVOID setAllInfo(const BasicImg &img, const MSize &size = MSize(0,0));
    MVOID setPQCtrl(const IAdvPQCtrl_const &pqCtrl);
    MVOID setExtParam(const MSize &size);
    MBOOL syncCache(NSCam::eCacheCtrl ctrl);
    MVOID accumulate(const char* name, const Feature::ILog &log, const MSize &inSize, const MRectF &crop, const MSize &outSize);
    MVOID accumulate(const char* name, const Feature::ILog &log, const MSizeF &inSize, const MRectF &crop, const MRectF &inZoomROI, const MRectF &outZoomROI, const MSizeF &outSize);

    MSize getImgSize() const;
};

class WarpImg
{
public:
    android::sp<IIBuffer> mBuffer;
    MSizeF                mInputSize;
    MRectF                mInputCrop;
    SyncCtrlData          mSyncCtrlData;

    WarpImg();
    WarpImg(const android::sp<IIBuffer> &img, const MSizeF &targetInSize, const MRectF &targetCrop, const SyncCtrlData &syncCtrlData);
    operator bool() const;
};

struct EISSourceDumpInfo
{
    enum DUMP_TYPE
    {
        TYPE_RECORD,
        TYPE_EXTRA,
    };
    MBOOL needDump = false;
    MBOOL markFrame = false;
    MUINT32 dumpIdx = 0;
    DUMP_TYPE dumpType;
    std::string filePath;
};

typedef std::unordered_map<MUINT32, android::sp<IBufferPool>> PoolMap;

class RSCResult
{
public:
    union RSC_STA_0
    {
        MUINT32 value;
        struct
        {
            MUINT16 sta_gmv_x; // regard RSC_STA as composition of gmv_x and gmv_y
            MUINT16 sta_gmv_y;
        };
        RSC_STA_0()
            : value(0)
        {
        }
        ~RSC_STA_0() {}
    };

    ImgBuffer mMV;
    ImgBuffer mBV;
    MSize     mRssoSize;
    RSC_STA_0 mRscSta; // gmv value of RSC
    MBOOL     mIsValid;

    RSCResult();
    RSCResult(const ImgBuffer &mv, const ImgBuffer &bv, const MSize& rssoSize, const RSC_STA_0& rscSta, MBOOL valid);
    ~RSCResult() {}
};

class DSDNImg
{
public:
    class Pack
    {
    public:
        Pack();
        Pack(const BasicImg &full);
    public:
        BasicImg    mFullImg;
        BasicImg    mDS1Img;
        ImgBuffer   mDS2Img;
    };

    DSDNImg();
    DSDNImg(const BasicImg &full, const BasicImg &slavefull);

public:
    DSDNImg::Pack mPackM;
    DSDNImg::Pack mPackS;
};

class DualBasicImg
{
public:
    BasicImg mMaster;
    BasicImg mSlave;
    DualBasicImg();
    DualBasicImg(const BasicImg &master);
    DualBasicImg(const BasicImg &master, const BasicImg &slave);

    operator bool() const;
};

class TSQRecInfo
{
public:
    MBOOL                   mNeedDrop = MFALSE;
    MBOOL                   mOverride = MFALSE;
    MRectF                  mOverrideCrop;
};

class VMDPReq
{
public:
    DualBasicImg    mInputImg;
    TSQRecInfo      mTSQRecInfo;
    SyncCtrlData    mSyncCtrlData;
public:
    VMDPReq() {}
    VMDPReq(BasicImg img, const SyncCtrlData &syncCtrlData)
    : mInputImg(img)
    , mSyncCtrlData(syncCtrlData)
    {}
};

class PMDPReq
{
public:
    using P2IO = NSCam::Feature::P2Util::P2IO;

    class Data
    {
    public:
        Data();
        Data(const BasicImg &in, const std::vector<P2IO> &out, const TunBuffer &tuning = NULL);
    public:
        BasicImg                                  mMDPIn;
        std::vector<P2IO>                         mMDPOuts;
        TunBuffer                                 mTuning;
    };

public:
    MVOID add(const BasicImg &in, const std::vector<P2IO> &out, const TunBuffer &tuning = NULL);

public:
    std::vector<Data> mDatas;
    std::list<TunBuffer> mExtraTuning; // for legacy P2ANode
};

class HelpReq
{
public:
    FeaturePipeParam::MSG_TYPE mCBMsg = FeaturePipeParam::MSG_INVALID;
    HelperMsg mHelperMsg              = HMSG_UNKNOWN;
    HelpReq();
    HelpReq(HelperMsg helperMsg);
    HelpReq(FeaturePipeParam::MSG_TYPE msg);
    HelpReq(FeaturePipeParam::MSG_TYPE msg, HelperMsg helperMsg);
    HelperMsg toHelperMsg() const;
};

class DepthImg
{
public:
    BasicImg  mCleanYuvImg = BasicImg();
    ImgBuffer mDMBGImg = nullptr;
    ImgBuffer mDepthMapImg = nullptr;
    ImgBuffer mDepthIntensity = nullptr;
    ImgBuffer mDebugImg = nullptr;
    MBOOL     mDepthSucess = MFALSE;
    std::shared_ptr<IMetadata> mpCopyHalOut = nullptr;
    std::shared_ptr<IMetadata> mpCopyAppOut = nullptr;
};

class TPIRes
{
public:
    using BasicImgMap = std::map<unsigned, BasicImg>;
    BasicImgMap mSFP;
    BasicImgMap mTP;
    std::map<unsigned, IMetadata*> mMeta;

public:
    TPIRes();
    TPIRes(const BasicImg &yuv);
    TPIRes(const DualBasicImg &dual);
    ~TPIRes() {}
    MVOID add(const DepthImg &depth);
    MVOID setZoomROI(const MRectF &roi);
    MRectF getZoomROI() const;
    BasicImg getSFP(unsigned id) const;
    BasicImg getTP(unsigned id) const;
    IMetadata* getMeta(unsigned id) const;
    MVOID setSFP(unsigned id, const BasicImg &img);
    MVOID setTP(unsigned id, const BasicImg &img);
    MVOID setMeta(unsigned id, IMetadata *meta);
    MUINT32 getImgArray(TPI_Image imgs[], unsigned count) const;
    MUINT32 getMetaArray(TPI_Meta metas[], unsigned count) const;
    MVOID updateViewInfo(const char* name, const Feature::ILog &log, const TPI_Image imgs[], unsigned count);

private:
    TPI_ViewInfo makeViewInfo(unsigned id, const BasicImg &img) const;
    MVOID updateViewInfo(const char* name, const Feature::ILog &log, const TPI_Image &img, unsigned srcID, unsigned dstID);

private:
    MRectF mZoomROI;
};

class NR3DMotion
{
public:
    MUINT32 mGMV = 0;
    MBOOL isGmvInfoValid = MFALSE;
    NR3D::NR3DMVInfo gmvInfoResults[NR3D_LAYER_MAX_NUM];
};

class DSDNInfo
{
public:
    std::vector<MINT32> mProfiles;
    MBOOL mLoopValid = MTRUE;
    MUINT32 mVer = 0;
};

class NR3DStat
{
public:
    MUINT32 mLastStat = 0;

    NR3DHALResult mNr3dHalResult;
};

class TuningData // Must be copiable
{
public:
    TuningParam mParam;
    IAdvPQCtrl_const mPQCtrl;
    MINT32      mProfile = -1;
};

class NextImg
{
public:
    NextImg();
    NextImg(const BasicImg &img, const NextIO::NextAttr &attr);
    operator bool() const;
    bool isValid() const;

public:
    BasicImg mImg;
    NextIO::NextAttr mAttr;
    IImageBuffer* getImageBufferPtr() const;
};


typedef std::map<NextIO::NextID, NextImg> NextImgMap;

class NextResult
{
public:
    NextImgMap getMasterMap() const;

public:
    std::map<MUINT32, NextImgMap> mMap;
    MUINT32 mMasterID = (MUINT32)-1;
    MUINT32 mSlaveID = (MUINT32)-1;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_TYPE_H_
