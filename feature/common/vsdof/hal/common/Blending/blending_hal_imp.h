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
#ifndef _BLENDING_HAL_IMP_H_
#define _BLENDING_HAL_IMP_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "BLENDING_HAL"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <libbokehblend/MTKBokehBlend.h>
#pragma GCC diagnostic pop

#include <blending_hal.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam3/feature/stereo/hal/FastLogger.h>
#include <mtkcam/utils/std/Log.h>
#include <vsdof/hal/ProfileUtil.h>

using namespace android;
using namespace StereoHAL;
using namespace NSCam;
using namespace std;

//debug.STEREO.log.hal.blending [0: disable] [1: enable]
#define LOG_PERPERTY  PROPERTY_ENABLE_LOG".hal.blending"

class BLENDING_HAL_IMP : public BLENDING_HAL
{
public:
    BLENDING_HAL_IMP(ENUM_STEREO_SCENARIO eScenario);
    virtual ~BLENDING_HAL_IMP();

    virtual bool BlendingHALRun(BLENDING_HAL_IN_DATA &inData, BLENDING_HAL_OUT_DATA &outData);
protected:

private:
    bool __init();
    bool __setBlendingParams(BLENDING_HAL_IN_DATA &blendingHalParam, BLENDING_HAL_OUT_DATA &blendingHalOutput);
    bool __runBlending(BLENDING_HAL_OUT_DATA &blendingHalOutput);

    //Log
    void __logInitData();
    void __logSetProcData();
    void __logBufferInfo(BokehBlendBufferInfo &buffer);

    //Dump
    void __dumpInitInfo();
    void __dumpProcInfo();
    void __dumpWorkingBuffer();
private:
    MTKBokehBlend   *__pBlendingDrv;

    MINT32          __magicNumber   = 0;
    MINT32          __requestNumber = 0;
    ENUM_STEREO_SCENARIO __eScenario;
    StereoArea      __imageArea;
    MSize           __imageContentSize;

    sp<IImageBuffer>        __workingBufferImg;
    BokehBlendInitInfo      __initInfo;
    BokehBlendProcInfo      __procInfo;
    // Tuning param
    vector<MUINT8>                  __alphaTable;
    vector<BokehBlendTuningParam>   __tuningParams;

    const bool      LOG_ENABLED;

    const MSize DMBG_SIZE;
    const int DMBG_IMG_SIZE;

    FastLogger              __fastLogger;

    //Dump
    TuningUtils::FILE_DUMP_NAMING_HINT *__dumpHint = NULL;
    int                     __dumpLevel = 2;
    const static size_t     __DUMP_BUFFER_SIZE = 512;
    size_t                  __initDumpSize = 0;
    char                    __dumpInitData[__DUMP_BUFFER_SIZE];
    char                    __dumpProcData[__DUMP_BUFFER_SIZE];
};

#if defined(__func__)
#undef __func__
#endif
#define __func__ __FUNCTION__

#define BLENDING_HAL_DEBUG
#ifdef BLENDING_HAL_DEBUG    // Enable debug log.
    #define MY_LOGD(fmt, arg...)    CAM_LOGD("[%s]" fmt, __func__, ##arg)
    #define MY_LOGI(fmt, arg...)    CAM_LOGI("[%s]" fmt, __func__, ##arg)
    #define MY_LOGW(fmt, arg...)    CAM_LOGW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
    #define MY_LOGE(fmt, arg...)    CAM_LOGE("[%s] %s ERROR(%5d):" fmt, __func__,__FILE__, __LINE__, ##arg)
#else   // Disable debug log.
    #define MY_LOGD(a,...)
    #define MY_LOGI(a,...)
    #define MY_LOGW(a,...)
    #define MY_LOGE(a,...)
#endif  // BLENDING_HAL_DEBUG

#define MY_LOGV_IF(cond, arg...)    if (cond) { MY_LOGV(arg); }
#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }


#define SINGLE_LINE_LOG 0
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

#endif