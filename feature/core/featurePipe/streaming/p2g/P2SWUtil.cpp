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

#include "P2SWUtil.h"

#include "hal/inc/camera_custom_3dnr.h"
#include <mtkcam/custom/ExifFactory.h>
#include <mtkcam/utils/exif/DebugExifUtils.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

#include "../DsdnHal.h"

#include "../DebugControl.h"
#define PIPE_CLASS_TAG "P2G_P2SWUTIL"
#define PIPE_TRACE TRACE_P2G_P2SW
#include <featurePipe/core/include/PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

using NS3Av3::IHalISP;
using NS3Av3::MetaSet_T;
using NSCam::Feature::P2Util::trySet;
using NSCam::Feature::P2Util::tryGet;
using NSCam::NSIoPipe::NSPostProc::INormalStream;
using namespace NSCam::NR3D;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

static auto getDebugExif()
{
  static auto const sInst = MAKE_DebugExif();
  return sInst;
}

static TunBuffer acquire(const LazyTunBufferPool &pool)
{
  TRACE_FUNC_ENTER();
  TunBuffer tun = pool.requestBuffer(LazyTunBufferPool::MEMCLR);
  TRACE_FUNC_EXIT();
  return tun;
}

static void acquire(const ILazy<TunBuffer> &buffer, const LazyTunBufferPool &pool)
{
  TRACE_FUNC_ENTER();
  if( buffer )
  {
    *buffer = acquire(pool);
  }
  TRACE_FUNC_EXIT();
}

static ImageInfo getImageInfo(const ILazy<GImg> &img)
{
    return img ? img->getImageInfo() : ImageInfo();
}

P2SWUtil::P2SWUtil()
{
  TRACE_FUNC_ENTER();
  m3dnrDebugEnable = ::property_get_int32(NR3D_PROP_DEBUG_ENABLE, 0);
  mLogLevel = ::property_get_int32(KEY_FORCE_PRINT_P2SWUTIL, 0);
  TRACE_FUNC_EXIT();
}

P2SWUtil::~P2SWUtil()
{
  TRACE_FUNC_ENTER();
  if( mReady )
  {
    uninit();
  }
  TRACE_FUNC_EXIT();
}

bool P2SWUtil::init(const std::shared_ptr<SensorContextMap> &sensorContextMap)
{
  TRACE_FUNC_ENTER();
  if( !mReady )
  {
    mSensorContextMap = sensorContextMap;
    mReady = true;
  }

  // init 3dnr hal instances
  for (int k = 0; k <  HAL_3DNR_INST_MAX_NUM; ++k)
  {
      mp3dnr[k] = nullptr;
  }

  TRACE_FUNC_EXIT();
  return true;
}

bool P2SWUtil::uninit()
{
  TRACE_FUNC_ENTER();
  if( mReady )
  {
    mReady = false;
    mSensorContextMap = NULL;
  }

  // releaes 3dnr hal instances
  for (int k = 0; k < HAL_3DNR_INST_MAX_NUM; ++k)
  {
      if (mp3dnr[k] != nullptr)
      {
         mp3dnr[k]->uninit(PIPE_CLASS_TAG);
         mp3dnr[k]->destroyInstance(PIPE_CLASS_TAG, k);
         mp3dnr[k] = nullptr;
      }
  }

  TRACE_FUNC_EXIT();
  return true;
}

void P2SWUtil::run(const DataPool &pool, const std::vector<P2SW> &p2sw, P2SW::TuningType type)
{
  TRACE_FUNC_ENTER();
  for( const P2SW &cfg : p2sw )
  {
    if( cfg.mIn.mTuningType == type )
    {
        if( type == P2SW::P2 )          runP2(pool, cfg);
        else if( type == P2SW::OMC )    runOMC(pool, cfg);
    }
  }
  TRACE_FUNC_EXIT();
}

static MBOOL isYuv(EImageFormat fmt)
{
    return !NSCam::isHalRawFormat(fmt);
}

void copyInMetaAndUpdate(const P2SW &cfg, MetaSet_T &inSet)
{
  TRACE_FUNC_ENTER();
  P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TH(p2a.r2y)::copyInMeta");
  inSet.appMeta = *cfg.mIn.mAppInMeta;
  inSet.halMeta = *cfg.mIn.mHalInMeta;

  trySet<MUINT8>(inSet.halMeta, MTK_3A_PGN_ENABLE,
                 cfg.mIn.mBasic.mResized ? 0 : 1);
  if( isYuv(cfg.mIn.mIMGI->getImgFormat()) )
  {
    trySet<MINT32>(inSet.halMeta, MTK_ISP_P2_IN_IMG_FMT, 1);
    MSize imgiSize = cfg.mIn.mIMGI->getImgSize();
    MINT32 resolution = (imgiSize.h << 16) | imgiSize.w;
    trySet<MINT32>(inSet.halMeta, MTK_ISP_P2_IN_IMG_RES_REVISED, resolution);
  }
  P2_CAM_TRACE_END(TRACE_ADVANCED);
  TRACE_FUNC_EXIT();
}

void updateOutMeta(const P2SW &cfg, IMetadata *halOut)
{
  TRACE_FUNC_ENTER();
  if( halOut )
  {
    trySet<MINT32>(halOut, MTK_PIPELINE_FRAME_NUMBER,
                   cfg.mIn.mBasic.mP2Pack.getFrameData().mMWFrameNo);
    trySet<MINT32>(halOut, MTK_PIPELINE_REQUEST_NUMBER,
                  cfg.mIn.mBasic.mP2Pack.getFrameData().mMWFrameRequestNo);
    if( cfg.mIn.mBasic.mFDCrop )
    {
      trySet<MRect>(halOut, MTK_P2NODE_FD_CROP_REGION, cfg.mIn.mBasic.mFDCrop.mRect);
    }
    if( cfg.mIn.mBasic.mFDSensorID != INVALID_SENSOR_ID )
    {
      trySet<MINT32>(halOut, MTK_DUALZOOM_FD_REAL_MASTER, cfg.mIn.mBasic.mFDSensorID);
    }
  }
  TRACE_FUNC_EXIT();
}

void updateDebugExif(const IMetadata *halIn, IMetadata *halOut)
{
  TRACE_FUNC_ENTER();
  MUINT8 needExif = 0;

  if( halIn && halOut &&
      tryGet<MUINT8>(halIn, MTK_HAL_REQUEST_REQUIRE_EXIF, needExif) &&
      needExif )
  {
    MINT32 vhdrMode = SENSOR_VHDR_MODE_NONE;
    if( tryGet<MINT32>(halIn, MTK_P1NODE_SENSOR_VHDR_MODE, vhdrMode) &&
        vhdrMode != SENSOR_VHDR_MODE_NONE )
    {
      std::map<MUINT32, MUINT32> debugInfoList;
      debugInfoList[getDebugExif()->getTagId_MF_TAG_IMAGE_HDR()] = 1;

      IMetadata exifMeta;
      tryGet<IMetadata>(halOut, MTK_3A_EXIF_METADATA, exifMeta);
      if( DebugExifUtils::setDebugExif(
            DebugExifUtils::DebugExifType::DEBUG_EXIF_MF,
            static_cast<MUINT32>(MTK_MF_EXIF_DBGINFO_MF_KEY),
            static_cast<MUINT32>(MTK_MF_EXIF_DBGINFO_MF_DATA),
            debugInfoList, &exifMeta) != NULL )
      {
        trySet<IMetadata>(halOut, MTK_3A_EXIF_METADATA, exifMeta);
      }
    }
  }
  TRACE_FUNC_EXIT();
}

void copyOutMetaAndUpdate(const P2SW &cfg, const MetaSet_T &inMetaSet, const MetaSet_T &outMetaSet)
{
  TRACE_FUNC_ENTER();
  P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TH(p2a.r2y)::copyOutMeta");
  if( cfg.mOut.mAppOutMeta != NULL )
  {
    *cfg.mOut.mAppOutMeta = outMetaSet.appMeta;
  }
  if( cfg.mOut.mExAppOutMeta != NULL )
  {
    *cfg.mOut.mExAppOutMeta = outMetaSet.appMeta;
  }
  if( cfg.mOut.mHalOutMeta != NULL )
  {
    *cfg.mOut.mHalOutMeta = inMetaSet.halMeta;
    *cfg.mOut.mHalOutMeta += outMetaSet.halMeta;
  }
  updateOutMeta(cfg, cfg.mOut.mHalOutMeta);
  updateDebugExif(cfg.mIn.mHalInMeta, cfg.mOut.mHalOutMeta);
  P2_CAM_TRACE_END(TRACE_ADVANCED);
  TRACE_FUNC_EXIT();
}

void* peak(const TunBuffer &buffer)
{
  return (buffer != NULL) ? buffer->mpVA : NULL;
}

void callSetISP(int32_t loglevel, const P2SW &cfg, IHalISP *halISP, MetaSet_T &inMetaSet, MetaSet_T *pOutMetaSet, TuningParam &tuningParam)
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TH(p2a.r2y)::makeTuningParam");

    tuningParam.pRegBuf = (*cfg.mOut.mTuning)->mpVA;
    tuningParam.pMfbBuf = peak(cfg.mOut.mMSFTuning);
    tuningParam.pMsfTbl = peak(cfg.mOut.mMSFSram);
    tuningParam.pLcsBuf = peak(cfg.mIn.mLCSO);
    void *lcs = tuningParam.pLcsBuf;
    tuningParam.pLceshoBuf = peak(cfg.mIn.mLCSHO);
    tuningParam.i4DcsMagicNo = -1;
    tuningParam.bSlave = !cfg.mIn.mBasic.mIsMaster;
    tuningParam.pDualSynInfo = peak(cfg.mIn.mSyncTun);

    IImageBuffer *dceso = NULL;
    if( cfg.mIn.mDCE_n_2 )
    {
      dceso = cfg.mIn.mDCE_n_2->getImageBufferPtr();
      if( dceso )
      {
        dceso->syncCache(eCACHECTRL_INVALID);
      }
    }
    tuningParam.pDcsBuf = dceso;
    tuningParam.i4DcsMagicNo = dceso ? cfg.mIn.mDCE_n_2_magic : -1;

    tuningParam.pLce4CALTM = cfg.mIn.mPQCtrl ? cfg.mIn.mPQCtrl->getDreTuning() : NULL;

    if( loglevel )
    {
        MY_LOGD("before setIsp sensor(%d) dsdnInd(%d)", cfg.mIn.mBasic.mSensorIndex, cfg.mIn.mDSDNIndex);
        NSCam::Feature::P2Util::printTuningParam(tuningParam);
    }
    P2_CAM_TRACE_BEGIN(TRACE_DEFAULT, "P2Util:Tuning");
    if( halISP && tuningParam.pRegBuf )
    {
      MINT32 ret3A = halISP->setP2Isp(0, inMetaSet, &tuningParam, pOutMetaSet);
      if( ret3A < 0 )
      {
        MY_LOGW("halISP->setIsp failed(ret=%d)", ret3A);
      }
    }
    else
    {
      MY_LOGW("skip setIsp: halISP=%p tuningParam=%p", halISP, tuningParam.pRegBuf);
    }
    if( lcs != tuningParam.pLcsBuf )
    {
      MY_LOGW("setIsp lcso not match !!! before setIsp=%p, after setIsp=%p", lcs, tuningParam.pLcsBuf);
    }

    TRACE_FUNC("in.dce=#%d/%p set.dce=#%d/%p out.enable=%d",
                cfg.mIn.mDCE_n_2_magic, dceso,
                tuningParam.i4DcsMagicNo, tuningParam.pDcsBuf,
                tuningParam.bDCES_Enalbe);
    P2_CAM_TRACE_END(TRACE_DEFAULT);
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    TRACE_FUNC_EXIT();
}

void P2SWUtil::runP2(const DataPool &pool, const P2SW &cfg)
{
  TRACE_FUNC_ENTER();
  if( cfg.mOut.mTuningData )
  {
    if( cfg.mFeature.has(Feature::SMVR_SUB) )
    {
      processSub(cfg);
    }
    else
    {
      acquireTuning(pool, cfg);
      processTuning(cfg);
    }
  }
  TRACE_FUNC_EXIT();
}

void P2SWUtil::runOMC(const DataPool &pool, const P2SW &cfg)
{
  TRACE_FUNC_ENTER();
  acquire(cfg.mOut.mOMCTuning, pool.mOmcTuning);
  if( cfg.mOut.mOMCTuning )
  {
    MetaSet_T inMetaSet;
    if( cfg.mIn.mHalInMeta == NULL || cfg.mIn.mAppInMeta == NULL )
    {
      MY_LOGE("Invalid tuning: appI/halI(%p/%p", cfg.mIn.mAppInMeta, cfg.mIn.mHalInMeta);
    }
    else
    {
      inMetaSet.appMeta = *cfg.mIn.mAppInMeta;
      inMetaSet.halMeta = *cfg.mIn.mHalInMeta;
    }
    SensorContextPtr ctx = mSensorContextMap->find(cfg.mIn.mBasic.mSensorIndex);
    MINT32 profile = (cfg.mIn.mDSDNInfo && cfg.mIn.mDSDNInfo->mProfiles.size())
                    ? cfg.mIn.mDSDNInfo->mProfiles[0] : -1;
    if( ctx && ctx->mHalISP && profile >= 0)
    {
      trySet<MUINT8>(inMetaSet.halMeta, MTK_3A_ISP_PROFILE, (MUINT8)profile);
      P2_CAM_TRACE_BEGIN(TRACE_DEFAULT, "IspCtrl::OMCTun");
      ctx->mHalISP->sendIspCtrl(NS3Av3::EISPCtrl_GetMssTuning, (MINTPTR)&inMetaSet, (MINTPTR)(*cfg.mOut.mOMCTuning)->mpVA);
      P2_CAM_TRACE_END(TRACE_DEFAULT);
    }
    else
    {
      MY_LOGE("Invalid omc tuning: halI(%p) sensorContext(%p), halIsp(%p), prof(%d)",
              cfg.mIn.mHalInMeta, ctx.get() , ctx ? ctx->mHalISP : NULL, profile);

    }
  }
  TRACE_FUNC_EXIT();
}

void P2SWUtil::acquireTuning(const DataPool &pool, const P2SW &cfg)
{
  TRACE_FUNC_ENTER();

  TunBuffer dreTuning;

  acquire(cfg.mOut.mTuning, pool.mTuning);
  if( cfg.mFeature.hasDSDN30() ) // TODO check only dsdnIndex > 0 need msf tuning
  {
    acquire(cfg.mOut.mMSFTuning, pool.mMsfTuning);
    acquire(cfg.mOut.mMSFSram, pool.mMsfSram);
  }
  if( cfg.mFeature.hasDRE_TUN() )
  {
    dreTuning = acquire(pool.mDreTuning);
  }

  if( cfg.mIn.mPQCtrl )
  {
    const IAdvPQCtrl &ctrl = cfg.mIn.mPQCtrl;
    ctrl->setIspTuning(cfg.mOut.mTuning ? *cfg.mOut.mTuning : NULL);
    ctrl->setDreTuning(dreTuning);
  }
  if( cfg.mOut.mTuningData )
  {
    cfg.mOut.mTuningData->mPQCtrl = cfg.mIn.mPQCtrl;
  }

  TRACE_FUNC_EXIT();
}

void P2SWUtil::processTuning(const P2SW &cfg)
{
  TRACE_FUNC_ENTER();
  TuningData tuning;
  SensorContextPtr ctx = mSensorContextMap->find(cfg.mIn.mBasic.mSensorIndex);
  if( !cfg.mIn.mIMGI || !cfg.mOut.mTuning )
  {
    MY_LOGE("Invalid input: imgi(%p) tun(%p)",
            cfg.mIn.mIMGI.get(), cfg.mOut.mTuning.get());
  }
  else if( cfg.mIn.mHalInMeta == NULL || cfg.mIn.mAppInMeta == NULL )
  {
    MY_LOGE("Invalid tuning: appI/O/O2(%p/%p/%p), halI/O(%p/%p)",
            cfg.mIn.mAppInMeta, cfg.mOut.mAppOutMeta, cfg.mOut.mExAppOutMeta,
            cfg.mIn.mHalInMeta, cfg.mOut.mHalOutMeta);
  }
  else if( !ctx )
  {
    MY_LOGD("Cannot find context for sensor ID=%d", cfg.mIn.mBasic.mSensorIndex);
  }
  else
  {
    {
      MetaSet_T inMetaSet;
      MetaSet_T outMetaSet;

      copyInMetaAndUpdate(cfg, inMetaSet);
      processDSDN(cfg, inMetaSet);
      processNR3D(cfg, ctx, inMetaSet);
      callSetISP(mLogLevel, cfg, ctx->mHalISP, inMetaSet, &outMetaSet, cfg.mOut.mTuningData->mParam);
      postProcessNR3D_legacy(cfg, ctx);
      copyOutMetaAndUpdate(cfg, inMetaSet, outMetaSet);

      P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TH(p2a.r2y)::freeMetaSet");
    }
    P2_CAM_TRACE_END(TRACE_ADVANCED);
  }
  TRACE_FUNC_EXIT();
}

void P2SWUtil::processSub(const P2SW &cfg) const
{
  TRACE_FUNC_ENTER();
  if( !cfg.mOut.mTuningData || !cfg.mBatchIn.mTuningData )
  {
    MY_LOGE("missing tuning data: out(%p) batch_in(%p)",
            cfg.mOut.mTuningData.get(), cfg.mBatchIn.mTuningData.get());
  }
  else if( cfg.mOut.mTuningData != cfg.mBatchIn.mTuningData )
  {
    *cfg.mOut.mTuningData = *cfg.mBatchIn.mTuningData;
    processSubLCEI(cfg);
  }
  TRACE_FUNC_EXIT();
}

void P2SWUtil::processSubLCEI(const P2SW &cfg) const
{
  TRACE_FUNC_ENTER();
  IImageBuffer *baseLcei = (IImageBuffer*)cfg.mBatchIn.mTuningData->mParam.pLcsBuf;
  if( baseLcei )
  {
    MBOOL useLcei = MFALSE;
    MSize baseSize(0,0), lceiSize(0,0);
    IImageBuffer *lcei = peak(cfg.mIn.mLCSO);
    baseSize = baseLcei->getImgSize();
    if( lcei )
    {
        lceiSize = lcei->getImgSize();
        useLcei = (lceiSize == baseSize ) ||
                  lcei->setExtParam(baseSize);
    }
    if( !useLcei )
    {
        MY_LOGE("missing lcei: base(%p)(%dx%d) lcei(%p/%p)(%dx%d) dsdn=%d",
                baseLcei, baseSize.w, baseSize.h, cfg.mIn.mLCSO.get(),
                lcei, lceiSize.w, lceiSize.h, cfg.mIn.mDSDNIndex);
    }
    cfg.mOut.mTuningData->mParam.pLcsBuf = useLcei ? (void*)lcei : NULL;
  }
  TRACE_FUNC_EXIT();
}

void P2SWUtil::processNR3D(const P2SW &cfg, const SensorContextPtr &ctx, MetaSet_T &inMetaSet)
{
  TRACE_FUNC_ENTER();
  if( cfg.mFeature.has(Feature::NR3D) )
  {
    if( !cfg.mIn.mNR3DStat)
    {
      MY_LOGW("3dnr: missing nr3d stat");
    }
    else
    {
      MBOOL dsdn = cfg.mFeature.has(Feature::DSDN25) || cfg.mFeature.hasDSDN30();
      MUINT32 dsdnCount = cfg.mIn.mDSDNCount;
      MUINT32 dsdnIndex = cfg.mIn.mDSDNIndex;

      ImageInfo imgi = getImageInfo(cfg.mIn.mIMGI);
      ImageInfo vipi = getImageInfo(cfg.mIn.mVIPI);

      MUINT32 fakeGMV = 0;
      if( cfg.mIn.mNR3DMotion )
      {
        fakeGMV = cfg.mIn.mNR3DMotion->mGMV;
      }

      TRACE_FUNC("3dnr: imgi(%dx%d) vipi(%dx%d:%zu)"
               "stat=%d gmv=%d dsdn(%d:%d/%d)",
                imgi.mSize.w, imgi.mSize.h,
                vipi.mSize.w, vipi.mSize.h, vipi.mStrideInBytes.size(),
                cfg.mIn.mNR3DStat->mLastStat, fakeGMV,
                dsdn, dsdnIndex, dsdnCount);

      cfg.mIn.mNR3DStat->mLastStat += dsdn ? dsdnIndex+dsdnCount : 2;

      MUINT32 nr3dFeatMask = decideNR3D_featureMask(cfg);
      processNR3D_feature(cfg, ctx, inMetaSet, nr3dFeatMask);

    }
  }
  TRACE_FUNC_EXIT();
}

void P2SWUtil::processDSDN(const P2SW &cfg, MetaSet_T &inSet)
{
  TRACE_FUNC_ENTER();
  MBOOL needProfile = MFALSE;
  MINT32 profile = (cfg.mIn.mDSDNInfo && (cfg.mIn.mDSDNInfo->mProfiles.size() > cfg.mIn.mDSDNIndex))
                    ? cfg.mIn.mDSDNInfo->mProfiles[cfg.mIn.mDSDNIndex] : -1;
  MUINT32 ver = cfg.mIn.mDSDNInfo ? cfg.mIn.mDSDNInfo->mVer : 0;
  trySet<MINT32>(inSet.halMeta, MTK_SW_DSDN_VERSION, (MINT32)ver);

  if( cfg.mFeature.hasDSDN25() || cfg.mFeature.hasDSDN30() )
  {
      trySet<MINT32>(inSet.halMeta, MTK_MSF_SCALE_INDEX, (MINT32)cfg.mIn.mDSDNIndex);
      needProfile = MTRUE;
      if( cfg.mFeature.hasDSDN30() && cfg.mIn.mDSDNIndex > 0 )
      {
          trySet<MINT32>(inSet.halMeta, MTK_MSF_FRAME_NUM, 0);
          trySet<MINT32>(inSet.halMeta, MTK_TOTAL_MULTI_FRAME_NUM, 2);
      }
  }
  else if( cfg.mFeature.hasDSDN20() && cfg.mIn.mDSDNIndex )
  {
      trySet<MUINT8>(inSet.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, 9);
      needProfile = MTRUE;
  }

  if( needProfile )
  {
    if( profile < 0 )
    {
        size_t size = cfg.mIn.mDSDNInfo ? cfg.mIn.mDSDNInfo->mProfiles.size() : 0;
        MY_LOGW("missing dsdn[%d] profile(%d) info(%p) info,profSize(%zu)", cfg.mIn.mDSDNIndex, profile, cfg.mIn.mDSDNInfo.get(), size);
    }
    else
    {
      trySet<MUINT8>(inSet.halMeta, MTK_3A_ISP_PROFILE, (MUINT8)profile);
      cfg.mOut.mTuningData->mProfile = profile;
    }
  }
  TRACE_FUNC_EXIT();
}

MUINT32 P2SWUtil::decideNR3D_featureMask(const P2SW &cfg)
{
    TRACE_FUNC_ENTER();
    bool isDSDN2530 = cfg.mFeature.has(Feature::DSDN25) || cfg.mFeature.has(Feature::DSDN30);
    bool isDSDN20 = cfg.mFeature.has(Feature::DSDN20);
    bool isSMVRB = cfg.mFeature.has(Feature::SMVR_MAIN) &&
        NR3DCustom::is3DNRSmvrBatchSupported();
//  bool isMulticam = cfg.mFeature.has(P2AGroup_Feature::VSDOF)
//      || cfg.mFeature.has(P2AGroup_Feature::W_T)
//      || cfg.mFeature.has(P2AGroup_Feature::TRICAM);
    MUINT32 featMask =  NR3D_FEAT_MASK_BASIC;
    if (isSMVRB)
    {
        if (isDSDN2530)
        {
            ENABLE_3DNR_SMVRB_DSDN25(featMask);
        }
        else if (isDSDN20)
        {
            ENABLE_3DNR_SMVRB_DSDN20(featMask);
        }
        else
        {
            ENABLE_3DNR_SMVRB(featMask);
        }
    }
    else
    {
        if (isDSDN2530)
        {
            ENABLE_3DNR_BASIC_DSDN25(featMask);
        }
        else if (isDSDN20)
        {
            ENABLE_3DNR_BASIC_DSDN20(featMask);
        }
        else
        {
            ENABLE_3DNR_BASIC(featMask);
        }
    }
    MY_LOGD_IF(mLogLevel >= 2,
        "3dnr: DSDN25or30(%d), DSDN20(%d), SMVRB(%d) --> FeatMask(0x%x)",
        isDSDN2530, isDSDN20, isSMVRB, featMask);

    TRACE_FUNC_EXIT();
    return featMask;
}

void P2SWUtil::prepareNR3D_common(
    const P2SW &cfg, const SensorContextPtr &ctx, const MUINT32 featMask, NR3DHALParam &nr3dHalParam)
{
    TRACE_FUNC_ENTER();

    ImageInfo imgi = getImageInfo(cfg.mIn.mIMGI);
    ImageInfo vipi = getImageInfo(cfg.mIn.mVIPI);

    nr3dHalParam.pTuningData = cfg.mOut.mTuning.get(); // mktodo: need to make sure Tuning is ready
    nr3dHalParam.p3A = ctx->mHal3A;

    // featMaks
    nr3dHalParam.featMask = featMask;

    // uniqueKey, requestNo, frameNo
    const MUINT32 sensorID = cfg.mIn.mBasic.mSensorIndex;
    TuningUtils::FILE_DUMP_NAMING_HINT hint = cfg.mIn.mBasic.mP2Pack.getSensorData(sensorID).mNDDHint;
    nr3dHalParam.uniqueKey = hint.UniqueKey;
    nr3dHalParam.requestNo = hint.RequestNo;
    nr3dHalParam.frameNo = hint.FrameNo;

    nr3dHalParam.needChkIso = MTRUE;
    nr3dHalParam.iso = cfg.mIn.mBasic.mP2Pack.getSensorData(sensorID).mISO;
    nr3dHalParam.isoThreshold = NR3DCustom::get_3dnr_off_iso_threshold(-1, m3dnrDebugEnable);

    nr3dHalParam.isCRZUsed = MFALSE; // fix: because no CRZ used is used
    nr3dHalParam.isIMGO = !cfg.mIn.mBasic.mResized;
    nr3dHalParam.isBinning = MFALSE; // fix value for now

    // prepare io size
    if (vipi.mSize.w != 0 &&
        vipi.mSize.h != 0)
    {
        MY_LOGD_IF(mLogLevel >= 2,
            "3dnr: vipiInfo: fmt(0x%x), stride_vec.size(%zu), size(%dx%d)",
            vipi.mFormat,
            vipi.mStrideInBytes.size(),
            vipi.mSize.w,
            vipi.mSize.h);

        MUINT32 vipiStride = 0;
        if (vipi.mStrideInBytes.size() > 0)
        {
            vipiStride = vipi.mStrideInBytes[0];
            MY_LOGD_IF(mLogLevel >= 2,
                "3dnr: cfg.mIn.mVIPI->mStrideInBytes[0]: %d",
                vipi.mStrideInBytes[0]);
        }

        nr3dHalParam.vipiInfo = NR3DHALParam::VipiInfo(
            vipi.mSize.w != 0,
            vipi.mFormat,
            vipiStride, // mkdbg: use 3600 as replacement
            vipi.mSize);
    }
    else
    {
        MY_LOGW("!!warn: mkdbg: VIPIInfo invalid");
    }

    // nr3dHalParam.p2aSrcCrop: no need
    nr3dHalParam.srcCrop = nr3dHalParam.dstRect =
        MRect(imgi.mSize.w, imgi.mSize.h);

    TRACE_FUNC_EXIT();
    return;
}

void P2SWUtil::processNR3D_feature(const P2SW &cfg, const SensorContextPtr &ctx, MetaSet_T &inMetaSet, const MUINT32 featMask)
{
    TRACE_FUNC_ENTER();

    NR3DHALParam nr3dHalParam;
    prepareNR3D_common(cfg, ctx, featMask, nr3dHalParam);

    const MUINT32 sensorID = cfg.mIn.mBasic.mSensorIndex;
    if (sensorID >= 0 && sensorID < HAL_3DNR_INST_MAX_NUM)
    {
        if (mp3dnr[sensorID] == nullptr)
        {
            mp3dnr[sensorID] = Hal3dnrBase::createInstance(PIPE_CLASS_TAG, sensorID);
            if (mp3dnr[sensorID] == nullptr)
            {
                MY_LOGE("!!err: 3dnr_hal instance fails to be created");
                return;
            }
            mp3dnr[sensorID]->init(PIPE_CLASS_TAG);
        }
    }
    else
    {
        MY_LOGE("!!err: sensorID(%d) is wrong", sensorID);
        return;
    }
    Hal3dnrBase* p3dnr = mp3dnr[sensorID];

    NR3DHALResult &rNr3dHalResult = cfg.mIn.mNR3DStat->mNr3dHalResult;
    if (HAS_3DNR_BASIC_DSDN25(featMask) || HAS_3DNR_BASIC_DSDN20(featMask) )
    {
        rNr3dHalResult.gmvInfo = cfg.mIn.mNR3DMotion->gmvInfoResults[cfg.mIn.mDSDNIndex];
        rNr3dHalResult.isGMVInfoUpdated = cfg.mIn.mNR3DMotion->isGmvInfoValid;

        nr3dHalParam.needChkIso = MFALSE;
    }
    else if (HAS_3DNR_SMVRB_DSDN25(featMask) || HAS_3DNR_SMVRB_DSDN20(featMask) ||
             HAS_3DNR_SMVRB(featMask) )
    {
        // just gmvInfoUpdate=true even though update3DNRMVInfo is not called
        rNr3dHalResult.isGMVInfoUpdated = MTRUE;
    }
    else // default basic
    {
        rNr3dHalResult.gmvInfo = cfg.mIn.mNR3DMotion->gmvInfoResults[0];
        rNr3dHalResult.isGMVInfoUpdated = cfg.mIn.mNR3DMotion->isGmvInfoValid;
    }

    // do3dnr
    if (p3dnr->do3dnrFlow(nr3dHalParam, rNr3dHalResult) == MTRUE)
    {
        // p3dnr->updateISPMetadata
        NR3D::NR3DTuningInfo Nr3dTuning;

        NR3DHwParam &nr3dHwParam = rNr3dHalResult.nr3dHwParam;
        Nr3dTuning.canEnable3dnrOnFrame = nr3dHwParam.ctrl_onEn;

        Nr3dTuning.isoThreshold = NR3DCustom::get_3dnr_off_iso_threshold(-1, m3dnrDebugEnable);
        Nr3dTuning.mvInfo = rNr3dHalResult.gmvInfo;
        Nr3dTuning.inputSize = nr3dHalParam.srcCrop.s;
        // If CRZ is used, we must correct following fields and review ISP code
        Nr3dTuning.inputCrop = nr3dHalParam.srcCrop;
        Nr3dTuning.gyroData = nr3dHalParam.gyroData;  // !!NOTES: gyro useless for now
        //prepare 3DNR35 before setp2isp
        Nr3dTuning.nr3dHwParam = nr3dHwParam;

        if (p3dnr->updateISPMetadata(&inMetaSet.halMeta, Nr3dTuning) != MTRUE)
        {
            MY_LOGW("!!warn: p3dnr->updateISPMetadata failed ");
        }
    }
    else
    {
        // MY_LOGW("!!warn: do3dnrFlow failed ");
    }

    TRACE_FUNC_EXIT();
}

void P2SWUtil::postProcessNR3D_legacy(const P2SW &cfg, const SensorContextPtr &ctx)
{
  TRACE_FUNC_ENTER();

  const NSCam::Feature::P2Util::P2SensorInfo &sensorInfo = cfg.mIn.mBasic.mP2Pack.getSensorInfo();

  if( cfg.mFeature.has(Feature::NR3D) )
  {
    if ( (!cfg.mIn.mNR3DStat))
    {
        MY_LOGW("!!warn: 3dnr: missing nr3d stat");
    }
    else if (sensorInfo.mSensorID >= 0)
    {
        ImageInfo imgi = getImageInfo(cfg.mIn.mIMGI);
        if (mp3dnr[sensorInfo.mSensorID] != nullptr)
        {
            NR3DHwParam &rNr3dHwParam = cfg.mIn.mNR3DStat->mNr3dHalResult.nr3dHwParam;
            mp3dnr[sensorInfo.mSensorID]->configNR3D_legacy(
                (*cfg.mOut.mTuning)->mpVA, ctx->mHal3A,
                MRect(imgi.mSize.w, imgi.mSize.h),
                rNr3dHwParam);
        }
        else
        {
            MY_LOGW("!!warn: 3dnr<%d> instance should be NOT null", sensorInfo.mSensorID);
        }
    }
  }
  TRACE_FUNC_EXIT();
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
