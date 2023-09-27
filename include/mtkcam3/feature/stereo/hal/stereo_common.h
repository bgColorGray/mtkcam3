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
#ifndef _STEREO_COMMON_H_
#define _STEREO_COMMON_H_

#define DUMP_START_CAPTURE  3000
#define WITH16ALIGN true
#define STEREO_PROPERTY_PREFIX  "vendor.STEREO."

#include <cstdint>
#include <math.h>
#include <vector>
#include <string>
#include <mtkcam/def/common.h>
#include <cutils/properties.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/imgbuf/IImageBuffer.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <mtkcam/drv/IHalSensor.h>
#pragma GCC diagnostic pop
#include <camera_custom_stereo.h>   //for ENUM_STEREO_CAM_SCENARIO
#include <sys/stat.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_keys.h>
#include <mtkcam3/feature/stereo/StereoCamEnum.h>
#include <sstream>
#include <iomanip>
#include <utility>
#include <algorithm>

namespace StereoHAL {

enum ENUM_STEREO_SCENARIO
{
    eSTEREO_SCENARIO_UNKNOWN,
    eSTEREO_SCENARIO_PREVIEW,
    eSTEREO_SCENARIO_RECORD,
    eSTEREO_SCENARIO_CAPTURE,
    TOTAL_STEREO_SCENARIO
};

enum ENUM_STEREO_SENSOR
{
    eSTEREO_SENSOR_UNKNOWN,
    eSTEREO_SENSOR_MAIN1,
    eSTEREO_SENSOR_MAIN2,
    eSTEREO_SENSOR_MAIN3,
    eSTEREO_SENSOR_MAIN4
};

typedef struct AF_WIN_COORDINATE_STRUCT
{
    MINT af_win_start_x;
    MINT af_win_start_y;
    MINT af_win_end_x;
    MINT af_win_end_y;

    inline  AF_WIN_COORDINATE_STRUCT()
            : af_win_start_x(0)
            , af_win_start_y(0)
            , af_win_end_x(0)
            , af_win_end_y(0)
            {
            }

    inline  AF_WIN_COORDINATE_STRUCT(MINT startX, MINT startY, MINT endX, MINT endY)
            : af_win_start_x(startX)
            , af_win_start_y(startY)
            , af_win_end_x(endX)
            , af_win_end_y(endY)
            {
            }

    inline  NSCam::MPoint centerPoint()
            {
                return NSCam::MPoint((af_win_start_x+af_win_end_x)/2, (af_win_start_y+af_win_end_y)/2);
            }

} *P_AF_WIN_COORDINATE_STRUCT;

typedef struct SENSOR_PHYSICAL_INFO_T
{
    int   orientation    = 0;    // MTK_SENSOR_ORIENTATION
    float focalLength    = 0.0f; // MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS
    float physicalWidth  = 0.0f; // MTK_SENSOR_INFO_PHYSICAL_SIZE[0]
    float physicalHeight = 0.0f; // MTK_SENSOR_INFO_PHYSICAL_SIZE[1]
} *P_SENSOR_PHYSICAL_INFO_T;

/**
 * \brief Get stereo scenario
 * \details Note that record sensor scenario is unsupported
 *
 * \param sensorCenario Sensor scenario
 * \return sensor scenario
 */
inline ENUM_STEREO_SCENARIO getStereoSenario(int sensorCenario)
{
    switch(sensorCenario) {
        case NSCam::SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
        case NSCam::SENSOR_SCENARIO_ID_CUSTOM2:
            return eSTEREO_SCENARIO_RECORD;
            break;
        case NSCam::SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
        case NSCam::SENSOR_SCENARIO_ID_CUSTOM1:
            return eSTEREO_SCENARIO_CAPTURE;
            break;
        default:
            break;
    }

    return eSTEREO_SCENARIO_UNKNOWN;
}

/**
* \brief Diff two time
* \details Diff two time, sample code:
*           struct timespec t_start, t_end, t_result;
*           clock_gettime(CLOCK_MONOTONIC, &t_start);
*           ...
*           clock_gettime(CLOCK_MONOTONIC, &t_end);
*           t_result = timeDiff(t_start, t_end);
*           ALOGD("Runnning Time: %lu.%.9lu", t_result.tv_sec, t_result.tv_nsec);
*
* \param timespec Start of the time
* \param timespec End of the time
*
* \return Diff of two times
*/
inline struct timespec timeDiff( struct timespec start, struct timespec end)
{
    struct timespec t_result;

    if( ( end.tv_nsec - start.tv_nsec ) < 0) {
        t_result.tv_sec = end.tv_sec - start.tv_sec - 1;
        t_result.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        t_result.tv_sec = end.tv_sec - start.tv_sec;
        t_result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return t_result;
}

/**
 * \brief Check system property's value, only for number values
 * \details Check system property's value, only for number values
 *
 * \param PROPERTY_NAME The property to query, e.g. "debug.STEREO.enable_verify"
 * \param DEFAULT Default value of the property. Default is 0.
 * \return -1: property not been set; otherwise the property value
 */
inline int checkStereoProperty(const char *PROPERTY_NAME, const int DEFAULT=0)
{
    return property_get_int32(PROPERTY_NAME, DEFAULT);
}

/**
 * \brief Set system property
 *
 * \param PROPERTY_NAME Property to set
 * \param val value of the property
 *
 * \return true if success
 */
inline bool setProperty(const char *PROPERTY_NAME, int val)
{
    if(NULL == PROPERTY_NAME) {
        return false;
    }

    char value[PROPERTY_VALUE_MAX];
    int ret = 1;
    if(sprintf(value, "%d", val) >= 0)
    {
        ret = property_set(PROPERTY_NAME, value);
    }

    return (0 == ret);
}

inline ENUM_STEREO_CAM_SCENARIO transferStereoScenario(ENUM_STEREO_SCENARIO s)
{
    switch(s) {
        case eSTEREO_SCENARIO_PREVIEW:
        default:
            return eStereoCamPreview;
            break;
        case eSTEREO_SCENARIO_RECORD:
            return eStereoCamRecord;
            break;
        case eSTEREO_SCENARIO_CAPTURE:
            return eStereoCamCapture;
            break;
    }

    return eStereoCamPreview;
}

inline NSCam::MPoint
rotatePoint(NSCam::MPoint ptIn, const NSCam::MSize DOMAIN_SIZE_HORIZONTAL, int rotate)
{
    if(0 == rotate) {
        return ptIn;
    }

    NSCam::MPoint newPt = ptIn;
    switch(rotate) {
        case 90:
            newPt.x = DOMAIN_SIZE_HORIZONTAL.h - ptIn.y;
            newPt.y = ptIn.x;
            break;
        case 180:
            newPt.x = DOMAIN_SIZE_HORIZONTAL.w - ptIn.x;
            newPt.y = DOMAIN_SIZE_HORIZONTAL.h - ptIn.y;
            break;
        case 270:
            newPt.x = ptIn.y;
            newPt.y = DOMAIN_SIZE_HORIZONTAL.w - ptIn.x;
            break;
        default:
            break;
    }

    return newPt;
}

inline NSCam::MRect
rotateRect(NSCam::MRect rect, const NSCam::MSize DOMAIN_SIZE_HORIZONTAL, int rotate)
{
    if(0 == rotate) {
        return rect;
    }

    NSCam::MRect newRect = rect;
    switch(rotate) {
        case 90:
            newRect.p = rotatePoint(rect.leftBottom(), DOMAIN_SIZE_HORIZONTAL, rotate);
            std::swap(newRect.s.w, newRect.s.h);
            break;
        case 180:
            newRect.p = rotatePoint(rect.rightBottom(), DOMAIN_SIZE_HORIZONTAL, rotate);
            break;
        case 270:
            newRect.p = rotatePoint(rect.rightTop(), DOMAIN_SIZE_HORIZONTAL, rotate);
            std::swap(newRect.s.w, newRect.s.h);
            break;
        default:
            break;
    }

    return newRect;
}

inline unsigned int StereoGCD(unsigned int u, unsigned int v)
{
    //The binary GCD algorithm, also known as Stein's algorithm
    if(0 == u) return v;
    if(0 == v) return u;

    //__builtin_ctz: count the number of tail zeros
    //equals to while ((v & 1) == 0) v >>= 1;
    int shift = __builtin_ctz(u | v);
    u >>= __builtin_ctz(u);
    unsigned int t = 0;
    do {
        v >>= __builtin_ctz(v);
        if (u > v) {
            t = v;
            v = u;
            u = t;
        }
        v = v - u;
    } while (v != 0);

    return u << shift;
}

/**
 * \brief Convert degree to radians
 *
 * \param degree degree
 * \return positive radians
 */
inline float degreeToRadians(float degree)
{
    degree = fmod(degree, 360.0f);
    if(degree < 0.0f) {
        degree += 360.0f;
    }
    return degree * M_PI/180.0f;
}

/**
 * \brief Convert radians to degree
 *
 * \param radians radians
 * \return positive degree
 */
inline float radiansToDegree(float radians)
{
    radians = fmod(radians, 2.0f*M_PI);
    if(radians < 0.0f) {
        radians += 2.0f*M_PI;
    }
    return radians * 180.0f/M_PI;
}

/**
 * \brief Ratio of degrees degree1/degree2
  *
 * \param degree1 Degree1
 * \param degree2 Degree2
 *
 * \return Ratio of degree, e.g. 50deg/60deg->0.8076
 */
inline float degreeRatio(float degree1, float degree2)
{
    return tan(degreeToRadians(degree1/2.0f))/tan(degreeToRadians(degree2/2.0f));
}

/**
 * \brief Ratio of radians/radians2
 *
 * \param radians1 Radians1
 * \param radians2 Radians2
 *
 * \return Ratio of radians'
 */
inline float radiansRatio(float radians1, float radians2)
{
    return tan(radians1/2.0f)/tan(radians2/2.0f);
}

class STEREO_IMAGE_RATIO_T
{
public:
    STEREO_IMAGE_RATIO_T() {}

    STEREO_IMAGE_RATIO_T(int m, int n, bool printInFixedWidth=false)
    {
        __initByInt(m, n, printInFixedWidth);
    }

    STEREO_IMAGE_RATIO_T(float m, float n, bool printInFixedWidth=false)
    {
        __initByFloat(m, n, printInFixedWidth);
    }

    // Construct by ratio string, e.g. "16:9", "19.5:9"
    explicit STEREO_IMAGE_RATIO_T(std::string ratioString, bool printInFixedWidth=false)
    {
        float m, n;
        if(sscanf(ratioString.c_str(), "%f:%f", &m, &n) < 2)
        {
            //Do nothing, just for code scan fix
            ALOGE("Fail to init ratio by string \"%s\"", ratioString.c_str());
            return;
        }

        __initByFloat(m, n, printInFixedWidth);
    }

    // Construct by image size, e.g. 1920x1080=>16:9, 2340x1080=>19.5:9
    explicit STEREO_IMAGE_RATIO_T(NSCam::MSize size, bool printInFixedWidth=false)
    {
        // Since 1088 is 16-aligned result of 1080, we use 1080 to calculate ratio
        if(size.h == 1088) {
            size.h = 1080;
        }

        float m = static_cast<float>(size.w);
        float n = static_cast<float>(size.h);

        // Handle special case for xx.x : 9
        if(size.h % 90 == 0 &&
           size.w % (size.h/90) == 0)
        {
            n = 9.0f;
            m /= (size.h/9);
        }

        __initByFloat(m, n, printInFixedWidth);
    }

    // Construct by legacy ratio
    explicit STEREO_IMAGE_RATIO_T(ENUM_STEREO_RATIO r, bool printInFixedWidth=false)
    {
        int m, n;
        __imageRatioMToN(r, m, n);
        __initByInt(m, n, printInFixedWidth);
    }

    // operator to STEREO_IMAGE_RATIO_T
    inline bool operator==(const STEREO_IMAGE_RATIO_T &rhs) const
    {
        return (__m == rhs.__m && __n == rhs.__n);
    }

    inline bool operator!=(const STEREO_IMAGE_RATIO_T &rhs) const
    {
        return (__m != rhs.__m || __n != rhs.__n);
    }

    inline STEREO_IMAGE_RATIO_T &operator=(const STEREO_IMAGE_RATIO_T &rhs)
    {
        __m = rhs.__m;
        __n = rhs.__n;
        __ratioString = rhs.__ratioString;
        return *this;
    }

    // operator to ENUM_STEREO_RATIO(legacy)
    inline bool operator==(const ENUM_STEREO_RATIO &r) const
    {
        int m, n;
        __imageRatioMToN(r, m, n);
        return (__m == m && __n == n);
    }

    inline bool operator!=(const ENUM_STEREO_RATIO &r) const
    {
        int m, n;
        __imageRatioMToN(r, m, n);
        return (__m != m || __n != n);
    }

    inline STEREO_IMAGE_RATIO_T &operator=(const ENUM_STEREO_RATIO &r)
    {
        int m, n;
        __imageRatioMToN(r, m, n);
        __initByInt(m, n);
        return *this;
    }

    inline void MToN(int &m, int &n) const {
        m = __m;
        n = __n;
    }

    inline operator std::string() const {
        return __ratioString;
    }

    inline operator const char *() const {
        return __ratioString.c_str();
    }

    inline operator size_t() const {
        return (__m << 16 | __n);
    }

private:
    void __initByInt(int m, int n, bool printInFixedWidth=false)
    {
        int gcd = StereoGCD(m, n);
        __m = m/gcd;
        __n = n/gcd;

        // Prepare string
        std::ostringstream oss;
        if (printInFixedWidth) {
            oss << std::setw(4) << std::right;
        }
        oss << __m << ":";

        if (printInFixedWidth) {
            oss << std::left;
        }
        oss << __n;
        __ratioString = oss.str();
    }

    void __initByFloat(float m, float n, bool printInFixedWidth=false)
    {
        int a = static_cast<int>(m*10.0f);
        int b = static_cast<int>(n*10.0f);
        if(a % 10 == 0 &&
           b % 10 == 0)
        {
            __initByInt(m, n, printInFixedWidth);
            return;
        }

        int gcd = StereoGCD(a, b);
        __m = a/gcd;
        __n = b/gcd;

        // Prepare string
        std::ostringstream oss;
        if (printInFixedWidth) {
            oss << std::setw(4) << std::right;
        }

        if (a % 10 == 0) {
            a /= 10;
            oss << a;
        } else {
            oss << std::fixed << std::setprecision(1) << m;
        }
        oss << ":";
        if (printInFixedWidth) {
            oss << std::left;
        }
        if (b % 10 == 0) {
            b /= 10;
            oss << b;
        } else {
            oss << std::fixed << std::setprecision(1) << b;
        }

        __ratioString = oss.str();
    }

    inline void __imageRatioMToN(ENUM_STEREO_RATIO ratio, int &m, int &n) const
    {
        if(eRatio_Unknown == ratio) {
            ratio = eRatio_Default;
        }

    #ifdef STEREO_RATIO_CONSEQUENT_BITS
        m = ratio >> STEREO_RATIO_CONSEQUENT_BITS;
        n = ratio & STEREO_RATIO_CONSEQUENT_MASK;
    #else
        switch(ratio) {
        case eRatio_16_9:
            m = 16;
            n = 9;
            break;
        case eRatio_4_3:
            m = 4;
            n = 3;
            break;
        default:
            __imageRatioMToN(eRatio_Default, m, n);
            break;
        }
    #endif
    }

private:
    int __m = 16;
    int __n = 9;
    std::string __ratioString;
};

const STEREO_IMAGE_RATIO_T eStereoRatio_16_9 = STEREO_IMAGE_RATIO_T(16, 9);
const STEREO_IMAGE_RATIO_T eStereoRatio_4_3  = STEREO_IMAGE_RATIO_T(4, 3);

inline int applyNAlign(unsigned int n, int &value) {
    if(n > 1 &&
       __builtin_popcount(n) == 1)  //must be power of 2, i.e. 2, 4, 8, 16, etc
    {
        int m = n-1;    //Up scale
        value = (value+m) & (~m);
    }
    return value;
}

inline int applyNAlignRounding(unsigned int n, int &value) {
    if(n > 1 &&
       __builtin_popcount(n) == 1)  //must be power of 2, i.e. 2, 4, 8, 16, etc
    {
        int m = n>>1;
        value = (value+m) & (~(n-1));
    }
    return value;
}

inline void fitSizeToImageRatio(NSCam::MSize &size, STEREO_IMAGE_RATIO_T ratio)
{
    int m, n;
    ratio.MToN(m, n);
    int baseSize = std::min(size.w/(m*2), size.h/(n*2));
    size.w = baseSize * m * 2;
    size.h = baseSize * n * 2;
}

inline std::string sizeToString(const NSCam::MSize size)
{
    return std::to_string(size.w)+"x"+std::to_string(size.h);
}

inline MUINT stringToSensorScenario(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    //Preview
    if(0 == s.compare("preview")) {
        return NSCam::SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
    }
    //Capture
    if(0 == s.compare("capture")) {
        return NSCam::SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
    }
    //Video
    if(0 == s.compare("video")) {
        return NSCam::SENSOR_SCENARIO_ID_NORMAL_VIDEO;
    }
    //High speed video
    if(0 == s.compare("hsvideo")) {
        return NSCam::SENSOR_SCENARIO_ID_SLIM_VIDEO1;
    }
    //Slim video
    if(0 == s.compare("slimvideo")) {
        return NSCam::SENSOR_SCENARIO_ID_SLIM_VIDEO2;
    }
    //Video
    if(0 == s.compare("video")) {
        return NSCam::SENSOR_SCENARIO_ID_NORMAL_VIDEO;
    }
    //Custom1
    if(0 == s.compare("custom1")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM1;
    }
    //Custom2
    if(0 == s.compare("custom2")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM2;
    }
    //Custom3
    if(0 == s.compare("custom3")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM3;
    }
    //Custom4
    if(0 == s.compare("custom4")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM4;
    }
    //Custom5
    if(0 == s.compare("custom5")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM5;
    }
    //Custom6
    if(0 == s.compare("custom6")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM6;
    }
    //Custom7
    if(0 == s.compare("custom7")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM7;
    }
    //Custom8
    if(0 == s.compare("custom8")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM8;
    }
    //Custom9
    if(0 == s.compare("custom9")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM9;
    }
    //Custom10
    if(0 == s.compare("custom10")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM10;
    }
    //Custom11
    if(0 == s.compare("custom11")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM11;
    }
    //Custom12
    if(0 == s.compare("custom12")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM12;
    }
    //Custom13
    if(0 == s.compare("custom13")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM13;
    }
    //Custom14
    if(0 == s.compare("custom14")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM14;
    }
    //Custom15
    if(0 == s.compare("custom15")) {
        return NSCam::SENSOR_SCENARIO_ID_CUSTOM15;
    }

    return NSCam::SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
}

inline std::string sensorScenarioToString(MUINT s)
{
    switch(s)
    {
    case NSCam::SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
        return std::string("preview");
        break;
    case NSCam::SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
    default:
        return std::string("capture");
        break;
    case NSCam::SENSOR_SCENARIO_ID_NORMAL_VIDEO:
        return std::string("video");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM1:
        return std::string("custom1");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM2:
        return std::string("custom2");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM3:
        return std::string("custom3");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM4:
        return std::string("custom4");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM5:
        return std::string("custom5");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM6:
        return std::string("custom6");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM7:
        return std::string("custom7");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM8:
        return std::string("custom8");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM9:
        return std::string("custom9");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM10:
        return std::string("custom10");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM11:
        return std::string("custom11");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM12:
        return std::string("custom12");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM13:
        return std::string("custom13");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM14:
        return std::string("custom14");
        break;
    case NSCam::SENSOR_SCENARIO_ID_CUSTOM15:
        return std::string("custom15");
        break;
    }
}

inline int stringToFeatureMode(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    int featuerMode = 0;

    //StereoCapture
    if(0 == s.compare(CUSTOM_KEY_STEREO_CAPTURE)) {
        featuerMode = NSCam::v1::Stereo::E_STEREO_FEATURE_CAPTURE;
    }
    //VSDoF
    if(s.find(CUSTOM_KEY_VSDOF) != std::string::npos) {
        featuerMode = NSCam::v1::Stereo::E_STEREO_FEATURE_VSDOF;
    }
    //Denoise
    if(0 == s.compare(CUSTOM_KEY_DENOISE)) {
        featuerMode = NSCam::v1::Stereo::E_STEREO_FEATURE_DENOISE;
    }
    //3rdParty
    if(0 == s.compare(CUSTOM_KEY_3RD_Party)) {
        featuerMode = NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY;
    }
    //Zoom
    if(0 == s.compare(CUSTOM_KEY_ZOOM)) {
        featuerMode = NSCam::v1::Stereo::E_DUALCAM_FEATURE_ZOOM;
    }
    //MtkDepthmap
    if(s.find(CUSTOM_KEY_MTK_DEPTHMAP) != std::string::npos) {
        featuerMode = NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP;
    }
    //Multi-cam
    if(s.find(CUSTOM_KEY_MULTI_CAM) != std::string::npos) {
        featuerMode = NSCam::v1::Stereo::E_STEREO_FEATURE_MULTI_CAM;
    }
    //Secure-cam
    if(s.find(CUSTOM_KEY_SECURE_CAMERA) != std::string::npos) {
        featuerMode = NSCam::v1::Stereo::E_SECURE_CAMERA;
    }

    if((s.find("+portrait") != std::string::npos)
        || (s.find("+p") != std::string::npos)
        || (s.find("+half") != std::string::npos))
    {
        featuerMode |= NSCam::v1::Stereo::E_STEREO_FEATURE_PORTRAIT_FLAG;
    }

    if((s.find("+hidl") != std::string::npos))
    {
        featuerMode |= NSCam::v1::Stereo::E_MULTICAM_FEATURE_HIDL_FLAG;
    }

    return featuerMode;
}

inline std::vector<std::string> featureModeToStrings(int featureMode)
{
    std::vector<std::string> features;
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_CAPTURE) {
        features.push_back(CUSTOM_KEY_STEREO_CAPTURE);
    }
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_VSDOF) {
        features.push_back(CUSTOM_KEY_VSDOF);
    }
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_DENOISE) {
        features.push_back(CUSTOM_KEY_DENOISE);
    }
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY) {
        features.push_back(CUSTOM_KEY_3RD_Party);
    }
    if(featureMode & NSCam::v1::Stereo::E_DUALCAM_FEATURE_ZOOM) {
        features.push_back(CUSTOM_KEY_ZOOM);
    }
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP) {
        features.push_back(CUSTOM_KEY_MTK_DEPTHMAP);
    }
    if(featureMode & NSCam::v1::Stereo::E_STEREO_FEATURE_MULTI_CAM) {
        features.push_back(CUSTOM_KEY_MULTI_CAM);
    }
    if(featureMode & NSCam::v1::Stereo::E_SECURE_CAMERA) {
        features.push_back(CUSTOM_KEY_SECURE_CAMERA);
    }

    return features;
}

inline NSCam::MSize stringToSize(std::string str)
{
    NSCam::MSize size;
    if(sscanf(str.c_str(), "%dx%d", &size.w, &size.h) <= 0)
    {
        //Do nothing, just for code scan fix
    }
    return size;
}

inline void base64Decode(const char *base64Str, std::vector<MUINT8> &buffer)
{
    size_t base64_len = strlen(base64Str);
    size_t out_len = base64_len/4 * 3;
    for(int i = base64_len-1; i >= 0; --i) {
        if('=' == base64Str[i]) {
            out_len--;
            base64_len--;
        } else {
            break;
        }
    }
    // MY_LOGD("Base64 size %zu, out size %zu", base64_len, out_len);
    if(buffer.size() != out_len) {
        buffer.resize(out_len);
    }

    static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

    size_t i = 0;
    int in_pos = 0;
    int out_pos = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (base64_len--) {
        char_array_4[i++] = base64_chars.find(base64Str[in_pos++]);
        if (i == 4) {
            buffer[out_pos++] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
            buffer[out_pos++] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            buffer[out_pos++] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

            i = 0;
        }
    }

    if (i) {
        char_array_3[0] = ( char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (size_t j = 0; (j < i - 1); j++) {
            buffer[out_pos++] = char_array_3[j];
        }
    }
}

inline bool isHorizontal(const int degree)
{
    return (degree == 0 || degree == 180);
}

inline bool isVertical(const int degree)
{
    return (degree == 90 || degree == 270);
}

inline bool isSlant(const int degree)
{
    return !(isHorizontal(degree) || isVertical(degree));
}

};

namespace std {
    template <>
    struct std::hash<StereoHAL::STEREO_IMAGE_RATIO_T>
    {
        size_t operator()(const StereoHAL::STEREO_IMAGE_RATIO_T& k) const
        {
            return (size_t)k;
        }
    };
}   // namespace std
#endif