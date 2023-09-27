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

#ifndef UTILS_IMGBUF_INCLUDE_IMAGEBUFFERHEAPFACTORYIMPL_H_
#define UTILS_IMGBUF_INCLUDE_IMAGEBUFFERHEAPFACTORYIMPL_H_

#include "LegacyImgParam.h"
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h>

#include <memory>
using namespace NSCam;
namespace mcam {

class ImageBufferHeapFactoryImpl {
 public:
  typedef mcam::IImageBufferAllocator::ImgParam ImgParam;

  static std::shared_ptr<mcam::IImageBufferHeap> create(
      char const* szCallerName,
      char const* bufName,
      ImgParam const& imgParam,
      bool enableLog = false);

//  static std::shared_ptr<mcam::IImageBufferHeap> createDummyFromBlobHeap(
//      char const* szCallerName,
//      char const* bufName,
//      std::shared_ptr<mcam::IImageBufferHeap> blobHeap,
//      size_t offsetInByte,
//      ImgParam const& imgParam,
//      bool enableLog = false);
//
//  static std::shared_ptr<mcam::IImageBufferHeap> createDummyFromBlobHeap(
//      char const* szCallerName,
//      char const* bufName,
//      int bufFd,
//      size_t offsetInByte,
//      ImgParam const& imgParam,
//      bool enableLog = false);

  const char* getMagicName() { return "HeapFactory"; }

 private:
  static mcam::LegacyImgParam convert2LegacyParam(
      mcam::IImageBufferAllocator::ImgParam const& imgParam);
};

};  // namespace NSCam

#endif  // UTILS_IMGBUF_INCLUDE_IMAGEBUFFERHEAPFACTORYIMPL_H_
//
