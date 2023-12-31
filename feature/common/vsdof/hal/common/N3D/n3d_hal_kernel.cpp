/*********************************************************************************************
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
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#include <stdlib.h>

#include "n3d_hal_kernel.h"         // For N3D_HAL class.
#include "../inc/stereo_dp_util.h"
#include <stereo_tuning_provider.h>
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_VSDOF_HAL);

using android::Mutex;           // For android::Mutex in stereo_hal.h.

/**************************************************************************
 *                      D E F I N E S / M A C R O S                       *
 **************************************************************************/

/**************************************************************************
 *     E N U M / S T R U C T / T Y P E D E F    D E C L A R A T I O N     *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *       P R I V A T E    F U N C T I O N    D E C L A R A T I O N        *
 **************************************************************************/
Mutex N3D_HAL_KERNEL::__sLock;
Mutex N3D_HAL_KERNEL::__sLogLock;

inline bool checkImageBuffer(IImageBuffer *image, const char *bufferName, int planeCountToCheck=1)
{
    if(image == nullptr) {
        MY_LOGE("%s is NULL", bufferName);
        return false;
    }

    if(planeCountToCheck > 0)
    {
        for(int p = 0; p < planeCountToCheck; ++p) {
            if(image->getBufVA(p) == 0) {
                MY_LOGE("%s, plane %d is NULL", bufferName, p);
                return false;
            }
        }
    }
    else
    {
        if(image->getImageBufferHeap() == nullptr ||
           image->getImageBufferHeap()->getHWBuffer() == nullptr)
        {
            MY_LOGE("%s, getHWBuffer is NULL", bufferName);
            return false;
        }
    }

    return true;
}

/**************************************************************************
 *       Public Functions                                                 *
 **************************************************************************/
void FEFM_RING_BUFFER_T::setHWFEFMData(HWFEFM_DATA &data)
{
    MUINT8 *pos = addr;
    for(int i = 0; i < MAX_GEO_LEVEL; i++) {
        if(data.geoDataMain1[i]) {
            feMain1[i].addr = pos;
            feMain1[i].size = data.geoDataMain1[i]->getBufSizeInBytes(0);
            ::memcpy(pos, (void *)data.geoDataMain1[i]->getBufVA(0), feMain1[i].size);
            pos += feMain1[i].size;

            totalLevel = i;
        }

        if(data.geoDataMain2[i]) {
            feMain2[i].addr = pos;
            feMain2[i].size = data.geoDataMain2[i]->getBufSizeInBytes(0);
            ::memcpy(pos, (void *)data.geoDataMain2[i]->getBufVA(0), feMain2[i].size);
            pos += feMain2[i].size;
        }

        if(data.geoDataLeftToRight[i]) {
            fmLR[i].addr = pos;
            fmLR[i].size = data.geoDataLeftToRight[i]->getBufSizeInBytes(0);
            ::memcpy(pos, (void *)data.geoDataLeftToRight[i]->getBufVA(0), fmLR[i].size);
            pos += fmLR[i].size;
        }

        if(data.geoDataRightToLeft[i]) {
            fmRL[i].addr = pos;
            fmRL[i].size = data.geoDataRightToLeft[i]->getBufSizeInBytes(0);
            ::memcpy(pos, (void *)data.geoDataRightToLeft[i]->getBufVA(0), fmRL[i].size);
            pos += fmRL[i].size;
        }
    }
}

N3D_HAL_KERNEL::N3D_HAL_KERNEL()
    : __fastLogger(LOG_TAG, LOG_PERPERTY)
{
    ::memset(&__algoInitInfo, 0, sizeof(__algoInitInfo));
    ::memset(&__algoProcInfo, 0, sizeof(__algoProcInfo));
    ::memset(&__algoTuningInfo, 0, sizeof(__algoTuningInfo));
    ::memset(&__algoResult, 0, sizeof(__algoResult));
    ::memset(&__algoWorkBufInfo, 0, sizeof(__algoWorkBufInfo));
}

N3D_HAL_KERNEL::N3D_HAL_KERNEL(N3D_HAL_KERNEL_INIT_PARAM n3dInitParam)
    : __fastLogger(LOG_TAG, LOG_PERPERTY)
{
    init(n3dInitParam);
}

N3D_HAL_KERNEL::~N3D_HAL_KERNEL()
{
    Mutex::Autolock lock(__sLock);    //To protect NVRAM access and staic instance

    __wakeupLearning = true;
    __hasFEFM = false;
    __learningCondVar.notify_all(); //wakeup __learningThread

    __wakeupGetRectInS = true;
    __rectInSCondVar.notify_all(); //wakeup __rectInSThread

    if(__learningThread.joinable()) {
        MY_LOGD("Wait learning thread to stop");
        __learningThread.join();
        MY_LOGD("Learning thread stopped");
    } else {
        MY_LOGD("Learning thread is not joinable");
    }

    if(__rectInSThread.joinable()) {
        MY_LOGD("Wait getRectInS thread to stop");
        __rectInSThread.join();
        MY_LOGD("GetRectInS thread stopped");
    } else {
        MY_LOGD("GetRectInS thread is not joinable");
    }

    if(__isInit &&
       !__DISABLE_GPU)
    {
        //Unregister GPU buffer
        if(__useWPE[1]) {
            __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_DESTROY_GPU_BUF, &__algoProcInfo, NULL);
        } else {
            __algoProcInfo.buffer_number = __n3dInitParam.inputImageBuffers.size();
            __algoProcInfo.GPU_BUF_IS_SRC = true;
            __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_DESTROY_GPU_BUF, &__algoProcInfo, NULL);

            __algoProcInfo.buffer_number = __n3dInitParam.outputImageBuffers.size();
            __algoProcInfo.GPU_BUF_IS_SRC = false;
            __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_DESTROY_GPU_BUF, &__algoProcInfo, NULL);
        }

        //Destroy GPU context
        __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_DESTROY_GPU_CTX, &__algoProcInfo, NULL);
    }

    if(__main1Mask) {
        delete [] __main1Mask;
        __main1Mask = NULL;
    }

    if(__inputMain2CPU) {
        delete [] __inputMain2CPU;
        __inputMain2CPU = NULL;
    }

    if(__pStereoDrv) {
        delete __pStereoDrv;
        __pStereoDrv = NULL;
    }

    StereoDpUtil::freeImageBuffer(LOG_TAG, __workBufImage);
    StereoDpUtil::freeImageBuffer(LOG_TAG, __fefmRingBufferImage);
}

void
N3D_HAL_KERNEL::init(N3D_HAL_KERNEL_INIT_PARAM n3dInitParam)
{
    MY_LOGD("+");
    ::memset(&__algoInitInfo, 0, sizeof(__algoInitInfo));
    ::memset(&__algoProcInfo, 0, sizeof(__algoProcInfo));
    ::memset(&__algoTuningInfo, 0, sizeof(__algoTuningInfo));
    ::memset(&__algoResult, 0, sizeof(__algoResult));
    ::memset(&__algoWorkBufInfo, 0, sizeof(__algoWorkBufInfo));

    __fastLogger.setSingleLineMode(SINGLE_LINE_LOG);
    if(!__RUN_N3D) {
        return;
    }

    __n3dInitParam = std::move(n3dInitParam);
    __eScenario    = __n3dInitParam.eScenario;

    __pStereoDrv = MTKStereoKernel::createInstance();
    CAM_ULOG_ASSERT(NSCam::Utils::ULog::MOD_VSDOF_HAL, __pStereoDrv != nullptr, "Create N3D instance fail!");
    MY_LOGD("pStereoDrv(%p)", static_cast<void*>(__pStereoDrv));

    switch(__n3dInitParam.eScenario) {
        case eSTEREO_SCENARIO_CAPTURE:
            __algoInitInfo.scenario = STEREO_KERNEL_SCENARIO_IMAGE_CAPTURE;
            break;
        case eSTEREO_SCENARIO_PREVIEW:
        case eSTEREO_SCENARIO_RECORD:
        default:
            // __algoInitInfo.scenario = STEREO_KERNEL_SCENARIO_IMAGE_PREVIEW;
            __algoInitInfo.scenario = STEREO_KERNEL_SCENARIO_VIDEO_RECORD;
            break;
    }

    __learningThread = std::thread([this]{__runN3DLearningThread();});
    __rectInSThread = std::thread([this]{__getRectInSThread();});
    MY_LOGD("-");
}

bool
N3D_HAL_KERNEL::initN3D(STEREO_KERNEL_SET_ENV_INFO_STRUCT &n3dInitInfo, NVRAM_CAMERA_GEOMETRY_STRUCT *nvram, float *LDC)
{
    MY_LOGD("+");
    __initN3DKernel(n3dInitInfo, nvram, LDC);
    __createFEFMRingBuffer();

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_KERNEL::initModel()
{
    MRESULT err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_MODEL_INIT, NULL, NULL);
    if(err) {
        MY_LOGD("STEREO_KERNEL_FEATURE_MODEL_INIT failed, err: %d", err);
    }

    return !err;
}

void
N3D_HAL_KERNEL::initMain1Mask(StereoArea areaMask)
{
    if(__main1Mask) {
        return;
    }

    MY_LOGD("+");
    //init __main1Mask
    __areaMask = areaMask;
    MUINT32 length = __areaMask.size.w * __areaMask.size.h;
    if(length > 0) {
        if(NULL == __main1Mask) {
            __main1Mask = new MUINT8[length];
            ::memset(__main1Mask, 0, sizeof(MUINT8)*length);
            MUINT8 *startPos = __main1Mask + __areaMask.startPt.x+__areaMask.size.w*__areaMask.startPt.y;
            const MUINT32 END_Y = __areaMask.contentSize().h;
            const MUINT32 CONTENT_W = __areaMask.contentSize().w * sizeof(MUINT8);
            for(unsigned int y = 0; y < END_Y; y++) {
                ::memset(startPos, 0xFF, CONTENT_W);
                startPos += __areaMask.size.w;
            }
        }
    } else {
        MY_LOGE("Size of mask is 0");
    }
    MY_LOGD("-");
}

bool
N3D_HAL_KERNEL::WarpMain1(IImageBuffer *main1Input, IImageBuffer *main1Output, IImageBuffer *main1Mask)
{
    if( !checkImageBuffer(main1Input,  "main1Input")  ||
        !checkImageBuffer(main1Output, "main1Output") ||
        !checkImageBuffer(main1Mask,   "main1Mask") )
    {
        return false;
    }

    AutoProfileUtil profile(LOG_TAG, "Warp main1");

    StereoArea newArea(main1Output->getImgSize());
    MSize srcContentSize = MSize(__algoInitInfo.iio_main.src_w, __algoInitInfo.iio_main.src_h);

    newArea.padding = newArea.size - srcContentSize;
    if(newArea.padding.w < 0 || newArea.padding.h < 0) {
        MY_LOGE("Input size (%d,%d) > output size (%dx%d)",
                srcContentSize.w, srcContentSize.h,
                newArea.size.w, newArea.size.h);
        return false;
    }

    newArea.startPt.x = newArea.padding.w/2;
    newArea.startPt.y = newArea.padding.h/2;

    if(__areaMask.size.w != main1Mask->getImgSize().w ||
       __areaMask.size.h != main1Mask->getImgSize().h)
    {
        MY_LOGE("Mask size mismatch: input size %dx%d, init size: %dx%d",
               main1Mask->getImgSize().w, main1Mask->getImgSize().h,
               __areaMask.size.w, __areaMask.size.h);
        return false;
    }

    ::memcpy((void*)main1Mask->getBufVA(0), __main1Mask, newArea.size.w * newArea.size.h * sizeof(MUINT8));

#if (HAS_P1YUV > 0)
    //P1 YUV does not rotate, so we have to rotate here
    StereoDpUtil::rotateBuffer(LOG_TAG,
                               (MUINT8*)main1Input->getBufVA(0),
                               main1Input->getImgSize(),
                               (MUINT8*)main1Output->getBufVA(0),
                               StereoSettingProvider::getModuleRotation(),
                               newArea);
#else
    MUINT8 *pImgIn = (MUINT8*)main1Input->getBufVA(0);
    MUINT8 *pImgOut = (MUINT8*)main1Output->getBufVA(0);
    int inputImageWidth = main1Input->getImgSize().w;
    ::memset(pImgOut, 0, newArea.size.w * newArea.size.h);
    //Copy Y
    pImgOut = pImgOut + newArea.size.w * newArea.startPt.y + newArea.startPt.x;
    for(int i = srcContentSize.h-1; i >= 0; --i, pImgOut += newArea.size.w, pImgIn += inputImageWidth)
    {
        ::memcpy(pImgOut, pImgIn, srcContentSize.w);
    }

    // MY_LOGD("srcContentSize %dx%d", srcContentSize.w, srcContentSize.h);
    // MY_LOGD("newArea size %dx%d, padding %dx%d, startPt (%d,%d)", newArea.size.w, newArea.size.h,
    //         newArea.padding.w, newArea.padding.h, newArea.startPt.x, newArea.startPt.y);

    //Copy UV
    if(3 == main1Input->getPlaneCount() &&
       3 == main1Output->getPlaneCount())
    {
        newArea /= 2;
        srcContentSize.w /= 2;
        srcContentSize.h /= 2;
        inputImageWidth /= 2;
        for(int p = 1; p < 3; ++p) {
            pImgIn = (MUINT8*)main1Input->getBufVA(p);
            pImgOut = (MUINT8*)main1Output->getBufVA(p);
            ::memset(pImgOut, 0, newArea.size.w * newArea.size.h);

            pImgOut = pImgOut + newArea.size.w * newArea.startPt.y + newArea.startPt.x;
            for(int i = srcContentSize.h-1; i >= 0; --i, pImgOut += newArea.size.w, pImgIn += inputImageWidth)
            {
                // MY_LOGD("plane %d col %d, pImgIn %p, pImgOut %p", p, i, pImgIn, pImgOut);
                ::memcpy(pImgOut, pImgIn, srcContentSize.w);
            }
        }
    }
#endif

    return true;
}

bool
N3D_HAL_KERNEL::WarpMain2(N3D_HAL_PARAM &n3dParams, STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain1, STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain2,
                          N3D_HAL_OUTPUT &n3dOutput)
{
    if(!__RUN_N3D) {
        return false;
    }

    if(!__isInit) {
        __doStereoKernelInit();
    }

    if(afInfoMain1) {
        __algoProcInfo.af_main = *afInfoMain1;
    }

    if(afInfoMain2) {
        __algoProcInfo.af_auxi = *afInfoMain2;
    }

    __setN3DParams(n3dParams, n3dOutput);
    __runN3DCommon(E_WARPING_STAGE);

    std::string dumpPath = __getN3DLogPath();
    if(__hasFEFM) {
        std::unique_lock<std::mutex> lk(__learningMutex); //wait last learning to finish if needed
        __wakeupLearning = true;
        lk.unlock();
        __learningCondVar.notify_all(); //run __runN3DLearningThread
    } else {
        __dumpN3DLog(dumpPath);
    }

    __updateSceneInfo(n3dOutput);
    n3dOutput.convOffset = ((MFLOAT*)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[2];
    n3dOutput.distance = ((MFLOAT*)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[3];

    return true;
}

bool
N3D_HAL_KERNEL::WarpMain2(N3D_HAL_PARAM_CAPTURE &n3dParams, STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain1, STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain2,
                          N3D_HAL_OUTPUT_CAPTURE &n3dOutput, STEREO_KERNEL_RESULT_STRUCT &algoResult)
{
    if(!__RUN_N3D) {
        return false;
    }

    if(!__isInit) {
        __doStereoKernelInit();
    }

    AutoProfileUtil profile(LOG_TAG, "N3DHALRun(Capture)");

    bool isSuccess  = false;

    if(afInfoMain1) {
        __algoProcInfo.af_main = *afInfoMain1;
    }

    if(afInfoMain2) {
        __algoProcInfo.af_auxi = *afInfoMain2;
    }

    __setN3DCaptureParams(n3dParams, n3dOutput);
    if(!__DISABLE_GPU) {
        isSuccess = __runN3DCapture();
    } else {
        // Run preview flow
        if(__RUN_N3D) {
            AutoProfileUtil profile(LOG_TAG, "N3D set proc(Capture+CPU)");
            MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SET_PROC_INFO, &__algoProcInfo, NULL);
            if (err) {
                MY_LOGE("StereoKernelFeatureCtrl(SET_PROC) fail. error code: %d.", err);
            } else {
                __logSetProcInfo("", __algoProcInfo);
            }
        }

        isSuccess = __runN3DCommon(E_LEARNING_STAGE);
        isSuccess = __runN3DCommon(E_WARPING_STAGE);
    }

    __updateSceneInfo(n3dOutput);
    n3dOutput.convOffset        = ((MFLOAT*)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[2];
    n3dOutput.warpingMatrixSize = __algoResult.out_n[STEREO_KERNEL_OUTPUT_RECT_CAP];
    if(n3dOutput.warpingMatrix) {
        const MUINT32 RESULT_SIZE = __algoResult.out_n[STEREO_KERNEL_OUTPUT_RECT_CAP] * sizeof(MFLOAT);
        if( RESULT_SIZE > 0 &&
            RESULT_SIZE <= StereoSettingProvider::getMaxWarpingMatrixBufferSizeInBytes() &&
            NULL != __algoResult.out_p[STEREO_KERNEL_OUTPUT_RECT_CAP] )
        {
            ::memcpy(n3dOutput.warpingMatrix,
                     __algoResult.out_p[STEREO_KERNEL_OUTPUT_RECT_CAP],
                     RESULT_SIZE);
        } else {
            MY_LOGE("Invalid warpping matrix size %d(Max: %d), out_p %p",
                    RESULT_SIZE,
                    StereoSettingProvider::getMaxWarpingMatrixBufferSizeInBytes(),
                    __algoResult.out_p[STEREO_KERNEL_OUTPUT_RECT_CAP]);
        }
    }

    algoResult = __algoResult;

    __dumpN3DLog(__getN3DLogPath());

    return isSuccess;
}

bool
N3D_HAL_KERNEL::WarpMain2(N3D_HAL_PARAM_WPE &n3dParams,
                          STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain1,
                          STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain2,
                          N3D_HAL_OUTPUT_WPE &n3dOutput)
{
    if(!__RUN_N3D) {
        return false;
    }

    if(!__isInit) {
        __doStereoKernelInit();
    }

    if(afInfoMain1) {
        __algoProcInfo.af_main = *afInfoMain1;
    }

    if(afInfoMain2) {
        __algoProcInfo.af_auxi = *afInfoMain2;
    }

    if( !checkImageBuffer(n3dOutput.warpMapMain2[0], "n3dOutput.warpMapMain2[0]", 0) ||
        !checkImageBuffer(n3dOutput.warpMapMain2[1], "n3dOutput.warpMapMain2[1]", 0) )
    {
        return false;
    }

    __algoProcInfo.runtime_cfg = 0;

    if(!__DISABLE_GPU) {
        __algoProcInfo.OutputGB_AuxiWarpMapX = (void*)n3dOutput.warpMapMain2[0]->getImageBufferHeap()->getHWBuffer();
        __algoProcInfo.OutputGB_AuxiWarpMapY = (void*)n3dOutput.warpMapMain2[1]->getImageBufferHeap()->getHWBuffer();
    } else {
        __algoProcInfo.addr_aw[0] = (MINT32*)n3dOutput.warpMapMain2[0]->getBufVA(0);
        __algoProcInfo.addr_aw[1] = (MINT32*)n3dOutput.warpMapMain2[1]->getBufVA(0);
        __algoProcInfo.addr_aw[2] = NULL;
    }

    //Main1 is optional, only for ActiveStereo
    if(n3dOutput.warpMapMain1[0] &&
       n3dOutput.warpMapMain1[1])
    {
        if(!__DISABLE_GPU) {
            __algoProcInfo.OutputGB_MainWarpMapX = (void*)n3dOutput.warpMapMain1[0]->getImageBufferHeap()->getHWBuffer();
            __algoProcInfo.OutputGB_MainWarpMapY = (void*)n3dOutput.warpMapMain1[1]->getImageBufferHeap()->getHWBuffer();
        } else {
            __algoProcInfo.addr_mw[0] = (MINT32*)n3dOutput.warpMapMain1[0]->getBufVA(0);
            __algoProcInfo.addr_mw[1] = (MINT32*)n3dOutput.warpMapMain1[1]->getBufVA(0);
            __algoProcInfo.addr_mw[2] = NULL;
        }
    }

    // Rotate main2 for WPE, optional
    if(n3dParams.rectifyImgMain2 != nullptr &&
       n3dOutput.rectifyImgMain2 != nullptr)
    {
        std::unique_lock<std::mutex> lk(__rectInSMutex); //wait last __getRectInS to finish if needed
        __rectInSInput = n3dParams.rectifyImgMain2;
        __rectInSOutput = n3dOutput.rectifyImgMain2;
        __wakeupGetRectInS = true;
        lk.unlock();
        __rectInSCondVar.notify_all(); //run __getRectInSThread
    }

    __updateRuntimeRRZOInfo(n3dParams.previewCropRatio, eSTEREO_SENSOR_MAIN1, __algoProcInfo.rrz_proc_main);
    __updateRuntimeRRZOInfo(n3dParams.previewCropRatio, eSTEREO_SENSOR_MAIN2, __algoProcInfo.rrz_proc_auxi);

    {
        AutoProfileUtil profile(LOG_TAG, "N3D set proc(Preview)");
        MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SET_PROC_INFO, &__algoProcInfo, NULL);
        if (err) {
            MY_LOGE("StereoKernelFeatureCtrl(SET_PROC) fail. error code: %d.", err);
        } else {
            __logSetProcInfo("", __algoProcInfo);
        }
    }

    __runN3DCommon(E_WARPING_STAGE);

    //Wait for RECT_IN_S to be ready
    std::unique_lock<std::mutex> lk(__rectInSMutex); //wait last __getRectInS to finish if needed
    __rectInSCondVarCaller.wait(lk, [this]{return __wakeupGetRectInS==false;});

    if(eSTEREO_SCENARIO_CAPTURE !=  __eScenario)
    {
        __dumpN3DLog(__getN3DLogPath());
    }
    else
    {
        __dumpN3DLog(__getN3DLogPath("_3_After_Warping"));
    }

    n3dOutput.convOffset = ((MFLOAT*)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[2];
    n3dOutput.distance = ((MFLOAT*)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[3];

    return true;
}

bool
N3D_HAL_KERNEL::runLearning(HWFEFM_DATA &hwfefmData, STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain1, STEREO_KERNEL_AF_INFO_STRUCT *afInfoMain2)
{
    if(!__RUN_N3D) {
        return false;
    }

    //Need at least one result of FEFM
    __hasFEFM = (NULL != hwfefmData.geoDataMain1[0] &&
                 NULL != hwfefmData.geoDataMain2[0] &&
                 NULL != hwfefmData.geoDataLeftToRight[0] &&
                 NULL != hwfefmData.geoDataRightToLeft[0]);

    __algoProcInfo.runtime_cfg = __hasFEFM;
    if(__hasFEFM) {
        __setFEFMData(hwfefmData);
        if(hwfefmData.dumpHint) {
            setDumpConfig(true, hwfefmData.dumpHint);
        }

        {
            AutoProfileUtil profile(LOG_TAG, "N3D set proc(learning)");
            if(afInfoMain1) {
                __algoProcInfo.af_main = *afInfoMain1;
            }

            if(afInfoMain2) {
                __algoProcInfo.af_auxi = *afInfoMain2;
            }

            MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SET_PROC_INFO, &__algoProcInfo, NULL);
            if (err) {
                MY_LOGE("StereoKernelFeatureCtrl(SET_PROC) fail. error code: %d.", err);
            } else {
                __logSetProcInfo("", __algoProcInfo);
            }
        }

        std::unique_lock<std::mutex> lk(__learningMutex); //wait last learning to finish
        if(eSTEREO_SCENARIO_CAPTURE != __eScenario) {
            __wakeupLearning = true;
            lk.unlock();
            __learningCondVar.notify_all(); //run __runN3DLearningThread
        } else {
            __runN3DLearning();
            lk.unlock();
        }
    } else {
        MY_LOGE("Invalid FEFM data: FE: %p %p FM: %p %p",
                hwfefmData.geoDataMain1[0],
                hwfefmData.geoDataMain2[0],
                hwfefmData.geoDataLeftToRight[0],
                hwfefmData.geoDataRightToLeft[0]);
    }

    return __hasFEFM;
}

bool
N3D_HAL_KERNEL::setNVRAM(const NVRAM_CAMERA_GEOMETRY_STRUCT *nvram)
{
    MY_LOGD("+");
    if(__RUN_N3D &&
       NULL != nvram)
    {
        MINT32 err = 0; // 0: no error. other value: error.
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_LOAD_NVRAM,
                                                    (void*)&nvram->StereoNvramData.StereoData, NULL);
        if (err)
        {
            MY_LOGE("StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_LOAD_NVRAM) fail. error code: %d.", err);
            return false;
        }
    }

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_KERNEL::updateNVRAM(NVRAM_CAMERA_GEOMETRY_STRUCT *nvram)
{
    if(__RUN_N3D &&
       NULL != nvram)
    {
        MINT32 err = 0; // 0: no error. other value: error.
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SAVE_NVRAM,
                                                    (void*)&nvram->StereoNvramData.StereoData, NULL);

        if(err) {
            MY_LOGE("StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SAVE_NVRAM) fail. error code: %d.", err);
            return false;
        }
    }

    return true;
}

MUINT32
N3D_HAL_KERNEL::getCalibrationOffsetInNVRAM()
{
    MUINT32 calibrationOffset = 0;
    MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_GET_EEPROM_START, NULL, &calibrationOffset);
    if(err) {
        MY_LOGE("STEREO_KERNEL_FEATURE_GET_EEPROM_START failed, err %d", err);
    }

    return calibrationOffset;
}

//Log
void
N3D_HAL_KERNEL::logN3DInitInfo()
{
    __logInitInfo(__algoInitInfo);
}

void
N3D_HAL_KERNEL::logN3DProcInfo()
{
    __logSetProcInfo("", __algoProcInfo);
}

void
N3D_HAL_KERNEL::logN3DResult(STEREO_KERNEL_RESULT_STRUCT &result)
{
    __logResult("[N3D Result]", result);
}

bool
N3D_HAL_KERNEL::__initGraphicBuffers(N3D_HAL_KERNEL_INIT_PARAM &n3dInitParam)
{
    AutoProfileUtil profile(LOG_TAG, "Init GraphicBuffers");

    MINT32 err = 0;
    //Create GPU context
    err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_CREATE_GPU_CTX, &__algoProcInfo, NULL);
    if(err) {
        MY_LOGE("Create GPU context failed, err %d", err);
        return false;
    }

    //Register GPU buffer
    if(__useWPE[1])
    {
        MY_LOGD_IF(__LOG_ENABLED, "Init outputWarpMapMain2, size %zu", n3dInitParam.outputWarpMapMain2[0].size());
        if(0 == n3dInitParam.outputWarpMapMain2[0].size() ||
           n3dInitParam.outputWarpMapMain2[0].size() != n3dInitParam.outputWarpMapMain2[1].size())
        {
            MY_LOGE("Size mismatch: %zu : %zu", n3dInitParam.outputWarpMapMain2[0].size(), n3dInitParam.outputWarpMapMain2[1].size());
            return false;
        }

        __algoProcInfo.buffer_number = n3dInitParam.outputWarpMapMain2[0].size();
        __algoProcInfo.GPU_FMT_IS_YUV = true;

        void *srcImageArrayX[__algoProcInfo.buffer_number];
        void *srcImageArrayY[__algoProcInfo.buffer_number];
        for(int i = 0; i < __algoProcInfo.buffer_number; i++) {
            srcImageArrayX[i] = n3dInitParam.outputWarpMapMain2[0][i]->getImageBufferHeap()->getHWBuffer();
            srcImageArrayY[i] = n3dInitParam.outputWarpMapMain2[1][i]->getImageBufferHeap()->getHWBuffer();
            MY_LOGD_IF(__LOG_ENABLED, "srcImageArrayX[%d]: %p, srcImageArrayY[%d]: %p",
                                        i, (void*)srcImageArrayX[i],
                                        i, (void*)srcImageArrayY[i]);
        }

        __algoProcInfo.OutputGB_AuxiWarpMapX = (void*)&srcImageArrayX;
        __algoProcInfo.OutputGB_AuxiWarpMapY = (void*)&srcImageArrayY;
        __algoProcInfo.GPU_BUF_IS_SRC = true;
        __algoProcInfo.source_color_domain = STEREO_KERNEL_EGLIMAGE_COLOR_SPACE_YUV_BT601_FULL;
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_CREATE_GPU_BUF, &__algoProcInfo, NULL);
        if(err) {
            MY_LOGE("Create src GPU buffer failed, err %d", err);
            return false;
        }
    } else {
        //Input
        __algoProcInfo.buffer_number = n3dInitParam.inputImageBuffers.size();
        __algoProcInfo.GPU_FMT_IS_YUV = true;

        void *srcImageArray[__algoProcInfo.buffer_number];
        for(int i = 0; i < __algoProcInfo.buffer_number; i++) {
            srcImageArray[i] = n3dInitParam.inputImageBuffers[i]->getImageBufferHeap()->getHWBuffer();
            MY_LOGD_IF(__LOG_ENABLED, "srcImageArray[%d]: %p", i, (void*)&srcImageArray[i]);
        }

        __algoProcInfo.InputGB = (void*)&srcImageArray;
        __algoProcInfo.GPU_BUF_IS_SRC = true;
        __algoProcInfo.source_color_domain = STEREO_KERNEL_EGLIMAGE_COLOR_SPACE_YUV_BT601_FULL;
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_CREATE_GPU_BUF, &__algoProcInfo, NULL);
        if(err) {
            MY_LOGE("Create src GPU buffer failed, err %d", err);
            return false;
        }

        //Output
        __algoProcInfo.buffer_number = n3dInitParam.outputImageBuffers.size();
        __algoProcInfo.GPU_FMT_IS_YUV = true;

        void *dstImageArray[__algoProcInfo.buffer_number];
        void *dstMaskArray[__algoProcInfo.buffer_number];
        for(int i = 0; i < __algoProcInfo.buffer_number; i++) {
            dstImageArray[i] = n3dInitParam.outputImageBuffers[i]->getImageBufferHeap()->getHWBuffer();
            dstMaskArray[i]  = n3dInitParam.outputMaskBuffers[i]->getImageBufferHeap()->getHWBuffer();
            MY_LOGD_IF(__LOG_ENABLED, "dstImageArray[%d]: %p dstMaskArray[%d]: %p)",
                        i, (void*)&dstImageArray[i],
                        i, (void*)&dstMaskArray[i]);
        }

        __algoProcInfo.OutputGB = (void*)&dstImageArray;
        __algoProcInfo.OutputGB_Mask = (void*)&dstMaskArray;
        __algoProcInfo.SPLIT_MASK = true;
        __algoProcInfo.GPU_BUF_IS_SRC = false;
        __algoProcInfo.source_color_domain = STEREO_KERNEL_EGLIMAGE_COLOR_SPACE_YUV_BT601_FULL;

        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_CREATE_GPU_BUF, &__algoProcInfo, NULL);
        if(err) {
            MY_LOGE("Create dst GPU buffer failed, err %d", err);
            return false;
        }
    }

    if(__useWPE[0])
    {
        MY_LOGD_IF(__LOG_ENABLED, "Init outputWarpMapMain1, size %zu", n3dInitParam.outputWarpMapMain1[0].size());
        if(0 == n3dInitParam.outputWarpMapMain1[0].size() ||
           n3dInitParam.outputWarpMapMain1[0].size() != n3dInitParam.outputWarpMapMain1[1].size())
        {
            MY_LOGE("Size mismatch: %zu : %zu", n3dInitParam.outputWarpMapMain1[0].size(), n3dInitParam.outputWarpMapMain1[1].size());
            return false;
        }

        __algoProcInfo.buffer_number = n3dInitParam.outputWarpMapMain1[0].size();
        __algoProcInfo.GPU_FMT_IS_YUV = true;

        void *srcImageArrayX[__algoProcInfo.buffer_number];
        void *srcImageArrayY[__algoProcInfo.buffer_number];
        for(int i = 0; i < __algoProcInfo.buffer_number; i++) {
            srcImageArrayX[i] = n3dInitParam.outputWarpMapMain1[0][i]->getImageBufferHeap()->getHWBuffer();
            srcImageArrayY[i] = n3dInitParam.outputWarpMapMain1[1][i]->getImageBufferHeap()->getHWBuffer();
            MY_LOGD_IF(__LOG_ENABLED, "srcImageArrayX[%d]: %p, srcImageArrayY[%d]: %p",
                                        i, (void*)srcImageArrayX[i],
                                        i, (void*)srcImageArrayY[i]);
        }

        __algoProcInfo.OutputGB_AuxiWarpMapX = (void*)&srcImageArrayX;
        __algoProcInfo.OutputGB_AuxiWarpMapY = (void*)&srcImageArrayY;
        __algoProcInfo.GPU_BUF_IS_SRC = true;
        __algoProcInfo.source_color_domain = STEREO_KERNEL_EGLIMAGE_COLOR_SPACE_YUV_BT601_FULL;
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_CREATE_GPU_BUF, &__algoProcInfo, NULL);
        if(err) {
            MY_LOGE("Create src GPU buffer failed, err %d", err);
            return false;
        }
    }

    return true;
}

bool
N3D_HAL_KERNEL::__initN3DKernel(STEREO_KERNEL_SET_ENV_INFO_STRUCT &n3dInitInfo, NVRAM_CAMERA_GEOMETRY_STRUCT *nvram, float *LDC)
{
    MY_LOGD("+");
    ::memcpy(&__algoInitInfo, &n3dInitInfo, sizeof(STEREO_KERNEL_SET_ENV_INFO_STRUCT));
    __DISABLE_GPU = __DISABLE_GPU || (0 == (__algoInitInfo.system_cfg & 0x1));

    __useWPE[0] = ((__algoInitInfo.system_cfg & (3<<13)) == (3<<13));
    __useWPE[1] = ((__algoInitInfo.system_cfg & (3<<15)) == (3<<15));

    __setLensInfo(LDC);
    setNVRAM(nvram);

    MRESULT err = 0;
    err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_GET_DEFAULT_TUNING, NULL, &__algoTuningInfo);
    if (err) {
        MY_LOGE("STEREO_KERNEL_FEATURE_GET_DEFAULT_TUNING fail. error code: %d.", err);
        return false;
    } else {
        __logTuningInfo("", __algoTuningInfo);
    }

    __algoInitInfo.dpe_convergence_offset = 0;
    TUNING_PAIR_LIST_T tuningParams;
    StereoTuningProvider::getN3DTuningInfo(__eScenario, tuningParams);
    for(auto &tuning : tuningParams) {
        if(tuning.first == "n3d.c_offset") {
            __algoInitInfo.dpe_convergence_offset = tuning.second;
        } else if(tuning.first == "n3d.p_tune") {
            __algoTuningInfo.p_tune = tuning.second;
        } else if(tuning.first == "n3d.l_tune") {
            __algoTuningInfo.l_tune = tuning.second;
        }
    }

    __doStereoKernelInit();

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_KERNEL::__setLensInfo(float *LDC)
{
    if(!__RUN_N3D ||
       NULL == LDC)
    {
        return true;
    }

    MRESULT err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_LOAD_LENS_INFO,
                                                        LDC, NULL);
    if(err) {
        MY_LOGE("STEREO_KERNEL_FEATURE_LOAD_LENS_INFO failed");
        return false;
    }

    return true;
}

void
N3D_HAL_KERNEL::__initWorkingBuffer(const size_t BUFFER_SIZE)
{
    if(!__RUN_N3D ||
       0 == BUFFER_SIZE)
    {
        return;
    }

    if(NULL != __workBufImage.get() &&
       __workBufImage.get()->getBufSizeInBytes(0) < BUFFER_SIZE)
    {
        StereoDpUtil::freeImageBuffer(LOG_TAG, __workBufImage);
    }

    if(StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_Y8, MSize(BUFFER_SIZE, 1), !IS_ALLOC_GB, __workBufImage)) {
        MY_LOGD_IF(__LOG_ENABLED, "Create working buffer with size %zu", BUFFER_SIZE);
        __algoWorkBufInfo.ext_mem_size       = BUFFER_SIZE;
        __algoWorkBufInfo.ext_mem_start_addr = (MUINT8 *)__workBufImage.get()->getBufVA(0);
        ::memset(__algoWorkBufInfo.ext_mem_start_addr, 0, __algoWorkBufInfo.ext_mem_size);

        MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SET_WORK_BUF_INFO,
                                                           &__algoWorkBufInfo, NULL);
        if (err) {
            MY_LOGE("STEREO_KERNEL_FEATURE_SET_WORK_BUF_INFO fail. error code: %d.", err);
        }
    } else {
        MY_LOGE("Fail to create working buffer with size %zu", BUFFER_SIZE);
    }
}

bool
N3D_HAL_KERNEL::__doStereoKernelInit()
{
    if(!__RUN_N3D ||
       __isInit)
    {
        return true;
    }

    MY_LOGD("+");
    AutoProfileUtil profile(LOG_TAG, "N3D init");

    MRESULT err = __pStereoDrv->StereoKernelInit(&__algoInitInfo, &__algoTuningInfo);
    if (err) {
        MY_LOGE("Init N3D algo failed(err %d)", err);
    } else {
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_GET_WORK_BUF_INFO, NULL,
                                                    &__algoInitInfo.working_buffer_size);
        if(err) {
            MY_LOGD("Get working buffer size failed, err: %d", err);
        } else {
            if(!__DISABLE_GPU) {
                __initGraphicBuffers(__n3dInitParam);
            }
            __initWorkingBuffer(__algoInitInfo.working_buffer_size);

            __isInit = initModel();
            if(__isInit) {
                Mutex::Autolock lock(__sLogLock);
                __logInitInfo(__algoInitInfo);
            }
        }
    }

    MY_LOGD("-");
    return __isInit;
}

void
N3D_HAL_KERNEL::__setN3DCommonParams(N3D_HAL_PARAM_COMMON &n3dParams, N3D_HAL_OUTPUT &n3dOutput)
{
    if( !checkImageBuffer(n3dOutput.rectifyImgMain2, "n3dOutput.rectifyImgMain2") ||
        !checkImageBuffer(n3dOutput.maskMain2,       "n3dOutput.maskMain2") )
    {
        return;
    }

    //Need at least one result of FEFM
    __hasFEFM = (NULL != n3dParams.hwfefmData.geoDataMain1[0] &&
                 NULL != n3dParams.hwfefmData.geoDataMain2[0] &&
                 NULL != n3dParams.hwfefmData.geoDataLeftToRight[0] &&
                 NULL != n3dParams.hwfefmData.geoDataRightToLeft[0]);

    __algoProcInfo.runtime_cfg = __hasFEFM;

    if(__hasFEFM) {
        __setFEFMData(n3dParams.hwfefmData);

        if( !checkImageBuffer(n3dParams.ccImage[0], "n3dParams.ccImage[0]") ||
            !checkImageBuffer(n3dParams.ccImage[1], "n3dParams.ccImage[1]") )
        {
            return;
        }

        // for Photometric Correction
        //buffer is squential, so getBufVA(0) can get whole image
        __algoProcInfo.addr_mp = (MUINT8*)n3dParams.ccImage[0]->getBufVA(0);
        __algoProcInfo.addr_ap = (MUINT8*)n3dParams.ccImage[1]->getBufVA(0);
    }

    __updateRuntimeRRZOInfo(n3dParams.previewCropRatio, eSTEREO_SENSOR_MAIN1, __algoProcInfo.rrz_proc_main);
    __updateRuntimeRRZOInfo(n3dParams.previewCropRatio, eSTEREO_SENSOR_MAIN2, __algoProcInfo.rrz_proc_auxi);
}

void
N3D_HAL_KERNEL::__setN3DParams(N3D_HAL_PARAM &n3dParams, N3D_HAL_OUTPUT &n3dOutput)
{
    __setN3DCommonParams(n3dParams, n3dOutput);

    if(!__DISABLE_GPU) {
        __algoProcInfo.InputGB       = (void*)n3dParams.rectifyImgMain2->getImageBufferHeap()->getHWBuffer();
        __algoProcInfo.OutputGB      = (void*)n3dOutput.rectifyImgMain2->getImageBufferHeap()->getHWBuffer();
        __algoProcInfo.OutputGB_Mask = (void*)n3dOutput.maskMain2->getImageBufferHeap()->getHWBuffer();
    } else {
        //buffer is squential, so getBufVA(0) can get whole image
        // __algoProcInfo.addr_as = (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(0);
        size_t imgLength = __algoInitInfo.iio_auxi.inp_w*__algoInitInfo.iio_auxi.inp_h;
        size_t bufferSize = imgLength*3/2;  //YV12
        if(NULL == __inputMain2CPU) {
            __inputMain2CPU = new(std::nothrow) MUINT8[bufferSize];
        }
        CAM_ULOG_ASSERT(NSCam::Utils::ULog::MOD_VSDOF_HAL, __inputMain2CPU != nullptr, "Allocate main2 buffer fail!");
        ::memset(__inputMain2CPU, 0, bufferSize);

        MUINT8 *dst = __inputMain2CPU;
        ::memcpy(dst, (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(0), imgLength);
        if(n3dParams.rectifyImgMain2->getBufVA(1) != 0) {
            dst += imgLength;
            imgLength >>= 2;
            ::memcpy(dst, (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(1), imgLength);
        }

        if(n3dParams.rectifyImgMain2->getBufVA(2) != 0) {
            dst += imgLength;
            ::memcpy(dst, (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(2), imgLength);
        }

        __algoProcInfo.addr_as = __inputMain2CPU;

        __algoProcInfo.addr_ad = (MUINT8*)n3dOutput.rectifyImgMain2->getBufVA(0);
        __algoProcInfo.addr_am = (MUINT8*)n3dOutput.maskMain2->getBufVA(0);
    }

    // EIS INFO.
    if(n3dParams.eisData.isON) {
        __algoProcInfo.eis[0] = n3dParams.eisData.eisOffset.x - (__algoInitInfo.flow_main.rrz_out_width  - n3dParams.eisData.eisImgSize.w)/2;
        __algoProcInfo.eis[1] = n3dParams.eisData.eisOffset.y - (__algoInitInfo.flow_main.rrz_out_height - n3dParams.eisData.eisImgSize.h)/2;
        __algoProcInfo.eis[2] = __algoInitInfo.flow_main.rrz_out_width;
        __algoProcInfo.eis[3] = __algoInitInfo.flow_main.rrz_out_height;
    } else {
        __algoProcInfo.eis[0] = 0;
        __algoProcInfo.eis[1] = 0;
        __algoProcInfo.eis[2] = 0;
        __algoProcInfo.eis[3] = 0;
    }

    if(__RUN_N3D) {
        AutoProfileUtil profile(LOG_TAG, "N3D set proc(Preview/VR)");
        MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SET_PROC_INFO, &__algoProcInfo, NULL);
        if (err) {
            MY_LOGE("StereoKernelFeatureCtrl(SET_PROC) fail. error code: %d.", err);
        } else {
            __logSetProcInfo("", __algoProcInfo);
        }
    }
}

void
N3D_HAL_KERNEL::__setN3DCaptureParams(N3D_HAL_PARAM_CAPTURE &n3dParams, N3D_HAL_OUTPUT_CAPTURE &n3dOutput)
{
    AutoProfileUtil profile(LOG_TAG, "Set params(Capture): Total   ");

    //Common Part
    __setN3DCommonParams(n3dParams, n3dOutput);

    //Capture Part
#if (1 == STEREO_DENOISE_SUPPORTED)
    MSize captureSize = StereoSizeProvider::getInstance()->captureImageSize();
    if(StereoSettingProvider::isDeNoise()) {
        //N3D should use tg size for capture since ratio cropping will happen after warping
        if(StereoHAL::isVertical(StereoSettingProvider::getModuleRotation())) {
            captureSize.w = __algoInitInfo.flow_main.tg_size_h;
            captureSize.h = __algoInitInfo.flow_main.tg_size_w;
        } else {
            captureSize.w = __algoInitInfo.flow_main.tg_size_w;
            captureSize.h = __algoInitInfo.flow_main.tg_size_h;
        }
    }
#endif

    if(!__DISABLE_GPU) {
        if( !checkImageBuffer(n3dParams.rectifyImgMain2, "n3dParams.rectifyImgMain2", 0) )
        {
            return;
        }
        __algoProcInfo.InputGB   = (void*)n3dParams.rectifyImgMain2->getImageBufferHeap()->getHWBuffer();
        __algoProcInfo.OutputGB  = (void*)n3dOutput.rectifyImgMain2->getImageBufferHeap()->getHWBuffer();
        __algoProcInfo.OutputGB_Mask = (void*)n3dOutput.maskMain2->getImageBufferHeap()->getHWBuffer();
    } else {
        if( !checkImageBuffer(n3dParams.rectifyImgMain2, "n3dParams.rectifyImgMain2", 3) )
        {
            return;
        }

        size_t imgLength = __algoInitInfo.iio_auxi.inp_w*__algoInitInfo.iio_auxi.inp_h;
        size_t bufferSize = imgLength*3/2;  //YV12
        if(NULL == __inputMain2CPU) {
            __inputMain2CPU = new(std::nothrow) MUINT8[bufferSize];
        }
        CAM_ULOG_ASSERT(NSCam::Utils::ULog::MOD_VSDOF_HAL, __inputMain2CPU != nullptr, "Allocate main2 buffer fail!");

        ::memset(__inputMain2CPU, 0, bufferSize);

        MUINT8 *dst = __inputMain2CPU;
        ::memcpy(dst, (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(0), imgLength);
        if(n3dParams.rectifyImgMain2->getBufVA(1) != 0) {
            dst += imgLength;
            imgLength >>= 2;
            ::memcpy(dst, (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(1), imgLength);
            dst += imgLength;
            ::memcpy(dst, (MUINT8*)n3dParams.rectifyImgMain2->getBufVA(2), imgLength);
        }

        __algoProcInfo.addr_as = __inputMain2CPU;
        __algoProcInfo.addr_ad = (MUINT8*)n3dOutput.rectifyImgMain2->getBufVA(0);
        __algoProcInfo.addr_am = (MUINT8*)n3dOutput.maskMain2->getBufVA(0);
    }

    if(__RUN_N3D) {
        if(!__DISABLE_GPU) {   //will print later
            __logSetProcInfo("", __algoProcInfo);
        }

        AutoProfileUtil profile2(LOG_TAG, "Set params(Capture): N3D Algo");
        MINT32 err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SET_PROC_INFO, &__algoProcInfo, NULL);
        if (err) {
            MY_LOGE("StereoKernelFeatureCtrl(SET_PROC) fail. error code: %d.", err);
        }
    }
}

bool
N3D_HAL_KERNEL::__runN3DCommon(int stage)
{
    if(!__RUN_N3D) {
        return true;
    }

    bool bResult = true;
    MINT32 err = 0; // 0: no error. other value: error.

    MY_LOGD_IF(__LOG_ENABLED, "StereoKernelMain, stage %d +", stage);
    {
        AutoProfileUtil profile(LOG_TAG, "Run N3D main");
        err = __pStereoDrv->StereoKernelMain(stage);
    }
    MY_LOGD_IF(__LOG_ENABLED, "StereoKernelMain -");

    if (err) {
        MY_LOGE("StereoKernelMain(%d) fail. error code: %d.", stage, err);
        bResult = MFALSE;
    } else if(0 == stage) {
        err = __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_GET_RESULT, NULL, &__algoResult);
        if (err) {
            MY_LOGE("StereoKernelFeatureCtrl(GET_RESULT) fail. error code: %d.", err);
            bResult = MFALSE;
        } else {
            __logResult("[N3D Result]", __algoResult);
        }
    }

    return bResult;
}

void
N3D_HAL_KERNEL::__createFEFMRingBuffer()
{
    MY_LOGD("+");
    size_t bufferSize = 0;
    MSize feoSize;
    MSize fmoSize;

    if (!StereoSettingProvider::isSlantCameraModule()) {
        bufferSize += (StereoSettingProvider::queryHWFEOBufferSize(MSize(__algoInitInfo.geo_img[0].img_main.act_width,
                                                                         __algoInitInfo.geo_img[0].img_main.act_height),
                                                                   StereoSettingProvider::fefmBlockSize(1),
                                                                   feoSize));
        bufferSize += StereoSettingProvider::queryHWFMOBufferSize(feoSize, fmoSize);

        bufferSize += (StereoSettingProvider::queryHWFEOBufferSize(MSize(__algoInitInfo.geo_img[1].img_main.act_width,
                                                                         __algoInitInfo.geo_img[1].img_main.act_height),
                                                                   StereoSettingProvider::fefmBlockSize(2),
                                                                   feoSize));
        bufferSize += StereoSettingProvider::queryHWFMOBufferSize(feoSize, fmoSize);
    } else {
        bufferSize += (StereoSettingProvider::queryHWFEOBufferSize(MSize(__algoInitInfo.geo_img[0].img_main.rot_width,
                                                                         __algoInitInfo.geo_img[0].img_main.rot_height),
                                                                   StereoSettingProvider::fefmBlockSize(1),
                                                                   feoSize));
        bufferSize += StereoSettingProvider::queryHWFMOBufferSize(feoSize, fmoSize);

        bufferSize += (StereoSettingProvider::queryHWFEOBufferSize(MSize(__algoInitInfo.geo_img[1].img_main.rot_width,
                                                                         __algoInitInfo.geo_img[1].img_main.rot_height),
                                                                   StereoSettingProvider::fefmBlockSize(2),
                                                                   feoSize));
        bufferSize += StereoSettingProvider::queryHWFMOBufferSize(feoSize, fmoSize);
    }

    bufferSize *= 2;

    MUINT8 *addr = NULL;
    if(StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_Y8, MSize(bufferSize*MAX_FEFM_RING_BUFFER_SIZE, 1), !IS_ALLOC_GB, __fefmRingBufferImage)) {
        MY_LOGD_IF(__LOG_ENABLED, "Create ring buffer with size %zu", bufferSize*MAX_FEFM_RING_BUFFER_SIZE);

        addr = (MUINT8 *)__fefmRingBufferImage.get()->getBufVA(0);
        for(auto &buf : __fefmRingBuffer)
        {
            buf.addr = addr;
            buf.size = bufferSize;

            addr += bufferSize;
        }
    } else {
        MY_LOGE("Fail to create ring buffer with size %zu", bufferSize*MAX_FEFM_RING_BUFFER_SIZE);
    }
    MY_LOGD("-");
}

void
N3D_HAL_KERNEL::__setFEFMData(HWFEFM_DATA &data)
{
    if(!__hasFEFM) {
        return;
    }

    //Prepare buffer
    FEFM_RING_BUFFER_T &ringBuffer = __fefmRingBuffer[__fefmRingBufferIndex];
    ringBuffer.setHWFEFMData(data);
    ++__fefmRingBufferIndex;
    if(__fefmRingBufferIndex >= MAX_FEFM_RING_BUFFER_SIZE)
    {
        __fefmRingBufferIndex = 0;
    }

    for(int i = 0; i < MAX_GEO_LEVEL; i++) {
        __algoProcInfo.addr_me[i] = (MUINT16*)ringBuffer.feMain1[i].addr;
        __algoProcInfo.addr_ae[i] = (MUINT16*)ringBuffer.feMain2[i].addr;
        __algoProcInfo.addr_lr[i] = (MUINT16*)ringBuffer.fmLR[i].addr;
        __algoProcInfo.addr_rl[i] = (MUINT16*)ringBuffer.fmRL[i].addr;
    }
}

bool
N3D_HAL_KERNEL::__runN3DCapture()
{
    MY_LOGD_IF(__LOG_ENABLED, "+");

    if(eSTEREO_SCENARIO_CAPTURE != __eScenario) {
        MY_LOGW("Wrong scenario, expect %d, fact: %d", eSTEREO_SCENARIO_CAPTURE, __eScenario);
        return false;
    }

    bool isSuccess = __runN3DCommon(E_LEARNING_STAGE);
    if(!isSuccess) {
        return false;
    }

    isSuccess = __runN3DCommon(E_WARPING_STAGE);
    if(!isSuccess) {
        return false;
    }

    if( __RUN_N3D &&
        1 == checkStereoProperty(PROPERTY_ALGO_BEBUG) )
    {
        static MUINT snLogCount = 0;
        __pStereoDrv->StereoKernelFeatureCtrl(STEREO_KERNEL_FEATURE_SAVE_LOG, &snLogCount, NULL);
    }

    MY_LOGD_IF(__LOG_ENABLED, "-");

    return isSuccess;
}

void
N3D_HAL_KERNEL::__runN3DLearning()
{
    if(eSTEREO_SCENARIO_CAPTURE !=  __eScenario)
    {
        std::string logPath = __getN3DLogPath();
        MY_LOGD("Run N3D leanring");
        __runN3DCommon(E_LEARNING_STAGE);
        __dumpN3DLog(logPath);
    }
    else
    {
        std::string logPath = __getN3DLogPath("_1_Before_Learning");
        __dumpN3DLog(logPath);
        MY_LOGD("Run N3D leanring");
        __runN3DCommon(E_LEARNING_STAGE);

        logPath = __getN3DLogPath("_2_After_Learning");
        __dumpN3DLog(logPath);
    }
}

void
N3D_HAL_KERNEL::__runN3DLearningThread()
{
    MY_LOGD("+");
    do {
        std::unique_lock<std::mutex> lk(__learningMutex);
        //Note: wait will unlock lk and then wait
        __learningCondVar.wait(lk, [this]{return __wakeupLearning;});
        __wakeupLearning = false;

        if(!__hasFEFM) {
            MY_LOGD("Leave learning thread");
            break;
        }

        __runN3DLearning();

        lk.unlock();
    } while(1);
    MY_LOGD("-");
}

void
N3D_HAL_KERNEL::__getRectInS()
{
    if(NULL == __rectInSInput ||
       NULL == __rectInSOutput)
    {
        return;
    }

    AutoProfileUtil profile(LOG_TAG, "Prepare RECT_IN_S");
    StereoDpUtil::rotateBuffer(LOG_TAG,
                               __rectInSInput,
                               __rectInSOutput,
                               StereoSettingProvider::getModuleRotation(),
                               StereoSizeProvider::getInstance()->
                                    getBufferSize(E_RECT_IN_S,
                                                  __n3dInitParam.eScenario));

     //These variables are pointer only, reset when RECT_IN_S is ready
     __rectInSInput  = NULL;
     __rectInSOutput = NULL;
}

void
N3D_HAL_KERNEL::__getRectInSThread()
{
    MY_LOGD("+");
    do {
        std::unique_lock<std::mutex> lk(__rectInSMutex);
        //Note: wait will unlock lk and then wait
        __rectInSCondVar.wait(lk, [this]{return __wakeupGetRectInS;});

        if(NULL == __rectInSInput ||
           NULL == __rectInSOutput)
        {
            MY_LOGD("Leave get RECT_IN_S thread");
            __wakeupGetRectInS = false;
            break;
        }

        __getRectInS();

        __wakeupGetRectInS = false;
        lk.unlock();
        __rectInSCondVarCaller.notify_all();
    } while(1);
    MY_LOGD("-");
}

void
N3D_HAL_KERNEL::__updateSceneInfo(N3D_HAL_OUTPUT &n3dOutput)
{
    n3dOutput.sceneInfoSize = __algoResult.out_n[STEREO_KERNEL_OUTPUT_SCENE_INFO];
    if(n3dOutput.sceneInfo) {
        const MUINT32 RESULT_SIZE = __algoResult.out_n[STEREO_KERNEL_OUTPUT_SCENE_INFO] * sizeof(MINT32);
        if( RESULT_SIZE > 0 &&
            RESULT_SIZE <= StereoSettingProvider::getMaxSceneInfoBufferSizeInBytes() &&
            NULL != __algoResult.out_p[STEREO_KERNEL_OUTPUT_SCENE_INFO] )
        {
            ::memcpy(n3dOutput.sceneInfo,
                     __algoResult.out_p[STEREO_KERNEL_OUTPUT_SCENE_INFO],
                     RESULT_SIZE);
            n3dOutput.sceneInfo[14] = __algoProcInfo.af_main.dac_i;
        } else {
            MY_LOGE("Invalid Scene Info size %d(Max: %d), out_p %p",
                    RESULT_SIZE,
                    StereoSettingProvider::getMaxSceneInfoBufferSizeInBytes(),
                    __algoResult.out_p[STEREO_KERNEL_OUTPUT_SCENE_INFO]);
        }

        n3dOutput.sceneInfo[9] = ( (__algoInitInfo.af_init_main.dac_mcr << 10) + (__algoInitInfo.af_init_main.dac_inf) );  //Short distance for denoise
    }
}

void
N3D_HAL_KERNEL::__updateRuntimeRRZOInfo(float zoomRatio,
                                        ENUM_STEREO_SENSOR sensor,
                                        STEREO_KERNEL_RRZ_INFO_STRUCT &rrzoInfo)
{
    MUINT32 junkStride;
    MSize   szRRZO;
    MRect   tgCropRect;
    StereoSizeProvider::getInstance()->getPass1Size( {  sensor,
                                                        eImgFmt_FG_BAYER10,
                                                        EPortIndex_RRZO,
                                                        __eScenario,
                                                        zoomRatio
                                                     },
                                                     //below are outputs
                                                     tgCropRect,
                                                     szRRZO,
                                                     junkStride);

    rrzoInfo.rrz_offset_x     = tgCropRect.p.x;
    rrzoInfo.rrz_offset_y     = tgCropRect.p.y;
    rrzoInfo.rrz_usage_width  = tgCropRect.s.w;
    rrzoInfo.rrz_usage_height = tgCropRect.s.h;
    rrzoInfo.rrz_out_width    = szRRZO.w;
    rrzoInfo.rrz_out_height   = szRRZO.h;
}
