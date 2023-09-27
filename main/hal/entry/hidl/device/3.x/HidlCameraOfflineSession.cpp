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

#include "HidlCameraOfflineSession.h"
#include "MyUtils.h"
#include <cutils/properties.h>
#include <mtkcam/utils/std/ULog.h> // will include <log/log.h> if LOG_TAG was defined

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
//
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::hidl_dev3;
// using namespace NSCam::v3::pipeline::model;
using namespace NSCam::Utils;
using namespace NSCam::Utils::ULog;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s::%s] " fmt, getLogPrefix().c_str(), __FUNCTION__, ##arg)
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
HidlCameraOfflineSession::
HidlCameraOfflineSession(const ::android::sp<ICamera3OfflineSession>& session)
    : ICameraOfflineSession()
    , mSession(session)
    , mLogPrefix(std::to_string(session->getInstanceId())+"-hidl-offSess")
    , mLogLevel(0)
    , mCameraDeviceCallback()
    , mResultMetadataQueue()
    // , mStateLog()
    // , mAppStreamManagerErrorState(std::make_shared<EventLogPrinter>(15, 25))
    // , mAppStreamManagerWarningState(std::make_shared<EventLogPrinter>(25, 15))
    // , mAppStreamManagerDebugState(std::make_shared<EventLogPrinter>())
    // , mConfigTimestamp(0)
{
    MY_LOGW("Not implement");
}


/******************************************************************************
 *
 ******************************************************************************/
HidlCameraOfflineSession::
~HidlCameraOfflineSession()
{
    MY_LOGW("Not implement");
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraOfflineSession::
create(
    const ::android::sp<ICamera3OfflineSession>& session
) -> HidlCameraOfflineSession*
{
    auto pInstance = new HidlCameraOfflineSession(session);

    if  ( ! pInstance ) {
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraOfflineSession  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraOfflineSession::
setCallback(const sp<V3_5::ICameraDeviceCallback>& callback)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    MY_LOGW("Not implement");
    auto sessionCallbacks = std::make_shared<v3::CameraDevice3SessionCallback>();

    mCameraDeviceCallback = callback;
    mSession->setCallback(sessionCallbacks);

    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraOfflineSession::
getCaptureResultMetadataQueue(getCaptureResultMetadataQueue_cb _hidl_cb)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    _hidl_cb(*mResultMetadataQueue->getDesc());
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraOfflineSession::
close()
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);

    MY_LOGW("Not implement");

    return Void();
}