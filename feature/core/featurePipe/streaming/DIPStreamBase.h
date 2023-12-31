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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2_DIP_STREAM_BASE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2_DIP_STREAM_BASE_H_

#include "DIPStreamBase_t.h"

#include <featurePipe/core/include/PipeLogHeaderBegin.h>
#include "DebugControl.h"
#define PIPE_TRACE TRACE_P2DIP_STREAM_BASE
#define PIPE_CLASS_TAG "DIPStreamBase"
#include <featurePipe/core/include/PipeLog.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

template <typename T>
DIPStreamBase<T>::DIPParams_Cookie::DIPParams_Cookie()
    : mCookie(NULL)
    , mDIPCB(NULL)
    , mDIPFailCB(NULL)
    , mDIPBlockCB(NULL)
{
}

template <typename T>
DIPStreamBase<T>::DIPParams_Cookie::DIPParams_Cookie(const T_Param &param)
    : mCookie(param.mpCookie)
    , mDIPCB(param.mpfnDIPCallback)
    , mDIPFailCB(param.mpfnDIPEnQFailCallback)
    , mDIPBlockCB(param.mpfnDIPEnQBlockCallback)
{
}

template <typename T>
void DIPStreamBase<T>::DIPParams_Cookie::replace(T_Param &param, T_Token token) const
{
    param.mpCookie = token;
    param.mpfnDIPCallback = staticOnDIPStreamCB;
    param.mpfnDIPEnQFailCallback = staticOnDIPStreamFailCB;
    param.mpfnDIPEnQBlockCallback = staticOnDIPStreamBlockCB;

    if( param.mEarlyCBIndex >= 0 && (MUINT32)param.mEarlyCBIndex < param.mvDIPFrameParams.size() )
    {
        T_FrameParam &f = param.mvDIPFrameParams[param.mEarlyCBIndex];
        f.mpCookie = token;
        f.mpfnCallback = staticOnDIPStreamEarlyCB;
    }
}

template <typename T>
void DIPStreamBase<T>::DIPParams_Cookie::restore(T_Param &param) const
{
    param.mpCookie = mCookie;
    param.mpfnDIPCallback = mDIPCB;
    param.mpfnDIPEnQFailCallback = mDIPFailCB;
    param.mpfnDIPEnQBlockCallback = mDIPBlockCB;
}

template <typename T>
typename DIPStreamBase<T>::T_Token DIPStreamBase<T>::DIPParams_Cookie::getToken(const T_Param &param)
{
    return param.mpCookie;
}

template <typename T>
void DIPStreamBase<T>::enqueDIPStreamBase(T_Stream *stream, const T_Param &param, const T_Data &data)
{
    TRACE_FUNC_ENTER();
    mCookieStore.enque(this, stream, param, data);
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::waitDIPStreamBaseDone()
{
    TRACE_FUNC_ENTER();
    mCookieStore.waitAllCallDone();
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::onDIPStreamBaseEarlyCB(const T_Data & /*data*/)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::onDIPStreamBaseFailCB(const T_Param &param, const T_Data &data)
{
    TRACE_FUNC_ENTER();
    this->onDIPStreamBaseCB(param, data);
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::onDIPStreamBaseBlockCB(const T_Param &param, const T_Data &data)
{
    TRACE_FUNC_ENTER();
    this->onDIPStreamBaseCB(param, data);
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::staticOnDIPStreamCB(T_Param &param)
{
    TRACE_FUNC_ENTER();
    T_Token token = T_Cookie::getToken(param);
    T_Store::staticProcessCB(MSG_COOKIE_DONE, param, token);
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::staticOnDIPStreamEarlyCB(MVOID* cookie)
{
    TRACE_FUNC_ENTER();
    T_Token token = T_Token(cookie);
    T_Param param;
    bool IS_LAST = false;
    T_Store::staticProcessCB(MSG_COOKIE_EARLY_CB, param, token, IS_LAST);
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::staticOnDIPStreamFailCB(T_Param &param)
{
    TRACE_FUNC_ENTER();
    T_Token token = T_Cookie::getToken(param);
    T_Store::staticProcessCB(MSG_COOKIE_FAIL, param, token);
    TRACE_FUNC_EXIT();
}

template <typename T>
void DIPStreamBase<T>::staticOnDIPStreamBlockCB(T_Param &param)
{
    TRACE_FUNC_ENTER();
    void *token = T_Cookie::getToken(param);
    T_Store::staticProcessCB(MSG_COOKIE_BLOCK, param, token);
    TRACE_FUNC_EXIT();
}

template <typename T>
bool DIPStreamBase<T>::onCookieStoreEnque(T_Stream *stream, T_Param &param)
{
    TRACE_FUNC_ENTER();
    bool ret = false;
    if( stream )
    {
        ret = stream->enque(param);
    }
    else
    {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_INVALID, "invalid DIPStream: value=NULL");
    }
    TRACE_FUNC_EXIT();
    return ret;
}

template <typename T>
void DIPStreamBase<T>::onCookieStoreCB(const T_Msg &msg, T_Param param, const T_Data &data)
{
    TRACE_FUNC_ENTER();
    switch(msg)
    {
    case MSG_COOKIE_FAIL:
      param.mDequeSuccess = MFALSE;
      this->onDIPStreamBaseFailCB(param, data);
      break;
    case MSG_COOKIE_BLOCK:
      param.mDequeSuccess = MFALSE;
      this->onDIPStreamBaseBlockCB(param, data);
      break;
    case MSG_COOKIE_EARLY_CB:
      this->onDIPStreamBaseEarlyCB(data);
      break;
    case MSG_COOKIE_DONE:
    default:
      this->onDIPStreamBaseCB(param, data);
      break;
    }
    TRACE_FUNC_EXIT();
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#include <featurePipe/core/include/PipeLogHeaderEnd.h>
#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2_DIP_STREAM_BASE_H_