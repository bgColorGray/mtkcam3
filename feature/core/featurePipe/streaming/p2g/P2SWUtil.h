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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_P2SW_UTIL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_P2SW_UTIL_H_

#include "core/P2GDefine.h"
#include "SensorContext.h"

#include <common/3dnr/3dnr_hal_base.h>
#include <mtkcam3/feature/3dnr/3dnr_defs.h>
using NSCam::NSIoPipe::NSPostProc::Hal3dnrBase;
using NS3Av3::MetaSet_T;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

class P2SWUtil
{
public:

public:
  P2SWUtil();
  ~P2SWUtil();
  bool init(const std::shared_ptr<SensorContextMap> &map);
  bool uninit();
  void run(const DataPool &pool, const std::vector<P2SW> &p2sw, P2SW::TuningType type);

private:
  void runP2(const DataPool &pool, const P2SW &cfg);
  void runOMC(const DataPool &pool, const P2SW &cfg);
  void acquireTuning(const DataPool &pool, const P2SW &cfg);
  void processTuning(const P2SW &cfg);
  void processSub(const P2SW &cfg) const;
  void processSubLCEI(const P2SW &cfg) const;
  void processNR3D(const P2SW &cfg, const SensorContextPtr &ctx, MetaSet_T &inMetSet);
  MUINT32 decideNR3D_featureMask(const P2SW &cfg);
  void processNR3D_feature(
    const P2SW &cfg, const SensorContextPtr &ctx, MetaSet_T &inMetaSet, const MUINT32 featMask);
  void prepareNR3D_common (
      const P2SW &cfg, const SensorContextPtr &ctx, const MUINT32 featMask,
      NR3DHALParam& nr3dHalParam);
  void postProcessNR3D_legacy(const P2SW &cfg, const SensorContextPtr &ctx);
  void processDSDN(const P2SW &cfg, NS3Av3::MetaSet_T &inSet);

private:
  bool mReady = false;
  std::shared_ptr<SensorContextMap> mSensorContextMap;

  Hal3dnrBase *mp3dnr[HAL_3DNR_INST_MAX_NUM] = { NULL };
  int32_t mLogLevel = 0;
  int32_t m3dnrDebugEnable = 0;
};

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_P2SW_UTIL_H_
