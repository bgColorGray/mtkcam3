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

 // Standard C header file
#include <time.h>
#include <string.h>
// Android system/core header file
#include <fstream>
// mtkcam custom header file

// mtkcam global header file
#include <mtkcam/utils/TuningUtils/CommonRule.h>
// Module header file
#include <DebugUtil.h>
// Local header file
#include "DepthMapPipeNode.h"
#include "./bufferPoolMgr/BaseBufferHandler.h"
#include "./bufferConfig/BaseBufferConfig.h"
// logging
#undef PIPE_CLASS_TAG
#define PIPE_CLASS_TAG "DepthMapPipeNode"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);
/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {
/*******************************************************************************
* NodeSignal Definition
********************************************************************************/

NodeSignal::NodeSignal()
: mSignal(0)
, mStatus(0)
{
}

NodeSignal::~NodeSignal()
{
}

// MVOID NodeSignal::setSignal(Signal signal)
// {
//     android::Mutex::Autolock lock(mMutex);
//     mSignal |= signal;
//     mCondition.broadcast();
// }

// MVOID NodeSignal::clearSignal(Signal signal)
// {
//     android::Mutex::Autolock lock(mMutex);
//     mSignal &= ~signal;
// }

// MBOOL NodeSignal::getSignal(Signal signal)
// {
//     android::Mutex::Autolock lock(mMutex);
//     return (mSignal & signal);
// }

// MVOID NodeSignal::waitSignal(Signal signal)
// {
//     android::Mutex::Autolock lock(mMutex);
//     while( !(mSignal & signal) )
//     {
//         mCondition.wait(mMutex);
//     }
// }

MVOID NodeSignal::setStatus(Status status)
{
    android::Mutex::Autolock lock(mMutex);
    mStatus |= status;
}

MVOID NodeSignal::clearStatus(Status status)
{
    android::Mutex::Autolock lock(mMutex);
    mStatus &= ~status;
}

MBOOL NodeSignal::getStatus(Status status)
{
    android::Mutex::Autolock lock(mMutex);
    return (mStatus & status);
}

/*******************************************************************************
* DepthMapDataHandler Definition
********************************************************************************/
DataIDToBIDMap DepthMapPipeNode::mDataIDToBIDMap = getDataIDToBIDMap();

const char* DepthMapDataHandler::ID2Name(DataID id)
{
#define MAKE_NAME_CASE(name) \
  case name: return #name;

  switch(id)
  {
    MAKE_NAME_CASE(ID_INVALID);
    MAKE_NAME_CASE(ROOT_ENQUE);
    MAKE_NAME_CASE(BAYER_ENQUE);

    MAKE_NAME_CASE(P2A_TO_N3D_PADDING_MATRIX);
    MAKE_NAME_CASE(P2A_TO_N3D_RECT2_FEO);
    MAKE_NAME_CASE(P2A_TO_N3D_FEOFMO);
    MAKE_NAME_CASE(P2A_TO_N3D_NOFEFM_RECT2);
    MAKE_NAME_CASE(P2A_TO_N3D_CAP_RECT2);
    MAKE_NAME_CASE(P2A_TO_DVP_MY_S);
    MAKE_NAME_CASE(P2A_TO_DLDEPTH_MY_S);
    MAKE_NAME_CASE(N3D_TO_WPE_RECT2_WARPMTX);
    MAKE_NAME_CASE(N3D_TO_WPE_WARPMTX);
    MAKE_NAME_CASE(N3D_TO_DVS_MVSV_MASK);
    MAKE_NAME_CASE(WPE_TO_DVS_WARP_IMG);
    MAKE_NAME_CASE(WPE_TO_DLDEPTH_MV_SV);
    MAKE_NAME_CASE(DVP_TO_GF_DMW_N_DEPTH);
    MAKE_NAME_CASE(DVS_TO_DLDEPTH_MVSV_NOC);
    MAKE_NAME_CASE(DVS_TO_DVP_NOC_CFM);
    MAKE_NAME_CASE(DVS_TO_SLANT_NOC_CFM);
    MAKE_NAME_CASE(DLDEPTH_TO_GF_DEPTHMAP);
    MAKE_NAME_CASE(DLDEPTH_TO_SLANT_DEPTH);
    MAKE_NAME_CASE(SLANT_TO_DVP_NOC_CFM);
    MAKE_NAME_CASE(SLANT_TO_GF_DEPTH_CFM);
    MAKE_NAME_CASE(SLANT_OUT_INTERNAL_DEPTH);
    MAKE_NAME_CASE(SLANT_OUT_DEPTHMAP);
    // DepthMap output
    MAKE_NAME_CASE(P2A_OUT_MV_F);
    MAKE_NAME_CASE(P2A_OUT_FD);
    MAKE_NAME_CASE(P2A_OUT_MV_F_CAP);
    MAKE_NAME_CASE(P2A_OUT_YUV_DONE);
    MAKE_NAME_CASE(DEPTHPIPE_BUF_RELEASED);
    MAKE_NAME_CASE(P2A_NORMAL_FRAME_DONE);
    MAKE_NAME_CASE(GF_OUT_DMBG);
    MAKE_NAME_CASE(GF_OUT_INTERNAL_DMBG);
    MAKE_NAME_CASE(DEPTHMAP_META_OUT);
    MAKE_NAME_CASE(DVS_OUT_DEPTHMAP);
    MAKE_NAME_CASE(DVS_OUT_INTERNAL_DEPTH);
    MAKE_NAME_CASE(DLDEPTH_OUT_DEPTHMAP);
    MAKE_NAME_CASE(DLDEPTH_OUT_INTERNAL_DEPTH);
    MAKE_NAME_CASE(DVP_OUT_DEPTHMAP);
    MAKE_NAME_CASE(DVP_OUT_INTERNAL_DEPTH);
    MAKE_NAME_CASE(GF_OUT_DEPTHMAP);
    MAKE_NAME_CASE(GF_OUT_INTERNAL_DEPTH);

    #ifdef GTEST
    MAKE_NAME_CASE(UT_OUT_FE);
    #endif
    MAKE_NAME_CASE(TO_DUMP_MAPPINGS);
    MAKE_NAME_CASE(TO_DUMP_IMG3O);
    MAKE_NAME_CASE(TO_DUMP_RAWS);
    MAKE_NAME_CASE(TO_DUMP_YUVS);
    MAKE_NAME_CASE(ERROR_OCCUR_NOTIFY);
    MAKE_NAME_CASE(QUEUED_FLOW_DONE);
    MAKE_NAME_CASE(REQUEST_DEPTH_NOT_READY);

  };
  MY_LOGW(" unknown id:%d", id);
  return "UNKNOWN";
#undef MAKE_NAME_CASE
}

DepthMapDataHandler::
DepthMapDataHandler()
{}

DepthMapDataHandler::
~DepthMapDataHandler()
{
}


DepthMapBufferID
DepthMapDataHandler::
mapQueuedBufferID(
    DepthMapRequestPtr pRequest,
    sp<DepthMapPipeOption> pOption,
    DepthMapBufferID bid)
{
    if(pRequest->isQueuedDepthRequest(pOption))
    {
        switch(bid)
        {
            case BID_META_IN_APP:
                return BID_META_IN_APP_QUEUED;
            case BID_META_IN_HAL_MAIN1:
                return BID_META_IN_HAL_MAIN1_QUEUED;
            case BID_META_IN_HAL_MAIN2:
                return BID_META_IN_HAL_MAIN2_QUEUED;
            case BID_META_OUT_APP:
                return BID_META_OUT_APP_QUEUED;
            case BID_META_OUT_HAL:
                return BID_META_OUT_HAL_QUEUED;
            case BID_META_IN_P1_RETURN:
                return BID_META_IN_P1_RETURN_QUEUED;
            default:
                break;
        }
    }
    return bid;
}
/*******************************************************************************
* DepthMapPipeNode Definition
********************************************************************************/
DepthMapPipeNode::
DepthMapPipeNode(
    const char *name,
    DepthMapPipeNodeID nodeID,
    PipeNodeConfigs config
)
: CamThreadNode(name)
, mpFlowOption(config.mpFlowOption)
, mpPipeOption(config.mpPipeOption)
, mNodeId(nodeID)
{
    miDumpBufSize  =::property_get_int32("vendor.dualfeature.dump.size"       , 0);
    miDumpStartIdx =::property_get_int32("vendor.dualfeature.dump.start"      , 0);
    miNDDLevel     =::property_get_int32("vendor.dualfeature.dump.NDDLevel"   , 2);
    mbDumpCapture  =::property_get_int32("vendor.dualfeature.dump.capture"    , 0);
    miBitTrueFrame =::property_get_int32("vendor.debug.vsdof.bitture.frameNo" , 0);
    miDepthDelay   =::property_get_int32("vendor.depthmap.pipe.enableWaitDepth", 0);
    // Bit true enabled, need dump bittrue frame
    if(miBitTrueFrame)
    {
        miDumpBufSize  = 1;
        miDumpStartIdx = 1;
        miDumpBufSize  =::property_get_int32("vendor.dualfeature.dump.size" , 1);
        miDumpStartIdx =::property_get_int32("vendor.dualfeature.dump.start", miBitTrueFrame);
    }
    // 3A operation may cost time, so we only extract by sensor id once
    if(miDumpBufSize > 0)
    {
        int main1Idx, main2Idx;
        StereoSettingProvider::getStereoSensorIndex(main1Idx, main2Idx);
        extract_by_SensorOpenId(&mDumpHint_Main1, main1Idx);
        extract_by_SensorOpenId(&mDumpHint_Main2, main2Idx);
    }
    MY_LOGD("miDebugLog=%d mbProfileLog=%d, miDumpBufSize=%d miDumpStartIdx=%d mbDumpCapture=%d miNDDLevel=%d miDepthDelay=%d",
            DepthPipeLoggingSetup::miDebugLog, DepthPipeLoggingSetup::mbProfileLog,
            miDumpBufSize, miDumpStartIdx, mbDumpCapture, miNDDLevel, miDepthDelay);
}

DepthMapPipeNode::~DepthMapPipeNode()
{
}

MVOID
DepthMapPipeNode::
dumpSensorMapping(DepthMapRequestPtr &pRequest)
{
    if(checkToDump(TO_DUMP_MAPPINGS, pRequest)
        && (pRequest->getRequestAttr().opState == eSTATE_NORMAL
        || pRequest->getRequestAttr().opState == eSTATE_RECORD))
    {
        const size_t PATH_SIZE = 1024;
        char filepath[PATH_SIZE];
        char writepath[PATH_SIZE];
        sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
        IMetadata* pInHalMeta_Main1 = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN1);
        extract(&mDumpHint_Main1, pInHalMeta_Main1);
        //
        IMetadata* pInHalMeta_Main2 = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN2);
        extract(&mDumpHint_Main2, pInHalMeta_Main2);

        TuningUtils::MakePrefix(filepath, PATH_SIZE, mDumpHint_Main1.UniqueKey, mDumpHint_Main1.FrameNo, mDumpHint_Main1.RequestNo);
        // write file path
        int err = snprintf(writepath, PATH_SIZE, "%s-%s_%d_%s_%d", filepath, SENSOR_DEV_TO_STRING(mDumpHint_Main1.SensorDev),
                    mDumpHint_Main1.FrameNo, SENSOR_DEV_TO_STRING(mDumpHint_Main2.SensorDev), mDumpHint_Main2.FrameNo);
        std::fstream fs;
        fs.open (writepath, std::fstream::out);
        fs.close();
    }
}

MBOOL
DepthMapPipeNode::
onDump(
    DataID id,
    DepthMapRequestPtr &pRequest,
    DumpConfig* config
)
{
// #ifdef GTEST_PROFILE
//     return MTRUE;
// #endif
#ifndef GTEST
    if(!checkToDump(id, pRequest))
    {
        VSDOF_LOGD("onDump reqID=%d dataid=%d(%s), checkDump failed! standalone=%d",
                    pRequest->getRequestNo(), id, ID2Name(id),
                    pRequest->getRequestAttr().opState == eSTATE_STANDALONE);
        return MFALSE;
    }
#endif
    MY_LOGD("%s onDump reqID=%d dataid=%d(%s)", getName(), pRequest->getRequestNo(), id, ID2Name(id));

    char* fileName = (config != NULL) ? config->fileName : NULL;
    char* postfix = (config != NULL) ? config->postfix : NULL;
    MBOOL bStridePostfix = (config != NULL) ? config->bStridePostfix : MFALSE;
    MUINT iReqIdx = pRequest->getRequestNo();

    if(mDataIDToBIDMap.indexOfKey(id)<0)
    {
        MY_LOGD("%s onDump: reqID=%d, cannot find BID map of the data id:%d! Chk BaseBufferConfig.cpp",
                getName(), pRequest->getRequestNo(), id);
        return MFALSE;
    }

    const size_t PATH_SIZE = 1024;
    char filepath[PATH_SIZE];
    //Get metadata
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IMetadata* pInHalMeta = pBufferHandler->requestMetadata(getNodeId(), BID_META_IN_HAL_MAIN1);
    extract(&mDumpHint_Main1, pInHalMeta);

    // change DataID if NDD is Level1 only
    if(miNDDLevel == 1)
        id = mapNDDL1DataID(id);
    // get the buffer id array for dumping
    const Vector<DepthMapBufferID>& vDumpBufferID = mDataIDToBIDMap.valueFor(id);
    char writepath[PATH_SIZE];
    char filename[PATH_SIZE];
    char strideStr[100];

    VSDOF_LOGD("dataID:%d buffer id size=%zu", id, vDumpBufferID.size());
    for(size_t i=0;i<vDumpBufferID.size();++i)
    {
        const DepthMapBufferID& oriBID = vDumpBufferID.itemAt(i);
        DepthMapBufferID BID = mpFlowOption->reMapBufferID(pRequest->getRequestAttr(), oriBID);

        VSDOF_LOGD("Dump -- index%zu, buffer id=%d", i, BID);
        IImageBuffer* pImgBuf = nullptr;
        MBOOL bRet = pBufferHandler->getEnqueBuffer(this->getNodeId(), BID, pImgBuf);
        if (BID == BID_N3D_OUT_MASK_M_STATIC) {
            pImgBuf = pBufferHandler->requestBuffer(this->getNodeId(), BID);
            bRet = MTRUE;
        }
        // use FileCache to dump buffer
        if(!bRet)
        {
            VSDOF_LOGD("Failed to get enqued buffer, id: %d", BID);
            continue;
        }

        int cx = 0;
        // stride string
        if(bStridePostfix) {
            cx = snprintf(filename, PATH_SIZE, "%s_%lu_reqID_%d", (fileName != NULL) ? fileName : onDumpBIDToName(BID),
                                                         pImgBuf->getBufStridesInBytes(0), pRequest->getRequestNo());
        } else {
            cx = snprintf(filename, PATH_SIZE, "%s_reqID_%d", (fileName != NULL) ? fileName : onDumpBIDToName(BID), pRequest->getRequestNo());
        }

        extract(&mDumpHint_Main1, pImgBuf);
        switch(BID) {
            case BID_P2A_IN_FSRAW1:
            case BID_P2A_IN_FSRAW2:
                genFileName_RAW(writepath, PATH_SIZE, &mDumpHint_Main1, TuningUtils::RAW_PORT_IMGO, filename);
            break;
            case BID_P2A_IN_RSRAW1:
            case BID_P2A_IN_RSRAW2:
                genFileName_RAW(writepath, PATH_SIZE, &mDumpHint_Main1, TuningUtils::RAW_PORT_RRZO, filename);
            break;
            case BID_P2A_OUT_RECT_IN1:
            case BID_P2A_OUT_RECT_IN2:
            {
                ENUM_STEREO_SCENARIO eScenario = (pRequest->getRequestAttr().opState == eSTATE_CAPTURE)
                                                 ? eSTEREO_SCENARIO_CAPTURE
                                                 : (pRequest->getRequestAttr().opState == eSTATE_NORMAL)
                                                 ? eSTEREO_SCENARIO_PREVIEW
                                                 : eSTEREO_SCENARIO_RECORD;
                Pass2SizeInfo pass2info;
                ENUM_PASS2_ROUND p2Round = (BID_P2A_OUT_RECT_IN1 == BID) ? PASS2A_2 : PASS2A_P_2;
                StereoSizeProvider::getInstance()->getPass2SizeInfo(p2Round, eScenario, pass2info);
                mDumpHint_Main1.ImgWidth  = pass2info.areaWDMA.size.w;
                mDumpHint_Main1.ImgHeight = pass2info.areaWDMA.size.h;
                genFileName_YUV(writepath, PATH_SIZE, &mDumpHint_Main1, TuningUtils::YUV_PORT_UNDEFINED, filename);
            }
            break;
            case BID_P2A_IN_LCSO1:
            case BID_P2A_IN_LCSO2:
                genFileName_LCSO(writepath, PATH_SIZE, &mDumpHint_Main1, filename);
            break;
            case BID_GF_OUT_DMBG:
            case BID_DVS_OUT_UNPROCESS_DEPTH:
                if(config)
                    snprintf(filename+cx, PATH_SIZE-cx, "_delay_%d", config->mDepthDelay);
                genFileName_YUV(writepath, PATH_SIZE, &mDumpHint_Main1, TuningUtils::YUV_PORT_UNDEFINED, filename);
            break;
            // TODO : set correct dump filename
            case BID_P2A_IN_YUV1:
            case BID_P2A_IN_YUV2:
                genFileName_YUV(writepath, PATH_SIZE, &mDumpHint_Main1, TuningUtils::YUV_PORT_UNDEFINED, filename);
            break;
            default:
                genFileName_YUV(writepath, PATH_SIZE, &mDumpHint_Main1, TuningUtils::YUV_PORT_UNDEFINED, filename);
            break;
        }

        VSDOF_LOGD("saveToFile: %s", writepath);
        pImgBuf->saveToFile(writepath);
    }

    return MTRUE;
}

MBOOL
DepthMapPipeNode::
handleData(DataID id, DepthMapRequestPtr pReq)
{
#ifdef GTEST_PARTIAL
    return CamThreadNode<DepthMapDataHandler>::handleData(id, pReq);
#else
    MBOOL bConnect = mpFlowOption->checkConnected(id);
    if(bConnect)
    {
        CamThreadNode<DepthMapDataHandler>::handleData(id, pReq);
        return MTRUE;
    }
    return MFALSE;
#endif
}


MBOOL
DepthMapPipeNode::
handleDump(
    DataID id,
    DepthMapRequestPtr &request,
    DumpConfig* config
)
{
    return this->onDump(id, request, config);
}

MBOOL
DepthMapPipeNode::
handleDataAndDump(
    DataID id,
    DepthMapRequestPtr &request,
    DumpConfig* config)
{
    // dump first and then handle data
    MBOOL bRet = this->onDump(id, request, config);
    bRet &= this->handleData(id, request);
    return bRet;
}

MBOOL DepthMapPipeNode::checkToDump(DataID id, DepthMapRequestPtr pRequest)
{
#ifdef GTEST
    return MTRUE;
#endif
    MINT32 iNodeDump =  getPropValue();

    if(getPropValue() == 0)
    {
        VSDOF_LOGD("node check failed!node: %s dataID: %d", getName(), id);
        return MFALSE;
    }

    if(getPropValue(id) == 0)
    {
        VSDOF_LOGD("dataID check failed!dataID: %d  %s", id, ID2Name(id));
        return MFALSE;
    }
    MUINT iReqIdx = pRequest->getRequestNo();
    // only non-capture, check dump index
    if(!mbDumpCapture && !checkDumpIndex(iReqIdx))
    {
        MY_LOGD("%s onDump reqID=%d dataid=%d(%s) request not in range!",
                getName(), iReqIdx, id, ID2Name(id));
        return MFALSE;
    }
    else if(mbDumpCapture && pRequest->getRequestAttr().opState != eSTATE_CAPTURE)
    {
        MY_LOGD("%s onDump reqID=%d dataid=%d(%s), this req is not capture and only dump capture, return!",
                getName(), iReqIdx, id, ID2Name(id));
        return MFALSE;
    }

    return MTRUE;
}

MBOOL
DepthMapPipeNode::checkDumpIndex(MUINT iReqIdx)
{
    return (iReqIdx >= miDumpStartIdx && iReqIdx < miDumpStartIdx + miDumpBufSize);
}

const char*
DepthMapPipeNode::
onDumpBIDToName(DepthMapBufferID BID)
{
#define MAKE_NAME_CASE(name) \
    case name: return #name;
    switch(BID)
    {
        MAKE_NAME_CASE(BID_P2A_IN_FSRAW1);
        MAKE_NAME_CASE(BID_P2A_IN_FSRAW2);
        MAKE_NAME_CASE(BID_P2A_IN_RSRAW1);
        MAKE_NAME_CASE(BID_P2A_IN_RSRAW2);
        MAKE_NAME_CASE(BID_P2A_IN_LCSO1);
        MAKE_NAME_CASE(BID_P2A_IN_LCSO2);
        MAKE_NAME_CASE(BID_P2A_IN_YUV1);
        MAKE_NAME_CASE(BID_P2A_IN_YUV2);
        MAKE_NAME_CASE(BID_P1_OUT_RECT_IN1); //P1YUV
        MAKE_NAME_CASE(BID_P1_OUT_RECT_IN2); //P1YUV
        MAKE_NAME_CASE(BID_GF_IN_DEBUG);
        // static buffers
        MAKE_NAME_CASE(BID_P2A_WARPMTX_X_MAIN1);
        MAKE_NAME_CASE(BID_P2A_WARPMTX_Y_MAIN1);
        MAKE_NAME_CASE(BID_P2A_WARPMTX_Z_MAIN1);
        MAKE_NAME_CASE(BID_P2A_WARPMTX_X_MAIN2);
        MAKE_NAME_CASE(BID_P2A_WARPMTX_Y_MAIN2);
        MAKE_NAME_CASE(BID_P2A_WARPMTX_Z_MAIN2);
        MAKE_NAME_CASE(BID_N3D_OUT_WARPMTX_MAIN1_X);
        MAKE_NAME_CASE(BID_N3D_OUT_WARPMTX_MAIN1_Y);
        MAKE_NAME_CASE(BID_N3D_OUT_WARPMTX_MAIN1_Z);
        MAKE_NAME_CASE(BID_N3D_OUT_MASK_M_STATIC);
        // internal P2A buffers
        MAKE_NAME_CASE(BID_P2A_FE1B_INPUT);
        MAKE_NAME_CASE(BID_P2A_FE1B_INPUT_NONSLANT);
        MAKE_NAME_CASE(BID_P2A_FE2B_INPUT);
        MAKE_NAME_CASE(BID_P2A_FE1C_INPUT);
        MAKE_NAME_CASE(BID_P2A_FE2C_INPUT);
        // P2A output
        MAKE_NAME_CASE(BID_P2A_OUT_FDIMG);
        MAKE_NAME_CASE(BID_P2A_OUT_FE1AO);
        MAKE_NAME_CASE(BID_P2A_OUT_FE2AO);
        MAKE_NAME_CASE(BID_P2A_OUT_FE1BO);
        MAKE_NAME_CASE(BID_P2A_OUT_FE2BO);
        MAKE_NAME_CASE(BID_P2A_OUT_FE1CO);
        MAKE_NAME_CASE(BID_P2A_OUT_FE2CO);
        MAKE_NAME_CASE(BID_P2A_OUT_RECT_IN1);
        MAKE_NAME_CASE(BID_P2A_OUT_RECT_IN2);
        MAKE_NAME_CASE(BID_P2A_OUT_MV_F);
        MAKE_NAME_CASE(BID_P2A_OUT_MV_F_CAP);
        MAKE_NAME_CASE(BID_P2A_OUT_FMAO_LR);
        MAKE_NAME_CASE(BID_P2A_OUT_FMAO_RL);
        MAKE_NAME_CASE(BID_P2A_OUT_FMBO_LR);
        MAKE_NAME_CASE(BID_P2A_OUT_FMBO_RL);
        MAKE_NAME_CASE(BID_P2A_OUT_FMCO_LR);
        MAKE_NAME_CASE(BID_P2A_OUT_FMCO_RL);
        MAKE_NAME_CASE(BID_P2A_OUT_MY_S);
        MAKE_NAME_CASE(BID_P2A_OUT_POSTVIEW);
        MAKE_NAME_CASE(BID_P2A_INTERNAL_IMG3O);
        MAKE_NAME_CASE(BID_P2A_TUNING);
        MAKE_NAME_CASE(BID_P2A_INTERNAL_FD);
        MAKE_NAME_CASE(BID_P2A_INTERNAL_IN_YUV1);
        MAKE_NAME_CASE(BID_P2A_INTERNAL_IN_YUV2);
        MAKE_NAME_CASE(BID_P2A_INTERNAL_IN_RRZ2);
        // N3D output
        MAKE_NAME_CASE(BID_N3D_OUT_MV_Y);
        MAKE_NAME_CASE(BID_N3D_OUT_MASK_M);
        MAKE_NAME_CASE(BID_N3D_OUT_WARPMTX_MAIN2_X);
        MAKE_NAME_CASE(BID_N3D_OUT_WARPMTX_MAIN2_Y);
        //
        MAKE_NAME_CASE(BID_WPE_OUT_SV_Y);
        MAKE_NAME_CASE(BID_WPE_OUT_MASK_S);
        // WPE
        MAKE_NAME_CASE(BID_WPE_IN_MASK_S);
        // DPE section
        MAKE_NAME_CASE(BID_DVS_OUT_DV_LR);
        MAKE_NAME_CASE(BID_OCC_OUT_CFM_M);
        MAKE_NAME_CASE(BID_OCC_OUT_CFM_M_NONSLANT);
        MAKE_NAME_CASE(BID_OCC_OUT_NOC_M);
        MAKE_NAME_CASE(BID_OCC_OUT_NOC_M_NONSLANT);
        MAKE_NAME_CASE(BID_ASF_OUT_CRM);
        MAKE_NAME_CASE(BID_ASF_OUT_RD);
        MAKE_NAME_CASE(BID_ASF_OUT_HF);
        MAKE_NAME_CASE(BID_WMF_OUT_DMW);
        // GF
        MAKE_NAME_CASE(BID_GF_INTERNAL_DEPTHMAP);
        MAKE_NAME_CASE(BID_GF_INTERNAL_DMBG);
        MAKE_NAME_CASE(BID_GF_OUT_DMBG);
        MAKE_NAME_CASE(BID_DLDEPH_DVS_OUT_DEPTH);
        MAKE_NAME_CASE(BID_DLDEPTH_INTERNAL_DEPTHMAP);

        #ifdef GTEST
        // UT output
        MAKE_NAME_CASE(BID_FE2_HWIN_MAIN1);
        MAKE_NAME_CASE(BID_FE2_HWIN_MAIN2);
        MAKE_NAME_CASE(BID_FE3_HWIN_MAIN1);
        MAKE_NAME_CASE(BID_FE3_HWIN_MAIN2);
        #endif

        MAKE_NAME_CASE(BID_META_IN_APP);
        MAKE_NAME_CASE(BID_META_IN_HAL_MAIN1);
        MAKE_NAME_CASE(BID_META_IN_HAL_MAIN2);
        MAKE_NAME_CASE(BID_META_OUT_APP);
        MAKE_NAME_CASE(BID_META_OUT_HAL);
        MAKE_NAME_CASE(BID_META_IN_APP_QUEUED);
        MAKE_NAME_CASE(BID_META_IN_HAL_MAIN1_QUEUED);
        MAKE_NAME_CASE(BID_META_IN_HAL_MAIN2_QUEUED);
        MAKE_NAME_CASE(BID_META_OUT_APP_QUEUED);
        MAKE_NAME_CASE(BID_META_OUT_HAL_QUEUED);
        MAKE_NAME_CASE(BID_PQ_PARAM);
        MAKE_NAME_CASE(BID_DP_PQ_PARAM);
        MAKE_NAME_CASE(BID_DEFAULT_WARPMAP_X);
        MAKE_NAME_CASE(BID_DEFAULT_WARPMAP_Y);

        default:
            MY_LOGW("unknown BID:%d", BID);
            return "unknown";
    }

#undef MAKE_NAME_CASE
}

DepthMapDataID
DepthMapPipeNode::
mapNDDL1DataID(DataID id)
{
    switch(id)
    {
        case TO_DUMP_YUVS :
            return TO_DUMP_YUVS_NDD_L1;
        case P2A_TO_N3D_FEOFMO :
            return P2A_TO_N3D_FEOFMO_NDD_L1;
        case DVP_TO_GF_DMW_N_DEPTH :
            return DVP_TO_GF_DMW_N_DEPTH_NDD_L1;
        default:
            break;
    }
    return id;
}

DepthMapBufferID
DepthMapPipeNode::
mapDebugInkBufferID(DepthMapRequestPtr pRequest, DepthDebugImg id)
{
    switch(id)
    {
        case eDEPTH_DEBUG_IMG_NONE :
            return BID_INVALID;
        case eDEPTH_DEBUG_IMG_BLUR_MAP :
            return (pRequest->isQueuedDepthRequest(mpPipeOption))
                    ? GF_OUT_INTERNAL_DMBG
                    : GF_OUT_DMBG;
        case eDEPTH_DEBUG_IMG_DISPARITY :
            return (pRequest->getRequestAttr().opState == eSTATE_NORMAL)
                    ? BID_WMF_OUT_DMW
                    : BID_DLDEPTH_INTERNAL_DEPTHMAP;
        case eDEPTH_DEBUG_IMG_CONFIDENCE :
            return BID_OCC_OUT_CFM_M;
        default :
            break;
    }
    return BID_INVALID;
}

MBOOL
DepthMapPipeNode::
mapOutputOnFlowType(
    DepthMapRequestPtr pRequest,
    DepthMapDataID *directDataId,
    DepthMapDataID *queuedDataId,
    DepthMapBufferID *Bid
)
{
    DepthMapPipeNodeID nodeId = getNodeId();
    if ( nodeId == eDPETHMAP_PIPE_NODEID_DVS ) {
        *directDataId = DVS_OUT_DEPTHMAP;
        *queuedDataId = DVS_OUT_INTERNAL_DEPTH;
        *Bid = BID_OCC_OUT_NOC_M;
        return MTRUE;
    }
    else if ( nodeId == eDPETHMAP_PIPE_NODEID_DVP ) {
        *directDataId = DVP_OUT_DEPTHMAP;
        *queuedDataId = DVP_OUT_INTERNAL_DEPTH;
        if ( pRequest->mbWMF_ON )
            *Bid = BID_WMF_OUT_DMW;
        else
            *Bid = BID_ASF_OUT_HF;
        return MTRUE;
    }
    else if ( nodeId == eDPETHMAP_PIPE_NODEID_DLDEPTH ) {
        *directDataId = DLDEPTH_OUT_DEPTHMAP;
        *queuedDataId = DLDEPTH_OUT_INTERNAL_DEPTH;
        *Bid = BID_DLDEPTH_INTERNAL_DEPTHMAP;
        return MTRUE;
    }
    else if ( nodeId == eDPETHMAP_PIPE_NODEID_GF ) {
        *directDataId = GF_OUT_DEPTHMAP;
        *queuedDataId = GF_OUT_INTERNAL_DEPTH;
        *Bid = BID_GF_INTERNAL_DEPTHMAP;
        return MTRUE;
    }
    else if (nodeId == eDPETHMAP_PIPE_NODEID_SLANT ) {
        *directDataId = SLANT_OUT_DEPTHMAP;
        *queuedDataId = SLANT_OUT_INTERNAL_DEPTH;
        if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_UNPROCESS_DEPTH ) {
            *Bid = BID_OCC_OUT_NOC_M_NONSLANT;
            return MTRUE;
        }
        else if ( mpPipeOption->mFeatureMode == eDEPTHNODE_MODE_MTK_AI_DEPTH ) {
            *Bid = BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT;
            return MTRUE;
        }
    }
    //
    MY_LOGE("invalid output node, %d", (int)nodeId);
    return MFALSE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
DepthMapPipeNode::
onHandle3rdPartyFlowDone(
    DepthMapRequestPtr pRequest
)
{
    DumpConfig config;
    MUINT32 iReqIdx = pRequest->getRequestNo();
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    DepthBufferInfo depthInfo;
    DepthMapDataID directDataId = ID_INVALID, queuedDataId = ID_INVALID;
    DepthMapBufferID BID = 0;
    mapOutputOnFlowType(pRequest, &directDataId, &queuedDataId, &BID);
    // create depthInfo
    MBOOL bRet = pRequest->getBufferHandler()->getEnquedSmartBuffer(
                                    getNodeId(), BID, depthInfo.mpDepthBuffer);
    if(bRet)
    {
        // flush depthmap
        depthInfo.mpDepthBuffer->mImageBuffer->syncCache(eCACHECTRL_INVALID);
        depthInfo.miReqIdx = iReqIdx;
    }
    else
    {
        MY_LOGE("Cannot find the internal/request depthMap buffer! BID=%d", BID);
        return MFALSE;
    }
    // Case 1 : Queued flow or depth delay is set more than 0, stored the depthInfo
    if(pRequest->isQueuedDepthRequest(mpPipeOption))
    {
        if(miDepthDelay)
            mpDepthStorage->push_back(depthInfo);
        else
            mpDepthStorage->setStoredData(depthInfo);
        // dump
        handleDump(queuedDataId, pRequest);
        // notify queue flow done
        handleData(QUEUED_FLOW_DONE, pRequest);
    }
    // Case 2 : Not queued flow, directly output Depth (NOC)
    else
    {
        if(!_copyBufferIntoRequestWithCrop(depthInfo.mpDepthBuffer->mImageBuffer.get(),
                                    pRequest, BID_DVS_OUT_UNPROCESS_DEPTH))
        {
            AEE_ASSERT("NodeID=%d Failed to copy BID_DVS_OUT_UNPROCESS_DEPTH", (int)getNodeId());
            return MFALSE;
        }
        config.mDepthDelay = miDepthDelay;
        mpDepthStorage->setLatestData(depthInfo);
        // notify DMBG done
        handleDataAndDump(directDataId, pRequest, &config);
    }
    return bRet;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
DepthMapPipeNode::
_copyBufferIntoRequest(
    IImageBuffer* pSrcBuffer,
    sp<DepthMapEffectRequest> pRequest,
    DepthMapBufferID bufferID
)
{
    VSDOF_LOGD("reqID=%d copy bufferID:%d (%s) into request!",
                pRequest->getRequestNo(), bufferID, onDumpBIDToName(bufferID));
    if(pRequest->isRequestBuffer(bufferID))
    {
        IImageBuffer* pImgBuf = pRequest->getBufferHandler()->requestBuffer(getNodeId(), bufferID);
        if(pImgBuf->getBufSizeInBytes(0) != pSrcBuffer->getBufSizeInBytes(0))
        {
            MY_LOGE("buffer size(id:%d %s) is not consistent!request buffer size=%dx%d(%lu bytes) src_buffer size=%dx%d(%lu bytes)",
                        bufferID, onDumpBIDToName(bufferID),
                        pImgBuf->getImgSize().w, pImgBuf->getImgSize().h, pImgBuf->getBufSizeInBytes(0),
                        pSrcBuffer->getImgSize().w, pSrcBuffer->getImgSize().h, pSrcBuffer->getBufSizeInBytes(0));
            return MFALSE;
        }
        memcpy( (void*)pImgBuf->getBufVA(0),
                (void*)pSrcBuffer->getBufVA(0),
                pImgBuf->getBufSizeInBytes(0));
        pImgBuf->syncCache(eCACHECTRL_FLUSH);
        // mark ready
        pRequest->setOutputBufferReady(bufferID);
    }
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
DepthMapPipeNode::
_copyBufferIntoRequestWithCrop(
    IImageBuffer* pSrcBuffer,
    sp<DepthMapEffectRequest> pRequest,
    DepthMapBufferID bufferID
)
{
    VSDOF_LOGD("reqID=%d copy bufferID:%d (%s) into request!",
                pRequest->getRequestNo(), bufferID, onDumpBIDToName(bufferID));
    if(pRequest->isRequestBuffer(bufferID))
    {
        IImageBuffer* pImgBuf = pRequest->getBufferHandler()->requestBuffer(getNodeId(), bufferID);
        MUINT8 *pSrc = (MUINT8*)pSrcBuffer->getBufVA(0);
        MSize srcSize = pSrcBuffer->getImgSize();
        MUINT8 *pDst = (MUINT8*)pImgBuf->getBufVA(0);
        MSize dstSize = pImgBuf->getImgSize();

        if(srcSize.w == dstSize.w &&
           srcSize.h == dstSize.h)
        {
            ::memcpy(pDst, pSrc, dstSize.w*dstSize.h);
        }
        else if(srcSize.w >= dstSize.w &&
                srcSize.h >= dstSize.h)
        {
            for(int row = 0; row < dstSize.h; ++row)
            {
                ::memcpy(pDst, pSrc, dstSize.w);
                pDst += dstSize.w;
                pSrc += srcSize.w;
            }
        }
        else
        {
            if ( srcSize.w == dstSize.h || srcSize.h == dstSize.w) {
                MUINT32 rot = 360 - (MUINT32)StereoSettingProvider::getModuleRotation();
                ENUM_ROTATION targetRotation = eRotate_0;
                if ( rot == 90) targetRotation = eRotate_90;
                else if ( rot == 180) targetRotation = eRotate_180;
                else if ( rot == 270) targetRotation = eRotate_270;
                _copyBufferIntoRequestWithCropAndRotate(pSrcBuffer, pRequest, bufferID, targetRotation);
            }
            else {
                MY_LOGE("Dst size(%dx%d) > src size(%dx%d)", dstSize.w, dstSize.h, srcSize.w, srcSize.h);
            }
        }

        pImgBuf->syncCache(eCACHECTRL_FLUSH);
        // mark ready
        pRequest->setOutputBufferReady(bufferID);
    }
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
DepthMapPipeNode::
_copyBufferIntoRequestWithCropAndRotate(
    IImageBuffer* pSrcBuffer,
    sp<DepthMapEffectRequest> pRequest,
    DepthMapBufferID bufferID,
    ENUM_ROTATION targetRotation
)
{
    if(pRequest->isRequestBuffer(bufferID))
    {
        IImageBuffer* pImgBuf = pRequest->getBufferHandler()->requestBuffer(getNodeId(), bufferID);
        MUINT8 *pSrc = (MUINT8*)pSrcBuffer->getBufVA(0);
        MSize srcSize = pSrcBuffer->getImgSize();
        MUINT8 *pDst = (MUINT8*)pImgBuf->getBufVA(0);
        MSize dstSize = pImgBuf->getImgSize();

        MUINT8 padding = 0;
        MINT32 writeRow = 0;
        MINT32 writeCol = 0;
        MINT32 writePos = 0;
        const MINT32 BUFFER_LEN = srcSize.w * srcSize.h;

        if(eRotate_180 == targetRotation) {
            padding = srcSize.w - dstSize.w;
            writeRow = 0;
            writeCol = 0;
            writePos = 0;
            for(int i = BUFFER_LEN-1; i >= 0; --i) {
                if ( srcSize.w - (i % srcSize.w) <= padding ) { // crop
                    continue;
                }
                *(pDst + writePos) = *(pSrc + i);

                ++writePos;
                ++writeCol;
                if(writeCol >= dstSize.w) {
                    ++writeRow;
                    writeCol = 0;
                    writePos = dstSize.w * writeRow;
                }
            }
        }
        // 270
        else if(eRotate_270 == targetRotation) {
            if ( srcSize.w > dstSize.h)
                padding = srcSize.w - dstSize.h;
            else
                padding = srcSize.h - dstSize.w;
            writeRow = 0;
            writeCol = dstSize.w - 1;
            writePos = writeCol;
            for(int i = BUFFER_LEN-1; i >= 0; --i) {
                if ( srcSize.w - (i % srcSize.w) <= padding ) { // crop
                    continue;
                }
                *(pDst + writePos) = *(pSrc + i);

                writePos += dstSize.w;
                ++writeRow;
                if(writeRow >= dstSize.h) {
                    writeRow = 0;
                    --writeCol;
                    writePos = writeCol;
                }
            }
        }
        // 90
        else if(eRotate_90 == targetRotation) {
            if ( srcSize.w > dstSize.h)
                padding = srcSize.w - dstSize.h;
            else
                padding = srcSize.h - dstSize.w;
            writeRow = dstSize.h - 1;
            writeCol = 0;
            writePos = dstSize.h * dstSize.w - dstSize.w;
            for(int i = BUFFER_LEN-1; i >= 0; --i) {
                if ( srcSize.w - (i % srcSize.w) <= padding ) { // crop
                    continue;
                }
                *(pDst + writePos) = *(pSrc + i);

                writePos -= dstSize.w;
                --writeRow;
                if(writeRow < 0) {
                    writeRow = dstSize.h - 1;
                    ++writeCol;
                    writePos = dstSize.h * dstSize.w - ( dstSize.w - writeCol);
                }
            }
        }
    }
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
DepthMapPipeNode::
copyBuffer(
    IImageBuffer* pSrcBuffer,
    IImageBuffer* pDstBuffer
)
{
    if(pDstBuffer->getPlaneCount() != pSrcBuffer->getPlaneCount())
    {
        MY_LOGE("buffer plane count is not consistent! dst_buffer:%lu, src_buffer:%lu",
                pDstBuffer->getPlaneCount(), pSrcBuffer->getPlaneCount());
        return MFALSE;
    }
    if(pDstBuffer->getBufSizeInBytes(0) != pSrcBuffer->getBufSizeInBytes(0))
    {
        MY_LOGE("buffer size is not consistent! dst_buffer size=%dx%d(%lu bytes) src_buffer size=%dx%d(%lu bytes)",
                    pDstBuffer->getImgSize().w, pDstBuffer->getImgSize().h, pDstBuffer->getBufSizeInBytes(0),
                    pSrcBuffer->getImgSize().w, pSrcBuffer->getImgSize().h, pSrcBuffer->getBufSizeInBytes(0));
        return MFALSE;
    }
    memcpy( (void*)pDstBuffer->getBufVA(0),
            (void*)pSrcBuffer->getBufVA(0),
            pDstBuffer->getBufSizeInBytes(0));
    pDstBuffer->syncCache(eCACHECTRL_FLUSH);
    return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL
DepthMapPipeNode::
copyBufferWithCrop(
    IImageBuffer* pSrcBuffer,
    IImageBuffer* pDstBuffer
)
{
    MUINT8 *pSrc = (MUINT8*)pSrcBuffer->getBufVA(0);
    MSize srcSize = pSrcBuffer->getImgSize();
    MUINT8 *pDst = (MUINT8*)pDstBuffer->getBufVA(0);
    MSize dstSize = pDstBuffer->getImgSize();

    if(srcSize.w == dstSize.w &&
        srcSize.h == dstSize.h)
    {
        ::memcpy(pDst, pSrc, dstSize.w*dstSize.h);
    }
    else if(srcSize.w >= dstSize.w &&
            srcSize.h >= dstSize.h)
    {
        for(int row = 0; row < dstSize.h; ++row)
        {
            ::memcpy(pDst, pSrc, dstSize.w);
            pDst += dstSize.w;
            pSrc += srcSize.w;
        }
    }
    else
    {
        MY_LOGE("Dst size(%dx%d) > src size(%dx%d)", dstSize.w, dstSize.h, srcSize.w, srcSize.h);
    }
    pDstBuffer->syncCache(eCACHECTRL_FLUSH);
    return MTRUE;
}


}; // namespace NSFeaturePipe_DepthMap
}; // namespace NSCamFeature
}; // namespace NSCam
