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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSS_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSS_NODE_H_

#include "StreamingFeatureNode.h"
#include "NullNode.h"
#include "DebugControl.h"

#if SUPPORT_MSS || SIMULATE_MSS

#include "MSSStreamBase.h"
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include "p2g/P2GMgr.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class MSSEnqueData : public WatchDogData
{
public:
    MSSEnqueData() {}
    MSSEnqueData(const MSSData &data)
        : mMSSData(data)
        , mRequest(data.mRequest)
        {}
public:
    RequestPtr  mRequest;
    MSSData     mMSSData;
};

class MSSNode : public StreamingFeatureNode
              , public MSSStreamBase<MSSEnqueData>
              , public P2G::IP2GNode
{
public:
    enum Type
    {
        T_CURRENT,
        T_PREVIOUS,
    };
    MSSNode(const char *name, MSSNode::Type type);
    virtual ~MSSNode();

public:
    virtual MBOOL onData(DataID id, const MSSData &data);

    MVOID setMSSWaitCtrl(std::shared_ptr<FrameControl> &wait);
    MVOID setMSSNotifyCtrl(std::shared_ptr<FrameControl> &notify);
    MVOID setMotionWaitCtrl(std::shared_ptr<FrameControl> &wait);
    MVOID setLastP2L1WaitCtrl(std::shared_ptr<FrameControl> &wait);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:
    using MSSStream = NSCam::NSIoPipe::NSEgn::IEgnStream<NSIoPipe::NSMFB20::MSSConfig>;
    using MSSParam = NSCam::NSIoPipe::NSEgn::EGNParams<NSIoPipe::NSMFB20::MSSConfig>;
    enum DumpMaskIndex
    {
        MASK_MSSO,
        MASK_OMC,
    };
    enum class ReadBack
    {
        TARGET_PMSSI,
        TARGET_OMCMV,
    };

    MBOOL processMSS(MSSData &data);
    MVOID waitBufferReady(const MSSData &data);
    NSIoPipe::NSMFB20::MSSConfig prepareMSSConfig(const RequestPtr &request, P2G::MSS &path);
    MVOID enqueMSSStream(const MSSParam &param, const MSSEnqueData &data);
    virtual MVOID onMSSStreamBaseCB(const MSSParam &param, const MSSEnqueData &data);
    MVOID handleResultData(const RequestPtr &request);
    MVOID dump(const RequestPtr &request, IImageBuffer *buffer, enum DumpMaskIndex mask, MBOOL isDump, const char *dumpStr, MBOOL isNDD, const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const TuningUtils::YUV_PORT &port, const char *nddStr = NULL) const;
    MVOID handleDump(const RequestPtr &request, const MSSData &data) const;
    MBOOL isPMSS() const;
    MVOID startMSSTimer(const RequestPtr &request) const;
    MVOID stopMSSTimer(const RequestPtr &request) const;
    MVOID startMSSEnqTimer(const RequestPtr &request) const;
    MVOID stopMSSEnqTimer(const RequestPtr &request) const;
    MVOID readBack(const RequestPtr &request, MUINT32 sensorID, ReadBack target, IImageBuffer *buffer);

private:
    MSSNode::Type mType = T_CURRENT;
    MBOOL mIsPMSS = MFALSE;
    std::string mTypeName;
    static const std::vector<DumpFilter> sFilterTable;

    MSSStream *mMSSStream = NULL;
    WaitQueue<MSSData> mDatas;

    std::shared_ptr<FrameControl> mMSSWait;
    std::shared_ptr<FrameControl> mMSSNotify;
    std::shared_ptr<FrameControl> mMotionWait;
    std::shared_ptr<FrameControl> mLastP2L1Wait;

    MBOOL mReadBack = MFALSE;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#else
#include "FrameControl.h"
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
class MSSNode : public NullNode
{
public:
    enum Type
    {
        T_CURRENT,
        T_PREVIOUS,
    };
    MSSNode(const char *name, MSSNode::Type /*type*/)
    : NullNode(name)
    {}
    MVOID setMSSWaitCtrl(std::shared_ptr<FrameControl> &/*wait*/){}
    MVOID setMSSNotifyCtrl(std::shared_ptr<FrameControl> &/*notify*/){}
    MVOID setMotionWaitCtrl(std::shared_ptr<FrameControl> &/*wait*/){}
    MVOID setLastP2L1WaitCtrl(std::shared_ptr<FrameControl> &/*wait*/){}

};
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // SUPPORT_MSS

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSS_NODE_H_

