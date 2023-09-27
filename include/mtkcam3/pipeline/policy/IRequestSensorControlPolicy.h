/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_IREQUESTSENSORCONTROLPOLICY_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_IREQUESTSENSORCONTROLPOLICY_H_
//
#include "types.h"

#include <functional>
#include <unordered_map>
#include <vector>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace requestsensorcontrol {
enum NotifyType
{
    SetMasterId,
    SetStreamingId,
    UpdateStreamingMeta,
    SwitchDone,
    IspQualitySwitchState,
    NextCapture,
    RequestDone,
    AfSearching,
    ConfigDone,
    Flush
};
/**
 * A structure definition for output parameters
 */
struct Configuration_SensorControl_Params
{
    SensorMap<bool>*                    pOut = nullptr; // Lagging config
    SensorMap<int>*                     pExclusiveGroup = nullptr; // groupId
    PipelineStaticInfo const*           pPipelineStaticInfo = nullptr;
    PipelineUserConfiguration const*    pPipelineUserConfiguration = nullptr;
    SeamlessSwitchFeatureSetting const* pSeamlessSwitchConfiguration = nullptr;
    MINT32                              iMaxP1SupportCount = 2;
};

struct RequestOutputParams
{
    MultiCamReqOutputParams* pMultiCamReqOutputParams = nullptr;

    using StreamIdMap = std::unordered_map<uint32_t, std::vector<StreamId_T>>;
    StreamIdMap* vMetaStreamId_from_CaptureNode_Physical = nullptr;
    StreamIdMap* vImageStreamId_from_CaptureNode_Physical = nullptr;

    StreamIdMap* vMetaStreamId_from_StreamNode_Physical = nullptr;
    StreamIdMap* vImageStreamId_from_StreamNode_Physical = nullptr;

    // Seamless Switch
    bool isSeamlessSwitching = false;
    uint32_t seamlessTargetSensorMode = 0;
    bool isLosslessZoom = false;
    uint32_t losslessZoomSensorMode = 0;
};


/**
 * A structure definition for input parameters
 */
struct RequestInputParams
{
    /**************************************************************************
     * Request parameters
     *
     * The parameters related to this capture request is shown as below.
     *
     **************************************************************************/

    uint32_t                                    requestNo = 0;
    /**
     * Request App metadata control, sent at the request stage.
     *
     * pRequest_ParsedAppMetaControl is a partial parsed result from pRequest_AppControl,
     * just for the purpose of a quick reference.
     */
    IMetadata const*                            pRequest_AppControl = nullptr;
    ParsedMetaControl const*                    pRequest_ParsedAppMetaControl = nullptr;
    SensorMap<uint32_t> const*                  pRequest_SensorMode = nullptr;

    bool                                        needP2CaptureNode = false;
    bool                                        needP2StreamNode = false;

    bool                                        needFusion = false;
    std::vector<int32_t>                        physicalSensorIdList;

    SensorMap<P1HwSetting> const*               pvP1HwSetting = nullptr;
    SensorMap<bool> const*                      pNeedP1Nodes = nullptr;
    // logical
    bool                                        bLogicalCaptureOutput = false;
    bool                                        bLogicalRawOutput = false;
    bool                                        bPhysicalRawOutput = false;
    bool                                        bIsHWReady = false;
};

class IRequestSensorControlPolicy
{
public:
    virtual ~IRequestSensorControlPolicy() = default;

    virtual auto    evaluateConfiguration(
                        Configuration_SensorControl_Params params
                    ) -> int                                                = 0;
    /**
     * The policy is in charge of reporting its requirement at the request stage.
     *
     * @param[in/out] params:
     *  Callers must ensure its content.
     *
     * @return
     *      true indicates success; otherwise failure.
     */
    virtual auto    evaluateRequest(
                        requestsensorcontrol::RequestOutputParams& /*out*/,
                        requestsensorcontrol::RequestInputParams const& /*in*/
                    ) -> int                                                = 0;
    virtual auto    sendPolicyDataCallback(
                        MUINTPTR arg1,
                        MUINTPTR arg2,
                        MUINTPTR arg3
                    ) -> bool                                               = 0;
};

//==============================================================================

/**
 * A structure for creation parameters.
 */
struct  CreationParams
{
    std::shared_ptr<PipelineStaticInfo const>       pPipelineStaticInfo;
    std::shared_ptr<PipelineUserConfiguration const>pPipelineUserConfiguration;
};

// multicam version
std::shared_ptr<requestsensorcontrol::IRequestSensorControlPolicy>
        makePolicy_RequestSensorControl_Multicam_Zoom(
                                    CreationParams const& params);


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace requestsensorcontrol
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_IREQUESTSENSORCONTROLPOLICY_H_

