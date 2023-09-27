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
#if defined(MTK_ION_SUPPORT)
#define LOG_TAG "MtkCam/SecHeap"
//
#include <include/ISecureImageBufferHeap.h>
//
#include <linux/ion_drv.h>
#include <linux/mman-proprietary.h>
#include <linux/mtk_ion.h>
#include <sys/mman.h>
#include <sys/prctl.h>
//

#include <ion.h>
#include <ion/ion.h>
//
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/std/ULog.h>

#include <atomic>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
//


#include "include/BaseImageBufferHeap.h"
#include "include/MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);
//
// using namespace android;
// using namespace NSCam;
// using namespace NSCam::Utils;

using NSCam::ISecureImageBufferHeap;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;
using mcam::IImageBufferAllocator;
using android::sp;
using android::String8;

#define ALIGN(x, alignment) (((x) + ((alignment)-1)) & ~((alignment)-1))
#define ARRAY_SIZE(_Array) (sizeof(_Array) / sizeof(_Array[0]))

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
 *  Image Buffer Heap (ION).
 ******************************************************************************/
namespace {
// class SecureBlobDerivedIonHeap;
class SecureImageBufferHeap : public ISecureImageBufferHeap,
                              public BaseImageBufferHeap {
  //    friend  class SecureBlobDerivedIonHeap;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Params for Allocations.
  typedef ISecureImageBufferHeap::AllocImgParam_t AllocImgParam_t;
  typedef ISecureImageBufferHeap::AllocExtraParam AllocExtraParam;

 public:  ////                    Creation.
  static std::shared_ptr<ISecureImageBufferHeap> create(
      char const* szCallerName,
      char const* bufName,
      mcam::LegacyImgParam const& legacyParam,
      mcam::IImageBufferAllocator::ImgParam const& imgParam,
      MBOOL const enableLog = MTRUE);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  BaseImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  size_t getBufCustomSizeInBytes(size_t index) const override;

  virtual char const* impGetMagicName() const { return magicName(); }
  virtual void* impGetMagicInstance() const { return const_cast<void*>
                                (reinterpret_cast<const void *>(this)); }
  virtual HeapInfoVect_t const& impGetHeapInfo() const {
    return *(const_cast<HeapInfoVect_t*>
              (reinterpret_cast<const HeapInfoVect_t *>(&mvHeapInfo))); }
  virtual MBOOL impInit(BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impUninit(BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impReconfig(BufInfoVect_t const& rvBufInfo);

 public:  ////
  virtual MBOOL impLockBuf(char const* szCallerName,
                           MINT usage,
                           BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impUnlockBuf(char const* szCallerName,
                             MINT usage,
                             BufInfoVect_t const& rvBufInfo);

  virtual MBOOL isContinuous() const { return isContiguousPlanes(); }

 protected:  ////
  virtual android::String8 impPrintLocked() const;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    ION Info.
  struct MyIonInfo {
    ion_user_handle_t ionHandle = 0;
    int ionFd = -1;
    size_t sizeInBytes = 0;  // the real allocated size
    MINTPTR pa = 0;
    MINTPTR va = 0;
    int prot = 0;  // prot flag on mmap()
  };
  typedef std::vector<MyIonInfo> MyIonInfoVect_t;

 protected:  ////                    Heap Info.
  struct MyHeapInfo : public HeapInfo {};
  typedef android::Vector<android::sp<MyHeapInfo> > MyHeapInfoVect_t;

 protected:  ////                    Buffer Info.
  struct MyBufInfo : public BufInfo {};
  typedef android::Vector<android::sp<MyBufInfo> > MyBufInfoVect_t;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  virtual MBOOL isContiguousPlanes() const { return mContiguousPlanes; }
  virtual MBOOL doAllocIon(MyIonInfo& rIonInfo,
                           size_t boundaryInBytesToAlloc,
                           NSCam::SecType secType);
  virtual MVOID doDeallocIon(MyIonInfo& rIonInfo);

  virtual MBOOL doMapPhyAddr(char const* szCallerName, MyIonInfo& rIonInfo);
  virtual MBOOL doUnmapPhyAddr(char const* szCallerName, MyIonInfo& rIonInfo);
  virtual MBOOL doFlushCache();
  virtual std::vector<size_t> doCalculateOffsets(
      std::vector<size_t> const& vBoundaryInBytes,
      std::vector<size_t> const& vSizeInBytes) const;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Instantiation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Destructor/Constructors.
  /**
   * Disallowed to directly delete a raw pointer.
   */
  virtual ~SecureImageBufferHeap() {}
  SecureImageBufferHeap(char const* szCallerName,
                        char const* bufName,
                        AllocImgParam_t const& rImgParam,
                        AllocExtraParam const& rExtraParam);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Info to Allocate.
  AllocExtraParam mExtraParam;
  MBOOL const
      mContiguousPlanes;  // contiguous memory for multiple planes if MTRUE
  size_t mBufStridesInBytesToAlloc[3];  // buffer strides in bytes.
  std::vector<size_t>
      mBufBoundaryInBytesToAlloc;  // the address will be a multiple of boundary
                                   // in bytes, which must be a power of two.
  size_t mBufCustomSizeInBytesToAlloc[3];    // customized buffer size in bytes,
                                             // it may be 0 as default
  size_t mBufReusableSizeInBytesToAlloc[3];  // reusable buffer size in bytes,
                                             // it may be 0 as default

 protected:  ////                    Info of Allocated Result.
  MyIonInfoVect_t mvIonInfo;
  MyHeapInfoVect_t mvHeapInfo;  //
  MyBufInfoVect_t mvBufInfo;    //
  std::vector<size_t> mvBufOffsetInBytes;
};

};  // namespace

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<ISecureImageBufferHeap> ISecureImageBufferHeap::createPtr(
    char const* szCallerName,
    char const* bufName,
    mcam::LegacyImgParam const& legacyParam,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    MBOOL const enableLog) {
  return SecureImageBufferHeap::create(szCallerName, bufName, legacyParam,
                                       imgParam, enableLog);
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<ISecureImageBufferHeap> SecureImageBufferHeap::create(
    char const* szCallerName,
    char const* bufName,
    mcam::LegacyImgParam const& legacyParam,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    MBOOL const enableLog) {
  // TODO(Elon Hsu) consider sec type with buffer usage
  // 1. create heap from legacy API
  AllocExtraParam rExtraParam(imgParam.nocache, 0, 0, imgParam.continuousPlane,
                              imgParam.secType);

  // 2. create image buffer
  SecureImageBufferHeap* pHeap = NULL;
  pHeap = new SecureImageBufferHeap(szCallerName, bufName, legacyParam,
                                    rExtraParam);
  if (!pHeap) {
    CAM_ULOGME("%s@ Fail to new", (szCallerName ? szCallerName : "unknown"));
    return NULL;
  }
  //
  if (!pHeap->onCreate(legacyParam.imgSize, imgParam.colorspace,
                       legacyParam.imgFormat, legacyParam.bufSize,
                       rExtraParam.secType, enableLog,
                       imgParam.hwsync)) {
    CAM_ULOGME("%s@ onCreate", (szCallerName ? szCallerName : "unknown"));
    delete pHeap;
    return NULL;
  }

  // 3. create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<SecureImageBufferHeap> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

/******************************************************************************
 *
 ******************************************************************************/
SecureImageBufferHeap::SecureImageBufferHeap(char const* szCallerName,
                                             char const* bufName,
                                             AllocImgParam_t const& rImgParam,
                                             AllocExtraParam const& rExtraParam)
    : BaseImageBufferHeap(szCallerName, bufName)
      //
      ,
      mExtraParam(rExtraParam),
      mContiguousPlanes(rExtraParam.contiguousPlanes),
      mBufBoundaryInBytesToAlloc(rImgParam.bufBoundaryInBytes,
                                 rImgParam.bufBoundaryInBytes +
                                     ARRAY_SIZE(rImgParam.bufBoundaryInBytes))
      //
      ,
      mvHeapInfo(),
      mvBufInfo() {
  ::memcpy(mBufStridesInBytesToAlloc, rImgParam.bufStridesInBytes,
           sizeof(mBufStridesInBytesToAlloc));
  ::memcpy(mBufCustomSizeInBytesToAlloc, rImgParam.bufCustomSizeInBytes,
           sizeof(mBufCustomSizeInBytesToAlloc));
  ::memcpy(mBufReusableSizeInBytesToAlloc, rImgParam.bufReusableSizeInBytes,
           sizeof(mBufReusableSizeInBytesToAlloc));
}

/******************************************************************************
 * Buffer minimum size in bytes; always legal.
 ******************************************************************************/
size_t SecureImageBufferHeap::getBufCustomSizeInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGI("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
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
SecureImageBufferHeap::impInit(BufInfoVect_t const& rvBufInfo) {
  //  rIonInfo.pa
  auto mapPA = [this](char const* szCallerName, MyIonInfo& rIonInfo) {
    if (CC_UNLIKELY(!doMapPhyAddr(szCallerName, rIonInfo))) {
      CAM_ULOGME("[mapPA] %s@ doMapPhyAddr", szCallerName);
      return false;
    }
    return true;
  };

  MBOOL ret = MFALSE;
  MBOOL bMapPhyAddr = ((getImgFormat() & 0xFF00) == NSCam::eImgFmt_RAW_START ||
                       (getImgFormat() & 0xFF00) == NSCam::eImgFmt_STA_START);
  //
  //  Allocate memory and setup mBufHeapInfo & rBufHeapInfo.
  //  Allocated memory of each plane is not contiguous.
  std::vector<size_t> vSizeInBytesToAlloc(getPlaneCount(), 0);
  mvHeapInfo.setCapacity(getPlaneCount());
  mvBufInfo.setCapacity(getPlaneCount());
  for (size_t i = 0; i < getPlaneCount(); i++) {
    if (!helpCheckBufStrides(i, mBufStridesInBytesToAlloc[i])) {
      goto lbExit;
    }
    //
    {
      sp<MyHeapInfo> pHeapInfo = new MyHeapInfo;
      mvHeapInfo.push_back(pHeapInfo);
      //
      sp<MyBufInfo> pBufInfo = new MyBufInfo;
      mvBufInfo.push_back(pBufInfo);
      pBufInfo->stridesInBytes = mBufStridesInBytesToAlloc[i];
      // if the customized buffer size is greater than the expected buffer size,
      // uses the given buffer size for vertical padding usage.
      size_t bufSizeInBytesToAlloc =
          (mBufCustomSizeInBytesToAlloc[i] > mBufReusableSizeInBytesToAlloc[i])
              ? mBufCustomSizeInBytesToAlloc[i]
              : mBufReusableSizeInBytesToAlloc[i];
      pBufInfo->sizeInBytes =
          helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i]) >
                  bufSizeInBytesToAlloc
              ? helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i])
              : bufSizeInBytesToAlloc;
      if (bufSizeInBytesToAlloc != 0 &&
          helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i]) !=
              bufSizeInBytesToAlloc) {
        MY_LOGI(
            "special case, calc size(%zu), min size(%zu), reusable size(%zu), "
            "final size(%zu)",
            helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i]),
            mBufCustomSizeInBytesToAlloc[i], mBufReusableSizeInBytesToAlloc[i],
            pBufInfo->sizeInBytes);
      }
      //
      //  setup return buffer information.
      rvBufInfo[i]->stridesInBytes = pBufInfo->stridesInBytes;
      rvBufInfo[i]->sizeInBytes = pBufInfo->sizeInBytes;

      //  the real size(s) to allocate memory.
      vSizeInBytesToAlloc[i] = pBufInfo->sizeInBytes;

      // allocate reusable buffer, resume the current data format's buffer size
      // info
      if (mBufCustomSizeInBytesToAlloc[i] !=
          mBufReusableSizeInBytesToAlloc[i]) {
        pBufInfo->sizeInBytes =
            helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i]) >
                    mBufCustomSizeInBytesToAlloc[i]
                ? helpQueryBufSizeInBytes(i, mBufStridesInBytesToAlloc[i])
                : mBufCustomSizeInBytesToAlloc[i];
        MY_LOGD("resume pBufInfo: sizeInBytes(%zu --> %zu)",
                rvBufInfo[i]->sizeInBytes, pBufInfo->sizeInBytes);
        rvBufInfo[i]->sizeInBytes = pBufInfo->sizeInBytes;
      }
    }
  }
  //
  mvBufOffsetInBytes =
      doCalculateOffsets(mBufBoundaryInBytesToAlloc, vSizeInBytesToAlloc);
  //
  // Allocate memory and setup mvIonInfo & mvHeapInfo.
  if (!isContiguousPlanes()) {
    //  Usecases:
    //      default (for most cases)

    mvIonInfo.resize(getPlaneCount());
    for (size_t i = 0; i < getPlaneCount(); i++) {
      auto& rIonInfo = mvIonInfo[i];
      rIonInfo.sizeInBytes = vSizeInBytesToAlloc[i];
      if (!doAllocIon(rIonInfo, mBufBoundaryInBytesToAlloc[i],
                      mExtraParam.secType)) {
        MY_LOGE("doAllocIon");
        goto lbExit;
      }
      mvHeapInfo[i]->heapID = rIonInfo.ionFd;

      // map physical Address for raw buffer in creating period
      if (bMapPhyAddr) {
        if (CC_UNLIKELY(!mapPA(mCallerName.c_str(), rIonInfo))) {
          goto lbExit;
        }
        mvBufInfo[i]->pa = rIonInfo.pa;
        rvBufInfo[i]->pa = rIonInfo.pa;
      }
    }
  } else {
    //  Usecases:
    //      Blob heap -> multi-plane (e.g. YV12) heap

    mvIonInfo.resize(1);
    auto& rIonInfo = mvIonInfo[0];
    rIonInfo.sizeInBytes = mvBufOffsetInBytes.back() +
                           vSizeInBytesToAlloc.back();  // total size in bytes
    if (!doAllocIon(rIonInfo, mBufBoundaryInBytesToAlloc[0],
                    mExtraParam.secType)) {
      MY_LOGE("doAllocIon");
      goto lbExit;
    }

    for (size_t i = 0; i < getPlaneCount(); i++) {
      mvHeapInfo[i]->heapID = rIonInfo.ionFd;
    }

    // map physical Address for raw buffer in creating period
    if (bMapPhyAddr) {
      if (CC_UNLIKELY(!mapPA(mCallerName.c_str(), rIonInfo))) {
        goto lbExit;
      }
      MINTPTR const pa = rIonInfo.pa;
      for (size_t i = 0; i < rvBufInfo.size(); i++) {
        rvBufInfo[i]->pa = mvBufInfo[i]->pa = pa + mvBufOffsetInBytes[i];
      }
    }
  }
  //
  ret = MTRUE;
lbExit:
  if (!ret) {
    impUninit(rvBufInfo);
    mIonDevice.reset();
  }
  MY_LOGD_IF(1, "- ret:%d", ret);
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::impReconfig(BufInfoVect_t const& rvBufInfo) {
  String8 log;
  std::vector<size_t> vSizeInBytes(rvBufInfo.size(), 0);
  MY_LOGD("rvBufInfo.size:%zu,  mvBufInfo.size:%zu", rvBufInfo.size(),
          mvBufInfo.size());

  for (size_t i = 0; i < rvBufInfo.size(); ++i) {
    sp<BufInfo> pSrc = rvBufInfo[i];
    sp<MyBufInfo> pDst = mvBufInfo[i];

    pDst->stridesInBytes = pSrc->stridesInBytes;
    pDst->sizeInBytes = pSrc->sizeInBytes;
    mBufCustomSizeInBytesToAlloc[i] = pSrc->sizeInBytes;
    vSizeInBytes[i] = pSrc->sizeInBytes;
    //
    log += String8::format(
        "[%zu] special case, min size(%zu), reusable size(%zu), final "
        "size(%zu), final stride(%zu)\n",
        i, mBufCustomSizeInBytesToAlloc[i], mBufReusableSizeInBytesToAlloc[i],
        pDst->sizeInBytes, pDst->stridesInBytes);
  }
  mvBufOffsetInBytes =
      doCalculateOffsets(mBufBoundaryInBytesToAlloc, vSizeInBytes);
  log += String8::format("offset: ");
  for (auto const& offset : mvBufOffsetInBytes) {
    log += String8::format("%zu ", offset);
  }
  MY_LOGI("%s", log.c_str());
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::impUninit(BufInfoVect_t const& rvBufInfo) {
  MY_LOGD("rvBufInfo.size:%zu,  mvBufInfo.size:%zu", rvBufInfo.size(),
          mvBufInfo.size());

  for (size_t i = 0; i < mvBufInfo.size(); i++) {
    auto pHeapInfo = mvHeapInfo[i];
    auto pBufInfo = mvBufInfo[i];
    //
    pHeapInfo->heapID = -1;
    pBufInfo->va = 0;
    pBufInfo->pa = 0;
  }

  // unlock buffer for current used planes.
  for (size_t i = 0; i < rvBufInfo.size(); i++) {
    auto pbufinfo = rvBufInfo[i];
    pbufinfo->va = 0;
    pbufinfo->pa = 0;
  }

  for (size_t i = 0; i < mvIonInfo.size(); i++) {
    auto& rIonInfo = mvIonInfo[i];
    if (rIonInfo.va) {
      if (mIonDevice) {
        ::ion_munmap(mIonDevice->getDeviceFd(),
                     reinterpret_cast<void*>(rIonInfo.va),
                     rIonInfo.sizeInBytes);
      }
      rIonInfo.va = 0;
      rIonInfo.prot = 0;
    }
    if (rIonInfo.pa) {
      doUnmapPhyAddr(mCallerName.c_str(), rIonInfo);
      rIonInfo.pa = 0;
    }
    //
    doDeallocIon(rIonInfo);
  }
  //
  mIonDevice.reset();
  //
  MY_LOGD_IF(1, "-");
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
SecureImageBufferHeap::doDeallocIon(MyIonInfo& rIonInfo) {
  //  ion_munmap: virtual address
#if 0
    if  ( 0 != rIonInfo.va ) {
        ::ion_munmap(mIonDevice->getDeviceFd(),
        reinterpret_cast<void*>rIonInfo.va, rIonInfo.sizeInBytes);
        rIonInfo.va = 0;
    }
#endif
  //
  //  ion_share_close: buf fd
  if (0 <= rIonInfo.ionFd) {
    ::ion_share_close(mIonDevice->getDeviceFd(), rIonInfo.ionFd);
    rIonInfo.ionFd = -1;
  }
  //
  //  ion_free: buf handle
  if (0 != rIonInfo.ionHandle) {
    if (ion_free(mIonDevice->getDeviceFd(), rIonInfo.ionHandle)) {
      MY_LOGE("ion_free is failed : ionDevFD(%d), ionHandle(%d)",
              mIonDevice->getDeviceFd(), rIonInfo.ionHandle);
    } else {
      rIonInfo.ionHandle = 0;
    }
  }
}

MBOOL
SecureImageBufferHeap::doAllocIon(MyIonInfo& rIonInfo,
                                  size_t boundaryInBytesToAlloc,
                                  NSCam::SecType secType) {
  int err = 0;
  int ion_prot_flags =
      mExtraParam.nocache ? 0 : (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC);
  int sec_type_flags = (secType == NSCam::SecType::mem_secure)
                    ? reinterpret_cast<int>(ION_HEAP_MULTIMEDIA_TYPE_2D_FR_MASK)
                    : reinterpret_cast<int>(ION_HEAP_MULTIMEDIA_TYPE_PROT_MASK);
  MY_LOGD("Allocate secure memory type : %d", sec_type_flags);

  if (ion_alloc(mIonDevice->getDeviceFd(), rIonInfo.sizeInBytes,
                boundaryInBytesToAlloc, sec_type_flags, ion_prot_flags,
                &rIonInfo.ionHandle)) {
    MY_LOGE("ion_alloc failed: ionDevFD(%d), BufSize(%zu)",
            mIonDevice->getDeviceFd(), rIonInfo.sizeInBytes);
    goto lbExit;
  }

  // obtain a file descriptor that can pass to other clients
  if (ion_share(mIonDevice->getDeviceFd(), rIonInfo.ionHandle,
                &rIonInfo.ionFd)) {
    MY_LOGE(
        "ion_share failed: ionDevFD(%d), BufSize(%zu),"
        " ionHandle(%d)",
        mIonDevice->getDeviceFd(), rIonInfo.sizeInBytes, rIonInfo.ionHandle);
    goto lbExit;
  }

  {
    // ion_mm_buf_debug_info_t::dbg_name[48] is made up of as below:
    //      [  size ]-[   format   ]-[      caller name    ]
    //      [ max 9 ]-[   max 14   ]-[        max 23       ]
    //  For example:
    //      5344x3008-YUY2-ZsdShot:Yuv
    //      1920x1080-FG_BAYER10-Hal:Image:Resiedraw
    std::string resolution =
        std::to_string(getImgSize().w) + "x" + std::to_string(getImgSize().h);
    std::string format =
        NSCam::Utils::Format::queryImageFormatName(getImgFormat());
    std::string dbgName = resolution.substr(0, 9) + "-" + format.substr(0, 14) +
                          "-" + mCallerName.c_str();

    struct ion_mm_data data;
    data.mm_cmd = ION_MM_SET_DEBUG_INFO;
    data.buf_debug_info_param.handle = rIonInfo.ionHandle;
    ::strncpy(data.buf_debug_info_param.dbg_name, dbgName.c_str(),
              sizeof(ion_mm_buf_debug_info_t::dbg_name));
    data.buf_debug_info_param
        .dbg_name[sizeof(ion_mm_buf_debug_info_t::dbg_name) - 1] = '\0';
    data.buf_debug_info_param.value1 = 0;
    data.buf_debug_info_param.value2 = 0;
    data.buf_debug_info_param.value3 = 0;
    data.buf_debug_info_param.value4 = 0;
    err = ::ion_custom_ioctl(mIonDevice->getDeviceFd(), ION_CMD_MULTIMEDIA,
                             &data);
    if (CC_UNLIKELY(0 != err)) {
      MY_LOGE("ion_custom_ioctl(ION_MM_SET_DEBUG_INFO) returns %d, BufFD:%d",
              err, rIonInfo.ionFd);
      goto lbExit;
    }
  }

  return MTRUE;
lbExit:
  return MFALSE;
}

/******************************************************************************
 *
 ******************************************************************************/
/* Performance consideration for Lock/unLockBuf
 *  Keep va/pa in mvBufInfo to avoid map va/pa each time and
 *  the va/pa will only be unmapped when destroying the heap.
 */
/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::impLockBuf(char const* szCallerName,
                                  MINT usage,
                                  BufInfoVect_t const& rvBufInfo) {
  auto fillSecureHandle = [](const struct ion_sys_data& sysData, MINT usage,
                             auto& myBufInfo, auto& bufInfo) {
    if (usage & mcam::eBUFFER_USAGE_SW_MASK) {
      myBufInfo->va = sysData.get_phys_param.phy_addr;
      bufInfo->va = sysData.get_phys_param.phy_addr;
    }

    if (usage & mcam::eBUFFER_USAGE_HW_MASK) {
      myBufInfo->pa = sysData.get_phys_param.phy_addr;
      bufInfo->pa = sysData.get_phys_param.phy_addr;
    }
  };

  MBOOL ret = MFALSE;
  if (mvIonInfo.size() == rvBufInfo.size()) {
    for (size_t i = 0; i < rvBufInfo.size(); i++) {
      auto& rIonInfo = mvIonInfo[i];
      struct ion_sys_data sys_data;

      //  SW and/or HW Access
      if ((usage & mcam::eBUFFER_USAGE_SW_MASK) ||
          (usage & mcam::eBUFFER_USAGE_HW_MASK)) {
        sys_data.sys_cmd = ION_SYS_GET_PHYS;
        sys_data.get_phys_param.handle = rIonInfo.ionHandle;
        if (ion_custom_ioctl(mIonDevice->getDeviceFd(), ION_CMD_SYSTEM,
                             &sys_data)) {
          MY_LOGE("ion_custom_ioctl failed to get secure handle");
          goto lbExit;
        }

        MY_LOGI("Secure memory locked: handle(0x%x) size(%lu)",
                sys_data.get_phys_param.phy_addr, sys_data.get_phys_param.len);

        fillSecureHandle(sys_data, usage, mvBufInfo[i], rvBufInfo[i]);
      }
    }

  } else if (mvIonInfo.size() == 1) {
    // Usecases: Blob heap -> multi-plane (e.g. YV12) heap
    auto& rIonInfo = mvIonInfo[0];
    struct ion_sys_data sys_data;

    //  SW and/or HW Access
    if ((usage & mcam::eBUFFER_USAGE_SW_MASK) ||
        (usage & mcam::eBUFFER_USAGE_HW_MASK)) {
      sys_data.sys_cmd = ION_SYS_GET_PHYS;
      sys_data.get_phys_param.handle = rIonInfo.ionHandle;
      if (ion_custom_ioctl(mIonDevice->getDeviceFd(), ION_CMD_SYSTEM,
                           &sys_data)) {
        MY_LOGE("ion_custom_ioctl failed to get secure handle");
        goto lbExit;
      }

      MY_LOGI("Secure memory allocated: handle(0x%x) size(%lu)",
              sys_data.get_phys_param.phy_addr, sys_data.get_phys_param.len);
      for (size_t i = 0; i < rvBufInfo.size(); i++)
        fillSecureHandle(sys_data, usage, mvBufInfo[i], rvBufInfo[i]);
    }
  }

  ret = MTRUE;

lbExit:
  if (!ret)
    impUnlockBuf(szCallerName, usage, rvBufInfo);

  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::impUnlockBuf(char const* szCallerName,
                                    MINT usage,
                                    BufInfoVect_t const& rvBufInfo) {
  for (size_t i = 0; i < rvBufInfo.size(); i++) {
    auto& pBufInfo(rvBufInfo[i]);

    //  HW Access
    if (usage & mcam::eBUFFER_USAGE_HW_MASK) {
      if (0 != pBufInfo->pa)
        pBufInfo->pa = 0;
      else
        MY_LOGW("%s@ skip PA=0 at %zu-th plane", szCallerName, i);
    }

    //  SW Access
    if (usage & mcam::eBUFFER_USAGE_SW_MASK) {
      if (0 != pBufInfo->va)
        pBufInfo->va = 0;
      else
        MY_LOGW("%s@ skip VA=0 at %zu-th plane", szCallerName, i);
    }
  }

  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::doMapPhyAddr(char const* szCallerName,
                                    MyIonInfo& rIonInfo) {
  HelperParamMapPA param;
  param.phyAddr = 0;
  param.virAddr = rIonInfo.va;
  param.ionFd = rIonInfo.ionFd;
  param.size = rIonInfo.sizeInBytes;
  param.security = mExtraParam.security;
  param.coherence = mExtraParam.coherence;
  if (!helpMapPhyAddr(szCallerName, param)) {
    MY_LOGE("helpMapPhyAddr");
    return MFALSE;
  }
  //
  rIonInfo.pa = param.phyAddr;
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::doUnmapPhyAddr(char const* szCallerName,
                                      MyIonInfo& rIonInfo) {
  HelperParamMapPA param;
  param.phyAddr = rIonInfo.pa;
  param.virAddr = rIonInfo.va;
  param.ionFd = rIonInfo.ionFd;
  param.size = rIonInfo.sizeInBytes;
  param.security = mExtraParam.security;
  param.coherence = mExtraParam.coherence;
  if (!helpUnmapPhyAddr(szCallerName, param)) {
    MY_LOGE("helpUnmapPhyAddr");
    return MFALSE;
  }
  //
  rIonInfo.pa = 0;
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
SecureImageBufferHeap::doFlushCache() {
#if 0
    Vector<HelperParamFlushCache> vParam;
    vParam.insertAt(0, mvHeapInfo.size());
    HelperParamFlushCache*const aParam = vParam.editArray();
    for (MUINT i = 0; i < vParam.size(); i++) {
        aParam[i].phyAddr = mvBufInfo[i]->pa;
        aParam[i].virAddr = mvBufInfo[i]->va;
        aParam[i].ionFd   = mvHeapInfo[i]->heapID;
        aParam[i].size    = mvBufInfo[i]->sizeInBytes;
    }
    if  ( !helpFlushCache(aParam, vParam.size()) ) {
        MY_LOGE("helpFlushCache");
        return  MFALSE;
    }
#endif
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
std::vector<size_t> SecureImageBufferHeap::doCalculateOffsets(
    std::vector<size_t> const& vBoundaryInBytes,
    std::vector<size_t> const& vSizeInBytes) const {
  std::vector<size_t> vOffset(getPlaneCount(),
                              0);  // offset=0 for multiple ion heap
  if (isContiguousPlanes()) {
    MINTPTR start = 0;
    for (size_t i = 0; i < vOffset.size(); i++) {
      start = (0 == vBoundaryInBytes[i] ? start
                                        : ALIGN(start, vBoundaryInBytes[i]));
      vOffset[i] = start;
      start += vSizeInBytes[i];
      // MY_LOGW_IF(0 < i && 0 == vOffset[i], "%zu start:%zu offset:%zu
      // size:%zu", i, start, vOffset[i], vSizeInBytes[i]);
    }
  }
  return vOffset;
}

/******************************************************************************
 *
 ******************************************************************************/
android::String8 SecureImageBufferHeap::impPrintLocked() const {
  android::String8 s;
  //
  s += android::String8::format("ion{fd/handle/size/pa/va}:[");
  for (auto const& v : mvIonInfo) {
    s += android::String8::format(" (%d/%#x %zu %08" PRIxPTR " %08" PRIxPTR ")",
                                  v.ionFd, v.ionHandle, v.sizeInBytes, v.pa,
                                  v.va);
  }
  s += " ]";
  //
  s += android::String8::format(" offset:[");
  for (auto const& v : mvBufOffsetInBytes) {
    s += android::String8::format(" %zu", v);
  }
  s += " ]";
  //
  s += android::String8::format(" boundary:[");
  for (size_t i = 0;
       i < mvBufOffsetInBytes.size() && i < mBufBoundaryInBytesToAlloc.size();
       i++) {
    s += android::String8::format(" %zu", mBufBoundaryInBytesToAlloc[i]);
  }
  s += " ]";
  //
  s += android::String8::format(" contiguousPlanes:%c",
                                (isContiguousPlanes() ? 'Y' : 'N'));
  //
  return s;
}

#endif  // MTK_ION_SUPPORT
