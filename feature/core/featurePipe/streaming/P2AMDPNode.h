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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2AMDP_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2AMDP_NODE_H_

#include "StreamingFeatureNode.h"
#include "MDPWrapper.h"
#include "p2g/P2GMgr.h"


namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {


class P2AMDPNode : public StreamingFeatureNode
                 , public P2G::IP2GNode
{
public:
    P2AMDPNode(const char *name);
    virtual ~P2AMDPNode();

public:
    virtual MBOOL onData(DataID id, const P2GHWData &data);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onThreadLoop();

private:
    enum DumpMaskIndex
    {
        MASK_FULL_M,
        MASK_NEXT_M,
        MASK_IMG2O_M,
        MASK_FULL_S,
        MASK_NEXT_S,
        MASK_IMG2O_S,
        MASK_TIMGO,
        MASK_DSDN30_DSW_WEI,
        MASK_DSDN25_SMVR_SUB,
        MASK_DSDN30_CONF_IDI,
        MASK_DNO_M,
        MASK_DNO_S,
        MASK_PMDP_ALL,
    };
    MVOID handleRequest(const PMDPReq &data, const RequestPtr &request);
    MBOOL processMDP(const RequestPtr &request, const BasicImg &mdpIn, std::vector<P2IO> mdpOuts, const TunBuffer &tuning, MUINT32 cycleTime);
    MUINT32 getMaxMDPCycleTimeMs(const PMDPReq &data, const RequestPtr &request);
    MVOID dump(const RequestPtr &request, IImageBuffer *buffer, enum DumpMaskIndex mask, MBOOL isDump, const char *dumpStr, MBOOL isNDD, const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const TuningUtils::YUV_PORT &port, const char *nddStr = NULL) const;
    MVOID handleDump(const RequestPtr &request, const P2HWHolder &data) const;
    MVOID handleResultData(const RequestPtr &request, const P2HWHolder &data);

private:
    static const std::vector<DumpFilter> sFilterTable;
    WaitQueue<P2GHWData> mMDPRequests;
    MDPWrapper mMDP;
    MUINT32 mNumMDPPort = 1;
    MUINT32 mForceExpectMS = 0;

};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_ROOT_NODE_H_
