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

#include "../DebugControl.h"
#define PIPE_CLASS_TAG "P2G_MGR"
#define PIPE_TRACE TRACE_P2G_MGR
#include <featurePipe/core/include/PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

#include "SensorContext.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

SensorContext::SensorContext(MUINT32 id)
{
    TRACE_FUNC_ENTER();
    init(id);
    TRACE_FUNC_EXIT();
}

SensorContext::~SensorContext()
{
    TRACE_FUNC_ENTER();
    uninit();
    TRACE_FUNC_EXIT();
}

void SensorContext::init(MUINT32 id)
{
    TRACE_FUNC_ENTER();
    {
        mSensorIndex = id;
        mHal3A = SUPPORT_3A_HAL ? MAKE_Hal3A(id, PIPE_CLASS_TAG) : NULL;
        mHalISP = SUPPORT_ISP_HAL ? MAKE_HalISP(id, PIPE_CLASS_TAG) : NULL;
    }
    TRACE_FUNC_EXIT();
}

void SensorContext::uninit()
{
    TRACE_FUNC_ENTER();
    {
        if( mHal3A )
        {
            mHal3A->destroyInstance(PIPE_CLASS_TAG);
            mHal3A = NULL;
        }
        if( mHalISP )
        {
            mHalISP->destroyInstance(PIPE_CLASS_TAG);
            mHalISP = NULL;
        }
    }
    TRACE_FUNC_EXIT();
}

SensorContextMap::SensorContextMap()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

SensorContextMap::~SensorContextMap()
{
    TRACE_FUNC_ENTER();
    if( mReady )
    {
        uninit();
    }
    TRACE_FUNC_EXIT();
}

void SensorContextMap::init(const std::vector<MUINT32> &ids)
{
    TRACE_FUNC_ENTER();
    if( !mReady )
    {
        for( const auto &id : ids )
        {
            if( mMap[id] == NULL )
            {
                mMap[id] = std::make_shared<SensorContext>(id);
            }
        }
        mReady = true;
    }
    TRACE_FUNC_EXIT();
}

void SensorContextMap::uninit()
{
    TRACE_FUNC_ENTER();
    if( mReady )
    {
        mMap.clear();
        mReady = false;
    }
    TRACE_FUNC_EXIT();
}

SensorContextPtr SensorContextMap::find(MUINT32 id) const
{
    TRACE_FUNC_ENTER();
    auto it = mMap.find(id);
    TRACE_FUNC_EXIT();
    return it != mMap.end() ? it->second : NULL;
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
