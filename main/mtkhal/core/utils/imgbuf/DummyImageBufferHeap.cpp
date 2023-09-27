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
#define LOG_TAG "MtkCam/DummyHeap"
//
#include <mtkcam/utils/std/ULog.h>

#include <memory>

#include "include/BaseImageBufferHeap.h"
#include "include/MyUtils.h"
#include "include/IDummyImageBufferHeap.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);
//
// using namespace android;
// using namespace NSCam;
// using namespace NSCam::Utils;
//

using NSCam::IDummyImageBufferHeap;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;
using mcam::IImageBufferAllocator;
using NSCam::PortBufInfo_dummy;
using NSCam::DummyHeapType::eDummyFromFD;
using NSCam::DummyHeapType::eDummyFromHeap;
using NSCam::Utils::dumpCallStack;
using android::sp;
using android::Vector;

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
 *  Image Buffer Heap (Dummy).
 ******************************************************************************/
namespace {
class DummyImageBufferHeap : public IDummyImageBufferHeap,
                             public BaseImageBufferHeap {
  friend class IDummyImageBufferHeap;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  BaseImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  virtual char const* impGetMagicName() const { return magicName(); }

  virtual void* impGetMagicInstance() const { return const_cast<void*>
                                    (reinterpret_cast<const void *>(this)); }

  virtual HeapInfoVect_t const& impGetHeapInfo() const { return mvHeapInfo; }

  virtual MBOOL impInit(BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impUninit(BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impReconfig(BufInfoVect_t const& rvBufInfo) { return MFALSE; }

 public:  ////
  virtual MBOOL impLockBuf(char const* szCallerName,
                           MINT usage,
                           BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impUnlockBuf(char const* szCallerName,
                             MINT usage,
                             BufInfoVect_t const& rvBufInfo);
  virtual size_t getStartOffsetInBytes(size_t index) const;

  static std::shared_ptr<IDummyImageBufferHeap> create(
      char const* szCallerName,
      char const* bufName,
      std::shared_ptr<mcam::IImageBufferHeap> blobHeap,
      size_t offsetInByte,
      ImgParam_t const& legacyParam,
      mcam::IImageBufferAllocator::ImgParam const& imgParam,
      MBOOL const enableLog = MTRUE);

  static std::shared_ptr<IDummyImageBufferHeap> create(
      char const* szCallerName,
      char const* bufName,
      int bufFd,
      size_t offsetInByte,
      ImgParam_t const& legacyParam,
      mcam::IImageBufferAllocator::ImgParam const& imgParam,
      MBOOL const enableLog = MTRUE);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Buffer Info.
  struct MyBufInfo : public BufInfo {
    size_t u4Offset;
    size_t iBoundaryInBytesToAlloc;
    //
    MyBufInfo() : BufInfo(), u4Offset(0), iBoundaryInBytesToAlloc(0) {}
  };
  typedef Vector<sp<MyBufInfo> > MyBufInfoVect_t;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Instantiation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Destructor/Constructors.
  /**
   * Disallowed to directly delete a raw pointer.
   */
  virtual ~DummyImageBufferHeap() {}
  DummyImageBufferHeap(char const* szCallerName,
                       char const* bufName,
                       ImgParam_t const& rImgParam,
                       PortBufInfo_dummy const& rPortBufInfo);

  size_t getBufCustomSizeInBytes(size_t index) const override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Info to Allocate.
  size_t mBufStridesInBytesToAlloc[3];     // buffer strides in bytes.
  size_t mBufCustomSizeInBytesToAlloc[3];  // buffer strides in bytes.

 protected:  ////                    Info of Allocated Result.
  PortBufInfo_dummy mPortBufInfo;  //
  HeapInfoVect_t mvHeapInfo;       //
  BufInfoVect_t mvBufInfo;         //
  MUINT32 totalSizeInBytes;
};
};  // namespace

/******************************************************************************
 *
 ******************************************************************************/
#define GET_BUF_VA(plane, va, index) (plane >= (index + 1)) ? va : 0
#define GET_BUF_ID(plane, memID, index) (plane >= (index + 1)) ? memID : 0

/******************************************************************************
 *
 ******************************************************************************/

std::shared_ptr<IDummyImageBufferHeap> DummyImageBufferHeap::create(
    char const* szCallerName,
    char const* bufName,
    std::shared_ptr<mcam::IImageBufferHeap> blobHeap,
    size_t offsetInByte,
    ImgParam_t const& legacyParam,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    MBOOL const enableLog) {
  // 1. create PortBufInfo_dummy
  PortBufInfo_dummy rPortBufInfo(offsetInByte, blobHeap->getHeapID(),
                                 blobHeap->getBufVA(0), 0,
                                 eDummyFromHeap, imgParam.nocache,
                                 blobHeap->getSecType(), 0, 0);

  // 2. create image buffer
  DummyImageBufferHeap* pHeap = NULL;
  pHeap = new DummyImageBufferHeap(szCallerName, bufName, legacyParam,
                                   rPortBufInfo);
  if (!pHeap) {
    CAM_ULOGME("Fail to new");
    return NULL;
  }
  auto ret = pHeap->onCreate(DummyImageBufferHeap::OnCreate{
      .imgSize = legacyParam.imgSize,
      .colorspace = imgParam.colorspace,
      .imgFormat = legacyParam.imgFormat,
      .bitstreamSize = 0,
      .secType = rPortBufInfo.secType,
      .enableLog = enableLog,
      .useSharedDeviceFd = 1,
      .hwsync = imgParam.hwsync,
  });
  if (!ret) {
    CAM_ULOGME("onCreate");
    delete pHeap;
    return NULL;
  }

  // 3. create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<DummyImageBufferHeap> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

std::shared_ptr<IDummyImageBufferHeap> DummyImageBufferHeap::create(
    char const* szCallerName,
    char const* bufName,
    int bufFd,
    size_t offsetInByte,
    ImgParam_t const& legacyParam,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    MBOOL const enableLog) {
  // 0. dup FD
  int dupFd = -1;

  if (bufFd <= 0) {
    CAM_ULOGME("invalid buffer fd: %d", bufFd);
    return nullptr;
  }

  dupFd = ::dup(bufFd);

  if (dupFd <= 0) {
    CAM_ULOGME("dup fd fail: %d", dupFd);
    return nullptr;
  }

  // 1. create PortBufInfo_dummy
  PortBufInfo_dummy rPortBufInfo(offsetInByte, dupFd, 0, 0, eDummyFromFD,
                                 imgParam.nocache, imgParam.secType, 0, 0);

  // 2. create image buffer
  DummyImageBufferHeap* pHeap = NULL;
  pHeap = new DummyImageBufferHeap(szCallerName, bufName, legacyParam,
                                   rPortBufInfo);
  if (!pHeap) {
    CAM_ULOGME("Fail to new");
    return NULL;
  }
  auto ret = pHeap->onCreate(DummyImageBufferHeap::OnCreate{
      .imgSize = legacyParam.imgSize,
      .imgFormat = legacyParam.imgFormat,
      .bitstreamSize = 0,
      .secType = rPortBufInfo.secType,
      .enableLog = enableLog,
      .useSharedDeviceFd = 1,
      .hwsync = imgParam.hwsync,
  });
  if (!ret) {
    CAM_ULOGME("onCreate");
    delete pHeap;
    return NULL;
  }

  // 3. create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<DummyImageBufferHeap> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

std::shared_ptr<IDummyImageBufferHeap> IDummyImageBufferHeap::createPtr(
    char const* szCallerName,
    char const* bufName,
    std::shared_ptr<mcam::IImageBufferHeap> blobHeap,
    size_t offsetInByte,
    ImgParam_t const& legacyParam,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    MBOOL const enableLog) {
  return DummyImageBufferHeap::create(szCallerName, bufName, blobHeap,
                                      offsetInByte, legacyParam, imgParam,
                                      enableLog);
}

std::shared_ptr<IDummyImageBufferHeap> IDummyImageBufferHeap::createPtr(
    char const* szCallerName,
    char const* bufName,
    int bufFd,
    size_t offsetInByte,
    ImgParam_t const& legacyParam,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    MBOOL const enableLog) {
  return DummyImageBufferHeap::create(szCallerName, bufName, bufFd,
                                      offsetInByte, legacyParam, imgParam,
                                      enableLog);
}

/******************************************************************************
 *
 ******************************************************************************/
DummyImageBufferHeap::DummyImageBufferHeap(
    char const* szCallerName,
    char const* bufName,
    ImgParam_t const& rImgParam,
    PortBufInfo_dummy const& rPortBufInfo)
    : BaseImageBufferHeap(szCallerName, bufName),
      mPortBufInfo(rPortBufInfo),
      mvHeapInfo(),
      mvBufInfo(),
      totalSizeInBytes(0) {
  ::memcpy(mBufStridesInBytesToAlloc, rImgParam.bufStridesInBytes,
           sizeof(mBufStridesInBytesToAlloc));
  ::memcpy(mBufCustomSizeInBytesToAlloc, rImgParam.bufCustomSizeInBytes,
           sizeof(mBufCustomSizeInBytesToAlloc));
}

size_t DummyImageBufferHeap::getBufCustomSizeInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    // MY_LOGD("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    // dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  return mBufCustomSizeInBytesToAlloc[index];
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
DummyImageBufferHeap::impInit(BufInfoVect_t const& rvBufInfo) {
  MBOOL ret = MFALSE;
  MUINT32 planesSizeInBytes = mPortBufInfo.offset;

  mvHeapInfo.setCapacity(getPlaneCount());
  mvBufInfo.setCapacity(getPlaneCount());
  totalSizeInBytes = 0;

  for (MUINT32 i = 0; i < getPlaneCount(); i++) {
    if (!helpCheckBufStrides(i, mBufStridesInBytesToAlloc[i])) {
      goto lbExit;
    }

    sp<HeapInfo> pHeapInfo = new HeapInfo;
    mvHeapInfo.push_back(pHeapInfo);
    pHeapInfo->heapID = mPortBufInfo.memID;

    sp<BufInfo> pBufInfo = new BufInfo;
    mvBufInfo.push_back(pBufInfo);

    // calculate size of planes
    size_t _size = helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i]);
    if (_size < mBufCustomSizeInBytesToAlloc[i])
      _size = mBufCustomSizeInBytesToAlloc[i];
    totalSizeInBytes += _size;

    pBufInfo->stridesInBytes = mBufStridesInBytesToAlloc[i];
    pBufInfo->sizeInBytes = _size;

    planesSizeInBytes += pBufInfo->sizeInBytes;
    //
    rvBufInfo[i]->stridesInBytes = pBufInfo->stridesInBytes;
    rvBufInfo[i]->sizeInBytes = pBufInfo->sizeInBytes;
  }

  ret = MTRUE;

lbExit:
  // MY_LOGD_IF(getLogCond(), "- ret:%d", ret);
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
DummyImageBufferHeap::impUninit(BufInfoVect_t const& /*rvBufInfo*/) {
  //
  // MY_LOGD_IF(getLogCond(), "-");

  if (mPortBufInfo.type == eDummyFromFD && mPortBufInfo.memID > 0) {
    close(mPortBufInfo.memID);
  }

  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
DummyImageBufferHeap::impLockBuf(char const* szCallerName,
                                 MINT usage,
                                 BufInfoVect_t const& rvBufInfo) {
  // From ISP7.0 do not support mva anymore.
  if (mPortBufInfo.type == eDummyFromFD && usage & mcam::eBUFFER_USAGE_HW_MASK)
    MY_LOGE("do not support map mva for eDummyFromFD so far");

  // map VA for dupFd
  if (mPortBufInfo.type == eDummyFromFD) {
    mPortBufInfo.virtAddr = (MUINTPTR)mmap(
        NULL, totalSizeInBytes + mPortBufInfo.offset, PROT_READ | PROT_WRITE,
        MAP_SHARED, mPortBufInfo.memID, 0);
    if (mPortBufInfo.virtAddr == 0 || mPortBufInfo.virtAddr == -1) {
      MY_LOGE("[mapVA] %s@ ion_mmap returns %#" PRIxPTR
              " - IonFD:%d sizeInBytes:%zu",
              szCallerName, mPortBufInfo.virtAddr, mPortBufInfo.memID,
              totalSizeInBytes + mPortBufInfo.offset);
      return MFALSE;
    }
  }

  MUINT32 offset = mPortBufInfo.offset;
  for (MUINT32 i = 0; i < rvBufInfo.size(); i++) {
    sp<BufInfo> pBufInfo = rvBufInfo[i];
    //  SW Access.
    pBufInfo->va = (((usage & mcam::eBUFFER_USAGE_SW_MASK) > 0)
                        ? mPortBufInfo.virtAddr + offset
                        : 0);

    //  HW Access.
    pBufInfo->pa = (((usage & mcam::eBUFFER_USAGE_HW_MASK) > 0)
                        ? mPortBufInfo.phyAddr + offset
                        : 0);

    offset += mvBufInfo[i]->sizeInBytes;
  }

  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
DummyImageBufferHeap::impUnlockBuf(char const* szCallerName,
                                   MINT usage,
                                   BufInfoVect_t const& rvBufInfo) {
  MBOOL ret = MTRUE;
  if (mPortBufInfo.type == eDummyFromFD && mPortBufInfo.memID > 0) {
    if (munmap(reinterpret_cast<void*>(mPortBufInfo.virtAddr),
               totalSizeInBytes + mPortBufInfo.offset)) {
      MY_LOGE("buf va munmap fail...");
      ret = MFALSE;
    }
  }

  for (MUINT32 i = 0; i < rvBufInfo.size(); i++) {
    sp<BufInfo> pBufInfo = rvBufInfo[i];
    //
    //  HW Access.
    if (0 != (usage & mcam::eBUFFER_USAGE_HW_MASK)) {
      if (0 != pBufInfo->pa) {
        pBufInfo->pa = 0;
      } else {
        MY_LOGW("%s@ skip PA=0 at %d-th plane", szCallerName, i);
      }
    }
    //
    //  SW Access.
    if (0 != (usage & mcam::eBUFFER_USAGE_SW_MASK)) {
      if (0 != pBufInfo->va) {
        pBufInfo->va = 0;
      } else {
        MY_LOGW("%s@ skip VA=0 at %d-th plane", szCallerName, i);
      }
    }
  }
  //
  return ret;
}

size_t DummyImageBufferHeap::getStartOffsetInBytes(size_t index) const {
  if (mPortBufInfo.offset < 0) {
    dumpCallStack(__FUNCTION__);
    MY_LOGF("invalid offset: %d", mPortBufInfo.offset);
    return 0;
  }
  return mPortBufInfo.offset;
}
