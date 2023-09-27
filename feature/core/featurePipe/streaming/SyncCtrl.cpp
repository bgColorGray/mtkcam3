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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#include "SyncCtrl.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "SyncCtrl"
#define PIPE_TRACE TRACE_SYNC_CTRL
#include <featurePipe/core/include/PipeLog.h>

#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);
using namespace NSCam::Utils::ULog;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

SyncCtrlData::SyncCtrlData()
{
}

SyncCtrlData::SyncCtrlData(const SyncOrder syncOrder, const MUINT32 frameGap)
    : mSyncOrder(syncOrder)
    , mP_R_Gap(frameGap)
{
}

MVOID ISyncCtrl::notifySignal(const SyncCtrlData &data, MUINT32 curFrame)
{
    TRACE_FUNC_ENTER();
    std::lock_guard<std::mutex> lock(mMutex);
    TRACE_FUNC("Order(%u) Signal(%u) -> (%u)", data.mSyncOrder, mSignalFrame, curFrame);
    if( curFrame <= mSignalFrame )
    {
        MY_LOGE("Order(%u) Signal(%d) -> (%d) is the same or time back.", data.mSyncOrder, mSignalFrame, curFrame);
    }
    mSignalFrame = curFrame;
    mCondition.notify_all();
    TRACE_FUNC_EXIT();
}

MVOID ISyncCtrl::waitSignal(const SyncCtrlData &data, MUINT32 reqNum)
{
    TRACE_FUNC("Order(%u) reqNum(%u) +", data.mSyncOrder, reqNum);
    if( data.mSyncOrder != SyncOrder::NO_SYNC )
    {
        std::unique_lock<std::mutex> lock(mMutex);
        TRACE_FUNC("Order(%u) reqNum(%u) waitFrame(%d) +", data.mSyncOrder, reqNum, getWaitFrame(data, reqNum));
        mCondition.wait(lock, [this, data, reqNum] { return waitPredicate(data, reqNum); });
        TRACE_FUNC("Order(%u) reqNum(%u) mSignalFrame(%d) -", data.mSyncOrder, reqNum, mSignalFrame);
    }
    TRACE_FUNC_EXIT();
}

MBOOL ISyncCtrl::waitPredicate(const SyncCtrlData &data, MUINT32 reqNum)
{
    return (mSignalFrame >= getWaitFrame(data, reqNum));
}

MVOID SyncCtrlData::wait(const std::shared_ptr<ISyncCtrl> &syncCtrl, MUINT32 reqNum) const
{
    if( syncCtrl )
    {
        syncCtrl->waitSignal(*this, reqNum);
    }
}

MVOID SyncCtrlData::notify(const std::shared_ptr<ISyncCtrl> &syncCtrl, MUINT32 reqNum) const
{
    if( syncCtrl )
    {
        syncCtrl->notifySignal(*this, reqNum);
    }
}

MUINT32 SyncCtrlData::getWaitFrame(const std::shared_ptr<ISyncCtrl> &syncCtrl, MUINT32 reqNum) const
{
    return syncCtrl ? syncCtrl->getWaitFrame(*this, reqNum) : 0;
}

MUINT32 SyncCtrlRwaitP::getWaitFrame(const SyncCtrlData &data, MUINT32 reqNum)
{
    return reqNum + data.mP_R_Gap;
}

MUINT32 SyncCtrlPwaitR::getWaitFrame(const SyncCtrlData &data, MUINT32 reqNum)
{
    return (reqNum > data.mP_R_Gap) ? (reqNum - data.mP_R_Gap - 1) : 0;
}

} // NSFeaturePipe
} // NSCamFeature
} // NSCam
