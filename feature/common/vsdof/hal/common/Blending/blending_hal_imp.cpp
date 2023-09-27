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
#include "blending_hal_imp.h"
#include <stereo_tuning_provider.h>
#include <mtkcam/utils/std/ULog.h>
#include <../inc/stereo_dp_util.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_VSDOF_HAL);

inline bool checkImageBuffer(IImageBuffer *image, const char *bufferName)
{
    if(image == nullptr) {
        MY_LOGE("%s is NULL", bufferName);
        return false;
    }

    size_t plaeCount = image->getPlaneCount();
    for(int p = 0; p < plaeCount; ++p) {
        if(image->getBufVA(p) == 0) {
            MY_LOGE("%s, plane %d is NULL", bufferName, p);
            return false;
        }
    }

    return true;
}

BLENDING_HAL *
BLENDING_HAL::createInstance(ENUM_STEREO_SCENARIO eScenario)
{
    return new BLENDING_HAL_IMP(eScenario);
}

BLENDING_HAL_IMP::BLENDING_HAL_IMP(ENUM_STEREO_SCENARIO eScenario)
    : __eScenario(eScenario)
    , LOG_ENABLED(StereoSettingProvider::isLogEnabled(LOG_PERPERTY))
    , DMBG_SIZE(StereoSizeProvider::getInstance()->getBufferSize(E_DMBG, eScenario))
    , DMBG_IMG_SIZE(DMBG_SIZE.w*DMBG_SIZE.h)
    , __fastLogger(LOG_TAG, LOG_PERPERTY)
{
    __fastLogger.setSingleLineMode(SINGLE_LINE_LOG);

    __pBlendingDrv = MTKBokehBlend::createInstance(DRV_BOKEH_BLEND_OBJ_SW);
    CAM_ULOG_ASSERT(NSCam::Utils::ULog::MOD_VSDOF_HAL, __pBlendingDrv != nullptr, "Blending instance is NULL!");

    //Init Blending
    ::memset(&__initInfo, 0, sizeof(__initInfo));
    ::memset(&__procInfo, 0, sizeof(__procInfo));

    //Init tuning
    //=== Init tuning info ===
    __initInfo.pTuningInfo = new BokehBlendTuningInfo();
    std::vector<std::pair<std::string, int>> tuningParamsList;
    StereoTuningProvider::getBlendingTuningInfo(__initInfo.pTuningInfo->coreNumber, __alphaTable, tuningParamsList, eScenario);
    __initInfo.pTuningInfo->alphaTblSize = __alphaTable.size();
    if(__initInfo.pTuningInfo->alphaTblSize > 0)
    {
        __initInfo.pTuningInfo->alphaTbl = &__alphaTable[0];
    }
#ifdef BOKEHBLEND_CUSTOM_PARAM
    for(auto &param : tuningParamsList) {
        __tuningParams.push_back({(char *)param.first.c_str(), param.second});
    }

    __initInfo.pTuningInfo->NumOfParam = __tuningParams.size();
    __initInfo.pTuningInfo->params = &__tuningParams[0];
#endif

    //=== Init sizes ===
    __initInfo.BlurWidth  = DMBG_SIZE.w;
    __initInfo.BlurHeight = DMBG_SIZE.h;

    // Get RRZO sizes to calculate padding
    MUINT32 junkStride;
    MSize   szMain1RRZO;
    MSize   szMain2RRZO;
    MRect   tgCropRect;

    StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN1,
                                                       eImgFmt_FG_BAYER10,
                                                       EPortIndex_RRZO,
                                                       __eScenario,
                                                       1.0f
                                                     },
                                                     //below are outputs
                                                     tgCropRect,
                                                     szMain1RRZO,
                                                     junkStride);

    StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN2,
                                                       eImgFmt_FG_BAYER10,
                                                       EPortIndex_RRZO,
                                                       __eScenario,
                                                       1.0f
                                                     },
                                                     //below are outputs
                                                     tgCropRect,
                                                     szMain2RRZO,
                                                     junkStride);

    __imageArea = szMain1RRZO;
    if(szMain2RRZO.w > szMain1RRZO.w ||
       szMain2RRZO.h > szMain1RRZO.h)
    {
        __imageArea = szMain2RRZO;
        __imageArea.padding.w = szMain2RRZO.w - szMain1RRZO.w;
        __imageArea.padding.h = szMain2RRZO.h - szMain1RRZO.h;
    }
    __imageContentSize = __imageArea.contentSize();

    __init();
};

BLENDING_HAL_IMP::~BLENDING_HAL_IMP()
{
    //Delete working buffer
    if(__initInfo.pTuningInfo) {
        delete __initInfo.pTuningInfo;
        __initInfo.pTuningInfo = NULL;
    }

    if(__workingBufferImg.get()) {
        StereoDpUtil::freeImageBuffer(LOG_TAG, __workingBufferImg);
    }

    if(__pBlendingDrv) {
        __pBlendingDrv->BokehBlendReset();
        __pBlendingDrv->destroyInstance(__pBlendingDrv);
        __pBlendingDrv = NULL;
    }
}

bool
BLENDING_HAL_IMP::BlendingHALRun(BLENDING_HAL_IN_DATA &inData, BLENDING_HAL_OUT_DATA &outData)
{
    __dumpHint      = inData.dumpHint;
    __dumpLevel     = inData.dumpLevel;
    __magicNumber   = inData.magicNumber;
    __requestNumber = inData.requestNumber;

    __dumpInitInfo();

    AutoProfileUtil profile(LOG_TAG, "BlendingHALRun");
    return __setBlendingParams(inData, outData) &&
           __runBlending(outData);
}

bool
BLENDING_HAL_IMP::__init()
{
    __initInfo.Width_Pad  = __imageArea.size.w;
    __initInfo.Height_Pad = __imageArea.size.h;
    __initInfo.Width      = __imageContentSize.w;
    __initInfo.Height     = __imageContentSize.h;

    if(__pBlendingDrv == nullptr)
    {
        MY_LOGE("Blending is null");
        return false;
    }

    __pBlendingDrv->BokehBlendReset();
    if(S_BOKEH_BLEND_OK != __pBlendingDrv->BokehBlendInit((void *)&__initInfo) )
    {
        MY_LOGE("Init fail");
        return false;
    }

    //Get working buffer size
    if(S_BOKEH_BLEND_OK !=  __pBlendingDrv->BokehBlendFeatureCtrl(BOKEH_BLEND_FEATURE_GET_WORKBUF_SIZE, NULL, &__initInfo.workingBuffSize))
    {
        MY_LOGE("Get working buffer size fail");
        return false;
    }

    //Allocate working buffer and set to Blending
    if(__initInfo.workingBuffSize > 0) {
        if(StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_Y8, MSize(__initInfo.workingBuffSize, 1), !IS_ALLOC_GB, __workingBufferImg)) {
            MY_LOGD_IF(LOG_ENABLED, "Alloc %d bytes for working buffer, addr %p",
                       __initInfo.workingBuffSize, __workingBufferImg.get());

            __initInfo.workingBuffAddr = (MUINT8*)__workingBufferImg->getBufVA(0);
            if(S_BOKEH_BLEND_OK != __pBlendingDrv->BokehBlendFeatureCtrl(BOKEH_BLEND_FEATURE_SET_WORKBUF_ADDR, &__initInfo, NULL))
            {
                MY_LOGE("Set working buffer fail");
                return false;
            }
        }
        else
        {
            MY_LOGE("Cannot create working buffer of size %d", __initInfo.workingBuffSize);
            return false;
        }
    }

    __logInitData();

    return true;
}

bool
BLENDING_HAL_IMP::__setBlendingParams(BLENDING_HAL_IN_DATA &param, BLENDING_HAL_OUT_DATA &output)
{
    AutoProfileUtil profile(LOG_TAG, "Set Proc");

    if( __pBlendingDrv == nullptr ||
        !checkImageBuffer(param.cleanImage,  "Clear Image") ||
        !checkImageBuffer(param.bokehImage,  "Bokeh Image") ||
        !checkImageBuffer(param.blurMap,     "Blue Map")    ||
        !checkImageBuffer(output.bokehImage, "Blending Image")
      )
    {
        return false;
    }

    __procInfo.numOfBuffer = 3; //clear, bokeh, blurMap

    auto ___setBlendingImage = [&] ( IImageBuffer *image,
                                     BOKEH_BLEND_BUFFER_TYPE_ENUM type,
                                     int index
                                   )
    {
        if(image == NULL) {
            return;
        }

        auto &buffer = __procInfo.bufferInfo[index];
        buffer.type = type;
        buffer.format = [&]
        {
            int format = image->getImgFormat();
            switch(format) {
                case eImgFmt_Y8:
                case eImgFmt_STA_BYTE:
                    return BOKEH_BLEND_IMAGE_YONLY;
                    break;
                case eImgFmt_YV12:
                    return BOKEH_BLEND_IMAGE_YV12;
                    break;
                case eImgFmt_YUY2:
                    return BOKEH_BLEND_IMAGE_YUY2;
                    break;
                case eImgFmt_NV12:
                    return BOKEH_BLEND_IMAGE_NV12;
                    break;
                case eImgFmt_NV21:
                    return BOKEH_BLEND_IMAGE_NV21;
                    break;
                case eImgFmt_RGB888:
                    return BOKEH_BLEND_IMAGE_RGB888;
                    break;
                case eImgFmt_I420:
                    return BOKEH_BLEND_IMAGE_I420;
                    break;
                case eImgFmt_I422:
                    return BOKEH_BLEND_IMAGE_I422;
                // case eImgFmt_YUV444:
                //     return BOKEH_BLEND_IMAGE_YUV444;
                default:
                    MY_LOGW("Unsupported format: %d", format);
                    return BOKEH_BLEND_IMAGE_YONLY;
            }
        }();

        buffer.width  = image->getImgSize().w;
        buffer.height = image->getImgSize().h;
        buffer.width_pad  = buffer.width;
        buffer.height_pad = buffer.height;
        buffer.stride = image->getBufStridesInBytes(0);
        buffer.BitNum = image->getPlaneBitsPerPixel(0);

        if(type != BOKEH_BLEND_BUFFER_TYPE_BLUR)
        {
            buffer.width_pad  = __imageArea.size.w;
            buffer.height_pad = __imageArea.size.h;
            buffer.width      = __imageContentSize.w;
            buffer.height     = __imageContentSize.h;
        }

        size_t planCount = image->getPlaneCount();
        buffer.planeAddr0 = (void *)image->getBufVA(0);
        buffer.planeAddr1 = (planCount > 1) ? (void *)image->getBufVA(1) : NULL;
        buffer.planeAddr2 = (planCount > 2) ? (void *)image->getBufVA(2) : NULL;
    };

    int imageIndex = 0;
    ___setBlendingImage(param.cleanImage,  BOKEH_BLEND_BUFFER_TYPE_CLEAR, imageIndex++);
    ___setBlendingImage(param.bokehImage,  BOKEH_BLEND_BUFFER_TYPE_BOKEH, imageIndex++);
    ___setBlendingImage(param.blurMap,     BOKEH_BLEND_BUFFER_TYPE_BLUR,  imageIndex++);
    ___setBlendingImage(output.bokehImage, BOKEH_BLEND_BUFFER_TYPE_BLEND, imageIndex++);
    __procInfo.numOfBuffer = imageIndex;

    __logSetProcData();
    __dumpProcInfo();

    if( S_BOKEH_BLEND_OK != __pBlendingDrv->BokehBlendFeatureCtrl(BOKEH_BLEND_FEATURE_SET_PROC_INFO, &__procInfo, NULL))
    {
        MY_LOGE("Blending SET PROC failed");
        return false;
    }

    return true;
}

bool
BLENDING_HAL_IMP::__runBlending(BLENDING_HAL_OUT_DATA &output)
{
    if(__pBlendingDrv == nullptr)
    {
        MY_LOGE("Blending is null");
        return false;
    }

    AutoProfileUtil profile(LOG_TAG, "Run Bleding");

    //================================
    //  Run Blending
    //================================
    MRESULT result = __pBlendingDrv->BokehBlendMain();
    if(S_BOKEH_BLEND_OK != result)
    {
        MY_LOGE("Blending failed");
        return false;
    }

    __dumpWorkingBuffer();

    return true;
}
