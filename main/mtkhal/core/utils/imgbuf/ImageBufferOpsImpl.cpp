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

#define LOG_TAG "MtkCam/ImageBufferOpsImpl"
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam/utils/imgbuf/IImageBufferOps.h>
#include <mtkcam/utils/std/ULog.h>

#include <ui/gralloc_extra.h>

#include <memory>
#include <vector>

CAM_ULOG_DECLARE_MODULE_ID(MOD_IMAGE_BUFFER);
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
/******************************************************************************
 *
 ******************************************************************************/

bool mcam::IImageBufferOps::update_SMVRInfo(
    const std::shared_ptr<mcam::IImageBuffer>& imageBuffer,
    const std::vector<uint64_t>& timestamps) {
  bool ret = false;
  mcam::IImageBufferHeap* heap = NULL;
  NSCam::IGraphicImageBufferHeap* graphicHeap = NULL;
  buffer_handle_t* handle = NULL;

  ge_smvr_info_t geInfo = {0};
  geInfo.frame_count = timestamps.size();
  const MUINT32 MAX = sizeof(geInfo.timestamp) / sizeof(geInfo.timestamp[0]);
  if (timestamps.size() > MAX) {
    MY_LOGW("timestamps's size(%d) > ge_smvr_info_t size(%d)",
            timestamps.size(), MAX);
  } else {
    for (MUINT32 i = 0, n = timestamps.size(); i < n; ++i) {
      geInfo.timestamp[i] = timestamps[i];
    }
  }

  if (imageBuffer) {
    heap = imageBuffer->getImageBufferHeap().get();
  }
  if (heap) {
    graphicHeap = NSCam::IGraphicImageBufferHeap::castFrom(heap);
  }
  if (graphicHeap) {
    handle = graphicHeap->getBufferHandlePtr();
  }
  if (handle == NULL) {
    MY_LOGW("Fail to cast GraphicBufferHeap: heap=%p graphicHeap=%p handle=%p",
            heap, graphicHeap, handle);
  } else {
    int result =
        gralloc_extra_perform(*handle, GRALLOC_EXTRA_SET_SMVR_INFO, &geInfo);
    if (result == 0) {
      ret = true;
    } else {
      MY_LOGW("gralloc_extra_perform(%d) != OK", result);
    }
  }

  return ret;
}
