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

#include <memory>
#include <string>
#include <vector>

#ifndef MAIN_MTKHAL_POSTPROC_INCLUDE_POSTPROCDEVICE_H_
#define MAIN_MTKHAL_POSTPROC_INCLUDE_POSTPROCDEVICE_H_

#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDevice.h"

namespace mcam {

class PostProcDevice : public IMtkcamDevice {
 public:  //// Definitions.
  explicit PostProcDevice(int32_t id);
  virtual ~PostProcDevice() = default;

 public:  //// V3.2
  auto  getResourceCost(
          CameraResourceCost& resourceCost __unused) const
          -> int override { return 0; }

  auto  setTorchMode(TorchMode mode __unused) -> int override { return 0; }

  auto  getPhysicalCameraCharacteristics(
          int32_t physicalId __unused,
          IMetadata& physicalMetadata __unused) const
          -> int override { return 0; }

  auto  isStreamCombinationSupported(
          const StreamConfiguration& streams __unused,
          bool& queryStatus __unused) const
          -> int override { return 0; }

 public:
  auto  getCameraCharacteristics(
          IMetadata& cameraCharacteristics) const
          -> int override;

  auto  open(
          const std::shared_ptr<IMtkcamDeviceCallback>& callback)
          -> std::shared_ptr<IMtkcamDeviceSession> override;

  auto  dumpState(
          NSCam::IPrinter& printer,
          const std::vector<std::string>& options)
          -> void override;

 private:
  int32_t mType;
};
}  // namespace mcam

#endif  // MAIN_MTKHAL_POSTPROC_INCLUDE_POSTPROCDEVICE_H_
