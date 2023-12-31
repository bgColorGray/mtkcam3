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
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/

/**
* @file HDRPolicyHelper.h
*
* HDRPolicyHelper Header File
*
*/

#ifndef _HDR_POLICY_HELPER_H_
#define _HDR_POLICY_HELPER_H_


#include <utils/String8.h>
#include <mtkcam/def/common.h>
#include <mtkcam/aaa/aaa_hal_common.h>
#include <custom/aaa/ae_param.h>
#include <mtkcam/aaa/IHal3A.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam3/feature/hdrDetection/Defs.h>
#include <mtkcam3/feature/vhdr/HDRStateEvaluator.h>
#include <mtkcam3/pipeline/def/types.h>


/**
  *@brief HDRPolicyHelper
*/

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {




enum FRAME_TYPE
{
    FRAME_TYPE_UNKNOWN = 0,
    FRAME_TYPE_PRESUB,
    FRAME_TYPE_PREDUMMY,
    FRAME_TYPE_MAIN,
    FRAME_TYPE_SUB,
    FRAME_TYPE_POSTDUMMY,
};

class AdditionalFrameInfo
{
public:

    struct FrameInfo
    {
        uint32_t p1Dma = 0;
        std::shared_ptr<NSCam::IMetadata> appMetadata = nullptr;
        std::shared_ptr<NSCam::IMetadata> halMetadata = nullptr;
    };

    struct AdditionalFrameSet
    {
        FrameInfo  mainFrame;
        std::vector<FrameInfo> preSubFrame;
        std::vector<FrameInfo> subFrame;
        std::vector<FrameInfo> preDummy;
        std::vector<FrameInfo> postDummy;
    };

    AdditionalFrameInfo() {}

    // TODO:
    //addFrameSet(AdditionalFrameSet& additionalFrameSet);
    // TODO:
    //addOneFrame(FRAME_TYPE frameType, FrameInfos& additionalFrame);
    bool addOneFrame(FRAME_TYPE frameType, uint32_t p1Dma, std::shared_ptr<NSCam::IMetadata> additionalApp, std::shared_ptr<NSCam::IMetadata> additionalHal);
    AdditionalFrameSet getAdditionalFrameSet();
private:
    AdditionalFrameSet mAdditionalFrameSet;
};

class HDRPolicyHelper final
{
public:
    HDRPolicyHelper(int32_t sensorId, uint32_t hdrHalMode);

    enum REQUEST_TYPE
    {
        REQUEST_STREAMING,
        REQUEST_CAPTURE,
        REQUEST_RECORD,
    };

    enum STRATEGY_MODE
    {
        STRATEGY_MODE_HAL_RECONFIG,
        STRATEGY_MODE_HAL_BYPASS,
    };

    enum SENSOR_TYPE
    {
        SENSOR_TYPE_NORMAL,
        SENSOR_TYPE_4CELL,
    };

    static const uint32_t GROUP_SIZE_NONE    = 0;
    static const uint32_t GROUP_SIZE_MSTREAM = 2;

    static uint32_t toHDRHalMode(uint32_t vHdrMode);
    static uint32_t toHDRSensorMode(uint32_t hdrHalMode);
    static STRATEGY_MODE toHDRStrategyMode(uint32_t hdrHalMode);

    bool negotiateRequestPolicy(AdditionalFrameInfo& additionalFrame);

    HDRMode getHDRAppMode();
    uint32_t getHDRHalMode();
    uint32_t getHDRHalRequestMode();
    uint32_t getHDRSensorMode();
    uint32_t getHDRExposure(FRAME_TYPE type);
    uint32_t getAETargetMode();
    uint32_t getFusionNum(FRAME_TYPE type);
    uint32_t getValidExposureNum();
    bool     getSeamlessSmoothResult(FRAME_TYPE type, uint32_t& requestAETargetMode, uint32_t& requestExpNum);
    bool     isConfigHdrOff();

    size_t getGroupSize();
    size_t getDummySize();
    uint32_t getDebugLevel();
    android::String8 getDebugMessage();

    bool isHDR();
    bool isHDRConfig();
    bool isVideoHDRConfig();
    bool isZSLHDR();
    bool isMulitFrameHDR();
    bool isStaggerHDR();
    bool needReconfiguration();
    bool notifyDummy();
    bool notifyManualExp();
    bool updateAppConfigMode(HDRMode appHdrMode);
    bool updateAppRequestMode(HDRMode appHdrMode, REQUEST_TYPE reqType, HDRMode appHDRIntent = HDRMode::ON);
    bool handleReconfiguration();

protected:

    enum SwitchModeStatus
    {
        eSwitchMode_HighLightMode,
        eSwitchMode_LowLightMode,
    };

    static SENSOR_TYPE getSensorType(int32_t sensorId);

    bool isAppHDR();
    bool isAppVideoHDR();
    bool isHalAppHDR();
    bool isHalAppVideoHDR();
    bool isHalHDR();
    bool needDummy();
    bool needPreSubFrame();
    void updateAppHDRMode(HDRMode appHdrMode, HDRMode halAppHdrMode);
    HDRReconfigInfo strategyAppHDRMode(HDRMode inAppHdrMode, HDRMode hdrHalAppMode,
        REQUEST_TYPE reqType, HDRMode appHDRIntent);
    HDRReconfigInfo evaluateHDRState(HDRMode inAppHdrMode, HDRMode hdrHalAppMode, HDRMode appHDRIntent);

private:
    uint32_t getReqAETargetMode(HDRState hdrState);
    HDRState toHDRState(const uint32_t hdrHalMode);

    HDRMode                               mHDRAppMode = HDRMode::OFF;
    HDRMode                            mHDRHalAppMode = HDRMode::OFF;
    HDRMode                            mHDRRequestMode = HDRMode::OFF;
    uint32_t                              mHDRHalMode = MTK_HDR_FEATURE_HDR_HAL_MODE_OFF;
    uint32_t                           mHDRSensorMode = SENSOR_VHDR_MODE_NONE;
    uint32_t                              mDebugLevel = 0;
    uint32_t                       mEnableBypassMode = 0;
    size_t                                  mNumDummy = 0;
    bool                                    mAddDummy = false;
    bool                               mLMVInvalidity = false;
    bool                                  mConfigured = false;
    bool                             mNeedReconfigure = false;
    bool                                mForceAppMode = false;
    bool                              mForceMainFrame = false;
    SwitchModeStatus             mIsoSwitchModeStatus = eSwitchMode_HighLightMode;
    STRATEGY_MODE                       mStrategyMode = STRATEGY_MODE_HAL_RECONFIG;
    SENSOR_TYPE                           mSensorType = SENSOR_TYPE_NORMAL;
    std::shared_ptr<NS3Av3::IHal3A>            mHal3a = nullptr;
    std::shared_ptr<HDRStateEvaluator> mHDRStateEvaluator = nullptr;
    std::mutex                            mHal3aLocker;
    int32_t                                 mSensorId = 0;
    HDRReconfigInfo                       mHDRRcfgInfo;
};

SensorMap<std::shared_ptr<HDRPolicyHelper>>
    createHDRPolicyHelper(const std::vector<int32_t>& sensorIdList);

}
}
}
}
}
#endif

