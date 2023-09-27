/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2019. All rights reserved.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef _MTKCAM_UTILS_FEATURE_CACHE_H_
#define _MTKCAM_UTILS_FEATURE_CACHE_H_

#include <mutex>
#include <mtkcam/def/UITypes.h>


// ---------------------------------------------------------------------------

namespace NSCam {
namespace Feature {
// ---------------------------------------------------------------------------
class DSDNCache
{
public:
    class Data
    {
    public:
        bool run = false;
        bool p1Control = false;
        uint32_t p1RatioMultiple = 1;
        uint32_t p1RatioDivider = 1;

        Data(){}
        Data(bool _run, bool _p1Control, uint32_t multi, uint32_t divide)
        :  run(_run)
        ,  p1Control(_p1Control)
        ,  p1RatioMultiple(multi)
        ,  p1RatioDivider(divide)
        {}

        MSize resizeP1(const MSize &p1Size) const;
    };
    static DSDNCache* getInstance();
    ~DSDNCache(){}
    void init();
    void uninit();
    void setData(const Data &data);
    Data getData() const;

private:
    DSDNCache() {};
    DSDNCache::Data mData;
    mutable std::mutex mMutex;
};

class EISCache
{
public:
    static EISCache* getInstance();
    ~EISCache(){}
    void init();
    void uninit();
    void setEISCrop(const int32_t xOffset, const int32_t yOffset,
                    const int32_t leftTopX, const int32_t leftTopY,
                    const int32_t rightDownX, const int32_t rightDownY,
                    const int32_t warpinW, const int32_t warpinH,
                    const int32_t warpoutW, const int32_t warpoutH,
                    const MRectF displayCrop);
    bool getEISCrop(int32_t& xOffset, int32_t& yOffset,
                    int32_t& leftTopX, int32_t& leftTopY,
                    int32_t& rightDownX, int32_t& rightDownY,
                    int32_t& warpinW, int32_t& warpinH,
                    int32_t& warpoutW, int32_t& warpoutH,
                    MRectF& displayCrop) const;
 private:
    int32_t mOffsetX = 0;
    int32_t mOffsetY = 0;
    int32_t mLeftTopX = 0;
    int32_t mLeftTopY = 0;
    int32_t mRightDownX = 0;
    int32_t mRightDownY = 0;
    int32_t mWarpinW = 0;
    int32_t mWarpinH = 0;
    int32_t mWarpoutW = 0;
    int32_t mWarpoutH = 0;
    MRectF mDisplayCrop;
    mutable std::mutex mMutex;
};


// ---------------------------------------------------------------------------

}; // namesapce Feature
}; // namespace NSCam

#endif // _MTKCAM_UTILS_FEATURE_CACHE_H_
