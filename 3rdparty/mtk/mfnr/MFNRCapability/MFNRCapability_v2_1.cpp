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
#define LOG_TAG "MFNRCapability_v2_1"

#include <mtkcam/utils/std/ULog.h>
#include <stdlib.h>
#include <mtkcam/utils/std/Time.h>

#include "MFNRCapability_v2_1.h"

using namespace NSCam;
using namespace plugin;
using namespace android;
using namespace mfll;
using namespace std;
using namespace NSCam::NSPipelinePlugin;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_MFNR);
#undef MY_LOGV
#undef MY_LOGD
#undef MY_LOGI
#undef MY_LOGW
#undef MY_LOGE
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD3(...)               do { if ( MY_DBG_COND(3) ) MY_LOGD(__VA_ARGS__); } while(0)


#define __DEBUG // enable debug

#ifdef __DEBUG
#include <mtkcam/utils/std/Trace.h>
#define FUNCTION_TRACE()                            CAM_ULOGM_FUNCLIFE()
#define FUNCTION_TRACE_NAME(name)                   CAM_ULOGM_TAGLIFE(name)
#define FUNCTION_TRACE_BEGIN(name)                  CAM_ULOGM_TAG_BEGIN(name)
#define FUNCTION_TRACE_END()                        CAM_ULOGM_TAG_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)    CAM_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)      CAM_TRACE_ASYNC_END(name, cookie)
#else
#define FUNCTION_TRACE()
#define FUNCTION_TRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)
#define FUNCTION_TRACE_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)
#endif

#ifdef __DEBUG
#include <memory>
#define FUNCTION_SCOPE \
auto __scope_logger__ = [](char const* f)->std::shared_ptr<const char>{ \
    CAM_ULOGMD("(%d)[%s] + ", ::gettid(), f); \
    return std::shared_ptr<const char>(f, [](char const* p){CAM_ULOGMD("(%d)[%s] -", ::gettid(), p);}); \
}(__FUNCTION__)
#else
#define FUNCTION_SCOPE
#endif


MFNRCapability_v2_1::MFNRCapability_v2_1()
{
    FUNCTION_SCOPE;
    m_dbgLevel = mfll::MfllProperty::getDebugLevel();
}

MFNRCapability_v2_1::~MFNRCapability_v2_1()
{
    FUNCTION_SCOPE;
}

bool MFNRCapability_v2_1::updateInputBufferInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel)
{
    if (pShotInfo == nullptr)
        return false;

    if (CC_LIKELY( pShotInfo->getIsFullSizeMc() )) {
        sel.mIBufferFull.addAcceptedFormat(eImgFmt_I422);
        sel.mIBufferSpecified.addAcceptedFormat(eImgFmt_Y8);
    }
    else {
        sel.mIBufferFull.addAcceptedFormat(eImgFmt_YUY2);
        sel.mIBufferSpecified.addAcceptedFormat(eImgFmt_I422);
    }
    //
    sel.mIBufferFull.addAcceptedSize(eImgSize_Full);
    sel.mIBufferSpecified.addAcceptedSize(eImgSize_Quarter);

    return true;
}

bool MFNRCapability_v2_1::updateOutputBufferInfo(Selection& sel)
{
    sel.mOBufferFull
        .setRequired(MTRUE)
        .addAcceptedFormat(eImgFmt_YUY2) // YUY2 first
        .addAcceptedFormat(eImgFmt_NV21)
        .addAcceptedFormat(eImgFmt_NV12)
        .addAcceptedFormat(eImgFmt_I420)
        .addAcceptedSize(eImgSize_Full);

    return true;
}

bool MFNRCapability_v2_1::updateMultiCamInfo(std::shared_ptr<MFNRShotInfo> /*pShotInfo*/, Selection& /*sel*/)
{
    return true;
}
