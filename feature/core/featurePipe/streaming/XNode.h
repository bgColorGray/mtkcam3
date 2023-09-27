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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_X_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_X_NODE_H_

#include "StreamingFeatureNode.h"
#include "MtkHeader.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class XNode : public StreamingFeatureNode
{
public:
    struct Inputs
    {
        NextImg mMasterFull;
        NextImg mMasterPreview;
        NextImg mMasterRecord;
        NextImg mMasterTiny;
        NextImg mSlaveFull;
        NextImg mSlavePreview;
        NextImg mSlaveRecord;
        NextImg mSlaveTiny;
    };
    struct Outputs
    {
        P2IO mPreview;
        P2IO mRecord;
        std::vector<P2IO> mExtra;
        P2IO mPhy1;
        P2IO mPhy2;
    };

public:
    XNode(const char *name);
    virtual ~XNode();

public:
    virtual MBOOL onData(DataID id, const NextData &data);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:
    android::sp<IBufferPool> createPool(const char *name, const MSize &size, EImageFormat fmt) const;
    MBOOL process(const RequestPtr &request, const NextResult &data);
    MBOOL getInput(const RequestPtr &request, const NextResult &data, Inputs &inputs);
    MBOOL getOutput(const RequestPtr &request, const NextResult &data, Outputs &outputs);
    MBOOL simpleCopy(const RequestPtr &request, const Inputs &inputs, const Outputs &outputs);
    MVOID printOut(const Outputs &out) const;
    MBOOL checkCropValid(const MRectF &crop, const MSizeF &inSize) const;

private:
    WaitQueue<NextData> mNextDatas;

    MBOOL mPrint = MFALSE;
    MBOOL mUseP2APQ = MFALSE;
    MBOOL mUseDivInput = MFALSE;
    MBOOL mUseTiny = MFALSE;
    MUINT32 mUseDummy = 0;
    android::sp<IBufferPool> mFullPool;
    android::sp<IBufferPool> mPreviewPool;
    android::sp<IBufferPool> mTinyPool;
    MUINT32 mBufferCount = 6;

    MDPWrapper mMDP;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_X_NODE_H_
