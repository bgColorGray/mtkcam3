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

#define LOG_TAG "mtkcam-ConvertImage"

#include <mtkcam3/main/mtkhal/core/utils/imgbuf/ConvertImageBuffer.h>
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>                   // NSCam
#include <mtkcam/utils/debug/debug.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam/utils/std/ULog.h>
//
#include "include/MyUtils.h"

using NSCam::Utils::Format::queryPlaneWidthInPixels;
using NSCam::Utils::Format::queryPlaneBitsPerPixel;

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);

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

#define ALIGN(x, alignment) (((x) + ((alignment)-1)) & ~((alignment)-1))

namespace mcam {

static auto convertImageParam(mcam::IImageBufferAllocator::ImgParam const& imgParam, size_t planeCount)
    -> NSCam::IImageBufferAllocator::ImgParam {
  MUINT32 format = imgParam.format;

  auto isValid = [](const size_t val[]) -> bool {
    size_t sum = 0;
    for (int i = 0; i < 3; i++)
      sum += val[i];
    return (sum > 0);
  };

  // 1. handle BLOB or JPEG
  if (imgParam.format == NSCam::eImgFmt_BLOB)
    return NSCam::IImageBufferAllocator::ImgParam(imgParam.size.w, 0);

  if (imgParam.format == NSCam::eImgFmt_JPEG) {
    NSCam::IImageBufferAllocator::ImgParam param(imgParam.size, imgParam.bufSizeInByte[0], 0);
    param.bufCustomSizeInBytes[0] = imgParam.bufSizeInByte[0];
    param.imgSize = imgParam.size;  // we should get (w, h) for JPEG getImgSize
    return param;
  }

  // 2. handle other formats
  // 2.1 calculate stride (in byte)
  size_t strides[3] = {0};
  if (isValid(imgParam.strideInByte)) {
    ::memcpy(strides, imgParam.strideInByte, sizeof(strides));
  } else {
    size_t w_align = (imgParam.strideAlignInPixel)
                         ? ALIGN(imgParam.size.w, imgParam.strideAlignInPixel)
                         : imgParam.size.w;

    for (int i = 0; i < planeCount; i++) {
      strides[i] = queryPlaneWidthInPixels(format, i, w_align);

      // cal into byte
      strides[i] = (strides[i] * queryPlaneBitsPerPixel(format, i) + 7) / 8;

      // byte align stride
      if (imgParam.strideAlignInByte)
        strides[i] = ALIGN(strides[i], imgParam.strideAlignInByte);
    }
  }

  size_t boundary[3] = {0};  // this is phased out field

  if (isValid(imgParam.bufSizeInByte))
    return NSCam::IImageBufferAllocator::ImgParam(format, imgParam.size, strides, boundary,
                                                  imgParam.bufSizeInByte, planeCount);
  else
    return NSCam::IImageBufferAllocator::ImgParam(format, imgParam.size, strides, boundary,
                                                  planeCount);
}

auto convertImageBufferHeap(std::shared_ptr<mcam::IImageBufferHeap> pMcamHeap)
    -> ::android::sp<NSCam::IImageBufferHeap> {
  ::android::sp<NSCam::IImageBufferHeap> pHeap = nullptr;

  auto pGraphicImageBufferHeap = mcam::IGraphicImageBufferHeap::castFrom(pMcamHeap.get());
  if (pGraphicImageBufferHeap != nullptr) { // IGraphic
    pHeap = NSCam::IGraphicImageBufferHeap::create(
            pGraphicImageBufferHeap->getCallerName().c_str(), // new
            pGraphicImageBufferHeap->getGrallocUsage(),
            pGraphicImageBufferHeap->getImgSize(),
            pGraphicImageBufferHeap->getImgFormat(),
            pGraphicImageBufferHeap->getBufferHandlePtr(),
            pGraphicImageBufferHeap->getAcquireFence(),
            pGraphicImageBufferHeap->getReleaseFence(),
            pGraphicImageBufferHeap->getStride(),             // new
            pGraphicImageBufferHeap->getSecType());

    pHeap->setColorArrangement(pGraphicImageBufferHeap->getColorArrangement());
  } else {  // Ion
    pMcamHeap->lockBuf(LOG_TAG);
    MY_LOGD("create IonHeap+, planeCount %zu, heapCount %zu", pMcamHeap->getPlaneCount(), pMcamHeap->getHeapIDCount());
    mcam::IImageBufferAllocator::ImgParam mcamImgParam(pMcamHeap->getImgFormat(), pMcamHeap->getImgSize(), 0);
    NSCam::IImageBufferAllocator::ImgParam imgParam = convertImageParam(mcamImgParam, pMcamHeap->getPlaneCount());
    NSCam::IIonImageBufferHeap::AllocExtraParam extraParam;
    extraParam.vIonFd.resize(pMcamHeap->getHeapIDCount());
    for (size_t i = 0 ; i < pMcamHeap->getHeapIDCount() ; i++) {
      auto& ionFd = extraParam.vIonFd[i];
      if (i > 0 && pMcamHeap->getHeapID(i) == pMcamHeap->getHeapID(i-1)) {
        extraParam.contiguousPlanes = MTRUE;
        ionFd = extraParam.vIonFd[i-1];
      } else {
        ionFd = ::dup(pMcamHeap->getHeapID(i));
      }
      MY_LOGD("dup fd(%zu): %d -> %d", i, pMcamHeap->getHeapID(i), ionFd);
    }
    pMcamHeap->unlockBuf(LOG_TAG);
    pHeap = ::android::sp<NSCam::IIonImageBufferHeap>(
              NSCam::IIonImageBufferHeap::create("mtkcam_hal/custom", imgParam, extraParam, MTRUE));
    MY_LOGD("create IonHeap-, pHeap %p", pHeap.get());
#if 0
    pMcamHeap->lockBuf(LOG_TAG);
    pHeap->lockBuf(LOG_TAG);
    MY_LOGD("format %d %d", pMcamHeap->getImgFormat(), pHeap->getImgFormat());
    MY_LOGD("ImgSize (%dx%d) (%dx%d)", pMcamHeap->getImgSize().w, pMcamHeap->getImgSize().h, pHeap->getImgSize().w, pHeap->getImgSize().h);
    MY_LOGD("ImgBitsPerPixel %zu %zu", pMcamHeap->getImgBitsPerPixel(), pHeap->getImgBitsPerPixel());
    MY_LOGD("BitstreamSize %zu %zu", pMcamHeap->getBitstreamSize(), pHeap->getBitstreamSize());
    MY_LOGD("planeCount %zu %zu", pMcamHeap->getPlaneCount(), pHeap->getPlaneCount());
    MY_LOGD("HeapIDCount %zu %zu", pMcamHeap->getHeapIDCount(), pHeap->getHeapIDCount());
    for (size_t i = 0 ; i < pMcamHeap->getHeapIDCount() ; i++) {
      MY_LOGD("plane:%d heapId %d %d", i, pMcamHeap->getHeapID(i), pHeap->getHeapID(i));
    }
    for (size_t i = 0 ; i < pMcamHeap->getPlaneCount() ; i++) {
      //MY_LOGD("plane:%d (pa/va) (%#" PRIxPTR "/%#" PRIxPTR ") (%#" PRIxPTR "/%#" PRIxPTR ")", i, pMcamHeap->getBufPA(i), pMcamHeap->getBufVA(i), pHeap->getBufPA(i), pMcamHeap->getBufVA(i));
      MY_LOGD("plane:%d (pa/va) (%#" PRIxPTR ") (%#" PRIxPTR "/%#" PRIxPTR ")", i, pMcamHeap->getBufVA(i), pHeap->getBufPA(i), pMcamHeap->getBufVA(i));
    }

    for (size_t i = 0 ; i < pMcamHeap->getPlaneCount() ; i++) {
      MY_LOGD("plane:%d getBufSizeInBytes %zu %zu", i, pMcamHeap->getBufSizeInBytes(i), pHeap->getBufSizeInBytes(i));
      MY_LOGD("plane:%d getBufStridesInBytes %zu %zu", i, pMcamHeap->getBufStridesInBytes(i), pHeap->getBufStridesInBytes(i));
      MY_LOGD("plane:%d getBufCustomSizeInBytes %zu %zu", i, pMcamHeap->getBufCustomSizeInBytes(i), pHeap->getBufCustomSizeInBytes(i));
    }

    pMcamHeap->unlockBuf(LOG_TAG);
    pHeap->unlockBuf(LOG_TAG);
#endif
  }
  return pHeap;
}
};  // namespace mcam