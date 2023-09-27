/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_IMTKCAMVIRTUALDEVICESESSION_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_IMTKCAMVIRTUALDEVICESESSION_H_
//
// #include "HidlCameraDevice.h"
//
// #include <mtkcam/utils/debug/IPrinter.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#pragma GCC diagnostic pop
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h>
#include <mtkcam3/main/mtkhal/devicemgr/ICameraDeviceManager.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/policy/types.h>
//
#include <map>
#include <memory>
#include <string>
#include <vector>

using NSCam::ICameraDeviceManager;
using NSCam::IMetadataConverter;
using NSCam::IMetadataProvider;
using NSCam::IPrinter;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {

class IMtkcamVirtualDeviceSession : virtual public ::android::RefBase {
 public:  //// Definitions.
  IMtkcamVirtualDeviceSession() = default;
  virtual ~IMtkcamVirtualDeviceSession() {}

  typedef ICameraDeviceManager::IVirtualDevice IVirtualDevice;

  struct CreationInfo {
    ICameraDeviceManager* mDeviceManager = nullptr;
    std::shared_ptr<IVirtualDevice::Info> mStaticDeviceInfo = nullptr;
    ::android::sp<IMetadataProvider> mMetadataProvider = nullptr;
    ::android::sp<IMetadataConverter> mMetadataConverter = nullptr;
    std::map<uint32_t, ::android::sp<IMetadataProvider>>
        mPhysicalMetadataProviders;
  };

 public:  //// Interfaces.
  virtual auto open(
      const std::shared_ptr<IMtkcamDeviceCallback> callback)
      -> ::android::status_t = 0;

  virtual auto dumpState(IPrinter& printer,
                         const std::vector<std::string>& options) -> void = 0;
};
};  // namespace core
};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
using mcam::core::IMtkcamVirtualDeviceSession;
extern std::shared_ptr<IMtkcamVirtualDeviceSession>
  createMtkCameraDeviceSession(
    IMtkcamVirtualDeviceSession::CreationInfo const& info);

#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_IMTKCAMVIRTUALDEVICESESSION_H_
