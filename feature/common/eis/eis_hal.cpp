/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES
 *AND AGREES THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK
 *SOFTWARE") RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO
 *BUYER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 *WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 *WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 *NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 *RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED
 *IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO
 *SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
 *SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 *LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT
 *MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR
 *REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK
 *FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS
 *PRINCIPLES.
 ************************************************************************************************/

/**
 * @file eis_hal.cpp
 *
 * EIS Hal Source File
 *
 */

#define LOG_TAG "EisHal"

#include <sys/stat.h>

#include <cstdio>
#include <queue>
#include <vector>

#include "android/sensor.h"
#include "camera_custom_eis.h"
#include "camera_custom_nvram.h"
#include "cutils/atomic.h"
#include "cutils/log.h"
#include "cutils/properties.h"
#include "eis/MtkEis_CalibrationData.h"
#include "eis_hal_imp.h"
#include "eis_macro.h"
#include "mtkcam/aaa/INvBufUtil.h"
#include "mtkcam/aaa/aaa_hal_common.h"
#include "mtkcam/drv/IHalSensor.h"
#include "mtkcam/drv/iopipe/CamIO/IHalCamIO.h"
#include "mtkcam/utils/imgbuf/IIonImageBufferHeap.h"
#include "mtkcam/utils/std/common.h"
#include "mtkcam/utils/sys/SensorProvider.h"
#include "mtkcam3/feature/lmv/lmv_hal.h"
#include "utils/SystemClock.h"
#include "utils/Trace.h"
#include "utils/threads.h"
using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::Utils;
using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSCamIOPipe;

using namespace NSCam::Utils;
using namespace NSCam::EIS;
using namespace NS3Av3;

/*******************************************************************************
 *
 ********************************************************************************/
#define EIS_HAL_DEBUG

#ifdef EIS_HAL_DEBUG

#undef __func__
#define __func__ __FUNCTION__

#include "mtkcam/utils/std/Log.h"
#include "mtkcam/utils/std/ULog.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_EIS_HAL);

#undef MY_LOGD
#undef MY_LOGD
#undef MY_LOGI
#undef MY_LOGW
#undef MY_LOGE
#undef MY_TRACE_API_LIFE
#undef MY_TRACE_FUNC_LIFE
#undef MY_TRACE_TAG_LIFE

#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s]" fmt, __func__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s]" fmt, __func__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s] %s ERROR(%5d):" fmt, __func__, __FILE__, __LINE__, ##arg)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_TRACE_API_LIFE() CAM_ULOGM_APILIFE()
#define MY_TRACE_FUNC_LIFE() CAM_ULOGM_FUNCLIFE()
#define MY_TRACE_TAG_LIFE(name) CAM_ULOGM_TAGLIFE(name)
#endif

#define EIS_HAL_NAME "EisHal"
#define intPartShift 8
#define floatPartShift (31 - intPartShift)
#define DEBUG_DUMP_FRAMW_NUM 10
queue<EIS_GyroRecord> emptyQueue;

MAKE_EISHAL_OBJ(0);
MAKE_EISHAL_OBJ(1);
MAKE_EISHAL_OBJ(2);
MAKE_EISHAL_OBJ(3);
MAKE_EISHAL_OBJ(4);
MAKE_EISHAL_OBJ(5);
MAKE_EISHAL_OBJ(6);
MAKE_EISHAL_OBJ(7);
MAKE_EISHAL_OBJ(8);
MAKE_EISHAL_OBJ(9);

#define EISO_BUFFER_NUM (30)
#define LEGACY_SOURCEDUMP (0)

/*******************************************************************************
 * GYRO Data
 ********************************************************************************/

typedef struct EIS_TSRecord_t {
  MUINT64 id;
  MUINT64 ts;
} EIS_TSRecord;

#define UNREASONABLE_GYRO_VALUE (10.0f)

#define IF_COND_RETURN_VALUE(cond, ret) \
  if (cond) {                           \
    MY_LOGE("Check fail!");             \
    return ret;                         \
  }

#define IF_COND_RETURN(cond) \
  if (cond) {                \
    MY_LOGE("Check fail!");  \
    return;                  \
  }

/*******************************************************************************
 *
 ********************************************************************************/

template <const unsigned int aSensorIdx>
EisHal* EisHalObj<aSensorIdx>::GetInstance(const EisInfo& eisInfo,
                                           MUINT32 MultiSensor) {
  static EisHalObj<aSensorIdx> spInstance(eisInfo, MultiSensor);
  return &spInstance;
}

template <const unsigned int aSensorIdx>
void EisHalObj<aSensorIdx>::destroyInstance(void) {}

MVOID EisHalImp::GetSensorData() {
  MBOOL bReversed = MFALSE;
  vector<SensorData> sensorData;
  {
    MY_TRACE_TAG_LIFE("SensorProvider::getAllSensorData");
    mpSensorProvider->getAllSensorData(SENSOR_TYPE_GYRO, sensorData);
  }

  EISHAL_SwitchIsGyroReverseNotZero(mSensorIdx, bReversed);

  for (MUINT32 i = 0; i < sensorData.size(); i++) {
    // place with ascending timestamp
    int index = (sensorData.front().timestamp < sensorData.back().timestamp)
                    ? i
                    : sensorData.size() - (i + 1);

    EIS_GyroRecord tmp;
    tmp.x = sensorData[index].gyro[0];
    if (UNLIKELY(bReversed)) {
      tmp.y = -sensorData[index].gyro[1];
      tmp.z = -sensorData[index].gyro[2];
    } else {
      tmp.y = sensorData[index].gyro[1];
      tmp.z = sensorData[index].gyro[2];
    }
    tmp.ts = sensorData[index].timestamp;
    EISHAL_SwitchPushGyroQueue(mSensorIdx, tmp);
  }

  mpSensorProvider->getAllSensorData(SENSOR_TYPE_ACCELERATION, sensorData);
  for (MUINT32 i = 0; i < sensorData.size(); i++) {
    int index = (sensorData.front().timestamp < sensorData.back().timestamp)
                    ? i
                    : sensorData.size() - (i + 1);
    mAccTimestamp.push_back(sensorData[index].timestamp);
    mAccData.push_back(sensorData[index].acceleration[0]);
    mAccData.push_back(sensorData[index].acceleration[1]);
    mAccData.push_back(sensorData[index].acceleration[2]);
  }

  if (mOisEnable) {
    mpSensorProvider->getAllSensorData(SENSOR_TYPE_OIS_DATA, sensorData);
    for (MUINT32 i = 0; i < sensorData.size(); i++) {
      // place with ascending timestamp
      int index = (sensorData.front().timestamp < sensorData.back().timestamp)
                      ? i
                      : sensorData.size() - (i + 1);
      mOisTimestamp.push_back(sensorData[index].timestamp);
      mOisX.push_back(sensorData[index].ois[0]);
      mOisY.push_back(sensorData[index].ois[1]);
      mOisHallX.push_back(sensorData[index].oisHall[0]);
      mOisHallY.push_back(sensorData[index].oisHall[1]);
    }
  }
}

/*******************************************************************************
 *
 ********************************************************************************/
EisHal* EisHal::CreateInstance(char const* userName,
                               const EisInfo& eisInfo,
                               const MUINT32& aSensorIdx,
                               MUINT32 MultiSensor) {
  MY_LOGD("%s", userName);
  return EisHalImp::GetInstance(eisInfo, aSensorIdx, MultiSensor);
}

/*******************************************************************************
 *
 ********************************************************************************/
EisHal* EisHalImp::GetInstance(const EisInfo& eisInfo,
                               const MUINT32& aSensorIdx,
                               MUINT32 MultiSensor) {
  switch (aSensorIdx) {
    case 0:
      return EisHalObj<0>::GetInstance(eisInfo, MultiSensor);
    case 1:
      return EisHalObj<1>::GetInstance(eisInfo, MultiSensor);
    case 2:
      return EisHalObj<2>::GetInstance(eisInfo, MultiSensor);
    case 3:
      return EisHalObj<3>::GetInstance(eisInfo, MultiSensor);
    case 4:
      return EisHalObj<4>::GetInstance(eisInfo, MultiSensor);
    case 5:
      return EisHalObj<5>::GetInstance(eisInfo, MultiSensor);
    case 6:
      return EisHalObj<6>::GetInstance(eisInfo, MultiSensor);
    case 7:
      return EisHalObj<7>::GetInstance(eisInfo, MultiSensor);
    case 8:
      return EisHalObj<8>::GetInstance(eisInfo, MultiSensor);
    case 9:
      return EisHalObj<9>::GetInstance(eisInfo, MultiSensor);
    default:
      MY_LOGW("Current limit is 10 sensors, use 0");
      return EisHalObj<0>::GetInstance(eisInfo, MultiSensor);
  }
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID EisHalImp::DestroyInstance(char const* userName) {
  Mutex::Autolock lock(mLock);
  MY_LOGD("%s", userName);

  //====== Check Reference Count ======
  if (mUsers <= 0) {
    MY_LOGD("mSensorIdx(%u) has 0 user", mSensorIdx);
    MUINT32 sensorIndex = mSensorIdx;

    switch (mSensorIdx) {
      case 0:
        EisHalObj<0>::destroyInstance();
        break;
      case 1:
        EisHalObj<1>::destroyInstance();
        break;
      case 2:
        EisHalObj<2>::destroyInstance();
        break;
      case 3:
        EisHalObj<3>::destroyInstance();
        break;
      case 4:
        EisHalObj<4>::destroyInstance();
        break;
      case 5:
        EisHalObj<5>::destroyInstance();
        break;
      case 6:
        EisHalObj<6>::destroyInstance();
        break;
      case 7:
        EisHalObj<7>::destroyInstance();
        break;
      case 8:
        EisHalObj<8>::destroyInstance();
        break;
      case 9:
        EisHalObj<9>::destroyInstance();
        break;
      default:
        MY_LOGW(" %d is not exist", mSensorIdx);
    }
    MY_LOGD("EISHal mSensorIdx(%u) is destroyed", sensorIndex);
  }
}

/*******************************************************************************
 *
 ********************************************************************************/
EisHalImp::EisHalImp(const EisInfo& eisInfo,
                     const MUINT32& aSensorIdx,
                     MUINT32 MultiSensor)
    : EisHal(),
      m_p3aHal(NULL),
      mEisInfo(eisInfo),
      mSensorIdx(aSensorIdx),
      mMultiSensor(MultiSensor) {
  mUsers = 0;

  //> member variable
  mSrzOutW = 0;
  mSrzOutH = 0;
  mGpuGridW = 0;
  mGpuGridH = 0;
  mMVWidth = 0;
  mMVHeight = 0;
  mDoEisCount = 0;  // Vent@20140427: Add for EIS GMV Sync Check.
  mFrameCnt = 0;
  mIsEisConfig = 0;
  mIsEisPlusConfig = 0;
  mEisPlusCropRatio = 0;
  mCurSensorIdx = 0;
  mEisSupport = MTRUE;
  mEisMode = 0;
  mEISInterval = 0;
  mSourceDumpIdx = 0;
  mGyroEnable = MFALSE;
  mAccEnable = MFALSE;
  mEisPlusResult.GridX = 0;
  mEisPlusResult.GridY = 0;
  mEisPlusResult.ClipX = 0;
  mEisPlusResult.ClipY = 0;
  mEisPlusResult.GridX_standard = 0;
  mEisPlusResult.GridY_standard = 0;

  mTsForAlgoDebug = 0;
  m_pNvram = NULL;
  mGisInputW = 0;
  mGisInputH = 0;
  mSkipWaitGyro = MFALSE;
  mbEMSaveFlag = MFALSE;

  mEMEnabled = 0;
  mDebugDump = 0;
  mSourceDump = 0;
  mQuickDump = 0;
  mFirstFrameTs = 0;
  mpSourceDumpFp = NULL;
  mpOISFp = NULL;
  mDisableGyroData = 0;
  mGyroOnly = 0;
  mImageOnly = 0;
  mForecDejello = 0;
  m_pLMVHal = NULL;
  m_pEisPlusAlg = NULL;
  m_pGisAlg = NULL;

  mChangedInCalibration = 0;
  mNVRAMRead = MFALSE;
  mSleepTime = 0;
  mbLastCalibration = MTRUE;
  m_pNVRAM_defParameter = NULL;
  m_pEisDbgBuf = NULL;
  m_pEisPlusWorkBuf = NULL;
  m_pGisWorkBuf = NULL;

  // Sensor
  m_pHalSensorList = NULL;
  m_pHalSensor = NULL;

  memset(&mEisPlusAlgoInitData, 0, sizeof(mEisPlusAlgoInitData));
  memset(&mEisPlusGetProcData, 0, sizeof(mEisPlusGetProcData));
  memset(&mGyroAlgoInitData, 0, sizeof(mGyroAlgoInitData));
}

/*******************************************************************************
 *
 ********************************************************************************/
MINT32 EisHalImp::Init() {
  MY_TRACE_API_LIFE();

  //====== Check Reference Count ======
  Mutex::Autolock lock(mLock);

  if (mUsers > 0) {
    android_atomic_inc(&mUsers);
    MY_LOGD("sensorIdx(%u) has %d users", mSensorIdx, mUsers);
    return EIS_RETURN_NO_ERROR;
  }

  //====== Dynamic Debug ======
  mEISInterval = EISCustom::getGyroIntervalMs();
  mEMEnabled = 0;
  mDebugDump = 0;
  mDisableGyroData = 0;

  mDebugDump = property_get_int32("vendor.debug.eis.dump", mDebugDump);
  mEMEnabled = property_get_int32("vendor.debug.eis.EMEnabled", mEMEnabled);
  mEISInterval =
      property_get_int32("vendor.debug.eis.setinterval", mEISInterval);
  mOISInterval =
      property_get_int32("vendor.debug.eis.ois.setinterval", mOISInterval);
  mDisableGyroData =
      property_get_int32("vendor.debug.eis.disableGyroData", mDisableGyroData);
  mSourceDump = property_get_int32("vendor.debug.eis.sourcedump", mSourceDump);
  mCentralCrop =
      property_get_int32("vendor.debug.eis.centralcrop", mCentralCrop);

  mGyroOnly = property_get_int32("vendor.debug.eis.GyroOnly", mGyroOnly);
  mImageOnly = property_get_int32("vendor.debug.eis.ImageOnly", mImageOnly);
  mForecDejello =
      property_get_int32("vendor.debug.eis.forceDejello", mForecDejello);

  if (mSourceDump >= 1) {
    if (mkdir(EIS_DUMP_FOLDER_PATH, S_IRWXU | S_IRWXG))
      MY_LOGW("mkdir %s failed", EIS_DUMP_FOLDER_PATH);
  }

  EISHAL_SwitchClearGyroQueue(mSensorIdx);
  EISHAL_SwitchSetGyroCountZero(mSensorIdx);

  mCurSensorIdx = mSensorIdx;
  m_pLMVHal = NULL;
  // Init GIS member data
  m_pNvram = NULL;
  mChangedInCalibration = 0;
  mNVRAMRead = MFALSE;
  mSleepTime = 0;

  EISHAL_SwitchSetGyroReverseValue(mSensorIdx, 0);
  EISHAL_SwitchSetLastGyroTimestampValue(mSensorIdx, 0);
  mbLastCalibration = MTRUE;
  mbEMSaveFlag = MFALSE;
  m_pNVRAM_defParameter = NULL;
  m_pNVRAM_defParameter = new NVRAM_CAMERA_FEATURE_GIS_STRUCT;
  memset(m_pNVRAM_defParameter, 0, sizeof(NVRAM_CAMERA_FEATURE_GIS_STRUCT));
  memset(mSensorInfo, 0, sizeof(mSensorInfo));

  MY_LOGD("(%p)  mSensorIdx(%u) init", this, mSensorIdx);

  //====== Create Sensor Object ======

  m_pHalSensorList = MAKE_HalSensorList();
  if (m_pHalSensorList == NULL) {
    MY_LOGE("IHalSensorList::get fail");
    goto create_fail_exit;
  }

  m_p3aHal = MAKE_Hal3A(static_cast<MINT32>(mSensorIdx), "EISHal");
  if (m_p3aHal == NULL) {
    MY_LOGE("m_p3aHal create fail");
    goto create_fail_exit;
  }

  //====== Create EIS Algorithm Object ======

  m_pEisPlusAlg = MTKEisPlus::createInstance(&mEisPlusInitParam);

  if (m_pEisPlusAlg == NULL) {
    MY_LOGE("MTKEisPlus::createInstance fail");
    goto create_fail_exit;
  }

  m_pGisAlg = MTKGyro::createInstance();

  if (m_pGisAlg == NULL) {
    MY_LOGE("MTKGyro::createInstance fail");
    goto create_fail_exit;
  }

#ifdef SENSOR_HUB_OIS
  mOisEnable = MTRUE;
#endif

  //====== Create Sensor Provider Object ======
  EnableSensor();

  //====== Increase User Count ======

  android_atomic_inc(&mUsers);

  MY_LOGD("-");
  return EIS_RETURN_NO_ERROR;

create_fail_exit:

  if (m_pHalSensorList != NULL) {
    m_pHalSensorList = NULL;
  }

  if (m_pEisPlusAlg != NULL) {
    m_pEisPlusAlg->EisPlusReset();
    m_pEisPlusAlg->destroyInstance(m_pEisPlusAlg);
    m_pEisPlusAlg = NULL;
  }

  MY_LOGD("-");
  return EIS_RETURN_NULL_OBJ;
}

/*******************************************************************************
 *
 ********************************************************************************/
MINT32 EisHalImp::Uninit() {
  MY_TRACE_API_LIFE();

  Mutex::Autolock lock(mLock);

  //====== Check Reference Count ======

  if (mUsers <= 0) {
    MY_LOGD("mSensorIdx(%u) has 0 user", mSensorIdx);
    return EIS_RETURN_NO_ERROR;
  }

  //====== Uninitialize ======

  android_atomic_dec(&mUsers);  // decrease referebce count

  if (mUsers == 0)  // there is no user
  {
    MINT32 err = EIS_RETURN_NO_ERROR;

    MY_LOGD("mSensorIdx(%u) uninit", mSensorIdx);

    //======  Release EIS Algo Object ======

    if (UNLIKELY(mDebugDump >= 2)) {
      MY_LOGD("mIsEisPlusConfig(%d)", mIsEisPlusConfig);
      if (mIsEisPlusConfig == 1) {
        err = m_pEisPlusAlg->EisPlusFeatureCtrl(EIS_PLUS_FEATURE_SAVE_LOG,
                                                (MVOID*)&mTsForAlgoDebug, NULL);
        if (err != S_EIS_PLUS_OK) {
          MY_LOGE("EisPlusFeatureCtrl(EIS_PLUS_FEATURE_SAVE_LOG) fail(0x%x)",
                  err);
        }
      }
    }

    if (UNLIKELY(mSourceDump || mDebugDump >= 2 || mEMEnabled == 1)) {
      MY_LOGD("mIsEisPlusConfig(%d)", mIsEisPlusConfig);
      if (mIsEisPlusConfig == 1) {
        err = m_pGisAlg->GyroFeatureCtrl(GYRO_FEATURE_SAVE_LOG,
                                         (MVOID*)&mTsForAlgoDebug, NULL);
        if (err != S_GYRO_OK) {
          MY_LOGE("GyroFeatureCtrl(GYRO_FEATURE_SAVE_LOG) fail(0x%x)", err);
        }
      }
    }

    // Writeback to NVRAM
    if (mNVRAMRead) {
      // Force update the NVRAM tmp buffer
      if (m_pNvram && m_pNVRAM_defParameter) {
        memcpy(&(m_pNvram->gis), m_pNVRAM_defParameter,
               sizeof(NVRAM_CAMERA_FEATURE_GIS_STRUCT));
      }
      if (auto pNvBufUtil = MAKE_NvBufUtil()) {
        err = pNvBufUtil->write(CAMERA_NVRAM_DATA_FEATURE,
                                mSensorInfo[mSensorIdx].mSensorDev);
      }
    }

    if (m_pGisAlg != NULL) {
      MY_LOGD("m_pGisAlg uninit");
      m_pGisAlg->GyroReset();
      m_pGisAlg->destroyInstance(m_pGisAlg);
      m_pGisAlg = NULL;
    }

    if (m_pEisPlusAlg != NULL) {
      MY_LOGD("m_pEisPlusAlg uninit");
      m_pEisPlusAlg->EisPlusReset();
      m_pEisPlusAlg->destroyInstance(m_pEisPlusAlg);
      m_pEisPlusAlg = NULL;
    }

    // Next-Gen EIS
    if (mpSensorProvider != NULL) {
      MY_LOGD("mpSensorProvider uninit");
      mpSensorProvider->disableSensor(SENSOR_TYPE_ACCELERATION);
      mpSensorProvider->disableSensor(SENSOR_TYPE_GYRO);
      if (mOisEnable) {
        mpSensorProvider->disableSensor(SENSOR_TYPE_OIS_DATA);
      }
      mpSensorProvider = NULL;
    }
    mOisEnable = MFALSE;

    //====== Destroy Sensor Object ======

    if (m_pHalSensorList != NULL) {
      m_pHalSensorList = NULL;
    }

    if (m_p3aHal != NULL) {
      m_p3aHal->destroyInstance("EISHal");
      m_p3aHal = NULL;
    }

    //======  Release Memory and IMem Object ======
    //>  free EIS Plus working buffer
    if (m_pEisPlusWorkBuf != NULL) {
      m_pEisPlusWorkBuf->unlockBuf("EISPlusWorkBuf");
      DestroyMemBuf(m_pEisPlusWorkBuf);
    }

    //>  free GIS working buffer

    if (m_pGisWorkBuf != NULL) {
      m_pGisWorkBuf->unlockBuf("GisWorkBuf");
      DestroyMemBuf(m_pGisWorkBuf);
    }

    //======  Reset Member Variable ======
    if (m_pLMVHal != NULL) {
      m_pLMVHal->DestroyInstance(LOG_TAG);
      m_pLMVHal = NULL;
    }

    mSrzOutW = 0;
    mSrzOutH = 0;
    mFrameCnt = 0;  // first frmae
    mIsEisConfig = 0;
    mIsEisPlusConfig = 0;
    mDoEisCount = 0;  // Vent@20140427: Add for EIS GMV Sync Check.
    mGpuGridW = 0;
    mGpuGridH = 0;
    mMVWidth = 0;
    mMVHeight = 0;
    mGyroEnable = MFALSE;
    mAccEnable = MFALSE;

    mChangedInCalibration = 0;
    mGisInputW = 0;
    mGisInputH = 0;
    mNVRAMRead = MFALSE;
    m_pNvram = NULL;
    mSleepTime = 0;
    mbLastCalibration = MTRUE;
    EISHAL_SwitchSetGyroCountZero(mSensorIdx);
    EISHAL_SwitchSetLastGyroTimestampValue(mSensorIdx, 0);

    mbEMSaveFlag = MFALSE;

    if (NULL != m_pNVRAM_defParameter) {
      delete m_pNVRAM_defParameter;
      m_pNVRAM_defParameter = NULL;
    }

    mEISInterval = 0;
    mEMEnabled = 0;
    mDebugDump = 0;
    mDisableGyroData = 0;
  } else {
    MY_LOGD("mSensorIdx(%u) has %d users", mSensorIdx, mUsers);
  }

  if (mpSourceDumpFp != NULL) {
    closeSourceDumpFile();
  }

  EISHAL_SwitchClearGyroQueue(mSensorIdx);

  return EIS_RETURN_NO_ERROR;
}

/*******************************************************************************
 *
 ********************************************************************************/
MINT32 EisHalImp::CreateMemBuf(MUINT32 memSize,
                               android::sp<IImageBuffer>& spImageBuf) {
  MY_TRACE_FUNC_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;
  IImageBufferAllocator* pImageBufferAlloc =
      IImageBufferAllocator::getInstance();

  IImageBufferAllocator::ImgParam bufParam((size_t)memSize, 0);
  spImageBuf = pImageBufferAlloc->alloc("EIS_HAL", bufParam);
  return err;
}

/******************************************************************************
 *
 *******************************************************************************/
MINT32 EisHalImp::DestroyMemBuf(android::sp<IImageBuffer>& spImageBuf) {
  MY_TRACE_FUNC_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;
  IImageBufferAllocator* pImageBufferAlloc =
      IImageBufferAllocator::getInstance();
  MY_LOGD("DestroyMemBuf");

  if (spImageBuf != NULL) {
    pImageBufferAlloc->free(spImageBuf.get());
    spImageBuf = NULL;
  }

  MY_LOGD("X");
  return err;
}

/*******************************************************************************
 *
 ********************************************************************************/
MINT32 EisHalImp::GetSensorInfo(MUINT32 sensorID) {
  MY_TRACE_FUNC_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;

  mSensorInfo[sensorID].mSensorDev =
      m_pHalSensorList->querySensorDevIdx(sensorID);

  m_pHalSensor = m_pHalSensorList->createSensor(EIS_HAL_NAME, 1, &sensorID);

  if (LIKELY(m_pHalSensor != NULL)) {
    err = m_pHalSensor->sendCommand(
        mSensorInfo[sensorID].mSensorDev, SENSOR_CMD_GET_PIXEL_CLOCK_FREQ,
        (MUINTPTR)&mSensorInfo[sensorID].mSensorPixelClock, 0, 0);
    if (err != EIS_RETURN_NO_ERROR) {
      MY_LOGE("SENSOR_CMD_GET_PIXEL_CLOCK_FREQ is fail(0x%x)", err);
      return EIS_RETURN_API_FAIL;
    }

    err = m_pHalSensor->sendCommand(
        mSensorInfo[sensorID].mSensorDev,
        SENSOR_CMD_GET_FRAME_SYNC_PIXEL_LINE_NUM,
        (MUINTPTR)&mSensorInfo[sensorID].mSensorLinePixel, 0, 0);
    if (err != EIS_RETURN_NO_ERROR) {
      MY_LOGE("SENSOR_CMD_GET_PIXEL_CLOCK_FREQ is fail(0x%x)", err);
      return EIS_RETURN_API_FAIL;
    }

    SensorCropWinInfo cropInfo;
    err = m_pHalSensor->sendCommand(
        mSensorInfo[sensorID].mSensorDev, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
        (MUINTPTR)&mSensorMode, (MUINTPTR)&cropInfo, 0);

    if (err != 0 || cropInfo.full_w == 0 || cropInfo.full_h == 0 ||
        cropInfo.w0_size == 0 || cropInfo.h0_size == 0) {
      MY_LOGW("Get sensor crop info failed[%d], (%d,%d,%d,%d)", err,
              cropInfo.full_w, cropInfo.full_h, cropInfo.w0_size,
              cropInfo.h0_size);
    } else {
      mSensorInfo[sensorID].mSensorCropInfo = cropInfo;
    }

    m_pHalSensor->destroyInstance(EIS_HAL_NAME);
    m_pHalSensor = NULL;

  } else {
    MY_LOGE("m_pHalSensorList->createSensor fail, m_pHalSensor == NULL");
  }

  MY_LOGD("mSensorDev(%u), pixelclock (%d), pixelline(%x)",
          mSensorInfo[sensorID].mSensorDev,
          mSensorInfo[sensorID].mSensorPixelClock,
          mSensorInfo[sensorID].mSensorLinePixel);

  return EIS_RETURN_NO_ERROR;
}

MINT32 EisHalImp::EnableSensor() {
  {
    MY_TRACE_TAG_LIFE("SensorProvider::createInstance");
    mpSensorProvider = SensorProvider::createInstance(EIS_HAL_NAME);
  }

  {
    MY_TRACE_TAG_LIFE("SensorProvider::enableSensor");
    mAccEnable =
        mpSensorProvider->enableSensor(SENSOR_TYPE_ACCELERATION, mAccInterval);
    mGyroEnable =
        mpSensorProvider->enableSensor(SENSOR_TYPE_GYRO, mEISInterval);
    if (mOisEnable) {
      mpSensorProvider->enableSensor(SENSOR_TYPE_OIS_DATA, mOISInterval);
    }
  }
  MY_LOGD("EN:(Acc,Gyro)=(%d,%d), interval(gyro,ois)=(%d,%d)", mAccEnable,
          mGyroEnable, mEISInterval, mOISInterval);

  if (m_pLMVHal == NULL) {
    m_pLMVHal = LMVHal::CreateInstance(LOG_TAG, mSensorIdx);
  }

  return EIS_RETURN_NO_ERROR;
}

MINT32 EisHalImp::CalcSensorTrs(MUINT32 sensorID, MUINT32 sensorHeight) {
  MDOUBLE tRS = 0.0f, numLine;
  numLine = mSensorInfo[sensorID].mSensorLinePixel & 0xFFFF;
  if (mSensorInfo[sensorID].mSensorPixelClock != 0) {
    tRS = numLine / mSensorInfo[sensorID].mSensorPixelClock;
    tRS = tRS * static_cast<float>(sensorHeight - 1);
  } else {
    MY_LOGW("mSensorPixelClock is 0, so can NOT get tRS");
  }

  MY_LOGD("tRS in table: %f, calculated tRS: %f",
          m_pNVRAM_defParameter->gis_deftRS[0], tRS);

  // Replace the tRS from the table by current sensor mode
  mSensorInfo[sensorID].mRecordParameter[0] = tRS;
  mTRS = tRS;
  tRS += mSensorInfo[sensorID].mRecordParameter[5];

  // Check 30 fps maxmum
  if (tRS > 0.042f) {
    MY_LOGW("30 fps tRS+tOffset: %f should be small than 0.042 ms", tRS);
  }
  mSensorInfo[sensorID].mtRSTime = (MINT64)((MDOUBLE)tRS * 1000000.0f);
  MY_LOGD("waiting gyro time: %" PRIi64 " ", mSensorInfo[sensorID].mtRSTime);
  return EIS_RETURN_NO_ERROR;
}

MVOID EisHalImp::AddMultiSensors(MUINT32 sensorID) {
  if (sensorID >= 0 && sensorID < MAX_SENSOR_ID) {
    if (mSensorInfo[sensorID].mUsed == MTRUE) {
      MY_LOGW("mSensorInfo[%d] is used", sensorID);
    } else {
      mSensorInfo[sensorID].mUsed = MTRUE;
      if (UNLIKELY(mDebugDump > 0)) {
        MY_LOGD("Add sensor=%d", sensorID);
      }
      if (EIS_RETURN_NO_ERROR != GetSensorInfo(sensorID)) {
        MY_LOGE("GetSensorInfo fail");
      }
    }
  }
}

MVOID EisHalImp::configMVNumber(const EIS_HAL_CONFIG_DATA& apEisConfig,
                                MINT32* mvWidth,
                                MINT32* mvHeight) {
  if (apEisConfig.srzOutW == 0 || apEisConfig.srzOutH == 0) {
    MY_LOGE("configMVNumber failed, width=%d, height=%d", apEisConfig.srzOutW,
            apEisConfig.srzOutH);
    return;
  }

  EISCustom::getMVNumber(apEisConfig.srzOutW, apEisConfig.srzOutH, mvWidth,
                         mvHeight);
}

MBOOL EisHalImp::getCalibrationData() {
  EIS50_CALIBRATION_DATA& caliData =
    mEisPlusAlgoInitData.EIS50_CalibrationData;
  // Consider use struct instead
  if (mSensorIdx == MAIN_SENSOR_ID) {
    caliData.mesh_width = MAINCAM_mesh_Width;
    caliData.mesh_height = MAINCAM_mesh_Height;
    caliData.meshVertXY = MAINCAM_meshVertXY;
    caliData.meshVertLngLat = MAINCAM_meshVertLngLat;
    caliData.meshTriangleVertIndices = MAINCAM_meshTriangleVertIndices;
    caliData.calibration_Domain_Width = MAINCAM_calibration_Domain_Width;
    caliData.calibration_Domain_Height = MAINCAM_calibration_Domain_Height;
    caliData.calibration_source = MAINCAM_calibration_source;
    caliData.num_AfLensPosition = MAINCAM_num_AfLensPosition;
    caliData.num_OpticalZoomPosition = MAINCAM_num_OpticalZoomPosition;
    caliData.normalized_AfLensPositions = MAINCAM_normalized_AfLensPositions;
    caliData.normalized_OpticalZoomPositions =
      MAINCAM_normalized_OpticalZoomPositions;
  } else if (mSensorIdx == FRONT_SENSOR_ID) {
    caliData.mesh_width = FRONTCAM_mesh_Width;
    caliData.mesh_height = FRONTCAM_mesh_Height;
    caliData.meshVertXY = FRONTCAM_meshVertXY;
    caliData.meshVertLngLat = FRONTCAM_meshVertLngLat;
    caliData.meshTriangleVertIndices = FRONTCAM_meshTriangleVertIndices;
    caliData.calibration_Domain_Width = FRONTCAM_calibration_Domain_Width;
    caliData.calibration_Domain_Height = FRONTCAM_calibration_Domain_Height;
    caliData.calibration_source = FRONTCAM_calibration_source;
    caliData.num_AfLensPosition = FRONTCAM_num_AfLensPosition;
    caliData.num_OpticalZoomPosition = FRONTCAM_num_OpticalZoomPosition;
    caliData.normalized_AfLensPositions = FRONTCAM_normalized_AfLensPositions;
    caliData.normalized_OpticalZoomPositions =
      FRONTCAM_normalized_OpticalZoomPositions;
  } else if (mSensorIdx == ULTRAWIDE_SENSOR_ID) {
    caliData.mesh_width = ULTRAWIDECAM_mesh_Width;
    caliData.mesh_height = ULTRAWIDECAM_mesh_Height;
    caliData.meshVertXY = ULTRAWIDECAM_meshVertXY;
    caliData.meshVertLngLat = ULTRAWIDECAM_meshVertLngLat;
    caliData.meshTriangleVertIndices = ULTRAWIDECAM_meshTriangleVertIndices;
    caliData.calibration_Domain_Width = ULTRAWIDECAM_calibration_Domain_Width;
    caliData.calibration_Domain_Height =
      ULTRAWIDECAM_calibration_Domain_Height;
    caliData.calibration_source = ULTRAWIDECAM_calibration_source;
    caliData.num_AfLensPosition = ULTRAWIDECAM_num_AfLensPosition;
    caliData.num_OpticalZoomPosition = ULTRAWIDECAM_num_OpticalZoomPosition;
    caliData.normalized_AfLensPositions =
      ULTRAWIDECAM_normalized_AfLensPositions;
    caliData.normalized_OpticalZoomPositions =
      ULTRAWIDECAM_normalized_OpticalZoomPositions;
  } else if (mSensorIdx == TELE_SENSOR_ID) {
    caliData.mesh_width = TELECAM_mesh_Width;
    caliData.mesh_height = TELECAM_mesh_Height;
    caliData.meshVertXY = TELECAM_meshVertXY;
    caliData.meshVertLngLat = TELECAM_meshVertLngLat;
    caliData.meshTriangleVertIndices = TELECAM_meshTriangleVertIndices;
    caliData.calibration_Domain_Width = TELECAM_calibration_Domain_Width;
    caliData.calibration_Domain_Height = TELECAM_calibration_Domain_Height;
    caliData.calibration_source = TELECAM_calibration_source;
    caliData.num_AfLensPosition = TELECAM_num_AfLensPosition;
    caliData.num_OpticalZoomPosition = TELECAM_num_OpticalZoomPosition;
    caliData.normalized_AfLensPositions = TELECAM_normalized_AfLensPositions;
    caliData.normalized_OpticalZoomPositions =
      TELECAM_normalized_OpticalZoomPositions;
  } else {
    caliData.calibration_source = 3;
    MY_LOGI("No EIS50_CalibrationData for sensor[%d]", mSensorIdx);
  }

  return true;
}

MINT32 EisHalImp::ConfigGyroAlgo(
    const EIS_HAL_CONFIG_DATA& aEisConfig,
    const EIS_PLUS_SET_ENV_INFO_STRUCT& eisInitData) {
  MY_TRACE_FUNC_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;

  GYRO_GET_PROC_INFO_STRUCT gyroGetProcData;
  GYRO_SET_WORKING_BUFFER_STRUCT gyroSetworkingbuffer;
  MUINT64 timewithSleep = elapsedRealtime();
  MUINT64 timewithoutSleep = uptimeMillis();

  if (m_pLMVHal->GetLMVStatus()) {
    m_pLMVHal->GetRegSetting(&mGyroAlgoInitData);
  }
  mGyroAlgoInitData.GyroCalInfo.GYRO_sample_rate = 1000 / mEISInterval;
  mGyroAlgoInitData.eis_mode = eisInitData.EIS_mode;

  // Enable available sensor
  mSensorInfo[mSensorIdx].mUsed = MTRUE;  // Default sensor must be enabled.
  if (aEisConfig.is_multiSensor) {
    AddMultiSensors(aEisConfig.fov_wide_idx);
    AddMultiSensors(aEisConfig.fov_tele_idx);
  }

  if (EIS_RETURN_NO_ERROR != GetSensorInfo(mSensorIdx)) {
    MY_LOGE("GetSensorInfo fail");
    return EIS_RETURN_INVALID_DRIVER;
  }

  mGyroAlgoInitData.MVWidth = mMVWidth;
  mGyroAlgoInitData.MVHeight = mMVHeight;

  mGyroAlgoInitData.sensor_Width = aEisConfig.sensor_Width;
  mGyroAlgoInitData.sensor_Height = aEisConfig.sensor_Height;

  mGyroAlgoInitData.rrz_crop_X = aEisConfig.rrz_crop_X;
  mGyroAlgoInitData.rrz_crop_Y = aEisConfig.rrz_crop_Y;

  mGyroAlgoInitData.rrz_crop_Width = aEisConfig.rrz_crop_Width;
  mGyroAlgoInitData.rrz_crop_Height = aEisConfig.rrz_crop_Height;

  mGyroAlgoInitData.rrz_scale_Width = aEisConfig.rrz_scale_Width;
  mGyroAlgoInitData.rrz_scale_Height = aEisConfig.rrz_scale_Height;

  mGyroAlgoInitData.fov_align_Width = aEisConfig.fov_align_Width;
  mGyroAlgoInitData.fov_align_Height = aEisConfig.fov_align_Height;

  mGyroAlgoInitData.FSCSlices = mFSCInfo.numSlices;
  mGyroAlgoInitData.FSC_en = mFSCInfo.isEnabled;
  mGyroAlgoInitData.EnableInterIntraAngle =
      (mAlgoEisMode == ALGO_EIS50_MODE) ? true : false;
  mGyroAlgoInitData.RowSliceNum = RowSliceNum;

  //====== Turn on Eis Configure One Time Flag ======
  if ((mSensorInfo[mSensorIdx].mSensorDev == SENSOR_DEV_SUB) ||
      (mSensorInfo[mSensorIdx].mSensorDev == SENSOR_DEV_SUB_2)) {
    EISHAL_SwitchSetGyroReverseValue(mSensorIdx, 1);
    MY_LOGD("mSensorDev(%u), GYRO data reversed",
            mSensorInfo[mSensorIdx].mSensorDev);
  } else {
    EISHAL_SwitchSetGyroReverseValue(mSensorIdx, 0);
    MY_LOGD("mSensorDev(%u), GYRO data normal",
            mSensorInfo[mSensorIdx].mSensorDev);
  }

  if (UNLIKELY(mMultiSensor == 1))  // Multi Sensors
  {
    MUINT32 i;
    for (i = 0; i < MAX_SENSOR_ID; i++) {
      if (mSensorInfo[i].mUsed == MTRUE) {
        //====== Read NVRAM calibration data ======
        auto pNvBufUtil = MAKE_NvBufUtil();
        err = (!pNvBufUtil) ? EIS_RETURN_NULL_OBJ
                            : pNvBufUtil->getBufAndRead(
                                  CAMERA_NVRAM_DATA_FEATURE,
                                  mSensorInfo[i].mSensorDev, (void*&)m_pNvram);
        if (err == 0) {
          if (i == mSensorIdx) {
            memcpy(m_pNVRAM_defParameter, &(m_pNvram->gis),
                   sizeof(NVRAM_CAMERA_FEATURE_GIS_STRUCT));
          }
          memcpy(mSensorInfo[i].mRecordParameter,
                 m_pNvram->gis.gis_defParameter3,
                 sizeof(mSensorInfo[i].mRecordParameter));
          if (UNLIKELY(mDebugDump > 0)) {
            MY_LOGD(
                "SensorIdx=%d, SensorDev=%d, param[0]=%f, param[1]=%f, "
                "param[2]=%f, param[3]=%f, param[4]=%f, param[5]=%f",
                i, mSensorInfo[i].mSensorDev,
                mSensorInfo[i].mRecordParameter[0],
                mSensorInfo[i].mRecordParameter[1],
                mSensorInfo[i].mRecordParameter[2],
                mSensorInfo[i].mRecordParameter[3],
                mSensorInfo[i].mRecordParameter[4],
                mSensorInfo[i].mRecordParameter[5]);
          }
          mSensorInfo[i].mDefWidth = m_pNvram->gis.gis_defWidth;
          mSensorInfo[i].mDefHeight = m_pNvram->gis.gis_defHeight;
          mSensorInfo[i].mDefCrop = m_pNvram->gis.gis_defCrop;
        } else {
          MY_LOGE(
              "INvBufUtil::getBufAndRead is failed! SensorIdx(%d), "
              "SensorDev(%d)",
              i, mSensorInfo[i].mSensorDev);
          return EIS_RETURN_NULL_OBJ;
        }
      }
    }
  } else  // Single sensor
  {
    //====== Read NVRAM calibration data ======
    if (auto pNvBufUtil = MAKE_NvBufUtil()) {
      err = pNvBufUtil->getBufAndRead(CAMERA_NVRAM_DATA_FEATURE,
                                      mSensorInfo[mSensorIdx].mSensorDev,
                                      (void*&)m_pNvram);
    }
    if (m_pNVRAM_defParameter && m_pNvram) {
      memcpy(m_pNVRAM_defParameter, &(m_pNvram->gis),
             sizeof(NVRAM_CAMERA_FEATURE_GIS_STRUCT));
      switch (aEisConfig.sensorMode) {
        case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
          memcpy(mSensorInfo[mSensorIdx].mRecordParameter,
                 m_pNvram->gis.gis_defParameter1,
                 sizeof(mSensorInfo[mSensorIdx].mRecordParameter));
          break;
        case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
          memcpy(mSensorInfo[mSensorIdx].mRecordParameter,
                 m_pNvram->gis.gis_defParameter2,
                 sizeof(mSensorInfo[mSensorIdx].mRecordParameter));
          break;
        case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
          memcpy(mSensorInfo[mSensorIdx].mRecordParameter,
                 m_pNvram->gis.gis_defParameter3,
                 sizeof(mSensorInfo[mSensorIdx].mRecordParameter));
          break;
        default:
          MY_LOGW("Unsupported sensorMode %d, use previewMode as default",
                  aEisConfig.sensorMode);
          memcpy(mSensorInfo[mSensorIdx].mRecordParameter,
                 m_pNvram->gis.gis_defParameter1,
                 sizeof(mSensorInfo[mSensorIdx].mRecordParameter));
          break;
      }
      mSensorInfo[mSensorIdx].mDefWidth = m_pNvram->gis.gis_defWidth;
      mSensorInfo[mSensorIdx].mDefHeight = m_pNvram->gis.gis_defHeight;
      mSensorInfo[mSensorIdx].mDefCrop = m_pNvram->gis.gis_defCrop;

    } else {
      MY_LOGE("m_pNVRAM_defParameter OR m_pNVRAM_defParameter is NULL\n");
      return EIS_RETURN_NULL_OBJ;
    }
  }

  mNVRAMRead = MFALSE;  // No write back

  //> prepare eisPlusAlgoInitData
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  CalcSensorTrs(mSensorIdx, aEisConfig.sensor_Height);

  EISCustom::getGISParameter(mSensorIdx,
                             mSensorInfo[mSensorIdx].mRecordParameter,
                             &(mSensorInfo[mSensorIdx].mDefWidth),
                             &(mSensorInfo[mSensorIdx].mDefHeight),
                             &(mSensorInfo[mSensorIdx].mDefCrop));

  mGyroAlgoInitData.param_Width = mSensorInfo[mSensorIdx].mDefWidth;
  mGyroAlgoInitData.param_Height = mSensorInfo[mSensorIdx].mDefHeight;
  mGyroAlgoInitData.param_crop_Y = mSensorInfo[mSensorIdx].mDefCrop;
  mGyroAlgoInitData.ProcMode = GYRO_PROC_MODE_MV;
  mGyroAlgoInitData.param = mSensorInfo[mSensorIdx].mRecordParameter;
  if (mStaggerNum > 1) {
    mGyroAlgoInitData.param[0] = mTRS * mStaggerNum;
    MY_LOGD("[EIS Stagger] StaggerNum[%d], tRS=%f", mStaggerNum,
            mTRS * mStaggerNum);
  } else {
    mGyroAlgoInitData.param[0] = mTRS;
  }
  memcpy(mGyroAlgoInitData.NVRAM_TuningParameters, m_pNvram->gis.gis_deftRS,
         sizeof(mGyroAlgoInitData.NVRAM_TuningParameters));
  mGyroAlgoInitData.sleep_t = timewithSleep - timewithoutSleep;
  mSleepTime = mGyroAlgoInitData.sleep_t;
  mFocalLength = mSensorInfo[mSensorIdx].mRecordParameter[4];

  MY_LOGD(
      "def data Rec:(%d, %d)(sensorMode:%d) %f    %f    %f    %f    %f    %f",
      mSensorInfo[mSensorIdx].mDefWidth, mSensorInfo[mSensorIdx].mDefHeight,
      aEisConfig.sensorMode, mSensorInfo[mSensorIdx].mRecordParameter[0],
      mSensorInfo[mSensorIdx].mRecordParameter[1],
      mSensorInfo[mSensorIdx].mRecordParameter[2],
      mSensorInfo[mSensorIdx].mRecordParameter[3],
      mSensorInfo[mSensorIdx].mRecordParameter[4],
      mSensorInfo[mSensorIdx].mRecordParameter[5]);

  mGyroAlgoInitData.crz_crop_X = aEisConfig.cropX;
  mGyroAlgoInitData.crz_crop_Y = aEisConfig.cropY;

  mGyroAlgoInitData.crz_crop_Width = aEisConfig.crzOutW;
  mGyroAlgoInitData.crz_crop_Height = aEisConfig.crzOutH;

  MBOOL isEIS30Opt = EIS_MODE_IS_EIS_30_ENABLED(mEisMode) ? MTRUE : MFALSE;

  if ((aEisConfig.srzOutW <= D1_WIDTH) && (aEisConfig.srzOutH <= D1_HEIGHT)) {
    mGyroAlgoInitData.block_size = 8;
  } else if ((aEisConfig.srzOutW <= ALGOPT_FHD_THR_W) &&
             (aEisConfig.srzOutH <= ALGOPT_FHD_THR_H)) {
    mGyroAlgoInitData.block_size = isEIS30Opt ? 48 : 16;
  } else {
    mGyroAlgoInitData.block_size = 96;
  }

  if (UNLIKELY(mSourceDump || mQuickDump || mDebugDump >= 2)) {
    mGyroAlgoInitData.debug = MTRUE;
  }
  MY_LOGD("sleep_t is (%" PRIu64
          ") aEisConfig IMG w(%d),h(%d) crz crop: (%d,%d,%dx%d) "
          "gyroAlgoInitData block_size(%f)",
          mGyroAlgoInitData.sleep_t, aEisConfig.imgiW, aEisConfig.imgiH,
          mGyroAlgoInitData.crz_crop_X, mGyroAlgoInitData.crz_crop_Y,
          mGyroAlgoInitData.crz_crop_Width, mGyroAlgoInitData.crz_crop_Height,
          mGyroAlgoInitData.block_size);

  if (UNLIKELY(mDebugDump > 0) && mFSCInfo.isEnabled) {
    MY_LOGD("FSC_en=%d(slice=%d)", mGyroAlgoInitData.FSC_en,
            mGyroAlgoInitData.FSCSlices);
  }

  err = m_pGisAlg->GyroInit(&mGyroAlgoInitData);
  if (err != S_GYRO_OK) {
    MY_LOGE("GyroInit fail(0x%x)", err);
    return EIS_RETURN_API_FAIL;
  }

  err = m_pGisAlg->GyroFeatureCtrl(GYRO_FEATURE_GET_PROC_INFO, NULL,
                                   &gyroGetProcData);
  if (err != S_GYRO_OK) {
    MY_LOGE("get Gyro proc info fail(0x%x)", err);
    return EIS_RETURN_API_FAIL;
  }

  CreateMemBuf(gyroGetProcData.ext_mem_size, m_pGisWorkBuf);
  m_pGisWorkBuf->lockBuf("GisWorkBuf", eBUFFER_USAGE_SW_MASK);
  if (!m_pGisWorkBuf->getBufVA(0)) {
    MY_LOGE("m_pGisWorkBuf create ImageBuffer fail");
    return EIS_RETURN_MEMORY_ERROR;
  }

  MY_LOGD("m_pGisWorkBuf : size(%u),virAdd(0x%p)", gyroGetProcData.ext_mem_size,
          (MVOID*)m_pGisWorkBuf->getBufVA(0));

  gyroSetworkingbuffer.extMemStartAddr = (MVOID*)m_pGisWorkBuf->getBufVA(0);
  gyroSetworkingbuffer.extMemSize = gyroGetProcData.ext_mem_size;

  err = m_pGisAlg->GyroFeatureCtrl(GYRO_FEATURE_SET_WORK_BUF_INFO,
                                   &gyroSetworkingbuffer, NULL);
  if (err != S_GYRO_OK) {
    MY_LOGE("mGisWorkBuf create IMem fail");
    return EIS_RETURN_API_FAIL;
  }

  return EIS_RETURN_NO_ERROR;
}

MINT32 EisHalImp::CreateEISPlusAlgoBuf() {
  MY_TRACE_FUNC_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;
  EIS_PLUS_SET_WORKING_BUFFER_STRUCT eisPlusWorkBufData;

  mEisPlusGetProcData.ext_mem_size = 0;
  mEisPlusGetProcData.Grid_H = 0;
  mEisPlusGetProcData.Grid_W = 0;
  mEisPlusGetProcData.iWidth = mGyroAlgoInitData.rrz_scale_Width;
  mEisPlusGetProcData.iHeight = mGyroAlgoInitData.rrz_scale_Height;

  MY_LOGD("[EIS50 debug] mEisPlusGetProcData (w,h)=(%d,%d)",
          mEisPlusGetProcData.iWidth, mEisPlusGetProcData.iHeight);

  eisPlusWorkBufData.extMemSize = 0;
  eisPlusWorkBufData.extMemStartAddr = NULL;

  //> Preapre EIS Plus Working Buffer

  err = m_pEisPlusAlg->EisPlusFeatureCtrl(EIS_PLUS_FEATURE_GET_PROC_INFO, NULL,
                                          &mEisPlusGetProcData);
  if (err != S_EIS_PLUS_OK) {
    MY_LOGE("EisPlus: EIS_PLUS_FEATURE_GET_PROC_INFO fail(0x%x)", err);
    return EIS_RETURN_API_FAIL;
  }

  MY_LOGD("ext_mem_size(%u)", mEisPlusGetProcData.ext_mem_size);

  CreateMemBuf(mEisPlusGetProcData.ext_mem_size, m_pEisPlusWorkBuf);
  m_pEisPlusWorkBuf->lockBuf("EISPlusWorkBuf", eBUFFER_USAGE_SW_MASK);
  if (!m_pEisPlusWorkBuf->getBufVA(0)) {
    MY_LOGE("m_pEisPlusWorkBuf create ImageBuffer fail");
    return EIS_RETURN_MEMORY_ERROR;
  }

  MY_LOGD("m_pEisPlusWorkBuf : size(%u),virAdd(0x%p)",
          mEisPlusGetProcData.ext_mem_size,
          (MVOID*)m_pEisPlusWorkBuf->getBufVA(0));

  eisPlusWorkBufData.extMemSize = mEisPlusGetProcData.ext_mem_size;
  eisPlusWorkBufData.extMemStartAddr = (MVOID*)m_pEisPlusWorkBuf->getBufVA(0);

  err = m_pEisPlusAlg->EisPlusFeatureCtrl(EIS_PLUS_FEATURE_SET_WORK_BUF_INFO,
                                          &eisPlusWorkBufData, NULL);
  if (err != S_EIS_PLUS_OK) {
    MY_LOGE("EisPlus: EIS_PLUS_FEATURE_SET_WORK_BUF_INFO fail(0x%x)", err);
    return EIS_RETURN_API_FAIL;
  }

  return EIS_RETURN_NO_ERROR;
}

/*******************************************************************************
 *
 ********************************************************************************/
MINT32 EisHalImp::ConfigRSCMEEis(const EIS_HAL_CONFIG_DATA& aEisConfig,
                                 const MUINT32 eisMode,
                                 const FSC_INFO_T* fscInfo) {
  MY_TRACE_API_LIFE();

  if (mEisSupport == MFALSE) {
    MY_LOGD("mSensorIdx(%u) not support EIS", mSensorIdx);
    return EIS_RETURN_NO_ERROR;
  }

  MINT32 err = EIS_RETURN_NO_ERROR;
  mSourceDumpIdx = aEisConfig.sourceDumpIdx;
  mQuickDump = aEisConfig.quickDump;

  if (mIsEisPlusConfig == 0) {
    MY_LOGD("EIS Plus first config");

    mEisMode = eisMode;
    mSensorMode = aEisConfig.sensorMode;
    mStaggerNum = aEisConfig.staggerNum;

    mAlgoEisMode =
        property_get_int32("vendor.debug.algo.eis.mode", mAlgoEisMode);
    mEisPlusAlgoInitData.EIS_mode = mAlgoEisMode;

    /* Configure matrix dimension
     */
    if (EIS_MODE_IS_EIS_22_ENABLED(mEisMode) ||
        (EIS_MODE_IS_EIS_25_ENABLED(mEisMode) &&
         !EIS_MODE_IS_EIS_DEJELLO_ENABLED(mEisMode))) {
      mGpuGridW = 2;
      mGpuGridH = 2;
    } else {
      if (mEisPlusAlgoInitData.EIS_mode == 6) {
        mGpuGridW = 31;
        mGpuGridH = 18;
      } else {
        mGpuGridW = 33;
        mGpuGridH = 19;
      }
    }

    // get EIS Plus customized data
    GetEisPlusCustomize(&mEisPlusAlgoInitData.eis_plus_tuning_data,
                        &aEisConfig);

    if (UNLIKELY(mDebugDump >= 2)) {
      mEisPlusAlgoInitData.debug = MTRUE;
      MY_LOGD("eisPlus debug(%d)", mEisPlusAlgoInitData.debug);
    }

    MY_LOGD(
        "EISMode=(0x%x), EIS Warp Matrix=Grid W(%d), H(%d) "
        "warping_mode(%d),effort(%d) search_range(%d,%d) "
        "crop_ratio(%d),stabilization_strength(%f)",
        eisMode, mGpuGridW, mGpuGridH,
        mEisPlusAlgoInitData.eis_plus_tuning_data.warping_mode,
        mEisPlusAlgoInitData.eis_plus_tuning_data.effort,
        mEisPlusAlgoInitData.eis_plus_tuning_data.search_range_x,
        mEisPlusAlgoInitData.eis_plus_tuning_data.search_range_y,
        mEisPlusAlgoInitData.eis_plus_tuning_data.crop_ratio,
        mEisPlusAlgoInitData.eis_plus_tuning_data.stabilization_strength);

    //> Init Eis plus

    // EIS 3.0
    GetEis30Customize(&mEisPlusAlgoInitData.eis30_tuning_data, &aEisConfig);

    mEisPlusAlgoInitData.MVWidth = mMVWidth;
    mEisPlusAlgoInitData.MVHeight = mMVHeight;

    mEisPlusAlgoInitData.Gvalid = mAccEnable;
    mEisPlusAlgoInitData.GyroValid = mGyroEnable;
    mEisPlusAlgoInitData.EIS_standard_en = aEisConfig.standard_EIS;
    mEisPlusAlgoInitData.EIS_advance_en = aEisConfig.advanced_EIS;
    mEisPlusAlgoInitData.FSC_margin = EISCustom::getFSCMargin();
    mEisPlusAlgoInitData.eis_LDC_data.DC_n = 0;
    mEisPlusAlgoInitData.SensorID = mSensorIdx;
    mEisPlusAlgoInitData.WarpGrid_ActualWidth = aEisConfig.warpmapSize.w;
    mEisPlusAlgoInitData.WarpGrid_ActualHeight = aEisConfig.warpmapSize.h;
    mEisPlusAlgoInitData.WarpGrid_StrideWidth = aEisConfig.warpmapStride.w;
    mEisPlusAlgoInitData.WarpGrid_StrideHeight = aEisConfig.warpmapStride.h;

    /* Set up EIS algo mode
     */
    if (EIS_MODE_IS_EIS_GYRO_ENABLED(mEisMode) &&
        EIS_MODE_IS_EIS_IMAGE_ENABLED(mEisMode)) {
      // Fusion
      mEisPlusAlgoInitData.en_gyro_fusion = 1;
      if (UNLIKELY(mDebugDump > 0)) {
        MY_LOGD("Run EIS 3.0 Fusion");
      }
    } else if (EIS_MODE_IS_EIS_GYRO_ENABLED(mEisMode) &&
               !EIS_MODE_IS_EIS_IMAGE_ENABLED(mEisMode)) {
      // Gyro-based Only
      mEisPlusAlgoInitData.en_gyro_fusion = 1;
      if (UNLIKELY(mDebugDump > 0)) {
        MY_LOGD("Run EIS 3.0 Gyro-based only");
      }
    } else if (EIS_MODE_IS_EIS_IMAGE_ENABLED(mEisMode) &&
               !EIS_MODE_IS_EIS_GYRO_ENABLED(mEisMode)) {
      // Image-based Only
      mEisPlusAlgoInitData.en_gyro_fusion = 0;
      mEisPlusAlgoInitData.GyroValid = 0;
      if (UNLIKELY(mDebugDump > 0)) {
        MY_LOGD("Run EIS 3.0 Image-based only");
      }
    } else {
      // fallthrough
      mEisPlusAlgoInitData.en_gyro_fusion = 0;
      MY_LOGE("Unknown EIS Mode!");
    }

    if (UNLIKELY((mSourceDump || mQuickDump) && mGyroOnly == 1)) {
      mEisPlusAlgoInitData.en_gyro_fusion = 1;
    }

    if (UNLIKELY((mSourceDump || mQuickDump) && mImageOnly == 1)) {
      mGyroOnly = 0;
      mEisPlusAlgoInitData.en_gyro_fusion = 0;
    }

    mEisPlusAlgoInitData.Customer_LDC_Data.CUSTOMER_LDC_TYPE = 0;

    switch (aEisConfig.warp_precision) {
      case 4:
        mEisPlusAlgoInitData.warpPrecision = EIS_PLUS_WARP_PRECISION_4_BITS;
        break;
      case 5:
        mEisPlusAlgoInitData.warpPrecision = EIS_PLUS_WARP_PRECISION_5_BITS;
        break;
      default:
        mEisPlusAlgoInitData.warpPrecision = EIS_PLUS_WARP_PRECISION_5_BITS;
        MY_LOGW(
            "Unsupported warp precision=%d, use default precision=5(idx=%d)",
            aEisConfig.warp_precision, mEisPlusAlgoInitData.warpPrecision);
        break;
    }

    if (fscInfo != NULL) {
      mFSCInfo.numSlices = fscInfo->numSlices;
      mFSCInfo.isEnabled = fscInfo->isEnabled;

      mEisPlusAlgoInitData.FSCSlices = mFSCInfo.numSlices;
      mEisPlusAlgoInitData.FSC_en = mFSCInfo.isEnabled;
    } else {
      mFSCInfo.numSlices = mEisPlusAlgoInitData.FSCSlices = 0;
      mFSCInfo.isEnabled = mEisPlusAlgoInitData.FSC_en = MFALSE;
    }

    if (UNLIKELY(mDebugDump > 0)) {
      MY_LOGD(
          "EIS_MODE=%d,GYRO_VALID=%d,en_gyro_fusion=%d,warpPrecision=%d,FSC_en="
          "%d(slice=%d)",
          mEisPlusAlgoInitData.EIS_mode, mEisPlusAlgoInitData.GyroValid,
          mEisPlusAlgoInitData.en_gyro_fusion,
          mEisPlusAlgoInitData.warpPrecision, mEisPlusAlgoInitData.FSC_en,
          mEisPlusAlgoInitData.FSCSlices);
    }

    ConfigGyroAlgo(aEisConfig, mEisPlusAlgoInitData);

    SensorCropWinInfo& cropInfo = mSensorInfo[mSensorIdx].mSensorCropInfo;

    mEisPlusAlgoInitData.SensorSizeInfo.full_Width = cropInfo.full_w;
    mEisPlusAlgoInitData.SensorSizeInfo.full_Height = cropInfo.full_h;
    mEisPlusAlgoInitData.SensorSizeInfo.cropX1 = cropInfo.x0_offset;
    mEisPlusAlgoInitData.SensorSizeInfo.cropY1 = cropInfo.y0_offset;
    mEisPlusAlgoInitData.SensorSizeInfo.cropped_Width1 = cropInfo.w0_size;
    mEisPlusAlgoInitData.SensorSizeInfo.cropped_Height1 = cropInfo.h0_size;
    mEisPlusAlgoInitData.SensorSizeInfo.resized_Width = cropInfo.scale_w;
    mEisPlusAlgoInitData.SensorSizeInfo.resized_Height = cropInfo.scale_h;
    mEisPlusAlgoInitData.SensorSizeInfo.cropX2 = cropInfo.x1_offset;
    mEisPlusAlgoInitData.SensorSizeInfo.cropY2 = cropInfo.y1_offset;
    mEisPlusAlgoInitData.SensorSizeInfo.cropped_Width2 = cropInfo.w1_size;
    mEisPlusAlgoInitData.SensorSizeInfo.cropped_Height2 = cropInfo.h1_size;
    mEisPlusAlgoInitData.SensorSizeInfo.cropX3 = cropInfo.x2_tg_offset;
    mEisPlusAlgoInitData.SensorSizeInfo.cropY3 = cropInfo.y2_tg_offset;
    mEisPlusAlgoInitData.SensorSizeInfo.cropped_Width3 = cropInfo.w2_tg_size;
    mEisPlusAlgoInitData.SensorSizeInfo.cropped_Height3 = cropInfo.h2_tg_size;
    mEisPlusAlgoInitData.FocalLength = mFocalLength;

    MY_LOGD(
        "[EIS50 debug][%d] full(%dx%d) crop1(%d,%d,%dx%d) resize(%dx%d)"
        " crop2(%d,%d,%dx%d) crop3(%d,%d,%dx%d) FocalLength=%f ",
        mEisPlusAlgoInitData.SensorID, cropInfo.full_w, cropInfo.full_h,
        cropInfo.x0_offset, cropInfo.y0_offset, cropInfo.w0_size,
        cropInfo.h0_size, cropInfo.scale_w, cropInfo.scale_h,
        cropInfo.x1_offset, cropInfo.y1_offset, cropInfo.w1_size,
        cropInfo.h1_size, cropInfo.x2_tg_offset, cropInfo.y2_tg_offset,
        cropInfo.w2_tg_size, cropInfo.h2_tg_size,
        mEisPlusAlgoInitData.FocalLength);

    MBOOL useNVRAMParam = MFALSE;
    if (m_pNvram != NULL) {
      useNVRAMParam = (m_pNvram->gis.gis_deftRerserved1[0] != 0);
    }

    MINT32 record30fps = 0, record60fps = 0, dmb_Extime_th = 0;
    float preview = 0, superPreview = 0, superRecord = 0, dmbGain = 0;

    if (property_get_int32(EIS_CUSTOM_PARAM, 0)) {
      record30fps = property_get_int32("vendor.debug.eis.param.record30fps", 0);
      record60fps = property_get_int32("vendor.debug.eis.param.record60fps", 0);
      dmb_Extime_th = property_get_int32("vendor.debug.eis.param.dmbth", 0);
      char cPreview[PROPERTY_VALUE_MAX] = "";
      char cSuperPreview[PROPERTY_VALUE_MAX] = "";
      char cSuperRecord[PROPERTY_VALUE_MAX] = "";
      char cDMBGain[PROPERTY_VALUE_MAX] = "";
      property_get("vendor.debug.eis.param.preview", cPreview, "0");
      property_get("vendor.debug.eis.param.superpreview", cSuperPreview, "0");
      property_get("vendor.debug.eis.param.superrecord", cSuperRecord, "0");
      property_get("vendor.debug.eis.param.dmbgain", cDMBGain, "0");
      preview = atof(cPreview);
      superPreview = atof(cSuperPreview);
      superRecord = atof(cSuperRecord);
      dmbGain = atof(cDMBGain);
    }

    mEisPlusAlgoInitData.Record_30fps =
        (record30fps != 0) ? record30fps
                           : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[0]
                                           : mEisPlusInitParam.Record_30fps;
    mEisPlusAlgoInitData.Record_60fps =
        (record60fps != 0) ? record60fps
                           : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[1]
                                           : mEisPlusInitParam.Record_60fps;
    mEisPlusAlgoInitData.Preview =
        (preview != 0) ? preview
                       : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[2]
                                       : mEisPlusInitParam.Preview;
    mEisPlusAlgoInitData.SuperEIS_Preview =
        (superPreview != 0)
            ? superPreview
            : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[3]
                            : mEisPlusInitParam.SuperEIS_Preview;
    mEisPlusAlgoInitData.SuperEIS_Record =
        (superRecord != 0) ? superRecord
                           : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[4]
                                           : mEisPlusInitParam.SuperEIS_Record;
    mEisPlusAlgoInitData.DMB_Extime_th =
        (dmb_Extime_th != 0)
            ? dmb_Extime_th
            : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[5]
                            : mEisPlusInitParam.DMB_Extime_th;
    mEisPlusAlgoInitData.DMB_gain =
        (dmbGain != 0) ? dmbGain
                       : useNVRAMParam ? m_pNvram->gis.gis_deftRerserved1[6]
                                       : mEisPlusInitParam.DMB_gain;
    MUINT32 usedParamNum = 7;

    size_t destSize = sizeof(mEisPlusAlgoInitData.NVRAM_TuningParameters);
    size_t srcSize = (sizeof(m_pNvram->gis.gis_deftRerserved1[0]) *
                      (GIS_MAXSUPPORTED_SMODE - usedParamNum)) +
                     sizeof(m_pNvram->gis.gis_deftRerserved2);

    if (destSize >= srcSize) {
      memcpy(&mEisPlusAlgoInitData.NVRAM_TuningParameters[0],
             &m_pNvram->gis.gis_deftRerserved1[usedParamNum],
             sizeof(m_pNvram->gis.gis_deftRerserved1[0]) *
                 (GIS_MAXSUPPORTED_SMODE - usedParamNum));
      memcpy(
          &mEisPlusAlgoInitData
               .NVRAM_TuningParameters[GIS_MAXSUPPORTED_SMODE - usedParamNum],
          &m_pNvram->gis.gis_deftRerserved2[0],
          sizeof(m_pNvram->gis.gis_deftRerserved2));
      MY_LOGD_IF(mDebugDump > 0,
                 "Copy rest of NVRAM param to NVRAM_TuningParameters");
    }

    MY_LOGD_IF(mDebugDump > 0,
               "NVRAM[%d] EIS params: 30fps=%d 60fps=%d Prv=%.2f SupPrv=%.2f "
               "SupRec=%.2f DMBth=%d DMBgain=%.2f",
               useNVRAMParam, mEisPlusAlgoInitData.Record_30fps,
               mEisPlusAlgoInitData.Record_60fps, mEisPlusAlgoInitData.Preview,
               mEisPlusAlgoInitData.SuperEIS_Preview,
               mEisPlusAlgoInitData.SuperEIS_Record,
               mEisPlusAlgoInitData.DMB_Extime_th,
               mEisPlusAlgoInitData.DMB_gain);

    getCalibrationData();

    err = m_pEisPlusAlg->EisPlusInit(&mEisPlusAlgoInitData);
    if (err != S_EIS_PLUS_OK) {
      MY_LOGE("EisPlusInit fail(0x%x)", err);
      return EIS_RETURN_API_FAIL;
    }

    CreateEISPlusAlgoBuf();

    mIsEisPlusConfig = 1;
  }

  if ((mSourceDump || mQuickDump) && aEisConfig.sourceDumpIdx == 1) {
    createSourceDumpFile(aEisConfig);

    BitTrueData data;
    data.eisPlusAlgoInitData = &mEisPlusAlgoInitData;
    data.eisPlusGetProcData = &mEisPlusGetProcData;
    data.gyroInitInfo = &mGyroAlgoInitData;
    dumpBitTrue(data);
  }

  return EIS_RETURN_NO_ERROR;
}

MINT32 EisHalImp::ExecuteGyroAlgo(const EIS_HAL_CONFIG_DATA* apEisConfig,
                                  const MINT64 aTimeStamp,
                                  const MUINT32 aExpTime,
                                  MUINT32 aLExpTime,
                                  GYRO_MV_RESULT_INFO_STRUCT& gyroMVresult,
                                  IMAGE_BASED_DATA* imgBaseData) {
  MY_TRACE_FUNC_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;
  GYRO_SET_PROC_INFO_STRUCT gyroSetProcData;
  MUINT64 gyro_t_frame_array[GYRO_DATA_PER_FRAME];
  MDOUBLE gyro_xyz_frame_array[GYRO_DATA_PER_FRAME * 3];

  memset(&gyroSetProcData, 0, sizeof(gyroSetProcData));

  if (UNLIKELY(mTsForAlgoDebug == 0)) {
    mTsForAlgoDebug = aTimeStamp;
  }

  if (UNLIKELY(mMultiSensor != 0))  // Support Multiple sensor mode
  {
    if (UNLIKELY(mCurSensorIdx != apEisConfig->sensorIdx)) {
      // Get the new sensor info
      mCurSensorIdx = apEisConfig->sensorIdx;
      if (mSensorInfo[mCurSensorIdx].mUsed == MTRUE) {
        CalcSensorTrs(mCurSensorIdx, apEisConfig->sensor_Height);
      } else {
        MY_LOGW("mSensorInfo[%d] is not used", mCurSensorIdx);
        AddMultiSensors(mCurSensorIdx);
        CalcSensorTrs(mCurSensorIdx, apEisConfig->sensor_Height);
      }
    }
  }

  MBOOL gyroEnabled = mpSensorProvider->isEnabled(SENSOR_TYPE_GYRO);

  if ((aTimeStamp != 0) && gyroEnabled) {
    MY_TRACE_TAG_LIFE("GetSensorData");

    MINT32 waitTime = 0;
    EIS_GyroRecord lastGyro;
    lastGyro.ts = 0;
    const MINT64 currentTarget = aTimeStamp + (mSleepTime * 1000L) +
                                 (mSensorInfo[mCurSensorIdx].mtRSTime * 1000L);
    do {
      if (waitTime >= (mEISInterval * 2)) {
        MY_LOGW("Wait gyro timedout! currentTarget= %" PRId64
                " ms, lastGyro= %" PRId64 " ms, systemTime= %" PRId64 " ms",
                currentTarget / 1000000, lastGyro.ts / 1000000, getTimeInMs());
        MY_LOGW(
            "Detect no gyro data coming timeout(%d ms), now force skip waiting",
            waitTime);
        break;
      }

      // Get sensor data from SensorProvider and push to GyroQueue
      GetSensorData();

      MBOOL bGTTarget = MFALSE;
      EISHAL_SwitchGyroQueueLock(mSensorIdx);
      mSkipWaitGyro = MFALSE;
      EISHAL_SwitchGyroQueueBack(mSensorIdx, lastGyro);
      EISHAL_SwitchIsLastGyroTimestampGT(mSensorIdx, currentTarget, bGTTarget);

      if (bGTTarget) {
        MY_LOGD("video (%" PRIi64 ") < global Gyro timestamp => go GIS ",
                currentTarget);
        EISHAL_SwitchGyroQueueUnlock(mSensorIdx);
        break;
      }

      if (mSkipWaitGyro == MTRUE) {
        MY_LOGD("skip wait Gyro: %d by next video trigger", waitTime);
        EISHAL_SwitchGyroQueueUnlock(mSensorIdx);
        break;
      }

      EISHAL_SwitchGyroQueueUnlock(mSensorIdx);
      if (lastGyro.ts >= currentTarget) {
        // Get gyro data successfully
        break;
      }

      waitTime++;

      if (UNLIKELY(mDebugDump >= 1)) {
        if (UNLIKELY(waitTime > 1)) {
          MY_LOGD("wait Gyro time: %d", waitTime);
        }
      }
      usleep(1000);  // 1ms
    } while (1);
  }

  EISHAL_SwitchGyroQueueLock(mSensorIdx);
  EISHAL_SwitchPopGyroQueue(mSensorIdx, gyro_t_frame_array,
                            gyro_xyz_frame_array, gyroSetProcData.gyro_num);
  EISHAL_SwitchGyroQueueUnlock(mSensorIdx);

  if (UNLIKELY(mDebugDump >= 1)) {
    MY_LOGD("Gyro data num: %d, video ts: %" PRIi64 " ",
            gyroSetProcData.gyro_num, aTimeStamp);
  }

  gyroSetProcData.MVWidth = mMVWidth;
  gyroSetProcData.MVHeight = mMVHeight;

  gyroSetProcData.frame_t = aTimeStamp;
  gyroSetProcData.frame_AE = aExpTime;

  // New for EIS 3.0
  gyroSetProcData.cam_idx = mCurSensorIdx;
  gyroSetProcData.sensor_Width = apEisConfig->sensor_Width;
  gyroSetProcData.sensor_Height = apEisConfig->sensor_Height;
  gyroSetProcData.param = mSensorInfo[mCurSensorIdx].mRecordParameter;
  if (mStaggerNum > 1) {
    gyroSetProcData.param[0] = mTRS * mStaggerNum;
  } else {
    gyroSetProcData.param[0] = mTRS;
  }
  gyroSetProcData.vHDR_idx = apEisConfig->vHDREnabled;
  gyroSetProcData.frame_LE = aLExpTime;
  gyroSetProcData.frame_SE = aExpTime;

  gyroSetProcData.gyro_t_frame = gyro_t_frame_array;
  gyroSetProcData.gyro_xyz_frame = gyro_xyz_frame_array;

  gyroSetProcData.rrz_crop_X = apEisConfig->rrz_crop_X;
  gyroSetProcData.rrz_crop_Y = apEisConfig->rrz_crop_Y;

  gyroSetProcData.rrz_crop_Width = apEisConfig->rrz_crop_Width;
  gyroSetProcData.rrz_crop_Height = apEisConfig->rrz_crop_Height;

  gyroSetProcData.rrz_scale_Width = apEisConfig->rrz_scale_Width;
  gyroSetProcData.rrz_scale_Height = apEisConfig->rrz_scale_Height;

  gyroSetProcData.crz_crop_X = apEisConfig->cropX;
  gyroSetProcData.crz_crop_Y = apEisConfig->cropY;

  gyroSetProcData.crz_crop_Width = apEisConfig->crzOutW;
  gyroSetProcData.crz_crop_Height = apEisConfig->crzOutH;

  gyroSetProcData.fov_align_Width = apEisConfig->fov_align_Width;
  gyroSetProcData.fov_align_Height = apEisConfig->fov_align_Height;
  gyroSetProcData.warp_grid[0].x = apEisConfig->warp_grid[0].x;
  gyroSetProcData.warp_grid[1].x = apEisConfig->warp_grid[1].x;
  gyroSetProcData.warp_grid[2].x = apEisConfig->warp_grid[2].x;
  gyroSetProcData.warp_grid[3].x = apEisConfig->warp_grid[3].x;
  gyroSetProcData.warp_grid[0].y = apEisConfig->warp_grid[0].y;
  gyroSetProcData.warp_grid[1].y = apEisConfig->warp_grid[1].y;
  gyroSetProcData.warp_grid[2].y = apEisConfig->warp_grid[2].y;
  gyroSetProcData.warp_grid[3].y = apEisConfig->warp_grid[3].y;

  if (mFSCInfo.isEnabled && imgBaseData != NULL &&
      imgBaseData->fscData != NULL) {
    gyroSetProcData.FSCScalingFactor = imgBaseData->fscData->scalingFactor;
  }

  MINT32 eis_data[EIS_WIN_NUM * 4];
  gyroSetProcData.val_LMV = apEisConfig->lmvDataEnabled;
  if (gyroSetProcData.val_LMV) {
    for (MINT32 i = 0; i < EIS_WIN_NUM; i++) {
      eis_data[4 * i + 0] = apEisConfig->lmv_data->i4LMV_X[i];
      eis_data[4 * i + 1] = apEisConfig->lmv_data->i4LMV_Y[i];
      eis_data[4 * i + 2] = apEisConfig->lmv_data->NewTrust_X[i];
      eis_data[4 * i + 3] = apEisConfig->lmv_data->NewTrust_Y[i];
    }
    gyroSetProcData.EIS_LMV = eis_data;
  }

  if (mOisEnable) {
    size_t oisNum = mOisTimestamp.size();
    for (int i = 0; i < oisNum; i++) {
      mOisX[i] = EISCustom::getOISPixelVal(mSensorIdx, mOisX[i]);
      mOisX[i] = EISCustom::getOISPixelVal(mSensorIdx, mOisX[i]);
    }
    gyroSetProcData.ois_t_frame = &mOisTimestamp[0];
    gyroSetProcData.ois_x_frame = &mOisX[0];
    gyroSetProcData.ois_y_frame = &mOisY[0];
    gyroSetProcData.ois_num = oisNum;
    if (mDebugDump > 0) {
      MY_LOGD("Got %zu OIS data", oisNum);
      for (MINT32 i = 0; i < oisNum; i++) {
        MY_LOGD("OIS data: [%d] TS(%" PRId64
                ") (hall_x,hall_y)=(%f,%f) (x,y)=(%f,%f)",
                i, mOisTimestamp[i], mOisHallX[i], mOisHallY[i], mOisX[i],
                mOisY[i]);
      }
    }
    if ((mSourceDump || mQuickDump)) {
      BitTrueData data;
      data.isOIS = true;
      dumpBitTrue(data);
    }
  } else {
    OISInfo_T rOISInfo;
    MINT32 oisDataNum = 0;
    m_p3aHal->send3ACtrl(E3ACtrl_GetOISPos, (MINTPTR)&rOISInfo, 0);
    // For int -> double
    MDOUBLE oisX[OIS_DATA_NUM];
    MDOUBLE oisY[OIS_DATA_NUM];
    for (int i = 0; i < OIS_DATA_NUM; i++) {
      oisX[i] = rOISInfo.i4OISHallPosX[i];
      oisY[i] = rOISInfo.i4OISHallPosY[i];
      if (rOISInfo.TimeStamp[i] == 0) {
        break;
      }
      oisDataNum++;
    }

    gyroSetProcData.ois_t_frame = (MUINT64*)rOISInfo.TimeStamp;
    gyroSetProcData.ois_x_frame = oisX;
    gyroSetProcData.ois_y_frame = oisY;
    gyroSetProcData.ois_num = oisDataNum;

    if (mDebugDump > 0) {
      for (MINT32 i = 0; i < oisDataNum; i++) {
        MY_LOGD("OIS data: [%d] TS(%" PRId64 ") (%f,%f)", i,
                rOISInfo.TimeStamp[i], oisX[i], oisY[i]);
      }
    }

    if ((mSourceDump || mQuickDump) && mpOISFp != NULL) {
      if (fwrite(&rOISInfo, sizeof(rOISInfo), 1, mpOISFp) == 0)
        MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }

  if ((mSourceDump || mQuickDump) && mpAccFp != NULL && (mSourceDumpIdx != 0)) {
    size_t accSize = mAccTimestamp.size();
    if (mDebugDump >= 1) {
      MY_LOGD("Write %zu ACCELERATION data", accSize);
    }
    if (fwrite(&accSize, sizeof(accSize), 1, mpAccFp) == 0 ||
        fwrite(&mAccTimestamp[0], sizeof(MUINT64) * mAccTimestamp.size(), 1,
               mpAccFp) == 0 ||
        fwrite(&mAccData[0], sizeof(MDOUBLE) * mAccData.size(), 1, mpAccFp) ==
            0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }
  mAccTimestamp.clear();
  mAccData.clear();
  gyroSetProcData.acc_num = 0;
  gyroSetProcData.mag_num = 0;
  gyroSetProcData.grv_num = 0;

  if (UNLIKELY(mDebugDump >= 1)) {
    MY_LOGD("crz offset x(%d),y(%d)", gyroSetProcData.crz_crop_X,
            gyroSetProcData.crz_crop_Y);
    MY_LOGD("crzOut w(%d),h(%d)", gyroSetProcData.crz_crop_Width,
            gyroSetProcData.crz_crop_Height);
    MY_LOGD(
        "cam_idx(%d), sensor_Width(%d), sensor_Height(%d), vHDR_idx(%d), "
        "SE(%d), LE(%d)",
        gyroSetProcData.cam_idx, gyroSetProcData.sensor_Width,
        gyroSetProcData.sensor_Height, gyroSetProcData.vHDR_idx,
        gyroSetProcData.frame_SE, gyroSetProcData.frame_LE);
    MY_LOGD("fov_align_Width(%d), fov_align_Height(%d)",
            gyroSetProcData.fov_align_Width, gyroSetProcData.fov_align_Height);
    MY_LOGD(
        "warp_gridX[0]=%f, warp_gridX[1]=%f, warp_gridX[2]=%f, "
        "warp_gridX[3]=%f",
        gyroSetProcData.warp_grid[0].x, gyroSetProcData.warp_grid[1].x,
        gyroSetProcData.warp_grid[2].x, gyroSetProcData.warp_grid[3].x);
    MY_LOGD(
        "warp_gridY[0]=%f, warp_gridY[1]=%f, warp_gridY[2]=%f, "
        "warp_gridY[3]=%f",
        gyroSetProcData.warp_grid[0].y, gyroSetProcData.warp_grid[1].y,
        gyroSetProcData.warp_grid[2].y, gyroSetProcData.warp_grid[3].y);
    MY_LOGD("MVWidth=%d, MVHeight=%d", gyroSetProcData.MVWidth,
            gyroSetProcData.MVHeight);
  }
  if (UNLIKELY(mDebugDump >= 2)) {
    MY_LOGD(
        "gyroSetProcData.param[0]=%f, gyroSetProcData.param[1]=%f, "
        "gyroSetProcData.param[2]=%f",
        gyroSetProcData.param[0], gyroSetProcData.param[1],
        gyroSetProcData.param[2]);
    MY_LOGD(
        "gyroSetProcData.param[3]=%f, gyroSetProcData.param[4]=%f, "
        "gyroSetProcData.param[5]=%f",
        gyroSetProcData.param[3], gyroSetProcData.param[4],
        gyroSetProcData.param[5]);
    if (mFSCInfo.isEnabled) {
      MUINT32* ptr = (MUINT32*)gyroSetProcData.FSCScalingFactor;
      MY_LOGD("FSCFactor(%p)", gyroSetProcData.FSCScalingFactor);
      for (MINT32 i = 0; i < mFSCInfo.numSlices; ++i) {
        MY_LOGD("FSC[%d]=%d", i, ptr[i]);
      }
    }
  }
  if (mDisableGyroData) {
    memset(gyroSetProcData.gyro_xyz_frame, 0, sizeof(gyro_xyz_frame_array));
  }
  //====== GIS Algorithm ======

  err = m_pGisAlg->GyroFeatureCtrl(GYRO_FEATURE_SET_PROC_INFO, &gyroSetProcData,
                                   NULL);
  if (err != S_GYRO_OK) {
    MY_LOGE("GIS:GYRO_FEATURE_SET_PROC_INFO fail(0x%x)", err);
    err = EIS_RETURN_API_FAIL;
    return err;
  }

  err = m_pGisAlg->GyroMain();
  if (err != S_GYRO_OK) {
    MY_LOGE("GIS:GyroMain fail(0x%x)", err);
    err = EIS_RETURN_API_FAIL;
    return err;
  }

  err = m_pGisAlg->GyroFeatureCtrl(GYRO_FEATURE_GET_MV_RESULT_INFO, NULL,
                                   &gyroMVresult);
  if (err != S_GYRO_OK) {
    MY_LOGE("GIS:GYRO_FEATURE_SET_PROC_INFO fail(0x%x)", err);
    err = EIS_RETURN_API_FAIL;
    return err;
  }

  if (mSourceDump || mQuickDump) {
    BitTrueData data;
    data.gyroProcData = &gyroSetProcData;
    data.gyroResult = &gyroMVresult;
    dumpBitTrue(data);
  }

  return EIS_RETURN_NO_ERROR;
}

MINT32 EisHalImp::DoRSCMEEis(EIS_HAL_CONFIG_DATA* apEisConfig,
                             IMAGE_BASED_DATA* imgBaseData,
                             MINT64 aTimeStamp,
                             MUINT32 aExpTime,
                             MUINT32 aLExpTime,
                             MUINT32 staggerNum,
                             EIS_HAL_OUT_DATA& outData) {
  MY_TRACE_API_LIFE();

  MINT32 err = EIS_RETURN_NO_ERROR;
  MUINT32 i;
  mStaggerNum = staggerNum;

  if (UNLIKELY(mEisSupport == MFALSE)) {
    MY_LOGD("mSensorIdx(%u) not support EIS", mSensorIdx);
    return EIS_RETURN_NO_ERROR;
  }

  //====== Check Config Data ======
  if (apEisConfig == NULL) {
    MY_LOGE("apEisConfig is NULL");
    err = EIS_RETURN_NULL_OBJ;
    return err;
  }

  //====== Setting EIS Plus Algo Process Data ======

  EIS_PLUS_SET_PROC_INFO_STRUCT eisPlusProcData;
  memset(&eisPlusProcData, 0, sizeof(eisPlusProcData));

  apEisConfig->lmvDataEnabled =
      (imgBaseData != NULL) ? imgBaseData->lmvData->enabled : MFALSE;
  if (apEisConfig->lmvDataEnabled) {
    apEisConfig->lmv_data = &imgBaseData->lmvData->data;
  }

  //> Set EisPlusProcData
  if (m_pLMVHal->GetLMVStatus())  // calibration
  {
    eisPlusProcData.eis_info.eis_gmv_conf[0] = apEisConfig->confX;
    eisPlusProcData.eis_info.eis_gmv_conf[1] = apEisConfig->confY;
    eisPlusProcData.eis_info.eis_gmv[0] = apEisConfig->gmv_X;
    eisPlusProcData.eis_info.eis_gmv[1] = apEisConfig->gmv_Y;
  } else {
    // Set EIS algo to NEVER use GMV
  }

  eisPlusProcData.imgiWidth = apEisConfig->imgiW;
  eisPlusProcData.imgiHeight = apEisConfig->imgiH;
  eisPlusProcData.CRZoWidth = apEisConfig->crzOutW;
  eisPlusProcData.CRZoHeight = apEisConfig->crzOutH;
  eisPlusProcData.SRZoWidth = apEisConfig->srzOutW;
  eisPlusProcData.SRZoHeight = apEisConfig->srzOutH;
  eisPlusProcData.oWidth = apEisConfig->feTargetW;
  eisPlusProcData.oHeight = apEisConfig->feTargetH;
  eisPlusProcData.TargetWidth = apEisConfig->gpuTargetW;
  eisPlusProcData.TargetHeight = apEisConfig->gpuTargetH;
  eisPlusProcData.cropX = apEisConfig->cropX;
  eisPlusProcData.cropY = apEisConfig->cropY;
  eisPlusProcData.MVWidth = mMVWidth;
  eisPlusProcData.MVHeight = mMVHeight;

  // New for EIS 3.0
  eisPlusProcData.RSSoWidth = apEisConfig->rssoWidth;
  eisPlusProcData.RSSoHeight = apEisConfig->rssoHeight - 1;
  eisPlusProcData.RSCLevel = 6;
  eisPlusProcData.mv_idx = mCurSensorIdx;
  eisPlusProcData.Trs = mSensorInfo[mCurSensorIdx].mRecordParameter[0];
  eisPlusProcData.FovAlignWidth = apEisConfig->fov_align_Width;
  eisPlusProcData.FovAlignHeight = apEisConfig->fov_align_Height;
  eisPlusProcData.WarpGrid[0].x = apEisConfig->warp_grid[0].x;
  eisPlusProcData.WarpGrid[1].x = apEisConfig->warp_grid[1].x;
  eisPlusProcData.WarpGrid[2].x = apEisConfig->warp_grid[2].x;
  eisPlusProcData.WarpGrid[3].x = apEisConfig->warp_grid[3].x;
  eisPlusProcData.WarpGrid[0].y = apEisConfig->warp_grid[0].y;
  eisPlusProcData.WarpGrid[1].y = apEisConfig->warp_grid[1].y;
  eisPlusProcData.WarpGrid[2].y = apEisConfig->warp_grid[2].y;
  eisPlusProcData.WarpGrid[3].y = apEisConfig->warp_grid[3].y;

  if (mFSCInfo.isEnabled && imgBaseData != NULL &&
      imgBaseData->fscData != NULL) {
    eisPlusProcData.FSCProcWidth = imgBaseData->fscData->procWidth;
    eisPlusProcData.FSCProcHeight = imgBaseData->fscData->procHeight;
    eisPlusProcData.FSCScalingFactor = imgBaseData->fscData->scalingFactor;
  }

  //> config EIS Plus data

  mSrzOutW = apEisConfig->srzOutW;
  mSrzOutH = apEisConfig->srzOutH;

  eisPlusProcData.frame_t = (MUINT64)aTimeStamp;
  eisPlusProcData.ShutterTime = aExpTime;
  eisPlusProcData.process_idx = apEisConfig->process_idx;
  eisPlusProcData.process_mode = apEisConfig->process_mode;
  eisPlusProcData.AE_data.ISO = apEisConfig->iso;
  eisPlusProcData.AE_data.exposure_time = aExpTime;
  eisPlusProcData.AE_data.gain = apEisConfig->gain;
  eisPlusProcData.frame_rate = apEisConfig->fps;
  eisPlusProcData.IntraFrameAngleHeight = RowSliceNum;

  if (UNLIKELY(mDebugDump >= 1)) {
    MY_LOGD("eisPlusProcData");
    MY_LOGD("algMode(%d), algCounter(%d)", eisPlusProcData.process_mode,
            eisPlusProcData.process_idx);
    MY_LOGD("eis_gmv_conf[0](%d)", eisPlusProcData.eis_info.eis_gmv_conf[0]);
    MY_LOGD("eis_gmv_conf[1](%d)", eisPlusProcData.eis_info.eis_gmv_conf[1]);
    MY_LOGD("eis_gmv[0](%f)", eisPlusProcData.eis_info.eis_gmv[0]);
    MY_LOGD("eis_gmv[1](%f)", eisPlusProcData.eis_info.eis_gmv[1]);
    MY_LOGD("block_size(%u)", (MUINT32)eisPlusProcData.block_size);
    MY_LOGD("imgi(%d,%d)", eisPlusProcData.imgiWidth,
            eisPlusProcData.imgiHeight);
    MY_LOGD("CRZ(%d,%d)", eisPlusProcData.CRZoWidth,
            eisPlusProcData.CRZoHeight);
    MY_LOGD("SRZ(%d,%d)", eisPlusProcData.SRZoWidth,
            eisPlusProcData.SRZoHeight);
    MY_LOGD("FeTarget(%u,%u)", eisPlusProcData.oWidth, eisPlusProcData.oHeight);
    MY_LOGD("target(%d,%d)", eisPlusProcData.TargetWidth,
            eisPlusProcData.TargetHeight);
    MY_LOGD("crop(%d,%d)", eisPlusProcData.cropX, eisPlusProcData.cropY);
    MY_LOGD("RSSoWidth(%d), RSSoHeight(%d), mv_idx(%d), Trs(%f)",
            eisPlusProcData.RSSoWidth, eisPlusProcData.RSSoHeight,
            eisPlusProcData.mv_idx, eisPlusProcData.Trs);
    MY_LOGD("FovAlignWidth(%d), FovAlignHeight(%d)",
            eisPlusProcData.FovAlignWidth, eisPlusProcData.FovAlignHeight);
    MY_LOGD(
        "WarpGridX[0]=%f, WarpGridX[1]=%f, WarpGridX[2]=%f, WarpGridX[3]=%f",
        eisPlusProcData.WarpGrid[0].x, eisPlusProcData.WarpGrid[1].x,
        eisPlusProcData.WarpGrid[2].x, eisPlusProcData.WarpGrid[3].x);
    MY_LOGD(
        "WarpGridY[0]=%f, WarpGridY[1]=%f, WarpGridY[2]=%f, WarpGridY[3]=%f",
        eisPlusProcData.WarpGrid[0].y, eisPlusProcData.WarpGrid[1].y,
        eisPlusProcData.WarpGrid[2].y, eisPlusProcData.WarpGrid[3].y);
    MY_LOGD("FSCProc(%dx%d)Factor@(%p)", eisPlusProcData.FSCProcWidth,
            eisPlusProcData.FSCProcHeight, eisPlusProcData.FSCScalingFactor);
    MY_LOGD("IntraFrameAngleHeight(%d)", eisPlusProcData.IntraFrameAngleHeight);
  }

  GYRO_MV_RESULT_INFO_STRUCT gyroMVresult;
  ExecuteGyroAlgo(apEisConfig, aTimeStamp, aExpTime, aLExpTime, gyroMVresult,
                  imgBaseData);

  SensorData sensorData;
  if (mpSensorProvider->getLatestSensorData(SENSOR_TYPE_GYRO, sensorData)) {
    for (i = 0; i < 3; i++) {
      // eisPlusProcData.sensor_info.AcceInfo[i] = sensorData.acceleration[i];
      eisPlusProcData.sensor_info.GyroInfo[i] = sensorData.gyro[i];
    }
  }

  eisPlusProcData.sensor_info.gyro_in_mv = gyroMVresult.mv;
  eisPlusProcData.sensor_info.valid_gyro_num = gyroMVresult.valid_gyro_num;
  eisPlusProcData.InterAngle = gyroMVresult.inter_frame_angle;
  eisPlusProcData.IntraAngle = gyroMVresult.intra_frame_angle;
  eisPlusProcData.InterFrameAngleWidth = 3;
  eisPlusProcData.InterFrameAngleHeight = 1;
  eisPlusProcData.IntraFrameAngleWidth = 3;
  eisPlusProcData.IntraFrameAngleHeight = RowSliceNum;
  // eisPlusProcData.ois = gyroMVresult.ois_mv;
  eisPlusProcData.OIS_positionX = gyroMVresult.OIS_positionX;
  eisPlusProcData.OIS_positionY = gyroMVresult.OIS_positionY;

  SensorCropWinInfo& cropInfo = mSensorInfo[mSensorIdx].mSensorCropInfo;

  eisPlusProcData.SensorSizeInfo.full_Width = cropInfo.full_w;
  eisPlusProcData.SensorSizeInfo.full_Height = cropInfo.full_h;
  eisPlusProcData.SensorSizeInfo.cropX1 = cropInfo.x0_offset;
  eisPlusProcData.SensorSizeInfo.cropY1 = cropInfo.y0_offset;
  eisPlusProcData.SensorSizeInfo.cropped_Width1 = cropInfo.w0_size;
  eisPlusProcData.SensorSizeInfo.cropped_Height1 = cropInfo.h0_size;
  eisPlusProcData.SensorSizeInfo.resized_Width = cropInfo.scale_w;
  eisPlusProcData.SensorSizeInfo.resized_Height = cropInfo.scale_h;
  eisPlusProcData.SensorSizeInfo.cropX2 = cropInfo.x1_offset;
  eisPlusProcData.SensorSizeInfo.cropY2 = cropInfo.y1_offset;
  eisPlusProcData.SensorSizeInfo.cropped_Width2 = cropInfo.w1_size;
  eisPlusProcData.SensorSizeInfo.cropped_Height2 = cropInfo.h1_size;
  eisPlusProcData.SensorSizeInfo.cropX3 = cropInfo.x2_tg_offset;
  eisPlusProcData.SensorSizeInfo.cropY3 = cropInfo.y2_tg_offset;
  eisPlusProcData.SensorSizeInfo.cropped_Width3 = cropInfo.w2_tg_size;
  eisPlusProcData.SensorSizeInfo.cropped_Height3 = cropInfo.h2_tg_size;

  eisPlusProcData.SensorSizeInfo.P1_CropX = apEisConfig->rrz_crop_X;
  eisPlusProcData.SensorSizeInfo.P1_CropY = apEisConfig->rrz_crop_Y;
  eisPlusProcData.SensorSizeInfo.P1_CropWidth = apEisConfig->rrz_crop_Width;
  eisPlusProcData.SensorSizeInfo.P1_CropHeight = apEisConfig->rrz_crop_Height;
  eisPlusProcData.SensorSizeInfo.P1_ResizedWidth = apEisConfig->rrz_scale_Width;
  eisPlusProcData.SensorSizeInfo.P1_ResizedHeight =
      apEisConfig->rrz_scale_Height;

  //> config EIS 3.0 RSCME relative configurations
  if (imgBaseData != NULL) {
    eisPlusProcData.sensor_info.fbuf_in_rsc_mv = imgBaseData->rscData->RSCME_mv;
    eisPlusProcData.sensor_info.fbuf_in_rsc_var =
        imgBaseData->rscData->RSCME_var;
  }

  if (UNLIKELY(mDebugDump >= 1)) {
    MY_LOGD("EN:(Acc,Gyro)=(%d,%d)", mAccEnable, mGyroEnable);
    MY_LOGD("EISPlus:Acc(%f,%f,%f)", eisPlusProcData.sensor_info.AcceInfo[0],
            eisPlusProcData.sensor_info.AcceInfo[1],
            eisPlusProcData.sensor_info.AcceInfo[2]);
    MY_LOGD("EISPlus:Gyro(%f,%f,%f)", eisPlusProcData.sensor_info.GyroInfo[0],
            eisPlusProcData.sensor_info.GyroInfo[1],
            eisPlusProcData.sensor_info.GyroInfo[2]);
    MY_LOGD(
        "[EIS50 debug] full(%dx%d) crop1(%d,%d,%dx%d) resize(%dx%d)"
        " crop2(%d,%d,%dx%d) crop3(%d,%d,%dx%d)"
        " P1_Crop(%d,%d,%dx%d) P1_Resize(%dx%d)",
        cropInfo.full_w, cropInfo.full_h, cropInfo.x0_offset,
        cropInfo.y0_offset, cropInfo.w0_size, cropInfo.h0_size,
        cropInfo.scale_w, cropInfo.scale_h, cropInfo.x1_offset,
        cropInfo.y1_offset, cropInfo.w1_size, cropInfo.h1_size,
        cropInfo.x2_tg_offset, cropInfo.y2_tg_offset, cropInfo.w2_tg_size,
        cropInfo.h2_tg_size, apEisConfig->rrz_crop_X, apEisConfig->rrz_crop_Y,
        apEisConfig->rrz_crop_Width, apEisConfig->rrz_crop_Height,
        apEisConfig->rrz_scale_Width, apEisConfig->rrz_scale_Height);
  }

  //====== EIS Plus Algorithm ======

  err = m_pEisPlusAlg->EisPlusFeatureCtrl(EIS_PLUS_FEATURE_SET_PROC_INFO,
                                          &eisPlusProcData, NULL);
  if (err != S_EIS_PLUS_OK) {
    MY_LOGE("EisPlus:EIS_PLUS_FEATURE_SET_PROC_INFO fail(0x%x)", err);
    err = EIS_RETURN_API_FAIL;
    return err;
  }

  err = m_pEisPlusAlg->EisPlusMain(&mEisPlusResult);
  if (err != S_EIS_PLUS_OK) {
    MY_LOGE("EisPlus:EisMain fail(0x%x)", err);
    err = EIS_RETURN_API_FAIL;
    return err;
  }

  if (mCentralCrop) {
    MUINT32 t, s;
    MUINT32 iWidth = apEisConfig->imgiW;
    MUINT32 iHeight = apEisConfig->imgiH;
    MUINT32 oWidth = apEisConfig->gpuTargetW;
    MUINT32 oHeight = apEisConfig->gpuTargetH;
    for (t = 0; t < mGpuGridH; t++) {
      for (s = 0; s < mGpuGridW; s++) {
        float indx_x = static_cast<float>((oWidth - 1) * s / (mGpuGridW - 1)) +
                       (iWidth - oWidth) / 2;
        float indx_y = static_cast<float>((oHeight - 1) * t / (mGpuGridH - 1)) +
                       (iHeight - oHeight) / 2;
        mEisPlusResult.GridX[t * mGpuGridW + s] = static_cast<int>(indx_x * 32);
        mEisPlusResult.GridY[t * mGpuGridW + s] = static_cast<int>(indx_y * 32);
        mEisPlusResult.GridX_standard[t * mGpuGridW + s] =
            static_cast<int>(indx_x * 32);
        mEisPlusResult.GridY_standard[t * mGpuGridW + s] =
            static_cast<int>(indx_y * 32);
      }
    }
    MY_LOGI("Force central crop warpmap");
  }

  // if (UNLIKELY(mDebugDump >= 1))
  {
    MY_LOGD("EisPlusMain- X: %d  Y: %d\n", mEisPlusResult.ClipX,
            mEisPlusResult.ClipY);
  }

  //====== Dynamic Debug ======
#if LEGACY_SOURCEDUMP
  {
    MUINT32 t, s;
    MUINT32 oWidth = apEisConfig->imgiW;
    MUINT32 oHeight = apEisConfig->imgiH;
    for (t = 0; t < mGpuGridH; t++) {
      for (s = 0; s < mGpuGridW; s++) {
        float indx_x = static_cast<float>(oWidth - 1) * s / (mGpuGridW - 1);
        float indx_y = static_cast<float>(oHeight - 1) * t / (mGpuGridH - 1);
        mEisPlusResult.GridX[t * mGpuGridW + s] = static_cast<int>(indx_x * 32);
        mEisPlusResult.GridY[t * mGpuGridW + s] = static_cast<int>(indx_y * 32);
      }
    }
  }
#endif

  if (mSourceDump || mQuickDump) {
    BitTrueData data;
    data.eisResult = &mEisPlusResult;
    data.eisProcInfo = &eisPlusProcData;
    if (imgBaseData != NULL) {
      data.rscData = imgBaseData->rscData;
      data.fscData = imgBaseData->fscData;
    }
    data.requestNo = apEisConfig->requestNo;
    dumpBitTrue(data);
  }

  if (UNLIKELY(mDebugDump >= 3)) {
    MY_LOGD("EIS WARP MAP");
    for (MUINT32 i = 0; i < mGpuGridW * mGpuGridH; ++i) {
      MY_LOGD("X[%u]=%d", i, mEisPlusResult.GridX[i]);
      MY_LOGD("Y[%u]=%d", i, mEisPlusResult.GridY[i]);
    }
    if (mFSCInfo.isEnabled) {
      MUINT32* ptr = (MUINT32*)eisPlusProcData.FSCScalingFactor;
      for (MINT32 i = 0; i < mFSCInfo.numSlices; ++i) {
        MY_LOGD("FSC[%d]=%d", i, ptr[i]);
      }
    }
  }

  if (mEisPlusAlgoInitData.EIS_standard_en &&
      (mEisPlusResult.GridX_standard != NULL) &&
      (mEisPlusResult.GridY_standard != NULL)) {
    MINT32 xCenter = ((mEisPlusResult.GridX_standard[0]+
                       mEisPlusResult.GridX_standard[mGpuGridW - 1]+
                       mEisPlusResult.GridX_standard[mGpuGridW * (mGpuGridH - 1)]+
                       mEisPlusResult.GridX_standard[(mGpuGridW * mGpuGridH) - 1]) / 4) / 32;
    MINT32 yCenter = ((mEisPlusResult.GridY_standard[0]+
                       mEisPlusResult.GridY_standard[mGpuGridW - 1]+
                       mEisPlusResult.GridY_standard[mGpuGridW * (mGpuGridH - 1)]+
                       mEisPlusResult.GridY_standard[(mGpuGridW * mGpuGridH) - 1]) / 4) / 32;
    outData.xOffset = xCenter - (apEisConfig->imgiW / 2);
    outData.yOffset = yCenter - (apEisConfig->imgiH / 2);
    outData.leftTopX = mEisPlusResult.GridX_standard[0];
    outData.leftTopY = mEisPlusResult.GridY_standard[0];
    outData.rightDownX = mEisPlusResult.GridX_standard[(mGpuGridW * mGpuGridH) - 1];
    outData.rightDownY = mEisPlusResult.GridY_standard[(mGpuGridW * mGpuGridH) - 1];
    MY_LOGD_IF(mDebugDump, "[EIS FD] xOffset=%d yOffset=%d leftTopX=%d leftTopY=%d rightDownX=%d rightDownY=%d",
        outData.xOffset, outData.yOffset, outData.leftTopX, outData.leftTopY,
        outData.rightDownX, outData.rightDownY);
  }

  mOisTimestamp.clear();
  mOisX.clear();
  mOisY.clear();
  mOisHallX.clear();
  mOisHallY.clear();

  if (mDebugDump >= 1) {
    MY_LOGD("-");
  }

  return EIS_RETURN_NO_ERROR;
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID EisHalImp::SetEisPlusWarpInfo(MINT32* const aGridX,
                                    MINT32* const aGridY,
                                    MINT32* const aGridX_standard,
                                    MINT32* const aGridY_standard) {
  mEisPlusResult.GridX = aGridX;
  mEisPlusResult.GridY = aGridY;
  mEisPlusResult.GridX_standard = aGridX_standard;
  mEisPlusResult.GridY_standard = aGridY_standard;

  if (mDebugDump >= 1) {
    MY_LOGD("[IN]grid VA(0x%p,0x%p), grid_standard VA(0x%p,0x%p)",
            (MVOID*)aGridX, (MVOID*)aGridY, (MVOID*)aGridX_standard,
            (MVOID*)aGridY_standard);
    MY_LOGD("[MEMBER]grid VA(0x%p,0x%p), , grid_standard VA(0x%p,0x%p)",
            (MVOID*)mEisPlusResult.GridX, (MVOID*)mEisPlusResult.GridY,
            (MVOID*)mEisPlusResult.GridX_standard,
            (MVOID*)mEisPlusResult.GridY_standard);
  }
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID EisHalImp::GetEisPlusCustomize(EIS_PLUS_TUNING_PARA_STRUCT* a_pTuningData,
                                     const EIS_HAL_CONFIG_DATA* apEisConfig) {
  MY_TRACE_FUNC_LIFE();

  configMVNumber(*apEisConfig, &mMVWidth, &mMVHeight);

  EIS_PLUS_Customize_Para_t customSetting;
  EISCustom::getEISPlusData(&customSetting, mEisInfo.videoConfig);

  a_pTuningData->warping_mode = static_cast<MINT32>(customSetting.warping_mode);
  a_pTuningData->effort = 2;  // limit to 400 points
  a_pTuningData->search_range_x = customSetting.search_range_x;
  a_pTuningData->search_range_y = customSetting.search_range_y;
  a_pTuningData->crop_ratio = customSetting.crop_ratio;
  a_pTuningData->gyro_still_time_th = customSetting.gyro_still_time_th;
  a_pTuningData->gyro_max_time_th = customSetting.gyro_max_time_th;
  a_pTuningData->gyro_similar_th = customSetting.gyro_similar_th;
  a_pTuningData->stabilization_strength = customSetting.stabilization_strength;
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID EisHalImp::GetEis30Customize(EIS30_TUNING_PARA_STRUCT* a_pTuningData,
                                   const EIS_HAL_CONFIG_DATA* apEisConfig) {
  MY_TRACE_FUNC_LIFE();

  (void)apEisConfig;

  EIS30_Customize_Tuning_Para_t customSetting;

  EISCustom::getEIS30Data(&customSetting);

  a_pTuningData->stabilization_strength = customSetting.stabilization_strength;
  a_pTuningData->stabilization_level = customSetting.stabilization_level;
  a_pTuningData->gyro_still_mv_th = customSetting.gyro_still_mv_th;
  a_pTuningData->gyro_still_mv_diff_th = customSetting.gyro_still_mv_diff_th;
}

MVOID EisHalImp::dumpBitTrue(const BitTrueData& data) {
  MUINT32 type = 0, writeSize = 0;
  if (mpSourceDumpFp == NULL || mSourceDumpIdx == 0) {
    MY_LOGD("Returned, mpSourceDumpFp = %p, SourceDumpIdx = %d", mpSourceDumpFp,
            mSourceDumpIdx);
    return;
  }

  // dump warpmap
  if (data.eisResult != NULL) {
    type = BitTrueData::TYPE_WARPMAP;
    writeSize = sizeof(MINT32) * mGpuGridH * mGpuGridW * 2;
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(data.eisResult->GridX, sizeof(MINT32) * mGpuGridH * mGpuGridW, 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.eisResult->GridY, sizeof(MINT32) * mGpuGridH * mGpuGridW, 1,
               mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }

    if (!mSizeInfoLog[BitTrueData::TYPE_WARPMAP]) {
      MY_LOGD_IF(mDebugDump, "WARPMAP%zu",
                  sizeof(MINT32) * mGpuGridH * mGpuGridW);
      mSizeInfoLog[BitTrueData::TYPE_WARPMAP] = 1;
    }
  }

  // dump eisPlusProcInfo
  if (data.eisProcInfo != NULL) {
    type = BitTrueData::TYPE_EISPROC;
    MINT32 gyroMVSize =
        (mMVWidth * mMVHeight * sizeof(MUINT8)) + (mMVHeight * sizeof(MUINT8));
    writeSize =
        sizeof(MUINT64) + sizeof(EIS_PLUS_SET_PROC_INFO_STRUCT) + gyroMVSize;
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(&data.eisProcInfo->frame_t, sizeof(MUINT64), 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.eisProcInfo, sizeof(EIS_PLUS_SET_PROC_INFO_STRUCT), 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.eisProcInfo->sensor_info.gyro_in_mv,
               mMVWidth * mMVHeight * sizeof(MUINT8), 1, mpSourceDumpFp) == 0 ||
        fwrite(data.eisProcInfo->sensor_info.valid_gyro_num,
               mMVHeight * sizeof(MUINT8), 1, mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }

    if (!mSizeInfoLog[BitTrueData::TYPE_EISPROC]) {
      MY_LOGD_IF(mDebugDump, "EIS_PLUS_SET_PROC_INFO_STRUCT%zu",
              sizeof(EIS_PLUS_SET_PROC_INFO_STRUCT));
      MY_LOGD_IF(mDebugDump, "gyro_in_mv%zu",
              mMVWidth * mMVHeight * sizeof(MUINT8));
      MY_LOGD_IF(mDebugDump, "valid_gyro_num%zu", mMVHeight * sizeof(MUINT8));
      mSizeInfoLog[BitTrueData::TYPE_EISPROC] = 1;
    }
  }

  // dump RSCME buffer
  if (data.rscData != NULL && data.rscData->RSCME_mv != NULL &&
      data.rscData->RSCME_var != NULL && data.eisProcInfo != NULL) {
    type = BitTrueData::TYPE_RSC;
    MUINT32 meMvWidth = (data.eisProcInfo->RSSoWidth + 1) / 2;
    MUINT32 meMvHeight = (data.eisProcInfo->RSSoHeight + 1) / 2;
    writeSize = sizeof(MUINT8) * (((meMvWidth + 6) / 7) * 16) * meMvHeight +
                sizeof(MUINT8) * meMvWidth * meMvHeight;
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(data.rscData->RSCME_mv,
               sizeof(MUINT8) * (((meMvWidth + 6) / 7) * 16) * meMvHeight, 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.rscData->RSCME_var, sizeof(MUINT8) * meMvWidth * meMvHeight,
               1, mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
    if (!mSizeInfoLog[BitTrueData::TYPE_RSC]) {
      mSizeInfoLog[BitTrueData::TYPE_RSC] = 1;
    }
  }

  // dump gyroProcData
  if (data.gyroProcData != NULL) {
    type = BitTrueData::TYPE_GYROPROC;
    writeSize = sizeof(MUINT32) + sizeof(GYRO_SET_PROC_INFO_STRUCT) +
                sizeof(mSensorInfo[mCurSensorIdx].mRecordParameter) +
                sizeof(MUINT64) * GYRO_DATA_PER_FRAME +
                sizeof(MDOUBLE) * GYRO_DATA_PER_FRAME * 3;
    if (data.gyroProcData->val_LMV)
      writeSize += sizeof(MINT32) * EIS_WIN_NUM * 4;
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(&data.gyroProcData->val_LMV, sizeof(MUINT32), 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.gyroProcData, sizeof(GYRO_SET_PROC_INFO_STRUCT), 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.gyroProcData->param,
               sizeof(mSensorInfo[mCurSensorIdx].mRecordParameter), 1,
               mpSourceDumpFp) == 0 ||
        fwrite(data.gyroProcData->gyro_t_frame,
               sizeof(MUINT64) * GYRO_DATA_PER_FRAME, 1, mpSourceDumpFp) == 0 ||
        fwrite(data.gyroProcData->gyro_xyz_frame,
               sizeof(MDOUBLE) * GYRO_DATA_PER_FRAME * 3, 1,
               mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
    if (data.gyroProcData->val_LMV) {
      if (fwrite(data.gyroProcData->EIS_LMV, sizeof(MINT32) * EIS_WIN_NUM * 4,
                 1, mpSourceDumpFp) == 0)
        MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
    if (!mSizeInfoLog[BitTrueData::TYPE_GYROPROC]) {
      MY_LOGD_IF(mDebugDump, "GYRO_SET_PROC_INFO_STRUCT%zu",
              sizeof(GYRO_SET_PROC_INFO_STRUCT));
      mSizeInfoLog[BitTrueData::TYPE_GYROPROC] = 1;
    }
  }

  if ((data.gyroResult != NULL) &&
      (data.gyroResult->inter_frame_angle != NULL) &&
      (data.gyroResult->intra_frame_angle != NULL)) {
    if (fwrite(data.gyroResult->inter_frame_angle, 3 * sizeof(double), 1, mpGyroResultFp) == 0 ||
        fwrite(data.gyroResult->intra_frame_angle, 3 * RowSliceNum * sizeof(double), 1, mpGyroResultFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }

  if (data.eisPlusAlgoInitData != NULL) {
    type = BitTrueData::TYPE_EISENV;
    writeSize = sizeof(EIS_PLUS_SET_ENV_INFO_STRUCT);
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(data.eisPlusAlgoInitData, sizeof(EIS_PLUS_SET_ENV_INFO_STRUCT),
               1, mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }

  if (data.eisPlusGetProcData != NULL) {
    type = BitTrueData::TYPE_GETPROC;
    writeSize = sizeof(EIS_PLUS_GET_PROC_INFO_STRUCT);
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(data.eisPlusGetProcData, sizeof(EIS_PLUS_GET_PROC_INFO_STRUCT),
               1, mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }

  if (data.gyroInitInfo != NULL) {
    type = BitTrueData::TYPE_GYROINIT;
    writeSize = sizeof(GYRO_INIT_INFO_STRUCT);
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(data.gyroInitInfo, sizeof(GYRO_INIT_INFO_STRUCT), 1,
               mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }

  if (data.fscData != NULL && data.fscData->scalingFactor != NULL) {
    MUINT32 fscsize = sizeof(MINT32) * mFSCInfo.numSlices;
    type = BitTrueData::TYPE_FSC;
    writeSize = sizeof(MINT32) + fscsize;
    if (fwrite(&type, sizeof(type), 1, mpSourceDumpFp) == 0 ||
        fwrite(&writeSize, sizeof(writeSize), 1, mpSourceDumpFp) == 0 ||
        fwrite(&data.requestNo, sizeof(MINT32), 1, mpSourceDumpFp) == 0 ||
        fwrite((unsigned char*)data.fscData->scalingFactor, fscsize, 1,
               mpSourceDumpFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
    if (!mSizeInfoLog[BitTrueData::TYPE_FSC]) {
      MY_LOGD_IF(mDebugDump, "fscScalingFactor%d", fscsize);
      mSizeInfoLog[BitTrueData::TYPE_FSC] = 1;
    }
  }

  if (data.isOIS && (mpOISFp != NULL)) {
    size_t oisNum = mOisTimestamp.size();
    if (fwrite(&oisNum, sizeof(oisNum), 1, mpOISFp) == 0 ||
        fwrite(&mOisTimestamp[0], sizeof(MUINT64) * mOisTimestamp.size(), 1,
               mpOISFp) == 0 ||
        fwrite(&mOisHallX[0], sizeof(MDOUBLE) * mOisHallX.size(), 1, mpOISFp) ==
            0 ||
        fwrite(&mOisHallY[0], sizeof(MDOUBLE) * mOisHallY.size(), 1, mpOISFp) ==
            0 ||
        fwrite(&mOisX[0], sizeof(MDOUBLE) * mOisX.size(), 1, mpOISFp) == 0 ||
        fwrite(&mOisY[0], sizeof(MDOUBLE) * mOisY.size(), 1, mpOISFp) == 0) {
      MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
    }
  }

  if (mSourceDumpIdx == 1) {
    if (mpDumpVersionFp != NULL && m_pGisAlg && m_pEisPlusAlg) {
      MINT32 version = PARSER_VERSION_ISP6;
      MINT32 gyroVersion = m_pGisAlg->getGyrolibVersion();
      MINT32 eisVersion = m_pEisPlusAlg->GetInterfaceVersion();
      if (fwrite(&version, sizeof(MINT32), 1, mpDumpVersionFp) == 0 ||
          fwrite(&gyroVersion, sizeof(MINT32), 1, mpDumpVersionFp) == 0 ||
          fwrite(&eisVersion, sizeof(MINT32), 1, mpDumpVersionFp) == 0) {
        MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
      }
    }

    if (mpCalibrationFp != NULL) {
      EIS50_CALIBRATION_DATA& caliData = mEisPlusAlgoInitData.EIS50_CalibrationData;
      if (fwrite(&mCalibrationSource, sizeof(MINT32), 1, mpCalibrationFp) == 0 ||
          fwrite(&caliData.calibration_Domain_Width, sizeof(MINT32), 1, mpCalibrationFp) == 0 ||
          fwrite(&caliData.calibration_Domain_Height, sizeof(MINT32), 1, mpCalibrationFp) == 0 ||
          fwrite(&caliData.mesh_width, sizeof(MINT32), 1, mpCalibrationFp) == 0 ||
          fwrite(&caliData.mesh_height, sizeof(MINT32), 1, mpCalibrationFp) == 0) {
        MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
      }
      if (mCalibrationSource != 3) {
        if (fwrite(&caliData.meshVertXY,
              sizeof(double) * caliData.mesh_width * caliData.mesh_height * 2,
              1, mpCalibrationFp) == 0 ||
            fwrite(&caliData.meshVertLngLat,
              sizeof(double) * caliData.mesh_width * caliData.mesh_height * 2,
              1, mpCalibrationFp) == 0 ||
            fwrite(&caliData.meshTriangleVertIndices,
              sizeof(MINT32) * (caliData.mesh_width-1) * (caliData.mesh_height-1) * 2 * 3,
              1, mpCalibrationFp) == 0) {
          MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
        }
      }
    }
  }
}

MVOID EisHalImp::createSourceDumpFile(const EIS_HAL_CONFIG_DATA& aEisConfig) {
  char path[100];
  mFirstFrameTs = aEisConfig.timestamp;
  if (snprintf(path, sizeof(path), EIS_DUMP_FOLDER_PATH "/%" PRId64 "",
               mFirstFrameTs) < 0 ||
      mkdir(path, S_IRWXU | S_IRWXG)) {
    MY_LOGE("mkdir %s failed", path);
    return;
  }

  // close previous file if needed
  if (mpSourceDumpFp != NULL) {
    closeSourceDumpFile();
  }

  IF_COND_RETURN_VALUE(
      snprintf(path, sizeof(path),
               EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64 ".bin",
               mFirstFrameTs, mFirstFrameTs) < 0, );
  mpSourceDumpFp = fopen(path, "ab");
  IF_COND_RETURN_VALUE(mpSourceDumpFp == NULL, );

  IF_COND_RETURN_VALUE(
      snprintf(path, sizeof(path),
               EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64 "_OIS.bin",
               mFirstFrameTs, mFirstFrameTs) < 0, );
  mpOISFp = fopen(path, "ab");
  IF_COND_RETURN_VALUE(mpOISFp == NULL, );

  IF_COND_RETURN_VALUE(
      snprintf(path, sizeof(path),
               EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64 "_ACC.bin",
               mFirstFrameTs, mFirstFrameTs) < 0, );
  mpAccFp = fopen(path, "ab");
  IF_COND_RETURN_VALUE(mpAccFp == NULL, );

  IF_COND_RETURN(snprintf(path, sizeof(path),
                          EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64
                                               "_VERSION.bin",
                          mFirstFrameTs, mFirstFrameTs) < 0);
  mpDumpVersionFp = fopen(path, "ab");
  IF_COND_RETURN(mpDumpVersionFp == NULL);
  IF_COND_RETURN(snprintf(path, sizeof(path),
                          EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64
                                               "_CALIBRATION.bin",
                          mFirstFrameTs, mFirstFrameTs) < 0);
  mpCalibrationFp = fopen(path, "ab");
  IF_COND_RETURN(mpCalibrationFp == NULL);

  IF_COND_RETURN(snprintf(path, sizeof(path),
                          EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64
                                               "_GYRO_RESULT.bin",
                          mFirstFrameTs, mFirstFrameTs) < 0);
  mpGyroResultFp = fopen(path, "ab");
  IF_COND_RETURN(mpGyroResultFp == NULL);

  memset(&mSizeInfoLog, 0, sizeof(mSizeInfoLog));
}

MVOID EisHalImp::closeSourceDumpFile() {
  if (fclose(mpSourceDumpFp) != 0 || fclose(mpOISFp) != 0 ||
      fclose(mpAccFp) != 0 || fclose(mpDumpVersionFp) != 0 ||
      fclose(mpCalibrationFp) != 0 ||
      fclose(mpGyroResultFp) != 0) {
    MY_LOGW("IO failed. errno(%d) reason: %s", errno, strerror(errno));
  }
  mpSourceDumpFp = NULL;
  mpOISFp = NULL;
  mpAccFp = NULL;
  mpDumpVersionFp = NULL;
  mpCalibrationFp = NULL;
  mpGyroResultFp = NULL;
}
