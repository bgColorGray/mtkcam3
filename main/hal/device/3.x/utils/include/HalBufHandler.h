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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_HALBUFFERHANDLER_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_HALBUFFERHANDLER_H_
//
#include <IHalBufHandler.h>
//
#include <utils/Thread.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/Condition.h>
//
#include <mtkcam3/pipeline/utils/streambuf/StreamBuffers.h>
#include <mtkcam/utils/gralloc/IGrallocHelper.h>
// #include <mtkcam3/pipeline/stream/IStreamInfoBuilder.h>
//
#include <map>
#include <memory>
#include <list>
//

using namespace NSCam::v3::Utils;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {

/**
 * HalBufHandler
 */
class HalBufHandler
    : public IHalBufHandler
    , public android::Thread
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions:
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:  ////

    // CommonInfo
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

    const int HAL_BUF_MIN_NUM = 1;

    // allocate buffers may timeout
    // this value sync with /frameworks/av/services/camera/libcameraservice/device3/Camera3Stream.h
    //          static const nsecs_t kWaitForBufferDuration = 3000000000LL; // 3000 ms
    const nsecs_t HAL_BUF_ALLOC_TIMEOUT_NS = 3000000000LL;

public:

    using HalBuf            = StreamBuffer;
    using HalBufSP          = std::shared_ptr<HalBuf>;
    using HalBufPool        = std::list<HalBufSP>;
    using HalBufPoolSP      = std::shared_ptr<HalBufPool>;
    using HalBufPoolMap     = std::map<StreamId_T,HalBufPoolSP>;
    using HalBufImportMap   = std::map<std::pair<StreamId_T,uint64_t>, HalBufSP>;
    using HalCntMap         = std::map<StreamId_T,int>;
    using HalCntMapSP       = std::shared_ptr<HalCntMap>;
    using HalCntFrameMap    = std::map<uint32_t, HalCntMapSP>;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Data Members.
    std::string const                       mInstanceName;
    std::shared_ptr<CommonInfo>             mCommonInfo = nullptr;

    android::Mutex                          mBufLock; // mutex for handle mBuf* members
    android::Condition                      mBufCond; // notify mBuf* status changed
    int                                     mBufMinNum = 1; // Minial buffers should keep in Pool
    HalBufPoolMap                           mBufPoolMap; // Pool list of buffer
    HalBufImportMap                         mHalBufImportMap; // map to record buffers which not import yet

    android::Mutex                          mCntLock; // mutex for handle mCnt* members
    HalCntMap                               mCntMaxMap; // streamId -> max stream buf num
    HalCntMap                               mCntAllocMap; // streamId -> current allocated of hal buf num (from HIDL)
    HalCntMap                               mCntInflightMap; // streamId -> current request num
    // mutable android::Mutex                  mInterfaceLock;

    std::shared_ptr<IIonDevice>             mIonDevice = nullptr;
    android::sp<IAppStreamManager>          mAppStreamMgr;
    // android::sp<IAppRequestUtil>            mAppRequestUtil;

protected:  ////    Operations.
    inline auto     getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }

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

    auto            _allocHalBuf(StreamId_T streamId, int num, HalBufPoolSP poolSP) -> int;
    auto            _dequeueHalBufPool(StreamId_T streamId, HalBufSP &rpStreamBufferSP) -> bool;
    auto            _releaseAllHalBuf(StreamId_T streamId, HalBufPoolSP poolSP) -> void;

    // Maintain Request counter
    // increase when receive request
    // decrease when buffers in performCallback

    auto            _getHalBufMaxCnt(StreamId_T streamId) -> int; // get max counter

    auto            _incHalBufAllocCnt(StreamId_T streamId, int cnt) -> int;
    auto            _decHalBufAllocCnt(StreamId_T streamId, int cnt) -> int;
    auto            _getHalBufAllocCnt(StreamId_T streamId) -> int; // get used counter

    auto            _incHalBufInflightCnt(StreamId_T streamId, int cnt) -> int;
    auto            _decHalBufInflightCnt(StreamId_T streamId, int cnt) -> int;
    auto            _getHalBufInflightCnt(StreamId_T streamId) -> int; // get used counter

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
                    HalBufHandler(
                        const CreationInfo& creationInfo,
                        android::sp<IAppStreamManager> pAppStreamMgr/*,
                        android::sp<IAppRequestUtil> pAppRequestUtil*/
                    );

    virtual auto    destroy() -> void;
    virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;
    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int;

    // virtual auto    getHalBufManagerStreamProvider() -> std::shared_ptr<IImageStreamBufferProvider>;

    virtual auto    requestStreamBuffer(
                        IImageStreamBufferProvider::requestStreamBuffer_cb cb,
                        IImageStreamBufferProvider::RequestStreamBuffer const& in
                    ) -> int;

    // virtual auto    waitHalBufCntAvailable(const std::vector<CaptureRequest>& requests) -> void;

    auto            allocHalBuf(HalBufSP &rpStreamBufferSP, IImageStreamBufferProvider::RequestStreamBuffer const& in) -> int;
    // auto            decHalBufReqCntByFrame(uint32_t requestNo) -> void;

    // "Todo: @endConfigureStreams"
    auto            setHalBufMaxCnt(const HalStreamConfiguration& halConfiguration) -> void; // configuration max count
    // "Todo: @submitRequest"
    auto            updateHalBufCnt(const std::vector<CaptureRequest>& requests) -> void; // update counting with requests
    // "Todo: @performCallback"
    auto            updateHalBufCnt(const std::vector<CaptureResult>& vBufferResult) -> void; // update counting with results

};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_HALBUFFERHANDLER_H_