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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_GIMG_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_GIMG_H_

#include "SFP_TypeWrapper.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

ILazy<GImg> makeGImg(IImageBuffer *img);
ILazy<GImg> makeGImg(const P2IO &io);
ILazy<GImg> makeGImg(const ImgBuffer &img, const MSize &size = MSize(), const MRectF &crop = MRectF(), MUINT32 dmaMode = ::NSCam::Feature::P2Util::DMAConstrain::DEFAULT, P2IO::TYPE type = P2IO::TYPE_UNKNOWN);
ILazy<GImg> makeGImg(LazyImgBufferPool &pool, const MSize &size = MSize(), const MRect &crop = MRect());
std::vector<P2G::ILazy<P2G::GImg>> makeGImg(const std::vector<P2IO> &ioList);

class GImg_IImageBufferPtr : public GImg
{
public:
    GImg_IImageBufferPtr(IImageBuffer *img);
    P2IO toP2IO() const;
    IImageBuffer* getImageBufferPtr() const;
    MRectF getCrop() const;
    MUINT32 getDmaConstrain() const;
private:
    IImageBuffer *mImg = NULL;
};

class GImg_P2IO : public GImg
{
public:
    GImg_P2IO(const P2IO &io);
    P2IO toP2IO() const;
    IImageBuffer* getImageBufferPtr() const;
    MRectF getCrop() const;
    bool isRotate() const;
    MUINT32 getDmaConstrain() const;
private:
    P2IO mP2IO;
};

class GImg_ImgBuffer : public GImg
{
public:
    GImg_ImgBuffer(const ImgBuffer &img, const P2IO &p2io);
    P2IO toP2IO() const;
    ImgBuffer getImgBuffer() const;
    IImageBuffer* getImageBufferPtr() const;
    MRectF getCrop() const;
    bool isRotate() const;
    MUINT32 getDmaConstrain() const;
private:
    ImgBuffer mImg;
    P2IO mP2IO;
};

class GImg_LazyImgBufferPool : public GImg
{
public:
    GImg_LazyImgBufferPool(const LazyImgBufferPool &pool, const MSize &size, const MRect &crop);
    void acquire(unsigned timeout);
    P2IO toP2IO() const;
    ImgBuffer getImgBuffer() const;
    IImageBuffer* getImageBufferPtr() const;
    MRectF getCrop() const;
    MUINT32 getDmaConstrain() const;
    MSize getImgSize() const;
    P2F::ImageInfo getImageInfo() const;
private:
    android::sp<IBufferPool> mPool;
    P2IO mP2IO;
    ImgBuffer mImg;
};

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_GIMG_H_
