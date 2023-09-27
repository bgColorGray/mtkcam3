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

#include <main/mtkhal/core/device/3.x/device/session/CaptureSessionBase.h>
// #include <main/mtkhal/core/device/3.x/app/AppStreamMgr.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/metastore/CaptureSessionParameters.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Misc.h>
#include <mtkcam/utils/std/ULog.h>  // will include <log/log.h> if LOG_TAG was defined
#include <mtkcam/utils/debug/Properties.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#include <future>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include "../MyUtils.h"

using mcam::core::HAL_DEAD_OBJECT;
using mcam::core::HAL_NAME_NOT_FOUND;
using mcam::core::HAL_NO_INIT;
using mcam::core::HAL_OK;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::REQ_APP_REQUEST;
using NSCam::Utils::ULog::ULogPrinter;
using mcam::core::device::policy::ConfigurationInputParams;
using mcam::core::device::policy::ConfigurationOutputParams;
using mcam::core::device::policy::DeviceSessionStaticInfo;
using mcam::core::device::policy::IDeviceSessionPolicyFactory;
using mcam::core::device::policy::PipelineSessionType;
using mcam::core::device::policy::PipelineUserConfiguration;
using mcam::core::device::policy::RequestInputParams;
using mcam::core::device::policy::RequestOutputParams;
using mcam::core::device::policy::ResultParams;
using NSCam::Utils::ULog::MOD_CAMERA_DEVICE;
using NSCam::Utils::ULog::REQ_APP_REQUEST;
using NSCam::Utils::ULog::ULogPrinter;
using NSCam::v3::pipeline::model::IPipelineModelManager;
using NSCam::v3::pipeline::model::IPipelineModel;
using NSCam::v3::pipeline::model::UserRequestParams;
using UserConfigurationParams =
    NSCam::v3::pipeline::model::UserConfigurationParams;
using mcam::core::Utils::AppImageStreamBufferProvider;
using mcam::core::Utils::IHalBufHandler;
using NSCam::CaptureSessionParameters;
using NSCam::eSTREAMID_META_APP_CONTROL;
using NSCam::Type2Type;
using NSCam::v3::eSTREAMTYPE_IMAGE_IN;
using NSCam::v3::IStreamInfo;
using NSCam::v3::STREAM_BUFFER_STATUS;
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
#define MY_LOGA(fmt, arg...)                                                 \
  CAM_ULOGM_ASSERT(0, "[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, \
                   ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_ULOGM_FATAL("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
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

/******************************************************************************
 *
 ******************************************************************************/
namespace {
struct TimeoutCaculator {
  nsecs_t const mStartTime;
  nsecs_t const mTimeout;

  explicit TimeoutCaculator(nsecs_t const t)
      : mStartTime(::systemTime()), mTimeout(t) {}

  nsecs_t timeoutToWait() const {
    nsecs_t const elapsedInterval = (::systemTime() - mStartTime);
    nsecs_t const timeoutToWait =
        (mTimeout > elapsedInterval) ? (mTimeout - elapsedInterval) : 0;
    return timeoutToWait;
  }
};
};  // namespace

namespace mcam {
namespace core {

#define ThisNamespace CaptureSessionBase
/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::~CaptureSessionBase() {
}

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::CaptureSessionBase(std::string const& name,
                                  CreationInfo const& info)
    : ICaptureSession(),
      mStaticInfo(info),
      mLogPrefix(name),
      mLogLevel(0),
      mStateLog(),
      mAppStreamManagerErrorState(std::make_shared<EventLogPrinter>(15, 25)),
      mAppStreamManagerWarningState(std::make_shared<EventLogPrinter>(25, 15)),
      mAppStreamManagerDebugState(std::make_shared<EventLogPrinter>()),
      mDeviceSessionPolicy(),
      mConfigTimestamp(0) {
  MY_LOGE_IF(mAppStreamManagerErrorState == nullptr,
             "Bad mAppStreamManagerErrorState");
  MY_LOGE_IF(mAppStreamManagerWarningState == nullptr,
             "Bad mAppStreamManagerWarningState");
  MY_LOGE_IF(mAppStreamManagerDebugState == nullptr,
             "Bad mAppStreamManagerDebugState");
  MY_LOGD("%p", this);

  mUseAppImageSBProvider =
      NSCam::Utils::Properties::property_get_int32(
                                "vendor.cam3dev.useappimagesbpvdr", 0);
  mUseLegacyAppStreamMgr =
      NSCam::Utils::Properties::property_get_int32(
                                "vendor.cam3dev.uselegacyappstreammgr", 0);
  MY_LOGD("mUseLegacyAppStreamMgr=%d", mUseLegacyAppStreamMgr);

  initialize();

  mStateLog.add("-> initialized");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getSafeAppStreamManager() const
    -> ::android::sp<IAppStreamManager> {
  //  Although mAppStreamManager is setup during opening camera,
  //  we're not sure any callback to this class will arrive
  //  between open and close calls.
  std::lock_guard<std::mutex> _l(mAppStreamManagerLock);
  return mAppStreamManager;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getSafeDeviceSessionPolicy() const
    -> std::shared_ptr<device::policy::IDeviceSessionPolicy> {
  std::lock_guard<std::mutex> _l(mDeviceSessionPolicyLock);
  return mDeviceSessionPolicy;
}

auto ThisNamespace::getSafeHalBufHandler() const
    -> ::android::sp<IHalBufHandler> {
  std::lock_guard<std::mutex> _l(mHalBufHandlerLock);
  return mHalBufHandler;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getSafePipelineModel() const
    -> ::android::sp<IPipelineModel> {
  std::lock_guard<std::mutex> _l(mPipelineModelLock);
  return mPipelineModel;
}

/******************************************************************************
 *
 ******************************************************************************/
/**
 * flush() must only return when there are no more outstanding buffers or
 * requests left in the HAL. The framework may call configure_streams (as
 * the HAL state is now quiesced) or may issue new requests.
 *
 * Performance requirements:
 *
 * The HAL should return from this call in 100ms, and must return from this
 * call in 1000ms. And this call must not be blocked longer than pipeline
 * latency (see S7 for definition).
 */
auto ThisNamespace::flushAndWait() -> int {
  CAM_ULOG_FUNCLIFE_GUARD(MOD_CAMERA_DEVICE);

  NSCam::Utils::CamProfile profile(__FUNCTION__, LOG_TAG);
  auto deviceName = getInstanceName();
  profile.print("+ %s", deviceName.c_str());
  //
  TimeoutCaculator toc(500000000);
  int err = HAL_OK;
  int Apperr = HAL_OK;
  nsecs_t timeout = 0;
  //
  do {
    auto pPipelineModel = getSafePipelineModel();
    if (pPipelineModel != 0) {
      err = pPipelineModel->beginFlush();
      MY_LOGW_IF(HAL_OK != err, "pPipelineModel->beginFlush err:%d(%s)", -err,
                 ::strerror(-err));
    }
    //
    // invoke HalBufHandler waitUntilDrained if exists
    //
    auto pAppStreamManager = getSafeAppStreamManager();
    Apperr = HAL_OK;
    if (pAppStreamManager != 0) {
      profile.print("waitUntilDrained +");
      Apperr = pAppStreamManager->waitUntilDrained(toc.timeoutToWait());
      profile.print("waitUntilDrained -");
      MY_LOGW_IF(HAL_OK != err, "AppStreamManager::waitUntilDrained err:%d(%s)",
                 -err, ::strerror(-err));
    }
    //
    if (pPipelineModel != 0) {
      pPipelineModel->endFlush();
    }
    timeout += 500000000;
  } while (HAL_OK != Apperr && timeout < getFlushAndWaitTimeout());
  //
  profile.print("-");
  return HAL_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::enableRequesting() -> void {
  std::lock_guard<std::mutex> _lRequesting(mRequestingLock);
  mRequestingAllowed.store(1, std::memory_order_relaxed);
  m1stRequestNotSent = true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::disableRequesting() -> void {
  std::lock_guard<std::mutex> _lRequesting(mRequestingLock);
  mRequestingAllowed.store(0, std::memory_order_relaxed);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::initialize() -> bool {
  MY_LOGD("+");
  mLogLevel = getCameraDevice3DebugLogLevel();

  const auto& entry =
      mStaticInfo.mMetadataProvider->getMtkStaticCharacteristics().entryFor(
          MTK_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION);
  if (entry.isEmpty()) {
    MY_LOGD("Disable HalBufHandler");
    mHalBufHandler = nullptr;
  }
  //
  mcam::core::Utils::CreationInfo creationInfo{
      .mInstanceId = getInstanceId(),
      .mCameraDeviceCallback = mStaticInfo.mCallback,
      .mMetadataProvider = mStaticInfo.mMetadataProvider,
      .mPhysicalMetadataProviders = mStaticInfo.mPhysicalMetadataProviders,
      .mMetadataConverter = mStaticInfo.mMetadataConverter,
      .mErrorPrinter =
          std::static_pointer_cast<IPrinter>(mAppStreamManagerErrorState),
      .mWarningPrinter =
          std::static_pointer_cast<IPrinter>(mAppStreamManagerWarningState),
      .mDebugPrinter =
          std::static_pointer_cast<IPrinter>(mAppStreamManagerDebugState),
      .mSupportBufferManagement = !entry.isEmpty(),
  };
  if (!IMetadata::getEntry<MINT64>(&mStaticInfo.sessionParams,
                                   MTK_HALCORE_CALLBACK_RULE,
                                   creationInfo.iRefineCbRule)) {
    // default setting apply all rules except metadata rule
    creationInfo.iRefineCbRule = -1;
  }
  //--------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> _l(mAppStreamManagerLock);
    if (mAppStreamManager != nullptr) {
      MY_LOGE("mAppStreamManager:%p != 0 while opening",
              mAppStreamManager.get());
      mAppStreamManager->destroy();
      mAppStreamManager = nullptr;
    }
    mAppStreamManager = IAppStreamManager::create(creationInfo);
    if (mAppStreamManager == nullptr) {
      mAppStreamManager = nullptr;
      MY_LOGE("IAppStreamManager::create");
      return HAL_NO_INIT;
    }
  }
  //--------------------------------------------------------------------------
  {
    ZoomRatioConverter::DimensionMap phyDimensionMap;
    ZoomRatioConverter::Dimension logicalDimension;
    auto staticMeta =
        mStaticInfo.mMetadataProvider->getMtkStaticCharacteristics();
    bool supportZoomRatio = true;

    IMetadata::IEntry ratioEntry =
        staticMeta.entryFor(MTK_CONTROL_ZOOM_RATIO_RANGE);
    if (ratioEntry.count() != 2) {
      MY_LOGD("invalid MTK_CONTROL_ZOOM_RATIO_RANGE count:%d",
              ratioEntry.count());
      supportZoomRatio = false;
    }

    MRect activeArray;
    if (!IMetadata::getEntry<MRect>(
            &staticMeta, MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION, activeArray)) {
      MY_LOGD("no MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION");
      supportZoomRatio = false;
    } else {
      logicalDimension.width = activeArray.s.w;
      logicalDimension.height = activeArray.s.h;

      MY_LOGD("get open id(%d) active array: w(%d), h(%d)", getInstanceId(),
              logicalDimension.width, logicalDimension.height);
    }

    // physical
    for (const auto& pair : mStaticInfo.mPhysicalMetadataProviders) {
      auto id = pair.first;
      auto pMetadataProvider_phy = pair.second;
      auto staticMeta = pMetadataProvider_phy->getMtkStaticCharacteristics();

      if (!IMetadata::getEntry<MRect>(
              &staticMeta, MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION, activeArray)) {
        MY_LOGE("No MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION found (phy)");
        continue;
      }

      MY_LOGD("get phy id(%d) active array: w(%d), h(%d)", id,
              (uint32_t)activeArray.s.w, (uint32_t)activeArray.s.h);

      ZoomRatioConverter::Dimension d = {(uint32_t)activeArray.s.w,
                                         (uint32_t)activeArray.s.h};
      phyDimensionMap.emplace(id, std::move(d));
    }

    mpZoomRatioConverter = ZoomRatioConverter::Create(
        supportZoomRatio, logicalDimension, phyDimensionMap);
  }
  //--------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> _l(mDeviceSessionPolicyLock);

    if (mDeviceSessionPolicy != nullptr) {
      MY_LOGE("mDeviceSessionPolicy:%p != 0 while opening",
              mDeviceSessionPolicy.get());
      mDeviceSessionPolicy->destroy();
      mDeviceSessionPolicy = nullptr;
    }

    auto staticInfo =
        std::make_shared<DeviceSessionStaticInfo>(DeviceSessionStaticInfo{
            .mInstanceId = getInstanceId(),
            .mSensorId = mStaticInfo.mSensorId,
            .mMetadataProvider = mStaticInfo.mMetadataProvider,
            .mPhysicalMetadataProviders =
                mStaticInfo.mPhysicalMetadataProviders,
            .mMetadataConverter = mStaticInfo.mMetadataConverter,
            .mErrorPrinter =
                std::static_pointer_cast<IPrinter>(mAppStreamManagerErrorState),
            .mWarningPrinter = std::static_pointer_cast<IPrinter>(
                mAppStreamManagerWarningState),
            .mDebugPrinter =
                std::static_pointer_cast<IPrinter>(mAppStreamManagerDebugState),
            .mPolicyStaticInfo = mStaticInfo.mPolicyStaticInfo,
            .mZoomRatioConverter = mpZoomRatioConverter,
        });

    mDeviceSessionPolicy =
        IDeviceSessionPolicyFactory::createDeviceSessionPolicyInstance(
            IDeviceSessionPolicyFactory::CreationParams{
                .mStaticInfo = staticInfo,
            });
    if (MY_UNLIKELY(mDeviceSessionPolicy == nullptr)) {
      MY_LOGE("IDeviceSessionPolicy::createDeviceSessionPolicyInstance");
      return HAL_NO_INIT;
    }
  }
  //--------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> _l(mHalBufHandlerLock);

    ::android::sp<IHalBufHandler> pHalBufHandler = nullptr;
    if (!creationInfo.mSupportBufferManagement) {
      MY_LOGD("Disable HalBufHandler");
    } else {
      MY_LOGD("Enable HalBufHandler, version 0x%x",
              (int)entry.itemAt(0, Type2Type<MUINT8>()));
      pHalBufHandler = IHalBufHandler::create(
          creationInfo, mAppStreamManager /*, mAppRequestUtil*/);
      if (pHalBufHandler == nullptr) {
        MY_LOGE("IHalBufHandler::create");
        return HAL_NO_INIT;
      }
    }
    //--------------------------------------------------------------------------
    mAppImageSBProvider = AppImageStreamBufferProvider::create(pHalBufHandler);
    if (mAppImageSBProvider == nullptr) {
      MY_LOGE("AppImageStreamBufferProvider::create");
      return HAL_NO_INIT;
    }
  }
  //--------------------------------------------------------------------------
//  createFrameBufferManager(&mFrameBufferManager, getLogPrefix().c_str());

  MY_LOGD("-");
  return HAL_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
waitUntilOpenDoneLocked() -> bool
{
    auto pPipelineModel = getSafePipelineModel();
    if  ( pPipelineModel != nullptr ) {
        return pPipelineModel->waitUntilOpenDone();
    }
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onCloseLocked() -> void {
  MY_LOGD("+");
  NSCam::Utils::CamProfile profile(__FUNCTION__, getInstanceName().c_str());

  if  ( ! waitUntilOpenDoneLocked() ) {
      MY_LOGE("open fail");
  }

  {
    disableRequesting();
    flushAndWait();
    profile.print("flush -");
  }
  //
  {
    std::lock_guard<std::mutex> _l(mPipelineModelLock);
    if (mPipelineModel != nullptr) {
      mPipelineModel->close();
      mPipelineModel = nullptr;
      profile.print("PipelineModel -");
    }
  }
  //
  {
    std::lock_guard<std::mutex> _l(mDeviceSessionPolicyLock);
    if (mDeviceSessionPolicy != nullptr) {
      mDeviceSessionPolicy->destroy();
      mDeviceSessionPolicy = nullptr;
      profile.print("mDeviceSessionPolicy -");
    }
  }
  //
  {
    std::lock_guard<std::mutex> _l(mAppStreamManagerLock);
    if (mAppStreamManager != nullptr) {
      mAppStreamManager->destroy();
      mAppStreamManager = nullptr;
      profile.print("AppStreamManager -");
    }
  }
  //
  {
    std::lock_guard<std::mutex> _l(mHalBufHandlerLock);
    if (mHalBufHandler != nullptr) {
      mHalBufHandler->destroy();
      mHalBufHandler = nullptr;
      profile.print("HalBufHandler -");
    }
  }
  //
  {
    std::lock_guard<std::mutex> _l(mImageConfigMapLock);
    mImageConfigMap.clear();
    mMetaConfigMap.clear();
  }

  //--------------------------------------------------------------------------
  profile.print("");
  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onConfigureStreamsLocked(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration)  // output
    -> int {
  MY_LOGD("+");
  CAM_ULOG_FUNCLIFE_GUARD(MOD_CAMERA_DEVICE);

  int err = HAL_OK;

  bool isCustomHalFlow = false;

  {
    disableRequesting();
    flushAndWait();
  }

  // dump input
  for (auto const& stream : requestedConfiguration.streams) {
    MY_LOGI("%s", toString(stream).c_str());
  }
  //
  auto pAppStreamManager = getSafeAppStreamManager();
  if (pAppStreamManager == 0) {
    MY_LOGE("Bad AppStreamManager");
    return HAL_NO_INIT;
  }
  pAppStreamManager->beginConfiguration(requestedConfiguration);
  //
  auto pParams = std::make_shared<PipelineUserConfiguration>();
  if (!pParams) {
    MY_LOGE("Bad PipelineUserConfiguration");
    return HAL_NO_INIT;
  }
  {
    mConfigTimestamp = (uint64_t)::systemTime();
    MY_LOGD("timestamp(%" PRIu64 ")", mConfigTimestamp);

    err = prepareUserConfigurationParamsLocked(
        mConfigTimestamp, requestedConfiguration, halConfiguration, pParams);
    if (HAL_OK != err) {
      MY_LOGE("fail to prepareUserConfigurationParamsLocked");
      return err;
    }
    //--------------------------------------------------------------------------
    {
      std::lock_guard<std::mutex> _l(mPipelineModelLock);
      auto pPipelineModelMgr = IPipelineModelManager::get();
      if  ( MY_UNLIKELY(pPipelineModelMgr == nullptr) ) {
          MY_LOGE("IPipelineModelManager::get() is null object!");
          return HAL_NO_INIT;
      }
      //
      auto pPipelineModel = pPipelineModelMgr->getPipelineModel(getInstanceId());
      if ( MY_UNLIKELY(pPipelineModel == nullptr) ) {
        MY_LOGE("IPipelineModelManager::getPipelineModel(%d) is null object!", getInstanceId());
        return HAL_NO_INIT;
      }
      //
      err = pPipelineModel->open(getInstanceName(), this);
      if  ( MY_UNLIKELY(HAL_OK != err) ) {
        MY_LOGE( "fail to IPipelinemodel->open() status:%d(%s)", -err, ::strerror(-err) );
        return HAL_NO_INIT;
      }
      mPipelineModel = pPipelineModel;
      //
      auto pUserCfgParams = std::make_shared<UserConfigurationParams>();
      if (!pUserCfgParams) {
        MY_LOGE("Bad UserConfigurationParams");
        return HAL_NO_INIT;
      }
      auto& pAppUserConfiguration = pParams->pAppUserConfiguration;
      pUserCfgParams->operationMode = pAppUserConfiguration->operationMode;
      pUserCfgParams->sessionParams = pAppUserConfiguration->sessionParams;
      pUserCfgParams->vImageStreams = pAppUserConfiguration->vImageStreams;
      pUserCfgParams->vMetaStreams = pAppUserConfiguration->vMetaStreams;
      pUserCfgParams->vMetaStreams_physical = pAppUserConfiguration->vMetaStreams_physical;
      pUserCfgParams->vMinFrameDuration = pAppUserConfiguration->vMinFrameDuration;
      pUserCfgParams->vStallFrameDuration = pAppUserConfiguration->vStallFrameDuration;
      pUserCfgParams->vPhysicCameras = pAppUserConfiguration->vPhysicCameras;
      pUserCfgParams->pStreamInfoBuilderFactory = pAppUserConfiguration->pStreamInfoBuilderFactory;
      pUserCfgParams->pImageStreamBufferProvider = mAppImageSBProvider;
      pUserCfgParams->configTimestamp = mConfigTimestamp;
      if  (!pPipelineModel->waitUntilOpenDone()) {
        MY_LOGE("open fail");
        return HAL_NO_INIT;
      }
      err = pPipelineModel->configure(pUserCfgParams);
      if  ( HAL_OK != err ) {
          MY_LOGE("fail to configure pipeline");
          return err;
      }
    }
    //--------------------------------------------------------------------------
  }

  {
    auto const& configParams =
      pParams->pAppUserConfiguration->sessionParams;
//    IMetadata::IEntry const& e1 =
//          configParams.entryFor(MTK_HAL3PLUS_STREAM_SETTINGS);
//    if (!e1.isEmpty()) {
//      MY_LOGI("find stream setting hal3+");
//      isCustomHalFlow = true;
//    }
  }

  // Set maximum buffer size in HalStream
  auto& vImageStreams =
      pParams->pAppUserConfiguration->vImageStreams;
  for (size_t i = 0; i < vImageStreams.size(); i++) {
    auto& halStream = halConfiguration.streams[i];
    auto& halMaxBufferSize = halStream.maxBuffers;
    StreamId_T const streamId = halStream.id;

    //  Hal3+ modify
    auto get_iter = vImageStreams.end();
    for (auto iter = vImageStreams.begin(); iter != vImageStreams.end();
         ++iter) {
      if ((iter->first & 0x00000000ffffffff) == streamId) {
        get_iter = iter;
      }
    }
    if (get_iter == vImageStreams.end()) {
      MY_LOGE("invalid streamId %" PRId64 "", streamId);
      return HAL_NAME_NOT_FOUND;
    }
    auto& halStreamInfo = get_iter->second;

    halMaxBufferSize = halStreamInfo->getMaxBufNum();
    MY_LOGD("[debug] halMaxBufferSize=%d HalStream.maxBuffers=%d",
            halMaxBufferSize, halStream.maxBuffers);
  }
  //
  pAppStreamManager->endConfiguration(halConfiguration);

  // configure
  // auto pAppRequestUtil = getSafeAppRequestUtil();
  // if  ( pAppRequestUtil == 0 ) {
  //     MY_LOGE("Bad AppRequestUtil");
  //     return HAL_NO_INIT;
  // }
  // pAppRequestUtil->setConfigMap(mImageConfigMap, mMetaConfigMap);
  //
  // auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
  // if  ( pBufferHandleCacheMgr == 0 ) {
  //     MY_LOGE("Bad BufferHandleCacheMgr");
  //     return HAL_NO_INIT;
  // }
  // pBufferHandleCacheMgr->createBufferCacheMap(requestedConfiguration);
  //
  enableRequesting();
  onEndConfiguration();
  MY_LOGD("-");
  return HAL_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onFlushLocked() -> int {
  MY_LOGD("");

  int status = HAL_OK;
  if  ( ! waitUntilOpenDoneLocked() ) {
      return HAL_NO_INIT;
  }
  disableRequesting();
  //
  status = flushAndWait();
  //
  enableRequesting();
  //
  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::prepareUserConfigurationParamsLocked(
    const uint64_t& configureTimeStamp,
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration,
    std::shared_ptr<PipelineUserConfiguration>& rCfgParams)
    -> int {
  int status = HAL_OK;
  // vImageStreams
  // vMetaStreams
  // vMetaStreams_physical
  // vMinFrameDuration
  // vStallFrameDuration
  // vPhysicCameras
  ConfigurationInputParams const in{
      .requestedConfiguration = &requestedConfiguration,
      .mConfigTimestamp = configureTimeStamp,
  };
  ConfigurationOutputParams out{
      .resultConfiguration = &halConfiguration,
      .pPipelineUserConfiguration = &rCfgParams,
      .imageConfigMap = &mImageConfigMap,
      .metaConfigMap = &mMetaConfigMap,
  };
  auto pDeviceSessionPolicy = getSafeDeviceSessionPolicy();
  if (MY_UNLIKELY(pDeviceSessionPolicy == nullptr)) {
    MY_LOGE("Bad DeviceSessionPolicy");
    return -ENODEV;
  }
  status = pDeviceSessionPolicy->evaluateConfiguration(&in, &out);

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getConfigImageStream(StreamId_T streamId) const
    -> android::sp<AppImageStreamInfo> {
  std::lock_guard<std::mutex> _l(mImageConfigMapLock);
  ssize_t const index = mImageConfigMap.indexOfKey(streamId);
  if (0 <= index) {
    return mImageConfigMap.valueAt(index) /*.pStreamInfo*/;
  }
  return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::checkRequestParams(
    const std::vector<CaptureRequest>& vRequests) -> int {
  int status = HAL_OK;

  MY_LOGF("Not implement yet!");
  return status;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NSCam::ICaptureSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer,
                              const std::vector<std::string>& options __unused)
    -> void {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  printer.printLine(
      "\n== state transition (most recent at bottom): "
      "Camera CaptureSession ==");
  mStateLog.print(printer);

  printer.printLine(
      "\n== error state (most recent at bottom): App Stream Manager ==");
  mAppStreamManagerErrorState->print(printer);

  printer.printLine(
      "\n== warning state (most recent at bottom): App Stream Manager ==");
  mAppStreamManagerWarningState->print(printer);

  printer.printLine(
      "\n== debug state (most recent at bottom): App Stream Manager ==");
  mAppStreamManagerDebugState->print(printer);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraDeviceSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::configureStreams(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration)  // output
    -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
  CAM_TRACE_CALL();

  int status = HAL_OK;
  std::ostringstream stateTag;
  stateTag << "-> configure (operationMode:"
         << std::hex << std::showbase
         << static_cast<uint32_t>(requestedConfiguration.operationMode) << ")";
  mStateLog.add((stateTag.str() + " +").c_str());

  // WrappedHalStreamConfiguration halStreamConfiguration;
  {
    std::lock_guard<std::mutex> _lOpsLock(mOpsLock);

    {  // update stream config counter
      std::lock_guard<std::mutex> _l(mStreamConfigCounterLock);
      mStreamConfigCounter = requestedConfiguration.streamConfigCounter;
    }

    status = onConfigureStreamsLocked(requestedConfiguration, halConfiguration);
    for (int i = 0; i < halConfiguration.streams.size(); ++i) {
      MY_LOGD("halstream.maxBuffers(%u)",
              halConfiguration.streams[i].maxBuffers);
    }
  }

  mStateLog.add((stateTag.str() + " - " +
                (0 == status ? "OK" : ::strerror(-status))).c_str());

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onProcessCaptureRequest(
    const std::vector<CaptureRequest>& requests,
    uint32_t& numRequestProcessed) -> int {
#define _UlogDiscard_(reqs)                                      \
  {                                                              \
    for (size_t i = 0; i < reqs.size(); i++) {                   \
      CAM_ULOG_DISCARD_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, \
                             reqs[i].frameNumber);               \
    }                                                            \
  }
  size_t const requestNum = requests.size();
  uint32_t const requestNo_1st = requests[0].frameNumber;

#define REQUEST_TIMEOUT_MS 2500

  for (auto& r : requests) {
    CAM_ULOG_ENTER_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, r.frameNumber,
                         REQUEST_TIMEOUT_MS);
  }

  int err = HAL_OK;

  ::android::Vector<Request> appRequests;

  auto pAppStreamManager = getSafeAppStreamManager();
  if (pAppStreamManager == 0) {
    MY_LOGE("Bad AppStreamManager");
    _UlogDiscard_(requests);
    return HAL_DEAD_OBJECT;
  }
  {
    MY_LOGD("[debug] Start request");
    // step1-2, should be moved to HIDL adapter part
    std::lock_guard<std::mutex> _lRequesting(mRequestingLock);

    auto pDeviceSessionPolicy = getSafeDeviceSessionPolicy();
    if (MY_UNLIKELY(pDeviceSessionPolicy == nullptr)) {
      MY_LOGE(
          "Bad DeviceSessionPolicy, might caused latest request metadata cache "
          "fails");
      _UlogDiscard_(requests);
      return HAL_DEAD_OBJECT;
    }

    RequestInputParams const in{
        .requests = &requests,
    };
    RequestOutputParams out{
        .requests = &appRequests,
    };
    err = pDeviceSessionPolicy->evaluateRequest(&in, &out);
    //
    if (HAL_OK != err) {
      MY_LOGE("Create Requests failed");
      _UlogDiscard_(requests);
      return err;
    }
    //
    MY_LOGD("[debug] createRequests done");
    // 3. handle flush case
    if (0 == mRequestingAllowed.load(std::memory_order_relaxed)) {
      MY_LOGW("submitting requests during flushing - requestNo_1st:%u #:%zu",
              requestNo_1st, requestNum);
      //
      // AppStreamMgr handle flushed requests
      pAppStreamManager->flushRequest(requests);
      numRequestProcessed = requests.size();
      return HAL_OK;
    }
  }

  auto pPipelineModel = getSafePipelineModel();
  if (pPipelineModel == 0) {
    MY_LOGE("Bad PipelineModel");
    _UlogDiscard_(requests);
    return HAL_NO_INIT;
  }

  // 4. prepare request params for AppStreamMgr to appRequests
  std::vector<std::shared_ptr<UserRequestParams>> vPipelineRequests(appRequests.size());
  err = pAppStreamManager->submitRequests(requests);
  if (HAL_OK != err) {
    _UlogDiscard_(requests);
    return err;
  }
  //
  MY_LOGD("[debug] submitRequests done");
  if (appRequests.size() > 0) {
    for (auto& appRequest : appRequests) {
      auto ctrlMeta =
          appRequest.vInputMetaBuffers.valueFor(eSTREAMID_META_APP_CONTROL);
      IMetadata* pAppInMeta = ctrlMeta->tryReadLock(LOG_TAG);
      // Update ULOG timeout for long exposure requests
      if (pAppInMeta) {
        MINT64 exposureTime = 0;
        if (IMetadata::getEntry<MINT64>(pAppInMeta, MTK_SENSOR_EXPOSURE_TIME,
                                        exposureTime)) {
          exposureTime /= 1000000L;
          exposureTime += 1000;  // Add some time for P2
          if (exposureTime > REQUEST_TIMEOUT_MS) {
            MY_LOGD("Extend timeout to %" PRId64 " ms for long exposure time",
                    exposureTime);
            CAM_ULOG_MODIFY_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST,
                                  appRequest.requestNo, exposureTime);
          }
        } else {
          MY_LOGD("Ignore to refer exposure time from app contrl meta");
        }
        ctrlMeta->unlock(LOG_TAG, pAppInMeta);
      }
    }
  }

#define _CLONE_(dst, src) \
            do  { \
                dst.clear(); \
                for (size_t j = 0; j < src.size(); j++) { \
                    dst.emplace( std::make_pair(src.keyAt(j), src.valueAt(j) ) ); \
                } \
            } while (0) \

    for ( size_t i=0; i<appRequests.size(); ++i ) {
        auto& pItem = vPipelineRequests[i];
        pItem = std::make_shared<UserRequestParams>();
        if ( !pItem ) {
            MY_LOGE("Bad UserRequestParams");
            _UlogDiscard_(requests);
            return HAL_NO_INIT;
        }
        pItem->requestNo = appRequests[i].requestNo;
        if ( !mUseAppImageSBProvider ) {
            _CLONE_(pItem->vIImageBuffers,    appRequests[i].vInputImageBuffers);
            _CLONE_(pItem->vOImageBuffers,    appRequests[i].vOutputImageBuffers);
        } else {
            mAppImageSBProvider->storeStreamBuffers(
                appRequests[i].requestNo,
                appRequests[i].vInputImageBuffers,
                appRequests[i].vOutputImageBuffers
            );
        }
        // _CLONE_(pItem->vOImageStreams,    appRequests[i].vOutputImageStreams);
        _CLONE_(pItem->vIMetaBuffers,     appRequests[i].vInputMetaBuffers);

        if(mpZoomRatioConverter)
            mpZoomRatioConverter->UpdateCaptureRequestMeta(pItem->requestNo, pItem->vIMetaBuffers);
    }
#undef  _CLONE_
    //
    //  Since this call may block, it should be performed out of locking.
    err = pPipelineModel->submitRequest(vPipelineRequests, numRequestProcessed);
    if  ( HAL_OK != err || requests.size() != numRequestProcessed ) {
        MY_LOGE("%u/%zu requests submitted sucessfully - err:%d(%s)",
            numRequestProcessed, vPipelineRequests.size(), -err, ::strerror(-err));
        pAppStreamManager->abortRequest(appRequests);
        numRequestProcessed = requests.size();
        _UlogDiscard_(requests);
        return HAL_OK;
    }
#undef _UlogDiscard_
    //
    return HAL_OK;
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
  auto status = onProcessCaptureRequest(requests, numRequestProcessed);
  MY_LOGD_IF(getLogLevel() >= 2,
             "- requestNo_1st:%u #:%zu numRequestProcessed:%u",
             requests[0].frameNumber, requests.size(), numRequestProcessed);

  if (m1stRequestNotSent) {
    if (HAL_OK == status) {
      m1stRequestNotSent = false;
      mStateLog.add("-> 1st request - OK");
      MY_LOGD("-> 1st request - OK");
    } else {
      mStateLog.add("-> 1st request - failure");
      MY_LOGE("-> 1st request - failure");
    }
  }

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::flush() -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  int status = HAL_OK;
  std::string const stateTag("-> flush");
  mStateLog.add((stateTag + " +").c_str());
  {
    std::lock_guard<std::mutex> _lOpsLock(mOpsLock);
    status = onFlushLocked();
  }
  mStateLog.add(
    (stateTag + " - " + (0 == status ? "OK" : ::strerror(-status))).c_str());

  return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::close() -> int {
  CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

  std::lock_guard<std::mutex> _lOpsLock(mOpsLock);

  onCloseLocked();

  return HAL_OK;
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

  MY_LOGD("+ streamConfigCounter %d", streamConfigCounter);
  for (auto streamId : streamIds) {
    MY_LOGD("streamId %d", (int)streamId);
  }

  {  // check stream config counter valid
    std::lock_guard<std::mutex> _l(mStreamConfigCounterLock);
    if (streamConfigCounter < mStreamConfigCounter) {
      MY_LOGD(
          "streamConfigCounter %d is stale (current %d), skipping "
          "signal_stream_flush call",
          streamConfigCounter, mStreamConfigCounter);
      return;
    }
  }

  auto status = onFlushLocked();

  MY_LOGD("- %s", (0 == status ? "OK" : ::strerror(-status)));

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::isReconfigurationRequired(
    const IMetadata& oldSessionParams,
    const IMetadata& newSessionParams,
    bool& reconfigurationNeeded)  // output
    -> int {
  if (!CaptureSessionParameters::isReconfigurationRequired(oldSessionParams,
                                                           newSessionParams)) {
    reconfigurationNeeded = false;
  }
  return HAL_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::switchToOffline(
    const std::vector<int64_t>& streamsToKeep __unused,
    CameraOfflineSessionInfo& offlineSessionInfo __unused)
    // ICameraOfflineSession& offlineSession
    -> int {
  return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineModelCallback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onUpdateResult(UpdateResultParams& params) -> void {
  auto pAppStreamManager = getSafeAppStreamManager();
  if (pAppStreamManager == 0) {
    MY_LOGE("Bad AppStreamManager");
    return;
  }
  pAppStreamManager->updateResult(params);
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::onFrameUpdated(
    NSCam::v3::pipeline::model::UserOnFrameUpdated const& params) -> void {
    CAM_TRACE_CALL();

    String8 const postfix = String8::format("requestNo:%u userId:%#" PRIxPTR " OAppMeta#(left:%zd this:%zu)",
        params.requestNo, params.userId, params.nOutMetaLeft, params.vOutMeta.size());

    MY_LOGD_IF(getLogLevel() >= 2, "+ %s", postfix.string());
    NSCam::Utils::CamProfile profile(__FUNCTION__, "CameraDevice3SessionImpl");

    auto pAppStreamManager = getSafeAppStreamManager();
    if  ( pAppStreamManager == 0 ) {
        MY_LOGE("Bad AppStreamManager");
        return;
    }
    profile.print_overtime(1, "getSafeAppStreamManager: %s", postfix.string());

    /*IAppStreamManager::*/UpdateResultParams tempParams = {
        .requestNo      = params.requestNo,
        .userId         = params.userId,
        .hasLastPartial = params.nOutMetaLeft<=0,
        .hasRealTimePartial = params.isRealTimeUpdate,
    };

    // shutter timestamp
    if ( params.timestampStartOfFrame != 0 ){
      int64_t shutterTimestamp = 0;
      bool hit = false;
      for ( auto& pStreamBuffer : params.vOutMeta) {
        IMetadata* pMetadata = pStreamBuffer->tryReadLock(LOG_TAG);
        IMetadata::IEntry const entry = pMetadata->entryFor(MTK_SENSOR_TIMESTAMP);
        pStreamBuffer->unlock(LOG_TAG, pMetadata);
        if (!entry.isEmpty()) {
           shutterTimestamp = entry.itemAt(0, Type2Type<MINT64>());
           hit = true;
           break;
        }
      }
      if (!hit) {
        shutterTimestamp = params.timestampStartOfFrame;
        MY_LOGW("Can't get MTK_SENSOR_TIMESTAMP, use timestampStartOfFrame");
      }
      UpdateResultParams requestStartedParams = {
        .requestNo = params.requestNo,
        .userId = 0,
        .shutterTimestamp = shutterTimestamp,
        .timestampStartOfFrame = params.timestampStartOfFrame,
      };
      onUpdateResult(requestStartedParams);
    }

    bool hasResultMetadataFailed = false;
    tempParams.resultMeta.setCapacity(params.vOutMeta.size());
    for ( size_t i=0; i<params.vOutMeta.size(); ++i ) {
        tempParams.resultMeta.add(params.vOutMeta[i]);
        if (params.vOutMeta[i]->hasStatus(STREAM_BUFFER_STATUS::ERROR))
        hasResultMetadataFailed = true;
    }

    if ( params.activePhysicalId >= 0 &&
        tempParams.resultMeta.size() > 0)
    {
        std::string idString = std::to_string(params.activePhysicalId);

        IMetadata::IEntry entry(MTK_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID);
        for(auto&& c : idString)
        {
            entry.push_back(c , Type2Type< MUINT8 >());
        }
        entry.push_back('\0' , Type2Type< MUINT8 >());

        if ( IMetadata* pMetadata = tempParams.resultMeta[0]->tryWriteLock(LOG_TAG) ) {
            pMetadata->update(entry.tag(), entry);
            tempParams.resultMeta[0]->unlock(LOG_TAG, pMetadata);
        }
        MY_LOGD_IF(getLogLevel() >= 2, "Set MTK_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID to %d", params.activePhysicalId);
    }

    bool isPhysicalMetaError = false;
    //Update physical meta if needed
    if (params.vPhysicalOutMeta.size() != 0){
        tempParams.physicalResult.setCapacity(params.vPhysicalOutMeta.size());
        for ( size_t i = 0; i < params.vPhysicalOutMeta.size(); ++i ) {
            sp<PhysicalResult> physicRes = new PhysicalResult;
            char camId[50];
            if(sprintf(camId, "%d", params.vPhysicalOutMeta.keyAt(i)) >= 0)
            {
                physicRes->physicalCameraId = std::string(camId);
                physicRes->physicalResultMeta = params.vPhysicalOutMeta.valueAt(i);
                tempParams.physicalResult.add(physicRes);
                if (physicRes->physicalResultMeta->hasStatus(STREAM_BUFFER_STATUS::ERROR)) {
                    // temp variable to igonre physical meta error
                    // hasResultMetadataFailed = true;
                }
            }
        }
    }
    ResultParams resultParams {
        .params                 = &tempParams,
    };
    auto pDeviceSessionPolicy = getSafeDeviceSessionPolicy();
    if ( CC_UNLIKELY(pDeviceSessionPolicy==nullptr) ) {
        MY_LOGE("Bad DeviceSessionPolicy");
        return;
    }
    pDeviceSessionPolicy->processResult(&resultParams);

    if(mpZoomRatioConverter)
        mpZoomRatioConverter->UpdateCaptureResultMeta(tempParams);

    onUpdateResult(tempParams);

    // [HAL3+][onResultMetadataFailed]
    if (hasResultMetadataFailed && !mUseLegacyAppStreamMgr) {
      UpdateResultParams resultErrorParams = {
          .requestNo = params.requestNo,
          .userId = 4660,  // (1234 in hex)
          .hasResultMetaError = true,
      };
      onUpdateResult(resultErrorParams);
    }

    profile.print_overtime(1, "updateResult: %s", postfix.string());
    MY_LOGD_IF(getLogLevel() >= 2, "- %s", postfix.string());
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onMetaResultAvailable(
    NSCam::v3::pipeline::model::UserOnMetaResultAvailable&& arg __unused
) -> void
{
    MY_LOGE("Not Implement");
}
/******************************************************************************
 * onImageBufferReleased
 * This method is called when some (input or output) image buffers are released.
 *
 * When acquiring image stream buffers from the image buffer provider (set on configure()),
 * this method must be used for the client to check a buffer's status.
 ******************************************************************************/
void
ThisNamespace::
onImageBufferReleased(NSCam::v3::pipeline::model::UserOnImageBufferReleased&& arg)
{
  for(auto &r : arg.results)
  {
    UpdateResultParams imageDoneParams = {
      .requestNo = arg.requestNo,
      .userId = 120,
    };
    {
      std::lock_guard<std::mutex> _l(mImageConfigMapLock);
      StreamId_T streamId = r.streamId;
      ssize_t index = mImageConfigMap.indexOfKey(streamId);
      if (index >= 0) {
        if (mImageConfigMap.valueAt(index)->getStreamType() ==
          eSTREAMTYPE_IMAGE_IN) {
          imageDoneParams.inputResultImages.setCapacity(1);
          imageDoneParams.inputResultImages.add(
              StreamBuffer{.streamId = static_cast<int32_t>(streamId),
                           .status = r.status & STREAM_BUFFER_STATUS::ERROR
                                         ? BufferStatus::ERROR
                                         : BufferStatus::OK});
        } else {
          imageDoneParams.outputResultImages.setCapacity(1);
          imageDoneParams.outputResultImages.add(
              StreamBuffer{.streamId = static_cast<int32_t>(streamId),
                           .status = r.status & STREAM_BUFFER_STATUS::ERROR
                                         ? BufferStatus::ERROR
                                         : BufferStatus::OK});
        }
      }
    }
    onUpdateResult(imageDoneParams);
  }
}

/******************************************************************************
 * This method is called when a request has fully completed no matter whether
 * the processing for that request succeeds or not.
 ******************************************************************************/
void
ThisNamespace::
onRequestCompleted(NSCam::v3::pipeline::model::UserOnRequestCompleted&& arg __unused)
{

}

/******************************************************************************
 *
 ******************************************************************************/
//auto ThisNamespace::onResultMetadataAvailable(
//    NSCam::v3::pipeline::model::OnResultMetadataAvailable&& arg __unused)
//    -> void {}

auto ThisNamespace::getConfigurationResults(
    const uint32_t streamConfigCounter,
    mcam::ExtConfigurationResults& ConfigurationResults) -> int {
#if 0
  {  // check stream config counter valid
    std::lock_guard<std::mutex> _l(mStreamConfigCounterLock);
    if (streamConfigCounter < mStreamConfigCounter) {
      MY_LOGE(
          "streamConfigCounter %d is stale (current %d), skipping "
          "getConfigurationResults call",
          streamConfigCounter, mStreamConfigCounter);
      return -EINVAL;
    }
  }

  {  // update configuration results
    std::lock_guard<std::mutex> _l(mImageConfigMapLock);
    for (size_t i = 0; i < mImageConfigMap.size(); i++) {
      IImageStreamInfo::BufPlanes_t BufPlanes =
          mImageConfigMap.valueAt(i)->getAllocBufPlanes();

      for (auto index = 0; index < BufPlanes.count; index++) {
        if (BufPlanes.planes[index].sizeInBytes > 0) {
          MY_LOGD_IF(getLogLevel() >= 2,
                     "BufPlane %zu: sizeInBytes: %zu, rowStrideInBytes: %zu",
                     index, BufPlanes.planes[index].sizeInBytes,
                     BufPlanes.planes[index].rowStrideInBytes);

          ConfigurationResults.streamResults[mImageConfigMap.keyAt(i)]
              .push_back(index);  // plane index
          ConfigurationResults.streamResults[mImageConfigMap.keyAt(i)]
              .push_back(static_cast<int64_t>(
                  BufPlanes.planes[index].rowStrideInBytes));  // width
          ConfigurationResults.streamResults[mImageConfigMap.keyAt(i)]
              .push_back(static_cast<int64_t>(
                  BufPlanes.planes[index].sizeInBytes /
                  BufPlanes.planes[index].rowStrideInBytes));  // high
          ConfigurationResults.streamResults[mImageConfigMap.keyAt(i)]
              .push_back(static_cast<int64_t>(
                  BufPlanes.planes[index].rowStrideInBytes));  // stride
        }
      }
    }

    for (auto& result : ConfigurationResults.streamResults) {
      MY_LOGD_IF(getLogLevel() >= 2, "stream id: %d", result.first);
      for (auto i = 0; i < result.second.size(); i += 4) {
        MY_LOGD_IF(getLogLevel() >= 2, "plan id: %d, w: %d, h: %d, stride: %d",
                   result.second[i], result.second[i + 1], result.second[i + 2],
                   result.second[i + 3]);
      }
    }
    return HAL_OK;
  }
#endif
  return HAL_NO_INIT;
}

};  // namespace core
};  // namespace mcam
