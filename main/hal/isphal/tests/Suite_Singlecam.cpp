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

#define DEBUG_LOG_TAG "isphal_test"
#define SINGLE_TEST_TIMEOUT 10000

#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "ISPDeviceTest.h"
#include "utils/AllocatorHelper.h"

#include <gtest/gtest.h>
#include <mtkisp_metadata.h>
#include <hardware/gralloc1.h>
#include <random>
#include <fstream>
#include <cutils/properties.h>
#include <system/camera_metadata.h>
#include <grallocusage/GrallocUsageConversion.h>

using ::android::hardware::hidl_vec;
using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::common::V1_0::PixelFormat;
using namespace ::vendor::mediatek::hardware::camera::isphal::V1_0;


constexpr uint64_t kProducerUsage(GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN |
        GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN | GRALLOC1_PRODUCER_USAGE_CAMERA);

constexpr uint64_t kConsumerUsage(GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN);

const int32_t kGralloc0Usage = android_convertGralloc1To0Usage(
        kProducerUsage, kConsumerUsage);

TEST_F(ISPDeviceTest, singleRequestYUV2JPEG)
{
    ISP_Test_Ops ops = {};
    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestYUV2YUV)
{
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                int isFHDCropping = property_get_int32("vendor.debug.camera.ISPY2Y.FHDCrop", 0);
                unsigned int FHD_WIDTH  = 1920;
                unsigned int FHD_HEIGHT = 1080;
                switch(__builtin_expect(isFHDCropping,0)){
                    case 1:
                        CAM_LOGD("adopt FHD size to test yuv cropping");
                        file.width = FHD_WIDTH;
                        file.height = FHD_HEIGHT;
                        file.size = FHD_WIDTH*FHD_HEIGHT*1.5;
                        break;

                    case 0:
                        CAM_LOGD("adopt original yuv size");
                        file.width = imageInput.width;
                        file.height = imageInput.height;
                        file.size = imageInput.size;
                        break;
                    default:
                        break;
                }
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestRAW2YUV)
{
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                //File& image(mOutputFiles[outputImageIndex]);
                file.width = imageInput.width;
                file.height = imageInput.height;
                file.size = imageInput.width * imageInput.height * 1.5;
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleZSL_RAW2YUV)
{
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                //File& image(mOutputFiles[outputImageIndex]);
                file.width = imageInput.width;
                file.height = imageInput.height;
                file.size = imageInput.width * imageInput.height * 1.5;
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestRAWHDR)
{
    ISP_Test_Ops ops = {};
    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestYUVHDR)
{
    ISP_Test_Ops ops = {};
    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestMFNR)
{
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                //File& image(mOutputFiles[outputImageIndex]);
                file.width = imageInput.width;
                file.height = imageInput.height;
                file.size = imageInput.width * imageInput.height * 1.5;
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            StatisticsInfo info;
            ISPMetadata metadata;
            initEmptyMetadata(metadata, 3);

            std::random_device rd;
            std::default_random_engine gen = std::default_random_engine(rd());
            std::uniform_int_distribution<int> dis(1,2000);
            std::uniform_int_distribution<int> dis2(1000,20000);
            int iso = dis(gen), exp = dis2(gen), captureHintForTuning = 1;
            CAM_LOGD("random iso: %d, exp: %dns", iso, exp);

            writeMeta(ANDROID_SENSOR_SENSITIVITY, 1, (void*)&iso, 1, &metadata);
            writeMeta(ANDROID_SENSOR_EXPOSURE_TIME, 1, (void*)&exp, 1, &metadata);
            writeMeta(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, 1, (void*)&captureHintForTuning, 1, &metadata);

            info.meta = metadata;
            isphal::V1_0::Status status(isphal::V1_0::Status::OK);
            uint32_t framecount = 0;
            std::vector<FeatureSetting> settings;
            Return<void> ret_t = mDevice->queryFeatureSetting(info, [&status, &framecount, &settings](
                        isphal::V1_0::Status _status,
                        uint32_t _framecount,
                        const hidl_vec<FeatureSetting>& _settings){
                    status = _status;
                    framecount = _framecount;
                    settings = _settings;
                    });
            CAM_LOGD("framecount: %d, size of settings: %zu", framecount, settings.size());
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestMFNRnoDrop)
{
    property_set("vendor.mfll.force", "1");
    property_set("vendor.mfll.drop_num", "0");
    property_set("vendor.mfll.capture_num", "6");
    property_set("vendor.mfll.blend_num", "6");
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                //File& image(mOutputFiles[outputImageIndex]);
                file.width = imageInput.width;
                file.height = imageInput.height;
                file.size = imageInput.width * imageInput.height * 1.5;
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            StatisticsInfo info;
            ISPMetadata metadata;
            initEmptyMetadata(metadata, 3);

            std::random_device rd;
            std::default_random_engine gen = std::default_random_engine(rd());
            std::uniform_int_distribution<int> dis(1,2000);
            std::uniform_int_distribution<int> dis2(1000,20000);
            int iso = dis(gen), exp = dis2(gen), captureHintForTuning = 1;
            CAM_LOGD("random iso: %d, exp: %dns", iso, exp);

            writeMeta(ANDROID_SENSOR_SENSITIVITY, 1, (void*)&iso, 1, &metadata);
            writeMeta(ANDROID_SENSOR_EXPOSURE_TIME, 1, (void*)&exp, 1, &metadata);
            writeMeta(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, 1, (void*)&captureHintForTuning, 1, &metadata);

            info.meta = metadata;
            isphal::V1_0::Status status(isphal::V1_0::Status::OK);
            uint32_t framecount = 0;
            std::vector<FeatureSetting> settings;
            Return<void> ret_t = mDevice->queryFeatureSetting(info, [&status, &framecount, &settings](
                        isphal::V1_0::Status _status,
                        uint32_t _framecount,
                        const hidl_vec<FeatureSetting>& _settings){
                    status = _status;
                    framecount = _framecount;
                    settings = _settings;
                    });
            CAM_LOGD("framecount: %d, size of settings: %d", framecount, settings.size());
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestMFNRDropN)
{
    property_set("vendor.mfll.force", "1");
    property_set("vendor.mfll.drop_num", "3");
    property_set("vendor.mfll.capture_num", "6");
    property_set("vendor.mfll.blend_num", "6");
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                file.width = imageInput.width;
                file.height = imageInput.height;
                file.size = imageInput.width * imageInput.height * 1.5;
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            StatisticsInfo info;
            ISPMetadata metadata;
            initEmptyMetadata(metadata, 3);

            std::random_device rd;
            std::default_random_engine gen = std::default_random_engine(rd());
            std::uniform_int_distribution<int> dis(1,2000);
            std::uniform_int_distribution<int> dis2(1000,20000);
            int iso = dis(gen), exp = dis2(gen), captureHintForTuning = 1;
            CAM_LOGD("random iso: %d, exp: %dns", iso, exp);

            writeMeta(ANDROID_SENSOR_SENSITIVITY, 1, (void*)&iso, 1, &metadata);
            writeMeta(ANDROID_SENSOR_EXPOSURE_TIME, 1, (void*)&exp, 1, &metadata);
            writeMeta(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, 1, (void*)&captureHintForTuning, 1, &metadata);

            info.meta = metadata;
            isphal::V1_0::Status status(isphal::V1_0::Status::OK);
            uint32_t framecount = 0;
            std::vector<FeatureSetting> settings;
            Return<void> ret_t = mDevice->queryFeatureSetting(info, [&status, &framecount, &settings](
                        isphal::V1_0::Status _status,
                        uint32_t _framecount,
                        const hidl_vec<FeatureSetting>& _settings){
                    status = _status;
                    framecount = _framecount;
                    settings = _settings;
                    });
            CAM_LOGD("framecount: %d, size of settings: %d", framecount, settings.size());
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestMFNRDropTo1)
{
    property_set("vendor.mfll.force", "1");
    property_set("vendor.mfll.drop_num", "5");
    property_set("vendor.mfll.capture_num", "6");
    property_set("vendor.mfll.blend_num", "6");
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            // Setup output yuv information, we query it from input
            const File& imageInput(inFs[0]);
            for(auto && file : outFs) {
                //File& image(mOutputFiles[outputImageIndex]);
                file.width = imageInput.width;
                file.height = imageInput.height;
                file.size = imageInput.width * imageInput.height * 1.5;
                file.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
            }
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            StatisticsInfo info;
            ISPMetadata metadata;
            initEmptyMetadata(metadata, 3);

            std::random_device rd;
            std::default_random_engine gen = std::default_random_engine(rd());
            std::uniform_int_distribution<int> dis(1,2000);
            std::uniform_int_distribution<int> dis2(1000,20000);
            int iso = dis(gen), exp = dis2(gen), captureHintForTuning = 1;
            CAM_LOGD("random iso: %d, exp: %dns", iso, exp);

            writeMeta(ANDROID_SENSOR_SENSITIVITY, 1, (void*)&iso, 1, &metadata);
            writeMeta(ANDROID_SENSOR_EXPOSURE_TIME, 1, (void*)&exp, 1, &metadata);
            writeMeta(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, 1, (void*)&captureHintForTuning, 1, &metadata);

            info.meta = metadata;
            isphal::V1_0::Status status(isphal::V1_0::Status::OK);
            uint32_t framecount = 0;
            std::vector<FeatureSetting> settings;
            Return<void> ret_t = mDevice->queryFeatureSetting(info, [&status, &framecount, &settings](
                        isphal::V1_0::Status _status,
                        uint32_t _framecount,
                        const hidl_vec<FeatureSetting>& _settings){
                    status = _status;
                    framecount = _framecount;
                    settings = _settings;
                    });
            CAM_LOGD("framecount: %d, size of settings: %d", framecount, settings.size());
            return 0;
        },
        .fOnAfterProcessOneReqPackage =
            [&](std::vector<ISPRequest> &requests,
                std::vector<RequestConfiguration> &reqConfig)
        {
            for (int i = 0 ; i < requests.size() ; i++) {
                auto &req = requests[i];
                for (int j = 0 ; j < req.OutputBuffers.size() ; j++) {
                    ISPBuffer &yuvBuffer = req.OutputBuffers[j];
                    File yuvFile = findFile(reqConfig[i].vOutBuffers[j].imageName);
                    YUV2JPEG(yuvBuffer, yuvFile);
                }
            }
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestAINR2YUV)
{
    jsonVerifier();

    // Setup output yuv information, we query it from input
    // Image output and tuning are allocated as YUV to store data
    const File& imageInput(mInputFiles[0]);
    for(auto && image : mOutputFiles) {
        image.width = imageInput.width;
        image.height = imageInput.height;
        image.size = imageInput.width * imageInput.height * 1.5;
        image.format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
    }

    configIsp();

    // Query StatisticsInfo
    StatisticsInfo info;
    ISPMetadata metadata;
    // TODO: We suggest "add_isp_metadata_entry" should support metadata re-allocate
    initEmptyMetadata(metadata, 3);; // 3 means that there are 3 entry of metadata

    int iso = 6000;
    int64_t shutter = 59999;
    int tuningHint = 2; // MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_AINR

    writeMeta(ANDROID_SENSOR_SENSITIVITY, 1, (void*)&iso, 1, &metadata);
    writeMeta(ANDROID_SENSOR_EXPOSURE_TIME, 2, (void*)&shutter, 1, &metadata);
    writeMeta(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, 1, (void*)&tuningHint, 1, &metadata);
    info.meta = metadata;

    isphal::V1_0::Status status(isphal::V1_0::Status::OK);
    std::vector<FeatureSetting> featureSettings;
    mDevice->queryFeatureSetting(info, [&status, &featureSettings](
                            isphal::V1_0::Status _status,
                            uint32_t framecount, const hidl_vec<FeatureSetting>& _settings) {
                                status = _status;
                                featureSettings = _settings;
                                CAM_LOGD("Capture frameCount(%d) size(%zu)", framecount, featureSettings.size());
                        });

    std::vector<ISPRequest> requests;
    buildIspRequests(requests);

    Return<isphal::V1_0::Status> ret = mDevice->processImgRequest(requests);
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in processImgRequest from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "processImgRequest failed";

    // wait result (tiemout is 1000 milliseconds by default)
    const int timeout = SINGLE_TEST_TIMEOUT;
    ASSERT_EQ(::android::OK, mHalInterface->waitResult(timeout))
        << "did not receive the result for " << timeout << " milliseconds";

    // Encode yuv to jpeg
    std::vector<ISPBuffer> yuvInputBuffers;
    for(auto && ispBuf : requests[0].OutputBuffers) {
        yuvInputBuffers.push_back(ispBuf);
    }

    YUV2JPEG(yuvInputBuffers, mOutputFiles);
}

TEST_F(ISPDeviceTest, singleRequestAINR2RAW)
{
    jsonVerifier();

    // Setup output yuv information, we query it from input
    // Image output and tuning are allocated as YUV to store data
    const File& imageInput(mInputFiles[0]);
    for(size_t i = 0; i < mOutputFiles.size(); i++) {
        mOutputFiles[i].width = imageInput.width;
        mOutputFiles[i].height = imageInput.height;
        if(i % 2 == 0) {
            mOutputFiles[i].size = imageInput.width * imageInput.height * 2;
            mOutputFiles[i].format = static_cast<int32_t>(PixelFormat::RAW16);
        } else {
            mOutputFiles[i].size = imageInput.width * imageInput.height * 1.5;
            mOutputFiles[i].format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
        }
    }

    configIsp();

    std::vector<ISPRequest> requests;
    buildIspRequests(requests);

    Return<isphal::V1_0::Status> ret = mDevice->processImgRequest(requests);
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in processImgRequest from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(ret, isphal::V1_0::Status::OK) << "processImgRequest failed";

    // wait result (tiemout is 1000 milliseconds by default)
    const int timeout = SINGLE_TEST_TIMEOUT;
    ASSERT_EQ(::android::OK, mHalInterface->waitResult(timeout))
        << "did not receive the result for " << timeout << " milliseconds";

#if 0
    // Encode raw to yuv
    std::vector<ISPBuffer> rawInputBuffers;
    for(auto && ispBuf : requests[0].OutputBuffers) {
        rawInputBuffers.push_back(ispBuf);
    }

    std::vector<ISPBuffer> outIspBuffers;
    RAW2YUV(rawInputBuffers, outIspBuffers, mOutputFiles);
#endif

    // Parse resolution from OutputFiles and determine format and size by requirement
    std::vector<File> inputYuvFiles;
    for (auto && file : mOutputFiles)
    {
        // get the first input image's width and height
        uint32_t width = file.width;
        uint32_t height = file.height;
        int32_t format = static_cast<int32_t>(PixelFormat::YCBCR_420_888);
        int32_t size = width * height * 1.5;
        std::string fileName = "result/result_01.yuv";
        std::vector<Json::UInt> rowStrides {0, 0, 0};

        File yuvFile(mRootDirectory.value.asString(), fileName,
                        width, height, format, std::move(rowStrides), size);
        inputYuvFiles.push_back(yuvFile);
    }
    std::vector<ISPBuffer> outIspBuffers;
    YUV2JPEG(outIspBuffers, inputYuvFiles);
}

TEST_F(ISPDeviceTest, singleRequestAINR2JPEG)
{
    ISP_Test_Ops ops = {
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            ISPMetadata metadata;
            initEmptyMetadata(metadata, 3);

            int iso = 6001;
            int exp = 33001;
            int captureHintForTuning = 2;
            CAM_LOGD("iso: %d, exp: %dns", iso, exp);

            writeMeta(ANDROID_SENSOR_SENSITIVITY, TYPE_MINT32, (void*)&iso, 1, &metadata);
            writeMeta(ANDROID_SENSOR_EXPOSURE_TIME, TYPE_MINT64, (void*)&exp, 1, &metadata);
            writeMeta(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, TYPE_MINT32, (void*)&captureHintForTuning, 1, &metadata);

            StatisticsInfo info;
            info.meta = metadata;
            isphal::V1_0::Status status(isphal::V1_0::Status::OK);
            uint32_t framecount = 0;
            std::vector<FeatureSetting> settings;

            Return<void> ret_t = mDevice->queryFeatureSetting(info, [&status, &framecount, &settings](
                        isphal::V1_0::Status _status,
                        uint32_t _framecount,
                        const hidl_vec<FeatureSetting>& _settings){
                    status = _status;
                    framecount = _framecount;
                    settings = _settings;
                    });

            CAM_LOGD("framecount: %d, size of settings: %zu", framecount, settings.size());
            EXPECT_TRUE(framecount != 0)
                << "not support AINR !!";
            return 0;
        }
    };

    Run(ops);
}

TEST_F(ISPDeviceTest, singleRequestAIHDR2JPEG)
{
    ISP_Test_Ops ops = {};

    Run(ops);
}

TEST_F(ISPDeviceTest, multipleRequestMFNR)
{
    // reconstruct buffers and settings from the JSON file
    jsonVerifier();
}
