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

#define LOG_TAG "mtkcam-P1HwSettingPolicy"

#include <mtkcam3/pipeline/policy/IConfigP1HwSettingPolicy.h>
//
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/aaa/IIspMgr.h>
#include <mtkcam/aaa/IHalISP.h>
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


/******************************************************************************
 *
 ******************************************************************************/
static int32_t evaluateDeviceRawBitDepth(const PipelineStaticInfo* pPipelineStaticInfo __attribute__((unused)))
{
    //Shouldn't be here.
    MY_LOGW("By default bitdepth:10");
    return 10;
}

/******************************************************************************
 *
 ******************************************************************************/
static bool evaluateImgoAllocConfig_useUnpackRaw(const PipelineStaticInfo* pPipelineStaticInfo)
{
    static bool isLowMem = ::property_get_bool("ro.config.low_ram", false);
    if ( isLowMem ) {
        return false; // false: pack raw.
    }

    auto openId = pPipelineStaticInfo->openId;

    std::string key = "persist.vendor.camera3.imgo.alloc.unpackraw.device" + std::to_string(openId);
    auto ret = property_get_int32(key.c_str(), -1);
    if ( -1 != ret ) {
        MY_LOGI("%s=%d", key.c_str(), ret);
        return (ret > 0); // true: unpack raw; false: pack raw.
    }

    return false; // false: pack raw.
}

/******************************************************************************
 * query Resize Max ratio from Isp mgr for quality-guaranteed
 ******************************************************************************/
static bool queryRecommendResizeMaxRatio(const MINT32 sensorId, MUINT32& recommendResizeRatio)
{
    // query resize quality-guaranteed ratio from isp mgr
    MUINT32 ispMaxResizeRatio = 0;
    MUINT32 ispMaxResizeDenominator = 0;
    NS3Av3::Buffer_Info tuningInfo;
    auto mpISP = MAKE_HalISP(sensorId, LOG_TAG);
    if (mpISP == nullptr) {
        MY_LOGA("sensorId : %d, create HalIsp FAILED!", sensorId);
        return false;
    } else {
        // [Work Around] use this function call to check platform
        if (mpISP->queryISPBufferInfo(tuningInfo)) {
            MINT32 ret = mpISP->sendIspCtrl(NS3Av3::EISPCtrl_GetMaxRrzRatio,
                reinterpret_cast<MINTPTR>(&ispMaxResizeDenominator), reinterpret_cast<MINTPTR>(nullptr));
            if (ret != 0) {
                MY_LOGW("EISPCtrl_GetMaxRrzRatio is not supported !, Skip..");
            }
        } else if (auto ispMgr = MAKE_IspMgr()) {
            ispMaxResizeDenominator = ispMgr->queryRRZ_MaxRatio();
        } else {
            MY_LOGA("create IspMgr FAILED!");
            return false;
        }
        mpISP->destroyInstance(LOG_TAG);
    }
    if (ispMaxResizeDenominator) {
        // The precision is 0.01
        ispMaxResizeRatio = (10000L/ispMaxResizeDenominator + 50)/ 100;
        recommendResizeRatio = ispMaxResizeRatio;
        MY_LOGD("Resize-Ratio-Percentage %u for isp quality", recommendResizeRatio);
    }

    return true;
}


static bool configP1HwSetting(
    std::vector<int32_t> const& sensorId,
    Configuration_P1HwSetting_Params const& params
) {
    auto pvOut = params.pvOut;
    auto pSensorSetting = params.pSensorSetting;
    //
    auto pStreamingFeatureSetting = params.pStreamingFeatureSetting;
    auto pSeamlessSwitchFeatureSetting = params.pSeamlessSwitchFeatureSetting;
    auto pPipelineNodesNeed = params.pPipelineNodesNeed;
    auto pPipelineStaticInfo = params.pPipelineStaticInfo;
    auto needP1Node = params.pPipelineNodesNeed->needP1Node;

    auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;
    auto const& pParsedAppConfiguration   = pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo = pPipelineUserConfiguration->pParsedAppImageStreamInfo;

    int32_t bitdepth = evaluateDeviceRawBitDepth(pPipelineStaticInfo);
    for (size_t idx = 0; idx < sensorId.size(); idx++)
    {
        if (needP1Node.count(sensorId[idx]) == 0) {
            continue;
        }
        else if (!needP1Node.at(sensorId[idx])){
            continue;
        }

        P1HwSetting setting;
        std::shared_ptr<NSCamHW::HwInfoHelper> infohelper = std::make_shared<NSCamHW::HwInfoHelper>(sensorId[idx]);
        bool ret = infohelper != nullptr
                && infohelper->updateInfos()
                && infohelper->queryPixelMode( (*pSensorSetting).at(sensorId[idx]).sensorMode, (*pSensorSetting).at(sensorId[idx]).sensorFps, setting.pixelMode)
                ;
        if ( CC_UNLIKELY(!ret) ) {
            MY_LOGE("idx : %zu, queryPixelMode error", idx);
            return UNKNOWN_ERROR;
        }
        MSize const& sensorSize = (*pSensorSetting).at(sensorId[idx]).sensorSize;

        //#define MIN_RRZO_RATIO_100X     (25)
        MINT32     rrzoMaxWidth = 0;
        infohelper->quertMaxRrzoWidth(rrzoMaxWidth);

        #define CHECK_TARGET_SIZE(_msg_, _size_) \
        MY_LOGD("%s: target size(%dx%d)", _msg_, _size_.w, _size_.h);

        #define ALIGN16(x) x = (((x) + 15) & ~(15));

        // estimate preview yuv max size
        MSize const max_preview_size = refine::align_2(
                MSize(rrzoMaxWidth, rrzoMaxWidth * sensorSize.h / sensorSize.w));
        //
        CHECK_TARGET_SIZE("max_rrzo_size", max_preview_size);
        MSize maxYuvStreamSize(0, 0);
        MSize largeYuvStreamSize(0, 0);
        MSize maxStreamingSize(0, 0);

        #define PRV_MAX_DURATION (34000000) //ns
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
        {
            MSize const streamSize = (n.second)->getImgSize();
            auto search = pPipelineUserConfiguration->vMinFrameDuration.find((n.second)->getStreamId());
            int64_t MinFrameDuration = 0;
            if (search != pPipelineUserConfiguration->vMinFrameDuration.end())
            {
                MinFrameDuration = search->second;
            }
            /*if ((n.second)->getUsageForConsumer() & (GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER))
            {
                refine::not_smaller_than(maxStreamingSize, streamSize);
            }*/
            // if stream's size is suitable to use rrzo
            if( streamSize.w <= max_preview_size.w && streamSize.h <= max_preview_size.h && MinFrameDuration <= PRV_MAX_DURATION)
                refine::not_smaller_than(maxYuvStreamSize, streamSize);
            else
                refine::not_smaller_than(largeYuvStreamSize, streamSize);
        }

        /*CHECK_TARGET_SIZE("stream size with hw usage", maxStreamingSize);
        if (maxStreamingSize.size() != 0)
        {
            maxYuvStreamSize = maxStreamingSize;
        }*/

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
                target_rrzo_size.h = target_rrzo_size.w * pStreamingFeatureSetting->rrzoHeightToWidth;
            }
            else
            {
                // align sensor aspect ratio
                if (target_rrzo_size.w * sensorSize.h > target_rrzo_size.h * sensorSize.w)
                {
                    target_rrzo_size.h = target_rrzo_size.w * sensorSize.h / sensorSize.w;
                }
                else
                {
                    target_rrzo_size.w = target_rrzo_size.h * sensorSize.w / sensorSize.h;
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
                if ( pStreamingFeatureSetting->bNeedLMV )
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
                /*MSize const min_rrzo_hw_size = refine::align_2(
                        MSize(sensorSize.w*MIN_RRZO_RATIO_100X/100, sensorSize.h*MIN_RRZO_RATIO_100X/100) );
                refine::not_smaller_than(target_rrzo_size, min_rrzo_hw_size);
                CHECK_TARGET_SIZE("rrz hw lower bound limitation", target_rrzo_size);*/
            }
            //  2. upper bounds
            {
                {
                    refine::not_larger_than(target_rrzo_size, max_preview_size);
                    CHECK_TARGET_SIZE("preview upper bound limitation", target_rrzo_size);
                }
            }
            MY_LOGD("rrzo size(%dx%d)", target_rrzo_size.w, target_rrzo_size.h);
            //
            // check hw limitation with pixel mode & stride
            MSize   rrzoSize;
            int32_t rrzoFormat = 0;
            size_t  rrzoStride = 0;
            uint32_t recommendResizeRatio = 0;
            ALIGN16(target_rrzo_size.w);
            ALIGN16(target_rrzo_size.h);
            refine::not_larger_than(target_rrzo_size, sensorSize);
            CHECK_TARGET_SIZE("sensor size upper bound limitation", target_rrzo_size);
            rrzoSize = target_rrzo_size;
            CAM_LOGD_IF(1, "aligned rrzo size(%dx%d)", target_rrzo_size.w, target_rrzo_size.h);
            if( ! infohelper->getRrzoFmt(bitdepth, rrzoFormat) ||
                ! queryRecommendResizeMaxRatio(sensorId[idx], recommendResizeRatio) ||
                ! infohelper->alignRrzoHwLimitation(target_rrzo_size, sensorSize, rrzoSize, recommendResizeRatio) ||
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
        // 5. PDENode needs full raw if NOT dualcam
        // 6. have capture node
        // 7. have mipi raw
        bool useImgo =
            (pParsedAppImageStreamInfo->pAppImage_Jpeg.get() /*&& ! mPipelineConfigParams.mbHasRecording*/) ||
            !pParsedAppImageStreamInfo->vAppImage_Output_RAW16.empty() ||
            !pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.empty() ||
            pParsedAppImageStreamInfo->pAppImage_Input_Yuv.get() ||
            !!largeYuvStreamSize ||
            !pParsedAppImageStreamInfo->vAppImage_Output_RAW10.empty() ||
            !pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical.empty() ||
            pParsedAppConfiguration->isType3PDSensorWithoutPDE ||
            (pParsedAppImageStreamInfo->secureInfo.type != SecType::mem_normal)  ||
            pPipelineNodesNeed->needP2CaptureNode ||
            property_get_int32("vendor.debug.feature.forceEnableIMGO", 0);

        if( useImgo )
        {
            bool useUFO = (pPipelineNodesNeed->needRaw16Node ||
                pParsedAppImageStreamInfo->pAppImage_Output_Priv.get() ||
                pParsedAppImageStreamInfo->pAppImage_Input_Priv.get() ||
                pParsedAppImageStreamInfo->pAppImage_Input_Yuv.get()) ? false : true;

            bool useMipiRaw10 = ( pParsedAppImageStreamInfo->vAppImage_Output_RAW10.size() ||
                                  pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical.size() )
                                  ? true : false;

            // Config check hw limitation with pixel mode & stride
            MSize   imgoConfigSize = sensorSize;
            int32_t imgoConfigFormat = 0;
            size_t  imgoConfigStride = 0;
            MSize   imgoSize = sensorSize;
            int32_t imgoFormat = 0;
            size_t  imgoStride = 0;
            MSize   imgoSeamlessFullSize = MSize();

            /*
             * Seamless Switch: decide imgo for seamless
             *   1. Seamless remosaic capture : imgo size for full mode. (pool/run-time)
             *   2. Seamless preview : imgo size for crop mode. (share with default imgo)
             */

            // isMain1SeamlessDefaultMode return true if it is main 1 sensor &&
            // main 1 sensor is in seamless default mode; otherwise, return false.
            // when doing remosaic reconfig capture, no need to update seamless related hw setting.
            auto isMain1SeamlessDefaultMode = [&pSensorSetting, &sensorId, &pSeamlessSwitchFeatureSetting]
                    (const size_t idex) -> bool {
                if (pSeamlessSwitchFeatureSetting->defaultScene < 0) {
                    MY_LOGE("cannot get seamless default scene");
                    return false;
                }
                const uint32_t defaultMode =
                        static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->defaultScene);
                if (idex == 0) {
                    const auto main1SensorId = sensorId[idex];
                    const auto iter = (*pSensorSetting).find(main1SensorId);
                    if (iter != (*pSensorSetting).end() &&
                            iter->second.sensorMode == defaultMode) {
                        return true;
                    }
                    return false;
                }
                return false;
            }(idx);
            setting.imgoSeamlessRequest.imageSize = MSize(0, 0);
            if (pSeamlessSwitchFeatureSetting->isSeamlessEnable && isMain1SeamlessDefaultMode) {
                if (pSeamlessSwitchFeatureSetting->isSeamlessCaptureEnable) {
                    // full mode : need full size imgo
                    MY_LOGE_IF(pSeamlessSwitchFeatureSetting->fullScene < 0,
                        "Seamless Switch: [Config] cannot get full mode");
                    const uint32_t fullMode = static_cast<uint32_t>(pSeamlessSwitchFeatureSetting->fullScene);
                    MSize fullSize = MSize(0, 0);
                    infohelper->getSensorSize(fullMode, fullSize);
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
                    infohelper->getSensorSize(cropMode, cropSize);
                    imgoConfigSize = cropSize;
                    imgoSize = cropSize;
                }
            }

            bool useUnpackRaw = evaluateImgoAllocConfig_useUnpackRaw(pPipelineStaticInfo);
            if( ! infohelper->getImgoFmt(bitdepth, imgoConfigFormat, useUFO, useUnpackRaw)
                || ! infohelper->alignPass1HwLimitation(imgoConfigFormat, MTRUE,
                                                      imgoConfigSize, imgoConfigStride ) )
            {
                MY_LOGE("wrong params about imgo");
                return BAD_VALUE;
            }
            // check hw limitation with pixel mode & stride
            if( ! infohelper->getImgoFmt(bitdepth, imgoFormat, useUFO, false, useMipiRaw10)
             || ! infohelper->alignPass1HwLimitation(imgoFormat, MTRUE/*isImgo*/,
                                                     imgoSize, imgoStride ) )
            {
                MY_LOGE("wrong params about imgo");
                return BAD_VALUE;
            }

            //
            bool isLowMem = ::property_get_bool("ro.config.low_ram", false);
            auto& imgoAlloc = setting.imgoAlloc;
            if(isLowMem) {
                imgoAlloc.imageSize = imgoSize;
                imgoAlloc.format = imgoFormat;
            } else {
                imgoAlloc.imageSize = imgoConfigSize;
                imgoAlloc.format = imgoConfigFormat;
            }
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

        // determine rsso size
        if( pStreamingFeatureSetting->bNeedLargeRsso )
        {
            setting.rssoSize = MSize(FSC_MAX_RSSO_WIDTH,512);//add extra 10% margin for FSC crop
        }
        else
        {
            setting.rssoSize = MSize(288,512);
        }
        if ( pPipelineNodesNeed->needFDNode && idx == 0 )
        {
            setting.fdyuvSize.w = 640;
            setting.fdyuvSize.h = (((640 * setting.rrzoDefaultRequest.imageSize.h) / setting.rrzoDefaultRequest.imageSize.w) + 1) & ~(1);
        }
        else
        {
            setting.fdyuvSize = MSize(0,0);
        }

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
        if (pStreamingFeatureSetting->eisInfo.mode != 0)
        {
            MY_LOGD("EIS disable frz");
            setting.camResConfig.Bits.Min_Useful_FRZ_ratio = 100;
        }
        CAM_ULOGMI("%s", toString(setting).c_str());
        (*pvOut).emplace(sensorId[idx], setting);
    }
    return true;
}

/**
 * Make a function target - default version
 */
FunctionType_Configuration_P1HwSettingPolicy makePolicy_Configuration_P1HwSetting_Default()
{
    return [](Configuration_P1HwSetting_Params const& params) -> int {
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;

        configP1HwSetting(pPipelineStaticInfo->sensorId, params);

        auto pvOut = params.pvOut;
        auto pSensorSetting = params.pSensorSetting;
        auto pParsedAppConfiguration = params.pPipelineUserConfiguration->pParsedAppConfiguration;

        // if config SMVR, skip PIP flow
        if (pParsedAppConfiguration->isConstrainedHighSpeedMode ||
            pParsedAppConfiguration->pParsedSMVRBatchInfo != nullptr)
        {
            MY_LOGD("Config SMVR, skip PIP flow");
            return OK;
        }

        int32_t anotherDeviceId = -1;
        auto& pipInfo = PIPInfo::getInstance();
        // vendor tag PIP
        if (pipInfo.getMaxOpenCamDevNum() >= 2) {
            for (auto devId : pipInfo.getOrderedLogicalDeviceId()) {
                if (devId != pPipelineStaticInfo->openId) {
                    anotherDeviceId = devId;
                    break;
                }
            }
        }
        // general PIP
        else {
            // look up table, get another device Id
            auto vLargestConcurrentStreamCameraIds = pipInfo.getLargestConcurrentStreamCameraIds();
            for (auto vStreamCameraIds : vLargestConcurrentStreamCameraIds) {
                MY_LOGD("Concurrent camera id: %d, %d", vStreamCameraIds[0], vStreamCameraIds[1]);
                if (vStreamCameraIds[0] == pPipelineStaticInfo->openId) {
                    anotherDeviceId = vStreamCameraIds[1];
                    break;
                } else if (vStreamCameraIds[1] == pPipelineStaticInfo->openId){
                    anotherDeviceId = vStreamCameraIds[0];
                    break;
                }
            }
        }
        MY_LOGD("another device id: %d", anotherDeviceId);

        // check p1 sync cability
        // If it uses camsv, it has to set usingCamSV flag.
        // It has to group all physic sensor to one and sent to p1 query function.
        // PIP
        if (pipInfo.getPIPConfigureDone() == false && anotherDeviceId != -1)
        {
            int subSensorId = pipInfo.generateSensorSetting(anotherDeviceId);
            if (subSensorId == -1) return OK;

            MY_LOGD("check hw resource");
            {
                // camsv check
                sCAM_QUERY_HW_RES_MGR queryHwRes;
                SEN_INFO sen_info_main, sen_info_sub;
                // 1. prepare sensor information and push to QueryInput queue.
                sen_info_main.sensorIdx = pPipelineStaticInfo->sensorId[0];
                sen_info_main.rrz_out_w = (*pvOut).at(pPipelineStaticInfo->sensorId[0]).rrzoDefaultRequest.imageSize.w;
                queryHwRes.QueryInput.push_back(sen_info_main);

                sen_info_sub.sensorIdx = subSensorId;
                sen_info_sub.rrz_out_w = pipInfo.queryRrzoWidth(anotherDeviceId);
                MY_LOGD("subSensor rrzo width(%d)", sen_info_sub.rrz_out_w);
                queryHwRes.QueryInput.push_back(sen_info_sub);

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
                for(size_t i=0;i<queryHwRes.QueryOutput.size();++i)
                {
                    MY_LOGA_IF((EPipeSelect_None == queryHwRes.QueryOutput[i].pipeSel),
                                "Hw resource overspec");
                    if(EPipeSelect_NormalSv == queryHwRes.QueryOutput[i].pipeSel)
                    {
                        MY_LOGW("DevId[%d] will use camsv.", queryHwRes.QueryOutput[i].sensorIdx);
                    }
                }
            }

            // check isp quality
            auto sensorId = pPipelineStaticInfo->sensorId[0];
            auto infohelper = IHwInfoHelperManager::get()->getHwInfoHelper(sensorId);
            IHwInfoHelper::QueryP1DrvIspParams params;
            MSize subSensorRrzoSize = MSize(1920, 1440); // hard code
            MUINT32 subSensorMode = 0;
            MUINT32 subSensorFps = 30;
            // 1. select max imgo size from second element to end.
            // currently, hard code to set sub sensor info
            // TODO : need to query by PIPInfo
            pipInfo.querySubSensorInfo(anotherDeviceId, subSensorId, subSensorMode, subSensorFps);
            /*
            subSensorIndex = 1;
            subSensorWidth = 2880;
            subSensorMode = 0;
            subSensorFps = 30;
            */
            // 2. set data.
            params.in.push_back(IHwInfoHelper::QueryP1DrvIspParams::CameraInputParams{
                .sensorId = sensorId,
                .sensorMode = (&(*pSensorSetting).at(sensorId))->sensorMode,
                .minProcessingFps = (&(*pSensorSetting).at(sensorId))->sensorFps,
                .rrzoImgSize = (*pvOut).at(sensorId).rrzoDefaultRequest.imageSize,
                // disable frz of one cam in PIP
                .Min_Useful_FRZ_ratio = 100,
            });

            params.in.push_back(IHwInfoHelper::QueryP1DrvIspParams::CameraInputParams{
                .sensorId = subSensorId,
                .sensorMode = subSensorMode,
                .minProcessingFps = (subSensorFps < 30 ? subSensorFps : 30),
                .rrzoImgSize = subSensorRrzoSize,
            });
            MY_LOGD("Query set: index_0(%d) index_1(%d)",
                                sensorId,
                                subSensorId);

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

            (*pvOut).at(sensorId).ispQuality = params.out[0].eIqLevel;
            (*pvOut).at(sensorId).camResConfig = params.out[0].eResConfig;

            pipInfo.addP1ConfigureResult(pPipelineStaticInfo->openId, pPipelineStaticInfo->sensorId[0], params.out[0].eIqLevel, params.out[0].eResConfig);
            pipInfo.addP1ConfigureResult(anotherDeviceId, subSensorId, params.out[1].eIqLevel, params.out[1].eResConfig);

            MY_LOGD("sensor id(%d) using setting(0) ispQuality(%s) tg(%d)",
                    pPipelineStaticInfo->sensorId[0],
                    ISP_QUALITY_TO_STRING(params.out[0].eIqLevel).c_str(),
                    params.out[0].eResConfig.Bits.targetTG);
            MY_LOGD("sensor id(%d) using setting(1) ispQuality(%s) tg(%d)",
                    subSensorId,
                    ISP_QUALITY_TO_STRING(params.out[1].eIqLevel).c_str(),
                    params.out[1].eResConfig.Bits.targetTG);
            MY_LOGD("PIP all P1 resource configure done");
            pipInfo.setPIPConfigureDone();
        }
        else if (anotherDeviceId != -1)
        {
            E_CamIQLevel quality = eCamIQ_L;
            CAM_RESCONFIG resConfig;
            pipInfo.getP1ConfigureResult(pPipelineStaticInfo->openId, anotherDeviceId, pPipelineStaticInfo->sensorId[0], quality, resConfig);
            MY_LOGD("already configure done, sensor id : %d, quality : %d", pPipelineStaticInfo->sensorId[0], (int)quality);
            (*pvOut).at(pPipelineStaticInfo->sensorId[0]).ispQuality = quality;
            (*pvOut).at(pPipelineStaticInfo->sensorId[0]).camResConfig = resConfig;
            pipInfo.set2ndCamConfigDone();
        }
        return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

