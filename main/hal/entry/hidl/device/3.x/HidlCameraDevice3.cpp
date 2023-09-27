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

#include "HidlCameraDevice3.h"
#include "HidlCameraDeviceSession.h"
#include "MyUtils.h"

#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
//
using namespace android;
using namespace NSCam;
using namespace NSCam::hidl_dev3;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("%d[HidlCameraDevice3::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
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
const std::string HidlCameraDevice3::MyDebuggee::mName{"ICameraDevice"};
auto HidlCameraDevice3::MyDebuggee::debug(android::Printer& printer, const std::vector<std::string>& options) -> void
{
    auto p = mContext.promote();
    if ( p != nullptr ) {
        p->debug(printer, options);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDevice3::
~HidlCameraDevice3()
{
    MY_LOGI("dtor");
    if ( mDebuggee != nullptr ) {
        if ( auto pDbgMgr = IDebuggeeManager::get() ) {
            pDbgMgr->detach(mDebuggee->mCookie);
        }
        mDebuggee = nullptr;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
HidlCameraDevice3::
HidlCameraDevice3(
    ::android::sp<CameraDevice3Impl> device
)
    : ICameraDevice()
    , mLogLevel(0)
    , mInstanceId(device->getInstanceId())
    , mMetadataConverter(IMetadataConverter::createInstance(IDefaultMetadataTagSet::singleton()->getTagSet()))
    , mDevice(device)
{
    MY_LOGI("ctor");
    mDebuggee = std::make_shared<MyDebuggee>(this);
    if ( auto pDbgMgr = IDebuggeeManager::get() ) {
        mDebuggee->mCookie = pDbgMgr->attach(mDebuggee, 1);
    }
    if (property_get_int32("vendor.debug.camera.hidl.metadump", 0) == 1)
      mLogLevel = 1;

}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDevice3::
create(
    ::android::sp<CameraDevice3Impl> device
) -> ::android::hidl::base::V1_0::IBase*
{
    auto pInstance = new HidlCameraDevice3(device);

    if  ( ! pInstance ) {
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
getResourceCost(getResourceCost_cb _hidl_cb)
{
    CameraResourceCost hidlCost;
    v3::CameraResourceCost mtkCost;

    int status = mDevice->getResourceCost(mtkCost);

    hidlCost.resourceCost = mtkCost.resourceCost;
    hidlCost.conflictingDevices.resize(mtkCost.conflictingDevices.size());
    for(int i = 0; i < mtkCost.conflictingDevices.size(); ++i) {
        hidlCost.conflictingDevices[i] = mtkCost.conflictingDevices[i];
    }

    _hidl_cb(mapToHidlCameraStatus(status), hidlCost);
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
getCameraCharacteristics(getCameraCharacteristics_cb _hidl_cb)
{
    CameraMetadata cameraCharacteristics;
    {
        const camera_metadata* p_camera_metadata = mDevice->getCameraCharacteristics();
        if ( p_camera_metadata == nullptr ){
            MY_LOGE("fail to getCameraCharacteristics");
        }
        size_t size = mMetadataConverter->getCameraMetadataSize(p_camera_metadata);
        MY_LOGD("CameraCharacteristics size: %zu", size);
        if (mLogLevel ==1) {
            IMetadata dstMeta;
            if ( ! mMetadataConverter->convert(p_camera_metadata, dstMeta) ) {
                MY_LOGE("Bad p_camera_metadata");
            }
            mMetadataConverter->dumpAll(dstMeta, -1);
        }
        cameraCharacteristics.setToExternal((uint8_t *)p_camera_metadata, size, false/*shouldOwn*/);
    }

    _hidl_cb(Status::OK, cameraCharacteristics);
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<Status>
HidlCameraDevice3::
setTorchMode(TorchMode mode)
{
    int status = mDevice->setTorchMode(static_cast<v3::TorchMode>(mode));
    return mapToHidlCameraStatus(status);
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
open(
    const ::android::sp<V3_2::ICameraDeviceCallback>& callback,
    open_cb _hidl_cb
)
{
    ::android::sp<ICameraDevice3Session> deviceSession = mDevice->open();
    if ( deviceSession == nullptr ) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }
    //
    auto hidlSession = ::android::sp<HidlCameraDeviceSession>(HidlCameraDeviceSession::create(deviceSession));
    ::android::status_t status = hidlSession->open(V3_5::ICameraDeviceCallback::castFrom(callback));
    //
    if  ( 0 != status ) {
        _hidl_cb(mapToHidlCameraStatus(status), nullptr);
    }
    else {
        _hidl_cb(mapToHidlCameraStatus(status), hidlSession);
    }
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
dumpState(
    const ::android::hardware::hidl_handle& handle
)
{
    ::android::hardware::hidl_vec<::android::hardware::hidl_string> options;
    debug(handle, options);
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
debug(const ::android::hardware::hidl_handle& handle __unused, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options __unused)
{
    //  validate handle
    if (handle == nullptr) {
        MY_LOGE("bad handle:%p", handle.getNativeHandle());
        return Void();
    }
    else if (handle->numFds != 1) {
        MY_LOGE("bad handle:%p numFds:%d", handle.getNativeHandle(), handle->numFds);
        return Void();
    }
    else if (handle->data[0] < 0) {
        MY_LOGE("bad handle:%p numFds:%d fd:%d < 0", handle.getNativeHandle(), handle->numFds, handle->data[0]);
        return Void();
    }

    FdPrinter printer(handle->data[0]);
    debug(printer, std::vector<std::string>{options.begin(), options.end()});

    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDevice3::
debug(android::Printer& printer, const std::vector<std::string>& options) -> void
{
    printer.printFormatLine("## Camera device [%s]", std::to_string(getInstanceId()).c_str());
    mDevice->dumpState(printer, options);
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
getPhysicalCameraCharacteristics(
    const ::android::hardware::hidl_string& physicalCameraId,
    getPhysicalCameraCharacteristics_cb _hidl_cb
)
{
    CameraMetadata cameraCharacteristics;
    int physicalId = strtol(physicalCameraId.c_str(), NULL, 10);

    camera_metadata* p_camera_metadata = nullptr;
    int status = mDevice->getPhysicalCameraCharacteristics(physicalId, p_camera_metadata);

    if (status == OK){
        size_t size = mMetadataConverter->getCameraMetadataSize(p_camera_metadata);
        cameraCharacteristics.setToExternal((uint8_t *)p_camera_metadata, size, false/*shouldOwn*/);
        _hidl_cb(Status::OK, cameraCharacteristics);
    }
    else
    {
        MY_LOGD("Physical camera ID %d is visible for logical ID %d", physicalId, getInstanceId());
        _hidl_cb(Status::ILLEGAL_ARGUMENT, cameraCharacteristics);
    }

    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
Return<void>
HidlCameraDevice3::
isStreamCombinationSupported(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration& streams,
    isStreamCombinationSupported_cb _hidl_cb
)
{
    bool isSupported = true;
    v3::StreamConfiguration mtkStreams;
    convertFromHidl(streams, mtkStreams);
    int status = mDevice->isStreamCombinationSupported(mtkStreams, isSupported);

    if (status == OK){
        _hidl_cb(Status::OK, isSupported);
    }
    else{
        _hidl_cb(Status::ILLEGAL_ARGUMENT, isSupported);
    }
    return Void();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
HidlCameraDevice3::
convertFromHidl(
    const ::android::hardware::camera::device::V3_4::StreamConfiguration& srcStreams,
    v3::StreamConfiguration& dstStreams
) -> void
{
    int size = srcStreams.streams.size();
    dstStreams.streams.resize(size);
    for ( int i=0 ; i<size ; i++ ){
        auto& srcStream = srcStreams.streams[i];
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

    dstStreams.operationMode        = static_cast<v3::StreamConfigurationMode> (srcStreams.operationMode);
    dstStreams.sessionParams        = reinterpret_cast<const camera_metadata_t*> (srcStreams.sessionParams.data());
    dstStreams.streamConfigCounter  = 0;  // V3_5

    return;
}

