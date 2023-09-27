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
/* MediaTek Inc. (C) 2020. All rights reserved.
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
#ifndef COREDEV_COREDEVICESTREAMIMPL_H_
#define COREDEV_COREDEVICESTREAMIMPL_H_

#include "CoreDeviceBase.h"

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <semaphore.h>
#include <condition_variable>

using std::shared_ptr;
using std::vector;
namespace NSCam {
namespace v3 {
namespace CoreDev {
namespace model {

class CoreDeviceStreamImpl
    : public CoreDeviceBase,
      public std::enable_shared_from_this<CoreDeviceStreamImpl> {
 public:
  virtual ~CoreDeviceStreamImpl();
  explicit CoreDeviceStreamImpl(int32_t id);

 public:
  int32_t configureCore(InternalStreamConfig const& config) override;
  int32_t processReqCore(int32_t reqNo,
                                 vector<shared_ptr<RequestData>> reqs) override;
  int32_t closeCore() override;

  int32_t flushCore() override;

  auto onDequeRequest(shared_ptr<RequestData>& req) -> int32_t;
  auto onProcessFrame(shared_ptr<RequestData> req) -> int32_t;
 public:
  bool mStop = false;

 private:
  int32_t mLogLevel = 0;
  bool mOpenLog = false;
  int32_t mId = 0;
  int32_t mCamId = -1;
  std::timed_mutex mLock;
  int64_t streamID_f0 = -1;
  int64_t streamID_f1 = -1;
  int64_t streamID_ME = -1;
  int64_t streamID_tME = -1;

  // for memory copy
  pthread_t mMainThread;
  std::vector<shared_ptr<RequestData>> mReqList;
  mutable std::mutex mReqLock;
  std::condition_variable mReqCond;
  std::condition_variable mFlushCond;
  int64_t streamID_input = -1;
  int64_t streamID_output = -1;

  // for dual physical stream
  int64_t streamID_input2 = -1;
};

};  // namespace model
};  // namespace CoreDev
};  // namespace v3
};  // namespace NSCam

#endif  // COREDEV_COREDEVICESTREAMIMPL_H_
