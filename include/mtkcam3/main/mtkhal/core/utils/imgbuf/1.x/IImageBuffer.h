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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_UTILS_IMGBUF_1_X_IIMAGEBUFFER_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_UTILS_IMGBUF_1_X_IIMAGEBUFFER_H_
//
#include "mtkcam/def/BuiltinTypes.h"
#include "mtkcam/def/UITypes.h"
#include "mtkcam/def/ImageBufferInfo.h"
#include "mtkcam/def/ImageDesc.h"
#include "mtkcam/def/ImageFormat.h"
// #include <mtkcam-interfaces/def/common.h>
#include <memory>
#include <string>
#include <vector>
//
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
using NSCam::eImgColorSpace_BT601_FULL;
using NSCam::eImgFmt_UNKNOWN;
using NSCam::ImageBufferInfo;
using NSCam::ImageDescId;
using NSCam::MSize;
using NSCam::SecType;

/******************************************************************************
 *  Image Buffer Usage.
 ******************************************************************************/
enum BufferUsageEnum {
  /* buffer is never read in software */
  eBUFFER_USAGE_SW_READ_NEVER = 0x00000000U,  // GRALLOC_USAGE_SW_READ_NEVER
  /* buffer is rarely read in software */
  eBUFFER_USAGE_SW_READ_RARELY = 0x00000002U,  // GRALLOC_USAGE_SW_READ_RARELY
  /* buffer is often read in software */
  eBUFFER_USAGE_SW_READ_OFTEN = 0x00000003U,  // GRALLOC_USAGE_SW_READ_OFTEN
  /* mask for the software read values */
  eBUFFER_USAGE_SW_READ_MASK = 0x0000000FU,  // GRALLOC_USAGE_SW_READ_MASK

  /* buffer is never written in software */
  eBUFFER_USAGE_SW_WRITE_NEVER = 0x00000000U,  // GRALLOC_USAGE_SW_WRITE_NEVER
  /* buffer is rarely written in software */
  eBUFFER_USAGE_SW_WRITE_RARELY = 0x00000020U,  // GRALLOC_USAGE_SW_WRITE_RARELY
  /* buffer is often written in software */
  eBUFFER_USAGE_SW_WRITE_OFTEN = 0x00000030U,  // GRALLOC_USAGE_SW_WRITE_OFTEN
  /* mask for the software write values */
  eBUFFER_USAGE_SW_WRITE_MASK = 0x000000F0U,  // GRALLOC_USAGE_SW_WRITE_MASK

  /* mask for the software access */
  eBUFFER_USAGE_SW_MASK =
      eBUFFER_USAGE_SW_READ_MASK | eBUFFER_USAGE_SW_WRITE_MASK,

  /* buffer will be used as an OpenGL ES texture (read by GPU) */
  eBUFFER_USAGE_HW_TEXTURE = 0x00000100U,  // GRALLOC_USAGE_HW_TEXTURE
  /* buffer will be used as an OpenGL ES render target (written by GPU) */
  eBUFFER_USAGE_HW_RENDER = 0x00000200U,  // GRALLOC_USAGE_HW_RENDER

  eBUFFER_USAGE_HW_COMPOSER = 0x00000800U,  // GRALLOC_USAGE_HW_COMPOSER

  eBUFFER_USAGE_HW_VIDEO_ENCODER =
      0x00010000U,  // GRALLOC_USAGE_HW_VIDEO_ENCODER

  /* buffer will be written by the HW camera pipeline */
  eBUFFER_USAGE_HW_CAMERA_WRITE = 0x00020000U,  // GRALLOC_USAGE_HW_CAMERA_WRITE
  /* buffer will be read by the HW camera pipeline */
  eBUFFER_USAGE_HW_CAMERA_READ = 0x00040000U,  // GRALLOC_USAGE_HW_CAMERA_READ
  /* mask for the camera access values */
  eBUFFER_USAGE_HW_CAMERA_MASK = 0x00060000U,  // GRALLOC_USAGE_HW_CAMERA_MASK
  /* buffer will be read and written by the HW camera pipeline */
  eBUFFER_USAGE_HW_CAMERA_READWRITE =
      eBUFFER_USAGE_HW_CAMERA_WRITE | eBUFFER_USAGE_HW_CAMERA_READ,

  /* mask for the hardware access */
  eBUFFER_USAGE_HW_MASK = 0x00071F00U,  // GRALLOC_USAGE_HW_MASK
};

/******************************************************************************
 *  Cache Flush Control Type.
 ******************************************************************************/
enum CacheCtrl { eCACHECTRL_FLUSH = 0, eCACHECTRL_INVALID = 1 };

/******************************************************************************
 *  ColorProfile Type.
 ******************************************************************************/
enum ColorProfile {
  eCOLORPROFILE_UNKNOWN,
  eCOLORPROFILE_BT601_LIMITED,
  eCOLORPROFILE_BT601_FULL
};

/******************************************************************************
 *
 ******************************************************************************/
class IImageBufferHeap;

/******************************************************************************
 *  Image Buffer Interface.
 ******************************************************************************/
class IImageBuffer {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Instantiation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:  ////                    Disallowed.
           /**
            * Copy constructor and Copy assignment are disallowed.
            */
  IImageBuffer(IImageBuffer const&);
  IImageBuffer& operator=(IImageBuffer const&);

 protected:  ////                    Destructor.
  IImageBuffer() {}

  //  Disallowed to directly delete a raw pointer.
  virtual ~IImageBuffer() {}

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Reference Counting.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Reference Counting.
  // TODO(Elon Hsu) Remove refbase related later.
  virtual MVOID incStrong(MVOID const* id) const = 0;
  virtual MVOID decStrong(MVOID const* id) const = 0;
  virtual MINT32 getStrongCount() const = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Image Attributes.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Image Attributes.
  virtual MINT getImgFormat() const = 0;
  virtual MSize const& getImgSize() const = 0;
  virtual size_t getImgBitsPerPixel() const = 0;
  virtual size_t getPlaneBitsPerPixel(size_t index) const = 0;
  virtual size_t getPlaneCount() const = 0;

  virtual size_t getBitstreamSize() const = 0;
  virtual MBOOL setBitstreamSize(size_t const bitstreamsize) = 0;

  /**
   *      * The values are defined in IHalSensor.h
   *           * like SENSOR_FORMAT_ORDER_R, SENSOR_FORMAT_ORDER_xx...
   *                */
  virtual void setColorArrangement(MINT32 const colorArrangement) = 0;
  virtual MINT32 getColorArrangement() const = 0;

  /**
   * Set or add image description.
   * @param id         Description ID
   * @param value      Value of the description
   * @param overwrite  If the description exists, overwrite = MTRUE can force to
   * overwrite the value. Otherwise, the setting will fail and nothing will be
   * changed.
   * @return MTRUE if the value is set successfully.
   *         MFALSE if id is invalid or description exists but the overwrite =
   * MFALSE.
   */
  virtual MBOOL setImgDesc(ImageDescId id,
                           MINT64 value,
                           MBOOL overwrite = MFALSE) = 0;

  /**
   * Get image description by ID.
   * @param id         Description ID
   * @param value      [OUT] The value of the description.
   * @return MTRUE if the description exists and the return value is valid.
   * Otherwise MFALSE.
   */
  virtual MBOOL getImgDesc(ImageDescId id, MINT64& value) const = 0;

  virtual std::shared_ptr<IImageBufferHeap> getImageBufferHeap() const = 0;

  // TODO(Elon Hsu) Survey it.
  virtual MBOOL setExtParam(MSize const& imgSize, size_t offsetInBytes = 0) = 0;

  // TODO(Elon Hsu) Survey it.
  virtual size_t getExtOffsetInBytes(size_t index) const = 0;

  virtual MINT32 getColorSpace() const = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Buffer Attributes.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Buffer Attributes.
  /**
   * Return a pointer to a null-terminated string to indicate a magic name of
   * buffer type.
   */
  virtual char const* getMagicName() const = 0;

  /**
   * Legal only after lockBuf().
   */
  virtual MINT32 getFD(size_t index = 0) const = 0;
  virtual MINT32 getPlaneFD(size_t index = 0) const = 0;

  /**
   * Legal only after lockBuf().
   */
  virtual size_t getFDCount() const = 0;

  /**
   * Buffer offset in bytes of a given plane.
   * Buf VA(i) = Buf Offset(i) + Heap VA(i)
   */
  // TODO(Elon Hsu) Survey it.
  virtual size_t getBufOffsetInBytes(size_t index) const = 0;

  /* TODO(Elon Hsu) ADDR(plane) = ADDR(FD) + getPlaneOffsetInBytes(plane) */
  virtual size_t getPlaneOffsetInBytes(size_t index) const = 0;

  /**
   * Buffer virtual address of a given plane;
   * legal only after lockBuf() with a SW usage.
   */
  virtual MINTPTR getBufVA(size_t index) const = 0;

  /**
   * Buffer size in bytes of a given plane; always legal.
   *
   * buffer size in bytes = buffer size in pixels x getPlaneBitsPerPixel(index)
   *
   * buffer size in pixels = buffer width stride in pixels
   *                       x(buffer height stride in pixels - 1)
   *                       + image width in pixels
   *
   * +---------+---------+---------+----------+
   * | Heap Pixel Array                       |
   * |                    O ROI Image         |
   * |                    = Buffer Size       |
   * |                                        |
   * |                    OOOOOOOOO===========|
   * |====================OOOOOOOOO===========|
   * |====================OOOOOOOOO===========|
   * |====================OOOOOOOOO           |
   * |                                        |
   * |                                        |
   * +---------+---------+---------+----------+
   *
   */
  virtual size_t getBufSizeInBytes(size_t index) const = 0;

  /**
   * Buffer Strides in bytes of a given plane; always legal.
   */
  virtual size_t getBufStridesInBytes(size_t index) const = 0;

  /**
   * Buffer Strides in pixel of a given plane; always legal.
   */
  virtual size_t getBufStridesInPixel(size_t index) const = 0;

  /**
   * Buffer scanlines of a given plane; always legal.
   */
  virtual size_t getBufScanlines(size_t index) const = 0;

  /**
   * Return a buffer type that indicates the buffer access permission.
   */
  virtual SecType getSecType() const = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Buffer Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  /**
   * A buffer is allowed to access only between the interval of lockBuf() and
   * unlockBuf(). Call lockBuf() with a usage flag before accessing a buffer,
   * and call unlockBuf() after finishing accessing it.
   *
   * Virtual address of a buffer, from getBufVA(), is legal only if a SW usage
   * is specified when lockBuf().
   *
   * Physical and virtual addresses are legal if HW and SW usages are
   * specified when lockBuf().
   *
   */
  virtual MBOOL lockBuf(char const* szCallerName,
                        MINT usage = eBUFFER_USAGE_HW_CAMERA_READWRITE |
                                     eBUFFER_USAGE_SW_READ_OFTEN) = 0;
  virtual MBOOL unlockBuf(char const* szCallerName) = 0;

 public:  ////                    Cache.
  virtual MBOOL syncCache(CacheCtrl const ctrl = eCACHECTRL_FLUSH) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  File Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    File Operations.
  virtual MBOOL saveToFile(char const* filepath) = 0;
  virtual MBOOL loadFromFile(char const* filepath) = 0;
  virtual MBOOL loadFromFiles(
      const std::vector<std::string>& vecPlaneFilePath) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Timestamp Accesssors.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Timestamp Accesssors.
  virtual MINT64 getTimestamp() const = 0;
  virtual MVOID setTimestamp(MINT64 const timestamp) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Fence Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Fence Operations.
  /**
   * The acquire sync fence for this buffer. The HAL must wait on this fence
   * fd before attempting to read from or write to this buffer.
   *
   * The framework may be set to -1 to indicate that no waiting is necessary
   * for this buffer.
   *
   * When the HAL returns an output buffer to the framework with
   * process_capture_result(), the acquire_fence must be set to -1. If the HAL
   * never waits on the acquire_fence due to an error in filling a buffer,
   * when calling process_capture_result() the HAL must set the release_fence
   * of the buffer to be the acquire_fence passed to it by the framework. This
   * will allow the framework to wait on the fence before reusing the buffer.
   *
   * For input buffers, the HAL must not change the acquire_fence field during
   * the process_capture_request() call.
   */
  virtual MINT getAcquireFence() const = 0;
  virtual MVOID setAcquireFence(MINT fence) = 0;

  /**
   * The release sync fence for this buffer. The HAL must set this fence when
   * returning buffers to the framework, or write -1 to indicate that no
   * waiting is required for this buffer.
   *
   * For the input buffer, the release fence must be set by the
   * process_capture_request() call. For the output buffers, the fences must
   * be set in the output_buffers array passed to process_capture_result().
   */
  virtual MINT getReleaseFence() const = 0;
  virtual MVOID setReleaseFence(MINT fence) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Buffer Operations for NDD utils
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  /* To achieve asynchronized NDD dump, we need to clone a new IImageBuffer
   * from user's IImageBuffer. Cloned IImageBuffer would allocate additional
   * DMA buffer. If buffer allocation fail, return nullptr instead.
   */
  virtual std::shared_ptr<IImageBuffer> clone() = 0;
};

class ImgBufCreator {
 public:
  explicit ImgBufCreator(MINT manualFormat = eImgFmt_UNKNOWN) {
    mCreatorFormat = manualFormat;
  }
  MINT generateFormat(IImageBufferHeap* heap);

 private:
  MINT mCreatorFormat;
};

/******************************************************************************
 *  Image Buffer Heap Interface.
 ******************************************************************************/
class IImageBufferHeap {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Instantiation.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private:  ////                    Disallowed.
  /**
   * Copy assignment are disallowed.
   */
  IImageBufferHeap& operator=(IImageBufferHeap const&);

 protected:  ////                    Destructor.
  /**
   * Disallowed to directly delete a raw pointer.
   */
  virtual ~IImageBufferHeap() {}

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Reference Counting.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Reference Counting.
  virtual MVOID incStrong(MVOID const* id) const = 0;
  virtual MVOID decStrong(MVOID const* id) const = 0;
  virtual MINT32 getStrongCount() const = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Image Attributes.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Image Attributes.
  virtual MINT getImgFormat() const = 0;
  virtual MSize const& getImgSize() const = 0;
  virtual size_t getImgBitsPerPixel() const = 0;
  virtual size_t getPlaneBitsPerPixel(size_t index) const = 0;
  virtual size_t getPlaneCount() const = 0;
  virtual size_t getBitstreamSize() const = 0;
  virtual MBOOL setBitstreamSize(size_t const bitstreamsize) = 0;
  /**
   * The values are defined in IHalSensor.h
   * like SENSOR_FORMAT_ORDER_R, SENSOR_FORMAT_ORDER_xx...
   */
  virtual void setColorArrangement(MINT32 const colorArrangement) = 0;
  virtual MINT32 getColorArrangement() const = 0;

  virtual MVOID setColorSpace(MINT32 colorspace) = 0;
  virtual MINT32 getColorSpace() const = 0;

  /**
   * Set or add image description.
   * @param id         Description ID
   * @param value      Value of the description
   * @param overwrite  If the description exists, overwrite = MTRUE can force to
   * overwrite the value. Otherwise, the setting will fail and nothing will be
   * changed.
   * @return MTRUE if the value is set successfully.
   *         MFALSE if id is invalid or description exists but the overwrite =
   * MFALSE.
   */
  virtual MBOOL setImgDesc(ImageDescId id,
                           MINT64 value,
                           MBOOL overwrite = MFALSE) = 0;

  /**
   * Get image description by ID.
   * @param id         Description ID
   * @param value      [OUT] The value of the description.
   * @return MTRUE if the description exists and the return value is valid.
   * Otherwise MFALSE.
   */
  virtual MBOOL getImgDesc(ImageDescId id, MINT64& value) const = 0;

  virtual MBOOL updateImgInfo(MSize const& imgSize,
                              MINT const imgFormat,
                              size_t const sizeInBytes[3],
                              size_t const rowStrideInBytes[3],
                              size_t const bufPlaneSize) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Buffer Attributes.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Buffer Attributes.
  /**
   * Return log condition to enable/disable printing log.
   */
  virtual MBOOL getLogCond() const = 0;

  /**
   * Return a pointer to a null-terminated string to indicate a magic name of
   * buffer type.
   */
  virtual char const* getMagicName() const = 0;

  /**
   * Return magic instance
   */
  virtual void* getMagicInstance() const = 0;

  /**
   * Heap ID could be ION fd, PMEM fd, and so on.
   * Legal only after lockBuf().
   */
  virtual MINT32 getHeapID(size_t index = 0) const = 0;

  /**
   * 0 <= Heap ID count <= plane count.
   * Legal only after lockBuf().
   */
  virtual size_t getHeapIDCount() const = 0;

  /**
   * Buffer virtual address of a given plane;
   * legal only after lockBuf() with a SW usage.
   */
  virtual MINTPTR getBufVA(size_t index) const = 0;

  /**
   * Buffer size in bytes of a given plane; always legal.
   */
  virtual size_t getBufSizeInBytes(size_t index) const = 0;

  /**
   * Buffer Strides in bytes of a given plane; always legal.
   */
  virtual size_t getBufStridesInBytes(size_t index) const = 0;

  /**
   * Buffer customized size, which means caller specified the buffer size he
   * wants of the given plane. Caller usually gives this value because he wants
   * vertical padding of image. This method returns 0 if caller didn't specify
   * the customized buffer size of the given plane.
   */
  virtual size_t getBufCustomSizeInBytes(size_t index) const = 0;

  /**
   * Graphic buffer from grallocImageBufferHeap.
   */
  virtual void* getHWBuffer() const = 0;

  /**
   * Return a buffer type that indicates the buffer access permission.
   */
  virtual SecType getSecType() const = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Fence Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Fence Operations.
  /**
   * The acquire sync fence for this buffer. The HAL must wait on this fence
   * fd before attempting to read from or write to this buffer.
   */
  virtual MINT getAcquireFence() const = 0;
  virtual MVOID setAcquireFence(MINT fence) = 0;

  /**
   * The release sync fence for this buffer. The HAL must set this fence when
   * returning buffers to the framework, or write -1 to indicate that no
   * waiting is required for this buffer.
   */
  virtual MINT getReleaseFence() const = 0;
  virtual MVOID setReleaseFence(MINT fence) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Buffer Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  /**
   * A buffer is allowed to access only between the interval of lockBuf() and
   * unlockBuf(). Call lockBuf() with a usage flag before accessing a buffer,
   * and call unlockBuf() after finishing accessing it.
   *
   * Virtual address of a buffer, from getBufVA(), is legal only if a SW usage
   * is specified when lockBuf().
   *
   * Physical and virtual addresses are legal if HW and SW usages are
   * specified when lockBuf().
   *
   */
  virtual MBOOL lockBuf(char const* szCallerName,
                        MINT usage = eBUFFER_USAGE_HW_CAMERA_READWRITE |
                                     eBUFFER_USAGE_SW_READ_OFTEN) = 0;
  virtual MBOOL unlockBuf(char const* szCallerName) = 0;

 public:  ////                    Cache.
  virtual MBOOL syncCache(CacheCtrl const ctrl) = 0;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IImageBuffer Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  /**
   * Create an IImageBuffer instance with its ROI equal to the image full
   * resolution of this heap.
   */
  virtual std::shared_ptr<IImageBuffer> createImageBuffer(
      ImgBufCreator* pCreator = NULL) = 0;

  /**
   * This call is legal only if the heap format is blob.
   *
   * From the given blob heap, create an IImageBuffer instance with a specified
   * offset and size, and its format equal to blob.
   */
  virtual std::shared_ptr<IImageBuffer> createImageBuffer_FromBlobHeap(
      size_t offsetInBytes,
      size_t sizeInBytes) = 0;

  /**
   * This call is legal only if the heap format is blob.
   *
   * From the given blob heap, create an IImageBuffer instance with a specified
   * offset, image format, image size in pixels, and buffer strides in pixels.
   */
  virtual std::shared_ptr<IImageBuffer> createImageBuffer_FromBlobHeap(
      size_t offsetInBytes,
      MINT32 imgFormat,
      MSize const& imgSize,
      size_t const bufStridesInBytes[3]) = 0;

  /**
   * Create an IImageBuffer instance indicating the left-side or right-side
   * buffer within a side-by-side image.
   *
   * Left side if isRightSide = 0; otherwise right side.
   */
  virtual std::shared_ptr<IImageBuffer> createImageBuffer_SideBySide(
      MBOOL isRightSide) = 0;

  /**
   * This call is legal only if the heap format is blob.
   *
   * From the given blob heap, create multiple IImageBuffer instances with a
   * specified ImageBufferInfo.
   *
   * Attention: Caller gets raw pointer and MUST free the pointer after using
   * it.
   *
   */
  virtual std::vector<std::shared_ptr<IImageBuffer>>
  createImageBuffers_FromBlobHeap(const ImageBufferInfo& info,
                                  const char* callerName,
                                  MBOOL dirty_content = MFALSE) = 0;

  virtual std::shared_ptr<IImageBufferHeap> getPtr() = 0;
};

class IImageBufferAllocator {
 public:
  struct ImgParam {
    /*************************************************
     * A. common field (Must)
     *************************************************/
    MINT32 format;      // Refer to ImageFormat.h
    NSCam::MSize size;  // Image resolution in pixels. For Blob or JPEG,
                        // imgSize = (bufSizeInByte, 1)

    // Combination of eBUFFER_USAGE. Notice that do not specify
    // CPU/GPU usage when user does not R/W the buffer with CPU/GPU.
    MUINT64 usage;  // Refer to eBUFFER_USAGE

    /*************************************************
     * B. common field (Opt.)
     *************************************************/
    SecType secType = SecType::mem_normal;
    bool nocache = false;
    bool continuousPlane = true;  // If TRUE, planes are continuous in memory
                                  // and share the same FD.
    MINT32 colorspace = eImgColorSpace_BT601_FULL;
    /**
     * If TRUE, multiple users are allowed to lock buffer,
     * for low-latency cases.
     */
    MBOOL hwsync = MFALSE;

    /*************************************************
     * C. Stride related field (Opt.)
     * User could specify stride by 1 or 2. (choose 1 first)
     * If not specify, planeSride = ALIGN(planeWidth * planeBitPerPixel, Byte)
     *************************************************/
    // 1. Stride in bytes
    size_t strideInByte[3] = {0};

    // 2. Stride alignment (do pixel and then byte align)
    size_t strideAlignInPixel = 0;
    size_t strideAlignInByte = 0;

    /*************************************************
     * D. Size related field (Opt.)
     * User could specify size by bufSizeInByte
     * If not specify, planeSize = planeStride * planeHeight
     *************************************************/
    // 1. BufSize in bytes
    size_t bufSizeInByte[3] = {0};

    /*************************************************
     * Constructor of ImgParam
     *************************************************/
    ImgParam(MINT32 _format, NSCam::MSize _size, MUINT64 _usage)
        : format(_format), size(_size), usage(_usage) {}

    static ImgParam getBlobParam(MINT32 bufsize, MUINT64 _usage) {
      return ImgParam(NSCam::eImgFmt_BLOB, NSCam::MSize(bufsize, 1), _usage);
    }

    static ImgParam getJpegParam(MINT32 bufsize, MUINT64 _usage) {
      return ImgParam(NSCam::eImgFmt_JPEG, NSCam::MSize(bufsize, 1), _usage);
    }
  };

 public:
  static std::shared_ptr<IImageBuffer> alloc(
      char const* clientName,  // the name of whom create the buf
      char const* bufName,     // the name of the buffer
      ImgParam const& imgParam,
      bool enableLog = false);
};

class IImageBufferHeapFactory {
 public:
  static std::shared_ptr<IImageBufferHeap> create(
      char const* clientName,  // the name of whom create the buf
      char const* bufName,     // the name of the buffer
      IImageBufferAllocator::ImgParam const& imgParam,
      bool enableLog = false);

  /* Create a dummy heap from blob without additional memory allocation and
   * the blob & dummy would share the same FD. Please notice the followings:
   * 1. ADDR(dummy) = ADDR(FD) + offsetInByte
   * 2. Need to lock blob heap before create dummy heap
   * 3. Do not unlock or destroy blob heap before all dummy heap destroy
   */
  static std::shared_ptr<IImageBufferHeap> createDummyFromBlobHeap(
      char const* clientName,  // the name of whom create the buf
      char const* bufName,     // the name of the buffer
      std::shared_ptr<IImageBufferHeap> blobHeap,
      size_t offsetInByte,
      IImageBufferAllocator::ImgParam const& imgParam,
      bool enableLog = false);

  /* Create a dummy heap from exist buffer FD without additional memory
   * allocation Please notice the followings:
   * 1. ADDR(dummy) = ADDR(FD) + offsetInByte
   * 2. Heap would dup the FD. We would not use FD from user
   */
  static std::shared_ptr<IImageBufferHeap> createDummyFromBufferFD(
      char const* clientName,  // the name of whom create the buf
      char const* bufName,     // the name of the buffer
      int bufFd,               // the FD of existing buffer
      size_t offsetInByte,
      IImageBufferAllocator::ImgParam const& imgParam,
      bool enableLog = false);

 protected:
  IImageBufferHeapFactory() {}
  virtual ~IImageBufferHeapFactory() {}
};

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace mtkcam_hal

#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_UTILS_IMGBUF_1_X_IIMAGEBUFFER_H_
