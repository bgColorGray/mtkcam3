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

#define LOG_TAG "mtkcam-ppl_fbm"

#include "FrameBufferManager.h"
//
#include <algorithm>
#include <chrono>
#include <functional>
//
#include <cutils/compiler.h>
#include <cutils/properties.h>
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Aee.h>
//
#include <mtkcam/utils/imgbuf/ISecureImageBufferHeap.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam3/pipeline/utils/streaminfo/ImageStreamInfo.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_CONTEXT);

using namespace android;
using namespace NSCam;
using namespace NSCam::Utils::ULog;
using namespace NSCam::v3;
using namespace NSCam::v3::pipeline;
using namespace NSCam::v3::pipeline::NSPipelineContext;

using HalImageStreamBufferT = NSCam::v3::Utils::HalImageStreamBuffer;
using AllocFuncT = BufferProducer::AllocFuncT;
using CallbackFuncT = BufferProducer::CallbackFuncT;

#define ThisNamespace   FrameBufferManagerImpl

#define BUFFER_SOURCE(_a_)                                                  \
    static_cast<IFrameBufferManager::Attribute>(                            \
            static_cast<uint32_t>(_a_)                                      \
          & static_cast<uint32_t>(StreamAttribute::BUFFER_SOURCE_MASK) )

#define VALIDATE_STRING(string) (string!=nullptr ? string : "")

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
static inline std::string toString_ConfigImageStream(const ThisNamespace::ConfigImageStream& o)
{
    std::string os;

    os += toString(o.attribute);

    if (CC_LIKELY( o.pStreamInfo != nullptr )) {
        os += VALIDATE_STRING(o.pStreamInfo->toString().c_str());
    }

    return os;
};


/******************************************************************************
 *
 ******************************************************************************/
static inline std::string toString_ConfigMetaStream(const ThisNamespace::ConfigMetaStream& o)
{
    std::string os;

    os += toString(o.attribute);

    if (CC_LIKELY( o.pStreamInfo != nullptr )) {
        os += VALIDATE_STRING(o.pStreamInfo->toString().c_str());
    }

    return os;
};


/******************************************************************************
 *
 ******************************************************************************/
static auto
generateImgParam(
    IImageStreamInfo const* pStreamInfo
) -> IImageBufferAllocator::ImgParam
{
    IImageStreamInfo::BufPlanes_t const& bufPlanes = pStreamInfo->getAllocBufPlanes();
    switch (pStreamInfo->getAllocImgFormat())
    {
    case eImgFmt_BLOB:{
        return  IImageBufferAllocator::ImgParam(
                    bufPlanes.planes[0].sizeInBytes,
                    0
                );
        }break;

    case eImgFmt_HEIF:
    case eImgFmt_JPEG:{
        return  IImageBufferAllocator::ImgParam(
                    pStreamInfo->getImgSize(),
                    bufPlanes.planes[0].sizeInBytes,
                    0
                );
        }break;

    default:
        break;
    }

    size_t bufStridesInBytes[3] = {0};
    size_t bufBoundaryInBytes[3]= {0};
    size_t bufCustomSizeInBytes[3] = {0};
    for (size_t i = 0; i < bufPlanes.count; i++) {
        bufStridesInBytes[i] = bufPlanes.planes[i].rowStrideInBytes;
        bufCustomSizeInBytes[i] = bufPlanes.planes[i].sizeInBytes;
    }
    return  IImageBufferAllocator::ImgParam(
                pStreamInfo->getAllocImgFormat(),
                pStreamInfo->getImgSize(),
                bufStridesInBytes, bufBoundaryInBytes,
                bufCustomSizeInBytes,
                bufPlanes.count
            );
}


/******************************************************************************
 *
 ******************************************************************************/
static bool isSameAllocLayout(IImageStreamInfo const* s1, IImageStreamInfo const* s2)
{
    //CAM_ULOGME("<CONFIG > %s", s1->toString().c_str());
    //CAM_ULOGME("<REQUEST> %s", s2->toString().c_str());

    if ( s1 == s2 )
        return true;

    if ( s1->getAllocImgFormat() != s2->getAllocImgFormat() )
        return false;

    auto const& bp1 = s1->getAllocBufPlanes();
    auto const& bp2 = s2->getAllocBufPlanes();
    if ( bp1.count != bp2.count )
        return false;

    for (size_t i = 0; i < bp1.count; i++) {
        if ( bp1.planes[i].sizeInBytes != bp2.planes[i].sizeInBytes
          || bp1.planes[i].rowStrideInBytes != bp2.planes[i].rowStrideInBytes ) {
            return false;
        }
    }

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
/**
 * Determine the image stream info for allocating a buffer heap pool.
 *
 * Given a set of image stream info(s), determine an IImageStreamInfo instance
 * for allocating a buffer heap pool. This function could return an existing
 * IImageStreamInfo instance or create a new one.
 *
 * @return
 *  An IImageStreamInfo instance only if all given stream info(s) can share a
 *  buffer pool; otherwise nullptr.
 */
static auto
evaluateSharedAllocStreamInfo(
    android::sp<IImageStreamInfo> const* pStreamInfos,
    size_t count,
    android::Printer& logPrinter
) -> android::sp<IImageStreamInfo>
{
    CAM_TRACE_CALL();

    MY_LOGF_IF(count==0 || pStreamInfos==nullptr || pStreamInfos[0]==nullptr,
        "Bad arguments - count:%zu pStreamInfos:%p", count, pStreamInfos);

    if ( 1 == count ) {
        return nullptr;
        //return pStreamInfos[0];
    }


    /**
     * More than one stream info sharing a buffer pool.
     */

    /**
     * Validate the equality.
     */
    auto const secureInfo = pStreamInfos[0]->getSecureInfo();
    auto const allocImgFormat = pStreamInfos[0]->getAllocImgFormat();
    auto const imgSize = pStreamInfos[0]->getImgSize();

    /**
     * Determine the maximum
     */
    std::string streamName("shared");
    size_t maxBufNum = 0, minInitBufNum = 0;

    size_t    allocBufSizeInBytes = 0;
    BufPlanes allocBufPlanes;
    ::memset(&allocBufPlanes, 0, sizeof(allocBufPlanes));
    auto toTotalSizeInBytes = [](auto const& bp){
            size_t s = 0;
            for (size_t i = 0; i < bp.count; i++) {
                s += bp.planes[i].sizeInBytes;
            }
            return s;
        };

    for (size_t i = 0; i < count; i++) {
        if ( auto pStreamInfo = pStreamInfos[i].get() ) {
            /**
             * Validate
             */
            auto toErrorMsg = [&](){
                    std::ostringstream oss;
                    oss << "[" << i << "|" << count << "] "
                        << "\'" << pStreamInfo->toString() << "\'" << " != "
                        << "\'" << pStreamInfos[0]->toString() << "\'"
                            ;
                    return oss.str();
                };
            if ( secureInfo.type != pStreamInfo->getSecureInfo().type ) {
                logPrinter.printFormatLine(
                    "ERROR [%s] Different SecureInfo: %s",
                    __FUNCTION__, toErrorMsg().c_str());
                return nullptr;
            }
            if ( allocImgFormat != pStreamInfo->getAllocImgFormat() ) {
                logPrinter.printFormatLine(
                    "ERROR [%s] Different AllocImgFormat: %s",
                    __FUNCTION__, toErrorMsg().c_str());
                return nullptr;
            }
            if ( allocImgFormat != eImgFmt_BLOB
              && imgSize != pStreamInfo->getImgSize() ) {
                logPrinter.printFormatLine(
                    "ERROR [%s] Different ImgSize & Non-BLOB: %s",
                    __FUNCTION__, toErrorMsg().c_str());
                return nullptr;
            }

            /**
             * Determine the maximum
             */
            streamName += "-";
            streamName += toHexString(pStreamInfo->getStreamId());

            maxBufNum     = std::max(maxBufNum, pStreamInfo->getMaxBufNum());
            minInitBufNum = std::max(minInitBufNum, pStreamInfo->getMinInitBufNum());

            const size_t _allocBufSizeInBytes = toTotalSizeInBytes(pStreamInfo->getAllocBufPlanes());
            if (allocBufSizeInBytes < _allocBufSizeInBytes) {
                allocBufSizeInBytes = _allocBufSizeInBytes;
                allocBufPlanes = pStreamInfo->getAllocBufPlanes();
            }
        }
    }
    {
        NSCam::v3::Utils::ImageStreamInfoBuilder builder(pStreamInfos[0].get());

        builder.setStreamId(-1L); // don't care
        builder.setStreamName(std::move(streamName));
        builder.setMaxBufNum(maxBufNum);
        builder.setMinInitBufNum(minInitBufNum);
        builder.setAllocBufPlanes(std::move(allocBufPlanes));

        auto ret = builder.build();
        if ( ret != nullptr ) {
            return ret;
        }
    }

    MY_LOGF("Never happen...");
    return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
 namespace
 {
    struct Allocator
    {
        ////    Data Members.
        android::sp<IImageStreamInfo>           mpStreamInfo;
        IImageBufferAllocator::ImgParam         mAllocImgParam;

        ////    Operations.
        Allocator(
            IImageStreamInfo* pStreamInfo,
            IImageBufferAllocator::ImgParam&& rAllocImgParam
        ) : mpStreamInfo(pStreamInfo), mAllocImgParam(rAllocImgParam) {}

        ////    Operator.
        IImageBufferHeap* operator()(const std::shared_ptr<IIonDevice>& pIonDevice = nullptr)
        {
            CAM_TRACE_NAME("Allocator");

            IImageBufferHeap* pImageBufferHeap = NULL;
            if (mpStreamInfo->isSecure())
            {
                pImageBufferHeap = ISecureImageBufferHeap::create(
                                        mpStreamInfo->getStreamName(),
                                        mAllocImgParam,
                                        ISecureImageBufferHeap::AllocExtraParam(0,1,0,MFALSE,mpStreamInfo->getSecureInfo().type),
                                        MFALSE
                                    );
                //
                if (CC_UNLIKELY(pImageBufferHeap == nullptr)) {
                    MY_LOGE("ISecureImageBufferHeap::create: %s", mpStreamInfo->toString().c_str());
                    return nullptr;
                }
            }
            else
            {
                pImageBufferHeap =
                    IIonImageBufferHeap::create(
                        mpStreamInfo->getStreamName(),
                        mAllocImgParam,
                        [&pIonDevice](){
                            IIonImageBufferHeap::AllocExtraParam tmp;
                            tmp.ionDevice = pIonDevice;
                            return tmp;
                        }(),
                        MFALSE
                    );
                //
                if (CC_UNLIKELY(pImageBufferHeap == nullptr)) {
                    MY_LOGE("IIonImageBufferHeap::create: %s", mpStreamInfo->toString().c_str());
                    return nullptr;
                }
            }
            return pImageBufferHeap;
        }
    };
 }


/******************************************************************************
 *
 ******************************************************************************/
static auto
allocateHalImageStreamBufferRuntime(
    IImageStreamInfo* pStreamInfo,
    android::sp<IImageStreamBuffer>& rpStreamBuffer
) -> int
{
    rpStreamBuffer = new HalImageStreamBufferT(
                            pStreamInfo,
                            nullptr,
                            Allocator(pStreamInfo, generateImgParam(pStreamInfo))()
                        );

    if (CC_UNLIKELY(rpStreamBuffer == nullptr)) {
        MY_LOGE("<REQUEST> %s", pStreamInfo->toString().c_str());
        return UNKNOWN_ERROR;
    }
    //MY_LOGD("[%s] runtime alloc imageSB (%p)", pStreamInfo->toString().c_str(), rpStreamBuffer.get());  //@@
    return OK;
};


/******************************************************************************
 *
 ******************************************************************************/
static auto
createHalStreamBufferPool(
    android::sp<IImageStreamInfo>const& pAllocatorStreamInfo,
    android::sp<IImageStreamInfo>const& pProducerStreamInfo,
    CallbackFuncT const& callbackFunc,
    android::sp<BufferProducer> pSharedPoolProducer = nullptr
) -> android::sp<BufferProducer>
{
    android::sp<BufferProducer> pPool =
        new BufferProducer(
                VALIDATE_STRING(pAllocatorStreamInfo->getStreamName()),
                pProducerStreamInfo,
                callbackFunc,
                pSharedPoolProducer
            );  // implment w/ Heap_Pool + SB_Producer

    if  (CC_UNLIKELY( pPool == nullptr )) {
        MY_LOGE("[P\"%s\" A\"%s\"] Failed to create a pool",
            pProducerStreamInfo->toString().c_str(),
            pAllocatorStreamInfo->toString().c_str());
        return nullptr;
    }

    int err = pPool->initProducer(
                        pAllocatorStreamInfo->getMaxBufNum(),
                        pAllocatorStreamInfo->getMinInitBufNum(),
                        Allocator(
                            pAllocatorStreamInfo.get(),
                            generateImgParam(pAllocatorStreamInfo.get())
                        )
                    );

    if  (CC_UNLIKELY( 0 != err )) {
        MY_LOGE("[P\"%s\" A\"%s\"] (%p) failed on initPool - err:%d(%s)",
            pProducerStreamInfo->toString().c_str(),
            pAllocatorStreamInfo->toString().c_str(),
            pPool.get(), err, ::strerror(-err));
        return nullptr;
    }

    return pPool;
}


/******************************************************************************
 *
 ******************************************************************************/
template <class T>
void releaseBuffer(T/*android::sp<IImageStreamBuffer>*/& rpBuffer)
{
    rpBuffer->releaseBuffer();  // return to the pool
    rpBuffer = nullptr;
}

template <>
void releaseBuffer(android::sp<IMetaStreamBuffer>& rpBuffer)
{
    rpBuffer = nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
extern "C" void
createFrameBufferManager(
    std::shared_ptr<IFrameBufferManager>* out,
    char const* name
)
{
    if (CC_UNLIKELY( out == nullptr )) {
        return;
    }

    auto pInst = std::make_shared<FrameBufferManagerImpl>(name);
    if (CC_UNLIKELY( pInst == nullptr )) {
        return;
    }

    bool ret = pInst->initialize(pInst);
    if (CC_UNLIKELY( ! ret )) {
        return;
    }

    *out = pInst;
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
FrameBufferManagerImpl(char const* name)
    : mName(VALIDATE_STRING(name))
    , mLogLevel(::property_get_int32("vendor.debug.camera.log.pipeline.fbm", 0))
    , mAcquireTimeoutMS(::property_get_int32("vendor.debug.camera.pipeline.fbm.acquire.timeout", 1500))
{
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
~FrameBufferManagerImpl()
{
    MY_LOGI("+ mDebuggeeCookie:%p", mDebuggeeCookie.get());
    if (auto pDbgMgr = IDebuggeeManager::get()) {
        pDbgMgr->detach(mDebuggeeCookie);
    }
    mDebuggeeCookie = nullptr;
    MY_LOGI("-");
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
initialize(std::shared_ptr<ThisT> sp) -> bool
{
    if (CC_LIKELY(sp != nullptr)) {
        if (auto pDbgMgr = IDebuggeeManager::get()) {
            mDebuggeeCookie = pDbgMgr->attach(sp, 0);
        }
    }
    //
    mWeakThis = sp;
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpState(
    android::Printer& printer
) -> void
{
    dumpConfigStreams(printer);
    dumpBufferPools(printer);
    dumpBufferState(printer);
    printer.printFormatLine("mInSelect(%d)", mInSelect.load());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpBufferState(
    android::Printer& printer
) -> void
{
    {
        std::unique_lock _li(mInFlightLock, std::defer_lock);
        if (_li.try_lock_for(std::chrono::milliseconds(500))) {
            dumpInFlightListLocked(printer);
        } else {
            printer.printLine("Timeout waiting for mInFlightLock");
        }
    }
    {
        std::unique_lock _lr(mReadyLock, std::defer_lock);
        if (_lr.try_lock_for(std::chrono::milliseconds(500))) {
            dumpReadyListLocked(printer);
        } else {
            printer.printLine("Timeout waiting for mReadyLock");
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpConfigStreams(
    android::Printer& printer
) -> void
{
    {
        printer.printLine("<image streams>");
        auto tmp = [this](){ android::RWLock::AutoRLock _l(mConfigLock); return mConfigImageStreams; }();
        for (auto const& v : tmp) {
            if ( auto p = v.second.get() ) {
                printer.printFormatLine("    %s", toString_ConfigImageStream(*p).c_str());
            }
        }
    }
    {
        printer.printLine("<meta streams>");
        auto tmp = [this](){ android::RWLock::AutoRLock _l(mConfigLock); return mConfigMetaStreams; }();
        for (auto const& v : tmp) {
            if ( auto p = v.second.get() ) {
                printer.printFormatLine("    %s", toString_ConfigMetaStream(*p).c_str());
            }
        }
    }
    {
        auto tmp = [this](){ android::RWLock::AutoRLock _l(mConfigLock); return mConfigPoolSharedStreams; }();
        if ( tmp != nullptr && !(*tmp).empty() ) {
            printer.printLine("<pool-shared streams>");
            for (auto const& s : *tmp) {
                std::ostringstream oss;
                oss << '(';
                oss << std::hex;
                for (auto streamId : s) {
                    oss << ' ' << streamId;
                }
                oss << " )";
                printer.printFormatLine("    %s", oss.str().c_str());
            }
        }
    }
    if (mConfigLogsLock.try_lock()) {
        if ( ! mConfigLogs.empty() ) {
            printer.printLine("<config messages>");
            for (auto const& msg : mConfigLogs) {
                printer.printFormatLine("    %s", msg.c_str());
            }
        }
        mConfigLogsLock.unlock();
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpBufferPools(
    android::Printer& printer
) -> void
{
    {
        printer.printLine("<image buffer pool>");
        auto tmp = [this](){ android::RWLock::AutoRLock _l(mConfigLock); return mConfigImageStreams; }();
        for (auto const& v : tmp) {
            if (auto p = v.second.get()) {
                if (p->pPool != nullptr) {
                    p->pPool->dump();  //TODO: dump by printer
                }
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::InFlightBufferSet::
toString() const -> std::string
{
    auto getString = [](auto const& readySbs, std::set<StreamId_T> const& all)
    {
        std::ostringstream ross, ioss;  // readySbs & inFlightSbs
        ross << std::hex;
        ioss << std::hex;
        for (auto const& sId : all) {
            if (std::find_if(readySbs.begin(), readySbs.end(), [sId](const auto& sb) {
                return sb && sId == sb->getStreamInfo()->getStreamId(); }) != readySbs.end()) {
                ross << " " << sId;
            } else {
                ioss << " " << sId;
            }
        }
        return "{ready:" + ross.str() + "; inflight:" + ioss.str() + "}";
    };
    //
    std::ostringstream oss;
    oss << "{"
        << " F" << frameNo
        << " R" << requestNo
        << " Ts" << sensorTimestamp
        << " halImage" << getString(vpHalImageStreamBuffers, trackParams->halImageStreams)
        << " halMeta" << getString(vpHalMetaStreamBuffers, trackParams->halMetaStreams)
        << " appMeta" << getString(vpAppMetaStreamBuffers, trackParams->appMetaStreams)
        << " }";
    return oss.str();
}

auto
ThisNamespace::
dumpInFlightListLocked(
    android::Printer& printer
) -> void
{
    printer.printLine("<InFlightList>");
    for (auto const& fbs : mInFlightList) {
        printer.printFormatLine("    %s", fbs.toString().c_str());
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
dumpReadyListLocked(
    android::Printer& printer
) -> void
{
    printer.printLine("<ReadyList>");
    for (auto const& hbs : mReadyList) {
        printer.printFormatLine("    %s", toString(hbs).c_str());
    }
    //
    std::ostringstream oss;
    for (auto const& pr : mReadyStreamCounters) {
        oss << std::hex << pr.first << "(" << std::dec << pr.second << ") ";
    }
    printer.printFormatLine("mReadyStreamCounters: %s", oss.str().c_str());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
beginConfigure() -> void
{
    {
        struct MyPrinter : public Printer
        {
            ULogPrinter             mLogPrinter;
            std::list<std::string>& mrLogs;
            std::mutex&             mrLogsLock;

            MyPrinter(std::list<std::string>& logs, std::mutex& logsLock)
                : mLogPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_INFO, nullptr)
                , mrLogs(logs)
                , mrLogsLock(logsLock)
            {}

        virtual void printLine(const char* string)
            {
                if (!string)  return;
                mLogPrinter.printLine(string);

                if (mrLogsLock.try_lock()) {
                    mrLogs.push_back(std::string(string));
                    mrLogsLock.unlock();
                }
            }
        };

        android::RWLock::AutoWLock _l(mConfigLock);
        mConfigPrinter = std::make_shared<MyPrinter>(mConfigLogs, mConfigLogsLock);
        MY_LOGF_IF(mConfigPrinter==nullptr, "Failed on std::make_shared<MyPrinter>()");
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
endConfigure() -> int
{
    {
        android::RWLock::AutoWLock _l(mConfigLock);
        mConfigPrinter = nullptr;
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
configureImageStreams(
    std::vector<AddImageStream> const& streams,
    std::vector<std::set<StreamId_T>> const* streamsWithSharedPool
) -> int
{
    CAM_TRACE_CALL();

    /**
     * Given a target stream id, transfer it from a set of test streams to the
     * result streams.
     *
     * @targetStreamID A given target stream id.
     *
     * @testStreams A set of test streams.
     *
     * @resultStreams The result streams.
     */
    auto transferTo = [](
            auto const targetStreamID,
            std::vector<AddImageStream>& testStreams,
            std::vector<AddImageStream>& resultStreams) -> bool
    {
        for (auto it = testStreams.begin(); it != testStreams.end(); it++) {
            if (auto pStreamInfo = it->pStreamInfo.get()) {
                if ( pStreamInfo->getStreamId() == targetStreamID ) {
                    resultStreams.push_back(*it);
                    testStreams.erase(it);
                    return true;
                }
            }
        }
        return false;
    };


    // Copy to a local variable.
    auto tempStreams = streams;

    // Process pool-shared streams.
    if ( streamsWithSharedPool != nullptr ) {
        for (auto const& sharedIDs : *streamsWithSharedPool) {

            std::vector<AddImageStream> tempSharedStreams;

            // Transfer all shared streams from tempStreams to tempSharedStreams.
            for (auto sharedID : sharedIDs) {
                if ( ! transferTo(sharedID, tempStreams, tempSharedStreams) ) {
                    mConfigPrinter->printFormatLine(
                        "WARNING [%s] Couldn't find the shared streamId:%s",
                        __FUNCTION__, toHexString(sharedID).c_str());
                }
            }

            // Validate all shared streams which having the same attributes.
            for (auto it = tempSharedStreams.begin(); it != tempSharedStreams.end(); ) {

                if ( BUFFER_SOURCE(it->attribute) != IFrameBufferManager::Attribute::BUFFER_SOURCE_POOL ) {
                    mConfigPrinter->printFormatLine(
                        "WARNING [%s] \'%s\': pool-shared but not BUFFER_SOURCE_POOL - attribute:%s",
                        __FUNCTION__, it->pStreamInfo->getStreamName(), toString(it->attribute).c_str());
                    tempStreams.push_back(*it);
                    it = tempSharedStreams.erase(it);
                    continue;
                }

                if ( it->attribute != tempSharedStreams[0].attribute ) {
                    mConfigPrinter->printFormatLine(
                        "WARNING [%s] \'%s\': different attribute - attribute:%s",
                        __FUNCTION__, it->pStreamInfo->getStreamName(), toString(it->attribute).c_str());
                    tempStreams.push_back(*it);
                    it = tempSharedStreams.erase(it);
                    continue;
                }

                it++;
            }

            // Add pool-shared streams.
            if ( ! tempSharedStreams.empty() ) {
                AddImageStreams tmp;
                tmp.attribute = tempSharedStreams[0].attribute;
                tmp.pProvider = nullptr;
                for (auto const& v : tempSharedStreams) {
                    tmp.vStreamInfo.push_back(v.pStreamInfo);
                }

                int err = addImageStreams(tmp);
                if (err != 0) {
                    return err;
                }
            }
        }
    }

    // Add non-pool-shared streams.
    for (auto const& v : tempStreams) {
        int err = addImageStream(v);
        if (err != 0) {
            return err;
        }
    }

    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
configureStreams(ConfigureStreams const& arg) -> int
{
    CAM_TRACE_NAME("IFrameBufferManager::configureStreams");

    // pool-shared (image) streams
    {
        android::RWLock::AutoWLock _l(mConfigLock);
        mConfigPoolSharedStreams = arg.configPoolSharedStreams;
    }

    // image streams
    {
        int err = configureImageStreams(
                    arg.configImageStreams,
                    arg.configPoolSharedStreams.get());
        if (err != 0) {
            return err;
        }
    }

    // meta streams
    for (auto const& v : arg.configMetaStreams) {
        int err = addMetaStream(v);
        if (err != 0) {
            return err;
        }
    }

    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
addImageStreams(AddImageStreams const& arg) -> int
{
    CAM_TRACE_CALL();

    auto addStream = [this, &arg](auto pStreamInfo, auto refineResult) -> int {
        auto pConfigStream = std::make_shared<ConfigImageStream>();
        if (CC_UNLIKELY( pConfigStream == nullptr )) {
            MY_LOGE("[%s] Failed on make_shared<ConfigImageStream>", pStreamInfo->toString().c_str());
            return NO_MEMORY;
        }

        pConfigStream->pStreamInfo = pStreamInfo;
        pConfigStream->attribute = arg.attribute;
        refineResult(pConfigStream.get());
        {
            android::RWLock::AutoWLock _l(mConfigLock);
            mConfigImageStreams[pStreamInfo->getStreamId()] = pConfigStream;
        }
        return OK;
    };

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    switch (BUFFER_SOURCE(arg.attribute))
    {
    case IFrameBufferManager::Attribute::BUFFER_SOURCE_POOL:{
        auto const& vStreamInfo = arg.vStreamInfo;
        if ( vStreamInfo.empty() ) {
            MY_LOGW("empty stream info");
            return OK;
        }

        android::sp<BufferProducer> pSharedPoolProducer;
        auto const pAllocatorStreamInfo =
            evaluateSharedAllocStreamInfo(vStreamInfo.data(), vStreamInfo.size(), *mConfigPrinter);

        for (auto const& pStreamInfo : vStreamInfo) {
            auto pPool = createHalStreamBufferPool(
                            /*Allocator*/(pAllocatorStreamInfo!=nullptr ? pAllocatorStreamInfo : pStreamInfo),
                            /*Producer*/  pStreamInfo,
                            [this](StreamId_T streamId, bool bDumpLog){ tryReleaseOneHistoryBufferSet(streamId, bDumpLog); },
                            pSharedPoolProducer
                        );
            if (CC_UNLIKELY( pPool == nullptr )) {
                MY_LOGE("[%s] Failed on createHalStreamBufferPool", pStreamInfo->toString().c_str());
                return UNKNOWN_ERROR;
            }

            // Keep the first producer for pool-shared case (i.e. pAllocatorStreamInfo!=nullptr)
            if (pSharedPoolProducer == nullptr && pAllocatorStreamInfo != nullptr) {
                pSharedPoolProducer = pPool;
            }

            int err = addStream(pStreamInfo,
                        [&pPool](ConfigImageStream* pConfigStream){
                            pConfigStream->pPool = pPool;
                        });
            if (err != 0) {
                return err;
            }
        }
        }break;

    case IFrameBufferManager::Attribute::BUFFER_SOURCE_PROVIDER:{
        for (auto const& pStreamInfo : arg.vStreamInfo) {
            if (CC_UNLIKELY( arg.pProvider == nullptr )) {
                MY_LOGE("[%s] Bad provider", pStreamInfo->toString().c_str());
                return BAD_VALUE;
            }
            int err = addStream(pStreamInfo,
                        [&arg](ConfigImageStream* pConfigStream){
                            pConfigStream->pProvider = arg.pProvider;
                        });
            if (err != 0) {
                return err;
            }
        }
        }break;

    default:{
        for (auto const& pStreamInfo : arg.vStreamInfo) {
            int err = addStream(pStreamInfo, [](ConfigImageStream* pConfigStream __unused){});
            if (err != 0) {
                return err;
            }
        }
        }break;
    }

    return OK;
}


auto
ThisNamespace::
addImageStream(AddImageStream const& arg) -> int
{
    AddImageStreams tmp;
    tmp.attribute = arg.attribute;
    tmp.pProvider = arg.pProvider;
    if ( arg.pStreamInfo != nullptr ) {
        tmp.vStreamInfo.push_back(arg.pStreamInfo);
    }
    return addImageStreams(tmp);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
addMetaStream(AddMetaStream const& arg) -> int
{
    auto pStreamInfo = arg.pStreamInfo;

    auto pConfigStream = std::make_shared<ConfigMetaStream>();
    if (CC_UNLIKELY( pConfigStream == nullptr )) {
        MY_LOGE("[%s] Failed on make_shared<ConfigMetaStream>", pStreamInfo->toString().c_str());
        return NO_MEMORY;
    }

    pConfigStream->pStreamInfo = arg.pStreamInfo;
    pConfigStream->attribute = arg.attribute;

    {
        android::RWLock::AutoWLock _l(mConfigLock);
        mConfigMetaStreams[pStreamInfo->getStreamId()] = pConfigStream;
    }

    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
queryConfigImageStream(
    StreamId_T streamId
) const -> std::shared_ptr<ConfigImageStream>
{
    android::RWLock::AutoRLock _l(mConfigLock);

    auto it = mConfigImageStreams.find(streamId);
    if (CC_LIKELY( it != mConfigImageStreams.end() )) {
        return it->second;
    }

    MY_LOGW("couldn't find streamId:%#" PRIx64 "", streamId);
    return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
template <typename T> inline
auto targetLikelyFront(T& List, int64_t const& ts)
{
    auto it = List.begin();
    for (; it != List.end(); ++it) {
        if (ts <= it->sensorTimestamp) break;
    }
    return it;
}

template <typename T> inline
auto targetLikelyBack(T& List, int64_t const& ts)
{
    auto rit = List.rbegin();
    for (; rit != List.rend(); ++rit) {
        if (ts > rit->sensorTimestamp) break;
    }
    if (rit == List.rend()) {
        return List.begin();
    }
    return rit.base();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getPoolStreams(StreamId_T const streamId) -> std::set<StreamId_T>
{
    if (!mConfigPoolSharedStreams) return {streamId};
    for (auto const& s : *mConfigPoolSharedStreams) {
        if (s.find(streamId) != s.end()) return s;
    }
    return {streamId};
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
increaseReadyCounters(HistoryBufferSet const& hbs, int32_t const step) -> void
{
    auto increaseCounter = [this, step](auto const& vSb) {
        for (auto const& sb : vSb) {
            auto const pStreamInfo = sb->getStreamInfo();
            if (pStreamInfo) {
                mReadyStreamCounters[*getPoolStreams(pStreamInfo->getStreamId()).begin()] += step;
            }
        }
    };
    //
    increaseCounter(hbs.vpHalImageStreamBuffers);
    increaseCounter(hbs.vpHalMetaStreamBuffers);
    increaseCounter(hbs.vpAppMetaStreamBuffers);
}


/******************************************************************************
 *
 ******************************************************************************/
template <typename T>
auto
ThisNamespace::
releaseOneBufferSet(T& bufferSet) -> void
{
    auto releaseBuffers = [](auto& vBuffers) {
        for (auto& buf : vBuffers) {
            releaseBuffer(buf);
        }
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    releaseBuffers(bufferSet.vpHalImageStreamBuffers);
    releaseBuffers(bufferSet.vpHalMetaStreamBuffers);
    releaseBuffers(bufferSet.vpAppMetaStreamBuffers);
    MY_LOGD_IF((mLogLevel > 1), "release [R%u F%u]", bufferSet.requestNo, bufferSet.frameNo);
}


auto
ThisNamespace::
tryReleaseOneHistoryBufferSet(StreamId_T streamId __unused, bool bDumpLog __unused) -> void
{
    std::set<StreamId_T> ss = getPoolStreams(streamId);
    MY_LOGD_IF((mLogLevel > 1), "stream(%#" PRIx64 ") ", streamId);
    {
        std::unique_lock _lr(mReadyLock);
        auto hbsIt = std::find_first_of(
            mReadyList.begin(), mReadyList.end(), ss.begin(), ss.end(),
            [](auto const& hbs, auto const streamId) {
                auto doContain = [streamId](auto const& vSb) {
                    return std::any_of(
                        vSb.begin(), vSb.end(), [streamId](auto const& sb) {
                            auto const pStreamInfo = sb->getStreamInfo();
                            return pStreamInfo && (pStreamInfo->getStreamId() == streamId);
                        });
                };
                return doContain(hbs.vpHalImageStreamBuffers) ||
                       doContain(hbs.vpHalMetaStreamBuffers) ||
                       doContain(hbs.vpAppMetaStreamBuffers);
            });
        if (hbsIt != mReadyList.end()) {
            increaseReadyCounters(*hbsIt, -1);
            auto tmp = std::move(*hbsIt);
            mReadyList.erase(hbsIt);
            _lr.unlock();
            releaseOneBufferSet(tmp);
            return;
        }
    }
    if (bDumpLog) {
        std::ostringstream oss;
        oss << std::hex;
        for (auto s : ss) oss << s << " ";
        MY_LOGW("stream(%#" PRIx64 "): no {%s} in ReadyList", streamId, oss.str().c_str());
        ULogPrinter printer(__ULOG_MODULE_ID, LOG_TAG,
                            DetailsType::DETAILS_DEBUG);
        dumpBufferPools(printer);
        dumpBufferState(printer);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
requestStreamBuffer(
    requestStreamBuffer_cb cb,
    RequestStreamBuffer const& arg
) -> int
{
    auto trace = [](auto const& arg, std::string const& title){
        CAM_TRACE_NAME(!ATRACE_ENABLED()?"":
            (title + " requestStreamBuffer " + VALIDATE_STRING(arg.streamInfo->getStreamName()) +
            "|request:" + std::to_string(arg.requestNo) + " frame:" + std::to_string(arg.frameNo)).c_str()
        );
    };

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int err = UNKNOWN_ERROR;
    auto pStreamInfo = const_cast<IImageStreamInfo*>(arg.streamInfo);
    auto const streamId = pStreamInfo->getStreamId();
    android::sp<IImageStreamBuffer> rpImageStreamBuffer;

    auto pConfigImageStream = queryConfigImageStream(streamId);
    if (CC_UNLIKELY( pConfigImageStream == nullptr )) {
        MY_LOGE("[requestNo:%u frameNo:%u] Couldn't find %s",
            arg.requestNo, arg.frameNo, toString_ConfigImageStream(*pConfigImageStream).c_str());
        return err;
    }

    switch (pConfigImageStream->attribute)
    {
    case IFrameBufferManager::Attribute::APP_IMAGE_PROVIDER:{
            auto pProvider = pConfigImageStream->pProvider;
            {
                trace(arg, "APP_IMAGE_PROVIDER");
                err = pProvider->requestStreamBuffer(
                        [&](auto const& sb){
                            rpImageStreamBuffer = sb;
                            cb(rpImageStreamBuffer);
                        },
                        IImageStreamBufferProvider::RequestStreamBuffer{
                            .pStreamInfo = pStreamInfo,
                            .timeout = UINT64_MAX,
                            .requestNo = arg.requestNo,
                        });
            }
        }break;

    case IFrameBufferManager::Attribute::HAL_IMAGE_PROVIDER:{
        if ( ! isSameAllocLayout(pConfigImageStream->pStreamInfo.get(), pStreamInfo) )
        {
            MY_LOGD("[requestNo:%u frameNo:%u] PROVIDER -> RUNTIME due to alloc layout change", arg.requestNo, arg.frameNo);
            {
                trace(arg, "PROVIDER -> RUNTIME");
                err = allocateHalImageStreamBufferRuntime(pStreamInfo, rpImageStreamBuffer);
                cb(rpImageStreamBuffer);
            }
        }
        else
        {
            auto pProvider = pConfigImageStream->pProvider;
            {
                trace(arg, "HAL_IMAGE_PROVIDER");
                err = pProvider->requestStreamBuffer(
                        [&](auto const& sb){
                            rpImageStreamBuffer = sb;
                            cb(rpImageStreamBuffer);
                        },

                        IImageStreamBufferProvider::RequestStreamBuffer{
                            .pStreamInfo = pStreamInfo,
                            .timeout = UINT64_MAX,
                            .requestNo = arg.requestNo,
                        });
            }
            //
            if  (   CC_LIKELY(rpImageStreamBuffer != nullptr)
                && (rpImageStreamBuffer->getStreamInfo() != pStreamInfo) )
            {
                rpImageStreamBuffer->replaceStreamInfo(pStreamInfo);
            }
        }
        }break;

    case IFrameBufferManager::Attribute::HAL_IMAGE_POOL:{
        if ( ! isSameAllocLayout(pConfigImageStream->pStreamInfo.get(), pStreamInfo) )
        {
            MY_LOGD("[requestNo:%u frameNo:%u] POOL -> RUNTIME due to alloc layout change", arg.requestNo, arg.frameNo);
            {
                trace(arg, "POOL -> RUNTIME");
                err = allocateHalImageStreamBufferRuntime(pStreamInfo, rpImageStreamBuffer);
                cb(rpImageStreamBuffer);
            }
        }
        else
        {
            auto pPool = pConfigImageStream->pPool;
            {
                trace(arg, "POOL");
                MY_LOGD_IF((mLogLevel > 1), "[requestNo:%u frameNo:%u] acquireFromPool (%s)", arg.requestNo, arg.frameNo, pPool->poolName());
                err = pPool->acquireFromPool(pPool->poolName(),
                                             rpImageStreamBuffer,
                                             ::ms2ns(mAcquireTimeoutMS));
                cb(rpImageStreamBuffer);
            }
            //
            if  (   CC_LIKELY(rpImageStreamBuffer != nullptr)
                && (rpImageStreamBuffer->getStreamInfo() != pStreamInfo) )
            {
                rpImageStreamBuffer->replaceStreamInfo(pStreamInfo);
            }
        }
        }break;

    case IFrameBufferManager::Attribute::HAL_IMAGE_RUNTIME:{
        {
            trace(arg, "RUNTIME");
            err = allocateHalImageStreamBufferRuntime(pStreamInfo, rpImageStreamBuffer);
            cb(rpImageStreamBuffer);
        }
        }break;

    default:{
        MY_LOGE("[requestNo:%u frameNo:%u] Unsupported attribute:%s for %s",
            arg.requestNo, arg.frameNo,
            toString(pConfigImageStream->attribute).c_str(),
            toString_ConfigImageStream(*pConfigImageStream).c_str());
        }break;
    }
    //
    MY_LOGE_IF(0!=err || rpImageStreamBuffer==nullptr,
        "[requestNo:%u frameNo:%u] err:%d(%s) %s",
        arg.requestNo, arg.frameNo, err, ::strerror(-err),
        toString_ConfigImageStream(*pConfigImageStream).c_str()
    );
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
returnResult(
    ReturnResult const& arg
) -> void
{
    std::ostringstream oss, *pOss = ((mLogLevel > 1) ? &oss : nullptr);

    if (pOss) {
        (*pOss) << "[R" << arg.requestNo << " F" << arg.frameNo << "]"
                << "TS:" << arg.sensorTimestamp
                << ' ' << (arg.isFrameDestroyed ? 'X' : ' ');
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Lambda Functions
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto releaseBuffers = [pOss](auto pBuffers) {
        if (pBuffers != nullptr) {
            for (auto& buf : *pBuffers) {
                if (buf != nullptr) {
                    if (pOss) {
                        (*pOss) << ' ' << buf->getName() << "(X) ";
                    }
                    releaseBuffer(buf);
                }
            }
            pBuffers->clear();
        }
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    bool addToReadyList = false;
    InFlightBufferSet tmp;
    {
        std::scoped_lock _li(mInFlightLock);
        auto pInFlight = find_if(mInFlightList.begin(), mInFlightList.end(),
                                 [&arg](auto const& fbs) { return fbs.frameNo == arg.frameNo; });

        if (pInFlight != mInFlightList.end()) {
            /**
             * Save buffers which are in the trackParams, and release the others.
             */
            auto saveBuffers = [pOss, pInFlight](auto pSrcList, auto& destVec, auto const& checkList)
            {
                if (pSrcList != nullptr) {
                    for (auto it = (*pSrcList).begin(); it != (*pSrcList).end(); ) {
                        auto const pStreamInfo = (*it)->getStreamInfo();
                        if (pStreamInfo && checkList.find(pStreamInfo->getStreamId()) != checkList.end()) {
                            if (pOss) {
                                (*pOss) << ' ' << (*it)->getName() << "(O) ";
                            }
                            destVec.push_back(std::move(*it));
                            (pInFlight->trackCnt)--;
                            it = (*pSrcList).erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            };
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            if (pInFlight->sensorTimestamp == 0)    pInFlight->sensorTimestamp = arg.sensorTimestamp;
            saveBuffers(arg.pHalImageStreamBuffers, pInFlight->vpHalImageStreamBuffers, pInFlight->trackParams->halImageStreams);
            saveBuffers(arg.pHalMetaStreamBuffers, pInFlight->vpHalMetaStreamBuffers, pInFlight->trackParams->halMetaStreams);
            saveBuffers(arg.pAppMetaStreamBuffers, pInFlight->vpAppMetaStreamBuffers, pInFlight->trackParams->appMetaStreams);

            if (arg.isFrameDestroyed && (pInFlight->trackCnt > 0)) {
                MY_LOGE("[requestNo:%u frameNo:%u] isFrameDestroyed(%d), but still have #%zu left",
                    arg.requestNo, arg.frameNo, arg.isFrameDestroyed, pInFlight->trackCnt);
                pInFlight->sensorTimestamp = 0;  // mark as error frame, releaseOneBufferSet in addToReadyList
                pInFlight->trackCnt = 0;  // redirect to move to addToReadyList
            }

            if (pInFlight->trackCnt == 0) {  // move to ReadyList
                tmp = std::move(*pInFlight);
                mInFlightList.erase(pInFlight);
                addToReadyList = true;
            }
        }
    }

    releaseBuffers(arg.pHalImageStreamBuffers);
    releaseBuffers(arg.pHalMetaStreamBuffers);
    releaseBuffers(arg.pAppMetaStreamBuffers);

    if (addToReadyList) {
        if (tmp.sensorTimestamp == 0 /*error frame*/)
        {
            if (pOss) {
                (*pOss) << " => release bufferSet (TS: " << tmp.sensorTimestamp << ")";
            }
            releaseOneBufferSet(tmp);
        }
        else
        {
            std::scoped_lock _lr(mReadyLock);
            auto it = mReadyList.insert(targetLikelyBack(mReadyList, tmp.sensorTimestamp),
                              HistoryBufferSet{
                                  .frameNo = tmp.frameNo,
                                  .requestNo = tmp.requestNo,
                                  .sensorTimestamp = tmp.sensorTimestamp,
                                  .vpHalImageStreamBuffers = std::move(tmp.vpHalImageStreamBuffers),
                                  .vpHalMetaStreamBuffers = std::move(tmp.vpHalMetaStreamBuffers),
                                  .vpAppMetaStreamBuffers = std::move(tmp.vpAppMetaStreamBuffers)
                                });
            increaseReadyCounters(*it, 1);

            if (pOss) {
                (*pOss) << " => move to ReadyList";
            }
        }
    }

    MY_LOGD_IF(pOss, "%s", (*pOss).str().c_str());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
trackFrameResult(TrackFrameResult const& arg) -> int
{
    if (arg.trackParams == nullptr ||
        (arg.trackParams->halImageStreams.empty() &&
         arg.trackParams->halMetaStreams.empty() &&
         arg.trackParams->appMetaStreams.empty())) {
        MY_LOGD_IF((mLogLevel > 1), "[R%u F%u] Nothing to track, no need to create FBS",
                    arg.requestNo, arg.frameNo);
    } else {
        {
            std::scoped_lock _li(mInFlightLock);
            mInFlightList.emplace_back(arg.frameNo, arg.requestNo, arg.trackParams);
        }
        MY_LOGD_IF((mLogLevel > 1), "[requestNo:%u frameNo:%u] create FBS, Track:%s",
                    arg.requestNo, arg.frameNo, toString(*arg.trackParams).c_str());
    }

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
enableFrameResultRecyclable(
    uint32_t frameNo,
    bool forceToRecycle
) -> void
{
    MY_LOGE_IF((mLogLevel > 1), "TODO - frameNo:%u forceToRecycle:%d", frameNo, forceToRecycle);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
beginSelect() -> std::optional<std::list<HistoryBufferSet>>
{
    mInSelect = true;
    std::list<HistoryBufferSet> destList;
    {
        std::scoped_lock _lr(mReadyLock);
        destList.splice(destList.begin(), mReadyList);
        for (auto const& hbs : destList) {
            increaseReadyCounters(hbs, -1);
        }
    }
    return destList.size()? std::make_optional(std::move(destList)) : std::nullopt;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
endSelect() -> void
{
    mInSelect = false;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
returnUnselectedSet(
    ReturnUnselectedSet&& arg
) -> void
{
    auto concat = [](auto& dest, auto& src){
        dest.insert(dest.end(), src.begin(), src.end());
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    MY_LOGD_IF((mLogLevel > 1), "[R%u F%u] welcome back, %s", arg.hbs.requestNo, arg.hbs.frameNo, arg.keep ? "keep" : "release");
    if (arg.keep == false) {
        releaseOneBufferSet(arg.hbs);
    } else {
        std::scoped_lock _lr(mReadyLock);
        increaseReadyCounters(arg.hbs, 1);
        auto it = targetLikelyFront(mReadyList, arg.hbs.sensorTimestamp);
        if (it != mReadyList.end() &&
            arg.hbs.sensorTimestamp == it->sensorTimestamp) {  // already exist. (partial return)
            concat(it->vpHalImageStreamBuffers, arg.hbs.vpHalImageStreamBuffers);
            concat(it->vpHalMetaStreamBuffers, arg.hbs.vpHalMetaStreamBuffers);
            concat(it->vpAppMetaStreamBuffers, arg.hbs.vpAppMetaStreamBuffers);
        } else {
            mReadyList.insert(it, std::move(arg.hbs));
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getInFlightInfos() const -> std::vector<ZslPolicy::InFlightInfo>
{
    auto generateRequiredStreams = [](auto const& trackParams) {
        return std::make_shared<ZslPolicy::RequiredStreams>(
            ZslPolicy::RequiredStreams{
                .halImages = trackParams->halImageStreams,
                .halMetas = trackParams->halMetaStreams,
                .appMetas = trackParams->appMetaStreams
            });
    };
    //
    std::scoped_lock _li(mInFlightLock);
    std::vector<ZslPolicy::InFlightInfo> out;
    out.reserve(mInFlightList.size());
    for (auto const&  fbs : mInFlightList) {
        out.emplace_back(ZslPolicy::InFlightInfo{
            .frameNo = fbs.frameNo,
            .requestNo = fbs.requestNo,
            .pContains = generateRequiredStreams(fbs.trackParams)
        });
    }
    return out;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
cloneHBS(HistoryBufferSet const& src) -> HistoryBufferSet
{
    HistoryBufferSet dest {
      .frameNo = src.frameNo,
      .requestNo = src.requestNo,
      .sensorTimestamp = src.sensorTimestamp
    };
    // share images (pool/runtime)
    auto addSharedImages = [&](auto const& from, auto& to) {
        for (auto const& sb : from) {
            auto pStreamInfo = const_cast<IImageStreamInfo*>(sb->getStreamInfo());
            auto const streamId = pStreamInfo->getStreamId();
            auto pConfigImageStream = queryConfigImageStream(streamId);
            if (pConfigImageStream->attribute == IFrameBufferManager::Attribute::HAL_IMAGE_POOL) {
                auto pPool = pConfigImageStream->pPool;
                if (auto newSb = pPool->shareSB(sb)) {
                    to.emplace_back(newSb);
                }
            }
        }
    };
    // copy metas
    auto addCopiedMetas = [](auto const& from, auto& to) {
        for (auto const& sb : from) {
            auto pStreamInfo = const_cast<IMetaStreamInfo*>(sb->getStreamInfo());
            auto const streamId = pStreamInfo->getStreamId();
            IMetadata* metadata = sb->tryReadLock(LOG_TAG);
            MY_LOGF_IF(metadata == nullptr, "fail to lock metaSB(%#" PRIx64 ")", streamId);
            IMetadata newMeta(*metadata);
            sb->unlock(LOG_TAG, metadata);
            if (auto newSb = Utils::HalMetaStreamBuffer::Allocator(pStreamInfo)(newMeta)) {
                 to.emplace_back(newSb);
            }
        }
    };
    //
    addSharedImages(src.vpHalImageStreamBuffers, dest.vpHalImageStreamBuffers);
    addCopiedMetas(src.vpHalMetaStreamBuffers, dest.vpHalMetaStreamBuffers);
    addCopiedMetas(src.vpAppMetaStreamBuffers, dest.vpAppMetaStreamBuffers);
    //
    MY_LOGD("cloned HBS=%s", toString(dest).c_str());
    return dest;
}


/******************************************************************************
 *
 ******************************************************************************/
const std::string ThisNamespace::mDebuggeeName{"NSCam::v3::NSPipelineContext::IFrameBufferManager"};

auto
ThisNamespace::
debug(
    android::Printer& printer,
    const std::vector<std::string>& options __unused
) -> void
{
    //@TODO
    dumpState(printer);
}
