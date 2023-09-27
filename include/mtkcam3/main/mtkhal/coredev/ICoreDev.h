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

#ifndef INCLUDE_MTKCAM_INTERFACES_COREDEV_ICOREDEV_H_
#define INCLUDE_MTKCAM_INTERFACES_COREDEV_ICOREDEV_H_

#include <stdio.h>

#include <mtkcam3/main/mtkhal/coredev/ICoreCallback.h>
#include <mtkcam3/main/mtkhal/coredev/types.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace NSCam {
namespace v3 {
namespace CoreDev {

struct SensorData {
  int32_t sensorId;
  std::shared_ptr<IMetadata> pAppCameraResult = nullptr;
  std::shared_ptr<IMetadata> pHalCameraResult = nullptr;
  std::shared_ptr<mcam::IImageBufferHeap> tuningBuf = nullptr;
  std::unordered_map<StreamId_T, std::shared_ptr<mcam::IImageBufferHeap>>
      inBufs;  // the buffer is output by camera sensor
};

struct RequestParams {
  uint32_t requestNo = 0;
  std::weak_ptr<ICoreCallback> pCallback;
  std::shared_ptr<IMetadata> pAppControl = nullptr;
  std::vector<std::vector<SensorData>> inData;
  std::unordered_map<StreamId_T, std::shared_ptr<mcam::IImageBufferHeap>> outBufs;
};

struct RequestData {
  explicit RequestData(std::string Name) { mUserName = Name; }

  uint32_t mRequestNo;
  uint32_t mIndex = 0;
  std::string mUserName;
  std::shared_ptr<IMetadata> mpAppControl = nullptr;
  std::unordered_map<int32_t /*sensor id*/, std::shared_ptr<IMetadata>>
      mvAppCameraResult;
  std::unordered_map<int32_t /*sensor id*/, std::shared_ptr<IMetadata>>
      mvPrivateMeta;
  std::shared_ptr<std::vector<uint8_t>> pSTTData = nullptr;
  std::unordered_map<StreamId_T, std::shared_ptr<mcam::IImageBufferHeap>> mInBufs;
  std::unordered_map<StreamId_T, std::shared_ptr<mcam::IImageBufferHeap>> mOutBufs;
};

class ICoreDev {
 public:
  static auto createCoreDev(CoreDevType type) -> std::shared_ptr<ICoreDev>;
  virtual ~ICoreDev() = default;
  virtual std::vector<std::shared_ptr<RequestData>> createReqData(
      std::shared_ptr<RequestParams> param) = 0;
  virtual int32_t configure(InternalStreamConfig const& config) = 0;
  virtual int32_t performCallback(ResultData const& result) = 0;
  virtual android::sp<IImageStreamInfo> getStreamInfo(
      StreamId_T const id) const = 0;
  virtual int32_t processReq(
      int32_t reqNo,
      std::vector<std::shared_ptr<RequestData>> reqs) = 0;
  virtual int32_t close() = 0;
  virtual int32_t flush() = 0;
};

};      // namespace CoreDev
};      // namespace v3
};      // namespace NSCam
#endif  // INCLUDE_MTKCAM_INTERFACES_COREDEV_ICOREDEV_H_
