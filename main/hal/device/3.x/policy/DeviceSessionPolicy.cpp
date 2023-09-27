/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include "DeviceSessionPolicy.h"
// #include <mtkcam3/main/hal/device/3.x/policy/IDeviceSessionPolicy.h>
#include <mtkcam/utils/std/ULog.h> // will include <log/log.h> if LOG_TAG was defined

using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::device::policy;
using namespace NSCam::Utils::ULog;


#define ThisNamespace       DeviceSessionPolicy

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_DEBUG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mStaticInfo->mDebugPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_WARN(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mStaticInfo->mWarningPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR_ULOG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mStaticInfo->mErrorPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mStaticInfo->mErrorPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_LOGV(...)                MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...)                MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...)                MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...)                MY_WARN (W, __VA_ARGS__)
#define MY_LOGE(...)                MY_ERROR_ULOG(E, __VA_ARGS__)
#define MY_LOGA(...)                MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)

//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
/******************************************************************************
 *
 ******************************************************************************/

#define CHECK_NOT_NULLPTR(_x_, _errmsg_) \
    do { \
        if ( CC_UNLIKELY(_x_==nullptr) ) {  \
            MY_LOGE("%s", _errmsg_);        \
            return -ENODEV;                 \
        }                                   \
    } while (0);


#define CHECK_OK(_x_, _errmsg_) \
    do { \
        auto result = _x_;                  \
        if ( CC_UNLIKELY(result != 0) ) {   \
            MY_LOGE("%s", _errmsg_);        \
            return result;                  \
        }                                   \
    } while (0);

/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
DeviceSessionPolicy(
    CreationParams const& creationParams
)
    : IDeviceSessionPolicy()
    , mStaticInfo(creationParams.mStaticInfo)
    , mInstanceName{std::to_string(creationParams.mStaticInfo->mInstanceId) + ":DeviceSessionPolicy"}
    , mConfigInfo(creationParams.mConfigInfo)
{

}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
~DeviceSessionPolicy()
{
    //
    MY_LOGD("destructor of devicesessionpolicy");
    NSCam::Utils::CamProfile profile(__FUNCTION__, mInstanceName.c_str());
    {
        std::lock_guard<std::mutex> _l(mConfigPolicyLock);
        if  ( mConfigPolicy != nullptr ) {
            mConfigPolicy->destroy();
            mConfigPolicy = nullptr;
            profile.print("AppConfigUtil -");
        }
    }
    //
    {
        std::lock_guard<std::mutex> _l(mRequestPolicyLock);
        if  ( mRequestPolicy != nullptr ) {
            mRequestPolicy->destroy();
            mRequestPolicy = nullptr;
            profile.print("AppRequestUtil -");
        }
    }

}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
initialize() -> int
{
    NSCam::v3::Utils::CreationInfo creationInfo{
        .mInstanceId            = mStaticInfo->mInstanceId,
        .mCameraDeviceCallback  = mStaticInfo->mCameraDeviceCallback.lock(),
        .mMetadataProvider      = mStaticInfo->mMetadataProvider,
        .mPhysicalMetadataProviders = mStaticInfo->mPhysicalMetadataProviders,
        .mMetadataConverter     = mStaticInfo->mMetadataConverter,
        .mErrorPrinter          = mStaticInfo->mErrorPrinter,
        .mWarningPrinter        = mStaticInfo->mWarningPrinter,
        .mDebugPrinter          = mStaticInfo->mDebugPrinter,
        // .mSupportBufferManagement = false,
        // .mImageConfigMapLock    = mImageConfigMapLock,
        // .mImageConfigMap        = mImageConfigMap,
        // .mMetaConfigMap         = mMetaConfigMap,
    };
    //--------------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> _l(mConfigPolicyLock);

        if  ( mConfigPolicy != nullptr ) {
            MY_LOGE("mAppConfigUtil:%p != 0 while initialize", mConfigPolicy.get());
            mConfigPolicy->destroy();
            mConfigPolicy = nullptr;
        }
        mConfigPolicy = IAppConfigUtil::create(creationInfo);
        if  ( mConfigPolicy == nullptr ) {
            MY_LOGE("IAppConfigUtil::create");
        }
    }
    //--------------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> _l(mRequestPolicyLock);

        if  ( mRequestPolicy != nullptr ) {
            MY_LOGE("mRequestPolicy:%p != 0 while initialize", mRequestPolicy.get());
            mRequestPolicy->destroy();
            mRequestPolicy = nullptr;
        }
        mRequestPolicy = IAppRequestUtil::create(creationInfo);
        if  ( mRequestPolicy == nullptr ) {
            MY_LOGE("IAppRequestUtil::create");
            return NO_INIT;
        }
    }
    //--------------------------------------------------------------------------

    return 0;
}




/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
evaluateConfiguration(
    ConfigurationInputParams const* in,
    ConfigurationOutputParams* out
) -> int
{
    CHECK_NOT_NULLPTR(in,  "Invalid input arguments");
    CHECK_NOT_NULLPTR(out, "Invalid output arguments");

    // preprocess before evaluation
    // CHECK_OK( onEvaluateSessionParams(in->sessionParams, out->configurationSessionParams) );

    // evaluation to generate output params, might integrate features flow
    CHECK_OK( onEvaluateConfiguration(in, out), "onEvaluateConfiguration" );

    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
evaluateRequest(
    RequestInputParams const* in,
    RequestOutputParams* out
) -> int
{
    CHECK_NOT_NULLPTR(in,  "Invalid input arguments");
    CHECK_NOT_NULLPTR(out, "Invalid output arguments");

    CHECK_OK( onEvaluateRequest(in, out), "onEvaluateRequest" );

    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
processResult(
    // ResultInputParams const* in,
    // ResultInOutputParams* out
    ResultParams* params
) -> int
{
    CHECK_NOT_NULLPTR(params,  "Invalid arguments");

    CHECK_OK( onProcessResult(params), "onProcessResult" );

    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
destroy() -> void
{
    MY_LOGW("not implement yet.");
}

/******************************************************************************
 *
 ******************************************************************************/
// auto
// ThisNamespace::
// onEvaluateSessionParams(
//     const camera_metadata *sessionParams
// ) -> int
// {

// }


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onEvaluateConfiguration(
    ConfigurationInputParams const* in,
    ConfigurationOutputParams* out
) -> int
{
    CHECK_NOT_NULLPTR(in,  "Invalid input arguments");
    CHECK_NOT_NULLPTR(out, "Invalid output arguments");

    int result = OK;
    {
        std::lock_guard<std::mutex> _l(mConfigPolicyLock);
        CHECK_NOT_NULLPTR(mConfigPolicy,  "Invalid ConfigPolicy");
        //
        MY_LOGD("[hidldc] in->requestedConfiguration=%p, out->resultConfiguration=%p, out->pipelineCfgParams=%p",
                in->requestedConfiguration, out->resultConfiguration, out->pipelineCfgParams);
        result = mConfigPolicy->beginConfigureStreams(
            *(in->requestedConfiguration),
            *(out->resultConfiguration),
            *(out->pipelineCfgParams)
        );
        if (result == OK) {
            mConfigPolicy->getConfigMap(*(out->imageConfigMap), *(out->metaConfigMap));
            MY_LOGD("[hidldc] out->imageConfigMap->size()=%zu, out->metaConfigMap->size()=%zu",
                        out->imageConfigMap->size(), out->metaConfigMap->size());
        }
        //
    }

    return result;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onEvaluateRequest(
    RequestInputParams const* in,
    RequestOutputParams* out
) -> int
{
    CHECK_NOT_NULLPTR(in,  "Invalid input arguments");
    CHECK_NOT_NULLPTR(out, "Invalid output arguments");

    int result = OK;
    {
        std::lock_guard<std::mutex> _l(mRequestPolicyLock);
        CHECK_NOT_NULLPTR(mRequestPolicy,  "Invalid RequestPolicy");
        //
        ImageConfigMap imgCfgMap;
        MetaConfigMap metaCfgMap;
        mConfigPolicy->getConfigMap(imgCfgMap, metaCfgMap);

        result = mRequestPolicy->createRequests(
            *(in->requests),
            *(out->requests),
            &imgCfgMap,
            &metaCfgMap
        );
    }

    return result;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
onProcessResult(ResultParams* params __unused) -> int
{
    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/

