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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#define LOG_TAG "mtkcam-RequestSensorControlPolicy"

#include <mtkcam/aaa/IHal3A.h> // get 3a info
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/metadata/IMetadata.h>
//
#include <mtkcam/drv/def/ICam_type.h>
//
#include <mtkcam3/3rdparty/core/sensor_control.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <camera_custom_stereo.h>
//
#include <math.h>
//
#include "RequestSensorControlPolicy_Multicam.h"
#include <mtkcam/utils/hw/HwInfoHelper.h>
//#include "MyUtils.h"

//
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
#endif
//
#define RUNTIME_QUERY_FOV 1

#define MAX_IDLE_DECIDE_FRAME_COUNT 160
#define MAX_STREAMING_FEATURE_CONTROL_FRAME_COUNT 30
//
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCamHW;
using namespace std;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace requestsensorcontrol {
/******************************************************************************
 *
 ******************************************************************************/
Hal3AObject::
Hal3AObject(
    const std::vector<int32_t> &sensorId
)
{
    for(auto&& id:sensorId)
    {
        auto pHal3a =
            std::shared_ptr<NS3Av3::IHal3A>(
                    MAKE_Hal3A(id, LOG_TAG),
                    [&](auto *p)->void
                    {
                        if(p)
                        {
                            p->destroyInstance(LOG_TAG);
                            p = nullptr;
                        }
                    }
        );
        mvHal3A.insert({id, pHal3a});
        mv3AInfo.insert({id, NS3Av3::DualZoomInfo_T()});
    }
}
/******************************************************************************
 *
 ******************************************************************************/
Hal3AObject::
~Hal3AObject()
{
    MY_LOGD("release object");
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Hal3AObject::
update()
{
    bool ret = false;
    NS3Av3::DualZoomInfo_T aaaInfo;
    for(auto&& hal3a:mvHal3A)
    {
        auto const& id = hal3a.first;
        auto const& pHal3A = hal3a.second;
        pHal3A->send3ACtrl(
                    NS3Av3::E3ACtrl_GetDualZoomInfo,
                    (MINTPTR)&aaaInfo,
                    0);
        auto iter = mv3AInfo.find(id);
        if(iter != mv3AInfo.end())
        {
            iter->second = aaaInfo;
            ret = true;
        }
        else
        {
            MY_LOGE("[Should not happened] get 3a info fail. (%d)", id);
            ret = false;
        }
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Hal3AObject::
isAFDone(
    int32_t sensorId
)
{
    auto iter = mv3AInfo.find(sensorId);
    if(iter != mv3AInfo.end())
    {
        return iter->second.bAFDone;
    }
    return false;
}
/******************************************************************************
 *
 ******************************************************************************/
MINT32
Hal3AObject::
getAEIso(
    int32_t sensorId
)
{
    auto iter = mv3AInfo.find(sensorId);
    if(iter != mv3AInfo.end())
    {
        return iter->second.i4AEIso;
    }
    return -1;
}
/******************************************************************************
 *
 ******************************************************************************/
MINT32
Hal3AObject::
getAELv_x10(
    int32_t sensorId
)
{
    auto iter = mv3AInfo.find(sensorId);
    if(iter != mv3AInfo.end())
    {
        return iter->second.i4AELv_x10;
    }
    return -1;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Hal3AObject::
is2ASyncReady(
    int32_t masterSensorId
)
{
    auto iter = mv3AInfo.find(masterSensorId);
    if(iter != mv3AInfo.end())
    {
        return iter->second.bSync2ADone;
    }
    return false;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Hal3AObject::
isAFSyncDone(
    int32_t masterSensorId
)
{
    auto iter = mv3AInfo.find(masterSensorId);
    if(iter != mv3AInfo.end())
    {
        return iter->second.bSyncAFDone;
    }
    return false;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Hal3AObject::
isAaoReady(
    std::vector<uint32_t> const& sensorList
)
{
    MBOOL ret = MTRUE;
    for(auto&& id:sensorList)
    {
        auto iter = mvHal3A.find(id);
        if(iter != mvHal3A.end())
        {
            MBOOL value;
            iter->second->send3ACtrl(
                        NS3Av3::E3ACtrl_GetAAOIsReady,
                        (MINTPTR)&value,
                        0);
            ret &= value;
        }
    }
    if(ret)
        return true;
    else
        return false;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Hal3AObject::
isFramesyncReady(
    int32_t masterSensorId
)
{
    auto iter = mv3AInfo.find(masterSensorId);
    if(iter != mv3AInfo.end())
    {
        if(iter->second.bIsFrameSyncDone)
            return true;
        else
            return false;
    }
    return false;
}
/******************************************************************************
 *
 ******************************************************************************/
bool Hal3AObject::setSync2ASetting(
    int32_t masterSensorId,
    int32_t resumeSensorId)
{
    auto iter = mvHal3A.find(masterSensorId);
    if(iter != mvHal3A.end())
    {
        MY_LOGI("master(%d) slave(%d)", masterSensorId, resumeSensorId);
        iter->second->send3ACtrl(
            NS3Av3::E3ACtrl_Sync3A_Sync2ASetting,
            masterSensorId,
            resumeSensorId);
        return true;
    }
    return false;
}
/******************************************************************************
 *
 ******************************************************************************/
DeviceInfoHelper::
DeviceInfoHelper(
    const int32_t &deviceId
)
{
    auto pSensorList = MAKE_HalSensorList();
    auto pLogicalDevice = MAKE_HalLogicalDeviceList();
    if(pSensorList == nullptr)
    {
        MY_LOGE("create sensor hal list fail.");
        return;
    }
    if(pLogicalDevice == nullptr)
    {
        MY_LOGE("logical device list fail.");
        return;
    }
    auto sensorId = pLogicalDevice->getSensorId(deviceId);
    for(auto&& id:sensorId)
    {
        IMetadata sensorMetadata = MAKE_HalLogicalDeviceList()->queryStaticInfo(id);
        // get sensor static info.
        auto sensorDevId = pSensorList->querySensorDevIdx(id);
        auto sensorStaticInfo = std::make_shared<SensorStaticInfo>();
        if(sensorStaticInfo != nullptr)
        {
            pSensorList->querySensorStaticInfo(sensorDevId, sensorStaticInfo.get());
            MFLOAT focalLength = .0f;
            updatePhysicalFocalLength(id, focalLength, sensorMetadata);
            mvPhysicalFocalLength.insert({id, focalLength});
#if RUNTIME_QUERY_FOV
            auto sensorFov = std::make_shared<MSizeF>();
            updatePhysicalSensorFov(id, focalLength, sensorFov, sensorMetadata);
            mvPhysicalSensorFov.insert({id, sensorFov});
#endif
            mvPhysicalSensorStaticInfo.insert({id, sensorStaticInfo});
        }
        // get active array size from static metadata.
        // (get physic sensor)
        auto activeArray = std::make_shared<MRect>();
        if(!IMetadata::getEntry<MRect>(&sensorMetadata, MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION, *activeArray.get()))
        {
            MY_LOGE("get MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION fail.");
        }
        MY_LOGD("[physical] sensorId(%d) active(%d, %d, %d, %d)",
                                                    id,
                                                    activeArray->p.x,
                                                    activeArray->p.y,
                                                    activeArray->s.w,
                                                    activeArray->s.h);
        mvPhysicalSensorActiveArray.insert({id, activeArray});
    }
    // qeury available focal length
    auto pMetadataProvider = NSMetadataProviderManager::valueForByDeviceId(deviceId);
    if(pMetadataProvider == nullptr)
    {
        MY_LOGA("MetaProvider is nullptr. id(%d)", deviceId);
    }
    auto logicalDeviceMeta = pMetadataProvider->getMtkStaticCharacteristics();
    auto availableFocalLengthEntry = logicalDeviceMeta.entryFor(MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS);
    for(size_t i=0;i<availableFocalLengthEntry.count();i++)
    {
        mvAvailableFocalLength.push_back(availableFocalLengthEntry.itemAt(i, Type2Type<MFLOAT>()));
        MY_LOGD("add available focal length(%f)", mvAvailableFocalLength[i]);
    }
}
/******************************************************************************
 *
 ******************************************************************************/
DeviceInfoHelper::
~DeviceInfoHelper()
{
    MY_LOGD("release device info helper");
}
/******************************************************************************
 *
 ******************************************************************************/
bool
DeviceInfoHelper::
getPhysicalSensorFov(
    int32_t physicalSensorId,
    float &h,
    float &v
)
{
    bool ret = false;
#if RUNTIME_QUERY_FOV
    auto iter = mvPhysicalSensorFov.find(physicalSensorId);
    if(iter != mvPhysicalSensorFov.end())
    {
        h = iter->second->w;
        v = iter->second->h;
        ret = true;
    }
    else
    {
        auto pSensorStaticInfo = mvPhysicalSensorStaticInfo[physicalSensorId];
        if(pSensorStaticInfo != nullptr)
        {
            h = pSensorStaticInfo->horizontalViewAngle;
            v = pSensorStaticInfo->verticalViewAngle;
        }
        else
        {
            MY_LOGE("cannot get fov value from static metadata sensorId (%d)", physicalSensorId);
        }
        ret = false;
    }
#else
    auto pSensorStaticInfo = mvPhysicalSensorStaticInfo[physicalSensorId];
    if(pSensorStaticInfo != nullptr)
    {
        h = pSensorStaticInfo->horizontalViewAngle;
        v = pSensorStaticInfo->verticalViewAngle;
    }
    else
    {
        MY_LOGE("cannot get fov value from static metadata sensorId (%d)", physicalSensorId);
        ret = false;
    }
#endif
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
DeviceInfoHelper::
getPhysicalSensorActiveArray(
    int32_t physicalSensorId,
    MRect &activeArray
)
{
    bool ret = false;
    auto pActiveArray = mvPhysicalSensorActiveArray[physicalSensorId];
    if(pActiveArray != nullptr)
    {
        activeArray = *pActiveArray.get();
        ret = true;
    }
    else
    {
        MY_LOGE("cannot get physical active array. sensor(%d)", physicalSensorId);
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
DeviceInfoHelper::
getPhysicalSensorFullSize(
    int32_t physicalSensorId,
    MSize &sensorSize
)
{
    bool ret = false;
    auto pActiveArray = mvPhysicalSensorActiveArray[physicalSensorId];
    if(pActiveArray != nullptr)
    {
        sensorSize = MSize(
                            pActiveArray->s.w,
                            pActiveArray->s.h
                    );
        ret = true;
    }
    else
    {
        MY_LOGE("cannot get physical active array. sensor(%d)", physicalSensorId);
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
DeviceInfoHelper::
getPhysicalFocalLength(
    int32_t physicalSensorId,
    MFLOAT &focalLength
)
{
    bool ret = false;
    auto iter = mvPhysicalFocalLength.find(physicalSensorId);
    if(iter != mvPhysicalFocalLength.end())
    {
        focalLength = iter->second;
    }
    else
    {
        MY_LOGE("cannot get physical focal length. sensor(%d)", physicalSensorId);
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
DeviceInfoHelper::
getLogicalFocalLength(
    std::vector<MFLOAT>& focalLength
)
{
    focalLength = mvAvailableFocalLength;
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
DeviceInfoHelper::
updatePhysicalFocalLength(
    int32_t sensorId __attribute__((unused)),
    MFLOAT &focalLength,
    IMetadata &sensorMetadata
) -> bool
{
    if(!IMetadata::getEntry<MFLOAT>(&sensorMetadata, MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, focalLength))
    {
        MY_LOGE("get MTK_LENS_INFO_AVAILABLE_FOCAL_LENGTHS fail.");
        return false;
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
DeviceInfoHelper::
updatePhysicalSensorFov(
    int32_t sensorId,
    MFLOAT &focalLength,
    std::shared_ptr<MSizeF>& pSensorFov,
    IMetadata &sensorMetadata
) -> bool
{
#define PI 3.14159265
    float fPhysicalSize_h = .0f;
    float fPhysicalSize_v = .0f;
    if(!IMetadata::getEntry<MFLOAT>(&sensorMetadata, MTK_SENSOR_INFO_PHYSICAL_SIZE, fPhysicalSize_h, 0))
    {
        MY_LOGE("get MTK_SENSOR_INFO_PHYSICAL_SIZE (h) fail.");
    }
    if(!IMetadata::getEntry<MFLOAT>(&sensorMetadata, MTK_SENSOR_INFO_PHYSICAL_SIZE, fPhysicalSize_v, 1))
    {
        MY_LOGE("get MTK_SENSOR_INFO_PHYSICAL_SIZE (v) fail.");
    }
    // compute fov
    double result_h = (2.0) * atan(fPhysicalSize_h/((2.0) * focalLength)) * 180 / PI;
    double result_v = (2.0) * atan(fPhysicalSize_v/((2.0) * focalLength)) * 180 / PI;
    pSensorFov->w = (MFLOAT)result_h;
    pSensorFov->h = (MFLOAT)result_v;
    MY_LOGD("sensorId(%d) focal_length(%f) physical_size(%f x %f) fov_h(%f) fov_v(%f)",
            sensorId, focalLength, fPhysicalSize_h, fPhysicalSize_v,
            pSensorFov->w, pSensorFov->h);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
ISPQualityManager::
ISPQualityManager(
    int32_t openId,
    uint32_t loglevel
) : mOpenId(openId),
    mLogLevel(loglevel)
{
    (void)openId;
    (void)mLogLevel;
}
/******************************************************************************
 *
 ******************************************************************************/
ISPQualityManager::
~ISPQualityManager()
{
}
/******************************************************************************
 *
 ******************************************************************************/
bool
ISPQualityManager::
init(
    SensorMap<P1HwSetting> const* pvP1HwSetting
)
{
    auto pLogicalDevice = MAKE_HalLogicalDeviceList();
    if(pLogicalDevice == nullptr)
    {
        MY_LOGE("logical device list fail.");
        return false;
    }
    auto sensorId = pLogicalDevice->getSensorId(mOpenId);
    int forceDSwitchControl = ::property_get_int32("vendor.debug.rear_ducam.off_twin", -1);
    if(forceDSwitchControl == 1 || forceDSwitchControl == 0) {
        if(forceDSwitchControl == 1) {
            MY_LOGI("disable D-Switch");
            mSupportIspQualitySwitch = false;
        }
        else {
            MY_LOGI("force enable d-switch");
            mSupportIspQualitySwitch = true;
        }
    }
    else {
        if(pvP1HwSetting != nullptr)
        {
            for (auto&& [sensorId, _setting] : *pvP1HwSetting)
            {
                if(_setting.ispQuality == eCamIQ_L)
                {
                    mSupportIspQualitySwitch = true;
                    break;
                }
            }
        }
    }
    if(sensorId.size() >= 2) {
        mPrvQualityFlag.quality = mBaseQualityFlag.quality = buildQualityFlag(
                                                            sensorId[0],
                                                            sensorId[1],
                                                            mSupportIspQualitySwitch);
    }
    MY_LOGI("supportSwitch(%d) mBaseQualityFlag(%d)",
                                mSupportIspQualitySwitch,
                                mBaseQualityFlag.quality);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
ISPQualityManager::
isSameToBaseQualityFlag()
{
    MY_LOGI("prv(%d) base(%d)", mPrvQualityFlag.quality, mBaseQualityFlag.quality);
    return (mPrvQualityFlag.quality == mBaseQualityFlag.quality);
}
/******************************************************************************
 *
 ******************************************************************************/
MUINT32
ISPQualityManager::
getQualityFlag(
    uint32_t const& requestNo,
    int32_t const&master,
    int32_t const&slave
)
{
    QualityFlag qualityFlag;
    if(mSupportIspQualitySwitch)
    {
        qualityFlag.reqNo = requestNo;
        qualityFlag.quality = buildQualityFlag(master, slave, mSupportIspQualitySwitch);
        if(qualityFlag.quality != mPrvQualityFlag.quality)
        {
            // quality switch occuring.
            std::lock_guard<std::mutex> lock(mLock);
            MY_LOGD("quality switch");
            bQualitySwitching = true;
        }
        mPrvQualityFlag = qualityFlag;
    }
    else
    {
        qualityFlag.quality = mBaseQualityFlag.quality;
    }
    return qualityFlag.quality;
}
/******************************************************************************
 *
 ******************************************************************************/
MUINT32
ISPQualityManager::
getBaseQualityFlag()
{
    return mBaseQualityFlag.quality;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
ISPQualityManager::
updateBaseQualityFlag(
    uint32_t const& requestNo,
    int32_t const& master,
    int32_t const& slave
)
{
    if(!mSupportIspQualitySwitch) return true;
    mBaseQualityFlag.reqNo = requestNo;
    mBaseQualityFlag.quality = buildQualityFlag(master, slave, mSupportIspQualitySwitch);
    MY_LOGI("[%" PRIu32 "] update base(%d) quality flag m(%d) s(%d)",
                                            requestNo,
                                            mBaseQualityFlag.quality,
                                            master,
                                            slave);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
ISPQualityManager::
revertQualityToBase()
{
    if(!mSupportIspQualitySwitch) return true;
    // quality switch occuring.
    std::lock_guard<std::mutex> lock(mLock);
    MY_LOGD("quality switch");
    bQualitySwitching = true;
    mPrvQualityFlag.quality = mBaseQualityFlag.quality;
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
ISPQualityManager::
qualitySwitchDone()
{
    // quality switch occuring.
    std::lock_guard<std::mutex> lock(mLock);
    bQualitySwitching = false;
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
ISPQualityManager::
isQualitySwitchDone()
{
    // quality switch occuring.
    std::lock_guard<std::mutex> lock(mLock);
    return !bQualitySwitching;
}
/******************************************************************************
 *
 ******************************************************************************/
MUINT32
ISPQualityManager::
buildQualityFlag(
    int32_t master,
    int32_t slave,
    bool supportQualitySwitch
)
{
    CAM_QUALITY config;
    if(supportQualitySwitch) {
        // set min sensor id to index 0.
        if(master < slave) {
            config.Bits.Qlvl_0 = E_CamIQLevel::eCamIQ_H;
            config.Bits.SensorIdx_0 = master;
            config.Bits.Qlvl_1 = E_CamIQLevel::eCamIQ_L;
            config.Bits.SensorIdx_1 = slave;
        }
        else {
            config.Bits.Qlvl_0 = E_CamIQLevel::eCamIQ_L;
            config.Bits.SensorIdx_0 = slave;
            config.Bits.Qlvl_1 = E_CamIQLevel::eCamIQ_H;
            config.Bits.SensorIdx_1 = master;
        }
    }
    else {
        config.Bits.Qlvl_0 = E_CamIQLevel::eCamIQ_H;
        config.Bits.SensorIdx_0 = master;
        config.Bits.Qlvl_1 = E_CamIQLevel::eCamIQ_H;
        config.Bits.SensorIdx_1 = slave;
    }
    MY_LOGD("[%d] ql_0(%d:%d) ql_1(%d:%d) value(%d)",
                supportQualitySwitch,
                config.Bits.SensorIdx_0, config.Bits.Qlvl_0,
                config.Bits.SensorIdx_1, config.Bits.Qlvl_1,
                config.Raw);
    return config.Raw;
}
/******************************************************************************
 *
 ******************************************************************************/
SensorStateManager::
SensorStateManager(
    int32_t openId,
    const std::vector<int32_t> &vSensorId,
    uint32_t maxOnlineCount,
    int32_t  initRequest,
    uint32_t loglevel
) : mLogLevel(loglevel),
    mAppInitReqCnt(initRequest),
    mMaxOnlineSensorCount(maxOnlineCount)
{
    for(size_t i=0;i<vSensorId.size();i++)
    {
        auto sensorState = SensorStateType::E_ONLINE;
        // set main0 streaming as default
        auto sensorOnlineStatue = (i == 0) ? sensorcontrol::SensorStatus::E_STREAMING : sensorcontrol::SensorStatus::E_STANDBY;

        if(i > (maxOnlineCount - 1))
        {
            sensorState = SensorStateType::E_OFFLINE;
            sensorOnlineStatue = sensorcontrol::SensorStatus::E_NONE;
        }
        SensorState s = {
            .eSensorState = sensorState,
            .eSensorOnlineStatus = sensorOnlineStatue,
            .iIdleCount = 0
        };
        mvSensorStateList.insert({vSensorId[i], std::move(s)});
    }
    // sensor group manager
    mpSensorGroupManager =
            std::unique_ptr<SensorGroupManager,
                            std::function<void(SensorGroupManager*)> >(
                                new SensorGroupManager(),
                                [](SensorGroupManager *p)
                                {
                                    if(p)
                                    {
                                        delete p;
                                    }
                                });
    // isp quality manager
    mpIspQualityManager =
            std::unique_ptr<ISPQualityManager,
                            std::function<void(ISPQualityManager*)> >(
                                new ISPQualityManager(
                                    openId,
                                    loglevel),
                                [](ISPQualityManager *p)
                                {
                                    if(p)
                                    {
                                        delete p;
                                    }
                                });
    MY_LOGD("loglevel(%" PRIu32 ") maxOnlineCount(%u)", loglevel, maxOnlineCount);
}
/******************************************************************************
 *
 ******************************************************************************/
SensorStateManager::
~SensorStateManager()
{
    MY_LOGD("release obj");
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
setExclusiveGroup(
    SensorMap<int> const* pGroupMap
)
{
    return mpSensorGroupManager->setGroupMap(pGroupMap);
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
update(
    sensorcontrol::SensorControlParamOut const& out,
    ParsedSensorStateResult &result,
    Input const& inputParams
)
{
    if(mpIspQualityManager == nullptr)
    {
        MY_LOGA("mpIspQualityManager is nullptr.");
        return false;
    }

    // copy alternactive crop region
    auto copyCropRegion = [&]() {
        for (auto&& item:out.vResult)
        {
            if (item.second != nullptr)
            {
                result.vAlternactiveCropRegion.insert(
                            {item.first,
                            item.second->mrAlternactiveCropRegion});
                result.vAlternactiveAfRegion.insert(
                            {item.first,
                            item.second->mrAlternactiveAfRegion});
                result.vAlternactiveAeRegion.insert(
                            {item.first,
                            item.second->mrAlternactiveAeRegion});
                result.vAlternactiveAwbRegion.insert(
                            {item.first,
                            item.second->mrAlternactiveAwbRegion});
                result.vAlternactiveCropRegion_Cap.insert(
                            {item.first,
                            item.second->mrAlternactiveCropRegion_Cap});
                result.vAlternactiveCropRegion_3A.insert(
                            {item.first,
                            item.second->mrAlternactiveCropRegion_3A});
            }
        }
    };
    copyCropRegion();

    if(mbInit)
    {
        mpIspQualityManager->init(inputParams.pvP1HwSetting);
    }

    if (getInitReqCnt() > 0)
    {
        setInitReqCnt(getInitReqCnt()-1);
    }

    //get previous masterid
    auto previousMasterId = mLastUpdateData.masterId;

    //remove isSensorSwitchGoing() check for quick switch

    if (inputParams.bFirstReqCapture) {
        MY_LOGD("First request is capture !");
        std::unordered_map<uint32_t, sensorcontrol::SensorStatus>
                            vResidueOnlineStateList;
        decideSensorOnOffState(out, result, vResidueOnlineStateList);
        // check quality flag is same to base.
        if(mpIspQualityManager->isSameToBaseQualityFlag())
        {
            updateSensorOnOffState(result);
            // update sensor state (streaming, standby, resume) to online list.
            if(!updateOnlineSensorResult(result, vResidueOnlineStateList))
            {
                MY_LOGA("updateOnlineSensorResult fail");
                goto lbExit;
            }
            // update base quality flag
            if(result.vStreamingSensorIdList.size() == 0)
            {
                MY_LOGD("switch behavior not defined, ignore");
                mLastUpdateData = result;
                goto lbExit;
            }
            int32_t masterId = result.vStreamingSensorIdList[0];
            if (result.vNeedOnlineSensorList.size()) {
                int32_t slaveId = result.vNeedOnlineSensorList[0];
                mpIspQualityManager->updateBaseQualityFlag(
                                                    inputParams.requestNo,
                                                    masterId,
                                                    slaveId);
                // in this case, isp quality should be base isp quality
                result.qualityFlag = mpIspQualityManager->getBaseQualityFlag();
                mLastUpdateData = result;
                MY_LOGD("start sensor switch base(%d)", result.qualityFlag);
            } else {
                MY_LOGD("skip switch");
            }
        }
        else
        {
            // using cache data to switch quality to base quality.
            result = mLastUpdateData;
            // in this case, isp quality should be base isp quality
            result.qualityFlag = mpIspQualityManager->getBaseQualityFlag();
            mpIspQualityManager->revertQualityToBase();
            MY_LOGD("[trigger] switch to base quality. (%d)", result.qualityFlag);
            goto lbExit;
        }
        mbInit = false;
    }
    else if((inputParams.isCaptureing || isSensorSwitchGoing() || !isSensorOKforSwitch ()))
    {
        MY_LOGD("using previous data. capture(%d) switching(%d) !OKforSwitch(%d)"
            , inputParams.isCaptureing, isSensorSwitchGoing(), !isSensorOKforSwitch ());
        // result sensor count first.
        resetIdleCountToAllSensors();
        // clone last updated result.
        mLastUpdateData.vResumeSensorIdList.clear();
        mLastUpdateData.vNeedOnlineSensorList.clear();
        mLastUpdateData.vNeedOfflineSensorList.clear();
        result = mLastUpdateData;

        if (!inputParams.isCaptureing) {
            // Update crop if not capture, ex., af searching 2x with wide
            copyCropRegion();
        }

        // no matter what always remove already standby sensor list.
        // (even zsl flow is enable, it may send to pending queue. but, sensor already
        // suspend...)
        for(auto iter=result.vStreamingSensorIdList.begin();
            iter != result.vStreamingSensorIdList.end();)
        {
            if(getSensorOnlineState(*iter) == sensorcontrol::SensorStatus::E_STANDBY)
            {
                MY_LOGD("sensor(%" PRIu32 ") is standby, remove it", *iter);
                iter = result.vStreamingSensorIdList.erase(iter);
                result.vStandbySensorIdList.assign(result.vGotoStandbySensorIdList.begin(),result.vGotoStandbySensorIdList.end());
                result.vGotoStandbySensorIdList.clear();
            }
            else
            {
                iter++;
            }
        }
    }
    else
    {
        std::unordered_map<uint32_t, sensorcontrol::SensorStatus>
                            vResidueOnlineStateList;
        decideSensorOnOffState(out, result, vResidueOnlineStateList);
        bool isNeedSensorSwitch =
                            ((result.vNeedOfflineSensorList.size() > 0) ||
                            (result.vNeedOnlineSensorList.size() > 0)) &&
                            isSensorOKforSwitch();
        if(!isNeedSensorSwitch)
        {
            // update sensor state (streaming, standby, resume) to online list.
            if(!updateOnlineSensorResult(result, vResidueOnlineStateList))
            {
                MY_LOGA("updateOnlineSensorResult fail");
                goto lbExit;
            }
            // update quality flag.
            getQualityFlag(result, inputParams, result.qualityFlag);
            mLastUpdateData = result;
        }
        else if(isNeedSensorSwitch &&
                mpIspQualityManager->isQualitySwitchDone())
        {
            // check quality flag is same to base.
            if(mpIspQualityManager->isSameToBaseQualityFlag())
            {
                updateSensorOnOffState(result);
                // update sensor state (streaming, standby, resume) to online list.
                if(!updateOnlineSensorResult(result, vResidueOnlineStateList))
                {
                    MY_LOGA("updateOnlineSensorResult fail");
                    goto lbExit;
                }
                // update base quality flag
                if(result.vStreamingSensorIdList.size() == 0)
                {
                    MY_LOGD("switch behavior not defined, ignore");
                    mLastUpdateData = result;
                    goto lbExit;
                }
                int32_t masterId = result.vStreamingSensorIdList[0];
                int32_t slaveId = result.vNeedOnlineSensorList[0];
                mpIspQualityManager->updateBaseQualityFlag(
                                                    inputParams.requestNo,
                                                    masterId,
                                                    slaveId);
                // in this case, isp quality should be base isp quality
                result.qualityFlag = mpIspQualityManager->getBaseQualityFlag();
                mLastUpdateData = result;
                MY_LOGD("start sensor switch base(%d)", result.qualityFlag);
            }
            else
            {
                // using cache data to switch quality to base quality.
                result = mLastUpdateData;
                // in this case, isp quality should be base isp quality
                result.qualityFlag = mpIspQualityManager->getBaseQualityFlag();
                mpIspQualityManager->revertQualityToBase();
                MY_LOGD("[trigger] switch to base quality. (%d)", result.qualityFlag);
                goto lbExit;
            }
        }
        else if(isNeedSensorSwitch &&
                !mpIspQualityManager->isQualitySwitchDone())
        {
            result = mLastUpdateData;
            result.qualityFlag = mpIspQualityManager->getBaseQualityFlag();
            MY_LOGD("switch to base quality. (%d)", result.qualityFlag);
        }
        else
        {
            result = mLastUpdateData;
            goto lbExit;
        }
        mbInit = false;
    }
    // Remove handleStreamingListForSwitchingFlow. In sensor switching case,
    // we still let StreamingSensorIdList include switching sensors for quick init request.
    // the main concept is that queue reqeust when p1Node not config done. (not config done = do switching)

    // handle sensor still switching, remove it from streaming list.
    // handleStreamingListForSwitchingFlow(result);
lbExit:
    result.previousMasterId = previousMasterId;
    dump(result);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
switchSensorDone()
{
    setSwitchGoingFlag(false);
    auto cur_Time = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = cur_Time - pre_time;
    MY_LOGI("switch done. using time: %lf\n", diff.count());
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
switchQualityDone()
{
    if(mpIspQualityManager != nullptr)
    {
        mpIspQualityManager->qualitySwitchDone();
    }
}
/******************************************************************************
 *
 ******************************************************************************/
SensorStateManager::SensorStateType
SensorStateManager::
getSensorState(
    uint32_t sensorId
) const
{
    SensorStateManager::SensorStateType type = E_INVALID;
    auto iter = mvSensorStateList.find(sensorId);
    if(iter != mvSensorStateList.end())
    {
        type = iter->second.eSensorState;
    }
    else
    {
        MY_LOGE("[%d] cannot get sensor state in mvSensorStateList.", sensorId);
    }
    return type;
}
/******************************************************************************
 *
 ******************************************************************************/
sensorcontrol::SensorStatus
SensorStateManager::
getSensorOnlineState(
    uint32_t sensorId
) const
{
    sensorcontrol::SensorStatus type = sensorcontrol::E_NONE;
    auto iter = mvSensorStateList.find(sensorId);
    if(iter != mvSensorStateList.end())
    {
        type = iter->second.eSensorOnlineStatus;
    }
    else
    {
        MY_LOGE("[%d] cannot get sensor online state in mvSensorStateList.", sensorId);
    }
    return type;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
isSensorSwitchGoing()
{
    std::lock_guard<std::mutex> lock(mLock);
    return mbSensorSwitchGoing;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
isSensorOKforSwitch()
{
    std::lock_guard<std::mutex> lock(mLock);

    if (mbConfigDone && mbAfDone && mbNextCapture && !mbPrecapTrigger)
    {
        mbSensorOKforSwitch = true;
    }
    else
    {
        mbSensorOKforSwitch = false;
        MY_LOGD("isSensorOKforSwitch: false, SensorConfigDone (%d) AfDone (%d) NextCapture (%d), mbPrecapTrigger (%d)",
            mbConfigDone, mbAfDone, mbNextCapture, mbPrecapTrigger);
    }

    return mbSensorOKforSwitch;
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setInitParsedSensorStateResult(uint32_t sensorId)
{
    auto it = std::find(mLastUpdateData.vStreamingSensorIdList.begin(),
        mLastUpdateData.vStreamingSensorIdList.end(),
        sensorId);
    if (it == mLastUpdateData.vStreamingSensorIdList.end())
    {
        mLastUpdateData.vStreamingSensorIdList.push_back(sensorId);
        MY_LOGD("init streaming sensor(%d)", sensorId);
    }
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setInitMasterId(int32_t sensorId)
{
    if (sensorId == -1)
    {
        mLastUpdateData.masterId = mLastUpdateData.vStreamingSensorIdList[0];
    }
    else
    {
        mLastUpdateData.masterId = sensorId;
    }
    MY_LOGD("init master sensor(%d)", mLastUpdateData.masterId);
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setSensorConfigureDone(bool flag)
{
    mbConfigDone = flag;
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setNextCaptureDone(bool flag)
{
    mbNextCapture = flag;
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setAfDone(bool flag)
{
    mbAfDone = flag;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
isInitRequestGoing()
{
    std::lock_guard<std::mutex> lock(mLock);
    MY_LOGD("isInitRequestGoing mInitReqCnt(%d), mbSensorSwitchGoing(%d), isInitRequestGoing(%d)", mInitReqCnt, mbSensorSwitchGoing,
                (mInitReqCnt != 0 || mbSensorSwitchGoing));
    return (mInitReqCnt != 0 || mbSensorSwitchGoing);
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setInitReqCnt(int initReqCnt)
{
    if (initReqCnt < 0)
        return;
    std::lock_guard<std::mutex> lock(mLock);
    mInitReqCnt = initReqCnt;
}
/******************************************************************************
 *
 ******************************************************************************/
int
SensorStateManager::
getInitReqCnt()
{
    std::lock_guard<std::mutex> lock(mLock);
    return mInitReqCnt;
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
checkAfDoneBeforeSwitch(const IMetadata* metadata, uint32_t requestNo)
{
    std::lock_guard<std::mutex> lock(mLock);
    MUINT8 curAfMode = -1;
    MUINT8 isAfTrigger = -1;
    bool isAfRegion = true;
    if (!IMetadata::getEntry<MUINT8>(metadata,
                                     MTK_CONTROL_AF_MODE,
                                     curAfMode)) {
        MY_LOGD("No MTK_CONTROL_AF_MODE");
    }
    if (!IMetadata::getEntry<MUINT8>(metadata,
                                     MTK_CONTROL_AF_TRIGGER,
                                     isAfTrigger)) {
        MY_LOGD("No MTK_CONTROL_AF_TRIGGER");
    }
    if (metadata->entryFor(MTK_CONTROL_AF_REGIONS).isEmpty()){
        isAfRegion = false;
        MY_LOGD("No MTK_CONTROL_AF_REGIONS");
    }
    MY_LOGD_IF(mLogLevel >= 2,
        "Req(%d), AF_MODE(%d), AF_TRIGGER(%d)", requestNo, curAfMode, isAfTrigger);

    // Touch AF trigger
    if (MTK_CONTROL_AF_MODE_AUTO == curAfMode &&
        MTK_CONTROL_AF_TRIGGER_START == isAfTrigger && isAfRegion) {
        MY_LOGD("Touch AF is triggered.");
        mTouchAfReqList.push_back(requestNo);
    }

    // If AF is canceled by AP, enable switching after cancel is done by AF
    if (MTK_CONTROL_AF_TRIGGER_CANCEL == isAfTrigger) {
        MY_LOGD("AF_TRIGGER is CANCEL");
        mbAfCancelTrigger = true;
    }

    auto dumpTouchAfReqList = [&](int loglevel){
        if (loglevel >= 2) {
            std::string dump_str = "dumpTouchAfReqList:";
            for (auto const& reqno : mTouchAfReqList) {
                dump_str = dump_str + " " + std::to_string(reqno);
            }
            MY_LOGD("%s", dump_str.c_str());
        }
    };

    if (mTouchAfReqList.empty()) {
        setAfDone(true);
    } else {
        setAfDone(false);
        dumpTouchAfReqList(mLogLevel);
    }
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
updateAfStateDone(uint32_t requestNo, uint32_t afstate)
{
    std::lock_guard<std::mutex> lock(mLock);
    int32_t AfSearchState = -1;

    // AF is already canceled, unblock sensor switch until 3a cancel is done
    if (mbAfCancelTrigger) {
        if (afstate != MTK_CONTROL_AF_STATE_ACTIVE_SCAN ||
            afstate != MTK_CONTROL_AF_STATE_PASSIVE_SCAN) {
            AfSearchState = AfSearchState::kCanceled;
        }
        if (AfSearchState == AfSearchState::kCanceled) {
            MY_LOGD("Req(%d) AF_STATE(%d): Cancel AF done, unblock switching.", requestNo, afstate);
            mTouchAfReqList.clear();
            mbAfCancelTrigger = false;
        }
    }

    // Touch AF blocks sensor switch
    if (!mTouchAfReqList.empty()) {
        if (afstate == MTK_CONTROL_AF_STATE_INACTIVE ||
            afstate == MTK_CONTROL_AF_STATE_FOCUSED_LOCKED ||
            afstate == MTK_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
            AfSearchState = AfSearchState::kTouchAfNotSearching;
        }
        else {
            AfSearchState = AfSearchState::kTouchAfSearching;
        }

        if (AfSearchState == AfSearchState::kTouchAfNotSearching) {
            MY_LOGD_IF(mLogLevel >=2,
                "Req(%d) AF_STATE(%d): AF locked/inactive, unblock switching.", requestNo, afstate);
            mTouchAfReqList.erase(std::remove_if(
                    mTouchAfReqList.begin(), mTouchAfReqList.end(),
                    [&](uint32_t reqno){return reqno <= requestNo;}),
                mTouchAfReqList.end());
        }
        else {
            MY_LOGD_IF(mLogLevel >=2,
                "Req(%d) AF_STATE(%d): AF isn't locked yet, block switching.", requestNo, afstate);
        }
    }
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
checkPrecaptureTrigger(const IMetadata* metadata, uint32_t requestNo)
{
    std::lock_guard<std::mutex> lock(mLock);
    MUINT8 precapStatus = -1;
    if (!IMetadata::getEntry<MUINT8>(metadata,
                                     MTK_CONTROL_AE_PRECAPTURE_TRIGGER,
                                     precapStatus)) {
        MY_LOGD("No MTK_CONTROL_AE_PRECAPTURE_TRIGGER");
        return;
    }
    if (precapStatus == MTK_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
        MY_LOGD("Req(%d), Precapture start (%d)", requestNo, precapStatus);
        mbPrecapTrigger = true;
    }
    if (precapStatus == MTK_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL) {
        MY_LOGD("Req(%d), Precapture cancel (%d)", requestNo, precapStatus);
        mbPrecapTrigger = false;
    }
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
resetPrecaptureTrigger()
{
    std::lock_guard<std::mutex> lock(mLock);
    MY_LOGD("Capture is done, reset precapture trigger");
    mbPrecapTrigger = false;
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setSwitchGoingFlag(
    bool flag
)
{
    std::lock_guard<std::mutex> lock(mLock);
    MY_LOGD("as-is(%d) to-be(%d)", mbSensorSwitchGoing, flag);
    mbSensorSwitchGoing = flag;
    // if switch done, clear list.
    if(!flag)
        mvSwitchingIdList.clear();
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setSensorState(
    uint32_t const& id,
    SensorStateType type
)
{
    auto iter = mvSensorStateList.find(id);
    if(iter != mvSensorStateList.end())
    {
        if(type == SensorStateType::E_ONLINE)
        {
            iter->second.eSensorState = SensorStateType::E_ONLINE;
            iter->second.eSensorOnlineStatus =
                        sensorcontrol::SensorStatus::E_STREAMING;
            iter->second.iIdleCount = 0;
        }
        else if(type == SensorStateType::E_OFFLINE)
        {
            iter->second.eSensorState = SensorStateType::E_OFFLINE;
            iter->second.eSensorOnlineStatus =
                        sensorcontrol::SensorStatus::E_NONE;
            iter->second.iIdleCount = 0;
        }
    }
}
/******************************************************************************
 *
 ******************************************************************************/
void
SensorStateManager::
setSensorOnlineState(
    uint32_t const& id,
    sensorcontrol::SensorStatus status
)
{
    auto iter = mvSensorStateList.find(id);
    if(iter != mvSensorStateList.end())
    {
        if(status == iter->second.eSensorOnlineStatus)
            return;

        if(status == sensorcontrol::SensorStatus::E_STREAMING)
        {
            iter->second.eSensorState = SensorStateType::E_ONLINE;
            iter->second.eSensorOnlineStatus = sensorcontrol::SensorStatus::E_STREAMING;
        }
        else if(status == sensorcontrol::SensorStatus::E_STANDBY)
        {
            iter->second.eSensorState = SensorStateType::E_ONLINE;
            iter->second.eSensorOnlineStatus = sensorcontrol::SensorStatus::E_STANDBY;
        }
        else if(status == sensorcontrol::SensorStatus::E_NONE)
        {
            iter->second.eSensorState = SensorStateType::E_OFFLINE;
            iter->second.eSensorOnlineStatus = sensorcontrol::SensorStatus::E_NONE;
        }

        iter->second.iIdleCount = 0;
    }
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
decideSensorOnOffState(
    sensorcontrol::SensorControlParamOut const& out,
    ParsedSensorStateResult &parsedSensorStateResult,
    std::unordered_map<uint32_t, sensorcontrol::SensorStatus> &vResidueOnlineStateList
)
{
    uint32_t streamingCount = 0;
    std::vector<uint32_t> vCurrent_Online_StandbyState;
    std::vector<uint32_t> vDecision_StreamingState;
    std::vector<uint32_t> vOnlineList;
    for(auto&& item:out.vResult)
    {
        if(item.second != nullptr)
        {
            if(item.second->isMaster)
            {
                parsedSensorStateResult.masterId = item.first;
            }
            // check streaming count whether greate than max online count.
            if(item.second->iSensorControl == sensorcontrol::SensorStatus::E_STREAMING)
            {
                vDecision_StreamingState.push_back(item.first);
                if(++streamingCount > mMaxOnlineSensorCount)
                {
                    MY_LOGE("streaming count (%u) > max online sensor count (%u)",
                                streamingCount,
                                mMaxOnlineSensorCount);
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
                    aee_system_exception(
                        LOG_TAG,
                        NULL,
                        DB_OPT_DEFAULT,
                        "streaming count check fail.\nCRDISPATCH_KEY:streaming count check fail.");
#endif
                    return false;
                }
                // check need online
                {
                    auto sensorState = getSensorState(item.first);
                    if(sensorState == E_INVALID)
                    {
                        return false;
                    }
                    // remove this flow for quick switching
                    else if(sensorState == E_OFFLINE)
                    {
                        // current decision need streaming, but sensor state is offline.
                        // push sensor id to need online sensor list.
                        parsedSensorStateResult.vNeedOnlineSensorList.push_back(item.first);
                        vOnlineList.push_back(item.first);
                        // Store decision to vResidueOnlineStateList.
                        vResidueOnlineStateList.insert({
                                                    item.first,
                                                    item.second->iSensorControl});
                    }
                    else // E_ONLINE
                    {
                        vOnlineList.push_back(item.first);
                        // store current sensor state is online, but online state is E_STANDBY.
                        // For, checking needs offline.
                        auto onlineState = getSensorOnlineState(item.first);
                        if(onlineState == sensorcontrol::SensorStatus::E_STANDBY)
                        {
                            vCurrent_Online_StandbyState.push_back(item.first);
                        }
                        // Store decision to vResidueOnlineStateList.
                        vResidueOnlineStateList.insert({
                                                    item.first,
                                                    item.second->iSensorControl});
                    }
                }
            }
            else if(item.second->iSensorControl == sensorcontrol::SensorStatus::E_STANDBY)
            {
                // get sensor state, and store online in online list.
                auto sensorState = getSensorState(item.first);
                if(sensorState == E_ONLINE)
                {
                    vOnlineList.push_back(item.first);
                    // store current sensor state is online, but online state is E_STANDBY.
                    // For checking needs offline.
                    auto onlineState = getSensorOnlineState(item.first);
                    if(onlineState == sensorcontrol::SensorStatus::E_STANDBY)
                    {
                        vCurrent_Online_StandbyState.push_back(item.first);
                    }
                    // Store decision to vResidueOnlineStateList.
                    vResidueOnlineStateList.insert({
                                                item.first,
                                                item.second->iSensorControl});
                }
            }
        }
        else
        {
            MY_LOGE("[%d] decision result is nullptr", item.first);
            return false;
        }
    }
    // after check decision result. It has to check whether some sensor need offline.
    if(vOnlineList.size() > mMaxOnlineSensorCount)
    {
        auto remove_item_from_online_list = [&vOnlineList](std::vector<uint32_t> &list)
        {
            for(auto&& id:list)
            {
                auto iter = std::find(vOnlineList.begin(), vOnlineList.end(), id);
                if(iter != vOnlineList.end())
                {
                    vOnlineList.erase(iter);
                }
            }
            return;
        };
        auto is_same_sensor_group = [this](uint32_t &val1, uint32_t &val2)
        {
            bool result = true;
            auto ret = mpSensorGroupManager->isSameGroup(val1, val2);
            result &= ret;
            MY_LOGD("check i(%" PRIu32 ") t(%" PRIu32 ") %d", val1, val2, ret);
            return result;
        };
        auto remove_item_from_residue_online_list = [&vResidueOnlineStateList](uint32_t id)
        {
            auto iter = vResidueOnlineStateList.find(id);
            if(iter != vResidueOnlineStateList.end())
            {
                vResidueOnlineStateList.erase(iter);
            }
            return;
        };
        unsigned int remove_size = vOnlineList.size() - mMaxOnlineSensorCount;
        // 1. remove decision wants streaming sensor id.
        remove_item_from_online_list(vDecision_StreamingState);
        // 2. select need offline from online and standby mode.
        std::vector<uint32_t> removeLise_OnlineList;
        if(vCurrent_Online_StandbyState.size() > 0 && remove_size > 0)
        {
            for(auto& neededOnlineId:parsedSensorStateResult.vNeedOnlineSensorList)
            {
                for(auto& id:vCurrent_Online_StandbyState)
                {
                    if(is_same_sensor_group(neededOnlineId, id))
                    {
                        MY_LOGD("online [%" PRIu32 "] is same group with [%" PRIu32 "](offline)",
                                                                neededOnlineId, id);
                        remove_item_from_residue_online_list(id);
                        parsedSensorStateResult.vNeedOfflineSensorList.push_back(id);
                        // if this pair already be removed, it has to add to removeLise_OnlineList.
                        removeLise_OnlineList.push_back(neededOnlineId);
                        remove_size--;
                    }
                }
            }
        }
        // 3. if remove_size > 0, select offline from online list.
        if(remove_size > 0)
        {
            for(auto& neededOnlineId:parsedSensorStateResult.vNeedOnlineSensorList)
            {
                if(std::find(removeLise_OnlineList.begin(), removeLise_OnlineList.end(), neededOnlineId) !=
                             removeLise_OnlineList.end()) {
                    // already be remove from last step.
                    continue;
                }
                for(auto& id:vOnlineList)
                {
                    if(is_same_sensor_group(neededOnlineId, id))
                    {
                        MY_LOGD("need online [%" PRIu32 "] is same group with [%" PRIu32 "](offline)",
                                                                neededOnlineId, id);
                        remove_item_from_residue_online_list(id);
                        parsedSensorStateResult.vNeedOfflineSensorList.push_back(id);
                        remove_size--;
                    }
                }
            }
        }
        // 4. [Error]
        if(remove_size > 0)
        {
            MY_LOGE("online(%zu) count(%d) remove_size(%" PRIu32 ")",
                            vOnlineList.size(),
                            mMaxOnlineSensorCount,
                            remove_size);
            MY_LOGE("offline logic error, please check");
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
            aee_system_exception(
                LOG_TAG,
                NULL,
                DB_OPT_DEFAULT,
                "offline logic error.\nCRDISPATCH_KEY:offline logic error.");
#endif
            return false;
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
updateSensorOnOffState(
    ParsedSensorStateResult const& parsedSensorStateResult
)
{
    for(auto&& item:parsedSensorStateResult.vNeedOnlineSensorList)
    {
        mvSwitchingIdList.push_back(item);
    }
    for(auto&& item:parsedSensorStateResult.vNeedOfflineSensorList)
    {
        mvSwitchingIdList.push_back(item);
    }
    // check need switch sensor
    if(parsedSensorStateResult.vNeedOnlineSensorList.size() > 0 ||
       parsedSensorStateResult.vNeedOfflineSensorList.size() > 0)
    {
        if(!isSensorSwitchGoing())
        {
            setSwitchGoingFlag(true);
            //reset initReuqst count
            setInitReqCnt(mAppInitReqCnt -1);
            for(auto&& item:parsedSensorStateResult.vNeedOnlineSensorList)
            {
                setSensorState(item, SensorStateType::E_ONLINE);
            }
            for(auto&& item:parsedSensorStateResult.vNeedOfflineSensorList)
            {
                setSensorState(item, SensorStateType::E_OFFLINE);
            }
            // start timer
            pre_time = std::chrono::system_clock::now();
        }
        else
        {
            MY_LOGW("want to switch, but already has task going.");
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
updateOnlineSensorResult(
    ParsedSensorStateResult &parsedSensorStateResult,
    std::unordered_map<uint32_t, sensorcontrol::SensorStatus>
                                        const& vResidueOnlineStateList
)
{
    // using mvSensorIdList as search rule, to keep camera open order.
    for(auto&& item:vResidueOnlineStateList)
    {
        const auto &id = item.first;
        // get sensor status info
        auto iter_sensor_state_info = mvSensorStateList.find(id);
        auto check_iter_sensor_state_info =
                    (iter_sensor_state_info != mvSensorStateList.end());
        if(check_iter_sensor_state_info)
        {
            // SensorStateInfo data
            auto &pSensorStateInfo = iter_sensor_state_info->second;
            // current online state
            const auto &eSensorOnlineResult = pSensorStateInfo.eSensorOnlineStatus;
            // decision online state
            const auto &eDecisionSensorOnlineResult = item.second;
            // is not switch
            //auto iter_switching = std::find(mvSwitchingIdList.begin(),
            //                                mvSwitchingIdList.end(),
            //                                id);
            // remove this flow for quick switching
            // bool ising) continue;Switching = (iter_switching != mvSwitchingIdList.end()) ? true : false;
            // if(isSwitch
            // check previous sensor status
            bool isGoToStandby =
                    (eSensorOnlineResult == sensorcontrol::SensorStatus::E_STREAMING) &&
                    (eDecisionSensorOnlineResult == sensorcontrol::SensorStatus::E_STANDBY);
            bool isStreaming =
                    (eDecisionSensorOnlineResult == sensorcontrol::SensorStatus::E_STREAMING);
            bool isGoToStreaming =
                    ((eSensorOnlineResult == sensorcontrol::SensorStatus::E_STANDBY) ||
                    (eSensorOnlineResult == sensorcontrol::SensorStatus::E_NONE)) &&
                    isStreaming;
            bool isStandby =
                    ((eSensorOnlineResult == sensorcontrol::SensorStatus::E_STANDBY) &&
                    (eDecisionSensorOnlineResult == sensorcontrol::SensorStatus::E_STANDBY)) ||
                    ((eSensorOnlineResult == sensorcontrol::SensorStatus::E_NONE) &&
                    (eDecisionSensorOnlineResult == sensorcontrol::SensorStatus::E_STANDBY));
            //
            if(!isStandby)
            {
                if(isStreaming)
                {
                    // reset idle count
                    pSensorStateInfo.iIdleCount = 0; // reset
                }
                if(isGoToStreaming)
                {
                    parsedSensorStateResult.vResumeSensorIdList.push_back(id);
                    MY_LOGD("sensorId (%d) want to go to streaming", id);
                    pSensorStateInfo.eSensorOnlineStatus = sensorcontrol::SensorStatus::E_STREAMING;
                }
                if(isGoToStandby && ((++pSensorStateInfo.iIdleCount) > MAX_IDLE_DECIDE_FRAME_COUNT))
                {
                    parsedSensorStateResult.vGotoStandbySensorIdList.push_back(id);
                    // need add to streaming list, otherwise vStreamingSensorIdList doesn't
                    // contain this id.
                    parsedSensorStateResult.vStreamingSensorIdList.push_back(id);
                    pSensorStateInfo.iIdleCount = 0; // reset
                    MY_LOGD("sensorId (%d) want to go to standby", id);
                    pSensorStateInfo.eSensorOnlineStatus = sensorcontrol::SensorStatus::E_STANDBY;
                }
                else
                {
                    parsedSensorStateResult.vStreamingSensorIdList.push_back(id);
                    MY_LOGD_IF(mLogLevel >= 1, "sensorId (%d) streaming isGoToStandby(%d)", id, isGoToStandby);
                }
            }
            else
            {
                parsedSensorStateResult.vStandbySensorIdList.push_back(id);
            }
            MY_LOGD_IF(mLogLevel >= 1, "sensorId (%d) isStreaming(%d), isStandby(%d), isGoToStreaming(%d), isGoToStandby(%d)",
                id, isStreaming, isStandby, isGoToStreaming, isGoToStandby);
        }
        else
        {
            MY_LOGE("cannot find id(%d) in mvSensorStateList", id);
            return false;
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
getQualityFlag(
    ParsedSensorStateResult const&result,
    Input const& inputParams,
    MUINT32 &qualityFlag
)
{
    int32_t slaveId = -1;
    for(auto& id:result.vStreamingSensorIdList)
    {
        if(result.masterId != id)
        {
            slaveId = id;
            break;
        }
    }
    qualityFlag =
                mpIspQualityManager->getQualityFlag(
                    inputParams.requestNo,
                    result.masterId,
                    slaveId
                );
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
handleStreamingListForSwitchingFlow(
    ParsedSensorStateResult &parsedSensorStateResult
)
{
    for(auto iter = parsedSensorStateResult.vStreamingSensorIdList.begin();
        iter != parsedSensorStateResult.vStreamingSensorIdList.end();)
    {
        auto iter_switching = std::find(mvSwitchingIdList.begin(),
                                          mvSwitchingIdList.end(),
                                          *iter);
        if(iter_switching != mvSwitchingIdList.end()) {
            iter = parsedSensorStateResult.vStreamingSensorIdList.erase(iter);
        }
        else {
            ++iter;
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
resetIdleCountToAllSensors()
{
    for(auto&& item:mvSensorStateList) {
        item.second.iIdleCount = 0;
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::
dump(
    ParsedSensorStateResult &parsedSensorStateResult
)
{
    // dump all list for debug
    String8 s("online sensor state:");
    s.appendFormat("\nStandbySensorIdList:");
    for(auto&& item:parsedSensorStateResult.vStandbySensorIdList)
    {
        s.appendFormat(" %d", item);
    }
    s.appendFormat("\nResumeSensorIdList:");
    for(auto&& item:parsedSensorStateResult.vResumeSensorIdList)
    {
        s.appendFormat(" %d", item);
    }
    s.appendFormat("\nStreamingSensorIdList:");
    for(auto&& item:parsedSensorStateResult.vStreamingSensorIdList)
    {
        s.appendFormat(" %d", item);
    }
    s.appendFormat("\nGotoStandbySensorIdList:");
    for(auto&& item:parsedSensorStateResult.vGotoStandbySensorIdList)
    {
        s.appendFormat(" %d", item);
    }
    s.appendFormat("\nneed online:");
    for(auto&& item:parsedSensorStateResult.vNeedOnlineSensorList)
    {
        s.appendFormat(" %d", item);
    }
    s.appendFormat("\nneed offline:");
    for(auto&& item:parsedSensorStateResult.vNeedOfflineSensorList)
    {
        s.appendFormat(" %d", item);
    }
    s.appendFormat("\nswitching:");
    for(auto&& item:mvSwitchingIdList)
    {
        s.appendFormat(" %d", item);
    }
    MY_LOGD("%s", s.string());
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
SensorStateManager::SensorGroupManager::
SensorGroupManager(){}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::SensorGroupManager::
setGroupMap(
    SensorMap<int> const* pGroupMap
)
{
    auto& groupMap = *pGroupMap;
    if (groupMap.empty()) {
        MY_LOGE("sensor group map is empty");
        return false;
    } else {
        mSensorMap = groupMap;
        android::String8 val("sensor group map:\n");
        for (auto& item : mSensorMap) {
            val.appendFormat("\t id(%d) group(%d)", item.first, item.second);
        }
        MY_LOGD("%s", val.string());
        return true;
    }
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStateManager::SensorGroupManager::
isSameGroup(
    uint32_t &val1,
    uint32_t &val2
)
{
    int temp_val1 = val1;
    int temp_val2 = val2;
    auto iter_val1 = mSensorMap.find(temp_val1);
    auto iter_val2 = mSensorMap.find(temp_val2);
    if(iter_val1 == mSensorMap.end() || iter_val2 == mSensorMap.end()) {
        MY_LOGA("query target [val1:%" PRIu32 "|val2:%" PRIu32 " not exist in sensor map, please check flow",
                    val1, val2);
        // cannot decide group, defalut trate as same group.
        return true;
    }
    if(iter_val1->second == iter_val2->second) {
        return true;
    }
    else {
        return false;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
SensorStatusDashboard::SensorStatusDashboard(
    const std::vector<int32_t> vSensorId,
    uint32_t loglevel) : mvSensorId(vSensorId), mLogLevel(loglevel)
{
  MY_LOGD("ctor");
}


/******************************************************************************
 *
 ******************************************************************************/
SensorStatusDashboard::~SensorStatusDashboard()
{
    MY_LOGD("release obj");
}


/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStatusDashboard::updateDecision(
  uint32_t requestNo, const std::vector<uint32_t>& streamingIds)
{
    std::lock_guard<std::mutex> lk(mMutex);
    std::set<uint32_t> sensorSet(streamingIds.begin(), streamingIds.end());
    mStreamingDecisionMap.emplace(requestNo, std::move(sensorSet));

    std::string streamingList;
    for (const auto& sensorId : streamingIds) {
        streamingList += std::to_string(sensorId);
    }

    MY_LOGD_IF(mLogLevel >= 2,
        "req(%u), decisionSensor(%s)", requestNo, streamingList.c_str());

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStatusDashboard::recordCallback(
    SensorStatusDashboard::SensorRecord const& record)
{
    std::lock_guard<std::mutex> lk(mMutex);

    if (record.isMaster) {
      mCurrentMasterSensor = record.sensorId;
    }

    auto it_decision = mStreamingDecisionMap.find(record.frameNumber);
    if (it_decision == mStreamingDecisionMap.end()) {
        MY_LOGE("No decision record (req:%u)", record.frameNumber);
        mvLatestStreamingSensor.emplace(record.sensorId);
        return false;
    }

    auto it_callback = mStreamingCallbackMap.find(record.frameNumber);
    if (it_callback == mStreamingCallbackMap.end()) {
        std::set<uint32_t> sensorSet{record.sensorId};
        mStreamingCallbackMap.emplace(record.frameNumber, std::move(sensorSet));
    } else {
        it_callback->second.emplace(record.sensorId);
    }

    auto cleanUpRequestHistory =
      [&](int32_t requestNo,
          std::unordered_map<int32_t, std::set<uint32_t>>& historyMap) {
      for (auto iter = historyMap.begin(); iter != historyMap.end(); ) {
          auto reqNumberKey = iter->first;
          // if map requestNo <= this one, cleanup
          if (reqNumberKey < requestNo) {
            iter = historyMap.erase(iter);
          } else {
            ++iter;
          }
      }
    };

    // update and cleanup
    if (mStreamingCallbackMap[record.frameNumber] ==
        mStreamingDecisionMap[record.frameNumber]) {
        // all expected sensors are arrived, update latest sensors
        mvLatestStreamingSensor.clear();
        mvLatestStreamingSensor = mStreamingCallbackMap[record.frameNumber];

        auto decision_sz = mStreamingDecisionMap.size();
        auto callback_sz = mStreamingCallbackMap.size();

        // remove older request history
        cleanUpRequestHistory(record.frameNumber, mStreamingDecisionMap);
        cleanUpRequestHistory(record.frameNumber, mStreamingCallbackMap);

        MY_LOGD_IF(
            mLogLevel >= 2, "[CLEANUP] decisionMap(size: %d->%d), callbackMap(size: %d->%d)",
            decision_sz, mStreamingDecisionMap.size(), callback_sz, mStreamingCallbackMap.size());
    }

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
SensorStatusDashboard::getStreamingIds(std::vector<int32_t>* pvSensorIds)
{
    std::lock_guard<std::mutex> lk(mMutex);
    std::string streamingList;
    auto& vSensorIds = (*pvSensorIds);
    vSensorIds.clear();
    for (auto const& sensorId : mvLatestStreamingSensor) {
        vSensorIds.push_back(static_cast<int32_t>(sensorId));
        streamingList += std::to_string(sensorId);
    }
    MY_LOGD_IF(mLogLevel >= 2, "latestStreamingSensor(%s), size(%zu)",
               streamingList.c_str(), (*pvSensorIds).size());
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
RequestSensorControlPolicy_Multicam::
RequestSensorControlPolicy_Multicam(
    CreationParams const& params
) : mPolicyParams(params)
{
    // create 3a object
    mpHal3AObj =
            std::shared_ptr<Hal3AObject>(
                    new Hal3AObject(
                        mPolicyParams.pPipelineStaticInfo->sensorId));
    // create device info helper
    mpDeviceInfoHelper =
            std::shared_ptr<DeviceInfoHelper>(
                    new DeviceInfoHelper(
                        mPolicyParams.pPipelineStaticInfo->openId));

    auto multicamInfo = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
    mMaxStreamingSize = 2;
    uint64_t timestamp = 0;
    if(multicamInfo)
    {
        timestamp = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->configTimestamp;
        mMaxStreamingSize = multicamInfo->mSupportPass1Number;
        if(multicamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
        {
            mFeatureMode = sensorcontrol::FeatureMode::E_Zoom;
        }
        else if(multicamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF)
        {
            mFeatureMode = sensorcontrol::FeatureMode::E_Bokeh;
        }
        else
        {
            mFeatureMode = sensorcontrol::FeatureMode::E_Multicam;
        }
        MY_LOGD("mFeatureMode(%" PRIu32 ")", mFeatureMode);
    }
    else
    {
        mFeatureMode = sensorcontrol::FeatureMode::E_None;
    }

    auto pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto entryPIPDevices =
            pParsedAppConfiguration->sessionParams.entryFor(MTK_STREAMING_FEATURE_PIP_DEVICES);
    // suppose PIP-FB has only 1 frontcam
    if (entryPIPDevices.count() == 2) {
        mMaxStreamingSize -= 1;
        MY_LOGD("PIP-FB, set mMaxStreamingSize: %d", mMaxStreamingSize);
    }

    mLogLevel = ::property_get_int32("vendor.debug.policy.sensorcontrol", -1);

    mSensorStateManager =
            std::unique_ptr<SensorStateManager,
                            std::function<void(SensorStateManager*)> >(
                                new SensorStateManager(
                                    mPolicyParams.pPipelineStaticInfo->openId,
                                    mPolicyParams.pPipelineStaticInfo->sensorId,
                                    mMaxStreamingSize,
                                    mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->initRequest,
                                    mLogLevel),
                                [](SensorStateManager *p)
                                {
                                    if(p)
                                    {
                                        delete p;
                                    }
                                });

    mBehaviorManager =
            std::unique_ptr<SensorControllerBehaviorManager,
                            std::function<void(SensorControllerBehaviorManager*)> >(
                                new SensorControllerBehaviorManager(
                                    mLogLevel),
                                [](SensorControllerBehaviorManager *p)
                                {
                                    if(p)
                                    {
                                        delete p;
                                    }
                                });

    mSensorStatusDashboard =
        std::unique_ptr<SensorStatusDashboard,
                        std::function<void(SensorStatusDashboard*)>>(
                            new SensorStatusDashboard(
                                mPolicyParams.pPipelineStaticInfo->sensorId,
                                mLogLevel),
                            [](SensorStatusDashboard *p) {
                                if (p) { delete p; }
                            });

    mpSensorControl = sensorcontrol::GetSensorControlInstance(timestamp);
}
/******************************************************************************
 *
 ******************************************************************************/
RequestSensorControlPolicy_Multicam::
~RequestSensorControlPolicy_Multicam()
{
    mpSensorControl->uninit();
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
evaluateConfiguration(
    Configuration_SensorControl_Params params
) -> int
{
    auto pParsedAppConfiguration = params.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto dualFeatureMode = pParsedAppConfiguration->pParsedMultiCamInfo->mDualFeatureMode;

    SensorMap<bool>& vLaggingConfigResult = *(params.pOut);
    auto pPipelineStaticInfo = params.pPipelineStaticInfo;

    mSensorStateManager->setSensorConfigureDone(false);

    int32_t masterId = -1;
    // determine lagging config
    if(dualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM || dualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF)

    {
        if (!DetermineP1LaggingConfig(params, vLaggingConfigResult/*out*/, masterId))
        {
            MY_LOGD("Use default lagging config setting");
        }
    }
    else
    {
        MY_LOGD("Not zoom feature, skip determining lagging config.");
        // if not zoom feature, it only can config p1node by mSupportPass1Number.
        for(int i = 0 ; i < pPipelineStaticInfo->sensorId.size() ; i++)
        {
            auto sensorId = pPipelineStaticInfo->sensorId[i];
            bool needLagging = false;
            if( i >= params.iMaxP1SupportCount )
                needLagging = true;

            vLaggingConfigResult.emplace(sensorId, needLagging);
        }
    }

    // set init steaming sensor list based on lagging config result
    for(int i = 0 ; i < pPipelineStaticInfo->sensorId.size() ; i++)
    {
        uint32_t sensorId = pPipelineStaticInfo->sensorId[i];

        auto it = vLaggingConfigResult.find(sensorId);
        if (it == vLaggingConfigResult.end())
            continue;

        bool lagging = vLaggingConfigResult[sensorId];
        sensorcontrol::SensorStatus status;
        if (lagging == false)
        {
            status = sensorcontrol::SensorStatus::E_STREAMING;
            mSensorStateManager->setInitParsedSensorStateResult(sensorId);
        }
        else
        {
            //fix crash of tele and uw streaming at the same time:
            //If tele/uw is master and need to switch to uw/tele,
            //wide need to standby so that tele/uw can be offlined in decideSensorOnOffState()
            if(i == 0){
                status = sensorcontrol::SensorStatus::E_STANDBY;
            }else{
                status = sensorcontrol::SensorStatus::E_NONE;
            }
        }
        mSensorStateManager->setSensorOnlineState(sensorId, status);
        MY_LOGD("sensor: %d, lagging: %d", sensorId, lagging);
    }
    mSensorStateManager->setInitMasterId(masterId);

    // prepare static meta for sensor control
    sensorcontrol::SensorControlParamInit param;

    // logical static metadata
    auto pLogicalMetadataProvider =
            NSMetadataProviderManager::valueForByDeviceId(pPipelineStaticInfo->openId);
    if (pLogicalMetadataProvider) {
        param.logicalStaticMeta = &pLogicalMetadataProvider->getMtkStaticCharacteristics();
    }

    // physical static metadata
    for (auto& sensorId : pPipelineStaticInfo->sensorId) {
        auto pMetadataProvider = NSMetadataProviderManager::valueFor(sensorId);
        if (pMetadataProvider) {
            auto staticMeta = pMetadataProvider->getMtkStaticCharacteristics();
            param.vPhysicalStaticMeta.emplace(sensorId, &staticMeta);
        }
    }
    mpSensorControl->init(param);

    // prepare info for exclusive group decision
    sensorcontrol::SensorGroupInfo info {
        .openId = pPipelineStaticInfo->openId,
        .sensorId = pPipelineStaticInfo->sensorId,
        .pGroupMap = params.pExclusiveGroup,
    };
    info.pipDeviceCount = [&](){
        auto entryPIPDevices =
            pParsedAppConfiguration->sessionParams.entryFor(MTK_STREAMING_FEATURE_PIP_DEVICES);
        return entryPIPDevices.count();
    }();
    for (auto const& id : pPipelineStaticInfo->sensorId) {
        float h = 0.0f;
        float v = 0.0f;
        mpDeviceInfoHelper->getPhysicalSensorFov(id, h, v);
        info.physicalFovMap.emplace(id, h);
    }

    // Generate exclusive group and update
    mpSensorControl->decideSensorGroup(info);
    if (!mSensorStateManager->setExclusiveGroup(info.pGroupMap)){
        MY_LOGE("Set exclusive group failed.");
        return false;
    }

    // Seamless Switch
    if (params.pSeamlessSwitchConfiguration) {
        // keep a copy of seamless switch config result from feature
        mSeamlessSwitchConfiguration = *(params.pSeamlessSwitchConfiguration);
        if (mSeamlessSwitchConfiguration.defaultScene >= 0) {
            mMain1CurrentSensorMode =
                static_cast<uint32_t>(mSeamlessSwitchConfiguration.defaultScene);
        }
        mZoom2TeleRatio = static_cast<float>(
                property_get_int32("vendor.debug.camera.seamless.tele.ratio", 4));
        MY_LOGI("Seamless Switch: [Config] workaround: init mMain1CurrentSensorMode = %u "
            "(with defaultScene), mZoom2TeleRatio = %f, get mSeamlessSwitchConfiguration = {%s}",
            mMain1CurrentSensorMode, mZoom2TeleRatio, toString(mSeamlessSwitchConfiguration).c_str());
    }
    return OK;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
DetermineP1LaggingConfig(
    Configuration_SensorControl_Params params,
    SensorMap<bool> &vNeedLaggingConfigOut,
    int32_t &masterId
) -> bool
{
    auto pPipelineStaticInfo = params.pPipelineStaticInfo;
    auto pParsedAppConfiguration = params.pPipelineUserConfiguration->pParsedAppConfiguration;
    bool isNeedLagConfig = true;

    // default : the first sensor no need lagging
    vNeedLaggingConfigOut.emplace(pPipelineStaticInfo->sensorId[0], false);
    for(int i = 1 ; i < pPipelineStaticInfo->sensorId.size() ; i++)
        vNeedLaggingConfigOut.emplace(pPipelineStaticInfo->sensorId[i], true);

    auto multicamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    if ( !multicamInfo )
    {
        MY_LOGD("No multicamInfo found");
        return false;
    }

    sensorcontrol::SensorControlParamOut senControlParamOut;
    sensorcontrol::SensorControlParamIn senControlParamIn;
    sensorcontrol::FeatureMode featureMode;

    // set feature
    if(multicamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
    {
        featureMode = sensorcontrol::FeatureMode::E_Zoom;
    }
    else if(multicamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF)
    {
        featureMode = sensorcontrol::FeatureMode::E_Bokeh;
        isNeedLagConfig = false;
        for(int i = 0 ; i < 2 ; i++)
        {
            uint32_t sensorId = pPipelineStaticInfo->sensorId[i];
            auto it = vNeedLaggingConfigOut.find(sensorId);
            if (it != vNeedLaggingConfigOut.end())
                it->second = false;
        }
        if(pPipelineStaticInfo->sensorId.size() > 2)
        {
            MY_LOGW("Device contains more than 2 sensor, sensor number(%zu)", pPipelineStaticInfo->sensorId.size());
        }
        MY_LOGI("Force set lag config to false Because it's Bokeh");
    }
    else
    {
        featureMode = sensorcontrol::FeatureMode::E_Multicam;
    }

    MY_LOGD("mFeatureMode(%" PRIu32 ")", featureMode);
    senControlParamIn.mFeatureMode = featureMode;

    // get crop region configuration
    IMetadata::IEntry cropEntry = pParsedAppConfiguration->sessionParams.entryFor(MTK_MULTI_CAM_CONFIG_SCALER_CROP_REGION);
    //MTK_MULTI_CAM_CONFIG_SCALER_CROP_REGION(left, top, right, bottom)
    if (cropEntry.count() == 4)
    {
        senControlParamIn.mrCropRegion.p.x = cropEntry.itemAt(0, Type2Type<MINT32>());
        senControlParamIn.mrCropRegion.p.y = cropEntry.itemAt(1, Type2Type<MINT32>());
        senControlParamIn.mrCropRegion.s.w = cropEntry.itemAt(2, Type2Type<MINT32>()) - senControlParamIn.mrCropRegion.p.x;
        senControlParamIn.mrCropRegion.s.h = cropEntry.itemAt(3, Type2Type<MINT32>()) - senControlParamIn.mrCropRegion.p.y;
    }
    else
    {
        MY_LOGE("Get MTK_MULTI_CAM_CONFIG_SCALER_CROP_REGION fail");
        return false;
    }

    //
    auto pLogicalDevice = MAKE_HalLogicalDeviceList();
    if(pLogicalDevice == nullptr)
    {
        MY_LOGE("create sensor hal list fail.");
        return false;
    }

    senControlParamIn.vSensorIdList = pPipelineStaticInfo->sensorId;

    for(auto&& sensorId : senControlParamIn.vSensorIdList)
    {
        auto pInfo = std::make_shared<sensorcontrol::SensorControlInfo>();
        if(pInfo == nullptr)
        {
            MY_LOGE("create SensorControlInfo fail");
            return false;
        }

       // IMetadata sensorMetadata = pLogicalDevice->queryStaticInfo(sensorId);
       // if(!IMetadata::getEntry<MRect>(&sensorMetadata, MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION, pInfo->mActiveArrayRegion))
       // {
       //     MY_LOGE("get MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION fail.");
        //    return false;
       // }
        getSensorData(sensorId, pInfo);
        senControlParamIn.vInfo.insert({sensorId, pInfo});
    }

    auto entryPIPDevices =
        pParsedAppConfiguration->sessionParams.entryFor(MTK_STREAMING_FEATURE_PIP_DEVICES);
    senControlParamIn.pipDeviceCount = entryPIPDevices.count();

    if (sensorcontrol::decision_sensor_control(senControlParamOut, senControlParamIn))
    {
        for(auto&& item : senControlParamOut.vResult)
        {
            bool needLagging = !(item.second->iSensorControl == sensorcontrol::SensorStatus::E_STREAMING);
            vNeedLaggingConfigOut.at(item.first) = needLagging;
            if (item.second->isMaster)
            {
                masterId = item.first;
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
handleSeamlessSwitch(
    requestsensorcontrol::RequestInputParams const& in,
    sensorcontrol::SensorControlParamOut &out,
    bool& isSeamlessSwitching,
    uint32_t& seamlessTargetSensorMode,
    bool& isLosslessZoom,
    uint32_t& losslessSensorMode
) -> bool
{
    if (!mPolicyParams.pPipelineStaticInfo->isSeamlessSwitchSupported ||
            !mSeamlessSwitchConfiguration.isSeamlessPreviewEnable) {
        return false;
    }
    if (!mSeamlessSwitchConfiguration.isInitted ||
            mSeamlessSwitchConfiguration.defaultScene < 0 ||
            mSeamlessSwitchConfiguration.cropScene < 0) {
        MY_LOGE("Seamless Switch: [R%u] something wrong with Scenes, "
            "mSeamlessSwitchConfiguration = {%s}",
            in.requestNo, toString(mSeamlessSwitchConfiguration).c_str());
        return false;
    }
    if (!in.bIsHWReady) {
        MY_LOGW("Seamless Switch: [R%u] hw not ready for seamless switch",
            in.requestNo);
        return true;
    }

    const uint32_t main1SensorId = mPolicyParams.pPipelineStaticInfo->sensorId[0];
    const uint32_t defaultMode = static_cast<uint32_t>(mSeamlessSwitchConfiguration.defaultScene);
    const uint32_t cropMode = static_cast<uint32_t>(mSeamlessSwitchConfiguration.cropScene);
    const float cropZoomRatio = mSeamlessSwitchConfiguration.cropZoomRatio;
    bool needPreviewSwitching = false;

    MY_LOGD_IF(mLogLevel >= 1,
        "Seamless Switch: [R%u Debug] mMain1CurrentSensorMode = %u, "
        "defaultMode = %u, cropMode = %u, zoomRatio = %f",
        in.requestNo, mMain1CurrentSensorMode, defaultMode, cropMode, out.fZoomRatio);

    const uint32_t sourceSensorMode = mMain1CurrentSensorMode;
    uint32_t targetSensorMode = 0;
    if (out.fZoomRatio >= cropZoomRatio && mMain1CurrentSensorMode == defaultMode) {
        targetSensorMode = cropMode;
        needPreviewSwitching = true;
    } else if (out.fZoomRatio < cropZoomRatio && mMain1CurrentSensorMode == cropMode) {
        targetSensorMode = defaultMode;
        needPreviewSwitching = true;
    }

    if (needPreviewSwitching) {
        MY_LOGI("Seamless Switch: [R%u Preview Switching] "
            "sensor mode (%u -> %u), zoomRatio = %f",
            in.requestNo, sourceSensorMode, targetSensorMode, out.fZoomRatio);
        isSeamlessSwitching = true;
        seamlessTargetSensorMode = targetSensorMode;
        mMain1CurrentSensorMode = targetSensorMode;
    }

    if (mMain1CurrentSensorMode == cropMode) {
        isLosslessZoom = true;
        losslessSensorMode = mMain1CurrentSensorMode;
        if (!convertRegions2SensorModeForSensorId(main1SensorId, mMain1CurrentSensorMode, out)) {
            MY_LOGE("Seamless Switch: [R:%u] convertRegions2SensorModeForSensorId failed!",
                in.requestNo);
        }
    }

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
convertRegions2SensorModeForSensorId(
    MUINT32 sensorId,
    MUINT32 targetMode,
    sensorcontrol::SensorControlParamOut &out
) -> bool
{
    auto result = out.vResult.at(sensorId);
    if(result == nullptr)
        return false;
    NSCamHW::HwTransHelper helper(sensorId);
    NSCamHW::HwMatrix mat;
    #define transformRect(srcRect,dstRect)                    \
        do {                                                  \
            MRect tempRect;                                   \
            tempRect = srcRect;                               \
            mat.transform(tempRect, dstRect);                 \
        } while(0)

    #define transformRegion(srcRegion,dstRegion)              \
        do {                                                  \
            MRect tempRect(MPoint(srcRegion[0],srcRegion[1]),MSize(srcRegion[2]-srcRegion[0],srcRegion[3]-srcRegion[0]));\
            MRect dstRect;                                    \
            mat.transform(tempRect, dstRect);                 \
            dstRegion.clear();                                \
            dstRegion.push_back(dstRect.leftTop().x);            \
            dstRegion.push_back(dstRect.leftTop().y);            \
            dstRegion.push_back(dstRect.leftTop().x+dstRect.width());\
            dstRegion.push_back(dstRect.leftTop().y+dstRect.height());\
        } while(0)

    #define printRect(rect)                                   \
        do {                                                  \
            MY_LOGD("Rect: %d %d %d*%d",rect.leftTop().x,rect.leftTop().y,rect.width(),rect.height());\
        } while(0)

    #define printRegion(region)                               \
        do {                                                  \
            if(region.size()>=4)                              \
                MY_LOGD("Region: %d %d %d %d",region[0],region[1],region[2],region[3]);\
        } while(0)

    if(!helper.getMatrixFromActive(targetMode, mat))
    {
        MY_LOGE("Get hw matrix failed. sensor id(%d) sensor mode(%d)",
                    sensorId,
                    targetMode);
        return false;
    }
    // for debugging print all the region first
    printRect(result->mrAlternactiveCropRegion);
    printRect(result->mrAlternactiveCropRegion_Cap);
    printRect(result->mrAlternactiveCropRegion_3A);
    printRegion(result->mrAlternactiveAfRegion);
    printRegion(result->mrAlternactiveAeRegion);
    printRegion(result->mrAlternactiveAwbRegion);

    transformRect(result->mrAlternactiveCropRegion,result->mrAlternactiveCropRegion);
    transformRect(result->mrAlternactiveCropRegion_Cap,result->mrAlternactiveCropRegion_Cap);
    transformRect(result->mrAlternactiveCropRegion_3A,result->mrAlternactiveCropRegion_3A);
    transformRegion(result->mrAlternactiveAfRegion,result->mrAlternactiveAfRegion);
    transformRegion(result->mrAlternactiveAeRegion,result->mrAlternactiveAeRegion);
    transformRegion(result->mrAlternactiveAwbRegion,result->mrAlternactiveAwbRegion);

    // for debugging print all the region again
    printRect(result->mrAlternactiveCropRegion);
    printRect(result->mrAlternactiveCropRegion_Cap);
    printRect(result->mrAlternactiveCropRegion_3A);
    printRegion(result->mrAlternactiveAfRegion);
    printRegion(result->mrAlternactiveAeRegion);
    printRegion(result->mrAlternactiveAwbRegion);
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
evaluateRequest(
    requestsensorcontrol::RequestOutputParams& out,
    requestsensorcontrol::RequestInputParams const& in
) -> int
{
    if(in.pRequest_SensorMode == nullptr)
    {
        MY_LOGE("in.pRequest_SensorMode is null, please check flow");
        return UNKNOWN_ERROR;
    }
    if(out.pMultiCamReqOutputParams == nullptr)
    {
        MY_LOGE("out.pMultiCamReqOutputParams is null");
        return UNKNOWN_ERROR;
    }

    // check touch af done before switch sensor flow
    mSensorStateManager->checkAfDoneBeforeSwitch(in.pRequest_AppControl, in.requestNo);
    mSensorStateManager->checkPrecaptureTrigger(in.pRequest_AppControl, in.requestNo);

    bool cachedNeedNotify3ASyncDone = false;
    {
        std::lock_guard<std::mutex> lk(mCapControlLock);

        // snapshot current request's notify3Async value,
        // in case switching is done during this evaluation
        // notify3ASync is set without sync2a metadata
        cachedNeedNotify3ASyncDone = mbNeedNotify3ASyncDone;

        // check is capture req
        if(in.needP2CaptureNode || in.bLogicalRawOutput)
        {
            //record the captureNumber
            mCaptureReqList.push_back(in.requestNo);
            mSensorStateManager->setNextCaptureDone(false);
            MINT32 captureCountTarget = 1;
            if(IMetadata::getEntry<MINT32>(in.pRequest_AppControl, MTK_MULTI_CAM_CAPTURE_COUNT, captureCountTarget))
            {
                if(captureCountTarget > 1)
                {
                    // capture > 1, using capture count control.
                    isUseCaptureCountControl = true;
                    if(!isCaptureing)
                    {
                        capureCount = captureCountTarget;
                        isCaptureing = true;
                    }
                    else
                    {
                        capureCount--;
                        if(capureCount == 1)
                        {
                            // turn flag off
                            isUseCaptureCountControl = false;
                        }
                    }
                }
                else if(captureCountTarget == 1)
                {
                    isUseCaptureCountControl = false;
                    isCaptureing = true;
                }
            }
            else
            {
                // If cannot get capture count, keep the same.
                isCaptureing = true;
                isUseCaptureCountControl = false;
            }
            MY_LOGD("capTotal(%d) less(%d) isCap(%d) useCapCountControl(%d)",
                                            captureCountTarget,
                                            capureCount,
                                            isCaptureing,
                                            isUseCaptureCountControl);
        }
        else
        {
            if(isUseCaptureCountControl)
            {
                isCaptureing = true;
            }
            else
            {
                isCaptureing = false;
            }
        }
    }
    sensorcontrol::SensorControlParamIn senControlParamIn;
    sensorcontrol::SensorControlParamOut senControlParamOut;
    ParsedSensorStateResult parsedSensorStateResult;
    //
    senControlParamIn.mFeatureMode = mFeatureMode;
    senControlParamIn.mRequestNo = in.requestNo;
    senControlParamIn.pAppControl = in.pRequest_AppControl;
    senControlParamIn.physicalSensorIdList = in.physicalSensorIdList;
    senControlParamIn.mbInSensorZoomEnable =
        mSeamlessSwitchConfiguration.isSeamlessEnable &&
        mSeamlessSwitchConfiguration.isSeamlessPreviewEnable;
    if (senControlParamIn.mbInSensorZoomEnable) {
        senControlParamIn.mZoom2TeleRatio = mZoom2TeleRatio;
    }
    //
    getPolicyData(senControlParamIn, in);

    auto entryPIPDevices =
        in.pRequest_AppControl->entryFor(MTK_STREAMING_FEATURE_PIP_DEVICES);
    senControlParamIn.pipDeviceCount = entryPIPDevices.count();

    bool ret = false;
    ret = mpSensorControl->doDecision(senControlParamOut, senControlParamIn);

    // dump third party decision result.
    dumpSensorControlParam(senControlParamOut, senControlParamIn, in);
    if(mSensorStateManager != nullptr)
    {
        handleSeamlessSwitch(in, senControlParamOut,
            out.isSeamlessSwitching, out.seamlessTargetSensorMode,
            out.isLosslessZoom, out.losslessZoomSensorMode);

        bool bFirstReqCapture = (mCurrentActiveFrameNo == -1 && (in.needP2CaptureNode || in.bLogicalRawOutput || in.bPhysicalRawOutput));
        SensorStateManager::Input sensorStateParam = {
                                                        .requestNo = in.requestNo,
                                                        .isCaptureing = isCaptureing,
                                                        .bFirstReqCapture = bFirstReqCapture,
                                                        .pvP1HwSetting = in.pvP1HwSetting};
        if(mSensorStateManager->update(senControlParamOut, parsedSensorStateResult, sensorStateParam))
        {
            // update metadata and specific behavior control
            if(mBehaviorManager != nullptr)
            {
                BehaviorUserData userData;
                // set ae init setting for 2A.
                {
                    std::lock_guard<std::mutex> lk(mCapControlLock);
                    for(auto&& resume_id:parsedSensorStateResult.vResumeSensorIdList)
                    {
                        // for go to streaming path, it has to set init ae to other cam.
                        mpHal3AObj->setSync2ASetting(mCurrentActiveMasterId, resume_id);
                    }
                }
                userData.parsedResult = &parsedSensorStateResult;
                userData.requestNo = in.requestNo;
                userData.masterId = parsedSensorStateResult.masterId;
                userData.vSensorId = mPolicyParams.pPipelineStaticInfo->sensorId;
                userData.vSensorMode = *in.pRequest_SensorMode;
                //userData.isSensorSwitchGoing = mSensorStateManager->isSensorSwitchGoing();
                userData.isSensorSwitchGoing = mSensorStateManager->isInitRequestGoing();
                if (parsedSensorStateResult.vStreamingSensorIdList.size() > 1 &&
                    cachedNeedNotify3ASyncDone)
                {
                    std::lock_guard<std::mutex> lk(mCapControlLock);
                    userData.isNeedNotifySync3A = true;
                    mbNeedNotify3ASyncDone = false;
                    MY_LOGI("set 3a notify");
                }
                out.pMultiCamReqOutputParams->masterId = parsedSensorStateResult.masterId;
                //for capture use
                out.pMultiCamReqOutputParams->prvMasterId = mCurrentActiveMasterId;
                out.pMultiCamReqOutputParams->qualityFlag = parsedSensorStateResult.qualityFlag;
                out.pMultiCamReqOutputParams->isZoomControlUpdate = senControlParamOut.isZoomControlUpdate;
                if(mFeatureMode != sensorcontrol::FeatureMode::E_Zoom)
                {
                    // zoom has special sync af control.
                    // zoom no needs always enable sync af.
                    userData.enableSyncAf = true;
                }
                userData.fZoomRatio = senControlParamOut.fZoomRatio;
                float cropRatio = 1.0/(userData.fZoomRatio);
                StereoSettingProvider::setPreviewCropRatio(cropRatio);
                mBehaviorManager->update(out, userData);
            }
            // In switch flow, it needs master and slave id for wake up next 3a setting.
            // cannot set this before mBehaviorManager->update.
            // Otherwise, sync3a will work incorrect timing.
            if(parsedSensorStateResult.vNeedOnlineSensorList.size() > 0 &&
               parsedSensorStateResult.vStreamingSensorIdList.size() > 0)
            {
                out.pMultiCamReqOutputParams->switchControl_Sync2ASensorList.push_back(parsedSensorStateResult.previousMasterId);
                MY_LOGD("master id = %d, pre-master id = %d", out.pMultiCamReqOutputParams->masterId, parsedSensorStateResult.previousMasterId);
                // sync 2a only supports 2 sensors, so no need to loop through
                // the list.
                auto id = parsedSensorStateResult.vNeedOnlineSensorList[0];
                out.pMultiCamReqOutputParams->switchControl_Sync2ASensorList.push_back(id);
                // if need offline sensor, original quality flag has to change.
                if(out.pMultiCamReqOutputParams->switchControl_Sync2ASensorList.size() < 2) {
                    MY_LOGA("switchControl_Sync2ASensorList(%zu) < 2",
                                out.pMultiCamReqOutputParams->switchControl_Sync2ASensorList.size());
                }
            }
            // data clone: todo
            out.pMultiCamReqOutputParams->flushSensorIdList = parsedSensorStateResult.vNeedOfflineSensorList;
            out.pMultiCamReqOutputParams->configSensorIdList = parsedSensorStateResult.vNeedOnlineSensorList;
            out.pMultiCamReqOutputParams->goToStandbySensorList = parsedSensorStateResult.vGotoStandbySensorIdList;
            out.pMultiCamReqOutputParams->streamingSensorList = parsedSensorStateResult.vStreamingSensorIdList;
            out.pMultiCamReqOutputParams->standbySensorList = parsedSensorStateResult.vStandbySensorIdList;
            out.pMultiCamReqOutputParams->resumeSensorList = parsedSensorStateResult.vResumeSensorIdList;
            out.pMultiCamReqOutputParams->bSwitchNoPreque = (bFirstReqCapture &&
                                                             out.pMultiCamReqOutputParams->configSensorIdList.size() > 0);
            // after decide streaming sensor, it has to update list which provide by P2NodeDecision
            bool ret = updateP2List(in, out, parsedSensorStateResult);
            if(!ret)
                return -1;
            {
                // push data to streaming list.
                std::lock_guard<std::mutex> lk(mCapControlLock);
                mvReqSensorStreamingMap.emplace(in.requestNo, out.pMultiCamReqOutputParams->streamingSensorList);
            }

            if (mSensorStatusDashboard.get()) {
                mSensorStatusDashboard->updateDecision(
                  in.requestNo, out.pMultiCamReqOutputParams->streamingSensorList);
            }
        }
        else
        {
            MY_LOGE("update sensor state fail");
            goto lbExit;
        }
    }
lbExit:
    return OK;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
sendPolicyDataCallback(
    MUINTPTR arg1,
    MUINTPTR arg2,
    MUINTPTR arg3
) -> bool
{
    std::lock_guard<std::mutex> lk(mCapControlLock);
    uint32_t type = *(uint32_t*)arg1;
    if(type == SetMasterId)
    {
        uint32_t frameno = *(uint32_t*)arg2;
        uint32_t masterid = *(uint32_t*)arg3;
        MY_LOGD("receive type(%" PRIu32 ") frameno(%" PRIu32 ") master(%" PRIu32 ") isCaptureing(%d)",
                    type, frameno, masterid, isCaptureing);
        if(!isCaptureing)
        {
            mCurrentActiveMasterId = masterid;
            mCurrentActiveFrameNo = frameno;
        }
        {
            SensorStatusDashboard::SensorRecord record = {
                .sensorId = masterid,
                .frameNumber = static_cast<int32_t>(frameno),
                .isMaster = true
            };
            mSensorStatusDashboard->recordCallback(record);

            // remove old request data
            for(auto iter = mvReqSensorStreamingMap.begin();
                iter != mvReqSensorStreamingMap.end();)
            {
                if(iter->first < mCurrentActiveFrameNo) {
                    iter = mvReqSensorStreamingMap.erase(iter);
                }
                else {
                    ++iter;
                }
            }
        }
    }
    else if (type == SetStreamingId)
    {
        uint32_t frameno = *(uint32_t*)arg2;
        uint32_t streamingId = *(uint32_t*)arg3;
        MY_LOGD("SetStreamingId receive type(%" PRIu32 ") frameno(%" PRIu32 ") streamingId(%d)",
                type, frameno, streamingId);
        {
            SensorStatusDashboard::SensorRecord record = {
                .sensorId = streamingId,
                .frameNumber = static_cast<int32_t>(frameno),
                .isMaster = false
            };
            mSensorStatusDashboard->recordCallback(record);
        }

    }
    else if (type == UpdateStreamingMeta)
    {
        uint32_t frameno = *(uint32_t*)arg2;
        std::vector<int32_t>* pvSensorIds = reinterpret_cast<std::vector<int32_t>*>(arg3);
        MY_LOGD("UpdateStreamingMeta, receive type(%" PRIu32 ") frameno(%" PRIu32 ")",
                type, frameno);
        mSensorStatusDashboard->getStreamingIds(pvSensorIds);
    }
    else if(type == SwitchDone)
    {
        MY_LOGI("switch done");
        if(mSensorStateManager != nullptr)
        {
            mSensorStateManager->switchSensorDone();
            mbNeedNotify3ASyncDone = true;
        }
    }
    else if(type == IspQualitySwitchState)
    {
        uint32_t state = *(uint32_t*)arg3;
        if(mSensorStateManager != nullptr)
        {
            if(state == IspSwitchState::Done)
            {
                MY_LOGI("isp quality switch done");
                mSensorStateManager->switchQualityDone();
            }
        }
    }
    else if (type == NextCapture)
    {
        uint32_t requstno = *(uint32_t*)arg2;
        auto reqIter =
            std::find(mCaptureReqList.begin(),
                      mCaptureReqList.end(),
                        requstno);
        if(reqIter != mCaptureReqList.end()){
            mCaptureReqList.erase(reqIter);
            if (mCaptureReqList.size() == 0 )
            {
                mSensorStateManager->setNextCaptureDone(true);
                mSensorStateManager->resetPrecaptureTrigger();
            }
        }
    }
    else if (type == RequestDone) {
        uint32_t requstno = *(uint32_t*)arg2;
        auto reqIter =
            std::find(mCaptureReqList.begin(),
                      mCaptureReqList.end(),
                      requstno);
        if(reqIter != mCaptureReqList.end()) {
            mCaptureReqList.erase(reqIter);
            if (mCaptureReqList.size() == 0) {
                mSensorStateManager->setNextCaptureDone(true);
                mSensorStateManager->resetPrecaptureTrigger();
            }
        }
    }
    else if (type == AfSearching)
    {
        uint32_t requestno = *(uint32_t*)arg2;
        uint32_t state = *(uint32_t*)arg3;
        if (mSensorStateManager != nullptr) {
            mSensorStateManager->updateAfStateDone(requestno, state);
        }
    }
    else if (type == ConfigDone)
    {
        mSensorStateManager->setSensorConfigureDone(true);
    }
    else if (type == Flush)
    {
        if (mSensorStateManager != nullptr) {
            auto vSensorIds = mPolicyParams.pPipelineStaticInfo->sensorId;
            for(size_t i=0;i<vSensorIds.size();i++)
            {
                if(i==0)   //main online others offline
                {
                    mSensorStateManager->setSensorState(vSensorIds[i], SensorStateManager::SensorStateType::E_ONLINE);
                }
                else
                {
                    mSensorStateManager->setSensorState(vSensorIds[i], SensorStateManager::SensorStateType::E_OFFLINE);
                }
            }
           mSensorStateManager->switchSensorDone();
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
getPolicyData(
    sensorcontrol::SensorControlParamIn &sensorControlIn,
    requestsensorcontrol::RequestInputParams const& in
) -> bool
{
    auto pAppMetadata = in.pRequest_AppControl;
    auto pvP1HwSetting = in.pvP1HwSetting;
    if(pAppMetadata == nullptr)
    {
        MY_LOGE("create pAppMetadata fail");
        return false;
    }

    for(const auto& _sensorId : mPolicyParams.pPipelineStaticInfo->sensorId)
    {
        if(in.pNeedP1Nodes->count(_sensorId) && in.pNeedP1Nodes->at(_sensorId))
        {
            sensorControlIn.vSensorIdList.push_back(_sensorId);
        }
    }

    // (1) set sensor related information. (3A data, sensor info, app data)
    for (auto&& sensorId : sensorControlIn.vSensorIdList)
    {
        auto pInfo = std::make_shared<sensorcontrol::SensorControlInfo>();
        if(pInfo == nullptr)
        {
            MY_LOGE("create SensorControlInfo fail");
            return false;
        }
        get3AData(sensorId, pInfo);
        getSensorData(sensorId, pInfo);
        if( pvP1HwSetting->count(sensorId) ) {
            pInfo->mRrzoSize = pvP1HwSetting->at(sensorId).rrzoDefaultRequest.imageSize;
        }
        sensorControlIn.vInfo.insert({sensorId, pInfo});
        MY_LOGD_IF(mLogLevel >= 1, "mSensorid:%d,RrzoSize:%dx%d",sensorId,pInfo->mRrzoSize.w,pInfo->mRrzoSize.h);
    }
    // get crop region from app metadata.
    IMetadata::IEntry af_entry = pAppMetadata->entryFor(MTK_CONTROL_AF_REGIONS);
    IMetadata::IEntry ae_entry = pAppMetadata->entryFor(MTK_CONTROL_AE_REGIONS);
    IMetadata::IEntry awb_entry = pAppMetadata->entryFor(MTK_CONTROL_AWB_REGIONS);

    MRect crop_region;
    if(IMetadata::getEntry<MRect>(pAppMetadata, MTK_SCALER_CROP_REGION, crop_region))
    {
        MY_LOGD_IF(mLogLevel >= 1, "crop region(%d, %d, %d, %d)",
                                                crop_region.p.x,
                                                crop_region.p.y,
                                                crop_region.s.w,
                                                crop_region.s.h);
    }
    sensorControlIn.mrCropRegion = crop_region;
    if (!af_entry.isEmpty()){
        vector<MINT32> mvaf_region;
        for(MINT32 k=0;k<af_entry.count();k++){
            mvaf_region.push_back(af_entry.itemAt(k, Type2Type< MINT32 >()));
        }
        auto af_xmin = af_entry.itemAt(0,   NSCam::Type2Type<MINT32>());
        auto af_ymin = af_entry.itemAt(1,   NSCam::Type2Type<MINT32>());
        auto af_xmax = af_entry.itemAt(2,   NSCam::Type2Type<MINT32>());
        auto af_ymax = af_entry.itemAt(3,   NSCam::Type2Type<MINT32>());
        sensorControlIn.mrAfRegion= mvaf_region;
        MY_LOGD_IF(mLogLevel >= 1, "af region(%d, %d, %d, %d)",
                                                af_xmin,
                                                af_ymin,
                                                af_xmax-af_xmin,
                                                af_ymax-af_ymin);
    }
    if (!ae_entry.isEmpty()){
        vector<MINT32> mvae_region;
        for(MINT32 k=0;k<ae_entry.count();k++){
            mvae_region.push_back(ae_entry.itemAt(k, Type2Type< MINT32 >()));
        }
        auto ae_xmin = ae_entry.itemAt(0,   NSCam::Type2Type<MINT32>());
        auto ae_ymin = ae_entry.itemAt(1,   NSCam::Type2Type<MINT32>());
        auto ae_xmax = ae_entry.itemAt(2,   NSCam::Type2Type<MINT32>());
        auto ae_ymax = ae_entry.itemAt(3,   NSCam::Type2Type<MINT32>());
        sensorControlIn.mrAeRegion= mvae_region;
        MY_LOGD_IF(mLogLevel >= 1, "ae region(%d, %d, %d, %d)",
                                                ae_xmin,
                                                ae_ymin,
                                                ae_xmax-ae_xmin,
                                                ae_ymax-ae_ymin);
    }
    if (!awb_entry.isEmpty()){
        vector<MINT32> mvawb_region;
        for(MINT32 k=0;k<awb_entry.count();k++){
            mvawb_region.push_back(awb_entry.itemAt(k, Type2Type< MINT32 >()));
        }
        auto awb_xmin = awb_entry.itemAt(0,   NSCam::Type2Type<MINT32>());
        auto awb_ymin = awb_entry.itemAt(1,   NSCam::Type2Type<MINT32>());
        auto awb_xmax = awb_entry.itemAt(2,   NSCam::Type2Type<MINT32>());
        auto awb_ymax = awb_entry.itemAt(3,   NSCam::Type2Type<MINT32>());
        sensorControlIn.mrAwbRegion= mvawb_region;
        MY_LOGD_IF(mLogLevel >= 1, "awb region(%d, %d, %d, %d)",
                                                awb_xmin,
                                                awb_ymin,
                                                awb_xmax-awb_xmin,
                                                awb_ymax-awb_ymin);
    }
    mpDeviceInfoHelper->getLogicalFocalLength(sensorControlIn.mAvailableFocalLength);
    if(IMetadata::getEntry<MFLOAT>(in.pRequest_AppControl, MTK_LENS_FOCAL_LENGTH, sensorControlIn.mFocalLength))
    {
        MY_LOGD_IF(mLogLevel >= 1, "focal_length(%f)", sensorControlIn.mFocalLength);
    }
    // get zoom ratio
    IMetadata::getEntry<MFLOAT>(pAppMetadata, MTK_MULTI_CAM_ZOOM_VALUE, sensorControlIn.mZoomRatio);
    // get app iso & EV
    IMetadata::getEntry<MINT32>(pAppMetadata, MTK_SENSOR_SENSITIVITY, sensorControlIn.mISO);
    IMetadata::getEntry<MINT32>(pAppMetadata, MTK_CONTROL_AE_EXPOSURE_COMPENSATION, sensorControlIn.mEV);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
get3AData(
    int32_t sensorId,
    std::shared_ptr<sensorcontrol::SensorControlInfo> &info
) -> bool
{
    if(info == nullptr)
    {
        MY_LOGE("SensorControlInfo is nullptr, sensorId (%d)", sensorId);
        return false;
    }
    // update 3a info first
    if(mpHal3AObj == nullptr)
    {
        MY_LOGE("mpHal3AObj is nullptr");
        return false;
    }
    mpHal3AObj->update(); // update 3a cache info.
    info->bAFDone = mpHal3AObj->isAFDone(sensorId);
    info->iAEIso = mpHal3AObj->getAEIso(sensorId);
    info->iAELV_x10 = mpHal3AObj->getAELv_x10(sensorId);
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
getSensorData(
    int32_t sensorId,
    std::shared_ptr<sensorcontrol::SensorControlInfo> &info
) -> bool
{
    if(info == nullptr)
    {
        MY_LOGE("SensorControlInfo is nullptr, sensorId (%d)", sensorId);
        return false;
    }
    if(mpDeviceInfoHelper == nullptr)
    {
        MY_LOGE("mpDeviceInfoHelper is nullptr, sensorId (%d)", sensorId);
        return false;
    }
    mpDeviceInfoHelper->getPhysicalSensorFov(
                                    sensorId,
                                    info->fSensorFov_H,
                                    info->fSensorFov_V);
    mpDeviceInfoHelper->getPhysicalSensorActiveArray(
                                    sensorId,
                                    info->mActiveArrayRegion);
    mpDeviceInfoHelper->getPhysicalSensorFullSize(
                                    sensorId,
                                    info->msSensorFullSize);
    mpDeviceInfoHelper->getPhysicalFocalLength(
                                    sensorId,
                                    info->mFocalLength);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
getDecisionResult(
    sensorcontrol::SensorControlParamOut &out,
    sensorcontrol::SensorControlParamIn &in
) -> bool
{
    auto ret = sensorcontrol::decision_sensor_control(out, in);
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
updateP2List(
    requestsensorcontrol::RequestInputParams const& in,
    requestsensorcontrol::RequestOutputParams & out,
    ParsedSensorStateResult const& result
) -> bool
{
    // check preview master is exist in streaming list first.
    uint32_t prvMasterId = -1;
    if(in.needP2CaptureNode || in.bLogicalCaptureOutput || in.bLogicalRawOutput || in.bPhysicalRawOutput)
    {
        std::lock_guard<std::mutex> lk(mCapControlLock);
        {
            prvMasterId = mCurrentActiveMasterId;
            auto prvMasterIter =
                    std::find(result.vStreamingSensorIdList.begin(),
                              result.vStreamingSensorIdList.end(),
                              mCurrentActiveMasterId);
            if(prvMasterIter == result.vStreamingSensorIdList.end())
            {
                if(result.vStreamingSensorIdList.size() > 0)
                {
                    prvMasterId = result.vStreamingSensorIdList[0];
                    MY_LOGI("capture request, preview master not exist in streaming list. assign first element.");
                }
                else
                {
                    MY_LOGI("capture request, but no streaming sensor. request fail");
                    return false;
                }
            }
        }
    }
    auto checkPhysicalLsitAndRemoveItemBySensorId = [&out](uint32_t id)
    {
        // remove data from p2 list.
        auto removeItemBySensorId = [](
                                    uint32_t &targetId,
                                    RequestOutputParams::StreamIdMap* pMap)
        {
            if(pMap == nullptr) return false;
            bool ret = false;
            auto iter = pMap->find(targetId);
            if(iter != pMap->end())
            {
                pMap->erase(iter);
                ret = true;
            }
            return ret;
        };
        bool hasMarkError = false;
        hasMarkError |= removeItemBySensorId(id, out.vMetaStreamId_from_CaptureNode_Physical);
        hasMarkError |= removeItemBySensorId(id, out.vImageStreamId_from_CaptureNode_Physical);
        hasMarkError |= removeItemBySensorId(id, out.vMetaStreamId_from_StreamNode_Physical);
        hasMarkError |= removeItemBySensorId(id, out.vImageStreamId_from_StreamNode_Physical);
        return hasMarkError;
    };
    auto updateStreamandMasterId = [&out](uint32_t const& masterId){
        out.pMultiCamReqOutputParams->prvMasterId = masterId;
        out.pMultiCamReqOutputParams->prvStreamingSensorList.push_back(masterId);
    };
    MINT32 isCShot = false;
    IMetadata::getEntry<MINT32>(in.pRequest_AppControl, MTK_CSHOT_FEATURE_CAPTURE, isCShot);
    {
        // for capture update.
        std::lock_guard<std::mutex> lk(mCapControlLock);

        // special flow for tk zoom: only support capture master cam.

        if(mCurrentActiveFrameNo == -1 && (in.needP2CaptureNode || in.bLogicalRawOutput || in.bPhysicalRawOutput))
        {
            MY_LOGD("First request is capturing");

            char tempbuf[1024] = {0};
            ::property_get("vendor.debug.camera.prvid", tempbuf, "-1");
            bool useDebugPrvId = (0 != strcmp(tempbuf, "-1"));

            if(useDebugPrvId)
            {
                MY_LOGD("Use debug prvId");

                char const *delim = ",";
                char * pch;
                pch = strtok(tempbuf, delim);

                // if zoom ratio is 1x at start, streamingList maybe only has 1 sensor.
                // In this case, we can't test physical stream,
                // so use property to customize prvlist and prvid for isp test.
                out.pMultiCamReqOutputParams->standbySensorList.clear();
                out.pMultiCamReqOutputParams->streamingSensorList.clear();

                while (pch != NULL)
                {
                    int value = atoi(pch);
                    out.pMultiCamReqOutputParams->prvStreamingSensorList.push_back((uint32_t)value);
                    out.pMultiCamReqOutputParams->streamingSensorList.push_back((uint32_t)value);
                    pch = strtok (NULL, delim);
                }

                out.pMultiCamReqOutputParams->prvMasterId = out.pMultiCamReqOutputParams->prvStreamingSensorList[0];
            }
            else // xts
            {
                auto list = out.pMultiCamReqOutputParams->streamingSensorList;
                out.pMultiCamReqOutputParams->prvMasterId = out.pMultiCamReqOutputParams->masterId;
                out.pMultiCamReqOutputParams->prvStreamingSensorList.assign(list.begin(), list.end());
            }
        }
        else if(in.needP2CaptureNode && !in.needFusion &&
           in.bLogicalCaptureOutput)
        {
            // master id is current active id, not get from sensor state manager.
            MY_LOGD("only capture master id(%" PRIu32 ")", prvMasterId);
            updateStreamandMasterId(prvMasterId);
        }
        else if(in.bLogicalRawOutput)
        {
            // master id is current active id, not get from sensor state manager.
            MY_LOGD("[Raw] only capture master id(%" PRIu32 ")", prvMasterId);
            updateStreamandMasterId(prvMasterId);
        }
        else if(in.needP2CaptureNode)
        {
            if(isCShot)
            {
                MY_LOGD("[CShot] only capture master id(%" PRIu32 ")", prvMasterId);
                updateStreamandMasterId(prvMasterId);
            }
            else
            {
                auto iter = mvReqSensorStreamingMap.find(mCurrentActiveFrameNo);
                if(iter != mvReqSensorStreamingMap.end())
                {
                    updateStreamandMasterId(prvMasterId);
                    for(auto&& id:iter->second)
                    {
                        auto invStreamIter =
                        std::find(result.vStreamingSensorIdList.begin(),
                                  result.vStreamingSensorIdList.end(),
                                  id);
                        if(prvMasterId != id && invStreamIter != result.vStreamingSensorIdList.end())
                        {
                            out.pMultiCamReqOutputParams->prvStreamingSensorList.push_back(id);
                        }
                    }
                }
                else
                {
                    MY_LOGE("cannot find current active frame number [%" PRIu32 "]", in.requestNo);
                }
            }
        }
        else if(in.bPhysicalRawOutput)
        {
            auto iter = mvReqSensorStreamingMap.find(mCurrentActiveFrameNo);
            if(iter != mvReqSensorStreamingMap.end())
            {
                out.pMultiCamReqOutputParams->prvMasterId = prvMasterId;
                out.pMultiCamReqOutputParams->prvStreamingSensorList.push_back(prvMasterId);
                for(auto&& id:iter->second)
                {
                    auto invStreamIter =
                    std::find(result.vStreamingSensorIdList.begin(),
                              result.vStreamingSensorIdList.end(),
                              id);
                    if(prvMasterId != id && invStreamIter != result.vStreamingSensorIdList.end())
                    {
                        out.pMultiCamReqOutputParams->prvStreamingSensorList.push_back(id);
                    }
                }
            }
        }
    }

    // if current output sensor is standby, remove it from p2 list.
    for(auto&& id:mPolicyParams.pPipelineStaticInfo->sensorId)
    {
        // 1. If sensor ID not exist in vStreamingSensorIdList, it means
        //    standby or offline.
        // 2. If CShot occured, it has to remove none master id streams.
        if(std::find(
                    out.pMultiCamReqOutputParams->streamingSensorList.begin(),
                    out.pMultiCamReqOutputParams->streamingSensorList.end(),
                    id) == out.pMultiCamReqOutputParams->streamingSensorList.end() ||
          (isCShot && (id != prvMasterId)))
        {
            if(checkPhysicalLsitAndRemoveItemBySensorId(id))
                out.pMultiCamReqOutputParams->markErrorSensorList.push_back(id);
        }
    }
    if(out.pMultiCamReqOutputParams->prvStreamingSensorList.size()>0)
    {
        android::String8 temp("prv streaming: ");
        for(auto&& id:out.pMultiCamReqOutputParams->prvStreamingSensorList)
        {
            temp.appendFormat("%" PRIu32 " ", id);
        }
        MY_LOGD("%s", temp.string());
    }
    if(out.pMultiCamReqOutputParams->markErrorSensorList.size()>0)
    {
        android::String8 temp("mark error: ");
        for(auto&& id:out.pMultiCamReqOutputParams->markErrorSensorList)
        {
            temp.appendFormat("%" PRIu32 " ", id);
        }
        MY_LOGD("%s", temp.string());
    }
    if (out.pMultiCamReqOutputParams->streamingSensorList.size()==0 && out.pMultiCamReqOutputParams->prvStreamingSensorList.size() == 0)
        {
            MY_LOGE("There is no camera in any StreamingSensorList. evaluate fail ");
            return false;
        }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
RequestSensorControlPolicy_Multicam::
dumpSensorControlParam(
    sensorcontrol::SensorControlParamOut &out,
    sensorcontrol::SensorControlParamIn &in,
    requestsensorcontrol::RequestInputParams const& req_in
) -> bool
{
    if(mLogLevel >= 2) {
        if(!mpHal3AObj)
        {
            MY_LOGE("mpHal3AObj instance not exist.");
            return false;
        }
        android::String8 dump_str("");
        dump_str.appendFormat("[Input]\nreq:(%d)\n", req_in.requestNo);
        dump_str.appendFormat("sensor list:\n");
        for(auto&& sensorId : in.vSensorIdList)
        {
            dump_str.appendFormat("%d", sensorId);
        }
        if (in.physicalSensorIdList.size() > 0)
        {
            dump_str.appendFormat("\nphysicalSensor list:\n");
            for(auto&& phy_sensorId : in.physicalSensorIdList)
            {
                dump_str.appendFormat("%d", phy_sensorId);
            }
        }
        dump_str.appendFormat("\nSensor control info:\n");
        for(auto&& item : in.vInfo)
        {
            dump_str.appendFormat("id: (%d)\n", item.first);
            dump_str.appendFormat("iSensorStatus:(%d) bAFDone:(%d) iAEIso:(%d) iAELV_x10:(%d)"
                                  " eSensorStatus:(%d) fSensorFov_H:(%f) fSensorFov_V:(%f)"
                                  " msSensorFullSize:(%dx%d) mActiveArrayRegion(%d,%d, %dx%d)\n",
                                  item.second->iSensorStatus, item.second->bAFDone,
                                  item.second->iAEIso, item.second->iAELV_x10,
                                  item.second->eSensorStatus, item.second->fSensorFov_H,
                                  item.second->fSensorFov_V, item.second->msSensorFullSize.w,
                                  item.second->msSensorFullSize.h, item.second->mActiveArrayRegion.p.x,
                                  item.second->mActiveArrayRegion.p.y, item.second->mActiveArrayRegion.s.w,
                                  item.second->mActiveArrayRegion.s.h);
            dump_str.appendFormat("bSync2ADone(%d)", mpHal3AObj->is2ASyncReady(item.first));
        }
        dump_str.appendFormat("mrCropRegion: (%d,%d, %dx%d)",
                                in.mrCropRegion.p.x,
                                in.mrCropRegion.p.y,
                                in.mrCropRegion.s.w,
                                in.mrCropRegion.s.h);

        dump_str.appendFormat("[Output]\n");
        dump_str.appendFormat("zoom ratio:(%f)", out.fZoomRatio);
        MY_LOGD("%s", dump_str.string());
        for(auto&& item : out.vResult)
        {
            MY_LOGD("id: (%d), iSensorControl:(%d) fAlternavtiveFov_H:(%f) fAlternavtiveFov_V:(%f) mrAlternactiveCropRegion(%d,%d, %dx%d)"
                                  " isMaster(%d) mrAlternactiveCropRegion_Cap(%d,%d, %dx%d) mrAlternactiveCropRegion_3A(%d,%d, %dx%d)\n",
                                  item.first,
                                  item.second->iSensorControl, item.second->fAlternavtiveFov_H,
                                  item.second->fAlternavtiveFov_V, item.second->mrAlternactiveCropRegion.p.x,
                                  item.second->mrAlternactiveCropRegion.p.y, item.second->mrAlternactiveCropRegion.s.w,
                                  item.second->mrAlternactiveCropRegion.s.h, item.second->isMaster,
                                  item.second->mrAlternactiveCropRegion_Cap.p.x, item.second->mrAlternactiveCropRegion_Cap.p.y,
                                  item.second->mrAlternactiveCropRegion_Cap.s.w, item.second->mrAlternactiveCropRegion_Cap.s.h,
                                  item.second->mrAlternactiveCropRegion_3A.p.x, item.second->mrAlternactiveCropRegion_3A.p.y,
                                  item.second->mrAlternactiveCropRegion_3A.s.w, item.second->mrAlternactiveCropRegion_3A.s.h);
        }
    }
    return true;
}
/******************************************************************************
 * Make a function target as a policy - multicam version
 ******************************************************************************/
std::shared_ptr<IRequestSensorControlPolicy>
                makePolicy_RequestSensorControl_Multicam_Zoom(
                    CreationParams const& params)
{
    return std::make_shared<RequestSensorControlPolicy_Multicam>(params);
}
/******************************************************************************
 *
 ******************************************************************************/
SensorControllerBehaviorManager::
SensorControllerBehaviorManager(
    int32_t logLevel
)
{
    // create behavior.
    mvBehaviorList.insert(
                    {BEHAVIOR::Sync3A,
                    std::make_shared<Sync3ABehavior>(logLevel)});
    mvBehaviorList.insert(
                    {BEHAVIOR::FrameSync,
                    std::make_shared<FrameSyncBehavior>(logLevel)});
    mvBehaviorList.insert(
                    {BEHAVIOR::SensorCrop,
                    std::make_shared<SensorCropBehavior>(logLevel)});
    mvBehaviorList.insert(
                    {BEHAVIOR::Streaming,
                    std::make_shared<StreamingBehavior>(logLevel)});
    //
    mLogLevel = logLevel;
}
/******************************************************************************
 *
 ******************************************************************************/
SensorControllerBehaviorManager::
~SensorControllerBehaviorManager()
{
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorControllerBehaviorManager::
update(
    RequestOutputParams& out,
    BehaviorUserData const& userData
)
{
    //if(!userData.isSensorSwitchGoing)
    {
        for(auto&& item:mvBehaviorList)
        {
            auto pBehavior = item.second;
            if(pBehavior != nullptr)
            {
                pBehavior->update(out, userData);
            }
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
Sync3ABehavior::
~Sync3ABehavior()
{
}
/******************************************************************************
 *
 ******************************************************************************/
bool
Sync3ABehavior::
update(
    RequestOutputParams& out,
    BehaviorUserData const& userData
)
{
    // if streaming list less than 2, skip behavior.
    // if SensorSwitchGoing than skip sync3A behavior
    if(userData.parsedResult->vStreamingSensorIdList.size() < 2 ||
       userData.parsedResult->vGotoStandbySensorIdList.size() > 0 ||
       userData.isSensorSwitchGoing == true)
    {
        // for single cam, no need to sync2a.
        // if someone wants to standby, no need to set sync 2A.
        out.pMultiCamReqOutputParams->needSync2A = false;
        out.pMultiCamReqOutputParams->needSyncAf = false;
        return true;
    }
    out.pMultiCamReqOutputParams->needSync2A = true;
    // check sensors support AF or not
    out.pMultiCamReqOutputParams->needSyncAf = false;
    int32_t StreamingAFSensorCount = 0;
    for(auto idx : userData.parsedResult->vStreamingSensorIdList)
    {
        bool isAF = false;
        auto got = mvIsSensorAf.find(idx);

        if(got == mvIsSensorAf.end())
        {
            isAF = StereoSettingProvider::isSensorAF(idx);
            mvIsSensorAf.emplace(idx, isAF);
        }
        else
        {
            isAF = got->second;
        }

        if(isAF == true)
        {
            StreamingAFSensorCount++;
        }
        bool needtoSyncAF = StreamingAFSensorCount >= 2;
        if(needtoSyncAF)
        {
            out.pMultiCamReqOutputParams->needSyncAf = true;
            break;
        }
    }
    if(!userData.enableSyncAf)
    {
        out.pMultiCamReqOutputParams->needSyncAf = false;
    }
    if(userData.isNeedNotifySync3A)
    {
        out.pMultiCamReqOutputParams->need3ASwitchDoneNotify = true;
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
FrameSyncBehavior::
~FrameSyncBehavior()
{
}
/******************************************************************************
 *
 ******************************************************************************/
bool
FrameSyncBehavior::
update(
    RequestOutputParams& out,
    BehaviorUserData const& userData
)
{
    MY_LOGD("isSensorSwitchGoing(%d)", userData.isSensorSwitchGoing);
    bool needSync = false;
    // if streaming list is less than 2, skip this module.
    if(userData.parsedResult->vStreamingSensorIdList.size() >= 2 &&
        userData.isSensorSwitchGoing == false)
    {
        needSync = true;
    }
    out.pMultiCamReqOutputParams->needFramesync = needSync;
    out.pMultiCamReqOutputParams->needSynchelper_3AEnq = false;  // no need 3A enq sync.
    out.pMultiCamReqOutputParams->needSynchelper_Timestamp = needSync;
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
SensorCropBehavior::
~SensorCropBehavior()
{
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorCropBehavior::
update(
    RequestOutputParams& out,
    BehaviorUserData const& userData
)
{
    if(needBuildTransformMatrix)
    {
        auto sensorId = userData.vSensorId;
        auto sensorMode = userData.vSensorMode;
        mDomainConvert =
            std::unique_ptr<DomainConvert,
                            std::function<void(DomainConvert*)> >(
                                new DomainConvert(
                                    sensorId,
                                    sensorMode),
                                [](DomainConvert *p)
                                {
                                    if(p)
                                    {
                                        delete p;
                                    }
                                });
        needBuildTransformMatrix = false;
    }
    updateSensorCrop(out, userData);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorCropBehavior::
updateSensorCrop(
    RequestOutputParams& out,
    BehaviorUserData const& userData
)
{
    MRect sensorCrop;
    std::vector<MINT32> af_region,ae_region,awb_region;
    for(auto&& id:userData.parsedResult->vStreamingSensorIdList)
    {
        if(getSensorCropRect(id, userData.parsedResult->vAlternactiveCropRegion, sensorCrop))
        {
            MRect tgCropRect;
            if(mDomainConvert->convertToTgDomain_simpletrans(id, sensorCrop, tgCropRect)){
                out.pMultiCamReqOutputParams->tgCropRegionList.emplace(id, tgCropRect);
            }
            out.pMultiCamReqOutputParams->sensorScalerCropRegionList.emplace(id, sensorCrop);
        }

        if(getSensorCropRect(id, userData.parsedResult->vAlternactiveCropRegion_3A, sensorCrop))
        {
            MRect tgCropRect;
            if(mDomainConvert->convertToTgDomain_simpletrans(id, sensorCrop, tgCropRect)){
                out.pMultiCamReqOutputParams->tgCropRegionList_3A.emplace(id, tgCropRect);
            }
        }

        if(get3aRegionRect(id, userData.parsedResult->vAlternactiveAfRegion, af_region))
        {
            out.pMultiCamReqOutputParams->sensorAfRegionList.emplace(id, af_region);
        }
        if(get3aRegionRect(id, userData.parsedResult->vAlternactiveAeRegion, ae_region))
        {
            out.pMultiCamReqOutputParams->sensorAeRegionList.emplace(id, ae_region);
        }
        if(get3aRegionRect(id, userData.parsedResult->vAlternactiveAwbRegion, awb_region))
        {
            out.pMultiCamReqOutputParams->sensorAwbRegionList.emplace(id, awb_region);
        }
    }
    for(auto&& item:userData.parsedResult->vAlternactiveCropRegion_Cap)
    {
        MRect tgCropRect;
        if(mDomainConvert->convertToTgDomain_simpletrans(item.first, item.second, tgCropRect)) {
            out.pMultiCamReqOutputParams->tgCropRegionList_Cap.emplace(item.first, tgCropRect);
        }
        out.pMultiCamReqOutputParams->sensorScalerCropRegionList_Cap.emplace(item.first, item.second);
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
bool
SensorCropBehavior::
getSensorCropRect(
    int32_t sensorId,
    std::unordered_map<uint32_t, MRect> const& vCropRegion,
    MRect &cropRect
)
{
    auto iter_sensor_param =
                vCropRegion.find(sensorId);
    auto check_iter_sensor_param =
                (iter_sensor_param != vCropRegion.end());
    if(check_iter_sensor_param)
    {
        cropRect = iter_sensor_param->second;
    }
    else
    {
        MY_LOGE("cannot get sensor crop info in vAlternactiveCropRegion. (%d)",
                                sensorId);
        return false;
    }
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
DomainConvert::
DomainConvert(
    std::vector<int32_t> &sensorIdList,
    SensorMap<uint32_t> &sensorModeList
)
{
    MY_LOGD("build covert matrix");
    buildTransformMatrix(sensorIdList, sensorModeList);
}

/******************************************************************************
 *
 ******************************************************************************/
DomainConvert::
~DomainConvert()
{
    MY_LOGD("release domain convert");
}

/******************************************************************************
 *
 ******************************************************************************/
auto
DomainConvert::
convertToTgDomain(
    int32_t sensorId,
    MRect &srcRect,
    MRect &dstRect
) -> bool
{
    auto iter = mvTransformMatrix.find(sensorId);
    if(iter != mvTransformMatrix.end())
    {
        auto matrix = iter->second;
        matrix.transform(srcRect, dstRect);
    }
    else
    {
        MY_LOGE("cannot get transform matrix");
        return false;
    }
    MY_LOGD_IF(0, "transform done: src(%d, %d, %dx%d) dst(%d, %d, %dx%d)",
            srcRect.p.x, srcRect.p.y, srcRect.s.w, srcRect.s.h,
            dstRect.p.x, dstRect.p.y, dstRect.s.w, dstRect.s.h);
    return true;
}

bool
SensorCropBehavior::
get3aRegionRect(
    int32_t sensorId,
    std::unordered_map<uint32_t, std::vector<MINT32>> const& v3aRegion,
    std::vector<MINT32> &cropRect
)
{
    auto iter_sensor_param =
                v3aRegion.find(sensorId);
    auto check_iter_sensor_param =
                (iter_sensor_param != v3aRegion.end());
    if(check_iter_sensor_param)
    {
        cropRect = iter_sensor_param->second;
    }
    else
    {
        MY_LOGE("cannot get 3a region info in vAlternactiveRegion. (%d)",
                                sensorId);
        return false;
    }
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/

auto
DomainConvert::
convertToTgDomain_simpletrans(
    int32_t sensorId,
    MRect &srcRect,
    MRect &dstRect
) -> bool
{
    auto iter = mvtransActive2Sensor.find(sensorId);
    if(iter != mvtransActive2Sensor.end())
    {
        auto tranActive2Sensor = iter->second;
        dstRect.p = NSCam::v3::transform(tranActive2Sensor,srcRect.p);
        dstRect.s = NSCam::v3::transform(tranActive2Sensor,srcRect.s);
    }
    else
    {
        MY_LOGE("cannot get transform matrix");
        return false;
    }
    MY_LOGD("simpletransform done: src(%d, %d, %dx%d) dst(%d, %d, %dx%d)",
            srcRect.p.x, srcRect.p.y, srcRect.s.w, srcRect.s.h,
            dstRect.p.x, dstRect.p.y, dstRect.s.w, dstRect.s.h);
    return true;
}

auto
DomainConvert::
buildTransformMatrix(
    std::vector<int32_t> &sensorIdList,
    SensorMap<uint32_t> &sensorModeList
) -> bool
{
    auto pLogicalDevice = MAKE_HalLogicalDeviceList();
    if(pLogicalDevice == nullptr){
        MY_LOGE("create LogicalDevice Fail");
        return false;
    }
    for (auto&& sensorId : sensorIdList)
    {
        MRect mActiveArrayRegion;
        NSCam::MSize sensorSize;
        IMetadata sensorMetadata = pLogicalDevice->queryStaticInfo(sensorId);
        if(!IMetadata::getEntry<MRect>(&sensorMetadata,MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION,mActiveArrayRegion))
        {
            MY_LOGE("get MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION fail");
            return false;
        }
        auto sensorMode = sensorModeList.at(sensorId);
        std::shared_ptr<HwInfoHelper> infohelper  = std::make_shared<HwInfoHelper>(sensorId);
        bool ret = (infohelper != nullptr)
             && infohelper->updateInfos()
             && infohelper->getSensorSize(sensorMode,sensorSize);
        if(!ret)
        {
            MY_LOGE("get sensor size fail");
        }
        NSCam::v3::simpleTransform tranActive2Sensor = NSCam::v3::simpleTransform(
            MPoint(0,0),mActiveArrayRegion.s,sensorSize);
        mvtransActive2Sensor.insert({sensorId,tranActive2Sensor});
        MY_LOGD("build simpleTransform. sensor(%d) mode(%d)",
                        sensorId,
                        sensorMode);

        NSCamHW::HwTransHelper helper(sensorId);
        NSCamHW::HwMatrix mat;
        if(!sensorModeList.count(sensorId))
        {
            MY_LOGE("sensorId(%d) not exist in sensorModeList",
                       sensorId);
            continue;
        }
        if(!helper.getMatrixFromActiveRatioAlign(sensorMode, mat))
        {
            MY_LOGE("Get hw matrix failed. sensor id(%d) sensor mode(%d)",
                        sensorId,
                        sensorMode);
            return false;
        }
        mvTransformMatrix.insert({sensorId, mat});
        MY_LOGD("build transform matrix. sensor(%d) mode(%d)",
                        sensorId,
                        sensorMode);
    }
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
StreamingBehavior::
~StreamingBehavior()
{
}
/******************************************************************************
 *
 ******************************************************************************/
bool
StreamingBehavior::
update(
    RequestOutputParams& out,
    BehaviorUserData const& userData
)
{
    if(userData.parsedResult->vStreamingSensorIdList.size() > 1)
    {
        if(preZoomRatio != userData.fZoomRatio)
        {
            count = 0;
            preZoomRatio = userData.fZoomRatio;
        }
        else
        {
            count++;
        }
        if(count < MAX_STREAMING_FEATURE_CONTROL_FRAME_COUNT)
        {
            out.pMultiCamReqOutputParams->isForceOffStreamingNr = false;
        }
        else
        {
            out.pMultiCamReqOutputParams->isForceOffStreamingNr = true;
        }
    }
    else
    {
        count = 0;
        preZoomRatio = userData.fZoomRatio;
    }
    MY_LOGD_IF(mLogLevel > 1, "streaming_count(%zu) count(%d) nrOff(%d) max(%d) preZoom(%f) zoom(%f)",
                userData.parsedResult->vStreamingSensorIdList.size(),
                count,
                out.pMultiCamReqOutputParams->isForceOffStreamingNr,
                (int)MAX_STREAMING_FEATURE_CONTROL_FRAME_COUNT,
                preZoomRatio,
                userData.fZoomRatio);
    return true;
}
};  //namespace requestsensorcontrol
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

