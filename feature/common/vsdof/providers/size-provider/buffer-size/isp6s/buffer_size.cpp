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
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>

#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include "../../stereo_size_provider_imp.h"
#include <vsdof/hal/ProfileUtil.h>
#include <stereo_tuning_provider.h>
#include "base_size.h"
#include "../../pass2/pass2A_size_provider.h"

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

StereoArea
StereoSizeProviderImp::__getBufferSize(BufferSizeQueryParam param)
{
    const StereoSizeConfig CONFIG_WO_PADDING_WO_ROTATE(STEREO_AREA_WO_PADDING, STEREO_AREA_WO_ROTATE);
    const StereoSizeConfig CONFIG_WO_PADDING_W_ROTATE(STEREO_AREA_WO_PADDING, STEREO_AREA_W_ROTATE);
    const StereoSizeConfig CONFIG_W_PADDING_W_ROTATE(STEREO_AREA_W_PADDING, STEREO_AREA_W_ROTATE);
    const StereoSizeConfig CONFIG_W_PADDING_WO_ROTATE(STEREO_AREA_W_PADDING, STEREO_AREA_WO_ROTATE);

    StereoArea result = STEREO_AREA_ZERO;
    auto __refineDLDP2Size = [&](StereoArea &area)
    {
        StereoArea MAX_AREA = StereoArea(1920, 1080).applyRatio(StereoSettingProvider::imageRatio(), E_KEEP_AREA_SIZE);
        if(area.size.w > MAX_AREA.size.w ||
           area.size.h > MAX_AREA.size.h)
        {
            MY_LOGW("Customized size > limitation, change from %dx%d to %dx%d",
                    area.size.w, area.size.h, MAX_AREA.size.w, MAX_AREA.size.h);
            area = MAX_AREA;
        } else {
            //Get FEFM input size(WROT)
            Pass2SizeInfo p2SizeInfo;
            getPass2SizeInfo(PASS2A, eSTEREO_SCENARIO_CAPTURE, p2SizeInfo, ROTATE_WO_SLANT);
            p2SizeInfo.areaWROT.rotatedByModule(ROTATE_WO_SLANT);
            if(area.size.w < p2SizeInfo.areaWROT.size.w ||
               area.size.h < p2SizeInfo.areaWROT.size.h)
            {
                MY_LOGW("Customized size < limitation, change from %dx%d to %dx%d",
                        area.size.w, area.size.h, p2SizeInfo.areaWROT.size.w, p2SizeInfo.areaWROT.size.h);
                area = p2SizeInfo.areaWROT;
            }
        }
    };

    switch(param.eName) {
        //P2A output for DL-Depth
        case E_DLD_P2A_M:
            {
                //Rule:
                //1. Max: 1920x1080
                //2. Min: FEFM input size
                //3. Can beCustomized in setting
                //4. Default value is decided by experiences(in thirdPartyDepthmapSize)
                result = thirdPartyDepthmapSize(StereoSettingProvider::imageRatio(), param.eScenario);
                __refineDLDP2Size(result);
                result.apply64Align();
            }
            break;
        case E_DLD_P2A_S:
            {
                result = getBufferSize(E_DLD_P2A_M);
                //Get FOV Cropped size
                result *= StereoSettingProvider::getStereoCameraFOVRatio(param.previewCropRatio);
                __refineDLDP2Size(result);
                result.apply64Align();
            }
            break;

        //DL Depth
        case E_DL_DEPTHMAP:
            {
                result = thirdPartyDepthmapSize(StereoSettingProvider::imageRatio(), param.eScenario);
                result.rotatedByModule(ROTATE_WO_SLANT);
            }
            break;
#if (1==HAS_VAIDEPTH)
        case E_VAIDEPTH_DEPTHMAP:
            result = StereoArea(__getVideoAIDepthModelSize());
            if(param.withSlantRotation) {
                result.rotatedByModule()
                      .apply2Align();
            }
            break;
#endif
        case E_MASK_IN_M:
        case E_RECT_IN_M:
            {
                switch(param.eScenario)
                {
                    case eSTEREO_SCENARIO_PREVIEW:
                    case eSTEREO_SCENARIO_RECORD:
                    default:
                        if(__useVAIDepthSize(param.eScenario)) {
#if (1==HAS_VAIDEPTH)

                            result = StereoArea(__getVideoAIDepthModelSize());
#endif
                        } else {
                            result = StereoSize::getStereoArea1x(CONFIG_WO_PADDING_WO_ROTATE);
                            if(param.withSlantRotation) {
#if (1==HAS_WPE)
                                result.apply64AlignToWidth()   // For WPE
                                      .applyRatio(StereoSettingProvider::imageRatio(), E_KEEP_WIDTH)
                                      .apply2AlignToHeight();
#else
                                result.apply2Align();
#endif
                            }
                        }
                        break;
#if (1==HAS_AIDEPTH)
                    case eSTEREO_SCENARIO_CAPTURE:
                        result = StereoArea(__getAIDepthModelSize());
                        break;
#endif
                }

                //P1 does not rotate
                if(result.size.h > result.size.w) {
                    std::swap(result.size.w, result.size.h);
                }
            }
            break;

        case E_MASK_IN_S:
        case E_RECT_IN_S:
            {
                result = getBufferSize(param.replaceName(E_RECT_IN_M).withoutSlant());
                //RECT_IN_M comes from P1 and it cannot rotate
                //RECT_IN_S(non-slant) comes from MDP which has rotate
                if (!StereoSettingProvider::isSlantCameraModule()) {
                    result.rotatedByModule();
                }

                if(StereoSettingProvider::isActiveStereo())
                {
                    break;
                }

                result.removePadding();

                //For quality enhancement
                const float WARPING_IMAGE_RATIO = 1.0f; //For better quality
                const float FOV_RATIO           = StereoSettingProvider::getStereoCameraFOVRatio(1.0f);
                const float IMAGE_RATIO         = WARPING_IMAGE_RATIO * FOV_RATIO;

                //WPE input requires stride to 64 bytes aligned for performance
                result.enlargeWith2AlignedRounding(IMAGE_RATIO);
#if (1==HAS_WPE)
                result.apply64AlignToWidth(!IS_KEEP_CONTENT, !IS_CENTER_CONTENT)
                      .apply2AlignToHeight();
#else
                result.apply32Align();
#endif

            }
            break;

        //N3D Output
        case E_MV_Y:
        case E_MASK_M_Y:
        case E_SV_Y:
        case E_MASK_S_Y:
            {
                if(param.eScenario == eSTEREO_SCENARIO_CAPTURE)
                {
                    result = getBufferSize(param.replaceName(E_RECT_IN_M));
                    break;
                }

                if(__useVAIDepthSize(param.eScenario)) {
#if (1==HAS_VAIDEPTH)

                    //For DPE
                    MSize modelSize = __getVideoAIDepthModelSize();
                    const int MARGIN = 32;
                    const int PADDING = MARGIN<<1;
                    modelSize.w += PADDING;
                    result = StereoArea(modelSize, MSize(PADDING, 0), MPoint(MARGIN, 0));

                    if (param.withSlantRotation) {
                        result.rotatedByModule(param.withSlantRotation);
                    }
#endif
                } else {
                    if( !param.withSlantRotation ) {
                        result = StereoSize::getStereoArea1x(CONFIG_W_PADDING_W_ROTATE);
                    } else {
                        result = StereoSize::getStereoArea1x(CONFIG_W_PADDING_WO_ROTATE);
                        result.rotatedByModule(param.withSlantRotation);
                    }
                }

                // For DVS input, image stiride must be 64 bytes-aligned
                result.apply64AlignToWidth()
                      .apply2AlignToContent();

                if ( E_SV_Y     == param.eName ||
                     E_MASK_S_Y == param.eName ) {
                    result.removePadding(IS_KEEP_SIZE);
                }
            }
            break;

        //DPE(ME) Output
        case E_DV_LR:
            {
                if (!param.withSlantRotation) {
#if (1==HAS_VAIDEPTH)
                    if(__useVAIDepthSize(param.eScenario))
                    {
                        result = __getVideoAIDepthModelSize();
                        result.size.w += 32;    //padding for single side
                    }
                    else
#endif
                    {
                        result = StereoSize::getStereoArea1x(CONFIG_W_PADDING_W_ROTATE);
                    }
                } else {
#if (1==HAS_VAIDEPTH)
                    if(__useVAIDepthSize(param.eScenario))
                    {
                        result = __getVideoAIDepthModelSize();
                        result.size.w += 32;    //padding for single side
                    }
                    else
#endif
                    {
                        result = StereoSize::getStereoArea1x(CONFIG_W_PADDING_WO_ROTATE);
                    }

                    result.rotatedByModule();
                }

                result.apply128AlignToWidth(IS_KEEP_CONTENT, !IS_CENTER_CONTENT)
                      .apply2AlignToContent()
                      .applyDoubleHeight();
            }
            break;

        //For WPE
        case E_WARP_MAP_M:
        case E_WARP_MAP_S:
            {
                result = getBufferSize(param.replaceName(E_SV_Y));

                //Only keep size
                result.removePadding(IS_KEEP_SIZE);

                // Rotated slant size will be a square, so no need to rotate
                if (!param.withSlantRotation) {
                    result.rotatedByModule();
                }
                result /= 2;

                //Check hardware ability
                if(result.size.w > WPE_MAX_SIZE.w ||
                   result.size.h > WPE_MAX_SIZE.h)
                {
                    if(!param.withSlantRotation) {
                        result = StereoArea(WPE_MAX_SIZE);
                        int m, n;
                        StereoSettingProvider::imageRatio().MToN(m, n);

                        if(m*WPE_MAX_SIZE.h/WPE_MAX_SIZE.w >= n) {
                            result.applyRatio(StereoSettingProvider::imageRatio(), E_KEEP_WIDTH);
                        } else {
                            result.applyRatio(StereoSettingProvider::imageRatio(), E_KEEP_HEIGHT);
                        }
                    } else {
                        result.size.w = std::min(WPE_MAX_SIZE.w, WPE_MAX_SIZE.h);
                        result.size.h = result.size.w;
                    }
                }

                // Rotated slant size will be a square, so no need to rotate
                if (!param.withSlantRotation) {
                    result.rotatedByModule();
                }

                //1. WPE has better performance if 128 bits aligned, and WARP_MAP is 32 bits/pixel
                //   so warp map must be 4 aligned(128/32=4)
                //2. GPU must be 32 pixel aligned
                result.apply32Align();
            }
            break;

        //OCC Output
        case E_MY_S:
            param.withoutSlant();
            if(!StereoSettingProvider::isMTKDepthmapMode()) {
                if(__useVAIDepthSize(param.eScenario)) {
                    result = getBufferSize(param.replaceName(E_VAIDEPTH_DEPTHMAP));
                } else {
                    result = StereoSize::getStereoArea1x(CONFIG_WO_PADDING_W_ROTATE);
                }

                result.apply16AlignToWidth(IS_KEEP_CONTENT, !IS_CENTER_CONTENT);
                break;
            }   //no need break here for mtk size
        case E_DMP_H:
        case E_CFM_H:
        case E_RESPO:
        case E_DMH:
        case E_OCC:
        case E_NOC:
        case E_CFM_M:
            {
                if(!param.withSlantRotation) {
                    result = (eSTEREO_SCENARIO_PREVIEW == param.eScenario)
                             ? StereoSize::getStereoArea1x(CONFIG_WO_PADDING_W_ROTATE)
                             : getBufferSize(param.replaceName(E_VAIDEPTH_DEPTHMAP));
                } else {
                    if (eSTEREO_SCENARIO_PREVIEW == param.eScenario) {
                        result = StereoSize::getStereoArea1x(CONFIG_WO_PADDING_WO_ROTATE);
                    } else {
                        result = getBufferSize(param.replaceName(E_VAIDEPTH_DEPTHMAP).withoutSlant());
                    }

                    result.rotatedByModule();
                }

                result.apply128AlignToWidth(IS_KEEP_CONTENT, !IS_CENTER_CONTENT)
                      .apply2AlignToContent();
            }
            break;
        case E_ASF_CRM:
        case E_ASF_RD:
        case E_ASF_HF:
        case E_DMW:
            {
                result = (__useVAIDepthSize(param.eScenario))
                         ? getBufferSize(param.replaceName(E_VAIDEPTH_DEPTHMAP))
                         : StereoSize::getStereoArea1x(CONFIG_WO_PADDING_W_ROTATE);

                if(E_ASF_CRM == param.eName) {
                    result.size.h *= 4;
                }

                result.apply128AlignToWidth(IS_KEEP_CONTENT, !IS_CENTER_CONTENT);
            }
            break;
        case E_BOKEH_PACKED_BUFFER:
            {
                result = getBufferSize(param.replaceName(E_DMW));
                if(StereoHAL::isVertical(param.capOrientation)) {  //90 or 270 degree
                    MSize::value_type s = result.size.w;
                    result.size.w = result.size.h;
                    result.size.h = s;
                }

                result.size.h *= 4;
            }
            break;
        //GF Input
        case E_GF_IN_IMG_4X:
            {
                result = StereoSize::getStereoArea4x(CONFIG_WO_PADDING_W_ROTATE);
            }
            break;

        case E_GF_IN_IMG_2X:
            {
                result = StereoSize::getStereoArea2x(CONFIG_WO_PADDING_W_ROTATE);
            }
            break;

        //GF Output
        case E_DMG:
        case E_DMBG:
            if(eSTEREO_SCENARIO_CAPTURE == param.eScenario) {
                result = getBufferSize(param.replaceName(E_DL_DEPTHMAP));
            } else if(__useVAIDepthSize(param.eScenario)) {
                result = getBufferSize(param.replaceName(E_VAIDEPTH_DEPTHMAP)).rotatedByModule();
            } else {
                result = StereoSize::getStereoArea1x(CONFIG_WO_PADDING_WO_ROTATE);
            }

            break;

        case E_DEPTH_MAP:
            if(eSTEREO_SCENARIO_CAPTURE == param.eScenario) {
                result = getBufferSize(param.replaceName(E_DL_DEPTHMAP).withoutSlant());
                break;
            }

            if(__useVAIDepthSize(param.eScenario)) {
                result = getBufferSize(param.replaceName(E_MY_S).withoutSlant());
                break;
            }

            if(StereoSettingProvider::is3rdParty(param.eScenario)){
                result = thirdPartyDepthmapSize(StereoSettingProvider::imageRatio());
            }
            else if(StereoSettingProvider::isMTKDepthmapMode())
            {
                result = StereoSize::getStereoArea1x(CONFIG_WO_PADDING_W_ROTATE);
            }
            else
            {
                result = getBufferSize(param.replaceName(E_DMG).withoutSlant());
            }
            break;

        case E_INK:
            result = StereoSize::getStereoArea2x(CONFIG_WO_PADDING_WO_ROTATE);
            break;

        //Bokeh Output
        case E_BOKEH_WROT: //Saved image
            switch(param.eScenario) {
                case eSTEREO_SCENARIO_PREVIEW:
                    result = STEREO_AREA_ZERO;
                    break;
                case eSTEREO_SCENARIO_RECORD:
                    result = Pass2A_SizeProvider::instance()->getWDMAArea(param);
                    break;
                case eSTEREO_SCENARIO_CAPTURE:
                    result = StereoArea(__captureSize);
                    break;
                default:
                    break;
            }
            break;
        case E_BOKEH_WDMA:
            switch(param.eScenario) {
                case eSTEREO_SCENARIO_PREVIEW:  //Display
                case eSTEREO_SCENARIO_RECORD:
                    result = Pass2A_SizeProvider::instance()->getWDMAArea(param);
                    break;
                case eSTEREO_SCENARIO_CAPTURE:  //Clean image
                    result = StereoArea(__captureSize);
                    break;
                default:
                    break;
            }
            break;
        case E_BOKEH_3DNR:
            switch(param.eScenario) {
                case eSTEREO_SCENARIO_PREVIEW:
                case eSTEREO_SCENARIO_RECORD:
                    result = Pass2A_SizeProvider::instance()->getWDMAArea(param);
                    break;
                default:
                    break;
            }
            break;
        case E_FE1B_INPUT:
            return Pass2A_SizeProvider::instance()->getWROTArea(param);
        default:
            break;
    }

    return result;
}
