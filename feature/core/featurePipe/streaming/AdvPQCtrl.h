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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_ADV_PQ_CTRL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_ADV_PQ_CTRL_H_

#include <string>
#include <memory>

#include <mtkcam3/feature/utils/p2/PQCtrl.h>
#include <featurePipe/core/include/TuningBufferPool.h>

using ::NSCam::Feature::P2Util::PQCtrl;
using ::NSCam::Feature::P2Util::IPQCtrl_const;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class AdvPQCtrl;
using IAdvPQCtrl = std::shared_ptr<AdvPQCtrl>;
using IAdvPQCtrl_const = std::shared_ptr<const AdvPQCtrl>;

class AdvPQCtrl : public PQCtrl
{
public:
    template <typename... Args>
    static std::shared_ptr<AdvPQCtrl> clone(Args&&... args)
    {
        return std::make_shared<AdvPQCtrl>(std::forward<Args>(args)...);
    }
public:
    AdvPQCtrl(const IPQCtrl_const &basicPQ, const char *name);
    AdvPQCtrl(const IAdvPQCtrl_const &advPQ, const char *name);
    virtual ~AdvPQCtrl();
    MVOID setIsDummy(MBOOL val);
    MVOID setIsDump(MBOOL val);
    MVOID setIs2ndDRE(MBOOL val);
    MVOID setIspTuning(const TunBuffer &ispTuning);
    MVOID setDreTuning(const TunBuffer &dreTuning);
public:
    virtual MBOOL         supportPQ() const;
    virtual MUINT32       getLogLevel() const;
    virtual MBOOL         isDummy() const;
    virtual MBOOL         isDump() const;
    virtual MBOOL         is2ndDRE() const;
    virtual MUINT32       getPQIndex() const;
    virtual MVOID         fillPQInfo(DpPqParam *pq) const;
    virtual const char*   getDebugName() const;
    virtual void*         getIspTuning() const;
    virtual void*         getDreTuning() const;
    virtual MUINT32       getDreTuningSize() const;
    virtual void*         getFDdata() const;
    virtual MUINT32       getSensorID() const;

    virtual MBOOL needDefaultPQ(eP2_PQ_PATH path) const;
    virtual MBOOL needCZ(eP2_PQ_PATH path) const;
    virtual MBOOL needHFG(eP2_PQ_PATH path) const;
    virtual MBOOL needDRE(eP2_PQ_PATH path) const;
    virtual MBOOL needHDR10(eP2_PQ_PATH path) const;

private:
    std::string mName;
    IPQCtrl_const mPQ;
    MBOOL mIsDummy = MFALSE;
    MBOOL mIsDump = MFALSE;
    MBOOL mIs2ndDRE = MFALSE;
    TunBuffer mIspTuning;
    TunBuffer mDreTuning;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_ADV_PQ_CTRL_H_
