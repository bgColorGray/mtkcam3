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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#ifndef _MTK_CAMERA_CAPTURE_FEATURE_PIPE_ENQUEISPWORKER_H_
#define _MTK_CAMERA_CAPTURE_FEATURE_PIPE_ENQUEISPWORKER_H_

#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
#include <mtkcam3/feature/lcenr/lcenr.h>
#include <mtkcam3/feature/msnr/IMsnr.h>
#include <mtkcam/aaa/IIspMgr.h>
#include <isp_tuning/isp_tuning.h>
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <mtkcam/utils/exif/DebugExifUtils.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include <semaphore.h>

using namespace NS3Av3;
using namespace NSIspTuning;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {
class IEnqueISPWorker
{
    friend P2ANode;
public:
    struct P2Input
    {
        IImageBuffer* mpBuf         = NULL;
        BufferID_T    mBufId        = NULL_BUFFER;
        MBOOL         mPureRaw      = MFALSE;
    };

    struct P2Output
    {
        IImageBuffer* mpBuf         = NULL;
        BufferID_T    mBufId        = NULL_BUFFER;
        TypeID_T      mTypeId       = NULL_TYPE;
        MBOOL         mHasCrop      = MFALSE;
        MRect         mCropRegion   = MRect(0,0);
        MBOOL         mEnableCZ     = MFALSE;
        MBOOL         mEnableHFG    = MFALSE;
        MUINT32       mTrans        = 0;
        MUINT32       mScaleRatio   = 0;
        MBOOL         mEarlyRelease = MFALSE;
    };

    struct MDPOutput : P2Output
    {
        MDPOutput() : P2Output() { }

        IImageBuffer* mpSource      = NULL;
        MRect         mSourceCrop   = MRect(0,0);
        MUINT32       mSourceTrans  = 0;
    };

    enum ScaleType
    {
        eScale_None,
        eScale_Down,
        eScale_Up,
    };

    struct LTMSettings
    {
        MBOOL mbExist = MFALSE;
        IMetadata::Memory mLTMData;
        MINT32 mLTMDataSize   = 0;
    };

    struct P2EnqueData
    {
        NodeID_T    mNodeId         = NULL_NODE;
        P2Input     mIMGI           = P2Input();
        P2Input     mLCEI           = P2Input();
        P2Input     mLCESHO         = P2Input();
        P2Input     mDCESI          = P2Input();
        P2Output    mIMG2O          = P2Output();
        P2Output    mWDMAO          = P2Output();
        P2Output    mWROTO          = P2Output();
        P2Output    mIMG3O          = P2Output();
        P2Output    mDCESO          = P2Output();
        P2Output    mTIMGO          = P2Output();
        P2Output    mMSSO           = P2Output();
        // Using MDP Copy
        MDPOutput   mCopy1          = MDPOutput();
        MDPOutput   mCopy2          = MDPOutput();

        IMetadata*  mpIMetaApp      = NULL;
        IMetadata*  mpIMetaHal      = NULL;
        IMetadata*  mpIMetaDynamic  = NULL;
        IMetadata*  mpOMetaHal      = NULL;
        IMetadata*  mpOMetaApp      = NULL;

        MINT32      mSensorId       = 0;
        MBOOL       mResized        = MFALSE;
        MBOOL       mYuvRep         = MFALSE;
        MINT32      mScaleType      = eScale_None;
        MSize       mScaleSize      = MSize(0,0);
        MBOOL       mEnableMFB      = MFALSE;
        MBOOL       mEnableDRE      = MFALSE;
        MBOOL       mEnableDCE      = MFALSE;
        MBOOL       mEnableVSDoF    = MFALSE;
        MBOOL       mEnableZoom     = MFALSE;
        MBOOL       mDebugLoadIn    = MFALSE;
        MBOOL       mDebugDump      = MFALSE;
        MUINT32     mDebugDumpFilter
                                    = 0;
        MBOOL       mDebugUnpack    = MFALSE;
        MBOOL       mDebugDumpMDP   = MFALSE;
        MBOOL       mTimeSharing    = MFALSE;
        MINT32      mIspProfile     = -1;
        MINT32      mIspStage       = -1;
        MINT32      mUniqueKey      = 0;
        MINT32      mRequestNo      = 0;
        MINT32      mFrameNo        = 0;
        MINT32      mFrameIndex     = 0;
        MINT32      mTaskId         = 0;
        MINT32      mRound          = 1;
        //
        std::shared_ptr<Mutex>      mpLockDCES;
        //
        std::shared_ptr<P2ANode::SharedData> mpSharedData;
        std::shared_ptr<P2ANode::RequestHolder>
                    mpHolder;
        LTMSettings mLTMSettings;
        //
        MBOOL       mbdirectRawBufCopy = MFALSE;
        MBOOL       mbMainFrame        = MTRUE;
        MBOOL       mbNeedOutMeta      = MFALSE;
    };

    class EnquePackage
    {
    public:
        std::shared_ptr<P2EnqueData>        mpEnqueData;
        PQParam*                            mpPQParam;
        NSIoPipe::NSMFB20::MSSConfig*       mpMSSConfig;
        CrspInfo*                           mpCrspParam;
        ModuleInfo*                         mpModuleSRZ4;
        ModuleInfo*                         mpModuleSRZ3;
        SmartTuningBuffer                   mpTuningData;
        android::sp<IIBuffer>               mUnpackBuffer;
        P2ANode*                            mpNode;
        MINT32                              miSensorIdx;
        MUINT32                             mTimgoType;
        MUINT32                             mRawDepthType;
        MUINT32                             mRawHDRType;
        MBOOL                               mbNeedToDump;
        Timer                               mTimer;
        shared_ptr<sem_t>                   mWait;

        EnquePackage()
            : mpPQParam(NULL)
            , mpMSSConfig(NULL)
            , mpCrspParam(NULL)
            , mpModuleSRZ4(NULL)
            , mpModuleSRZ3(NULL)
            , mpTuningData(NULL)
            , mpNode(NULL)
            , miSensorIdx(-1)
            , mTimgoType(EDIPTimgoDump_NONE)
            , mRawDepthType(EDIPRAWDepthType_NONE)
            , mRawHDRType(EDIPRawHDRType_NONE)
            , mbNeedToDump(MTRUE)
            , mWait(NULL)
        {
        }

        ~EnquePackage();
    };

     // Static information
    struct EnqueWorkerInfo
    {
        RequestPtr                     mpRequest = nullptr;
        sp<CaptureFeatureNodeRequest>  mpNodeRequest = nullptr;

        MINT32                    mFrameNo        = 0;
        MINT32                    mRequestNo      = 0;
        MINT32                    mSensorId       = 0;
        MINT32                    mSubSensorId    = 0;
        MUINT                     mSensorFmt      = -1;
        MUINT                     mSubSensorFmt   = -1;

        IMetadata*                mpIMetaDynamic  = nullptr;
        IMetadata*                mpIMetaApp      = nullptr;
        IMetadata*                mpIMetaHal      = nullptr;
        IMetadata*                mpOMetaApp      = nullptr;
        IMetadata*                mpOMetaHal      = nullptr;
        IMetadata*                mpIMetaHal2     = nullptr;
        IMetadata*                mpIMetaDynamic2 = nullptr;
        //
        shared_ptr<P2ANode::RequestHolder> pHolder       = nullptr;
        shared_ptr<P2ANode::SharedData>    pSharedData   = nullptr;

        MINT32                    mUniqueKey     = 0;
        ProfileMapperKey          mMappingKey;

        MBOOL                     isVsdof       = MFALSE;
        MBOOL                     isZoom        = MFALSE;
        MBOOL                     isPhysical    = MFALSE;
        MBOOL                     isRunMSNR     = MFALSE;
        MBOOL                     isRawInput    = MFALSE;
        MBOOL                     isDualMode    = MFALSE;
        MBOOL                     isForceMargin = MFALSE;
        ESensorCombination        sensorCom;

        MBOOL                     isYuvRep      = MFALSE;
        MBOOL                     hasYUVOutput  = MFALSE;
        MBOOL                     isCShot       = MFALSE;

        MBOOL                     isMFB         = MFALSE;
        MBOOL                     isDRE         = MFALSE;
        MBOOL                     isRunDCE      = MFALSE;
        MBOOL                     isNotApplyDCE = MFALSE;
        MsnrMode                  mMsnrmode;

        NodeID_T                  mNodeId = 0;
        P2ANode*                  mNode = nullptr;

        IImageBuffer*             mpRound1       = nullptr;
        IImageBuffer*             mpDCES_Buf     = nullptr;
        IImageBuffer*             mpRound1_sub   = nullptr;
        IImageBuffer*             mpDCES_Buf_sub = nullptr;

        EnqueWorkerInfo(P2ANode* node, NodeID_T nodeId, RequestPtr& pRequest);
    };
    EnqueWorkerInfo*              mInfo;


protected:
    // prepare ltm enque data
    MVOID prepareLTMSettingData(IImageBuffer* pInBuffer, IMetadata* pInHalMeta, LTMSettings& outLTMSetting);
    MVOID updateSensorCropRegion(RequestPtr& pRequest, MRect cropRegion);
    // acquire dceso buffer
    IImageBuffer* acquireDCESOBuffer(MBOOL isMain = MTRUE);
    // dual sync info
    MVOID dualSyncInfo(TuningParam& tuningParam, P2EnqueData& enqueData, MBOOL isMaster);
    // direct buffer copy by using CPU
    MERROR directBufCopy(const IImageBuffer* pSrcBuf, IImageBuffer* pDstBuf, QParams& rParams);

public:
    static MVOID onP2SuccessCallback(QParams& rParams);
    static MVOID onP2FailedCallback(QParams& rParams);
    static MBOOL copyBuffers(EnquePackage* pkg);
    static MVOID unpackRawBuffer(IImageBuffer* pPkInBuf, IImageBuffer* pUpkOutBuf );
    // check if request contains yuv output
    static MBOOL checkHasYUVOutput(sp<CaptureFeatureNodeRequest> pNodeReq);
    // The debug buffer size will be allocated according to IMGI
    MVOID addDebugBuffer(std::shared_ptr<P2EnqueData>& pEnqueData);

    // the timing that the next worker should wait until this worker has finished P2 callback, ex: DCE second round should
    // wait until the first round P2 callback, then passing through setP2ISP
    enum eWaitTiming
    {
        NO_NEED_WAIT,
        BEFORE_SETP2ISP
    };
    virtual MBOOL run(const shared_ptr<sem_t>& waitForTiming=NULL, eWaitTiming timing=NO_NEED_WAIT) = 0;
    MINT32 fnQueryISPProfile(const ProfileMapperKey& key, const EProfileMappingStages& stage);
    MBOOL enqueISP(shared_ptr<P2EnqueData>& pEnqueData, const shared_ptr<sem_t>& waitForTiming=NULL, eWaitTiming timing=NO_NEED_WAIT);
    MBOOL addNextTask(IEnqueISPWorker* nextWorker, eWaitTiming timing=NO_NEED_WAIT)
    {
        mNextWorker = nextWorker;
        mTiming = timing;
        mWaitForTiming = std::make_shared<sem_t>();
        sem_init(mWaitForTiming.get(), 0, 0);
        return MTRUE;
    }

    MBOOL getBuffer(shared_ptr<P2EnqueData>& pEnqueData);
    MVOID onDeviceTuning(shared_ptr<P2EnqueData>& pEnqueData);
    MBOOL getCropRegion(shared_ptr<P2EnqueData>& pEnqueData, sp<CropCalculator::Factor> pFactor);
    MVOID fillPQParam(shared_ptr<P2EnqueData>& pEnqueData, TuningParam& tuningParam, PQParam* pPQParam);
    MBOOL setP2ISP(EnquePackage* pPackage, TuningParam& tuningParam);
    MBOOL createQParam(EnquePackage* pPackage, TuningParam& tuningParam, NSIoPipe::QParams& qParam);
    MBOOL isCovered(P2Output& rSrc, MDPOutput& rDst);
    // Main Full size common part for R2Y and Y2Y
    MBOOL fMainFullCommon(shared_ptr<P2EnqueData>& pEnqueData, const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL getCrop(P2EnqueData& rEnqueData, BufferID_T typeId, MRect& cropRegion, MUINT32& trans, MUINT32& scaleRatio, MBOOL& needCrop);

public:
    // enum P2Task
    // {
    //     TASK_M1_RRZO,      TASK_RSZ_MAIN
    //     TASK_M1_IMGO_RAW,  TASK_FULL_RAWOUT
    //     TASK_M1_IMGO_PRE,  TASK_DCE_MAIN_1st
    //     TASK_M1_IMGO,      TASK_DCE_MAIN_2nd
    //                        TASK_FULL_MAIN


    //     TASK_M2_RRZO,      TASK_RSZ_SUB
    //     TASK_M2_IMGO_PRE,  TASK_DCE_SUB_1st
    //     TASK_M2_IMGO,      TASK_FULL_SUB
    //                        TASK_YUV_REPRO
    //                        TASK_NONE
    // };
    enum TaskID
    {
        TASK_RSZ_MAIN,
        TASK_FULL_RAWOUT,
        TASK_DCE_MAIN_1st,
        TASK_DCE_MAIN_2nd,
        TASK_FULL_MAIN,
        TASK_RSZ_SUB,
        TASK_DCE_SUB_1st,
        TASK_FULL_SUB,
        TASK_YUV_REPRO,
        TASK_NONE
    };

protected:
    std::shared_ptr<sem_t> mWaitForTiming = nullptr;
    IEnqueISPWorker* mNextWorker = nullptr;
    eWaitTiming mTiming = NO_NEED_WAIT;
    TaskID mTaskId = TASK_NONE;
    virtual ~IEnqueISPWorker()
    {
        if(mWaitForTiming)
            sem_destroy(mWaitForTiming.get());
    }
};

// Raw to Raw process
class EnqueISPWorker_R2R : public IEnqueISPWorker
{
public:
    MBOOL run(const shared_ptr<sem_t>& waitForTiming=NULL, eWaitTiming timing=NO_NEED_WAIT);
    EnqueISPWorker_R2R(EnqueWorkerInfo* info, TaskID task=TASK_FULL_RAWOUT)
    {
        mInfo   = info;
        mTaskId = task;
    }
    ~EnqueISPWorker_R2R(){}
};

// Raw to YUV process
class EnqueISPWorker_R2Y : public IEnqueISPWorker
{
public:
    MBOOL run(const shared_ptr<sem_t>& waitForTiming=NULL, eWaitTiming timing=NO_NEED_WAIT);
    EnqueISPWorker_R2Y(EnqueWorkerInfo* info, TaskID task)
    {
        mInfo   = info;
        mTaskId = task;
    }
    ~EnqueISPWorker_R2Y(){}
    MBOOL fDCE1stRound(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL fDCE2ndRound(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL fMainFullfunction(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL fDCEsub1stround(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL fFullsub(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL fRSZMain(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    MBOOL fRSZsub(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
};

// YUV reprocessing
class EnqueISPWorker_Y2Y : public IEnqueISPWorker
{
public:
    MBOOL run(const shared_ptr<sem_t>& waitForTiming=NULL, eWaitTiming timing=NO_NEED_WAIT);
    EnqueISPWorker_Y2Y(EnqueWorkerInfo* info, TaskID task=TASK_YUV_REPRO)
    {
        mInfo   = info;
        mTaskId = task;
    }
    MBOOL fYuvReprofunction(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing);
    ~EnqueISPWorker_Y2Y(){}
};

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam


namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class QParamGenerator
{
    public:
         struct FrameInfo
        {
            MUINT32 FrameNo;
            MUINT32 RequestNo;
            MUINT32 Timestamp;
            MINT32 UniqueKey;
            MINT32 IspProfile;
            MINT32 SensorID;
        };
        QParamGenerator(MUINT32 iSensorIdx, ENormalStreamTag streamTag);
        MVOID addCrop(eCropGroup groupID, MPoint startLoc, MSize cropSize, MSize resizeDst, bool isMDPCrop=false);
        MVOID addInput(NSCam::NSIoPipe::PortID portID, IImageBuffer* pImgBuf);
        MVOID addOutput(NSCam::NSIoPipe::PortID portID, IImageBuffer* pImgBuf, MINT32 transform);
        MVOID addModuleInfo(MUINT32 moduleTag, MVOID* moduleStruct);
        MVOID addExtraParam(EPostProcCmdIndex cmdIdx, MVOID* param);
        MVOID insertTuningBuf(MVOID* pTuningBuf);
        MVOID setInfo(const FrameInfo& frameInfo, MINT8 dumpFlag);
        MBOOL generate(QParams& rQParam);
        MBOOL checkValid();

    private:
        FrameParams mPass2EnqueFrame;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_CAPTURE_FEATURE_PIPE_ENQUEISPWORKER_H_
