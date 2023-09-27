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

#define LOG_TAG "MtkCam/GraphicImageBufferHeap"
//
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam/utils/std/ULog.h>

#include <graphics_mtk_defs.h>
#include <linux/ion_drv.h>
#include <ui/gralloc_extra.h>
#include <vndk/hardware_buffer.h>

#include <memory>

#include "include/BaseImageBufferHeap.h"
#include "include/MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);
//
// using namespace android;
// using namespace NSCam;
//

using android::sp;
using android::Vector;
using android::status_t;
using android::OK;
using mcam::IGraphicImageBufferHeap;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;
using NSCam::MSize;
using mcam::android::AppBufferHandleHolder;

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

/******************************************************************************
 *  Image Buffer Heap.
 ******************************************************************************/
namespace {
class GraphicImageBufferHeapBase
    : public IGraphicImageBufferHeap,
      public BaseImageBufferHeap {
  friend class IGraphicImageBufferHeap;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IGraphicImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Accessors.
  virtual buffer_handle_t getBufferHandle() const {
    return mBufferParams.handle;
  }
  virtual buffer_handle_t* getBufferHandlePtr() const {
    return const_cast<buffer_handle_t*>(&mBufferParams.handle);
  }
  virtual MINT getAcquireFence() const { return mAcquireFence; }
  virtual MVOID setAcquireFence(MINT fence) { mAcquireFence = fence; }
  virtual MINT getReleaseFence() const { return mReleaseFence; }
  virtual MVOID setReleaseFence(MINT fence) { mReleaseFence = fence; }

  virtual int getGrallocUsage() const { return mBufferParams.usage; }
  virtual int getStride() const { return mBufferParams.stride[0]; }
  virtual ::android::String8 getCallerName() const { return mCallerName; }
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  BaseImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  virtual char const* impGetMagicName() const { return magicName(); }
  virtual void* impGetMagicInstance() const { return
                  const_cast<void*>(reinterpret_cast<const void *>(this)); }
  virtual HeapInfoVect_t const& impGetHeapInfo() const {
    return *(const_cast<HeapInfoVect_t*>
            (reinterpret_cast<const HeapInfoVect_t *>(&mvHeapInfo)));
  }
  virtual MBOOL impInit(BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impUninit(BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impReconfig(BufInfoVect_t const& rvBufInfo __unused) {
    return MFALSE;
  }

 public:  ////
  virtual MBOOL impLockBuf(char const* szCallerName,
                           MINT usage,
                           BufInfoVect_t const& rvBufInfo);
  virtual MBOOL impUnlockBuf(char const* szCallerName,
                             MINT usage,
                             BufInfoVect_t const& rvBufInfo);
  virtual void* getHWBuffer() const {
    return
    reinterpret_cast<void*>(ANativeWindowBuffer_getHardwareBuffer(mpANWBuffer));
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Heap Info.
  struct MyHeapInfo : public HeapInfo {};
  typedef Vector<sp<MyHeapInfo> > MyHeapInfoVect_t;

 protected:  ////                    Buffer Info.
  typedef Vector<sp<BufInfo> > MyBufInfoVect_t;

  struct BufferParams {
    int width_for_lock = 0;
    int height_for_lock = 0;
    int width = 0;
    int height = 0;
    int stride[3] = {0};
    int format = 0;
    int usage = 0;
    uintptr_t layerCount = 0;
    buffer_handle_t handle = nullptr;
    std::shared_ptr<AppBufferHandleHolder> handleHolder = nullptr;
    bool owner = false;
    NSCam::SecType secType = NSCam::SecType::mem_normal;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //  Instantiation.
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                    Destructor/Constructors.
  /**
   * Disallowed to directly delete a raw pointer.
   */
  virtual ~GraphicImageBufferHeapBase() = default;
  GraphicImageBufferHeapBase(char const* szCallerName,
                             BufferParams& rBufParams,
                             ANativeWindowBuffer* pANWBuffer,
                             MINT const acquire_fence,
                             MINT const release_fence);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Data Members.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////
  MyHeapInfoVect_t mvHeapInfo;
  MyBufInfoVect_t mvBufInfo;
  NSCam::GrallocDynamicInfo mGrallocInfo;

 protected:  ////
  BufferParams mBufferParams;
  ANativeWindowBuffer* mpANWBuffer;
  MINT mAcquireFence;
  MINT mReleaseFence;
  MBOOL isGrallocLocked;
};

class NormalGraphicImageBufferHeap final : public GraphicImageBufferHeapBase {
 protected:
  friend class IGraphicImageBufferHeap;
  /**
   * Disallowed to directly delete a raw pointer.
   */
  NormalGraphicImageBufferHeap(char const* szCallerName,
                               BufferParams& rBufParams,
                               ANativeWindowBuffer* pANWBuffer,
                               MINT const acquire_fence,
                               MINT const release_fence)
      : GraphicImageBufferHeapBase(szCallerName,
                                   rBufParams,
                                   pANWBuffer,
                                   acquire_fence,
                                   release_fence) {}

  ~NormalGraphicImageBufferHeap() = default;
};

class SecureGraphicImageBufferHeap final : public GraphicImageBufferHeapBase {
 public:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  BaseImageBufferHeap Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  MBOOL impLockBuf(char const* szCallerName,
                   MINT usage,
                   BufInfoVect_t const& rvBufInfo) override;
  MBOOL impUnlockBuf(char const* szCallerName,
                     MINT usage,
                     BufInfoVect_t const& rvBufInfo) override;

 protected:
  friend class IGraphicImageBufferHeap;
  /**
   * Disallowed to directly delete a raw pointer.
   */
  SecureGraphicImageBufferHeap(char const* szCallerName,
                               BufferParams& rBufParams,
                               ANativeWindowBuffer* pANWBuffer,
                               MINT const acquire_fence,
                               MINT const release_fence);

  ~SecureGraphicImageBufferHeap() = default;

 private:
  SECHAND mSecureHandle;

  void getSecureHandle(SECHAND& secureHandle);
};

};  // namespace

IGraphicImageBufferHeap* IGraphicImageBufferHeap::castFrom(
    mcam::IImageBufferHeap* pImageBufferHeap) {
  if (CC_UNLIKELY(strcmp(pImageBufferHeap->getMagicName(),
                         IGraphicImageBufferHeap::magicName()))) {
    //        CAM_ULOGMW("MagicName (%s,%s) is different!",
    //        pImageBufferHeap->getMagicName(),IGraphicImageBufferHeap::magicName());
    return nullptr;
  } else {
    return static_cast<GraphicImageBufferHeapBase*>(
        pImageBufferHeap->getMagicInstance());
  }
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<IGraphicImageBufferHeap> IGraphicImageBufferHeap::create(
    char const* szCallerName,
    uint32_t width,
    uint32_t height,
    uint32_t usage,
    buffer_handle_t* buffer,
    MINT const acquire_fence,
    MINT const release_fence,
    bool keepOwnership,
    NSCam::SecType secType,
    std::shared_ptr<IIonDevice> ionDevice) {
  if (CC_UNLIKELY(!buffer)) {
    CAM_ULOGME("bad buffer_handle_t");
    return nullptr;
  }
  //
  if (CC_UNLIKELY(!*buffer)) {
    CAM_ULOGME("bad *buffer_handle_t");
    return nullptr;
  }
  //
  if (keepOwnership == true) {
    CAM_ULOGME("not implement keepOwnership=ture");
    return nullptr;
  }
  //
  mcam::GrallocStaticInfo staticInfo;
  status_t status =
      NSCam::IGrallocHelper::singleton()->query(*buffer, &staticInfo, nullptr);
  if (CC_UNLIKELY(OK != status)) {
    CAM_ULOGME(
        "cannot query the real format from buffer_handle_t - status:%d(%s)",
        status, ::strerror(-status));
    return nullptr;
  }
  //
  MSize alloc_size = MSize(width, height);
  if (staticInfo.format ==
      NSCam::eImgFmt_BLOB)  // resize from [w x h] to [w*h x 1]
    alloc_size = MSize(width * height, 1);
  //
  GraphicImageBufferHeapBase::BufferParams rBufParams = {
      .width_for_lock = staticInfo.widthInPixels,
      .height_for_lock = staticInfo.heightInPixels,
      .width = alloc_size.w,
      .height = alloc_size.h,
      .format = staticInfo.format,
      .usage = static_cast<int>(usage),
      .layerCount = 1,
      .handle = const_cast<native_handle_t*>(*buffer),
      .owner = keepOwnership,
      .secType = secType,
  };
  auto* pHeap = [&]() -> GraphicImageBufferHeapBase* {
    if (secType == NSCam::SecType::mem_normal)
      return new NormalGraphicImageBufferHeap(szCallerName, rBufParams, nullptr,
                                              acquire_fence, release_fence);

    return new SecureGraphicImageBufferHeap(szCallerName, rBufParams, nullptr,
                                            acquire_fence, release_fence);
  }();
  //
  if (CC_UNLIKELY(!pHeap)) {
    CAM_ULOGME("Fail to new a heap");
    return nullptr;
  }
  MSize const imgSize(rBufParams.width, rBufParams.height);
  // MINT format = pGraphicBuffer->getPixelFormat();
  MINT format = staticInfo.format;
  switch (format) {
    case NSCam::eImgFmt_BLOB:
      CAM_ULOGMW("create() based-on buffer_handle_t...");
#if 0
        CAM_ULOGMW("force to convert BLOB format to JPEG format");
        format = NSCam::eImgFmt_JPEG;
#else
      CAM_ULOGMW("should we convert BLOB format to JPEG format ???");
#endif
      break;
  }
  auto ret = pHeap->onCreate(GraphicImageBufferHeapBase::OnCreate{
      .imgSize = imgSize,
      .imgFormat = format,
      .bitstreamSize = 0,
      .secType = secType,
      .enableLog = MFALSE,
      .useSharedDeviceFd = -1,
      .ionDevice = ionDevice,
      .hwsync = MFALSE,
  });
  if (CC_UNLIKELY(!ret)) {
    CAM_ULOGME("onCreate width(%d) height(%d)", imgSize.w, imgSize.h);
    delete pHeap;
    return nullptr;
  }

  // create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<GraphicImageBufferHeapBase> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<IGraphicImageBufferHeap> IGraphicImageBufferHeap::create(
    char const* szCallerName,
    uint32_t usage,
    MSize imgSize,
    int format,
    buffer_handle_t* buffer,
    MINT const acquire_fence,
    MINT const release_fence,
    int stride,
    NSCam::SecType secType,
    std::shared_ptr<IIonDevice> ionDevice,
    std::shared_ptr<AppBufferHandleHolder> handleHolder) {
  if (CC_UNLIKELY(!buffer)) {
    CAM_ULOGME("bad buffer_handle_t");
    return nullptr;
  }
  //
  if (CC_UNLIKELY(!*buffer)) {
    CAM_ULOGME("bad *buffer_handle_t");
    return nullptr;
  }
  //
  mcam::GrallocStaticInfo staticInfo;
  status_t status =
      NSCam::IGrallocHelper::singleton()->query(*buffer, &staticInfo, nullptr);
  if (CC_UNLIKELY(OK != status)) {
    CAM_ULOGME(
        "cannot query the real format from buffer_handle_t - status:%d(%s)",
        status, ::strerror(-status));
    return nullptr;
  }
  //
  MSize alloc_size = imgSize;
  if (staticInfo.format ==
      NSCam::eImgFmt_BLOB)  // resize from [w x h] to [w*h x 1]
    alloc_size = MSize(imgSize.w * imgSize.h, 1);
  //
  switch (staticInfo.format) {
    case NSCam::eImgFmt_BLOB:
      if (format == NSCam::eImgFmt_JPEG) {
        CAM_ULOGMD("BLOB -> JPEG");
        break;
      } else if (format == NSCam::eImgFmt_JPEG_APP_SEGMENT) {
        CAM_ULOGMD("BLOB -> JPEG_APP_SEGMENT");
        break;
      } else if (format == NSCam::eImgFmt_ISP_TUING) {
        CAM_LOGD_IF(1, "BLOB -> ISP_TUNING");
        break;
      }
      CAM_ULOGME("the buffer format is BLOB, but the given format:%d != JPEG",
                 format);
      return nullptr;
      break;
    case HAL_PIXEL_FORMAT_RAW16:
      CAM_ULOGMD("RAW16 -> %#x(%s)", format,
                 NSCam::Utils::Format::queryImageFormatName(format).c_str());
      break;
    case HAL_PIXEL_FORMAT_RAW10:
      CAM_ULOGMD("RAW10 -> %#x(%s)", format,
                 NSCam::Utils::Format::queryImageFormatName(format).c_str());
      break;
    case HAL_PIXEL_FORMAT_CAMERA_OPAQUE:
      if ((format >= NSCam::eImgFmt_BAYER8 &&
           format <= NSCam::eImgFmt_BAYER14) ||
          (format >= NSCam::eImgFmt_UFO_BAYER8 &&
           format <= NSCam::eImgFmt_UFO_BAYER14)) {
        CAM_ULOGMI("HAL_PIXEL_FORMAT_CAMERA_OPAQUE -> PACK_RAW(0x%X)", format);
        break;
      }
      if (CC_UNLIKELY(format != staticInfo.format &&
                      format != NSCam::eImgFmt_BLOB)) {
        CAM_ULOGME(
            "the buffer_handle_t format:%d is neither BLOB, nor the given "
            "format:%d",
            staticInfo.format, format);
        return nullptr;
      }
      break;
    default:
      if (CC_UNLIKELY(format != staticInfo.format &&
                      format != NSCam::eImgFmt_BLOB)) {
        CAM_ULOGME(
            "the buffer_handle_t format:%d is neither BLOB, nor the given "
            "format:%d",
            staticInfo.format, format);
        return nullptr;
      }
      break;
  }
  //
  GraphicImageBufferHeapBase::BufferParams rBufParams = {
      .width_for_lock = staticInfo.widthInPixels,
      .height_for_lock = staticInfo.heightInPixels,
      .width = alloc_size.w,
      .height = alloc_size.h,
      .format = format,
      .usage = static_cast<int>(usage),
      .layerCount = 1,
      .handle = const_cast<native_handle_t*>(*buffer),
      .handleHolder = handleHolder,
      .owner = false,
      .secType = secType,
  };
  rBufParams.stride[0] = stride;

  auto* pHeap = [&]() -> GraphicImageBufferHeapBase* {
    if (secType == NSCam::SecType::mem_normal)
      return new NormalGraphicImageBufferHeap(szCallerName, rBufParams, nullptr,
                                              acquire_fence, release_fence);

    return new SecureGraphicImageBufferHeap(szCallerName, rBufParams, nullptr,
                                            acquire_fence, release_fence);
  }();
  //
  if (CC_UNLIKELY(!pHeap)) {
    CAM_ULOGME("Fail to new a heap");
    return nullptr;
  }
  //
  // BLOB: [w*h x 1]
  // JPEG: [w*h x 1] or [w x h]?
  auto ret = pHeap->onCreate(GraphicImageBufferHeapBase::OnCreate{
      .imgSize = alloc_size,
      .imgFormat = format,
      .bitstreamSize = 0,
      .secType = secType,
      .enableLog = MFALSE,
      .useSharedDeviceFd = -1,
      .ionDevice = ionDevice,
      .hwsync = MFALSE,
  });
  if (CC_UNLIKELY(!ret)) {
    CAM_ULOGME("onCreate imgSize:%dx%d format:%d buffer format:%d", imgSize.w,
               imgSize.h, format, staticInfo.format);
    delete pHeap;
    return nullptr;
  }

  // create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<GraphicImageBufferHeapBase> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

/******************************************************************************
 *
 ******************************************************************************/
std::shared_ptr<IGraphicImageBufferHeap> IGraphicImageBufferHeap::create(
    char const* szCallerName,
    ANativeWindowBuffer* pANWBuffer,
    MINT const acquire_fence,
    MINT const release_fence,
    bool keepOwnership,
    NSCam::SecType secType,
    std::shared_ptr<IIonDevice> ionDevice) {
  if (CC_UNLIKELY(pANWBuffer == 0)) {
    CAM_ULOGME("NULL ANativeWindowBuffer");
    return nullptr;
  }
  //
  if (keepOwnership == true) {
    CAM_ULOGME("not implement keepOwnership=ture");
    return nullptr;
  }
  //
  GraphicImageBufferHeapBase::BufferParams rBufParams = {
      .width = pANWBuffer->width,
      .height = pANWBuffer->height,
      .format = pANWBuffer->format,
      .usage = static_cast<int>(pANWBuffer->usage),
      .layerCount = pANWBuffer->layerCount,
      .handle = const_cast<native_handle_t*>(pANWBuffer->handle),
      .owner = keepOwnership,
      .secType = secType,
  };
  rBufParams.stride[0] = pANWBuffer->stride;
  //
  mcam::GrallocStaticInfo staticInfo;
  status_t status = NSCam::IGrallocHelper::singleton()->query(
      rBufParams.handle, &staticInfo, nullptr);
  if (CC_UNLIKELY(OK != status)) {
    CAM_ULOGME(
        "cannot query the real format from buffer_handle_t - status:%d(%s)",
        status, ::strerror(-status));
    return nullptr;
  }
  rBufParams.width_for_lock = staticInfo.widthInPixels;
  rBufParams.height_for_lock = staticInfo.heightInPixels;
  //
  MINT const format = staticInfo.format;
  // MINT const format1 = pGraphicBuffer->getPixelFormat();
  MSize const imgSize(rBufParams.width, rBufParams.height);
  //
  auto* pHeap = [&]() -> GraphicImageBufferHeapBase* {
    if (secType == NSCam::SecType::mem_normal)
      return new NormalGraphicImageBufferHeap(
          szCallerName, rBufParams, pANWBuffer, acquire_fence, release_fence);

    return new SecureGraphicImageBufferHeap(
        szCallerName, rBufParams, pANWBuffer, acquire_fence, release_fence);
  }();
  if (CC_UNLIKELY(!pHeap)) {
    CAM_ULOGME("Fail to new a heap");
    return nullptr;
  }
  auto ret = pHeap->onCreate(GraphicImageBufferHeapBase::OnCreate{
      .imgSize = imgSize,
      .imgFormat = format,
      .bitstreamSize = 0,
      .secType = secType,
      .enableLog = MFALSE,
      .useSharedDeviceFd = -1,
      .ionDevice = ionDevice,
      .hwsync = MFALSE,
  });
  if (CC_UNLIKELY(!ret)) {
    CAM_ULOGME("onCreate width(%d) height(%d)", imgSize.w, imgSize.h);
    delete pHeap;
    return nullptr;
  }

  // create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<GraphicImageBufferHeapBase> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

std::shared_ptr<IGraphicImageBufferHeap>
IGraphicImageBufferHeap::createFromBlob(
  char const* szCallerName,
  mcam::IImageBufferAllocator::ImgParam param,
  buffer_handle_t* buffer,
  MINT const acquire_fence,
  MINT const release_fence,
  std::shared_ptr<IIonDevice> ionDevice,
  std::shared_ptr<AppBufferHandleHolder> handleHolder) {
  if (CC_UNLIKELY(!buffer)) {
    CAM_ULOGME("bad buffer_handle_t");
    return nullptr;
  }
  //
  if (CC_UNLIKELY(!*buffer)) {
    CAM_ULOGME("bad *buffer_handle_t");
    return nullptr;
  }
  //
  mcam::GrallocStaticInfo staticInfo;
  status_t status =
      NSCam::IGrallocHelper::singleton()->query(*buffer, &staticInfo, nullptr);
  if (CC_UNLIKELY(OK != status)) {
    CAM_ULOGME(
        "cannot query the real format from buffer_handle_t - status:%d(%s)",
        status, ::strerror(-status));
    return nullptr;
  }

  if (staticInfo.format != HAL_PIXEL_FORMAT_BLOB) {
    CAM_ULOGME("%s do not support %s", __FUNCTION__,
        NSCam::Utils::Format::queryImageFormatName(param.format).c_str());
    return nullptr;
  } else {
    CAM_ULOGMD("BLOB -> %s",
        NSCam::Utils::Format::queryImageFormatName(param.format).c_str());
  }

  if (param.bufSizeInByte[0] != 0 || param.bufSizeInByte[1] != 0 ||
      param.bufSizeInByte[2] != 0) {
    CAM_ULOGME("%s do not support customized size", __FUNCTION__);
    return nullptr;
  }

  GraphicImageBufferHeapBase::BufferParams rBufParams = {
      .width_for_lock     = staticInfo.widthInPixels,
      .height_for_lock    = staticInfo.heightInPixels,
      .width = param.size.w,
      .height = param.size.h,
      .format = param.format,
      .usage = static_cast<int>(param.usage),
      .layerCount = 1,
      .handle = const_cast<native_handle_t*>(*buffer),
      .handleHolder = handleHolder,
      .owner = false,
      .secType = param.secType,
  };

  /*  type of rBufParams.stride is int; type of strideInByte is size_t */
  for (int i = 0; i < sizeof(rBufParams.stride) /
       sizeof(rBufParams.stride[0]); i++)
    rBufParams.stride[i] = param.strideInByte[i];

  auto* pHeap = [&]() -> GraphicImageBufferHeapBase* {
    if (param.secType == NSCam::SecType::mem_normal)
      return new NormalGraphicImageBufferHeap(szCallerName, rBufParams, nullptr,
                                              acquire_fence, release_fence);

    return new SecureGraphicImageBufferHeap(szCallerName, rBufParams, nullptr,
                                            acquire_fence, release_fence);
  }();
  //
  if (CC_UNLIKELY(!pHeap)) {
    CAM_ULOGME("Fail to new a heap");
    return nullptr;
  }
  //
  auto ret = pHeap->onCreate(GraphicImageBufferHeapBase::OnCreate{
      .imgSize = param.size,
      .imgFormat = param.format,
      .bitstreamSize = 0,
      .secType = param.secType,
      .enableLog = MFALSE,
      .useSharedDeviceFd = -1,
      .ionDevice = ionDevice,
      .hwsync = MFALSE,
  });
  if (CC_UNLIKELY(!ret)) {
    CAM_ULOGME("onCreate imgSize:%dx%d format:%d buffer format:%d",
                param.size.w, param.size.h, param.format, staticInfo.format);
    delete pHeap;
    return nullptr;
  }

  // create shared_ptr & handle the onFirstRef & onLastStrongRef
  auto mDeleter = [](auto p) {
    p->_onLastStrongRef();
    delete p;
  };
  std::shared_ptr<GraphicImageBufferHeapBase> heapPtr(pHeap, mDeleter);

  // we need to call _onFirstRef after shared_ptr created
  heapPtr->_onFirstRef();

  return heapPtr;
}

/******************************************************************************
 *
 ******************************************************************************/
GraphicImageBufferHeapBase::GraphicImageBufferHeapBase(
    char const* szCallerName,
    BufferParams& rBufParams,
    ANativeWindowBuffer* pANWBuffer,
    MINT const acquire_fence,
    MINT const release_fence)
    : IGraphicImageBufferHeap(),
      BaseImageBufferHeap(szCallerName, "none")
      //
      ,
      mvHeapInfo(),
      mvBufInfo(),
      mGrallocInfo()
      //
      ,
      mBufferParams(rBufParams),
      mpANWBuffer(nullptr),
      mAcquireFence(acquire_fence),
      mReleaseFence(release_fence),
      isGrallocLocked(MFALSE) {
  mpANWBuffer = pANWBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
GraphicImageBufferHeapBase::impInit(BufInfoVect_t const& rvBufInfo) {
  mcam::GrallocStaticInfo staticInfo;
  NSCam::IGrallocHelper* const pGrallocHelper =
      NSCam::IGrallocHelper::singleton();
  //
  status_t status = OK;
  status = pGrallocHelper->setDirtyCamera(getBufferHandle());
  if (CC_UNLIKELY(OK != status)) {
    MY_LOGE("cannot set dirty - status:%d(%s)", status, ::strerror(-status));
    return MFALSE;
  }
  //
  status = pGrallocHelper->setColorspace_JFIF(getBufferHandle());
  if (CC_UNLIKELY(OK != status)) {
    MY_LOGE("cannot set colorspace JFIF - status:%d(%s)", status,
            ::strerror(-status));
    return MFALSE;
  }
  //
  Vector<int> const& ionFds = mGrallocInfo.ionFds;
  status = pGrallocHelper->query(getBufferHandle(), &staticInfo, &mGrallocInfo);
  if (CC_UNLIKELY(OK != status)) {
    MY_LOGE("cannot query the real format from buffer_handle_t - status:%d[%s]",
            status, ::strerror(status));
    return MFALSE;
  }
  //
  mvHeapInfo.setCapacity(getPlaneCount());
  mvBufInfo.setCapacity(getPlaneCount());

  // For hal3+, we allow app buffer which is Blob transformed into any fmt.
  if (staticInfo.format == HAL_PIXEL_FORMAT_BLOB) {
    // 1.handle stride
    for (auto i = 0 ; i < getPlaneCount(); i++) {
      if (mBufferParams.stride[i] == 0) {
        mBufferParams.stride[i] =
           (NSCam::Utils::Format::queryPlaneWidthInPixels(getImgFormat(),
            i, mBufferParams.width) *
            NSCam::Utils::Format::queryPlaneBitsPerPixel(getImgFormat(),
            i) + 7) / 8;
      } else if (helpCheckBufStrides(i, mBufferParams.stride[i]) == MFALSE) {
        return MFALSE;
      }
    }

    // 2. map PA
    HelperParamMapPA param;
    ::memset(&param, 0, sizeof(param));
    param.phyAddr = 0;
    param.virAddr = 0;
    param.ionFd = ionFds[0];
    param.size = staticInfo.planes[0].sizeInBytes;
    if (CC_UNLIKELY(!helpMapPhyAddr(__FUNCTION__, param))) {
      MY_LOGE("helpMapPhyAddr");
      return MFALSE;
    }
    MINTPTR pa = param.phyAddr;

    // fill buffer/heap information
    for (auto i = 0 ; i < getPlaneCount(); i++) {
      sp<MyHeapInfo> pHeapInfo = new MyHeapInfo;
      mvHeapInfo.push_back(pHeapInfo);
      pHeapInfo->heapID = ionFds[0];

      sp<BufInfo> pBufInfo = new BufInfo;
      mvBufInfo.push_back(pBufInfo);
      pBufInfo->stridesInBytes = mBufferParams.stride[i];
      pBufInfo->sizeInBytes = helpQueryBufSizeInBytes(i,
              pBufInfo->stridesInBytes);
      pBufInfo->pa = pa;
      pa += pBufInfo->sizeInBytes;

      rvBufInfo[i]->stridesInBytes = pBufInfo->stridesInBytes;
      rvBufInfo[i]->sizeInBytes = pBufInfo->sizeInBytes;
    }
  } else if (NSCam::eImgFmt_BLOB != getImgFormat()) {  // non-Blob
    if (ionFds.size() == 1) {
      size_t sizeInBytes = 0;
      for (size_t i = 0; i < staticInfo.planes.size(); i++) {
        sizeInBytes += staticInfo.planes[i].sizeInBytes;
      }
      //
      HelperParamMapPA param;
      ::memset(&param, 0, sizeof(param));
      param.phyAddr = 0;
      param.virAddr = 0;
      param.ionFd = ionFds[0];
      param.size = sizeInBytes;
      if (CC_UNLIKELY(!helpMapPhyAddr(__FUNCTION__, param))) {
        MY_LOGE("helpMapPhyAddr");
        return MFALSE;
      }
      //
      MINTPTR pa = param.phyAddr;
      for (size_t i = 0; i < getPlaneCount(); i++) {
        sp<MyHeapInfo> pHeapInfo = new MyHeapInfo;
        mvHeapInfo.push_back(pHeapInfo);
        pHeapInfo->heapID = ionFds[0];
        //
        sp<BufInfo> pBufInfo = new BufInfo;
        mvBufInfo.push_back(pBufInfo);
        // TODO(MTK) only use mBufferParams.stride[0]?
        pBufInfo->stridesInBytes = mBufferParams.stride[0] > 0
                                       ? mBufferParams.stride[0]
                                       : staticInfo.planes[i].rowStrideInBytes;
        pBufInfo->sizeInBytes = staticInfo.planes[i].sizeInBytes;
        pBufInfo->pa = pa;
        pa += pBufInfo->sizeInBytes;
        //
        rvBufInfo[i]->stridesInBytes = pBufInfo->stridesInBytes;
        rvBufInfo[i]->sizeInBytes = pBufInfo->sizeInBytes;
      }
    } else if (ionFds.size() == getPlaneCount()) {
      MY_LOGD_IF(0, "getPlaneCount():%zu", getPlaneCount());
      for (size_t i = 0; i < getPlaneCount(); i++) {
        HelperParamMapPA param;
        ::memset(&param, 0, sizeof(param));
        param.phyAddr = 0;
        param.virAddr = 0;
        param.ionFd = ionFds[i];
        param.size = staticInfo.planes[i].sizeInBytes;
        if (CC_UNLIKELY(!helpMapPhyAddr(__FUNCTION__, param))) {
          MY_LOGE("helpMapPhyAddr");
          return MFALSE;
        }
        MINTPTR pa = param.phyAddr;
        //
        sp<MyHeapInfo> pHeapInfo = new MyHeapInfo;
        mvHeapInfo.push_back(pHeapInfo);
        pHeapInfo->heapID = ionFds[i];
        //
        sp<BufInfo> pBufInfo = new BufInfo;
        mvBufInfo.push_back(pBufInfo);
        pBufInfo->stridesInBytes = staticInfo.planes[i].rowStrideInBytes;
        pBufInfo->sizeInBytes = staticInfo.planes[i].sizeInBytes;
        pBufInfo->pa = pa;
        //
        rvBufInfo[i]->stridesInBytes = pBufInfo->stridesInBytes;
        rvBufInfo[i]->sizeInBytes = pBufInfo->sizeInBytes;
      }
    } else {
      MY_LOGE("Unsupported ionFds:#%zu plane:#%zu", ionFds.size(),
              getPlaneCount());
      return MFALSE;
    }

  } else {  // TODO(MTK) this for blob fmt, maybe remove it...
    MSize imgSize = getImgSize();

    if (ionFds.size() != 1) {
      MY_LOGE("!!err: Blob should have ionFds.size=1 but actual-size=%zu",
              ionFds.size());
      return MFALSE;
    }

    size_t sizeInBytes = imgSize.w * imgSize.h;

    rvBufInfo[0]->stridesInBytes = sizeInBytes;
    rvBufInfo[0]->sizeInBytes = sizeInBytes;

    //
    HelperParamMapPA param;
    ::memset(&param, 0, sizeof(param));
    param.phyAddr = 0;
    param.virAddr = 0;
    param.ionFd = ionFds[0];
    param.size = sizeInBytes;
    if (CC_UNLIKELY(!helpMapPhyAddr(__FUNCTION__, param))) {
      MY_LOGE("helpMapPhyAddr");
      return MFALSE;
    }
    //
    MINTPTR pa = param.phyAddr;
    sp<MyHeapInfo> pHeapInfo = new MyHeapInfo;
    mvHeapInfo.push_back(pHeapInfo);
    pHeapInfo->heapID = ionFds[0];
    //
    sp<BufInfo> pBufInfo = new BufInfo;
    mvBufInfo.push_back(pBufInfo);
    pBufInfo->stridesInBytes = sizeInBytes;
    pBufInfo->sizeInBytes = sizeInBytes;
    pBufInfo->pa = pa;
    pa += pBufInfo->sizeInBytes;
    //
    rvBufInfo[0]->stridesInBytes = pBufInfo->stridesInBytes;
    rvBufInfo[0]->sizeInBytes = pBufInfo->sizeInBytes;
  }

  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
GraphicImageBufferHeapBase::impUninit(BufInfoVect_t const& /*rvBufInfo*/) {
  Vector<int> const& ionFds = mGrallocInfo.ionFds;
  //
  if (ionFds.size() == 1) {
    size_t sizeInBytes = 0;
    for (size_t i = 0; i < mvBufInfo.size(); i++) {
      sizeInBytes += mvBufInfo[i]->sizeInBytes;
    }
    //
    HelperParamMapPA param;
    ::memset(&param, 0, sizeof(param));
    param.phyAddr = (mvBufInfo.size() > 0) ? mvBufInfo[0]->pa : 0;
    param.virAddr = 0;
    param.ionFd = ionFds[0];
    param.size = sizeInBytes;
    if (CC_UNLIKELY(!helpUnmapPhyAddr(__FUNCTION__, param))) {
      MY_LOGE("helpUnmapPhyAddr");
    }
  } else if (ionFds.size() == getPlaneCount()) {
    for (size_t i = 0; i < getPlaneCount(); i++) {
      HelperParamMapPA param;
      ::memset(&param, 0, sizeof(param));
      param.phyAddr = mvBufInfo[i]->pa;
      param.virAddr = 0;
      param.ionFd = ionFds[i];
      param.size = mvBufInfo[i]->sizeInBytes;
      if (CC_UNLIKELY(!helpUnmapPhyAddr(__FUNCTION__, param))) {
        MY_LOGE("helpUnmapPhyAddr");
      }
    }
  } else {
    MY_LOGE("Unsupported ionFds:#%zu plane:#%zu", ionFds.size(),
            getPlaneCount());
  }
  //
  mvBufInfo.clear();
  mvHeapInfo.clear();
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
GraphicImageBufferHeapBase::impLockBuf(char const* szCallerName,
                                       MINT usage,
                                       BufInfoVect_t const& rvBufInfo) {
  void* vaddr = nullptr;
  if ((usage & (mcam::eBUFFER_USAGE_SW_MASK) != 0)) {
    isGrallocLocked = MTRUE;
    status_t status = NSCam::IGrallocHelper::singleton()->lock(
      mBufferParams.handle, mBufferParams.usage, mBufferParams.width_for_lock,
      mBufferParams.height_for_lock, &vaddr);
    if (CC_UNLIKELY(OK != status)) {
      MY_LOGE(
        "%s IGrallocHelper::lock - status:%d(%s) usage:(Req:%#x|Config:%#x) "
        "format:%#x w:%d h:%d",
        szCallerName, status, ::strerror(-status), usage, mBufferParams.usage,
        mBufferParams.format, mBufferParams.width_for_lock,
        mBufferParams.height_for_lock);
      return MFALSE;
    }
  }
  //
  //  SW Access.
  if (0 != (usage & (mcam::eBUFFER_USAGE_SW_MASK))) {
    MY_LOGF_IF(0 == vaddr,
               "SW Access but va=0 - usage:(Req:%#x|Config:%#x) format:%#x",
               usage, mBufferParams.usage, mBufferParams.format);
    MY_LOGF_IF(1 < mGrallocInfo.ionFds.size(), "[Not Implement] ionFds:#%zu>1",
               mGrallocInfo.ionFds.size());
  }
  MINTPTR va = reinterpret_cast<MINTPTR>(vaddr);
  for (size_t i = 0; i < getPlaneCount(); i++) {
    //  SW Access.
    if (0 != (usage & (mcam::eBUFFER_USAGE_SW_MASK))) {
      rvBufInfo[i]->va = va;
      va += mvBufInfo[i]->sizeInBytes;
    }
    //
    //  HW Access
    if (0 != (usage & (~mcam::eBUFFER_USAGE_SW_MASK))) {
      rvBufInfo[i]->pa = mvBufInfo[i]->pa;
    }
  }
  //
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL
GraphicImageBufferHeapBase::impUnlockBuf(char const* szCallerName,
                                         MINT usage,
                                         BufInfoVect_t const& rvBufInfo) {
  for (size_t i = 0; i < getPlaneCount(); i++) {
    rvBufInfo[i]->va = 0;
    rvBufInfo[i]->pa = 0;
  }
  //
  if (isGrallocLocked == MTRUE) {
    status_t status =
        NSCam::IGrallocHelper::singleton()->unlock(mBufferParams.handle);
    MY_LOGW_IF(OK != status,
               "%s GraphicBuffer::unlock - status:%d(%s) usage:%#x",
               szCallerName, status, ::strerror(-status), usage);
    isGrallocLocked = MFALSE;
  }
  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
SecureGraphicImageBufferHeap::SecureGraphicImageBufferHeap(
    char const* szCallerName,
    BufferParams& rBufParams,
    ANativeWindowBuffer* pANWBuffer,
    MINT const acquire_fence,
    MINT const release_fence)
    : GraphicImageBufferHeapBase(szCallerName,
                                 rBufParams,
                                 pANWBuffer,
                                 acquire_fence,
                                 release_fence) {
  getSecureHandle(mSecureHandle);
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL SecureGraphicImageBufferHeap::impLockBuf(char const* szCallerName,
                                               MINT usage,
                                               BufInfoVect_t const& rvBufInfo) {
  for (size_t i = 0; i < getPlaneCount(); i++) {
    if ((usage & mcam::eBUFFER_USAGE_SW_MASK) > 0)
      rvBufInfo[i]->va = mSecureHandle;

    if ((usage & mcam::eBUFFER_USAGE_HW_MASK) > 0)
      rvBufInfo[i]->pa = mSecureHandle;
  }

  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
MBOOL SecureGraphicImageBufferHeap::impUnlockBuf(
    char const* szCallerName,
    MINT usage,
    BufInfoVect_t const& rvBufInfo) {
  for (size_t i = 0; i < getPlaneCount(); i++) {
    rvBufInfo[i]->va = 0;
    rvBufInfo[i]->pa = 0;
  }

  return MTRUE;
}

/******************************************************************************
 *
 ******************************************************************************/
void SecureGraphicImageBufferHeap::getSecureHandle(SECHAND& secureHandle) {
  buffer_handle_t bufferHandle(mBufferParams.handle);
  MY_LOGA_IF(!bufferHandle, "invalid buffer handle");
  int retval = gralloc_extra_query(
      bufferHandle,
      GRALLOC_EXTRA_ATTRIBUTE_QUERY::GRALLOC_EXTRA_GET_SECURE_HANDLE,
      reinterpret_cast<void*>(&secureHandle));
  MY_LOGA_IF(retval != GRALLOC_EXTRA_OK, "get secure handle failed: FD(%u)",
             bufferHandle->data[0]);
}
