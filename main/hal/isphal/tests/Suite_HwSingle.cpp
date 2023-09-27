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

#include "ISPModuleTest.h"
#include "ISPDeviceTest.h"

#include <cutils/properties.h>
#include <grallocusage/GrallocUsageConversion.h>
#include <gtest/gtest.h>
#include <hardware/gralloc1.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkisp_metadata.h>
#include <utils/AllocatorHelper.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>

#include <fstream>
#include <iostream>

#define TEST_TIMEOUT 20000

using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::hidl_vec;
using ::android::sp;

constexpr uint64_t kProducerUsage(GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN |
        GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN | GRALLOC1_PRODUCER_USAGE_CAMERA);

constexpr uint64_t kConsumerUsage(GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN);

const int32_t kGralloc0Usage = android_convertGralloc1To0Usage(
        kProducerUsage, kConsumerUsage);

TEST_F(ISPDeviceTest, WPE_Rotate)
{
    File firstImageInput;
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs __attribute__((unused)), std::vector<File> &outFs __attribute__((unused))) {
            std::vector<File> warpmapFiles;
            std::vector<StreamProfile> warpmapStreamProfiles;
            firstImageInput = inFs[0];
            genWarpMapFilesAndStreamProfiles(
                warpmapFiles /* out */,
                warpmapStreamProfiles /* out */);

            {
                mInputFiles.insert(mInputFiles.end(),
                                    warpmapFiles.begin(),
                                    warpmapFiles.end());

                auto &inputStreams = mStreamConfiguration.inputStreams;
                inputStreams.insert(inputStreams.end(),
                                    warpmapStreamProfiles.begin(),
                                    warpmapStreamProfiles.end());
            }
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            int warp_crop[4] = {};
            int crop_source[4] = {};
            for(auto& requests : processPackage) {
                for(auto& request : requests) {
                    {
                        struct Metadata meta_warpoutput;
                        meta_warpoutput.name = "com.mediatek.singlehwsetting.warpoutput";
                        meta_warpoutput.data.i32[0] = 0;
                        meta_warpoutput.data.i32[1] = 0;
                        meta_warpoutput.data.i32[2] = firstImageInput.width;
                        meta_warpoutput.data.i32[3] = firstImageInput.height;
                        meta_warpoutput.tag = MTK_SINGLEHW_SETTING_WARP_OUTPUT;
                        meta_warpoutput.type = Metadata::Type::INT32;
                        meta_warpoutput.size = 4;
                        request.vMetadata.push_back(meta_warpoutput);
                    }

                    {
                        struct Metadata meta_sourcecrop;
                        meta_sourcecrop.name = "com.mediatek.singlehwsetting.sourcecrop";
                        meta_sourcecrop.data.i32[0] = 0;
                        meta_sourcecrop.data.i32[1] = 0;
                        meta_sourcecrop.data.i32[2] = firstImageInput.width;
                        meta_sourcecrop.data.i32[3] = firstImageInput.height;
                        meta_sourcecrop.tag = MTK_SINGLEHW_SETTING_SOURCE_CROP;
                        meta_sourcecrop.type = Metadata::Type::INT32;
                        meta_sourcecrop.size = 4;
                        request.vMetadata.push_back(meta_sourcecrop);
                    }
                }
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

TEST_F(ISPDeviceTest, WPE_HwBitTrue)
{
    ISP_Test_Ops ops = {
    };

    Run(ops);

    bitTrueForWPE();
}

TEST_F(ISPDeviceTest, MDP_Flip)
{
    ISP_Test_Ops ops = {
    };
    Run(ops);
}

#define ALIGN_2(x)     (((x) + 1) & (~1))

TEST_F(ISPDeviceTest, MDP_Crop)
{
    File firstInputImage;
    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            firstInputImage = inFs[0];
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            for (auto& vReqConfig : processPackage)
            {
                Metadata ispMeta;
                ispMeta.name = "com.mediatek.singlehwsetting.sourcecrop";
                ispMeta.tag = MTK_SINGLEHW_SETTING_SOURCE_CROP;
                ispMeta.type = (Metadata::Type)TYPE_MINT32;
                ispMeta.size = 4;
                ispMeta.data.i32[0] = 50;
                ispMeta.data.i32[1] = 50;
                ispMeta.data.i32[2] = ALIGN_2( firstInputImage.width / 2);
                ispMeta.data.i32[3] = ALIGN_2( firstInputImage.height / 2);

                for (auto& reqConfig : vReqConfig) {
                    reqConfig.vMetadata.push_back(std::move(ispMeta));
                }
            }
            return 0;
        }
    };
    Run(ops);
}

TEST_F(ISPDeviceTest, MDP_Rotate)
{
    File *firstInputImage = nullptr;
    File *firstOutputImage = nullptr;

    ISP_Test_Ops ops = {
        .fOnBeforeConfig = [&](std::vector<File> &inFs, std::vector<File> &outFs) {
            firstInputImage = &inFs[0];
            firstOutputImage = &outFs[0];
            return 0;
        },
        .fOnBeforeRequest = [&](
            std::vector<std::vector<RequestConfiguration>> &processPackage)
        -> int {
            uint32_t allocStrideRes = 0;
            uint64_t kConsumerUsage(GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN);
            if(!firstInputImage || !firstOutputImage) {
                CAM_LOGE("firstInputImage or firstOutputImage null !");
            }

            AllocatorHelper::getInstance().queryStride(
                firstInputImage->width,
                firstInputImage->height,
                kConsumerUsage,
                firstInputImage->getPixelFormat(),
                &allocStrideRes);

            // HAL will output to dst buffer but doesn't refer dst buffer's width and stride.
            // It causes isphal_test fills the padding if width < pixel length < stride in writing file stage.
            // So set output file's width as stride for writing file reason.
            CAM_LOGD("output image width %d -> %d", firstOutputImage->width, allocStrideRes);
            firstOutputImage->width = allocStrideRes;
            return 0;
        }
    };
    Run(ops);
}

TEST_F(ISPDeviceTest, MDP_1For2)
{
    ISP_Test_Ops ops = {
    };
    Run(ops);
}