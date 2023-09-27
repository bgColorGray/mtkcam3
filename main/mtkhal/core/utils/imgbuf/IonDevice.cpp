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

#define LOG_TAG "mtkcam-IonDevice"

#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IIonDevice.h>
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam/utils/std/ULog.h>
//

#include <sys/mman.h>
#include <sys/resource.h>
#include <cutils/properties.h>
#include <utils/Mutex.h>
#include <ion.h>
#include <ion/ion.h>
//

#include <memory>
#include <vector>

#include <atomic>
#include <list>
#include <string>
#include <unordered_map>
//

#include "include/MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);
//
// using namespace NSCam;

using NSCam::IDebuggee;
using mcam::IIonDevice;
using mcam::IIonDeviceProvider;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGV(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGI_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGI(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if (CC_UNLIKELY(cond)) {  \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

/******************************************************************************
 *
 ******************************************************************************/
namespace {
class IonDeviceProviderImpl : public IIonDeviceProvider,
                              public IDebuggee {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  struct IonDevice;
  using DeviceMapT = std::unordered_map<std::string, std::weak_ptr<IonDevice>>;

  struct IonDevice : public IIonDevice {
   public:
    std::string const mUserName;
    struct timespec mTimestamp;
    int mDeviceFd = -1;

   public:
    explicit IonDevice(char const* userName);
    virtual ~IonDevice();
    virtual auto getDeviceFd() const -> int { return mDeviceFd; }
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  static std::string mDebuggeeName;
  std::shared_ptr<NSCam::IDebuggeeCookie> mDebuggeeCookie = nullptr;
  NSCam::Utils::LogTool* mLogTool = nullptr;

  mutable android::Mutex mLock;
  DeviceMapT mDeviceMap;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////            IDebuggee
  virtual auto debuggeeName() const -> std::string { return mDebuggeeName; }
  virtual auto debug(android::Printer& printer,
                     const std::vector<std::string>& options __unused) -> void {
    print(printer);
  }

 public:
  virtual int32_t queryLegacyIon();

 public:
  static auto get() -> IonDeviceProviderImpl*;
  IonDeviceProviderImpl();
  ~IonDeviceProviderImpl();
  auto detach(IonDevice* pDevice) -> void;
  auto attach(std::shared_ptr<IonDevice>& pDevice) -> bool;

 public:
  virtual auto print(android::Printer& printer) const -> void;

  virtual auto makeIonDevice(char const* userName, int useSharedDeviceFd)
      -> std::shared_ptr<IIonDevice>;
};
  std::string IonDeviceProviderImpl::mDebuggeeName =
    "mcam::IIonDeviceProvider";
};  // namespace

/******************************************************************************
 *
 ******************************************************************************/


/******************************************************************************
 *
 ******************************************************************************/
auto IonDeviceProviderImpl::get() -> IonDeviceProviderImpl* {
  // Make sure IDebuggeeManager singleton is fully constructed before this
  // singleton. So that it's safe to access IDebuggeeManager instance from this
  // singleton's destructor.
  static auto pDbgMgr = NSCam::IDebuggeeManager::get();
  static auto inst = std::make_shared<IonDeviceProviderImpl>();
  static auto init __unused = []() {
    if (CC_UNLIKELY(inst == nullptr)) {
      MY_LOGF("Fail on std::make_shared<IonDeviceProviderImpl>()");
      return false;
    }
    if (CC_LIKELY(pDbgMgr)) {
      inst->mDebuggeeCookie = pDbgMgr->attach(inst);
    }
    return true;
  }();
  MY_LOGF_IF((inst == nullptr),
             "nullptr instance on IonDeviceProviderImpl::get()");
  return inst.get();
}

/******************************************************************************
 *
 ******************************************************************************/
auto IIonDeviceProvider::get() -> IIonDeviceProvider* {
  return IonDeviceProviderImpl::get();
}

/******************************************************************************
 *
 ******************************************************************************/
IonDeviceProviderImpl::IonDeviceProviderImpl()
    : IIonDeviceProvider(), mLogTool(NSCam::Utils::LogTool::get()) {
  // After refactoring, the number of ion client is redecued so
  // we do not need to share ion fd anymore.
}

/******************************************************************************
 *
 ******************************************************************************/
IonDeviceProviderImpl::~IonDeviceProviderImpl() {
  MY_LOGD("+ mDebuggeeCookie:%p", mDebuggeeCookie.get());
  mDebuggeeCookie = nullptr;
  MY_LOGD("-");
}

/******************************************************************************
 *
 ******************************************************************************/
auto IonDeviceProviderImpl::print(android::Printer& printer) const -> void {
  DeviceMapT map;
  if (mLock.timedLock(50000000 /* 50ms */) == 0) {
    map = mDeviceMap;
    mLock.unlock();
  } else {
    printer.printLine("Timed out on lock");
  }

  for (auto const& iter : map) {
    auto s = iter.second.lock();
    if (s == nullptr) {
      printer.printLine("dead user (outside locking)");
    } else {
      printer.printFormatLine(
          "  %s fd=%d %s",
          mLogTool->convertToFormattedLogTime(&s->mTimestamp).c_str(),
          s->mDeviceFd, s->mUserName.c_str());
    }
    s = nullptr;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto IonDeviceProviderImpl::makeIonDevice(char const* userName,
                                          int useSharedDeviceFd)
    -> std::shared_ptr<IIonDevice> {
  // 1. parse user name
  std::string name(userName);
  if (name.find("-") != std::string::npos)
    name = name.substr(0, name.find("-"));

  std::shared_ptr<IonDevice> pDevice;
  {
    // 2. find if IonDevice exist.
    android::Mutex::Autolock _l(mLock);
    auto it = mDeviceMap.find(name);
    if (it != mDeviceMap.end()) {
      pDevice = it->second.lock();
      if (pDevice)
        return pDevice;
    }
  }

  // 3. create new one
  pDevice = std::make_shared<IonDevice>(name.c_str());

  if (pDevice == nullptr) {
    MY_LOGE("User %s: fail to make a new device", name.c_str());
    return nullptr;
  }

  if (!attach(pDevice)) {
    MY_LOGE("User %s: fail to attach the device", name.c_str());
    pDevice = nullptr;
    return nullptr;
  }

  return pDevice;
}

/******************************************************************************
 *
 ******************************************************************************/
int32_t IonDeviceProviderImpl::queryLegacyIon() {
  static std::atomic_int32_t ver = -1;

  if (ver != -1)
    return ver;

  {
    int fd = ion_open();

    if (fd < 0) {
      MY_LOGA("open /dev/ion failed...");
    } else {
      if (ion_is_legacy(fd)) {
        ver = 1;
      } else {
        ver = 0;
      }

      if (ion_close(fd) < 0)
        MY_LOGE("cannot close /dev/ion");
    }
  }

  return ver;
}

/******************************************************************************
 *
 ******************************************************************************/
auto IonDeviceProviderImpl::attach(std::shared_ptr<IonDevice>& pDevice)
    -> bool {
  // return false if ion_open fail
  if (pDevice->mDeviceFd <= 0)
    return false;

  {
    android::Mutex::Autolock _l(mLock);

    mDeviceMap[pDevice->mUserName] = pDevice;
    mLogTool->getCurrentLogTime(&pDevice->mTimestamp);
  }

  return true;
}

/******************************************************************************
 *
 ******************************************************************************/
auto IonDeviceProviderImpl::detach(IonDevice* pDevice) -> void {
#if defined(MTK_ION_SUPPORT)
  ::ion_close(pDevice->mDeviceFd);
#else
  MY_LOGE("MTK_ION_SUPPORT is not defined");
#endif
}

/******************************************************************************
 *
 ******************************************************************************/
IonDeviceProviderImpl::IonDevice::IonDevice(char const* userName)
    : IIonDevice(), mUserName(userName) {
  ::memset(&mTimestamp, 0, sizeof(mTimestamp));

#if defined(MTK_ION_SUPPORT)
  if (IIonDeviceProvider::get()->queryLegacyIon() > 0) {
    mDeviceFd = ::mt_ion_open(userName);
  } else {
    mDeviceFd = ion_open();
  }
  if (mDeviceFd <= 0)
    MY_LOGE("user:%s fail to mt_ion_open()", userName);
#else
  mDeviceFd = -1;
  MY_LOGE("MTK_ION_SUPPORT is not defined");
#endif
}

/******************************************************************************
 *
 ******************************************************************************/
IonDeviceProviderImpl::IonDevice::~IonDevice() {
  if (mDeviceFd <= 0) {
    // Data members (e.g. mIterator) are not initialized if mDeviceFd==-1.
    // It may happen such as: attach() fails -> ~IonDevice() -> detach()
    MY_LOGW("Don't detach %p since mDeviceFd is invalid", this);
  } else {
    IonDeviceProviderImpl::get()->detach(this);
  }
}
