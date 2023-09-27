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

#include "PathEngine.h"

#ifdef P2G_ENGINE_UT
  #include "test/UT_LogUtil.h"
#else // P2G_ENGINE_UT
  #include "../../DebugControl.h"
  #define PIPE_CLASS_TAG "P2G_PathEngine"
  #define PIPE_TRACE TRACE_P2G_PATH_ENGINE
  #include <featurePipe/core/include/PipeLog.h>

  CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);
#endif // P2G_ENGINE_UT

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

PathEngine::PathEngine()
{
}

PathEngine::~PathEngine()
{
  if( mState != State::NONE )
  {
    uninit();
  }
}

bool PathEngine::init(const DataPool &pool, const Feature &featureSet, bool is4K, bool is60fps,bool supportIMG3O)
{
  bool ret = true;
  if( mState == State::NONE )
  {
    mSupMultiRotate = false; // or need to query MDP rotate capability with different platform
    mPool = pool;
    mIs4K = is4K;
    mIs60FPS = is60fps;
    mFeatureSet = featureSet;
    mUseIMG3O = supportIMG3O;
    ret = initCheck(mPool, mFeatureSet);
    mState = ret ? State::READY : State::FAIL;
  }
  return ret;
}

bool PathEngine::uninit()
{
  if( mState != State::NONE )
  {
    mPool = DataPool();
    mState = State::NONE;
  }
  return true;
}

MSS PathEngine::prepareMSS(const IO &io)
{
  MSS mss;
  if( io.has(Feature::DSDN25) || io.has(Feature::DSDN30) || io.has(Feature::DSDN30_INIT) )
  {
    unsigned count = io.mIn.mDSDNSizes.size();
    mss.mFeature          = io.mFeature;
    mss.mIn.mMSSI         = io.mIn.mIMGI;
    mss.mIn.mSensorID     = io.mIn.mBasic.mSensorIndex;
    mss.mIn.mMSSOSizes    = io.mIn.mDSDNSizes;
    mss.mOut.mMSSOs.resize(count);
    for( unsigned i = 0; i < count; ++i )
    {
      mss.mOut.mMSSOs[i] = GImg::make(mPool.mDsImgs[i], io.mIn.mDSDNSizes[i]);
    }
    mss.mValid = true;
  }
  return mss;
}

MSS PathEngine::preparePMSS(const IO &io, const MSS &mss)
{
  MSS pmss;
  unsigned total = io.mIn.mDSDNSizes.size();
  if( (io.has(Feature::DSDN30) || io.has(Feature::DSDN30_INIT)) && total && mss.mOut.mMSSOs.size())
  {
    unsigned count = total; // currently Ln is no used, but still output by hw
    bool loopValid = io.has(Feature::DSDN30) && io.mLoopIn && io.mLoopIn->mSub.size();
    pmss.mFeature          = io.mFeature;
    pmss.mIn.mMSSI         = loopValid ? io.mLoopIn->mSub[0].mIMG3O : mss.mOut.mMSSOs[0];
    pmss.mIn.mSensorID     = io.mIn.mBasic.mSensorIndex;
    pmss.mIn.mOMCMV        = io.mOut.mOMCMV;
    pmss.mIn.mOMCTuning    = mPool.mOmcTuning.requestLazy();
    pmss.mIn.mMSSOSizes    = io.mIn.mDSDNSizes;
    pmss.mOut.mMSSOs.resize(count);
    for( unsigned i = 0; i < count; ++i )
    {
      pmss.mOut.mMSSOs[i] = GImg::make(mPool.mPDsImgs[i], io.mIn.mDSDNSizes[i]);
    }
    pmss.mValid = true;
  }
  return pmss;
}

bool PathEngine::prepareP2SW(const Feature &f, const IO &io, const SubIO &subIO, unsigned dsdnIndex, P2SW &p2sw)
{
  TRACE_FUNC_ENTER();
  unsigned subIndex = dsdnIndex ? dsdnIndex - 1: 0;
  bool need = true;

  p2sw.mFeature             = f;
  p2sw.mIn.mBasic           = io.mIn.mBasic;
  p2sw.mIn.mAppInMeta       = io.mIn.mAppInMeta;
  p2sw.mIn.mHalInMeta       = io.mIn.mHalInMeta;
  p2sw.mIn.mDSDNInfo        = io.mIn.mDSDNInfo;
  p2sw.mIn.mDSDNCount       = io.mIn.mDSDNSizes.size();
  p2sw.mIn.mDSDNIndex       = dsdnIndex;

  if( !f.has(Feature::SMVR_SUB) )
  {
    if( dsdnIndex == 0 )
    {
      p2sw.mIn.mPQCtrl          = io.mIn.mPQCtrl;
      p2sw.mIn.mSyncTun         = io.mIn.mSyncTun;
      p2sw.mIn.mLCSO            = io.mIn.mLCSO;
      p2sw.mIn.mLCSHO           = io.mIn.mLCSHO;
      p2sw.mIn.mIMGI            = io.mIn.mIMGI;
      p2sw.mIn.mNR3DMotion      = io.mIn.mNR3DMotion;
      p2sw.mOut.mAppOutMeta     = io.mOut.mAppOutMeta;
      p2sw.mOut.mExAppOutMeta   = io.mOut.mExAppOutMeta;
      p2sw.mOut.mHalOutMeta     = io.mOut.mHalOutMeta;
      p2sw.mOut.mTuning         = io.mOut.mTuning ? io.mOut.mTuning
                                                  : mPool.mTuning.requestLazy();
      if( io.mLoopIn )
      {
        if( f.has(Feature::DCE) )
        {
          p2sw.mIn.mDCE_n_2         = io.mLoopIn->mDCE_n_2;
          p2sw.mIn.mDCE_n_2_magic   = io.mLoopIn->mDCE_n_2_magic;
        }
        if( f.has(Feature::NR3D) )
        {
          p2sw.mIn.mVIPI            = io.mLoopIn->mIMG3O;
          p2sw.mIn.mNR3DStat        = io.mLoopIn->mNR3DStat;
        }
      }
    }
    else // if( dsdnIndex > 0 )
    {
      p2sw.mIn.mIMGI            = subIO.mIMGI;
      p2sw.mIn.mNR3DMotion      = io.mIn.mNR3DMotion;
      if( f.has(Feature::NR3D) && io.mLoopIn &&
          subIndex < io.mLoopIn->mSub.size() )
      {
        p2sw.mIn.mVIPI          = io.mLoopIn->mSub[subIndex].mIMG3O;
        p2sw.mIn.mNR3DStat      = io.mLoopIn->mSub[subIndex].mNR3DStat;
      }
      p2sw.mOut.mTuning         = mPool.mTuning.requestLazy();
      if( f.hasDSDN30() )
      {
        p2sw.mOut.mMSFTuning    = mPool.mMsfTuning.requestLazy();
        p2sw.mOut.mMSFSram      = mPool.mMsfSram.requestLazy();
      }
    }
    p2sw.mOut.mTuningData       = mPool.mTuningData.requestLazy();
  }
  else // if( io.has(Feature::SMVR_SUB) )
  {
    need = ( io.mIn.mIMGI != io.mBatchIn->mIMGI );
    if( dsdnIndex == 0 )
    {
      p2sw.mIn.mLCSO            = io.mIn.mLCSO;
      p2sw.mBatchIn.mTuningData = io.mBatchIn->mTuningData;
      p2sw.mOut.mTuning         = io.mBatchIn->mTuning;
      p2sw.mOut.mTuningData     = need ? mPool.mTuningData.requestLazy()
                                       : io.mBatchIn->mTuningData;
    }
    else if( io.mBatchIn->mSub.size() > subIndex )
    {
      p2sw.mBatchIn.mTuningData = io.mBatchIn->mSub[subIndex].mTuningData;
      p2sw.mOut.mTuning         = io.mBatchIn->mSub[subIndex].mTuning;
      p2sw.mOut.mTuningData     = need ? mPool.mTuningData.requestLazy()
                                  : io.mBatchIn->mSub[subIndex].mTuningData;
    }
  }
  TRACE_FUNC_EXIT();
  return need;
}

bool PathEngine::prepareP2HW(const Feature &f, const IO &io, const SubIO &subIO, unsigned dsdnIndex, const P2SW &p2sw, P2HW &p2hw, PMDP &pmdp)
{
  TRACE_FUNC_ENTER();
  unsigned subIndex = dsdnIndex ? dsdnIndex - 1: 0;
  p2hw.mFeature           = f;
  p2hw.mNeedEarlyCB       = f.hasDSDN30() && (dsdnIndex == 1);
  p2hw.mIn.mBasic         = io.mIn.mBasic;
  p2hw.mIn.mTuning        = p2sw.mOut.mTuning;
  p2hw.mIn.mMSFTuning     = p2sw.mOut.mMSFTuning;
  p2hw.mIn.mMSFSram       = p2sw.mOut.mMSFSram;
  p2hw.mIn.mTuningData    = p2sw.mOut.mTuningData;
  p2hw.mOut.mP2Obj        = mPool.mP2Obj.requestLazy();
  p2hw.mIn.mDSDNCount     = io.mIn.mDSDNSizes.size();
  p2hw.mIn.mDSDNIndex     = dsdnIndex;

  if( dsdnIndex == 0 )
  {
    p2hw.mIn.mIMGI          = io.mIn.mIMGI;
    p2hw.mIn.mDSI           = subIO.mDSI;
    if( f.has(Feature::NR3D) && io.mLoopIn )
    {
      p2hw.mIn.mVIPI        = io.mLoopIn->mIMG3O;
    }
    if( f.has(Feature::DCE) )
    {
      p2hw.mOut.mDCESO = GImg::make(mPool.mDceso);
    }
    if( f.has(Feature::TIMGO) )
    {
      p2hw.mOut.mTIMGO = GImg::make(mPool.mTimgo);
    }
    p2hw.mOut.mIMG2O        = subIO.mIMG2O;
    prepareP2GOut(io, p2hw, pmdp);
  }
  else // if( dsdnIndex > 0 )
  {
    p2hw.mIn.mIMGI = subIO.mIMGI;
    p2hw.mIn.mDSI = subIO.mDSI;
    if( f.has(Feature::NR3D) && io.mLoopIn &&
        subIndex < io.mLoopIn->mSub.size() )
    {
      p2hw.mIn.mVIPI = io.mLoopIn->mSub[subIndex].mIMG3O;
    }
    p2hw.mOut.mIMG3O = subIO.mDNO;
    p2hw.mOut.mIMG2O = subIO.mIMG2O;
    if( f.hasDSDN30() )
    {
      p2hw.mIn.mREFI        = subIO.mREFI;
      p2hw.mIn.mDSWI        = subIO.mDSWI;
      p2hw.mIn.m_n_1_DSWI   = subIO.m_n_1_DSWI;
      p2hw.mIn.mCONFI       = subIO.mCONFI;
      p2hw.mIn.mIDI         = subIO.mIDI;
      p2hw.mIn.mWEIGHTI     = subIO.mWEIGHTI;
      p2hw.mOut.mDSWO       = subIO.mDSWO;
    }
  }
  TRACE_FUNC_EXIT();
  return true;
}

void PathEngine::fillFull(P2HW &p2hw, bool &wdmaUsed, bool &wrotUsed, const ILazy<GImg> &full)
{
  if( full )
  {
    if( mUseIMG3O )
    {
      p2hw.mOut.mIMG3O = full;
    }
    else
    {
      fill(p2hw, wdmaUsed, wrotUsed, full);
    }
  }
}

void append(std::vector<ILazy<GImg>> &to, const ILazy<GImg> &from)
{
    if( from ) to.push_back(from);
}

void append(std::vector<ILazy<GImg>> &to, const std::vector<ILazy<GImg>> &from)
{
    for( const ILazy<GImg> &e : from )
    {
        if( e ) to.push_back(e);
    }
}

bool PathEngine::prepareP2GOut(const IO &io, P2HW &p2hw, PMDP &pmdp)
{
  bool wdmaUsed = false, wrotUsed = false, needFull = false;
  ILazy<GImg> full;

  std::vector<ILazy<GImg>> extra;
  append(extra, io.mOut.mNext);
  append(extra, io.mOut.mDisplay);
  append(extra, io.mOut.mRecord);
  append(extra, io.mOut.mPhy);
  append(extra, io.mOut.mExtra);
  append(extra, io.mOut.mLarge);

  bool allowExtra = allowAppOut(io);
  needFull = (io.mFeature.has(Feature::NR3D)) ||
             (extra.size() > 2) ||
             (extra.size() == 2 &&
              GImg::isRotate(extra[0]) && GImg::isRotate(extra[1])) ||
             (!allowExtra && !extra.empty());

  if( io.mOut.mFull )
  {
    full = io.mOut.mFull;
  }
  else if( needFull )
  {
    // always acquire full by p2a thread
    full = GImg::make(mPool.mFullImg, io.mIn.mIMGISize, io.mIn.mSrcCrop);
    full->acquire();
  }

  fillFull(p2hw, wdmaUsed, wrotUsed, full);
  allowExtra = allowExtra && (mUseIMG3O || !full);
  //fill(p2hw, wdmaUsed, wrotUsed, io.mOut.mNext);
  if( !allowExtra )
  {
    wdmaUsed = wrotUsed = true;
  }
  bool rotFilled = tryFillRotate(p2hw, wrotUsed, extra);
  fillAll(p2hw, pmdp, wdmaUsed, wrotUsed, extra, rotFilled, mSupMultiRotate);

  if( pmdp.mOut.mExtra.size() )
  {
    pmdp.mIn.mFull = full;
    pmdp.mIn.mTuning = p2hw.mIn.mTuning;
    pmdp.mIn.mPQCtrl = io.mIn.mPQCtrl;
  }

  return true;
}

void PathEngine::prepareBasicLoopOut(const IO &io)
{
  TRACE_FUNC_ENTER();
  if( io.mLoopOut )
  {
    io.mLoopOut->mBasic             = io.mIn.mBasic;
    if( io.mLoopIn )
    {
      io.mLoopOut->mNR3DStat        = io.mLoopIn->mNR3DStat;
      io.mLoopOut->mSub             = io.mLoopIn->mSub;
      if( io.mFeature.hasDCE() )
      {
        io.mLoopOut->mDCE_n_2_magic = io.mLoopIn->mDCE_n_1_magic;
        io.mLoopOut->mDCE_n_2       = io.mLoopIn->mDCE_n_1;
        io.mLoopOut->mDCE_n_1_magic = io.mIn.mDCE_n_magic;
      }
    }
  }
  TRACE_FUNC_EXIT();
}

void PathEngine::updateLoopOut(const IO &io, unsigned dsdnIndex, const P2SW &p2sw, const P2HW &p2hw)
{
  (void)p2sw;
  TRACE_FUNC_ENTER();
  unsigned subIndex = dsdnIndex ? dsdnIndex - 1: 0;
  if( io.mLoopOut )
  {
    if( dsdnIndex == 0 )
    {
      io.mLoopOut->mIMG3O         = p2hw.mOut.mIMG3O;
      if( io.mFeature.hasDCE() )
      {
        io.mLoopOut->mDCE_n_1     = p2hw.mOut.mDCESO;
      }
    }
    else if( subIndex < io.mLoopOut->mSub.size() )
    {
      io.mLoopOut->mSub[subIndex].mIMG3O = p2hw.mOut.mIMG3O;
    }
  }
  TRACE_FUNC_EXIT();
}

void PathEngine::updateBatchOut(const IO &io, unsigned dsdnIndex, const P2SW &p2sw)
{
  TRACE_FUNC_ENTER();
  unsigned subIndex = dsdnIndex ? dsdnIndex - 1: 0;
  if( io.mFeature.has(Feature::SMVR_MAIN) )
  {
    if( dsdnIndex == 0 )
    {
      io.mBatchOut->mIMGI           = io.mIn.mIMGI;
      io.mBatchOut->mTuning         = p2sw.mOut.mTuning;
      io.mBatchOut->mTuningData     = p2sw.mOut.mTuningData;
    }
    else if( io.mBatchOut->mSub.size() > subIndex )
    {
      io.mBatchOut->mSub[subIndex].mTuning = p2sw.mOut.mTuning;
      io.mBatchOut->mSub[subIndex].mTuningData = p2sw.mOut.mTuningData;
    }
  }
  else if( io.mFeature.has(Feature::SMVR_SUB) )
  {
    if( io.mBatchIn && io.mBatchOut && io.mBatchIn != io.mBatchOut )
    {
      *io.mBatchOut = *io.mBatchIn;
    }
  }
  TRACE_FUNC_EXIT();
}

bool PathEngine::allowAppOut(const IO &io) const
{
  bool ret = true;
  if( mIs4K && mIs60FPS
      && (io.mFeature.hasDSDN25() || io.mFeature.hasDSDN30())
      && !io.mFeature.has(Feature::EARLY_DISP) )
  {
    ret = false;
  }
  return ret;
}

bool PathEngine::needLoop() const
{
  return mFeatureSet.has(Feature::NR3D | Feature::DCE);
}

bool PathEngine::needBatchIn(const IO &io) const
{
  return io.mFeature.has(Feature::SMVR_SUB);
}

bool PathEngine::needBatchOut(const IO &io) const
{
  return io.mFeature.has(Feature::SMVR_MAIN);
}

bool PathEngine::validate(const IO &io) const
{
  bool ret = true;
  ret = validate(io.mLoopIn || !needLoop(), "missing loop in obj") &&
        validate(io.mLoopOut || !needLoop(), "missing loop out obj") &&
        validate(io.mBatchIn || !needBatchIn(io), "missing batch in obj") &&
        validate(io.mBatchOut || !needBatchOut(io), "missing batch out obj");
  return ret;
}

bool PathEngine::validate(bool cond, const char *msg) const
{
  if( !cond )
  {
    MY_LOGE("%s", msg);
  }
  return cond;
}

void PathEngine::estimatePathCount(const std::vector<IO> &ioList, size_t &cPMss, size_t &cMss, size_t &cP2SW, size_t &cP2HW)
{
  cMss = cPMss = 0;
  cP2SW = cP2HW = ioList.size();
  for( const IO &io : ioList )
  {
    size_t dsdn = io.mIn.mDSDNSizes.size();
    if( dsdn )
    {
      ++cMss;
      cP2SW += dsdn - 1;
      cP2HW += dsdn - 1;
      if( io.mFeature.hasDSDN30() )
      {
        ++cPMss;
        cP2SW += 1; // motion calculation
      }
    }
  }
}

Path PathEngine::run(const std::vector<IO> &ioList)
{
  Path path;
  bool valid = true;

  for( const IO &io : ioList )
  {
    valid = valid && validate(io);
  }

  if( valid )
  {
    size_t cPMss, cMss, cP2SW, cP2HW;
    estimatePathCount(ioList, cPMss, cMss, cP2SW, cP2HW);
    path.mPMSS.reserve(cPMss);
    path.mMSS.reserve(cMss);
    path.mP2SW.reserve(cP2SW);
    path.mP2HW.reserve(cP2HW);
    for( const IO &io : ioList )
    {
      generatePath(io, path);
    }
  }

  return path;
}

void PathEngine::generatePath(const IO &io, Path &path)
{
  TRACE_FUNC_ENTER();
  SubIO lastSub;

  prepareBasicLoopOut(io);

  generatePreSubRun(io, path, lastSub);
  generateMainRun(io, path, lastSub);
  generatePostSubRun(io, path);

  TRACE_FUNC_EXIT();
}

static void resizeSubOut(const IO &io, unsigned subRun)
{
  TRACE_FUNC_ENTER();
  if( io.mLoopOut && io.mLoopOut->mSub.size() < subRun )
  {
    MY_LOGW("invalid sub size=%zu < %d", io.mLoopOut->mSub.size(), subRun);
    io.mLoopOut->mSub.resize(subRun);
  }
  if( io.mBatchOut && io.has(Feature::SMVR_MAIN) )
  {
    io.mBatchOut->mSub.resize(subRun);
  }
  TRACE_FUNC_EXIT();
}

static ILazy<GImg> getLastMSSO(const MSS &mss)
{
  TRACE_FUNC_ENTER();
  ILazy<GImg> lastMsso;
  size_t n = mss.mOut.mMSSOs.size();
  if( n > 0 )
  {
    lastMsso = mss.mOut.mMSSOs[n-1];
  }
  TRACE_FUNC_EXIT();
  return lastMsso;
}

void PathEngine::generatePreSubRun(const IO &io, Path &path, SubIO &lastSub)
{
  TRACE_FUNC_ENTER();
  unsigned dsdnCount = io.mIn.mDSDNSizes.size();
  bool dsdn30 = io.has(Feature::DSDN30) || io.has(Feature::DSDN30_INIT);
  bool needPreSub = io.has(Feature::DSDN25) || dsdn30;
  if( needPreSub && dsdnCount )
  {
    resizeSubOut(io, dsdnCount-1);
    MSS mss = prepareMSS(io);
    path.mMSS.push_back(mss);
    MSS pmss = preparePMSS(io, mss);
    if( pmss.mValid ) path.mPMSS.push_back(pmss);

    if( dsdn30 )
    {
      P2SW sw;
      sw.mIn.mBasic = io.mIn.mBasic;
      sw.mIn.mTuningType = P2SW::OMC;
      sw.mIn.mHalInMeta = io.mIn.mHalInMeta;
      sw.mIn.mAppInMeta = io.mIn.mAppInMeta;
      sw.mIn.mDSDNInfo = io.mIn.mDSDNInfo;
      sw.mOut.mOMCTuning = pmss.mIn.mOMCTuning;
      path.mP2SW.push_back(sw);

      lastSub.mCONFI = GImg::make(mPool.mConf, io.mIn.mConfSize);
      lastSub.mIDI = GImg::make(mPool.mIdi, io.mIn.mIdiSize);
    }

    lastSub.mDNO = getLastMSSO(mss);
    for( unsigned dsdnIndex = dsdnCount - 1; dsdnIndex > 0; --dsdnIndex )
    {
      bool needSubSW = false, needSubHW = false;
      SubIO subIO;
      P2SW subSW;
      P2HW subHW;
      PMDP subPMDP;

      unsigned dsIndex = dsdnIndex - 1;
      subIO.mIMGISize = io.mIn.mDSDNSizes[dsIndex];
      subIO.mIMGI = mss.mOut.mMSSOs[dsIndex];
      subIO.mDSI = lastSub.mDNO;
      subIO.mDSWI = lastSub.mDSWO;
      subIO.m_n_1_DSWI = lastSub.mDSWI;
      subIO.mDNO = GImg::make(mPool.mDnImgs[dsIndex], subIO.mIMGISize);
      if( dsdn30 )
      {
        subIO.mWEIGHTI = GImg::make(mPool.mMsfWeightImgs[dsIndex], subIO.mIMGISize);
        subIO.mREFI = pmss.mOut.mMSSOs[dsIndex];
        subIO.mDSWO = (dsIndex > 0) ? GImg::make(mPool.mMsfDsWeightImgs[dsIndex], subIO.mIMGISize) : NULL;
        subIO.mCONFI = lastSub.mCONFI;
        subIO.mIDI = lastSub.mIDI;
      }
      lastSub = subIO;

      Feature f = io.mFeature;
      if( f.hasDSDN30() ) f -= Feature::NR3D;
      needSubSW = prepareP2SW(f, io, subIO, dsdnIndex, subSW);
      needSubHW = prepareP2HW(f, io, subIO, dsdnIndex, subSW, subHW, subPMDP);

      updateBatchOut(io, dsdnIndex, subSW);
      updateLoopOut(io, dsdnIndex, subSW, subHW);
      if( needSubSW ) path.mP2SW.push_back(subSW);
      if( needSubHW ) path.mP2HW.push_back(subHW);
    }
  }
  TRACE_FUNC_EXIT();
}

void PathEngine::generateMainRun(const IO &io, Path &path, const SubIO &lastSub)
{
  TRACE_FUNC_ENTER();
  bool needP2SW = false, needP2HW = false;
  PMDP pmdp;
  P2SW p2sw;
  P2HW p2hw;
  SubIO subIO;

  subIO.mDSI = lastSub.mDNO;
  subIO.mIMG2O = io.has(Feature::DSDN20) ? io.mOut.mDS1 : io.mOut.mFD;

  Feature f = io.mFeature;
  if ( f.has(Feature::DSDN20) && f.has(Feature::NR3D) )
      f -= Feature::NR3D;
  needP2SW = prepareP2SW(f, io, subIO, 0, p2sw);
  needP2HW = prepareP2HW(f, io, subIO, 0, p2sw, p2hw, pmdp);
  updateBatchOut(io, 0, p2sw);
  updateLoopOut(io, 0, p2sw, p2hw);
  if( needP2SW ) path.mP2SW.push_back(p2sw);
  if( needP2HW ) path.mP2HW.push_back(p2hw);
  if( pmdp.mIn.mFull ) path.mPMDP.push_back(pmdp);
  TRACE_FUNC_EXIT();
}

void PathEngine::generatePostSubRun(const IO &io, Path &path)
{
  TRACE_FUNC_ENTER();
  if( io.has(Feature::DSDN20) && io.mIn.mDSDNSizes.size() )
  {
    unsigned dsdnIndex = 1;
    SubIO subIO;
    P2SW subSW;
    P2HW subHW;
    PMDP subPMDP;

    subIO.mIMGISize = io.mIn.mDSDNSizes[dsdnIndex-1];
    subIO.mIMGI = io.mOut.mDS1;
    subIO.mDNO = io.mOut.mDN1;
    subIO.mIMG2O = io.mOut.mFD;

    resizeSubOut(io, 1);
    bool needSubSW = prepareP2SW(io.mFeature, io, subIO, dsdnIndex, subSW);
    bool needSubHW = prepareP2HW(io.mFeature, io, subIO, dsdnIndex, subSW, subHW, subPMDP);

    updateLoopOut(io, dsdnIndex, subSW, subHW);
    if( needSubSW ) path.mP2SW.push_back(subSW);
    if( needSubHW ) path.mP2HW.push_back(subHW);
  }
  TRACE_FUNC_EXIT();
}

bool PathEngine::initCheck(const DataPool &pool, const Feature &f) const
{
  bool ret = true;
  ret = validate(pool.mTuning, "missing tuning pool") &&
        validate(!f.hasDUAL() || pool.mSyncTuning, "missing sync tuning") &&
        validate(!f.hasNR3D() || pool.mFullImg, "missing full pool") &&
        validate(!f.hasDCE() || pool.mDceso, "missing dceso pool") &&
        validate(!f.hasTIMGO() || pool.mTimgo, "missing timgo pool") &&
        validate(pool.mTuningData, "missing tuning data pool") &&
        validate(pool.mP2Obj, "missing p2obj pool");
  return ret;
}

void PathEngine::fill(P2HW &p2hw, bool &wdmaUsed, bool &wrotUsed, const ILazy<GImg> &io)
{
  if( io )
  {
    if( !wdmaUsed )
    {
      p2hw.mOut.mWDMAO = io;
      wdmaUsed = true;
    }
    else if ( !wrotUsed )
    {
      p2hw.mOut.mWROTO = io;
      wrotUsed = true;
    }
  }
}

void PathEngine::fill(P2HW &p2hw, PMDP &pmdp, bool &wdmaUsed, bool &wrotUsed, const ILazy<GImg> &io, bool supMultiRotate)
{
  if( io )
  {
    bool allowWDMAO = supMultiRotate || !GImg::isRotate(io);
    if( !wdmaUsed && allowWDMAO )
    {
      p2hw.mOut.mWDMAO = io;
      wdmaUsed = true;
    }
    else if( !wrotUsed )
    {
      p2hw.mOut.mWROTO = io;
      wrotUsed = true;
    }
    else
    {
      pmdp.mOut.mExtra.push_back(io);
    }
  }
}

bool PathEngine::tryFillRotate(P2HW &p2hw, bool &wrotUsed, const std::vector<ILazy<GImg>> &list)
{
  bool filled = false;
  if( !wrotUsed )
  {
    for( const ILazy<GImg> &io : list )
    {
      if( GImg::isRotate(io) )
      {
        p2hw.mOut.mWROTO = io;
        wrotUsed = true;
        filled = true;
        break;
      }
    }
  }
  return filled;
}

void PathEngine::fillAll(P2HW &p2hw, PMDP &pmdp, bool &wdmaUsed, bool &wrotUsed, const std::vector<ILazy<GImg>> &list, bool skipFirstRotate, bool supMultiRotate)
{
  for( const ILazy<GImg> &io : list )
  {
    if( skipFirstRotate && GImg::isRotate(io) )
    {
      skipFirstRotate = false;
    }
    else
    {
      fill(p2hw, pmdp, wdmaUsed, wrotUsed, io, supMultiRotate);
    }
  }
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
