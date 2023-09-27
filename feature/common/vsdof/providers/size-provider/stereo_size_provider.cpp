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
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "StereoSizeProvider"

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
CAM_ULOG_DECLARE_MODULE_ID(CAM_ULOG_MODULE_ID);

#include <math.h>
#include <algorithm>
#include <string.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/drv/iopipe/CamIO/Cam_QueryDef.h>

#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include "stereo_size_provider_imp.h"
#include "pass2/pass2A_size_providers.h"
#include <vsdof/hal/ProfileUtil.h>
#include <stereo_tuning_provider.h>
#include "../setting-provider/stereo_setting_provider_kernel.h"
#include "base_size.h"
#include <stereo_crop_util.h>

#if (1==HAS_AIDEPTH)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <MTKAIDepth.h>
#pragma GCC diagnostic pop
#endif

#if (1==HAS_VAIDEPTH)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <MTKVideoAIDepth.h>
#pragma GCC diagnostic pop
#endif

using namespace android;
using namespace NSCam;

#define STEREO_SIZE_PROVIDER_DEBUG

#ifdef STEREO_SIZE_PROVIDER_DEBUG    // Enable debug log.

#undef __func__
#define __func__ __FUNCTION__

#define MY_LOGD(fmt, arg...)    CAM_ULOGMD("[%s]" fmt, __func__, ##arg)
#define MY_LOGI(fmt, arg...)    CAM_ULOGMI("[%s]" fmt, __func__, ##arg)
#define MY_LOGW(fmt, arg...)    CAM_ULOGMW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
#define MY_LOGE(fmt, arg...)    CAM_ULOGME("[%s] %s ERROR(%5d):" fmt, __func__,__FILE__, __LINE__, ##arg)

#else   // Disable debug log.
#define MY_LOGD(a,...)
#define MY_LOGI(a,...)
#define MY_LOGW(a,...)
#define MY_LOGE(a,...)
#endif  // STEREO_SIZE_PROVIDER_DEBUG

#define MY_LOGV_IF(cond, arg...)    if (cond) { MY_LOGV(arg); }
#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }

const char *STEREO_BUFFER_NAMES[] =
{
    //P2A output for N3D
    "RECT_IN_M",
    "RECT_IN_S",
    "MASK_IN_M",
    "MASK_IN_S",
    //P2A output for DL-Depth
    "DLD_P2A_M",
    "DLD_P2A_S",

    //N3D Output
    "MV_Y",
    "MASK_M_Y",
    "SV_Y",
    "MASK_S_Y",
    "LDC",

    "WARP_MAP_M",
    "WARP_MAP_S",

    //N3D before MDP for capture
    "MV_Y_LARGE",
    "MASK_M_Y_LARGE",
    "SV_Y_LARGE",
    "MASK_S_Y_LARGE",

    //DPE Output
    "DMP_H",
    "CFM_H",
    "RESPO",

    //DPE2
    "DV_LR",
    "ASF_CRM",
    "ASF_RD",
    "ASF_HF",
    "CFM_M",

    //DL Depth
    "DL_DEPTHMAP",

    //OCC Output
    "MY_S",
    "DMH",
    "OCC",
    "NOC",

    //WMF Output
    "DMW",
    "DEPTH_MAP",

    //Video AIDepth output
    "VAIDEPTH_DEPTHMAP",

    //GF Output
    "DMBG",
    "DMG",
    "INK",

    //GF input sizes is from output size, so put them into the same group
    "GF_IN_IMG_2X",
    "GF_IN_IMG_4X",

    //Bokeh Output
    "BOKEH_WROT",
    "BOKEH_WDMA", //Clean image
    "BOKEH_3DNR",
    "BOKEH_PACKED_BUFFER",

    // Slant camera
    "FE1B_INPUT",
};

static_assert(STEREO_BUFFER_NAME_COUNT == sizeof(STEREO_BUFFER_NAMES)/sizeof(*STEREO_BUFFER_NAMES),
              "Please check content of STEREO_BUFFER_NAMES and ENUM_BUFFER_NAME");

// STEREO_BASE_SIZE_4_3 is defined in base_size.h under platform folder or using default if not exist

std::unordered_map<int, StereoArea> sizeCache1x;
StereoArea
StereoSize::getStereoArea1x(const StereoSizeConfig &config)
{
    StereoArea area = STEREO_BASE_SIZE_4_3;
    const int MAX_CONTENT_WIDTH = 640;  //DPE input constrain
    const int MAX_PADDING_WIDTH = 128;  //DPE input constrain

#if (1 == CUSTOMIZE_SLANT_SIZE)
    if(StereoSettingProvider::isSlantCameraModule())
    {
        area = STEREO_BASE_SIZE_4_3_SLANT;
    }
#else
#ifdef CUSTOMIZE_REFINE_NONE_SIZE
    if((config.featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP) &&
       E_DEPTHMAP_REFINE_NONE == config.refineLevel)
    {
        area = STEREO_BASE_SIZE_4_3_REFINE_NONE;
    }
#endif  // CUSTOMIZE_REFINE_NONE_SIZE
#endif  // CUSTOMIZE_SLANT_SIZE

    const bool IS_VERTICAL_MODULE = StereoHAL::isVertical(config.moduleRotation);
    STEREO_IMAGE_RATIO_T imageRatio = config.imageRatio;
    if(!StereoSizeProviderImp::getInstance()->getCustomziedSize(imageRatio, area)) {
        auto iter = sizeCache1x.find(imageRatio);
        if(iter != sizeCache1x.end()) {
            area = iter->second;
        } else {
            int m, n;
            imageRatio.MToN(m, n);

            ENUM_IMAGE_RATIO_ALIGNMENT alignment = E_KEEP_AREA_SIZE;
#ifdef CUSTOMIZE_RATIO_ALIGNMENT
            alignment = CUSTOMIZE_RATIO_ALIGNMENT;
#endif
            if(E_KEEP_WIDTH == alignment) {
                if(m * 3 / 4 < n) {
                    alignment = E_KEEP_HEIGHT;
                }
            }
            else if(E_KEEP_HEIGHT == alignment)
            {
                if(n * 4 / 3 < m) {
                    alignment = E_KEEP_WIDTH;
                }
            }

            area.applyRatio(imageRatio, alignment);

            MSize pvSize = config.previewSize;
            if(area.size.w > pvSize.w ||
               area.size.h > pvSize.h)
            {
                MY_LOGD("Base size changes to preview size %dx%d", pvSize.w, pvSize.h);
                area.size = pvSize;
            }

            area.rotatedByModule(ROTATE_WO_SLANT);

            MSize contentSize = area.contentSize();
            int nAlign = 16;
#ifdef CUSTOMIZE_BUFFER_ALIGNMENT
            nAlign = CUSTOMIZE_BUFFER_ALIGNMENT;
#endif
            applyNAlign(nAlign, contentSize.w);

            if(contentSize.w > MAX_CONTENT_WIDTH)
            {
                contentSize.w = MAX_CONTENT_WIDTH;
                applyNAlign(nAlign, contentSize.w);
            }

            if(area.padding.w > MAX_PADDING_WIDTH)
            {
                area.startPt.x -= (area.padding.w-MAX_PADDING_WIDTH)/2;
                area.padding.w = MAX_PADDING_WIDTH;
            }

            if(IS_VERTICAL_MODULE) {
                contentSize.h = contentSize.w * m / n;
            } else {
                contentSize.h = contentSize.w * n / m;
            }
            contentSize.h = (contentSize.h+1) & (~1);

            area.size = area.padding + contentSize;

            sizeCache1x.emplace(imageRatio, area);

            char msg[128];
            if(sprintf(msg, "StereoArea1x(%s, %s rotate)", (const char *)imageRatio,
                       IS_VERTICAL_MODULE ? "w":"wo") >= 0)
            {
                area.print(LOG_TAG, msg);
            }
        }
    }

    //Vertical size is rotated, only rotate for horizontal request
    if(IS_VERTICAL_MODULE &&
       !config.needRotate)
    {
        area.rotate(90);
    }

    if(!config.hasPadding) {
        area.removePadding();
    }

    return area;
}

//===============================================================
//  StereoSizeProvider implementation
//===============================================================

//===============================================================
//  Singleton and init operations
//===============================================================
Mutex __lock;
Mutex StereoSizeProviderImp::__pass1SizeLock;
Mutex StereoSizeProviderImp::__activeCropLock;
Mutex StereoSizeProviderImp::__bufferSizeLock;
Mutex StereoSizeProviderImp::__resetLock;

StereoSizeProviderImp *g_pInstance = nullptr;
#define IS_CACHED (true)

StereoSizeProvider *
StereoSizeProvider::getInstance()
{
    if(g_pInstance == nullptr) {
        Mutex::Autolock lock(__lock);
        // Double check is required
        if(g_pInstance == nullptr) {
            g_pInstance = new StereoSizeProviderImp();
        }
    }

    return g_pInstance;
}

void
StereoSizeProvider::destroyInstance()
{
    if(g_pInstance) {
        Mutex::Autolock lock(__lock);
        // Double check is required
        if(g_pInstance) {
            delete g_pInstance;
            g_pInstance = nullptr;
        }
    }
}

StereoSizeProviderImp::StereoSizeProviderImp()
    : __fastLogger(LOG_TAG, PROPERTY_ENABLE_LOG)
{
    __fastLogger.setSingleLineMode(false);
}

StereoSizeProviderImp::~StereoSizeProviderImp()
{
}

void
StereoSizeProviderImp::reset()
{
    Mutex::Autolock lock(__resetLock);
    __LOG_ENABLED = StereoSettingProvider::isLogEnabled();
    MY_LOGD_IF(__LOG_ENABLED, "");

    __imgoYuvSizes.clear();
    __rrzoYuvSizes.clear();
    __customizedSize.clear();
    __aidepthModelSize.clear();
    __vaidepthModelSize.clear();
    __pass1SizeMap.clear();
    __activeCropMap.clear();
    __bufferSizeMap.clear();
    sizeCache1x.clear();
}

bool
StereoSizeProviderImp::getPass1Size( const int32_t SENSOR_INDEX,
                                     NSCam::EImageFormat format,
                                     NSImageio::NSIspio::EPortIndex port,
                                     StereoHAL::ENUM_STEREO_SCENARIO scenario,
                                     NSCam::MRect &tgCropRect,
                                     NSCam::MSize &outSize,
                                     MUINT32 &strideInBytes,
                                     float previewCropRatio,
                                     STEREO_IMAGE_RATIO_T imageRatio
                                   )
{
    ENUM_STEREO_SENSOR sensor = StereoSettingProvider::sensorIdToEnum(SENSOR_INDEX);
    if(sensor == eSTEREO_SENSOR_UNKNOWN) {
        return false;
    }

    return getPass1Size({sensor, format, port, scenario, previewCropRatio ,imageRatio}, tgCropRect, outSize, strideInBytes);
}

bool
StereoSizeProviderImp::getPass1Size( ENUM_STEREO_SENSOR sensor,
                                     EImageFormat format,
                                     EPortIndex port,
                                     ENUM_STEREO_SCENARIO scenario,
                                     MRect &tgCropRect,
                                     MSize &outSize,
                                     MUINT32 &strideInBytes,
                                     float previewCropRatio,
                                     STEREO_IMAGE_RATIO_T imageRatio
                                   )
{
    return getPass1Size({sensor, format, port, scenario, previewCropRatio, imageRatio},
                        tgCropRect, outSize, strideInBytes );
}

bool
StereoSizeProviderImp::getPass1Size( P1SizeQueryParam param,
                                     NSCam::MRect &tgCropRect,
                                     NSCam::MSize &outSize,
                                     MUINT32      &strideInBytes )
{
    P1SizeResult result;
    bool isSuccess = getPass1Size(param, result);
    tgCropRect    = result.tgCropRect;
    outSize       = result.outSize;
    strideInBytes = result.strideInBytes;
    return isSuccess;
}

StereoSizeProvider::SIZE_KEY_TYPE
StereoSizeProvider::P1SizeQueryParam::getHashKey() const
{
    enum {
        SENSOR_BITS          = 3,   //Up to 7(~main7) sensors in a logical device
        PORT_BITS            = 3,   //RRZO, IMGO, CRZO_R2, RSSO_R2
        SCENARIO_BITS        = 2,
        ZOOM_RATIO_BITS      = 7,   // 0.xx->XX
        FORMAT_BITS          = 17,  //Last line
    };

    // SENSOR_BITS
    SIZE_KEY_TYPE key = sensor;
    int shiftBits = SENSOR_BITS;

    // PORT_BITS
    switch(port) {
        case EPortIndex_RRZO:
        case EPortIndex_YUVO:
        default:
            // Default is key |= 0<<shiftBits
            break;
        case EPortIndex_IMGO:
            key |= 1<<shiftBits;
            break;
        case EPortIndex_CRZO_R2:
            key |= 2<<shiftBits;
            break;
        case EPortIndex_RSSO_R2:
            key |= 3<<shiftBits;
            break;
    }
    shiftBits += PORT_BITS;

    // SCENARIO_BITS
    key |= scenario<<shiftBits;
    shiftBits += SCENARIO_BITS;

    int zoomRatio = static_cast<int>(previewCropRatio*100);
    key |= zoomRatio<<shiftBits;
    shiftBits += ZOOM_RATIO_BITS;

    key |= format << shiftBits; //put this line at last

    return key;
}

std::string
StereoSizeProvider::P1SizeQueryParam::toString() const
{
    std::ostringstream oss;
    int sensorID = StereoSettingProvider::sensorEnumToId(sensor);
    oss << "Sensor main" << sensor << "(" << sensorID << ")";
    oss << ", Port ";
    switch(port)
    {
        case EPortIndex_RRZO:
            oss << "RRZO";
            break;
        case EPortIndex_IMGO:
            oss << "IMGO";
            break;
        case EPortIndex_CRZO_R2:
            oss << "CRZO_R2";
            break;
        case EPortIndex_RSSO_R2:
            oss << "RSSO_R2";
            break;
        case EPortIndex_YUVO:
            oss << "YUVO";
            break;
        default:
            oss << "Unknown/Unsupported";
    }
    oss << ", Format 0x" << std::hex << format << std::dec;
    oss << ", Scenario " << scenario;
    int zoomRatio = static_cast<int>(previewCropRatio*100);
    if(zoomRatio < 100) {
        oss << ", Preview Crop Ratio: 0." << zoomRatio;
    } else {
        oss << ", Preview Crop Ratio: 1.0";
    }

    return oss.str();
}

std::string
StereoSizeProvider::P1SizeResult::toString() const
{
    std::ostringstream oss;
    oss << "tg (" << tgCropRect.p.x << ", " << tgCropRect.p.y << "), " << tgCropRect.s.w << "x" << tgCropRect.s.h;
    oss << ", outSize " << outSize.w << "x" << outSize.h;
    oss << ", stride " << strideInBytes;
    return oss.str();
}

bool
StereoSizeProviderImp::getPass1Size( P1SizeQueryParam param, P1SizeResult &output )
{
    // AutoProfileUtil profile(LOG_TAG, "getPass1Size");
    STEREO_IMAGE_RATIO_T currentRatio = (EPortIndex_IMGO == param.port) ? eStereoRatio_4_3 : param.imageRatio;

    int sensorScenario = StereoSettingProvider::getCurrentSensorScenario(param.sensor);
    if(SENSOR_SCENARIO_ID_UNNAMED_START == sensorScenario) {
        MY_LOGE("Unknown sensor scenario for sensor %d", param.sensor-1);
        return false;
    }

    if(param.previewCropRatio <= 0 ||
       param.previewCropRatio > 1) {
        param.previewCropRatio = 1.0f;
    }
    SIZE_KEY_TYPE key = param.getHashKey();

    int m, n;
    currentRatio.MToN(m, n);

    int sensorID = StereoSettingProvider::sensorEnumToId(param.sensor);
    if(sensorID < 0) {
        return false;
    }

    auto printSize = [&](bool isCached) {
        if(!__LOG_ENABLED) {
            return;
        }

        std::ostringstream oss;
        if(isCached) {
            oss << "Use";
        } else {
            oss << "New";
        }
        oss << " result of " << param.toString() << ": " << output.toString();

        // For printing
        #ifdef __func__
        #undef __func__
        #endif
        #define __func__ "getPass1Size"
        MY_LOGD("%s", oss.str().c_str());
        #undef __func__
    };

    {
        Mutex::Autolock lock(__pass1SizeLock);
        auto iter = __pass1SizeMap.find(key);
        if(iter != __pass1SizeMap.end()) {
            output = iter->second;
            printSize(IS_CACHED);
            return true;
        }
    }

    auto saveToCache = [&] {
        Mutex::Autolock lock(__pass1SizeLock);
        __pass1SizeMap.emplace(key, output);
        printSize(!IS_CACHED);
    };

    //Return true if size has been changed
    auto updateSizeAndStride = [&](MSize &size, MUINT32 &stride) -> bool
    {
        // For printing
        #ifdef __func__
        #undef __func__
        #endif
        #define __func__ "getPass1Size"
        auto paramStr = param.toString();
        MY_LOGD("[updateSizeAndStride] param %s", paramStr.c_str());
        #undef __func__

        NSCam::NSIoPipe::NSCamIOPipe::NormalPipe_QueryInfo queryRst;
        NSCam::NSIoPipe::NSCamIOPipe::NormalPipe_QueryIn input;
        input.width = size.w;
        input.pixMode = NSCam::NSIoPipe::NSCamIOPipe::ePixMode_NONE;
        NSCam::NSIoPipe::NSCamIOPipe::INormalPipeModule::get()->query(
                param.port,
                NSCam::NSIoPipe::NSCamIOPipe::ENPipeQueryCmd_X_PIX|
                NSCam::NSIoPipe::NSCamIOPipe::ENPipeQueryCmd_STRIDE_BYTE,
                param.format,
                input,
                queryRst);

        stride = queryRst.stride_byte;
        if(EPortIndex_IMGO != param.port &&
           size.w != queryRst.x_pix)
        {
            size.w = queryRst.x_pix;
            return true;
        }

        return false;
    };

    // Prepare sensor hal
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(pHalDeviceList == nullptr) {
        return false;
    }
    int sensorDevIndex = pHalDeviceList->querySensorDevIdx(sensorID);
    IHalSensorList* sensorList = MAKE_HalSensorList();
    if(NULL == sensorList) {
        MY_LOGE("Cannot get sensor list");
        return false;
    }

    IHalSensor* pIHalSensor = sensorList->createSensor(LOG_TAG, sensorID);
    if(NULL == pIHalSensor) {
        MY_LOGE("Cannot get hal sensor");
        return false;
    }

    //Get sensor crop win info
    SensorCropWinInfo rSensorCropInfo;
    int err = pIHalSensor->sendCommand(sensorDevIndex, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                       (MUINTPTR)&sensorScenario, (MUINTPTR)&rSensorCropInfo, 0);
    pIHalSensor->destroyInstance(LOG_TAG);

    if(err) {
        MY_LOGE("Cannot get sensor crop win info");
        return false;
    }

    output.tgCropRect.p.x = 0;
    output.tgCropRect.p.y = 0;
    output.tgCropRect.s.w = rSensorCropInfo.w2_tg_size;
    output.tgCropRect.s.h = rSensorCropInfo.h2_tg_size;

    output.outSize.w = rSensorCropInfo.w2_tg_size;
    output.outSize.h = rSensorCropInfo.h2_tg_size;

    // IMGO does not support resize & crop, so we just update stride and return
    if( EPortIndex_IMGO == param.port ) {
        updateSizeAndStride(output.outSize, output.strideInBytes);
        saveToCache();
        return true;
    }

    //Calculate TG CROP without FOV cropping
    if( EPortIndex_RRZO == param.port ||
        EPortIndex_YUVO == param.port )
    {
        __getRRZOSize(param.sensor, param.format, param.scenario,
                      currentRatio, param.previewCropRatio,
                      output.tgCropRect, output.outSize);
    }
    else if( EPortIndex_CRZO_R2 == param.port ||
             EPortIndex_RSSO_R2 == param.port )
    {
        //P1 YUV is used only for preview
        //RRZO is for preview/record only, if scenario is capture, query by preview
        ENUM_STEREO_SCENARIO queryScenario = (eSTEREO_SCENARIO_CAPTURE == param.scenario)
                                             ? eSTEREO_SCENARIO_PREVIEW
                                             : param.scenario;
        MSize outputSize = [&] {
            StereoArea area;
            switch(param.sensor) {
                case eSTEREO_SENSOR_MAIN1:
                default:
                    area = getBufferSize(E_RECT_IN_M, queryScenario, false, param.previewCropRatio);
                    break;
                case eSTEREO_SENSOR_MAIN2:
                    area = getBufferSize(E_RECT_IN_S, queryScenario, false, param.previewCropRatio);
                    break;
            }

            // P1 does not rotate
            area.removePadding();
            if(area.size.w < area.size.h) {
                std::swap(area.size.w, area.size.h);
            }

#if (1==HAS_WPE)
            // Main2 always uses WPE to warp
            // Main1 only uses WPE to warp for slant module
            if (param.sensor == eSTEREO_SENSOR_MAIN2 ||
                StereoSettingProvider::isSlantCameraModule()) {
                area.apply64Align();
            }
#endif
            return area.size;
        }();

#if (2==HAS_P1YUV)
        __getTGCrop(param.scenario, param.sensor, currentRatio,
                    param.previewCropRatio, output.tgCropRect);
#else
        // YUV sizes come from RRZO, so they must <= RRZO size
        MSize rrzoSize;
        MUINT32 junkStride;
        if(getPass1Size( {param.sensor, eImgFmt_FG_BAYER10, EPortIndex_RRZO, param.scenario, param.previewCropRatio, currentRatio},
                         output.tgCropRect, rrzoSize, junkStride ))
        {
            // MT6853/MT6833 has hardware limitation: YUV ports' size must <= RRZO size/2
#if (1==HAS_WPE)
            if (param.sensor == eSTEREO_SENSOR_MAIN2 ||
                StereoSettingProvider::isSlantCameraModule()) {
                rrzoSize.w = (rrzoSize.w/2) & ~63;
                rrzoSize.h = (rrzoSize.h/2) & ~1;
            } else {
                // Main1 without slant is warpped by CPU
                rrzoSize.w = (rrzoSize.w/2) & ~1;
                rrzoSize.h = (rrzoSize.h/2) & ~1;
            }
#else
            if (param.sensor == eSTEREO_SENSOR_MAIN2) {
                // Main1 is warpped by GPU
                rrzoSize.w = (rrzoSize.w/2) & ~32;
                rrzoSize.h = (rrzoSize.h/2) & ~32;
            } else {
                // Main1 is warpped by CPU
                rrzoSize.w = (rrzoSize.w/2) & ~1;
                rrzoSize.h = (rrzoSize.h/2) & ~1;
            }
#endif  // #if (1==HAS_WPE)
            if(outputSize.w > rrzoSize.w ||
               outputSize.h > rrzoSize.h)
            {
                MY_LOGD("RSSO_R2 changes from %dx%d to %dx%d",
                        outputSize.w, outputSize.h, rrzoSize.w, rrzoSize.h);
                outputSize = rrzoSize;
            }
        }
#endif  // #if (2!=HAS_P1YUV)

        //Make sure output size meet the following rules:
        // 1. image ratio fit 4:3 or 16:9
        // 2. smaller than TG size
        // 3. Is even number
        output.outSize = output.tgCropRect.s;
        fitSizeToImageRatio(output.outSize, currentRatio);

        //Pass1 does not support rotation
        if(outputSize.w < output.outSize.w ||
           outputSize.h < output.outSize.h)
        {
            output.outSize = outputSize;
        }
    }

    // Make sure all sizes <= IMGO size
    output.outSize.w = std::min((int)rSensorCropInfo.w2_tg_size, output.outSize.w);
    output.outSize.h = std::min((int)rSensorCropInfo.h2_tg_size, output.outSize.h);

    bool isSizeUpdated = updateSizeAndStride(output.outSize, output.strideInBytes);
    if(isSizeUpdated)
    {
        // Keep flexibility for 3rd party to config sizes
        if(!StereoSettingProvider::is3rdParty(param.scenario))
        {
        #if (1==HAS_WPE)
            if (EPortIndex_CRZO_R2 == param.port ||
                EPortIndex_RSSO_R2 == param.port) {
                StereoHAL::applyNAlign(64, output.outSize.w);
            }
        #endif
            // Fit ratio and apply 2-align
            output.outSize.h = (output.outSize.w * n / m)&~0x1;
        }

        MY_LOGD("Out size changes to %dx%d", output.outSize.w, output.outSize.h);
    }

    saveToCache();

    return true;
}

void StereoSizeProviderImp::__getRRZOSize(ENUM_STEREO_SENSOR sensor,
                                          EImageFormat format,
                                          ENUM_STEREO_SCENARIO scenario,
                                          STEREO_IMAGE_RATIO_T currentRatio,
                                          float previewCropRatio,
                                          MRect &tgCropRect, MSize &outSize) {
  const MSize TG_SIZE = tgCropRect.s;
  const int OUTPUT_ALIGNMENT = 64;
  // RRZO is for preview/record only, if scenario is capture, query by preview
  ENUM_STEREO_SCENARIO queryScenario = (eSTEREO_SCENARIO_CAPTURE == scenario)
                                           ? eSTEREO_SCENARIO_PREVIEW
                                           : scenario;

  // Check min RRZO size
  auto pModule = NSCam::NSIoPipe::NSCamIOPipe::INormalPipeModule::get();
  int RRZ_CAPIBILITY = 25;  // means 0.25
  if (pModule) {
    NSCam::NSIoPipe::NSCamIOPipe::NormalPipe_QueryInfo queryRst;
    NSCam::NSIoPipe::NSCamIOPipe::NormalPipe_QueryIn input;
    pModule->query(EPortIndex_RRZO,
                   NSCam::NSIoPipe::NSCamIOPipe::ENPipeQueryCmd_BS_RATIO,
                   format, input, queryRst);
    RRZ_CAPIBILITY = queryRst.bs_ratio;
  }

  auto getMinRRZOSize = [&](const MSize &tgSize) -> MSize {
    MSize result((tgSize.w * RRZ_CAPIBILITY / 100 + 1) & ~1,
                 (tgSize.h * RRZ_CAPIBILITY / 100 + 1) & ~1);
    fitSizeToImageRatio(result, currentRatio);
    return result;
  };

  int m, n;
  currentRatio.MToN(m, n);

  // For RRZO quality limitation, will apply to
  // 1. Main1 in VSDoF
  // 2. Every camera in multicam and zoom
  const MSize QUALITY_LIMITATION(1280, 960);
  auto __applyQualityLimiation = [&](MSize &rrzoSize) -> MSize {
    if (StereoSettingProvider::is3rdParty(queryScenario) ||
        eSTEREO_SENSOR_MAIN1 == sensor) {
      if (QUALITY_LIMITATION.w * n / m < QUALITY_LIMITATION.h) {
        if (rrzoSize.h < QUALITY_LIMITATION.h) {
          rrzoSize.h = QUALITY_LIMITATION.h;
          rrzoSize.w = rrzoSize.h * m / n;
        }
      } else {
        if (rrzoSize.w < QUALITY_LIMITATION.w) {
          rrzoSize.w = QUALITY_LIMITATION.w;
          rrzoSize.h = rrzoSize.w * n / m;
        }
      }
    }

    return rrzoSize;
  };

  if (StereoSettingProvider::is3rdParty(queryScenario)) {
    CropUtil::cropRectByImageRatio(tgCropRect, currentRatio);
    MSize MIN_RRZ_SIZE = getMinRRZOSize(tgCropRect.s);

    int sensorID = StereoSettingProvider::sensorEnumToId(sensor);
    outSize = [&] {
      if (sensorID >= 0) {
        auto iter = __rrzoYuvSizes.find(sensorID);
        if (iter != __rrzoYuvSizes.end()) {
          return iter->second;
        }
      }

      MSize result = StereoSizeProvider::getInstance()->getPreviewSize();
      result.w = std::min(result.w, tgCropRect.s.w);
      result.h = std::min(result.h, tgCropRect.s.h);
      __applyQualityLimiation(result);

      StereoHAL::applyNAlign(8, result.w);
      StereoHAL::applyNAlign(2, result.h);

      MY_LOGI(
          "NOTICE! RRZOYUV Size is not configured for sensor %d, adjust to "
          "%dx%d",
          sensorID, result.w, result.h);
      return result;
    }();

    // Enlarge and apply P2 constrain
    int eisFactor = static_cast<int>(StereoSettingProvider::getEISFactor());
    if (eisFactor > 100) {
      MSize newSize = outSize;
      newSize.w = newSize.w * eisFactor / 100;
      StereoHAL::applyNAlign(8, newSize.w);
      newSize.h = newSize.h * eisFactor / 100;
      StereoHAL::applyNAlign(2, newSize.h);
      outSize = newSize;
    }

    if (outSize.w < MIN_RRZ_SIZE.w || outSize.h < MIN_RRZ_SIZE.h) {
      __fastLogger.FastLogD(
          "[%s]RRZO %dx%d->%dx%d (limited by RRZ capibility: %.2f)",
          __FUNCTION__, outSize.w, outSize.h, MIN_RRZ_SIZE.w, MIN_RRZ_SIZE.h,
          RRZ_CAPIBILITY / 100.0f);
      outSize = MIN_RRZ_SIZE;
    }

    __fastLogger.FastLogD("[%s]3rdParty RRZO size=%dx%d, sensor: main%d(%d)",
                          __FUNCTION__, outSize.w, outSize.h, sensor, sensorID);
  } else {
    // 1. Get the crop and the size required for P2.
    //    For main2, this combination will produce an image with similar
    //    object size to main1. Since output size is unchangeable during
    //    runtime, we can only adjust TG crop if the FOV changes. So, if the
    //    size requires to adjust, tg crop needs to adjust, too

    MSize newOutSize;
    if (previewCropRatio == 1.0f) {
      __cropRectByFOVCropAndImageRatio(sensor, currentRatio, previewCropRatio,
                                       tgCropRect);

      // 2. Calculate the ratio from the original size and dest size for main2
      //    Adjust tg crop for the size change
      // Use smaller size for RRZO if possible
      if (eSTEREO_SENSOR_MAIN1 == sensor) {
        // Compare with PASS2A.WDMA (Preview size)
        outSize = Pass2A_SizeProvider::instance()->getWDMAArea(
            {queryScenario, ROTATE_WO_SLANT, previewCropRatio});
      } else {
        // Compare with PASS2A_P.WROT (FE input size)
        outSize = Pass2A_P_SizeProvider::instance()->getWROTArea(
            {queryScenario, ROTATE_WO_SLANT, 1.0f});
      }

      // P1 doesn't rotate, the image will be rotated in P2
      if (outSize.w < outSize.h) {
        std::swap(outSize.w, outSize.h);
      }

      newOutSize = outSize;
    } else {
      float minRatio = StereoSettingProvider::getVSDoFMinPass1CropRatio();

      // If ratio is under minRatio, use the result of minRatio
      if (previewCropRatio < minRatio) {
        __fastLogger
            .FastLogD(
                "[%s] Preview crop ratio %f(%.2fx) is limited by %f(%.2fx)",
                __FUNCTION__, previewCropRatio, 1.0f / previewCropRatio,
                minRatio, 1.0f / minRatio)
            .print();
        previewCropRatio = minRatio;

        MUINT32 junkStride;
        getPass1Size({sensor, eImgFmt_FG_BAYER10, EPortIndex_RRZO, scenario,
                      minRatio, currentRatio},
                     tgCropRect, outSize, junkStride);
        return;
      }

      __cropRectByFOVCropAndImageRatio(sensor, currentRatio, previewCropRatio,
                                       tgCropRect);

      // Output size should not change by zoom ratio, so we have to keep using
      // size of 1.0x
      MUINT32 junkStride;
      MRect junkTgCrop;
      getPass1Size({sensor, eImgFmt_FG_BAYER10, EPortIndex_RRZO, scenario, 1.0f,
                    currentRatio},
                   junkTgCrop, outSize, junkStride);

      // Zooming may cause size change, if so, tg crop requires to update by
      // the ratio
      newOutSize = outSize;
      if (previewCropRatio < 1.0f) {
        newOutSize = Pass2A_P_SizeProvider::instance()->getWROTArea(
            {queryScenario, ROTATE_WO_SLANT, previewCropRatio});
      }
    }

    // Output size must meets the following rules:
    // 0.(6853/6833 only): RSSO_R2 <= RRZO/2
    // 1. Bigger than quality limitation
    // 2. 64 align(for P2S)
    // 3. Smaller than hw max size
    // 4. Bigger than minimum scale size

#if (2 == HAS_P1YUV)
    // Apply rule 0: RSSO_R2 <= RRZO/2
    // For MT6853/MT6833 RSSO_R2 size is limited by RRZO/2
    // So if RSSO_R2 is not large enough, we need to enlarge RRZO size
    {
      MSize rssor2Size;
      MUINT32 junkStride;
      MRect junkTgCrop;
      if (getPass1Size({sensor, eImgFmt_Y8, EPortIndex_RSSO_R2, scenario, 1.0f,
                        currentRatio},
                       junkTgCrop, rssor2Size, junkStride)) {
        rssor2Size.w *= 2;
        rssor2Size.h *= 2;
        if (newOutSize.w < rssor2Size.w || newOutSize.h < rssor2Size.h) {
          newOutSize.w = std::max(newOutSize.w, rssor2Size.w);
          newOutSize.h = std::max(newOutSize.h, rssor2Size.h);
          __fastLogger.FastLogD("[%s]Update output size from %dx%d to %dx%d",
                                __FUNCTION__, outSize.w, outSize.h,
                                newOutSize.w, newOutSize.h);
        }
      }
    }
#endif

    // Apply rule 1: Bigger than quality limitation
    {
      MSize newOutSizeQuality = __applyQualityLimiation(newOutSize);
      if (__LOG_ENABLED && (newOutSizeQuality.w != newOutSize.w ||
                            newOutSizeQuality.h != newOutSize.h)) {
        __fastLogger.FastLogD(
            "[%s]Output size changes from %dx%d to %dx%d for quality",
            __FUNCTION__, newOutSize.w, newOutSize.h, newOutSizeQuality.w,
            newOutSizeQuality.h);
      }
      newOutSize = newOutSizeQuality;
    }

    // Apply rule 2: 64 align(for P2S)
    {
      MSize newOutSizeAlign = newOutSize;
      applyNAlign(OUTPUT_ALIGNMENT, newOutSizeAlign.w);
      applyNAlign(OUTPUT_ALIGNMENT, newOutSizeAlign.h);
      if (__LOG_ENABLED && (newOutSizeAlign.w != newOutSize.w ||
                            newOutSizeAlign.h != newOutSize.h)) {
        __fastLogger.FastLogD(
            "[%s]Output size changes from %dx%d to %dx%d for aligment",
            __FUNCTION__, newOutSize.w, newOutSize.h, newOutSizeAlign.w,
            newOutSizeAlign.h);
      }
      newOutSize = newOutSizeAlign;
    }

    // Apply rule 3: Smaller than max size
    {
      if (pModule) {
        NSCam::NSIoPipe::NSCamIOPipe::sCAM_QUERY_MAX_PREVIEW_SIZE maxRRZOWidth;
        maxRRZOWidth.QueryOutput = newOutSize.w;
        if (pModule->query(
                NSCam::NSIoPipe::NSCamIOPipe::ENPipeQueryCmd_MAX_PREVIEW_SIZE,
                (MUINTPTR)&maxRRZOWidth)) {
          if (newOutSize.w > maxRRZOWidth.QueryOutput) {
            __fastLogger.FastLogD("[%s]Change RRZO width from %d to %d",
                                  __FUNCTION__, newOutSize.w,
                                  maxRRZOWidth.QueryOutput);
            newOutSize.w = maxRRZOWidth.QueryOutput & ~(OUTPUT_ALIGNMENT - 1);
            newOutSize.h = newOutSize.w * n / m;
            applyNAlign(OUTPUT_ALIGNMENT, newOutSize.h);
          }
        }
      }
    }

    // Apply rule 4. Bigger than minimum scale size
    {
      // 1. Since output ratio is related to cropped TG area, so we have get
      // new TG crop first
      if (outSize.w != newOutSize.w) {
        float scale = static_cast<float>(newOutSize.w) / outSize.w;
        if (scale > 1.0f) {
          int newTGWidth = tgCropRect.s.w * scale;
          if (newTGWidth <= TG_SIZE.w) {
            tgCropRect.s.w = newTGWidth;
            tgCropRect.p.x = (TG_SIZE.w - newTGWidth) / 2;
          } else {
            __fastLogger.FastLogD(
                "[%s]Limited to enlarge TG crop width by full size "
                "(%d->%d->%d)",
                __FUNCTION__, tgCropRect.s.w, newTGWidth, TG_SIZE.w);
            tgCropRect.s.w = TG_SIZE.w;
            tgCropRect.p.x = 0;
          }
        } else {
          __fastLogger.FastLogD(
              "[%s]Ignore to shrink TG crop for smaller output size(%dx%d -> "
              "%dx%d)",
              __FUNCTION__, outSize.w, outSize.h, newOutSize.w, newOutSize.h);
          newOutSize.w = outSize.w;
        }
      }

      if (outSize.h != newOutSize.h) {
        float scale = static_cast<float>(newOutSize.h) / outSize.h;
        if (scale > 1.0f) {
          int newTGHeight = tgCropRect.s.h * scale;
          if (newTGHeight <= TG_SIZE.h) {
            tgCropRect.s.h = newTGHeight;
            tgCropRect.p.y = (TG_SIZE.h - newTGHeight) / 2;
          } else {
            __fastLogger.FastLogD(
                "[%s]Limited to enlarge TG crop height by full size "
                "(%d->%d->%d)",
                __FUNCTION__, tgCropRect.s.h, newTGHeight, TG_SIZE.h);
            tgCropRect.s.h = TG_SIZE.h;
            tgCropRect.p.y = 0;
          }
        } else {
          __fastLogger.FastLogD(
              "[%s]Ignore to shrink TG crop for smaller output size(%dx%d -> "
              "%dx%d)",
              __FUNCTION__, outSize.w, outSize.h, newOutSize.w, newOutSize.h);
          newOutSize.h = outSize.h;
        }
      }
    }

    // As we said before, we should only use size from 1x
    if (1.0f == previewCropRatio) {
      // 2. Calaulate minimum output size from HW capability
      //    Only 1x ratio need to check since outSize of other ratios aligns 1x
      MSize MIN_RRZ_SIZE = getMinRRZOSize(tgCropRect.s);
      if (newOutSize.w < MIN_RRZ_SIZE.w || newOutSize.h < MIN_RRZ_SIZE.h) {
        MSize newMinSize = newOutSize;
        newMinSize.w = std::max(newMinSize.w, MIN_RRZ_SIZE.w);
        newMinSize.h = std::max(newMinSize.h, MIN_RRZ_SIZE.h);
        applyNAlign(OUTPUT_ALIGNMENT, newMinSize.w);
        applyNAlign(OUTPUT_ALIGNMENT, newMinSize.h);

        __fastLogger.FastLogD(
            "[%s]Output size changes from %dx%d to %dx%d for minimum "
            "resizing",
            __FUNCTION__, newOutSize.w, newOutSize.h, newMinSize.w,
            newMinSize.h);
        newOutSize = newMinSize;
      }

      outSize = newOutSize;
    }

    // Since outSize may very due some reason(alignment, hw limitation, etc),
    // we have to check if tg crop is smaller than outSize
    MRect oldTgCrop = tgCropRect;
    if (tgCropRect.s.w < outSize.w) {
      tgCropRect.p.x -= (outSize.w - tgCropRect.s.w) / 2;
      tgCropRect.p.x = std::max(0, tgCropRect.p.x);
      tgCropRect.s.w = outSize.w;
    }

    if (tgCropRect.s.h < outSize.h) {
      tgCropRect.p.y -= (outSize.h - tgCropRect.s.h) / 2;
      tgCropRect.p.y = std::max(0, tgCropRect.p.y);
      tgCropRect.s.h = outSize.h;
    }

    if (tgCropRect.s.w != oldTgCrop.s.w || tgCropRect.s.h != oldTgCrop.s.h) {
      __fastLogger.FastLogD(
          "[%s] Tg crop changes from (%d, %d) %dx%d to (%d, %d) %dx%d for "
          "output size %dx%d",
          __FUNCTION__, oldTgCrop.p.x, oldTgCrop.p.y, oldTgCrop.s.w,
          oldTgCrop.s.h, tgCropRect.p.x, tgCropRect.p.y, tgCropRect.s.w,
          tgCropRect.s.h, outSize.w, outSize.h);
    }
  }

  __fastLogger.print();
}

bool StereoSizeProviderImp::getPass1ActiveArrayCrop(
      const int32_t SENSOR_INDEX, MRect &cropRect, bool isCropNeeded,
      float previewCropRatio) {
    ENUM_STEREO_SENSOR sensor = StereoSettingProvider::sensorIdToEnum(SENSOR_INDEX);
    if(sensor == eSTEREO_SENSOR_UNKNOWN) {
        return false;
    }

    return getPass1ActiveArrayCrop(sensor, cropRect, isCropNeeded, previewCropRatio);
}

bool
StereoSizeProviderImp::getPass1ActiveArrayCrop(ENUM_STEREO_SENSOR sensor, MRect &cropRect, bool isCropNeeded, float previewCropRatio)
{
    enum {
        SENSOR_BITS          = 4,   //Up to 16 sensors in a logical device
        NEED_CROP_BITS       = 1,
        ZOOM_RATIO_BITS      = 7,   // 0.xx->XX
    };

    // SENSOR_BITS
    SIZE_KEY_TYPE key = sensor;
    int shiftBits = SENSOR_BITS;

    // NEED_CROP_BIT
    key |= isCropNeeded << shiftBits;
    shiftBits += NEED_CROP_BITS;

    // Zoom Ratio
    int zoomRatio = static_cast<int>(previewCropRatio*100);
    key |= zoomRatio << shiftBits;
    shiftBits += ZOOM_RATIO_BITS;

    // Prepare sensor hal
    int32_t sensorId = StereoSettingProvider::sensorEnumToId(sensor);
    if(sensorId < 0) {
        return false;
    }

    STEREO_IMAGE_RATIO_T imageRatio = StereoSettingProvider::imageRatio();
    auto printSize = [&](bool isCached) {
        if(!__LOG_ENABLED) {
            return;
        }

        // For printing
        #ifdef __func__
        #undef __func__
        #endif
        #define __func__ "getPass1ActiveArrayCrop"
        MY_LOGD("Active domain for sensor %d: offset (%d, %d), size %dx%d, ratio %s, is crop %d, preview crop ratio %.2f(cache %d)",
                sensorId, cropRect.p.x, cropRect.p.y, cropRect.s.w, cropRect.s.h, (const char *)imageRatio, isCropNeeded,
                previewCropRatio, isCached);
        #undef __func__
    };

    {
        Mutex::Autolock lock(__activeCropLock);
        auto iter = __activeCropMap.find(key);
        if(iter != __activeCropMap.end()) {
            cropRect = iter->second;
            printSize(IS_CACHED);
            return true;
        }
    }

    if(cropRect.s == MSIZE_ZERO &&
       cropRect.p == MPOINT_ZERO)
    {
        // Prepare sensor static info
        auto pHalDeviceList = MAKE_HalLogicalDeviceList();
        if(pHalDeviceList == nullptr) {
            return false;
        }
        int sensorDevIndex = pHalDeviceList->querySensorDevIdx(sensorId);
        SensorStaticInfo sensorStaticInfo;
        memset(&sensorStaticInfo, 0, sizeof(SensorStaticInfo));
        pHalDeviceList->querySensorStaticInfo(sensorDevIndex, &sensorStaticInfo);

        cropRect.p = MPOINT_ZERO;
        cropRect.s = MSize(sensorStaticInfo.captureWidth, sensorStaticInfo.captureHeight);
    }

    if(isCropNeeded)
    {
        // Use preview to query since capture should align preview's setting
        if(StereoSettingProvider::is3rdParty(eSTEREO_SCENARIO_PREVIEW)) {
            CropUtil::cropRectByImageRatio(cropRect, imageRatio);
        } else {
            __cropRectByFOVCropAndImageRatio(sensor, imageRatio, previewCropRatio, cropRect);
        }
    }

    {
        Mutex::Autolock lock(__activeCropLock);
        __activeCropMap[key] = cropRect;
    }

    printSize(!IS_CACHED);
    return true;
}

bool
StereoSizeProviderImp::getPass2SizeInfo(ENUM_PASS2_ROUND round,
                                        ENUM_STEREO_SCENARIO eScenario,
                                        Pass2SizeInfo &pass2SizeInfo,
                                        bool withSlant,
                                        float previewCropRatio) const
{
    P2SizeQueryParam param(round, eScenario, withSlant, previewCropRatio);
    return getPass2SizeInfo(param, pass2SizeInfo);
}

bool
StereoSizeProviderImp::getPass2SizeInfo(P2SizeQueryParam param,
                                        StereoHAL::Pass2SizeInfo &pass2SizeInfo ) const
{
    bool isSuccess = true;

    param.withSlant = param.withSlant && StereoSettingProvider::isSlantCameraModule();

    switch(param.round) {
        case PASS2A:
            pass2SizeInfo = Pass2A_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_2:
            pass2SizeInfo = Pass2A_2_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_3:
            pass2SizeInfo = Pass2A_3_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_P:
            pass2SizeInfo = Pass2A_P_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_P_2:
            pass2SizeInfo = Pass2A_P_2_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_P_3:
            pass2SizeInfo = Pass2A_P_3_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_B:
            pass2SizeInfo = Pass2A_B_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_CROP:
            pass2SizeInfo = Pass2A_Crop_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_B_CROP:
            pass2SizeInfo = Pass2A_B_Crop_SizeProvider::instance()->sizeInfo(param);
            break;
        case PASS2A_P_CROP:
            pass2SizeInfo = Pass2A_P_Crop_SizeProvider::instance()->sizeInfo(param);
            break;
        default:
            isSuccess = false;
    }

    return isSuccess;
}

#if (1==HAS_AIDEPTH)
MSize
StereoSizeProviderImp::__getAIDepthModelSize(STEREO_IMAGE_RATIO_T ratio)
{
    MSize result(896, 512);
    auto iter = __aidepthModelSize.find(ratio);
    if(iter != __aidepthModelSize.end())
    {
        result = iter->second;
    }
    else
    {
        MTKAIDepth *pAIDepth = MTKAIDepth::createInstance(DRV_AIDEPTH_OBJ_SW);
        if(pAIDepth) {
            AIDepthModelInfo info;
#if 0
            if(StereoHAL::isVertical(StereoSettingProvider::getModuleRotation()))
            {
                if(eStereoRatio_4_3 == StereoSettingProvider::imageRatio())
                {
                    pAIDepth->AIDepthGetModelInfo(AIDEPTH_FEATURE_3_4, (void *)&info);
                }
                else
                {
                    pAIDepth->AIDepthGetModelInfo(AIDEPTH_FEATURE_9_16, (void *)&info);
                }
                result.w = info.modelHeight;
                result.h = info.modelWidth;
            }
            else
            {
                if(eStereoRatio_4_3 == StereoSettingProvider::imageRatio())
                {
                    pAIDepth->AIDepthGetModelInfo(AIDEPTH_FEATURE_4_3, (void *)&info);
                }
                else
                {
                    pAIDepth->AIDepthGetModelInfo(AIDEPTH_FEATURE_16_9, (void *)&info);
                }
                result.w = info.modelWidth;
                result.h = info.modelHeight;
            }
#else
            pAIDepth->AIDepthGetModelInfo(AIDEPTH_FEATURE_9_16, (void *)&info);
            result.w = info.modelHeight;
            result.h = info.modelWidth;
#endif
            pAIDepth->AIDepthReset();
            pAIDepth->destroyInstance(pAIDepth);
            pAIDepth = NULL;

            __aidepthModelSize.emplace(ratio, result);
        }
    }

    return result;
}
#endif  //#if (1==HAS_AIDEPTH)

#if (1==HAS_VAIDEPTH)
MSize
StereoSizeProviderImp::__getVideoAIDepthModelSize(STEREO_IMAGE_RATIO_T ratio)
{
    MSize result(512, 288);
    auto iter = __vaidepthModelSize.find(ratio);
    if(iter != __vaidepthModelSize.end())
    {
        result = iter->second;
    }
    else
    {
        MTKVideoAIDepth *pAIDepth = MTKVideoAIDepth::createInstance(DRV_VIDEOAIDEPTH_OBJ_SW);
        if(pAIDepth) {
            VideoAIDepthModelInfo info;
            MUINT32 queryType = VIDEOAIDEPTH_FEATURE_16_9;
            if(StereoHAL::isVertical(StereoSettingProvider::getModuleRotation())) {
                queryType = VIDEOAIDEPTH_FEATURE_9_16;
            }
            // else
            // {
            //     if(ratio == eStereoRatio_4_3) {
            //         queryType = VIDEOAIDEPTH_FEATURE_4_3;
            //     } else {
            //         queryType = VIDEOAIDEPTH_FEATURE_16_9;
            //     }
            // }
            pAIDepth->VideoAIDepthGetModelInfo(queryType, (void *)&info);
            result.w = info.modelWidth;
            result.h = info.modelHeight;

            pAIDepth->VideoAIDepthReset();
            pAIDepth->destroyInstance(pAIDepth);
            pAIDepth = NULL;

            __vaidepthModelSize.emplace(ratio, result);
        }
    }

    MY_LOGD("VAIDepth model size %dx%d", result.w, result.h);
    return result;
}

bool
StereoSizeProviderImp::__useVAIDepthSize(ENUM_STEREO_SCENARIO eScenario)
{
    if(StereoSettingProvider::isMTKDepthmapMode()) {
        auto refineLevel = StereoSettingProvider::getDepthmapRefineLevel(eScenario);
        if (E_DEPTHMAP_REFINE_AI_DEPTH == refineLevel) {
            return true;
        } else if (E_DEPTHMAP_REFINE_NONE == refineLevel ||
                   E_DEPTHMAP_REFINE_HOLE_FILLED == refineLevel)
        {
            return false;
        }
    }

    return (eScenario == eSTEREO_SCENARIO_RECORD);
}

#else

bool
StereoSizeProviderImp::__useVAIDepthSize(ENUM_STEREO_SCENARIO eScenario __attribute__((unused)))
{
    return false;
}

#endif  //#if (1==HAS_AIDEPTHV)

StereoArea
StereoSizeProviderImp::getBufferSize(ENUM_BUFFER_NAME eName,
                                     ENUM_STEREO_SCENARIO eScenario,
                                     bool withSlantRotation,
                                     float previewCropRatio,
                                     int capOrientation)
{
    return getBufferSize({eName, eScenario, withSlantRotation, previewCropRatio, capOrientation});
}

StereoSizeProvider::SIZE_KEY_TYPE
StereoSizeProvider::BufferSizeQueryParam::getHashKey() const
{
    enum {
        BUFFER_NAME_BITS     = 8,
        SCENARIO_BITS        = 2,
        ZOOM_RATIO_BITS      = 7,   // 0.xx->XX
        SLANT_ROTATION_BITS  = 1,
        CAP_ORIENTATIN_BITS  = 2,   //1: 90, 2: 180, 3: 270
    };

    // SENSOR_BITS
    SIZE_KEY_TYPE key = eName;
    int shiftBits = BUFFER_NAME_BITS;

    // SCENARIO_BITS
    key |= eScenario<<shiftBits;
    shiftBits += SCENARIO_BITS;

    // ZOOM_RATIO_BITS
    int zoomRatio = static_cast<int>(previewCropRatio*100);
    key |= zoomRatio<<shiftBits;
    shiftBits += ZOOM_RATIO_BITS;

    // SLANT_ROTATION_BITS
    key |= withSlantRotation<<shiftBits;
    shiftBits += SLANT_ROTATION_BITS;

    //put this line at last
    if(eSTEREO_SCENARIO_CAPTURE == eScenario)
    {
        switch(capOrientation)
        {
            case 0:
            default:
                break;
            case 90:
                key |= 1 << shiftBits;
                break;
            case 270:
                key |= 3 << shiftBits;
                break;
            case 180:
                key |= 2 << shiftBits;
                break;
        }
    }

    return key;
}

std::string
StereoSizeProvider::BufferSizeQueryParam::toString() const
{
    std::ostringstream oss;
    oss << "Buffer " << STEREO_BUFFER_NAMES[static_cast<size_t>(eName)]
        << ", scenario " << eScenario
        << ", capOrientation" << capOrientation
        << ", preview crop ratio " << previewCropRatio
        << ", w slant " << withSlantRotation
        << ", image ratio " << static_cast<std::string>(imageRatio);

    oss << ", feature(" << featureMode << "): ";
    std::vector<std::string> features = StereoHAL::featureModeToStrings(featureMode);
    std::copy(features.begin(), features.end()-1, std::ostream_iterator<std::string>(oss, "+"));
    oss << features.back();

    if(!useCacheResult) {
        oss << ", not to cache";
    }

    return oss.str();
}

StereoArea
StereoSizeProviderImp::getBufferSize(BufferSizeQueryParam param)
{
    SIZE_KEY_TYPE key = param.getHashKey();
    StereoArea result;

    bool isCached = false;
    if(param.useCacheResult)
    {
        {
            Mutex::Autolock lock(__bufferSizeLock);
            auto iter = __bufferSizeMap.find(key);
            if(iter != __bufferSizeMap.end()) {
                isCached = true;
                result = iter->second;
            }
        }

        if(!isCached) {
            result = __getBufferSize(param);
            Mutex::Autolock lock(__bufferSizeLock);
            __bufferSizeMap.emplace(key, result);
        }
    }
    else
    {
        result = __getBufferSize(param);
    }

    if(__LOG_ENABLED) {
        auto keyString = param.toString();
        MY_LOGD("%s: size %dx%d, padding %dx%d, startPt (%d, %d)(%s)",
                keyString.c_str(),
                result.size.w, result.size.h,
                result.padding.w, result.padding.h,
                result.startPt.x, result.startPt.y,
                (isCached) ? "Cached" : "New");
    }

    return result;
}

void
StereoSizeProviderImp::setCaptureImageSize(int w, int h)
{
    __captureSize.w = w;
    __captureSize.h = h;
    MY_LOGD("Set capture size %dx%d", w, h);
}

bool
StereoSizeProviderImp::getcustomYUVSize( ENUM_STEREO_SENSOR sensor,
                                         EPortIndex port,
                                         MSize &outSize
                                       )
{
    outSize = MSIZE_ZERO;
    int32_t sensorID = StereoSettingProvider::sensorEnumToId(sensor);
    if( EPortIndex_IMGO == port)
    {
        outSize = [&] {
            auto iter = __imgoYuvSizes.find(sensorID);
            if(iter != __imgoYuvSizes.end())
            {
                return iter->second;
            }

            Pass2SizeInfo p2SizeInfo;
            ENUM_PASS2_ROUND queryRound = (eSTEREO_SENSOR_MAIN1 == sensor) ? PASS2A_CROP : PASS2A_P_CROP;
            getPass2SizeInfo(queryRound, eSTEREO_SCENARIO_CAPTURE, p2SizeInfo);
            return p2SizeInfo.areaWDMA.contentSize();
        }();

        //apply 4 align for MFLL
        StereoHAL::applyNAlign(4, outSize.w);
        StereoHAL::applyNAlign(4, outSize.h);
    }
    else
    {
        //Use preview to query since RRZO is for preview
        if(StereoSettingProvider::is3rdParty(eSTEREO_SCENARIO_PREVIEW)) {
            auto iter = __rrzoYuvSizes.find(sensorID);
            if(iter != __rrzoYuvSizes.end())
            {
                outSize = iter->second;
            } else if(sensor == eSTEREO_SENSOR_MAIN1) {
                outSize = getPreviewSize();
            } else {
                Pass2SizeInfo p2SizeInfo;
                if(sensor == eSTEREO_SENSOR_MAIN2)
                {
                    getPass2SizeInfo(PASS2A_P_CROP, eSTEREO_SCENARIO_CAPTURE, p2SizeInfo);
                    outSize = getPreviewSize();
                }
                else
                {
                    getcustomYUVSize(sensor, EPortIndex_IMGO, outSize);
                }

                outSize.w /= 2;
                outSize.h /= 2;
            }

            StereoHAL::applyNAlign(2, outSize.w);
            StereoHAL::applyNAlign(2, outSize.h);
        } else {
            MUINT32 junkStride;
            MRect   tgCropRect;
            ENUM_STEREO_SCENARIO scenario = (StereoSettingProvider::isRecording())
                                            ? eSTEREO_SCENARIO_RECORD
                                            : eSTEREO_SCENARIO_PREVIEW;

            getPass1Size(sensor, eImgFmt_FG_BAYER10, EPortIndex_RRZO, scenario,
                         //below are outputs
                         tgCropRect, outSize, junkStride);
        }
    }

    return true;
}

MSize
StereoSizeProviderImp::thirdPartyDepthmapSize(STEREO_IMAGE_RATIO_T ratio, ENUM_STEREO_SCENARIO senario) const
{
    StereoSensorConbinationSetting_T *sensorCombination = StereoSettingProviderKernel::getInstance()->getSensorCombinationSetting();
    MSize depthmapSize;
    if(eSTEREO_SCENARIO_PREVIEW == senario) {
        if(sensorCombination) {
            if(sensorCombination->depthmapSize.find(ratio) != sensorCombination->depthmapSize.end()) {
                depthmapSize = sensorCombination->depthmapSize[ratio];
            }
        }

        int m, n;
        ratio.MToN(m, n);
        if(0 == depthmapSize.w ||
           0 == depthmapSize.h)
        {
            StereoArea area(480, 360);
            if(m * 3 / 4 >= n) {
                area.applyRatio(ratio, E_KEEP_WIDTH);
            } else {
                area.applyRatio(ratio, E_KEEP_HEIGHT);
            }
            depthmapSize = area.size;
        }

        MY_LOGD("3rd party preview depthmap size for %s: %dx%d", (const char *)ratio, depthmapSize.w, depthmapSize.h);
    } else {
        if(sensorCombination) {
            if(sensorCombination->depthmapSizeCapture.find(ratio) != sensorCombination->depthmapSizeCapture.end()) {
                depthmapSize = sensorCombination->depthmapSizeCapture[ratio];
            }
        }

        if(0 == depthmapSize.w ||
           0 == depthmapSize.h)
        {
            Pass2SizeInfo p2SizeInfo;
            getPass2SizeInfo(PASS2A_CROP, eSTEREO_SCENARIO_CAPTURE, p2SizeInfo);
            p2SizeInfo.areaWDMA /= 4;
            p2SizeInfo.areaWDMA.removePadding().apply2Align();
            depthmapSize = p2SizeInfo.areaWDMA.contentSize();
        }

        MY_LOGD("3rd party capture depthmap size for %s: %dx%d", (const char *)ratio, depthmapSize.w, depthmapSize.h);
    }

    return depthmapSize;
}

void
StereoSizeProviderImp::setPreviewSize(NSCam::MSize size)
{
    __previewSize = size;
}

bool
StereoSizeProviderImp::
getDualcamP2IMGOYuvCropResizeInfo(const int SENSOR_INDEX, NSCam::MRect &cropRect, MSize &targetSize)
{
    int sensorIndex[2] = {0};
    bool isOK = StereoSettingProvider::getStereoSensorIndex(sensorIndex[0], sensorIndex[1]);
    if(!isOK) {
        return false;
    }

    ENUM_PASS2_ROUND queryRound = PASS2A_CROP;
    ENUM_STEREO_SENSOR sensor;
    if(SENSOR_INDEX == sensorIndex[0]) {
        queryRound = PASS2A_CROP;
        sensor = eSTEREO_SENSOR_MAIN1;
    } else if(SENSOR_INDEX == sensorIndex[1]) {
        queryRound = PASS2A_P_CROP;
        sensor = eSTEREO_SENSOR_MAIN2;
    } else {
        MY_LOGE("Invalid sensor index %d. Available sensor indexes: %d, %d", SENSOR_INDEX, sensorIndex[0], sensorIndex[1]);
        return false;
    }

    //Get Crop
    Pass2SizeInfo p2SizeInfo;
    getPass2SizeInfo(queryRound, eSTEREO_SCENARIO_CAPTURE, p2SizeInfo);
    cropRect.s = p2SizeInfo.areaWDMA.contentSize();
    cropRect.p = p2SizeInfo.areaWDMA.startPt;

    //Get size
    getcustomYUVSize(sensor, EPortIndex_IMGO, targetSize);

    MY_LOGD_IF(__LOG_ENABLED, "Crop rect: (%d, %d) %dx%d, Size %dx%d",
                            cropRect.p.x, cropRect.p.y, cropRect.s.w, cropRect.s.h,
                            targetSize.w, targetSize.h);

    return true;
}

/*******************************************************************************
 *
 ********************************************************************************/
std::vector<NSCam::MSize>
StereoSizeProviderImp::getMtkDepthmapSizes(std::vector<NSCam::MSize> previewSizes)
{
    StereoSizeConfig sizeConfig(STEREO_AREA_WO_PADDING, STEREO_AREA_W_ROTATE);
    sizeConfig.featureMode = E_STEREO_FEATURE_MTK_DEPTHMAP;
    sizeConfig.refineLevel = E_DEPTHMAP_REFINE_NONE;
    bool isHIDL = StereoSettingProvider::isHIDL();

    std::unordered_map<STEREO_IMAGE_RATIO_T, MSize> depthmapSizes;
    STEREO_IMAGE_RATIO_T ratio;
    MSize size;
    BufferSizeQueryParam bufferParam(
        E_DEPTH_MAP,
        eSTEREO_SCENARIO_PREVIEW,
        StereoSettingProvider::isSlantCameraModule(),
        1.0f,
        0,
        ratio, //will update later in for loop
        E_STEREO_FEATURE_MTK_DEPTHMAP
    );
    bufferParam.useCacheResult = false;

    for(auto &pvSize : previewSizes)
    {
        if(pvSize.w < 480)
        {
            MY_LOGD("Ignore small YUV size %dx%d", pvSize.w, pvSize.h);
            continue;
        }

        ratio = StereoHAL::STEREO_IMAGE_RATIO_T(pvSize);
        if(depthmapSizes.find(ratio) == depthmapSizes.end())
        {
            if(!isHIDL) {
                sizeConfig.imageRatio  = ratio;
                sizeConfig.previewSize = pvSize;
                size = StereoSize::getStereoArea1x(sizeConfig);

                if(size.h > size.w)
                {
                    std::swap(size.w, size.h);
                }
            } else {
                // For HIDL flow, we'll return rotated NOC map for 3rd party
                bufferParam.imageRatio = ratio;
                size = StereoSizeProvider::getInstance()->getBufferSize(bufferParam);
            }

            depthmapSizes.emplace(ratio, size);

            MY_LOGD("Add cam %d depthmap size %dx%d for %s(%dx%d)",
                    StereoSettingProvider::getLogicalDeviceID(),
                    size.w, size.h, (const char *)ratio, pvSize.w, pvSize.h);
        }
    }

    std::vector<NSCam::MSize> result;
    for(auto &n : depthmapSizes)
    {
        result.push_back(n.second);
    }
    return result;
}

/*******************************************************************************
 *
 ********************************************************************************/
bool
StereoSizeProviderImp::__getCenterCrop(MSize &srcSize, MRect &rCrop )
{
    // calculate the required image hight according to image ratio
    int iHeight = srcSize.h;
    int m, n;
    StereoSettingProvider::imageRatio().MToN(m, n);
    if(eStereoRatio_4_3 != StereoSettingProvider::imageRatio()) {
        iHeight = srcSize.w * n / m;
        applyNAlign(2, iHeight);
    }

    if(abs(iHeight-srcSize.h) == 0)
    {
        rCrop.p = MPoint(0,0);
        rCrop.s = srcSize;
    }
    else
    {
        rCrop.p.x = 0;
        rCrop.p.y = (srcSize.h - iHeight)/2;
        rCrop.s.w = srcSize.w;
        rCrop.s.h = iHeight;
    }

    MY_LOGD_IF(__LOG_ENABLED, "srcSize:(%d,%d) ratio:%s, rCropStartPt:(%d, %d) rCropSize:(%d,%d)",
                            srcSize.w, srcSize.h, (const char *)StereoSettingProvider::imageRatio(),
                            rCrop.p.x, rCrop.p.y, rCrop.s.w, rCrop.s.h);

    // apply 16-align to height
    if(eStereoRatio_4_3 != StereoSettingProvider::imageRatio()) {
        applyNAlign(16, rCrop.s.h);
        MY_LOGD_IF(__LOG_ENABLED, "srcSize after 16 align:(%d,%d) ratio:%s, rCropStartPt:(%d, %d) rCropSize:(%d,%d)",
                                srcSize.w, srcSize.h, (const char *)StereoSettingProvider::imageRatio(),
                                rCrop.p.x, rCrop.p.y, rCrop.s.w, rCrop.s.h);
    }

    return MTRUE;
}

float
StereoSizeProviderImp::__getFOVCropRatio(ENUM_STEREO_SENSOR sensor, float previewCropRatio)
{
    switch(sensor)
    {
        case eSTEREO_SENSOR_MAIN1:
            return StereoSettingProvider::getMain1FOVCropRatio(previewCropRatio);
        case eSTEREO_SENSOR_MAIN2:
            return StereoSettingProvider::getMain2FOVCropRatio(previewCropRatio);
        default:
            return 1.0f;
    }
};

void
StereoSizeProviderImp::__cropRectByFOVCropAndImageRatio(ENUM_STEREO_SENSOR sensor, STEREO_IMAGE_RATIO_T imageRatio, float previewCropRatio, MRect &rect)
{
    float CROP_RATIO = __getFOVCropRatio(sensor, previewCropRatio);
    if(CROP_RATIO < 1.0f &&
       CROP_RATIO > 0.0f)
    {
        CropUtil::cropRectByFOV(sensor, rect, CROP_RATIO);
        MY_LOGD_IF(__LOG_ENABLED, "Crop TG by FOV ratio %.2f, result: tg (%d, %d) %dx%d",
                                CROP_RATIO, rect.p.x, rect.p.y, rect.s.w, rect.s.h);
    }

    CropUtil::cropRectByImageRatio(rect, imageRatio);
    MY_LOGD_IF(__LOG_ENABLED, "Crop TG by image ratio %s, result: offset (%d, %d) size %dx%d",
                            (const char *)imageRatio, rect.p.x, rect.p.y, rect.s.w, rect.s.h);
}

void StereoSizeProviderImp::__getTGCrop(ENUM_STEREO_SCENARIO scenario,
                                        ENUM_STEREO_SENSOR sensor,
                                        STEREO_IMAGE_RATIO_T imageRatio,
                                        float previewCropRatio,
                                        MRect &tgCropRect) {
  if (StereoSettingProvider::is3rdParty(scenario)) {
    CropUtil::cropRectByImageRatio(tgCropRect, imageRatio);
  } else {
    if (previewCropRatio < 1.0f) {
      float minRatio = StereoSettingProvider::getVSDoFMinPass1CropRatio();
      // If ratio is under minRatio, use the result of minRatio
      if (previewCropRatio < minRatio) {
        previewCropRatio = minRatio;
      }
    }
    __cropRectByFOVCropAndImageRatio(sensor, imageRatio, previewCropRatio,
                                     tgCropRect);
  }
}
