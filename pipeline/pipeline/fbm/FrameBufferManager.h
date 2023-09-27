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

#ifndef _MTK_HARDWARE_MTKCAM3_PIPELINE_PIPELINE_FBM_FRAMEBUFFERMANAGER_H_
#define _MTK_HARDWARE_MTKCAM3_PIPELINE_PIPELINE_FBM_FRAMEBUFFERMANAGER_H_
//
#include <mtkcam3/pipeline/pipeline/IFrameBufferManager.h>
//
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
//
#include <utils/RWLock.h> //stl doesn't support shared_mutex (read-write lock) until c++17
//
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/std/Trace.h>
//
#include "BufferProducer.h"


namespace NSCam::v3::pipeline::NSPipelineContext {

/******************************************************************************
 *
 ******************************************************************************/
class FrameBufferManagerImpl
    : public IFrameBufferManager
    , public IPipelineFrameStreamBufferProvider
    , public IPipelineFrameHalResultProcessor
    , public IDebuggee
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definition.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    using ThisT = FrameBufferManagerImpl;

    ////    Configuration
    struct ConfigImageStream
    {
        android::sp<IImageStreamInfo>           pStreamInfo = nullptr;
        Attribute                               attribute;
        android::sp<BufferProducer>             pPool = nullptr;
        IImageStreamBufferProviderT             pProvider = nullptr;
    };

    struct ConfigMetaStream
    {
        android::sp<IMetaStreamInfo>            pStreamInfo = nullptr;
        Attribute                               attribute;
    };

protected:  ////    Data
    struct InFlightBufferSet
    {
        uint32_t                                frameNo = 0;
        uint32_t                                requestNo = 0;
        int64_t                                 sensorTimestamp = 0;
        std::vector<android::sp<IImageStreamBuffer>>
                                                vpHalImageStreamBuffers;
        std::vector<android::sp<IMetaStreamBuffer>>
                                                vpHalMetaStreamBuffers;
        std::vector<android::sp<IMetaStreamBuffer>>
                                                vpAppMetaStreamBuffers;
        //
        std::shared_ptr<TrackFrameResultParams const>
                                                trackParams;
        size_t                                  trackCnt = 0;
        // user-declared ctor
        InFlightBufferSet(
            uint32_t frameNo,
            uint32_t requestNo,
            std::shared_ptr<TrackFrameResultParams const>const& trackParams
        ) : frameNo(frameNo), requestNo(requestNo), trackParams(trackParams)
        {
            vpHalImageStreamBuffers.reserve(trackParams->halImageStreams.size());
            vpHalMetaStreamBuffers.reserve(trackParams->halMetaStreams.size());
            vpAppMetaStreamBuffers.reserve(trackParams->appMetaStreams.size());
            trackCnt = trackParams->halImageStreams.size() +
                       trackParams->halMetaStreams.size() +
                       trackParams->appMetaStreams.size();
        }
        // defalut ctor
        InFlightBufferSet() = default;
        // move ops (copy ops = delete)
        InFlightBufferSet(InFlightBufferSet&&) = default;
        InFlightBufferSet& operator=(InFlightBufferSet&&) = default;
        //
        std::string toString() const;
    };


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                        Constant
    const std::string                   mName;
    std::weak_ptr<ThisT>                mWeakThis;
    const int32_t                       mLogLevel;

    static const std::string            mDebuggeeName;
    std::shared_ptr<IDebuggeeCookie>    mDebuggeeCookie{nullptr};

protected:  ////                        Configuration
    mutable android::RWLock             mConfigLock; // read-write lock

    /**
     * A list of stream sets; the streams in a set share the same buffer pool.
     */
    std::shared_ptr<std::vector<std::set<StreamId_T>>>
                                        mConfigPoolSharedStreams;

    std::map<StreamId_T, std::shared_ptr<ConfigImageStream>>
                                        mConfigImageStreams;

    std::map<StreamId_T, std::shared_ptr<ConfigMetaStream>>
                                        mConfigMetaStreams;

    std::shared_ptr<android::Printer>   mConfigPrinter;
    std::list<std::string>              mConfigLogs;
    std::mutex                          mConfigLogsLock;

protected:  ////                        Data
    std::list<InFlightBufferSet>        mInFlightList;
    mutable std::timed_mutex            mInFlightLock;

    std::list<HistoryBufferSet>         mReadyList;
    std::timed_mutex                    mReadyLock;
    std::map<StreamId_T, int32_t>       mReadyStreamCounters;  // if shared streams, use first streamId as key

    const int32_t                       mAcquireTimeoutMS;
    std::atomic_bool                    mInSelect{false};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation
                    FrameBufferManagerImpl(char const* name);
                    ~FrameBufferManagerImpl();
    auto            initialize(std::shared_ptr<ThisT> sp) -> bool;

public:     ////    Attributes
    auto            getName() const -> char const* { return mName.c_str(); }

    auto            queryConfigImageStream(
                        StreamId_T streamId
                    ) const -> std::shared_ptr<ConfigImageStream>;

protected:  ////    Operation
                    struct AddImageStreams
                    {
                        std::vector<android::sp<IImageStreamInfo>>
                                                            vStreamInfo;
                        Attribute                           attribute;
                        IImageStreamBufferProviderT         pProvider = nullptr;
                    };
    auto            addImageStreams(AddImageStreams const& arg) -> int;

    auto            configureImageStreams(
                        std::vector<AddImageStream> const& streams,
                        std::vector<std::set<StreamId_T>> const* streamsWithSharedPool
                    ) -> int;

protected:  ////    Operation
    template <typename T>
    auto            releaseOneBufferSet(T& bufferSet) -> void;

    auto            tryReleaseOneHistoryBufferSet(StreamId_T streamId, bool bDumpLog) -> void;

    auto            increaseReadyCounters(HistoryBufferSet const& hbs, int32_t const step) -> void;

    auto            getPoolStreams(StreamId_T const streamId) -> std::set<StreamId_T>;

protected:  ////    Debugging
    auto            dumpConfigStreams(
                        android::Printer& printer
                    ) -> void;

    auto            dumpBufferPools(
                        android::Printer& printer
                    ) -> void;

    auto            dumpInFlightListLocked(
                        android::Printer& printer
                    ) -> void;

    auto            dumpReadyListLocked(
                        android::Printer& printer
                    ) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineFrameStreamBufferProvider Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operation
    virtual auto    requestStreamBuffer(
                        requestStreamBuffer_cb cb,
                        RequestStreamBuffer const& arg
                    ) -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineFrameHalResultProcessor Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operation
    virtual auto    returnResult(
                        ReturnResult const& arg
                    ) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IFrameBufferManager Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Attributes
    virtual auto    getPipelineFrameBufferProvider() const
                        -> std::shared_ptr<IPipelineFrameStreamBufferProvider>
                    { return mWeakThis.lock(); }

    virtual auto    getPipelineFrameResultProcessor() const
                        -> std::shared_ptr<IPipelineFrameHalResultProcessor>
                    { return mWeakThis.lock(); }

public:     ////    Configuration
    virtual auto    beginConfigure() -> void;
    virtual auto    endConfigure() -> int;

    virtual auto    addImageStream(AddImageStream const& arg) -> int;
    virtual auto    addMetaStream(AddMetaStream const& arg) -> int;
    virtual auto    configureStreams(ConfigureStreams const& arg) -> int;

public:     ////    (Frame) Operation
    virtual auto    trackFrameResult(TrackFrameResult const& arg) -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IHistoryBufferProvider Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    (Frame) Operation
    virtual auto    enableFrameResultRecyclable(
                        uint32_t frameNo,
                        bool forceToRecycle
                    ) -> void;

public:     ////    Select
    virtual auto    beginSelect() -> std::optional<std::list<HistoryBufferSet>>;

    virtual auto    endSelect() -> void;

    virtual auto    returnUnselectedSet(
                        ReturnUnselectedSet&& arg
                    ) -> void;

    virtual auto    getInFlightInfos() const -> std::vector<ZslPolicy::InFlightInfo>;

    virtual auto    cloneHBS(HistoryBufferSet const& src) -> HistoryBufferSet;

public:     ////    Debugging
    virtual auto    dumpState(
                        android::Printer& printer
                    ) -> void;

    virtual auto    dumpBufferState(
                        android::Printer& printer
                    ) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IDebuggee Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.
    virtual auto    debuggeeName() const -> std::string { return mDebuggeeName; }
    virtual auto    debug(
                        android::Printer& printer,
                        const std::vector<std::string>& options
                    ) -> void;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace NSCam::v3::pipeline::NSPipelineContext
#endif  //_MTK_HARDWARE_MTKCAM3_PIPELINE_PIPELINE_FBM_FRAMEBUFFERMANAGER_H_

