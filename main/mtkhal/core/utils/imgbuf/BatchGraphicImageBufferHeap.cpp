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

#define LOG_TAG "MtkCam/BatchGraphicImageBufferHeap"
//
#include <mtkcam/utils/std/ULog.h>

#include <sync/sync.h>
#include <memory>
#include <vector>

#include "include/BatchGraphicImageBufferHeap.h"
#include "include/BaseImageBufferHeap.h"
#include "include/MyUtils.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);
//
// using namespace android;
// using namespace NSCam;
//

using NSCam::BatchGraphicImageBufferHeap;
using mcam::NSImageBufferHeap::BaseImageBufferHeap;
using mcam::IImageBuffer;
using NSCam::ImageBufferInfo;

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
 *
 ******************************************************************************/
BatchGraphicImageBufferHeap::BatchGraphicImageBufferHeap(
    char const* szCallerName,
    std::vector<std::shared_ptr<mcam::IImageBufferHeap>>& vHeap)
    : BaseImageBufferHeap(szCallerName, "none") {
  mvHeap = vHeap;
}

/******************************************************************************
 *
 ******************************************************************************/
std::vector<std::shared_ptr<mcam::IImageBuffer>>
BatchGraphicImageBufferHeap::createImageBuffers_FromBlobHeap(
    const NSCam::ImageBufferInfo& info __unused,
    const char* callerName __unused,
    MBOOL dirty_content __unused) {
  std::vector<std::shared_ptr<mcam::IImageBuffer>> vpImageBuffer;

  MINT32 bufCount = mvHeap.size();
  if (CC_UNLIKELY(getImgFormat() != NSCam::eImgFmt_BLOB)) {
    MY_LOGE("Heap format(0x%x) is illegal.", getImgFormat());
    return vpImageBuffer;
  }

  if (CC_UNLIKELY(bufCount == 0)) {
    MY_LOGE("buffer count is Zero");
    return vpImageBuffer;
  }
  // acquire fence, special case for AOSP SMVR to batch image
  auto waitAcquireFence = [&](auto& pImageHeap) -> int {
    int err = 0;
    if (pImageHeap->getAcquireFence() != -1) {
      int acquire_fence = ::dup(pImageHeap->getAcquireFence());
      if (acquire_fence < 0) {
        MY_LOGE("acquire_fence = %d < 0", acquire_fence);
        return -1;
      }
      err = ::sync_wait(acquire_fence, 1000);  // wait fence for 1000 ms
      if (err < 0) {
        MY_LOGW("sync_wait() failed: %s", strerror(errno));
        ::close(acquire_fence);
        return err;
      }
      ::close(acquire_fence);
      pImageHeap->setAcquireFence(-1);
    }
    return err;
  };
  for (MINT32 i = 0; i < bufCount; i++) {
    if (mvHeap[i] == nullptr || waitAcquireFence(mvHeap[i]) < 0) {
      MY_LOGE("waitAcquireFence fail");
      return vpImageBuffer;
    }
  }
  //
  vpImageBuffer.resize(bufCount);
  for (MINT32 i = 0; i < bufCount; i++) {
    vpImageBuffer[i] = mvHeap[i]->createImageBuffer();

    if (CC_UNLIKELY(vpImageBuffer[i] == nullptr)) {
      MY_LOGE("create ImageBuffer fail!!");
      // ensure to free vpImageBuffer since it's raw pointers
      for (size_t j = 0; j < vpImageBuffer.size(); j++) {
        vpImageBuffer[j] = 0;
      }
      vpImageBuffer.clear();
      return vpImageBuffer;
    }
  }

  return vpImageBuffer;
}
