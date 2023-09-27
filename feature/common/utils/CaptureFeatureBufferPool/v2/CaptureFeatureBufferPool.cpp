/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
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

#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
#include <mtkcam/utils/imgbuf/IDummyImageBufferHeap.h>

#include <cutils/properties.h>
#include <utils/String8.h>

#include <algorithm>

#include "CaptureFeatureBufferPool.h"

#include <mtkcam/drv/IHalSensor.h>

#include <unistd.h>

// using
using NSCam::ICaptureFeatureBuffer;
using NSCam::ICaptureFeatureBufferPool;
using NSCam::CaptureFeatureBuffer;
using NSCam::CaptureFeatureBufferPool;
using NSCam::CaptureFeatureBufferPoolCore;
using NSCam::IImageBuffer;
using NSCam::IIonImageBufferHeap;
using NSCam::IDummyImageBufferHeap;
using namespace NSCam::Utils::Format;
using namespace NSCam::Utils::ULog;

// typedef
typedef std::shared_ptr<CaptureFeatureBufferPoolCore> PoolCorePtr;
typedef ICaptureFeatureBuffer::BufferInfo BufferInfoT;
typedef std::shared_ptr<CaptureFeatureBuffer> BufferPtr;
typedef std::shared_ptr<ICaptureFeatureBuffer> IBufferPtr;

// log
#define LOG_TAG "CaptureFeatureBufferPool2"
CAM_ULOG_DECLARE_MODULE_ID(MOD_UTILITY);

#define MY_LOGD(fmt, arg...)        CAM_ULOGMD(fmt, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI(fmt, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME(fmt, ##arg)

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
static const unsigned int AEE_DB_FLAGS = DB_OPT_NE_JBT_TRACES |
                                         DB_OPT_PROCESS_COREDUMP |
                                         DB_OPT_PROC_MEM |
                                         DB_OPT_PID_SMAPS |
                                         DB_OPT_LOW_MEMORY_KILLER |
                                         DB_OPT_DUMPSYS_PROCSTATS |
                                         DB_OPT_FTRACE;

#define MY_LOGA(fmt, arg...) \
    do { \
        android::String8 const str = android::String8::format(fmt, ##arg); \
        MY_LOGE("ASSERT(%s)", str.c_str()); \
        aee_system_exception(LOG_TAG, NULL, AEE_DB_FLAGS, str.c_str()); \
    } while(0)
#else
#define MY_LOGA(fmt, arg...) \
    do { \
        android::String8 const str = android::String8::format(fmt, ##arg); \
        MY_LOGE("ASSERT(%s)", str.c_str()); \
    } while(0)
#endif

// MACRO
#define BUFFER_USAGE (eBUFFER_USAGE_SW_READ_OFTEN | \
                      eBUFFER_USAGE_SW_WRITE_OFTEN | \
                      eBUFFER_USAGE_HW_CAMERA_READWRITE)
#define ALIGN_CEIL(x,a)     (((x) + (a) - 1L) & ~((a) - 1L))

// --------------------- CaptureFeatureBuffer -----------------------//
CaptureFeatureBuffer::CaptureFeatureBuffer(
    sp<IIonImageBufferHeap> heap, MUINT32 size)
    : state_(kAlive)
    , src_(kPool)
    , size_(size)
    , heap_(heap)
    , buf_(nullptr)
    , info_(BufferInfoT(0, 0, 0))
{
}

CaptureFeatureBuffer::~CaptureFeatureBuffer()
{
    abandon();
}

MBOOL CaptureFeatureBuffer::prepare(BufferInfoT info)
{
    std::lock_guard<std::mutex> _l(lock_);

    // check state and heap
    if (state_ == kAbandoned) {
        MY_LOGE("buffer abandoned");
        return MFALSE;
    }

    if (heap_ == nullptr) {
        MY_LOGE("heap is nullptr");
        return MFALSE;
    }

    // reset buffer and info
    if (buf_ != nullptr) {
        buf_->unlockBuf("CapPool");
        buf_ = nullptr;
    }
    memset(&info_, 0, sizeof(BufferInfoT));

    // prepare param and check size
    MUINT32 buf_size = 0, align_w, plane_cnt;
    MUINT32 stride[3] = {0};
    MUINTPTR vaddr[3] = {0};
    MUINTPTR paddr[3] = {0};

    // do pixel aligned
    align_w = ALIGN_CEIL(info.w, info.align_pixel);
    plane_cnt = queryPlaneCount(info.fmt);

    for (auto i = 0; i < plane_cnt; i++) {
        // calculate stride in byte
        stride[i] =
            (queryPlaneWidthInPixels(info.fmt, i, align_w) *
            queryPlaneBitsPerPixel(info.fmt, i) + 7) / 8;

        // do byte alignment
        stride[i] = ALIGN_CEIL(stride[i], info.align_byte);

        // calcuate addr & buf size
        vaddr[i] = heap_->getBufVA(0) + buf_size;
        paddr[i] = heap_->getBufPA(0) + buf_size;
        buf_size += stride[i] * queryPlaneHeightInPixels(info.fmt, i, info.h);
    }

    MY_LOGI("%ux%u %u,%u %u", info.w, info.h, info.align_pixel, info.align_byte, stride[0]);

    if (buf_size > size_) {
        MY_LOGD("request:%u>size:%u; fmt:%u size:%ux%u align:%u,%d",
                buf_size, size_, info.fmt, info.w, info.h,
                info.align_pixel, info.align_byte);
        return MFALSE;
    }

    // prepare param to create image buffer
    MINT32 boundary[3] = {0}; // un-used
    IImageBufferAllocator::ImgParam param(
        info.fmt, MSize(info.w, info.h), stride, boundary, plane_cnt);
    struct PortBufInfo_dummy portInfo(
            heap_->getHeapID(), vaddr, paddr, plane_cnt);
    portInfo.offset = 0;

    // create dummy heap & image buffer
    sp<IDummyImageBufferHeap> heap =
        IDummyImageBufferHeap::create(LOG_TAG, param, portInfo);
    buf_ = heap->createImageBuffer();
    if (buf_ == nullptr) {
        MY_LOGE("create buffer fail...");
        return MFALSE;
    }

    buf_->lockBuf("CapPool");

    // update BufferInfoT
    info_ = info;

    return MTRUE;
}

void CaptureFeatureBuffer::abandon()
{
    std::lock_guard<std::mutex> _l(lock_);

    if (buf_ != nullptr) {
        buf_->unlockBuf("CapPool");
        buf_ = nullptr;
    }

    if (heap_ != nullptr) {
        heap_->unlockBuf("CapPool");
        heap_ = nullptr;
    }

    state_ = kAbandoned;
}

// --------------------- CaptureFeatureBufferPool ----------------------- //
std::shared_ptr<ICaptureFeatureBufferPool>
ICaptureFeatureBufferPool::getInstance(char* name)
{
    return std::make_shared<CaptureFeatureBufferPool>(name);
}

CaptureFeatureBufferPool::CaptureFeatureBufferPool(char* name)
    : name_(name)
{
    core_ = CaptureFeatureBufferPoolCore::getInstance();
    core_->attach(this);

}

CaptureFeatureBufferPool::~CaptureFeatureBufferPool()
{

    if (used_buffer_.size() > 0) {
        MY_LOGA("%s pool has %u buffers when destroy",
                name_.c_str(), used_buffer_.size());
    }

    core_->detach(this);
    core_ = nullptr;
}

MUINT32 CaptureFeatureBufferPool::buf_size() const
{
    return core_->buf_size();
}

IBufferPtr
CaptureFeatureBufferPool::getBuffer(BufferInfoT info)
{
    BufferPtr buffer = core_->getBuffer(info);

    if (buffer != nullptr) {
        std::lock_guard<std::mutex> _l(lock_);
        used_buffer_.push_back(buffer);
        MY_LOGD("get buffer by %s", name_.c_str());
    }

    return buffer;
}

MBOOL
CaptureFeatureBufferPool::putBuffer(IBufferPtr buffer)
{
    if (buffer == nullptr) {
        MY_LOGE("buffer is nullptr");
        return MFALSE;
    }

    std::lock_guard<std::mutex> _l(lock_);
    // check if buffer in used_buffer_
    for (int i = 0; i < used_buffer_.size(); i++) {
        if (dynamic_cast<ICaptureFeatureBuffer*>(used_buffer_[i].get()) ==
                                                buffer.get()) {
            // put back the buffer into core & erase from pool
            core_->putBuffer(used_buffer_[i]);
            used_buffer_.erase(used_buffer_.begin() + i);
            return MTRUE;
        }
    }

    MY_LOGA("%s cannot find buffer in used_buffer_", name_.c_str());

    return MFALSE;
}

// ------------------- CaptureFeatureBufferPoolCore --------------------- //
PoolCorePtr
CaptureFeatureBufferPoolCore::getInstance()
{
    static PoolCorePtr core = nullptr;
    static std::mutex lock;

    std::lock_guard<std::mutex> _l(lock);
    if (core == nullptr)
        core = std::make_shared<CaptureFeatureBufferPoolCore>();

    return core;
}

MBOOL CaptureFeatureBufferPoolCore::attach(CaptureFeatureBufferPool* pool)
{
    std::lock_guard<std::mutex> _l(lock_);

    // add the pool from core
    attach_pool_.push_back(pool);

    // First pool attached in core, do init
    if (attach_pool_.size() == 1) {
        MY_LOGD("add init task into jobQueue");
        state_ = kInit;
        // debounce for quick leave/enter camera case
        cv_.notify_one();
        jobQueue_->addJob(
            std::bind(&CaptureFeatureBufferPoolCore::init, this));
    }

    return MTRUE;
}

MBOOL CaptureFeatureBufferPoolCore::detach(CaptureFeatureBufferPool* pool)
{
    std::lock_guard<std::mutex> _l(lock_);

    auto f = find(attach_pool_.begin(), attach_pool_.end(), pool);
    if (f == attach_pool_.end()) {
        MY_LOGA("cannot find pool");
        return MFALSE;
    }

    // remove the pool from core
    attach_pool_.erase(f);

    // No pool attached in core, do uninit
    if (attach_pool_.size() == 0) {
        MY_LOGD("add uninit task into jobQueue");
        state_ = kUninit;
        jobQueue_->addJob(
            std::bind(&CaptureFeatureBufferPoolCore::uninit, this));
    }

    return MTRUE;
}

BufferPtr
CaptureFeatureBufferPoolCore::getBuffer(BufferInfoT info)
{
    // get available buffer
    BufferPtr buffer = nullptr;
    {
        std::lock_guard<std::mutex> _l(lock_);
        if (avail_buffer_.size() > 0) {
            buffer = avail_buffer_.back();
            avail_buffer_.pop_back();
        } else if (buf_cur_cnt_ < buf_max_cnt_) {
          MY_LOGD("late alloc new buf");
          buffer = alloc();
          if (buffer == nullptr) {
            MY_LOGE("fail to alloc buf");
            return nullptr;
          }
          buf_cur_cnt_++;
          MY_LOGD("new %u th heap for pool", buf_cur_cnt_.load());
        } else {
            return nullptr;
        }
    }

    // prepare the buffer
    if (buffer->prepare(info) == MFALSE) {
        //MY_LOGE("fail to prepare buffer");

        // push the buffer back to pool
        std::lock_guard<std::mutex> _l(lock_);
        avail_buffer_.push_back(buffer);

        return nullptr;
    }

    return buffer;
}

MBOOL
CaptureFeatureBufferPoolCore::putBuffer(BufferPtr buffer)
{
    std::lock_guard<std::mutex> _l(lock_);

    avail_buffer_.push_back(buffer);

    return MTRUE;
}

MBOOL
CaptureFeatureBufferPoolCore::init(void)
{
    MY_LOGD("%s+ wxh=%ux%u, size=%u, max_cnt=%u",
            __func__, w_, h_, buf_size_, buf_max_cnt_);

#if 0
    while (buf_cur_cnt_ < buf_max_cnt_) {
        // check if current status is uniniting, if so stop allocate new buffer
        if (state_ == kUninit) {
            MY_LOGI("Uninit is called when pool initing");
            return MTRUE;
        }

        // If fail to alloc heap, delay and try again
        BufferPtr buffer = alloc();
        if (buffer == nullptr) {
            MY_LOGE("alloc fail delay 100ms and try again");
            usleep(100 * 1000);
            continue;
        }

        // insert into heap
        {
            std::lock_guard<std::mutex> _l(lock_);
            buf_cur_cnt_++;
            avail_buffer_.push_back(buffer);
            MY_LOGD("push %u th heap into pool", buf_cur_cnt_.load());
        }

    }
    MY_LOGD("%s-", __func__);
#else
    MY_LOGD("%s- late allocate mode", __func__);
#endif

    return MTRUE;
}

MBOOL
CaptureFeatureBufferPoolCore::uninit(void)
{
    // debounce for quick leave/enter camera case
    {
        MUINT32 delay_in_sec = 3;
        MY_LOGD("%s delay %us...", __func__, delay_in_sec);
        std::unique_lock<std::mutex> _l(lock_);
        cv_.wait_for(_l, std::chrono::seconds(delay_in_sec));
        MY_LOGD("%s+", __func__);
    }

    while (1) {
        if (state_ == kInit) {
            MY_LOGI("Init is called when pool uniniting");
            return MTRUE;
        }

        BufferPtr buffer = nullptr;

        {
            std::lock_guard<std::mutex> _l(lock_);
            if (avail_buffer_.size() != 0 && buf_cur_cnt_ > 0) {
                buffer = avail_buffer_.back();
                avail_buffer_.pop_back();
                buf_cur_cnt_--;
                MY_LOGD("release a buffer, cnt:%d", buf_cur_cnt_.load());
            }
        }

        if (buffer != nullptr) {
            buffer->abandon();
        }

        if (buf_cur_cnt_ == 0) {
            break;
        }

    }

    MY_LOGD("%s-", __func__);

    return MTRUE;
}

BufferPtr
CaptureFeatureBufferPoolCore::alloc(void)
{
    IIonImageBufferHeap::AllocImgParam_t blobParam(buf_size_, 0);
    IIonImageBufferHeap::AllocExtraParam extraParam;

    extraParam.ionDevice = ion_device_;
    sp<IIonImageBufferHeap> heap =
        IIonImageBufferHeap::create("CapPool", blobParam, extraParam, MFALSE);

    if (heap == nullptr) {
        MY_LOGE("fail to alloc buffer heap");
        return nullptr;
    }

    if (heap->lockBuf("CapPool", BUFFER_USAGE) == MFALSE) {
        MY_LOGE("fail to lock buffer heap");
        heap = nullptr; /* release the heap */
        return nullptr;
    }

    std::shared_ptr<CaptureFeatureBuffer> buffer =
        std::make_shared<CaptureFeatureBuffer>(heap, buf_size_);

    return buffer;
}


CaptureFeatureBufferPoolCore::CaptureFeatureBufferPoolCore()
    : state_(kUninit)
    , buf_cur_cnt_(0)
    , buf_size_(0)
    , w_(0)
    , h_(0)
{
    jobQueue_ =
        std::make_shared<NSCam::JobQueue<void()>>("CaptureFeatureBufferPool");

    // MFNR need YUV420 12bits * 7
    buf_max_cnt_ =
        ::property_get_int32("vendor.debug.camera.capfeaturebufpool.count",
                            CAP_FEATURE_BUF_POOL_BUF_CNT);

    // IHalSensorList
    IHalSensorList *const sensor_list = MAKE_HalSensorList();
    if (sensor_list == nullptr) {
        MY_LOGA("cannot get sensor list");
    }

    for (int i = 0; i < sensor_list->queryNumberOfSensors(); i++) {
        /* query sensor info */
        MUINT32 dev = (MUINT32)sensor_list->querySensorDevIdx(i);
        NSCam::SensorStaticInfo info;
        memset(&info, 0, sizeof(NSCam::SensorStaticInfo));
        sensor_list->querySensorStaticInfo(dev, &info);

        MUINT32 w = ALIGN_CEIL(info.previewWidth, 128);
        MUINT32 h = ALIGN_CEIL(info.previewHeight, 128);
        MUINT32 size = w * h * 9 / 4; /* YUV420 12bits (1.5 x 1.5) */
        MY_LOGD("sensor id:%u size=%ux%u", dev, w, h);
        if (size > buf_size_) {
            buf_size_ = size;
            w_ = w;
            h_ = h;
        }
    }

    ion_device_ = IIonDeviceProvider::get()->makeIonDevice("CapPool");
}

CaptureFeatureBufferPoolCore::~CaptureFeatureBufferPoolCore()
{
}
