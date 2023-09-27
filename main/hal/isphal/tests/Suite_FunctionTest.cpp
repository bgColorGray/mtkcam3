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
#include <gtest/gtest.h>
#include <vendor/mediatek/hardware/camera/isphal/1.0/IDevice.h>
using ::android::hardware::hidl_vec;
using ::android::sp;

TEST_F(ISPModuleTest, linkAndUnlinkHWBinderDeath)
{
    // link to death
    sp<HalInterface> halInterface;
    linkToDeath(halInterface);

    // unlink the registered death recipient
    unlinkToDeath(halInterface);
}

TEST_F(ISPModuleTest, openAndCloseISPDevice)
{
    // open ISP device
    sp<IDevice> device;
    openISPDevice(device);

    // close ISP device
    closeISPDevice(device);
}

TEST_F(ISPDeviceTest, getISPSupportedFormats)
{
    isphal::V1_0::Status status(isphal::V1_0::Status::OK);
    std::vector<ISPFormat> supportedFormats;
    auto getSupportedFormat = [&](Direction direction) {
        if (direction == Direction::IN)
        {
            Return<void> ret =
                mDevice->getISPSupportInputFormat([&status, &supportedFormats](
                            isphal::V1_0::Status _status,
                            const hidl_vec<ISPFormat>& _supportedFormats) {
                        status = _status;
                        supportedFormats = _supportedFormats;
                        });
            ASSERT_TRUE(ret.isOk())
                << "Transaction error in getISPSupportInputFormat from ISPDevice: "
                << ret.description().c_str();
            ASSERT_EQ(status, isphal::V1_0::Status::OK) << "getISPSupportInputFormat failed("
                << toString(status) << ")";

            return;
        }

        Return<void> ret =
            mDevice->getISPSupportOutputFormat([&status, &supportedFormats](
                        isphal::V1_0::Status _status,
                        const hidl_vec<ISPFormat>& _supportedFormats) {
                    status = _status;
                    supportedFormats = _supportedFormats;
                    });
        ASSERT_TRUE(ret.isOk())
            << "Transaction error in getISPSupportOutputFormat from ISPDevice: "
            << ret.description().c_str();
        ASSERT_EQ(status, isphal::V1_0::Status::OK) << "getISPSupportOutputFormat failed("
            << toString(status) << ")";
    };

    // get supported input format
    getSupportedFormat(Direction::IN);
    ISPDeviceTest::formatPrinter(Direction::IN, status, supportedFormats);

    // get supported output format
    getSupportedFormat(Direction::OUT);
    ISPDeviceTest::formatPrinter(Direction::OUT, status, supportedFormats);
}

TEST_F(ISPDeviceTest, getISPSupportFeatures)
{
    isphal::V1_0::Status status(isphal::V1_0::Status::OK);

    // get supported features
    std::vector<ISPFeatureTagInfo> tagInfos;
    Return<void> ret = mDevice->getISPSupportFeatures([&status, &tagInfos](
                isphal::V1_0::Status _status,
                const hidl_vec<ISPFeatureTagInfo>& _tagInfos){
            status = _status;
            tagInfos = _tagInfos;
            });
    ASSERT_TRUE(ret.isOk())
        << "Transaction error in getISPSupportFeatures from ISPDevice: "
        << ret.description().c_str();
    ASSERT_EQ(status, isphal::V1_0::Status::OK) << "getISPSupportFeatures failed("
        << toString(status) << ")";

    ISPDeviceTest::featureTagInfoPrinter(tagInfos);
}

#ifdef ENABLE_JPEG_BUFFER_TEST
/**
 * This test fixture is for testing class JPEGBufferTest.
 */
class JPEGBufferTest : public TestBase
{
};

TEST_F(JPEGBufferTest, JPEGStream)
{
    const std::string jpegBufferName("/sdcard/jpeg.blob");

    const auto jpegBufferSize = getFileSize(jpegBufferName);
    ASSERT_NE(jpegBufferSize, -1) << "get file size failed";

    std::unique_ptr<char[]> jpegBuffer(new char[jpegBufferSize]);

    // read jpeg stream buffer from file
    std::ifstream ifs(jpegBufferName, (std::ios::in | std::ios::binary));
    ASSERT_TRUE(ifs.is_open()) << strerror(errno) << ": " << jpegBufferName << "\n";
    ifs.read(jpegBuffer.get(), jpegBufferSize);
    ifs.close();

    // show CameraBlob
    CAM_LOGD("parse jpegBuffer(%s) size(%ld)",
            jpegBufferName.c_str(), jpegBufferSize);
    const CameraBlob *transportHeader = reinterpret_cast<const CameraBlob*>(
            jpegBuffer.get() + jpegBufferSize - sizeof(CameraBlob));
    CAM_LOGD("%s", toString(*transportHeader).c_str());

    // extract jpeg image from jpeg stream buffer and save it to file
    std::ofstream ofs(jpegBufferName + ".jpeg", (std::ios::out | std::ios::binary));
    ASSERT_TRUE(ofs.is_open()) << strerror(errno) << ": " << jpegBufferName << "\n";
    ofs.write(jpegBuffer.get(), transportHeader->blobSize);
    ofs.close();
}
#endif // ENABLE_JPEG_BUFFER_TEST