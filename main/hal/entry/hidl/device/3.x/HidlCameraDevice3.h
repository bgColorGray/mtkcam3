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

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_HIDL_HIDLCAMERADEVICE3_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_HIDL_HIDLCAMERADEVICE3_H_

#include <memory>
#include <string>
//
#include <utils/String8.h>
#include <utils/RWLock.h>
#include <utils/Mutex.h>
//
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam3/main/hal/devicemgr/ICameraDeviceManager.h>
//
#include <HidlCameraDevice.h>
#include "../../../../device/3.x/device/CameraDevice3Impl.h"

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace hidl_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraDevice3
    : public ::android::hardware::camera::device::V3_6::ICameraDevice
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    struct  MyDebuggee : public IDebuggee
    {
        static const std::string        mName;
        std::shared_ptr<IDebuggeeCookie>mCookie = nullptr;
        android::wp<HidlCameraDevice3>  mContext = nullptr;

                        MyDebuggee(HidlCameraDevice3* p) : mContext(p) {}
        virtual auto    debuggeeName() const -> std::string { return mName; }
        virtual auto    debug(
                            android::Printer& printer,
                            const std::vector<std::string>& options
                        ) -> void;
    };


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    //  setup during constructor
    int32_t                                 mLogLevel = 0;                  //  log level.
    int32_t                                 mInstanceId = -1;
    std::shared_ptr<MyDebuggee>             mDebuggee = nullptr;
    ::android::sp<IMetadataConverter>       mMetadataConverter = nullptr;
    ::android::sp<CameraDevice3Impl>        mDevice = nullptr;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation.
    virtual         ~HidlCameraDevice3();
                    HidlCameraDevice3(::android::sp<CameraDevice3Impl> device);
    static auto     create(::android::sp<CameraDevice3Impl> device) -> ::android::hidl::base::V1_0::IBase*;

public:     ////    Operations.
    auto            getLogLevel() const             { return mLogLevel; }
    auto            getInstanceId() const           { return mInstanceId; }
    auto const&     getMetadataConverter() const    { return mMetadataConverter; }
    auto            debug(android::Printer& printer, const std::vector<std::string>& options) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraDevice Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    virtual Return<void>    getResourceCost(getResourceCost_cb _hidl_cb) override;

    virtual Return<void>    getCameraCharacteristics(getCameraCharacteristics_cb _hidl_cb) override;

    virtual Return<Status>  setTorchMode(TorchMode mode) override;

    virtual Return<void>    open(const ::android::sp<V3_2::ICameraDeviceCallback>& callback, open_cb _hidl_cb) override;

    virtual Return<void>    dumpState(const ::android::hardware::hidl_handle& handle) override;

    virtual Return<void>    debug(
                                const ::android::hardware::hidl_handle& fd,
                                const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;

    virtual Return<void>    getPhysicalCameraCharacteristics(
                                const ::android::hardware::hidl_string& physicalCameraId,
                                getPhysicalCameraCharacteristics_cb _hidl_cb) override;

    virtual Return<void>    isStreamCombinationSupported(
                                const ::android::hardware::camera::device::V3_4::StreamConfiguration& streams,
                                isStreamCombinationSupported_cb _hidl_cb) override;

private:    ////        interface/structure conversion helper
    virtual auto            convertFromHidl(
                                const ::android::hardware::camera::device::V3_4::StreamConfiguration& srcStreams,
                                NSCam::v3::StreamConfiguration& dstStreams) -> void;

};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace hidl_dev3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_HIDL_HIDLCAMERADEVICE3_H_

