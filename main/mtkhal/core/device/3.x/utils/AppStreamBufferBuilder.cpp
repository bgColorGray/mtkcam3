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

#include "include/AppStreamBufferBuilder.h"
#include "MyUtils.h"
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/ConvertImageBuffer.h>

#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/drv/IHalSensor.h>

#include <utils/RefBase.h>
#include <memory>
#include <string>

using NSCam::Utils::ULog::DetailsType;
using NSCam::Utils::ULog::ULogPrinter;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
                    CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)

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
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

#define VALIDATE_STRING(string) (string != nullptr ? string : "UNKNOWN")

using NSCam::SENSOR_FORMAT_ORDER_RAW_R;
using NSCam::SENSOR_FORMAT_ORDER_RAW_Gr;
using NSCam::SENSOR_FORMAT_ORDER_RAW_Gb;
using NSCam::SENSOR_FORMAT_ORDER_RAW_B;
using NSCam::IMetadata;
//using NSCam::IIonDevice;
using NSCam::IImageBufferAllocator;
using NSCam::eImgFmt_JPEG;
using NSCam::eImgFmt_BLOB;
using NSCam::eImgFmt_BLOB_START;
using NSCam::eImgFmt_JPEG_APP_SEGMENT;
using NSCam::v3::eSTREAMTYPE_IMAGE_IN;
//using NSCam::IImageBufferHeap;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace Utils {

static auto convertToSensorColorOrder(uint8_t colorFilterArrangement)
    -> int32_t {
  switch (colorFilterArrangement) {
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
      return (int32_t)SENSOR_FORMAT_ORDER_RAW_R;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
      return (int32_t)SENSOR_FORMAT_ORDER_RAW_Gr;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
      return (int32_t)SENSOR_FORMAT_ORDER_RAW_Gb;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
      return (int32_t)SENSOR_FORMAT_ORDER_RAW_B;
    default:
      break;
  }
  MY_LOGE("Unsupported Color Filter Arrangement:%d", colorFilterArrangement);
  return -1;
}

/******************************************************************************
 *
 ******************************************************************************/
auto makeAppImageStreamBuffer(BuildImageStreamBufferInputParams const& in)
    -> ::android::sp<AppImageStreamBuffer> {
  MY_LOGD("StreamBuffer:%p streamId:%d bufferId:%" PRIu64
          " status:%d handle:%p",
          &in.streamBuffer, in.streamBuffer.streamId, in.streamBuffer.bufferId,
          in.streamBuffer.status, in.streamBuffer.buffer.get());

  if (in.streamBuffer.streamId == -1 || in.streamBuffer.bufferId == 0) {
    MY_LOGE("invalid streamBuffer streamId:%d bufferId:%" PRIu64 " handle:%p",
            in.streamBuffer.streamId, in.streamBuffer.bufferId,
            in.streamBuffer.buffer.get());
    return nullptr;
  }
  //
  if (in.pStreamInfo == nullptr) {
    MY_LOGE(
        "Null ImageStreamInfo, failed to allocate streamBuffer, return null");
    return nullptr;
  }
  // mcam::IImageBufferHeap ==> NSCam::IImageBufferHeap
  ::android::sp<NSCam::IImageBufferHeap> pHeap = mcam::convertImageBufferHeap(in.streamBuffer.buffer);
  //
  auto pStreamBuffer =
     AppImageStreamBuffer::Allocator(const_cast<IImageStreamInfo*>(
          in.pStreamInfo))(in.streamBuffer.bufferId, in.streamBuffer.buffer, pHeap.get());

  if (pStreamBuffer == nullptr) {
    ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG,
                           DetailsType::DETAILS_DEBUG,
                           "createImageStreamBuffer-Fail");
    MY_LOGE(
        "fail to create AppImageStreamBuffer - streamId:%u bufferId:%" PRIu64
        " ",
        in.streamBuffer.streamId, in.streamBuffer.bufferId);
    return nullptr;
  }
  return pStreamBuffer;
}
};  // namespace Utils
};  // namespace core
};  // namespace mcam
