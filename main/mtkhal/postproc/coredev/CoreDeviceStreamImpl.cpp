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
#define LOG_TAG "MtkCam/CoreDeviceStream"

#include "coredev/CoreDeviceStreamImpl.h"
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include "mtkcam/utils/debug/Properties.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_HAL_SERVER);

#define Core_OK 0
#define Not_OK -1

using std::make_shared;
using std::move;
using std::shared_ptr;
using std::vector;

namespace NSCam {
namespace v3 {
namespace CoreDev {
namespace model {

#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)

#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)

static MVOID* mainThreadLoop(MVOID* arg) {
  CoreDeviceStreamImpl* impl = reinterpret_cast<CoreDeviceStreamImpl*>(arg);
  shared_ptr<RequestData> pReq = nullptr;
  while (!impl->mStop) {
    if (0 == impl->onDequeRequest(pReq) && pReq != 0) {
      impl->onProcessFrame(pReq);
    }
    pReq = nullptr;
  }
  return nullptr;
}

CoreDeviceStreamImpl::~CoreDeviceStreamImpl() {
  MY_LOGI("destroy CoreDeviceStreamImpl");
}

CoreDeviceStreamImpl::CoreDeviceStreamImpl(int32_t id) : mId(id) {
  MY_LOGI("create CoreDeviceStreamImpl");
}

auto CoreDeviceStreamImpl::configureCore(InternalStreamConfig const& config)
    -> int {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("configure + ");
  mLogLevel = NSCam::Utils::Properties::property_get_int32(
            "vendor.debug.camera.coredevice.stream.dump", 0);
  if (mLogLevel > 0) {
    mOpenLog = true;
  }
  if (config.vInputInfo.size() == 0) {
    MY_LOGW("no input configure");
    return -1;
  }
  if (config.vOutputInfo.size() == 0) {
    MY_LOGW("no output configure");
    return -1;
  }
  streamID_input = config.vInputInfo.begin()->first;
  streamID_output = config.vOutputInfo.begin()->first;

  // for 2 physical stream, only
  if (config.vInputInfo.size() > 1) {
    auto it = std::next(config.vInputInfo.begin(), 1);
    streamID_input2 = it->first;
    MY_LOGD("add in2(0x%llX)", streamID_input2);
  }

  mStop = false;
  pthread_create(&mMainThread, NULL, mainThreadLoop, this);
  pthread_setname_np(mMainThread, "PostProc@VRP");
  MY_LOGD("in(0x%llX), out(%llX)", streamID_input, streamID_output);
  MY_LOGD("configure - ");
  return Core_OK;
}

auto CoreDeviceStreamImpl::processReqCore(int32_t reqNo,
                                          vector<shared_ptr<RequestData>> reqs)
    -> int {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD_IF(mOpenLog, "processReqCore + ");
  int err = Core_OK;

  for (size_t r = 0; r < reqs.size(); ++r) {
    std::unique_lock<std::mutex> _reqLock(mReqLock);
    mReqList.push_back(reqs[r]);
  }
  mReqCond.notify_all();

  MY_LOGD_IF(mOpenLog, "processReqCore - ");
  return err;
}

auto CoreDeviceStreamImpl::closeCore() -> int32_t {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("closeCore + ");
  flushCore();
  mStop = true;
  mReqCond.notify_all();
  MY_LOGD("wait thread");
  pthread_join(mMainThread, NULL);
  MY_LOGD("closeCore - ");
  return Core_OK;
}

auto CoreDeviceStreamImpl::flushCore() -> int32_t {
  MY_LOGD("flushCore + ");
  std::unique_lock<std::mutex> _reqLock(mReqLock);
  if (!mReqList.empty()) {
    MY_LOGD("wait requests callback");
    mFlushCond.wait(_reqLock);
  }
  MY_LOGD("flushCore - ");
  return Core_OK;
}

auto CoreDeviceStreamImpl::onDequeRequest(shared_ptr<RequestData>& req)
    -> int32_t {
  std::unique_lock<std::mutex> _l(mReqLock);
  MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_0");
  while (mReqList.empty() && !mStop) {
    mReqCond.wait(_l);
  }
  if (!mReqList.empty()) {
    MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_1");
    req = *mReqList.begin();
    mReqList.erase(mReqList.begin());
  } else {
    MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_2");
    mFlushCond.notify_all();
  }
  return 0;
}

auto CoreDeviceStreamImpl::onProcessFrame(shared_ptr<RequestData> req)
    -> int32_t {
  std::shared_ptr<mcam::IImageBufferHeap> inBuf;
  if (req->mInBufs.count(streamID_input2) > 0) {
    inBuf = req->mInBufs.at(streamID_input2);
    MY_LOGD("set inbuf2: 0x%X", streamID_input2);
  } else {
    inBuf = req->mInBufs.at(streamID_input);
  }

  auto outBuf = req->mOutBufs.at(streamID_output);
  if (inBuf->getImgFormat() != outBuf->getImgFormat()) {
    MY_LOGW("format is different, cannot copy");
  } else {
    inBuf->lockBuf("CoreDevVRP-IN", mcam::eBUFFER_USAGE_SW_MASK);
    outBuf->lockBuf("CoreDevVRP-OUT", mcam::eBUFFER_USAGE_SW_MASK);
    for (int i = 0; i < inBuf->getPlaneCount(); i++) {
      auto inPtr = inBuf->getBufVA(i);
      auto inBufSize = inBuf->getBufSizeInBytes(i);
      auto outPtr = outBuf->getBufVA(i);
      auto outBufSize = outBuf->getBufSizeInBytes(i);
      MY_LOGD("copy buffer : addr(0x%" PRIxPTR
        ") size(%d) -> addr(0x%" PRIxPTR
        ") size(%d)",
        inPtr, inBufSize, outPtr, outBufSize);
      memcpy((void *)outPtr, (void *)inPtr, inBufSize);
    }
    inBuf->unlockBuf("CoreDevVRP-IN");
    outBuf->unlockBuf("CoreDevVRP-OUT");
  }
  ResultData result;
  result.requestNo = req->mRequestNo;
  result.bCompleted = true;
  performCallback(result);
  return 0;
}

};  // namespace model
};  // namespace CoreDev
};  // namespace v3
};  // namespace NSCam
