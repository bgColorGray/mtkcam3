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

/**
 * This test fixture is for testing class IISPModule.
 */

#define DEBUG_LOG_TAG "isphal_test"
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "utils/Singleton.h"
#include "ISPModuleTest.h"
#include "types.h"

#include <vendor/mediatek/hardware/camera/isphal/1.0/IISPModule.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/ICallback.h>

#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
#include <system/camera_metadata.h>
#include <mtkisp_metadata.h>

#include <android/hardware/camera/device/3.2/types.h>

#include <android/hardware/graphics/common/1.0/types.h>

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

ISPModuleTest::ISPModuleTest()
{
    init();
}

ISPModuleTest::~ISPModuleTest()
{
    // disconnect from ISPModule
    mISPModule.clear();
}

void ISPModuleTest::init()
{
    // connect to ISPModule
    mISPModule = IISPModule::getService("internal/0");
    ASSERT_TRUE(mISPModule) << "failed to get ISPModule interface";
}

void ISPModuleTest::linkToDeath(sp<HalInterface>& halInterface)
{
    halInterface = new HalInterface();
    Return<bool> linked = mISPModule->linkToDeath(
            halInterface->getHidlDeathRecipient(),
            reinterpret_cast<uint64_t>(halInterface.get()) /*cookie*/);

    ASSERT_TRUE(linked.isOk())
        << "Transaction error in linking to ISPModule death: "
        << linked.description().c_str();
    ASSERT_TRUE(linked) << "Unable to link to ISPModule death notifications";
}

void ISPModuleTest::unlinkToDeath(const sp<HalInterface>& halInterface)
{
    Return<bool> unlinked =
        mISPModule->unlinkToDeath(halInterface->getHidlDeathRecipient());

    ASSERT_TRUE(unlinked.isOk())
        << "Transaction error in unlinking from ISPModule death: "
        << unlinked.description().c_str();
    ASSERT_TRUE(unlinked) << "Unable to unlink from ISPModule death notifications";
}

void ISPModuleTest::openISPDevice(sp<IDevice>& device)
{
    auto status(isphal::V1_0::Status::OK);
    Return<void> ret = mISPModule->openISPDevice([&status, &device](
                isphal::V1_0::Status _status, const sp<IDevice>& _device) {
                status = _status;
                device = _device;
                return Void();
            });

    ASSERT_TRUE(ret.isOk())
        << "Transaction error in openISPDevice from ISPModule: "
        << ret.description().c_str();
    ASSERT_EQ(status, isphal::V1_0::Status::OK)
        << "openISPDevice failed(" << toString(status) << ")";
    ASSERT_TRUE(device) << "invalid ISP device";
}

void ISPModuleTest::closeISPDevice(sp<IDevice>& device)
{
    Return<isphal::V1_0::Status> ret = device->closeISPDevice();
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in closeISPDevice from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "closeISPDevice failed";

    device.clear();
}