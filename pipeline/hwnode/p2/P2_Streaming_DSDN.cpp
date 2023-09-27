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
 * MediaTek Inc. (C) 2017. All rights reserved.
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

#include <cutils/properties.h>
#include "P2_StreamingProcessor.h"

#include "P2_DebugControl.h"
#define P2_CLASS_TAG    Streaming_DSDN
#define P2_TRACE        TRACE_STREAMING_DSDN
#include "P2_LogHeader.h"

#include <camera_custom_dsdn.h>
#include <mtkcam3/feature/stereo/StereoCamEnum.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_P2_STR_PROC);

#define F_ON    1
#define F_OFF   0
#define F_AUTO (-1)

    // -1: auto, 0: off 1: DSDN2.0 on, 2: DSDN2.5 on
#define KEY_P2S_DSDN_MODE           "vendor.debug.p2s.dsdn.mode"
#define KEY_P2S_DSDN_THRESHOLD      "vendor.debug.p2s.dsdn.threshold"
#define KEY_P2S_DSDN_MAX_LAYER      "vendor.debug.p2s.dsdn.maxlayer"
#define KEY_P2S_DSDN_SLAVE          "vendor.debug.p2s.dsdn.slave" // -1 : auto , 0: off. 1:on
#define KEY_P2S_DSDN_P1_CTRL        "vendor.debug.p2s.dsdn.p1ctrl" // -1 : auto , 0: off. 1:on
#define KEY_P2S_DSDN_P1_DS_RATIO    "vendor.debug.p2s.dsdn.p1dsRatio" // 0 : auto , N : ratio = N/100
#define KEY_P2S_DSDN_FLAG           "vendor.debug.p2s.dsdn.flag"
#define ISO_OFFSET 800
#define DSDN_MIN_IN_WIDTH 640
#define DSDN_MIN_IN_HEIGHT 320

using NSCam::NSCamFeature::NSFeaturePipe::MASK_DSDN20;
using NSCam::NSCamFeature::NSFeaturePipe::MASK_DSDN25;
using NSCam::NSCamFeature::NSFeaturePipe::MASK_DSDN30;
using namespace NSCam::v3;
using NSCam::Feature::DSDNCache;
using NSCam::NSCamFeature::DSDNParam;
using NSCam::NSCamFeature::DSDNRatio;

namespace P2
{

MVOID StreamingProcessor::initDSDN(const P2ConfigInfo &config)
{
    DSDNCache::getInstance()->init();
    DSDNCache::getInstance()->setData(DSDNCache::Data());
    for( MUINT32 sID : config.mAllSensorID )
    {
        mDSDNState[sID] = MFALSE;
        mDSDNTargetState[sID] = MFALSE;
    }
}

MVOID StreamingProcessor::uninitDSDN()
{
    DSDNCache::getInstance()->setData(DSDNCache::Data());
    DSDNCache::getInstance()->uninit();
}

DSDNCustom::DUAL_MODE parseDSDNDual(MUINT32 dualMode)
{
    DSDNCustom::DUAL_MODE mode = DSDNCustom::DUAL_MODE_SINGLE;
    if( (dualMode & NSCam::v1::Stereo::E_STEREO_FEATURE_VSDOF)
        || (dualMode & NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP) )
    {
        mode = DSDNCustom::DUAL_MODE_HW_DEPTH;
    }
    else if( dualMode != 0 )
    {
        mode = DSDNCustom::DUAL_MODE_OTHER;
    }
    return mode;
}

DSDNCustom::ScenarioParam prepareSceneParam(const P2ConfigInfo &config, MBOOL isAdvEis)
{
    DSDNCustom::ScenarioParam sceneParam;
    sceneParam.hasAdvEIS = isAdvEis;
    sceneParam.videoW = config.mUsageHint.mOutCfg.mVideoSize.w;
    sceneParam.videoH = config.mUsageHint.mOutCfg.mVideoSize.h;
    sceneParam.streamW = config.mUsageHint.mStreamingSize.w;
    sceneParam.streamH = config.mUsageHint.mStreamingSize.h;
    sceneParam.smvrFps = (config.mP2Type == P2_BATCH_SMVR) ? 30 * config.mUsageHint.mInCfg.mP1Batch : 0;
    sceneParam.fps = config.mUsageHint.mInCfg.mReqFps;
    sceneParam.dualMode = parseDSDNDual(config.mUsageHint.mDualMode);
    return sceneParam;
}

DSDNParam::MODE convertTo(MUINT32 mode)
{
    if (NSCam::IImageBufferAllocator::queryIonVersion() == NSCam::eION_VERSION_AOSP)
    {
        MY_LOGE("Temp close DSDN for GKI");
        mode = 0;
    }
    switch(mode)
    {
    case DSDNCustom::DSDN_MODE_OFF  : return DSDNParam::MODE_OFF;
    case DSDNCustom::DSDN_MODE_20   : return DSDNParam::MODE_20;
    case DSDNCustom::DSDN_MODE_25   : return DSDNParam::MODE_25;
    case DSDNCustom::DSDN_MODE_30   : return DSDNParam::MODE_30;
    default: return DSDNParam::MODE_OFF;
    }
}

DSDNCache::Data StreamingProcessor::prepareDSDNCache(const ILog &log, MBOOL targetRun, const P2PlatInfo::NVRamDSDN &nvram)
{
    DSDNRatio outRatio(nvram.mP1OutRatioMultiple, nvram.mP1OutRatioDivider);
    const DSDNRatio &maxRatio = mPipeUsageHint.mDSDNParam.mMaxP1Ratio;
    if( mDSDNDebugP1Ctrl == F_ON )
    {
        mDSDNDebugP1DsRatio = std::min<MINT32>(property_get_int32(KEY_P2S_DSDN_P1_DS_RATIO, 0), 100);
        outRatio = (mDSDNDebugP1DsRatio > 0) ? DSDNRatio(mDSDNDebugP1DsRatio, 100) : outRatio;
    }
    else if( maxRatio < outRatio )
    {
        MY_S_LOGW(log, "NVRam p1 ratio (%d/%d) > maxAllowRatio(%d/%d), use maxAllowRatio", outRatio.mMul, outRatio.mDiv, maxRatio.mMul, maxRatio.mDiv);
        outRatio = maxRatio;
    }

    bool p1Ctrl = true;
    return DSDNCache::Data(targetRun, p1Ctrl, outRatio.mMul, outRatio.mDiv);
}

NSCam::NSCamFeature::DSDNParam StreamingProcessor::queryDSDNParam(const P2ConfigInfo &config)
{
    TRACE_FUNC_ENTER();
    DSDNCustom::ScenarioParam sceneParam = prepareSceneParam(config, isAdvEIS(config.mUsageHint.mEisInfo.mode));
    DSDNCustom::Config dsdnConfig = DSDNCustom::getInstance()->getConfig(sceneParam);

    MINT32 debugMode = property_get_int32(KEY_P2S_DSDN_MODE, F_AUTO);
    mDSDNDebugISO_H = property_get_int32(KEY_P2S_DSDN_THRESHOLD, 0);
    MINT32 debugLayer = property_get_int32(KEY_P2S_DSDN_MAX_LAYER, -1);
    mDSDNDebugP1Ctrl = property_get_int32(KEY_P2S_DSDN_P1_CTRL, F_AUTO);
    mDSDNDebugSlave = property_get_int32(KEY_P2S_DSDN_SLAVE, F_AUTO);
    mDSDNDebugFlag = property_get_int32(KEY_P2S_DSDN_FLAG, -1);

    if(config.mUsageHint.mDsdnHint == 0) // Pipeline force off
    {
        dsdnConfig.mode = DSDNCustom::DSDN_MODE_OFF;
    }
#ifndef SUPPORT_VPU
    if(dsdnConfig.mode == DSDNCustom::DSDN_MODE_20)
    {
        dsdnConfig.mode = DSDNCustom::DSDN_MODE_OFF;
        MY_LOGE("DSDN Mode = DSDN20, but VPU not support! Force disable DSDN");
    }
#endif
    dsdnConfig.flag = ( mDSDNDebugFlag != -1 ) ? mDSDNDebugFlag : dsdnConfig.flag;

    NSCam::NSCamFeature::DSDNParam out;
    out.mMode = convertTo( (debugMode >= 0) ? debugMode : (MUINT32)dsdnConfig.mode );
    out.mMaxDSLayer = (debugLayer > 0) ? (MUINT32)debugLayer : dsdnConfig.maxLayer;
    out.mDynamicP1Ctrl = (mDSDNDebugP1Ctrl == F_ON) || (mDSDNDebugP1Ctrl != F_OFF && dsdnConfig.mControlP1Out);
    out.mMaxP1Ratio = DSDNRatio(std::max<MUINT32>(dsdnConfig.maxP1RatioMultiple, 1), std::max<MUINT32>(dsdnConfig.maxP1RatioDivider, 1));
    out.mMaxDsRatio = DSDNRatio(std::max<MUINT32>(dsdnConfig.maxRatioMultiple, 1), std::max<MUINT32>(dsdnConfig.maxRatioDivider, 1));
    out.mDSDN20_10Bit = dsdnConfig.flag & DSDNCustom::DFLAG_20_10bit ;
    out.mDSDN30_GyroEnable = dsdnConfig.flag & DSDNCustom::DFLAG_GYRO_ENABLE ;
    MY_LOGD("dsdn30 gyro enable: %d", out.mDSDN30_GyroEnable);
    TRACE_FUNC_EXIT();
    return out;
}

MBOOL StreamingProcessor::prepareDSDN(P2Util::SimpleIn &input, const ILog &log)
{
    MBOOL run = MFALSE;
    if( mPipeUsageHint.mDSDNParam.isDSDN() )
    {
        MBOOL targetRun = MFALSE;
        MUINT32 sID = input.getSensorId();
        const P2SensorData &sensorData = input.mRequest->mP2Pack.getSensorData(sID);
        const P2PlatInfo::NVRamDSDN &nvramData = sensorData.mNvramDsdn;
        MSize inSize = (input.mIMGI != NULL) ? input.mIMGI->getImgSize() : MSize(0,0);
        MINT32 isoH = (mDSDNDebugISO_H > 0) ? mDSDNDebugISO_H : nvramData.mIsoThreshold;
        MINT32 isoL = (isoH > ISO_OFFSET) ? isoH - ISO_OFFSET : isoH;
        MINT iso = sensorData.mISO;
        MBOOL allowNR = input.isMaster() || (sensorData.mStreamingNR == MTK_DUALZOOM_STREAMING_NR_AUTO) || (mDSDNDebugSlave == F_ON);
        targetRun = allowNR && (isoH != 0) && ( (iso >= isoH) || (mDSDNTargetState[sID] && iso >= isoL) );

        if( mPipeUsageHint.mDSDNParam.mDynamicP1Ctrl )
        {
            DSDNCache::getInstance()->setData( prepareDSDNCache(log, targetRun, nvramData) );
            // Get run or not in this frame
            IMetadata *inHal = input.mRequest->getMetaPtr(IN_P1_HAL);
            run = (tryGetMetadata<MBOOL>(inHal, MTK_P2NODE_DSDN_ENABLE, run) && run);
        }
        else
        {
            run = targetRun;
        }
        run = (run && (inSize.w >= DSDN_MIN_IN_WIDTH) && (inSize.h >= DSDN_MIN_IN_HEIGHT));
        TRACE_S_FUNC(log, "s(%d), allow(%d)  isMaster(%d) DSDN_%s target=%d, run=%d iso=%d(%d:%d), nvram threshold(%d), debugThreshold(%d), ratio(m/d)=(%d,%d), initRatio(m/d)P1(m/d)=(%d,%d),(%d/%d) nvramP1Ratio(m/d)=(%d,%d), dbgP1Ctrl/R(%d/%d), in=%dx%d",
                            sID, allowNR, input.isMaster(), mPipeUsageHint.mDSDNParam.isDSDN20() ? "20" : mPipeUsageHint.mDSDNParam.isDSDN25() ? "25" : "30",
                            targetRun, run, iso, isoL, isoH, nvramData.mIsoThreshold, mDSDNDebugISO_H, nvramData.mRatioMultiple, nvramData.mRatioDivider,
                            mPipeUsageHint.mDSDNParam.mMaxDsRatio.mMul, mPipeUsageHint.mDSDNParam.mMaxDsRatio.mDiv, mPipeUsageHint.mDSDNParam.mMaxP1Ratio.mMul, mPipeUsageHint.mDSDNParam.mMaxP1Ratio.mDiv,
                            nvramData.mP1OutRatioMultiple, nvramData.mP1OutRatioDivider, mDSDNDebugP1Ctrl, mDSDNDebugP1DsRatio, inSize.w, inSize.h);
        MUINT32 mask = mPipeUsageHint.mDSDNParam.isDSDN20() ? MASK_DSDN20 : mPipeUsageHint.mDSDNParam.isDSDN25() ? MASK_DSDN25 : MASK_DSDN30;
        input.mFeatureParam.setFeatureMask(mask, run);
        mDSDNState[sID] = run;
        mDSDNTargetState[sID] = targetRun;
        MY_LOGD_IF(mP2Info.getConfigInfo().mAutoTestLog, "[CAT][DSDN] threshold:%d sId:%d mode:%s", nvramData.mIsoThreshold, sID, mPipeUsageHint.mDSDNParam.toModeStr());
    }
    return run;
}

} // namespace P2
