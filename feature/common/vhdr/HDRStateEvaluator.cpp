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

#undef LOG_TAG
#define LOG_TAG "mtkcam-HDRStateEvaluator"

#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam3/feature/vhdr/HDRStateEvaluator.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_VHDR_HAL);

// for AE P-line index debounce at max index
#define MAX_INDEX1_STABLE_THRES 20
#define MAX_INDEX2_STABLE_THRES 10
#define ISO1_THRES_VALUE 2000
#define ISO2_THRES_VALUE 6400

#define SHUTER_BASE_60 8333
#define SHUTER_BASE_50 10000


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

#define IF_NULL_RETURN_VALUE(input, ret) \
    if (input == NULL) { \
        MY_LOGE("NULL value!"); \
        return ret; \
    }

using namespace NS3Av3;
using std::string;

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {

HDRStateEvaluator::HDRStateEvaluator(MUINT32 sensorId, HDRState initialState)
    : mSensorId(sensorId),
      mInitialHDRState(initialState)
{
    mDebugLevel = ::property_get_int32("vendor.debug.camera.hdrstate.evaluator", 0);
    mDebugHDRMode = ::property_get_int32("vendor.debug.camera.hdrstate.dbghdrmode", 0);
    mDebugNormalMode = ::property_get_int32("vendor.debug.camera.hdrstate.dbgnormalmode", 0);

    memset(&strNormalToHDR, 0, sizeof(FlickPara));
    memset(&strHdrToNormal, 0, sizeof(FlickPara));

    mpHal3a = MAKE_Hal3A(sensorId, LOG_TAG);
    if(mpHal3a == NULL) {
        MY_LOGE("mpHal3a create failed");
    }
    mpHalISP = MAKE_HalISP(sensorId, LOG_TAG);
    if(mpHalISP == NULL) {
        MY_LOGE("mpHalISP create failed");
    }
}

HDRStateEvaluator::~HDRStateEvaluator()
{
    MY_LOGD("HDRStateEvaluator destructor, unInit 3AHal/HalISP");
    if (mpHal3a != NULL) {
        mpHal3a->destroyInstance(LOG_TAG);
        mpHal3a = NULL;
    }
    if (mpHalISP != NULL) {
        mpHalISP->destroyInstance(LOG_TAG);
        mpHalISP = NULL;
    }
}

HDRReconfigInfo HDRStateEvaluator::evaluateHDRState(const HDRReconfigInfo inHdrRcfgInfo, const HDRMode appHDRIntent)
{
    IF_NULL_RETURN_VALUE(mpHal3a, inHdrRcfgInfo);
    mpHal3a->send3ACtrl(E3ACtrl_GetVHDRReconfigInfo, (MINTPTR)&mAeReconfigInfo, 0);

    if (!mIsoInitialized && (mAeReconfigInfo.seam_tun_para != NULL) && (mAeReconfigInfo.seam_tun_para[0] != 0)) {
        mAeReconfigInfo.i4AESeamlessSmoothResult = STATE_IDLE;
        seamlessVer = mAeReconfigInfo.seam_tun_para[0];
        mIdx1StableCountThres = mAeReconfigInfo.seam_tun_para[1];
        mIdx2StableCountThres = mAeReconfigInfo.seam_tun_para[2];
        mISOIdx1Thres_50Hz = mAeReconfigInfo.seam_tun_para[3];
        mISOIdx2Thres_50Hz = mAeReconfigInfo.seam_tun_para[4];
        mISOIdx1Thres_60Hz = mAeReconfigInfo.seam_tun_para[5];
        mISOIdx2Thres_60Hz = mAeReconfigInfo.seam_tun_para[6];
        mISOIdx1StableCount = mIdx1StableCountThres;
        mISOIdx2StableCount = mIdx2StableCountThres;
        mIsoInitialized = true;
        dr_th_Normal = mAeReconfigInfo.seam_tun_para[21];
        dr_th_HDR = mAeReconfigInfo.seam_tun_para[22];
        strNormalToHDR.flick_50hz_conf_th = mAeReconfigInfo.seam_tun_para[23];
        strNormalToHDR.flick_60hz_conf_th = mAeReconfigInfo.seam_tun_para[24];
        strNormalToHDR.flick_50hz_framecnt_th = mAeReconfigInfo.seam_tun_para[25];
        strNormalToHDR.flick_60hz_framecnt_th = mAeReconfigInfo.seam_tun_para[26];
        strHdrToNormal.flick_50hz_conf_th = mAeReconfigInfo.seam_tun_para[27];
        strHdrToNormal.flick_60hz_conf_th = mAeReconfigInfo.seam_tun_para[28];
        strHdrToNormal.flick_50hz_framecnt_th = mAeReconfigInfo.seam_tun_para[29];
        strHdrToNormal.flick_60hz_framecnt_th = mAeReconfigInfo.seam_tun_para[30];
        mShutterStbCntHighTh = mAeReconfigInfo.seam_tun_para[31];
        mShutterStbCntLowTh = mAeReconfigInfo.seam_tun_para[32];
        mShutterHighTh_3exp = mAeReconfigInfo.seam_tun_para[33];
        mShutterLowTh_3exp = mAeReconfigInfo.seam_tun_para[34];
        mShutterHighTh_2exp = mAeReconfigInfo.seam_tun_para[35];
        mShutterLowTh_2exp = mAeReconfigInfo.seam_tun_para[36];
        mDRHightTh_Normal = mAeReconfigInfo.seam_tun_para[41];
        mDRLowTh_Normal = mAeReconfigInfo.seam_tun_para[42];
        mDRStbCntHighTh_Normal = mAeReconfigInfo.seam_tun_para[43];
        mDRStbCntLowTh_Normal = mAeReconfigInfo.seam_tun_para[44];
        mDRHightTh_HDR_3exp = mAeReconfigInfo.seam_tun_para[45];
        mDRLowTh_HDR_3exp = mAeReconfigInfo.seam_tun_para[46];
        mDRStbCntHighTh_HDR_3exp = mAeReconfigInfo.seam_tun_para[47];
        mDRStbCntLowTh_HDR_3exp = mAeReconfigInfo.seam_tun_para[48];
        mDRHightTh_HDR_2exp = mAeReconfigInfo.seam_tun_para[49];
        mDRLowTh_HDR_2exp = mAeReconfigInfo.seam_tun_para[50];
        mDRStbCntHighTh_HDR_2exp = mAeReconfigInfo.seam_tun_para[51];
        mDRStbCntLowTh_HDR_2exp = mAeReconfigInfo.seam_tun_para[52];

        mHDRRatioHighTh_HDR_3exp        = mAeReconfigInfo.seam_tun_para[55];
        mHDRRatioLowTh_HDR_3exp         = mAeReconfigInfo.seam_tun_para[56];
        mHDRRatioStbCntHighTh_HDR_3exp  = mAeReconfigInfo.seam_tun_para[57];
        mHDRRatioStbCntLowTh_HDR_3exp   = mAeReconfigInfo.seam_tun_para[58];
        mHDRRatioHightTh_HDR_2exp       = mAeReconfigInfo.seam_tun_para[59];
        mHDRRatioLowTh_HDR_2exp         = mAeReconfigInfo.seam_tun_para[60];
        mHDRRatioStbCntHighTh_HDR_2exp  = mAeReconfigInfo.seam_tun_para[61];
        mHDRRatioStbCntLowTh_HDR_2exp   = mAeReconfigInfo.seam_tun_para[62];

        mHdrRcfgInfo.hdrState = mInitialHDRState;

        // Reset ISOThresStatus if needed
        bool ret = (mAeReconfigInfo.i4AEFlickerMode == 0) ?
                   resetISOThresStatus(mISOIdx1Thres_60Hz, mISOIdx2Thres_60Hz) :
                   resetISOThresStatus(mISOIdx1Thres_50Hz, mISOIdx2Thres_50Hz);

        MY_LOGD("[IsoInitialized] isoStableCountThres(%d,%d) isoIdxThreshold_50HZ/60HZ(%d,%d/%d,%d) isoStableCount(%d,%d) seamlessSmoothResult(%s)",
            mIdx1StableCountThres, mIdx2StableCountThres,
            mISOIdx1Thres_50Hz, mISOIdx2Thres_50Hz, mISOIdx1Thres_60Hz, mISOIdx2Thres_60Hz,
            mISOIdx1StableCount, mISOIdx2StableCount,
            printSeamlessResult(mAeReconfigInfo.i4AESeamlessSmoothResult).c_str());

        MY_LOGD("[Seamless]ver:%d, %d/%d, %d/%d/%d/%d, %d/%d/%d/%d\n",
            seamlessVer,dr_th_Normal,dr_th_HDR,
            strNormalToHDR.flick_50hz_conf_th,
            strNormalToHDR.flick_60hz_conf_th,
            strNormalToHDR.flick_50hz_framecnt_th,
            strNormalToHDR.flick_60hz_framecnt_th,
            strHdrToNormal.flick_50hz_conf_th,
            strHdrToNormal.flick_60hz_conf_th,
            strHdrToNormal.flick_50hz_framecnt_th,
            strHdrToNormal.flick_50hz_framecnt_th
            );
    }

    if (!mIsoInitialized) {
        return inHdrRcfgInfo;
    }

    if (mAeReconfigInfo.i4AESeamlessSmoothResult == STATE_DONE) {
       if (mHdrRcfgInfo.hdrProcState == STATE_DONE) {
            // Do nothing, process done may be received multiple times
        } else {
            mHdrRcfgInfo.hdrState = mHdrRcfgInfo.reqHdrState;
            mHdrRcfgInfo.reqHdrState = HDR_STATE_NONE;
            mHdrRcfgInfo.hdrProcState = STATE_DONE;
        }
        //reset cnt
        mDRStbCntHigh = 0;
        mDRStbCntLow = 0;
        mIsHDR = false;
    }
    else if (mAeReconfigInfo.i4AESeamlessSmoothResult == STATE_IDLE) {
        if (mCfgHdrOff) {
            if (mCfgHdrOffCnt == CONFIG_HDR_OFF_INDEX) {
                mCfgHdrOffCnt++;
                mHdrRcfgInfo.hdrState = mHdrRcfgInfo.reqHdrState = HDR_STATE_1EXP;
                MY_LOGD("[mCfgHdrOff] mCfgHdrOffCnt:%d reqHdrState:%s\n",
                    mCfgHdrOffCnt, printHDRState(mHdrRcfgInfo.reqHdrState).c_str());
                return mHdrRcfgInfo;
            }
            mCfgHdrOffCnt++;
        }

        if (mCfgHdrOff && (mCfgHdrOffCnt > CONFIG_HDR_OFF_INDEX)) {
            mCfgHdrOff = false;
        }

        if (mHdrRcfgInfo.hdrProcState != STATE_PROCESSING) {
            //HDRState powerRet = checkPowerThermal();
            HDRState drRet = checkDynamicRange();
            HDRState hdrRatioRet = checkHDRRatio();
            HDRState shutterRet = checkShutter();
            //HDRState switchmode = checkFlicker();
            HDRState isoRet = (mAeReconfigInfo.i4AEFlickerMode == 0) ?
                              checkISOThres(mISOIdx1Thres_60Hz, mISOIdx2Thres_60Hz) :
                              checkISOThres(mISOIdx1Thres_50Hz, mISOIdx2Thres_50Hz);
            //MY_LOGD("[Ret]power:%d, dr:%d, switch:%d, iso:%d, exp:%d, hdrState:%d/%d\n",powerRet,drRet,switchmode,isoRet, mAeReconfigInfo.u4LEExpo, inHdrRcfgInfo.hdrState,mHdrRcfgInfo.hdrState);
            MY_LOGD("[Ret] hdrRatio:%d, dr:%d, iso:%d, exp:%d/%d, hdrState:%d/%d\n", hdrRatioRet, drRet, isoRet, shutterRet, mAeReconfigInfo.u4LEExpo, inHdrRcfgInfo.hdrState, mHdrRcfgInfo.hdrState);

            switch(mHdrRcfgInfo.hdrState)
            {
                case HDR_STATE_1EXP:
                    if(isoRet == HDR_STATE_2EXP)    //customer request condition
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_2EXP;
                    }
                    else
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_NONE;
                    }
                    break;
                case HDR_STATE_2EXP:
                    if(isoRet == HDR_STATE_1EXP)
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_1EXP;
                    }
                    else if((shutterRet == HDR_STATE_3EXP) &&  //wait to add LE-th para
                            (hdrRatioRet == HDR_STATE_3EXP))
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_3EXP;
                    }
                    else
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_NONE;
                    }
                    break;
                case HDR_STATE_3EXP:
                    if((isoRet == HDR_STATE_1EXP) ||
                        (shutterRet == HDR_STATE_2EXP) ||  //wait to add LE-th para
                       (hdrRatioRet == HDR_STATE_2EXP))
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_2EXP;
                    }
                    else
                    {
                        mHdrRcfgInfo.reqHdrState = HDR_STATE_NONE;
                    }
                    break;
                default:
                    mHdrRcfgInfo.reqHdrState = mInitialHDRState;
            }

            mHdrRcfgInfo.hdrProcState = (mHdrRcfgInfo.reqHdrState == HDR_STATE_NONE) ? (STATE_IDLE):(STATE_PROCESSING);
        }
    }
    else {
        // AE algo processing
    }

    MY_S_LOGD_IF((mDebugLevel > 0), "AESeamlessSmoothResult=%s (state,reqState)=(%s,%s) HDRProcState(%s)",
        printSeamlessResult(mAeReconfigInfo.i4AESeamlessSmoothResult).c_str(),
        printHDRState(mHdrRcfgInfo.hdrState).c_str(),
        printHDRState(mHdrRcfgInfo.reqHdrState).c_str(),
        printHDRProcState(mHdrRcfgInfo.hdrProcState).c_str());

    return mHdrRcfgInfo;
}

bool HDRStateEvaluator::isConfigHdrOff() {
    return mCfgHdrOff;
}
HDRState HDRStateEvaluator::checkISOThres(const MUINT32 isoIdx1Thres, const MUINT32 isoIdx2Thres)
{
    HDRState hdrState = mHdrRcfgInfo.hdrState;
    EnvironmentStatus environment = mCurrentEnvironment;
    MINT32 isoIdx1Status  = ISO_UNKNOWN;
    MINT32 isoIdx2Status = ISO_UNKNOWN;
    MUINT32 isoValue = 0;

    IF_NULL_RETURN_VALUE(mpHalISP, hdrState);
    mpHalISP->sendIspCtrl(NS3Av3::EISPCtrl_GetHDRISO, (MINTPTR)&isoValue, 0);

    // Get ISO stable count
    if ((isoValue >= isoIdx1Thres) && (mISOIdx1StableCount < 2 * mIdx1StableCountThres)) {
        mISOIdx1StableCount++;
    } else if (mISOIdx1StableCount > 0) {
        mISOIdx1StableCount--;
    }

    if ((isoValue >= isoIdx2Thres) && (mISOIdx2StableCount < 2 * mIdx2StableCountThres)) {
        mISOIdx2StableCount++;
    } else if (mISOIdx2StableCount > 0) {
        mISOIdx2StableCount--;
    }

    // Identify ISO status
    if (mISOIdx1StableCount >= 2 * mIdx1StableCountThres) {
        // > ISO value 1 and stable
        isoIdx1Status = ISO_LARGER_AND_STABLE;
    } else if (mISOIdx1StableCount <= 0) {
        // < ISO value 1 and stable
        isoIdx1Status = ISO_SMALLER_AND_STABLE;
    } else {
        isoIdx1Status = ISO_UNSTABLE;
    }

    if (mISOIdx2StableCount >= 2 * mIdx2StableCountThres) {
        // > ISO value 2 and stable
        isoIdx2Status = ISO_LARGER_AND_STABLE;
    } else if (mISOIdx2StableCount <= 0) {
        // < ISO value 1 and stable
        isoIdx2Status = ISO_SMALLER_AND_STABLE;
    } else {
        isoIdx2Status = ISO_UNSTABLE;
    }


    if( mCurrentEnvironment == eEnv_HighLight ) {
        // High light (low iso) -> Low light (high iso) w/ stable, strategy = HDR OFF
        if( isoIdx2Status == ISO_LARGER_AND_STABLE ) {
            environment = eEnv_LowLight;
            switch(mHdrRcfgInfo.hdrState)
            {
                case HDR_STATE_1EXP:
                    hdrState = HDR_STATE_1EXP;
                    break;
                case HDR_STATE_2EXP:
                    hdrState = HDR_STATE_1EXP;
                    break;
                case HDR_STATE_3EXP:
                    hdrState = HDR_STATE_2EXP;
                    break;
                default:
                    hdrState = mHdrRcfgInfo.hdrState;
                    break;
            }
        }
    }
    else if( mCurrentEnvironment == eEnv_LowLight ) {
        // Low light (high iso), strategy = HDR OFF
        switch(mHdrRcfgInfo.hdrState)
        {
            case HDR_STATE_1EXP:
                 hdrState = HDR_STATE_1EXP;
                 break;
            case HDR_STATE_2EXP:
                 hdrState = HDR_STATE_1EXP;
                 break;
            case HDR_STATE_3EXP:
                 hdrState = HDR_STATE_2EXP;
                 break;
            default:
                 hdrState = mHdrRcfgInfo.hdrState;
                 break;
        }
        // Low light (high iso) -> High light (low iso) w/ stable, strategy = HDR ON
        if( isoIdx1Status == ISO_SMALLER_AND_STABLE ) {
            environment = eEnv_HighLight;
            switch(mHdrRcfgInfo.hdrState)
            {
                case HDR_STATE_1EXP:
                     hdrState = HDR_STATE_2EXP;
                     break;
                case HDR_STATE_2EXP:
                     hdrState = HDR_STATE_2EXP;
                     break;
                case HDR_STATE_3EXP:
                     hdrState = HDR_STATE_3EXP;
                     break;
                default:
                     hdrState = mHdrRcfgInfo.hdrState;
                     break;
            }
        }
    }

    MY_S_LOGD_IF((mCurrentEnvironment != environment) || (mDebugLevel > 0),
                 "HDRstate[%s] iso:%d Environment(%d=>%d), isoStableCount(%d,%d) isoThres(%d,%d) isoIdx(%d,%d)",
                 printHDRState(hdrState).c_str(), isoValue, mCurrentEnvironment, environment,
                 mISOIdx1StableCount, mISOIdx2StableCount, isoIdx1Thres, isoIdx2Thres,
                 isoIdx1Status, isoIdx2Status);

    mCurrentEnvironment = environment;
    return hdrState;
}

HDRState HDRStateEvaluator::checkHDRRatio()
{
    HDRState hdrState = mHdrRcfgInfo.hdrState;
    MINT32 hdrRatioHighStatus  = DR_UNKNOWN;
    MINT32 hdrRatioLowStatus = DR_UNKNOWN;
    MUINT32 mHDRRatioHighTh = 0;
    MUINT32 mHDRRatioLowTh = 0;
    MUINT32 mHDRRatioStbCntHighTh = 0;
    MUINT32 mHDRRatioStbCntLowTh = 0;

    if(mAeReconfigInfo.u4Hdr_Ratio < 100)
        return hdrState;

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_2EXP:
            mHDRRatioHighTh = mHDRRatioHightTh_HDR_2exp;
            mHDRRatioLowTh = mHDRRatioLowTh_HDR_2exp;
            mHDRRatioStbCntHighTh = mHDRRatioStbCntHighTh_HDR_2exp;
            mHDRRatioStbCntLowTh = mHDRRatioStbCntLowTh_HDR_2exp;
            break;
        case HDR_STATE_3EXP:
            mHDRRatioHighTh = mHDRRatioHighTh_HDR_3exp;
            mHDRRatioLowTh = mHDRRatioLowTh_HDR_3exp;
            mHDRRatioStbCntHighTh = mHDRRatioStbCntHighTh_HDR_3exp;
            mHDRRatioStbCntLowTh = mHDRRatioStbCntLowTh_HDR_3exp;
            break;
        case HDR_STATE_1EXP:
        default:
            mHDRRatioHighTh = 100;
            mHDRRatioLowTh = 100;
            mHDRRatioStbCntHighTh = 10;
            mHDRRatioStbCntLowTh = 10;
            break;
    }

    if((mHDRRatioHighTh <= 0) || (mHDRRatioLowTh <= 0) ||
        (mHDRRatioStbCntHighTh <= 0) || (mHDRRatioStbCntLowTh <= 0))
    {
        MY_S_LOGD_IF((mDebugLevel > 1), "[HDRRatio] untuned para return hdr_ratio:%d, thd:%d/%d, CNTth:%d/%d, HDRState:%d/%d\n",
            mAeReconfigInfo.u4Hdr_Ratio,
            mHDRRatioHighTh, mHDRRatioLowTh,
            mHDRRatioStbCntHighTh, mHDRRatioStbCntLowTh,
            mHdrRcfgInfo.hdrState, hdrState);
        return hdrState;
    }

    if((mAeReconfigInfo.u4Hdr_Ratio >= mHDRRatioHighTh) && (mHDRRatioStbCntHigh <= mHDRRatioStbCntHighTh))
        mHDRRatioStbCntHigh ++;
    else if(mHDRRatioStbCntHigh > 0)
        mHDRRatioStbCntHigh --;

    if((mAeReconfigInfo.u4Hdr_Ratio <= mHDRRatioLowTh) && (mHDRRatioStbCntLow <= mHDRRatioStbCntLowTh))
        mHDRRatioStbCntLow ++;
    else if(mHDRRatioStbCntLow > 0)
        mHDRRatioStbCntLow --;

    hdrRatioHighStatus = (mHDRRatioStbCntHigh >= mHDRRatioStbCntHighTh) ? (DR_HIGH_STABLE):(DR_UNSTABLE);
    hdrRatioLowStatus = (mHDRRatioStbCntLow >= mHDRRatioStbCntLowTh) ? (DR_LOW_STABLE):(DR_UNSTABLE);

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_2EXP:
            hdrState = (hdrRatioHighStatus == DR_HIGH_STABLE) ? (HDR_STATE_3EXP):(HDR_STATE_2EXP);
            break;
        case HDR_STATE_3EXP:
            hdrState = (hdrRatioLowStatus == DR_LOW_STABLE) ? (HDR_STATE_2EXP):(HDR_STATE_3EXP);
            break;
        default:
            hdrState = mHdrRcfgInfo.hdrState;
            break;
    }
    MY_S_LOGD_IF((mDebugLevel > 1), "[HDRRatio] status:%d/%d, cnt:%d/%d, hdr_ratio:%d, thd:%d/%d, CNTth:%d/%d, HDRState:%d/%d\n",
        hdrRatioHighStatus, hdrRatioLowStatus,
        mHDRRatioStbCntHigh, mHDRRatioStbCntLow, mAeReconfigInfo.u4Hdr_Ratio, mHDRRatioHighTh, mHDRRatioLowTh, mHDRRatioStbCntHighTh, mHDRRatioStbCntLowTh, mHdrRcfgInfo.hdrState, hdrState);

    return hdrState;
}

HDRState HDRStateEvaluator::checkDynamicRange()
{
    HDRState hdrState = mHdrRcfgInfo.hdrState;
    MINT32 drHighStatus  = DR_UNKNOWN;
    MINT32 drLowStatus = DR_UNKNOWN;
    MUINT32 mDRHightTh = 0;
    MUINT32 mDRLowTh = 0;
    MUINT32 mDRStbCntHighTh = 0;
    MUINT32 mDRStbCntLowTh = 0;

    if(mAeReconfigInfo.u4Dr_Ratio == -1)
        return hdrState;

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_1EXP:
            mDRHightTh = mDRHightTh_Normal;
            mDRLowTh = mDRLowTh_Normal;
            mDRStbCntHighTh = mDRStbCntHighTh_Normal;
            mDRStbCntLowTh = mDRStbCntLowTh_Normal;
            break;
        case HDR_STATE_2EXP:
            mDRHightTh = mDRHightTh_HDR_2exp;
            mDRLowTh = mDRLowTh_HDR_2exp;
            mDRStbCntHighTh = mDRStbCntHighTh_HDR_2exp;
            mDRStbCntLowTh = mDRStbCntLowTh_HDR_2exp;
            break;
        case HDR_STATE_3EXP:
            mDRHightTh = mDRHightTh_HDR_3exp;
            mDRLowTh = mDRLowTh_HDR_3exp;
            mDRStbCntHighTh = mDRStbCntHighTh_HDR_3exp;
            mDRStbCntLowTh = mDRStbCntLowTh_HDR_3exp;
            break;
        default:
            mDRHightTh = mDRHightTh_Normal;
            mDRLowTh = mDRLowTh_Normal;
            mDRStbCntHighTh = mDRStbCntHighTh_Normal;
            mDRStbCntLowTh = mDRStbCntLowTh_Normal;
            break;
    }

    if((mAeReconfigInfo.u4Dr_Ratio >= mDRHightTh) && (mDRStbCntHigh <= mDRStbCntHighTh))
        mDRStbCntHigh ++;
    else if(mDRStbCntHigh > 0)
        mDRStbCntHigh --;

    if((mAeReconfigInfo.u4Dr_Ratio <= mDRLowTh) && (mDRStbCntLow <= mDRStbCntLowTh))
        mDRStbCntLow ++;
    else if(mDRStbCntLow > 0)
        mDRStbCntLow --;

    drHighStatus = (mDRStbCntHigh >= mDRStbCntHighTh) ? (DR_HIGH_STABLE):(DR_UNSTABLE);
    drLowStatus = (mDRStbCntLow >= mDRStbCntLowTh) ? (DR_LOW_STABLE):(DR_UNSTABLE);

    if(drHighStatus == DR_HIGH_STABLE)
        mIsHDR = true;
    else if (drLowStatus == DR_LOW_STABLE)
        mIsHDR = false;

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_1EXP:
            hdrState = (mIsHDR == true) ? (HDR_STATE_2EXP):(HDR_STATE_1EXP);
            break;
        case HDR_STATE_2EXP:
            hdrState = (mIsHDR == true) ? (HDR_STATE_3EXP):(HDR_STATE_2EXP);
            break;
        case HDR_STATE_3EXP:
            hdrState = (mIsHDR == true) ? (HDR_STATE_3EXP):(HDR_STATE_2EXP);
            break;
        default:
            hdrState = mHdrRcfgInfo.hdrState;
            break;
    }
    MY_S_LOGD_IF((mDebugLevel > 1), "[DR]isHDR:%d, status:%d/%d, cnt:%d/%d, hdr_ratio:%d, DR:%d, DRth:%d/%d, CNTth:%d/%d, HDRState:%d/%d\n",mIsHDR, drHighStatus, drLowStatus,
        mDRStbCntHigh, mDRStbCntLow, mAeReconfigInfo.u4Hdr_Ratio, mAeReconfigInfo.u4Dr_Ratio, mDRHightTh, mDRLowTh, mDRStbCntHighTh, mDRStbCntLowTh, mHdrRcfgInfo.hdrState, hdrState);

    return hdrState;
}

HDRState HDRStateEvaluator::checkShutter()
{
    HDRState hdrState = mHdrRcfgInfo.hdrState;
    MINT32 highStatus  = DR_UNKNOWN;
    MINT32 lowStatus = DR_UNKNOWN;
    MUINT32 shutterHighTh = 0;
    MUINT32 shutterLowTh = 0;
    MUINT32 stbCntHighTh = 0;
    MUINT32 stbCntLowTh = 0;

    if(mAeReconfigInfo.u4LEExpo <= 0)
        return hdrState;

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_2EXP:
            shutterHighTh = mShutterHighTh_2exp;
            shutterLowTh = mShutterLowTh_2exp;
            stbCntHighTh = mShutterStbCntHighTh;
            stbCntLowTh = mShutterStbCntLowTh;
            break;
        case HDR_STATE_3EXP:
            shutterHighTh = mShutterHighTh_3exp;
            shutterLowTh = mShutterLowTh_3exp;
            stbCntHighTh = mShutterStbCntHighTh;
            stbCntLowTh = mShutterStbCntLowTh;
            break;
        case HDR_STATE_1EXP:
        default:
            shutterHighTh = 0;
            shutterLowTh = 0;
            stbCntHighTh = mShutterStbCntHighTh;
            stbCntLowTh = mShutterStbCntLowTh;
            break;
    }

    if((shutterHighTh <= 0) || (shutterLowTh <= 0) || (stbCntHighTh <= 0) || (stbCntLowTh <= 0))
    {
        MY_S_LOGD_IF((mDebugLevel > 1), "[Shutter] untuned para return ne_shutter:%d, thd:%d/%d, CNTth:%d/%d, HDRState:%d/%d\n",
            mAeReconfigInfo.u4LEExpo, shutterHighTh, shutterLowTh, stbCntHighTh, stbCntLowTh, mHdrRcfgInfo.hdrState, hdrState);
        return hdrState;
    }

    if((mAeReconfigInfo.u4LEExpo >= shutterHighTh) && (mShutterStbCntHigh <= stbCntHighTh))
        mShutterStbCntHigh++;
    else if(mShutterStbCntHigh > 0)
        mShutterStbCntHigh--;

    if((mAeReconfigInfo.u4LEExpo <= shutterLowTh) && (mShutterStbCntLow <= stbCntLowTh))
        mShutterStbCntLow++;
    else if(mShutterStbCntLow > 0)
        mShutterStbCntLow--;

    highStatus = (mShutterStbCntHigh >= stbCntHighTh) ? (DR_HIGH_STABLE) : (DR_UNSTABLE);
    lowStatus = (mShutterStbCntLow >= stbCntLowTh) ? (DR_LOW_STABLE) : (DR_UNSTABLE);

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_2EXP:
            if(lowStatus == DR_LOW_STABLE)
            {
                hdrState = HDR_STATE_3EXP;
            }
            else
            {
                hdrState = HDR_STATE_2EXP;
            }
            //hdrState = (is3Exp == true) ? (HDR_STATE_3EXP):(HDR_STATE_2EXP);
            break;
        case HDR_STATE_3EXP:
            if(highStatus == DR_HIGH_STABLE)
            {
                hdrState = HDR_STATE_2EXP;
            }
            else
            {
                hdrState = HDR_STATE_3EXP;
            }
            //hdrState = (is3Exp == true) ? (HDR_STATE_3EXP):(HDR_STATE_2EXP);
            break;
        default:
            hdrState = mHdrRcfgInfo.hdrState;
            break;
    }
    MY_S_LOGD_IF((mDebugLevel > 1), "[Shutter] status:%d/%d, cnt:%d/%d, ne_shutter:%d, thd:%d/%d, CNTth:%d/%d, HDRState:%d/%d\n",
        highStatus, lowStatus,
        mShutterStbCntHigh, mShutterStbCntLow, mAeReconfigInfo.u4LEExpo, shutterHighTh, shutterLowTh, stbCntHighTh, stbCntLowTh, mHdrRcfgInfo.hdrState, hdrState);

    return hdrState;
}

HDRState HDRStateEvaluator::checkPowerThermal()
{
    HDRState hdrState = mHdrRcfgInfo.hdrState;
    bool isPowerThermal = false; /*wait AP implement*/
    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_1EXP:
            hdrState = (isPowerThermal == true) ? (HDR_STATE_1EXP):(HDR_STATE_2EXP);
            break;
        case HDR_STATE_2EXP:
            hdrState = (isPowerThermal == true) ? (HDR_STATE_1EXP):(HDR_STATE_2EXP);
             break;
        case HDR_STATE_3EXP:
            hdrState = (isPowerThermal == true) ? (HDR_STATE_2EXP):(HDR_STATE_3EXP);
            break;
        default:
            hdrState = mHdrRcfgInfo.hdrState;
            break;
    }
    MY_S_LOGD_IF((mDebugLevel > 1), "[checkPowerThermal]power:%d, cur_stat:%d, req_stat:%d\n", isPowerThermal, mHdrRcfgInfo.hdrState, hdrState);

    return hdrState;
}
bool HDRStateEvaluator::switchHandle_NormalToHDR(MUINT32 shutter_result)
{
    bool normal_to_HDR = false;

    /*current mode:normal, target mode:HDR*/
    bool isnor_NE_flick = (mAeReconfigInfo.u4LEExpo < shutter_result) ? (true) : (false);
    bool isHDR_NE_flick = (mAeReconfigInfo.u4NextLEExpo < shutter_result) ? (true) : (false);
    bool isHDR_SE_flick = (mAeReconfigInfo.u4NextSEExpo < shutter_result) ? (true) : (false);

    MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]shutter_result:%d, flick:%d/%d/%d \n",shutter_result, isnor_NE_flick, isHDR_NE_flick,isHDR_SE_flick);

    if(isHDR_NE_flick){
        if(isnor_NE_flick)
            normal_to_HDR = true;
        else
            normal_to_HDR = false;
    }
    else{
        normal_to_HDR = true;
    }

    return normal_to_HDR;
}
bool HDRStateEvaluator::checkFlicker_NormalToHDR()
{
    bool isflicker = false;
    bool flick_50hz = false;
    bool flick_60hz = false;
    MINT32 flick_50hz_conf_th = strNormalToHDR.flick_50hz_conf_th;
    MINT32 flick_60hz_conf_th = strNormalToHDR.flick_60hz_conf_th;
    MUINT32 flick_50hz_framecnt_th = strNormalToHDR.flick_50hz_framecnt_th;
    MUINT32 flick_60hz_framecnt_th = strNormalToHDR.flick_60hz_framecnt_th;

    MUINT32 shutter_base = SHUTER_BASE_60;

    if(mAeReconfigInfo.bIsFlickerActive)
    {
        if(mAeReconfigInfo.i4Flicker50HzScore > flick_50hz_conf_th)
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF] = FLICKER_50HZ;
        else if(mAeReconfigInfo.i4Flicker60HzScore > flick_60hz_conf_th)
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF] = FLICKER_60HZ;
        else
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF] = FLICKER_UNKNOWN;

        MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]idx:%d, flcik_result:%d, 50hz:%d/%d, 60hz:%d/%d\n", flicker_frame_cnt,
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF],
            mAeReconfigInfo.i4Flicker50HzScore, flick_50hz_conf_th,
            mAeReconfigInfo.i4Flicker60HzScore, flick_60hz_conf_th);
        flicker_frame_cnt ++;
        flicker_frame_cnt = (flicker_frame_cnt >= MAX_FLICKER_BUFF) ? (0):(flicker_frame_cnt);
    }
    else
    {
        memset(flicker_buff, 0, MAX_FLICKER_BUFF * sizeof(MINT32));
    }

    flicker_50hz_cnt = 0;
    flicker_60hz_cnt = 0;

    for(int i = 0; i < MAX_FLICKER_BUFF; i++)
    {
        switch(flicker_buff[i])
        {
            case FLICKER_50HZ:
                flicker_50hz_cnt ++;
                break;
            case FLICKER_60HZ:
                flicker_60hz_cnt ++;
                break;
            case FLICKER_UNKNOWN:
                break;
            default:
                break;
        }
        MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]flcik_buf[%d]:%d 50/60:%d/%d\n",i, flicker_buff[i],flicker_50hz_cnt,flicker_60hz_cnt);
    }

    if(flicker_50hz_cnt > flicker_60hz_cnt)
        flick_50hz = (flicker_50hz_cnt > flick_50hz_framecnt_th) ? (true) : (false);
    else if(flicker_60hz_cnt > flicker_50hz_cnt)
        flick_60hz = (flicker_60hz_cnt > flick_60hz_framecnt_th) ? (true) : (false);
    else if((flicker_60hz_cnt == flicker_50hz_cnt) && (flicker_50hz_cnt > 0)){
        flick_50hz = true; flick_60hz = false; /*flicker pority depend on country*/
    }
    else{
        flick_50hz = false; flick_60hz = false;
    }

    MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]flcik_cnt:%d/%d frm:%d/%d\n",flicker_50hz_cnt,flicker_60hz_cnt,flick_50hz_framecnt_th,flick_60hz_framecnt_th);

    if(flick_50hz){
        /*if next shutter time < 10000 then flicker */
        shutter_base = SHUTER_BASE_50;
    }
    else if(flick_60hz){
        /*if next shutter time < 8333 then flicker */
        shutter_base = SHUTER_BASE_60;
    }

    isflicker = (switchHandle_NormalToHDR(shutter_base) & (flick_50hz|flick_60hz));
    MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]isflicker:%d,switch:%d, flick 50/60:%d/%d\n",isflicker,switchHandle_NormalToHDR(shutter_base),flick_50hz,flick_60hz);

    return isflicker;
}
bool HDRStateEvaluator::switchHandle_HDRToNormal(MUINT32 shutter_result)
{
    bool HDR_to_normal = false;
    /*current mode:HDR, target mode:normal*/
    bool isHDR_NE_flick = (mAeReconfigInfo.u4LEExpo < shutter_result) ? (true) : (false);
    bool isHDR_SE_flick = (mAeReconfigInfo.u4SEExpo < shutter_result) ? (true) : (false);
    bool isnor_NE_flick = (mAeReconfigInfo.u4NextLEExpo < shutter_result) ? (true) : (false);

    MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]shutter_result:%d, flick:%d/%d/%d \n",shutter_result, isHDR_NE_flick, isHDR_SE_flick,isnor_NE_flick);

    if(isnor_NE_flick){
        HDR_to_normal = false;
    }
    else{
        if(isHDR_NE_flick)
            HDR_to_normal = true;
        else
            HDR_to_normal = false;
    }

    return HDR_to_normal;
}
bool HDRStateEvaluator::checkFlicker_HDRToNormal()
{
    bool isflicker = false;
    bool flick_50hz = false;
    bool flick_60hz = false;
    MINT32 flick_50hz_conf_th = strHdrToNormal.flick_50hz_conf_th;
    MINT32 flick_60hz_conf_th = strHdrToNormal.flick_60hz_conf_th;
    MUINT32 flick_50hz_framecnt_th = strHdrToNormal.flick_50hz_framecnt_th;
    MUINT32 flick_60hz_framecnt_th = strHdrToNormal.flick_60hz_framecnt_th;
    MUINT32 shutter_base = SHUTER_BASE_60;

    if(mAeReconfigInfo.bIsFlickerActive)
    {
        if(mAeReconfigInfo.i4Flicker50HzScore > flick_50hz_conf_th)
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF] = FLICKER_50HZ;
        else if(mAeReconfigInfo.i4Flicker60HzScore > flick_60hz_conf_th)
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF] = FLICKER_60HZ;
        else
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF] = FLICKER_UNKNOWN;

        MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]idx:%d, flcik_result:%d, 50hz:%d/%d, 60hz:%d/%d\n",flicker_frame_cnt,
            flicker_buff[flicker_frame_cnt % MAX_FLICKER_BUFF],
            mAeReconfigInfo.i4Flicker50HzScore, flick_50hz_conf_th,
            mAeReconfigInfo.i4Flicker60HzScore, flick_60hz_conf_th);

        flicker_frame_cnt ++;
        flicker_frame_cnt = (flicker_frame_cnt >= MAX_FLICKER_BUFF) ? (0):(flicker_frame_cnt);
    }
    else
    {
        memset(flicker_buff, 0, MAX_FLICKER_BUFF * sizeof(MINT32));
    }

    flicker_50hz_cnt = 0;
    flicker_60hz_cnt = 0;

    for(int i = 0; i < MAX_FLICKER_BUFF; i++)
    {
        switch(flicker_buff[i])
        {
            case FLICKER_50HZ:
                flicker_50hz_cnt ++;
                break;
            case FLICKER_60HZ:
                flicker_60hz_cnt ++;
                break;
            case FLICKER_UNKNOWN:
                break;
            default:
                break;
        }

        MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]flcik_buf[%d]:%d 50/60:%d/%d\n",i,flicker_buff[i],flicker_50hz_cnt,flicker_60hz_cnt);
    }

    if(flicker_50hz_cnt > flicker_60hz_cnt)
        flick_50hz = (flicker_50hz_cnt > flick_50hz_framecnt_th) ? (true) : (false);
    else if(flicker_60hz_cnt > flicker_50hz_cnt)
        flick_60hz = (flicker_60hz_cnt > flick_60hz_framecnt_th) ? (true) : (false);
    else if((flicker_60hz_cnt == flicker_50hz_cnt) && (flicker_50hz_cnt > 0)){
        flick_50hz = true; flick_60hz = false; /*flicker pority depend on country*/
    }
    else{
        flick_50hz = false; flick_60hz = false;
    }
    MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]flcik_cnt:%d/%d frm:%d/%d\n",flicker_50hz_cnt,flicker_60hz_cnt,flick_50hz_framecnt_th,flick_60hz_framecnt_th);

    if(flick_50hz){
        /*if next shutter time < 10000 then flicker */
        shutter_base = SHUTER_BASE_50;
    }
    else if(flick_60hz){
        /*if next shutter time < 8333 then flicker */
        shutter_base = SHUTER_BASE_60;
    }

    isflicker = (switchHandle_HDRToNormal(shutter_base) & (flick_50hz|flick_60hz));
    MY_S_LOGD_IF((mDebugLevel > 1), "[Detect]isflicker:%d,switch:%d, flick 50/60:%d/%d\n",isflicker,switchHandle_HDRToNormal(shutter_base),flick_50hz,flick_60hz);

    return isflicker;
}
HDRState HDRStateEvaluator::checkFlicker()
{
    HDRState hdrState = mHdrRcfgInfo.hdrState;
    bool isSwitch = false;

    switch(mHdrRcfgInfo.hdrState)
    {
        case HDR_STATE_1EXP:
            isSwitch = checkFlicker_NormalToHDR();
            hdrState = (isSwitch == true) ? (HDR_STATE_2EXP):(HDR_STATE_1EXP);
            break;
        case HDR_STATE_2EXP:
            isSwitch = checkFlicker_HDRToNormal();
            hdrState = (isSwitch == true) ? (HDR_STATE_1EXP):(HDR_STATE_2EXP);
            break;
        case HDR_STATE_3EXP:
            isSwitch = checkFlicker_HDRToNormal();
            hdrState = (isSwitch == true) ? (HDR_STATE_2EXP):(HDR_STATE_3EXP);
            break;
        default:
            break;
    }
    MY_S_LOGD_IF((mDebugLevel > 1), "[checkFlicker]isSwitch:%d, cur_stat:%d, req_stat:%d\n",
                                                                  isSwitch,
                                                                  mHdrRcfgInfo.hdrState,
                                                                  hdrState);
    return hdrState;
}

bool HDRStateEvaluator::resetISOThresStatus(const MUINT32 isoIdx1Thres, const MUINT32 isoIdx2Thres)
{
    MUINT32 isoValue = 0;
    IF_NULL_RETURN_VALUE(mpHalISP, false);

    mpHalISP->sendIspCtrl(NS3Av3::EISPCtrl_GetHDRISO, (MINTPTR)&isoValue, 0);

    if(isoValue >= isoIdx1Thres) {
       mISOIdx1StableCount = 2 * mIdx1StableCountThres;
    }
    else {
       mISOIdx1StableCount = 0;
    }

    if(isoValue >= isoIdx2Thres) {
       mCfgHdrOff = true;
       mISOIdx2StableCount = 2 * mIdx2StableCountThres;
    }
    else {
       mISOIdx2StableCount = 0;
    }

    MY_LOGD("ISO=%d isoThres(%d,%d), reset mISOIdx1StableCount/mISOIdx2StableCount to %d/%d CfgHdrOff[%d]",
        isoValue, isoIdx1Thres, isoIdx2Thres, mISOIdx1StableCount, mISOIdx2StableCount, mCfgHdrOff);
    return true;
}
string HDRStateEvaluator::printHDRState(const HDRState state) {
    switch (state) {
    case HDR_STATE_NONE:
        return string("HDR_STATE_NONE");
        break;
    case HDR_STATE_1EXP:
        return string("HDR_STATE_1EXP");
        break;
    case HDR_STATE_2EXP:
        return string("HDR_STATE_2EXP");
        break;
    case HDR_STATE_3EXP:
        return string("HDR_STATE_3EXP");
        break;
    default:
        return string("Invalid HDRState");
    }
}
string HDRStateEvaluator::printHDRProcState(const HDRProcState procState) {
    switch (procState) {
    case STATE_PROCESSING:
        return string("STATE_PROCESSING");
        break;
    case STATE_DONE:
        return string("STATE_DONE");
        break;
    case STATE_IDLE:
        return string("STATE_IDLE");
        break;
    default:
        return string("Invalid HDRProcState");
    }
}
string HDRStateEvaluator::printSeamlessResult(const int32_t seamless_smooth_result) {
    switch (seamless_smooth_result) {
    case STATE_PROCESSING:
        return string("STATE_PROCESSING");
        break;
    case STATE_DONE:
        return string("STATE_DONE");
        break;
    case STATE_IDLE:
        return string("STATE_IDLE");
        break;
    default:
        return string("Invalid seamless_smooth_result");
    }
}
}
}
}
}
}

