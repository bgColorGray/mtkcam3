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

#ifndef MAIN_MTKHAL_HIDL_INCLUDE_IBUFFERHANDLECACHEMGR_H_
#define MAIN_MTKHAL_HIDL_INCLUDE_IBUFFERHANDLECACHEMGR_H_

#include "HidlCameraDevice.h"

#include <cutils/native_handle.h>
#include <mtkcam3/main/mtkhal/android/device/3.x/types.h>
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>

#include <memory>
#include <vector>

typedef int32_t StreamId_T;

using ::android::hardware::camera::device::V3_2::BufferCache;
using ::android::hardware::camera::device::V3_4::CaptureRequest;
using ::android::hardware::camera::device::V3_5::StreamConfiguration;

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace hidl {

// BufferHandle for createGraphicImageBuffer
struct BufferHandle {
  uint64_t bufferId = 0;
  buffer_handle_t bufferHandle = nullptr;
  std::shared_ptr<mcam::android::AppBufferHandleHolder>
      appBufferHandleHolder = nullptr;
  int32_t ionFd = -1;
  struct timespec cachedTime = {};
};

struct RequestBufferCache {
  uint32_t frameNumber;
  ::android::KeyedVector<StreamId_T, BufferHandle> bufferHandleMap;
};

/**
 * An interface of buffer handle cache.
 */
class IBufferHandleCacheMgr : public virtual ::android::RefBase {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  static auto create() -> IBufferHandleCacheMgr*;

  virtual auto destroy() -> void = 0;

  virtual auto configBufferCacheMap(
      const StreamConfiguration& requestedConfiguration) -> void = 0;

  virtual auto removeBufferCache(const hidl_vec<BufferCache>& cachesToRemove)
      -> void = 0;

  virtual auto registerBufferCache(
      const hidl_vec<CaptureRequest>& requests,
      std::vector<RequestBufferCache>& vBufferCache) -> void = 0;

  // virtual auto    markBufferFreeByOthers(
  //                     std::vector<mcam::android::CaptureResult>&
  //                     results
  //                 )   -> void                                             =
  //                 0;

  virtual auto dumpState() -> void = 0;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace hidl
};      // namespace mcam
#endif  // MAIN_MTKHAL_HIDL_INCLUDE_IBUFFERHANDLECACHEMGR_H_
