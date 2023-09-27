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
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OmR
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

#define LOG_TAG "MtkCam/fdNodeImp"

//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/cat/CamAutoTestLogger.h>
#include <mtkcam3/pipeline/pipeline/IPipelineNode.h>
#include "../BaseNode.h"
#include <mtkcam3/pipeline/hwnode/FDNode.h>
#include <mtkcam3/feature/FaceDetection/fd_hal_base.h>
#include <faces.h>
// GS
#include <mtkcam3/feature/Gesture/gs_hal_base.h>
//
#include <mtkcam/utils/hw/IFDContainer.h>
#include <mtkcam/utils/imgbuf/ISecureImageBufferHeap.h>
// EIS
#include <mtkcam3/feature/utils/FeatureCache.h>
//
#include <utils/RWLock.h>
#include <utils/Thread.h>
//
#include <sys/prctl.h>

//for sensor
#include <math.h>
#include <mtkcam/utils/sys/SensorProvider.h>
//
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/aaa/IHal3A.h>
#include <mtkcam/aaa/IHalISP.h>
// Dump naming rule
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
// DMA Buf
#include "BufferAllocator/BufferAllocator.h"
#include "BufferAllocator/BufferAllocatorWrapper.h"

#include "../MyUtils.h"

#include <cutils/properties.h>
#include <ctime>

using namespace android;
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/hw/HwTransform.h>

#if (1==STEREO_SUPPORTED)
#include <mtkcam3/feature/stereo/hal/stereo_fd_proxy.h>
#endif

using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::Utils::Sync;
using namespace NS3Av3;
using namespace NSCam::Utils;
using NSCamHW::HwMatrix;
using NSCamHW::HwTransHelper;
using NSCam::Feature::EISCache;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGCAT(fmt, arg...)      CAM_ULOGMD("[CAT][FD] " fmt, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#define max(a,b)  ((a) < (b) ? (b) : (a))
#define min(a,b)  ((a) < (b) ? (a) : (b))

#include <semaphore.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FD_NODE);
#define FD_SKIP_NUM (0)
#define DUMP_SENSOR_LOG (0)
#define DUMP_IMAGE (0)
#define LANDMARK_NUM (5)

#define FREE_DATA(_P_) \
    if ( _P_ != NULL ) { \
        delete [] _P_; \
        _P_ = NULL; \
    }

struct FDImage {
    int w;
    int h;
    MUINT8* AddrY;
    MUINT8* AddrU;
    MUINT8* AddrV;
    MUINT8* PAddrY;
    MUINT32 FD_Y = 0;
    MINT32 format;
    MINT32 planes;
    MINT32 gammaIndex = 0;
    MINT32 lvdata = 0;
    MINT64 timestamp;
    sp<IImageBuffer> pImg;
    MINTPTR SecureHandle = 0;
};

static timespec gUpdateTime;


/******************************************************************************
 *
 ******************************************************************************/
//
//  [Input]
//      Image/Yuv
//      Meta/Request
//
//  [Output]
//      Meta/Result
//
namespace {
class FdNodeImp
    : public BaseNode
    , public FdNode
    , protected Thread
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                            Definitions.
    typedef android::sp<IPipelineFrame>                     QueNode_T;
    typedef android::List<QueNode_T>                        Que_T;
    //typedef android::Vector<android::sp<IMetaStreamInfo> >  MetaStreamInfoSetT;
    //typedef android::Vector<android::sp<IImageStreamInfo> > ImageStreamInfoSetT;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Definitions.

    /**
     * Initialization Parameters.
     */



protected:  ////                    Data Members. (Config)
    mutable RWLock                  mConfigRWLock;
    sp<IMetaStreamInfo>             mpOutMetaStreamInfo_Result;
    sp<IMetaStreamInfo>             mpInMetaStreamInfo_Request;
    sp<IMetaStreamInfo>             mpInMetaStreamInfo_P2Result;
    sp<IImageStreamInfo>            mpInImageStreamInfo_Yuv;
    sp<IImageStreamInfo>            mpHalImage_P1_Rrzo;
    //ImageStreamInfoSetT             mpInImageStreamInfo_Yuv;

protected:  ////                    Data Members. (Request Queue)
    mutable Mutex                   mRequestQueueLock;
    mutable Mutex                   mResultLock;
    Condition                       mResultCond;
    mutable Mutex                   mFDRunningLock;
    Condition                       mRequestQueueCond;
    MBOOL                           mbRequestDrained;
    Condition                       mbRequestDrainedCond;
    Que_T                           mRequestQueue;

    MINT32                          mLogLevel;
    MINT32                          mDump;
    bool                            mForceFDEnable;

protected:  ////                    Data Members. (Request Queue)
    IHal3A*                         mp3AHal;
    IHalISP*                        mpIspHal;

protected:  ////                    Operations.
    MERROR                          onDequeRequest(
                                        android::sp<IPipelineFrame>& rpFrame
                                    );
    MVOID                           onProcessFrame(
                                        android::sp<IPipelineFrame> const& pFrame
                                    );

    MVOID                           waitForRequestDrained();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations in base class Thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void                    requestExit();

    // Good place to do one-time initializations
    virtual status_t                readyToRun();

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool                    threadLoop();
    //
    MVOID                           ReturnFDResult(
                                        IMetadata*              pOutMetadataResult,
                                        IMetadata*              pInpMetadataRequest,
                                        IMetadata*              pInpMetadataHalResult
                                    );

    MVOID                           ReturnCustomResult(
                                        IMetadata*              pOutMetadataResult,
                                        MtkCameraFaceMetadata*  pFaceMeta,
                                        float                   FaceRatio
                                    );

    MVOID                           RunFaceDetection();
    // +++ FD Async mode
    static MVOID*                   FDHalThreadLoop(MVOID*);

    MVOID                           setFDLock(MBOOL val);

    // --- FD Async mode

    MINT32                          doGestureDetect(MUINT8* pImage, MINT32 RotateInfo);

    MINT32                          getGestureResult();

    MINT32                          onInitFDProc();

    MINT32                          convertResult(MtkCameraFaceMetadata* meta, bool isUI);

    MVOID                           prepareFDParams(
                                                                IMetadata*              pInpMetadataRequest,
                                                                IMetadata*              pInpMetadataHalResult,
                                                                MSize                   imgSize
                                                            );

    MINT32                          calculateRotateDegree(MINT32 &PhoneDevRot);


    MINT32                          tryToUpdateOldData(
                                                                IMetadata*              pOutMetadataResult,
                                                                MINT32                  FDMode
                                                            );

    MINT32                          calculateP1FDCrop(IMetadata*              pInpMetadataRequest,
                                                              IMetadata*              pInpMetadataHalResult);


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Operations.
                                    FdNodeImp();

                                    ~FdNodeImp();

    virtual MERROR                  config(ConfigParams const& rParams);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineNode Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Operations.

    virtual MERROR                  init(InitParams const& rParams);

    virtual MERROR                  uninit();

    virtual MERROR                  flush();

    virtual MERROR                  flush(
                                        android::sp<IPipelineFrame> const &pFrame
                                    );


    virtual MERROR                  queue(
                                        android::sp<IPipelineFrame> pFrame
                                    );

private:
    // crop-related
    MRect                           mActiveArray;
    MtkCameraFaceMetadata*          mpDetectedFaces;
    MtkCameraFaceMetadata*          mpDetected3AFaces;
    MBOOL                           mbInited;
    MBOOL                           mbDoFD;
    MBOOL                           mbCalculateP1FD;
    MUINT8*                         mFDImgBuffer;// = new unsigned char[640*480];
    MUINT8*                         mFDWorkingBuffer;
    MUINT32                         mFDWorkingBufferSize;
    halFDBase*                      mpFDHalObj;
    halGSBase*                      mpGSHalObj;
    MINT32                          mImageWidth;
    MINT32                          mImageHeight;
    MUINT8*                         mPureYBuf;
    struct FDImage                  mDupImage;
    MINT32                          mFDStopped = 0;
    MINT32                          mSensorRot = 0;
    MINT32                          mSensorFacing = 0;
    // SD
    MINT32                          mSD_Result;
    MINT32                          mCurrent_mode;
    MINT32                          mPrevSD;
    // GS
    MINT32                          mGDEnable;
    // EIS
    EISCache*                       mpEISCache = nullptr;
    bool                            mIsEISOn = false;
    MINT32                          mEISScale_W = 100;
    MINT32                          mEISScale_H = 100;
    MSize                           mPreviewSize;
    MSize                           mWarpOutSize;
    MRectF                          mDisplayCrop;
    MINT32                          mAppOffset_X = 0;
    MINT32                          mAppOffset_Y = 0;
    MINT32                          mZoomRatio_x100 = 100;
    // FSC
    MBOOL                           mFSCEnable = false;
    MINT32                          mFSCMargin = 105;

    MINT32                          mFDProcInited;
    MINT32                          mFirstUpdate = 0;

    //Sensor
    android::sp<SensorProvider>     mpSensorProvider;
    MINT32                          mprvDegree = 0;
    MRect                           mcropRegion;
    MRect                           mUIcropRegion;
    MRect                           mEISWarpRegion;
    MSize                           mSensorSize;
    MSize                           mSensorSize2Active;
    // +++ FD Async mode
    pthread_t                       mFDHalThread;
    sem_t                           semFD;
    mutable Mutex                   mFDLock;
    MBOOL                           mIsFDBusy = false;
    MBOOL                           mStopFD = false;

    IImageBufferAllocator*          mAllocator;

    // --- FD Async mode
    // ISP 6.0
    MBOOL                           misDirectYuv = false;
    MINT32                          mFDPeriod;
    MINT32                          mConfigPeriod;
    MINT32                          mVideoFDPeriod;
    MINT32                          mVsdofFDPeriod;
    MINT64                          mLastFrameTime = 0;
    MINT32                          mFDCountTime;
    MSize                           mRealFDYUVSize = MSize(0, 0);
    // for dual zoom
    float                           mFovRatio = 0.0;
    MPoint                          mShiftPoint = MPoint(0, 0);
    MINT32                          mMasterIndex = 0;
    MINT32                          mSkipStoreFDData = false;
    std::vector<IHal3A*>            mvp3AHal;
    std::vector<IHalISP*>           mvpIspHal;
    std::vector<MINT32>             mvSensorId;
    std::vector<halFDBase*>         mvpFDHalObj;
    //
    MINT32                          mSensorMode;
    HwMatrix                        mSensor2Active;
    //
    float                           mFaceRatio = 0.0;
    // secure camera
    MINT32                          misSecure = false;
    SecType                         mSecureMemType = SecType::mem_normal;
    sp<ISecureImageBufferHeap> mpSecureImageBufferHeap = nullptr;
    sp<IImageBuffer> mpSecureImageBuffer = nullptr;
    /*MUINT8*                         mpSecureData[3];
    MUINT8*                         mpSecureData3A[3];*/

    MBOOL                           misVsdof = false;
    MBOOL                           misVideoMode = false;

    // monitor FD fps
    MBOOL   mbNeedReCount = false;
    MINT64  mFdStartTime;
    MINT64  mFdEndTime;
    MINT32  mFdDoCounter = 0;
    // for 3A
    MINT32  mMagicNo = -1;
    // for physical stream
    std::vector<ConfigParams::PhysicalStream> mvpPhysicalStreamsInfo;

    // async
    MBOOL                           mForceAsyncMode = false;
    MINT32  mLastFrameNo = 0;
};
};  //namespace

/******************************************************************************
 *
 ******************************************************************************/
FdNodeImp::
FdNodeImp()
    : BaseNode()
    , FdNode()
    //
    , mConfigRWLock()
    //
    , mRequestQueueLock()
    , mResultLock()
    , mResultCond()
    , mFDRunningLock()
    , mRequestQueueCond()
    , mbRequestDrained(MFALSE)
    , mbRequestDrainedCond()
    , mRequestQueue()
    //
    , mpDetectedFaces(NULL)
    , mpDetected3AFaces(NULL)
    , mbInited(MFALSE)
    , mbDoFD(MFALSE)
    , mFDImgBuffer(NULL)
    , mFDWorkingBuffer(NULL)
    , mFDWorkingBufferSize(0)
    , mpFDHalObj(NULL)
    , mpGSHalObj(NULL)
    , mImageWidth(0)
    , mImageHeight(0)
    , mPureYBuf(NULL)
    , mSD_Result(0)
    , mCurrent_mode(0)
    , mPrevSD(0)
    , mGDEnable(0)
    , mFDProcInited(0)
{
    mNodeName = "FdNode";//default name
    mAllocator = NULL;
    mp3AHal = NULL;
    mpIspHal = NULL;
    mDupImage.pImg = NULL;
    mpSensorProvider = NULL;
    mvpFDHalObj.clear();

    char cLogLevel[PROPERTY_VALUE_MAX] = {0};
    ::property_get("vendor.debug.camera.log", cLogLevel, "0");
    mLogLevel = atoi(cLogLevel);
    if ( mLogLevel == 0 ) {
        ::property_get("vendor.debug.camera.log.FDNode", cLogLevel, "0");
        mLogLevel = atoi(cLogLevel);
    }
    mDump = ::property_get_int32("vendor.debug.camera.dump.FDNode", 0);
    #ifdef MTKCAM_FD_PERIOD
    mConfigPeriod = mFDPeriod = ::property_get_int32("vendor.debug.camera.FDPeriod", MTKCAM_FD_PERIOD);
    #else
    mConfigPeriod = mFDPeriod = ::property_get_int32("vendor.debug.camera.FDPeriod", 33);
    #endif
    mVsdofFDPeriod = ::property_get_int32("vendor.debug.camera.VsdofFDPeriod", 33);
    mVideoFDPeriod = ::property_get_int32("vendor.debug.camera.VideoFDPeriod", 33);
    mForceFDEnable = ::property_get_bool("persist.vendor.debug.camera3.ForceFDOn", false);
    mForceAsyncMode = ::property_get_bool("vendor.debug.camera.FDAsync", false);

    int secType = ::property_get_int32("vendor.debug.camera.SecureFD", 0);
    switch(secType)
    {
        case 0:
            mSecureMemType = SecType::mem_normal;
            break;
        case 1:
            mSecureMemType = SecType::mem_protected;
            break;
        case 2:
            mSecureMemType = SecType::mem_secure;
            break;
        default:
            mSecureMemType = SecType::mem_normal;
    }
}

/******************************************************************************
 *
 ******************************************************************************/
FdNodeImp::
~FdNodeImp()
{
    MY_LOGD("FDNode -");
}

MINT32
FdNodeImp::
onInitFDProc()
{
    return 0;
}

MINT32
FdNodeImp::
convertResult(MtkCameraFaceMetadata* meta, bool isUI)
{
    MRect cropInfo = isUI ? mUIcropRegion : mcropRegion;
    #define CONVERT_X(_val) _val = ((_val+1000) * cropInfo.s.w/2000) + cropInfo.p.x
    #define CONVERT_Y(_val) _val = ((_val+1000) * cropInfo.s.h/2000) + cropInfo.p.y
    for (int i = 0; i < meta->number_of_faces; i++)
    {
        CONVERT_X(meta->faces[i].rect[0]); // Left
        CONVERT_Y(meta->faces[i].rect[1]); // Top
        CONVERT_X(meta->faces[i].rect[2]); // Right
        CONVERT_Y(meta->faces[i].rect[3]); // Bottom
        //
        CONVERT_X(meta->leyex0[i]);
        CONVERT_Y(meta->leyey0[i]);
        CONVERT_X(meta->leyex1[i]);
        CONVERT_Y(meta->leyey1[i]);
        CONVERT_X(meta->reyex0[i]);
        CONVERT_Y(meta->reyey0[i]);
        CONVERT_X(meta->reyex1[i]);
        CONVERT_Y(meta->reyey1[i]);
        CONVERT_X(meta->mouthx0[i]);
        CONVERT_Y(meta->mouthy0[i]);
        CONVERT_X(meta->mouthx1[i]);
        CONVERT_Y(meta->mouthy1[i]);
        CONVERT_X(meta->nosex[i]);
        CONVERT_Y(meta->nosey[i]);
        //
        CONVERT_X(meta->leyeux[i]);
        CONVERT_Y(meta->leyeuy[i]);
        CONVERT_X(meta->leyedx[i]);
        CONVERT_Y(meta->leyedy[i]);
        CONVERT_X(meta->reyeux[i]);
        CONVERT_Y(meta->reyeuy[i]);
        CONVERT_X(meta->reyedx[i]);
        CONVERT_Y(meta->reyedy[i]);

    }
    #undef CONVERT_X
    #undef CONVERT_Y
    if (isUI)
    {
        if (mIsEISOn && mEISWarpRegion.s.w > 0 && mEISWarpRegion.s.h > 0) {
            for (int i = 0; i < meta->number_of_faces; i++) {
                MY_LOGD_IF(mLogLevel, "Face rect on EIS warp in(%d, %d, %d, %d)", meta->faces[i].rect[0],
                meta->faces[i].rect[1], meta->faces[i].rect[2], meta->faces[i].rect[3]);

                meta->faces[i].rect[0] = meta->faces[i].rect[0] - mEISWarpRegion.p.x;
                meta->faces[i].rect[1] = meta->faces[i].rect[1] - mEISWarpRegion.p.y;
                meta->faces[i].rect[2] = meta->faces[i].rect[2] - mEISWarpRegion.p.x;
                meta->faces[i].rect[3] = meta->faces[i].rect[3] - mEISWarpRegion.p.y;
                MY_LOGD_IF(mLogLevel, "Face rect on EIS warp crop(%d, %d, %d, %d)", meta->faces[i].rect[0],
                meta->faces[i].rect[1], meta->faces[i].rect[2], meta->faces[i].rect[3]);

                meta->faces[i].rect[0] = (meta->faces[i].rect[0] * mWarpOutSize.w) / mEISWarpRegion.s.w;
                meta->faces[i].rect[1] = (meta->faces[i].rect[1] * mWarpOutSize.h) / mEISWarpRegion.s.h;
                meta->faces[i].rect[2] = (meta->faces[i].rect[2] * mWarpOutSize.w) / mEISWarpRegion.s.w;
                meta->faces[i].rect[3] = (meta->faces[i].rect[3] * mWarpOutSize.h) / mEISWarpRegion.s.h;
                MY_LOGD_IF(mLogLevel, "WarpOutSize:%dx%d, EISWarpRegion size:%dx%d",
                mWarpOutSize.w, mWarpOutSize.h, mEISWarpRegion.s.w, mEISWarpRegion.s.h);
                MY_LOGD_IF(mLogLevel, "Face rect on EIS buffer(%d, %d, %d, %d)", meta->faces[i].rect[0],
                meta->faces[i].rect[1], meta->faces[i].rect[2], meta->faces[i].rect[3]);

                meta->faces[i].rect[0] = meta->faces[i].rect[0] - (int)mDisplayCrop.p.x;
                meta->faces[i].rect[1] = meta->faces[i].rect[1] - (int)mDisplayCrop.p.y;
                meta->faces[i].rect[2] = meta->faces[i].rect[2] - (int)mDisplayCrop.p.x;
                meta->faces[i].rect[3] = meta->faces[i].rect[3] - (int)mDisplayCrop.p.y;
                MY_LOGD_IF(mLogLevel, "Face rect on EIS buffer crop(%d, %d, %d, %d)", meta->faces[i].rect[0],
                meta->faces[i].rect[1], meta->faces[i].rect[2], meta->faces[i].rect[3]);

                meta->faces[i].rect[0] = meta->faces[i].rect[0] * mEISScale_W / 100;
                meta->faces[i].rect[1] = meta->faces[i].rect[1] * mEISScale_H / 100;
                meta->faces[i].rect[2] = meta->faces[i].rect[2] * mEISScale_W / 100;
                meta->faces[i].rect[3] = meta->faces[i].rect[3] * mEISScale_H / 100;
                MY_LOGD_IF(mLogLevel, "EISScale_W: %d, EISScale_H: %d, APP face rect(%d, %d, %d, %d)",
                mEISScale_W, mEISScale_H, meta->faces[i].rect[0], meta->faces[i].rect[1], meta->faces[i].rect[2], meta->faces[i].rect[3]);

                meta->faces[i].rect[0] += mAppOffset_X;
                meta->faces[i].rect[1] += mAppOffset_Y;
                meta->faces[i].rect[2] += mAppOffset_X;
                meta->faces[i].rect[3] += mAppOffset_Y;
            }
        }
        return 0;
    }
    // set hal 3A
    if(mp3AHal)
    {
        MY_LOGI_IF(mLogLevel, "set 3A fd info");
        mp3AHal->setFDInfoOnActiveArray(meta);
    }
    if (mpIspHal)
    {
        mpIspHal->setFDInfo(meta);
    }
    // copy fd info to fd container
    if (mSkipStoreFDData == false)
    {
        auto fdWriter = IFDContainer::createInstance(LOG_TAG, IFDContainer::eFDContainer_Opt_Write);
        auto fddata = fdWriter->editLock(meta->timestamp);
        MY_LOGI_IF(mLogLevel, "store to fd container");
        clock_gettime(CLOCK_MONOTONIC, &gUpdateTime);
        if (fddata != NULL)
        {
            memcpy(&(fddata->facedata), meta, sizeof(MtkCameraFaceMetadata));
            fddata->sensorId = mvSensorId[mMasterIndex];
            fdWriter->editUnlock(fddata);
        }
        else
        {
            MY_LOGW("get container FD buffer null");
        }
    }
    return 0;
}

MINT32
FdNodeImp::
calculateRotateDegree(MINT32 &PhoneDevRot)
{
    SensorData acceDa;
    MBOOL acceDaVd = mpSensorProvider->getLatestSensorData( SENSOR_TYPE_ACCELERATION, acceDa);

    if( acceDaVd && acceDa.timestamp)
    {
        float X,Y,Z;
        float OneEightyOverPi = 57.29577957855f;
        float magnitude;
        float angle;
        int ori = 0;
        X = -(acceDa.acceleration[0]);
        Y = -(acceDa.acceleration[1]);
        Z = -(acceDa.acceleration[2]);
        magnitude = X*X + Y*Y;
        if ( mLogLevel )
        {
            MY_LOGD("Acc(%f,%f,%f,%" PRId64 ")",
                    acceDa.acceleration[0],
                    acceDa.acceleration[1],
                    acceDa.acceleration[2],
                    acceDa.timestamp);
        }
        if(magnitude * 4 >= Z*Z)
        {
            angle = atan2(-Y, X) * OneEightyOverPi;
            ori = 180 - round(angle);

            while(ori >= 360) {
                ori -= 360;
            }
            while(ori < 0) {
                ori += 360;
            }

            MY_LOGD_IF(mLogLevel, "angle : %d", ori);
            if(ori >= 45 && ori < 135) {
                ori = 90;
            } else if (ori >= 135 && ori < 225) {
                ori = 180;
            } else if (ori >= 225 && ori < 315) {
                ori = 270;
            } else {
                ori = 0;
            }
            MY_LOGD_IF(mLogLevel, "grotation : %d", ori);
            PhoneDevRot = ori + mSensorRot;
            return true;
        } else {
            MY_LOGW_IF(mLogLevel, "magnitude too small, cannot trust");
        }
    }
    return false;
}


MVOID
FdNodeImp::
RunFaceDetection()
{
    Mutex::Autolock _l(mFDRunningLock);
    MINT32 RotateInfo = 0;
    MINT32 ret = false;
    MUINT8* pDstY = NULL;
    MINT32 Duration = 0; //ms, use it to control FD fps
    if(mpSensorProvider != NULL) {
        ret = calculateRotateDegree(RotateInfo);
        MY_LOGD_IF(mLogLevel, "status : %d, after calculateRotateDegree : %d", ret, RotateInfo);
    } else {
        RotateInfo = 360;
    }
    MY_LOGI_IF(mLogLevel, "rotatetion : %d, sensor facing : %d", RotateInfo, mSensorFacing);
    if ( ret )
    {
        if(mSensorFacing == MTK_LENS_FACING_BACK) {
        } else if(mSensorFacing == MTK_LENS_FACING_FRONT) {
            RotateInfo = -RotateInfo;
        } else {
            RotateInfo = 360;
        }
        while(RotateInfo < 0) {
            RotateInfo += 360;
        }
        while(RotateInfo >= 360) {
            RotateInfo -= 360;
        }
        mprvDegree = RotateInfo;
    }
    else
    {
        RotateInfo = mprvDegree;
    }
    MY_LOGD_IF(mLogLevel, "after calculate rot : %d", RotateInfo);
    if(mFDStopped)
        return ;

    #if 0
    if(mp3AHal)
    {
        mp3AHal->setFDEnable(true);
    }
    #endif

/*
        char str_dstfile_0[128];
        FILE *cfptr_1;
        int read_size;
        sprintf(str_dstfile_0, "/system/data/640x480_Y_9_1.raw");
        if((cfptr_1=fopen(str_dstfile_0,"rb")) ==NULL)
        {
            printf("File could not be opened \n");
        }
        else
        {
            read_size = fread(FDImgBuffer, sizeof(unsigned char), 640*480, cfptr_1);
            fclose(cfptr_1);
        }
*/

    int srcWidth = mDupImage.w;
    int srcHeight = mDupImage.h;
    int numFace = 0;
    int FDRet = 0;
    int GSRet = 0;
    MINT32 AEStable = 1;
    mpFDHalObj = mvpFDHalObj[mMasterIndex];
    if(mImageWidth == 0 || mImageHeight == 0) {
        for (size_t i = 0; i < mvpFDHalObj.size(); i++)
        {
            FDRet |= mvpFDHalObj[i]->halFDInit(srcWidth, srcHeight, mFDWorkingBuffer, mFDWorkingBufferSize, MTRUE, mCurrent_mode, LANDMARK_NUM, misSecure == false);
        }
        GSRet |= mpGSHalObj->halGSInit(srcWidth, srcHeight, mFDWorkingBuffer, mFDWorkingBufferSize);
    } else if(mImageWidth != srcWidth || mImageHeight != srcHeight || mCurrent_mode != mPrevSD) {
        for (size_t i = 0; i < mvpFDHalObj.size(); i++)
        {
            FDRet |= mvpFDHalObj[i]->halFDUninit();
            FDRet |= mvpFDHalObj[i]->halFDInit(srcWidth, srcHeight, mFDWorkingBuffer, mFDWorkingBufferSize, MTRUE, mCurrent_mode, LANDMARK_NUM, misSecure == false);
        }
        if(mImageWidth != srcWidth || mImageHeight != srcHeight) {
            GSRet |= mpGSHalObj->halGSUninit();
            GSRet |= mpGSHalObj->halGSInit(srcWidth, srcHeight, mFDWorkingBuffer, mFDWorkingBufferSize);
        }
    }
    if(FDRet || GSRet) {
        MY_LOGW("Init Failed!! FD status : %d, GS status : %d", FDRet, GSRet);
        return ;
    }
    mPrevSD = mCurrent_mode;
    mImageWidth = srcWidth;
    mImageHeight = srcHeight;

    MINT32 isDoFD = 0;

    if (mLastFrameTime != 0)
    {
        Duration = (mDupImage.timestamp - mLastFrameTime) / 1000000;
        mFDCountTime += Duration;
    }
    // FD patch for Cshot ++
    if (Duration <= 0 || mFDCountTime > (mFDPeriod*10))
    {
        // wrong timestamp(maybe zsl mode) or worng count time
        // reset count time for FD work normally
        MY_LOGI("check FD frame timestamp : %" PRId64 , mDupImage.timestamp);
        MY_LOGI("check FD count : %d, duration : %d", mFDCountTime, Duration);
        mFDCountTime = mFDPeriod;
    }
    // FD patch for Cshot --
    if (mFDCountTime >= mFDPeriod)
    {
        isDoFD = 1;
    }
    MY_LOGI_IF(mLogLevel, "FD frame timestamp : %" PRId64, mDupImage.timestamp);
    MY_LOGI_IF(mLogLevel, "FD count : %d, duration : %d", mFDCountTime, Duration);

    mp3AHal = mvp3AHal[mMasterIndex];
    mpIspHal = mvpIspHal[mMasterIndex];

    if (mFdDoCounter == 0) {
        mFdStartTime = mDupImage.timestamp;
    }

    if (isDoFD || mLastFrameTime == 0)
    {

        MY_LOGD_IF(mLogLevel, "halFDDo In.");

        if(mDupImage.planes == 1) {
            if (mPureYBuf == nullptr)
            {
                mPureYBuf = new unsigned char[640*480*2];
            }
            pDstY = mPureYBuf;
            if (mCurrent_mode == HAL_FD_MODE_SD || mGDEnable || mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW40) {
                mpFDHalObj->halFDYUYV2ExtractY(pDstY, (MUINT8 *)mDupImage.AddrY, srcWidth, srcHeight);
            }
        } else {
            pDstY = (MUINT8 *)mDupImage.AddrY;
        }

        if(mp3AHal)
        {
            mp3AHal->send3ACtrl(E3ACtrl_GetIsAEStable, (MINTPTR)&AEStable, 0);
            MY_LOGD_IF(mLogLevel, "AE Stable : %d", AEStable);
        }

        // Do FD
        struct FD_Frame_Parameters Param;
        Param.pScaleImages = NULL;
        Param.pRGB565Image = mDupImage.AddrY;
        Param.pPureYImage  = pDstY;
        Param.pImageBufferVirtual = misSecure ? nullptr : (MUINT8 *)mDupImage.AddrY;
        Param.pImageBufferPhyP0 = misSecure ? (MUINT8 *)mDupImage.SecureHandle : (MUINT8 *)mDupImage.PAddrY;
        if (mSecureMemType != SecType::mem_normal) Param.pImageBufferPhyP0 = (MUINT8*)mpSecureImageBuffer->getBufPA(0);
        Param.pImageBufferPhyP1 = NULL;
        Param.pImageBufferPhyP2 = NULL;
        Param.Rotation_Info = RotateInfo;
        Param.SDEnable = (mCurrent_mode == HAL_FD_MODE_SD);
        Param.AEStable = AEStable;
        // use padding to pass real fd yuv size
        Param.padding_w = mRealFDYUVSize.w;
        Param.padding_h = mRealFDYUVSize.h;
        Param.gammaType = mDupImage.gammaIndex;
        Param.LvValue = mDupImage.lvdata;
        Param.MagicNum = mMagicNo;
        Param.FD_Y = mDupImage.FD_Y;
        FDRet = mpFDHalObj->halFDDo(Param);
        if(FDRet) {
            MY_LOGW("halFDDo Failed!! FD status : %d", FDRet);
            for (size_t i = 0; i < mvpFDHalObj.size(); i++)
            {
                mvpFDHalObj[i]->halFDUninit();
                mvpFDHalObj[i]->halFDInit(srcWidth, srcHeight, mFDWorkingBuffer, mFDWorkingBufferSize, misSecure == false, mCurrent_mode, LANDMARK_NUM, misSecure == false);
            }
            mpGSHalObj->halGSUninit();
            mImageWidth = 0;
            mImageHeight = 0;
            return ;
        }
        mFdDoCounter ++;
        if (mFdDoCounter == 60 || mbNeedReCount) {
            mFdEndTime = (mbNeedReCount)? mLastFrameTime : mDupImage.timestamp;
            if (CamAutoTestLogger::checkEnable() && (mFdEndTime > mFdStartTime)) {
                float fps = (float) mFdDoCounter * 1000.0f / ((mFdEndTime - mFdStartTime) / 1000000);
                MY_LOGCAT("sensor_index:%d FD_FPS:%f", getOpenId(), fps);
            }
            mbNeedReCount = false;
            mFdDoCounter = 0;
        }
        mFDCountTime = (mFDCountTime < mFDPeriod)? 0:(mFDCountTime - mFDPeriod);

        MY_LOGD_IF(mLogLevel, "halFDDo Out.");
    }
    if (mDupImage.timestamp > mLastFrameTime)
    {
        mLastFrameTime = mDupImage.timestamp;
    }

    {
        Mutex::Autolock _l(mResultLock);
        // reset face number
        mpDetectedFaces->number_of_faces = 0;

        numFace = mpFDHalObj->halFDGetFaceResult(mpDetectedFaces, 1);
        // apply EIS margin and sensor size if use P1 direct FD yuv
        if (misDirectYuv) {
            for (int i = 0; i < mpDetectedFaces->number_of_faces; i++) {
                for (int j = 0; j < 4; j++) {
                    // if (j % 2 == 0) {
                    //     mpDetectedFaces->faces[i].rect[j] = mpDetectedFaces->faces[i].rect[j] * mEISScale_W / 100;
                    // } else {
                    //     mpDetectedFaces->faces[i].rect[j] = mpDetectedFaces->faces[i].rect[j] * mEISScale_H / 100;
                    // }
                    if (misVsdof) {
                        mpDetectedFaces->faces[i].rect[j] = mpDetectedFaces->faces[i].rect[j] * mActiveArray.s.w / mSensorSize.w;
                    }
                    if (mFSCEnable && !mIsEISOn) {
                        mpDetectedFaces->faces[i].rect[j] = mpDetectedFaces->faces[i].rect[j] * mFSCMargin / 100;
                    }
                }
            }
        }

        MY_LOGD_IF(mLogLevel, "NumFace = %d, ", numFace);

        {
            MUINT32 faceArea = 0;
            for(int i = 0; i < mpDetectedFaces->number_of_faces; i++)
            {
                faceArea += (mpDetectedFaces->faces[i].rect[2] - mpDetectedFaces->faces[i].rect[0]) * (mpDetectedFaces->faces[i].rect[3] - mpDetectedFaces->faces[i].rect[1]);
            }
            mFaceRatio = (float)(faceArea) / (2000.0*2000.0);
            MY_LOGD_IF(mLogLevel, "faceArea = %d, mFaceRatio = %f", faceArea, mFaceRatio);
        }

        mpDetected3AFaces->number_of_faces = 0;
        numFace = mpFDHalObj->halFDGetFaceResult(mpDetected3AFaces, 0);
        MY_LOGI_IF(mLogLevel, "3A NumFace = %d, ", numFace);

        if(mCurrent_mode == HAL_FD_MODE_SD) {
            mSD_Result = mpFDHalObj->halSDGetSmileResult();
            MY_LOGD_IF(mLogLevel, "Smile Result:%d", mSD_Result);
        }
        mpDetected3AFaces->ImgWidth = mpDetectedFaces->ImgWidth = srcWidth;
        mpDetected3AFaces->ImgHeight = mpDetectedFaces->ImgHeight = srcHeight;
        mpDetected3AFaces->timestamp = mpDetectedFaces->timestamp = mDupImage.timestamp;
        // for 3A
        mpDetected3AFaces->magicNo = mpDetectedFaces->magicNo = mMagicNo;
        MY_LOGD_IF(mLogLevel, "[Before 3A] magicNo: %d", mpDetected3AFaces->magicNo);
        #if (1==STEREO_SUPPORTED)
        StereoFDProxy::setFaces(mpDetectedFaces);
        #endif
        // convert FD result to meet hal3 type and send to 3A
        convertResult(mpDetectedFaces, true);
        convertResult(mpDetected3AFaces, false);

        mFirstUpdate = 1;
    }

    // Do GD
    if(numFace>0)
    {
        doGestureDetect(pDstY, RotateInfo);
        getGestureResult();
    }

    mResultCond.broadcast();
}


MVOID
FdNodeImp::
setFDLock(MBOOL val)
{
    Mutex::Autolock _l(mFDLock);

    mIsFDBusy = val;
    return;
}

MVOID*
FdNodeImp::
FDHalThreadLoop(MVOID* arg)
{
    FdNodeImp *_FDNode = reinterpret_cast<FdNodeImp*>(arg);
    while(1) {
        sem_wait(&_FDNode->semFD);
        if(_FDNode->mStopFD) break;
        _FDNode->RunFaceDetection();
        _FDNode->setFDLock(MFALSE);
    }
    return NULL;
}


/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
init(InitParams const& rParams)
{
    CAM_TRACE_NAME("FdNode init");
    MY_LOGD("FdNode Initial !!!");
    {
        RWLock::AutoWLock _l(mConfigRWLock);
        //
        mOpenId  = rParams.openId;
        mNodeId  = rParams.nodeId;
        mNodeName= rParams.nodeName;
    }

    mp3AHal = MAKE_Hal3A(getOpenId(), getNodeName());
    mpIspHal = MAKE_HalISP(getOpenId(), getNodeName());

    mvp3AHal.push_back(mp3AHal);
    mvpIspHal.push_back(mpIspHal);
    mvSensorId.push_back(mOpenId);

    for (int i = 0; i < rParams.subOpenIdList.size(); i++)
    {
        mvp3AHal.push_back(MAKE_Hal3A(rParams.subOpenIdList[i], getNodeName()));
        mvpIspHal.push_back(MAKE_HalISP(rParams.subOpenIdList[i], getNodeName()));
        mvSensorId.push_back(rParams.subOpenIdList[i]);
    }

    mFDStopped = MFALSE;

    mpDetectedFaces = new MtkCameraFaceMetadata;
    if ( NULL != mpDetectedFaces )
    {
        mpDetectedFaces->number_of_faces = 0;
    } else {
        MY_LOGE("Fail to allocate FaceMetadata buffer");
        return UNKNOWN_ERROR;
    }

    mpDetected3AFaces = new MtkCameraFaceMetadata;
    if ( NULL != mpDetected3AFaces )
    {
        mpDetected3AFaces->number_of_faces = 0;
    } else {
        MY_LOGE("Fail to allocate GSMetadata buffer");
        return UNKNOWN_ERROR;
    }

    {
        auto fdWriter = IFDContainer::createInstance(LOG_TAG, IFDContainer::eFDContainer_Opt_Write);
        MY_LOGI_IF(mLogLevel, "claer all fd container data");
        if (fdWriter != NULL)
        {
            fdWriter->clearInfo();
        }
        else
        {
            MY_LOGW("get container FD buffer null");
        }
    }

    mFDWorkingBufferSize = 409600; //400K: 1024*400

    mpFDHalObj = halFDBase::createInstance(HAL_FD_OBJ_FDFT_SW);
    if ( mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW40 )
    {
        mFDWorkingBufferSize = 4194304; //4M: 1024*1024*4
    }
    mFDWorkingBuffer = new unsigned char[mFDWorkingBufferSize];
    mvpFDHalObj.push_back(mpFDHalObj);
    for (size_t i = 0; i < rParams.subOpenIdList.size(); i++)
    {
        auto pFDHal = halFDBase::createInstance(HAL_FD_OBJ_FDFT_SW);
        mvpFDHalObj.push_back(pFDHal);
    }
    mImageWidth = 0;
    mImageHeight = 0;

    mpGSHalObj = halGSBase::createInstance(HAL_GS_OBJ_SW);

    mFDProcInited = MFALSE;

    mFirstUpdate = 0;

    mSensorMode = -1;

    // +++ FD Async mode
    if (mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW50 || mForceAsyncMode)
    {
        mIsFDBusy = MFALSE;
        mStopFD = MFALSE;
        mAllocator = IImageBufferAllocator::getInstance();
        IImageBufferAllocator::ImgParam imgParam(640*640*2,0);
        mDupImage.pImg = mAllocator->alloc("FDTempBuf", imgParam);
        if ( mDupImage.pImg.get() == 0 )
        {
            MY_LOGE("NULL Buffer\n");
            return UNKNOWN_ERROR;
        }
        if ( !mDupImage.pImg->lockBuf( "FDTempBuf", (eBUFFER_USAGE_HW_CAMERA_READ | eBUFFER_USAGE_SW_MASK)) )
        {
            MY_LOGE("lock Buffer failed\n");
            return UNKNOWN_ERROR;
        }
        MY_LOGD("allocator buffer : %" PRIXPTR "", mDupImage.pImg->getBufVA(0));
        mDupImage.pImg->syncCache(eCACHECTRL_INVALID);
        mDupImage.AddrY = (MUINT8 *)mDupImage.pImg->getBufVA(0);
        mDupImage.AddrU = mDupImage.AddrY + 640*640;
        mDupImage.AddrV = mDupImage.AddrU + ((640*640) >> 2);

        sem_init(&semFD, 0, 0);
        pthread_create(&mFDHalThread, NULL, FDHalThreadLoop, this);
        pthread_setname_np(mFDHalThread, "FDExecThread");
    }
    // --- FD Async mode

    if( mpSensorProvider == NULL)
    {
        mpSensorProvider = SensorProvider::createInstance(LOG_TAG);

        if( mpSensorProvider != NULL)
        {
            MUINT32 interval = 60;

            if( mpSensorProvider->enableSensor(SENSOR_TYPE_ACCELERATION, interval))
            {
                CAM_ULOGMD("enable SensorProvider success");
            }
            else
            {
                CAM_ULOGME("Enable SensorProvider fail");
            }
        }
    }
    mprvDegree = 360;
    mLastFrameTime = 0;
    mFDCountTime = 0;

    if(mp3AHal && mForceFDEnable)
    {
        mp3AHal->setFDEnable(true);
    }

    mbInited = MTRUE;

    MERROR err = run("FdNodeImp::init");

    //
    {
        //sp<IMetadataProvider> pMetadataProvider = IMetadataProvider::create(getOpenId());
        sp<IMetadataProvider> pMetadataProvider = NSMetadataProviderManager::valueFor(getOpenId());
        if( ! pMetadataProvider.get() ) {
            MY_LOGE(" ! pMetadataProvider.get() ");
            return DEAD_OBJECT;
        }

        IMetadata static_meta = pMetadataProvider->getMtkStaticCharacteristics();
        {
            IMetadata::IEntry active_array_entry = static_meta.entryFor(MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION);
            if( !active_array_entry.isEmpty() ) {
                mActiveArray = active_array_entry.itemAt(0, Type2Type<MRect>());
                MY_LOGD_IF(1,"FD Node: active array(%d, %d, %dx%d)",
                        mActiveArray.p.x, mActiveArray.p.y, mActiveArray.s.w, mActiveArray.s.h);
            } else {
                MY_LOGE("no static info: MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION");
                return UNKNOWN_ERROR;
            }
        }
        {
            IMetadata::IEntry facing_entry = static_meta.entryFor(MTK_SENSOR_INFO_FACING);
            mSensorFacing = -1;
            if( !facing_entry.isEmpty() ) {
                mSensorFacing = facing_entry.itemAt(0, Type2Type<MUINT8>());
                MY_LOGD_IF(1,"FD Node: sensor facing : %d", mSensorFacing);
            } else {
                MY_LOGE("no static info: MTK_SENSOR_INFO_FACING");
                return UNKNOWN_ERROR;
            }
        }
        {
            IMetadata::IEntry rot_entry = static_meta.entryFor(MTK_SENSOR_INFO_ORIENTATION);
            mSensorRot = 0;
            if( !rot_entry.isEmpty() ) {
                mSensorRot = rot_entry.itemAt(0, Type2Type<MINT32>());
                MY_LOGD_IF(1,"FD Node: sensor orientation : %d", mSensorRot);
                if(mSensorFacing == MTK_LENS_FACING_BACK) {
                    mSensorRot -= 90;
                } else if(mSensorFacing == MTK_LENS_FACING_FRONT) {
                    mSensorRot -= 270;
                    #if (1==HORIZONTAL_SCREEN)
                    mSensorRot += 90;
                    #endif
                } else {
                    mSensorRot = 0;
                }
            } else {
                MY_LOGE("no static info: MTK_SENSOR_INFO_ORIENTATION");
                return UNKNOWN_ERROR;
            }
        }

    }
    //

    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
uninit()
{
    flush();
    //
    {
        RWLock::AutoWLock _l(mConfigRWLock);
        //
        mpOutMetaStreamInfo_Result = 0;
        mpInMetaStreamInfo_Request = 0;
        mpInImageStreamInfo_Yuv = 0;
        mpHalImage_P1_Rrzo = 0;
    }

    requestExit();

    join();

    // +++ FD Async mode
    if (mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW50 || mForceAsyncMode)
    {
        mStopFD = MTRUE;
        sem_post(&semFD);
        pthread_join(mFDHalThread, NULL);
        sem_destroy(&semFD);
        if(mDupImage.pImg != NULL && mAllocator != NULL) {
            mDupImage.pImg->unlockBuf("FDTempBuf");
            mAllocator->free(mDupImage.pImg.get());
            mDupImage.pImg = NULL;
        }
    } else {
        mDupImage.pImg = NULL;
    }
    // --- FD Async mode

    {
        // Delete face metadata buffer
        if ( mpDetectedFaces != NULL )
        {
            delete mpDetectedFaces;
            mpDetectedFaces = NULL;
        }

        if ( mpDetected3AFaces != NULL )
        {
            delete mpDetected3AFaces;
            mpDetected3AFaces = NULL;
        }
    }

    FREE_DATA(mFDImgBuffer)
    FREE_DATA(mFDWorkingBuffer)
    FREE_DATA(mPureYBuf)

    for (size_t i = 0; i < mvpFDHalObj.size(); i++)
    {
        if (mvpFDHalObj[i] != nullptr) {
          mvpFDHalObj[i]->halFDUninit();
          mvpFDHalObj[i]->destroyInstance();
        } else {
          MY_LOGW("mvpFDHalObj[%lu] is nullptr", i);
        }
    }
    mpFDHalObj = NULL;
    mvpFDHalObj.clear();


    if(mpGSHalObj != NULL) {
        mpGSHalObj->halGSUninit();
        mpGSHalObj->destroyInstance();
        mpGSHalObj = NULL;
    }

    mImageWidth = 0;
    mImageHeight = 0;

    if(mFDProcInited) {
        #if 0
        if(mpSensorListener != NULL)
        {
            MY_LOGD("mpSensorListener uninit");
            mpSensorListener->disableSensor(SensorListener::SensorType_Acc);
            mpSensorListener->destroyInstance();
            mpSensorListener = NULL;
        }
        #endif
        mFDProcInited = MFALSE;
    }

    if(mp3AHal && mForceFDEnable)
    {
        mp3AHal->setFDEnable(false);
    }

    for (int i = 0; i < mvSensorId.size(); i++)
    {
        if (mvp3AHal[i])
        {
            mvp3AHal[i]->destroyInstance(getNodeName());
        }
        if (mvpIspHal[i])
        {
            mvpIspHal[i]->destroyInstance(getNodeName());
        }
    }
    mvp3AHal.clear();
    mvpIspHal.clear();
    mvSensorId.clear();
    mvpPhysicalStreamsInfo.clear();
    mp3AHal = NULL;
    mpIspHal = NULL;

    if( mpSensorProvider != NULL)
    {
        mpSensorProvider->disableSensor(SENSOR_TYPE_ACCELERATION);
        mpSensorProvider = NULL;
    }

    if ( mpSecureImageBuffer != nullptr)
    {
        mpSecureImageBuffer->unlockBuf("SecureFDTempBuf");
    }

    mbInited = MFALSE;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
config(ConfigParams const& rParams)
{
    CAM_TRACE_NAME("FdNode config");
    if  ( ! rParams.pInAppMeta.get() ) {
        return BAD_VALUE;
    }
    //if  ( ! rParams.pInHalMeta.get() ) {
    //    return BAD_VALUE;
    //}
    if  ( ! rParams.pOutAppMeta.get() ) {
        return BAD_VALUE;
    }
    //if  ( ! rParams.pOutHalMeta.get() ) {
    //    return BAD_VALUE;
    //}
//    if  (  0 == rParams.vInImage.size() ) {
//        return BAD_VALUE;
//    }
    //if  (  0 == rParams.vOutImage.size() ) {
    //    return BAD_VALUE;
    //}
    //
    //
    RWLock::AutoWLock _l(mConfigRWLock);
    //
    mpInMetaStreamInfo_Request = rParams.pInAppMeta;
    mpInMetaStreamInfo_P2Result = rParams.pInHalMeta;
    mpInImageStreamInfo_Yuv    = rParams.vInImage;
    mpOutMetaStreamInfo_Result = rParams.pOutAppMeta;
    mPreviewSize.w = rParams.preview_W;
    mPreviewSize.h = rParams.preview_H;
    mpHalImage_P1_Rrzo = rParams.P1_Rrzo;
    mIsEISOn = rParams.isEisOn;

    if (mIsEISOn) {
        mpEISCache = EISCache::getInstance();
        mpEISCache->init();
    }

    misDirectYuv = rParams.isDirectYuv;
    misVsdof = rParams.isVsdof;
    mFSCEnable = rParams.fscEnable;

    for (auto streamInfo : rParams.vPhysicalStreamsInfo)
    {
        if (streamInfo.pOutAppPhysicalMeta == nullptr)
        {
            MY_LOGE("pOutAppPhysicalMeta is null");
            continue;
        }
        mvpPhysicalStreamsInfo.push_back(streamInfo);
    }
    misVideoMode = rParams.isVideoMode;

    // set to ISP
    MUINT32 FDSource = misDirectYuv ? 0 : 1;
    for (int i = 0; i < mvpIspHal.size(); i++)
    {
        mvpIspHal[i]->sendIspCtrl(EISPCtrl_SetFdSource, (MINTPTR)(&FDSource), 0);
    }

    if (mpInImageStreamInfo_Yuv != nullptr)
    {
        misSecure = mpInImageStreamInfo_Yuv->isSecure();
        MY_LOGI("is FD in secure mode : %d", misSecure);
        if (mSecureMemType != SecType::mem_normal)
        {
            misSecure = true;
            IImageBufferAllocator::ImgParam imgParam(640*640,0);
            ISecureImageBufferHeap::AllocExtraParam extraParam(0,1,0,MFALSE,mSecureMemType);
            mpSecureImageBufferHeap = ISecureImageBufferHeap::create(
                                        mpInImageStreamInfo_Yuv->getStreamName(),
                                        imgParam,
                                        extraParam,
                                        MFALSE
                                    );
            if (mpSecureImageBufferHeap == nullptr) {
                MY_LOGE("Secure Image Buffer Heap create fail");
                return BAD_VALUE;
            }
            mpSecureImageBuffer = mpSecureImageBufferHeap->createImageBuffer();
            if (mpSecureImageBuffer == nullptr) {
                MY_LOGE("Secure Image Buffer create fail");
                return BAD_VALUE;
            }
            MUINT const usage = eBUFFER_USAGE_SW_READ_OFTEN |
                eBUFFER_USAGE_HW_CAMERA_READWRITE
                ;
            mpSecureImageBuffer->lockBuf("SecureFDTempBuf", usage);
            MY_LOGD("Property set, allocate new secure buffer to debug");
        }
    }

    //
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
queue(
    android::sp<IPipelineFrame> pFrame
)
{
    Mutex::Autolock _l(mRequestQueueLock);

    MY_LOGD_IF(mLogLevel, "[queue] In frameNo : %d", pFrame->getFrameNo());
    //
    //  Make sure the request with a smaller frame number has a higher priority.
    Que_T::iterator it = mRequestQueue.end();
    for (; it != mRequestQueue.begin(); ) {
        --it;
        if  ( 0 <= (MINT32)(pFrame->getFrameNo() - (*it)->getFrameNo()) ) {
            ++it;   //insert(): insert before the current node
            break;
        }
    }
    mRequestQueue.insert(it, pFrame);
    mRequestQueueCond.broadcast();
    //
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
FdNodeImp::
waitForRequestDrained()
{
    //
    Mutex::Autolock _l(mRequestQueueLock);
    if( !mbRequestDrained ) {
        MY_LOGD("wait for request drained");
        mbRequestDrainedCond.wait(mRequestQueueLock);
    }
    //
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
flush(android::sp<IPipelineFrame> const &pFrame)
{
    return BaseNode::flush(pFrame);
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
flush()
{
    MY_LOGD("+");
    //
    // 1. clear requests
    {
        Mutex::Autolock _l(mRequestQueueLock);
        //
        Que_T::iterator it = mRequestQueue.begin();
        while ( it != mRequestQueue.end() ) {
            BaseNode::flush(*it);
            it = mRequestQueue.erase(it);
        }
    }
    //
    // 2. wait enque thread
    waitForRequestDrained();
    //
    // 3. clear working buffer

    MY_LOGD("-");

    //return INVALID_OPERATION;
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
FdNodeImp::
onDequeRequest(
    android::sp<IPipelineFrame>& rpFrame
)
{
    Mutex::Autolock _l(mRequestQueueLock);

     MY_LOGD_IF(mLogLevel, "[onDequeRequest] In++");
    //
    //  Wait until the queue is not empty or this thread will exit.

    mbDoFD = MFALSE;

    while ( mRequestQueue.empty() && ! exitPending() )
    {
        // set dained flag
        mbRequestDrained = MTRUE;
        mbRequestDrainedCond.signal();
        //
        mbDoFD = MTRUE;
        status_t status = mRequestQueueCond.wait(mRequestQueueLock);
        MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_1");

        if  ( OK != status ) {
            MY_LOGW(
                "wait status:%d:%s, mRequestQueue.size:%zu, exitPending:%d",
                status, ::strerror(-status), mRequestQueue.size(), exitPending()
            );
        }
    }

    //
    if  ( exitPending() ) {
        MY_LOGW("[exitPending] mRequestQueue.size:%zu", mRequestQueue.size());
        return DEAD_OBJECT;
    }
    if(mRequestQueue.size() == 1) {
        mbDoFD = MTRUE;
    }
    //
    //  Here the queue is not empty, take the first request from the queue.
    MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_3 RequestQueue Size = %zu, IsDoFD : %d", mRequestQueue.size(), mbDoFD);
    mbRequestDrained = MFALSE;
    rpFrame = *mRequestQueue.begin();
    mRequestQueue.erase(mRequestQueue.begin());
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
// Ask this object's thread to exit. This function is asynchronous, when the
// function returns the thread might still be running. Of course, this
// function can be called from a different thread.
void
FdNodeImp::
requestExit()
{
    MY_LOGD("+");
    Mutex::Autolock _l(mRequestQueueLock);

    Thread::requestExit();

    mRequestQueueCond.signal();
    MY_LOGD("-");
}


/******************************************************************************
 *
 ******************************************************************************/
// Good place to do one-time initializations
status_t
FdNodeImp::
readyToRun()
{
    ::prctl(PR_SET_NAME, (unsigned long)"Cam@FdNodeImp", 0, 0, 0);
    //
    //mi4ThreadId = ::gettid();
    //  thread policy & priority
    //  Notes:
    //      Even if pthread_create() with SCHED_OTHER policy, a newly-created thread
    //      may inherit the non-SCHED_OTHER policy & priority of the thread creator.
    //      And thus, we must set the expected policy & priority after a thread creation.
    //MY_LOGW("Not Implement !!! Need set thread priority & policy");

    int const policy    = SCHED_OTHER;
    int const priority  = 0;
    //
    //
    struct sched_param sched_p;
    ::sched_getparam(0, &sched_p);
    sched_p.sched_priority = priority;  //  Note: "priority" is nice value
    sched_setscheduler(0, policy, &sched_p);
    //setpriority(PRIO_PROCESS, 0, priority);
    ::sched_getparam(0, &sched_p);
    //
    //
    MY_LOGD(
        "Tid: %d, policy: %d, priority: %d"
        , ::gettid(), ::sched_getscheduler(0)
        , sched_p.sched_priority
    );

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
FdNodeImp::
threadLoop()
{
    sp<IPipelineFrame> pFrame;
    if  (
            OK == onDequeRequest(pFrame)
        &&  pFrame != 0
        )
    {
        onProcessFrame(pFrame);
        return true;
    }

    MY_LOGD("FDnode exit threadloop");

    return  false;
}

/******************************************************************************
 *
 ******************************************************************************/
MINT32
FdNodeImp::
doGestureDetect(
    MUINT8* pImage __unused,
    MINT32 RotateInfo __unused
)
{
    #if 0
    if(mGDEnable) {
        if(mpGSHalObj == NULL || mpDetectedFaces == NULL || mpDetectedGestures == NULL || pImage == NULL)
            return -1;
        mpGSHalObj->halGSDo(pImage, RotateInfo);
    }
    #endif
    return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
MINT32
FdNodeImp::
getGestureResult(
)
{
    #if 0
    MINT32 GD_Result;
    if(mGDEnable) {
        Mutex::Autolock _l(mResultLock);
        if(mpDetectedFaces == NULL || mpDetectedGestures == NULL)
            return -1;
        GD_Result = mpGSHalObj->halGSGetGestureResult(mpDetectedGestures);
        /////////////////////////////////////////////////////////////////////
        // cpchen: filter GS results with FD results: no gesture inside face regions
        /////////////////////////////////////////////////////////////////////
        bool bEnableGSFilterWithFD = true;
        float fIntersetAreaRatio = 0.25f;
        float fMaxRelativeRatio = 3.0f;
        if( bEnableGSFilterWithFD )
        {
           int newCount = 0;
           for (int gi = 0; gi < mpDetectedGestures->number_of_faces; ++gi)
           {
              // gesture rectangle
              int gx0 = mpDetectedGestures->faces[gi].rect[0];
              int gy0 = mpDetectedGestures->faces[gi].rect[1];
              int gx1 = mpDetectedGestures->faces[gi].rect[2];
              int gy1 = mpDetectedGestures->faces[gi].rect[3];
              int garea = (gx1 - gx0) * (gy1 - gy0);

              bool bIsOverlap = false;
              for (int fi = 0; fi < mpDetectedFaces->number_of_faces; ++fi)
              {
                 // face rectangle
                 int fx0 = mpDetectedFaces->faces[fi].rect[0];
                 int fy0 = mpDetectedFaces->faces[fi].rect[1];
                 int fx1 = mpDetectedFaces->faces[fi].rect[2];
                 int fy1 = mpDetectedFaces->faces[fi].rect[3];
                 int farea = (fx1 - fx0) * (fy1 - fy0);

                 // interset rectangle
                 int ix0 = max(gx0, fx0);
                 int iy0 = max(gy0, fy0);
                 int ix1 = min(gx1, fx1);
                 int iy1 = min(gy1, fy1);
                 int iarea = 0;
                 if ((ix1 > ix0) && (iy1 > iy0))
                    iarea = (ix1 - ix0) * (iy1 - iy0);

                 // overlap determination
                 float minArea = min(garea, farea);
                 float overlapRatio = (float)iarea / minArea;
                 float relativeRatio = (float)farea / garea;

                 if (overlapRatio >= fIntersetAreaRatio)
                 {
                    bIsOverlap = true;
                    break;
                 }
              } // end of for each face rectangle

              // skip overlapped gesture rectangles, move non-overlapped gesture rectangles forward
              if (!bIsOverlap)
              {
                 mpDetectedGestures->faces[newCount].rect[0] = mpDetectedGestures->faces[gi].rect[0];
                 mpDetectedGestures->faces[newCount].rect[1] = mpDetectedGestures->faces[gi].rect[1];
                 mpDetectedGestures->faces[newCount].rect[2] = mpDetectedGestures->faces[gi].rect[2];
                 mpDetectedGestures->faces[newCount].rect[3] = mpDetectedGestures->faces[gi].rect[3];
                 mpDetectedGestures->faces[newCount].score = mpDetectedGestures->faces[gi].score;
                 mpDetectedGestures->faces[newCount].id = mpDetectedGestures->faces[gi].id;
                 mpDetectedGestures->faces[newCount].left_eye[0] = mpDetectedGestures->faces[gi].left_eye[0];
                 mpDetectedGestures->faces[newCount].left_eye[1] = mpDetectedGestures->faces[gi].left_eye[1];
                 mpDetectedGestures->faces[newCount].right_eye[0] = mpDetectedGestures->faces[gi].right_eye[0];
                 mpDetectedGestures->faces[newCount].right_eye[1] = mpDetectedGestures->faces[gi].right_eye[1];
                 mpDetectedGestures->faces[newCount].mouth[0] = mpDetectedGestures->faces[gi].mouth[0];
                 mpDetectedGestures->faces[newCount].mouth[1] = mpDetectedGestures->faces[gi].mouth[1];
                 mpDetectedGestures->posInfo[newCount].rop_dir = mpDetectedGestures->posInfo[gi].rop_dir;
                 mpDetectedGestures->posInfo[newCount].rip_dir = mpDetectedGestures->posInfo[gi].rip_dir;
                 ++newCount;
              }
           }
           // number of gesture rectangles after filtering
           mpDetectedGestures->number_of_faces = newCount;
           GD_Result = newCount;

           // debug message
           if (GD_Result == 0)
              MY_LOGD("Scenario GD: Gesture detected but filtered out by face!!!");
        }
        /////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////
        // cpchen: face is a prerequiste of gesture shot, no face no gesture shot
        /////////////////////////////////////////////////////////////////////
        bool bEnableGSPrerequisteWithFD = true;
        if (bEnableGSPrerequisteWithFD && mpDetectedFaces->number_of_faces == 0)
        {
           mpDetectedGestures->number_of_faces = 0;
           GD_Result = 0;

           // debug message
           MY_LOGD("Scenario GD: Gesture detected but no face!");
        }
        /////////////////////////////////////////////////////////////////////

        MY_LOGD("Scenario GD Result: %d",GD_Result );
    }
    #endif
    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
MINT32
FdNodeImp::
tryToUpdateOldData(
    IMetadata*              pOutMetadataResult,
    MINT32                  FDMode
)
{
#define FDTOLERENCE (600) //ms
    timespec now;
    MINT32 diffms = 0;
    clock_gettime(CLOCK_MONOTONIC, &now);
    diffms = ((now.tv_sec - gUpdateTime.tv_sec)*1000) + ((now.tv_nsec - gUpdateTime.tv_nsec)/1000000);
    if ((gUpdateTime.tv_sec == 0 && gUpdateTime.tv_nsec == 0) || diffms > FDTOLERENCE)
    {
        MY_LOGD("time diffms is large : %d", diffms);
        return -1;
    }
    auto fdReader = IFDContainer::createInstance(LOG_TAG,  IFDContainer::eFDContainer_Opt_Read);
    auto fdData = fdReader->queryLock();
    MY_LOGD_IF(mLogLevel, "get FD data : %lu", fdData.size());
    if (fdData.size() > 0)
    {
        auto fdChunk = fdData.back();
        if (CC_LIKELY( fdChunk != nullptr ) && fdChunk->facedata.number_of_faces > 0)
        {
            MY_LOGD_IF(mLogLevel, "Number_of_faces: %d",fdChunk->facedata.number_of_faces);
            // face,  15 is the max number of faces
            IMetadata::IEntry face_rect_tag(MTK_STATISTICS_FACE_RECTANGLES);
            for (size_t i = 0 ; i < fdChunk->facedata.number_of_faces; i++)
            {
                MRect face_rect;
                face_rect.p.x = fdChunk->facedata.faces[i].rect[0];  //Left
                face_rect.p.y = fdChunk->facedata.faces[i].rect[1];  //Top
                face_rect.s.w = fdChunk->facedata.faces[i].rect[2];  //Right
                face_rect.s.h = fdChunk->facedata.faces[i].rect[3];  //Bottom
                face_rect_tag.push_back(face_rect, Type2Type<MRect>());
            }
            pOutMetadataResult->update(MTK_STATISTICS_FACE_RECTANGLES, face_rect_tag);

            IMetadata::IEntry face_landmark_tag(MTK_STATISTICS_FACE_LANDMARKS);
            MINT32 face_landmark;
            MINT32 face_id;
            MUINT8 face_score;
            for(size_t i = 0; i < fdChunk->facedata.number_of_faces; i++)
            {
                face_landmark = (FDMode == MTK_STATISTICS_FACE_DETECT_MODE_FULL) ? (fdChunk->facedata.faces[i].left_eye[0]) : 0;  //left_eye_x
                face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
                face_landmark = (FDMode == MTK_STATISTICS_FACE_DETECT_MODE_FULL) ? (fdChunk->facedata.faces[i].left_eye[1]) : 0;  //left_eye_y
                face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
                face_landmark = (FDMode == MTK_STATISTICS_FACE_DETECT_MODE_FULL) ? (fdChunk->facedata.faces[i].right_eye[0]) : 0;  //right_eye_x
                face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
                face_landmark = (FDMode == MTK_STATISTICS_FACE_DETECT_MODE_FULL) ? (fdChunk->facedata.faces[i].right_eye[1]) : 0;  //right_eye_y
                face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
                face_landmark = (FDMode == MTK_STATISTICS_FACE_DETECT_MODE_FULL) ? (fdChunk->facedata.faces[i].mouth[0]) : 0;  //mouth_x
                face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
                face_landmark = (FDMode == MTK_STATISTICS_FACE_DETECT_MODE_FULL) ? (fdChunk->facedata.faces[i].mouth[1]) : 0;  //mouth_y
                face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
            }
            pOutMetadataResult->update(MTK_STATISTICS_FACE_LANDMARKS, face_landmark_tag);

            IMetadata::IEntry face_id_tag(MTK_STATISTICS_FACE_IDS);
            for(size_t i = 0; i < fdChunk->facedata.number_of_faces; i++)
            {
                face_id = fdChunk->facedata.faces[i].id;
                face_id_tag.push_back(face_id, Type2Type<MINT32>());
            }
            pOutMetadataResult->update(MTK_STATISTICS_FACE_IDS, face_id_tag);

            IMetadata::IEntry face_score_tag(MTK_STATISTICS_FACE_SCORES);
            for(size_t i = 0; i < fdChunk->facedata.number_of_faces; i++)
            {
                face_score = fdChunk->facedata.faces[i].score;
                face_score_tag.push_back(face_score, Type2Type<MUINT8>());
            }
            pOutMetadataResult->update(MTK_STATISTICS_FACE_SCORES, face_score_tag);
            fdReader->queryUnlock(fdData);
            return 0;

        }
    }
    fdReader->queryUnlock(fdData);
    return -1;
}

MVOID
FdNodeImp::
ReturnCustomResult(
                    IMetadata*              pOutMetadataResult __unused,
                    MtkCameraFaceMetadata*  pFaceMeta __unused,
                    float                   FaceRatio __unused
)
{
    // return custom vendor tag result
    // sample 1: return one value
    /*
     * IMetadata::IEntry tag1(CUSTOM_TAG1);
     * tag1.push_back(<data>, Type2Type<MINT32>()); // MINT32 is data type
     * pOutMetadataResult->update(CUSTOM_TAG1, tag1);
     * */

     // sample 2 : return a array
     /*
     * IMetadata::IEntry tag2(CUSTOM_TAG2);
     * forloop
     * {
     *     tag2.push_back(<data>, Type2Type<MINT32>()); // MINT32 is data type
     * }
     * pOutMetadataResult->update(CUSTOM_TAG2, tag2);
     * */
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
FdNodeImp::
ReturnFDResult(
    IMetadata*              pOutMetadataResult,
    IMetadata*              pInpMetadataRequest,
    IMetadata*              pInpMetadataHalResult __unused
)
{
    MRect scalerCropRegion;
    MRect cropRegion;
    MRect face_rect;
    MINT32 face_landmark;
    MINT32 face_id;
    MUINT8 face_score;
    MINT32 FDEn = 0;
    MINT32 GDEn = 0;
    MINT32 SDEn = 0;
    MUINT8 FDMode = MTK_STATISTICS_FACE_DETECT_MODE_OFF;
    bool   bMultiZoomMode = false;

    int fake_face = (mLogLevel >= 0x11);

    Mutex::Autolock _l(mResultLock);

    {
        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_STATISTICS_FACE_DETECT_MODE);
            if(!entryMode.isEmpty() && MTK_STATISTICS_FACE_DETECT_MODE_OFF != entryMode.itemAt(0, Type2Type<MUINT8>())) {
                FDEn = 1;
                FDMode = entryMode.itemAt(0, Type2Type<MUINT8>());
                pOutMetadataResult->update(MTK_STATISTICS_FACE_DETECT_MODE, entryMode);
            }
        }
        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_FACE_FEATURE_GESTURE_MODE);
            if(!entryMode.isEmpty() && MTK_FACE_FEATURE_GESTURE_MODE_OFF != entryMode.itemAt(0, Type2Type<MINT32>()))
                GDEn = 1;
        }
        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_FACE_FEATURE_SMILE_DETECT_MODE);
            if(!entryMode.isEmpty() && MTK_FACE_FEATURE_SMILE_DETECT_MODE_OFF != entryMode.itemAt(0, Type2Type<MINT32>()))
                SDEn = 1;
        }
        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_SCALER_CROP_REGION);
            if(!entryMode.isEmpty())
            {
                scalerCropRegion = entryMode.itemAt(0, Type2Type<MRect>());
            }
            else
            {
                MY_LOGE("Cannot get scaler crop region");
                scalerCropRegion = mUIcropRegion;
            }
        }
        // for 3rd party multi zoom algo
        /*{
            IMetadata::IEntry multifeatureEntry = pInpMetadataRequest->entryFor(MTK_MULTI_CAM_FEATURE_MODE);
            if ( !multifeatureEntry.isEmpty()
               && multifeatureEntry.itemAt(0, Type2Type<MINT32>()) == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
            {
                bMultiZoomMode = true;
            }
        }*/
    }
    FDEn = (FDEn || mForceFDEnable) && !bMultiZoomMode;

    if(fake_face) {
        mpDetectedFaces->number_of_faces = 2;
        mpDetectedFaces->faces[0].rect[0] = mUIcropRegion.p.x;
        mpDetectedFaces->faces[0].rect[1] = mUIcropRegion.p.y;
        mpDetectedFaces->faces[0].rect[2] = mUIcropRegion.p.x + 150;
        mpDetectedFaces->faces[0].rect[3] = mUIcropRegion.p.y + 150;

        mpDetectedFaces->faces[1].rect[0] = mcropRegion.p.x;
        mpDetectedFaces->faces[1].rect[1] = mcropRegion.p.y;
        mpDetectedFaces->faces[1].rect[2] = mcropRegion.p.x + 150;
        mpDetectedFaces->faces[1].rect[3] = mcropRegion.p.y + 150;
    }

    if (FDEn)
    {
        if (!mFirstUpdate && mpFDHalObj->halFDGetVersion() != HAL_FD_VER_SW36)
        {
            MY_LOGD("FD mpDetectedFaces->number_of_faces : %d", mpDetectedFaces->number_of_faces);
        }
    }

    //if(mpDetectedFaces->number_of_faces == 0)
    //    return;
    #if 0
    if(GDEn) {
        IMetadata::IEntry gd_tag(MTK_FACE_FEATURE_GESTURE_RESULT);
        gd_tag.push_back(mpDetectedGestures->number_of_faces != 0, Type2Type<MINT32>());
        pOutMetadataResult->update(MTK_FACE_FEATURE_GESTURE_RESULT, gd_tag);
    }
    #endif
    if(SDEn) {
        IMetadata::IEntry sd_tag(MTK_FACE_FEATURE_SMILE_DETECT_RESULT);
        sd_tag.push_back(mSD_Result != 0, Type2Type<MINT32>());
        pOutMetadataResult->update(MTK_FACE_FEATURE_SMILE_DETECT_RESULT, sd_tag);
    }
    if(FDEn) {
        // Filter faces that are out of scaler crop region
        unsigned int counter = 0;
        MY_LOGD_IF(mLogLevel, "scalerCropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d", scalerCropRegion.p.x, scalerCropRegion.p.y, scalerCropRegion.s.w, scalerCropRegion.s.h);
        for(int i=0; i<mpDetectedFaces->number_of_faces;i++)
        {
            if (mpDetected3AFaces->visible_percent[counter] < 50) {
                int j;
                for(j = i; j < (mpDetectedFaces->number_of_faces - 1); j++) {
                    mpDetectedFaces->faces[j].rect[0] = mpDetectedFaces->faces[j+1].rect[0];
                    mpDetectedFaces->faces[j].rect[1] = mpDetectedFaces->faces[j+1].rect[1];
                    mpDetectedFaces->faces[j].rect[2] = mpDetectedFaces->faces[j+1].rect[2];
                    mpDetectedFaces->faces[j].rect[3] = mpDetectedFaces->faces[j+1].rect[3];
                    mpDetectedFaces->faces[j].score = mpDetectedFaces->faces[j+1].score;
                    mpDetectedFaces->faces[j].id = mpDetectedFaces->faces[j+1].id;
                    mpDetectedFaces->posInfo[j].rop_dir = mpDetectedFaces->posInfo[j+1].rop_dir;
                    mpDetectedFaces->posInfo[j].rip_dir = mpDetectedFaces->posInfo[j+1].rip_dir;
                    mpDetectedFaces->faces_type[j] = mpDetectedFaces->faces_type[j+1];
                    mpDetectedFaces->fld_rop[j] = mpDetectedFaces->fld_rop[j+1];
                    mpDetectedFaces->fld_rip[j] = mpDetectedFaces->fld_rip[j+1];
                }
                mpDetectedFaces->faces[j].rect[0] = 0;
                mpDetectedFaces->faces[j].rect[1] = 0;
                mpDetectedFaces->faces[j].rect[2] = 0;
                mpDetectedFaces->faces[j].rect[3] = 0;
                mpDetectedFaces->faces[j].score = 0;
                mpDetectedFaces->posInfo[j].rop_dir = 0;
                mpDetectedFaces->posInfo[j].rip_dir = 0;
                mpDetectedFaces->number_of_faces--;
                i--;
            }
            counter ++;
        }
        //Push_back Rectangle (face_rect)
        IMetadata::IEntry face_rect_tag(MTK_STATISTICS_FACE_RECTANGLES);
        for(int i=0; i<mpDetectedFaces->number_of_faces;i++)
        {
            face_rect.p.x = mpDetectedFaces->faces[i].rect[0];  //Left
            face_rect.p.y = mpDetectedFaces->faces[i].rect[1];  //Top
            face_rect.s.w = mpDetectedFaces->faces[i].rect[2];  //Right
            face_rect.s.h = mpDetectedFaces->faces[i].rect[3];  //Bottom
            if(fake_face) {
                mpDetectedFaces->faces[i].score = 100;
                MY_LOGD("face num : %d, position : (%d, %d) , (%d, %d)", i, face_rect.p.x, face_rect.p.y, face_rect.s.w, face_rect.s.h);
            }

            if (mpDetectedFaces->faces[i].score > 100) {
                mpDetectedFaces->faces[i].score = 100;
            }
            face_rect_tag.push_back(face_rect, Type2Type<MRect>());
        }
        pOutMetadataResult->update(MTK_STATISTICS_FACE_RECTANGLES, face_rect_tag);

        //Push_back IDs
        IMetadata::IEntry face_id_tag(MTK_STATISTICS_FACE_IDS);
        for(int i=0; i<mpDetectedFaces->number_of_faces;i++)
        {
            face_id = mpDetectedFaces->faces[i].id;
            face_id_tag.push_back(face_id, Type2Type<MINT32>());
        }
        pOutMetadataResult->update(MTK_STATISTICS_FACE_IDS, face_id_tag);

        //Push_back Score
        IMetadata::IEntry face_score_tag(MTK_STATISTICS_FACE_SCORES);
        for(int i=0; i<mpDetectedFaces->number_of_faces;i++)
        {
            face_score = mpDetectedFaces->faces[i].score;
            face_score_tag.push_back(face_score, Type2Type<MUINT8>());
        }
        pOutMetadataResult->update(MTK_STATISTICS_FACE_SCORES, face_score_tag);

        // Add additional metadata
        IMetadata::IEntry face_info_tag(MTK_FACE_FEATURE_FACE_ADDITIONAL_RESULT);
        int LM_Num = mpDetectedFaces->number_of_faces > LANDMARK_NUM ? LANDMARK_NUM : mpDetectedFaces->number_of_faces;
        for(int i = 0; i < LM_Num; i++)
        {
            face_info_tag.push_back(mpDetectedFaces->fld_rop[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->fld_rip[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyex0[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyey0[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyex1[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyey1[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyex0[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyey0[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyex1[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyey1[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->mouthx0[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->mouthy0[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->mouthx1[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->mouthy1[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->nosex[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->nosey[i], Type2Type<MINT32>());

            face_info_tag.push_back(mpDetectedFaces->leyeux[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyeuy[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyedx[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->leyedy[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyeux[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyeuy[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyedx[i], Type2Type<MINT32>());
            face_info_tag.push_back(mpDetectedFaces->reyedy[i], Type2Type<MINT32>());

            face_info_tag.push_back(mpDetectedFaces->fa_cv[i], Type2Type<MINT32>());
         }
        pOutMetadataResult->update(MTK_FACE_FEATURE_FACE_ADDITIONAL_RESULT, face_info_tag);

        //Push_back Landmark (face_landmark)
        IMetadata::IEntry face_landmark_tag(MTK_STATISTICS_FACE_LANDMARKS);
        for(int i=0; i < LM_Num; i++)
        {
            face_landmark = (mpDetectedFaces->leyex0[i] + mpDetectedFaces->leyex1[i]) >> 1;  //left_eye_x
            face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
            face_landmark = (mpDetectedFaces->leyey0[i] + mpDetectedFaces->leyey1[i]) >> 1;  //left_eye_y
            face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
            face_landmark = (mpDetectedFaces->reyex0[i] + mpDetectedFaces->reyex1[i]) >> 1;  //right_eye_x
            face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
            face_landmark = (mpDetectedFaces->reyey0[i] + mpDetectedFaces->reyey1[i]) >> 1;  //right_eye_y
            face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
            face_landmark = (mpDetectedFaces->mouthx0[i] + mpDetectedFaces->mouthx1[i]) >> 1;  //mouth_x
            face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
            face_landmark = (mpDetectedFaces->mouthy0[i] + mpDetectedFaces->mouthy1[i]) >> 1;  //mouth_y
            face_landmark_tag.push_back(face_landmark, Type2Type<MINT32>());
        }
        pOutMetadataResult->update(MTK_STATISTICS_FACE_LANDMARKS, face_landmark_tag);

        // for customer
        ReturnCustomResult(pOutMetadataResult, mpDetectedFaces, mFaceRatio);
    }
}

static void getImageCrop(MRect &cropRegion, MSize &imgSize)
{
    MINT32 oldCropW, oldCropH;
    oldCropW = cropRegion.s.w;
    oldCropH = cropRegion.s.h;
    if((cropRegion.s.w * imgSize.h) > (cropRegion.s.h * imgSize.w)) //pillarbox
    {
        //MY_LOGD("pillarbox");
        cropRegion.s.w = div_round(cropRegion.s.h * imgSize.w, imgSize.h);
        cropRegion.s.h = cropRegion.s.h;
        cropRegion.p.x = cropRegion.p.x + ((oldCropW - cropRegion.s.w) >> 1);
        cropRegion.p.y = cropRegion.p.y;
    } else if((cropRegion.s.w * imgSize.h) < (cropRegion.s.h * imgSize.w)){ // letterbox
        //MY_LOGD("letterbox");
        cropRegion.s.w = cropRegion.s.w;
        cropRegion.s.h = div_round(cropRegion.s.w * imgSize.h, imgSize.w);
        cropRegion.p.x = cropRegion.p.x;
        cropRegion.p.y = cropRegion.p.y + ((oldCropH - cropRegion.s.h) >> 1);
    }
    return ;
}

/******************************************************************************
 *
 ******************************************************************************/
MINT32
FdNodeImp::
calculateP1FDCrop(
    IMetadata*              pInpMetadataRequest __unused,
    IMetadata*              pInpMetadataHalResult
)
{
    IMetadata::IEntry YUVCropEntry = pInpMetadataHalResult->entryFor(MTK_P1NODE_YUV_RESIZER1_CROP_REGION);
    IMetadata::IEntry P1CropEntry = pInpMetadataHalResult->entryFor(MTK_P1NODE_SENSOR_CROP_REGION);
    IMetadata::IEntry P1ResizeEntry = pInpMetadataHalResult->entryFor(MTK_P1NODE_RESIZER_SIZE);
    IMetadata::IEntry SensorModeEntry = pInpMetadataHalResult->entryFor(MTK_P1NODE_SENSOR_MODE);
    mbCalculateP1FD = false;
    if (!YUVCropEntry.isEmpty() && !P1CropEntry.isEmpty()  && !SensorModeEntry.isEmpty() )
    {
        if (mSensorMode != SensorModeEntry.itemAt(0, Type2Type<MINT32>()))
        {
            mSensorMode = SensorModeEntry.itemAt(0, Type2Type<MINT32>());
            MY_LOGD("update sensor mode : %d", mSensorMode);
            HwTransHelper trans(mvSensorId[0]);
            if (!trans.getMatrixToActive(mSensorMode, mSensor2Active))
            {
                return false;
            }
        }
        MRect P1Crop = P1CropEntry.itemAt(0, Type2Type<MRect>());
        MRect FDYUVCrop = YUVCropEntry.itemAt(0, Type2Type<MRect>());
        MRect FDYUVCrop_copy = FDYUVCrop;
        //MSize Rrzo = P1ResizeEntry.itemAt(0, Type2Type<MSize>());
        MSize Rrzo = mpHalImage_P1_Rrzo->getImgSize();
        //
        MY_LOGD_IF(mLogLevel, "p1 CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d", P1Crop.p.x, P1Crop.p.y, P1Crop.s.w, P1Crop.s.h);
        MY_LOGD_IF(mLogLevel, "p1 ResizeSize: w:%d, h:%d" ,Rrzo.w, Rrzo.h);
        MY_LOGD_IF(mLogLevel, "[Before] fdyuv CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d", FDYUVCrop.p.x, FDYUVCrop.p.y, FDYUVCrop.s.w, FDYUVCrop.s.h);
        MY_LOGD_IF(mLogLevel, "UI: old CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d", mcropRegion.p.x, mcropRegion.p.y, mcropRegion.s.w, mcropRegion.s.h);
        float activeCropRatio = (float) mActiveArray.s.w / (float) mcropRegion.s.w;
        // map to P1 sensor domain
        FDYUVCrop.s.w = FDYUVCrop.s.w * P1Crop.s.w / Rrzo.w;
        FDYUVCrop.s.h = FDYUVCrop.s.h * P1Crop.s.h / Rrzo.h;
        FDYUVCrop.p.x = FDYUVCrop.p.x * P1Crop.s.w / Rrzo.w + P1Crop.p.x;
        FDYUVCrop.p.y = FDYUVCrop.p.y * P1Crop.s.h / Rrzo.h + P1Crop.p.y;
        MY_LOGD_IF(mLogLevel, "[After] fdyuv CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d", FDYUVCrop.p.x, FDYUVCrop.p.y, FDYUVCrop.s.w, FDYUVCrop.s.h);

        // apply EIS offset
        if (mIsEISOn && mpEISCache != nullptr) {
            int32_t xOffSet = 0;
            int32_t yOffSet = 0;
            int32_t topLeft_x = 0;
            int32_t topLeft_y = 0;
            int32_t bottomRight_x = 0;
            int32_t bottomRight_y = 0;
            int32_t eisWarpIn_W = 0;
            int32_t eisWarpIn_H = 0;
            int32_t eisWarpOut_W = 0;
            int32_t eisWarpOut_H = 0;
            if (!mpEISCache->getEISCrop(xOffSet, yOffSet,
                                    topLeft_x, topLeft_y,
                                    bottomRight_x, bottomRight_y,
                                    eisWarpIn_W, eisWarpIn_H,
                                    eisWarpOut_W, eisWarpOut_H,
                                    mDisplayCrop)) {
                MY_LOGE("Get EIS Crop fail!");
            }
            mWarpOutSize.w = eisWarpOut_W;
            mWarpOutSize.h = eisWarpOut_H;
            topLeft_x /= 32;
            topLeft_y /= 32;
            bottomRight_x /= 32;
            bottomRight_y /= 32;
            MY_LOGD_IF(mLogLevel, "EIS xOffSet: %d, yOffSet:%d, topleft:(%d, %d), bottomRight:(%d, %d), EISWarpIn: %dx%d, EISWarpOut: %dx%d, displayCrop(%f, %f, %fx%f)",
                   xOffSet, yOffSet, topLeft_x, topLeft_y, bottomRight_x, bottomRight_y, eisWarpIn_W, eisWarpIn_H, eisWarpOut_W, eisWarpOut_H,
                   mDisplayCrop.p.x, mDisplayCrop.p.y, mDisplayCrop.s.w, mDisplayCrop.s.h);
            MY_LOGD_IF(mLogLevel, "Rrzo size: %dx%d", Rrzo.w, Rrzo.h);
            if (mDisplayCrop.s.w >= eisWarpOut_W || mDisplayCrop.s.h >= eisWarpOut_H) {
                mDisplayCrop.s.w = eisWarpOut_W;
                mDisplayCrop.s.h = eisWarpOut_H;
                mDisplayCrop.p.x = 0;
                mDisplayCrop.p.y = 0;
            } else {
                mDisplayCrop.p.x = (eisWarpOut_W - mDisplayCrop.s.w) / 2;
                mDisplayCrop.p.y = (eisWarpOut_H - mDisplayCrop.s.h) / 2;
            }
            MY_LOGD_IF(mLogLevel, "[Adjust] display crop(%f, %f, %fx%f)",
                       mDisplayCrop.p.x, mDisplayCrop.p.y, mDisplayCrop.s.w, mDisplayCrop.s.h);
            if (eisWarpIn_W == 0 || eisWarpIn_H == 0 || eisWarpOut_W == 0 || eisWarpOut_H == 0) {
                mSensor2Active.transform(mSensorSize, mSensorSize2Active);
                mSensor2Active.transform(FDYUVCrop, mcropRegion);
                mUIcropRegion = mcropRegion;
                mbCalculateP1FD = true;
                return true;
            }
            if (bottomRight_x == 0) bottomRight_x = eisWarpIn_W;
            if (bottomRight_y == 0) bottomRight_y = eisWarpIn_H;
            mEISWarpRegion.s.w = bottomRight_x - topLeft_x;
            mEISWarpRegion.s.h = bottomRight_y - topLeft_y;
            mEISWarpRegion.p.x = topLeft_x;
            mEISWarpRegion.p.y = topLeft_y;
            MY_LOGD_IF(mLogLevel, "EIS warp region (%d, %d, %dx%d)",
                   mEISWarpRegion.p.x, mEISWarpRegion.p.y, mEISWarpRegion.s.w, mEISWarpRegion.s.h);

            mEISScale_W = mUIcropRegion.s.w * 100 / (int)mDisplayCrop.s.w;
            mAppOffset_X = (mActiveArray.s.w - mUIcropRegion.s.w) / 2;
            mEISScale_H = mUIcropRegion.s.h * 100 / (int)mDisplayCrop.s.h;
            mAppOffset_Y = (mActiveArray.s.h - mUIcropRegion.s.h) / 2;

            if (mPreviewSize.w > 0 && mPreviewSize.h > 0) {
                MY_LOGD_IF(mLogLevel, "preview size: %dx%d", mPreviewSize.w, mPreviewSize.h);
                mEISScale_W = mUIcropRegion.s.w * 100 / (int)mDisplayCrop.s.w;
                mEISScale_H = (mUIcropRegion.s.w * mPreviewSize.h / mPreviewSize.w) * 100 / (int)mDisplayCrop.s.h;
                mAppOffset_Y = (mActiveArray.s.h - (mUIcropRegion.s.w * mPreviewSize.h / mPreviewSize.w)) / 2;
            }

            int rrzToWarpInScaleW = eisWarpIn_W * 100 / Rrzo.w;
            int rrzToWarpInScaleH = eisWarpIn_H * 100 / Rrzo.h;
            MY_LOGD_IF(mLogLevel, "rrzToWarpInScaleW: %d, rrzToWarpInScaleH: %d", rrzToWarpInScaleW, rrzToWarpInScaleH);
            FDYUVCrop_copy.s.w = FDYUVCrop_copy.s.w * rrzToWarpInScaleW / 100;
            FDYUVCrop_copy.s.h = FDYUVCrop_copy.s.h * rrzToWarpInScaleH / 100;
            FDYUVCrop_copy.p.x = (eisWarpIn_W - FDYUVCrop_copy.s.w) / 2;
            FDYUVCrop_copy.p.y = (eisWarpIn_H - FDYUVCrop_copy.s.h) / 2;

            mUIcropRegion.p.x = FDYUVCrop_copy.p.x;
            mUIcropRegion.p.y = FDYUVCrop_copy.p.y;
            mUIcropRegion.s.w = FDYUVCrop_copy.s.w;
            mUIcropRegion.s.h = FDYUVCrop_copy.s.h;

            MY_LOGD_IF(mLogLevel, "UICrop(%d, %d, %dx%d), EIS Scale(%d, %d), AppOffSet(%d, %d)",
                       mUIcropRegion.p.x, mUIcropRegion.p.y, mUIcropRegion.s.w, mUIcropRegion.s.h,
                       mEISScale_W, mEISScale_H, mAppOffset_X, mAppOffset_Y);

            mSensor2Active.transform(FDYUVCrop, mcropRegion);
            return true;
        }
        // deal with sensor crop
        mSensor2Active.transform(mSensorSize, mSensorSize2Active);
        mSensor2Active.transform(FDYUVCrop, mUIcropRegion);
        MY_LOGD_IF(mLogLevel, "transform from sensor to active: p.x:%d, p.y:%d, s.w:%d, s.h:%d", mUIcropRegion.p.x, mUIcropRegion.p.y, mUIcropRegion.s.w, mUIcropRegion.s.h);
        if (mvSensorId.size() == 1 && mActiveArray.s.w > mSensorSize2Active.w) {
          int ratioX100 = mActiveArray.s.w * 100 / mSensorSize2Active.w;
          mUIcropRegion.s.w = mUIcropRegion.s.w * ratioX100 / 100;
          mUIcropRegion.s.h = mUIcropRegion.s.h * ratioX100 / 100;
          mUIcropRegion.p.x = (mActiveArray.s.w - mUIcropRegion.s.w) / 2;
          mUIcropRegion.p.y = (mActiveArray.s.h - mUIcropRegion.s.h) / 2;
        }
        MY_LOGD_IF(mLogLevel, "UI: new CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d", mUIcropRegion.p.x, mUIcropRegion.p.y, mUIcropRegion.s.w, mUIcropRegion.s.h);
        mbCalculateP1FD = true;
        return true;
    }
    return false;
}


/******************************************************************************
 *
 ******************************************************************************/
MVOID
FdNodeImp::
prepareFDParams(
    IMetadata*              pInpMetadataRequest,
    IMetadata*              pInpMetadataHalResult,
    MSize                   imgSize
)
{
    IMetadata::IEntry recordEntry = pInpMetadataHalResult->entryFor(MTK_PIPELINE_VIDEO_RECORD);
    IMetadata::IEntry FDCrop = pInpMetadataHalResult->entryFor(MTK_P2NODE_FD_CROP_REGION);
    IMetadata::IEntry syncEntry = pInpMetadataHalResult->entryFor(MTK_STEREO_SYNC2A_MASTER_SLAVE);
    IMetadata::IEntry zoomShiftEntry = pInpMetadataHalResult->entryFor(MTK_DUALZOOM_CENTER_SHIFT);
    IMetadata::IEntry ratioEntry = pInpMetadataHalResult->entryFor(MTK_DUALZOOM_FOV_RATIO);
    IMetadata::IEntry masterEntry = pInpMetadataHalResult->entryFor(MTK_DUALZOOM_FD_REAL_MASTER);
    IMetadata::IEntry previewIdEntry = pInpMetadataHalResult->entryFor(MTK_DUALZOOM_REAL_MASTER);
    IMetadata::IEntry multifeatureEntry = pInpMetadataRequest->entryFor(MTK_MULTI_CAM_FEATURE_MODE);
    // magic number for 3A
    IMetadata::IEntry magicNumberEntry = pInpMetadataHalResult->entryFor(MTK_P1NODE_PROCESSOR_MAGICNUM);
    if (magicNumberEntry.isEmpty()) {
        MY_LOGW("Cannot get magic number from MTK_P1NODE_PROCESSOR_MAGICNUM");
    }
    else {
        mMagicNo = magicNumberEntry.itemAt(0, Type2Type<MINT32>());
    }

    bool isDualZoom = false;
    bool isP2CropInfo = false;
    if (misVsdof) {
        mFDPeriod = mVsdofFDPeriod;
    }
    else if ( !recordEntry.isEmpty() )
    {
        mFDPeriod = mVideoFDPeriod;
    }
    else
    {
        mFDPeriod = mConfigPeriod;
    }
    {
        if ( !multifeatureEntry.isEmpty()
           && multifeatureEntry.itemAt(0, Type2Type<MINT32>()) == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
        {
            isDualZoom = true;
            int master_sensor = !masterEntry.isEmpty() ? masterEntry.itemAt(0, Type2Type<MINT32>()) : syncEntry.itemAt(0, Type2Type<MINT32>());
            int preview_sensor = !previewIdEntry.isEmpty() ? previewIdEntry.itemAt(0, Type2Type<MINT32>()) : master_sensor;
            if (master_sensor == getOpenId())
            {
                mMasterIndex = 0;
            }
            else
            {
                for (int i = 1; i < mvSensorId.size(); i++)
                {
                    if (master_sensor == mvSensorId[i])
                    {
                        mMasterIndex = i;
                        break;
                    }
                }
            }
            if (preview_sensor != master_sensor)
            {
                mSkipStoreFDData = true;
            }
            else
            {
                mSkipStoreFDData = false;
            }
            if (mFovRatio == 0.0 && !ratioEntry.isEmpty())
            {
                mFovRatio = ratioEntry.itemAt(0, Type2Type<MFLOAT>());
            }
            if (mShiftPoint.isOrigin() && !zoomShiftEntry.isEmpty())
            {
                MPoint point = zoomShiftEntry.itemAt(0, Type2Type<MPoint>());
                mShiftPoint.x = mActiveArray.p.x + ((mActiveArray.s.w - (mActiveArray.s.w / mFovRatio))/2) + point.x;
                mShiftPoint.y = mActiveArray.p.y + ((mActiveArray.s.h - (mActiveArray.s.h / mFovRatio))/2) + point.y;
            }
        }
    }
    if( !FDCrop.isEmpty() ) {
        mcropRegion = FDCrop.itemAt(0, Type2Type<MRect>());
        isP2CropInfo = true;
    }
    else
    {
        MY_LOGD("no FDCrop from P2, use App scaler crop");
        IMetadata::IEntry entry = pInpMetadataRequest->entryFor(MTK_SCALER_CROP_REGION);
        if( !entry.isEmpty() ) {
            mcropRegion = entry.itemAt(0, Type2Type<MRect>());
            mUIcropRegion = entry.itemAt(0, Type2Type<MRect>());
        }
        else
        {
            MY_LOGW("GetCropRegion Fail!");
        }
    }
    if (isP2CropInfo == true)
    {
        IMetadata::IEntry entry = pInpMetadataRequest->entryFor(MTK_SCALER_CROP_REGION);
        if( !entry.isEmpty() ) {
            mUIcropRegion = entry.itemAt(0, Type2Type<MRect>());
        }
        else
        {
            MY_LOGW("Get UI CropRegion Fail!");
        }
    }
    if (misDirectYuv)
    {
        IMetadata::IEntry Gamma = pInpMetadataHalResult->entryFor(MTK_FEATURE_FACE_APPLIED_GAMMA);
        if( !Gamma.isEmpty() )
        {
            mDupImage.gammaIndex= Gamma.itemAt(0, Type2Type<MINT32>());
        }
        else
        {
            MY_LOGW("Get gamma index fail!!!");
        }
        MY_LOGD_IF(mLogLevel, "applied gamma index : %d ", mDupImage.gammaIndex);

        IMetadata::IEntry LvData = pInpMetadataHalResult->entryFor(MTK_3A_AE_LV_VALUE);
        if( !LvData.isEmpty() )
        {
            mDupImage.lvdata = LvData.itemAt(0, Type2Type<MINT32>());
        }
        else
        {
            MY_LOGD("Get LV value fail!!!");
        }
        MY_LOGD_IF(mLogLevel, "Lv value : %d ", mDupImage.lvdata);
    }

    IMetadata::IEntry FDTime = pInpMetadataHalResult->entryFor(MTK_P1NODE_FRAME_START_TIMESTAMP);
    if( !FDTime.isEmpty() )
    {
        mDupImage.timestamp = FDTime.itemAt(0, Type2Type<MINT64>());
    }
    else
    {
        MY_LOGW("Get timestamp fail!!!");
    }
    MY_LOGD_IF(mLogLevel, "frame start time : %" PRId64 " ", mDupImage.timestamp);

    mRealFDYUVSize = MSize(0, 0);  // reset
    // store P1 real yuv size
    IMetadata::IEntry yuvSize = pInpMetadataHalResult->entryFor(MTK_P1NODE_YUV_RESIZER1_SIZE);
    if (!yuvSize.isEmpty()) {
        mRealFDYUVSize = yuvSize.itemAt(0, Type2Type<MSize>());
        MY_LOGD_IF(mLogLevel, "FDNode mRealFDYUVSize: %dx%d", mRealFDYUVSize.w, mRealFDYUVSize.h);
    }

    if (calculateP1FDCrop(pInpMetadataRequest, pInpMetadataHalResult))
    {
        return;
    }

    mZoomRatio_x100 = mActiveArray.s.w * 100 / mUIcropRegion.s.w;

    MY_LOGD_IF(mLogLevel, "old CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d, ",mcropRegion.p.x, mcropRegion.p.y, mcropRegion.s.w, mcropRegion.s.h);
    getImageCrop(mcropRegion, imgSize);
    MY_LOGD_IF(mLogLevel, "new CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d, ",mcropRegion.p.x, mcropRegion.p.y, mcropRegion.s.w, mcropRegion.s.h);

    MY_LOGD_IF(mLogLevel, "UI: old CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d, ",mUIcropRegion.p.x, mUIcropRegion.p.y, mUIcropRegion.s.w, mUIcropRegion.s.h);
    getImageCrop(mUIcropRegion, imgSize);

    // For EIS flow
    if (mIsEISOn && mpEISCache != nullptr) {
        int32_t xOffSet = 0;
        int32_t yOffSet = 0;
        int32_t topLeft_x = 0;
        int32_t topLeft_y = 0;
        int32_t bottomRight_x = 0;
        int32_t bottomRight_y = 0;
        int32_t eisWarpIn_W = 0;
        int32_t eisWarpIn_H = 0;
        int32_t eisWarpOut_W = 0;
        int32_t eisWarpOut_H = 0;
        if (!mpEISCache->getEISCrop(xOffSet, yOffSet,
                                    topLeft_x, topLeft_y,
                                    bottomRight_x, bottomRight_y,
                                    eisWarpIn_W, eisWarpIn_H,
                                    eisWarpOut_W, eisWarpOut_H,
                                    mDisplayCrop)) {
            MY_LOGE("Get EIS Crop fail!");
        }
        mWarpOutSize.w = eisWarpOut_W;
        mWarpOutSize.h = eisWarpOut_H;
        topLeft_x /= 32;
        topLeft_y /= 32;
        bottomRight_x /= 32;
        bottomRight_y /= 32;
        MY_LOGD_IF(mLogLevel, "EIS xOffSet: %d, yOffSet:%d, topleft:(%d, %d), bottomRight:(%d, %d), EISWarpIn: %dx%d, EISWarpOut: %dx%d, displayCrop(%f, %f, %fx%f)",
                   xOffSet, yOffSet, topLeft_x, topLeft_y, bottomRight_x, bottomRight_y, eisWarpIn_W, eisWarpIn_H, eisWarpOut_W, eisWarpOut_H,
                   mDisplayCrop.p.x, mDisplayCrop.p.y, mDisplayCrop.s.w, mDisplayCrop.s.h);
        if (mDisplayCrop.s.w >= eisWarpOut_W || mDisplayCrop.s.h >= eisWarpOut_H) {
            mDisplayCrop.s.w = eisWarpOut_W;
            mDisplayCrop.s.h = eisWarpOut_H;
            mDisplayCrop.p.x = 0;
            mDisplayCrop.p.y = 0;
        } else {
            mDisplayCrop.p.x = (eisWarpOut_W - mDisplayCrop.s.w) / 2;
            mDisplayCrop.p.y = (eisWarpOut_H - mDisplayCrop.s.h) / 2;
        }
        MY_LOGD_IF(mLogLevel, "[Adjust] display crop(%f, %f, %fx%f)",
                   mDisplayCrop.p.x, mDisplayCrop.p.y, mDisplayCrop.s.w, mDisplayCrop.s.h);

        if (eisWarpIn_W == 0 || eisWarpIn_H == 0 || eisWarpOut_W == 0 || eisWarpOut_H == 0) {
            return;
        }
        if (bottomRight_x == 0) bottomRight_x = eisWarpIn_W;
        if (bottomRight_y == 0) bottomRight_y = eisWarpIn_H;
        mEISWarpRegion.p.x = topLeft_x;
        mEISWarpRegion.p.y = topLeft_y;
        mEISWarpRegion.s.w = bottomRight_x - topLeft_x;
        mEISWarpRegion.s.h = bottomRight_y - topLeft_y;

        mEISScale_W = mUIcropRegion.s.w * 100 / (int)mDisplayCrop.s.w;
        mAppOffset_X = (mActiveArray.s.w - mUIcropRegion.s.w) / 2;
        mEISScale_H = mUIcropRegion.s.h * 100 / (int)mDisplayCrop.s.h;
        mAppOffset_Y = (mActiveArray.s.h - mUIcropRegion.s.h) / 2;
        // mEISScale_W = mUIcropRegion.s.w * 100 / (int)mDisplayCrop.s.w;
        // mAppOffset_X = (mActiveArray.s.w - mUIcropRegion.s.w) / 2;
        // mEISScale_H = mUIcropRegion.s.h * 100 / (int)mDisplayCrop.s.h;
        // mAppOffset_Y = (mActiveArray.s.h - mUIcropRegion.s.h) / 2;
        if (mPreviewSize.w > 0 && mPreviewSize.h > 0) {
            MY_LOGD_IF(mLogLevel, "preview size: %dx%d", mPreviewSize.w, mPreviewSize.h);
            mEISScale_W = mUIcropRegion.s.w * 100 / (int)mDisplayCrop.s.w;
            mEISScale_H = (mUIcropRegion.s.w * mPreviewSize.h / mPreviewSize.w) * 100 / (int)mDisplayCrop.s.h;
            mAppOffset_Y = (mActiveArray.s.h - (mUIcropRegion.s.w * mPreviewSize.h / mPreviewSize.w)) / 2;
        }
        MY_LOGD_IF(mLogLevel, "UICrop(%d, %d, %dx%d), EIS Scale(%d, %d), AppOffSet(%d, %d)",
                   mUIcropRegion.p.x, mUIcropRegion.p.y, mUIcropRegion.s.w, mUIcropRegion.s.h,
                   mEISScale_W, mEISScale_H, mAppOffset_X, mAppOffset_Y);


        mUIcropRegion.p.x = 0;
        mUIcropRegion.p.y = 0;
        mUIcropRegion.s.w = eisWarpIn_W;
        mUIcropRegion.s.h = eisWarpIn_H;
    }

    MY_LOGD_IF(mLogLevel, "UI: new CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d, ",mUIcropRegion.p.x, mUIcropRegion.p.y, mUIcropRegion.s.w, mUIcropRegion.s.h);

    if (isDualZoom && mMasterIndex != 0 && mFovRatio != 0.0 && !mShiftPoint.isOrigin())
    {
        if (!isP2CropInfo)
        {
            // calculate wide to tele
            mcropRegion.s.w = mcropRegion.s.w * mFovRatio;
            mcropRegion.s.h = mcropRegion.s.h * mFovRatio;
            mcropRegion.p.x = (mcropRegion.p.x - mShiftPoint.x) * mFovRatio;
            mcropRegion.p.y = (mcropRegion.p.y - mShiftPoint.y) * mFovRatio;
            MY_LOGD_IF(mLogLevel, "dualzoom new CropRegion: p.x:%d, p.y:%d, s.w:%d, s.h:%d, ",mcropRegion.p.x, mcropRegion.p.y, mcropRegion.s.w, mcropRegion.s.h);
        }
    }

}


/******************************************************************************
 *
 ******************************************************************************/
MVOID
FdNodeImp::
onProcessFrame(
    android::sp<IPipelineFrame> const& pFrame
)
{
    MY_LOGD_IF(mLogLevel, "[onProcessFrame] In FrameNo : %d", pFrame->getFrameNo());
    if (pFrame->getFrameNo() > 1) {
        if (pFrame->getFrameNo() - mLastFrameNo > 1) {
            mbNeedReCount = true;
        }
    }
    mLastFrameNo = pFrame->getFrameNo();

    StreamId_T const        streamIdOutMetaResult       = (mpOutMetaStreamInfo_Result != nullptr) ?
                                           mpOutMetaStreamInfo_Result->getStreamId() : -1;
    sp<IMetaStreamBuffer>   pOutMetaStreamBufferResult  = NULL;
    IMetadata*              pOutMetadataResult          = NULL;
    //
    StreamId_T const        streamIdInpMetaRequest      = (mpInMetaStreamInfo_Request != nullptr) ?
                                           mpInMetaStreamInfo_Request->getStreamId() : -1;
    sp<IMetaStreamBuffer>   pInpMetaStreamBufferRequest = NULL;
    IMetadata*              pInpMetadataRequest         = NULL;
    //
    StreamId_T const        streamIdInpMetaP2Result      = (mpInMetaStreamInfo_P2Result != nullptr) ?
                                           mpInMetaStreamInfo_P2Result->getStreamId() : -1;
    sp<IMetaStreamBuffer>   pInpMetaStreamBufferP2Result = NULL;
    IMetadata*              pInpMetadataHalResult         = NULL;
    //
    StreamId_T const        streamIdInpImageYuv         = (mpInImageStreamInfo_Yuv != nullptr) ?
                                           mpInImageStreamInfo_Yuv->getStreamId() : -1;
    sp<IImageStreamBuffer>  pInpImageStreamBufferYuv    = NULL;
    sp<IImageBufferHeap>    pInpImageBufferHeapYuv      = NULL;
    sp<IImageBuffer>        pInpImageBufferYuv          = NULL;
    //
    //IStreamInfoSet const& rStreamInfoSet= pFrame->getStreamInfoSet();
    IStreamBufferSet& rStreamBufferSet  = pFrame->getStreamBufferSet();
    //
    MINT32 success = 0;
    MINT32 SDEn = 0;
    MINT32 GDEn = 0;
    //MINT32 RotateInfo = 0;
    MINT32 FDEn = 0;
    MINT32 RunFD = 0;
    MINT32 isStillCapture = 0;
    MINT32 evSetting = 0;
    MINT32 skipRunFD = 0;
    MINT32 inSensorZoomMode = MTK_CONTROL_CAPTURE_INSENSORZOOM_MODE_OFF;

    auto getMetaBuffer = [&] (StreamId_T const streamId, sp<IMetaStreamBuffer> &metaBuffer) -> int
    {
        MERROR const err = ensureMetaBufferAvailable_(
            pFrame->getFrameNo(),
            streamId,
            rStreamBufferSet,
            metaBuffer
        );
        if( err != OK ) {
            MY_LOGW("cannot get meta: streamId %#" PRIx64" of frame %d",
                    streamId,
                    pFrame->getFrameNo());
            return false;
        }
        return true;
    };

    if(!mbInited)
        goto lbExit;

    if(!mFDProcInited) {
        onInitFDProc();
        mFDProcInited = MTRUE;
    }
    //
    ////////////////////////////////////////////////////////////////////////////
    //  Ensure buffers available.
    ////////////////////////////////////////////////////////////////////////////
    //  Input Meta Stream: Request
    if( streamIdInpMetaRequest == -1 || !getMetaBuffer(streamIdInpMetaRequest, pInpMetaStreamBufferRequest) )
    {
        MY_LOGD("Cannot get input app metadata");
        goto lbExit;
    }
    //  Output Meta Stream: Result
    if( streamIdOutMetaResult == -1 || !getMetaBuffer(streamIdOutMetaResult, pOutMetaStreamBufferResult) )
    {
        MY_LOGD("Cannot get output app metadata");
    }
    //  Input Meta Stream: P2 hal result
    if( streamIdInpMetaP2Result == -1 || !getMetaBuffer(streamIdInpMetaP2Result, pInpMetaStreamBufferP2Result) )
    {
        MY_LOGD("Cannot get input hal metadata");
    }
    //
    //  Input Image Stream: YUV
    if( streamIdInpImageYuv != -1 )
    {
        StreamId_T const streamId = streamIdInpImageYuv;
        MERROR const err = ensureImageBufferAvailable_(
            pFrame->getFrameNo(),
            streamId,
            rStreamBufferSet,
            pInpImageStreamBufferYuv
        );
        //Should check the returned error code!!!
        if( err != OK ) {
            MY_LOGW("cannot get input YUV: streamId %#" PRIx64" of frame %d",
                    streamId,
                    pFrame->getFrameNo());
        }
    }

    success = 1;

    ////////////////////////////////////////////////////////////////////////////
    //  Prepare buffers before using.
    ////////////////////////////////////////////////////////////////////////////
    {
        //  Output Meta Stream: Result
        if (pOutMetaStreamBufferResult != nullptr)
        {
            pOutMetadataResult = pOutMetaStreamBufferResult->tryWriteLock(getNodeName());
            if (pOutMetadataResult == NULL)
            {
                MY_LOGW("pOutMetadataResult == NULL");
                goto lbExit;
            }
        }
        //
        //  Input Meta Stream: Request
        if (pInpMetaStreamBufferRequest != nullptr)
        {
            pInpMetadataRequest = pInpMetaStreamBufferRequest->tryReadLock(getNodeName());
            if (pInpMetadataRequest == NULL)
            {
                MY_LOGW("pInpMetadataRequest == NULL");
                goto lbExit;
            }
        }
        //  Input Meta Stream: P2 hal result
        if (pInpMetaStreamBufferP2Result != nullptr)
        {
            pInpMetadataHalResult = pInpMetaStreamBufferP2Result->tryReadLock(getNodeName());
            if (pInpMetadataHalResult == NULL)
            {
                MY_LOGW("pInpMetadataHalResult == NULL");
                goto lbExit;
            }
        }
        //
        //  Input Image Stream: YUV
        if (pInpImageStreamBufferYuv != nullptr)
        {
            //  Query the group usage.
            //MUINT const groupUsage = pInpImageStreamBufferYuv->queryGroupUsage(getNodeId());
            //
            pInpImageBufferHeapYuv = pInpImageStreamBufferYuv->tryReadLock(getNodeName());
            if (pInpImageBufferHeapYuv == NULL)
            {
                MY_LOGW("pInpImageBufferHeapYuv == NULL");
                goto lbExit;
            }
            pInpImageBufferYuv = pInpImageBufferHeapYuv->createImageBuffer();
            if (pInpImageBufferYuv == NULL)
            {
                MY_LOGW("pInpImageBufferYuv == NULL");
                goto lbExit;
            }


            MUINT const usage = eBUFFER_USAGE_SW_READ_OFTEN |
                eBUFFER_USAGE_HW_CAMERA_READWRITE
                ;
            pInpImageBufferYuv->lockBuf(getNodeName(), usage);

        }
    }

    // prepare metadata for physical streams
    for (auto physicalStreamInfo : mvpPhysicalStreamsInfo)
    {
        if (physicalStreamInfo.sensorId == mvSensorId[mMasterIndex]) continue;

        auto mpOutMetaPhysicalStreamInfo = physicalStreamInfo.pOutAppPhysicalMeta;
        // output metadata
        StreamId_T const        streamIdOutPhysicalMetaResult       = (mpOutMetaPhysicalStreamInfo != nullptr) ?
                                           mpOutMetaPhysicalStreamInfo->getStreamId() : -1;
        sp<IMetaStreamBuffer>   pOutPhysicalMetaStreamBufferResult  = NULL;
        IMetadata*              pOutPhysicalMetadataResult          = NULL;

        // ensure buffer available
        if( streamIdOutPhysicalMetaResult == -1 || !getMetaBuffer(streamIdOutPhysicalMetaResult, pOutPhysicalMetaStreamBufferResult) )
        {
            MY_LOGD("Cannot get output physical app metadata");
            goto lbExit;
        }

        // prepare buffer before using
        if (pOutPhysicalMetaStreamBufferResult != nullptr)
        {
            pOutPhysicalMetadataResult = pOutPhysicalMetaStreamBufferResult->tryWriteLock(getNodeName());
            if (pOutPhysicalMetadataResult == NULL)
            {
                MY_LOGW("pOutPhysicalMetadataResult == NULL");
                goto lbExit;
            }
        }
        else
        {
            MY_LOGW("pOutPhysicalMetaStreamBufferResult == nullptr");
            goto lbExit;
        }

        //
        ReturnFDResult(pOutPhysicalMetadataResult, pInpMetadataRequest, pInpMetadataHalResult);

        // done, release buffer
        {
            StreamId_T const outStreamId = streamIdOutPhysicalMetaResult;
            //
            if (outStreamId != -1)
            {
                if  ( pOutPhysicalMetaStreamBufferResult != NULL )
                {
                    if (pOutPhysicalMetadataResult != NULL)
                    {
                        pOutPhysicalMetaStreamBufferResult->unlock(getNodeName(), pOutPhysicalMetadataResult);
                    }
                    rStreamBufferSet.markUserStatus(
                        outStreamId, getNodeId(),
                        IUsersManager::UserStatus::USED |
                        IUsersManager::UserStatus::RELEASE
                    );
                }
                else
                {
                    rStreamBufferSet.markUserStatus(
                        outStreamId, getNodeId(),
                        IUsersManager::UserStatus::RELEASE
                    );
                }
            }
        }
    }
//*********************************************************************//
#if (0)
       MY_LOGD("Dump1! \n");
    static int Count=0;
    Count++;
    if((Count%3) ==0)
    //if(1)
    {
       char szFileName[100];
       //sprintf(szFileName, "data/fd_data/640x480_YY_%d.raw", Count);
       sprintf(szFileName, "/sdcard/640x480_Y1_%d.raw", Count);

       FILE * pRawFp = fopen(szFileName, "wb");
       if (NULL == pRawFp )
       {
           MY_LOGD("Can't open file to save RAW Image\n");
           while(1);
       }
       int i4WriteCnt = fwrite((void *)pInpImageBufferYuv->getBufVA(0),1, (640*480),pRawFp);
       fflush(pRawFp);
       fclose(pRawFp);

    }
#endif
//*********************************************************************//

    //****************************************************//
    //Test
    //SDEn =  1; //Pass
    //GDEn =  1; //Pass
    //****************************************************//
    {
        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_STATISTICS_FACE_DETECT_MODE);
            if(!entryMode.isEmpty() && MTK_STATISTICS_FACE_DETECT_MODE_OFF != entryMode.itemAt(0, Type2Type<MUINT8>()))
                FDEn = 1;
        }
        {
            #if MTKCAM_DISABLE_FACE_PRIORITY == 0
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_CONTROL_SCENE_MODE);
            if(!entryMode.isEmpty() && MTK_CONTROL_SCENE_MODE_FACE_PRIORITY == entryMode.itemAt(0, Type2Type<MUINT8>())) {
                FDEn = 1;
            }
            #endif
        }

        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_FACE_FEATURE_GESTURE_MODE);
            if(!entryMode.isEmpty() && MTK_FACE_FEATURE_GESTURE_MODE_OFF != entryMode.itemAt(0, Type2Type<MINT32>()))
                GDEn = 1;
        }
        {
            IMetadata::IEntry const& entryMode = pInpMetadataRequest->entryFor(MTK_FACE_FEATURE_SMILE_DETECT_MODE);
            if(!entryMode.isEmpty() && MTK_FACE_FEATURE_SMILE_DETECT_MODE_OFF != entryMode.itemAt(0, Type2Type<MINT32>()))
                SDEn = 1;
        }

        {
            IMetadata::IEntry const& entryIntent = pInpMetadataRequest->entryFor(MTK_CONTROL_CAPTURE_INTENT);
            if(!entryIntent.isEmpty() && MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE == entryIntent.itemAt(0, Type2Type<MUINT8>()))
                isStillCapture = 1;
        }
        {
            IMetadata::IEntry const& entryEv = pInpMetadataRequest->entryFor(MTK_CONTROL_AE_EXPOSURE_COMPENSATION);
            if(!entryEv.isEmpty())
                evSetting = entryEv.itemAt(0, Type2Type<MINT32>());
        }
    }
    FDEn = FDEn || mForceFDEnable;
    MY_LOGD_IF(mLogLevel, "FD_DEBUG : FDEn : %d, GDEn : %d, SDEn : %d", FDEn, GDEn, SDEn);
    RunFD = FDEn | GDEn | SDEn;
    if(!RunFD && pInpMetaStreamBufferP2Result != nullptr) {
        MY_LOGD("FD disalbed....just skip FD node");
        goto lbExit;
    }
    mFDStopped = MFALSE;

    if (pFrame->IsReprocessFrame())
    {
        MY_LOGD("zsl request, by pass run FD");
        goto lbReturnFD;
    }

    while (isStillCapture && mIsFDBusy)
    {
        MY_LOGD_IF(mLogLevel, "still cap request, fd need sync");
        usleep(5000); // sleep 5ms
    }
    if (isStillCapture && ( evSetting < -8 ))
    {
        MY_LOGD("still capture, evsetting(%d) is too small and skip FD", evSetting);
        skipRunFD = 1;
    }
    if (pFrame->IsReprocessFrame())
    {
        MY_LOGD("reprocess capture, skip FD");
        skipRunFD = 1;
    }
    //
    IMetadata::getEntry<MINT32>(pInpMetadataRequest, MTK_CONTROL_CAPTURE_INSENSORZOOM_MODE, inSensorZoomMode);
    if (inSensorZoomMode != MTK_CONTROL_CAPTURE_INSENSORZOOM_MODE_OFF) {
        MY_LOGD("In sensor zoom, bypass run FD");
        goto lbReturnFD;
    }
    if(!mIsFDBusy && pInpImageBufferYuv != nullptr && pInpMetadataHalResult != nullptr && !skipRunFD) {
        IMetadata::IEntry const& sensorSizeEntry = pInpMetadataHalResult->entryFor(MTK_HAL_REQUEST_SENSOR_SIZE);
        mSensorSize = sensorSizeEntry.itemAt(0, Type2Type<MSize>());

        if (mpFDHalObj->halFDGetVersion() >= HAL_FD_VER_HW50 && !mForceAsyncMode) {
            mDupImage.pImg = pInpImageBufferYuv;
        }
        mDupImage.w = pInpImageBufferYuv->getImgSize().w;
        mDupImage.h = pInpImageBufferYuv->getImgSize().h;
        mDupImage.planes = pInpImageBufferYuv->getPlaneCount();
        if (misSecure)
        {
            if (BufferAllocator::CheckIonSupport()) {
                mDupImage.SecureHandle = pInpImageBufferYuv->getBufVA(0);
            } else {
                #if SUPPORT_SECURE_TZMP2
                  mDupImage.FD_Y = pInpImageBufferYuv->getFD(0);
                #else
                  mDupImage.SecureHandle = pInpImageBufferYuv->getBufVA(0);
                #endif
            }
        }
        else
        {
            if(mDupImage.planes == 3) {
                if (mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW50 || mForceAsyncMode) {
                    memcpy(mDupImage.AddrY, (void *)pInpImageBufferYuv->getBufVA(0), pInpImageBufferYuv->getImgSize().w*pInpImageBufferYuv->getImgSize().h);
                    memcpy(mDupImage.AddrU, (void *)pInpImageBufferYuv->getBufVA(1), (pInpImageBufferYuv->getImgSize().w*pInpImageBufferYuv->getImgSize().h) >> 2);
                    memcpy(mDupImage.AddrV, (void *)pInpImageBufferYuv->getBufVA(2), (pInpImageBufferYuv->getImgSize().w*pInpImageBufferYuv->getImgSize().h) >> 2);
                } else {
                    mDupImage.AddrY = (MUINT8 *)pInpImageBufferYuv->getBufVA(0);
                    mDupImage.AddrU = (MUINT8 *)pInpImageBufferYuv->getBufVA(1);
                    mDupImage.AddrV = (MUINT8 *)pInpImageBufferYuv->getBufVA(2);
                }
            } else if (mDupImage.planes == 1) {
                if (mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW50 || mForceAsyncMode) {
                    memcpy(mDupImage.AddrY, (void *)pInpImageBufferYuv->getBufVA(0), pInpImageBufferYuv->getImgSize().w*pInpImageBufferYuv->getImgSize().h*2);
                } else {
                    mDupImage.AddrY = (MUINT8 *)pInpImageBufferYuv->getBufVA(0);
                }
            } else if(mDupImage.planes == 2) {
                MY_LOGW("FD node could not be here, not support buffer plane == 2");
            }
            if (mpFDHalObj->halFDGetVersion() < HAL_FD_VER_HW50 || mForceAsyncMode) {
                mDupImage.PAddrY = (MUINT8 *)mDupImage.pImg->getBufPA(0);
                mDupImage.FD_Y = mDupImage.pImg->getFD(0);
            } else {
                mDupImage.PAddrY = (MUINT8 *)pInpImageBufferYuv->getBufPA(0);
                mDupImage.FD_Y = pInpImageBufferYuv->getFD(0);
            }
            MY_LOGD_IF(mLogLevel, "Image Buffer PA: %p", mDupImage.PAddrY);
            mDupImage.pImg->syncCache(eCACHECTRL_FLUSH);
        }

        if (SDEn) mCurrent_mode = HAL_FD_MODE_SD;
        if (misVsdof) mCurrent_mode = HAL_FD_MODE_VSDOF;
        mGDEnable = GDEn;
        if (mDump && ::property_get_int32("vendor.debug.camera.dump.FDNode.Perframe", 0))
        {
            char szFileName[150];
            FILE * pRawFp;
            int i4WriteCnt;
            NSCam::TuningUtils::FILE_DUMP_NAMING_HINT hint;

            {
                IMetadata::IEntry const& entry = pInpMetadataHalResult->entryFor(MTK_PIPELINE_UNIQUE_KEY);
                if(!entry.isEmpty())
                {
                    hint.UniqueKey = entry.itemAt(0, Type2Type<MINT32>());
                }
            }
            {
                IMetadata::IEntry const& entry = pInpMetadataHalResult->entryFor(MTK_PIPELINE_FRAME_NUMBER);
                if(!entry.isEmpty())
                {
                    hint.FrameNo = entry.itemAt(0, Type2Type<MINT32>());
                }
            }
            {
                IMetadata::IEntry const& entry = pInpMetadataHalResult->entryFor(MTK_PIPELINE_REQUEST_NUMBER);
                if(!entry.isEmpty())
                {
                    hint.RequestNo = entry.itemAt(0, Type2Type<MINT32>());
                }
            }
            hint.ImgFormat = eImgFmt_YUY2;
            hint.ImgWidth = mDupImage.w;
            hint.ImgHeight = mDupImage.h;
            NSCam::TuningUtils::extract_by_SensorOpenId(&hint, getOpenId());

            NSCam::TuningUtils::genFileName_YUV(szFileName, 128, &hint, NSCam::TuningUtils::YUV_PORT_CRZO1, "");
            pRawFp = fopen(szFileName, "wb");
            if (NULL == pRawFp )
            {
                MY_LOGD("Can't open file to save RAW Image\n");
                while(1);
            }
            i4WriteCnt = fwrite((void *)mDupImage.AddrY,1, (mDupImage.w * mDupImage.h * 2),pRawFp);
            fflush(pRawFp);
            fclose(pRawFp);
            MY_LOGI("FD node dump image : %s", szFileName);
        }
        prepareFDParams(pInpMetadataRequest, pInpMetadataHalResult, pInpImageBufferYuv->getImgSize());
        if (isStillCapture || misSecure || (mpFDHalObj->halFDGetVersion() >= HAL_FD_VER_HW50 && !mForceAsyncMode))
        {
            // Skip FD if detection is late (even in secure mode)
            if (isStillCapture || mbDoFD)
            {
                RunFaceDetection();
            }
        }
        else
        {
            setFDLock(MTRUE);
            sem_post(&semFD);
        }
    }
lbReturnFD:
    if (pOutMetadataResult != nullptr)
    {
        ReturnFDResult(pOutMetadataResult, pInpMetadataRequest, pInpMetadataHalResult);
    }

    ////////////////////////////////////////////////////////////////////////////
    //  Release buffers after using.
    ////////////////////////////////////////////////////////////////////////////
lbExit:
    //
    //  Output Meta Stream: Result
    {
        StreamId_T const streamId = streamIdOutMetaResult;
        //
        if (streamId != -1)
        {
            if  ( pOutMetaStreamBufferResult != NULL )
            {
                //Buffer Producer must set this status.
                pOutMetaStreamBufferResult->markStatus(
                    success ?
                    STREAM_BUFFER_STATUS::WRITE_OK :
                    STREAM_BUFFER_STATUS::WRITE_ERROR
                );
                if (pOutMetadataResult != NULL)
                {
                    pOutMetaStreamBufferResult->unlock(getNodeName(), pOutMetadataResult);
                }
                //
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::USED |
                    IUsersManager::UserStatus::RELEASE
                );
            }
            else
            {
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::RELEASE
                );
            }
        }
    }
    //
    //  Input Meta Stream: Request
    {
        StreamId_T const streamId = streamIdInpMetaRequest;
        //
        if (streamId != -1)
        {
            if  ( pInpMetaStreamBufferRequest != NULL )
            {
                if (pInpMetadataRequest != NULL)
                {
                    pInpMetaStreamBufferRequest->unlock(getNodeName(), pInpMetadataRequest);
                }
                //
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::USED |
                    IUsersManager::UserStatus::RELEASE
                );
            }
            else
            {
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::RELEASE
                );
            }
        }
    }
    //  Input Meta Stream: P2 hal
    {
        StreamId_T const streamId = streamIdInpMetaP2Result;
        //
        if (streamId != -1)
        {
            if  ( pInpMetaStreamBufferP2Result!= NULL )
            {
                if (pInpMetadataHalResult != NULL)
                {
                    pInpMetaStreamBufferP2Result->unlock(getNodeName(), pInpMetadataHalResult);
                }
                //
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::USED |
                    IUsersManager::UserStatus::RELEASE
                );
            }
            else
            {
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::RELEASE
                );
            }
        }
    }
    //
    //  Input Image Stream: YUV
    {
        StreamId_T const streamId = streamIdInpImageYuv;
        //
        if (streamId != -1)
        {
            if  ( pInpImageStreamBufferYuv != NULL )
            {
                if (pInpImageBufferYuv != NULL)
                {
                    pInpImageBufferYuv->unlockBuf(getNodeName());
                }
                if (pInpImageBufferHeapYuv != NULL)
                {
                    pInpImageStreamBufferYuv->unlock(getNodeName(), pInpImageBufferHeapYuv.get());
                }
                //
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::USED |
                    IUsersManager::UserStatus::RELEASE
                );
            }
            else
            {
                rStreamBufferSet.markUserStatus(
                    streamId, getNodeId(),
                    IUsersManager::UserStatus::RELEASE
                );
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    //  Apply buffers to release.
    ////////////////////////////////////////////////////////////////////////////
    rStreamBufferSet.applyRelease(getNodeId());

    ////////////////////////////////////////////////////////////////////////////
    //  Dispatch
    ////////////////////////////////////////////////////////////////////////////
    onDispatchFrame(pFrame);
}


/******************************************************************************
 *
 ******************************************************************************/
#if (1)
sp<FdNode>
FdNode::
createInstance()
{
    return new FdNodeImp();

}

#endif
