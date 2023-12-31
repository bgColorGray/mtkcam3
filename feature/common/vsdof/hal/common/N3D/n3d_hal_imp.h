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
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifndef N3D_HAL_IMP_H_
#define N3D_HAL_IMP_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "N3D_HAL"

#include <bitset>
#include <thread>
#include <n3d_hal.h>

#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <vector>
#include <cutils/atomic.h>
#include <MTKStereoKernel.h>
#include <mtkcam/aaa/IHal3A.h>
#include <camera_custom_nvram.h>
#include <mtkcam3/feature/stereo/hal/FastLogger.h>
#include <mtkcam/utils/std/Log.h>
#include <vsdof/hal/ProfileUtil.h>
#include <mtkcam/drv/mem/cam_cal_drv.h> //For CAM_CAL_DATA_STRUCT

#include <sys/stat.h>
#include <unistd.h>

#include "n3d_hal_kernel.h"
#include <mtkcam/utils/LogicalCam/LogicalCamJSONUtil.h>

using namespace android;
using namespace NS3Av3;
using namespace StereoHAL;
using namespace NSCam;
using namespace std;

#define PROPERTY_ENABLE_CC      STEREO_PROPERTY_PREFIX"enable_cc"
#define PROPERTY_DUMP_NVRAM     STEREO_PROPERTY_PREFIX"dump_nvram"
#define PROPERTY_DUMP_OUTPUT    STEREO_PROPERTY_PREFIX"dump_n3d"
#define PROPERTY_DUMP_CAPTURE   STEREO_PROPERTY_PREFIX"dump_n3d_capture"
#define PROPERTY_DUMP_JSON      STEREO_PROPERTY_PREFIX"dump_json"
#define PROPERTY_DUMP_CAL       STEREO_PROPERTY_PREFIX"dump_cal"
#define PROPERTY_SYSTEM_CFG     STEREO_PROPERTY_PREFIX"system_cfg"

#define EXTERN_EEPROM_PATH      "/sdcard/EEPROM_STEREO"
#define EXTERN_LDC_PATH         "/sdcard/LensINFO"

//NOTICE: property has 31 characters limitation
//debug.STEREO.log.hal.n3d [0: disable] [1: enable]
#define LOG_PERPERTY  PROPERTY_ENABLE_LOG".hal.n3d"

class N3D_HAL_IMP : public N3D_HAL
{
public:
    N3D_HAL_IMP();

    virtual void init(const N3D_HAL_INIT_PARAM &n3dInitParam);
    virtual void init(const N3D_HAL_INIT_PARAM_WPE &n3dInitParam);
    virtual void setConvergence(void *data, size_t len);

    virtual ~N3D_HAL_IMP();

    virtual bool N3DHALWarpMain1(IImageBuffer *main1Input, IImageBuffer *main1Output, IImageBuffer *main1Mask);

    virtual bool N3DHALRun(N3D_HAL_PARAM &n3dParams, N3D_HAL_OUTPUT &n3dOutput);

    virtual bool N3DHALRun(N3D_HAL_PARAM_CAPTURE &n3dParams, N3D_HAL_OUTPUT_CAPTURE &n3dOutput);

    virtual char *getStereoExtraData();

    virtual bool getWarpMapMain2(N3D_HAL_PARAM_WPE &n3dParams, N3D_HAL_OUTPUT_WPE &n3dOutput);

    virtual bool runN3DLearning(HWFEFM_DATA &hwfefmData);

private:
    bool __initN3DHAL(const N3D_HAL_INIT_PARAM_COMMON *n3dInitParam);
    void __initAFWinTransform(ENUM_STEREO_SCENARIO scenario);
    bool __saveNVRAM();
    bool __loadNVRAM();
    bool __syncInternalNVRAM(N3D_HAL_KERNEL *pSrcStereoDrv, N3D_HAL_KERNEL *pDstStereoDrv);
    void __updateInitInfoInRuntime(N3D_HAL_PARAM &n3dParams);
    bool __loadExternalCalibrationData(float *output);

    //For capture
    const char *__prepareStereoExtraData();

    bool __getFEOInputInfo(StereoSizeProvider::P2SizeQueryParam param,
                           STEREO_KERNEL_IMG_INFO_STRUCT &imgInfo);
    bool __initN3DGeoInfo(STEREO_KERNEL_GEO_INFO_STRUCT geo_img[]);
    bool __initCCImgInfo(STEREO_KERNEL_IMG_INFO_STRUCT &ccImgInfo);
    bool __initAFInfo();
    MUINT32 __prepareSystemConfig();

    bool __initLensInfo(const int32_t SENSOR_INDEX, DAF_TBL_STRUCT *&pAFTable, bool &isAF);
    bool __getStereoRemapInfo(STEREO_KERNEL_FLOW_INFO_STRUCT &infoMain1,
                             STEREO_KERNEL_FLOW_INFO_STRUCT &infoMain2,
                             ENUM_STEREO_SCENARIO eScenario);
    void __transferAFWin(const AF_WIN_COORDINATE_STRUCT &in, AF_WIN_COORDINATE_STRUCT &out);
    STEREO_KERNEL_AF_INFO_STRUCT *__getRuntimeAFParams(DAF_TBL_STRUCT *afTable, MUINT32 magicNuimber, STEREO_KERNEL_AF_INFO_STRUCT &result);

    //n3d_hal_imp.log.cpp
    void __logImgInfo(const char *prefix, const STEREO_KERNEL_IMG_INFO_STRUCT &imgInfo);
    void __logRemapInfo(const char *prefix, const STEREO_KERNEL_FLOW_INFO_STRUCT &remapInfo);
    void __logAFInitInfo(const char *prefix, const STEREO_KERNEL_AF_INIT_STRUCT &afInitInfo);
    void __logTuningInfo(const char *prefix, const STEREO_KERNEL_TUNING_PARA_STRUCT &tuningInfo);
    void __logSetProcInfo(const char *prefix, const STEREO_KERNEL_SET_PROC_INFO_STRUCT &setprocInfo);
    void __logResult(const char *prefix, const STEREO_KERNEL_RESULT_STRUCT &n3dResult);

    //n3d_hal_imp.dump.cpp
    void __dumpNVRAM(bool isInput);
    void __dumpLDC();
    void __dumpN3DExtraData();

private:
    static Mutex            __sLock; //static member is mutable, so we no need to add mutable
    static Mutex            __sLogLock;
    static Mutex            __sRunLock;

    const bool              __LOG_ENABLED  = StereoSettingProvider::isLogEnabled(LOG_PERPERTY);
    const bool              __DISABLE_GPU  = checkStereoProperty(PROPERTY_DISABLE_GPU);

    const bool              __RUN_N3D      = !checkStereoProperty(PROPERTY_SKIP_N3D);
    const bool              __DUMP_CAPTURE = checkStereoProperty(PROPERTY_DUMP_CAPTURE);
    const bool              __DUMP_PREVIEW = checkStereoProperty(PROPERTY_DUMP_OUTPUT);
    const bool              __DUMP_NVRAM   = checkStereoProperty(PROPERTY_DUMP_NVRAM);

    ENUM_STEREO_SCENARIO     __eScenario             = eSTEREO_SCENARIO_UNKNOWN;

    DAF_TBL_STRUCT          *__pAFTable[2]           = {NULL, NULL};
    bool                     __isAF[2]               = {false, false};
    MUINT32                  __magicNumber[2]        = {0, 0};
    MINT32                   __requestNumber         = 0;
    time_t                   __timestamp             = 0;
    TuningUtils::FILE_DUMP_NAMING_HINT   *__dumpHint = NULL;
    int                     __dumpLevel = 1;
    std::string             __ldcString;

    N3D_HAL_KERNEL          __n3dKernel;
    STEREO_KERNEL_SET_ENV_INFO_STRUCT __algoInitInfo;
    STEREO_KERNEL_AF_INFO_STRUCT      __afInfoMain1;
    STEREO_KERNEL_AF_INFO_STRUCT      __afInfoMain2;
    STEREO_KERNEL_RESULT_STRUCT       __algoResult;
    bool                    __isInit = false;
    bool                    __useWPE = false;

    std::string              __stereoExtraData;
    MUINT32                  __captureOrientation = 0;

    NVRAM_CAMERA_GEOMETRY_STRUCT* __pVoidGeoData = NULL;
    static NVRAM_CAMERA_GEOMETRY_STRUCT* __spVoidGeoData; //Shared between Preview/VR <-> Capture
    static bool             __useExternalCalibration;
    static bool             __hasCalibration;
    static N3D_HAL_KERNEL  *__spPreviewKernel;
    static N3D_HAL_KERNEL  *__spCaptureKernel;
    static int              __nvramSensorDevIndex;

    float                   __afScaleW = 1.0f;
    float                   __afScaleH = 1.0f;
    int                     __afOffsetX = 0;
    int                     __afOffsetY = 0;

    FastLogger              __fastLogger;

    MPoint                  __touchPt;  //For FF

    MUINT                   __sensorSenarios[2];
    bool                    __wasAFStable[2] = {false, false};

    MUINT8                 *__convAddr = 0; // point to the address of NVRAM
    MUINT32                 __convergenceData[128];
    const size_t            __CONV_SIZE = sizeof(__convergenceData);  //bytes
};

#if (1==SINGLE_LINE_LOG)
    #define FAST_LOGD(fmt, arg...)  __fastLogger.FastLogD(fmt, ##arg)
    #define FAST_LOGI(fmt, arg...)  __fastLogger.FastLogI(fmt, ##arg)
#else
    #define FAST_LOGD(fmt, arg...)  __fastLogger.FastLogD("[%s]" fmt, __func__, ##arg)
    #define FAST_LOGI(fmt, arg...)  __fastLogger.FastLogI("[%s]" fmt, __func__, ##arg)
#endif
#define FAST_LOGW(fmt, arg...)  __fastLogger.FastLogW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
#define FAST_LOGE(fmt, arg...)  __fastLogger.FastLogE("[%s] %s ERROR(%5d):" fmt, __func__,__FILE__, __LINE__, ##arg)

#define FAST_LOG_PRINT  __fastLogger.print()

#define IS_DUMP_ENALED \
    ( __RUN_N3D && \
      NULL != __dumpHint \
    )

#endif  // N3D_HAL_IMP_H_