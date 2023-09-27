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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#define PIPE_CLASS_TAG "EISNode"

#include "EISNode.h"

#include <sys/resource.h>
#include <sys/stat.h>

#include <algorithm>

#include "WarpBase.h"
#include "featurePipe/core/include/PipeLog.h"
#include "mtkcam/drv/iopipe/CamIO/IHalCamIO.h"
#include "mtkcam3/feature/eis/eis_hal.h"
#include "mtkcam3/feature/fsc/fsc_ext.h"
#include "mtkcam3/feature/utils/FeatureCache.h"
#include "system/thread_defs.h"
#include "utils/ThreadDefs.h"
#define PIPE_TRACE TRACE_EIS_NODE

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

#define EISNODE_THREAD_PRIORITY (ANDROID_PRIORITY_FOREGROUND - 3)

#define EISNODE_DEBUG_FSC_BUFFER "vendor.debug.eis.fsc"
#define EISNODE_DUMP_FOLDER_PATH "/data/vendor/dump"
#define EIS_TIMEOUT_MS 100
#define EIS_TIMEOUT_FLUSH_MS 1000

#if SUPPORT_EIS

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

EISNode::EISNode(const char* name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_EIS),
      StreamingFeatureNode(name),
      mWarpMapPool(NULL),
      mpEisHal(NULL),
      mWarpGridSize(2, 2) {
  TRACE_FUNC_ENTER();

  mEnableDump =
      (getPropertyValue(EISNODE_DEBUG_FSC_BUFFER, 0) == 1) ? MTRUE : MFALSE;
  TRACE_FUNC_EXIT();
}

EISNode::~EISNode() {
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
}

MBOOL EISNode::onData(DataID id, const RequestPtr& data) {
  TRACE_FUNC_ENTER();
  TRACE_FUNC("Frame %d: %s arrived", data->mRequestNo, ID2Name(id));
  MBOOL ret = MFALSE;
  if (id == ID_P2G_TO_EIS_P2DONE || id == ID_ROOT_TO_EIS) {
    mRequests.enque(data);
    ret = MTRUE;
  }
  TRACE_FUNC_EXIT();
  return ret;
}

MBOOL EISNode::onData(DataID id, const RSCData& data) {
  TRACE_FUNC_ENTER();
  TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
  MBOOL ret = MFALSE;
  if (id == ID_RSC_TO_EIS) {
    mRSCDatas.enque(data);
    ret = MTRUE;
  }
  TRACE_FUNC_EXIT();
  return ret;
}

MBOOL EISNode::onInit() {
  TRACE_FUNC_ENTER();
  StreamingFeatureNode::onInit();
  this->setTimeoutMS(EIS_TIMEOUT_MS);
  this->addWaitQueue(&mRequests);
  if (mPipeUsage.supportRSCNode()) {
    this->addWaitQueue(&mRSCDatas);
  }

  initEIS();
  mSourceDump = getPropertyValue("vendor.debug.eis.sourcedump", mSourceDump);
  TRACE_FUNC_EXIT();
  return MTRUE;
}

MBOOL EISNode::onUninit() {
  TRACE_FUNC_ENTER();

  if (mpEisHal) {
    mpEisHal->Uninit();
    mpEisHal->DestroyInstance("FeaturePipe_EisNode");
    mpEisHal = NULL;
  }

  TRACE_FUNC_EXIT();
  return MTRUE;
}

MBOOL EISNode::onThreadStart() {
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  int EIS_WARP_MAP_NUM = mEisInfo.previewEIS ? 12 : 6;
  prepareBufferPool();
  if (mWarpMapPool != NULL) {
    mWarpMapPool->allocate(EIS_WARP_MAP_NUM);
    ret = MTRUE;
  }
  MY_LOGD("%s allocate %d warpmaps", STR_ALLOCATE, EIS_WARP_MAP_NUM);

  ::setpriority(PRIO_PROCESS, 0, EISNODE_THREAD_PRIORITY);
  TRACE_FUNC_EXIT();
  return ret;
}

MBOOL EISNode::onThreadStop() {
  TRACE_FUNC_ENTER();
  FatImageBufferPool::destroy(mWarpMapPool);

  if (mQueue.size()) {
    MY_LOGW("Queue not empty size=%zu", mQueue.size());
    mQueue.clear();
    mRefCount = 0;
  }
  TRACE_FUNC_EXIT();
  return MTRUE;
}

MBOOL EISNode::onThreadLoop() {
  TRACE_FUNC("Waitloop");
  RequestPtr request;
  RSCData rscData;

  if (waitAllQueue()) {
    if (mPipeUsage.supportRSCNode()) {
      if (!mRSCDatas.deque(rscData)) {
        MY_LOGE("RSCData deque out of sync");
        return MFALSE;
      }
    }
    if (!mRequests.deque(request)) {
      MY_LOGE("P2A done deque out of sync");
      return MFALSE;
    } else if (request == NULL) {
      MY_LOGE("No p2aRequest data!");
      return MFALSE;
    }

    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    if (needFlushAll(request)) {
      this->flushAllProcess(rscData.mData);
    }
    request->mTimer.startEIS();
    markNodeEnter(request);
    TRACE_FUNC("Frame %d in EIS needEIS=%d queue size=%zu hasRecord=%d",
               request->mRequestNo, request->needEIS(), mQueue.size(),
               request->hasRecordOutput());
    if (request->needEIS()) {
      processEIS(request, rscData.mData);
    } else {
      WarpImg dummyWarpImg;
      if (mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW))
        handleWarpResult(request, dummyWarpImg, ID_EIS_TO_WARP_P);
      if (mPipeUsage.supportWarpNode(SFN_HINT::RECORD))
        handleWarpResult(request, dummyWarpImg, ID_EIS_TO_WARP_R);
    }
    request->mTimer.stopEIS();
  }

  MUINT32 idleTime = mNodeSignal->getIdleTimeMS();
  mInFlush = mNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH) ||
             (idleTime > EIS_TIMEOUT_FLUSH_MS);
  if (mInFlush) {
    MY_LOGI("Need flush signal(%d) waitTimeMs(%u)",
            mNodeSignal->getStatus(NodeSignal::STATUS_IN_FLUSH), idleTime);
    this->flushAllProcess(rscData.mData);
  }
  TRACE_FUNC_EXIT();
  return MTRUE;
}

MBOOL EISNode::initEIS() {
  TRACE_FUNC_ENTER();

  mEisInfo.mode = mPipeUsage.getEISMode();
  mEisInfo.factor = mPipeUsage.getEISFactor();
  mEisInfo.videoConfig = mPipeUsage.getEISVideoConfig();
  mEisInfo.queueSize = mPipeUsage.getEISQueueSize();
  mEisInfo.startFrame = mPipeUsage.getEISStartFrame();
  mEisInfo.previewEIS = mPipeUsage.supportPreviewEIS();

  if (mPipeUsage.supportFSC()) {
    mFSCInfo.isEnabled = MTRUE;
    mFSCInfo.numSlices = FSC_WARPING_SLICE_NUM;
  }

  mpEisHal = EisHal::CreateInstance("FeaturePipe_EisNode", mEisInfo,
                                    mSensorIndex, MFALSE);
  if (mpEisHal == NULL) {
    MY_LOGE("FeaturePipe_EisNode: Create EIS Instance failed");
    return MFALSE;
  }

  if (EIS_RETURN_NO_ERROR != mpEisHal->Init()) {
    MY_LOGE("FeaturePipe_EisNode: mpEisHal init failed");
    return MFALSE;
  }

  TRACE_FUNC_EXIT();
  return MTRUE;
}

MVOID EISNode::uninitEIS() {}

MVOID EISNode::prepareBufferPool() {
  MINT32 eis_mode =
      property_get_int32("vendor.debug.algo.eis.mode", ALGO_EIS50_MODE);
  if (eis_mode == ALGO_EIS35_MODE) {
    mWarpGridSize = MSize(31, 18);
  } else {
    mWarpGridSize = MSize(33, 19);
  }

  MY_LOGD("EIS Warp Matrix = Grid W(%d), H(%d), EISMode(0x%x)", mWarpGridSize.w,
          mWarpGridSize.h, mEisInfo.mode);

  mWarpMapPool = FatImageBufferPool::create(
      "eis_warpmap", mWarpGridSize, eImgFmt_WARP_2PLANE,
      FatImageBufferPool::USAGE_HW_AND_SW);
}

MVOID EISNode::processEIS(const RequestPtr& request, const RSCResult& rsc) {
  TRACE_FUNC_ENTER();
  EIS_HAL_CONFIG_DATA config;

  prepareEIS(request, config);
  if (EIS_MODE_IS_EIS_30_ENABLED(mEisInfo.mode)) {
    processEIS30(request, config, rsc);
  } else {
    MY_LOGE("Frame %d missing EISMode(%d)", request->mRequestNo, mEisInfo.mode);
  }

  TRACE_FUNC_EXIT();
}

MBOOL EISNode::prepareEIS(const RequestPtr& request,
                          EIS_HAL_CONFIG_DATA& config) {
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  extractConfig(request, mEISHalCfgData);
  ret = applyConfig(request, mEISHalCfgData);
  memcpy(&config, &mEISHalCfgData, sizeof(EIS_HAL_CONFIG_DATA));
  TRACE_FUNC_EXIT();
  return ret;
}

MVOID EISNode::extractConfig(const RequestPtr& request,
                             EIS_HAL_CONFIG_DATA& config) {
  TRACE_FUNC_ENTER();
  MSize sensorSize = request->getVar<MSize>(SFP_VAR::EIS_SENSOR_SIZE, MSize());
  MRect scalerCrop = request->getVar<MRect>(SFP_VAR::EIS_SCALER_CROP, MRect());
  MSize scalerOutSize =
      request->getVar<MSize>(SFP_VAR::EIS_SCALER_SIZE, MSize());
  MSizeF gpuTargetSize;

  config.gmv_X = request->getVar<MINT32>(SFP_VAR::EIS_GMV_X, 0);
  config.gmv_Y = request->getVar<MINT32>(SFP_VAR::EIS_GMV_Y, 0);
  config.confX = request->getVar<MINT32>(SFP_VAR::EIS_CONF_X, 0);
  config.confY = request->getVar<MINT32>(SFP_VAR::EIS_CONF_Y, 0);

  config.sensorIdx = request->getMasterID();
  config.vHDREnabled = request->needVHDR() ? 1 : 0;
  config.sensor_Width = sensorSize.w;
  config.sensor_Height = sensorSize.h;
  config.rrz_crop_Width = scalerCrop.s.w;
  config.rrz_crop_Height = scalerCrop.s.h;
  config.rrz_crop_X = scalerCrop.p.x;
  config.rrz_crop_Y = scalerCrop.p.y;
  config.rrz_scale_Width = scalerOutSize.w;
  config.rrz_scale_Height = scalerOutSize.h;
  config.standard_EIS = mEisInfo.previewEIS;
  config.advanced_EIS = mPipeUsage.supportEIS_Q();
  config.sensorMode = request->getVar<MINT32>(SFP_VAR::EIS_SENSOR_MODE, 0);
  config.iso = request->getVar<MINT32>(SFP_VAR::EIS_ISO, 0);
  config.gain = request->getVar<MINT32>(SFP_VAR::EIS_GAIN, 0);
  config.fps = std::max(request->mP2Pack.getFrameData().mMinFps, 30);
  config.requestNo = request->mRequestNo;
  config.staggerNum = mStaggerNum =
      request->getVar<MUINT32>(SFP_VAR::EIS_STAGGER_VALID_NUM, 0);
  config.warpmapSize = mWarpGridSize;
  config.warpmapStride = mWarpGridSize;

  MSize inSize = request->getEISInputSize();
  MSizeF inSizeF(inSize.w, inSize.h);
  inSize = request->getEISInputSize();
  gpuTargetSize = inSizeF - request->getEISMarginPixel() * 2;

  if (mPipeUsage.supportFSC()) {
    MFLOAT eisWidthRatio = request->getVar<MFLOAT>(SFP_VAR::EIS_WIDTH_RATIO, 0);
    MFLOAT eisHeightRatio =
        request->getVar<MFLOAT>(SFP_VAR::EIS_HEIGHT_RATIO, 0);
    // Algo targetSize is relative to FSC max cropped region
    MSize margin = request->getFSCMaxMarginPixel();
    gpuTargetSize.w = (inSizeF.w - margin.w * 2) * eisWidthRatio;
    gpuTargetSize.h = (inSizeF.h - margin.h * 2) * eisHeightRatio;
  }

  config.gpuTargetW = gpuTargetSize.w;
  config.gpuTargetH = gpuTargetSize.h;

  config.crzOutW = request->getEISInputSize().w;
  config.crzOutH = request->getEISInputSize().h;
  config.srzOutW = request->getEISInputSize().w;
  config.srzOutH = request->getEISInputSize().h;
  config.feTargetW = request->getEISInputSize().w;
  config.feTargetH = request->getEISInputSize().h;
  config.cropX = 0;  // No longer use CRZ crop
  config.cropY = 0;  // No longer use CRZ crop
  config.imgiW = request->getEISInputSize().w;
  config.imgiH = request->getEISInputSize().h;
  config.warp_precision = mPipeUsage.getWarpPrecision();
  config.recordNo = request->mRecordNo;
  config.timestamp = request->getVar<MINT64>(SFP_VAR::EIS_TIMESTAMP, 0);

  if (mUsePerFrameSetting) {
    mEISQuickDump = property_get_int32("vendor.debug.eis.quick.dump", 0);
  }

  if (mSourceDump && request->mRecordNo >= 1) {
    // dump recording scenario
    mDumpIdx = (request->mRecordNo == 1) ? 1 : (mDumpIdx + 1);
  } else if ((mSourceDump && request->hasExtraOutput()) || mEISQuickDump) {
    // dump preview with extraBuffer scenario
    mDumpIdx = (request->mRequestNo == 0) ? 1 : (mDumpIdx + 1);
  } else if ((request->mRecordNo < mRecordNo) || !mEISQuickDump) {
    // record stopped or NDD dump stopped
    mDumpIdx = 0;
  }

  mRecordNo = request->mRecordNo;
  config.sourceDumpIdx = mDumpIdx;
  config.quickDump = mEISQuickDump;

  if (mEnableDump && mLogCount == 0) {
    mLogCount = (request->getVar<MINT64>(SFP_VAR::EIS_TIMESTAMP, 0) /
                 (MUINT64)1000000000LL);
    char path[256] = {0};
    int result =
        snprintf(path, sizeof(path), EISNODE_DUMP_FOLDER_PATH "/%d", mLogCount);
    if (result < 0 || mkdir(EISNODE_DUMP_FOLDER_PATH, S_IRWXU | S_IRWXG) ||
        mkdir(path, S_IRWXU | S_IRWXG)) {
      MY_LOGW("mkdir/snprintf operation failed, result=%d", result);
    }
  }
  MY_LOGD(
      "Frame:%d/"
      "%d(App=%d),eisMode:0x%x,factor:%d,sensor(%dx%d),scalerCrop:(%d,%d)(%dx%"
      "d)=>Out(%dx%d),imgi(%u,%u),crz(%u,%u),srz(%u,%u),feT(%u,%u),gpuT(%u,%u),"
      "crop(%u,%u),pri(%d),fsc(%d/%d).qSize=%zu,qAction=%d,qCounter=%d",
      request->mRequestNo, request->mRecordNo,
      request->getVar<IStreamingFeaturePipe::eAppMode>(
          SFP_VAR::APP_MODE, IStreamingFeaturePipe::APP_PHOTO_PREVIEW),
      mEisInfo.mode, mEisInfo.factor, config.sensor_Width, config.sensor_Height,
      config.rrz_crop_X, config.rrz_crop_Y, config.rrz_crop_Width,
      config.rrz_crop_Height, config.rrz_scale_Width, config.rrz_scale_Height,
      config.imgiW, config.imgiH, config.crzOutW, config.crzOutH,
      config.srzOutW, config.srzOutH, config.feTargetW, config.feTargetH,
      config.gpuTargetW, config.gpuTargetH, config.cropX, config.cropY,
      config.warp_precision, mFSCInfo.isEnabled, mFSCInfo.numSlices,
      mQueue.size(), request->getEISQAction(), request->getEISQCounter());

  MY_LOGD_IF(
      request->mP2Pack.getConfigInfo().mAutoTestLog,
      "[CAT][EIS] imgi_w:%u imgi_h:%u warpout_w:%u warpout_h:%u factor:%d",
      config.imgiW, config.imgiH, config.gpuTargetW, config.gpuTargetH,
      mEisInfo.factor);
  TRACE_FUNC_EXIT();
}

MBOOL EISNode::applyConfig(const RequestPtr& request,
                           EIS_HAL_CONFIG_DATA& config) {
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  MINT32 eisRet = 0;

  if (EIS_MODE_IS_EIS_30_ENABLED(mEisInfo.mode)) {
    eisRet = request->needFSC()
                 ? mpEisHal->ConfigRSCMEEis(config, mEisInfo.mode, &mFSCInfo)
                 : mpEisHal->ConfigRSCMEEis(config, mEisInfo.mode);
    ret = (eisRet == EIS_RETURN_NO_ERROR);
    MY_LOGE_IF(!ret, "EISNode ConfigRSCMEEis fail (%d)", eisRet);
  }

  TRACE_FUNC_EXIT();
  return ret;
}

MBOOL EISNode::processEIS30(const RequestPtr& request,
                            EIS_HAL_CONFIG_DATA& config,
                            const RSCResult& rsc,
                            const MBOOL isFlush) {
  TRACE_FUNC_ENTER();

  ImgBuffer recordWarpMap = NULL;
  ImgBuffer displayWarpMap = NULL;
  FSC::FSC_WARPING_DATA_STRUCT fsc;

  IMAGE_BASED_DATA imgBaseData;
  LMV_PACKAGE lmvCfg;
  RSCME_PACKAGE rscCfg;
  FSC_PACKAGE fscCfg;

  MINT64 ts = request->getVar<MINT64>(SFP_VAR::EIS_TIMESTAMP, 0);
  MINT64 expTime = request->getVar<MINT32>(SFP_VAR::EIS_EXP_TIME, 0);
  MINT64 longExpTime = request->getVar<MINT32>(SFP_VAR::EIS_LONGEXP_TIME, 0);

  OutputInfo dispInfo;
  MRectF dispCrop;
  if (request->getOutputInfo(IO_TYPE_DISPLAY, dispInfo)) {
      dispCrop = dispInfo.mCropRect;
  }

  recordWarpMap = mWarpMapPool->requestIIBuffer();
  if (mEisInfo.previewEIS) {
    displayWarpMap = mWarpMapPool->requestIIBuffer();
  }

  lmvCfg.enabled = request->tryGetVar<EIS_STATISTIC_STRUCT>(
      SFP_VAR::EIS_LMV_DATA, lmvCfg.data);
  prepareRSC(rsc, rscCfg, config);
  prepareFSC(request, fsc, fscCfg);

  imgBaseData.lmvData = &lmvCfg;
  imgBaseData.rscData = &rscCfg;
  imgBaseData.fscData = &fscCfg;

  EIS_HAL_CONFIG_DATA oriConfigData = config;
  EISQData qData = decideQData(request, config, isFlush);

  EIS_HAL_CONFIG_DATA cfgData =
      (oriConfigData.imgiW) != 0 ? oriConfigData : qData.mConfig;
  configAdvEISConfig(request, recordWarpMap, displayWarpMap, qData, cfgData);
  EIS_HAL_OUT_DATA outData;
  mpEisHal->DoRSCMEEis(&cfgData, &imgBaseData, ts, expTime, longExpTime,
                       mStaggerNum, outData);

  NSCam::Feature::EISCache::getInstance()->setEISCrop(
        outData.xOffset, outData.yOffset,
        outData.leftTopX, outData.leftTopY,
        outData.rightDownX, outData.rightDownY,
        config.imgiW, config.imgiH,
        config.gpuTargetW, config.gpuTargetH,
        dispCrop);
  handleQDataWarpResult(request, recordWarpMap, displayWarpMap, qData, isFlush);

  TRACE_FUNC_EXIT();
  return MTRUE;
}

MINTPTR getVA(const ImgBuffer& img) {
  MINTPTR va = NULL;
  if (img != NULL) {
    IImageBuffer* ptr = NULL;
    ptr = img->getImageBufferPtr();
    ptr->syncCache(eCACHECTRL_INVALID);
    va = ptr->getBufVA(0);
  }
  return va;
}

template <typename T>
T getVal(const ImgBuffer& img) {
  T val = 0;
  if (img != NULL) {
    IImageBuffer* ptr = NULL;
    MINTPTR va = NULL;
    ptr = img->getImageBufferPtr();
    ptr->syncCache(eCACHECTRL_INVALID);
    va = ptr->getBufVA(0);
    val = *((T*)va);
  }
  return val;
}

MVOID EISNode::prepareRSC(const RSCResult& rsc,
                          RSCME_PACKAGE& rscCfg,
                          EIS_HAL_CONFIG_DATA& config) {
  TRACE_FUNC_ENTER();
  if (rsc.mIsValid) {
    rscCfg.RSCME_mv = (MUINT8*)getVA(rsc.mMV);
    rscCfg.RSCME_var = (MUINT8*)getVA(rsc.mBV);
    config.rssoWidth = rsc.mRssoSize.w;
    config.rssoHeight = rsc.mRssoSize.h;
  }
  TRACE_FUNC_EXIT();
}

MVOID EISNode::prepareFSC(const RequestPtr& request,
                          FSC::FSC_WARPING_DATA_STRUCT& fsc,
                          FSC_PACKAGE& fscCfg) {
  TRACE_FUNC_ENTER();
  if (mPipeUsage.supportFSC()) {
    MBOOL ret = request->tryGetVar<FSC::FSC_WARPING_DATA_STRUCT>(
        SFP_VAR::FSC_RRZO_WARP_DATA, fsc);
    if (ret) {
      fscCfg.procWidth =
          request->getEISInputSize().w - request->getFSCMaxMarginPixel().w * 2;
      fscCfg.procHeight =
          request->getEISInputSize().h - request->getFSCMaxMarginPixel().h * 2;
      fscCfg.scalingFactor = fsc.fsc_warp_result.scale_list;
      TRACE_FUNC("inSize(%dx%d),FSCMaxMargin(%dx%d)=>FSC proc(%dx%d)",
                 request->getEISInputSize().w, request->getEISInputSize().h,
                 request->getFSCMaxMarginPixel().w,
                 request->getFSCMaxMarginPixel().h, fscCfg.procWidth,
                 fscCfg.procHeight);
    } else {
      MY_LOGW("Cannot get FSC data");
    }

    if (request->needDump() || mEnableDump) {
      MUINT32 size = sizeof(FSC_WARPING_RESULT_INFO_STRUCT);

      char path[256] = {0};
      int result = snprintf(path, sizeof(path),
                            EISNODE_DUMP_FOLDER_PATH
                            "%d/%04d_r%04d_fsc_warp_data_%d_%d.bin",
                            mLogCount, request->mRequestNo, request->mRecordNo,
                            size, request->needFSC());
      if (result < 0) {
        MY_LOGW("snprintf return error %d", result);
        return;
      }
      dumpData((char*)&fsc.fsc_warp_result, size, path);
    }
  }
  TRACE_FUNC_EXIT();
}

MSizeF EISNode::getDomainOffset(const RequestPtr& request) {
  TRACE_FUNC_ENTER();
  MSizeF domainOffset = request->getEISMarginPixel();
  TRACE_FUNC("frame(%d) EISNode domainoffset=(%fx%f)", request->mRequestNo,
             domainOffset.w, domainOffset.h);
  TRACE_FUNC_EXIT();
  return domainOffset;
}

EISQData EISNode::decideQData(const RequestPtr& request,
                              const EIS_HAL_CONFIG_DATA& config,
                              const MBOOL needFlush) {
  TRACE_FUNC_ENTER();
  EISQ_ACTION qAction = request->getEISQAction();
  MUINT32 qCounter = request->getEISQCounter();
  RequestPtr qDataRequest = mPipeUsage.supportEIS_RQQ() ? request : NULL;
  EISQData qData(qDataRequest, config, request->getEISCropRegion());
  MBOOL noQ = MTRUE;

  if (needFlush) {
    qAction = EISQ_ACTION_STOP;
    if (!mQueue.empty()) {
      qData = mQueue.front();
      mQueue.pop_front();
      if (mRefCount) {
        --mRefCount;
      }
    }
  } else if (qAction == EISQ_ACTION_PUSH) {
    mQueue.push_back(qData);
    ++mRefCount;
    noQ = MFALSE;
    qData.pushOnly = !needFlush;
  } else if (qAction == EISQ_ACTION_RUN || qAction == EISQ_ACTION_STOP ||
             qAction == EISQ_ACTION_NO) {
    mQueue.push_back(qData);
    qData = mQueue.front();
    mQueue.pop_front();
    if (qAction == EISQ_ACTION_STOP && mRefCount) {
      --mRefCount;
    }
    noQ = MFALSE;
  } else if (qAction == EISQ_ACTION_INIT || qAction == EISQ_ACTION_READY) {
  }

  switch (qAction) {
    case EISQ_ACTION_NO:
      qData.setAlg(EISALG_QUEUE_NONE, 0);
      break;
    case EISQ_ACTION_INIT:
      mFlushDone = MFALSE;
      qData.setAlg(EISALG_QUEUE_INIT, 0);
      break;
    case EISQ_ACTION_PUSH:
      qData.setAlg(EISALG_QUEUE_WAIT, qCounter);
      break;
    case EISQ_ACTION_RUN:
      qData.setAlg(EISALG_QUEUE, mEisInfo.queueSize);
      break;
    case EISQ_ACTION_STOP:
      if (!mFlushDone) {
        qData.setAlg(EISALG_QUEUE_STOP, mRefCount);
      } else {
        qData.setAlg(EISALG_QUEUE_NONE, 0);
      }
      break;
    case EISQ_ACTION_READY:
      qData.setAlg(EISALG_QUEUE_NONE, 0);
      break;
    default:
      qData.setAlg(EISALG_QUEUE_NONE, 0);
      break;
  }

  if ((noQ && mQueue.size()) || (qCounter != mRefCount)) {
    MY_LOGW("noQ=%d, QSize=%zu, action=%d, qCounter=%d, refCount=%d", noQ,
            mQueue.size(), qAction, qCounter, mRefCount);
  }

  TRACE_FUNC(
      "Frame: %d, qReq=%d QSize=%zu qAction=%d qCounter=%d algMode=%d "
      "algCounter=%d pushOnly=%d needFlush=%d",
      request->mRequestNo, qData.getReqNo(), mQueue.size(), qAction, qCounter,
      qData.mAlgMode, qData.mAlgCounter, qData.pushOnly, needFlush);
  TRACE_FUNC_EXIT();
  return qData;
}

MVOID EISNode::configAdvEISConfig(const RequestPtr& request,
                                  const ImgBuffer& recordWarp,
                                  const ImgBuffer& displayWarp,
                                  const EISQData& qData,
                                  EIS_HAL_CONFIG_DATA& config) {
  (void)request;
  TRACE_FUNC_ENTER();
  mpEisHal->SetEisPlusWarpInfo(
      recordWarp != NULL ? (MINT32*)(recordWarp->getImageBuffer())->getBufVA(0)
                         : NULL,
      recordWarp != NULL ? (MINT32*)(recordWarp->getImageBuffer())->getBufVA(1)
                         : NULL,
      displayWarp != NULL
          ? (MINT32*)(displayWarp->getImageBuffer())->getBufVA(0)
          : NULL,
      displayWarp != NULL
          ? (MINT32*)(displayWarp->getImageBuffer())->getBufVA(1)
          : NULL);

  config.process_mode = mInFlush ? EISALG_QUEUE_NONE : qData.mAlgMode;
  config.process_idx = mInFlush ? 0 : qData.mAlgCounter;

  TRACE_FUNC("Frame: %d, Queue size: %zu, _mode: %d, _idx: %d",
             request->mRequestNo, mQueue.size(), config.process_mode,
             config.process_idx);
  TRACE_FUNC_EXIT();
}

MVOID EISNode::sourceDump(const RequestPtr& request) {
  if (mSourceDump || mUsePerFrameSetting) {
    if (request->needEIS()) {
      EISSourceDumpInfo info;
      info.dumpType = (request->mRecordNo >= 1) ? EISSourceDumpInfo::TYPE_RECORD
                                                : EISSourceDumpInfo::TYPE_EXTRA;
      if (mDumpIdx == 1) {
        char path[100];
        MINT64 timestamp = request->getVar<MINT64>(SFP_VAR::EIS_TIMESTAMP, 0);
        if (snprintf(path, sizeof(path), EIS_DUMP_FOLDER_PATH "/%" PRId64 "",
                     timestamp) < 0 ||
            mkdir(path, S_IRWXU | S_IRWXG)) {
          MY_LOGW("snprintf/mkdir operation failed %s", path);
        }
        if (snprintf(path, sizeof(path),
                     EIS_DUMP_FOLDER_PATH "/%" PRId64 "/%" PRId64 ".jpg",
                     timestamp, timestamp) < 0) {
          MY_LOGW("snprintf failed");
        }
        info.filePath.assign(path);
        mDumpFileCreated = 1;

        if (mEISQuickDump || mSourceDump) {
          char frameNo[100];
          TuningUtils::FILE_DUMP_NAMING_HINT hint =
              request->mP2Pack.getSensorData(request->mMasterID).mNDDHint;
          if (snprintf(frameNo, sizeof(frameNo), "%d_%d_%d", hint.UniqueKey,
                       hint.FrameNo, hint.RequestNo) < 0 ||
              snprintf(path, sizeof(path),
                       EIS_DUMP_FOLDER_PATH "/%" PRId64
                                            "/First_EIS_Dump_FrameNo.txt",
                       timestamp) < 0) {
            MY_LOGW("snprintf failed");
          }

          FILE* pFile = fopen(path, "wb");
          if (pFile != NULL) {
            if (fwrite(frameNo, strlen(frameNo), 1, pFile) == 0 ||
                fclose(pFile) != 0) {
              MY_LOGW("IO failed. errno(%d) reason: %s", errno,
                      strerror(errno));
            }
          }
        }
      }

      if (mDumpIdx >= 300 && mDumpIdx <= 302) {
        // mark frame for sync warped/unwarped video
        info.markFrame = true;
      }
      // info.needDump = true;
      info.dumpIdx = mDumpIdx;

      request->setEISDumpInfo(info);
    } else {
      mDumpFileCreated = 0;
      mDumpIdx = 0;
    }
  }
}

MBOOL EISNode::handleWarpResult(const RequestPtr& request,
                                const WarpImg& warpImg,
                                const StreamingFeatureDataID toWarpDataID) {
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  sourceDump(request);
  TRACE_FUNC("Frame: %d EISNode Send Data %s warpmapImg=%p",
             request->mRequestNo, ID2Name(toWarpDataID), warpImg.mBuffer.get());
  markNodeExit(request);
  ret = handleData(toWarpDataID, WarpData(warpImg, request));

  TRACE_FUNC_EXIT();
  return ret;
}

MVOID EISNode::handleQDataWarpResult(const RequestPtr& request,
                                     const ImgBuffer& recordWarp,
                                     const ImgBuffer& displayWarp,
                                     const EISQData& qData,
                                     const MBOOL isFlush) {
  RequestPtr qRequest = mPipeUsage.supportEIS_RQQ() ? qData.mRequest : request;
  MINT32 frameGap = request->mRequestNo - qRequest->mRequestNo;
  if (mPipeUsage.supportWarpNode(SFN_HINT::PREVIEW) && !isFlush) {
    SyncOrder order = qData.pushOnly ? SyncOrder::NO_SYNC : SyncOrder::A;
    SyncCtrlData syncCtrlData(order, frameGap);
    MSizeF targetIn(request->getEISInputSize().w, request->getEISInputSize().h);
    MRectF warpCrop = twoByteAlign(request->getEISCropRegion());
    WarpImg warpImg = WarpImg(displayWarp, targetIn, warpCrop, syncCtrlData);

    handleWarpResult(request, warpImg, ID_EIS_TO_WARP_P);
  }
  if (mPipeUsage.supportWarpNode(SFN_HINT::RECORD)) {
    TRACE_FUNC(
        "Frame Req:%d qReq:%d recordWarpMap:%p isFlush:%d pushOnly:%d "
        "preEnc:%d handleWarpResult",
        request->mRequestNo, qRequest->mRequestNo, recordWarp.get(), isFlush,
        qData.pushOnly, mPipeUsage.encodePreviewEIS());
    SyncCtrlData syncCtrlData(SyncOrder::B, frameGap);
    MSizeF targetIn(qData.mConfig.imgiW, qData.mConfig.imgiH);
    MRectF warpCrop = twoByteAlign(qData.mWarpCrop);
    WarpImg warpImg =
        WarpImg(mPipeUsage.encodePreviewEIS() ? displayWarp : recordWarp,
                targetIn, warpCrop, syncCtrlData);

    // ToDo : move has record output check to warp node
    if ((mPipeUsage.supportEIS_TSQ() && qData.pushOnly) ||
        !request->hasRecordOutput()) {
      warpImg.mBuffer = NULL;
    }
    MBOOL needHandle =
        mPipeUsage.supportEIS_TSQ() ? (!isFlush) : (!qData.pushOnly);
    if (needHandle) {
      handleWarpResult(qRequest, warpImg, ID_EIS_TO_WARP_R);
    }
  }
}

MRectF EISNode::twoByteAlign(const MRectF crop) {
  TRACE_FUNC_ENTER();
  MSize intValue;
  MRectF outCrop = crop;
  {}
  intValue.w = crop.s.w;
  intValue.h = crop.s.h;
  outCrop.s.w = (intValue.w & ~(0x1)) + (crop.s.w - intValue.w);
  outCrop.s.h = (intValue.h & ~(0x1)) + (crop.s.h - intValue.h);
  TRACE_FUNC_EXIT();
  return outCrop;
}

MBOOL EISNode::needFlushAll(const RequestPtr& request) {
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  if (mQueue.size()) {
    ret = !request->needEIS() ||
          (request->getEISQAction() == EISQ_ACTION_STOP) ||
          (request->getEISQAction() == EISQ_ACTION_NO);
  }
  TRACE_FUNC_EXIT();
  return ret;
}

MVOID EISNode::flushDummy() {
  TRACE_FUNC_ENTER();
  EISQData qData;
  qData = mQueue.front();
  mQueue.pop_front();
  if (mRefCount) {
    --mRefCount;
  }
  if (mPipeUsage.supportEIS_TSQ() || qData.mRequest == NULL) {
    MY_LOGW("pop request=%p tsq=%d mQueue.size:%zu", qData.mRequest.get(),
            mPipeUsage.supportEIS_TSQ(), mQueue.size());
  } else if (mPipeUsage.supportEIS_RQQ()) {
    MY_LOGW("RQQ flush dummy frame: %d pop mQueue.size:%zu", qData.getReqNo(),
            mQueue.size());
    if (mPipeUsage.supportWarpNode(SFN_HINT::RECORD)) {
      WarpImg dummyWarp;
      handleWarpResult(qData.mRequest, dummyWarp, ID_EIS_TO_WARP_R);
    }
  }
  TRACE_FUNC_EXIT();
}

MVOID EISNode::flushAllProcess(const RSCResult& rsc) {
  TRACE_FUNC_ENTER();
  while (mQueue.size()) {
    RequestPtr request = mQueue.front().mRequest;
    if (mPipeUsage.supportEIS_RQQ() &&
        EIS_MODE_IS_EIS_30_ENABLED(mEisInfo.mode) &&
        request->hasRecordOutput()) {
      MY_LOGD("flush process frame: %d ", request->mRequestNo);
      EIS_HAL_CONFIG_DATA config;
      processEIS30(request, config, rsc, MTRUE);
    } else {
      flushDummy();
    }
  }
  mFlushDone = MTRUE;
  MY_LOGI("mQueue is empty");
  TRACE_FUNC_EXIT();
}

}  // namespace NSFeaturePipe
}  // namespace NSCamFeature
}  // namespace NSCam

#endif  // SUPPORT_EIS
