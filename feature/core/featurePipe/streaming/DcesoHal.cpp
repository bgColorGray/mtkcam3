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

#include "DcesoHal.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "DcesoHal"
#define PIPE_TRACE TRACE_DCESO_HAL
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

DcesoHal::DcesoHal()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

DcesoHal::~DcesoHal()
{
    TRACE_FUNC_ENTER();
    uninit();
    TRACE_FUNC_EXIT();
}

MBOOL DcesoHal::init(MBOOL hasBufferInfo, const NS3Av3::Buffer_Info &ispBufferInfo)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( mState == State::IDLE )
    {
        if( !hasBufferInfo )
        {
            MY_LOGE("missing isp buffer info");
        }
        else
        {
            mIsSupport = ispBufferInfo.DCESO_Param.bSupport;
            mSize = ispBufferInfo.DCESO_Param.size;
            mFormat = (EImageFormat)ispBufferInfo.DCESO_Param.format;
            ret = MTRUE;
        }
        mState = ret ? State::READY : State::FAIL;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL DcesoHal::uninit()
{
    mState = State::IDLE;
    return MTRUE;
}

MUINT32 DcesoHal::getDelayCount() const
{
    return DCESO_DELAY_COUNT;
}

MBOOL DcesoHal::isSupport() const
{
    return mIsSupport;
}

MSize DcesoHal::getBufferSize() const
{
    return mSize;
}

EImageFormat DcesoHal::getBufferFmt() const
{
    return mFormat;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
