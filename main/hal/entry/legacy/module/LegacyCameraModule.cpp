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

#include "LegacyCameraModule.h"
#include "../device/LegacyCameraDevice3.h"
#include "../../../device/3.x/device/CameraDevice3Impl.h"
//
#include <errno.h>
#include <cutils/properties.h>
#include <utils/Printer.h>
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/common.h>
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IVendorTagDescriptor.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam3/main/hal/devicemgr/ICameraDeviceManager.h>
//#include <mtkcam3/main/hal/device/3.x/device/types.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
//
using namespace android;
using namespace NSCam;
using namespace NSCam::legacy_dev3;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#if 1
#define MODULE_HAL_PUBLIC_API_START     MY_LOGD("+")
#define MODULE_HAL_PUBLIC_API_END       MY_LOGD("-")
#else
#define MODULE_HAL_PUBLIC_API_START
#define MODULE_HAL_PUBLIC_API_END
#endif


/******************************************************************************
 *
 ******************************************************************************/
LegacyCameraModule::
~LegacyCameraModule()
{
    MY_LOGI("dtor");
}


/******************************************************************************
 *
 ******************************************************************************/
LegacyCameraModule::
LegacyCameraModule(ICameraDeviceManager* manager)
    : mManager(manager)
{
    MY_LOGI("ctor");
    mModuleCallbacks = android::sp<LegacyCameraModule>(this);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraModule::
initialize() -> bool
{
    CAM_TRACE_NAME(LOG_TAG ":initialize");

    std::vector<std::string> deviceNameList;

    // global vendor tags should be setup before enumerating devices...
    auto pVendorTagDesc = NSCam::getVendorTagDescriptor();
    if  ( ! pVendorTagDesc ) {
        MY_LOGE("bad pVendorTagDesc");
        return false;
    }

    ::memset(&mVendorTagOps, 0, sizeof(mVendorTagOps));
#warning "cherry-pick implementation of VendorTagDesc::getVendorTagOps"
    pVendorTagDesc->getVendorTagOps(&mVendorTagOps);
    // CAM_ULOGMD("get_tag_count(%p) get_all_tags(%p) get_section_name(%p) get_tag_name(%p) get_tag_type(%p)", mVendorTagOps.get_tag_count, mVendorTagOps.get_all_tags, mVendorTagOps.get_section_name, mVendorTagOps.get_tag_name, mVendorTagOps.get_tag_type);

    LegacyCameraModule::getDevicesNameList(deviceNameList);

    for(auto const& deviceName : deviceNameList) {
    // Device Map
        size_t pos = deviceName.rfind('/');
        std::string cameraId = deviceName.substr(pos+1);
        int deviceId = std::stoi(cameraId);
        mCameraDeviceNames.insert(std::pair<std::string, std::string>(deviceName, cameraId));
        MY_LOGD("mCameraDeviceNames %d, %s", deviceId, cameraId.c_str());
    }

    // for(std::map<std::string, std::string>::iterator it = mCameraDeviceNames.begin(); it != mCameraDeviceNames.end(); it++)
    //   MY_LOGD("mCameraDeviceNames <%s, %s>", it->first.c_str(), it->second.c_str());

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
open(
    hw_device_t** device,
    hw_module_t const* module,
    char const* name,
    uint32_t device_version
)
{
    MY_LOGD("+");
    int status;
    int32_t const i4OpenId = (name != NULL) ? ::atoi(name) : -1;
    if  ( 0 == device_version ) {
        camera_info info;
        if  ( OK != (status = getDeviceInfo(i4OpenId, info)) ) {
            return status;
        }
        device_version = info.device_version;
    }
    //
    RWLock::AutoWLock _l(mDataRWLock);
    status = openDeviceLocked(device, module, i4OpenId, device_version);
    MY_LOGD("- %d", status);
    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
int32_t
LegacyCameraModule::
getNumberOfDevices()
{
    MY_LOGD("+");
    //
    RWLock::AutoWLock _l(mDataRWLock);
    std::vector<std::string> deviceNameList;
    //
    int status = mManager->getDeviceNameList(deviceNameList);
    if (status != OK) {
        MY_LOGE("getDeviceNameList error");
        return OK;
    }
    //
    return deviceNameList.size();
}

/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
getDevicesNameList(std::vector<std::string>& deviceNameList)
{
    MY_LOGD("+");
    //
    RWLock::AutoWLock _l(mDataRWLock);
    //
    int status = mManager->getDeviceNameList(deviceNameList);
    if (status != OK) {
        MY_LOGE("getDeviceNameList error");
        return OK;
    }
    MY_LOGD("-");

    return status;
}

/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
getDeviceInfo(int const deviceId, camera_info& rInfo)
{
    MODULE_HAL_PUBLIC_API_START;
    RWLock::AutoWLock _l(mDataRWLock);

    android::sp<CameraDevice3Impl> pDevice;
    getDeviceInterfaceWithId(deviceId, &pDevice);

    uint32_t major = pDevice->getMajorVersion();
    uint32_t minor = pDevice->getMinorVersion();
    auto instanceId = pDevice->getInstanceId();
    auto pStaticMeta = NSMetadataProviderManager::valueForByDeviceId(instanceId);
    if ( CC_UNLIKELY(!pStaticMeta) ) {
        MY_LOGF("get deviceInfo id(%d) instanceId(%d) fail: StaticMetadata(%p)", deviceId, instanceId, pStaticMeta);
        return -EINVAL;
    }

    MY_LOGD("virtualDevice(%p) device_version(0x%x)", pDevice.get(), HARDWARE_DEVICE_API_VERSION(major, minor));
    rInfo.device_version = (pDevice) ? HARDWARE_DEVICE_API_VERSION(major, minor) : 0;
    rInfo.static_camera_characteristics = pDevice->getCameraCharacteristics();
    rInfo.facing    = (pStaticMeta->getDeviceFacing() == MTK_LENS_FACING_FRONT)
                      ? CAMERA_FACING_FRONT : CAMERA_FACING_BACK;
    rInfo.orientation    = pStaticMeta->getDeviceWantedOrientation();
    //
    getResourceCost(deviceId, rInfo);
    //
    MY_LOGD("deviceId:%d instanceId:%d device_version:0x%x facing:%d orientation:%d", deviceId, instanceId, rInfo.device_version, rInfo.facing, rInfo.orientation);
    MODULE_HAL_PUBLIC_API_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
setCallbacks(camera_module_callbacks_t const* callbacks)
{
    MODULE_HAL_PUBLIC_API_START;
    RWLock::AutoWLock _l(mDataRWLock);
    //
    MY_LOGD("setCallbacks:%p", callbacks);
    mpModuleCallbacks = callbacks;
    //
    mManager->setCallbacks(mModuleCallbacks);

    MODULE_HAL_PUBLIC_API_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
void
LegacyCameraModule::
getVendorTagOps(
    vendor_tag_ops_t* ops
)
{
    MODULE_HAL_PUBLIC_API_START;
    MY_LOGD("ops:%p", ops);
    *ops = mVendorTagOps;
    MODULE_HAL_PUBLIC_API_END;
}


/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
setTorchMode(int const deviceId, bool enabled)
{
    MODULE_HAL_PUBLIC_API_START;
    v3::TorchMode mode = (enabled) ? v3::TorchMode::ON : v3::TorchMode::OFF;

    android::sp<CameraDevice3Impl> pDevice;
    getDeviceInterfaceWithId(deviceId, &pDevice);

    int status = pDevice->setTorchMode(mode);

    MODULE_HAL_PUBLIC_API_END;
    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
getPhysicalCameraInfo(int physical_camera_id, camera_metadata_t **static_metadata)
{
    MODULE_HAL_PUBLIC_API_START;

    android::sp<CameraDevice3Impl> pDevice;
    int deviceId;
    mManager->getDeviceIdWithPhysicalId(physical_camera_id, deviceId);
    getDeviceInterfaceWithId(deviceId, &pDevice);
    int status = pDevice->getPhysicalCameraCharacteristics(physical_camera_id, *static_metadata);

    MODULE_HAL_PUBLIC_API_END;
    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
isStreamCombinationSupported(int deviceId, const camera_stream_combination_t *streams)
{
    // Return values:
    // 0:           In case the stream combination is supported.
    // -EINVAL:     In case the stream combination is not supported.
    // -ENOSYS:     In case stream combination query is not supported.
    MODULE_HAL_PUBLIC_API_START;
    bool isSupported = true;

    android::sp<CameraDevice3Impl> pDevice;
    getDeviceInterfaceWithId(deviceId, &pDevice);

    v3::StreamConfiguration mtkStreams;
    // convertFromLegacy(*streams, mtkStreams);
    convertFromLegacy(streams, &mtkStreams);
    int status = pDevice->isStreamCombinationSupported(mtkStreams, isSupported);
    if (status != OK) {
        MY_LOGW("fail to check isStreamCombinationSupported, return %d", status);
        return -ENOSYS;
    }

    MODULE_HAL_PUBLIC_API_END;
    if (isSupported) {
        return 0;
    }
    else {
        return -EINVAL;
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
LegacyCameraModule::
notifyDeviceStateChange(uint64_t deviceState)
{
    MODULE_HAL_PUBLIC_API_START;
    mManager->notifyDeviceStateChange(static_cast<v3::DeviceState>(deviceState));

    MODULE_HAL_PUBLIC_API_END;
    return  /*android::NAME_NOT_FOUND*/;
}


/******************************************************************************
 *
 ******************************************************************************/
int
LegacyCameraModule::
openDeviceLocked(
    hw_device_t** device,
    hw_module_t const* module,
    int32_t const deviceId,
    uint32_t device_version
)
{
    int status;
    android::sp<CameraDevice3Impl> pDevice;
    getDeviceInterfaceWithId(deviceId, &pDevice);

    int systraceLevel = ::property_get_int32("vendor.debug.mtkcam.systrace.level", MTKCAM_SYSTRACE_LEVEL_DEFAULT);
    MY_LOGI("open camera3 device id(%d) systraceLevel(%d) CamraDevice3Impl(%p)", deviceId, systraceLevel, pDevice.get());

    android::sp<ICameraDevice3Session> session = pDevice->open();
    if  ( session == nullptr ) {
        MY_LOGE("fail to open CameraDevice3Impl:%d ", deviceId);
        return BAD_VALUE;
    }

    // create LegacyCameraDevice3
    auto legacyDevice = new LegacyCameraDevice3(deviceId, session);
    if ( legacyDevice == nullptr )
    {
        MY_LOGE("fail to create LegacyCameraDevice3:%d", deviceId);
        return BAD_VALUE;
    }

    // open device successfully.
    {
        *device = const_cast<hw_device_t*>(legacyDevice->get_hw_device());
        legacyDevice->set_hw_module(module);
        status = legacyDevice->open(mpModuleCallbacks);
        // legacyDevice->set_module_callbacks(mpModuleCallbacks);
        // legacyDevice->setDeviceManager(this);
    }
    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraModule::
getResourceCost(
    int const deviceId,
    camera_info& rInfo
) -> int
{
    android::sp<CameraDevice3Impl> pDevice;
    getDeviceInterfaceWithId(deviceId, &pDevice);

    v3::CameraResourceCost rResourceCost;
    int status = pDevice->getResourceCost(rResourceCost);

    // structure tranformation
    auto& conflictingDevicesId = rResourceCost.conflictingDeviceIds;
    rInfo.resource_cost = rResourceCost.resourceCost;
    rInfo.conflicting_devices_length = rResourceCost.conflictingDevices.size();
    rInfo.conflicting_devices = new char*[conflictingDevicesId.size()];
    for(int i = 0; i < conflictingDevicesId.size(); ++i) {
        size_t deviceName_size = strlen(conflictingDevicesId[i].c_str()) + 1;
        rInfo.conflicting_devices[i] = new char[deviceName_size];
        memset(rInfo.conflicting_devices[i], '\0', deviceName_size);
        strcpy(rInfo.conflicting_devices[i], conflictingDevicesId[i].c_str());
    }

    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraModule::
getDeviceInterfaceWithId(
    int const deviceId,
    android::sp<CameraDevice3Impl> *pDevice
) -> int
{
    std::string deviceName;
    int status = mManager->getDeviceName(deviceId, &deviceName);
    if  ( OK != status ) {
        MY_LOGE("[%d] getDeviceName fail, deviceName=%s", deviceId, deviceName.c_str());
        return BAD_VALUE;
    }

    status = mManager->getDeviceInterface(deviceName, *pDevice);
    if  ( OK != status || (*pDevice).get()==nullptr ) {
        MY_LOGE("[%d] getDeviceInterface fail", deviceId);
        return BAD_VALUE;
    }

    return status;
}


/******************************************************************************
 * NSCam::ICameraDeviceManager::Callback interface
 ******************************************************************************/
/******************************************************************************
 *
 ******************************************************************************/
void
LegacyCameraModule::
onCameraDeviceStatusChange(char const* deviceName, uint32_t new_status)
{
    v3::CameraDeviceStatus const status = (v3::CameraDeviceStatus) new_status;
    std::map<std::string, std::string>::iterator camIt = mCameraDeviceNames.find(deviceName);

    mpModuleCallbacks->camera_device_status_change(mpModuleCallbacks, std::stoi(camIt->first), new_status);
    MY_LOGI("Implement, %s CameraDeviceStatus:%u camId:%d", deviceName, status, std::stoi(camIt->first));
}


/******************************************************************************
 *
 ******************************************************************************/
void
LegacyCameraModule::
onTorchModeStatusChange(char const* deviceName, uint32_t new_status)
{
    v3::TorchModeStatus const status = (v3::TorchModeStatus) new_status;
    std::map<std::string, std::string>::iterator camIt = mCameraDeviceNames.find(deviceName);

    mpModuleCallbacks->torch_mode_status_change(mpModuleCallbacks, camIt->second.c_str(), new_status);

    MY_LOGI("Implement, %s TorchModeStatus:%u camId:%s", deviceName, status, camIt->second.c_str());
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraModule::
convertFromLegacy(
    const camera_stream_combination_t* srcStreams,
    v3::StreamConfiguration* dstStreams
) -> void
{
    int size = srcStreams->num_streams;
    dstStreams->streams.resize(size);
    // camera_stream_t *srcStream = srcStreams->streams;

    for (int i = 0; i < size; i++) {
        camera_stream_t srcStream = (srcStreams->streams)[i];
        NSCam::v3::Stream *dstStream = &(dstStreams->streams[i]);
        //
        dstStream->streamType = static_cast<v3::StreamType> (srcStream.stream_type);
        dstStream->width = srcStream.width;
        dstStream->height = srcStream.height;
        dstStream->format = static_cast<android_pixel_format_t> (srcStream.format);
        dstStream->usage = srcStream.usage;
        dstStream->dataSpace = srcStream.data_space;
        dstStream->rotation = static_cast<v3::StreamRotation> (srcStream.rotation);
        //
        dstStream->physicalCameraId = srcStream.physical_camera_id;
        MY_LOGD("Stream Combined info streamType:%d %dx%d format:0x%x usage:0x%" PRIx64 " dataSpace:0x%x rotation:%d",
                 dstStream->streamType,
                 dstStream->width, dstStream->height,
                 dstStream->format, dstStream->usage,
                 dstStream->dataSpace, dstStream->rotation);
    }

    dstStreams->operationMode = static_cast<v3::StreamConfigurationMode> (srcStreams->operation_mode);
}


/******************************************************************************
 * createion
 ******************************************************************************/
/******************************************************************************
 *
 ******************************************************************************/
extern "C"
LegacyCameraModule*
createLegacyCameraModule(ICameraDeviceManager* manager)
{
    const std::string& moduleName = manager->getType();
    MY_LOGI("+ %s", moduleName.c_str());
    //
    if ( ! manager ) {
        MY_LOGE("bad CameraDeviceManager");
        return nullptr;
    }
    //
    auto module = new LegacyCameraModule(manager);
    if ( ! module ) {
        MY_LOGE("cannot allocate LegacyCameraModule %s", moduleName.c_str());
        return nullptr;
    }
    //
    if ( ! module->initialize() ) {
        MY_LOGE("LegacyCameraModule %s initialize failed", moduleName.c_str());
        delete module;
        module = nullptr;
    }
    //
    MY_LOGI("- %s module:%p manager:%p", moduleName.c_str(), module, manager);
    return module;
}
