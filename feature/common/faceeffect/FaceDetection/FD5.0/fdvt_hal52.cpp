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

#include "fdvt_hal52.h"
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
void* halFDVT52::onBGThreadLoop(void* arg)
{
    halFDVT52 *_this = reinterpret_cast<halFDVT52*>(arg);
    while(1) {
        ::sem_wait(&(_this->mSemBGExec));
        {
            if(_this->mBGStop) {
                ::sem_post(&(_this->mSemBGIdle));
                break;
            }
            Mutex::Autolock _l(_this->mThreadSyncMutex);
            _this->mpMTKFDVTObj->FDVTMainPostPhase();
            _this->mBGBusy = false;
            ::sem_post(&(_this->mSemBGIdle));
        }
    }
    return NULL;
}


/*******************************************************************************
* callback
********************************************************************************/
void FDVTCallback52(EGNParams<FDVTConfig>& rParams)
{
    halFDVT52 *pFDHal = (halFDVT52 *)rParams.mpCookie;
    pFDHal->handleFDCallback(rParams);
    return;
}


// internal function
bool
halFDVT52::handleFDCallback(
    EGNParams<FDVTConfig>& rParams
)
{
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

    return true;
}
//


/*******************************************************************************
* Public API
********************************************************************************/
halFDBase*
halFDVT52::
getInstance()
{
    MY_LOGD("[getInstance()] halFDVT52");
    Mutex::Autolock _l(gLock);
    halFDBase* pHalFD;

    pHalFD = new halFDVT52();

    return  pHalFD;

}


void
halFDVT52::
destroyInstance()
{
    MY_LOGD("[destroyInstance()] halFDVT52");
    Mutex::Autolock _l(gLock);
    delete this;

}


halFDVT52::halFDVT52()
{
    mpMTKFDVTObj = NULL;

    mFDW = 0;
    mFDH = 0;
    mInited = 0;
    mDoDumpImage = 0;
    mFDRefresh = 3;
    mBGStop = 0;
    mAttDir = NSCam::NSIoPipe::DEGREE_0;
    mFDDir = NSCam::NSIoPipe::DEGREE_0;
    mpFDStream = nullptr;
    mworkbufList = nullptr;
    mpMTKFDVTObj = MTKDetection::createInstance(DRV_FD_OBJ_HW);
}


halFDVT52::~halFDVT52()
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
halFDVT52::halFDGetVersion(
)
{
    return HAL_FD_VER_HW52;
}


bool
halFDVT52::doHWAttribute(
    void *pCal_Data __unused,
    void *pOption
)
{
    if (mbHwTimeout) {
        MY_LOGE("FD HW driver timeout...please check...");
        return false;
    }
    int err = OK;
    fd_cal_struct *pfd_cal_data = (fd_cal_struct *)pCal_Data;
    FdOptions *pFDOption = (FdOptions *)pOption;

    int attr = AIE_ATTR_TYPE_GENDER;
    {
        for (int task = 0; task < mAIEAttrTask[attr]; task++)
        {
            if (mtodoList[attr][task] >= MAX_CROP_NUM) {
                MY_LOGE("mtodoList[%d][%d] = %d exceed MAX_CROP_NUM(16), skip this round", attr, task, mtodoList[attr][task]);
                continue;
            }
            int psz = mpatchSize[mtodoList[attr][task]];
            if (psz == 0)
            {
                break;
            }
            {
                Mutex::Autolock __l(mSyncLock);
                Mutex::Autolock _l(mDriverLock);
                mFdvtParams_attr.mpEngineID = NSCam::NSIoPipe::eFDVT;
                mFdvtParams_attr.mpfnCallback = FDVTCallback52;
                mFdvtParams_attr.mpCookie = this;

                mFdvtParams_attr.mEGNConfigVec.at(0).FD_MODE = NSCam::NSIoPipe::ATTRIBUTEMODE;
                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_FMT = NSCam::NSIoPipe::FMT_YUYV;
                mFdvtParams_attr.mEGNConfigVec.at(0).source_img_address_UV = nullptr;

                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_WIDTH = mFDW;
                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_STRIDE = mFDW*2;
                mFdvtParams_attr.mEGNConfigVec.at(0).SRC_IMG_HEIGHT = mFDH;
                mFdvtParams_attr.mEGNConfigVec.at(0).source_img_address = (MUINT64*)pfd_cal_data->srcbuffer_phyical_addr_plane1;
                mFdvtParams_attr.mEGNConfigVec.at(0).INPUT_ROTATE_DEGREE = mAttDir;
                mFdvtParams_attr.mEGNConfigVec.at(0).enROI = true;
                mFdvtParams_attr.mEGNConfigVec.at(0).src_roi.x1 = mworkbufList[mtodoList[attr][task]][0];
                mFdvtParams_attr.mEGNConfigVec.at(0).src_roi.y1 = mworkbufList[mtodoList[attr][task]][1];
                mFdvtParams_attr.mEGNConfigVec.at(0).src_roi.x2 = mworkbufList[mtodoList[attr][task]][2];
                mFdvtParams_attr.mEGNConfigVec.at(0).src_roi.y2 = mworkbufList[mtodoList[attr][task]][3];
                mFdvtParams_attr.mEGNConfigVec.at(0).enPadding = true;
                mFdvtParams_attr.mEGNConfigVec.at(0).src_padding.left = mworkbufList[mtodoList[attr][task]][4];
                mFdvtParams_attr.mEGNConfigVec.at(0).src_padding.up = mworkbufList[mtodoList[attr][task]][5];
                mFdvtParams_attr.mEGNConfigVec.at(0).src_padding.right = mworkbufList[mtodoList[attr][task]][6];
                mFdvtParams_attr.mEGNConfigVec.at(0).src_padding.down = mworkbufList[mtodoList[attr][task]][7];
                //
                mFdvtParams_attr.mEGNConfigVec.at(0).pyramid_base_width = (misSecureFD)? 480 : pFDOption->pyramid_size;
                mFdvtParams_attr.mEGNConfigVec.at(0).pyramid_base_height = mFdvtParams_attr.mEGNConfigVec.at(0).pyramid_base_width * mFDH / mFDW;
                mFdvtParams_attr.mEGNConfigVec.at(0).number_of_pyramid = (misSecureFD)? 3 : pFDOption->pyramid_num;

                mpFDStream->EGNenque(mFdvtParams_attr);

                err = mAttrCond.waitRelative(mDriverLock, 2000000000); // wait 2 sec if time out
                // mFdvtParams.mEGNConfigVec.clear();

                if  ( OK != err ) {
                    // error handling
                    MY_LOGE("FD HW driver run attribute timeout...please check...");
                    mbHwTimeout = true;
                    return false;
                }
            }
            {
                mgender_FM[mtodoList[attr][task]][0] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_GENDER_RESULT.RESULT[0];
                mgender_FM[mtodoList[attr][task]][1] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_GENDER_RESULT.RESULT[1];
                mgender_FM[mtodoList[attr][task]][2] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_RACE_RESULT.RESULT[0];
                mgender_FM[mtodoList[attr][task]][3] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_RACE_RESULT.RESULT[1];
                mgender_FM[mtodoList[attr][task]][4] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_RACE_RESULT.RESULT[2];
                mgender_FM[mtodoList[attr][task]][5] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_IS_INDIAN_RESULT.RESULT[0];
                mgender_FM[mtodoList[attr][task]][6] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_IS_INDIAN_RESULT.RESULT[1];
                mgender_FM[mtodoList[attr][task]][7] = mAttDrvResult.ATTRIBUTEOUTPUT->MERGED_AGE_RESULT.RESULT[0];
                mbufStatus[attr][mtodoList[attr][task]] = 2;
            }
        }
    }

    return true;
}

bool
halFDVT52::doHWFaceDetection(
    void *pCal_Data, void *pOption
)
{
    if (mbHwTimeout) {
        MY_LOGE("FD HW driver timeout...please check...");
        return false;
    }
    Mutex::Autolock __l(mSyncLock);
    Mutex::Autolock _l(mDriverLock);
    fd_cal_struct *pfd_cal_data = (fd_cal_struct *)pCal_Data;
    FdOptions  *pFDOption = (FdOptions *) pOption;
    int err = OK;

    mFdvtParams_face.mpEngineID = NSCam::NSIoPipe::eFDVT;
    mFdvtParams_face.mpfnCallback = FDVTCallback52;
    mFdvtParams_face.mpCookie = this;

    mFdvtParams_face.mEGNConfigVec.at(0).FD_MODE = NSCam::NSIoPipe::FDMODE;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_WIDTH = mFDW;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_STRIDE = mFDW*2;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_HEIGHT = mFDH;
    mFdvtParams_face.mEGNConfigVec.at(0).SRC_IMG_FMT = NSCam::NSIoPipe::FMT_YUYV;
    if (misSecureFD) {
        #if SUPPORT_SECURE_TZMP2
            MUINTPTR fdY = mFD_Y;
            mFdvtParams_face.mEGNConfigVec.at(0).source_img_address = (MUINT64*)fdY;
        #else
            mFdvtParams_face.mEGNConfigVec.at(0).source_img_address =
                (MUINT64*)pfd_cal_data->srcbuffer_phyical_addr_plane1;
        #endif
    } else {
        mFdvtParams_face.mEGNConfigVec.at(0).source_img_address = (MUINT64*)pfd_cal_data->srcbuffer_phyical_addr_plane1;
    }
    mFdvtParams_face.mEGNConfigVec.at(0).source_img_address_UV = nullptr;
    //
    mFdvtParams_face.mEGNConfigVec.at(0).pyramid_base_width = (misSecureFD)? 480 : pFDOption->pyramid_size;
    mFdvtParams_face.mEGNConfigVec.at(0).pyramid_base_height = mFdvtParams_face.mEGNConfigVec.at(0).pyramid_base_width * mFDH / mFDW;
    mFdvtParams_face.mEGNConfigVec.at(0).number_of_pyramid = (misSecureFD)? 3 : pFDOption->pyramid_num;

    if (pFDOption->doROIFD)
    {
        mFdvtParams_face.mEGNConfigVec.at(0).enROI = true;
        mFdvtParams_face.mEGNConfigVec.at(0).src_roi.x1 = pFDOption->ROIFDinfo[0];
        mFdvtParams_face.mEGNConfigVec.at(0).src_roi.y1 = pFDOption->ROIFDinfo[1];
        mFdvtParams_face.mEGNConfigVec.at(0).src_roi.x2 = pFDOption->ROIFDinfo[2];
        mFdvtParams_face.mEGNConfigVec.at(0).src_roi.y2 = pFDOption->ROIFDinfo[3];
        mFdvtParams_face.mEGNConfigVec.at(0).enPadding = true;
        mFdvtParams_face.mEGNConfigVec.at(0).src_padding.left = pFDOption->ROIFDinfo[4];
        mFdvtParams_face.mEGNConfigVec.at(0).src_padding.up = pFDOption->ROIFDinfo[5];
        mFdvtParams_face.mEGNConfigVec.at(0).src_padding.right = pFDOption->ROIFDinfo[6];
        mFdvtParams_face.mEGNConfigVec.at(0).src_padding.down = pFDOption->ROIFDinfo[7];
    }

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
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT->PYRAMID0_RESULT.fd_partial_result = 0;
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT->PYRAMID1_RESULT.fd_partial_result = 0;
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT->PYRAMID2_RESULT.fd_partial_result = 0;

    mpFDStream->EGNenque(mFdvtParams_face);

    err = mFDCond.waitRelative(mDriverLock, 2000000000); // wait 2 sec if time out
    if  ( OK != err ) {
        // error handling
        MY_LOGE("FD HW driver timeout...please check...");
        mbHwTimeout = true;
        return false;
    }

    auto handleFDResult = [&] (NSCam::NSIoPipe::FDRESULT &DrvResult, int Idx) -> int
    {
        MUINT32 i = 0, j = 0;
        for(i = Idx; i < (DrvResult.fd_partial_result + Idx); i++, j++)
        {
            #define CLIP(_x, min, max) \
            _x < min ? min : _x > max ? max : _x

            pfd_cal_data->face_candi_pos_x0[i] = DrvResult.anchor_x0[j];
            pfd_cal_data->face_candi_pos_y0[i] = DrvResult.anchor_y0[j];
            pfd_cal_data->face_candi_pos_x1[i] = DrvResult.anchor_x1[j];
            pfd_cal_data->face_candi_pos_y1[i] = DrvResult.anchor_y1[j];

            pfd_cal_data->fld_leye_x0[i]  = DrvResult.rip_landmark_score0[j];
            pfd_cal_data->fld_leye_y0[i]  = DrvResult.rip_landmark_score1[j];
            pfd_cal_data->fld_leye_x1[i]  = DrvResult.rip_landmark_score2[j];
            pfd_cal_data->fld_leye_y1[i]  = DrvResult.rip_landmark_score3[j];
            pfd_cal_data->fld_reye_x0[i]  = DrvResult.rip_landmark_score4[j];
            pfd_cal_data->fld_reye_y0[i]  = DrvResult.rip_landmark_score5[j];
            pfd_cal_data->fld_reye_x1[i]  = DrvResult.rip_landmark_score6[j];
            pfd_cal_data->fld_reye_y1[i]  = DrvResult.rop_landmark_score0[j];
            pfd_cal_data->fld_nose_x[i]   = DrvResult.rop_landmark_score1[j];
            pfd_cal_data->fld_nose_y[i]   = DrvResult.rop_landmark_score2[j];

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
    FaceNum = handleFDResult(mFdvtDrvResult.FDOUTPUT->PYRAMID0_RESULT, FaceNum);
    FaceNum = handleFDResult(mFdvtDrvResult.FDOUTPUT->PYRAMID1_RESULT, FaceNum);
    FaceNum = handleFDResult(mFdvtDrvResult.FDOUTPUT->PYRAMID2_RESULT, FaceNum);

    if (CC_UNLIKELY(FaceNum != mFdvtDrvResult.FDOUTPUT->FD_TOTAL_NUM))
    {
        MY_LOGW("please check face pyramid total num(%d) != FD_TOTAL_NUM(%d)", FaceNum, mFdvtDrvResult.FDOUTPUT->FD_TOTAL_NUM);
        MY_LOGW("Pyramid0 : %d, Pyramid1 : %d, Pyramid2 : %d", mFdvtDrvResult.FDOUTPUT->PYRAMID0_RESULT.fd_partial_result
                                                             , mFdvtDrvResult.FDOUTPUT->PYRAMID1_RESULT.fd_partial_result
                                                             , mFdvtDrvResult.FDOUTPUT->PYRAMID2_RESULT.fd_partial_result);
    }


    return true;
}


void
halFDVT52::halFDDoByVersion(
    void *pCal_Data, void *pOption
)
{
    fd_cal_struct *pfd_cal_data = (fd_cal_struct *)pCal_Data;
    FdOptions  *pFDOption = (FdOptions *) pOption;
    {
        Mutex::Autolock _l(mThreadSyncMutex);
        // use BG thread to do mpMTKFDVTObj->FDVTMainPostPhase() for landmark detection
        if ( !mBGBusy )
        {
            mBGBusy = true;
            ::sem_post(&mSemBGExec);
        }
    }

    if ( (pFDOption->doGender || pFDOption->doPose) && !misSecureFD && mFdvtDrvResult.FDOUTPUT != nullptr) // run attribute
    {
        if (mFdvtDrvResult.FDOUTPUT->FD_TOTAL_NUM > 0) {
            mpMTKFDVTObj->FDVTMainCropPhaseV2(mtodoList, mbufStatus, mworkbufList, mpatchSize, mAIEAttrTask);
            mAttDir = mFDDir;
            doHWAttribute(pfd_cal_data, pOption);
            mpMTKFDVTObj->FDVTMainJoinPhaseV2(mbufStatus[AIE_ATTR_TYPE_GENDER], mgender_FM, 4);
        }
    }

    {
        ::sem_wait(&mSemBGIdle);
    }
    {
        MUINT32 wait = 0;

        while(mBGBusy && wait < 50)
        {
            usleep(1000);
            wait++;
            MY_LOGD("wait 1ms for landmark");
        }
    }
    mpMTKFDVTObj->FDVTMainJoinPhaseV2(mbufStatus[AIE_ATTR_TYPE_POSE], mgender_FM, -1);
}


MINT32
halFDVT52::allocateAttributeBuffer(
)
{
    mworkbufList = (unsigned short**)malloc(sizeof(unsigned short*) * MAX_CROP_NUM);
    if (mworkbufList == nullptr) {
        MY_LOGE("allocate attribute buffer fail!");
        return MFALSE;
    }
    // crx0 cry0 crx1 cry1 padL padT padR padB
    for (int i = 0; i < MAX_CROP_NUM; i++) {
        mworkbufList[i] = (unsigned short*)malloc(sizeof(unsigned short) * 8);
        if (mworkbufList[i] == nullptr) {
            MY_LOGE("allocate attribute buffer fail!");
            return MFALSE;
        }
    }
    return MTRUE;
}


void
halFDVT52::deallocateAttributeBuffer(
)
{
    if (mworkbufList)
    {
        for (int i = 0; i < MAX_CROP_NUM; i++) free(mworkbufList[i]);
        free(mworkbufList);
        mworkbufList = nullptr;
    }
}


MUINT32
halFDVT52::initFDDriver(
    MUINT32 fdW
)
{
    // initialize hw face config & result
    mFdvtParams_face.mEGNConfigVec.resize(1);
    mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT = new NSCam::NSIoPipe::FD_RESULT;
    mFdvtParams_face.mEGNConfigVec.at(0).ATTRIBUTEOUTPUT = new NSCam::NSIoPipe::ATTRIBUTE_RESULT;
    mFdvtParams_face.mEGNConfigVec.at(0).POSEOUTPUT = new NSCam::NSIoPipe::POSE_RESULT;

    // initialize hw attribute config & result
    mFdvtParams_attr.mEGNConfigVec.resize(1);
    mFdvtParams_attr.mEGNConfigVec.at(0).FDOUTPUT = new NSCam::NSIoPipe::FD_RESULT;
    mFdvtParams_attr.mEGNConfigVec.at(0).ATTRIBUTEOUTPUT = new NSCam::NSIoPipe::ATTRIBUTE_RESULT;
    mFdvtParams_attr.mEGNConfigVec.at(0).POSEOUTPUT = new NSCam::NSIoPipe::POSE_RESULT;

    mpFDStream = NSCam::NSIoPipe::NSEgn::IEgnStream<FDVTConfig>::createInstance(LOG_TAG);

    if (mpFDStream == nullptr) {
        MY_LOGE("Create EgnStream Fail");
        return 0;
    }
    INITParams<FDVTConfig> InitParams;
    InitParams.srcImg_maxWidth = fdW;
    InitParams.srcImg_maxHeight = fdW * 1.5;
    InitParams.feature_threshold = (-64);
    InitParams.pyramid_width = 504;
    InitParams.pyramid_height = 640;
    InitParams.isFdvtSecure = misSecureFD;
    #if SUPPORT_SECURE_TZMP2
        if (misSecureFD) InitParams.sec_mem_type = 3;
    #endif

    mpFDStream->init(InitParams);

    return InitParams.fd_version;
}


struct tuning_para*
halFDVT52::getTuningParaByVersion(
)
{
    MY_LOGD("[tuning_para] tuning_52");
    return &tuning_52;
}


void
halFDVT52::createBGThread(
)
{
    MY_LOGD("[createBGThread] BGThreadLoop_v52");
    pthread_create(&mBGThread, NULL, onBGThreadLoop, this);
    pthread_setname_np(mBGThread, "FDV52_BGThread");
}


void
halFDVT52::uninitFDDriver(
)
{
    delete(mFdvtParams_face.mEGNConfigVec.at(0).FDOUTPUT);
    delete(mFdvtParams_face.mEGNConfigVec.at(0).ATTRIBUTEOUTPUT);
    delete(mFdvtParams_face.mEGNConfigVec.at(0).POSEOUTPUT);

    delete(mFdvtParams_attr.mEGNConfigVec.at(0).FDOUTPUT);
    delete(mFdvtParams_attr.mEGNConfigVec.at(0).ATTRIBUTEOUTPUT);
    delete(mFdvtParams_attr.mEGNConfigVec.at(0).POSEOUTPUT);

    mpFDStream->uninit();
    mpFDStream->destroyInstance(LOG_TAG);
}

MUINT8
halFDVT52::getFDVersionForInit(
)
{
    return MTKCAM_HWFD_MAIN_VERSION + 2;
}
