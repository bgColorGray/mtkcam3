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

#include "ISPDeviceTest.h"
#define DEBUG_LOG_TAG "isphal_test"
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>

#include "utils/Singleton.h"
#include "utils/AllocatorHelper.h"
#include "utils/Camera3DeviceHelper.h"
#include "utils/JSONContainer.h"
#include "types.h"

#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
#include <system/camera_metadata.h>
#include <mtkisp_metadata.h>

#include <android/hardware/camera/device/3.2/types.h>
#include <android/hardware/graphics/common/1.2/types.h>
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
#include <json/json.h>
#include <sys/stat.h>
#include <random>
#include <thread>

using namespace isphal_test;
using namespace std;
using namespace ::vendor::mediatek::hardware::camera;
using namespace ::vendor::mediatek::hardware::camera::isphal::V1_0;
using ::android::hardware::graphics::common::V1_2::Dataspace;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::common::V1_0::PixelFormat;
using ::android::hardware::graphics::mapper::V4_0::IMapper;
using ::android::hardware::graphics::mapper::V4_0::BufferDescriptor;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;
using ::android::sp;


constexpr uint64_t kProducerUsage(GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN |
        GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN | GRALLOC1_PRODUCER_USAGE_CAMERA);

constexpr uint64_t kConsumerUsage(GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN);

const int32_t kGralloc0Usage = android_convertGralloc1To0Usage(
        kProducerUsage, kConsumerUsage);

extern std::unordered_map<std::string, uint32_t> gIspMetaMap;
extern bool gbWaitData;
extern bool gbReapting;
extern int  gFrameNumForReapting;

template <typename T>
const std::string stringify(const std::vector<T>& v)
{
    std::stringstream ss;

    ss << "{ ";
    // enum type
    for (size_t i = 0; i < v.size(); i++)
    {
        if (i != 0) ss << ", ";
        ss << NSCam::toLiteral(v[i]);
    }
    ss << " }";

    return ss.str();
}

template <>
const std::string stringify(const std::vector<ISPFormat>& v)
{
    std::stringstream ss;

    for (const auto& ispFormat : v)
        ss << toString(ispFormat);

    return ss.str();
}

long getFileSize(const std::string& fileName)
{
    struct stat stat_buf;
    int rc = stat(fileName.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

ISPDeviceTest::ISPDeviceTest()
{
    init();
}

ISPDeviceTest::~ISPDeviceTest()
{
    // close ISP device
    closeISPDevice(mDevice);

    // unlink from death
    unlinkToDeath(mHalInterface);
}

void ISPDeviceTest::init()
{
    // link to death
    linkToDeath(mHalInterface);

    // open ISP device
    openISPDevice(mDevice);
}

int ISPDeviceTest::loadFromFile(hidl_handle &rawHandle, const File& file, const PixelFormat format)
{
    buffer_handle_t importedBuffer = rawHandle.getNativeHandle();
    MapperHelper::getInstance().importBuffer(importedBuffer);
    int fence = -1;

    std::ifstream ifs;
    ifs.open(file.getFullyQualifiedName(), (std::ios::in | std::ios::binary));

    if (gbWaitData == true)
    {
        size_t size = 0;
        if (file.isYUV())
            size = file.width * file.height * 1.5;
        else if (file.isRAW())
            size = file.width * file.height * 2;

        CAM_LOGD("use waiting flow + %s", file.getFullyQualifiedName().c_str());
        int timeout = 10000;
        int waitTime = 25;

        while (file.isImage() && getFileSize(file.getFullyQualifiedName()) != size && timeout > 0 ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
            timeout -= waitTime;
        }
        while (!ifs.is_open() && timeout > 0 ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
            timeout -= waitTime;
            ifs.open(file.getFullyQualifiedName(), (std::ios::in | std::ios::binary));
        }

        if (timeout <= 0)
            CAM_LOGE("wait input data %s timeout !", file.getFullyQualifiedName().c_str());

        CAM_LOGD("use waiting flow -", file.getFullyQualifiedName().c_str());
    }

    CAM_LOGD("Jovi getFileSize: %ld", getFileSize(file.getFullyQualifiedName()));

    EXPECT_TRUE(ifs.is_open()) << strerror(errno) << ": " <<
        file.getFullyQualifiedName();

    if (ifs.is_open() != true)
        return -1;

    // lock buffer for writing
    if(format == PixelFormat::YCBCR_420_888) {
        const IMapper::Rect region { 0, 0, static_cast<int32_t>(file.width),
            static_cast<int32_t>(file.height) };

        android_ycbcr layout = {
          .y = nullptr,
          .cb = nullptr,
          .cr = nullptr,
          .ystride = 0,
          .cstride = 0,
          .chroma_step = 0
        };

        MapperHelper::getInstance().lockYCbCr_4_0(importedBuffer,
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN),
                region, fence, layout);

        CAM_LOGD("Load YUV stride(%zu, %zu)", layout.ystride, layout.cstride);

        auto yData = static_cast<uint8_t*>(layout.y);
        auto cbData = static_cast<uint8_t*>(layout.cb);
        auto crData = static_cast<uint8_t*>(layout.cr);
        uint8_t padding = 0;

        char value;
        for (uint32_t y = 0; y < file.height; y++)
        {
            for (uint32_t x = 0; x < layout.ystride; x++)
            {
                if (x < file.width) {
                    if (ifs.get(value))
                        yData[layout.ystride * y + x] = value;
                    else
                        EXPECT_TRUE(false) << "cannot read more data from file "
                            << file.getFullyQualifiedName();
                }
                else {
                    yData[layout.ystride * y + x] = padding;
                }
            }
        }

        uint32_t uvHeight = file.height >> 1;
        uint32_t uvWidth = file.width >> 1;
        uint32_t uvStride = layout.ystride >> 1;

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
                    if (x < uvWidth) {
                        if (ifs.get(value))
                            crData[layout.cstride * y + step] = value;
                        else
                            EXPECT_TRUE(false) << "cannot read more data from file "
                                << file.getFullyQualifiedName();
                    }
                    else {
                        crData[layout.cstride * y + step] = padding;
                    }

                    if (x < uvWidth) {
                        if (ifs.get(value))
                            cbData[layout.cstride * y + step] = value;
                        else
                            EXPECT_TRUE(false) << "cannot read more data from file "
                                << file.getFullyQualifiedName();
                    }
                    else {
                        cbData[layout.cstride * y + step] = padding;
                    }
                }
            }
        }
        else
        {
            for (uint32_t y = 0; y < uvHeight; y++) {
                for (uint32_t x = 0, step = 0; x < uvStride; x++, step+=layout.chroma_step) {
                    if (x < uvWidth) {
                        if (ifs.get(value))
                            cbData[layout.cstride * y + step] = value;
                        else
                            EXPECT_TRUE(false) << "cannot read more data from file "
                                << file.getFullyQualifiedName();
                    }
                    else {
                        cbData[layout.cstride * y + step] = padding;
                    }
                }
            }

            for (uint32_t y = 0; y < uvHeight; y++) {
                for (uint32_t x = 0, step = 0; x < uvStride; x++, step+=layout.chroma_step) {
                    if (x < uvWidth) {
                        if (ifs.get(value))
                            crData[layout.cstride * y + step] = value;
                        else
                            EXPECT_TRUE(false) << "cannot read more data from file "
                                << file.getFullyQualifiedName();
                    }
                    else {
                        crData[layout.cstride * y + step] = padding;
                    }
                }
            }
        }
    }
    else if(format == PixelFormat::RAW16) {
        // lock buffer for writing
        const IMapper::Rect region { 0, 0, static_cast<int32_t>(file.width),
            static_cast<int32_t>(file.height) };
        int fence = -1;
        void* data = nullptr;

        MapperHelper::getInstance().lock(importedBuffer,
                (BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN),
                region, fence, data);
        EXPECT_NE(data, nullptr) << "lock image failed";

        // Load raw image from file
        char value = 0;
        uint8_t *addr = static_cast<uint8_t *>(data);
        int32_t writedSize = 0;
        while (writedSize < file.size) {
            ifs.get(value);
            *addr = value;
            addr++;
            writedSize++;
         }
    }
    else if(format == PixelFormat::BLOB || format == PixelFormat::IMPLEMENTATION_DEFINED) {
        // lock buffer for writing
        const IMapper::Rect region { 0, 0, static_cast<int32_t>(file.size), 1 };
        int fence = -1;
        void* data = nullptr;

        MapperHelper::getInstance().lock(importedBuffer,
                (BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN),
                region, fence, data);
        EXPECT_NE(data, nullptr) << "lock image failed";

        // Load raw image from file
        char value = 0;
        uint8_t *addr = static_cast<uint8_t *>(data);
        int32_t writedSize = 0;
        while (writedSize < file.size) {
            ifs.get(value);
            *addr = value;
            addr++;
            writedSize++;
         }

        CAM_LOGD("writedSize(%d)", writedSize);
    }
    else {
        ifs.close();
        EXPECT_TRUE(false) << "format not support loading";
        return 0;
    }

    ifs.close();

    MapperHelper::getInstance().unlock(importedBuffer, fence);
    if (fence >= 0)
        close(fence);

    MapperHelper::getInstance().freeBuffer(importedBuffer);
    return 0;
}

void ISPDeviceTest::jsonVerifier(bool printerEnabled)
{
    CAM_LOGD("jsonVerifier +");

    const std::string& jsonFileName(JSONContainer::kTestCaseSettingFile);

    // test suite and test names
    const std::string testSuiteName(TestBase::getTestSuiteName());
    const std::string testName(TestBase::getTestName());

    const Json::Value& testSuite =
        JSONContainer::getInstance().getTestCaseSetting()[testSuiteName.c_str()];
    ASSERT_FALSE(testSuite.isNull())
        << "Test suite " << testSuiteName << " does not exist in " << jsonFileName;
    const Json::Value& test = testSuite[testName.c_str()];
    ASSERT_FALSE(test.isNull())
        << "Test " << testName << " does not exist in " << jsonFileName;

    mTestSuite = ValueNamePair(testSuiteName, testSuite);
    mTest = ValueNamePair(testName, test);

    // camera device name
    const std::string cameraIdName("cameraId");

    const Json::Value& cameraId = mTest.value[cameraIdName.c_str()];
    ASSERT_FALSE(cameraId.isNull())
        << cameraIdName << " does not exist in " << jsonFileName;

    Camera3DeviceHelper::getInstance().getCameraDeviceName(
            cameraId.asString(), mCameraDeviceName);

    CAM_LOGD("cameraIdName(%s) cameraDeviceName(%s)",
            cameraId.asString().c_str(), mCameraDeviceName.c_str());

    // root directory and total requests
    const std::string rootDirectoryName("rootDirectory");
    const std::string totalRequestsName("totalRequests");

    const Json::Value& rootDirectory = mTest.value[rootDirectoryName.c_str()];
    ASSERT_FALSE(rootDirectory.isNull())
        << rootDirectoryName << " does not exist in " << jsonFileName;
    const Json::Value& totalRequests = mTest.value[totalRequestsName.c_str()];
    ASSERT_FALSE(totalRequests.isNull())
        << totalRequestsName << " does not exist in " << jsonFileName;

    mRootDirectory = ValueNamePair(rootDirectoryName, rootDirectory);
    mTotalRequests = ValueNamePair(totalRequestsName, totalRequests);

    CAM_LOGD("%s(%s) %s(%u)",
            rootDirectoryName.c_str(), mRootDirectory.value.asString().c_str(),
            totalRequestsName.c_str(), mTotalRequests.value.asUInt());

    // input/output files
    const std::string inputFilesName("inputFiles");
    const std::string outputFilesName("outputFiles");

    const Json::Value& inputFiles = mTest.value[inputFilesName.c_str()];
    ASSERT_FALSE(inputFiles.isNull())
        << inputFilesName << " does not exist in " << jsonFileName;
    Json::Value& outputFiles = mTest.value[outputFilesName.c_str()];
    ASSERT_FALSE(outputFiles.isNull())
        << outputFilesName << " does not exist in " << jsonFileName;

    // construct file structure
    auto constructFileList = [this](const Json::Value& files , std::vector<File>& fileList)
    {
        for (Json::ArrayIndex i = 0; i < files.size(); i++)
        {
            const auto& inputFile(files[i]);
            std::vector<Json::UInt> rowStrides {
                inputFile["rowStrides"][0].asUInt(),
                inputFile["rowStrides"][1].asUInt(),
                inputFile["rowStrides"][2].asUInt()};

            File file(mRootDirectory.value.asString(), inputFile["name"].asString(),
                    inputFile["width"].asUInt(), inputFile["height"].asUInt(),
                    inputFile["format"].asInt(), std::move(rowStrides),
                    inputFile["size"].asUInt());

            // if the file size in JSON file is not set,
            // we set it queried from the file system
            if (file.size == 0)
            {
                const std::string fqn(file.getFullyQualifiedName());
                const auto fileSize = getFileSize(fqn);
                if (fileSize != -1)
                {
                    CAM_LOGW("[%s] fize size in JSON is 0, we set it queried " \
                            "from the file system(%ld)",
                            fqn.c_str(), fileSize);
                    file.size = fileSize;
                }
            }

            if(file.isTuning())
            {
                // tuning file format is always BLOB;
                file.format = static_cast<int32_t>(PixelFormat::BLOB);
            }

            fileList.push_back(std::move(file));
        }
    };

    constructFileList(inputFiles, mInputFiles);
    constructFileList(outputFiles, mOutputFiles);

    // NOTE: image and metadata are paired per input buffer
    ASSERT_TRUE(mInputFiles.size() > 0)
        << "the number of intput files must be greater than 0";
    ASSERT_TRUE(mOutputFiles.size() > 0)
        << "the number of output files must be greater than 0";

    // set up outputFile data which are unfilled in JSON
    {
        File firstInputImage = mInputFiles[0];
        for(auto && inputImage : mInputFiles)
        {
            if(inputImage.isImage())
            {
                firstInputImage = inputImage;
                break;
            }
        }

        for (auto&& file : mOutputFiles)
        {
            if( !file.isImage() )
                continue;

            File &image = file;

            if (image.isYUV()) {
                image.width  = image.width ? image.width : firstInputImage.width ;
                image.height = image.height ? image.height : firstInputImage.height;
                image.size = image.size ? image.size : firstInputImage.size;
                image.format = (image.format) ? image.format : static_cast<int32_t>(PixelFormat::YCBCR_420_888);

                bool hasRowStride = false;
                for(const auto& stride : image.rowStride)
                {
                    if(stride != 0)
                    {
                        hasRowStride = true;
                        break;
                    }
                }
                image.rowStride = hasRowStride ? image.rowStride : firstInputImage.rowStride;
            }
            else if (image.isJPEG()) {
                // NOTE: JPEG's width and height is the same as the input ones

                // get the first input image's width and height
                image.width = firstInputImage.width;
                image.height = firstInputImage.height;
                image.format = static_cast<int32_t>(PixelFormat::BLOB);

                if (mCameraDeviceName.empty())
                {
                    CAM_LOGW("camera device name is empty");
                    continue;
                }

                // get the maximum size in bytes for the compressed JPEG buffer
                // TODO: check if need to align Camera3Device::getJpegBufferSiz() in
                // frameworks/av/services/camera/libcameraservice/device3/Camera3Device.cpp
                CameraMetadata cameraMetadata;
                Camera3DeviceHelper::getInstance().getCameraCharacteristics(
                        mCameraDeviceName, cameraMetadata);

                camera_metadata_t *staticMeta = clone_camera_metadata(
                        reinterpret_cast<const camera_metadata_t*>(cameraMetadata.data()));
                ASSERT_NE(nullptr, staticMeta) << "clone camera metadata failed";
                camera_metadata_ro_entry entry;
                auto status = find_camera_metadata_ro_entry(staticMeta,
                        ANDROID_JPEG_MAX_SIZE, &entry);
                ASSERT_TRUE((status == 0) && (entry.count > 0)) <<
                    mCameraDeviceName << ": can't find maximum JPEG size in static metadata";
                // update jpeg's size
                image.size = entry.data.i32[0] * (property_get_bool("vendor.isphidl.heif.enable", 0) ?
                        2 : 1);
                CAM_LOGD("image.size:%zu", image.size);
                free_camera_metadata(staticMeta);
            }
        }
    }

    auto printFileList = [this](const std::vector<File>& files, Direction direction)
    {
        for (const auto& file : files)
            CAM_LOGD("testName(%s) %sFileName(%s) width(%u) height(%u) format(%s) "
                    "rowStride[ %d %d %d ] size(%u)",
                    mTest.name.c_str(),
                    direction == Direction::IN ? "input" : "output",
                    file.getFullyQualifiedName().c_str(),
                    file.width, file.height, toString(file.getPixelFormat()).c_str(),
                    file.rowStride[0], file.rowStride[1] ,file.rowStride[2],
                    file.size);
    };

    if (printerEnabled)
    {
        printFileList(mInputFiles, Direction::IN);
        printFileList(mOutputFiles, Direction::OUT);
    }

    // stream configuration
    const std::string streamConfigurationName("ispStreamConfiguration");
    const std::string inputStreamsName("inputStreams");
    const std::string outputStreamsName("outputStreams");
    const std::string metadataName("metadata");

    const Json::Value& streamConfiguration =
        mTest.value[streamConfigurationName.c_str()];
    ASSERT_FALSE(streamConfiguration.isNull())
        << streamConfigurationName << " does not exist in " << jsonFileName;
    const Json::Value& inputStreams =
        streamConfiguration[inputStreamsName.c_str()];
    ASSERT_FALSE(inputStreams.isNull())
        << inputStreamsName << " does not exist in " << jsonFileName;
    const Json::Value& outputStreams =
        streamConfiguration[outputStreamsName.c_str()];
    ASSERT_FALSE(outputStreams.isNull())
        << outputStreamsName << " does not exist in " << jsonFileName;
    const Json::Value& metadata =
        streamConfiguration[metadataName.c_str()];
    ASSERT_FALSE(metadata.isNull())
        << metadataName << " does not exist in " << jsonFileName;

    auto constructStreams = [](const Json::Value& values, std::vector<StreamProfile>& list)
    {
        const std::string sId("id");
        const std::string sFile("file");
        const std::string sRealFormat("realFormat");
        for (Json::ArrayIndex i = 0; i < values.size(); i++)
        {
            StreamProfile profile;
            const Json::Value& id = values[i][sId.c_str()];
            ASSERT_FALSE(id.isNull())
                << sId << " does not exist in " << jsonFileName;

            profile.streamId = id.asUInt64();
            profile.fileName = values[i][sFile.c_str()].asString();
            profile.realFormat = values[i][sRealFormat.c_str()].asString();
            list.push_back(profile);
        }
    };

    auto constructMeta = [](const Json::Value& srcMeta, std::vector<Metadata> &metaMap)
    {
        CAM_LOGD("constructMeta size(%d)", srcMeta.size());
        for(auto && iter : gIspMetaMap) {
            const auto& tagString(srcMeta[iter.first.c_str()]);
            if(tagString.isNull()) {
                continue;
            }

            Metadata ispMeta;
            ispMeta.name = iter.first;
            ispMeta.tag = static_cast<uint32_t>(iter.second);

            int type = tagString["type"].asInt();

            if(type == TYPE_MBOOL)  //because Test APK can't distinguish byte & bool, so we need to use additional type for bool
            {
                type = TYPE_MUINT8;
            }
            ispMeta.type = static_cast<Metadata::Type>(type);

            // array : only support int32 so far
            auto values = tagString["value"];
            if( type == TYPE_MINT32 && values.isArray() )
            {
                std::stringstream dataSs;

                for (Json::ArrayIndex i = 0 ; i < values.size() ; i++)
                {
                    dataSs << values[i].asString() << " ";
                    ispMeta.data.i32[i] = values[i].asInt();
                }
                ispMeta.size = values.size();

                CAM_LOGD("Tag name(%s, 0x%x), type(%d), size(%d), value( %s)"
                    , ispMeta.name.c_str(), ispMeta.tag, ispMeta.type, ispMeta.size, dataSs.str().c_str());
            }
            else
            {
                // TODO: remove hard code here
                ispMeta.size = 1;
                ispMeta.data.i32[0] = values.asInt();

                CAM_LOGD("Tag name(%s, 0x%x), value(%d), type(%d)", ispMeta.name.c_str(), ispMeta.tag
                    , ispMeta.data.i32[0], ispMeta.type);
            }

            metaMap.push_back(std::move(ispMeta));
        }
    };

    constructStreams(inputStreams, mStreamConfiguration.inputStreams);
    constructStreams(outputStreams, mStreamConfiguration.outputStreams);
    constructMeta(metadata, mStreamConfiguration.vMetadata);

    if (printerEnabled)
    {
        auto printStreamList = [](const std::vector<StreamProfile>& profiles, Direction direction)
        {
            std::stringstream ss;
            ss << (direction == Direction::IN ? "input" : "output") << " stream = {";
            for (const auto& profile : profiles) {
                ss << " id("          << profile.streamId   << ")"
                   << " fileName("    << profile.fileName   << ")"
                   << " realFormat("  << profile.realFormat << ")";
            }
            ss << " }";
            CAM_LOGD("%s", ss.str().c_str());
        };

        printStreamList(mStreamConfiguration.inputStreams, Direction::IN);
        printStreamList(mStreamConfiguration.outputStreams, Direction::OUT);
    }

    // request configuration
    const std::string requestConfigurationName("ispRequestConfiguration");
    const std::string requestNumberName("requestNumber");
    const std::string inputBuffersName("inputBuffers");
    const std::string outputBuffersName("outputBuffers");
    const std::string requestMetadataName("metadata");

    const Json::Value& requestConfiguration =
        mTest.value[requestConfigurationName.c_str()];
    ASSERT_FALSE(requestConfiguration.isNull())
        << requestConfigurationName << " does not exist in " << jsonFileName;

    auto constructStreamFilePairsMap = [&](const Json::Value& values,
                                          Direction direction,
                                          std::vector<RequestConfiguration::ReqBuffer> &mapOut)
    {
        for (const auto& value : values){
            RequestConfiguration::ReqBuffer rb;
            rb.streamId = value["id"].asUInt64();
            rb.imageName = (!value["image"].isNull()) ? value["image"].asString() : "";
            rb.tuningName = (!value["tuning"].isNull()) ? value["tuning"].asString() : "";
            if(direction == Direction::IN)
                rb.useGarbage = (!value["useGarbage"].isNull()) ? value["useGarbage"].asInt() : 0;
            else
                rb.useGarbage = 1;
            mapOut.push_back(rb);
        }
    };

    for (Json::ArrayIndex i = 0; i < requestConfiguration.size(); i++)
    {
        auto procPackConfig = requestConfiguration[i];
        std::vector<RequestConfiguration> reqsConfig;

        for(Json::ArrayIndex j = 0; j < procPackConfig.size(); j++) {

            const Json::Value& _config = procPackConfig[j];

            ASSERT_FALSE(_config.isNull())
                << "[" << j << "]" << " procPackConfig does not exist in " << jsonFileName;
            RequestConfiguration config;

            const Json::Value& requestNumber = _config[requestNumberName.c_str()];
            ASSERT_FALSE(requestNumber.isNull())
                << requestNumberName << " does not exist in " << jsonFileName;
            config.requestNumber = requestNumber.asUInt();

            const Json::Value& inputBuffers = _config[inputBuffersName.c_str()];
            ASSERT_FALSE(inputBuffers.isNull())
                << inputBuffersName << " does not exist in " << jsonFileName;
            constructStreamFilePairsMap(inputBuffers, Direction::IN, config.vInBuffers);

            const Json::Value& outputBuffers = _config[outputBuffersName.c_str()];
            ASSERT_FALSE(outputBuffers.isNull())
                << outputBuffersName << " does not exist in " << jsonFileName;
            constructStreamFilePairsMap(outputBuffers, Direction::OUT, config.vOutBuffers);

            const Json::Value& requestMetadata = _config[requestMetadataName.c_str()];
            ASSERT_FALSE(requestMetadata.isNull())
                << requestMetadataName << " does not exist in " << jsonFileName;

            constructMeta(requestMetadata, config.vMetadata);

            reqsConfig.push_back(std::move(config));
        }

        // NOTE: the number of requestion configurations MUST be the same as that
        //       of total requests
        ASSERT_EQ(getTotalRequests(), reqsConfig.size())
            << "the number of total requests is not the same " \
               "as that of request configurations";

        mProcessPackage.push_back(std::move(reqsConfig));
    }

    // print all reqs
    auto printRequestConfiguration = [](const RequestConfiguration& config)
    {
        CAM_LOGD("requestNumber(%d)", config.requestNumber);

        std::stringstream ss;
        for(const auto& ispMeta : config.vMetadata) {
            ss << "Tag name(" << ispMeta.name << ", "
               << "0x" << setfill('0') << setw(8) << right << hex << ispMeta.tag << ")"
               << ", type(" << (int32_t)ispMeta.type << ")"
               << ", size(" << ispMeta.size << ")";
        }
        CAM_LOGD("%s", ss.str().c_str());

        ss.str(std::string());
        ss << "input files = {";
        for(const auto& rb : config.vInBuffers) {
            ss << " streamId[" << rb.streamId <<"]"
               << " image(" << rb.imageName.c_str() << ")"
               << " tuning(" << rb.tuningName.c_str() << ")"
               << " useGarbage(" << rb.useGarbage << ")";
        }
        ss << " }";
        CAM_LOGD("%s", ss.str().c_str());

        ss.str(std::string());
        ss << "output files = {";
        for(const auto& rb : config.vOutBuffers) {
            ss << " streamId[" << rb.streamId <<"]"
               << " image(" << rb.imageName.c_str() <<")"
               << " tuning(" << rb.tuningName.c_str() << ")"
               << " useGarbage(" << rb.useGarbage << ")";
        }
        ss << " }";
        CAM_LOGD("%s", ss.str().c_str());
    };

    if (printerEnabled)
    {
        for(std::size_t j = 0; j < mProcessPackage.size(); j++)
        {
            CAM_LOGD("[ReqPackage %zu]", j);
            auto &reqs = mProcessPackage[j];

            for (std::size_t i = 0; i < reqs.size(); i++)
            {
                CAM_LOGD("[requestConfiguration %zu]", i);
                printRequestConfiguration(reqs[i]);
            }
        }
    }

    CAM_LOGD("jsonVerifier -");
}

void ISPDeviceTest::formatPrinter(Direction direction,
        isphal::V1_0::Status status,
        const std::vector<ISPFormat>& supportedFormats) const
{
    const std::string directionString(
            direction == Direction::IN ? "input" : "output");

    EXPECT_EQ(status, isphal::V1_0::Status::OK) << "get supported " << directionString
        << " format failed(" << toString(status) << ")";
    EXPECT_TRUE(supportedFormats.size() > 0)
        << "no supported " << directionString << " format";
    CAM_LOGD_IF(status == isphal::V1_0::Status::OK, "supported %s format = %s",
            directionString.c_str(), stringify<ISPFormat>(supportedFormats).c_str());
}

void ISPDeviceTest::featureTagInfoPrinter(
        const std::vector<ISPFeatureTagInfo>& tagInfos) const
{
    for (const auto& tagInfo : tagInfos)
        CAM_LOGD("%s", toString(tagInfo).c_str());
}

void ISPDeviceTest::initEmptyMetadata(ISPMetadata& metadata, int count)
{
    std::vector<uint8_t> emptyMetadata;
    ASSERT_EQ(initial_isp_metadata_vector(emptyMetadata, count), 0);
    metadata = emptyMetadata;
}

int ISPDeviceTest::writeMeta(Metadata src, ISPMetadata *metadata)
{
    uint32_t type = (uint32_t)src.type;
    uint32_t tag = src.tag;
    int size = src.size;
    auto data = src.data;

    int32_t dataChunk[8];
    std::stringstream dataSs;

    if(type == TYPE_MINT32)
    {
        for(int i = 0 ; i < size ; i++)
        {
            dataSs << (int)data.i32[i] << " ";
            dataChunk[i] = data.i32[i];
        }
    }
    else if (type == TYPE_MUINT8 || type == TYPE_MBOOL)
    {
        dataSs << data.i32[0] << " ";
        dataChunk[0] = data.i32[0];
    }
    else if (type == TYPE_MINT64)
    {
        dataSs << data.i64[0] << " ";
        dataChunk[0] = data.i64[0];
    }
    else
    {
        dataSs << data.i32[0] << " ";
        dataChunk[0] = data.i32[0];
    }

    writeMeta(tag, type, (void*)&dataChunk, size, metadata);
    return 0;
}
int ISPDeviceTest::writeMeta(uint32_t tag, uint32_t type, void* value, int size, ISPMetadata *metadata)
{
    auto err = add_isp_metadata_entry(*(reinterpret_cast<std::vector<uint8_t>*>(metadata)), tag, type, value,
                           size * sizeof(int), nullptr);
    if (err)
    {
        CAM_LOGW("tag[0x%x] is not support with err[%d], maybe data size is too large", tag, err);
    }

    return 0;
}

void ISPDeviceTest::configIsp()
{
    ConfigStreamParam params;
    // in
    for(const auto &profile : mStreamConfiguration.inputStreams){
        StreamProfileParam profileParam = {
            .streamId = profile.streamId,
            .file = findFile(profile.fileName),
            .realFormat = profile.realFormat,
        };
        params.inputStreams.push_back(profileParam);
    }
    // out
    for(const auto &profile : mStreamConfiguration.outputStreams){
        StreamProfileParam profileParam = {
            .streamId = profile.streamId,
            .file = findFile(profile.fileName),
            .realFormat = profile.realFormat,
        };
        params.outputStreams.push_back(profileParam);
    }
    params.vMetadata = mStreamConfiguration.vMetadata;

    //
    ISPStreamConfiguration hidlConfiguration;
    constructHidlConfiguration(hidlConfiguration /*out*/, params);

    // configure ISP device
    Return<isphal::V1_0::Status> ret = mDevice->configureISPDevice(
            hidlConfiguration, mHalInterface->getICallback());
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in configureISPDevice from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "configureISPDevice failed";
}

void ISPDeviceTest::constructHidlConfiguration(ISPStreamConfiguration &hidlConfiguration /*out*/, ConfigStreamParam &params)
{
    auto constructISPStreamsByFile = [&](Direction direction, std::vector<StreamProfileParam> vParam)
    {
        std::vector<ISPStream> ispStreams;
        for(const auto& param : vParam)
        {
            auto streamId = param.streamId;
            const auto &file = param.file;

            ISPStream ispStream {
                .id = streamId,
                .size   = file.size,
                .width  = file.width,
                .height = file.height,
                .format = {
                    file.getPixelFormat(),
                    static_cast<DataspaceFlags>(!file.isJPEG() ? Dataspace::UNKNOWN : property_get_bool("vendor.isphidl.heif.enable", 0) ? Dataspace::HEIF : Dataspace::V0_JFIF),
                    //static_cast<DataspaceFlags>(file.isJPEG() ? Dataspace::V0_JFIF : Dataspace::UNKNOWN),
                },
                .usage = direction == Direction::IN ?
                    GRALLOC1_CONSUMER_USAGE_NONE : kConsumerUsage,
                .transform = 0
            };

            // query stride
            uint32_t allocStrideRes = 0;
            AllocatorHelper::getInstance().queryStride(
                ispStream.width, ispStream.height, ispStream.usage, file.getPixelFormat(),
                    &allocStrideRes);

            ispStream.stride = file.isJPEG() ?
                hidl_vec<uint32_t>({ file.size, 0, 0 }) :
                hidl_vec<uint32_t>({ allocStrideRes, 0, 0 });

            ispStreams.push_back(std::move(ispStream));
        }

        return ispStreams;
    };

    hidlConfiguration.InputStreams
        = constructISPStreamsByFile(Direction::IN, params.inputStreams);
    hidlConfiguration.OutputStreams
        = constructISPStreamsByFile(Direction::OUT, params.outputStreams);

    // write metadata
    ISPMetadata configureParams;
    initEmptyMetadata(configureParams);
    for(const auto& srcmeta : params.vMetadata)
        writeMeta(srcmeta, &configureParams);

    hidlConfiguration.ConfigureParams = configureParams;

    CAM_LOGD("[Input Streams]");
    for (const auto& inputStream : hidlConfiguration.InputStreams)
        CAM_LOGD("%s", toString(inputStream).c_str());
    CAM_LOGD("[Output Streams]");
    for (const auto& outputStream : hidlConfiguration.OutputStreams)
        CAM_LOGD("%s", toString(outputStream).c_str());
}

int ISPDeviceTest::buildIspRequests(std::vector<ISPRequest> &out, std::vector<RequestConfiguration> &vInReq)
{
    auto findFile = [&](std::vector<File> v, std::string filename)
    {
        for(auto& f : v)
        {
            if(filename.compare(f.fileName) == 0)
                return f;
        }
        return File();
    };

    // build BuildRequestParams from vInReq
    std::vector<BuildRequestParam> requestParams;
    for (const auto& requestConfig : vInReq)
    {
        BuildRequestParam reqParam;
        reqParam.reqNum = gbReapting ? gFrameNumForReapting++ : requestConfig.requestNumber;
        reqParam.vMetadata = requestConfig.vMetadata;

        // build buffer params
        for(const auto& rb : requestConfig.vInBuffers) {
            auto streamId = rb.streamId;
            auto imageFile = findFile(mInputFiles, rb.imageName);
            auto tuningFile = findFile(mInputFiles, rb.tuningName);

            reqParam.bufferParams.push_back(
                AllocRequestBufferParam {
                    .direction = Direction::IN,
                    .streamId = streamId,
                    .imageFile = imageFile,
                    .tuningFile = tuningFile,
                    .useGarbage = rb.useGarbage,
            });
        }
        for(const auto& rb : requestConfig.vOutBuffers) {
            auto streamId = rb.streamId;
            auto imageFile = findFile(mOutputFiles, rb.imageName);
            auto tuningFile = findFile(mOutputFiles, rb.tuningName);

            reqParam.bufferParams.push_back(
                AllocRequestBufferParam{
                    .direction = Direction::OUT,
                    .streamId = streamId,
                    .imageFile = imageFile,
                    .tuningFile = tuningFile,
                    .useGarbage = rb.useGarbage,
            });
        }

        requestParams.push_back(reqParam);
    }

    if (buildIspRequests(out, requestParams))
        return -1;
    return 0;
}

int ISPDeviceTest::buildIspRequests(std::vector<ISPRequest> &out, std::vector<BuildRequestParam> &params)
{
    for(const auto& param : params)
    {
        ISPRequest request;
        if (buildIspRequestAndRecord(request, param))
            return -1;
        out.push_back(std::move(request));
    }
    return 0;
}

int ISPDeviceTest::buildIspRequestAndRecord(ISPRequest &out, const BuildRequestParam &param)
{
    CAM_LOGD("construct ISP request %" PRId64 "", param.reqNum);

    std::vector<ISPBuffer> ins;
    std::vector<ISPBuffer> outs;
    HalInterface::FileBufferPairs inputFileBuffersPairs;
    HalInterface::FileBufferPairs outputFileBuffersPairs;

    for(const auto& bufferParam : param.bufferParams)
    {
        ISPBuffer buffer;
        //
        if (allocIspRequestBuffer(buffer, bufferParam))
            return -1;
        //
        if(bufferParam.direction == Direction::IN) {
            ins.push_back(buffer);
            inputFileBuffersPairs.push_back(std::make_pair(bufferParam.imageFile, buffer));
        }
        else {
            outs.push_back(buffer);
            outputFileBuffersPairs.push_back(std::make_pair(bufferParam.imageFile, buffer));
        }
    }

    // construct metadata
    ISPMetadata settings;
    initEmptyMetadata(settings);
    for(const auto& srcmeta : param.vMetadata)
        writeMeta(srcmeta, &settings);

    out.reqNumber = param.reqNum;
    out.settings = settings;
    out.InputBuffers = ins;
    out.OutputBuffers = outs;

    mHalInterface->recordISPRequest(param.reqNum,
        std::move(inputFileBuffersPairs), std::move(outputFileBuffersPairs));

    return 0;
}

int ISPDeviceTest::allocIspRequestBuffer(ISPBuffer &out, const AllocRequestBufferParam &param)
{
    auto &imageFile = param.imageFile;
    auto &tuningFile = param.tuningFile;
    hidl_handle image_handle;
    hidl_handle metadata_handle = hidl_handle();

    if (allocAndLoad(image_handle, imageFile, param.direction, !param.useGarbage))
        return -1;

    // NOTE: Jpeg no need tuning data, and isn't garbage if Direction::IN
    if( tuningFile.fileName.find(".tuning") != std::string::npos && !imageFile.isJPEG() )
    {
        if(tuningFile.format != static_cast<int32_t>(PixelFormat::BLOB))
        {
            CAM_LOGW("tuning file should be BLOB !");
        }

        if (allocAndLoad(metadata_handle, tuningFile, param.direction,
                       param.direction == Direction::IN))
            return -1;
    }

    out.id = param.streamId;
    out.buffer = image_handle;
    out.tuningData = metadata_handle;
    return 0;
}

int ISPDeviceTest::allocAndLoad(hidl_handle &handle, const File &file, Direction direction, bool needLoad)
{
    uint32_t allocStrideRes = 0;
    bool useBlob = (file.isJPEG() ||
                    (file.getPixelFormat() == PixelFormat::BLOB) ||
                    file.fileName.find(".tuning") != std::string::npos
                   );
    auto w = (useBlob) ? file.size : file.width;
    auto h = (useBlob) ? 1 : file.height;
    auto format = (useBlob) ? PixelFormat::BLOB : file.getPixelFormat();

    AllocatorHelper::getInstance().allocateGraphicBuffer(
                w, h, kGralloc0Usage, format,
                &handle, &allocStrideRes);

    CAM_LOGD("%s file(%s)(%s), w(%d) h(%d) format(%d) allocStrideRes(%u)",
                direction == Direction::IN ? "input" : "output",
                file.fileName.c_str(),
                toString(handle).c_str(),
                w, h, static_cast<int32_t>(format),
                allocStrideRes);

    if(needLoad)
        loadFromFile(handle, file, file.getPixelFormat());

    return 0;
};

void ISPDeviceTest::Run(ISP_Test_Ops ops)
{
    jsonVerifier();
    //
    if(ops.fOnBeforeConfig)
        ops.fOnBeforeConfig(mInputFiles, mOutputFiles);
    //
    configIsp();
    //
    if(ops.fOnBeforeRequest)
        ops.fOnBeforeRequest(mProcessPackage);
    //

    int testcount = 0;
    do {
    for(int i = 0 ; i < mProcessPackage.size() ; i++)
    {
        std::vector<RequestConfiguration> reqConfig
            = mProcessPackage[i];
        std::vector<ISPRequest> requests;
        if (buildIspRequests(requests/*out*/, reqConfig/*in*/)) {
            CAM_LOGE("buildIspRequests failed !");
            return;
        }

        CAM_LOGD("processImgRequest +");
        Return<isphal::V1_0::Status> ret = mDevice->processImgRequest(requests);
        CAM_LOGD("processImgRequest -");
        ASSERT_TRUE(ret.isOk())
            << "Transaction error in processImgRequest from ISPDevice: "
            << ret.description().c_str();
        ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "processImgRequest failed";

        //
        const int timeout = SINGLE_TEST_TIMEOUT;
        ASSERT_EQ(::android::OK, mHalInterface->waitResult(timeout))
            << "did not receive the result for " << timeout << " milliseconds";

        //
        if(ops.fOnAfterProcessOneReqPackage
           && requests.size() == reqConfig.size()) {
            ops.fOnAfterProcessOneReqPackage(requests, reqConfig);
        }
        else {
            CAM_LOGE("requests size: %zu != reqConfig size: %zu, skip."
                ,requests.size(), reqConfig.size());
        }
    }
    CAM_LOGD("Jovi testcount : %d", testcount);
    testcount++;
    } while (gbReapting);
}

void ISPDeviceTest::YUV2JPEG(ISPBuffer &yuvBuffer,File &yuvFile)
{
#define STREAMID_Y2J_INPUT  100
#define STREAMID_Y2J_OUTPUT 101

    CAM_LOGD("YUV2JPEG +");

    if (mCameraDeviceName.empty())
    {
        CAM_LOGW("camera device name is empty");
    }

    // get the maximum size in bytes for the compressed JPEG buffer
    // TODO: check if need to align Camera3Device::getJpegBufferSiz() in
    // frameworks/av/services/camera/libcameraservice/device3/Camera3Device.cpp
    CameraMetadata cameraMetadata;
    Camera3DeviceHelper::getInstance().getCameraCharacteristics(
            mCameraDeviceName, cameraMetadata);

    camera_metadata_t *staticMeta = clone_camera_metadata(
            reinterpret_cast<const camera_metadata_t*>(cameraMetadata.data()));
    ASSERT_NE(nullptr, staticMeta) << "clone camera metadata failed";
    camera_metadata_ro_entry entry;
    auto status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_JPEG_MAX_SIZE, &entry);
    ASSERT_TRUE((status == 0) && (entry.count > 0)) <<
        mCameraDeviceName << ": can't find maximum JPEG size in static metadata";

    int32_t jpgSize = entry.data.i32[0];
    free_camera_metadata(staticMeta);

    // Prepare file information
    // NOTE: JPEG's width and height is the same as the input ones
    uint32_t width = yuvFile.width;
    uint32_t height = yuvFile.height;
    int32_t format = static_cast<int32_t>(PixelFormat::BLOB);
    std::vector<Json::UInt> rowStrides {0, 0, 0};
    std::string fileName = "result/result_01.jpg";
    File jpgFile(mRootDirectory.value.asString(), fileName,
                    width, height, format, std::move(rowStrides), jpgSize);

    // instance of IDevice; valid after ctor
    sp<IDevice> device = nullptr;
    openISPDevice(device);

    // configure ISP device
    ISPStreamConfiguration hidlConfiguration;
    ConfigStreamParam params;
    params.inputStreams.push_back(
        StreamProfileParam{
            .streamId = STREAMID_Y2J_INPUT,
            .file = yuvFile,
        });
    params.outputStreams.push_back(
        StreamProfileParam{
            .streamId = STREAMID_Y2J_OUTPUT,
            .file = jpgFile,
        });
    constructHidlConfiguration(hidlConfiguration /*out*/, params);

// remove directives if need to set crop region
#if 0
    auto initEmptyMetadata = [](ISPMetadata& metadata)
    {
        std::vector<uint8_t> emptyMetadata;
        ASSERT_EQ(initial_isp_metadata_vector(emptyMetadata, 1), 0);

        ////////////////////////////////////////////////////
        //test: set crop
        uint32_t metaType = 1;
        int rectVec[4] = {1280, 1440, 640, 480};

        auto err = add_isp_metadata_entry(emptyMetadata, MTK_SCALER_CROP_REGION, metaType, (void *)(rectVec),4 * sizeof(int), nullptr);
        if (err)
        {
            CAM_LOGW("MTK_SCALER_CROP_REGION set fail! ret(%d)", err);
        }
        CAM_LOGD("set crop (x(%d) y(%d) w(%d) h(%d)",rectVec[0],rectVec[1],rectVec[2],rectVec[3]);

        ////////////////////////////////////////////////////

        metadata = emptyMetadata;
    };

    ISPMetadata configureParams;
    initEmptyMetadata(configureParams);
    hidlConfiguration.ConfigureParams = configureParams;
#endif

    // configure ISP device
    Return<isphal::V1_0::Status> ret = device->configureISPDevice(
            hidlConfiguration, mHalInterface->getICallback());
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in configureISPDevice from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "configureISPDevice failed";

    // just create 1 request
    std::vector<ISPRequest> requests;
    ISPRequest request;

    ISPBuffer outBuffer;  // initialized
    outBuffer.tuningDataSize = 0;
    allocIspRequestBuffer(outBuffer,
        AllocRequestBufferParam {
            .direction = Direction::OUT,
            .streamId = STREAMID_Y2J_OUTPUT,
            .imageFile = jpgFile,
            .tuningFile = File(),
            .useGarbage = 1,
        });

    ISPMetadata settings;
    initEmptyMetadata(settings);

    yuvBuffer.id = STREAMID_Y2J_INPUT;

    if (yuvBuffer.tuningData == nullptr) {
        CAM_LOGD("input tuning is null, try use tuning data from left path");
        bool found = false;
        for (const auto& f : mInputFiles) {
            if (f.isTuning()) {
                hidl_handle metadata_handle = hidl_handle();
                allocAndLoad(metadata_handle, f, Direction::IN, true /*needLoad*/);
                yuvBuffer.tuningData = metadata_handle;
                found = true;
                break;
            }
        }
        if (!found) {
            CAM_LOGE("no tuning data found !");
            return;
        }
    }

    request.reqNumber = gbReapting ? gFrameNumForReapting++ : 2;
    request.settings = settings;
    request.InputBuffers = std::vector<ISPBuffer>{yuvBuffer};
    request.OutputBuffers = std::vector<ISPBuffer>{outBuffer};
    requests.push_back(request);



    // record ISPRequest
    HalInterface::FileBufferPairs inputFileBuffersPairs{std::make_pair(yuvFile, yuvBuffer)};
    HalInterface::FileBufferPairs outputFileBuffersPairs{std::make_pair(jpgFile, outBuffer)};

    mHalInterface->recordISPRequest(request.reqNumber,
        std::move(inputFileBuffersPairs), std::move(outputFileBuffersPairs));

    // process image requests
    ret = device->processImgRequest(requests);
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in processImgRequest from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "processImgRequest failed";

    // wait result (tiemout is 1000 milliseconds by default)
    const int timeout = SINGLE_TEST_TIMEOUT;
    ASSERT_EQ(::android::OK, mHalInterface->waitResult(timeout))
        << "did not receive the result for " << timeout << " milliseconds";

    // close ISP device
    closeISPDevice(device);

    CAM_LOGD("YUV2JPEG -");
}

File ISPDeviceTest::findFile(std::string filename)
{
    for(auto& f : mInputFiles)
    {
        if(filename.compare(f.fileName) == 0)
            return f;
    }
    for(auto& f : mOutputFiles)
    {
        if(filename.compare(f.fileName) == 0)
            return f;
    }
    CAM_LOGD("File(%s) not found!", filename.c_str());
    return File();
};

////////////////
// For WPE
////////////////
void ISPDeviceTest::genWarpMapFilesAndStreamProfiles(
    std::vector<File> &warpMapFiles,
    std::vector<StreamProfile> &warpmapStreamProfiles)
{
    const Json::Value& config = mTest.value["ispWarpmapConfig"];
    const Json::Value& warpxJsonValue = config["warpx"];
    const Json::Value& warpyJsonValue = config["warpy"];
    const Json::UInt WarpMapW = config["width"].asUInt();
    const Json::UInt WarpMapH = config["height"].asUInt();

    const File inputImage = mInputFiles[0];
    const File outputImage = mOutputFiles[0];
    const Json::UInt WarpMapSz  = WarpMapW * WarpMapH;
    const std::string rootPath = mRootDirectory.value.asString();

    std::vector<Json::UInt> rowStrides {WarpMapSz * 4, 0, 0};
    File file_warpX(rootPath, warpxJsonValue["name"].asString(),
                    WarpMapW, WarpMapH,
                    Json::UInt(PixelFormat::BLOB), std::vector<Json::UInt>{WarpMapW * 4, 0, 0},
                    WarpMapSz * 4);

    File file_warpY(rootPath, warpyJsonValue["name"].asString(),
                    WarpMapW, WarpMapH,
                    Json::UInt(PixelFormat::BLOB), std::vector<Json::UInt>{WarpMapW * 4, 0, 0},
                    WarpMapSz * 4);

    CAM_LOGD("warpmap width(%d), height(%d), rowStrides(%d)",
        WarpMapW, WarpMapH, WarpMapW * 4);

    warpMapFiles.push_back(file_warpX);
    warpMapFiles.push_back(file_warpY);

    warpmapStreamProfiles.push_back(
        StreamProfile {
            .streamId = warpxJsonValue["streamId"].asUInt64(),
            .fileName = warpxJsonValue["name"].asString()
        }
    );
    warpmapStreamProfiles.push_back(
        StreamProfile {
            .streamId = warpyJsonValue["streamId"].asUInt64(),
            .fileName = warpyJsonValue["name"].asString()
        }
    );

    CAM_LOGD("warpmap x : path(%s), streamId(%lld), ",
        file_warpX.getFullyQualifiedName().c_str(), warpxJsonValue["streamId"].asUInt64());
    CAM_LOGD("warpmap y : path(%s), streamId(%lld), ",
        file_warpY.getFullyQualifiedName().c_str(), warpyJsonValue["streamId"].asUInt64());

    std::ofstream ofs_x(file_warpX.getFullyQualifiedName(), std::ofstream::out);
    std::ofstream ofs_y(file_warpY.getFullyQualifiedName(), std::ofstream::out);

    // 2 * 2
    ofs_x << (char)0x00 << (char)0x00 << (char)0x00 << (char)0x00
          << (char)0xE0 << (char)0xDF << (char)0x01 << (char)0x00
          << (char)0x00 << (char)0x00 << (char)0x00 << (char)0x00
          << (char)0xE0 << (char)0xDF << (char)0x01 << (char)0x00;

    ofs_y << (char)0x00 << (char)0x00 << (char)0x00 << (char)0x00
          << (char)0x00 << (char)0x00 << (char)0x00 << (char)0x00
          << (char)0xE0 << (char)0x0D << (char)0x01 << (char)0x00
          << (char)0xE0 << (char)0x0D << (char)0x01 << (char)0x00;

    ofs_x.close();
    ofs_y.close();
}

void ISPDeviceTest::bitTrueForWPE()
{
    const std::string rootPath = mRootDirectory.value.asString();

    std::ifstream ifs_golden( rootPath + "/golden.yuv",
                (std::ios::in | std::ios::binary));

    std::ifstream ifs_result( rootPath + "/result.yuv",
                (std::ios::in | std::ios::binary));

    int failCount = 0;
    auto outputFile = mOutputFiles[0];

    // only y plane
    for (uint32_t y = 0; y < outputFile.height; y++)
    {
        for (uint32_t x = 0; x < outputFile.rowStride[0]; x++)
        {
            char value;
            char value2;

            if (ifs_golden.get(value) && ifs_result.get(value2) )
            {
                if(value != value2)
                {
                    if(failCount < 10)
                        CAM_LOGE("Error at x(%d), y(%d)" , x , y );

                    failCount++;
                }
            }
            else
            {
                CAM_LOGE("Bit true get value failed !");
                ASSERT_TRUE(false) << "Bit true failed !";
                return;
            }
        }
    }

    if(failCount == 0)
        CAM_LOGD("Bit true pass !");
    else
        ASSERT_TRUE(false) << "Bit true failed !";

    ifs_golden.close();
    ifs_result.close();
}
