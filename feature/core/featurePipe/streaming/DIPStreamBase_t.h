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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2_DIP_STREAM_BASE_T_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2_DIP_STREAM_BASE_T_H_

#include "MtkHeader.h"

#include "CookieStore.h"
#include <mtkcam3/feature/utils/p2/DIPStream.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

template <typename T>
class DIPStreamBase
{
public:
    typedef T T_Data;
    typedef DIPStreamBase<T_Data> T_User;
    enum T_Msg { MSG_COOKIE_DONE, MSG_COOKIE_FAIL, MSG_COOKIE_BLOCK, MSG_COOKIE_EARLY_CB };
    typedef Feature::P2Util::DIPStream T_Stream;
    typedef Feature::P2Util::DIPParams T_Param;
    typedef Feature::P2Util::DIPFrameParams T_FrameParam;
    typedef T_CookieStoreToken T_Token;
    typedef CookieStore<T_User> T_Store;

    class DIPParams_Cookie
    {
    public:
        DIPParams_Cookie();
        DIPParams_Cookie(const T_Param &param);
        ~DIPParams_Cookie() {}
        void replace(T_Param &param, T_Token token) const;
        void restore(T_Param &param) const;
        static T_Token getToken(const T_Param &param);

    private:
        void* mCookie;
        T_Param::PFN_DIP_CALLBACK_T mDIPCB;
        T_Param::PFN_DIP_CALLBACK_T mDIPFailCB;
        T_Param::PFN_DIP_CALLBACK_T mDIPBlockCB;
    };
    typedef DIPParams_Cookie T_Cookie;

public:
    virtual ~DIPStreamBase() {}
    virtual void enqueDIPStreamBase(T_Stream *stream, const T_Param &param, const T_Data &data);
    void waitDIPStreamBaseDone();

protected:
    virtual void onDIPStreamBaseCB(const T_Param &param, const T_Data &data) = 0;
    virtual void onDIPStreamBaseEarlyCB(const T_Data &data);
    virtual void onDIPStreamBaseFailCB(const T_Param &param, const T_Data &data);
    virtual void onDIPStreamBaseBlockCB(const T_Param &param, const T_Data &data);

public:
    static void staticOnDIPStreamCB(T_Param &param);
    static void staticOnDIPStreamEarlyCB(MVOID* cookie);
    static void staticOnDIPStreamFailCB(T_Param &param);
    static void staticOnDIPStreamBlockCB(T_Param &param);

private:
    virtual bool onCookieStoreEnque(T_Stream *stream, T_Param &param);
    virtual void onCookieStoreCB(const T_Msg &msg, T_Param param, const T_Data &data);

private:
    friend T_Store;
    T_Store mCookieStore;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2_DIP_STREAM_BASE_T_H_
