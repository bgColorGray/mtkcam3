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

#define LOG_TAG "mtkcam-RequestSettingPolicyMediator_AOV"

#include <mtkcam3/pipeline/policy/IRequestSettingPolicyMediator.h>
#include <mtkcam3/pipeline/policy/InterfaceTableDef.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/std/ULog.h>

#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::pipeline;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::v3::pipeline::policy::pipelinesetting;
using namespace NSCam::v3::pipeline::NSPipelineContext;
using namespace NSCam::v3::Utils;

#define ThisNamespace   RequestSettingPolicyMediator_AOV

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGD(fmt, arg...)  CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)  CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)  CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)  CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)  CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...) do { if (           (cond)) { MY_LOGD(__VA_ARGS__); } } while(0)
#define MY_LOGI_IF(cond, ...) do { if (           (cond)) { MY_LOGI(__VA_ARGS__); } } while(0)
#define MY_LOGW_IF(cond, ...) do { if (CC_UNLIKELY(cond)) { MY_LOGW(__VA_ARGS__); } } while(0)
#define MY_LOGE_IF(cond, ...) do { if (CC_UNLIKELY(cond)) { MY_LOGE(__VA_ARGS__); } } while(0)
#define MY_LOGF_IF(cond, ...) do { if (CC_UNLIKELY(cond)) { MY_LOGF(__VA_ARGS__); } } while(0)


/******************************************************************************
 *
 ******************************************************************************/
class ThisNamespace
    : public IRequestSettingPolicyMediator
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Static data (unchangable)
    std::shared_ptr<PipelineStaticInfo const>       mPipelineStaticInfo;
    std::shared_ptr<PipelineUserConfiguration const>mPipelineUserConfiguration;
    std::shared_ptr<PolicyTable const>              mPolicyTable;

    int32_t                                         mUniqueKey = 0;
    uint32_t                                        mMasterSensorId = 0;
    android::sp<IImageStreamInfo>                   mAppImage_Output_AOV;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation.
                    ThisNamespace(MediatorCreationParams const& params);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IRequestSettingPolicyMediator Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.
    virtual auto    evaluateRequest(
                        RequestOutputParams& out,
                        RequestInputParams const& in
                    ) -> int override;
};


/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<IRequestSettingPolicyMediator>
makeRequestSettingPolicyMediator_AOV(MediatorCreationParams const& params) {
    return std::make_shared<ThisNamespace>(params);
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
ThisNamespace(MediatorCreationParams const& params)
    : IRequestSettingPolicyMediator()
    , mPipelineStaticInfo(params.pPipelineStaticInfo)
    , mPipelineUserConfiguration(params.pPipelineUserConfiguration)
    , mPolicyTable(params.pPolicyTable)
    , mUniqueKey(NSCam::Utils::TimeTool::getReadableTime())
    , mMasterSensorId(mPipelineStaticInfo->sensorId[0])
    , mAppImage_Output_AOV(mPipelineUserConfiguration->pParsedAppConfiguration->aovImageStreamInfo)
{
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
evaluateRequest(
    RequestOutputParams& out,
    RequestInputParams const& in) -> int
{
    CAM_TRACE_CALL();

    // (2) feature setting
    {
        out.sensorMode = *in.pSensorMode;
        // out.vboostControl = featureOut.vboostControl;
        // out.CPUProfile = featureOut.CPUProfile;
    }

    // (3) build every frames out param
    auto buildOutParam = [&]()
    {
        MY_LOGD("build out frame param +");
        auto result = std::make_shared<RequestResultParams>();
        MY_LOGF_IF(!result, "make_shared");

        // build topology
        {
            result->nodesNeed = *in.pConfiguration_PipelineNodesNeed;
            result->edges = in.pConfiguration_PipelineTopology->edges;
            result->roots = in.pConfiguration_PipelineTopology->roots;
            for (auto nodeId : result->roots) {
                result->nodeSet.push_back(nodeId);
            }
        }

        // build IO map
        for (auto nodeId : result->roots)
        {
            auto sensorId = mMasterSensorId;  // Should map nodeId to sensorId
            auto const& configP1Streams = (*in.pConfiguration_StreamInfo_P1).at(sensorId);

            result->nodeIOMapMeta [nodeId] = IOMapSet().add(IOMap()
                .addIn( in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId())
                .addIn( configP1Streams.pHalMeta_Control->getStreamId())
                .addOut(configP1Streams.pAppMeta_DynamicP1->getStreamId())
                .addOut(configP1Streams.pHalMeta_DynamicP1->getStreamId())
            );

            result->nodeIOMapImage[nodeId] = IOMapSet().add(IOMap()
                .addOut(mAppImage_Output_AOV->getStreamId())
            );
        }

        // additionalApp / additionalHal
        {
            result->additionalApp = std::make_shared<IMetadata>();
            MY_LOGF_IF(!result->additionalApp, "make_shared");
            for (auto [sensorId, needP1] : result->nodesNeed.needP1Node) {
                if (needP1) {
                    auto p = result->additionalHal[sensorId] = std::make_shared<IMetadata>();
                    MY_LOGF_IF(!p, "make_shared");
                }
            }
        }

        // update Metdata
        for (auto const& [sensorId, pAdditionalHal] : result->additionalHal)
        {
            if (!pAdditionalHal)
                continue;

//            (*pAdditionalHal) += (*nddOut.pNDDHalMeta);
            IMetadata::setEntry<MINT32>(pAdditionalHal.get(), MTK_PIPELINE_UNIQUE_KEY, mUniqueKey);
            IMetadata::setEntry<MUINT8>(pAdditionalHal.get(), MTK_HAL_REQUEST_REPEAT, in.pRequest_ParsedAppMetaControl->repeating);
            IMetadata::setEntry<MSize>(pAdditionalHal.get(), MTK_HAL_REQUEST_SENSOR_SIZE, in.pSensorSize->at(sensorId));
            IMetadata::setEntry<MINT32>(pAdditionalHal.get(), MTK_HAL_REQUEST_SENSOR_ID, sensorId);
            IMetadata::setEntry<MINT32>(pAdditionalHal.get(), MTK_HAL_REQUEST_DEVICE_ID, mPipelineStaticInfo->openId);
        }

        out.mainFrame = result;
        MY_LOGD("build out frame param -");
    };

    buildOutParam();
    return 0;
}

