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

#ifndef _MTK_CAMERA_DUMMY_RSC_STREAM_H_
#define _MTK_CAMERA_DUMMY_RSC_STREAM_H_

#include <queue>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>

#include "MtkHeader.h"

#include <mtkcam/drv/iopipe/PostProc/IHalRscPipe.h>
#include <mtkcam/drv/iopipe/PostProc/IRscStream.h>

namespace NSCam {

typedef NSCam::NSIoPipe::NSRsc::RSCParams RSCParam;
typedef NSCam::NSIoPipe::NSRsc::IHalRscPipe IHalRscPipe;

class RSCStream : public IHalRscPipe
{
public:
    static RSCStream* createInstance(const char* szCallerName);

    MBOOL init();
    MBOOL uninit();
    MBOOL RSCenque(const RSCParam &param);
    MBOOL RSCdeque(RSCParam &param, MINT64 i8TimeoutNs = -1);
    MVOID destroyInstance(const char* szCallerName);
    const char* getPipeName() const;

protected:
    virtual ~RSCStream();

private:
    RSCStream();

    class ProcessThread : public android::Thread
    {
    public:
        ProcessThread();
        virtual ~ProcessThread();
        void enque(const RSCParam &param);
        void signalStop();

    public:
        android::status_t readyToRun();
        bool threadLoop();

    private:
        bool waitParam(RSCParam &param);
        void processParam(RSCParam param);

    private:
        std::queue<RSCParam> mQueue;
        android::Mutex mThreadMutex;
        android::Condition mThreadCondition;
        bool mStop;
    };

    android::Mutex mMutex;
    android::sp<ProcessThread> mProcessThread;
};

} // namespace NSCam

#endif // _MTK_CAMERA_DUMMY_RSC_STREAM_H_
