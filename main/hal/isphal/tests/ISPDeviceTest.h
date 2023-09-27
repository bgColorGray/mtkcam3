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
#include <memory>
#include <unordered_map>
#include <json/json.h>

#include <android/hardware/camera/device/3.2/types.h>
#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
#include <android/hardware/graphics/common/1.0/types.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>

#include "ISPModuleTest.h"
#include "types.h"

using namespace isphal_test;
using namespace ::vendor::mediatek::hardware::camera;
using namespace ::vendor::mediatek::hardware::camera::isphal::V1_0;

using ::android::hardware::camera::provider::V2_6::ICameraProvider;
using ::android::hardware::hidl_handle;
using ::android::sp;

class ISPDeviceTest : public ISPModuleTest
{
protected:
    ISPDeviceTest();
    ~ISPDeviceTest();

    // instance of IDevice; valid after ctor
    sp<IDevice> mDevice;

    // instance of IDevice valid after ctor
    sp<HalInterface> mHalInterface;

    // camera device name
    //NOTE: The test app sets camera id, which is written to the JSON setting file
    //      and is a small incrementing integer for "internal" device types,
    //      with 0 being the main back-facing camera and 1 being
    //      the main front-facing camera, if they exist.
    //
    //      Then we transfers the camera id into a fully qualified
    //      camera device name that is used for manipulating
    //      the V3_x::ICameraDevice for static metadata retrieval.
    std::string mCameraDeviceName;

    // mandatory test case setting properties
    struct ValueNamePair
    {
        std::string name;
        Json::Value value;

        ValueNamePair() = default;
        explicit ValueNamePair(const std::string& _name, const Json::Value& _value)
            : name(_name) , value(_value) {}

        ValueNamePair& operator=(const ValueNamePair& other) {
            if (this != &other)
            {
                name = other.name;
                value = other.value;
            }
            return *this;
        }

        ValueNamePair& operator=(ValueNamePair&& other) {
            if (this != &other)
            {
                name = std::move(other.name);
                value = other.value;
                other.value = Json::Value::null;
            }
            return *this;
        }
    };

    ValueNamePair mTestSuite;
    ValueNamePair mTest;

    ValueNamePair mRootDirectory;
    ValueNamePair mTotalRequests;
    size_t getTotalRequests() const { return mTotalRequests.value.asUInt(); }

    std::vector<File> mInputFiles;
    std::vector<File> mOutputFiles;

    /* create from json file */

    struct Metadata
    {
        std::string name;
        uint32_t tag;
        //
        union {
        uint8_t u8[32];
        int32_t i32[8];
        float   f32[8];
        int64_t i64[4];
        } data;
        uint32_t padding = 0; // align 8 bytes

        enum class Type : int32_t {
            BYTE,
            INT32,
            INT64,
            FLOAT
        };
        Type type;
        int  size;
    };

    struct StreamProfile {
        uint64_t    streamId;
        std::string fileName;
        std::string realFormat;
    };

    struct StreamConfiguration
    {
        // TODO: define struct Stream to substitute uint64_t
        std::vector<StreamProfile> inputStreams;
        std::vector<StreamProfile> outputStreams;
        std::vector<Metadata> vMetadata;
    };

    struct RequestConfiguration
    {
        struct ReqBuffer
        {
            uint64_t    streamId  = -1;
            std::string imageName;
            std::string tuningName;
            bool useGarbage = 0;
        };

        // TODO: define struct Request to substitute uint64_t
        std::vector<ReqBuffer> vInBuffers;
        std::vector<ReqBuffer> vOutBuffers;

        uint32_t requestNumber;
        std::vector<Metadata> vMetadata;
    };

    enum class Direction : int {
        IN,
        OUT
    };

    /* config param */
    struct StreamProfileParam {
        uint64_t    streamId;
        File file;
        std::string realFormat;
    };

    struct ConfigStreamParam {
        std::vector<StreamProfileParam> inputStreams;
        std::vector<StreamProfileParam> outputStreams;
        std::vector<Metadata> vMetadata;
    };

    /* requeset param */
    struct AllocRequestBufferParam {
        Direction direction;
        uint64_t streamId;
        File imageFile;
        File tuningFile;
        bool useGarbage = 0;
    };

    struct BuildRequestParam {
        uint64_t reqNum;
        std::vector<Metadata> vMetadata;
        std::vector<AllocRequestBufferParam> bufferParams;
    };

    struct ISP_Test_Ops {
        std::function<int(std::vector<File> &inFs, std::vector<File> &outFs)>
            fOnBeforeConfig;
        std::function<
            int(std::vector<std::vector<RequestConfiguration>> &processPackage)
            > fOnBeforeRequest;
        std::function<int(std::vector<ISPRequest> &requests,
                          std::vector<RequestConfiguration> &reqConfig)>
            fOnAfterProcessOneReqPackage;
    };

    // NOTE: the number of requestion configurations MUST be the same as that
    //       of total requests
    StreamConfiguration mStreamConfiguration;
    std::vector<std::vector<RequestConfiguration>> mProcessPackage;

    // All loaded settings from a JSON file are verified and reconstruct here
    // NOTE: If any of the property name are removed/added/changed,
    //       the implementation of this API MUST BE also updated.
    void jsonVerifier(bool printerEnabled = true);

    // Load files from directory
    int loadFromFile(hidl_handle &rawHandle, const File& file, const PixelFormat format);
    void configIsp();
    int buildIspRequests(std::vector<ISPRequest> &out, std::vector<RequestConfiguration> &vInReq);
    int buildIspRequests(std::vector<ISPRequest> &out, std::vector<BuildRequestParam> &params);
    int buildIspRequestAndRecord(ISPRequest &out, const BuildRequestParam &param);
    int allocIspRequestBuffer(ISPBuffer &out, const AllocRequestBufferParam &param);
    File findFile(std::string filename);
    void YUV2JPEG(ISPBuffer &inIspBuffers, File &inputFile);
    void Run(ISP_Test_Ops ops);
    void formatPrinter(Direction direction, isphal::V1_0::Status status,
            const std::vector<ISPFormat>& supportedFormats) const;
    void featureTagInfoPrinter(const std::vector<ISPFeatureTagInfo>& tagInfos) const;
    int allocAndLoad(hidl_handle &handle, const File &file, Direction direction, bool needLoad);

    static void constructHidlConfiguration(ISPStreamConfiguration &hidlConfiguration /*out*/, ConfigStreamParam &params);
    static void initEmptyMetadata(ISPMetadata& metadata, int count = 20);
    static int  writeMeta(Metadata src, ISPMetadata *metadata);
    static int  writeMeta(uint32_t tag, uint32_t type, void* value, int size, ISPMetadata *metadata);

    //
    void genWarpMapFilesAndStreamProfiles(
            std::vector<File> &warpMapFiles,
            std::vector<StreamProfile> &warpStreams);
    void bitTrueForWPE();

    // phase out
    void YUV2JPEG(std::vector<ISPBuffer> &inIspBuffers __attribute__((unused)), std::vector<File> &inputFiles __attribute__((unused))){}
    void buildIspRequests(std::vector<ISPRequest> &out __attribute__((unused))){}
private:
    void init();
};