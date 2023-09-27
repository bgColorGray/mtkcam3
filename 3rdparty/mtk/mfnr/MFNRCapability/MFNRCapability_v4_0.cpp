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
#define LOG_TAG "MFNRCapability_v4_0"

#include <mtkcam/utils/std/ULog.h>
#include <stdlib.h>
#include <mtkcam/utils/std/Time.h>

#include "MFNRCapability_v4_0.h"

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


MFNRCapability_v4_0::MFNRCapability_v4_0()
{
    FUNCTION_SCOPE;
    mSupport10BitOutput = property_get_int32("vendor.debug.p2c.10bits.enable", 1);
    m_dbgLevel = mfll::MfllProperty::getDebugLevel();
}

MFNRCapability_v4_0::~MFNRCapability_v4_0()
{
    FUNCTION_SCOPE;
}


bool MFNRCapability_v4_0::updateInputBufferInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel)
{
    if (pShotInfo == nullptr)
        return false;

    if (sel.mState.mMultiCamFeatureMode != MTK_MULTI_CAM_FEATURE_MODE_VSDOF) { // Normal MFNR
        sel.mIBufferFull.addAcceptedFormat(eImgFmt_MTK_YUV_P010);
        sel.mIBufferSpecified.addAcceptedFormat(eImgFmt_NV12/*eImgFmt_Y8*/); //Due to postview issue
        sel.mIBufferResized.addAcceptedFormat(eImgFmt_MTK_YUV_P012);
    }
    else { // VSDOF not support P2-MSS
        sel.mIBufferFull.addAcceptedFormat(eImgFmt_MTK_YUV_P010);
        sel.mIBufferSpecified.addAcceptedFormat(eImgFmt_NV12/*eImgFmt_Y8*/); //Due to postview issue
        sel.mIBufferResized.addAcceptedFormat(eImgFmt_MTK_YUV_P010);
        pShotInfo->setInputYuvFmt(InputYuvFmt_YUV_P010);
    }
    {
        MSize fullSize = calcDownScaleSize(pShotInfo->getSizeSrc(), pShotInfo->getDownscaleDividend(), pShotInfo->getDownscaleDivisor());
        MSize quaterSize = calcDownScaleSize(fullSize, 1, (!pShotInfo->getEnableDownscale()?4:2));
        MSize mssSize = MSize((fullSize.w+3)/4*2, (fullSize.h+3)/4*2);
        sel.mIBufferFull.addAcceptedSize(eImgSize_Specified).setSpecifiedSize(fullSize);
        sel.mIBufferSpecified.addAcceptedSize(eImgSize_Specified).setSpecifiedSize(quaterSize);
        sel.mIBufferResized.addAcceptedSize(eImgSize_Specified).setSpecifiedSize(mssSize);
    }

    if (sel.mRequestIndex == 0) {
        // special required for MFNR4.0
        sel.mIBufferDCES.setRequired(MTRUE);
        sel.mIBufferResized.setRequired(MTRUE).setAlignment(getMYUVAlign().w, getMYUVAlign().h);
        if (sel.mState.mMultiCamFeatureMode != MTK_MULTI_CAM_FEATURE_MODE_VSDOF) // Normal MFNR
            sel.mIBufferResized.setCapability(eBufCap_MSS);
        else
            sel.mIBufferResized.setCapability(eBufCap_IMGO);
    }

    return true;
}

bool MFNRCapability_v4_0::updateOutputBufferInfo(Selection& sel)
{
    if (mSupport10BitOutput && sel.mState.mMultiCamFeatureMode != MTK_MULTI_CAM_FEATURE_MODE_VSDOF) // Normal MFNR
        sel.mOBufferFull.addAcceptedFormat(eImgFmt_MTK_YUV_P010);
    sel.mOBufferFull
        .setRequired(MTRUE)
        //.addAcceptedFormat(eImgFmt_MTK_YUV_P010) // YUV 10 bit Packed first
        .addAcceptedFormat(eImgFmt_NV12) // NV12 first
        .addAcceptedFormat(eImgFmt_YUY2)
        .addAcceptedFormat(eImgFmt_NV21)
        .addAcceptedFormat(eImgFmt_I420)
        .addAcceptedSize(eImgSize_Full);

    return true;
}

bool MFNRCapability_v4_0::updateMultiCamInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel)
{
    if (CC_LIKELY( !sel.mState.mDualCamDedicatedYuvSize )) {
        MY_LOGD("Fov is not found");
    }
    else {
        pShotInfo->setSizeSrc(sel.mState.mDualCamDedicatedYuvSize);
        MY_LOGI("Apply FOV size, SensorSize(%dx%d), Fov(%dx%d), "
            , sel.mState.mSensorSize.w, sel.mState.mSensorSize.h
            , sel.mState.mDualCamDedicatedYuvSize.w, sel.mState.mDualCamDedicatedYuvSize.h);
    }
    MY_LOGD("Set multicam feature, mode:%d", sel.mState.mMultiCamFeatureMode);
    pShotInfo->setMulitCamFeatureMode(sel.mState.mMultiCamFeatureMode);

    return true;
}
