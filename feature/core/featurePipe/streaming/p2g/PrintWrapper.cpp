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

#include "PrintWrapper.h"

#include "../DebugControl.h"
#define PIPE_CLASS_TAG "P2G_PathEngine"
#define PIPE_TRACE TRACE_P2G_PATH_ENGINE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

#define MSIZE_S     "(%dx%d)"
#define MSIZE_(s)   s.w, s.h

#define MRECT_S     "(%dx%d)@(%d,%d)"
#define MRECT_(r)   r.s.w, r.s.h, r.p.x, r.p.y

#define IMRECT_S    "(%dx%d)@(%d,%d)"
#define IMRECT_(r)  r.s.w, r.s.h, r.p.x, r.p.y

#define IMRECTF_S   "(%fx%f)@(%f,%f)"
#define IMRECTF_(r) r.s.w, r.s.h, r.p.x, r.p.y

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

static MSize getImgSize(const ILazy<GImg> &img)
{
  return img ? img->getImgSize() : MSize();
}

static MRectF getImgCrop(const ILazy<GImg> &img)
{
  return img ? img->getCrop() : MRectF(0,0);
}

void print(const DataPool &pool)
{
  MY_LOGD("tun=%p",         pool.mTuning.get());
  MY_LOGD("sync_tun=%p",    pool.mSyncTuning.get());
  MY_LOGD("dre_tun=%p",     pool.mDreTuning.get());
  MY_LOGD("msftun=%p",      pool.mMsfTuning.get());
  MY_LOGD("msf_sram=%p",    pool.mMsfSram.get());
  MY_LOGD("omc_tun=%p",     pool.mOmcTuning.get());

  MY_LOGD("full=%p",        pool.mFullImg.get());
  MY_LOGD("dceso=%p",       pool.mDceso.get());
  MY_LOGD("timgo=%p",       pool.mTimgo.get());
  MY_LOGD("conf=%p",        pool.mConf.get());
  MY_LOGD("idi=%p",         pool.mIdi.get());
  MY_LOGD("omc=%p",         pool.mOmc.get());

  MY_LOGD("ds=%zu",         pool.mDsImgs.size());
  MY_LOGD("dn=%zu",         pool.mDnImgs.size());
  MY_LOGD("pds=%zu",        pool.mPDsImgs.size());
  MY_LOGD("msfWeight=%zu",  pool.mMsfWeightImgs.size());
  MY_LOGD("msfDsWeight=%zu",  pool.mMsfDsWeightImgs.size());

}

void print(const std::vector<IO> &cfg)
{
  for( const IO &io : cfg )
  {
    Log log;
    print(log, io);
    MY_SAFE_LOGD(log.c_str(), log.length());
  }
}

void print(Log &log, const IO &cfg)
{
  log.append("[scene]%s\n", toName(cfg.mScene));
  print(log, cfg.mFeature);
  print(log, cfg.mIn);
  print(log, cfg.mOut);
  print(log, "IO_LoopIn", cfg.mLoopIn);
  print(log, "IO_LoopOut", cfg.mLoopOut);
  print(log, "IO_BatchIn", cfg.mBatchIn);
  print(log, "IO_BatchOut", cfg.mBatchOut);
}

void print(Log &log, const Feature &feat)
{
  log.append("[feature]");
  for( Feature::FID fid : sAllFeature )
  {
    if( feat.has(fid) )
    {
      log.append("%s,", toName(fid));
    }
  }
  log.append("\n");
}

void print(Log &log, const char *str, const POD &cfg)
{
  log.append("[%s_basic]sensorID=%d\n",           str, cfg.mSensorIndex);
  log.append("[%s_basic]p2pack=%p (valid=%d)\n",  str, &cfg.mP2Pack, cfg.mP2Pack.isValid());
  log.append("[%s_basic]p2tag=%d\n",              str, cfg.mP2Tag);
  log.append("[%s_basic]resized=%d\n",            str, cfg.mResized);
  log.append("[%s_basic]yuvIn=%d\n",              str, cfg.mIsYuvIn);
  log.append("[%s_basic]isMaster=%d\n",           str, cfg.mIsMaster);
  log.append("[%s_basic]reg_dump=%d\n",           str, cfg.mRegDump);
  log.append("[%s_basic]src_crop=" MRECT_S "\n",  str, MRECT_(cfg.mSrcCrop));
  log.append("[%s_basic]fd_crop=" IMRECT_S "\n",   str, IMRECT_(cfg.mFDCrop));
  log.append("[%s_basic]timgo_type=%d\n",         str, cfg.mTimgoType);
}

void print(Log &log, const IO::InData &cfg)
{
  MSize imgi = getImgSize(cfg.mIMGI);
  MSize lcso = getImgSize(cfg.mLCSO);
  MSize lcsho = getImgSize(cfg.mLCSHO);

  print(log, "IO_In", cfg.mBasic);
  log.append("[IO_In]pq_ctrl=%p\n",           cfg.mPQCtrl.get());
  log.append("[IO_In]imgi_size=(%dx%d)\n",    MSIZE_(cfg.mIMGISize));
  log.append("[IO_In]src_crop=" MRECT_S "\n", MRECT_(cfg.mSrcCrop));
  log.append("[IO_In]app_in=%p\n",            cfg.mAppInMeta);
  log.append("[IO_In]hal_in=%p\n",            cfg.mHalInMeta);
  log.append("[IO_In]imgi=%p(%dx%d)\n",       cfg.mIMGI.get(), MSIZE_(imgi));
  log.append("[IO_In]lcso=%p(%dx%d)\n",       cfg.mLCSO.get(), MSIZE_(lcso));
  log.append("[IO_In]lcsho=%p(%dx%d)\n",      cfg.mLCSHO.get(), MSIZE_(lcsho));
  log.append("[IO_In]confi=%dx%d\n",          MSIZE_(cfg.mConfSize));
  log.append("[IO_In]idi=%dx%d\n",            MSIZE_(cfg.mIdiSize));
  log.append("[IO_In]sync_tun=%p\n",          cfg.mSyncTun.get());
  log.append("[IO_In]dce_n_magic=%d\n",       cfg.mDCE_n_magic);
  log.append("[IO_In]3dnr_motion=%p\n",       cfg.mNR3DMotion.get());
  log.append("[IO_In]dsdn[%zu]={",            cfg.mDSDNSizes.size());
  for( const MSize &size : cfg.mDSDNSizes )
  {
    log.append("%dx%d,", size.w, size.h);
  }
  log.append("}\n");
  if( cfg.mDSDNInfo )
  {
    log.append("[IO_In]dsdnInfo.loopValid(%d), Prof[%zu]={", cfg.mDSDNInfo->mLoopValid, cfg.mDSDNInfo->mProfiles.size());
    for( const MINT32 prof : cfg.mDSDNInfo->mProfiles )
    {
        log.append("%d,", prof);
    }
    log.append("}\n");
  }
  else
  {
    log.append("[IO_In]dsdnInfo.Prof = NULL\n");
  }
}

void print(Log &log, const IO::OutData &cfg)
{
  MSize disp = getImgSize(cfg.mDisplay);
  MSize rec = getImgSize(cfg.mRecord);
  MSize fd = getImgSize(cfg.mFD);
  MRectF fdCrop = getImgCrop(cfg.mFD);
  MSize ds1 = getImgSize(cfg.mDS1);
  MSize phy = getImgSize(cfg.mPhy);
  MSize full = getImgSize(cfg.mFull);

  log.append("[IO_Out]display=%p(%dx%d)\n", cfg.mDisplay.get(), MSIZE_(disp));
  log.append("[IO_Out]record=%p(%dx%d)\n",  cfg.mRecord.get(), MSIZE_(rec));
  log.append("[IO_Out]fd=%p(%dx%d) " IMRECTF_S "\n", cfg.mFD.get(), MSIZE_(fd), IMRECTF_(fdCrop));
  log.append("[IO_Out]phy=%p(%dx%d)\n",     cfg.mPhy.get(), MSIZE_(phy));
  log.append("[IO_Out]extra=%zu\n",         cfg.mExtra.size());
  log.append("[IO_Out]large=%zu\n",         cfg.mLarge.size());
  log.append("[IO_Out]full=%p(%dx%d)\n",    cfg.mFull.get(), MSIZE_(full));
  log.append("[IO_Out]next=%zu\n",          cfg.mNext.size());
  log.append("[IO_Out]ds1=%p(%dx%d)\n",     cfg.mDS1.get(), MSIZE_(ds1));
  log.append("[IO_Out]dn1=%p\n",            cfg.mDN1.get());
  log.append("[IO_Out]omcmv=%p\n",          cfg.mOMCMV.get());
  log.append("[IO_Out]app_out=%p\n",        cfg.mAppOutMeta);
  log.append("[IO_Out]ex_app_out=%p\n",     cfg.mExAppOutMeta);
  log.append("[IO_Out]hal_out=%p\n",        cfg.mHalOutMeta);
}

void print(Log &log, const char *str, const IO::LoopData_const &cfg)
{
  if( cfg )
  {
    log.append("[%s]img3o=%p\n",         str, cfg->mIMG3O.get());
    log.append("[%s]dce_n_1=%p\n",       str, cfg->mDCE_n_1.get());
    log.append("[%s]dce_n_1_magic=%d\n", str, cfg->mDCE_n_1_magic);
    log.append("[%s]dce_n_2=%p\n",       str, cfg->mDCE_n_2.get());
    log.append("[%s]dce_n_2_magic=%d\n", str, cfg->mDCE_n_2_magic);
    log.append("[%s]nr3d_stat=%p\n",     str, cfg->mNR3DStat.get());
  }
}

void print(Log &log, const char *str, const IO::BatchData_const &cfg)
{
  if( cfg )
  {
    log.append("[%s]tun_buffer=%p\n",   str, cfg->mTuning.get());
    log.append("[%s]tun_data=%p\n",     str, cfg->mTuningData.get());
    log.append("[%s]imgi=%p->%p\n",     str, cfg->mIMGI.get(), peak(cfg->mIMGI));
  }
}

void print(const Path &cfg)
{
  Log log;
  print(log, cfg.mP2SW);
  print(log, cfg.mP2HW);
  print(log, cfg.mPMDP);
  print(log, cfg.mMSS, "MSS");
  print(log, cfg.mPMSS, "PMSS");
}

void print(Log &lg, const std::vector<P2SW> &cfg)
{
  (void)lg;
  for( unsigned i = 0, n = cfg.size(); i < n; ++i )
  {
    Log log;
    print(log, cfg[i], i, n);
    MY_LOGD("%s", log.c_str());
  }
}

void print(Log &lg, const std::vector<P2HW> &cfg)
{
  (void)lg;
  for( unsigned i = 0, n = cfg.size(); i < n; ++i )
  {
    Log log;
    print(log, cfg[i], i, n);
    MY_SAFE_LOGD(log.c_str(), log.length());
  }
}

void print(Log &lg, const std::vector<PMDP> &cfg)
{
  (void)lg;
  for( unsigned i = 0, n = cfg.size(); i < n; ++i )
  {
    Log log;
    print(log, cfg[i], i, n);
    MY_LOGD("%s", log.c_str());
  }
}

void print(Log &lg, const std::vector<MSS> &cfg, const char* str)
{
  (void)lg;
  for( unsigned i = 0, n = cfg.size(); i < n; ++i )
  {
    Log log;
    print(log, cfg[i], i, n);
    MY_LOGD("%s,%s", str ? str : "--", log.c_str());
  }
}

void print(Log &log, const P2SW &cfg, unsigned i, unsigned count)
{
  char str[256] = "";
  int res = snprintf(str, sizeof(str)-1, "P2SW(%d/%d)", i, count);
  MY_LOGE_IF( (res < 0), "snprintf failed!");
  const P2SW::In &in = cfg.mIn;
  const P2SW::Out &out = cfg.mOut;
  MSize imgi = getImgSize(in.mIMGI);
  MSize vipi = getImgSize(in.mVIPI);

  print(log, str, in.mBasic);
  print(log, cfg.mFeature);
  log.append("[%s_in]batch_tundata=%p\n", str, cfg.mBatchIn.mTuningData.get());
  log.append("[%s_in]tunType=%s\n",       str, (in.mTuningType == P2SW::OMC) ? "omc" : "p2");
  log.append("[%s_in]imgi=%p(%dx%d)\n",   str, in.mIMGI.get(), MSIZE_(imgi));
  log.append("[%s_in]app_in=%p\n",        str, in.mAppInMeta);
  log.append("[%s_in]hal_in=%p\n",        str, in.mHalInMeta);
  log.append("[%s_in]lcso=%p\n",          str, in.mLCSO.get());
  log.append("[%s_in]lcsho=%p\n",         str, in.mLCSHO.get());

  log.append("[%s_in]dce_n_2=%p\n",       str, in.mDCE_n_2.get());
  log.append("[%s_in]dce_n_2_magic=%d\n", str, in.mDCE_n_2_magic);

  log.append("[%s_in]vipi=%p(%dx%d)\n",   str, in.mVIPI.get(), MSIZE_(vipi));
  log.append("[%s_in]dsdn=%d/%d\n",       str, in.mDSDNIndex, in.mDSDNCount);

  log.append("[%s_out]app_out=%p\n",      str, out.mAppOutMeta);
  log.append("[%s_out]ex_app_out=%p\n",   str, out.mExAppOutMeta);
  log.append("[%s_out]hal_out=%p\n",      str, out.mHalOutMeta);
  log.append("[%s_out]tun=%p\n",          str, out.mTuning.get());
  log.append("[%s_out]msftun=%p\n",       str, out.mMSFTuning.get());
  log.append("[%s_out]msfSram=%p\n",      str, out.mMSFSram.get());
  log.append("[%s_out]omcTun=%p\n",       str, out.mOMCTuning.get());
  log.append("[%s_out]tun_data=%p\n",     str, out.mTuningData.get());
}

void print(Log &log, const P2HW &cfg, unsigned i, unsigned count)
{
  char str[256] = "";
  int res = snprintf(str, sizeof(str)-1, "P2HW(%d/%d)", i, count);
  MY_LOGE_IF( (res < 0), "snprintf failed!");

  const P2HW::In &in = cfg.mIn;
  const P2HW::Out &out = cfg.mOut;

  MSize imgi    = getImgSize(in.mIMGI);
  MSize vipi    = getImgSize(in.mVIPI);
  MSize dsi     = getImgSize(in.mDSI);
  MSize refi    = getImgSize(in.mREFI);
  MSize dswi    = getImgSize(in.mDSWI);
  MSize n1_dswi = getImgSize(in.m_n_1_DSWI);
  MSize idi     = getImgSize(in.mIDI);
  MSize confi   = getImgSize(in.mCONFI);
  MSize weighti = getImgSize(in.mWEIGHTI);
  MSize dswo    = getImgSize(out.mDSWO);
  MSize img2o   = getImgSize(out.mIMG2O);
  MSize img3o   = getImgSize(out.mIMG3O);
  MSize dceso   = getImgSize(out.mDCESO);
  MSize timgo   = getImgSize(out.mTIMGO);
  MSize wdmao   = getImgSize(out.mWDMAO);
  MSize wroto   = getImgSize(out.mWROTO);


  print(log, str, in.mBasic);
  log.append("[%s_in]earlyCB=%d\n",         str, cfg.mNeedEarlyCB);
  log.append("[%s_in]tun=%p\n",             str, in.mTuning.get());
  log.append("[%s_in]msftun=%p\n",          str, in.mMSFTuning.get());
  log.append("[%s_in]msfSram=%p\n",         str, in.mMSFSram.get());
  log.append("[%s_in]tun_data=%p\n",        str, in.mTuningData.get());
  log.append("[%s_in]imgi=%p(%dx%d)\n",     str, in.mIMGI.get(), MSIZE_(imgi));
  log.append("[%s_in]vipi=%p(%dx%d)\n",     str, in.mVIPI.get(), MSIZE_(vipi));
  log.append("[%s_in]msfdsi=%p(%dx%d)\n",   str, in.mDSI.get(), MSIZE_(dsi));
  log.append("[%s_in]msfrefi=%p(%dx%d)\n",  str, in.mREFI.get(), MSIZE_(refi));
  log.append("[%s_in]msfdswi=%p(%dx%d)\n",  str, in.mDSWI.get(), MSIZE_(dswi));
  log.append("[%s_in]msf_n_1_dswi=%p(%dx%d)\n",  str, in.m_n_1_DSWI.get(), MSIZE_(n1_dswi));
  log.append("[%s_in]msfidi=%p(%dx%d)\n",   str, in.mIDI.get(), MSIZE_(idi));
  log.append("[%s_in]msfconfi=%p(%dx%d)\n", str, in.mCONFI.get(), MSIZE_(confi));
  log.append("[%s_in]msfweighti=%p(%dx%d)\n", str, in.mWEIGHTI.get(), MSIZE_(weighti));
  log.append("[%s_in]dsdn=%d/%d\n",         str, in.mDSDNIndex, in.mDSDNCount);

  log.append("[%s_out]msfdswo=%p(%dx%d)\n", str, out.mDSWO.get(), MSIZE_(dswo));
  log.append("[%s_out]img2o=%p(%dx%d)\n",   str, out.mIMG2O.get(), MSIZE_(img2o));
  log.append("[%s_out]img3o=%p(%dx%d)\n",   str, out.mIMG3O.get(), MSIZE_(img3o));
  log.append("[%s_out]dceso=%p(%dx%d)\n",   str, out.mDCESO.get(), MSIZE_(dceso));
  log.append("[%s_out]timgo=%p(%dx%d)\n",   str, out.mTIMGO.get(), MSIZE_(timgo));
  log.append("[%s_out]wdmao=%p(%dx%d)\n",   str, out.mWDMAO.get(), MSIZE_(wdmao));
  log.append("[%s_out]wroto=%p(%dx%d)\n",   str, out.mWROTO.get(), MSIZE_(wroto));
  log.append("[%s_out]p2obj=%p\n",          str, out.mP2Obj.get());
}

void print(Log &log, const PMDP &cfg, unsigned i, unsigned count)
{
  char str[256] = "";
  int res = snprintf(str, sizeof(str)-1, "PMDP(%d/%d)", i, count);
  MY_LOGE_IF( (res < 0), "snprintf failed!");

  log.append("[%s_in]in=%p\n",               str, cfg.mIn.mFull.get());
  log.append("[%s_in]tuning=%p\n",           str, cfg.mIn.mTuning.get());
  log.append("[%s_out]out=%zu\n",            str, cfg.mOut.mExtra.size());
}

void print(Log &log, const MSS &cfg, unsigned i, unsigned count)
{
  char str[256] = "";
  int res = snprintf(str, sizeof(str)-1, "MSS(%d/%d)", i, count);
  MY_LOGE_IF( (res < 0), "snprintf failed!");

  print(log, cfg.mFeature);
  log.append("[%s_in]sensorID=%d\n",           str, cfg.mIn.mSensorID);
  log.append("[%s_in]mssi=%p\n",               str, cfg.mIn.mMSSI.get());
  log.append("[%s_in]omcmv=%p\n",              str, cfg.mIn.mOMCMV.get());
  log.append("[%s_in]omvTun=%p\n",             str, cfg.mIn.mOMCTuning.get());
  for( unsigned j = 0, n = cfg.mIn.mMSSOSizes.size(); j < n; ++j )
  {
    log.append("[%s_in]size[%d]=(%dx%d)\n",  str, j, cfg.mIn.mMSSOSizes[j].w,
                                                cfg.mIn.mMSSOSizes[j].h);
  }
  for( unsigned j = 0, n = cfg.mOut.mMSSOs.size(); j < n; ++j )
  {
    log.append("[%s_out]msso[%d]=%p\n",  str, j, cfg.mOut.mMSSOs[j].get());
  }
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
