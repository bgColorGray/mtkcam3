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

#include "fdvt_hal51.h"
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

#define ENABLE_FD_DEBUG_LOG (1)
#define USE_SW_FD_TO_DEBUG (0)
#if (MTKCAM_FDFT_USE_HW == '1') && (USE_SW_FD_TO_DEBUG == 0)
#define USE_HW_FD (1)

#define USE_HW_FD (0)
#endif

#define MHAL_NO_ERROR 0
#define MHAL_INPUT_SIZE_ERROR 1
#define MHAL_UNINIT_ERROR 2
#define MHAL_REINIT_ERROR 3

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
* static function
********************************************************************************/
void* halFDVT51::onBGThreadLoop(void* arg)
{
    halFDVT51 *_this = reinterpret_cast<halFDVT51*>(arg);
    while(1) {
        ::sem_wait(&(_this->mSemBGExec));
        if(_this->mBGStop) {
            break;
        }
        _this->doHWAttribute(nullptr);
        _this->mBGBusy = false;
    }
    return NULL;
}


/*******************************************************************************
* callback
********************************************************************************/
void FDVTCallback51(EGNParams<FDVTConfig>& rParams)
{
    halFDVT51 *pFDHal = (halFDVT51 *)rParams.mpCookie;
    pFDHal->handleFDCallback(rParams);
    return;
}


// internal function
bool
halFDVT51::handleFDCallback(
    EGNParams<FDVTConfig>& rParams
)
{
    MY_LOGD("Base Callback Start!");
    Mutex::Autolock _l(mDriverLock);
    if (rParams.mEGNConfigVec.size() > 0)
    {
        if (rParams.mEGNConfigVec[0].FD_MODE == NSCam::NSIoPipe::FDMODE)
        {
            mFdvtDrvResult = rParams.mEGNConfigVec[0];
            mFDCond.signal();
        }
        else
        {
            mAttDrvResult = rParams.mEGNConfigVec[0];
            mAttrCond.signal();
        }
    }
    MY_LOGD("Base Callback End!");

    return true;
}
//


/*******************************************************************************
* Public API
********************************************************************************/
halFDBase*
halFDVT51::
getInstance()
{
    MY_LOGD("[getInstance()] halFDVT51");
    Mutex::Autolock _l(gLock);
    halFDBase* pHalFD;

    pHalFD = new halFDVT51();

    return  pHalFD;

}

void
halFDVT51::
destroyInstance()
{
    MY_LOGD("[destroyInstance()] halFDVT51");
    Mutex::Autolock _l(gLock);
    delete this;

}

halFDVT51::halFDVT51()
{
    mpMTKFDVTObj = NULL;

    mFDW = 0;
    mFDH = 0;
    mInited = 0;
    mDoDumpImage = 0;
    mFDRefresh = 3;
    mpAttrWorkBuf = NULL;
    mAllocator = NULL;
    mBGStop = 0;
    mAttDir = NSCam::NSIoPipe::DEGREE_0;
    mFDDir = NSCam::NSIoPipe::DEGREE_0;
    mworkbufList = nullptr;
    mpFDStream = nullptr;
    mpMTKFDVTObj = MTKDetection::createInstance(DRV_FD_OBJ_HW);
}

halFDVT51::~halFDVT51()
{
    mFDW = 0;
    mFDH = 0;

    if (mpMTKFDVTObj) {
        mpMTKFDVTObj->destroyInstance();
    }
    MY_LOGD("[Destroy] mpMTKFDVTObj->destroyInstance");

    mpMTKFDVTObj = NULL;
}


MINT32
halFDVT51::halFDGetVersion(
)
{
    return HAL_FD_VER_HW51;
}


// internal function
bool
halFDVT51::doHWAttribute(
    void *pCal_Data __unused
)
{
    if (mbHwTimeout) {
        MY_LOGE("FD HW driver timeout...please check...");
        return false;
    }

    int err = OK;
    fd_cal_struct *pfd_cal_data = (fd_cal_struct *)pCal_Data;

    mpAttrWorkBuf->syncCache();
    for (int attr = 0; attr < MAX_AIE_ATTR_TYPE; attr++)

    {
        for (int task = 0; task < mAIEAttrTask[attr]; task++)
        {
            int psz = mpatchSize[mtodoList[attr][task]];
            if (psz == 0)
            {
                break;
            }
            {
                mPriorityLock.lock();
                Mutex::Autolock __l(mSyncLock);
                mPriorityLock.unlock();
                Mutex::Autolock _l(mDriverLock);
                mFdvtParams_attr.mpEngineID = NSCam::NSIoPipe::eFDVT;
                mFdvtParams_attr.mpfnCallback = FDVTCallback51;
                mFdvtParams_attr.mpCookie = this;

                mFdvtParams_attr.mEGNConfigVec.at(0).FD_MODE = (attr == AIE_ATTR_TYPE_GENDER) ? NSCam::NSIoPipe::ATTRIBUTEMODE : NSCam::NSIoPipe::POSEMODE;
                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_FMT = NSCam::NSIoPipe::FMT_YUYV;
                mFdvtParams_attr.mEGNConfigVec.at(0).source_img_address_UV = nullptr;

                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_WIDTH = psz;
                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_STRIDE = psz;
                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_HEIGHT = psz;
                mFdvtParams_attr.mEGNConfigVec.at(0).source_img_address = (MUINT64*)mworkbufListPA[mtodoList[attr][task]];
                mFdvtParams_attr.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = NSCam::NSIoPipe::DEGREE_0;

                mpFDStream->EGNenque(mFdvtParams_attr);

                err = mAttrCond.waitRelative(mDriverLock, 2000000000); // wait 2 sec if time out
                // mFdvtParams_attr.mEGNConfigVec.clear();

                if  ( OK != err ) {
                    // error handling
                    MY_LOGE("FD HW driver run attribute timeout...please check...");
                    mbHwTimeout = true;
                    return false;
                }
            }
            {
                Mutex::Autolock _l(mThreadSyncMutex);
                if (attr == AIE_ATTR_TYPE_GENDER)
                {
                    memcpy(mgender_FM[mtodoList[attr][task]], &(mAttDrvResult.ATTRIBUTEOUTPUT), sizeof(struct NSCam::NSIoPipe::ATTRIBUTE_RESULT));
                }
                else
                {
                    memcpy(mpose_FM[mtodoList[attr][task]], &(mAttDrvResult.POSEOUTPUT), sizeof(struct NSCam::NSIoPipe::POSE_RESULT));
                }

                mbufStatus[attr][mtodoList[attr][task]] = 2;
            }
        }
    }

    return true;
}

bool
halFDVT51::doHWFaceDetection(
    void *pCal_Data, void *pOption
)
{
    if (mbHwTimeout) {
        MY_LOGE("FD HW driver timeout...please check...");
        return false;
    }
    std::lock_guard<std::mutex> ___1(mPriorityLock);
    Mutex::Autolock __l(mSyncLock);
    Mutex::Autolock _l(mDriverLock);
    fd_cal_struct *pfd_cal_data = (fd_cal_struct *)pCal_Data;
    //FdOptions  *pFDOption = (FdOptions *) pOption;
    int err = OK;

    mFdvtParams_face.mpEngineID = NSCam::NSIoPipe::eFDVT;
    mFdvtParams_face.mpfnCallback = FDVTCallback51;
    mFdvtParams_face.mpCookie = this;

    mFdvtParams_face.mEGNConfigVec.at(0).FD_MODE = NSCam::NSIoPipe::FDMODE;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_WIDTH = mFDW;
    //mFdvtParams.mEGNConfigVec.at(0).SRC_IMG_STRIDE = mFDW*2;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_HEIGHT = mFDH;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_FMT = NSCam::NSIoPipe::FMT_YUYV;
    mFdvtParams_face.mEGNConfigVec.at(0).source_img_address = (MUINT64*)pfd_cal_data->srcbuffer_phyical_addr_plane1;
    mFdvtParams_face.mEGNConfigVec.at(0).source_img_address_UV = nullptr;

    switch(pfd_cal_data->current_feature_index)
    {
        case 1:
        case 2:
        case 3:
            mFDDir = mFdvtParams_face.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = NSCam::NSIoPipe::DEGREE_0;
            break;
        case 4:
        case 6:
        case 8:
            mFDDir = mFdvtParams_face.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = NSCam::NSIoPipe::DEGREE_90;
            break;
        case 5:
        case 7:
        case 9:
            mFDDir = mFdvtParams_face.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = NSCam::NSIoPipe::DEGREE_270;
            break;
        case 10:
        case 11:
        case 12:
            mFDDir = mFdvtParams_face.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = NSCam::NSIoPipe::DEGREE_180;
            break;
        default:
            MY_LOGW("error fd direction : %d", pfd_cal_data->current_feature_index);
            mFDDir = mFdvtParams_face.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = NSCam::NSIoPipe::DEGREE_0;
            break;
    }
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT.PYRAMID0_RESULT.fd_partial_result = 0;
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT.PYRAMID1_RESULT.fd_partial_result = 0;
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT.PYRAMID2_RESULT.fd_partial_result = 0;

    mpFDStream->EGNenque(mFdvtParams_face);

    err = mFDCond.waitRelative(mDriverLock, 2000000000); // wait 2 sec if time out
    if  ( OK != err ) {
        // error handling
        MY_LOGE("FD HW driver timeout...please check...");
        mbHwTimeout = true;
        return false;
    }
    float widthRatio = (float)mFDhw_W / (float)mFDW;
    float heightRatio = (float)mFDhw_H / (float)mFDH;

    auto handleFDResult = [&] (NSCam::NSIoPipe::FDRESULT &DrvResult, int Idx) -> int
    {
        MUINT32 i = 0, j = 0;
        MY_LOGD_IF(ENABLE_FD_DEBUG_LOG, "debug : Idx=%d, result=%d", Idx, DrvResult.fd_partial_result);
        for(i = Idx; i < (DrvResult.fd_partial_result + Idx); i++, j++)
        {
            #define CLIP(_x, min, max) \
            _x < min ? min : _x > max ? max : _x

            pfd_cal_data->face_candi_pos_x0[i] = CLIP((int)(DrvResult.anchor_x0[j] * widthRatio), 0, mFDhw_W - 1);
            pfd_cal_data->face_candi_pos_y0[i] = CLIP((int)(DrvResult.anchor_y0[j] * heightRatio), 0, mFDhw_H - 1);
            pfd_cal_data->face_candi_pos_x1[i] = CLIP((int)(DrvResult.anchor_x1[j] * widthRatio), 0, mFDhw_W - 1);
            pfd_cal_data->face_candi_pos_y1[i] = CLIP((int)(DrvResult.anchor_y1[j] * heightRatio), 0, mFDhw_H - 1);
            //
            pfd_cal_data->face_reliabiliy_value[i] = DrvResult.anchor_score[j] + 3600;
            pfd_cal_data->display_flag[i] = (kal_bool)1;
            pfd_cal_data->face_feature_set_index[i] = pfd_cal_data->current_feature_index;
            pfd_cal_data->rip_dir[i] = pfd_cal_data->current_feature_index;
            pfd_cal_data->rop_dir[i] = 0;
            pfd_cal_data->result_type[i] = GFD_RST_TYPE;

        }
        return i;
    };
    int FaceNum = 0;
    FaceNum = handleFDResult(mFdvtDrvResult.FDOUTPUT.PYRAMID0_RESULT, FaceNum);
    FaceNum = handleFDResult(mFdvtDrvResult.FDOUTPUT.PYRAMID1_RESULT, FaceNum);
    FaceNum = handleFDResult(mFdvtDrvResult.FDOUTPUT.PYRAMID2_RESULT, FaceNum);

    if (CC_UNLIKELY(FaceNum != mFdvtDrvResult.FDOUTPUT.FD_TOTAL_NUM))
    {
        MY_LOGW("please check face pyramid total num(%d) != FD_TOTAL_NUM(%d)", FaceNum, mFdvtDrvResult.FDOUTPUT.FD_TOTAL_NUM);
        MY_LOGW("Pyramid0 : %d, Pyramid1 : %d, Pyramid2 : %d", mFdvtDrvResult.FDOUTPUT.PYRAMID0_RESULT.fd_partial_result
                                                             , mFdvtDrvResult.FDOUTPUT.PYRAMID1_RESULT.fd_partial_result
                                                             , mFdvtDrvResult.FDOUTPUT.PYRAMID2_RESULT.fd_partial_result);
    }


    return true;
}
//


void
halFDVT51::halFDDoByVersion(
    void *pCal_Data, void *pOption
)
{
    MY_LOGD("[halFDDoByVersion] fd version 51");
    fd_cal_struct *pfd_cal_data = (fd_cal_struct *)pCal_Data;
    FdOptions  *pFDOption = (FdOptions *) pOption;
    if ( (pFDOption->doGender || pFDOption->doPose) && !misSecureFD && mFdvtDrvResult.FDOUTPUT.FD_TOTAL_NUM > 0) // run attribute
    {
        //
        if (gGenderDebug)
        {
            mpMTKFDVTObj->FDVTMainCropPhase(mtodoList, mbufStatus, mworkbufList, mpatchSize, mAIEAttrTask);
            mAttDir = mFDDir;
            doHWAttribute(nullptr);
            mpMTKFDVTObj->FDVTMainJoinPhase(mbufStatus[AIE_ATTR_TYPE_GENDER], mgender_FM, AIE_ATTR_TYPE_GENDER);
            mpMTKFDVTObj->FDVTMainJoinPhase(mbufStatus[AIE_ATTR_TYPE_POSE], mpose_FM, AIE_ATTR_TYPE_POSE);
        }

        else
        {
            Mutex::Autolock _l(mThreadSyncMutex);
            if ( !mBGBusy )
            {
                mBGBusy = true;
                mpMTKFDVTObj->FDVTMainJoinPhase(mbufStatus[AIE_ATTR_TYPE_GENDER], mgender_FM, AIE_ATTR_TYPE_GENDER);
                mpMTKFDVTObj->FDVTMainJoinPhase(mbufStatus[AIE_ATTR_TYPE_POSE], mpose_FM, AIE_ATTR_TYPE_POSE);
                mpMTKFDVTObj->FDVTMainCropPhase(mtodoList, mbufStatus, mworkbufList, mpatchSize, mAIEAttrTask);
                mAttDir = mFDDir;
                ::sem_post(&mSemBGExec);
            }
        }
    }
    mpMTKFDVTObj->FDVTMainPostPhase();
    mpMTKFDVTObj->FDVTMainJoinPhase(mbufStatus[AIE_ATTR_TYPE_POSE], mpose_FM, -1);
}


MINT32
halFDVT51::allocateAttributeBuffer(

)
{
    if (mGenderNum || mPoseNum)
    {
        mworkbufList = (unsigned char**)malloc(sizeof(unsigned char*) * MAX_CROP_NUM);
        mAllocator = IImageBufferAllocator::getInstance();
        IImageBufferAllocator::ImgParam imgParam(MAX_CROP_NUM * sizeof(unsigned char) * MAX_CROP_W * MAX_CROP_W * 2, 16);
        mpAttrWorkBuf = mAllocator->alloc("FDAttributeBuf", imgParam);
        if ( mpAttrWorkBuf.get() == 0 )
        {
            MY_LOGE("NULL Buffer\n");
            return MFALSE;
        }
        if ( !mpAttrWorkBuf->lockBuf( "FDAttributeBuf", (eBUFFER_USAGE_HW_CAMERA_READ | eBUFFER_USAGE_SW_MASK)) )
        {
            MY_LOGE("lock Buffer failed\n");
            return MFALSE;
        }
        MY_LOGD("allocator buffer : %" PRIXPTR "", mpAttrWorkBuf->getBufVA(0));
        for(int i = 0; i < MAX_CROP_NUM; i++)
        {
            mworkbufList[i] = (unsigned char*)(mpAttrWorkBuf->getBufVA(0) + (i * sizeof(unsigned char) * MAX_CROP_W * MAX_CROP_W * 2));
            mworkbufListPA[i] = (mpAttrWorkBuf->getBufPA(0) + (i * sizeof(unsigned char) * MAX_CROP_W * MAX_CROP_W * 2));
            MY_LOGD("mworkbufList[%d] : %p", i, mworkbufList[i]);
        }
    }
    return MTRUE;
}


void
halFDVT51::deallocateAttributeBuffer(
)
{
    if(mpAttrWorkBuf != NULL && mAllocator != NULL) {
        mpAttrWorkBuf->unlockBuf("FDAttributeBuf");
        mAllocator->free(mpAttrWorkBuf.get());
        mpAttrWorkBuf = NULL;
    }
    if(mworkbufList) {
        free(mworkbufList);
        mworkbufList = nullptr;
    }
}


MUINT32
halFDVT51::initFDDriver(
    MUINT32 fdW
)
{
    // initialize hw face config & result
    mFdvtParams_face.mEGNConfigVec.resize(1);

    // initialize hw attribute config & result
    mFdvtParams_attr.mEGNConfigVec.resize(1);

    mpFDStream = NSCam::NSIoPipe::NSEgn::IEgnStream<FDVTConfig>::createInstance(LOG_TAG);

    if (mpFDStream == nullptr) {
        MY_LOGE("[initFDDriver] mpFDStream is nullptr");
        return 0;
    }

    INITParams<FDVTConfig> InitParams;
    InitParams.srcImg_maxWidth = fdW;
    InitParams.srcImg_maxHeight = fdW * 1.5;
    InitParams.feature_threshold = (-64);
    InitParams.isFdvtSecure = misSecureFD;
    mpFDStream->init(InitParams);

    return 51;
}


struct tuning_para*
halFDVT51::getTuningParaByVersion(
)
{
    MY_LOGD("[tuning_para] tuning_51");
    return &tuning_51;
}


void
halFDVT51::createBGThread(
)
{
    MY_LOGD("[createBGThread] BGThreadLoop_v51");
    pthread_create(&mBGThread, NULL, onBGThreadLoop, this);
    pthread_setname_np(mBGThread, "FDV51_BGThread");
}


void
halFDVT51::uninitFDDriver(
)
{
    mpFDStream->uninit();
    mpFDStream->destroyInstance(LOG_TAG);
}


MUINT8
halFDVT51::getFDVersionForInit(
)
{
    return MTKCAM_HWFD_MAIN_VERSION + 1;
}
