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
#include <string>

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_VSDOF_HAL
#include "n3d_hal_imp.h"         // For N3D_HAL class.
#include "../inc/stereo_dp_util.h"

#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/aaa/INvBufUtil.h>
#include <af_param.h>          // For AF_ROI_SEL_FD
#include <camera_custom_stereo.h>   // For CUST_STEREO_* definitions.
#include <math.h>

#include <vsdof/hal/extradata_def.h>

#include <vsdof/hal/json_util.h>
#include <stereo_crop_util.h>
#include <mtkcam3/feature/stereo/hal/stereo_fd_proxy.h>
#include <fefm_setting_provider.h>
#include <algorithm>
#include <stereo_tuning_provider.h>
#include <mtkcam/utils/calibration/ICalibrationProvider.h>

using android::Mutex;           // For android::Mutex in stereo_hal.h.

/**************************************************************************
 *                      D E F I N E S / M A C R O S                       *
 **************************************************************************/
const bool DUMP_INPUT_NVRAM  = true;
const bool DUMP_OUTPUT_NVRAM = !DUMP_INPUT_NVRAM;

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
Mutex __instLock;
Mutex N3D_HAL_IMP::__sLock;
Mutex N3D_HAL_IMP::__sLogLock;
Mutex N3D_HAL_IMP::__sRunLock;
NVRAM_CAMERA_GEOMETRY_STRUCT* N3D_HAL_IMP::__spVoidGeoData = NULL;
bool N3D_HAL_IMP::__useExternalCalibration = false;
bool N3D_HAL_IMP::__hasCalibration = false;
N3D_HAL_KERNEL *N3D_HAL_IMP::__spPreviewKernel = NULL;
N3D_HAL_KERNEL *N3D_HAL_IMP::__spCaptureKernel = NULL;
int N3D_HAL_IMP::__nvramSensorDevIndex = 0;

Mutex __main2MaskLock;
std::unordered_map<ENUM_STEREO_SCENARIO, StereoArea>        __main2InputMaskAreaMap;
std::unordered_map<ENUM_STEREO_SCENARIO, StereoArea>        __main2OutputMaskAreaMap;
std::unordered_map<ENUM_STEREO_SCENARIO, sp<IImageBuffer>>  __main2InitMaskMap;

bool
__initMain2MaskArea(ENUM_STEREO_SCENARIO scenario)
{
    MY_LOGD("+");
    Mutex::Autolock lock(__main2MaskLock);

    auto iter = __main2InputMaskAreaMap.find(scenario);
    if(iter != __main2InputMaskAreaMap.end()) {
        MY_LOGD("Main2 mask for scenario %d exists", scenario);
        return true;
    }

    StereoImageMargin maskMargin;
    StereoTuningProvider::getN3DMaskMargin(scenario, maskMargin);

    // Output mask area
    StereoArea main2OutputMaskArea = StereoSizeProvider::getInstance()->getBufferSize(E_SV_Y, scenario);
    main2OutputMaskArea.padding.w = maskMargin.left + maskMargin.right;
    main2OutputMaskArea.padding.h = maskMargin.top + maskMargin.bottom;
    main2OutputMaskArea.startPt.x = maskMargin.left;
    main2OutputMaskArea.startPt.y = maskMargin.top;

    // Input mask area
#if HAS_HW_DPE <= 2
    StereoArea main2InputMaskArea = StereoSizeProvider::getInstance()->getBufferSize(E_MASK_IN_S, scenario);
#else
    MUINT32 junkStride;
    MSize   szMain2RRZO;
    MRect   tgCropRect;
    StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN2,
                                                       eImgFmt_Y8,
                                                       EPortIndex_RSSO_R2,
                                                       scenario,
                                                       1.0f
                                                     },
                                                     //below are outputs
                                                     tgCropRect,
                                                     szMain2RRZO,
                                                     junkStride);

    if(StereoSettingProvider::getModuleRotation() & 0x2)
    {
        std::swap(szMain2RRZO.w, szMain2RRZO.h);
    }

    StereoArea main2InputMaskArea(szMain2RRZO);
#endif

    // Main2 input mask: insect of input area & tuning margin
    int diff = maskMargin.left - main2InputMaskArea.startPt.x;
    if(diff > 0)
    {
        main2InputMaskArea.padding.w += diff;
        main2InputMaskArea.startPt.x = maskMargin.left;
    }

    diff = maskMargin.right - (main2InputMaskArea.padding.w - main2InputMaskArea.startPt.x);
    if(diff > 0)
    {
        main2InputMaskArea.padding.w += diff;
    }

    diff = maskMargin.top - main2InputMaskArea.startPt.y;
    if(diff > 0)
    {
        main2InputMaskArea.padding.h += diff;
        main2InputMaskArea.startPt.y = maskMargin.top;
    }

    diff = maskMargin.bottom - (main2InputMaskArea.padding.h - main2InputMaskArea.startPt.y);
    if(diff > 0)
    {
        main2InputMaskArea.padding.h += diff;
    }

    // Alloc main2 init mask buffer
    sp<IImageBuffer> main2InitMask;
    StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_Y8, main2InputMaskArea.size, !IS_ALLOC_GB, main2InitMask);
    MUINT8 *image = (MUINT8 *)main2InitMask->getBufVA(0);
    ::memset(image, 0xFF, main2InitMask->getBufSizeInBytes(0));
    const MSize CONTENT_SIZE = main2InputMaskArea.contentSize();

    //Top
    if(main2InputMaskArea.startPt.y > 0)
    {
        ::memset(image, 0, main2InputMaskArea.size.w * main2InputMaskArea.startPt.y);
    }

    //Bottom
    if(main2InputMaskArea.startPt.y+CONTENT_SIZE.h-1 < main2InputMaskArea.size.h) {
        ::memset(image+(main2InputMaskArea.startPt.y+CONTENT_SIZE.h)*main2InputMaskArea.size.w, 0,
                 main2InputMaskArea.size.w * (main2InputMaskArea.padding.h-main2InputMaskArea.startPt.y));
    }

    //Left & Right
    MUINT8 *leftPos  = image + (main2InputMaskArea.startPt.y+CONTENT_SIZE.h-1) * main2InputMaskArea.size.w;
    MUINT8 *rightPos = leftPos + main2InputMaskArea.startPt.x + CONTENT_SIZE.w;
    const int RIGHT_SPACE = main2InputMaskArea.padding.w - main2InputMaskArea.startPt.x;
    for(int h = CONTENT_SIZE.h-1; h >= 0; --h, leftPos -= main2InputMaskArea.size.w, rightPos -= main2InputMaskArea.size.w) {
        ::memset(leftPos, 0, main2InputMaskArea.startPt.x);
        ::memset(rightPos, 0, RIGHT_SPACE);
    }

    __main2InputMaskAreaMap.emplace(scenario, main2InputMaskArea);
    __main2OutputMaskAreaMap.emplace(scenario, main2OutputMaskArea);
    __main2InitMaskMap.emplace(scenario, main2InitMask);

    // char dumpName[256];
    // sprintf(dumpName, "/data/vendor/camera_dump/MASK_IN_S_content_%dx%d_padding_%d_%d_%d_%d_size_%dx%d.y",
    //        CONTENT_SIZE.w, CONTENT_SIZE.h,
    //        maskMargin.top, maskMargin.bottom, maskMargin.right, maskMargin.left,
    //        main2InputMaskArea.size.w, main2InputMaskArea.size.h);
    // main2InitMask->saveToFile(dumpName);

    MY_LOGD("-");
    return true;
}

std::weak_ptr<N3D_HAL> __wpPreviewN3D;
std::weak_ptr<N3D_HAL> __wpCaptureN3D;

/**************************************************************************
 *       Public Functions                                                 *
 **************************************************************************/
std::shared_ptr<N3D_HAL>
N3D_HAL::createInstance(const N3D_HAL_INIT_PARAM &n3dInitParam)
{
    Mutex::Autolock lock(__instLock);
    shared_ptr<N3D_HAL> ptr;
    if(n3dInitParam.eScenario == eSTEREO_SCENARIO_CAPTURE) {
        ptr = __wpCaptureN3D.lock();
        if(!ptr) {
            ptr.reset(new N3D_HAL_IMP());
            __wpCaptureN3D = ptr;
            ptr->init(n3dInitParam);
        }
        return ptr;
    } else {
        ptr = __wpPreviewN3D.lock();
        if(!ptr) {
            ptr.reset(new N3D_HAL_IMP());
            __wpPreviewN3D = ptr;
            ptr->init(n3dInitParam);
        }
        return ptr;
    }

    return ptr;
}

//========================================================================
//
//========================================================================
std::shared_ptr<N3D_HAL>
N3D_HAL::createInstance(const N3D_HAL_INIT_PARAM_WPE &n3dInitParam)
{
    Mutex::Autolock lock(__instLock);
    shared_ptr<N3D_HAL> ptr;
    if(n3dInitParam.eScenario == eSTEREO_SCENARIO_CAPTURE) {
        ptr = __wpCaptureN3D.lock();
        if(!ptr) {
            ptr.reset(new N3D_HAL_IMP());
            __wpCaptureN3D = ptr;
            ptr->init(n3dInitParam);
        }
        return ptr;
    } else {
        ptr = __wpPreviewN3D.lock();
        if(!ptr) {
            ptr.reset(new N3D_HAL_IMP());
            __wpPreviewN3D = ptr;
            ptr->init(n3dInitParam);
        }
        return ptr;
    }

    return ptr;
}

shared_ptr<N3D_HAL>
N3D_HAL::createInstance(ENUM_STEREO_SCENARIO scenario)
{
    Mutex::Autolock lock(__instLock);
    shared_ptr<N3D_HAL> ptr;
    if(scenario == eSTEREO_SCENARIO_CAPTURE) {
        ptr = __wpCaptureN3D.lock();
        if(!ptr) {
            ptr.reset(new N3D_HAL_IMP());
            __wpCaptureN3D = ptr;
        }

        return ptr;
    } else {
        ptr = __wpPreviewN3D.lock();
        if(!ptr) {
            ptr.reset(new N3D_HAL_IMP());
            __wpPreviewN3D = ptr;
        }

        return ptr;
    }

    return ptr;
}

N3D_HAL_IMP::N3D_HAL_IMP()
    : __fastLogger(LOG_TAG, LOG_PERPERTY)
{
    ::memset(&__algoInitInfo, 0, sizeof(__algoInitInfo));
    ::memset(&__afInfoMain1, 0, sizeof(__afInfoMain1));
    ::memset(&__afInfoMain2, 0, sizeof(__afInfoMain2));
    ::memset(&__algoResult, 0, sizeof(__algoResult));
}

void
N3D_HAL_IMP::init(const N3D_HAL_INIT_PARAM &n3dInitParam)
{
    __n3dKernel.init(N3D_HAL_KERNEL_INIT_PARAM(n3dInitParam));
    __initN3DHAL(&n3dInitParam);
}

void
N3D_HAL_IMP::init(const N3D_HAL_INIT_PARAM_WPE &n3dInitParam)
{
    MY_LOGD("+");
    __n3dKernel.init(N3D_HAL_KERNEL_INIT_PARAM(n3dInitParam));

#if (HAS_WPE!=0)
    __useWPE = true;
#endif

    __initN3DHAL(&n3dInitParam);
    MY_LOGD("-");
}

N3D_HAL_IMP::~N3D_HAL_IMP()
{
    MY_LOGD("%s +", (__eScenario == eSTEREO_SCENARIO_CAPTURE)?"Cap":"PV");
    {
        Mutex::Autolock lock(__main2MaskLock);
        auto maskBufferIter = __main2InitMaskMap.find(__eScenario);
        if(maskBufferIter != __main2InitMaskMap.end())
        {
            StereoDpUtil::freeImageBuffer(LOG_TAG, maskBufferIter->second);
            __main2InitMaskMap.erase(maskBufferIter);
        }

        auto main2InputMaskAreaIter = __main2InputMaskAreaMap.find(__eScenario);
        if(main2InputMaskAreaIter != __main2InputMaskAreaMap.end())
        {
            __main2InputMaskAreaMap.erase(main2InputMaskAreaIter);
        }

        auto main2OutputMaskAreaIter = __main2OutputMaskAreaMap.find(__eScenario);
        if(main2OutputMaskAreaIter != __main2OutputMaskAreaMap.end())
        {
            __main2OutputMaskAreaMap.erase(main2OutputMaskAreaIter);
        }
    }

    Mutex::Autolock lock(__sLock);    //To protect NVRAM access and staic instance

    if(__spCaptureKernel == &__n3dKernel) {
        __spCaptureKernel = NULL;
    } else if(__spPreviewKernel == &__n3dKernel) {
        __spPreviewKernel = NULL;
        __saveNVRAM();
    } else {
        MY_LOGD("Stereo drv does not match: this %p, preview %p, capture %p",
                &__n3dKernel, __spPreviewKernel, __spCaptureKernel);
    }

    MY_LOGD("%s -", (__eScenario == eSTEREO_SCENARIO_CAPTURE)?"Cap":"PV");
}

void
N3D_HAL_IMP::setConvergence(void *data, size_t len)
{
    if(len <= __CONV_SIZE) {
        ::memcpy(__convergenceData, data, len);
    } else {
        MY_LOGE("Convergence data is oversized: %zu > %zu", len, __CONV_SIZE);
    }
}

bool
N3D_HAL_IMP::__initN3DHAL(const N3D_HAL_INIT_PARAM_COMMON *n3dInitParam)
{
    MY_LOGD("+");
    __fastLogger.setSingleLineMode(SINGLE_LINE_LOG);
    ::memset(&__algoResult, 0, sizeof(__algoResult));
    ::memset(&__algoInitInfo, 0, sizeof(__algoInitInfo));
    __sensorSenarios[0] = StereoSettingProvider::getSensorScenarioMain1();
    __sensorSenarios[1] = StereoSettingProvider::getSensorScenarioMain2();

    switch(n3dInitParam->eScenario) {
        case eSTEREO_SCENARIO_CAPTURE:
            __algoInitInfo.scenario = STEREO_KERNEL_SCENARIO_IMAGE_CAPTURE;
            __spCaptureKernel = &__n3dKernel;
            __eScenario = eSTEREO_SCENARIO_CAPTURE;
            break;
        case eSTEREO_SCENARIO_PREVIEW:
        case eSTEREO_SCENARIO_RECORD:
        default:
            // __algoInitInfo.scenario = STEREO_KERNEL_SCENARIO_IMAGE_PREVIEW;
            __algoInitInfo.scenario = STEREO_KERNEL_SCENARIO_VIDEO_RECORD;
            __spPreviewKernel = &__n3dKernel;
            __eScenario = n3dInitParam->eScenario;
            break;
    }

    if(__main2InitMaskMap.find(__eScenario) == __main2InitMaskMap.end())
    {
        __initMain2MaskArea(__eScenario);
    }

    int32_t main1Idx, main2Idx;
    StereoSettingProvider::getStereoSensorIndex(main1Idx, main2Idx);

    //Make sure FEFM settings sync to N3D
    FEFMSettingProvider::getInstance()->init(main1Idx, main2Idx);
    if(!__initLensInfo(main1Idx, __pAFTable[0], __isAF[0])) {
        return false;
    }

    if(!__initLensInfo(main2Idx, __pAFTable[1], __isAF[1])) {
        return false;
    }

    __initAFWinTransform(n3dInitParam->eScenario);

    StereoArea areaRectInM = StereoSizeProvider::getInstance()->getBufferSize(E_RECT_IN_M, __eScenario);
#if (HAS_P1YUV > 0)
    if(StereoHAL::isVertical(StereoSettingProvider::getModuleRotation()) &&
       areaRectInM.size.w > areaRectInM.size.h)
    {
        areaRectInM.rotatedByModule();
    }
#endif
    __algoInitInfo.iio_main.inp_w = areaRectInM.size.w;
    __algoInitInfo.iio_main.inp_h = areaRectInM.size.h;

    __algoInitInfo.iio_main.src_w = areaRectInM.contentSize().w;
    __algoInitInfo.iio_main.src_h = areaRectInM.contentSize().h;

    StereoArea areaOutput;
    if(eSTEREO_SCENARIO_CAPTURE != __eScenario) {
        StereoArea auxiInputArea = StereoSizeProvider::getInstance()->getBufferSize({E_RECT_IN_S, __eScenario, true, 1.0f});
#if (HAS_P1YUV > 1)
        auxiInputArea.removePadding(IS_KEEP_SIZE);
        // RSSO_R2 may be limited by hw capability,
        // so we have to check real output size
        MUINT32 junkStride;
        MSize   szMain2RRZO;
        MRect   tgCropRect;
        StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN2,
                                                           eImgFmt_Y8,
                                                           EPortIndex_RSSO_R2,
                                                           __eScenario,
                                                           1.0f
                                                         },
                                                         //below are outputs
                                                         tgCropRect,
                                                         szMain2RRZO,
                                                         junkStride);

        if(StereoSettingProvider::getModuleRotation() & 0x2)
        {
            std::swap(szMain2RRZO.w, szMain2RRZO.h);
        }

        if(auxiInputArea.size.w != szMain2RRZO.w ||
           auxiInputArea.size.h != szMain2RRZO.h)
        {
            MY_LOGD("Update content size: %dx%d->%dx%d",
                    auxiInputArea.size.w, auxiInputArea.size.h,
                    szMain2RRZO.w, szMain2RRZO.h);
            auxiInputArea.size = szMain2RRZO;
        }
#endif
        MSize contentSize = auxiInputArea.contentSize();
        __algoInitInfo.iio_auxi.src_w = contentSize.w;
        __algoInitInfo.iio_auxi.src_h = contentSize.h;
        __algoInitInfo.iio_auxi.inp_w = auxiInputArea.size.w;
        __algoInitInfo.iio_auxi.inp_h = auxiInputArea.size.h;

        areaOutput = StereoSizeProvider::getInstance()->getBufferSize(E_MV_Y, __eScenario);
        __algoInitInfo.iio_main.out_w = areaOutput.size.w;
        __algoInitInfo.iio_main.out_h = areaOutput.size.h;
    } else {
#if (0==HAS_AIDEPTH)
        Pass2SizeInfo pass2SizeInfo;
        StereoSizeProvider::getInstance()->getPass2SizeInfo(PASS2A_P_2, __eScenario, pass2SizeInfo);
        __algoInitInfo.iio_auxi.src_w = pass2SizeInfo.areaWDMA.size.w;
        __algoInitInfo.iio_auxi.src_h = pass2SizeInfo.areaWDMA.size.h;
        __algoInitInfo.iio_auxi.inp_w = pass2SizeInfo.areaWDMA.size.w;
        __algoInitInfo.iio_auxi.inp_h = pass2SizeInfo.areaWDMA.size.h;

        areaOutput = StereoSizeProvider::getInstance()->getBufferSize(E_MV_Y_LARGE, __eScenario);
        __algoInitInfo.iio_main.out_w = areaOutput.size.w;
        __algoInitInfo.iio_main.out_h = areaOutput.size.h;
#else
        StereoArea areaInput = StereoSizeProvider::getInstance()->getBufferSize({E_RECT_IN_S, __eScenario, true, 1.0f});
        __algoInitInfo.iio_auxi.inp_w = areaInput.size.w;
        __algoInitInfo.iio_auxi.inp_h = areaInput.size.h;
        __algoInitInfo.iio_auxi.src_w = areaInput.size.w;
        __algoInitInfo.iio_auxi.src_h = areaInput.size.h;

        areaOutput = StereoSizeProvider::getInstance()->getBufferSize(E_MV_Y, __eScenario);
        __algoInitInfo.iio_main.out_w = areaOutput.size.w;
        __algoInitInfo.iio_main.out_h = areaOutput.size.h;
#endif
    }

    __algoInitInfo.iio_auxi.out_w = __algoInitInfo.iio_main.out_w;
    __algoInitInfo.iio_auxi.out_h = __algoInitInfo.iio_main.out_h;

    if(__useWPE) {
        StereoArea wpeArea = StereoSizeProvider::getInstance()->getBufferSize(E_WARP_MAP_S, __eScenario);
        __algoInitInfo.iio_auxi.wpe_w = wpeArea.size.w;
        __algoInitInfo.iio_auxi.wpe_h = wpeArea.size.h;

        wpeArea = StereoSizeProvider::getInstance()->getBufferSize(E_WARP_MAP_M, __eScenario);
        __algoInitInfo.iio_main.wpe_w = wpeArea.size.w;
        __algoInitInfo.iio_main.wpe_h = wpeArea.size.h;
    }

    // HWFE INPUT - the actual size for HWFE (after SRZ)
    __algoInitInfo.geo_info  = n3dInitParam->fefmRound;   //N3D_HAL_INIT_PARAM_COMMON.fefmRound
    __algoInitInfo.geo_info |= 3<<2;
    __initN3DGeoInfo(__algoInitInfo.geo_img);              //FEFM setting

    // COLOR CORRECTION INPUT
    __initCCImgInfo(__algoInitInfo.pho_img);       //settings of main = auxi

    // Learning
    SensorFOV main1FOV, main2FOV;
    StereoSettingProvider::getStereoCameraFOV(main1FOV, main2FOV);
    __algoInitInfo.fov_main[0]     = main1FOV.fov_horizontal;
    __algoInitInfo.fov_main[1]     = main1FOV.fov_vertical;
    __algoInitInfo.fov_auxi[0]     = main2FOV.fov_horizontal;
    __algoInitInfo.fov_auxi[1]     = main2FOV.fov_vertical;

    __algoInitInfo.baseline     = StereoSettingProvider::getStereoBaseline();
    __loadNVRAM();  //Will update __useExternalCalibration, must called before __prepareSystemConfig
    __algoInitInfo.system_cfg   = __prepareSystemConfig();
    if(StereoSettingProvider::isSlantCameraModule()) {
        __algoInitInfo.module_angle = StereoSettingProvider::getModuleRotation();
    }

    // Learning Coordinates RE-MAPPING
    __getStereoRemapInfo(__algoInitInfo.flow_main, __algoInitInfo.flow_auxi, __eScenario);

    ::memset(&__algoInitInfo.af_init_main, 0, sizeof(STEREO_KERNEL_AF_INIT_STRUCT));
    ::memset(&__algoInitInfo.af_init_auxi, 0, sizeof(STEREO_KERNEL_AF_INIT_STRUCT));

    __initAFInfo();

    StereoArea maskArea = StereoSizeProvider::getInstance()->getBufferSize(E_MASK_M_Y, __eScenario);
    __n3dKernel.initMain1Mask(maskArea);
    __n3dKernel.initN3D(__algoInitInfo, __spVoidGeoData, &(StereoSettingProvider::getLDCTable()[0]));

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_IMP::N3DHALWarpMain1(IImageBuffer *main1Input, IImageBuffer *main1Output, IImageBuffer *main1Mask)
{
    return __n3dKernel.WarpMain1(main1Input, main1Output, main1Mask);
}

bool
N3D_HAL_IMP::N3DHALRun(N3D_HAL_PARAM &n3dParams, N3D_HAL_OUTPUT &n3dOutput)
{
    //Capture and preview instance cannot run simultaneously
    Mutex::Autolock lock(__sRunLock);

    if(eSTEREO_SCENARIO_CAPTURE == __eScenario) {
        MY_LOGW("Cannot run capture here");
        return false;
    }

    AutoProfileUtil profile(LOG_TAG, "N3DHALRun(Preview/VR)");

    __magicNumber[0] = n3dParams.magicNumber[0];
    __magicNumber[1] = n3dParams.magicNumber[1];
    __requestNumber  = n3dParams.requestNumber;
    __timestamp      = n3dParams.timestamp;
    __dumpHint       = n3dParams.dumpHint;
    __dumpLevel      = n3dParams.dumpLevel;
    MY_LOGD_IF(__LOG_ENABLED, "[Magic] %d, %d [Request] %d", __magicNumber[0], __magicNumber[1], __requestNumber);

    //Scenario may change due to EIS, re-init N3D if needed
    // if(!__isInit) {
    //     ENUM_STEREO_SCENARIO scenario = (n3dParams.eisData.isON) ? eSTEREO_SCENARIO_RECORD : eSTEREO_SCENARIO_PREVIEW;
    //     MUINT sensorScenarioMain1 = StereoSettingProvider::getSensorScenarioMain1();
    //     MUINT sensorScenarioMain2 = StereoSettingProvider::getSensorScenarioMain2();
    //     if(__sensorSenarios[0] != sensorScenarioMain1 ||
    //        __sensorSenarios[1] != sensorScenarioMain2 ||
    //        __eScenario != scenario)
    //     {
    //         __eScenario = scenario;
    //         __sensorSenarios[0] = sensorScenarioMain1;
    //         __sensorSenarios[1] = sensorScenarioMain2;

    //         Pass2SizeInfo pass2SizeInfo;
    //         StereoSizeProvider::getInstance()->getPass2SizeInfo(PASS2A_2, scenario, pass2SizeInfo);
    //         __algoInitInfo.iio_main.src_w = pass2SizeInfo.areaWDMA.size.w;
    //         __algoInitInfo.iio_main.src_h = pass2SizeInfo.areaWDMA.size.h;
    //         __algoInitInfo.iio_main.inp_w = __algoInitInfo.iio_main.src_w;
    //         __algoInitInfo.iio_main.inp_h = __algoInitInfo.iio_main.src_h;

    //         StereoSizeProvider::getInstance()->getPass2SizeInfo(PASS2A_P_2, scenario, pass2SizeInfo);
    //         __algoInitInfo.iio_auxi.src_w = pass2SizeInfo.areaWDMA.size.w;
    //         __algoInitInfo.iio_auxi.src_h = pass2SizeInfo.areaWDMA.size.h;
    //         __algoInitInfo.iio_auxi.inp_w = pass2SizeInfo.areaWDMA.size.w;
    //         __algoInitInfo.iio_auxi.inp_h = pass2SizeInfo.areaWDMA.size.h;

    //         MY_LOGI("Src size change: main1: %dx%d, main2: %dx%d, re-init N3D",
    //                 __algoInitInfo.iio_main.src_w, __algoInitInfo.iio_main.src_h,
    //                 __algoInitInfo.iio_auxi.src_w, __algoInitInfo.iio_auxi.src_h);

    //         // Learning Coordinates RE-MAPPING
    //         if(__eScenario != scenario) {
    //             __getStereoRemapInfo(__algoInitInfo.flow_main, __algoInitInfo.flow_auxi, __eScenario);
    //         }
    //     }

    //     __n3dKernel.initMain1Mask(StereoSizeProvider::getInstance()->getBufferSize(E_MV_Y, __eScenario));
    //     __n3dKernel.initN3D(__algoInitInfo, __spVoidGeoData, &(StereoSettingProvider::getLDCTable()[0]));
    //     __isInit = true;
    // }

    if(IS_DUMP_ENALED) {
        __dumpNVRAM(DUMP_INPUT_NVRAM);
        __n3dKernel.setDumpConfig(true, __dumpHint, __dumpLevel);
    } else {
        __n3dKernel.setDumpConfig(false);
    }

    //Not to learn even fefm buffers are given when not AF done frame
    __getRuntimeAFParams(__pAFTable[0], __magicNumber[0], __afInfoMain1);
    __getRuntimeAFParams(__pAFTable[1], __magicNumber[1], __afInfoMain2);

    if(n3dParams.hwfefmData.geoDataMain1[0] &&
       n3dParams.hwfefmData.geoDataMain2[0] &&
       n3dParams.hwfefmData.geoDataLeftToRight[0] &&
       n3dParams.hwfefmData.geoDataRightToLeft[0] &&
       __isAF[0])
    {
        bool needToLearn = false;
        if(!__isAF[1])
        {
            needToLearn = (!__wasAFStable[0] && __afInfoMain1.dac_v);
        }
        else
        {
            needToLearn = (__afInfoMain1.dac_v && __afInfoMain2.dac_v &&
                           !(__wasAFStable[0] && __wasAFStable[1]));
        }

        if(!needToLearn) {
            n3dParams.hwfefmData.geoDataMain1[0]       = NULL;
            n3dParams.hwfefmData.geoDataMain2[0]       = NULL;
            n3dParams.hwfefmData.geoDataLeftToRight[0] = NULL;
            n3dParams.hwfefmData.geoDataRightToLeft[0] = NULL;
        }
    }

    __wasAFStable[0] = __afInfoMain1.dac_v;
    __wasAFStable[1] = __afInfoMain2.dac_v;

    __n3dKernel.WarpMain2(n3dParams, &__afInfoMain1, &__afInfoMain2, n3dOutput);

    if(NULL != n3dParams.rectifyImgMain1 &&
       NULL != n3dOutput.rectifyImgMain1 &&
       NULL != n3dOutput.maskMain1)
    {
        __n3dKernel.WarpMain1(n3dParams.rectifyImgMain1, n3dOutput.rectifyImgMain1, n3dOutput.maskMain1);
    }

    if(IS_DUMP_ENALED) {
        __n3dKernel.dumpN3DInitConfig();    //Init may happen in WarpMain2
        __n3dKernel.dumpN3DRuntimeConfig();
        __dumpNVRAM(DUMP_OUTPUT_NVRAM);
    }

    return true;
}

bool
N3D_HAL_IMP::N3DHALRun(N3D_HAL_PARAM_CAPTURE &n3dParams, N3D_HAL_OUTPUT_CAPTURE &n3dOutput)
{
    //Capture and preview instance cannot run simultaneously
    Mutex::Autolock lock(__sRunLock);

    AutoProfileUtil profile(LOG_TAG, "N3DHALRun(Capture)");
    __magicNumber[0] = n3dParams.magicNumber[0];
    __magicNumber[1] = n3dParams.magicNumber[1];
    __requestNumber  = n3dParams.requestNumber;
    __timestamp      = n3dParams.timestamp;
    MY_LOGD_IF(__LOG_ENABLED, "[Magic] %d, %d [Request] %d", __magicNumber[0], __magicNumber[1], __requestNumber);

    __syncInternalNVRAM(__spPreviewKernel, __spCaptureKernel);   //nvram: preview->capture

    if(IS_DUMP_ENALED) {
        __dumpNVRAM(DUMP_INPUT_NVRAM);
        __n3dKernel.setDumpConfig(true, __dumpHint, __dumpLevel);
    } else {
        __n3dKernel.setDumpConfig(false);
    }

    bool isSuccess = __n3dKernel.WarpMain2(n3dParams,
                                           __getRuntimeAFParams(__pAFTable[0], __magicNumber[0], __afInfoMain1),
                                           __getRuntimeAFParams(__pAFTable[1], __magicNumber[1], __afInfoMain2),
                                           n3dOutput, __algoResult);

    __n3dKernel.WarpMain1(n3dParams.rectifyImgMain1, n3dOutput.rectifyImgMain1, n3dOutput.maskMain1);

    //Denoise will learn in capture
    if( StereoSettingProvider::isDeNoise() )
    {
        __syncInternalNVRAM(__spCaptureKernel, __spPreviewKernel);   //nvram: capture->preview
    }

    if(IS_DUMP_ENALED) {
        __n3dKernel.dumpN3DInitConfig();    //Init may happen in WarpMain2
        __n3dKernel.dumpN3DRuntimeConfig();
        __dumpNVRAM(DUMP_OUTPUT_NVRAM);
    }

    __prepareStereoExtraData();

    return isSuccess;
}

char *
N3D_HAL_IMP::getStereoExtraData()
{
    //Only support capture
    if(eSTEREO_SCENARIO_CAPTURE != __eScenario) {
        return NULL;
    }

    if(0 == __stereoExtraData.size()) {
        __prepareStereoExtraData();
    }

    return (char *)__stereoExtraData.c_str();
}

bool
N3D_HAL_IMP::getWarpMapMain2(N3D_HAL_PARAM_WPE &n3dParams, N3D_HAL_OUTPUT_WPE &n3dOutput)
{
    //Capture and preview instance cannot run simultaneously
    Mutex::Autolock lock(__sRunLock);

    const char *PROFILE_NAME = (eSTEREO_SCENARIO_CAPTURE != __eScenario)
                               ? "getWarpMapMain2(Preview)"
                               : "getWarpMapMain2(Capture)";

    AutoProfileUtil profile(LOG_TAG, PROFILE_NAME);

    __magicNumber[0] = n3dParams.magicNumber[0];
    __magicNumber[1] = n3dParams.magicNumber[1];
    __requestNumber  = n3dParams.requestNumber;
    __dumpHint       = n3dParams.dumpHint;
    __dumpLevel      = n3dParams.dumpLevel;
    MY_LOGD_IF(__LOG_ENABLED, "[Magic] %d, %d [Request] %d", __magicNumber[0], __magicNumber[1], __requestNumber);

    if(IS_DUMP_ENALED) {
        __dumpNVRAM(DUMP_INPUT_NVRAM);
        __n3dKernel.setDumpConfig(true, __dumpHint, __dumpLevel);
    } else {
        __n3dKernel.setDumpConfig(false);
    }

    __n3dKernel.WarpMain2(n3dParams,
                          __getRuntimeAFParams(__pAFTable[0], __magicNumber[0], __afInfoMain1),
                          __getRuntimeAFParams(__pAFTable[1], __magicNumber[1], __afInfoMain2),
                          n3dOutput);

    if(IS_DUMP_ENALED) {
        __n3dKernel.dumpN3DInitConfig();    //Init may happen in WarpMain2
        __n3dKernel.dumpN3DRuntimeConfig();
        __dumpNVRAM(DUMP_OUTPUT_NVRAM);
    }

    return true;
}

bool
N3D_HAL_IMP::runN3DLearning(HWFEFM_DATA &hwfefmData)
{
    if(eSTEREO_SCENARIO_CAPTURE == __eScenario) {
        //Sync preview learning data from preview to capture
        __syncInternalNVRAM(__spPreviewKernel, __spCaptureKernel);
        __n3dKernel.initModel();
    }

    __magicNumber[0] = hwfefmData.magicNumber[0];
    __magicNumber[1] = hwfefmData.magicNumber[1];

    return __n3dKernel.runLearning(hwfefmData,
                                   __getRuntimeAFParams(__pAFTable[0], __magicNumber[0], __afInfoMain1),
                                   __getRuntimeAFParams(__pAFTable[1], __magicNumber[1], __afInfoMain2));
}

void
N3D_HAL::refineMask(ENUM_STEREO_SCENARIO scenario, NSCam::IImageBuffer *mask, bool initMask)
{
    if(NULL == mask) {
        return;
    }

    const char *PROFILE_TAG = (initMask) ? "refineMask(1)" : "refineMask(2)";
    AutoProfileUtil profile(LOG_TAG, PROFILE_TAG);

    MSize MASK_SIZE = mask->getImgSize();
    if(initMask)
    {
        auto iter = __main2InitMaskMap.find(scenario);
        if(iter == __main2InitMaskMap.end())
        {
            MY_LOGE("Mask for scenario %d was not initialized", scenario);
            return;
        }

        sp<IImageBuffer> main2InitMask = iter->second;
        size_t bufferSize = main2InitMask->getBufSizeInBytes(0);
        if(mask->getBufSizeInBytes(0) != bufferSize)
        {
            MY_LOGE("Buffer size mismatch: %zu(%dx%d) vs %zu(%dx%d)",
                    mask->getBufSizeInBytes(0), MASK_SIZE.w, MASK_SIZE.h,
                    bufferSize, main2InitMask->getImgSize().w, main2InitMask->getImgSize().h);
        }
        else
        {
            ::memcpy((MUINT8*)mask->getBufVA(0), (MUINT8*)main2InitMask->getBufVA(0), bufferSize);
        }
    }
    else
    {
        auto iter = __main2OutputMaskAreaMap.find(scenario);
        if(iter == __main2OutputMaskAreaMap.end())
        {
            MY_LOGE("Mask area for scenario %d was not initialized", scenario);
            return;
        }

        StereoArea main2MaskArea = iter->second;
        if(main2MaskArea.size.w != MASK_SIZE.w ||
           main2MaskArea.size.h != MASK_SIZE.h)
        {
            MY_LOGE("Mask size mismatch, MASK_SIZE %dx%d, main2Mask %dx%d",
                    MASK_SIZE.w, MASK_SIZE.h, main2MaskArea.size.w, main2MaskArea.size.h);
        }
        else
        {
            MUINT8 *image = (MUINT8 *)mask->getBufVA(0);
            MUINT8 *contentPos = NULL;
            MSize CONTENT_SIZE = main2MaskArea.contentSize();
            MUINT8 *leftPos  = image + (main2MaskArea.startPt.y+CONTENT_SIZE.h-1) * main2MaskArea.size.w;
            MUINT8 *rightPos = leftPos + main2MaskArea.startPt.x + CONTENT_SIZE.w;
            const int RIGHT_SPACE = main2MaskArea.padding.w - main2MaskArea.startPt.x;

            // Top
            if(main2MaskArea.startPt.y > 0)
            {
                ::memset(image, 0, main2MaskArea.startPt.y * main2MaskArea.size.w);
            }

            // Bottom
            if(main2MaskArea.startPt.y+CONTENT_SIZE.h-1 < main2MaskArea.size.h)
            {
                ::memset(image+(main2MaskArea.startPt.y+CONTENT_SIZE.h)*main2MaskArea.size.w, 0,
                         main2MaskArea.size.w * (main2MaskArea.padding.h-main2MaskArea.startPt.y));
            }

            // Left & Right
            for(int h = CONTENT_SIZE.h-1; h >= 0; --h, leftPos -= main2MaskArea.size.w, rightPos -= main2MaskArea.size.w) {
                ::memset(leftPos, 0, main2MaskArea.startPt.x);
                ::memset(rightPos, 0, RIGHT_SPACE);

                //truncate values
                contentPos = leftPos + main2MaskArea.startPt.x;
                for(int p = 0; p < CONTENT_SIZE.w; ++p, ++contentPos) {
                    if(*contentPos != 0xFF) {
                        *contentPos = 0;
                    }
                }
            }
        }
    }
}

/**************************************************************************
 *       Private Functions                                                *
 **************************************************************************/
bool
N3D_HAL_IMP::__getFEOInputInfo(StereoSizeProvider::P2SizeQueryParam param,
                               STEREO_KERNEL_IMG_INFO_STRUCT &imgInfo)
{
    imgInfo.depth    = 1;    //pixel depth, YUV:1, RGB: 3, RGBA: 4
    imgInfo.format   = 0;    //YUV:0, RGB: 1

    Pass2SizeInfo pass2Info;
    param.withSlant = false;
    StereoSizeProvider::getInstance()->getPass2SizeInfo(param, pass2Info);

    imgInfo.width        = pass2Info.areaFEO.size.w;
    imgInfo.height       = pass2Info.areaFEO.size.h;
    imgInfo.stride       = imgInfo.width;
    imgInfo.act_width    = pass2Info.areaFEO.size.w - pass2Info.areaFEO.padding.w;
    imgInfo.act_height   = pass2Info.areaFEO.size.h - pass2Info.areaFEO.padding.h;

    if(param.round <= PASS2A_3) {
        imgInfo.offset_x = 0;
        imgInfo.offset_y = 0;
    } else {
        imgInfo.offset_x = (imgInfo.width - imgInfo.act_width)>>1;
        imgInfo.offset_y = (imgInfo.height - imgInfo.act_height)>>1;
    }

    if (StereoSettingProvider::isSlantCameraModule()) {
        param.withSlant = true;
        StereoSizeProvider::getInstance()->getPass2SizeInfo(param, pass2Info);
        imgInfo.rot_width  = pass2Info.areaFEO.size.w;
        imgInfo.rot_height = pass2Info.areaFEO.size.h;
    }

    return true;
}

bool
N3D_HAL_IMP::__initN3DGeoInfo(STEREO_KERNEL_GEO_INFO_STRUCT geo_img[])
{
    MY_LOGD("+");
    ::memset(geo_img, 0, MAX_GEO_LEVEL * sizeof(STEREO_KERNEL_GEO_INFO_STRUCT));
    StereoSizeProvider::P2SizeQueryParam param(__eScenario, true, 1.0f);
//    if(MAX_GEO_LEVEL > 1) {
        geo_img[0].size = StereoSettingProvider::fefmBlockSize(1);    //16
        __getFEOInputInfo(param.replaceRound(PASS2A_2),   geo_img[0].img_main);
        __getFEOInputInfo(param.replaceRound(PASS2A_P_2), geo_img[0].img_auxi);
//    }

//    if(MAX_GEO_LEVEL > 2) {
        geo_img[1].size = StereoSettingProvider::fefmBlockSize(2);    //8
        __getFEOInputInfo(param.replaceRound(PASS2A_3),   geo_img[1].img_main);
        __getFEOInputInfo(param.replaceRound(PASS2A_P_3), geo_img[1].img_auxi);
//    }

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_IMP::__initCCImgInfo(STEREO_KERNEL_IMG_INFO_STRUCT &ccImgInfo)
{
    MY_LOGD("+");
    Pass2SizeInfo pass2Info;
    StereoSizeProvider::P2SizeQueryParam param(PASS2A_3, __eScenario, true, 1.0f);
    StereoSizeProvider::getInstance()->getPass2SizeInfo(param, pass2Info);
    MSize szCCImg = pass2Info.areaIMG2O;
    ccImgInfo.width         = szCCImg.w;
    ccImgInfo.height        = szCCImg.h;
    ccImgInfo.depth         = 1;            //pixel depth, YUV:1, RGB: 3, RGBA: 4
    ccImgInfo.stride        = szCCImg.w;
    ccImgInfo.format        = 4;//0;            //YUV:0, RGB: 1

    StereoSizeProvider::getInstance()->getPass2SizeInfo(param.replaceRound(PASS2A_P_3), pass2Info);
    szCCImg = pass2Info.areaIMG2O;
    ccImgInfo.act_width     = szCCImg.w;
    ccImgInfo.act_height    = szCCImg.h;
    ccImgInfo.offset_x      = 0;
    ccImgInfo.offset_y      = 0;

    MY_LOGD("-");
    return true;
}

void
N3D_HAL_IMP::__initAFWinTransform(ENUM_STEREO_SCENARIO scenario)
{
    MY_LOGD("+");
    MINT32 err = 0;
    int main1SensorIndex, main2SensorIndex;
    StereoSettingProvider::getStereoSensorIndex(main1SensorIndex, main2SensorIndex);

    int main1SensorDevIndex, main2SensorDevIndex;
    StereoSettingProvider::getStereoSensorDevIndex(main1SensorDevIndex, main2SensorDevIndex);

    IHalSensorList* sensorList = MAKE_HalSensorList();

    if(NULL == sensorList) {
        MY_LOGE("Cannot get sensor list");
    } else {
        //========= Get main1 size =========
        IHalSensor* pIHalSensor = sensorList->createSensor(LOG_TAG, main1SensorIndex);
        if(NULL == pIHalSensor) {
            MY_LOGE("Cannot get hal sensor of main1");
            err = 1;
        } else {
            SensorCropWinInfo sensorCropInfo;
            ::memset(&sensorCropInfo, 0, sizeof(SensorCropWinInfo));
            int sensorScenario = StereoSettingProvider::getSensorScenarioMain1();
            err = pIHalSensor->sendCommand(main1SensorDevIndex, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                           (MUINTPTR)&sensorScenario, (MUINTPTR)&sensorCropInfo, 0);

            if(err) {
                MY_LOGE("Cannot get sensor crop info for scenario %d", scenario);
            } else {
                //For image refocus
                int m, n;
                StereoSettingProvider::imageRatio().MToN(m, n);
                const float HEIGHT_RATIO = n/(float)m / (3.0f/4.0f);
                __afOffsetX = sensorCropInfo.x2_tg_offset;
                __afOffsetY = sensorCropInfo.y2_tg_offset + sensorCropInfo.w2_tg_size * (1.0f-HEIGHT_RATIO)/2.0f;
                __afScaleW = 2000.0f / sensorCropInfo.w2_tg_size;
                __afScaleH = 2000.0f / sensorCropInfo.h2_tg_size * HEIGHT_RATIO;

                MY_LOGD_IF(__LOG_ENABLED,
                           "AF Transform: scenario: %d, offset(%d, %d), scale(%f, %f)",
                           scenario, __afOffsetX, __afOffsetY, __afScaleW, __afScaleH);
            }

            pIHalSensor->destroyInstance(LOG_TAG);
        }
    }
    MY_LOGD("-");
}

void
N3D_HAL_IMP::__transferAFWin(const AF_WIN_COORDINATE_STRUCT &in, AF_WIN_COORDINATE_STRUCT &out)
{

    if(eSTEREO_SCENARIO_CAPTURE == __eScenario) {
        //For image refocus
        //ROI correction
        int x_correction = 0;
        if(in.af_win_start_x < __afOffsetX) {
            x_correction = __afOffsetX - in.af_win_start_x;
        }

        int y_correction = 0;
        if(in.af_win_start_y < __afOffsetY) {
            y_correction = __afOffsetY - in.af_win_start_y;
        }

        int correction = MAX(x_correction, y_correction);

        //ROI correction should not change the center point
        out.af_win_start_x = (in.af_win_start_x - __afOffsetX + correction) * __afScaleW - 1000.0f;
        out.af_win_start_y = (in.af_win_start_y - __afOffsetY + correction) * __afScaleH - 1000.0f;
        out.af_win_end_x   = (in.af_win_end_x   - __afOffsetX - correction) * __afScaleW - 1000.0f;
        out.af_win_end_y   = (in.af_win_end_y   - __afOffsetY - correction) * __afScaleH - 1000.0f;

        //Check boundary and adjust
        int offset = out.af_win_start_x - (-1000);
        if(offset < 0) {
            out.af_win_start_x = -1000;
            out.af_win_end_x += offset;
        }

        offset = out.af_win_start_y - (-1000);
        if(offset < 0) {
            out.af_win_start_y = -1000;
            out.af_win_end_y += offset;
        }

        offset = out.af_win_end_x - 1000;
        if(offset > 0) {
            out.af_win_end_x = 1000;
            out.af_win_start_x += offset;
        }

        offset = out.af_win_end_y - 1000;
        if(offset > 0) {
            out.af_win_end_y = 1000;
            out.af_win_start_y += offset;
        }
    } else {
        //For Depth AF
        out.af_win_start_x = in.af_win_start_x - __afOffsetX;
        out.af_win_start_y = in.af_win_start_y - __afOffsetY;
        out.af_win_end_x   = in.af_win_end_x   - __afOffsetX;
        out.af_win_end_y   = in.af_win_end_y   - __afOffsetY;

        if(eSTEREO_SCENARIO_RECORD == __eScenario) {
            out.af_win_start_x *= __afScaleW;
            out.af_win_start_y *= __afScaleH;
            out.af_win_end_x   *= __afScaleW;
            out.af_win_end_y   *= __afScaleH;
        }
    }

    // Verify input AF window to protect camera from NE
    if (in.af_win_start_x == 0 && in.af_win_start_y == 0 && in.af_win_end_x == 0 && in.af_win_end_y == 0 ) {
        out.af_win_start_x = 0;
        out.af_win_start_y = 0;
        out.af_win_end_x   = 0;
        out.af_win_end_y   = 0;
        MY_LOGW("Invalid AF ROI, force output window to be (%d %d %d %d)", out.af_win_start_x, out.af_win_start_y, out.af_win_end_x, out.af_win_end_y);
    }

    MY_LOGD_IF(__LOG_ENABLED,
               "AF ROI: %d %d %d %d -> %d %d %d %d",
               in.af_win_start_x, in.af_win_start_y, in.af_win_end_x, in.af_win_end_y,
               out.af_win_start_x, out.af_win_start_y, out.af_win_end_x, out.af_win_end_y);
}

STEREO_KERNEL_AF_INFO_STRUCT *
N3D_HAL_IMP::__getRuntimeAFParams(DAF_TBL_STRUCT *afTable, MUINT32 magicNuimber, STEREO_KERNEL_AF_INFO_STRUCT &result)
{
    if(NULL == afTable) {
        return NULL;
    }

    const int AF_INDEX = magicNuimber % DAF_TBL_QLEN;
    DAF_VEC_STRUCT *afVec = &afTable->daf_vec[AF_INDEX];
    result.dac_i    = (afVec->posture_dac > 0) ? afVec->posture_dac : afVec->af_dac_pos;
    result.dac_v    = afVec->is_af_stable;
    result.dac_c    = afVec->af_confidence;
    result.dac_w[0] = afVec->af_win_start_x;
    result.dac_w[1] = afVec->af_win_start_y;
    result.dac_w[2] = afVec->af_win_end_x;
    result.dac_w[3] = afVec->af_win_end_y;
    afTable->is_daf_run |= E_DAF_RUN_STEREO;  //Backup plan, should be set when init

    return &result;
}

bool
N3D_HAL_IMP::__initLensInfo(const int32_t SENSOR_INDEX, DAF_TBL_STRUCT *&pAFTable, bool &isAF)
{
    MY_LOGD("+");
    const int DEFAULT_DAC = 0;
    isAF = false;

    IHal3A *pHal3A = MAKE_Hal3A(SENSOR_INDEX, LOG_TAG);
    if(NULL == pHal3A) {
        MY_LOGE("Cannot get 3A HAL of sensor %d", SENSOR_INDEX);
        return false;
    } else {
        isAF = StereoSettingProvider::isSensorAF(SENSOR_INDEX);
        if(!isAF) {
            MY_LOGD_IF(__LOG_ENABLED, "[FF] Use default min dac of sensor %d: %d", SENSOR_INDEX, DEFAULT_DAC);
        } else {
            pHal3A->send3ACtrl(NS3Av3::E3ACtrl_GetAFDAFTable, (MUINTPTR)&pAFTable, 0);
            if(pAFTable) {
                if (0 == pAFTable->af_dac_min) {
                    MY_LOGD("[AF] Cannot get af_dac_min of sensor %d yet", SENSOR_INDEX);
                } else {
                    pAFTable->is_daf_run |= E_DAF_RUN_STEREO;
                    MY_LOGD_IF(__LOG_ENABLED, "[AF] Min dac of sensor %d: %d", SENSOR_INDEX, pAFTable->af_dac_min);
                }
            } else {
                MY_LOGE("[AF] Cannot get AF table of sensor %d", SENSOR_INDEX);
            }
        }

        pHal3A->destroyInstance(LOG_TAG);
    }

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_IMP::__getStereoRemapInfo(STEREO_KERNEL_FLOW_INFO_STRUCT &infoMain1,
                                 STEREO_KERNEL_FLOW_INFO_STRUCT &infoMain2,
                                 ENUM_STEREO_SCENARIO eScenario)
{
    MY_LOGD("+");
    int sensorIndex[2];
    StereoSettingProvider::getStereoSensorIndex(sensorIndex[0], sensorIndex[1]);

    int sensorDevIndex[2];
    StereoSettingProvider::getStereoSensorDevIndex(sensorDevIndex[0], sensorDevIndex[1]);

    STEREO_KERNEL_FLOW_INFO_STRUCT *kernelRemapInfos[2] { &infoMain1, &infoMain2 };
    ENUM_STEREO_SENSOR sensorEnums[2] { eSTEREO_SENSOR_MAIN1, eSTEREO_SENSOR_MAIN2 };

    IHalSensorList* sensorList = MAKE_HalSensorList();

    if(NULL == sensorList) {
        MY_LOGE("Cannot get sensor list");
    } else {
        SensorCropWinInfo rSensorCropInfo;
        MUINT32 junkStride;
        MSize   szMain1RRZO;
        MRect   tgCropRect;
        MINT32  err = 0;
        //========= Get main1 size =========
        for(int sensor = 0 ; sensor < 2; sensor++) {
            IHalSensor* pIHalSensor = sensorList->createSensor(LOG_TAG, sensorIndex[sensor]);
            if(NULL == pIHalSensor) {
                MY_LOGE("Cannot get hal sensor of sensor %d", sensorIndex[sensor]);
                err = 1;
            } else {
                ::memset(&rSensorCropInfo, 0, sizeof(SensorCropWinInfo));
                err = pIHalSensor->sendCommand(sensorDevIndex[sensor], SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                               (MUINTPTR)&__sensorSenarios[sensor], (MUINTPTR)&rSensorCropInfo, 0);
            }

            if(!err) {
                StereoSizeProvider::getInstance()->getPass1Size( { sensorEnums[sensor],
                                                                   eImgFmt_FG_BAYER10,
                                                                   EPortIndex_RRZO,
                                                                   eScenario,
                                                                   1.0f
                                                                 },
                                                                 //below are outputs
                                                                 tgCropRect,
                                                                 szMain1RRZO,
                                                                 junkStride);

                STEREO_KERNEL_FLOW_INFO_STRUCT *kernelInfo = kernelRemapInfos[sensor];

                kernelInfo->pixel_array_width  = rSensorCropInfo.full_w;
                kernelInfo->pixel_array_height = rSensorCropInfo.full_h ;
                kernelInfo->sensor_offset_x0   = rSensorCropInfo.x0_offset ;
                kernelInfo->sensor_offset_y0   = rSensorCropInfo.y0_offset ;
                kernelInfo->sensor_size_w0     = rSensorCropInfo.w0_size ;
                kernelInfo->sensor_size_h0     = rSensorCropInfo.h0_size ;
                kernelInfo->sensor_scale_w     = rSensorCropInfo.scale_w ;
                kernelInfo->sensor_scale_h     = rSensorCropInfo.scale_h ;
                kernelInfo->sensor_offset_x1   = rSensorCropInfo.x1_offset ;
                kernelInfo->sensor_offset_y1   = rSensorCropInfo.y1_offset ;
                kernelInfo->tg_offset_x        = rSensorCropInfo.x2_tg_offset ;
                kernelInfo->tg_offset_y        = rSensorCropInfo.y2_tg_offset ;
                kernelInfo->sensor_size_w1     = rSensorCropInfo.w1_size ;
                kernelInfo->sensor_size_h1     = rSensorCropInfo.h1_size ;
                kernelInfo->tg_size_w          = rSensorCropInfo.w2_tg_size ;
                kernelInfo->tg_size_h          = rSensorCropInfo.h2_tg_size ;
                kernelInfo->mdp_rotate         = !StereoSettingProvider::isSlantCameraModule() &&
                                                 (StereoSettingProvider::getModuleRotation() != 0) ;

                //Pre-crop main1 FOV if difference of FOV is not good enough
                if(0 == sensor)
                {
                    if(eSTEREO_SCENARIO_CAPTURE == eScenario) {
                        Pass2SizeInfo pass2SizeInfo;
                        StereoSizeProvider::getInstance()->getPass2SizeInfo(PASS2A_CROP, eScenario, pass2SizeInfo);
                        StereoArea cropArea = pass2SizeInfo.areaWDMA;
                        kernelInfo->rrz_usage_width  = cropArea.contentSize().w;
                        kernelInfo->rrz_usage_height = cropArea.contentSize().h;

                        kernelInfo->rrz_offset_x = cropArea.startPt.x;
                        kernelInfo->rrz_offset_y = cropArea.startPt.y;
                    } else {
                        kernelInfo->rrz_usage_width  = tgCropRect.s.w;
                        kernelInfo->rrz_usage_height = tgCropRect.s.h;
                        kernelInfo->rrz_offset_x     = tgCropRect.p.x;
                        kernelInfo->rrz_offset_y     = tgCropRect.p.y;
                    }
                } else {    //Only for RRZO
                    kernelInfo->rrz_usage_width  = tgCropRect.s.w;
                    kernelInfo->rrz_usage_height = tgCropRect.s.h;

                    kernelInfo->rrz_offset_x     = tgCropRect.p.x;
                    kernelInfo->rrz_offset_y     = tgCropRect.p.y;
                }

                kernelInfo->rrz_out_width  = szMain1RRZO.w;
                kernelInfo->rrz_out_height = szMain1RRZO.h;
            }

            if(pIHalSensor) {
                pIHalSensor->destroyInstance(LOG_TAG);
            }
        }
    }

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_IMP::__initAFInfo()
{
    MY_LOGD("+");
    ::memset(&__afInfoMain1, 0, sizeof(__afInfoMain1));
    ::memset(&__afInfoMain2, 0, sizeof(__afInfoMain2));

    //Get calibration data
    ENUM_STEREO_SENSOR sensor;
    STEREO_KERNEL_AF_INIT_STRUCT *afInitInfo[2] = {&__algoInitInfo.af_init_main, &__algoInitInfo.af_init_auxi};

    int32_t sensorIdx[2];
    StereoSettingProvider::getStereoSensorIndex(sensorIdx[0], sensorIdx[1]);
    int32_t sensorDevIdx[2];
    StereoSettingProvider::getStereoSensorDevIndex(sensorDevIdx[0], sensorDevIdx[1]);

    //Using ZSD by default
    CamCalDrvBase *pCamCalDrvObj = CamCalDrvBase::createInstance();
    CAM_CAL_DATA_STRUCT camCalData;
    MUINT32 err = 0;
    int dac_str = 0;

    for(int i = 0; i < 2; ++i) {
        if(__isAF[i]) {
            err = pCamCalDrvObj->GetCamCalCalData(sensorDevIdx[i], CAMERA_CAM_CAL_DATA_3A_GAIN, (void *)&camCalData);
            if(!err) {
                afInitInfo[i]->dac_inf = camCalData.Single2A.S2aAf[0];
                afInitInfo[i]->dac_mcr = camCalData.Single2A.S2aAf[1];
                dac_str = afInitInfo[i]->dac_inf - (afInitInfo[i]->dac_mcr - afInitInfo[i]->dac_inf)*0.2f;
                if(dac_str < 0) {
                    dac_str = 0;
                }
                afInitInfo[i]->dac_str = dac_str;

                sensor = (0 == i) ? eSTEREO_SENSOR_MAIN1 : eSTEREO_SENSOR_MAIN2;

                //If no value, try to get from EEPROM again
                if(!StereoSettingProvider::getCalibrationDistance(sensor, afInitInfo[i]->dist_mcr, afInitInfo[i]->dist_inf))
                {
                    afInitInfo[i]->dist_mcr = camCalData.Single2A.S2aAF_t.AF_Macro_pattern_distance;    //unit: mm
                    afInitInfo[i]->dist_inf = camCalData.Single2A.S2aAF_t.AF_infinite_pattern_distance; //unit: mm
                }
            } else {
                MY_LOGE("Read CAMERA_CAM_CAL_DATA_3A_GAIN failed");
            }
        } else {
            ::memset(afInitInfo[i], 0, sizeof(STEREO_KERNEL_AF_INIT_STRUCT));
        }
    }

    pCamCalDrvObj->destroyInstance();

    MY_LOGD("-");
    return true;
}

MUINT32
N3D_HAL_IMP::__prepareSystemConfig()
{
    MUINT32 config = 0x450003;   //0 01 00 0 10 10 00 00 00 0 00 0 0 0 0 1 1

    if( __DISABLE_GPU )
    {
        config &= ~(1);  //disable GPU
    }

    if(!__hasCalibration) {
        config |= (1<<2);
    }

    if( checkStereoProperty(PROPERTY_ENABLE_CC, 0) == 1 ) {
        config &= ~(1<<3);
    }

    config |= ((StereoSettingProvider::LDCEnabled()||__useExternalCalibration)<<4);

    if(StereoSettingProvider::getSensorRelativePosition()) {
        config |= (1<<6);
    }

    if( StereoHAL::isVertical(StereoSettingProvider::getModuleRotation()) )
    {
        config  |= (1<<7);

        if(270 == StereoSettingProvider::getModuleRotation())
        {
            config  |= (1<<27);
        }
    }

    config |= (__isAF[0] << 9);
    config |= (__isAF[1] << 10);

    if(!StereoSettingProvider::isBayerPlusMono()) {
        // config |= (1 << 12);    //auxi Y only
    }

    if(__useWPE) {
        config |= 3<<15;    //main2
        // config |= 3<<13;    //main1
    } else {
        config |= 2<<15;    //main2
    }

    config |= __useExternalCalibration<<31;
    config |= (!__useExternalCalibration)<<30;  //For mtk internal phone

    // Check tuning for config
    config |= 1 << 28;  // Turn on LDC golden diff
    config |= 1 << 29;  // Turn on convergence by default

    auto modifyConfig = [&](int bit, int val) {
        if (val == 0) {
            config &= ~(1 << bit);
        } else {
            config |= 1 << bit;
        }
    };

    TUNING_PAIR_LIST_T tuningParams;
    StereoTuningProvider::getN3DTuningInfo(__eScenario, tuningParams);
    for(auto &tuning : tuningParams) {
        int bit = -1;
        if (tuning.first == "n3d.conv_en") {
            bit = 29;
        } else if (tuning.first == "n3d.golden_curve_diff") {
            // auto LDC = StereoSettingProvider::getLDCTable();
            // if (LDC.size() > 1) {
                bit = 28;
            // }
        }

        if (bit >= 0) {
            modifyConfig(bit, tuning.second);
            FAST_LOGD("Overwrite bit %d to %d(%s)", bit, tuning.second,
                      tuning.first.c_str());
        }
    }

    // Check bit overwrite
    const size_t MAX_BITS = sizeof(config)*8;
    std::string BIT_PREFIX = "n3d.system_cfg.bit";
    for (auto &tuning : tuningParams) {
        auto pos = tuning.first.find(BIT_PREFIX);
        if (pos != std::string::npos) {
            auto name = tuning.first;
            name.erase(0, BIT_PREFIX.length());
            int bit = stoi(name);
            if (bit >= 0 && bit < MAX_BITS) {
                modifyConfig(bit, tuning.second);
                FAST_LOGD("Overwrite bit %d to %d", bit, tuning.second);
            }
        }
    }
    FAST_LOG_PRINT;
    config = checkStereoProperty(PROPERTY_SYSTEM_CFG, config);

    return config;
}

bool
N3D_HAL_IMP::__saveNVRAM()
{
    if(NULL == __spVoidGeoData) {
        return false;
    }

    AutoProfileUtil profile(LOG_TAG, "Save NARAM");

    MBOOL bResult = MTRUE;
    if(__RUN_N3D) {
        __n3dKernel.updateNVRAM(__spVoidGeoData);
        ::memcpy(__convAddr, __convergenceData, __CONV_SIZE);

        auto pNvBufUtil = MAKE_NvBufUtil();
        MINT32 err = (!pNvBufUtil) ? -1 : pNvBufUtil->write(CAMERA_NVRAM_DATA_GEOMETRY, __nvramSensorDevIndex);
        if(err) {
            MY_LOGE("Write to NVRAM fail. pNvBufUtil:%p", pNvBufUtil);
            bResult = MFALSE;
        }
    }

    return bResult;
}

bool
N3D_HAL_IMP::__loadNVRAM()
{
    MY_LOGD("+");
    Mutex::Autolock lock(__sLock);

    if(__eScenario == eSTEREO_SCENARIO_CAPTURE)
    {
        MY_LOGW("Ignore to load nvram in capture instance");
        return false;
    }

    if(__spPreviewKernel != &__n3dKernel)
    {
        MY_LOGW("Ignore to load nvram due to instance inconsistency: this %p, preview %p", &__n3dKernel, __spPreviewKernel);
        return false;
    }

    AutoProfileUtil profile(LOG_TAG, "Load NVRAM");

    MINT32 err = 0;
    int32_t main2DevIdx;
    StereoSettingProvider::getStereoSensorDevIndex(__nvramSensorDevIndex, main2DevIdx);
    auto pNvBufUtil = MAKE_NvBufUtil();
    err = (!pNvBufUtil) ? -1 : pNvBufUtil->getBufAndRead(CAMERA_NVRAM_DATA_GEOMETRY, __nvramSensorDevIndex, (void*&)__spVoidGeoData);
    if(err ||
       NULL == __spVoidGeoData)
    {
        MY_LOGE("Read NVRAM fail, data: %p pNvBufUtil:%p", __spVoidGeoData, pNvBufUtil);
        return false;
    }

    MY_LOGD_IF(__LOG_ENABLED, "NVRAM %p", __spVoidGeoData);

    //Reset NVRAM if needed
    if(1 == checkStereoProperty("vendor.STEREO.reset_nvram")) {
        MY_LOGD("Reset NVRAM");
        ::memset(__spVoidGeoData, 0, sizeof(NVRAM_CAMERA_GEOMETRY_STRUCT));
    }

    //Save calibration data into nvram
    MUINT32 calibrationOffset = __n3dKernel.getCalibrationOffsetInNVRAM();
    void *calibrationAddr = &(__spVoidGeoData->StereoNvramData.StereoData[calibrationOffset]);
    ::memset(calibrationAddr, 0, CAM_CAL_Stereo_Data_SIZE);

    //Load convergence
    __convAddr = reinterpret_cast<MUINT8 *>(&(__spVoidGeoData->StereoNvramData.StereoData[0]))
                 +sizeof(__spVoidGeoData->StereoNvramData.StereoData) - __CONV_SIZE;

    // Note: We no longer load calibraion from N3D_HAL_IMP::__loadExternalCalibrationData
    //       Please integrate calibration loader to the following location
    //       vendor/mediatek/proprietary/hardware/mtkcam/utils/calibration/CalibrationConvertor/CalibrationConvertor.cpp
    //       CalibrationConvertor::convertToMTKFormat(CalibrationResultInMTKFormat &result)
    CalibrationResultInMTKFormat cal;
    auto openId = StereoSettingProvider::getLogicalDeviceID();
    __hasCalibration = ICalibrationProvider::getInstance(openId)->getCalibration(E_CALIBRATION_MTK_FORMAT, &cal);
    if(__hasCalibration) {
        ::memcpy(calibrationAddr, cal.calibration, sizeof(cal.calibration));

        __useExternalCalibration = (cal.source != E_MTK_CALIBRATION_SOURCE_EXTERNAL &&
                                    cal.source != E_MTK_CALIBRATION_SOURCE_SETTING);
    }
    else
    {
        MY_LOGI("No stereo calibration data");
    }

    MY_LOGD("-");
    return true;
}

bool
N3D_HAL_IMP::__syncInternalNVRAM(N3D_HAL_KERNEL *pSrcStereoKernel, N3D_HAL_KERNEL *pDstStereoKernel)
{
    Mutex::Autolock lock(__sLock);

    if(!__RUN_N3D) {
        return true;
    }

    MINT32 err = 0; // 0: no error. other value: error.
    //Step 1: get nvram data from pSrcStereoKernel
    if(pSrcStereoKernel) {
        pSrcStereoKernel->updateNVRAM(__spVoidGeoData);
    } else {
        MY_LOGW("No source stereo drv");
        err = 1;
    }

    //Step 2: save nvram data to pDstStereoKernel
    if(!err) {
        if(pDstStereoKernel) {
            err = !pDstStereoKernel->setNVRAM(__spVoidGeoData);
        } else {
            MY_LOGW("No dst stereo drv");
        }
    }

    return true;
}

const char *
N3D_HAL_IMP::__prepareStereoExtraData()
{
    AutoProfileUtil profile(LOG_TAG, "Thread: Main2 mask-2: Encode extra data");

#if (1 == STEREO_DENOISE_SUPPORTED)
    if( StereoSettingProvider::isDeNoise() &&
        DENOISE_MODE == E_DENOISE_MODE_NORMAL )
    {
        __stereoExtraData = "{}";
        return __stereoExtraData.c_str();
    }
#endif

    json extraDataJson;

//    "JPS_size": {
//        "width": 4352,
//        "height": 1152
//    },
    extraDataJson[EXTRA_DATA_JPS_SIZE][EXTRA_DATA_WIDTH]  = 0;
    extraDataJson[EXTRA_DATA_JPS_SIZE][EXTRA_DATA_HEIGHT] = 0;

//    "output_image_size" : {
//        "width": 2176,
//        "height": 1152
//    },
    extraDataJson[EXTRA_DATA_OUTPUT_IMAGE_SIZE][EXTRA_DATA_WIDTH]  = 0;
    extraDataJson[EXTRA_DATA_OUTPUT_IMAGE_SIZE][EXTRA_DATA_HEIGHT] = 0;

//    "main_cam_align_shift" : {
//        "x": 30,
//        "y": 10
//    },
    extraDataJson[EXTRA_DATA_MAIN_CAM_ALIGN_SHIFT][EXTRA_DATA_X] = ((MFLOAT *)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[0];
    extraDataJson[EXTRA_DATA_MAIN_CAM_ALIGN_SHIFT][EXTRA_DATA_Y] = ((MFLOAT *)__algoResult.out_p[STEREO_KERNEL_OUTPUT_DEPTH])[1];

//    "capture_orientation": {
//        "orientations_values": "0: none, 1: flip_horizontal, 2: flip_vertical, 4: 90, 3: 180, 7: 270",
//        "orientation": 0
//    },
    int cap_ori = 0;
    switch(__captureOrientation){
        case 90:
            cap_ori = 4;
            break;
        case 180:
            cap_ori = 3;
            break;
        case 270:
            cap_ori = 7;
            break;
        case 0:
        default:
            cap_ori = 0;
            break;
    }
    extraDataJson[EXTRA_DATA_CAPTURE_ORIENTATION][EXTRA_DATA_ORIENTATIONS_VALUES] =
        "0: none, 1: flip_horizontal, 2: flip_vertical, 4: 90, 3: 180, 7: 270";
    extraDataJson[EXTRA_DATA_CAPTURE_ORIENTATION][EXTRA_DATA_ORIENTATION] = cap_ori;

//    "depthmap_orientation": {
//        "depthmap_orientation_values": "0: none, 90: 90 degrees CW, 180: 180 degrees CW, 270: 270 degrees CW",
//        "orientation": 0
//    },
    extraDataJson[EXTRA_DATA_DEPTHMAP_ORIENTATION][EXTRA_DATA_DEPTHMAP_ORIENTATION_VALUES] =
        "0: none, 90: 90 degrees CW, 180: 180 degrees CW, 270: 270 degrees CW";
    extraDataJson[EXTRA_DATA_DEPTHMAP_ORIENTATION][EXTRA_DATA_ORIENTATION] = 0;

//    "depth_buffer_size": {
//        "width": 480,
//        "height": 270
//    },
    MSize szDepthMap = StereoSizeProvider::getInstance()->getBufferSize(E_DEPTH_MAP, __eScenario);
    if( 90 == __captureOrientation ||
       270 == __captureOrientation)
    {
        szDepthMap = MSize(szDepthMap.h, szDepthMap.w);
    }
    extraDataJson[EXTRA_DATA_DEPTH_BUFFER_SIZE][EXTRA_DATA_WIDTH]  = szDepthMap.w;
    extraDataJson[EXTRA_DATA_DEPTH_BUFFER_SIZE][EXTRA_DATA_HEIGHT] = szDepthMap.h;

//    "input_image_size": {
//        "width": 1920,
//        "height": 1088
//    },
    MSize viewSize = szDepthMap * 4;
    extraDataJson[EXTRA_DATA_INPUT_IMAGE_SIZE][EXTRA_DATA_WIDTH]  = viewSize.w;
    extraDataJson[EXTRA_DATA_INPUT_IMAGE_SIZE][EXTRA_DATA_HEIGHT] = viewSize.h;

//    "sensor_relative_position": {
//        "relative_position_values": "0: main-minor, 1: minor-main",
//        "relative_position": 0
//    },
    extraDataJson[EXTRA_DATA_SENSOR_RELATIVE_POSITION][EXTRA_DATA_RELATIVE_POSITION_VALUES] =
        "0: main-minor, 1: minor-main";
    extraDataJson[EXTRA_DATA_SENSOR_RELATIVE_POSITION][EXTRA_DATA_RELATIVE_POSITION] = (int)StereoSettingProvider::getSensorRelativePosition();

//    "focus_roi": {
//        "top": 0,
//        "left": 10,
//        "bottom": 10,
//        "right": 30
//    },
    const int AF_INDEX = __magicNumber[0] % DAF_TBL_QLEN;
    DAF_VEC_STRUCT *afVec = &__pAFTable[0]->daf_vec[AF_INDEX];
    AF_WIN_COORDINATE_STRUCT apAFWin(0, 0, 0, 0);
    if(__isAF[0] &&
       NULL != __pAFTable[0])
    {
        AF_WIN_COORDINATE_STRUCT afWin(afVec->af_win_start_x, afVec->af_win_start_y,
                                       afVec->af_win_end_x, afVec->af_win_end_y);
        __transferAFWin(afWin, apAFWin);
    } else {   //FF
        if(StereoFDProxy::isFocusedOnFace()) {
            MtkCameraFace face = StereoFDProxy::getFocusedFace();
            apAFWin.af_win_start_x = face.rect[2];
            apAFWin.af_win_start_y = face.rect[1];
            apAFWin.af_win_end_x   = face.rect[0];
            apAFWin.af_win_end_y   = face.rect[3];
        } else {
            apAFWin.af_win_start_x = __touchPt.x;
            apAFWin.af_win_start_y = __touchPt.y;
            apAFWin.af_win_end_x   = __touchPt.x;
            apAFWin.af_win_end_y   = __touchPt.y;
        }
    }

    if(0 != __captureOrientation) {
        MPoint ptTopLeft = MPoint(apAFWin.af_win_start_x, apAFWin.af_win_start_y);
        MPoint ptBottomRight = MPoint(apAFWin.af_win_end_x, apAFWin.af_win_end_y);

        MY_LOGD_IF(__LOG_ENABLED, "AF ROI Before Rotate (%d, %d) (%d, %d)",
                   ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y);

        const MSize ROI_DOMAIN(2000, 2000);     //-1000~1000
        const MPoint ORIGIN_OFFSET(1000, 1000);
        MRect roi = StereoHAL::rotateRect( MRect( ptTopLeft+ORIGIN_OFFSET, ptBottomRight+ORIGIN_OFFSET ),
                                           ROI_DOMAIN,
                                           __captureOrientation );
        ptTopLeft = roi.leftTop() - ORIGIN_OFFSET;
        ptBottomRight = roi.rightBottom() - ORIGIN_OFFSET;

        MY_LOGD_IF(__LOG_ENABLED, "AF ROI After Rotate (%d, %d) (%d, %d), rotate %d",
                   ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y,
                   __captureOrientation);

        apAFWin.af_win_start_x = ptTopLeft.x;
        apAFWin.af_win_start_y = ptTopLeft.y;
        apAFWin.af_win_end_x   = ptBottomRight.x;
        apAFWin.af_win_end_y   = ptBottomRight.y;
    }

    extraDataJson[EXTRA_DATA_FOCUS_ROI][EXTRA_DATA_TOP]    = apAFWin.af_win_start_y;
    extraDataJson[EXTRA_DATA_FOCUS_ROI][EXTRA_DATA_LEFT]   = apAFWin.af_win_start_x;
    extraDataJson[EXTRA_DATA_FOCUS_ROI][EXTRA_DATA_BOTTOM] = apAFWin.af_win_end_y;
    extraDataJson[EXTRA_DATA_FOCUS_ROI][EXTRA_DATA_RIGHT]  = apAFWin.af_win_end_x;

//    "focus_info": {
//        "is_face": 0,
//        "face_ratio": 0.07,
//        "dac_cur": 200,
//        "dac_min": 100,
//        "dac_max": 300,
//        "focus_type": 3
//    }
    if(__isAF[0] &&
       NULL != __pAFTable[0])
    {
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_IS_FACE]    = (int)(AF_ROI_SEL_FD == afVec->af_roi_sel);
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_FACE_RATIO] = (apAFWin.af_win_end_x-apAFWin.af_win_start_x+1)/2000.0f;
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_DAC_CUR]    = (afVec->posture_dac > 0) ? afVec->posture_dac : afVec->af_dac_pos;
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_DAC_MIN]    = __pAFTable[0]->af_dac_min;
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_DAC_MAX]    = __pAFTable[0]->af_dac_max;
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_FOCUS_TYPE] = afVec->af_roi_sel;
    } else {    //FF
        if(StereoFDProxy::isFocusedOnFace()) {
            MRect rect = StereoFDProxy::getFocusedFaceRect();
            extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_FACE_RATIO] = rect.s.w/2000.0f;
            extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_IS_FACE]    = 1;
            extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_FOCUS_TYPE] = AF_ROI_SEL_FD;
        } else {
            extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_FOCUS_TYPE] = AF_ROI_SEL_AP;
        }
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_DAC_CUR] = 0;
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_DAC_MIN] = 0;
        extraDataJson[EXTRA_DATA_FOCUS_INFO][EXTRA_DATA_DAC_MAX] = 0;
    }

//    "GFocus": {
//        "BlurAtInfinity": 0.014506519,
//        "FocalDistance": 25.612852,
//        "FocalPointX": 0.5,
//        "FocalPointY": 0.5
//    },
    extraDataJson[EXTRA_DATA_GFOCUS][EXTRA_DATA_BLURATINFINITY] = 0.014506519;
    extraDataJson[EXTRA_DATA_GFOCUS][EXTRA_DATA_FOCALDISTANCE]  = 25.612852;
    extraDataJson[EXTRA_DATA_GFOCUS][EXTRA_DATA_FOCALPOINTX]    = 0.5;
    extraDataJson[EXTRA_DATA_GFOCUS][EXTRA_DATA_FOCALPOINTY]    = 0.5;

//    "GImage" : {
//        "Mime": "image/jpeg"
//    },
    extraDataJson[EXTRA_DATA_GIMAGE][EXTRA_DATA_MIME] = "image/jpeg";

//    "GDepth": {
//        "Format": "RangeInverse",
//        "Near": 15.12828254699707,
//        "Far": 97.0217514038086,
//        "Mime": "image/png"
//    },
    extraDataJson[EXTRA_DATA_GDEPTH][EXTRA_DATA_FORMAT] = "RangeInverse";
    extraDataJson[EXTRA_DATA_GDEPTH][EXTRA_DATA_NEAR]   = 15.12828254699707;
    extraDataJson[EXTRA_DATA_GDEPTH][EXTRA_DATA_FAR]    = 97.0217514038086;
    extraDataJson[EXTRA_DATA_GDEPTH][EXTRA_DATA_MIME]   = "image/png";

    __stereoExtraData = extraDataJson.dump();
    __dumpN3DExtraData();

    return __stereoExtraData.c_str();
}
