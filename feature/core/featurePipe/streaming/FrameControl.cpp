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

#include "FrameControl.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "FrameControl"
#define PIPE_TRACE TRACE_FRAME_CONTROL
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);


namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

FrameControl::FrameControl()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

FrameControl::~FrameControl()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID FrameControl::notify(MUINT32 requestNo)
{
    TRACE_FUNC_ENTER();
    std::lock_guard<std::mutex> _lock(mMutex);
    if( mCurrent == requestNo )
    {
        // do nothing
    }
    else if( mCurrent + 1 == requestNo )
    {
        mCurrent = requestNo;
    }
    else if( requestNo > (mCurrent + 1) )
    {
        MY_LOGW("Frame control try jump out of sequence %d=>%d, reserve %d", mCurrent, requestNo, requestNo);
        mReserveList.push_back(requestNo);
        mReserveList.sort();
    }
    else
    {
        MUINT32 max = std::max(mCurrent, requestNo);
        MY_LOGE("Frame control jump out of sequence %d=>%d, set to %d", mCurrent, requestNo, max);
        mCurrent = max;
    }

    if( mReserveList.size() )
    {
        MUINT32 target = mCurrent + 1;
        auto it = mReserveList.begin();
        while( it != mReserveList.end() )
        {
            if( *it == target )
            {
                mCurrent = target;
                ++target;
                MY_LOGW("Frame control late notify %d, current=>%d", *it, mCurrent);
                it = mReserveList.erase(it);
            }
            else if( *it < target )
            {
                MY_LOGE("Frame control reserve list get (%d) < target(%d), Strange!!, directly erase it", *it, target);
                it = mReserveList.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    mCond.notify_all();
    TRACE_FUNC_EXIT();
}

MBOOL FrameControl::wait(MUINT32 requestNo) const
{
    TRACE_FUNC_ENTER();
    if( mCurrent < requestNo )
    {
        std::unique_lock<std::mutex> _lock(mMutex);
        mCond.wait(_lock, [&] { return mCurrent >= requestNo; });
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL FrameControl::wait(MUINT32 requestNo, MUINT32 diff) const
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    ret = (requestNo > diff) && wait(requestNo-diff);
    TRACE_FUNC_EXIT();
    return ret;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
