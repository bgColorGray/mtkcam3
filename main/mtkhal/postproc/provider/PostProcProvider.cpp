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

#define LOG_TAG "MTKHAL/PostProcProvider"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../include/PostProcDevice.h"
#include "DeviceList.h"
#include "mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h"
#include "mtkcam/utils/std/Log.h"
#include "mtkcam/utils/std/Trace.h"
#include "mtkcam/utils/std/ULog.h"


CAM_ULOG_DECLARE_MODULE_ID(MOD_POSTPROC_DEVICE);

#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)

#if 1
#define FUNC_START MY_LOGD("+")
#define FUNC_END MY_LOGD("-")
#else
#define FUNC_START
#define FUNC_END
#endif

namespace mcam {

/******************************************************************************
 *
 ******************************************************************************/
class PostProcProviderImp : public IMtkcamProvider {
 public:  //// Definitions.
  PostProcProviderImp();
  virtual ~PostProcProviderImp() = default;

  auto  setCallback(
          const std::shared_ptr<IMtkcamProviderCallback>& callback __unused)
          -> int override { return 0; }

  auto  getVendorTags(std::vector<VendorTagSection>& sections __unused)
          const -> void override {}
  auto  isSetTorchModeSupported()
          const -> bool override { return false; }
  auto  notifyDeviceStateChange(DeviceState newState __unused)
          -> int override { return 0; }
  auto  getConcurrentStreamingCameraIds(
          std::vector<std::vector<int32_t>>& cameraIds __unused) const
          -> int override { return 0; };
  auto  isConcurrentStreamCombinationSupported(
          std::vector<CameraIdAndStreamCombination>& configs __unused,
          bool& queryStatus __unused) const -> int override
          { return 0; }

  //  implementation
  auto  getCameraIdList(std::vector<int32_t>& rCameraDeviceIds) const
          -> int override;
  auto  getDeviceInterface(
          const int32_t cameraDeviceId,
          std::shared_ptr<IMtkcamDevice>& rpDevice) const
          -> int override;
  auto  debug(
          std::shared_ptr<IPrinter> printer,
          const std::vector<std::string>& options)
          -> void override;

 private:
  std::string mName = "PostProcProvider";
  std::map<int32_t, std::shared_ptr<IMtkcamDevice>> mDevTable;
};

static std::mutex gLock;
std::shared_ptr<mcam::IMtkcamProvider> gPostProcProvider = nullptr;

/******************************************************************************
 *
 ******************************************************************************/
PostProcProviderImp::PostProcProviderImp() {
  for (auto id : gSupportedDevList) {
    mDevTable.emplace(id, std::make_shared<PostProcDevice>(id));
  }
}

auto PostProcProviderImp::getCameraIdList(
    std::vector<int32_t>& rCameraDeviceIds) const
    -> int {
  rCameraDeviceIds = gSupportedDevList;
  return 0;
}

auto PostProcProviderImp::getDeviceInterface(
    const int32_t cameraDeviceId,
    std::shared_ptr<IMtkcamDevice>& rpDevice) const
    -> int {
  if (mDevTable.count(cameraDeviceId) == 0) {
    MY_LOGE("getDeviceInterface failed, not support id : %d", cameraDeviceId);
    return -1;
  }
  rpDevice = mDevTable.at(cameraDeviceId);
  return 0;
}

auto PostProcProviderImp::debug(
    std::shared_ptr<IPrinter> printer __unused,
    const std::vector<std::string>& options __unused)
    -> void {
  return;
}

/******************************************************************************
 *
 ******************************************************************************/

}  // namespace mcam

std::shared_ptr<mcam::IMtkcamProvider>
getPostProcProviderInstance(
  IProviderCreationParams const* params __unused) {
  std::lock_guard<std::mutex> _Lock(mcam::gLock);
  if (mcam::gPostProcProvider == nullptr) {
    mcam::gPostProcProvider =
     std::make_shared<mcam::PostProcProviderImp>();
  }
  return mcam::gPostProcProvider;
}

