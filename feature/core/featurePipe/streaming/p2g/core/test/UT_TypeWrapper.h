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

#include "UT_FakePool.h"
//using namespace NSCam::NSCamFeature::NSFeaturePipe;

#define DATA(type, var, def)    type var = type()
#define POOL(type, var, name, buf) FakePool<buf> var = FakePool<buf>(name)

#define DECL_FAKE_CLASS(name)   \
class _##name {};               \
using name = FakeData<_##name>;

#define DECL_FAKE_PTR_CLASS_TEMPLATE(name)                  \
template<typename T> class name : public IFakeData          \
{                                                           \
public:                                                     \
  name() {}                                                 \
  name(decltype(NULL) n) {}                                 \
  template <typename... Args>                               \
  name(Args&&... args) : IFakeData(args...) {}              \
  name<T>& operator=(decltype(NULL) rhs)                    \
  {                                                         \
    *this = name<T>();                                      \
    return *this; }                                         \
};

DECL_FAKE_PTR_CLASS_TEMPLATE(ILazy);
DECL_FAKE_PTR_CLASS_TEMPLATE(sp);

DECL_FAKE_CLASS(ENormalStreamTag);
DECL_FAKE_CLASS(IAdvPQCtrl);
DECL_FAKE_CLASS(IAdvPQCtrl_const);
DECL_FAKE_CLASS(ImgBuffer);
DECL_FAKE_CLASS(ImageInfo);
DECL_FAKE_CLASS(IMetadata);
DECL_FAKE_CLASS(IMetadataPtr);
DECL_FAKE_CLASS(IMRect);
DECL_FAKE_CLASS(IMRectF);
DECL_FAKE_CLASS(MBOOL);
DECL_FAKE_CLASS(MINT32);
DECL_FAKE_CLASS(MRect);
DECL_FAKE_CLASS(MSize);
DECL_FAKE_CLASS(MUINT32);
DECL_FAKE_CLASS(NR3DMotion);
DECL_FAKE_CLASS(NR3DStat);
DECL_FAKE_CLASS(P2IO);
DECL_FAKE_CLASS(P2Obj);
DECL_FAKE_CLASS(P2Pack);
DECL_FAKE_CLASS(POD);
DECL_FAKE_CLASS(TunBuffer);
DECL_FAKE_CLASS(TuningData);

template <typename T>
class LazyDataPool
{
public:
  LazyDataPool() {}
  LazyDataPool(const std::string &name) : mValid(true), mPool(name) {}
  operator bool() const { return mValid; }
  ILazy<T> requestLazy() { return mPool.request(); }
private:
  bool mValid = false;
  FakePool<ILazy<T>> mPool;
};

using LazyTunBufferPool = LazyDataPool<TunBuffer>;
using LazyImgBufferPool = LazyDataPool<ImgBuffer>;

class GImg
{
public:
  operator bool() const;

public:
  static ILazy<GImg> make(LazyImgBufferPool &pool, const MSize &size = MSize(), const MRect &crop = MRect());
  static bool isRotate(const ILazy<GImg> &img);
};
#define DECL_FAKE_POOL(type, name) using name = FakePool<type>;
DECL_FAKE_POOL(ILazy<GImg>,         FakeImgPool);
DECL_FAKE_POOL(IMetadataPtr,        FakeMetaPool);
DECL_FAKE_POOL(POD,                 FakePODPool);
DECL_FAKE_POOL(IMRect,              FakeRecordCropPool);
DECL_FAKE_POOL(P2Pack,              FakeP2PackPool);

DECL_FAKE_POOL(ILazy<TunBuffer>,    FakeTunPool);
DECL_FAKE_POOL(ILazy<TuningData>,   FakeTunDataPool);
DECL_FAKE_POOL(TunBuffer,           FakeTunBufPool);

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_WRAPPER_H_
