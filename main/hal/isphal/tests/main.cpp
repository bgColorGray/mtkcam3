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
#define MULTI_TEST_TIMEOUT 20000
#define SINGLE_TEST_TIMEOUT 10000
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/TypeTraits.h>

#include "utils/JSONContainer.h"
#include "types.h"

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
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
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

using namespace isphal_test;


// ------------------------------------------------------------------------

constexpr uint64_t kProducerUsage(GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN |
        GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN | GRALLOC1_PRODUCER_USAGE_CAMERA);

constexpr uint64_t kConsumerUsage(GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN);

const int32_t kGralloc0Usage = android_convertGralloc1To0Usage(
        kProducerUsage, kConsumerUsage);

bool gbWaitData = false;
bool gbReapting = false;
int  gFrameNumForReapting = 0;

std::unordered_map<std::string, uint32_t> gIspMetaMap {
    { "com.mediatek.control.capture.hintForIspTuning", MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING },
    { "com.mediatek.control.capture.hintForIspFrameCount", MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_COUNT },
    { "com.mediatek.control.capture.hintForIspFrameIndex", MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX },
    { "com.mediatek.control.capture.hintForIspFrameIndex", MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX },
    { "com.android.control.aeMode.sensitivity", ANDROID_SENSOR_SENSITIVITY},
    { "com.mediatek.control.capture.highQualityYuv", MTK_CONTROL_CAPTURE_HIGH_QUALITY_YUV },
    { "mtk.hal.request.sensorid", MTK_HAL_REQUEST_SENSOR_ID },
    { "com.mediatek.control.ae.regions", MTK_CONTROL_AE_REGIONS},
    { "com.mediatek.singlehwsetting.module", MTK_SINGLEHW_SETTING_MODULE},
    { "com.mediatek.singlehwsetting.sourcecrop", MTK_SINGLEHW_SETTING_SOURCE_CROP},
    { "com.mediatek.singlehwsetting.transform", MTK_SINGLEHW_SETTING_TRANSFORM},
    { "com.mediatek.singlehwsetting.videostream", MTK_SINGLEHW_SETTING_VIDEO_STREAM},
    { "com.mediatek.singlehwsetting.warpmapx", MTK_SINGLEHW_SETTING_WARP_MAP_X},
    { "com.mediatek.singlehwsetting.warpmapy", MTK_SINGLEHW_SETTING_WARP_MAP_Y},
    { "com.mediatek.singlehwsetting.warpoutput", MTK_SINGLEHW_SETTING_WARP_OUTPUT},
    { "com.mediatek.hdrfeature.hdrMode", MTK_HDR_FEATURE_HDR_MODE}
};

// ------------------------------------------------------------------------

int runGoogleTest()
{
    return RUN_ALL_TESTS();
}

// ------------------------------------------------------------------------

std::ostream& printOneValue(const Json::Value& value, std::stringstream& ss)
{
    switch (value.type())
    {
        case Json::nullValue:
            ss << "(null)";
            break;
        case Json::intValue:
            ss << value.asInt();
            break;
        case Json::uintValue:
            ss << value.asUInt();
            break;
        case Json::realValue:
            ss << value.asDouble();
            break;
        case Json::booleanValue:
            ss << value.asBool();
            break;
        case Json::stringValue:
            ss << value.asCString();
            break;
        default:
            ss << "unknown type=[" << value.type() << "]";
    }

    return ss;
}

void traverseJSON(const Json::Value& root, size_t depth)
{
    static std::stringstream ss;

    CAM_LOGV("(%zu) root type(%d) size(%u)", depth, root.type(), root.size());

    // traverse non-leaf node (object or array) recursively
    if (root.size() > 0)
    {
        if (root.type() == Json::objectValue)
            ss << std::setfill(' ') << std::setw(depth+1) << "{" << std::endl;
        else if (root.type() == Json::arrayValue)
            ss << std::setfill(' ') << std::setw(depth+1) << "[" << std::endl;

        Json::ValueConstIterator end = root.end();
        for (Json::ValueConstIterator it = root.begin(); it != end; ++it)
        {
            // do not break line if the next level reaches the leaf node
            if ((it->size() >= 0) && (root.type() == Json::arrayValue))
                ss << std::setfill(' ') << std::setw(depth+1) << ' ';
            else if ((it->size() >= 0) && (root.type() == Json::objectValue))
                ss << std::setfill(' ') << std::setw(depth+1) << ' ';

            // print either the index of the member name of the referenced value
            printOneValue(it.key(), ss) << ": ";

            // do not break line if the next level reaches the leaf node
            if (it->size() > 0)
            {
                ss << std::endl;
                CAM_LOGD("%s", ss.str().c_str());
                ss.str(std::string());
            }

            traverseJSON(*it, depth+1);
        }

        if (root.type() == Json::objectValue)
            ss << std::endl << std::setfill(' ') << std::setw(depth+1) << "}";
        else if (root.type() == Json::arrayValue)
            ss << std::endl << std::setfill(' ') << std::setw(depth+1) << "]";

        ss << std::endl;
        CAM_LOGD("%s", ss.str().c_str());
        ss.str(std::string());

        return;
    }

    // leaf node (value except object or arrary type)
    printOneValue(root, ss) << std::endl;
    CAM_LOGD("%s", ss.str().c_str());
    ss.str(std::string());
}

int jsonTest()
{
    const Json::Value& testCaseSetting(
            JSONContainer::getInstance().getTestCaseSetting());

    // 0. traverse JSON object/array
    traverseJSON(testCaseSetting, 1);

    // 1. Show test suite names
    std::vector<std::string> testSuiteNameList;
    {
        Json::ValueConstIterator end = testCaseSetting.end();
        for (Json::ValueConstIterator it = testCaseSetting.begin(); it != end; ++it)
        {
            std::stringstream ss;
            printOneValue(it.key(), ss);
            testSuiteNameList.push_back(ss.str());
            CAM_LOGD("Test suite: %s", ss.str().c_str());
        }
    }

    // 2. Show test names
    std::vector<std::string> testNameList;
    for (const auto& testSuiteName : testSuiteNameList)
    {
        const Json::Value testSuite = testCaseSetting[testSuiteName.c_str()];
        Json::ValueConstIterator end = testSuite.end();
        for (Json::ValueConstIterator it = testSuite.begin(); it != end; ++it)
        {
            std::stringstream ss;
            printOneValue(it.key(), ss);
            testNameList.push_back(ss.str());
            CAM_LOGD("Test name: %s", ss.str().c_str());
        }
    }

    return ::android::OK;
}

void showUsage(const char *programName)
{
    std::printf("Usage: %s [OPTION]\n\n", programName);
    std::printf("DESCRIPTION\n");
    std::printf("\t-g [--help|-h|-?] \texecute Google tests.\n");
    std::printf("\t-j\t\t\texecute JSON tests.\n");
    std::printf("\t\t\t\t--help, -h or -? lists the supported flags and their usage\n");
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        showUsage(argv[0]);
        return ::android::OK;
    }

    // parse the command line and remove all recognized Google Test flags
    // must call this function before calling RUN_ALL_TESTS()
    ::testing::InitGoogleTest(&argc, argv);

    int opt;
    while ((opt = getopt(argc, argv, "gjrw")) != -1)
    {
        switch (opt)
        {
            case 'g':
                if (!!JSONContainer::getInstance().getTestCaseSetting())
                    runGoogleTest();
                break;
            case 'j':
                if (!!JSONContainer::getInstance().getTestCaseSetting())
                    jsonTest();
                break;
            case 'r':
                if (!!JSONContainer::getInstance().getTestCaseSetting()) {
                    gbReapting = true;
                    runGoogleTest();
                }
                break;
            case 'w':
                if (!!JSONContainer::getInstance().getTestCaseSetting()) {
                    gbWaitData = true;
                    runGoogleTest();
                }
                break;
            default:
                break;
        }
    }

    return ::android::OK;
}