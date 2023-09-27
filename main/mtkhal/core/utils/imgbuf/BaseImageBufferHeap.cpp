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

#define LOG_TAG "MtkCam/BaseHeap"

#include <mtkcam/utils/std/ULog.h>

#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <ion.h>
#include <ion/ion.h>
#include <linux/dma-buf.h>
#include <linux/ion_drv.h>  // for ION_CMDS, ION_CACHE_SYNC_TYPE
#include <mt_iommu_port.h>

#include <memory>
#include <string>
#include <vector>

#include <list>

#include "include/BaseImageBufferHeap.h"
#include "include/BaseImageBuffer.h"
#include "include/MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);

// using namespace NSCam::Utils::ULog;
// using namespace android;
// using namespace NSCam;
// using namespace NSCam::Utils;
// using namespace mcam::NSImageBufferHeap;
// using namespace mcam::NSImageBuffer;
//
using android::sp;
using android::Vector;
using android::String8;
using NSCam::IDebuggee;
using mcam::NSImageBufferHeap::IManager;
using mcam::IImageBufferHeap;
using mcam::IImageBuffer;
using mcam::NSImageBuffer::BaseImageBuffer;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;
using mcam::NSImageBuffer::IRegistrationCookie;
using NSCam::Utils::dumpCallStack;
using NSCam::Utils::Format::checkValidFormat;
using NSCam::Utils::Format::queryPlaneCount;
using NSCam::Utils::Format::queryPlaneBitsPerPixel;
using NSCam::Utils::Format::queryImageBitsPerPixel;
using NSCam::Utils::Format::queryPlaneWidthInPixels;
using NSCam::Utils::Format::queryPlaneHeightInPixels;
using NSCam::Utils::Format::checkValidBufferInfo;
using NSCam::Utils::ULog::ULogPrinter;
//using NSCam::Utils::ULog::DetailsType;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV("[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD("[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI("[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW("[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME("[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_ULOGM_ASSERT(0, "[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_ULOGM_FATAL("[%s::%s] " fmt, getMagicName(), __FUNCTION__, ##arg)
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
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

/******************************************************************************
 *
 ******************************************************************************/
namespace {
struct ManagerImpl : public IManager, public IDebuggee {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  struct Cookie;
  using CookieListT = std::list<std::weak_ptr<Cookie>>;
  struct Cookie : public IRegistrationCookie {
    CookieListT::iterator mIterator;
    std::weak_ptr<BaseImageBufferHeap> mHeap;

    virtual ~Cookie() {
      mHeap.reset();
      ManagerImpl::get()->detach(this);
    }
  };

 protected:
  static std::string mDebuggeeName;
  std::shared_ptr<NSCam::IDebuggeeCookie> mDebuggeeCookie = nullptr;

  mutable android::Mutex mLock;
  CookieListT mCookieList;

 public:  ////    IDebuggee
  virtual auto debuggeeName() const -> std::string { return mDebuggeeName; }
  virtual auto debug(android::Printer& printer,
                     const std::vector<std::string>& options __unused) -> void {
    CookieListT list;
    {
      android::Mutex::Autolock _l(mLock);
      list = mCookieList;
    }

    for (auto const& v : list) {
      auto s = v.lock();
      if (s == nullptr || s->mHeap.expired()) {
        printer.printLine("dead image buffer heap (outside locking)");
      } else {
        std::shared_ptr<BaseImageBufferHeap> heap = s->mHeap.lock();
        if (heap != nullptr) {
          heap->print(printer, 2);
        }
      }
      s = nullptr;
    }
  }

 public:
  static auto get() -> ManagerImpl* {
    static auto inst = std::make_shared<ManagerImpl>();
    static auto init __unused = []() {
      if (CC_UNLIKELY(inst == nullptr)) {
        CAM_LOGF("Fail on std::make_shared<ManagerImpl>()");
        return false;
      }
      if (auto pDbgMgr = NSCam::IDebuggeeManager::get()) {
        inst->mDebuggeeCookie = pDbgMgr->attach(inst);
      }
      return true;
    }();
    return inst.get();
  }

  virtual auto detach(Cookie* c) -> void {
    android::Mutex::Autolock _l(mLock);
    c->mHeap.reset();
    if (mCookieList.end() != c->mIterator) {
      mCookieList.erase(c->mIterator);
      c->mIterator = mCookieList.end();
    }
  }

 public:
  virtual auto attach(std::shared_ptr<BaseImageBufferHeap> pHeap)
      -> std::shared_ptr<IRegistrationCookie> {
    if (pHeap == nullptr) {
      CAM_ULOGME("%s: bad heap: null", __FUNCTION__);
      return nullptr;
    }

    auto cookie = std::make_shared<Cookie>();
    if (cookie == nullptr) {
      CAM_ULOGME("%s: fail to make a new cookie", __FUNCTION__);
      return nullptr;
    }

    android::Mutex::Autolock _l(mLock);
    auto iter = mCookieList.insert(mCookieList.end(), cookie);
    cookie->mIterator = iter;
    cookie->mHeap = pHeap;
    return cookie;
  }
};
    std::string ManagerImpl::mDebuggeeName = "mcam::IImageBufferHeapManager";
};  // namespace

/******************************************************************************
 *
 ******************************************************************************/
auto IManager::get() -> IManager* {
  return ManagerImpl::get();
}

/******************************************************************************
 *
 ******************************************************************************/
BaseImageBufferHeap::~BaseImageBufferHeap() {
  if (mRegCookie) {
    MY_LOGW("[%s] + mRegCookie:%p != nullptr - %s %p %8dx%-4d %s(%#x) %s",
            __FUNCTION__, mRegCookie.get(),
            NSCam::Utils::LogTool::get()
                ->convertToFormattedLogTime(&mCreationTimestamp)
                .c_str(),
            this, mImgSize.w, mImgSize.h,
            NSCam::Utils::Format::queryImageFormatName(mImgFormat).c_str(),
            mImgFormat, mCallerName.c_str());
    dumpCallStack(__FUNCTION__);
    mRegCookie = nullptr;
    MY_LOGW(
        "[%s] - During dtor, virtual functions shouldn't be called in print()",
        __FUNCTION__);
  }
  if (mCreator != NULL) {
    delete mCreator;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
BaseImageBufferHeap::BaseImageBufferHeap(char const* szCallerName,
                                         char const* bufName)
    : mcam::IImageBufferHeap()
      //
      ,
      mBufMap(),
      mInitMtx(),
      mLockMtx(),
      mLockCount(0),
      mLockUsage(0),
      mvBufInfo()
      //
      ,
      mImgSize(),
      mImgFormat(NSCam::eImgFmt_BLOB),
      mPlaneCount(0),
      mBitstreamSize(0),
      mSecType(NSCam::SecType::mem_normal),
      mColorArrangement(-1),
      mColorSpace(eImgColorSpace_BT601_FULL)
      //
      ,
      mEnableLog(MTRUE),
      mCallerName(szCallerName),
      mBufName(bufName),
      mCreator(NULL) {
  NSCam::Utils::LogTool::get()->getCurrentLogTime(&mCreationTimestamp);
  mCreator = new mcam::ImgBufCreator();
  mDebug = property_get_int32("vendor.debug.camera.imgbuf.log", 0);
}

/******************************************************************************
 *
 ******************************************************************************/
void BaseImageBufferHeap::_onFirstRef() {
// TODO(MTK): Remove this workaround which is for FPGA early porting
#if 0
  mRegCookie = ManagerImpl::get()->attach(shared_from_this());
  if (mRegCookie == nullptr) {
    MY_LOGE("fail to register itself");
  }
#else
  mRegCookie = nullptr;
#endif
}

/******************************************************************************
 *
 ******************************************************************************/
void BaseImageBufferHeap::_onLastStrongRef() {
  MY_LOGD_IF(getLogCond(), "%s %p %dx%d %s(%#x) %s",
             NSCam::Utils::LogTool::get()
                 ->convertToFormattedLogTime(&mCreationTimestamp)
                 .c_str(),
             this, mImgSize.w, mImgSize.h,
             NSCam::Utils::Format::queryImageFormatName(mImgFormat).c_str(),
             mImgFormat, mCallerName.c_str());
  //
  mRegCookie = nullptr;
  //
  Mutex::Autolock _l(mInitMtx);
  uninitLocked();

  if (0 != mLockCount) {
    MY_LOGE("Not unlock before release heap - LockCount:%d", mLockCount);
    dumpInfoLocked();
    dumpCallStack(__FUNCTION__);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::onCreate(OnCreate const& arg) {
  nsecs_t const startTime = ::systemTime();
#if 0
    if (CC_UNLIKELY(NSCam::eImgFmt_JPEG == arg.imgFormat )) {
        MY_LOGE("Cannnot create JPEG format heap");
        dumpCallStack(__FUNCTION__);
        return MFALSE;
    }
#endif
  if (CC_UNLIKELY(!checkValidFormat(arg.imgFormat))) {
    MY_LOGE("Unsupported Image Format!!");
    dumpCallStack(__FUNCTION__);
    return MFALSE;
  }
  if (CC_UNLIKELY(!arg.imgSize)) {
    MY_LOGE("Unvalid Image Size(%dx%d)", arg.imgSize.w, arg.imgSize.h);
    dumpCallStack(__FUNCTION__);
    return MFALSE;
  }
  //
  Mutex::Autolock _l(mInitMtx);
  //
  mImgSize = arg.imgSize;
  mImgFormat = arg.imgFormat;
  mBitstreamSize = arg.bitstreamSize;
  mPlaneCount = queryPlaneCount(arg.imgFormat);
  mSecType = arg.secType;
  mEnableLog = arg.enableLog;
  mUseSharedDeviceFd = arg.useSharedDeviceFd;
  mColorSpace = arg.colorspace;
  mhwsync = arg.hwsync;
  //
  MBOOL ret = initLocked(arg.ionDevice);
  mCreationTimeCost = ::systemTime() - startTime;
  //
  MY_LOGD_IF(
      0,
      "[%s] this:%p %dx%d format:%#x secType:%u init:%d cost(ns):%" PRId64 "",
      mCallerName.string(), this, arg.imgSize.w, arg.imgSize.h, arg.imgFormat,
      toLiteral(arg.secType), ret, mCreationTimeCost);
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::onCreate(MSize const& imgSize,
                              MINT32 const colorspace,
                              MINT const imgFormat,
                              size_t const bitstreamSize,
                              NSCam::SecType const secType,
                              MBOOL const enableLog,
                              MBOOL const hwsync) {
  return onCreate(OnCreate{
      .imgSize = imgSize,
      .colorspace = colorspace,
      .imgFormat = imgFormat,
      .bitstreamSize = bitstreamSize,
      .secType = secType,
      .enableLog = enableLog,
      .hwsync = hwsync,
  });
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBufferHeap::getPlaneBitsPerPixel(size_t index) const {
  return queryPlaneBitsPerPixel(getImgFormat(), index);
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBufferHeap::getImgBitsPerPixel() const {
  return queryImageBitsPerPixel(getImgFormat());
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::setBitstreamSize(size_t const bitstreamsize) {
  switch (getImgFormat()) {
    case NSCam::eImgFmt_JPEG:
    case NSCam::eImgFmt_BLOB:
    case NSCam::eImgFmt_JPEG_APP_SEGMENT:
      break;
    default:
      MY_LOGE("%s@ bad format:%#x", getMagicName(), getImgFormat());
      return MFALSE;
      break;
  }
  //
  if (bitstreamsize > getBufSizeInBytes(0)) {
    MY_LOGE("%s@ bitstreamSize:%zu > heap buffer size:%zu", getMagicName(),
            bitstreamsize, getBufSizeInBytes(0));
    return MFALSE;
  }
  //
  Mutex::Autolock _l(mLockMtx);
  mBitstreamSize = bitstreamsize;
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
void BaseImageBufferHeap::setColorArrangement(MINT32 const colorArrangement) {
  Mutex::Autolock _l(mLockMtx);
  mColorArrangement = colorArrangement;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::setImgDesc(NSCam::ImageDescId id,
                                MINT64 value,
                                MBOOL overwrite) {
  static_assert(static_cast<size_t>(NSCam::eIMAGE_DESC_ID_MAX) < 20U,
                "Too many IDs, we had better review or use a more economical "
                "data structure");

  if (static_cast<size_t>(id) >=
      static_cast<size_t>(NSCam::eIMAGE_DESC_ID_MAX)) {
    MY_LOGE("Invalid ImageDescId: %d", id);
    return MFALSE;
  }

  Mutex::Autolock _l(mLockMtx);

  ImageDescItem* item = &mImageDesc[static_cast<size_t>(id)];
  if (item->isValid && !overwrite) {
    return MFALSE;
  }

  item->isValid = MTRUE;
  item->value = value;

  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::getImgDesc(NSCam::ImageDescId id, MINT64& value) const {
  if (static_cast<size_t>(id) >=
      static_cast<size_t>(NSCam::eIMAGE_DESC_ID_MAX)) {
    MY_LOGE("Invalid ImageDescId: %d", id);
    return MFALSE;
  }

  Mutex::Autolock _l(mLockMtx);

  const ImageDescItem* item = &mImageDesc[static_cast<size_t>(id)];
  if (!item->isValid) {
    return MFALSE;
  }

  value = item->value;
  return MTRUE;
}

/******************************************************************************
 * Heap ID could be ION fd, PMEM fd, and so on.
 * Legal only after lock().
 ******************************************************************************/
MINT32
BaseImageBufferHeap::getHeapID(size_t index) const {
  Mutex::Autolock _l(mLockMtx);
  //
  if (0 >= mLockCount) {
    MY_LOGE("This call is legal only after lock()");
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  HeapInfoVect_t const& rvHeapInfo = impGetHeapInfo();
  if (index >= rvHeapInfo.size()) {
    MY_LOGE("this:%p Invalid index:%zu >= %zu", this, index, rvHeapInfo.size());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  return rvHeapInfo[index]->heapID;
}

/******************************************************************************
 * 0 <= Heap ID count <= plane count.
 * Legal only after lock().
 ******************************************************************************/
size_t BaseImageBufferHeap::getHeapIDCount() const {
  Mutex::Autolock _l(mLockMtx);
  //
  if (0 >= mLockCount) {
    MY_LOGE("This call is legal only after lock()");
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  return impGetHeapInfo().size();
}

/******************************************************************************
 * Buffer physical address; legal only after lock() with HW usage.
 ******************************************************************************/
// MINTPTR
// getBufPA(size_t index) {
//   if (index >= getPlaneCount()) {
//     MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
//     dumpCallStack(__FUNCTION__);
//     return 0;
//   }
//   //
//   //
//   Mutex::Autolock _l(mLockMtx);
//   //
//   if (0 == mLockCount || 0 == (mLockUsage & mcam::eBUFFER_USAGE_HW_MASK)) {
//     MY_LOGE(
//         "This call is legal only after lockBuf() with HW usage -"
//         "LockCount:%d Usage:%#x",
//         mLockCount, mLockUsage);
//     dumpCallStack(__FUNCTION__);
//     return 0;
//   }
//   //
//   return mvBufInfo[index]->pa;
// }

/******************************************************************************
 * Buffer virtual address; legal only after lock() with SW usage.
 ******************************************************************************/
MINTPTR
BaseImageBufferHeap::getBufVA(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  if (0 == mLockCount || 0 == (mLockUsage & mcam::eBUFFER_USAGE_SW_MASK)) {
    MY_LOGE(
        "This call is legal only after lockBuf() with SW usage - LockCount:%d "
        "Usage:%#x",
        mLockCount, mLockUsage);
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  return mvBufInfo[index]->va;
}

/******************************************************************************
 * Buffer size in bytes; always legal.
 ******************************************************************************/
size_t BaseImageBufferHeap::getBufSizeInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  return mvBufInfo[index]->sizeInBytes;
}

/******************************************************************************
 * Buffer Strides in bytes; always legal.
 ******************************************************************************/
size_t BaseImageBufferHeap::getBufStridesInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  return mvBufInfo[index]->stridesInBytes;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::lockBuf(char const* szCallerName, MINT usage) {
  Mutex::Autolock _l(mLockMtx);
  return lockBufLocked(szCallerName, usage);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::unlockBuf(char const* szCallerName) {
  Mutex::Autolock _l(mLockMtx);
  return unlockBufLocked(szCallerName);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::lockBufLocked(char const* szCallerName, MINT usage) {
  auto add_lock_info = [this, szCallerName]() {
    auto iter = mLockInfoList.emplace(mLockInfoList.begin());
    iter->mUser = szCallerName;
    iter->mTid = ::gettid();
    NSCam::Utils::LogTool::get()->getCurrentLogTime(&iter->mTimestamp);
  };

  if (0 < mLockCount) {
    MINT const readUsage = mcam::eBUFFER_USAGE_SW_READ_MASK |
                           mcam::eBUFFER_USAGE_HW_CAMERA_READ |
                           mcam::eBUFFER_USAGE_HW_TEXTURE;
    if ((!(usage & ~readUsage) && mLockUsage == usage) ||
        (getImgFormat() == NSCam::eImgFmt_BLOB) ||
         // !!NOTES: Multi-ImageBuffers belonging of
         // Blob-format heap might call lockBuf multiple
         // times
         (getHwSync() == MTRUE)
    ) {
      mLockCount++;
      add_lock_info();
      return MTRUE;
    } else {
      MY_LOGE("%s@ count:%d, usage:%#x, can't lock with usage:%#x",
              szCallerName, mLockCount, mLockUsage, usage);
      dumpInfoLocked();
      return MFALSE;
    }
  }
  //
  if (!impLockBuf(szCallerName, usage, mvBufInfo)) {
    MY_LOGE("%s@ impLockBuf() usage:%#x", szCallerName, usage);
    dumpInfoLocked();
    return MFALSE;
  }
  //
  //  Check Buffer Info.
  if (getPlaneCount() != mvBufInfo.size()) {
    MY_LOGE("%s@ BufInfo.size(%zu) != PlaneCount(%zu)", szCallerName,
            mvBufInfo.size(), getPlaneCount());
    dumpInfoLocked();
    return MFALSE;
  }
  //
  for (size_t i = 0; i < mvBufInfo.size(); i++) {
    if (0 != (usage & mcam::eBUFFER_USAGE_SW_MASK) && 0 == mvBufInfo[i]->va) {
      MY_LOGE("%s@ Bad result at %zu-th plane: va=0 with SW usage:%#x",
              szCallerName, i, usage);
      dumpInfoLocked();
      return MFALSE;
    }

    // if (IIonDeviceProvider::get()->queryLegacyIon() &&
    //     0 != (usage & mcam::eBUFFER_USAGE_HW_MASK) &&
    //           0 == mvBufInfo[i]->pa) {
    //   MY_LOGE("%s@ Bad result at %zu-th plane: pa=0 with HW usage:%#x",
    //           szCallerName, i, usage);
    //   dumpInfoLocked();
    //   return MFALSE;
    // }
  }
  //
  mLockUsage = usage;
  mLockCount++;
  add_lock_info();
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::unlockBufLocked(char const* szCallerName) {
  auto del_lock_info = [this, szCallerName]() {
    // in stack order
    auto itToDelete = mLockInfoList.begin();
    for (; itToDelete != mLockInfoList.end(); itToDelete++) {
      if (itToDelete->mUser == szCallerName) {
        // delete the 1st one whose user name & tid match.
        if (itToDelete->mTid == ::gettid()) {
          break;
        }

        auto it = itToDelete;
        it++;
        for (; it != mLockInfoList.end(); it++) {
          if (it->mUser == szCallerName && it->mTid == ::gettid()) {
            itToDelete = it;
            break;
          }
        }

        // no tid matches...
        // delete the 1st one whose user name matches.
        break;
      }
    }
    if (itToDelete != mLockInfoList.end()) {
      mLockInfoList.erase(itToDelete);
    }
  };

  if (1 < mLockCount) {
    mLockCount--;
    MY_LOGD_IF(mDebug, "%s@ still locked (%d)", szCallerName, mLockCount);
    del_lock_info();
    return MTRUE;
  }
  //
  if (0 == mLockCount) {
    MY_LOGW("%s@ Never lock", szCallerName);
    dumpInfoLocked();
    return MFALSE;
  }
  //
  if (!impUnlockBuf(szCallerName, mLockUsage, mvBufInfo)) {
    MY_LOGE("%s@ impUnlockBuf() usage:%#x", szCallerName, mLockUsage);
    dumpInfoLocked();
    return MFALSE;
  }
  //
  mLockUsage = 0;
  mLockCount--;
  del_lock_info();
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::initLocked(std::shared_ptr<IIonDevice> const& pDevice) {
  MBOOL ret = MFALSE;
  //
  mIonDevice = (pDevice != nullptr)
                   ? pDevice
                   : IIonDeviceProvider::get()->makeIonDevice(
                         mCallerName.c_str(), mUseSharedDeviceFd);
  if (mIonDevice == nullptr) {
    MY_LOGE("fail to makeIonDevice");
    goto lbExit;
  }
  //
  mBufMap.clear();
  //
  mvBufInfo.clear();
  mvBufInfo.setCapacity(getPlaneCount());
  for (size_t i = 0; i < getPlaneCount(); i++) {
    mvBufInfo.push_back(new BufInfo);
  }
  //
  if (!impInit(mvBufInfo)) {
    MY_LOGE("%s@ impInit()", getMagicName());
    goto lbExit;
  }
  //
  for (size_t i = 0; i < mvBufInfo.size(); i++) {
    if (mvBufInfo[i]->stridesInBytes <= 0) {
      MY_LOGE("%s@ Bad result at %zu-th plane: strides:%zu", getMagicName(), i,
              mvBufInfo[i]->stridesInBytes);
      goto lbExit;
    }
  }
  //
  ret = MTRUE;
lbExit:
  if (!ret) {
    uninitLocked();
  }
  //
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::uninitLocked() {
  if (!impUninit(mvBufInfo)) {
    MY_LOGE("%s@ impUninit()", getMagicName());
  }
  mvBufInfo.clear();
  mRegCookie = nullptr;
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
BaseImageBufferHeap::dumpInfoLocked() {
//  ULogPrinter printer(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_INFO);
  android::LogPrinter printer(LOG_TAG);
  printLocked(printer, 2);
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
BaseImageBufferHeap::printLocked(android::Printer& printer, uint32_t indent) {
  //  Legal only after lockBuf().
  String8 s8HeapID("heap-id:[");
  HeapInfoVect_t const& rvHeapInfo = impGetHeapInfo();
  for (size_t i = 0; i < rvHeapInfo.size(); i++) {
    s8HeapID += String8::format(" %d", rvHeapInfo[i]->heapID);
  }
  s8HeapID += " ]";

  String8 s8BufInfo("buf{stride/size/pa/va}:[");
  for (size_t i = 0; i < mvBufInfo.size(); i++) {
    auto const& v = *mvBufInfo[i];
    s8BufInfo += String8::format(" (%zu %zu %08" PRIxPTR " %08" PRIxPTR ")",
                                 v.stridesInBytes, v.sizeInBytes, v.pa, v.va);
  }
  s8BufInfo += " ]";

  printer.printFormatLine(
      "%-*c%s cost(us):%5" PRId64
      " #strong:%d-1 %8dx%-4d %s(%#x) %s %s %s %s %s",
      indent, ' ',
      NSCam::Utils::LogTool::get()
          ->convertToFormattedLogTime(&mCreationTimestamp)
          .c_str(),
      mCreationTimeCost / 1000, getStrongCount(), mImgSize.w, mImgSize.h,
      NSCam::Utils::Format::queryImageFormatName(mImgFormat).c_str(),
      mImgFormat, getMagicName(), mCallerName.c_str(), s8HeapID.c_str(),
      s8BufInfo.c_str(), impPrintLocked().c_str());

  if (mLockCount != 0 || mLockUsage != 0) {
    printer.printFormatLine("%-*c    #lock:%d usage:%#x", indent, ' ',
                            mLockCount, mLockUsage);
  }

  for (auto const& v : mLockInfoList) {
    printer.printFormatLine("%-*c    %s %-5d %s", indent, ' ',
                            NSCam::Utils::LogTool::get()
                                ->convertToFormattedLogTime(&v.mTimestamp)
                                .c_str(),
                            v.mTid, v.mUser.c_str());
  }
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
BaseImageBufferHeap::print(android::Printer& printer, uint32_t indent) {
  Mutex::Autolock _l(mLockMtx);
  printLocked(printer, indent);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::syncCache(mcam::CacheCtrl const ctrl) {
  //
  HeapInfoVect_t const& rvHeapInfo = impGetHeapInfo();
  size_t const num = getPlaneCount();
  Vector<HelperParamFlushCache> vInfo;
  vInfo.insertAt(0, num);
  HelperParamFlushCache* const aInfo = vInfo.editArray();
  for (size_t i = 0; i < num; i++) {
    aInfo[i].phyAddr = mvBufInfo[i]->pa;
    aInfo[i].virAddr = mvBufInfo[i]->va;
    aInfo[i].ionFd =
        (rvHeapInfo.size() > 1) ? rvHeapInfo[i]->heapID : rvHeapInfo[0]->heapID;
    aInfo[i].size = mvBufInfo[i]->sizeInBytes;
  }
  //
  if (!helpFlushCache(ctrl, aInfo, num)) {
    MY_LOGE("helpFlushCache");
    return MFALSE;
  }
  //
  return MTRUE;
}

MINT mcam::ImgBufCreator::generateFormat(mcam::IImageBufferHeap* heap) {
  if (mCreatorFormat != NSCam::eImgFmt_UNKNOWN) {
    return mCreatorFormat;
  }
  if (heap == NULL)
    return mCreatorFormat;
  switch ((NSCam::EImageFormat)heap->getImgFormat()) {
    case NSCam::eImgFmt_UFO_BAYER8:
    case NSCam::eImgFmt_UFO_BAYER10:
    case NSCam::eImgFmt_UFO_BAYER12:
    case NSCam::eImgFmt_UFO_BAYER14:
    case NSCam::eImgFmt_UFO_FG_BAYER8:
    case NSCam::eImgFmt_UFO_FG_BAYER10:
    case NSCam::eImgFmt_UFO_FG_BAYER12:
    case NSCam::eImgFmt_UFO_FG_BAYER14: {
      heap->lockBuf("mcam::ImgBufCreator", GRALLOC_USAGE_SW_READ_OFTEN);
      MINT format = *(reinterpret_cast<MUINT32*>(heap->getBufVA(2)));
      heap->unlockBuf("mcam::ImgBufCreator");
      return format;
    }
    default:
      break;
  }
  return heap->getImgFormat();
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<mcam::IImageBuffer> BaseImageBufferHeap::createImageBuffer(
    mcam::ImgBufCreator* pCreator) {
  size_t bufStridesInBytes[3] = {0, 0, 0};
  MINT format = NSCam::eImgFmt_UNKNOWN;

  for (size_t i = 0; i < getPlaneCount(); ++i) {
    bufStridesInBytes[i] = getBufStridesInBytes(i);
  }
  shared_ptr<BaseImageBuffer> pImgBuffer = NULL;

  if (pCreator != NULL) {
    format = pCreator->generateFormat(this);
  } else {
    format =
        ((mCreator != NULL) ? mCreator->generateFormat(this) : getImgFormat());
  }

  pImgBuffer = std::make_shared<BaseImageBuffer>(
      shared_from_this(), getImgSize(), format, getBitstreamSize(),
      bufStridesInBytes, 0, getSecType());

  if (!pImgBuffer) {
    MY_LOGE("Fail to new");
    return NULL;
  }
  //
  if (!pImgBuffer->onCreate()) {
    MY_LOGE("onCreate");
    pImgBuffer = 0;
    return NULL;
  }
  //
  return pImgBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<mcam::IImageBuffer>
BaseImageBufferHeap::createImageBuffer_FromBlobHeap(size_t offsetInBytes,
                                                    size_t sizeInBytes) {
  if (getImgFormat() != NSCam::eImgFmt_BLOB &&
      getImgFormat() != NSCam::eImgFmt_CAMERA_OPAQUE &&
      getImgFormat() != NSCam::eImgFmt_JPEG &&
      getImgFormat() != NSCam::eImgFmt_JPEG_APP_SEGMENT) {
    MY_LOGE("Heap format(0x%x) is illegal.", getImgFormat());
    return NULL;
  }
  //
  MSize imgSize(sizeInBytes, getImgSize().h);
  size_t bufStridesInBytes[] = {sizeInBytes, 0, 0};
  shared_ptr<BaseImageBuffer> pImgBuffer = NULL;
  pImgBuffer = std::make_shared<BaseImageBuffer>(
      shared_from_this(), imgSize, getImgFormat(), getBitstreamSize(),
      bufStridesInBytes, offsetInBytes, getSecType());
  //
  if (!pImgBuffer) {
    MY_LOGE("Fail to new");
    return NULL;
  }
  //
  if (!pImgBuffer->onCreate()) {
    MY_LOGE("onCreate");
    pImgBuffer = 0;
    return NULL;
  }
  //
  return pImgBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<mcam::IImageBuffer>
BaseImageBufferHeap::createImageBuffer_FromBlobHeap(
    size_t offsetInBytes,
    MINT32 imgFormat,
    MSize const& imgSize,
    size_t const bufStridesInBytes[3]) {
  if (getImgFormat() != NSCam::eImgFmt_BLOB &&
      getImgFormat() != NSCam::eImgFmt_CAMERA_OPAQUE &&
      getImgFormat() != NSCam::eImgFmt_JPEG &&
      getImgFormat() != NSCam::eImgFmt_JPEG_APP_SEGMENT) {
    MY_LOGE("Heap format(0x%x) is illegal.", getImgFormat());
    return NULL;
  }
  //
  shared_ptr<BaseImageBuffer> pImgBuffer = NULL;
  pImgBuffer = std::make_shared<BaseImageBuffer>(
      shared_from_this(), imgSize, imgFormat, getBitstreamSize(),
      bufStridesInBytes, offsetInBytes, getSecType());
  //
  if (!pImgBuffer) {
    MY_LOGE("Fail to new");
    return NULL;
  }
  //
  if (!pImgBuffer->onCreate()) {
    MY_LOGE("onCreate");
    pImgBuffer = 0;
    return NULL;
  }
  //
  return pImgBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
std::vector<std::shared_ptr<mcam::IImageBuffer>>
BaseImageBufferHeap::createImageBuffers_FromBlobHeap(
    const NSCam::ImageBufferInfo& info,
    const char* callerName __unused,
    MBOOL dirty_content) {
  std::vector<shared_ptr<mcam::IImageBuffer>> vpImageBuffer;

  MINT32 bufCount = info.count;
  if (CC_UNLIKELY(getImgFormat() != NSCam::eImgFmt_BLOB)) {
    MY_LOGE("Heap format(0x%x) is illegal.", getImgFormat());
    return vpImageBuffer;
  }

  if (CC_UNLIKELY(bufCount == 0)) {
    MY_LOGE("buffer count is Zero");
    return vpImageBuffer;
  }

  size_t bufStridesInBytes[3] = {0};
  auto& bufPlanes = info.bufPlanes;
  for (size_t i = 0; i < bufPlanes.count; i++) {
    bufStridesInBytes[i] = bufPlanes.planes[i].rowStrideInBytes;
  }

  vpImageBuffer.resize(bufCount);
  for (MINT32 i = 0; i < bufCount; i++) {
    vpImageBuffer[i] = createImageBuffer_FromBlobHeap(
        info.startOffset + (info.bufStep * i), info.imgFormat,
        MSize(info.imgWidth, info.imgHeight), bufStridesInBytes);
    if (CC_UNLIKELY(vpImageBuffer[i] == nullptr)) {
      MY_LOGE("create ImageBuffer fail!!");
      // ensure to free vpImageBuffer since it's raw pointers
      for (size_t j = 0; j < vpImageBuffer.size(); j++) {
        vpImageBuffer[j] = 0;
      }
      vpImageBuffer.clear();
      return vpImageBuffer;
    }
    if (!dirty_content && info.imgFormat >= NSCam::eImgFmt_UFO_START &&
        info.imgFormat <= NSCam::eImgFmt_UFO_END) {
      if (!vpImageBuffer[i]->lockBuf("mcam::ImgBufCreator",
                                     GRALLOC_USAGE_SW_READ_OFTEN)) {
        MY_LOGE("lockBuf fail!");
        for (size_t j = 0; j < vpImageBuffer.size(); j++) {
          vpImageBuffer[j] = 0;
        }
        vpImageBuffer.clear();
        return vpImageBuffer;
      } else {
        MINT format = *(reinterpret_cast<MUINT32*>(
                        vpImageBuffer[i]->getBufVA(2)));
        vpImageBuffer[i]->unlockBuf("mcam::ImgBufCreator");

        if (format != 0 && (MINT)format != (MINT)info.imgFormat) {
          vpImageBuffer[i] = 0;
          vpImageBuffer[i] = createImageBuffer_FromBlobHeap(
              info.startOffset + (info.bufStep * i), format,
              MSize(info.imgWidth, info.imgHeight), bufStridesInBytes);
        }
      }
    }
  }

  return vpImageBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<mcam::IImageBuffer>
BaseImageBufferHeap::createImageBuffer_SideBySide(MBOOL isRightSide) {
  size_t imgWidth = getImgSize().w >> 1;
  size_t imgHeight = getImgSize().h;
  size_t offset = (isRightSide) ? (imgWidth * getPlaneBitsPerPixel(0)) >> 3 : 0;
  MSize SBSImgSize(imgWidth, imgHeight);
  size_t bufStridesInBytes[3] = {0, 0, 0};
  for (size_t i = 0; i < getPlaneCount(); i++) {
    bufStridesInBytes[i] = (NSCam::eImgFmt_BLOB == getImgFormat())
                               ? getBufStridesInBytes(i) >> 1
                               : getBufStridesInBytes(i);
  }
  //
  shared_ptr<BaseImageBuffer> pImgBuffer = NULL;
  pImgBuffer = std::make_shared<BaseImageBuffer>(
      shared_from_this(), SBSImgSize, getImgFormat(), getBitstreamSize(),
      bufStridesInBytes, offset, getSecType());
  if (!pImgBuffer) {
    MY_LOGE("Fail to new");
    return NULL;
  }
  //
  if (!pImgBuffer->onCreate()) {
    MY_LOGE("onCreate");
    pImgBuffer = 0;
    return NULL;
  }
  //
  return pImgBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::helpMapPhyAddr(char const* szCallerName,
                                    HelperParamMapPA& rParam) {
  MINT32 const memID = rParam.ionFd;
  MBOOL ret_status = MTRUE;
  if (IIonDeviceProvider::get()->queryLegacyIon() == 0) {
    MY_LOGD("AOSP ION do not support mapping PA");
    rParam.phyAddr = 0;
  } else if (0 > mBufMap.indexOfKey(memID)) {
    if (0 > memID) {
      MY_LOGE("error memID(%d)", memID);
    } else {
      ion_user_handle_t pIonHandle;
      struct ion_sys_data sys_data;
      struct ion_mm_data mm_data;
      //
      // a. get handle from IonBufFd and increase handle ref count
      if (ion_import(mIonDevice->getDeviceFd(), memID, &pIonHandle)) {
        MY_LOGE("ion_import fail, memId(0x%x)", memID);
        return MFALSE;
      }
      /*
          ION_MM_GET_IOVA only support normal memory command.
          So it needs to use ION_SYS_GET_PHYS to query secure handle.
          TODO: It should be noted that there are some platforms which are not
         enable IOVA. They still need to use ION_SYS_GET_PHYS to get the mva
         address. Maybe there are other buffer type usecases in the future. It
         needs to refine this.
      */
      if (mSecType == NSCam::SecType::mem_normal) {
        // b c: use get_iova replace config_buffer & get_phys
        memset(reinterpret_cast<void*>(&mm_data), 0, sizeof(mm_data));
        mm_data.mm_cmd = ION_MM_GET_IOVA;
        mm_data.get_phys_param.handle = pIonHandle;
        mm_data.get_phys_param.module_id = M4U_PORT_CAM_MDP;
        mm_data.get_phys_param.security = rParam.security;
        mm_data.get_phys_param.coherent = rParam.coherence;
        if (ion_custom_ioctl(mIonDevice->getDeviceFd(), ION_CMD_MULTIMEDIA,
                             &mm_data)) {
          MY_LOGE("ion_custom_ioctl get_phys_param failed!");
          ret_status = MFALSE;
        } else {
          rParam.phyAddr = (MINTPTR)(mm_data.get_phys_param.phy_addr);
          mBufMap.add(memID, rParam.phyAddr);
        }
      } else {
        // b c: use get_phys to get the secure handle
        memset(reinterpret_cast<void*>(&sys_data), 0, sizeof(sys_data));
        sys_data.sys_cmd = ION_SYS_GET_PHYS;
        sys_data.get_phys_param.handle = pIonHandle;
        sys_data.get_phys_param.phy_addr = 0;
        sys_data.get_phys_param.len = 0;
        if (ion_custom_ioctl(mIonDevice->getDeviceFd(), ION_CMD_SYSTEM,
                             &sys_data)) {
          MY_LOGE("ion_custom_ioctl get_phys_param failed!");
          ret_status = MFALSE;
        } else {
          rParam.phyAddr = (MINTPTR)(sys_data.get_phys_param.phy_addr);
          mBufMap.add(memID, rParam.phyAddr);
        }
      }

      // d. decrease handle ref count
      if (ion_free(mIonDevice->getDeviceFd(), pIonHandle)) {
        MY_LOGE("ion_free fail");
        ret_status = MFALSE;
      }
    }
    // we do not have to map iova in ISP7.0
    MY_LOGD_IF(0,
               "[%s] mID(0x%x),size(%zu),VA(0x%" PRIxPTR "),PA(0x%" PRIxPTR
               "),S/C(%d/%d)",
               szCallerName, rParam.ionFd, rParam.size, rParam.virAddr,
               rParam.phyAddr, rParam.security, rParam.coherence);
  } else {
    rParam.phyAddr = mBufMap.valueFor(memID);
  }
  //
  return ret_status;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::helpUnmapPhyAddr(char const* szCallerName,
                                      HelperParamMapPA const& rParam) {
  if (0 <= mBufMap.indexOfKey(rParam.ionFd)) {
    mBufMap.removeItem(rParam.ionFd);
    MY_LOGD_IF(1,
               "[%s] mID(0x%x),size(%zu),VA(0x%" PRIxPTR "),PA(0x%" PRIxPTR
               "),S/C(%d/%d)",
               szCallerName, rParam.ionFd, rParam.size, rParam.virAddr,
               rParam.phyAddr, rParam.security, rParam.coherence);
  }
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::helpFlushCache(mcam::CacheCtrl const ctrl,
                                    HelperParamFlushCache const* paParam,
                                    size_t const num) {
  if (IIonDeviceProvider::get()->queryLegacyIon()) {
    return helpFlushCacheLegacy(ctrl, paParam, num);
  } else {
    return helpFlushCacheAOSP(ctrl, paParam, num);
  }
}

MBOOL
BaseImageBufferHeap::helpFlushCacheAOSP(mcam::CacheCtrl const ctrl,
                                        HelperParamFlushCache const* paParam,
                                        size_t const num) {
  MY_LOGA_IF(NULL == paParam || 0 == num, "Bad arguments: %p %zu", paParam,
             num);

  struct dma_buf_sync sync;

  for (size_t i = 0; i < num; i++) {
    MY_LOGD("ctrl(%d), num[%zu]: mID(0x%x),VA(0x%" PRIxPTR ")", ctrl, i,
            paParam[i].ionFd, paParam[i].virAddr);
    MINT32 const memID = paParam[i].ionFd;
    if (0 > memID) {
      MY_LOGE("error memID(%d)", memID);
      return MFALSE;
    } else {
      if (ctrl == mcam::eCACHECTRL_FLUSH) {
        sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
        if (ioctl(memID, DMA_BUF_IOCTL_SYNC, &sync)) {
          MY_LOGE("flush start fail...");
          return MFALSE;
        }

        sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
        if (ioctl(memID, DMA_BUF_IOCTL_SYNC, &sync)) {
          MY_LOGE("flush end fail...");
          return MFALSE;
        }
      } else if (ctrl == mcam::eCACHECTRL_INVALID) {
        sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
        if (ioctl(memID, DMA_BUF_IOCTL_SYNC, &sync)) {
          MY_LOGE("invalidate start fail...");
          return MFALSE;
        }
        sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
        if (ioctl(memID, DMA_BUF_IOCTL_SYNC, &sync)) {
          MY_LOGE("invalidate end fail...");
          return MFALSE;
        }
      } else {
        MY_LOGE("invalid flush type");
        return MFALSE;
      }
    }
  }
  return MTRUE;
}

MBOOL
BaseImageBufferHeap::helpFlushCacheLegacy(mcam::CacheCtrl const ctrl,
                                          HelperParamFlushCache const* paParam,
                                          size_t const num) {
  MY_LOGA_IF(NULL == paParam || 0 == num, "Bad arguments: %p %zu", paParam,
             num);
  //
  ION_CACHE_SYNC_TYPE CACHECTRL = ION_CACHE_FLUSH_BY_RANGE;
  switch (ctrl) {
    case mcam::eCACHECTRL_FLUSH:
      CACHECTRL = ION_CACHE_FLUSH_BY_RANGE;
      break;
    case mcam::eCACHECTRL_INVALID:
      CACHECTRL = ION_CACHE_INVALID_BY_RANGE;
      break;
    default:
      MY_LOGE("ERR cmd(%d)", ctrl);
      break;
  }
  //
  for (size_t i = 0; i < num; i++) {
    MY_LOGD_IF(getLogCond(),
               "ctrl(%d), num[%zu]: mID(0x%x),size(%zu),VA(0x%" PRIxPTR
               "),PA(0x%" PRIxPTR ")",
               ctrl, i, paParam[i].ionFd, paParam[i].size, paParam[i].virAddr,
               paParam[i].phyAddr);
    // MINTPTR const pa    = paParam[i].phyAddr;
    // MINTPTR const va    = paParam[i].virAddr;
    MINT32 const memID = paParam[i].ionFd;
    // size_t const size   = paParam[i].size;
    //
    if (0 > memID) {
      MY_LOGE("error memID(%d)", memID);
    } else {
      // a. get handle of ION_IOC_SHARE from IonBufFd and increase handle ref
      // count
      ion_user_handle_t pIonHandle;
      if (ion_import(mIonDevice->getDeviceFd(), memID, &pIonHandle)) {
        MY_LOGE("ion_import fail, memId(0x%x)", memID);
      }
      // b. cache sync by range
      struct ion_sys_data sys_data;
      sys_data.sys_cmd = ION_SYS_CACHE_SYNC;
      sys_data.cache_sync_param.handle = pIonHandle;
      sys_data.cache_sync_param.sync_type = CACHECTRL;
      sys_data.cache_sync_param.va =
                         reinterpret_cast<void*>(paParam[i].virAddr);
      sys_data.cache_sync_param.size = paParam[i].size;
      if (ion_custom_ioctl(mIonDevice->getDeviceFd(), ION_CMD_SYSTEM,
                           &sys_data)) {
        MY_LOGE("ION_SYS_CACHE_SYNC fail, memID(0x%x)", memID);
      }
      // c. decrease handle ref count
      if (ion_free(mIonDevice->getDeviceFd(), pIonHandle)) {
        MY_LOGE("ion_free fail");
      }
    }
  }
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::helpCheckBufStrides(
    size_t const planeIndex,
    size_t const planeBufStridesInBytes) const {
  if (checkValidBufferInfo(getImgFormat())) {
    size_t const planeImgWidthInPixels = queryPlaneWidthInPixels(
        getImgFormat(), planeIndex, getImgSize().w);
    size_t const planeBitsPerPixel = getPlaneBitsPerPixel(planeIndex);
    size_t const roundUpValue =
        ((planeBufStridesInBytes << 3) % planeBitsPerPixel > 0) ? 1 : 0;
    size_t const planeBufStridesInPixels =
        (planeBufStridesInBytes << 3) / planeBitsPerPixel + roundUpValue;
    //
    if (planeBufStridesInPixels < planeImgWidthInPixels) {
      MY_LOGE(
          "[%dx%d image @ %zu-th plane] Bad width stride in pixels: given "
          "buffer stride:%zu < image stride:%zu. stride in bytes(%zu) bpp(%zu)",
          getImgSize().w, getImgSize().h, planeIndex, planeBufStridesInPixels,
          planeImgWidthInPixels, planeBufStridesInBytes, planeBitsPerPixel);
      return MFALSE;
    }
  }
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBufferHeap::helpQueryBufSizeInBytes(
    size_t const planeIndex,
    size_t const planeStridesInBytes) const {
  MY_LOGF_IF(planeIndex >= getPlaneCount(), "Bad index:%zu >= PlaneCount:%zu",
             planeIndex, getPlaneCount());
  //
  size_t const planeImgHeight = queryPlaneHeightInPixels(
      getImgFormat(), planeIndex, getImgSize().h);
  return planeStridesInBytes * planeImgHeight;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBufferHeap::updateImgInfo(MSize const& imgSize,
                                   MINT const imgFormat,
                                   size_t const sizeInBytes[3],
                                   size_t const rowStrideInBytes[3],
                                   size_t const bufPlaneSize) {
  if (NSCam::eImgFmt_JPEG == imgFormat) {
    MY_LOGE("Cannnot create JPEG format heap");
    return MFALSE;
  }
  if (!checkValidFormat(imgFormat)) {
    MY_LOGE("Unsupported Image Format!!");
    return MFALSE;
  }
  if (!imgSize) {
    MY_LOGE("Unvalid Image Size(%dx%d)", imgSize.w, imgSize.h);
    return MFALSE;
  }
  //
  Mutex::Autolock _l(mLockMtx);
  //
  mImgSize = imgSize;
  mImgFormat = imgFormat;
  // setBitstreamSize(stridesInBytes);
  mPlaneCount = queryPlaneCount(imgFormat);
  //
  MY_LOGD("[%s] this:%p %dx%d format:%#x planes:%zu", mCallerName.string(),
          this, imgSize.w, imgSize.h, imgFormat, mPlaneCount);

  mBufMap.clear();
  mvBufInfo.clear();
  mvBufInfo.setCapacity(getPlaneCount());
  for (size_t i = 0; i < getPlaneCount(); i++) {
    if (i >= bufPlaneSize) {
      MY_LOGE("bufInfo[%zu] over the bufPlaneSize:%zu", i, bufPlaneSize);
      break;
    }
    sp<BufInfo> bufInfo = new BufInfo();
    bufInfo->stridesInBytes = rowStrideInBytes[i];
    bufInfo->sizeInBytes = sizeInBytes[i];
    mvBufInfo.push_back(bufInfo);
    MY_LOGD("stride:%zu, sizeInBytes:%zu", bufInfo->stridesInBytes,
            bufInfo->sizeInBytes);
  }
  //
  if (!impReconfig(mvBufInfo)) {
    MY_LOGE("%s@ impReconfig()", getMagicName());
  }
  //
  for (size_t i = 0; i < mvBufInfo.size(); i++) {
    if (mvBufInfo[i]->stridesInBytes <= 0) {
      MY_LOGE("%s@ Bad result at %zu-th plane: strides:%zu", getMagicName(), i,
              mvBufInfo[i]->stridesInBytes);
    }
  }
  return MTRUE;
}

size_t BaseImageBufferHeap::getOffsetInBytes() const {
  return 0;
}
