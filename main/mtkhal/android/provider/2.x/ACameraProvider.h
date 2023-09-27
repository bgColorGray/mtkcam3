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
 * MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef MAIN_MTKHAL_ANDROID_PROVIDER_2_X_ACAMERAPROVIDER_H_
#define MAIN_MTKHAL_ANDROID_PROVIDER_2_X_ACAMERAPROVIDER_H_

#include <main/mtkhal/android/common/1.x/ACameraCommon.h>
#include <mtkcam3/main/mtkhal/android/provider/2.x/IACameraProvider.h>
#include <mtkcam3/main/mtkhal/android/provider/2.x/IACameraProviderCallback.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProviderCallback.h>

#include <memory>
#include <string>
#include <vector>

using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;

namespace mcam {
namespace android {

/******************************************************************************
 *
 ******************************************************************************/
class ACameraProvider : public IACameraProvider,
                        public IMtkcamProviderCallback,
                        public std::enable_shared_from_this<ACameraProvider> {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  struct CreationInfo {
    std::string providerName;
    std::shared_ptr<IMtkcamProvider> pProvider;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  std::string mName;
  std::shared_ptr<IMtkcamProvider> mProvider;
  std::shared_ptr<IACameraProviderCallback> mCallback;
  std::mutex mCallbackLock;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Instantiation.
  virtual ~ACameraProvider();
  explicit ACameraProvider(const CreationInfo& info);
  static auto create(const CreationInfo& info)
      -> std::shared_ptr<IACameraProvider>;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IACameraProvider Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto setCallback(
      const std::shared_ptr<IACameraProviderCallback>& callback) -> int;

  virtual auto getCameraIdList(std::vector<int32_t>& rCameraDeviceIds) const
      -> int;

  virtual auto isSetTorchModeSupported() const -> bool;

  virtual auto getDeviceInterface(const int32_t cameraDeviceId,
                                  std::shared_ptr<IACameraDevice>& rpDevice)
      -> int;

  virtual auto notifyDeviceStateChange(DeviceState newState) -> int;

  virtual auto getConcurrentStreamingCameraIds(
      std::vector<std::vector<int32_t>>& cameraIds) const -> int;

  virtual auto isConcurrentStreamCombinationSupported(
      const std::vector<CameraIdAndStreamCombination>& configs,
      bool& queryStatus) -> int;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Warning: This should be removed after custom zone moved down.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  virtual auto debug(std::shared_ptr<IPrinter> printer,
                     const std::vector<std::string>& options) -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IMtkcamProviderCallback Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  virtual auto cameraDeviceStatusChange(
      int32_t cameraDeviceId,
      mcam::CameraDeviceStatus newStatus) -> void;

  virtual auto torchModeStatusChange(int32_t cameraDeviceId,
                                     mcam::TorchModeStatus newStatus)
      -> void;

  virtual auto physicalCameraDeviceStatusChange(
      int32_t cameraDeviceId,
      int32_t physicalDeviceId,
      mcam::CameraDeviceStatus newStatus) -> void;
};

}  // namespace android
}  // namespace mcam

#endif  // MAIN_MTKHAL_ANDROID_PROVIDER_2_X_ACAMERAPROVIDER_H_
