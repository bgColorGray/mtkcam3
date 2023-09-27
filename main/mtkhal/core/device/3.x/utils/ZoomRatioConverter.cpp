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

#define LOG_TAG "MtkCam/ZoomRatioConverter"
#include "ZoomRatioConverter.h"
#include <mtkcam/utils/std/ULog.h>  // will include <log/log.h> if LOG_TAG was defined

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)

#define MY_LOGV_LV1(...)    \
  do {                      \
    if ((mLogLv > 0)) {     \
      MY_LOGV(__VA_ARGS__); \
    }                       \
  } while (0)
#define MY_LOGD_LV1(...)    \
  do {                      \
    if ((mLogLv > 0)) {     \
      MY_LOGD(__VA_ARGS__); \
    }                       \
  } while (0)
#define MY_LOGI_LV1(...)    \
  do {                      \
    if ((mLogLv > 0)) {     \
      MY_LOGI(__VA_ARGS__); \
    }                       \
  } while (0)
#define MY_LOGW_LV1(...)    \
  do {                      \
    if ((mLogLv > 0)) {     \
      MY_LOGW(__VA_ARGS__); \
    }                       \
  } while (0)
#define MY_LOGE_LV1(...)    \
  do {                      \
    if ((mLogLv > 0)) {     \
      MY_LOGE(__VA_ARGS__); \
    }                       \
  } while (0)

using NSCam::MSize;
using NSCam::MRect;
using NSCam::MPoint;
using NSCam::TYPE_MINT32;
using NSCam::TYPE_MSize;
using NSCam::TYPE_MRect;
using NSCam::TYPE_MPoint;
using NSCam::Type2Type;
using NSCam::v3::STREAM_BUFFER_STATUS;

namespace mcam {
namespace core {

auto ZoomRatioConverter::Create(const bool supportZoomRatio,
                                Dimension d,
                                DimensionMap map)
    -> std::shared_ptr<ZoomRatioConverter> {
  int32_t logLv =
      ::property_get_int32("vendor.debug.camera.log.ZoomRatioConverter", 0);
  MY_LOGD("Create ZoomRatioConverter support(%d), logLv(%d)", supportZoomRatio,
          logLv);
  return std::make_shared<ZoomRatioConverter>(supportZoomRatio, d, map, logLv);
}

auto ZoomRatioConverter::UpdateCaptureRequestSingleMeta(
    const uint32_t reqNum,
    StreamId_T streamId,
    android::sp<IMetaStreamBuffer> pMetaBuffer) -> void {
  if (!mSupportZoomRatio) {
    UpdateCropRegionWithoutConvert(pMetaBuffer, true);
    return;
  }

  Dimension d;
  if (!GetDimensionByMetaBuffer(-1 /*logical*/, d /*out*/)) {
    MY_LOGE("Get dimension failed, R%d stream(%s)", reqNum,
            pMetaBuffer->getName());
    return;
  }

  IMetadata* pMetadata = pMetaBuffer->tryWriteLock(LOG_TAG);

  if (pMetadata != nullptr) {
    IMetadata::IEntry entry = pMetadata->entryFor(MTK_CONTROL_ZOOM_RATIO);
    if (!entry.isEmpty()) {
      float zoom_ratio = entry.itemAt(0, Type2Type<MFLOAT>());

      if (zoom_ratio > 0) {
        {
          std::lock_guard<std::mutex> lock(mMapLock);
          mReqRatioMap.emplace(reqNum, zoom_ratio);
          MY_LOGD_LV1("Set: R%d, stream(%s), zoom_ratio(%f), size(%d)", reqNum,
                      pMetaBuffer->getName(), zoom_ratio, mReqRatioMap.size());
        }
        ApplyZoomRatio(pMetadata, zoom_ratio, d, true, false);
      } else {
        MY_LOGE("Invalid zoom ratio(%f)", zoom_ratio);
      }
    } else {
      MY_LOGD_LV1("CONTROL_ZOOM_RATIO not found");
    }

    pMetaBuffer->unlock(LOG_TAG, pMetadata);
  } else {
    MY_LOGE("Get pMetadata failed: R%d, stream(%s)", reqNum,
            pMetaBuffer->getName());
  }
}

auto ZoomRatioConverter::UpdateCaptureRequestMeta(
    const uint32_t reqNum,
    std::unordered_map<StreamId_T, android::sp<IMetaStreamBuffer>>&
        requestMetaMap) -> void {
  if (!mSupportZoomRatio) {
    for (auto && [ streamId, pMetaBuffer ] : requestMetaMap)
      UpdateCropRegionWithoutConvert(pMetaBuffer, true);

    return;
  }

  for (auto && [ streamId, pMetaBuffer ] : requestMetaMap) {
    Dimension d;
    if (!GetDimensionByMetaBuffer(-1 /*logical*/, d /*out*/)) {
      MY_LOGE("Get dimension failed, R%d stream(%s)", reqNum,
              pMetaBuffer->getName());
      continue;
    }

    IMetadata* pMetadata = pMetaBuffer->tryWriteLock(LOG_TAG);

    if (pMetadata != nullptr) {
      IMetadata::IEntry entry = pMetadata->entryFor(MTK_CONTROL_ZOOM_RATIO);
      if (!entry.isEmpty()) {
        float zoom_ratio = entry.itemAt(0, Type2Type<MFLOAT>());

        if (zoom_ratio > 0) {
          {
            std::lock_guard<std::mutex> lock(mMapLock);
            mReqRatioMap.emplace(reqNum, zoom_ratio);
            MY_LOGD_LV1("Set: R%d, stream(%s), zoom_ratio(%f), size(%d)",
                        reqNum, pMetaBuffer->getName(), zoom_ratio,
                        mReqRatioMap.size());
          }
          ApplyZoomRatio(pMetadata, zoom_ratio, d, true, false);
        } else {
          MY_LOGE("Invalid zoom ratio(%f)", zoom_ratio);
        }
      } else {
        MY_LOGD_LV1("CONTROL_ZOOM_RATIO not found");
      }

      pMetaBuffer->unlock(LOG_TAG, pMetadata);
    } else {
      MY_LOGE("Get pMetadata failed: R%d, stream(%s)", reqNum,
              pMetaBuffer->getName());
    }
  }
}

auto ZoomRatioConverter::UpdateCaptureResultMeta(UpdateResultParams& resultMeta)
    -> void {
  const uint32_t reqNum = resultMeta.requestNo;
  android::Vector<android::sp<IMetaStreamBuffer>>& vResultMeta =
    resultMeta.resultMeta;
  android::Vector<android::sp<PhysicalResult>>& vResultMeta_phy =
    resultMeta.physicalResult;
  const bool isLast = resultMeta.hasLastPartial;

  if (!mSupportZoomRatio) {
    for (int i = 0; i < vResultMeta.size(); i++)
      UpdateCropRegionWithoutConvert(vResultMeta[i], false);

    return;
  }

  float zoom_ratio = 0;

  {
    std::lock_guard<std::mutex> lock(mMapLock);
    if (mReqRatioMap.find(reqNum) != mReqRatioMap.end()) {
      zoom_ratio = mReqRatioMap.at(reqNum);
    } else {
      MY_LOGD("R%d no zoom ratio found in map", reqNum);
      return;
    }

    if (isLast) {
      mReqRatioMap.erase(reqNum);
      MY_LOGD_LV1("Remove: R%d", reqNum);
    }
  }

  for (int i = 0; i < vResultMeta.size(); i++) {
    if (vResultMeta[i]->hasStatus(STREAM_BUFFER_STATUS::ERROR)) {
      MY_LOGD("Skip error metabuffer: R%d, stream(%s)", reqNum,
              vResultMeta[i]->getName());
      continue;
    }

    Dimension d;
    if (!GetDimensionByMetaBuffer(-1 /*logical*/, d /*out*/)) {
      MY_LOGE("Get dimension failed, R%d stream(%s)", reqNum,
              vResultMeta[i]->getName());
      continue;
    }

    IMetadata* pMetadata = vResultMeta[i]->tryWriteLock(LOG_TAG);
    if (pMetadata != nullptr) {
      MY_LOGD_LV1("Get: R%d, stream(%s), zoom_ratio(%f)", reqNum,
                  vResultMeta[i]->getName(), zoom_ratio);
      ApplyZoomRatio(pMetadata, zoom_ratio, d, false, false);
      vResultMeta[i]->unlock(LOG_TAG, pMetadata);
    } else {
      MY_LOGE("Get pMetadata failed: R%d, stream(%s)", reqNum,
              vResultMeta[i]->getName());
    }
  }

  for (int i = 0; i < vResultMeta_phy.size(); i++) {
    auto& physicRes = vResultMeta_phy[i]->physicalResultMeta;
    auto phyId = std::stoi(vResultMeta_phy[i]->physicalCameraId);

    if (physicRes->hasStatus(STREAM_BUFFER_STATUS::ERROR)) {
      MY_LOGD("Skip error metabuffer: R%d, stream(%s)", reqNum,
              physicRes->getName());
      continue;
    }

    Dimension d;
    if (!GetDimensionByMetaBuffer(phyId, d /*out*/)) {
      MY_LOGE("Get phy dimension failed, R%d stream(%s)", reqNum,
              physicRes->getName());
      continue;
    }

    IMetadata* pMetadata = physicRes->tryWriteLock(LOG_TAG);
    if (pMetadata != nullptr) {
      MY_LOGD_LV1("Get: R%d, stream(%s), zoom_ratio(%f)", reqNum,
                  physicRes->getName(), zoom_ratio);
      ApplyZoomRatio(pMetadata, zoom_ratio, d, false, true);
      physicRes->unlock(LOG_TAG, pMetadata);
    } else {
      MY_LOGE("Get pMetadata failed: R%d, stream(%s)", reqNum,
              physicRes->getName());
    }
  }
}

auto ZoomRatioConverter::UpdateCropRegionWithoutConvert(
    android::sp<IMetaStreamBuffer> pMetaBuffer,
    const bool isReq) -> void {
  if (pMetaBuffer->hasStatus(STREAM_BUFFER_STATUS::ERROR)) {
    MY_LOGD("Skip error metabuffer: stream(%s)", pMetaBuffer->getName());
    return;
  }

  IMetadata* pMetadata = pMetaBuffer->tryWriteLock(LOG_TAG);

  if (pMetadata != nullptr) {
    if (isReq) {
      IMetadata::IEntry entry = pMetadata->entryFor(MTK_SCALER_CROP_APP_REGION);
      if (!entry.isEmpty()) {
        MRect rect = entry.itemAt(0, Type2Type<MRect>());

        if (rect.s.w != 0 && rect.s.h != 0) {
          IMetadata::IEntry entryNew(MTK_SCALER_CROP_REGION);
          entryNew.push_back(rect, Type2Type<MRect>());
          pMetadata->update(MTK_SCALER_CROP_REGION, entryNew);
        }
      }
    } else {
      // MTK_SCALER_CROP_APP_REGION <---> ANDROID_SCALER_CROP_REGION
      // MTK_SCALER_CROP_REGION become a vendor tag, and need to send to FWK,
      // remove it!
      IMetadata::IEntry entry = pMetadata->entryFor(MTK_SCALER_CROP_REGION);
      if (!entry.isEmpty()) {
        pMetadata->remove(MTK_SCALER_CROP_REGION);
      }
    }
    pMetaBuffer->unlock(LOG_TAG, pMetadata);
  } else {
    MY_LOGE("Get pMetadata failed: stream(%s)", pMetaBuffer->getName());
  }
}

auto ZoomRatioConverter::ApplyZoomRatio(IMetadata* pMetadata,
                                        const float zoom_ratio,
                                        const Dimension& active_array_dimension,
                                        const bool is_request,
                                        const bool is_phy) -> void {
  if (mLogLv > 0) {
    MY_LOGD("Bef isReq(%d)", is_request);
    Dump(pMetadata);
  }

  UpdateCropRegion(pMetadata, zoom_ratio, active_array_dimension, is_request,
                   is_phy);
  if (is_request) {
    Update3ARegion(pMetadata, zoom_ratio, MTK_CONTROL_AF_REGIONS,
                   active_array_dimension, is_request);
    Update3ARegion(pMetadata, zoom_ratio, MTK_CONTROL_AE_REGIONS,
                   active_array_dimension, is_request);
    Update3ARegion(pMetadata, zoom_ratio, MTK_CONTROL_AWB_REGIONS,
                   active_array_dimension, is_request);
  } else {
    Update3ARegion(pMetadata, zoom_ratio, MTK_CONTROL_AF_REGIONS,
                   active_array_dimension, is_request);
    Update3ARegion(pMetadata, zoom_ratio, MTK_CONTROL_AE_REGIONS,
                   active_array_dimension, is_request);
    Update3ARegion(pMetadata, zoom_ratio, MTK_CONTROL_AWB_REGIONS,
                   active_array_dimension, is_request);
    Update3AFeatureRegion(pMetadata, zoom_ratio, MTK_3A_FEATURE_AF_ROI,
                          active_array_dimension, is_request);
    Update3AFeatureRegion(pMetadata, zoom_ratio, MTK_3A_FEATURE_AE_ROI,
                          active_array_dimension, is_request);
    Update3AFeatureRegion(pMetadata, zoom_ratio, MTK_3A_FEATURE_AWB_ROI,
                          active_array_dimension, is_request);
    UpdateFace(pMetadata, zoom_ratio, active_array_dimension);
  }

  if (mLogLv > 0) {
    MY_LOGD("Aft");
    Dump(pMetadata);
  }
}

auto ZoomRatioConverter::UpdateCropRegion(
    IMetadata* pMetadata,
    const float zoom_ratio,
    const Dimension& active_array_dimension,
    const bool is_request,
    const bool is_phy) -> void {
  if (is_request) {
    IMetadata::IEntry entry = pMetadata->entryFor(MTK_SCALER_CROP_APP_REGION);
    if (!entry.isEmpty()) {
      MRect rect = entry.itemAt(0, Type2Type<MRect>());

      if (rect.s.w == 0 || rect.s.h == 0) {
        return;
      }

      ConvertZoomRatio(zoom_ratio, rect, active_array_dimension);

      IMetadata::IEntry entryNew(MTK_SCALER_CROP_REGION);
      entryNew.push_back(rect, Type2Type<MRect>());
      pMetadata->update(MTK_SCALER_CROP_REGION, entryNew);
    }
  } else {  // result
    if (is_phy) {
      IMetadata::IEntry entry = pMetadata->entryFor(MTK_SCALER_CROP_REGION);
      if (!entry.isEmpty()) {
        MRect rect = entry.itemAt(0, Type2Type<MRect>());

        if (rect.s.w != 0 && rect.s.h != 0) {
          IMetadata::IEntry entryNew(MTK_SCALER_CROP_APP_REGION);
          entryNew.push_back(rect, Type2Type<MRect>());
          pMetadata->update(MTK_SCALER_CROP_APP_REGION, entryNew);
        }
      }
    }

    // MTK_SCALER_CROP_APP_REGION <---> ANDROID_SCALER_CROP_REGION
    // MTK_SCALER_CROP_REGION is no longer a Android meta tag, so no need to
    // send to FWK, remove it!
    IMetadata::IEntry entry = pMetadata->entryFor(MTK_SCALER_CROP_REGION);
    if (!entry.isEmpty()) {
      pMetadata->remove(MTK_SCALER_CROP_REGION);
    }
  }
}

auto ZoomRatioConverter::Update3ARegion(IMetadata* pMetadata,
                                        const float zoom_ratio,
                                        const mtk_camera_metadata_tag_t tag_id,
                                        const Dimension& active_array_dimension,
                                        const bool is_request) -> void {
  IMetadata::IEntry entry = pMetadata->entryFor(tag_id);

  if (!entry.isEmpty()) {
    MINT32 numRgns = entry.count() / 5;

    IMetadata::IEntry entryUpdated(tag_id);

    for (size_t i = 0; i < numRgns; i++) {
      int32_t left = entry.itemAt(i * 5 + 0, Type2Type<MINT32>());
      int32_t top = entry.itemAt(i * 5 + 1, Type2Type<MINT32>());
      int32_t right = entry.itemAt(i * 5 + 2, Type2Type<MINT32>());
      int32_t bottom = entry.itemAt(i * 5 + 3, Type2Type<MINT32>());
      int32_t weight = entry.itemAt(i * 5 + 4, Type2Type<MINT32>());
      int32_t width = right - left;
      int32_t height = bottom - top;

      if (left != 0 || top != 0 || width != 0 || height != 0) {
        if (is_request) {
          ConvertZoomRatio(zoom_ratio, left, top, width, height,
                           active_array_dimension);
        } else {
          RevertZoomRatio(zoom_ratio, left, top, width, height,
                          active_array_dimension);
        }
        right = left + width;
        bottom = top + height;
      }

      entryUpdated.push_back(left, Type2Type<MINT32>());
      entryUpdated.push_back(top, Type2Type<MINT32>());
      entryUpdated.push_back(right, Type2Type<MINT32>());
      entryUpdated.push_back(bottom, Type2Type<MINT32>());
      entryUpdated.push_back(weight, Type2Type<MINT32>());
    }

    pMetadata->update(tag_id, entryUpdated);
  }
}

auto ZoomRatioConverter::Update3AFeatureRegion(
    IMetadata* pMetadata,
    const float zoom_ratio,
    const mtk_camera_metadata_tag_t tag_id,
    const Dimension& active_array_dimension,
    const bool is_request) -> void {
  IMetadata::IEntry entry = pMetadata->entryFor(tag_id);

  if (!entry.isEmpty()) {
    int32_t entryCount = entry.count();
    if (entryCount < 7 ||
        entryCount != entry.itemAt(1, Type2Type<MINT32>()) * 5 + 2) {
      MY_LOGE_LV1("invalid entry, tag(0x%x)", tag_id);
    }
    MINT32 numRgns = (entryCount - 2) / 5;
    IMetadata::IEntry entryReverted(tag_id);

    entryReverted.push_back(entry.itemAt(0, Type2Type<MINT32>()),
                            Type2Type<MINT32>());
    entryReverted.push_back(entry.itemAt(1, Type2Type<MINT32>()),
                            Type2Type<MINT32>());

    for (size_t i = 0; i < numRgns; i++) {
      int32_t left = entry.itemAt(2 + i * 5 + 0, Type2Type<MINT32>());
      int32_t top = entry.itemAt(2 + i * 5 + 1, Type2Type<MINT32>());
      int32_t right = entry.itemAt(2 + i * 5 + 2, Type2Type<MINT32>());
      int32_t bottom = entry.itemAt(2 + i * 5 + 3, Type2Type<MINT32>());
      int32_t weight = entry.itemAt(2 + i * 5 + 4, Type2Type<MINT32>());
      int32_t width = right - left;
      int32_t height = bottom - top;

      if (left != 0 || top != 0 || width != 0 || height != 0) {
        RevertZoomRatio(zoom_ratio, left, top, width, height,
                        active_array_dimension);
        right = left + width - 1;
        bottom = top + height - 1;
      }

      entryReverted.push_back(left, Type2Type<MINT32>());
      entryReverted.push_back(top, Type2Type<MINT32>());
      entryReverted.push_back(right, Type2Type<MINT32>());
      entryReverted.push_back(bottom, Type2Type<MINT32>());
      entryReverted.push_back(weight, Type2Type<MINT32>());
    }

    pMetadata->update(tag_id, entryReverted);
  }
}

auto ZoomRatioConverter::UpdateFace(IMetadata* pMetadata,
                                    const float zoom_ratio,
                                    const Dimension& active_array_dimension)
    -> void {
  ////// Face Rect
  IMetadata::IEntry entry = pMetadata->entryFor(MTK_STATISTICS_FACE_RECTANGLES);
  if (!entry.isEmpty()) {
    IMetadata::IEntry entryRectUpdated(MTK_STATISTICS_FACE_RECTANGLES);
    int32_t faceCount = entry.count();
    for (size_t i = 0; i < faceCount; i++) {
      MRect faceRect = entry.itemAt(i, Type2Type<MRect>());
      // (right, bottom) -> (w, h)
      faceRect.s.w = faceRect.s.w - faceRect.p.x;
      faceRect.s.h = faceRect.s.h - faceRect.p.y;

      RevertZoomRatio(zoom_ratio, faceRect, active_array_dimension);

      // (w, h) -> (right, bottom)
      faceRect.s.w = faceRect.s.w + faceRect.p.x;
      faceRect.s.h = faceRect.s.h + faceRect.p.y;
      entryRectUpdated.push_back(faceRect, Type2Type<MRect>());
    }
    pMetadata->update(MTK_STATISTICS_FACE_RECTANGLES, entryRectUpdated);
  }

  ////// Face Landmarks
  IMetadata::IEntry entryLandmark =
    pMetadata->entryFor(MTK_STATISTICS_FACE_LANDMARKS);
  if (!entryLandmark.isEmpty()) {
    int32_t landmarkCount = entryLandmark.count() / 6;
    IMetadata::IEntry entryLandmarkUpdated(MTK_STATISTICS_FACE_LANDMARKS);
    for (size_t i = 0; i < landmarkCount; i++) {
      // left eye
      int32_t x = entryLandmark.itemAt(i * 6 + 0, Type2Type<MINT32>());
      int32_t y = entryLandmark.itemAt(i * 6 + 1, Type2Type<MINT32>());
      RevertZoomRatio(zoom_ratio, x, y, active_array_dimension);
      entryLandmarkUpdated.push_back(x, Type2Type<MINT32>());
      entryLandmarkUpdated.push_back(y, Type2Type<MINT32>());
      // right eye
      x = entryLandmark.itemAt(i * 6 + 2, Type2Type<MINT32>());
      y = entryLandmark.itemAt(i * 6 + 3, Type2Type<MINT32>());
      RevertZoomRatio(zoom_ratio, x, y, active_array_dimension);
      entryLandmarkUpdated.push_back(x, Type2Type<MINT32>());
      entryLandmarkUpdated.push_back(y, Type2Type<MINT32>());
      // mouth
      x = entryLandmark.itemAt(i * 6 + 4, Type2Type<MINT32>());
      y = entryLandmark.itemAt(i * 6 + 5, Type2Type<MINT32>());
      RevertZoomRatio(zoom_ratio, x, y, active_array_dimension);
      entryLandmarkUpdated.push_back(x, Type2Type<MINT32>());
      entryLandmarkUpdated.push_back(y, Type2Type<MINT32>());
    }
    pMetadata->update(MTK_STATISTICS_FACE_LANDMARKS, entryLandmarkUpdated);
  }
}

auto ZoomRatioConverter::ConvertZoomRatio(
    const float zoom_ratio,
    MRect& rect,
    const Dimension& active_array_dimension) -> void {
  int32_t left = rect.p.x;
  int32_t top = rect.p.y;
  int32_t width = rect.s.w;
  int32_t height = rect.s.h;

  ConvertZoomRatio(zoom_ratio, left, top, width, height,
                   active_array_dimension);

  rect.p.x = left;
  rect.p.y = top;
  rect.s.w = width;
  rect.s.h = height;
}

auto ZoomRatioConverter::ConvertZoomRatio(
    const float zoom_ratio,
    int32_t& left,
    int32_t& top,
    int32_t& width,
    int32_t& height,
    const Dimension& active_array_dimension) -> void {
  if (zoom_ratio <= 0) {
    MY_LOGE("invalid params");
    return;
  }

  left = std::round(left / zoom_ratio +
                    0.5f * active_array_dimension.width *
                        (1.0f - 1.0f / zoom_ratio));
  top = std::round(top / zoom_ratio +
                   0.5f * active_array_dimension.height *
                       (1.0f - 1.0f / zoom_ratio));
  width = std::round(width / zoom_ratio);
  height = std::round(height / zoom_ratio);

#define ALIGN_2(x) (((x) + 1) & (~1))
  left = ALIGN_2(left);
  top = ALIGN_2(top);
  width = ALIGN_2(width);
  height = ALIGN_2(height);
#undef ALIGN_2

  if (zoom_ratio >= 1.0f) {
    CorrectRegionBoundary(left, top, width, height,
                          active_array_dimension.width,
                          active_array_dimension.height);
  }
}

auto ZoomRatioConverter::RevertZoomRatio(
    const float zoom_ratio,
    MRect& rect,
    const Dimension& active_array_dimension) -> void {
  int32_t left = rect.p.x;
  int32_t top = rect.p.y;
  int32_t width = rect.s.w;
  int32_t height = rect.s.h;

  RevertZoomRatio(zoom_ratio, left, top, width, height, active_array_dimension);
  rect.p.x = left;
  rect.p.y = top;
  rect.s.w = width;
  rect.s.h = height;
}

auto ZoomRatioConverter::RevertZoomRatio(
    const float zoom_ratio,
    int32_t& left,
    int32_t& top,
    int32_t& width,
    int32_t& height,
    const Dimension& active_array_dimension) -> void {
  left = std::round(left * zoom_ratio -
                    0.5f * active_array_dimension.width * (zoom_ratio - 1.0f));
  top = std::round(top * zoom_ratio -
                   0.5f * active_array_dimension.height * (zoom_ratio - 1.0f));
  width = std::round(width * zoom_ratio);
  height = std::round(height * zoom_ratio);

  CorrectRegionBoundary(left, top, width, height, active_array_dimension.width,
                        active_array_dimension.height);
}

auto ZoomRatioConverter::RevertZoomRatio(
    const float zoom_ratio,
    int32_t& x,
    int32_t& y,
    const Dimension& active_array_dimension) -> void {
  x = std::round(x * zoom_ratio -
                 0.5f * active_array_dimension.width * (zoom_ratio - 1.0f));
  y = std::round(y * zoom_ratio -
                 0.5f * active_array_dimension.height * (zoom_ratio - 1.0f));
}

auto ZoomRatioConverter::CorrectRegionBoundary(int32_t& left,
                                               int32_t& top,
                                               int32_t& width,
                                               int32_t& height,
                                               const int32_t bound_w,
                                               const int32_t bound_h) -> void {
  left = std::max(left, 0);
  left = std::min(left, bound_w - 1);

  top = std::max(top, 0);
  top = std::min(top, bound_h - 1);

  width = std::max(width, 1);
  width = std::min(width, bound_w - left);

  height = std::max(height, 1);
  height = std::min(height, bound_h - top);
}

auto ZoomRatioConverter::GetDimensionByMetaBuffer(const int32_t phyId,
                                                  Dimension& d) -> bool {
  // logical
  if (phyId == -1) {
    d.width = mActiveArrayDimension.width;
    d.height = mActiveArrayDimension.height;
    MY_LOGD_LV1("get logical dimension, w(%d), h(%d)", d.width, d.height);
    return true;
  } else {  // physical
    if (mActiveArrayDimension_Physical.find(phyId) !=
        mActiveArrayDimension_Physical.end()) {
      d.width = mActiveArrayDimension_Physical[phyId].width;
      d.height = mActiveArrayDimension_Physical[phyId].height;
      MY_LOGD_LV1("get phy(%d) dimension, w(%d), h(%d)", phyId, d.width,
                  d.height);
      return true;
    }
  }

  return false;
}

auto ZoomRatioConverter::Dump(const IMetadata* pMetadata) -> void {
  DumpTag(pMetadata, MTK_SCALER_CROP_APP_REGION, "CROP_APP_REGION ");
  DumpTag(pMetadata, MTK_SCALER_CROP_REGION, "CROP_REGION ");
  DumpTag(pMetadata, MTK_CONTROL_AE_REGIONS, "AE_REGIONS ");
  DumpTag(pMetadata, MTK_CONTROL_AF_REGIONS, "AF_REGIONS ");
  DumpTag(pMetadata, MTK_CONTROL_AWB_REGIONS, "AWB_REGIONS ");
  DumpTag(pMetadata, MTK_3A_FEATURE_AE_ROI, "AE_ROI ");
  DumpTag(pMetadata, MTK_3A_FEATURE_AF_ROI, "AF_ROI ");
  DumpTag(pMetadata, MTK_3A_FEATURE_AWB_ROI, "AWB_ROI ");
  DumpTag(pMetadata, MTK_STATISTICS_FACE_RECTANGLES, "FACE_RECTANGLES ");
  DumpTag(pMetadata, MTK_STATISTICS_FACE_LANDMARKS, "FACE_LANDMARKS ");
}

auto ZoomRatioConverter::DumpTag(const IMetadata* pMetadata,
                                 const MUINT32 tag_id,
                                 const std::string prefix) -> void {
  std::stringstream ss;
  ss << prefix;
  IMetadata::IEntry entry = pMetadata->entryFor(tag_id);

  if (!entry.isEmpty()) {
    switch (entry.type()) {
      case TYPE_MINT32:
        for (size_t j = 0; j < entry.count(); j++)
          ss << entry.itemAt(j, Type2Type<MINT32>()) << " ";
        break;
      case TYPE_MSize:
        for (size_t j = 0; j < entry.count(); j++) {
          MSize src_size = entry.itemAt(j, Type2Type<MSize>());
          ss << "size(" << src_size.w << "," << src_size.h << ") ";
        }
        break;
      case TYPE_MRect:
        for (size_t j = 0; j < entry.count(); j++) {
          MRect src_rect = entry.itemAt(j, Type2Type<MRect>());
          ss << "rect(" << src_rect.p.x << "," << src_rect.p.y << ","
             << src_rect.s.w << "," << src_rect.s.h << ") ";
        }
        break;
      case TYPE_MPoint:
        for (size_t j = 0; j < entry.count(); j++) {
          MPoint src_point = entry.itemAt(j, Type2Type<MPoint>());
          ss << "point(" << src_point.x << "," << src_point.y << ") ";
        }
        break;
      default:
        ss << "unsupported type:" << entry.type();
    }

    MY_LOGD("%s", ss.str().c_str());
  }
}
};  // namespace core
};  // namespace mcam
