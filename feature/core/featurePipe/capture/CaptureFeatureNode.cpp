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

#include "CaptureFeatureNode.h"
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

#define PIPE_CLASS_TAG "Node"
#define PIPE_TRACE TRACE_CAPTURE_FEATURE_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam3/feature/utils/p2/P2Util.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_CAPTURE);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

const char* CaptureFeatureDataHandler::ID2Name(DataID id)
{
    return PathID2Name(id);
}

CaptureFeatureDataHandler::~CaptureFeatureDataHandler()
{
}

CaptureFeatureNode::CaptureFeatureNode(NodeID_T nid, const char *name, MUINT32 uLogLevel, MINT32 policy, MINT32 priority)
    : CamThreadNode(name, policy, priority)
    , mSensorIndex(-1)
    , mSensorIndex2(-1)
    , mNodeId(nid)
    , mLogLevel(uLogLevel)
{
    mbDumpCamPipe = ::property_get_int32(Feature::P2Util::DUMP_CAM_PIPE_PROPERTIES, 0);
}

CaptureFeatureNode::~CaptureFeatureNode()
{
}

MBOOL CaptureFeatureNode::onInit()
{
    return MTRUE;
}

MVOID CaptureFeatureNode::setSensorIndex(MINT32 sensorIndex, MINT32 sensorIndex2)
{
    mSensorIndex = sensorIndex;
    mSensorIndex2 = sensorIndex2;
}

MVOID CaptureFeatureNode::setSensorList(const std::vector<MINT32> sensorList)
{
    mSensorList.assign(sensorList.begin(), sensorList.end());
}


MVOID CaptureFeatureNode::setLogLevel(MUINT32 uLogLevel)
{
    mLogLevel = uLogLevel;
}

MVOID CaptureFeatureNode::setCropCalculator(const android::sp<CropCalculator> &rCropCalculator)
{
    mpCropCalculator = rCropCalculator;
}

MVOID CaptureFeatureNode::setFovCalculator(const android::sp<FovCalculator> &rFovCalculator)
{
    mpFOVCalculator = rFovCalculator;
}

MVOID CaptureFeatureNode::setUsageHint(const ICaptureFeaturePipe::UsageHint &rUsageHint)
{
    mUsageHint = rUsageHint;
}

MVOID CaptureFeatureNode::setTaskQueue(const android::sp<CaptureTaskQueue>& pTaskQueue)
{
    mpTaskQueue = pTaskQueue;
}

MVOID CaptureFeatureNode::dispatch(const RequestPtr &pRequest, NodeID_T nodeId)
{
    if (nodeId == NULL_NODE)
        nodeId = getNodeID();

    Vector<NodeID_T> vNextNodes = pRequest->getNextNodes(nodeId);

    pRequest->stopTimer(nodeId);
    markNodeExit(pRequest);
    pRequest->lock();
    pRequest->finishNode_Locked(nodeId);
    for (NodeID_T nextNode : vNextNodes)
    {
        MY_LOGD_IF(mLogLevel, "find path=%s  %s", NodeID2Name(nodeId), NodeID2Name(nextNode));
        PathID_T pathId = FindPath(nodeId, nextNode);
        if (pathId != NULL_PATH) {
            pRequest->startTimer(nextNode);
            markNodeEnter(pRequest);
            pRequest->finishPath_Locked(pathId);
            handleData(pathId, pRequest);
            MY_LOGD_IF(mLogLevel, "Goto %s", PathID2Name(pathId));
        }
    }

    if (vNextNodes.size() == 0 && pRequest->isFinished_Locked())
    {
        handleData(PID_DEQUE, pRequest);
        pRequest->unlock();
        return;
    }
    pRequest->unlock();
}

template <typename T, typename ...Ts>
MBOOL CaptureFeatureNode::dumpData(RequestPtr &request, IImageBuffer *buffer, T fmt, Ts... args)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( buffer && fmt )
    {
        char name[256];

        if (snprintf(name, 256, fmt, args...) < 0) {
            MY_LOGW("create filename fails");
            strncpy(name, "NA", sizeof(name));
            name[sizeof(name)-1] = 0;
        }

        ret = dumpNamedData(request, buffer, name);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL CaptureFeatureNode::dumpNamedData(RequestPtr &request, IImageBuffer *buffer, const char *name)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( buffer && name )
    {
        MUINT32 stride, pbpp, ibpp, width, height, size;
        stride = buffer->getBufStridesInBytes(0);
        pbpp = buffer->getPlaneBitsPerPixel(0);
        ibpp = buffer->getImgBitsPerPixel();
        size = buffer->getBufSizeInBytes(0);
        pbpp = pbpp ? pbpp : 8;
        width = stride * 8 / pbpp;
        width = width ? width : 1;
        ibpp = ibpp ? ibpp : 8;
        height = size / width;
        if( buffer->getPlaneCount() == 1 )
        {
          height = height * 8 / ibpp;
        }

        char path[256];
        if(snprintf(path, sizeof(path), "/sdcard/dump/%04d_%s_%dx%d.bin", request->getRequestNo(), name, width, height) < 0)
            MY_LOGW("snprintf[/sdcard/dump/%04d_%s_%dx%d.bin] failed", request->getRequestNo(), name, width, height);

        TRACE_FUNC("dump to %s", path);
        buffer->saveToFile(path);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MUINT32 CaptureFeatureNode::dumpData(const char *buffer, MUINT32 size, const char *filename)
{
    uint32_t writeCount = 0;
    int fd = ::open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if( fd < 0 )
    {
        MY_LOGE("Cannot create file [%s]", filename);
    }
    else
    {
        for( int cnt = 0, nw = 0; writeCount < size; ++cnt )
        {
            nw = ::write(fd, buffer + writeCount, size - writeCount);
            if( nw < 0 )
            {
                MY_LOGE("Cannot write to file [%s]", filename);
                break;
            }
            writeCount += nw;
        }
        ::close(fd);
    }
    return writeCount;
}

MBOOL CaptureFeatureNode::loadData(IImageBuffer *buffer, const char *filename)
{
    MBOOL ret = MFALSE;
    if( buffer )
    {
        loadData((char*)buffer->getBufVA(0), 0, filename);
        ret = MTRUE;
    }
    return MFALSE;
}

MUINT32 CaptureFeatureNode::loadData(char *buffer, size_t size, const char *filename)
{
    uint32_t readCount = 0;
    int fd = ::open(filename, O_RDONLY);
    if( fd < 0 )
    {
        MY_LOGE("Cannot open file [%s]", filename);
    }
    else
    {
        if( size == 0 )
        {
            off_t readSize = ::lseek(fd, 0, SEEK_END);
            size = (readSize < 0) ? 0 : readSize;
            ::lseek(fd, 0, SEEK_SET);
        }
        for( int cnt = 0, nr = 0; readCount < size; ++cnt )
        {
            nr = ::read(fd, buffer + readCount, size - readCount);
            if( nr < 0 )
            {
                MY_LOGE("Cannot read from file [%s]", filename);
                break;
            }
            readCount += nr;
        }
        ::close(fd);
    }
    return readCount;
}

MBOOL CaptureFeatureNode::needToDump(const RequestPtr pRequest)
{
    if(mbDumpCamPipe){
        MY_LOGD_IF(mLogLevel, "mbDumpCamPipe is on, force to dump");
        return MTRUE;
    }
    if(pRequest->hasParameter(PID_NEED_NDD)){
        MY_LOGD_IF(mLogLevel, "find PID_NEED_NDD parameter, mIsHidlIsp: %d", mUsageHint.mIsHidlIsp);
        return MTRUE;
    }
    else{
        MY_LOGD_IF(mLogLevel, "no PID_NEED_NDD parameter, mIsHidlIsp: %d", mUsageHint.mIsHidlIsp);
        return MFALSE;
    }
}

MVOID CaptureFeatureNode::markNodeEnter(const RequestPtr &request) const
{
    CAM_ULOG_ENTER(this, Utils::ULog::REQ_STR_FPIPE_REQUEST, request->getRequestNo());
}

MVOID CaptureFeatureNode::markNodeExit(const RequestPtr &request) const
{
    CAM_ULOG_EXIT(this, Utils::ULog::REQ_STR_FPIPE_REQUEST, request->getRequestNo());
}

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
