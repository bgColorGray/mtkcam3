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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#ifndef PIPELINE_POLICY_CUSTOM_FEATURESETTINGPOLICY_STREAMING_CUSTOM_H_
#define PIPELINE_POLICY_CUSTOM_FEATURESETTINGPOLICY_STREAMING_CUSTOM_H_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "mtkcam/utils/metadata/IMetadata.h"
#include "mtkcam/utils/sys/Cam3CPUCtrl.h"
#include "mtkcam3/pipeline/def/types.h"

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {

static const int32_t CUSTOM_STREAM_DEFAULT = -1;

struct CustomStreamConfiguration {
  int32_t proprietaryClient = CUSTOM_STREAM_DEFAULT;
  int32_t reqFps = CUSTOM_STREAM_DEFAULT;
  int32_t dsdnHint = CUSTOM_STREAM_DEFAULT;
  int32_t nr3dMode = CUSTOM_STREAM_DEFAULT;
  int32_t eisFactor = CUSTOM_STREAM_DEFAULT;
};

struct FrameDescriptor {
  std::shared_ptr<NSCam::IMetadata> appMetadata = nullptr;
  std::shared_ptr<NSCam::IMetadata> halMetadata = nullptr;
};

struct CustomStreamData {
  SensorMap<FrameDescriptor> mainFrame;
  std::vector<SensorMap<FrameDescriptor>> preDummyFrame;
  Cam3CPUCtrl::ENUM_CAM3_CPU_CTRL_FPSGO_PROFILE cpuProfile =
      Cam3CPUCtrl::E_CAM3_CPU_CTRL_FPSGO_DISABLE;
};

static inline bool hasCustomStreamConfiguration(
    const CustomStreamConfiguration& customConfig) {
  if (customConfig.proprietaryClient != CUSTOM_STREAM_DEFAULT ||
      customConfig.reqFps != CUSTOM_STREAM_DEFAULT ||
      customConfig.dsdnHint != CUSTOM_STREAM_DEFAULT ||
      customConfig.nr3dMode != CUSTOM_STREAM_DEFAULT ||
      customConfig.eisFactor != CUSTOM_STREAM_DEFAULT) {
    return true;
  }
  return false;
}

static inline std::string toString(const CustomStreamConfiguration& o) {
  std::stringstream sstream;
  sstream << "(" << &o << "){"
          << " .proprietaryClient=" << o.proprietaryClient
          << " .reqFps=" << o.reqFps << " .dsdnHint=" << o.dsdnHint
          << " .nr3dMode=" << o.nr3dMode << " .eisFactor=" << o.eisFactor
          << "}";
  return sstream.str();
}

static inline std::string toString(const CustomStreamData& o) {
  std::stringstream sstream;
  sstream << "(" << &o << "){"
          << " .cpuProfile=" << o.cpuProfile;
  sstream << " .preDummyFrame=(" << o.preDummyFrame.size() << ")";
  for (const auto& predummy : o.preDummyFrame) {
    sstream << "[ ";
    for (const auto& pair : predummy) {
        sstream << pair.first << " ";
    }
    sstream << "]";
  }
  if (!o.mainFrame.empty()) {
    sstream << " .mainFrame=(1)[ ";
    for (const auto& pair : o.mainFrame) {
      if (pair.second.appMetadata != nullptr ||
          pair.second.halMetadata != nullptr) {
          sstream << pair.first << " ";
      }
    }
    sstream << "]";
  }
  sstream << "}";
  return sstream.str();
}

};  // namespace featuresetting
};  // namespace policy
};  // namespace pipeline
};  // namespace v3
};  // namespace NSCam

#endif  // PIPELINE_POLICY_CUSTOM_FEATURESETTINGPOLICY_STREAMING_CUSTOM_H_
