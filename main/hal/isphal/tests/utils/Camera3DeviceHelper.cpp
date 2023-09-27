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

#include "Camera3DeviceHelper.h"
#define DEBUG_LOG_TAG "isphal_test"
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "utils/Singleton.h"
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

using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
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

const std::string Camera3DeviceHelper::kCameraProviderName("internal/0");

Camera3DeviceHelper::Camera3DeviceHelper()
{
    init();
}

void Camera3DeviceHelper::init()
{
    mProvider = ICameraProvider::getService(kCameraProviderName);
    ASSERT_TRUE(mProvider) << "failed to get camera provider interface";

    CAM_LOGI("camera provider(%s) name(%s) discovered",
            ICameraProvider::descriptor, kCameraProviderName.c_str());
}

void Camera3DeviceHelper::getCameraDeviceName(
        const std::string& cameraId, std::string& cameraDeviceName)
{
    // find the desired camera device name
    Return<void> ret;
    ret = mProvider->getCameraIdList(
        [&](auto status, const auto& idList) {
            ASSERT_EQ(::android::hardware::camera::common::V1_0::Status::OK, status)
                << "getCameraIdList failed: " << toString(status);

            for (size_t i = 0; i < idList.size(); i++)
            {
                const std::string& _cameraDeviceName(idList[i]);

                CAM_LOGV("camera device name[%zu] is %s", i, _cameraDeviceName.c_str());
                // TODO: check if need to keep the parsed information in mind
                // Extract major/minor versions, type and id
                uint16_t major, minor;
                std::string type, id;
                ::android::status_t res = parseDeviceName(_cameraDeviceName,
                    &major, &minor, &type, &id);
                ASSERT_EQ(::android::OK, res) << "parseDeviceName failed: " <<
                    strerror(-res) << " (" << res << ")";

                if (id.compare(cameraId) != 0)
                    continue;

                cameraDeviceName = _cameraDeviceName;
                CAM_LOGD("camera device name(%s) found", cameraDeviceName.c_str());
                break;
            }
        });
    ASSERT_TRUE(ret.isOk());
}

::android::status_t Camera3DeviceHelper::parseDeviceName(const std::string& name,
        uint16_t *major, uint16_t *minor, std::string *type, std::string *id)
{
    // Format must be "device@<major>.<minor>/<type>/<id>"

#define ERROR_MSG_PREFIX "%s: Invalid device name '%s'. " \
    "Should match 'device@<major>.<minor>/<type>/<id>' - "

    if (!major || !minor || !type || !id)
        return ::android::INVALID_OPERATION;

    // Verify starting prefix
    const char expectedPrefix[] = "device@";

    if (name.find(expectedPrefix) != 0) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "does not start with '%s'",
                __FUNCTION__, name.c_str(), expectedPrefix);
        return ::android::BAD_VALUE;
    }

    // Extract major/minor versions
    constexpr std::string::size_type atIdx = sizeof(expectedPrefix) - 2;
    std::string::size_type dotIdx = name.find('.', atIdx);
    if (dotIdx == std::string::npos) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "does not have @<major>. version section",
                __FUNCTION__, name.c_str());
        return ::android::BAD_VALUE;
    }
    std::string::size_type typeSlashIdx = name.find('/', dotIdx);
    if (typeSlashIdx == std::string::npos) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "does not have .<minor>/ version section",
                __FUNCTION__, name.c_str());
        return ::android::BAD_VALUE;
    }

    char *endPtr;
    errno = 0;
    long majorVal = strtol(name.c_str() + atIdx + 1, &endPtr, 10);
    if (errno != 0) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "cannot parse major version: %s (%d)",
                __FUNCTION__, name.c_str(), strerror(errno), errno);
        return ::android::BAD_VALUE;
    }
    if (endPtr != name.c_str() + dotIdx) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "major version has unexpected length",
                __FUNCTION__, name.c_str());
        return ::android::BAD_VALUE;
    }
    long minorVal = strtol(name.c_str() + dotIdx + 1, &endPtr, 10);
    if (errno != 0) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "cannot parse minor version: %s (%d)",
                __FUNCTION__, name.c_str(), strerror(errno), errno);
        return ::android::BAD_VALUE;
    }
    if (endPtr != name.c_str() + typeSlashIdx) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "minor version has unexpected length",
                __FUNCTION__, name.c_str());
        return ::android::BAD_VALUE;
    }
    if (majorVal < 0 || majorVal > UINT16_MAX || minorVal < 0 || minorVal > UINT16_MAX) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "major/minor version is out of range of uint16_t: %ld.%ld",
                __FUNCTION__, name.c_str(), majorVal, minorVal);
        return ::android::BAD_VALUE;
    }

    // Extract type and id

    std::string::size_type instanceSlashIdx = name.find('/', typeSlashIdx + 1);
    if (instanceSlashIdx == std::string::npos) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "does not have /<type>/ component",
                __FUNCTION__, name.c_str());
        return ::android::BAD_VALUE;
    }
    std::string typeVal = name.substr(typeSlashIdx + 1, instanceSlashIdx - typeSlashIdx - 1);

    if (instanceSlashIdx == name.size() - 1) {
        CAM_LOGE(ERROR_MSG_PREFIX
                "does not have an /<id> component",
                __FUNCTION__, name.c_str());
        return ::android::BAD_VALUE;
    }
    std::string idVal = name.substr(instanceSlashIdx + 1);

#undef ERROR_MSG_PREFIX

    *major = static_cast<uint16_t>(majorVal);
    *minor = static_cast<uint16_t>(minorVal);
    *type = typeVal;
    *id = idVal;

    return ::android::OK;
}

void Camera3DeviceHelper::getCameraCharacteristics(
        const std::string& cameraDeviceName, CameraMetadata& cameraMetadata)
{
    Return<::android::hardware::camera::common::V1_0::Status> status(
            ::android::hardware::camera::common::V1_0::Status::OK);

    // get camera characteristics
    CAM_LOGD("getCameraCharacteristics: camera device(%s)", cameraDeviceName.c_str());
    sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_x;
    Return<void> ret = mProvider->getCameraDeviceInterface_V3_x(
            cameraDeviceName, [&](auto status, const auto& device3) {
            ASSERT_EQ(::android::hardware::camera::common::V1_0::Status::OK, status)
                << "getCameraDeviceInterface_V3_x failed(" << toString(status) << ")";
            ASSERT_NE(device3, nullptr);
            device3_x = device3;
            });
    ASSERT_TRUE(ret.isOk());

    // get cahced cameraCharacteristics if exist
    auto search = mCameraCharacteristics.find(cameraDeviceName);
    if (search != mCameraCharacteristics.end())
    {
        cameraMetadata = search->second;
        return;
    }

    if (device3_x == nullptr) {
      ASSERT_TRUE(false) << "device3_x is null !";
    }

    ret = device3_x->getCameraCharacteristics([&status, &cameraMetadata](
                ::android::hardware::camera::common::V1_0::Status s,
                ::android::hardware::camera::device::V3_2::CameraMetadata metadata) {
            status = s;
            if (s == ::android::hardware::camera::common::V1_0::Status::OK)
            {
                camera_metadata_t *buffer =
                reinterpret_cast<camera_metadata_t*>(metadata.data());
                size_t expectedSize = metadata.size();
                int res = validate_camera_metadata_structure(buffer, &expectedSize);
                if (res == 0)
                {
                    cameraMetadata = metadata;
                    return;
                }

                CAM_LOGE("validate camera metadata structure failed(%d)", res);
                status = ::android::hardware::camera::common::V1_0::Status::INTERNAL_ERROR;
            }
    });
    ASSERT_TRUE(ret.isOk())
        << "Transaction error getting camera characteristics for device: "
        << ret.description().c_str();
    ASSERT_EQ(::android::hardware::camera::common::V1_0::Status::OK, status)
        << "Unable to get camera characteristics for device: " << toString(status);

    // store cameraCharacteristics for later use
    bool isOK = mCameraCharacteristics.emplace(cameraDeviceName, cameraMetadata).second;
    ASSERT_TRUE(isOK) << cameraDeviceName << ": emplace camera characteristics failed";
};