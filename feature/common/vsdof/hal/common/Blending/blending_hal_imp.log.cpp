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

void
BLENDING_HAL_IMP::__logInitData()
{
    if(!LOG_ENABLED) {
        return;
    }

    FAST_LOGD("========= BLENDING Init Info =========");
    FAST_LOGD("[Width]          %d", __initInfo.Width_Pad);
    FAST_LOGD("[Height]         %d", __initInfo.Height_Pad);
    FAST_LOGD("[Width_Pad]      %d", __initInfo.Width);
    FAST_LOGD("[Height_Pad]     %d", __initInfo.Height);
    //__initInfo
    if(__initInfo.pTuningInfo->alphaTblSize > 0)
    {
        std::ostringstream oss;
        for(int i = 0; i < __initInfo.pTuningInfo->alphaTblSize; ++i) {
            oss << (int)__initInfo.pTuningInfo->alphaTbl[i] << " ";
        }
        FAST_LOGD("[Tuning Table](%d) %s", __initInfo.pTuningInfo->alphaTblSize, oss.str().c_str());
    }

#ifdef BOKEHBLEND_CUSTOM_PARAM
    FAST_LOGD("[TuningInfo.NumOfParam]    %d", __initInfo.pTuningInfo->NumOfParam);
    for(MUINT32 j = 0; j < __initInfo.pTuningInfo->NumOfParam; ++j) {
        FAST_LOGD("[TuningInfo.params][%d]     %s: %d", j,
                  __initInfo.pTuningInfo->params[j].key,
                  __initInfo.pTuningInfo->params[j].value);
    }
#endif

    FAST_LOG_PRINT;
}

void
BLENDING_HAL_IMP::__logSetProcData()
{
    if(!LOG_ENABLED) {
        return;
    }

    FAST_LOGD("========= Blending Proc Data =========");
    for(int i = 0;i < __procInfo.numOfBuffer; ++i) {
        __logBufferInfo(__procInfo.bufferInfo[i]);
    }

    FAST_LOG_PRINT;
}

void
BLENDING_HAL_IMP::__logBufferInfo(BokehBlendBufferInfo &buffer)
{
    FAST_LOGD("[Buffer]         %s(type %d)",
              [&]()
              {
                  switch(buffer.type)
                  {
                      case BOKEH_BLEND_BUFFER_TYPE_BLUR:
                          return "Blur map";
                          break;
                      case BOKEH_BLEND_BUFFER_TYPE_CLEAR:
                          return "Clear image";
                          break;
                      case BOKEH_BLEND_BUFFER_TYPE_BOKEH:
                          return "Bokeh Image";
                          break;
                      case BOKEH_BLEND_BUFFER_TYPE_BLEND:
                          return "Blending Image";
                          break;
                      default:
                          return "Unknown";
                  }
              }(),
              buffer.type);
    FAST_LOGD("[Format]         %s",
              [&]()
              {
                  switch(buffer.format)
                  {
                      case BOKEH_BLEND_IMAGE_YV12:
                          return "YV12";
                          break;
                      case BOKEH_BLEND_IMAGE_NV12:
                          return "NV12";
                          break;
                      case BOKEH_BLEND_IMAGE_NV21:
                          return "NV21";
                          break;
                      case BOKEH_BLEND_IMAGE_YONLY:
                          return "Y8";
                          break;
                      case BOKEH_BLEND_IMAGE_YUY2:
                          return "YUY2";
                          break;
                      case BOKEH_BLEND_IMAGE_RGB888:
                          return "RGB888";
                          break;
                      case BOKEH_BLEND_IMAGE_I420:
                          return "I420";
                          break;
                      case BOKEH_BLEND_IMAGE_I422:
                          return "I422";
                          break;
                      case BOKEH_BLEND_IMAGE_YUV444:
                          return "YUV444";
                          break;
                      default:
                          return "Unknown";
                  }
              }());

    FAST_LOGD("[Width]          %d", buffer.width);
    FAST_LOGD("[Height]         %d", buffer.height);
    FAST_LOGD("[Width_Pad]      %d", buffer.width_pad);
    FAST_LOGD("[Height_Pad]     %d", buffer.height_pad);
    FAST_LOGD("[Stride]         %d", buffer.stride);
    FAST_LOGD("[BitNum]         %d", buffer.BitNum);
    FAST_LOGD("[Plane Address]  %p %p %p", buffer.planeAddr0, buffer.planeAddr1, buffer.planeAddr2);
}