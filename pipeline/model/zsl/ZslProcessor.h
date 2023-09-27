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

#ifndef _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_INCLUDE_IMPL_ZSLPROCESSOR_H_
#define _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_INCLUDE_IMPL_ZSLPROCESSOR_H_

#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <vector>

#include <utils/Timers.h>
#include "impl/IZslProcessor.h"
#include "mtkcam3/pipeline/pipeline/IPipelineContext.h"
#include "mtkcam3/pipeline/pipeline/IFrameBufferManager.h"
#include "mtkcam3/pipeline/utils/streambuf/StreamBuffers.h"
#include "mtkcam3/pipeline/model/types.h"
#include "mtkcam/utils/metadata/IMetadata.h"
#include <mtkcam/utils/debug/debug.h>


/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace std;
using namespace NSCam;
using namespace NSCam::v3::pipeline::NSPipelineContext;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::pipeline::policy;


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace model {


class ZslProcessor
    : public IZslProcessor
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    using HistoryBufferSet = IHistoryBufferProvider::HistoryBufferSet;
    using ISelector = NSCam::ZslPolicy::ISelector;
    using RequiredStreams = NSCam::ZslPolicy::RequiredStreams;
    using SbToMetaT = std::pair<android::sp<NSCam::v3::IMetaStreamBuffer>, IMetadata*>;
    using SensorModeMap = SensorMap<uint32_t>;

public:
    struct  MyDebuggee : public IDebuggee
    {
        static const std::string            mName;
        std::shared_ptr<IDebuggeeCookie>    mCookie = nullptr;
        android::wp<ZslProcessor>           mZslProcessor = nullptr;

                        MyDebuggee(ZslProcessor* p) : mZslProcessor(p) {}
        virtual auto    debuggeeName() const -> std::string { return mName; }
        virtual auto    debug(
                            android::Printer& printer,
                            const std::vector<std::string>& options
                        ) -> void override;
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Instantiation data (initialized at the creation stage).
    const MUINT8                            mTimeSource;
    const std::string                       mUserName;
    const int32_t                           mLogLevel;
    //
    std::shared_ptr<MyDebuggee>             mDebuggee = nullptr;

protected:  ////    Configuration data (initialized at the configuration stage).
    SensorMap<ParsedStreamInfo_P1>          mvParsedStreamInfo_P1;
    android::wp<IPipelineModelCallback>     mPipelineModelCallback;

protected:  ////    Request data.
    struct FrameObject
    {
        std::shared_ptr<RequiredStreams>    pRequiredStreams;  // select stage
        std::shared_ptr<SensorModeMap>      pRequiredSensorModes;  // select stage
        std::shared_ptr<IFrameBuilder>      pFrameBuilder;     // build stage
    };
    struct ZslReqObject
    {
        uint32_t                                requestNo = 0;
        std::chrono::system_clock::time_point   timePoint;  //TODO: isTimeout & ZSD_ts
        ZslPolicyParams                         policyParams;
        int64_t                                 zsdTimestampNs = 0;
        std::shared_ptr<IHistoryBufferProvider> pHistoryBufferProvider;
        mutable std::map<size_t, FrameObject>   vFrameObj;  // key: frameIdx
        // To select/operate inflight frames
        mutable std::map<size_t, FrameObject>   preSelectFrameObjs;
        mutable ISelector::SelectOutputParams   preSelectResult;
        // AutoTunning, NDD dump
        std::vector<std::shared_ptr<IFrameBuilder>> tuningFrames;
        bool isDone() const {
            // Note: currently is not guarantee to wait for lock/unlock/release.
            return vFrameObj.empty() && preSelectFrameObjs.empty();
        }
    };
    //
    std::deque<ZslReqObject>                mvPendingRequest;
    std::map<uint32_t, int64_t>             mvFakeShutter;  // key: reqNo, value: fakeShutter
    int64_t                                 mFakeShutterNs; // timestamp to cb as fakeshutter (for pending req)

    mutable std::timed_mutex                mLock;
    int64_t                                 mZSLTimeDiffMs;

    // Selector
    // key: name of selector; value: hbs it locks.
    std::map<string, std::list<HistoryBufferSet>>
                                            mLockedMap;
    std::shared_ptr<IHistoryBufferProvider> pLockedHBP;  // return hbs when flush
    //
    std::shared_ptr<void>                   mCustomLibHandle;
    std::function<ISelector*()>             mCustomSelectorCreator;
    ISelector*                              mpCustomSelector = nullptr;
    //
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ISelector impl
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    class DefaultSelector : public ISelector
    {
    public:     ////    Interfaces.
        auto getName() const -> char const* override { return "DefaultSelector"; }
        auto select(SelectInputParams const& in) const -> SelectOutputParams override;
    };
    std::shared_ptr<DefaultSelector>        mpDefaultSelector;
    //
    class CShotSelector : public ISelector  // TODO: move
    {
    public:     ////    Interfaces.
        auto getName() const -> char const* override { return "CShotSelector"; }
        auto select(SelectInputParams const& in) const -> SelectOutputParams override;
    };
    std::shared_ptr<CShotSelector>          mpCShotSelector;
    //
    class MultiframeSelector : public ISelector  // TODO: move
    {
    public:     ////    Interfaces.
        auto getName() const -> char const* override { return "MultiframeSelector"; }
        auto select(SelectInputParams const& in) const -> SelectOutputParams override;
    };
    std::shared_ptr<MultiframeSelector>     mpMultiframeSelector;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:

    auto    print(
                android::Printer& printer,
                ZslReqObject const& o
            ) -> void;

    auto    toString(ZslReqObject const& o) -> std::string;

    auto    dumpPendingToString() -> std::string;

    auto    dumpLockedMap(android::Printer& printer) -> void;

    auto    callbackFakeShutter(uint32_t const reqNo) -> void;

    struct BuildFrameParams
    {
        std::shared_ptr<HistoryBufferSet>   pHbs;
        std::shared_ptr<IFrameBuilder>      pFrameBuilder;
        // Additional metadata ISelector ask to append.
        std::map<StreamId_T, std::shared_ptr<IMetadata>>
                                            additionalMetas;
    };
    auto    buildFrame_Locked(
                uint32_t const requestNo,
                std::vector<BuildFrameParams>const& in
            ) -> FrameBuildersOfOneRequest;

    auto    toString(BuildFrameParams const& o) -> std::string;

    auto    processZslRequest(
                ZslReqObject const& zslReqObj
            ) -> FrameBuildersOfOneRequest;

    auto    selectBuf_Locked(
                ZslReqObject const& in
            ) -> std::vector<BuildFrameParams>;

    auto    select(
                ZslReqObject const& reqObj,
                ZslPolicy::ISelector const* pSelector,
                std::list<HistoryBufferSet>const& historyBuffers,
                std::list<HistoryBufferSet>const& lockedBuffers,
                std::vector<ZslPolicy::InFlightInfo>const& inflightFrames
            ) -> ISelector::SelectOutputParams;

    auto    getSelector(
                ZslPolicy::ISelector const* pPolicySelector
            ) -> ZslPolicy::ISelector const*;

    auto    generateZslRequestInfo(
                ZslReqObject const& reqObj,
                std::vector<SbToMetaT>& lockedMetaSbs
            ) -> ISelector::ZslRequestInfo;

    auto    generateZslFrameInfos(
                std::map<size_t, FrameObject>const& vFrameObj,
                std::vector<SbToMetaT>& lockedMetaSbs
            ) -> std::map<size_t, ISelector::ZslFrameInfo>;

    int64_t getFakeShutterNs(uint32_t const requestNo);

    auto    prepareFrames(
                std::vector<std::shared_ptr<pipelinesetting::RequestResultParams>>const& vReqResult,
                std::vector<std::shared_ptr<IFrameBuilder>>const& vFrameBuilder,
                SensorMap<uint32_t>const& vSensorMode,
                std::vector<uint32_t>const& streamingSensorList,
                std::map<size_t, ZslProcessor::FrameObject>& vFrameObj,
                std::vector<std::shared_ptr<IFrameBuilder>>& tuningFrames
            ) -> void;

    auto    prepareSelectedParams(
                std::map<size_t, ISelector::ZslFrameResult>const& vSelected,
                std::map<size_t, FrameObject>& vFrameObj,
                std::map<ISelector::ZslFrameIdxT, ISelector::ZslFrameResult>& preSelected,//
                std::map<size_t, FrameObject>& preSelectFrameObjs,//
                std::list<HistoryBufferSet>& historyBuffers,
                std::list<HistoryBufferSet>& lockedBuffers,
                std::vector<ZslPolicy::InFlightInfo>const& inflights,//
                std::shared_ptr<IHistoryBufferProvider>const& pHBP,
                std::vector<BuildFrameParams>& out
            ) -> void;

    auto    prepareTimedoutParams(
                std::map<size_t, FrameObject>& vFrameObj,
                std::list<HistoryBufferSet>& historyBuffers,
                std::vector<BuildFrameParams>& out
            ) -> void;


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Operations.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   public:  ////    Instantiation.
                    ZslProcessor(int32_t openId, std::string const& name);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IDebuggee Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.

    virtual auto    debug(
                        android::Printer& printer,
                        const std::vector<std::string>& options
                    ) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  RefBase Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Interfaces.

    virtual void    onFirstRef();
    virtual void    onLastStrongRef(const void* id);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IZslProcessor Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.

    virtual auto    configure(ZslConfigParams const& in) -> int override;

    /**
     * The IZslProcessor is in charge of evaluating zsl/non-zsl flow might be applied to this input
     * params from User(application) and Capture Setting Policy module.
     *
     * @param[in] in:
     *  Callers must promise its content. The callee is not allowed to modify it.
     *
     * @param[out] out:
     *  On this call, the callee must allocate and set up its content.
     *
     * @return
     *      OK indicates success; otherwise failure.
     */
    virtual auto    submitZslRequest(
                        ZslRequestParams&& in
                    ) -> std::vector<FrameBuildersOfOneRequest> override;


    virtual auto    processZslPendingRequest(uint32_t const requestNo)
                        -> std::vector<FrameBuildersOfOneRequest> override;


    /**
     * The IZslProcessor has pending zsl capture request. User should submitZslRequest even
     * though there exists no zsl capture request in current reqeust.
     *
     * @param[in] in:
     *
     * @param[out] out:
     *
     * @return
     *      true indicates has pending request;
     */
    virtual auto    hasZslPendingRequest() const -> bool override;


    /**
     * The IZslProcessor is in charge of update configured metadata to HistoryBufferContainer.
     *
     * @param[in] in:
     *  Callers must promise its content. The callee is not allowed to modify it.
     *
     * @param[out] out:
     *  On this call, the callee must allocate and set up its content.
     *
     * @return
     *
     */
    virtual auto    onFrameUpdated(
                        uint32_t const requestNo,
                        IPipelineBufferSetFrameControl::IAppCallback::Result const& result
                    ) -> void override;

    /**
     * The IZslProcessor is in charge of pipeline flush and mark error for StreamBuffer
     *
     * @param[in] in:
     *
     * @param[out] out:
     *
     * @return
     *
     */
    virtual auto    flush() -> std::vector<FrameBuildersOfOneRequest> override;

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace model
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_INCLUDE_IMPL_ZSLPROCESSOR_H_

