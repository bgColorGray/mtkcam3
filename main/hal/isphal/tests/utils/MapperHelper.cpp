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

#include "MapperHelper.h"
#define DEBUG_LOG_TAG "isphal_test"
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "types.h"

#include <vendor/mediatek/hardware/camera/isphal/1.0/IISPModule.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/ICallback.h>

#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
#include <system/camera_metadata.h>
#include <mtkisp_metadata.h>

#include <android/hardware/camera/device/3.2/types.h>

#include <android/hardware/graphics/common/1.0/types.h>

#include <gralloctypes/Gralloc4.h>
#include <grallocusage/GrallocUsageConversion.h>
#include <hardware/gralloc1.h>

#include <cutils/native_handle.h>
#include <cutils/properties.h>
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
using ::android::hardware::graphics::mapper::V4_0::BufferDescriptor;
using ::android::hardware::graphics::mapper::V4_0::Error;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::device::V3_2::CameraBlob;
using ::android::hardware::camera::device::V3_2::CameraBlobId;
using ::android::hardware::camera::provider::V2_6::ICameraProvider;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;
using ::android::RefBase;
using ::android::sp;
using ::android::wp;

MapperHelper::MapperHelper()
{
    init();
}

void MapperHelper::init()
{
    // get IMapper service
    mMapper = IMapper::getService();
    ASSERT_TRUE(mMapper) << "failed to get IMapper interface";
}

void MapperHelper::createDescriptor(
        const IMapper::BufferDescriptorInfo& descriptorInfo,
        BufferDescriptor& descriptor)
{
    auto ret = mMapper->createDescriptor(
        descriptorInfo, [&descriptor](const auto& err, const auto& desc) {
            ASSERT_EQ(err, ::android::hardware::graphics::mapper::V4_0::Error::NONE)
            << "IMapper::createDescriptor() failed(" << toString(err) << ")";
            descriptor = desc;
        });
    ASSERT_TRUE(ret.isOk());
}

void MapperHelper::importBuffer(buffer_handle_t& outBufferHandle)
{
    buffer_handle_t importedHandle;
    auto ret = mMapper->importBuffer(hidl_handle(outBufferHandle),
            [&importedHandle](const auto& err, const auto& buffer)
            {
                ASSERT_EQ(
                    err, ::android::hardware::graphics::mapper::V4_0::Error::NONE);

                importedHandle = static_cast<buffer_handle_t>(buffer);
            });
    ASSERT_TRUE(ret.isOk()) << "importBuffer failed";

    outBufferHandle = importedHandle;
}

void MapperHelper::freeBuffer(buffer_handle_t bufferHandle) const
{
    if (!bufferHandle)
        return;

    auto ret = mMapper->freeBuffer(const_cast<native_handle_t*>(bufferHandle));
    ASSERT_TRUE(ret.isOk()) << "freeBuffer failed(" << toString(ret) << ")";
}

android::status_t MapperHelper::lock(buffer_handle_t bufferHandle, uint64_t cpuUsage,
            const IMapper::Rect& accessRegion, int acquireFence, void*& buffer) const
{
    auto _bufferHandle = const_cast<native_handle_t*>(bufferHandle);

    // put acquireFence in a hidl_handle
    hidl_handle acquireFenceHandle;
    NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 1, 0);
    if (acquireFence >= 0)
    {
        auto h = native_handle_init(acquireFenceStorage, 1, 0);
        h->data[0] = acquireFence;
        acquireFenceHandle = h;
    }

    auto ret = mMapper->lock(_bufferHandle, cpuUsage, accessRegion, acquireFenceHandle,
            [&](const auto& err, const auto& tmpBuffer)
            {
                ASSERT_EQ(
                    err, ::android::hardware::graphics::mapper::V4_0::Error::NONE)
                << "lock buffer" << _bufferHandle << " failed";

                buffer = tmpBuffer;
                CAM_LOGD("lock addr(0x%" PRIxPTR ")", reinterpret_cast<intptr_t>(tmpBuffer));
            });
    // we own acquireFence even on errors
    if (acquireFence >= 0)
        close(acquireFence);

    if(!ret.isOk())
        CAM_LOGE("lock fail!");

    return android::NO_ERROR;
}

template <class T>
android::status_t MapperHelper::get(buffer_handle_t bufferHandle, const MetadataType& metadataType,
                             DecodeFunction<T> decodeFunction, T* outMetadata) const
{
    if (!outMetadata) {
        return android::BAD_VALUE;
    }

    hidl_vec<uint8_t> vec;
    Error error;
    auto ret = mMapper->get(const_cast<native_handle_t*>(bufferHandle), metadataType,
                            [&](const auto& tmpError, const hidl_vec<uint8_t>& tmpVec) {
                                error = tmpError;
                                vec = tmpVec;
                            });

    if (!ret.isOk()) {
        error = Error::NO_RESOURCES;
    }

    if (error != Error::NONE) {
        ALOGE("get(%s, %" PRIu64 ", ...) failed with %d", metadataType.name.c_str(),
              metadataType.value, error);
        return static_cast<android::status_t>(error);
    }

    return decodeFunction(vec, outMetadata);
}


android::status_t MapperHelper::getPlaneLayouts(buffer_handle_t bufferHandle,
                                         std::vector<PlaneLayout>* outPlaneLayouts) const
{
    return get(bufferHandle, android::gralloc4::MetadataType_PlaneLayouts, android::gralloc4::decodePlaneLayouts,
               outPlaneLayouts);
}

android::status_t MapperHelper::lockYCbCr_4_0(buffer_handle_t bufferHandle, uint64_t cpuUsage,
        const IMapper::Rect& accessRegion, int acquireFence,
        android_ycbcr &outYcbcr) const
{
    std::vector<PlaneLayout> planeLayouts;
    android::status_t error = getPlaneLayouts(bufferHandle, &planeLayouts);
    if (error != android::NO_ERROR) {
        return error;
    }

    void* data = nullptr;
    error = lock(bufferHandle, cpuUsage, accessRegion, acquireFence, data);
    if (error != android::NO_ERROR) {
        return error;
    }

    outYcbcr.y = nullptr;
    outYcbcr.cb = nullptr;
    outYcbcr.cr = nullptr;
    outYcbcr.ystride = 0;
    outYcbcr.cstride = 0;
    outYcbcr.chroma_step = 0;

    for (const auto& planeLayout : planeLayouts) {
        for (const auto& planeLayoutComponent : planeLayout.components) {
            if (!android::gralloc4::isStandardPlaneLayoutComponentType(planeLayoutComponent.type)) {
                continue;
            }
            if (0 != planeLayoutComponent.offsetInBits % 8) {
                //unlock(bufferHandle);
                return android::BAD_VALUE;
            }

            uint8_t* tmpData = static_cast<uint8_t*>(data) + planeLayout.offsetInBytes +
                    (planeLayoutComponent.offsetInBits / 8);
            uint64_t sampleIncrementInBytes;

            auto type = static_cast<PlaneLayoutComponentType>(planeLayoutComponent.type.value);
            switch (type) {
                case PlaneLayoutComponentType::Y:
                    if ((outYcbcr.y != nullptr) || (planeLayoutComponent.sizeInBits != 8) ||
                        (planeLayout.sampleIncrementInBits != 8)) {
                        //unlock(bufferHandle);
                        return android::BAD_VALUE;
                    }
                    outYcbcr.y = tmpData;
                    outYcbcr.ystride = planeLayout.strideInBytes;
                    break;

                case PlaneLayoutComponentType::CB:
                case PlaneLayoutComponentType::CR:
                    if (planeLayout.sampleIncrementInBits % 8 != 0) {
                        //unlock(bufferHandle);
                        return android::BAD_VALUE;
                    }

                    sampleIncrementInBytes = planeLayout.sampleIncrementInBits / 8;
                    if ((sampleIncrementInBytes != 1) && (sampleIncrementInBytes != 2)) {
                        //unlock(bufferHandle);
                        return android::BAD_VALUE;
                    }

                    if (outYcbcr.cstride == 0 && outYcbcr.chroma_step == 0) {
                        outYcbcr.cstride = planeLayout.strideInBytes;
                        outYcbcr.chroma_step = sampleIncrementInBytes;
                    } else {
                        if ((static_cast<int64_t>(outYcbcr.cstride) != planeLayout.strideInBytes) ||
                            (outYcbcr.chroma_step != sampleIncrementInBytes)) {
                            //unlock(bufferHandle);
                            return android::BAD_VALUE;
                        }
                    }

                    if (type == PlaneLayoutComponentType::CB) {
                        if (outYcbcr.cb != nullptr) {
                            //unlock(bufferHandle);
                            return android::BAD_VALUE;
                        }
                        outYcbcr.cb = tmpData;
                    } else {
                        if (outYcbcr.cr != nullptr) {
                            //unlock(bufferHandle);
                            return android::BAD_VALUE;
                        }
                        outYcbcr.cr = tmpData;
                    }
                    break;
                default:
                    break;
            };
        }
    }

    return static_cast<android::status_t>(Error::NONE);
}

void MapperHelper::unlock(
        buffer_handle_t bufferHandle, int& releaseFence) const
{
    auto _bufferHandle = const_cast<native_handle_t*>(bufferHandle);

    releaseFence = -1;

    auto ret = mMapper->unlock(_bufferHandle,
            [&](const auto& err, const auto& tmpReleaseFence)
            {
                ASSERT_EQ(
                    err, ::android::hardware::graphics::mapper::V4_0::Error::NONE)
                << "unlock failed";

                auto fenceHandle = tmpReleaseFence.getNativeHandle();
                if (fenceHandle && fenceHandle->numFds == 1)
                {
                    int fd = dup(fenceHandle->data[0]);
                    if (fd >= 0) {
                        releaseFence = fd;
                    } else {
                        CAM_LOGD("failed to dup unlock release fence");
                        sync_wait(fenceHandle->data[0], -1);
                    }
                }
            });

    ASSERT_TRUE(ret.isOk());
}