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

#ifndef INCLUDE_MTKCAM_ANDROID_UTILS_IMGBUF_IGRAPHICIMAGEBUFFERHEAP_H_
#define INCLUDE_MTKCAM_ANDROID_UTILS_IMGBUF_IGRAPHICIMAGEBUFFERHEAP_H_
//
#include <mtkcam3/main/mtkhal/android/device/3.x/types.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IIonDevice.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h>
//
#include <system/window.h>

#include <memory>
#include <vector>
using namespace NSCam;
using mcam::android::AppBufferHandleHolder;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
using mcam::IImageBuffer;
using mcam::IImageBufferHeap;
using NSCam::MSize;
using std::shared_ptr;

/******************************************************************************
 *  Image Buffer Heap.
 ******************************************************************************/
class IGraphicImageBufferHeap : public virtual mcam::IImageBufferHeap {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Creation.
  static std::shared_ptr<IGraphicImageBufferHeap> create(
      char const* szCallerName,
      ANativeWindowBuffer* pANWBuffer,
      MINT const acquire_fence = -1,
      MINT const release_fence = -1,
      bool keepOwnership = false,
      NSCam::SecType secType = NSCam::SecType::mem_normal,
      std::shared_ptr<IIonDevice> ionDevice = nullptr);
          // An ion device will be created internally if it's nullptr.

  static std::shared_ptr<IGraphicImageBufferHeap> create(
      char const* szCallerName,
      uint32_t width,
      uint32_t height,
      uint32_t usage,
      buffer_handle_t* buffer,
      MINT const acquire_fence = -1,
      MINT const release_fence = -1,
      bool keepOwnership = false,
      NSCam::SecType secType = NSCam::SecType::mem_normal,
      std::shared_ptr<IIonDevice> ionDevice = nullptr);
          // An ion device will be created internally if it's nullptr.

  /**
   * Given a graphic buffer handle, create a instance of
   * IGraphicImageBufferHeap.
   *
   * @param szCallerName  The caller's name
   * @param usage         The usage for the allocation
   * @param imgSize       The image resolution, in pixels.
   * @param format        The image format, used on getImgFormat().
   *                      It could be eImgFmt_JPEG even if
   *                      the format from the given 'buffer' is BLOB.
   * @param buffer        The given graphic buffer handle.
   * @param acquire_fence The acquire fence.
   * @param release_fence The release fence.
   * @param stride        set by user defined
   * @return nullptr if it failed on this call and no heap can be created.
   *         non nullptr if a heap is created successfully.
   */
  static std::shared_ptr<IGraphicImageBufferHeap> create(
      char const* szCallerName,
      uint32_t usage,
      MSize imgSize,
      int format,
      buffer_handle_t* buffer,
      MINT const acquire_fence = -1,
      MINT const release_fence = -1,
      int stride = 0,
      NSCam::SecType secType = NSCam::SecType::mem_normal,
      std::shared_ptr<IIonDevice> ionDevice = nullptr,
      std::shared_ptr<AppBufferHandleHolder> handleHolder = nullptr);
          // An ion device will be created internally if it's nullptr.

  /**
   * Given a blob graphic buffer handle, create an IGraphicImageBufferHeap.
   *
   * @param szCallerName  The caller's name
   * @param param         The given image parameter
   * @param buffer        The given graphic buffer handle.
   * @param acquire_fence The acquire fence.
   * @param release_fence The release fence.
   * @return nullptr if it failed on this call and no heap can be created.
   *         non nullptr if a heap is created successfully.
   */
  static std::shared_ptr<IGraphicImageBufferHeap> createFromBlob(
      char const* szCallerName,
      mcam::IImageBufferAllocator::ImgParam param,
      buffer_handle_t* buffer,
      MINT const acquire_fence = -1,
      MINT const release_fence = -1,
      std::shared_ptr<IIonDevice> ionDevice = nullptr,
      std::shared_ptr<AppBufferHandleHolder> handleHolder = nullptr);
          // An ion device will be created internally if it's nullptr.

  /**
   * Given multiple IGraphicImageBufferHeap, create a instance of
   * IGraphicImageBufferHeap.
   *
   * @param szCallerName  The caller's name
   * @param vHeap         multiple IGraphicImageBufferHeap
   * @return nullptr if it failed on this call and no heap can be created.
   *         non nullptr if a heap is created successfully.
   */
  static std::shared_ptr<IGraphicImageBufferHeap> create(
      char const* szCallerName,
      std::vector<std::shared_ptr<mcam::IImageBufferHeap>>& vHeap);

 protected:  ////                    Destructor/Constructors.
  /**
   * Disallowed to directly delete a raw pointer.
   */
  virtual ~IGraphicImageBufferHeap() {}

 public:  ////                    Attributes.
  static char const* magicName() { return "IGraphicImageBufferHeap"; }

  static IGraphicImageBufferHeap* castFrom(mcam::IImageBufferHeap* pImageBufferHeap);

 public:  ////                    Accessors.
  virtual buffer_handle_t* getBufferHandlePtr() const = 0;

  virtual int getGrallocUsage() const = 0;

  virtual int getStride() const = 0;

  virtual ::android::String8 getCallerName() const = 0;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace NSCam
#endif  // INCLUDE_MTKCAM_ANDROID_UTILS_IMGBUF_IGRAPHICIMAGEBUFFERHEAP_H_
