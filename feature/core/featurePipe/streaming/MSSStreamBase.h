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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSS_STREAM_BASE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSS_STREAM_BASE_H_

#include "MSSStreamBase_t.h"

#include <featurePipe/core/include/PipeLogHeaderBegin.h>
#include "DebugControl.h"
#define PIPE_TRACE TRACE_MSS_STREAM_BASE
#define PIPE_CLASS_TAG "MSSStreamBase"
#include <featurePipe/core/include/PipeLog.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

template <typename T>
MSSStreamBase<T>::MSSStream_Cookie::MSSStream_Cookie()
    : mCookie(NULL)
    , mCB(NULL)
{
}

template <typename T>
MSSStreamBase<T>::MSSStream_Cookie::MSSStream_Cookie(const T_Param &param)
    : mCookie(param.mpCookie)
    , mCB(param.mpfnCallback)
{
}

template <typename T>
void MSSStreamBase<T>::MSSStream_Cookie::replace(T_Param &param, T_Token token) const
{
    param.mpCookie = token;
    param.mpfnCallback = staticOnMSSStreamCB;
}

template <typename T>
void MSSStreamBase<T>::MSSStream_Cookie::restore(T_Param &param) const
{
    param.mpCookie = mCookie;
    param.mpfnCallback = mCB;
}

template <typename T>
typename MSSStreamBase<T>::T_Token  MSSStreamBase<T>::MSSStream_Cookie::getToken(const T_Param &param)
{
    return param.mpCookie;
}

template <typename T>
void MSSStreamBase<T>::enqueMSSStreamBase(T_Stream *stream, const T_Param &param, const T_Data &data)
{
    TRACE_FUNC_ENTER();
    mCookieStore.enque(this, stream, param, data);
    TRACE_FUNC_EXIT();
}

template <typename T>
void MSSStreamBase<T>::waitMSSStreamBaseDone()
{
    TRACE_FUNC_ENTER();
    mCookieStore.waitAllCallDone();
    TRACE_FUNC_EXIT();
}

template <typename T>
void MSSStreamBase<T>::staticOnMSSStreamCB(T_Param &param)
{
    TRACE_FUNC_ENTER();
    T_Token token = T_Cookie::getToken(param);
    T_Store::staticProcessCB(MSG_COOKIE_DONE, param, token);
    TRACE_FUNC_EXIT();
}

template <typename T>
bool MSSStreamBase<T>::onCookieStoreEnque(T_Stream *stream, const T_Param &param)
{
    TRACE_FUNC_ENTER();
    bool ret = false;
    if( stream )
    {
        ret = stream->EGNenque(param);
    }
    TRACE_FUNC_EXIT();
    return ret;
}

template <typename T>
void MSSStreamBase<T>::onCookieStoreCB(const T_Msg &msg, T_Param param, const T_Data &data)
{
    TRACE_FUNC_ENTER();
    switch(msg)
    {
    case MSG_COOKIE_FAIL:
        this->onMSSStreamBaseCB(param, data);
        break;
    case MSG_COOKIE_DONE:
    default:
        this->onMSSStreamBaseCB(param, data);
        break;
    }
    TRACE_FUNC_EXIT();
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#include <featurePipe/core/include/PipeLogHeaderEnd.h>
#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_MSS_STREAM_BASE_H_

