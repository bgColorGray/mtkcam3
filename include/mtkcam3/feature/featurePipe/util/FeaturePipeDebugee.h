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

#ifndef _MTK_CAMERA_FEATURE_PIPE_UTIL_FEATURE_PIPE_DEBUGEE_H_
#define _MTK_CAMERA_FEATURE_PIPE_UTIL_FEATURE_PIPE_DEBUGEE_H_

#include <mtkcam/utils/debug/debug.h>

#define FPIPE_DEBUGEE_NAME "NSCam::NSCamFeature::NSFeaturePipe::FeaturePipeDebugee"
#define FPD_CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

template <typename Pipe_T>
class FeaturePipeDebugee
{

public:
    FeaturePipeDebugee() {}
    FeaturePipeDebugee(const Pipe_T *pFPipe)
    {
        mDebuggee = std::make_shared<MyDebuggee>(pFPipe);
        if( auto pDbgMgr = IDebuggeeManager::get() )
        {
            mDebuggee->mCookie = pDbgMgr->attach(mDebuggee, 1);
        }
    }
    virtual ~FeaturePipeDebugee()
    {
        if( mDebuggee != nullptr )
        {
            if( auto pDbgMgr = IDebuggeeManager::get() )
            {
                pDbgMgr->detach(mDebuggee->mCookie);
            }
            mDebuggee = nullptr;
        }
    }

private:
    struct MyDebuggee : public IDebuggee
    {
        std::shared_ptr<IDebuggeeCookie> mCookie = nullptr;
        const Pipe_T *mPipe = NULL;

        MyDebuggee(const Pipe_T *pFPipe)
            : mPipe(pFPipe)
        {}

        virtual ~MyDebuggee() {}
        virtual std::string debuggeeName() const { return FPIPE_DEBUGEE_NAME; }
        virtual void debug(android::Printer &printer,
                           const std::vector<std::string> &options);
    };

private:
    std::shared_ptr<MyDebuggee> mDebuggee = nullptr;
};

template <typename Pipe_T>
MVOID FeaturePipeDebugee<Pipe_T>::MyDebuggee::debug(android::Printer &printer, const std::vector<std::string> &options __unused)
{
    if(FPD_CC_LIKELY(mPipe != nullptr))
    {
        mPipe->onDumpStatus(printer);
    }
}

}; // NSFeaturePipe
}; // NSCamFeature
}; // NSCam

#endif // _MTK_CAMERA_FEATURE_PIPE_UTIL_FEATURE_PIPE_DEBUGEE_H_
