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
#include <mtkcam/utils/std/common.h>

#include <ion/ion.h>
#include <sys/ioctl.h>
#include <atomic>

#include <memory>

//#include "include/IDummyImageBufferHeap.h"
//#include "include/IGrallocImageBufferHeap.h"
#include "include/IIonImageBufferHeap.h"
//#include "include/ISecureImageBufferHeap.h"
#include "include/ImageBufferHeapFactoryImpl.h"

// using namespace NSCam;
using NSCam::Utils::Format::queryPlaneWidthInPixels;
using NSCam::Utils::Format::queryPlaneBitsPerPixel;
using std::shared_ptr;
using mcam::ImageBufferHeapFactoryImpl;
//using NSCam::IGrallocImageBufferHeap;
//using NSCam::ISecureImageBufferHeap;
using mcam::IIonImageBufferHeap;
using mcam::ImageBufferHeapFactoryImpl;
//using NSCam::IDummyImageBufferHeap;

#define ALIGN(x, alignment) (((x) + ((alignment)-1)) & ~((alignment)-1))

shared_ptr<mcam::IImageBufferHeap> ImageBufferHeapFactoryImpl::create(
    char const* szCallerName,
    char const* bufName,
    ImgParam const& imgParam,
    bool enableLog) {
  mcam::LegacyImgParam legacyParam = convert2LegacyParam(imgParam);
  MUINT64 usage = imgParam.usage;

//  if ((usage & mcam::eBUFFER_USAGE_HW_TEXTURE) |
//      (usage & mcam::eBUFFER_USAGE_HW_RENDER))
//    return IGrallocImageBufferHeap::createPtr(szCallerName, bufName,
//                                              legacyParam, imgParam, enableLog);
//
//  if (imgParam.secType != SecType::mem_normal)
//    return ISecureImageBufferHeap::createPtr(szCallerName, bufName, legacyParam,
//                                             imgParam, enableLog);

  return IIonImageBufferHeap::createPtr(szCallerName, bufName, legacyParam,
                                        imgParam, enableLog);
}

//shared_ptr<mcam::IImageBufferHeap>
//ImageBufferHeapFactoryImpl::createDummyFromBlobHeap(
//    char const* szCallerName,
//    char const* bufName,
//    shared_ptr<mcam::IImageBufferHeap> blobHeap,
//    size_t offsetInByte,
//    ImgParam const& imgParam,
//    bool enableLog) {
//  mcam::LegacyImgParam legacyParam = convert2LegacyParam(imgParam);
//
//  return IDummyImageBufferHeap::createPtr(szCallerName, bufName, blobHeap,
//                                          offsetInByte, legacyParam, imgParam,
//                                          enableLog);
//}
//
//shared_ptr<mcam::IImageBufferHeap>
//ImageBufferHeapFactoryImpl::createDummyFromBlobHeap(char const* szCallerName,
//                                                    char const* bufName,
//                                                    int bufFd,
//                                                    size_t offsetInByte,
//                                                    ImgParam const& imgParam,
//                                                    bool enableLog) {
//  mcam::LegacyImgParam legacyParam = convert2LegacyParam(imgParam);
//
//  return IDummyImageBufferHeap::createPtr(szCallerName, bufName, bufFd,
//                                          offsetInByte, legacyParam, imgParam,
//                                          enableLog);
//}

mcam::LegacyImgParam ImageBufferHeapFactoryImpl::convert2LegacyParam(
    mcam::IImageBufferAllocator::ImgParam const& imgParam) {
  MUINT32 format = imgParam.format;
  size_t planeCount = NSCam::Utils::Format::queryPlaneCount(format);

  auto isValid = [](const size_t val[]) -> bool {
    size_t sum = 0;
    for (int i = 0; i < 3; i++)
      sum += val[i];
    return (sum > 0);
  };

  // 1. handle BLOB or JPEG
  if (imgParam.format == NSCam::eImgFmt_BLOB)
    return mcam::LegacyImgParam(imgParam.size.w, 0);

  if (imgParam.format == NSCam::eImgFmt_JPEG) {
    mcam::LegacyImgParam param(imgParam.size, imgParam.bufSizeInByte[0], 0);
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
    return mcam::LegacyImgParam(format, imgParam.size, strides, boundary,
                                 imgParam.bufSizeInByte, planeCount);
  else
    return mcam::LegacyImgParam(format, imgParam.size, strides, boundary,
                                 planeCount);
}

std::shared_ptr<mcam::IImageBuffer> mcam::IImageBufferAllocator::alloc(
    char const* clientName,
    char const* bufName,
    ImgParam const& imgParam,
    bool enableLog) {
  /* TODO(Elon Hsu) concat clientName & bufName */
  shared_ptr<mcam::IImageBufferHeap> heap = ImageBufferHeapFactoryImpl::create(
      clientName, bufName, imgParam, enableLog);
  return heap->createImageBuffer();
}

std::shared_ptr<mcam::IImageBufferHeap> mcam::IImageBufferHeapFactory::create(
    char const* clientName,
    char const* bufName,
    mcam::IImageBufferAllocator::ImgParam const& imgParam,
    bool enableLog) {
  /* TODO(Elon Hsu) concat clientName & bufName */
  return ImageBufferHeapFactoryImpl::create(clientName, bufName, imgParam,
                                            enableLog);
}

//std::shared_ptr<mcam::IImageBufferHeap>
//mcam::IImageBufferHeapFactory::createDummyFromBlobHeap(
//    char const* clientName,
//    char const* bufName,
//    std::shared_ptr<mcam::IImageBufferHeap> blobHeap,
//    size_t offsetInByte,
//    mcam::IImageBufferAllocator::ImgParam const& imgParam,
//    bool enableLog) {
//  /* TODO(Elon Hsu) concat clientName & bufName */
//  return ImageBufferHeapFactoryImpl::createDummyFromBlobHeap(
//      clientName, bufName, blobHeap, offsetInByte, imgParam, enableLog);
//}
//
//std::shared_ptr<mcam::IImageBufferHeap>
//mcam::IImageBufferHeapFactory::createDummyFromBufferFD(
//    char const* clientName,
//    char const* bufName,
//    int bufFd,
//    size_t offsetInByte,
//    mcam::IImageBufferAllocator::ImgParam const& imgParam,
//    bool enableLog) {
//  /* TODO(Elon Hsu) concat clientName & bufName */
//  return ImageBufferHeapFactoryImpl::createDummyFromBlobHeap(
//      clientName, bufName, bufFd, offsetInByte, imgParam, enableLog);
//}
