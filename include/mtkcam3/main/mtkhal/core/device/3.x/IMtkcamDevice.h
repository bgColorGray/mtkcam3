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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_IMTKCAMDEVICE_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_IMTKCAMDEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "mtkcam3/main/mtkhal/core/common/1.x/types.h"
#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h"
#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h"
#include "mtkcam3/main/mtkhal/core/device/3.x/types.h"
#include "mtkcam/utils/debug/IPrinter.h"
#include "mtkcam/utils/metadata/IMetadata.h"

using NSCam::IMetadata;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {

class IMtkcamDevice {
 public:  //// Definitions.
  IMtkcamDevice() = default;
  virtual ~IMtkcamDevice() = default;

  // struct CreationInfo {};

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// V3.2
  virtual auto  getResourceCost(
                  CameraResourceCost& resourceCost) const
                  -> int = 0;

  virtual auto  getCameraCharacteristics(
                  IMetadata& cameraCharacteristics) const
                  -> int = 0;

  virtual auto  setTorchMode(TorchMode mode) -> int = 0;

  virtual auto  open(
                  const std::shared_ptr<IMtkcamDeviceCallback>& callback)
                  -> std::shared_ptr<IMtkcamDeviceSession> = 0;

  virtual auto  dumpState(
                  NSCam::IPrinter& printer,
                  const std::vector<std::string>& options)
                  -> void = 0;

 public:  //// V3.5
  virtual auto  getPhysicalCameraCharacteristics(
                  int32_t physicalId,
                  IMetadata& physicalMetadata) const
                  -> int = 0;

  virtual auto  isStreamCombinationSupported(
                  const StreamConfiguration& streams,
                  bool& queryStatus) const
                  -> int = 0;
};

};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_IMTKCAMDEVICE_H_
