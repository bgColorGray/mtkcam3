/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

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
*      TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/

//! \file HDRPolicyHelper.cpp


#undef LOG_TAG
#define LOG_TAG "mtkcam-HDRPolicyHelper"

#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/drv/iopipe/CamIO/INormalPipe.h>
#include <mtkcam/drv/iopipe/CamIO/Cam_QueryDef.h>
#include <mtkcam3/feature/vhdr/HDRPolicyHelper.h>
#include <mtkcam3/pipeline/policy/types.h>


#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
#endif


CAM_ULOG_DECLARE_MODULE_ID(MOD_VHDR_HAL);


#define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))


#define VHDR_LOG(lv, fmt, arg...)      CAM_ULOGM##lv("[%s]" fmt, __func__, ##arg)
#define VHDR_S_LOG(lv, fmt, arg...)    CAM_ULOGM##lv("[%s][Cam::%d]" fmt, __func__, mSensorId, ##arg)

#define VHDR_DO(cmd) do { cmd; } while(0)

#define MY_LOGD(fmt, arg...)           VHDR_DO( VHDR_LOG(D, fmt, ##arg))
#define MY_LOGI(fmt, arg...)           VHDR_DO( VHDR_LOG(I, fmt, ##arg))
#define MY_LOGW(fmt, arg...)           VHDR_DO( VHDR_LOG(W, fmt, ##arg))
#define MY_LOGE(fmt, arg...)           VHDR_DO( VHDR_LOG(E, fmt, ##arg))
#define MY_LOGE_IF(c, fmt, arg...)     VHDR_DO(if(c) VHDR_LOG(E, fmt, ##arg))

#define MY_S_LOGD(fmt, arg...)         VHDR_DO( VHDR_S_LOG(D, fmt, ##arg))
#define MY_S_LOGI(fmt, arg...)         VHDR_DO( VHDR_S_LOG(I, fmt, ##arg))
#define MY_S_LOGW(fmt, arg...)         VHDR_DO( VHDR_S_LOG(W, fmt, ##arg))
#define MY_S_LOGE(fmt, arg...)         VHDR_DO( VHDR_S_LOG(E, fmt, ##arg))
#define MY_S_LOGD_IF(c, fmt, arg...)   VHDR_DO(if(c) VHDR_S_LOG(D, fmt, ##arg))

#define MY_TRACE_API_LIFE()            CAM_ULOGM_APILIFE()
#define MY_TRACE_FUNC_LIFE()           CAM_ULOGM_FUNCLIFE()
#define MY_TRACE_TAG_LIFE(name)        CAM_ULOGM_TAGLIFE(name)

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
static const unsigned int VHDR_AEE_DB_FLAGS = DB_OPT_NE_JBT_TRACES | DB_OPT_PROCESS_COREDUMP | DB_OPT_PROC_MEM | DB_OPT_PID_SMAPS |
                                              DB_OPT_LOW_MEMORY_KILLER | DB_OPT_DUMPSYS_PROCSTATS | DB_OPT_FTRACE;

#define MY_LOGA(fmt, arg...)                                                            \
        do {                                                                            \
            android::String8 const str = android::String8::format(fmt, ##arg);          \
            MY_LOGE("ASSERT(%s)", str.c_str());                                         \
            aee_system_exception(LOG_TAG, NULL, VHDR_AEE_DB_FLAGS, str.c_str());        \
            int ret = raise(SIGKILL);                                                   \
            MY_LOGE_IF((ret != 0), "Kill Process Fail");                                \
        } while(0)
#else
#define MY_LOGA(fmt, arg...)                                                            \
        do {                                                                            \
            android::String8 const str = android::String8::format(fmt, ##arg);          \
            MY_LOGE("ASSERT(%s)", str.c_str());                                         \
        } while(0)
#endif

#define ENABLE_VHDR     (1)
#define DEBUG_VHDR      (0)
#define DEBUG_APP_HDR   (-1)
#define DEBUG_DUMMY_HDR (1)
#define ENABLE_BYPASS_MODE (1)
#define USE_FW_RECONFIG   (1)
#define ENABLE_CAPTURE_HAL_RECONFIG (0)

using namespace NSCam::v3::pipeline::policy;
using namespace NS3Av3; //IHal3A
using namespace NSCam::NSIoPipe::NSCamIOPipe;

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {


uint32_t getHDRHalCapabilityMode(int32_t sensorId) {
    MY_TRACE_API_LIFE();

    sp<NSCam::IMetadataProvider> metaProvider = NSCam::NSMetadataProviderManager::valueFor(sensorId);
    const auto& staticMeta = metaProvider->getMtkStaticCharacteristics();

    if( !::property_get_int32("vendor.debug.camera.vhdr.enabled", ENABLE_VHDR) ) {
        return MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
    }

    uint32_t hdrMode = MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;

    IMetadata::IEntry availMstream = staticMeta.entryFor(MTK_HDR_FEATURE_AVAILABLE_MSTREAM_HDR_MODES);
    for( size_t i = 0 ; i < availMstream.count() ; i++ )
    {
        hdrMode = availMstream.itemAt(i, Type2Type<MINT32>());
        if( hdrMode != MTK_HDR_FEATURE_HDR_HAL_MODE_OFF ) {
            return hdrMode;
        }
    }

    IHalSensorList *const pIHalSensorList = MAKE_HalSensorList(); // singleton, no need to release
    if (pIHalSensorList) {
        MUINT32 sensorDev = (MUINT32) pIHalSensorList->querySensorDevIdx(sensorId);
        NSCam::SensorStaticInfo sensorStaticInfo;
        memset(&sensorStaticInfo, 0, sizeof(NSCam::SensorStaticInfo));
        pIHalSensorList->querySensorStaticInfo(sensorDev, &sensorStaticInfo);

        //Check Stagger sensor capability
        if (sensorStaticInfo.HDR_Support >= HDR_SUPPORT_STAGGER_MIN && sensorStaticInfo.HDR_Support < HDR_SUPPORT_STAGGER_MAX) {
            IMetadata::IEntry availStagger = staticMeta.entryFor(MTK_HDR_FEATURE_AVAILABLE_STAGGER_HDR_MODES);
            for( size_t i = 0 ; i < availStagger.count() ; i++ )
            {
                hdrMode = availStagger.itemAt(i, Type2Type<MINT32>());
                if( hdrMode != MTK_HDR_FEATURE_HDR_HAL_MODE_OFF ) {
                    break;
                }
            }
        }

        //Check Stagger platform capability
        auto pModule = INormalPipeModule::get();
        sCAM_QUERY_SUPPORT_EXP_NUM sExpNum;
        if ((hdrMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) && pModule->query((MUINT32)ENPipeQueryCmd_SUPPORT_EXP_NUM, (MUINTPTR)&sExpNum)) {
            if (sExpNum.QueryOutput == 2) {
                return MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP;
            }
            else if (sExpNum.QueryOutput == 3) {
                return hdrMode;
            }
            else {
                MY_LOGW("Platform supported exposure number(%d)", sExpNum.QueryOutput);
            }
        }
        hdrMode = MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
    }

    uint32_t vhdrMode = MTK_HDR_FEATURE_VHDR_MODE_OFF;
    IMetadata::IEntry availVhdrEntry = staticMeta.entryFor(MTK_HDR_FEATURE_AVAILABLE_VHDR_MODES);
    for( size_t i = 0 ; i < availVhdrEntry.count() ; i++ ) {
        vhdrMode = availVhdrEntry.itemAt(i, Type2Type<MINT32>());
        if( vhdrMode != MTK_HDR_FEATURE_VHDR_MODE_OFF ) {
            auto pHalDeviceList = MAKE_HalLogicalDeviceList();
            if( CC_UNLIKELY( pHalDeviceList == NULL) ) {
                MY_LOGE("pHalDeviceList::get fail");
            }
            else {
                MY_TRACE_TAG_LIFE("SensorStaticInfo::querySensorStaticInfo");
                NSCam::SensorStaticInfo sensorStaticInfo;
                pHalDeviceList->querySensorStaticInfo(pHalDeviceList->querySensorDevIdx(sensorId), &sensorStaticInfo);
                if (sensorStaticInfo.HDR_Support > 0) {
                    return HDRPolicyHelper::toHDRHalMode(vhdrMode);
                }
                else {
                    MY_LOGA("Error: Sensor capability mismatch=>metadata(0x%x),HDR_Support(%d)"
                            "\nCRDISPATCH_KEY:VHDR Sensor capability mismatch",
                            vhdrMode, sensorStaticInfo.HDR_Support);

                }
            }
        }
    }

    return MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
}

static SensorMap<uint32_t> getHDRPolicyMap(const std::vector<int32_t>&
    sensorIdList) {
    SensorMap<uint32_t> hdrHalPolicyMap;
    // capability
    for (const auto& sensorId: sensorIdList) {
        hdrHalPolicyMap.emplace(sensorId, getHDRHalCapabilityMode(sensorId));
    }
    // negotiation
    for (const auto& policyMap: hdrHalPolicyMap) {
        if (::property_get_int32("vendor.debug.camera.hal3.vhdr", DEBUG_VHDR)) {
            MY_LOGD("[capability]HDRPolicyMap[%d](%d)",
                    policyMap.first, policyMap.second);
        }
        // TODO(MTK): use driver querying function
        if (policyMap.second & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
            for (auto& negotation: hdrHalPolicyMap) {
                if ((negotation.first != policyMap.first) &&
                    (negotation.second &
                     MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW) ) {
                    MY_LOGI("[%d]HDRHal(%d)->(0) "
                            "due to [%d]HDRHal(%d)",
                            negotation.first, negotation.second,
                            policyMap.first, policyMap.second);
                    negotation.second = MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
                }
            }
        }
    }
    if (::property_get_int32("vendor.debug.camera.hal3.vhdr", DEBUG_VHDR)) {
        for (const auto& pair: hdrHalPolicyMap) {
            MY_LOGD("[negotiation]HDRPolicyMap[%d](%d)",
                    pair.first, pair.second);
        }
    }
    return hdrHalPolicyMap;
}

HDRPolicyHelper::
HDRPolicyHelper(
    int32_t sensorId,
    uint32_t hdrHalMode
)
{
    MY_TRACE_API_LIFE();

    mSensorId      = sensorId;
    mHDRHalMode    = hdrHalMode;
    mSensorType    = getSensorType(sensorId);
    mHDRSensorMode = toHDRSensorMode(mHDRHalMode);
    mStrategyMode  = toHDRStrategyMode(mHDRHalMode);
    mDebugLevel    = ::property_get_int32("vendor.debug.camera.hal3.vhdr", DEBUG_VHDR);
    mNumDummy      = ::property_get_int32("vendor.debug.camera.hal3.dummycount", DEBUG_DUMMY_HDR);// for dummyconunt debug after doing capture
    mEnableBypassMode = ::property_get_int32("vendor.debug.mstream.strategy", ENABLE_BYPASS_MODE);

    if( mDebugLevel ) {
        uint32_t forceAppMode = ::property_get_int32("vendor.debug.camera.hal3.appHdrMode", DEBUG_APP_HDR);
        if( forceAppMode >= static_cast<uint32_t>(HDRMode::OFF) && forceAppMode < static_cast<uint32_t>(HDRMode::NUM) ) {
            mHDRAppMode = static_cast<HDRMode>((uint8_t)forceAppMode);
            mHDRHalAppMode = static_cast<HDRMode>((uint8_t)forceAppMode);
            mForceAppMode = true;
            MY_S_LOGD("[DebugMode]Force AppHdrMode(%d)=>AppHdrMode(%hhu),HalAppHdrMode(%hhu)",
                    forceAppMode, mHDRAppMode, mHDRHalAppMode);
        }
        MY_S_LOGD("(%p):HDRAppMode(%hhu),HDRHalAppMode(%hhu),HDRHalMode(0x%x),HDRSensorMode(0x%x),StrategyMode(%d),SensorType(%d)",
                this, mHDRAppMode, mHDRHalAppMode, mHDRHalMode, mHDRSensorMode, mStrategyMode, mSensorType);
    }

    if( ::property_get_int32("vendor.debug.mstream.nosubframe", 0) ) {
        mForceMainFrame = true;
    }

    {
        MY_TRACE_TAG_LIFE("IHal3A::MAKE_Hal3A");

        std::shared_ptr<IHal3A> hal3a
        (
            MAKE_Hal3A(mSensorId, LOG_TAG),
            [](IHal3A* p){ if(p) p->destroyInstance(LOG_TAG); }
        );
        if (hal3a.get()) {
            mHal3a = hal3a;
        }
        else {
            MY_S_LOGE("Cannot get HAL3A instance!");
        }
    }
}

auto
HDRPolicyHelper::
toHDRHalMode(
    uint32_t vHdrMode
) -> uint32_t
{
    switch( vHdrMode )
    {
    case MTK_HDR_FEATURE_VHDR_MODE_MVHDR:
        return MTK_HDR_FEATURE_HDR_HAL_MODE_MVHDR;
    default:
        return MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
    }
    return MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
}

auto
HDRPolicyHelper::
toHDRSensorMode(
    uint32_t hdrHalMode
) -> uint32_t
{
    switch( hdrHalMode )
    {
    case MTK_HDR_FEATURE_HDR_HAL_MODE_MVHDR:
        return SENSOR_VHDR_MODE_MVHDR;
    default:
        return SENSOR_VHDR_MODE_NONE;
    }
    return SENSOR_VHDR_MODE_NONE;
}

auto
HDRPolicyHelper::
toHDRStrategyMode(
    uint32_t hdrHalMode
) -> STRATEGY_MODE
{
    if( hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW ||
        hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
        return STRATEGY_MODE_HAL_BYPASS;
    }
    return STRATEGY_MODE_HAL_RECONFIG;
}

auto
HDRPolicyHelper::
getHDRAppMode(
) -> HDRMode
{
    return mHDRHalAppMode;
}

auto
HDRPolicyHelper::
getHDRHalMode(
) -> uint32_t
{
    return mHDRHalMode;
}

auto
HDRPolicyHelper::
getHDRHalRequestMode(
) -> uint32_t
{
    if( isHDR() ) {
        if( getHDRHalMode() & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW ) {
            // mstream no support capture, be sure to mark error for capture + streaming
            return MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW;
        }
        return getHDRHalMode();
    }
    return MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
}

auto
HDRPolicyHelper::
getHDRSensorMode(
) -> uint32_t
{
    return mHDRSensorMode;
}

auto
HDRPolicyHelper::
getHDRExposure(
    FRAME_TYPE type
) -> uint32_t
{
    if( isHDR() && ( getHDRHalMode() & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW )  ) {
        switch( type ) {
        case FRAME_TYPE_MAIN:
            return MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_NORMAL;
        case FRAME_TYPE_PRESUB:
            return MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_SHORT;
        case FRAME_TYPE_SUB:
        case FRAME_TYPE_PREDUMMY:
        case FRAME_TYPE_POSTDUMMY:
        case FRAME_TYPE_UNKNOWN:
        default:
            return MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_NONE;
        }
    }
    return MTK_3A_FEATURE_AE_EXPOSURE_LEVEL_NONE;
}

auto
HDRPolicyHelper::
getDebugLevel(
) -> uint32_t
{
    return mDebugLevel;
}

auto
HDRPolicyHelper::
getGroupSize(
) -> size_t
{
    if( isVideoHDRConfig() && (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW) ) {
        return GROUP_SIZE_MSTREAM;
    }
    return GROUP_SIZE_NONE;
}

auto
HDRPolicyHelper::
getDummySize(
) -> size_t
{
    return mNumDummy;
}


auto
HDRPolicyHelper::
getDebugMessage(
) -> android::String8
{
    return android::String8::format("HDRPolicyHelper(%p)=StrategyMode(%d),SensorType(%d),"
               "HDRAppMode(%hhu),HDRHalAppMode(%hhu),HDRHalMode(0x%x),HDRSensorMode(0x%x),AETargetMode(%d),"
               "isAppHDR(%d),isAppVideoHDR(%d),isHalAppHDR(%d),isHalAppVideoHDR(%d),"
               "isHalHDR(%d),isHDR(%d),isZSLHDR(%d),isMulitFrameHDR(%d),needReconfiguration(%d)",
               this, mStrategyMode, mSensorType,
               mHDRAppMode, mHDRHalAppMode, getHDRHalMode(), getHDRSensorMode(), getAETargetMode(),
               isAppHDR(), isAppVideoHDR(), isHalAppHDR(), isHalAppVideoHDR(),
               isHalHDR(), isHDR(), isZSLHDR(), isMulitFrameHDR(), needReconfiguration());
}

auto
HDRPolicyHelper::
isHDR(
) -> bool
{
    return ( isHalAppHDR() && isHalHDR() && isZSLHDR() ) || ( isHalAppVideoHDR() && isHalHDR() );
}

auto
HDRPolicyHelper::
isHDRConfig(
) -> bool
{
    return ( isAppHDR() && isHalHDR() && isZSLHDR() ) || ( isAppVideoHDR() && isHalHDR() );
}

auto
HDRPolicyHelper::
isVideoHDRConfig(
) -> bool
{
    return ( isAppVideoHDR() && isHalHDR() );
}

auto
HDRPolicyHelper::
isZSLHDR(
) -> bool
{
    return ( (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE) ||
             (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) );
}

auto
HDRPolicyHelper::
isMulitFrameHDR(
) -> bool
{
    return ( mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW );
}

auto
HDRPolicyHelper::
isStaggerHDR(
) -> bool
{
    return (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER);
}

auto
HDRPolicyHelper::
needReconfiguration(
) -> bool
{
    return mNeedReconfigure;
}

auto
HDRPolicyHelper::
needDummy(
) -> bool
{
    return ( isHDR() && !isZSLHDR() && ( mAddDummy && mNumDummy >= 1) );
}

auto
HDRPolicyHelper::
needPreSubFrame(
) -> bool
{
    return isHDR() && isMulitFrameHDR() && !mForceMainFrame;
}

auto
HDRPolicyHelper::
negotiateRequestPolicy(
    AdditionalFrameInfo& additionalFrame
) -> bool
{
    MY_TRACE_API_LIFE();

    if( needPreSubFrame() ) {
        size_t preSubFrameNum = (getGroupSize() > 1) ?
                                (getGroupSize() /*all*/ - 1 /*main*/) : 0;
        for (size_t i = 0; i < preSubFrameNum ; i++) {
            std::shared_ptr<NSCam::IMetadata> additionalHal = std::make_shared<IMetadata>();
            uint32_t p1Dma = 0;
            if( isMulitFrameHDR() ) {
                p1Dma |= P1_IMGO;
            }

            int ret = 0;
            ret |= IMetadata::setEntry<MINT32>(additionalHal.get(), MTK_3A_ISP_BYPASS_LCE, 1);
            ret |= IMetadata::setEntry<MINT32>(additionalHal.get(), MTK_3A_FEATURE_AE_EXPOSURE_LEVEL,
                                               getHDRExposure(FRAME_TYPE_PRESUB));
            ret |= IMetadata::setEntry<MINT32>(additionalHal.get(), MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM,
                                               MTK_STAGGER_VALID_EXPOSURE_NON);
            ret |= IMetadata::setEntry<MINT32>(additionalHal.get(), MTK_3A_ISP_FUS_NUM,
                                               MTK_3A_ISP_FUS_NUM_NON);

            if( ret != 0 ) {
                MY_S_LOGE("Set PreSubFrame metadata fail");
                return false;
            }

            additionalFrame.addOneFrame(FRAME_TYPE_PRESUB, p1Dma, nullptr, additionalHal);
        }
    }

    if( needDummy() ) {
        for( size_t i = 0; i < mNumDummy; i++ ) {
            additionalFrame.addOneFrame(FRAME_TYPE_PREDUMMY, 0, nullptr, nullptr);
        }
        mAddDummy = false;
    }

    if( mLMVInvalidity ) {
        std::shared_ptr<NSCam::IMetadata> additionalHal = std::make_shared<IMetadata>();
        IMetadata::setEntry<MINT32>(additionalHal.get(), MTK_LMV_VALIDITY, 0);
        additionalFrame.addOneFrame(FRAME_TYPE_MAIN, 0, nullptr, additionalHal);

        mLMVInvalidity = false;
    }

    MY_S_LOGD_IF(getDebugLevel(), "preSubFrame(%zu),preDummy(%zu),subFrame(%zu),postDummy(%zu)",
        additionalFrame.getAdditionalFrameSet().preSubFrame.size(), additionalFrame.getAdditionalFrameSet().preDummy.size(),
        additionalFrame.getAdditionalFrameSet().subFrame.size(), additionalFrame.getAdditionalFrameSet().postDummy.size());

    if( getDebugLevel() >= 3 ) {
        if( additionalFrame.getAdditionalFrameSet().mainFrame.appMetadata != nullptr ) {
            additionalFrame.getAdditionalFrameSet().mainFrame.appMetadata->dump();
        }
        if( additionalFrame.getAdditionalFrameSet().mainFrame.halMetadata != nullptr ) {
            additionalFrame.getAdditionalFrameSet().mainFrame.halMetadata->dump();
        }
        for( const auto &preSubFrame : additionalFrame.getAdditionalFrameSet().preSubFrame ) {
            preSubFrame.appMetadata->dump();
            preSubFrame.halMetadata->dump();
        }
        for( const auto &preDummy : additionalFrame.getAdditionalFrameSet().preDummy ) {
            preDummy.appMetadata->dump();
            preDummy.halMetadata->dump();
        }
        for( const auto &subFrame : additionalFrame.getAdditionalFrameSet().subFrame ) {
            subFrame.appMetadata->dump();
            subFrame.halMetadata->dump();
        }
        for( const auto &postDummy : additionalFrame.getAdditionalFrameSet().postDummy ) {
            postDummy.appMetadata->dump();
            postDummy.halMetadata->dump();
        }
    }

    return true;
}

auto
HDRPolicyHelper::
notifyDummy(
) -> bool
{
    if( isHDR() && !isZSLHDR() && (mNumDummy >= 1) ) {
        mAddDummy = true;
        MY_S_LOGD("Dummy Notified:isAppHDR(%d),isHalAppHDR(%d),isHalHDR(%d),isZSLHDR(%d),AddDummy(%d),NumDummy(%zu)",
                isAppHDR(), isHalAppHDR(), isHalHDR(), isZSLHDR(), mAddDummy, mNumDummy);
    }
    return true;
}

auto
HDRPolicyHelper::
notifyManualExp(
) -> bool
{
    mLMVInvalidity = true;
    MY_S_LOGD_IF(getDebugLevel(), "notifyManualExp Notified");
    return true;
}

auto
HDRPolicyHelper::
handleReconfiguration(
) -> bool
{
    mNeedReconfigure = false;
    MY_S_LOGD("Strategy(%d) trigger reconfiguration", mStrategyMode);
    return true;
}

auto
HDRPolicyHelper::
getSensorType(
    int32_t sensorId
) -> SENSOR_TYPE
{
    MY_TRACE_API_LIFE();

    const uint32_t STATIC_SENSOR_TYPE_4CELL = 4;
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if( CC_UNLIKELY( pHalDeviceList == NULL) ) {
        MY_LOGE("pHalDeviceList::get fail");
    }
    else {
        MY_TRACE_TAG_LIFE("SensorStaticInfo::querySensorStaticInfo");
        NSCam::SensorStaticInfo sensorStaticInfo;
        pHalDeviceList->querySensorStaticInfo(pHalDeviceList->querySensorDevIdx(sensorId), &sensorStaticInfo);
        if( sensorStaticInfo.HDR_Support == STATIC_SENSOR_TYPE_4CELL ) {
            return SENSOR_TYPE_4CELL;
        }
    }
    return SENSOR_TYPE_NORMAL;
}

auto
HDRPolicyHelper::
getAETargetMode(
) -> uint32_t
{
    if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW) {
        if (mHDRRcfgInfo.hdrState >= HDR_STATE_2EXP) {
            return MTK_3A_FEATURE_AE_TARGET_MODE_MSTREAM_VHDR;
        }
        else if (isHDRConfig() && isAppVideoHDR()){
            return MTK_3A_FEATURE_AE_TARGET_MODE_MSTREAM_VHDR_RTO1X;
        }
    }
    else if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
        if (mHDRRcfgInfo.hdrState == HDR_STATE_2EXP) {
            return MTK_3A_FEATURE_AE_TARGET_MODE_STAGGER_2EXP;
        }
        else if (mHDRRcfgInfo.hdrState == HDR_STATE_3EXP) {
            return MTK_3A_FEATURE_AE_TARGET_MODE_STAGGER_3EXP;
        }
    }
    return MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL;
}

auto
HDRPolicyHelper::
getReqAETargetMode(
    HDRState hdrState
) -> uint32_t
{
    if( mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW ) {
        return (hdrState >= HDR_STATE_2EXP) ? MTK_3A_FEATURE_AE_TARGET_MODE_MSTREAM_VHDR :
                                              MTK_3A_FEATURE_AE_TARGET_MODE_MSTREAM_VHDR_RTO1X;
    }
    else if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
        if (hdrState == HDR_STATE_2EXP) {
            return MTK_3A_FEATURE_AE_TARGET_MODE_STAGGER_2EXP;
        }
        else if (hdrState == HDR_STATE_3EXP) {
            return MTK_3A_FEATURE_AE_TARGET_MODE_STAGGER_3EXP;
        }
    }
    else {
        return MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL;
    }

    return MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL;
}

auto
HDRPolicyHelper::
getFusionNum(
    FRAME_TYPE frameType
) -> uint32_t
{
    if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW) {
        if ((frameType == FRAME_TYPE_MAIN) &&
            (mHDRRcfgInfo.hdrState >= HDR_STATE_2EXP)) {
            return MTK_3A_ISP_FUS_NUM_2;
        }
        else {
            return MTK_3A_ISP_FUS_NUM_NON;
        }
    }
    else if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
        if (mHDRRcfgInfo.hdrState == HDR_STATE_3EXP) {
            return MTK_3A_ISP_FUS_NUM_3;
        }
        else if (mHDRRcfgInfo.hdrState == HDR_STATE_2EXP) {
            return MTK_3A_ISP_FUS_NUM_2;
        }
        else if (mHDRRcfgInfo.hdrState == HDR_STATE_1EXP) {
            return MTK_3A_ISP_FUS_NUM_1;
        }
    }
    MY_S_LOGD_IF(getDebugLevel(),
                 "hdrHalMode(%d),frameType(%d) no support, use(0). %s",
                 mHDRHalMode, frameType, getDebugMessage().string());
    return MTK_3A_ISP_FUS_NUM_NON;
}

auto
HDRPolicyHelper::
getValidExposureNum(
) -> uint32_t
{
    if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
        switch (mHDRRcfgInfo.hdrState) {
        case HDR_STATE_1EXP:
            return MTK_STAGGER_VALID_EXPOSURE_1;
        case HDR_STATE_2EXP:
            return MTK_STAGGER_VALID_EXPOSURE_2;
        case HDR_STATE_3EXP:
            return MTK_STAGGER_VALID_EXPOSURE_3;
        default:
            break;
        }
    }
    else if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW) {
        return MTK_STAGGER_VALID_EXPOSURE_NON;
    }

    return MTK_STAGGER_VALID_EXPOSURE_NON;
}

auto
HDRPolicyHelper::
getSeamlessSmoothResult(
    FRAME_TYPE frameType, uint32_t& requestAETargetMode, uint32_t& requestExpNum
) -> bool
{
    if ((mHDRRcfgInfo.hdrProcState == STATE_IDLE) ||
        (mHDRRcfgInfo.hdrProcState == STATE_DONE)) {
        requestAETargetMode = MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL;
        requestExpNum = MTK_STAGGER_VALID_EXPOSURE_NON;
    }
    else if (mHDRRcfgInfo.hdrProcState == STATE_PROCESSING) {
        requestAETargetMode = getReqAETargetMode(mHDRRcfgInfo.reqHdrState);

        if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
            switch (mHDRRcfgInfo.reqHdrState) {
            case HDR_STATE_1EXP:
                requestExpNum = MTK_STAGGER_VALID_EXPOSURE_1;
                break;
            case HDR_STATE_2EXP:
                requestExpNum = MTK_STAGGER_VALID_EXPOSURE_2;
                break;
            case HDR_STATE_3EXP:
                requestExpNum = MTK_STAGGER_VALID_EXPOSURE_3;
                break;
            default:
                MY_LOGE("Should not be here! [%d]", mHDRRcfgInfo.reqHdrState);
            }
        }
        else if (mHDRHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW) {
            requestExpNum = MTK_STAGGER_VALID_EXPOSURE_NON;
        }
    }

    MY_S_LOGD_IF(1 /*getDebugLevel()*/, "frameType[%d] requestAETargetMode=%d requestExpNum=%d",
                   frameType, requestAETargetMode, requestExpNum);

    return true;
}

bool HDRPolicyHelper::isConfigHdrOff() {
    bool cfgOff = false;
    if (mHDRStateEvaluator != NULL) {
        cfgOff = mHDRStateEvaluator->isConfigHdrOff();
    }
    return cfgOff;
}

auto
HDRPolicyHelper::
isAppHDR(
) -> bool
{
    return ( mHDRAppMode == HDRMode::ON || isAppVideoHDR() );
}

auto
HDRPolicyHelper::
isAppVideoHDR(
) -> bool
{
    return ( mHDRAppMode == HDRMode::VIDEO_ON || mHDRAppMode == HDRMode::VIDEO_AUTO );
}

auto
HDRPolicyHelper::
isHalAppHDR(
) -> bool
{
    return ( mHDRHalAppMode == HDRMode::ON || isHalAppVideoHDR() );
}

auto
HDRPolicyHelper::
isHalAppVideoHDR(
) -> bool
{
    return ( mHDRHalAppMode == HDRMode::VIDEO_ON || mHDRHalAppMode == HDRMode::VIDEO_AUTO );
}


auto
HDRPolicyHelper::
isHalHDR(
) -> bool
{
    return ( mHDRHalMode != MTK_HDR_FEATURE_HDR_HAL_MODE_OFF );
}

auto
HDRPolicyHelper::
updateAppConfigMode(
    HDRMode appHdrMode
) -> bool
{
    if( !mConfigured ) {
        bool isAPPChanged = ( mHDRAppMode != appHdrMode );
        bool isHalAppSwitchOff = isHDR();
        updateAppHDRMode(appHdrMode, appHdrMode);
        bool isHalAppSwitchOn = isHDR();
        if (appHdrMode == HDRMode::VIDEO_ON) {
            mHDRRcfgInfo.hdrState = toHDRState(mHDRHalMode);
            mHDRStateEvaluator = std::make_shared<HDRStateEvaluator>(mSensorId, mHDRRcfgInfo.hdrState);
        } else {
            mHDRRcfgInfo.hdrState = toHDRState(MTK_HDR_FEATURE_HDR_HAL_MODE_OFF);
        }

        MY_S_LOGD_IF(getDebugLevel(),
            "[CONFIG]inAppHdrMode(%hhu):AppHdrMode(%hhu),HalAppHdrMode(%hhu) "
            "HDRState(%d)",
            appHdrMode, mHDRAppMode, mHDRHalAppMode, mHDRRcfgInfo.hdrState);
        mConfigured = true;
    }
    return true;
}

auto
HDRPolicyHelper::
updateAppRequestMode(
    HDRMode appHdrMode,
    REQUEST_TYPE reqType,
    HDRMode appHDRIntent
) -> bool
{
    bool isAPPChanged = ( mHDRAppMode != appHdrMode );
    HDRMode outAppHdrMode = (appHdrMode == HDRMode::ON ) ?
                            HDRMode::OFF /* Capture configure as off */ : appHdrMode;
    HDRReconfigInfo hdrRcfgInfo = strategyAppHDRMode(outAppHdrMode, mHDRHalAppMode, reqType, appHDRIntent);
    bool isStrategyChanged = false;
    if (hdrRcfgInfo.hdrProcState == STATE_PROCESSING) {
        mHDRRcfgInfo.hdrProcState = STATE_PROCESSING;
        mHDRRcfgInfo.reqHdrState = hdrRcfgInfo.reqHdrState;
    }
    else if ((hdrRcfgInfo.hdrProcState == STATE_DONE) &&
             (hdrRcfgInfo.hdrState != mHDRRcfgInfo.hdrState)) {
        mHDRRcfgInfo = hdrRcfgInfo;
        isStrategyChanged = true;
    }

    MY_S_LOGD_IF(getDebugLevel(), "AppHdrMode(%hhu=>%hhu %hhu),reqType(%d)=>reqHdrState(%d),isAPPChanged(%d),isStrategyChanged(%d)",
                 mHDRAppMode, appHdrMode, outAppHdrMode, reqType, hdrRcfgInfo.reqHdrState, isAPPChanged, isStrategyChanged);

    if( isAPPChanged || isStrategyChanged ) {
        bool isCaptureSwitch = ( mHDRAppMode == HDRMode::ON || appHdrMode == HDRMode::ON );
        bool isHalAppSwitchOff = isHDR();
        HDRMode outHalAppHdrMode = (mHDRRcfgInfo.hdrState <= HDR_STATE_1EXP) ? HDRMode::OFF :
                                                                               HDRMode::VIDEO_ON;
        updateAppHDRMode(appHdrMode/*App*/, outHalAppHdrMode/*HalApp*/);
        bool isHalAppSwitchOn = isHDR();
        // Determine HAL reconfiguration
        if( isAPPChanged && isCaptureSwitch )
        {
            // Triggerred by
            // 1. Capture frame: Config HalAPP as OFF to make HDR OFF pipeline
            // 2. First streaming after capture frame: APP (ON -> video ON) to reconfig as HDR ON pipeline
            #if ENABLE_CAPTURE_HAL_RECONFIG
            mNeedReconfigure = true;
            #endif
        }
        else if( isHalAppSwitchOff || isHalAppSwitchOn )
        {
            if( mStrategyMode == STRATEGY_MODE_HAL_RECONFIG && isStrategyChanged ) {
                 mNeedReconfigure = true;
            }
        }
    }

    return true;
}

auto
HDRPolicyHelper::
updateAppHDRMode(
    HDRMode appHdrMode,
    HDRMode halAppHdrMode
) -> void
{
    if( !mForceAppMode ) {
        if( mHDRAppMode != appHdrMode ||
            mHDRHalAppMode != halAppHdrMode ) {
            MY_S_LOGD("App HDR mode changed: HDRApp(%hhu=>%hhu),HDRHalApp(%hhu=>%hhu)",
                    mHDRAppMode, appHdrMode, mHDRHalAppMode, halAppHdrMode);
            mHDRAppMode = appHdrMode;
            mHDRHalAppMode = halAppHdrMode;
        }
    }
}

auto
HDRPolicyHelper::
strategyAppHDRMode(
    HDRMode inAppHdrMode,
    HDRMode hdrHalAppMode,
    REQUEST_TYPE reqType,
    HDRMode appHDRIntent
) -> HDRReconfigInfo
{
    MY_TRACE_API_LIFE();

    if( isAppHDR() && isHalHDR() ) {
        if( mEnableBypassMode && (isMulitFrameHDR() || isStaggerHDR()) ) {
            if( reqType == REQUEST_STREAMING || reqType == REQUEST_RECORD ) {
                return evaluateHDRState(inAppHdrMode, hdrHalAppMode, appHDRIntent);
            }
        }
        else if( mHDRHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_MVHDR ) {
            if( reqType == REQUEST_STREAMING ) {
                return evaluateHDRState(inAppHdrMode, hdrHalAppMode, appHDRIntent);
            }
        }
    }
    return mHDRRcfgInfo;
}

auto
HDRPolicyHelper::
evaluateHDRState(
    HDRMode inAppHdrMode,
    HDRMode hdrHalAppMode,
    HDRMode appHDRIntent
) -> HDRReconfigInfo
{
    MY_TRACE_API_LIFE();
    HDRReconfigInfo hdrRcfgInfo;

    if (inAppHdrMode == HDRMode::OFF) {
        MY_LOGI("APP set HDRMode::OFF, bypass evaluateHDRState");
        hdrRcfgInfo.hdrState = HDR_STATE_1EXP;
        return hdrRcfgInfo;
    }

    if (mHDRStateEvaluator != NULL)
    {
        hdrRcfgInfo = mHDRStateEvaluator->evaluateHDRState(mHDRRcfgInfo, appHDRIntent);
    }
    else if (inAppHdrMode == HDRMode::VIDEO_ON) {
        MY_S_LOGE("HDRMode == VIDEO_ON but mHDRStateEvaluator == NULL");
    }

    MY_S_LOGD_IF(mHDRRcfgInfo.hdrState != hdrRcfgInfo.reqHdrState, "HDR state changed: (%hhu=>%hhu)",
                 mHDRRcfgInfo.hdrState, hdrRcfgInfo.reqHdrState);

    return hdrRcfgInfo;
}

auto
AdditionalFrameInfo::
addOneFrame(
    FRAME_TYPE frameType,
    uint32_t p1Dma,
    std::shared_ptr<NSCam::IMetadata> additionalApp,
    std::shared_ptr<NSCam::IMetadata> additionalHal
) -> bool
{
    MY_TRACE_API_LIFE();

    std::shared_ptr<NSCam::IMetadata>  outMetaApp = std::make_shared<IMetadata>();
    std::shared_ptr<NSCam::IMetadata>  outMetaHal = std::make_shared<IMetadata>();
    if (additionalApp.get() != nullptr) {
        *outMetaApp += *additionalApp;
    }
    if (additionalHal.get() != nullptr) {
        *outMetaHal += *additionalHal;
    }

    FrameInfo newFrame {
        .p1Dma = p1Dma,
        .appMetadata = outMetaApp,
        .halMetadata = outMetaHal,
    };

    switch (frameType) {
    case FRAME_TYPE_MAIN:
        mAdditionalFrameSet.mainFrame = newFrame;
        break;
    case FRAME_TYPE_PRESUB:
        mAdditionalFrameSet.preSubFrame.push_back(newFrame);
        break;
    case FRAME_TYPE_PREDUMMY:
        mAdditionalFrameSet.preDummy.push_back(newFrame);
        break;
    case FRAME_TYPE_SUB:
        mAdditionalFrameSet.subFrame.push_back(newFrame);
        break;
    case FRAME_TYPE_POSTDUMMY:
        mAdditionalFrameSet.postDummy.push_back(newFrame);
        break;
    case FRAME_TYPE_UNKNOWN:
    default:
        MY_LOGW("No push invalid frametype(%d)", frameType);
        return false;
    }

    return true;
}

auto
AdditionalFrameInfo::
getAdditionalFrameSet(
) -> AdditionalFrameSet
{
    return mAdditionalFrameSet;
}

SensorMap<std::shared_ptr<HDRPolicyHelper>>
    createHDRPolicyHelper(const std::vector<int32_t>& sensorIdList) {
    SensorMap<std::shared_ptr<HDRPolicyHelper>> hdrPolicyHelper;
    for (const auto& hdrPolicy : getHDRPolicyMap(sensorIdList)) {
        auto hdrHelper = std::make_shared<HDRPolicyHelper>(
            hdrPolicy.first, hdrPolicy.second);
        hdrPolicyHelper.emplace(hdrPolicy.first, hdrHelper);
    }
    return hdrPolicyHelper;
}

HDRState HDRPolicyHelper::toHDRState(const uint32_t hdrHalMode) {
    switch (hdrHalMode) {
    case MTK_HDR_FEATURE_HDR_HAL_MODE_OFF:
        return HDR_STATE_NONE;
        break;
    case MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE:
    case MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW:
    case MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW:
    case MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP:
        return HDR_STATE_2EXP;
        break;
    case MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP:
        return HDR_STATE_3EXP;
        break;
    default:
        MY_LOGI("Unsupported hdrHalMode %d", hdrHalMode);
        return HDR_STATE_NONE;
    }
}

}
}
}
}
}


