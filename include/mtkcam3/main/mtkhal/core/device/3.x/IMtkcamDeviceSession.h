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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_IMTKCAMDEVICESESSION_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_IMTKCAMDEVICESESSION_H_


#include <memory>
#include <vector>

#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamOfflineSession.h"
#include "mtkcam3/main/mtkhal/core/device/3.x/types.h"
#include "mtkcam/utils/metadata/IMetadata.h"

using NSCam::IMetadata;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {

class IMtkcamDeviceSession {
 public:  //// Definitions.
  IMtkcamDeviceSession() = default;
  virtual ~IMtkcamDeviceSession() = default;

  // struct CreationInfo {};

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  //// V3.2
  virtual auto  constructDefaultRequestSettings(
                  const RequestTemplate type,
                  IMetadata& requestTemplate)
                  -> int = 0;

  /**
   * configureStreams(_3_3, _3_4, _3_5, _3_6)
   */
  virtual auto  configureStreams(
                  const StreamConfiguration& requestedConfiguration,
                  HalStreamConfiguration& halConfiguration)  // output
                  -> int = 0;

  /**
   * processCaptureRequest(_3_4)
   */
  virtual auto  processCaptureRequest(
                  const std::vector<CaptureRequest>& requests,
                  uint32_t& numRequestProcessed)  // output
                  -> int = 0;

  virtual auto  flush() -> int = 0;

  virtual auto  close() -> int = 0;

 public:  //// V3.5
  virtual auto  signalStreamFlush(
                  const std::vector<int32_t>& streamIds,
                  uint32_t streamConfigCounter)
                  -> void = 0;

  virtual auto  isReconfigurationRequired(
                  const IMetadata& oldSessionParams,
                  const IMetadata& newSessionParams,
                  bool& reconfigurationNeeded) const  // output
                  -> int = 0;

 public:  //// V3.6
  virtual auto  switchToOffline(
                  const std::vector<int64_t>& streamsToKeep,
                  CameraOfflineSessionInfo& offlineSessionInfo,
                  std::shared_ptr<IMtkcamOfflineSession>& offlineSession)
                  -> int = 0;

#if 1

 public:  //// WARNING: It should be removed after custom zone moved down.
  virtual auto getConfigurationResults(
    const uint32_t streamConfigCounter,
    ExtConfigurationResults& ConfigurationResults) -> int = 0;

#endif
};

};  // namespace mcam

/******************************************************************************
 *
 ******************************************************************************/
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_IMTKCAMDEVICESESSION_H_
