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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_ZOOMRATIOCONVERTER_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_ZOOMRATIOCONVERTER_H_

#include <IAppStreamManager.h>
#include <cutils/properties.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/mtk_metadata_types.h>
#include <unordered_map>
#include <memory>
#include <string>

using NSCam::MRect;

namespace mcam {
namespace core {

class ZoomRatioConverter {
 public:
  struct Dimension {
    uint32_t width = 0;
    uint32_t height = 0;
  };

  typedef std::unordered_map<uint32_t, Dimension> DimensionMap;

  ZoomRatioConverter(bool supportZoomRatio,
                     Dimension d,
                     DimensionMap map,
                     int32_t logLv)
      : mSupportZoomRatio(supportZoomRatio),
        mActiveArrayDimension(d),
        mActiveArrayDimension_Physical(map),
        mLogLv(logLv) {}

  static auto Create(bool supportZoomRatio, Dimension d, DimensionMap map)
      -> std::shared_ptr<ZoomRatioConverter>;

  auto UpdateCaptureRequestSingleMeta(
      const uint32_t reqNum,
      StreamId_T streamId,
      android::sp<IMetaStreamBuffer> pMetaBuffer) -> void;

  auto UpdateCaptureRequestMeta(
      const uint32_t reqNum,
      std::unordered_map<StreamId_T, android::sp<IMetaStreamBuffer>>&
          requestMetaMap) -> void;

  auto UpdateCaptureResultMeta(UpdateResultParams& resultMeta) -> void;

 private:
  auto UpdateCropRegionWithoutConvert(
      android::sp<IMetaStreamBuffer> pMetaBuffer,
      const bool isReq) -> void;

  auto ApplyZoomRatio(IMetadata* metadata,
                      const float zoom_ratio,
                      const Dimension& active_array_dimension,
                      const bool is_request,
                      const bool is_phy) -> void;

  auto UpdateCropRegion(IMetadata* metadata,
                        const float zoom_ratio,
                        const Dimension& active_array_dimension,
                        const bool is_request,
                        const bool is_phy) -> void;

  auto Update3ARegion(IMetadata* pMetadata,
                      const float zoom_ratio,
                      const mtk_camera_metadata_tag_t tag_id,
                      const Dimension& active_array_dimension,
                      const bool is_request) -> void;

  auto Update3AFeatureRegion(IMetadata* pMetadata,
                             const float zoom_ratio,
                             const mtk_camera_metadata_tag_t tag_id,
                             const Dimension& active_array_dimension,
                             const bool is_request) -> void;

  auto UpdateFace(IMetadata* pMetadata,
                  const float zoom_ratio,
                  const Dimension& active_array_dimension) -> void;

  auto ConvertZoomRatio(const float zoom_ratio,
                        MRect& rect,
                        const Dimension& active_array_dimension) -> void;

  auto ConvertZoomRatio(const float zoom_ratio,
                        int32_t& left,
                        int32_t& top,
                        int32_t& width,
                        int32_t& height,
                        const Dimension& active_array_dimension) -> void;

  auto RevertZoomRatio(const float zoom_ratio,
                       MRect& rect,
                       const Dimension& active_array_dimension) -> void;

  auto RevertZoomRatio(const float zoom_ratio,
                       int32_t& left,
                       int32_t& top,
                       int32_t& width,
                       int32_t& height,
                       const Dimension& active_array_dimension) -> void;

  auto RevertZoomRatio(const float zoom_ratio,
                       int32_t& x,
                       int32_t& y,
                       const Dimension& active_array_dimension) -> void;

  auto CorrectRegionBoundary(int32_t& left,
                             int32_t& top,
                             int32_t& width,
                             int32_t& height,
                             const int32_t bound_w,
                             const int32_t bound_h) -> void;

  auto GetDimensionByMetaBuffer(const int32_t physicalId, Dimension& d) -> bool;

  auto Dump(const IMetadata* pMetadata) -> void;

  auto DumpTag(const IMetadata* pMetadata,
               const MUINT32 tag_id,
               const std::string prefix) -> void;

  bool mSupportZoomRatio = false;
  Dimension mActiveArrayDimension;
  DimensionMap mActiveArrayDimension_Physical;
  int32_t mLogLv = 0;

  std::mutex mMapLock;
  std::unordered_map<uint32_t, float> mReqRatioMap;
};

static inline std::string toString(MRect const& r) {
  std::ostringstream oss;
  oss << "[";
  oss << r.p.x << ", " << r.p.y << ", " << r.s.w << ", " << r.s.h;
  oss << "]";
  return oss.str();
}

}  // namespace core
}  // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_INCLUDE_ZOOMRATIOCONVERTER_H_
