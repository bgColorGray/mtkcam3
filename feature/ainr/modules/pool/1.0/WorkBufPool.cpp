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
#define LOG_TAG "AinrCore/WorkBufPool"

#include "WorkBufPool.h"

#include <mtkcam3/feature/utils/ImageBufferUtils.h>
#include <mtkcam3/feature/ainr/AinrUlog.h>


using namespace android;
using namespace ainr;
using namespace NSCam;

std::shared_ptr<IWorkBufPool> IWorkBufPool::createInstance(
        std::shared_ptr<NSCam::IIonDevice> spIonDevice,
        MBOOL useCapFeaturePool) {
    auto pool
            = std::make_shared<WorkBufPool>(spIonDevice, useCapFeaturePool);
    return pool;
}

WorkBufPool::WorkBufPool(std::shared_ptr<IIonDevice> spIonDevice,
        MBOOL useCapFeaturePool)
    : mIonDevice(spIonDevice)
    , mUnderAlloc(false) {
    if (useCapFeaturePool == MTRUE)
        mCapBufPool = ICaptureFeatureBufferPool::getInstance("ainr");
    else
        mCapBufPool = nullptr;
}

WorkBufPool::~WorkBufPool(void) {
    CAM_ULOGM_FUNCLIFE_ALWAYS();

    std::unique_lock<std::mutex> _lk(mMx);

    // We should not destruct during buffer allocation
    if (mUnderAlloc) {
        ainrLogI("wait for allocation finished");
        mCondAlloc.wait(_lk, [this](){return !mUnderAlloc;});
    }

    if (CC_UNLIKELY(!mInUsedBuffers.empty())) {
        ainrLogF("There are some buffers being used and not in pool!!!");
        return;
    }

    for (auto &buf : mAvailableBuffers) {
        buf->free();
    }
}

enum AinrErr WorkBufPool::allocBuffers(const BufferInfo &info,
        bool isAsync, int32_t bufferCount) {
    ainrLogD("Width(%d), height(%d), format(0x%x) aligned(%d)",
        info.width, info.height, info.format, info.aligned);

    {
        std::lock_guard<std::mutex> _lk(mMx);
        mUnderAlloc = true;
    }

    /* allocate buffer */
    for (int i = 0; i < bufferCount; i++) {
        std::shared_ptr<WorkBuf> workBuf = nullptr;

        if (mCapBufPool) {
            workBuf = WorkBuf::alloc(info, mCapBufPool, mIonDevice);
        } else {
            workBuf = WorkBuf::alloc(info, mIonDevice);
        }

        {
            std::lock_guard<std::mutex> _lk(mMx);
            mAvailableBuffers.push_back(workBuf);
            mCond.notify_one();
        }
    }

    {
        std::lock_guard<std::mutex> _lk(mMx);
        mUnderAlloc = false;
        mCondAlloc.notify_one();
    }


    // TODO: Implement async version

    return AinrErr_Ok;
}

SpImgBuffer WorkBufPool::acquireBufferFromPool() {
    std::unique_lock<std::mutex> _lk(mMx);
    if (mAvailableBuffers.empty()) {
        CAM_ULOGM_TAGLIFE("Wait available buffers");
        ainrLogD("Wait available buffers");
        mCond.wait(_lk, [this](){return !mAvailableBuffers.empty();});
        ainrLogD("Wait available buffers done");
    }

    SpWorkBuffer buf = mAvailableBuffers.back();
    mAvailableBuffers.pop_back();
    mInUsedBuffers.push_back(buf);

    return buf->buf();
}

void WorkBufPool::releaseBufferToPool(SpImgBuffer &buf) {
    std::lock_guard<std::mutex> _lk(mMx);
    for (auto itr = mInUsedBuffers.begin();
            itr != mInUsedBuffers.end(); itr++) {
        if ((*itr)->buf().get() == buf.get()) {
            mAvailableBuffers.push_back(*itr);
            mInUsedBuffers.erase(itr);
            break;
        }
    }
    mCond.notify_one();
    return;
}

void WorkBufPool::releaseAllBuffersToPool() {
    std::lock_guard<std::mutex> _lk(mMx);
    for (auto &buf : mInUsedBuffers) {
        mAvailableBuffers.push_back(buf);
    }
    mInUsedBuffers.clear();
    mCond.notify_all();
    return;
}

void WorkBuf::free(void)
{
    if (mSrc == kAlloc) {
        NSCam::ImageBufferUtils::getInstance().deallocBuffer(mBuf.get());
    } else {
        mCapBufPool->putBuffer(mCapBuf);
    }
}

/* create buffer from pool or alloc by ourself */
std::shared_ptr<WorkBuf> WorkBuf::alloc(const IWorkBufPool::BufferInfo &info,
    std::shared_ptr<NSCam::ICaptureFeatureBufferPool> pool,
    std::shared_ptr<NSCam::IIonDevice> ionDevice)
{
    /* param for capture pool */
    struct NSCam::ICaptureFeatureBuffer::BufferInfo capPoolInfo(
            info.width, info.height, info.format);
    if (info.isPixelAligned) {
        capPoolInfo.align_pixel = info.aligned;
    } else {
        capPoolInfo.align_byte = info.aligned;
    }

    /* try to get buffer from pool */
    std::shared_ptr<WorkBuf> workBuf = nullptr;
    std::shared_ptr<ICaptureFeatureBuffer> capBuf =
        pool->getBuffer(capPoolInfo);

    if (capBuf) {
        workBuf = std::make_shared<WorkBuf>(kCapPool, capBuf->ptr(),
                                    capBuf, pool);
    } else {
        workBuf = alloc(info, ionDevice);
    }

    return workBuf;
}

std::shared_ptr<WorkBuf> WorkBuf::alloc(const IWorkBufPool::BufferInfo &info,
    std::shared_ptr<NSCam::IIonDevice> ionDevice)
{
    /* param for ImageBufferUtils*/
    ImageBufferUtils::BufferInfo bufInfo(
            info.width, info.height, info.format);
    bufInfo.aligned = info.aligned;
    bufInfo.isPixelAligned = info.isPixelAligned;

    SpImgBuffer buf = nullptr;
    enum Source src = kAlloc;

    auto ret = ImageBufferUtils::getInstance().allocBuffer(
            bufInfo, buf, ionDevice);
    if (CC_UNLIKELY(ret == MFALSE)) {
        ainrLogF("allocate buffer error!!!");
        return nullptr;
    }

    return std::make_shared<WorkBuf>(kAlloc, buf, nullptr, nullptr);
}
