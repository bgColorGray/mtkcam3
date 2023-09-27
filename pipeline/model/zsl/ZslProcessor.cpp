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
#define LOG_TAG "MtkCam/ZslProc"

#include "ZslProcessor.h"
#include <dlfcn.h>  // dlopen
#include "MyUtils.h"
#include "mtkcam/drv/iopipe/CamIO/IHalCamIO.h"
#include "mtkcam/utils/metadata/client/mtk_metadata_tag.h"
#include "mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h"
#include "mtkcam/utils/metastore/IMetadataProvider.h"
#include <mtkcam/utils/metadata/mtk_metadata_types.h>
#include "mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h"
#include "mtkcam3/pipeline/hwnode/StreamId.h"
#include <algorithm>
#include <queue>
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_ZSL);

using namespace NSCam::v3::pipeline::model;
using namespace NSCam::Utils::ULog;
using IMetaStreamBuffer = NSCam::v3::IMetaStreamBuffer;
using IImageStreamBuffer = NSCam::v3::IImageStreamBuffer;
using IMetaStreamInfo = NSCam::v3::IMetaStreamInfo;
using HalMetaStreamBufferAllocatorT = NSCam::v3::Utils::HalMetaStreamBuffer::Allocator;
using HistoryBufferSet = IHistoryBufferProvider::HistoryBufferSet;
using ISelector = NSCam::ZslPolicy::ISelector;
using RequiredStreams = NSCam::ZslPolicy::RequiredStreams;
using SensorModeMap = ISelector::SensorModeMap;
using SbToMetaT = std::pair<android::sp<NSCam::v3::IMetaStreamBuffer>, IMetadata*>;

#define SUPPORT_AUTO_TUNNING 0
// TO-DO: move to custom folder
constexpr int64_t ZSLShutterDelayTimeDiffMs = 0;
constexpr uint32_t ZSLForcePolicyDisable = 0xefffffff;
constexpr int64_t ZSLForceTimeoutDisable = -1;

/******************************************************************************
 *
 ******************************************************************************/
static inline bool isP1AppMeta(StreamId_T const& streamId)
{
    return (eSTREAMID_META_APP_DYNAMIC_01 <= streamId)
        && (streamId <= eSTREAMID_META_APP_DYNAMIC_01_MAIN3);
}

inline int64_t ZslProcessor::getFakeShutterNs(uint32_t const requestNo)
{
    return (mvFakeShutter.find(requestNo) != mvFakeShutter.end()) ? mvFakeShutter[requestNo] : 0;
}

static inline int64_t getCurrentTimeNs(const MUINT8 timeSource)
{
    return (timeSource == MTK_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN) ?
            ::systemTime(SYSTEM_TIME_MONOTONIC)  // UNKNOWN  = 0
        :   ::systemTime(SYSTEM_TIME_BOOTTIME);  // REALTIME = 1
}

static MUINT8 getTimeSource(int32_t openId)
{
    MUINT8 src = MTK_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;  // default
    if (auto pMetadataProvider = NSMetadataProviderManager::valueForByDeviceId(openId);
        pMetadataProvider == nullptr) {
        MY_LOGW("openId(%d): can not get IMetadataProvider", openId);
    } else if (IMetadata::getEntry<MUINT8>(&pMetadataProvider->getMtkStaticCharacteristics(),
        MTK_SENSOR_INFO_TIMESTAMP_SOURCE, src) == false) {
        MY_LOGW("can not get MTK_SENSOR_INFO_TIMESTAMP_SOURCE, use REALTIME as default");
    }
    return src;
}

static inline int64_t getDurationMs(chrono::system_clock::time_point const& tp)
{
    return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - tp).count();
}

template <typename T> inline
std::string toString_StreamIds(const T& items, char const* name)
{
    std::ostringstream oss;
    oss << name << "{ " << std::showbase << std::hex;
    for (auto const& streamId : items) {
        oss << streamId << " ";
    }
    oss <<  "}";
    return oss.str();
}

template <typename T>
static auto findByFrameNo(T& vHbs, uint32_t const frameNo) {
    return std::find_if(vHbs.begin(), vHbs.end(),
        [frameNo](auto& hbs) { return hbs.frameNo == frameNo; });
}


template <typename T>
static auto findByStreamId(T& vSb, StreamId_T const& streamId) {
    return std::find_if(vSb.begin(), vSb.end(),
        [streamId](auto& sb) { return sb->getStreamInfo()->getStreamId() == streamId; });
}

static StreamId_T getCtrlByDynamic(StreamId_T const& in) {
    switch (in) {
        // P1HalMetaDynamic => P1HalMetaControl
        case eSTREAMID_META_PIPE_DYNAMIC_01:
            return eSTREAMID_META_PIPE_CONTROL;
        case eSTREAMID_META_PIPE_DYNAMIC_01_MAIN2:
            return eSTREAMID_META_PIPE_CONTROL_MAIN2;
        case eSTREAMID_META_PIPE_DYNAMIC_01_MAIN3:
            return eSTREAMID_META_PIPE_CONTROL_MAIN3;
        // P1AppMetaDynamic => eSTREAMID_META_APP_CONTROL
        case eSTREAMID_META_APP_DYNAMIC_01:
        case eSTREAMID_META_APP_DYNAMIC_01_MAIN2:
        case eSTREAMID_META_APP_DYNAMIC_01_MAIN3:
            return eSTREAMID_META_APP_CONTROL;
        default:
            return -1;
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void releaseBuffer(android::sp<IImageStreamBuffer>& rpBuffer)
{
    //MY_LOGD("release imageSB: (%#" PRIx64 ":%s)", rpBuffer->getStreamInfo()->getStreamId(), rpBuffer->getStreamInfo()->getStreamName());
    rpBuffer->releaseBuffer();  // return to the pool
    rpBuffer = nullptr;
}

void releaseBuffer(android::sp<IMetaStreamBuffer>& rpBuffer)
{
    //MY_LOGD("metaSB release automatically: (%#" PRIx64 ":%s)", rpBuffer->getStreamInfo()->getStreamId(), rpBuffer->getStreamInfo()->getStreamName());
    rpBuffer = nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
/**
 * !!! DO NOT MODIFIED THESE HELPER FUNCTIONS !!!
 * Please clone and rename if modifications are needed.
 */
template <typename T>
static bool isMatched(
    NSCam::IMetadata const* meta,
    uint32_t const tag,
    T const& targetVal,
    char const* caller,
    bool backwardCompatible = false)
{
    T val;
    if (NSCam::IMetadata::getEntry<T>(meta, tag, val)) {
        MY_LOGD("[%s] tag(%#x) val=%s target=%s bc=%d", caller, tag,
                std::to_string(val).c_str(), std::to_string(targetVal).c_str(), backwardCompatible);
        return (val == targetVal);
    }
    MY_LOGE("[%s] tag(%#x) not found!", caller, tag);
    return backwardCompatible;
}

/******************************************************************************
 *
 ******************************************************************************/
template <typename T>
static bool isMatched(
    NSCam::IMetadata const* meta,
    uint32_t const tag,
    T const* targetArr,
    size_t arrSize,
    char const* caller,
    bool backwardCompatible = false)
{
    std::string targetStr;
    for (size_t i = 0; i < arrSize; i++) {
        targetStr += std::to_string(targetArr[i]);
        if (i != arrSize - 1) targetStr += ",";
    }
    //
    T val;
    if (NSCam::IMetadata::getEntry<T>(meta, tag, val)) {
        MY_LOGD("[%s] tag(%#x) val=%s target=%s bc=%d", caller, tag,
                std::to_string(val).c_str(), targetStr.c_str(), backwardCompatible);
        return std::find(targetArr, targetArr + arrSize, val) != (targetArr + arrSize);
    }
    MY_LOGE("[%s] tag(%#x) not found!", caller, tag);
    return backwardCompatible;
}

/******************************************************************************
 *
 ******************************************************************************/
static bool isAFStable(NSCam::IMetadata const* appMeta)
{
    // get entries: afMode, afState, lensState
    uint8_t afMode = 0, afState = 0, lensState = 0;
    if (!NSCam::IMetadata::getEntry<uint8_t>(appMeta, MTK_CONTROL_AF_MODE, afMode)) {
        MY_LOGE("tag(%#x) not found!", MTK_CONTROL_AF_MODE);
        return false;
    }
    if (!NSCam::IMetadata::getEntry<uint8_t>(appMeta, MTK_CONTROL_AF_STATE, afState)) {
        MY_LOGE("tag(%#x) not found!", MTK_CONTROL_AF_STATE);
        return false;
    }
    if (!NSCam::IMetadata::getEntry<uint8_t>(appMeta, MTK_LENS_STATE, lensState)) {
        MY_LOGE("tag(%#x) not found!", MTK_LENS_STATE);
        return false;
    }
    std::string str;
    str.append("afMode=").append(std::to_string(afMode))
        .append(" afState=").append(std::to_string(afState))
        .append(" lensState=").append(std::to_string(lensState));
    // check conditions
    if (afMode == MTK_CONTROL_AF_MODE_AUTO || afMode == MTK_CONTROL_AF_MODE_MACRO) {
        if (afState == MTK_CONTROL_AF_STATE_INACTIVE ||
            afState == MTK_CONTROL_AF_STATE_PASSIVE_SCAN ||
            afState == MTK_CONTROL_AF_STATE_ACTIVE_SCAN || lensState == MTK_LENS_STATE_MOVING) {
            if (afState == MTK_CONTROL_AF_STATE_INACTIVE &&
                lensState == MTK_LENS_STATE_STATIONARY) {
                // modification1: no statinary count in hal3 version.
                // this information would be passed by af/lens module if needed.
                MY_LOGD("AF ok: %s", str.c_str());
                return true;
            }
            MY_LOGW("AF fail: %s", str.c_str());
            return false;
        }
    } else if (afMode == MTK_CONTROL_AF_MODE_CONTINUOUS_VIDEO ||
               afMode == MTK_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
        if (afState == MTK_CONTROL_AF_STATE_INACTIVE ||
            afState == MTK_CONTROL_AF_STATE_PASSIVE_SCAN ||
            afState == MTK_CONTROL_AF_STATE_ACTIVE_SCAN || lensState == MTK_LENS_STATE_MOVING) {
            // modification2: no inactive count in hal3 version.
            // we do not need to count inactive count  default afstate is NOT_FOCUS
            if (afState == MTK_CONTROL_AF_STATE_INACTIVE &&
                lensState == MTK_LENS_STATE_STATIONARY) {
                // modification1: no statinary count in hal3 version.
                // this information would be passed by af/lens module if needed.
                MY_LOGD("AF ok: %s", str.c_str());
                return true;
            }
            MY_LOGW("AF fail: %s", str.c_str());
            return false;
        }
    }
    MY_LOGD("AF ok: %s", str.c_str());
    return true;
}

static bool isAEStable(NSCam::IMetadata const* appMeta) {
    constexpr uint8_t arr[] = {MTK_CONTROL_AE_STATE_CONVERGED, MTK_CONTROL_AE_STATE_LOCKED};
    return isMatched<uint8_t>(appMeta, MTK_CONTROL_AE_STATE, arr, sizeof(arr) / sizeof(arr[0]), "isAEStable");
}

static bool isAESoftStable(NSCam::IMetadata const* halMeta) {
    return isMatched<int32_t>(halMeta, MTK_3A_AE_ZSL_STABLE, 1, "isAESoftStable", true);
}

static bool isFameSync(NSCam::IMetadata const* halMeta) {
    return isMatched<int64_t>(halMeta, MTK_FRAMESYNC_RESULT, MTK_FRAMESYNC_RESULT_PASS, "isFameSync", true);
}

/******************************************************************************
 *
 ******************************************************************************/
static bool isAFStable(
    ISelector::HistoryFrame const& hf,
    RequiredStreams const& rss)
{
    return std::all_of(hf.appMetas.cbegin(), hf.appMetas.cend(), [&rs=rss.appMetas](auto const& pr) {
        if (rs.find(pr.first->getStreamId()) == rs.end()) return true;  // not required
        bool ret = isAFStable(pr.second);
        MY_LOGW_IF(!ret, "%s: X", pr.first->getStreamName());
        return ret;
    });
}

__unused static bool isAEStable(
    ISelector::HistoryFrame const& hf,
    RequiredStreams const& rss)
{
    return std::all_of(hf.appMetas.cbegin(), hf.appMetas.cend(), [&rs=rss.appMetas](auto const& pr) {
        if (rs.find(pr.first->getStreamId()) == rs.end()) return true;  // not required
        bool ret = isAEStable(pr.second);
        MY_LOGW_IF(!ret, "%s: X", pr.first->getStreamName());
        return ret;
    });
}

static bool isAESoftStable(
    ISelector::HistoryFrame const& hf,
    RequiredStreams const& rss)
{
    return std::all_of(hf.halMetas.cbegin(), hf.halMetas.cend(), [&rs=rss.halMetas](auto const& pr) {
        if (rs.find(pr.first->getStreamId()) == rs.end()) return true;  // not required
        bool ret = isAESoftStable(pr.second);
        MY_LOGW_IF(!ret, "%s: X", pr.first->getStreamName());
        return ret;
    });
}

static bool isFameSync(
    ISelector::HistoryFrame const& hf,
    RequiredStreams const& rss)
{
    return std::any_of(hf.halMetas.cbegin(), hf.halMetas.cend(), [&rs=rss.halMetas](auto const& pr) {
        if (rs.find(pr.first->getStreamId()) == rs.end()) return true;  // not required
        bool ret = isFameSync(pr.second);
        MY_LOGW_IF(!ret, "%s: X", pr.first->getStreamName());
        return ret;
    });
}


/******************************************************************************
 *
 ******************************************************************************/
static bool isError(std::vector<android::sp<IImageStreamBuffer>>const& vHalImageSb) {
    return std::any_of(vHalImageSb.cbegin(), vHalImageSb.cend(), [](auto const& sb){
        bool ret = sb->getStatus() & NSCam::v3::STREAM_BUFFER_STATUS::ERROR;
        MY_LOGW_IF(ret, "%s: X", sb->getStreamInfo()->getStreamName());
        return ret;
    });
}

static bool isPDPureRaw(std::vector<android::sp<IMetaStreamBuffer>>const& vHalMetaSb) {
    return std::any_of(vHalMetaSb.cbegin(), vHalMetaSb.cend(), [](auto const& sb) {
        IMetadata* meta = sb->tryReadLock(LOG_TAG);
        bool ret = isMatched<MINT32>(meta, MTK_P1NODE_RAW_TYPE,
            NSIoPipe::NSCamIOPipe::EPipe_PURE_RAW, "isPDPureRaw");
        sb->unlock(LOG_TAG, meta);
        MY_LOGW_IF(ret, "%s: X", sb->getStreamInfo()->getStreamName());
        return ret;
    });
}

static auto
getHistoryBuffers(
    std::shared_ptr<IHistoryBufferProvider>const& pHBP,
    bool needPDProcessedRaw = false
) -> std::list<HistoryBufferSet>
{
    auto canNotUse = [&] (auto& hbs) -> bool {
        if (isError(hbs.vpHalImageStreamBuffers) ||
            (needPDProcessedRaw ? isPDPureRaw(hbs.vpHalMetaStreamBuffers) : false)) {
            MY_LOGI("hbs[F%u] is problematic, return and release", hbs.frameNo);
            pHBP->returnUnselectedSet(
                IHistoryBufferProvider::ReturnUnselectedSet{
                    .hbs = std::move(hbs),
                    .keep = false
                });
            return true;
        }
        return false;
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::list<HistoryBufferSet> out;
    auto opt = pHBP->beginSelect();
    if (opt) {
        out = *std::move(opt);
        out.remove_if(canNotUse);
    }
    return out;
}

static auto
returnHistoryBuffers(
    std::shared_ptr<IHistoryBufferProvider>const& pHBP,
    std::list<HistoryBufferSet>& historyBuffers,
    bool keep = true
) -> void
{
    for (auto&& hbs : historyBuffers) {
        pHBP->returnUnselectedSet(
            IHistoryBufferProvider::ReturnUnselectedSet{
                .hbs = std::move(hbs),
                .keep = keep
            });
    }
    pHBP->endSelect();
    historyBuffers.clear();
}


/******************************************************************************
 *
 ******************************************************************************/
const std::string ZslProcessor::MyDebuggee::mName{"NSCam::v3::pipeline::model::IZslProcessor"};
auto ZslProcessor::MyDebuggee::debug(android::Printer& printer, const std::vector<std::string>& options) -> void
{
    auto p = mZslProcessor.promote();
    if ( CC_LIKELY(p != nullptr) ) {
        p->debug(printer, options);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
ZslProcessor::
ZslProcessor(int32_t openId, std::string const& name)
    : mTimeSource(getTimeSource(openId))
    , mUserName(name)
    , mLogLevel(::property_get_int32("vendor.debug.camera.log.pipelinemodel.zsl", 0))
    , mFakeShutterNs(getCurrentTimeNs(mTimeSource))
{
    int64_t default_zsl_ts_diff = ::property_get_int64("ro.vendor.camera3.zsl.default", 0);
    int64_t project_zsl_ts_diff = ::property_get_int64("ro.vendor.camera3.zsl.project", 0);
    int64_t user_zsl_ts_diff = ::property_get_int64("vendor.debug.camera3.zsl.timestamp", ZSLShutterDelayTimeDiffMs);
    mZSLTimeDiffMs = default_zsl_ts_diff + project_zsl_ts_diff + user_zsl_ts_diff;
    MY_LOGI("zsdOffsetMs(%" PRId64 ") = default(%" PRId64 ")+project(%" PRId64 ")+user(%" PRId64 ")",
            mZSLTimeDiffMs, default_zsl_ts_diff, project_zsl_ts_diff, user_zsl_ts_diff);
    MY_LOGI("mTimeSource(%d) mFakeShutterNs(%" PRId64 ")", (int)mTimeSource, mFakeShutterNs);
}


/******************************************************************************
 *
 ******************************************************************************/
android::sp<IZslProcessor>
IZslProcessor::
createInstance(
    int32_t openId,
    string const& name
)
{
    sp<ZslProcessor> pZslProcessor = new ZslProcessor(openId, name);
    if (CC_UNLIKELY(!pZslProcessor.get())) {
        MY_LOGE("create zsl processor instance fail");
        return nullptr;
    }

    return pZslProcessor;
}


/******************************************************************************
 *
 ******************************************************************************/
void
ZslProcessor::
onFirstRef()
{
    mDebuggee = std::make_shared<MyDebuggee>(this);
    if (CC_LIKELY(mDebuggee != nullptr)) {
        if (auto pDbgMgr = IDebuggeeManager::get()) {
            mDebuggee->mCookie = pDbgMgr->attach(mDebuggee, 0);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
void
ZslProcessor::
onLastStrongRef(const void* id __unused)
{
    if (CC_LIKELY(mDebuggee != nullptr)) {
        if (auto pDbgMgr = IDebuggeeManager::get()) {
            pDbgMgr->detach(mDebuggee->mCookie);
        }
        mDebuggee = nullptr;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
configure(ZslConfigParams const& in) -> int
{
    CAM_TRACE_CALL();

    std::scoped_lock _l(mLock);
    // check if there is pending request
    if (mvPendingRequest.size()) {
        MY_LOGE("%zu ZSL pending requests not finish yet, 1st-reqNo: %u", mvPendingRequest.size(),
                mvPendingRequest[0].requestNo);
        return INVALID_OPERATION;
    }

    // config params
    mvParsedStreamInfo_P1 = *(in.pParsedStreamInfo_P1);
    mPipelineModelCallback = in.pCallback;

    // get custom selector from .so
    mCustomLibHandle =
        std::shared_ptr<void>(dlopen("libmtkcam_zsl_customselector.so", RTLD_LAZY), dlclose);
    if (mCustomLibHandle) {
        mCustomSelectorCreator =
            reinterpret_cast<ISelector* (*)()>(dlsym(mCustomLibHandle.get(), "create"));
        if (!mCustomSelectorCreator) {
            MY_LOGW("CustomSelector - dlsym fail: %s", dlerror());
            mpCustomSelector = mCustomSelectorCreator();
        }
    } else {
        dlerror();
    }

    // create default selector!
    mpDefaultSelector = std::make_shared<DefaultSelector>();
    mpCShotSelector = std::make_shared<CShotSelector>();  // FIXME:
    mpMultiframeSelector = std::make_shared<MultiframeSelector>();  // FIXME:

    MY_LOGI("mCustomLibHandle(%p) mpCustomSelector(%p)",
             mCustomLibHandle.get(), mpCustomSelector);
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
debug(
    android::Printer& printer __unused,
    const std::vector<std::string>& options __unused
) -> void
{
    std::unique_lock _l(mLock, std::defer_lock);
    if (_l.try_lock_for(std::chrono::milliseconds(100))) {
        printer.printLine(" *Pending Request*");
        android::PrefixPrinter prefixPrinter(printer, "    ");
        for (auto const& pendingReqObj : mvPendingRequest) {
            printer.printFormatLine("  requestNo:%u", pendingReqObj.requestNo);
            print(prefixPrinter, pendingReqObj);
        }
    } else {
        printer.printLine("Timeout waiting for mLock");
    }
}

auto
ZslProcessor::
print(
    android::Printer& printer,
    ZslReqObject const& o
) -> void
{
    printer.printFormatLine("%s", toString(o).c_str());
}

auto
ZslProcessor::
toString(ZslReqObject const& o) -> std::string
{
    auto toString = [](std::map<size_t, FrameObject>const& v){
        std::ostringstream oss;
        for (auto const& pr : v) {
            auto const& frameObj = pr.second;
            oss << "\n" << std::to_string(pr.first) << "-{"
                << " required=" << ZslPolicy::toString(*frameObj.pRequiredStreams)
                << " pFB=" << frameObj.pFrameBuilder << "("
                << NSPipelineContext::toString(frameObj.pFrameBuilder->getGroupFrameType()) << ")"
                << " }";
        }
        return oss.str();
    };
    //
    std::ostringstream oss;
    oss << "{"
        << " requestNo=" << o.requestNo
        << " duration=" << getDurationMs(o.timePoint)
        //<< " policyParams=" << policy::toString(o.policyParams)
        << " zsdTs=" << o.zsdTimestampNs
        << " pHBP=" << std::hex << o.pHistoryBufferProvider
        << " vFrameObj=" << toString(o.vFrameObj)
        << " }";
    return oss.str();
};

auto
ZslProcessor::
toString(BuildFrameParams const& o) -> std::string
{
    std::ostringstream oss;
    oss << "{"
        << " hbs=" << (o.pHbs ? NSPipelineContext::toString(*o.pHbs) : "X")
        << " pFB=" << o.pFrameBuilder << "("
        << NSPipelineContext::toString(o.pFrameBuilder->getGroupFrameType()) << ")"
        << " addiMeta(#" << o.additionalMetas.size() << ")"
        << " }";
    return oss.str();
}

auto
ZslProcessor::
dumpPendingToString() -> std::string
{
    std::ostringstream oss;
    for (auto const& pendingReqObj : mvPendingRequest) {
        oss << "R" << pendingReqObj.requestNo << " ";
    }
    return oss.str();
}

auto
ZslProcessor::
dumpLockedMap(
    android::Printer& printer
) -> void
{
    for (auto it = mLockedMap.begin(); it != mLockedMap.end();) {
        if (it->second.empty()) {  // remove empty
            it = mLockedMap.erase(it);
        } else {
            printer.printFormatLine("dump locked [%s]", it->first.c_str());
            for (auto const& hbs : it->second) {
                printer.printFormatLine("   %s", NSPipelineContext::toString(hbs).c_str());
            }
            ++it;
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
#if SUPPORT_AUTO_TUNNING
static bool hasAutoTuningHint(std::vector<android::sp<IMetaStreamBuffer>>const& vHalMetaSb) {
    return std::any_of(vHalMetaSb.cbegin(), vHalMetaSb.cend(), [](auto const& sb) {
        IMetadata* meta = sb->tryReadLock(LOG_TAG);
        MINT32 data;
        bool ret = IMetadata::getEntry<MINT32>(meta, MTK_HAL_REQUEST_ADDITIONAL_TEST_FRAME_COUNT, data);
        sb->unlock(LOG_TAG, meta);
        MY_LOGD_IF(ret, "%s: [AutoTuning] TEST_FRAME_COUNT=(%d)", sb->getStreamInfo()->getStreamName(), data);
        return ret;
    });
}
#endif

auto
ZslProcessor::
prepareFrames(
    std::vector<std::shared_ptr<pipelinesetting::RequestResultParams>>const& vReqResult,
    std::vector<std::shared_ptr<IFrameBuilder>>const& vFrameBuilder,
    SensorMap<uint32_t>const& vSensorMode,
    std::vector<uint32_t>const& streamingSensorList,
    std::map<size_t, ZslProcessor::FrameObject>& vFrameObj,
    std::vector<std::shared_ptr<IFrameBuilder>>& tuningFrames
) -> void
{
    auto isDummy = [](auto const& p) {
        return p && (p->getGroupFrameType() == GroupFrameType::PREDUMMY ||
                     p->getGroupFrameType() == GroupFrameType::POSTDUMMY);
    };
    //
    auto isAutoTuningFrame = [](auto const& p) {
#if SUPPORT_AUTO_TUNNING
        std::vector<android::sp<IMetaStreamBuffer>> vHalMetaSb;
        p->getHalMetaStreamBuffers(vHalMetaSb);
        return hasAutoTuningHint(vHalMetaSb);
#endif
      return false;
    };
    //
    auto generateRequiredStreams = [](auto const& pReqResult) {
        return std::make_shared<RequiredStreams>(
            RequiredStreams{
                .halImages = pReqResult->trackFrameResultParams->halImageStreams,
                .halMetas = pReqResult->trackFrameResultParams->halMetaStreams,
                .appMetas = pReqResult->trackFrameResultParams->appMetaStreams
            });
    };
    auto generateRequiredSensorModes = [&vSensorMode, &streamingSensorList]() {
        // store sensor modes of streaming sensors
        SensorModeMap out;
        for (const auto& [sensorId, sensorMode] : vSensorMode) {
            if (std::find(streamingSensorList.begin(), streamingSensorList.end(), sensorId)
                    != streamingSensorList.end()) {
                out.emplace(sensorId, sensorMode);
            }
        }
        return std::make_shared<SensorModeMap>(out);
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    MY_LOGF_IF(vReqResult.size() != vFrameBuilder.size(),
            "mismatch - #vReqResult:%zu #vFrameBuilder:%zu",
            vReqResult.size(), vFrameBuilder.size());
    size_t idx = 0;
    auto pRequiredSensorModes = generateRequiredSensorModes();
    for (size_t i = 0; i < vFrameBuilder.size(); i++) {
        if (isAutoTuningFrame(vFrameBuilder[i])) {
            tuningFrames.emplace_back(vFrameBuilder[i]);
        } else if (isDummy(vFrameBuilder[i])) {
            // FIXME: feature should not ask for dummy if don't needed.
            MY_LOGE("skip (%zu,%s)", i,
                    NSPipelineContext::toString(vFrameBuilder[i]->getGroupFrameType()).c_str());
        } else {
            vFrameObj.emplace(idx++,
                FrameObject{
                    .pRequiredStreams = generateRequiredStreams(vReqResult[i]),
                    .pRequiredSensorModes = pRequiredSensorModes,
                    .pFrameBuilder = vFrameBuilder[i]
                });
        }
    }
}

static void appendTuningFrames(
    std::vector<std::shared_ptr<IFrameBuilder>> const& tuningFrames,
    std::vector<std::shared_ptr<IFrameBuilder>>& vFrameBuilder) {
    vFrameBuilder.insert(vFrameBuilder.end(), tuningFrames.begin(), tuningFrames.end());
}

auto
ZslProcessor::
submitZslRequest(
    ZslRequestParams&& in
) -> std::vector<FrameBuildersOfOneRequest>
{
    CAM_TRACE_CALL();
    FUNC_START_PUBLIC;
    auto const reqNo = in.requestNo;
    // ===== Prepare zsl capture request =====
    ZslReqObject reqObj = {
        .requestNo = in.requestNo,
        .timePoint = chrono::system_clock::now(),
        .policyParams = in.zslPolicyParams,
        .zsdTimestampNs = getCurrentTimeNs(mTimeSource) - mZSLTimeDiffMs * 1000000,
        .pHistoryBufferProvider = in.pHistoryBufferProvider
    };
    prepareFrames(in.vReqResult, in.vFrameBuilder, *(in.pvSensorMode), *(in.pStreamingSensorList),
                  reqObj.vFrameObj, reqObj.tuningFrames);

    // ===== Process previous pending request =====
    auto vOut = processZslPendingRequest(reqNo);

    // ===== Process zsl capture request =====
    std::scoped_lock _l(mLock);

    auto out = processZslRequest(reqObj);
    // [success] nothing left
    if (reqObj.isDone()) {
        MY_LOGI("[requestNo:%u]: success. (selected:#%zu, tuning:#%zu)", reqNo, out.size(), reqObj.tuningFrames.size());
        appendTuningFrames(reqObj.tuningFrames, out);
        vOut.emplace_back(std::move(out));
        return vOut;
    }

    // [fail] cb fakeShutter & add to pendingList
    callbackFakeShutter(reqNo);  // not to block following requests
    mvPendingRequest.emplace_back(std::move(reqObj));

    MY_LOGW("[requestNo:%u]: fail. (selected:#%zu, preselect:#%zu, remaining:#%zu) => add to pending [fakeShutter:%" PRId64 "; pendingReq(#%zu):%s]",
            reqNo, out.size(), mvPendingRequest.back().preSelectFrameObjs.size(), mvPendingRequest.back().vFrameObj.size(),
            mvFakeShutter[reqNo], mvPendingRequest.size(), dumpPendingToString().c_str());

    ULogPrinter debugPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_WARNING);
    in.pHistoryBufferProvider->dumpBufferState(debugPrinter);

    if (out.size()) {  // partial
        vOut.emplace_back(std::move(out));
    }
    FUNC_END_PUBLIC;
    return vOut;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
processZslPendingRequest(
    uint32_t const reqNo
) -> std::vector<FrameBuildersOfOneRequest>
{
    CAM_TRACE_CALL();
    std::vector<FrameBuildersOfOneRequest> vOut;
    std::scoped_lock _l(mLock);
    // ===== Process pending queue =====
    for (auto it = mvPendingRequest.begin(); it != mvPendingRequest.end(); ) {
        auto& reqObj = *it;
        auto out = processZslRequest(reqObj);
        // [success]
        if (reqObj.isDone()) {
            it = mvPendingRequest.erase(it);
            MY_LOGI("[requestNo:%u]: success. (reqNo:%u, selected:#%zu, tuning:#%zu) => remove from pending (#%zu)",
                    reqNo, reqObj.requestNo, out.size(), reqObj.tuningFrames.size(), mvPendingRequest.size());
            appendTuningFrames(reqObj.tuningFrames, out);
            vOut.emplace_back(std::move(out));
            continue;
        }
        // [fail]
        MY_LOGW("[requestNo:%u]: fail. (reqNo:%u, selected:#%zu, preselect:#%zu, remaining:#%zu) => keep pending (#%zu)",
                reqNo, reqObj.requestNo, out.size(), reqObj.preSelectFrameObjs.size(), reqObj.vFrameObj.size(), mvPendingRequest.size());
        if (out.size()) {  // partial
            vOut.emplace_back(std::move(out));
        }
        ++it;
    }
    return vOut;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
hasZslPendingRequest() const -> bool
{
    std::scoped_lock _l(mLock);
    return mvPendingRequest.empty() ? false : true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
onFrameUpdated(
    uint32_t const requestNo,
    IPipelineBufferSetFrameControl::IAppCallback::Result const& result
) -> void
{
    CAM_TRACE_CALL();
    std::scoped_lock _l(mLock);
    if (auto fakeIt = mvFakeShutter.find(requestNo);
        fakeIt != mvFakeShutter.end())
    {
        // remove MTK_SENSOR_TIMESTAMP to avoid cb shutter twice
        for (auto const& pMetaStream : result.vAppOutMeta)
        {
            if (isP1AppMeta(pMetaStream->getStreamInfo()->getStreamId()))
            {
                IMetadata* pP1OutAppMeta = pMetaStream->tryWriteLock(LOG_TAG);
                if (CC_UNLIKELY(pP1OutAppMeta == nullptr)) {
                    MY_LOGW("cannot lock metadata");
                } else {
                    if (pP1OutAppMeta->remove(MTK_SENSOR_TIMESTAMP) == OK) {
                        MY_LOGD("(%#" PRIx64 ":%s) remove MTK_SENSOR_TIMESTAMP tag(%#x) [requestNo:%u, fakeShutter:%" PRId64 "]",
                                pMetaStream->getStreamInfo()->getStreamId(),
                                pMetaStream->getStreamInfo()->getStreamName(),
                                MTK_SENSOR_TIMESTAMP, fakeIt->first, fakeIt->second);
                    } else {
                        MY_LOGW("(%#" PRIx64 ":%s) can not find MTK_SENSOR_TIMESTAMP tag(%#x)",
                                pMetaStream->getStreamInfo()->getStreamId(),
                                pMetaStream->getStreamInfo()->getStreamName(),
                                MTK_SENSOR_TIMESTAMP);
                    }
                }
                pMetaStream->unlock(LOG_TAG, pP1OutAppMeta);
            }
        }

        // get last meta (only main frame called onFrameUpdated)
        if (result.bFrameEnd) {
            mvFakeShutter.erase(fakeIt);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
flush() -> std::vector<FrameBuildersOfOneRequest>
{
    CAM_TRACE_CALL();
    FUNC_START_PUBLIC;
    std::scoped_lock _l(mLock);
    std::vector<FrameBuildersOfOneRequest> vOut;

    for (auto& reqObj : mvPendingRequest)
    {
        FrameBuildersOfOneRequest out;
        reqObj.vFrameObj.merge(reqObj.preSelectFrameObjs);
        for (auto& pr : reqObj.vFrameObj)
        {
            auto& pFrameBuilder = pr.second.pFrameBuilder;
            if (CC_UNLIKELY(pFrameBuilder == nullptr)) {
                MY_LOGE("bad pFrameBuilder - requestNo:%u", reqObj.requestNo);
                return {};
            }

            auto fakeIt = mvFakeShutter.find(reqObj.requestNo);
            int64_t reprocessTS = (fakeIt == mvFakeShutter.end()) ? 0 : fakeIt->second;
            pFrameBuilder->setReprocessSensorTimestamp(reprocessTS);
            pFrameBuilder->setReprocessFrame(false);

            out.push_back(pFrameBuilder);
        }
        appendTuningFrames(reqObj.tuningFrames, out);
        vOut.emplace_back(out);
        MY_LOGI("[requestNo:%u]: change pending to normal capture (frames:#%zu + tuning:#%zu)",
                reqObj.requestNo, out.size(), reqObj.tuningFrames.size());
    }
    mvPendingRequest.clear();

    // return lockedBuffers
    for (auto& pr : mLockedMap) {
        MY_LOGI("return and release lockedBuffers of [%s](#%zu)", pr.first.c_str(), pr.second.size());
        returnHistoryBuffers(pLockedHBP, pr.second, false);
    }

    FUNC_END_PUBLIC;
    return vOut;
}

/******************************************************************************
 *      Implementation
 ******************************************************************************/
auto
ZslProcessor::
processZslRequest(
    ZslReqObject const& zslReqObj
) -> FrameBuildersOfOneRequest
{
    auto vBuildFrameObj = selectBuf_Locked(zslReqObj);
    return buildFrame_Locked(zslReqObj.requestNo, vBuildFrameObj);
}


auto
ZslProcessor::
callbackFakeShutter(uint32_t const reqNo) -> void
{
    CAM_TRACE_CALL();
    auto const pCallback = mPipelineModelCallback.promote();
    if (CC_UNLIKELY(!pCallback.get())) {
        MY_LOGE("can not promote pCallback for shutter");
        return;
    }

    // generate sp<IMetaStreamBuffer> with only MTK_SENSOR_TIMESTAMP (w/ the value of mFakeShutterNs)

    // take any IMetaStreamInfo to allocate
    auto streamInfo_P1 = mvParsedStreamInfo_P1.begin()->second;
    sp<IMetaStreamBuffer> pShutterMetaBuffer = HalMetaStreamBufferAllocatorT(streamInfo_P1.pHalMeta_DynamicP1.get())();

    IMetadata* meta = pShutterMetaBuffer->tryWriteLock(LOG_TAG);
    IMetadata::setEntry<MINT64>(meta, MTK_SENSOR_TIMESTAMP, (mFakeShutterNs += 1000));  // add 1ms to ensure unique fakeShutter
    pShutterMetaBuffer->unlock(LOG_TAG, meta);
    pShutterMetaBuffer->finishUserSetup();

    // cb fakeShutter
    pCallback->onFrameUpdated(
        UserOnFrameUpdated{
            .requestNo = reqNo,
            .nOutMetaLeft = 1,
            .vOutMeta = { pShutterMetaBuffer }
        });
    MY_LOGD("cb fakeShutter (reqNo:%u, timestamp:%" PRId64 ")", reqNo, mFakeShutterNs);

    // record fakeShutter  (setReprocessSensorTimestamp @buildFrame_Locked ; remove from vAppOutMeta @onFrameUpdated)
    mvFakeShutter.emplace(reqNo, mFakeShutterNs);
}


/******************************************************************************
 *
 ******************************************************************************/
static auto
prepareHalImage(
    std::vector<sp<IImageStreamBuffer>>const& vHistorySb,  // [in]
    std::shared_ptr<IFrameBuilder>const& pFrameBuilder     // [in]
) -> void
{
    CAM_TRACE_CALL();
    for (auto const& sb : vHistorySb)
    {
        if (sb == nullptr)  continue;
        auto const pStreamInfo = sb->getStreamInfo();
        if (pStreamInfo == nullptr)  continue;
        auto const streamId = pStreamInfo->getStreamId();
        auto const streamName = pStreamInfo->getStreamName();

        // reset history sb status and user manager
        sb->clearStatus();
        sp<NSCam::v3::IUsersManager> pUsersManager = new UsersManager(streamId, streamName);
        sb->setUsersManager(pUsersManager);
        MY_LOGD("reset (%#" PRIx64 ":%s)->UsersManager(%p)", streamId, streamName, pUsersManager.get());

        // set hal image of this req.
        pFrameBuilder->setHalImageStreamBuffer(streamId, sb);
    }
}

static auto
prepareHalMeta(
    std::vector<sp<IMetaStreamBuffer>>const& vHistorySb,  // [in]
    std::shared_ptr<IFrameBuilder>const& pFrameBuilder    // [in]
) -> void
{
    CAM_TRACE_CALL();
    // 0. get hal meta of this req.
    std::vector<android::sp<IMetaStreamBuffer>> vHalMetaSb;
    pFrameBuilder->getHalMetaStreamBuffers(vHalMetaSb);
    for (auto const& sb : vHistorySb)
    {
        ASSERT_IF_NULLPTR(sb, "history sb=nullptr (HalMeta)");
        auto pStreamInfo = const_cast<IMetaStreamInfo*>(sb->getStreamInfo());
        ASSERT_IF_NULLPTR(pStreamInfo, "history streamInfo=nullptr (HalMeta)");
        auto const streamId = pStreamInfo->getStreamId();
        auto const streamName = pStreamInfo->getStreamName();

        // 1. create new meta with history data
        IMetadata* pHistoryMeta = sb->tryReadLock(LOG_TAG);
        ASSERT_IF_NULLPTR(pHistoryMeta, "history metadata=nullptr (HalMeta)(%#" PRIx64 ":%s)", streamId, streamName);
        IMetadata newMeta(*pHistoryMeta);
        sb->unlock(LOG_TAG, pHistoryMeta);

        // 2. find control meta
        auto pCtrlSb = findByStreamId(vHalMetaSb, getCtrlByDynamic(streamId));
        if (pCtrlSb != vHalMetaSb.end()) {
            // 3. append corresponding control meta to new meta
            IMetadata* pCtrlMeta = (*pCtrlSb)->tryReadLock(LOG_TAG);
            ASSERT_IF_NULLPTR(pCtrlMeta, "ctrl metadata=nullptr (HalMeta)");
            newMeta += *pCtrlMeta;
            (*pCtrlSb)->unlock(LOG_TAG, pCtrlMeta);
            MY_LOGD("prepare (%#" PRIx64 ":%s): copy history, update all (%#" PRIx64 ":%s)",
                    streamId, streamName, (*pCtrlSb)->getStreamInfo()->getStreamId(), (*pCtrlSb)->getStreamInfo()->getStreamName());
        }

        // 4. set hal meta of this req.
        sp<IMetaStreamBuffer> newSb = HalMetaStreamBufferAllocatorT(pStreamInfo)(newMeta);
        ASSERT_IF_NULLPTR(newSb, "new sb=nullptr (HalMeta)(%#" PRIx64 ":%s)", streamId, streamName);
        pFrameBuilder->setHalMetaStreamBuffer(streamId, newSb);
    }
}

static auto
prepareAppMeta(
    std::vector<sp<IMetaStreamBuffer>>const& vHistorySb,  // [in]
    std::shared_ptr<IFrameBuilder>const& pFrameBuilder    // [in]
) -> void
{
    // TODO: white listing P1/3A/ISP/Flash related AppMetas(*) that are related to P1Out images.
    // right now we are assuming: whiteLists = AppMetas - blackLists - vendorTags(**)
    // (*)  AppMetas = P1Out App Results = pAppMeta_DynamicP1
    // (**) vendorTags = vendorTags - P1/3A/ISP/Flash related vendorTags - vendorTagWhiteLists
    static const unordered_set<int> blackLists {  // tags that should not use history
    // ========== necessary post processing tag ==========
        MTK_CONTROL_CAPTURE_INTENT,
        MTK_NOISE_REDUCTION_MODE,
        MTK_COLOR_CORRECTION_ABERRATION_MODE,
        MTK_COLOR_CORRECTION_MODE,
        MTK_TONEMAP_MODE,
        MTK_SHADING_MODE,
        MTK_HOT_PIXEL_MODE,
        MTK_EDGE_MODE,
        MTK_JPEG_ORIENTATION
    // ========== customize tag ==========
    };
    static const unordered_set<int> vendorTagWhiteLists {  // isVendorTag, but still need to use history (P1/3A/ISP/Flash)
    // ========== customize tag ==========
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Lambda Functions
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto isNeedToUseHistory = [](int tag) -> bool {
        if (blackLists.find(tag) != blackLists.end()) {
            ///MY_LOGD("tag(%#x) is in blackLists, do not use history", tag);
            return false;
        }
        static const bool useHistoryVendorTag = ::property_get_bool("persist.vendor.camera3.zsl.vendortag.usehistory",0);
        if ((!useHistoryVendorTag) && (tag & MTK_VENDOR_TAG_SECTION_START)) {  /* isVendorTag */
            // vendorTags should not use history, except for (P1/3A/ISP/Flash)
            bool isVendorTagWhiteLists =  /* use history */
                (vendorTagWhiteLists.find(tag) != vendorTagWhiteLists.end()) ||
                ((tag & 0xFFFF0000) ==  MTK_3A_FEATURE_START) ||
                ((tag & 0xFFFF0000) == MTK_FLASH_FEATURE_START);
            ///MY_LOGD("tag(%#x) is vendorTag, use history(%d) for P1 related tags", tag, isVendorTagWhiteLists);
            return isVendorTagWhiteLists ? true : false;
        }
        return true;
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    CAM_TRACE_CALL();
    // 1. get app meta of this req, and find app control meta
    std::vector<android::sp<IMetaStreamBuffer>> vAppMetaSb;
    pFrameBuilder->getAppMetaStreamBuffers(vAppMetaSb);
    // right now multicam only have one AppCtrl, but there might be multiple in the future
    auto pCtrlSb = findByStreamId(vAppMetaSb, eSTREAMID_META_APP_CONTROL);
    for (auto const& sb : vHistorySb)
    {
        ASSERT_IF_NULLPTR(sb, "history sb=nullptr (AppMeta)");
        auto pStreamInfo = const_cast<IMetaStreamInfo*>(sb->getStreamInfo());
        ASSERT_IF_NULLPTR(pStreamInfo, "history streamInfo=nullptr (AppMeta)");
        auto const streamId = pStreamInfo->getStreamId();
        auto const streamName = pStreamInfo->getStreamName();

        // create meta with AppCtrl data, update/overwrite whiteLists tags w/ history value
        if (pCtrlSb != vAppMetaSb.end())
        {
            // 2. create new sb w/ AppCtrl value
            IMetadata* pCtrlMeta = (*pCtrlSb)->tryReadLock(LOG_TAG);
            ASSERT_IF_NULLPTR(pCtrlMeta, "ctrl metadata=nullptr (AppMeta)");
            IMetadata newMeta(*pCtrlMeta);
            (*pCtrlSb)->unlock(LOG_TAG, pCtrlMeta);

            // 3. prepare AppMetas: update white listed entries (w/ history value)
            {
                IMetadata* pHistoryMeta = sb->tryReadLock(LOG_TAG);
                ASSERT_IF_NULLPTR(pHistoryMeta, "history metadata=nullptr (AppMeta)(%#" PRIx64 ":%s)", streamId, streamName);
                for (size_t i = 0; i < pHistoryMeta->count(); ++i) {
                    auto const inEntry = pHistoryMeta->entryAt(i);
                    if (isNeedToUseHistory(inEntry.tag())) {
                        newMeta.update(inEntry);
                    }
                }
                sb->unlock(LOG_TAG, pHistoryMeta);
            }

            // 4. set app meta of this req.
            sp<IMetaStreamBuffer> newSb = HalMetaStreamBufferAllocatorT(pStreamInfo)(newMeta);
            ASSERT_IF_NULLPTR(newSb, "new sb=nullptr (AppMeta)(%#" PRIx64 ":%s)", streamId, streamName);
            pFrameBuilder->setAppMetaStreamBuffer(streamId, newSb);
            MY_LOGD("prepare (%#" PRIx64 ":%s): copy (%#" PRIx64 ":%s), update P1 related tags",
                    streamId, streamName, (*pCtrlSb)->getStreamInfo()->getStreamId(), (*pCtrlSb)->getStreamInfo()->getStreamName());
        }
    }
}

static auto
appendAdditionalMeta(
    std::map<StreamId_T, std::shared_ptr<IMetadata>>const& additionalMetas,  // [in]
    std::shared_ptr<IFrameBuilder>const& pFrameBuilder    // [in]
) -> void
{
    auto append = [](auto const& sb, IMetadata const& addiMeta){
        IMetadata* meta = sb->tryWriteLock(LOG_TAG);
        if (meta == nullptr) {
            MY_LOGE("cannot lock metadata");
            return;
        }
        *meta += addiMeta;
        sb->unlock(LOG_TAG, meta);
    };
    //
    std::vector<android::sp<IMetaStreamBuffer>> vAppMetaSb;
    pFrameBuilder->getAppMetaStreamBuffers(vAppMetaSb);
    std::vector<android::sp<IMetaStreamBuffer>> vHalMetaSb;
    pFrameBuilder->getHalMetaStreamBuffers(vHalMetaSb);
    //
    for (auto const& pr : additionalMetas) {
        auto& streamId = pr.first;
        MY_LOGD("(%#" PRIx64 ") append additionalMetas", streamId);
        // isAppMeta
        auto appSbIt = findByStreamId(vAppMetaSb, streamId);
        if (appSbIt != vAppMetaSb.end()) {
            append(*appSbIt, *pr.second);
            continue;
        }
        // isHalMeta
        auto halSbIt = findByStreamId(vHalMetaSb, streamId);
        if (halSbIt != vHalMetaSb.end()) {
            append(*halSbIt, *pr.second);
            continue;
        }
        //
        MY_LOGE("cannot find streamId(%#" PRIx64 ") to append additionalMetas", streamId);
    }
}

auto
ZslProcessor::
buildFrame_Locked(
    uint32_t const requestNo,
    std::vector<BuildFrameParams>const& in
) -> FrameBuildersOfOneRequest
{
    CAM_TRACE_CALL();
    FrameBuildersOfOneRequest out;

    for (auto const& o : in) {
        auto& pHbs = o.pHbs;
        auto& pFrameBuilder = o.pFrameBuilder;
        int64_t reprocessTS = getFakeShutterNs(requestNo);
        // update frameBuilder with history buffers.
        if (pHbs) {
            prepareHalImage(pHbs->vpHalImageStreamBuffers, pFrameBuilder);
            prepareHalMeta(pHbs->vpHalMetaStreamBuffers, pFrameBuilder);
            prepareAppMeta(pHbs->vpAppMetaStreamBuffers, pFrameBuilder);
            // haven't callback fakeShutter before, set ts.
            if (reprocessTS == 0) {
                reprocessTS = pHbs->sensorTimestamp;
            }
            // FIXME: update fakeShutter, move to somewhere else
            mFakeShutterNs = pHbs->sensorTimestamp;
        }
        // append zsl additional metadatas (ISelector/custom)
        if (o.additionalMetas.size()) {
            appendAdditionalMeta(o.additionalMetas, pFrameBuilder);
        }
        //
        pFrameBuilder->setReprocessSensorTimestamp(reprocessTS);
        pFrameBuilder->setReprocessFrame(pHbs ? true : false);
        out.emplace_back(pFrameBuilder);
    }

    return out;
}


/******************************************************************************
 *
 ******************************************************************************/
static auto getMetaFromSb(  // don't forget to unlock!
    android::sp<IMetaStreamBuffer>const& metaSb,  // in
    std::vector<SbToMetaT>& lockedMetaSbs  // in/out
) -> NSCam::IMetadata*
{
    NSCam::IMetadata* meta = metaSb->tryReadLock(LOG_TAG);
    lockedMetaSbs.emplace_back(metaSb, meta);
    return meta;
}

static void unlockMetaSbs(std::vector<SbToMetaT>const& lockedMetaSbs) {
    for (auto const& pr : lockedMetaSbs) {
        pr.first->unlock(LOG_TAG, pr.second);
    }
}

static auto getMetaInfoMap(
    std::vector<android::sp<IMetaStreamBuffer>>const& vSb,
    std::vector<SbToMetaT>& lockedMetaSbs
) -> std::map<IMetaStreamInfo const*, NSCam::IMetadata const*>
{
    std::map<IMetaStreamInfo const*, NSCam::IMetadata const*> out;
    for (auto const& sb : vSb) {
        out.emplace(sb->getStreamInfo(), getMetaFromSb(sb, lockedMetaSbs));
    }
    return out;
}

static auto getImageInfoSet(
    std::vector<android::sp<IImageStreamBuffer>>const& vSb
) -> std::set<NSCam::v3::IImageStreamInfo const*>
{
    std::set<NSCam::v3::IImageStreamInfo const*> out;
    for (auto const& sb : vSb) {
        out.emplace(sb->getStreamInfo());
    }
    return out;
}

static auto
generateHistoryFrames(
    std::list<HistoryBufferSet>const& historyBuffers,
    std::list<HistoryBufferSet>const& lockedBuffers,
    std::vector<SbToMetaT>& lockedMetaSbs
) -> std::map<ISelector::HistoryFrameNoT, ISelector::HistoryFrame>
{
    std::map<ISelector::HistoryFrameNoT, ISelector::HistoryFrame> out;
    auto addHistoryFrame = [&](HistoryBufferSet const& hbs, bool locked)
    {
        out.emplace(hbs.frameNo,
            ISelector::HistoryFrame{
                .frameNo = hbs.frameNo,
                .requestNo = hbs.requestNo,
                .sensorTimestampNs = hbs.sensorTimestamp,
                .halImages = getImageInfoSet(hbs.vpHalImageStreamBuffers),
                .halMetas = getMetaInfoMap(hbs.vpHalMetaStreamBuffers, lockedMetaSbs),
                .appMetas = getMetaInfoMap(hbs.vpAppMetaStreamBuffers, lockedMetaSbs),
                .locked = locked,
            });
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for (auto const& hbs : historyBuffers) {
        addHistoryFrame(hbs, false);
    }
    for (auto const& hbs : lockedBuffers) {
        addHistoryFrame(hbs, true);
    }
    // TODO: dump
    return out;
}


auto
ZslProcessor::
getSelector(
    ZslPolicy::ISelector const* pPolicySelector
) -> ZslPolicy::ISelector const*
{
    // Priority: Plugin > Custom > Default
    static int32_t customSelectorState =
        ::property_get_int32("vendor.debug.camera.zsl.custom.select", 0);
    // 0: Plugin > Custom > Default (default)
    // 1: force customSelect (if exist)
    // -1: disable

    return (pPolicySelector && !(mpCustomSelector != nullptr && customSelectorState == 1))
        /* 1. Plugin Selector */
        ? pPolicySelector : ((mpCustomSelector && customSelectorState != -1)
        /* 2. Custom Selector */
        ? mpCustomSelector
        /* 3. Default Selector */
        : mpDefaultSelector.get());
}


auto
ZslProcessor::
generateZslFrameInfos(
    std::map<size_t, FrameObject>const& vFrameObj,
    std::vector<SbToMetaT>& lockedMetaSbs
) -> std::map<ISelector::ZslFrameIdxT, ISelector::ZslFrameInfo>
{
    std::map<ISelector::ZslFrameIdxT, ISelector::ZslFrameInfo> out;
    for (auto const& pr : vFrameObj) {
        // get ctrl metas
        std::vector<android::sp<IMetaStreamBuffer>> vCtrlSb;
        pr.second.pFrameBuilder->getHalMetaStreamBuffers(vCtrlSb);
        pr.second.pFrameBuilder->getAppMetaStreamBuffers(vCtrlSb);
        //
        out.emplace(pr.first, ISelector::ZslFrameInfo{
            .pRequired = pr.second.pRequiredStreams,
            .ctrlMetas = getMetaInfoMap(vCtrlSb, lockedMetaSbs),
            .pRequiredSensorModes = pr.second.pRequiredSensorModes
        });
    }
    return out;
}

auto
ZslProcessor::
generateZslRequestInfo(
    ZslReqObject const& reqObj,
    std::vector<SbToMetaT>& lockedMetaSbs
) -> ISelector::ZslRequestInfo
{
    return ISelector::ZslRequestInfo{
        .requestNo = reqObj.requestNo,
        .frames = generateZslFrameInfos(reqObj.vFrameObj, lockedMetaSbs),
        .durationMs = getDurationMs(reqObj.timePoint),
        .zsdTimestampNs = reqObj.zsdTimestampNs,
        .suggestFrameSync = reqObj.policyParams.needFrameSync,
    };
}

auto
ZslProcessor::
select(
    ZslReqObject const& reqObj,
    ZslPolicy::ISelector const* pSelector,
    std::list<HistoryBufferSet>const& historyBuffers,
    std::list<HistoryBufferSet>const& lockedBuffers,
    std::vector<NSCam::ZslPolicy::InFlightInfo>const& inflightInfos
) -> ISelector::SelectOutputParams
{
    std::vector<SbToMetaT> lockedMetaSbs;
    auto out = pSelector->select(
        ISelector::SelectInputParams{
            .zslRequest = generateZslRequestInfo(reqObj, lockedMetaSbs),
            .historyFrames = generateHistoryFrames(historyBuffers, lockedBuffers, lockedMetaSbs),
            .inflightFrames = inflightInfos
        });
    unlockMetaSbs(lockedMetaSbs);
    return out;
}


/******************************************************************************
 *
 ******************************************************************************/
static bool containsRequiredStreamBuffer(
    HistoryBufferSet const& hbs,
    RequiredStreams const& rss)
{
    auto containsRequiredStreamBuffer = [](auto& vSb, auto const& rs){
        return std::all_of(rs.begin(), rs.end(), [&vSb](auto rId){
            return findByStreamId(vSb, rId) != vSb.end();
        });
    };
    // TODO: log
    return containsRequiredStreamBuffer(hbs.vpHalImageStreamBuffers, rss.halImages) &&
           containsRequiredStreamBuffer(hbs.vpHalMetaStreamBuffers, rss.halMetas) &&
           containsRequiredStreamBuffer(hbs.vpAppMetaStreamBuffers, rss.appMetas);
}

static bool containRequiredSensorMode(
    ISelector::HistoryFrame const& hf,
    SensorModeMap const& requiredSensorModes)
{
    auto toString = [](const SensorModeMap& modes) -> std::string {
        std::string os = "";
        for (const auto& [id, mode] : modes) {
            os += "[" + std::to_string(id) + "] = " + std::to_string(mode) + ",";
        }
        return os;
    };

    // parse sensor modes from history frames
    SensorModeMap histSensorModes;
    {
        for (const auto& iter : hf.halMetas) {
            const auto& pHalMeta = iter.second;
            int32_t sensorId = -1, sensorMode = -1;
            if (!NSCam::IMetadata::getEntry<int32_t>(pHalMeta, MTK_HAL_REQUEST_SENSOR_ID, sensorId)) {
                MY_LOGE("tag(%#x) MTK_HAL_REQUEST_SENSOR_ID not found!", MTK_HAL_REQUEST_SENSOR_ID);
                return false;
            }
            if (!NSCam::IMetadata::getEntry<int32_t>(pHalMeta, MTK_P1NODE_SENSOR_MODE, sensorMode)) {
                MY_LOGE("tag(%#x) MTK_P1NODE_SENSOR_MODE not found!", MTK_P1NODE_SENSOR_MODE);
                return false;
            }
            histSensorModes.emplace(sensorId, sensorMode);
        }
    }

    // check if contain all required sensor modes
    for (const auto& [rSensorId, rSensorMode] : requiredSensorModes) {
        auto iter = histSensorModes.find(rSensorId);
        if (iter == histSensorModes.end()) {
            MY_LOGW("cannot find required sensorId(%d) : history = {%s}, required = {%s}",
                rSensorId, toString(histSensorModes).c_str(), toString(requiredSensorModes).c_str());
            return false;
        }
        if (iter->second != rSensorMode) {
            MY_LOGW("cannot find required sensorMode(%d) : history = {%s}, required = {%s}",
                rSensorMode, toString(histSensorModes).c_str(), toString(requiredSensorModes).c_str());
            return false;
        }
    }

    MY_LOGD("requiredSensorMode ok : history = {%s}, required = {%s}",
        toString(histSensorModes).c_str(), toString(requiredSensorModes).c_str());
    return true;
}

static void releaseUnrequiredStreamBuffer(
    HistoryBufferSet& hbs,
    RequiredStreams const& rss)
{
    auto releaseUnrequiredStreamBuffer = [](auto& vSb, auto const& rs) {
        vSb.erase(std::remove_if(vSb.begin(), vSb.end(), [&rs](auto& sb) {
            if (rs.count(sb->getStreamInfo()->getStreamId())) return false;
            releaseBuffer(sb); return true; }), vSb.end());
    };
    // TODO: log
    releaseUnrequiredStreamBuffer(hbs.vpHalImageStreamBuffers, rss.halImages);
    releaseUnrequiredStreamBuffer(hbs.vpHalMetaStreamBuffers, rss.halMetas);
    releaseUnrequiredStreamBuffer(hbs.vpAppMetaStreamBuffers, rss.appMetas);
}

static auto
getSelectedHbs(
    uint32_t const frameNo,
    std::list<HistoryBufferSet>& historyBuffers,
    std::list<HistoryBufferSet>& lockedBuffers,
    RequiredStreams const& rss
) -> std::shared_ptr<HistoryBufferSet>
{
    auto getHbsFrom = [&rss, frameNo](auto& vHbs)
        -> std::shared_ptr<HistoryBufferSet>
    {
        auto it = findByFrameNo(vHbs, frameNo);
        if (it != vHbs.end()) {
            if (containsRequiredStreamBuffer(*it, rss)) {
                releaseUnrequiredStreamBuffer(*it, rss);
                auto pHbs = std::make_shared<HistoryBufferSet>(std::move(*it));
                vHbs.erase(it);
                return pHbs;
            }
            MY_LOGE("hbs[F%u] does not contain all required streams", frameNo);
        }
        return nullptr;
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto pHbs = getHbsFrom(historyBuffers);
    MY_LOGD_IF(pHbs, "get hbs[F%u] from history", frameNo);
    if (!pHbs) {
        pHbs = getHbsFrom(lockedBuffers);
        MY_LOGD_IF(pHbs, "get hbs[F%u] from locked", frameNo);
    }
    MY_LOGE_IF(!pHbs, "cannot get hbs[F%u]", frameNo);
    return pHbs;
}

static auto
getClonedHbs(
    uint32_t const frameNo,
    std::list<HistoryBufferSet>& historyBuffers,
    std::list<HistoryBufferSet>& lockedBuffers,
    RequiredStreams const& rss,
    std::shared_ptr<IHistoryBufferProvider>const& pHBP
) -> std::shared_ptr<HistoryBufferSet>
{
    auto getHbsFrom = [&](auto& vHbs)
        -> std::shared_ptr<HistoryBufferSet>
    {
        auto it = findByFrameNo(vHbs, frameNo);
        if (it != vHbs.end()) {
            auto pHbs = std::make_shared<HistoryBufferSet>(pHBP->cloneHBS(*it));
            if (containsRequiredStreamBuffer(*pHbs, rss)) {
                releaseUnrequiredStreamBuffer(*pHbs, rss);
                return pHbs;
            }
            MY_LOGE("hbs[F%u] does not contain all required streams", frameNo);
        }
        return nullptr;
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto pHbs = getHbsFrom(historyBuffers);
    MY_LOGD_IF(pHbs, "clone hbs[F%u] from history", frameNo);
    if (!pHbs) {
        pHbs = getHbsFrom(lockedBuffers);
        MY_LOGD_IF(pHbs, "clone hbs[F%u] from locked", frameNo);
    }
    MY_LOGE_IF(!pHbs, "cannot get hbs[F%u]", frameNo);
    return pHbs;
}

static auto
getTimedoutHbs(
    std::list<HistoryBufferSet>& historyBuffers,
    RequiredStreams const& rss
) -> std::shared_ptr<HistoryBufferSet>
{
    std::shared_ptr<HistoryBufferSet> pHbs;
    auto it = std::find_if(historyBuffers.begin(), historyBuffers.end(),
            [&rss](auto const& hbs) { return containsRequiredStreamBuffer(hbs, rss); });
    if (it != historyBuffers.end()) {
        releaseUnrequiredStreamBuffer(*it, rss);
        pHbs = std::make_shared<HistoryBufferSet>(std::move(*it));
        historyBuffers.erase(it);
    }
    MY_LOGD("TIMEDOUT: get hbs[F%d]", (pHbs ? (int)pHbs->frameNo : -1));
    return pHbs;
}

static auto
isInflight(
    uint32_t const frameNo,
    std::vector<ZslPolicy::InFlightInfo>const& inflights
) -> bool
{
    return std::find_if(inflights.begin(), inflights.end(),
          [frameNo](auto& inflight) {
              return inflight.frameNo == frameNo;
          }) != inflights.end();
};

auto
ZslProcessor::
prepareSelectedParams(
    std::map<ISelector::ZslFrameIdxT, ISelector::ZslFrameResult>const& vSelected,
    std::map<size_t, FrameObject>& vFrameObj,
    std::map<ISelector::ZslFrameIdxT, ISelector::ZslFrameResult>& preSelected,//
    std::map<size_t, FrameObject>& preSelectFrameObjs,//
    std::list<HistoryBufferSet>& historyBuffers,
    std::list<HistoryBufferSet>& lockedBuffers,
    std::vector<ZslPolicy::InFlightInfo>const& inflights,//
    std::shared_ptr<IHistoryBufferProvider>const& pHBP,
    std::vector<BuildFrameParams>& out
) -> void
{
    auto getHbs = [&](auto const& zsf, auto const& rss) -> std::shared_ptr<HistoryBufferSet> {
        if (zsf.frameNo == -1) return nullptr;
        if (zsf.clone) return getClonedHbs(zsf.frameNo, historyBuffers, lockedBuffers, rss, pHBP);
        return getSelectedHbs(zsf.frameNo, historyBuffers, lockedBuffers, rss);
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // handle selected:
    // 1. check frameIndex
    // 2. check if inflight => preselect
    // 3. get hbs from historyBuffers or lockedBuffers
    // 4. check containsRequiredStreamBuffer; then releaseUnrequiredStreamBuffer
    auto selIt = vSelected.begin();
    auto objIt = vFrameObj.begin();
    while (selIt != vSelected.end() && objIt != vFrameObj.end()) {
        if (objIt->first == selIt->first) {
            auto& frameObj = objIt->second;
            auto& zslFrameResult = selIt->second;
            if (isInflight(zslFrameResult.frameNo, inflights)) {
                preSelectFrameObjs.emplace(*objIt);
                preSelected.emplace(*selIt);
            } else {
                out.emplace_back(
                  BuildFrameParams{
                      .pHbs = getHbs(zslFrameResult, *frameObj.pRequiredStreams),
                      .pFrameBuilder = frameObj.pFrameBuilder,
                      .additionalMetas = zslFrameResult.additionalMetas}
                );
            }
            objIt = vFrameObj.erase(objIt);
            ++selIt;
        } else if (objIt->first < selIt->first) {
            ++objIt;
        } else {
            MY_LOGF("wrong frameIdx! objIdx=%zu, selIdx=%zu", objIt->first, selIt->first);
        }
    }
    MY_LOGE_IF(selIt != vSelected.end(), "select redundant#%d", (int)std::distance(selIt, vSelected.end()));
}

auto
ZslProcessor::
prepareTimedoutParams(
    std::map<size_t, FrameObject>& vFrameObj,
    std::list<HistoryBufferSet>& historyBuffers,
    std::vector<BuildFrameParams>& out
) -> void
{
    for (auto const& pr : vFrameObj) {
        auto const& frameObj = pr.second;
        out.emplace_back(BuildFrameParams{
            .pHbs = getTimedoutHbs(historyBuffers, *frameObj.pRequiredStreams),
            .pFrameBuilder = frameObj.pFrameBuilder,
        });
    }
    vFrameObj.clear();
    MY_LOGW("ZSL TIMED_OUT!");  // TODO:
}

static auto
lockBuffersForSelector(
    std::map<ISelector::HistoryFrameNoT, ISelector::SelectOutputParams::OpT>const& ops,
    std::map<ISelector::HistoryFrameNoT, ISelector::SelectOutputParams::OpT>& preOps,//
    std::list<HistoryBufferSet>& historyBuffers,
    std::list<HistoryBufferSet>& lockedBuffers,
    std::vector<ZslPolicy::InFlightInfo>const& inflights//
) -> void
{
    for (auto& pr : ops) {
        if (pr.second != ISelector::SelectOutputParams::OpT::lock) continue;
        auto const frameNo = pr.first;
        if (isInflight(frameNo, inflights)) {
            preOps[pr.first] = pr.second;  // overwrite
            MY_LOGI("inflight hbs[F%u] to lock", frameNo);
        } else {
            auto it = findByFrameNo(historyBuffers, frameNo);
            if (it != historyBuffers.end()) {
                lockedBuffers.splice(lockedBuffers.end(), historyBuffers, it);
            } else {
                MY_LOGE("cannot find hbs[F%u] to lock", frameNo);
            }
        }
    }
}

static auto
unlockBuffersForSelector(
    std::map<ISelector::HistoryFrameNoT, ISelector::SelectOutputParams::OpT>const& ops,
    std::map<ISelector::HistoryFrameNoT, ISelector::SelectOutputParams::OpT>& preOps,//
    std::list<HistoryBufferSet>& historyBuffers,
    std::list<HistoryBufferSet>& lockedBuffers,
    std::vector<ZslPolicy::InFlightInfo>const& inflights//
) -> void
{
    for (auto& pr : ops) {
        if (pr.second != ISelector::SelectOutputParams::OpT::unlock) continue;
        auto const frameNo = pr.first;
        if (isInflight(frameNo, inflights)) {
            preOps[pr.first] = pr.second;  // overwrite
            MY_LOGI("inflight hbs[F%u] to unlock", frameNo);
        } else {
            auto it = findByFrameNo(lockedBuffers, frameNo);
            if (it != lockedBuffers.end()) {
                historyBuffers.splice(historyBuffers.begin(), lockedBuffers, it);
            } else {
                MY_LOGE("cannot find hbs[F%u] to unlock", frameNo);
            }
        }
    }
}

static auto
releaseBuffersForSelector(
    std::map<ISelector::HistoryFrameNoT, ISelector::SelectOutputParams::OpT>const& ops,
    std::map<ISelector::HistoryFrameNoT, ISelector::SelectOutputParams::OpT>& preOps,//
    std::list<HistoryBufferSet>& historyBuffers,
    std::list<HistoryBufferSet>& lockedBuffers,
    std::vector<ZslPolicy::InFlightInfo>const& inflights,//
    std::shared_ptr<IHistoryBufferProvider>const& pHBP
) -> void
{
    for (auto& pr : ops) {
        if (pr.second != ISelector::SelectOutputParams::OpT::release) continue;
        auto const frameNo = pr.first;
        if (isInflight(frameNo, inflights)) {
            preOps[pr.first] = pr.second;  // overwrite
            MY_LOGI("inflight hbs[F%u] to release", frameNo);
        } else {
            auto it = findByFrameNo(historyBuffers, frameNo);
            if (it != historyBuffers.end()) {
                pHBP->returnUnselectedSet(
                    IHistoryBufferProvider::ReturnUnselectedSet{
                        .hbs = std::move(*it),
                        .keep = false
                    });
                historyBuffers.erase(it);
            } else {
                auto jt = findByFrameNo(lockedBuffers, frameNo);
                if (jt != lockedBuffers.end()) {
                    pHBP->returnUnselectedSet(
                        IHistoryBufferProvider::ReturnUnselectedSet{
                            .hbs = std::move(*jt),
                            .keep = false
                        });
                    lockedBuffers.erase(jt);
                } else {
                    MY_LOGE("cannot find hbs[F%u] to release", frameNo);
                }
            }
        }
    }
}


static auto
merge(
    ISelector::SelectOutputParams& base,
    ISelector::SelectOutputParams& src
) -> void
{
    base.selected.merge(src.selected);
    base.operations.merge(src.operations);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::
selectBuf_Locked(
    ZslReqObject const& reqObj
) -> std::vector<BuildFrameParams>
{
    CAM_TRACE_CALL();
    auto const& pHBP = reqObj.pHistoryBufferProvider;
    auto const& policyParams = reqObj.policyParams;
    // 0. print infos of this zsl request.
    MY_LOGI("IN:%s", toString(reqObj).c_str());

    // 1. get history buffers
    auto historyBuffers = getHistoryBuffers(pHBP, policyParams.needPDProcessedRaw);

    // 2. select
    // 2-1. prepare
    int64_t timeoutMs = ZslPolicy::kTimeoutMs;
    auto adaptSelector = [this, &timeoutMs](auto const& params)
        -> ZslPolicy::ISelector const* {  // TODO:
        if (params.pSelector) return params.pSelector.get();
        if (params.pZslPluginParams) {
            uint32_t criteria = params.pZslPluginParams->mPolicy;
            if (criteria == ZslPolicy::None) return mpCShotSelector.get();
            if (criteria & ZslPolicy::ContinuousFrame) {
                timeoutMs = params.pZslPluginParams->mTimeouts;
                return mpMultiframeSelector.get();
            }
        }
        return nullptr;
    };
    //
    auto pSelector = getSelector(adaptSelector(policyParams));
    auto const selectorName = (pSelector->getName()) ? std::string(pSelector->getName()) : "";
    auto& lockedBuffers = mLockedMap[selectorName];
    if (!pLockedHBP) pLockedHBP = pHBP;  // FIXME:
    auto inflightInfos = pHBP->getInFlightInfos();

    // 2-2. dump select input
    MY_LOGI("selector(%s) historyBuffers(#%zu) lockedBuffers(#%zu)",
            selectorName.c_str(), historyBuffers.size(), lockedBuffers.size());

    // 2-3. select
    ISelector::SelectOutputParams result;
    if (reqObj.vFrameObj.size()) {
        result = select(reqObj, pSelector, historyBuffers, lockedBuffers, inflightInfos);
        MY_LOGI("selectOut: %s", ZslPolicy::toString(result).c_str());
    } else {
        MY_LOGI("selectOut: no need to trigger selector to select, pre-select only");
    }

    MY_LOGD("pre-selectOut: %s", ZslPolicy::toString(reqObj.preSelectResult).c_str());
    // merge cur & pre
    merge(result, reqObj.preSelectResult);
    reqObj.vFrameObj.merge(reqObj.preSelectFrameObjs);
    if (reqObj.preSelectFrameObjs.size()) {  // this should not happend
        MY_LOGE("duplicate selection!");
        reqObj.preSelectFrameObjs.clear();
    }

    // 3. handle result
    // 3-1. selected
    std::vector<BuildFrameParams> out;
    prepareSelectedParams(result.selected, reqObj.vFrameObj,
                          reqObj.preSelectResult.selected, reqObj.preSelectFrameObjs,
                          historyBuffers, lockedBuffers, inflightInfos, pHBP, out);

    // 3-2. operations (lock / unlock / release)
    lockBuffersForSelector(result.operations, reqObj.preSelectResult.operations,
                           historyBuffers, lockedBuffers, inflightInfos);
    unlockBuffersForSelector(result.operations, reqObj.preSelectResult.operations,
                             historyBuffers, lockedBuffers, inflightInfos);
    releaseBuffersForSelector(result.operations, reqObj.preSelectResult.operations,
                              historyBuffers, lockedBuffers, inflightInfos, pHBP);

    // 4. handle timeout / return hbs / return expiredLocked.
    // 4-1. handle timeout
    if (!reqObj.isDone() && getDurationMs(reqObj.timePoint) > timeoutMs) {
        ULogPrinter wPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_WARNING);
        pHBP->dumpBufferState(wPrinter);
        reqObj.vFrameObj.merge(reqObj.preSelectFrameObjs);
        prepareTimedoutParams(reqObj.vFrameObj, historyBuffers, out);
    }

    // 4-2. return hbs
    returnHistoryBuffers(pHBP, historyBuffers);
    pHBP->endSelect();

    // dump fbm
    static bool dumpFbm = ::property_get_bool("vendor.debug.camera.zsl.dumpfbm", 0);
    if (!reqObj.isDone() || dumpFbm) {
        ULogPrinter iPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_INFO);
        pHBP->dumpBufferState(iPrinter);
    }

    // dump selection
    {
        MY_LOGI("OUT(#%zu):", out.size());
        for (auto const& o : out) {
            MY_LOGI("%s", toString(o).c_str());
        }
        MY_LOGI("Inflight(Sel#%zu, Op#%zu): %s",
                reqObj.preSelectResult.selected.size(),
                reqObj.preSelectResult.operations.size(),
                ZslPolicy::toString(reqObj.preSelectResult).c_str());
    }

    // dump lockedMap
    {
        ULogPrinter iPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_INFO);
        dumpLockedMap(iPrinter);
    }

    return out;
}


/******************************************************************************
 *
 ******************************************************************************/
static bool
containsRequiredStreamBuffer(
    std::set<v3::IImageStreamInfo const*> imgSet,
    std::set<StreamId_T> const& rs)
{
    return std::all_of(rs.begin(), rs.end(), [&imgSet](auto rId){
        return (std::find_if(imgSet.begin(), imgSet.end(), [rId](const auto& pStreamInfo){
            return rId == pStreamInfo->getStreamId(); }) != imgSet.end());
        });
}

static bool
containsRequiredStreamBuffer(
    std::map<v3::IMetaStreamInfo const*, IMetadata const*> metaMap,
    std::set<StreamId_T> const& rs)
{
    return std::all_of(rs.begin(), rs.end(), [&metaMap](auto rId){
        return (std::find_if(metaMap.begin(), metaMap.end(), [rId](const auto& pr){
            return rId == pr.first->getStreamId(); }) != metaMap.end());
        });
}

static bool
containsRequiredStreamBuffer(
    ISelector::HistoryFrame const& hf,
    RequiredStreams const& rss)
{
    // TODO: log
    return containsRequiredStreamBuffer(hf.halImages, rss.halImages) &&
           containsRequiredStreamBuffer(hf.halMetas, rss.halMetas) &&
           containsRequiredStreamBuffer(hf.appMetas, rss.appMetas);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ZslProcessor::DefaultSelector::
select(ISelector::SelectInputParams const& in) const -> ISelector::SelectOutputParams
{
    auto isOk = [zslReq = in.zslRequest](auto const& hfpr, auto const& zfpr) {
        if (hfpr.second.sensorTimestampNs < zslReq.zsdTimestampNs) {
            MY_LOGW("[DefaultSelector] hbs[F%u] Ts(%" PRId64 ") > zsd(%" PRId64 "), shutterDelay=%" PRId64 "ms X",
                    hfpr.first, hfpr.second.sensorTimestampNs, zslReq.zsdTimestampNs,
                    (hfpr.second.sensorTimestampNs - zslReq.zsdTimestampNs) / 1000000);
            return false;
        }
        if (!containsRequiredStreamBuffer(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] containsRequiredStreamBuffer X", hfpr.first);
            return false;
        }
        if (!containRequiredSensorMode(hfpr.second, *zfpr.second.pRequiredSensorModes)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] containRequiredSensorMode X", hfpr.first);
            return false;
        }
        // add check sensor mode
        if (!isAFStable(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] isAFStable X", hfpr.first);
            return false;
        }
        if (!isAESoftStable(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] isAESoftStable X", hfpr.first);
            return false;
        }
        if (zslReq.suggestFrameSync && !isFameSync(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] isFameSync X", hfpr.first);
            return false;
        }
        return true;
    };
    //
    ///MY_LOGD("selectIn: %s", ZslPolicy::toString(in).c_str());
    SelectOutputParams out;
    auto& hfs = in.historyFrames;
    auto hfIt = hfs.begin();
    auto& vSelected = out.selected;
    uint32_t const zslReqNo = in.zslRequest.requestNo;
    for (auto const& zfpr : in.zslRequest.frames) {
        for (; hfIt != hfs.end() && !isOk(*hfIt, zfpr); ++hfIt) {
            MY_LOGW("[DefaultSelector] zslReqNo(%u) hbs[F%u] X", zslReqNo, hfIt->first);
        }
        if (hfIt == hfs.end()) break;
        MY_LOGI("[DefaultSelector] zslReqNo(%u) hbs[F%u] O => select", zslReqNo, hfIt->first);
        vSelected.emplace(zfpr.first, ZslFrameResult{.frameNo = hfIt->first});
        ++hfIt;
    }
    MY_LOGD("[DefaultSelector] zslReqNo(%u) durationMs(%" PRId64 ") #select(%zu/%zu)",
            zslReqNo, in.zslRequest.durationMs, vSelected.size(), in.zslRequest.frames.size());
    ///MY_LOGD("selectOut: %s", ZslPolicy::toString(out).c_str());
    return out;
}

auto
ZslProcessor::CShotSelector::
select(ISelector::SelectInputParams const& in) const -> ISelector::SelectOutputParams
{
    auto isOk = [zslReq = in.zslRequest](auto const& hfpr, auto const& zfpr) {
        if (!containsRequiredStreamBuffer(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[CShotSelector] hbs[F%u] containsRequiredStreamBuffer X", hfpr.first);
            return false;
        }
        if (!containRequiredSensorMode(hfpr.second, *zfpr.second.pRequiredSensorModes)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] containRequiredSensorMode X", hfpr.first);
            return false;
        }
        return true;
    };
    //
    SelectOutputParams out;
    auto& hfs = in.historyFrames;
    auto hfIt = hfs.begin();
    auto& vSelected = out.selected;
    uint32_t const zslReqNo = in.zslRequest.requestNo;
    for (auto const& zfpr : in.zslRequest.frames) {
        for (; hfIt != hfs.end() && !isOk(*hfIt, zfpr); ++hfIt) {
            MY_LOGW("[CShotSelector] zslReqNo(%u) hbs[F%u] X", zslReqNo, hfIt->first);
        }
        if (hfIt == hfs.end()) break;
        MY_LOGW("[CShotSelector] zslReqNo(%u) hbs[F%u] O => select", zslReqNo, hfIt->first);
        vSelected.emplace(zfpr.first, ZslFrameResult{.frameNo = hfIt->first});
        ++hfIt;
    }
    MY_LOGD("[CShotSelector] zslReqNo(%u) durationMs(%" PRId64 ") #select(%zu/%zu)",
            zslReqNo, in.zslRequest.durationMs, vSelected.size(), in.zslRequest.frames.size());
    return out;
}

auto
ZslProcessor::MultiframeSelector::
select(ISelector::SelectInputParams const& in) const -> ISelector::SelectOutputParams
{
    auto isOk = [zslReq = in.zslRequest](auto const& hfpr, auto const& zfpr) {
        if (!containsRequiredStreamBuffer(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[MultiframeSelector] hbs[F%u] containsRequiredStreamBuffer X", hfpr.first);
            return false;
        }
        if (!containRequiredSensorMode(hfpr.second, *zfpr.second.pRequiredSensorModes)) {
            MY_LOGW("[DefaultSelector] hbs[F%u] containRequiredSensorMode X", hfpr.first);
            return false;
        }
        if (!isAFStable(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[MultiframeSelector] hbs[F%u] isAFStable X", hfpr.first);
            return false;
        }
        if (!isAESoftStable(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[MultiframeSelector] hbs[F%u] isAESoftStable X", hfpr.first);
            return false;
        }
        if (zslReq.suggestFrameSync && !isFameSync(hfpr.second, *zfpr.second.pRequired)) {
            MY_LOGW("[MultiframeSelector] hbs[F%u] isFameSync X", hfpr.first);
            return false;
        }
        return true;
    };
    //
    SelectOutputParams out;
    auto& zfs = in.zslRequest.frames;
    auto zfIt = zfs.begin();
    size_t const frameSize = zfs.size();
    uint32_t const zslReqNo = in.zslRequest.requestNo;
    auto& hfs = in.historyFrames;
    if (hfs.empty()) {
        MY_LOGW("[MultiframeSelector] zslReqNo(%u) durationMs(%" PRId64 ") hfs(%zu)/frameSize(%zu) => PENDING",
                zslReqNo, in.zslRequest.durationMs, hfs.size(), frameSize);
        return out;
    }
    auto zsdHfIt = std::lower_bound(hfs.begin(), hfs.end(), in.zslRequest.zsdTimestampNs,
        [](auto const& hfpr, auto const& zsdTs){
            return hfpr.second.sensorTimestampNs < zsdTs;
        });
    if (zsdHfIt == hfs.end()) {
        MY_LOGW("[MultiframeSelector] future ts, use latest");
        --zsdHfIt;
    }
    auto zsdIdx = std::distance(hfs.begin(), zsdHfIt);
    auto hfIt = std::prev(zsdHfIt, std::min(zsdIdx, (decltype(zsdIdx))(frameSize - 1)));
    // ensure selected frames contain zsd frame.
    MY_LOGD("[MultiframeSelector] zslReqNo(%u) frameSize(%zu) zsdIdx(%zu)/shutterDelay(%" PRId64 "ms) startIdx(%zu)/shutterDelay(%" PRId64 "ms)",
            zslReqNo, frameSize, (size_t)zsdIdx, (zsdHfIt->second.sensorTimestampNs - in.zslRequest.zsdTimestampNs) / 1000000,
            (size_t)std::distance(hfs.begin(), hfIt), (hfIt->second.sensorTimestampNs - in.zslRequest.zsdTimestampNs) / 1000000);
    auto& vSelected = out.selected;
    for (; hfIt != hfs.end(); ++hfIt) {
        if (vSelected.size() == frameSize) break;  // success
        if (isOk(*hfIt, *zfIt)) {
            vSelected.emplace(zfIt->first, ZslFrameResult{.frameNo = hfIt->first});
            ++zfIt;
            MY_LOGI("[MultiframeSelector] zslReqNo(%u) hbs[F%u] O => select", zslReqNo, hfIt->first);
        } else {
            // clear selected, start over
            vSelected.clear();
            zfIt = zfs.begin();
            MY_LOGW("[MultiframeSelector] zslReqNo(%u) hbs[F%u] X => break continuous, start over",
                    zslReqNo, hfIt->first);
        }
    }
    //
    if (vSelected.size() == frameSize) {  // success
        MY_LOGI("[MultiframeSelector] zslReqNo(%u) durationMs(%" PRId64 ") #select(%zu/%zu) => SUCCESS",
                zslReqNo, in.zslRequest.durationMs, vSelected.size(), frameSize);
    } else {
        MY_LOGD("[MultiframeSelector] zslReqNo(%u) durationMs(%" PRId64 ") #select(%zu/%zu) => PENDING",
                zslReqNo, in.zslRequest.durationMs, vSelected.size(), frameSize);
        vSelected.clear();
    }
    return out;
}