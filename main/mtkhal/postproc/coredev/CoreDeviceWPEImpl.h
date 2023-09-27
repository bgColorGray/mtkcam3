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
#ifndef COREDEV_COREDEVICEWPEIMPL_H_
#define COREDEV_COREDEVICEWPEIMPL_H_

#include "CoreDeviceBase.h"

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <list>
#include <utils/Thread.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IGraphicImageBufferHeap.h> // mcam
#include <mtkcam/utils/imgbuf/IGraphicImageBufferHeap.h>                   // NSCam
#include <DpIspStream.h>

using namespace android;
using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSPostProc;


using std::shared_ptr;
using std::vector;
namespace NSCam {
namespace v3 {
namespace CoreDev {
namespace model {


class CoreDeviceWPEImpl
    : public CoreDeviceBase,
      public std::enable_shared_from_this<CoreDeviceWPEImpl> {
 public:
  virtual ~CoreDeviceWPEImpl();
  explicit CoreDeviceWPEImpl(int32_t id);

 public:
  int32_t configureCore(InternalStreamConfig const& config) override;
  int32_t processReqCore(int32_t reqNo,
                                 vector<shared_ptr<RequestData>> reqs) override;
  int32_t closeCore() override;

  int32_t flushCore() override;

 public:
  virtual int32_t convertReqToNormalStream(shared_ptr<RequestData>& req);
  virtual int32_t convertReqToDpIspStream(shared_ptr<RequestData>& req);
  auto onDequeRequest(shared_ptr<RequestData>& req) -> int32_t;
  auto processReq( shared_ptr<RequestData> req )  -> int32_t;
  virtual auto uninit() -> int32_t;

  bool mStop = false;

 protected:
  static auto WPECallback_Pass(QParams& rParams)  -> void;
  auto convertImageBufferHeap(std::shared_ptr<mcam::IImageBufferHeap> pMcamHeap)
       -> ::android::sp<NSCam::IImageBufferHeap>;

  mutable Mutex mReqQueueLock;
  android::Condition mRequestQueueCond;
  android::Condition mFlushCond;
  NSIoPipe::NSPostProc::INormalStream* mpStream = nullptr;
  std::list<shared_ptr<RequestData>>  mvReqQueue;
  DpIspStream* mpDpIspStream = nullptr;

 private:
  int32_t mLogLevel = 0;
  bool mOpenLog = false;
  bool dumpWPE = false;
  bool dumpImage = false;
  bool mPurePQDIP = false;
  bool mInited = false;
  int32_t mId = 0;
  int32_t mCamId = -1;
  std::timed_mutex mLock;
  int64_t streamID_srcImage = -1;
  int64_t streamID_warpMap = -1;
  pthread_t mMainThread;

  struct EnqData
  {
    uint32_t requestNo;
    std::weak_ptr<CoreDeviceWPEImpl> mwpDevice;
    vector<shared_ptr<mcam::IImageBufferHeap>> mmcambuf;
    vector<::android::sp<NSCam::IImageBufferHeap>> mNSCambuf;
  };
};

};  // namespace model
};  // namespace CoreDev
};  // namespace v3
};  // namespace NSCam

#endif  // COREDEV_COREDEVICEWPEIMPL_H_
