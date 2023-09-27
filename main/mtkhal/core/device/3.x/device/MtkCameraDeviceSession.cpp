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

#include "main/mtkhal/core/device/3.x/device/MtkCameraDeviceSession.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "MyUtils.h"
#include "mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h"
#include "mtkcam3/feature/stereo/hal/stereo_setting_provider.h"
//#include "mtkcam-interfaces/hw/camsys/cam_coordinator/ICamCoordinator.h"
#include "mtkcam/utils/cat/CamAutoTestLogger.h"
#include "mtkcam/utils/metadata/client/mtk_metadata_tag.h"
#include "mtkcam/utils/std/Misc.h"
#include "mtkcam/utils/std/ULog.h"  // will include <log/log.h> if LOG_TAG was defined
#include "mtkcam/utils/debug/Properties.h"
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

using mcam::core::HAL_NO_INIT;
using mcam::core::HAL_OK;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::ITemplateRequest;
using NSCam::CameraInfo;
using NSCam::MRect;
using NSCam::Type2Type;
using NSCam::SENSOR_RAW_FMT_NONE;
using NSCam::eTransform_ROT_90;
using NSCam::eTransform_ROT_180;
using NSCam::eTransform_ROT_270;
using NSCam::eTransform_None;
using NSCam::DEVICE_FEATURE_VSDOF;
using NSCam::DEVICE_FEATURE_SECURE_CAMERA;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_ULOGM_ASSERT(0, "[%s::%s] " fmt, getLogPrefix().c_str(), \
                  __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_ULOGM_FATAL("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGCAT(fmt, arg...) \
  CAM_ULOGMD("[CAT][Pipeline] " fmt, ##arg)
//
#define MY_LOGV_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGV(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGI_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGI(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

namespace mcam {
namespace core {

template <typename T>
inline MBOOL tryGetMetadata(IMetadata const* metadata,
                            MUINT32 const tag,
                            T& rVal) {
  if (metadata == NULL) {
    CAM_ULOGME("InMetadata == NULL");
    return MFALSE;
  }

  IMetadata::IEntry entry = metadata->entryFor(tag);
  if (!entry.isEmpty()) {
    rVal = entry.itemAt(0, Type2Type<T>());
    return MTRUE;
  }
  return MFALSE;
}

#define ThisNamespace MtkCameraDeviceSession
/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::~MtkCameraDeviceSession() {
  if (mpCpuCtrl) {
    mpCpuCtrl->destroyInstance();
    mpCpuCtrl = NULL;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::MtkCameraDeviceSession(CreationInfo const& info)
    : IMtkcamVirtualDeviceSession(),
      IMtkcamDeviceSession(),
      mStaticInfo(info),
      mLogPrefix(std::to_string(info.mStaticDeviceInfo->mInstanceId) +
                 "-session"),
      mLogLevel(0),
      mStateLog(),
      mPolicyStaticInfo(std::make_shared<PolicyStaticInfo>()) {
  MY_LOGD("%p", this);

  mCpuPerfTime = NSCam::Utils::Properties::property_get_int32(
                  "vendor.cam3dev.cpuperftime", CPU_TIMEOUT);
  MY_LOGD("Create CPU Ctrl, timeout %d ms", mCpuPerfTime);
  mpCpuCtrl = Cam3CPUCtrl::createInstance();

  auto pHalLogicalDeviceList = MAKE_HalLogicalDeviceList();
  if (pHalLogicalDeviceList) {
    mSensorId =
        pHalLogicalDeviceList->getSensorId(info.mStaticDeviceInfo->mInstanceId);
  }
  mStateLog.add("-> initialized");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getSafeCaptureSession() const
    -> android::sp<ICaptureSession> {
  std::lock_guard<std::mutex> _l(mCaptureSessionLock);
  return mCaptureSession;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::waitUntilOpenDone() -> bool {
  MY_LOGD_IF(getLogLevel() >= 1, "+");
  //
  bool ret = true;
//  for (auto& fut : mvOpenFutures) {
//    bool result = fut.get();
//    if (MY_UNLIKELY(!result)) {
//      MY_LOGE("Fail to init");
//      ret = false;
//    }
//  }
//  //
//  mvOpenFutures.clear();
  MY_LOGD_IF(getLogLevel() >= 1, "- ret:%d", ret);
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::initCameraInfo(
    const PipelineSessionType& sessionType,
    const StreamConfiguration& requestedConfiguration,
    CameraInfoMapT& cameraInfoMap) -> std::vector<int32_t> {
  /* decide sensor list */
  std::vector<int32_t> sensorId;
  // session type
  if (sessionType == PipelineSessionType::MULTICAM_VSDOF) {
    for (int i = 0; i < 2; i++) {
      if (i < mSensorId.size()) {
        sensorId.push_back(mSensorId[i]);
      }
    }
  } else if (sessionType == PipelineSessionType::MULTICAM_ZOOM) {
    sensorId = mSensorId;
  }

  // physical streams
  for (auto const& stream : requestedConfiguration.streams) {
    if (stream.physicalCameraId != -1) {
      auto const& physicalId = stream.physicalCameraId;

      auto it = std::find(sensorId.begin(), sensorId.end(), physicalId);
      if (it == sensorId.end())
        sensorId.push_back(physicalId);
    }
  }

  // single camera
  if (sensorId.size() == 0)
    sensorId = mSensorId;

  for (size_t i = 0; i < sensorId.size(); i++) {
    android::sp<IMetadataProvider> pMetadataProvider =
        NSCam::NSMetadataProviderManager::valueFor(sensorId[i]);
    if (pMetadataProvider != NULL) {
      const IMetadata& mrStaticMetadata =
          pMetadataProvider->getMtkStaticCharacteristics();
      MRect activeArray;
      CameraInfo info;
      float aperture;
      float focalLengh;
      MINT32 orientation;

//      std::shared_ptr<ICamCoordinator> pCamCoordinator =
//              ICamCoordinator::getInstance();
//      MY_LOGF_IF(!pCamCoordinator, "Bad pCamCoordinator");
//      if (MY_UNLIKELY(!pCamCoordinator->getSensorRawFmtType(sensorId[i],
//                                                        info.sensorRawType))) {
//        info.sensorRawType = SENSOR_RAW_FMT_NONE;
//        MY_LOGW("sensorId[%zu]:%d fail on getSensorRawFmtType", i, sensorId[i]);
//      }

      if (tryGetMetadata<MRect>(&mrStaticMetadata,
                                MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION,
                                activeArray)) {
        info.activeArray[0] = activeArray.p.x;
        info.activeArray[1] = activeArray.p.y;
        info.activeArray[2] = activeArray.s.w;
        info.activeArray[3] = activeArray.s.h;
      } else {
        MY_LOGE(
            "sensor:%d, no static info: MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION",
            sensorId[i]);
        continue;
      }

      if (tryGetMetadata<MFLOAT>(&mrStaticMetadata,
                                 MTK_LENS_INFO_AVAILABLE_APERTURES, aperture)) {
        info.aperture = aperture;
      } else {
        MY_LOGE("sensor:%d, no static info: MTK_LENS_INFO_AVAILABLE_APERTURES",
                sensorId[i]);
        continue;
      }

      if (tryGetMetadata<MFLOAT>(&mrStaticMetadata,
                                 MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
                                 focalLengh)) {
        info.focalLength = focalLengh;
      } else {
        MY_LOGE(
            "sensor:%d, no static info: MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS",
            sensorId[i]);
        continue;
      }

      if (!tryGetMetadata<MUINT8>(&mrStaticMetadata, MTK_SENSOR_INFO_FACING,
                                  info.sensorFacing)) {
        MY_LOGE("sensor:%d, no static info: MTK_SENSOR_INFO_FACING",
                sensorId[i]);
        continue;
      }

      if (tryGetMetadata<MINT32>(&mrStaticMetadata, MTK_SENSOR_INFO_ORIENTATION,
                                 orientation)) {
        info.sensorTransform =
            orientation == 90
                ? eTransform_ROT_90
                : orientation == 180 ? eTransform_ROT_180
                                     : orientation == 270 ? eTransform_ROT_270
                                                          : eTransform_None;
      } else {
        MY_LOGE("sensor:%d, no static info: MTK_SENSOR_INFO_ORIENTATION",
                sensorId[i]);
        continue;
      }

      // get sensor physical size
      IMetadata::IEntry entry =
          mrStaticMetadata.entryFor(MTK_SENSOR_INFO_PHYSICAL_SIZE);
      if (entry.count() == 2) {
        info.sensorPhysicalSize[0] = entry.itemAt(0, Type2Type<MFLOAT>());
        info.sensorPhysicalSize[1] = entry.itemAt(1, Type2Type<MFLOAT>());
      } else {
        MY_LOGW("sensor:%d, MTK_SENSOR_INFO_PHYSICAL_SIZE size not eq to 2.",
                sensorId[i]);
        continue;
      }
      cameraInfoMap.insert(std::make_pair(sensorId[i], info));
    } else {
      MY_LOGD("no metadata provider, sensor:%d", sensorId[i]);
    }
  }
  return sensorId;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::initPolicyStaticInfo() -> void {
  mPolicyStaticInfo->uniqueKey = NSCam::Utils::TimeTool::getReadableTime();
  mPolicyStaticInfo->openId = getInstanceId();
  //  mPolicyStaticInfo->sensorId = mSensorId;
  mPolicyStaticInfo->isDualDevice = mSensorId.size() > 1;

  auto pHalLogicalDeviceList = MAKE_HalLogicalDeviceList();

  if (pHalLogicalDeviceList) {
    MINT32 dualFeature =
        pHalLogicalDeviceList->getSupportedFeature(mPolicyStaticInfo->openId);
//    if (dualFeature & DEVICE_FEATURE_VSDOF) {
//      mPolicyStaticInfo->numsP1YUV = StereoSettingProvider::getP1YUVSupported();
//    }
    mPolicyStaticInfo->isSecureDevice =
        ((pHalLogicalDeviceList->getSupportedFeature(
              mPolicyStaticInfo->openId) &
          DEVICE_FEATURE_SECURE_CAMERA) &
         DEVICE_FEATURE_SECURE_CAMERA) > 0;
  }

//  std::shared_ptr<ICamCoordinator> pCamCoordinator =
//          ICamCoordinator::getInstance();
//  MY_LOGF_IF(!pCamCoordinator, "Bad pCamCoordinator");
//  for (size_t i = 0; i < mSensorId.size(); i++) {
//    if (pCamCoordinator->get4CellSensorSupported(mSensorId[i])) {
//      mPolicyStaticInfo->is4CellSensor = true;
//      mPolicyStaticInfo->v4CellSensorIds.emplace(mSensorId[i]);
//    }
//  }

  mPolicyStaticInfo->isP1DirectFDYUV = [this]() {
    bool isP1DirectFDYUV = false;

    const char* keyDebug = "persist.vendor.debug.camera.directfdyuv";
    int32_t debugDirectFDYUV = NSCam::Utils::Properties::property_get_int32(
                                keyDebug, -1);
    if (debugDirectFDYUV != -1) {
      MY_LOGI("%s=%d", keyDebug, debugDirectFDYUV);
      isP1DirectFDYUV = (debugDirectFDYUV == 1);
    } else {
      const char* key = "ro.vendor.camera.directfdyuv.support";
      isP1DirectFDYUV =
        (1 == NSCam::Utils::Properties::property_get_int32(key, 0));
    }

    return isP1DirectFDYUV;
  }();

  if (CamAutoTestLogger::checkEnable()) {
    MY_LOGCAT("useP1DirectFDYUV:%d", mPolicyStaticInfo->isP1DirectFDYUV);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::decideCaptureSession(
    const StreamConfiguration& requestedConfiguration) -> PipelineSessionType {
  auto type = PipelineSessionType::BASIC;
  do {
    ////////////////////////////////////////////////////////////////////////////
    //  SMVR
    ////////////////////////////////////////////////////////////////////////////
    if ((MUINT32)requestedConfiguration.operationMode == 1) {
      /* CONSTRAINED_HIGH_SPEED_MODE */
      MY_LOGI("SMVRConstraint");
      type = PipelineSessionType::SMVR;
      break;
    }

    ////////////////////////////////////////////////////////////////////////////
    //  SMVRBatch
    ////////////////////////////////////////////////////////////////////////////
    {
      auto const& entry = requestedConfiguration.sessionParams
                         .entryFor(MTK_SMVR_FEATURE_SMVR_MODE);
      if (!entry.isEmpty() && entry.count() >= 2) {
        MY_LOGI("SMVRBatch");
        type = PipelineSessionType::SMVR_BATCH;
        break;
      }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Multicam
    ////////////////////////////////////////////////////////////////////////////
    if (mPolicyStaticInfo->isDualDevice) {
//      StereoSettingProvider::setLogicalDeviceID(mPolicyStaticInfo->openId);

      int32_t mode = -1;
      int32_t iFeatureMode = -1;
      auto const& entry = requestedConfiguration.sessionParams
                         .entryFor(MTK_MULTI_CAM_FEATURE_MODE);
      if (!entry.isEmpty()) {
        mode = entry.itemAt(0, Type2Type<MINT32>());
      }
      iFeatureMode = NSCam::Utils::Properties::property_get_int32(
                      "vendor.camera.forceFeatureMode", mode);
      // single cam mode does not set MTK_MULTI_CAM_FEATURE_MODE and
      // vPhysicCameras should be zero.
      bool bPhysicCamera = false;
      for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
        const auto& srcStream = requestedConfiguration.streams[i];
        if (srcStream.physicalCameraId != -1) {
          bPhysicCamera = true;
          break;
        }
      }

      bool isSingleCamFlow = ((iFeatureMode == -1) && (!bPhysicCamera));
      if (!isSingleCamFlow) {
        std::string name;
        switch (iFeatureMode) {
          case MTK_MULTI_CAM_FEATURE_MODE_VSDOF:
            type = PipelineSessionType::MULTICAM_VSDOF;
            name = "Vsdof";
            break;
          case MTK_MULTI_CAM_FEATURE_MODE_ZOOM:
            type = PipelineSessionType::MULTICAM_ZOOM;
            name = "Zoom";
            break;
          default:
            type = PipelineSessionType::MULTICAM;
            name = "Multicam";
            break;
        }
        MY_LOGI("%s", name.c_str());
        break;
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    // 4Cell
    ////////////////////////////////////////////////////////////////////////////
    {
      if (mPolicyStaticInfo->is4CellSensor) {
        type = PipelineSessionType::MULTI_CELL;
        MY_LOGI("4Cell");
        break;
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    //  Default
    ////////////////////////////////////////////////////////////////////////////
    {
      type = PipelineSessionType::BASIC;
      MY_LOGI("Default");
      break;
    }

    MY_LOGE("Never here");
  } while (1);

  return type;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NSCam::IMtkcamVirtualDeviceSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer,
                              const std::vector<std::string>& options __unused)
    -> void {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  printer.printLine(
      "\n== state transition (most recent at bottom): Camera Device ==");
  mStateLog.print(printer);

  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    pCaptureSession->dumpState(printer, options);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::open(
    const std::shared_ptr<IMtkcamDeviceCallback> callback)
    -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  std::lock_guard<std::mutex> _lOpsLock(mOpsLock);
  MY_LOGI("+");

  std::string const stateTag("-> open");
  mStateLog.add((stateTag + " +").c_str());

  mLogLevel = getCameraDevice3DebugLogLevel();

  mCameraDeviceCallback = callback;

  if (mpCpuCtrl) {
    MY_LOGD("Enter camera perf mode");
    mpCpuCtrl->enterCameraPerf(mCpuPerfTime);
  }

  mDisplayIdleDelayUtil.enable();

  int status = HAL_OK;

  do {
    auto pDeviceManager = mStaticInfo.mDeviceManager;
    auto const& instanceName = mStaticInfo.mStaticDeviceInfo->mInstanceName;

    status = pDeviceManager->startOpenDevice(instanceName);
    if (HAL_OK != status) {
      pDeviceManager->updatePowerOnDone();
      break;
    }
    pDeviceManager->updatePowerOnDone();

    if (HAL_OK != status) {
      pDeviceManager->finishOpenDevice(instanceName, true /*cancel*/);
      break;
    }

    status = pDeviceManager->finishOpenDevice(instanceName, false /*cancel*/);
    if (HAL_OK != status) {
      break;
    }
  } while (0);

  MY_LOGI("-");

  mStateLog.add(
    (stateTag + " - " + (0 == status ? "OK" : ::strerror(-status))).c_str());

  return status;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraDeviceSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::constructDefaultRequestSettings(
    const RequestTemplate type, IMetadata& requestTemplate) -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  std::ostringstream stateTag;
  stateTag << "-> constructDefaultRequestSettings (type:"
      << std::hex << std::showbase << type << ")";
  mStateLog.add((stateTag.str() + " +").c_str());
  MY_LOGD_IF(1, "%s", stateTag.str().c_str());

  ITemplateRequest* obj =
    NSCam::NSTemplateRequestManager::valueFor(getInstanceId());
  if (obj == nullptr) {
    obj = ITemplateRequest::getInstance(getInstanceId());
    NSCam::NSTemplateRequestManager::add(getInstanceId(), obj);
  }

  mStateLog.add((stateTag.str() + " -").c_str());
  requestTemplate = obj->getMtkData(static_cast<int>(type));
  return HAL_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::configureStreams(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration)  // output
    -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();
  MY_LOGD("tryrun +");

  std::lock_guard<std::mutex> _lOpsLock(mOpsLock);

  class scopedVar {
   public:
    scopedVar(Cam3CPUCtrl* pCpuCtrl, MUINT32 pCpuPerfTime)
        : mCpuCtrl(pCpuCtrl), mpCpuPerfTime(pCpuPerfTime) {
      if (mCpuCtrl)
        mCpuCtrl->enterCPUBoost(mpCpuPerfTime, &handle);
    }
    ~scopedVar() {
      if (mCpuCtrl)
        mCpuCtrl->exitCPUBoost(&handle);
    }

   private:
    int handle = 0;
    MUINT32 mpCpuPerfTime;
    Cam3CPUCtrl* mCpuCtrl;
  } _localVar(mpCpuCtrl, mCpuPerfTime);

  if (!waitUntilOpenDone()) {
    MY_LOGE("open fail");
    return HAL_NO_INIT;
  }

  {
    std::lock_guard<std::mutex> _l(mCaptureSessionLock);
    if (mCaptureSession) {
      mCaptureSession->close();
      mCaptureSession = nullptr;
    }
    initPolicyStaticInfo();
    auto sessionType = decideCaptureSession(requestedConfiguration);
    mPolicyStaticInfo->sensorId = initCameraInfo(
        sessionType, requestedConfiguration, mPolicyStaticInfo->cameraInfoMap);
    mPolicyStaticInfo->sessionType = sessionType;

    ICaptureSession::CreationInfo info;
    info.mStaticDeviceInfo = mStaticInfo.mStaticDeviceInfo;
    info.mMetadataProvider = mStaticInfo.mMetadataProvider;
    info.mMetadataConverter = mStaticInfo.mMetadataConverter;
    info.mPhysicalMetadataProviders = mStaticInfo.mPhysicalMetadataProviders;
    if (auto cb = mCameraDeviceCallback.lock()) {
      info.mCallback = cb;
    }
    info.mSensorId = mSensorId;
    info.mPolicyStaticInfo = mPolicyStaticInfo;
    info.sessionParams = requestedConfiguration.sessionParams;
    mCaptureSession = createCaptureSession(info);
  }

  auto pCaptureSession = getSafeCaptureSession();
  int status = HAL_NO_INIT;
  if (pCaptureSession != 0) {
    status = pCaptureSession->configureStreams(requestedConfiguration,
                                               halConfiguration);
  }

  MY_LOGI("tryrun -");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::processCaptureRequest(
    const std::vector<CaptureRequest>& requests,
    /*const std::vector<BufferCache>& cachesToRemove, */
    uint32_t& numRequestProcessed)  // output
    -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
  CAM_TRACE_CALL();

  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    return pCaptureSession->processCaptureRequest(requests,
                                                  numRequestProcessed);
  }

  return HAL_NO_INIT;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::flush() -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("tryrun +");

  std::lock_guard<std::mutex> _lOpsLock(mOpsLock);

  if (!waitUntilOpenDone()) {
    MY_LOGE("open fail");
    return HAL_NO_INIT;
  }

  int handle = 0;
  if (mpCpuCtrl) {
    MY_LOGD("Enter CPU full run mode, timeout: %d ms", mCpuPerfTime);
    mpCpuCtrl->enterCPUBoost(mCpuPerfTime, &handle);
  }

  auto pCaptureSession = getSafeCaptureSession();
  int status = HAL_NO_INIT;
  if (pCaptureSession != 0) {
    status = pCaptureSession->flush();
  }

  MY_LOGI("tryrun -");
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::close() -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  MY_LOGD("tryrun +");

  std::lock_guard<std::mutex> _lOpsLock(mOpsLock);

  std::string const stateTag("-> close");
  mStateLog.add((stateTag + " +").c_str());

  if (!waitUntilOpenDone()) {
    MY_LOGE("open fail");
    return HAL_NO_INIT;
  }

  mCameraDeviceCallback.reset();

  int handle = 0;
  if (mpCpuCtrl) {
    MY_LOGD("Enter CPU full run mode, timeout: %d ms", mCpuPerfTime);
    mpCpuCtrl->enterCPUBoost(mCpuPerfTime, &handle);
  }

  int status = HAL_OK;

  auto pDeviceManager = mStaticInfo.mDeviceManager;
  auto const& instanceName = mStaticInfo.mStaticDeviceInfo->mInstanceName;

  status = pDeviceManager->startCloseDevice(instanceName);
  if (HAL_OK != status) {
    MY_LOGW("startCloseDevice [%d %s]", -status, ::strerror(-status));
  }

  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    pCaptureSession->close();
  }
  status = pDeviceManager->finishCloseDevice(instanceName);
  if (HAL_OK != status) {
    MY_LOGW("finishCloseDevice [%d %s]", -status, ::strerror(-status));
  }

  if (mpCpuCtrl) {
    MY_LOGD("Exit camera perf mode");
    mpCpuCtrl->exitCPUBoost(&handle);
    mpCpuCtrl->exitCameraPerf();
  }

  mDisplayIdleDelayUtil.disable();

  {
    std::lock_guard<std::mutex> _l(mCaptureSessionLock);
    mCaptureSession = nullptr;
  }

  mStateLog.add(
    (stateTag + " - " + (0 == status ? "OK" : ::strerror(-status))).c_str());

  MY_LOGI("tryrun -");
  return status;
}

/******************************************************************************
 * V3.5
 * signalStreamFlush:
 *
 * Signaling HAL camera service is about to perform configureStreams_3_5 and
 * HAL must return all buffers of designated streams. HAL must finish
 * inflight requests normally and return all buffers that belongs to the
 * designated streams through processCaptureResult or returnStreamBuffer
 * API in a timely manner, or camera service will run into a fatal error.
 *
 * Note that this call serves as an optional hint and camera service may
 * skip sending this call if all buffers are already returned.
 *
 * @param streamIds The ID of streams camera service need all of its
 *     buffers returned.
 *
 * @param streamConfigCounter Note that due to concurrency nature, it is
 *     possible the signalStreamFlush call arrives later than the
 *     corresponding configureStreams_3_5 call, HAL must check
 *     streamConfigCounter for such race condition. If the counter is less
 *     than the counter in the last configureStreams_3_5 call HAL last
 *     received, the call is stale and HAL should just return this call.
 ******************************************************************************/
auto ThisNamespace::signalStreamFlush(const std::vector<int32_t>& streamIds,
                                      uint32_t streamConfigCounter) -> void {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  std::lock_guard<std::mutex> _lOpsLock(mOpsLock);

  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    return pCaptureSession->signalStreamFlush(streamIds, streamConfigCounter);
  }

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::isReconfigurationRequired(
    const IMetadata& oldSessionParams,
    const IMetadata& newSessionParams,
    bool& reconfigurationNeeded) const  // output
    -> int {
  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    return pCaptureSession->isReconfigurationRequired(
        oldSessionParams, newSessionParams, reconfigurationNeeded);
  }
  return HAL_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::switchToOffline(
    const std::vector<int64_t>& streamsToKeep __unused,
    CameraOfflineSessionInfo& offlineSessionInfo __unused,
    std::shared_ptr<IMtkcamOfflineSession>& offlineSession __unused)
    -> int {
  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    return pCaptureSession->switchToOffline(streamsToKeep, offlineSessionInfo);
  }

  return HAL_NO_INIT;
}

#if 1
auto ThisNamespace::getConfigurationResults(
    const uint32_t streamConfigCounter,
    ExtConfigurationResults& ConfigurationResults) -> int {
  auto pCaptureSession = getSafeCaptureSession();
  if (pCaptureSession != 0) {
    return pCaptureSession->getConfigurationResults(streamConfigCounter,
                                                    ConfigurationResults);
  }

  return HAL_NO_INIT;
}
#endif
};  // namespace core
};  // namespace mcam
