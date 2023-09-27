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

#include "AdvPQCtrl.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "AdvPQCtrl"
#define PIPE_TRACE TRACE_ADV_PQ_CTRL
#include <featurePipe/core/include/PipeLog.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

AdvPQCtrl::AdvPQCtrl(const IPQCtrl_const &basicPQ, const char *name)
    : mName(name ? name : "unknown")
    , mPQ(basicPQ)
{
    if( basicPQ )
    {
        mIsDump = basicPQ->isDump();
    }
}

AdvPQCtrl::AdvPQCtrl(const IAdvPQCtrl_const &advPQ, const char *name)
    : mName(name ? name : "unknown")
{
    if( advPQ )
    {
        mPQ = advPQ->mPQ;
        mIsDummy = advPQ->mIsDummy;
        mIsDump = advPQ->mIsDump;
        mIs2ndDRE = advPQ->mIs2ndDRE;
        mIspTuning = advPQ->mIspTuning;
        mDreTuning = advPQ->mDreTuning;
    }
}

AdvPQCtrl::~AdvPQCtrl()
{
}

MVOID AdvPQCtrl::setIsDummy(MBOOL dummy)
{
    mIsDummy = dummy;
}

MVOID AdvPQCtrl::setIsDump(MBOOL dump)
{
    mIsDump = dump;
}

MVOID AdvPQCtrl::setIs2ndDRE(MBOOL dre)
{
    mIs2ndDRE = dre;
}

MVOID AdvPQCtrl::setIspTuning(const TunBuffer &ispTuning)
{
    mIspTuning = ispTuning;
}

MVOID AdvPQCtrl::setDreTuning(const TunBuffer &dreTuning)
{
    mDreTuning = dreTuning;
}

#define ADD_ADV_PQ_CTRL(type, func, val)      \
type AdvPQCtrl::func() const                  \
{                                             \
    return mPQ ? mPQ->func() : val;           \
}
#define ADD_ADV_PQ_PATH_CTRL(func)            \
MBOOL AdvPQCtrl::func(eP2_PQ_PATH path) const \
{                                             \
    return mPQ ? mPQ->func(path) : MFALSE;    \
}

ADD_ADV_PQ_CTRL(MBOOL,      supportPQ,    MFALSE);
ADD_ADV_PQ_CTRL(MUINT32,    getLogLevel,  0);
ADD_ADV_PQ_CTRL(MUINT32,    getPQIndex,   0);
ADD_ADV_PQ_CTRL(void*,      getFDdata,    NULL);
ADD_ADV_PQ_CTRL(MUINT32,    getSensorID,  MUINT32(-1));
ADD_ADV_PQ_PATH_CTRL(needDefaultPQ);
ADD_ADV_PQ_PATH_CTRL(needCZ);
ADD_ADV_PQ_PATH_CTRL(needHFG);
ADD_ADV_PQ_PATH_CTRL(needDRE);
ADD_ADV_PQ_PATH_CTRL(needHDR10);

MVOID AdvPQCtrl::fillPQInfo(DpPqParam *pq) const
{
    if( mPQ )
    {
        mPQ->fillPQInfo(pq);
    }
}

MBOOL AdvPQCtrl::isDummy() const
{
    return mIsDummy;
}

MBOOL AdvPQCtrl::isDump() const
{
    return mIsDump;
}

MBOOL AdvPQCtrl::is2ndDRE() const
{
    return mIs2ndDRE;
}

const char* AdvPQCtrl::getDebugName() const
{
    return mName.c_str();
}

void* AdvPQCtrl::getIspTuning() const
{
    return (mIspTuning != NULL) ? mIspTuning->mpVA : NULL;
}

void* AdvPQCtrl::getDreTuning() const
{
    return (mDreTuning != NULL) ? mDreTuning->mpVA : NULL;
}

MUINT32 AdvPQCtrl::getDreTuningSize() const
{
    return (mDreTuning != NULL) ? mDreTuning->mSize : 0;
}

} // NSFeaturePipe
} // NSCamFeature
} // NSCam
