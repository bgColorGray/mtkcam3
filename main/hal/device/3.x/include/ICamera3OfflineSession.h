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

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_DEVICE_3_X_INCLUDE_ICAMERA3OFFLINESESSION_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_DEVICE_3_X_INCLUDE_ICAMERA3OFFLINESESSION_H_
//
#include <memory>
#include <string>
#include <vector>
//
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#pragma GCC diagnostic pop
#include <mtkcam3/main/hal/devicemgr/ICameraDeviceManager.h>
#include <mtkcam3/main/hal/device/3.x/device/mtkcamhal_types.h>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {

class ICamera3OfflineSession
    : virtual public ::android::RefBase
{
public:     ////    Definitions.
                        ICamera3OfflineSession() = default;
    virtual             ~ICamera3OfflineSession() {}

    typedef ICameraDeviceManager::IVirtualDevice IVirtualDevice;

    struct  CreationInfo
    {
        ICameraDeviceManager*                   mDeviceManager = nullptr;
        std::shared_ptr<IVirtualDevice::Info>   mStaticDeviceInfo = nullptr;
        ::android::sp<IMetadataProvider>        mMetadataProvider = nullptr;
        ::android::sp<IMetadataConverter>       mMetadataConverter = nullptr;
        std::map<uint32_t, ::android::sp<IMetadataProvider>> mPhysicalMetadataProviders;
    };

public:     ////    Interfaces.

    virtual auto    getInstanceId() const -> int32_t                        = 0;

    virtual auto    setCallback(
                        const std::shared_ptr<v3::CameraDevice3SessionCallback>& callback
                    ) -> void = 0;

    virtual auto    close() -> int = 0;
};
};  //namespace NSCam


/******************************************************************************
 *
 ******************************************************************************/
extern "C"
NSCam::ICamera3OfflineSession*
createCamera3OfflineSession(
    NSCam::ICamera3OfflineSession::CreationInfo const& info
);


#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_DEVICE_3_X_INCLUDE_ICAMERA3OFFLINESESSION_H_

