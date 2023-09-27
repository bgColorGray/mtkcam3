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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_PROVIDER_2_X_IMTKCAMPROVIDER_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_PROVIDER_2_X_IMTKCAMPROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "mtkcam3/main/mtkhal/core/common/1.x/types.h"
#include "mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProviderCallback.h"
#include "mtkcam3/main/mtkhal/core/provider/2.x/types.h"
#include "mtkcam/utils/debug/IPrinter.h"

using NSCam::IPrinter;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {

class IMtkcamDevice;

class IMtkcamProvider {
 public:  //// Definitions.
  IMtkcamProvider() = default;
  virtual ~IMtkcamProvider() = default;

  // struct CreationInfo {};

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// V2.4
  virtual auto setCallback(
      const std::shared_ptr<IMtkcamProviderCallback>& callback)
      -> int = 0;

  virtual auto  getVendorTags(std::vector<VendorTagSection>& sections) const
                  -> void = 0;

  /**
   * The device instance names are listed by intergers.
   */
  virtual auto  getCameraIdList(std::vector<int32_t>& rCameraDeviceIds) const
                  -> int = 0;

  virtual auto  isSetTorchModeSupported() const -> bool = 0;

  virtual auto  getDeviceInterface(
                  const int32_t cameraDeviceId,
                  std::shared_ptr<IMtkcamDevice>& rpDevice) const
                  -> int = 0;

 public:  //// V2.5
  virtual auto  notifyDeviceStateChange(DeviceState newState) -> int = 0;

 public:  //// V2.6
  virtual auto  getConcurrentStreamingCameraIds(
                  std::vector<std::vector<int32_t>>& cameraIds) const
                  -> int = 0;

  virtual auto  isConcurrentStreamCombinationSupported(
                  std::vector<CameraIdAndStreamCombination>& configs,
                  bool& queryStatus) const
                  -> int = 0;

 public:  ////
  virtual auto  debug(
                  std::shared_ptr<IPrinter> printer,
                  const std::vector<std::string>& options)
                  -> void = 0;
};

};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
struct IProviderCreationParams {
  std::string providerName;
};

// implemented by custom zone library: libmtkcam_hal_custom.so
extern std::shared_ptr<mcam::IMtkcamProvider>
getCustomProviderInstance(IProviderCreationParams const* params);

// implemented by mtk hal library: libmtkcam_hal_core.so
extern std::shared_ptr<mcam::IMtkcamProvider>
getMtkcamProviderInstance(IProviderCreationParams const* params);

// implemented by post processing library: libmtkcam_postprocprovider.so
extern std::shared_ptr<mcam::IMtkcamProvider>
getPostProcProviderInstance(IProviderCreationParams const* params);

/******************************************************************************
 *
 ******************************************************************************/
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_PROVIDER_2_X_IMTKCAMPROVIDER_H_
