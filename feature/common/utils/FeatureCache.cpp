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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#include <mtkcam3/feature/utils/FeatureCache.h>

namespace NSCam {
namespace Feature {

MSize DSDNCache::Data::resizeP1(const MSize &p1Size) const
{
    MSize out(p1Size);
    if( p1RatioMultiple < p1RatioDivider && p1RatioDivider != 0)
    {
        out.w = p1Size.w * p1RatioMultiple / p1RatioDivider;
        out.h = p1Size.h * p1RatioMultiple / p1RatioDivider;
        out.w &= ~1;
        out.h &= ~1;
    }
    return out;
}

DSDNCache* DSDNCache::getInstance()
{
    static DSDNCache inst;
    return &inst;
}

void DSDNCache::init()
{
}

void DSDNCache::uninit()
{
}

void DSDNCache::setData(const Data &data)
{
    std::lock_guard<std::mutex> _lock(mMutex);
    mData = data;
}

DSDNCache::Data DSDNCache::getData() const
{
    std::lock_guard<std::mutex> _lock(mMutex);
    return mData;
}

EISCache* EISCache::getInstance()
{
    static EISCache inst;
    return &inst;
}

void EISCache::init()
{
}

void EISCache::uninit()
{
}

void EISCache::setEISCrop(const int32_t xOffset, const int32_t yOffset,
                          const int32_t leftTopX, const int32_t leftTopY,
                          const int32_t rightDownX, const int32_t rightDownY,
                          const int32_t warpinW, const int32_t warpinH,
                          const int32_t warpoutW, const int32_t warpoutH,
                          const MRectF displayCrop)
{
    std::lock_guard<std::mutex> _lock(mMutex);
    mOffsetX = xOffset;
    mOffsetY = yOffset;
    mLeftTopX = leftTopX;
    mLeftTopY = leftTopY;
    mRightDownX = rightDownX;
    mRightDownY = rightDownY;
    mWarpinW = warpinW;
    mWarpinH = warpinH;
    mWarpoutW = warpoutW;
    mWarpoutH = warpoutH;
    mDisplayCrop = displayCrop;
}

bool EISCache::getEISCrop(int32_t& xOffset, int32_t& yOffset,
                          int32_t& leftTopX, int32_t& leftTopY,
                          int32_t& rightDownX, int32_t& rightDownY,
                          int32_t& warpinW, int32_t& warpinH,
                          int32_t& warpoutW, int32_t& warpoutH,
                          MRectF& displayCrop) const
{
    std::lock_guard<std::mutex> _lock(mMutex);
    xOffset = mOffsetX;
    yOffset = mOffsetY;
    leftTopX = mLeftTopX;
    leftTopY = mLeftTopY;
    rightDownX = mRightDownX;
    rightDownY = mRightDownY;
    warpinW = mWarpinW;
    warpinH = mWarpinH;
    warpoutW = mWarpoutW;
    warpoutH = mWarpoutH;
    displayCrop = mDisplayCrop;
    return true;
}

} // namespace Feature
} // namespace NSCam
