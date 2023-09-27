/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "mtkcam-P1HwSettingPolicyMC"

#include <mtkcam3/pipeline/policy/IConfigP1HwSettingPolicy.h>
//
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/drv/iopipe/CamIO/INormalPipe.h>
#include <mtkcam/drv/iopipe/CamIO/Cam_QueryDef.h>
#include <mtkcam3/feature/fsc/fsc_defs.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/feature/PIPInfo.h>
//
#include "mtkcam3/feature/stereo/hal/stereo_size_provider.h"
//
#include "myutils.h"
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::NSIoPipe::NSCamIOPipe;
using namespace NSCam::FSC;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {


/**
 * Make a function target - multicam version
 */
FunctionType_Configuration_P1HwSettingPolicy makePolicy_Configuration_P1HwSetting_multicam()
{
    return [](Configuration_P1HwSetting_Params const& params) -> int {
        auto pvOut = params.pvOut;
        auto pSensorSetting = params.pSensorSetting;
        auto pStreamingFeatureSetting = params.pStreamingFeatureSetting;
        auto pSeamlessSwitchFeatureSetting = params.pSeamlessSwitchFeatureSetting;
        auto pPipelineNodesNeed __unused = params.pPipelineNodesNeed;
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;
        auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;
        auto const& pParsedAppImageStreamInfo = pPipelineUserConfiguration->pParsedAppImageStreamInfo;

        for ( const auto& [sensorId, needP1] : pPipelineNodesNeed->needP1Node )
        {
            if(!needP1)
                continue;

            MY_LOGD("update sensor id[%d] imgo and rrzo", sensorId);
            P1HwSetting setting;
            std::shared_ptr<NSCamHW::HwInfoHelper> infohelper = std::make_shared<NSCamHW::HwInfoHelper>(sensorId);
            bool ret = infohelper != nullptr
                    && infohelper->updateInfos()
                    && infohelper->queryPixelMode( (*pSensorSetting).at(sensorId).sensorMode, (*pSensorSetting).at(sensorId).sensorFps, setting.pixelMode)
                        ;
            if ( CC_UNLIKELY(!ret) ) {
                MY_LOGE("sensorId : %d, queryPixelMode error", sensorId);
                return UNKNOWN_ERROR;
            }
            MSize const& sensorSize = (*pSensorSetting).at(sensorId).sensorSize;

            //#define MIN_RRZO_RATIO_100X     (25)
            MINT32     rrzoMaxWidth = 0;
            infohelper->quertMaxRrzoWidth(rrzoMaxWidth);

            #define CHECK_TARGET_SIZE(_msg_, _size_) \
            MY_LOGD_IF( 1, "%s: target size(%dx%d)", _msg_, _size_.w, _size_.h);

            #define ALIGN16(x) x = (((x) + 15) & ~(15));
            // estimate preview yuv max size
            MSize const max_preview_size = refine::align_2(
                    MSize(rrzoMaxWidth, rrzoMaxWidth * sensorSize.h / sensorSize.w));
            CHECK_TARGET_SIZE("max_rrzo_size", max_preview_size);
            //
            MSize maxYuvStreamSize;
            MSize largeYuvStreamSize;
            MSize stereoPreviewSize = StereoSizeProvider::getInstance()->getPreviewSize();

            #define PRV_MAX_DURATION (34000000) //ns
            for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
            {
                MSize const streamSize = (n.second)->getImgSize();
                if(streamSize.w > stereoPreviewSize.w && streamSize.h > stereoPreviewSize.h)
                    continue;
                auto search = pPipelineUserConfiguration->vMinFrameDuration.find((n.second)->getStreamId());
                int64_t MinFrameDuration = 0;
                if (search != pPipelineUserConfiguration->vMinFrameDuration.end())
                {
                    MinFrameDuration = search->second;
                }
                bool candidatePreview = (
                        ((n.second)->getUsageForConsumer() & (GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER)) != 0
                        || MinFrameDuration <= PRV_MAX_DURATION
                        );
                // if stream's size is suitable to use rrzo
                if( streamSize.w <= max_preview_size.w && streamSize.h <= max_preview_size.h && candidatePreview)
                    refine::not_smaller_than(maxYuvStreamSize, streamSize);
                else
                    refine::not_smaller_than(largeYuvStreamSize, streamSize);
            }
            if (maxYuvStreamSize.w == 0 || maxYuvStreamSize.h == 0)
            {
                // check physical
                auto iter = pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.find(sensorId);
                if(iter != pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.end())
                {
                    for(auto&& item:iter->second)
                    {
                        auto imgSize = item->getImgSize();
                        auto search = pPipelineUserConfiguration->vMinFrameDuration.find(item->getStreamId());
                        int64_t MinFrameDuration = 0;
                        if (search != pPipelineUserConfiguration->vMinFrameDuration.end())
                        {
                            MinFrameDuration = search->second;
                        }
                        if(MinFrameDuration > PRV_MAX_DURATION)
                            continue;
                        else
                        {
                            refine::not_smaller_than(maxYuvStreamSize, imgSize);
                        }
                    }
                    MY_LOGD("set max physical size(%dx%d)", maxYuvStreamSize.w, maxYuvStreamSize.h);
                }
                else
                {
                    // check physical
                    auto iter = pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.find(sensorId);
                    if(iter != pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.end())
                    {
                        for(auto&& item:iter->second)
                        {
                            auto imgSize = item->getImgSize();
                            if(imgSize.w > stereoPreviewSize.w && imgSize.h > stereoPreviewSize.h)
                                continue;
                            else
                            {
                                maxYuvStreamSize.w = imgSize.w;
                                maxYuvStreamSize.h = imgSize.h;
                            }
                        }
                        MY_LOGD("set max physical size(%dx%d)", maxYuvStreamSize.w, maxYuvStreamSize.h);
                    }
                    else
                    {
                        MY_LOGW("all yuv size is larger than max rrzo size, set default rrzo to 1280x720");
                        maxYuvStreamSize.w = 1280;
                        maxYuvStreamSize.h = 720;
                    }
                }
            }
                        const MSize hd_size = MSize(1280, 960);
                        if (maxYuvStreamSize.w < hd_size.w && maxYuvStreamSize.h < hd_size.h)
                        {
                            MY_LOGI("maxYuvStreamSize is too small(%dx%d)", maxYuvStreamSize.w, maxYuvStreamSize.h);
                            maxYuvStreamSize.h = hd_size.h;
                            maxYuvStreamSize.w = hd_size.w;
                            MY_LOGI("maxYuvStreamSize set rrzo to %dx%d", maxYuvStreamSize.w, maxYuvStreamSize.h);
                        }
            // use resized raw if
            // 1. raw sensor
            // 2. some streams need this
            if( infohelper->isRaw() )
            {
                //
                // currently, should always enable resized raw due to some reasons...
                //
                // initial value
                MSize target_rrzo_size = maxYuvStreamSize;
                target_rrzo_size.w *= pStreamingFeatureSetting->targetRrzoRatio;
                target_rrzo_size.h *= pStreamingFeatureSetting->targetRrzoRatio;
                if ( pStreamingFeatureSetting->rrzoHeightToWidth != 0 )
                {
                    ALIGN16(target_rrzo_size.w);
                    target_rrzo_size.h = target_rrzo_size.w * pStreamingFeatureSetting->rrzoHeightToWidth;
                    ALIGN16(target_rrzo_size.h);
                }
                else
                {
                    // align sensor aspect ratio
                    if (target_rrzo_size.w * sensorSize.h > target_rrzo_size.h * sensorSize.w)
                    {
                        ALIGN16(target_rrzo_size.w);
                        target_rrzo_size.h = target_rrzo_size.w * sensorSize.h / sensorSize.w;
                        ALIGN16(target_rrzo_size.h);
                    }
                    else
                    {
                        ALIGN16(target_rrzo_size.h);
                        target_rrzo_size.w = target_rrzo_size.h * sensorSize.w / sensorSize.h;
                        ALIGN16(target_rrzo_size.w);
                    }
                }
                MINT32 user_w = ::property_get_int32("vendor.camera.force_rrzo_width", 0);
                MINT32 user_h = ::property_get_int32("vendor.camera.force_rrzo_height", 0);
                if (user_w && user_h)
                {
                    target_rrzo_size.w = user_w;
                    target_rrzo_size.h = user_h;
                }
                CHECK_TARGET_SIZE("sensor size", sensorSize);
                CHECK_TARGET_SIZE("target rrzo stream", target_rrzo_size);
                // apply limitations
                //  1. lower bounds
                {
                    // get eis ownership and apply eis hw limitation
                    if ( pStreamingFeatureSetting->bIsEIS )
                    {
                        MUINT32 minRrzoEisW = pStreamingFeatureSetting->minRrzoEisW;
                        MSize const min_rrzo_eis_size = refine::align_2(
                                MSize(minRrzoEisW, minRrzoEisW * sensorSize.h / sensorSize.w));
                        refine::not_smaller_than(target_rrzo_size, min_rrzo_eis_size);
                        MINT32 lossless = pStreamingFeatureSetting->eisInfo.lossless;
                        target_rrzo_size = refine::align_2(
                                refine::scale_roundup(target_rrzo_size,
                                lossless ? pStreamingFeatureSetting->eisInfo.factor : 100, 100)
                                );
                       CHECK_TARGET_SIZE("eis lower bound limitation", target_rrzo_size);
                    }
                }
                //  2. upper bounds
                {
                    {
                        refine::not_larger_than(target_rrzo_size, max_preview_size);
                        CHECK_TARGET_SIZE("preview upper bound limitation", target_rrzo_size);
                    }
                    refine::not_larger_than(target_rrzo_size, sensorSize);
                    CHECK_TARGET_SIZE("sensor size upper bound limitation", target_rrzo_size);
                }
                MY_LOGD_IF(1, "rrzo size(%dx%d)", target_rrzo_size.w, target_rrzo_size.h);
                //
                // check hw limitation with pixel mode & stride
                MSize   rrzoSize = target_rrzo_size;
                int32_t rrzoFormat = 0;
                size_t  rrzoStride = 0;
                if( ! infohelper->getRrzoFmt(10, rrzoFormat) ||
                    ! infohelper->alignRrzoHwLimitation(target_rrzo_size, sensorSize, rrzoSize) ||
                    ! infohelper->alignPass1HwLimitation(rrzoFormat, MFALSE/*isImgo*/,
                                                         rrzoSize, rrzoStride ) )
                {
                    MY_LOGE("wrong params about rrzo");
                    return BAD_VALUE;
                }

                auto& rrzoDefaultRequest = setting.rrzoDefaultRequest;
                rrzoDefaultRequest.imageSize = rrzoSize;
                rrzoDefaultRequest.format = rrzoFormat;
            }
            //
            // use full raw, if
            // 1. jpeg stream (&& not met BW limit)
            // 2. raw stream
            // 3. opaque stream
            // 4. or stream's size is beyond rrzo's limit
            // 5. need P2Capture node
            // 6. have mipi raw
            bool useImgo =
                (pParsedAppImageStreamInfo->pAppImage_Jpeg.get() /*&& ! mPipelineConfigParams.mbHasRecording*/) ||
                !pParsedAppImageStreamInfo->vAppImage_Output_RAW16.empty() ||
                !pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.empty() ||
                pParsedAppImageStreamInfo->pAppImage_Input_Yuv.get() ||
                !!largeYuvStreamSize ||
                !pParsedAppImageStreamInfo->vAppImage_Output_RAW10.empty() ||
                !pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical.empty() ||
                property_get_int32("vendor.debug.feature.forceEnableIMGO", 0) ||
                pPipelineNodesNeed->needP2CaptureNode;

            if( useImgo )
            {
                bool useMipiRaw10 = ( pParsedAppImageStreamInfo->vAppImage_Output_RAW10.size() ||
                                      pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical.size() )
                        ? true : false;

                // check hw limitation with pixel mode & stride
                MSize   imgoSize = sensorSize;
                int32_t imgoFormat = 0;
                size_t  imgoStride = 0;
                MSize   imgoSeamlessFullSize = MSize();

                /*
                 * Seamless Switch: decide imgo for seamless
                 *   1. Seamless remosaic capture : imgo size for full mode. (pool/run-time)
                 *   2. Seamless preview : imgo size for crop mode. (share with default imgo)
                 */
                const auto main1SensorId = pPipelineStaticInfo->sensorId[0];
                auto isSeamlessDefaultMode = [&pSensorSetting, &pSeamlessSwitchFeatureSetting]
                        (const uint32_t sensorId) -> bool {
                    if (pSeamlessSwitchFeatureSetting->defaultScene < 0) {
                        MY_LOGE("cannot get seamless default scene");
                        return false;
                    }
                    const uint32_t defaultMode =
                            static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->defaultScene);

                    const auto iter = (*pSensorSetting).find(sensorId);
                    if (iter != (*pSensorSetting).end() &&
                            iter->second.sensorMode == defaultMode) {
                        return true;
                    }
                    return false;
                };
                if (pSeamlessSwitchFeatureSetting->isSeamlessEnable &&
                        sensorId == main1SensorId && isSeamlessDefaultMode(sensorId)) {
                    setting.imgoSeamlessRequest.imageSize = MSize(0, 0);
                    if (pSeamlessSwitchFeatureSetting->isSeamlessCaptureEnable) {
                        // full mode : need full size imgo
                        MY_LOGE_IF(pSeamlessSwitchFeatureSetting->fullScene < 0,
                            "Seamless Switch: [Config] cannot get full mode");
                        const uint32_t fullMode = static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->fullScene);
                        MSize fullSize = MSize(0, 0);
                        if (!infohelper->getSensorSize(fullMode, fullSize)) {
                            MY_LOGE("Seamless Switch: [Config] cannot get full mode sensor size");
                        }
                        MY_LOGD("Seamless Switch: [Config] full mode = %u, get full size = %dx%d",
                            fullMode, fullSize.w, fullSize.h);
                        imgoSeamlessFullSize = fullSize;
                    }

                    if (pSeamlessSwitchFeatureSetting->isSeamlessPreviewEnable) {
                        // crop mode : share same setting with default mode
                        MY_LOGE_IF(pSeamlessSwitchFeatureSetting->cropScene < 0,
                            "Seamless Switch: [Config] cannot get crop mode");
                        const uint32_t cropMode = static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->cropScene);
                        MSize cropSize = MSize(0, 0);
                        if (!infohelper->getSensorSize(cropMode, cropSize)) {
                            MY_LOGE("Seamless Switch: [Config] cannot get crop mode sensor size");
                        }
                        MY_LOGD("Seamless Switch: [Config] crop mode = %u, get crop size = %dx%d",
                            cropMode, cropSize.w, cropSize.h);
                        imgoSize = cropSize;
                    }
                }

                if( ! infohelper->getImgoFmt(10, imgoFormat, false, false, useMipiRaw10) ||
                    ! infohelper->alignPass1HwLimitation(imgoFormat, MTRUE/*isImgo*/,
                                                         imgoSize, imgoStride ) )
                {
                    MY_LOGE("wrong params about imgo");
                    return BAD_VALUE;
                }

                auto& imgoAlloc = setting.imgoAlloc;
                imgoAlloc.imageSize = imgoSize;
                imgoAlloc.format = imgoFormat;

                auto& imgoRequest = setting.imgoDefaultRequest;
                imgoRequest.imageSize = imgoSize;
                imgoRequest.format = imgoFormat;

                if (imgoSeamlessFullSize != MSize(0, 0)) {
                    auto& seamlessReq = setting.imgoSeamlessRequest;
                    seamlessReq.imageSize = imgoSeamlessFullSize;
                    seamlessReq.format = imgoFormat;
                }
            }
            else
            {
                setting.imgoAlloc.imageSize = MSize(0,0);
                setting.imgoDefaultRequest.imageSize = MSize(0,0);
                setting.imgoSeamlessRequest.imageSize = MSize(0,0);
            }
            // update rsso size
            if( EFSC_FSC_ENABLED(pStreamingFeatureSetting->fscMode) )
            {
                setting.rssoSize = MSize(FSC_MAX_RSSO_WIDTH,512);//add extra 10% margin for FSC crop
            }
            else
            {
                setting.rssoSize = MSize(288,512);
            }
            // [After ISP 6.0] begin
            // FD
            if(pPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV)
            {
                MY_LOGD("support direct p1 fd yuv.");
                setting.fdyuvSize.w = 640;
                setting.fdyuvSize.h = (((640 * setting.rrzoDefaultRequest.imageSize.h) / setting.rrzoDefaultRequest.imageSize.w) + 1) & ~(1);
            }
            else
            {
                setting.fdyuvSize = MSize(0,0);
            }
            // AIAWB
            if ( pPipelineStaticInfo->isAIAWBSupported )
            {
                setting.AIAWBYuvSize.w = 640;
                setting.AIAWBYuvSize.h = (640 * sensorSize.h) / sensorSize.w;
                setting.AIAWBPort = P1_FDYUV;
            }
            else
            {
                setting.AIAWBYuvSize = MSize(0,0);
                setting.AIAWBPort = 0;
            }
            // scaled YUV
            if(pPipelineStaticInfo->numsP1YUV > NSCam::v1::Stereo::NO_P1_YUV)
            {
                auto sensorIdx = pPipelineStaticInfo->getIndexFromSensorId(sensorId);
                auto enum_stereo_sensor = (sensorIdx == 0) ?
                                StereoHAL::eSTEREO_SENSOR_MAIN1 :
                                StereoHAL::eSTEREO_SENSOR_MAIN2;
                MRect rect;
                MSize size;
                MUINT32 q_stride;
                auto numsP1YUV = pPipelineStaticInfo->numsP1YUV;
                bool useRSSOR2 = (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_RSSO_R2 || numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_YUVO_RSSO_R2);
                if(numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_CRZO)
                {
                    StereoSizeProvider::getInstance()->getPass1Size(
                        enum_stereo_sensor,
                        eImgFmt_NV16,
                        NSImageio::NSIspio::EPortIndex_CRZO_R2,
                        StereoHAL::eSTEREO_SCENARIO_PREVIEW, // in this mode, stereo only support zsd.
                        rect,
                        size,
                        q_stride);
                    setting.scaledYuvSize = size;
                    MY_LOGD("[%d] support direct p1 rectify yuv. (%dx%d)",
                                    sensorIdx,
                                    setting.scaledYuvSize.w,
                                    setting.scaledYuvSize.h);
                }
                else if(useRSSOR2)
                {
                    if(pParsedAppImageStreamInfo->hasVideoConsumer)
                    {
                         StereoSizeProvider::getInstance()->getPass1Size(
                            enum_stereo_sensor,
                            eImgFmt_Y8,
                            NSImageio::NSIspio::EPortIndex_RSSO_R2,
                            StereoHAL::eSTEREO_SCENARIO_RECORD, // in this mode, stereo only support zsd.
                            rect,
                            size,
                            q_stride);
                        setting.scaledYuvSize = size;
                        MY_LOGD("[%d] [Record Mode] support direct p1 rectify yuv. (%dx%d)",
                                        sensorIdx,
                                        setting.scaledYuvSize.w,
                                        setting.scaledYuvSize.h);
                    }
                    else
                    {
                        StereoSizeProvider::getInstance()->getPass1Size(
                            enum_stereo_sensor,
                            eImgFmt_Y8,
                            NSImageio::NSIspio::EPortIndex_RSSO_R2,
                            StereoHAL::eSTEREO_SCENARIO_PREVIEW, // in this mode, stereo only support zsd.
                            rect,
                            size,
                            q_stride);
                        setting.scaledYuvSize = size;
                        MY_LOGD("[%d] [Preview Mode] support direct p1 rectify yuv. (%dx%d)",
                                        sensorIdx,
                                        setting.scaledYuvSize.w,
                                        setting.scaledYuvSize.h);
                    }
                }
            }
            else
            {
                setting.scaledYuvSize = MSize(0,0);
            }
            // [After ISP 6.0] end
            CAM_ULOGMI("%s", toString(setting).c_str());
            (*pvOut).emplace(sensorId, setting);
        }

        // check p1 sync cability
        // If it uses camsv, it has to set usingCamSV flag.
        // It has to group all physic sensor to one and sent to p1 query function.
        {
            auto& pipInfo = PIPInfo::getInstance();
            MY_LOGD("check hw resource");
            {
                // camsv check
                sCAM_QUERY_HW_RES_MGR queryHwRes;
                SEN_INFO sen_info;
                // 1. prepare sensor information and push to QueryInput queue.
                for (const auto& [_sensorId, _hwSetting] : *pvOut)
                {
                    sen_info.sensorIdx = _sensorId;
                    sen_info.rrz_out_w = _hwSetting.rrzoDefaultRequest.imageSize.w;
                    queryHwRes.QueryInput.push_back(sen_info);
                }
                // 2. create query object
                auto pModule = NSCam::NSIoPipe::NSCamIOPipe::INormalPipeModule::get();
                if(!CC_UNLIKELY(pModule))
                {
                    MY_LOGE("create normal pipe module fail.");
                    return UNKNOWN_ERROR;
                }
                // 3. query
                auto ret = pModule->query(queryHwRes.Cmd, (MUINTPTR)&queryHwRes);
                MY_LOGW_IF(!ret, "cannot use NormalPipeModule to query Hw resource.");
                // 4. check result
                for(size_t i = 0 ; i < queryHwRes.QueryOutput.size() ; ++i)
                {
                    MY_LOGA_IF((EPipeSelect_None == queryHwRes.QueryOutput[i].pipeSel),
                                "Hw resource overspec");
                    if(EPipeSelect_NormalSv == queryHwRes.QueryOutput[i].pipeSel)
                    {
                        auto sensorId = pPipelineStaticInfo->sensorId[i];
                        pvOut->at(sensorId).usingCamSV = MTRUE;
                        MY_LOGI("DevId[%d] will use camsv.", queryHwRes.QueryOutput[i].sensorIdx);
                    }
                }
            }

            auto main1SensorId = pPipelineStaticInfo->sensorId[0];
            auto main2SensorId = pPipelineStaticInfo->sensorId[1];
            auto numsP1YUV = pPipelineStaticInfo->numsP1YUV;
            bool useRSSOR2 = (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_RSSO_R2 || numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_YUVO_RSSO_R2);
            if(useRSSOR2)
            {
                pvOut->at(main1SensorId).canSupportScaledYuv = MTRUE;
                pvOut->at(main2SensorId).canSupportScaledYuv = MTRUE;
            }
            else if(numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_CRZO)
            {
                // workaround
                // scaled yuv capability.
                pvOut->at(main1SensorId).canSupportScaledYuv = MTRUE;
                pvOut->at(main2SensorId).canSupportScaledYuv = MFALSE;
            }
            else
            {
                pvOut->at(main1SensorId).canSupportScaledYuv = MFALSE;
                pvOut->at(main2SensorId).canSupportScaledYuv = MFALSE;
            }
            // check isp quality
            if(pPipelineStaticInfo->sensorId.size() > 1)
            {
                auto mainSensorId = main1SensorId;
                MSize mainSensorRrzoSize = (*pvOut).at(mainSensorId).rrzoDefaultRequest.imageSize;
                MUINT32 mainSensorMode = (*pSensorSetting).at(mainSensorId).sensorMode;
                MUINT32 mainSensorFps = (*pSensorSetting).at(mainSensorId).sensorFps;

                auto subSensorId = main2SensorId;
                MSize subSensorRrzoSize = (*pvOut).at(subSensorId).rrzoDefaultRequest.imageSize;
                MUINT32 subSensorMode = (*pSensorSetting).at(subSensorId).sensorMode;
                MUINT32 subSensorFps = (*pSensorSetting).at(subSensorId).sensorFps;

                auto infohelper = IHwInfoHelperManager::get()->getHwInfoHelper(mainSensorId);
                IHwInfoHelper::QueryP1DrvIspParams params;

                MINT32 maxSizeWidth = 0;

                int forceDSwitchControl = ::property_get_int32("vendor.debug.rear_ducam.off_twin", -1);
                MBOOL offTwin = (MTK_MULTI_CAM_FEATURE_MODE_ZOOM ==
                                pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo->mDualFeatureMode)?
                                MTRUE:MFALSE;
                if(forceDSwitchControl >= 1) offTwin = false;
                else if(forceDSwitchControl == 0) offTwin = true;

                // 1. select max imgo size from second element to end.
                for(const auto& sensorId : pPipelineStaticInfo->sensorId)
                {
                    // because all sensor will use same ratio, so only check width.
                    if(pvOut->at(sensorId).imgoDefaultRequest.imageSize.w > maxSizeWidth && (sensorId != main1SensorId))
                    {
                        subSensorId = sensorId;
                        maxSizeWidth = (*pvOut).at(sensorId).imgoDefaultRequest.imageSize.w;
                        subSensorRrzoSize = (*pvOut).at(sensorId).rrzoDefaultRequest.imageSize;
                        subSensorMode = (*pSensorSetting).at(sensorId).sensorMode;
                        subSensorFps = (*pSensorSetting).at(sensorId).sensorFps;
                    }
                }

                // If pip enable, use PIP setting
                int anotherDeviceId = -1;
                if (pipInfo.getMaxOpenCamDevNum() >= 2) {
                    for (auto id : pipInfo.getOrderedLogicalDeviceId()) {
                        if (id != pPipelineStaticInfo->openId) {
                            anotherDeviceId = id;
                        }
                    }
                    if (anotherDeviceId == -1) {
                        MY_LOGE("Cannot get another device from ordered logical device list");
                        return UNKNOWN_ERROR;
                    }
                }

                if (pipInfo.getMaxOpenCamDevNum() >= 2 && pipInfo.getPIPConfigureDone() == false) {
                    if(mainSensorRrzoSize.w < subSensorRrzoSize.w) {
                        mainSensorId = subSensorId;
                        mainSensorRrzoSize = subSensorRrzoSize;
                        mainSensorMode = subSensorMode;
                        mainSensorFps = subSensorFps;
                    }
                    subSensorId = pipInfo.generateSensorSetting(anotherDeviceId);
                    if (subSensorId == -1)
                    {
                        MY_LOGE("Cannot get largest sensor from another device");
                        return UNKNOWN_ERROR;
                    }
                    subSensorRrzoSize = MSize(1920, 1440); // hard code
                    pipInfo.querySubSensorInfo(anotherDeviceId, subSensorId, subSensorMode, subSensorFps);
                }

                // 2. set data.
                if (pipInfo.getMaxOpenCamDevNum() < 2 || (pipInfo.getMaxOpenCamDevNum() >= 2 && pipInfo.getPIPConfigureDone() == false)) {
                    params.in.push_back(IHwInfoHelper::QueryP1DrvIspParams::CameraInputParams{
                        .sensorId = mainSensorId,
                        .sensorMode = mainSensorMode,
                        .minProcessingFps = ((mainSensorFps < 30)? mainSensorFps : 30),
                        .rrzoImgSize = mainSensorRrzoSize,
                        .offTwin = offTwin,
                    });

                    params.in.push_back(IHwInfoHelper::QueryP1DrvIspParams::CameraInputParams{
                        .sensorId = subSensorId,
                        .sensorMode = subSensorMode,
                        .minProcessingFps = ((subSensorFps < 30)? subSensorFps : 30),
                        .rrzoImgSize = subSensorRrzoSize,
                        .offTwin = offTwin,
                    });


                    MY_LOGD("Query set: index_0(%d) index_1(%d) offTwin(%d)",
                                mainSensorId, subSensorId, offTwin);
                    // 3. query result.
                    infohelper->queryP1DrvIspCapability(params);
                    if(params.out.size() != params.in.size())
                    {
                        MY_LOGE("in(%zu) != out(%zu), please check setting.", params.in.size(), params.out.size());
                        return UNKNOWN_ERROR;
                    }

                    // 4. assign value to out param
                    auto ISP_QUALITY_TO_STRING = [](uint32_t value) -> std::string
                    {
                        switch (value)
                        {
                            case eCamIQ_L:
                                return std::string("LOW");
                            case eCamIQ_H:
                                return std::string("Hight");
                            case eCamIQ_MAX:
                                return std::string("Max");
                            default:
                                return std::string("UNKNOWN");
                        }
                    };
                    size_t index = 0;
                    for(const auto& sensorId : pPipelineStaticInfo->sensorId)
                    {
                        if (pipInfo.getMaxOpenCamDevNum() < 2) {
                            if(sensorId == mainSensorId) index = 0;
                            else index = 1;
                        } else {
                            index = 0;
                        }
                        (*pvOut).at(sensorId).ispQuality = params.out[index].eIqLevel;
                        (*pvOut).at(sensorId).camResConfig = params.out[index].eResConfig;
                        MY_LOGD("sensor id(%d) using setting(%zu) ispQuality(%s) tg(%d)",
                                sensorId,
                                index,
                                ISP_QUALITY_TO_STRING((*pvOut).at(sensorId).ispQuality).c_str(),
                                (*pvOut).at(sensorId).camResConfig.Bits.targetTG);
                    }
                    if (pipInfo.getMaxOpenCamDevNum() >= 2) {
                        const auto deviceSensorMap = pipInfo.getDeviceSensorMap();
                        for (auto sensorId : deviceSensorMap.at(pPipelineStaticInfo->openId)) {
                            pipInfo.addP1ConfigureResult(pPipelineStaticInfo->openId, sensorId, params.out[0].eIqLevel, params.out[0].eResConfig);
                        }

                        for (auto sensorId : deviceSensorMap.at(anotherDeviceId)) {
                            pipInfo.addP1ConfigureResult(anotherDeviceId, sensorId, params.out[1].eIqLevel, params.out[1].eResConfig);
                        }
                        MY_LOGD("PIP all P1 resource configure done");
                        pipInfo.setPIPConfigureDone();
                    }
                } else {
                    // PIP already configure done
                    E_CamIQLevel quality = eCamIQ_L;
                    CAM_RESCONFIG resConfig;
                    for (const auto& sensorId : pPipelineStaticInfo->sensorId) {
                        pipInfo.getP1ConfigureResult(pPipelineStaticInfo->openId, anotherDeviceId, sensorId, quality, resConfig);
                        MY_LOGD("already configure done, sensor id : %d, quality : %d", sensorId, (int)quality);
                        (*pvOut).at(sensorId).ispQuality = quality;
                        (*pvOut).at(sensorId).camResConfig = resConfig;
                    }
                }
            }
        }
        return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

