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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#ifndef _MTKCAM_FEATURE_UTILS_P2_IO_H_
#define _MTKCAM_FEATURE_UTILS_P2_IO_H_

#include <mtkcam/def/common.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>

#include <mtkcam3/feature/utils/p2/PQCtrl.h>

namespace NSCam {
namespace Feature {
namespace P2Util {

class DMAConstrain
{
public:
    enum Flag
    {
        NONE        = 0,
        ALIGN2BYTE  = 1 << 0,  // p2s original usage
        NOSUBPIXEL  = 1 << 1,  // disable MDP sub-pixel
    };
    static constexpr MUINT32 const DEFAULT = (DMAConstrain::ALIGN2BYTE|DMAConstrain::NOSUBPIXEL);
};

class P2Flag
{
public:
    enum
    {
        FLAG_NONE       = 0,
        FLAG_RESIZED    = 1 << 1,
        FLAG_LMV        = 1 << 2,
    };
};

class P2IO
{
public:
    enum TYPE
    {
        TYPE_UNKNOWN    = 0,
        TYPE_DISPLAY,
        TYPE_RECORD,
        TYPE_VSS,
        TYPE_FD,
        TYPE_APP_DEPTH,
    };
    P2IO() {}
    P2IO(IImageBuffer *buf, MINT32 trans, P2IO::TYPE type)
    : mBuffer(buf)
    , mTransform(trans)
    , mType(type)
    {}

    P2IO(IImageBuffer *buf, MINT32 trans, P2IO::TYPE type,
         const MRectF &crop, const MSize &size, MUINT32 dmaConstrain)
    : mBuffer(buf)
    , mTransform(trans)
    , mType(type)
    , mCropRect(crop)
    , mCropDstSize(size)
    , mDMAConstrain(dmaConstrain)
    {}

    P2IO(const NSCam::NSIoPipe::Output &out, const NSCam::NSIoPipe::MCrpRsInfo &crop)
    : mBuffer(out.mBuffer)
    , mTransform(out.mTransform)
    , mCropRect(MRectF(crop.mCropRect.p_integral, crop.mCropRect.s))
    , mCropDstSize(crop.mResizeDst)
    {
        switch (out.mPortID.capbility)
        {
            case NSIoPipe::EPortCapbility_Disp:
                mType = TYPE_DISPLAY;
                break;
            case NSIoPipe::EPortCapbility_Rcrd:
                mType = TYPE_RECORD;
                break;
            default:
                mType = TYPE_VSS;
                break;
        }
    }


    operator bool() const { return isValid(); }
    MBOOL isValid() const { return mBuffer != NULL; }
    MSize getImgSize() const
    {
        return mBuffer ? mBuffer->getImgSize() : MSize(0,0);
    }
    MSize getTransformSize() const
    {
        MSize size(0, 0);
        if( mBuffer )
        {
            size = mBuffer->getImgSize();
            if( static_cast<MUINT32>(mTransform) & eTransform_ROT_90 )
            {
                size = MSize(size.h, size.w);
            }
        }
        return size;
    }
    MBOOL isCropValid() const
    {
        return mCropRect.s.w > 0 && mCropRect.s.h > 0 &&
               mCropDstSize.w > 0 && mCropDstSize.h > 0;
    }

public:
    IImageBuffer *mBuffer = NULL;
    MUINT32 mTransform = 0;
    TYPE mType = TYPE_UNKNOWN;

    MRectF mCropRect;
    MSize mCropDstSize;
    MUINT32 mDMAConstrain = DMAConstrain::DEFAULT;
    MUINT32 mTimestampCode = 0;
    std::shared_ptr<const PQCtrl> mPQCtrl;
};

class P2IOPack
{
public:
    MBOOL isResized() const { return mFlag & P2Flag::FLAG_RESIZED; }
    MBOOL useLMV() const { return mFlag & P2Flag::FLAG_LMV; }
    MBOOL isValid() const
    {
        return mIMGI.isValid() &&
               (mWROTO.isValid() || mWDMAO.isValid() ||
                mIMG3O.isValid() || mIMG2O.isValid());
    }

public:
    MUINT32 mFlag = P2Flag::FLAG_NONE;

    P2IO mIMGI;
    P2IO mVIPI;

    IImageBuffer *mMSFDSI = NULL;
    IImageBuffer *mMSFREFI = NULL;
    IImageBuffer *mMSFDSWI = NULL;
    IImageBuffer *mMSF_n_1_DSWI = NULL;

    IImageBuffer *mMSFCONFI = NULL;
    IImageBuffer *mMSFIDI = NULL;
    IImageBuffer *mMSFWEIGHTI = NULL;
    IImageBuffer *mMSFDSWO = NULL;

    P2IO mIMG2O;
    P2IO mIMG3O;
    P2IO mWDMAO;
    P2IO mWROTO;

    P2IO mLCSO;
    P2IO mLCSHO;
    P2IO mDCESO;
    P2IO mTIMGO;
};

} // namespace P2Util
} // namespace Feature
} // namespace NSCam

#endif // _MTKCAM_FEATURE_UTILS_P2_IO_H_
