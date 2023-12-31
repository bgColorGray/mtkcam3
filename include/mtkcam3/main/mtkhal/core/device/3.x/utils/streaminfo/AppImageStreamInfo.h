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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_STREAMINFO_APPIMAGESTREAMINFO_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_STREAMINFO_APPIMAGESTREAMINFO_H_
//
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include <mtkcam3/pipeline/stream/IStreamInfoBuilder.h>
//
#include <mtkcam/utils/debug/IPrinter.h>
#include <utils/String8.h>
//
#include <memory>
#include <string>
#include <vector>
/******************************************************************************
 *
 ******************************************************************************/
using NSCam::v3::IImageStreamInfo;
using NSCam::v3::IImageStreamInfoBuilder;
using NSCam::v3::IStreamInfoBuilderFactory;
using NSCam::ImageBufferInfo;
using NSCam::SecureInfo;
using NSCam::MSize;
using NSCam::IPrinter;
using NSCam::eImgFmt_UNKNOWN;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace Utils {

struct Camera3ImageStreamInfoBuilder;

/**
 * camera3 image stream info.
 */
class Camera3ImageStreamInfo : public IImageStreamInfo {
  friend Camera3ImageStreamInfoBuilder;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Definitions.
  struct CreationInfo {
    android::String8 mStreamName;
    BufPlanes_t mvbufPlanes;
    /**
     *  image format in reality.
     *
     *  If HalStream::overrideFormat is NOT set by HAL,
     *  then the request format == HalStream::overrideFormat;
     *  if not, the real format == HalStream::overrideFormat.
     */
    MINT mImgFormat = 0;

    /**
     *  original image format.
     *
     *  Always keep original image format of each stream id
     *  to reduce duplicate setting in static metadata.
     */
    MINT mOriImgFormat = 0;

    MINT mRealAllocFormat = 0;

    /** android/hardware/camera/device/3.x/types.h */
    Stream mStream;
    StreamId_T mStreamId;

    HalStream mHalStream;

    /**
     *  image buffer info at request stage
     */
    ImageBufferInfo mImageBufferInfo;
    MINT mSensorId = -1;

    /**
     *  Secure Info
     */
    SecureInfo mSecureInfo;
  };

 protected:  ////                    Data Members.
  CreationInfo mInfo;
  MINT mSensorId = -1;
  StreamId_T mStreamId;   /** Stream::id, HalStream::id */
  StreamId_T mBundleId = -1L; /** Stream::id, HalStream::id, for grouping */
  MUINT32 mTransform = 0; /** like Stream::rotation */
  /**
   * hardware/interfaces/camera/device/3.4/types.hal
   *
   *
   * Stream::usage
   *
   * The gralloc usage flags for this stream, as needed by the consumer of
   * the stream.
   *
   * The usage flags from the producer and the consumer must be combined
   * together and then passed to the platform gralloc HAL module for
   * allocating the gralloc buffers for each stream.
   *
   * The HAL may use these consumer flags to decide stream configuration. For
   * streamType INPUT, the value of this field is always 0. For all streams
   * passed via configureStreams(), the HAL must set its own
   * additional usage flags in its output HalStreamConfiguration.
   *
   * The usage flag for an output stream may be bitwise combination of usage
   * flags for multiple consumers, for the purpose of sharing one camera
   * stream between those consumers. The HAL must fail configureStreams call
   * with ILLEGAL_ARGUMENT if the combined flags cannot be supported due to
   * imcompatible buffer format, dataSpace, or other hardware limitations.
   *
   *
   * HalStream::producerUsage / HalStream::consumerUsage
   *
   * The gralloc usage flags for this stream, as needed by the HAL.
   *
   * For output streams, these are the HAL's producer usage flags. For input
   * streams, these are the HAL's consumer usage flags. The usage flags from
   * the producer and the consumer must be combined together and then passed
   * to the platform graphics allocator HAL for allocating the gralloc buffers
   * for each stream.
   *
   * If the stream's type is INPUT, then producerUsage must be 0, and
   * consumerUsage must be set. For other types, producerUsage must be set,
   * and consumerUsage must be 0.
   */
  MUINT64 mHalUsage = 0;
  MUINT64 mConsumerUsage = 0;  // shouldn't be defined

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  static auto castFrom(IStreamInfo const* pInfo)
      -> Camera3ImageStreamInfo const*;

 public:  ////                    Instantiation.
  explicit Camera3ImageStreamInfo(CreationInfo const& info);
  ///< copy constructor
  Camera3ImageStreamInfo(Camera3ImageStreamInfo const& other,
                         int imgFormat = eImgFmt_UNKNOWN,
                         size_t const stride = 0,
                         MSize size = MSize(0, 0));

 public:  ////                    Attributes.
  Stream const& getStream() const;
  virtual MSize getLandscapeSize() const;
  virtual void dumpState(IPrinter& printer, uint32_t indent = 0) const;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IImageStreamInfo Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////                    Attributes.
  /**
   * Usage for buffer consumer.
   */
  virtual MUINT64 getUsageForConsumer() const;

  /**
   * Usage for buffer allocator.
   *
   * @remark It must equal to the gralloc usage passed to the platform
   *  graphics allocator HAL for allocating the gralloc buffers.
   */
  virtual MUINT64 getUsageForAllocator() const;

  virtual MINT getImgFormat() const;
  virtual MINT getOriImgFormat() const;
  virtual MSize getImgSize() const;
  virtual BufPlanes_t const& getBufPlanes() const;
  virtual MUINT32 getTransform() const;
  virtual int setTransform(MUINT32 transform);
  virtual MUINT32 getDataSpace() const;
  virtual BufPlanes_t const& getAllocBufPlanes() const;
  virtual MINT getAllocImgFormat() const;
  virtual ImageBufferInfo const& getImageBufferInfo() const;
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  IStreamInfo Interface.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Attributes.
  virtual auto getNativeHandle() const -> void const* { return this; }

 public:  ////                    Attributes.
  virtual char const* getStreamName() const;
  virtual StreamId_T getStreamId() const;
  virtual StreamId_T getBundleId() const;
  virtual MUINT32 getStreamType() const;
  virtual size_t getMaxBufNum() const;
  virtual MVOID setMaxBufNum(size_t count);
  virtual size_t getMinInitBufNum() const;
  virtual MBOOL isSecure() const;
  virtual SecureInfo const& getSecureInfo() const;
  virtual auto toString() const -> android::String8;
//  virtual auto toString8() const -> android::String8;
  virtual MINT getPhysicalCameraId() const;
};

/**
 * image stream info builder.
 */
struct Camera3ImageStreamInfoBuilder : public IImageStreamInfoBuilder {
  android::sp<Camera3ImageStreamInfo> mStreamInfo;
  explicit Camera3ImageStreamInfoBuilder(IImageStreamInfo const* info);
  virtual auto build() const -> android::sp<IImageStreamInfo>;
  virtual void setStreamId(StreamId_T streamId);
  virtual void setAllocImgFormat(int format);
  virtual void setAllocBufPlanes(BufPlanesT&& bufPlanes);
  virtual void setAllocBufPlanes(BufPlanesT const& bufPlanes);
  virtual void setImageBufferInfo(ImageBufferInfo&& info);
  virtual void setImageBufferInfo(ImageBufferInfo const& info);
  virtual void setBundleId(StreamId_T const id);
  virtual void setImgSize(MSize const& imgSize);
  virtual void setImgFormat(int format);
  virtual void setBufPlanes(BufPlanesT&& bufPlanes);
  virtual void setBufPlanes(BufPlanesT const& bufPlanes);

  virtual void setStreamName(std::string&& name __unused) {}  /* not support */
  virtual void setStreamType(uint32_t streamType __unused) {} /* not support */
  virtual void setSecureInfo(SecureInfo const& secureInfo __unused) {
  }                                                       /* not support */
  virtual void setMaxBufNum(size_t maxBufNum __unused) {} /* not support */
  virtual void setMinInitBufNum(size_t minInitBufNum __unused) {
  }                                                           /* not support */
  virtual void setUsageForAllocator(uint_t usage __unused) {} /* not support */
  virtual void setBufCount(size_t count __unused) {}          /* not support */
  virtual void setBufStep(size_t step __unused) {}            /* not support */
  virtual void setBufStartOffset(size_t offset __unused) {}   /* not support */
  virtual void setDataSpace(uint32_t dataspace __unused) {}   /* not support */
  virtual void setTransform(uint32_t transform __unused) {}   /* not support */
//  virtual void setPrivateData(SetPrivateData const& arg __unused) {
//  } /* not support */
};

struct Camera3StreamInfoBuilderFactory : public IStreamInfoBuilderFactory {
  virtual auto makeImageStreamInfoBuilder() const
      -> std::shared_ptr<IImageStreamInfoBuilder> {
    return nullptr; /* not support */
  }

  virtual auto makeImageStreamInfoBuilder(IImageStreamInfo const* info) const
      -> std::shared_ptr<IImageStreamInfoBuilder> {
    return std::make_shared<Camera3ImageStreamInfoBuilder>(info);
  }
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace Utils
};      // namespace core
};      // namespace mcam
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_STREAMINFO_APPIMAGESTREAMINFO_H_
