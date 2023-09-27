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
 * MediaTek Inc. (C) 2018. All rights reserved.
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
#define LOG_TAG "mtkcam-FeatureSettingPolicy"

#include "FeatureSettingPolicy.h"
//
#include <algorithm>
#include <tuple>
// MTKCAM
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/sys/Cam3CPUCtrl.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

// MTKCAM3
#include <mtkcam3/feature/hdrDetection/Defs.h>
#include <mtkcam3/feature/lmv/lmv_ext.h>
#include <mtkcam3/feature/eis/eis_ext.h>
#include <mtkcam3/feature/3dnr/3dnr_defs.h>
#include <mtkcam3/feature/utils/FeatureProfileHelper.h>
#include <mtkcam3/feature/fsc/fsc_defs.h>
#include <mtkcam3/feature/vhdr/HDRPolicyHelper.h>
#include <mtkcam3/feature/utils/FeatureCache.h>

#include <camera_custom_eis.h>
#include <camera_custom_3dnr.h>
#include <camera_custom_fsc.h>
#include <camera_custom_pq.h>
#include <camera_custom_dsdn.h>

// dual cam
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <camera_custom_stereo.h>
#include <isp_tuning.h>
#include <mtkcam/aaa/aaa_hal_common.h>
//
#include <mtkcam/utils/hw/IScenarioControlV3.h>
//
#include <mtkcam/utils/feature/PIPInfo.h>

#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FEATURE_SETTING_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::v3::pipeline::policy::featuresetting;
using namespace NSCam::v3::pipeline::policy::scenariomgr;
using namespace NSCam::v3::pipeline::model;
using namespace NS3Av3; //IHal3A
using namespace NSCamHW; //HwInfoHelper
using namespace NSCam::NR3D;
using namespace NSCam::FSC;
using namespace NSCam::Feature;
using namespace NSCam::v3::pipeline::model;
using namespace NSCam::plugin::streaminfo;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)

#define SENSOR_INDEX_MASTER   (0)
#define DEBUG_EISEM  (0)
#define DEBUG_EIS30  (0)
#define DEBUG_EIS_DISABLE (0)
#ifndef NR3D_SUPPORTED
#define FORCE_3DNR   (0)
#else
#define FORCE_3DNR   (1)
#endif
#define DEBUG_TSQ    (2)
#define DEFAULT_HDR10_SPEC (0)

#define DISPLAY_MIN_DURATION_NS (33333333)


#define KEY_FSP_STREAMING_CUSTOM_DISABLE "vendor.debug.featuresetting.custom.disable"

static inline std::pair<NSCam::v3::StreamId_T, MSize> max(const std::pair<NSCam::v3::StreamId_T, MSize> &lhs, const std::pair<NSCam::v3::StreamId_T, MSize> &rhs)
{
    return (lhs.second.size() > rhs.second.size()) ? lhs : rhs;
}
static inline std::pair<NSCam::v3::StreamId_T, MSize> min(const std::pair<NSCam::v3::StreamId_T, MSize> &lhs, const std::pair<NSCam::v3::StreamId_T, MSize> &rhs)
{
    return (lhs.second.size() < rhs.second.size() && lhs.second.size() != 0) ? lhs : rhs;
}
static inline std::pair<NSCam::v3::StreamId_T, MSize> toPair(const sp<NSCam::v3::IImageStreamInfo> pStreamInfo)
{
    return std::make_pair(pStreamInfo->getStreamId(), pStreamInfo->getImgSize());
}

auto
FeatureSettingPolicy::
updateStreamData(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool
{
    CAM_ULOGM_APILIFE();

    MY_LOGD_IF(mbDebug, "master sensorId=%d", mReqMasterId);

    IMetadata const* pAppMetaControl = in->pRequest_AppControl;
    auto pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;

    MetadataPtr pOutMetaHal = std::make_shared<IMetadata>();
    MetadataPtr pOutMetaApp = std::make_shared<IMetadata>();

    // APP mode
    int32_t recordState = -1;
    uint32_t AppMode = 0;
    if( IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_STREAMING_FEATURE_RECORD_STATE, recordState) )
    {   // App has set recordState Tag
        if( recordState == MTK_STREAMING_FEATURE_RECORD_STATE_PREVIEW )
        {
            if( in->pRequest_AppImageStreamInfo->hasVideoConsumer )
                AppMode = MTK_FEATUREPIPE_VIDEO_STOP;
            else
                AppMode = MTK_FEATUREPIPE_VIDEO_PREVIEW;
        }
        else if (recordState == MTK_STREAMING_FEATURE_RECORD_STATE_RECORD &&
                 in->pRequest_AppImageStreamInfo->hasVideoConsumer)
        {
            AppMode = MTK_FEATUREPIPE_VIDEO_RECORD;
        }
        else
        {
            AppMode = mConfigOutputParams.StreamingParams.mLastAppInfo.appMode;
            MY_LOGW("Unknown or Not Supported app recordState(%d), use last appMode=%d",
                    recordState, mConfigOutputParams.StreamingParams.mLastAppInfo.appMode);
        }
    }
    else
    {   // App has NOT set recordState Tag
        if( in->pRequest_AppImageStreamInfo->hasVideoConsumer )
            AppMode = MTK_FEATUREPIPE_VIDEO_RECORD;
        else if( in->Configuration_HasRecording )
            AppMode = MTK_FEATUREPIPE_VIDEO_PREVIEW;
        else
            AppMode = MTK_FEATUREPIPE_PHOTO_PREVIEW;
    }
    IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_FEATUREPIPE_APP_MODE, AppMode);

    // EIS
    auto isEISOn = [&] (void) -> bool
    {
        MUINT8 appEisMode = 0;
        MINT32 advEisMode = 0;
        if( ( IMetadata::getEntry<MUINT8>(pAppMetaControl, MTK_CONTROL_VIDEO_STABILIZATION_MODE, appEisMode) &&
              appEisMode == MTK_CONTROL_VIDEO_STABILIZATION_MODE_ON ) ||
            ( IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_EIS_FEATURE_EIS_MODE, advEisMode) &&
                 advEisMode == MTK_EIS_FEATURE_EIS_MODE_ON ) )
        {
            return true;
        }
        else
        {
            return false;
        }
    };
    if( mConfigOutputParams.StreamingParams.bEnableTSQ )
    {
        //MBOOL needOverrideTime = isEisQEnabled(pipelineParam.currentAdvSetting);
        // Now change to use Gralloc Extra to set timestamp
        IMetadata::setEntry<MBOOL>(pOutMetaHal.get(), MTK_EIS_NEED_OVERRIDE_TIMESTAMP, 1);
        //MY_LOGD("TSQ ON");
    }

    if( AppMode != mConfigOutputParams.StreamingParams.mLastAppInfo.appMode ||
        recordState != mConfigOutputParams.StreamingParams.mLastAppInfo.recordState ||
        isEISOn() != mConfigOutputParams.StreamingParams.mLastAppInfo.eisOn )
    {
        MY_LOGI("AppInfo changed:appMode(%d=>%d),recordState(%d=>%d),eisOn(%d=>%d)",
                mConfigOutputParams.StreamingParams.mLastAppInfo.appMode, AppMode,
                mConfigOutputParams.StreamingParams.mLastAppInfo.recordState, recordState,
                mConfigOutputParams.StreamingParams.mLastAppInfo.eisOn, isEISOn());
        mConfigOutputParams.StreamingParams.mLastAppInfo.appMode = AppMode;
        mConfigOutputParams.StreamingParams.mLastAppInfo.recordState = recordState;
        mConfigOutputParams.StreamingParams.mLastAppInfo.eisOn = isEISOn();
    }

    if (mConfigOutputParams.StreamingParams.reqFps == 30) {
        out->CPUProfile = Cam3CPUCtrl::E_CAM3_CPU_CTRL_FPSGO_CPC;
    }

    // update metadata to all streaming sensors in main frame
    auto sensorList = mReqSubSensorIdList;
    sensorList.push_back(mReqMasterId);
    for( const auto &sensorId : sensorList ) {
        updateRequestResultParams_Streaming(
            in,
            out->mainFrame,
            pOutMetaApp,
            pOutMetaHal,
            0 /* will update in sensor-specifc */,
            sensorId);
    }

    return true;
}

auto
FeatureSettingPolicy::
updateStreamData_Sensor(
    RequestOutputParams* out,
    RequestInputParams const* in,
    uint32_t sensorId
) -> bool
{
    CAM_ULOGM_APILIFE();

    auto pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;
    IMetadata const* pAppMetaControl = in->pRequest_AppControl;

    MetadataPtr pOutMetaHal = std::make_shared<IMetadata>();
    MetadataPtr pOutMetaApp = std::make_shared<IMetadata>();
    uint32_t needP1Dma = 0;
    if( !getStreamP1DmaConfig(needP1Dma, in, sensorId) ) {
        MY_LOGE("[getStreamP1DmaConfig] cam:%d main P1Dma output is invalid: 0x%X", sensorId, needP1Dma);
    }

    // SMVR
    if( pParsedSMVRBatchInfo != nullptr ) // SMVRBatch
    {
        IMetadata::IEntry const& entry = pAppMetaControl->entryFor(MTK_CONTROL_AE_TARGET_FPS_RANGE);
        if( entry.isEmpty() )
        {
            MY_LOGW("SMVRBatch: no MTK_CONTROL_AE_TARGET_FPS_RANGE");
        }
        else
        {
            MINT32 i4MinFps = entry.itemAt(0, Type2Type< MINT32 >());
            MINT32 i4MaxFps = entry.itemAt(1, Type2Type< MINT32 >());
            MUINT8 fps = i4MinFps == 30  ? MTK_SMVR_FPS_30 :
                         i4MinFps == 120 ? MTK_SMVR_FPS_120 :
                         i4MinFps == 240 ? MTK_SMVR_FPS_240 :
                         i4MinFps == 480 ? MTK_SMVR_FPS_480 :
                         i4MinFps == 960 ? MTK_SMVR_FPS_960 : MTK_SMVR_FPS_30;

            IMetadata::setEntry<MUINT8>(pOutMetaHal.get(), MTK_HAL_REQUEST_SMVR_FPS, fps);

            MY_LOGD_IF(2 <= pParsedSMVRBatchInfo->logLevel,
                "SMVRBatch(%d): requestNo=%d, AppMode=%d, i4MinFps=%d, i4MaxFps=%d, SMVR_FPS=%d",
                sensorId, in->requestNo, mConfigOutputParams.StreamingParams.mLastAppInfo.appMode, i4MinFps, i4MaxFps, fps);
        }
    }
    else if( mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->isConstrainedHighSpeedMode )
    {
        // SMVRConstraint
        IMetadata::IEntry const& entry = pAppMetaControl->entryFor(MTK_CONTROL_AE_TARGET_FPS_RANGE);
        if( entry.isEmpty() ) {
            MY_LOGW("SMVRConstraint: no MTK_CONTROL_AE_TARGET_FPS_RANGE");
        }
        else {
            MINT32 i4MinFps = entry.itemAt(0, Type2Type< MINT32 >());
            MINT32 i4MaxFps = entry.itemAt(1, Type2Type< MINT32 >());
            MUINT8 fps = i4MinFps == 30  ? MTK_SMVR_FPS_30 :
                         i4MinFps == 120 ? MTK_SMVR_FPS_120 :
                         i4MinFps == 240 ? MTK_SMVR_FPS_240 :
                         i4MinFps == 480 ? MTK_SMVR_FPS_480 :
                         i4MinFps == 960 ? MTK_SMVR_FPS_960 : MTK_SMVR_FPS_30;

            IMetadata::setEntry<MUINT8>(pOutMetaHal.get(), MTK_HAL_REQUEST_SMVR_FPS, fps);

            MY_LOGD_IF(2 <= mbDebug,
                "SMVRConstraint(%d): requestNo=%d, AppMode=%d, i4MinFps=%d, i4MaxFps=%d, SMVR_FPS=%d",
                sensorId, in->requestNo, mConfigOutputParams.StreamingParams.mLastAppInfo.appMode, i4MinFps, i4MaxFps, fps);
        }
    }
    else
    {
        MY_LOGD_IF(2 <= mbDebug, "cam:%d No need to update SMVRBatch or SMVRConstraint fps policy", sensorId);
    }

    // HDR
    MINT32 appHDRMode = mForcedVHDRMode ? mForcedVHDRMode : 0;
    if( appHDRMode ||
        (in->Configuration_HasRecording &&
        IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_HDR_FEATURE_HDR_MODE, appHDRMode)) ) {
        mHDRHelper.at(sensorId)->updateAppRequestMode(static_cast<HDRMode>((uint8_t)appHDRMode),
                                         (mConfigOutputParams.StreamingParams.mLastAppInfo.appMode == MTK_FEATUREPIPE_VIDEO_RECORD ||
                                          mConfigOutputParams.StreamingParams.mLastAppInfo.appMode == MTK_FEATUREPIPE_VIDEO_STOP ) ?
                                         HDRPolicyHelper::REQUEST_RECORD : HDRPolicyHelper::REQUEST_STREAMING);
        if (mHDRHelper.at(sensorId)->isConfigHdrOff() &&
            (mHDRHelper.at(sensorId)->getHDRHalMode() & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER)) {
            MY_LOGD("set MTK_HDR_RECONFIG_FROM_BEGIN");
            IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_HDR_RECONFIG_FROM_BEGIN, 1);
        }
    }
    if( mHDRHelper.at(sensorId)->isHDR() ) {
        IMetadata::setEntry<MUINT8>(pOutMetaHal.get(), MTK_3A_HDR_MODE, static_cast<MUINT8>(mHDRHelper.at(sensorId)->getHDRAppMode()));
        MINT32 reqHdrHalMode = static_cast<MINT32>(mHDRHelper.at(sensorId)->getHDRHalRequestMode());
        IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_HDR_FEATURE_HDR_HAL_MODE, reqHdrHalMode);
        IMetadata::setEntry<MINT32>(pOutMetaApp.get(), MTK_HDR_FEATURE_VHDR_RESULT, reqHdrHalMode);
        if( mHDRHelper.at(sensorId)->isMulitFrameHDR() ) {
            IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_3A_FEATURE_AE_EXPOSURE_LEVEL,
                                        mHDRHelper.at(sensorId)->getHDRExposure(FRAME_TYPE_MAIN));
            out->CPUProfile = Cam3CPUCtrl::E_CAM3_CPU_CTRL_FPSGO_HIGH;
        }
    }

    MINT32 aeTargetMode = mHDRHelper.at(sensorId)->getAETargetMode();
    MINT32 validExpNum = mHDRHelper.at(sensorId)->getValidExposureNum();
    MINT32 fusionNum = mHDRHelper.at(sensorId)->getFusionNum(FRAME_TYPE_MAIN);
    if (mHDRHelper.at(sensorId)->getDebugLevel()) {
      MINT32 debugExpNum = ::property_get_int32("vendor.debug.stagger.expnum", 0);
      if (debugExpNum != 0) {
        switch (debugExpNum) {
        case 1:
            aeTargetMode = MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL;
            validExpNum = MTK_STAGGER_VALID_EXPOSURE_1;
            fusionNum = MTK_3A_ISP_FUS_NUM_1;
            break;
        case 2:
            aeTargetMode = MTK_3A_FEATURE_AE_TARGET_MODE_STAGGER_2EXP;
            validExpNum = MTK_STAGGER_VALID_EXPOSURE_2;
            fusionNum = MTK_3A_ISP_FUS_NUM_2;
            break;
        case 3:
            aeTargetMode = MTK_3A_FEATURE_AE_TARGET_MODE_STAGGER_3EXP;
            validExpNum = MTK_STAGGER_VALID_EXPOSURE_3;
            fusionNum = MTK_3A_ISP_FUS_NUM_3;
            break;
        default:
            MY_LOGE("exposNum %d not supported", debugExpNum);
        }
        MY_LOGD("[Stagger debug] debugExpNum = %d", debugExpNum);
      }
    }

    IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_3A_FEATURE_AE_TARGET_MODE, aeTargetMode);
    IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM, validExpNum);
    IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_3A_ISP_FUS_NUM, fusionNum);
    uint32_t requestAeTargetMode = 0, requestExposureNum = 0;
    mHDRHelper.at(sensorId)->getSeamlessSmoothResult(FRAME_TYPE_MAIN, requestAeTargetMode, requestExposureNum);
    IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_3A_FEATURE_AE_REQUEST_TARGET_MODE, requestAeTargetMode);
    IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_3A_FEATURE_AE_REQUEST_EXPOSURE_NUM, requestExposureNum);

    MY_LOGD_IF(mHDRHelper.at(sensorId)->getDebugLevel(),
        "isHDR()=%d, hdrHalMode=%d, (REQ aeTargetMode,REQ validExposureNum)=(%d,%d), (aeTargetMode,validExposureNum,fusNum)=(%d,%d,%d)",
        mHDRHelper.at(sensorId)->isHDR(), mHDRHelper.at(sensorId)->getHDRHalRequestMode(),
        requestAeTargetMode, requestExposureNum,
        mHDRHelper.at(sensorId)->getAETargetMode(),
        mHDRHelper.at(sensorId)->getValidExposureNum(),
        mHDRHelper.at(sensorId)->getFusionNum(FRAME_TYPE_MAIN));

    // DSDN
    MSize cfgRrzoSize;
    if( (in->pConfiguration_StreamInfo_P1->at(sensorId).pHalImage_P1_Rrzo != nullptr) ) {
        cfgRrzoSize = in->pConfiguration_StreamInfo_P1->at(sensorId).pHalImage_P1_Rrzo->getImgSize();
    }
    else {
        MY_LOGW("Suspious: no RRZO, set fixedRRZOSize(%dx%d) => no fix rrzo", cfgRrzoSize.w, cfgRrzoSize.h);
    }
    MSize finalRrzoSize = cfgRrzoSize;
    DSDNCache::Data dsdnData = DSDNCache::getInstance()->getData();
    if( dsdnData.p1Control && dsdnData.run )
    {
        finalRrzoSize = dsdnData.resizeP1(cfgRrzoSize);
        IMetadata::setEntry<MSize>(pOutMetaHal.get(), MTK_P1NODE_RESIZER_SET_SIZE, finalRrzoSize);
        IMetadata::setEntry<MBOOL>(pOutMetaHal.get(), MTK_P2NODE_DSDN_ENABLE, MTRUE);
    }
    // if (isEISOn() || mConfigOutputParams.StreamingParams.targetRrzoRatio > 1.0f)
    {
        out->fixedRRZOSize.emplace(sensorId, finalRrzoSize);
    }
    MY_LOGD_IF(mbDebug, "cam:%d reqNo(%d) DSDN Cache (run/p1Control/m/d)=(%d/%d/%d/%d), finalRrz(%dx%d)",
                    sensorId, in->requestNo,
                    dsdnData.run, dsdnData.p1Control, dsdnData.p1RatioMultiple, dsdnData.p1RatioDivider,
                    out->fixedRRZOSize.at(sensorId).w, out->fixedRRZOSize.at(sensorId).h);

    // Profile
    MINT32 fMask = ProfileParam::FMASK_NONE;
    if( mConfigOutputParams.StreamingParams.mLastAppInfo.eisOn ) {
        fMask |= ProfileParam::FMASK_EIS_ON;
    }
    if( mConfigOutputParams.StreamingParams.bIsHdr10 ) {
        fMask |= ProfileParam::FMASK_HDR10_ON;
    }
    if( mHDRHelper.at(sensorId)->isHDR() ) {
        fMask |= ProfileParam::FMASK_VIDEO_HDR_ON;
    }
    MINT32 fFlag = ProfileParam::FLAG_NONE;
    if( mConfigOutputParams.StreamingParams.mLastAppInfo.appMode == MTK_FEATUREPIPE_VIDEO_RECORD ) {
        fFlag |= ProfileParam::FLAG_RECORDING;
    }
    uint32_t hdrHalmode = mHDRHelper.at(sensorId)->isVideoHDRConfig() ?
                          mHDRHelper.at(sensorId)->getHDRHalMode() :
                          MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
    ProfileParam profileParam(
         finalRrzoSize,//stream size
         hdrHalmode, //hdrHalmode
         0,       //sensormode
         fFlag, // TODO set flag by isZSDPureRawStreaming or not
         fMask
    );
    MUINT8 profile = 0;
    if( !FeatureProfileHelper::getStreamingProf(profile, profileParam) )
    {
        #if (2==HAS_P1YUV)
        // Didn't use FeatureProfileHelper isp Profile, because is not VHDR/EIS feature.
        if(in->Configuration_HasRecording && (miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF) )
        {
            //VSDOF Video Record ISP Profile
            profile = NSIspTuning::EIspProfile_VSDOF_Video_Main_toW;
        }
        #else
        if(in->Configuration_HasRecording)
        {
            profile = NSIspTuning::EIspProfile_Video;
        }
        else {
            profile = NSIspTuning::EIspProfile_Preview;
        }
        #endif
    }
    MY_LOGD_IF(mbDebug, "updateStreamData IspProfile(%d)", profile);
    IMetadata::setEntry<MUINT8>(pOutMetaHal.get(), MTK_3A_ISP_PROFILE, profile);

    // Private data
    appendPrivateDataUpdateFunc_Streaming(out, in, sensorId);

    if (!updateStreamMultiFrame(out, in, pOutMetaApp, pOutMetaHal, sensorId)) {
        MY_LOGE("updateStreamMultiFrame failed");
    }

    updateRequestResultParams_Streaming(
        in,
        out->mainFrame,
        pOutMetaApp,
        pOutMetaHal,
        needP1Dma,
        sensorId);

    return true;
}

auto
FeatureSettingPolicy::
emplaceStreamData(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool {
    CAM_ULOGM_APILIFE();

    static const bool disableCustom = ::property_get_bool(
        KEY_FSP_STREAMING_CUSTOM_DISABLE, 0);
    if (disableCustom) {
        MY_LOGD_IF(1 <= mbDebug, "Force disable customized setting");
        return true;
    }

    // no default strategy that can be customized now
    // apply customized strategy directly
    const CustomStreamData customData =
        extractStreamData_Custom(in);

    for (const auto& pair : customData.mainFrame) {
        auto _sensorId = pair.first;
        auto _frameData = pair.second;
        uint32_t camP1Dma = 0;
        if (!getStreamP1DmaConfig(camP1Dma, in, _sensorId)) {
            MY_LOGE("[getStreamP1DmaConfig] cam:%d P1Dma "
                    "output is invalid: 0x%X", _sensorId, camP1Dma);
        }
        std::shared_ptr<NSCam::IMetadata> outMetaApp = std::make_shared<IMetadata>();
        std::shared_ptr<NSCam::IMetadata> outMetaHal = std::make_shared<IMetadata>();
        if (_frameData.appMetadata.get() != nullptr) {
            *outMetaApp += *_frameData.appMetadata;
        }
        if (_frameData.halMetadata.get() != nullptr) {
            *outMetaHal += *_frameData.halMetadata;
        }

        updateRequestResultParams_Streaming(
            in,
            out->mainFrame,
            outMetaApp,
            outMetaHal,
            camP1Dma,
            _sensorId);
    }
    for (size_t i = 0; i < customData.preDummyFrame.size(); i++ ) {
        for (const auto& pair : customData.preDummyFrame[i]) {
            auto _sensorId = pair.first;
            auto _frameData = pair.second;
            uint32_t camP1Dma = 0;
            if (!getStreamP1DmaConfig(camP1Dma, in, _sensorId)) {
                MY_LOGE("[getStreamP1DmaConfig] cam:%d P1Dma "
                        "output is invalid: 0x%X", _sensorId, camP1Dma);
            }

            std::shared_ptr<NSCam::IMetadata> outMetaApp =
                std::make_shared<IMetadata>();
            std::shared_ptr<NSCam::IMetadata> outMetaHal =
                std::make_shared<IMetadata>();
            /* note that it won't append metadata from main-frame */
            if (_frameData.appMetadata.get() != nullptr) {
                *outMetaApp += *_frameData.appMetadata;
            }
            if (_frameData.halMetadata.get() != nullptr) {
                *outMetaHal += *_frameData.halMetadata;
            }
            std::shared_ptr<RequestResultParams> newFrame = nullptr;
            if (out->preDummyFrames.size() > i) {
                newFrame = out->preDummyFrames[i];  // append
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    camP1Dma,
                    _sensorId);
                out->preDummyFrames[i] = newFrame;
            }
            else {
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    camP1Dma,
                    _sensorId);
                out->preDummyFrames.push_back(newFrame);
            }
        }
    }

    if (customData.cpuProfile != Cam3CPUCtrl::E_CAM3_CPU_CTRL_FPSGO_DISABLE) {
        out->CPUProfile = customData.cpuProfile;
    }
    MY_LOGD_IF(mbDebug>=1, "StreamData CPUProfile(%d)", out->CPUProfile);

    return true;
}

auto
FeatureSettingPolicy::
evaluateStreamSetting(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool
{
    CAM_ULOGM_APILIFE();

    if (in->needP2StreamNode) {
        CAM_TRACE_CALL();
        bool isCaptureUpdate = false;
        decideReqMasterAndSubSensorlist(in, mPolicyParams, isCaptureUpdate);

        if (!in->needP2CaptureNode ||
            in->pRequest_AppImageStreamInfo->hasVideoConsumer) {
            // TODO: only porting some streaming feature, temporary.
            // not yet implement all stream feature setting evaluate
            MY_LOGD_IF(in->needP2CaptureNode,
                       "stream feature setting evaluate "
                       "with capture behavior in video record");

            updateStreamData(out, in);
            if (mPolicyParams.bIsLogicalCam) {
                updateStreamData_Sensor(out, in, mReqMasterId);
                for (const auto& sensorId : mReqSubSensorIdList) {
                    updateStreamData_Sensor(out, in, sensorId);
                }
            }
            else {  //physical
                updateStreamData_Sensor(out, in, mPolicyParams
                    .pPipelineStaticInfo->openId/*physical sensor id*/);
            }
            emplaceStreamData(out, in);
        }
        else {
            /**************************************************************
             * NOTE:
             * In this stage,
             * MTK_3A_ISP_PROFILE and sensor setting has been configured
             * for capture behavior.
             *************************************************************/
            // stream policy with capture policy and behavior.
            // It may occurs some quality limitation duting captue behavior.
            // For example, change sensor mode, 3A sensor setting, P1 ISP profile.
            // Capture+streaming feature combination policy
            // TODO: implement for customized streaming feature setting evaluate with capture behavior
            MY_LOGD("not yet implement for stream feature setting evaluate "
                    "with capture behavior");
        }

        // update dual cam preview
        if( DualDevicePath::MultiCamControl == mDualDevicePath ) {
            if( mMultiCamStreamingUpdater )
            {
                mMultiCamStreamingUpdater(out, in);
            }
            else
            {
                MY_LOGD("mMultiCamStreamingUpdater is nullptr");
            }
        }

        MY_LOGD_IF(2 <= mbDebug, "stream request frames count(mainFrame:%d, subFrames:%zu, preDummyFrames:%zu, postDummyFrames:%zu)",
                (out->mainFrame.get() != nullptr), out->subFrames.size(),
                out->preDummyFrames.size(), out->postDummyFrames.size());
    }
    return true;
}

auto FeatureSettingPolicy::
    getPIPInfo(ParsedPIPInfo &outPIPInfo)
-> void
{
    MBOOL ret = MTRUE;

    auto const& pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pPipelineStaticInfo = mPolicyParams.pPipelineStaticInfo;
    const int32_t currDevId = pPipelineStaticInfo->openId;
    MINT32 previewEisEnable = 0;
    IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams, MTK_EIS_FEATURE_PREVIEW_EIS, previewEisEnable);
    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;

    PIPInfo &mwPIPInfo = PIPInfo::getInstance();
    // copy to avoid thread-safe issue
    std::map<MINT, std::vector<MINT>> devSensorMap = mwPIPInfo.getDeviceSensorMap();
    std::vector<MINT> logicalDevIDs = mwPIPInfo.getOrderedLogicalDeviceId();
    const int32_t numDevSensorMap = devSensorMap.size();
    const int32_t numLogicalDevIDs = logicalDevIDs.size();

    // --- check parameters ---
    if ( numDevSensorMap == 0 || numLogicalDevIDs == 0 ||
         (numDevSensorMap != numLogicalDevIDs) )
    {
        ret = MFALSE;
    }

    // prepare output
    const bool isVendorPIP = (mwPIPInfo.getMaxOpenCamDevNum() != 0);
    const bool debugEnable = ::property_get_int32("vendor.debug.pip.debug", 0);

    outPIPInfo.logLevel = debugEnable ? ::property_get_int32("vendor.debug.pip.logLevel", 0) : 0;
    outPIPInfo.isMulticam = (devSensorMap[currDevId].size() > 1);
    for (int32_t idx = 0; idx < numLogicalDevIDs; idx++ )
    {
        MINT devID = logicalDevIDs[idx];
        if ( devID == currDevId )
        {
            const int32_t openOrder = idx + 1;
            if ( !(pParsedSMVRBatchInfo != nullptr ||
                 pParsedAppConfiguration->operationMode == 1) )
            {
                if ( isVendorPIP )
                {
                    outPIPInfo.PQIdx = (openOrder == 1) ? CUSTOM_PQ_VEN_PIP_1ST :
                                       (openOrder == 2) ? CUSTOM_PQ_VEN_PIP_2ND :
                                                          CUSTOM_PQ_VEN_PIP_OTH ;
                }
                else
                {
                    outPIPInfo.PQIdx = (openOrder == 1) ? CUSTOM_PQ_NORMAL :
                                       (openOrder == 2) ? CUSTOM_PQ_GEN_PIP_2ND :
                                                          CUSTOM_PQ_GEN_PIP_OTH ;
                }
            }
            outPIPInfo.currDevId = currDevId;
            outPIPInfo.openOrder = openOrder;
        }
        else
            outPIPInfo.isOtherMulticam = (devSensorMap[idx].size() > 1);

        MY_LOGD("pip: logicalDevID[%d] Info(devID(%d), #sensor(%zu))", idx, devID, devSensorMap[devID].size());
    }
    outPIPInfo.isVendorPIP = isVendorPIP;
    outPIPInfo.isValid = ret;

    // previewEis: vPIP: if multi: off, gPIP: if (open_2nd+multi): off
    outPIPInfo.isPreviewEisOn = outPIPInfo.isValid &&
        (outPIPInfo.isVendorPIP || outPIPInfo.openOrder >= 2) &&
        !outPIPInfo.isMulticam &&
        previewEisEnable;
    // MV: vPIP: LMV, gPIP: RSC(1st), LMV(2nd)
    outPIPInfo.isRscOn = outPIPInfo.isValid &&
        !outPIPInfo.isVendorPIP && outPIPInfo.openOrder < 2;
    // AISS: vPIP: off, gPIP: on (1st), off (2nd)
    outPIPInfo.isAISSOn = outPIPInfo.isValid &&
        !outPIPInfo.isVendorPIP && outPIPInfo.openOrder < 2 &&
        pParsedAppConfiguration->supportAIShutter;

    MY_LOGD("pip: #devSensorMap(%zu), #LogicalDevID(%zu), isValid(%d), "
        "isVendorPIP(%d), currDevId(%d), openOrder(%d), isMulti(%d), "
        "isOtherMulti(%d), PQIdx(%d), PreviewEisOn(%d,meta(%d)), RSCOn(%d), "
        "isAISSOn(%d), logLevel(%d)",
        devSensorMap.size(), logicalDevIDs.size(), outPIPInfo.isValid,
        outPIPInfo.isVendorPIP, outPIPInfo.currDevId, outPIPInfo.openOrder, outPIPInfo.isMulticam,
        outPIPInfo.isOtherMulticam, outPIPInfo.PQIdx, outPIPInfo.isPreviewEisOn, previewEisEnable,
        outPIPInfo.isRscOn, outPIPInfo.isAISSOn, outPIPInfo.logLevel);
}

auto
FeatureSettingPolicy::
evaluateStreamConfiguration(
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in __unused
) -> bool
{
    CAM_ULOGM_APILIFE();

    extractStreamConfiguration(out, in);
    updateStreamConfiguration(out, in);
    for( const auto& sensorId : mPolicyParams.pPipelineStaticInfo->sensorId ) {
        if( !updateStreamConfiguration_Sensor(out, in, sensorId)) {
            MY_LOGE("evaluateStreamConfiguration_Sensor failed");
        }
    }
    MY_LOGD("out->StreamingParams:%s", policy::toString(out->StreamingParams).c_str());

    return true;
}

auto
FeatureSettingPolicy::
updateStreamConfiguration(
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in __unused
) -> bool
{
    CAM_TRACE_CALL();
    CAM_ULOGM_APILIFE();

    auto const& pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo = mPolicyParams.pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto const& pPipelineStaticInfo = mPolicyParams.pPipelineStaticInfo;
    auto const& pipInfo = out->StreamingParams.pipInfo;
    MINT32 forceTSQ = ::property_get_int32("vendor.debug.camera.hal3.tsq", DEBUG_TSQ);
    MINT32 hdr10Spec = ::property_get_int32("vendor.cam.record.hdr_format", DEFAULT_HDR10_SPEC);
    bool enableHdr10 = false;

    sp<IImageStreamInfo> pStreamInfo;
    std::pair<StreamId_T, MSize> pairPrefDisp(-1, MSize(0,0)), pairDisp(-1, MSize(0,0)), pairRec(-1, MSize(0,0)), pairExtra(-1, MSize(0,0)), pairExtraSlow(-1, MSize(0,0));
    auto pMetadataProvider = NSMetadataProviderManager::valueForByDeviceId(pPipelineStaticInfo->openId);
    IMetadata::IEntry entryMinDuration = pMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_SCALER_AVAILABLE_MIN_FRAME_DURATIONS);

    auto preparePair = [&](android::sp<IImageStreamInfo> imgStreamInfo) {
        MINT64 streamDuration = 0;
        if( (pStreamInfo = imgStreamInfo).get()
            && getStreamDuration(entryMinDuration, pStreamInfo->getOriImgFormat(), pStreamInfo->getImgSize(), streamDuration) )
        {
            if( streamDuration <= DISPLAY_MIN_DURATION_NS && (pStreamInfo->getUsageForConsumer() & GRALLOC_USAGE_HW_COMPOSER) )
            {
                pairPrefDisp = min(pairPrefDisp, toPair(pStreamInfo));
            }
            else if( streamDuration <= DISPLAY_MIN_DURATION_NS && (pStreamInfo->getUsageForConsumer() & GRALLOC_USAGE_HW_TEXTURE) )
            {
                pairDisp = min(pairDisp, toPair(pStreamInfo));
            }
            else if( pStreamInfo->getUsageForConsumer() & GRALLOC_USAGE_HW_VIDEO_ENCODER )
            {
                pairRec = max(pairRec, toPair(pStreamInfo));
            }
            else if( streamDuration <= DISPLAY_MIN_DURATION_NS )
            {
                pairExtra = max(pairExtra, toPair(pStreamInfo));
            }
            else
            {
                pairExtraSlow = max(pairExtraSlow, toPair(pStreamInfo));
            }

            if( pStreamInfo->getImgFormat() == HAL_PIXEL_FORMAT_YCBCR_P010 )
            {
                enableHdr10 = true;
            }
        }

    };

    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
    {
        preparePair(n.second);
    }

    if (pParsedAppImageStreamInfo->pAppImage_Depth != nullptr) {
        preparePair(pParsedAppImageStreamInfo->pAppImage_Depth);
    }

    mMdpTargetSize = pairRec.second.size() ? pairRec.second :
                     pairPrefDisp.second.size() ? pairPrefDisp.second :
                     pairDisp.second.size() ? pairDisp.second :
                     pairExtra.second.size() ? pairExtra.second :
                     pairExtraSlow.second;
    out->StreamingParams.dispStreamId = pairPrefDisp.second.size() ? pairPrefDisp.first :
                                        pairDisp.second.size() ? pairDisp.first :
                                        pairExtra.second.size() ? pairExtra.first :
                                        -1;

    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;

    if( ::needControlMmdvfs() && (pPipelineStaticInfo->isDualDevice) )
    {
        MY_LOGD("vsdof enable bwc control");
        out->StreamingParams.BWCScenario = IScenarioControlV3::Scenario_ContinuousShot;
        FEATURE_CFG_ENABLE_MASK(
                        out->StreamingParams.BWCFeatureFlag,
                        IScenarioControlV3::FEATURE_VSDOF_PREVIEW);
    }
    // query features by scenario during config
    ScenarioFeatures scenarioFeatures;
    StreamingScenarioConfig scenarioConfig;
    ScenarioHint scenarioHint;
    toTPIDualHint(scenarioHint);
    scenarioHint.isVideoMode = pParsedAppImageStreamInfo->hasVideoConsumer;
    scenarioHint.operationMode = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->operationMode;
    //TODO:
    //scenarioHint.captureScenarioIndex = ?   /* hint from vendor tag */
    //scenarioHint.streamingScenarioIndex = ? /* hint from vendor tag */
    int32_t openId = pPipelineStaticInfo->openId;
    auto pAppMetadata = &mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->sessionParams;

    int32_t scenario = -1;
    if( !get_streaming_scenario(scenario, scenarioFeatures, scenarioConfig, scenarioHint, pAppMetadata) ) {
        MY_LOGE("cannot get streaming scenario");
        return false;
    }

    for( const auto featureSet : scenarioFeatures.vFeatureSet ) {
        MY_LOGI("scenario(%s) for (openId:%d, scenario:%d, config: #fixedMargin=%zu, #queue=%d ) support feature:%s(%#" PRIx64"), feature combination:%s(%#" PRIx64")",
                scenarioFeatures.scenarioName.c_str(),
                openId, scenario,
                scenarioConfig.fixedMargins.size(), scenarioConfig.queueCount,
                featureSet.featureName.c_str(),
                static_cast<MINT64>(featureSet.feature),
                featureSet.featureCombinationName.c_str(),
                static_cast<MINT64>(featureSet.featureCombination));
        out->StreamingParams.supportedScenarioFeatures |= (featureSet.feature | featureSet.featureCombination);
    }

    for( auto margin : scenarioConfig.fixedMargins ) {
        if( margin <= 0 || margin > 80 ) {
            MY_LOGE("invalid streaming scenario margin(%d)", margin);
        }
        else {
            out->StreamingParams.targetRrzoRatio *= float(100 + margin) / 100.0f;
            MY_LOGI("add margin (%d)", margin);
        }
    }

    // HDR10
    out->StreamingParams.bIsHdr10 = enableHdr10;
    out->StreamingParams.hdr10Spec = hdr10Spec;

    // ISP6.0 FD
    if( in->isP1DirectFDYUV )
    {
        if( ::property_get_int32("vendor.debug.camera.fd.disable", 0) == 0 )
        {
            out->StreamingParams.bNeedP1FDYUV = 1;
        }
    }

    // ISP6.0 scaled yuv
    if ( miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF && in->numsP1YUV )
    {
        out->StreamingParams.bNeedP1ScaledYUV = 1;
    }

    out->StreamingParams.minRrzoEisW = LMV_MIN_RRZ;
    out->StreamingParams.bEnableGyroMV = ::property_get_int32("ro.vendor.camera.aishutter.support", 0) ? MTRUE : MFALSE;

    // force 4K30 sensor mode match
    if( out->StreamingParams.reqFps == 30 && pParsedAppImageStreamInfo->hasVideo4K )
    {
        out->StreamingParams.fpsFor4K = 30;
        out->StreamingParams.aspectRatioFor4K = MSize(3840, 2160);
    }

    // TPI eis TSQ or RQQ
    if ( forceTSQ == 1 )
    {
        out->StreamingParams.bEnableTSQ = MTRUE;
    }
    else if( forceTSQ == 2 )
    {
        out->StreamingParams.bEnableTSQ = MFALSE;
    }
    if ( !out->StreamingParams.bEnableTSQ && scenarioConfig.queueCount > 0 )
    {
        out->StreamingParams.eisExtraBufNum += scenarioConfig.queueCount;
    }

    // Total margin
    out->StreamingParams.totalMarginRatio = out->StreamingParams.targetRrzoRatio;
    if( out->StreamingParams.eisInfo.factor )
    {
        out->StreamingParams.totalMarginRatio *= (out->StreamingParams.eisInfo.factor / 100.f);
    }
    if( out->StreamingParams.fscMaxMargin )
    {
        out->StreamingParams.totalMarginRatio *= (10000.f / (10000 - out->StreamingParams.fscMaxMargin));
    }

    // CZ
    out->StreamingParams.bSupportCZ = ::property_get_int32("vendor.camera.mdp.cz.enable", 0);

    // DRE
    if( pParsedAppConfiguration->operationMode == 1 )
    {
        out->StreamingParams.bSupportDRE = 0;
    }
    else
    {
        out->StreamingParams.bSupportDRE = ::property_get_int32("vendor.camera.mdp.dre.enable", 0);
    }

    // HFG
    out->StreamingParams.bSupportHFG = ::property_get_int32("vendor.camera.mdp.hfg.enable", 0);

    // LMV
    if( pParsedAppConfiguration->operationMode != 1 /* CONSTRAINED_HIGH_SPEED_MODE */ &&
        pParsedSMVRBatchInfo == nullptr &&
        ( E3DNR_MODE_MASK_ENABLED(out->StreamingParams.nr3dMode, (E3DNR_MODE_MASK_UI_SUPPORT | E3DNR_MODE_MASK_HAL_FORCE_SUPPORT)) ||
          out->StreamingParams.eisInfo.mode) )
    {
        out->StreamingParams.bNeedLMV =
            (::property_get_int32("vendor.camera.lmv.force.enable", 1) > 0);
    }

    // RSS
    if( pParsedAppConfiguration->operationMode != 1 /* CONSTRAINED_HIGH_SPEED_MODE */ &&
        pParsedSMVRBatchInfo == nullptr &&
        ( ( EIS_MODE_IS_EIS_30_ENABLED(out->StreamingParams.eisInfo.mode) &&
            EIS_MODE_IS_EIS_IMAGE_ENABLED(out->StreamingParams.eisInfo.mode) ) ||
          ( E3DNR_MODE_MASK_ENABLED(out->StreamingParams.nr3dMode, (E3DNR_MODE_MASK_UI_SUPPORT | E3DNR_MODE_MASK_HAL_FORCE_SUPPORT)) &&
            E3DNR_MODE_MASK_ENABLED(out->StreamingParams.nr3dMode, E3DNR_MODE_MASK_RSC_EN) ) ) )
    {
        out->StreamingParams.bNeedRSS =
            (::property_get_int32("vendor.camera.rss.force.enable", 1) > 0);
        out->StreamingParams.bNeedLargeRsso = EIS_MODE_IS_EIS_30_ENABLED(out->StreamingParams.eisInfo.mode)
                                                && EIS_MODE_IS_EIS_IMAGE_ENABLED(out->StreamingParams.eisInfo.mode)
                                                && EFSC_FSC_ENABLED(out->StreamingParams.fscMode);
    }
    if( pParsedAppConfiguration->supportAIShutter && !(pParsedAppConfiguration->operationMode == 1 || pParsedSMVRBatchInfo != nullptr) )
    {
        out->StreamingParams.bNeedRSS =
            (::property_get_int32("vendor.camera.rss.force.enable", 1) > 0);
    }
    if ( pipInfo.isValid && !pipInfo.isRscOn && out->StreamingParams.bNeedRSS )
    {
        MY_LOGW("pip: !!warn:: disable RSS by policy");
        out->StreamingParams.bNeedRSS = false;
    }

    // Update Additional P1 Output Buffer
    if( in->needStreamingQuality )
    {
        out->StreamingParams.additionalHalP1OutputBufferNum.rrzo += 2;
        out->StreamingParams.additionalHalP1OutputBufferNum.lcso += 2;
        if( out->StreamingParams.bIsEIS == true )
        {
            out->StreamingParams.additionalHalP1OutputBufferNum.rrzo += 1;
            out->StreamingParams.additionalHalP1OutputBufferNum.lcso += 1;
        }
    }
    else {
        MY_LOGI("no need streaming quality output(needStreamingQuality:%d), no need additional buffers for streaming", in->needStreamingQuality);
    }

    MY_LOGI("dispStreamId[0x%09" PRIx64 "], MdpTargetSize(%dx%d), stream detail: disp[0x%09" PRIx64 "](%dx%d), rec[0x%09" PRIx64 "](%dx%d), extra[0x%09" PRIx64 "](%dx%d), extraSlow[0x%09" PRIx64 "](%dx%d);"
            "support features:%#" PRIx64", rrz ratio(%f), force4K(fps/size)=(%d/%dx%d) "
            ".AppConfiguration(#sessionParams=%d,operation=%d,supportAIShutter:%d) "
            ".AppImageStreamInfo(videoConsumer=%d)",
            out->StreamingParams.dispStreamId, mMdpTargetSize.w, mMdpTargetSize.h,
            pairDisp.first, pairDisp.second.w, pairDisp.second.h,
            pairRec.first, pairRec.second.w, pairRec.second.h,
            pairExtra.first, pairExtra.second.w, pairExtra.second.h,
            pairExtraSlow.first, pairExtraSlow.second.w, pairExtraSlow.second.h,
            out->StreamingParams.supportedScenarioFeatures,
            out->StreamingParams.targetRrzoRatio, out->StreamingParams.fpsFor4K,
            out->StreamingParams.aspectRatioFor4K.w,
            out->StreamingParams.aspectRatioFor4K.h,
            pParsedAppConfiguration->sessionParams.count(),
            pParsedAppConfiguration->operationMode,
            pParsedAppConfiguration->supportAIShutter,
            pParsedAppImageStreamInfo->hasVideoConsumer);
    return true;
}

auto
FeatureSettingPolicy::
updateStreamConfiguration_Sensor(
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in __unused,
    uint32_t sensorId
) -> bool
{
    CAM_ULOGM_APILIFE();

    auto const& pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo = mPolicyParams.pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;

    mForcedVHDRMode = ::property_get_int32("vendor.debug.force.vhdr.mode", 0);
    MINT32 hdrModeInt = mForcedVHDRMode ? mForcedVHDRMode : 0;
    if( hdrModeInt ||
        (pParsedAppImageStreamInfo->hasVideoConsumer &&
        IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams, MTK_HDR_FEATURE_HDR_MODE, hdrModeInt)) ) {
        mHDRHelper.at(sensorId)->updateAppConfigMode(static_cast<HDRMode>((uint8_t)hdrModeInt));
        if( mHDRHelper.at(sensorId)->isVideoHDRConfig() ) {
            mHDRHelper.at(sensorId)->updateAppRequestMode(static_cast<HDRMode>((uint8_t)hdrModeInt), HDRPolicyHelper::REQUEST_STREAMING);
        }
    }
    else {
        MY_LOGD_IF(mbDebug, "App no set MTK_HDR_FEATURE_HDR_MODE");
    }

    bool isStreaming = (in->reconfigCategory == ReCfgCtg::STREAMING ||
                        in->reconfigCategory == ReCfgCtg::NO);
    if( isStreaming && mHDRHelper.at(sensorId)->isVideoHDRConfig() ) {
        out->StreamingParams.p1ConfigParam.emplace(sensorId,
        StreamingFeatureSetting::P1ConfigParam {
            .hdrHalMode =  mHDRHelper.at(sensorId)->getHDRHalMode(),
            .hdrSensorMode = mHDRHelper.at(sensorId)->getHDRSensorMode(),
            .aeTargetMode = mHDRHelper.at(sensorId)->getAETargetMode(),
            .validExposureNum = mHDRHelper.at(sensorId)->getValidExposureNum(),
            .fusNum = mHDRHelper.at(sensorId)->getFusionNum(
                FRAME_TYPE_PREDUMMY),
        });
        out->StreamingParams.batchSize.emplace(sensorId,
            mHDRHelper.at(sensorId)->getGroupSize());

        if( mHDRHelper.at(sensorId)->isMulitFrameHDR() ||
            mHDRHelper.at(sensorId)->isConfigHdrOff()) {
            out->StreamingParams.bDisableInitRequest = true;
            if( mHDRHelper.at(sensorId)->getHDRHalMode() & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW )
            {
                out->StreamingParams.bForceIMGOPool = true;
            }
            if( out->isZslMode ) {
                out->StreamingParams.additionalHalP1OutputBufferNum.imgo = 20;
            }
        }
    }
    else {
        out->StreamingParams.p1ConfigParam.emplace(sensorId,
        StreamingFeatureSetting::P1ConfigParam {
            .hdrHalMode = MTK_HDR_FEATURE_HDR_HAL_MODE_OFF,
            .hdrSensorMode = SENSOR_VHDR_MODE_NONE,
            .aeTargetMode = MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL,
            .validExposureNum = MTK_STAGGER_VALID_EXPOSURE_NON,
            .fusNum = MTK_3A_ISP_FUS_NUM_NON,
        });
        out->StreamingParams.batchSize.emplace(sensorId, 0);
    }

    MY_LOGD_IF(mHDRHelper.at(sensorId)->getDebugLevel(), "%s.isZSLMode(%d)",
               mHDRHelper.at(sensorId)->getDebugMessage().string(), out->isZslMode);

    return true;
}

auto
FeatureSettingPolicy::
extractStreamConfiguration(
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in __unused
) -> bool {
    CAM_ULOGM_APILIFE();

    if (!extractStreamConfiguration_Default(out, in)) {
        MY_LOGE("extractStreamConfiguration_Default failed!");
        return false;
    }

    static const bool disableCustom = ::property_get_bool(
        KEY_FSP_STREAMING_CUSTOM_DISABLE, 0);
    const CustomStreamConfiguration customConfig = (!disableCustom) ?
        extractStreamConfiguration_Custom(in) : CustomStreamConfiguration{};
    MY_LOGD_IF(disableCustom, "Force disable customized configuration");

    if (!negotiateStreamConfiguration(customConfig, out, in)) {
        MY_LOGE("negotiateStreamConfiguration failed!");
        return false;
    }

    return true;
}

auto
FeatureSettingPolicy::
extractStreamConfiguration_Default(
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in __unused
) -> bool {
    CAM_ULOGM_APILIFE();

    const auto& pParsedAppConfiguration =
        mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    const auto& pParsedAppImageStreamInfo =
        mPolicyParams.pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    const auto& pPipelineStaticInfo =
        mPolicyParams.pPipelineStaticInfo;
    const auto& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    const auto& pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ?
        pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;
    const auto& pipInfo = out->StreamingParams.pipInfo;
    const bool isMultiZoom = (pParsedMultiCamInfo != nullptr) &&
                       (pParsedMultiCamInfo->mDualFeatureMode ==
                        MTK_MULTI_CAM_FEATURE_MODE_ZOOM);

    MINT32 forceEisEMMode = ::property_get_int32(
        "vendor.debug.eis.EMEnabled", DEBUG_EISEM);
    MINT32 forceEis30 = ::property_get_int32(
        "vendor.debug.camera.hal3.eis30", DEBUG_EIS30);
    MINT32 forceTSQ = ::property_get_int32(
        "vendor.debug.camera.hal3.tsq", DEBUG_TSQ);
    MINT32 force3DNR = ::property_get_int32(
        NR3D_PROP_3DNR_ENABLE, FORCE_3DNR);  // default on

    // PIP
    getPIPInfo(out->StreamingParams.pipInfo);

    // streaming FPS
    MINT32 streamHfpsMode = MTK_STREAMING_FEATURE_HFPS_MODE_NORMAL;
    MINT32 currentFps = 30;
    IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams,
        MTK_STREAMING_FEATURE_HFPS_MODE, streamHfpsMode);
    if (streamHfpsMode == MTK_STREAMING_FEATURE_HFPS_MODE_60FPS) {
        currentFps = 60;
    }
    out->StreamingParams.reqFps = currentFps;

    // proprietaryClient
    int32_t proprietaryClient = 0;
    IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams,
        MTK_CONFIGURE_SETTING_PROPRIETARY, proprietaryClient);

    // PQ
    int32_t bForceSupportPQ =
        ::property_get_int32("vendor.camera.mdp.pq.enable", -1);
    out->StreamingParams.bSupportPQ = (bForceSupportPQ != -1) ?
        (bForceSupportPQ > 0) : (proprietaryClient > 0);


    // 3DNR
    out->StreamingParams.nr3dMode = 0;
    MINT32 e3DnrMode = MTK_NR_FEATURE_3DNR_MODE_OFF;
    MBOOL isAPSupport3DNR = MFALSE;
    if( IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams,
                                    MTK_NR_FEATURE_3DNR_MODE, e3DnrMode) &&
        e3DnrMode == MTK_NR_FEATURE_3DNR_MODE_ON ) {
        isAPSupport3DNR = MTRUE;
    }
    if( force3DNR ) {
        out->StreamingParams.nr3dMode |= E3DNR_MODE_MASK_UI_SUPPORT;
        out->StreamingParams.nr3dMode |= E3DNR_MODE_MASK_HAL_FORCE_SUPPORT;
    }

    // FSC
    out->StreamingParams.fscMode = EFSC_MODE_MASK_FSC_NONE;
    auto isAFSupport = [&] (void) -> bool {
        for( const auto& [_sensorId, _hal3a] : mHal3a ) {
            NS3Av3::FeatureParam_T r3ASupportedParam;
            {
                std::lock_guard<std::mutex> _l(mHal3aLocker);
                if( _hal3a->send3ACtrl(NS3Av3::E3ACtrl_GetSupportedInfo,
                        reinterpret_cast<MINTPTR>(&r3ASupportedParam), 0) ) {
                    MY_LOGD_IF(mbDebug, "3A[%d]AF(%d)",
                        _sensorId, r3ASupportedParam.u4MaxFocusAreaNum);
                    if( r3ASupportedParam.u4MaxFocusAreaNum > 0 ) {
                        continue;
                    }
                }
                else {
                    MY_LOGW("Cannot query AF ability from 3A[%d]", _sensorId);
                }
                return false;
            }
        }
        return ( !mHal3a.empty() ) ? true : false;
    };
    if( pParsedAppImageStreamInfo->hasVideoConsumer ) {  // video mode
        MUINT32 fsc_mask = FSCCustom::USAGE_MASK_NONE;
        if( pParsedAppConfiguration->operationMode == 1
            /* CONSTRAINED_HIGH_SPEED_MODE */
            || pParsedSMVRBatchInfo != nullptr /* SMVRBatch */ ) {
            fsc_mask |= FSCCustom::USAGE_MASK_HIGHSPEED;
        }
        if( pPipelineStaticInfo->isDualDevice ||
            !mPolicyParams.bIsLogicalCam /* physical */ ||
            isMultiZoom ||
            (miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF) ) {
            fsc_mask |= FSCCustom::USAGE_MASK_DUAL_ZOOM;
        }
        if( FSCCustom::isEnabledFSC(fsc_mask) && isAFSupport() ) {
            out->StreamingParams.fscMode |= EFSC_MODE_MASK_FSC_EN;
            if (::property_get_int32(FSC_DEBUG_ENABLE_PROPERTY, 0))
                out->StreamingParams.fscMode |= EFSC_MODE_MASK_DEBUG_LEVEL;
            if (::property_get_int32(FSC_SUBPIXEL_ENABLE_PROPERTY, 1))
                out->StreamingParams.fscMode |= EFSC_MODE_MASK_SUBPIXEL_EN;
            out->StreamingParams.fscMaxMargin = FSCCustom::getMaxCropRatio();
        }
    }

    // EIS
    MUINT8 appEisMode = 0;
    MINT32 advEisMode = 0;
    MINT32 previewEis = 0;
    if( !IMetadata::getEntry<MUINT8>(&pParsedAppConfiguration->sessionParams, MTK_CONTROL_VIDEO_STABILIZATION_MODE, appEisMode) )
    {
        MY_LOGD_IF(mbDebug,"No MTK_CONTROL_VIDEO_STABILIZATION_MODE in sessionParams");
    }
    if( !IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams, MTK_EIS_FEATURE_EIS_MODE, advEisMode) )
    {
        MY_LOGD_IF(mbDebug,"No MTK_EIS_FEATURE_EIS_MODE in sessionParams");
    }
    IMetadata::getEntry<MINT32>(&pParsedAppConfiguration->sessionParams, MTK_EIS_FEATURE_PREVIEW_EIS, previewEis);

    out->StreamingParams.eisExtraBufNum = 0;
    MBOOL eis35Compatible4k = !pParsedAppImageStreamInfo->hasVideo4K || (pParsedAppImageStreamInfo->hasVideo4K && (EISCustom::get4kEIS35Fps() >= currentFps));
    out->StreamingParams.bPreviewEIS = ( (previewEis || forceEis30) && EISCustom::isSupportPreviewEIS() && eis35Compatible4k )
                                    || (pipInfo.isValid && pipInfo.isPreviewEisOn);

    if ( (!isMultiZoom && pParsedAppConfiguration->operationMode != 1 /* CONSTRAINED_HIGH_SPEED_MODE */ &&
             (advEisMode == MTK_EIS_FEATURE_EIS_MODE_ON || appEisMode == MTK_CONTROL_VIDEO_STABILIZATION_MODE_ON || forceEis30 || out->StreamingParams.bPreviewEIS) )
         || (pipInfo.isValid && pipInfo.isPreviewEisOn) )
    {
        out->StreamingParams.bIsEIS = true;
        MUINT32 eisMask = EISCustom::USAGE_MASK_NONE;
        MUINT32 videoCfg = EISCustom::VIDEO_CFG_FHD;
        if( pParsedAppImageStreamInfo->hasVideo4K ) {
            eisMask |= EISCustom::USAGE_MASK_4K2K;
            videoCfg = EISCustom::VIDEO_CFG_4K2K;
            out->StreamingParams.eisInfo.videoConfig = NSCam::EIS::VIDEO_CFG_4K2K;
        }
        else
        {
            out->StreamingParams.eisInfo.videoConfig = NSCam::EIS::VIDEO_CFG_FHD;
        }

        if( EISCustom::isSupportAdvEIS_HAL3() || forceEis30 ) {
            // must be TK app
            out->StreamingParams.eisInfo.mode = EISCustom::getEISMode(eisMask);
            // FSC+ only support EIS3.0
            if( EFSC_FSC_ENABLED(out->StreamingParams.fscMode) )
            {
                //EIS1.2 per-frame on/off
                if(!EIS_MODE_IS_EIS_30_ENABLED(out->StreamingParams.eisInfo.mode) && !EIS_MODE_IS_EIS_12_ENABLED(out->StreamingParams.eisInfo.mode)) {
                    MY_LOGI("disable FSC due to combine with EIS 2.x version!");
                    out->StreamingParams.fscMode = EFSC_MODE_MASK_FSC_NONE;
                }
            }
            if( EFSC_FSC_ENABLED(out->StreamingParams.fscMode) ) {
                eisMask |= EISCustom::USAGE_MASK_FSC;
            }
            out->StreamingParams.eisInfo.factor =
                EIS_MODE_IS_EIS_12_ENABLED(out->StreamingParams.eisInfo.mode) ?
                EISCustom::getEIS12Factor() : EISCustom::getEISFactor(videoCfg, eisMask);
            out->StreamingParams.eisInfo.queueSize = EISCustom::getForwardFrames(videoCfg);
            out->StreamingParams.eisInfo.startFrame = EISCustom::getForwardStartFrame() * (currentFps / 30);
            out->StreamingParams.eisInfo.lossless = EISCustom::isEnabledLosslessMode() &&
                                                    !pParsedAppImageStreamInfo->hasVideo4K;
            out->StreamingParams.eisInfo.supportQ = advEisMode && EISCustom::isEnabledForwardMode(videoCfg);
            out->StreamingParams.eisInfo.supportRSC = (out->StreamingParams.bPreviewEIS ||
                                                      EIS_MODE_IS_EIS_30_ENABLED(out->StreamingParams.eisInfo.mode) );
            if ( pipInfo.isValid && !pipInfo.isRscOn && out->StreamingParams.eisInfo.supportRSC )
            {
                MY_LOGW("pip: !!warn: disable RSC(EIS) by policy");
                out->StreamingParams.eisInfo.supportRSC = 0;
            }
            out->StreamingParams.eisInfo.previewEIS = out->StreamingParams.bPreviewEIS;
            out->StreamingParams.bEnableTSQ = true;
            if( forceTSQ == 2 ) { // force disable tsq
                out->StreamingParams.bEnableTSQ = false;
            }
            if( !out->StreamingParams.bEnableTSQ ) {
                out->StreamingParams.eisExtraBufNum = EISCustom::getForwardFrames(videoCfg);
            }
            float desireRatio = EISCustom::getHeightToWidthRatio();
            float desireHeight = desireRatio * pParsedAppImageStreamInfo->maxYuvSize.w;
            if( desireHeight > pParsedAppImageStreamInfo->maxYuvSize.h )
            {
                out->StreamingParams.rrzoHeightToWidth = desireRatio;
            }
        }
        else {
            out->StreamingParams.eisInfo.mode = (1<<EIS_MODE_EIS_12);
            out->StreamingParams.eisInfo.factor = EISCustom::getEIS12Factor();
            out->StreamingParams.bEnableTSQ = false;
        }
    }
    else if( EISCustom::isSupportAdvEIS_HAL3() && forceEisEMMode )
    {
        out->StreamingParams.bIsEIS = false;
        out->StreamingParams.eisInfo.mode = (1<<EIS_MODE_CALIBRATION);
        out->StreamingParams.fscMode = EFSC_MODE_MASK_FSC_NONE;
        out->StreamingParams.eisInfo.factor = 100;
        out->StreamingParams.bEnableTSQ = false;
    }
    else
    {
        out->StreamingParams.bIsEIS = false;
        out->StreamingParams.eisInfo.mode = 0;
        out->StreamingParams.eisInfo.factor = 100;
        out->StreamingParams.bEnableTSQ = false;
    }
    // DSDN
    out->StreamingParams.dsdnHint = 1;
    DSDNCustom::ScenarioParam sceneParam;
    sceneParam.hasAdvEIS = (advEisMode == MTK_EIS_FEATURE_EIS_MODE_ON);
    sceneParam.videoW = pParsedAppImageStreamInfo->videoImageSize.w;
    sceneParam.videoH = pParsedAppImageStreamInfo->videoImageSize.h;
    sceneParam.streamW = pParsedAppImageStreamInfo->maxYuvSize.w;
    sceneParam.streamH = pParsedAppImageStreamInfo->maxYuvSize.h;
    sceneParam.smvrFps = pParsedSMVRBatchInfo ? 30 * pParsedSMVRBatchInfo->p1BatchNum : 0;
    sceneParam.fps = currentFps;
    DSDNCustom::Config dsdnConfig = DSDNCustom::getInstance()->getConfig(sceneParam);
    if((dsdnConfig.flag & DSDNCustom::DFLAG_GYRO_ENABLE))
    {
        out->StreamingParams.dsdn30GyroEnable = 1;
    }

    MY_LOGD(".AppConfiguration(#sessionParams=%d,operation=%d) "
            ".AppImageStreamInfo(videoConsumer=%d) "
            ".3DNR(force=%d,ap=%d,dual=%d,stereo=%d) "
            ".EIS(ap=%d,adv=%d,adv_support=%d,EM=%d,multiZoom:%d); "
            "proprietaryClient:%d, AF:%d, AIShutter:%d",
            pParsedAppConfiguration->sessionParams.count(), pParsedAppConfiguration->operationMode,
            pParsedAppImageStreamInfo->hasVideoConsumer,
            force3DNR, isAPSupport3DNR, pPipelineStaticInfo->isDualDevice, StereoSettingProvider::getStereoFeatureMode(),
            appEisMode, advEisMode, EISCustom::isSupportAdvEIS_HAL3(), forceEisEMMode, isMultiZoom,
            proprietaryClient, isAFSupport(), pParsedAppConfiguration->supportAIShutter);

    return true;
}

auto
FeatureSettingPolicy::
negotiateStreamConfiguration(
    CustomStreamConfiguration const& customConfig,
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in __unused
) -> bool {
    CAM_ULOGM_APILIFE();

    const android::String8 defaultConfigStr = policy::toString(out->StreamingParams);

    /* apply customization
     */
    if (customConfig.proprietaryClient != CUSTOM_STREAM_DEFAULT) {
        out->StreamingParams.bSupportPQ = (customConfig.proprietaryClient > 0);
    }
    if (customConfig.reqFps != CUSTOM_STREAM_DEFAULT) {
        out->StreamingParams.reqFps = customConfig.reqFps;
    }
    if (customConfig.dsdnHint != CUSTOM_STREAM_DEFAULT) {
        // 0: force off, 1: on
        out->StreamingParams.dsdnHint = customConfig.dsdnHint;
    }
    if (customConfig.nr3dMode != CUSTOM_STREAM_DEFAULT) {
        out->StreamingParams.nr3dMode = customConfig.nr3dMode;
    }
    if (customConfig.eisFactor != CUSTOM_STREAM_DEFAULT) {
        out->StreamingParams.eisInfo.factor =
            (out->StreamingParams.eisInfo.mode != 0) ?
            customConfig.eisFactor : 100;
    }
    const android::String8 customConfigStr = policy::toString(out->StreamingParams);

    /* check hard constraints
     */
    const auto& pParsedAppConfiguration =
        mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    const auto& pParsedAppImageStreamInfo =
        mPolicyParams.pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    const auto& pPipelineStaticInfo =
        mPolicyParams.pPipelineStaticInfo;
    const auto& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    const auto& pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ?
        pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;
    const auto& pipInfo = out->StreamingParams.pipInfo;
    const bool isMultiZoom = (pParsedMultiCamInfo != nullptr) &&
                             (pParsedMultiCamInfo->mDualFeatureMode ==
                              MTK_MULTI_CAM_FEATURE_MODE_ZOOM);
    // DSDN
    if (pParsedAppImageStreamInfo->secureInfo.type != SecType::mem_normal) {
        out->StreamingParams.dsdnHint = 0;
    }
    // 3DNR
    // rule: slow motion--> smvrB: 3dnr on, smvrC: 3dnr off
    if (pParsedAppConfiguration->operationMode == 1 /* smvrC */
         || ( pParsedSMVRBatchInfo != nullptr &&
              NR3DCustom::is3DNRSmvrBatchSupported() != MTRUE)) {
        out->StreamingParams.nr3dMode = 0;
    }
    if (E3DNR_MODE_MASK_ENABLED(out->StreamingParams.nr3dMode,
             (E3DNR_MODE_MASK_UI_SUPPORT |
              E3DNR_MODE_MASK_HAL_FORCE_SUPPORT)) ) {
         if (::property_get_int32(NR3D_PROP_SL2E_ENABLE, 1) ) {
             out->StreamingParams.nr3dMode |= E3DNR_MODE_MASK_SL2E_EN;
         }
         MUINT32 nr3d_mask = NR3DCustom::USAGE_MASK_NONE;
         // cache dual cam feature mode for later check.

         if (pParsedAppConfiguration->operationMode == 1 /* smvrC */
             || pParsedSMVRBatchInfo != nullptr /* smvrB */
            )
         {
             nr3d_mask |= NR3DCustom::USAGE_MASK_HIGHSPEED;
         }
         if (pPipelineStaticInfo->isDualDevice || isMultiZoom ) {
             nr3d_mask |= NR3DCustom::USAGE_MASK_DUAL_ZOOM;
         }

         if ( pParsedAppImageStreamInfo->secureInfo.type == SecType::mem_normal &&
              NR3DCustom::isEnabledRSC(nr3d_mask) &&
              !(pParsedAppConfiguration->operationMode == 1 || pParsedSMVRBatchInfo != nullptr) )
         {
             out->StreamingParams.nr3dMode |= E3DNR_MODE_MASK_RSC_EN;
         }
         if ( pipInfo.isValid && !pipInfo.isRscOn && (out->StreamingParams.nr3dMode & E3DNR_MODE_MASK_RSC_EN) )
         {
             MY_LOGW("pip: !!warn: disable RSC(3DNR) by policy");
             out->StreamingParams.nr3dMode &= ~E3DNR_MODE_MASK_RSC_EN;
         }
         if ( pParsedAppImageStreamInfo->secureInfo.type == SecType::mem_normal &&
               NR3DCustom::is3DNRSmvrBatchSupported() &&
              (pParsedAppConfiguration->operationMode == 1 || pParsedSMVRBatchInfo != nullptr) )
         {
             out->StreamingParams.nr3dMode |= E3DNR_MODE_MASK_SMVR_EN;
         }
     }

    if (hasCustomStreamConfiguration(customConfig)) {
        MY_LOGD_IF(3 <= mbDebug, "(default)streamConfiguration:%s",
                   defaultConfigStr.c_str());
        MY_LOGD_IF(3 <= mbDebug, "(customized)streamConfiguration:%s",
                   customConfigStr.c_str());
        MY_LOGI("(extractStreamConfiguration_Custom)Customization:%s "
                "(out)streamConfiguration:%s",
                toString(customConfig).c_str(),
                policy::toString(out->StreamingParams).c_str());
    }
    return true;
}

auto
FeatureSettingPolicy::
updateStreamMultiFrame(
    RequestOutputParams* out,
    RequestInputParams const* in,
    NSPipelinePlugin::MetadataPtr pOutMetaApp_Additional,
    NSPipelinePlugin::MetadataPtr pOutMetaHal_Additional,
    uint32_t sensorId
) -> bool
{
    CAM_ULOGM_APILIFE();

    IMetadata const* pAppMetaControl = in->pRequest_AppControl;

    // SMVR
    if( mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->isConstrainedHighSpeedMode )
    {
        IMetadata::IEntry const& entry = pAppMetaControl->entryFor(MTK_CONTROL_AE_TARGET_FPS_RANGE);
        if( entry.isEmpty() )
        {
            MY_LOGW("SMVRConstraint: no MTK_CONTROL_AE_TARGET_FPS_RANGE");
        }
        else
        {
            MINT32 i4MinFps = entry.itemAt(0, Type2Type< MINT32 >());
            MINT32 i4MaxFps = entry.itemAt(1, Type2Type< MINT32 >());
            MINT32 postDummyReqs = i4MinFps == 30 ? (i4MaxFps/i4MinFps -1) : 0;

            uint32_t needP1Dma = 0;
            if ( !getStreamP1DmaConfig(needP1Dma, in, sensorId) ) {
                MY_LOGE("[getStreamP1DmaConfig] cam:%d postDummy P1Dma output is invalid: 0x%X", sensorId, needP1Dma);
            }

            std::shared_ptr<RequestResultParams> postDummyFrame = nullptr;
            for( MINT32 i = 0 ; i < postDummyReqs ; i++ ) {
                if( out->postDummyFrames.size() > i ) {
                    postDummyFrame = out->postDummyFrames[i]; // append
                    updateRequestResultParams_Streaming(
                        in,
                        postDummyFrame,
                        nullptr,
                        nullptr,
                        needP1Dma,
                        sensorId);
                    out->postDummyFrames[i] = postDummyFrame;
                }
                else {
                    updateRequestResultParams_Streaming(
                        in,
                        postDummyFrame,
                        nullptr,
                        nullptr,
                        needP1Dma,
                        sensorId);
                    out->postDummyFrames.push_back(postDummyFrame);
                }
            }
        }
    }

    // HDR
    if( mHDRHelper.at(sensorId)->isHDR() ) {
        AdditionalFrameInfo additionalFrame;
        if( !mHDRHelper.at(sensorId)->negotiateRequestPolicy(additionalFrame) ) {
            MY_LOGW("negotiateRequestPolicy failed");
        }

        // mainFrame
        {
            std::shared_ptr<NSCam::IMetadata>  outMetaApp = std::make_shared<IMetadata>();
            std::shared_ptr<NSCam::IMetadata>  outMetaHal = std::make_shared<IMetadata>();
            if (pOutMetaApp_Additional.get() != nullptr) {
                *outMetaApp += *pOutMetaApp_Additional;
            }
            if (pOutMetaHal_Additional.get() != nullptr) {
                *outMetaHal += *pOutMetaHal_Additional;
            }
            if (additionalFrame.getAdditionalFrameSet().mainFrame.appMetadata.get() != nullptr) {
                *outMetaApp += *additionalFrame.getAdditionalFrameSet().mainFrame.appMetadata;
            }
            if (additionalFrame.getAdditionalFrameSet().mainFrame.halMetadata.get() != nullptr) {
                *outMetaHal += *additionalFrame.getAdditionalFrameSet().mainFrame.halMetadata;
            }

            updateRequestResultParams_Streaming(
                in,
                out->mainFrame,
                outMetaApp,
                outMetaHal,
                additionalFrame.getAdditionalFrameSet().mainFrame.p1Dma,
                sensorId);
        }

        // preSubFrame
        for( size_t i = 0; i < additionalFrame.getAdditionalFrameSet().preSubFrame.size(); i++ ) {
            std::shared_ptr<RequestResultParams> newFrame = nullptr;
            std::shared_ptr<NSCam::IMetadata>  outMetaApp = std::make_shared<IMetadata>();
            std::shared_ptr<NSCam::IMetadata>  outMetaHal = std::make_shared<IMetadata>();
            // Append metadata from main-frame then update preSubFrame
            if( out->mainFrame.get() != nullptr ) {
                if( out->mainFrame->additionalApp.get() != nullptr ) {
                    *outMetaApp += *(out->mainFrame->additionalApp);
                }
                if( out->mainFrame->additionalHal.find(sensorId) != out->mainFrame->additionalHal.end() ) {
                    *outMetaHal += *(out->mainFrame->additionalHal.at(sensorId));
                }
            }
            if( pOutMetaApp_Additional.get() != nullptr ) {
                *outMetaApp += *pOutMetaApp_Additional;
            }
            if( pOutMetaHal_Additional.get() != nullptr ) {
                *outMetaHal += *pOutMetaHal_Additional;
            }
            if( additionalFrame.getAdditionalFrameSet().preSubFrame[i].appMetadata.get() != nullptr ) {
                *outMetaApp += *additionalFrame.getAdditionalFrameSet().preSubFrame[i].appMetadata;
            }
            if( additionalFrame.getAdditionalFrameSet().preSubFrame[i].halMetadata.get() != nullptr ) {
                *outMetaHal += *additionalFrame.getAdditionalFrameSet().preSubFrame[i].halMetadata;
            }

            if( out->preSubFrames.size() > i ) {
                newFrame = out->preSubFrames[i]; // append
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    additionalFrame.getAdditionalFrameSet().preSubFrame[i].p1Dma,
                    sensorId);
                out->preSubFrames[i] = newFrame;
            }
            else {
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    additionalFrame.getAdditionalFrameSet().preSubFrame[i].p1Dma,
                    sensorId);
                out->preSubFrames.push_back(newFrame);
            }
        }
        // subFrame
        for( size_t i = 0; i < additionalFrame.getAdditionalFrameSet().subFrame.size(); i++ ) {
            std::shared_ptr<RequestResultParams> newFrame = nullptr;
            std::shared_ptr<NSCam::IMetadata>  outMetaApp = std::make_shared<IMetadata>();
            std::shared_ptr<NSCam::IMetadata>  outMetaHal = std::make_shared<IMetadata>();
            // Append metadata from main-frame then update subFrame
            if( out->mainFrame.get() != nullptr ) {
                if( out->mainFrame->additionalApp.get() != nullptr ) {
                    *outMetaApp += *(out->mainFrame->additionalApp);
                }
                if( out->mainFrame->additionalHal.find(sensorId) != out->mainFrame->additionalHal.end() ) {
                    *outMetaHal += *(out->mainFrame->additionalHal.at(sensorId));
                }
            }
            if( pOutMetaApp_Additional.get() != nullptr ) {
                *outMetaApp += *pOutMetaApp_Additional;
            }
            if( pOutMetaHal_Additional.get() != nullptr ) {
                *outMetaHal += *pOutMetaHal_Additional;
            }
            if( additionalFrame.getAdditionalFrameSet().subFrame[i].appMetadata.get() != nullptr ) {
                *outMetaApp += *additionalFrame.getAdditionalFrameSet().subFrame[i].appMetadata;
            }
            if( additionalFrame.getAdditionalFrameSet().subFrame[i].halMetadata.get() != nullptr ) {
                *outMetaHal += *additionalFrame.getAdditionalFrameSet().subFrame[i].halMetadata;
            }

            if( out->subFrames.size() > i ) {
                newFrame = out->subFrames[i]; // append
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    additionalFrame.getAdditionalFrameSet().subFrame[i].p1Dma,
                    sensorId);
                out->subFrames[i] = newFrame;
            }
            else {
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    additionalFrame.getAdditionalFrameSet().subFrame[i].p1Dma,
                    sensorId);
                out->subFrames.push_back(newFrame);
            }
        }
        // preDummy
        for( size_t i = 0; i < additionalFrame.getAdditionalFrameSet().preDummy.size(); i++ ) {
            uint32_t camP1Dma = 0;
            if( !getStreamP1DmaConfig(camP1Dma, in, sensorId) ) {
                MY_LOGE("[getCaptureP1DmaConfig] cam:%d main P1Dma output is invalid: 0x%X", sensorId, camP1Dma);
                return MFALSE;
            }

            std::shared_ptr<RequestResultParams> newFrame = nullptr;
            std::shared_ptr<NSCam::IMetadata>  outMetaApp = std::make_shared<IMetadata>();
            std::shared_ptr<NSCam::IMetadata>  outMetaHal = std::make_shared<IMetadata>();
            // Append metadata from main-frame then update preDummyFrame
            if( out->mainFrame.get() != nullptr ) {
                if( out->mainFrame->additionalApp.get() != nullptr ) {
                    *outMetaApp += *(out->mainFrame->additionalApp);
                }
                if( out->mainFrame->additionalHal.find(sensorId) != out->mainFrame->additionalHal.end() ) {
                    *outMetaHal += *(out->mainFrame->additionalHal.at(sensorId));
                }
            }
            if( pOutMetaApp_Additional.get() != nullptr ) {
                *outMetaApp += *pOutMetaApp_Additional;
            }
            if( pOutMetaHal_Additional.get() != nullptr ) {
                *outMetaHal += *pOutMetaHal_Additional;
            }
            if( additionalFrame.getAdditionalFrameSet().preDummy[i].appMetadata.get() != nullptr ) {
                *outMetaApp += *additionalFrame.getAdditionalFrameSet().preDummy[i].appMetadata;
            }
            if( additionalFrame.getAdditionalFrameSet().preDummy[i].halMetadata.get() != nullptr ) {
                *outMetaHal += *additionalFrame.getAdditionalFrameSet().preDummy[i].halMetadata;
            }

            if( out->preDummyFrames.size() > i ) {
                newFrame = out->preDummyFrames[i]; // append
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    ( additionalFrame.getAdditionalFrameSet().preDummy[i].p1Dma | camP1Dma ),
                    sensorId);
                out->preDummyFrames[i] = newFrame;
            }
            else {
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    ( additionalFrame.getAdditionalFrameSet().preDummy[i].p1Dma | camP1Dma ),
                    sensorId);
                out->preDummyFrames.push_back(newFrame);
            }
        }
        // postDummy
        for( size_t i = 0; i < additionalFrame.getAdditionalFrameSet().postDummy.size(); i++ ) {
            uint32_t camP1Dma = 0;
            if( !getStreamP1DmaConfig(camP1Dma, in, sensorId) ) {
                MY_LOGE("[getCaptureP1DmaConfig] cam:%d main P1Dma output is invalid: 0x%X", sensorId, camP1Dma);
                return MFALSE;
            }

            std::shared_ptr<RequestResultParams> newFrame = nullptr;
            std::shared_ptr<NSCam::IMetadata>  outMetaApp = std::make_shared<IMetadata>();
            std::shared_ptr<NSCam::IMetadata>  outMetaHal = std::make_shared<IMetadata>();
            // Append metadata from main-frame then update postDummyFrame
            if( out->mainFrame.get() != nullptr ) {
                if( out->mainFrame->additionalApp.get() != nullptr ) {
                    *outMetaApp += *(out->mainFrame->additionalApp);
                }
                if( out->mainFrame->additionalHal.find(sensorId) != out->mainFrame->additionalHal.end() ) {
                    *outMetaHal += *(out->mainFrame->additionalHal.at(sensorId));
                }
            }
            if( pOutMetaApp_Additional.get() != nullptr ) {
                *outMetaApp += *pOutMetaApp_Additional;
            }
            if( pOutMetaHal_Additional.get() != nullptr ) {
                *outMetaHal += *pOutMetaHal_Additional;
            }
            if( additionalFrame.getAdditionalFrameSet().postDummy[i].appMetadata.get() != nullptr ) {
                *outMetaApp += *additionalFrame.getAdditionalFrameSet().postDummy[i].appMetadata;
            }
            if( additionalFrame.getAdditionalFrameSet().postDummy[i].halMetadata.get() != nullptr ) {
                *outMetaHal += *additionalFrame.getAdditionalFrameSet().postDummy[i].halMetadata;
            }

            if( out->postDummyFrames.size() > i ) {
                newFrame = out->postDummyFrames[i]; // append
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    ( additionalFrame.getAdditionalFrameSet().postDummy[i].p1Dma | camP1Dma ),
                    sensorId);
                out->postDummyFrames[i] = newFrame;
            }
            else {
                updateRequestResultParams_Streaming(
                    in,
                    newFrame,
                    outMetaApp,
                    outMetaHal,
                    ( additionalFrame.getAdditionalFrameSet().postDummy[i].p1Dma | camP1Dma ),
                    sensorId);
                out->postDummyFrames.push_back(newFrame);
            }
        }
    }

    if( mHDRHelper.at(sensorId)->isVideoHDRConfig() && mHDRHelper.at(sensorId)->isMulitFrameHDR() ) {
        size_t batchSize = out->preSubFrames.size() + out->subFrames.size() + 1 /*main frame*/;
        if( batchSize > 1 ) {
            out->batchPolicy = BatchPolicy {
                .size      = batchSize,
                .frameType = BatchPolicy::STREAMING,
            };
        }
    }

    MY_LOGD_IF(mHDRHelper.at(sensorId)->getDebugLevel(),
               "cam:%d,main:%d,sub:%zu,preSub:%zu,preDummy:%zu,postDummy:%zu,batchPolicy:(size:%zu,type:0x%x).HDRPolicy(%p)=%s",
               sensorId, (out->mainFrame.get() != nullptr), out->subFrames.size(), out->preSubFrames.size(), out->preDummyFrames.size(), out->postDummyFrames.size(),
               out->batchPolicy.size, out->batchPolicy.frameType, mHDRHelper.at(sensorId).get(), mHDRHelper.at(sensorId)->getDebugMessage().string());

    return true;
}

auto
FeatureSettingPolicy::
getStreamP1DmaConfig(
    uint32_t &needP1Dma,
    RequestInputParams const* in,
    uint32_t sensorId
) -> bool
{
    auto pConfig_StreamInfo_P1 = in->pConfiguration_StreamInfo_P1;
    if( pConfig_StreamInfo_P1->find(sensorId) == pConfig_StreamInfo_P1->end() )
    {
        MY_LOGE("sensorId(%d) doesn't exist in pConfiguration_StreamInfo_P1", sensorId);
        return false;
    }
    if( (*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo != nullptr )
    {
        needP1Dma |= P1_RRZO;
    }
    auto needImgo = [this] (MSize imgSize, MSize rrzoSize) -> int
    {
        // if isZslMode=true, must output IMGO for ZSL buffer pool
        return (imgSize.w > rrzoSize.w) || (imgSize.h > rrzoSize.h) || mConfigOutputParams.isZslMode;
    };

    if( (*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Imgo != nullptr &&
        (*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo != nullptr )
    {
        if( needImgo(in->maxP2StreamSize, (*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo->getImgSize()) ) {
            needP1Dma |= P1_IMGO;
        }
    }
    if( (*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Lcso != nullptr )
    {
        needP1Dma |= P1_LCSO;
    }
    if( (*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Rsso != nullptr )
    {
        needP1Dma |= P1_RSSO;
    }

    return true;
}

auto
FeatureSettingPolicy::
appendPrivateDataUpdateFunc_Streaming(
    RequestOutputParams* out,
    RequestInputParams const* in,
    uint32_t sensorId
) -> void
{
    CAM_ULOGM_APILIFE();

    auto pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;
    struct UpdateStreamRefParam
    {
        bool            isRecording = false;
        bool            smvrBatch = false;
    };
    std::shared_ptr<UpdateStreamRefParam> pParam = std::make_shared<UpdateStreamRefParam>();
    pParam->isRecording = in->pRequest_AppImageStreamInfo->hasVideoConsumer;
    pParam->smvrBatch = (pParsedSMVRBatchInfo != nullptr);

    auto privateData = std::bind(
                    [](UpdateStreamPrivateDataInput const& input, const std::shared_ptr<UpdateStreamRefParam> param)
                                -> std::optional<plugin::streaminfo::PrivateDataT>
        {
            std::optional<plugin::streaminfo::PrivateDataT> out = std::nullopt;
            if( param && input.data )
            {
                switch( (PrivateDataId)input.dataId )
                {
                    case PrivateDataId::P1STT:
                    {
                        if( param && input.data && param->smvrBatch && !param->isRecording )
                        {
                            using SttInfo = NSCam::plugin::streaminfo::P1STT;
                            plugin::streaminfo::PrivateDataT data = copy_privdata(*(input.data)); //copy
                            SttInfo *pStt = NSCam::plugin::streaminfo::get_variable_data_if<SttInfo>(data);
                            if( pStt )
                            {
                                if (pStt->useLcso)
                                {
                                    pStt->mLcsoInfo.count = 1;
                                }
                                if (pStt->useLcsho)
                                {
                                    pStt->mLcshoInfo.count = 1;
                                }
                            }
                            else
                            {
                                MY_LOGE("Convert to STT info failed!!");
                            }
                            out = data;
                        }
                        break;
                    }
                    default:
                        MY_LOGW("Unknown Private Datat ID(%u)", input.dataId);
                        break;
                } // switch case end
            }
            else
            {
                MY_LOGE("Param(%p) or input data(%p) not exist!!", param.get(), input.data);
            }
            return out;
        },
        placeholders::_1, pParam );

    out->funcUpdatePrivateData.emplace(sensorId, privateData);
}

auto
FeatureSettingPolicy::
updateRequestResultParams_Streaming(
    RequestInputParams const* in,
    std::shared_ptr<RequestResultParams> &requestParams,
    NSPipelinePlugin::MetadataPtr pOutMetaApp_Additional,
    NSPipelinePlugin::MetadataPtr pOutMetaHal_Additional,
    uint32_t needP1Dma,
    uint32_t sensorId
) -> bool
{
    if( in->pMultiCamReqOutputParams != nullptr ) {
        if( std::find(
                    in->pMultiCamReqOutputParams->streamingSensorList.begin(),
                    in->pMultiCamReqOutputParams->streamingSensorList.end(),
                    sensorId) == in->pMultiCamReqOutputParams->streamingSensorList.end() )
        {
            needP1Dma = 0;
            MY_LOGI("sensorId(%d) is not in streaming list, force no p1dma", sensorId);
            return true;
        }
    }

    return updateRequestResultParams(
            requestParams,
            pOutMetaApp_Additional,
            pOutMetaHal_Additional,
            needP1Dma,
            sensorId);
}

