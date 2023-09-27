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

#define LOG_TAG "mtkcam-PipelineModelSessionMultiCam"
//
#include "PipelineModelSessionMultiCam.h"
//
#include "MyUtils.h"
//
#include <camera_custom_stereo.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <kd_imgsensor_define.h>    // camsv sensor setting
#include <bandwidth_control.h>      // bwc
#include <mtkcam/drv/IHwSyncDrv.h>  // hwsync
#include <mtkcam/aaa/IHal3A.h>      // query ae init shutter


#include <mtkcam/utils/hw/IScenarioControlV3.h>

#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h>
//
#include <mtkcam3/pipeline/prerelease/IPreReleaseRequest.h>
//
#include <mtkcam3/pipeline/policy/IRequestSensorControlPolicy.h>
//
#if '1'== MTKCAM_HAVE_CAM_MANAGER
#include <mtkcam/utils/std/common.h>
#include <mtkcam/utils/hw/CamManager.h>
#endif

#include <mtkcam3/pipeline/hwnode/NodeId.h>

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
#endif
#include "../utils/include/impl/PipelineContextBuilder.h"
// function scope
#define __DEBUG
#define __SCOPE_TIMER
#ifdef __DEBUG
#define FUNCTION_SCOPE      auto __scope_logger__ = create_scope_logger(__FUNCTION__)
#include <memory>
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_MULTICAM_PIPELINE_MODEL);
static std::shared_ptr<char> create_scope_logger(const char* functionName)
{
    char* pText = const_cast<char*>(functionName);
    CAM_ULOGMD("[%s] +",pText);
    return std::shared_ptr<char>(pText, [](char* p){ CAM_ULOGMD("[%s] -", p); });
}
#else
#define FUNCTION_SCOPE
#endif // function scope

#define UPDATE_FRAME_DEBUG 0
/******************************************************************************
 *
 ******************************************************************************/
using namespace NSCam::v3::pipeline::model;
using namespace NSCam::v1::Stereo;
using namespace NSCam::v3::pipeline::prerelease;

#define ThisNamespace   PipelineModelSessionMultiCam

/******************************************************************************
 *
 ******************************************************************************/
bool
MultiCamMetadataUtility::
filterResultByResultKey(
    int32_t sensorId,
    const IMetadata& src,
    IMetadata& dst
)
{
    // get static metadata
    auto metadataProvider = NSMetadataProviderManager::valueForByDeviceId(sensorId);
    // get available result key
    auto entry = metadataProvider->getMtkStaticCharacteristics().entryFor(MTK_REQUEST_AVAILABLE_RESULT_KEYS);
    if(!entry.isEmpty())
    {
        // do filter
        for(size_t i=0;i<entry.count();++i)
        {
            auto tag = entry.itemAt(i, Type2Type<MINT32>());
            auto tag_entry = src.entryFor(tag);
            if(!tag_entry.isEmpty())
            {
                dst.update(tag_entry.tag(), tag_entry);
            }
        }
        //manual add
        {
            auto tag_entry = src.entryFor(MTK_SENSOR_ROLLING_SHUTTER_SKEW);
            if(!tag_entry.isEmpty())
            {
                dst.update(tag_entry.tag(), tag_entry);
            }

            /*
             MTK_SCALER_CROP_REGION is no longer a Android meta tag, but still
             need to pass to ZoomRatioConverter.

             ZoomRatioConverter need to use MTK_SCALER_CROP_REGION to update to
             MTK_SCALER_CROP_APP_REGION, and then remove MTK_SCALER_CROP_REGION
             from AppOutMeta.
             */
            auto crop_entry = src.entryFor(MTK_SCALER_CROP_REGION);
            if(!crop_entry.isEmpty())
            {
                dst.update(crop_entry.tag(), crop_entry);
            }
        }
    }
    else
    {
        CAM_ULOGMD("available result key not exist, ignore filter.");
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
MulticamZoomProcessor::
MulticamZoomProcessor(
    std::shared_ptr<policy::pipelinesetting::IPipelineSettingPolicy> pSettingPolicy
) : mpSettingPolicy(pSettingPolicy)
{
    // add callback id list
    mvCallback_id.emplace(SetMasterId, std::vector<uint32_t>());
    mvCallback_id.emplace(SwitchSensorDone, std::vector<uint32_t>());
    mvCallback_id.emplace(IspSwitchState, std::vector<uint32_t>());
    mvCallback_id.emplace(NextCapture, std::vector<uint32_t>());
    mvCallback_id.emplace(AfSearching, std::vector<uint32_t>());
}
/******************************************************************************
 *
 ******************************************************************************/
MulticamZoomProcessor::
~MulticamZoomProcessor()
{
    MY_LOGD("release");
}
/******************************************************************************
 *
 ******************************************************************************/
auto
MulticamZoomProcessor::
sendNotify(
    TYPE type,
    MUINTPTR arg1,
    MUINTPTR arg2,
    MUINTPTR arg3 __unused
) -> bool
{
    bool ret = false;
    if(auto spt = mpSettingPolicy.lock())
    {
        switch(type)
        {
            case SetMasterId:
            {
                auto frameno = arg1;
                auto sensorId = arg2;
                if(!isAlreadyNotify(frameno, type)) {
                    uint32_t type =
                                policy::requestsensorcontrol::NotifyType::SetMasterId;
                    spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                (MUINTPTR)&frameno,
                                (MUINTPTR)&sensorId);
                    {
                        std::lock_guard<std::mutex> lock(mLock);
                        mvCallback_id[SetMasterId].push_back(frameno);
                    }
                }
            }
            break;
            case SetStreamingId:
            {
                auto frameno = arg1;
                auto sensorId = arg2;
                uint32_t type =
                    policy::requestsensorcontrol::NotifyType::SetStreamingId;
                spt->sendPolicyDataCallback(
                    policy::pipelinesetting::PolicyType::SensorControlPolicy,
                    (MUINTPTR)&type,
                    (MUINTPTR)&frameno,
                    (MUINTPTR)&sensorId);
            }
            break;
            case UpdateStreamingMeta:
            {
                auto frameno = arg1;
                auto& streamingIds = arg2;
                uint32_t type =
                    policy::requestsensorcontrol::NotifyType::UpdateStreamingMeta;
                spt->sendPolicyDataCallback(
                    policy::pipelinesetting::PolicyType::SensorControlPolicy,
                    (MUINTPTR)&type,
                    (MUINTPTR)&frameno,
                    streamingIds);
            }
            break;
            case SwitchSensorDone:
            {
                MY_LOGD("send switch done");
                uint32_t type =
                                policy::requestsensorcontrol::NotifyType::SwitchDone;
                spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                0,
                                0);
            }
            break;
            case IspSwitchState:
            {
                auto frameno = arg1;
                auto state = arg2;
                if(!isAlreadyNotify(frameno, type)) {
                    uint32_t type =
                                policy::requestsensorcontrol::NotifyType::IspQualitySwitchState;
                    spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                (MUINTPTR)&frameno,
                                (MUINTPTR)&state);
                    {
                        std::lock_guard<std::mutex> lock(mLock);
                        mvCallback_id[IspSwitchState].push_back(frameno);
                    }
                }
            }
            break;
            case NextCapture:
            {
                auto requestNo = arg1;
                uint32_t type =
                                policy::requestsensorcontrol::NotifyType::NextCapture;
                spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                (MUINTPTR)&requestNo,
                                0);
                {
                    std::lock_guard<std::mutex> lock(mLock);
                    mvCallback_id[NextCapture].push_back(requestNo);
                }
            }
            break;
            case RequestDone:
            {
                auto requestNo = arg1;
                uint32_t type =
                                policy::requestsensorcontrol::NotifyType::RequestDone;
                spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                (MUINTPTR)&requestNo,
                                0);
            }
            break;
            case AfSearching:
            {
                auto requestNo = arg1;
                auto state = arg2;
                uint32_t type = policy::requestsensorcontrol::NotifyType::AfSearching;
                spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                (MUINTPTR)&requestNo,
                                (MUINTPTR)&state);
                {
                    std::lock_guard<std::mutex> lock(mLock);
                    mvCallback_id[AfSearching].push_back(requestNo);
                }
            }
            break;
            case ConfigDone:
            {
                uint32_t type = policy::requestsensorcontrol::NotifyType::ConfigDone;
                spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                0,
                                0);
            }
            break;
            case Flush:
            {
                uint32_t type = policy::requestsensorcontrol::NotifyType::Flush;
                spt->sendPolicyDataCallback(
                                policy::pipelinesetting::PolicyType::SensorControlPolicy,
                                (MUINTPTR)&type,
                                0,
                                0);
            }
            break;
            default:
                break;
        }
        ret = true;
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
MulticamZoomProcessor::
isAlreadyNotify(
    uint32_t frameNo,
    TYPE type
) -> bool
{
    std::lock_guard<std::mutex> lock(mLock);
    bool ret = false;
    auto iter = mvCallback_id.find(type);
    if(iter != mvCallback_id.end()) {
        auto iter_req = std::find(
                            iter->second.begin(),
                            iter->second.end(),
                            frameNo);
        if(iter_req != iter->second.end()) {
            ret = true;
        }
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
MulticamZoomProcessor::
requestFinish(
    uint32_t frameNo
) -> bool
{
    std::lock_guard<std::mutex> lock(mLock);
    for(auto& item:mvCallback_id)
    {
        auto iter_req = std::find(
                            item.second.begin(),
                            item.second.end(),
                            frameNo);
        if(iter_req != item.second.end())
            item.second.erase(iter_req);
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
makeInstance(
    std::string const& name,
    CtorParams const& rCtorParams __unused
) -> android::sp<IPipelineModelSession>
{
    android::sp<ThisNamespace> pSession = new ThisNamespace(name, rCtorParams);
    if  ( CC_UNLIKELY(pSession==nullptr) ) {
        CAM_ULOGME("[%s] Bad pSession", __FUNCTION__);
        return nullptr;
    }

    int const err = pSession->configure();
    if  ( CC_UNLIKELY(err != 0) ) {
        CAM_ULOGME("[%s] err:%d(%s) - Fail on configure()", __FUNCTION__, err, ::strerror(-err));
        return nullptr;
    }

    return pSession;
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
ThisNamespace(
    std::string const& name,
    CtorParams const& rCtorParams)
    : PipelineModelSessionDefault(name, rCtorParams)
{
    auto const& pParsedAppConfiguration = mStaticInfo.pUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    if(pParsedMultiCamInfo->mDualDevicePath == NSCam::v3::pipeline::policy::DualDevicePath::MultiCamControl)
    {
        thermal_policy_name = "thermal_policy_03";
        mDualFeatureConfiguration = std::bind(&ThisNamespace::configure_multicam, this);
        mDualFeatureFrameUpdate = std::bind(
                                        &ThisNamespace::updateFrame_multicam,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3);
        // create processor
        mpMulticamZoomProcessor =
                            std::make_shared<MulticamZoomProcessor>(mPipelineSettingPolicy);
    }
    else
    {
        MY_LOGE("should not happened (%d)", pParsedMultiCamInfo->mDualDevicePath);
    }
    int value = property_get_int32("vendor.debug.camera.dualsession", 0);
    if(value)
        mbShowLog = true;
    else
        mbShowLog = false;
    // create job queue
    mJobQueue = std::make_shared<NSCam::JobQueue<void()>>(LOG_TAG);
    mConfigJobQueue = std::make_shared<NSCam::JobQueue<void()>>(LOG_TAG);
    // create hal3a
    for(auto&& sensorId:mStaticInfo.pPipelineStaticInfo->sensorId)
    {
        auto pHal3a =
            std::shared_ptr<NS3Av3::IHal3A>(
                    MAKE_Hal3A(sensorId, LOG_TAG),
                    [&](auto *p)->void
                    {
                        if(p)
                        {
                            p->destroyInstance(LOG_TAG);
                            p = nullptr;
                        }
                    }
        );
        mvHal3AList.insert({sensorId, pHal3a});
    }
}
/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
~ThisNamespace()
{
    MY_LOGD("reset value");
    // 1. reset stereo setting provider
    // StereoSettingProvider::setStereoFeatureMode(0);
    try{
        if(!StereoSettingProvider::setLogicalDeviceParam({mStaticInfo.pPipelineStaticInfo->openId, 0}))
        {
            MY_LOGE("setLogicalDeviceParam failed!!");
        }
    }
    catch(std::exception &e){
        MY_LOGE("[Exception: %s]", e.what());
    }
    // 2. if sensor sync enable, it has to reset.
    if(SensorSyncType::CALIBRATED == mSensorSyncType)
    {
        bool ret = setSensorSyncToSensorDriver(false);
        if(!ret)
        {
            MY_LOGA("setSensorSyncToSensorDriver failed");
        }
    }
    // 3. reset log
    setLogLevelToEngLoad(0, 0);
    // 4. reset thermal polocy

    // uninit P1Node for BGService's requirement
    IPreReleaseRequestMgr::getInstance()->uninit();

#if '1'== MTKCAM_HAVE_CAM_MANAGER
    if(!thermal_policy_name.empty())
    {
        Utils::CamManager::getInstance()->setThermalPolicy(thermal_policy_name.c_str(), 0);
        //property_set("vendor.thermal.manager.data.dis-policy", thermal_policy_name.c_str());
    }
#endif
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
configure() -> int
{
    if(mDualFeatureConfiguration)
    {
        mDualFeatureConfiguration();
    }

    // call base
    return PipelineModelSessionDefault::configure();
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateBeforeBuildPipelineContext() -> int
{
    auto ret = UNKNOWN_ERROR;
    auto queryAeInitShutter = [](
                                MUINT32 const& openId,
                                uint32_t const& sensorMode,
                                MUINT32 &initShutter)
    {
        auto queryResult = UNKNOWN_ERROR;
        auto mp3A = MAKE_Hal3A(openId, LOG_TAG);

        if(CC_UNLIKELY(!mp3A)){
            return queryResult;
        }

        mp3A->setSensorMode(sensorMode);
        mp3A->send3ACtrl(NS3Av3::E3ACtrl_GetInitExposureTime, reinterpret_cast<MINTPTR>(&initShutter), NULL);

        mp3A->destroyInstance(LOG_TAG);
        return OK;
    };
    auto setHwSyncParam = [](
                            MUINT32 const& dvfsLevel,
                            MUINT32 const& sensorDevId,
                            MUINT32 const& aeInitShutter)
    {
        HWSyncDrv* pHwSync = MAKE_HWSyncDrv();
        if(CC_UNLIKELY(!pHwSync)){
            return UNKNOWN_ERROR;
        }
        MUINT32 ret = pHwSync->sendCommand(
                            HW_SYNC_CMD_ENUM::HW_SYNC_CMD_SET_PARA,
                            sensorDevId,
                            dvfsLevel,      // mmdvfs level
                            aeInitShutter   // init shutter time
        );
        pHwSync->destroyInstance();
        if(ret != 0) return UNKNOWN_ERROR;
        else return OK;
    };
    // set hwsync param, before build pipeline context.
    MUINT32 aeInitShutter = 33000;
    MUINT32 dvfsLevel = 3; // for workaroun, need wait will impl query function in scenario control
    // get open id list
    auto pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(CC_UNLIKELY(pHalDeviceList == nullptr))
    {
        MY_LOGE("pHalDeviceList is null");
        return ret;
    }
    auto openIdList = pHalDeviceList->getSensorId(mStaticInfo.pPipelineStaticInfo->openId);
    if(CC_UNLIKELY(!mConfigInfo2))
    {
        MY_LOGE("mConfigInfo2 is null");
        goto lbExit;
    }
    if(openIdList.size() != mConfigInfo2->mvSensorSetting.size())
    {
        MY_LOGE("something wrong, please check sensor list and sensor setting.");
        goto lbExit;
    }
    for(unsigned long i = 0 ; i < openIdList.size() ; ++i)
    {
        RETURN_ERROR_IF_NOT_OK(
                                queryAeInitShutter(
                                                openIdList[i],
                                                mConfigInfo2->mvSensorSetting.at(openIdList[i]).sensorMode,
                                                aeInitShutter),
                                "query ae init shutter fail");

        MY_LOGD("setup hwsync driver + , openId(%d), sensorDev(%d), dvfsLevel(%d), aeInitShutter(%d)",
            openIdList[i],
            mvSensorDevIdList[i],
            dvfsLevel,
            aeInitShutter
        );
        // 2. set param to hwsync
        RETURN_ERROR_IF_NOT_OK(
                                setHwSyncParam(
                                                dvfsLevel,
                                                mvSensorDevIdList[i],
                                                aeInitShutter
                                                ),
                                "set hwsync param fail");
        MY_LOGD("setup hwsync driver -");
    }
    // call base
    PipelineModelSessionDefault::updateBeforeBuildPipelineContext();
    ret = OK;
lbExit:
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
configure_multicam() -> bool
{
    auto const& pParsedAppConfiguration = mStaticInfo.pUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    auto const& pParsedAppImageStreamInfo = mStaticInfo.pUserConfiguration->pParsedAppImageStreamInfo;
    // 1. get multi-cam feature mode
    mMultiCamFeatureMode = pParsedMultiCamInfo->mDualFeatureMode;
    MINT32 stereoMode = (pParsedMultiCamInfo->mStreamingFeatureMode|
                         pParsedMultiCamInfo->mCaptureFeatureMode);
    // 2. get preview size for multi-cam
    prepareSensorObject();
    MSize preview_size(0, 0);
    evaluatePreviewSize(preview_size);
    MINT32 eisFactor = 100;
    bool bPortraitMode = false;
    if (MTK_MULTI_CAM_FEATURE_MODE_VSDOF == mMultiCamFeatureMode) {
        MINT32 iPreviewMode = -1;
        IMetadata::getEntry<MINT32>(
                        &pParsedAppConfiguration->sessionParams,
                        MTK_VSDOF_FEATURE_PREVIEW_MODE,
                        iPreviewMode);
        bPortraitMode = (iPreviewMode == MTK_VSDOF_FEATURE_PREVIEW_MODE_HALF)?true:false;
        eisFactor =  (mConfigInfo2==nullptr) ? 100 : mConfigInfo2->mStreamingFeatureSetting.eisInfo.factor;
        mStaticInfo.pPipelineStaticInfo->numsP1YUV = StereoSettingProvider::getP1YUVSupported();
        MY_LOGD("portrait(%d) EIS Factor(%d)",bPortraitMode, eisFactor);
    }
    else {
        mStaticInfo.pPipelineStaticInfo->numsP1YUV = NSCam::v1::Stereo::NO_P1_YUV;
        MY_LOGD("Set numsP1YUV to 0 becasue it's not VSDOF");
    }
    StereoSettingProvider::setLogicalDeviceParam({mStaticInfo.pPipelineStaticInfo->openId,
                                                  stereoMode, preview_size,
                                                  bPortraitMode,
                                                  pParsedAppImageStreamInfo->hasVideoConsumer,
                                                  static_cast<MUINT32>(eisFactor)});
    MY_LOGD("stereo mode(%d) preview(%dx%d)",
                                            stereoMode,
                                            preview_size.w,
                                            preview_size.h);

    // 3. check sync related information
    IHalLogicalDeviceList* pHalDeviceList;
    pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(CC_UNLIKELY(pHalDeviceList == nullptr))
    {
        MY_LOGE("pHalDeviceList is null");
        return false;
    }
    mSensorSyncType = pHalDeviceList->getSyncType(mStaticInfo.pPipelineStaticInfo->openId);
    if(SensorSyncType::CALIBRATED == mSensorSyncType)
    {
        bool ret = setSensorSyncToSensorDriver(true);
        if(!ret)
        {
            MY_LOGA("setSensorSyncToSensorDriver faile");
            return false;
        }
    }
    // 4. and set log level
    {
        // update log level (for dual cam case, it will print more log than single cam.)
        setLogLevelToEngLoad(1, 1, 20000);
    }
    // 5. set vsdof thermal policy
#if '1'== MTKCAM_HAVE_CAM_MANAGER
    if(!thermal_policy_name.empty())
    {
        Utils::CamManager::getInstance()->setThermalPolicy(thermal_policy_name.c_str(), 1);
    }
#endif
    // 6. check if needs to use multi-thread to create pipeline.
    if(::usingMultithreadForPipelineContext())
    {
        mbUsingParallelNodeToBuildPipelineContext = true;
    }
    else
    {
        mbUsingParallelNodeToBuildPipelineContext = false;
    }
    return true;
}

/******************************************************************************
 * For zoom
 ******************************************************************************/
auto
ThisNamespace::
updateFrame_multicam(
    MUINT32 const requestNo,
    MINTPTR const userId,
    Result const& result
) -> bool
{
#if 0
    // dump streamid for debug
    android::String8 _debugLog("userId");
    _debugLog.appendFormat("(%#" PRIxPTR "): ", userId);
    for(auto&& appOut:result.vAppOutMeta)
    {
        _debugLog.appendFormat("(%" PRIx64 ") ", appOut->getStreamInfo()->getStreamId());
    }
    MY_LOGI("app out: req(%" PRIu32 ") %s", requestNo, _debugLog.string());
#endif
    if(result.bFrameEnd)
    {
        if(mpMulticamZoomProcessor != nullptr)
            mpMulticamZoomProcessor->requestFinish(requestNo);
    }
    if(((result.vAppOutMeta.size() == 0) &&
       (result.vHalOutMeta.size() == 0)) ||
       (mvRootNodeOpenIdList.size() == 1))
    {
        MY_LOGD("u(%#" PRIxPTR ") r(%d) notify",
                                        userId,
                                        requestNo);
        // frame end will be set to true in this moment.
        PipelineModelSessionDefault::updateFrame(requestNo, userId, result);
        return true;
    }
    auto isInPhysicalAppMetadataMap = [&result](uint32_t &id, StreamId_T streamId) {
        for(auto&& item:result.vPhysicalAppStreamIds) {
            auto iter = std::find(
                                item.second.begin(),
                                item.second.end(),
                                streamId);
                if(iter != item.second.end()) {
                id = item.first;
                return true;
            }
        }
        return false;
    };
    auto cloneNewMetaStreamBuffer = [](
                                uint32_t camid,
                                android::sp<NSCam::v3::IMetaStreamBuffer> metadata) {
        IMetadata filteredMetadata;
        sp<IMetaStreamInfo> pStreamInfo = new NSCam::v3::Utils::MetaStreamInfo(
                                    metadata->getStreamInfo()->getStreamName(),
                                    metadata->getStreamInfo()->getStreamId(),
                                    eSTREAMTYPE_META_OUT,
                                    1);
        auto srcMeta = metadata->tryReadLock(LOG_TAG);
        if (CC_UNLIKELY(srcMeta==nullptr))
        {
            MY_LOGD("going to send srcMeta NULL to filterResultByResultKey");
            return android::sp<NSCam::v3::IMetaStreamBuffer>(nullptr);
        }
        MultiCamMetadataUtility::filterResultByResultKey(
                                camid,
                                *srcMeta,
                                filteredMetadata);
        metadata->unlock(LOG_TAG, srcMeta);
        android::sp<NSCam::v3::IMetaStreamBuffer> filteredMetaStreamBuf =
                    HalMetaStreamBuffer::Allocator(pStreamInfo.get())(filteredMetadata);
        filteredMetaStreamBuf->finishUserSetup();
        filteredMetaStreamBuf->markStatus(metadata->getStatus());
        return filteredMetaStreamBuf;
    };
    auto dumpAllTag = [](IMetadata* buf)
    {
        for(MUINT32 i = 0; i<buf->count(); i++)
        {
            MY_LOGD("tag: (%x)", buf->entryAt(i).tag());
        }
    };
    auto nodeIdToCamId = [this](NodeId_T nodeId)
    {
        auto iter = mvRootNodeOpenIdList.find(nodeId);
        if(iter != mvRootNodeOpenIdList.end())
        {
            return iter->second;
        }
        return -1;
    };
    // get master id
    MINT32 masterCamId = -1;
    if(!getMasterCamId(requestNo, userId, result, masterCamId))
    {
        //callback directly when not contain p1 result
        MY_LOGD("cannot get master cam id");
        Result logicalResult;
        logicalResult.frameNo = result.frameNo;
        logicalResult.sensorTimestamp = result.sensorTimestamp;
        logicalResult.nAppOutMetaLeft = result.nAppOutMetaLeft;
        logicalResult.nHalOutMetaLeft = result.nHalOutMetaLeft;
        logicalResult.vHalOutMeta = result.vHalOutMeta;
        logicalResult.bFrameEnd = result.bFrameEnd;
        logicalResult.nPhysicalID = result.nPhysicalID;
        // physical metadata callback
        std::unordered_map<MINT32, Result> physicalResult;
        for (size_t i = 0; i < result.vAppOutMetaByNodeId.size(); i++)
        {
            for(auto&& metaStreamBuffer:result.vAppOutMetaByNodeId.valueAt(i))
            {
                // check need physical app metadata.
                uint32_t phy_camId = 0;
                if(!!metaStreamBuffer &&
                    isInPhysicalAppMetadataMap(
                            phy_camId,
                            metaStreamBuffer->getStreamInfo()->getStreamId())) {
                    MY_LOGD("[%u] push physical metadata %#" PRIx64 ".",
                                            phy_camId,
                                            metaStreamBuffer->getStreamInfo()->getStreamId());
                    auto *buf = metaStreamBuffer->tryReadLock(LOG_TAG);
                    //dumpAllTag(buf);
                    metaStreamBuffer->unlock(LOG_TAG, buf);
                    // add to vPhysicalOutMeta
                    logicalResult.vPhysicalOutMeta.add(
                                    phy_camId,
                                    cloneNewMetaStreamBuffer(phy_camId, metaStreamBuffer));
                }
                logicalResult.vAppOutMeta.add(metaStreamBuffer);
            }
        }
        PipelineModelSessionDefault::updateFrame(requestNo, userId, logicalResult);
        return true;
    }

    bool isMasterCam = [&](){
        for (size_t i = 0; i < result.vAppOutMetaByNodeId.size(); i++) {
            auto nodeId = result.vAppOutMetaByNodeId.keyAt(i);
            auto camId = nodeIdToCamId(nodeId);
            if (masterCamId == camId) return true;
        }
        return false;
    }();

    // switch sensor flow by master af state
    MUINT8 afState = -1;
    if (isMasterCam) {
        getAfState(requestNo, userId, result, afState);
    }

    // handle isp quality
    handleIspQualityCheck(requestNo, result);
    android::String8 debugLog("");
    // logical metadata callback
    Result logicalResult;
    logicalResult.nAppOutMetaLeft = result.nAppOutMetaLeft;
    logicalResult.nHalOutMetaLeft = result.nHalOutMetaLeft;
    logicalResult.frameNo = result.frameNo;
    logicalResult.bFrameEnd = result.bFrameEnd;
    logicalResult.nPhysicalID = result.nPhysicalID;
    // non-root node metadata
    Result nonRootResult;
    nonRootResult.nAppOutMetaLeft = result.nAppOutMetaLeft;
    nonRootResult.nHalOutMetaLeft = result.nHalOutMetaLeft;
    nonRootResult.frameNo = result.frameNo;
    nonRootResult.bFrameEnd = result.bFrameEnd;
    // physical metadata callback
    std::unordered_map<MINT32, Result> physicalResult;
    for(auto&& rootIdItem : mvRootNodeOpenIdList)
    {
        auto& nodeId = rootIdItem.first;
        auto& camId = rootIdItem.second;
        physicalResult.insert({camId, Result()});
    }
    // 1. seperate metadata by each callback.
    for (size_t i = 0; i < result.vAppOutMetaByNodeId.size(); i++)
    {
        auto nodeId = result.vAppOutMetaByNodeId.keyAt(i);
        auto camId = nodeIdToCamId(nodeId);
        for(auto&& metaStreamBuffer:result.vAppOutMetaByNodeId.valueAt(i))
        {
            if(camId != -1)
            {
                physicalResult[camId].vAppOutMeta.push_back(metaStreamBuffer);
                debugLog.appendFormat("push app metadata %#" PRIx64 " to pid(%d)\n",
                                        metaStreamBuffer->getStreamInfo()->getStreamId(),
                                        camId);
                sendNotify(MulticamZoomProcessor::TYPE::SetStreamingId, requestNo, camId, 0);
            }
            else
            {
                uint32_t phy_camId = 0;
                if(!isInPhysicalAppMetadataMap(
                        phy_camId,
                        metaStreamBuffer->getStreamInfo()->getStreamId()))
                {
                    // if not exist in physical metadata, add to nonRootResult.
                    nonRootResult.vAppOutMeta.push_back(metaStreamBuffer);
                    debugLog.appendFormat("push app metadata %#" PRIx64 " to nonRootResult\n",
                                        metaStreamBuffer->getStreamInfo()->getStreamId());
                }
            }
            // check need physical app metadata.
            uint32_t phy_camId = 0;
            if(!!metaStreamBuffer &&
                isInPhysicalAppMetadataMap(
                        phy_camId,
                        metaStreamBuffer->getStreamInfo()->getStreamId())) {
                debugLog.appendFormat("[%u] push physical metadata %#" PRIx64 ".\n",
                                        phy_camId,
                                        metaStreamBuffer->getStreamInfo()->getStreamId());
                auto *buf = metaStreamBuffer->tryReadLock(LOG_TAG);
                //dumpAllTag(buf);
                metaStreamBuffer->unlock(LOG_TAG, buf);
                // add to vPhysicalOutMeta
                logicalResult.vPhysicalOutMeta.add(
                                phy_camId,
                                cloneNewMetaStreamBuffer(phy_camId, metaStreamBuffer));
            }
        }
    }
    for (size_t i = 0; i < result.vHalOutMetaByNodeId.size(); i++)
    {
        auto nodeId = result.vHalOutMetaByNodeId.keyAt(i);
        auto camId = nodeIdToCamId(nodeId);
        for(auto&& metaStreamBuffer:result.vHalOutMetaByNodeId.valueAt(i))
        {
            if(camId != -1)
            {
                physicalResult[camId].vHalOutMeta.push_back(metaStreamBuffer);
                debugLog.appendFormat("push hal metadata %#" PRIx64 " to pid(%d)\n",
                                        metaStreamBuffer->getStreamInfo()->getStreamId(),
                                        camId);
            }
            else
            {
                nonRootResult.vHalOutMeta.push_back(metaStreamBuffer);
                debugLog.appendFormat("push hal metadata %#" PRIx64 " to nonRootResult\n",
                                        metaStreamBuffer->getStreamInfo()->getStreamId());
            }
        }
    }
    // 2. set logical metadata
    // set logical metadata with nonRootResult and master result
    {
        // 2.1 set nonRootResult.vAppOutMeta
        if (!nonRootResult.vAppOutMeta.isEmpty()) {
          for(auto&& item:nonRootResult.vAppOutMeta)
          {
              if(item == nullptr)
                  return false;
              logicalResult.vAppOutMeta.push_back(item);
              debugLog.appendFormat("push app metadata %#" PRIx64 " to logical(nonRootResult)\n",
                                          item->getStreamInfo()->getStreamId());
          }
        }
        // 2.2 set nonRootResult.vHalOutMeta
        if (!nonRootResult.vHalOutMeta.isEmpty()) {
          for(auto&& item:nonRootResult.vHalOutMeta)
          {
              if(item == nullptr)
                  return false;
              logicalResult.vHalOutMeta.push_back(item);
              debugLog.appendFormat("push hal metadata %#" PRIx64 " to logical(nonRootResult)\n",
                                          item->getStreamInfo()->getStreamId());
          }
        }
        // 2.3 if master id can find, append it.

        if (masterCamId >= 0)
        {
            auto iter = physicalResult.find(masterCamId);
            if(iter != physicalResult.end())
            {
                if(!iter->second.vAppOutMeta.isEmpty()) {
                    for(auto&& item:iter->second.vAppOutMeta)
                    {
                        if(item == nullptr)
                            return false;
                        std::vector<int32_t> vStreamingSensorList;
                        sendNotify(MulticamZoomProcessor::TYPE::UpdateStreamingMeta,
                                    requestNo, reinterpret_cast<MUINTPTR>(&vStreamingSensorList), 0);

                        // fill latest streaming sensors
                        auto *buf = item->tryWriteLock(LOG_TAG);
                        if (buf != nullptr) {
                            IMetadata::IEntry entry(MTK_MULTI_CAM_STREAMING_ID);
                            for (auto const& sensorId : vStreamingSensorList) {
                                entry.push_back(sensorId, Type2Type<MINT32>());
                                debugLog.appendFormat(" logical add sensor(%d) to meta\n", sensorId);
                            }
                            buf->update(entry.tag(), entry);
                            item->unlock(LOG_TAG, buf);
                        } else {
                            MY_LOGW("(master:%d) skip UpdateStreamingMeta", masterCamId);
                        }
                        //
                        logicalResult.vAppOutMeta.push_back(item);
                        debugLog.appendFormat("push app metadata %#" PRIx64 " to logical\n",
                                            item->getStreamInfo()->getStreamId());
                    }
                }
                if(!iter->second.vHalOutMeta.isEmpty()) {
                    for(auto&& item:iter->second.vHalOutMeta)
                    {
                        if(item == nullptr)
                            return false;
                        logicalResult.vHalOutMeta.push_back(item);
                        debugLog.appendFormat("push hal metadata %#" PRIx64 " to logical\n",
                                                item->getStreamInfo()->getStreamId());
                    }
                }
            }
        }
        else
            MY_LOGA("Intent to find physicalResult by negative masterCamId ");
    }
    // 3. callback logical result
    if(logicalResult.vAppOutMeta.size() > 0 ||
       logicalResult.vHalOutMeta.size() > 0 ||
       logicalResult.vPhysicalOutMeta.size() > 0)
    {
        MY_LOGD("app.size(%zu) hal.size(%zu) phy(%zu)",
                    logicalResult.vAppOutMeta.size(),
                    logicalResult.vHalOutMeta.size(),
                    logicalResult.vPhysicalOutMeta.size());
        PipelineModelSessionDefault::updateFrame(requestNo, userId, logicalResult);
        debugLog.appendFormat("updateFrame ");
    }
    // 4. callback less result for zsl
    if(mpZslProcessor.get())
    {
        Result zslResult;
        zslResult.nAppOutMetaLeft = result.nAppOutMetaLeft;
        zslResult.nHalOutMetaLeft = result.nHalOutMetaLeft;
        zslResult.frameNo = result.frameNo;
        zslResult.bFrameEnd = result.bFrameEnd;
        for(auto&& physicalItem : physicalResult)
        {
            if(physicalItem.first != masterCamId)
            {
                for(auto&& item:physicalItem.second.vAppOutMeta)
                {
                    if(item == nullptr)
                        return false;
                    zslResult.vAppOutMeta.push_back(item);
                    debugLog.appendFormat("push app metadata %#" PRIx64 " to zslResult ",
                                        item->getStreamInfo()->getStreamId());
                }
                for(auto&& item:physicalItem.second.vHalOutMeta)
                {
                    if(item == nullptr)
                        return false;
                    zslResult.vHalOutMeta.push_back(item);
                    debugLog.appendFormat("push hal metadata %#" PRIx64 " to zslResult ",
                                        item->getStreamInfo()->getStreamId());
                }
            }
        }
        if(zslResult.vAppOutMeta.size() > 0 ||
           zslResult.vHalOutMeta.size() > 0)
        {
            mpZslProcessor->onFrameUpdated(requestNo, zslResult);
            debugLog.appendFormat("onFrameUpdated ");
        }
    }
    MY_LOGD("u(%#" PRIxPTR ") r(%d) %s",
                                    userId,
                                    requestNo,
                                    debugLog.string());
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
prepareSensorObject() -> bool
{
    IHalSensorList* const pHalSensorList = MAKE_HalSensorList();
    for(auto& sensorId : mStaticInfo.pPipelineStaticInfo->sensorId)
    {
        // 1. build sensor dev id list.
        MUINT32 sensorDevId = pHalSensorList->querySensorDevIdx(
                                sensorId);
        mvSensorDevIdList.push_back(sensorDevId);
        // 2. create sensor hal interface.
        auto sensorHalInterface = pHalSensorList->createSensor(
                                            LOG_TAG,
                                            sensorId);
        mvSensorHalInterface.push_back(sensorHalInterface);
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
setSensorSyncToSensorDriver(bool enable) -> bool
{
    bool ret = true;
    MINT retSensorCmd = 0;
    bool dualCamMode = (enable) ? true : false;
    MUINT32 syncMode = SENSOR_MASTER_SYNC_MODE;
    MY_LOGD("enable(%d)", enable);
    IHalLogicalDeviceList* pHalDeviceList;
    pHalDeviceList = MAKE_HalLogicalDeviceList();
    if(CC_UNLIKELY(pHalDeviceList == nullptr))
    {
        MY_LOGE("pHalDeviceList is null");
        return ret;
    }
    MUINT32 masterDevId =
            pHalDeviceList->getSensorSyncMasterDevId(mStaticInfo.pPipelineStaticInfo->openId);
    if(masterDevId == 0xFF)
    {
        MY_LOGW("cannot support sensor sync");
        goto lbExit;
    }
    if((mvSensorHalInterface.size() < 2) &&
        mvSensorHalInterface.size() != mvSensorDevIdList.size())
    {
        MY_LOGA("mvSensorHalInterface less than 2 / sensorInterface not equal to sensorDevId(%zu:%zu)",
                    mvSensorHalInterface.size(),
                    mvSensorDevIdList.size());
        ret = false;
        goto lbExit;
    }
    // 1. assign master/slave mode to sensor driver.
    for(unsigned long i=0;i<mvSensorHalInterface.size();++i)
    {
        if(enable)
        {
            if(mvSensorDevIdList[i] == masterDevId)
            {
                syncMode = SENSOR_MASTER_SYNC_MODE;
            }
            else
            {
                syncMode = SENSOR_SLAVE_SYNC_MODE;
            }
        }
        else
        {
            syncMode = SENSOR_MASTER_SYNC_MODE;
        }
        retSensorCmd = mvSensorHalInterface[i]->sendCommand(
                                    mvSensorDevIdList[i],
                                    SENSOR_CMD_SET_SENSOR_SYNC_MODE,
                                    (MUINTPTR)&syncMode,
                                    0 /* unused */,
                                    0 /* unused */);
        if(retSensorCmd != 0)
        {
            ret = false;
            MY_LOGA("sendCommand (SENSOR_CMD_SET_SENSOR_SYNC_MODE)(dev:%d) fail", mvSensorDevIdList[i]);
            goto lbExit;
        }
    }
    // 2. set dual cam mode to sensor driver.
    // only set this value to main sensor.
    retSensorCmd = mvSensorHalInterface[0]->sendCommand(
                            mvSensorDevIdList[0],
                            SENSOR_CMD_SET_DUAL_CAM_MODE,
                            (MUINTPTR)&dualCamMode,
                            0 /* unused */,
                            0 /* unused */);
    if(retSensorCmd != 0)
    {
        ret = false;
        MY_LOGA("sendCommand (SENSOR_CMD_SET_DUAL_CAM_MODE) fail");
        goto lbExit;
    }
lbExit:
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
evaluatePreviewSize(MSize &size) -> bool
{
    auto const& pPipelineUserConfiguration = mStaticInfo.pUserConfiguration.get();
    auto const& pParsedAppConfiguration = mStaticInfo.pUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo = mStaticInfo.pUserConfiguration->pParsedAppImageStreamInfo;
    IMetadata::IEntry const entry = pParsedAppConfiguration->sessionParams.entryFor(MTK_VSDOF_FEATURE_PREVIEW_SIZE);
    if(!entry.isEmpty())
    {
        size.w = entry.itemAt(0, Type2Type<MINT32>());
        size.h = entry.itemAt(1, Type2Type<MINT32>());
    }
    else
    {
        auto getPreviewSizeDone = [&](android::sp<IImageStreamInfo> streamInfo){
            if (streamInfo == nullptr) return false;
            int consumer_usage = streamInfo->getUsageForConsumer();
            int allocate_usage = streamInfo->getUsageForAllocator();
            MY_LOGD("consumer : %X, allocate : %X", consumer_usage, allocate_usage);
            // assume: only one preview size.
            // if there more than one YUV stream use GRALLOC_USAGE_HW_TEXTURE usage,
            // it may need AP to send preview size by session parameter.
            if (consumer_usage & GRALLOC_USAGE_HW_TEXTURE) {
                size.w = streamInfo->getImgSize().w;
                size.h = streamInfo->getImgSize().h;
                return true;
            }
            if (consumer_usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                return false;
            }
            size.w = streamInfo->getImgSize().w;
            size.h = streamInfo->getImgSize().h;
            return false;
        };

        for (auto& mapItem : pParsedAppImageStreamInfo->vAppImage_Output_Proc)
        {
            if (getPreviewSizeDone(mapItem.second)) break;
            else continue;
        }

        if (size.w == 0 || size.h == 0) {
            for (auto& mapItem : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical)
            {
                for (auto& streamInfo : mapItem.second) {
                    if (getPreviewSizeDone(streamInfo)) break;
                    else continue;
                }
            }
        }
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getMasterCamId(
    MUINT32 const requestNo,
    MINTPTR const userId,
    Result const& result,
    MINT32 &masterId
) -> bool
{
    bool ret = false;
    for(auto&& metadata:result.vHalOutMeta)
    {
        IMetadata* buf = metadata->tryReadLock(LOG_TAG);
        if(buf != nullptr)
        {
            ret = IMetadata::getEntry<MINT32>(buf, MTK_DUALZOOM_REAL_MASTER, masterId);
            metadata->unlock(LOG_TAG, buf);
            if(ret) // find master cam.
            {
                break;
            }
        }
    }
    if(ret == false)
    {
        MINT32 qtyLevel = -1;
        // workaround: in zoom case, not using synchelper to decide master cam yet.
        for(auto&& rootNodeItem : mvRootNodeOpenIdList)
        {
            auto& nodeId = rootNodeItem.first;
            auto& camId = rootNodeItem.second;
            ssize_t index = result.vHalOutMetaByNodeId.indexOfKey(nodeId);
            if ( index < 0)
                continue;
            auto metaList = result.vHalOutMetaByNodeId.valueAt(index);
            if(metaList.size() == 0) continue;
            IMetadata* buf = metaList[0]->tryReadLock(LOG_TAG);
            if(buf != nullptr && !buf->isEmpty())
            {
                IMetadata::getEntry<MINT32>(buf, MTK_P1NODE_RESIZE_QUALITY_LEVEL, qtyLevel);
                IMetadata::getEntry<MINT32>(buf, MTK_STEREO_SYNC2A_MASTER_SLAVE, masterId);
                if(masterId == -1)
                {
                    masterId = camId;
                    ret = true;
                }
                else if(camId == masterId)
                {
                    ret = true;
                }
                metaList[0]->unlock(LOG_TAG, buf);
                if(ret)
                {
                    MY_LOGD("[%#" PRIxPTR " :%d] result, qtyLevel(%d), masterId(%d)",
                                    userId, requestNo, qtyLevel, masterId);
                    break;
                }
            }
            else
            {
                MY_LOGD("buf(%p) or tag may empty", buf);
            }
        }
    }
    // notify master id to policy.
    if(ret && !result.bIsReprocessFrame)
        sendNotify(MulticamZoomProcessor::TYPE::SetMasterId, requestNo, masterId, 0);
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getAfState(
    MUINT32 const requestNo,
    MINTPTR const userId __unused,
    Result const& result,
    MUINT8& afstate
) -> bool
{
    bool ret = false;
    for (auto&& metadata:result.vAppOutMeta) {
        IMetadata* buf = metadata->tryReadLock(LOG_TAG);
        if (buf != nullptr) {
            if (IMetadata::getEntry<MUINT8>(buf, MTK_CONTROL_AF_STATE, afstate)) {
                ret = true;
            }
            metadata->unlock(LOG_TAG, buf);
            if (ret) break;
        }
    }

    if (ret) {
        sendNotify(MulticamZoomProcessor::TYPE::AfSearching, requestNo, afstate, 0);
    }
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
handleIspQualityCheck(
    MUINT32 const requestNo,
    Result const& result
) -> bool
{
    if(mpMulticamZoomProcessor)
    {
        if(mpMulticamZoomProcessor->isAlreadyNotify(requestNo, MulticamZoomProcessor::TYPE::IspSwitchState))
        {
            return true;
        }
    }
    auto getIspTagValue = [&result] (MUINT32 tag)
    {
        MINT32 value = -1;
        for(auto&& metadata:result.vHalOutMeta)
        {
            IMetadata* buf = metadata->tryReadLock(LOG_TAG);
            if(buf != nullptr)
            {
                auto ret = IMetadata::getEntry<MINT32>(buf, tag, value);
                metadata->unlock(LOG_TAG, buf);
                if(ret)
                {
                    break;
                }
            }
        }
        return value;
    };
    auto resize_quality_status = getIspTagValue(MTK_P1NODE_RESIZE_QUALITY_STATUS);
    if(resize_quality_status == -1) return true;
    if(resize_quality_status == MTK_P1_RESIZE_QUALITY_STATUS_ACCEPT)
    {
        MY_LOGD("[%d] accept", requestNo);
        isIspQualityAccept = true;
    }
    auto resize_quality_switch = getIspTagValue(MTK_P1NODE_RESIZE_QUALITY_SWITCHING);
    MINT32 state = policy::IspSwitchState::Idle;
    MY_LOGD("[%d] resize_quality_status(%d) resize_quality_switch(%d)", requestNo, resize_quality_status, resize_quality_switch);
    if(isIspQualityAccept && !resize_quality_switch)
    {
        // switch done
        MY_LOGD("[%d] done", requestNo);
        state = policy::IspSwitchState::Done;
        isIspQualityAccept = false;
    }
    else if(isIspQualityAccept && resize_quality_switch)
    {
        // switching
        state = policy::IspSwitchState::Switching;
    }
    sendNotify(MulticamZoomProcessor::TYPE::IspSwitchState, requestNo, state, 0);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
doSwitchSensorFlow(
    policy::pipelinesetting::RequestOutputParams const& reqOutput __unused
) -> bool
{
    FUNCTION_SCOPE;
    auto pPipelineContext = getCurrentPipelineContext();
    if(pPipelineContext.get())
    {
        if(reqOutput.multicamReqOutputParams.flushSensorIdList.size() !=
           reqOutput.multicamReqOutputParams.configSensorIdList.size())
        {
            MY_LOGW("flushSensorIdList(%zu) != configSensorIdList(%zu)",
                    reqOutput.multicamReqOutputParams.flushSensorIdList.size(),
                    reqOutput.multicamReqOutputParams.configSensorIdList.size());
        }
        // 1. notify 3a to stop sync2A
        std::vector<uint32_t> streamingList;
        getStreamingSensorId(reqOutput, streamingList);
        ctrl3AOnStreamingSensor(streamingList, NS3Av3::E3ACtrl_SetSync3ADevDoSwitch);
        // 2. process flush list
        flushNode(
                    pPipelineContext,
                    reqOutput.multicamReqOutputParams.flushSensorIdList);
        // 3. call sync2AInit
        setSync2A(reqOutput.multicamReqOutputParams.switchControl_Sync2ASensorList);
        // 4. process config list
        configNode(
                    pPipelineContext,
                    reqOutput.multicamReqOutputParams.configSensorIdList);
        // 5. send notify
        sendNotify(MulticamZoomProcessor::TYPE::SwitchSensorDone, 0, 0, 0);
    }
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
doP1ReConfigFlow(
    policy::pipelinesetting::RequestOutputParams const& reqOutput __unused
) -> bool
{
    FUNCTION_SCOPE;
    //TODO: config p1Node
    // // 4. process config list
    // configNode(
    //             pPipelineContext,
    //             reqOutput.multicamReqOutputParams.configSensorIdList);
    // // 5. send notify
    // sendNotify(MulticamZoomProcessor::TYPE::SwitchSensorDone, 0, 0, 0);
    return true;
}

/******************************************************************************
 * current flush node flow, only for p1node.
 ******************************************************************************/
auto
ThisNamespace::
getP1NodeIdBySensorId(
    uint32_t const& sensorId,
    NodeId_T &nodeId
) -> bool
{
    bool ret = false;
    for(auto&& rootNodeItem:mvRootNodeOpenIdList)
    {
        if((uint32_t)rootNodeItem.second == sensorId)
        {
            nodeId = rootNodeItem.first;
            ret = true;
            break;
        }
    }
    return ret;
}
/******************************************************************************
 * current flush node flow, only for p1node.
 ******************************************************************************/
auto
ThisNamespace::
flushNode(
    sp<IPipelineContext> pPipelineContext,
    std::vector<uint32_t> const& sensorList
) -> bool
{
    bool ret = true;
    for(auto&& sensorId:sensorList)
    {
        NodeId_T nodeId;
        if(!getP1NodeIdBySensorId(sensorId, nodeId))
        {
            continue;
        }
        MERROR err;
        auto pNodeActor = pPipelineContext->queryINodeActor(nodeId);
        {
            if(pNodeActor == nullptr)
            {
                MY_LOGW("get NodeActor(%" PRIdPTR ") fail", nodeId);
                ret &= false;
                goto lbExit;
            }
            IPipelineNode* node = pNodeActor->getNode();
            if( node == nullptr )
            {
                MY_LOGW("get node(%" PRIdPTR ") fail", nodeId);
                ret &= false;
                goto lbExit;
            }
            if( node->flush() != OK )
            {
                MY_LOGW("flush node(%" PRIdPTR ") fail", nodeId);
                ret &= false;
                goto lbExit;
            }
        }
    }
lbExit:
    return ret;
}
/******************************************************************************
 * current kick node flow, only for p1node.
 ******************************************************************************/
auto
ThisNamespace::
kickNode(
    sp<IPipelineContext> pPipelineContext,
    std::vector<uint32_t> const& sensorList
) -> bool
{
    bool ret = true;
    for(auto&& sensorId:sensorList)
    {
        NodeId_T nodeId;
        if(!getP1NodeIdBySensorId(sensorId, nodeId))
        {
            continue;
        }
        MERROR err;
        auto pNodeActor = pPipelineContext->queryINodeActor(nodeId);
        {
            if(pNodeActor == nullptr)
            {
                MY_LOGW("get NodeActor(%" PRIdPTR ") fail", nodeId);
                ret &= false;
                goto lbExit;
            }
            IPipelineNode* node = pNodeActor->getNode();
            if( node == nullptr )
            {
                MY_LOGW("get node(%" PRIdPTR ") fail", nodeId);
                ret &= false;
                goto lbExit;
            }
            if( node->kick() != OK )
            {
                MY_LOGW("kick node(%" PRIdPTR ") fail", nodeId);
                ret &= false;
                goto lbExit;
            }
        }
    }
lbExit:
    return ret;
}

/******************************************************************************
 * current preconfig node flow, only for p1node.
 ******************************************************************************/
auto
ThisNamespace::
preConfigNode(
    sp<IPipelineContext> pPipelineContext,
    std::vector<uint32_t> const& sensorList
) -> bool
{
    bool ret = true;
    for(auto&& sensorId:sensorList)
    {
        NodeId_T nodeId;
        if(!getP1NodeIdBySensorId(sensorId, nodeId))
        {
            continue;
        }
        PreConfigInputParams const in{
            .bEnablePreQueue = true,
            .nodeId = nodeId,
        };

        if(pPipelineContext.get())
        {
            if(preconfigureP1ForPipelineContext(pPipelineContext, in) != OK)
            {
                MY_LOGE("preConfig p1node fail");
                ret = false;
                goto lbExit;
            }
        }
    }
lbExit:
    return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
setSync2A(
    std::vector<uint32_t> const& sensorIdList
) -> bool
{
    if(sensorIdList.size() != 2)
    {
        MY_LOGE("no need to set sync 2a setting");
        return false;
    }
    auto iter = mvHal3AList.find(sensorIdList[0]);
    if(iter != mvHal3AList.end())
    {
        MY_LOGD("set sync2a init m(%d) s(%d)", sensorIdList[0], sensorIdList[1]);
        iter->second->send3ACtrl(
                    NS3Av3::E3ACtrl_T::E3ACtrl_Sync3A_Sync2ASetting,
                     sensorIdList[0], sensorIdList[1]);
    }
    else
    {
        MY_LOGE("cannot get sync3a instance");
    }
    return true;
}
/******************************************************************************
 * current config node flow, only for p1node.
 ******************************************************************************/
auto
ThisNamespace::
configNode(
    sp<IPipelineContext> pPipelineContext,
    std::vector<uint32_t> const& sensorIdList
) -> bool
{
    FUNCTION_SCOPE;
    bool ret = true;
    auto pipelineNodesNeed = mConfigInfo2->mPipelineNodesNeed;
    // reset all p1
    for (auto&& [_sensorId, _needP1] : pipelineNodesNeed.needP1Node)
    {
        _needP1 = false;
    }
    for (auto&& sensorId : sensorIdList)
    {
        NodeId_T nodeId;
        if(!getP1NodeIdBySensorId(sensorId, nodeId))
        {
            continue;
        }
        // find cam id index.
        pipelineNodesNeed.needP1Node.at(sensorId) = true;
    }
    // overrid needed p1node: set specific node to true.
    BuildPipelineContextInputParams const in{
        .pipelineName                   = getSessionName(),
        .pPipelineStaticInfo            = mStaticInfo.pPipelineStaticInfo.get(),
        .pPipelineUserConfiguration     = mStaticInfo.pUserConfiguration.get(),
        .pPipelineUserConfiguration2    = mStaticInfo.pUserConfiguration2.get(),
        .pParsedStreamInfo_NonP1        = &mConfigInfo2->mParsedStreamInfo_NonP1,
        .pParsedStreamInfo_P1           = &mConfigInfo2->mvParsedStreamInfo_P1,
        .pSensorSetting                 = &mConfigInfo2->mvSensorSetting,
        .pvP1HwSetting                  = &mConfigInfo2->mvP1HwSetting,
        .pPipelineNodesNeed             = &pipelineNodesNeed,
        .pPipelineTopology              = &mConfigInfo2->mPipelineTopology,
        .pStreamingFeatureSetting       = &mConfigInfo2->mStreamingFeatureSetting,
        .pCaptureFeatureSetting         = &mConfigInfo2->mCaptureFeatureSetting,
        .pSeamlessSwitchFeatureSetting  = &mConfigInfo2->mSeamlessSwitchFeatureSetting,
        .pBatchSize                     = &mConfigInfo2->mStreamingFeatureSetting.batchSize,
        .pOldPipelineContext            = nullptr,
        .pDataCallback                  = this,
        .bUsingParallelNodeToBuildPipelineContext
                                        = mbUsingParallelNodeToBuildPipelineContext,
        .bUsingMultiThreadToBuildPipelineContext
                                        = false,
        .bIsReconfigure                 = true,
        .bIsSwitchSensor                = true,
    };
    if(reconfigureP1ForPipelineContext(pPipelineContext, in) != OK)
    {
        MY_LOGE("re-config p1node fail");
        ret = false;
        goto lbExit;
    }
    ret = true;
lbExit:
    return ret;
}
/******************************************************************************
 * In sensor switch flow, it has to notify sync3a to stop do sync2A before
 * flush P1Node.
 * And, only notify streaming sensor.
 * How to decide streaming sensor?
 * If one of reqOutput.multicamReqOutputParams.switchControl_Sync2ASensorList element
 * not exist in reqOutput.multicamReqOutputParams.configSensorIdList.
 * This id is streaming.
 ******************************************************************************/
auto
ThisNamespace::
getStreamingSensorId(
    policy::pipelinesetting::RequestOutputParams const& reqOutput __unused,
    std::vector<uint32_t>& sensorIdList
) -> void
{
    /*for(auto&& id:reqOutput.multicamReqOutputParams.switchControl_Sync2ASensorList)
    {
        auto iter = std::find(
                        reqOutput.multicamReqOutputParams.configSensorIdList.begin(),
                        reqOutput.multicamReqOutputParams.configSensorIdList.end(),
                        id);
        if(iter == reqOutput.multicamReqOutputParams.configSensorIdList.end())
        {
            sensorIdList.push_back(id);
        }
    }*/
    for(auto&& id:mStaticInfo.pPipelineStaticInfo->sensorId)
    {
        sensorIdList.push_back((uint32_t)id);
    }
}
/******************************************************************************
 * todo: can set more parameter to 3a?
 ******************************************************************************/
auto
ThisNamespace::
ctrl3AOnStreamingSensor(
    std::vector<uint32_t>& sensorIdList,
    NS3Av3::E3ACtrl_T cmd
) -> bool
{
    for(auto&& id:sensorIdList)
    {
        auto iter = mvHal3AList.find(id);
        if(iter == mvHal3AList.end())
        {
            MY_LOGA("cannot find IHal3A in list(%" PRIu32 "", id);
        }
        if(cmd == NS3Av3::E3ACtrl_SetSync3ADevDoSwitch)
        {
            iter->second->send3ACtrl(cmd, MTRUE, NULL);
            MY_LOGD("send stop sync2a(%" PRIu32 "", id);
        }
    }
    return true;
}
/******************************************************************************
 * current config node flow, only for p1node.
 ******************************************************************************/
auto
ThisNamespace::
sendNotify(
    MulticamZoomProcessor::TYPE type,
    MUINTPTR arg1,
    MUINTPTR arg2,
    MUINTPTR arg3
) -> bool
{
    if(mpMulticamZoomProcessor)
    {
        mpMulticamZoomProcessor->sendNotify(type, arg1, arg2, arg3);
    }
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
handleMarkErrorList(
    std::shared_ptr<policy::pipelinesetting::RequestOutputParams>const& pReqOutputParams,
    std::shared_ptr<ParsedAppRequest>const& pRequest
) -> bool
{
    //get masterslave id list
    auto const& markErrorSensorIdList = pReqOutputParams->multicamReqOutputParams.markErrorSensorList;
    auto const& markErrorStreamList = pReqOutputParams->markErrorStreamList;
    auto const& pParsedAppImageStreamInfo = pRequest->pParsedAppImageStreamInfo;
    auto const& pParsedAppImageStreamBuffers = pRequest->pParsedAppImageStreamBuffers;
    Result result;
    result.frameNo = pRequest->requestNo;
    result.bFrameEnd = false;
    result.nAppOutMetaLeft = 2;
    std::vector<UserOnImageBufferReleased::Result> results;
    for(auto&& sensorId:markErrorSensorIdList)
    {
        MY_LOGD("Mark physical Error buffer. Request:%u flush camera id is:(%u)",
                                            pRequest->requestNo,
                                            sensorId);
        // if not exist in masterSlaveSensorIdList, it means not streaming state.
        // mark error and remove from list.
        auto iter_image_streaminfo = pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.find(sensorId);
        if(iter_image_streaminfo != pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.end())
        {
            // find stream buffer, and mark to Error.
            for(auto&& streamInfo:iter_image_streaminfo->second)
            {
                auto iter_streamBuffer = pParsedAppImageStreamBuffers->vOImageBuffers.find(
                                                                                streamInfo->getStreamId());
                // stream buffer
                if(iter_streamBuffer != pParsedAppImageStreamBuffers->vOImageBuffers.end())
                {
                    iter_streamBuffer->second->finishUserSetup();
                    iter_streamBuffer->second->markStatus(STREAM_BUFFER_STATUS::ERROR);
                }
                // hal buffer
                {
                    results.push_back(UserOnImageBufferReleased::Result{
                        .streamId = streamInfo->getStreamId(),
                        .status = STREAM_BUFFER_STATUS::ERROR,
                        });
                }
            }
            // streaminfo, remove directly.
            pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.erase(iter_image_streaminfo);
        }
        auto iter_raw_streaminfo = pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.find(sensorId);
        if(iter_raw_streaminfo != pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.end())
        {
            // find stream buffer, and mark to Error.
            for(auto&& streamInfo:iter_raw_streaminfo->second)
            {
                auto iter_streamBuffer = pParsedAppImageStreamBuffers->vOImageBuffers.find(
                                                                                streamInfo->getStreamId());
                // stream buffer
                if(iter_streamBuffer != pParsedAppImageStreamBuffers->vOImageBuffers.end())
                {
                    iter_streamBuffer->second->finishUserSetup();
                    iter_streamBuffer->second->markStatus(STREAM_BUFFER_STATUS::ERROR);
                }
                // hal buffer
                {
                    results.push_back(UserOnImageBufferReleased::Result{
                        .streamId = streamInfo->getStreamId(),
                        .status = STREAM_BUFFER_STATUS::ERROR,
                        });
                }
            }
            // streaminfo, remove directly.
            pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.erase(iter_raw_streaminfo);
        }
        // create dummy physical metadata
        {
            auto createDummyMetaStreamBuffer = [](uint32_t camid) {
                IMetadata filteredMetadata;
                std::string streaming_name("dummy:");
                streaming_name += std::to_string(camid);
                StreamId_T appStreamId = eSTREAMID_META_APP_DYNAMIC_PHYSICAL_DUMMY_CAM0 + (StreamId_T)camid;
                sp<IMetaStreamInfo> pStreamInfo = new NSCam::v3::Utils::MetaStreamInfo(
                                            streaming_name.c_str(),
                                            appStreamId,
                                            eSTREAMTYPE_META_OUT,
                                            1);
                android::sp<NSCam::v3::IMetaStreamBuffer> filteredMetaStreamBuf =
                            HalMetaStreamBuffer::Allocator(pStreamInfo.get())(filteredMetadata);
                filteredMetaStreamBuf->finishUserSetup();
                filteredMetaStreamBuf->markStatus(STREAM_BUFFER_STATUS::ERROR);
                return filteredMetaStreamBuf;
            };
            auto metaStreamBuffer = createDummyMetaStreamBuffer(sensorId);
            result.vPhysicalOutMeta.add(sensorId, metaStreamBuffer);
        }
    }
    // physical
    for (auto&& streamId : markErrorStreamList) {
        auto streamBuffer = pParsedAppImageStreamBuffers->vOImageBuffers.find(streamId);
        if (streamBuffer == pParsedAppImageStreamBuffers->vOImageBuffers.end()) break;

        for (auto& Entry : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical){
            auto& vStreamInfo = Entry.second;
            for (auto it = vStreamInfo.begin(); it != vStreamInfo.end(); ++it) {
                if (streamId == (*it)->getStreamId()) {
                    MY_LOGD("Mark physical stream error: Req(%u), streamId(%" PRIi64 ")", pRequest->requestNo, streamId);
                    streamBuffer->second->finishUserSetup();
                    streamBuffer->second->markStatus(STREAM_BUFFER_STATUS::ERROR);
                    // hal buffer
                    {
                        results.push_back(UserOnImageBufferReleased::Result{
                            .streamId = streamId,
                            .status = STREAM_BUFFER_STATUS::ERROR,
                            });
                    }
                    // streaminfo, remove directly.
                    vStreamInfo.erase(it);
                    break;
                }
            }
        }
    }

    // mark depth stream as error
    const bool needDepthStreamMarkError = pReqOutputParams->needDepthStreamMarkError;
    if (needDepthStreamMarkError && pParsedAppImageStreamInfo->pAppImage_Depth.get())
    {
        MY_LOGD("depth stream is only available for P2S currently, mark depth stream as error");
        auto image_streamInfo  = pParsedAppImageStreamInfo->pAppImage_Depth;
        auto iter_streamBuffer = pParsedAppImageStreamBuffers->vOImageBuffers.find(image_streamInfo->getStreamId());
        // stream buffer
        if (iter_streamBuffer != pParsedAppImageStreamBuffers->vOImageBuffers.end())
        {
            MY_LOGD("return depth buffer error");
            iter_streamBuffer->second->finishUserSetup();
            iter_streamBuffer->second->markStatus(STREAM_BUFFER_STATUS::ERROR);
        }
        // hal buffer
        {
            results.push_back(UserOnImageBufferReleased::Result{
                .streamId = image_streamInfo->getStreamId(),
                .status = STREAM_BUFFER_STATUS::ERROR,
                });
        }
        // streaminfo, remove directly.
        pParsedAppImageStreamInfo->pAppImage_Depth.clear();
    }

    // hal buffer requires callback directly to mark error
    if( !results.empty() )
    {
        auto pCallback = mPipelineModelCallback.promote();
        if (CC_UNLIKELY( pCallback == nullptr )) {
            MY_LOGE("Fail to promote IPipelineModelCallback");
            return BAD_VALUE;
        }
        pCallback->onImageBufferReleased(UserOnImageBufferReleased{
            .requestNo = pRequest->requestNo,
            .results = std::move(results),
            });
    }

    updateFrameTimestamp(
                        pRequest->requestNo,
                        0,
                        result,
                        0);
    return true;
}
/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateFrame(
    MUINT32 const requestNo,
    MINTPTR const userId,
    Result const& result
) -> MVOID
{
    if(mDualFeatureFrameUpdate)
    {
        mDualFeatureFrameUpdate(requestNo, userId, result);
    }
    else
    {
        MY_LOGE("mDualFeatureFrameUpdate not set");
        PipelineModelSessionDefault::updateFrame(requestNo, userId, result);
    }
    return;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
beginFlush()->int
{
    std::lock_guard<std::mutex> lock(mFlushLock);
    MY_LOGI("Flush call");
    isFlushing = true;
    sendNotify(MulticamZoomProcessor::TYPE::Flush,0,0,0);
    if(mJobQueue != nullptr)
    {
        mJobQueue->flush();
        mJobQueue->waitJobsDone();
    }
    if(mConfigJobQueue != nullptr)
    {
        mConfigJobQueue->flush();
        mConfigJobQueue->waitJobsDone();
    }
    return PipelineModelSessionDefault::beginFlush();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
endFlush()->void
{
    std::lock_guard<std::mutex> lock(mFlushLock);
    isFlushing = false;
    return PipelineModelSessionDefault::endFlush();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onRequest_Reconfiguration(
    std::shared_ptr<ConfigInfo2>& pConfigInfo2 __unused,
    policy::pipelinesetting::RequestOutputParams const& reqOutput __unused,
    std::shared_ptr<ParsedAppRequest>const& pRequest __unused
) -> int
{
    CAM_TRACE_CALL();
    // check need wake up thread to do re-config flow.
    if(reqOutput.multicamReqOutputParams.flushSensorIdList.size() != 0 ||
       reqOutput.multicamReqOutputParams.configSensorIdList.size())
    {
        std::lock_guard<std::mutex> lock(mFlushLock);
        MY_LOGD("wakeup background thread for reconfig. flushing(%d)", isFlushing);

        if (reqOutput.multicamReqOutputParams.bSwitchNoPreque) {
            if(mJobQueue != nullptr && !isFlushing)
            {
                mJobQueue->addJob(
                    std::bind([](intptr_t arg1,
                                 policy::pipelinesetting::RequestOutputParams const arg2)->void
                    {
                        if(arg1 == 0) return;
                        ThisNamespace* pObj = reinterpret_cast<ThisNamespace*>(arg1);
                        pObj->doSwitchSensorFlow(arg2);
                    },
                    reinterpret_cast<intptr_t>(this),
                    reqOutput));
            }
            mJobQueue->waitJobsDone();
        } else {
            // do pre-config before wakeup thread
            if(!isFlushing){
                preConfigNode(getCurrentPipelineContext(),
                              reqOutput.multicamReqOutputParams.configSensorIdList);
            }
            if(!isFlushing){
            // kick p1node request for quick switch
            kickNode(getCurrentPipelineContext(),
                     reqOutput.multicamReqOutputParams.streamingSensorList);
            }
            if(mJobQueue != nullptr && !isFlushing)
            {
                mJobQueue->addJob(
                    std::bind([](intptr_t arg1,
                                 policy::pipelinesetting::RequestOutputParams const arg2)->void
                    {
                        if(arg1 == 0) return;
                        ThisNamespace* pObj = reinterpret_cast<ThisNamespace*>(arg1);
                        pObj->doSwitchSensorFlow(arg2);
                    },
                    reinterpret_cast<intptr_t>(this),
                    reqOutput));
            }

            if(mConfigJobQueue != nullptr && !isFlushing)
            {
                mConfigJobQueue->addJob(
                    std::bind([](intptr_t arg1,
                                 policy::pipelinesetting::RequestOutputParams const arg2)->void
                    {
                        if(arg1 == 0) return;
                        ThisNamespace* pObj = reinterpret_cast<ThisNamespace*>(arg1);
                        pObj->doP1ReConfigFlow(arg2);
                    },
                    reinterpret_cast<intptr_t>(this),
                    reqOutput));
            }
        }
    }
    PipelineModelSessionDefault::onRequest_Reconfiguration(
                            pConfigInfo2,
                            reqOutput,
                            pRequest);
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onRequest_ProcessEvaluatedFrame(
    std::shared_ptr<policy::pipelinesetting::RequestOutputParams>const& pReqOutputParams __unused,
    std::shared_ptr<ParsedAppRequest>const& pRequest __unused,
    std::shared_ptr<ConfigInfo2>const& pConfigInfo2 __unused
) -> int
{
    CAM_TRACE_CALL();
    handleMarkErrorList(pReqOutputParams, pRequest);
    auto checkRequestSensorId = [&pReqOutputParams](){
        auto const& streamingSensorIdList = pReqOutputParams->multicamReqOutputParams.streamingSensorList;
        auto const& prvStreamingSensorList = pReqOutputParams->multicamReqOutputParams.prvStreamingSensorList;
        if (streamingSensorIdList.size() <= 0 && prvStreamingSensorList.size() <= 0)
        {
            return UNKNOWN_ERROR;
        }
        return OK;
    };
    int req_res = checkRequestSensorId();
    // update crop region
    std::unordered_map<int32_t, MRect> cropRegion;
    for(auto& item:pReqOutputParams->multicamReqOutputParams.tgCropRegionList){
        cropRegion[(int)item.first] = item.second;
        MY_LOGI_IF(mbShowLog > 0, "[requestNo:%u] cam:%d, crop:(%d, %d, %dx%d)",
                                                                pRequest->requestNo,
                                                                (int)item.first,
                                                                item.second.p.x,
                                                                item.second.p.y,
                                                                item.second.s.w,
                                                                item.second.s.h);
    }

    auto pPipelineContext = getCurrentPipelineContext();
    if(pPipelineContext != nullptr)
    {
        auto synchelper = pPipelineContext->getMultiCamSyncHelper();
        if(synchelper != nullptr) {
            synchelper->setSensorCropRegion((int)pRequest->requestNo, cropRegion);
        }
    }
    RETURN_ERROR_IF_NOT_OK( req_res, "[requestNo:%u] checkRequestSensorId", pRequest->requestNo );
    int res = PipelineModelSessionDefault::onRequest_ProcessEvaluatedFrame(
                            pReqOutputParams,
                            pRequest,
                            pConfigInfo2);
    RETURN_ERROR_IF_NOT_OK( res, "[requestNo:%u] onRequest_ProcessEvaluatedFrame", pRequest->requestNo );
    return OK;
}
MVOID
ThisNamespace::
onNextCaptureCallBack(
    MUINT32   requestNo,
    MINTPTR   nodeId __unused,
    MUINT32   requestCnt,
    MBOOL     bSkipCheck
)
{
    sendNotify(MulticamZoomProcessor::TYPE::NextCapture, requestNo, 0, 0);
    PipelineModelSessionDefault::onNextCaptureCallBack(requestNo, nodeId, requestCnt, bSkipCheck);
}

MVOID
ThisNamespace::
onConfigureDone()
{
    sendNotify(MulticamZoomProcessor::TYPE::ConfigDone, 0, 0, 0);
}

MVOID
ThisNamespace::
onRequestDone(MUINT32 requestNo)
{
    sendNotify(MulticamZoomProcessor::TYPE::RequestDone, requestNo, 0, 0);
}

auto
ThisNamespace::
onPipelineFrameDestroy(PipelineFrameDestroy&& arg) -> void
{
    onRequestDone(arg.requestNo);
    PipelineModelSessionBase::onPipelineFrameDestroy(std::move(arg));
}
