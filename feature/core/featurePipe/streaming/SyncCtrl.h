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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_SYNC_CTRL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_SYNC_CTRL_H_

#include <memory>
#include <mutex>
#include <condition_variable>
#include <mtkcam/def/common.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

enum class SyncOrder
{
    NO_SYNC,
    A,
    B,
};

class SyncCtrlData;

class ISyncCtrl
{
public:
    virtual ~ISyncCtrl() {}
    MVOID waitSignal(const SyncCtrlData &data, MUINT32 reqNum);
    MVOID notifySignal(const SyncCtrlData &data, MUINT32 reqNum);

    virtual MBOOL waitPredicate(const SyncCtrlData &data, MUINT32 reqNum);
    virtual MUINT32 getWaitFrame(const SyncCtrlData &data, MUINT32 reqNum) = 0;

protected:
    std::mutex mMutex;
    std::condition_variable mCondition;
    MUINT32 mSignalFrame = 0;
};

class SyncCtrlRwaitP : public ISyncCtrl
{
public:
    MUINT32 getWaitFrame(const SyncCtrlData &data, MUINT32 reqNum) override;
};

class SyncCtrlPwaitR : public ISyncCtrl
{
public:
    MUINT32 getWaitFrame(const SyncCtrlData &data, MUINT32 reqNum) override;
};

class SyncCtrlData
{
public:
    SyncCtrlData();
    SyncCtrlData(const SyncOrder syncOrder, const MUINT32 frameGap);
    MVOID wait(const std::shared_ptr<ISyncCtrl> &syncCtrl, MUINT32 reqNum) const;
    MVOID notify(const std::shared_ptr<ISyncCtrl> &syncCtrl, MUINT32 reqNum) const;
    MUINT32 getWaitFrame(const std::shared_ptr<ISyncCtrl> &syncCtrl, MUINT32 reqNum) const;

private:
    SyncOrder mSyncOrder = SyncOrder::NO_SYNC;
    MUINT32 mP_R_Gap = 0;

    friend class ISyncCtrl;
    friend class SyncCtrlRwaitP;
    friend class SyncCtrlPwaitR;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_SYNC_CTRL_H_