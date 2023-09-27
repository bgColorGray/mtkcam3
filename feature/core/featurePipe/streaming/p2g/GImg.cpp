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

#include "GImg.h"

#include "../DebugControl.h"
#define PIPE_CLASS_TAG "P2G_MGR"
#define PIPE_TRACE TRACE_P2G_MGR
#include <featurePipe/core/include/PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

ILazy<GImg> makeGImg(IImageBuffer *img)
{
    return makeLazy<GImg_IImageBufferPtr>(img);
}

ILazy<GImg> makeGImg(const P2IO &io)
{
    return makeLazy<GImg_P2IO>(io);
}

ILazy<GImg> makeGImg(const ImgBuffer &img, const MSize &size, const MRectF &crop, MUINT32 dmaMode, P2IO::TYPE type)
{
    P2IO p2io;
    p2io.mCropDstSize = size;
    p2io.mCropRect = crop;
    p2io.mDMAConstrain = dmaMode;
    p2io.mType = type;
    return makeLazy<GImg_ImgBuffer>(img, p2io);
}

ILazy<GImg> makeGImg(LazyImgBufferPool &pool, const MSize &size, const MRect &crop)
{
    return makeLazy<GImg_LazyImgBufferPool>(pool, size, crop);
}

std::vector<P2G::ILazy<P2G::GImg>> makeGImg(const std::vector<P2IO> &ioList)
{
    std::vector<P2G::ILazy<P2G::GImg>> imgs;
    imgs.reserve(ioList.size());
    for( const P2IO &io : ioList )
    {
        imgs.emplace_back(P2G::makeGImg(io));
    }
    return imgs;
}


GImg_IImageBufferPtr::GImg_IImageBufferPtr(IImageBuffer *img)
    : mImg(img)
{
}

P2IO GImg_IImageBufferPtr::toP2IO() const
{
    P2IO io;
    io.mBuffer = mImg;
    return io;
}

IImageBuffer* GImg_IImageBufferPtr::getImageBufferPtr() const
{
    return mImg;
}

MRectF GImg_IImageBufferPtr::getCrop() const
{
    return MRectF(0, 0);
}

MUINT32 GImg_IImageBufferPtr::getDmaConstrain() const
{
    return ::NSCam::Feature::P2Util::DMAConstrain::DEFAULT;
}

GImg_P2IO::GImg_P2IO(const P2IO &io)
    : mP2IO(io)
{
}

P2IO GImg_P2IO::toP2IO() const
{
    return mP2IO;
}

IImageBuffer* GImg_P2IO::getImageBufferPtr() const
{
    return mP2IO.mBuffer;
}

MRectF GImg_P2IO::getCrop() const
{
    return mP2IO.mCropRect;
}

bool GImg_P2IO::isRotate() const
{
    return mP2IO.mTransform;
}

MUINT32 GImg_P2IO::getDmaConstrain() const
{
    return mP2IO.mDMAConstrain;
}

GImg_ImgBuffer::GImg_ImgBuffer(const ImgBuffer &img, const P2IO &io)
    : mImg(img)
    , mP2IO(io)
{
    mP2IO.mBuffer = getImageBufferPtr();
    if( mP2IO.mBuffer )
    {
        if( mP2IO.mCropDstSize.size() &&
            (mP2IO.mCropDstSize != mP2IO.mBuffer->getImgSize()) )
        {
            mP2IO.mBuffer->setExtParam(mP2IO.mCropDstSize);
        }
    }
}

P2IO GImg_ImgBuffer::toP2IO() const
{
    return mP2IO;
}

ImgBuffer GImg_ImgBuffer::getImgBuffer() const
{
    return mImg;
}

IImageBuffer* GImg_ImgBuffer::getImageBufferPtr() const
{
    return mImg ? mImg->getImageBufferPtr() : NULL;
}

MRectF GImg_ImgBuffer::getCrop() const
{
    return mP2IO.mCropRect;
}

bool GImg_ImgBuffer::isRotate() const
{
    return mP2IO.mTransform;
}


MUINT32 GImg_ImgBuffer::getDmaConstrain() const
{
    return mP2IO.mDMAConstrain;
}

GImg_LazyImgBufferPool::GImg_LazyImgBufferPool(const LazyImgBufferPool &pool, const MSize &size, const MRect &crop)
    : mPool(pool.getPool())
{
    mP2IO.mCropDstSize = size;
    mP2IO.mCropRect = crop;
}

void GImg_LazyImgBufferPool::acquire(unsigned timeout)
{
    (void)timeout;
    if( !mImg && mPool )
    {
        mImg = mPool->requestIIBuffer();
        if( mImg )
        {
            if( mP2IO.mCropDstSize.size() )
            {
                mImg->setExtParam(mP2IO.mCropDstSize);
            }
            mP2IO.mBuffer = mImg->getImageBufferPtr();
        }
    }
}

P2IO GImg_LazyImgBufferPool::toP2IO() const
{
    return mP2IO;
}

ImgBuffer GImg_LazyImgBufferPool::getImgBuffer() const
{
    return mImg;
}

IImageBuffer* GImg_LazyImgBufferPool::getImageBufferPtr() const
{
    return mP2IO.mBuffer;
}

MRectF GImg_LazyImgBufferPool::getCrop() const
{
    return mP2IO.mCropRect;
}

MSize GImg_LazyImgBufferPool::getImgSize() const
{
    return mP2IO.mCropDstSize.size() ? mP2IO.mCropDstSize :
           mP2IO.mBuffer ? mP2IO.mBuffer->getImgSize() :
           MSize();
}

MUINT32 GImg_LazyImgBufferPool::getDmaConstrain() const
{
    return mP2IO.mDMAConstrain;
}

P2F::ImageInfo GImg_LazyImgBufferPool::getImageInfo() const
{
    P2F::ImageInfo info;
    if( mPool )
    {
        info = mPool->getImageInfo();
        if( mP2IO.mCropDstSize.size() )
        {
            info.mSize = mP2IO.mCropDstSize;
        }
    }
    return info;
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
