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
 * @file eis_hal_imp.h
 *
 * EIS Hal Implementation Header File
 *
 */

#ifndef FEATURE_COMMON_EIS_EIS_HAL_IMP_H_
#define FEATURE_COMMON_EIS_EIS_HAL_IMP_H_

#include <queue>
#include <vector>

#include "Condition.h"
#include "Mutex.h"
#include "mtkcam/aaa/IHal3A.h"
#include "mtkcam/utils/imgbuf/IImageBuffer.h"
#include "mtkcam3/feature/eis/eis_hal.h"

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define MAX_MEMORY_SIZE (40)
#define TSRECORD_MAXSIZE (108000)
#define EIS_ACCELEROMETER (1)
#define EIS_GYROSCOPE (2)
#define GYRO_DATA_PER_FRAME (100)
#define MAX_SENSOR_ID (10)
#define OIS_MAX_NUM (200)

#define MAIN_SENSOR_ID 0
#define FRONT_SENSOR_ID 1
#define ULTRAWIDE_SENSOR_ID 2
#define TELE_SENSOR_ID 3

typedef struct EIS_GyroRecord_t {
  MINT32 id;
  MFLOAT x;
  MFLOAT y;
  MFLOAT z;
  MINT64 ts;
} EIS_GyroRecord;

class EIS_SensorInfo {
 public:
  MUINT32 mUsed;
  MUINT32 mSensorDev;
  MINT64 mtRSTime;
  MUINT32 mSensorPixelClock;
  MUINT32 mSensorLinePixel;
  MUINT32 mDefWidth;
  MUINT32 mDefHeight;
  MUINT32 mDefCrop;
  MDOUBLE mRecordParameter[6];
  NSCam::SensorCropWinInfo mSensorCropInfo;

  EIS_SensorInfo() {
    MUINT32 i;
    mUsed = 0;
    mSensorDev = 0;
    mtRSTime = 0;
    mSensorPixelClock = 0;
    mSensorLinePixel = 0;
    mDefWidth = 0;
    mDefHeight = 0;
    mDefCrop = 0;
    for (i = 0; i < 6; i++) {
      mRecordParameter[i] = 0;
    }
    memset(&mSensorCropInfo, 0, sizeof(mSensorCropInfo));
  }
};

typedef struct GyroEISStatistics_t {
  MINT32 eis_data[EIS_WIN_NUM * 4];
  MUINT64 ts;
} GyroEISStatistics;

struct NVRAM_CAMERA_FEATURE_STRUCT_t;
struct NVRAM_CAMERA_FEATURE_GIS_STRUCT_t;

class LMVHal;
namespace NSCam {
namespace Utils {
class SensorProvider;
}
}  // namespace NSCam

/**
 *@class EisHalImp
 *@brief Implementation of EisHal class
 */
class EisHalImp : public EisHal {
  template <const unsigned int>
  friend class EisHalObj;

 private:
  EisHalImp(const EisHalImp&);
  EisHalImp& operator=(const EisHalImp&);
  /**
   *@brief EisHalImp constructor
   */
  EisHalImp(const NSCam::EIS::EisInfo& eisInfo,
            const MUINT32& aSensorIdx,
            MUINT32 MultiSensor = 0);

  /**
   *@brief EisHalImp destructor
   */
  ~EisHalImp() {}

 public:
  /**
   *@brief Create EisHal object
   *@param[in] aSensorIdx : sensor index
   *@return
   *-EisHal object
   */
  static EisHal* GetInstance(const NSCam::EIS::EisInfo& eisInfo,
                             const MUINT32& aSensorIdx,
                             MUINT32 MultiSensor = 0);

  /**
   *@brief Destroy EisHal object
   *@param[in] userName : user name,i.e. who destroy EIS object
   */
  virtual MVOID DestroyInstance(char const* userName);

  /**
   *@brief Initialization function
   *@param[in] aSensorIdx : sensor index
   *@return
   *-EIS_RETURN_NO_ERROR indicates success, otherwise indicates fail
   */
  virtual MINT32 Init();

  /**
   *@brief Unitialization function
   *@return
   *-EIS_RETURN_NO_ERROR indicates success, otherwise indicates fail
   */
  virtual MINT32 Uninit();

  /**
   *@brief Configure EIS
   *@details Use this API after pass1/pass2 config and before pass1/pass2 start
   *@param[in] aEisPass : indicate pass1 or pass2
   *@param[in] aEisConfig : EIS config data
   *@return
   *-EIS_RETURN_NO_ERROR indicates success, otherwise indicates fail
   */
  virtual MINT32 ConfigRSCMEEis(const EIS_HAL_CONFIG_DATA& aEisConfig,
                                const MUINT32 eisMode = 0,
                                const FSC_INFO_T* fscInfo = NULL);

  virtual MINT32 DoRSCMEEis(EIS_HAL_CONFIG_DATA* apEisConfig,
                            IMAGE_BASED_DATA* imgBaseData,
                            MINT64 aTimeStamp,
                            MUINT32 aExpTime,
                            MUINT32 aLExpTime,
                            MUINT32 staggerNum,
                            EIS_HAL_OUT_DATA& outData);

  /**
   *@brief Get EIS Plus Warp infomation
   *@param[in] aGridX : pointer to array
   *@param[in] aGridY : pointer to array
   */
  virtual MVOID SetEisPlusWarpInfo(MINT32* const aGridX,
                                   MINT32* const aGridY,
                                   MINT32* const aGridX_standard,
                                   MINT32* const aGridY_standard);

  virtual MVOID AddMultiSensors(MUINT32 sensorID);

  virtual MVOID configMVNumber(const EIS_HAL_CONFIG_DATA& apEisConfig,
                               MINT32* mvWidth,
                               MINT32* mvHeight);

  virtual MBOOL getCalibrationData();

 private:
  struct BitTrueData {
    enum DataType {
      TYPE_WARPMAP,
      TYPE_EISPROC,
      TYPE_RSC,
      TYPE_GYROPROC,
      TYPE_EISENV,
      TYPE_GETPROC,
      TYPE_GYROINIT,
      TYPE_FSC,
      TYPE_OIS,
      TYPE_COUNT
    };

    BitTrueData()
        : eisResult(NULL),
          eisProcInfo(NULL),
          rscData(NULL),
          gyroResult(NULL),
          gyroProcData(NULL),
          eisPlusAlgoInitData(NULL),
          eisPlusGetProcData(NULL),
          gyroInitInfo(NULL),
          fscData(NULL) {}
    EIS_PLUS_RESULT_INFO_STRUCT* eisResult;
    EIS_PLUS_SET_PROC_INFO_STRUCT* eisProcInfo;
    RSCME_PACKAGE* rscData;
    GYRO_MV_RESULT_INFO_STRUCT* gyroResult;
    GYRO_SET_PROC_INFO_STRUCT* gyroProcData;
    EIS_PLUS_SET_ENV_INFO_STRUCT* eisPlusAlgoInitData;
    EIS_PLUS_GET_PROC_INFO_STRUCT* eisPlusGetProcData;
    GYRO_INIT_INFO_STRUCT* gyroInitInfo;
    FSC_PACKAGE* fscData;
    MINT32 requestNo = 0;
    MBOOL isOIS = false;
  };

  /**
   *@brief Get sensor info
   *@return
   *-EIS_RETURN_NO_ERROR indicates success, otherwise indicates fail
   */
  MINT32 GetSensorInfo(MUINT32 sensorID);
  MINT32 CalcSensorTrs(MUINT32 sensorID, MUINT32 sensorHeight);

  /**
   *@brief Get EIS2.0 customize info
   *@param[in] apEisConfig : EIS config data, mainly for pass2
   *@param[out] a_pTuningData : EIS_PLUS_TUNING_PARA_STRUCT
   */
  MVOID GetEisPlusCustomize(EIS_PLUS_TUNING_PARA_STRUCT* a_pTuningData,
                            const EIS_HAL_CONFIG_DATA* apEisConfig = NULL);

  /**
   *@brief Get EIS3.0 customize info
   *@param[in] apEisConfig : EIS config data, mainly for pass2
   *@param[out] a_pTuningData : EIS30_TUNING_PARA_STRUCT
   */
  MVOID GetEis30Customize(EIS30_TUNING_PARA_STRUCT* a_pTuningData,
                          const EIS_HAL_CONFIG_DATA* apEisConfig = NULL);

  /**
   *@brief Create IMem buffer
   *@param[in,out] memSize : memory size, will align to L1 cache
   *@param[in] bufCnt : how many buffer
   *@param[in,out] bufInfo : IMem object
   *@return
   *-EIS_RETURN_NO_ERROR indicates success, otherwise indicates fail
   */
  MINT32 CreateMemBuf(MUINT32 memSize, android::sp<NSCam::IImageBuffer>& spImageBuf);
  /**
   *@brief Destroy IMem buffer
   *@param[in] bufCnt : how many buffer
   *@param[in,out] bufInfo : IMem object
   *@return
   *-EIS_RETURN_NO_ERROR indicates success, otherwise indicates fail
   */
  MINT32 DestroyMemBuf(android::sp<NSCam::IImageBuffer>& spImageBuf);
  MINT32 EnableSensor();

  MINT32 CreateEISPlusAlgoBuf();
  MINT32 ConfigGyroAlgo(const EIS_HAL_CONFIG_DATA& aEisConfig,
                        const EIS_PLUS_SET_ENV_INFO_STRUCT& eisInitData);
  MINT32 ExecuteGyroAlgo(const EIS_HAL_CONFIG_DATA* apEisConfig,
                         const MINT64 aTimeStamp,
                         const MUINT32 aExpTime,
                         const MUINT32 aLExpTime,
                         GYRO_MV_RESULT_INFO_STRUCT& gyroMVresult,
                         IMAGE_BASED_DATA* imgBaseData = NULL);
  MVOID GetSensorData();
  MVOID dumpBitTrue(const BitTrueData& data);
  MVOID createSourceDumpFile(const EIS_HAL_CONFIG_DATA& aEisConfig);
  MVOID closeSourceDumpFile();

  /***************************************************************************************/

  volatile MINT32 mUsers;
  mutable android::Mutex mLock;
  mutable android::Mutex mP1Lock;
  mutable android::Mutex mP2Lock;

  // EIS algo object
  MTKGyro* m_pGisAlg;
  MTKEisPlus* m_pEisPlusAlg;
  EIS_PLUS_SET_ENV_INFO_STRUCT mEisPlusAlgoInitData;
  EIS_PLUS_GET_PROC_INFO_STRUCT mEisPlusGetProcData;
  GYRO_INIT_INFO_STRUCT mGyroAlgoInitData;
  EIS_PLUS_INIT_PARAM_STRUCT mEisPlusInitParam;

  android::sp<NSCam::IImageBuffer> m_pEisDbgBuf;
  android::sp<NSCam::IImageBuffer> m_pEisPlusWorkBuf;
  android::sp<NSCam::IImageBuffer> m_pGisWorkBuf;

  // EIS Plus Member Variable
  MUINT32 mSrzOutW;
  MUINT32 mSrzOutH;
  MUINT32 mGpuGridW;
  MUINT32 mGpuGridH;
  MINT32 mMVWidth;
  MINT32 mMVHeight;

  // EIS result
  MUINT32 mDoEisCount;  // Vent@20140427: Add for EIS GMV Sync Check.

  // EISPlus result
  EIS_PLUS_RESULT_INFO_STRUCT mEisPlusResult;

  // member variable
  MUINT32 mFrameCnt;
  MUINT32 mIsEisConfig;
  MUINT32 mIsEisPlusConfig;
  MUINT32 mEisPlusCropRatio;
  MBOOL mEisSupport;
  FSC_INFO mFSCInfo;
  MUINT32 mEisMode;
  MINT32 mEISInterval;
  MINT32 mAccInterval = 20;
  MINT32 mOISInterval = 5;
  MINT32 mSourceDumpIdx = 0;
  MINT32 mCentralCrop = 0;
  MINT32 mAlgoEisMode = ALGO_EIS50_MODE;
  MFLOAT mFocalLength = 0;

  android::sp<NSCam::Utils::SensorProvider> mpSensorProvider = NULL;

  // Sensor Hal
  NSCam::IHalSensorList* m_pHalSensorList;
  NSCam::IHalSensor* m_pHalSensor;
  // 3A
  NS3Av3::IHal3A* m_p3aHal;

  NSCam::EIS::EisInfo mEisInfo;
  const MUINT32 mSensorIdx;
  const MUINT32 mMultiSensor;
  MUINT32 mSensorMode = 0;
  // MUINT32 mSensorDev;
  MBOOL mGyroEnable;
  MBOOL mAccEnable;
  MBOOL mOisEnable = MFALSE;
  MUINT64 mTsForAlgoDebug;
  NVRAM_CAMERA_FEATURE_STRUCT_t* m_pNvram;
  MUINT32 mChangedInCalibration;
  MUINT32 mGisInputW;
  MUINT32 mGisInputH;
  MBOOL mNVRAMRead;
  MINT64 mSleepTime;
  MBOOL mSkipWaitGyro;
  MBOOL mbLastCalibration;
  MUINT32 mStaggerNum = 0;
  MINT32 mSourceDump;
  MINT32 mQuickDump;
  MINT32 mCalibrationSource = 3;
  MINT64 mFirstFrameTs;
  FILE* mpTsLogFp = NULL;
  FILE* mpSourceDumpFp = NULL;
  FILE* mpOISFp = NULL;
  FILE* mpAccFp = NULL;
  FILE* mpDumpVersionFp = NULL;
  FILE* mpCalibrationFp = NULL;
  FILE* mpGyroResultFp = NULL;
  MUINT32 mSizeInfoLog[BitTrueData::TYPE_COUNT] = {0};
  MDOUBLE mTRS = 0;
  // workaround
  const MINT32 RowSliceNum = 19;

  MUINT32 mCurSensorIdx;
  EIS_SensorInfo mSensorInfo[MAX_SENSOR_ID];

  MBOOL mbEMSaveFlag;
  NVRAM_CAMERA_FEATURE_GIS_STRUCT_t* m_pNVRAM_defParameter;

  LMVHal* m_pLMVHal;
  std::vector<MUINT64> mOisTimestamp;
  std::vector<MDOUBLE> mOisX;
  std::vector<MDOUBLE> mOisY;
  std::vector<MDOUBLE> mOisHallX;
  std::vector<MDOUBLE> mOisHallY;

  std::vector<MUINT64> mAccTimestamp;
  std::vector<MDOUBLE> mAccData;

 public:
  MINT32 mEMEnabled;
  MINT32 mDebugDump;
  MINT32 mDisableGyroData;
  MINT32 mGyroOnly;
  MINT32 mImageOnly;
  MINT32 mForecDejello;
};

/**
 *@class EisHalObj
 *@brief singleton object for each EisHal which is seperated by sensor index
 */

template <const unsigned int aSensorIdx>
class EisHalObj : public EisHalImp {
 private:
  EisHalObj(const NSCam::EIS::EisInfo& eisInfo, MUINT32 MultiSensor = 0)
      : EisHalImp(eisInfo, aSensorIdx, MultiSensor) {}
  ~EisHalObj() {}

 public:
  static EisHal* GetInstance(const NSCam::EIS::EisInfo& eisInfo,
                             MUINT32 MultiSensor = 0);
  static void destroyInstance(void);

  static std::queue<EIS_GyroRecord> gGyroDataQueue;
  static android::Mutex gGyroQueueLock;
  static android::Condition gWaitGyroCond;
  static MUINT32 gGyroCount;
  static MUINT32 gGyroReverse;
  static MINT64 gLastGyroTimestamp;
};

#endif  // FEATURE_COMMON_EIS_EIS_HAL_IMP_H_
