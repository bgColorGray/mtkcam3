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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IBUFFERHANDLECACHEMGR_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IBUFFERHANDLECACHEMGR_H_

#include "HidlCameraDevice.h"

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <cutils/native_handle.h>
#include <mtkcam3/main/hal/device/3.x/device/mtkcamhal_types.h> // v3::CaptureResult

#include <vector>

typedef int32_t                     StreamId_T;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace hidl_dev3 {
namespace Utils {

// BufferHandle for createGraphicImageBuffer
struct  BufferHandle
{
    uint64_t        bufferId = 0;
    buffer_handle_t bufferHandle = nullptr;
    std::shared_ptr<NSCam::v3::AppBufferHandleHolder> appBufferHandleHolder = nullptr;
    int32_t         ionFd = -1;
    struct timespec cachedTime = {};
};

struct RequestBufferCache
{

    uint32_t frameNumber;
    android::KeyedVector<StreamId_T, BufferHandle> bufferHandleMap;
};

/**
 * An interface of buffer handle cache.
 */
class IBufferHandleCacheMgr
    : public virtual android::RefBase
{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.

    static auto     create(
                    ) -> IBufferHandleCacheMgr*;

    virtual auto    destroy() -> void                                       = 0;

    virtual auto    configBufferCacheMap(
                        const V3_5::StreamConfiguration& requestedConfiguration
                    )   -> void                                             = 0;

    virtual auto    removeBufferCache(
                        const hidl_vec<V3_2::BufferCache>& cachesToRemove
                    )   -> void                                             = 0;

    virtual auto    registerBufferCache(
                        const hidl_vec<V3_4::CaptureRequest>& requests,
                        std::vector<RequestBufferCache>& vBufferCache
                    )   -> void                                             = 0;

    // virtual auto    markBufferFreeByOthers(
    //                     std::vector<NSCam::v3::CaptureResult>& results
    //                 )   -> void                                             = 0;


    virtual auto    dumpState() -> void                                     = 0;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};  //namespace hidl_dev3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IBUFFERHANDLECACHEMGR_H_

