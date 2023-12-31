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

#ifndef _MTKCAM_FEATURE_UTILS_PQ_CTRL_H_
#define _MTKCAM_FEATURE_UTILS_PQ_CTRL_H_

#include <memory>
#include <mtkcam/def/common.h>
#include <mtkcam3/feature/utils/p2/P2PlatInfo.h>

struct DpPqParam;

namespace NSCam {
namespace Feature {
namespace P2Util {

class PQCtrl
{
public:
    virtual               ~PQCtrl() {}
public:
    virtual MBOOL         supportPQ() const = 0;
    virtual MUINT32       getLogLevel() const = 0;
    virtual MBOOL         isDummy() const = 0;
    virtual MBOOL         isDump() const = 0;
    virtual MBOOL         is2ndDRE() const = 0;
    virtual MUINT32       getPQIndex() const = 0;
    virtual MVOID         fillPQInfo(DpPqParam*) const = 0;
    virtual const char*   getDebugName() const = 0;
    virtual void*         getIspTuning() const = 0;
    virtual void*         getDreTuning() const = 0;
    virtual MUINT32       getDreTuningSize() const = 0;
    virtual void*         getFDdata() const = 0;
    virtual MUINT32       getSensorID() const = 0;

    virtual MBOOL needDefaultPQ(eP2_PQ_PATH path) const = 0;
    virtual MBOOL needCZ(eP2_PQ_PATH path) const = 0;
    virtual MBOOL needHFG(eP2_PQ_PATH path) const = 0;
    virtual MBOOL needDRE(eP2_PQ_PATH path) const = 0;
    virtual MBOOL needHDR10(eP2_PQ_PATH path) const = 0;
};
using IPQCtrl_const = std::shared_ptr<const PQCtrl>;

} // namespace P2Util
} // namespace Feature
} // namespace NSCam

#endif // _MTKCAM_FEATURE_UTILS_PQ_CTRL_H_
