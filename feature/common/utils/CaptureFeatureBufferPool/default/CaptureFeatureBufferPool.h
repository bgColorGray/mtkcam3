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
    inline BufferStateT state(void) {return kAbandoned;}
    inline const BufferInfoT& info(void) {return info_;}
    inline sp<IImageBuffer> ptr(void) {return nullptr;}

public:
    CaptureFeatureBuffer(void): info_(BufferInfoT(0, 0, 0)) {}
    ~CaptureFeatureBuffer(void) {}

private: // data member
    BufferInfoT info_;
};

class CaptureFeatureBufferPool : public ICaptureFeatureBufferPool {

typedef std::shared_ptr<CaptureFeatureBufferPoolCore> PoolCorePtr;
typedef std::shared_ptr<ICaptureFeatureBuffer> IBufferPtr;
typedef std::shared_ptr<CaptureFeatureBuffer> BufferPtr;

public: // interface
    IBufferPtr getBuffer(BufferInfoT info) {return nullptr;}
    MBOOL putBuffer(IBufferPtr buffer) {return MFALSE;}
    MUINT32 buf_size(void) const {return 0;};

public:
    CaptureFeatureBufferPool(void) {}
    ~CaptureFeatureBufferPool(void) {}
};

};  // NSCam

#endif
