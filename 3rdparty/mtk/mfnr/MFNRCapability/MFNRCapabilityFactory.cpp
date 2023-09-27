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
#define LOG_TAG "MFNRCapabilityFactory"

#include <mtkcam/utils/std/ULog.h>
#include <stdlib.h>

#include "../inc/IMFNRCapability.h"
#include "MFNRCapability_default.h"
#include "MFNRCapability_v1_4.h"
#include "MFNRCapability_v2_0.h"
#include "MFNRCapability_v2_1.h"
#include "MFNRCapability_v2_5.h"
#include "MFNRCapability_v3_0.h"
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

std::shared_ptr<IMFNRCapability> IMFNRCapabilityFactory::createCapability()
{
    MY_LOGD("%s", __FUNCTION__);
    sp<IMfllCore> pCore = IMfllCore::createInstance();

    if (pCore == nullptr) {
        MY_LOGE("create MfllCore failed!! cannot create Capability");
        return nullptr;
    }

    auto CoreVersion = MFLL_MAKE_REVISION(MFLL_MAJOR_VER(pCore->getVersion()), MFLL_MINOR_VER(pCore->getVersion()), 0);

    switch (CoreVersion) {
        case MFLL_MAKE_REVISION(1, 4, 0):
            return std::make_shared<MFNRCapability_v1_4>();
        case MFLL_MAKE_REVISION(2, 0, 0):
            return std::make_shared<MFNRCapability_v2_0>();
        case MFLL_MAKE_REVISION(2, 1, 0):
            return std::make_shared<MFNRCapability_v2_1>();
        case MFLL_MAKE_REVISION(2, 5, 0):
            return std::make_shared<MFNRCapability_v2_5>();
        case MFLL_MAKE_REVISION(3, 0, 0):
            return std::make_shared<MFNRCapability_v3_0>();
        case MFLL_MAKE_REVISION(4, 0, 0):
            return std::make_shared<MFNRCapability_v4_0>();
        default:
            MY_LOGE("MFNRCapability is not support for mfnrcore(%s)", pCore->getVersionString().c_str());
            return nullptr;
    }

    return nullptr;
}
