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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_WRAPPER_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_WRAPPER_H_

#include <featurePipe/core/include/ImageBufferPool.h>
#include <featurePipe/core/include/TuningBufferPool.h>
#include <mtkcam3/feature/utils/p2/P2IO.h>
#include <mtkcam3/feature/utils/p2/P2Util.h>
#include <mtkcam3/feature/utils/p2/P2Pack.h>
#include "../StreamingFeatureType.h"

#include <memory>

using ::NSCam::Feature::P2Util::IPQCtrl_const;
using ::NSCam::Feature::P2Util::P2IO;
using ::NSCam::Feature::P2Util::P2Obj;
using ::NSCam::Feature::P2Util::P2Pack;
using ::NSCam::NSIoPipe::NSPostProc::ENormalStreamTag_Normal;
using ::NSCam::P2F::ImageInfo;

#define DATA(type, var, def)                  type var = def
#define POOL(type, var, name, buf)            type var = NULL

#define DEFAULT_DCE_MAGIC -1

using IMetadataPtr = IMetadata*;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

// Common and life cycle insensitive data
class POD
{
public:
    using P2Pack = Feature::P2Util::P2Pack;
    using ENOrmalStreamTag = NSIoPipe::NSPostProc::ENormalStreamTag;
    using P2_RUN_INDEX = NSIoPipe::P2_RUN_INDEX;

public:
    MUINT32           mSensorIndex = (MUINT32)(-1);
    MUINT32           mFDSensorID = (MUINT32)(-1);
    P2Pack            mP2Pack;
    IPQCtrl_const     mBasicPQ;
    ENormalStreamTag  mP2Tag = NSIoPipe::NSPostProc::ENormalStreamTag_Normal;
    P2_RUN_INDEX mP2DebugIndex = NSIoPipe::P2_RUN_UNKNOWN;
    MBOOL             mRegDump = MFALSE;
    MBOOL             mResized = MFALSE;
    MBOOL             mIsYuvIn = MFALSE;
    MBOOL             mIsMaster = MTRUE;
    MRect             mSrcCrop;
    IMRect            mFDCrop;
    MUINT32           mTimgoType = 0;
    MINT32            mDCE_n_magic = DEFAULT_DCE_MAGIC;

    MUINT32           mDSDNTotalLayer = 0;
};

template <typename T>
using ILazy = std::shared_ptr<T>;

template <typename T, typename... Args>
ILazy<T> makeLazy(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T>
class LazyDataPool
{
public:
    operator bool() const { return true; }
    ILazy<T> requestLazy() { return makeLazy<T>(); }
};

class LazyTunBufferPool
{
public:
    enum eMem { NO_MEMCLR = 0, MEMCLR = 1 };

public:
    LazyTunBufferPool();
    LazyTunBufferPool(const android::sp<TuningBufferPool> &pool);

    MVOID setNeed(MUINT32 need);
    MVOID destroy();
    ILazy<TunBuffer> requestLazy();
    TunBuffer requestBuffer(eMem = NO_MEMCLR) const;

    MUINT32 getNeed() const;
    TuningBufferPool* get() const;
    android::sp<TuningBufferPool> getPool() const;
    operator bool() const;

private:
    android::sp<TuningBufferPool> mPool;
    MUINT32 mNeed = 0;
};

class LazyImgBufferPool
{
public:
    LazyImgBufferPool();
    LazyImgBufferPool(const android::sp<IBufferPool> &pool);

    MVOID setNeed(MUINT32 need);
    MVOID destroy();

    ILazy<ImgBuffer> requestLazy();
    ImgBuffer requestBuffer();

    MUINT32 getNeed() const;
    IBufferPool* get() const;
    android::sp<IBufferPool> getPool() const;
    P2F::ImageInfo getImageInfo() const;
    operator bool() const;

private:
    android::sp<IBufferPool> mPool;
    MUINT32 mNeed = 0;
};

class GImg
{
public:
    virtual ~GImg();
    virtual void acquire(unsigned timeout = 0);
    virtual ImgBuffer getImgBuffer() const;
    virtual P2IO toP2IO() const = 0;
    virtual IImageBuffer* getImageBufferPtr() const = 0;
    virtual MRectF getCrop() const = 0;
    virtual MUINT32 getDmaConstrain() const = 0;
    virtual MSize getImgSize() const;
    virtual EImageFormat getImgFormat() const;
    virtual P2F::ImageInfo getImageInfo() const;
    virtual bool isRotate() const;

public:
    static ILazy<GImg> make(LazyImgBufferPool &pool, const MSize &size = MSize(), const MRect &crop = MRect());
    static bool isRotate(const ILazy<GImg> &img);
};

IImageBuffer* peak(const ILazy<GImg> &img);
IImageBuffer* acquirePeak(const ILazy<GImg> &img, unsigned timeout = 0);
void* peak(const ILazy<TunBuffer> &buffer);

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_WRAPPER_H_
