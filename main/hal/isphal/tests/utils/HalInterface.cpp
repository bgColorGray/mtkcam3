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

#include "HalInterface.h"
#define DEBUG_LOG_TAG "isphal_test"
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000

#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "utils/Singleton.h"
#include "utils/MapperHelper.h"
#include "types.h"

#include <vendor/mediatek/hardware/camera/isphal/1.0/IISPModule.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/ICallback.h>

#include <system/camera_metadata.h>
#include <mtkisp_metadata.h>

#include <android/hardware/camera/device/3.2/types.h>
#include <android/hardware/graphics/common/1.0/types.h>
#include <android/hardware/graphics/allocator/4.0/IAllocator.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>

#include <grallocusage/GrallocUsageConversion.h>

#include <hardware/gralloc1.h>

#include <cutils/native_handle.h>
#include <cutils/properties.h>
// To avoid tag not sync with MTK HAL we included
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

#include <sys/stat.h>

using namespace isphal_test;

using namespace ::vendor::mediatek::hardware::camera;
using namespace ::vendor::mediatek::hardware::camera::isphal::V1_0;

using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::mapper::V4_0::IMapper;
using ::android::hardware::camera::device::V3_2::CameraBlob;
using ::android::hardware::camera::device::V3_2::CameraBlobId;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;
using ::android::RefBase;
using ::android::sp;
using ::android::wp;

HalInterface::HalInterface()
{
    // register the recipient for a notification if the hwbinder object goes away
    mClientCallback = new HalInterface::ClientCallback(this);
}

HalInterface::~HalInterface()
{
}

void HalInterface::recordISPRequest(uint32_t requestNumber,
        FileBufferPairs&& inputFileBufferPairs, FileBufferPairs&& outputFileBufferPairs)
{
    ASSERT_TRUE(mClientCallback);

    bool isOK = mClientCallback->mCaptureRequest.emplace(requestNumber,
            ClientCallback::CaptureRequest(requestNumber,
                std::move(inputFileBufferPairs), std::move(outputFileBufferPairs))).second;
    ASSERT_TRUE(isOK) << __FUNCTION__ << ": emplace capture request " <<
        requestNumber << " failed";
}

::android::status_t HalInterface::waitResult(int timeout)
{
    return mClientCallback->waitResult(timeout);
}

::android::status_t HalInterface::ClientCallback::waitResult(int timeout)
{
    std::unique_lock<std::mutex> l(mResultMutex);

    CAM_LOGD("Wait ISP result");
    if (timeout == TIMEOUT_NEVER)
    {
        const auto kTimeout = std::chrono::milliseconds(3000);
        do {
            std::cv_status ret = mResultCV.wait_for(l, kTimeout);
            if (ret == std::cv_status::no_timeout)
                return ::android::OK;

            // show timed-out message verbosely
            CAM_LOGE("waitResult timeout in %lld milliseconds, wait again...",
                    kTimeout.count());
        } while (true);
    }

    const auto kTimeout = std::chrono::milliseconds(timeout);
    const std::cv_status ret = mResultCV.wait_for(l, kTimeout);
    if (ret == std::cv_status::timeout)
    {
        CAM_LOGE("waitResult timeout in %d milliseconds", timeout);
        return ::android::TIMED_OUT;
    }

    return ::android::OK;
}

void HalInterface::ClientCallback::serviceDied(
        uint64_t cookie, const wp<IBase>& who)
{
    (void) who;
    CAM_LOGI("ISPModule(%" PRIu64 ") has died", cookie);

    // TODO: do something here if necessary when the HAL server has gone

    // release HIDL interface
    auto parent = mInterface.promote();
    if (parent)
        parent->mClientCallback.clear();
}

Return<void> HalInterface::ClientCallback::processResult(
        const ISPResult& result)
{
    if (result.status != isphal::V1_0::Status::OK)
    {
        CAM_LOGE("processResult %u failed", result.reqNumber);
        return Void();
    }

    auto search = mCaptureRequest.find(result.reqNumber);
    if (search == mCaptureRequest.end())
    {
        CAM_LOGE("cannot find capture request %u", result.reqNumber);
        return Void();
    }

    // notify the outstanding test case to finish its remaining task
    CAM_LOGD("Isp callback done");

    // writeFile may cost performance therefore we set it after condition notify
    writeFile(search->second);
    mResultCV.notify_one();

    // remove result from the list of the original rquests
    mCaptureRequest.erase(search);
    return Void();
}

Return<void> HalInterface::ClientCallback::processEarlyResult(
        const ISPResult& result __attribute__((unused)))
{
    // TBD

    return Void();
}

void HalInterface::ClientCallback::writeFile(const CaptureRequest& request)
{
    const uint32_t requestNumber(request.requestNumber);
    //const std::vector<File>& outputFiles(request.outputBuffers.first);
    //const std::vector<ISPBuffer>& outputBuffers(request.outputBuffers.second);
    const auto &outputFileBufferPairs(request.outputFileBufferPairs);

    //const size_t size = outputFiles.size();
    //for (size_t i = 0; i < size; i++)
    for (const auto& [imageFile, outBuffer] : outputFileBufferPairs)
    {
        //const File &imageFile(pair.first);
        const std::string &imageFQN(imageFile.getFullyQualifiedName());
        CAM_LOGD("writing file (%s)", imageFQN.c_str());

        // create working directories
        std::regex rx("[a-z_0-9]+\\.{1}(jpeg|jpg|yuv|raw)$", std::regex::icase);
        std::smatch sm;
        if (std::regex_search(imageFQN, sm, rx))
        {
            CAM_LOGV("matched prefix: %s entire matched expression: %s",
                    sm.prefix().str().c_str(), sm.str(0).c_str());
            int ret = mkdir(sm.prefix().str().c_str(),
                    S_IRWXU | S_IRGRP | S_IXGRP  | S_IROTH | S_IXOTH);
            ASSERT_TRUE((ret == 0) || (errno == EEXIST))
                << "fail to create directory " << sm.prefix()
                << ": " << strerror(errno);
        }
        else
        {
            CAM_LOGD("Cannot open file");
            ASSERT_TRUE(false) << "cannot open file:" << imageFQN;
        }

        // open file
        std::ofstream ofs(imageFQN, std::ofstream::out);
        ASSERT_TRUE(ofs.is_open()) << strerror(errno) << ": " << imageFQN;

        CAM_LOGD("processResult: requestNumber(%u)", requestNumber);

        buffer_handle_t importedBuffer = outBuffer.buffer.getNativeHandle();
        MapperHelper::getInstance().importBuffer(importedBuffer);

        if (imageFile.isJPEG())
        {
            // NOTE: jpeg is a blob format
            // lock buffer for writing
            const IMapper::Rect region { 0, 0, static_cast<int32_t>(imageFile.size), 1u };
            int fence = -1;
            void* data = nullptr;

            MapperHelper::getInstance().lock(importedBuffer,
                    (BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN),
                    region, fence, data);
            ASSERT_NE(data, nullptr) << "lock metadata failed";

            // show CameraBlob
            const char *jpegBuffer = reinterpret_cast<const char*>(data);
            const CameraBlob *transportHeader = reinterpret_cast<const CameraBlob*>(
                    jpegBuffer + imageFile.size - sizeof(CameraBlob));
            CAM_LOGD("%s", toString(*transportHeader).c_str());

            // extract jpeg image from jpeg stream buffer and save it to file
            ofs.write(jpegBuffer, transportHeader->blobSize);
            ofs.close();

            MapperHelper::getInstance().unlock(importedBuffer, fence);
            if (fence >= 0)
                close(fence);
        }
        else if (imageFile.format == static_cast<int>(PixelFormat::RAW16) ||
                 imageFile.format == static_cast<int>(PixelFormat::BLOB))
        {
            // NOTE: jpeg is a blob format
            // lock buffer for writing
            const IMapper::Rect region { 0, 0, static_cast<int32_t>(imageFile.size), 1u };
            int fence = -1;
            void* data = nullptr;

            MapperHelper::getInstance().lock(importedBuffer,
                    (BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN),
                    region, fence, data);
            ASSERT_NE(data, nullptr) << "lock metadata failed";

            // show CameraBlob
            const char *buffer = reinterpret_cast<const char*>(data);
            ofs.write(buffer, imageFile.size);
            ofs.close();

            MapperHelper::getInstance().unlock(importedBuffer, fence);
            if (fence >= 0)
                close(fence);
        }
        else if (imageFile.format == static_cast<int>(PixelFormat::YCBCR_420_888))
        {
            // TODO: handle non-JPEG cases (e.g.YUV) and save result into file system
            //ASSERT_TRUE(false) << "Please implement this part";
            // lock buffer for writing
            const IMapper::Rect region { 0, 0, static_cast<int32_t>(imageFile.width),
                static_cast<int32_t>(imageFile.height) };
            int fence = -1;
            uint8_t padding = 0;

            android_ycbcr layout;
            MapperHelper::getInstance().lockYCbCr_4_0(importedBuffer,
                    static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN),
                    region, fence, layout);

            CAM_LOGD("Write YUV stride(%d, %d), image w/h(%d, %d)",
                      layout.ystride, layout.cstride, imageFile.width, imageFile.height);

            auto yData = static_cast<uint8_t*>(layout.y);
            auto cbData = static_cast<uint8_t*>(layout.cb);
            auto crData = static_cast<uint8_t*>(layout.cr);

            for (uint32_t y = 0; y < imageFile.height; y++)
            {
                for (uint32_t x = 0; x < layout.ystride; x++)
                {
                    if(x < imageFile.width) {
                        ofs << yData[layout.ystride * y + x];
                    } else {
                        ofs << padding;
                    }
                }
            }

            uint32_t uvHeight = imageFile.height >> 1;
            uint32_t uvWidth = imageFile.width >> 1;
            uint32_t uvStride = layout.cstride >> 1;

            // NOTE: if chromaStep is 2, adopt semiplanar arrangement
            //       to access the cb/cr point
            //       (chroma values are interleaved and each chroma
            //       value is one byte).
            //       Otherwise, we adopt planar arrangement
            //       to access the cb/cr point respectively.
            // FIXME: get the actual pixel format to known the UV ordering
            if (layout.chroma_step == 2)
            {
                for (uint32_t y = 0; y < uvHeight; y++) {
                    for (uint32_t x = 0, step = 0; x < uvStride; x++, step += layout.chroma_step) {
                        if(x < uvWidth) {
                            ofs << crData[layout.cstride * y + step];
                            ofs << cbData[layout.cstride * y + step];
                        } else {
                            ofs << padding;
                            ofs << padding;
                        }
                    }
                }
            }

            MapperHelper::getInstance().unlock(importedBuffer, fence);
            if (fence >= 0)
                close(fence);
        }
        else
        {
            ASSERT_TRUE(0)
                << "Unknow format: "
                << static_cast<int>(imageFile.format);
        }

        MapperHelper::getInstance().freeBuffer(importedBuffer);

        // close file
        ofs.close();
    }
}
