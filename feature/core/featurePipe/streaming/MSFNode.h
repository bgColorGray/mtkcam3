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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSF_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSF_NODE_H_

#include "StreamingFeatureNode.h"
#include "DIPStreamBase.h"
#include "p2g/P2GMgr.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class MSFEnqueData : public WatchDogData
{
public:
    MSFEnqueData() {}
    MSFEnqueData(const P2GHWData &data)
        : mRequest(data.mRequest)
        , mP2GHWData(data)
        {}
public:
    RequestPtr  mRequest;
    P2GHWData   mP2GHWData;
};

class MSFNode : public StreamingFeatureNode
              , public DIPStreamBase<MSFEnqueData>
              , public P2G::IP2GNode
{
public:
    MSFNode(const char *name);
    virtual ~MSFNode();

    MVOID setDIPStream(Feature::P2Util::DIPStream *stream);
    MVOID setLastP2L1NotifyCtrl(std::shared_ptr<FrameControl> &notify);

public:
    virtual MBOOL onData(DataID id, const RequestPtr &data);
    virtual MBOOL onData(DataID id, const P2GHWData &data);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

protected:
    virtual MVOID onDIPStreamBaseCB(const Feature::P2Util::DIPParams &dipParams, const MSFEnqueData &request);
    virtual MVOID onDIPStreamBaseEarlyCB(const MSFEnqueData &request);

private:

    MVOID handleResultData(const P2GHWData &data);
    MBOOL processMSF(const RequestPtr &request, const P2GHWData &data);
    MUINT32 getExpectMS(const RequestPtr &request) const;
    MVOID enqueFeatureStream(Feature::P2Util::DIPParams &params, const MSFEnqueData &data);

private:
    static const std::vector<DumpFilter> sFilterTable;

    WaitQueue<RequestPtr> mMSSReqs;
    WaitQueue<RequestPtr> mPMSSReqs;
    WaitQueue<RequestPtr> mP2SWReqs;
    WaitQueue<P2GHWData> mP2GHWDatas;

    Feature::P2Util::DIPStream *mDIPStream = NULL;
    MUINT32   mForceExpectMS = 0;
    MUINT32   mForceSmvrHighClk = 0;
    MUINT32   mForceDSDN30HighClk = 0;
    MBOOL     mNeedPMSS = MFALSE;
    std::shared_ptr<FrameControl> mLastP2L1Nofiy;
};



} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_MSF_NODE_H_

