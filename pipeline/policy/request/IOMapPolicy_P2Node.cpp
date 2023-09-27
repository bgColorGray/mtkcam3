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

#define LOG_TAG "mtkcam-P2NodeIOMapPolicy"

#include <mtkcam3/pipeline/policy/IIOMapPolicy.h>
//
#include <mtkcam3/pipeline/hwnode/NodeId.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>

#include <bitset>
#include <map>
#include <unordered_map>
#include <utility>
#include <mtkcam/utils/std/ULog.h>
//
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);

#define CAP_IOMAP_COMBINE false
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam::v3::pipeline;
using namespace NSCam::v3::pipeline::NSPipelineContext;
using namespace NSCam::v3::pipeline::policy::iomap;


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
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {

/******************************************************************************
 *
 ******************************************************************************/
static
bool
getIsNeedImgo(StreamId_T streamid __attribute__((unused)), RequestInputParams const& in, MSize rrzoSize __attribute__((unused)), bool config_isRecord __attribute__((unused)))
{
    // for reprocessing flow, it has to use imgo.
    if ((in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv.get()) ||
        (in.pRequest_AppImageStreamInfo->pAppImage_Input_Priv.get()) ||
        (in.pRequest_AppImageStreamInfo->pAppImage_Input_Yuv.get()))
    {
        return true;
    }

    return false;
}

/******************************************************************************
 *
 ******************************************************************************/
static
bool
dataCheck(
    RequestInputParams const& in
)
{
    bool ret = true;
    if(in.pPipelineStaticInfo == nullptr) {
        MY_LOGE("pPipelineStaticInfo is nullptr");
        ret &= false;
        goto lbExit;
    }
    if(in.pPipelineStaticInfo->sensorId.size() <
       in.pRequest_PipelineNodesNeed->needP1Node.size()) {
        MY_LOGE("mismatch: sensorId(%zu) p1node(%zu)",
                in.pPipelineStaticInfo->sensorId.size(),
                in.pRequest_PipelineNodesNeed->needP1Node.size());
        ret &= false;
    }
lbExit:
    return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
static
bool
addStreamIdToStreamMapBySensorId(
    std::unordered_map<uint32_t, std::vector<StreamId_T>>* const pStreamMap,
    uint32_t const id,
    StreamId_T const streamid
)
{
    if(pStreamMap != nullptr)
    {
        auto it = pStreamMap->find(id);

        if( it != pStreamMap->end() )
        {
            it->second.push_back(streamid);
        }
        else
        {
            std::vector<StreamId_T> temp;
            temp.push_back(streamid);
            pStreamMap->insert({id, std::move(temp)});
        }
    }

    return OK;
};

/******************************************************************************
 *
 ******************************************************************************/
static
MERROR
evaluateRequest_P2StreamNode(
    RequestOutputParams& out,
    RequestInputParams const& in
)
{
    if(!dataCheck(in)) {
        return OK;
    }

    if((in.pvImageStreamId_from_StreamNode == nullptr) ||
       (in.pvMetaStreamId_from_StreamNode == nullptr))
    {
        MY_LOGE("pvImageStreamId_from_StreamNode(%p)"
                "pvMetaStreamId_from_StreamNode(%p)",
                in.pvImageStreamId_from_StreamNode,
                in.pvMetaStreamId_from_StreamNode);
        return OK;
    }

    if ( !in.pRequest_PipelineNodesNeed->needP2StreamNode || !in.isMainFrame ) {
        MY_LOGD("No need P2StreamNode");
        return OK;
    }

    // define local struct.
    struct IOMapGroup {
        IOMap imgoMap;
        IOMap rrzoMap;
        IOMap metaMap;
        IOMap subMetaMap;
        MSize rrzoSize;
    };
    //
    auto isMain1 = [&in](MINT32 sensorId) {
        if( in.pPipelineStaticInfo->getIndexFromSensorId(sensorId) == 0 )
            return true;
        return false;
    };

    IOMapGroup logicalIOMapGroup;
    // <sensor id, IOMapGroup>
    std::unordered_map<uint32_t, IOMapGroup> physicalIOMapGroupMap;

    auto const& needP1Node = in.pRequest_PipelineNodesNeed->needP1Node;
    auto const& needP1Dma = *in.pRequest_NeedP1Dma;
    auto const& pvStreamIdOutFromStreaming_Image_Logical = *in.pvImageStreamId_from_StreamNode;
    auto const& pvStreamIdOutFromStreaming_Meta_Logical = *in.pvMetaStreamId_from_StreamNode;

    // construct physical IOMap groups
    for (const auto& [_sensorId, _needP1] : needP1Node)
    {
        if ( _needP1 )
        {
            IOMapGroup iomap;
            physicalIOMapGroupMap.emplace(_sensorId, iomap);
        }
    }

    bool hasImgo = false;
    bool hasRrzo = false;

    // build physical image-in and meta-in
    for (auto&& [_sensorId, _phygroup] : physicalIOMapGroupMap)
    {
        const auto need_P1Dma = needP1Dma.at(_sensorId);
        auto& p1Info = in.pConfiguration_StreamInfo_P1->at(_sensorId);
        sp<IImageStreamInfo> rrzoInfo = p1Info.pHalImage_P1_Rrzo;
        sp<IImageStreamInfo> imgoInfo =
            ( in.isSeamlessRemosaicCapture && isMain1(_sensorId) )
                ? p1Info.pHalImage_P1_SeamlessImgo
                : p1Info.pHalImage_P1_Imgo;

        if( (need_P1Dma & P1_IMGO) || in.isSeamlessRemosaicCapture) {
            _phygroup.imgoMap.addIn(imgoInfo->getStreamId());
            hasImgo = true;

            if(need_P1Dma & P1_LCSO) {
                _phygroup.imgoMap.addIn(p1Info.pHalImage_P1_Lcso->getStreamId());
            }

            if(need_P1Dma & P1_RSSO) {
                _phygroup.imgoMap.addIn(p1Info.pHalImage_P1_Rsso->getStreamId());
            }
        }

        if(need_P1Dma & P1_RRZO) {
            _phygroup.rrzoMap.addIn(rrzoInfo->getStreamId());
            _phygroup.rrzoSize = rrzoInfo->getImgSize();
            hasRrzo = true;

            if(need_P1Dma & P1_LCSO) {
                _phygroup.rrzoMap.addIn(p1Info.pHalImage_P1_Lcso->getStreamId());
            }

            if(need_P1Dma & P1_RSSO) {
                _phygroup.rrzoMap.addIn(p1Info.pHalImage_P1_Rsso->getStreamId());
            }
        }

        if (need_P1Dma & P1_SCALEDYUV) {
            if( p1Info.pHalImage_P1_ScaledYuv != nullptr )
            {
                if(_phygroup.imgoMap.sizeIn() > 0)
                    _phygroup.imgoMap.addIn(p1Info.pHalImage_P1_ScaledYuv->getStreamId());
                if(_phygroup.rrzoMap.sizeIn() > 0)
                    _phygroup.rrzoMap.addIn(p1Info.pHalImage_P1_ScaledYuv->getStreamId());
            }
            if( p1Info.pHalImage_P1_RssoR2 != nullptr ){
                if(_phygroup.imgoMap.sizeIn() > 0)
                    _phygroup.imgoMap.addIn(p1Info.pHalImage_P1_RssoR2->getStreamId());
                if(_phygroup.rrzoMap.sizeIn() > 0)
                    _phygroup.rrzoMap.addIn(p1Info.pHalImage_P1_RssoR2->getStreamId());
            }
        }

        // TrackingFocus
        if(in.pRequest_AppImageStreamInfo->hasTrackingFocus)
        {
            _phygroup.rrzoMap.addIn(p1Info.pHalImage_P1_FDYuv->getStreamId());
        }

        _phygroup.metaMap.addIn(p1Info.pAppMeta_DynamicP1->getStreamId());
        _phygroup.metaMap.addIn(p1Info.pHalMeta_DynamicP1->getStreamId());
        _phygroup.metaMap.addIn(in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId());
        _phygroup.subMetaMap.addIn(p1Info.pAppMeta_DynamicP1->getStreamId());
        _phygroup.subMetaMap.addIn(p1Info.pHalMeta_DynamicP1->getStreamId());
        _phygroup.subMetaMap.addIn(in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId());
    }

    // check any imgo/rrzo are set or not.
    if (!(hasImgo || hasRrzo)) {
        MY_LOGE("No Imgo or Rrzo");
        return OK;
    }

    // build physical image-out and pvMetaStreamId_All_Physical
    if( (in.pvImageStreamId_from_StreamNode_Physical != nullptr) &&
        (in.pvMetaStreamId_from_StreamNode_Physical != nullptr) )
    {
        auto const& pvStreamIdOutFromStreaming_Image_Physical = *in.pvImageStreamId_from_StreamNode_Physical;
        auto const& pvStreamIdOutFromStreaming_Meta_Physical = *in.pvMetaStreamId_from_StreamNode_Physical;

        for (auto&& [_sensorId, _vStreamIds] : pvStreamIdOutFromStreaming_Image_Physical)
        {
            auto iter = physicalIOMapGroupMap.find(_sensorId);
            if(iter == physicalIOMapGroupMap.end()) {
                MY_LOGE("need output physical stream, but cannot find id(%" PRIu32 ") in physicalIOMapGroupMap",
                            _sensorId);
                return OK;
            }
            IOMapGroup &_phygroup = iter->second;

            for (auto&& _streamId : _vStreamIds)
            {
                size_t imgo_size = _phygroup.imgoMap.sizeIn();
                size_t rrzo_size = _phygroup.rrzoMap.sizeIn();

                if((imgo_size > 0) && (rrzo_size > 0)) {
                    // if target output size > rrzo size, use imgo size.
                    if( getIsNeedImgo(_streamId, in, _phygroup.rrzoSize, in.Configuration_HasRecording) ) {
                        _phygroup.imgoMap.addOut(_streamId);
                    }
                    else {
                        _phygroup.rrzoMap.addOut(_streamId);
                    }
                }
                else if(imgo_size > 0) {
                    _phygroup.imgoMap.addOut(_streamId);
                }
                else {
                    _phygroup.rrzoMap.addOut(_streamId);
                }
            }

            // check needs output physical stream or not.
            // If needs output physical stream, add P1 app metadata to pAppMeta_DynamicP1.
            if(!in.pConfiguration_StreamInfo_P1->count(_sensorId))
            {
                MY_LOGE("cannot find sensor id in pConfiguration_StreamInfo_P1");
                continue;
            }
            auto const& p1Info = in.pConfiguration_StreamInfo_P1->at(_sensorId);
            addStreamIdToStreamMapBySensorId(out.pvMetaStreamId_All_Physical,
                _sensorId, p1Info.pAppMeta_DynamicP1->getStreamId());
        }

        // build physical meta-out
        for( auto&& [_sensorId, _vStreamIds] : pvStreamIdOutFromStreaming_Meta_Physical )
        {
            auto iter = physicalIOMapGroupMap.find(_sensorId);

            if(iter != physicalIOMapGroupMap.end())
            {
                for (auto&& _streamId : _vStreamIds)
                {
                    iter->second.metaMap.addOut(_streamId);
                    addStreamIdToStreamMapBySensorId(out.pvMetaStreamId_All_Physical,
                        _sensorId, _streamId);
                }
            }
        }
    }

    // build logical image-in and image-out
    for(auto&& outputStreamId : pvStreamIdOutFromStreaming_Image_Logical)
    {
        bool bAddToImgo = false;

        for (auto&& [_sensorId, _phygroup] : physicalIOMapGroupMap)
        {
            bool needCheckImgo = false;

            if(isMain1(_sensorId))
            {
                // cam_0 need imgo .
                needCheckImgo = true;
            }

            size_t imgo_size = _phygroup.imgoMap.sizeIn();
            size_t rrzo_size = _phygroup.rrzoMap.sizeIn();

            if((imgo_size > 0) && (rrzo_size > 0) && needCheckImgo) {
                // if target output size > rrzo size, use imgo size.
                if( getIsNeedImgo(outputStreamId, in, _phygroup.rrzoSize, in.Configuration_HasRecording) ) {
                    logicalIOMapGroup.imgoMap.addIn(_phygroup.imgoMap.vIn);
                    bAddToImgo = true;
                }
                else {
                    logicalIOMapGroup.rrzoMap.addIn(_phygroup.rrzoMap.vIn);
                    bAddToImgo = false;
                }
            }
            else if((imgo_size > 0) && needCheckImgo) {
                logicalIOMapGroup.imgoMap.addIn(_phygroup.imgoMap.vIn);
                bAddToImgo = true;
            }
            else {
                logicalIOMapGroup.rrzoMap.addIn(_phygroup.rrzoMap.vIn);
                bAddToImgo = false;
            }

            // set metadata
            logicalIOMapGroup.metaMap.addIn(_phygroup.metaMap.vIn);
            logicalIOMapGroup.subMetaMap.addIn(_phygroup.subMetaMap.vIn);
        }

        // add output stream to logical output iomap
        if(bAddToImgo) {
            logicalIOMapGroup.imgoMap.addOut(outputStreamId);
        }
        else {
            logicalIOMapGroup.rrzoMap.addOut(outputStreamId);
        }
    }

    // build logical meta-in and meta-out
    for(const auto streamId : pvStreamIdOutFromStreaming_Meta_Logical) {
        logicalIOMapGroup.metaMap.addOut(streamId);
    }
    logicalIOMapGroup.metaMap.addIn(in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId());
    logicalIOMapGroup.subMetaMap.addIn(in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId());

    // build IOMapSet result
    IOMapSet P2ImgIO, P2MetaIO;
    auto buildP2IOMapSet = [&P2ImgIO, &P2MetaIO](IOMapGroup &group)
    {
        int needMetaCount = 0;
        if (group.imgoMap.sizeOut() > 0) {
            //dumpIOMap(group.imgoMap);
            P2ImgIO.add(group.imgoMap);
            needMetaCount++;
        }
        if (group.rrzoMap.sizeOut() > 0) {
            //dumpIOMap(group.rrzoMap);
            P2ImgIO.add(group.rrzoMap);
            needMetaCount++;
        }

        if(needMetaCount > 0)
        {
            P2MetaIO.add(group.metaMap);
            if(needMetaCount > 1)
            {
                P2MetaIO.add(group.subMetaMap);
            }
        }
    };

    // logical
    {
        if (logicalIOMapGroup.imgoMap.sizeOut() > 0) {
            out.bP2UseImgo = true;
        }
        buildP2IOMapSet(logicalIOMapGroup);
    }
    // physical
    {
        for (auto&& [_sensorId, _group] : physicalIOMapGroupMap) {
            buildP2IOMapSet(_group);
        }
    }

    (*out.pNodeIOMapImage)[eNODEID_P2StreamNode] = P2ImgIO;
    (*out.pNodeIOMapMeta) [eNODEID_P2StreamNode] = P2MetaIO;

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
static
MERROR
evaluateRequest_P2CaptureNode(
    RequestOutputParams& out,
    RequestInputParams const& in
)
{
    if(!dataCheck(in)) {
        return OK;
    }

    if((in.pvImageStreamId_from_CaptureNode == nullptr) ||
       (in.pvMetaStreamId_from_CaptureNode == nullptr)) {
        MY_LOGE("pvImageStreamId_from_CaptureNode(%p)"
                "pvMetaStreamId_from_CaptureNode(%p)",
                in.pvImageStreamId_from_CaptureNode,
                in.pvMetaStreamId_from_CaptureNode);
        return OK;
    }

    if ( !in.pRequest_PipelineNodesNeed->needP2CaptureNode )
    {
        return OK;
    }

    // define local struct.
    struct IOMapGroup {
        IOMap imageMap;
        IOMap metaMap;
        MSize rrzoSize;
    };
    //
    auto isMain1 = [&in](MINT32 sensorId) {
        if( in.pPipelineStaticInfo->getIndexFromSensorId(sensorId) == 0 )
            return true;
        return false;
    };

    IOMapGroup logicalIOMapGroup;
    // <sensor id, IOMapGroup>
    std::unordered_map<uint32_t, IOMapGroup> physicalIOMapGroupMap;

    auto const& pRequest_PipelineNodesNeed = in.pRequest_PipelineNodesNeed;
    auto const& needP1Node = in.pRequest_PipelineNodesNeed->needP1Node;
    auto const& needP1Dma = *in.pRequest_NeedP1Dma;
    auto const& pRequest_AppImageStreamInfo = in.pRequest_AppImageStreamInfo;
    auto const& pvStreamIdOutFromCapture_Image_Logical  = *in.pvImageStreamId_from_CaptureNode;
    auto const& pvStreamIdOutFromCapture_Meta_Logical   = *in.pvMetaStreamId_from_CaptureNode;

    bool hasImgo = false;

    // construct physical IOMap groups
    for (const auto& [_sensorId, _needP1] : needP1Node)
    {
        if ( _needP1 )
        {
            IOMapGroup iomap;
            physicalIOMapGroupMap.emplace(_sensorId, iomap);
        }
    }

    // build physical image-in and meta-in
    for (auto&& [_sensorId, _phygroup] : physicalIOMapGroupMap)
    {
        const auto need_P1Dma = needP1Dma.at(_sensorId);

        auto& p1Info = in.pConfiguration_StreamInfo_P1->at(_sensorId);
        sp<IImageStreamInfo> rrzoInfo = p1Info.pHalImage_P1_Rrzo;
        sp<IImageStreamInfo> imgoInfo =
            ( in.isSeamlessRemosaicCapture && isMain1(_sensorId) )
                ? p1Info.pHalImage_P1_SeamlessImgo
                : p1Info.pHalImage_P1_Imgo;

        if (isMain1(_sensorId)) // first element is master id
        {
            if ( auto p = in.pRequest_AppImageStreamInfo->pAppImage_Input_Priv.get() )
            {
                imgoInfo = p;
            }
            else if ( auto p = in.pRequest_AppImageStreamInfo->pAppImage_Input_Yuv.get() )
            {
                imgoInfo = p;
            }
            else if ( auto p = in.pRequest_AppImageStreamInfo->pAppImage_Input_RAW16.get() )
            {
                imgoInfo = p;
            }
            else if(in.isAppRawOutFromP2C == false)
            {
                if ( auto p = in.pRequest_AppImageStreamInfo->pAppImage_Output_Priv.get() )
                {
                    imgoInfo = p;
                }
                // RAW16
                else if(false == pRequest_PipelineNodesNeed->needRaw16Node
                        && pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() != 0)
                {
                    // P2Node directly outputs App RAW16 stream: (if no Raw16Node).
                    auto it = pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.find(in.pPipelineStaticInfo->openId);
                    if (it != pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.end())
                    {
                        imgoInfo = it->second.get();
                    }
                }
            }
        }

        // build iomap - physical image in (1/8)
        if( (need_P1Dma & P1_IMGO) || in.isSeamlessRemosaicCapture ) {
            _phygroup.imageMap.addIn(imgoInfo->getStreamId());
            hasImgo = true;
        }

        if(need_P1Dma & P1_RRZO) {
            _phygroup.imageMap.addIn(rrzoInfo->getStreamId());
            _phygroup.rrzoSize = rrzoInfo->getImgSize();
        }

        if(need_P1Dma & P1_LCSO) {
            _phygroup.imageMap.addIn(p1Info.pHalImage_P1_Lcso->getStreamId());
        }

        // build iomap - physical meta in
        _phygroup.metaMap.addIn(p1Info.pHalMeta_DynamicP1->getStreamId());
        _phygroup.metaMap.addIn(in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId());

        auto pAppMeta_DynamicP1 = p1Info.pAppMeta_DynamicP1;
        _phygroup.metaMap.addIn(pAppMeta_DynamicP1->getStreamId());
    }

    if (!hasImgo)
    {
        MY_LOGE("No Imgo");
        return OK;
    }

    if(in.pvImageStreamId_from_CaptureNode_Physical != nullptr)
    {
        auto const& pvStreamIdOutFromCapture_Image_Physical = *in.pvImageStreamId_from_CaptureNode_Physical;

        // build physical image-out and pvMetaStreamId_All_Physical
        for (auto&& [_sensorId, vStreamIds] : pvStreamIdOutFromCapture_Image_Physical)
        {
            MY_LOGD("sensor(%d) need physical image", _sensorId);
            auto iter = physicalIOMapGroupMap.find(_sensorId);
            if(iter == physicalIOMapGroupMap.end())
            {
                MY_LOGE("need output physical stream, but cannot find id(%" PRIu32 ") in physicalIOMapGroupMap",
                            _sensorId);
                return OK;
            }
            IOMapGroup &_phygroup = iter->second;

            for(auto&& streamId : vStreamIds)
            {
                if (in.isMainFrame)
                {
                    _phygroup.imageMap.addOut(streamId);
                }
            }

            // check needs output physical stream or not.
            // If needs output physical stream, add P1 app metadata to pAppMeta_DynamicP1.
            if(!in.pConfiguration_StreamInfo_P1->count(_sensorId))
            {
                MY_LOGE("cannot find sensor id in pConfiguration_StreamInfo_P1");
                continue;
            }
            auto const& p1Info = in.pConfiguration_StreamInfo_P1->at(_sensorId);
            addStreamIdToStreamMapBySensorId(out.pvMetaStreamId_All_Physical,
                _sensorId, p1Info.pAppMeta_DynamicP1->getStreamId());
        }
    }

    // build physical meta-out
    if(in.pMultiCamReqOutputParams != nullptr &&
       in.pvMetaStreamId_from_CaptureNode_Physical != nullptr)
    {
        auto &activeSensors = in.pMultiCamReqOutputParams->prvStreamingSensorList;
        auto const& pvStreamIdOutFromCapture_Meta_Physical  = *in.pvMetaStreamId_from_CaptureNode_Physical;

        for ( auto&& [_sensorId, _vStreamIds] : pvStreamIdOutFromCapture_Meta_Physical )
        {
            MY_LOGD("sensor(%d) need physical meta", _sensorId);
            if(std::find(activeSensors.begin(), activeSensors.end(), _sensorId) == activeSensors.end())
            {
                MY_LOGD("Ignore to add output physical metadata for inactive sensor %d", _sensorId);
                continue;
            }

            auto iter = physicalIOMapGroupMap.find(_sensorId);
            if(iter != physicalIOMapGroupMap.end())
            {
                for (auto&& _streamId : _vStreamIds)
                {
                    iter->second.metaMap.addOut(_streamId);
                    addStreamIdToStreamMapBySensorId(out.pvMetaStreamId_All_Physical,
                        _sensorId, _streamId);
                }
            }
        }
    }

    // build logical
    {
        for(auto&& outputStreamId : pvStreamIdOutFromCapture_Image_Logical)
        {
            // add logical input stream id by query physicalIOMapGroupMap.
            // build logical image-in and meta-in
            for (auto&& [_sensorId, _phygroup] : physicalIOMapGroupMap)
            {
                logicalIOMapGroup.imageMap.addIn(_phygroup.imageMap.vIn);
                logicalIOMapGroup.metaMap.addIn(_phygroup.metaMap.vIn);
            }
            if (in.isMainFrame)
            {
                logicalIOMapGroup.imageMap.addOut(outputStreamId);
            }
        }

        logicalIOMapGroup.metaMap.addIn(
            in.pConfiguration_StreamInfo_NonP1->pAppMeta_Control->getStreamId());

        for(const auto& streamId : pvStreamIdOutFromCapture_Meta_Logical) {
            logicalIOMapGroup.metaMap.addOut(streamId);
        }

        if( in.isAppRawOutFromP2C )
        {
            sp<IImageStreamInfo> pImgoOutput = nullptr;

            // Priv Raw
            if(pRequest_AppImageStreamInfo->pAppImage_Output_Priv.get())
            {
                // P2Node directly outputs App private stream.
                pImgoOutput = pRequest_AppImageStreamInfo->pAppImage_Output_Priv.get();
            }
            // RAW16
            else if(pRequest_PipelineNodesNeed->needRaw16Node == false)
            {
                if(pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() != 0)
                {
                    // P2Node directly outputs App RAW16 stream: (if no Raw16Node).
                    auto it = pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.find(in.pPipelineStaticInfo->openId);
                    if (it != pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.end())
                    {
                        pImgoOutput = it->second.get();
                    }
                }
                else if(pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() != 0) {
                    // P2Node directly outputs App RAW16 stream: (if no Raw16Node).
                    auto it = pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.find(in.pPipelineStaticInfo->openId);
                    if (it != pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.end())
                    {
                        pImgoOutput = it->second.get();
                    }
                }
            }

            if ( pImgoOutput != nullptr ) {
                logicalIOMapGroup.imageMap.addOut(pImgoOutput->getStreamId());
            }
        }
    }

    // build IOMapSet result
    IOMapSet P2ImgIO, P2MetaIO;
    auto buildP2IOMapSet = [&P2ImgIO, &P2MetaIO, &in](IOMapGroup &group, bool isPhysical) {
        auto combineIOMapSet = [](IOMapSet &source, IOMap &target, bool needCombine) {
            if(needCombine) {
                if(source.size() == 0) {
                    // if target size is zero, add directly.
                    source.add(target);
                }
                else {
                    // in combine flow, source size always is 1.
                    source.editItemAt(0).addIn(target.vIn);
                    source.editItemAt(0).addOut(target.vOut);
                }
            }
            else {
                source.add(target);
            }
            return;
        };

        if(
            // skip if no image since image and metadata should be 1-1 mapped
            (group.imageMap.sizeOut() == 0 && group.imageMap.sizeIn() == 0)
        )
        {
            MY_LOGD("isMainFrame %d isPhysical %d imageMap.sizeOut %zu, imageMap.sizeIn %zu, skip buildP2IOMapSet",
                    in.isMainFrame, isPhysical, group.imageMap.sizeOut(), group.imageMap.sizeIn());
            return;
        }

        group.imageMap.isPhysical = isPhysical;
        group.metaMap.isPhysical = isPhysical;
        combineIOMapSet(P2ImgIO, group.imageMap, CAP_IOMAP_COMBINE);
        combineIOMapSet(P2MetaIO, group.metaMap, CAP_IOMAP_COMBINE);
    };
    // logical
    {
        if (logicalIOMapGroup.imageMap.sizeOut() > 0) {
            out.bP2UseImgo = true;
        }
        buildP2IOMapSet(logicalIOMapGroup, false);
    }

    // physical
    {
        if (in.pvImageStreamId_from_CaptureNode_Physical != nullptr) {
            auto const& pvStreamIdOutFromCapture_Image_Physical = *in.pvImageStreamId_from_CaptureNode_Physical;
            for(auto&& n : physicalIOMapGroupMap) {
                auto iter = pvStreamIdOutFromCapture_Image_Physical.find(n.first);
                bool isAskedByApp = iter != pvStreamIdOutFromCapture_Image_Physical.end();
                if (isAskedByApp) {
                    buildP2IOMapSet(n.second, true);
                } else {
                    MY_LOGD("sensorId(%d) physical stream is not asked by app", n.first);
                }
            }
        } else {
            MY_LOGD("in.pvImageStreamId_from_CaptureNode_Physical is null");
        }
    }
    (*out.pNodeIOMapImage)[eNODEID_P2CaptureNode] = P2ImgIO;
    (*out.pNodeIOMapMeta) [eNODEID_P2CaptureNode] = P2MetaIO;

    return OK;
}


/******************************************************************************
 * Make a function target as a policy - multicam version
 ******************************************************************************/
FunctionType_IOMapPolicy_P2Node makePolicy_IOMap_P2Node_Default()
{
    return [](
        RequestOutputParams& out,
        RequestInputParams const& in
    ) -> int
    {
        evaluateRequest_P2StreamNode(out, in);
        evaluateRequest_P2CaptureNode(out, in);
        return OK;
    };
}

};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

