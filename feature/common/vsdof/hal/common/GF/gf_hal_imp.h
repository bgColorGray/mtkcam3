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
#ifndef _GF_HAL_IMP_H_
#define _GF_HAL_IMP_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "GF_HAL"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <libgf/MTKGF.h>
#pragma GCC diagnostic pop

#include <gf_hal.h>
#include <n3d_hal.h>
#include <mtkcam/aaa/aaa_hal_common.h> // For DAF_TBL_STRUCT
#include <vector>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <deque>
#include <mtkcam3/feature/stereo/hal/FastLogger.h>
#include <mtkcam/utils/std/Log.h>
#include <vsdof/hal/ProfileUtil.h>
#include <thread>

using namespace NS3Av3;
using namespace android;
using namespace StereoHAL;
using namespace NSCam;
using namespace std;

//NOTICE: property has 31 characters limitation
//debug.STEREO.log.hal.gf [0: disable] [1: enable]
#define LOG_PERPERTY  PROPERTY_ENABLE_LOG".hal.gf"

enum ENUM_GF_TRIGGER_TIMING
{
    E_TRIGGER_BEFORE_AND_AFTER_AF,
    E_TRIGGER_AFTER_AF,
};

class GF_HAL_IMP : public GF_HAL
{
public:
    GF_HAL_IMP(GF_HAL_INIT_DATA &initData);
    virtual ~GF_HAL_IMP();

    virtual bool GFHALRun(GF_HAL_IN_DATA &inData, GF_HAL_OUT_DATA &outData);

    virtual bool GFAFRun(GF_HAL_IN_DATA &inData);
protected:

private:

    struct CoordTransformParam {
        float scaleW  = 1.0f;
        float scaleH  = 1.0f;
        int   offsetX = 0;
        int   offsetY = 0;

        CoordTransformParam() {}

        CoordTransformParam(float w, float h, int x, int y)
        {
            scaleW  = w;
            scaleH  = h;
            offsetX = x;
            offsetY = y;
        }

        CoordTransformParam(float w, float h, MPoint offset)
        {
            scaleW  = w;
            scaleH  = h;
            offsetX = offset.x;
            offsetY = offset.y;
        }

        CoordTransformParam(MSize srcSize, MPoint offset, MSize dstSize)
        {
            scaleW  = static_cast<float>(dstSize.w)/srcSize.w;
            scaleH  = static_cast<float>(dstSize.h)/srcSize.h;
            offsetX = offset.x;
            offsetY = offset.y;
        }

        CoordTransformParam(const MRect &srcRect, MSize dstSize)
        {
            scaleW  = static_cast<float>(dstSize.w)/srcRect.s.w;
            scaleH  = static_cast<float>(dstSize.h)/srcRect.s.h;
            offsetX = srcRect.p.x;
            offsetY = srcRect.p.y;
        }
    };

    void _setGFParams(GF_HAL_IN_DATA &gfHalParam);
    void _runGF(GF_HAL_OUT_DATA &gfHalOutput);
    void __runGFAFThread();

    void _initAFWinTransform();
    void _initFDDomainTransform();
    bool _validateAFPoint(MPoint &ptAF);
    MPoint &_rotateAFPoint(MPoint &ptAF);
    MPoint _AFToGFPoint(const MPoint &ptAF);
    MRect _getAFRect(const size_t AF_INDEX);
    MPoint _getTouchPoint(MPoint ptIn);
    void _updateDACInfo(const size_t AF_INDEX, GFDacInfo &dacInfo);
    void _removePadding(IImageBuffer *src, IImageBuffer *dst, StereoArea &srcArea);

    void _clearTransformedImages();
    bool _rotateResult(GFBufferInfo &gfResult, IImageBuffer *targetBuffer);

    void __facesToFDinfo(MtkCameraFaceMetadata *fd, GFFdInfo &fdInfo, size_t fdIndex);
    void __facePointToGFPoint(int inX, int inY, int &outX, int &outY);
    MPoint __faceRectToTriggerPoint(MRect &rect);
    MRect __faceToMRect(MtkCameraFace &face);
    MRect __faceRectToFocusRect(MRect &rect);
    CoordTransformParam &__getFDTransformParam(float main1RoomRatio);
    CoordTransformParam &__getAFTransformParam(float main1RoomRatio);

    void __updateZoomParam(float zoomRatio, GFProcInfo &procInfo);

    bool __loadConvergence();

    //Log
    void __logInitData();
    void __logSetProcData();
    void __logGFResult();
    void __logGFBufferInfo(const GFBufferInfo &buf, int index = -1);

    //Dump
    void __dumpInitInfo();
    void __dumpProcInfo();
    void __dumpResultInfo();
    void __dumpWorkingBuffer();
private:
    std::shared_ptr<N3D_HAL> __n3d = nullptr;
    MTKGF           *__pGfDrv;
    bool            __outputDepthmap = false;
    bool            __isFirstFrame = true;  //Assumption: GF HAL will be re-created for VR mode

    size_t          __magicNumber   = 0;
    size_t          __requestNumber = 0;
    ENUM_STEREO_SCENARIO __eScenario;

    GFInitInfo      __initInfo;
    GFProcInfo      __procInfo;
    GFResultInfo    __resultInfo;
    const uint32_t  __GF_MAX_FD_COUNT = sizeof(__procInfo.face)/sizeof(__procInfo.face[0]);

    const bool      LOG_ENABLED;

    DAF_TBL_STRUCT          *__pAFTable      = NULL;
    bool                     __isAFSupported = false;
    static MPoint            s_lastGFPoint;
    static bool              s_wasAFTrigger;
    bool                     __hasFace = false;

    // Domain transform
    std::unordered_map<float, CoordTransformParam> __fdTransformMap;
    std::unordered_map<float, CoordTransformParam> __afTransformMap;

    std::vector<sp<IImageBuffer>>    __transformedImages;

    sp<IImageBuffer>    __confidenceMap;
    sp<IImageBuffer>    __workBufImage;        // Working Buffer

    // multithread GFAF
    thread                  __GFAFThread;
    std::mutex              __GFAFMutex;
    std::condition_variable __GFAFCondVar;
    bool                    __wakeupGFAF = false;
    bool                    __leaveGFAF = false;

    bool            __triggerAfterCapture = false;

    const MSize DMBG_SIZE;
    const int DMBG_IMG_SIZE;
    StereoArea __confidenceArea;
    StereoArea __depthInputArea;

    FastLogger              __fastLogger;

    int                     __triggerTiming = checkStereoProperty("vendor.STEREO.gf_trigger_timing", E_TRIGGER_BEFORE_AND_AFTER_AF);
    int                     __isFDFirst = checkStereoProperty("vendor.STEREO.gf_fd_first", 1);

    MPoint                  __lastTouchPoint = MPoint(-1, -1);

    float                   __previewCropRatio = 1.0f;
    const float             __MIN_P1_CROP_RATIO = StereoSettingProvider::getVSDoFMinPass1CropRatio();

    //Dump
    TuningUtils::FILE_DUMP_NAMING_HINT *__dumpHint = NULL;
    const static size_t     __DUMP_BUFFER_SIZE = 512;
    size_t                  __initDumpSize = 0;
    std::string             __dumpInitData;
    char                    __dumpProcData[__DUMP_BUFFER_SIZE];
    char                    __dumpResultData[__DUMP_BUFFER_SIZE];
};

#if defined(__func__)
#undef __func__
#endif
#define __func__ __FUNCTION__

#define GF_HAL_DEBUG
#ifdef GF_HAL_DEBUG    // Enable debug log.
    #define MY_LOGD(fmt, arg...)    CAM_LOGD("[%s]" fmt, __func__, ##arg)
    #define MY_LOGI(fmt, arg...)    CAM_LOGI("[%s]" fmt, __func__, ##arg)
    #define MY_LOGW(fmt, arg...)    CAM_LOGW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
    #define MY_LOGE(fmt, arg...)    CAM_LOGE("[%s] %s ERROR(%5d):" fmt, __func__,__FILE__, __LINE__, ##arg)
#else   // Disable debug log.
    #define MY_LOGD(a,...)
    #define MY_LOGI(a,...)
    #define MY_LOGW(a,...)
    #define MY_LOGE(a,...)
#endif  // GF_HAL_DEBUG

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