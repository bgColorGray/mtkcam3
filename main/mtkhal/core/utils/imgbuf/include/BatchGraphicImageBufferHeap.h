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
 * MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef UTILS_IMGBUF_INCLUDE_BATCHGRAPHICIMAGEBUFFERHEAP_H_
#define UTILS_IMGBUF_INCLUDE_BATCHGRAPHICIMAGEBUFFERHEAP_H_

#include <graphics_mtk_defs.h>
#include <ui/gralloc_extra.h>
#include <vndk/hardware_buffer.h>

#include <memory>
#include <vector>

#include "BaseImageBufferHeap.h"

using mcam::IImageBuffer;
using NSCam::ImageBufferInfo;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;

/******************************************************************************
 *  Image Buffer Heap.
 ******************************************************************************/
namespace mcam {
class BatchGraphicImageBufferHeap : public BaseImageBufferHeap {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  mcam::IImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Accessors.
  virtual std::vector<std::shared_ptr<mcam::IImageBuffer>>
  createImageBuffers_FromBlobHeap(const ImageBufferInfo& info,
                                  const char* callerName,
                                  MBOOL dirty_content = MFALSE);
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  BaseImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  virtual char const* impGetMagicName() const {
    return "BatchGraphicImageBufferHeap";
  }
  virtual void* impGetMagicInstance() const {
    return const_cast<void*>(reinterpret_cast<const void*>(this));
  }
  virtual HeapInfoVect_t const& impGetHeapInfo() const {
    static HeapInfoVect_t heapInfo;
    return heapInfo;
  }
  virtual MBOOL impInit(BufInfoVect_t const& rvBufInfo __unused) {
    return MTRUE;
  }
  virtual MBOOL impUninit(BufInfoVect_t const& rvBufInfo __unused) {
    return MTRUE;
  }
  virtual MBOOL impReconfig(BufInfoVect_t const& rvBufInfo __unused) {
    return MTRUE;
  }

 public:  ////
  virtual MBOOL impLockBuf(char const* szCallerName,
                           MINT usage,
                           BufInfoVect_t const& rvBufInfo) {
    return true;
  }
  virtual MBOOL impUnlockBuf(char const* szCallerName,
                             MINT usage,
                             BufInfoVect_t const& rvBufInfo) {
    return true;
  }
  virtual void* getHWBuffer() const { return nullptr; }

 public:  ////                    Destructor/Constructors.
  /**
   * Disallowed to directly delete a raw pointer.
   */
  virtual ~BatchGraphicImageBufferHeap() = default;
  BatchGraphicImageBufferHeap(
      char const* szCallerName,
      std::vector<std::shared_ptr<mcam::IImageBufferHeap>>& vHeap);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  std::vector<std::shared_ptr<mcam::IImageBufferHeap>> mvHeap;
};

};  // namespace NSCam

#endif  // UTILS_IMGBUF_INCLUDE_BATCHGRAPHICIMAGEBUFFERHEAP_H_
