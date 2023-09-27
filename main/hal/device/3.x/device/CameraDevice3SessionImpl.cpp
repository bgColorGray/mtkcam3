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

#include "CameraDevice3SessionImpl.h"
#include "MyUtils.h"
#include <cutils/properties.h>
#include <mtkcam/utils/std/ULog.h> // will include <log/log.h> if LOG_TAG was defined
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metastore/CaptureSessionParameters.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam/utils/std/Misc.h>

#include <set>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
//
using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::pipeline::model;
using namespace NSCam::v3::device::policy;
using namespace NSCam::Utils;
using namespace NSCam::Utils::ULog;

#define ThisNamespace   CameraDevice3SessionImpl

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
 *
 ******************************************************************************/
namespace {
struct TimeoutCaculator
{
    nsecs_t const mStartTime;
    nsecs_t const mTimeout;

            TimeoutCaculator(nsecs_t const t)
                : mStartTime(::systemTime())
                , mTimeout(t)
            {}

    nsecs_t timeoutToWait() const
            {
                nsecs_t const elapsedInterval = (::systemTime() - mStartTime);
                nsecs_t const timeoutToWait = (mTimeout > elapsedInterval)
                                            ? (mTimeout - elapsedInterval)
                                            :   0
                                            ;
                return timeoutToWait;
            }
};

struct ClosingThread
{
 private:
  static bool closePipelineModel(const ::android::sp<IPipelineModel> pPipelineModel) {
    if  ( pPipelineModel != nullptr ) {
      pPipelineModel->close();
    }
    return true;
  }

 public:
  int32_t               mInstanceId;
  std::set<MINT32>      mSensorIds;
  std::future<bool>     mFuture;

  ClosingThread(const int32_t instanceId,
                const std::vector<MINT32>& sensorIds,
                const ::android::sp<IPipelineModel> pPipelineModel)
      : mInstanceId(instanceId),
        mSensorIds(),
        mFuture(std::async(closePipelineModel, pPipelineModel)) {
    for (auto& id : sensorIds) {
        mSensorIds.emplace(id);
    }
  }

  /**
   * @brief
   *    If the sensor id is conflicted,
   *    the open thread should wait until close done.
   *
   * @param sensorIds
   *    sensor ids of opening device.
   * @return true
   *    This ClosingThread needs to be removed.
   * @return false
   *    otherwise.
   */
  bool checkAndWait(const std::vector<MINT32>& sensorIds) {
    for (auto& id : sensorIds) {
      if (mSensorIds.find(id) != mSensorIds.end()) {
        if (mFuture.valid() && mFuture.get()) {
          CAM_ULOGMI("async closing device %" PRId32 " done: %s",
                     mInstanceId, toString().c_str());
        }
        return true;
      }
    }
    return false;
  }

  std::string toString() {
    std::string os = "sensorid:";
    for (auto& id : mSensorIds) {
      os += std::to_string(id) + ", ";
    }
    return os;
  }
};

static std::mutex                   gClosingThreadsLock;
static std::vector<ClosingThread>   gClosingThreads;
};


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::CommandHandler::
CommandHandler(int32_t instanceId)
    : Thread(false/*canCallJava*/)
    , mLogPrefix(std::to_string(instanceId)+"-session-cmd")
    , mCommand(nullptr)
    , mCommandName()
{
}

ThisNamespace::CommandHandler::
~CommandHandler()
{
    MY_LOGW("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::CommandHandler::
print(::android::Printer& printer) const -> void
{
    printer.printFormatLine("\n== CommandHandler (tid:%d) isRunning:%d exitPending:%d ==", getTid(), isRunning(), exitPending());

    std::string commandName;
    if ( OK == mLock.timedLock(getDumpTryLockTimeout()) ) {
        commandName = mCommandName;
        mLock.unlock();
    }
    else {
        printer.printLine("CommandHandler: lock failed");
        commandName = mCommandName;
    }

    if (commandName.empty()) {
        printer.printLine("   No pending command");
    }
    else {
        printer.printFormatLine("   Command \"%s\" is in flight", commandName.c_str());
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::CommandHandler::
threadLoop() -> bool
{
    CommandT command = nullptr;
    {
        Mutex::Autolock _l(mLock);
        while ( ! exitPending() && ! mCommand ) {
            int err = mCond.wait(mLock);
            if (OK != err) {
                MY_LOGW("exitPending:%d err:%d(%s)", exitPending(), err, ::strerror(-err));
                MY_LOGW("%s command(%s)", (!mCommand ? "invalid" : "valid"), (!mCommandName.empty() ? mCommandName.c_str() : "empty"));
            }
        }
        command = mCommand;
    }

    if ( command != nullptr ) {
        command();
    }

    {
        Mutex::Autolock _l(mLock);

        mCommandName.clear();
        mCommand = nullptr;
        mCond.broadcast();
    }

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::CommandHandler::
requestExitAndWait(nsecs_t const timeout) -> int
{
    auto h = this->add("requestExitAndWait", [this](){
        Thread::requestExit();
        MY_LOGD("requestExitAndWait done");
    });
    if ( h == nullptr ) {
        Thread::requestExit();
        MY_LOGW("requestExitAndWait cannot run");
        return NO_INIT;
    }
    return this->waitDone(h, timeout);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::CommandHandler::
waitDone(void* handle, nsecs_t const timeout) -> int
{
    if (this != handle) {
        MY_LOGE("this:%p != handle:%p", this, handle);
        return NO_INIT;
    }
    //
    TimeoutCaculator toc(timeout);
    //
    Mutex::Autolock _l(mLock);
    int err = OK;
    while ( ! exitPending() && mCommand != nullptr )
    {
        err = mCond.waitRelative(mLock, toc.timeoutToWait());
        if  ( OK != err ) {
            break;
        }
    }
    //
    if  ( mCommand != nullptr ) {
        MY_LOGE(
            "%s command(%s) isRunning:%d exitPending:%d timeout(ns):%" PRId64 " elapsed(ns):%" PRId64 " err:%d(%s)",
            (!mCommand ? "invalid" : "valid"),
            (!mCommandName.empty() ? mCommandName.c_str() : "empty"),
            isRunning(), exitPending(), timeout, toc.timeoutToWait(), err, ::strerror(-err)
        );
        return exitPending() ? DEAD_OBJECT : err;
    }

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::CommandHandler::
add(char const* name, CommandT cmd) -> void*
{
    Mutex::Autolock _l(mLock);

    if ( mCommand != nullptr ) {
        MY_LOGE("fail to add a new command \"%s\" since the previous command \"%s\" is still in flight", name, (!mCommandName.empty() ? mCommandName.c_str() : "empty"));
        return nullptr;
    }

    mCommandName = (name) ? name : "";
    mCommand = cmd;
    mCond.broadcast();
    return this;
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
~CameraDevice3SessionImpl()
{
    if(mpCpuCtrl)
    {
        mpCpuCtrl->destroyInstance();
        mpCpuCtrl = NULL;
    }
    {
      Mutex::Autolock _l(mAppStreamManagerLock);
      if(mAppStreamManager) {
        mAppStreamManager->destroy();
        mAppStreamManager = nullptr;
      }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
CameraDevice3SessionImpl(CreationInfo const& info)
    : ICameraDevice3Session()
    , mStaticInfo(info)
    , mLogPrefix(std::to_string(info.mStaticDeviceInfo->mInstanceId)+"-session")
    , mLogLevel(0)
    , mStateLog()
    , mCameraDeviceCallback()
    , mAppStreamManagerErrorState(std::make_shared<EventLogPrinter>(15, 25))
    , mAppStreamManagerWarningState(std::make_shared<EventLogPrinter>(25, 15))
    , mAppStreamManagerDebugState(std::make_shared<EventLogPrinter>())
    , mDeviceSessionPolicy()
    , mConfigTimestamp(0)
{
    MY_LOGE_IF(mAppStreamManagerErrorState==nullptr, "Bad mAppStreamManagerErrorState");
    MY_LOGE_IF(mAppStreamManagerWarningState==nullptr, "Bad mAppStreamManagerWarningState");
    MY_LOGE_IF(mAppStreamManagerDebugState==nullptr, "Bad mAppStreamManagerDebugState");
    MY_LOGD("%p", this);
    mStateLog.add("-> initialized");

    mCpuPerfTime = ::property_get_int32("vendor.cam3dev.cpuperftime", CPU_TIMEOUT);
    MY_LOGD("Create CPU Ctrl, timeout %d ms", mCpuPerfTime);
    mpCpuCtrl = Cam3CPUCtrl::createInstance();

    IHalLogicalDeviceList* pHalDeviceList = MAKE_HalLogicalDeviceList();
    MY_LOGA_IF(CC_UNLIKELY(!pHalDeviceList), "cannot get HalLogicalDeviceList");
    mSensorIds = pHalDeviceList->getSensorId(getInstanceId());

    mUseAppImageSBProvider = ::property_get_int32("vendor.cam3dev.useappimagesbpvdr", 0);
    mEarlyCallbackMeta = ::property_get_int32("vendor.camera.util.earlyCallbackMeta", 1);

    // ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getSafeAppStreamManager() const -> ::android::sp<IAppStreamManager>
{
    //  Although mAppStreamManager is setup during opening camera,
    //  we're not sure any callback to this class will arrive
    //  between open and close calls.
    Mutex::Autolock _l(mAppStreamManagerLock);
    return mAppStreamManager;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getSafeDeviceSessionPolicy() const -> std::shared_ptr<device::policy::IDeviceSessionPolicy>
{
    std::lock_guard<std::mutex> _l(mDeviceSessionPolicyLock);
    return mDeviceSessionPolicy;
}

// auto
// ThisNamespace::
// getSafeAppRequestUtil() const -> ::android::sp<IAppRequestUtil>
// {
//     Mutex::Autolock _l(mAppRequestUtilLock);
//     return mAppRequestUtil;
// }

auto
ThisNamespace::
getSafeHalBufHandler() const -> ::android::sp<IHalBufHandler>
{
    Mutex::Autolock _l(mHalBufHandlerLock);
    return mHalBufHandler;
}

// auto
// ThisNamespace::
// getSafeAppConfigUtil() const -> ::android::sp<IAppConfigUtil>
// {
//     Mutex::Autolock _l(mAppConfigUtilLock);
//     return mAppConfigUtil;
// }


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getSafePipelineModel() const -> ::android::sp<IPipelineModel>
{
    Mutex::Autolock _l(mPipelineModelLock);
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
auto
ThisNamespace::
flushAndWait() -> ::android::status_t
{
    CAM_ULOG_FUNCLIFE_GUARD(MOD_CAMERA_DEVICE);

    NSCam::Utils::CamProfile profile(__FUNCTION__, LOG_TAG);
    profile.print("+ %s", getInstanceName().c_str());
    //
    TimeoutCaculator toc(500000000);
    int err = OK;
    int Apperr = OK;
    nsecs_t timeout = 0;
    //
    do
    {
        auto pPipelineModel = getSafePipelineModel();
        if  ( pPipelineModel != 0 ) {
            err = pPipelineModel->beginFlush();
            MY_LOGW_IF(OK!=err, "pPipelineModel->beginFlush err:%d(%s)", -err, ::strerror(-err));
        }
        //
        // invoke HalBufHandler waitUntilDrained if exists
        //
        auto pAppStreamManager = getSafeAppStreamManager();
        Apperr = OK;
        if  ( pAppStreamManager != 0 ) {
            pAppStreamManager->flush();
            profile.print("waitUntilDrained +");
            Apperr = pAppStreamManager->waitUntilDrained(toc.timeoutToWait());
            profile.print("waitUntilDrained -");
            MY_LOGW_IF(OK!=err, "AppStreamManager::waitUntilDrained err:%d(%s)", -err, ::strerror(-err));
        }
        //
        if  ( pPipelineModel != 0 ) {
            pPipelineModel->endFlush();
        }
        timeout += 500000000;
    } while ( OK != Apperr && timeout < getFlushAndWaitTimeout() );
    //
    profile.print("-");
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
enableRequesting() -> void
{
    Mutex::Autolock _lRequesting(mRequestingLock);
    ::android_atomic_release_store(1, &mRequestingAllowed);
    m1stRequestNotSent = true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
disableRequesting() -> void
{
    Mutex::Autolock _lRequesting(mRequestingLock);
    ::android_atomic_release_store(0, &mRequestingAllowed);
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
auto
ThisNamespace::
tryRunCommandLocked(
    nsecs_t const timeout __unused,
    char const* commandName,
    CommandHandler::CommandT command
) -> int
{
    // if ( mCommandHandler == nullptr ) {
    //     MY_LOGE("Bad mCommandHandler, commandName %s", (commandName)?commandName:"NULL");
    //     return NO_INIT;
    // }

    // auto h = mCommandHandler->add(commandName, command);
    // if ( ! h ) {
    //     return NO_INIT;
    // }

    // do {
    //     MY_LOGD("Run command %s, timeout %zu ms +", commandName, timeout/1000000);
    //     auto err = mCommandHandler->waitDone(h, timeout);
    //     if ( err == OK ) {
    //         break;
    //     }
    //     handleCommandFailureLocked(err);
    // } while(true);

    MY_LOGD("Run command %s +", commandName);
    command();
    MY_LOGI("Run command %s -", commandName);
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
handleCommandFailureLocked(int status) -> void
{
    MY_LOGE("err:%d(%s)", status, ::strerror(-status));

    // Async to dump debugging information with a timeout of 1 second.
    mDumpFut = std::async(std::launch::async,
        [](auto pDeviceManager, auto pCommandHandler)
        {
            if ( pDeviceManager ) {
                pDeviceManager->debug(std::make_shared<ULogPrinter>(__ULOG_MODULE_ID, LOG_TAG), {});
            }

            CallStackLogger csl;
            // dump the callstack of command handler thread
            if(::property_get_bool("vendor.cam3dev.dumpthread", true) &&
                pCommandHandler != nullptr)
            {
                csl.logThread(LOG_TAG, ANDROID_LOG_INFO, pCommandHandler->getTid());
            }

            // dump the callstack of this process
            if(::property_get_bool("vendor.cam3dev.dumpprocess", true)) {
                csl.logProcess(LOG_TAG);
            }
        },
        mStaticInfo.mDeviceManager,
        mCommandHandler
    );

#if MTKCAM_TARGET_BUILD_VARIANT==0
    // Suicide only in user build.
    // Async to commit suicide (kill camerahalserver) after 1 second.
    std::async(std::launch::async, [](){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        CAM_ULOGMI("commit suicide: kill camerahalserver - raise(SIGINT)");
        ::raise(SIGINT);
        CAM_ULOGMI("commit suicide: kill camerahalserver - raise(SIGTERM)");
        ::raise(SIGTERM);
        CAM_ULOGMI("commit suicide: kill camerahalserver - raise(SIGKILL)");
        ::raise(SIGKILL);
    }).wait_for(std::chrono::seconds(0));
#endif
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onOpenLocked(
    const std::shared_ptr<CameraDevice3SessionCallback>& callback
) -> ::android::status_t
{
    MY_LOGD("+");
    mLogLevel = getCameraDevice3DebugLogLevel();

    mCameraDeviceCallback = callback;

    const auto &entry = mStaticInfo.mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION);
    if(entry.isEmpty()) {
        MY_LOGD("Disable HalBufHandler");
        mHalBufHandler = nullptr;
    }

    NSCam::v3::Utils::CreationInfo creationInfo{
        .mInstanceId            = getInstanceId(),
        .mCameraDeviceCallback  = callback,
        .mMetadataProvider      = mStaticInfo.mMetadataProvider,
        .mPhysicalMetadataProviders = mStaticInfo.mPhysicalMetadataProviders,
        .mMetadataConverter     = mStaticInfo.mMetadataConverter,
        .mErrorPrinter          = std::static_pointer_cast<android::Printer>(mAppStreamManagerErrorState),
        .mWarningPrinter        = std::static_pointer_cast<android::Printer>(mAppStreamManagerWarningState),
        .mDebugPrinter          = std::static_pointer_cast<android::Printer>(mAppStreamManagerDebugState),
        .mSupportBufferManagement = !entry.isEmpty(),
        // .mImageConfigMapLock    = mImageConfigMapLock,
        // .mImageConfigMap        = mImageConfigMap,
        // .mMetaConfigMap         = mMetaConfigMap,
    };
    //--------------------------------------------------------------------------
    {
        Mutex::Autolock _l(mAppStreamManagerLock);

        if  ( mAppStreamManager != nullptr ) {
            MY_LOGE("mAppStreamManager:%p != 0 while opening", mAppStreamManager.get());
            mAppStreamManager->destroy();
            mAppStreamManager = nullptr;
        }
        mAppStreamManager = IAppStreamManager::create(creationInfo);
        if  ( mAppStreamManager == nullptr ) {
            MY_LOGE("IAppStreamManager::create");
            return NO_INIT;
        }
    }
    //--------------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> _l(mDeviceSessionPolicyLock);

        if  ( mDeviceSessionPolicy != nullptr ) {
            MY_LOGE("mDeviceSessionPolicy:%p != 0 while opening", mDeviceSessionPolicy.get());
            mDeviceSessionPolicy->destroy();
            mDeviceSessionPolicy = nullptr;
        }

        auto staticInfo = std::make_shared<DeviceSessionStaticInfo>(
            DeviceSessionStaticInfo{
                .mInstanceId                    = getInstanceId(),
                .mCameraDeviceCallback          = callback,
                .mMetadataProvider              = mStaticInfo.mMetadataProvider,
                .mPhysicalMetadataProviders     = mStaticInfo.mPhysicalMetadataProviders,
                .mMetadataConverter             = mStaticInfo.mMetadataConverter,
                // staticInfo->mGrallocHelper                 = IGrallocHelper::singleton(),
                .mErrorPrinter                  = std::static_pointer_cast<android::Printer>(mAppStreamManagerErrorState),
                .mWarningPrinter                = std::static_pointer_cast<android::Printer>(mAppStreamManagerWarningState),
                .mDebugPrinter                  = std::static_pointer_cast<android::Printer>(mAppStreamManagerDebugState),
            }
        );
            // staticInfo->mInstanceId                    = getInstanceId();
            // staticInfo->mMetadataProvider              = mStaticInfo.mMetadataProvider;
            // staticInfo->mPhysicalMetadataProviders     = mStaticInfo.mPhysicalMetadataProviders;
            // staticInfo->mMetadataConverter             = mStaticInfo.mMetadataConverter;
            // // staticInfo->mGrallocHelper                 = IGrallocHelper::singleton();
            // staticInfo->mErrorPrinter                  = std::static_pointer_cast<android::Printer>(mAppStreamManagerErrorState);
            // staticInfo->mWarningPrinter                = std::static_pointer_cast<android::Printer>(mAppStreamManagerWarningState);
            // staticInfo->mDebugPrinter                  = std::static_pointer_cast<android::Printer>(mAppStreamManagerDebugState);

        mDeviceSessionPolicy = IDeviceSessionPolicyFactory::createDeviceSessionPolicyInstance(
            IDeviceSessionPolicyFactory::CreationParams{
                .mStaticInfo                    = staticInfo,
            }
        );
        if  ( CC_UNLIKELY(mDeviceSessionPolicy==nullptr)  ) {
            MY_LOGE("IDeviceSessionPolicy::createDeviceSessionPolicyInstance");
            return NO_INIT;
        }
    }
    //--------------------------------------------------------------------------
    // {
    //     Mutex::Autolock _l(mAppConfigUtilLock);

    //     if  ( mAppConfigUtil != nullptr ) {
    //         MY_LOGE("mAppConfigUtil:%p != 0 while opening", mAppConfigUtil.get());
    //         mAppConfigUtil->destroy();
    //         mAppConfigUtil = nullptr;
    //     }
    //     mAppConfigUtil = IAppConfigUtil::create(creationInfo);
    //     if  ( mAppConfigUtil == nullptr ) {
    //         MY_LOGE("IAppConfigUtil::create");
    //     }
    // }
    //--------------------------------------------------------------------------
    {
        Mutex::Autolock _l(mHalBufHandlerLock);

        ::android::sp<IHalBufHandler> pHalBufHandler = nullptr;
        if( !creationInfo.mSupportBufferManagement ) {
            MY_LOGD("Disable HalBufHandler");
        } else {
            MY_LOGD("Enable HalBufHandler, version 0x%x",(int) entry.itemAt(0,Type2Type<MUINT8>()));
            pHalBufHandler = IHalBufHandler::create(creationInfo, mAppStreamManager/*, mAppRequestUtil*/);
            if  ( pHalBufHandler == nullptr ) {
                MY_LOGE("IHalBufHandler::create");
                return NO_INIT;
            }
        }

        mAppImageSBProvider = AppImageStreamBufferProvider::create(pHalBufHandler);
        if  ( mAppImageSBProvider == nullptr ) {
            MY_LOGE("AppImageStreamBufferProvider::create");
            return NO_INIT;
        }

        // if( !creationInfo.mSupportBufferManagement ) {
        //     MY_LOGD("Disable HalBufHandler");
        //     mHalBufHandler = nullptr;
        // }
        // else {
        //     MY_LOGD("Enable HalBufHandler, version 0x%x",(int) entry.itemAt(0,Type2Type<MUINT8>()));
        //     mHalBufHandler = IHalBufHandler::create(creationInfo, mAppStreamManager, mAppRequestUtil);
        //     if  ( mHalBufHandler == nullptr ) {
        //         MY_LOGE("IHalBufHandler::create");
        //         return false;
        //     }

        //     mAppImageSBProvider = AppImageStreamBufferProvider::create(mHalBufHandler);
        //     if  ( mAppImageSBProvider == nullptr ) {
        //         MY_LOGE("AppImageStreamBufferProvider::create");
        //         return false;
        //     }
        // }
    }
    //--------------------------------------------------------------------------
    // {
    //     Mutex::Autolock _l(mAppRequestUtilLock);

    //     if  ( mAppRequestUtil != nullptr ) {
    //         MY_LOGE("mAppRequestUtil:%p != 0 while opening", mAppRequestUtil.get());
    //         mAppRequestUtil->destroy();
    //         mAppRequestUtil = nullptr;
    //     }
    //     mAppRequestUtil = IAppRequestUtil::create(creationInfo/*, mRequestMetadataQueue*/);
    //     if  ( mAppRequestUtil == nullptr ) {
    //         MY_LOGE("IAppRequestUtil::create");
    //         return NO_INIT;
    //     }
    // }
    //--------------------------------------------------------------------------
#if defined(SUPPORT_ASYNC_CLOSING)
    {
        std::lock_guard<std::mutex> _l(gClosingThreadsLock);
        auto it = gClosingThreads.end();
        while (it != gClosingThreads.begin()) {
            it--;
            if (it->checkAndWait(mSensorIds)) {
                gClosingThreads.erase(it);
            }
        }
    }
#endif
    //--------------------------------------------------------------------------
    {
        Mutex::Autolock _l(mPipelineModelLock);
        auto pPipelineModelMgr = IPipelineModelManager::get();
        if  ( CC_UNLIKELY(pPipelineModelMgr == nullptr) ) {
            MY_LOGE("IPipelineModelManager::get() is null object!");
            return NO_INIT;
        }
        //
        auto pPipelineModel = pPipelineModelMgr->getPipelineModel( getInstanceId() );
        if ( CC_UNLIKELY(pPipelineModel == nullptr) ) {
            MY_LOGE("IPipelineModelManager::getPipelineModel(%d) is null object!", getInstanceId());
            return NO_INIT;
        }
        //
        ::android::status_t err = OK;

        //
        //mPipelineModelCallback = new PipelineModelCallback(weak_from_this());
        //mPipelineModelCallback->setToParent(weak_from_this());

        err = pPipelineModel->open(getInstanceName(), this);
        if  ( CC_UNLIKELY(OK != err) ) {
            MY_LOGE( "fail to IPipelinemodel->open() status:%d(%s)", -err, ::strerror(-err) );
            return NO_INIT;
        }
        mPipelineModel = pPipelineModel;
        mConfigTimestamp = (uint64_t)::systemTime();
        MY_LOGD("timestamp(%" PRIu64 ")", mConfigTimestamp);
    }
    //--------------------------------------------------------------------------
    {
        ZoomRatioConverter::DimensionMap phyDimensionMap;
        ZoomRatioConverter::Dimension logicalDimension;
        auto staticMeta = mStaticInfo.mMetadataProvider->getMtkStaticCharacteristics();
        bool supportZoomRatio = true;

        IMetadata::IEntry ratioEntry = staticMeta.entryFor(MTK_CONTROL_ZOOM_RATIO_RANGE);
        if( ratioEntry.count() != 2 )
        {
            MY_LOGD("invalid MTK_CONTROL_ZOOM_RATIO_RANGE count:%d", ratioEntry.count());
            supportZoomRatio = false;
        }

        MRect activeArray;
        if(!IMetadata::getEntry<MRect>(&staticMeta, MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION, activeArray)) {
            MY_LOGD("no MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION");
            supportZoomRatio = false;
        }
        else {
            logicalDimension.width = activeArray.s.w;
            logicalDimension.height = activeArray.s.h;

            MY_LOGD("get open id(%d) active array: w(%d), h(%d)",
                        getInstanceId(), logicalDimension.width, logicalDimension.height);
        }

        // physical
        for(const auto& pair : mStaticInfo.mPhysicalMetadataProviders)
        {
            auto id = pair.first;
            auto pMetadataProvider_phy = pair.second;
            auto staticMeta = pMetadataProvider_phy->getMtkStaticCharacteristics();

            if(!IMetadata::getEntry<MRect>(&staticMeta, MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION, activeArray))
            {
                MY_LOGE("No MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION found (phy)");
                continue;
            }

            MY_LOGD("get phy id(%d) active array: w(%d), h(%d)",
                        id, (uint32_t)activeArray.s.w, (uint32_t)activeArray.s.h);

            ZoomRatioConverter::Dimension d = { (uint32_t)activeArray.s.w,
                                                (uint32_t)activeArray.s.h };
            phyDimensionMap.emplace(id, std::move(d));
        }

        mpZoomRatioConverter = ZoomRatioConverter::Create(supportZoomRatio, logicalDimension, phyDimensionMap);
    }

    IHalLogicalDeviceList const* pHalDeviceList = MAKE_HalLogicalDeviceList();
    if (pHalDeviceList != nullptr) {
        mEnableLogicalMulticam = pHalDeviceList->isLogicalMulticam(getInstanceId());
        MY_LOGD("mEnableLogicalMulticam(%d), instanceId(%d)",
                mEnableLogicalMulticam, getInstanceId());
    }

    MY_LOGD("-");
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onCloseLocked() -> void
{
    MY_LOGD("+");
    NSCam::Utils::CamProfile profile(__FUNCTION__, getInstanceName().c_str());

    if  ( ! waitUntilOpenDoneLocked() ) {
        MY_LOGE("open fail");
    }
    //
    {
        disableRequesting();
        flushAndWait();
        profile.print("flush -");
    }
    //
    // mCameraDeviceCallback = nullptr;
    //
    {
        Mutex::Autolock _l(mPipelineModelLock);
#if defined(SUPPORT_ASYNC_CLOSING)
        {
            std::lock_guard<std::mutex> _l(gClosingThreadsLock);
            auto thread = ClosingThread(getInstanceId(), mSensorIds, mPipelineModel);
            MY_LOGD("close thread gClosingThreads: %s", thread.toString().c_str());
            gClosingThreads.push_back(std::move(thread));
        }
#else
        if (mPipelineModel != nullptr) {
            mPipelineModel->close();
        }
#endif
        mPipelineModel = nullptr;
        profile.print("PipelineModel -");
    }
    //
    {
        std::lock_guard<std::mutex> _l(mDeviceSessionPolicyLock);
        if  ( mDeviceSessionPolicy != nullptr ) {
            mDeviceSessionPolicy->destroy();
            mDeviceSessionPolicy = nullptr;
            profile.print("mDeviceSessionPolicy -");
        }
    }
    //
    {
        Mutex::Autolock _l(mAppStreamManagerLock);
        if  ( mAppStreamManager != nullptr ) {
            mAppStreamManager->destroy();
            mAppStreamManager = nullptr;
            profile.print("AppStreamManager -");
        }
    }
    //
    {
        Mutex::Autolock _l(mHalBufHandlerLock);
        if  ( mHalBufHandler != nullptr ) {
            mHalBufHandler->destroy();
            mHalBufHandler = nullptr;
            profile.print("HalBufHandler -");
        }
    }
    //
    {
        Mutex::Autolock _l(mImageConfigMapLock);
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
auto
ThisNamespace::
onConfigureStreamsLocked(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration    //output
) -> ::android::status_t
{
    MY_LOGD("+");
    CAM_ULOG_FUNCLIFE_GUARD(MOD_CAMERA_DEVICE);

    ::android::status_t err = OK;

    if(mpCpuCtrl)
    {
        MY_LOGD("Enter CPU full run mode, timeout: %d ms", mCpuPerfTime);
        mpCpuCtrl->enterCPUBoost(mCpuPerfTime, &mCpuHandle);
        mIsBoosted = true;
    }

    if  ( ! waitUntilOpenDoneLocked() ) {
        return NO_INIT;
    }
    //
    {
        disableRequesting();
        flushAndWait();
    }
    //
    //
    err = checkConfigParams(requestedConfiguration);
    if  ( OK != err ) {
        MY_LOGE("checkConfigParams failed");
        return err;
    }
    auto pAppStreamManager = getSafeAppStreamManager();
    if  ( pAppStreamManager == 0 ) {
        MY_LOGE("Bad AppStreamManager");
        return NO_INIT;
    }
    //
    NSCam::v3::ConfigAppStreams appStreams;
    // mandatory TODO: prepare appStreams.vImageStreams/vMetaStreams
    // optional TODO: prepare appStreams.vMetaStreams_physical/vPhysicCameras
    //
    {
        auto pPipelineModel = getSafePipelineModel();
        if  ( pPipelineModel == 0 ) {
            MY_LOGE("Bad PipelineModel");
            return NO_INIT;
        }
        //
        auto pParams = std::make_shared<UserConfigurationParams>();
        if ( ! pParams ) {
            MY_LOGE("Bad UserConfigurationParams");
            return NO_INIT;
        }
        err = prepareUserConfigurationParamsLocked(requestedConfiguration, halConfiguration, pParams);
        if  ( OK != err ) {
            MY_LOGE("fail to prepareUserConfigurationParamsLocked");
            return err;
        }

        // Set appStreams
        appStreams.operationMode = static_cast<uint32_t>(requestedConfiguration.operationMode);
#define _CLONE_(dst, src) \
            do { \
                dst.clear(); \
                for ( const auto& item : src) { \
                    dst.add( item.first, item.second ); \
                } \
            } while (0) \

        _CLONE_(appStreams.vImageStreams,   pParams->vImageStreams);
        _CLONE_(appStreams.vMetaStreams ,   pParams->vMetaStreams);
#undef _CLONE_
        //
        err = pPipelineModel->configure(pParams);
        if  ( OK != err ) {
            MY_LOGE("fail to configure pipeline");
            return err;
        }
    }
    //
    //
    // Set maximum buffer size in HalStream
    for (size_t i = 0; i < appStreams.vImageStreams.size(); i++) {
        auto& halStream = halConfiguration.streams[i];
        auto& halMaxBufferSize = halStream.maxBuffers;
        StreamId_T const streamId = halStream.id;
        ssize_t index = appStreams.vImageStreams.indexOfKey(streamId);
        if ( index<0 ) {
            MY_LOGE("invalid streamId %" PRId64 "", streamId);
            return NAME_NOT_FOUND;
        }
        auto& halStreamInfo = appStreams.vImageStreams.valueAt(index);
        halMaxBufferSize = halStreamInfo->getMaxBufNum();
        MY_LOGD("halMaxBufferSize=%d HalStream.maxBuffers=%d", halMaxBufferSize, halStream.maxBuffers);
    }
    // setConfigMap
    {
        Mutex::Autolock _l(mImageConfigMapLock);
        MY_LOGD("[setConfigMap] mImageConfigMap.size()=%zu, mMetaConfigMap.size()=%zu",
                    mImageConfigMap.size(), mMetaConfigMap.size());
        pAppStreamManager->setConfigMap(mImageConfigMap, mMetaConfigMap);
    }
    // Register to AppStreamMgr
    err = pAppStreamManager->configureStreams(requestedConfiguration, /*halConfiguration,*/ appStreams);
    if  ( OK != err ) {
        MY_LOGE("fail to endConfigureStreams");
        return err;
    }

    // configure
    // auto pAppRequestUtil = getSafeAppRequestUtil();
    // if  ( pAppRequestUtil == 0 ) {
    //     MY_LOGE("Bad AppRequestUtil");
    //     return NO_INIT;
    // }
    // pAppRequestUtil->setConfigMap(mImageConfigMap, mMetaConfigMap);
    //
    // auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
    // if  ( pBufferHandleCacheMgr == 0 ) {
    //     MY_LOGE("Bad BufferHandleCacheMgr");
    //     return NO_INIT;
    // }
    // pBufferHandleCacheMgr->createBufferCacheMap(requestedConfiguration);
    //
    enableRequesting();
    MY_LOGD("-");
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onFlushLocked() -> ::android::status_t
{
    MY_LOGD("");

    ::android::status_t status = OK;

    if  ( ! waitUntilOpenDoneLocked() ) {
        return NO_INIT;
    }
    //
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
auto
ThisNamespace::
onProcessCaptureRequest(
    const std::vector<CaptureRequest>& requests,
    uint32_t& numRequestProcessed
) -> ::android::status_t
{
#define _UlogDiscard_(reqs)\
{ \
    for (size_t i = 0; i < reqs.size(); i++){ \
        CAM_ULOG_DISCARD_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, reqs[i].frameNumber); \
    } \
}
    size_t const requestNum = requests.size();
    uint32_t const requestNo_1st = requests[0].frameNumber;

    #define REQUEST_TIMEOUT_MS 2500
    for(auto &r : requests) {
        CAM_ULOG_ENTER_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, r.frameNumber, REQUEST_TIMEOUT_MS);
    }

    ::android::status_t err = OK;
    auto pPipelineModel = getSafePipelineModel();

    ::android::Vector<Request> appRequests;
    auto pAppStreamManager = getSafeAppStreamManager();
    if  ( pAppStreamManager == 0 ) {
        MY_LOGE("Bad AppStreamManager");
        _UlogDiscard_(requests);
        return DEAD_OBJECT;
    }

    {
        // step1-2, should be moved to HIDL adapter part
        Mutex::Autolock _lRequesting(mRequestingLock);

        auto pDeviceSessionPolicy = getSafeDeviceSessionPolicy();
        if ( CC_UNLIKELY(pDeviceSessionPolicy==nullptr) ) {
            MY_LOGE("Bad DeviceSessionPolicy, might caused latest request metadata cache fails");
            _UlogDiscard_(requests);
            return DEAD_OBJECT;
        }
        RequestInputParams const in{
            .requests         = &requests,
        };
        RequestOutputParams out{
            .requests         = &appRequests,
        };
        err = pDeviceSessionPolicy->evaluateRequest(&in, &out);

        if  ( OK != err ) {
            MY_LOGE("Create Requests failed");
            _UlogDiscard_(requests);
            return err;
        }

        // 3. handle flush case
        if  ( 0 == ::android_atomic_acquire_load(&mRequestingAllowed) ) {
            MY_LOGW("submitting requests during flushing - requestNo_1st:%u #:%zu", requestNo_1st, requestNum);
            for(auto &r : requests) {
                CAM_ULOG_EXIT_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, r.frameNumber);
            }
            //
            // AppStreamMgr handle flushed requests
            pAppStreamManager->flushRequest(requests);
            numRequestProcessed = requests.size();
            return OK;
        }
    }
    if  ( pPipelineModel == 0 ) {
        MY_LOGE("Bad PipelineModel");
        _UlogDiscard_(requests);
        return NO_INIT;
    }

    // 4. create requests params -> feature/customization
    std::vector<std::shared_ptr<UserRequestParams>> vPipelineRequests(appRequests.size());
    err = createUserRequestParamsLocked(vPipelineRequests);
    if  ( OK != err ) {
        _UlogDiscard_(requests);
        return err;
    }

    // 5. prepare request params for AppStreamMgr to appRequests
    err = pAppStreamManager->submitRequests(appRequests);
    if  ( OK != err ) {
        _UlogDiscard_(requests);
        return err;
    }

    // If it's a non-repeating request, to signal framework that camera hal has received
    // this request, we use early callback to return metadata to framework.
    // Batched requests don't use early callback.
    if (mEarlyCallbackMeta && requestNum == 1 && requests[0].settings != nullptr) {
        pAppStreamManager->earlyCallbackMeta(appRequests[0]);
    }

    // Update ULOG timeout for long exposure requests
    if ( appRequests.size() >0 )
    {
        for(auto &appRequest : appRequests)
        {
            auto ctrlMeta = appRequest.vInputMetaBuffers.valueFor(eSTREAMID_META_APP_CONTROL);
            IMetadata* pAppInMeta = ctrlMeta->tryReadLock(LOG_TAG);
            if(pAppInMeta)
            {
                MINT64 exposureTime = 0;
                if(IMetadata::getEntry<MINT64>(pAppInMeta, MTK_SENSOR_EXPOSURE_TIME, exposureTime))
                {
                    exposureTime /= 1000000L;
                    exposureTime += 1000; // Add some time for P2
                    if(exposureTime > REQUEST_TIMEOUT_MS)
                    {
                        MY_LOGD("Extend timeout to %" PRId64 " ms for long exposure time", exposureTime);
                        CAM_ULOG_MODIFY_GUARD(MOD_CAMERA_DEVICE, REQ_APP_REQUEST, appRequest.requestNo, exposureTime);
                    }
                }
                else
                {
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
            return NO_INIT;
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
    if  ( OK != err || requests.size() != numRequestProcessed ) {
        MY_LOGE("%u/%zu requests submitted sucessfully - err:%d(%s)",
            numRequestProcessed, vPipelineRequests.size(), -err, ::strerror(-err));
        pAppStreamManager->abortRequest(appRequests);
        numRequestProcessed = requests.size();
        _UlogDiscard_(requests);
        return OK;
    }
#undef _UlogDiscard_
    //
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareUserConfigurationParamsLocked(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration,
    std::shared_ptr<pipeline::model::UserConfigurationParams>& rCfgParams
)   -> ::android::status_t
{
    ::android::status_t status = OK;
    // operationMode
    rCfgParams->operationMode = static_cast<uint32_t>(((StreamConfiguration&)requestedConfiguration).operationMode);

    // sessionParams
    if ( requestedConfiguration.sessionParams != nullptr &&
        get_camera_metadata_entry_count(requestedConfiguration.sessionParams) > 0 ) {
        // read settings from session parameters
        if ( ! mStaticInfo.mMetadataConverter->convert(requestedConfiguration.sessionParams, rCfgParams->sessionParams) ) {
            MY_LOGE("Bad Session parameters");
            return -ENODEV;
        }
        mStaticInfo.mMetadataConverter->dumpAll(rCfgParams->sessionParams, 0);
    }

    // vImageStreams
    // vMetaStreams
    // vMetaStreams_physical
    // vMinFrameDuration
    // vStallFrameDuration
    // vPhysicCameras
    ConfigurationInputParams const in{
        .requestedConfiguration         = &requestedConfiguration,
    };
    ConfigurationOutputParams out{
        .resultConfiguration            = &halConfiguration,
        .pipelineCfgParams              = &rCfgParams,
        .imageConfigMap                 = &mImageConfigMap,
        .metaConfigMap                  = &mMetaConfigMap,
    };
    auto pDeviceSessionPolicy = getSafeDeviceSessionPolicy();
        if ( CC_UNLIKELY(pDeviceSessionPolicy==nullptr) ) {
            MY_LOGE("Bad DeviceSessionPolicy");
            return -ENODEV;
        }
    status = pDeviceSessionPolicy->evaluateConfiguration(&in, &out);

    // auto pAppConfigUtil = getSafeAppConfigUtil();
    // status = pAppConfigUtil->beginConfigureStreams(requestedConfiguration, halConfiguration, rCfgParams);

    // get Image/Meta config map from AppConfigUtil
    // pAppConfigUtil->getConfigMap(mImageConfigMap, mMetaConfigMap);
    // MY_LOGD("[debug] rCfgParams.vMetaStreams.size()=%u", rCfgParams->vMetaStreams.size());
    // // pStreamInfoBuilderFactory
    // rCfgParams->pStreamInfoBuilderFactory = pAppConfigUtil->getAppStreamInfoBuilderFactory();
    // rCfgParams->pStreamInfoBuilderFactory = out.pStreamInfoBuilderFactory;

    // temp pImageStreamBufferProvider
    // auto pHalBufHandler = getSafeHalBufHandler();
    // if  ( pHalBufHandler == 0 ) {
    //     MY_LOGW("NOT support HalBufhandler");
    // }
    // rCfgParams->pImageStreamBufferProvider = pHalBufHandler?  pHalBufHandler->getHalBufManagerStreamProvider() : nullptr;

    rCfgParams->pImageStreamBufferProvider = mAppImageSBProvider;
    MY_LOGD("pImageStreamBufferProvider %p", rCfgParams->pImageStreamBufferProvider.get());

    // configTimestamp
    rCfgParams->configTimestamp = mConfigTimestamp;
    //
    return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
checkConfigParams(
    const StreamConfiguration& requestedConfiguration
)   -> ::android::status_t
{
    ::android::status_t status = OK;

    // check overall configuration      -> AppStreamMgr::checkStreams
    // check each stream configuration  -> AppStreamMgr::checkStream
    CAM_ULOGM_FUNCLIFE();
    const vector<Stream>& vStreamConfigured = requestedConfiguration.streams;
    if  ( 0 == vStreamConfigured.size() ) {
        MY_LOGE("StreamConfiguration.streams.size() = 0");
        return -EINVAL;
    }
    //
    //
    KeyedVector<StreamType, size_t> typeNum;
    typeNum.add(StreamType::OUTPUT,          0);
    typeNum.add(StreamType::INPUT,           0);
    //
    KeyedVector<StreamRotation, size_t> outRotationNum;
    outRotationNum.add(StreamRotation::ROTATION_0,      0);
    outRotationNum.add(StreamRotation::ROTATION_90,     0);
    outRotationNum.add(StreamRotation::ROTATION_180,    0);
    outRotationNum.add(StreamRotation::ROTATION_270,    0);
    //
    for (const auto& stream : vStreamConfigured) {
        //
        status = checkStream(stream);
        if  ( OK != status ) {
            MY_LOGE("streams[id:%d] has a bad status: %d(%s)", stream.id, status, ::strerror(-status));
            return status;
        }
        //
        typeNum.editValueFor(stream.streamType)++;
        if ( StreamType::INPUT != stream.streamType )
            outRotationNum.editValueFor(stream.rotation)++;
    }

    /**
     * At most one input-capable stream may be defined (INPUT or BIDIRECTIONAL)
     * in a single configuration.
     *
     * At least one output-capable stream must be defined (OUTPUT or
     * BIDIRECTIONAL).
     */
    /*
     *
     * Return values:
     *
     *  0:      On successful stream configuration
     *
     * -EINVAL: If the requested stream configuration is invalid. Some examples
     *          of invalid stream configurations include:
     *
     *          - Including more than 1 input-capable stream (INPUT or
     *            BIDIRECTIONAL)
     *
     *          - Not including any output-capable streams (OUTPUT or
     *            BIDIRECTIONAL)
     *
     *          - Including too many output streams of a certain format.
     *
     *          - Unsupported rotation configuration (only applies to
     *            devices with version >= CAMERA_DEVICE_API_VERSION_3_3)
     */
    if  ( (typeNum[(int)StreamType::INPUT] > 1) ||  (typeNum[(int)StreamType::OUTPUT] == 0) )
    {
        MY_LOGE( "bad stream count: (out, in)=(%zu, %zu)",
                 typeNum[(int)StreamType::OUTPUT], typeNum[(int)StreamType::INPUT] );
        return  -EINVAL;
    }
    //
    size_t const num_rotation_not0 = outRotationNum[(int)StreamRotation::ROTATION_90] +
                                     outRotationNum[(int)StreamRotation::ROTATION_180] +
                                     outRotationNum[(int)StreamRotation::ROTATION_270];
    if ( num_rotation_not0 > 1 )
    {
        MY_LOGW("more than one output streams need to rotation: (0, 90, 180, 270)=(%zu,%zu,%zu,%zu)",
                outRotationNum[(int)StreamRotation::ROTATION_0],   outRotationNum[(int)StreamRotation::ROTATION_90],
                outRotationNum[(int)StreamRotation::ROTATION_180], outRotationNum[(int)StreamRotation::ROTATION_270]);
        return -EINVAL;
    }
    //
    return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
checkStream(
    const Stream& rStream
)   -> ::android::status_t
{
    /**
     * Return values:
     *
     *  OK:     On successful stream configuration
     *
     * -EINVAL: If the requested stream configuration is invalid. Some examples
     *          of invalid stream configurations include:
     *
     *          - Including streams with unsupported formats, or an unsupported
     *            size for that format.
     *
     *          ....
     *
     *          - Unsupported rotation configuration (only applies to
     *            devices with version >= CAMERA_DEVICE_API_VERSION_3_3)
     */
    if ( HAL_DATASPACE_DEPTH == rStream.dataSpace ) {
        MY_LOGE("Not support depth dataspace! %s", toString(rStream).c_str());
        return -EINVAL;
    } else if (static_cast<int>(HAL_DATASPACE_JPEG_APP_SEGMENTS) == static_cast<int>(rStream.dataSpace) ) {
        IMetadata::IEntry const& entry = mStaticInfo.mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_HEIC_INFO_SUPPORTED);
        if ( entry.isEmpty() ) {
            MY_LOGE("no static MTK_HEIC_INFO_SUPPORTED");
            return -EINVAL;
        } else {
            MBOOL heicSupport = entry.itemAt(0, Type2Type<MUINT8>());
            if (heicSupport) return OK;
            else return -EINVAL;
        }
    } else if ( HAL_DATASPACE_UNKNOWN != rStream.dataSpace ) {
        // MY_LOGW("framework stream dataspace:0x%08x(%s) %s", rStream.dataSpace, mStaticInfo.mGrallocHelper->queryDataspaceName(rStream.dataSpace).c_str(), toString(rStream).c_str());
        MY_LOGW("framework stream dataspace:0x%08x %s", rStream.dataSpace, toString(rStream).c_str());
    }
    //
    switch ( rStream.rotation )
    {
    case StreamRotation::ROTATION_0:
        break;
    case StreamRotation::ROTATION_90:
    case StreamRotation::ROTATION_180:
    case StreamRotation::ROTATION_270:
        MY_LOGI("%s", toString(rStream).c_str());
        if ( StreamType::INPUT == rStream.streamType ) {
            MY_LOGE("input stream cannot support rotation");
            return -EINVAL;
        }

        switch (rStream.format)
        {
        case HAL_PIXEL_FORMAT_RAW16:
        case HAL_PIXEL_FORMAT_RAW_OPAQUE:
            MY_LOGE("raw stream cannot support rotation");
            return -EINVAL;
        default:
            break;
        }

        break;
    default:
        MY_LOGE("rotation:%d out of bounds - %s", rStream.rotation, toString(rStream).c_str());
        return -EINVAL;
    }
    //
    sp<IMetadataProvider> currentMetadataProvider = nullptr;
    if (rStream.physicalCameraId.size() != 0){
        MUINT32 vid = (MUINT32)std::stoi((std::string)rStream.physicalCameraId);
        MY_LOGI("Check the setting of physical camera %u instead of logical camera for streamId %d", vid, rStream.id);
        auto search = mStaticInfo.mPhysicalMetadataProviders.find(vid);
        if (search != mStaticInfo.mPhysicalMetadataProviders.end()){
            currentMetadataProvider = search->second;
        }
        else{
            MY_LOGE("Couldn't find corresponding metadataprovider of physical cam %u", vid);
            return -EINVAL;
        }
    }
    else{
        currentMetadataProvider = mStaticInfo.mMetadataProvider;
    }
    IMetadata::IEntry const& entryScaler = currentMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    if ( entryScaler.isEmpty() )
    {
        MY_LOGE("no static MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS");
        return -EINVAL;
    }

    // android.scaler.availableStreamConfigurations
    // int32 x n x 4
    sp<AppImageStreamInfo> pInfo = getConfigImageStream(rStream.id);
    for (MUINT i = 0; i < entryScaler.count(); i += 4 )
    {
        int32_t const format = entryScaler.itemAt(i, Type2Type< MINT32 >());
        if ( (int32_t)rStream.format==format ||
             (pInfo.get() && pInfo->getOriImgFormat()==format ) )
        {
            MUINT32 scalerWidth  = entryScaler.itemAt(i + 1, Type2Type< MINT32 >());
            MUINT32 scalerHeight = entryScaler.itemAt(i + 2, Type2Type< MINT32 >());

            if ( ( rStream.width == scalerWidth && rStream.height == scalerHeight ) ||
                 ( rStream.rotation&StreamRotation::ROTATION_90 &&
                   rStream.width == scalerHeight && rStream.height == scalerWidth ) )
            {
                return OK;
            }
        }
    }
    MY_LOGE("unsupported size %dx%d for format 0x%x/rotation:%d - %s",
            rStream.width, rStream.height, rStream.format, rStream.rotation, toString(rStream).c_str());
    return -EINVAL;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getConfigImageStream(
    StreamId_T streamId
) const -> android::sp<AppImageStreamInfo>
{
    Mutex::Autolock _l(mImageConfigMapLock);
    ssize_t const index = mImageConfigMap.indexOfKey(streamId);
    if  ( 0 <= index ) {
        return mImageConfigMap.valueAt(index)/*.pStreamInfo*/;
    }
    return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
checkRequestParams(
    const vector<CaptureRequest>& vRequests __unused
)   -> ::android::status_t
{
    ::android::status_t status = OK;

    MY_LOGF("Not implement yet!");
    return status;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
createUserRequestParamsLocked(
    std::vector<std::shared_ptr<UserRequestParams>>& rvReqParams __unused
) -> ::android::status_t
{
    ::android::status_t status = OK;

    // Not implement yet
    return status;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NSCam::ICameraDevice3Session Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpState(::android::Printer& printer, const std::vector<std::string>& options __unused) -> void
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    printer.printLine("\n== state transition (most recent at bottom): Camera device ==");
    mStateLog.print(printer);

    printer.printLine("\n== error state (most recent at bottom): App Stream Manager ==");
    mAppStreamManagerErrorState->print(printer);

    printer.printLine("\n== warning state (most recent at bottom): App Stream Manager ==");
    mAppStreamManagerWarningState->print(printer);

    printer.printLine("\n== debug state (most recent at bottom): App Stream Manager ==");
    mAppStreamManagerDebugState->print(printer);

    auto pCommandHandler = mCommandHandler;
    if ( pCommandHandler != nullptr ) {
        pCommandHandler->print(printer);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
open(
    const std::shared_ptr<CameraDevice3SessionCallback> callback
) -> ::android::status_t
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    ::android::Mutex::Autolock _lOpsLock(mOpsLock);
    MY_LOGI("+");

    // //unlink to death notification for existed device callback
    // if ( mCameraDeviceCallback != nullptr ) {
    //     mCameraDeviceCallback->unlinkToDeath(this);
    //     mCameraDeviceCallback = nullptr;
    //     ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));
    // }

    // //link to death notification for device callback
    // if ( callback != nullptr ) {
    //     hardware::Return<bool> linked = callback->linkToDeath(this, (uint64_t)this);
    //     if (!linked.isOk()) {
    //         MY_LOGE("Transaction error in linking to mCameraDeviceCallback death: %s", linked.description().c_str());
    //     } else if (!linked) {
    //         MY_LOGW("Unable to link to mCameraDeviceCallback death notifications");
    //     }
    //     callback->getDebugInfo([this](const auto& info){
    //         mLinkToDeathDebugInfo = info;
    //     });
    //     MY_LOGD("Link death to ICameraDeviceCallback %s", toString(mLinkToDeathDebugInfo).c_str());
    // }

    if(mpCpuCtrl)
    {
        MY_LOGD("Enter camera perf mode");
        mpCpuCtrl->enterCameraPerf(mCpuPerfTime);
    }

    mDisplayIdleDelayUtil.enable();

    ::android::status_t status = OK;
    String8 const stateTag("-> open");
    mStateLog.add(stateTag + " +");

    do {

        // if (callback == nullptr) {
        //     MY_LOGE("cannot open camera. callback is null!");
        //     status = BAD_VALUE;
        //     break;
        // }

        auto pDeviceManager = mStaticInfo.mDeviceManager;
        auto const& instanceName = mStaticInfo.mStaticDeviceInfo->mInstanceName;

        status = pDeviceManager->startOpenDevice(instanceName);
        if  ( OK != status ) {
            pDeviceManager->updatePowerOnDone();
            break;
        }

        do {
            //------------------------------------------------------------------
            mCommandHandler = new CommandHandler(getInstanceId());
            if  ( mCommandHandler == nullptr ) {
                MY_LOGE("Bad mCommandHandler");
                status = NO_INIT;
                break;
            }
            else {
                const std::string threadName{std::to_string(getInstanceId())+":dev3-cmd"};
                status = mCommandHandler->run(threadName.c_str());
                if  ( OK != status ) {
                    MY_LOGE("Fail to run the thread %s - status:%d(%s)", threadName.c_str(), status, ::strerror(-status));
                    mCommandHandler = nullptr;
                    status = NO_INIT;
                    break;
                }
            }
            //------------------------------------------------------------------
            int err = NO_INIT;
            status = tryRunCommandLocked(getWaitCommandTimeout(), "onOpenLocked", [&, this](){
                err = onOpenLocked(callback);
            });
            if ( status == OK ) {
                status = err;
            }
        } while (0);

        pDeviceManager->updatePowerOnDone();

        if  ( OK != status ) {
            pDeviceManager->finishOpenDevice(instanceName, true/*cancel*/);
            break;
        }

        status = pDeviceManager->finishOpenDevice(instanceName, false/*cancel*/);
        if  ( OK != status ) {
            break;
        }

    } while (0);

    mStateLog.add(stateTag + " - " + (0==status ? "OK" : ::strerror(-status)));
    return status;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraDeviceSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
constructDefaultRequestSettings(
    RequestTemplate type
    // android::sp<IMetadata>& rpMetadata          //output
    // camera_metadata *templateSetting __unused          //output
) -> const camera_metadata_t*
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    String8 const stateTag(String8::format("-> constructDefaultRequestSettings (type:%#x)", type));
    mStateLog.add(stateTag + " +");
    MY_LOGD_IF(1, "%s", stateTag.c_str());

    ITemplateRequest* obj = NSTemplateRequestManager::valueFor(getInstanceId());
    if  (obj == nullptr) {
        obj = ITemplateRequest::getInstance(getInstanceId());
        NSTemplateRequestManager::add(getInstanceId(), obj);
    }

    mStateLog.add(stateTag + " -");
    return obj->getData(static_cast<int>(type));
    //return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
configureStreams(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration //output
) -> int
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    CAM_TRACE_CALL();
    //get fps
    int fps = -1;
    IMetadata metaSessionParams;
    if (requestedConfiguration.sessionParams != nullptr &&
        get_camera_metadata_entry_count(requestedConfiguration.sessionParams) > 0 ) {
        MY_LOGD("camera default configureStreams setConfigTimeCPUCtrl");
        // read settings from session parameters
        if ( ! mStaticInfo.mMetadataConverter->convert(requestedConfiguration.sessionParams, metaSessionParams) ) {
            MY_LOGD("Bad Session parameters");
        }
    }
    const NSCam::IMetadata::IEntry& entry =metaSessionParams.entryFor(MTK_STREAMING_FEATURE_HFPS_MODE);
    if (!entry.isEmpty()) {
         MINT32 streamHfpsMode = entry.itemAt(0, NSCam::Type2Type<MINT32>());
         fps = streamHfpsMode;
    }
    // get maxWidth
    int maxWidth = 0;
    for(const Stream& stream : requestedConfiguration.streams) {
        if(stream.streamType != StreamType::OUTPUT)
            continue;
            // check preview or record buffer
        if((stream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) ||
            (stream.usage & GRALLOC_USAGE_HW_TEXTURE) ||
            (stream.usage & GRALLOC_USAGE_HW_COMPOSER)) {
           maxWidth = (stream.width > maxWidth) ? stream.width : maxWidth;
        }
    }
    // setConfigTimeCPUCtrl save param
    mCustConfigTimeCPUCtrl.setConfigTimeCPUCtrl(fps,maxWidth);
    ::android::status_t status = OK;
    String8 const stateTag(String8::format("-> configure (operationMode:%#x)", requestedConfiguration.operationMode));
    mStateLog.add(stateTag + " +");

    //WrappedHalStreamConfiguration halStreamConfiguration;
    {
        ::android::Mutex::Autolock _lOpsLock(mOpsLock);

        { // update stream config counter
            Mutex::Autolock _l(mStreamConfigCounterLock);
            mStreamConfigCounter = requestedConfiguration.streamConfigCounter;
        }

        int err = NO_INIT;
        status = tryRunCommandLocked(getWaitCommandTimeout(), "onConfigureStreamsLocked", [&, this](){
            err = onConfigureStreamsLocked(requestedConfiguration, halConfiguration);
            for ( int i=0; i<halConfiguration.streams.size(); ++i ) {
                MY_LOGD("halstream.maxBuffers(%u)", halConfiguration.streams[i].maxBuffers);
            }

        });
        if ( status == OK ) {
            status = err;
        }
    }

    //_hidl_cb(mapToHidlCameraStatus(status), halStreamConfiguration);

    mStateLog.add(stateTag + " - " + (0==status ? "OK" : ::strerror(-status)));

    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
processCaptureRequest(
    const std::vector<CaptureRequest>& requests,
    /*const std::vector<BufferCache>& cachesToRemove, */
    uint32_t& numRequestProcessed               //output
) -> int
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
    CAM_TRACE_CALL();
    auto status = onProcessCaptureRequest(requests, numRequestProcessed);

    MY_LOGD_IF(getLogLevel() >= 2, "- requestNo_1st:%u #:%zu numRequestProcessed:%u", requests[0].frameNumber, requests.size(), numRequestProcessed);

    if  ( m1stRequestNotSent ) {
        if  (OK == status) {
            m1stRequestNotSent = false;
            mStateLog.add("-> 1st request - OK");
            MY_LOGD("-> 1st request - OK");
        }
        else {
            mStateLog.add("-> 1st request - failure");
            MY_LOGE("-> 1st request - failure");
        }
    }

    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
flush() -> int
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    int handle = 0;
    if(mpCpuCtrl)
    {
        MY_LOGD("Enter CPU full run mode, timeout: %d ms", mCpuPerfTime);
        mpCpuCtrl->enterCPUBoost(mCpuPerfTime, &handle);
    }

    ::android::status_t status = OK;
    String8 const stateTag("-> flush");
    mStateLog.add(stateTag + " +");
    {
        ::android::Mutex::Autolock _lOpsLock(mOpsLock);

        int err = NO_INIT;
        status = tryRunCommandLocked(getWaitCommandTimeout(), "onFlushLocked", [&, this](){
            err = onFlushLocked();
        });
        if ( status == OK ) {
            status = err;
        }
    }
    mStateLog.add(stateTag + " - " + (0==status ? "OK" : ::strerror(-status)));

    return status;
}



/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
close() -> int
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    ::android::Mutex::Autolock _lOpsLock(mOpsLock);
    if ( mCommandHandler == nullptr ) {
        MY_LOGI("Ignore to close");
        return NO_INIT;
    }

    MY_LOGI("+");

    int handle = 0;
    if(mpCpuCtrl)
    {
        MY_LOGD("Enter CPU full run mode, timeout: %d ms", mCpuPerfTime);
        mpCpuCtrl->enterCPUBoost(mCpuPerfTime, &handle);
    }

    ::android::status_t status = OK;
    String8 const stateTag("-> close");
    mStateLog.add(stateTag + " +");

    auto pDeviceManager = mStaticInfo.mDeviceManager;
    auto const& instanceName = mStaticInfo.mStaticDeviceInfo->mInstanceName;

    status = pDeviceManager->startCloseDevice(instanceName);
    if  ( OK != status ) {
        MY_LOGW("startCloseDevice [%d %s]", -status, ::strerror(-status));
    }

    {
        status = tryRunCommandLocked(getWaitCommandTimeout(), "onCloseLocked", [&, this](){
            onCloseLocked();
        });
        mCommandHandler->requestExitAndWait(getExitCommandTimeout());
        mCommandHandler = nullptr;
    }

    status = pDeviceManager->finishCloseDevice(instanceName);
    if  ( OK != status ) {
        MY_LOGW("finishCloseDevice [%d %s]", -status, ::strerror(-status));
    }

    if(mpCpuCtrl)
    {
        MY_LOGD("Exit camera perf mode");
        mpCpuCtrl->exitCPUBoost(&handle);
        mpCpuCtrl->exitCameraPerf();
    }
    // resetConfigTimeCPUCtrl-- reset camera default fpsgo param when close camera
    mCustConfigTimeCPUCtrl.resetConfigTimeCPUCtrl();

    mStateLog.add(stateTag + " - " + (0==status ? "OK" : ::strerror(-status)));

    mDisplayIdleDelayUtil.disable();
    MY_LOGI("-");
    return OK;
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
auto
ThisNamespace::
signalStreamFlush(
    const std::vector<int32_t>& streamIds,
    uint32_t streamConfigCounter
) -> void
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    ::android::Mutex::Autolock _lOpsLock(mOpsLock);

    MY_LOGD("+ streamConfigCounter %d",streamConfigCounter);
    for(auto streamId: streamIds) {
        MY_LOGD("streamId %d", (int)streamId);
    }

    { // check stream config counter valid
        Mutex::Autolock _l(mStreamConfigCounterLock);
        if (streamConfigCounter < mStreamConfigCounter) {
            MY_LOGD("streamConfigCounter %d is stale (current %d), skipping signal_stream_flush call",
                    streamConfigCounter, mStreamConfigCounter);
            return;
        }
    }

    ::android::status_t status = OK;
    int err = NO_INIT;
    status = tryRunCommandLocked(getWaitCommandTimeout(), "onFlushLocked", [&, this](){
            err = onFlushLocked();
        });
    if ( status == OK ) {
        status = err;
    }

    MY_LOGD("- %s", (0==status ? "OK" : ::strerror(-status)));

    return;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
isReconfigurationRequired(
    const camera_metadata& oldSessionParams,
    const camera_metadata& newSessionParams,
    bool& reconfigurationNeeded                 //output
) -> ::android::status_t
{
    IMetadata oldMetadata;
    IMetadata newMetadata;
    if ( ! mStaticInfo.mMetadataConverter->convert(&oldSessionParams, oldMetadata) ) {
        MY_LOGE("Bad Session parameters: oldSessionParams");
        return -ENODEV;
    }
    if ( ! mStaticInfo.mMetadataConverter->convert(&newSessionParams, newMetadata) ) {
        MY_LOGE("Bad Session parameters: newSessionParams");
        return -ENODEV;
    }
    if (!CaptureSessionParameters::isReconfigurationRequired(oldMetadata, newMetadata)) {
        reconfigurationNeeded = false;
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
switchToOffline(
    const std::vector<int32_t>& streamsToKeep __unused,
    CameraOfflineSessionInfo& offlineSessionInfo __unused/*,
    ICameraOfflineSession& offlineSession*/
) -> int
{
    return 0;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineModelCallback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onFrameUpdated(
    pipeline::model::UserOnFrameUpdated const& params
) -> void
{
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
        .realTimePartial = (params.isRealTimeUpdate) ? RealTime::HARD : RealTime::NON,
    };

    if ( params.timestampStartOfFrame != 0 ){
        tempParams.timestampStartOfFrame = params.timestampStartOfFrame;
    }

    tempParams.resultMeta.setCapacity(params.vOutMeta.size());
    for ( size_t i=0; i<params.vOutMeta.size(); ++i ) {
        tempParams.resultMeta.add(params.vOutMeta[i]);
    }

    if (mEnableLogicalMulticam && params.activePhysicalId >= 0 &&
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

    pAppStreamManager->updateResult(tempParams);

    if (mIsBoosted && params.nOutMetaLeft<=0 && mpCpuCtrl) {
        mpCpuCtrl->exitCPUBoost(&mCpuHandle);
        mIsBoosted = false;
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
    pipeline::model::UserOnMetaResultAvailable&& arg __unused
) -> void
{
    CAM_TRACE_CALL();

    String8 const postfix = String8::format("frameNo:%u userId:%s",
        arg.requestNo, arg.callerName.c_str());

    MY_LOGD_IF(getLogLevel() >= 2, "+ %s", postfix.string());
    NSCam::Utils::CamProfile profile(__FUNCTION__, "CameraDevice3SessionImpl");

    auto pAppStreamManager = getSafeAppStreamManager();
    if  ( pAppStreamManager == 0 ) {
        MY_LOGE("Bad AppStreamManager");
        return;
    }
    profile.print_overtime(1, "getSafeAppStreamManager: %s", postfix.string());

    sp<IAppStreamManager::UpdateAvailableParams> tempParams = new IAppStreamManager::UpdateAvailableParams;
    tempParams->frameNo = arg.requestNo;
    tempParams->callerName = arg.callerName;
    tempParams->resultMetadata = (*arg.resultMetadata);
    tempParams->userId = arg.userId;
    pAppStreamManager->updateAvailableMetaResult(tempParams);

    profile.print_overtime(1, "onMetaResultAvailable: %s", postfix.string());
    MY_LOGD_IF(getLogLevel() >= 2, "- %s", postfix.string());
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
onImageBufferReleased(pipeline::model::UserOnImageBufferReleased&& arg)
{
    ::android::sp<IAppStreamManager> pAppStreamManager = nullptr;
    for(auto &r : arg.results)
    {
        if(r.status & STREAM_BUFFER_STATUS::ERROR)
        {
            if(pAppStreamManager == nullptr)
            {
                pAppStreamManager = getSafeAppStreamManager();
                if(pAppStreamManager == nullptr)
                    break;
            }
            MY_LOGD_IF(getLogLevel() >= 2,"requestNo %d, streamId %d, status %d", (int)arg.requestNo, (int)r.streamId, (int)r.status);
            pAppStreamManager->markStreamError(arg.requestNo , r.streamId);
        }
    }
}


/******************************************************************************
 * This method is called when a request has fully completed no matter whether
 * the processing for that request succeeds or not.
 ******************************************************************************/
void
ThisNamespace::
onRequestCompleted(pipeline::model::UserOnRequestCompleted&& arg __unused)
{

}
