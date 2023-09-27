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

#include "HidlCameraDeviceSession.h"
#include "MyUtils.h"
#include <TypesWrapper.h>
#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>

#include <string>
#include <mtkcam/utils/std/DebugTimer.h>

#include <system/camera_metadata.h>

// Size of request metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr size_t CAMERA_REQUEST_METADATA_QUEUE_SIZE = 2 << 20 /* 2MB */;
// Size of result metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr size_t CAMERA_RESULT_METADATA_QUEUE_SIZE  = 16 << 20 /* 16MB */;

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
//
using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::hidl_dev3;
using namespace NSCam::hidl_dev3::Utils;
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
HidlCameraDeviceSession::
~HidlCameraDeviceSession()
{
    if (!mIsClosed) {
        /**
         * If cameraserver was killed, both serviceDied and ~session will
         * be called.
         * If ~session was called first, session cannot receive serviceDied,
         * and it will make CameraDevice3Session keep opening until manaul
         * killing camerahalserver.
         * To avoid this, serviceDied should be called if session was
         * deconstructed before be closed.
         */
        MY_LOGW("Session wasn't closed before be deconstructed.");
        if ( CC_UNLIKELY( mSession==nullptr ) ) {
            MY_LOGE("Bad CameraDevice3Session");
        } else {
            auto status = mSession->close();
            if ( CC_UNLIKELY(status!=OK) ) {
                MY_LOGE("HidlCameraDeviceSession close fail");
            }
        }
        mIsClosed = true;
    }
    MY_LOGI("dtor");
}


/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDeviceSession::
HidlCameraDeviceSession(const ::android::sp<ICameraDevice3Session>& session)
    : ICameraDeviceSession()
    , mSession(session)
    , mCameraDeviceCallback(nullptr)
    , mLogPrefix(std::to_string(session->getInstanceId())+"-hidl-session")
    , mIsClosed(false)
    , mBufferHandleCacheMgr()
    , mRequestMetadataQueue()
    , mResultMetadataQueue()
{
    MY_LOGI("ctor");

    mConvertTimeDebug = property_get_int32("vendor.debug.camera.log.convertTime", 0);
    mAutoTestDebug = property_get_bool("vendor.autotesttool.enable", 0);

    if ( CC_UNLIKELY(mSession==nullptr) ) {
        MY_LOGE("session does not exist");
    }
    ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
create(
    const ::android::sp<ICameraDevice3Session>& session
)-> HidlCameraDeviceSession*
{
    auto pInstance = new HidlCameraDeviceSession(session);

    if  ( ! pInstance ) {
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
open(
    const ::android::sp<V3_5::ICameraDeviceCallback>& callback
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    mIsClosed = false;
    //unlink to death notification for existed device callback
    if ( mCameraDeviceCallback != nullptr ) {
        mCameraDeviceCallback->unlinkToDeath(this);
        mCameraDeviceCallback = nullptr;
        ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));
    }

    //link to death notification for device callback
    if ( callback != nullptr ) {
        hardware::Return<bool> linked = callback->linkToDeath(this, (uint64_t)this);
        if (!linked.isOk()) {
            MY_LOGE("Transaction error in linking to mCameraDeviceCallback death: %s", linked.description().c_str());
        } else if (!linked) {
            MY_LOGW("Unable to link to mCameraDeviceCallback death notifications");
        }
        callback->getDebugInfo([this](const auto& info){
            mLinkToDeathDebugInfo = info;
        });
        MY_LOGD("Link death to ICameraDeviceCallback %s", toString(mLinkToDeathDebugInfo).c_str());
    }

    mCameraDeviceCallback = callback;

    auto status = initialize();
    if ( CC_UNLIKELY(status!=OK) ) {
        MY_LOGE("HidlCameraDeviceSession initialization fail");
        return status;
    }

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
initialize() -> ::android::status_t
{
    CAM_TRACE_CALL();
    status_t status = OK;
    {
        MY_LOGI("FMQ request size : %zu, result size : %zu", CAMERA_REQUEST_METADATA_QUEUE_SIZE, CAMERA_RESULT_METADATA_QUEUE_SIZE);
        mRequestMetadataQueue = std::make_unique<RequestMetadataQueue>(
            CAMERA_REQUEST_METADATA_QUEUE_SIZE, false /* non blocking */);
        if  ( !mRequestMetadataQueue || !mRequestMetadataQueue->isValid() ) {
            MY_LOGE("invalid request fmq");
            return BAD_VALUE;
        }
        mResultMetadataQueue = std::make_unique<ResultMetadataQueue>(
            CAMERA_RESULT_METADATA_QUEUE_SIZE, false /* non blocking */);
        if  ( !mResultMetadataQueue || !mResultMetadataQueue->isValid() ) {
            MY_LOGE("invalid result fmq");
            return BAD_VALUE;
        }
    }

    //--------------------------------------------------------------------------
    {
        Mutex::Autolock _l(mBufferHandleCacheMgrLock);

        if  ( mBufferHandleCacheMgr != nullptr ) {
            MY_LOGE("mBufferHandleCacheMgr:%p != 0 while opening", mBufferHandleCacheMgr.get());
            mBufferHandleCacheMgr->destroy();
            mBufferHandleCacheMgr = nullptr;
        }
        mBufferHandleCacheMgr = IBufferHandleCacheMgr::create();
        if  ( mBufferHandleCacheMgr == nullptr ) {
            MY_LOGE("IBufferHandleCacheMgr::create");
            return NO_INIT;
        }
    }

    mSessionCallbacks = createSessionCallbacks();

    status = mSession->open(mSessionCallbacks);

    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
getSafeBufferHandleCacheMgr() const -> ::android::sp<IBufferHandleCacheMgr>
{
    Mutex::Autolock _l(mBufferHandleCacheMgrLock);
    return mBufferHandleCacheMgr;
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
constructDefaultRequestSettings(RequestTemplate type, constructDefaultRequestSettings_cb _hidl_cb)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    CAM_TRACE_CALL();

    V3_2::CameraMetadata metadata;
    if ( CC_UNLIKELY( mSession==nullptr ) ) {
        _hidl_cb(Status::INTERNAL_ERROR, metadata);
        return Void();
    }

    const camera_metadata *templateSettings = mSession->constructDefaultRequestSettings(static_cast<v3::RequestTemplate>(type));
    if ( CC_UNLIKELY( templateSettings==nullptr ) ) {
        _hidl_cb(Status::INTERNAL_ERROR, metadata);
        return Void();
    }

    size_t size = ::get_camera_metadata_size(templateSettings);
    metadata.setToExternal((uint8_t *)templateSettings, size, false/*shouldOwn*/);
    _hidl_cb(Status::OK, metadata);

    return Void();
}




/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
configureStreams(const V3_2::StreamConfiguration& requestedConfiguration __unused, configureStreams_cb _hidl_cb __unused)
{
    return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
configureStreams_3_3(const V3_2::StreamConfiguration& requestedConfiguration __unused, configureStreams_3_3_cb _hidl_cb __unused)
{
    return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
configureStreams_3_4(const V3_4::StreamConfiguration& requestedConfiguration __unused, configureStreams_3_4_cb _hidl_cb __unused)
{
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
configureStreams_3_5(const V3_5::StreamConfiguration& requestedConfiguration, configureStreams_3_5_cb _hidl_cb)
{
    MY_LOGD("+");
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    CAM_TRACE_CALL();

    WrappedHalStreamConfiguration halStreamConfiguration;
    if ( CC_UNLIKELY( mSession==nullptr ) ) {
        MY_LOGE("Bad CameraDevice3Session");
        // ulog
        _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
    }

    NSCam::v3::StreamConfiguration input;
    NSCam::v3::HalStreamConfiguration output;

    // transfer from hidl to mtk
    convertStreamConfigurationFromHidl(requestedConfiguration, input);

    auto status = mSession->configureStreams(input, output);

    auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
    if  ( pBufferHandleCacheMgr == 0 ) {
        MY_LOGE("Bad BufferHandleCacheMgr");
        _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
    }
    pBufferHandleCacheMgr->configBufferCacheMap(requestedConfiguration);

    //transfer from mtk to hidl
    convertHalStreamConfigurationToHidl(output, halStreamConfiguration);
    _hidl_cb(mapToHidlCameraStatus(status), halStreamConfiguration);
    MY_LOGD("-");
    return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
//V3.6 is identical to V3.5
Return<void>
HidlCameraDeviceSession::
configureStreams_3_6(const V3_5::StreamConfiguration& requestedConfiguration, configureStreams_3_6_cb _hidl_cb)
{
    MY_LOGD("+");
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    CAM_TRACE_CALL();

    WrappedHalStreamConfiguration halStreamConfiguration;
    if ( CC_UNLIKELY( mSession==nullptr ) ) {
        MY_LOGE("Bad CameraDevice3Session");
        // ulog
        _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
    }

    NSCam::v3::StreamConfiguration input;
    NSCam::v3::HalStreamConfiguration output;

    // transfer from hidl to mtk
    convertStreamConfigurationFromHidl(requestedConfiguration, input);

    auto status = mSession->configureStreams(input, output);

    auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
    if  ( pBufferHandleCacheMgr == 0 ) {
        MY_LOGE("Bad BufferHandleCacheMgr");
        _hidl_cb(Status::ILLEGAL_ARGUMENT, halStreamConfiguration);
    }
    pBufferHandleCacheMgr->configBufferCacheMap(requestedConfiguration);

    //transfer from mtk to hidl
    convertHalStreamConfigurationToHidl(output, halStreamConfiguration);
    _hidl_cb(mapToHidlCameraStatus(status), halStreamConfiguration);
    MY_LOGD("-");
    return Void();
}



/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
getCaptureRequestMetadataQueue(getCaptureRequestMetadataQueue_cb _hidl_cb)
{
    CAM_TRACE_CALL();
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    _hidl_cb(*mRequestMetadataQueue->getDesc());
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
getCaptureResultMetadataQueue(getCaptureResultMetadataQueue_cb _hidl_cb)
{
    CAM_TRACE_CALL();
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    _hidl_cb(*mResultMetadataQueue->getDesc());
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
processCaptureRequest(const hidl_vec<V3_2::CaptureRequest>& requests, const hidl_vec<BufferCache>& cachesToRemove, processCaptureRequest_cb _hidl_cb)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
    CAM_TRACE_CALL();
    uint32_t numRequestProcessed = 0;

    status_t status = OK;
    // halbufhandler->waithalbufcntavailable
    {
        auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
        if  ( pBufferHandleCacheMgr == 0 ) {
            MY_LOGE("Bad BufferHandleCacheMgr");
            _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
        }

        pBufferHandleCacheMgr->removeBufferCache(cachesToRemove);

        int i = 0;
        hidl_vec<V3_4::CaptureRequest> v34Requests(requests.size());
        for(auto &r : requests) {
            v34Requests[i++] = (V3_4::CaptureRequest &)WrappedCaptureRequest(r);
        }

        std::vector<RequestBufferCache> vBufferCache;
        pBufferHandleCacheMgr->registerBufferCache(v34Requests, vBufferCache);

        // pBufferHandleCacheMgr->dumpState();
        // MY_LOGD("[debug] registerBufferCache done, vBufferCache.size()=%u", vBufferCache.size());

        if ( CC_UNLIKELY( mSession==nullptr ) ) {
            MY_LOGE("Bad CameraDevice3Session");
            _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
        }

        // transfer from hidl to mtk structure
        vector<NSCam::v3::CaptureRequest> captureRequests;
        CameraMetadata metadata_queue_settings;
        auto result = convertAllRequestFromHidl(v34Requests, mRequestMetadataQueue.get(), captureRequests, metadata_queue_settings ,&vBufferCache);

        if ( CC_UNLIKELY( result != OK ) ) {
            MY_LOGE("======== dump all requests ========");
            for ( auto& request : requests) {
                MY_LOGE("%s", toString(request).c_str());
            }
            if (captureRequests.empty()) {
                MY_LOGE("Bad request, don't handle request");
                _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
                return Void();
            }
        }

        status = static_cast<status_t>(mSession->processCaptureRequest(captureRequests, numRequestProcessed));
    }

    _hidl_cb(mapToHidlCameraStatus(status), numRequestProcessed);
    return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
processCaptureRequest_3_4(const hidl_vec<V3_4::CaptureRequest>& requests, const hidl_vec<BufferCache>& cachesToRemove, processCaptureRequest_3_4_cb _hidl_cb)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
    CAM_TRACE_CALL();
    uint32_t numRequestProcessed = 0;

    status_t status = OK;
    // halbufhandler->waithalbufcntavailable
    {
        auto pBufferHandleCacheMgr = getSafeBufferHandleCacheMgr();
        if  ( pBufferHandleCacheMgr == 0 ) {
            MY_LOGE("Bad BufferHandleCacheMgr");
            _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
        }

        pBufferHandleCacheMgr->removeBufferCache(cachesToRemove);

        std::vector<RequestBufferCache> vBufferCache;
        pBufferHandleCacheMgr->registerBufferCache(requests, vBufferCache);

        // pBufferHandleCacheMgr->dumpState();
        // MY_LOGD("[debug] registerBufferCache done, vBufferCache.size()=%u", vBufferCache.size());

        if ( CC_UNLIKELY( mSession == nullptr ) ) {
            MY_LOGE("Bad CameraDevice3Session");
            _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
        }
        // transfer from hidl to mtk structure
        vector<NSCam::v3::CaptureRequest> captureRequests;
        CameraMetadata metadata_queue_settings;
        auto result = convertAllRequestFromHidl(requests, mRequestMetadataQueue.get(), captureRequests, metadata_queue_settings, &vBufferCache);

        if ( CC_UNLIKELY( result != OK ) ) {
            MY_LOGE("======== dump all requests ========");
            for ( auto& request : requests) {
                MY_LOGE("%s", toString(request).c_str());
            }
            if (captureRequests.empty()) {
                MY_LOGE("Bad request, don't handle request");
                _hidl_cb(Status::ILLEGAL_ARGUMENT, numRequestProcessed);
                return Void();
            }
        }

        status = static_cast<status_t>(mSession->processCaptureRequest(captureRequests, numRequestProcessed));
    }

    _hidl_cb(mapToHidlCameraStatus(status), numRequestProcessed);
    return Void();
}

//V3.5
Return<void>
HidlCameraDeviceSession::
signalStreamFlush(const hidl_vec<int32_t>& streamIds, uint32_t streamConfigCounter)
{
    CAM_TRACE_CALL();
    if ( CC_UNLIKELY( mSession==nullptr ) )
        MY_LOGE("Bad CameraDevice3Session");
    std::vector<int32_t> vStreamIds;
    for(auto &id: streamIds)
    {
        vStreamIds.push_back(id);
    }
    mSession->signalStreamFlush(vStreamIds, streamConfigCounter);

    return Void();
}

Return<void>
HidlCameraDeviceSession::
isReconfigurationRequired(const hidl_vec<uint8_t>& oldSessionParams, const hidl_vec<uint8_t>& newSessionParams, isReconfigurationRequired_cb _hidl_cb)
{
    CAM_TRACE_CALL();
    const camera_metadata *oldMetadata;
    const camera_metadata *newMetadata;
    oldMetadata = reinterpret_cast<const camera_metadata_t*> (oldSessionParams.data());
    newMetadata = reinterpret_cast<const camera_metadata_t*> (newSessionParams.data());

    bool needReconfig = true;
    auto res = mSession->isReconfigurationRequired(*oldMetadata, *newMetadata, needReconfig);


    _hidl_cb(mapToHidlCameraStatus(res), needReconfig);

    return Void();
}

//V3.6
Return<void>
HidlCameraDeviceSession::
switchToOffline(const hidl_vec<int32_t>& streamsToKeep __unused, switchToOffline_cb _hidl_cb)
{
    CAM_TRACE_CALL();
    MY_LOGW("Not implement, return ILLEGAL_ARGUMENT.");

    V3_6::CameraOfflineSessionInfo info;
    sp<V3_6::ICameraOfflineSession> session;

    // VTS will check halStreamConfig.streams[0].supportOffline
    // if it's not supported, offline session and info will not be accessed.
    _hidl_cb(Status::ILLEGAL_ARGUMENT, info, session);

    return Void();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<Status>
HidlCameraDeviceSession::
flush()
{
    MY_LOGD("+");
    CAM_TRACE_CALL();
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    if ( CC_UNLIKELY( mSession==nullptr ) ) {
        MY_LOGE("Bad CameraDevice3Session");
        return Status::INTERNAL_ERROR;
    }

    auto status = mSession->flush();

    MY_LOGD("-");
    return mapToHidlCameraStatus(status);
}

/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDeviceSession::
close()
{
    CAM_TRACE_CALL();
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    mIsClosed = true;

    if ( CC_UNLIKELY( mSession==nullptr ) )
        MY_LOGE("Bad CameraDevice3Session");

    auto status = mSession->close();
    if ( CC_UNLIKELY(status!=OK) ) {
        MY_LOGE("HidlCameraDeviceSession close fail");
    }

    //unlink to death notification for existed device callback
    {
        Mutex::Autolock _l(mCameraDeviceCallbackLock);
        if ( mCameraDeviceCallback != nullptr ) {
            mCameraDeviceCallback->unlinkToDeath(this);
            mCameraDeviceCallback = nullptr;
        }
    }
    ::memset(&mLinkToDeathDebugInfo, 0, sizeof(mLinkToDeathDebugInfo));

    //
    {
        Mutex::Autolock _l(mBufferHandleCacheMgrLock);
        if  ( mBufferHandleCacheMgr != nullptr ) {
            mBufferHandleCacheMgr->destroy();
            mBufferHandleCacheMgr = nullptr;
        }
    }
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
void
HidlCameraDeviceSession::
serviceDied(uint64_t cookie __unused, const wp<hidl::base::V1_0::IBase>& who __unused)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    if (cookie != (uint64_t)this) {
        MY_LOGW("Unexpected ICameraDevice3Session serviceDied cookie 0x%" PRIx64 ", expected %p", cookie, this);
    }

    MY_LOGW("ICameraDevice3Session has died - %s; removing it - cookie:0x%" PRIx64 " this: %p", toString(mLinkToDeathDebugInfo).c_str(), cookie, this);

    //force to close camera
    MY_LOGW("CameraService has died, force to close camera device %d", mSession->getInstanceId());

    flush();
    close();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
createSessionCallbacks()
-> std::shared_ptr<v3::CameraDevice3SessionCallback>
{
    CAM_TRACE_CALL();
    auto sessionCallbacks = std::make_shared<v3::CameraDevice3SessionCallback>();
    if ( sessionCallbacks != nullptr )
    {
        sessionCallbacks->processCaptureResult = v3::ProcessCaptureResultFunc(
            [this](std::vector<v3::CaptureResult>& results) {
                processCaptureResult(results);
            }
        );
        sessionCallbacks->notify = v3::NotifyFunc(
            [this](const std::vector<v3::NotifyMsg>& msgs) {
                notify(msgs);
            }
        );
        sessionCallbacks->requestStreamBuffers = v3::RequestStreamBuffersFunc(
            [this](const std::vector<v3::BufferRequest>& vBufferRequests,
                    std::vector<v3::StreamBufferRet>* pvBufferReturns) {
                return requestStreamBuffers(vBufferRequests, pvBufferReturns);
            }
        );
        sessionCallbacks->returnStreamBuffers = v3::ReturnStreamBuffersFunc(
            [this](const std::vector<v3::StreamBuffer>& buffers) {
                returnStreamBuffers(buffers);
            }
        );
        return sessionCallbacks;
    }
    MY_LOGE("fail to create CameraDevice3SessionCallback");
    return nullptr;
}



/******************************************************************************
 *
 ******************************************************************************/

auto
HidlCameraDeviceSession::
processCaptureResult(std::vector<v3::CaptureResult>& results)
-> void
{
    CAM_TRACE_CALL();
    std::vector<V3_4::CaptureResult> hidl_results;

    status_t res = convertToHidlCaptureResults(results, &hidl_results);
    if (res != OK) {
        MY_LOGE("Converting hal capture result failed: %s(%d)",
        strerror(-res), res);
    }

    if  ( auto pResultMetadataQueue = mResultMetadataQueue.get() )
    {
        for(size_t i = 0; i<hidl_results.size() ;i++)
        {
            auto& hidl_result = hidl_results[i];
            //write logical meta into FMQ
            if(hidl_result.v3_2.result.size() > 0)
            {
                if( CC_LIKELY(pResultMetadataQueue->write(
                                hidl_result.v3_2.result.data(),
                                hidl_result.v3_2.result.size())))
                {
                    hidl_result.v3_2.fmqResultSize = hidl_result.v3_2.result.size();
                    hidl_result.v3_2.result = hidl_vec<uint8_t>();  // resize(0)
                }
                else
                {
                    hidl_result.v3_2.fmqResultSize = 0;
                    MY_LOGW("fail to write meta to mResultMetadataQueue, data=%p, size=%zu",
                        hidl_result.v3_2.result.data(), hidl_result.v3_2.result.size());
                }
            }

            // write physical meta into FMQ
            if (hidl_result.physicalCameraMetadata.size() > 0)
            for(size_t i = 0; i<hidl_result.physicalCameraMetadata.size() ;i++)
            {
                if( CC_LIKELY(pResultMetadataQueue->write(
                                    hidl_result.physicalCameraMetadata[i].metadata.data(),
                                    hidl_result.physicalCameraMetadata[i].metadata.size())) )
                {
                    hidl_result.physicalCameraMetadata[i].fmqMetadataSize = hidl_result.physicalCameraMetadata[i].metadata.size();
                    hidl_result.physicalCameraMetadata[i].metadata = hidl_vec<uint8_t>();  // resize(0)
                }
                else
                {
                    MY_LOGW("fail to write physical meta to mResultMetadataQueue, data=%p, size=%zu",
                    hidl_result.physicalCameraMetadata[i].metadata.data(), hidl_result.physicalCameraMetadata[i].metadata.size());
                }
            }
        }
    }
    else
    {
        MY_LOGE("TODO: callback meta by non-FMQ");
    }

    // call back
    hidl_vec<V3_4::CaptureResult> vecCaptureResult;
    vecCaptureResult.setToExternal(hidl_results.data(), hidl_results.size());
    if (mCameraDeviceCallback != nullptr) {
        auto result = mCameraDeviceCallback->processCaptureResult_3_4(vecCaptureResult);
        if (CC_UNLIKELY(!result.isOk())) {
            MY_LOGE("Transaction error in ICameraDeviceCallback::processCaptureResult_3_4: %s", result.description().c_str());
        }
    } else {
        std::string log = "mCameraDeviceCallback is null, drop requests:";
        for (auto const& r : hidl_results) {
            log += " " + std::to_string(r.v3_2.frameNumber);
        }
        MY_LOGE("%s", log.c_str());
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
notify(const std::vector<v3::NotifyMsg>& msgs)
-> void
{
    CAM_TRACE_CALL();
    if  ( ! msgs.empty() ) {
        // convert hal NotifyMsg to hidl
        std::vector<V3_2::NotifyMsg> hidl_msgs;
        auto res = convertToHidlNotifyMsgs(msgs, &hidl_msgs);
        if (OK != res)
        {
            MY_LOGE("Fail to convert hal NotifyMsg to hidl");
            return;
        }
        hidl_vec<NotifyMsg> vecNotifyMsg;
        vecNotifyMsg.setToExternal(hidl_msgs.data(), hidl_msgs.size());
        if (mCameraDeviceCallback != nullptr) {
            auto result = mCameraDeviceCallback->notify(vecNotifyMsg);
            if (CC_UNLIKELY(!result.isOk())) {
                MY_LOGE("Transaction error in ICameraDeviceCallback::notify: %s", result.description().c_str());
            }
        } else {
            std::string log = "mCameraDeviceCallback is null, drop requests:";
            for (auto const& m : hidl_msgs) {
                if (m.type == V3_2::MsgType::ERROR) {
                    log += " " + std::to_string(m.msg.error.frameNumber);
                } else if (m.type == V3_2::MsgType::SHUTTER) {
                    log += " " + std::to_string(m.msg.shutter.frameNumber);
                }
            }
            MY_LOGE("%s", log.c_str());
        }
    }
}



/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
requestStreamBuffers(
    const std::vector<v3::BufferRequest>& vBufferRequests __unused,
    std::vector<v3::StreamBufferRet>* pvBufferReturns __unused
) -> v3::BufferRequestStatus
{
    CAM_TRACE_CALL();
    // TODO: implement, and remember to cache buffer handle"
    // V3_5::BufferRequestStatus status;
    // BufferRequestStatus hidl_status;
    // hidl_vec<StreamBufferRet> stream_buffer_returns;
    // auto cb_status = hidl_device_callback_->requestStreamBuffers(
    //     hidl_buffer_requests, [&hidl_status, &stream_buffer_returns](
    //                             BufferRequestStatus status_ret,
    //                             const hidl_vec<StreamBufferRet>& buffer_ret) {
    //     hidl_status = status_ret;
    //     stream_buffer_returns = std::move(buffer_ret);
    // });
    // if (!cb_status.isOk()) {
    //     ALOGE("%s: Transaction request stream buffers error: %s", __FUNCTION__,
    //           cb_status.description().c_str());
    //     return BufferRequestStatus::kFailedUnknown;
    // }
    return v3::BufferRequestStatus::FAILED_UNKNOWN;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
returnStreamBuffers(const std::vector<v3::StreamBuffer>& buffers __unused)
-> void
{
    CAM_TRACE_CALL();

}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
convertStreamConfigurationFromHidl(
    const V3_5::StreamConfiguration& srcStreams,
    NSCam::v3::StreamConfiguration& dstStreams
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    for ( auto& stream : srcStreams.v3_4.streams) {
        MY_LOGD("%s", toString(stream).c_str());
    }
    MY_LOGD("operationMode=%u", srcStreams.v3_4.operationMode);

    timer.start();

    int size = srcStreams.v3_4.streams.size();
    dstStreams.streams.resize(size);
    for ( int i=0 ; i<size ; i++ ){
        auto& srcStream = srcStreams.v3_4.streams[i];
        auto& dstStream = dstStreams.streams[i];
        //
        dstStream.id            = srcStream.v3_2.id;
        dstStream.streamType    = static_cast<v3::StreamType> (srcStream.v3_2.streamType);
        dstStream.width         = srcStream.v3_2.width;
        dstStream.height        = srcStream.v3_2.height;
        dstStream.format        = static_cast<android_pixel_format_t> (srcStream.v3_2.format);
        dstStream.usage         = srcStream.v3_2.usage;
        dstStream.dataSpace     = static_cast<android_dataspace_t> (srcStream.v3_2.dataSpace);
        dstStream.rotation      = static_cast<v3::StreamRotation> (srcStream.v3_2.rotation);
        //
        dstStream.physicalCameraId  = static_cast<std::string> (srcStream.physicalCameraId);
        dstStream.bufferSize        = srcStream.bufferSize;
    }

    dstStreams.operationMode        = static_cast<v3::StreamConfigurationMode> (srcStreams.v3_4.operationMode);
    dstStreams.sessionParams        = reinterpret_cast<const camera_metadata_t*> (srcStreams.v3_4.sessionParams.data());
    dstStreams.streamConfigCounter  = srcStreams.streamConfigCounter;  // V3_5


    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert StreamConfiguration From Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    for ( auto& stream : dstStreams.streams) {
        MY_LOGD("%s", toString(stream).c_str());
    }
    MY_LOGD("operationMode=%u", dstStreams.operationMode);
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
convertHalStreamConfigurationToHidl(
    const NSCam::v3::HalStreamConfiguration& srcStreams,
    V3_6::HalStreamConfiguration& dstStreams
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    int size = srcStreams.streams.size();
    dstStreams.streams.resize(size);

    for ( int i=0 ; i<size ; i++ ) {
        auto& srcStream                         = srcStreams.streams[i];
        auto& dstStream                         = dstStreams.streams[i];
        //
        dstStream.v3_4.v3_3.v3_2.id             = srcStream.id;
        dstStream.v3_4.v3_3.v3_2.overrideFormat = static_cast<::android::hardware::graphics::common::V1_0::PixelFormat> (srcStream.overrideFormat);
        dstStream.v3_4.v3_3.v3_2.producerUsage  = srcStream.producerUsage;
        dstStream.v3_4.v3_3.v3_2.consumerUsage  = srcStream.consumerUsage;
        dstStream.v3_4.v3_3.v3_2.maxBuffers     = srcStream.maxBuffers;
        dstStream.v3_4.v3_3.overrideDataSpace   = srcStream.overrideDataSpace;
        dstStream.v3_4.physicalCameraId         = srcStream.physicalCameraId;
        dstStream.supportOffline                = srcStream.supportOffline;
        MY_LOGD("HalStream.id(%d) HalStream.maxBuffers(%u)", dstStream.v3_4.v3_3.v3_2.id, dstStream.v3_4.v3_3.v3_2.maxBuffers);
        MY_LOGD("srcHalStream.id(%d) srcHalStream.maxBuffers(%u)", srcStream.id, srcStream.maxBuffers);
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert HAL StreamConfiguration to Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
convertAllRequestFromHidl(
    const hidl_vec<V3_4::CaptureRequest>& inRequests,
    RequestMetadataQueue* pRequestFMQ,
    std::vector<NSCam::v3::CaptureRequest>& outRequests,
    CameraMetadata& metadata_queue_settings,
    std::vector<RequestBufferCache>* requestBufferCache
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    status_t result = OK;

    for ( auto& in : inRequests ) {
        v3::CaptureRequest out = {};
        RequestBufferCache* currentBufferCache = {};

        for (auto& bufferCache : *requestBufferCache) {
            if (bufferCache.frameNumber == in.v3_2.frameNumber) {
                currentBufferCache = &bufferCache;
                break;
            }
        }
        if(CC_LIKELY(OK == convertToHalCaptureRequest(in, pRequestFMQ, &out, metadata_queue_settings, currentBufferCache)))
        {
            outRequests.push_back(std::move(out));
        }
        else
        {
            MY_LOGE("convert to Hal request failed, dump reqeust info : %s", toString(in).c_str());
            result = BAD_VALUE;
        }
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert all Hidl CaptureRequest to HAL : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return result;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDeviceSession::
convertToHalCaptureRequest(
    const V3_4::CaptureRequest& hidl_request,
    RequestMetadataQueue* request_metadata_queue,
    NSCam::v3::CaptureRequest* hal_request,
    CameraMetadata& metadata_queue_settings,
    RequestBufferCache* requestBufferCache
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    // NSCam::Utils::DebugTimer timer;
    // timer.start();

    if (hal_request == nullptr)
    {
        MY_LOGE("hal_request is nullptr");
        return BAD_VALUE;
    }
    hal_request->frameNumber = hidl_request.v3_2.frameNumber;

    //1. ConvertToHalMetadata: logical metadata
    status_t res = convertToHalMetadata(
        hidl_request.v3_2.fmqSettingsSize, request_metadata_queue,
        hidl_request.v3_2.settings, metadata_queue_settings,
        const_cast<camera_metadata **> (&(hal_request->settings)));

    if (res != OK) {
        MY_LOGE("Converting metadata failed: %s(%d)",
            strerror(-res), res);
        return res;
    }

    //2. Convert inputBuffers from HIDL
    if (hidl_request.v3_2.inputBuffer.buffer != nullptr) {
        NSCam::v3::StreamBuffer hal_inputBuffer = {};
        const BufferHandle* importedBufferHandle = &(requestBufferCache->bufferHandleMap.valueFor(hidl_request.v3_2.inputBuffer.streamId));
        res = convertToHalStreamBuffer(hidl_request.v3_2.inputBuffer, &hal_inputBuffer, importedBufferHandle);
        if (res != OK) {
            MY_LOGE("Converting hal stream buffer failed: %s(%d)",
                strerror(-res), res);
        return res;
        }
        hal_request->inputBuffer = hal_inputBuffer;
    }

    //3. Convert outputBuffers from HIDL
    for (auto& buffer : hidl_request.v3_2.outputBuffers) {
        NSCam::v3::StreamBuffer hal_outputBuffer = {};
        const BufferHandle* importedBufferHandle = &(requestBufferCache->bufferHandleMap.valueFor(buffer.streamId));
        status_t res = convertToHalStreamBuffer(buffer, &hal_outputBuffer, importedBufferHandle);
        if (res != OK) {
            MY_LOGE("Converting hal stream buffer failed: %s(%d)",
                strerror(-res), res);
        return res;
        }

        hal_request->outputBuffers.push_back(std::move(hal_outputBuffer));
    }

    //4. ConvertToHalMetadata: physical metadata
    for (auto& hidl_physical_settings : hidl_request.physicalCameraSettings) {
        NSCam::v3::PhysicalCameraSetting hal_PhysicalCameraSetting;
        hal_PhysicalCameraSetting.physicalCameraId = hidl_physical_settings.physicalCameraId;

        res = convertToHalMetadata(
            hidl_physical_settings.fmqSettingsSize, request_metadata_queue,
            hidl_physical_settings.settings, metadata_queue_settings,
            const_cast<camera_metadata **> (&(hal_PhysicalCameraSetting.settings)));
        if (res != OK) {
                MY_LOGE("Converting to HAL physical metadata failed: %s(%d)",
                strerror(-res), res);
                return res;
        }
        hal_request->physicalCameraSettings.push_back(std::move(hal_PhysicalCameraSetting));
    }

    // timer.stop();
    // MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert Hidl CaptureRequest to HAL : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());
    return OK;
}

auto
HidlCameraDeviceSession::
convertToHalMetadata(
uint32_t message_queue_setting_size,
RequestMetadataQueue* request_metadata_queue,
const CameraMetadata& request_settings,
CameraMetadata& metadata_queue_settings,
camera_metadata** hal_metadata
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    if (hal_metadata == nullptr)
    {
        MY_LOGE("hal_metadata is nullptr.");
        return BAD_VALUE;
    }

    const camera_metadata_t *metadata = nullptr;

    if (message_queue_setting_size == 0)
    {
        // Use the settings in the request.
        if (request_settings.size() != 0)
        {
            metadata =
                reinterpret_cast<const camera_metadata_t *>(request_settings.data());
        }
    }
    else
    {
        // Read the settings from request metadata queue.
        if (request_metadata_queue == nullptr) {
            MY_LOGE("request_metadata_queue is nullptr");
            return BAD_VALUE;
        }

        metadata_queue_settings.resize(message_queue_setting_size);
        bool success = false;
        if (message_queue_setting_size <= request_metadata_queue->availableToRead()) {
            success = request_metadata_queue->read(metadata_queue_settings.data(),
                                                   message_queue_setting_size);
        }
        if (!success) {
            MY_LOGE("Failed to read from request metadata queue, size=%" PRIu32 ", available=%zu",
                    message_queue_setting_size, request_metadata_queue->availableToRead());
            return BAD_VALUE;
        }

        metadata = reinterpret_cast<const camera_metadata_t *>(
            metadata_queue_settings.data());
    }

    if (metadata == nullptr)
    {
        *hal_metadata = NULL;
        return OK;
    }

    printAutoTestLog(metadata, false);

    *hal_metadata = const_cast<camera_metadata *>(metadata);

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert Metadata from Hidl: %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return OK;
}

auto
HidlCameraDeviceSession::
convertToHalStreamBuffer(
const V3_2::StreamBuffer& hidl_buffer,
NSCam::v3::StreamBuffer* hal_buffer,
const Utils::BufferHandle* importedBufferHandle
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    if (hal_buffer == nullptr) {
        MY_LOGE("hal_buffer is nullptr.");
        return BAD_VALUE;
    }

    hal_buffer->streamId = hidl_buffer.streamId;
    hal_buffer->bufferId = hidl_buffer.bufferId;
    hal_buffer->buffer = importedBufferHandle->bufferHandle;
    hal_buffer->appBufferHandleHolder = importedBufferHandle->appBufferHandleHolder;
    hal_buffer->status = static_cast<NSCam::v3::BufferStatus>(hidl_buffer.status);
    if (hidl_buffer.acquireFence.getNativeHandle() != nullptr) {
        hal_buffer->acquireFenceOp.type = NSCam::v3::FenceType::HDL;
        hal_buffer->acquireFenceOp.hdl = hidl_buffer.acquireFence.getNativeHandle();
        if (hal_buffer->acquireFenceOp.hdl == nullptr) {
            MY_LOGE("Cloning Hal stream buffer acquire fence failed");
        }
    }
    hal_buffer->releaseFenceOp.hdl = nullptr;

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert StreamBuffer from Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return OK;
}

auto
HidlCameraDeviceSession::
convertToHidlCaptureResults(
const std::vector<v3::CaptureResult>& hal_captureResults,
std::vector<V3_4::CaptureResult>* hidl_captureResults
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    for ( auto& hal_result : hal_captureResults ) {
        V3_4::CaptureResult hidl_Result = {};
        convertToHidlCaptureResult(hal_result, &hidl_Result);
        hidl_captureResults->push_back(std::move(hidl_Result));
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert all captureResults to Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return OK;
}

auto
HidlCameraDeviceSession::
convertToHidlCaptureResult(
const v3::CaptureResult& hal_captureResult,
V3_4::CaptureResult* hidl_captureResult
) -> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    //Convert the result metadata, and add it to CaptureResult
    if (hal_captureResult.resultSize !=0)
    {
        printAutoTestLog(hal_captureResult.result, true);
        hidl_vec<uint8_t> hidl_meta;
        hidl_meta.setToExternal((uint8_t *)hal_captureResult.result, hal_captureResult.resultSize);
        hidl_captureResult->v3_2.result = std::move(hidl_meta);
    }

    //2. Convert outputBuffers to HIDL
    if( !hal_captureResult.outputBuffers.empty())
    {
        hidl_vec<StreamBuffer> vOutBuffers;
        size_t Psize = hal_captureResult.outputBuffers.size();
        vOutBuffers.resize(Psize);

        for (size_t i = 0; i < Psize; i++) {
            status_t res = convertToHidlStreamBuffer(hal_captureResult.outputBuffers[i], &vOutBuffers[i], hal_captureResult.frameNumber);
            if (res != OK) {
                MY_LOGE("Converting hidl stream buffer failed: %s(%d)",
                strerror(-res), res);
                return res;
            }
        }
        hidl_captureResult->v3_2.outputBuffers = std::move(vOutBuffers);
    }
    else
    {
        hidl_captureResult->v3_2.outputBuffers = hidl_vec<V3_2::StreamBuffer>();
    }
    //3. Convert inputBuffer to HIDL
    if(hal_captureResult.inputBuffer.streamId != v3::NO_STREAM)
    {
        hidl_vec<StreamBuffer> vInputBuffer(1);

        status_t res = convertToHidlStreamBuffer(hal_captureResult.inputBuffer, &vInputBuffer[0], hal_captureResult.frameNumber);
        if (res != OK) {
            MY_LOGE("Converting hidl stream buffer failed: %s(%d)",
            strerror(-res), res);
            return res;
        }
        hidl_captureResult->v3_2.inputBuffer = vInputBuffer[0];
    }
    else
    {
        hidl_captureResult->v3_2.inputBuffer = {.streamId = -1};    // force assign -1 indicating no input buffer
    }

    hidl_captureResult->v3_2.frameNumber = hal_captureResult.frameNumber;
    hidl_captureResult->v3_2.fmqResultSize = 0;
    hidl_captureResult->v3_2.partialResult = hal_captureResult.partialResult;

    //physical meta call back
    if( ! hal_captureResult.physicalCameraMetadata.empty())
    {
        // write physical meta into FMQ with the last partial logical meta
        hidl_captureResult->physicalCameraMetadata.resize(hal_captureResult.physicalCameraMetadata.size());
        for(size_t i = 0; i< hal_captureResult.physicalCameraMetadata.size(); i++)
        {
            camera_metadata *metadata = const_cast<camera_metadata_t*>(hal_captureResult.physicalCameraMetadata[i].metadata);
            size_t metadata_size = hal_captureResult.physicalCameraMetadata[i].resultSize ;

            hidl_vec<uint8_t> physicTemp;
            physicTemp.setToExternal((uint8_t *)metadata, metadata_size);
            V3_4::PhysicalCameraMetadata physicalCameraMetadata =
            {
                .fmqMetadataSize = 0,
                .physicalCameraId = hal_captureResult.physicalCameraMetadata[i].physicalCameraId,
                .metadata = std::move(physicTemp),
            };
            hidl_captureResult->physicalCameraMetadata[i] = physicalCameraMetadata;
        }
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert CaptureResult to Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return OK;
}

auto
HidlCameraDeviceSession::
convertToHidlStreamBuffer(
const NSCam::v3::StreamBuffer& hal_buffer,
V3_2::StreamBuffer* hidl_buffer,
uint32_t requestNo
) -> ::android::status_t
{
    CAM_ULOGM_DTAG_BEGIN(ATRACE_ENABLED(),
                         "convertToHidlStreamBuffer:convertImage stream %#" PRIx32 " |request:%u",
                         hal_buffer.streamId, requestNo);
    NSCam::Utils::DebugTimer timer;
    timer.start();

    if (hidl_buffer == nullptr) {
        MY_LOGE("hidl_buffer is nullptr.");
        return BAD_VALUE;
    }

    hidl_buffer->streamId = hal_buffer.streamId;
    hidl_buffer->bufferId = hal_buffer.bufferId;
    hidl_buffer->buffer = nullptr;
    hidl_buffer->status = static_cast<V3_2::BufferStatus>(hal_buffer.status);
    hidl_buffer->acquireFence = hal_buffer.acquireFenceOp.hdl;
    hidl_buffer->releaseFence.setTo(const_cast<native_handle_t*>(hal_buffer.releaseFenceOp.hdl), true);

    MY_LOGD_IF(mConvertTimeDebug >=2, "hal_buffer.streamId(%d) .bufferId(%" PRId64 ") .buffer(%p) .status(%s) .acquireFence(%p) .releaseFence(%p)",
            hal_buffer.streamId, hal_buffer.bufferId, hal_buffer.buffer,
            (hal_buffer.status==NSCam::v3::BufferStatus::OK)? "OK" : "ERROR",
            hal_buffer.acquireFenceOp.hdl, hal_buffer.releaseFenceOp.hdl);

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert StreamBuffer to Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    CAM_ULOGM_DTAG_END();
    return OK;
}

auto
HidlCameraDeviceSession::
convertToHidlNotifyMsgs(
const std::vector<v3::NotifyMsg>& hal_messages,
std::vector<V3_2::NotifyMsg>* hidl_messages)
-> ::android::status_t
{
    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    if (hidl_messages == nullptr) {
        MY_LOGE("hidl_message is nullptr.");
        return BAD_VALUE;
    }

    for ( auto& hal_msg : hal_messages ) {

        V3_2::NotifyMsg hidl_msg ={};

        switch (hal_msg.type) {
            case v3::MsgType::ERROR:
            {
                hidl_msg.type = V3_2::MsgType::ERROR;
                auto res_error = convertToHidlErrorMessage(hal_msg.msg.error,
                                            &hidl_msg.msg.error);
                if (res_error != OK) {
                    MY_LOGE("Converting to HIDL error message failed: %s(%d)", strerror(-res_error), res_error);
                    return res_error;
                }
                break;
            }
            case v3::MsgType::SHUTTER:
            {
                hidl_msg.type = V3_2::MsgType::SHUTTER;
                auto res_shutter = convertToHidlShutterMessage(hal_msg.msg.shutter,
                                                &hidl_msg.msg.shutter);
                if (res_shutter != OK) {
                    MY_LOGE("Converting to HIDL shutter message failed: %s(%d)",strerror(-res_shutter), res_shutter);
                    return res_shutter;
                }
                break;
            }
            default:
            {
                MY_LOGE("Unknown message type: %u", hal_msg.type);
                return BAD_VALUE;
            }
        }

        hidl_messages->push_back(std::move(hidl_msg));
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert StreamBuffer to Hidl : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());
    return OK;
}

auto
HidlCameraDeviceSession::
convertToHidlErrorMessage(
const v3::ErrorMsg& hal_error,
V3_2::ErrorMsg* hidl_error)
-> ::android::status_t
{
    CAM_TRACE_CALL();
    if (hidl_error == nullptr) {
        MY_LOGE("hidl_error is nullptr.");
        return BAD_VALUE;
    }

    hidl_error->frameNumber = hal_error.frameNumber;
    hidl_error->errorStreamId = hal_error.errorStreamId;

    switch (hal_error.errorCode) {
        case v3::ErrorCode::ERROR_DEVICE:
            hidl_error->errorCode = V3_2::ErrorCode::ERROR_DEVICE;
            break;
        case v3::ErrorCode::ERROR_REQUEST:
            hidl_error->errorCode = V3_2::ErrorCode::ERROR_REQUEST;
            break;
        case v3::ErrorCode::ERROR_RESULT:
            hidl_error->errorCode = V3_2::ErrorCode::ERROR_RESULT;
            break;
        case v3::ErrorCode::ERROR_BUFFER:
            hidl_error->errorCode = V3_2::ErrorCode::ERROR_BUFFER;
            break;
        default:
            MY_LOGE("Unknown error code: %u", hal_error.errorCode);
            return BAD_VALUE;
    }

    return OK;
}

auto
HidlCameraDeviceSession::
convertToHidlShutterMessage(
const v3::ShutterMsg& hal_shutter,
V3_2::ShutterMsg* hidl_shutter)
-> ::android::status_t
{
    CAM_TRACE_CALL();
    if (hidl_shutter == nullptr) {
        MY_LOGE("hidl_shutter is nullptr.");
        return BAD_VALUE;
    }

    hidl_shutter->frameNumber = hal_shutter.frameNumber;
    hidl_shutter->timestamp = hal_shutter.timestamp;
    return OK;
}

auto
HidlCameraDeviceSession::
printAutoTestLog(
const camera_metadata_t* pMetadata,
bool bIsOutput)
-> void
{
    if ( !mAutoTestDebug )
        return;
    int result;
    camera_metadata_ro_entry_t entry;
    result = find_camera_metadata_ro_entry(
                                    pMetadata,
                                    ANDROID_CONTROL_AF_TRIGGER,
                                    &entry);
    uint8_t afTrigger = entry.data.u8[0];
    if ( result != -ENOENT ) {
        if (afTrigger == ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN ||
            afTrigger == ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED ||
            afTrigger == ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
            CAM_ULOGMD("[CAT][AF] ANDROID_CONTROL_AF_TRIGGER:%u IOType:%d",
                        afTrigger, bIsOutput);
        }
    }
}

