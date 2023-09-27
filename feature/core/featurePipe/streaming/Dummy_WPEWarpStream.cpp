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

#include "Dummy_WPEWarpStream.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "DummyWPEWarpStream"
#define PIPE_TRACE TRACE_WPE_STREAM_BASE
#include <featurePipe/core/include/PipeLog.h>

#include "GPUWarpStream.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

Dummy_WPEWarpStream* Dummy_WPEWarpStream::createInstance()
{
    return new Dummy_WPEWarpStream();
}

Dummy_WPEWarpStream::Dummy_WPEWarpStream()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

Dummy_WPEWarpStream::~Dummy_WPEWarpStream()
{
    TRACE_FUNC_ENTER();
    this->uninit();
    TRACE_FUNC_EXIT();
}

MBOOL Dummy_WPEWarpStream::init(const MUINT32 &sensorIdx, const MSize &maxImageSize, const MSize &maxWarpSize)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    ret = mGPUWarpStream.init(sensorIdx, 0, maxImageSize, maxWarpSize);
    TRACE_FUNC_EXIT();
    return ret;
}

MVOID Dummy_WPEWarpStream::uninit()
{
    TRACE_FUNC_ENTER();
    mGPUWarpStream.uninit();
    TRACE_FUNC_EXIT();
}

MBOOL Dummy_WPEWarpStream::enque(Dummy_WPEParam param)
{
    TRACE_FUNC_ENTER();
    MY_LOGD("in dummy wpe");
    WarpParam warpParam;
    warpParam.mRequest = param.mRequest;
    warpParam.mIn = param.mIn;
    warpParam.mWarpOut = param.mOut;
    warpParam.mWarpMap = param.mWarpMap;
    warpParam.mInSize = param.mInSize;
    warpParam.mOutSize = param.mOutSize;
    warpParam.mMDPOut = param.mMDPOut;
    this->enqueWarpStreamBase(&mGPUWarpStream, warpParam, param);
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID Dummy_WPEWarpStream::onWarpStreamBaseCB(const WarpParam &warpParam, const Dummy_WPEParam &param)
{
    TRACE_FUNC_ENTER();
    if( param.mCallback )
    {
        Dummy_WPEParam temp = param;
        temp.mResult = warpParam.mResult;
        param.mCallback(temp);
    }
    TRACE_FUNC_EXIT();
}

MVOID Dummy_WPEWarpStream::destroyInstance()
{
    TRACE_FUNC_ENTER();
    delete this;
    TRACE_FUNC_EXIT();
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam