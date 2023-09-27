/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2020. All rights reserved.
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
#ifndef __WORKBUFPOOL_H__
#define __WORKBUFPOOL_H__

// AINR Core Lib
#include <mtkcam3/feature/ainr/IWorkBufPool.h>

#include <mtkcam/def/UITypes.h>  //NSCam::MSize
#include <mtkcam/utils/imgbuf/IIonDevice.h>

#include <mtkcam3/feature/utils/ImageBufferUtils.h>
#include <mtkcam3/feature/utils/ICaptureFeatureBufferPool.h>

// STL
#include <memory>  //std::shared_ptr
#include <condition_variable>
#include <thread>
#include <future>
#include <mutex>
#include <vector>

namespace ainr {

class WorkBuf {
public:
    enum Source {
        kAlloc,
        kCapPool,
    };

    /* alloc from capture feature pool */
    static std::shared_ptr<WorkBuf> alloc(const IWorkBufPool::BufferInfo &info,
            std::shared_ptr<NSCam::ICaptureFeatureBufferPool> pool,
            std::shared_ptr<NSCam::IIonDevice> ionDevice);

    /* alloc from ImageBufferUtils */
    static std::shared_ptr<WorkBuf> alloc(const IWorkBufPool::BufferInfo &info,
            std::shared_ptr<NSCam::IIonDevice> ionDevice);

    void free(void);

    inline SpImgBuffer buf(void) {return mBuf;}

    WorkBuf(enum Source src, SpImgBuffer buf,
            std::shared_ptr<NSCam::ICaptureFeatureBuffer> capBuf,
            std::shared_ptr<NSCam::ICaptureFeatureBufferPool> capBufPool)
    : mSrc(src)
    , mBuf(buf)
    , mCapBuf(capBuf)
    , mCapBufPool(capBufPool)
    {}

    ~WorkBuf() {
        mBuf = nullptr;
        mCapBuf = nullptr;
        mCapBufPool = nullptr;
    }

private:
    enum Source mSrc;
    SpImgBuffer mBuf;
    std::shared_ptr<NSCam::ICaptureFeatureBuffer> mCapBuf;
    std::shared_ptr<NSCam::ICaptureFeatureBufferPool> mCapBufPool;
};

class WorkBufPool : public IWorkBufPool {

typedef std::shared_ptr<WorkBuf> SpWorkBuffer;

public:
    WorkBufPool(std::shared_ptr<NSCam::IIonDevice> spIonDevice = nullptr,
            MBOOL useCapFeaturePool = MFALSE);

/* implementation from IWorkBufPool */
public:
    virtual ~WorkBufPool(void);
    /* Allocate working buffers
     *  @param isAsync                  Allocate buffers asynchronously or not.
     *  @param bufferCount              Allocate buffers' count.
     *  @return                         Return AinrErr_Ok if all buffers being allocated.
     */
    enum AinrErr allocBuffers(const BufferInfo &info, bool isAsync, int32_t bufferCount);

    /* Acquire image buffer from pool.
     * Thread safe method.
     *  @param params       AiParam indicate basic buffer information.
     *  @return bufPackage   Input/Output buffers.
     */
    SpImgBuffer acquireBufferFromPool();

    /* Release image buffer into pool.
     *  @param buf          Image buffer which you want to release it.
     *  @return             Return AinrErr_Ok if the buffer be released.
     */
    void releaseBufferToPool(SpImgBuffer &buf);

    /* Release all acquired buffers back to pool.
     */
    void releaseAllBuffersToPool();
private:
    //BufferInfo mBufferInfo;
    std::shared_ptr<NSCam::IIonDevice> mIonDevice;
    std::vector<SpWorkBuffer> mAvailableBuffers;
    std::vector<SpWorkBuffer> mInUsedBuffers;
    mutable std::mutex mMx;
    std::condition_variable mCond;

    // Prevent destruct WorkBufPool when buffer allocating
    std::condition_variable mCondAlloc;
    volatile bool mUnderAlloc;

    std::shared_ptr<NSCam::ICaptureFeatureBufferPool> mCapBufPool;
};
}; /* namespace ainr */
#endif  //__WORKBUFPOOL_H__
