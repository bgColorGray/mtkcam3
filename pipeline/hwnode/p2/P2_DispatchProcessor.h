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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#ifndef _MTKCAM_HWNODE_P2_DISPATCH_PROCESSOR_H_
#define _MTKCAM_HWNODE_P2_DISPATCH_PROCESSOR_H_

#include "P2_Processor.h"
#include "P2_BasicProcessor.h"
#include "P2_StreamingProcessor.h"

#include "P2_DumpPlugin.h"
#include "P2_ScanlinePlugin.h"
#include "P2_DrawIDPlugin.h"

namespace P2
{

class DispatchProcessor : virtual public Processor<P2InitParam, P2ConfigParam, sp<P2FrameRequest>>
{
public:
    DispatchProcessor();
    virtual ~DispatchProcessor();

protected:
    virtual MBOOL onInit(const P2InitParam &param);
    virtual MVOID onUninit();
    virtual MVOID onThreadStart();
    virtual MVOID onThreadStop();
    virtual MBOOL onConfig(const P2ConfigParam &param);
    virtual MBOOL onEnque(const sp<P2FrameRequest> &request);
    virtual MVOID onNotifyFlush();
    virtual MVOID onWaitFlush();

private:
    MBOOL needBasicProcess(const sp<P2Request> &request);

private:
    ILog mLog;
    P2Info mP2Info;
    sp<P2DumpPlugin> mDumpPlugin;
    sp<P2ScanlinePlugin> mScanlinePlugin;
    sp<P2DrawIDPlugin> mDrawIDPlugin;

    BasicProcessor mBasicProcessor;
    StreamingProcessor mStreamingProcessor;

    MUINT32 mForceProcessor = VAL_P2_PROC_DFT_POLICY;
    MBOOL mEnableBasic = MFALSE;
    MBOOL mEnableStreaming = MFALSE;
};

} // namespace P2

#endif // _MTKCAM_HWNODE_P2_DISPATCH_PROCESSOR_H_
