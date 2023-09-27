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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_CORE_DEFINE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_CORE_DEFINE_H_

#include <memory>
#include <vector>

#pragma push_macro("DATA")
#pragma push_macro("POOL")

#include "TypeWrapper.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

struct DataPool
{
  LazyTunBufferPool mTuning;
  LazyTunBufferPool mSyncTuning;
  LazyTunBufferPool mDreTuning;
  LazyTunBufferPool mMsfTuning;
  LazyTunBufferPool mMsfSram;
  LazyTunBufferPool mOmcTuning;

  LazyImgBufferPool mFullImg;
  LazyImgBufferPool mDceso;
  LazyImgBufferPool mTimgo;
  LazyImgBufferPool mConf;
  LazyImgBufferPool mIdi;
  LazyImgBufferPool mOmc;

  std::vector<LazyImgBufferPool> mDsImgs;
  std::vector<LazyImgBufferPool> mDnImgs;
  std::vector<LazyImgBufferPool> mPDsImgs;
  std::vector<LazyImgBufferPool> mMsfWeightImgs;
  std::vector<LazyImgBufferPool> mMsfDsWeightImgs;

  LazyDataPool<TuningData> mTuningData;
  LazyDataPool<P2Obj> mP2Obj;
};

enum class Scene
{
  GM, // general master
  GS, // general slave
  PM, // physical master
  PS, // physical slave
  LM, // large master
  LS, // large slave
};

constexpr Scene sAllScene[] = { Scene::GM, Scene::GS, Scene::PM, Scene::PS, Scene::LM, Scene::LS };

enum class LoopScene
{
  GEN, // general
  PHY, // physical
  LAR, // large
};

constexpr LoopScene sAllLoopScene[] = { LoopScene::GEN, LoopScene::PHY, LoopScene::LAR };

LoopScene toLoopScene(Scene scene);

class Feature
{
public:
  enum FID {
    NONE          = 0,
    DUAL          = 1 << 0,
    SMVR_MAIN     = 1 << 1,
    SMVR_SUB      = 1 << 2,
    NR3D          = 1 << 3,
    DCE           = 1 << 4,
    DRE_TUN       = 1 << 5,
    TIMGO         = 1 << 6,
    DSDN20        = 1 << 7,
    DSDN25        = 1 << 8,
    EARLY_DISP    = 1 << 9,
    DSDN30        = 1 << 10,
    DSDN30_INIT   = 1 << 11,
  };

  Feature();
  Feature(unsigned mask);

  bool has(unsigned mask) const;
  bool hasDUAL() const;
  bool hasSMVR_MAIN() const;
  bool hasSMVR_SUB() const;
  bool hasNR3D() const;
  bool hasDCE() const;
  bool hasDRE_TUN() const;
  bool hasTIMGO() const;
  bool hasDSDN20() const;
  bool hasDSDN25() const;
  bool hasDSDN30() const;

  void add(unsigned mask);
  void operator+=(unsigned mask);
  void operator-=(unsigned mask);
  Feature operator+(Feature rhs) const;

private:
  unsigned mMask = NONE;
};

constexpr Feature::FID sAllFeature[] = { Feature::DUAL, Feature::SMVR_MAIN, Feature::SMVR_SUB, Feature::NR3D, Feature::DCE, Feature::DRE_TUN, Feature::TIMGO, Feature::DSDN20, Feature::DSDN25, Feature::DSDN30, Feature::DSDN30_INIT,Feature::EARLY_DISP };

struct IO_InData
{
  DATA(POD,                   mBasic,           POD());
  DATA(IAdvPQCtrl,            mPQCtrl,          NULL);
  DATA(MSize,                 mIMGISize,        MSize());
  DATA(MRect,                 mSrcCrop,         MRect());

  DATA(IMetadataPtr,          mAppInMeta,       NULL);
  DATA(IMetadataPtr,          mHalInMeta,       NULL);
  DATA(ILazy<GImg>,           mIMGI,            NULL);
  DATA(ILazy<GImg>,           mLCSO,            NULL);
  DATA(ILazy<GImg>,           mLCSHO,           NULL);
  DATA(TunBuffer,             mSyncTun,         NULL);

  DATA(MINT32,                mDCE_n_magic,     DEFAULT_DCE_MAGIC);

  DATA(ILazy<NR3DMotion>,     mNR3DMotion,      NULL);
  DATA(ILazy<DSDNInfo>,       mDSDNInfo,        NULL);

  std::vector<MSize> mDSDNSizes;
  DATA(MSize,                 mConfSize,        MSize());
  DATA(MSize,                 mIdiSize,         MSize());
};

struct IO_OutData
{
  DATA(ILazy<GImg>,           mDisplay,         NULL);
  DATA(ILazy<GImg>,           mRecord,          NULL);
  DATA(ILazy<GImg>,           mFD,              NULL);
  DATA(ILazy<GImg>,           mPhy,             NULL);
  std::vector<ILazy<GImg>>    mExtra;
  std::vector<ILazy<GImg>>    mLarge;
  DATA(ILazy<GImg>,           mFull,            NULL);
  std::vector<ILazy<GImg>>    mNext;
  DATA(ILazy<GImg>,           mDS1,             NULL);
  DATA(ILazy<GImg>,           mDN1,             NULL);
  DATA(ILazy<GImg>,           mOMCMV,           NULL);
  DATA(IMetadataPtr,          mAppOutMeta,      NULL);
  DATA(IMetadataPtr,          mExAppOutMeta,    NULL);
  DATA(IMetadataPtr,          mHalOutMeta,      NULL);
  DATA(ILazy<TunBuffer>,      mTuning,          NULL);
};

struct IO_SubData
{
  DATA(MSize,                 mIMGISize,        NULL);
  DATA(ILazy<GImg>,           mIMGI,            NULL);
  DATA(ILazy<GImg>,           mDSI,             NULL);
  DATA(ILazy<GImg>,           mWEIGHTI,         NULL);
  DATA(ILazy<GImg>,           mREFI,            NULL);
  DATA(ILazy<GImg>,           mDSWI,            NULL);
  DATA(ILazy<GImg>,           m_n_1_DSWI,       NULL);
  DATA(ILazy<GImg>,           mCONFI,           NULL);
  DATA(ILazy<GImg>,           mIDI,             NULL);
  DATA(ILazy<GImg>,           mDNO,             NULL);
  DATA(ILazy<GImg>,           mIMG2O,           NULL);
  DATA(ILazy<GImg>,           mDSWO,            NULL);
};
using SubIO = IO_SubData;

struct IO_LoopData
{
  DATA(POD,                   mBasic,           POD());

  DATA(ILazy<GImg>,           mIMG3O,           NULL);

  DATA(ILazy<GImg>,           mDCE_n_1,         NULL);
  DATA(MINT32,                mDCE_n_1_magic,   DEFAULT_DCE_MAGIC);
  DATA(ILazy<GImg>,           mDCE_n_2,         NULL);
  DATA(MINT32,                mDCE_n_2_magic,   DEFAULT_DCE_MAGIC);

  DATA(ILazy<NR3DStat>,       mNR3DStat,        NULL);

  struct SubLoopData
  {
    DATA(ILazy<GImg>,         mIMG3O,           NULL);
    DATA(ILazy<NR3DStat>,     mNR3DStat,        NULL);
  };

  std::vector<SubLoopData>   mSub;
};

struct IO_BatchData
{
  DATA(ILazy<TunBuffer>,      mTuning,          NULL);
  DATA(ILazy<TuningData>,     mTuningData,      NULL);
  DATA(ILazy<GImg>,           mIMGI,            NULL);

  struct SubBatchData
  {
    DATA(ILazy<TunBuffer>,    mTuning,          NULL);
    DATA(ILazy<TuningData>,   mTuningData,      NULL);
  };

  std::vector<SubBatchData>   mSub;
};

struct P2SW
{
  Feature            mFeature;
  enum TuningType
  {
    P2,
    OMC,
  };

  struct BatchIn
  {
    DATA(ILazy<TuningData>,   mTuningData,      NULL);
  } mBatchIn;

  struct In
  {
    DATA(POD,                 mBasic,           POD());
    DATA(TuningType,          mTuningType,      P2SW::P2);
    DATA(IAdvPQCtrl,          mPQCtrl,          NULL);
    DATA(ILazy<GImg>,         mIMGI,            NULL);

    DATA(IMetadataPtr,        mAppInMeta,       NULL);
    DATA(IMetadataPtr,        mHalInMeta,       NULL);
    DATA(ILazy<GImg>,         mLCSO,            NULL);
    DATA(ILazy<GImg>,         mLCSHO,           NULL);

    DATA(ILazy<GImg>,         mDCE_n_2,         NULL);
    DATA(MINT32,              mDCE_n_2_magic,   DEFAULT_DCE_MAGIC);
    DATA(ILazy<NR3DMotion>,   mNR3DMotion,      NULL);
    DATA(ILazy<NR3DStat>,     mNR3DStat,        NULL);
    DATA(ILazy<DSDNInfo>,     mDSDNInfo,        NULL);
    DATA(ILazy<GImg>,         mVIPI,            NULL);
    DATA(TunBuffer,           mSyncTun,         NULL);

    unsigned                  mDSDNCount = 0;
    unsigned                  mDSDNIndex = 0;
  } mIn;

  struct Out
  {
    DATA(IMetadataPtr,        mAppOutMeta,      NULL);
    DATA(IMetadataPtr,        mExAppOutMeta,    NULL);
    DATA(IMetadataPtr,        mHalOutMeta,      NULL);
    DATA(ILazy<TunBuffer>,    mTuning,          NULL);
    DATA(ILazy<TunBuffer>,    mMSFTuning,       NULL);
    DATA(ILazy<TunBuffer>,    mMSFSram,         NULL);
    DATA(ILazy<TunBuffer>,    mOMCTuning,       NULL);
    DATA(ILazy<TuningData>,   mTuningData,      NULL);
  } mOut;

};

struct P2HW
{
  Feature            mFeature;
  bool               mNeedEarlyCB = false;

  struct In
  {
    DATA(POD,                 mBasic,           POD());

    DATA(ILazy<TunBuffer>,    mTuning,          NULL);
    DATA(ILazy<TunBuffer>,    mMSFTuning,       NULL);
    DATA(ILazy<TunBuffer>,    mMSFSram,         NULL);
    DATA(ILazy<TuningData>,   mTuningData,      NULL);

    DATA(ILazy<GImg>,         mIMGI,            NULL);
    DATA(ILazy<GImg>,         mVIPI,            NULL);
    DATA(ILazy<GImg>,         mDSI,             NULL);
    DATA(ILazy<GImg>,         mREFI,            NULL);
    DATA(ILazy<GImg>,         mDSWI,            NULL);
    DATA(ILazy<GImg>,         m_n_1_DSWI,       NULL);
    DATA(ILazy<GImg>,         mIDI,             NULL);
    DATA(ILazy<GImg>,         mCONFI,           NULL);
    DATA(ILazy<GImg>,         mWEIGHTI,         NULL);

    unsigned                  mDSDNCount = 0;
    unsigned                  mDSDNIndex = 0;

  } mIn;

  struct Out
  {
    DATA(ILazy<GImg>,         mIMG2O,           NULL);
    DATA(ILazy<GImg>,         mIMG3O,           NULL);
    DATA(ILazy<GImg>,         mDCESO,           NULL);
    DATA(ILazy<GImg>,         mTIMGO,           NULL);
    DATA(ILazy<GImg>,         mWDMAO,           NULL);
    DATA(ILazy<GImg>,         mWROTO,           NULL);
    DATA(ILazy<GImg>,         mDSWO,            NULL);
    DATA(ILazy<P2Obj>,        mP2Obj,           NULL);
  } mOut;
};

struct PMDP
{
  struct In
  {
    DATA(ILazy<GImg>,         mFull,            NULL);
    DATA(ILazy<TunBuffer>,    mTuning,          NULL);
    DATA(IAdvPQCtrl,          mPQCtrl,          NULL);
  } mIn;

  struct Out
  {
    std::vector<ILazy<GImg>>  mExtra;
  } mOut;
};

struct MSS
{
  Feature            mFeature;

  struct In
  {
    DATA(MUINT32,             mSensorID,       (MUINT32)(-1));
    DATA(ILazy<GImg>,         mMSSI,            NULL);
    DATA(ILazy<GImg>,         mOMCMV,           NULL);
    DATA(ILazy<TunBuffer>,    mOMCTuning,       NULL);
    std::vector<MSize>        mMSSOSizes;
  } mIn;

  struct Out
  {
    std::vector<ILazy<GImg>>  mMSSOs;
  } mOut;
  bool mValid = false;
};

class IO
{
public:
  using InData = IO_InData;
  using OutData = IO_OutData;
  using LoopData = std::shared_ptr<IO_LoopData>;
  using LoopData_const = std::shared_ptr<const IO_LoopData>;
  using BatchData = std::shared_ptr<IO_BatchData>;
  using BatchData_const = std::shared_ptr<const IO_BatchData>;

public:
  IO() {}
  IO(unsigned feature) : mFeature(feature) {}
  IO(Scene scene) : mScene(scene) {}
  IO(Scene scene, unsigned feature) : mScene(scene), mFeature(feature) {}

  bool has(Feature::FID fid) const;

public:
  static LoopData makeLoopData();
  static BatchData makeBatchData();

public:
  Scene                                       mScene = Scene::GM;
  Feature                                     mFeature;
  InData                                      mIn;
  OutData                                     mOut;
  LoopData_const                              mLoopIn;
  LoopData                                    mLoopOut;
  BatchData_const                             mBatchIn;
  BatchData                                   mBatchOut;
};

class Path
{
public:
  void clear() { *this = Path(); }

public:
  std::vector<P2SW>                           mP2SW;
  std::vector<P2HW>                           mP2HW;
  std::vector<PMDP>                           mPMDP;
  std::vector<MSS>                            mMSS;
  std::vector<MSS>                            mPMSS;
};

const char *toName(Scene scene);
const char *toName(Feature::FID fid);
bool isMaster(Scene scene);

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#pragma pop_macro("DATA")
#pragma pop_macro("POOL")

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_CORE_DEFINE_H_
