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

#include "P2HWUtil.h"
#include <mtkcam3/feature/utils/p2/P2Util.h>

#include "../DebugControl.h"
#define PIPE_CLASS_TAG "P2G_P2HWUtil"
#define PIPE_TRACE TRACE_P2G_P2HWUTIL
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam/utils/TuningUtils/FileReadRule.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

using NSCam::Feature::P2Util::DIPStream;
using NSCam::Feature::P2Util::DIPParams;
using NSCam::Feature::P2Util::DIPFrameParams;
using NSCam::Feature::P2Util::P2Flag;
using NSCam::Feature::P2Util::P2IOPack;
using NSCam::Feature::SecureBufferControl;
using NSCam::NSIoPipe::EDIPSecDMAType;
using NSCam::NSIoPipe::EDIPSecureEnum;
using NSCam::NSIoPipe::NSPostProc::INormalStream;
using NSCam::Feature::P2Util::P2SensorData;
using namespace NSCam::Feature::P2Util;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

// local function

bool processSecure(EDIPSecureEnum &dstTag, void **handle, SecureBufferControl &control, NSIoPipe::EDIPSecDMAType type, EDIPSecureEnum srcTag, IImageBuffer *buffer)
{
  TRACE_FUNC_ENTER();
  bool ret = true;

  if( type == NSIoPipe::EDIPSecDMAType::EDIPSecDMAType_IMAGE )
  {
    dstTag = srcTag;
  }
  else if( (type == NSIoPipe::EDIPSecDMAType::EDIPSecDMAType_TUNE_SHARED ||
            type == NSIoPipe::EDIPSecDMAType::EDIPSecDMAType_TUNE_SHARED_FD) &&
            handle != NULL )
  {
    auto isVA = (type == NSIoPipe::EDIPSecDMAType::EDIPSecDMAType_TUNE_SHARED);
    *handle = (buffer != NULL) ? (void*)control.registerAndGetSecHandle(buffer, isVA) : NULL;
    dstTag = (*handle != NULL) ? srcTag : NSIoPipe::EDIPSecureEnum::EDIPSecure_NONE;
    if( *handle == NULL )
    {
      ret = false;
    }
  }
  else // if( type == NSIoPipe::EDIPSecDMAType::EDIPSecDMAType_TUNE )
  {
    dstTag = NSIoPipe::EDIPSecureEnum::EDIPSecure_NONE;
  }
  TRACE_FUNC_EXIT();
  return ret;
}

void processSecure(DIPFrameParams &frame, SecureBufferControl &control)
{
  TRACE_FUNC_ENTER();
  P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "TH(p2a.ps)::processSecure");
  MUINT32 logLevel = control.mLogLevel;
  NSIoPipe::EDIPSecureEnum secTag = control.getSecureEnum();
  frame.mSecureFra = control.needSecure();

  if( frame.mSecureFra )
  {
    for( auto &in : frame.mvIn )
    {
      EDIPSecDMAType dmaType = INormalStream::needSecureBuffer(in.mPortID.index);
      void *handle = NULL;
      bool secRet = processSecure(in.mSecureTag, &handle, control,
                                  dmaType, secTag, in.mBuffer);
      in.mSecHandle = (MUINTPTR)handle;

      MY_LOGD_IF(logLevel, "In PortID(%u) Buffer(%p) DMAType(%u) SecTag(%d) => SecTag(%d) SecHandle(0x%x/%p)", in.mPortID.index, in.mBuffer, dmaType, secTag, in.mSecureTag, in.mSecHandle, handle);

      MY_LOGE_IF(!secRet, "Invalid sec result: In PortID(%u) Buffer(%p) DMAType(%u) SecTag(%d) => SecTag(%d) SecHandle(0x%x/%p)", in.mPortID.index, in.mBuffer, dmaType, secTag, in.mSecureTag, in.mSecHandle, handle);
    }

    for( auto &out : frame.mvOut )
    {
      EDIPSecDMAType dmaType = INormalStream::needSecureBuffer(out.mPortID.index);
      bool secRet = processSecure(out.mSecureTag, NULL, control,
                                  dmaType, secTag, out.mBuffer);

      MY_LOGD_IF(logLevel, "Out PortID(%u) Buffer(%p) DMAType(%u) SecTag(%d) => SecTag(%d)", out.mPortID.index, out.mBuffer, dmaType, secTag, out.mSecureTag);

      MY_LOGE_IF(!secRet, "Invalid sec result: Out PortID(%u) Buffer(%p) DMAType(%u) SecTag(%d) => SecTag(%d)", out.mPortID.index, out.mBuffer, dmaType, secTag, out.mSecureTag);
    }
  }
  P2_CAM_TRACE_END(TRACE_ADVANCED);
  TRACE_FUNC_EXIT();
}

// P2HWUtil function

P2HWUtil::P2HWUtil()
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
}

P2HWUtil::~P2HWUtil()
{
  TRACE_FUNC_ENTER();
  uninit();
  TRACE_FUNC_EXIT();
}

bool P2HWUtil::init(NSCam::SecType secureType, MUINT32 p2RBMask)
{
  TRACE_FUNC_ENTER();
  mSecureControl.init(secureType);
  mSupportMSF = !property_get_bool("vendor.debug.fpipe.p2g.no_msf", false);
  mP2RBMask = p2RBMask;
  TRACE_FUNC_EXIT();
  return true;
}

bool P2HWUtil::uninit()
{
  TRACE_FUNC_ENTER();
  mSecureControl.uninit();
  TRACE_FUNC_EXIT();
  return true;
}

P2HWUtil::DIPParams P2HWUtil::run(const DataPool &pool, const std::vector<P2HW> &p2hw, MUINT32 optimizeLevel)
{
  (void)pool;
  TRACE_FUNC_ENTER();
  DIPParams params;
  params.mvDIPFrameParams.reserve(p2hw.size());
  if( optimizeLevel )
  {
    for( int i = optimizeLevel; i > 0; --i )
    {
      unsigned index = i - 1;
      for( const P2HW &cfg : p2hw )
      {
        if( (cfg.mIn.mDSDNIndex == index) && check(cfg) )
        {
          params.mvDIPFrameParams.emplace_back(make(cfg));
        }
        if( cfg.mNeedEarlyCB )
        {
          params.mEarlyCBIndex = params.mvDIPFrameParams.size() - 1;
        }
      }
    }
  }
  if( p2hw.size() != params.mvDIPFrameParams.size() )
  {
    for( const P2HW &cfg : p2hw )
    {
      if( (cfg.mIn.mDSDNIndex >= optimizeLevel) && check(cfg) )
      {
        params.mvDIPFrameParams.emplace_back(make(cfg));
      }
      if( cfg.mNeedEarlyCB )
      {
        params.mEarlyCBIndex = params.mvDIPFrameParams.size() - 1;
      }
    }
  }
  if( p2hw.size() != params.mvDIPFrameParams.size() )
  {
    MY_LOGW("p2hw run size mismatch: p2hw=%zu dipParams=%zu",
            p2hw.size(), params.mvDIPFrameParams.size());
  }
  TRACE_FUNC_EXIT();
  return params;
}

bool P2HWUtil::check(bool cond, const char *log) const
{
  if( !cond )
  {
    MY_LOGE("%s", log);
  }
  return cond;
}

bool P2HWUtil::check(const P2HW &cfg) const
{
  TRACE_FUNC_ENTER();
  bool ret = true;
  ret = ret && check(cfg.mIn.mBasic.mP2Pack.isValid(),  "P2Pack is invalid");
  ret = ret && check(bool(cfg.mIn.mTuningData),         "TuningData = NULL");
  ret = ret && check(bool(cfg.mIn.mTuning),             "TuningBuf = NULL");
  ret = ret && check(bool(cfg.mOut.mP2Obj),             "P2Obj = NULL");
  ret = ret && check(bool(cfg.mIn.mIMGI),               "<IMGI> = NULL");
  ret = ret && check(peak(cfg.mIn.mIMGI),               "IMGI = NULL");
  TRACE_FUNC_EXIT();
  return ret;
}

MSize toMSize(const MSizeF sizeF)
{
  return MSize(sizeF.w, sizeF.h);
}

void updateTimestamp(const P2IO &io, MUINT64 timestamp)
{
  if( io.mBuffer )
  {
    if( io.mTimestampCode )
    {
      timestamp = timestamp/1000*1000 + io.mTimestampCode;
    }
    io.mBuffer->setTimestamp((MINT64)timestamp);
  }
}

P2IO toP2IO(const ILazy<GImg> &img, const char *name, unsigned timeout = 0)
{
  (void)name;
  P2IO io;
  if( img )
  {
    img->acquire(timeout);
    io = img->toP2IO();
  }
  return io;
}

MVOID P2HWUtil::readbackVipi(const P2HW &cfg)
{
  TRACE_FUNC_ENTER();

  if (peak(cfg.mIn.mVIPI) != NULL)
  {
      const P2SensorData& sensorData = cfg.mIn.mBasic.mP2Pack.getSensorData();
      const MINT magic3A = sensorData.mMagic3A;

      const int32_t MAX_KEY_LEN = 128;
      char key[MAX_KEY_LEN] = { 0 };
      int res = 0;

      if ( cfg.mFeature.hasDSDN25() || cfg.mFeature.hasDSDN30() ) {
          if ( cfg.mIn.mDSDNIndex == 0 )
              res = snprintf(key, sizeof(key)-1, "vipi");
          else
              res = snprintf(key, sizeof(key)-1, "msf_%d_vipi", cfg.mIn.mDSDNIndex);
      }
      else if ( cfg.mFeature.hasDSDN20() ) {
          if ( cfg.mIn.mDSDNIndex == 1 ) // dsdn21 enables NR3D at L1
              res = snprintf(key, sizeof(key)-1, "p2nr_vipi");
      }
      else { /* do nothing */ }

      MY_LOGW_IF((res< 0), "snprintf failed!");

      readbackBuf(PIPE_CLASS_TAG, sensorData.mNDDHint, (mP2RBMask & P2RB_BIT_READ_P2INBUF),
          magic3A, key, peak(cfg.mIn.mVIPI));
  }

  TRACE_FUNC_EXIT();
}

P2HWUtil::DIPFrameParams P2HWUtil::make(const P2HW &cfg)
{
  TRACE_FUNC_ENTER();
  DIPFrameParams params;
  P2IOPack io;
  MINT64 timestamp = 0;

  IAdvPQCtrl_const pqCtrl = cfg.mIn.mTuningData->mPQCtrl;

  io.mFlag |= cfg.mIn.mBasic.mResized ? P2Flag::FLAG_RESIZED : 0;
  io.mIMGI.mBuffer = peak(cfg.mIn.mIMGI);
  io.mVIPI.mBuffer = peak(cfg.mIn.mVIPI);

  if ( (mP2RBMask & P2RB_BIT_ENABLE) )
  {
      readbackVipi(cfg);
  }

  timestamp = io.mIMGI.mBuffer ? io.mIMGI.mBuffer->getTimestamp() : 0;

  if( mSupportMSF && cfg.mIn.mDSI )
  {
    io.mMSFDSI = peak(cfg.mIn.mDSI);
    io.mMSFREFI = peak(cfg.mIn.mREFI);
    io.mMSFDSWI = peak(cfg.mIn.mDSWI);
    io.mMSF_n_1_DSWI = peak(cfg.mIn.m_n_1_DSWI);

    io.mMSFCONFI = acquirePeak(cfg.mIn.mCONFI);
    io.mMSFIDI = acquirePeak(cfg.mIn.mIDI);
    io.mMSFWEIGHTI = acquirePeak(cfg.mIn.mWEIGHTI);
    io.mMSFDSWO = acquirePeak(cfg.mOut.mDSWO);

    cfg.mOut.mP2Obj->mMSFConfig.msf_scenario = io.mMSFREFI ? NSIoPipe::NSMFB20::MSF_DSDN_REFI_DL_MODE : NSIoPipe::NSMFB20::MSF_DSDN_DL_MODE;
    cfg.mOut.mP2Obj->mMSFConfig.frame_Idx = 1;
    cfg.mOut.mP2Obj->mMSFConfig.frame_Total = io.mMSFREFI ? 2 : 1;
    cfg.mOut.mP2Obj->mMSFConfig.scale_Idx = cfg.mIn.mDSDNIndex;
    cfg.mOut.mP2Obj->mMSFConfig.scale_Total = cfg.mIn.mDSDNCount + 1;
  }

  if( cfg.mOut.mIMG2O ) io.mIMG2O = toP2IO(cfg.mOut.mIMG2O, "img2o");
  if( cfg.mOut.mIMG3O ) io.mIMG3O = toP2IO(cfg.mOut.mIMG3O, "img3o");
  if( cfg.mOut.mWDMAO ) io.mWDMAO = toP2IO(cfg.mOut.mWDMAO, "wdmao");
  if( cfg.mOut.mWROTO ) io.mWROTO = toP2IO(cfg.mOut.mWROTO, "wroto");
  if( cfg.mOut.mDCESO ) io.mDCESO = toP2IO(cfg.mOut.mDCESO, "dceso");
  if( cfg.mOut.mTIMGO ) io.mTIMGO = toP2IO(cfg.mOut.mTIMGO, "timgo");

  if( io.mWDMAO.isValid() )
  {
    cfg.mOut.mP2Obj->setWdmaPQ(makeDpPqParam(pqCtrl.get(), io.mWDMAO.mType, io.mWDMAO.mBuffer, PIPE_CLASS_TAG));
  }

  if( io.mWROTO.isValid() )
  {
    cfg.mOut.mP2Obj->setWrotPQ(makeDpPqParam(pqCtrl.get(), io.mWROTO.mType, io.mWROTO.mBuffer, PIPE_CLASS_TAG));
  }

  if( io.mTIMGO.isValid() )
  {
    cfg.mOut.mP2Obj->mTimgoParam = cfg.mIn.mBasic.mTimgoType;
  }

  updateTimestamp(io.mIMG2O, timestamp);
  updateTimestamp(io.mIMG3O, timestamp);
  updateTimestamp(io.mWDMAO, timestamp);
  updateTimestamp(io.mWROTO, timestamp);

  cfg.mOut.mP2Obj->mRunIndex = cfg.mIn.mBasic.mP2DebugIndex;
  cfg.mOut.mP2Obj->mProfile = cfg.mIn.mTuningData->mProfile;

  params = makeDIPFrameParams(cfg.mIn.mBasic.mP2Pack, cfg.mIn.mBasic.mP2Tag, io, cfg.mOut.mP2Obj->toPtrTable(), cfg.mIn.mTuningData->mParam, cfg.mIn.mBasic.mRegDump);

  processSecure(params, mSecureControl);

  TRACE_FUNC_EXIT();
  return params;
}


} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
