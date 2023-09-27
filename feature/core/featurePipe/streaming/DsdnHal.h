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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_DSDN_HAL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_DSDN_HAL_H_

#include "MtkHeader.h"
#include <set>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class DsdnHal
{
public:
    enum eDSDNImg { FULL, DS, DN, PDS, OMCMV, CONF, IDI, MSF_WEIGHT, DS_WEIGHT };

public:
    DsdnHal();
    ~DsdnHal();

    MVOID init(const DSDNParam &param, const MSize &fullSize, const MSize &fdSize);
    MVOID uninit();

    MBOOL isSupportDSDN20() const;
    MBOOL isSupportDSDN25() const;
    MBOOL isSupportDSDN30() const;

    EImageFormat getFormat(eDSDNImg img) const;
    MUINT32 getVersion() const;
    MUINT32 getMaxDSLayer() const;
    std::vector<MSize> getMaxDSSizes() const;
    std::vector<MSize> getDSSizes(const MSize &inSize, const MSize &nextSize, const MSize &fdSize, const DSDNRatio &ratio) const;
    MSize getSize(const std::vector<MSize> &dsSizes, eDSDNImg img) const;
    std::vector<MINT32> getProfiles(MINT32 hdrHalMode) const;
    MBOOL isLoopValid(const MSize &curInSize, MINT64 curTime, MBOOL curOn,
                    const MSize &preInSize, MINT64 preTime, MBOOL preOn) const;

private:
    std::vector<MSize> getDSSizes_25(const MSize &fullSize) const;
    std::vector<MSize> getDSSizes_20(const MSize &fullSize, const MSize &fdSize, DSDNRatio ratio) const;
    MSize getConfSize(const std::vector<MSize> &dsSizes) const;
    MSize getIdiSize(const std::vector<MSize> &dsSizes) const;
    MSize getOmcSize(const std::vector<MSize> &dsSizes) const;
    MVOID initDSDN20Profile();
    static MINT32 getDSDN25Profile(MUINT32 layer, MINT32 hdrHalMode);

private:
    EImageFormat mFullFmt = eImgFmt_UNKNOWN;
    EImageFormat mDSFmt = eImgFmt_UNKNOWN;
    EImageFormat mDNFmt = eImgFmt_UNKNOWN;
    EImageFormat mPDSFmt = eImgFmt_UNKNOWN;
    MINT32 mDSDN20Profile = -1;

    DSDNParam mCfg;
    MSize mMaxFullSize;
    MSize mMaxFDSize;

    DSDNRatio mMaxRatio;
    DSDNRatio mDebugRatio;

    MUINT32 mMinSize = 0;
    MUINT32 mVer = 0;
    MBOOL mDebugFmtNV21 = MFALSE;
    MBOOL mForceLoopValid = MFALSE;

    MSize mDS1Size;
    MSize mDS2Size;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_DSDN_HAL_H_
