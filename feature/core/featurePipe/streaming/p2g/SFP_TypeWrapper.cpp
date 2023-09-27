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

#include "SFP_TypeWrapper.h"
#include "GImg.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

LazyTunBufferPool::LazyTunBufferPool()
{
}

LazyTunBufferPool::LazyTunBufferPool(const sp<TuningBufferPool> &pool)
    : mPool(pool)
{
}

MVOID LazyTunBufferPool::setNeed(MUINT32 need)
{
    mNeed = need;
}

MVOID LazyTunBufferPool::destroy()
{
    TuningBufferPool::destroy(mPool);
}

ILazy<TunBuffer> LazyTunBufferPool::requestLazy()
{
    return makeLazy<TunBuffer>();
}

TunBuffer LazyTunBufferPool::requestBuffer(eMem memclr) const
{
    TunBuffer buf = mPool ? mPool->request() : NULL;
    if( (buf != NULL) && memclr )
    {
        memset(buf->mpVA, 0, mPool->getBufSize());
    }
    return buf;
}

MUINT32 LazyTunBufferPool::getNeed() const
{
    return mNeed;
}

TuningBufferPool* LazyTunBufferPool::get() const
{
    return mPool.get();
}

android::sp<TuningBufferPool> LazyTunBufferPool::getPool() const
{
    return mPool;
}

LazyTunBufferPool::operator bool() const
{
    return mPool.get();
}

LazyImgBufferPool::LazyImgBufferPool()
{
}

LazyImgBufferPool::LazyImgBufferPool(const android::sp<IBufferPool> &pool)
    : mPool(pool)
{
}

MVOID LazyImgBufferPool::setNeed(MUINT32 need)
{
    mNeed = need;
}

MVOID LazyImgBufferPool::destroy()
{
    IBufferPool::destroy(mPool);
}

ILazy<ImgBuffer> LazyImgBufferPool::requestLazy()
{
    return makeLazy<ImgBuffer>();
}

ImgBuffer LazyImgBufferPool::requestBuffer()
{
    return mPool ? mPool->requestIIBuffer() : NULL;
}

MUINT32 LazyImgBufferPool::getNeed() const
{
    return mNeed;
}

IBufferPool* LazyImgBufferPool::get() const
{
    return mPool.get();
}

android::sp<IBufferPool> LazyImgBufferPool::getPool() const
{
    return mPool;
}

P2F::ImageInfo LazyImgBufferPool::getImageInfo() const
{
    return mPool ? mPool->getImageInfo() : P2F::ImageInfo();
}

LazyImgBufferPool::operator bool() const
{
    return mPool.get();
}

GImg::~GImg()
{
}

void GImg::acquire(unsigned /*timeout*/)
{
}

ImgBuffer GImg::getImgBuffer() const
{
    return NULL;
}

MSize GImg::getImgSize() const
{
    MSize size;
    IImageBuffer *img = getImageBufferPtr();
    if( img )
    {
        size = img->getImgSize();
    }
    return size;
}

EImageFormat GImg::getImgFormat() const
{
    EImageFormat fmt = eImgFmt_UNKNOWN;
    IImageBuffer *img = getImageBufferPtr();
    if( img )
    {
        fmt = (EImageFormat)img->getImgFormat();
    }
    return fmt;
}

P2F::ImageInfo GImg::getImageInfo() const
{
    P2F::ImageInfo info;
    IImageBuffer *img = getImageBufferPtr();
    if( img )
    {
        MUINT32 plane = img->getPlaneCount();
        info.mFormat = (EImageFormat)img->getImgFormat();
        info.mSize = img->getImgSize();
        info.mStrideInBytes.reserve(plane);
        for( MUINT32 i = 0; i < plane; ++i )
        {
            info.mStrideInBytes.push_back(img->getBufStridesInBytes(i));
        }
    }
    return info;
}

bool GImg::isRotate() const
{
    return false;
}

ILazy<GImg> GImg::make(LazyImgBufferPool &pool, const MSize &size, const MRect &crop)
{
    return makeGImg(pool, size, crop);
}

bool GImg::isRotate(const ILazy<GImg> &img)
{
    return img && img->isRotate();
}

IImageBuffer* peak(const ILazy<GImg> &img)
{
    return img ? img->getImageBufferPtr() : NULL;
}

IImageBuffer* acquirePeak(const ILazy<GImg> &img, unsigned /*timeout*/)
{
    IImageBuffer *out = NULL;
    if( img )
    {
        img->acquire();
        out = img->getImageBufferPtr();
    }
    return out;
}

void* peak(const ILazy<TunBuffer> &buffer)
{
    return (buffer && (*buffer != NULL)) ? (*buffer)->mpVA : NULL;
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
