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

#define LOG_TAG "MtkCam/ImgBuf"

#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/debug/Properties.h>

#include <uree/mem.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <uree/system.h>
#include <utils/String8.h>

#include <string>
#include <memory>
#include <limits>
#include <vector>

#include "include/BaseImageBuffer.h"
#include "include/MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);

// using namespace android;
// using namespace NSCam;
// using namespace NSCam::Utils;
// using namespace NSCam::Utils::ULog;
// using namespace mcam::NSImageBuffer;
// using namespace mcam::NSImageBufferHeap;

using android::String8;
using android::Mutex;
using mcam::NSImageBuffer::BaseImageBuffer;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;
using NSCam::Utils::dumpCallStack;
using NSCam::Utils::Format::checkValidBufferInfo;
using NSCam::Utils::Format::queryPlaneWidthInPixels;
using NSCam::Utils::Format::queryPlaneHeightInPixels;
using NSCam::Utils::Format::queryPlaneBitsPerPixel;
using NSCam::Utils::Format::queryImageBitsPerPixel;
//using NSCam::Utils::ULog::ULogPrinter;
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
//
#define BYTE2PIXEL(BPP, BYTE) ((BYTE << 3) / BPP)
#define PIXEL2BYTE(BPP, PIXEL) ((PIXEL * BPP) >> 3)

/******************************************************************************
 *
 ******************************************************************************/
// Note(Elon Hsu) the implementation of deconstructor is the same as
// onLastStrongRef
BaseImageBuffer::~BaseImageBuffer() {
  MY_LOGD_IF(mspImgBufHeap->getLogCond(), "this:%p %dx%d format:%#x", this,
             mImgSize.w, mImgSize.h, mImgFormat);

  //
  //
  mvImgBufInfo.clear();
  mvBufHeapInfo.clear();
  //
  if (0 != mLockCount) {
    // MY_LOGE("Not unlock before release heap - LockCount:%d", mLockCount);
    this->dumpBuffer();
    dumpCallStack(__FUNCTION__);
  }
  //
  if (mspImgBufHeap != 0) {
    mspImgBufHeap = 0;
  }
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::onCreate() {
  MY_LOGD_IF(1, "%s@ this:%p %dx%d, offset(%zu) fmt(0x%x), plane(%zu)",
             getMagicName(), this, getImgSize().w, getImgSize().h, mOffset,
             getImgFormat(), getPlaneCount());
  //
  mvBufHeapInfo.clear();
  mvBufHeapInfo.setCapacity(mspImgBufHeap->getPlaneCount());
  String8 str;
  for (size_t i = 0; i < mspImgBufHeap->getPlaneCount(); i++) {
    mvBufHeapInfo.push_back(new BaseImageBufferHeap::BufInfo);
    mvBufHeapInfo[i]->stridesInBytes = mspImgBufHeap->getBufStridesInBytes(i);
    mvBufHeapInfo[i]->sizeInBytes = mspImgBufHeap->getBufSizeInBytes(i);
    if (mspImgBufHeap->getLogCond()) {
      str.appendFormat("plane(%zu) heap stride(%zu)(bytes), sizeInBytes(%zu); ",
                       i, mvBufHeapInfo[i]->stridesInBytes,
                       mvBufHeapInfo[i]->sizeInBytes);
    }
  }
  MY_LOGD_IF(0, "%s", str.string());
  //
  mvImgBufInfo.clear();
  mvImgBufInfo.setCapacity(getPlaneCount());
  for (size_t i = 0; i < getPlaneCount(); ++i) {
    mvImgBufInfo.push_back(new ImgBufInfo);
  }
  //
  size_t imgBufSize = 0;  // buffer size of n planes.
  //
  for (size_t i = 0; i < mvImgBufInfo.size(); ++i) {
    bool bCheckValid = checkValidBufferInfo(getImgFormat());

    // (plane) strides in bytes
    mvImgBufInfo[i]->stridesInBytes = mStrides[i];
    //
    // (plane) offset in bytes
    if (NSCam::eImgFmt_BLOB != mspImgBufHeap->getImgFormat()) {
      size_t const planeOffsetInPixels = queryPlaneWidthInPixels(
          getImgFormat(), i, BYTE2PIXEL(getPlaneBitsPerPixel(i), mOffset));
      mvImgBufInfo[i]->offsetInBytes = PIXEL2BYTE(
          getPlaneBitsPerPixel(i), planeOffsetInPixels);  // size in bytes.
    } else {
      mvImgBufInfo[i]->offsetInBytes = mOffset;
    }

    if (bCheckValid) {
      if (NSCam::eImgFmt_BLOB != mspImgBufHeap->getImgFormat()) {
        MY_LOGW_IF(mOffset != mvImgBufInfo[i]->offsetInBytes,
                   "%s@ Bad offset at %zu-th plane: mOffset(%zu) -> "
                   "(%f)(pixels) -> offsetInBytes(%zu)",
                   getMagicName(), i, mOffset,
                   (MFLOAT)mOffset * 8 / getPlaneBitsPerPixel(i),
                   mvImgBufInfo[i]->offsetInBytes);
      }
    }
    //
    // (plane) size in bytes
    size_t const imgWidthInPixels = queryPlaneWidthInPixels(
        getImgFormat(), i, (size_t)getImgSize().w);
    size_t const imgHeightInPixels = queryPlaneHeightInPixels(
        getImgFormat(), i, (size_t)getImgSize().h);
    size_t const planeBitsPerPixel = getPlaneBitsPerPixel(i);
    size_t const roundUpValue =
        (imgWidthInPixels * planeBitsPerPixel % 8 > 0) ? 1 : 0;
    size_t const imgWidthInBytes =
        (imgWidthInPixels * planeBitsPerPixel / 8) + roundUpValue;
    //
    if (bCheckValid) {
      if (mvImgBufInfo[i]->stridesInBytes <= 0 ||
          mvImgBufInfo[i]->stridesInBytes < imgWidthInBytes) {
        MY_LOGE(
            "%s@ Bad result at %zu-th plane: bpp(%zu), width(%zu pixels/%zu "
            "bytes), strides(%zu bytes)",
            getMagicName(), i, planeBitsPerPixel, imgWidthInPixels,
            imgWidthInBytes, mvImgBufInfo[i]->stridesInBytes);
        return MFALSE;
      }
    }
    switch (getImgFormat()) {
      // [NOTE] create JPEG image buffer from BLOB heap.
      case NSCam::eImgFmt_JPEG:
      case NSCam::eImgFmt_BLOB:
        mvImgBufInfo[i]->sizeInBytes = mvImgBufInfo[i]->stridesInBytes;
        break;
      default:
        // UFO don't need to check this, Blob=>UFO will report error while
        // getBufCustomSizeInBytes called
        if (bCheckValid && mspImgBufHeap->getBufCustomSizeInBytes(i) != 0) {
          mvImgBufInfo[i]->sizeInBytes =
              (0 == mvImgBufInfo[i]->offsetInBytes)
                  ? mspImgBufHeap->getBufSizeInBytes(i)
                  : mvImgBufInfo[i]->stridesInBytes * (imgHeightInPixels - 1) +
                        imgWidthInBytes;
          if (mvImgBufInfo[i]->stridesInBytes * imgHeightInPixels !=
                  mspImgBufHeap->getBufSizeInBytes(i) ||
              0 != mvImgBufInfo[i]->offsetInBytes) {
            MY_LOGD(
                "special case, fmt(%d), plane(%zu), s(%zu), w(%zu), h(%zu), "
                "offset(%zu), s*h(%zu), heap size(%zu),",
                getImgFormat(), i, mvImgBufInfo[i]->stridesInBytes,
                imgWidthInBytes, imgHeightInPixels,
                mvImgBufInfo[i]->offsetInBytes,
                mvImgBufInfo[i]->stridesInBytes * imgHeightInPixels,
                mspImgBufHeap->getBufSizeInBytes(i));
          }
        } else {
          mvImgBufInfo[i]->sizeInBytes =
              (0 == mvImgBufInfo[i]->offsetInBytes)
                  ? mvImgBufInfo[i]->stridesInBytes * imgHeightInPixels
                  : mvImgBufInfo[i]->stridesInBytes * (imgHeightInPixels - 1) +
                        imgWidthInBytes;
        }
        break;
    }

    imgBufSize += mvImgBufInfo[i]->sizeInBytes;
    //
    if (NSCam::eImgFmt_BLOB !=
        mspImgBufHeap->getImgFormat()) {  // check  ROI(x,y) + ROI(w,h) <= heap
                                          // stride(w,h)
      if (bCheckValid) {
        size_t const planeStartXInBytes =
            mvImgBufInfo[i]->offsetInBytes %
            mspImgBufHeap->getBufStridesInBytes(i);
        size_t const planeStartYInBytes =
            mvImgBufInfo[i]->offsetInBytes /
            mspImgBufHeap->getBufStridesInBytes(i);
        size_t const planeStartXInPixels =
            BYTE2PIXEL(getPlaneBitsPerPixel(i), planeStartXInBytes);
        size_t const planeStartYInPixels =
            BYTE2PIXEL(getPlaneBitsPerPixel(i), planeStartYInBytes);
        size_t const planeStridesInPixels = BYTE2PIXEL(
            getPlaneBitsPerPixel(i), mspImgBufHeap->getBufStridesInBytes(i));
        size_t const planeHeightInPixels =
            queryPlaneHeightInPixels(getImgFormat(), i, getImgSize().h);
        NSCam::MRect roi(
            NSCam::MPoint(planeStartXInPixels, planeStartYInPixels),
            MSize(imgWidthInPixels, imgHeightInPixels));
        MY_LOGW_IF(mspImgBufHeap->getLogCond() &&
                       mspImgBufHeap->getBufStridesInBytes(i) !=
                           (MUINT32)PIXEL2BYTE(getPlaneBitsPerPixel(i),
                                               planeStridesInPixels),
                   "%s@ Bad stride at %zu-th plane: heapStridesInBytes(%zu) -> "
                   "(%f)(pixels) -> StridesInBytes(%zu)",
                   getMagicName(), i, mspImgBufHeap->getBufStridesInBytes(i),
                   (MFLOAT)mspImgBufHeap->getBufStridesInBytes(i) * 8 /
                       getPlaneBitsPerPixel(i),
                   PIXEL2BYTE(getPlaneBitsPerPixel(i), planeStridesInPixels));
        if ((size_t)roi.leftTop().x + (size_t)roi.width() >
                planeStridesInPixels ||
            (size_t)roi.leftTop().y + (size_t)roi.height() >
                planeHeightInPixels) {
          MY_LOGE(
              "%s@ Bad image buffer at %zu-th plane: strides:%zux%zu(pixels), "
              "roi:(%d,%d,%d,%d)",
              getMagicName(), i, planeStridesInPixels, planeHeightInPixels,
              roi.leftTop().x, roi.leftTop().y, roi.width(), roi.height());
          return MFALSE;
        }
        if (mvImgBufInfo[i]->offsetInBytes + mvImgBufInfo[i]->sizeInBytes >
            mspImgBufHeap->getBufSizeInBytes(i)) {
          MY_LOGE(
              "%s@ Bad image buffer at %zu-th plane: offset(%zu) + "
              "bufSize(%zu) > heap bufSize(%zu)",
              getMagicName(), i, getBufOffsetInBytes(i), getBufSizeInBytes(i),
              mspImgBufHeap->getBufSizeInBytes(i));
          return MFALSE;
        }
      }
    } else if (NSCam::eImgFmt_BLOB == getImgFormat() ||
               NSCam::eImgFmt_JPEG ==
                   getImgFormat()) {  // check BLOB buffer size <= BLOB heap
                                      // size
      if (mvImgBufInfo[i]->offsetInBytes + mvImgBufInfo[i]->sizeInBytes >
          mspImgBufHeap->getBufSizeInBytes(i)) {
        MY_LOGE(
            "%s@ blob buffer offset(%zu)(bytes) + size(%zu) > blob heap buffer "
            "size(%zu)",
            getMagicName(), getBufOffsetInBytes(i), getBufSizeInBytes(i),
            mspImgBufHeap->getBufSizeInBytes(i));
        return MFALSE;
      }
    }
  }
  //
  if (NSCam::eImgFmt_BLOB == mspImgBufHeap->getImgFormat() &&
      NSCam::eImgFmt_BLOB !=
          getImgFormat()) {  // create non-BLOB image buffer from BLOB heap.
    if (imgBufSize > mspImgBufHeap->getBufSizeInBytes(0)) {
      for (size_t i = 0; i < getPlaneCount(); i++) {
        MY_LOGE("plane(%zu) bit(%zu), buf stride(%zu), bufSize(%zu)", i,
                getPlaneBitsPerPixel(i), getBufStridesInBytes(i),
                getBufSizeInBytes(i));
      }
      MY_LOGE("%s@ buffer size(%zu) > blob heap buffer size(%zu)",
              getMagicName(), imgBufSize, mspImgBufHeap->getBufSizeInBytes(0));
      return MFALSE;
    }
  }
  //
  if (mspImgBufHeap->getLogCond()) {
    str.clear();
    str.appendFormat("%s@ this:%p %dx%d, fmt(0x%x), secType(%u), ",
                     getMagicName(), this, getImgSize().w, getImgSize().h,
                     getImgFormat(), toLiteral(getSecType()));
    for (size_t i = 0; i < getPlaneCount(); i++) {
      str.appendFormat(
          "plane(%zu) bit(%zu), buf offset(%zu), stride(%zu), bufSize(%zu); ",
          i, getPlaneBitsPerPixel(i), getBufOffsetInBytes(i),
          getBufStridesInBytes(i), getBufSizeInBytes(i));
    }
    MY_LOGD("%s", str.string());
  }
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
void BaseImageBuffer::setColorArrangement(MINT32 const colorArrangement) {
  Mutex::Autolock _l(mLockMtx);
  mColorArrangement = colorArrangement;
  mspImgBufHeap->setColorArrangement(colorArrangement);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::setImgDesc(NSCam::ImageDescId id,
                            MINT64 value,
                            MBOOL overwrite) {
  return mspImgBufHeap->setImgDesc(id, value, overwrite);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::getImgDesc(NSCam::ImageDescId id, MINT64& value) const {
  return mspImgBufHeap->getImgDesc(id, value);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::setBitstreamSize(size_t const bitstreamsize) {
  if (NSCam::eImgFmt_JPEG != getImgFormat()) {
    MY_LOGE("%s@ wrong format(0x%x), can not set bitstream size",
            getMagicName(), getImgFormat());
    return MFALSE;
  }
  if (bitstreamsize > mspImgBufHeap->getBufSizeInBytes(0)) {
    MY_LOGE("%s@ bitstream size(%zu) > heap buffer size(%zu)", getMagicName(),
            bitstreamsize, mspImgBufHeap->getBufSizeInBytes(0));
    return MFALSE;
  }
  //
  mBitstreamSize = bitstreamsize;
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBuffer::getPlaneBitsPerPixel(size_t index) const {
  return queryPlaneBitsPerPixel(getImgFormat(), index);
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBuffer::getImgBitsPerPixel() const {
  return queryImageBitsPerPixel(getImgFormat());
}

/******************************************************************************
 *
 ******************************************************************************/
MINT32
BaseImageBuffer::getPlaneFD(size_t index) const {
  if (mspImgBufHeap->getHeapIDCount() == 1 &&
      mspImgBufHeap->getImgFormat() == eImgFmt_BLOB)
    return mspImgBufHeap->getHeapID(0);
  else
    return mspImgBufHeap->getHeapID(index);
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBuffer::getBufOffsetInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  return mvImgBufInfo[index]->offsetInBytes;
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBuffer::getPlaneOffsetInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  Mutex::Autolock _l(mLockMtx);

  // One plane heap (BLOB) or heap is continuous (conti-YUV)
  if (mspImgBufHeap->isContinuous() == MTRUE ||
      mspImgBufHeap->getPlaneCount() == 1) {
    // calc offset for different plane
    size_t offset = 0;

    for (size_t i = 0; i < index; ++i)
      offset += mvImgBufInfo[i]->sizeInBytes;

    auto res = mspImgBufHeap->getStartOffsetInBytes(index) +
               mvImgBufInfo[index]->offsetInBytes + offset;
    return res;
  } else {
    auto res = mspImgBufHeap->getStartOffsetInBytes(index) +
               mvImgBufInfo[index]->offsetInBytes;
    return res;
  }
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
//   MUINT32 offset = getBufOffsetInBytes(index);
//   //
//   Mutex::Autolock _l(mLockMtx);
//   //
//   if (0 == mLockCount || 0 == (mLockUsage & mcam::eBUFFER_USAGE_HW_MASK)) {
//     MY_LOGE(
//         "This call is legal only after lockBuf() with HW usage"
//         "- LockCount:%d Usage:%#x",
//         mLockCount, mLockUsage);
//     dumpCallStack(__FUNCTION__);
//     return 0;
//   }

//   //
//   // Buf PA(i) = Heap PA(i) + Buf Offset(i)
//   return mvImgBufInfo[index]->pa + offset;
// }

/******************************************************************************
 * Buffer virtual address; legal only after lock() with SW usage.
 ******************************************************************************/
MINTPTR
BaseImageBuffer::getBufVA(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  size_t offset = getBufOffsetInBytes(index);
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
  // Buf VA(i) = Heap VA(i) + Buf Offset(i)
  return mvImgBufInfo[index]->va + offset;
}

/******************************************************************************
 * Buffer size in bytes; always legal.
 ******************************************************************************/
size_t BaseImageBuffer::getBufSizeInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  return mvImgBufInfo[index]->sizeInBytes;
}

/******************************************************************************
 * Buffer Strides in bytes; always legal.
 ******************************************************************************/
size_t BaseImageBuffer::getBufStridesInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  return mvImgBufInfo[index]->stridesInBytes;
}

/******************************************************************************
 * Buffer Strides in pixel; always legal.
 ******************************************************************************/
size_t BaseImageBuffer::getBufStridesInPixel(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  const auto bpp = getPlaneBitsPerPixel(index);
  if (__builtin_expect(bpp == 0, false)) {
    MY_LOGE("Bad BitsPerPixel value(0) at plane %zu", index);
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  return (mvImgBufInfo[index]->stridesInBytes * 8) / bpp;  // unit of bbp is bit
}

/******************************************************************************
 * Buffer Scanlines; always legal.
 ******************************************************************************/
size_t BaseImageBuffer::getBufScanlines(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  if (__builtin_expect(mvImgBufInfo[index]->stridesInBytes == 0, false)) {
    MY_LOGE("Bad stridesInBytes value (0), index=%zu", index);
    dumpCallStack(__FUNCTION__);
    return 0;
  }

  return mvImgBufInfo[index]->sizeInBytes / mvImgBufInfo[index]->stridesInBytes;
}
/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::setExtParam(MSize const& imgSize, size_t offsetInBytes) {
  MBOOL ret = MFALSE;
  size_t const sizeInBytes = getBufSizeInBytes(0) + getBufOffsetInBytes(0);
  size_t const bitsPerPixel = getPlaneBitsPerPixel(0);
  size_t const strideInBytes = getBufStridesInBytes(0);
  size_t const strideInPixels = BYTE2PIXEL(bitsPerPixel, strideInBytes);
  size_t const roundUpValue = (imgSize.w * bitsPerPixel % 8 > 0) ? 1 : 0;
  size_t const imgWidthInBytes = (imgSize.w * bitsPerPixel / 8) + roundUpValue;
  size_t const imgSizeInBytes =
      (0 == offsetInBytes) ? strideInBytes * imgSize.h
                           : strideInBytes * (imgSize.h - 1) + imgWidthInBytes;

  if ((size_t)imgSize.w > strideInPixels || imgSize.h > mBufHeight) {
    MY_LOGE("invalid image size(%dx%d)>(%zux%d), strideInBytes(%zu)", imgSize.w,
            imgSize.h, strideInPixels, mBufHeight, strideInBytes);
    MY_LOGE("Input: %dx%d offsetInBytes:%zu", imgSize.w, imgSize.h,
            offsetInBytes);
    auto pHeap = mspImgBufHeap;
    if (pHeap != nullptr) {
      android::LogPrinter logPrinter(LOG_TAG, ANDROID_LOG_ERROR, "[setExtParam] ");
      pHeap->print(logPrinter, 0);
    }
    goto lbExit;
  }
  if (imgSizeInBytes + offsetInBytes > sizeInBytes) {
    MY_LOGE("oversize S(%dx%d):(%zu) + Offset(%zu) > original size(%zu)",
            imgSize.w, imgSize.h, imgSizeInBytes, offsetInBytes, sizeInBytes);
    goto lbExit;
  }
  //
  //
  {
    Mutex::Autolock _l(mLockMtx);
    //
    if (mImgSize != imgSize ||
        mvImgBufInfo[0]->extOffsetInBytes != offsetInBytes) {
      // MY_LOGD("update imgSize(%dx%d -> %dx%d), offset(%zu->%zu) @0-plane",
      //    mImgSize.w, mImgSize.h, imgSize.w, imgSize.h,
      //    mvImgBufInfo[0]->extOffsetInBytes, offsetInBytes);
      //
      mImgSize = imgSize;
      for (size_t i = 0; i < getPlaneCount(); i++) {
        mvImgBufInfo[i]->extOffsetInBytes = queryPlaneWidthInPixels(
            getImgFormat(), i,
            BYTE2PIXEL(getPlaneBitsPerPixel(i), offsetInBytes));
      }
    }
  }
  ret = MTRUE;
lbExit:
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
size_t BaseImageBuffer::getExtOffsetInBytes(size_t index) const {
  if (index >= getPlaneCount()) {
    MY_LOGE("Bad index(%zu) >= PlaneCount(%zu)", index, getPlaneCount());
    dumpCallStack(__FUNCTION__);
    return 0;
  }
  //
  //
  Mutex::Autolock _l(mLockMtx);
  //
  return mvImgBufInfo[index]->extOffsetInBytes;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::lockBuf(char const* szCallerName, MINT usage) {
  Mutex::Autolock _l(mLockMtx);
  //
  MBOOL ret = lockBufLocked(szCallerName, usage);
  //
  if (getPlaneCount() == mspImgBufHeap->getPlaneCount()) {
    for (size_t i = 0; i < mvImgBufInfo.size(); ++i) {
      mvImgBufInfo[i]->pa = mvBufHeapInfo[i]->pa;
      mvImgBufInfo[i]->va = mvBufHeapInfo[i]->va;
    }
  } else {  // non-BLOB image buffer created from BLOB heap.
    mvImgBufInfo[0]->pa = mvBufHeapInfo[0]->pa;
    mvImgBufInfo[0]->va = mvBufHeapInfo[0]->va;
    for (size_t i = 1; i < mvImgBufInfo.size(); ++i) {
      mvImgBufInfo[i]->pa =
          (0 == mvImgBufInfo[0]->pa)
              ? 0
              : mvImgBufInfo[i - 1]->pa + mvImgBufInfo[i - 1]->sizeInBytes;
      mvImgBufInfo[i]->va =
          (0 == mvImgBufInfo[0]->va)
              ? 0
              : mvImgBufInfo[i - 1]->va + mvImgBufInfo[i - 1]->sizeInBytes;
    }
  }
  //
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::unlockBuf(char const* szCallerName) {
  Mutex::Autolock _l(mLockMtx);
  //
  MBOOL ret = unlockBufLocked(szCallerName);
  //
  if (getPlaneCount() == mspImgBufHeap->getPlaneCount()) {
    for (size_t i = 0; i < mvImgBufInfo.size(); ++i) {
      mvImgBufInfo[i]->pa = mvBufHeapInfo[i]->pa;
      mvImgBufInfo[i]->va = mvBufHeapInfo[i]->va;
    }
  } else {  // non-BLOB image buffer created from BLOB heap.
    mvImgBufInfo[0]->pa = mvBufHeapInfo[0]->pa;
    mvImgBufInfo[0]->va = mvBufHeapInfo[0]->va;
    for (size_t i = 1; i < mvImgBufInfo.size(); ++i) {
      mvImgBufInfo[i]->pa =
          (0 == mvImgBufInfo[0]->pa) ? 0 : mvImgBufInfo[i]->pa;
      mvImgBufInfo[i]->va =
          (0 == mvImgBufInfo[0]->va) ? 0 : mvImgBufInfo[i]->va;
    }
  }
  //
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::lockBufLocked(char const* szCallerName, MINT usage) {
  MY_LOGD_IF(0 < mLockCount, "%s@ Has locked - LockCount:%d", szCallerName,
             mLockCount);
  //
  if (!mspImgBufHeap->lockBuf(szCallerName, usage)) {
    MY_LOGE("%s@ impLockBuf() usage:%#x", szCallerName, usage);
    return MFALSE;
  }
  //
  //  Check Buffer Info.
  if (mspImgBufHeap->getPlaneCount() != mvBufHeapInfo.size()) {
    MY_LOGE("%s@ BufInfo.size(%zu) != PlaneCount(%zu)", szCallerName,
            mvBufHeapInfo.size(), mspImgBufHeap->getPlaneCount());
    return MFALSE;
  }
  //
  for (size_t i = 0; i < mvBufHeapInfo.size(); i++) {
    mvBufHeapInfo[i]->va = (0 != (usage & mcam::eBUFFER_USAGE_SW_MASK))
                               ? mspImgBufHeap->getBufVA(i)
                               : 0;
    // mvBufHeapInfo[i]->pa = (0 != (usage & mcam::eBUFFER_USAGE_HW_MASK))
    //                            ? mspImgBufHeap->getBufPA(i)
    //                            : 0;
    //
    if (0 != (usage & mcam::eBUFFER_USAGE_SW_MASK) &&
        0 == mvBufHeapInfo[i]->va) {
      MY_LOGE("%s@ Bad result at %zu-th plane: va=0 with SW usage:%#x",
              szCallerName, i, usage);
      return MFALSE;
    }
    //
    // if (IIonDeviceProvider::get()->queryLegacyIon() &&
    //     0 != (usage & mcam::eBUFFER_USAGE_HW_MASK) &&
    //     0 == mvBufHeapInfo[i]->pa) {
    //   MY_LOGE("%s@ Bad result at %zu-th plane: pa=0 with HW usage:%#x",
    //           szCallerName, i, usage);
    //   return MFALSE;
    // }
  }
  //
  mLockUsage = usage;
  mLockCount++;
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::unlockBufLocked(char const* szCallerName) {
  if (0 == mLockCount) {
    MY_LOGW("%s@ Never lock", szCallerName);
    return MFALSE;
  }
  //
  if (!mspImgBufHeap->unlockBuf(szCallerName)) {
    MY_LOGE("%s@ impUnlockBuf() usage:%#x", szCallerName, mLockUsage);
    return MFALSE;
  }
  for (size_t i = 0; i < mvBufHeapInfo.size(); i++) {
    mvBufHeapInfo[i]->va = 0;
    mvBufHeapInfo[i]->pa = 0;
  }
  //
  mLockUsage = 0;
  mLockCount--;
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
std::unique_ptr<char[], std::function<void(char[])>>
BaseImageBuffer::toWorldSharedMemory(uint32_t secureHandle, size_t size) {
  // check if M-TEE is supported or not
#ifdef __LP64__
  const std::string UREE_LIBRARY_PATH("/vendor/lib64/libgz_uree.so");
#else
  const std::string UREE_LIBRARY_PATH("/vendor/lib/libgz_uree.so");
#endif
  // load library
  // NOTE: We esteem this library to be a plug-in.
  //       EL2 is not supported with the absence of this library.
  void* handle = dlopen(UREE_LIBRARY_PATH.c_str(), RTLD_LAZY);
  if (!handle) {
    MY_LOGW("cannot load %s(%s)", UREE_LIBRARY_PATH.c_str(), dlerror());
    std::unique_ptr<char[], std::function<void(char[])>> wsm;
    return wsm;
  }

  auto closeHandle = [](void* handle) {
    // close library
    if (handle) {
      dlclose(handle);
    }
    return nullptr;
  };

  // reset error
  dlerror();

  // load symbols
  bool isSymbolsLoaded = true;
  auto loadSymbol = [&](const char* symbolName = "") {
    void* symbolAddr = dlsym(handle, symbolName);
    if (!symbolAddr) {
      isSymbolsLoaded = false;
      MY_LOGE("%s", dlerror());
    }
    return symbolAddr;
  };

  const std::string MEM_SRV_NAME("com.mediatek.geniezone.srv.mem");
  typedef TZ_RESULT CreateSession_t(const char*, UREE_SESSION_HANDLE*);
  const auto createSession =
      reinterpret_cast<CreateSession_t*>(loadSymbol("UREE_CreateSession"));

  typedef TZ_RESULT RegisterSharedmem_t(
      UREE_SESSION_HANDLE, UREE_SHAREDMEM_HANDLE*, UREE_SHAREDMEM_PARAM*);
  const auto registerSharedMem = reinterpret_cast<RegisterSharedmem_t*>(
      loadSymbol("UREE_RegisterSharedmem"));

  typedef TZ_RESULT ION_CP_Chm2Shm_t(UREE_SESSION_HANDLE, UREE_SHAREDMEM_HANDLE,
                                     UREE_ION_HANDLE, uint32_t);
  const auto secureHandleToWSM =
      reinterpret_cast<ION_CP_Chm2Shm_t*>(loadSymbol("UREE_ION_CP_Chm2Shm"));

  typedef TZ_RESULT UnregisterSharedmem_t(UREE_SESSION_HANDLE,
                                          UREE_SHAREDMEM_HANDLE);
  const auto unregisterSharedMem = reinterpret_cast<UnregisterSharedmem_t*>(
      loadSymbol("UREE_UnregisterSharedmem"));

  typedef TZ_RESULT CloseSession_t(UREE_SESSION_HANDLE);
  auto closeSession =
      reinterpret_cast<CloseSession_t*>(loadSymbol("UREE_CloseSession"));

  if (!isSymbolsLoaded) {
    MY_LOGE("%s", dlerror());
    handle = closeHandle(handle);
    std::unique_ptr<char[], std::function<void(char[])>> wsm;
    return wsm;
  }

  // allocate a World Shared Memory (WSM) accessible among EL0, EL1 and EL2
  // NOTE: WSM for EL2 must be aligned to memory page
  auto memptr = reinterpret_cast<char*>(memalign(sysconf(_SC_PAGESIZE), size));
  MY_LOGD("secureHandle(%#x): WSM(%#" PRIxPTR ") size(%zu) allocated",
          secureHandle, reinterpret_cast<uintptr_t>(memptr), size);
  std::unique_ptr<char[], std::function<void(char[])>> wsm(
      memptr, [this](char memory[]) {
        free(memory);
        MY_LOGD("WSM(%#" PRIxPTR ") released",
                reinterpret_cast<uintptr_t>(memory));
      });

  // create session for shared memory
  UREE_SESSION_HANDLE memSrvSession;
  TZ_RESULT ret = (*createSession)(MEM_SRV_NAME.c_str(), &memSrvSession);
  MY_LOGE_IF(ret != TZ_RESULT_SUCCESS, "create mem sesion failed(%#x)", ret);

  // register WSM to EL2
#if defined(__LP64__)
  constexpr auto MAX_VALUE = std::numeric_limits<uint32_t>::max();
#endif

  UREE_SHAREDMEM_HANDLE wsmHandle;
  UREE_SHAREDMEM_PARAM wsmParam {
    .buffer = memptr,
#if defined(__LP64__)
    .size = (size <= MAX_VALUE) ? static_cast<uint32_t>(size) : MAX_VALUE
#else
    .size = size
#endif
  };

#if defined(__LP64__)
  MY_LOGE_IF(CC_UNLIKELY(size > MAX_VALUE),
             "buffer size(%zu) to be copied exceeds %u", size, MAX_VALUE);
#endif

  ret = (*registerSharedMem)(memSrvSession, &wsmHandle, &wsmParam);
  MY_LOGE_IF(ret != TZ_RESULT_SUCCESS, "register WSM to EL2 failed(%#x)", ret);

  // a deep copy of secure-to-shared memory
  // NOTE: This API is disabled by default on production load
  //       (i.e. Android user build) for security reason.
  ret = (*secureHandleToWSM)(memSrvSession, wsmHandle, secureHandle, size);
  if (ret != TZ_RESULT_SUCCESS) {
    MY_LOGW_IF(ret == static_cast<int>(TZ_RESULT_ERROR_NOT_SUPPORTED),
               "UREE_ION_CP_Chm2Shm is not supported(%#x)", ret);
    MY_LOGE_IF(ret != static_cast<int>(TZ_RESULT_ERROR_NOT_SUPPORTED),
               "copy from secure(%u) to shared memory(%#" PRIxPTR
               ") failed(%#x)",
               secureHandle, reinterpret_cast<uintptr_t>(memptr), ret);
  }

  // unregister WMS from EL2
  ret = (*unregisterSharedMem)(memSrvSession, wsmHandle);
  MY_LOGE_IF(ret != TZ_RESULT_SUCCESS,
             "unregister shared memory from EL2 failed(%#x)", ret);

  // close session for shared memory
  ret = (*closeSession)(memSrvSession);
  MY_LOGE_IF(ret != TZ_RESULT_SUCCESS, "close mem sesion failed(%#x)", ret);

  // close library
  handle = closeHandle(handle);

  return wsm;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL BaseImageBuffer::openFile(int& fd,
                                android::sp<NSCam::IFileCache>& pCache,
                                MBOOL useCache,
                                char const* filepath) {
  MBOOL ret = MTRUE;
  if (useCache) {
    pCache = NSCam::IFileCache::open(filepath);
    if (!pCache.get()) {
      MY_LOGE("fail to open %s (IFileCache)", filepath);
      ret = MFALSE;
    }
  } else {
    fd = ::open(filepath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if (fd < 0) {
      MY_LOGE("fail to open %s: %s", filepath, ::strerror(errno));
      ret = MFALSE;
    }
  }
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/

MBOOL BaseImageBuffer::writeFile(int& fd,
                                 android::sp<NSCam::IFileCache>& pCache,
                                 MBOOL useCache,
                                 void* pBuf,
                                 size_t size,
                                 char const* filepath,
                                 size_t index) {
  MBOOL ret = MTRUE;
  if (useCache) {
    unsigned int cSize = (unsigned int)size;
    unsigned int nw = pCache->write(pBuf, cSize);
    if (nw != cSize) {
      MY_LOGE("fail to write %s, %zu-th plane (IFileCache)", filepath, index);
      ret = MFALSE;
    }
  } else {
    size_t nw = ::write(fd, pBuf, size);
    if (nw != size) {
      MY_LOGE("fail to write %s, %zu-th plane (err=%s)", filepath, index,
              ::strerror(errno));
      ret = MFALSE;
    }
  }
  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
std::vector<std::string>
BaseImageBuffer::genMultiFilePath(char const* src) {
  std::vector<std::string> vDst;

  MBOOL bUfo = mImgFormat == NSCam::eImgFmt_UFO_BAYER8
            || mImgFormat == NSCam::eImgFmt_UFO_BAYER10
            || mImgFormat == NSCam::eImgFmt_UFO_BAYER12
            || mImgFormat == NSCam::eImgFmt_UFO_BAYER14
            || mImgFormat == NSCam::eImgFmt_UFO_FG_BAYER8
            || mImgFormat == NSCam::eImgFmt_UFO_FG_BAYER10
            || mImgFormat == NSCam::eImgFmt_UFO_FG_BAYER12
            || mImgFormat == NSCam::eImgFmt_UFO_FG_BAYER14;

  MBOOL bWarp = mImgFormat == NSCam::eImgFmt_WARP_2PLANE;

  MBOOL bMultiPlanePacked = mImgFormat == NSCam::eImgFmt_MTK_YUV_P210
                         || mImgFormat == NSCam::eImgFmt_MTK_YVU_P210
                         || mImgFormat == NSCam::eImgFmt_MTK_YUV_P210_3PLANE
                         || mImgFormat == NSCam::eImgFmt_MTK_YUV_P010
                         || mImgFormat == NSCam::eImgFmt_MTK_YVU_P010
                         || mImgFormat == NSCam::eImgFmt_MTK_YUV_P010_3PLANE
                         || mImgFormat == NSCam::eImgFmt_MTK_YUV_P012
                         || mImgFormat == NSCam::eImgFmt_MTK_YVU_P012;

  if (bMultiPlanePacked) {
    for (size_t i = 0; i < getPlaneCount(); ++i) {
      size_t pos;

      std::string tmp = src;
      std::string token_old = "img3o";
      std::string token_new = (i == 0) ? "img3o"
                            : (i == 1) ? "img3bo"
                            : "img3co";
      pos = 0;
      if ((pos = tmp.find(token_old, pos)) != std::string::npos) {
        tmp.replace(pos, token_old.length(), token_new);
      }

      token_old = "-PW";  // matching to TuningUtil genFileName usrString
      token_new = (i == 0) ? "-yplane-PW"
                : (i == 1 && getPlaneCount() == 2) ? "-cplane-PW"
                : (i == 1) ? "-uplane-PW" : "-vplane-PW";
      pos = 0;
      if ((pos = tmp.find(token_old, pos)) != std::string::npos) {
        tmp.replace(pos, token_old.length(), token_new);
      }

      vDst.push_back(tmp);
    }
  } else if (bWarp) {
     for (size_t i = 0; i < getPlaneCount(); ++i) {
      size_t pos;

      std::string tmp = src;
      std::string token_old = "-PW";
      std::string token_new = (i == 0) ? "-mapx-PW" : "-mapy-PW";
      pos = 0;
      if ((pos = tmp.find(token_old, pos)) != std::string::npos) {
        tmp.replace(pos, token_old.length(), token_new);
      }

      vDst.push_back(tmp);
    }
  } else if (bUfo) {
    vDst.push_back(src);

    char tmp[512];
    ::snprintf(tmp, sizeof(tmp), "%s.ltbl", src);
    vDst.push_back(tmp);
    ::snprintf(tmp, sizeof(tmp), "%s.meta", src);
    vDst.push_back(tmp);
  } else {
    vDst.push_back(src);
  }

  // debug
  for (auto& dst : vDst) {
    MY_LOGD("Load from/Save to %s", dst.c_str());
  }

  return vDst;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::saveToFile(char const* filepath) {
  static MBOOL sFileCacheEn = (NSCam::Utils::Properties::property_get_int32(
                                   "vendor.debug.camera.imgBuf.enFC", 0) > 0);
  MBOOL ret = MFALSE;
  std::vector<int> fd;
  std::vector<android::sp<NSCam::IFileCache>> fileCache;

  MY_LOGD("save to %s, chche(%d)", filepath, sFileCacheEn);

  // generate multiple file paths based on mImgFormat
  std::vector<std::string> vFilePath = genMultiFilePath(filepath);

  fd.resize(vFilePath.size());
  std::fill(fd.begin(), fd.end(), -1);

  fileCache.resize(vFilePath.size());

  if (0 == (mLockUsage & mcam::eBUFFER_USAGE_SW_READ_MASK)) {
    MY_LOGE("mLockUsage(0x%x) can not read VA", mLockUsage);
    goto lbExit;
  }

  /* Open File */
  for (size_t i = 0; i < vFilePath.size(); ++i) {
    if (!openFile(fd[i], fileCache[i], sFileCacheEn, vFilePath[i].c_str())) {
      goto lbExit;
    }
  }

  /* Write File */
  if (vFilePath.size() == 1) {
    if (mSecType != NSCam::SecType::mem_normal) {
      // TODO(MTK) check if continuous allocated or not
      // NOTE: Secure buffers are consecutive virtual memory from the
      //       software viewpoint and they are mapped into a big,
      //       physically-contiguous memory blocks allocated by the
      //       Contiguous Memory Allocator (or CMA) in the Linux Kernel.
      //
      //       We allocate a block of secure buffer for multi-plane formats
      //       that can be addressed by offset.
      size_t size = 0;
      for (size_t i = 0; i < getPlaneCount(); i++)
        size += getBufSizeInBytes(i);

      auto memptr =
          toWorldSharedMemory(static_cast<uint32_t>(getBufVA(0)), size);
      if (!writeFile(fd[0], fileCache[0], sFileCacheEn, memptr.get(), size,
                     filepath, 0)) {
        MY_LOGE("Write Secure Buf failed!");
      }
    } else {
      for (size_t i = 0; i < getPlaneCount(); i++) {
        MUINT8* pBuf = reinterpret_cast<MUINT8*>(getBufVA(i));
        size_t size = getBufSizeInBytes(i);
        if (!writeFile(fd[0], fileCache[0], sFileCacheEn, pBuf, size, filepath,
                       i)) {
          break;
        }
      }
    }
  } else {
    for (size_t i = 0; i < getPlaneCount(); i++) {
      if (mSecType != NSCam::SecType::mem_normal) {
        size_t size = getBufSizeInBytes(i);
        auto memptr =
            toWorldSharedMemory(static_cast<uint32_t>(getBufVA(i)), size);
        if (!writeFile(fd[i], fileCache[i], sFileCacheEn, memptr.get(), size,
                       filepath, i)) {
          MY_LOGE("Write Secure Buf (UFO or packed format) failed!");
        }
        continue;
      }

      MUINT8* pBuf = reinterpret_cast<MUINT8*>(getBufVA(i));
      size_t size = getBufSizeInBytes(i);
      if (!writeFile(fd[i], fileCache[i], sFileCacheEn, pBuf, size, filepath,
                     i)) {
        MY_LOGE("Write (UFO or packed format) file failed!");
      }
    }
  }

  ret = MTRUE;

lbExit:
  for (size_t i = 0; i < fd.size(); ++i) {
    if (fd[i] >= 0)
      ::close(fd[i]);
  }

  return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::loadFromFile(char const* filepath) {
  // generate multiple file paths based on mImgFormat
  std::vector<std::string> vFilePath = genMultiFilePath(filepath);

  // if vFilePath > 1, use loadFromFiles
  if (vFilePath.size() > 1)
    return loadFromFiles(vFilePath);

  MBOOL ret = MFALSE;
  MBOOL isLocked = MFALSE;
  int fd = -1;
  int filesize = 0;
  //
  isLocked = lockBuf(filepath, mcam::eBUFFER_USAGE_SW_WRITE_OFTEN);
  if (!isLocked) {
    MY_LOGE("lockBuf fail");
    goto lbExit;
  }
  //
  MY_LOGD("load from %s", filepath);
  fd = ::open(filepath, O_RDONLY);
  if (fd < 0) {
    MY_LOGE("fail to open %s: %s", filepath, ::strerror(errno));
    goto lbExit;
  }
  //
  filesize = ::lseek(fd, 0, SEEK_END);
  ::lseek(fd, 0, SEEK_SET);
  //
  for (MUINT i = 0; i < getPlaneCount(); i++) {
    MUINT8* pBuf = reinterpret_cast<MUINT8*>(getBufVA(i));
    MUINT bytesToRead = getBufSizeInBytes(i);
    MUINT bytesRead = 0;
    int nr = 0, cnt = 0;
    while (0 < bytesToRead) {
      MY_LOGD(
          "read from %d-th plane, read count:%d, bytesToRead:%d, bytesRead:%d",
          i, cnt, bytesToRead, bytesRead);
      nr = ::read(fd, pBuf + bytesRead, bytesToRead - bytesRead);
      if (nr < 0) {
        MY_LOGE(
            "fail to read from %s, %d-th plane, read-count:%d, read-bytes:%d : "
            "%s",
            filepath, i, cnt, bytesRead, ::strerror(errno));
        goto lbExit;
      }
      bytesToRead -= nr;
      bytesRead += nr;
      cnt++;
    }
  }
  //
  ret = MTRUE;
lbExit:
  //
  if (fd >= 0) {
    ::close(fd);
  }
  //
  if (isLocked) {
    unlockBuf(filepath);
  }

  return ret;
}

/******************************************************************************
 * vecPlaneFilePath: each file represents data for each plane
 *     ex: vecPlaneFilePatch[0] for plane-0
 *
 ******************************************************************************/
MBOOL
BaseImageBuffer::loadFromFiles(
    const std::vector<std::string>& vecPlaneFilePath) {
  MINT32 planeCnt = getPlaneCount();
  if (planeCnt == 0 || planeCnt != vecPlaneFilePath.size()) {
    MY_LOGE("#plane(%d) != #PlaneFilePath(%zu)", planeCnt,
            vecPlaneFilePath.size());
    return MFALSE;
  }

  const char* lockUser = vecPlaneFilePath[0].c_str();
  MBOOL isLocked = lockBuf(lockUser, mcam::eBUFFER_USAGE_SW_WRITE_OFTEN);
  if (!isLocked) {
    MY_LOGE("lockBuf fail: user(%s)", (lockUser ? lockUser : "nullstr"));
    return MFALSE;
  }

  MBOOL ret = MTRUE;
  for (size_t i = 0; i < planeCnt; i++) {
    int fd = -1, filesize = 0;
    const char* filepath = vecPlaneFilePath[i].c_str();
    fd = ::open(filepath, O_RDONLY);
    if (fd < 0) {
      MY_LOGE("fail to open %s: %s", filepath, ::strerror(errno));
      ret = MFALSE;
      break;
    }
    //
    filesize = ::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
    //
    MUINT8* pBuf = reinterpret_cast<MUINT8*>(getBufVA(i));
    MUINT bytesRead = 0, totalBytesToRead = getBufSizeInBytes(i);
    int nr = 0, cnt = 0;
    while (ret && totalBytesToRead > bytesRead && bytesRead < filesize) {
      nr = ::read(fd, pBuf + bytesRead, totalBytesToRead - bytesRead);
      if (nr < 0) {
        MY_LOGE("readCnt(%d): fail to load from '%s'@plane-%d, errStr: %s", cnt,
                filepath, i, ::strerror(errno));
        ret = MFALSE;
        break;
      }
      bytesRead += nr;
      cnt++;
      MY_LOGD(
          "readCnt(%d): load from '%s'@plane-%d, filesize(%d), "
          "totalBytesToRead(%d), bytesRead(%d)",
          cnt, filepath, i, filesize, totalBytesToRead, bytesRead);
    }
    if (fd >= 0) {
      ::close(fd);
      fd = -1;
    }
  }  // for loop end
  if (isLocked)
    unlockBuf(lockUser);
  //
  return ret;
}

std::shared_ptr<mcam::IImageBuffer> BaseImageBuffer::clone() {
  return nullptr;
}

void BaseImageBuffer::dumpBuffer() {
  if (!mspImgBufHeap) {
    MY_LOGA("null mspImgBufHeap");
    return;
  }
  MINT fmt = mspImgBufHeap->getImgFormat();
  MY_LOGE("Name:%s-%s, size:%dx%d, fmt:%s, lock cnt:%d",
          mspImgBufHeap->getCallerName().c_str(),
          mspImgBufHeap->getBufName().c_str(),
          mspImgBufHeap->getImgSize().w,
          mspImgBufHeap->getImgSize().h,
          Utils::Format::queryImageFormatName(fmt).c_str(),
          mLockCount);
}
