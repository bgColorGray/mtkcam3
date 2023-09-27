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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_BUFFERHANDLECACHE_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_BUFFERHANDLECACHE_H_
//
#include <IBufferHandleCacheMgr.h>
//
#include <unordered_map>
#include <unordered_set>
//
#include <mtkcam3/pipeline/utils/streambuf/StreamBuffers.h>
#include <mtkcam/utils/imgbuf/IGraphicImageBufferHeap.h>
//
// #include "AppStreamBuffers.h"
//

using namespace NSCam::v3::Utils;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace hidl_dev3 {
namespace Utils {

/**
 * Buffer Handle Cache
 */
class BufferHandleCache
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:

protected:
    struct  BufferHandle
    {
        uint64_t        bufferId = 0;
        buffer_handle_t bufferHandle = nullptr;
        std::shared_ptr<NSCam::v3::AppBufferHandleHolder> appBufferHandleHolder = nullptr;
        int32_t         ionFd = -1;
        struct timespec cachedTime;
    };

    // key: bufferId sent via HIDL interface
    // value: imported buffer_handle_t
    using BufferHandleMap = std::unordered_map<uint64_t, BufferHandle>;
    BufferHandleMap             mBufferHandleMap;
    std::string const           mInstanceName;
    int32_t                     mLogLevel = 0;
    StreamId_T                  mStreamId = -1;

    // A buffer id set, used to record the buffers which would be freed by others.
    // The set would be updated in BufferHandleCache::markBufferFreeByOthers,
    // and the buffers won't be freed in BufferHandleCache::clear.
    // ex: jpeg buffers when pre-release
    std::unordered_set<uint64_t>mBuffersFreedByOthers;

public:
                        ~BufferHandleCache();
                        BufferHandleCache(StreamId_T streamId, int32_t level);

    auto                dumpState(android::Printer& printer, uint32_t indent = 0) const -> void;

    auto                importBuffer(
                            uint64_t bufferId,
                            buffer_handle_t& importedBufferHandle,
                            std::shared_ptr<NSCam::v3::AppBufferHandleHolder>& appBufferHandleHolder
                        )   const -> int;

    auto                cacheBuffer(
                            uint64_t bufferId,
                            buffer_handle_t importedBufferHandle,
                            std::shared_ptr<NSCam::v3::AppBufferHandleHolder> appBufferHandleHolder
                        )   -> int;

    auto                uncacheBuffer(uint64_t bufferId) -> void;

    auto                markBufferFreeByOthers(uint64_t bufferId) -> void;

    auto                clear() -> void;

};


/**
 * Buffer Handle Cache Manager
 */
class BufferHandleCacheMgr
    : public IBufferHandleCacheMgr
{
public:
    ~BufferHandleCacheMgr();
    BufferHandleCacheMgr();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IBufferHandleCacheMgr Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.

    virtual auto        destroy() -> void override;

    virtual auto        removeBufferCache(
                            const hidl_vec<BufferCache>& cachesToRemove
                        )   -> void override;

    virtual auto        configBufferCacheMap(
                            const V3_5::StreamConfiguration& requestedConfiguration
                        )   -> void override;

    virtual auto        registerBufferCache(
                            const hidl_vec<V3_4::CaptureRequest>& requests,
                            std::vector<RequestBufferCache>& vBufferCache
                        )   -> void override;

    // virtual auto        markBufferFreeByOthers(
    //                         std::vector<NSCam::v3::CaptureResult>& results
    //                     )   -> void override;

    virtual auto    dumpState() -> void override;

private:

    auto                registerBuffer(
                            V3_2::StreamBuffer buf,
                            RequestBufferCache& cacheItem
                        )   -> void;

    std::string const                                   mInstanceName;
    int32_t                                             mLogLevel = 0;

    mutable android::Mutex                              mBufferHandleCacheMapLock;
    android::KeyedVector<StreamId_T, std::shared_ptr<BufferHandleCache>>
                                                        mBufferHandleCacheMap;
};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_BUFFERHANDLECACHE_H_