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

#ifndef _MTKCAM3_FEATURE_DSDN_DSDN_TYPE_H_
#define _MTKCAM3_FEATURE_DSDN_DSDN_TYPE_H_

#include <mtkcam/def/common.h>

namespace NSCam {
namespace NSCamFeature {

class DSDNRatio
{
public:
    DSDNRatio() {}
    DSDNRatio(MUINT32 mul, MUINT32 div) : mMul(mul), mDiv(div) {}
    MBOOL isValid() const { return mMul && mDiv; }
    operator bool() const { return isValid(); }
    bool operator <(const DSDNRatio &rhs) const { return ((mMul * rhs.mDiv) < (mDiv * rhs.mMul));}

public:
    MUINT32 mMul = 1;
    MUINT32 mDiv = 1;
};

class DSDNParam
{
public:
    enum MODE
    {
        MODE_OFF = 0,
        MODE_20,
        MODE_25,
        MODE_30,
    };

public:
    MODE    mMode = MODE_OFF;
    MUINT32 mMaxDSLayer = 0;
    MBOOL   mDynamicP1Ctrl = MFALSE;
    DSDNRatio mMaxP1Ratio;
    DSDNRatio mMaxDsRatio;
    MBOOL   mDSDN20_10Bit = MFALSE;
    MBOOL   mDSDN30_GyroEnable = MFALSE;

public:
    MBOOL isDSDN20() const { return mMode == MODE_20; }
    MBOOL isDSDN25() const { return mMode == MODE_25; }
    MBOOL isDSDN30() const { return mMode == MODE_30; }
    MBOOL isDSDN() const { return isDSDN20() || isDSDN25() || isDSDN30(); }
    MBOOL isDSDN20Bit10() const { return mDSDN20_10Bit; }
    MBOOL isGyroEnable() const { return mDSDN30_GyroEnable; }
    const char* toModeStr() const
    {
        return isDSDN25() ? "dsdn_25" :
               isDSDN20() ? "dsdn_20" :
               isDSDN30() ? "dsdn_30" :"dsdn_off";
    }
};

}; // NSCamFeature
}; // NSCam

#endif // _MTKCAM3_FEATURE_DSDN_DSDN_TYPE_H_
