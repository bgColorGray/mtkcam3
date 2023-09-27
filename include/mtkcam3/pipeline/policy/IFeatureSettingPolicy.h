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

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_IFEATURESETTINGPOLICY_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_IFEATURESETTINGPOLICY_H_
//
#include "types.h"
//
//// MTKCAM
#include <mtkcam/def/common.h>
#include <mtkcam/utils/sys/Cam3CPUCtrl.h>
// MTKCAM3
#include <mtkcam3/feature/eis/EisInfo.h>
#include <mtkcam3/plugin/streaminfo/types.h>
//
#include <map>
#include <memory>
#include <vector>


using namespace NSCam::v3::pipeline::policy;
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {


/**
 * Used in the following structures:
 *      RequestOutputParams
 */
struct RequestResultParams
{
    /**
     * P1 DMA need, sent at the request stage.
     *
     * The value shows which dma are needed.
     * For example,
     *      (needP1Dma.at(1) & P1_IMGO) != 0 indicates that IMGO is needed for sensor id 1.
     *      (needP1Dma.at(3) & P1_RRZO) != 0 indicates that RRZO is needed for sensor id 3.
     */
    SensorMap<uint32_t>                         needP1Dma;

    /**
     * Using to decide current camera id need P2 or not.
     *
     * The value shows which dma are needed.
     * For example,
     *      (needP2Process.at(1) & P2_STREAMING) != 0 indicates that P2Streaming is needed for sensorId 1.
     *      (needP2Process.at(3) & P2_STREAMING) != 0 indicates that P2Streaming is needed for sensorId 3.
     */
    SensorMap<uint32_t>                         needP2Process;

    /**
     * keep tuning setting.
     *
     * The value shows the requirement of keeping tuning setting.
     */
    bool                                        needKeepP1BuffersForAppReprocess = false;
    /**
     * Additional metadata
     *
     * The design should avoid to override the app metadata control as possible as we can.
     */
    std::shared_ptr<IMetadata>                  additionalApp;
    // for multicam case, it will contain more than one hal metadata.
    SensorMap<std::shared_ptr<IMetadata>>       additionalHal;

    /**
     * for seamless remosaic capture
     */
    bool                                        needSeamlessRemosaicCapture = false;
};

/**
 *  Input data for update IStreamInfo.privateData.
 */
struct UpdateStreamPrivateDataInput
{
    uint32_t   dataId = 0;
    plugin::streaminfo::PrivateDataT const *data = nullptr;
};

/**
 *  Per-frame stream info's private data can be updated by FeaturePolicy demand.
 *  For example, SMVR batch may need update buffer layout in preview so that P1/P2 only see 1 buffer in stream
 *  If return std::nullopt, the private data & stream info will not be updated
 */
using FunctionType_UpdatePrivateData
    = std::function<std::optional<plugin::streaminfo::PrivateDataT>(
        UpdateStreamPrivateDataInput const& /*in*/
    )>;

/**
 * Used on the output of evaluateRequest().
 *
 * Request-stage policy
 *  $ Need to re-configure or not?
 *  $ New sensor mode settings if changed (e.g. 4cell)
 *  $ The frame sequence: pre-dummy frames, main frame, sub frames, post-dummy frames
 *  $ IMGO / RRZO / RSSO settings
 *    . Process RAW (e.g. type3 PD sensor)
 *    . format: UFO/Unpack RAW/Pack RAW (e.g. HDR)
 *    . size (e.g. streaming)
 *  $ Frame rate change (via metadata) (e.g. 60fps capture for special sensors)
 *  $ ZSL flow or non-ZSL flow
 *  $ ZSL selection policy
 *  $ ......
 */
struct RequestOutputParams
{
    /**
     * If this is true, it means this policy requests to re-configure the pipeline. In this case,
     * all the following output results are evaluated based on the after-reconfiguration setting,
     * not the before-reconfiguration setting.
     */
    bool                                        needReconfiguration = false;

    /**
     * The frame sequence is as below:
     *       pre dummy frame 0
     *       pre dummy frame ...
     *       pre dummy frame X-1
     *       pre   sub frame 0
     *       pre   sub frame ...
     *       pre   sub frame P-1
     *            main frame        (should be aligned to the request sent from users)
     *             sub frame 0
     *             sub frame ...
     *             sub frame Y-1
     *      post dummy frame 0
     *      post dummy frame ...
     *      post dummy frame Z-1
     *
     * The policy module is in charge of allocating the memory when needed.
     */
    std::shared_ptr<RequestResultParams>                mainFrame;
    std::vector<std::shared_ptr<RequestResultParams>>   subFrames;
    std::vector<std::shared_ptr<RequestResultParams>>   preSubFrames;
    std::vector<std::shared_ptr<RequestResultParams>>   preDummyFrames;     //  need dummy frames
    std::vector<std::shared_ptr<RequestResultParams>>   postDummyFrames;    //  need dummy frames

    /**
     * ZSL still capture flow is needed if true; otherwise not needed.
     */
    bool                                        needZslFlow = false;
    ZslPolicyParams                             zslPolicyParams = {};

    /**
     * CShot
     */
    bool                                        bCshotRequest = false;

    /**
     * Drop previous preview request for speed-up this request's exposure behavior.
     * (only support drop one preview request)
     */
    bool                                        bDropPreviousPreviewRequest  = false;

    // boost BWC
    std::vector<BoostControl>                   vboostControl;

    /**
     * Hint pipeline to set cpu profile setting
     */
    Cam3CPUCtrl::ENUM_CAM3_CPU_CTRL_FPSGO_PROFILE            CPUProfile = Cam3CPUCtrl::E_CAM3_CPU_CTRL_FPSGO_DISABLE;

    /**
     * Reconfig Category
     * 0:No reconfig, 1:Stream reconfig, 2:Capture reconfig
     */
    ReCfgCtg                                    reconfigCategory = ReCfgCtg::NO;

    /**
     * BatchPolicy
     */
    BatchPolicy                                 batchPolicy;

    /**
     *  The result sensor setting
     */
    SensorMap<uint32_t>                         sensorMode;

    /**
     *  The stream list that to skip P2Streaming
     */
    std::vector<StreamId_T>                     skipP2StreamList;

    /**
     * [TODO]
     *
     *  $ New sensor mode settings if changed (e.g. 4cell)
     *  $ IMGO / RRZO / RSSO settings
     *    . Process RAW (e.g. type3 PD sensor)
     *    . format: UFO/Unpack RAW/Pack RAW (e.g. HDR)
     *    . size (e.g. streaming)
     *  $ ZSL flow or non-ZSL flow
     *  $ ZSL selection policy
     *  $ ......
     */
     /**
     * Need P1 output unpacked raw format.
     */
    bool                                        needUnpackRaw   = false;
    bool                                        keepZslBuffer   = true;

    SensorMap<MSize>                            fixedRRZOSize;

    /**
     * StreamInfo's Private Data may need to be updated by feature requirement.
     * E.g.  SMVR batch mode need modify buffer layout to "1" during SMVR preview.
     * Thie function will be called by pipeline policy to do private data updated if needed.
     * In the future, RRZO/IMGO/STT stream info may all call this function to do update.
     */
    SensorMap<FunctionType_UpdatePrivateData>   funcUpdatePrivateData;

    // Seamless Switch
    bool                                        isSeamlessCaptureRequest = false;
    bool                                        isSeamlessPreviewRequest = false;
};


/**
 * Used on the input of evaluateRequest().
 */
struct RequestInputParams
{
    /**************************************************************************
     * Request parameters
     *
     * The parameters related to this capture request is shown as below.
     *
     **************************************************************************/

    /**
     * Request number, sent at the request stage.
     *
     */
    uint32_t                                    requestNo = 0;

    /**
     * Request App metadata control, sent at the request stage.
     *
     * pRequest_ParsedAppMetaControl is a partial parsed result from pRequest_AppControl,
     * just for the purpose of a quick reference.
     */
    IMetadata const*                            pRequest_AppControl = nullptr;
    ParsedMetaControl const*                    pRequest_ParsedAppMetaControl = nullptr;

    ParsedAppImageStreamInfo const*             pRequest_AppImageStreamInfo = nullptr;

    /**
     * configure stage data
     */
    SensorMap<ParsedStreamInfo_P1> const*       pConfiguration_StreamInfo_P1 = nullptr;
    bool                                        Configuration_HasRecording = false;

    /**
     * request hint for p2 feature pipeline nodes. (from P2NodeDecisionPolicy)
     *
     * true indicates its corresponding request stream out from
     * dedicated feature pipeline node during the request stage.
     */
    bool                                        needP2CaptureNode = false;
    MSize                                       maxP2CaptureSize;
    bool                                        needP2StreamNode = false;
    MSize                                       maxP2StreamSize;

    IMetadata const*                            pNDDMetadata = nullptr;

    /**
     * request hint for output RAW buffer for App.
     */
    bool                                        needRawOutput = false;

    /**
     * request hint for output P2C output YUV
     */
    bool                                        needYuvOutputFromP2C = false;

    /**************************************************************************
     * Current Setting
     *
     **************************************************************************/
    /**
     *  The current sensor setting
     */
    SensorMap<uint32_t>                         sensorMode;

    /**************************************************************************
     * multi-cam used hal metadata control
     **************************************************************************/
    MultiCamReqOutputParams const*              pMultiCamReqOutputParams = nullptr;

    /**
     * Seamless Switch
     * @param pConfiguration_SeamlessSwitch
     * only logical feature setting will do correct parsing of seamless
     * switch info, so we need to import as input and cannot store and use as a
     * member data.
     * @param bIsHWReady
     * seamless switching is available after hw (p1/p1drv) returns ready hint.
     * @param bIsMultiCamSeamless
     * currently, in-sensor-zoom flow control on multi-cam device is implemented
     * in sensor control, we only need to update sensor mode in feature setting.
     * @param bIsSeamlessSwitching
     * @param seamlessTargetSensorMode
     * is available during bIsSeamlessSwitching is true.
     * @param bIsLosslessZoom
     * @param losslessZoomSensorMode
     * is available during bIsLosslessZoom is true.
     * @note bIsLosslessZoom and losslessZoomSensorMode are only describing
     * main-cam in multi-cam device.
     */
    SeamlessSwitchFeatureSetting const*         pConfiguration_SeamlessSwitch = nullptr;
    bool                                        bIsHWReady = false;
    bool                                        bIsMultiCamSeamless = false;
    bool                                        bIsSeamlessSwitching = false;
    uint32_t                                    seamlessTargetSensorMode = 0;
    bool                                        bIsLosslessZoom = false;
    uint32_t                                    losslessZoomSensorMode = 0;
};

struct ConfigurationInputParams
{
    bool                                        isP1DirectFDYUV = false;
    MINT32                                      numsP1YUV = -1;
    MINT32                                      uniqueKey = 0;

    // config for capture/streaming quality output.
    bool                                        needCaptureQuality = true;   // need high quality for capture
    bool                                        needStreamingQuality = true; // need high performance for streaming

    /**
     * Reconfig Category
     * 0:No reconfig, 1:Stream reconfig, 2:Capture reconfig
     */
    ReCfgCtg                                    reconfigCategory = ReCfgCtg::NO;
};


/**
 * Used on the output of evaluateConfiguration().
 *
 * At the configuration stage, capture feature and streaming feature policy will
 * output their both requirements.
 */
struct ConfigurationOutputParams
{
    StreamingFeatureSetting                     StreamingParams;
    CaptureFeatureSetting                       CaptureParams;
    SeamlessSwitchFeatureSetting                SeamlessSwitchParams;

    /**
     * Is Zsl mode configured or not?
     */
    bool                                        isZslMode = false;
};


/**
 *
 */
class IFeatureSettingPolicy
{
public:
    virtual         ~IFeatureSettingPolicy() = default;

    /**
    * The policy is in charge of reporting its requirement at the configuration stage.
    *
    * @param[in] in:
    *  Callers must promise its content. The callee is not allowed to modify it.
    *
    * @param[out] out:
    *  On this call, the callee must allocate and set up its content.
    *
    * @return
    *      true indicates success; otherwise failure.
    */
    virtual auto    evaluateConfiguration(
                        ConfigurationOutputParams* out,
                        ConfigurationInputParams const* in
                    ) -> int                                                = 0;

    /**
     * The policy is in charge of reporting its requirement at the request stage.
     *
     * @param[out] out:
     *  Callers must ensure it's a non-nullptr.
     *
     * @param[in] in:
     *  Callers must promise its content. The callee is not allowed to modify it.
     *
     * @return
     *      true indicates success; otherwise failure.
     */
    virtual auto    evaluateRequest(
                        RequestOutputParams* out,
                        RequestInputParams const* in
                    ) -> int                                                = 0;

};

/**
 * A structure for creation parameters.
 */
struct  CreationParams
{
    // info for all feature
    std::shared_ptr<PipelineStaticInfo const>       pPipelineStaticInfo;

    // info for streaming feature
    std::shared_ptr<PipelineUserConfiguration const>pPipelineUserConfiguration;

    // using to hint current FeatureSettingPolicy is logical type or not.
    bool bIsLogicalCam = true;
    // TODO: use the struct for expandability.
};

auto createFeatureSettingPolicyInstance(
    CreationParams const& params
) -> std::shared_ptr<IFeatureSettingPolicy>;

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_IFEATURESETTINGPOLICY_H_

