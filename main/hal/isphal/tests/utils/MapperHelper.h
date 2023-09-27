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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#pragma once
#include "Singleton.h"
#include <android/hardware/graphics/common/1.0/types.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <cutils/native_handle.h>
#include <gralloctypes/Gralloc4.h>
#include <hidl/HidlSupport.h>
#include <system/graphics.h>

using ::android::hardware::graphics::mapper::V4_0::IMapper;
using ::android::hardware::graphics::mapper::V4_0::BufferDescriptor;
using ::android::sp;

using ::android::hardware::graphics::common::V1_0::PixelFormat;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
using MetadataType = android::hardware::graphics::mapper::V4_0::IMapper::MetadataType;

class MapperHelper : public Singleton<MapperHelper>
{
public:
    MapperHelper();

    // Creates a buffer descriptor, which is used with IAllocator to allocate
    // buffers
    void createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo,
            BufferDescriptor& descriptor);

    // NOTE: This function replace the imported buffer handle in-place.
    //       The returned outBufferHandle must be freed with freeBuffer().
    void importBuffer(buffer_handle_t& outBufferHandle);
    void freeBuffer(buffer_handle_t bufferHandle) const;
    // The ownership of acquireFence is always transferred to the callee, even
    // on error

    template <class T>
    using DecodeFunction = android::status_t (*)(const android::hardware::hidl_vec<uint8_t>& input, T* output);

    template <class T>
    android::status_t get(
            buffer_handle_t bufferHandle,
            const android::hardware::graphics::mapper::V4_0::IMapper::MetadataType& metadataType,
            DecodeFunction<T> decodeFunction, T* outMetadata) const;

    android::status_t getPlaneLayouts(buffer_handle_t bufferHandle,
                             std::vector<PlaneLayout>* outPlaneLayouts) const;
    android::status_t lock(buffer_handle_t bufferHandle, uint64_t cpuUsage,
            const IMapper::Rect& accessRegion, int acquireFence, void*& buffer) const;
    android::status_t lockYCbCr_4_0(buffer_handle_t bufferHandle, uint64_t usage, const IMapper::Rect& bounds,
            int acquireFence, android_ycbcr &ycbcr) const;

    // unlock returns a fence sync object (or -1) and the fence sync object is
    // owned by the caller
    void unlock(buffer_handle_t bufferHandle, int& releaseFence) const;

private:
    sp<IMapper> mMapper;

    void init();
};