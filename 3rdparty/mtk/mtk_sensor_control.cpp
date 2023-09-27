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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#define LOG_TAG "mtkcam-mtk_sensor_control"
//
#include <cutils/properties.h>
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/common.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
//
#include <mtkcam3/3rdparty/mtk/mtk_sensor_control.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
//
#include <map>
#include <memory>
#include <vector>
#include <unordered_map>
#include <math.h>

/******************************************************************************
 *
 ******************************************************************************/
#define __DEBUG // enable function scope debug
#ifdef __DEBUG
#include <memory>

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);
#define FUNCTION_SCOPE \
 auto __scope_logger__ = [](char const* f)->std::shared_ptr<const char>{ \
    CAM_ULOGMD("(%d)[%s] + ", ::gettid(), f); \
    return std::shared_ptr<const char>(f, [](char const* p){CAM_ULOGMD("(%d)[%s] -", ::gettid(), p);}); \
}(__FUNCTION__)
#else
#define FUNCTION_SCOPE
#endif

#define MAX_IDLE_DECIDE_FRAME_COUNT 160
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
/******************************************************************************
 *
 ******************************************************************************/

#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
/******************************************************************************
 *
 ******************************************************************************/

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

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace sensorcontrol {

static float sPreZoomRatio = .0f;

static bool convertAppCropRegionForEachSensor(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    auto iter = in.vInfo.find(in.vSensorIdList[0]);
    if(iter == in.vInfo.end())
    {
        MY_LOGE("cannot find in vInfo. sensorId(%d)", in.vSensorIdList[0]);
        return false;
    }
    auto masterInfo = iter->second;
    MRect appCropRegion;

    // check debug prop
    auto iDebugZoomRatio = property_get_int32("vendor.debug.camera.zoomRatio", 0);
    float fDebugZoomRatio = (iDebugZoomRatio * 1.0f) / 10;

    if (iDebugZoomRatio) {
        appCropRegion.s.w = (float)masterInfo->mActiveArrayRegion.s.w / fDebugZoomRatio;
        appCropRegion.s.h = (float)masterInfo->mActiveArrayRegion.s.h / fDebugZoomRatio;
        appCropRegion.p.x = ((float)masterInfo->mActiveArrayRegion.s.w - (float)appCropRegion.s.w) * .5f;
        appCropRegion.p.y = ((float)masterInfo->mActiveArrayRegion.s.h - (float)appCropRegion.s.h) * .5f;
    }
    else if (in.mZoomRatio != .0f)
    {
        // if zoom ratio not equal to 0, it needs to convert zoom ratio to cam::0 crop size.
        // used to simulate app crop region.
        appCropRegion.s.w = (float)masterInfo->mActiveArrayRegion.s.w / (float)in.mZoomRatio;
        appCropRegion.s.h = (float)masterInfo->mActiveArrayRegion.s.h / (float)in.mZoomRatio;
        appCropRegion.p.x = ((float)masterInfo->mActiveArrayRegion.s.w - (float)appCropRegion.s.w) * .5f;
        appCropRegion.p.y = ((float)masterInfo->mActiveArrayRegion.s.h - (float)appCropRegion.s.h) * .5f;
    }
    else
    {
        appCropRegion = in.mrCropRegion;
    }
    float zoomRatioW = (float)masterInfo->mActiveArrayRegion.s.w / (float)appCropRegion.s.w;
    //float zoomRatioH = (float)masterInfo->mActiveArrayRegion.s.h / (float)appCropRegion.s.h;
    float defaultFov_H  = masterInfo->fSensorFov_H;
    float defaultFov_V  = masterInfo->fSensorFov_V;
    out.fZoomRatio = zoomRatioW;
    MY_LOGD("debug(%d), Zoom ratio(%f), appCropRegion(%d, %d), %dx%d",
            iDebugZoomRatio != 0 ? true : false,
            out.fZoomRatio, appCropRegion.p.x, appCropRegion.p.y, appCropRegion.s.w, appCropRegion.s.h);

    if (iDebugZoomRatio) {
        for(auto& item:in.vInfo)
        {
            auto sSensorControlResult = std::make_shared<SensorControlResult>();
            sSensorControlResult->iSensorControl = SensorStatus::E_STREAMING;
            sSensorControlResult->mrAlternactiveCropRegion = appCropRegion;
            sSensorControlResult->mrAlternactiveCropRegion_Cap = appCropRegion;
            sSensorControlResult->mrAlternactiveCropRegion_3A = appCropRegion;
            out.vResult.insert({item.first, sSensorControlResult});
        }
    }
    else{
        // for tricam, convert crop region to each sensor.
        for(auto& item:in.vInfo)
        {
            auto sSensorControlResult = std::make_shared<SensorControlResult>();
            sSensorControlResult->iSensorControl = SensorStatus::E_STREAMING;

            auto sensorInfo = item.second;
            // app view
            if (sensorInfo->fSensorFov_H == defaultFov_H)
            {
                // check wide sensor's active array region
                int targetWidth = appCropRegion.s.w;
                int targetHeight = appCropRegion.s.h;
                auto targetPointX = (sensorInfo->mActiveArrayRegion.s.w - targetWidth)/2 + sensorInfo->mActiveArrayRegion.p.x;
                auto targetPointY = (sensorInfo->mActiveArrayRegion.s.h - targetHeight)/2 + sensorInfo->mActiveArrayRegion.p.y;
                MRect dstRect;
                // Guarantee positive crop for Tg convertion
                // ex., zoomRatio = 0.6x, app crop for wide may be negative
                if (targetPointX < 0 || targetPointY < 0) {
                    dstRect = sensorInfo->mActiveArrayRegion;
                }
                else {
                    dstRect = MRect(MPoint(targetPointX, targetPointY), MSize(targetWidth, targetHeight));
                }

                sSensorControlResult->mrAlternactiveCropRegion = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_Cap = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_3A = dstRect;
            }
            else
            {
                // FOV ratio with radian
                auto degreeRatio_H = degreeRatio(sensorInfo->fSensorFov_H, defaultFov_H);
                auto degreeRatio_V = degreeRatio(sensorInfo->fSensorFov_V, defaultFov_V);

                // defaultH/targetH = 1.3395x
                // out.iZoomRatio / 1.3395x = target zoom ratio
                int targetWidth = int(sensorInfo->mActiveArrayRegion.s.w / ((out.fZoomRatio  * degreeRatio_H)));
                int targetHeight = int(sensorInfo->mActiveArrayRegion.s.h / ((out.fZoomRatio * degreeRatio_V)));
                auto targetPointX = (sensorInfo->mActiveArrayRegion.s.w - targetWidth)/2 + sensorInfo->mActiveArrayRegion.p.x;
                auto targetPointY = (sensorInfo->mActiveArrayRegion.s.h - targetHeight)/2 + sensorInfo->mActiveArrayRegion.p.y;
                MRect dstRect;
                // centro crop, e.g. ori = 100, target = 50, start point =10 ===>target start point= (100-50)/2 + 10 = 35
                if (targetPointX < 0 || targetPointY < 0)
                {
                    dstRect = sensorInfo->mActiveArrayRegion;
                }
                else
                {
                    dstRect = MRect(MPoint(targetPointX, targetPointY), MSize(targetWidth, targetHeight));
                }

                sSensorControlResult->mrAlternactiveCropRegion = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_Cap = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_3A = dstRect;
                MY_LOGD("cam(%d), Zoom ratio(%f), degreeRatio_H(%f), degreeRatio_V(%f),target crop rect(%d, %d), %dx%d",
                    item.first, out.fZoomRatio, degreeRatio_H, degreeRatio_V, dstRect.p.x, dstRect.p.y, dstRect.s.w, dstRect.s.h);
            }
            out.vResult.insert({item.first, sSensorControlResult});
        }
    }
    return true;
}

static bool convert3ARegionsForEachSensor(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    auto iter = in.vInfo.find(in.vSensorIdList[0]);
    if(iter == in.vInfo.end())
    {
        MY_LOGE("cannot find in vInfo. sensorId(%d)", in.vSensorIdList[0]);
        return false;
    }
    auto masterInfo = iter->second;
    MRect appCropRegion;
    if(in.mZoomRatio != .0f)
    {
        // if zoom ratio not equal to 0, it needs to convert zoom ratio to cam::0 crop size.
        // used to simulate app crop region.
        appCropRegion.s.w = (float)masterInfo->mActiveArrayRegion.s.w / (float)in.mZoomRatio;
        appCropRegion.s.h = (float)masterInfo->mActiveArrayRegion.s.h / (float)in.mZoomRatio;
        appCropRegion.p.x = ((float)masterInfo->mActiveArrayRegion.s.w - (float)appCropRegion.s.w) * .5f;
        appCropRegion.p.y = ((float)masterInfo->mActiveArrayRegion.s.h - (float)appCropRegion.s.h) * .5f;
    }
    else
    {
        appCropRegion = in.mrCropRegion;
    }
    float defaultFov_H  = masterInfo->fSensorFov_H;
    float defaultFov_V  = masterInfo->fSensorFov_V;

    // for tricam, convert crop region to each sensor.
    for(auto& item:in.vInfo)
    {
        std::vector<MINT32> dst_afRects;
        std::vector<MINT32> dst_aeRects;
        std::vector<MINT32> dst_awbRects;

        auto iter_v = out.vResult.find(item.first);
        auto sensorInfo = item.second;
        // app view
        dst_afRects.assign(in.mrAfRegion.begin(),in.mrAfRegion.end());
        dst_aeRects.assign(in.mrAeRegion.begin(),in.mrAeRegion.end());
        dst_awbRects.assign(in.mrAwbRegion.begin(),in.mrAwbRegion.end());
        if(sensorInfo->fSensorFov_H != defaultFov_H)
        {
            // FOV ratio with radian
            auto degreeRatio_H = degreeRatio(sensorInfo->fSensorFov_H, defaultFov_H);
            auto degreeRatio_V = degreeRatio(sensorInfo->fSensorFov_V, defaultFov_V);
            if (out.fZoomRatio < 1.0)
            {
                degreeRatio_H = degreeRatio(defaultFov_H, sensorInfo->fSensorFov_H);
                degreeRatio_V = degreeRatio(defaultFov_V, sensorInfo->fSensorFov_V);
            }
            // for AE/AWB/AF region convert
            // for af
            MRect ActiveArrayWide = masterInfo->mActiveArrayRegion;
            MRect  mrActiveArray = sensorInfo->mActiveArrayRegion;
            auto transformRegions = [&](std::vector<MINT32> region,std::vector<MINT32> resultRegion){
                for (MINT32 i = 0;i<region.size()/5;i++){
                    MINT32 mrRegion_p_xmax,mrRegion_p_ymax,mrRegion_p_xmin,mrRegion_p_ymin;
                    mrRegion_p_xmin = (MINT32)((region[i*5+0] / ((float)(ActiveArrayWide.s.w))* degreeRatio_H + (1-degreeRatio_H)/2) * (mrActiveArray.s.w ));
                    mrRegion_p_ymin = (MINT32)((region[i*5+1] / ((float)(ActiveArrayWide.s.h))* degreeRatio_V + (1-degreeRatio_V)/2) * (mrActiveArray.s.h ));
                    mrRegion_p_xmax = mrRegion_p_xmin + region[i*5+2] - region[i*5+0];
                    mrRegion_p_ymax = mrRegion_p_ymin + region[i*5+3] - region[i*5+1];

                    resultRegion[i*5+0] = mrRegion_p_xmin;
                    resultRegion[i*5+1] = mrRegion_p_ymin;
                    resultRegion[i*5+2] = mrRegion_p_xmax;
                    resultRegion[i*5+3] = mrRegion_p_ymax;
                    resultRegion[i*5+4] = region[i*5+4];
                }
                return true;
            };
            transformRegions(in.mrAfRegion,dst_afRects);
            transformRegions(in.mrAeRegion,dst_aeRects);
            transformRegions(in.mrAwbRegion,dst_awbRects);
            // convert end
        }
        if ((iter_v != out.vResult.end()) && (iter_v->second != nullptr)) {
            iter_v->second->mrAlternactiveAfRegion.assign(dst_afRects.begin(),dst_afRects.end());
            iter_v->second->mrAlternactiveAeRegion.assign(dst_aeRects.begin(),dst_aeRects.end());
            iter_v->second->mrAlternactiveAwbRegion.assign(dst_awbRects.begin(),dst_awbRects.end());

            if (iter_v->second->mrAlternactiveAfRegion.size() >= 5) {
                MY_LOGD("ROI Convert: cam(%d) target af rect(%d, %d), %dx%d",
                            item.first,
                            iter_v->second->mrAlternactiveAfRegion[0],
                            iter_v->second->mrAlternactiveAfRegion[1],
                            iter_v->second->mrAlternactiveAfRegion[2] - iter_v->second->mrAlternactiveAfRegion[0],
                            iter_v->second->mrAlternactiveAfRegion[3] - iter_v->second->mrAlternactiveAfRegion[1]);
            }

        } else {
            MY_LOGE("ROI Convert: null ptr!");
        }

    }
    return true;
}

bool updateSensorState(
    SensorControlParamOut &out,
    int32_t sensorId,
    bool isMaster,
    SensorStatus sensorState
)
{
    auto iter = out.vResult.find(sensorId);
    if (iter != out.vResult.end()) {
        if (iter->second != nullptr) {
            iter->second->isMaster = isMaster;
            iter->second->iSensorControl = sensorState;
        }
        else {
            return false;
        }
    }
    return true;
}

static bool decideSensorStateForceStreaming(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    char tempbuf[1024]= "-1";
    ::property_get("vendor.debug.camera.decisionSensorStreaming", tempbuf, "-1");
    bool forceSensorStreaming = (0 != strcmp(tempbuf, "-1"));

    if (forceSensorStreaming) {
        char const *delim = ",";
        char* saveptr = NULL;
        char * pch;

        for (auto const &sensor : in.vSensorIdList) {
            updateSensorState(out, sensor, false, SensorStatus::E_STANDBY);
        }

        pch = strtok_r(tempbuf, delim, &saveptr);
        int firstSensorId = atoi(pch);
        pch = strtok_r(NULL, delim, &saveptr);

        // first sensor is master
        updateSensorState(out, firstSensorId, true, SensorStatus::E_STREAMING);

        // remaining sensors
        while (pch != NULL)
        {
            int value = atoi(pch);
            updateSensorState(out, value, false, SensorStatus::E_STREAMING);
            pch = strtok_r(NULL, delim, &saveptr);
        }
        return true;
    }
    return false;
}

static bool decideSensorStateZoomRatio(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    MY_LOGD("%f", out.fZoomRatio);
    // handle empty sensor list situation
    if (in.vSensorIdList.size() < 1) {
        MY_LOGE("Empty Sensor List");
        return false;
    }
    if (in.mFeatureMode == FeatureMode::E_Bokeh)
    {
        // if sensor count more than 2, it will be set standby.
        // first element is master cam.
        for (size_t i=0;i<in.vSensorIdList.size();++i)
        {
            auto sensorId = in.vSensorIdList[i];
            if (i >= 2)
            {
                updateSensorState(out, sensorId, false, SensorStatus::E_STANDBY);
            }
            if (i == 0)
            {
                updateSensorState(out, sensorId, true, SensorStatus::E_STREAMING);
            }
            if (i == 1)
            {
                updateSensorState(out, sensorId, false, SensorStatus::E_STREAMING);
            }
        }
        return true;
    }
    // Master Sensor Decision
    // master: streaming, slave(s): standby
    // =====================================
    // compare focal length with baseline (w: wide)
    // => smaller focal len, larger fov (u: ultra wide)
    // => larger focal len, smaller fov (t: tele)
    std::unordered_map<char, int> sensor_order;
    auto fov_base = in.vInfo.at(in.vSensorIdList[0])->mFocalLength;
    if (fov_base <= 0.f)
        MY_LOGW("Invalid focal length (%.2f)", fov_base);
    sensor_order.insert({'w', 0});
    for (int i=1; i < in.vSensorIdList.size(); ++i) {
        if (in.vInfo.at(in.vSensorIdList[i])->mFocalLength < fov_base){
            sensor_order.insert({'u', i});
        } else {
            sensor_order.insert({'t', i});
        }
    }
    // default master index 0 (Wide)
    int master_idx = 0;
    const float zoom2TeleRatio = in.mbInSensorZoomEnable ? in.mZoom2TeleRatio : 2.0f;
    if (out.fZoomRatio < 1.0f) {
        // Switch to Ultra Wide when ratio < 1.0
        if (sensor_order.find('u') != sensor_order.end())
            master_idx = sensor_order['u'];
    } else if (out.fZoomRatio >= zoom2TeleRatio) {
        // Switch to Tele when ratio >= 2.0
        if (sensor_order.find('t') != sensor_order.end())
            master_idx = sensor_order['t'];
    }
    // Update Sensor State
    for (int i=0; i < in.vSensorIdList.size(); ++i) {
        if (i == master_idx) {
            updateSensorState(out, in.vSensorIdList[i], true, SensorStatus::E_STREAMING);
        } else {
            updateSensorState(out, in.vSensorIdList[i], false, SensorStatus::E_STANDBY);
        }
    }
    return true;
}

static bool decideSensorStatePip(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    // skip custom streaming sensor id due to hw limitation
    if (in.pipDeviceCount != 1) {
        MY_LOGD("Skip custom streaming sensor id, pipDeviceCount(%d)", in.pipDeviceCount);
        return false;
    }

    // PIP (back+back)
    auto getCustomStreamingSensorId = [&in](){
        // example: use ultra-wide
        int targetId = -1;
        float maxFov = 0;
        for (auto const& item : in.vInfo) {
            if (item.second->fSensorFov_H > maxFov) {
                targetId = item.first;
                maxFov = item.second->fSensorFov_H;
            }
        }
        return targetId;
    };

    int pipDeviceCount = in.pipDeviceCount;
    int customStreamingSensorId = getCustomStreamingSensorId();

    if (customStreamingSensorId != -1) {
        bool isMaster = false;
        auto iter = out.vResult.find(customStreamingSensorId);
        if (iter != out.vResult.end()) {
            if (iter->second != nullptr) {
                isMaster = iter->second->isMaster;
            }
        }
        updateSensorState(out, customStreamingSensorId, isMaster, SensorStatus::E_STREAMING);
        MY_LOGD("CustomStreamingSensorId(%d), pipDeviceCount(%d)",
                customStreamingSensorId, pipDeviceCount);
    } else {
        MY_LOGE("cannot find custom streaming sensor id");
    }

    return true;
}

static bool decideSensorState(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    bool isPip = (in.pipDeviceCount != 0) ? true : false;

    // 1. force streaming (for debug)
    if (decideSensorStateForceStreaming(out, in)) {
        return true;
    }

    // 2. zoom ratio by app crop
    decideSensorStateZoomRatio(out, in);

    // 3. PIP default streaming
    if (isPip) {
        decideSensorStatePip(out, in);
    }

    // Config time, only 1 sensor is streaming
    if (sPreZoomRatio == 0) {
        return true;
    }

    // Request time, pip can't active all cameras
    if (!isPip) {
        // Request time, when user zoom in/out, active all cameras
        // For Hw limitation, this flow only support sensor size is 2.
        if (sPreZoomRatio != 0 && sPreZoomRatio != out.fZoomRatio && in.vSensorIdList.size() == 2) {
            for (auto&& item : out.vResult) {
                item.second->iSensorControl = SensorStatus::E_STREAMING;
            }
            MY_LOGD("sPreZoomRatio=%f, out.fZoomRatio=%f, active slave camera",
                    sPreZoomRatio, out.fZoomRatio);
        }
    }
    sPreZoomRatio = out.fZoomRatio;
    return true;
}

bool mtk_decision_sensor_control_zoom(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    if (!convertAppCropRegionForEachSensor(out, in)) {
        return false;
    }
    if (!convert3ARegionsForEachSensor(out, in)) {
        return false;
    }
    if (!decideSensorState(out, in)) {
        return false;
    }
    return true;
}

bool
mtk_decision_sensor_control_always_streaming(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    out.fZoomRatio = 1.0f;
    // if sensor count more than 2, it will be set standby.
    // first element is master cam.
    int inlistSensor = 0;
    int maxSensorId = 0;
    if (in.physicalSensorIdList.size() > 0)
    {
        for(size_t i=0;i<in.physicalSensorIdList.size();++i){
            auto sensorId = in.physicalSensorIdList[i];
            if (maxSensorId < sensorId) maxSensorId = sensorId;
            SensorStatus iSensorStatus = SensorStatus::E_STREAMING;
            auto sSensorControlResult = std::make_shared<SensorControlResult>();
            sSensorControlResult->iSensorControl = iSensorStatus;
            sSensorControlResult->mrAlternactiveCropRegion = in.mrCropRegion;
            sSensorControlResult->mrAlternactiveCropRegion_Cap = in.mrCropRegion;
            sSensorControlResult->mrAlternactiveCropRegion_3A = in.mrCropRegion;
            out.vResult.insert({sensorId, sSensorControlResult});
            inlistSensor++;
        }
        for(size_t i=0, cnt=0;i<=maxSensorId;i++){
            auto iter = out.vResult.find(i);
            if(iter != out.vResult.end()) {
                if(iter->second->iSensorControl == SensorStatus::E_STREAMING) {
                    if (cnt == 0) iter->second->isMaster = true;
                    if (cnt >= 2) iter->second->iSensorControl = SensorStatus::E_STANDBY;
                    cnt++;
                }
            }
        }
    }

    auto iter = in.vInfo.find(in.vSensorIdList[0]);
    if(iter == in.vInfo.end())
    {
        MY_LOGE("cannot find in vInfo. sensorId(%d)", in.vSensorIdList[0]);
        return false;
    }
    auto masterInfo = iter->second;
    MRect appCropRegion;
    if(in.mZoomRatio != .0f)
    {
        // if zoom ratio not equal to 0, it needs to convert zoom ratio to cam::0 crop size.
        // used to simulate app crop region.
        appCropRegion.s.w = (float)masterInfo->mActiveArrayRegion.s.w / (float)in.mZoomRatio;
        appCropRegion.s.h = (float)masterInfo->mActiveArrayRegion.s.h / (float)in.mZoomRatio;
        appCropRegion.p.x = ((float)masterInfo->mActiveArrayRegion.s.w - (float)appCropRegion.s.w) * .5f;
        appCropRegion.p.y = ((float)masterInfo->mActiveArrayRegion.s.h - (float)appCropRegion.s.h) * .5f;
    }
    else
    {
        appCropRegion = in.mrCropRegion;
    }
    float defaultFov_H  = masterInfo->fSensorFov_H;
    float defaultFov_V  = masterInfo->fSensorFov_V;
    MY_LOGD("Zoom ratio(%f), appCropRegion(%d, %d), %dx%d",
                out.fZoomRatio, appCropRegion.p.x, appCropRegion.p.y, appCropRegion.s.w, appCropRegion.s.h);

    for(size_t i=0;i<in.vSensorIdList.size();++i)
    {
        auto sensorId = in.vSensorIdList[i];
        auto iter = out.vResult.find(sensorId);
        if(iter == out.vResult.end()){
            SensorStatus iSensorStatus = SensorStatus::E_STREAMING;
            if(i>=2 || inlistSensor>=2) iSensorStatus = SensorStatus::E_STANDBY;
            auto sSensorControlResult = std::make_shared<SensorControlResult>();
            sSensorControlResult->iSensorControl = iSensorStatus;

            auto sensorInfo_iter = in.vInfo.find(sensorId);
            if(sensorInfo_iter == in.vInfo.end()) {
                MY_LOGE("cannot find in vInfo. sensorId(%d)", sensorId);
                continue;
            }
            auto sensorInfo = sensorInfo_iter->second;

            // app view
            if (sensorInfo->fSensorFov_H == defaultFov_H)
            {
                // check wide sensor's active array region
                int targetWidth = appCropRegion.s.w;
                int targetHeight = appCropRegion.s.h;
                auto targetPointX = (sensorInfo->mActiveArrayRegion.s.w - targetWidth)/2 + sensorInfo->mActiveArrayRegion.p.x;
                auto targetPointY = (sensorInfo->mActiveArrayRegion.s.h - targetHeight)/2 + sensorInfo->mActiveArrayRegion.p.y;
                MRect dstRect;
                // Guarantee positive crop for Tg convertion
                // ex., zoomRatio = 0.6x, app crop for wide may be negative
                if (targetPointX < 0 || targetPointY < 0) {
                    dstRect = sensorInfo->mActiveArrayRegion;
                }
                else {
                    dstRect = MRect(MPoint(targetPointX, targetPointY), MSize(targetWidth, targetHeight));
                }

                sSensorControlResult->mrAlternactiveCropRegion = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_Cap = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_3A = dstRect;
            }
            else
            {
                // FOV ratio with radian
                auto degreeRatio_H = degreeRatio(sensorInfo->fSensorFov_H, defaultFov_H);
                auto degreeRatio_V = degreeRatio(sensorInfo->fSensorFov_V, defaultFov_V);

                // defaultH/targetH = 1.3395x
                // out.iZoomRatio / 1.3395x = target zoom ratio
                int targetWidth = int(sensorInfo->mActiveArrayRegion.s.w / ((out.fZoomRatio  * degreeRatio_H)));
                int targetHeight = int(sensorInfo->mActiveArrayRegion.s.h / ((out.fZoomRatio * degreeRatio_V)));
                auto targetPointX = (sensorInfo->mActiveArrayRegion.s.w - targetWidth)/2 + sensorInfo->mActiveArrayRegion.p.x;
                auto targetPointY = (sensorInfo->mActiveArrayRegion.s.h - targetHeight)/2 + sensorInfo->mActiveArrayRegion.p.y;
                MRect dstRect;
                // centro crop, e.g. ori = 100, target = 50, start point =10 ===>target start point= (100-50)/2 + 10 = 35
                if (targetPointX < 0 || targetPointY < 0)
                {
                    dstRect = sensorInfo->mActiveArrayRegion;
                }
                else
                {
                    dstRect = MRect(MPoint(targetPointX, targetPointY), MSize(targetWidth, targetHeight));
                }

                sSensorControlResult->mrAlternactiveCropRegion = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_Cap = dstRect;
                sSensorControlResult->mrAlternactiveCropRegion_3A = dstRect;
                MY_LOGD("cam(%d), Zoom ratio(%f), degreeRatio_H(%f), degreeRatio_V(%f),target crop rect(%d, %d), %dx%d",
                    sensorId, out.fZoomRatio, degreeRatio_H, degreeRatio_V, dstRect.p.x, dstRect.p.y, dstRect.s.w, dstRect.s.h);
            }
            if(i==0 && inlistSensor==0) sSensorControlResult->isMaster = true;
            out.vResult.insert({sensorId, sSensorControlResult});
            inlistSensor++;
        }
    }

    if (!convert3ARegionsForEachSensor(out, in)) {
        return false;
    }

    return true;
}

bool mtk_decision_sensor_control(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
)
{
    if (in.vSensorIdList.size() > 3)
    {
        MY_LOGE("Not support");
        return false;
    }
    if (in.mFeatureMode == FeatureMode::E_Zoom)
    {
        mtk_decision_sensor_control_zoom(out, in);
    }
    else if (in.mFeatureMode == FeatureMode::E_Bokeh)
    {
        mtk_decision_sensor_control_zoom(out, in);
    }
    else if (in.mFeatureMode == FeatureMode::E_Multicam)
    {
        mtk_decision_sensor_control_always_streaming(out, in);
    }
    else
    {
        MY_LOGA("not support mode(%" PRIu32 ")", in.mFeatureMode);
    }
    return true;
}

auto
MtkSensorControl::
createInstance(
    uint64_t timestamp
) -> std::shared_ptr<MtkSensorControl>
{
    return std::make_shared<MtkSensorControl>(timestamp);
}

MtkSensorControl::
MtkSensorControl(
    uint64_t timestamp __attribute__((unused))
)
{
}

auto
MtkSensorControl::
init(SensorControlParamInit param __attribute__((unused))) -> bool
{
    return true;
}

auto
MtkSensorControl::
decideSensorGroup(SensorGroupInfo& info) -> bool
{
    // Only called by PIP-BB,
    // which has resources to set main streaming sensor id
    // Or we use zoom ratio to decide streaming state
    auto getCustomStreamingSensorId = [&info](){
        if (info.pipDeviceCount != 1) return -1;

        // example: use ultra-wide
        int targetId = -1;
        float maxFov = 0;
        for (auto const& item : info.physicalFovMap) {
            if (item.second > maxFov) {
                targetId = item.first;
                maxFov = item.second;
            }
        }
        MY_LOGD("CustomStreamingSensorId(%d), Fov(%f), pipDeviceCount(%d)",
                targetId, maxFov, info.pipDeviceCount);
        return targetId;
    };

    // sensors in same exclusive group can't do streaming simultaneously
    auto getExclusiveGroup = [&](int32_t pipDeviceCount){
        SensorMap<int> exclusiveGroup;
        switch (pipDeviceCount)
        {
            case 0:
                // non-PIP, use default exclusive setting
                for (auto const& item : StereoSettingProvider::getExclusiveGroup(info.openId)) {
                    exclusiveGroup.emplace(item.first, item.second);
                }
                break;
            case 1:
                // for back + back
                // customStreamingSensor(UW) is always streaming
                // so exclusive table: (UW), (W,T)
                for (auto const& id : info.sensorId) {
                    if (id == getCustomStreamingSensorId()) {
                        exclusiveGroup.emplace(id, 0);
                    } else {
                        exclusiveGroup.emplace(id, 1);
                    }
                }
                break;
            case 2:
                // for back + front
                // all sensors are exclusive bacause only 1 sensor can streaming
                // so exclusive table: (W,T,UW)
                for (auto const& id : info.sensorId) {
                    exclusiveGroup.emplace(id, 0);
                }
                break;
            default:
                MY_LOGE("Invalid PipDevicesCount(%d), set to default", pipDeviceCount);
                for (auto const& item : StereoSettingProvider::getExclusiveGroup(info.openId)) {
                    exclusiveGroup.emplace(item.first, item.second);
                }
                break;
        }
        return exclusiveGroup;
    };

    *(info.pGroupMap) = getExclusiveGroup(info.pipDeviceCount);
    return true;
}

auto
MtkSensorControl::
doDecision(
    SensorControlParamOut &out,
    SensorControlParamIn const& in
) -> bool
{
    if (in.vSensorIdList.size() > 3)
    {
        MY_LOGE("Not support");
        return false;
    }
    if (in.mFeatureMode == FeatureMode::E_Zoom)
    {
        mtk_decision_sensor_control_zoom(out, in);
    }
    else if (in.mFeatureMode == FeatureMode::E_Bokeh)
    {
        mtk_decision_sensor_control_zoom(out, in);
    }
    else if (in.mFeatureMode == FeatureMode::E_Multicam)
    {
        mtk_decision_sensor_control_always_streaming(out, in);
    }
    else
    {
        MY_LOGA("not support mode(%" PRIu32 ")", in.mFeatureMode);
    }
    return true;
}

auto
MtkSensorControl::
uninit(
) -> bool
{
    return true;
}

};  //namespace sensorcontrol
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
