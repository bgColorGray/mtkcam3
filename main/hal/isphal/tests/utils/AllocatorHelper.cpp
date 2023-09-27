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

#include "AllocatorHelper.h"
#define DEBUG_LOG_TAG "isphal_test"
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "MapperHelper.h"
#include "types.h"

#include <vendor/mediatek/hardware/camera/isphal/1.0/IISPModule.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/ICallback.h>

#include <system/camera_metadata.h>
#include <mtkisp_metadata.h>

#include <android/hardware/camera/device/3.2/types.h>

#include <grallocusage/GrallocUsageConversion.h>

#include <hardware/gralloc1.h>

#include <cutils/native_handle.h>
// To avoid tag not sync with MTK HAL we included
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
//#include <system/camera_metadata_tags.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <cassert>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <regex>

#include <sync/sync.h>

#include <gtest/gtest.h>

#include <json/json.h>

#include <sys/stat.h>
#include <cutils/properties.h>
#include <random>

using namespace isphal_test;

using namespace ::vendor::mediatek::hardware::camera;
using namespace ::vendor::mediatek::hardware::camera::isphal::V1_0;

using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::mapper::V4_0::IMapper;
using ::android::hardware::graphics::mapper::V4_0::BufferDescriptor;
using ::android::hardware::graphics::allocator::V4_0::IAllocator;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;
using ::android::RefBase;
using ::android::sp;
using ::android::wp;

AllocatorHelper::AllocatorHelper()
{
    init();
}

void AllocatorHelper::init()
{
    // get IAllocator service
    mAllocator = IAllocator::getService();
    ASSERT_TRUE(mAllocator) << "failed to get IAllocator interface";
}

void AllocatorHelper::allocateGraphicBuffer(
        uint32_t width, uint32_t height, uint64_t usage, PixelFormat format,
        hidl_handle *handle, uint32_t *rowStride) const
{
    ASSERT_NE(handle, nullptr);

    IMapper::BufferDescriptorInfo descriptorInfo {
            .width = width,
            .height = height,
            /*
             * NOTE: A buffer with multiple layers may be used as
             * the backing store of an array texture.
             * All layers of a buffer share the same characteristics (e.g.,
             * dimensions, format, usage). Devices that do not support
             * GRALLOC1_CAPABILITY_LAYERED_BUFFERS must allocate only buffers
             * with a single layer.
             */
            .layerCount = 1,
            .format = (::android::hardware::graphics::common::V1_2::PixelFormat)format,
            .usage = usage
        };

    BufferDescriptor descriptor;
    MapperHelper::getInstance().createDescriptor(descriptorInfo, descriptor);

    auto ret = mAllocator->allocate(descriptor, 1u,
        [&](auto err, uint32_t stride, const auto& buffers) {
            if (rowStride)
                *rowStride = stride;
            ASSERT_EQ(err, ::android::hardware::graphics::mapper::V4_0::Error::NONE)
                << "IAllocator::allocate() failed(" << toString(err) << ")";
            ASSERT_EQ(buffers.size(), 1u);
            *handle = buffers[0];
        });
    ASSERT_TRUE(ret.isOk());
}

void AllocatorHelper::queryStride(
        uint32_t width, uint32_t height, uint64_t usage, PixelFormat format,
        uint32_t *rowStride) const
{
    buffer_handle_t handle = nullptr;

    IMapper::BufferDescriptorInfo descriptorInfo {
            .width = width,
            .height = height,
            /*
             * NOTE: A buffer with multiple layers may be used as
             * the backing store of an array texture.
             * All layers of a buffer share the same characteristics (e.g.,
             * dimensions, format, usage). Devices that do not support
             * GRALLOC1_CAPABILITY_LAYERED_BUFFERS must allocate only buffers
             * with a single layer.
             */
            .layerCount = 1,
            .format = (::android::hardware::graphics::common::V1_2::PixelFormat)format,
            .usage = usage
        };

    BufferDescriptor descriptor;
    MapperHelper::getInstance().createDescriptor(descriptorInfo, descriptor);

    auto ret = mAllocator->allocate(descriptor, 1u,
        [&](auto err, uint32_t stride, const auto& buffers) {
            if (rowStride)
                *rowStride = stride;
            ASSERT_EQ(err, ::android::hardware::graphics::mapper::V4_0::Error::NONE)
                << "IAllocator::allocate() failed(" << toString(err) << ")";
            ASSERT_EQ(buffers.size(), 1u);
            handle = buffers[0];
        });

    if (handle)
        MapperHelper::getInstance().freeBuffer(handle);

    CAM_LOGD("use w(%d), h(%d), f(%d) u(%d) to query Stride: %d",
             width, height, format, usage, *rowStride);
}