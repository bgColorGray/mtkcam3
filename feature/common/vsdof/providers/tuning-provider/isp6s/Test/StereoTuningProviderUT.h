/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifndef STEREO_TUNING_PROVIDER_UT_H_
#define STEREO_TUNING_PROVIDER_UT_H_

#include <limits.h>
#include <gtest/gtest.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <stereo_tuning_provider.h>

using namespace NSCam;
using namespace android;
using namespace NSCam::v1::Stereo;
using namespace StereoHAL;

// #define MY_LOGD(fmt, arg...)    printf("[D][%s]" fmt"\n", __func__, ##arg)
// #define MY_LOGI(fmt, arg...)    printf("[I][%s]" fmt"\n", __func__, ##arg)
// #define MY_LOGW(fmt, arg...)    printf("[W][%s] WRN(%5d):" fmt"\n", __func__, __LINE__, ##arg)
// #define MY_LOGE(fmt, arg...)    printf("[E][%s] %s ERROR(%5d):" fmt"\n", __func__,__FILE__, __LINE__, ##arg)

// #define FUNC_START MY_LOGD("[%s] +", __FUNCTION__)
// #define FUNC_END   MY_LOGD("[%s] -", __FUNCTION__)

#define PRINT_SIZE  1

inline void print(const char *tag, MSize size)
{
#if PRINT_SIZE
    printf("%s: %dx%d\n", tag, size.w, size.h);
#endif
}

inline void print(const char *tag, MRect rect)
{
#if PRINT_SIZE
    printf("%s: (%d, %d), %dx%d\n", tag, rect.p.x, rect.p.y, rect.s.w, rect.s.h);
#endif
}

inline void print(const char *tag, StereoArea area)
{
#if PRINT_SIZE
    if(area.padding.w != 0 || area.padding.h != 0) {
    printf("%s: Size %dx%d, Padding %dx%d, StartPt (%d, %d), ContentSize %dx%d\n", tag,
           area.size.w, area.size.h, area.padding.w, area.padding.h,
           area.startPt.x, area.startPt.y, area.contentSize().w, area.contentSize().h);
    } else {
        printf("%s: Size %dx%d\n", tag, area.size.w, area.size.h);
    }
#endif
}

template<class T>
inline bool isEqual(T value, T expect)
{
    if(value != expect) {
        print("[Value ]", value);
        print("[Expect]", expect);

        return false;
    }

    return true;
}

#define MYEXPECT_EQ(val1, val2) EXPECT_TRUE(isEqual(val1, val2))

class StereoTuningProviderUTBase: public ::testing::Test
{
public:
    StereoTuningProviderUTBase()
    {
        int input;
        printf("Select scenario:\n"
               " [0]Preview\n"
               " [1]Record\n"
               " [2]Capture:\n"
               " > ");
        cin >> input;
        switch(input)
        {
            case 0:
            default:
                _scenario = eSTEREO_SCENARIO_PREVIEW;
                break;
            case 1:
                _scenario = eSTEREO_SCENARIO_RECORD;
                break;
            case 2:
                _scenario = eSTEREO_SCENARIO_CAPTURE;
                break;
        }

        printf("Select feature mode:\n"
               " [0]VSDoF\n"
               " [1]Mtkdepthmap\n"
               " [2]3rd Party\n"
               " [3]Zoom\n"
               " [4]Multicam \n"
               " > ");

        cin >> input;
        const MUINT32 EIS_FACTOR = 119;
        const bool IS_PORTRAIT = false;
        const bool IS_RECORD = (_scenario == eSTEREO_SCENARIO_RECORD);
        switch(input) {
            case 0:
            default:
                StereoSettingProvider::setStereoFeatureMode(E_STEREO_FEATURE_VSDOF, IS_PORTRAIT, IS_RECORD, EIS_FACTOR);
            break;
            case 1:
                StereoSettingProvider::setStereoFeatureMode(E_STEREO_FEATURE_MTK_DEPTHMAP, IS_PORTRAIT, IS_RECORD, EIS_FACTOR);
            break;
            case 2:
                StereoSettingProvider::setStereoFeatureMode(E_STEREO_FEATURE_THIRD_PARTY, IS_PORTRAIT, IS_RECORD, EIS_FACTOR);
            break;
            case 3:
                StereoSettingProvider::setStereoFeatureMode(E_DUALCAM_FEATURE_ZOOM, IS_PORTRAIT, IS_RECORD, EIS_FACTOR);
            break;
            case 4:
                StereoSettingProvider::setStereoFeatureMode(E_STEREO_FEATURE_MULTI_CAM, IS_PORTRAIT, IS_RECORD, EIS_FACTOR);
            break;
        }

        //Update sensor sceanrio
        MUINT sensorScenarioMain1;
        MUINT sensorScenarioMain2;
        StereoSettingProvider::getSensorScenario(
                                StereoSettingProvider::getStereoFeatureMode(),
                                false,
                                sensorScenarioMain1,
                                sensorScenarioMain2,
                                true);

        printf("Select preview size:\n"
               "  [0] 1920x1080 (16:9)\n"
               "  [1] 1440x1080 ( 4:3)\n"
               "  [2] 2280x1080 (19:9)\n"
               "  [3] 2160x1080 (18:9)\n"
               "  [4] 2268x1080 (21:10)\n"
               "  [5]  640x480  ( 4:3)\n"
               "  [6]  640x360  (16:9)\n"
               "  [7] 1280x720  (16:9)\n"
               "  [99] Manually input size\n"
               " > "
               );
        MSize pvSize = [&] {
            cin >> input;
            switch(input) {
                case 0:
                default:
                    return MSize(1920, 1080);
                case 1:
                    return MSize(1440, 1080);
                case 2:
                    return MSize(2280, 1080);
                case 3:
                    return MSize(2160, 1080);
                case 4:
                    return MSize(2268, 1080);
                case 5:
                    return MSize(640, 480);
                case 6:
                    return MSize(640, 360);
                case 7:
                    return MSize(1280, 720);
                case 99:
                    {
                        printf("Please input size with format WxH: ");
                        MSize size;
                        std::string s;
                        cin >> s;
                        sscanf(s.c_str(), "%dx%d", &size.w, &size.h);
                        return size;
                    }
            }
        }();
        StereoSettingProvider::setPreviewSize(pvSize);

        printf("============================\n");
        switch(_scenario)
        {
            case eSTEREO_SCENARIO_PREVIEW:
            default:
                printf("Scenario: Preview\n");
                break;
            case eSTEREO_SCENARIO_CAPTURE:
                printf("Scenario: Capture\n");
                break;
            case eSTEREO_SCENARIO_RECORD:
                printf("Scenario: Record\n");
                break;
        }

        switch(StereoSettingProvider::getStereoFeatureMode()) {
        case E_STEREO_FEATURE_VSDOF:
        default:
            printf("Feature Mode: VSDoF\n");
            break;
        // case E_STEREO_FEATURE_ACTIVE_STEREO:
        //     printf("Feature Mode: ActiveStereo\n");
        //     break;
        }

        printf("Image Ratio: %s\n", (const char *)StereoSettingProvider::imageRatio());

        printf("============================\n");
    }

    virtual ~StereoTuningProviderUTBase() {}

protected:
    virtual void SetUp() {
        StereoTuningProvider::getDPETuningInfo(&_config, _scenario);
    }

    virtual void TearDown() {}

    virtual void init() {}

protected:
    ENUM_STEREO_SCENARIO _scenario;
    DPE_Config _config;
};

#endif