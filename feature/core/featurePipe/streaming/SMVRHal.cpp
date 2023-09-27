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

#include "SMVRHal.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "SMVRHal"
#define PIPE_TRACE TRACE_SMVR_HAL
#include <featurePipe/core/include/PipeLog.h>

#define KEY_SMVR_SPEED "vendor.debug.fpipe.p2sm.speed"
#define KEY_SMVR_BURST "vendor.debug.fpipe.p2sm.burst"

#define BURST_SEC 1

#include <cutils/properties.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

SMVRHal::SMVRHal()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

SMVRHal::~SMVRHal()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL SMVRHal::init(const StreamingFeaturePipeUsage &pipeUsage)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( mState == State::IDLE )
    {
        MUINT32 hintSpeed = pipeUsage.getSMVRSpeed();
        MINT32 propSpeed = property_get_int32(KEY_SMVR_SPEED, 0);

        mState = State::READY;
        mFullSize = pipeUsage.getStreamingSize();
        mIsSupport = pipeUsage.supportSMP2();
        mP1Speed = pipeUsage.getP1Batch();
        mP2Speed = propSpeed > 0 ? propSpeed : hintSpeed;
        mRecover = (mP2Speed && (mP1Speed > mP2Speed)) ?
                   (mP1Speed - mP2Speed) * BURST_SEC * 30 / mP2Speed : 0;

        MY_LOGI("SMVR enable=%d size=(%dx%d) p1=%d p2=%d->%d recover=%d",
                mIsSupport, mFullSize.w, mFullSize.h,
                mP1Speed, hintSpeed, mP2Speed, mRecover);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL SMVRHal::uninit()
{
    mState = State::IDLE;
    return MTRUE;
}

MBOOL SMVRHal::isSupport() const
{
    return mIsSupport;
}

MUINT32 SMVRHal::getP1Speed() const
{
    return mP1Speed;
}

MUINT32 SMVRHal::getP2Speed() const
{
    return mP2Speed;
}

MUINT32 SMVRHal::getRecover() const
{
    return mRecover;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
