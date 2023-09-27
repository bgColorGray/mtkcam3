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

#define LOG_TAG "mtkcam-TopologyPolicy"

#include "MyUtils.h"

#include <mtkcam3/pipeline/policy/ITopologyPolicy.h>
#include <mtkcam3/pipeline/hwnode/NodeId.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam/utils/std/ULog.h>

#include <bitset>
#include <unordered_set>

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;


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

using namespace topology;

#ifndef RETURN_ERROR_IF_NOT_OK
#define RETURN_ERROR_IF_NOT_OK(_expr_, fmt, arg...)                     \
    do {                                                                \
        int const err = (_expr_);                                       \
        if( CC_UNLIKELY(err != 0) ) {                                   \
            MY_LOGE("err:%d(%s) - " fmt, err, ::strerror(-err), ##arg); \
            return err;                                                 \
        }                                                               \
    } while(0)
#endif

/******************************************************************************
 *
 ******************************************************************************/
static inline std::string edgeToString(NodeId_T from, NodeId_T to)
{
    std::string os;
    os += toHexString(from);
    os += "->";
    os += toHexString(to);
    return os;
}

auto SetNode_Raw16 (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused,
    int const& masterSensorIndex,
    std::vector<int> &vNeededP1IndexForDmaNull
) -> int
{
    auto const                        pCfgNodesNeed   = in.pConfiguration_PipelineNodesNeed;
    auto const                        pReqImageSet    = in.pRequest_AppImageStreamInfo;
    PipelineNodesNeed*                pNodesNeed      = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet        = out.pNodeSet;
    RequestOutputParams::NodeEdgeSet* pEdges          = out.pEdges;

    bool hasRawStreamInfo = pReqImageSet != nullptr &&
             (pReqImageSet->vAppImage_Output_RAW16.size() > 0 ||
              pReqImageSet->vAppImage_Output_RAW16_Physical.size() > 0)
             ;

    bool needToAddRaw16Node = pCfgNodesNeed->needRaw16Node &&
             hasRawStreamInfo &&
             !in.isAppRawOutFromP2C
             ;

    if (needToAddRaw16Node)
    {
        // for logical raw, only output main sensor.
        if(pReqImageSet->vAppImage_Output_RAW16.size() > 0)
        {
            if(masterSensorIndex < 0)
            {
                MY_LOGE("illegal master sensor index(%d)", masterSensorIndex);
                return -EINVAL;
            }

            int const Raw16Node = NodeIdUtils::getRaw16NodeId(masterSensorIndex);
            int const P1_Node = NodeIdUtils::getP1NodeId(masterSensorIndex);

            MY_LOGD("add raw16 edge(logical), masterSensorIndex : %d, Raw16Node : %x, P1_Node : %x",
                masterSensorIndex, Raw16Node, P1_Node);

            pNodeSet->push_back(Raw16Node);
            pEdges->addEdge(P1_Node, Raw16Node);
            pNodesNeed->needRaw16Node = true;
            vNeededP1IndexForDmaNull.push_back(masterSensorIndex);
        }

        // for physical raw
        for( const auto& raw16 : pReqImageSet->vAppImage_Output_RAW16_Physical )
        {
            int const index = in.pPipelineStaticInfo->getIndexFromSensorId(raw16.first);
            if(index < 0)
            {
                MY_LOGE("index of sensorId(%d) not found", raw16.first);
                return -EINVAL;
            }

            int const Raw16Node = NodeIdUtils::getRaw16NodeId(index);
            int const P1_Node = NodeIdUtils::getP1NodeId(index);

            MY_LOGD("add raw16 edge(physical), masterSensorIndex : %d, Raw16Node : %x, P1_Node : %x",
                masterSensorIndex, Raw16Node, P1_Node);

            pNodeSet->push_back(Raw16Node);
            pEdges->addEdge(P1_Node, Raw16Node);
            pNodesNeed->needRaw16Node = true;
            vNeededP1IndexForDmaNull.push_back(index);
        }
    }

    return OK;
}

auto SetNode_RawIspTuningDataPackNode (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused,
    int const& masterSensorIndex,
    std::vector<int> &vNeededP1IndexForDmaNull
) -> int
{
    auto const                        pCfgNodesNeed   = in.pConfiguration_PipelineNodesNeed;
    auto const                        pReqImageSet    = in.pRequest_AppImageStreamInfo;
    PipelineNodesNeed*                pNodesNeed      = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet        = out.pNodeSet;
    RequestOutputParams::NodeEdgeSet* pEdges          = out.pEdges;

    bool hasIspTuningDataRawStreamInfo = pReqImageSet != nullptr &&
             ( pReqImageSet->pAppImage_Output_IspTuningData_Raw.get() ||
              !pReqImageSet->vAppImage_Output_IspTuningData_Raw_Physical.empty())
             ; /* TODO : IspRawStreamInfo always empty, need to fix it. */

    bool hasCaptureIspTuningReqRawTag =
             (in.pRequest_ParsedAppMetaControl != nullptr &&
              in.pRequest_ParsedAppMetaControl->control_isp_tuning & MTK_CONTROL_CAPTURE_ISP_TUNING_REQ_RAW);

    bool needIspRawPackNode =
             hasIspTuningDataRawStreamInfo || hasCaptureIspTuningReqRawTag ;

    if (needIspRawPackNode)
    {
        // P1RawIspTuningDataPackNode
        if (pCfgNodesNeed->needP1RawIspTuningDataPackNode &&
            !in.isAppRawOutFromP2C)
        {
            // TODO : isp tuningdata raw stream need to fix

            if(pReqImageSet->vAppImage_Output_RAW16.size() > 0 ||
               pReqImageSet->vAppImage_Output_RAW10.size() > 0)
            {
                MY_LOGD("add P1RawIspTuningDataPackNode with edge (logical)");

                if(masterSensorIndex < 0)
                {
                    MY_LOGE("illegal master sensor index(%d)", masterSensorIndex);
                    return -EINVAL;
                }

                int const P1_Node = NodeIdUtils::getP1NodeId(masterSensorIndex);
                pEdges->addEdge(P1_Node, eNODEID_P1RawIspTuningDataPackNode);
                pNodesNeed->needP1RawIspTuningDataPackNode = true;
                pNodeSet->push_back(eNODEID_P1RawIspTuningDataPackNode);
                vNeededP1IndexForDmaNull.push_back(masterSensorIndex);
            }
            else if(!pReqImageSet->vAppImage_Output_RAW16_Physical.empty())
            {
                MY_LOGD("set P1RawIspTuningDataPackNode with edge (physical)");
                for(auto &stream : pReqImageSet->vAppImage_Output_RAW16_Physical)
                {
                    int const p1Index = in.pPipelineStaticInfo->getIndexFromSensorId(stream.first);
                    int const P1_NODE_ID = NodeIdUtils::getP1NodeId(p1Index);
                    pEdges->addEdge(P1_NODE_ID, eNODEID_P1RawIspTuningDataPackNode);
                    pNodesNeed->needP1RawIspTuningDataPackNode = true;
                    pNodeSet->push_back(eNODEID_P1RawIspTuningDataPackNode);
                }
            }
            else if(!pReqImageSet->vAppImage_Output_RAW10_Physical.empty())
            {
                MY_LOGD("set P1RawIspTuningDataPackNode with edge (physical)");
                for(auto &stream : pReqImageSet->vAppImage_Output_RAW10_Physical)
                {
                    int const p1Index = in.pPipelineStaticInfo->getIndexFromSensorId(stream.first);
                    if (p1Index >= 0) {
                        int const P1_NODE_ID =
                            NodeIdUtils::getP1NodeId(p1Index);
                        pEdges->addEdge(P1_NODE_ID, eNODEID_P1RawIspTuningDataPackNode);
                        pNodesNeed->needP1RawIspTuningDataPackNode = true;
                        pNodeSet->push_back(eNODEID_P1RawIspTuningDataPackNode);
                    }
                }
            }

            if(in.pMultiCamReqOutputParams != nullptr)
            {
                auto prvlist = in.pMultiCamReqOutputParams->prvStreamingSensorList;

                for(auto &stream : pReqImageSet->vAppImage_Output_IspTuningData_Raw_Physical)
                {
                    if ( std::find(prvlist.begin(), prvlist.end(), stream.first) != prvlist.end() )
                    {
                        MY_LOGD("add P1RawIspTuningDataPackNode with edge (physical)");

                        int const p1Index = in.pPipelineStaticInfo->getIndexFromSensorId(stream.first);
                        if( p1Index < 0 )
                        {
                            MY_LOGE("index of sensorId(%d) not found", stream.first);
                            return -EINVAL;
                        }

                        int const P1_NODE_ID = NodeIdUtils::getP1NodeId(p1Index);
                        pEdges->addEdge(P1_NODE_ID, eNODEID_P1RawIspTuningDataPackNode);
                        pNodesNeed->needP1RawIspTuningDataPackNode = true;
                        vNeededP1IndexForDmaNull.push_back(p1Index);
                    }
                }
            }
            if(pNodesNeed->needP1RawIspTuningDataPackNode)
            {
                pNodeSet->push_back(eNODEID_P1RawIspTuningDataPackNode);
            }
        }
        // P2RawIspTuningDataPackNode
        else if (pCfgNodesNeed->needP2RawIspTuningDataPackNode &&
                 in.needP2CaptureNode)
        {
            MY_LOGD("add P2RawIspTuningDataPackNode edge");
            pEdges->addEdge(eNODEID_P2CaptureNode, eNODEID_P2RawIspTuningDataPackNode);
            pNodesNeed->needP2RawIspTuningDataPackNode = true;
            pNodeSet->push_back(eNODEID_P2RawIspTuningDataPackNode);
            vNeededP1IndexForDmaNull.push_back(masterSensorIndex);
        }
    }

    return OK;
}

auto SetNode_Jpeg (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused
) -> int
{
    auto const                        pReqImageSet    = in.pRequest_AppImageStreamInfo;
    PipelineNodesNeed*                pNodesNeed      = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet        = out.pNodeSet;

    if ( pReqImageSet != nullptr && pReqImageSet->pAppImage_Jpeg.get() )
    {
        RequestOutputParams::NodeEdgeSet* pEdges = out.pEdges;
        auto const pCfgStream_NonP1              = in.pConfiguration_StreamInfo_NonP1;

        bool found = false;
        const auto& streamId = (pCfgStream_NonP1->pHalImage_Jpeg_YUV != nullptr) ?
            pCfgStream_NonP1->pHalImage_Jpeg_YUV->getStreamId() : eSTREAMID_NO_STREAM;

        for ( const auto& s : *(in.pvImageStreamId_from_CaptureNode) ) {
            if ( s == streamId ) {
                pEdges->addEdge(eNODEID_P2CaptureNode, eNODEID_JpegNode);
                found = true;
                break;
            }
        }

        if ( !found ) {
            for ( const auto& s : *(in.pvImageStreamId_from_StreamNode) ) {
                if ( s == streamId ) {
                    pEdges->addEdge(eNODEID_P2StreamNode, eNODEID_JpegNode);
                    found = true;
                    break;
                }
            }
        }

        if ( !found ) {
            // Heif : JPEG_APP_SEGMENT needs connect JpegNode
            if (pReqImageSet->pAppImage_Jpeg->getImgFormat() == eImgFmt_JPEG_APP_SEGMENT) {
                pEdges->addEdge(eNODEID_P2CaptureNode, eNODEID_JpegNode);
                found = true;
            }
        }

        if ( !found ) {
            MY_LOGE("no p2 streaming&capture node w/ jpeg output");
            return -EINVAL;
        }

        pNodesNeed->needJpegNode = true;
        pNodeSet->push_back(eNODEID_JpegNode);
    }

    return OK;
}

auto SetNode_P2YuvIspTuningDataPackNode (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused
) -> int
{
    auto const                        pCfgNodesNeed   = in.pConfiguration_PipelineNodesNeed;
    auto const                        pReqImageSet    = in.pRequest_AppImageStreamInfo;
    PipelineNodesNeed*                pNodesNeed      = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet        = out.pNodeSet;
    RequestOutputParams::NodeEdgeSet* pEdges          = out.pEdges;

    bool hasIspTuningDataYuvStreamInfo = pReqImageSet != nullptr &&
             ( pReqImageSet->pAppImage_Output_IspTuningData_Yuv.get() ||
              !pReqImageSet->vAppImage_Output_IspTuningData_Yuv_Physical.empty())
             ;

    bool hasCaptureIspTuningReqYuvTag =
             in.pRequest_ParsedAppMetaControl != nullptr &&
             (in.pRequest_ParsedAppMetaControl->control_isp_tuning & MTK_CONTROL_CAPTURE_ISP_TUNING_REQ_YUV)
             ;

    if (pCfgNodesNeed->needP2YuvIspTuningDataPackNode
         && in.needP2CaptureNode
         && ( hasIspTuningDataYuvStreamInfo || hasCaptureIspTuningReqYuvTag ))
    {
        pEdges->addEdge(eNODEID_P2CaptureNode, eNODEID_P2YuvIspTuningDataPackNode);
        pNodesNeed->needP2YuvIspTuningDataPackNode = true;
        pNodeSet->push_back(eNODEID_P2YuvIspTuningDataPackNode);
    }

    return OK;
}

auto SetNode_FD (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused
) -> int
{
    PipelineNodesNeed*                pNodesNeed      = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet        = out.pNodeSet;
    RequestOutputParams::NodeEdgeSet* pEdges          = out.pEdges;

    if ( in.isFdEnabled )
    {
        pNodesNeed->needFDNode = true;
        if ( in.useP1FDYUV )
        {
            auto targetFd_idx = in.pPipelineStaticInfo->getIndexFromSensorId(in.targetFd);
            if (targetFd_idx >= 0) {
                NodeId_T fdNodeId =
                    NodeIdUtils::getFDNodeId((size_t) targetFd_idx);
                NodeId_T p1NodeId =
                    NodeIdUtils::getP1NodeId((size_t) targetFd_idx);
                pNodeSet->push_back(fdNodeId);
                pEdges->addEdge(p1NodeId, fdNodeId);
            }
        }
        else
        {
            pNodeSet->push_back(eNODEID_FDNode);
            if ( in.needP2CaptureNode && !in.needP2StreamNode )
            {
                pEdges->addEdge(eNODEID_P2CaptureNode, eNODEID_FDNode);
            }
            else if ( in.needP2StreamNode )
            {
                pEdges->addEdge(eNODEID_P2StreamNode, eNODEID_FDNode);
            }
        }
    }

    return OK;
}

auto SetNode_Streaming (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused,
    std::vector<int> const& vP1ToStreaming
) -> int
{
    PipelineNodesNeed*                pNodesNeed = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet   = out.pNodeSet;
    RequestOutputParams::NodeEdgeSet* pEdges     = out.pEdges;

    if ( in.needP2StreamNode )
    {
        pNodesNeed->needP2StreamNode = true;
        pNodeSet->push_back(eNODEID_P2StreamNode);

        for (const auto& p1node : vP1ToStreaming)
        {
            pEdges->addEdge(p1node, eNODEID_P2StreamNode);
        }
    }

    return OK;
}

auto SetNode_Capture (
    RequestOutputParams& out __unused,
    RequestInputParams const& in __unused,
    std::vector<int> const& vP1ToCapture
) -> int
{
    PipelineNodesNeed*                pNodesNeed = out.pNodesNeed;
    std::vector<NodeId_T>*            pNodeSet   = out.pNodeSet;
    RequestOutputParams::NodeEdgeSet* pEdges     = out.pEdges;

    if ( in.needP2CaptureNode )
    {
        pNodesNeed->needP2CaptureNode = true;
        pNodeSet->push_back(eNODEID_P2CaptureNode);

        for (const auto& p1node : vP1ToCapture)
        {
            pEdges->addEdge(p1node, eNODEID_P2CaptureNode);
        }
    }

    return OK;
}

auto CheckParam(
    RequestInputParams const& in __unused
) -> int
{
    auto const pCfgNodesNeed    = in.pConfiguration_PipelineNodesNeed;
    auto const pCfgStream_NonP1 = in.pConfiguration_StreamInfo_NonP1;
    auto const pReqImageSet     = in.pRequest_AppImageStreamInfo;

    if ( CC_UNLIKELY( !pCfgNodesNeed || !pCfgStream_NonP1) )
    {
        MY_LOGE("null configuration params");
        return -EINVAL;
    }

    return OK;
}

/**
 * Make a function target as a policy - default version
 */
FunctionType_TopologyPolicy makePolicy_Topology_Default()
{
    return [](
        RequestOutputParams& out __unused,
        RequestInputParams const& in __unused
    ) -> int
    {
        SensorMap<uint32_t> const*      pvNeedP1Dma = in.pvNeedP1Dma;
        PipelineNodesNeed*                pNodesNeed = out.pNodesNeed;
        std::vector<NodeId_T>*            pNodeSet   = out.pNodeSet;
        RequestOutputParams::NodeSet*     pRootNodes = out.pRootNodes;
        RequestOutputParams::NodeEdgeSet* pEdges     = out.pEdges;

        RETURN_ERROR_IF_NOT_OK( CheckParam(in),
            "Check parameter fail");

        int masterSensorIndex = -1;

        if (in.pMultiCamReqOutputParams &&
            in.pMultiCamReqOutputParams->prvStreamingSensorList.size() > 0)
        {
            // Index 0 is always master id
            int id = in.pMultiCamReqOutputParams->prvStreamingSensorList[0];
            masterSensorIndex = in.pPipelineStaticInfo->getIndexFromSensorId(id);
        }
        else // n = 1
        {
            masterSensorIndex = 0;
        }

        std::vector<int> vNeededP1IndexForDmaNull;

        RETURN_ERROR_IF_NOT_OK(
            SetNode_Raw16(out, in, masterSensorIndex, vNeededP1IndexForDmaNull /*out*/),
            "Set Raw16Node fail");

        RETURN_ERROR_IF_NOT_OK(
            SetNode_RawIspTuningDataPackNode(out, in, masterSensorIndex, vNeededP1IndexForDmaNull /*out*/),
            "Set RawIspTuningDataPackNode fail");

        std::vector<int> vP1ToStreaming;
        std::vector<int> vP1ToCapture;

        if(pvNeedP1Dma == nullptr)
        {
            for(auto&& idx : vNeededP1IndexForDmaNull)
            {
                if(idx < 0)
                {
                    MY_LOGE("illegal sensor index(%d)", idx);
                    return -EINVAL;
                }

                // handle p1 node.
                pRootNodes->add(NodeIdUtils::getP1NodeId(idx));
                pNodeSet->push_back(NodeIdUtils::getP1NodeId(idx));
                auto sensorId = in.pPipelineStaticInfo->sensorId[idx];
                pNodesNeed->needP1Node.at(sensorId) = true;
            }
        }
        else
        {
            for (const auto& [_sensorId, _p1Dma] : *pvNeedP1Dma)
            {
                auto sensorIndex = in.pPipelineStaticInfo->getIndexFromSensorId(_sensorId);
                if(sensorIndex < 0)
                {
                    MY_LOGE("illegal sensor index(%d)", sensorIndex);
                    continue;
                }
                if(!_p1Dma)
                {
                    pNodesNeed->needP1Node.emplace(_sensorId, false);
                    continue;
                }

                auto p1NodeId = NodeIdUtils::getP1NodeId(sensorIndex);
                pRootNodes->add(p1NodeId);
                pNodesNeed->needP1Node.emplace(_sensorId, true);
                pNodeSet->push_back(p1NodeId);
                vP1ToStreaming.push_back(p1NodeId);
                vP1ToCapture.push_back(p1NodeId);
            }
        }

        if ( in.isDummyFrame )
        {
            return OK;
        }

        RETURN_ERROR_IF_NOT_OK(
            SetNode_Jpeg(out, in),
            "Set JpegNode fail");

        RETURN_ERROR_IF_NOT_OK(
            SetNode_P2YuvIspTuningDataPackNode(out, in),
            "Set P2YuvIspTuningDataPackNode fail");

        RETURN_ERROR_IF_NOT_OK(
            SetNode_FD(out, in),
            "Set FD Node fail");

        RETURN_ERROR_IF_NOT_OK(
            SetNode_Streaming(out, in, vP1ToStreaming),
            "Set StreamingNode fail");

        RETURN_ERROR_IF_NOT_OK(
            SetNode_Capture(out, in, vP1ToCapture),
            "Set CaptureNode fail");

        return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

