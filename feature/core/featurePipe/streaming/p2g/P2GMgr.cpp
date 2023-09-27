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

#include "P2GMgr.h"

#include <thread>

#include "../DebugControl.h"
#define PIPE_CLASS_TAG "P2G_MGR"
#define PIPE_TRACE TRACE_P2G_MGR
#define INTERVAL_TIME 3
#include <featurePipe/core/include/PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

#include "../StreamingFeatureData.h"
#include "../StreamingFeature_Common.h"
#include "PrintWrapper.h"
#include <common/3dnr/3dnr_hal_base.h>
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <mtkcam3/feature/utils/p2/P2Util.h>

using namespace NSCam::NR3D;
using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam::Feature::P2Util;
using android::sp;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

P2GMgr::P2GMgr()
{
  TRACE_FUNC_ENTER();
  mNoFat = (property_get_int32(KEY_FORCE_P2G_NO_FAT, 0) == 1) ||
           (property_get_int32(KEY_USE_PER_FRAME_SETTING, 0) == 1);
  mNeedPrint = property_get_int32(KEY_FORCE_PRINT_P2G, 0);
  mForceThreadRelease = property_get_int32(KEY_FORCE_TH_RELEASE, 1);
  TRACE_FUNC_EXIT();
}

P2GMgr::~P2GMgr()
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
}

static MVOID count(const std::vector<LazyImgBufferPool> &pools, MUINT32 &level, MUINT32 &f0, MUINT32 &total)
{
    level = pools.size();
    f0 = level ? pools[0].getNeed() : 0;
    total = 0;
    for( const LazyImgBufferPool &p : pools )
    {
        total += p.getNeed();
    }
}

static MVOID printBufferPool(const DataPool &pool, MBOOL noFat)
{
    TRACE_FUNC_ENTER();
    MUINT32 ds0 = 0, dsLevel = 0, dsTotal = 0;
    MUINT32 dn0 = 0, dnLevel = 0, dnTotal = 0;
    MUINT32 pds0 = 0, pdsLevel = 0, pdsTotal = 0;
    MUINT32 wei0 = 0, weiLevel = 0, weiTotal = 0;
    MUINT32 dsw0 = 0, dswLevel = 0, dswTotal = 0;

    count(pool.mDsImgs,             ds0,    dsLevel,    dsTotal);
    count(pool.mDnImgs,             dn0,    dnLevel,    dnTotal);
    count(pool.mPDsImgs,            pds0,   pdsLevel,   pdsTotal);
    count(pool.mMsfWeightImgs,      wei0,   weiLevel,   weiTotal);
    count(pool.mMsfDsWeightImgs,    dsw0,   dswLevel,   dswTotal);

    MY_LOGI("BufferPool: fat[%d],tun[%d]sync[%d]dre_tun[%d]msftun[%d]msfSram[%d]omctun[%d]"
            "dce[%d]timg[%d]full[%d]conf[%d]idi[%d]omc[%d]"
            "ds[%dx%d=>%d]dn[%dx%d=>%d]pds[%dx%d=>%d]wei[%dx%d=>%d]dsw[%dx%d=>%d]",
        !noFat,
        pool.mTuning.getNeed(),
        pool.mSyncTuning.getNeed(),
        pool.mDreTuning.getNeed(),
        pool.mMsfTuning.getNeed(),
        pool.mMsfSram.getNeed(),
        pool.mOmcTuning.getNeed(),
        pool.mDceso.getNeed(),
        pool.mTimgo.getNeed(),
        pool.mFullImg.getNeed(),
        pool.mConf.getNeed(),
        pool.mIdi.getNeed(),
        pool.mOmc.getNeed(),
        ds0, dsLevel, dsTotal,
        dn0, dnLevel, dnTotal,
        pds0, pdsLevel, pdsTotal,
        wei0, pdsLevel, weiTotal,
        dsw0, dswLevel, dswTotal);
    TRACE_FUNC_EXIT();
}

MBOOL P2GMgr::init(const StreamingFeaturePipeUsage &usage)
{
  TRACE_FUNC_ENTER();
  MBOOL ret = MTRUE;

  mPipeUsage = usage;
  mSensorIndex = mPipeUsage.getSensorIndex();
  mTuningByteSize = NSIoPipe::NSPostProc::INormalStream::getRegTableSize();
  initSensorProvider();
  initHalISP();
  initTimgoHal();
  init3DNR();
  initDsdnHal();
  initSMVR();

  initPool();
  initBufferNum();
  initHwOptimize();
  printBufferPool(mPool, mNoFat);

  ret = mPathEngine.init(mPool, mFeatureSet, mPipeUsage.support4K2K(), mPipeUsage.support60FPS(),mPipeUsage.supportIMG3O()) &&
        mP2SWUtil.init(mSensorContextMap) &&
        mP2HWUtil.init(mPipeUsage.getSecureType(), mPipeUsage.getP2RBMask());
  TRACE_FUNC_EXIT();
  return ret;
}

MBOOL P2GMgr::uninit()
{
  TRACE_FUNC_ENTER();
  P2_CAM_TRACE_CALL(TRACE_ADVANCED);
  mP2HWUtil.uninit();
  mP2SWUtil.uninit();
  mPathEngine.uninit();
  uninitPool();
  uninitSMVR();
  uninitDsdnHal();
  uninit3DNR();
  uninitTimgoHal();
  uninitHalISP();
  uninitSensorProvider();
  TRACE_FUNC_EXIT();
  return MTRUE;
}

template <typename T>
MVOID allocate(const char *str, T &lazyPool)
{
    allocate(str, lazyPool.getPool(), lazyPool.getNeed());
}

MVOID P2GMgr::allocateP2GOutBuffer(MBOOL delay)
{
    if( !delay )
    {
        allocate("mFullImg", mPool.mFullImg);
        allocate("mSyncTuning", mPool.mSyncTuning);
        allocate("mDreTuning", mPool.mDreTuning);
        allocate("mOMCMV", mPool.mOmc);
    }
}

MVOID P2GMgr::allocateP2GInBuffer(MBOOL delay)
{
    if( !delay )
    {
        allocate("mConfImg", mPool.mConf);
        allocate("mIdiImg", mPool.mIdi);
    }
}

MVOID P2GMgr::allocateP2SWBuffer(MBOOL delay)
{
    if( !delay )
    {
        allocate("mTuning", mPool.mTuning);
        allocate("mMsfTuning", mPool.mMsfTuning);
        allocate("mMsfSram", mPool.mMsfSram);
        allocate("mOmcTuning", mPool.mOmcTuning);
    }
}

MVOID P2GMgr::allocateP2HWBuffer(MBOOL delay)
{
    if( !delay )
    {
        allocate("mTimgo", mPool.mTimgo);
        allocate("mDceso", mPool.mDceso);
        for( const LazyImgBufferPool &pool : mPool.mDnImgs )
        {
            allocate("dn", pool);
        }
        for( const LazyImgBufferPool &pool : mPool.mMsfWeightImgs )
        {
            allocate("msfWeight", pool);
        }
        for( const LazyImgBufferPool &pool : mPool.mMsfDsWeightImgs )
        {
            allocate("msfDsWeight", pool);
        }
    }
}

MVOID P2GMgr::allocateDsBuffer(MBOOL delay)
{
    if( !delay )
    {
        for( const LazyImgBufferPool &pool : mPool.mDsImgs )
        {
            allocate("ds", pool);
        }
    }
}

MVOID P2GMgr::allocatePDsBuffer(MBOOL delay)
{
    if( !delay )
    {
        for( const LazyImgBufferPool &pool : mPool.mPDsImgs )
        {
            allocate("pds", pool);
        }
    }
}

MBOOL P2GMgr::isOneP2G() const
{
    return !mPipeUsage.supportDSDN25() && !mPipeUsage.supportDSDN30();
}

MBOOL P2GMgr::needPrint() const
{
    return mNeedPrint;
}

MBOOL P2GMgr::isSupport(Feature::FID fid) const
{
    return mFeatureSet.has(fid);
}

MBOOL P2GMgr::isSupportDCE() const
{
    return isSupport(P2G::Feature::DCE);
}

MBOOL P2GMgr::isSupportDRETun() const
{
    return isSupport(P2G::Feature::DRE_TUN);
}

MBOOL P2GMgr::isSupportTIMGO() const
{
    return isSupport(P2G::Feature::TIMGO);
}

const char* P2GMgr::getTimgoTypeStr() const
{
    return mTimgoHal.getTypeStr();
}

P2F::ImageInfo P2GMgr::getFullImageInfo() const
{
    return mPool.mFullImg.getImageInfo();
}

TunBuffer P2GMgr::requestSyncTuningBuffer()
{
    LazyTunBufferPool::eMem mem = LazyTunBufferPool::MEMCLR;
    return mPool.mSyncTuning.requestBuffer(mem);
}

ImgBuffer P2GMgr::requestFullImgBuffer()
{
    return mPool.mFullImg.requestBuffer();
}

ImgBuffer P2GMgr::requestDsBuffer(unsigned index)
{
    ImgBuffer buffer;
    if( index < mPool.mDsImgs.size() )
    {
        buffer = mPool.mDsImgs[index].requestBuffer();
    }
    return buffer;
}

ImgBuffer P2GMgr::requestDnBuffer(unsigned index)
{
    ImgBuffer buffer;
    if( index < mPool.mDnImgs.size() )
    {
        buffer = mPool.mDnImgs[index].requestBuffer();
    }
    return buffer;
}

std::vector<IO> P2GMgr::replaceLoopIn(const std::vector<IO> &ioList)
{
    std::vector<IO> tempIO = ioList;
    for( IO &io : tempIO )
    {
        if( io.mLoopIn )
        {
            IO::LoopData loop = IO::makeLoopData();
            *loop = (*io.mLoopIn);
            MSize fullSize = io.mIn.mIMGISize;
            if( io.mFeature.hasNR3D() && !io.mFeature.hasDSDN20() )
            {
                loop->mIMG3O = GImg::make(mPool.mFullImg, fullSize);
                loop->mIMG3O->acquire();
                MY_LOGD("p2rb: img3o size: %dx%d", fullSize.w, fullSize.h);
            }
            if( io.mFeature.hasDSDN20() || io.mFeature.hasDSDN25() || io.mFeature.hasDSDN30() )
            {
                MUINT32 layer = loop->mSub.size();
                std::vector<MSize> dsSizes = io.mIn.mDSDNSizes;
                for( MUINT32 i = 0; i < layer; ++i )
                {
                    loop->mSub[i].mIMG3O = GImg::make(mPool.mDnImgs[i], dsSizes[i]);
                    loop->mSub[i].mIMG3O->acquire();
                    MY_LOGD("p2rb: dsdnSize[%d]:%dx%d", i, dsSizes[i].w, dsSizes[i].h);
                }
                if( io.mFeature.hasDSDN30() )
                {
                    io.mFeature -= Feature::DSDN30_INIT;
                    io.mFeature += Feature::DSDN30;
                }
            }
            io.mLoopIn = loop;
        }
    }
    return tempIO;
}

Path P2GMgr::calcPath(const std::vector<IO> &ioList, MUINT32 reqNo)
{
    MBOOL firstRB = (reqNo == 1 && (mPipeUsage.getP2RBMask() & P2RB_BIT_ENABLE));
    Path path = firstRB ? mPathEngine.run(replaceLoopIn(ioList))
                        : mPathEngine.run(ioList);

    if( needPrint() )
    {
        print(ioList);
        print(path);
    }
    return path;
}

P2GMgr::DIPParams P2GMgr::runP2HW(const std::vector<P2HW> &p2hw)
{
    return mP2HWUtil.run(mPool, p2hw, mP2HWOptimizeLevel);
}

void P2GMgr::runP2SW(const std::vector<P2SW> &p2sw, P2SW::TuningType type)
{
    return mP2SWUtil.run(mPool, p2sw, type);
}

MVOID P2GMgr::waitFrameDepDone(MUINT32 requestNo)
{
    TRACE_FUNC_ENTER();
    if( mDcesoHal.isSupport() )
    {
        mFrameControl.wait(requestNo, 2);
    }
    TRACE_FUNC_ENTER();
}

MVOID P2GMgr::notifyFrameDone(MUINT32 requestNo)
{
    TRACE_FUNC_ENTER();
    mFrameControl.notify(requestNo);
    TRACE_FUNC_EXIT();
}

IO::LoopData P2GMgr::makeInitLoopData(const Feature &feature)
{
    TRACE_FUNC_ENTER();
    IO::LoopData loop = IO::makeLoopData();
    if( feature.has(Feature::NR3D) )
    {
        loop->mNR3DStat = std::make_shared<NR3DStat>();
        if( feature.hasDSDN25() || feature.hasDSDN20() || feature.hasDSDN30() )
        {
            MUINT32 total = mDsdnHal.getMaxDSLayer();
            loop->mSub.resize(total);
            for( MUINT32 i = 0; i < total; ++i )
            {
                loop->mSub[i].mNR3DStat = std::make_shared<NR3DStat>();
            }
        }
    }
    TRACE_FUNC_EXIT();
    return loop;
}

POD P2GMgr::makePOD(const RequestPtr &request, MUINT32 sensorID, Scene scene)
{
    TRACE_FUNC_ENTER();
    POD data;

    data.mSensorIndex = sensorID;
    data.mP2Tag = NSIoPipe::NSPostProc::ENormalStreamTag_Normal;
    data.mP2DebugIndex = NSIoPipe::P2_RUN_S_P2A;
    data.mP2Pack = request->mP2Pack.getP2Pack(sensorID);
    data.mBasicPQ = request->getBasicPQ(sensorID);
    data.mRegDump = request->needRegDump();
    data.mIsMaster = P2G::isMaster(scene);
    data.mFDSensorID = request->mFDSensorID;
    data.mResized = MTRUE;
    data.mIsYuvIn = MFALSE;
    data.mTimgoType = mTimgoHal.getType();

    data.mFDCrop.mIsValid = request->getSensorVarMap(sensorID).tryGet<MRect>(SFP_VAR::FD_CROP_ACTIVE_REGION, data.mFDCrop.mRect);
    data.mSrcCrop = request->getSrcCropInfo(sensorID).mSrcCrop;

    TRACE_FUNC_EXIT();
    return data;
}

ILazy<NR3DMotion> P2GMgr::makeMotion() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return makeLazy<NR3DMotion>();
}

Hal3dnrBase* P2GMgr::get3dnrHalInstance(MUINT sensorID)
{
    TRACE_FUNC_ENTER();

    MBOOL ret = MTRUE;
    if ( sensorID >= 0 && sensorID < HAL_3DNR_INST_MAX_NUM)
    {
        if (mp3dnr[sensorID] == nullptr)
        {
            mp3dnr[sensorID] = Hal3dnrBase::createInstance(PIPE_CLASS_TAG, sensorID);
            if (mp3dnr[sensorID] != nullptr)
            {
                mp3dnr[sensorID]->init(PIPE_CLASS_TAG);
            }
            else
            {
                MY_LOGE("!!err: 3dnr_hal instance fails to be created");
                ret = MFALSE;
            }
        }
    }
    else
    {
        MY_LOGE("!!err: sensorID(%d) is wrong", sensorID);
        ret = MFALSE;
    }

    TRACE_FUNC_EXIT();
    return (ret ? mp3dnr[sensorID] : nullptr);
}

MBOOL P2GMgr::runUpdateMotion(const RequestPtr &request, const P2SWHolder &sw, const RSCResult &rsc, MBOOL bSlave)
{
    (void)rsc;
    TRACE_FUNC_ENTER();

    MBOOL ret = MTRUE;
    if ( request == NULL || sw.mMotion == NULL )
    {
        MY_LOGW("3dnr: param invalid: request(%p), sw.Motion(%p)",
            request.get(), sw.mMotion.get());
        TRACE_FUNC_EXIT();
        return MFALSE;
    }

    MUINT32 sensorID = bSlave ? request->mSlaveID : request->mMasterID;
    Hal3dnrBase *p3dnr = get3dnrHalInstance(sensorID);
    if ( p3dnr == NULL )
    {
        MY_LOGE("3dnr: 3dnrHal instance can't be null: sensorID(%d)", sensorID);
        TRACE_FUNC_EXIT();
        return MFALSE;
    }
    NR3DHALParam nr3dHalParam;
    // fill in tunning param3
    const P2SensorData &sData = request->mP2Pack.getSensorData(sensorID);
    nr3dHalParam.mNvramInfo.mMS5_RSV1 = sData.mNvramMsyuv_3DNR.mMSYUV_MS5_RSV1;
    nr3dHalParam.mNvramInfo.mMS5_RSV2 = sData.mNvramMsyuv_3DNR.mMSYUV_MS5_RSV2;
    nr3dHalParam.mNvramInfo.mMS5_RSV3 = sData.mNvramMsyuv_3DNR.mMSYUV_MS5_RSV3;
    nr3dHalParam.mNvramInfo.mMS5_RSV4 = sData.mNvramMsyuv_3DNR.mMSYUV_MS5_RSV4;
    // uniqueKey, requestNo, frameNo
    TuningUtils::FILE_DUMP_NAMING_HINT hint = sData.mNDDHint;
    nr3dHalParam.uniqueKey = hint.UniqueKey;
    nr3dHalParam.requestNo = hint.RequestNo;
    nr3dHalParam.frameNo = hint.FrameNo;

    if(mPipeUsage.supportDSDN30() && mPipeUsage.isGyroEnable() && mpSensorProvider != NULL)
    {
        // Fill the gyro data in
        if(!(mpSensorProvider->getGyroMV(sData.FrameStartTime, intervalInMs, mGyroMVResult)))
        {
            MY_LOGE("GyroMVResult query fail:ts(%" PRIu64 "),mvs(%d),gyroNum(%d),height(%d),width(%d) ", mGyroMVResult.frameTs, mGyroMVResult.mvSize, mGyroMVResult.gyroNumSize,mGyroMVResult.mvHeight,mGyroMVResult.mvWidth);
        }
    }
    nr3dHalParam.gyroData = NR3D::GyroData();
    nr3dHalParam.gyroData.gyroInfo.gyro_in_mv = mGyroMVResult.mv;
    nr3dHalParam.gyroData.gyroInfo.gyroMvWidth = mGyroMVResult.mvWidth;
    nr3dHalParam.gyroData.gyroInfo.gyroMvHeight = mGyroMVResult.mvHeight;
    // mktodo: feastMaks should be common
    nr3dHalParam.featMask = NR3D_FEAT_MASK_BASIC_DSDN25;

    // lmv related info
    NR3D::NR3DMVInfo defaultMvInfo;
    nr3dHalParam.gmvInfo = request->getVar<NR3D::NR3DMVInfo>(
        SFP_VAR::NR3D_MV_INFO, defaultMvInfo);

    // RSC related
    const SrcCropInfo srcCropInfo = request->getSrcCropInfo(sensorID);
    if (rsc.mIsValid)
    {
        ImgBuffer pV = rsc.mMV;
        if (pV.get() && pV->getImageBufferPtr())
            nr3dHalParam.rscInfo.pMV = pV->getImageBufferPtr()->getBufVA(0);
        pV = rsc.mBV;
        if (pV.get() && pV->getImageBufferPtr())
            nr3dHalParam.rscInfo.pBV = pV->getImageBufferPtr()->getBufVA(0);
        nr3dHalParam.rscInfo.rrzoSize = srcCropInfo.mRRZOSize;
        nr3dHalParam.rscInfo.rssoSize = rsc.mRssoSize;
        nr3dHalParam.rscInfo.staGMV = rsc.mRscSta.value;
        nr3dHalParam.rscInfo.isValid = rsc.mIsValid;
    }

    // prepare srcCropSizes / dstSizes
    MSize srcCropSizes[NR3D_LAYER_MAX_NUM];
    MSize dstSizes[NR3D_LAYER_MAX_NUM];
    const std::vector<MSize>& p2Sizes = bSlave ? sw.mSlaveP2Sizes : sw.mP2Sizes;
    for (int k = 0; k < p2Sizes.size(); ++k)
    {
        // index=0 is always the max size
        srcCropSizes[k] = dstSizes[k]= p2Sizes[k];
    }

    P2_CAM_TRACE_CALL(TRACE_DEFAULT);

    NR3DMotion *pNr3dMotion = bSlave ? sw.mSlaveMotion.get() : sw.mMotion.get();
    if (pNr3dMotion == nullptr )
    {
        MY_LOGW("3dnr: motion is NULL: isSlave(%d), masterMotion(%p), slaveMotion(%p)",
            bSlave, sw.mMotion.get(), sw.mSlaveMotion.get() );
        return MFALSE;
    }

    pNr3dMotion->isGmvInfoValid = MFALSE;
    IImageBuffer *omc = peak(bSlave ? sw.mSlaveOMCMV : sw.mOMCMV);
    ret = p3dnr->update3DNRMvInfo(
        nr3dHalParam,
        nullptr,
        NR3D_FEAT_MASK_BASIC_DSDN25,
        p2Sizes.size(),
        srcCropSizes,
        dstSizes,
        nr3dHalParam.uniqueKey,
        nr3dHalParam.requestNo,
        nr3dHalParam.frameNo,
        nr3dHalParam.gmvInfo,
        nr3dHalParam.rscInfo,
        pNr3dMotion->gmvInfoResults,
        pNr3dMotion->isGmvInfoValid,
        omc);

    if (ret == MTRUE)
    {
        // mktodo: used for P2NR (DSDN20)?
        request->setVar<NR3D::NR3DMVInfo>(SFP_VAR::NR3D_MV_INFO, pNr3dMotion->gmvInfoResults[0]);
    }

    ILazy<DSDNInfo> dsdnInfo = bSlave ? sw.mSlaveDSDNInfo : sw.mDSDNInfo;
    if( omc && dsdnInfo && !dsdnInfo->mLoopValid )
    {
        initBuffer(omc);
    }

    TRACE_FUNC_EXIT();
    return ret;
}

ILazy<DSDNInfo> P2GMgr::makeDSDNInfo() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return makeLazy<DSDNInfo>();
}

P2GMgr::DsdnQueryOut P2GMgr::queryDSDNInfos(MUINT32 sensorID, const RequestPtr &request, const MSize &next, MBOOL lastRun)
{
    TRACE_FUNC_ENTER();
    P2GMgr::DsdnQueryOut out;

    MBOOL dsdn25 = request->needDSDN25(sensorID);
    MBOOL dsdn30 = request->needDSDN30(sensorID);
    MBOOL dsdn20 = request->needDSDN20(sensorID);
    MSize inSize = request->getSrcCropInfo(sensorID).mSrcCrop.s;
    if( dsdn25 || dsdn30 || dsdn20 )
    {
        MSize fdSize = request->getFDSize();
        MBOOL loopValid = MTRUE;
        const P2SensorData &sData = request->mP2Pack.getSensorData(sensorID);
        DSDNRatio ratio(sData.mNvramDsdn.mRatioMultiple, sData.mNvramDsdn.mRatioDivider);
        out.dsSizes = mDsdnHal.getDSSizes(inSize, next, fdSize, ratio);
        if( dsdn30 )
        {
            out.omc = P2G::makeGImg(mPool.mOmc.requestBuffer(), mDsdnHal.getSize(out.dsSizes, DsdnHal::OMCMV));
            out.confSize = mDsdnHal.getSize(out.dsSizes, DsdnHal::CONF);
            out.idiSize = mDsdnHal.getSize(out.dsSizes, DsdnHal::IDI);
            EnqueInfo::Data lastCache = request->getLastEnqData(sensorID);
            EnqueInfo::Data curCache = request->getCurEnqData(sensorID);
            loopValid = mDsdnHal.isLoopValid(curCache.mRRZSize, curCache.mTime, MTRUE, lastCache.mRRZSize, lastCache.mTime, (HAS_DSDN30(lastCache.mFeatureMask) && lastRun));
        }
        out.outInfo = makeDSDNInfo();

        if( out.outInfo )
        {
            out.outInfo->mProfiles = mDsdnHal.getProfiles(sData.mHDRHalMode);
            out.outInfo->mLoopValid = loopValid;
            out.outInfo->mVer = mDsdnHal.getVersion();
        }
        else
        {
            MY_LOGE("sID(%d) DSDN Info is NULL!, not updated profile & loopValid !!!", sensorID);
        }
    }

    MUINT32 p2 = std::max<MUINT32>(1, (dsdn25 || dsdn30) ? out.dsSizes.size() :
                                      dsdn20 ? out.dsSizes.size()+1 :
                                      1);
    out.p2Sizes.reserve(p2);
    out.p2Sizes.push_back(inSize);
    for( MUINT32 i = 0; i < (p2-1); ++i )
    {
        out.p2Sizes.push_back(out.dsSizes[i]);
    }
    TRACE_FUNC_EXIT();
    return out;
}

SMVRHal P2GMgr::getSMVRHal() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return mSMVRHal;
}

MVOID P2GMgr::initHalISP()
{
    TRACE_FUNC_ENTER();
    SensorContextPtr ctx;
    mSensorContextMap = std::make_shared<SensorContextMap>();
    mSensorContextMap->init(mPipeUsage.getAllSensorIDs());
    ctx = mSensorContextMap->find(mSensorIndex);

    if( mIspMgr == NULL && SUPPORT_ISP_HAL )
    {
        mIspMgr = MAKE_IspMgr();
    }
    if( ctx && ctx->mHalISP )
    {
        mHasISPBufferInfo = ctx->mHalISP->queryISPBufferInfo(mISPBufferInfo);
    }
    if( mHasISPBufferInfo )
    {
        mSyncTuningByteSize = mISPBufferInfo.u4DualSyncInfoSize;
    }
    else if( mIspMgr )
    {
        mSyncTuningByteSize = mIspMgr->queryDualSyncInfoSize();
    }

    initDcesoHal();
    initDreHal();

    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitHalISP()
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    uninitDreHal();
    uninitDcesoHal();

    if( mIspMgr )
    {
        //mISPMgr->destroyInstance();
        mIspMgr = NULL;
    }
    if( mSensorContextMap )
    {
        mSensorContextMap->uninit();
        mSensorContextMap= NULL;
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::allocateGyroBuffer()
{
    if(mpSensorProvider != NULL)
    {
        mpSensorProvider->queryGyroMVInfo(mGyroMVResult.mvSize, mGyroMVResult.gyroNumSize);
    }
    MBOOL isInValid = (mGyroMVResult.mvSize == 0) || (mGyroMVResult.gyroNumSize == 0);
    if (!isInValid)
    {
        mGyroMVResult.mv = (MUINT8*)malloc(mGyroMVResult.mvSize);
        mGyroMVResult.valid_gyro_num = (MUINT8*)malloc(mGyroMVResult.gyroNumSize);
    }else
    {
        MY_LOGE("mGyroMVResult.mvSize(%d) .gyroNum(%d)", mGyroMVResult.mvSize,mGyroMVResult.gyroNumSize);
    }
}

MVOID P2GMgr::initSensorProvider()
{
    TRACE_FUNC_ENTER();
    if(mPipeUsage.supportDSDN30() && mPipeUsage.isGyroEnable())
    {
        MY_LOGD("Create Sensorprovider");
        mpSensorProvider = NULL;
        intervalInMs = property_get_int32("vendor.debug.fpipe.p2gmgr.intervalms", INTERVAL_TIME);
        mpSensorProvider = SensorProvider::createInstance(PIPE_CLASS_TAG);
        if(mpSensorProvider != NULL)
        {
            MBOOL mGyroEnable = mpSensorProvider->enableSensor(SENSOR_TYPE_GYRO, intervalInMs);
            MY_LOGD("Gyro Enable");
        }
        else
        {
            MY_LOGE("Fail to create sensorProvider");
        }
        allocateGyroBuffer();
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitSensorProvider()
{
    if (mpSensorProvider != NULL)
    {
        P2_CAM_TRACE_CALL(TRACE_ADVANCED);
        freeGyroBuffer();
        MY_LOGD("mpSensorProvider uninit");
        mpSensorProvider->disableSensor(SENSOR_TYPE_GYRO);
        mpSensorProvider = NULL;
    }
}

MVOID P2GMgr::freeGyroBuffer()
{
    if (mGyroMVResult.mv != NULL)
    {
        MY_LOGD("Free gyro data");
        free(mGyroMVResult.mv);
        free(mGyroMVResult.valid_gyro_num);
    }
}

MVOID P2GMgr::initDcesoHal()
{
    TRACE_FUNC_ENTER();
    mDcesoHal.init(mHasISPBufferInfo, mISPBufferInfo);
    if( mDcesoHal.isSupport() )
    {
        mFeatureSet.add(Feature::DCE);
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitDcesoHal()
{
    TRACE_FUNC_ENTER();
    mDcesoHal.uninit();
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initDreHal()
{
    TRACE_FUNC_ENTER();
    mDreHal.init(mHasISPBufferInfo, mISPBufferInfo);
    if( mDreHal.isSupport() )
    {
        mDreTuningByteSize = mDreHal.getTuningByteSize();
        mFeatureSet.add(Feature::DRE_TUN);
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitDreHal()
{
    TRACE_FUNC_ENTER();
    mDreHal.uninit();
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initTimgoHal()
{
    TRACE_FUNC_ENTER();
    mTimgoHal.init(mPipeUsage.getStreamingSize());
    if( mTimgoHal.isSupport() )
    {
        mFeatureSet.add(Feature::TIMGO);
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::init3DNR()
{
    TRACE_FUNC_ENTER();
    if( mPipeUsage.support3DNR() )
    {
        mFeatureSet.add(Feature::NR3D);
    }
    // init 3dnr hal instances
    for (int k = 0; k <  HAL_3DNR_INST_MAX_NUM; ++k)
    {
        mp3dnr[k] = nullptr;
    }

    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninit3DNR()
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
     // releaes 3dnr hal instances
    for (int k = 0; k <  HAL_3DNR_INST_MAX_NUM; ++k)
    {
        if (mp3dnr[k] != nullptr)
        {
           mp3dnr[k]->uninit(PIPE_CLASS_TAG);
           mp3dnr[k]->destroyInstance(PIPE_CLASS_TAG, k);
           mp3dnr[k] = nullptr;
        }
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitTimgoHal()
{
    TRACE_FUNC_ENTER();
    mTimgoHal.uninit();
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initDsdnHal()
{
    TRACE_FUNC_ENTER();
    mDsdnHal.init(mPipeUsage.getDSDNParam(), mPipeUsage.getStreamingSize(), mPipeUsage.getFDSize());
    if( mDsdnHal.isSupportDSDN25() ) mFeatureSet.add(Feature::DSDN25);
    if( mDsdnHal.isSupportDSDN20() ) mFeatureSet.add(Feature::DSDN20);
    if( mDsdnHal.isSupportDSDN30() )
    {
        mFeatureSet.add(Feature::DSDN30);
        mFeatureSet.add(Feature::DSDN30_INIT);
    }

    if( mDsdnHal.isSupportDSDN30() )
    {
        mMsfTuningByteSize = mISPBufferInfo.MSF_Tuning_Buffer_Size;
        mMsfSramByteSize = mISPBufferInfo.MSF_Sram_Tbl_Size;
        mOmcTuningByteSize = mISPBufferInfo.OMC_Tuning_Buffer_Size;

        if( !mHasISPBufferInfo || !mMsfTuningByteSize || !mMsfSramByteSize || !mOmcTuningByteSize)
        {
            MY_LOGF("DSDN 3.0 tuningBuf size error !! hasBufInfo(%d) msfTun(%d), msfSram(%d), omcTun(%d)", mHasISPBufferInfo, mMsfTuningByteSize, mMsfSramByteSize, mOmcTuningByteSize);
        }
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initDsdnPools()
{
    TRACE_FUNC_ENTER();
    MUINT32 size = mMsfTuningByteSize ? mMsfTuningByteSize : 256;
    mPool.mMsfTuning = TuningBufferPool::create("fpipe.p2g.msftun", size, TuningBufferPool::BUF_PROTECT_RUN);
    size = mMsfSramByteSize ? mMsfSramByteSize : 256;
    mPool.mMsfSram = TuningBufferPool::create("fpipe.p2g.msfsram", size, TuningBufferPool::BUF_PROTECT_RUN);
    size = mOmcTuningByteSize ? mOmcTuningByteSize : 256;
    mPool.mOmcTuning = TuningBufferPool::create("fpipe.p2g.omctun", size, TuningBufferPool::BUF_PROTECT_RUN);

    std::vector<MSize> dsSize = mDsdnHal.getMaxDSSizes();
    mPool.mConf = createConfPool("fpipe.p2g.conf", dsSize);
    mPool.mIdi = createIdiPool("fpipe.p2g.idi", dsSize);
    mPool.mOmc = createOmcPool("fpipe.p2g.omc", dsSize);

    struct multiTable
    {
        MBOOL                           need;
        const char                      *name;
        const DsdnHal::eDSDNImg         img;
        std::vector<LazyImgBufferPool>  &out;
    } myMultiTable[] =
    {
        { mPipeUsage.supportDSDN(),     "fpipe.p2g.dso",        DsdnHal::DS,            mPool.mDsImgs},
        { mPipeUsage.supportDSDN(),     "fpipe.p2g.dno",        DsdnHal::DN,            mPool.mDnImgs},
        { mPipeUsage.supportDSDN30(),   "fpipe.p2g.pdso",       DsdnHal::PDS,           mPool.mPDsImgs},
        { mPipeUsage.supportDSDN30(),   "fpipe.p2g.weight",     DsdnHal::MSF_WEIGHT,    mPool.mMsfWeightImgs},
        { mPipeUsage.supportDSDN30(),   "fpipe.p2g.dsweight",   DsdnHal::DS_WEIGHT,     mPool.mMsfDsWeightImgs},
    };
    for( const multiTable &t : myMultiTable )
    {
        if( t.need )
        {
            t.out = createMultiPools(t.name, dsSize, mDsdnHal.getFormat(t.img));
        }
    }

    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitDsdnPools(DataPool& pool)
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    pool.mMsfTuning.destroy();
    pool.mMsfSram.destroy();
    pool.mOmcTuning.destroy();
    pool.mConf.destroy();
    pool.mIdi.destroy();
    pool.mOmc.destroy();

    auto destroy = [](std::vector<LazyImgBufferPool> &pools) {
        TRACE_FUNC_ENTER();
        for( LazyImgBufferPool &pool : pools )
        {
            pool.destroy();
        }
        pools.clear();
        TRACE_FUNC_EXIT();
    };

    destroy(pool.mDsImgs);
    destroy(pool.mDnImgs);
    destroy(pool.mPDsImgs);
    destroy(pool.mMsfWeightImgs);
    destroy(pool.mMsfDsWeightImgs);
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitDsdnHal()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initSMVR()
{
    TRACE_FUNC_ENTER();
    mSMVRHal.init(mPipeUsage);
    if( mSMVRHal.isSupport() )
    {
        mFeatureSet.add(Feature::SMVR_MAIN);
        mFeatureSet.add(Feature::SMVR_SUB);
        if( mFeatureSet.has(Feature::DSDN25) )
        {
            MUINT32 noOptimize = property_get_int32("vendor.debug.fpipe.smvr.no_optimize", 0);
            mP2HWOptimizeLevel = noOptimize ? 0 : mDsdnHal.getMaxDSLayer();
        }
    }
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitSMVR()
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    mSMVRHal.uninit();
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initPool()
{
    TRACE_FUNC_ENTER();
    mPool.mTuning = createTuningPool("fpipe.p2g.tuning");
    mPool.mSyncTuning = createSyncTuningPool("fpipe.p2g.syncTuning");
    mPool.mDreTuning = createDreTuningPool("fpipe.p2g.dreTuning");
    mPool.mFullImg = createFullImgPool("fpipe.p2g.fullImg");
    mPool.mDceso = createDcesoPool("fpipe.p2g.dceso");
    mPool.mTimgo = createTimgoPool("fpipe.p2g.timgo");
    initDsdnPools();
    TRACE_FUNC_EXIT();
}

static MVOID setPoolNeed(std::vector<LazyImgBufferPool> &pools, MUINT32 num)
{
    for( LazyImgBufferPool &pool : pools )
    {
        pool.setNeed(num);
    }
}

MVOID P2GMgr::initBufferNum()
{
    TRACE_FUNC_ENTER();
    MBOOL isDual = mPipeUsage.getNumSensor() >= 2;
    MBOOL isDsdn20 = mFeatureSet.hasDSDN20();
    MBOOL isDsdn25 = mFeatureSet.hasDSDN25();
    MBOOL isDsdn30 = mFeatureSet.hasDSDN30();
    MBOOL isNr3d = mFeatureSet.hasNR3D();
    MBOOL isDre = mFeatureSet.hasDRE_TUN();
    MUINT32 numScene = mPipeUsage.getNumScene();
    MUINT32 dce = mDcesoHal.getDelayCount();
    MUINT32 p2 = std::max<MUINT32>(1, mDsdnHal.getMaxDSLayer());
    MUINT32 ext = mPipeUsage.getNumP2GExtImg();
    MUINT32 extPQ = mPipeUsage.getNumP2GExtPQ();
    MUINT32 numTun = 0, numDCE = 0, numDRE = 0, numMsfTun = 0, numMsfSrm = 0, numOmcTun = 0;
    MUINT32 numFull = 0, numDS = 0, numDN = 0, numPDs = 0, numWei = 0, numDsWei = 0;
    MUINT32 numConf = 0, numIdi = 0, numOmc = 0;
    MUINT32 nrBufRatio = mPipeUsage.supportSlaveNR() ? 2 : 1;

    if( mFeatureSet.has(Feature::SMVR_MAIN) )
    {
        MUINT32 smvr = mSMVRHal.getP2Speed();
        MUINT32 smvrQ = mSMVRHal.getRecover();
        numTun = (smvr + smvrQ + 3) * p2;
        numDCE = 2*smvr + dce;
        numDRE = isDre ? (smvrQ ? 4 + smvrQ + smvr : 4) : 0;
        numDS = isDsdn25 ? 3 * smvr + 3 : 0;
        numDN = isDsdn25 ? 2 * smvr + 4 : 0;
        numFull = (isNr3d || isDsdn25) ? 2 * smvr + 4 : 3;
    }
    else
    {
        numTun = 3 * (numScene + p2) + extPQ;
        numDCE = (2 + dce) * numScene;
        numDRE = isDre ? (4 + extPQ) * nrBufRatio : 0;
        numDS = (isDsdn20 || isDsdn25 || isDsdn30) ? 3*nrBufRatio : 0;
        numDN = (isNr3d || isDsdn20 || isDsdn25 || isDsdn30) ? 3*nrBufRatio : 0;
        numFull = (isNr3d || isDsdn20 || isDsdn25 || isDsdn30) ? ext + 3*nrBufRatio : ext;
        mPool.mSyncTuning.setNeed(isDual ? numScene : 0);
        mPool.mTimgo.setNeed(3);
    }

    if( isDsdn30 )
    {
        numMsfTun = numMsfSrm = 3 * (numScene + p2);
        numPDs = numDS;
        numWei = numDsWei = numDN;
        numConf = numIdi = numOmc = numOmcTun = numDS;

        setPoolNeed(mPool.mPDsImgs, numPDs);
        setPoolNeed(mPool.mMsfWeightImgs, numWei);
        setPoolNeed(mPool.mMsfDsWeightImgs, numDsWei);
        mPool.mMsfTuning.setNeed(numMsfTun);
        mPool.mMsfSram.setNeed(numMsfSrm);
        mPool.mOmcTuning.setNeed(numOmcTun);
        mPool.mConf.setNeed(numConf);
        mPool.mIdi.setNeed(numIdi);
        mPool.mOmc.setNeed(numOmc);
    }

    mPool.mTuning.setNeed(numTun);
    mPool.mDceso.setNeed(numDCE);
    mPool.mDreTuning.setNeed(numDRE);
    mPool.mFullImg.setNeed(numFull);
    setPoolNeed(mPool.mDsImgs, numDS);
    setPoolNeed(mPool.mDnImgs, numDN);
    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::uninitPool()
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);

#if defined(SUPPORT_ASYNC_CLOSING)
    std::thread releaseThread;

    if (mForceThreadRelease) {
        releaseThread = std::thread(uninitDsdnPools, std::ref(mPool));
    }
#endif

    mPool.mTuning.destroy();
    mPool.mSyncTuning.destroy();
    mPool.mDreTuning.destroy();
    mPool.mFullImg.destroy();
    mPool.mDceso.destroy();
    mPool.mTimgo.destroy();

#if defined(SUPPORT_ASYNC_CLOSING)
    if (mForceThreadRelease) {
        releaseThread.join();
    } else {
        uninitDsdnPools(mPool);
    }
#else
    uninitDsdnPools(mPool);
#endif

    TRACE_FUNC_EXIT();
}

MVOID P2GMgr::initHwOptimize()
{
    TRACE_FUNC_ENTER();
    MBOOL smvrDsdn = mFeatureSet.has(Feature::SMVR_MAIN) && mFeatureSet.has(Feature::DSDN25);
    MBOOL multiDsdn30 = mFeatureSet.has(Feature::DSDN30) && mPipeUsage.supportSlaveNR();
    if( smvrDsdn || multiDsdn30 )
    {
        MUINT32 noOptimize = property_get_int32("vendor.debug.fpipe.smvr.no_optimize", 0);
        mP2HWOptimizeLevel = noOptimize ? 0 : mDsdnHal.getMaxDSLayer();
    }
    TRACE_FUNC_EXIT();
}


sp<TuningBufferPool> P2GMgr::createTuningPool(const char *name)
{
    TRACE_FUNC_ENTER();
    sp<TuningBufferPool> pool;
    MUINT32 size = std::max<MUINT32>(mTuningByteSize, 256);
    pool = TuningBufferPool::create(name, size, TuningBufferPool::BUF_PROTECT_RUN);
    return pool;
}

sp<TuningBufferPool> P2GMgr::createSyncTuningPool(const char *name)
{
    TRACE_FUNC_ENTER();
    sp<TuningBufferPool> pool;
    MUINT32 size = std::max<MUINT32>(mSyncTuningByteSize, 256);
    pool = TuningBufferPool::create(name, size, TuningBufferPool::BUF_PROTECT_RUN);
    TRACE_FUNC_EXIT();
    return pool;
}

sp<TuningBufferPool> P2GMgr::createDreTuningPool(const char *name)
{
    TRACE_FUNC_ENTER();
    sp<TuningBufferPool> pool;
    MUINT32 size = mDreTuningByteSize ? mDreTuningByteSize : 256;
    pool = TuningBufferPool::create(name, size, TuningBufferPool::BUF_PROTECT_RUN);
    return pool;
}

bool isImg3oFmt(const EImageFormat &fmt)
{
    bool ret = false;
    switch(fmt)
    {
        case eImgFmt_YUY2:
        case eImgFmt_NV21:
        case eImgFmt_YV12:
        case eImgFmt_YUYV_Y210:
        case eImgFmt_YVYU_Y210:
        case eImgFmt_UYVY_Y210:
        case eImgFmt_VYUY_Y210:
        case eImgFmt_MTK_YUYV_Y210:
        case eImgFmt_MTK_YVYU_Y210:
        case eImgFmt_MTK_UYVY_Y210:
        case eImgFmt_MTK_VYUY_Y210:
        case eImgFmt_YUV_P010:
        case eImgFmt_YVU_P010:
        case eImgFmt_MTK_YUV_P010:
        case eImgFmt_MTK_YVU_P010:
        case eImgFmt_UFBC_NV12:
        case eImgFmt_UFBC_NV21:
        case eImgFmt_UFBC_MTK_YUV_P010:
        case eImgFmt_UFBC_MTK_YVU_P010:
            ret = true;
            break;
        default:
            ret = false;
    }
    return ret;
}

EImageFormat decideFullFormat(const EImageFormat &oriFmt)
{
    EImageFormat format = oriFmt;
    char propFmtStr[PROPERTY_VALUE_MAX] = "";
    property_get("vendor.debug.fpipe.img3o.imgformat", propFmtStr, "");
    if(propFmtStr[0])
    {
        EImageFormat propFmt = NSCam::NSCamFeature::NSFeaturePipe::toEImageFormat(propFmtStr);
        if(isImg3oFmt(propFmt))
        {
            MY_LOGI("Property set img3o format from 0x%x(%s) to 0x%x(%s)", oriFmt, NSCam::NSCamFeature::NSFeaturePipe::toName(oriFmt), propFmt, propFmtStr);
            format = propFmt;
        }
        else
        {
            MY_LOGW("unsupported img3o format 0x%x(%s), supported format: NV21/UFBC_NV21/MTK_YUV_P010/MTK_YUYV_Y210/...", propFmt, propFmtStr);
        }
    }

    return format;
}

sp<IBufferPool> P2GMgr::createFullImgPool(const char *name)
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;

    EImageFormat format = mPipeUsage.getFullImgFormat();
    format = decideFullFormat(format);

    MSize size = align(mPipeUsage.getStreamingSize(), BUF_ALLOC_ALIGNMENT_BIT);

    if ( mPipeUsage.isSecureP2() )
    {
        pool =  SecureBufferPool::create(name, size, format, ImageBufferPool::USAGE_HW);
    }
    else if( mPipeUsage.supportGraphicBuffer() && isGrallocFormat(format) )
    {
        NativeBufferWrapper::ColorSpace color = NativeBufferWrapper::NOT_SET;
        if( mPipeUsage.supportEISNode() )
        {
            color = NativeBufferWrapper::YUV_BT601_FULL;
        }
        pool = GraphicBufferPool::create(name, size, format, GraphicBufferPool::USAGE_HW_TEXTURE, color);
    }
    else
    {
        pool = ImageBufferPool::create(name, size, format, ImageBufferPool::USAGE_HW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

sp<IBufferPool> P2GMgr::createDcesoPool(const char *name)
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;
    if( mDcesoHal.isSupport() )
    {
        MSize size = mDcesoHal.getBufferSize();
        pool = ImageBufferPool::create(name, size, mDcesoHal.getBufferFmt(), ImageBufferPool::USAGE_HW_AND_SW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

sp<IBufferPool> P2GMgr::createTimgoPool(const char *name)
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;
    if( mTimgoHal.isSupport() )
    {
        EImageFormat fmt = mTimgoHal.getBufferFmt();
        MSize size = mTimgoHal.getBufferSize();
        pool = ImageBufferPool::create(name, size, fmt, ImageBufferPool::USAGE_HW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

sp<IBufferPool> P2GMgr::createConfPool(const char *name, const std::vector<MSize> &dsSizes)
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;
    MSize size = mDsdnHal.getSize(dsSizes, DsdnHal::CONF);
    if( mPipeUsage.supportDSDN30() && isValid(size))
    {
        pool = ImageBufferPool::create(name, size, mDsdnHal.getFormat(DsdnHal::CONF), ImageBufferPool::USAGE_HW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

sp<IBufferPool> P2GMgr::createIdiPool(const char *name, const std::vector<MSize> &dsSizes)
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;
    MSize size = mDsdnHal.getSize(dsSizes, DsdnHal::IDI);
    if( mPipeUsage.supportDSDN30() && isValid(size))
    {
        pool = ImageBufferPool::create(name, size, mDsdnHal.getFormat(DsdnHal::IDI), ImageBufferPool::USAGE_HW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

sp<IBufferPool> P2GMgr::createOmcPool(const char *name, const std::vector<MSize> &dsSizes)
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;
    MSize size = mDsdnHal.getSize(dsSizes, DsdnHal::OMCMV);
    if( mPipeUsage.supportDSDN30() && isValid(size))
    {
        pool = ImageBufferPool::create(name, align(size, BUF_ALLOC_ALIGNMENT_BIT), mDsdnHal.getFormat(DsdnHal::OMCMV), ImageBufferPool::USAGE_HW_AND_SW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

std::vector<LazyImgBufferPool> P2GMgr::createMultiPools(const char *name, const std::vector<MSize> &sizes, const EImageFormat &fmt)
{
    std::vector<LazyImgBufferPool> pools;
    size_t layer = sizes.size();
    pools.resize(layer);
    for( size_t i = 0; i < layer; ++i )
    {
        MSize size = align(sizes[i], BUF_ALLOC_ALIGNMENT_BIT);
        pools[i] = createFatPool(name, size, fmt);
    }
    return pools;
}

sp<IBufferPool> P2GMgr::createFatPool(const char *name, const MSize &size, const EImageFormat &fmt) const
{
    TRACE_FUNC_ENTER();
    sp<IBufferPool> pool;
    if( mNoFat )
    {
        pool = ImageBufferPool::create(name, size, fmt,
                                       ImageBufferPool::USAGE_HW);
    }
    else
    {
        pool = FatImageBufferPool::create(name, size, fmt,
                                          FatImageBufferPool::USAGE_HW);
    }
    TRACE_FUNC_EXIT();
    return pool;
}

PMDPReq makePMDPReq(const std::vector<PMDP> &list)
{
    TRACE_FUNC_ENTER();
    PMDPReq req;
    for( const PMDP &pmdp : list )
    {
        BasicImg full;
        full.mBuffer = pmdp.mIn.mFull ? pmdp.mIn.mFull->getImgBuffer() : NULL;
        full.mPQCtrl = pmdp.mIn.mPQCtrl;
        TunBuffer tun = pmdp.mIn.mTuning ? *pmdp.mIn.mTuning : NULL;
        std::vector<P2IO> extra;
        extra.reserve(pmdp.mOut.mExtra.size());
        for( const ILazy<GImg> &e : pmdp.mOut.mExtra )
        {
            extra.emplace_back(e->toP2IO());
        }
        req.add(full, extra, tun);
    }
    TRACE_FUNC_ENTER();
    return req;
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
