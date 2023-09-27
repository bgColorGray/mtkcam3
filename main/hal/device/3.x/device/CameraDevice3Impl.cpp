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

#include "CameraDevice3Impl.h"
#include "MyUtils.h"
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/std/ULog.h>

#include <unordered_set>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

//
using namespace android;
using namespace NSCam;


/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("%d[CameraDevice3Impl::%s] " fmt, getInstanceId(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#define VALIDATE_STRING(string) (string!=nullptr ? string : "")

/******************************************************************************
 *
 ******************************************************************************/
CameraDevice3Impl::
~CameraDevice3Impl()
{
    MY_LOGD("[hidldc] dtor new camera device");
}


/******************************************************************************
 *
 ******************************************************************************/
CameraDevice3Impl::
CameraDevice3Impl(
    ICameraDeviceManager* deviceManager,
    IMetadataProvider* metadataProvider,
    std::map<uint32_t, ::android::sp<IMetadataProvider>>const& physicalMetadataProviders,
    char const* deviceType,
    int32_t instanceId,
    int32_t virtualInstanceId
)
    : ICameraDeviceManager::IVirtualDevice()
    , mLogLevel(0)
    , mDeviceManager(deviceManager)
    , mStaticDeviceInfo(nullptr)
    , mMetadataProvider(metadataProvider)
    , mMetadataConverter(IMetadataConverter::createInstance(IDefaultMetadataTagSet::singleton()->getTagSet()))
    , mPhysicalMetadataProviders(physicalMetadataProviders)
{
    mStaticDeviceInfo = std::make_shared<Info>();
    if (mStaticDeviceInfo != nullptr) {
        auto s = String8::format("device@%u.%u/%s/%d", kMajorDeviceVersion, kMinorDeviceVersion, deviceType, virtualInstanceId);
        auto n = s.string();
        mStaticDeviceInfo->mInstanceName    = (n)?n:"";
        mStaticDeviceInfo->mInstanceId      = instanceId;
        mStaticDeviceInfo->mVirtualInstanceId = virtualInstanceId;
        mStaticDeviceInfo->mMajorVersion    = kMajorDeviceVersion;
        mStaticDeviceInfo->mMinorVersion    = kMinorDeviceVersion;
        mStaticDeviceInfo->mHasFlashUnit    = metadataProvider->getDeviceHasFlashLight();
        mStaticDeviceInfo->mFacing          = metadataProvider->getDeviceFacing();
    }

    mLogLevel = getCameraDevice3DebugLogLevel();
    //
    MY_LOGD("[hidldc] ctor new camera device");
    MY_LOGD("%p mStaticDeviceInfo:%p MetadataProvider:%p MetadataConverter:%p debug.camera.log.<CameraDevice3>:%d",
        this, mStaticDeviceInfo.get(), mMetadataProvider.get(), mMetadataConverter.get(), mLogLevel);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
initialize(
    ICameraDevice3Session* session
) -> bool
{
    if (mStaticDeviceInfo == nullptr) {
        MY_LOGE("bad mStaticDeviceInfo");
        return false;
    }

    if (session == nullptr) {
        MY_LOGE("bad session");
        return false;
    }

    mSession = ::android::sp<ICameraDevice3Session>(session);

    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
getDeviceInterface(
    ::android::sp<CameraDevice3Impl>& rpDevice
) const -> ::android::status_t
{
    rpDevice = ::android::sp<CameraDevice3Impl>(const_cast<CameraDevice3Impl*>(this));
    return android::OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
getDeviceInfo() const -> Info const&
{
    return *mStaticDeviceInfo;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
getResourceCost(v3::CameraResourceCost& resCost) -> int
{
    ::android::Mutex::Autolock _getRscLock(mGetResourceLock);

    // NOTE: DEVICE_ID is the camera instance id defined by the camera provider HAL.
    //       It is either a small incrementing integer for "internal" device types,
    //       with 0 being the main back-facing camera and 1 being the main
    //       front-facing camera, if they exist.
    //       Or, for external devices, a unique serial number (if possible) that can be
    //       used to identify the device reliably when it is disconnected and reconnected.
    const MUINT DEVICE_ID = mStaticDeviceInfo->mInstanceId;
    resCost.resourceCost = 100;

    IHalLogicalDeviceList* pHalDeviceList = MAKE_HalLogicalDeviceList();

    std::vector<String8> conflictingList;
    auto& conflictingIdStringList = resCost.conflictingDeviceIds; // for legacy
    if(pHalDeviceList)
    {
        std::vector<MINT32> sensorList = pHalDeviceList->getSensorId(DEVICE_ID);
#if 0
        if(sensorList.size() > 1)
        {
            resCost.resourceCost = 100;
        }
        else
        {
            MBOOL canPowerOn = MTRUE;
            NSCamHW::HwInfoHelper helper(DEVICE_ID);
            helper.getSensorPowerOnPredictionResult(canPowerOn);
            resCost.resourceCost = (canPowerOn) ? 50 : 100;
        }
#endif
        // SENSOR_COUNT is the amount of imager(s)
        const MUINT SENSOR_COUNT = pHalDeviceList->queryNumberOfSensors();

        // DEVICE_COUNT is the amount of camera device(s)
        const MUINT DEVICE_COUNT = pHalDeviceList->queryNumberOfDevices();

        // conflictingDeviceIDList records camera device(s) in conflictingList
        std::unordered_set<MUINT> conflictingDeviceIDList;

        // helpers
        auto isFeatureSupported = [&pHalDeviceList](MUINT deviceID, DEVICE_FEATURE_TYPE type)
        {
            return (pHalDeviceList->getSupportedFeature(deviceID) & type) == type;
        };
        auto inConflictingDeviceIDList = [&conflictingDeviceIDList](MUINT deviceID) {
            return conflictingDeviceIDList.find(deviceID) != conflictingDeviceIDList.end(); };
        auto isPhysicalCameraDevice = [&SENSOR_COUNT](MUINT deviceID) {
            return deviceID < SENSOR_COUNT; };
        auto containLogicalCameraDevice = [&DEVICE_COUNT, &SENSOR_COUNT]() {
            return DEVICE_COUNT > SENSOR_COUNT; };
        auto addToList = [&conflictingList, &conflictingDeviceIDList, &conflictingIdStringList](
                std::string type, MUINT deviceID)
        {
            conflictingList.push_back(String8::format("device@%u.%u/%s/%u",
                        kMajorDeviceVersion, kMinorDeviceVersion,
                        type.c_str(), deviceID));
            // for legacy
            conflictingIdStringList.push_back(VALIDATE_STRING(String8::format("%u", deviceID).string()));
            conflictingDeviceIDList.insert(deviceID);
        };

        if(isPhysicalCameraDevice(DEVICE_ID))
        {
            // [physical camera device(s)]

            // use case: all physical camera device(s) if existing logical camera device(s)
            if(containLogicalCameraDevice())
            {
                // add the corresponding logical camera device(s) into conflictingList
                // of a physical camera device
                for(MUINT s = SENSOR_COUNT; s < DEVICE_COUNT; ++s)
                {
                    std::vector<MINT32> sensors = pHalDeviceList->getSensorId(s);
                    bool hasSameSensor = false;
                    for(auto &sensor : sensors)
                    {
                        if(sensor == DEVICE_ID)
                        {
                            hasSameSensor = true;
                            break;
                        }
                    }

                    if(hasSameSensor)
                    {
                        // the logical camera device exists in the conflictingDeviceIDList, do nothing.
                        if(inConflictingDeviceIDList(s))
                            continue;

                        addToList("internal", s);
                    }

                    // because the secure camera device is realized by the logical camera device,
                    // add the logical camera device into conflictingList if it is a secure one.
                    if(isFeatureSupported(s, DEVICE_FEATURE_SECURE_CAMERA))
                    {
                        // the logical camera device exists in the conflictingDeviceIDList, do nothing.
                        if(inConflictingDeviceIDList(s))
                            continue;

                        addToList("internal", s);
                    }
                }
            }
        }
        else
        {
            // [logical camera device(s)]

            // use case: secure camera device
            if(isFeatureSupported(DEVICE_ID, DEVICE_FEATURE_SECURE_CAMERA))
            {
                // add all physical camera device into conflictingList
                for(auto s = 0; s < DEVICE_COUNT; ++s)
                {
                    // if the camera device either exists in the conflictingDeviceIDList,
                    // or is the same as itself, do nothing.
                    if(inConflictingDeviceIDList(s) || (s == DEVICE_ID))
                        continue;

                    addToList("internal", s);
                }
            }

            // use case: multi-camera device
            for(auto &s : sensorList)
            {
                // the physical camera device exists in the conflictingDeviceIDList,
                // do nothing.
                if(inConflictingDeviceIDList(s))
                    continue;

                addToList("internal", s);
            }
        }
    }
    resCost.conflictingDevices.resize(conflictingList.size());
    for(int i = 0; i < conflictingList.size(); ++i) {
        resCost.conflictingDevices[i] = VALIDATE_STRING(conflictingList[i].string());
    }

    std::string conflictString;
    if(resCost.conflictingDevices.size() > 0)
    {
        for (size_t i = 0; i < resCost.conflictingDevices.size(); i++) {
            conflictString += resCost.conflictingDevices[i];
            if(i < resCost.conflictingDevices.size() - 1) {
                conflictString += ", ";
            }
        }
    }

    MY_LOGD("Camera Device %d: resourceCost %d, conflict to [%s]",
            DEVICE_ID, resCost.resourceCost, conflictString.c_str());

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
getCameraCharacteristics(
) -> const camera_metadata*
{
    return mMetadataProvider->getStaticCharacteristics();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
setTorchMode(
    v3::TorchMode mode
) -> int
{
    return mDeviceManager->setTorchMode(mStaticDeviceInfo->mInstanceName, static_cast<uint32_t>(mode));
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
open() -> ::android::sp<ICameraDevice3Session>
{
    int systraceLevel = ::property_get_int32("vendor.debug.mtkcam.systrace.level", MTKCAM_SYSTRACE_LEVEL_DEFAULT);
    MY_LOGI("open camera3 device (%s) systraceLevel(%d) instanceId(%d) vid(%d)",
                        mStaticDeviceInfo->mInstanceName.c_str(),
                        systraceLevel,
                        mStaticDeviceInfo->mInstanceId,
                        mStaticDeviceInfo->mVirtualInstanceId);
    // mSession->open();
    return mSession;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void
{
    printer.printFormatLine("## Camera device [%s]", mStaticDeviceInfo->mInstanceName.c_str());
    mSession->dumpState(printer, options);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
getPhysicalCameraCharacteristics(
    int physicalId,
    camera_metadata* physicalMetadata
) -> int
{
    MY_LOGD("ID: %d", physicalId);

    bool isHidden = mDeviceManager->isHiddenCamera(physicalId);
    if(isHidden) {
        auto pMetadataProvider = NSMetadataProviderManager::valueForByDeviceId(physicalId);
        camera_metadata const* pCameraMetadata = pMetadataProvider->getStaticCharacteristics();
        physicalMetadata = const_cast<camera_metadata*>(pCameraMetadata);
    }
    else
    {
        MY_LOGD("Physical camera ID %d is visible for logical ID %d", physicalId, mStaticDeviceInfo->mInstanceId);
        return BAD_VALUE;
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
isStreamCombinationSupported(const v3::StreamConfiguration& streams, bool& isSupported) -> int
{
    isSupported = true;

    if(streams.streams.size() == 0) {
        MY_LOGD("No stream found in stream configuration");
        isSupported = false;
        return OK;
    }

    for(auto &stream : streams.streams)
    {
        if(stream.width  == 0 ||
            stream.height == 0)
        {
            MY_LOGD("Invalid stream size: %dx%d", stream.width, stream.height);
            isSupported = false;
            break;
        }

        //TODO: Qury by ISP capability
    }

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
CameraDevice3Impl::
getDeviceSession() -> const ::android::sp<ICameraDevice3Session>
{
    return mSession;
}

