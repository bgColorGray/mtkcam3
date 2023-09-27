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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#include "P2_Util.h"
#include "P2_DebugControl.h"
#define P2_CLASS_TAG    P2Util
#define P2_TRACE        TRACE_P2_UTIL
#include "P2_LogHeader.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_P2_PROC_COMMON);

#define toBool(x) (!!(x))
#define FORCE_TEST_MDP 0

#define USAGE_HW_AND_SW (NSCam::eBUFFER_USAGE_SW_READ_OFTEN|NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN|NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE)

using NSCam::EImageFormat;
using NSCam::IImageBufferAllocator;
using namespace NSCam::Utils::Format;

namespace P2
{

P2Util::SimpleIO::SimpleIO()
    : mResized(MFALSE)
    , mUseLMV(MFALSE)
{
}

MVOID P2Util::SimpleIO::setUseLMV(MBOOL useLMV)
{
    mUseLMV = useLMV;
}

MBOOL P2Util::SimpleIO::hasInput() const
{
    return isValid(mIMGI);
}

MBOOL P2Util::SimpleIO::hasOutput() const
{
    return isValid(mIMG2O) ||
           isValid(mIMG3O) ||
           isValid(mWROTO) ||
           isValid(mWDMAO);
}

MBOOL P2Util::SimpleIO::isResized() const
{
    return mResized;
}

MBOOL P2Util::SimpleIO::isYuvIn() const
{
    MBOOL ret = MFALSE;
    if( isValid(mIMGI) )
    {
        NSCam::EImageFormat fmt = mIMGI->getImgFormat();
        ret = (fmt != NSCam::eImgFmt_UNKNOWN) && !NSCam::isHalRawFormat(fmt);
    }
    return ret;
}

MSize P2Util::SimpleIO::getInputSize() const
{
    MSize size(0, 0);
    if( isValid(mIMGI) )
    {
        size = mIMGI->getImgSize();
    }
    return size;
}

MVOID P2Util::SimpleIO::updateResult(MBOOL result) const
{
    if( mIMG2O != NULL ) mIMG2O->updateResult(result);
    if( mWROTO != NULL ) mWROTO->updateResult(result);
    if( mWDMAO != NULL ) mWDMAO->updateResult(result);
}

MVOID P2Util::SimpleIO::dropRecord() const
{
    if( mWROTO != NULL && mWROTO->isRecord() )
    {
        mWROTO->updateResult(MFALSE);
    }
    if( mWDMAO != NULL && mWDMAO->isRecord() )
    {
        mWDMAO->updateResult(MFALSE);
    }
}

MBOOL P2Util::SimpleIO::findDisplayAndRelease(MBOOL result)
{
    MBOOL ret = MFALSE;
    if (mWROTO != nullptr && mWROTO->isDisplay())
    {
        mWROTO->updateResult(result);
        mWROTO = nullptr;
        ret = MTRUE;
    }
    else if (mWDMAO != nullptr && mWDMAO->isDisplay())
    {
        mWDMAO->updateResult(result);
        mWDMAO = nullptr;
        ret = MTRUE;
    }
    return ret;
}

MVOID P2Util::SimpleIO::earlyRelease(MUINT32 mask, MBOOL result)
{
    if( mask & P2Util::RELEASE_DISP )
    {
        mIMGI = NULL;
        mLCEI = NULL;
        mLCSHO = NULL;
        if( mWROTO != NULL && mWROTO->isDisplay() )
        {
            mWROTO->updateResult(result);
            mWROTO = NULL;
        }
        if( mWDMAO != NULL && mWDMAO->isDisplay() )
        {
            mWDMAO->updateResult(result);
            mWDMAO = NULL;
        }
    }
    if( mask & P2Util::RELEASE_ALL_EXCEPT_REC )
    {
        mIMGI = NULL;
        mLCEI = NULL;
        mLCSHO = NULL;
        if( mWROTO != NULL && !mWROTO->isRecord() )
        {
            mWROTO->updateResult(result);
            mWROTO = NULL;
        }
        if( mWDMAO != NULL && !mWDMAO->isRecord() )
        {
            mWDMAO->updateResult(result);
            mWDMAO = NULL;
        }
        if( mIMG2O != NULL )
        {
            mIMG2O->updateResult(result);
            mIMG2O = NULL;
        }
    }
    if( mask & P2Util::RELEASE_FD )
    {
        if( mIMG2O != NULL )
        {
            mIMG2O->updateResult(result);
            mIMG2O = NULL;
        }
    }
}

sp<P2Img> P2Util::SimpleIO::getMDPSrc() const
{
    if( mWDMAO != NULL )
    {
        return mWDMAO;
    }
    else if( mWROTO != NULL )
    {
        return mWROTO;
    }
    return NULL;
}

sp<P2Img> P2Util::SimpleIO::getLcso() const
{
    return mLCEI;
}

static P2IO::TYPE toP2IOType(const IMG_TYPE type)
{
    P2IO::TYPE out = P2IO::TYPE_VSS;
    switch(type)
    {
    case IMG_TYPE_DISPLAY:  out = P2IO::TYPE_DISPLAY;   break;
    case IMG_TYPE_RECORD:   out = P2IO::TYPE_RECORD;    break;
    case IMG_TYPE_FD:       out = P2IO::TYPE_FD;        break;
    default:                                            break;
    }
    return out;
}

P2IO toP2IO(const sp<P2Img> &img)
{
    P2IO io;
    if( isValid(img) )
    {
        io.mBuffer = img->getIImageBufferPtr();
        io.mType = toP2IOType(img->getImgType());
        io.mTransform = img->getTransform();
    }
    return io;
}

MVOID updateCrop(const ILog &log, const sp<Cropper> cropper, const MUINT32 cropFlag, P2IO &io)
{
    if( io.isValid() )
    {
        io.mCropRect = cropper->calcViewAngleF(log, io.getTransformSize(), cropFlag, 1.0f, 1.0f, (DMAConstrain::DEFAULT));
    }
}

P2IOPack P2Util::SimpleIO::toP2IOPack(const P2Pack &p2Pack) const
{
    P2IOPack pack;
    //pack.mMargin = mMargin;
    pack.mFlag |= mResized ? P2Flag::FLAG_RESIZED : 0;
    pack.mFlag |= mUseLMV ? P2Flag::FLAG_LMV : 0;
    //pack.mFlag |= mUseMargin ? P2Flag::FLAG_MARGIN : 0;

    pack.mIMGI = toP2IO(mIMGI);
    pack.mIMG2O = toP2IO(mIMG2O);
    pack.mWDMAO = toP2IO(mWDMAO);
    pack.mWROTO = toP2IO(mWROTO);

    pack.mLCSO = toP2IO(mLCEI);
    pack.mLCSHO = toP2IO(mLCSHO);

    // update P2IO crop
    const sp<Cropper> cropper = p2Pack.getSensorData().mCropper;
    MUINT32 cropFlag = 0;
    cropFlag |= mResized ? Cropper::USE_RESIZED : 0;
    cropFlag |= mUseLMV ? Cropper::USE_EIS_12 : 0;

    if( cropper == NULL )
    {
        MY_S_LOGE(p2Pack.mLog, "Invalid p2pack=%d, cropper=NULL", pack.isValid());
    }
    else
    {
        updateCrop(p2Pack.mLog, cropper, cropFlag, pack.mIMG2O);
        updateCrop(p2Pack.mLog, cropper, cropFlag, pack.mWDMAO);
        updateCrop(p2Pack.mLog, cropper, cropFlag, pack.mWROTO);
    }

    return pack;
}

MVOID P2Util::SimpleIO::printIO(const ILog &log) const
{
    MY_S_LOGD(log, "resize(%d),lmv(%d)", mResized, mUseLMV);
    MY_S_LOGD(log, "imgi(%d),lcei(%d), lcsho(%d), img2o(%d),img3o(%d),wroto(%d),wdmao(%d)",
              isValid(mIMGI), isValid(mLCEI), isValid(mLCSHO),
              isValid(mIMG2O), isValid(mIMG3O),
              isValid(mWROTO), isValid(mWDMAO));
}

P2Util::SimpleIO P2Util::extractSimpleIO(const sp<P2Request> &request, MUINT32 portFlag)
{
    ILog log = spToILog(request);
    TRACE_S_FUNC_ENTER(log);
    SimpleIO io;
    MBOOL useVenc = !!(portFlag & P2Util::USE_VENC);
    if( isValid(request->mImg[IN_RESIZED]) )
    {
        io.mResized = MTRUE;
        io.mIMGI = std::move(request->mImg[IN_RESIZED]);
    }
    else if( isValid(request->mImg[IN_FULL]) )
    {
        io.mResized = MFALSE;
        io.mIMGI = std::move(request->mImg[IN_FULL]);
    }
    if( isValid(request->mImg[IN_LCSO]) )
    {
        io.mLCEI = std::move(request->mImg[IN_LCSO]);
    }
    if( isValid(request->mImg[IN_LCSHO]) )
    {
        io.mLCSHO = std::move(request->mImg[IN_LCSHO]);
    }
    if( isValid(request->mImg[OUT_FD]) )
    {
        io.mIMG2O = std::move(request->mImg[OUT_FD]);
    }
#if FORCE_TEST_MDP
    io.mWDMAO = useVenc ? NULL : P2Util::extractOut(request, P2Util::FIND_NO_ROTATE);
    if( io.mWDMAO == NULL )
    {
        io.mWROTO = P2Util::extractOut(request, P2Util::FIND_ROTATE);
    }
#else
    io.mWROTO = P2Util::extractOut(request, P2Util::FIND_ROTATE);
    if( io.mWROTO == NULL )
    {
        io.mWROTO = P2Util::extractOut(request, P2Util::FIND_NO_ROTATE);
    }
    io.mWDMAO = useVenc ? NULL : P2Util::extractOut(request, P2Util::FIND_NO_ROTATE);
#endif // FORCE_TEST_MDP
    TRACE_S_FUNC_EXIT(log);
    return io;
}

TuningParam P2Util::xmakeTuning(const P2Pack &p2Pack, const SimpleIO &io, NS3Av3::IHalISP *halISP, P2MetaSet &metaSet)
{
    const ILog &log = p2Pack.mLog;
    TuningParam tuning;
    MetaSet_T inMetaSet, outMetaSet, *pOutMetaSet = NULL;

    inMetaSet.MagicNum = 0;
    inMetaSet.appMeta = metaSet.mInApp;
    inMetaSet.halMeta = metaSet.mInHal;
    pOutMetaSet = metaSet.mHasOutput ? &outMetaSet : NULL;

    if( io.isYuvIn() )
    {
        MSize imgiSize = io.getInputSize();
        MINT32 res = (imgiSize.h << 16) | imgiSize.w;
        trySet<MINT32>(inMetaSet.halMeta, MTK_ISP_P2_IN_IMG_RES_REVISED, res);
        trySet<MINT32>(inMetaSet.halMeta, MTK_ISP_P2_IN_IMG_FMT, 1);
    }

    void *regBuffer = allocateRegBuffer();
    tuning = ::NSCam::Feature::P2Util::makeTuningParam(log, p2Pack, halISP, inMetaSet, pOutMetaSet, io.isResized(), regBuffer, toIImageBufferPtr(io.mLCEI), -1, NULL, MFALSE, NULL, toIImageBufferPtr(io.mLCSHO));

    if( metaSet.mHasOutput )
    {
        metaSet.mOutApp = outMetaSet.appMeta;
        metaSet.mOutHal = outMetaSet.halMeta;
    }
    return tuning;
}

DIPParams P2Util::xmakeDIPParams(const P2Pack &p2Pack, const SimpleIO &io, const TuningParam &tuning, const P2ObjPtr &p2ObjPtr)
{
    DIPParams dipParams;
    dipParams = ::NSCam::Feature::P2Util::makeDIPParams(p2Pack, ENormalStreamTag_Prv, io.toP2IOPack(p2Pack), p2ObjPtr, tuning);

    return dipParams;
}

const char *P2Util::p2PQIdx2String(const MUINT32 pqIdx)
{
    if (pqIdx == P2_PQ_DEFAULT)
    {
        return (const char*)"pq_default";
    }
    else if (pqIdx == P2_PQ_NORMAL)
    {
        return (const char*)"pq_normal";
    }
    else if (pqIdx == P2_PQ_EIS12_VIDEO_4k)
    {
        return (const char*)"pq_eis12_v4k";
    }
    else if (pqIdx == P2_PQ_EIS30_VIDEO)
    {
        return (const char*)"pq_eis30_v";
    }
    else if (pqIdx == P2_PQ_EIS30_VIDEO_4k)
    {
        return (const char*)"pq_eis30_v4k";
    }
    else if (pqIdx == P2_PQ_EIS35_VIDEO)
    {
        return (const char*)"pq_eis35_v";
    }
    else if (pqIdx == P2_PQ_EIS35_VIDEO_4k)
    {
        return (const char*)"pq_eis35_v4k";
    }
    else if (pqIdx == P2_PQ_EIS35_NO_VIDEO)
    {
        return (const char*)"pq_eis35_noV";
    }
    else if (pqIdx == P2_PQ_SMVRCONSTRAINT)
    {
        return (const char*)"pq_smvrC";
    }
    else if (pqIdx == P2_PQ_SMVRBATCH)
    {
        return (const char*)"pq_smvrB";
    }
    else if (pqIdx == P2_PQ_GEN_PIP_2ND)
    {
        return (const char*)"pq_gen_pip_2nd";
    }
    else if (pqIdx == P2_PQ_GEN_PIP_OTH)
    {
        return (const char*)"pq_gen_pip_oth";
    }
    else if (pqIdx == P2_PQ_VEN_PIP_1ST)
    {
        return (const char*)"pq_ven_pip_1st";
    }
    else if (pqIdx == P2_PQ_VEN_PIP_2ND)
    {
        return (const char*)"pq_ven_pip_2nd";
    }
    else if (pqIdx == P2_PQ_VEN_PIP_OTH)
    {
        return (const char*)"pq_ven_pip_oth";
    }
    else
    {
        return (const char*)"pq_unknown";
    }
}


MVOID P2Util::releaseTuning(TuningParam &tuning)
{
    releaseRegBuffer(tuning.pRegBuf);
    tuning.pRegBuf = NULL;
}

sp<P2Img> P2Util::extractOut(const sp<P2Request> &request, MUINT32 target)
{
    ILog log = spToILog(request);
    TRACE_S_FUNC_ENTER(log);
    sp<P2Img> out;
    MBOOL useRotate = toBool(target & P2Util::FIND_ROTATE);
    MBOOL checkRotate = useRotate != toBool(target & P2Util::FIND_NO_ROTATE);
    MBOOL useDisp = toBool(target & P2Util::FIND_DISP);
    MBOOL useVideo = toBool(target & P2Util::FIND_VIDEO);
    MBOOL checkType = useDisp || useVideo;
    if( request != NULL )
    {
        MSize max(0, 0);
        auto maxIt = request->mImgOutArray.end();
        for( auto it = request->mImgOutArray.begin(), end = request->mImgOutArray.end(); it != end; ++it )
        {
            if( isValid(*it) )
            {
                if( checkRotate &&
                    (useRotate != toBool((*it)->getTransform())) )
                {
                    continue;
                }
                if( checkType &&
                    !(useDisp && toBool((*it)->isDisplay())) &&
                    !(useVideo && toBool((*it)->isRecord())) )
                {
                    continue;
                }
                MSize size = (*it)->getImgSize();
                if( size.w*size.h > max.w*max.h )
                {
                    max = size;
                    maxIt = it;
                }
            }
        }
        if( maxIt != request->mImgOutArray.end() )
        {
            out = *maxIt;
            (*maxIt) = NULL;
        }
    }
    TRACE_S_FUNC_EXIT(log);
    return out;
}

P2Util::SimpleIn::SimpleIn(MUINT32 sensorId, sp<P2Request> pRequest)
: mRequest(pRequest)
, mSensorId(sensorId)
, mIsMaster(pRequest->mP2Pack.getFrameData().mMasterSensorID == sensorId)
{
}

MUINT32 P2Util::SimpleIn::getSensorId() const
{
    return mSensorId;
}

MVOID P2Util::SimpleIn::setUseLMV(MBOOL useLMV)
{
    mUseLMV = useLMV;
}

MVOID P2Util::SimpleIn::setISResized(MBOOL isResized)
{
    mResized = isResized;
}

MBOOL P2Util::SimpleIn::isMaster() const
{
    return mIsMaster;
}

MBOOL P2Util::SimpleIn::isResized() const
{
    return mResized;
}

MBOOL P2Util::SimpleIn::useLMV() const
{
    return mUseLMV;
}

MBOOL P2Util::SimpleIn::useCropRatio() const
{
    return mUseCropRatio;
}

MSize P2Util::SimpleIn::getInputSize() const
{
    return mIMGI->getImgSize();
}

sp<P2Img> P2Util::SimpleIn::getLcso() const
{
    return mLCEI;
}

MVOID P2Util::SimpleIn::addCropRatio(const char* name, const float cropRatio)
{
    addCropRatio(name, cropRatio, cropRatio);
}

MVOID P2Util::SimpleIn::addCropRatio(const char* name, const float widthCropRatio, const float heightCropRatio)
{
    TRACE_FUNC_ENTER();
    mUseCropRatio = MTRUE;
    mCropRatio.w *= widthCropRatio;
    mCropRatio.h *= heightCropRatio;
    TRACE_FUNC("%s cropRatio=(%f,%f), total cropRatio=(%f,%f)", name, widthCropRatio, heightCropRatio, mCropRatio.w, mCropRatio.h);
    TRACE_FUNC_EXIT();
}

MBOOL P2Util::SimpleIn::hasCropRatio() const
{
    return mUseCropRatio;
}

MSizeF P2Util::SimpleIn::getCropRatio() const
{
    return mCropRatio;
}

MVOID P2Util::SimpleIn::releaseAllImg()
{
    mIMGI = nullptr;
    mLCEI = nullptr;
    mLCSHO = nullptr;
    mRSSO = nullptr;
    mRSSOR2 = nullptr;
    mPreRSSO = nullptr;
    mFullYuv  = nullptr;
    mRrzYuv1  = nullptr;
    mRrzYuv2  = nullptr;
    mAIYuv = nullptr;
}

P2Util::SimpleOut::SimpleOut(MUINT32 sensorId, sp<P2Request> pRequest, sp<P2Img>& pImg)
:  mRequest(pRequest)
,  mDMAConstrain(NSCam::Feature::P2Util::DMAConstrain::DEFAULT)
,  mSensorId(sensorId)
{
    mImg = std::move(pImg);
}

MUINT32 P2Util::SimpleOut::getSensorId() const
{
    return mSensorId;
}

MVOID P2Util::SimpleOut::setIsFD(MBOOL isFDBuffer)
{
    mFD = isFDBuffer;
}

MVOID P2Util::SimpleOut::setIsAppDepth(MBOOL isAppDepth)
{
    mAppDepth = isAppDepth;
}

MBOOL P2Util::SimpleOut::isDisplay() const
{
    return mImg->isDisplay();
}

MBOOL P2Util::SimpleOut::isRecord() const
{
    return mImg->isRecord();
}

MBOOL P2Util::SimpleOut::isFD() const
{
    return mFD;
}

MBOOL P2Util::SimpleOut::isAppDepth() const
{
    return mAppDepth;
}

MBOOL P2Util::SimpleOut::isMDPOutput() const
{
    ID_IMG outID = mImg->getID();
    return (outID == OUT_YUV) || (outID == OUT_JPEG_YUV) || (outID == OUT_THN_YUV);
}

MBOOL P2Util::SimpleOut::isPhysical() const
{
    return mImg->isPhysicalStream();
}

BasicPQCtrl::BasicPQCtrl(MUINT32 sensorID, const P2Pack &p2pack, const std::shared_ptr<FD_DATATYPE> &fd, MBOOL isDump, MBOOL fdPrint)
    : mSensorID(sensorID)
    , mP2Pack(p2pack.getP2Pack(sensorID))
    , mFD(fd)
    , mIsDump(isDump)
    , mFDPrint(fdPrint)
    , mConfig(mP2Pack.getConfigInfo())
    , mPlat(mP2Pack.getSensorInfo().mPlatInfo)
    , mPQIdx(mConfig.mUsageHint.mP2PQIndex)
{
    mPQ = ::NSCam::Feature::P2Util::makeDpPqParam(mP2Pack, sensorID);
}

BasicPQCtrl::~BasicPQCtrl()
{
}

MBOOL BasicPQCtrl::supportPQ() const
{
    return mConfig.mSupportPQ;
}

MUINT32 BasicPQCtrl::getSensorID() const
{
    return mSensorID;
}

MUINT32 BasicPQCtrl::getLogLevel() const
{
    return getADBLogLevel();
}

MBOOL BasicPQCtrl::isDummy() const
{
    return MFALSE;
}

MBOOL BasicPQCtrl::isDump() const
{
    return mIsDump;
}

MBOOL BasicPQCtrl::is2ndDRE() const
{
    return MFALSE;
}

MUINT32 BasicPQCtrl::getPQIndex() const
{
    return mPQIdx;
}

MVOID BasicPQCtrl::fillPQInfo(DpPqParam *pq) const
{
    if( pq )
    {
        (*pq) = mPQ;
    }
}

const char* BasicPQCtrl::getDebugName() const
{
    return "p2s";
}

void* BasicPQCtrl::getIspTuning() const
{
    return NULL;
}

void* BasicPQCtrl::getDreTuning() const
{
    return NULL;
}

MUINT32 BasicPQCtrl::getDreTuningSize() const
{
    return 0;
}

void* BasicPQCtrl::getFDdata() const
{
    if( mFDPrint )
    {
        if( mFD.get() )
        {
            MY_LOGD("FDdata(%p) .number_of_faces(%d) .ImgWidthxImgHeight(%dx%d) .timestamp(%" PRId64 ")\n",
                    mFD.get(), mFD->facedata.number_of_faces,
                    mFD->facedata.ImgWidth, mFD->facedata.ImgHeight, mFD->facedata.timestamp);
        }
        else
        {
            MY_LOGW("FDdata is null !");
        }
    }
    return mFD.get();
}

MBOOL BasicPQCtrl::needDefaultPQ(eP2_PQ_PATH path) const
{
    return mPlat->supportDefaultPQPath(path);
}

MBOOL BasicPQCtrl::needCZ(eP2_PQ_PATH path) const
{
    return mConfig.mSupportClearZoom && mPlat->supportCZPath(mPQIdx, path);
}

MBOOL BasicPQCtrl::needHFG(eP2_PQ_PATH path) const
{
    return mConfig.mSupportHFG && mPlat->supportHFGPath(mPQIdx, path);
}

MBOOL BasicPQCtrl::needDRE(eP2_PQ_PATH path) const
{
    return mConfig.mSupportDRE && mPlat->supportDREPath(mPQIdx, path);
}

MBOOL BasicPQCtrl::needHDR10(eP2_PQ_PATH /*path*/) const
{
    return mConfig.mUsageHint.mIsHdr10;
}

MUINT32 BasicPQCtrl::getADBLogLevel()
{
    static MUINT32 sLogLevel = property_get_int32(KEY_PQ_CTRL_PRINT, 0);
    return sLogLevel;
}

static MUINT32 queryStrideInBytes(EImageFormat fmt, MUINT32 i, MUINT32 width)
{
    MUINT32 byte = 0;
    byte = queryPlaneWidthInPixels(fmt, i, width) * queryPlaneBitsPerPixel(fmt, i) / 8;
    return byte;
}

static IImageBufferAllocator::ImgParam makeImgParam(EImageFormat fmt, const MSize &size)
{
    const MUINT32 MAX_PLANE = 3;
    size_t stride[MAX_PLANE] = { 0 };
    size_t boundary[MAX_PLANE] = { 0 };
    MUINT32 plane = queryPlaneCount(fmt);

    if( plane > MAX_PLANE )
    {
        MY_LOGE("plane(%d) > %d for fmt:0x%x, force set to %d", plane, MAX_PLANE, fmt, MAX_PLANE);
        plane = (MUINT32)MAX_PLANE;
    }
    for( MUINT32 i = 0; i < plane && i < MAX_PLANE; ++i )
    {
        stride[i] = queryStrideInBytes(fmt, i, size.w);
    }

    return IImageBufferAllocator::ImgParam(fmt, size, stride, boundary, plane);
}

static sp<IImageBuffer> createIImageBuffer(const char *name, EImageFormat fmt, const MSize &size)
{
    sp<IImageBufferHeap> heap;
    sp<IImageBuffer> buffer;

    heap = NSCam::IIonImageBufferHeap::create(name, makeImgParam(fmt, size));
    if( heap )
    {
        buffer = heap->createImageBuffer();
        buffer->lockBuf(name, USAGE_HW_AND_SW);
    }
    else
    {
        MY_LOGE("IonImageBufferHeap create failed: %s", name);
    }

    return buffer;
}

Pool::Pool()
{
}

Pool::~Pool()
{
    TRACE_FUNC_ENTER();
    if( mBuffer )
    {
        destroy();
    }
    TRACE_FUNC_EXIT();
}

MVOID Pool::create(const char *name, EImageFormat fmt, const MSize &size)
{
    TRACE_FUNC_ENTER();
    if( !mBuffer )
    {
        mName = (name != NULL) ? name : "unknown";
        mBuffer = createIImageBuffer(mName.c_str(), fmt, size);
    }
    TRACE_FUNC_EXIT();
}

MVOID Pool::destroy()
{
    TRACE_FUNC_ENTER();
    if( mBuffer )
    {
        mBuffer->unlockBuf(mName.c_str());
        mBuffer = NULL;
    }
    TRACE_FUNC_EXIT();
}

sp<IImageBuffer> Pool::getBuffer() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return mBuffer;
}

MBOOL copyImageBuffer(IImageBuffer *src, IImageBuffer *dst)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;

    if( !src || !dst )
    {
        MY_LOGE("Invalid buffers src=%p dst=%p", src, dst);
        ret = MFALSE;
    }
    else if( src->getImgSize() != dst->getImgSize() )
    {
        MY_LOGE("Mismatch buffer size src(%dx%d) dst(%dx%d)",
                src->getImgSize().w, src->getImgSize().h,
                dst->getImgSize().w, dst->getImgSize().h);
        ret = MFALSE;
    }
    else
    {
        unsigned srcPlane = src->getPlaneCount();
        unsigned dstPlane = dst->getPlaneCount();

        if( !srcPlane || !dstPlane ||
            (srcPlane != dstPlane && srcPlane != 1 && dstPlane != 1) )
        {
            MY_LOGE("Mismatch buffer plane src(%d) dst(%d)", srcPlane, dstPlane);
            ret = MFALSE;
        }
        for( unsigned i = 0; i < srcPlane; ++i )
        {
            if( !src->getBufVA(i) )
            {
                MY_LOGE("Invalid src plane[%d] VA", i);
                ret = MFALSE;
            }
        }
        for( unsigned i = 0; i < dstPlane; ++i )
        {
            if( !dst->getBufVA(i) )
            {
                MY_LOGE("Invalid dst plane[%d] VA", i);
                ret = MFALSE;
            }
        }

        if( ret )
        {
            char *srcVA = NULL, *dstVA = NULL;
            size_t srcSize = 0;
            size_t dstSize = 0;
            size_t srcStride = 0;
            size_t dstStride = 0;

            for( unsigned i = 0; i < srcPlane && i < dstPlane; ++i )
            {
                if( i < srcPlane )
                {
                    srcVA = (char*)src->getBufVA(i);
                }
                if( i < dstPlane )
                {
                    dstVA = (char*)dst->getBufVA(i);
                }

                srcSize = src->getBufSizeInBytes(i);
                dstSize = dst->getBufSizeInBytes(i);
                srcStride = src->getBufStridesInBytes(i);
                dstStride = dst->getBufStridesInBytes(i);
                if( srcStride == dstStride )
                {
                    memcpy((void*)dstVA, (void*)srcVA, (srcSize <= dstSize) ? srcSize : dstSize );
                    MY_LOGD("plane[%d] memcpy %p(%zu)=>%p(%zu)",
                              i, srcVA, srcSize, dstVA, dstSize);
                }
                else
                {
                    MY_LOGD("plane[%d] memcpy %p(%zu)=>%p(%zu), stride: src(%zu) dst(%zu)",
                              i, srcVA, srcSize, dstVA, dstSize,
                              srcStride, dstStride);
                    size_t stride = (srcStride < dstStride) ? srcStride : dstStride;
                    unsigned height = dstSize / dstStride;
                    for( unsigned j = 0; j < height; ++j )
                    {
                        memcpy((void*)dstVA, (void*)srcVA, stride);
                        srcVA += srcStride;
                        dstVA += dstStride;
                    }
                }
            }
        }
    }

    TRACE_FUNC_EXIT();
    return ret;
}
} // namespace P2
