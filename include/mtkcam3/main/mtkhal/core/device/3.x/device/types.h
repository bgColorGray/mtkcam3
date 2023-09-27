/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_TYPES_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_TYPES_H_

#include <mtkcam3/main/mtkhal/core/device/3.x/utils/streambuffer/AppStreamBuffers.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/utils/streaminfo/AppImageStreamInfo.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/policy/types.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/def/UITypes.h>
#include <mtkcam3/pipeline/stream/IStreamBufferProvider.h>
#include <mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h>
//#include <mtkcam-interfaces/pipeline/utils/stream/Factory.h>

#include <utils/KeyedVector.h>
#include <utils/RefBase.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

using NSCam::v3::IMetaStreamBuffer;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {

using AppImageStreamInfo = mcam::core::Utils::Camera3ImageStreamInfo;
using AppMetaStreamInfo = NSCam::v3::Utils::MetaStreamInfo;

typedef android::KeyedVector<StreamId_T, android::sp<AppImageStreamInfo>>
    ImageConfigMap;
typedef android::KeyedVector<StreamId_T, android::sp<AppMetaStreamInfo>>
    MetaConfigMap;

enum HalStatus {
  HAL_OK = 0,
  HAL_NO_ERROR = 0,
  HAL_UNKNOWN_ERROR = (-2147483647 - 1),  // INT32_MIN value
  HAL_NO_MEMORY           = -ENOMEM,
  HAL_INVALID_OPERATION   = -ENOSYS,
  HAL_BAD_VALUE           = -EINVAL,
  HAL_BAD_TYPE            = (HAL_UNKNOWN_ERROR + 1),
  HAL_NAME_NOT_FOUND      = -ENOENT,
  HAL_PERMISSION_DENIED   = -EPERM,
  HAL_NO_INIT             = -ENODEV,
  HAL_ALREADY_EXISTS      = -EEXIST,
  HAL_DEAD_OBJECT         = -EPIPE,
};

struct Request {
  /*****
   * Assigned by App Stream Manager.
   */

  /**
   * @param frame number.
   */
  uint32_t requestNo;

  /**
   * @param input image stream buffers.
   */
  android::KeyedVector<StreamId_T, android::sp<Utils::AppImageStreamBuffer>>
      vInputImageBuffers;

  /**
   * @param output image stream buffers.
   */
  android::KeyedVector<StreamId_T, android::sp<Utils::AppImageStreamBuffer>>
      vOutputImageBuffers;

  /**
   * @param input image stream info.
   */
  android::KeyedVector<StreamId_T, android::sp<IImageStreamInfo>>
      vInputImageStreams;

  /**
   * @param output image stream info.
   */
  android::KeyedVector<StreamId_T, android::sp<IImageStreamInfo>>
      vOutputImageStreams;

  /**
   * @param input meta stream buffers.
   */
  android::KeyedVector<StreamId_T, android::sp<IMetaStreamBuffer>>
      vInputMetaBuffers;

  /**
   * please refer to pipeline/model/Types.h
   */
  std::unordered_map<StreamId_T, uint8_t> streamImageProcessingSettings;
  /**
   * please refer to pipeline/model/Types.h
   */
  std::map<StreamId_T, NSCam::MRect> streamCropSettings;
};

using ConfigAppStreams = mcam::core::device::policy::AppUserConfiguration;

struct PhysicalResult : public android::RefBase {
  /**
   * @param[in] physic camera id
   */
  std::string physicalCameraId;

  /**
   * @param[in] physic result partial metadata to update.
   */
  android::sp<IMetaStreamBuffer> physicalResultMeta;
};

struct UpdateResultParams {
  /**
   * @param[in] the frame number to update.
   */
  uint32_t requestNo = 0;

  /**
   * @param[in] user id.
   *  In fact, it is pipeline node id from the viewpoint of pipeline
   * implementation, but the pipeline users have no such knowledge.
   */
  intptr_t userId = 0;

  /**
   * @param[in] hasLastPartial: contain last partial metadata in result partial
   * metadata vector.
   */
  bool hasLastPartial = false;

  /**
   * @param[in] sensorTimestamp: sensor timestamp for notify framework
   */
  int64_t shutterTimestamp = 0;

  /**
   * @param[in] timestampStartOfFrame: referenced by De-jitter mechanism.
   */
  int64_t timestampStartOfFrame = 0;

  /**
   * @param[in] hasRealTimePartial: contain real-time partial metadata in result
   * partial metadata vector. If true, then the result would be callbacked
   * instantly. Or the result callback would be pending util the next real-time
   * result arrives (or the last result arrives)
   */
  bool hasRealTimePartial = false;

  /**
   * @param[in] result partial metadata to update.
   */
  android::Vector<android::sp<IMetaStreamBuffer>> resultMeta;

  /**
   * @param[in] result partial hal metadata to update.
   */
  android::Vector<android::sp<IMetaStreamBuffer>> resultHalMeta;

  /**
   * @param[in] phsycial result to update.
   */
  android::Vector<android::sp<PhysicalResult>> physicalResult;

  // [HAL3+] onResultMetadataFailed
  /**
   * @brief[in] callback for meta is error
   */
  bool hasResultMetaError = false;

  // [HAL3+] I/F for image callback
  /**
   * @param[in] result image to update
   */
  android::Vector<StreamBuffer> outputResultImages;
  android::Vector<StreamBuffer> inputResultImages;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace core
};      // namespace mcam
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_DEVICE_TYPES_H_
