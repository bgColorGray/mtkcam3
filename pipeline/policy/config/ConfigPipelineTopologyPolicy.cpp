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

#define LOG_TAG "mtkcam-ConfigPipelineTopologyPolicy"

#include <mtkcam3/pipeline/policy/IConfigPipelineTopologyPolicy.h>
//
#include <mtkcam3/pipeline/hwnode/NodeId.h>
#include <mtkcam/utils/std/ULog.h>
#include "MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);


/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam::v3::pipeline::policy;


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


/****************************************** ************************************
 *
 ******************************************************************************/
template <typename fromT, typename toT>
static inline std::string edgeToString(fromT from, toT to)
{
    std::string os;
    os += toHexString(from);
    os += "->";
    os += toHexString(to);
    return os;
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToJpegNode(PipelineTopology::NodeEdgeSet& edges, PipelineNodesNeed const* pPipelineNodesNeed)
{
    if ( pPipelineNodesNeed->needJpegNode ) {

        if ( pPipelineNodesNeed->needP2StreamNode ) {
            edges.addEdge(eNODEID_P2StreamNode, eNODEID_JpegNode);
        }

        if ( pPipelineNodesNeed->needP2CaptureNode ) {
            edges.addEdge(eNODEID_P2CaptureNode, eNODEID_JpegNode);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToP1RawIspTuningDataPackNode(PipelineTopology::NodeEdgeSet& edges, PipelineNodesNeed const* pPipelineNodesNeed)
{
    // for p1 raw isp tuning data pack, it has to
    // connect all p1 node to raw isp tuning data pack node.
    if ( pPipelineNodesNeed->needP1RawIspTuningDataPackNode ) {
        for (size_t i = 0; i < pPipelineNodesNeed->needP1Node.size(); i++) {
            auto p1nodeId = NodeIdUtils::getP1NodeId(i);
            edges.addEdge(p1nodeId, eNODEID_P1RawIspTuningDataPackNode);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToP2YuvIspTuningDataPackNode(PipelineTopology::NodeEdgeSet& edges, PipelineNodesNeed const* pPipelineNodesNeed)
{
    if ( pPipelineNodesNeed->needP2YuvIspTuningDataPackNode ) {
        edges.addEdge(eNODEID_P2CaptureNode, eNODEID_P2YuvIspTuningDataPackNode);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
static void connectToP2RawIspTuningDataPackNode(PipelineTopology::NodeEdgeSet& edges, PipelineNodesNeed const* pPipelineNodesNeed)
{
    if ( pPipelineNodesNeed->needP2RawIspTuningDataPackNode ) {
        edges.addEdge(eNODEID_P2CaptureNode, eNODEID_P2RawIspTuningDataPackNode);
    }
}

/******************************************************************************
 *
 ******************************************************************************/

static void connectToFDNode(PipelineTopology::NodeEdgeSet& edges,
                            PipelineNodesNeed const* pPipelineNodesNeed,
                            std::shared_ptr<ParsedAppConfiguration> pParsedAppConfiguration,
                            PipelineStaticInfo const* pPipelineStaticInfo)
{
    if ( pPipelineNodesNeed->needFDNode ) {

        if ( pParsedAppConfiguration->useP1DirectFDYUV )
        {
            if (pParsedAppConfiguration->pParsedMultiCamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
            {
                for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node) {
                    if (_needP1) {
                        int idx = pPipelineStaticInfo->getIndexFromSensorId(_sensorId);
                        if(idx >= 0)
                            edges.addEdge(NodeIdUtils::getP1NodeId(idx), NodeIdUtils::getFDNodeId(idx));
                    }
                }
            }
            else
            {
                edges.addEdge(NodeIdUtils::getP1NodeId(0), eNODEID_FDNode);
            }
        }
        else
        {
            if ( pPipelineNodesNeed->needP2StreamNode )
            {
                edges.addEdge(eNODEID_P2StreamNode, eNODEID_FDNode);
            }
            if ( pPipelineNodesNeed->needP2CaptureNode )
            {
                edges.addEdge(eNODEID_P2CaptureNode, eNODEID_FDNode);
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToRaw16Node(PipelineTopology::NodeEdgeSet& edges,
                               PipelineNodesNeed const* pPipelineNodesNeed,
                               PipelineUserConfiguration const* pPipelineUserConfiguration,
                               PipelineStaticInfo const* pPipelineStaticInfo)
{
    if ( pPipelineNodesNeed->needRaw16Node ) {
        for (auto const& [_sensorId, _stream] : pPipelineUserConfiguration->pParsedAppImageStreamInfo->vAppImage_Output_RAW16)
        {
            int const index = pPipelineStaticInfo->getIndexFromSensorId(_sensorId);
            if ( index>=0 && pPipelineNodesNeed->needP1Node.count(_sensorId) && pPipelineNodesNeed->needP1Node.at(_sensorId) ) {
                edges.addEdge(NodeIdUtils::getP1NodeId(index), NodeIdUtils::getRaw16NodeId(index));
                MY_LOGD("add raw16 edge : %s", edgeToString(NodeIdUtils::getP1NodeId(index), NodeIdUtils::getRaw16NodeId(index)).c_str());
            }
        }

        for (auto const& [_sensorId, _vStreams] : pPipelineUserConfiguration->pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical)
        {
            if(_vStreams.size() > 0) {
                int const index = pPipelineStaticInfo->getIndexFromSensorId(_sensorId);
                if ( index>=0 && pPipelineNodesNeed->needP1Node.count(_sensorId) && pPipelineNodesNeed->needP1Node.at(_sensorId) ) {
                    edges.addEdge(NodeIdUtils::getP1NodeId(index), NodeIdUtils::getRaw16NodeId(index));
                    MY_LOGD("add raw16 edge(physical) : %s", edgeToString(NodeIdUtils::getP1NodeId(index), NodeIdUtils::getRaw16NodeId(index)).c_str());
                }
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToPDENode(PipelineTopology::NodeEdgeSet& edges,
                             PipelineNodesNeed const* pPipelineNodesNeed,
                             PipelineStaticInfo const* pPipelineStaticInfo)
{
    if ( pPipelineNodesNeed->needPDENode ) {
        auto main1SensorId = pPipelineStaticInfo->sensorId[0];
        if ( pPipelineNodesNeed->needP1Node.count(main1SensorId) &&
             pPipelineNodesNeed->needP1Node.at(main1SensorId) )
        {
            edges.addEdge(NodeIdUtils::getP1NodeId(0), eNODEID_PDENode);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToP2StreamNode(PipelineTopology::NodeEdgeSet& edges,
                                  PipelineNodesNeed const* pPipelineNodesNeed,
                                  PipelineStaticInfo const* pPipelineStaticInfo)
{
    if ( pPipelineNodesNeed->needP2StreamNode ) {
        for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node) {
            if (_needP1) {
                auto sensorIndex = pPipelineStaticInfo->getIndexFromSensorId(_sensorId);
                if (sensorIndex >= 0) {
                    edges.addEdge(NodeIdUtils::getP1NodeId(sensorIndex), eNODEID_P2StreamNode);
                }
            }
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void connectToP2CaptureNode(PipelineTopology::NodeEdgeSet& edges,
                                   PipelineNodesNeed const* pPipelineNodesNeed,
                                   PipelineStaticInfo const* pPipelineStaticInfo)
{
    if ( pPipelineNodesNeed->needP2CaptureNode ) {
        for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node) {
            if (_needP1) {
                auto sensorIndex = pPipelineStaticInfo->getIndexFromSensorId(_sensorId);
                if (sensorIndex >= 0) {
                    edges.addEdge(NodeIdUtils::getP1NodeId(sensorIndex), eNODEID_P2CaptureNode);
                }
            }
        }
    }
}


/**
 * default version
 */
FunctionType_Configuration_PipelineTopologyPolicy makePolicy_Configuration_PipelineTopology_Default()
{
    return [](Configuration_PipelineTopology_Params const& params) -> int {
        auto pPipelineNodesNeed = params.pPipelineNodesNeed;
        auto pOut = params.pOut;
        auto& edges = pOut->edges;

        //roots
        for (size_t i = 0; i < pPipelineNodesNeed->needP1Node.size(); i++) {
            pOut->roots.add(NodeIdUtils::getP1NodeId(i));
        }

        //edges
        connectToP2StreamNode(edges, pPipelineNodesNeed, params.pPipelineStaticInfo);
        connectToP2CaptureNode(edges, pPipelineNodesNeed, params.pPipelineStaticInfo);
        connectToRaw16Node(edges, pPipelineNodesNeed, params.pPipelineUserConfiguration, params.pPipelineStaticInfo);
        connectToPDENode(edges, pPipelineNodesNeed, params.pPipelineStaticInfo);
        connectToFDNode(edges, pPipelineNodesNeed, params.pPipelineUserConfiguration->pParsedAppConfiguration,
                            params.pPipelineStaticInfo);
        connectToJpegNode(edges, pPipelineNodesNeed);
        connectToP1RawIspTuningDataPackNode(edges, pPipelineNodesNeed);
        connectToP2YuvIspTuningDataPackNode(edges, pPipelineNodesNeed);
        connectToP2RawIspTuningDataPackNode(edges, pPipelineNodesNeed);

        return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

