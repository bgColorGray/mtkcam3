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

#include "StreamingFeatureType.h"
#include "StreamingFeatureNode.h"
#include "StreamingFeature_Common.h"
#include <camera_custom_eis.h>
#include <camera_custom_dualzoom.h>

#include <utility>

#define PIPE_CLASS_TAG "Type"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_TYPE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam3/feature/DualCam/DualCam.Common.h>

#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);
using namespace NSCam::Utils::ULog;

using NSCam::NSIoPipe::QParams;
using NSCam::NSIoPipe::MCropRect;
using NSCam::Feature::P2Util::P2IO;
using NSCam::Feature::P2Util::P2Pack;
using NSCam::Feature::P2Util::P2SensorData;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

IMRect::IMRect()
{
}

IMRect::IMRect(const IMRect &src)
    : mIsValid(src.mIsValid)
    , mRect(src.mRect)
{
}

IMRect::IMRect(const MSize &size, const MPoint &point, MBOOL valid)
    : mIsValid(valid)
    , mRect(MRect(point, size))
{
}

IMRect::IMRect(const MRect &rect, MBOOL valid)
    : mIsValid(valid)
    , mRect(rect)
{
}

IMRect& IMRect::operator=(const IMRect &src)
{
    mIsValid = src.mIsValid;
    mRect = src.mRect;
    return *this;
}

IMRect::operator bool() const
{
    return mIsValid;
}

MRect IMRect::toMRect() const
{
    return mIsValid ? mRect : MRect();
}

MRectF IMRect::toMRectF() const
{
    return mIsValid ? MRectF(mRect.p, mRect.s) : MRectF();
}

IMRectF::IMRectF()
{
}

IMRectF::IMRectF(const IMRectF &src)
    : mIsValid(src.mIsValid)
    , mRect(src.mRect)
{
}

IMRectF::IMRectF(const MSizeF &size, const MPointF &point, MBOOL valid)
    : mIsValid(valid)
    , mRect(MRectF(point, size))
{
}

IMRectF::IMRectF(const MRectF &rect, MBOOL valid)
    : mIsValid(valid)
    , mRect(rect)
{
}

IMRectF& IMRectF::operator=(const IMRectF &src)
{
    mIsValid = src.mIsValid;
    mRect = src.mRect;
    return *this;
}

IMRectF::operator bool() const
{
    return mIsValid;
}

MRectF IMRectF::toMRectF() const
{
    return mIsValid ? mRect : MRectF();
}

ITrackingData::ITrackingData()
{
}

ITrackingData::ITrackingData(const MSizeF &size, const MPointF &point)
    : mInitROI(MRectF(point, size))
    , mTrackingROI(MRectF(point, size))
{
}

ITrackingData::ITrackingData(const MRectF &rect)
    : mInitROI(rect)
    , mTrackingROI(rect)
{
}

FeaturePipeParam::MSG_TYPE toFPPMsg(HelperMsg msg)
{
    switch(msg)
    {
    case HMSG_FRAME_DONE:   return FeaturePipeParam::MSG_FRAME_DONE;
    case HMSG_DISPLAY_DONE: return FeaturePipeParam::MSG_DISPLAY_DONE;
    case HMSG_TRACKING_DONE: return FeaturePipeParam::MSG_TRACKING_DONE;
    default:                return FeaturePipeParam::MSG_INVALID;
    };
}

MVOID HelperRWData::markReady(HelperMsg msg)
{
    if( msg < HMSG_COUNT )
    {
        mMsg[msg] |= HMSG_STATE_READY;
    }
}

MVOID HelperRWData::markDone(HelperMsg msg)
{
    if( msg < HMSG_COUNT )
    {
        mMsg[msg] |= HMSG_STATE_DONE;
    }
}

MBOOL HelperRWData::isReady(HelperMsg msg) const
{
    return (msg < HMSG_COUNT) && (mMsg[msg] & HMSG_STATE_READY) && !(mMsg[msg] & HMSG_STATE_DONE);
}

MBOOL HelperRWData::isReadyOrDone(HelperMsg msg) const
{
    return (msg < HMSG_COUNT) &&
           (mMsg[msg] & (HMSG_STATE_READY|HMSG_STATE_DONE));
}

MBOOL HelperRWData::isReadyOrDone(const std::set<HelperMsg> &msgs) const
{
    MBOOL ret = MTRUE;
    for( const HelperMsg &m : msgs )
    {
        ret = ret && m < HMSG_COUNT &&
              (mMsg[m] & (HMSG_STATE_READY|HMSG_STATE_DONE));
    }
    return ret;
}

RSCResult::RSCResult()
    : mMV(NULL)
    , mBV(NULL)
    , mIsValid(MFALSE)
{
}

RSCResult::RSCResult(const ImgBuffer &mv, const ImgBuffer &bv, const MSize& rssoSize, const RSC_STA_0& rscSta, MBOOL valid)
    : mMV(mv)
    , mBV(bv)
    , mRssoSize(rssoSize)
    , mRscSta(rscSta)
    , mIsValid(valid)
{
}

DSDNImg::DSDNImg()
{
}

DSDNImg::DSDNImg(const BasicImg &full, const BasicImg &slavefull)
    : mPackM(full)
    , mPackS(slavefull)
{
}

DSDNImg::Pack::Pack()
{
}

DSDNImg::Pack::Pack(const BasicImg &full)
    : mFullImg(full)
{
}

MVOID DomainTransform::accumulate(const char* name, const Feature::ILog &log, const MSize &inSize, const MRectF &inCrop, const MSize &outSize)
{
    accumulate(name, log, MSizeF(inSize), inCrop, MSizeF(outSize));
}

MVOID DomainTransform::accumulate(const char* name, const Feature::ILog &log, const MSizeF &inSize, const MRectF &inCrop, const MSizeF &outSize)
{
    if(isValid(inSize) && isValidCrop(inSize, inCrop) && isValid(outSize))
    {
        MPointF calX((inSize.w - inCrop.s.w) / 2.0f, (inSize.h - inCrop.s.h) / 2.0f);
        MSizeF calS(1.0f, 1.0f);
        if(outSize != inCrop.s)
        {
            calS.w = outSize.w / inCrop.s.w;
            calS.h = outSize.h / inCrop.s.h;
            MY_S_LOGI_IF(log.getLogLevel(), log, "Transform (%s) inCrop.s != outSize," MCropF_STR "," MSizeF_STR,
                            name, MCropF_ARG(inCrop), MSizeF_ARG(outSize));
        }

        MPointF newOffset;
        MSizeF newScale;
        newOffset.x = (mOffset.x + calX.x) * calS.w;
        newOffset.y = (mOffset.y + calX.y) * calS.h;
        newScale.w = mScale.w * calS.w;
        newScale.h = mScale.h * calS.h;

        MY_S_LOGD_IF(log.getLogLevel(), log, "Transform (%s) old=" MTransF_STR " + inSize=" MSizeF_STR ", inCrop=" MCropF_STR ", outSize=" MSizeF_STR "calX=" MPointF_STR ",calS=" MSizeF_STR ",->new=" MTransF_STR,
                        name, MTransF_ARG(mOffset, mScale), MSizeF_ARG(inSize), MCropF_ARG(inCrop), MSizeF_ARG(outSize), MPointF_ARG(calX), MSizeF_ARG(calS), MTransF_ARG(newOffset, newScale));
        mOffset = newOffset;
        mScale = newScale;
    }
    else
    {
        MY_S_LOGE(log, "Transform accumulate error!!! (%s) inCrop=" MCropF_STR " inSize=" MSizeF_STR " outSize=" MSizeF_STR,
                    name, MCropF_ARG(inCrop), MSizeF_ARG(inSize), MSizeF_ARG(outSize));
    }
}

MVOID DomainTransform::accumulate(const char* name, const Feature::ILog &log, const MRectF &inZoomROI, const MRectF &outZoomROI, const MSizeF &outSize)
{
    if(isValid(inZoomROI.s) && isValidCrop(outSize, outZoomROI))
    {
        MSizeF calS(outZoomROI.s.w / inZoomROI.s.w, outZoomROI.s.h / inZoomROI.s.h);
        MPointF newOffset;
        MSizeF newScale;
        newOffset.x = (mOffset.x + inZoomROI.p.x) * calS.w - outZoomROI.p.x;
        newOffset.y = (mOffset.y + inZoomROI.p.y) * calS.h - outZoomROI.p.y;
        newScale.w = mScale.w * calS.w;
        newScale.h = mScale.h * calS.h;

        MY_S_LOGD_IF(log.getLogLevel(), log, "Transform (%s) old=" MTransF_STR " + inZoomROI=" MCropF_STR ", outZoomROI=" MCropF_STR ",outSize=" MSizeF_STR ",calS=" MSizeF_STR ",->new=" MTransF_STR,
                        name, MTransF_ARG(mOffset, mScale), MCropF_ARG(inZoomROI), MCropF_ARG(outZoomROI), MSizeF_ARG(outSize), MSizeF_ARG(calS), MTransF_ARG(newOffset, newScale));
        mOffset = newOffset;
        mScale = newScale;
    }
    else
    {
        MY_S_LOGE(log, "Transform accumulate error!!! (%s) inZoomROI=" MCropF_STR " outZoomROI=" MCropF_STR,
                    name, MCropF_ARG(inZoomROI), MCropF_ARG(outZoomROI));
    }
}

MRectF DomainTransform::applyTo(const MRectF &crop) const
{
    MRectF out;
    out.p.x = crop.p.x * mScale.w - mOffset.x;
    out.p.y = crop.p.y * mScale.h - mOffset.y;
    out.s.w = crop.s.w * mScale.w;
    out.s.h = crop.s.h * mScale.h;


    if(SFP_ABS(out.p.x, 0.0f) < 0.0001f)
    {
        out.p.x = 0.0f;
    }
    if(SFP_ABS(out.p.y, 0.0f) < 0.0001f)
    {
        out.p.y = 0.0f;
    }
    return out;
}

BasicImg::BasicImg()
    : mBuffer(NULL)
    , mIsReady(MTRUE)
{
}

BasicImg::BasicImg(const ImgBuffer &img, MBOOL isReady)
    : mBuffer(img)
    , mIsReady(isReady)
{
}

BasicImg::BasicImg(const ImgBuffer &img, MUINT32 sensorID, const MRectF &crop, const MSize &sensorSize, MBOOL isReady)
    : mBuffer(img)
    , mSensorClipInfo(sensorID, crop, sensorSize)
    , mIsReady(isReady)
{
}

BasicImg::operator bool() const
{
    return isValid();
}

MBOOL BasicImg::operator==(const BasicImg &rhs) const
{
  return (mBuffer == rhs.mBuffer);
}

MBOOL BasicImg::operator!=(const BasicImg &rhs) const
{
  return (mBuffer != rhs.mBuffer);
}

MBOOL BasicImg::isValid() const
{
    return mBuffer != NULL;
}

IImageBuffer* BasicImg::getImageBufferPtr() const
{
    return mBuffer != NULL ? mBuffer->getImageBufferPtr() : NULL;
}

MVOID BasicImg::setAllInfo(const BasicImg &img, const MSize &size)
{
    mSensorClipInfo = img.mSensorClipInfo;
    mTransform = img.mTransform;
    if( mBuffer != NULL && img.mBuffer != NULL )
    {
        mBuffer->setExtParam(NSFeaturePipe::isValid(size) ? size : img.mBuffer->getImgSize());
        mBuffer->setTimestamp(img.mBuffer->getTimestamp());
    }
    mPQCtrl = img.mPQCtrl;
}

MVOID BasicImg::setPQCtrl(const IAdvPQCtrl_const &pqCtrl)
{
    mPQCtrl = pqCtrl;
}

MVOID BasicImg::setExtParam(const MSize &size)
{
    if( mBuffer != NULL && size != mBuffer->getImgSize() )
    {
        mBuffer->setExtParam(size);
    }
}

MBOOL BasicImg::syncCache(NSCam::eCacheCtrl ctrl)
{
    return (mBuffer != NULL) && mBuffer->syncCache(ctrl);
}

MVOID BasicImg::accumulate(const char* name, const Feature::ILog &log, const MSize &inSize, const MRectF &crop, const MSize &outSize)
{
    mSensorClipInfo.accumulate(name, log, inSize, crop);
    mTransform.accumulate(name, log, inSize, crop, outSize);
}

MVOID BasicImg::accumulate(const char* name, const Feature::ILog &log, const MSizeF &inSize, const MRectF &crop, const MRectF &inZoomROI, const MRectF &outZoomROI, const MSizeF &outSize)
{
    mSensorClipInfo.accumulate(name, log, inSize, crop);
    if( isValidCrop(inSize, inZoomROI) && isValidCrop(outSize, outZoomROI) )
    {
        mTransform.accumulate(name, log, inZoomROI, outZoomROI, outSize);
    }
    else
    {
        MY_S_LOGE(log, "ZoomROI inValid (%s) inSize=" MSizeF_STR ",inZoomROI=" MCropF_STR ",outSize=" MSizeF_STR ", outZoomROI=" MCropF_STR,
                    name, MSizeF_ARG(inSize), MCropF_ARG(inZoomROI), MSizeF_ARG(outSize), MCropF_ARG(outZoomROI));
    }
}

BasicImg::SensorClipInfo::SensorClipInfo()
{}

BasicImg::SensorClipInfo::SensorClipInfo(MUINT32 sensorID, const MRectF &crop, const MSize &sensorSize)
    : mSensorID(sensorID)
    , mSensorCrop(crop)
    , mSensorSize(sensorSize)
{}

BasicImg::SensorClipInfo::SensorClipInfo(MUINT32 sensorID, const MRect &crop, const MSize &sensorSize)
    : mSensorID(sensorID)
    , mSensorCrop(crop.p, crop.s)
    , mSensorSize(sensorSize)
{}

MVOID BasicImg::SensorClipInfo::accumulate(const char* name, const Feature::ILog &log, const MSize &inputSize, const MRectF &cropInInput)
{
    accumulate(name, log, MSizeF(inputSize), cropInInput);
}

MVOID BasicImg::SensorClipInfo::accumulate(const char* name, const Feature::ILog &log, const MSizeF &inputSize, const MRectF &cropInInput)
{
    if( NSFeaturePipe::isValid(inputSize) )
    {
        MRectF newCrop;
        MSizeF ratio = MSizeF(mSensorCrop.s.w / inputSize.w , mSensorCrop.s.h / inputSize.h);
        newCrop.p.x = mSensorCrop.p.x + cropInInput.p.x * ratio.w;
        newCrop.p.y = mSensorCrop.p.y + cropInInput.p.y * ratio.h;
        newCrop.s.w = cropInInput.s.w * ratio.w;
        newCrop.s.h = cropInInput.s.h * ratio.h;

        MY_S_LOGD_IF(log.getLogLevel(), log, "SensorClipInfo (%s) old=" MCropF_STR " + inSize=" MSizeF_STR "/inCrop=" MCropF_STR "->newCrop=" MCropF_STR,
                        name, MCropF_ARG(mSensorCrop), MSizeF_ARG(inputSize), MCropF_ARG(cropInInput), MCropF_ARG(newCrop));
        if( !isValidCrop(mSensorSize, newCrop) )
        {
            MY_S_LOGE(log, "SensorClipInfo result error! (%s) old=" MCropF_STR " + inSize=" MSizeF_STR "/inCrop=" MCropF_STR "->newCrop=" MCropF_STR ", sSize=" MSize_STR,
                        name, MCropF_ARG(mSensorCrop), MSizeF_ARG(inputSize), MCropF_ARG(cropInInput), MCropF_ARG(newCrop), MSize_ARG(mSensorSize));
        }
        mSensorCrop = newCrop;
    }
    else
    {
        MY_S_LOGE(log, "SensorClipInfo size error!!! (%s) oldCrop=" MCropF_STR " inSize=" MSizeF_STR,
                    name, MCropF_ARG(mSensorCrop), MSizeF_ARG(inputSize));
    }
}

MSize BasicImg::getImgSize() const
{
    return mBuffer ? mBuffer->getImgSize() : MSize();
}

WarpImg::WarpImg()
{
}

WarpImg::WarpImg(const ImgBuffer &img, const MSizeF &targetInSize, const MRectF &targetCrop, const SyncCtrlData &syncCtrlData)
    : mBuffer(img)
    , mInputSize(targetInSize)
    , mInputCrop(targetCrop)
    , mSyncCtrlData(syncCtrlData)
{
}

WarpImg::operator bool() const
{
    return (mBuffer != NULL);
}

DualBasicImg::DualBasicImg()
{
}

DualBasicImg::DualBasicImg(const BasicImg &master)
    : mMaster(master)
{
}

DualBasicImg::DualBasicImg(const BasicImg &master, const BasicImg &slave)
    : mMaster(master)
    , mSlave(slave)
{
}

DualBasicImg::operator bool() const
{
    return (mMaster || mSlave);
}

PMDPReq::Data::Data()
{
}

PMDPReq::Data::Data(const BasicImg &in, const std::vector<P2IO> &out, const TunBuffer &tuning)
    : mMDPIn(in)
    , mMDPOuts(out)
    , mTuning(tuning)
{
}

MVOID PMDPReq::add(const BasicImg &in, const std::vector<P2IO> &out, const TunBuffer &tuning)
{
    mDatas.emplace_back(Data(in, out, tuning));
}

HelpReq::HelpReq()
{
}

HelpReq::HelpReq(HelperMsg helperMsg)
    : mHelperMsg(helperMsg)
{
}

HelpReq::HelpReq(FeaturePipeParam::MSG_TYPE msg)
    : mCBMsg(msg)
{
}

HelpReq::HelpReq(FeaturePipeParam::MSG_TYPE msg, HelperMsg helperMsg)
    : mCBMsg(msg)
    , mHelperMsg(helperMsg)
{
}

HelperMsg HelpReq::toHelperMsg() const
{
    HelperMsg msg = mHelperMsg;
    if( msg == HMSG_UNKNOWN )
    {
        switch( mCBMsg )
        {
        case FeaturePipeParam::MSG_FRAME_DONE:
            msg = HMSG_FRAME_DONE;
            break;
        case FeaturePipeParam::MSG_DISPLAY_DONE:
            msg = HMSG_DISPLAY_DONE;
            break;
        case FeaturePipeParam::MSG_RSSO_DONE:
            msg = HMSG_RSSO_DONE;
            break;
        case FeaturePipeParam::MSG_TRACKING_DONE:
            msg = HMSG_TRACKING_DONE;
            break;
        default:
            msg = HMSG_UNKNOWN;
            break;
        }
    }
    return msg;
}

TPIRes::TPIRes()
{
}

TPIRes::TPIRes(const BasicImg &yuv)
{
    mSFP[TPI_BUFFER_ID_MTK_YUV] = yuv;
}

TPIRes::TPIRes(const DualBasicImg &dual)
{
    mSFP[TPI_BUFFER_ID_MTK_YUV] = dual.mMaster;
    mSFP[TPI_BUFFER_ID_MTK_YUV_2] = dual.mSlave;
}

MVOID TPIRes::add(const DepthImg &depth)
{
    mSFP[TPI_BUFFER_ID_MTK_DEPTH] = depth.mDepthMapImg;
    mSFP[TPI_BUFFER_ID_MTK_DEPTH_INTENSITY] = depth.mDepthIntensity;
}

MVOID TPIRes::setZoomROI(const MRectF &roi)
{
    mZoomROI = roi;
}

MRectF TPIRes::getZoomROI() const
{
    return mZoomROI;
}

BasicImg TPIRes::getSFP(unsigned id) const
{
    BasicImg ret;
    auto it = mSFP.find(id);
    if( it != mSFP.end() )
    {
        ret = it->second;
    }
    return ret;
}

BasicImg TPIRes::getTP(unsigned id) const
{
    BasicImg ret;
    auto it = mTP.find(id);
    if( it != mTP.end() )
    {
        ret = it->second;
    }
    return ret;
}

IMetadata* TPIRes::getMeta(unsigned id) const
{
    IMetadata *ret = NULL;
    auto it = mMeta.find(id);
    if( it != mMeta.end() )
    {
        ret = it->second;
    }
    return ret;
}

MVOID TPIRes::setSFP(unsigned id, const BasicImg &img)
{
    mSFP[id] = img;
}

MVOID TPIRes::setTP(unsigned id, const BasicImg &img)
{
    mTP[id] = img;
}

MVOID TPIRes::setMeta(unsigned id, IMetadata *meta)
{
    mMeta[id] = meta;
}

MUINT32 TPIRes::getImgArray(TPI_Image imgs[], unsigned count) const
{
    MUINT32 index = 0;
    for( auto &img : mTP )
    {
        if( index < count && img.second.mBuffer != NULL )
        {
            imgs[index].mBufferID = img.first;
            imgs[index].mBufferPtr = img.second.mBuffer->getImageBufferPtr();
            imgs[index].mViewInfo = makeViewInfo(img.first, img.second);
            ++index;
        }
    }
    return index;
}

MUINT32 TPIRes::getMetaArray(TPI_Meta metas[], unsigned count) const
{
    MUINT32 index = 0;
    for( auto &meta : mMeta )
    {
        if( index < count )
        {
            metas[index].mMetaID = meta.first;
            metas[index].mMetaPtr = meta.second;
            ++index;
        }
    }
    return index;
}

TPI_ViewInfo TPIRes::makeViewInfo(unsigned id, const BasicImg &img) const
{
    TPI_ViewInfo info;
    // is input yuv
    if( id == TPI_BUFFER_ID_MTK_YUV ||
        id == TPI_BUFFER_ID_MTK_YUV_2 )
    {
        info.mSensorID = img.mSensorClipInfo.mSensorID;
        info.mSensorSize = img.mSensorClipInfo.mSensorSize;
        info.mSensorClip = img.mSensorClipInfo.mSensorCrop;
        if( id == TPI_BUFFER_ID_MTK_YUV )
        {
            info.mSrcZoomROI = mZoomROI;
        }
    }
    return info;
}

MVOID TPIRes::updateViewInfo(const char* name, const Feature::ILog &log, const TPI_Image imgs[], unsigned count)
{
    for( unsigned i = 0; i < count; ++i )
    {
        unsigned id = imgs[i].mBufferID;
        unsigned src = id;
        // TODO eisq: need to handle output base on main 2 case
        switch( id )
        {
        case TPI_BUFFER_ID_MTK_OUT_YUV:
        case TPI_BUFFER_ID_MTK_OUT_DISPLAY:
            src = TPI_BUFFER_ID_MTK_YUV;
            break;
        case TPI_BUFFER_ID_MTK_OUT_RECORD:
            src = TPI_BUFFER_ID_MTK_Q_RECORD_IN;
            break;
        case TPI_BUFFER_ID_MTK_OUT_YUV_2:
            src = TPI_BUFFER_ID_MTK_YUV_2;
            break;
        default:
            break;
        }
        if( src != id )
        {
            updateViewInfo(name, log, imgs[i], src, id);
        }
    }
}

MVOID TPIRes::updateViewInfo(const char* name, const Feature::ILog &log, const TPI_Image &img, unsigned srcID, unsigned dstID)
{
    auto srcIt = mTP.find(srcID);
    auto dstIt = mTP.find(dstID);

    if( srcIt != mTP.end() && dstIt != mTP.end() )
    {
        const BasicImg &src = srcIt->second;
        const TPI_ViewInfo &info = img.mViewInfo;
        BasicImg &dst = dstIt->second;

        if( isValid(info.mDstZoomROI.s) )
        {
            // use zoom roi to update transform
            // sensor info need update by real src sensor
            // transform need update by main1 transform
            MSizeF dstSize = isValid(info.mDstImageClip.s) ? info.mDstImageClip.s : MSizeF(dst.mBuffer->getImgSize());
            MSize srcSize = src.mBuffer->getImgSize();
            MRectF srcClip = isValid(info.mSrcImageClip.s) ? info.mSrcImageClip : MRectF(MPoint(), srcSize);
            dst.accumulate(name, log, srcSize, srcClip, getZoomROI(), info.mDstZoomROI, dstSize);
            dst.mBuffer->setExtParam(dstSize.toMSize());
        }
        else if( isValid(info.mSrcImageClip.s) && isValid(info.mDstImageClip.s) )
        {
            MSize dstSize = info.mDstImageClip.s.toMSize();
            dst.accumulate(name, log, src.mBuffer->getImgSize(), info.mSrcImageClip, dstSize);
            dst.mBuffer->setExtParam(dstSize);
        }
        else
        {
            MY_S_LOGI_IF(log.getLogLevel(), log,
               "(%s) not accumulate. src/dst ID(0x%x/0x%x), dst size(" MSize_STR "),trans(" MTransF_STR "),sID(%d),sCrop(" MCropF_STR ")",
                name, srcID, dstID, MSize_ARG(dst.mBuffer->getImgSize()), MTransF_ARG(dst.mTransform.mOffset, dst.mTransform.mScale),
                dst.mSensorClipInfo.mSensorID, MCropF_ARG(dst.mSensorClipInfo.mSensorCrop));
        }

    }
    else
    {
        MY_S_LOGW(log, "(%s) buffer not found! , src:ID(0x%x)/found(%d), dst:ID(0x%x)/found(%d)",
                name, srcID, srcIt != mTP.end(), dstID, dstIt != mTP.end());
    }
}

NextImg::NextImg()
{
}

NextImg::NextImg(const BasicImg &img, const NextIO::NextAttr &attr)
    : mImg(img)
    , mAttr(attr)
{
}

NextImg::operator bool() const
{
    return isValid();
}

bool NextImg::isValid() const
{
    return mImg;
}

IImageBuffer* NextImg::getImageBufferPtr() const
{
    return mImg.getImageBufferPtr();
}

NextImgMap NextResult::getMasterMap() const
{
    auto it = mMap.find(mMasterID);
    return (it != mMap.end()) ? it->second : decltype(mMap)::mapped_type();
}

} // NSFeaturePipe
} // NSCamFeature
} // NSCam
