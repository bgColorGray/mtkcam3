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

#define LOG_TAG "securecamera_test"

#include "utils/TestBase.h"
#include "system/types.h"
#include "graphics/types.h"
#include "graphics/AllocatorHelper.h"
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/TypeTraits.h>
#include <sync/sync.h>

// ------------------------------------------------------------------------

namespace NSCam {
namespace tests {

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_HAL_SERVER);

using ::android::hardware::graphics::common::V1_2::BufferUsage;
using ::android::hardware::graphics::common::V1_2::PixelFormat;
using ::android::hardware::hidl_handle;

// ------------------------------------------------------------------------

/**
 * This test fixture is for testing class
 * ::android::hardware::graphics::mapper::V4_0::IMapper and
 * ::android::hardware::graphics::allocator::V4_0::IAllocator
 * with buffer usage.
 */
class MapperAndAllocatorTest : public TestBase,
    public ::testing::WithParamInterface<uint64_t>
{
protected:
    void SetUp() override;
    void TearDown() override;

    static constexpr uint32_t kImageWidth = 640;
    static constexpr uint32_t kImageHeight = 480;

   AllocatorHelper& mAllocator = AllocatorHelper::getInstance();
}; // class MapperAndAllocatorTest

void MapperAndAllocatorTest::SetUp()
{
    TestBase::SetUp();
}

void MapperAndAllocatorTest::TearDown()
{
    TestBase::TearDown();
}

// ------------------------------------------------------------------------

TEST_P(MapperAndAllocatorTest, AllocateWithPrivateUsage)
{
    const auto bufferUsage = GetParam();

    hidl_handle bufferHandle;

    if (NSCam::kSkipSecureBuffer &&
           (toLiteral(MTKCam_BufferUsage::PROT_CAMERA_LEGACY) ==
           (toLiteral(MTKCam_BufferUsage::PROT_CAMERA_LEGACY) &
            toLiteral(MTKCam_BufferUsage(bufferUsage))))
        )
    {
        GTEST_SKIP();
    }

    mAllocator.allocateGraphicBuffer(
            kImageWidth, kImageHeight, bufferUsage,
            PixelFormat::IMPLEMENTATION_DEFINED, &bufferHandle);
}
INSTANTIATE_TEST_SUITE_P(GRALLOC, MapperAndAllocatorTest,
    ::testing::Values(
        MTKCam_BufferUsage::PROT_CAMERA_OUTPUT        /* ImageReader case */,
        MTKCam_BufferUsage::PROT_CAMERA_INPUT         /* ImageWriter case */,
        MTKCam_BufferUsage::PROT_CAMERA_BIDIRECTIONAL /* ImageReader and ImageWriter case */,
        MTKCam_BufferUsage::PROT_CAMERA_LEGACY        /* Legacy camera HAL3 secure buffer heap */));

} // namespace tests
} // namespace NSCam
