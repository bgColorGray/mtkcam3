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

#ifndef COREDEV_COREDEVICEBASE_H_
#define COREDEV_COREDEVICEBASE_H_

#include <stdio.h>

#include <mtkcam3/main/mtkhal/coredev/ICoreCallback.h>
#include <mtkcam3/main/mtkhal/coredev/ICoreDev.h>
#include <mtkcam3/main/mtkhal/coredev/types.h>
#include <mtkcam/def/CameraInfo.h>


#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using std::mutex;
using std::string;
using std::shared_ptr;
using std::make_shared;
using std::unordered_map;
using std::vector;

namespace NSCam {
namespace v3 {
namespace CoreDev {

class CoreDeviceBase : public ICoreDev {
 public:
  CoreDeviceBase();
  virtual ~CoreDeviceBase() = default;
  virtual int32_t configure(InternalStreamConfig const& config);
  virtual vector<std::shared_ptr<RequestData>> createReqData(
      shared_ptr<RequestParams> param);
  virtual int32_t performCallback(ResultData const& result);
  virtual int32_t processReq(int32_t reqNo,
                             vector<std::shared_ptr<RequestData>> reqs);
  virtual int32_t close();
  virtual int32_t flush();
  virtual android::sp<IImageStreamInfo> getStreamInfo(StreamId_T id) const;

 protected:
  virtual int32_t configureCore(InternalStreamConfig const& config __unused) {
    return 0;
  }
  virtual int32_t processReqCore(int32_t reqNo __unused,
                                 vector<std::shared_ptr<RequestData>> reqs
                                     __unused) {
    return 0;
  }
  virtual int32_t closeCore() { return 0; }
  virtual int32_t flushCore() { return 0; }

  virtual int32_t setRequestData(int32_t idx,
                                 shared_ptr<RequestParams> param,
                                 std::shared_ptr<RequestData> outData);

 protected:
  std::mutex mLock;
  std::mutex mCBLock;
  // configure
  std::string DevName;
  shared_ptr<IMetadata> mpConfigureParam;
  std::vector<int32_t> mSensorIds;
  std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> mvInputInfo;
  std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> mvOutputInfo;
  std::unordered_map<StreamId_T, android::sp<IMetaStreamInfo>> mvTuningMetaInfo;
  std::unordered_map<uint32_t, shared_ptr<RequestParams>> mvInflightReqs;
  std::shared_ptr<CameraInfoMapT> mCameraInfoMap = nullptr;
};

};      // namespace CoreDev
};      // namespace v3
};      // namespace NSCam
#endif  // COREDEV_COREDEVICEBASE_H_
