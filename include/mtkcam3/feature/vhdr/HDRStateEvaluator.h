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

#ifndef _HDR_STATE_EVALUATOR_H_
#define _HDR_STATE_EVALUATOR_H_
#define MAX_FLICKER_BUFF 10
#define CONFIG_HDR_OFF_INDEX 2

#include <mtkcam3/feature/hdrDetection/Defs.h>
#include <mtkcam/aaa/aaa_hal_common.h>
#include <mtkcam/aaa/IHal3A.h>
#include <mtkcam/aaa/IHalISP.h>
#include <string.h>

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {

enum HDRProcState {
    STATE_PROCESSING = 0,
    STATE_DONE,
    STATE_IDLE,
};
// !! Do not change the order !!
enum HDRState {
    HDR_STATE_NONE = 0,
    HDR_STATE_1EXP,
    HDR_STATE_2EXP,
    HDR_STATE_3EXP,
};
struct HDRReconfigInfo {
    HDRProcState hdrProcState = STATE_IDLE;
    HDRState hdrState = HDR_STATE_NONE;
    HDRState reqHdrState = HDR_STATE_NONE;
};

class HDRStateEvaluator
{
public:
    static const int ISO_LARGER_AND_STABLE = 0;
    static const int ISO_SMALLER_AND_STABLE = 1;
    static const int ISO_UNSTABLE = 2;
    static const int ISO_UNKNOWN = -1;

    static const int FLICKER_NONE = 0;
    static const int FLICKER_50HZ = 1;
    static const int FLICKER_60HZ = 2;
    static const int FLICKER_UNKNOWN = -1;

    static const int DR_HIGH_STABLE = 0;
    static const int DR_LOW_STABLE = 1;
    static const int DR_UNSTABLE = 2;
    static const int DR_UNKNOWN = -1;

    HDRStateEvaluator(MUINT32 sensorId, HDRState initialState);
    ~HDRStateEvaluator();

    HDRReconfigInfo evaluateHDRState(const HDRReconfigInfo inHdrRcfgInfo, const HDRMode appHDRIntent);
    bool    isConfigHdrOff();

private:
    enum EnvironmentStatus
    {
        eEnv_HighLight,
        eEnv_LowLight,
    };

    typedef struct
    {
        MINT32 flick_50hz_conf_th;
        MINT32 flick_60hz_conf_th;
        MUINT32 flick_50hz_framecnt_th;
        MUINT32 flick_60hz_framecnt_th;
    }FlickPara;

    HDRState checkISOThres(const MUINT32 isoIdx1Thres, const MUINT32 isoIdx2Thres);
    HDRState checkHDRRatio();
    HDRState checkDynamicRange();
    HDRState checkShutter();
    HDRState checkPowerThermal();
    bool switchHandle_NormalToHDR(MUINT32 shutter_result);
    bool switchHandle_HDRToNormal(MUINT32 shutter_result);
    bool checkFlicker_NormalToHDR();
    bool checkFlicker_HDRToNormal();
    HDRState checkFlicker();
    bool resetISOThresStatus(const MUINT32 isoIdx1Thres, const MUINT32 isoIdx2Thres);
    std::string printHDRState(const HDRState state);
    std::string printHDRProcState(const HDRProcState procState);
    std::string printSeamlessResult(const int32_t seamless_smooth_result);

    EnvironmentStatus    mCurrentEnvironment = eEnv_HighLight;
    //HDRMode                         mHDRMode = HDRMode::OFF;
    NS3Av3::VHDRReconfigInfo_T               mAeReconfigInfo;
    HDRReconfigInfo                 mHdrRcfgInfo;
    const HDRState       mInitialHDRState = HDR_STATE_NONE;

    MUINT32                      mDebugLevel = 0;
    MUINT32                    mDebugHDRMode = 0;
    MUINT32                 mDebugNormalMode = 0;
    MUINT32              mISOIdx1StableCount = 0;
    MUINT32              mISOIdx2StableCount = 0;
    MUINT32            mIdx1StableCountThres = 0;
    MUINT32            mIdx2StableCountThres = 0;
    MUINT32               mISOIdx1Thres_50Hz = 0;
    MUINT32               mISOIdx2Thres_50Hz = 0;
    MUINT32               mISOIdx1Thres_60Hz = 0;
    MUINT32               mISOIdx2Thres_60Hz = 0;
    MUINT32                        mSensorId = 0;
    bool                 mIsoInitialized = false;
    MUINT32               mShutterStbCntHigh = 0;
    MUINT32                mShutterStbCntLow = 0;
    MUINT32             mShutterStbCntHighTh = 0;
    MUINT32              mShutterStbCntLowTh = 0;
    MUINT32              mShutterHighTh_3exp = 0;
    MUINT32               mShutterLowTh_3exp = 0;
    MUINT32              mShutterHighTh_2exp = 0;
    MUINT32               mShutterLowTh_2exp = 0;
    MUINT32                    mDRStbCntHigh = 0;
    MUINT32                     mDRStbCntLow = 0;
    MUINT32                mDRHightTh_Normal = 0;
    MUINT32                  mDRLowTh_Normal = 0;
    MUINT32           mDRStbCntHighTh_Normal = 0;
    MUINT32            mDRStbCntLowTh_Normal = 0;
    MUINT32                   mDRHightTh_HDR_3exp = 0;
    MUINT32                     mDRLowTh_HDR_3exp = 0;
    MUINT32              mDRStbCntHighTh_HDR_3exp = 0;
    MUINT32               mDRStbCntLowTh_HDR_3exp = 0;
    MUINT32                   mDRHightTh_HDR_2exp = 0;
    MUINT32                     mDRLowTh_HDR_2exp = 0;
    MUINT32              mDRStbCntHighTh_HDR_2exp = 0;
    MUINT32               mDRStbCntLowTh_HDR_2exp = 0;
    MUINT32                    mHDRRatioStbCntHigh = 0;
    MUINT32                     mHDRRatioStbCntLow = 0;
    MUINT32 mHDRRatioHighTh_HDR_3exp        = 0;
    MUINT32 mHDRRatioLowTh_HDR_3exp         = 0;
    MUINT32 mHDRRatioStbCntHighTh_HDR_3exp  = 0;
    MUINT32 mHDRRatioStbCntLowTh_HDR_3exp   = 0;
    MUINT32 mHDRRatioHightTh_HDR_2exp       = 0;
    MUINT32 mHDRRatioLowTh_HDR_2exp         = 0;
    MUINT32 mHDRRatioStbCntHighTh_HDR_2exp  = 0;
    MUINT32 mHDRRatioStbCntLowTh_HDR_2exp   = 0;
    bool                          mIsHDR = false;
    MUINT32                 flicker_50hz_cnt = 0;
    MUINT32                 flicker_60hz_cnt = 0;
    MUINT32                flicker_frame_cnt = 0;
    MINT32   flicker_buff[MAX_FLICKER_BUFF] = {0};
    MUINT32                      seamlessVer = 0;
    MUINT32                     dr_th_Normal = 0;
    MUINT32                        dr_th_HDR = 0;
    MINT32                        seam_BV_th = 0;
    FlickPara                     strHdrToNormal;
    FlickPara                     strNormalToHDR;

    // If HDR on in low light, bypass seamless smooth and disable HDR directly
    bool                      mCfgHdrOff = false;
    MUINT32                    mCfgHdrOffCnt = 0;

    NS3Av3::IHal3A*                  mpHal3a = NULL;
    NS3Av3::IHalISP*                mpHalISP = NULL;
};

}
}
}
}
}
#endif  // _HDR_STATE_EVALUATOR_H_

