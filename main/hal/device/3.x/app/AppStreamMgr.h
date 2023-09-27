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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_APP_APPSTREAMMGR_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_APP_APPSTREAMMGR_H_
//
#include "../include/IAppStreamManager.h"
//
// #include <mtkcam3/main/hal/device/utils/streambuffer/AppStreamBuffers.h>
// #include <mtkcam3/main/hal/device/utils/streaminfo/AppImageStreamInfo.h>
// #include <mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h>
#include <mtkcam/utils/gralloc/IGrallocHelper.h>
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/VsyncProvider/IVsyncProvider.h>
//
#include <atomic>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
//
#include <time.h>
//
#include <utils/BitSet.h>
#include <utils/Condition.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#include <utils/Thread.h>
//
// #include <fmq/MessageQueue.h>
// #include <hidl/MQDescriptor.h>

namespace NSCam {
namespace v3 {
/******************************************************************************
 *
 ******************************************************************************/

// Size of request metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr size_t CAMERA_REQUEST_METADATA_QUEUE_SIZE = 2 << 20 /* 2MB */;
// Size of result metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE  = 16 << 20 /* 32MB */;

/**
 * An implementation of App stream manager.
 */
class AppStreamMgr
    : public IAppStreamManager
{
    friend  class IAppStreamManager;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Common Info.

    using   AppImageStreamBuffer = NSCam::v3::Utils::AppImageStreamBuffer;
    // using   AppBufferHandleHolder = NSCam::v3::Utils::AppImageStreamBuffer::AppBufferHandleHolder;
    using   AppMetaStreamBuffer = NSCam::v3::Utils::AppMetaStreamBuffer;
    using   AppImageStreamInfo = NSCam::v3::Utils::Camera3ImageStreamInfo;
    using   AppMetaStreamInfo = NSCam::v3::Utils::MetaStreamInfo;


    struct  CommonInfo
    {
        int32_t                                     mLogLevel = 0;
        int32_t                                     mInstanceId = -1;
        std::shared_ptr<android::Printer>           mErrorPrinter = nullptr;
        std::shared_ptr<android::Printer>           mWarningPrinter = nullptr;
        std::shared_ptr<android::Printer>           mDebugPrinter = nullptr;
        std::weak_ptr<CameraDevice3SessionCallback> mDeviceCallback;
        android::sp<IMetadataProvider>              mMetadataProvider = nullptr;
        std::map<uint32_t, android::sp<IMetadataProvider>> mPhysicalMetadataProviders;
        android::sp<IMetadataConverter>             mMetadataConverter = nullptr;
        IGrallocHelper*                             mGrallocHelper = nullptr;
        size_t                                      mAtMostMetaStreamCount = 0;
    };

    std::shared_ptr<CommonInfo> mCommonInfo = nullptr;


public:     ////    Debugging

    struct  MyDebuggee : public IDebuggee
    {
        static const std::string        mName;
        std::shared_ptr<IDebuggeeCookie>mCookie = nullptr;
        android::wp<AppStreamMgr>       mContext = nullptr;

                        MyDebuggee(AppStreamMgr* p) : mContext(p) {}
        virtual auto    debuggeeName() const -> std::string { return mName; }
        virtual auto    debug(
                            android::Printer& printer,
                            const std::vector<std::string>& options
                        ) -> void;
    };


public:     ////    Callback

    struct  CallbackParcel
    {
        struct  ImageItem
        {
            android::sp<AppImageStreamBuffer>       buffer;
            android::sp<AppImageStreamInfo>         stream;
        };

        struct  MetaItem
        {
            android::sp<IMetaStreamBuffer>          buffer;
            uint32_t                                bufferNo = 0; //partial_result
        };

        struct  PhysicMetaItem
        {
            android::sp<IMetaStreamBuffer>          buffer;
            std::string                             camId;
        };

        struct  Error
        {
            android::sp<AppImageStreamInfo>         stream;
            ErrorCode                               errorCode = ErrorCode::ERROR_REQUEST;
        };

        struct  Shutter
            : public android::LightRefBase<Shutter>
        {
            uint64_t                                timestamp = 0;
        };

        android::Vector<ImageItem>                  vInputImageItem;
        android::Vector<ImageItem>                  vOutputImageItem;
        android::Vector<MetaItem>                   vOutputMetaItem;
        android::Vector<PhysicMetaItem>             vOutputPhysicMetaItem;
        android::Vector<Error>                      vError;
        android::sp<Shutter>                        shutter;
        uint64_t                                    timestampShutter = 0;
        uint32_t                                    requestNo = 0;
        bool                                        valid = false;
        bool                                        needIgnore = false;
        bool                                        needRemove = false;
    };


public:     ////    Result Queue

    struct  ResultItem
        : public android::LightRefBase<ResultItem>
    {
        uint32_t                                    requestNo = 0;
        bool                                        lastPartial = false; // partial metadata
        RealTime                                    realTimePartial = RealTime::NON;
        android::Vector<android::sp<IMetaStreamBuffer>>
                                                    buffer;
        int64_t                                     timestampStartOfFrame = 0; //SOF for De-jitter
        android::KeyedVector<std::string, android::Vector<android::sp<IMetaStreamBuffer>>>
                                                    physicBuffer;   //use physica cam ID as key
    };

    using   ResultQueueT = android::KeyedVector<uint32_t, android::sp<ResultItem>>;


public:     ////

    //             +-----------------------------------------------------------+
    //             |                    Camera Frameworks                      |
    //             +-----------------------------------------------------------+
    //                     ^                                   ^
    //              +-------------+                     +-------------+     +-------------+
    //              |  MsgCbUtil  |<--------------------|  ResCbUtil  |<----| CamDejitter |
    //              +-------------+  [shutter/message]  +-------------+     +-------------+
    //                                                         ^
    //                                          + -------------| < [metadata/image]
    //                                          |
    //                                 +-----------------+   +-----------------+
    //                                 | CallbackHandler |   |  HalBufHandler  | < [IImageStreamBufferProvider]
    //                                 +-----------------+   +-----------------+
    //                                          ^                     v
    //                                 +-----------------+            |
    //             +---------------+   |   BatchHandler  |<-----------+
    //  [config] > | ConfigHandler | > |        ^        |            v
    //             +---------------+   |                 |   +----------------+
    //                                 |   FrameHandler  | < | RequestHandler | < [request]
    //                                 +-----------------+   +----------------+
    //                                          ^
    //                                 +----------------+
    //                                 |  ResultHandler |
    //                                 +----------------+
    //                                          ^
    //                                      [result]
    class   ResultHandler;
    class   FrameHandler;
    class   BatchHandler;
    class   CallbackHandler;
    class   HalBufHandler;
    class   CamDejitter;
    class   CamDejitterImp;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                Data Members.
    std::string const           mInstanceName;
    std::shared_ptr<MyDebuggee> mDebuggee = nullptr;
    mutable android::Mutex      mInterfaceLock;

protected:  ////                Data Members (CONFIG/REQUEST/RESULT/FRAME/CALLBACK)
    android::sp<ResultHandler>     mResultHandler   = nullptr;
    android::sp<FrameHandler>      mFrameHandler    = nullptr;
    android::sp<BatchHandler>      mBatchHandler    = nullptr;
    android::sp<CallbackHandler>   mCallbackHandler = nullptr;
    std::shared_ptr<CamDejitter>   mCamDejitter     = nullptr;
    // android::sp<HalBufHandler>  mHalBufHandler = nullptr;

    std::shared_ptr<IStreamInfoBuilderFactory>
                                mStreamInfoBuilderFactory;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Operations.
    auto            getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }
    auto            dumpStateLocked(android::Printer& printer, const std::vector<std::string>& options) -> void;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IAppStreamManager Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.
                    AppStreamMgr(const CreationInfo& creationInfo);

    auto            initialize() -> bool;

public:     ////    IAppStreamManager Interfaces.

    virtual auto    destroy() -> void;

    virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;


    virtual auto    setConfigMap(
                        ImageConfigMap& imageConfigMap,
                        MetaConfigMap& metaConfigMap
                        )  -> void;

    virtual auto    configureStreams(
                        const StreamConfiguration& requestedConfiguration,
                        /*const HalStreamConfiguration& halConfiguration    __unused,*/
                        const ConfigAppStreams& rStreams
                        ) -> int;

    virtual auto    flushRequest(
                        const std::vector<CaptureRequest>& requests
                    )   -> void;

    virtual auto    updateResult(UpdateResultParams const& params) -> void override;

    virtual auto    updateAvailableMetaResult(const android::sp<UpdateAvailableParams>& param) -> void override;

    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int;


    virtual auto    submitRequests(
                        /*const std::vector<CaptureRequest>& requests,*/
                        const android::Vector<Request>& rRequests
                    )   -> int;

    virtual auto    updateStreamBuffer(
                        uint32_t frameNo,
                        StreamId_T streamId,
                        android::sp<IImageStreamBuffer> const pBuffer
                    )   -> int;

    virtual auto    abortRequest(
                        const android::Vector<Request>& requests
                        ) -> void;

    virtual auto    earlyCallbackMeta(
                        const Request& request
                        ) -> void;

    virtual auto    getAppStreamInfoBuilderFactory() const -> std::shared_ptr<IStreamInfoBuilderFactory>;

    // virtual auto    getHalBufManagerStreamProvider(::android::sp<IAppStreamManager> pAppStreamManager) -> std::shared_ptr<IImageStreamBufferProvider>;
    // virtual auto    requestStreamBuffer( android::sp<IImageStreamBuffer>& rpImageStreamBuffer, IImageStreamBufferProvider::RequestStreamBuffer const& in) -> int;
    virtual auto    markStreamError(uint32_t requestNo, StreamId_T streamId) -> void;
    // virtual auto    waitHalBufCntAvailable(const std::vector<CaptureRequest>& requests) -> void;

    virtual auto    flush() -> void;
};


/**
 * Result Handler
 */
class AppStreamMgr::ResultHandler
    : public android::Thread
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////
    std::string const           mInstanceName;
    std::shared_ptr<CommonInfo> mCommonInfo = nullptr;
    android::sp<FrameHandler>   mFrameHandler = nullptr;

protected:  ////
    mutable android::Mutex      mResultQueueLock;
    android::Condition          mResultQueueCond;
    ResultQueueT                mResultQueue;

protected:  ////

    auto            getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }
    auto            dequeResult(ResultQueueT& rvResult) -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Thread Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual auto    requestExit() -> void;

    // Good place to do one-time initializations
    virtual auto    readyToRun() -> android::status_t;

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual auto    threadLoop() -> bool;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
                    ResultHandler(
                        std::shared_ptr<CommonInfo> pCommonInfo,
                        android::sp<FrameHandler> pFrameHandler
                    );

    virtual auto    destroy() -> void;

    auto            enqueResult(UpdateResultParams const& params) -> int;

};


/**
 * Frame Handler
 */
class AppStreamMgr::FrameHandler
    : public android::RefBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions:
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    struct  FrameParcel;
    struct  MetaItem;
    struct  MetaItemSet;
    struct  PhysicMetaItem;
    struct  PhysicMetaItemSet;
    struct  ImageItem;
    struct  ImageItemSet;

    /**
     * IN_FLIGHT    -> PRE_RELEASE
     * IN_FLIGHT    -> VALID
     * IN_FLIGHT    -> ERROR
     * PRE_RELEASE  -> VALID
     * PRE_RELEASE  -> ERROR
     */
    struct  State
    {
        enum T
        {
            IN_FLIGHT,
            PRE_RELEASE,
            VALID,
            ERROR,
        };
    };

    struct  HistoryBit
    {
        enum
        {
            RETURNED,
            ERROR_SENT_FRAME,
            ERROR_SENT_META,
            ERROR_SENT_IMAGE,
        };
    };

    enum class RequestType : uint32_t {
        NORMAL = 0,
        ZSL_STILL_CAPTURE = 1,
        REPROCESSING = 2,
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions: Meta Stream
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    struct  MetaItem
        : public android::RefBase
    {
        FrameParcel*                        pFrame = nullptr;
        MetaItemSet*                        pItemSet = nullptr;
        State::T                            state = State::IN_FLIGHT;
        android::BitSet32                   history;//HistoryBit::xxx
        android::sp<IMetaStreamBuffer>      buffer;
        MUINT32                             bufferNo = 0;//partial_result
    };

    struct  MetaItemSet
        : public android::DefaultKeyedVector<StreamId_T, android::sp<MetaItem> >
    {
        bool                                asInput;
        bool                                hasLastPartial;
        RealTime                            realTimePartial;
        size_t                              numReturnedStreams;
        size_t                              numValidStreams;
        size_t                              numErrorStreams;
                                            //
                                            MetaItemSet(MBOOL _asInput)
                                                : asInput(_asInput)
                                                , hasLastPartial(false)
                                                , realTimePartial(RealTime::NON)
                                                , numReturnedStreams(0)
                                                , numValidStreams(0)
                                                , numErrorStreams(0)
                                            {}
    };

    struct  PhysicMetaItem
        : public android::RefBase
    {
        FrameParcel*                        pFrame = nullptr;
        PhysicMetaItemSet*                  pItemSet = nullptr;
        State::T                            state = State::IN_FLIGHT;
        android::sp<IMetaStreamBuffer>      buffer;
        MUINT32                             bufferNo = 0;//partial_result
        std::string                         physicCameraId;
    };

    struct  PhysicMetaItemSet
        : public android::DefaultKeyedVector<StreamId_T, android::sp<PhysicMetaItem> >
    {
        size_t                              numReturnedStreams;
        size_t                              numValidStreams;
        size_t                              numErrorStreams;
                                            //
                                            PhysicMetaItemSet()
                                                : numReturnedStreams(0)
                                                , numValidStreams(0)
                                                , numErrorStreams(0)
                                            {}
    };

    struct  MetaConfigItem
    {
        android::sp<AppMetaStreamInfo>      pStreamInfo;
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions: Image Stream
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    struct  ImageItemFrameQueue
        : public android::List<android::sp<ImageItem> >
    {
                                            ImageItemFrameQueue()
                                            {}
    };

    struct  ImageItem
        : public android::RefBase
    {
        FrameParcel*                        pFrame = nullptr;
        ImageItemSet*                       pItemSet = nullptr;
        State::T                            state = State::IN_FLIGHT;
        android::BitSet32                   history;//HistoryBit::xxx
        android::sp<AppImageStreamBuffer>   buffer = nullptr;
        bool                                buffer_error=false;
        StreamId_T                          streamId;
        ImageItemFrameQueue::iterator       iter;
    };

    struct  ImageItemSet
        : public android::DefaultKeyedVector<StreamId_T, android::sp<ImageItem> >
    {
        MBOOL                               asInput;
        size_t                              numReturnedStreams;
        size_t                              numValidStreams;
        size_t                              numErrorStreams;
                                            //
                                            ImageItemSet(MBOOL _asInput)
                                                : asInput(_asInput)
                                                , numReturnedStreams(0)
                                                , numValidStreams(0)
                                                , numErrorStreams(0)
                                            {}
    };

    struct  ImageConfigItem
    {
        android::sp<AppImageStreamInfo>     pStreamInfo;
        ImageItemFrameQueue                 vItemFrameQueue;
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions: Frame Parcel & Queue
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    struct  FrameParcel
        : public android::RefBase
    {
        ImageItemSet                        vOutputImageItem = MFALSE;
        ImageItemSet                        vInputImageItem = MTRUE;
        MetaItemSet                         vOutputMetaItem = MFALSE;
        PhysicMetaItemSet                   vOutputPhysicMetaItem;
        MetaItemSet                         vInputMetaItem = MTRUE;
        MUINT32                             requestNo = 0;
        android::BitSet32                   errors; //HistoryBit::ERROR_SENT_xxx
        MUINT64                             shutterTimestamp = 0;
        bool                                bShutterCallbacked = false;
        RequestType                         eRequestType = RequestType::NORMAL;
        timespec                            requestTimestamp{0, 0};
        int64_t                             startTimestamp = 0; //used in De-jitter
    };

    struct  FrameQueue
        : public android::List<android::sp<FrameParcel> >
    {
        MUINT32                             latestResultRequestNo = 0;
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Data Members.
    std::string const                                   mInstanceName;
    std::shared_ptr<CommonInfo>                         mCommonInfo = nullptr;
    android::sp<BatchHandler>                           mBatchHandler = nullptr;
    bool                                                bIsDejitterEnabled = false;
    bool                                                bEnableMetaPending = false;
    bool                                                bEnableMetaHitched = false;

protected:  ////    Data Members (CONFIG/REQUEST)
    mutable android::Mutex                              mFrameQueueLock;
    android::Condition                                  mFrameQueueCond;
    FrameQueue                                          mFrameQueue;

    mutable android::Mutex                              mImageConfigMapLock;
    android::KeyedVector<StreamId_T, ImageConfigItem>   mImageConfigMap;
    // android::KeyedVector<StreamId_T, MetaConfigItem>    mMetaConfigMap;
    uint32_t                                            mOperationMode = 0xffffffff;

    //shutter notify callback related
    mutable android::Mutex                              mShutterQueueLock;
    android::KeyedVector<MUINT32, android::wp<FrameParcel>>      mShutterQueue;

    /**
     * For MetaHitched, to record if there is last partial updated in updateResult.
     * If yes, the soft realtime result can be prepared to callback in updateCallback.
     * Otherwise, the soft realtime result should keep pending.
     */
    bool                                                bHitchable = false;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations:
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Operations.
    auto            dumpStateLocked(android::Printer& printer) const -> void;
    auto            dumpLocked() const -> void;
    auto            dump() const -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations: Request
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Operations.
    virtual auto    registerStreamBuffers(
                        android::KeyedVector<
                            StreamId_T,
                            android::sp<AppImageStreamBuffer>
                                            > const& buffers,
                        android::sp<FrameParcel> const pFrame,
                        ImageItemSet*const pItemSet
                    )   -> int;

    virtual auto    registerStreamBuffers(
                        android::KeyedVector<
                            StreamId_T,
                            android::sp<IImageStreamInfo>
                                            > const& buffers,
                        android::sp<FrameParcel> const pFrame,
                        ImageItemSet*const pItemSet
                    )   -> int;

    virtual auto    registerStreamBuffers(
                        android::KeyedVector<
                            StreamId_T,
                            android::sp<IMetaStreamBuffer>
                                            > const& buffers,
                        android::sp<FrameParcel> const pFrame,
                        MetaItemSet*const pItemSet
                    )   -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations: Result
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Operations.

    /**
     * @param[in] frame: a given frame to check.
     *
     * @return
     *      ==0: uncertain
     *      > 0: it is indeed a request error
     *      < 0: it is indeed NOT a request error
     */
    MINT    checkRequestError(FrameParcel const& frame);

    bool    removeEmptyResult(ResultQueueT & rvResult);

protected:  ////    Operations.
    virtual auto    prepareErrorFrame(
                        CallbackParcel& rCbParcel,
                        android::sp<FrameParcel> const& pFrame
                    )   -> void;

    virtual auto    prepareErrorMetaIfPossible(
                        CallbackParcel& rCbParcel,
                        android::sp<MetaItem> const& pItem
                    )   -> void;

    virtual auto    prepareErrorImage(
                        CallbackParcel& rCbParcel,
                        android::sp<ImageItem> const& pItem
                    )   -> void;

protected:  ////    Operations.
    virtual auto    isShutterReturnable(
                        android::sp<MetaItem> const& pItem
                    )   const -> bool;

    virtual auto    prepareShutterNotificationIfPossible(
                        CallbackParcel& rCbParcel,
                        android::sp<MetaItem> const& pItem
                    )   -> bool;

    virtual auto    updateShutterTimeIfPossible(
                            android::sp<MetaItem> &pItem
                        )   -> void;

    virtual auto    prepareReturnMeta(
                        CallbackParcel& rCbParcel,
                        android::sp<MetaItem> const& pItem
                    )   -> void;

    virtual auto    preparePhysicReturnMeta(
                        CallbackParcel& rCbParcel,
                        android::sp<FrameParcel> const& rFrameParcel
                    )   -> void;

    virtual auto    isReturnable(
                        android::sp<MetaItem> const& pItem,
                        android::String8& debug_log
                    )   const -> bool;

protected:  ////    Operations.
    virtual auto    prepareReturnImage(
                        CallbackParcel& rCbParcel,
                        android::sp<ImageItem> const& pItem
                    )   -> void;

    virtual auto    isReturnable(
                        android::sp<ImageItem> const& pItem
                    )   const -> bool;

protected:  ////    Operations.
    virtual auto    isFrameRemovable(
                        android::sp<FrameParcel> const& pFrame
                    )   const -> bool;

    virtual auto    prepareCallbackIfPossible(
                        CallbackParcel& rCbParcel,
                        MetaItemSet& rItemSet,
                        android::String8& debug_log
                    )   -> bool;

    virtual auto    prepareCallbackIfPossible(
                        CallbackParcel& rCbParcel,
                        ImageItemSet& rItemSet
                    )   -> bool;

protected:  ////    Operations.
    virtual auto    updateItemSet(MetaItemSet& rItemSet) -> void;
    virtual auto    updateItemSet(PhysicMetaItemSet& rItemSet) -> void;
    virtual auto    updateItemSet(ImageItemSet& rItemSet) -> void;
    virtual auto    updateResult(ResultQueueT const& rvResult) -> void;
    virtual auto    updateCallback(std::list<CallbackParcel>& rCbList) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
                    FrameHandler(
                        std::shared_ptr<CommonInfo> pCommonInfo,
                        android::sp<BatchHandler> pBatchHandler
                    );

    virtual auto    destroy() -> void;

    virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;

    virtual auto    setOperationMode(uint32_t operationMode) -> void;

    virtual auto    removeUnusedConfigStream(std::unordered_set<StreamId_T>const& usedStreamIds) -> void;
    // virtual auto    addConfigStream(AppMetaStreamInfo* pStreamInfo) -> void;
    virtual auto    getConfigImageStream(StreamId_T streamId) const -> android::sp<AppImageStreamInfo>;
    // virtual auto    getConfigMetaStream(StreamId_T streamId) const -> android::sp<AppMetaStreamInfo>;

    virtual auto    setConfigMap(
                        ImageConfigMap& imageConfigMap,
                        MetaConfigMap& metaConfigMap
                        )  -> void;

    virtual auto    registerFrame(Request const& rRequest) -> int;

    virtual auto    update(ResultQueueT const& rvResult) -> void;

    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int;

    virtual auto    updateStreamBuffer(uint32_t requestNo, StreamId_T streamId,
                        android::sp<IImageStreamBuffer> const pBuffer)   -> int;

    virtual auto    dumpIfHasInflightRequest() const -> void;

};


/**
 * Batch Handler
 */
class AppStreamMgr::BatchHandler
    : public android::RefBase
{
    friend  class AppStreamMgr;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
        struct InFlightBatch {
            // Protect access to entire struct. Acquire this lock before read/write any data or
            // calling any methods. processCaptureResult and notify will compete for this lock
            // HIDL IPCs might be issued while the lock is held
            mutable android::Mutex  mLock;

            uint32_t                mBatchNo;
            uint32_t                mFirstFrame;
            uint32_t                mLastFrame;
            uint32_t                mBatchSize;
            std::vector<uint32_t>   mHasLastPartial;
            std::vector<uint32_t>   mShutterReturned;
            std::vector<uint32_t>   mFrameRemoved;
            //bool                    mRemoved = false;
            uint32_t                mFrameHasMetaResult = 0;
            uint32_t                mFrameHasImageResult = 0;
            uint32_t                mFrameRemovedCount = 0;
       };

protected:  ////
    std::string const               mInstanceName;
    std::shared_ptr<CommonInfo>     mCommonInfo = nullptr;
    android::sp<CallbackHandler>    mCallbackHandler = nullptr;

    // to record id of batched streams.
    std::vector<uint32_t>           mBatchedStreams;
    uint32_t                        mBatchCounter;
    mutable android::Mutex          mLock;
    std::vector<std::shared_ptr<InFlightBatch>>
                                    mInFlightBatches;
    mutable android::Mutex          mMergedParcelLock;
    std::list<CallbackParcel>       mMergedParcels;

public:
    enum { NOT_BATCHED = -1 };

protected:  ////

    auto            getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
                    BatchHandler(
                        std::shared_ptr<CommonInfo> pCommonInfo,
                        android::sp<CallbackHandler> pCallbackHandler
                    );

    virtual auto    destroy() -> void;

    virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;

    virtual auto    dumpStateLocked(android::Printer& printer) const -> void;

    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int;

    virtual auto    push(std::list<CallbackParcel>& item) -> void;

    // Operations to be modified.
    virtual auto resetBatchStreamId() -> void;

    virtual auto checkStreamUsageforBatchMode(const android::sp<AppImageStreamInfo>) -> bool;

    virtual auto registerBatch(const android::Vector<Request>) -> int;

    virtual auto getBatchLocked(uint32_t frameNumber) -> std::shared_ptr<InFlightBatch>;

    virtual auto removeBatchLocked(uint32_t batchNum) -> void;

    virtual auto appendParcel(CallbackParcel srcParcel,CallbackParcel &dstParcel) -> void;

    virtual auto updateCallback(std::list<CallbackParcel> cbParcels, std::list<CallbackParcel>& rUpdatedList) -> void;
};


/**
 * Callback Handler
 */

struct   DejitterInfo {
    bool hasDisplayBuf = false;
    uint32_t frameNumber = 0;
    int64_t targetTime = 0;
};
struct HidlCallbackParcel
{
    std::vector<uint32_t> vRequestNo;
    std::vector<NotifyMsg> vNotifyMsg;
    std::vector<NotifyMsg> vErrorMsg;
    std::vector<CaptureResult> vCaptureResult;
    std::vector<CaptureResult> vBufferResult;
    std::vector<camera_metadata*> vTempMetadataResult;
    DejitterInfo mDejitterInfo;
};

struct HidlCallbackQueue
    : public std::list<HidlCallbackParcel>
{
};

class AppStreamMgr::CallbackHandler
    : public android::RefBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions:
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:

    struct  CallbackQueue
        : public std::list<CallbackParcel>
    {
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Data Members.
    std::string const                       mInstanceName;
    std::shared_ptr<CommonInfo>             mCommonInfo = nullptr;
    // android::sp<HalBufHandler>              mHalBufHandler = nullptr;
    std::shared_ptr<CamDejitter>            mCamDejitter = nullptr;

    // std::shared_ptr<ResultMetadataQueue>    mResultMetadataQueue = nullptr;
    bool                                    bEnableMetaMerge;
    bool                                    bEnableDejitter;

protected:  ////    Operations.
    auto            getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }

    auto            convertCallbackParcelToHidl(std::list<CallbackParcel>& item, HidlCallbackQueue& res, HidlCallbackParcel& msg) -> void;

    // auto            convertShutterToHidl(CallbackParcel const& cbParcel, std::vector<NotifyMsg>& rvMsg) -> void;
    // auto            convertErrorToHidl(CallbackParcel const& cbParcel, std::vector<NotifyMsg>& rvMsg) -> void;
    // auto            convertMetaToHidl(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void;
    // auto            convertPhysicMetaToHidl(CallbackParcel const& cbParcel, CaptureResult& rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void;
    // auto            convertMetaToHidlWithMergeEnabled(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void;
    // auto            convertImageToHidl(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult) -> void;

protected:
    auto            convertShutterResult(CallbackParcel const& cbParcel, std::vector<NotifyMsg>& rvMsg) -> void;
    auto            convertErrorResult(CallbackParcel const& cbParcel, std::vector<NotifyMsg>& rvMsg) -> void;
    auto            convertMetaResult(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void;
    auto            convertPhysicMetaResult(CallbackParcel const& cbParcel, CaptureResult& rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void;
    auto            convertMetaResultWithMergeEnabled(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult, std::vector<camera_metadata*>& rvResultMetadata) -> void;
    auto            convertImageResult(CallbackParcel const& cbParcel, std::vector<CaptureResult>& rvResult) -> void;
    auto            convertImageResultWithDejitter(CallbackParcel const& cbParcel, HidlCallbackQueue& rHCbParcels) -> void;


protected:  ////    Operations (Trace)
    auto            traceDisplayIf(
                        uint32_t requestNo,
                        uint64_t timestampShutter,
                        const CallbackParcel::ImageItem& item
                    )   -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CallbackHelper
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    class CallbackHelper
        : public android::Thread
    {
    public:
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Thread Interfaces.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    public:     ////
        // Ask this object's thread to exit. This function is asynchronous, when the
        // function returns the thread might still be running. Of course, this
        // function can be called from a different thread.
        virtual auto    requestExit() -> void;

        // Good place to do one-time initializations
        virtual auto    readyToRun() -> android::status_t;

    private:
        // Derived class must implement threadLoop(). The thread starts its life
        // here. There are two ways of using the Thread object:
        // 1) loop: if threadLoop() returns true, it will be called again if
        //          requestExit() wasn't called.
        // 2) once: if threadLoop() returns false, the thread will exit upon return.
        virtual auto    threadLoop() -> bool;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Operations.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    protected:
        auto            waitUntilQueue1NotEmpty() -> bool;

        auto            performCallback() -> void;

        auto            getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Interfaces.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    public:     ////    Operations.

                        CallbackHelper(
                            std::shared_ptr<CommonInfo> pCommonInfo,
                            std::string const sInstanceName,
                            android::sp<CallbackHandler> pCallbackHandler,
                            android::sp<CallbackHelper> pCallbackHelper_Msg,
                            std::shared_ptr<CamDejitter> pCamDejitter
                        );

        virtual void    push(HidlCallbackParcel& hCbParcel);

        virtual void    push(HidlCallbackQueue& hCbParcel);

        virtual auto    destroy() -> void;

        virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;

        virtual auto    waitUntilDrained(nsecs_t const timeout) -> int;
    private:
        std::string const                       mInstanceName;
        std::shared_ptr<CommonInfo>             mCommonInfo = nullptr;

        mutable android::Mutex                  mQueue1Lock;
        android::Condition                      mQueue1Cond;
        HidlCallbackQueue                       mQueue1;

        mutable android::Mutex                  mQueue2Lock;
        android::Condition                      mQueue2Cond;
        HidlCallbackQueue                       mQueue2;

        android::sp<CallbackHandler>    mCallbackHandler = nullptr;
        std::shared_ptr<CamDejitter>        mCamDejitter = nullptr;
        android::sp<CallbackHelper>  mCallbackHelper_Msg = nullptr;
        bool mCbHasRes = false;
    };

    android::sp<CallbackHelper>  mResCbHelper = nullptr;
    android::sp<CallbackHelper>  mMsgCbHelper = nullptr;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.

                    CallbackHandler(std::shared_ptr<CommonInfo> pCommonInfo,
                                    std::shared_ptr<CamDejitter> pCamDejitter);

                    ~CallbackHandler(){};

    virtual auto    destroy() -> void;

    virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;

    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int;

    virtual auto    push(std::list<CallbackParcel>& item) -> void;

};

class AppStreamMgr::CamDejitter
: public std::enable_shared_from_this<AppStreamMgr::CamDejitter> {
 public:
  static auto create(std::shared_ptr<CommonInfo> pCommonInfo)
    -> std::shared_ptr<AppStreamMgr::CamDejitter>;

  CamDejitter() {}
  virtual ~CamDejitter() {}

  virtual auto destroy()
      -> void = 0;

  virtual auto enable()
      -> bool {return mEnable;}

  virtual auto storeSofTimestamp(uint32_t framenumber, int64_t timestamp)
      -> void = 0;

  virtual auto scanSrcQueue(HidlCallbackQueue& srcQueue)
      -> void = 0;

  virtual auto splice(HidlCallbackQueue& dstQueue, HidlCallbackQueue& srcQueue)
      -> void = 0;

  virtual auto waitForDejitter()
      -> bool = 0;

  virtual auto wakeupAndRefresh()
      -> void = 0;

  virtual auto postCallback()
      -> void = 0;

  virtual auto flush()
      -> void = 0;

  virtual auto configure(
    const StreamConfiguration& requestedConfiguration)
      -> bool = 0;

  virtual auto getDejitterStreamId()
      -> int32_t {return mEnable ? mDejitterStreamId : -1;}

 protected:
  std::atomic<bool> mEnable = false;
  int32_t mDejitterStreamId = -1;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CamDejitterImp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class AppStreamMgr::CamDejitterImp
: public AppStreamMgr::CamDejitter {
 public:
  explicit CamDejitterImp(std::shared_ptr<CommonInfo> pCommonInfo);
  ~CamDejitterImp();

  auto destroy()
      -> void {}

  auto storeSofTimestamp(uint32_t framenumber, int64_t timestamp)
      -> void;

  auto scanSrcQueue(HidlCallbackQueue& srcQueue)
      -> void;

  auto splice(HidlCallbackQueue& dstQueue, HidlCallbackQueue& srcQueue)
      -> void;

  auto waitForDejitter()
      -> bool;

  auto wakeupAndRefresh()
      -> void;

  auto postCallback()
      -> void;

  auto flush()
      -> void;

  auto configure(
    const StreamConfiguration& requestedConfiguration)
      -> bool;

 protected:
  std::string const mInstanceName;
  std::shared_ptr<CommonInfo> mCommonInfo = nullptr;

  std::condition_variable mWakeupCond;
  std::mutex mWakeupLock;
  bool mRefresh = false;
  std::mutex mTimestampMapLock;
  std::map<uint32_t, int64_t> mSOFMap;  // <framenumber, timestamp(ns)>
  uint32_t mTargetFramenumber = 0;
  bool mDebugLogOn = false;

  int64_t mTargetTime = 0;
  int64_t mLastEstimateLatency = 0;
  float mLastCovariance = 0.0f;
  uint64_t mLastSOF = 0;
  int64_t mCushionTime = 8300000;

  auto resetFilter()
      -> void;
  auto kalmanFilter(int64_t measuredLatency)
      -> int64_t;
  auto tryActivateHalDejitter(const StreamConfiguration& rStreams)
      -> bool;
};

};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_APP_APPSTREAMMGR_H_

