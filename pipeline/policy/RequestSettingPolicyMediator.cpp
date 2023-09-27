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

#define LOG_TAG "mtkcam-RequestSettingPolicyMediator"

#include <algorithm> // std::find

#include <mtkcam3/pipeline/policy/IRequestSettingPolicyMediator.h>
#include <mtkcam3/pipeline/policy/InterfaceTableDef.h>
#include <mtkcam3/pipeline/utils/streaminfo/ImageStreamInfo.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/hw/HwTransform.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/cat/CamAutoTestLogger.h>
// for 60 fps +
#include <mtkcam/aaa/IHal3A.h>
// for 60 fps -

#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::pipeline;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::v3::pipeline::policy::pipelinesetting;
using namespace NSCam::v3::Utils;
using namespace NSCamHW;


#define ThisNamespace   RequestSettingPolicyMediator_Default

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

#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//


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
    std::shared_ptr<IStreamInfoBuilderFactory const>mAppStreamInfoBuilderFactory;

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

private:
    virtual auto    getSensorIndexBySensorId(
                        uint32_t sensorId,
                        uint32_t &index
                    ) -> bool;
    virtual auto    combineFeatureParam(
                        featuresetting::RequestOutputParams &target,
                        uint32_t source_id,
                        featuresetting::RequestOutputParams &source
                    ) -> bool;
    virtual auto    updateSensorModeToOutFeatureParam(
                        featuresetting::RequestOutputParams &out,
                        uint32_t sensorMode,
                        uint32_t sensorIndex
                    ) -> bool;
    virtual auto    updateBoostControlToOutFeatureParam(
                        featuresetting::RequestOutputParams &out,
                        BoostControl boostControl
                    ) -> bool;
    virtual auto    updateRequestResultParams(
                        std::shared_ptr<featuresetting::RequestResultParams> &target,
                        std::shared_ptr<featuresetting::RequestResultParams> &slaveOut,
                        uint32_t sensorId
                    ) -> bool;
    virtual auto    getNeededSensorListForPhysicalFeatureSetting(
                        RequestInputParams const& in,
                        p2nodedecision::RequestOutputParams const& p2DecisionOut,
                        std::vector<uint32_t> const& vStreamingSensorList,
                        std::vector<uint32_t> const& vPrvStreamingSensorList
                    ) -> std::vector<uint32_t>;

private:
    bool                                            misFdEnabled = false;
    bool                                            mpreviousIsCapture = false;

    // for 60 fps +
    int                                             mwaitToResetCnt = 0;
    NS3Av3::IHal3A*                                 mp3AHal = nullptr;
    int                                             mLogLevel = 0;
    int                                             mDebugMode = -1;
    // for 60 fps -
};


/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<IRequestSettingPolicyMediator>
makeRequestSettingPolicyMediator_Default(MediatorCreationParams const& params)
{
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
    , mAppStreamInfoBuilderFactory(params.pAppStreamInfoBuilderFactory)
{
    // for 60 fps +
    mLogLevel = ::property_get_int32("persist.vendor.debug.camera.policy.log", 0);
    mDebugMode = ::property_get_int32("persist.vendor.debug.camera.policy.mode", -1);
    // for 60 fps -
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
evaluateRequest(
    RequestOutputParams& out,
    RequestInputParams const& in
) -> int
{
    CAM_TRACE_CALL();
    NSCam::Utils::CameraProfile profile(LOG_TAG, __FUNCTION__);

    fdintent::RequestOutputParams fdOut;
    fdintent::RequestInputParams fdIn;
    nddmeta::RequestOutputParams nddOut;
    nddmeta::RequestInputParams nddIn;
    p2nodedecision::RequestOutputParams p2DecisionOut;
    p2nodedecision::RequestInputParams p2DecisionIn;

    // logical stream feature setting
    featuresetting::RequestOutputParams featureOut;
    featuresetting::RequestInputParams featureIn;

    // physical stream feature setting
    std::unordered_map<uint32_t, featuresetting::RequestOutputParams> vFeatureOut_Physical;
    capturestreamupdater::RequestOutputParams capUpdaterOut;
    capturestreamupdater::RequestInputParams capUpdaterIn;
    android::sp<IImageStreamInfo>              pJpeg_YUV = nullptr;
    android::sp<IImageStreamInfo>              pThumbnail_YUV = nullptr;
    // [Jpeg pack]
    android::sp<IImageStreamInfo>              pJpeg_Sub_YUV = nullptr;
    android::sp<IImageStreamInfo>              pDepth_YUV = nullptr;
    bool                                       isAppRawOutFromP2C = false;

    // For RequestSensorControlPolicy
    requestsensorcontrol::RequestOutputParams reqSensorControlOut;
    requestsensorcontrol::RequestInputParams reqSensorControlIn;

    bool noP2Node = !in.pConfiguration_PipelineNodesNeed->needP2CaptureNode && !in.pConfiguration_PipelineNodesNeed->needP2StreamNode;
    uint32_t main1SensorId = mPipelineStaticInfo->sensorId[0];

    // (1) is face detection intent?
    if (CC_LIKELY(mPolicyTable->fFaceDetectionIntent != nullptr))
    {
        fdIn.hasFDNodeConfigured = in.pConfiguration_PipelineNodesNeed->needFDNode;
        fdIn.isFdEnabled_LastFrame = misFdEnabled;
        fdIn.pRequest_AppImageStreamInfo = in.pRequest_AppImageStreamInfo;
        fdIn.pRequest_AppControl = in.pRequest_AppControl;
        fdIn.pRequest_ParsedAppMetaControl = in.pRequest_ParsedAppMetaControl;
        mPolicyTable->fFaceDetectionIntent(fdOut, fdIn);
        if (fdOut.hasFDMeta)
        {
            misFdEnabled = fdOut.isFDMetaEn;
        }
    }
    // (1.1) update NDD metadata
    if (CC_LIKELY(mPolicyTable->fNDDMetadata != nullptr))
    {
        nddIn.requestNo = in.requestNo;
        nddIn.uniKey = in.mUniKey;
        nddIn.pRequest_AppControl = in.pRequest_AppControl;
        nddIn.pRequest_ParsedAppMetaControl = in.pRequest_ParsedAppMetaControl;

        mPolicyTable->fNDDMetadata(nddOut, nddIn);
    }

    // (2.1) are any capture streams updated?
    if ( in.pRequest_AppImageStreamInfo->pAppImage_Jpeg != nullptr && CC_LIKELY(mPolicyTable->fCaptureStreamUpdater != nullptr) )
    {
        MINT imgFormat = in.pRequest_AppImageStreamInfo->pAppImage_Jpeg->getImgFormat();
        capUpdaterIn.sensorID                        = main1SensorId;
        capUpdaterIn.pRequest_AppControl             = in.pRequest_AppControl;
        capUpdaterIn.pRequest_ParsedAppMetaControl   = in.pRequest_ParsedAppMetaControl;
        capUpdaterIn.isJpegRotationSupported         = (imgFormat == eImgFmt_JPEG || imgFormat == eImgFmt_HEIF) ? true : false; // use property to control is support or not?
        capUpdaterIn.pConfiguration_HalImage_Jpeg_YUV      = &(in.pConfiguration_StreamInfo_NonP1->pHalImage_Jpeg_YUV);
        capUpdaterIn.pConfiguration_HalImage_Thumbnail_YUV = &(in.pConfiguration_StreamInfo_NonP1->pHalImage_Thumbnail_YUV);
        // prepare out buffer
        capUpdaterOut.pHalImage_Jpeg_YUV             = &pJpeg_YUV;
        capUpdaterOut.pHalImage_Thumbnail_YUV        = &pThumbnail_YUV;
        // [Jpeg pack] 0 means fine to capture bokeh image.
        {
            if(in.pConfiguration_StreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV != nullptr)
            {
                capUpdaterIn.pConfiguration_HalImage_Jpeg_Sub_YUV  = &(in.pConfiguration_StreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV);
                capUpdaterIn.isSupportJpegPack = true;
            }
            if(in.pConfiguration_StreamInfo_NonP1->pHalImage_Depth_YUV != nullptr)
            {
                capUpdaterIn.pConfiguration_HalImage_Depth_YUV = &(in.pConfiguration_StreamInfo_NonP1->pHalImage_Depth_YUV);
            }
            capUpdaterOut.pHalImage_Jpeg_Sub_YUV         = &pJpeg_Sub_YUV;
            capUpdaterOut.pHalImage_Depth_YUV            = &pDepth_YUV;
        }
        mPolicyTable->fCaptureStreamUpdater(capUpdaterOut, capUpdaterIn);
    }

    // (2.2) P2Node decision: the responsibility of P2StreamNode and P2CaptureNode
    if (CC_LIKELY(mPolicyTable->fP2NodeDecision != nullptr))
    {
        p2DecisionIn.requestNo                       = in.requestNo;
        p2DecisionIn.hasP2CaptureNode                = in.pConfiguration_PipelineNodesNeed->needP2CaptureNode;
        p2DecisionIn.hasP2StreamNode                 = in.pConfiguration_PipelineNodesNeed->needP2StreamNode;
        if (in.pRequest_ParsedAppMetaControl->control_captureIntent == MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE
         && in.pRequest_ParsedAppMetaControl->control_enableZsl)
        {
            MY_LOGD("isFDMetaEn : %d", fdOut.isFDMetaEn);
            p2DecisionIn.isFdEnabled                 = false;
        }
        else
        {
            p2DecisionIn.isFdEnabled                 = fdOut.isFDMetaEn && !mPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV;
        }
        p2DecisionIn.pConfiguration_StreamInfo_NonP1 = in.pConfiguration_StreamInfo_NonP1;
        p2DecisionIn.pConfiguration_StreamInfo_P1    = &((*(in.pConfiguration_StreamInfo_P1)).at(main1SensorId)); // use main1 info
        p2DecisionIn.pRequest_AppControl             = in.pRequest_AppControl;
        p2DecisionIn.pRequest_AppImageStreamInfo     = in.pRequest_AppImageStreamInfo;
        p2DecisionIn.Configuration_HasRecording      = mPipelineUserConfiguration->pParsedAppImageStreamInfo->hasVideoConsumer;
        p2DecisionIn.pRequest_ParsedAppMetaControl   = in.pRequest_ParsedAppMetaControl;
        p2DecisionIn.needThumbnail                   = (pThumbnail_YUV != nullptr);
        mPolicyTable->fP2NodeDecision(p2DecisionOut, p2DecisionIn);
    }

    // (2.2.5) check sensor control policy
    if (mPolicyTable->pRequestSensorControlPolicy != nullptr)
    {
        std::vector<int32_t> physicalStreamSensorList;
        if(in.pRequest_AppImageStreamInfo->outputPhysicalSensors.size() > 0)
        {
            for (size_t i = 0; i < in.pRequest_AppImageStreamInfo->outputPhysicalSensors.size(); i++)
            {
                physicalStreamSensorList.push_back(in.pRequest_AppImageStreamInfo->outputPhysicalSensors[i]);
            }
        }

        reqSensorControlIn.requestNo           = in.requestNo;
        reqSensorControlIn.pRequest_AppControl = in.pRequest_AppControl;
        reqSensorControlIn.pRequest_ParsedAppMetaControl = in.pRequest_ParsedAppMetaControl;
        reqSensorControlIn.pRequest_SensorMode = in.pSensorMode;
        reqSensorControlIn.pvP1HwSetting = in.pP1HwSetting;
        reqSensorControlIn.pNeedP1Nodes = &in.pConfiguration_PipelineNodesNeed->needP1Node;
        reqSensorControlIn.needP2CaptureNode = p2DecisionOut.needP2CaptureNode;
        reqSensorControlIn.needP2StreamNode = p2DecisionOut.needP2StreamNode;
        reqSensorControlIn.needFusion = true;
        reqSensorControlIn.physicalSensorIdList = physicalStreamSensorList;
        // if contains capture yuv or needs output logical Raw, set flag to true.
        reqSensorControlIn.bLogicalCaptureOutput = (p2DecisionOut.vImageStreamId_from_CaptureNode.size()>0)?true:false;
        reqSensorControlIn.bLogicalRawOutput = (
            in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0 ||
            in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() > 0 )
            ? true : false;
        reqSensorControlIn.bPhysicalRawOutput = (
            in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.size()>0 ||
            in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.size() > 0 )
            ? true : false;
        reqSensorControlIn.bIsHWReady = in.isP1HwReady;
        reqSensorControlOut.vMetaStreamId_from_CaptureNode_Physical= &p2DecisionOut.vMetaStreamId_from_CaptureNode_Physical;
        reqSensorControlOut.vImageStreamId_from_CaptureNode_Physical= &p2DecisionOut.vImageStreamId_from_CaptureNode_Physical;
        reqSensorControlOut.vMetaStreamId_from_StreamNode_Physical= &p2DecisionOut.vMetaStreamId_from_StreamNode_Physical;
        reqSensorControlOut.vImageStreamId_from_StreamNode_Physical= &p2DecisionOut.vImageStreamId_from_StreamNode_Physical;
        reqSensorControlOut.pMultiCamReqOutputParams = &out.multicamReqOutputParams;
        int ret = mPolicyTable->pRequestSensorControlPolicy->evaluateRequest(reqSensorControlOut, reqSensorControlIn);
        if (ret == -1)
        {
            MY_LOGE("RequestSensorControlPolicy has evaluateRequest Error. Jump rest part");
            return OK;
        }
        // for loslesszoom, reset seamless sensor mode to configured sensor mode
        auto pParam = reqSensorControlOut.pMultiCamReqOutputParams;
        auto iter = std::find(pParam->configSensorIdList.begin(),pParam->configSensorIdList.end(),main1SensorId);
    }
    else // single
    {
        out.multicamReqOutputParams.streamingSensorList.push_back(mPipelineStaticInfo->openId);
        bool singleCamCapture = p2DecisionOut.needP2CaptureNode
                                 || in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0
                                 || in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() > 0;

        if(singleCamCapture)
        {
            auto sensorId = mPipelineStaticInfo->openId;

            // If opening logical multicam but using single cam flow, the openId
            // isn't physical. It should be change to master cam to capture.
            if(mPipelineStaticInfo->getIndexFromSensorId(sensorId) < 0)
            {
                sensorId = mPipelineStaticInfo->sensorId[0];
                MY_LOGD("Use main cam(%d) to capture", sensorId);
            }

            out.multicamReqOutputParams.prvStreamingSensorList.push_back(sensorId);
            out.multicamReqOutputParams.prvMasterId = sensorId;
        }
    }

    // (2.3) feature setting
    if (CC_LIKELY(mPolicyTable->mFeaturePolicy != nullptr))
    {
        featureIn.requestNo                          = in.requestNo;
        featureIn.Configuration_HasRecording         = mPipelineUserConfiguration->pParsedAppImageStreamInfo->hasVideoConsumer;
        featureIn.maxP2CaptureSize                   = p2DecisionOut.maxP2CaptureSize;
        featureIn.maxP2StreamSize                    = p2DecisionOut.maxP2StreamSize;
        featureIn.needP2CaptureNode                  = p2DecisionOut.needP2CaptureNode;
        featureIn.needP2StreamNode                   = p2DecisionOut.needP2StreamNode;
        featureIn.pConfiguration_StreamInfo_P1       = in.pConfiguration_StreamInfo_P1;
        featureIn.pRequest_AppControl                = in.pRequest_AppControl;
        featureIn.pRequest_AppImageStreamInfo        = in.pRequest_AppImageStreamInfo;
        featureIn.pRequest_ParsedAppMetaControl      = in.pRequest_ParsedAppMetaControl;
        if (mPolicyTable->pRequestSensorControlPolicy != nullptr)
        {
            featureIn.pMultiCamReqOutputParams           = &out.multicamReqOutputParams;
        }
        featureIn.needRawOutput                      = in.pRequest_AppImageStreamInfo->hasRawOut;
        featureIn.needYuvOutputFromP2C               = p2DecisionOut.needYuvOutputFromP2C;
        featureIn.pNDDMetadata                       = nddOut.pNDDHalMeta.get();

        for (const auto& it : in.pConfiguration_PipelineNodesNeed->needP1Node)
        {
            if (it.second == true) {
                featureIn.sensorMode.emplace(it.first, in.pSensorMode->at(it.first));
            }
        }

        bool needLogicalCapture =
                (p2DecisionOut.vImageStreamId_from_CaptureNode.size() > 0)
                 || (in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0)
                 || (in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() > 0)
                 || (in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv.get())
                ;

        bool needLogicalFeatureSetting =
                (mPolicyTable->pRequestSensorControlPolicy == nullptr) // means single
                 || needLogicalCapture;

        bool needPhysicalFeatureSetting =
                mPolicyTable->pRequestSensorControlPolicy != nullptr;

        if (needLogicalFeatureSetting || needPhysicalFeatureSetting) {
            if (featureIn.needP2CaptureNode || featureIn.needRawOutput) {
                // Log for Auto Test tool
                CAT_LOGD("[CAT][Event] Capture:Stamp (reqNo:%u)", in.requestNo);
            }
        }

        // Seamless Switch for multi-cam device
        featureIn.pConfiguration_SeamlessSwitch      = in.pConfiguration_SeamlessSwitch;
        featureIn.bIsHWReady                         = in.isP1HwReady;
        if (mPolicyTable->pRequestSensorControlPolicy) {
            featureIn.bIsMultiCamSeamless = true;
            featureIn.bIsSeamlessSwitching = reqSensorControlOut.isSeamlessSwitching;
            featureIn.seamlessTargetSensorMode = reqSensorControlOut.seamlessTargetSensorMode;
            featureIn.bIsLosslessZoom = reqSensorControlOut.isLosslessZoom;
            featureIn.losslessZoomSensorMode = reqSensorControlOut.losslessZoomSensorMode;
        }

        profile.stopWatch();

        if(needLogicalFeatureSetting)
        {
            // call logical feature setting policy.
            if (CC_LIKELY(mPolicyTable->mFeaturePolicy != nullptr))
            {
                MY_LOGD_IF(mLogLevel,"logical capture update [%" PRId32 "]",
                in.requestNo);
                mPolicyTable->mFeaturePolicy->evaluateRequest(&featureOut, &featureIn);
            }
        }

        if(needPhysicalFeatureSetting)
        {
            std::vector<uint32_t> neededPhysicalSensorList =
                getNeededSensorListForPhysicalFeatureSetting(in, p2DecisionOut,
                                                            out.multicamReqOutputParams.streamingSensorList,
                                                            out.multicamReqOutputParams.prvStreamingSensorList);

            for (const auto& sensorId : neededPhysicalSensorList)
            {
                MY_LOGD_IF(mLogLevel,"[%" PRId32 "] physical update "
                "[%" PRId32 "]", sensorId, in.requestNo);

                auto p = mPolicyTable->mFeaturePolicy_Physical.find(sensorId);
                if (p != mPolicyTable->mFeaturePolicy_Physical.end() && CC_LIKELY(p->second != nullptr))
                {
                    featuresetting::RequestOutputParams slaveOutParam;
                    p->second->evaluateRequest(&slaveOutParam, &featureIn);

                    // check specific id already exist or not.
                    auto iter = vFeatureOut_Physical.find(sensorId);
                    if(iter == vFeatureOut_Physical.end())
                    {
                        vFeatureOut_Physical.emplace(sensorId, slaveOutParam);
                    }
                    else
                    {
                        combineFeatureParam(iter->second, sensorId, slaveOutParam);
                    }
                }
            }
        }

        // combine master & slave result
        for(auto& [_sensorId, _featureOut_phy] : vFeatureOut_Physical)
        {
            combineFeatureParam(featureOut, _sensorId, _featureOut_phy);
        }

        profile.print_overtime('W', 10000000/*10ms*/, "FeatureSettingPolicy::evaluateRequest");

        out.needZslFlow                              = featureOut.needZslFlow;
        out.zslPolicyParams                          = featureOut.zslPolicyParams;
        out.bCshotRequest                            = featureOut.bCshotRequest;
        out.needReconfiguration                      = featureOut.needReconfiguration;
        out.sensorMode                               = featureOut.sensorMode;
        out.reconfigCategory                         = featureOut.reconfigCategory;
        out.vboostControl                            = featureOut.vboostControl;
        out.CPUProfile                               = featureOut.CPUProfile;
        out.keepZslBuffer                            = featureOut.keepZslBuffer;
        out.bkickRequest                             = featureOut.bDropPreviousPreviewRequest && !mpreviousIsCapture;
        if (featureOut.bDropPreviousPreviewRequest)
        {
            MY_LOGD("feature want kick request, out.bkickRequest(%d)", out.bkickRequest);
        }
    }

    if (p2DecisionOut.needP2CaptureNode || in.pRequest_AppImageStreamInfo->hasRawOut)
    {
        mpreviousIsCapture = true;
    }
    else
    {
        mpreviousIsCapture = false;
    }

    // if zsl is enable and need output raw, the raw need to be output from P2C
    auto checkIfNeedRefineCaptureNodeDecision = [](featuresetting::RequestOutputParams const& featureOut,
                                                   ParsedAppImageStreamInfo const* pRequest_AppImageStreamInfo,
                                                   bool const& isNeedP2ProcessRaw,
                                                   bool const& isP1DirectAppRaw)
                                                   -> bool
    {
        if(pRequest_AppImageStreamInfo && isP1DirectAppRaw)
        {
            return ((featureOut.needZslFlow || isNeedP2ProcessRaw) && pRequest_AppImageStreamInfo->hasRawOut);
        }
        return false;
    };
    //
    if ( (isAppRawOutFromP2C = checkIfNeedRefineCaptureNodeDecision( featureOut,in.pRequest_AppImageStreamInfo,
                                                                   ((mPipelineStaticInfo->isNeedP2ProcessRaw && in.pRequest_ParsedAppMetaControl->control_processRaw)
                                                                    || (in.isSW4Cell && out.needReconfiguration  && in.pRequest_AppImageStreamInfo->hasRawOut)),
                                                                    mPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectAppRaw)) == true )
    {
        MY_LOGD("need output raw from p2c");
        if(in.pRequest_AppImageStreamInfo->hasRawOut)
        {
            bool captureNodeAddRawOut = false;
            //priv raw
            if(in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv)
            {
                p2DecisionOut.vImageStreamId_from_CaptureNode.push_back(in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv->getStreamId());
                captureNodeAddRawOut = true;
                MY_LOGD("Add Priv raw (%#" PRIxPTR ") to CaptureNode",in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv->getStreamId());
            }

            //logical raw16
            for(const auto& item : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16)
            {
                p2DecisionOut.vImageStreamId_from_CaptureNode.push_back(item.second->getStreamId());
                captureNodeAddRawOut = true;
                MY_LOGD("Add sensor(%d) logical raw16 (%#" PRIx64 ") to CaptureNode", item.first, item.second->getStreamId());
            }

            //logical mipiraw10
            for(const auto& item:in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10)
            {
                auto iter = in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.find(item.first);
                if(iter != in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.end())
                {
                    p2DecisionOut.vImageStreamId_from_CaptureNode.push_back(iter->second->getStreamId());
                    captureNodeAddRawOut = true;
                    MY_LOGD("Add sensor(%d) mipi raw 10 (%#" PRIxPTR ") to CaptureNode",iter->first,iter->second->getStreamId());
                }
            }

            //physical raw16
            for(const auto& item : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical)
            {
                auto prvList = out.multicamReqOutputParams.prvStreamingSensorList;

                if( std::find(prvList.begin(), prvList.end(), item.first)
                        != prvList.end() )
                {
                    std::vector<StreamId_T> temp;

                    for( const auto &n : item.second )
                    {
                        temp.push_back(n->getStreamId());
                        MY_LOGD("Add sensor(%d) physical raw16 (%#" PRIxPTR ") to CaptureNode", item.first, n->getStreamId());
                    }

                    p2DecisionOut.vImageStreamId_from_CaptureNode_Physical.emplace(item.first, temp);
                    captureNodeAddRawOut = true;
                }
            }
            //physical raw10
            for(const auto& item : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical)
            {
                auto prvList = out.multicamReqOutputParams.prvStreamingSensorList;

                if( std::find(prvList.begin(), prvList.end(), item.first)
                        != prvList.end() )
                {
                    std::vector<StreamId_T> temp;

                    for( const auto &n : item.second )
                    {
                        temp.push_back(n->getStreamId());
                        MY_LOGD("Add sensor(%d) physical raw10 (%#" PRIxPTR ") to CaptureNode", item.first, n->getStreamId());
                    }

                    p2DecisionOut.vImageStreamId_from_CaptureNode_Physical.emplace(item.first, temp);
                    captureNodeAddRawOut = true;
                }
            }

            //metadata & set needP2CaptureNode
            if(captureNodeAddRawOut == true)
            {
                p2DecisionOut.vMetaStreamId_from_CaptureNode.push_back(in.pConfiguration_StreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode->getStreamId());
                p2DecisionOut.vMetaStreamId_from_CaptureNode.push_back(in.pConfiguration_StreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode->getStreamId());
                p2DecisionOut.needP2CaptureNode = true;
            }
        }
    }

    if (featureOut.needZslFlow && in.isZSLMode && in.pRequest_AppImageStreamInfo->previewStreamId != -1)
    {
        auto& imageIds_Capture = p2DecisionOut.vImageStreamId_from_CaptureNode;
        auto& imageIds_Stream = p2DecisionOut.vImageStreamId_from_StreamNode;
        auto& imageIds_Capture_Physical = p2DecisionOut.vImageStreamId_from_CaptureNode_Physical;
        auto& imageIds_Stream_Physical = p2DecisionOut.vImageStreamId_from_StreamNode_Physical;
        auto previewStreamId = in.pRequest_AppImageStreamInfo->previewStreamId;

        MY_LOGD("is zsl capture, preview buffer need removed to prevent preview abnormal");
        auto iter_cap = std::find(imageIds_Capture.begin(), imageIds_Capture.end(), previewStreamId);
        if (iter_cap != imageIds_Capture.end())
        {
            MY_LOGD("Remove logical preview buffer from capture");
            out.markErrorStreamList.push_back(*iter_cap);
            imageIds_Capture.erase(iter_cap);
        }
        auto iter_prv = std::find(imageIds_Stream.begin(), imageIds_Stream.end(), previewStreamId);
        if (iter_prv != p2DecisionOut.vImageStreamId_from_StreamNode.end())
        {
            MY_LOGD("Remove logical preview buffer from stream");
            out.markErrorStreamList.push_back(*iter_prv);
            imageIds_Stream.erase(iter_prv);
        }

        // Mark physical preview buffer as error for capture and streaming
        for (auto& Entry : imageIds_Capture_Physical){
            auto& imageIds_Capture = Entry.second;
            auto iter_cap_phy = std::find(imageIds_Capture.begin(), imageIds_Capture.end(), previewStreamId);
            if (iter_cap_phy != imageIds_Capture.end()) {
                MY_LOGD("Remove physical preview buffer from capture");
                out.markErrorStreamList.push_back(*iter_cap_phy);
                imageIds_Capture.erase(iter_cap_phy);
            }
        }

        for (auto& Entry : imageIds_Stream_Physical){
            auto& imageIds_Stream = Entry.second;
            auto iter_cap_phy = std::find(imageIds_Stream.begin(), imageIds_Stream.end(), previewStreamId);
            if (iter_cap_phy != imageIds_Stream.end()) {
                MY_LOGD("Remove physical preview buffer from stream");
                out.markErrorStreamList.push_back(*iter_cap_phy);
                imageIds_Stream.erase(iter_cap_phy);
            }
        }

        if (imageIds_Capture.size() == 0 && imageIds_Capture_Physical.size() == 0) {
            p2DecisionOut.needP2CaptureNode = false;
        }
        if (imageIds_Stream.size() == 0 && imageIds_Stream_Physical.size() == 0) {
            p2DecisionOut.needP2StreamNode = false;
        }
    }

    if (featureOut.needZslFlow && in.isZSLMode
        && in.pRequest_AppImageStreamInfo->pAppImage_Depth != nullptr)
    {
        MY_LOGD("zsl capture, depth buffer need to be removed.");
        auto depthStreamId = in.pRequest_AppImageStreamInfo->pAppImage_Depth->getStreamId();

        auto iter_cap = std::find(
                p2DecisionOut.vImageStreamId_from_CaptureNode.begin(),
                p2DecisionOut.vImageStreamId_from_CaptureNode.end(), depthStreamId);
        if (iter_cap != p2DecisionOut.vImageStreamId_from_CaptureNode.end())
        {
            MY_LOGD("remove depth buffer from CaptureNode");
            p2DecisionOut.vImageStreamId_from_CaptureNode.erase(iter_cap);
            if (p2DecisionOut.vImageStreamId_from_CaptureNode.size() == 0)
            {
                p2DecisionOut.needP2CaptureNode = false;
            }
        }

        auto iter_stream = std::find(
                p2DecisionOut.vImageStreamId_from_StreamNode.begin(),
                p2DecisionOut.vImageStreamId_from_StreamNode.end(), depthStreamId);
        if (iter_stream != p2DecisionOut.vImageStreamId_from_StreamNode.end())
        {
            MY_LOGD("remove depth buffer from StreamNode");
            p2DecisionOut.vImageStreamId_from_StreamNode.erase(iter_stream);
            if (p2DecisionOut.vImageStreamId_from_StreamNode.size() == 0)
            {
                p2DecisionOut.needP2StreamNode = false;
            }
        }
    }

    bool bNeedDepthStreamMarkError = false;
    if (featureOut.needZslFlow && in.isZSLMode && p2DecisionOut.needP2CaptureNode) {
        bNeedDepthStreamMarkError = true;
    }
    else {
        bNeedDepthStreamMarkError = false;
    }

    // If depth stream is marked as error, handle in handleMarkErrorList
    if (in.pRequest_AppImageStreamInfo->pAppImage_Depth != nullptr) {
        out.needDepthStreamMarkError = bNeedDepthStreamMarkError;
        MY_LOGD("Depth stream mark error (%d)", out.needDepthStreamMarkError);
    }

    //
    if (mPipelineStaticInfo->is4CellSensor
     && out.needReconfiguration
     && out.reconfigCategory != ReCfgCtg::STREAMING)
    {
        // for 4cell reconfig mode for capture, force disable streaming node node
        MY_LOGD("Force disable streaming node for 4cell reconfig");
        p2DecisionOut.needP2StreamNode = false;
        for (int i = 0; i < p2DecisionOut.vImageStreamId_from_StreamNode.size(); i++)
        {
            if (p2DecisionOut.vImageStreamId_from_StreamNode[i] < eSTREAMID_BEGIN_OF_INTERNAL)
            {
                p2DecisionOut.vImageStreamId_from_CaptureNode.push_back(p2DecisionOut.vImageStreamId_from_StreamNode[i]);
            }
        }
        if( p2DecisionOut.vImageStreamId_from_CaptureNode.size()>0 )
        {
            p2DecisionOut.needP2CaptureNode =true;
        }
        p2DecisionOut.vImageStreamId_from_StreamNode.clear();
    }
    for ( auto&& streamId : featureOut.skipP2StreamList )
    {
        auto iter_prv = std::find(p2DecisionOut.vImageStreamId_from_StreamNode.begin(),
                                  p2DecisionOut.vImageStreamId_from_StreamNode.end(),
                                  streamId);
        if (iter_prv != p2DecisionOut.vImageStreamId_from_StreamNode.end())
        {
            out.markErrorStreamList.push_back(*iter_prv);
            MY_LOGD("remove stream buffer(%#" PRIx64 ") from p2stream",
                *iter_prv);
            p2DecisionOut.vImageStreamId_from_StreamNode.erase(iter_prv);
            if (p2DecisionOut.vImageStreamId_from_StreamNode.size() == 0)
            {
                p2DecisionOut.needP2StreamNode = false;
            }
        }
    }
    //
    out.isP1OnlyOutImgo =
        mPipelineStaticInfo->is4CellSensor && out.needReconfiguration &&
        out.reconfigCategory == ReCfgCtg::HIGH_RES_REMOSAIC && [main1SensorId]() {
            NSCamHW::HwInfoHelper helper(main1SensorId);
            helper.updateInfos();
            MSize captureSize, customModeSize;
            auto ret = helper.getSensorSize(
                SENSOR_SCENARIO_ID_NORMAL_CAPTURE, captureSize);
            auto ret2 = helper.getSensorSize(
                SENSOR_SCENARIO_ID_CUSTOM3, customModeSize);
            return helper.isBypassModeSupported(
                (customModeSize.w > captureSize.w && customModeSize.h > captureSize.h)
                    ? customModeSize : captureSize);
        }();
    MY_LOGI_IF(out.isP1OnlyOutImgo, "isP1OnlyOutImgo");

    // (3) build every frames out param
    #define FRAME_MAIN       (0)
    #define FRAME_PRE_SUB    (1)
    #define FRAME_SUB        (2)
    #define FRAME_PRE_DUMMY  (3)
    #define FRAME_POST_DUMMY (4)
    size_t batchIndex = 0;
    auto buildOutParam = [&] (std::shared_ptr<featuresetting::RequestResultParams> const setting, int frameGroupType) -> int
    {
        MY_LOGD_IF(mLogLevel, "build out frame param +");
        std::shared_ptr<RequestResultParams> result = std::make_shared<RequestResultParams>();
        bool isMain = (frameGroupType == FRAME_MAIN);
        bool isDummyFrame = (frameGroupType == FRAME_PRE_DUMMY || frameGroupType == FRAME_POST_DUMMY);

        MINT32 targetFdSensorId = main1SensorId;
        int32_t dualFeatureMode = mPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo->mDualFeatureMode;
        // In multizoom case, decide whether master do FD or slave do FD
        if (isMain && setting && dualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
        {
            // if multicam, query FD sensor idx from MTK_DUALZOOM_FD_TARGET_MASTER
            if(mPolicyTable->pRequestSensorControlPolicy != nullptr &&
               setting->additionalHal.count(main1SensorId))
            {
                IMetadata::getEntry<MINT32>(setting->additionalHal.at(main1SensorId).get(),
                                                MTK_DUALZOOM_FD_TARGET_MASTER, targetFdSensorId);
                auto targetFdIdx = mPipelineStaticInfo->getIndexFromSensorId(targetFdSensorId);

                if(targetFdIdx == -1)
                {
                    MY_LOGE("invalid targetFdSensorId (%d)", targetFdSensorId);
                    return -EINVAL;
                }
            }
            // check P1 Dma need
            if (!setting->needP1Dma.count(targetFdSensorId) ||
	        setting->needP1Dma.at(targetFdSensorId) == 0)
            {
                for (const auto& [_sensorId, _p1Dma] : setting->needP1Dma) {
                    if (_p1Dma != 0) {
                        targetFdSensorId = _sensorId;
                        break;
                    }
                }
            }
        }

        // build topology
        topology::RequestOutputParams topologyOut;
        topology::RequestInputParams topologyIn;
        if (CC_LIKELY(mPolicyTable->fTopology != nullptr))
        {
            //For 2 Physical Raw
            SensorMap<uint32_t>* dmalist = nullptr;
            SensorMap<uint32_t>  dmaTemp;
            if(setting == nullptr)
            {
                if (in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.size() == 0 &&
                    in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.size() == 0)
                {
                    dmalist = nullptr;
                }
                else
                {
                    uint32_t dma = P1_IMGO;
                    dma |= P1_RRZO;
                    for(const auto& raw16 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical)
                    {
                        int const index = mPipelineStaticInfo->getIndexFromSensorId(raw16.first);
                        if (index >= 0)
                            dmaTemp.emplace(raw16.first, dma);
                    }
                    for(const auto& raw10 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical)
                    {
                        int const index = mPipelineStaticInfo->getIndexFromSensorId(raw10.first);
                        if (index >= 0)
                            dmaTemp.emplace(raw10.first, dma);
                    }
                    if(out.multicamReqOutputParams.prvStreamingSensorList.size() > 0)
                    {
                        if(in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0)
                        {
                            int const index = mPipelineStaticInfo->getIndexFromSensorId(
                                                        out.multicamReqOutputParams.prvStreamingSensorList[0]);
                            if (index >= 0)
                                dmaTemp.emplace(out.multicamReqOutputParams.prvStreamingSensorList[0], dma);
                        }
                        for(const auto& raw10 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10)
                        {
                            int const index = mPipelineStaticInfo->getIndexFromSensorId(
                                                        out.multicamReqOutputParams.prvStreamingSensorList[0]);
                            if (index >= 0)
                                dmaTemp.emplace(out.multicamReqOutputParams.prvStreamingSensorList[0], dma);
                        }
                    }
                    dmalist = &(dmaTemp);
                }
            }
            else
            {
                dmalist = &(setting->needP1Dma);
            }
            topologyIn.isDummyFrame = isDummyFrame;
            topologyIn.isMainFrame = isMain;
            topologyIn.isFdEnabled  = (isMain) ? fdOut.isFdEnabled : false;
            topologyIn.useP1FDYUV   = mPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV;
            topologyIn.needP2CaptureNode = p2DecisionOut.needP2CaptureNode;
            topologyIn.needP2StreamNode  = (isMain) ? p2DecisionOut.needP2StreamNode : false; // p2 stream node don't need process sub frame
            topologyIn.pConfiguration_PipelineNodesNeed = in.pConfiguration_PipelineNodesNeed; // topology no need to ref?
            topologyIn.pConfiguration_StreamInfo_NonP1 = in.pConfiguration_StreamInfo_NonP1;
            topologyIn.pPipelineStaticInfo = mPipelineStaticInfo.get();
            topologyIn.pRequest_AppImageStreamInfo = (isMain) ? in.pRequest_AppImageStreamInfo : nullptr;
            topologyIn.pRequest_ParsedAppMetaControl = (isMain) ? in.pRequest_ParsedAppMetaControl : nullptr;
            topologyIn.pvImageStreamId_from_CaptureNode = &(p2DecisionOut.vImageStreamId_from_CaptureNode);
            topologyIn.pvImageStreamId_from_StreamNode = &(p2DecisionOut.vImageStreamId_from_StreamNode);
            topologyIn.pvMetaStreamId_from_CaptureNode = &(p2DecisionOut.vMetaStreamId_from_CaptureNode);
            topologyIn.pvMetaStreamId_from_StreamNode = &(p2DecisionOut.vMetaStreamId_from_StreamNode);
            topologyIn.pvNeedP1Dma = dmalist;
            topologyIn.isAppRawOutFromP2C= isAppRawOutFromP2C;
            topologyIn.pMultiCamReqOutputParams = &out.multicamReqOutputParams;
            topologyIn.targetFd = targetFdSensorId;
            // prepare output buffer
            topologyOut.pNodesNeed     = &(result->nodesNeed);
            topologyOut.pNodeSet       = &(result->nodeSet);
            topologyOut.pRootNodes     = &(result->roots);
            topologyOut.pEdges         = &(result->edges);
            mPolicyTable->fTopology(topologyOut, topologyIn);
        }

        // build P2 IO map
        iomap::RequestOutputParams iomapOut;
        iomap::RequestInputParams iomapIn;
        if ( CC_LIKELY(mPolicyTable->fIOMap_P2Node != nullptr)
          && CC_LIKELY(mPolicyTable->fIOMap_NonP2Node != nullptr) )
        {
            SensorMap<uint32_t> needP1Dma;
            iomapIn.pConfiguration_StreamInfo_NonP1 = in.pConfiguration_StreamInfo_NonP1;
            iomapIn.pConfiguration_StreamInfo_P1    = in.pConfiguration_StreamInfo_P1;
            iomapIn.pRequest_HalImage_Thumbnail_YUV = pThumbnail_YUV.get();
            iomapIn.pRequest_AppImageStreamInfo     = in.pRequest_AppImageStreamInfo;
            iomapIn.pRequest_ParsedAppMetaControl   = in.pRequest_ParsedAppMetaControl;
            iomapIn.pParsedMultiCamInfo             = mPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo.get();
            if (setting != nullptr)
            {
                result->needKeepP1BuffersForAppReprocess = setting->needKeepP1BuffersForAppReprocess;
                if ( setting->needP1Dma.count(main1SensorId) &&
                     in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv.get() )
                {
                    setting->needP1Dma.at(main1SensorId) |= P1_IMGO;
                }
                if (in.pConfiguration_StreamInfo_P1->count(main1SensorId)
                    && in.pConfiguration_StreamInfo_P1->at(main1SensorId).pHalImage_P1_FDYuv
                    && isMain
                    && fdOut.isFdEnabled)
                {
                    for (auto&& [ _sensorId, _p1Dma ] : setting->needP1Dma)
                    {
                        _p1Dma |= P1_FDYUV;
                    }
                }
                auto numsP1YUV = mPipelineStaticInfo->numsP1YUV;
                bool useRSSOR2 = (numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_RSSO_R2 || numsP1YUV == NSCam::v1::Stereo::HAS_P1_YUV_YUVO_RSSO_R2);
                if (setting->needP1Dma.count(main1SensorId) && numsP1YUV > NSCam::v1::Stereo::NO_P1_YUV && isMain)
                {
                    setting->needP1Dma.at(main1SensorId) |= P1_SCALEDYUV;
                }
                if (setting->needP1Dma.size() > 0 && useRSSOR2 && isMain)
                {
                    for (auto&& [_sensorId, _p1Dma] : setting->needP1Dma)
                    {
                        _p1Dma |= P1_SCALEDYUV;
                    }
                }

                if( in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0 ||
                    in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() > 0)
                {
                    if(out.multicamReqOutputParams.prvStreamingSensorList.size() > 0)
                    {
                        int const masterSensorId = out.multicamReqOutputParams.prvStreamingSensorList[0];
                        MY_LOGD("Raw sesnroId : %d", masterSensorId);
                        if ( isMain && setting->needP1Dma.count(masterSensorId) )
                        {
                            setting->needP1Dma.at(masterSensorId) |= P1_IMGO;
                        }
                    }
                    else
                    {
                        MY_LOGE("capture raw fail");
                        return -EINVAL;
                    }
                }

                for( const auto& raw16 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical )
                {
                    auto sensorId = raw16.first;
                    MY_LOGD("Raw16 id(physical) : %d", sensorId);
                    if ( isMain && setting->needP1Dma.count(sensorId) )
                    {
                        setting->needP1Dma.at(sensorId) |= P1_IMGO;
                    }
                }

                for( const auto& raw16 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical )
                {
                    auto sensorId = raw16.first;
                    MY_LOGD("Raw10 id(physical) : %d", sensorId);
                    if ( isMain && setting->needP1Dma.count(sensorId) )
                    {
                        setting->needP1Dma.at(sensorId) |= P1_IMGO;
                    }
                }

                iomapIn.pRequest_NeedP1Dma              = &(setting->needP1Dma);
            }
            else
            {
                for (const auto& [_sensorId, _needP1] : topologyOut.pNodesNeed->needP1Node)
                {
                    if (_needP1 == true)
                        needP1Dma.emplace(_sensorId, P1_IMGO);
                }
                iomapIn.pRequest_NeedP1Dma              = &(needP1Dma);
            }
            iomapIn.pRequest_PipelineNodesNeed      = topologyOut.pNodesNeed;
            iomapIn.pRequest_NodeEdges              = topologyOut.pEdges;
            iomapIn.pvImageStreamId_from_CaptureNode = &(p2DecisionOut.vImageStreamId_from_CaptureNode);
            iomapIn.pvImageStreamId_from_StreamNode = &(p2DecisionOut.vImageStreamId_from_StreamNode);
            iomapIn.pvImageStreamId_from_CaptureNode_Physical = &(p2DecisionOut.vImageStreamId_from_CaptureNode_Physical);
            iomapIn.pvImageStreamId_from_StreamNode_Physical = &(p2DecisionOut.vImageStreamId_from_StreamNode_Physical);
            iomapIn.pvMetaStreamId_from_CaptureNode = &(p2DecisionOut.vMetaStreamId_from_CaptureNode);
            iomapIn.pvMetaStreamId_from_StreamNode = &(p2DecisionOut.vMetaStreamId_from_StreamNode);
            iomapIn.pvMetaStreamId_from_CaptureNode_Physical = &(p2DecisionOut.vMetaStreamId_from_CaptureNode_Physical);
            iomapIn.pvMetaStreamId_from_StreamNode_Physical = &(p2DecisionOut.vMetaStreamId_from_StreamNode_Physical);
            iomapIn.Configuration_HasRecording      = mPipelineUserConfiguration->pParsedAppImageStreamInfo->hasVideoConsumer;
            iomapIn.isMainFrame                     = isMain;
            iomapIn.pPipelineStaticInfo              = mPipelineStaticInfo.get();
            iomapIn.isDummyFrame                    = isDummyFrame;
            iomapIn.useP1FDYUV                      = mPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV;
            iomapIn.isAppRawOutFromP2C                       = isAppRawOutFromP2C;
            iomapIn.isP1OnlyOutImgo                 = out.isP1OnlyOutImgo;
            iomapIn.pMultiCamReqOutputParams = &out.multicamReqOutputParams;
            iomapIn.masterId = (mPolicyTable->pRequestSensorControlPolicy != nullptr) ?
                                    out.multicamReqOutputParams.prvMasterId : main1SensorId;
            iomapIn.isZSLMode                       = in.isZSLMode;
            iomapIn.targetFd = targetFdSensorId;
            iomapIn.isSeamlessRemosaicCapture = setting->needSeamlessRemosaicCapture;

            // prepare output buffer
            iomapOut.pNodeIOMapImage          = &(result->nodeIOMapImage);
            iomapOut.pNodeIOMapMeta           = &(result->nodeIOMapMeta);
            iomapOut.pvMetaStreamId_All_Physical = &(result->physicalMetaStreamIds);
            iomapOut.pTrackFrameResultParams  = &(result->trackFrameResultParams);
            if ( !isDummyFrame )
            {
                mPolicyTable->fIOMap_P2Node(iomapOut, iomapIn);
            }
            mPolicyTable->fIOMap_NonP2Node(iomapOut, iomapIn);
        }

        // Check need to modify P1 IMGO format or not
        {
            bool needToUpdateHalP1StreamInfo =
                        ( featureOut.needUnpackRaw )
                    &&  ( !isDummyFrame )
                    &&  ( in.pRequest_AppImageStreamInfo->pAppImage_Input_RAW16 == nullptr )
                    &&  ( in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.empty() )
                    &&  ( in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.empty() )
                    &&  ( in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.empty() )
                    &&  ( in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.empty() )
                    &&  ( in.pConfiguration_StreamInfo_P1 != nullptr )
                    &&  ( in.pConfiguration_StreamInfo_P1->size() );
            if (needToUpdateHalP1StreamInfo)
            {
                MINT32 fmt = eImgFmt_BAYER10_UNPAK;
                sp<IImageStreamInfo> imgoStreamInfo = (*(in.pConfiguration_StreamInfo_P1)).at(main1SensorId).pHalImage_P1_Imgo;
                if ( imgoStreamInfo != nullptr ) {
                    IImageStreamInfo::BufPlanes_t bufPlanes;
                    auto infohelper = IHwInfoHelperManager::get()->getHwInfoHelper(main1SensorId);
                    MY_LOGF_IF(infohelper == nullptr, "getHwInfoHelper");
                    // Get buffer plane by hwHepler
                    bool ret = infohelper->getDefaultBufPlanes_Imgo(bufPlanes, fmt, imgoStreamInfo->getImgSize());
                    MY_LOGF_IF(!ret, "IHwInfoHelper::getDefaultBufPlanes_Imgo");

                    IImageStreamInfo::BufPlanes_t allocBufPlanes;
                    allocBufPlanes.count = 1;
                    allocBufPlanes.planes[0].sizeInBytes = bufPlanes.planes[0].sizeInBytes;
                    allocBufPlanes.planes[0].rowStrideInBytes = bufPlanes.planes[0].sizeInBytes;
                    auto pStreamInfo =
                        ImageStreamInfoBuilder(imgoStreamInfo.get())
                        .setBufPlanes(bufPlanes)
                        .setImgFormat(fmt)
                        .setAllocBufPlanes(allocBufPlanes)
                        .setAllocImgFormat(eImgFmt_BLOB)
                        .build();
                    MY_LOGI("Format change %s", pStreamInfo->toString().c_str());
                    // Update stream information for per-frame control
                    result->vUpdatedImageStreamInfo[imgoStreamInfo->getStreamId()] = pStreamInfo;
                }
            }
        }

        if (isMain)
        {
            if ( pJpeg_YUV != nullptr)
            {
                result->vUpdatedImageStreamInfo[pJpeg_YUV->getStreamId()] = pJpeg_YUV;
            }
            if ( pThumbnail_YUV != nullptr)
            {
                result->vUpdatedImageStreamInfo[pThumbnail_YUV->getStreamId()] = pThumbnail_YUV;
            }
            if( pJpeg_Sub_YUV != nullptr)
            {
                result->vUpdatedImageStreamInfo[pJpeg_Sub_YUV->getStreamId()] = pJpeg_Sub_YUV;
            }
            if( pDepth_YUV != nullptr)
            {
                result->vUpdatedImageStreamInfo[pDepth_YUV->getStreamId()] = pDepth_YUV;
            }
        }
        if (setting != nullptr)
        {
            result->additionalApp = setting->additionalApp;
            result->additionalHal = setting->additionalHal;
        }
        else
        {
            result->additionalApp = std::make_shared<IMetadata>();

            if(topologyOut.pNodesNeed != nullptr)
            {
                for (const auto& [_sensorId, _needP1] : topologyOut.pNodesNeed->needP1Node)
                {
                    result->additionalHal.emplace(_sensorId, std::make_shared<IMetadata>());
                }
            }
        }

        // update Metdata
        if (CC_LIKELY(mPolicyTable->pRequestMetadataPolicy != nullptr))
        {
            requestmetadata::EvaluateRequestParams metdataParams;
            metdataParams.requestNo                     = in.requestNo;
            metdataParams.isZSLMode                     = in.isZSLMode;
            metdataParams.pRequest_AppImageStreamInfo   = in.pRequest_AppImageStreamInfo;
            metdataParams.pRequest_ParsedAppMetaControl = in.pRequest_ParsedAppMetaControl;
            metdataParams.pSensorSize                   = in.pSensorSize;
            metdataParams.pAdditionalApp                = result->additionalApp;
            metdataParams.pAdditionalHal                = &result->additionalHal;
            metdataParams.pRequest_AppControl           = in.pRequest_AppControl;
            metdataParams.needReconfigure               = featureOut.needReconfiguration;
            metdataParams.pSensorId                     = &mPipelineStaticInfo->sensorId;
            metdataParams.needExif                      = (p2DecisionOut.needP2CaptureNode || in.pRequest_AppImageStreamInfo->hasRawOut);
            metdataParams.targetFd                      = targetFdSensorId;
            metdataParams.isFdEnabled                   = (isMain) ? fdOut.isFdEnabled : false;
            metdataParams.needZslFlow                   = featureOut.needZslFlow;
            metdataParams.pNDDMetadata                  = nddOut.pNDDHalMeta.get();

            if (mPolicyTable->pRequestSensorControlPolicy != nullptr)
            {
                metdataParams.pvNeedP1Node                  = &topologyOut.pNodesNeed->needP1Node;
                metdataParams.pMultiCamReqOutputParams      = &out.multicamReqOutputParams;
            }
            for (const auto& [_sensorId, _streamInfo_P1] : *(in.pConfiguration_StreamInfo_P1))
            {
                if (_streamInfo_P1.pHalImage_P1_Rrzo != nullptr)
                {
                    metdataParams.RrzoSize.emplace(_sensorId, _streamInfo_P1.pHalImage_P1_Rrzo->getImgSize());
                }
                if (_sensorId == targetFdSensorId && _streamInfo_P1.pHalImage_P1_FDYuv != nullptr)
                {
                    metdataParams.FDYuvSize = _streamInfo_P1.pHalImage_P1_FDYuv->getImgSize();
                }
            }
            for (const auto& [_sensorId, _fixedRRZOSize] : featureOut.fixedRRZOSize)
            {
                metdataParams.fixedRRZOSize.emplace(_sensorId, _fixedRRZOSize);
            }

            SensorMap<AdditionalRawInfo> additionalRawInfoMap;
            if (CC_LIKELY(mPolicyTable->pRequestSensorControlPolicy != nullptr)) /* multicam case */
            {
                auto getConfigImgoSize = [&in](MINT32 sensorId)
                {
                    return in.pConfiguration_StreamInfo_P1->at(sensorId).pHalImage_P1_Imgo->getImgSize();
                };
                /*auto getConfigImgoStride = [&in](MINT32 sensorId)
                {
                    return in.pConfiguration_StreamInfo_P1->at(sensorId).pHalImage_P1_Imgo->getBufPlanes().planes[0].rowStrideInBytes;
                };*/

                // logical
                for( const auto& raw16 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16 )
                {
                    auto sensorListSize = mPipelineStaticInfo->sensorId.size();
                    if(sensorListSize > 1 &&
                       out.multicamReqOutputParams.prvStreamingSensorList.size()>0)
                    {
                        // multicam device: check prvStreamingSensorList data
                        int const sensorId = out.multicamReqOutputParams.prvStreamingSensorList[0];
                        AdditionalRawInfo additionalRawInfo;
                        // set capacity
                        if( !in.pConfiguration_StreamInfo_P1->count(sensorId) )
                        {
                            MY_LOGA("not exist in StreamInfo_P1 : id(%" PRId32 ")", sensorId);
                            continue;
                        }
                        additionalRawInfo.imgSize = getConfigImgoSize(sensorId);
                        additionalRawInfo.format = raw16.second->getImgFormat();
                        additionalRawInfo.stride = raw16.second->getBufPlanes().planes[0].rowStrideInBytes;
                        // for logical output raw, it has to update replace image stream flow.
                        if(mAppStreamInfoBuilderFactory != nullptr)
                        {
                            // get clone data
                            auto pImageStreamInfo = mAppStreamInfoBuilderFactory->makeImageStreamInfoBuilder(raw16.second.get());
                            if(pImageStreamInfo == nullptr)
                            {
                                MY_LOGE("make image stream info builder fail (%#" PRIx64 ")", raw16.second->getStreamId());
                                return UNKNOWN_ERROR;
                            }
                            // set actual raw image size.
                            pImageStreamInfo->setImgSize(additionalRawInfo.imgSize);
                            if(isMain) {
                                // only update replace raw image stream info in main frame.
                                result->vUpdatedImageStreamInfo[raw16.second->getStreamId()] = pImageStreamInfo->build();
                            }
                        }

                        additionalRawInfoMap.emplace(sensorId, additionalRawInfo);
                    }
                }

                // physical
                for( const auto& raw16 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical )
                {
                    // multicam device: check prvStreamingSensorList data
                    AdditionalRawInfo additionalRawInfo;
                    int const raw16SensorId = raw16.first;
                    if(! in.pConfiguration_StreamInfo_P1->count(raw16SensorId))
                    {
                        MY_LOGA("not exist in StreamInfo_P1 : id(%" PRId32 ")", raw16SensorId);
                        continue;
                    }
                    // set capacity
                    additionalRawInfo.imgSize = getConfigImgoSize(raw16SensorId);
                    additionalRawInfo.format = raw16.second[0]->getImgFormat();
                    additionalRawInfo.stride = raw16.second[0]->getBufPlanes().planes[0].rowStrideInBytes;

                    if(additionalRawInfoMap.count(raw16SensorId))
                        additionalRawInfoMap.at(raw16SensorId) = additionalRawInfo;
                    else
                        additionalRawInfoMap.emplace(raw16SensorId, additionalRawInfo);
                }

                // logical
                for( const auto& raw10 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10 )
                {
                    auto sensorListSize = mPipelineStaticInfo->sensorId.size();
                    if(sensorListSize > 1 &&
                       out.multicamReqOutputParams.prvStreamingSensorList.size()>0)
                    {
                        // multicam device: check prvStreamingSensorList data
                        int const sensorId = out.multicamReqOutputParams.prvStreamingSensorList[0];
                        AdditionalRawInfo additionalRawInfo;
                        // set capacity
                        if( !in.pConfiguration_StreamInfo_P1->count(sensorId) )
                        {
                            MY_LOGA("not exist in StreamInfo_P1 : id(%" PRId32 ")", sensorId);
                            continue;
                        }
                        additionalRawInfo.imgSize = getConfigImgoSize(sensorId);
                        additionalRawInfo.format = raw10.second->getImgFormat();
                        additionalRawInfo.stride = raw10.second->getBufPlanes().planes[0].rowStrideInBytes;
                        // for logical output raw, it has to update replace image stream flow.
                        if(mAppStreamInfoBuilderFactory != nullptr)
                        {
                            // get clone data
                            auto pImageStreamInfo = mAppStreamInfoBuilderFactory->makeImageStreamInfoBuilder(raw10.second.get());
                            if(pImageStreamInfo == nullptr)
                            {
                                MY_LOGE("make image stream info builder fail (%#" PRIx64 ")", raw10.second->getStreamId());
                                return UNKNOWN_ERROR;
                            }
                            // set actual raw image size.
                            pImageStreamInfo->setImgSize(additionalRawInfo.imgSize);
                            if(isMain) {
                                // only update replace raw image stream info in main frame.
                                result->vUpdatedImageStreamInfo[raw10.second->getStreamId()] = pImageStreamInfo->build();
                            }
                        }

                        additionalRawInfoMap.emplace(sensorId, additionalRawInfo);
                    }
                }
                // physical
                for( const auto& raw10 : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical )
                {
                    // multicam device: check prvStreamingSensorList data
                    AdditionalRawInfo additionalRawInfo;
                    int const raw10SensorId = raw10.first;
                    if(! in.pConfiguration_StreamInfo_P1->count(raw10SensorId))
                    {
                        MY_LOGA("not exist in StreamInfo_P1 : id(%" PRId32 ")", raw10SensorId);
                        continue;
                    }
                    // set capacity
                    additionalRawInfo.imgSize = getConfigImgoSize(raw10SensorId);
                    additionalRawInfo.format = raw10.second[0]->getImgFormat();
                    additionalRawInfo.stride = raw10.second[0]->getBufPlanes().planes[0].rowStrideInBytes;

                    if(additionalRawInfoMap.count(raw10SensorId))
                        additionalRawInfoMap.at(raw10SensorId) = additionalRawInfo;
                    else
                        additionalRawInfoMap.emplace(raw10SensorId, additionalRawInfo);
                }

                metdataParams.additionalRawInfo = &additionalRawInfoMap;
            }
            mPolicyTable->pRequestMetadataPolicy->evaluateRequest(metdataParams);
        }

        auto evaulateBatchInfo = [&] (uint32_t frameType) -> void
        {
            if (featureOut.batchPolicy.size != 0 && (featureOut.batchPolicy.frameType & frameType))
            {
                result->batchSize = featureOut.batchPolicy.size;
                result->batchIndex = batchIndex;
                batchIndex++;
                if (batchIndex >= featureOut.batchPolicy.size) {
                    batchIndex = 0;
                }
            }
        };

        switch (frameGroupType)
        {
        case FRAME_MAIN:
            evaulateBatchInfo(BatchPolicy::MAIN);
            out.mainFrame = result;
            break;
        case FRAME_PRE_SUB:
            evaulateBatchInfo(BatchPolicy::PRE_SUB);
            out.preSubFrames.push_back(result);
            break;
        case FRAME_SUB:
            evaulateBatchInfo(BatchPolicy::SUB);
            out.subFrames.push_back(result);
            break;
        case FRAME_PRE_DUMMY:
            evaulateBatchInfo(BatchPolicy::PRE_DUMMY);
            out.preDummyFrames.push_back(result);
            break;
        case FRAME_POST_DUMMY:
            evaulateBatchInfo(BatchPolicy::POST_DUMMY);
            out.postDummyFrames.push_back(result);
            break;
        default:
            MY_LOGW("Cannot be here");
            break;
        }

        MY_LOGD_IF(mLogLevel, "build out frame param -");
        return OK;
    };

    if (noP2Node && !in.pRequest_AppImageStreamInfo->hasRawOut)
    {
        buildOutParam(nullptr, FRAME_MAIN);
        MY_LOGD("no p2 node process done");
        return OK;
    }

    // pre-dummy -> pre-sub -> main -> sub -> post-dummy
    for (auto const& it : featureOut.preDummyFrames)
    {
        buildOutParam(it, FRAME_PRE_DUMMY);
    }
    for (auto const& it : featureOut.preSubFrames)
    {
        buildOutParam(it, FRAME_PRE_SUB);
    }
    buildOutParam(featureOut.mainFrame, FRAME_MAIN);
    for (auto const& it : featureOut.subFrames)
    {
        buildOutParam(it, FRAME_SUB);
    }
    for (auto const& it : featureOut.postDummyFrames)
    {
        buildOutParam(it, FRAME_POST_DUMMY);
    }
    // for 60 fps +
    if (mwaitToResetCnt > 0)
    {
        mwaitToResetCnt--;
        if (mwaitToResetCnt == 0)
        {
            mp3AHal = MAKE_Hal3A(mPipelineStaticInfo->sensorId[0], LOG_TAG);
            // reset fps control
            if (mp3AHal) {
                MY_LOGI_IF(mLogLevel, "reset fps");
                mp3AHal->send3ACtrl(NS3Av3::E3ACtrl_SetCaptureMaxFPS, (MINTPTR)0, (MINTPTR)0);
                mp3AHal->destroyInstance(LOG_TAG);
            }
        }
    }
    auto needHighSpeedCap = [&] () -> bool
    {
        MY_LOGI_IF(mLogLevel, "needcap(%d), needZsl(%d), subFrame(%zu), postFrame(%zu), preFrame(%zu)"
                              ", FrameCount(%d), FrameIndex(%d)"
                            , p2DecisionOut.needP2CaptureNode
                            , featureOut.needZslFlow
                            , featureOut.subFrames.size()
                            , featureOut.postDummyFrames.size()
                            , featureOut.preDummyFrames.size()
                            , in.pRequest_ParsedAppMetaControl->control_isp_frm_count
                            , in.pRequest_ParsedAppMetaControl->control_isp_frm_index);
        if (mDebugMode == -1)
        {
            const uint32_t sensorId = mPipelineStaticInfo->sensorId[0];
            bool ret = p2DecisionOut.needP2CaptureNode &&                               // need p2 capture node
               mPipelineStaticInfo->sensorId.size() == 1 &&                    // sensor count == 1
               (in.pSensorFps != nullptr && (*(in.pSensorFps)).at(sensorId) > 30) &&     // sensor fps > 30
               !mPipelineStaticInfo->isDualDevice &&                            // not dual camera
               !mPipelineUserConfiguration->pParsedAppImageStreamInfo->hasVideoConsumer && // not video mode
               ((featureOut.subFrames.size() > 0 ||
                 featureOut.postDummyFrames.size() > 0 ||
                 featureOut.preDummyFrames.size() > 0) ||                          // multiframe capture
                 (in.pRequest_ParsedAppMetaControl->control_isp_frm_count > 1 &&
                  in.pRequest_ParsedAppMetaControl->control_isp_frm_index == 0)
               );
               //!featureOut.needZslFlow;                                         // no need zsl flow
            if (p2DecisionOut.needP2CaptureNode)
            {
                if (in.pRequest_ParsedAppMetaControl->control_isp_frm_count > 1) {
                    // reset mwaitToResetCnt = 2 during app multiframe capture
                    mwaitToResetCnt = 2;
                }
            }
            return ret;
        }
        else
        {
            return p2DecisionOut.needP2CaptureNode && mDebugMode;
        }
    };
    if (needHighSpeedCap())
    {
        if (in.pSensorFps != nullptr)
        {
            const uint32_t sensorId = mPipelineStaticInfo->sensorId[0];
            MY_LOGI_IF(mLogLevel, "force maxfps to (%d)", (*(in.pSensorFps)).at(sensorId));

            mp3AHal = MAKE_Hal3A(mPipelineStaticInfo->sensorId[0], LOG_TAG);
            if (mp3AHal) {
                // boost fps control
                int32_t isAfStable = 0;
                uint32_t timeoutCnt = 0;
                #define WAIT_AF_TIMEOUT (60)
                while (mPipelineStaticInfo->isAFSupported)
                {
                    mp3AHal->send3ACtrl(NS3Av3::E3ACtrl_GetAFStable, (MINTPTR)&isAfStable, (MINTPTR)0);
                    timeoutCnt++;
                    if (isAfStable || (timeoutCnt > WAIT_AF_TIMEOUT)) {
                        break;
                    }
                    usleep(16000); // 16ms
                }
                mp3AHal->send3ACtrl(NS3Av3::E3ACtrl_SetCaptureMaxFPS, (MINTPTR)1, (MINTPTR)((*(in.pSensorFps)).at(sensorId) * 1000));
                mp3AHal->destroyInstance(LOG_TAG);
                mwaitToResetCnt = 2;
            }
        }
    }
    // for 60 fps -
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getSensorIndexBySensorId(
    uint32_t sensorId,
    uint32_t &index
) -> bool
{
    bool ret = false;
    for(size_t i=0;i<mPipelineStaticInfo->sensorId.size();i++)
    {
        if(sensorId == (uint32_t)mPipelineStaticInfo->sensorId[i])
        {
            index = i;
            ret = true;
            break;
        }
    }
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
combineFeatureParam(
    featuresetting::RequestOutputParams &target,
    uint32_t source_id,
    featuresetting::RequestOutputParams &source
) -> bool
{
    if (!source.mainFrame) {
        MY_LOGD("Null mainFrame in source id(%d)", source_id);
        return true;
    }
    // if dma > 1 means needs output p1 data.
    if(source.mainFrame->needP1Dma.size() > 0) {
        if(source_id >= 0) {
            const auto iter = source.sensorMode.find(source_id);
            if (iter != source.sensorMode.end()) {
                updateSensorModeToOutFeatureParam(target, iter->second, source_id);
            }

            for (auto&& item : source.vboostControl) {
                updateBoostControlToOutFeatureParam(
                                        target,
                                        item);
            }
            // main frame append/override
            updateRequestResultParams(target.mainFrame, source.mainFrame, source_id);
            // subFrame
            for(size_t i=0;i<source.subFrames.size();i++)
            {
                auto &elem = source.subFrames[i];
                MY_LOGD("build subFrames id(%d)", source_id);
                if(elem == nullptr){
                    target.subFrames.push_back(nullptr);
                }
                else {
                    if(i >= target.subFrames.size())
                    {
                        // new append
                        auto targetSubFrames = std::make_shared<featuresetting::RequestResultParams>();
                        updateRequestResultParams(targetSubFrames, elem, source_id);
                        target.subFrames.push_back(targetSubFrames);
                    }
                    else
                    {
                        updateRequestResultParams(target.subFrames[i], elem, source_id);
                    }
                }
            }
            // preDummyFrames
            for(size_t i=0;i<source.preDummyFrames.size();i++)
            {
                auto &elem = source.preDummyFrames[i];
                MY_LOGD("build preDummyFrames id(%d)", source_id);
                if(elem == nullptr){
                    target.preDummyFrames.push_back(nullptr);
                }
                else {
                    if(i >= target.preDummyFrames.size())
                    {
                        // new append
                        auto targetpreDummyFrames = std::make_shared<featuresetting::RequestResultParams>();
                        updateRequestResultParams(targetpreDummyFrames, elem, source_id);
                        target.preDummyFrames.push_back(targetpreDummyFrames);
                    }
                    else
                    {
                        updateRequestResultParams(target.preDummyFrames[i], elem, source_id);
                    }
                }
            }
            // postDummyFrames
            for(size_t i=0;i<source.postDummyFrames.size();i++)
            {
                auto &elem = source.postDummyFrames[i];
                MY_LOGD("build postDummyFrames id(%d)", source_id);
                if(elem == nullptr){
                    target.postDummyFrames.push_back(nullptr);
                }
                else {
                    if(i >= target.postDummyFrames.size())
                    {
                        // new append
                        auto targetpostDummyFrames = std::make_shared<featuresetting::RequestResultParams>();
                        updateRequestResultParams(targetpostDummyFrames, elem, source_id);
                        target.postDummyFrames.push_back(targetpostDummyFrames);
                    }
                    else
                    {
                        updateRequestResultParams(target.postDummyFrames[i], elem, source_id);
                    }
                }
            }
            target.needZslFlow = (source.needZslFlow || target.needZslFlow);
            target.needReconfiguration = (source.needReconfiguration || target.needReconfiguration);
            // manually combine ZslPolicyParams
            {
                auto& tar = target.zslPolicyParams;
                auto& src = source.zslPolicyParams;
                if (tar.pZslPluginParams == nullptr)
                    tar.pZslPluginParams = src.pZslPluginParams;
                if (tar.pSelector == nullptr)
                    tar.pSelector = src.pSelector;
                tar.needPDProcessedRaw |= src.needPDProcessedRaw;
                tar.needFrameSync |= src.needFrameSync;
            }
            target.keepZslBuffer &= source.keepZslBuffer;
            target.reconfigCategory = source.reconfigCategory;
        }
    }
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateSensorModeToOutFeatureParam(
    featuresetting::RequestOutputParams &target,
    uint32_t sensorMode,
    uint32_t sensorId
) -> bool
{
    if(!target.sensorMode.count(sensorId))
        target.sensorMode.emplace(sensorId, sensorMode);
    else
        target.sensorMode.at(sensorId) = sensorMode;
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateBoostControlToOutFeatureParam(
    featuresetting::RequestOutputParams &target,
    BoostControl boostControl
) -> bool
{
    target.vboostControl.push_back(boostControl);
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
updateRequestResultParams(
    std::shared_ptr<featuresetting::RequestResultParams> &target,
    std::shared_ptr<featuresetting::RequestResultParams> &physicalOut,
    uint32_t sensorId
) -> bool
{
    if(physicalOut == nullptr) {
        MY_LOGE("target(%p) slave out(%p)", target.get(), physicalOut.get());
        return false;
    }
    if(target == nullptr) {
        target = std::make_shared<featuresetting::RequestResultParams>();
        target->needKeepP1BuffersForAppReprocess =
                physicalOut->needKeepP1BuffersForAppReprocess;
        target->additionalApp = physicalOut->additionalApp;
    }
    // check slave size
    if(physicalOut->needP1Dma.size() > 1 &&
      (physicalOut->needP1Dma.size() != physicalOut->additionalHal.size())) {
        MY_LOGE("dma(%zu) hal size(%zu), need check flow",
                            physicalOut->needP1Dma.size(),
                            physicalOut->additionalHal.size());
        return false;
    }

    // dma
    if(target->needP1Dma.count(sensorId))
        target->needP1Dma.at(sensorId) |= physicalOut->needP1Dma.at(sensorId);
    else
        target->needP1Dma.emplace(sensorId, physicalOut->needP1Dma.at(sensorId));

    // hal
    if(target->additionalHal.count(sensorId))
        *(target->additionalHal.at(sensorId)) += *(physicalOut->additionalHal.at(sensorId));
    else
        target->additionalHal.emplace(sensorId, physicalOut->additionalHal.at(sensorId));

    return true;
}

auto
ThisNamespace::
getNeededSensorListForPhysicalFeatureSetting(
    RequestInputParams const& in,
    p2nodedecision::RequestOutputParams const& p2DecisionOut,
    std::vector<uint32_t> const& vStreamingSensorList,
    std::vector<uint32_t> const& vPrvStreamingSensorList
) -> std::vector<uint32_t>
{
    std::vector<uint32_t> vNeededSensors;

    // check need update id.
    for(auto&& item : p2DecisionOut.vImageStreamId_from_StreamNode_Physical)
    {
        vNeededSensors.push_back(item.first);
    }

    for(auto&& item : p2DecisionOut.vImageStreamId_from_CaptureNode_Physical)
    {
        if(find(vNeededSensors.begin(), vNeededSensors.end(), item.first) == vNeededSensors.end())
        {
            vNeededSensors.push_back(item.first);
        }
    }

    for(auto&& item : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical)
    {
        if(find(vNeededSensors.begin(), vNeededSensors.end(), item.first) == vNeededSensors.end())
        {
            vNeededSensors.push_back(item.first);
        }
    }

    for(auto&& item : in.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical)
    {
        if(find(vNeededSensors.begin(), vNeededSensors.end(), item.first) == vNeededSensors.end())
        {
            vNeededSensors.push_back(item.first);
        }
    }

    if(p2DecisionOut.vImageStreamId_from_StreamNode.size() > 0)
    {
        // update streaming stream as physical control.
        MY_LOGD_IF(mLogLevel > 0, "logical streaming update");
        for(auto&& id : vStreamingSensorList)
        {
            if(find(vNeededSensors.begin(), vNeededSensors.end(), id) == vNeededSensors.end())
            {
                vNeededSensors.push_back(id);
            }
        }
    }

    // for multiple physical streams
    if (in.pRequest_AppImageStreamInfo->outputPhysicalSensors.size() >= 1)
    {
        MY_LOGD_IF(mLogLevel > 0, "for physical streams");
        for(auto&& id : vStreamingSensorList)
        {
            if(find(vNeededSensors.begin(), vNeededSensors.end(), id) == vNeededSensors.end())
            {
                vNeededSensors.push_back(id);
            }
        }

        for(auto&& id : vPrvStreamingSensorList)
        {
            if(find(vNeededSensors.begin(), vNeededSensors.end(), id) == vNeededSensors.end())
            {
                vNeededSensors.push_back(id);
            }
        }
    }
    MY_LOGW_IF(vNeededSensors.size() > 2,"vNeededSensors size is %zu, larger than 2!", vNeededSensors.size());
    return vNeededSensors;
}
