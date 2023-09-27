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

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_LEGACY_LEGACYCAMERAMODULE_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_LEGACY_LEGACYCAMERAMODULE_H_

#include <utils/Mutex.h>
#include <utils/RWLock.h>
#include <utils/StrongPointer.h>
#include <hardware/camera3.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam3/main/hal/devicemgr/ICameraDeviceManager.h>
//
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace legacy_dev3 {


/******************************************************************************
 *
 ******************************************************************************/
class LegacyCameraModule
    : public NSCam::ICameraDeviceManager::Callback
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Instantiation.
    virtual             ~LegacyCameraModule();
                        LegacyCameraModule(NSCam::ICameraDeviceManager* manager);
    virtual auto        initialize() -> bool;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICamDevice Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    virtual auto    open(
                        hw_device_t** device,
                        hw_module_t const* module,
                        char const* name,
                        uint32_t device_version
                    ) -> int;

    // virtual auto   close(ICamDevice* pDevice) -> android::status_t;

    virtual auto    getNumberOfDevices() -> int32_t;

    virtual auto    getDeviceInfo(
                        int const deviceId,
                        camera_info& rInfo
                    ) -> int;

    virtual auto    setCallbacks(
                        camera_module_callbacks_t const* callbacks
                    ) -> int;

    virtual auto    getVendorTagOps(
                        vendor_tag_ops_t* ops
                    ) -> void;

    virtual auto    setTorchMode(
                        int const deviceId,
                        bool enabled
                    ) -> int;

    virtual auto    getPhysicalCameraInfo(
                        int physical_camera_id,
                        camera_metadata_t **static_metadata
                    ) -> int;

    virtual auto    isStreamCombinationSupported(
                        int camera_id,
                        const camera_stream_combination_t *streams
                    ) -> int;

    virtual auto    notifyDeviceStateChange(uint64_t deviceState) -> void;

protected:

    virtual auto    openDeviceLocked(
                        hw_device_t** device,
                        hw_module_t const* module,
                        int32_t const deviceId,
                        uint32_t device_version
                    ) -> android::status_t;

    virtual auto    getResourceCost(
                        int const deviceId,
                        camera_info& rInfo
                    ) -> android::status_t;

    virtual auto    getDeviceInterfaceWithId(
                        int const deviceId,
                        android::sp<CameraDevice3Impl>* pDevice
                    ) -> int;

    virtual auto    getDevicesNameList(
                        std::vector<std::string>& deviceNameList
                    ) -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraDeviceManager::Callback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual void    onCameraDeviceStatusChange(
                        char const* deviceName, uint32_t new_status
                    ) override;

    virtual void    onTorchModeStatusChange(
                        char const* deviceName, uint32_t new_status
                    ) override;

private:    ////        interface/structure conversion helper
    virtual auto            convertFromLegacy(
                                const camera_stream_combination_t* srcStreams,
                                NSCam::v3::StreamConfiguration* dstStreams) -> void;
                                // const camera_stream_combination_t& srcStreams,
                                // NSCam::v3::StreamConfiguration& dstStreams) -> void;

    // virtual auto            createModuleCallbacks() -> std::shared_ptr<ICameraDeviceManager::Callback>;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    NSCam::ICameraDeviceManager*                   mManager;
    android::sp<Callback>                          mModuleCallbacks;
    std::map<std::string, std::string>             mCameraDeviceNames;
    vendor_tag_ops_t                               mVendorTagOps;
    mutable ::android::Mutex                       mGetResourceLock;
    mutable ::android::RWLock                      mDataRWLock;
    camera_module_callbacks_t const*               mpModuleCallbacks = nullptr;
};

};  //namespace legacy_dev3
};  //namespace NSCam


/******************************************************************************
 *
 ******************************************************************************/
extern "C"
NSCam::legacy_dev3::LegacyCameraModule*
createLegacyCameraModule(NSCam::ICameraDeviceManager* manager);

#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_LEGACY_LEGACYCAMERAMODULE_H_

