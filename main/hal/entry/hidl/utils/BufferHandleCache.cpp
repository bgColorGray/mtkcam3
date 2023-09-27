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

// #include <BufferHandleCache.h>
#include "BufferHandleCache.h"
#include <cutils/properties.h>
#include "MyUtils.h"
//

namespace globlBufferHandle {
    std::vector<buffer_handle_t> gfailedBufferHandle;
}
using namespace android;
using namespace NSCam;
using namespace NSCam::hidl_dev3;
using namespace NSCam::hidl_dev3::Utils;
using namespace globlBufferHandle;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_DEBUG(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_WARN(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_LOGV(...)                MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...)                MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...)                MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...)                MY_WARN (W, __VA_ARGS__)
#define MY_LOGE(...)                MY_ERROR(E, __VA_ARGS__)
#define MY_LOGA(...)                MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//
#if 0
#define FUNC_START     MY_LOGD("+")
#define FUNC_END       MY_LOGD("-")
#else
#define FUNC_START
#define FUNC_END
#endif

/******************************************************************************
 *
 ******************************************************************************/
NSCam::v3::AppBufferHandleHolder::
AppBufferHandleHolder(buffer_handle_t handle)
{
    bufferHandle = handle;
}

/******************************************************************************
 *
 ******************************************************************************/
NSCam::v3::AppBufferHandleHolder::
~AppBufferHandleHolder()
{
    if  ( bufferHandle != nullptr )
    {
        // if ( freeByOthers )
        // {
        //     ALOGI("skip clearing buffer handle %p, which must be a pre-released buffer", bufferHandle);
        // }
        // else
        // {
            ALOGD("[%s] free buffer handle: %p", __FUNCTION__, bufferHandle);
            if  ( auto helper = IGrallocHelper::singleton() ) {
                int ret = helper->freeBuffer(bufferHandle);
                //free fail
                if(ret != 0)
                {
                    ALOGE("[%s] free buffer(%p) failed with %d", __FUNCTION__, bufferHandle, ret);
                    gfailedBufferHandle.push_back(bufferHandle);
                }
                for(size_t i = 0; i < gfailedBufferHandle.size();)
                {
                    ALOGD("[%s] i = %zu, gfailedBufferHandle.size() = %zu",__FUNCTION__,  i, gfailedBufferHandle.size());
                    int ret = helper->freeBuffer(gfailedBufferHandle[i]);
                    //free sucessfully
                    if(ret == 0)
                    {
                        ALOGD("[%s] previous free failed buffer(%p), now free sucessfully",__FUNCTION__,  gfailedBufferHandle[i]);
                        gfailedBufferHandle[i] = nullptr;
                        gfailedBufferHandle.erase(gfailedBufferHandle.begin() + i);
                    }
                    else{
                        ALOGE("[%s] free buffer(%p) still failed with %d", __FUNCTION__, gfailedBufferHandle[i], ret);
                        i++;
                    }
                }
            }
        // }
    }
    bufferHandle = nullptr;
}

/******************************************************************************
 * BufferHandleCacheMgr
 ******************************************************************************/
auto
IBufferHandleCacheMgr::
create() -> IBufferHandleCacheMgr*
{
    auto pInstance = new BufferHandleCacheMgr();
    if  ( ! pInstance ) {
        delete pInstance;
        return nullptr;
    }
    return pInstance;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCacheMgr::
destroy() -> void
{
    MY_LOGD("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCacheMgr::
configBufferCacheMap(
    const V3_5::StreamConfiguration& requestedConfiguration
) -> void
{
    Mutex::Autolock _l(mBufferHandleCacheMapLock);
    std::unordered_set<StreamId_T> usedStreamIds;
    usedStreamIds.reserve(requestedConfiguration.v3_4.streams.size());
    // add bufferCache
    for ( size_t i = 0; i < requestedConfiguration.v3_4.streams.size(); ++i ) {
        StreamId_T streamId = requestedConfiguration.v3_4.streams[i].v3_2.id;
        ssize_t const index = mBufferHandleCacheMap.indexOfKey(streamId);
        if ( index < 0) {
            std::shared_ptr<BufferHandleCache> pBufferHandleCache = std::make_shared<BufferHandleCache>(streamId, mLogLevel);
            mBufferHandleCacheMap.add(streamId, pBufferHandleCache);
        }
        else {
            MY_LOGD("streamId:%#x, already exists in mBufferHandleCacheMap, keep cache", streamId);
        }
        usedStreamIds.insert(streamId);
    }
    // remove unused bufferCache
    for (ssize_t i = mBufferHandleCacheMap.size() - 1; i >= 0; i--)
    {
        auto const streamId = mBufferHandleCacheMap.keyAt(i);
        auto const it = usedStreamIds.find(streamId);
        if ( it == usedStreamIds.end() ) {
            //  remove unsued stream
            MY_LOGD("remove unused BufferHandleCache, streamId:%#x", streamId);
            mBufferHandleCacheMap.removeItemsAt(i);
        }
    }
    MY_LOGD("mBufferHandleCacheMap.size()=%zu", mBufferHandleCacheMap.size());
}

/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCacheMgr::
removeBufferCache(const hidl_vec<BufferCache>& cachesToRemove) -> void
{
    Mutex::Autolock _l(mBufferHandleCacheMapLock);
    for ( auto& cache : cachesToRemove ) {
        ssize_t const index = mBufferHandleCacheMap.indexOfKey(cache.streamId);
        if  ( 0 > index ) {
            MY_LOGE("bad streamId:%#x: bufferId:%" PRIu64, cache.streamId, cache.bufferId);
            return;
        }

        if ( auto pBufferHandleCache = mBufferHandleCacheMap.valueAt(index) ) {
            pBufferHandleCache->uncacheBuffer(cache.bufferId);
        }
        else {
            MY_LOGE("bad streamId:%#x has no buffer handle cache - bufferId:%" PRIu64, cache.streamId, cache.bufferId);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCacheMgr::
registerBufferCache(
    const hidl_vec<V3_4::CaptureRequest>& requests,
    std::vector<RequestBufferCache>& vBufferCache
) -> void
{
    Mutex::Autolock _l(mBufferHandleCacheMapLock);
    for ( auto& req : requests ) {
        RequestBufferCache cacheItem{
            .frameNumber = req.v3_2.frameNumber
        };
        // input buffer
        if ( req.v3_2.inputBuffer.streamId != -1 &&
             req.v3_2.inputBuffer.bufferId != 0 ) {
            registerBuffer(req.v3_2.inputBuffer, cacheItem);
        }
        // ouptut buffers
        for ( auto& buf : req.v3_2.outputBuffers ){
            registerBuffer(buf, cacheItem);
        }
        vBufferCache.push_back(cacheItem);
    }
}

void
BufferHandleCacheMgr::
registerBuffer(V3_2::StreamBuffer buf, RequestBufferCache& cacheItem)
{
    ssize_t const index = mBufferHandleCacheMap.indexOfKey(buf.streamId);
    if  ( 0 <= index ) {
        MY_LOGD_IF(mLogLevel >= 1, "buffer handle already cached, streamId:%#" PRIx32 ": bufferId:%" PRIu64, buf.streamId, buf.bufferId);
    }
    else
    {
        MY_LOGE("bad streamId:%#x: bufferId:%" PRIu64, buf.streamId, buf.bufferId);
        return;
    }

    BufferHandle bufferHandle;
    bufferHandle.bufferId = buf.bufferId;

    auto pBufferHandleCache = mBufferHandleCacheMap.valueAt(index);

    // query buffer handle from gralloc (import)
    if ( pBufferHandleCache ){
        // import buffer from gralloc
        bufferHandle.bufferHandle = buf.buffer;
        MY_LOGD_IF(mLogLevel >= 1, "From HIDL bufferId:%" PRIu64 ", %p", buf.bufferId, bufferHandle.bufferHandle);
        pBufferHandleCache->importBuffer(buf.bufferId, bufferHandle.bufferHandle, bufferHandle.appBufferHandleHolder);
        MY_LOGD_IF(mLogLevel >= 1, "From GPU  bufferId:%" PRIu64 ", %p", buf.bufferId, bufferHandle.bufferHandle);
        // cache buffer handle
        if ( bufferHandle.appBufferHandleHolder == nullptr ) {
            bufferHandle.appBufferHandleHolder = std::make_shared<NSCam::v3::AppBufferHandleHolder>(bufferHandle.bufferHandle);
        }
        pBufferHandleCache->cacheBuffer(buf.bufferId, bufferHandle.bufferHandle, bufferHandle.appBufferHandleHolder);
    }
    else {
        MY_LOGE("bad streamId:%#x has no buffer handle cache - bufferId:%" PRIu64, buf.streamId, buf.bufferId);
    }
    cacheItem.bufferHandleMap.add(buf.streamId, bufferHandle);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCacheMgr::
dumpState() -> void
{
    Mutex::Autolock _l(mBufferHandleCacheMapLock);
    NSCam::Utils::ULog::ULogPrinter p(NSCam::Utils::ULog::MOD_UTILITY, LOG_TAG);
    for ( size_t i = 0; i < mBufferHandleCacheMap.size(); i++ ) {
        const auto& pBufferHandleCache = mBufferHandleCacheMap.valueAt(i);
        if ( pBufferHandleCache ) {
            pBufferHandleCache->dumpState(p, 6);
        }
        else{
            MY_LOGE("has no buffer handle cache");
        }
    }
}


/******************************************************************************
 * BufferHandleCache
 ******************************************************************************/
BufferHandleCacheMgr::
BufferHandleCacheMgr()
    : mInstanceName{"-BufferHandleCacheMgr"}
    , mLogLevel(property_get_int32("vendor.camera.debug.bufferHandleCacheMgr", 0))
{
}


/******************************************************************************
 *
 ******************************************************************************/
BufferHandleCacheMgr::
~BufferHandleCacheMgr()
{
    MY_LOGD("dtor");
}

/******************************************************************************
 *
 ******************************************************************************/
static
auto
queryIonFd(buffer_handle_t bufferHandle) -> int
{
    mcam::GrallocStaticInfo staticInfo;
    GrallocDynamicInfo dynamicInfo;
    auto helper = IGrallocHelper::singleton();
    if  ( ! helper || 0 != helper->query(bufferHandle, &staticInfo, &dynamicInfo) || dynamicInfo.ionFds.empty() )
    {
        return -1;
    }

    return dynamicInfo.ionFds[0];
}


/******************************************************************************
 *
 ******************************************************************************/
static
auto
mapBufferHandle(buffer_handle_t& bufferHandle) -> void
{
    auto helper = IGrallocHelper::singleton();
    if  ( ! helper || ! helper->importBuffer(bufferHandle) || bufferHandle == nullptr )
    {
        bufferHandle = nullptr;
    }
}


/******************************************************************************
 * BufferHandleCache
 ******************************************************************************/
BufferHandleCache::
BufferHandleCache(StreamId_T streamId, int32_t level)
    : mInstanceName{"-BufferHandleCache"}
    , mLogLevel(level)
    , mStreamId(streamId)
{
}


/******************************************************************************
 *
 ******************************************************************************/
BufferHandleCache::
~BufferHandleCache()
{
    clear();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCache::
dumpState(android::Printer& printer, uint32_t indent) const -> void
{
    for (auto const& pair : mBufferHandleMap) {
        printer.printFormatLine("%-*cbufferId:%3" PRIu64 " -> handle:%p %3d ionFd:%3d  (%s)",
            indent, ' ', pair.second.bufferId, pair.second.bufferHandle, pair.second.bufferHandle->data[0], pair.second.ionFd,
            NSCam::Utils::LogTool::get()->convertToFormattedLogTime(&pair.second.cachedTime).c_str()
        );
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCache::
importBuffer(
    uint64_t bufferId,
    buffer_handle_t& importedBufferHandle,
    std::shared_ptr<NSCam::v3::AppBufferHandleHolder>& appBufferHandleHolder
)   const -> int
{
    FUNC_START;
    auto it = mBufferHandleMap.find(bufferId);
    if ( it == mBufferHandleMap.end() ) {
        if ( importedBufferHandle == nullptr ) {
            MY_LOGE("bufferId %" PRIu64 " has null buffer handle!", bufferId);
            return -EINVAL;
        }

        mapBufferHandle(importedBufferHandle);
        if (importedBufferHandle == nullptr) {
          MY_LOGE("bufferId %" PRIu64 ": fail to import from gralloc", bufferId);
          return -ENODEV;
        }
        MY_LOGD("bufferId %" PRIu64 ": %p %3d - 1st imported", bufferId, importedBufferHandle, importedBufferHandle->data[0]);
    }
    else if ( importedBufferHandle != nullptr ) {
        MY_LOGE("bufferId %" PRIu64 ": has a cached buffer handle:%p != %p",
            bufferId, it->second.bufferHandle, importedBufferHandle);
        return -ENODEV;
    }
    else {
        importedBufferHandle = it->second.bufferHandle;
        appBufferHandleHolder = it->second.appBufferHandleHolder;
        MY_LOGD_IF(mLogLevel >= 1, "bufferId %" PRIu64 ": %p %3d - from cache", bufferId, importedBufferHandle, importedBufferHandle->data[0]);
    }
    //
    if ( importedBufferHandle == nullptr ) {
        MY_LOGE("bufferId %" PRIu64 ": fail to import", bufferId);
        return -ENODEV;
    }
    // MY_LOGD("bufferId %" PRIu64 ": %p", bufferId, importedBufferHandle);
    //
    FUNC_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCache::
cacheBuffer(
    uint64_t bufferId,
    buffer_handle_t importedBufferHandle,
    std::shared_ptr<NSCam::v3::AppBufferHandleHolder> appBufferHandleHolder
)   -> int
{
    FUNC_START;
    auto it = mBufferHandleMap.find(bufferId);
    if ( it == mBufferHandleMap.end() ) {
        mBufferHandleMap[bufferId] = {
            .bufferId = bufferId,
            .bufferHandle = importedBufferHandle,
            .appBufferHandleHolder = appBufferHandleHolder,
            .ionFd = queryIonFd(importedBufferHandle),
        };
        NSCam::Utils::LogTool::get()->getCurrentLogTime(&mBufferHandleMap[bufferId].cachedTime);
    }
    else if (it->second.bufferHandle != importedBufferHandle) {
        MY_LOGE("bufferId %" PRIu64 ": has a cached buffer handle:%p != %p",
            bufferId, it->second.bufferHandle, importedBufferHandle);
        return -ENODEV;
    }
    else if ((it->second.appBufferHandleHolder)->bufferHandle != appBufferHandleHolder->bufferHandle){
        MY_LOGE("bufferId %" PRIu64 "has different AppStreamBufferHandles:%p != %p",
        bufferId, (it->second.appBufferHandleHolder)->bufferHandle, appBufferHandleHolder->bufferHandle);
        return -ENODEV;
    }
    MY_LOGD_IF(mLogLevel >= 1,
               "bufferId %-3" PRIu64 " -> %p %3d",
               bufferId, importedBufferHandle, importedBufferHandle->data[0]);
    //
    FUNC_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCache::
uncacheBuffer(uint64_t bufferId) -> void
{
    FUNC_START;
    auto it = mBufferHandleMap.find(bufferId);
    if ( it == mBufferHandleMap.end() ) {
        MY_LOGW("bufferId:%" PRIu64 ": has not been cached!", bufferId);
    }
    else {
        MY_LOGD("bufferId %-3" PRIu64 " -> %p %3d", bufferId, it->second.bufferHandle, it->second.bufferHandle->data[0]);
        it->second.bufferHandle = nullptr;
        mBufferHandleMap.erase(it);
    }
    FUNC_END;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
BufferHandleCache::
clear() -> void
{
    FUNC_START;
    MY_LOGD("streamId:%#x + mBufferHandleMap.size#:%zu", mStreamId, mBufferHandleMap.size());
    for (auto& pair : mBufferHandleMap) {
        if (pair.second.appBufferHandleHolder.use_count() > 1){
            MY_LOGW("Buffer won't be freed after clearing! bufferId: %" PRIu64 ", buffer: %p, fd: %d, count: %ld", pair.first, pair.second.bufferHandle, pair.second.ionFd, pair.second.appBufferHandleHolder.use_count());
        }
        pair.second.bufferHandle = nullptr;
    }
    mBufferHandleMap.clear();
    FUNC_END;
}

