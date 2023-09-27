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

#define LOG_TAG "mtkcam-ConfigSettingPolicyMediator"

#include <mtkcam3/pipeline/policy/IConfigSettingPolicyMediator.h>
#include <mtkcam3/pipeline/policy/InterfaceTableDef.h>
//
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/feature/PIPInfo.h>

#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);


/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3::pipeline;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::v3::pipeline::policy::pipelinesetting;
using namespace NSCam::v3::pipeline::policy::requestsensorcontrol;

#define ThisNamespace   ConfigSettingPolicyMediator_Default

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


/******************************************************************************
 *
 ******************************************************************************/
#define RETURN_IF_ERROR(_expr_, fmt, arg...)                            \
    do {                                                                \
        int const err = (_expr_);                                       \
        if( CC_UNLIKELY(err != 0) ) {                                   \
            MY_LOGE("err:%d(%s) - " fmt, err, ::strerror(-err), ##arg); \
            return err;                                                 \
        }                                                               \
    } while(0)


/******************************************************************************
 *
 ******************************************************************************/
class ThisNamespace
    : public IConfigSettingPolicyMediator
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Static data (unchangable)
    std::shared_ptr<PipelineStaticInfo const>       mPipelineStaticInfo;
    std::shared_ptr<PipelineUserConfiguration const>mPipelineUserConfiguration;
    std::shared_ptr<PolicyTable const>              mPolicyTable;
    MINT32                                          mUniqueKey = 0;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation.
                    ThisNamespace(MediatorCreationParams const& params);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IConfigSettingPolicyMediator Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.

    virtual auto    evaluateConfiguration(
                        ConfigurationOutputParams& out,
                        ConfigurationInputParams const& in
                    ) -> int override;

};


/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<IConfigSettingPolicyMediator>
makeConfigSettingPolicyMediator_Default(MediatorCreationParams const& params)
{
    return std::make_shared<ThisNamespace>(params);
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
ThisNamespace(MediatorCreationParams const& params)
    : IConfigSettingPolicyMediator()
    , mPipelineStaticInfo(params.pPipelineStaticInfo)
    , mPipelineUserConfiguration(params.pPipelineUserConfiguration)
    , mPolicyTable(params.pPolicyTable)
{
    mUniqueKey = NSCam::Utils::TimeTool::getReadableTime();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
evaluateConfiguration(
    ConfigurationOutputParams& out,
    ConfigurationInputParams const& in __unused
) -> int
{
    //---------------------------------
    // 1st level

    featuresetting::ConfigurationInputParams featureIn;
    featuresetting::ConfigurationOutputParams featureOut;
    featureIn.isP1DirectFDYUV = mPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV;
    featureIn.numsP1YUV = mPipelineStaticInfo->numsP1YUV;
    if(featureIn.numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_CRZO ){
        featureIn.isP1DirectFDYUV = false;
        MY_LOGD("Force P1FD to off for HW ,numsP1YUV (%d)",featureIn.numsP1YUV );
    }
    featureIn.uniqueKey = mUniqueKey;
    featureIn.needCaptureQuality = (mPipelineUserConfiguration->pParsedAppImageStreamInfo->secureInfo.type==SecType::mem_normal) ? true : false;
    featureIn.reconfigCategory = in.reconfigCategory;
    RETURN_IF_ERROR(
        mPolicyTable->mFeaturePolicy->evaluateConfiguration(
        &featureOut, &featureIn),
        "mFeaturePolicy->evaluateConfiguration"
    );
    // init for other feature policy (do capture pipe init)
    for(auto&& featurePolicy:mPolicyTable->mFeaturePolicy_Physical)
    {
        if(featurePolicy.second != nullptr)
        {
            featuresetting::ConfigurationOutputParams featureOut_physical;
            RETURN_IF_ERROR(
                featurePolicy.second->evaluateConfiguration(
                &featureOut_physical, &featureIn),
                "mFeaturePolicy->evaluateConfiguration (slave)"
            );
        }
    }

    *(out.pStreamingFeatureSetting) = featureOut.StreamingParams;
    *(out.pCaptureFeatureSetting) = featureOut.CaptureParams;
    *(out.pSeamlessSwitchFeatureSetting) = featureOut.SeamlessSwitchParams;

    if (out.pIsZSLMode != nullptr)
    {
        *(out.pIsZSLMode) = featureOut.isZslMode;
    }

    if (out.pUniKey != nullptr)
    {
        *(out.pUniKey) = mUniqueKey;
    }

    MY_LOGD("Configuring fConfigPipelineNodesNeed");

    if(!in.bypassNodeNeedPolicy)
    {
        RETURN_IF_ERROR(
        mPolicyTable->fConfigPipelineNodesNeed(Configuration_PipelineNodesNeed_Params{
            .pOut = out.pPipelineNodesNeed,
            .pPipelineStaticInfo = mPipelineStaticInfo.get(),
            .pPipelineUserConfiguration = mPipelineUserConfiguration.get(),
            .bIsZslMode = featureOut.isZslMode}),
        "fConfigPipelineNodesNeed" );
    }

    MY_LOGD("Configuring fConfigPipelineTopology");

    RETURN_IF_ERROR(
        mPolicyTable->fConfigPipelineTopology(Configuration_PipelineTopology_Params{
            .pOut = out.pPipelineTopology,
            .pPipelineNodesNeed = out.pPipelineNodesNeed,
            .pPipelineStaticInfo = mPipelineStaticInfo.get(),
            .pPipelineUserConfiguration = mPipelineUserConfiguration.get()}),
        "fConfigPipelineTopology" );

    //---------------------------------
    // 2nd level

    MY_LOGD("Configuring fConfigSensorSetting");

    if(!in.bypassSensorSetting)
    {
        RETURN_IF_ERROR(
            mPolicyTable->fConfigSensorSetting(Configuration_SensorSetting_Params{
                .pvOut = out.pSensorSetting,
                .pStreamingFeatureSetting = out.pStreamingFeatureSetting,
                .pSeamlessSwitchFeatureSetting = out.pSeamlessSwitchFeatureSetting,
                .pPipelineStaticInfo = mPipelineStaticInfo.get(),
                .pPipelineUserConfiguration = mPipelineUserConfiguration.get()}),
            "fConfigSensorSetting" );
    }

    // Config PIP sensor setting
    auto& pipInfo = PIPInfo::getInstance();
    // update sensor setting
    if (pipInfo.getPIPConfigureDone() == false) {
        for (const auto& it : out.pPipelineNodesNeed->needP1Node) {
            if (it.second == true) {
                pipInfo.addSensorSetting(mPipelineStaticInfo->openId, it.first, out.pSensorSetting->at(it.first));
            }
        }
    }

    //---------------------------------
    // 3rd level

    MY_LOGD("Configuring fConfigP1HwSetting");

    RETURN_IF_ERROR(
        mPolicyTable->fConfigP1HwSetting(Configuration_P1HwSetting_Params{
            .pvOut = out.pP1HwSetting,
            .pSensorSetting = out.pSensorSetting,
            .pStreamingFeatureSetting = out.pStreamingFeatureSetting,
            .pSeamlessSwitchFeatureSetting = out.pSeamlessSwitchFeatureSetting,
            .pPipelineNodesNeed = out.pPipelineNodesNeed,
            .pPipelineStaticInfo = mPipelineStaticInfo.get(),
            .pPipelineUserConfiguration = mPipelineUserConfiguration.get()}),
        "fConfigP1HwSetting" );

    MY_LOGD("Configuring fConfigP1DmaNeed");

    RETURN_IF_ERROR(
        mPolicyTable->fConfigP1DmaNeed(
            out.pP1DmaNeed,
            out.pP1HwSetting,
            out.pStreamingFeatureSetting,
            mPipelineStaticInfo.get(), mPipelineUserConfiguration.get()),
        "fConfigP1DmaNeed" );

    MY_LOGD("Configuring fConfigStreamInfo_P1");

    RETURN_IF_ERROR(
        mPolicyTable->fConfigStreamInfo_P1(Configuration_StreamInfo_P1_Params{
            .pvOut = out.pParsedStreamInfo_P1,
            .pvSensorSetting = out.pSensorSetting,
            .pvP1HwSetting = out.pP1HwSetting,
            .pvP1DmaNeed = out.pP1DmaNeed,
            .pPipelineNodesNeed = out.pPipelineNodesNeed,
            .pStreamingFeatureSetting = out.pStreamingFeatureSetting,
            .pCaptureFeatureSetting = out.pCaptureFeatureSetting,
            .pPipelineStaticInfo = mPipelineStaticInfo.get(),
            .pPipelineUserConfiguration = mPipelineUserConfiguration.get()}),
        "fConfigStreamInfo_P1" );

    //---------------------------------
    // 4th level

    MY_LOGD("Configuring fConfigStreamInfo_NonP1");

    RETURN_IF_ERROR(
        mPolicyTable->fConfigStreamInfo_NonP1(Configuration_StreamInfo_NonP1_Params{
            .pOut = out.pParsedStreamInfo_NonP1,
            .pPipelineNodesNeed = out.pPipelineNodesNeed,
            .pStreamingFeatureSetting = out.pStreamingFeatureSetting,
            .pCaptureFeatureSetting = out.pCaptureFeatureSetting,
            .pPipelineStaticInfo = mPipelineStaticInfo.get(),
            .pPipelineUserConfiguration = mPipelineUserConfiguration.get(),
            .pvSensorSetting = out.pSensorSetting}),
        "fConfigStreamInfo_NonP1" );

    SensorMap<int> exclusiveGroup;
    if ( mPolicyTable->pRequestSensorControlPolicy != nullptr) {
        auto multicamInfo = mPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
        MINT32 maxP1SupportSize = 2;
        if(multicamInfo != nullptr) {
            maxP1SupportSize = multicamInfo->mSupportPass1Number;
            MY_LOGD("max support p1 size(%d)", maxP1SupportSize);
        }
        RETURN_IF_ERROR(
            mPolicyTable->pRequestSensorControlPolicy->evaluateConfiguration(Configuration_SensorControl_Params{
                    .pOut = &(out.pPipelineNodesNeed->vNeedLaggingConfig),
                    .pExclusiveGroup = &exclusiveGroup,
                    .pPipelineStaticInfo = mPipelineStaticInfo.get(),
                    .pPipelineUserConfiguration = mPipelineUserConfiguration.get(),
                    .pSeamlessSwitchConfiguration = out.pSeamlessSwitchFeatureSetting,
                    .iMaxP1SupportCount = maxP1SupportSize,
                }),
            "pRequestSensorControlPolicy->evaluateConfiguration");
    }

    if ( mPolicyTable->fConfigBufferPoolSharedStream != nullptr
     && out.pPoolSharedStreams != nullptr ) {
        RETURN_IF_ERROR(
            mPolicyTable->fConfigBufferPoolSharedStream(Configuration_BufferPoolSharedStream_Params{
                .pOut = out.pPoolSharedStreams,
                .pParsedStreamInfo_P1 = out.pParsedStreamInfo_P1,
                .pParsedStreamInfo_NonP1 = out.pParsedStreamInfo_NonP1,
                .pPipelineStaticInfo = mPipelineStaticInfo.get(),
                .pPipelineUserConfiguration = mPipelineUserConfiguration.get(),
                .pExclusiveGroup = &exclusiveGroup}),
            "fConfigBufferPoolSharedStream" );
    }

    MY_LOGD("Configure mediator done");

    return OK;
}

