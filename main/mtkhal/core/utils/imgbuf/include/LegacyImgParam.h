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

#ifndef UTILS_IMGBUF_INCLUDE_LEGACYIMGPARAM_H_
#define UTILS_IMGBUF_INCLUDE_LEGACYIMGPARAM_H_

#include <mtkcam/def/common.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h>
using namespace NSCam;
// TODO(MTK) legacyParam is temporarily, we would remove it
// after we re-factor the implementation of imagebufferheap
namespace mcam {
struct LegacyImgParam {
  MINT32 imgFormat;              // image format.
  MSize imgSize;                 // image resolution in pixels.
  MSize jpgSize;                 // jpeg resolution in pixels. for JPEG memory.
  size_t bufSize;                // buffer size in bytes. for JPEG memory.
  size_t bufStridesInBytes[3];   // buffer strides(width) in bytes.
  size_t bufBoundaryInBytes[3];  // the address will be a multiple of boundary
                                 // in bytes, which must be a power of two.
  size_t bufCustomSizeInBytes[3];  // the buffer size that caller wants to
                                   // specifiy manually, usually use for
                                   // vertical padding, can be 0.
  size_t
      bufReusableSizeInBytes[3];  // the buffer size that caller wants to reuse,
                                  // usually use for vertical padding, can be 0.
  //
  //  Image (width strides in bytes and minimum allocate size in byte).
  //  All T1 ,T2, T3 and T4 are expected to size_t.
  template <typename T1, typename T2, typename T3, typename T4>
  LegacyImgParam(MINT32 const _imgFormat,
                 MSize const& _imgSize,
                 T1 const _bufStridesInBytes[],
                 T2 const _bufBoundaryInBytes[],
                 T3 const _bufCustomSizeInBytes[],
                 T4 const _bufReusableSizeInBytes[],
                 size_t const _planeCount)
      : imgFormat(_imgFormat),
        imgSize(_imgSize),
        jpgSize(_imgSize),
        bufSize(0) {
    for (size_t i = 0; i < _planeCount; i++) {
      bufStridesInBytes[i] = _bufStridesInBytes[i];
      bufBoundaryInBytes[i] = _bufBoundaryInBytes[i];
      bufCustomSizeInBytes[i] = _bufCustomSizeInBytes[i];
      bufReusableSizeInBytes[i] = _bufReusableSizeInBytes[i];
    }
  }
  //  Image (width strides in bytes and minimum allocate size in byte).
  //  All T1 ,T2 and T3 are expected to size_t.
  template <typename T1, typename T2, typename T3>
  LegacyImgParam(MINT32 const _imgFormat,
                 MSize const& _imgSize,
                 T1 const _bufStridesInBytes[],
                 T2 const _bufBoundaryInBytes[],
                 T3 const _bufCustomSizeInBytes[],
                 size_t const _planeCount)
      : imgFormat(_imgFormat),
        imgSize(_imgSize),
        jpgSize(_imgSize),
        bufSize(0) {
    for (size_t i = 0; i < _planeCount; i++) {
      bufStridesInBytes[i] = _bufStridesInBytes[i];
      bufBoundaryInBytes[i] = _bufBoundaryInBytes[i];
      bufCustomSizeInBytes[i] = _bufCustomSizeInBytes[i];
      bufReusableSizeInBytes[i] = 0;
    }
  }
  //
  //  Image (width strides in bytes).
  //  Both T1 and T2 are expected to size_t.
  template <typename T1, typename T2>
  LegacyImgParam(MINT32 const _imgFormat,
                 MSize const& _imgSize,
                 T1 const _bufStridesInBytes[],
                 T2 const _bufBoundaryInBytes[],
                 size_t const _planeCount)
      : imgFormat(_imgFormat),
        imgSize(_imgSize),
        jpgSize(_imgSize),
        bufSize(0) {
    for (size_t i = 0; i < _planeCount; i++) {
      bufStridesInBytes[i] = _bufStridesInBytes[i];
      bufBoundaryInBytes[i] = _bufBoundaryInBytes[i];
      bufCustomSizeInBytes[i] = 0;
      bufReusableSizeInBytes[i] = 0;
    }
  }
  //
  //  BLOB memory.
  LegacyImgParam(
      size_t const _bufSize,  // buffer size in bytes.
      size_t const
          _bufBoundaryInBytes  // the address will be a multiple of boundary in
                               // bytes, which must be a power of two.
      )
      : imgFormat(eImgFmt_BLOB),
        imgSize(MSize(_bufSize, 1)),
        jpgSize(MSize(_bufSize, 1)),
        bufSize(_bufSize) {
    bufStridesInBytes[0] = _bufSize;
    bufBoundaryInBytes[0] = _bufBoundaryInBytes;
    bufCustomSizeInBytes[0] = 0;
    bufReusableSizeInBytes[0] = 0;
  }
  //
  //  JPEG memory.
  LegacyImgParam(
      MSize const& _imgSize,  // image resolution in pixels.
      size_t const _bufSize,  // buffer size in bytes.
      size_t const
          _bufBoundaryInBytes  // the address will be a multiple of boundary in
                               // bytes, which must be a power of two.
      )
      : imgFormat(eImgFmt_JPEG),
        imgSize(MSize(_bufSize, 1)),
        jpgSize(_imgSize),
        bufSize(_bufSize) {
    bufStridesInBytes[0] = _bufSize;
    bufBoundaryInBytes[0] = _bufBoundaryInBytes;
    bufCustomSizeInBytes[0] = 0;
    bufReusableSizeInBytes[0] = 0;
  }
};

struct LegacyExtraParam {
  MINT32 usage;
  MINT32 nocache;
  //
  LegacyExtraParam(MINT32 _usage = mcam::eBUFFER_USAGE_SW_MASK |
                                   mcam::eBUFFER_USAGE_HW_MASK,
                   MINT32 _nocache = 0)
      : usage(_usage), nocache(_nocache) {}
};
};  // namespace NSCam

#endif  // UTILS_IMGBUF_INCLUDE_LEGACYIMGPARAM_H_
