/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_DEVICE_DEPEND_CAMERADEVICE3SESSIONIMPL_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_DEVICE_DEPEND_CAMERADEVICE3SESSIONIMPL_H_
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <mtkcam3/pipeline/model/IPipelineModelManager.h>
#pragma GCC diagnostic pop
//
#include "CustConfigTimeCPUCtrl.h"
#include "../utils/include/AppImageStreamBufferProvider.h"
#include <ICameraDevice3Session.h>
// #include <CameraDevice3Session.h>
#include <IAppStreamManager.h>
#include <IAppConfigUtil.h>
#include <IAppRequestUtil.h>
// #include <IBufferHandleCacheMgr.h>
#include <IHalBufHandler.h>
#include <mtkcam3/main/hal/device/3.x/policy/IDeviceSessionPolicy.h>
//
#include <functional>
//
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
//
#include "EventLog.h"
#include "DisplayIdleDelayUtil.h"
#include <mtkcam/utils/sys/Cam3CPUCtrl.h>
#include <mtkcam/utils/std/CallStackLogger.h>
#include <future>
#include "ZoomRatioConverter.h"

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {

//class PipelineModelCallback; // forward declaration
/******************************************************************************
 *
 ******************************************************************************/
class CameraDevice3SessionImpl
    : public ICameraDevice3Session
    , std::enable_shared_from_this<CameraDevice3SessionImpl>
    , public pipeline::model::IPipelineModelCallback
{
public :
    using   AppImageStreamInfo = NSCam::v3::Utils::Camera3ImageStreamInfo;
    using   AppMetaStreamInfo  = NSCam::v3::Utils::MetaStreamInfo;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    static const nsecs_t _ONE_MS = 1000000;

    static bool isLowMemoryDevice()
    {
        return ::property_get_bool("ro.config.low_ram", false);
    }

    nsecs_t getExitCommandTimeout() const
    {
        #if (MTKCAM_TARGET_BUILD_VARIANT==2)
        if(isLowMemoryDevice()) {
            return ::property_get_int32("vendor.cam3dev.exitcommandtimeout", 30000) * _ONE_MS;
        }
        #endif

        return 10000000000; //10s
    }

    nsecs_t getWaitCommandTimeout() const
    {
        #if (MTKCAM_TARGET_BUILD_VARIANT==2)
        if(isLowMemoryDevice()) {
            return ::property_get_int32("vendor.cam3dev.waitcommandtimeout", 30000) * _ONE_MS;
        }
        #endif

        return 10000000000; //10s
    }

    nsecs_t getFlushAndWaitTimeout() const
    {
        #if (MTKCAM_TARGET_BUILD_VARIANT==2)
        if(isLowMemoryDevice()) {
            return ::property_get_int32("vendor.cam3dev.flushandwaittimeout", 9000) * _ONE_MS;
        }
        #endif

        return 3000000000;  //3s
    }


    class   CommandHandler : public android::Thread
    {
    public:
        using CommandT = std::function<void()>;

    protected:
        std::string const                   mLogPrefix;
        mutable ::android::Mutex            mLock;
        ::android::Condition                mCond;
        CommandT                            mCommand;
        std::string                         mCommandName;

    private:
        virtual auto    threadLoop() -> bool;
        auto const&     getLogPrefix() const            { return mLogPrefix; }

    public:
                        CommandHandler(int32_t instanceId);
        virtual         ~CommandHandler();
        auto            print(::android::Printer& printer) const -> void;
        auto            requestExitAndWait(nsecs_t const timeout) -> int;
        auto            waitDone(void* handle, nsecs_t const timeout) -> int;
        auto            add(char const* name, CommandT cmd) -> void*;

    protected:
        nsecs_t getDumpTryLockTimeout() const
        {
            #if (MTKCAM_TARGET_BUILD_VARIANT==2)
            if(isLowMemoryDevice()) {
                return ::property_get_int32("vendor.cam3dev.dumptrylocktimeout", 900) * _ONE_MS;
            }
            #endif

            return 300000000;   //300ms
        }
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    //  setup during constructor
    CreationInfo                            mStaticInfo;
    std::string const                       mLogPrefix;
    mutable ::android::Mutex                mOpsLock;

    //  setup during opening camera
    int32_t                                 mLogLevel = 0;
    EventLog                                mStateLog;
    // ::android::sp<V3_5::ICameraDeviceCallback>    mCameraDeviceCallback = nullptr;
    std::weak_ptr<CameraDevice3SessionCallback>     mCameraDeviceCallback;
    ::android::sp<CommandHandler>           mCommandHandler = nullptr;

    mutable android::Mutex                  mAppStreamManagerLock;
    ::android::sp<IAppStreamManager>        mAppStreamManager = nullptr;
    mutable android::Mutex                  mAppConfigUtilLock;
    ::android::sp<IAppConfigUtil>           mAppConfigUtil    = nullptr;
    std::shared_ptr<EventLogPrinter>        mAppStreamManagerErrorState;
    std::shared_ptr<EventLogPrinter>        mAppStreamManagerWarningState;
    std::shared_ptr<EventLogPrinter>        mAppStreamManagerDebugState;
    // AppRequestUtil
    mutable android::Mutex                  mAppRequestUtilLock;
    ::android::sp<IAppRequestUtil>          mAppRequestUtil = nullptr;
    // HalBufHandler
    mutable android::Mutex                  mHalBufHandlerLock;
    ::android::sp<IHalBufHandler>           mHalBufHandler;

    mutable std::mutex                      mDeviceSessionPolicyLock;
    std::shared_ptr<device::policy::IDeviceSessionPolicy>
                                            mDeviceSessionPolicy;

    int32_t                                 mUseAppImageSBProvider = 0;
    std::shared_ptr<AppImageStreamBufferProvider> mAppImageSBProvider = nullptr;

    mutable android::Mutex                  mPipelineModelLock;
    ::android::sp<pipeline::model::IPipelineModel>
                                            mPipelineModel = nullptr;
    //  setup during configuring streams
    mutable android::Mutex                  mStreamConfigCounterLock;
    MUINT32                                 mStreamConfigCounter = 1;

    //  setup during submitting requests.
    mutable android::Mutex                  mRequestingLock;
    MINT32                                  mRequestingAllowed = 0;
    bool                                    m1stRequestNotSent = true;

    //CPU Control
    Cam3CPUCtrl*                            mpCpuCtrl = nullptr;
    MUINT32                                 mCpuPerfTime = 1000;   //1 sec
    DisplayIdleDelayUtil                    mDisplayIdleDelayUtil;

    // linkToDeath
    // ::android::hidl::base::V1_0::DebugInfo  mLinkToDeathDebugInfo;

    // Callstack
    NSCam::Utils::CallStackLogger           mCallstaskLogger;

    // Prevent dump thread blocking
    std::future<void>                       mDumpFut;

    // scenario timestamp
    uint64_t                                mConfigTimestamp;

    // Config Maps
    mutable android::Mutex                              mImageConfigMapLock;
    ImageConfigMap   mImageConfigMap;
    MetaConfigMap    mMetaConfigMap;

    // scenario timestamp
    std::shared_ptr<ZoomRatioConverter>     mpZoomRatioConverter = nullptr;

    // A switch for early callback non-repeating metadata.
    bool                                    mEarlyCallbackMeta = false;

    // is logical Multicam device enabled
    bool                                    mEnableLogicalMulticam = false;

    // async closing
    std::vector<MINT32>                     mSensorIds;

    // boost from configure to first frame
    int mCpuHandle = 0;
    bool mIsBoosted = false;
    CustConfigTimeCPUCtrl mCustConfigTimeCPUCtrl;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Instantiation.
    virtual             ~CameraDevice3SessionImpl();
                        CameraDevice3SessionImpl(CreationInfo const& info);

public:     ////        Operations.
    auto const&         getDeviceInfo() const           { return *mStaticInfo.mStaticDeviceInfo; }
    auto const&         getInstanceName() const         { return getDeviceInfo().mInstanceName; }
    int32_t             getInstanceId() const           { return getDeviceInfo().mInstanceId; }
    auto                getLogLevel() const             { return mLogLevel; }
    auto const&         getLogPrefix() const            { return mLogPrefix; }

protected:  ////        Operations.
    auto                getSafeAppStreamManager() const -> ::android::sp<IAppStreamManager>;
    auto                getSafeDeviceSessionPolicy() const -> std::shared_ptr<device::policy::IDeviceSessionPolicy>;
    auto                getSafeAppConfigUtil() const -> ::android::sp<IAppConfigUtil>;
    auto                getSafeAppRequestUtil() const -> ::android::sp<IAppRequestUtil>;

    auto                getSafeHalBufHandler() const -> ::android::sp<IHalBufHandler>;
    auto                getSafePipelineModel() const -> ::android::sp<pipeline::model::IPipelineModel>;
    auto                flushAndWait() -> ::android::status_t;

protected:  ////        Operations.
    auto                enableRequesting() -> void;
    auto                disableRequesting() -> void;

protected:  ////        Operations.
    virtual auto        waitUntilOpenDoneLocked() -> bool;
    virtual auto        tryRunCommandLocked(nsecs_t const timeout, char const* commandName, CommandHandler::CommandT command) -> int;
    virtual auto        handleCommandFailureLocked(int status) -> void;

protected:  ////        [Template method] Operations.
    virtual auto        onOpenLocked(
                            const std::shared_ptr<CameraDevice3SessionCallback>& callback
                            ) -> ::android::status_t;

    virtual auto        onCloseLocked() -> void;

    virtual auto        onConfigureStreamsLocked(
                            const StreamConfiguration& requestedConfiguration,
                            HalStreamConfiguration& halConfiguration    //output
                        ) -> ::android::status_t;

    virtual auto        onFlushLocked() -> ::android::status_t;

    virtual auto        onProcessCaptureRequest(
                            const std::vector<CaptureRequest>& wrappedRequests,
                            // const hidl_vec<BufferCache>& cachesToRemove,
                            uint32_t& numRequestProcessed
                            ) -> ::android::status_t;

    virtual auto        prepareUserConfigurationParamsLocked(
                            const StreamConfiguration& requestedConfiguration,
                            HalStreamConfiguration& halConfiguration,
                            std::shared_ptr<pipeline::model::UserConfigurationParams>& rCfgParams
                            ) -> ::android::status_t;

    virtual auto        checkConfigParams(
                            const StreamConfiguration& requestedConfiguration
                            ) -> ::android::status_t;

    virtual auto        checkStream(
                            const Stream& rStream
                            ) -> ::android::status_t;

    virtual auto        getConfigImageStream(
                            StreamId_T streamId
                            ) const -> android::sp<AppImageStreamInfo>;

    virtual auto        checkRequestParams(
                            const std::vector<CaptureRequest>& vRequests
                            ) -> ::android::status_t;

    virtual auto        createUserRequestParamsLocked(
                            std::vector<std::shared_ptr<pipeline::model::UserRequestParams>>& rvReqParams
                            ) -> ::android::status_t;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NSCam::ICameraDevice3Session Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:

    virtual auto        dumpState(
                            ::android::Printer& printer,
                            const std::vector<std::string>& options
                            ) -> void;

    virtual auto        open(
                            const std::shared_ptr<CameraDevice3SessionCallback> callback
                        ) -> ::android::status_t;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CameraDevice3Session Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    virtual auto    constructDefaultRequestSettings(
                        RequestTemplate type
                    ) -> const camera_metadata_t* override;

    virtual auto    configureStreams(
                        const StreamConfiguration& requestedConfiguration,
                        HalStreamConfiguration& halConfiguration    //output
                    ) -> int override;

    virtual auto    processCaptureRequest(
                        const std::vector<CaptureRequest>& requests,
                        uint32_t& numRequestProcessed               //output
                    ) -> int override;

    virtual auto    flush() -> int override;
    virtual auto    close() -> int override;

    virtual auto    signalStreamFlush(
                        const std::vector<int32_t>& streamIds,
                        uint32_t streamConfigCounter
                    ) -> void override;

    virtual auto    isReconfigurationRequired(
                        const camera_metadata& oldSessionParams,
                        const camera_metadata& newSessionParams,
                        bool& reconfigurationNeeded                 //output
                    ) -> ::android::status_t override;

    virtual auto    switchToOffline(
                        const std::vector<int32_t>& streamsToKeep,
                        v3::CameraOfflineSessionInfo& offlineSessionInfo
                    ) -> int override;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineModelCallback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Operations.
    virtual auto    onFrameUpdated(
                        pipeline::model::UserOnFrameUpdated const& params
                    ) -> void;

    virtual auto    onMetaResultAvailable(
                        pipeline::model::UserOnMetaResultAvailable&& arg
                    ) -> void;

    virtual auto    onImageBufferReleased(
                        pipeline::model::UserOnImageBufferReleased&& arg __unused
                    ) -> void;

    virtual auto    onRequestCompleted(
                        pipeline::model::UserOnRequestCompleted&& arg __unused
                    ) -> void;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_DEVICE_DEPEND_CAMERADEVICE3SESSIONIMPL_H_

