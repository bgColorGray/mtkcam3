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

#ifndef _MTK_CAMERA_FEATURE_PIPE_VENDOR_MDP_NODE_H_
#define _MTK_CAMERA_FEATURE_PIPE_VENDOR_MDP_NODE_H_

#include "StreamingFeatureNode.h"
#include "MDPWrapper.h"

#include <vector>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class VendorMDPNode : public StreamingFeatureNode
{
public:
    typedef NSCam::NSIoPipe::Output Output;

    VendorMDPNode(const char *name, SFN_HINT typeHint);
    virtual ~VendorMDPNode();

public:
    virtual MBOOL onData(DataID id, const VMDPReqData &data);
    virtual MBOOL onData(DataID id, const DualBasicImgData &data);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;
    MVOID setSyncCtrlWait(const std::shared_ptr<ISyncCtrl> &syncCtrl);
    MVOID setSyncCtrlSignal(const std::shared_ptr<ISyncCtrl> &syncCtrl);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:
    MVOID setTimerIndex();
    SFN_TYPE::SFN_VALUE getSFNType();
    MBOOL processVendorMDP(const RequestPtr &request, const VMDPReq &mdpReq, const SyncCtrlData &syncCtrlData);
    MBOOL processGeneralMDP(const RequestPtr &request, const DualBasicImg &dualImg, BasicImg &nextFullImg, ImgBuffer &asyncImg, NextIO::NextReq &nextReq, const TSQRecInfo &tsqRecInfo, const SyncCtrlData &syncCtrlData);
    MBOOL processMDP(const char* name, const RequestPtr &request, const BasicImg &fullImg, const MDPWrapper::P2IO_OUTPUT_ARRAY& outputs, MUINT32 cycleMs);
    MVOID waitSync(const RequestPtr &request, const SyncCtrlData &syncCtrlData);
    MVOID handleResultData(const RequestPtr &request, const VMDPReq &mdpReq, const BasicImg &nextFullImg, const NextIO::NextReq &nextReq);
    MVOID dumpAdditionalInfo(android::Printer &printer) const override;
    MVOID dropRecord(const RequestPtr &request);
    MDPWrapper::P2IO_OUTPUT_ARRAY prepareMasterMDPOut(const RequestPtr &request, const BasicImg &fullImg, BasicImg &nextFullImg, ImgBuffer &asyncImg, NextIO::NextReq &nextReq, const TSQRecInfo &tsqRecInfo);
    MDPWrapper::P2IO_OUTPUT_ARRAY prepareSlaveMDPOut(const RequestPtr &request, const BasicImg &fullImg);
    MVOID refineCrop(const RequestPtr &request, const BasicImg &fullImg, P2IO &out);
    MVOID overrideTSQRecInfo(const BasicImg &fullImg, const TSQRecInfo &tsqRecInfo, P2IO &out);
    MUINT32 getMaxMDPCycleTimeMs(const RequestPtr &request, MUINT32 masterOuts, MUINT32 slaveOuts);

private:
    SFN_HINT mSFNHint = SFN_HINT::NONE;
    SFN_TYPE mSFNType = SFN_TYPE::BAD;
    MUINT32 mTimerIndex = 0;
    WaitQueue<VMDPReqData> mData;
    MDPWrapper mMDP;
    MUINT32 mNumMDPPort = 1;
    MUINT32 mForceExpectMS = 0;
    FeaturePipeParam::MSG_TYPE mDoneMsg = FeaturePipeParam::MSG_INVALID;
    const char* mLog = "";
    std::shared_ptr<ISyncCtrl> mSyncCtrlWait;
    std::shared_ptr<ISyncCtrl> mSyncCtrlSignal;
    MUINT32 mWaitFrame = 0;
    MBOOL mSupportPhyDSDN20 = MFALSE;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_FEATURE_PIPE_VENDOR_MDP_NODE_H_
