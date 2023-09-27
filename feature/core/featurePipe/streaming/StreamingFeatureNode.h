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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_NODE_H_

#include <featurePipe/core/include/CamThreadNode.h>
#include <featurePipe/core/include/SeqUtil.h>
#include <featurePipe/core/include/DebugUtil.h>
#include "StreamingFeature_Common.h"
#include "StreamingFeatureData.h"
#include "StreamingFeaturePipeUsage.h"
#include "MtkHeader.h"

#include <mutex>
#include <condition_variable>
#include <utils/RefBase.h>

#define DISPATCH_KEY_P2 "Camera_Driver_P2"
#define DISPATCH_KEY_MSS "Camera_Driver_MSS"
#define DISPATCH_KEY_RSC "Camera_Driver_RSC"
#define DISPATCH_KEY_WPE "Camera_Driver_WPE"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
enum StreamingFeatureDataID {
    ID_INVALID,
    ID_ROOT_ENQUE,
    ID_ROOT_TO_P2G,
    ID_ROOT_TO_RSC,
    ID_ROOT_TO_DEPTH,
    ID_ROOT_TO_TOF,
    ID_ROOT_TO_EIS,
    ID_ROOT_TO_TRACKING,
    ID_P2G_TO_P2SW,
    ID_P2G_TO_PMSS,
    ID_P2G_TO_MSS,
    ID_P2G_TO_MSF,
    ID_P2G_TO_WARP_P,
    ID_P2G_TO_WARP_R,
    ID_P2G_TO_EIS_P2DONE,
    ID_P2G_TO_VNR,
    ID_P2G_TO_PMDP,
    ID_P2G_TO_HELPER,
    ID_P2G_TO_BOKEH,
    ID_P2G_TO_XNODE,
    ID_VNR_TO_NEXT_FULLIMG,
    ID_VNR_TO_NEXT_P,
    ID_VNR_TO_NEXT_R,
    ID_DEPTH_TO_HELPER,
    ID_BOKEH_TO_HELPER,
    ID_WARP_TO_HELPER,
    ID_ASYNC_TO_HELPER,
    ID_DISP_TO_HELPER,
    ID_HELPER_TO_ASYNC,
    ID_MSS_TO_MSF,
    ID_PMSS_TO_MSF,
    ID_P2SW_TO_MSF,
    ID_EIS_TO_WARP_P,
    ID_EIS_TO_WARP_R,
    ID_P2G_TO_VENDOR_FULLIMG,
    ID_TRACKING_TO_HELPER,
    ID_BOKEH_TO_VENDOR_FULLIMG,
    ID_BOKEH_TO_WARP_P_FULLIMG,
    ID_BOKEH_TO_WARP_R_FULLIMG,
    ID_BOKEH_TO_XNODE,
    ID_TPI_TO_NEXT,
    ID_TPI_TO_NEXT_P,
    ID_TPI_TO_NEXT_R,
    ID_VMDP_TO_NEXT,
    ID_VMDP_TO_NEXT_P,
    ID_VMDP_TO_NEXT_R,
    ID_VMDP_TO_XNODE,
    ID_VMDP_TO_HELPER,
    ID_RSC_TO_HELPER,
    ID_RSC_TO_EIS,
    ID_XNODE_TO_HELPER,
    ID_DEPTH_TO_BOKEH,
    ID_DEPTH_P2_TO_BOKEH,
    ID_DEPTH_TO_VENDOR,
    ID_DEPTH_P2_TO_VENDOR,
    ID_TOF_TO_NEXT,
    ID_RSC_TO_P2G,
};

struct DumpFilter
{
    MUINT32 index = 0;
    const char *name = NULL;
    MBOOL defaultDump = MFALSE;
    DumpFilter(MUINT32 _index, const char *_name, MBOOL _defaultDump)
    : index(_index)
    , name(_name)
    , defaultDump(_defaultDump)
    {}
    DumpFilter() {}
};

class NodeSignal : public virtual android::RefBase
{
public:
    enum Signal
    {
        SIGNAL_GPU_READY = 0x01 << 0,
    };

    enum Status
    {
        STATUS_IN_FLUSH = 0x01 << 0,
    };

    NodeSignal();
    virtual ~NodeSignal();
    MVOID setSignal(Signal signal);
    MVOID clearSignal(Signal signal);
    MBOOL getSignal(Signal signal);
    MVOID waitSignal(Signal signal);

    MVOID setStatus(Status status);
    MVOID clearStatus(Status status);
    MBOOL getStatus(Status status);
    MUINT32 getIdleTimeMS();
    MVOID notifyEnque();

private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    MUINT32 mSignal;
    MUINT32 mStatus;
    timespec mLastEnqueTime;
};

class StreamingFeatureDataHandler : virtual public CamNodeULogHandler
{
public:
    typedef StreamingFeatureDataID DataID;
public:
    virtual ~StreamingFeatureDataHandler();
    virtual MBOOL onData(DataID, const RequestPtr&)     { return MFALSE; }
    virtual MBOOL onData(DataID, const ImgBufferData&)  { return MFALSE; }
    virtual MBOOL onData(DataID, const EisConfigData&)  { return MFALSE; }
    virtual MBOOL onData(DataID, const CBMsgData&)      { return MFALSE; }
    virtual MBOOL onData(DataID, const HelperData&)     { return MFALSE; }
    virtual MBOOL onData(DataID, const RSCData&)        { return MFALSE; }
    virtual MBOOL onData(DataID, const DSDNData&)       { return MFALSE; }
    virtual MBOOL onData(DataID, const BasicImgData&)   { return MFALSE; }
    virtual MBOOL onData(DataID, const DepthImgData&)       { return MFALSE; }
    virtual MBOOL onData(DataID, const DualBasicImgData&)    { return MFALSE; }
    virtual MBOOL onData(DataID, const PMDPReqData&)    { return MFALSE; }
    virtual MBOOL onData(DataID, const TPIData&)        { return MFALSE; }
    virtual MBOOL onData(DataID, const VMDPReqData&)    { return MFALSE; }
    virtual MBOOL onData(DataID, const WarpData&)       { return MFALSE; }
    virtual MBOOL onData(DataID, const MSSData&)        { return MFALSE; }
    virtual MBOOL onData(DataID, const P2GSWData&)      { return MFALSE; }
    virtual MBOOL onData(DataID, const P2GHWData&)      { return MFALSE; }
    virtual MBOOL onData(DataID, const NextData&)       { return MFALSE; }
    virtual MBOOL onData(DataID, const TrackingData&)   { return MFALSE; }

    static const char* ID2Name(DataID id);

    template<typename T>
    static unsigned getSeq(const T &data)
    {
        return data.mRequest->mRequestNo;
    }
    static unsigned getSeq(const RequestPtr &data)
    {
        return data->mRequestNo;
    }

    static const bool supportSeq = true;
};

class StreamingFeatureNode : public StreamingFeatureDataHandler, public CamThreadNode<StreamingFeatureDataHandler>
{
public:
    typedef CamGraph<StreamingFeatureNode> Graph_T;
    typedef StreamingFeatureDataHandler Handler_T;

public:
    StreamingFeatureNode(const char *name);
    virtual ~StreamingFeatureNode();
    MVOID setSensorIndex(MUINT32 sensorIndex);
    MVOID setPipeUsage(const StreamingFeaturePipeUsage &usage);
    MVOID setNodeSignal(const android::sp<NodeSignal> &nodeSignal);

    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo&) const;

protected:

    virtual MBOOL onInit();
    virtual MBOOL onUninit()         { return MTRUE; }
    virtual MBOOL onThreadStart()    { return MTRUE; }
    virtual MBOOL onThreadStop()     { return MTRUE; }
    virtual MBOOL onThreadLoop() = 0;
    typedef const char* (*MaskFunc)(MUINT32);
    MVOID enableDumpMask(const std::vector<DumpFilter> &vFilter, const char *postFix = NULL);
    MBOOL allowDump(MUINT8 maskIndex) const;


    static MBOOL dumpNddData(const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const BasicImg &img, const TuningUtils::YUV_PORT &portIndex, const char *pUserStr = NULL);
    static MBOOL dumpNddData(const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const ImgBuffer &img, const TuningUtils::YUV_PORT &portIndex, const char *pUserStr = NULL);
    static MBOOL dumpNddData(TuningUtils::FILE_DUMP_NAMING_HINT hint, IImageBuffer *buffer, const TuningUtils::YUV_PORT &portIndex, const char *pUserStr = NULL);

    MVOID drawScanLine(IImageBuffer *buffer);

    template <typename... Args>
    static MBOOL dumpData(const RequestPtr &request, const ImgBuffer &buffer, const char *fmt, Args... args)
    {
        return (buffer != NULL) && dumpNamedData(request, buffer->getImageBufferPtr(), TO_PRINTF_ARGS_FN(fmt, args...));
    }
    template <typename... Args>
    static MBOOL dumpData(const RequestPtr &request, const BasicImg &buffer, const char *fmt, Args... args)
    {
        return (buffer.mBuffer != NULL) && dumpNamedData(request, buffer.mBuffer->getImageBufferPtr(), TO_PRINTF_ARGS_FN(fmt, args...));
    }
    template <typename... Args>
    static MBOOL dumpData(const RequestPtr &request, IImageBuffer *buffer, const char *fmt, Args... args)
    {
        return (buffer != NULL) && dumpNamedData(request, buffer, TO_PRINTF_ARGS_FN(fmt, args...));
    }

    static MBOOL dumpNamedData(const RequestPtr &request, IImageBuffer *buffer, const PRINTF_ARGS_FN &argsFn);
    static MUINT32 dumpData(const char *buffer, MUINT32 size, const char *filename);
    static MBOOL loadData(IImageBuffer *buffer, const char *filename);
    static MUINT32 loadData(char *buffer, size_t size, const char *filename);
    MVOID markNodeEnter(const RequestPtr &request) const;
    MVOID markNodeExit(const RequestPtr &request) const;
    Utils::ULog::ULogTimeBombHandle makeWatchDog(const RequestPtr &request, const char* dispatchKey) const;

protected:
    MUINT32 mSensorIndex;
    MINT32 mNodeDebugLV;
    StreamingFeaturePipeUsage mPipeUsage;
    android::sp<NodeSignal> mNodeSignal;
    DebugScanLine *mDebugScanLine;
    MUINT32 mDebugDumpMask = 0;
    MBOOL mUsePerFrameSetting = MFALSE;
};

class SFN_TYPE
{
public:
    enum SFN_VALUE
    {
        UNI,
        DIV,
        TWIN_P,
        TWIN_R,
        BAD,
    };
    SFN_TYPE() {}
    SFN_TYPE(SFN_VALUE type)
    : mType(type)
    {}
    MBOOL isValid() const;
    MBOOL isUni() const;
    MBOOL isDiv() const;
    MBOOL isTwinP() const;
    MBOOL isTwinR() const;
    MBOOL isPreview() const;
    MBOOL isRecord() const;
    const char* Type2Name() const;
    const char* getDumpPostfix() const;

private:
    SFN_VALUE mType = BAD;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_NODE_H_
