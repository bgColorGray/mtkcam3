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
#define LOG_TAG "mtkcam-FeatureSettingPolicy"
//
#include <algorithm>
#include <tuple>
// MTKCAM
#include <mtkcam/utils/cat/CamAutoTestLogger.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/sys/MemoryInfo.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>
// dual cam
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <mtkcam3/feature/vhdr/HDRPolicyHelper.h>
#include <mtkcam3/feature/chdr/CHDRPolicyHelper.h>
#include <isp_tuning.h>
#include <mtkcam/aaa/aaa_hal_common.h>
//
#include "FeatureSettingPolicy.h"
#include <mtkcam/utils/hw/IScenarioControlV3.h>
// debug exif
#include <mtkcam/utils/exif/DebugExifUtils.h>
#include <debug_exif/cam/dbg_cam_param.h>
// aee
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
#endif
//
#include "MyUtils.h"

#define DO_SLAVE_FD_PERIOD (3)

CAM_ULOG_DECLARE_MODULE_ID(MOD_FEATURE_SETTING_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::v3::pipeline::policy::featuresetting;
using namespace NSCam::v3::pipeline::policy::scenariomgr;
using namespace NSCam::v3::pipeline::model;
using namespace NS3Av3; //IHal3A
using namespace NSCamHW; //HwInfoHelper
using namespace NSCam::v3::pipeline::model;
using namespace NSCam::plugin::streaminfo;

using NSCam::TuningUtils::scenariorecorder::ResultParam;
using NSCam::TuningUtils::scenariorecorder::IScenarioRecorder;
using NSCam::TuningUtils::scenariorecorder::DECISION_FEATURE;
using NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)

#define WRITE_SCENARIO_RECORDER_RESULT(reqNo, sensorId, toHeadline, fmt, arg...) \
{                                                                                \
    char log[1024];                                                              \
    if (0 > snprintf(log, sizeof(log), fmt, ##arg)) {                            \
        MY_LOGE("snprintf failed");                                              \
    }                                                                            \
    writeScenarioRecorderResult(reqNo, sensorId, log, toHeadline);               \
}

#define WRITE_SCENARIO_RESULT_DEFAULT(reqNo, sensorId, fmt, arg...) \
WRITE_SCENARIO_RECORDER_RESULT(reqNo, sensorId, false, fmt, ##arg)

#define WRITE_SCENARIO_RESULT_HEADLINE(reqNo, sensorId, fmt, arg...) \
WRITE_SCENARIO_RECORDER_RESULT(reqNo, sensorId, true, fmt, ##arg)

// TODO: rename
#define SENSOR_INDEX_MAIN   (0)
#define SENSOR_INDEX_SUB1   (1)
#define SENSOR_INDEX_SUB2   (2)
// TODO: disable thsi flag before MP
#define DEBUG_FEATURE_SETTING_POLICY  (0)

/******************************************************************************
 *
 ******************************************************************************/
template<typename TPlugin>
class PluginWrapper
{
public:
    using PluginPtr      = typename TPlugin::Ptr;
    using ProviderPtr    = typename TPlugin::IProvider::Ptr;
    using InterfacePtr   = typename TPlugin::IInterface::Ptr;
    using SelectionPtr   = typename TPlugin::Selection::Ptr;
    using Selection      = typename TPlugin::Selection;
    using Property       = typename TPlugin::Property;
    using ProviderPtrs   = std::vector<ProviderPtr>;
public:
    PluginWrapper(const std::string& name, MUINT64 uSupportedFeatures = ~0, MINT32 iUniqueKey = -1, const MBOOL isPhysical = MFALSE);
    ~PluginWrapper();
public:
    auto getName() const -> const std::string&;
    auto isKeyFeatureExisting(MINT64 combinedKeyFeature, MINT64& keyFeature) const -> MBOOL;
    auto tryGetKeyFeature(MINT64 combinedKeyFeature, MINT64& keyFeature, MINT8& keyFeatureIndex) const -> MBOOL;
    auto getProvider(MINT64 combinedKeyFeature, MINT64& keyFeature) -> ProviderPtr;
    auto getProviders() -> ProviderPtrs;
    auto createSelection() const -> SelectionPtr;
    auto offer(Selection& sel) const -> MVOID;
    auto keepSelection(const uint32_t requestNo, ProviderPtr& providerPtr, SelectionPtr& sel) -> MVOID;
    auto pushSelection(const uint32_t requestNo, const uint8_t frameCount) -> MVOID;
    auto cancel() -> MVOID;
private:
    using ProviderPtrMap = std::unordered_map<MUINT64, ProviderPtr>;
    using SelectionPtrMap = std::unordered_map<ProviderPtr, std::vector<SelectionPtr>>;
private:
    const std::string       mName;
    const MINT64            mSupportedFeatures;
    const MINT32            mUniqueKey;
    PluginPtr               mInstancePtr;
    ProviderPtrs            mProviderPtrsSortedByPriorty;
    SelectionPtrMap         mTempSelectionPtrMap;
    InterfacePtr            mInterfacePtr;
    const MBOOL             mIsPhysical;
};

/******************************************************************************
 *
 ******************************************************************************/
template<typename TPlugin>
PluginWrapper<TPlugin>::
PluginWrapper(const std::string& name, MUINT64 uSupportedFeatures, MINT32 uniqueKey, const MBOOL isPhysical)
: mName(name)
, mSupportedFeatures(uSupportedFeatures)
, mUniqueKey(uniqueKey)
, mIsPhysical(isPhysical)
{
    CAM_ULOGM_APILIFE();
    MY_LOGD("ctor:%p, name:%s, supportedFeatures:%#" PRIx64 ", uniqueKey:%d",
        this, mName.c_str(), mSupportedFeatures, mUniqueKey);
    mInstancePtr = TPlugin::getInstance(mUniqueKey);
    if (mInstancePtr) {
        mInterfacePtr = mInstancePtr->getInterface();
        auto& providers = mInstancePtr->getProviders(/*mSupportedFeatures*/);
        mProviderPtrsSortedByPriorty = providers;
        std::sort(mProviderPtrsSortedByPriorty.begin(), mProviderPtrsSortedByPriorty.end(),
            [] (const ProviderPtr& p1, const ProviderPtr& p2) {
                return p1->property().mPriority > p2->property().mPriority;
            }
        );

        for (auto& provider : mProviderPtrsSortedByPriorty) {
            const Property& property = provider->property();
            MY_LOGD("find provider... name:%s, algo(%#" PRIx64"), priority(0x%x)", property.mName, property.mFeatures, property.mPriority);
        }
    }
    else {
        MY_LOGW("cannot get instance for %s features strategy", mName.c_str());
    }
}

template<typename TPlugin>
PluginWrapper<TPlugin>::
~PluginWrapper()
{
    MY_LOGD("dtor:%p name:%s, uniqueKey:%d",
        this, mName.c_str(), mUniqueKey);
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
getName() const -> const std::string&
{
    return mName;
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
isKeyFeatureExisting(MINT64 combinedKeyFeature, MINT64& keyFeature) const -> MBOOL
{
    MINT8  keyFeatureIndex = 0;
    return tryGetKeyFeature(combinedKeyFeature, keyFeature, keyFeatureIndex);
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
tryGetKeyFeature(MINT64 combinedKeyFeature, MINT64& keyFeature, MINT8& keyFeatureIndex) const -> MBOOL
{
    for(MUINT8 i=0; i < mProviderPtrsSortedByPriorty.size(); i++) {
        auto providerPtr = mProviderPtrsSortedByPriorty.at(i);
        keyFeature = providerPtr->property().mFeatures;
        if ((keyFeature & combinedKeyFeature) != 0) {
            keyFeatureIndex = i;
            return MTRUE;
        }
    }

    // if no plugin found, must hint no feature be chose.
    keyFeature = 0;
    keyFeatureIndex = 0;
    return MFALSE;
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
getProvider(MINT64 combinedKeyFeature, MINT64& keyFeature) -> ProviderPtr
{
    MINT8  keyFeatureIndex = 0;
    return tryGetKeyFeature(combinedKeyFeature, keyFeature, keyFeatureIndex) ? mProviderPtrsSortedByPriorty[keyFeatureIndex] : nullptr;
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
getProviders() -> ProviderPtrs
{
    ProviderPtrs ret;
    ret = mProviderPtrsSortedByPriorty;
    return std::move(ret);
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
createSelection() const -> SelectionPtr
{
    return mInstancePtr->createSelection();
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
offer(Selection& sel) const -> MVOID
{
    mInterfacePtr->offer(sel);
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
keepSelection(const uint32_t requestNo, ProviderPtr& providerPtr, SelectionPtr& sel) -> MVOID
{
    if (mTempSelectionPtrMap.find(providerPtr) != mTempSelectionPtrMap.end()) {
        mTempSelectionPtrMap[providerPtr].push_back(sel);
        MY_LOGD("%s: selection size:%zu, requestNo:%u",getName().c_str(), mTempSelectionPtrMap[providerPtr].size(), requestNo);
    }
    else {
        std::vector<SelectionPtr> vSelection;
        vSelection.push_back(sel);
        mTempSelectionPtrMap[providerPtr] = vSelection;
        MY_LOGD("%s: new selection size:%zu, requestNo:%u", getName().c_str(), mTempSelectionPtrMap[providerPtr].size(), requestNo);
    }
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
pushSelection(const uint32_t requestNo, const uint8_t frameCount) -> MVOID
{
    for (auto item : mTempSelectionPtrMap) {
        auto providerPtr = item.first;
        auto vSelection  = item.second;
        MY_LOGD("%s: selection size:%zu, frameCount:%d", getName().c_str(), vSelection.size(), frameCount);
        if (frameCount > 1 && vSelection.size() == 1) {
            auto sel = vSelection.front();
            for (size_t i = 0; i < frameCount; i++) {
                MY_LOGD("%s: duplicate selection for multiframe(count:%d, index:%zu)", getName().c_str(), frameCount, i);

                auto pSelection = std::make_shared<Selection>(*sel);
                pSelection->mTokenPtr = Selection::createToken(mUniqueKey, requestNo, i, pSelection->mSensorId, mIsPhysical);
                mInstancePtr->pushSelection(providerPtr, pSelection);
            }
        }
        else {
            for (auto sel : vSelection) {
                mInstancePtr->pushSelection(providerPtr, sel);
            }
        }
        vSelection.clear();
    }
    mTempSelectionPtrMap.clear();
}

template<typename TPlugin>
auto
PluginWrapper<TPlugin>::
cancel() -> MVOID
{
    for (auto item : mTempSelectionPtrMap) {
        auto providerPtr = item.first;
        auto vSelection  = item.second;
        if (providerPtr.get()) {
            //providerPtr->cancel();
        }
        MY_LOGD("%s: selection size:%zu", getName().c_str(), vSelection.size());
        vSelection.clear();
    }
    mTempSelectionPtrMap.clear();
}

#define DEFINE_PLUGINWRAPER(CLASSNAME, PLUGINNAME)                                                                      \
class FeatureSettingPolicy::CLASSNAME final: public PluginWrapper<NSCam::NSPipelinePlugin::PLUGINNAME>                  \
{                                                                                                                       \
public:                                                                                                                 \
    /*Feature Provider*/                                                                                                \
    CLASSNAME(MUINT64 uSupportedFeatures, MINT32 iUniqueKey, const MBOOL isPhysical)                                                            \
    : PluginWrapper<NSCam::NSPipelinePlugin::PLUGINNAME>(#PLUGINNAME, uSupportedFeatures, iUniqueKey, isPhysical)       \
    {                                                                                                                   \
    }                                                                                                                   \
}
DEFINE_PLUGINWRAPER(MFPPluginWrapper, MultiFramePlugin);
DEFINE_PLUGINWRAPER(RawPluginWrapper, RawPlugin);
DEFINE_PLUGINWRAPER(YuvPluginWrapper, YuvPlugin);
DEFINE_PLUGINWRAPER(BokehPluginWraper, BokehPlugin);
DEFINE_PLUGINWRAPER(DepthPluginWraper, DepthPlugin);
DEFINE_PLUGINWRAPER(FusionPluginWraper, FusionPlugin);
#undef DEFINE_PLUGINWRAPER

/******************************************************************************
 *
 ******************************************************************************/
FeatureSettingPolicy::
FeatureSettingPolicy(
    CreationParams const& params
)
    :mPolicyParams(params)
{
    CAM_ULOGM_APILIFE();
    MY_LOGI("(%p) ctor, openId(%d), sensors_count(%zu), is_dual_cam(%d)",
            this,
            mPolicyParams.pPipelineStaticInfo->openId,
            mPolicyParams.pPipelineStaticInfo->sensorId.size(),
            mPolicyParams.pPipelineStaticInfo->isDualDevice);
    mbDebug = ::property_get_int32("vendor.debug.camera.featuresetting.log", DEBUG_FEATURE_SETTING_POLICY);
    // forced feature strategy for debug
    mForcedKeyFeatures = ::property_get_int64("vendor.debug.featuresetting.keyfeature", -1);
    mForcedFeatureCombination = ::property_get_int64("vendor.debug.featuresetting.featurecombination", -1);

    mForcedIspTuningHint = ::property_get_int32("vendor.debug.featuresetting.isptuninghint", -1);

    // multicam mode flag init
    auto& pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    if(pParsedMultiCamInfo != nullptr) {
        mDualDevicePath = pParsedMultiCamInfo->mDualDevicePath;
        miMultiCamFeatureMode = pParsedMultiCamInfo->mDualFeatureMode;
        // single flow:
        // 1. dual device && dual device not equal to multicam.
        // 2. not dual device
        bool bUsingSingleFlow = ((mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo->mSupportPhysicalOutput == false)
                                && (miMultiCamFeatureMode == -1));

        if (bUsingSingleFlow) {
            MY_LOGD("using single flow");
        }
        else {
            // check need sync af
            MY_LOGD("multicam flow. feature mode(%d)", miMultiCamFeatureMode);
            mMultiCamStreamingUpdater =
                            std::bind(
                                &FeatureSettingPolicy::updateMultiCamStreamingData,
                                this,
                                std::placeholders::_1,
                                std::placeholders::_2);
        }
    }

    // create Hal3A for 3A info query
    mHal3a.clear();
    for (auto& id : mPolicyParams.pPipelineStaticInfo->sensorId)
    {
        std::shared_ptr<IHal3A> hal3a
        (
            MAKE_Hal3A(id, LOG_TAG),
            [](IHal3A* p){ if(p) p->destroyInstance(LOG_TAG); }
        );
        if (hal3a.get()) {
            mHal3a.emplace(id, hal3a);
        }
        else {
            MY_LOGE("sensorId(%d) MAKE_Hal3A failed, it nullptr", id);
        }
    }

    // for HDR
    mHDRHelper = createHDRPolicyHelper(
        mPolicyParams.pPipelineStaticInfo->sensorId);
    mCHDRHelper = createCHDRPolicyHelper(mHDRHelper);

    mScEnable = IScenarioRecorder::getInstance()->isScenarioRecorderOn();
}

FeatureSettingPolicy::
~FeatureSettingPolicy()
{
    MY_LOGI("(%p) dtor", this);
}

auto
FeatureSettingPolicy::
decideReqMasterAndSubSensorlist(
    RequestInputParams const* in,
    CreationParams &policyParams,
    bool const isCaptureUpdate
) -> void
{
    if(in->pMultiCamReqOutputParams != nullptr)
    {
        String8 s("");
        mReqSubSensorIdList.clear();

        if(policyParams.bIsLogicalCam)
        {
            const std::vector<uint32_t>* pIdList;
            if(isCaptureUpdate)
            {
                mReqMasterId = in->pMultiCamReqOutputParams->prvMasterId;
                pIdList = &in->pMultiCamReqOutputParams->prvStreamingSensorList;
            }
            else
            {
                mReqMasterId = in->pMultiCamReqOutputParams->masterId;
                pIdList = &in->pMultiCamReqOutputParams->streamingSensorList;
            }

            for (const auto& id : *pIdList)
            {
                if(id != mReqMasterId) {
                    mReqSubSensorIdList.push_back(id);
                }
            }

            s.appendFormat("Logical m(%d)", mReqMasterId);
            for (const auto& sensorId : mReqSubSensorIdList)
            {
                s.appendFormat(" s(%d)", sensorId);
            }
        }
        else
        {
            mReqMasterId = policyParams.pPipelineStaticInfo->openId;
            s.appendFormat("Physical m(%d)", mReqMasterId);
        }

        MY_LOGI_IF(isCaptureUpdate, "%s", s.string());
    }
    else
    {
        bool isManuallyAddedDevice = true;

        for(auto& id:policyParams.pPipelineStaticInfo->sensorId)
        {
            if(policyParams.pPipelineStaticInfo->openId == id) {
                // if sensor id is same to open id, it means physical sensor device.
                // (In current added device rule, physical sensor id will same to
                // device Id.)
                isManuallyAddedDevice &= false;
            }
        }

        if(isManuallyAddedDevice)
        {
            mReqMasterId = policyParams.pPipelineStaticInfo->sensorId[0];
        }
        else
        {
            mReqMasterId = policyParams.pPipelineStaticInfo->openId;
        }
    }
}

auto
FeatureSettingPolicy::
collectDefaultSensorInfo(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool
{
    //CAM_ULOGM_APILIFE();
    bool ret = true;

    // keep first request config as default setting (ex: defualt sensor mode).
    if (mDefaultConfig.bInit == false) {
        MY_LOGI("keep the first request config as default config");
        mDefaultConfig.sensorMode = in->sensorMode;
        mDefaultConfig.bInit = true;
    }
    // use the default setting, features will update it later.
    out->sensorMode = mDefaultConfig.sensorMode;

    return ret;
}

auto
FeatureSettingPolicy::
collectHDRCaptureInfo() -> void {
    for (auto& it : mCHDRHelper) {
        auto sensorId = it.first;
        auto& chdrHelper = it.second;
        auto& hdrHelper = mHDRHelper.at(sensorId);
        chdrHelper->updateInfo(hdrHelper);
    }
}

auto
FeatureSettingPolicy::
collectParsedStrategyCaptureInfo(
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in
) -> bool
{
    //CAM_ULOGM_APILIFE();
    bool ret = true;
    // collect parsed info for capture feature strategy
    if (CC_UNLIKELY(in->pRequest_ParsedAppMetaControl == nullptr)) {
        MY_LOGW("cannot get ParsedMetaControl, invalid nullptr");
    }
    else {
        parsedInfo.isZslModeOn = mConfigOutputParams.isZslMode;

#if MTKCAM_NO_SUPPORT_ZSL_RAW
        if (in->pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0) {
            parsedInfo.isZslFlowOn = false;
            MY_LOGD("Not support ZSL RAW because hardware limitation !");
        }
        else {
            parsedInfo.isZslFlowOn = in->pRequest_ParsedAppMetaControl->control_enableZsl
                && in->pRequest_ParsedAppMetaControl->control_captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        }
#else
        parsedInfo.isZslFlowOn = in->pRequest_ParsedAppMetaControl->control_enableZsl
            && in->pRequest_ParsedAppMetaControl->control_captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
#endif
    }
    // obtain latest real iso information for capture strategy
    {
        CaptureParam_T captureParam;

        auto hal3a = mHal3a.at(mReqMasterId);
        if (hal3a.get()) {
            std::lock_guard<std::mutex> _l(mHal3aLocker);
            ExpSettingParam_T expParam;
            hal3a->send3ACtrl(E3ACtrl_GetExposureInfo,  (MINTPTR)&expParam, 0);
            hal3a->send3ACtrl(E3ACtrl_GetExposureParam, (MINTPTR)&captureParam, 0);
        }
        else {
            MY_LOGE("create IHal3A instance failed! cannot get current real iso for strategy");
            ::memset(&captureParam, 0, sizeof(CaptureParam_T));
            ret = false;
        }
        parsedInfo.realIso = captureParam.u4RealISO;
        parsedInfo.exposureTime = captureParam.u4Eposuretime; //us

        // query flash status from Hal3A
        int isFlashOn = 0;
        hal3a->send3ACtrl(NS3Av3::E3ACtrl_GetIsFlashOnCapture, (MINTPTR)&isFlashOn, 0);
        if (isFlashOn) {
            parsedInfo.isFlashOn = true;
        }
    }
    // get info from AppControl Metadata
    {
        auto pAppMetaControl = in->pRequest_AppControl;
        //
        MUINT8 aeLock = MTK_CONTROL_AE_LOCK_OFF;
        MINT32 evSetting = 0;
        MINT32 manualIso = 0;
        MINT64 manualExposureTime = 0;
        if (IMetadata::getEntry<MUINT8>(pAppMetaControl, MTK_CONTROL_AE_MODE, parsedInfo.aeMode)) {
            MY_LOGD("App set AE mode = %d", parsedInfo.aeMode);
        }
        if (IMetadata::getEntry<MUINT8>(pAppMetaControl, MTK_CONTROL_AE_LOCK, aeLock)) {
            MY_LOGD("App set AE lock = %d", aeLock);
        }
        if (parsedInfo.aeMode  == MTK_CONTROL_AE_MODE_OFF) {
            MY_LOGI("it is manual AE (mode:%d, lock:%d)", parsedInfo.aeMode, aeLock);
            parsedInfo.isAppManual3A = MTRUE;
            if (IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_SENSOR_SENSITIVITY, manualIso) && manualIso > 0) {
                parsedInfo.realIso = manualIso;
                MY_LOGD("App set ISO = %d", parsedInfo.realIso);
            }
            if (IMetadata::getEntry<MINT64>(pAppMetaControl, MTK_SENSOR_EXPOSURE_TIME, manualExposureTime) && manualExposureTime > 0) {
                parsedInfo.exposureTime = manualExposureTime/1000; //ns to us
                MY_LOGD("App set shutter = %d", parsedInfo.exposureTime);
            }
            if (IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_CONTROL_AE_EXPOSURE_COMPENSATION, evSetting)) {
                parsedInfo.evSetting = evSetting;
                MY_LOGD("App set EV = %d", parsedInfo.evSetting);
            }
        }
        else if (aeLock == MTK_CONTROL_AE_LOCK_ON) {
            // AE lock only, but not AE mode off.
            // the 3A setting (iso, shutter) maybe lock during preview
            if (IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_CONTROL_AE_EXPOSURE_COMPENSATION, evSetting)) {
                parsedInfo.isAppManual3A = MTRUE;
                parsedInfo.evSetting = evSetting;
                MY_LOGI("It is AE lock(mode:%d, lock:%d) and App set EV = %d",parsedInfo.aeMode, aeLock, parsedInfo.evSetting);
            }
        }
        // check for debug
        if (parsedInfo.isZslFlowOn && parsedInfo.isAppManual3A) {
            MY_LOGW("unknow behavior: App request manual AE and ZSL at the same time.");
        }
        if (!IMetadata::getEntry<MUINT8>(pAppMetaControl, MTK_FLASH_MODE, parsedInfo.flashMode)) {
            MY_LOGD("get metadata MTK_FLASH_MODE failed! cannot get current flash mode for strategy");
        }
        MY_LOGD("App request aeMode:%d, flashMode:%d", parsedInfo.aeMode, parsedInfo.flashMode);
        if (parsedInfo.aeMode  == MTK_CONTROL_AE_MODE_ON_ALWAYS_FLASH ||
            (parsedInfo.aeMode == MTK_CONTROL_AE_MODE_ON && parsedInfo.flashMode == MTK_FLASH_MODE_TORCH)) {
            parsedInfo.isFlashOn = true;
        }
        MINT32 cshot = 0;
        MUINT8 captureIntent = MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        if(!IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_CSHOT_FEATURE_CAPTURE, cshot)) {
            MY_LOGD_IF(2 <= mbDebug, "cannot get MTK_CSHOT_FEATURE_CAPTURE");
        }
        if(!IMetadata::getEntry<MUINT8>(pAppMetaControl, MTK_CONTROL_CAPTURE_INTENT, captureIntent)){
            MY_LOGD_IF(2 <= mbDebug, "cannot get MTK_CONTROL_CAPTURE_INTENT");
        }
        parsedInfo.captureIntent = captureIntent;
        MBOOL bFastCapture = !(captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE || captureIntent == MTK_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT || captureIntent == MTK_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG);
        MY_LOGD("cshot %d, captureIntent %d, bFastCapture %d", cshot, captureIntent, bFastCapture);
        if (cshot || bFastCapture) {
            parsedInfo.isCShot       = true;
            parsedInfo.isFastCapture = bFastCapture;
        }

        //TODO: temp solution, Edward should fix
        if(in->needRawOutput) {
            parsedInfo.isCShot       = false;
            parsedInfo.isFastCapture = false;
            MY_LOGD("need RAW output, will not be treated as cshot");
        }

        // app request frame for isp hidl flow usage
        MINT32 isp_tuning_hint = -1;
        if (in->pRequest_AppImageStreamInfo->pAppImage_Output_IspTuningData_Yuv || in->pRequest_AppImageStreamInfo->pAppImage_Output_IspTuningData_Raw
            || (in->pRequest_AppControl != nullptr && in->pRequest_ParsedAppMetaControl->control_isp_tuning != 0)) {
            if (IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, isp_tuning_hint)) {
                MY_LOGD("get metadata MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING:%d", isp_tuning_hint);
                parsedInfo.ispTuningHint = isp_tuning_hint;
            }
            else {
                parsedInfo.ispTuningHint = MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_DEFAULT_NONE;
                MY_LOGD("App request IspTuningData_Yuv w/o MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING (set as default:%d)", parsedInfo.ispTuningHint);
            }
        }
        if (mForcedIspTuningHint >= 0) {
            parsedInfo.ispTuningHint = mForcedIspTuningHint;
            MY_LOGI("force ispTuningHint(%d) for feature setting policy", parsedInfo.ispTuningHint);
        }
        // app hint to request frame count/index for multiframe reprocessing
        parsedInfo.ispTuningFrameCount = -1;
        parsedInfo.ispTuningFrameIndex = -1;
        if (IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_COUNT, parsedInfo.ispTuningFrameCount)) {
            MY_LOGD("get metadata MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_COUNT:%d", parsedInfo.ispTuningFrameCount);
        }
        if (IMetadata::getEntry<MINT32>(pAppMetaControl, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX, parsedInfo.ispTuningFrameIndex)) {
            MY_LOGD("get metadata MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX:%d", parsedInfo.ispTuningFrameIndex);
        }

        if (parsedInfo.ispTuningFrameCount > 0) {
            if (parsedInfo.ispTuningFrameIndex == 0){
                parsedInfo.ispTuningOutputFirstFrame = true;
                MY_LOGD("It is first frame to output for app due to app set frame hint(count:%d, index:%d)",
                        parsedInfo. ispTuningFrameCount, parsedInfo.ispTuningFrameIndex);
            }
            if (parsedInfo.ispTuningFrameIndex+1 == parsedInfo.ispTuningFrameCount){
                parsedInfo.ispTuningOutputLastFrame = true;
                MY_LOGD("It is last frame to output for app due to app set frame hint(count:%d, index:%d)",
                        parsedInfo. ispTuningFrameCount, parsedInfo.ispTuningFrameIndex);
            }
        }
    }
    // check
    if (in->pRequest_AppImageStreamInfo->pAppImage_Input_Yuv) {
        parsedInfo.isYuvReprocess = true;
    }
    if (in->pRequest_AppImageStreamInfo->pAppImage_Input_Priv || in->pRequest_AppImageStreamInfo->pAppImage_Input_RAW16) {
        parsedInfo.isRawReprocess = true;
    }
    if (parsedInfo.ispTuningHint >= 0) {
        if (in->needRawOutput) {
            // is request output RAW16
            parsedInfo.isRawRequestForIspHidl = true;
        }
        if (in->needP2CaptureNode) {
            // is request output cpature IQ yuv
            parsedInfo.isYuvRequestForIspHidl = true;
        }
    }

    auto isManualExp = [] (void) -> bool
    {
        // TODO: add manual ae condition and return false here
        if( ::property_get_int32("vendor.debug.hdr.manualexp", 0) ) {
            return true;
        }
        return false;
    };
    if (isManualExp())
    {
        if (!mHDRHelper.at(mReqMasterId)->notifyManualExp()) {
             MY_LOGE("HDRPolciyHelper(%d) notifyManualExp failed", mReqMasterId);
        }
        if (in->pRequest_AppImageStreamInfo->previewStreamId != -1) {
            out->skipP2StreamList.push_back(in->pRequest_AppImageStreamInfo->previewStreamId);
        }
    }
    //after doing capture, vhdr need to add dummy frame
    if (!mHDRHelper.at(mReqMasterId)->notifyDummy()) {
        MY_LOGE("HDRPolciyHelper(%d) notifyDummy failed", mReqMasterId);
    }

    parsedInfo.freeMemoryMBytes = (NSCam::NSMemoryInfo::getFreeMemorySize()/1024/1024); // convert Bytes to MB

    // for HDR
    collectHDRCaptureInfo();

    MY_LOGD("strategy info for capture feature(isZsl(mode:%d, flow:%d), (isCShot/isFastCapture:%d/%d), isFlashOn:%d, iso:%d, shutterTimeUs:%d), reprocess(raw:%d, yuv:%d), freeMem:%dMB, request_for_isp_hidl(raw:%d, yuv:%d), isManualExp(%d)",
            parsedInfo.isZslModeOn, parsedInfo.isZslFlowOn, parsedInfo.isCShot, parsedInfo.isFastCapture, parsedInfo.isFlashOn, parsedInfo.realIso, parsedInfo.exposureTime,
            parsedInfo.isRawReprocess, parsedInfo.isYuvReprocess, parsedInfo.freeMemoryMBytes, parsedInfo.isRawRequestForIspHidl, parsedInfo.isYuvRequestForIspHidl, isManualExp());
    return ret;
}

auto
FeatureSettingPolicy::
updateCaptureDebugInfo(
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in __unused
) -> bool
{

    if (out->mainFrame.get() == nullptr) {
        MY_LOGE("no mainFrame to update debug exif info, cannot update debug exif");
        return false;
    }

    // collect all frames' Hal Metadata for debug exif updating
    // main frame
    std::vector<MetadataPtr> vHalMeta;
    vHalMeta.push_back(out->mainFrame->additionalHal.at(mReqMasterId));
    // subframes
    for (size_t i = 0; i < out->subFrames.size(); i++) {
        auto subFrame = out->subFrames[i];
        if (subFrame.get()) {
            vHalMeta.push_back(subFrame->additionalHal.at(mReqMasterId));
        }
    }

    for (size_t i = 0; i < vHalMeta.size(); i++) {
        auto pHalMeta = vHalMeta[i];
        if (CC_UNLIKELY(pHalMeta.get() == nullptr)) {
            MY_LOGE("pHalMeta[%zu] is invalid nullptr!!", i);
            return false;
        }
        IMetadata::setEntry<MINT32>(pHalMeta.get(), MTK_FEATURE_FREE_MEMORY_MBYTE, parsedInfo.freeMemoryMBytes);
    }

    return true;
}

auto
FeatureSettingPolicy::
getCaptureP1DmaConfig(
    uint32_t &needP1Dma,
    RequestInputParams const* in,
    uint32_t sensorId,
    bool needRrzoBuffer //TODO: modify default value to false after ZSL support partial P1 buffer request
) -> bool
{
    bool ret = true;
    auto pConfig_StreamInfo_P1 = in->pConfiguration_StreamInfo_P1;

    if (!pConfig_StreamInfo_P1->count(sensorId))
    {
        MY_LOGE("sensorId(%d) doesn't exist in pConfiguration_StreamInfo_P1", sensorId);
        return false;
    }

    // IMGO
    if ((*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Imgo != nullptr) {
        needP1Dma |= P1_IMGO;
    }
    else {
        MY_LOGI("The current pipeline config without IMGO output");
    }
    // RRZO
    if ((*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo != nullptr) {
        if (in->needP2StreamNode || miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF || needRrzoBuffer) {
            // must need rrzo for streaming and vsdof
            needP1Dma |= P1_RRZO;
        }
        else {
            // capture reuest will decide the requirement of rrzo by triggered provider
            MY_LOGD("needP2StreamNode(%d), miMultiCamFeatureMode(%d), the rrzo will request by triggered provider",
                    in->needP2StreamNode, miMultiCamFeatureMode);
        }
    }
    else {
        MY_LOGI("The current pipeline config without RRZO output");
    }
    // LCSO
    if ((*pConfig_StreamInfo_P1).at(sensorId).pHalImage_P1_Lcso != nullptr) {
        needP1Dma |= P1_LCSO;
    }
    else {
        MY_LOGD("The current pipeline config without LCSO output");
    }
    if ( !(needP1Dma & (P1_IMGO|P1_RRZO)) ) {
        MY_LOGW("P1Dma output without source buffer, sensorId(%u)", sensorId);
        ret = false;
    }
    return ret;
}

auto
FeatureSettingPolicy::
updateRequestResultParams(
    std::shared_ptr<RequestResultParams> &requestParams,
    MetadataPtr pOutMetaApp_Additional,
    MetadataPtr pOutMetaHal_Additional,
    uint32_t needP1Dma,
    uint32_t sensorId,
    MINT64 featureCombination,
    MINT32 requestIndex,
    MINT32 requestCount
) -> bool
{
    //CAM_ULOGM_APILIFE();
    MY_LOGD_IF(2 <= mbDebug, "updateRequestResultParams for physical sensorId:%d", sensorId);
    MetadataPtr pOutMetaApp = std::make_shared<IMetadata>();
    MetadataPtr pOutMetaHal = std::make_shared<IMetadata>();
    if (pOutMetaApp_Additional.get() != nullptr) {
        *pOutMetaApp += *pOutMetaApp_Additional;
    }
    if (pOutMetaHal_Additional.get() != nullptr) {
        *pOutMetaHal += *pOutMetaHal_Additional;
    }
    //check ISP profile
    MUINT8 ispProfile = 0;
    if (IMetadata::getEntry<MUINT8>(pOutMetaHal.get(), MTK_3A_ISP_PROFILE, ispProfile)) {
        MY_LOGD_IF(2 <= mbDebug, "updated isp profile(%d)", ispProfile);
    }
    else {
        MY_LOGD_IF(2 <= mbDebug, "no updated isp profile in pOutMetaHal");
    }

    //check RAW processed type
    MINT32 rawType = -1;
    if (IMetadata::getEntry<MINT32>(pOutMetaHal.get(), MTK_P1NODE_RAW_TYPE, rawType)) {
        MY_LOGD("indicated rawType(%d) by featureCombination(%#" PRIx64")", rawType, featureCombination);
    }
    else {
        MY_LOGD_IF(2 <= mbDebug, "no indicated MTK_P1NODE_RAW_TYPE in pOutMetaHal");
    }

    // per-frame update MTK_3A_ISP_MDP_TARGET_SIZE
    IMetadata::setEntry<MSize>(pOutMetaHal.get(), MTK_3A_ISP_MDP_TARGET_SIZE, mMdpTargetSize);
    MY_LOGD_IF(2 <= mbDebug, "MdpTargetSize(%dx%d)", mMdpTargetSize.w, mMdpTargetSize.h);

    if (featureCombination != -1) {
        MY_LOGD_IF(2 <= mbDebug, "update featureCombination=%#" PRIx64"", featureCombination);
        if (mPolicyParams.bIsLogicalCam)
            IMetadata::setEntry<MINT64>(pOutMetaHal.get(), MTK_FEATURE_CAPTURE, featureCombination);
        else
            IMetadata::setEntry<MINT64>(pOutMetaHal.get(), MTK_FEATURE_CAPTURE_PHYSICAL, featureCombination);
    }
    if (requestIndex || requestCount) {
        MY_LOGD_IF(2 <= mbDebug, "update MTK_HAL_REQUEST_INDEX=%d, MTK_HAL_REQUEST_COUNT=%d", requestIndex, requestCount);
        IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_HAL_REQUEST_INDEX, requestIndex);
        IMetadata::setEntry<MINT32>(pOutMetaHal.get(), MTK_HAL_REQUEST_COUNT, requestCount);
    }
    if (CC_UNLIKELY(2 <= mbDebug)) {
        (*pOutMetaApp).dump();
        (*pOutMetaHal).dump();
    }
    const MBOOL isMainSensorId = (sensorId == mReqMasterId);
    if (requestParams.get() == nullptr) {
        MY_LOGD_IF(2 <= mbDebug, "no frame setting, create a new one");
        requestParams = std::make_shared<RequestResultParams>();
        // App Metadata, just main sensor has the app metadata
        if(isMainSensorId) {
            requestParams->additionalApp = pOutMetaApp;
        };
        // HAl Metadata
        MY_LOGD_IF(2 <= mbDebug, "additionalHal for sensor id:%u", sensorId);
        requestParams->additionalHal.emplace(sensorId, pOutMetaHal);
        //P1Dma requirement
        MY_LOGD_IF(2 <= mbDebug, "needP1Dma for sensor id:%u", sensorId);
        requestParams->needP1Dma.emplace(sensorId, needP1Dma);
    }
    else {
        MY_LOGD_IF(2 <= mbDebug, "frame setting has been created by previous policy, update it");
        // App Metadata, just main sensor has the app metadata
        if (isMainSensorId) {
            if (requestParams->additionalApp.get() != nullptr) {
                MY_LOGD_IF(2 <= mbDebug, "[App] append the setting");
                *(requestParams->additionalApp) += *pOutMetaApp;
            }
            else {
                MY_LOGD_IF(2 <= mbDebug, "no app metadata, use the new one");
                requestParams->additionalApp = pOutMetaApp;
            }
        }

        // HAl Metadata
        if (!requestParams->additionalHal.count(sensorId))
        {
            MY_LOGD_IF(2 <= mbDebug, "additionalHal for sensor id:%u", sensorId);
            requestParams->additionalHal.emplace(sensorId, pOutMetaHal);
        }
        else {
            MY_LOGD_IF(2 <= mbDebug, "[Hal] append the setting for sensor id:%u", sensorId);
            *(requestParams->additionalHal.at(sensorId)) += *pOutMetaHal;
        }
        //P1Dma requirement
        MY_LOGD_IF(2 <= mbDebug, "needP1Dma for sensor id:%u", sensorId);
        if (!requestParams->needP1Dma.count(sensorId))
            requestParams->needP1Dma.emplace(sensorId, needP1Dma);
        else
            requestParams->needP1Dma.at(sensorId) |= needP1Dma;
    }
    return true;
}

auto
FeatureSettingPolicy::
queryPolicyState(
    Policy::State& state,
    uint32_t sensorId,
    ParsedStrategyInfo const& parsedInfo,
    RequestOutputParams const* out,
    RequestInputParams const* in
) -> bool
{
    if (in && in->pRequest_AppImageStreamInfo) {
        // provide the App request image buffers type and purpose
        // for feature plugins execute suitable behavior
        state.pParsedAppImageStreamInfo = in->pRequest_AppImageStreamInfo;
    }
    else {
        MY_LOGW("in->pRequest_AppImageStreamInfo is invalid nullptr");
    }

    state.mZslPoolReady    = parsedInfo.isZslModeOn;
    state.mZslRequest      = parsedInfo.isZslFlowOn;
    state.mFlashFired      = parsedInfo.isFlashOn;
    state.mExposureTime    = parsedInfo.exposureTime;
    state.mRealIso         = parsedInfo.realIso;
    state.mAppManual3A     = parsedInfo.isAppManual3A;
    state.mFreeMemoryMBytes = parsedInfo.freeMemoryMBytes;
    MY_LOGD("policy sensor id %d", sensorId);
    // check manual 3A setting.
    if (out->mainFrame.get()) {
        IMetadata const* pAppMeta = out->mainFrame->additionalApp.get();
        MUINT8 aeMode = MTK_CONTROL_AE_MODE_ON;
        if (IMetadata::getEntry<MUINT8>(pAppMeta, MTK_CONTROL_AE_MODE, aeMode) &&
            aeMode  == MTK_CONTROL_AE_MODE_OFF)
        {
            MINT32 manualIso = 0;
            MINT64 manualExposureTime = 0;
            if (IMetadata::getEntry<MINT32>(pAppMeta, MTK_SENSOR_SENSITIVITY, manualIso) &&
                IMetadata::getEntry<MINT64>(pAppMeta, MTK_SENSOR_EXPOSURE_TIME, manualExposureTime) &&
                manualIso > 0 && manualExposureTime > 0)
            {
                state.mRealIso        = manualIso;
                state.mExposureTime   = manualExposureTime/1000; //ns to us
                // not support Zsl for manual AE
                state.mZslPoolReady = false;
                state.mZslRequest   = false;
                MY_LOGI("capture frame setting has been set as manual iso(%u -> %u)/exposure(%u -> %u)us by previous pluign, and must set Zsl off",
                        parsedInfo.realIso, state.mRealIso, parsedInfo.exposureTime, state.mExposureTime);
            }
            else {
                MY_LOGD("it is not manual iso(%d)/exposure(%" PRId64 " ns)", manualIso, manualExposureTime);
            }
        }
        else {
            MY_LOGD_IF(mbDebug, "it is not manual ae mode(%d)", aeMode);
        }
    }
    else {
        MY_LOGD_IF(mbDebug, "no need to check mainFram info(%p)", out->mainFrame.get());
    }

    // get sensor info (the info is after reconfigure if need)
    state.mSensorMode   = out->sensorMode.at(sensorId);
    uint32_t needP1Dma = 0;
    if (!getCaptureP1DmaConfig(needP1Dma, in, sensorId) ){
        MY_LOGE("P1Dma output is invalid: 0x%X", needP1Dma);
        return false;
    }
    HwInfoHelper helper(sensorId);
    if (!helper.updateInfos()) {
        MY_LOGE("HwInfoHelper cannot properly update infos");
        return false;
    }
    //
    uint32_t pixelMode = 0;
    MINT32 sensorFps = 0;
    if (!helper.getSensorSize(state.mSensorMode, state.mSensorSize) ||
        !helper.getSensorFps(state.mSensorMode, sensorFps) ||
        !helper.queryPixelMode(state.mSensorMode, sensorFps, pixelMode)) {
        MY_LOGE("cannot get params about sensor");
        return false;
    }
    //
    int32_t bitDepth = 10;
    helper.getRecommendRawBitDepth(bitDepth);
    //
    MINT format = 0;
    size_t stride = 0;
    if (needP1Dma & P1_IMGO) {
        // use IMGO as source for capture
        if (!helper.getImgoFmt(bitDepth, format) ||
            !helper.alignPass1HwLimitation(pixelMode, format, true/*isImgo*/, state.mSensorSize, stride) ) {
            MY_LOGE("cannot query raw buffer info about imgo");
            return false;
        }
    }
    else {
        // use RRZO as source for capture
        auto rrzoSize = (*in->pConfiguration_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo->getImgSize();
        MY_LOGW("no IMGO buffer, use RRZO size(%d, %d) as capture source image (for better quality, not suggest to use RRZO to capture)",
                rrzoSize.w, rrzoSize.h);
    }

    // get dualcam state info
    state.mMultiCamFeatureMode = miMultiCamFeatureMode;
    const MBOOL isDualCamVSDoFMode = (state.mMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF);
    if (isDualCamVSDoFMode) {
        StereoSizeProvider::getInstance()->getDualcamP2IMGOYuvCropResizeInfo(
                sensorId,
                state.mDualCamDedicatedFov,
                state.mDualCamDedicatedYuvSize);
        MY_LOGD("dualcam indicated P2A output yuv(sensorId:%d, FOV(x:%d,y:%d,w:%d,h:%d), yuv size(%dx%d))",
            sensorId,
            state.mDualCamDedicatedFov.p.x, state.mDualCamDedicatedFov.p.y,
            state.mDualCamDedicatedFov.s.w, state.mDualCamDedicatedFov.s.h,
            state.mDualCamDedicatedYuvSize.w, state.mDualCamDedicatedYuvSize.h);
    } else {
        MY_LOGD("multicam feature mode, sensorId:%d, mode:%d", sensorId, state.mMultiCamFeatureMode);
    }

    // get hdr mode
    if (mCHDRHelper.at(mReqMasterId)->isHDR()) {
        state.mHDRHalMode = mCHDRHelper.at(mReqMasterId)->getHDRHalMode();
        state.mValidExpNum = mCHDRHelper.at(mReqMasterId)->getValidExposureNum();
    }

    MY_LOGD("zslPoolReady:%d, zslRequest:%d, flashFired:%d, appManual3A(%d), exposureTime:%u, realIso:%u",
        state.mZslPoolReady, state.mZslRequest, state.mFlashFired, state.mAppManual3A, state.mExposureTime, state.mRealIso);
    MY_LOGD("sensor(Id:%d, mode:%u, size(%d, %d))",
        sensorId, state.mSensorMode,
        state.mSensorSize.w, state.mSensorSize.h);
    return true;
}

auto
FeatureSettingPolicy::
updatePolicyDecision(
    RequestOutputParams* out,
    uint32_t sensorId,
    Policy::Decision const& decision,
    RequestInputParams const* in __unused
) -> bool
{
    out->needZslFlow &= decision.mZslEnabled;
    // update Zsl requirement
    if (out->needZslFlow) {
        out->zslPolicyParams.pZslPluginParams = std::make_shared<ZslPolicy::ZslPluginParams>(decision.mZslPolicy);  // TODO: remove
        out->zslPolicyParams.pSelector = decision.pZslSelector;
        MY_LOGD("update needZslFlow(%d), zslPluginParams=%s, pSelector=%s", out->needZslFlow,
                ZslPolicy::toString(out->zslPolicyParams.pZslPluginParams).c_str(), ZslPolicy::toString(out->zslPolicyParams.pSelector).c_str());
    }

    if (decision.mSensorMode != SENSOR_SCENARIO_ID_UNNAMED_START) {
        if (!out->sensorMode.count(sensorId)) {
            MY_LOGW("default sensorMode doesn't contain the key:%u", sensorId);
            return false;
        }
        out->sensorMode.at(sensorId) = decision.mSensorMode;
        MY_LOGD("feature request sensorMode:%d", decision.mSensorMode);
    }

    // update Raw buffer format requirement
    if (CC_UNLIKELY(decision.mNeedUnpackRaw == true)) {
        bool originalNeedUnpackRaw = out->needUnpackRaw;
        out->needUnpackRaw |= decision.mNeedUnpackRaw;
        MY_LOGI("set needUnpackRaw(%d|%d => %d)", originalNeedUnpackRaw, decision.mNeedUnpackRaw, out->needUnpackRaw);
    }

    // if the feature support & required to drop previous preview frame to speed-up this request's exposure behavior
    if (out->bDropPreviousPreviewRequest != decision.mDropPreviousPreviewRequest) {
        bool originalDrop = out->bDropPreviousPreviewRequest;
        out->bDropPreviousPreviewRequest |= decision.mDropPreviousPreviewRequest;
        MY_LOGI("set drop previous preview request(%d|%d => %d)",
                originalDrop, decision.mDropPreviousPreviewRequest, out->bDropPreviousPreviewRequest);
    }

    // update setting for boostControl
    updateBoostControl(out, decision.mBoostControl);

    return true;
}

template <typename T>
auto
FeatureSettingPolicy::
syncMasterMetaToSlave(
    MetadataPtr masterMeta,
    MetadataPtr slaveMeta,
    IMetadata::Tag_t tag
) -> void
{
    T val = 0;
    if (IMetadata::getEntry<T>(masterMeta.get(), tag, val)) {
        MY_LOGD("update additional meta %X(%d)", tag, val);
        IMetadata::setEntry<T>(slaveMeta.get(), tag, val);
    } else {
        MY_LOGD("cannot find tag %X", tag);
    }
}

auto
FeatureSettingPolicy::
updateDualCamRequestOutputParams(
    RequestOutputParams* out,
    MetadataPtr pOutMetaApp_Additional,
    MetadataPtr pOutMetaHal_Additional,
    uint32_t mainCamP1Dma,
    uint32_t sub1CamP1Dma,
    MINT64 featureCombination,
    unordered_map<uint32_t, MetadataPtr>& halMetaPtrList,
    RequestInputParams const* in
) -> bool
{
    CAM_ULOGM_APILIFE();

    // Only 1 sub sensor so far
    const MINT32 subSensorId = mReqSubSensorIdList[0];
    //
    BoostControl boostControl;
    boostControl.boostScenario = IScenarioControlV3::Scenario_ContinuousShot;
    FEATURE_CFG_ENABLE_MASK(boostControl.featureFlag, IScenarioControlV3::FEATURE_STEREO_CAPTURE);
    updateBoostControl(out, boostControl);
    MY_LOGD("update boostControl for DualCam Capture (boostScenario:0x%X, featureFlag:0x%X)",
            boostControl.boostScenario, boostControl.featureFlag);
    auto masterId = in->pMultiCamReqOutputParams->prvMasterId;
    MetadataPtr pOutMetaHal_Additional_Master = std::make_shared<IMetadata>();
    *pOutMetaHal_Additional_Master += *pOutMetaHal_Additional;
    {
        auto iter = halMetaPtrList.find(masterId);
        if(iter != halMetaPtrList.end())
        {
            *pOutMetaHal_Additional_Master += *iter->second;
        }
    }
    MetadataPtr pOutMetaHal_Additional_Slave = std::make_shared<IMetadata>();
    *pOutMetaHal_Additional_Slave += *pOutMetaHal_Additional;
    *pOutMetaHal_Additional_Slave += *out->mainFrame->additionalHal.at(mReqMasterId);
    {
        for(auto&& item:in->pMultiCamReqOutputParams->streamingSensorList)
        {
            if(item != masterId)
            {
                auto iter = halMetaPtrList.find(item);
                if(iter != halMetaPtrList.end())
                {
                    *pOutMetaHal_Additional_Slave += *iter->second;
                    break;
                }
            }
        }
    }
    // update mainFrame
    // main1 mainFrame
    updateRequestResultParams(
        out->mainFrame,
        pOutMetaApp_Additional,
        pOutMetaHal_Additional_Master,
        mainCamP1Dma,
        mReqMasterId,
        featureCombination);
    // main2 mainFrame
    updateRequestResultParams(
        out->mainFrame,
        nullptr, // sub sensor no need to set app metadata
        pOutMetaHal_Additional_Slave, // duplicate main1 metadata
        sub1CamP1Dma,
        subSensorId,
        featureCombination);
    // update subFrames
    MY_LOGD("update subFrames size(%zu)", out->subFrames.size());
    for (size_t i = 0; i < out->subFrames.size(); i++) {
        auto subFrame = out->subFrames[i];
        if (subFrame.get()) {
            MY_LOGI("subFrames[%zu] has existed(addr:%p)", i,subFrame.get());
            // main1 subFrame
            updateRequestResultParams(
                subFrame,
                pOutMetaApp_Additional,
                pOutMetaHal_Additional_Master,
                mainCamP1Dma,
                mReqMasterId,
                featureCombination);
            // main2 subFrame
            MINT32 requestIndex = -1;
            MINT32 requestCount = 0;
            IMetadata::getEntry<MINT32>(subFrame->additionalHal.at(mReqMasterId).get(), MTK_HAL_REQUEST_INDEX, requestIndex);
            IMetadata::getEntry<MINT32>(subFrame->additionalHal.at(mReqMasterId).get(), MTK_HAL_REQUEST_COUNT, requestCount);
            updateRequestResultParams(
                subFrame,
                nullptr, // sub sensor no need to set app metadata
                pOutMetaHal_Additional_Slave, // duplicate main1 metadata
                sub1CamP1Dma,
                subSensorId,
                featureCombination,
                requestIndex,
                requestCount);
        }
        else {
            MY_LOGE("subFrames[%zu] is invalid", i);
        }

    }
    // update preDummyFrames
    MY_LOGD("update preDummyFrames size(%zu)", out->preDummyFrames.size());
    for (size_t i = 0; i < out->preDummyFrames.size(); i++) {
        auto preDummyFrame = out->preDummyFrames[i];
        if (preDummyFrame.get()) {
            MY_LOGE("preDummyFrames[%zu] has existed(addr:%p)",i, preDummyFrame.get());

            MetadataPtr pOutMetaHal_Additional_Slave_PreDummy = std::make_shared<IMetadata>();
            *pOutMetaHal_Additional_Slave_PreDummy += *pOutMetaHal_Additional_Slave;
            syncMasterMetaToSlave<MBOOL>(preDummyFrame->additionalHal.at(mReqMasterId), pOutMetaHal_Additional_Slave_PreDummy, MTK_3A_DUMMY_BEFORE_REQUEST_FRAME);
            // main1 subFrame
            updateRequestResultParams(
                preDummyFrame,
                pOutMetaApp_Additional,
                pOutMetaHal_Additional_Master,
                mainCamP1Dma,
                mReqMasterId,
                featureCombination);
            // main2 subFrame
            updateRequestResultParams(
                preDummyFrame,
                nullptr, // sub sensor no need to set app metadata
                pOutMetaHal_Additional_Slave_PreDummy, // duplicate main1 metadata
                sub1CamP1Dma,
                subSensorId,
                featureCombination);
        }
        else {
            MY_LOGE("preDummyFrames[%zu] is invalid", i);
        }
    }
    // update postDummyFrames
    MY_LOGD("update postDummyFrames size(%zu)", out->postDummyFrames.size());
    for (size_t i = 0; i < out->postDummyFrames.size(); i++) {
        auto postDummyFrame = out->postDummyFrames[i];
        if (postDummyFrame.get()) {
            MY_LOGI("postDummyFrames[%zu] has existed(addr:%p)", i, postDummyFrame.get());

            MetadataPtr pOutMetaHal_Additional_Slave_PostDummy = std::make_shared<IMetadata>();
            *pOutMetaHal_Additional_Slave_PostDummy += *pOutMetaHal_Additional_Slave;
            syncMasterMetaToSlave<MBOOL>(postDummyFrame->additionalHal.at(mReqMasterId), pOutMetaHal_Additional_Slave_PostDummy, MTK_3A_DUMMY_AFTER_REQUEST_FRAME);

            // main1 subFrame
            updateRequestResultParams(
                postDummyFrame,
                pOutMetaApp_Additional,
                pOutMetaHal_Additional_Master,
                mainCamP1Dma,
                mReqMasterId,
                featureCombination);
            // main2 subFrame
            updateRequestResultParams(
                postDummyFrame,
                nullptr, // sub sensor no need to set app metadata
                pOutMetaHal_Additional_Slave_PostDummy, // duplicate main1 metadata
                sub1CamP1Dma,
                subSensorId,
                featureCombination);
        }
        else
        {
            MY_LOGE("postDummyFrames[%zu] is invalid", i);
        }
    }
    return true;
};

auto
FeatureSettingPolicy::
updateRequestOutputAllFramesMetadata(
    RequestOutputParams* out,
    MetadataPtr pOutMetaApp_Additional,
    MetadataPtr pOutMetaHal_Additional
) -> bool
{
    if (out->mainFrame.get() == nullptr) {
        MY_LOGW("no mainFrame could be updated!!");
        return false;
    }
    //
    // update mainFrame
    for (const auto& [_sensorId, streamInfo_P1] : out->mainFrame->additionalHal) {
        MY_LOGD_IF(mbDebug, "update mainFrame, sensor id:%d", _sensorId);
        updateRequestResultParams(
            out->mainFrame,
            (_sensorId == mReqMasterId) ? pOutMetaApp_Additional : nullptr, // sub sensor no need to set app metadata
            pOutMetaHal_Additional,
            0,
            _sensorId);
    }
    // update subFrames
    MY_LOGD_IF(mbDebug, "update subFrames size(%zu)", out->subFrames.size());
    for (size_t i = 0; i < out->subFrames.size(); i++) {
        auto& subFrame = out->subFrames[i];
        if (subFrame.get()) {
            MY_LOGD_IF(mbDebug, "subFrames[%zu] has existed(addr:%p)", i, subFrame.get());
            for (const auto& [_sensorId, _additionalHal] : subFrame->additionalHal) {
                MY_LOGD_IF(mbDebug, "update subFrame, sensor id:%d", _sensorId);
                updateRequestResultParams(
                    subFrame,
                    (_sensorId == mReqMasterId) ? pOutMetaApp_Additional : nullptr, // sub sensor no need to set app metadata
                    pOutMetaHal_Additional,
                    0,
                    _sensorId);
            }
        }
        else {
            MY_LOGW("subFrames[%zu] is invalid", i);
        }
    }
    //
    return true;
}

auto
FeatureSettingPolicy::
strategyMultiFramePlugin(
    MINT64 combinedKeyFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64& featureCombination, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64& foundFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in
) -> bool
{
    auto provider = mMFPPluginWrapperPtr->getProvider(combinedKeyFeature, foundFeature);
    if (provider) {
        // for MultiFramePlugin key feature (ex: HDR, MFNR, 3rd party multi-frame algo,etc )
        // negotiate and query feature requirement
        auto pAppMetaControl = in->pRequest_AppControl;
        auto property =  provider->property();
        auto pSelection = mMFPPluginWrapperPtr->createSelection();
        MFP_Selection& sel = *pSelection;
        sel.mRequestIndex = 0;
        mMFPPluginWrapperPtr->offer(sel);
        //
        uint32_t mainCamP1Dma = 0;
        if ( !getCaptureP1DmaConfig(mainCamP1Dma, in, mReqMasterId, property.mNeedRrzoBuffer) ){
            MY_LOGE("main P1Dma output is invalid: 0x%X", mainCamP1Dma);
            return false;
        }
        // check the requirement of rrzo by provider's request
        MY_LOGI_IF(property.mNeedRrzoBuffer, "feature(%s) request rrzo dma buffer, P1Dma(0x%X)", property.mName, mainCamP1Dma);

        // update app metadata for plugin reference
        MetadataPtr pInMetaApp = std::make_shared<IMetadata>(*pAppMetaControl);
        sel.mIMetadataApp.setControl(pInMetaApp);
        // update previous Hal ouput for plugin reference
        MetadataPtr pInMetaHal = std::make_shared<IMetadata>();
        if (out->mainFrame.get() && out->mainFrame->additionalHal.at(mReqMasterId)) {
            auto pHalMeta = out->mainFrame->additionalHal.at(mReqMasterId);
            *pInMetaHal += *pHalMeta;
            MY_LOGD("append default Hal meta");
        }
        sel.mIMetadataHal.setControl(pInMetaHal);
        if (parsedInfo.ispTuningHint >= 0 && parsedInfo.isRawRequestForIspHidl == true) {
            // if request raw frame from camera for isp hidl reprocessing.
            sel.mIspHidlStage = ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA;
        }
        sel.mSensorId = mReqMasterId;
        // query state  for plugin provider negotiate
        if (!queryPolicyState(
                    sel.mState,
                    mReqMasterId,
                    parsedInfo, out, in)) {
            MY_LOGE("cannot query state for plugin provider negotiate!");
            return false;
        }
        if (provider->negotiate(sel) == OK && sel.mRequestCount > 0) {
            MY_LOGD("MultiFrame request count : %d", sel.mRequestCount);
            if (!updatePolicyDecision( out, mReqMasterId, sel.mDecision, in)) {
                MY_LOGW("update config info failed!");
                return false;
            }
            if (CC_LIKELY(sel.mDecision.mProcess)) {
                pSelection->mTokenPtr = MFP_Selection::createToken(mUniqueKey, in->requestNo, 0, sel.mSensorId, !mPolicyParams.bIsLogicalCam);
                mMFPPluginWrapperPtr->keepSelection(in->requestNo, provider, pSelection);
            }
            else {
                MY_LOGD("%s(%s) bypass process, only decide frames requirement",
                        mMFPPluginWrapperPtr->getName().c_str(), property.mName);
                featureCombination &= ~foundFeature;
            }
            MetadataPtr pOutMetaApp_Additional = sel.mIMetadataApp.getAddtional();
            // keep original hal meta & update feature plugin's meta
            MetadataPtr pOutMetaHal_Additional = std::make_shared<IMetadata>();
            *pOutMetaHal_Additional += *pInMetaHal;
            if (sel.mIMetadataHal.getAddtional() != nullptr) {
                *pOutMetaHal_Additional += *(sel.mIMetadataHal.getAddtional());
            }
            updateRequestResultParams(
                    out->mainFrame,
                    pOutMetaApp_Additional,
                    pOutMetaHal_Additional,
                    mainCamP1Dma,
                    mReqMasterId,
                    featureCombination,
                    0,
                    sel.mRequestCount);

            auto getDummyFrames = [this]
            (
                RequestOutputParams* out,
                MFP_Selection& sel,
                const uint32_t camP1Dma,
                const uint32_t sensorId
            ) -> MBOOL
            {
                // get preDummyFrames if the key feature requiresd
                MY_LOGD("preDummyFrames count:%d", sel.mFrontDummy);
                if (sel.mFrontDummy > 0) {
                    for (MINT32 i = 0; i < sel.mFrontDummy; i++) {
                        MetadataPtr pAppDummy_Additional = sel.mIMetadataApp.getDummy();
                        MetadataPtr pHalDummy_Additional = sel.mIMetadataHal.getDummy();
                        IMetadata::setEntry<MBOOL>(pHalDummy_Additional.get(), MTK_3A_DUMMY_BEFORE_REQUEST_FRAME, 1);
                        std::shared_ptr<RequestResultParams> preDummyFrame = nullptr;
                        updateRequestResultParams(
                                preDummyFrame,
                                pAppDummy_Additional,
                                pHalDummy_Additional,
                                camP1Dma,
                                sensorId);
                        //
                        out->preDummyFrames.push_back(preDummyFrame);
                    }
                }
                // get postDummyFrames if the key feature requiresd
                MY_LOGD("postDummyFrames count:%d", sel.mPostDummy);
                if (sel.mPostDummy > 0) {
                    for (MINT32 i = 0; i < sel.mPostDummy; i++) {
                        MetadataPtr pAppDummy_Additional = sel.mIMetadataApp.getDummy();
                        MetadataPtr pHalDummy_Additional = sel.mIMetadataHal.getDummy();
                        IMetadata::setEntry<MBOOL>(pHalDummy_Additional.get(), MTK_3A_DUMMY_AFTER_REQUEST_FRAME, 1);
                        std::shared_ptr<RequestResultParams> postDummyFrame = nullptr;
                        updateRequestResultParams(
                                postDummyFrame,
                                pAppDummy_Additional,
                                pHalDummy_Additional,
                                camP1Dma,
                                sensorId);
                        //
                        out->postDummyFrames.push_back(postDummyFrame);
                    }
                }
                return true;
            };
            // get the dummy frames if the first main negotiate return the front/rear dummy info.
            getDummyFrames(out, sel, mainCamP1Dma, mReqMasterId);
            for (uint32_t i = 1; i < sel.mRequestCount; i++)
            {
                auto pSubSelection = mMFPPluginWrapperPtr->createSelection();
                if (CC_LIKELY(sel.mDecision.mProcess)) {
                    pSubSelection->mTokenPtr = MFP_Selection::createToken(mUniqueKey, in->requestNo, i, sel.mSensorId, !mPolicyParams.bIsLogicalCam);
                    mMFPPluginWrapperPtr->keepSelection(in->requestNo, provider, pSubSelection);
                }
                else {
                    MY_LOGD("%s(%s) bypass process, only decide frames requirement",
                            mMFPPluginWrapperPtr->getName().c_str(), property.mName);
                    featureCombination &= ~foundFeature;
                }
                MFP_Selection& subsel = *pSubSelection;
                subsel.mState = sel.mState;
                subsel.mRequestIndex = i;
                mMFPPluginWrapperPtr->offer(subsel);
                subsel.mIMetadataApp.setControl(pInMetaApp);
                // update previous Hal ouput for plugin reference
                subsel.mIMetadataHal.setControl(pInMetaHal);
                provider->negotiate(subsel);
                // add metadata
                pOutMetaApp_Additional = subsel.mIMetadataApp.getAddtional();
                // keep original hal meta & update feature plugin's hal meta
                pOutMetaHal_Additional = std::make_shared<IMetadata>();
                *pOutMetaHal_Additional += *pInMetaHal;
                if (subsel.mIMetadataHal.getAddtional() != nullptr) {
                    *pOutMetaHal_Additional += *(subsel.mIMetadataHal.getAddtional());
                }
                std::shared_ptr<RequestResultParams> subFrame = nullptr;
                auto subFrameIndex = i - 1;
                if (out->subFrames.size() > subFrameIndex) {
                    MY_LOGI("subFrames size(%zu), subFrames[%d] has existed(addr:%p)",
                            out->subFrames.size(), subFrameIndex, (out->subFrames[subFrameIndex]).get());
                    subFrame = out->subFrames[subFrameIndex];
                    updateRequestResultParams(
                            subFrame,
                            pOutMetaApp_Additional,
                            pOutMetaHal_Additional,
                            mainCamP1Dma,
                            mReqMasterId,
                            featureCombination,
                            i,
                            sel.mRequestCount);
                    //
                    out->subFrames[i] = subFrame;
                }
                else {
                    MY_LOGD("subFrames size(%zu), no subFrames[%d], must ceate a new one", out->subFrames.size(), subFrameIndex);
                    updateRequestResultParams(
                            subFrame,
                            pOutMetaApp_Additional,
                            pOutMetaHal_Additional,
                            mainCamP1Dma,
                            mReqMasterId,
                            featureCombination,
                            i,
                            sel.mRequestCount);
                    //
                    out->subFrames.push_back(subFrame);
                }
                // get the dummy frames if the subframes negotiate return the front/rear dummy info.
                getDummyFrames(out, subsel, mainCamP1Dma, mReqMasterId);
            }
            MY_LOGD("%s(%s), trigger provider(mRequestCount:%d) for foundFeature(%#" PRIx64")",
                    mMFPPluginWrapperPtr->getName().c_str(), property.mName, sel.mRequestCount, foundFeature);
        }
        else {
            MY_LOGD("%s(%s), no need to trigger provider(mRequestCount:%d) for foundFeature(%#" PRIx64")",
                    mMFPPluginWrapperPtr->getName().c_str(), property.mName, sel.mRequestCount, foundFeature);
            return false;
        }
    }
    else
    {
        MY_LOGD_IF(mbDebug, "no provider for multiframe key feature(%#" PRIx64")", combinedKeyFeature);
    }

    return true;
}

auto
FeatureSettingPolicy::
strategySingleRawPlugin(
    MINT64 combinedKeyFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64& featureCombination, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64& foundFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in
) -> bool
{
    auto provider = mRawPluginWrapperPtr->getProvider(combinedKeyFeature, foundFeature);
    if (provider) {
        // for RawPlugin key feature (ex: SW 4Cell) negotiate and query feature requirement
        uint32_t mainCamP1Dma = 0;
        if ( !getCaptureP1DmaConfig(mainCamP1Dma, in, mReqMasterId) ){
            MY_LOGE("main P1Dma output is invalid: 0x%X", mainCamP1Dma);
            return false;
        }
        auto pAppMetaControl = in->pRequest_AppControl;
        auto property =  provider->property();
        auto pSelection = mRawPluginWrapperPtr->createSelection();
        Raw_Selection& sel = *pSelection;
        mRawPluginWrapperPtr->offer(sel);
        // update app metadata for plugin reference
        MetadataPtr pInMetaApp = std::make_shared<IMetadata>(*pAppMetaControl);
        sel.mIMetadataApp.setControl(pInMetaApp);
        // update previous Hal ouput for plugin reference
        MetadataPtr pInMetaHal = std::make_shared<IMetadata>();
        if (out->mainFrame.get() && out->mainFrame->additionalHal.at(mReqMasterId)) {
            auto pHalMeta = out->mainFrame->additionalHal.at(mReqMasterId);
            *pInMetaHal += *pHalMeta;
            MY_LOGD("append default Hal meta");
        }
        sel.mIMetadataHal.setControl(pInMetaHal);
        //
        if (parsedInfo.ispTuningHint >= 0 && parsedInfo.isRawRequestForIspHidl == true) {
            // if request raw frame from camera for isp hidl reprocessing.
            sel.mIspHidlStage = ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA;
        }
        sel.mSensorId = mReqMasterId;
        // query state  for plugin provider strategy
        if (!queryPolicyState(
                    sel.mState,
                    mReqMasterId,
                    parsedInfo, out, in)) {
            MY_LOGE("cannot query state for plugin provider negotiate!");
            return false;
        }
        if (provider->negotiate(sel) == OK) {
            if (!updatePolicyDecision( out, mReqMasterId, sel.mDecision, in)) {
                MY_LOGW("update config info failed!");
                return false;
            }
            //
            if (CC_LIKELY(sel.mDecision.mProcess)) {
                pSelection->mTokenPtr = Raw_Selection::createToken(mUniqueKey, in->requestNo, 0, sel.mSensorId, !mPolicyParams.bIsLogicalCam);
                mRawPluginWrapperPtr->keepSelection(in->requestNo, provider, pSelection);
            }
            else {
                MY_LOGD("%s(%s) bypass process, only decide frames requirement",
                        mRawPluginWrapperPtr->getName().c_str(), property.mName);
                featureCombination &= ~foundFeature;
            }
            MetadataPtr pOutMetaApp_Additional = sel.mIMetadataApp.getAddtional();
            // keep original hal meta & update feature plugin's hal meta
            MetadataPtr pOutMetaHal_Additional = std::make_shared<IMetadata>();
            *pOutMetaHal_Additional += *pInMetaHal;
            if (sel.mIMetadataHal.getAddtional() != nullptr) {
                *pOutMetaHal_Additional += *(sel.mIMetadataHal.getAddtional());
            }
            updateRequestResultParams(
                    out->mainFrame,
                    pOutMetaApp_Additional,
                    pOutMetaHal_Additional,
                    mainCamP1Dma,
                    mReqMasterId,
                    featureCombination);
            //
            MY_LOGD("%s(%s), trigger provider for foundFeature(%#" PRIx64")",
                mRawPluginWrapperPtr->getName().c_str(), property.mName, foundFeature);
        }
        else {
            MY_LOGD("%s(%s), no need to trigger provider for foundFeature(%#" PRIx64")",
                mRawPluginWrapperPtr->getName().c_str(), property.mName, foundFeature);
            return false;
        }
    }
    else
    {
        MY_LOGD_IF(mbDebug, "no provider for single raw key feature(%#" PRIx64")", combinedKeyFeature);
    }

    return true;
}


auto
FeatureSettingPolicy::
strategyDualCamPlugin(
    MINT64 combinedKeyFeature __unused, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64& featureCombination, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64& foundFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo __unused,
    RequestInputParams const* in,
    unordered_map<uint32_t, MetadataPtr>& halMetaPtrList
) -> bool
{
    CAM_ULOGM_APILIFE();
    MINT64 depthKeyFeature = 0;
    MINT64 bokehKeyFeature = 0;
    MINT64 fusionKeyFeature = 0;
    const MBOOL isDualCamVSDoFMode = (miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF);
    const MBOOL isDualCamZoomMode = (miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM);
    if(mDualDevicePath == DualDevicePath::MultiCamControl)
    {
        uint32_t mainCamP1Dma = 0;
        if ( !getCaptureP1DmaConfig(mainCamP1Dma, in, mReqMasterId) ) {
            MY_LOGE("main P1Dma output is invalid: 0x%X", mainCamP1Dma);
            return false;
        }
        uint32_t sub1CamP1Dma = 0;
        if ( !getCaptureP1DmaConfig(sub1CamP1Dma, in, mReqSubSensorIdList[0]) ) {
            MY_LOGE("sub1 P1Dma output is invalid: 0x%X", sub1CamP1Dma);
            return false;
        }
        // for reprocess flow, main2 dma has to set 0.
        if(in->pRequest_AppImageStreamInfo->pAppImage_Input_Yuv != nullptr) {
            MY_LOGD("multicam yuv reprocess, no needs to connect main2");
            sub1CamP1Dma = 0;
        }

        if(isDualCamVSDoFMode)
        {
            const MBOOL hasFeatureVSDoF = mDepthPluginWraperPtr->isKeyFeatureExisting(featureCombination, depthKeyFeature)
                                        && mBokehPluginWraperPtr->isKeyFeatureExisting(featureCombination, bokehKeyFeature);
            const MBOOL hasFeatureFusion = mFusionPluginWraperPtr->isKeyFeatureExisting(featureCombination, fusionKeyFeature);

            if (hasFeatureVSDoF) {
                MY_LOGD("update vsdof request output params, depth:%#" PRIx64 ", bokeh:%#" PRIx64, depthKeyFeature, bokehKeyFeature);
                // TODO: update additional metadata for depth and bokeh
            }
            else if (hasFeatureFusion) {
                MY_LOGD("update vsdof request output params, fusion:%#" PRIx64, fusionKeyFeature);
                // TODO: update additional metadata for fusion
            }
        } else if (isDualCamZoomMode) {
            const MBOOL hasFeatureFusion = mFusionPluginWraperPtr->isKeyFeatureExisting(featureCombination, fusionKeyFeature);

            if (hasFeatureFusion) {
                MY_LOGD("update zoom request output params, fusion:%#" PRIx64, fusionKeyFeature);
                // TODO: update additional metadata for fusion
            } else {
                featureCombination &= ~(TP_FEATURE_ZOOM_FUSION);
                MY_LOGW("zoom without fusion plugin, remove fusion combination, combined:%#" PRIx64, featureCombination);
            }
        }

        // update additional metadata for dual cam
        MetadataPtr pOutMetaApp_Additional = std::make_shared<IMetadata>();
        MetadataPtr pOutMetaHal_Additional = std::make_shared<IMetadata>();

        updateDualCamRequestOutputParams(
                        out,
                        pOutMetaApp_Additional,
                        pOutMetaHal_Additional,
                        mainCamP1Dma,
                        sub1CamP1Dma,
                        featureCombination,
                        halMetaPtrList,
                        in);
    }
    else
    {
        MY_LOGD("doesn't find any feature, not dualcam(vsdof mode/multicam)");
    }
    //
    foundFeature = depthKeyFeature|bokehKeyFeature|fusionKeyFeature;
    if (foundFeature) {
        MY_LOGD("found feature(%#" PRIx64") for dual cam", foundFeature);
    }
    return true;
}

auto
FeatureSettingPolicy::
strategyNormalSingleCapture(
    MINT64 combinedKeyFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64 featureCombination, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in
) -> bool
{
    if (out->subFrames.size() > 0) {
        MY_LOGD_IF(mbDebug, "frames setting has been updated by multiframe plugin");
        return true;
    }

    CAM_ULOGM_APILIFE();

    // general single frame capture's sub feature combination and requirement
    uint32_t mainCamP1Dma = 0;
    if ( !getCaptureP1DmaConfig(mainCamP1Dma, in, mReqMasterId) ){
        MY_LOGE("main P1Dma output is invalid: 0x%X", mainCamP1Dma);
        return false;
    }
    // zsl policy for general single frame capture
    auto makeCShotZslPluginParams = []() {
        static auto pCShotParams = std::make_shared<ZslPolicy::ZslPluginParams>(
            ZslPolicy::ZslPluginParams{ .mPolicy = ZslPolicy::None });
        return pCShotParams;
    };
    //
    if (parsedInfo.isZslModeOn && parsedInfo.isZslFlowOn) {
        if (out->needZslFlow == false) {
            // features may force to disable ZSL due to P1/3A exposure setting.
            MY_LOGI("ZSL has been disabled by feature(%#" PRIx64") requirement", combinedKeyFeature);
        }
        else if (parsedInfo.isCShot) {
            MY_LOGD("CShot always trigger ZSL if ZSL enable");
            out->needZslFlow = true;
            out->zslPolicyParams.pZslPluginParams = makeCShotZslPluginParams();
            MY_LOGD("(CShot) set needZslFlow, zslPluginParams=%s", ZslPolicy::toString(out->zslPolicyParams.pZslPluginParams).c_str());
            // Log for auto test tool
            CAT_LOGD("[CAT][Event] CShot:Stamp (reqNo:%u)", in->requestNo);
        }
        else if (parsedInfo.isFlashOn || parsedInfo.isAppManual3A) {
            MY_LOGD("not support Zsl due to (isFlashOn:%d, isManual3A:%d, isZslModeOn:%d, isZslFlowOn:%d)",
                    parsedInfo.isFlashOn, parsedInfo.isAppManual3A, parsedInfo.isZslModeOn, parsedInfo.isZslFlowOn);
            out->needZslFlow = false;
        }
        else if (parsedInfo.isRawReprocess || parsedInfo.isYuvReprocess) {
            if (out->needZslFlow) {
                MY_LOGW("not support zsl(%d) for raw/yuv reprocess, forced disable zsl", out->needZslFlow);
                out->needZslFlow = false;
            }
        }
        else if (parsedInfo.ispTuningHint >= 0 && out->zslPolicyParams.pZslPluginParams == nullptr) {  // if request Zsl frame for isp hidl reprocessing.
            out->needZslFlow = true;
            MY_LOGD("(IspHidl) set needZslFlow, zslPluginParams=%s DEFAULT", ZslPolicy::toString(out->zslPolicyParams.pZslPluginParams).c_str());
        }
        else if (out->zslPolicyParams.pZslPluginParams == nullptr) {
            out->needZslFlow = true;
            MY_LOGD("(Default) set needZslFlow, zslPluginParams=%s DEFAULT", ZslPolicy::toString(out->zslPolicyParams.pZslPluginParams).c_str());
        }
        else {
            MY_LOGD("(Plugin) set zslPluginParams=%s", ZslPolicy::toString(out->zslPolicyParams.pZslPluginParams).c_str());
        }
    }
    else {
        MY_LOGD("not support Zsl due to (isFlashOn:%d, isManual3A:%d, isZslModeOn:%d, isZslFlowOn:%d)",
            parsedInfo.isFlashOn, parsedInfo.isAppManual3A, parsedInfo.isZslModeOn, parsedInfo.isZslFlowOn);
        out->needZslFlow = false;
    }
    //
    if (parsedInfo.isCShot && !parsedInfo.isFastCapture) {
        out->bCshotRequest = true;
        // boot scenario for CShot.
        BoostControl boostControl;
        boostControl.boostScenario = IScenarioControlV3::Scenario_ContinuousShot;
        updateBoostControl(out, boostControl);
        MY_LOGD("update boostControl for CShot (boostScenario:0x%X, featureFlag:0x%X)",
                boostControl.boostScenario, boostControl.featureFlag);
    }

    MetadataPtr pOutMetaApp = nullptr;
    if(parsedInfo.isFastCapture) {
        MY_LOGD("request is FastCapture, will be seen as CShot");
        pOutMetaApp = std::make_shared<IMetadata>();
        IMetadata::setEntry<MINT32>(pOutMetaApp.get(), MTK_CSHOT_FEATURE_CAPTURE, MTK_CSHOT_FEATURE_CAPTURE_ON);
    }

    // update request result (frames metadata)
    updateRequestResultParams(
        out->mainFrame,
        pOutMetaApp,
        nullptr, /* no additional metadata from provider*/
        mainCamP1Dma,
        mReqMasterId,
        featureCombination);

    MY_LOGD_IF(mbDebug, "trigger single frame feature:%#" PRIx64", feature combination:%#" PRIx64"",
            combinedKeyFeature, featureCombination);
    return true;
}

auto
FeatureSettingPolicy::
dumpRequestOutputParams(
    RequestOutputParams* out,
    RequestInputParams const* in,
    bool forcedEnable = false
) -> bool
{
    // TODO: refactoring for following code
    if (CC_UNLIKELY(in->needP2CaptureNode || in->needRawOutput || forcedEnable)) {
        CAM_ULOGM_APILIFE();
        MY_LOGD("req#:%u, needP2S(%d), needP2C(%d), needRawOut(%d)", in->requestNo, in->needP2StreamNode, in->needP2CaptureNode, in->needRawOutput);
        // dump sensor mode
        for (const auto& [_sensorId, _mode] : out->sensorMode) {
            MY_LOGD("sensor(id:%d): sensorMode(%d)", _sensorId, _mode);
        }

        // dump boostControl setting
        for (auto& control : out->vboostControl) {
            MY_LOGD("current boostControl(size:%zu): boostScenario(0x%X), featureFlag(0x%X)",
                    out->vboostControl.size(), control.boostScenario, control.featureFlag);
        }

        // dump frames count and needUnpackRaw info
        MY_LOGD("request frames count(mainFrame:%d, subFrames:%zu, preDummyFrames:%zu, postDummyFrames:%zu), needUnpackRaw(%d)",
                (out->mainFrame.get() != nullptr), out->subFrames.size(),
                out->preDummyFrames.size(), out->postDummyFrames.size(),
                out->needUnpackRaw);

        // dump mainFrame
        if(out->mainFrame.get()) {
            //
            for (const auto& [_sensorId, _p1Dma] : out->mainFrame->needP1Dma) {
                MY_LOGD("mainFrame: needP1Dma, id:%d, value:0x%X", _sensorId, _p1Dma);
            }
            //
            if (CC_UNLIKELY(mbDebug)) {
                for (const auto& [_sensorId, _additionalHal] : out->mainFrame->additionalHal) {
                    MY_LOGD("mainFrame: dump addition hal metadata for sensor:%d, count:%u", _sensorId, _additionalHal->count());
                    _additionalHal->dump();

                    // dump MTK_FEATURE_CAPTURE/MTK_FEATURE_CAPTURE_PHYSICAL info for capture
                    if (CC_UNLIKELY(in->needP2CaptureNode || in->needRawOutput)) {
                        bool ret = false;
                        MINT64 featureCombination = 0;
                        if (mPolicyParams.bIsLogicalCam)
                            ret = IMetadata::getEntry<MINT64>(_additionalHal.get(), MTK_FEATURE_CAPTURE, featureCombination);
                        else
                            ret = IMetadata::getEntry<MINT64>(_additionalHal.get(), MTK_FEATURE_CAPTURE_PHYSICAL, featureCombination);
                        if (ret) {
                            MY_LOGD("mainFrame %s featureCombination=%#" PRIx64"",
                                    (mPolicyParams.bIsLogicalCam ? "MTK_FEATURE_CAPTURE" : "MTK_FEATURE_CAPTURE_PHYSICAL"),
                                    featureCombination);
                        }
                        else {
                            MY_LOGW("mainFrame w/o featureCombination");
                        }
                    }
                }
                MY_LOGD("dump addition app metadata");
                out->mainFrame->additionalApp->dump();
            }
        }
        else
        {
            MY_LOGW("failed to get main frame");
        }

        // dump subFrames
        if (CC_UNLIKELY(mbDebug)) {
            for (size_t i = 0; i < out->subFrames.size(); i++) {
                auto subFrame = out->subFrames[i];
                if (subFrame.get()) {
                    //
                    for (const auto& [_sensorId, _p1Dma] : subFrame->needP1Dma) {
                        MY_LOGD("subFrame[%zu]: needP1Dma, id:%d, value:0x%X", i, _sensorId, _p1Dma);
                    }
                    //
                    for (const auto& [_sensorId, _additionalHal] : subFrame->additionalHal) {
                        MY_LOGD("subFrame[%zu]: dump addition hal metadata for id:%d, count:%u", i, _sensorId, _additionalHal->count());
                        _additionalHal->dump();
                        // dump MTK_FEATURE_CAPTURE/MTK_FEATURE_CAPTURE_PHYSICAL info for capture
                        if (CC_UNLIKELY(in->needP2CaptureNode || in->needRawOutput)) {
                            bool ret = false;
                            MINT64 featureCombination = 0;
                            if (mPolicyParams.bIsLogicalCam)
                                ret = IMetadata::getEntry<MINT64>(_additionalHal.get(), MTK_FEATURE_CAPTURE, featureCombination);
                            else
                                ret = IMetadata::getEntry<MINT64>(_additionalHal.get(), MTK_FEATURE_CAPTURE_PHYSICAL, featureCombination);
                            if (ret) {
                                MY_LOGD("subFrame[%zu]: featureCombination=%#" PRIx64"", i, featureCombination);
                            }
                            else {
                                MY_LOGW("subFrame[%zu]: w/o featureCombination=%#" PRIx64"", i, featureCombination);
                            }
                        }
                    }
                    MY_LOGD("subFrame[%zu]: dump addition app metadata", i);
                    out->mainFrame->additionalApp->dump();
                }
            }
        }

        // reconfig & zsl info.
        MY_LOGD("needReconfiguration:%d, zsl(need:%d, zslPolicyParams:%s), drop previous preview request(%d), keepZslBuffer(%d)",
                out->needReconfiguration, out->needZslFlow, policy::toString(out->zslPolicyParams).c_str(),
                out->bDropPreviousPreviewRequest, out->keepZslBuffer);
    }
    return true;
}

auto
FeatureSettingPolicy::
updatePluginSelection(
    const uint32_t requestNo,
    bool isFeatureTrigger,
    uint8_t frameCount
) -> bool
{
    if (isFeatureTrigger) {
        mMFPPluginWrapperPtr->pushSelection(requestNo, frameCount);
        mRawPluginWrapperPtr->pushSelection(requestNo, frameCount);
    }
    else {
        mMFPPluginWrapperPtr->cancel();
        mRawPluginWrapperPtr->cancel();
    }

    return true;
}

auto
FeatureSettingPolicy::
updateMtkCaptureFeatureHint(
    MINT64 combinedKeyFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool
{
    MetadataPtr pOutMetaHal = std::make_shared<IMetadata>();
    (*pOutMetaHal.get()) += *(in->pNDDMetadata);

    // update key feature combination for strategy
    updateRequestResultParams(out->mainFrame, nullptr, pOutMetaHal, 0, mReqMasterId, combinedKeyFeature);

    return true;
}

auto
FeatureSettingPolicy::
updateCaptureVHDRInfo(
    RequestOutputParams* out
) -> void
{
    auto updateMetadata = [&](std::shared_ptr<RequestResultParams>& frame) {
        MetadataPtr pOutMetaHal = std::make_shared<IMetadata>();
        mCHDRHelper.at(mReqMasterId)->updateMetadata(
                pOutMetaHal,
                frame->additionalHal.at(mReqMasterId),
                out->needZslFlow);
        updateRequestResultParams(
                frame,
                nullptr, /* no additional metadata from provider*/
                pOutMetaHal,
                0,
                mReqMasterId);
    };

    updateMetadata(out->mainFrame);
    for (auto& preSubFrame : out->preSubFrames) {
        updateMetadata(preSubFrame);
    }
    for (auto& subFrame : out->subFrames) {
        updateMetadata(subFrame);
    }
    for (auto& preDummyFrame : out->preDummyFrames) {
        updateMetadata(preDummyFrame);
    }
    for (auto& postDummyFrame : out->postDummyFrames) {
        updateMetadata(postDummyFrame);
    }
}

auto
FeatureSettingPolicy::
writeScenarioRecorderResult(
    const int32_t requestNo,
    const uint32_t sensorId,
    const char* log,
    const bool toHealline
) -> void
{
    if (mScEnable && log != nullptr) {
        ResultParam param;
        //param.dbgNumInfo.uniquekey = -1; // no uniquekey yet
        //param.dbgNumInfo.magicNum = -1;  // no magic number yet
        param.dbgNumInfo.reqNum = requestNo;
        param.decisionType = DECISION_FEATURE;
        param.moduleId = NSCam::Utils::ULog::MOD_FEATURE_SETTING_POLICY;
        param.sensorId = sensorId;
        param.isCapture = true;
        param.writeToHeadline = toHealline;
        WRITE_EXECUTION_RESULT_INTO_FILE(param, "%s", log);
    }
}

auto
FeatureSettingPolicy::
strategyCaptureFeature(
    MINT64 combinedKeyFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    MINT64 featureCombination, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in
) -> bool
{
    CAM_ULOGM_APILIFE();
    MY_LOGD("strategy for combined key feature(%#" PRIx64"), feature combination(%#" PRIx64")",
            combinedKeyFeature, featureCombination);
    if (CC_UNLIKELY(out == nullptr)) {
        MY_LOGE("out is nullptr");
        return false;
    }

    if (CC_UNLIKELY(mForcedKeyFeatures >= 0)) {
        combinedKeyFeature = mForcedKeyFeatures;
        MY_LOGW("forced key feature(%#" PRIx64")", combinedKeyFeature);
    }
    if (CC_UNLIKELY(mForcedFeatureCombination >= 0)) {
        featureCombination = mForcedFeatureCombination;
        MY_LOGW("forced feature combination(%#" PRIx64")", featureCombination);
    }

    RequestOutputParams temp_out = {};
    temp_out.zslPolicyParams = {};
    if (out->mainFrame.get()) {
        MY_LOGI("clear previous invalid frames setting");
        out->mainFrame = nullptr;
        out->subFrames.clear();
        out->preDummyFrames.clear();
        out->postDummyFrames.clear();
    }
    temp_out = *out;
    //
    MINT64 foundFeature = 0;
    //
    if (parsedInfo.isZslModeOn && parsedInfo.isZslFlowOn) {
        // Set ZSL decison default value by App ZSL config/request flag,
        // but features may force to disable ZSL due to P1/3A exposure setting,
        // the final capability will be confirmed by features.
        temp_out.needZslFlow = true;
        MY_LOGD("App request ZSL(config mode:%d, request flow:%d) for capture", parsedInfo.isZslModeOn, parsedInfo.isZslFlowOn);
    }
    //
    updateMtkCaptureFeatureHint(combinedKeyFeature, &temp_out, in);
    //
    if (combinedKeyFeature) { /* not MTK_FEATURE_NORMAL */
        MINT64 checkFeatures = combinedKeyFeature;
        //
        do {
            if (!strategySingleRawPlugin(checkFeatures, featureCombination, foundFeature, &temp_out, parsedInfo, in)) {
                MY_LOGD("no need to trigger feature(%#" PRIx64") for features(key:%#" PRIx64", combined:%#" PRIx64")",
                        foundFeature, combinedKeyFeature, featureCombination);
                return false;
            }
            checkFeatures &= ~foundFeature;
        } while (foundFeature && checkFeatures); // to find next raw plugin until no foundfeature(==0)
        //
        MY_LOGD_IF(checkFeatures, "continue to find next plugin for %#" PRIx64"", checkFeatures);
        //
        if (!strategyMultiFramePlugin(checkFeatures, featureCombination, foundFeature, &temp_out, parsedInfo, in)) {
            MY_LOGD("no need to trigger feature(%#" PRIx64") for features(key:%#" PRIx64", combined:%#" PRIx64")",
                    foundFeature, combinedKeyFeature, featureCombination);
            return false;
        }
        checkFeatures &= ~foundFeature;
        //
        if (checkFeatures) {
            MY_LOGD("some key features(%#" PRIx64") still not found for features(%#" PRIx64")",
                    checkFeatures, combinedKeyFeature);
            return false;
        }
    }
    else {
        MY_LOGD("no combinated key feature, use default normal single capture");
    }
    // update basic requirement
    if (!strategyNormalSingleCapture(combinedKeyFeature, featureCombination, &temp_out, parsedInfo, in)) {
        MY_LOGW("update capture setting failed!");
        return false;
    }
    //
    if (parsedInfo.isCShot || parsedInfo.isYuvReprocess || parsedInfo.isRawReprocess) {
        MY_LOGD("no need dummy frames, isCShot(%d), isYuvReprocess(%d), isRawReprocess(%d)",
                parsedInfo.isCShot, parsedInfo.isYuvReprocess, parsedInfo.isRawReprocess);
    }
    else { // check and update dummy frames requirement for perfect 3A stable...
        updateCaptureDummyFrames(combinedKeyFeature, &temp_out, parsedInfo, in);
    }
    //
    updateCaptureVHDRInfo(&temp_out);
    //
    if (!updateCaptureDebugInfo(&temp_out, parsedInfo, in)) {
        MY_LOGW("updateCaptureDebugInfo failed!");
    }
    //
    if(mDualDevicePath == DualDevicePath::MultiCamControl) {
        MY_LOGD("multicam scenario");
        unordered_map<uint32_t, MetadataPtr> halMetaPtrList;
        multicamSyncMetadata(in, out, halMetaPtrList);

        if (temp_out.needZslFlow && mReqSubSensorIdList.size() > 0) {
            // if contains sub sensor(s), and need zsl flow.
            // set framesync flag to zsl policy.
            temp_out.zslPolicyParams.needFrameSync = true;
        }
        if (mReqSubSensorIdList.size() > 0 && mPolicyParams.bIsLogicalCam) { // dual cam
            if (!strategyDualCamPlugin(combinedKeyFeature, featureCombination, foundFeature, &temp_out, parsedInfo, in, halMetaPtrList)) {
                MY_LOGD("no need to trigger feature(%#" PRIx64") for features(key:%#" PRIx64", combined:%#" PRIx64")",
                        foundFeature, combinedKeyFeature, featureCombination);
                return false;
            }
        }
        else {
            MY_LOGD("capture id(%" PRIu32 ")", mReqMasterId);
            updateRequestResultParams(
                            temp_out.mainFrame,
                            nullptr,
                            halMetaPtrList[mReqMasterId],
                            0,
                            mReqMasterId,
                            featureCombination);
            auto updateSubFrame = [this,
                                   &halMetaPtrList,
                                   &featureCombination](
                                    std::vector<std::shared_ptr<RequestResultParams>>& subFrames,
                                    const MINT32 sensorId) {
                for(auto&& sub:subFrames) {
                    updateRequestResultParams(
                                    sub,
                                    nullptr,
                                    halMetaPtrList[mReqMasterId],
                                    0,
                                    sensorId,
                                    featureCombination);
                }
            };
            updateSubFrame(temp_out.subFrames, mReqMasterId);
            updateSubFrame(temp_out.preDummyFrames, mReqMasterId);
            updateSubFrame(temp_out.postDummyFrames, mReqMasterId);
        }
    }

    {
        // default boost control for all capture behavior.
        // TODO: the boost control will be handled by isp driver, and IScenarioControlV3 will be phased-out after ISP6.x
        BoostControl boostControl;
        boostControl.boostScenario = IScenarioControlV3::Scenario_ContinuousShot;
        updateBoostControl(out, boostControl);
        MY_LOGD("update boostControl for default capture behavior (boostScenario:0x%X, featureFlag:0x%X)",
                boostControl.boostScenario, boostControl.featureFlag);
    }

    // update result
    *out = temp_out;

    return true;
}

auto
FeatureSettingPolicy::
updateCaptureDummyFrames(
    MINT64 combinedKeyFeature, /*eFeatureIndexMtk and eFeatureIndexCustomer*/
    RequestOutputParams* out,
    const ParsedStrategyInfo& parsedInfo,
    RequestInputParams const* in
) -> void
{
    CAM_ULOGM_APILIFE();

    int8_t preDummyCount = 0;
    int8_t postDummyCount = 0;

    if (out->preDummyFrames.size() || out->postDummyFrames.size()) {
        MY_LOGI("feature(%#" PRIx64") has choose dummy frames(pre:%zu, post:%zu)",
                combinedKeyFeature, out->preDummyFrames.size(), out->postDummyFrames.size());

        {
            MY_LOGD("update hint for dummy frames");
            MetadataPtr pPreDummyApp_Additional = std::make_shared<IMetadata>();
            MetadataPtr pPreDummyHal_Additional = std::make_shared<IMetadata>();
            MetadataPtr pPostDummyApp_Additional = std::make_shared<IMetadata>();
            MetadataPtr pPostDummyHal_Additional = std::make_shared<IMetadata>();

            // update dummy frame hint for 3A behavior control
            // TODO: workaround, must update the hint to be dummy frames hint, no need to set 3A setting here
            IMetadata::setEntry<MBOOL>(pPreDummyApp_Additional.get(), MTK_3A_DUMMY_BEFORE_REQUEST_FRAME, 1);
            IMetadata::setEntry<MBOOL>(pPostDummyApp_Additional.get(), MTK_3A_DUMMY_AFTER_REQUEST_FRAME, 1);
            //
            MY_LOGD_IF(mbDebug, "update preDummyFrames size(%zu)", out->preDummyFrames.size());
            for (size_t i = 0; i < out->preDummyFrames.size(); i++) {
                auto& preDummyFrame = out->preDummyFrames[i];
                if (preDummyFrame.get()) {
                    MY_LOGD_IF(mbDebug, "preDummyFrameFrames[%zu] has existed(addr:%p)", i, preDummyFrame.get());
                    updateRequestResultParams(
                        preDummyFrame,
                        pPreDummyApp_Additional,
                        pPreDummyHal_Additional,
                        0,
                        mReqMasterId);
                }
                else {
                    MY_LOGW("preDummyFrames[%zu] is invalid", i);
                }
            }
            //
            MY_LOGD_IF(mbDebug, "update postDummyFrames size(%zu)", out->postDummyFrames.size());
            for (size_t i = 0; i < out->postDummyFrames.size(); i++) {
                auto& postDummyFrame = out->postDummyFrames[i];
                if (postDummyFrame.get()) {
                    MY_LOGD_IF(mbDebug, "postDummyFrameFrames[%zu] has existed(addr:%p)", i, postDummyFrame.get());
                    updateRequestResultParams(
                        postDummyFrame,
                        pPostDummyApp_Additional,
                        pPostDummyHal_Additional,
                        0,
                        mReqMasterId);
                }
                else {
                    MY_LOGW("postDummyFrames[%zu] is invalid", i);
                }
            }
        }

        return;
    }
    //
    auto hal3a = mHal3a.at(mReqMasterId);
    if (hal3a.get() == nullptr) {
        MY_LOGW("cannot get hal3a sensorId(%d), it is nullptr!", mReqMasterId);
        return;
    }

    // lambda for choose maximum count
    auto updateDummyCount = [&preDummyCount, &postDummyCount]
    (
        int8_t preCount,
        int8_t postCount
    ) -> void
    {
        preDummyCount = std::max(preDummyCount, preCount);
        postDummyCount = std::max(postDummyCount, postCount);
    };

    // lambda to check manual 3A
    auto isManual3aSetting = []
    (
        IMetadata const* pAppMeta,
        IMetadata const* pHalMeta
    ) -> bool
    {
        if (pAppMeta && pHalMeta) {
            // check manual AE (method.1)
            MUINT8 aeMode = MTK_CONTROL_AE_MODE_ON;
            if (IMetadata::getEntry<MUINT8>(pAppMeta, MTK_CONTROL_AE_MODE, aeMode)) {
                if (aeMode == MTK_CONTROL_AE_MODE_OFF) {
                    MY_LOGD("get MTK_CONTROL_AE_MODE(%d), it is manual AE", aeMode);
                    return true;
                }
            }
            // check manual AE (method.2)
            IMetadata::Memory capParams;
            capParams.resize(sizeof(CaptureParam_T));
            if (IMetadata::getEntry<IMetadata::Memory>(pHalMeta, MTK_3A_AE_CAP_PARAM, capParams)) {
                MY_LOGD("get MTK_3A_AE_CAP_PARAM, it is manual AE");
                return true;
            }
            // check manual WB
            MUINT8 awLock = MFALSE;
            if (IMetadata::getEntry<MUINT8>(pAppMeta, MTK_CONTROL_AWB_LOCK, awLock) && awLock) {
                MY_LOGD("get MTK_CONTROL_AWB_LOCK(%d), it is manual WB", awLock);
                return true;
            }
        }
        else {
            MY_LOGW("no metadata(app:%p, hal:%p) to query hint", pAppMeta, pHalMeta);
        }

        return false;
    };
    //
    bool bIsManual3A = false;
    if (CC_LIKELY(out->mainFrame.get())) {
        IMetadata const* pAppMeta = out->mainFrame->additionalApp.get();
        IMetadata const* pHalMeta = out->mainFrame->additionalHal.at(mReqMasterId).get();
        bIsManual3A = isManual3aSetting(pAppMeta, pHalMeta);
    }
    else {
        MY_LOGD("no metadata info due to no mainFrame");
    }
    //
    if (parsedInfo.ispTuningFrameCount > 0) {
        // Prevent 3rd party camera Apk from unstandard abnormal behavior,
        // only support dummy frames if App set manual AE with frame count index hint.
        MY_LOGD_IF(parsedInfo.isAppManual3A, "App set AE manaul(count:%d, index:%d)",
                   parsedInfo.ispTuningFrameCount, parsedInfo.ispTuningFrameIndex);
        bIsManual3A |= parsedInfo.isAppManual3A;
    }
    //
    if (bIsManual3A) {
        // get manual 3a delay frames count from 3a hal
        MUINT32 delayedFrames = 0;
        {
            std::lock_guard<std::mutex> _l(mHal3aLocker);
            hal3a->send3ACtrl(E3ACtrl_GetCaptureDelayFrame, reinterpret_cast<MINTPTR>(&delayedFrames), 0);
        }
        MY_LOGD("delayedFrames count:%d due to manual 3A", delayedFrames);
        //
        updateDummyCount(0, delayedFrames);
    }

    if (parsedInfo.isFlashOn) {
        rHAL3AFlashCapDummyInfo_T flashCapDummyInfo;
        rHAL3AFlashCapDummyCnt_T flashCapDummyCnt;
        flashCapDummyInfo.u4AeMode = parsedInfo.aeMode;
        flashCapDummyInfo.u4StrobeMode = parsedInfo.flashMode;
        flashCapDummyInfo.u4CaptureIntent = parsedInfo.captureIntent;
        hal3a->send3ACtrl(E3ACtrl_GetFlashCapDummyCnt, reinterpret_cast<MINTPTR>(&flashCapDummyInfo), reinterpret_cast<MINTPTR>(&flashCapDummyCnt));
        updateDummyCount(flashCapDummyCnt.u4CntBefore, flashCapDummyCnt.u4CntAfter);
        MY_LOGD("dummy frames count(pre:%d, post:%d) from Hal3A requirement, due to flash on(aeMode:%d, flashMode:%d, captureIntent:%d)",
                flashCapDummyCnt.u4CntBefore, flashCapDummyCnt.u4CntAfter,
                flashCapDummyInfo.u4AeMode, flashCapDummyInfo.u4StrobeMode, flashCapDummyInfo.u4CaptureIntent);
    }

    if (parsedInfo.ispTuningFrameCount > 0) {
        if (parsedInfo.ispTuningOutputFirstFrame == false) {
            preDummyCount = 0;
            MY_LOGD("only first frame need pre-dummy for 3A, update dummy frames count(pre:%d, post:%d) for this request(count:%d, index:%d)",
                    preDummyCount, postDummyCount, parsedInfo.ispTuningFrameCount, parsedInfo.ispTuningFrameIndex);
        }
        if (parsedInfo.ispTuningOutputLastFrame == false) {
            postDummyCount = 0;
            MY_LOGD("only last frame need post-dummy for 3A, update dummy frames count(pre:%d, post:%d) for this request(count:%d, index:%d)",
                    preDummyCount, postDummyCount, parsedInfo.ispTuningFrameCount, parsedInfo.ispTuningFrameIndex);
        }
    }

    MY_LOGD("dummy frames result(pre:%d, post:%d)", preDummyCount, postDummyCount);

    uint32_t camP1Dma = 0;
    if ( !getCaptureP1DmaConfig(camP1Dma, in, mReqMasterId) ){
        MY_LOGE("main P1Dma output is invalid: 0x%X", camP1Dma);
        return;
    }

    // update preDummyFrames
    for (MINT32 i = 0; i < preDummyCount; i++) {
        MetadataPtr pAppDummy_Additional = std::make_shared<IMetadata>();
        MetadataPtr pHalDummy_Additional = std::make_shared<IMetadata>();
        // update info for pre-dummy frames for flash/3a stable
        IMetadata::setEntry<MBOOL>(pHalDummy_Additional.get(), MTK_3A_DUMMY_BEFORE_REQUEST_FRAME, 1);
        //
        std::shared_ptr<RequestResultParams> preDummyFrame = nullptr;
        updateRequestResultParams(
                preDummyFrame,
                pAppDummy_Additional,
                pHalDummy_Additional,
                camP1Dma,
                mReqMasterId);
        //
        out->preDummyFrames.push_back(preDummyFrame);
    }

    // update postDummyFrames
    for (MINT32 i = 0; i < postDummyCount; i++) {
        MetadataPtr pAppDummy_Additional = std::make_shared<IMetadata>();
        MetadataPtr pHalDummy_Additional = std::make_shared<IMetadata>();
        // update info for post-dummy(delay) frames to restore 3A for preview stable
        IMetadata::setEntry<MBOOL>(pHalDummy_Additional.get(), MTK_3A_DUMMY_AFTER_REQUEST_FRAME, 1);
        //
        std::shared_ptr<RequestResultParams> postDummyFrame = nullptr;
        updateRequestResultParams(
                postDummyFrame,
                pAppDummy_Additional,
                pHalDummy_Additional,
                camP1Dma,
                mReqMasterId);
        //
        out->postDummyFrames.push_back(postDummyFrame);
    }

    // check result
    if (out->preDummyFrames.size() || out->postDummyFrames.size()) {
        MY_LOGI("feature(%#" PRIx64") append dummy frames(pre:%zu, post:%zu) due to isFlashOn(%d), isManual3A(%d)",
                combinedKeyFeature, out->preDummyFrames.size(), out->postDummyFrames.size(),
                parsedInfo.isFlashOn, bIsManual3A);

        if (out->needZslFlow) {
            MY_LOGW("not support Zsl buffer due to isFlashOn(%d) or isManual3A(%d)", parsedInfo.isFlashOn, bIsManual3A);
            out->needZslFlow = false;
        }
    }

    return;
}

auto
FeatureSettingPolicy::
toTPIDualHint(
    ScenarioHint &hint
) -> void
{
    hint.isDualCam = mPolicyParams.pPipelineStaticInfo->isDualDevice;
    hint.dualDevicePath = static_cast<int32_t>(mDualDevicePath);
    hint.multiCamFeatureMode = miMultiCamFeatureMode;
    int stereoMode = StereoSettingProvider::getStereoFeatureMode();
    if( hint.isDualCam )
    {
        if( stereoMode & v1::Stereo::E_STEREO_FEATURE_VSDOF )
            hint.mDualFeatureMode = DualFeatureMode_MTK_VSDOF;
        else if( stereoMode & v1::Stereo::E_STEREO_FEATURE_MTK_DEPTHMAP )
            hint.mDualFeatureMode = DualFeatureMode_HW_DEPTH;
        else if( stereoMode & v1::Stereo::E_STEREO_FEATURE_THIRD_PARTY )
            hint.mDualFeatureMode = DualFeatureMode_YUV;
        else if( stereoMode & v1::Stereo::E_DUALCAM_FEATURE_ZOOM)
            hint.mDualFeatureMode = DualFeatureMode_MTK_FOVA;
        else
            hint.mDualFeatureMode = DualFeatureMode_NONE;
    }
}

auto
FeatureSettingPolicy::
updateMulticamSensorControl(
    RequestInputParams const* in,
    uint32_t const& sensorId,
    MetadataPtr pHalMeta,
    uint32_t &dma
) -> void
{
    if(in == nullptr) {
        MY_LOGE("RequestInputParams is nullptr");
        return;
    }
    if(in->pMultiCamReqOutputParams == nullptr) {
        MY_LOGE("pMultiCamReqOutputParams is nullptr");
        return;
    }
    if(pHalMeta == nullptr)
    {
        MY_LOGE("pHalMeta is nullptr");
        return;
    }
    bool exist = false;
    // 1. check go to standby control
    {
        // if sensor wants to go to standby, it has to add MTK_P1_SENSOR_STATUS_HW_STANDBY,
        // to hal metadata tag.
        if(std::find(
                    in->pMultiCamReqOutputParams->goToStandbySensorList.begin(),
                    in->pMultiCamReqOutputParams->goToStandbySensorList.end(),
                    sensorId) != in->pMultiCamReqOutputParams->goToStandbySensorList.end())
        {
            // exist in go to standby list.
            exist = true;
            IMetadata::setEntry<MINT32>(
                                        pHalMeta.get(),
                                        MTK_P1NODE_SENSOR_STATUS,
                                        MTK_P1_SENSOR_STATUS_HW_STANDBY);
            MY_LOGI("req(%" PRIu32 ") sensorid(%" PRIu32 ") set standby flag",
                                    in->requestNo, sensorId);
        }
    }
    // 2. check resume control
    {
        // if sensor wants to resume, it has to add MTK_P1_SENSOR_STATUS_STREAMING,
        // to hal metadata tag.
        if(std::find(
                    in->pMultiCamReqOutputParams->resumeSensorList.begin(),
                    in->pMultiCamReqOutputParams->resumeSensorList.end(),
                    sensorId) != in->pMultiCamReqOutputParams->resumeSensorList.end())
        {
            // exist in resume standby list.
            exist = true;
            IMetadata::setEntry<MINT32>(
                                        pHalMeta.get(),
                                        MTK_P1NODE_SENSOR_STATUS,
                                        MTK_P1_SENSOR_STATUS_STREAMING);
            MY_LOGI("req(%" PRIu32 ") sensorid(%" PRIu32 ") set resume flag",
                                    in->requestNo, sensorId);
        }
    }
    // 3. check continuous standby
    {
        // if sensor is already in standby mode, it has to set dma to 0.
        // otherwise, IOMap will request P1 to output image. And, it will
        // cause preview freeze.
        if(std::find(
                    in->pMultiCamReqOutputParams->standbySensorList.begin(),
                    in->pMultiCamReqOutputParams->standbySensorList.end(),
                    sensorId) != in->pMultiCamReqOutputParams->standbySensorList.end())
        {
            exist = true;
            dma = 0;
        }
    }
    // 4. check MarkErrorSensorList
    {
        // if sensor id is exist in MarkErrorSensorList, it means no needs to output image
        // and no need output p1 dma buffer
        if(std::find(
                    in->pMultiCamReqOutputParams->markErrorSensorList.begin(),
                    in->pMultiCamReqOutputParams->markErrorSensorList.end(),
                    sensorId) != in->pMultiCamReqOutputParams->markErrorSensorList.end())
        {
            exist = true;
            dma = 0;
        }
    }
    // 4. check is streaming
    {
        if(std::find(
                    in->pMultiCamReqOutputParams->streamingSensorList.begin(),
                    in->pMultiCamReqOutputParams->streamingSensorList.end(),
                    sensorId) != in->pMultiCamReqOutputParams->streamingSensorList.end())
        {
            exist = true;
            if (dma == 0)
            {
                uint32_t dma = P1_IMGO;
                dma |= P1_RRZO;
            }
            MY_LOGI("req(%" PRIu32 ") sensorid(%" PRIu32 ") is in streamingSensorList",
                                    in->requestNo, sensorId);
        }
    }
    if(!exist)
    {
        // if sensor id not exist in above list, it means current sensor is not active.
        dma = 0;
    }
    return;
}

auto
FeatureSettingPolicy::
queryMulticamTgCrop(
    RequestInputParams const* in,
    uint32_t const& sensorId,
    MRect &tgCrop,
    bool &isCapture,
    bool is3A
) -> bool
{
    bool ret = false;
    if(in == nullptr || in->pMultiCamReqOutputParams == nullptr) {
        return ret;
    }
    if(isCapture)
    {
        // tg crop
        auto iter = in->pMultiCamReqOutputParams->tgCropRegionList_Cap.find(sensorId);
        if(iter != in->pMultiCamReqOutputParams->tgCropRegionList_Cap.end()) {
            tgCrop = iter->second;
            ret = true;
        }
    }
    else
    {
        // tg crop
        if(is3A)
        {
            auto iter = in->pMultiCamReqOutputParams->tgCropRegionList_3A.find(sensorId);
            if(iter != in->pMultiCamReqOutputParams->tgCropRegionList_3A.end()) {
                tgCrop = iter->second;
                ret = true;
            }
        }
        else
        {
            auto iter = in->pMultiCamReqOutputParams->tgCropRegionList.find(sensorId);
            if(iter != in->pMultiCamReqOutputParams->tgCropRegionList.end()) {
                tgCrop = iter->second;
                ret = true;
            }
        }
    }
    return ret;
}

auto
FeatureSettingPolicy::
queryMulticamScalerCrop(
    RequestInputParams const* in,
    uint32_t const& sensorId,
    MRect &scalerCrop,
    bool &isCapture
) -> bool
{
    bool ret = false;
    if(in == nullptr || in->pMultiCamReqOutputParams == nullptr) {
        return ret;
    }
    if(isCapture)
    {
        // scaler crop
        auto iter = in->pMultiCamReqOutputParams->sensorScalerCropRegionList_Cap.find(sensorId);
        if(iter != in->pMultiCamReqOutputParams->sensorScalerCropRegionList_Cap.end()) {
            scalerCrop = iter->second;
            ret = true;
        }
    }
    else
    {
        // scaler crop
        auto iter = in->pMultiCamReqOutputParams->sensorScalerCropRegionList.find(sensorId);
        if(iter != in->pMultiCamReqOutputParams->sensorScalerCropRegionList.end()) {
            scalerCrop = iter->second;
            ret = true;
        }
    }
    return ret;
}

auto
FeatureSettingPolicy::
queryMulticam3aRegion(
    RequestInputParams const* in,
    uint32_t const& sensorId,
    std::vector<MINT32> &mrafRegion,
    std::vector<MINT32> &mraeRegion,
    std::vector<MINT32> &mrawbRegion
) -> bool
{
    bool ret = false;
    if(in == nullptr || in->pMultiCamReqOutputParams == nullptr) {
        return ret;
    }
    //
    ret = true;
    auto iter_ = in->pMultiCamReqOutputParams->sensorAfRegionList.find(sensorId);
    if(iter_ != in->pMultiCamReqOutputParams->sensorAfRegionList.end()) {
        mrafRegion = iter_->second;
    }else{
        ret = false;
    }
    auto iter_1 = in->pMultiCamReqOutputParams->sensorAeRegionList.find(sensorId);
    if(iter_1 != in->pMultiCamReqOutputParams->sensorAeRegionList.end()) {
        mraeRegion = iter_1->second;
    }else{
        ret = false;
    }
    auto iter_2 = in->pMultiCamReqOutputParams->sensorAwbRegionList.find(sensorId);
    if(iter_2 != in->pMultiCamReqOutputParams->sensorAwbRegionList.end()) {
        mrawbRegion = iter_2->second;
    }else{
        ret = false;
    }
    return ret;
}

bool
FeatureSettingPolicy::
isPhysicalStreamUpdate()
{
    // for physical stream update
    // Physical setting:
    // isDualDevice is false, but contain feature mode.
    bool ret = false;
    auto& pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    if (pParsedMultiCamInfo != nullptr &&
        (!mPolicyParams.pPipelineStaticInfo->isDualDevice) &&
        (DualDevicePath::MultiCamControl == pParsedMultiCamInfo->mDualDevicePath)&&
        (-1 != pParsedMultiCamInfo->mDualFeatureMode)) {
        ret = true;
    }
    return ret;
}

auto
FeatureSettingPolicy::
getStreamDuration(
    const IMetadata::IEntry& entry,
    const MINT64 format,
    const MSize& size,
    MINT64& duration
) -> bool
{
    if(entry.isEmpty()) {
        MY_LOGW("Static meta : MTK_SCALER_AVAILABLE_MIN_FRAME_DURATIONS entry is empty!");
        return false;
    }
    else
    {
        for (size_t i = 0, count = entry.count(); i + 3 < count; i += 4) {
            if (entry.itemAt(i    , Type2Type<MINT64>()) == format &&
                entry.itemAt(i + 1, Type2Type<MINT64>()) == (MINT64)size.w &&
                entry.itemAt(i + 2, Type2Type<MINT64>()) == (MINT64)size.h)
            {
                duration = entry.itemAt(i + 3, Type2Type<MINT64>());
                return true;
            }
        }
    }
    return false;
}

bool
FeatureSettingPolicy::
multicamSyncMetadata(
    RequestInputParams const* in,
    RequestOutputParams* out,
    unordered_map<uint32_t, MetadataPtr>& halMetaPtrList
)
{
    auto const& sensorIdList = mPolicyParams.pPipelineStaticInfo->sensorId;
    auto const& vStreamingSensorList = in->pMultiCamReqOutputParams->prvStreamingSensorList;
    auto updateSensorCrop = [&in, this](
                            int32_t sensorId,
                            IMetadata* metadata)
    {
        bool isVSDoF3rdParty = (StereoSettingProvider::getMtkDepthFlow()==1) ? true: false;
        if(miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF && !isVSDoF3rdParty)
        {
            // vsdof crop info is compute by StereSizeProvider.
            MRect crop;
            queryVsdofSensorCropRect(sensorId, in, crop);
            IMetadata::setEntry<MRect>(metadata, MTK_P1NODE_SENSOR_CROP_REGION, crop);
            IMetadata::setEntry<MRect>(metadata, MTK_DUALZOOM_CAP_CROP, crop);
            MY_LOGD("b_sc(%d, %d, %dx%d)", crop.p.x, crop.p.y, crop.s.w, crop.s.h);
            MRect crop3A;
            queryVsdof3APreviewCropRect(sensorId, in, crop3A);
            IMetadata::setEntry<MRect>(metadata, MTK_3A_PRV_CROP_REGION, crop3A);
            MY_LOGD("3a b_sc(%d, %d, %dx%d)", crop3A.p.x, crop3A.p.y, crop3A.s.w, crop3A.s.h);
        }
        else
        {
            MRect tgCrop_prv, tgCrop_cap, scalerCrop;
            bool isCapture = true;
            bool isPreview = !isCapture;
            if(queryMulticamTgCrop(in, sensorId, tgCrop_prv, isPreview))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_P1NODE_SENSOR_CROP_REGION, tgCrop_prv);
                IMetadata::setEntry<MRect>(metadata, MTK_3A_PRV_CROP_REGION, tgCrop_prv);
            }
            if(queryMulticamTgCrop(in, sensorId, tgCrop_cap, isCapture))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_DUALZOOM_CAP_CROP, tgCrop_cap);
            }
            if(queryMulticamScalerCrop(in, sensorId, scalerCrop, isCapture))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_SENSOR_SCALER_CROP_REGION, scalerCrop);
            }
            MY_LOGD("sensorId(%d) tg(%d, %d, %dx%d) tg_cap(%d, %d, %dx%d) z_sc(%d, %d, %dx%d)",
                                sensorId,
                                tgCrop_prv.p.x, tgCrop_prv.p.y, tgCrop_prv.s.w, tgCrop_prv.s.h,
                                tgCrop_cap.p.x, tgCrop_cap.p.y, tgCrop_cap.s.w, tgCrop_cap.s.h,
                                scalerCrop.p.x, scalerCrop.p.y, scalerCrop.s.w, scalerCrop.s.h);
        }
        return true;
    };

    // init halMetaPtrList
    for(auto sensorId:sensorIdList)
    {
        halMetaPtrList.insert({sensorId, std::make_shared<IMetadata>()});
    }
    // add crop
    for (auto&& sensorId : vStreamingSensorList)
    {
        auto iter = halMetaPtrList.find(sensorId);
        if(iter != halMetaPtrList.end() && iter->second != nullptr)
        {
            MY_LOGI("crop update sensorId(%d)", sensorId);
            // update sensor crop
            if(!out->needZslFlow)
                updateSensorCrop(sensorId, halMetaPtrList[sensorId].get());
        }
    }
    // for FD
    for (auto&& sensorId : vStreamingSensorList)
    {
        auto iter = halMetaPtrList.find(sensorId);
        if(iter != halMetaPtrList.end() && iter->second != nullptr)
        {
            IMetadata::setEntry<MINT32>(iter->second.get(), MTK_DUALZOOM_FD_TARGET_MASTER,
                                                (MINT32)in->pMultiCamReqOutputParams->prvMasterId);
        }
    }
    return true;
}

auto
FeatureSettingPolicy::
updateBoostControl(
    RequestOutputParams* out,
    const BoostControl& boostControl
) -> bool
{
    bool bUpdated = false;
    bool ret = true;

    if (boostControl.boostScenario != IScenarioControlV3::Scenario_None) { // check valid
        for (auto& control : out->vboostControl) {
            if (control.boostScenario == boostControl.boostScenario) {
                int32_t originalFeatureFlag = control.featureFlag;
                control.featureFlag |= boostControl.featureFlag;
                MY_LOGD("update boostControl(size:%zu): boostScenario(0x%X), featureFlag(0x%X|0x%X => 0x%X)",
                        out->vboostControl.size(), control.boostScenario, originalFeatureFlag, boostControl.featureFlag, control.featureFlag);
                bUpdated = true;
            }
            else {
                MY_LOGD_IF(mbDebug, "current boostControl(size:%zu): boostScenario(0x%X), featureFlag(0x%X)",
                        out->vboostControl.size(), control.boostScenario, control.featureFlag);
            }
        }
        //
        if (!bUpdated) {
            out->vboostControl.push_back(boostControl);
            MY_LOGD("add boostControl(size:%zu): boostScenario(0x%X), featureFlag(0x%X)",
                    out->vboostControl.size(), boostControl.boostScenario, boostControl.featureFlag);
        }
        ret = true;
    }
    else {
        if (boostControl.featureFlag != IScenarioControlV3::FEATURE_NONE) {
            MY_LOGW("invalid boostControl(boostScenario:0x%X, featureFlag:0x%X)",
                    boostControl.boostScenario, boostControl.featureFlag);
            ret = false;
        }
        else {
            MY_LOGD_IF(mbDebug, "no need to update boostControl(boostScenario:0x%X, featureFlag:0x%X)",
                    boostControl.boostScenario, boostControl.featureFlag);
            ret = true;
        }
    }

    return ret;
}

auto
FeatureSettingPolicy::
evaluateCaptureSetting(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool
{
    if (in->pMultiCamReqOutputParams != nullptr) {
        if (!checkNeedUpdateSetting(in->pMultiCamReqOutputParams->prvStreamingSensorList)) {
            // ignore update.
            return true;
        }
    }
    if (in->needP2CaptureNode || in->needRawOutput) {

        CAM_TRACE_CALL();
        CAM_ULOGM_APILIFE();

        MY_LOGI("(%p) capture req#:%u", this, in->requestNo);

        bool isCaptureUpdate = true;
        decideReqMasterAndSubSensorlist(in, mPolicyParams, isCaptureUpdate);

        ParsedStrategyInfo parsedInfo;
        if (!collectParsedStrategyCaptureInfo(out, parsedInfo, in)) {
            MY_LOGE("collectParsedStrategyInfo failed!");
            return false;
        }

        // AF rqeuest for ZSD Capture Intent
        auto hal3a = mHal3a.at(mReqMasterId);
        if (hal3a.get()) {
            std::lock_guard<std::mutex> _l(mHal3aLocker);
            hal3a->send3ACtrl(E3ACtrl_MTKCaptureHint, 0, 0);
        } else {
            MY_LOGF("MAKE_Hal3A failed sensorId(%d), it nullptr", mReqMasterId);
        }

        ScenarioFeatures scenarioFeatures;
        CaptureScenarioConfig scenarioConfig;
        ScenarioHint scenarioHint;
        toTPIDualHint(scenarioHint);
        scenarioHint.operationMode = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->operationMode;
        scenarioHint.isVideoMode = mPolicyParams.pPipelineUserConfiguration->pParsedAppImageStreamInfo->hasVideoConsumer;
        scenarioHint.isSuperNightMode = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->isSuperNightMode;
        scenarioHint.isIspHidlTuningEnable = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->hasTuningEnable;
        scenarioHint.isCShot = parsedInfo.isCShot;
        scenarioHint.isYuvReprocess = parsedInfo.isYuvReprocess;
        scenarioHint.isRawReprocess = parsedInfo.isRawReprocess;
        scenarioHint.isYuvRequestForIspHidl = parsedInfo.isYuvRequestForIspHidl;
        scenarioHint.isRawRequestForIspHidl = parsedInfo.isRawRequestForIspHidl;
        scenarioHint.ispTuningHint = parsedInfo.ispTuningHint;
        scenarioHint.isLogical = mPolicyParams.bIsLogicalCam;
        if (in->needRawOutput && scenarioHint.isRawRequestForIspHidl == false) {
            // App request Dng or Opaue
            scenarioHint.isRawOrDngRequest = true;
        }
        //TODO:
        //scenarioHint.captureScenarioIndex = ? /* hint from vendor tag */
        int32_t openId = mPolicyParams.pPipelineStaticInfo->openId;
        auto pAppMetadata = in->pRequest_AppControl;

        // Decide the master and slave sensor id of the stream
        if ((mDualDevicePath == DualDevicePath::MultiCamControl) && !mPolicyParams.bIsLogicalCam) {
            if (in->pMultiCamReqOutputParams != nullptr) {
                MINT32 masterId = (MINT32)in->pMultiCamReqOutputParams->prvMasterId;
                if (openId == masterId)
                    scenarioHint.isMaster = MTRUE;
                else
                    scenarioHint.isMaster = MFALSE;
            }
        }

        MY_LOGD("openId(%d), sensorId(%d), subsensorId(%d), sub sensor nums %zu",
                 openId,
                 mReqMasterId,
                 (mReqSubSensorIdList.size() > 0) ? mReqSubSensorIdList[0] : -1,
                 mReqSubSensorIdList.size());

        int32_t scenario = -1;
        if (!get_capture_scenario(scenario, scenarioFeatures, scenarioConfig, scenarioHint, pAppMetadata)) {
            MY_LOGE("cannot get capture scenario");
            return false;
        }
        else {
            MY_LOGD("find scenario:%s for (openId:%d, scenario:%d)",
                    scenarioFeatures.scenarioName.c_str(), openId, scenario);
        }

        if (CC_UNLIKELY(scenarioConfig.needUnpackRaw == true)) {
            bool originalNeedUnpackRaw = out->needUnpackRaw;
            out->needUnpackRaw |= scenarioConfig.needUnpackRaw;
            MY_LOGI("set needUnpackRaw(%d|%d => %d) for scenario:%s",
                    originalNeedUnpackRaw, scenarioConfig.needUnpackRaw, out->needUnpackRaw, scenarioFeatures.scenarioName.c_str());
        }

        bool isFeatureTrigger = false;
        for (auto &featureSet : scenarioFeatures.vFeatureSet) {
            // evaluate key feature plugin and feature combination for feature strategy policy.
            if (strategyCaptureFeature( featureSet.feature, featureSet.featureCombination, out, parsedInfo, in )) {
                isFeatureTrigger = true;
                uint8_t frameCount = out->subFrames.size() + 1;
                MY_LOGI("trigger feature:%s(%#" PRIx64"), feature combination:%s(%#" PRIx64") for req#%u",
                        featureSet.featureName.c_str(),
                        static_cast<MINT64>(featureSet.feature),
                        featureSet.featureCombinationName.c_str(),
                        static_cast<MINT64>(featureSet.featureCombination),
                        in->requestNo);
                updatePluginSelection(in->requestNo, isFeatureTrigger, frameCount);

                WRITE_SCENARIO_RESULT_HEADLINE(
                    in->requestNo, mReqMasterId,
                    "trigger feature:%s, feature combination:%s",
                    featureSet.featureName.c_str(),
                    featureSet.featureCombinationName.c_str());
                break;
            }
            else{
                isFeatureTrigger = false;
                MY_LOGD("no need to trigger feature:%s(%#" PRIx64"), feature combination:%s(%#" PRIx64")",
                        featureSet.featureName.c_str(),
                        static_cast<MINT64>(featureSet.feature),
                        featureSet.featureCombinationName.c_str(),
                        static_cast<MINT64>(featureSet.featureCombination));
                updatePluginSelection(in->requestNo, isFeatureTrigger);

                WRITE_SCENARIO_RESULT_DEFAULT(
                    in->requestNo, mReqMasterId,
                    "no need to trigger feature:%s, feature combination:%s",
                    featureSet.featureName.c_str(),
                    featureSet.featureCombinationName.c_str());
            }
        }

        if (!isFeatureTrigger) {
            MY_LOGE("no feature can be triggered!");
            return false;
        }

        // No need to keep buffer in zsl
        out->keepZslBuffer = false;

        MY_LOGD("capture request frames count(mainFrame:%d, subFrames:%zu, preDummyFrames:%zu, postDummyFrames:%zu)",
                (out->mainFrame.get() != nullptr), out->subFrames.size(),
                out->preDummyFrames.size(), out->postDummyFrames.size());
    }
    return true;
}

auto
FeatureSettingPolicy::
evaluateReconfiguration(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> bool
{
    CAM_TRACE_CALL();
    //CAM_ULOGM_APILIFE();
    out->needReconfiguration = false;
    out->reconfigCategory = ReCfgCtg::NO;
    for (const auto& [_sensorId, _mode] : in->sensorMode) {
        if (_mode != out->sensorMode.at(_sensorId)) {
            MY_LOGD("sensorId:%d sensorMode(%d --> %d) is changed",
                _sensorId, _mode, out->sensorMode.at(_sensorId));
            out->needReconfiguration = true;
            out->reconfigCategory = ReCfgCtg::HIGH_RES_REMOSAIC;
        }

        MINT32 forceReconfig = ::property_get_bool("vendor.debug.camera.hal3.pure.reconfig.test", -1);
        if(forceReconfig == 1){
            out->needReconfiguration = true;
            out->reconfigCategory = ReCfgCtg::STREAMING;
        }
        else if(forceReconfig == 0){
            out->needReconfiguration = false;
            out->reconfigCategory = ReCfgCtg::NO;
        }

        // sensor mode is not the same as preview default (cannot execute zsl)
        if (out->needReconfiguration == true ||
            mDefaultConfig.sensorMode.at(_sensorId) != out->sensorMode.at(_sensorId)) {
            out->needZslFlow = false;
            MY_LOGD("must reconfiguration, capture new frames w/o zsl flow");
        }
    }

    for (const auto& [_sensorId, _hdrHelper] : mHDRHelper) {
        if (_hdrHelper->needReconfiguration()) {
            out->needReconfiguration = true;
            out->reconfigCategory = ReCfgCtg::STREAMING;
            if (!_hdrHelper->handleReconfiguration()) {
                MY_LOGE("HDR(%d) handleReconfiguration failed(Category=%hhu)", _sensorId, out->reconfigCategory);
            }
        }
        if (in->needP2CaptureNode && _hdrHelper->isVideoHDRConfig()
                && !(_hdrHelper->isMulitFrameHDR() || _hdrHelper->isStaggerHDR())) {
            // check if VHDR is on or configured, reconfigure the sensor mode
            // if current mode is Mstream HDR, it doesn't need reconfiguration
            out->needReconfiguration = true;
            out->reconfigCategory = ReCfgCtg::CAPTURE;
            MY_LOGD("HDR(%d) need reconfiguration, capture new frames w/o vhdr", _sensorId);
        }
    }

    // zsl policy debug
    MY_LOGD_IF(out->needZslFlow, "needZslFlow:%d, zslPolicyParams:%s",
            out->needZslFlow, policy::toString(out->zslPolicyParams).c_str());

    return true;
}

auto
FeatureSettingPolicy::
getCaptureProvidersByScenarioFeatures(
    ConfigurationOutputParams* out __unused,
    ConfigurationInputParams const* in __unused
) -> bool
{
    CAM_ULOGM_APILIFE();

    auto supportedFeatures = out->CaptureParams.supportedScenarioFeatures;
    auto pluginUniqueKey = in->uniqueKey;
    auto isPhysical = !mPolicyParams.bIsLogicalCam;
    MY_LOGI("support features:%#" PRIx64 " uniqueKey:%d isPhysical %d", supportedFeatures, pluginUniqueKey, isPhysical);
    //
    mMFPPluginWrapperPtr = std::make_shared<MFPPluginWrapper>(supportedFeatures, pluginUniqueKey, isPhysical);
    mRawPluginWrapperPtr = std::make_shared<RawPluginWrapper>(supportedFeatures, pluginUniqueKey, isPhysical);
    mYuvPluginWrapperPtr = std::make_shared<YuvPluginWrapper>(supportedFeatures, pluginUniqueKey, isPhysical);
    if (mPolicyParams.pPipelineStaticInfo->isDualDevice) {
        MY_LOGD("current multicam feature mode (%d)", miMultiCamFeatureMode);
        if(miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF)
        {
            mBokehPluginWraperPtr = std::make_shared<BokehPluginWraper>(supportedFeatures, pluginUniqueKey, isPhysical);
            mDepthPluginWraperPtr = std::make_shared<DepthPluginWraper>(supportedFeatures, pluginUniqueKey, isPhysical);
            mFusionPluginWraperPtr = std::make_shared<FusionPluginWraper>(supportedFeatures, pluginUniqueKey, isPhysical);
            // query preview process main2 period.
            // if value is 1, it means each main2 frame will send to p2 streaming node.
            // if value is 2, it means 2 main2 frames will send 1 frame to streaming node.
            mPeriodForStreamingProcessDepth = StereoSettingProvider::getMain2OutputFrequency();
            mPeriodForStreamingProcessDepth = (MUINT32)::property_get_int32("vendor.debug.camera.depthperiod", (MINT32)mPeriodForStreamingProcessDepth);
            if(mPeriodForStreamingProcessDepth == 0) mPeriodForStreamingProcessDepth = 1;
            MY_LOGD("mPeriodForStreamingProcessDepth(%d)", mPeriodForStreamingProcessDepth);
        }
        else if (miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM) {
            mFusionPluginWraperPtr = std::make_shared<FusionPluginWraper>(supportedFeatures, pluginUniqueKey, isPhysical);
            MY_LOGD("zoom scenario");
        }
        else
        {
            MY_LOGD("multicam scenario");
        }
    }
    // query sensor count
    mSensorCount = mPolicyParams.pPipelineStaticInfo->sensorId.size();

    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
FeatureSettingPolicy::
evaluateCaptureConfiguration(
    ConfigurationOutputParams* out __unused,
    ConfigurationInputParams const* in __unused
) -> bool
{
    CAM_TRACE_CALL();
    CAM_ULOGM_APILIFE();

    // query features by scenario during config
    ScenarioFeatures scenarioFeatures;
    CaptureScenarioConfig scenarioConfig;
    ScenarioHint scenarioHint;
    toTPIDualHint(scenarioHint);
    scenarioHint.operationMode = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->operationMode;
    scenarioHint.isIspHidlTuningEnable = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->hasTuningEnable;
    scenarioHint.isSuperNightMode = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->isSuperNightMode;
    scenarioHint.isLogical = mPolicyParams.bIsLogicalCam;
    //TODO:
    //scenarioHint.captureScenarioIndex = ?   /* hint from vendor tag */
    //scenarioHint.streamingScenarioIndex = ? /* hint from vendor tag */
    int32_t openId = mPolicyParams.pPipelineStaticInfo->openId;
    auto pAppMetadata = &mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->sessionParams;

    int32_t scenario = -1;
    if (!get_capture_scenario(scenario, scenarioFeatures, scenarioConfig, scenarioHint, pAppMetadata)) {
        MY_LOGE("cannot get capture scenario");
        return false;
    }
    else {
        MY_LOGD("find scenario:%s for (openId:%d, scenario:%d)",
                scenarioFeatures.scenarioName.c_str(), openId, scenario);
    }

    for (auto &featureSet : scenarioFeatures.vFeatureSet) {
        MY_LOGI("scenario(%s) support feature:%s(%#" PRIx64"), feature combination:%s(%#" PRIx64")",
                scenarioFeatures.scenarioName.c_str(),
                featureSet.featureName.c_str(),
                static_cast<MINT64>(featureSet.feature),
                featureSet.featureCombinationName.c_str(),
                static_cast<MINT64>(featureSet.featureCombination));
        out->CaptureParams.supportedScenarioFeatures |= (featureSet.feature | featureSet.featureCombination);
    }
    MY_LOGD("support features:%#" PRIx64"", out->CaptureParams.supportedScenarioFeatures);

    mUniqueKey = in->uniqueKey;
    out->CaptureParams.pluginUniqueKey = in->uniqueKey;
    MY_LOGD("(%p) uniqueKey:%d", this, mUniqueKey);

    if (!getCaptureProvidersByScenarioFeatures(out, in)) {
        MY_LOGE("createCapturePluginByScenarioFeatures failed!");
        return false;
    }

    // set the dualFeatureMode for VSDOF or ZOOM
    auto const& pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto& pParsedMultiCamInfo = pParsedAppConfiguration->pParsedMultiCamInfo;
    if (pParsedMultiCamInfo->mCaptureFeatureMode != -1) {
        out->CaptureParams.dualFeatureMode = pParsedMultiCamInfo->mCaptureFeatureMode;
        MY_LOGD("dualFeatureMode:%#" PRIx64"", out->CaptureParams.dualFeatureMode);
    }

    // query additional capture buffer usage count from multiframe features
    if (in->needCaptureQuality) {
        for (auto iter : mMFPPluginWrapperPtr->getProviders()) {
            const MultiFramePlugin::Property& property =  iter->property();
            auto supportedFeatures = out->CaptureParams.supportedScenarioFeatures;
            if ((property.mFeatures & supportedFeatures) != 0) {
                MY_LOGD("provider(%s) algo(%#" PRIx64"), zsdBufferMaxNum:%u, needRrzoBuffer:%d",
                        property.mName, property.mFeatures, property.mZsdBufferMaxNum, property.mNeedRrzoBuffer);
                if (property.mZsdBufferMaxNum > out->CaptureParams.additionalHalP1OutputBufferNum.imgo) {
                    out->CaptureParams.additionalHalP1OutputBufferNum.imgo =  property.mZsdBufferMaxNum;
                    out->CaptureParams.additionalHalP1OutputBufferNum.lcso =  property.mZsdBufferMaxNum; // for tuning, must pairing with imgo num
                }
                if (property.mNeedRrzoBuffer && property.mZsdBufferMaxNum > out->CaptureParams.additionalHalP1OutputBufferNum.rrzo) {
                    out->CaptureParams.additionalHalP1OutputBufferNum.rrzo =  property.mZsdBufferMaxNum; // caputre feature request to use rrzo
                }
                // dualcam mode
                if (miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF) {
                    // rrzo for depth/bokeh usage in dual cam vsdof mode
                    MY_LOGI("additional rrzofor(%u->%u) for MTK_MULTI_CAM_FEATURE_MODE_VSDOF",
                            out->CaptureParams.additionalHalP1OutputBufferNum.rrzo,
                            out->CaptureParams.additionalHalP1OutputBufferNum.imgo);
                    out->CaptureParams.additionalHalP1OutputBufferNum.rrzo = out->CaptureParams.additionalHalP1OutputBufferNum.imgo;
                }
            }
            else {
                MY_LOGD("no need this provider(%s) algo(%#" PRIx64"), zsdBufferMaxNum:%u at this sceanrio(%s)",
                        property.mName, property.mFeatures, property.mZsdBufferMaxNum, scenarioFeatures.scenarioName.c_str());
            }
        }
    }
    else {
        MY_LOGI("no need capture quality output(needCaptureQuality:%d), no need additional buffers for capture", in->needCaptureQuality);
    }
    // update scenarioconfig
    if (CC_UNLIKELY(scenarioConfig.maxAppJpegStreamNum == 0)) {
        MY_LOGW("invalid maxAppJpegStreamNum(%d), set to 1 at least", scenarioConfig.maxAppJpegStreamNum);
        out->CaptureParams.maxAppJpegStreamNum = 1;
    }
    else {
        out->CaptureParams.maxAppJpegStreamNum = scenarioConfig.maxAppJpegStreamNum;
    }
    if (CC_UNLIKELY(scenarioConfig.maxAppRaw16OutputBufferNum == 0)) {
        MY_LOGW("invalid maxAppRaw16OutputBufferNum(%d), set to 1 at least", scenarioConfig.maxAppRaw16OutputBufferNum);
        out->CaptureParams.maxAppRaw16OutputBufferNum = 1;
    }
    else {
        out->CaptureParams.maxAppRaw16OutputBufferNum = scenarioConfig.maxAppRaw16OutputBufferNum;
    }
    MY_LOGI("out->CaptureParams:%s", policy::toString(out->CaptureParams).c_str());

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
FeatureSettingPolicy::
evaluateZslConfiguration(
    ConfigurationOutputParams& out,
    ConfigurationInputParams const& in  __unused
) -> void
{
    auto pPipelineStaticInfo        = mPolicyParams.pPipelineStaticInfo;
    auto pPipelineUserConfiguration = mPolicyParams.pPipelineUserConfiguration;
    auto pParsedAppImageStreamInfo  = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto pSessionParams             = &pPipelineUserConfiguration->pParsedAppConfiguration->sessionParams;

    static bool mbEnableDefaultZslMode = ::property_get_bool("persist.vendor.camera.enable.default.zsl", 0);

    // check zsl enable tag in config stage
    MUINT8 bConfigEnableZsl = false;
    if ( IMetadata::getEntry<MUINT8>(pSessionParams, MTK_CONTROL_CAPTURE_ZSL_MODE, bConfigEnableZsl) )
    {
        MY_LOGI("ZSL mode in SessionParams meta (0x%x) : %d", MTK_CONTROL_CAPTURE_ZSL_MODE, bConfigEnableZsl);
    }
    else if (mbEnableDefaultZslMode)
    {
        // get default zsl mode
        auto pMetadataProvider = NSMetadataProviderManager::valueForByDeviceId(pPipelineStaticInfo->openId);
        IMetadata::IEntry const& entry = pMetadataProvider->getMtkStaticCharacteristics()
                                         .entryFor(MTK_CONTROL_CAPTURE_DEFAULT_ZSL_MODE);
        if ( !entry.isEmpty() ) {
            bConfigEnableZsl = entry.itemAt(0, Type2Type<MUINT8>());
            MY_LOGI("default ZSL mode in static meta (0x%x) : %d", MTK_CONTROL_CAPTURE_DEFAULT_ZSL_MODE, bConfigEnableZsl);
        }
    } else {
        MY_LOGI("MTK_CONTROL_CAPTURE_DEFAULT_ZSL_MODE has no effect : mbEnableDefaultZslMode(%d)", mbEnableDefaultZslMode);
    }
    out.isZslMode = (bConfigEnableZsl) ? true: false;

    // VR mode do not to enable ZSL
    if ( pParsedAppImageStreamInfo->hasVideoConsumer )
    {
        MY_LOGI("Force to disable ZSL in VR");
        out.isZslMode = false;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
FeatureSettingPolicy::
evaluateConfiguration(
    ConfigurationOutputParams* out,
    ConfigurationInputParams const* in
) -> int
{
    CAM_TRACE_NAME("FeatureSettingPolicy::evaluateConfiguration");
    CAM_ULOGM_APILIFE();
    //
    evaluateZslConfiguration(*out, *in);
    //
    if (CC_UNLIKELY(!evaluateCaptureConfiguration(out, in))) {
        CAM_ULOGME("evaluate capture configuration failed!");
        return -ENODEV;
    }
    if (CC_UNLIKELY(!evaluateSeamlessSwitchConfiguration(out, in))) {
        CAM_ULOGME("evaluate seamless switch configuration failed!");
        return -ENODEV;
    }
    if (CC_UNLIKELY(!evaluateStreamConfiguration(out, in))) {
        CAM_ULOGME("evaluate stream configuration failed!");
        return -ENODEV;
    }
    // Default config params for feature strategy.
    mConfigInputParams = *in;
    mConfigOutputParams = *out;
    //
    return OK;
}

bool FeatureSettingPolicy::
updateMultiCamStreamingData(
    RequestOutputParams* out __unused,
    RequestInputParams const* in __unused
)
{
    if (in->pMultiCamReqOutputParams != nullptr) {
        if (!checkNeedUpdateSetting(in->pMultiCamReqOutputParams->streamingSensorList)) {
            // ignore update.
            return true;
        }
    } else {
        MY_LOGE("in->pMultiCamReqOutputParams is nullptr");
        return false;
    }

    MINT32 cshot = 0;
    if (!IMetadata::getEntry<MINT32>(in->pRequest_AppControl, MTK_CSHOT_FEATURE_CAPTURE, cshot)) {
        MY_LOGD_IF(2 <= mbDebug, "cannot get MTK_CSHOT_FEATURE_CAPTURE");
    }

    // Considered cases:
    //  - zsl capture
    //  - non-zsl cshot with 2 streaming sensors
    if ((in->needP2CaptureNode || in->pMultiCamReqOutputParams->prvStreamingSensorList.size() > 0) &&
        (in->pRequest_ParsedAppMetaControl->control_enableZsl || !!cshot)) {
        MY_LOGD("not yet implement for stream feature setting evaluate with capture behavior");
        return true;
    }

    auto const& sensorIdList = mPolicyParams.pPipelineStaticInfo->sensorId;
    auto const& vStreamingSensorList = in->pMultiCamReqOutputParams->streamingSensorList;
    auto pConfig_StreamInfo_P1 = in->pConfiguration_StreamInfo_P1;
    unordered_map<uint32_t, uint32_t> dmaList;
    unordered_map<uint32_t, MetadataPtr> halMetaPtrList;
    bool isYUVReprocessing = false;
    // is yuv reprocess flow
    if(in->pRequest_ParsedAppMetaControl != nullptr){
        if(in->pRequest_ParsedAppMetaControl->control_captureIntent ==
            MTK_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG) {
                MY_LOGD("YUV reprocessing");
                isYUVReprocessing = true;
            }
    }
    // init halMetaPtrList
    for(auto sensorId : sensorIdList)
    {
        halMetaPtrList.insert({sensorId, std::make_shared<IMetadata>()});
    }
    auto queryNeedP1DmaResult = [&pConfig_StreamInfo_P1, &isYUVReprocessing, this]
                                (int sensorId, MSize yuvSize)
    {
        uint32_t needP1Dma = 0;
        // In physical case, no need P1 DMA if yuv reprocessing.
        if((isYUVReprocessing && sensorId != mReqMasterId)) {
            return needP1Dma;
        }

        if (!pConfig_StreamInfo_P1->count(sensorId))
        {
            MY_LOGE("sensorId(%d) doesn't exist in pConfiguration_StreamInfo_P1", sensorId);
            return needP1Dma;
        }
        //
        if (pConfig_StreamInfo_P1->at(sensorId).pHalImage_P1_Rrzo != nullptr)
        {
            needP1Dma |= P1_RRZO;
        }
        auto needImgo = [] (MSize imgSize, MSize rrzoSize) -> int
        {
            return (imgSize.w > rrzoSize.w) || (imgSize.h > rrzoSize.h);
        };
        if ((pConfig_StreamInfo_P1->at(sensorId).pHalImage_P1_Imgo != nullptr)&&
            (pConfig_StreamInfo_P1->at(sensorId).pHalImage_P1_Rrzo != nullptr)&&
            (needImgo(yuvSize, pConfig_StreamInfo_P1->at(sensorId).pHalImage_P1_Rrzo->getImgSize())))
        {
            needP1Dma |= P1_IMGO;
        }
        if (pConfig_StreamInfo_P1->at(sensorId).pHalImage_P1_Lcso != nullptr)
        {
            needP1Dma |= P1_LCSO;
        }
        if (pConfig_StreamInfo_P1->at(sensorId).pHalImage_P1_Rsso != nullptr)
        {
            needP1Dma |= P1_RSSO;
        }
        // for zsl case, it has to set imgo output
        if(mConfigOutputParams.isZslMode)
        {
            needP1Dma |= P1_IMGO;
        }
        return needP1Dma;
    };

    auto updateIspProfile = [&pConfig_StreamInfo_P1, this](
                            int32_t masterId,
                            int32_t sensorId,
                            IMetadata* metadata)
    {
        bool isVSDoF3rdParty = (StereoSettingProvider::getMtkDepthFlow()==1) ? true: false;
        if(miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF && !isVSDoF3rdParty)
        {
            // set isp profile
            MUINT8 ispprofile = NSIspTuning::EIspProfile_Preview;
            auto scaledYuvStreamInfo = pConfig_StreamInfo_P1->at(masterId).pHalImage_P1_ScaledYuv;
            auto numsP1YUV = StereoSettingProvider::getP1YUVSupported();
            bool useRSSOR2 =  (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_RSSO_R2 || numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_YUVO_RSSO_R2);
            if (useRSSOR2)
                scaledYuvStreamInfo = pConfig_StreamInfo_P1->at(masterId).pHalImage_P1_RssoR2;
            // main1 support p1 yuv.
            if(scaledYuvStreamInfo != nullptr)
            {
                MRect yuvCropRect;
                queryVsdofP1YuvCropRect(masterId, scaledYuvStreamInfo, yuvCropRect);
                IMetadata::setEntry<MRect>(metadata, MTK_P1NODE_YUV_RESIZER2_CROP_REGION, yuvCropRect);
                ispprofile = StereoSettingProvider::getISPProfileForP1YUV(sensorId);
            }
            // if needs update isp profile, remove previous setting first.
            metadata->remove(MTK_3A_ISP_PROFILE);
            IMetadata::setEntry<MUINT8>(metadata, MTK_3A_ISP_PROFILE, ispprofile);
        }
        // if camsv path, it has to set hal metadata to covert mono img to yuv.
        if (pConfig_StreamInfo_P1->at(masterId).pHalImage_P1_Rrzo == nullptr)
        {
            IMetadata::setEntry<MINT32>(metadata, MTK_ISP_P2_IN_IMG_FMT, 5);
        }
        return true;
    };
    auto updateSensorCrop = [&in, this](
                            int32_t sensorId,
                            IMetadata* metadata)
    {
        bool isVSDoF3rdParty = (StereoSettingProvider::getMtkDepthFlow()==1) ? true: false;
        if(miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF && !isVSDoF3rdParty)
        {
            // vsdof crop info is compute by StereSizeProvider.
            MRect crop;
            queryVsdofSensorCropRect(sensorId, in, crop);
            IMetadata::setEntry<MRect>(metadata, MTK_P1NODE_SENSOR_CROP_REGION, crop);
            IMetadata::setEntry<MRect>(metadata, MTK_DUALZOOM_CAP_CROP, crop);
            MRect crop3A;
            queryVsdof3APreviewCropRect(sensorId, in, crop3A);
            IMetadata::setEntry<MRect>(metadata, MTK_3A_PRV_CROP_REGION, crop3A);
        }
        else
        {
            MRect tgCrop, scalerCrop;
            std::vector<MINT32> mvaf_region,mvae_region,mvawb_region;
            bool isCapture = false;
            bool isPreview = !isCapture;
            if(queryMulticamTgCrop(in, sensorId, tgCrop, isCapture))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_P1NODE_SENSOR_CROP_REGION, tgCrop);
            }
            if(queryMulticamTgCrop(in, sensorId, tgCrop, isCapture, true))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_3A_PRV_CROP_REGION, tgCrop);
            }
            if(queryMulticamTgCrop(in, sensorId, tgCrop, isPreview))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_DUALZOOM_CAP_CROP, tgCrop);
            }
            if(queryMulticamScalerCrop(in, sensorId, scalerCrop, isCapture))
            {
                IMetadata::setEntry<MRect>(metadata, MTK_SENSOR_SCALER_CROP_REGION, scalerCrop);
            }
            if(queryMulticam3aRegion(in, sensorId, mvaf_region,mvae_region,mvawb_region))
            {
                IMetadata::IEntry af_region_entry(MTK_MULTI_CAM_AF_ROI);
                IMetadata::IEntry ae_region_entry(MTK_MULTI_CAM_AE_ROI);
                IMetadata::IEntry awb_region_entry(MTK_MULTI_CAM_AWB_ROI);
                for(MINT32 i = 0;i<mvaf_region.size();i++){
                    af_region_entry.push_back(mvaf_region[i],Type2Type< MINT32 >());
                    metadata->update(MTK_MULTI_CAM_AF_ROI,af_region_entry);
                }
                for(MINT32 i = 0;i<mvae_region.size();i++){
                    ae_region_entry.push_back(mvae_region[i],Type2Type< MINT32 >());
                    metadata->update(MTK_MULTI_CAM_AE_ROI,ae_region_entry);
                }
                for(MINT32 i = 0;i<mvawb_region.size();i++){
                    awb_region_entry.push_back(mvawb_region[i],Type2Type< MINT32 >());
                    metadata->update(MTK_MULTI_CAM_AWB_ROI,awb_region_entry);
                }
            }
            if (mvaf_region.size() >= 3)
            {
                MY_LOGD("sensorId(%d) af_region(%d, %d, %dx%d)",
                                    sensorId,
                                    mvaf_region[0],
                                    mvaf_region[1],
                                    mvaf_region[2]-mvaf_region[0],
                                    mvaf_region[3]-mvaf_region[1]);
            }
        }
        return true;
    };
    // duplicate main1 metadata to each other.
    if(mReqSubSensorIdList.size() > 0)
    {
        auto halMeta = out->mainFrame->additionalHal.find(mReqMasterId);
        if ( halMeta != out->mainFrame->additionalHal.end() ) {
            auto appMeta = out->mainFrame->additionalApp;
            for (const auto& sensorId : mReqSubSensorIdList)
            {
                updateRequestResultParams(out->mainFrame,
                                      appMeta,
                                      halMeta->second,
                                      0,
                                      sensorId);
            }
        }
    }
    // check sensor control
    {
        MSize imgSize = in->maxP2StreamSize;
        for (const auto& sensorId : sensorIdList)
        {
            uint32_t dma = queryNeedP1DmaResult(sensorId, imgSize);
            updateMulticamSensorControl(in, sensorId, halMetaPtrList[sensorId], dma);
            auto iter = dmaList.find(sensorId);
            if(iter != dmaList.end())
            {
                iter->second |= dma;
            }
            else
            {
                dmaList.insert({sensorId, dma});
            }
        }
    }

    // update metadata
    if(vStreamingSensorList.size() > 1 && !isYUVReprocessing)
    {
        auto doSlaveFDPeriod = ::property_get_int32("vendor.debug.camera.slave_fd_period", DO_SLAVE_FD_PERIOD);
        doSlaveFDPeriod = (doSlaveFDPeriod == 0 ? 1 : doSlaveFDPeriod);
        bool doSlaveFd = in->requestNo % doSlaveFDPeriod;
        for (auto&& sensorId : vStreamingSensorList)
        {
            auto iter = halMetaPtrList.find(sensorId);
            if(iter != halMetaPtrList.end() && iter->second != nullptr)
            {
                if(in->pMultiCamReqOutputParams->need3ASwitchDoneNotify) {
                    IMetadata::setEntry<MINT32>(iter->second.get(), MTK_STEREO_NOTIFY,
                                                    (1<<NS3Av3::E_SYNC3A_NOTIFY::E_SYNC3A_NOTIFY_SWITCH_ON));
                    MY_LOGI("set 3a notify");
                }
                // set isp profile
                updateIspProfile(mReqMasterId, sensorId, iter->second.get());
                // update sensor crop
                updateSensorCrop(mReqMasterId, iter->second.get());
                if(sensorId != in->pMultiCamReqOutputParams->masterId){
                    // only update streaming nr in slave sensor.
                    if(in->pMultiCamReqOutputParams->isForceOffStreamingNr || miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF){
                        IMetadata::setEntry<MINT32>(
                                                iter->second.get(),
                                                MTK_DUALZOOM_STREAMING_NR,
                                                MTK_DUALZOOM_STREAMING_NR_OFF);
                    }
                    else {
                        IMetadata::setEntry<MINT32>(
                                                iter->second.get(),
                                                MTK_DUALZOOM_STREAMING_NR,
                                                MTK_DUALZOOM_STREAMING_NR_AUTO);
                    }
                }

                // target fd decision
                if(miMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
                {
                    if(doSlaveFd)
                    {
                        IMetadata::setEntry<MINT32>(iter->second.get(), MTK_DUALZOOM_FD_TARGET_MASTER,
                                                            (MINT32)in->pMultiCamReqOutputParams->masterId);
                    }
                    else
                    {
                        for(auto& id:vStreamingSensorList)
                        {
                            if(id != in->pMultiCamReqOutputParams->masterId)
                            {
                                IMetadata::setEntry<MINT32>(iter->second.get(), MTK_DUALZOOM_FD_TARGET_MASTER, (MINT32)id);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    IMetadata::setEntry<MINT32>(iter->second.get(), MTK_DUALZOOM_FD_TARGET_MASTER,
                                                        (MINT32)in->pMultiCamReqOutputParams->masterId);
                }
            }
        }
    }
    else
    {
        for (auto&& sensorId:vStreamingSensorList)
        {
            auto iter = halMetaPtrList.find(sensorId);
            if (iter != halMetaPtrList.end() && iter->second != nullptr)
            {
                // set isp profile
                updateIspProfile(mReqMasterId, sensorId, iter->second.get());
                // update sensor crop
                updateSensorCrop(sensorId, iter->second.get());
            }
        }
    }

    // decide need skip depth compute or not
    bool needSkipSubImage = (in->requestNo % mPeriodForStreamingProcessDepth) != 0;

    for (auto&& sensorId : sensorIdList)
    {
        updateRequestResultParams(out->mainFrame,
                            nullptr,
                            halMetaPtrList[sensorId],
                            dmaList[sensorId],
                            sensorId);

        if(needSkipSubImage && sensorId != mReqMasterId) // idx != 0 means not master cam.
        {
            out->mainFrame->needP2Process.emplace(sensorId, 0);
        }
        else
        {
            out->mainFrame->needP2Process.emplace(sensorId, P2_STREAMING);
        }
    }
    return true;
}

bool FeatureSettingPolicy::
checkNeedUpdateSetting(
    std::vector<uint32_t> const& ids
)
{
    // check can update capture setting or not.
    if (!mPolicyParams.bIsLogicalCam) {
        // if it is physical feature setting policy, check exist in streaming list
        // to decide needs update or not.
        auto iter = std::find(
                              ids.begin(),
                              ids.end(),
                              mPolicyParams.pPipelineStaticInfo->openId);
        if(iter == ids.end()) {
            MY_LOGW("ignore to update sensorId(%" PRIu32 ")",
                                    mPolicyParams.pPipelineStaticInfo->openId);
            return false;
        }
    }
    return true;
}

bool
FeatureSettingPolicy::
queryVsdof3APreviewCropRect(
    int32_t sensorId,
    RequestInputParams const* in,
    MRect& sensorCrop
)
{
    // get rrzo format
    auto RrzoStreamInfo = (*in->pConfiguration_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo;
    // get sensor crop
    if(RrzoStreamInfo != nullptr)
    {
        StereoSizeProvider::getInstance()->getPass1ActiveArrayCrop(
                sensorId,
                sensorCrop);
    }
    else
    {
        // if rrzo is null, asign imgo crop size
        StereoSizeProvider::getInstance()->getPass1ActiveArrayCrop(
                sensorId,
                sensorCrop,
                false);
    }
    MY_LOGD("queryVsdof3APreviewCropRect sensorId(%d) sensorCrop(%d, %d, %dx%d", sensorId, sensorCrop.p.x, sensorCrop.p.y, sensorCrop.s.w, sensorCrop.s.h);
    return true;
}

bool
FeatureSettingPolicy::
queryVsdofSensorCropRect(
    int32_t sensorId,
    RequestInputParams const* in,
    MRect& sensorCrop
)
{
    bool ret = false;
    // get rrzo format
    auto rrzoStreamInfo = (*in->pConfiguration_StreamInfo_P1).at(sensorId).pHalImage_P1_Rrzo;
    // get sensor crop
    MUINT32 q_stride;
    NSCam::MSize size;
    if(rrzoStreamInfo != nullptr)
    {
        auto numsP1YUV = StereoSettingProvider::getP1YUVSupported();
        if (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_YUVO_RSSO_R2){
            StereoSizeProvider::getInstance()->getPass1Size(
                sensorId,
                (EImageFormat)rrzoStreamInfo->getImgFormat(),
                NSImageio::NSIspio::EPortIndex_YUVO,
                StereoHAL::eSTEREO_SCENARIO_PREVIEW, // in this mode, stereo only support zsd.
                (MRect&)sensorCrop,
                size,
                q_stride);
        }
        //HAS_P1_YUV_CRZO means 2RRZO+1CRZO ,HAS_P1_YUV_RSSO_R2 means 2RRZO+2RSSOR2
        else if (numsP1YUV >= NSCam::v1::Stereo::NO_P1_YUV){
            StereoSizeProvider::getInstance()->getPass1Size(
                sensorId,
                (EImageFormat)rrzoStreamInfo->getImgFormat(),
                NSImageio::NSIspio::EPortIndex_RRZO,
                StereoHAL::eSTEREO_SCENARIO_PREVIEW, // in this mode, stereo only support zsd.
                (MRect&)sensorCrop,
                size,
                q_stride);
        }
        else
        {
            MY_LOGE("Should check numsP1YUV(%d)!! It should not less than 0",numsP1YUV);
        }
    }
    else
    {
        // if rrzo is null, assign imgo crop size
        auto imgoStreamInfo = (*in->pConfiguration_StreamInfo_P1).at(sensorId).pHalImage_P1_Imgo;
        StereoSizeProvider::getInstance()->getPass1Size(
                sensorId,
                (EImageFormat)imgoStreamInfo->getImgFormat(),
                NSImageio::NSIspio::EPortIndex_IMGO,
                StereoHAL::eSTEREO_SCENARIO_CAPTURE, // in this mode, stereo only support zsd.
                (MRect&)sensorCrop,
                size,
                q_stride);
    }
    ret = true;
    return ret;
}

bool
FeatureSettingPolicy::
queryVsdofP1YuvCropRect(
    int32_t sensorId,
    sp<IImageStreamInfo>& imgStreamInfo,
    MRect& yuvCropRect
)
{
    bool ret = false;
    MUINT32 q_stride;
    NSCam::MSize size;
    auto numsP1YUV = StereoSettingProvider::getP1YUVSupported();
    bool useRSSOR2 =  (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_RSSO_R2 || numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_YUVO_RSSO_R2);
    if(imgStreamInfo == nullptr)
    {
        MY_LOGE("Should not happened!");
        goto lbExit;
    }
    if (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_CRZO){
    StereoSizeProvider::getInstance()->getPass1Size(
                sensorId,
                (EImageFormat)imgStreamInfo->getImgFormat(),
                NSImageio::NSIspio::EPortIndex_CRZO_R2,
                StereoHAL::eSTEREO_SCENARIO_PREVIEW, // in this mode, stereo only support zsd.
                (MRect&)yuvCropRect,
                size,
                q_stride);
     }
     else if (useRSSOR2){
         StereoSizeProvider::getInstance()->getPass1Size(
                sensorId,
                (EImageFormat)imgStreamInfo->getImgFormat(),
                NSImageio::NSIspio::EPortIndex_RSSO_R2,
                StereoHAL::eSTEREO_SCENARIO_PREVIEW, // in this mode, stereo only support zsd.
                (MRect&)yuvCropRect,
                size,
                q_stride);
    }
    else
        MY_LOGE("Should check numsP1YUV!!");
    ret = true;
lbExit:
    return ret;
}

auto
FeatureSettingPolicy::
evaluateRequest(
    RequestOutputParams* out,
    RequestInputParams const* in
) -> int
{
    CAM_TRACE_NAME("FeatureSettingPolicy::evaluateRequest");
    //CAM_ULOGM_APILIFE();

    // check setting valid.
    if CC_UNLIKELY(out == nullptr || in == nullptr) {
        CAM_ULOGME("invalid in(%p), out(%p) for evaluate", in, out);
        return -ENODEV;
    }

    if (!collectDefaultSensorInfo(out, in)) {
        MY_LOGE("collectDefaultSensorInfo failed!");
        return -ENODEV;
    }

    // P2 capture feature policy
    if (!evaluateCaptureSetting(out, in)) {
        MY_LOGE("evaluateCaptureSetting failed!");
        return -ENODEV;
    }

    // P2 streaming feature policy
    if (!evaluateStreamSetting(out, in)) {
        MY_LOGE("evaluateStreamSetting failed!");
        return -ENODEV;
    }

    // update needReconfiguration info.
    if (!evaluateReconfiguration(out, in)) {
        MY_LOGE("evaluateReconfiguration failed!");
        return -ENODEV;
    }

    // Seamless Switch: should be evaluated after evaluateReconfiguration
    // to handle seamless remosic capture
    if (!evaluateSeamlessSwitchSetting(out, in)) {
        MY_LOGE("evaluateSeamlessSwitchSetting failed!");
        return -ENODEV;
    }

    // dump output request params for debug.
    dumpRequestOutputParams(out, in, mbDebug);

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {
auto createFeatureSettingPolicyInstance(
    CreationParams const& params
) -> std::shared_ptr<IFeatureSettingPolicy>
{
    CAM_ULOGM_APILIFE();
    // check the policy params is valid.
    if (CC_UNLIKELY(params.pPipelineStaticInfo.get() == nullptr)) {
        CAM_ULOGME("pPipelineStaticInfo is invalid nullptr");
        return nullptr;
    }
    if (CC_UNLIKELY(params.pPipelineUserConfiguration.get() == nullptr)) {
        CAM_ULOGME("pPipelineUserConfiguration is invalid nullptr");
        return nullptr;
    }
    int32_t openId = params.pPipelineStaticInfo->openId;
    if (CC_UNLIKELY(openId < 0)) {
        CAM_ULOGME("openId is invalid(%d)", openId);
        return nullptr;
    }
    if (CC_UNLIKELY(params.pPipelineStaticInfo->sensorId.empty())) {
        CAM_ULOGME("sensorId is empty(size:%zu)", params.pPipelineStaticInfo->sensorId.size());
        return nullptr;
    }
    for (unsigned int i=0; i<params.pPipelineStaticInfo->sensorId.size(); i++) {
        int32_t sensorId = params.pPipelineStaticInfo->sensorId[i];
        CAM_ULOGMD("sensorId[%d]=%d", i, sensorId);
        if (CC_UNLIKELY(sensorId < 0)) {
            CAM_ULOGME("sensorId is invalid(%d)", sensorId);
            return nullptr;
        }
    }
    // you have got an instance for feature setting policy.
    return std::make_shared<FeatureSettingPolicy>(params);
}
};  //namespace
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
