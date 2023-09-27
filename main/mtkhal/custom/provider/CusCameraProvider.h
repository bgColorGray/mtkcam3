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

#ifndef MAIN_MTKHAL_CUSTOM_PROVIDER_CUSCAMERAPROVIDER_H_
#define MAIN_MTKHAL_CUSTOM_PROVIDER_CUSCAMERAPROVIDER_H_

#include <dlfcn.h>

#include <memory>
#include <string>
#include <vector>

#include "../../../../include/mtkcam3/main/mtkhal/core/common/1.x/types.h"
#include "../../../../include/mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h"
#include "../../../../include/mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProviderCallback.h"
#include "../../../../include/mtkcam3/main/mtkhal/core/provider/2.x/types.h"
//
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace custom {
//
using mcam::DeviceState;
using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;
using mcam::Status;

/******************************************************************************
 *
 ******************************************************************************/
class CusCameraProvider
    : public mcam::IMtkcamProvider,
      public mcam::IMtkcamProviderCallback,
      public std::enable_shared_from_this<CusCameraProvider> {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  struct CreationInfo {
    std::string providerName;
    std::shared_ptr<IMtkcamProvider> pProvider;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                Data Members.
  std::mutex mProviderCallbackLock;
  std::shared_ptr<mcam::IMtkcamProviderCallback> mProviderCallback =
      nullptr;
  mutable std::mutex mMtkcamProviderLock;
  std::shared_ptr<mcam::IMtkcamProvider> mMtkcamProvider = nullptr;

 protected:  ////                Operations.
  // HAL3+
  auto getSafeMtkCamProvider() const
      -> std::shared_ptr<mcam::IMtkcamProvider>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  virtual ~CusCameraProvider();
  explicit CusCameraProvider(const CreationInfo& info);
  static auto create(const CreationInfo& info)
      -> std::shared_ptr<IMtkcamProvider>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::hidl_death_recipient
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::provider::Vx_x::ICameraProvider Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// V2_4
  virtual int setCallback(
      const std::shared_ptr<mcam::IMtkcamProviderCallback>& callback);

  virtual auto getVendorTags(
      std::vector<mcam::VendorTagSection>& sections) const -> void;

  virtual int getCameraIdList(std::vector<int32_t>& rCameraDeviceIds) const;

  virtual bool isSetTorchModeSupported() const;

  virtual int getDeviceInterface(
      const int32_t cameraDeviceId,
      std::shared_ptr<mcam::IMtkcamDevice>& rpDevice) const;

 public:  //// V2_5
  virtual int notifyDeviceStateChange(mcam::DeviceState newState);

 public:  //// V2_6
  virtual int getConcurrentStreamingCameraIds(
      std::vector<std::vector<int32_t>>& cameraIds) const;

  virtual int isConcurrentStreamCombinationSupported(
      std::vector<mcam::CameraIdAndStreamCombination>& configs,
      bool& queryStatus) const;
  virtual void debug(std::shared_ptr<NSCam::IPrinter> printer,
                     const std::vector<std::string>& options);

  /******************************************************************************
   * ::android::hardware::camera::provider::Vx_x::ICameraProviderCallback
   *Interfaces.
   ******************************************************************************/
 public:
  virtual void cameraDeviceStatusChange(int32_t cameraDeviceId,
                                        CameraDeviceStatus newStatus);

  virtual void torchModeStatusChange(int32_t cameraDeviceId,
                                     TorchModeStatus newStatus);
  virtual void physicalCameraDeviceStatusChange(int32_t cameraDeviceId,
                                                int32_t physicalDeviceId,
                                                CameraDeviceStatus newStatus);
};

}  // namespace custom
}  // namespace mcam

#endif  // MAIN_MTKHAL_CUSTOM_PROVIDER_CUSCAMERAPROVIDER_H_
