/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_DEVICE_CAMERADEVICE3IMPL_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_DEVICE_CAMERADEVICE3IMPL_H_
//
#include "../include/ICameraDevice3Session.h"
//
#include <memory>
#include <string>
//
#include <utils/String8.h>
#include <utils/RWLock.h>
#include <utils/Mutex.h>
//
// #include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam3/main/hal/devicemgr/ICameraDeviceManager.h>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {


/******************************************************************************
 *
 ******************************************************************************/
class CameraDevice3Impl
    : public ICameraDeviceManager::IVirtualDevice
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// public:     ////

//     struct  MyDebuggee : public IDebuggee
//     {
//         static const std::string        mName;
//         std::shared_ptr<IDebuggeeCookie>mCookie = nullptr;
//         android::wp<CameraDevice3Impl>  mContext = nullptr;

//                         MyDebuggee(CameraDevice3Impl* p) : mContext(p) {}
//         virtual auto    debuggeeName() const -> std::string { return mName; }
//         virtual auto    debug(
//                             android::Printer& printer,
//                             const std::vector<std::string>& options
//                         ) -> void;
//     };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    //  setup during constructor
    int32_t                                 mLogLevel = 0;                  //  log level.
    ICameraDeviceManager*                   mDeviceManager = nullptr;       //  device manager.
    std::shared_ptr<Info>                   mStaticDeviceInfo = nullptr;    //  device info
    // std::shared_ptr<MyDebuggee>             mDebuggee = nullptr;
    ::android::sp<IMetadataProvider>        mMetadataProvider = nullptr;
    ::android::sp<IMetadataConverter>       mMetadataConverter = nullptr;
    ::android::sp<ICameraDevice3Session>    mSession = nullptr;
    std::map<uint32_t, ::android::sp<IMetadataProvider>> mPhysicalMetadataProviders;
    mutable ::android::Mutex                mGetResourceLock;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation.
    virtual         ~CameraDevice3Impl();
                    CameraDevice3Impl(
                        ICameraDeviceManager* deviceManager,
                        IMetadataProvider* metadataProvider,
                        std::map<uint32_t, ::android::sp<IMetadataProvider>>const& physicalMetadataProviders,
                        char const* deviceType,
                        int32_t instanceId,
                        int32_t virtualInstanceId);

    virtual auto    initialize(
                        ICameraDevice3Session* session
                        ) -> bool;

public:     ////    Operations.
    auto            getLogLevel() const             { return mLogLevel; }
    auto const&     getStaticDeviceInfo() const     { return mStaticDeviceInfo; }
    auto const&     getMetadataConverter() const    { return mMetadataConverter; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    ICameraDeviceManager::IVirtualDevice Interfaces.

    virtual auto    getDeviceInterface(
                        ::android::sp<CameraDevice3Impl>& rpDevice
                        ) const -> ::android::status_t override;

    virtual auto    getDeviceInfo() const -> Info const& override;

public:     ////    ICameraDevice Interfaces implement.

    virtual auto    getResourceCost(v3::CameraResourceCost& mtkCost) -> int;

    virtual auto    setTorchMode(v3::TorchMode mode) -> ::android::status_t;

    virtual auto    open() -> ::android::sp<ICameraDevice3Session>;

    virtual auto    dumpState(android::Printer& printer, const std::vector<std::string>& options) -> void;

    virtual auto    getCameraCharacteristics() -> const camera_metadata_t*;

    virtual auto    getPhysicalCameraCharacteristics(
                        int physicalId,
                        camera_metadata* physicalMetadata) -> int;

    virtual auto    isStreamCombinationSupported(
                        const v3::StreamConfiguration& streams,
                        bool& isSupported) -> int;

public:     ////    Operations

    virtual auto    getDeviceSession() -> const ::android::sp<ICameraDevice3Session>;

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_DEVICE_CAMERADEVICE3IMPL_H_

