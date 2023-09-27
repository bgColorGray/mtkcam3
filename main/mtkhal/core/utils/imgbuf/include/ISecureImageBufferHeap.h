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

#ifndef UTILS_IMGBUF_INCLUDE_ISECUREIMAGEBUFFERHEAP_H_
#define UTILS_IMGBUF_INCLUDE_ISECUREIMAGEBUFFERHEAP_H_

#include "LegacyImgParam.h"
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h>

#include <memory>
using namespace NSCam;
namespace mcam {
using mcam::IImageBuffer;
using mcam::IImageBufferHeap;
using NSCam::MSize;
using std::shared_ptr;

class ISecureImageBufferHeap : public virtual mcam::IImageBufferHeap {
 public:
  struct AllocExtraParam {
    MINT32 nocache;
    MINT32 security;
    MINT32 coherence;
    MBOOL contiguousPlanes;  // contiguous memory for multiple planes if MTRUE
    NSCam::SecType secType;

    AllocExtraParam(MINT32 _nocache = 0,
                    MINT32 _security = 0,
                    MINT32 _coherence = 0,
                    MBOOL _contiguousPlanes = MFALSE,
                    NSCam::SecType _type = NSCam::SecType::mem_secure)
        : nocache(_nocache),
          security(_security),
          coherence(_coherence),
          contiguousPlanes(_contiguousPlanes),
          secType(_type) {}
  };

  using AllocImgParam_t = mcam::LegacyImgParam;
  static std::shared_ptr<ISecureImageBufferHeap> createPtr(
      char const* szCallerName,
      char const* bufName,
      mcam::LegacyImgParam const& legacyParam,
      mcam::IImageBufferAllocator::ImgParam const& imgParam,
      MBOOL const enableLog = MTRUE);

  static char const* magicName() { return "SecHeap"; }

 protected:
  /**
   * Disallowed to directly delete a raw pointer publicly.
   */
  virtual ~ISecureImageBufferHeap() = default;
};  // class ISecureImageBufferHeap

}  // namespace NSCam

#endif  // UTILS_IMGBUF_INCLUDE_ISECUREIMAGEBUFFERHEAP_H_