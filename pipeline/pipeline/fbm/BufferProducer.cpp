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

#define LOG_TAG "mtkcam-fbm_producer"

#include "BufferProducer.h"
//
#include <sstream>

#include <mtkcam/utils/imgbuf/ImageBufferHeap.h>
#include <mtkcam3/pipeline/utils/streaminfo/ImageStreamInfo.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
//
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_CONTEXT);

using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::Utils;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::pipeline::NSPipelineContext;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGD1(...)               MY_LOGD_IF((mLogLevel>=1),__VA_ARGS__)
#define MY_LOGD2(...)               MY_LOGD_IF((mLogLevel>=2),__VA_ARGS__)
#define MY_LOGD3(...)               MY_LOGD_IF((mLogLevel>=3),__VA_ARGS__)
//
#if 0
#define FUNC_START     MY_LOGD("%p:+", this)
#define FUNC_END       MY_LOGD("%p:-", this)
#else
#define FUNC_START
#define FUNC_END
#endif


/******************************************************************************
 *
 ******************************************************************************/
BufferProducer::
BufferProducer(
    std::string poolName,
    sp<IImageStreamInfo> pStreamInfo,
    CallbackFuncT const& callbackFunc,
    android::sp<BufferProducer> pSharedPoolProducer
)
    : mLogLevel(::property_get_int32("vendor.debug.camera.log.pipeline.fbm.producer", 0))
    , mPoolName(poolName)
    , mpStreamInfo(pStreamInfo)
    , mpBufferPool(pSharedPoolProducer != nullptr ? pSharedPoolProducer->mpBufferPool : nullptr)
    , mCallbackFunc(callbackFunc)
{
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::onLastStrongRef(const void* /*id*/) -> void
{
    FUNC_START;
    clear();
    FUNC_END;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
toString() const -> std::string
{
    std::ostringstream oss;

    oss << "Producer("
        << this
        << " " << std::showbase << std::hex << mpStreamInfo->getStreamId()
        << " " << mpStreamInfo->getStreamName()
        << ")";

    oss << " ";
    oss << "Pool(" << mPoolName << ")";

    return oss.str();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
allocatePoolBuffersLocked(
    AllocFuncT& allocateFunc
) -> android::status_t
{
    CAM_TRACE_CALL();

    FUNC_START;
    //
    if (mpBufferPool == nullptr) {
        if (!allocateFunc) {
            MY_LOGE("%s: cannot get allocator", toString().c_str());
            return NO_MEMORY;
        }
        android::sp<BufferHeapPool> pPool = new BufferHeapPool(mPoolName, allocateFunc);
        if (pPool == nullptr) {
            MY_LOGE("%s: cannot create BufferHeapPool", toString().c_str());
            return NO_MEMORY;
        }
        mpBufferPool = pPool;
    }
    //
    status_t err = OK;
    err = mpBufferPool->allocateBuffers(mpStreamInfo->getStreamName(),
                                       mMaxNumberOfBuffers,
                                       mMinNumberOfInitialCommittedBuffers);

    if (err != OK) {
        MY_LOGE("Producer(%p) cannot allocate buffer from pool(%p)",
                this, mpBufferPool.get());
        return err;
    }
    FUNC_END;
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
#define CHECK_POOL(pPool)                                           \
    do {                                                            \
        if (pPool == nullptr) {                                     \
            MY_LOGE("%s: no BufferHeapPool", toString().c_str());   \
            return UNKNOWN_ERROR;                                   \
        }                                                           \
    } while (0)


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
initProducer(
    size_t maxNumberOfBuffers,
    size_t minNumberOfInitialCommittedBuffers,
    AllocFuncT&& allocateFunc
) -> status_t
{
    CAM_TRACE_NAME((std::string("initProducer-")+mPoolName).c_str());

    FUNC_START;

    {
        Mutex::Autolock _l(mBufMapLock);
        mMaxNumberOfBuffers = maxNumberOfBuffers;
    }
    setMinInitBufferCount(minNumberOfInitialCommittedBuffers);
    //
    status_t err = OK;
    {
        Mutex::Autolock _l(mLock);
        MY_LOGD1("inited(%s)", (mbInited == true) ? "TRUE" : "FALSE");
        if (!mbInited) {
            err = allocatePoolBuffersLocked(allocateFunc);
            mbInited = true;
        }
    }
    FUNC_END;
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
setMaxBufferCount(
    size_t maxNumberOfBuffers
) -> status_t
{
    FUNC_START;
    {
        Mutex::Autolock _l(mBufMapLock);
        mMaxNumberOfBuffers = maxNumberOfBuffers;
        if (mpBufferPool.get())
            mpBufferPool->updateBufferCount(maxNumberOfBuffers);
    }
    FUNC_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
setMinInitBufferCount(size_t count) -> void
{
    {
        Mutex::Autolock _l(mBufMapLock);
        mMinNumberOfInitialCommittedBuffers = count;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
setBufferHeapPool(
    sp<BufferHeapPool> pPool
) -> status_t
{
    FUNC_START;
    {
        Mutex::Autolock _l(mBufMapLock);
        if (mHeapToSbs.size()) {
            MY_LOGE("%s: in-use", toString().c_str());
            return UNKNOWN_ERROR;
        }
    }
    //
    {
        Mutex::Autolock _l(mLock);
        mpBufferPool = pPool;
        mbInited = false;
    }
    //
    FUNC_END;
    return OK;

}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
hasAcquiredMaxBuffer() -> bool
{
    FUNC_START;
    //
    Mutex::Autolock _l(mBufMapLock);
    MY_LOGD2("%s: in-use(%zu) / max(%zu)", toString().c_str(), mHeapToSbs.size(), mMaxNumberOfBuffers);
    Mutex::Autolock _ll(mLock);
    FUNC_END;
    return mpBufferPool->isPoolEmpty();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
clear() -> void
{
    FUNC_START;
    MY_LOGD("%s: clear pool(%p)->size(%zu)",
        toString().c_str(), mpBufferPool.get(), mHeapToSbs.size());
    //
    {
        Mutex::Autolock _l(mBufMapLock);
        for (auto& pr : mHeapToSbs) {
            auto& pHeap = pr.first;
            auto& vSb = pr.second;
            if (vSb.size()) {
                Mutex::Autolock _l(mLock);
                mpBufferPool->releaseToPool(mpStreamInfo->getStreamName(), pHeap);
            }
        }
        mBufferMap.clear();
        mHeapToSbs.clear();
    }
    FUNC_END;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
dump() -> void
{
    FUNC_START;
    {
        Mutex::Autolock _l(mBufMapLock);
        MY_LOGI("Producer(%p) stream(%#" PRIx64 ":%s) pool(%p) size(%zu) [Heap]{SB}",
                this, mpStreamInfo->getStreamId(), mpStreamInfo->getStreamName(),
                mpBufferPool.get(), mHeapToSbs.size());
        //
        for (auto const & pr : mHeapToSbs) {
            auto& pHeap = pr.first;
            auto& vSb = pr.second;
            std::ostringstream oss;
            for (auto const& sb : vSb) {
                oss << sb << " ";
            }
            MY_LOGI("[%p]{%s}", pHeap.get(), oss.str().c_str());
        }
    }
    //
    {
        Mutex::Autolock _l(mLock);
        mpBufferPool->dumpPool();
    }
    FUNC_END;
}


/******************************************************************************
 *  IStreamBufferPool
 ******************************************************************************/
auto
BufferProducer::
acquireFromPool(
    char const* szCallerName __unused,
    sp<IImageStreamBuffer>& rpStreamBuffer,
    nsecs_t nsTimeout
) -> status_t
{
    FUNC_START;
    auto pPool = [this](){ Mutex::Autolock _l(mLock); return mpBufferPool; }();
    sp<IImageBufferHeap> pHeap;
    {
        CHECK_POOL(pPool);
        auto startTime = std::chrono::system_clock::now();
        int retry_times = 1;
        while (OK != pPool->acquireFromPool(mpStreamInfo->getStreamName(), pHeap))
        {
            if (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - startTime) >
                std::chrono::nanoseconds(nsTimeout)) {  // timeout
                MY_LOGW("%s: cannot acquire from pool after timeout(%" PRId64 " ns).",
                        toString().c_str(), nsTimeout);
                return NO_MEMORY;
            }

            if (hasAcquiredMaxBuffer()) {
                bool isDumpLog = (mLogLevel > 0) || ((retry_times > 30) && (retry_times % 10 == 0));
                mCallbackFunc(mpStreamInfo->getStreamId(), isDumpLog);
                if (OK == pPool->acquireFromPool(mpStreamInfo->getStreamName(), pHeap)) {
                    MY_LOGD1("%s: acquire buffer after notify", toString().c_str());
                    break;
                }
            }

            if (Mutex::Autolock _l(mLock); OK != mCond.waitRelative(mLock, 10000000)) {  // wait signal or 10ms to retry
                MY_LOGD1("%s: retry #%d", toString().c_str(), retry_times);
                retry_times++;
            }
        }
    }

    rpStreamBuffer = new HalImageStreamBuffer(mpStreamInfo, this, pHeap);
    if (rpStreamBuffer == nullptr) {
        MY_LOGE("cannot new HalImageStreamBuffer. Something wrong...");
        return NO_MEMORY;
    }

    {
        Mutex::Autolock _l(mBufMapLock);
        mBufferMap.emplace(rpStreamBuffer.get(), pHeap);
        MY_LOGD2("acquire - buffer:%p pHeap:%p", rpStreamBuffer.get(), pHeap.get());
        mHeapToSbs[pHeap].emplace(rpStreamBuffer.get());
    }
    FUNC_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
releaseToPool(
    char const* szCallerName __unused,
    sp<IImageStreamBuffer> pStreamBuffer
) -> status_t
{
    FUNC_START;
    //
    size_t sharedHeapNum = 0;
    sp<IImageBufferHeap> rpHeap;
    {
        Mutex::Autolock _l(mBufMapLock);
        rpHeap = mBufferMap[pStreamBuffer.get()];
        mBufferMap.erase(pStreamBuffer.get());
        auto& vSb = mHeapToSbs[rpHeap];
        vSb.erase(pStreamBuffer.get());
        sharedHeapNum = vSb.size();
    }
    uint32_t bufStatus = pStreamBuffer->getStatus();
    MY_LOGD_IF((bufStatus & STREAM_BUFFER_STATUS::ERROR) || (mLogLevel >= 2),
               "release - buffer:%p bBufStatus:%#x pHeap:%p(sharedNum:%zu)",
               pStreamBuffer.get(), bufStatus, rpHeap.get(), sharedHeapNum);
    //
    if (!sharedHeapNum) {
        Mutex::Autolock _l(mLock);
        CHECK_POOL(mpBufferPool);
        mpBufferPool->releaseToPool(mpStreamInfo->getStreamName(), rpHeap);
        mCond.signal();
    }
    //
    FUNC_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferProducer::
shareSB(android::sp<IImageStreamBuffer> const& src __unused) -> android::sp<IImageStreamBuffer>
{
    Mutex::Autolock _l(mBufMapLock);
    auto bufMapIter = mBufferMap.find(src.get());
    if (bufMapIter != mBufferMap.end()) {
        auto pHeap = bufMapIter->second;
        auto h2sIter = mHeapToSbs.find(pHeap);
        if (h2sIter == mHeapToSbs.end()) {
            MY_LOGE("SB(%p:%s) - Heap not in mHeapToSbs", src.get(), src->getStreamInfo()->getStreamName());
            return nullptr;
        }
        sp<IImageStreamBuffer> dup = new HalImageStreamBuffer(mpStreamInfo, this, pHeap);
        h2sIter->second.emplace(dup.get());
        mBufferMap.emplace(dup.get(), pHeap);
        MY_LOGI("create shared SB(%p:%s) from pHeap:%p(sharedNum:%zu)",
                src.get(), src->getStreamInfo()->getStreamName(), pHeap.get(), h2sIter->second.size());
        return dup;
    }
    MY_LOGE("SB(%p:%s) not in mBufferMap", src.get(), src->getStreamInfo()->getStreamName());
    return nullptr;
}
