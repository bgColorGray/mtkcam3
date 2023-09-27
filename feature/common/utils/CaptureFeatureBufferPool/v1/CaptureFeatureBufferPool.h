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
#ifndef _CAPTURE_FEATURE_BUFFER_POOL_H_
#define _CAPTURE_FEATURE_BUFFER_POOL_H_

#include <mtkcam3/feature/utils/ICaptureFeatureBufferPool.h>
#include <mtkcam/utils/std/JobQueue.h>
#include <mtkcam/utils/imgbuf/IIonDevice.h>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>


namespace NSCam {

class CaptureFeatureBufferPoolCore;

class CaptureFeatureBuffer : public ICaptureFeatureBuffer {

public: // interface
    inline BufferStateT state(void) {return state_;}
    inline const BufferInfoT& info(void) {return info_;}
    inline sp<IImageBuffer> ptr(void) {return buf_;}

public:
    // prepare buf according to BufferInfoT
    MBOOL prepare(BufferInfoT info);
    // release buf, heap and label state into kAbandoned
    void abandon(void);

public:
    typedef enum BufferSrc {
        kPool,
        kDynamic,
    } BufferSrcT;

    inline BufferSrcT src(void) {return src_;}

public:
    CaptureFeatureBuffer(sp<IIonImageBufferHeap> heap, MUINT32 size);
    ~CaptureFeatureBuffer(void);

private: // data member
    std::mutex lock_;
    BufferStateT state_;
    BufferSrcT src_;
    BufferInfoT info_;
    MUINT32 size_;
    sp<IIonImageBufferHeap> heap_;
    sp<IImageBuffer> buf_;
};

class CaptureFeatureBufferPool : public ICaptureFeatureBufferPool {

typedef std::shared_ptr<CaptureFeatureBufferPoolCore> PoolCorePtr;
typedef std::shared_ptr<ICaptureFeatureBuffer> IBufferPtr;
typedef std::shared_ptr<CaptureFeatureBuffer> BufferPtr;

public: // interface
    IBufferPtr getBuffer(BufferInfoT info);
    MBOOL putBuffer(IBufferPtr buffer);
    MUINT32 buf_size(void) const;

public:
    CaptureFeatureBufferPool(char* name);
    ~CaptureFeatureBufferPool(void);

private:
    std::mutex lock_;
    PoolCorePtr core_;
    std::string name_;
    std::vector<std::shared_ptr<CaptureFeatureBuffer>> used_buffer_;
};

class CaptureFeatureBufferPoolCore {

typedef std::shared_ptr<CaptureFeatureBufferPoolCore> PoolCorePtr;
typedef ICaptureFeatureBuffer::BufferInfo BufferInfoT;
typedef std::shared_ptr<CaptureFeatureBuffer> BufferPtr;

enum {
    kInit,
    kUninit
};

public: // Operation for CaptureFeatureBufferPool
    static PoolCorePtr getInstance(void);
    MBOOL attach(CaptureFeatureBufferPool* pool);
    MBOOL detach(CaptureFeatureBufferPool* pool);
    BufferPtr getBuffer(BufferInfoT info);
    MBOOL putBuffer(BufferPtr buffer);
    MUINT32 buf_size(void) {return buf_size_;};

private:
    MBOOL init(void); // called when first pool attached on it
    MBOOL uninit(void); // called when no pool attached on in
    BufferPtr alloc(void);

public:
    CaptureFeatureBufferPoolCore(void);
    ~CaptureFeatureBufferPoolCore(void);

private:
    std::mutex lock_;
    std::condition_variable cv_;

    std::atomic<MUINT32> state_;
    std::atomic<MUINT32> buf_cur_cnt_;
    MUINT32 buf_max_cnt_;
    MUINT32 buf_size_;
    MUINT32 w_;
    MUINT32 h_;

    std::shared_ptr<NSCam::JobQueue<void()>> jobQueue_;

    std::shared_ptr<IIonDevice> ion_device_;
    std::vector<BufferPtr> avail_buffer_;
    std::vector<CaptureFeatureBufferPool*> attach_pool_;
};

};  // NSCam

#endif
