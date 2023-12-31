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

#define LOG_TAG "MtkCam/P1NodeConnectLMV"
//
#include "P1ConnectLMV.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
namespace NSCam {
namespace v3 {
namespace NSP1Node {

//
#define SUPPORT_LMV             (1)
#define FORCE_EIS_ON            (SUPPORT_LMV && (0))
#define FORCE_3DNR_ON           (SUPPORT_LMV && (0))

#if 1
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  P1ConnectLMV Implementation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::support(void)
{
    #if SUPPORT_LMV
    return MTRUE;
    #else
    return MFALSE;
    #endif
};

/******************************************************************************
 *
 ******************************************************************************/
MINT32
P1ConnectLMV::getOpenId(void)
{
    return mOpenId;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::init(
    sp<IImageBuffer> & rEISOBuf,
    MUINT32 eisMode,
    const MUINT32 eisFactor
)
{
    uninit();
    //
    Mutex::Autolock _l(mLock);
    mIsConfig3A = MFALSE;
    mEisMode = eisMode;
    mIsCalibration = EIS_MODE_IS_CALIBRATION_ENABLED(mEisMode) ? MTRUE : MFALSE;
    MY_LOGD2("mEisMode=0x%x, mIsCalibration=%d", mEisMode, mIsCalibration);
    P1_TRACE_S_BEGIN(SLG_S, "P1Connect:LMV-init");
    mpLMV = LMVHal::CreateInstance(LOG_TAG, mOpenId);
    if (mpLMV == NULL) {
        MY_LOGE("LMVHal::CreateInstance fail");
        return MFALSE;
    }
    mpLMV->Init(eisFactor);
    IHalSensorList* sensorList = MAKE_HalSensorList();
    if (sensorList == NULL) {
        MY_LOGE("Get-SensorList fail");
        return MFALSE;
    }
    mConfigData.sensorType = sensorList->queryType(mOpenId);

    #if SUPPORT_EIS
    if( mIsCalibration ) {
        mpGISCalibration =
            GisCalibration::CreateInstance(LOG_TAG, mOpenId, eisFactor);
        if (mpGISCalibration == NULL) {
            MY_LOGE("GisCalibration::CreateInstance fail");
            return MFALSE;
        }
        mpGISCalibration->Init();
    }
    #endif
    P1_TRACE_S_END(); // "P1Connect:LMV-init"

    // [TODO] get pEISOBuf from EIS
    mpLMV->GetBufLMV(rEISOBuf);
    if (rEISOBuf == NULL) {
        MY_LOGE("LMVHal::GetBufLMV fail");
        return MFALSE;
    }
    return MTRUE;
};

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::uninit(void)
{
    Mutex::Autolock _l(mLock);
    P1_TRACE_S_BEGIN(SLG_S, "P1Connect:LMV-uninit");
    if(mpLMV) {
        mpLMV->Uninit();
        mpLMV->DestroyInstance(LOG_TAG);
        mpLMV = NULL;
    }
    //
    #if SUPPORT_EIS
    if(mpGISCalibration) {
        mpGISCalibration->Uninit();
        mpGISCalibration->DestroyInstance(LOG_TAG);
        mpGISCalibration = NULL;
    }
    #endif
    mIsConfig3A = MFALSE;
    P1_TRACE_S_END(); // "P1Connect:LMV-uninit"
    return MFALSE;
};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::config(void)
{
    Mutex::Autolock _l(mLock);
    if ((mpLMV != NULL) && (mpLMV->GetLMVSupportInfo(mOpenId))) {
        P1_TRACE_S_BEGIN(SLG_S, "P1Connect:LMV-ConfigLMV");
        mpLMV->ConfigLMV(mConfigData);
        P1_TRACE_S_END(); // "P1Connect:LMV-ConfigLMV"
    }
};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::enableSensor(void)
{
    Mutex::Autolock _l(mLock);
    if ((mpLMV != NULL) && (mpLMV->GetLMVSupportInfo(mOpenId))) {
        P1_TRACE_S_BEGIN(SLG_S, "P1Connect:LMV-EnableGyroSensor");
        mpLMV->EnableGyroSensor();
        P1_TRACE_S_END(); // "P1Connect:LMV-EnableGyroSensor"
    }
};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::enableOIS(IHal3A_T * p3A)
{
    Mutex::Autolock _l(mLock);
    if (EIS_MODE_IS_CALIBRATION_ENABLED(mEisMode) && mpLMV && p3A) {
        //Enable OIS
        MY_LOGD2("[LMVHal] mEisMode:%d => Enable OIS \n", mEisMode);
        P1_TRACE_S_BEGIN(SLG_R, "P1Connect:LMV-SetEnableOIS");
        p3A ->send3ACtrl(E3ACtrl_SetEnableOIS, 1, 0);
        P1_TRACE_S_END(); // "P1Connect:LMV-SetEnableOIS"
        mEisMode = EIS_MODE_OFF;
    }

};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::getBuf(
    sp<IImageBuffer> & rEISOBuf
)
{
    Mutex::Autolock _l(mLock);
    P1_TRACE_S_BEGIN(SLG_I, "P1Connect:LMV-GetBufLMV");
    mpLMV->GetBufLMV(rEISOBuf);
    P1_TRACE_S_END(); // "P1Connect:LMV-GetBufLMV"
    if (rEISOBuf == NULL) {
        MY_LOGE("LMVHal::GetBufLMV fail");
    }
};

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::isEISOn(
    IMetadata* const inApp
)
{
    if (inApp == NULL) {
        return MFALSE;
    }
    MUINT8 eisMode = MTK_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    MINT32 adveisMode = MTK_EIS_FEATURE_EIS_MODE_OFF;
    if(!tryGetMetadata<MUINT8>(inApp,
        MTK_CONTROL_VIDEO_STABILIZATION_MODE, eisMode)) {
        MY_LOGW_IF(0, "no MTK_CONTROL_VIDEO_STABILIZATION_MODE");
    }
    if( !tryGetMetadata<MINT32>(inApp, MTK_EIS_FEATURE_EIS_MODE, adveisMode) ) {
        MY_LOGD2("no MTK_EIS_FEATURE_EIS_MODE");
    }

#if FORCE_EIS_ON
    eisMode = MTK_CONTROL_VIDEO_STABILIZATION_MODE_ON;
#endif
    return ( eisMode == MTK_CONTROL_VIDEO_STABILIZATION_MODE_ON ||
             adveisMode == MTK_EIS_FEATURE_EIS_MODE_ON );
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::isLMVOn(
    IMetadata* const inHal
)
{
    if (inHal == NULL) {
        return MTRUE;
    }
    const MINT32 PIPELINE_LMV_FORCE_OFF = 0;
    MINT32 lmvValid = -1;
    if( !tryGetMetadata<MINT32>(inHal, MTK_LMV_VALIDITY, lmvValid) ) {
        MY_LOGD3("no MTK_LMV_SW_VALIDITY");
    }

    return ( lmvValid != PIPELINE_LMV_FORCE_OFF ) ? MTRUE : MFALSE;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
P1ConnectLMV::isGMVOn(
    IMetadata* const inHal
)
{
    if (inHal == NULL) {
        return MFALSE;
    }
    MBOOL gmvMode = MFALSE;
    if( !tryGetMetadata<MBOOL>(inHal, MTK_P1NODE_ISNEED_GMV, gmvMode) ) {
        MY_LOGD2("no MTK_P1NODE_ISNEED_GMV");
    }

    return gmvMode;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::is3DNROn(
    IMetadata* const inApp,
    IMetadata* const inHal
)
{
    if (inApp == NULL) {
        return MFALSE;
    }
    MINT32 e3DnrMode = MTK_NR_FEATURE_3DNR_MODE_OFF;
    if(!tryGetMetadata<MINT32>(inApp,
        MTK_NR_FEATURE_3DNR_MODE, e3DnrMode)) {
        MY_LOGW_IF(0, "no MTK_NR_FEATURE_3DNR_MODE");
    }

    MINT32 eHal3DnrMode = MTK_NR_FEATURE_3DNR_MODE_ON;
    if(!tryGetMetadata<MINT32>(inHal,
        MTK_DUALZOOM_3DNR_MODE, eHal3DnrMode))
    {
        // Only valid for dual cam. On single cam, we don't care HAL meta,
        // and can assume HAL is "ON" on single cam.
        eHal3DnrMode = MTK_NR_FEATURE_3DNR_MODE_ON;
        MY_LOGD_IF(0, "no MTK_NR_FEATURE_3DNR_MODE in HAL");
    }

#if FORCE_3DNR_ON
    e3DnrMode = MTK_NR_FEATURE_3DNR_MODE_ON;
    eHal3DnrMode = MTK_NR_FEATURE_3DNR_MODE_ON;
#endif
    return (e3DnrMode == MTK_NR_FEATURE_3DNR_MODE_ON && eHal3DnrMode == MTK_NR_FEATURE_3DNR_MODE_ON);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
P1ConnectLMV::checkSwitchOut(
    IMetadata* const inHAL
)
{
    if (inHAL == NULL) {
        return MFALSE;
    }
    MBOOL result = MFALSE;
    //
    #if 1 // #if 0 /* for forced disable UNI_SWITCH_OUT */
    MINT32 needSwitchOut = 0;
    if (tryGetMetadata<MINT32>(
        inHAL, MTK_LMV_SEND_SWITCH_OUT, needSwitchOut)) {
        if (needSwitchOut == 1) {
            result = MTRUE;
        }
    }
    #endif
    //
    return result;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::
prepareData(const LMV_HAL_ENQUEUE& rData)
{
#if SUPPORT_LMV
    if(mpLMV == NULL) {
        MY_LOGW("LMV not ready (%d)", rData.magicNum);
        return;
    }
    MY_LOGI3("LMV PrepareData(%d)", rData.magicNum);
    mpLMV->PrepareLMV(rData);
#endif
};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::processDequeFrame(
    NSCam::NSIoPipe::NSCamIOPipe::QBufInfo&  pBufInfo
)
{
    Mutex::Autolock _l(mLock);
    // call LMV notify function
    if (mpLMV) {
        mpLMV->NotifyLMV(pBufInfo);
    }
};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::processDropFrame(
    android::sp<NSCam::IImageBuffer>& spBuf
)
{
    Mutex::Autolock _l(mLock);
    if (spBuf != NULL && mpLMV) {
        mpLMV->NotifyLMV(spBuf);
    }
};

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::
processResult(
    MBOOL isBinEn,
    MBOOL isConfigEis,
    MBOOL isConfigRrz,
    IMetadata* pInAPP,
    IMetadata* pInHAL,
    MetaSet_T & result3A,
    IHal3A_T* p3A,
    MINT32 const currMagicNum,
    MUINT32 const currSofIdx,
    MUINT32 const lastSofIdx,
    MUINT32 const uniSwitchState,
    QBufInfo const &deqBuf,
    MUINT32 const bufIdxEis,
    MUINT32 const bufIdxRrz,
    IMetadata & rOutputLMV
)
{
    MINT64 exposureTime = 0;
    MINT64 enableMFNR = 0;

#if 1 // for EISO related processing
    if( ( isConfigEis && ( bufIdxEis < deqBuf.mvOut.size() ) ) &&
         pInAPP != NULL )
    {

#ifdef SUPPORT_MFNR
         enableMFNR=1;
#else
         enableMFNR=0;
#endif

        if( isLMVOn(pInHAL) && ( isEISOn(pInAPP) || isGMVOn(pInHAL) || is3DNROn(pInAPP, pInHAL) || mIsCalibration || enableMFNR ) )
        {
            MUINT8 cap_mode = 0;

            if (!tryGetMetadata<MUINT8>
                (pInAPP, MTK_CONTROL_CAPTURE_INTENT, cap_mode)) {
                MY_LOGW("no MTK_CONTROL_CAPTURE_INTENT");
            }

            if (!tryGetMetadata<MINT64>
                (&result3A.appMeta, MTK_SENSOR_EXPOSURE_TIME, exposureTime)) {
                MY_LOGW("no MTK_SENSOR_EXPOSURE_TIME");
            }

            processLMV(isBinEn, p3A,currMagicNum, currSofIdx, lastSofIdx,
                deqBuf, bufIdxEis, bufIdxRrz, cap_mode, exposureTime, rOutputLMV);
        }
    }
    else if( ( isConfigRrz && ( bufIdxRrz < deqBuf.mvOut.size() ) ) &&
             pInAPP != NULL )
    {
        if( (isEISOn(pInAPP) || isGMVOn(pInHAL) ) &&
            ( EIS_MODE_IS_EIS_30_ENABLED(mEisMode) ||
              EIS_MODE_IS_EIS_25_ENABLED(mEisMode) ||
              EIS_MODE_IS_EIS_22_ENABLED(mEisMode) ) )
        {

            MINT32 iExpTime,ihwTS,ilwTS;
            MUINT32 k;
            const MINT64 aTimestamp =
                deqBuf.mvOut[bufIdxRrz].mMetaData.mTimeStamp;

            if (!tryGetMetadata<MINT64>(&result3A.appMeta,
                MTK_SENSOR_EXPOSURE_TIME, exposureTime)) {
                MY_LOGW("no MTK_SENSOR_EXPOSURE_TIME");
            }
            iExpTime = exposureTime/((MINT64)1000);//(ns to us) << frame duration
            ihwTS = (aTimestamp >> 32)&0xFFFFFFFF; //High word
            ilwTS = (aTimestamp & 0xFFFFFFFF);     //Low word
            IMetadata::IEntry entry(MTK_EIS_REGION);
            for (k=0; k<LMV_REGION_INDEX_EXPTIME; k++)
            {
                entry.push_back(0, Type2Type< MINT32 >());
            }
            /* Store required data for Advanced EIS */
            entry.push_back(iExpTime, Type2Type< MINT32 >());
            entry.push_back(ihwTS, Type2Type< MINT32 >());
            entry.push_back(ilwTS, Type2Type< MINT32 >());
            entry.push_back(0, Type2Type< MINT32 >()); // MAX_GMV
            entry.push_back(isBinEn, Type2Type< MBOOL >());
            rOutputLMV.update(MTK_EIS_REGION, entry);
            MY_LOGD1("[LMVHal] eisMode: %d, iExpTime: %d, BinEn: %d\n",
                mEisMode, iExpTime, isBinEn);
        }
    }
#endif

    //
    if (uniSwitchState != UNI_SWITCH_STATE_NONE) {
        IMetadata::IEntry entry(MTK_LMV_SWITCH_OUT_RESULT);
        MINT32 lmv_result = P1NODE_METADATA_INVALID_VALUE;
        switch (uniSwitchState) {
            case UNI_SWITCH_STATE_ACT_ACCEPT:
                lmv_result = MTK_LMV_RESULT_OK;
                break;
            case UNI_SWITCH_STATE_ACT_IGNORE:
                lmv_result = MTK_LMV_RESULT_FAILED;
                break;
            case UNI_SWITCH_STATE_ACT_REJECT:
                lmv_result = MTK_LMV_RESULT_SWITCHING;
                break;
            default:
                MY_LOGW("UNI SwitchOut REQ not act:%d at (%d)",
                    currMagicNum, uniSwitchState);
                break;
        };
        if (lmv_result >= MTK_LMV_RESULT_OK) {
            entry.push_back(lmv_result, Type2Type< MINT32 >());
            rOutputLMV.update(MTK_LMV_SWITCH_OUT_RESULT, entry);
        }
        MY_LOGD0("UNI SwitchOut END (%d) state:%d lmv_result=(%d)",
            currMagicNum, uniSwitchState, lmv_result);
    }

    MY_LOGD3("LMV (bin%d eis%d rrz%d) node(%d) sof(%d/%d) uni(%d) ",
        isBinEn, isConfigEis, isConfigRrz,
        currMagicNum, currSofIdx, lastSofIdx, uniSwitchState);
    if (LOGD_LV3) {
        if ((isConfigEis) && (bufIdxEis != P1_PORT_BUF_IDX_NONE) &&
            (bufIdxEis < deqBuf.mvOut.size())) {
            MY_LOGD3("DEQeis[%d]=(%d)(%dx%d)",
                bufIdxEis, deqBuf.mvOut[bufIdxEis].mSize,
                deqBuf.mvOut[bufIdxEis].mMetaData.mDstSize.w,
                deqBuf.mvOut[bufIdxEis].mMetaData.mDstSize.h);
        }
        if ((isConfigEis) && (bufIdxRrz != P1_PORT_BUF_IDX_NONE) &&
            (bufIdxRrz < deqBuf.mvOut.size())) {
            MY_LOGD3("DEQrrz[%d]=(%d)(%dx%d)",
                bufIdxRrz, deqBuf.mvOut[bufIdxRrz].mSize,
                deqBuf.mvOut[bufIdxRrz].mMetaData.mDstSize.w,
                deqBuf.mvOut[bufIdxRrz].mMetaData.mDstSize.h);
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
P1ConnectLMV::
processLMV(
    MBOOL isBinEn,
    IHal3A_T* p3A,
    MINT32 const currMagicNum,
    MUINT32 const currSofIdx,
    MUINT32 const lastSofIdx,
    QBufInfo const &deqBuf,
    MUINT32 const bufIdxEis,
    MUINT32 const bufIdxRrz,
    MUINT8 captureIntent,
    MINT64 exposureTime,
    IMetadata & rOutputLMV
)
{
    Mutex::Autolock _l(mLock);
    if (deqBuf.mvOut.size() == 0 || deqBuf.mvOut.size() < bufIdxEis) {
        MY_LOGW("DeQ Buf is invalid (%d<%zu), result count (%d)",
            rOutputLMV.count(), deqBuf.mvOut.size(), bufIdxEis);
        return;
    }
    #if SUPPORT_LMV
    if(mpLMV == NULL) {
        MY_LOGW("LMV not ready (%d)", currMagicNum);
        return;
    }

    //One-shot only for a session
    #if SUPPORT_EIS
    if( mIsCalibration && mpGISCalibration != NULL )
    {
        LMV_HAL_CONFIG_DATA config;
        mpGISCalibration->ConfigCalibration(config);
    }
    #endif

    MBOOL isLastSkipped = CHECK_LAST_FRAME_SKIPPED(lastSofIdx, currSofIdx);
    MUINT32 X_INT, Y_INT, X_FLOAT, Y_FLOAT, WIDTH, HEIGHT, ISFROMRRZ;
    MINT32 GMV_X, GMV_Y, MVtoCenterX, MVtoCenterY,iExpTime,ihwTS,ilwTS;
    MUINT32 ConfX,ConfY;
    MUINT32 MAX_GMV;
    EIS_STATISTIC_STRUCT  lmvData;
    memset(&lmvData, 0, sizeof(lmvData));
    const MINT64 aTimestamp = deqBuf.mvOut[bufIdxEis].mMetaData.mTimeStamp;

    MBOOL isLmvValid = MFALSE;
    if (deqBuf.mvOut[bufIdxEis].mSize > 0
        /*
        && deqBuf.mvOut[deqBufIdx].mMetaData.mDstSize.w > 0
        && deqBuf.mvOut[deqBufIdx].mMetaData.mDstSize.h > 0
        */
        ) {
        isLmvValid = MTRUE;
    } else {
        isLmvValid = MFALSE;
    }
    IMetadata::IEntry validityEntry(MTK_LMV_VALIDITY);
    validityEntry.push_back((isLmvValid ? 1 : 0), Type2Type< MINT32 >());
    rOutputLMV.update(MTK_LMV_VALIDITY, validityEntry);

    if (isLmvValid)
    {
        mpLMV->DoLMVCalc(deqBuf, bufIdxRrz, bufIdxEis);
        {
            P1_TRACE_S_BEGIN(SLG_I, "P1Connect:LMV-Result");
            mpLMV->GetLMVResult(X_INT, X_FLOAT, Y_INT, Y_FLOAT, WIDTH, HEIGHT,
                MVtoCenterX, MVtoCenterY, ISFROMRRZ);
            mpLMV->GetGmv(GMV_X, GMV_Y, &ConfX, &ConfY, &MAX_GMV);
            mpLMV->GetLMVStatistic(&lmvData);
            P1_TRACE_S_END(); // "P1Connect:LMV-Result"
        }
    }

    if (EISCustom::isEnabledLMVData())
    {
        IMetadata::Memory eisStatistic;
        eisStatistic.resize(sizeof(EIS_STATISTIC_STRUCT));
        memcpy(eisStatistic.editArray(), &lmvData, sizeof(EIS_STATISTIC_STRUCT));

        IMetadata::IEntry lmvDataEntry(MTK_EIS_LMV_DATA);
        lmvDataEntry.push_back(eisStatistic, Type2Type< IMetadata::Memory >());
        rOutputLMV.update(lmvDataEntry.tag(), lmvDataEntry);
    }

    iExpTime = exposureTime/((MINT64)1000);//(ns to us) << frame duration
    ihwTS = (aTimestamp >> 32)&0xFFFFFFFF; //High word
    ilwTS = (aTimestamp & 0xFFFFFFFF);     //Low word
    /* Store required data for EIS 1.2 */
    IMetadata::IEntry entry(MTK_EIS_REGION);
    if (isLmvValid)
    {
        entry.push_back(X_INT, Type2Type< MINT32 >());
        entry.push_back(X_FLOAT, Type2Type< MINT32 >());
        entry.push_back(Y_INT, Type2Type< MINT32 >());
        entry.push_back(Y_FLOAT, Type2Type< MINT32 >());
        entry.push_back(WIDTH, Type2Type< MINT32 >());
        entry.push_back(HEIGHT, Type2Type< MINT32 >());
        entry.push_back(MVtoCenterX, Type2Type< MINT32 >());
        entry.push_back(MVtoCenterY, Type2Type< MINT32 >());
        entry.push_back(ISFROMRRZ, Type2Type< MINT32 >());
        entry.push_back(GMV_X, Type2Type< MINT32 >());
        entry.push_back(GMV_Y, Type2Type< MINT32 >());
        entry.push_back(ConfX, Type2Type< MINT32 >());
        entry.push_back(ConfY, Type2Type< MINT32 >());
        entry.push_back(iExpTime, Type2Type< MINT32 >());
        entry.push_back(ihwTS, Type2Type< MINT32 >());
        entry.push_back(ilwTS, Type2Type< MINT32 >());
        entry.push_back(MAX_GMV, Type2Type< MINT32 >());
        entry.push_back(isBinEn, Type2Type< MBOOL >());
        mLMVLastData = LMVData(X_INT, X_FLOAT, Y_INT, Y_FLOAT, WIDTH, HEIGHT, MVtoCenterX, MVtoCenterY,
                               ISFROMRRZ, GMV_X, GMV_Y, ConfX, ConfY, iExpTime, ihwTS, ilwTS, MAX_GMV, isBinEn);
        mIsInitResult = MTRUE;
    }
    else
    {
        if( !mIsInitResult )
        {
            mLMVLastData = mpLMV->getDefaultData(deqBuf, bufIdxRrz);
            mIsInitResult = MTRUE;
        }
        entry.push_back(mLMVLastData.cmv_x_int, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.cmv_x_float, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.cmv_y_int, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.cmv_y_float, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.width, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.height, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.cmv_x_center, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.cmv_y_center, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.isFromRRZ, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.gmv_x, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.gmv_y, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.conf_x, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.conf_y, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.expTime, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.hwTs, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.lwTs, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.maxGMV, Type2Type< MINT32 >());
        entry.push_back(mLMVLastData.isFrontBin, Type2Type< MBOOL >());
    }
    rOutputLMV.update(MTK_EIS_REGION, entry);

    MY_LOGD2("LMV(%s): %d, %d, %" PRId64 ", %d, %d, %p, "
        "Bin:%d, Mnum:%d, SofId:@%d,%d. "
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
        isLmvValid ? "valid" : "INVALID",
        mEisMode, captureIntent, exposureTime, bufIdxEis, bufIdxRrz, (void*)p3A,
        isBinEn, currMagicNum, currSofIdx, lastSofIdx,
        mLMVLastData.cmv_x_int, mLMVLastData.cmv_x_float, mLMVLastData.cmv_y_int, mLMVLastData.cmv_y_float,
        mLMVLastData.width, mLMVLastData.height, mLMVLastData.cmv_x_center, mLMVLastData.cmv_y_center,
        mLMVLastData.gmv_x, mLMVLastData.gmv_y, mLMVLastData.conf_x, mLMVLastData.conf_y, isLastSkipped, mLMVLastData.maxGMV, isBinEn);

    if( !mIsConfig3A )
    {
        if( mIsCalibration )
        {
            //Disable OIS
            p3A->send3ACtrl(E3ACtrl_SetEnableOIS, 0, 0);
            MY_LOGD2("[LMVHal] mEisMode:%d  => Disable OIS \n", mEisMode);
        }
        else if( mEisMode == EIS_MODE_OFF )
        {
            //Enable OIS
            p3A->send3ACtrl(E3ACtrl_SetEnableOIS, 1, 0);
            MY_LOGD2("[LMVHal] mEisMode:%d => Enable OIS \n", mEisMode);
        }
        mIsConfig3A = MTRUE;
    }

    #if SUPPORT_EIS
    if( mIsCalibration && mpGISCalibration != NULL )
    {
        LMV_HAL_CONFIG_DATA  config;

        MY_LOGD2("[LMVHal] captureIntent: %d, mEisMode: %d \n",
            captureIntent, mEisMode);
        for( MINT32 i = 0; i < LMV_MAX_WIN_NUM ; i++ )
        {
            config.lmvData.i4LMV_X[i] = lmvData.i4LMV_X[i];
            config.lmvData.i4LMV_Y[i] = lmvData.i4LMV_Y[i];
            config.lmvData.NewTrust_X[i] = lmvData.NewTrust_X[i];
            config.lmvData.NewTrust_Y[i] = lmvData.NewTrust_Y[i];
        }

        //GIS calibration is in preview ONLY!!
        if( captureIntent ==  MTK_CONTROL_CAPTURE_INTENT_PREVIEW )
        {
            P1_TRACE_S_BEGIN(SLG_I, "P1Connect:LMV-Calibrate");
            mpGISCalibration->DoCalibration(&config, aTimestamp, exposureTime);
            P1_TRACE_S_END(); // "P1Connect:LMV-Calibrate"
        }
    }
    #endif
    #else
    MUINT32 eisMode = 0;
    MY_LOGD2("EIS NotSupport : %d, %d, %" PRId64 ", %d, %d, %p, "
        "Bin:%d, Mnum:%d, SofId:@%d,%d",
        eisMode, captureIntent, exposureTime, bufIdxEis, bufIdxRrz, (void*)p3A,
        isBinEn, currMagicNum, currSofIdx, lastSofIdx);
    #endif // SUPPORT_LMV
};

#endif


};//namespace NSP1Node
};//namespace v3
};//namespace NSCam


