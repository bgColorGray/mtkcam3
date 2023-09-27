/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
 /*
** $Log: fdvt_hal.cpp $
 *
*/
#define LOG_TAG "mHalFDVT"

#define FDVT_DDP_SUPPORT

#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <faces.h>

#include "fd_hal50_base.h"
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>

#include "camera_custom_fd.h"

#include <mtkcam/def/PriorityDefs.h>
#include <sys/prctl.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FD_HAL);

using namespace android;


//****************************//
//-------------------------------------------//
//  Global face detection related parameter  //
//-------------------------------------------//

#define USE_SW_FD_TO_DEBUG (0)
#if (MTKCAM_FDFT_USE_HW == '1') && (USE_SW_FD_TO_DEBUG == 0)
#define USE_HW_FD (1)

#define USE_HW_FD (0)
#endif

#define MHAL_NO_ERROR 0
#define MHAL_INPUT_SIZE_ERROR 1
#define MHAL_UNINIT_ERROR 2
#define MHAL_REINIT_ERROR 3
#define MHAL_BAD_VALUE 4

#undef MAX_FACE_NUM
#define MAX_FACE_NUM 15

// FD algo parameter predefine
#define MHAL_FDVT_FDBUF_W (400)
#define FD_DRIVER_MAX_WIDTH (640)
#define FD_DRIVER_MAX_HEIGHT (640)


#define DLFD_SMOOTH_PARAM (2)
#define DLFD_SMOOTH_PARAM_BASE (2816)

//static MUINT32 __unused image_width_array[FD_SCALES]  = {400, 320, 258, 214, 180, 150, 126, 104, 88, 74, 62, 52, 42, 34};
//static MUINT32 __unused image_height_array[FD_SCALES] = {300, 240, 194, 162, 136, 114, 96, 78, 66, 56, 48, 40, 32, 26};

static Mutex&       gLock = *new Mutex();
static Mutex&       gInitLock = *new Mutex();
static int         gGenderDebug = 0;
//****************************//

/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

/*******************************************************************************
* Public API
********************************************************************************/
MINT32
halFDVT50Base::halFDInit(
    MUINT32 fdW,
    MUINT32 fdH,
    MUINT8 *WorkingBuffer,
    MUINT32 WorkingBufferSize,
    MBOOL   isP1Yuv,
    MUINT8  Current_mode,  //0:FD, 1:SD, 2:vFB 3:CFB 4:VSDOF
    MINT32 FldNum,
    MBOOL isNormalWorld
)
{
    Mutex::Autolock _l(gInitLock);
    MINT32 err = MHAL_NO_ERROR;
    MTKFDFTInitInfo FDFTInitInfo;
    FD_Customize_PARA FDCustomData;
    // +++ Difference between FD version (tuning_para)
    struct tuning_para *pParams = getTuningParaByVersion();
    if (pParams == nullptr) {
        MY_LOGE("Cannot get tuning parameters!");
        return MHAL_BAD_VALUE;
    }
    // --- Difference between FD version (tuning_para)

    if(mInited) {
        MY_LOGW("Warning!!! FDVT HAL OBJ is already inited!!!!");
        MY_LOGW("Old Width/Height : %d/%d, Parameter Width/Height : %d/%d", mFDW, mFDH, fdW, fdH);
        return MHAL_REINIT_ERROR;
    }
    {
        char cLogLevel[PROPERTY_VALUE_MAX] = {"0"};
        ::property_get("vendor.debug.camera.fd.dumpimage", cLogLevel, "0");
        mDoDumpImage = atoi(cLogLevel);

        ::property_get("vendor.debug.camera.log.FDNode", cLogLevel, "0");
        mLogLevel = atoi(cLogLevel);
    }
    // Start initial FD
    mCurrentMode = Current_mode;

    MY_LOGD("[mHalFDInit] Current_mode:%d, SrcW:%d, SrcH:%d, ",Current_mode, fdW, fdH);


    get_fd_CustomizeData(&FDCustomData);

    // Set FD/FT buffer resolution
    mFDW = fdW;
    mFDH = fdH;
    mFTWidth  = fdW;
    mFTHeight = fdH;

    mFDhw_W = FDFTInitInfo.FDBufWidth = mFDW;
    mFDhw_H = FDFTInitInfo.FDBufHeight = mFDH;
    FDFTInitInfo.FDTBufWidth = mFTWidth;
    FDFTInitInfo.FDTBufHeight =  mFTHeight;
    FDFTInitInfo.FDSrcWidth = mFDW;
    FDFTInitInfo.FDSrcHeight = mFDH;

    // Set FD/FT initial parameters
    // algo tuning
    FDFTInitInfo.FDThreshold = pParams->FDThreshold;
    FDFTInitInfo.MajorFaceDecision = pParams->MajorFaceDecision;
    FDFTInitInfo.DelayThreshold = pParams->DelayThreshold;
    FDFTInitInfo.DelayCount = pParams->DelayCount;
    FDFTInitInfo.DisLimit = pParams->DisLimit;
    FDFTInitInfo.DecreaseStep = pParams->DecreaseStep;
    FDFTInitInfo.FDMINSZ = pParams->FDMINSZ;
    FDFTInitInfo.OTBndOverlap = pParams->OTBndOverlap;
    FDFTInitInfo.OTRatio = pParams->OTRatio;
    FDFTInitInfo.OTds = pParams->OTds;
    FDFTInitInfo.OTtype = pParams->OTtype;
    FDFTInitInfo.SmoothLevel = pParams->SmoothLevel;
    // dynamic pyramid
    FDFTInitInfo.CamID = -1;
    FDFTInitInfo.CameraFOV = 0;
    // VSDOF
    if (mCurrentMode == HAL_FD_MODE_VSDOF) FDFTInitInfo.SmoothLevel = 0;
    FDFTInitInfo.Momentum = pParams->Momentum;
    FDFTInitInfo.SmoothLevelUI = pParams->SmoothLevelUI;
    FDFTInitInfo.MomentumUI = pParams->MomentumUI;
    FDFTInitInfo.MaxTrackCount = pParams->MaxTrackCount;
    FDFTInitInfo.SilentModeFDSkipNum = ::property_get_int32("vendor.debug.camera.fdskipnum", pParams->SilentModeFDSkipNum);
    FDFTInitInfo.HistUpdateSkipNum = pParams->HistUpdateSkipNum;
    FDFTInitInfo.FDSkipStep = pParams->FDSkipStep;
    FDFTInitInfo.FDRectify = pParams->FDRectify;
    mFDRefresh = FDFTInitInfo.FDRefresh = pParams->FDRefresh;

    // MW control tuning
    mFDFilterRatio = FDCustomData.FDSizeRatio;
    FDFTInitInfo.WorkingBufAddr = WorkingBuffer;
    FDFTInitInfo.WorkingBufSize = WorkingBufferSize;
    FDFTInitInfo.FDThreadNum = FDCustomData.FDThreadNum;
    FDFTInitInfo.OTFlow = isP1Yuv ? HWPIPE_FLOW_P1 : HWPIPE_FLOW_P2;
    FDFTInitInfo.FDImageArrayNum = 0;
    FDFTInitInfo.FDImageWidthArray = NULL;
    FDFTInitInfo.FDImageHeightArray = NULL;
    FDFTInitInfo.FDCurrent_mode = 0;
    FDFTInitInfo.FDModel = FDCustomData.FDModel;
    FDFTInitInfo.FDMinFaceLevel = 0;
    FDFTInitInfo.FDMaxFaceLevel = 13;
    FDFTInitInfo.FDImgFmtCH1 = FACEDETECT_IMG_Y_SINGLE;
    FDFTInitInfo.FDImgFmtCH2 = FACEDETECT_IMG_RGB565;
    FDFTInitInfo.SDImgFmtCH1 = FACEDETECT_IMG_Y_SCALES;
    FDFTInitInfo.SDImgFmtCH2 = FACEDETECT_IMG_Y_SINGLE;
    FDFTInitInfo.SDThreshold = FDCustomData.SDThreshold;
    FDFTInitInfo.SDMainFaceMust = FDCustomData.SDMainFaceMust;
    FDFTInitInfo.GSensor = FDCustomData.GSensor;
    FDFTInitInfo.GenScaleImageBySw = 1;
    FDFTInitInfo.ParallelRGB565Conversion = false;
    FDFTInitInfo.LockOtBufferFunc = nullptr;
    FDFTInitInfo.UnlockOtBufferFunc = nullptr;
    FDFTInitInfo.lockAgent = nullptr;

    misSecureFD = FDFTInitInfo.isSecureFD = !isNormalWorld;
    mFLDNum = FDFTInitInfo.LandmarkEnableCnt = misSecureFD ? 0 : ::property_get_int32("vendor.debug.camera.fld.count", FldNum);
    FDFTInitInfo.FLDAttribConfig = 0; // enable gender detection
    mGenderNum = FDFTInitInfo.GenderEnableCnt = misSecureFD ? 0 : ::property_get_int32("vendor.debug.camera.gender.count", 5);
    mPoseNum = FDFTInitInfo.PoseEnableCnt = misSecureFD ? 0 : ::property_get_int32("vendor.debug.camera.pose.count", 5);
    //FDFTInitInfo.gender_status_mutexAddr = &mattribureMutex;
    FDFTInitInfo.FDVersion = getFDVersionForInit();

    gGenderDebug   = ::property_get_int32("vendor.debug.camera.fd.genderdebug", 0);

    // +++ Difference between FD version (workBufList)
    if (allocateAttributeBuffer() == MFALSE)
    {
        return MFALSE;
    }
    // --- Difference between FD version (workBufList)

    mBGBusy = false;
    mBGStop = 0;
    sem_init(&mSemBGExec, 0, 0);
    sem_init(&mSemBGIdle, 0, 0);

    // +++ Difference between FD version (BGThreadLoop)
    createBGThread();
    // --- Difference between FD version (BGThreadLoop)

    mbHwTimeout = false;
    // +++ Difference between FD version (init driver)
    FDFTInitInfo.ModelVersion = initFDDriver(fdW);
    // --- Difference between FD version (init driver)

    // dump initial info
    dumpFDParam(FDFTInitInfo);
    // Set initial info to FD algo
    mpMTKFDVTObj->FDVTInit(&FDFTInitInfo);

    MY_LOGD("[%s] End", __FUNCTION__);
    mFrameCount = 0;
    mDetectedFaceNum = 0;

    mInited = 1;
    return err;
}


MINT32
halFDVT50Base::halFDDo(
struct FD_Frame_Parameters &Param
)
{
    Mutex::Autolock _l(gInitLock);
    FdOptions FDOps;
    MBOOL SDEnable;
    MINT32 StartPos = 0;
    fd_cal_struct *pfd_cal_data;


    SDEnable = Param.SDEnable && (mCurrentMode == HAL_FD_MODE_SD);

    if(!mInited) {
        return MHAL_UNINIT_ERROR;
    }

    FACEDETECT_GSENSOR_DIRECTION direction;
    if( Param.Rotation_Info == 0 )
        direction = FACEDETECT_GSENSOR_DIRECTION_0;
    else if( Param.Rotation_Info == 90 )
        direction = FACEDETECT_GSENSOR_DIRECTION_270;
    else if( Param.Rotation_Info == 270 )
        direction = FACEDETECT_GSENSOR_DIRECTION_90;
    else if( Param.Rotation_Info == 180 )
        direction = FACEDETECT_GSENSOR_DIRECTION_180;
    else
        direction = FACEDETECT_GSENSOR_DIRECTION_NO_SENSOR;

    // Set FD operation
    FDOps.fd_state = FDVT_GFD_MODE;
    FDOps.direction = direction;
    FDOps.fd_scale_start_position = StartPos;
    FDOps.gfd_fast_mode = 0;
    FDOps.ae_stable = Param.AEStable;
    FDOps.ForceFDMode = FDVT_GFD_MODE;
    if (Param.pImageBufferPhyP2 != NULL) {
        FDOps.inputPlaneCount = 3;
    } else if (Param.pImageBufferPhyP1 != NULL) {
        FDOps.inputPlaneCount = 2;
    } else {
        FDOps.inputPlaneCount = 1;
    }
    FDOps.ImageBufferPhyPlane1 = Param.pImageBufferPhyP0;
    FDOps.ImageBufferPhyPlane2 = Param.pImageBufferPhyP1;
    FDOps.ImageBufferPhyPlane3 = Param.pImageBufferPhyP2;
    FDOps.ImageBufferRGB565 = Param.pRGB565Image;
    FDOps.ImageBufferSrcVirtual = (MUINT8*)Param.pImageBufferVirtual;
    // use start to pass real fd yuv size
    FDOps.startW = Param.padding_w;
    FDOps.startH = Param.padding_h;
    FDOps.model_version = 0;
    FDOps.curr_gtype = Param.gammaType;
    FDOps.LV = Param.LvValue;
    FDOps.DynamicLandmarkCnt = 0;
    FDOps.magicNo = Param.MagicNum;
    // dynamic pyramid
    FDOps.MainCamID = -1;
    mFD_Y = Param.FD_Y;
    {
        FDVT_OPERATION_MODE_ENUM mode;
        mpMTKFDVTObj->FDVTGetMode(&mode);

        mpMTKFDVTObj->FDVTMain(&FDOps);
        if (FDOps.doPhase2)
        {
            pfd_cal_data = mpMTKFDVTObj->FDGetCalData();//Get From Algo
            if (pfd_cal_data == nullptr) return MHAL_BAD_VALUE;

            for(int i = 0; i < MAX_FACE_SEL_NUM; i++) {
                pfd_cal_data->display_flag[i] = (kal_bool)0;
            }
            {
                doHWFaceDetection(pfd_cal_data, &FDOps);
            }

            MY_LOGD_IF(mLogLevel, "finish doHWFaceDetection");

            mpMTKFDVTObj->FDVTMainFastPhase(mGammaCtrl);

            // +++ Difference between FD version (The process flow after FDVTMainFastPhase())
            halFDDoByVersion(pfd_cal_data, &FDOps);
            // --- Difference between FD version (The process flow after FDVTMainFastPhase())
        }
    }

    mFrameCount++;

    return MHAL_NO_ERROR;
}


MINT32
halFDVT50Base::halFDUninit(
)
{
    //MHAL_LOG("[halFDUninit] IN \n");
    Mutex::Autolock _l(gInitLock);

    if(!mInited) {
        MY_LOGW("FD HAL Object is already uninited...");
        return MHAL_NO_ERROR;
    }

    mBGStop = 1;
    ::sem_post(&mSemBGExec);
    pthread_join(mBGThread, NULL);
    sem_destroy(&mSemBGExec);
    sem_destroy(&mSemBGIdle);

    mpMTKFDVTObj->FDVTReset();

    uninitFDDriver();

    // +++ Difference between FD version (workBufList)
    deallocateAttributeBuffer();
    // --- Difference between FD version (workBufList)

    mInited = 0;

    return MHAL_NO_ERROR;
}


MINT32
halFDVT50Base::halGetGammaSetting(MINT32* &gammaCtrl)
{
    gammaCtrl = mGammaCtrl;
    return 0;
}

MINT32
halFDVT50Base::halFDGetFaceInfo(
    MtkCameraFaceMetadata *fd_info_result
)
{
    MUINT8 i;

    if( (mFdResult_Num < 0) || (mFdResult_Num > 15) )
         mFdResult_Num = 0;

    fd_info_result->number_of_faces =  mFdResult_Num;

    for(i=0;i<mFdResult_Num;i++)
    {
        fd_info_result->faces[i].rect[0] = mFdResult[i].rect[0];
        fd_info_result->faces[i].rect[1] = mFdResult[i].rect[1];
        fd_info_result->faces[i].rect[2] = mFdResult[i].rect[2];
        fd_info_result->faces[i].rect[3] = mFdResult[i].rect[3];
        fd_info_result->faces[i].score = mFdResult[i].score;
        fd_info_result->posInfo[i].rop_dir = mFdResult[i].rop_dir;
        fd_info_result->posInfo[i].rip_dir  = mFdResult[i].rip_dir;
    }

    return MHAL_NO_ERROR;
}


MINT32
halFDVT50Base::halFDGetFaceResult(
    MtkCameraFaceMetadata *fd_result,
    MINT32 ResultMode
)
{
    result pbuf[MAX_FACE_NUM];
    MUINT8 i;
    MINT8 DrawMode = 0;
    MINT32 PrevFaceNum = mDetectedFaceNum;

    if (ResultMode == 1)
    {
        // for UI
        mpMTKFDVTObj->FDVTGetICSResult((MUINT8 *) fd_result, (MUINT8 *) pbuf, mFDhw_W, mFDhw_H, 0, 0, 0, 5);
    }
    else
    {
        // for 3A
        mpMTKFDVTObj->FDVTGetICSResult((MUINT8 *) fd_result, (MUINT8 *) pbuf, mFDhw_W, mFDhw_H, 0, 0, 0, 1);
    }

    MY_LOGD_IF(mLogLevel, "index[0] ICSresult : (%d, %d), (%d, %d), index[0] fld rip(%d)/rop(%d), GenderLabel(%d)",
      fd_result->faces[0].rect[0], fd_result->faces[0].rect[1], fd_result->faces[0].rect[2], fd_result->faces[0].rect[3],
      fd_result->fld_rip[0], fd_result->fld_rop[0], fd_result->GenderLabel[0]);

    mDetectedFaceNum = mFdResult_Num = fd_result->number_of_faces;

    fd_result->CNNFaces.PortEnable = 0;
    fd_result->CNNFaces.IsTrueFace = 0;
    //
    fd_result->landmarkNum = mFLDNum;
    fd_result->genderNum = mGenderNum;
    fd_result->poseNum = mPoseNum;

    fd_result->gmIndex = mGammaCtrl[0];
    fd_result->gmData = &mGammaCtrl[1];
    fd_result->gmSize = 192;

    //Facial Size Filter
    for(i=0;i < mFdResult_Num;i++)
    {
        int diff = fd_result->faces[i].rect[3] - fd_result->faces[i].rect[1];
        float ratio = (float)diff / 2000.0;
        if(ratio < mFDFilterRatio) {
            int j;
            for(j = i; j < (mFdResult_Num - 1); j++) {
                fd_result->faces[j].rect[0] = fd_result->faces[j+1].rect[0];
                fd_result->faces[j].rect[1] = fd_result->faces[j+1].rect[1];
                fd_result->faces[j].rect[2] = fd_result->faces[j+1].rect[2];
                fd_result->faces[j].rect[3] = fd_result->faces[j+1].rect[3];
                fd_result->faces[j].score = fd_result->faces[j+1].score;
                fd_result->faces[j].id = fd_result->faces[j+1].id;
                fd_result->posInfo[j].rop_dir = fd_result->posInfo[j+1].rop_dir;
                fd_result->posInfo[j].rip_dir = fd_result->posInfo[j+1].rip_dir;
                fd_result->faces_type[j] = fd_result->faces_type[j+1];
            }
            fd_result->faces[j].rect[0] = 0;
            fd_result->faces[j].rect[1] = 0;
            fd_result->faces[j].rect[2] = 0;
            fd_result->faces[j].rect[3] = 0;
            fd_result->faces[j].score = 0;
            fd_result->posInfo[j].rop_dir = 0;
            fd_result->posInfo[j].rip_dir = 0;
            fd_result->number_of_faces--;
            mFdResult_Num--;
            i--;
        }
    }

    for(i=0;i<mFdResult_Num;i++)
    {
          mFdResult[i].rect[0] = fd_result->faces[i].rect[0];
          mFdResult[i].rect[1] = fd_result->faces[i].rect[1];
          mFdResult[i].rect[2] = fd_result->faces[i].rect[2];
          mFdResult[i].rect[3] = fd_result->faces[i].rect[3];
          mFdResult[i].score = fd_result->faces[i].score;
          mFdResult[i].rop_dir = fd_result->posInfo[i].rop_dir;
          mFdResult[i].rip_dir  = fd_result->posInfo[i].rip_dir;
    }
    for(i=mFdResult_Num;i<MAX_FACE_NUM;i++)
    {
          mFdResult[i].rect[0] = 0;
          mFdResult[i].rect[1] = 0;
          mFdResult[i].rect[2] = 0;
          mFdResult[i].rect[3] = 0;
          mFdResult[i].score = 0;
          mFdResult[i].rop_dir = 0;
          mFdResult[i].rip_dir  = 0;
    }
    return mFdResult_Num;
}

void halFDVT50Base::dumpFDImages(MUINT8* pFDImgBuffer __unused, MUINT8* pFTImgBuffer __unused)
{
}

// Dump FD information
void halFDVT50Base::dumpFDParam(MTKFDFTInitInfo &FDFTInitInfo)
{
    MY_LOGD("FDThreshold = %d", FDFTInitInfo.FDThreshold);
    MY_LOGD("MajorFaceDecision = %d", FDFTInitInfo.MajorFaceDecision);
    MY_LOGD("FDTBufW/H = %d/%d", FDFTInitInfo.FDTBufWidth, FDFTInitInfo.FDTBufHeight);

    MY_LOGD("GSensor = %d", FDFTInitInfo.GSensor);
    #if (USE_HW_FD)&&(HW_FD_SUBVERSION == 2)
    MY_LOGD("FDMINSZ = %d", FDFTInitInfo.FDMINSZ);
    #endif
    #if MTK_ALGO_PSD_MODE
    MY_LOGD("FDVersion = %d, Version = %d", FDFTInitInfo.FDVersion, halFDGetVersion());
    #endif
    MY_LOGD("ModelVersion = %d", FDFTInitInfo.ModelVersion);
}
