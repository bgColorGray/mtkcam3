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

#ifndef MAIN_MTKHAL_HIDL_INCLUDE_TYPESWRAPPER_H_
#define MAIN_MTKHAL_HIDL_INCLUDE_TYPESWRAPPER_H_
//
#include <android/hardware/camera/device/3.4/types.h>
#include <android/hardware/camera/device/3.5/types.h>
#include <android/hardware/camera/device/3.6/types.h>
#include <typeinfo>

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace hidl {

struct WrappedStream;
struct WrappedStreamConfiguration;
struct WrappedHalStream;
struct WrappedHalStreamConfiguration;
struct WrappedCaptureRequest;
struct WrappedCaptureResult;

struct WrappedStream {
  explicit WrappedStream(
      const ::android::hardware::camera::device::V3_2::Stream& p) {
    __v34Instance.v3_2 = p;
  }

  explicit WrappedStream(
      const ::android::hardware::camera::device::V3_4::Stream& p) {
    __v34Instance = p;
  }

  operator ::android::hardware::camera::device::V3_2::Stream() const {
    return __v34Instance.v3_2;
  }
  operator ::android::hardware::camera::device::V3_2::Stream&() {
    return __v34Instance.v3_2;
  }

  operator ::android::hardware::camera::device::V3_4::Stream() const {
    return __v34Instance;
  }
  operator ::android::hardware::camera::device::V3_4::Stream&() {
    return __v34Instance;
  }

 private:
  ::android::hardware::camera::device::V3_4::Stream __v34Instance;
};

struct WrappedStreamConfiguration {
 public:
  WrappedStreamConfiguration(
      const ::android::hardware::camera::device::V3_2::StreamConfiguration& p) {
    __v35Instance.v3_4.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::Stream>(p.streams.size());
    int i = 0;
    for (auto& s : p.streams) {
      (::android::hardware::camera::device::V3_4::Stream)
          __v35Instance.v3_4.streams[i++] =
          (::android::hardware::camera::device::V3_4::Stream&)WrappedStream(s);
    }
    __v35Instance.v3_4.operationMode = p.operationMode;
    __v35Instance.streamConfigCounter = 0;
  }

  WrappedStreamConfiguration(
      const ::android::hardware::camera::device::V3_4::StreamConfiguration& p) {
    __v35Instance.v3_4 = p;
    __v35Instance.streamConfigCounter = 0;
  }

  WrappedStreamConfiguration(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration& p) {
    __v35Instance = p;
  }

  operator ::android::hardware::camera::device::V3_2::StreamConfiguration()
      const {
    ::android::hardware::camera::device::V3_2::StreamConfiguration v32Instance;
    v32Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::Stream>(
        __v35Instance.v3_4.streams.size());
    int i = 0;
    for (auto& s : __v35Instance.v3_4.streams) {
      v32Instance.streams[i++] =
          (::android::hardware::camera::device::V3_2::Stream&)WrappedStream(s);
    }
    v32Instance.operationMode = __v35Instance.v3_4.operationMode;
    return v32Instance;
  }

  operator ::android::hardware::camera::device::V3_2::StreamConfiguration&() {
    __v32Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::Stream>(
        __v35Instance.v3_4.streams.size());
    int i = 0;
    for (auto& s : __v35Instance.v3_4.streams) {
      __v32Instance.streams[i++] =
          (::android::hardware::camera::device::V3_2::Stream&)WrappedStream(s);
    }
    __v32Instance.operationMode = __v35Instance.v3_4.operationMode;
    return __v32Instance;
  }

  operator ::android::hardware::camera::device::V3_4::StreamConfiguration()
      const {
    return __v35Instance.v3_4;
  }
  operator ::android::hardware::camera::device::V3_4::StreamConfiguration&() {
    return __v35Instance.v3_4;
  }

  operator ::android::hardware::camera::device::V3_5::StreamConfiguration()
      const {
    return __v35Instance;
  }
  operator ::android::hardware::camera::device::V3_5::StreamConfiguration&() {
    return __v35Instance;
  }

 private:
  ::android::hardware::camera::device::V3_5::StreamConfiguration __v35Instance;
  ::android::hardware::camera::device::V3_2::StreamConfiguration __v32Instance;
};

struct WrappedHalStream {
  WrappedHalStream(
      const ::android::hardware::camera::device::V3_2::HalStream& p) {
    __v36Instance.v3_4.v3_3.v3_2 = p;

    // No value:
    // __v34Instance.v3_3.overrideDataSpace
    // __v34Instance.physicalCameraId
  }

  WrappedHalStream(
      const ::android::hardware::camera::device::V3_3::HalStream& p) {
    __v36Instance.v3_4.v3_3 = p;
  }

  WrappedHalStream(
      const ::android::hardware::camera::device::V3_4::HalStream& p) {
    __v36Instance.v3_4 = p;
  }

  WrappedHalStream(
      const ::android::hardware::camera::device::V3_6::HalStream& p) {
    __v36Instance = p;
  }

  operator ::android::hardware::camera::device::V3_2::HalStream() const {
    return __v36Instance.v3_4.v3_3.v3_2;
  }
  operator ::android::hardware::camera::device::V3_2::HalStream&() {
    return __v36Instance.v3_4.v3_3.v3_2;
  }

  operator ::android::hardware::camera::device::V3_3::HalStream() const {
    return __v36Instance.v3_4.v3_3;
  }
  operator ::android::hardware::camera::device::V3_3::HalStream&() {
    return __v36Instance.v3_4.v3_3;
  }

  operator ::android::hardware::camera::device::V3_4::HalStream() const {
    return __v36Instance.v3_4;
  }
  operator ::android::hardware::camera::device::V3_4::HalStream&() {
    return __v36Instance.v3_4;
  }

  operator ::android::hardware::camera::device::V3_6::HalStream() const {
    return __v36Instance;
  }
  operator ::android::hardware::camera::device::V3_6::HalStream&() {
    return __v36Instance;
  }

 private:
  ::android::hardware::camera::device::V3_6::HalStream __v36Instance;
};

struct WrappedHalStreamConfiguration {
  WrappedHalStreamConfiguration() {}

  WrappedHalStreamConfiguration(
      const ::android::hardware::camera::device::V3_2::HalStreamConfiguration&
          p) {
    __v36Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_6::HalStream>(p.streams.size());
    int i = 0;
    for (auto& s : p.streams) {
      __v36Instance.streams[i++] =
          (::android::hardware::camera::device::V3_6::HalStream&)
              WrappedHalStream(s);
    }
  }

  WrappedHalStreamConfiguration(
      ::android::hardware::camera::device::V3_2::HalStreamConfiguration& p) {
    __v36Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_6::HalStream>(p.streams.size());
    int i = 0;
    for (auto& s : p.streams) {
      __v36Instance.streams[i++] =
          (::android::hardware::camera::device::V3_6::HalStream&)
              WrappedHalStream(s);
    }
  }

  WrappedHalStreamConfiguration(
      const ::android::hardware::camera::device::V3_4::HalStreamConfiguration&
          p) {
    __v36Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_6::HalStream>(p.streams.size());
    int i = 0;
    for (auto& s : p.streams) {
      __v36Instance.streams[i++] =
          (::android::hardware::camera::device::V3_6::HalStream&)
              WrappedHalStream(s);
    }
  }

  WrappedHalStreamConfiguration(
      const ::android::hardware::camera::device::V3_6::HalStreamConfiguration&
          p) {
    __v36Instance = p;
  }

  operator ::android::hardware::camera::device::V3_2::HalStreamConfiguration()
      const {
    ::android::hardware::camera::device::V3_2::HalStreamConfiguration
        v32Instance;
    v32Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::HalStream>(
        __v36Instance.streams.size());
    int i = 0;
    for (auto& s : __v36Instance.streams) {
      v32Instance.streams[i++] =
          (::android::hardware::camera::device::V3_2::HalStream&)
              WrappedHalStream(s);
    }

    return v32Instance;
  }

  operator ::android::hardware::camera::device::V3_2::
      HalStreamConfiguration&() {
    __v32Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_2::HalStream>(
        __v36Instance.streams.size());
    int i = 0;
    for (auto& s : __v36Instance.streams) {
      __v32Instance.streams[i++] =
          (::android::hardware::camera::device::V3_2::HalStream&)
              WrappedHalStream(s);
    }

    return __v32Instance;
  }

  operator ::android::hardware::camera::device::V3_3::HalStreamConfiguration()
      const {
    ::android::hardware::camera::device::V3_3::HalStreamConfiguration
        v33Instance;
    v33Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_3::HalStream>(
        __v36Instance.streams.size());
    int i = 0;
    for (auto& s : __v36Instance.streams) {
      v33Instance.streams[i++] =
          (::android::hardware::camera::device::V3_3::HalStream&)
              WrappedHalStream(s);
    }

    return v33Instance;
  }

  operator ::android::hardware::camera::device::V3_3::
      HalStreamConfiguration&() {
    __v33Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_3::HalStream>(
        __v36Instance.streams.size());
    int i = 0;
    for (auto& s : __v36Instance.streams) {
      __v33Instance.streams[i++] =
          (::android::hardware::camera::device::V3_3::HalStream&)
              WrappedHalStream(s);
    }

    return __v33Instance;
  }

  operator ::android::hardware::camera::device::V3_4::HalStreamConfiguration()
      const {
    ::android::hardware::camera::device::V3_4::HalStreamConfiguration
        v34Instance;
    v34Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::HalStream>(
        __v36Instance.streams.size());
    int i = 0;
    for (auto& s : __v36Instance.streams) {
      v34Instance.streams[i++] =
          (::android::hardware::camera::device::V3_4::HalStream&)
              WrappedHalStream(s);
    }

    return v34Instance;
  }

  operator ::android::hardware::camera::device::V3_4::
      HalStreamConfiguration&() {
    __v34Instance.streams = ::android::hardware::hidl_vec<
        ::android::hardware::camera::device::V3_4::HalStream>(
        __v36Instance.streams.size());
    int i = 0;
    for (auto& s : __v36Instance.streams) {
      __v34Instance.streams[i++] =
          (::android::hardware::camera::device::V3_4::HalStream&)
              WrappedHalStream(s);
    }

    return __v34Instance;
  }

  operator ::android::hardware::camera::device::V3_6::HalStreamConfiguration()
      const {
    return __v36Instance;
  }
  operator ::android::hardware::camera::device::V3_6::
      HalStreamConfiguration&() {
    return __v36Instance;
  }

 private:
  ::android::hardware::camera::device::V3_6::HalStreamConfiguration
      __v36Instance;
  ::android::hardware::camera::device::V3_4::HalStreamConfiguration
      __v34Instance;
  ::android::hardware::camera::device::V3_3::HalStreamConfiguration
      __v33Instance;
  ::android::hardware::camera::device::V3_2::HalStreamConfiguration
      __v32Instance;
};

struct WrappedCaptureRequest {
  WrappedCaptureRequest(
      const ::android::hardware::camera::device::V3_2::CaptureRequest& p) {
    __v34Instance.v3_2 = p;
  }

  WrappedCaptureRequest(
      const ::android::hardware::camera::device::V3_4::CaptureRequest& p) {
    __v34Instance = p;
  }

  operator ::android::hardware::camera::device::V3_2::CaptureRequest() const {
    return __v34Instance.v3_2;
  }
  operator ::android::hardware::camera::device::V3_2::CaptureRequest&() {
    return __v34Instance.v3_2;
  }

  operator ::android::hardware::camera::device::V3_4::CaptureRequest() const {
    return __v34Instance;
  }
  operator ::android::hardware::camera::device::V3_4::CaptureRequest&() {
    return __v34Instance;
  }

 private:
  ::android::hardware::camera::device::V3_4::CaptureRequest __v34Instance;
};

struct WrappedCaptureResult {
  WrappedCaptureResult(
      const ::android::hardware::camera::device::V3_2::CaptureResult& p) {
    __v34Instance.v3_2 = p;
  }

  WrappedCaptureResult(
      const ::android::hardware::camera::device::V3_4::CaptureResult& p) {
    __v34Instance = p;
  }

  operator ::android::hardware::camera::device::V3_2::CaptureResult() const {
    return __v34Instance.v3_2;
  }
  operator ::android::hardware::camera::device::V3_2::CaptureResult&() {
    return __v34Instance.v3_2;
  }

  operator ::android::hardware::camera::device::V3_4::CaptureResult() const {
    return __v34Instance;
  }
  operator ::android::hardware::camera::device::V3_4::CaptureResult&() {
    return __v34Instance;
  }

 private:
  ::android::hardware::camera::device::V3_4::CaptureResult __v34Instance;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace hidl
};      // namespace mcam
#endif  // MAIN_MTKHAL_HIDL_INCLUDE_TYPESWRAPPER_H_
