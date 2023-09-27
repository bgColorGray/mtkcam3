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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_P2HW_UTIL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_P2HW_UTIL_H_

#include "core/P2GDefine.h"
#include <mtkcam3/feature/utils/p2/DIPStream.h>
#include <mtkcam3/feature/utils/SecureBufferControlUtils.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

class P2HWUtil
{
public:
  using DIPParams = ::NSCam::Feature::P2Util::DIPParams;
  using DIPFrameParams = ::NSCam::Feature::P2Util::DIPFrameParams;

public:
  P2HWUtil();
  ~P2HWUtil();
  bool init(NSCam::SecType secureType, MUINT32 p2RBMask);
  bool uninit();
  DIPParams run(const DataPool &pool, const std::vector<P2HW> &p2hw, MUINT32 optimizeLevel);

private:
  bool check(bool cond, const char *log) const;
  bool check(const P2HW &cfg) const;
  MVOID readbackVipi(const P2HW &cfg);
  DIPFrameParams make(const P2HW &cfg);

  P2IO toFullP2IO(const DataPool &pool, const ILazy<ImgBuffer> &buffer, const IMRectF &crop);

private:
  ::NSCam::Feature::SecureBufferControl mSecureControl;
  MBOOL mSupportMSF = MTRUE;

  MUINT32 mP2RBMask = 0;
};

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_P2HW_UTIL_H_
