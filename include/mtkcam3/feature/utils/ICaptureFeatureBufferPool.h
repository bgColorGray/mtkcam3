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

#ifndef _ICAPTURE_FEATURE_BUFFER_POOL_H_
#define _ICAPTURE_FEATURE_BUFFER_POOL_H_

#include <mtkcam/utils/imgbuf/IImageBuffer.h>
#include <mtkcam/def/BuiltinTypes.h>
#include <utils/StrongPointer.h>
#include <string>
#include <memory>

using namespace android;

namespace NSCam {

class ICaptureFeatureBuffer {
public: // struct and typedef
    typedef struct BufferInfo {
        MUINT32 w;
        MUINT32 h;
        MUINT32 fmt;
        MUINT32 align_pixel;
        MUINT32 align_byte;
        explicit BufferInfo(MUINT32 w, MUINT32 h, MUINT32 fmt)
            : w(w)
            , h(h)
            , fmt(fmt)
            , align_pixel(1)
            , align_byte(1)
        {}
    } BufferInfoT;

    typedef enum BufferState {
        kAlive,
        kAbandoned,
    } BufferStateT;

public: // Interface
    // Accessor
    virtual BufferStateT state(void) = 0;
    virtual const BufferInfoT& info(void) = 0;
    virtual sp<IImageBuffer> ptr(void) = 0;

public:
    virtual ~ICaptureFeatureBuffer(void) {}
};

class ICaptureFeatureBufferPool {

public: // typedef
    typedef ICaptureFeatureBuffer::BufferInfo BufferInfoT;
    typedef std::shared_ptr<ICaptureFeatureBuffer> IBufferPtr;

public: // Interface
    // Operation
    /* Follow the rules below otherwise assertion triggered:
     * 1. Use the same instance to put/get BufferPtr
     * 2. Need to put all BufferPtr before instance free.
     * 3. User needs to handle if getBuffer fail.
     * 4. User does not need to lock/unlock for IBufferPtr
     */
    static std::shared_ptr<ICaptureFeatureBufferPool> getInstance(char* user);
    virtual IBufferPtr getBuffer(BufferInfoT info) = 0;
    virtual MBOOL putBuffer(IBufferPtr buffer) = 0;

    // Accessor
    virtual MUINT32 buf_size(void) const = 0; // size of each buffer

public:
    virtual ~ICaptureFeatureBufferPool(void) {}
};

};  // NSCam

#endif
