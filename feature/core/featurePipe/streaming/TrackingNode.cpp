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
 * MediaTek Inc. (C) 2021. All rights reserved.
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

#include "TrackingNode.h"
#include "StreamingFeature_Common.h"
#define PIPE_CLASS_TAG "TrackingNode"
#define PIPE_TRACE TrackingNode
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_TRACKING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
using NSCamHW::HwMatrix;
using NSCamHW::HwTransHelper;
using Utils::SensorData;
using Utils::SENSOR_TYPE_ACCELERATION;
using Utils::SensorProvider;
using namespace NSCam::NSIoPipe;

TrackingNode::TrackingNode(const char *name)
    : StreamingFeatureNode(name)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mAITrackingYuvIn);
    mAllocator = NULL;
    mTrackingImage.pImg = NULL;
    TRACE_FUNC_EXIT();
}

TrackingNode::~TrackingNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL TrackingNode::onData(DataID id, const RequestPtr &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data.mRequest->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if (id == ID_ROOT_TO_TRACKING)
    {
        mAITrackingYuvIn.enque(data);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL TrackingNode::onInit()
{
    TRACE_FUNC_ENTER();
    MBOOL ret        = MTRUE;
    StreamingFeatureNode::onInit();
    mDebugLog = property_get_bool("vendor.debug.TrackingNodeLog", MFALSE);
    mpSensorProvider = SensorProvider::createInstance(LOG_TAG);
    MUINT32 interval = 60;
    if( mpSensorProvider->enableSensor(SENSOR_TYPE_ACCELERATION, interval))
        CAM_ULOGMD("enable SensorProvider success");
    else
        CAM_ULOGME("Enable SensorProvider fail");

    if( mpSensorProvider->enableSensor(Utils::SENSOR_TYPE_GYRO, interval))
        CAM_ULOGMD("enable SensorProvider success");
    else
        CAM_ULOGME("Enable SensorProvider fail");

    std::vector<MUINT32> vSensorAll = mPipeUsage.getAllSensorIDs();
    for (int i = 0; i < vSensorAll.size(); i++)
    {
        mvp3AHal.emplace(vSensorAll[i], MAKE_Hal3A(vSensorAll[i], "TrackingNode"));
    }

    IImageBufferAllocator::ImgParam imgParam(640*480*2, 0);
    mKeptImage.pImg = mAllocator->alloc("TrackingNode", imgParam);
    if ( !mKeptImage.pImg->lockBuf( "TrackingNode", (eBUFFER_USAGE_HW_CAMERA_READ | eBUFFER_USAGE_SW_MASK)) )
    {
       MY_LOGE("lock Buffer failed");
    }
    else if(mKeptImage.pImg)
    {
        mKeptImage.AddrY = (MUINT8 *)mKeptImage.pImg->getBufVA(0);
        mKeptImage.pImg->syncCache(eCACHECTRL_FLUSH);
    }

    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL TrackingNode::onUninit()
{
    TRACE_FUNC_ENTER();
    mTrackingImage.pImg = NULL;
    if( mpSensorProvider != NULL)
    {
        mpSensorProvider->disableSensor(SENSOR_TYPE_ACCELERATION);
        mpSensorProvider = NULL;
    }

    if( mpSensorProvider != NULL)
    {
        mpSensorProvider->disableSensor(Utils::SENSOR_TYPE_GYRO);
        mpSensorProvider = NULL;
    }

    // 3A hal
    std::vector<MUINT32> vSensorAll = mPipeUsage.getAllSensorIDs();
    for (int i = 0; i < vSensorAll.size(); i++)
    {
        if (mvp3AHal.at(vSensorAll[i]))
        {
            mvp3AHal.at(vSensorAll[i])->destroyInstance("TrackingNode");
        }
    }
    mvp3AHal.clear();
    if(mKeptImage.pImg)
    {
        mKeptImage.pImg->unlockBuf("TrackingNode");
        mAllocator->free(mKeptImage.pImg.get());
        mKeptImage.pImg = NULL;
        mKeptImage.AddrY = NULL;
    }
    //
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL TrackingNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    Timer timer;
    timer.start();
    timer.stop();
    TRACE_FUNC("mWarpMapPool %s %d buf in %d ms", STR_ALLOCATE,
        mPipeUsage.getNumP2ABuffer(), timer.getElapsed());
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL TrackingNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL TrackingNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr request;
    CAM_TRACE_CALL();
    if (!waitAllQueue())
    {
        return MFALSE;
    }
    if (!mAITrackingYuvIn.deque(request))
    {
        MY_LOGE("Input Tracking YUV deque out of sync");
        return MFALSE;
    }
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;

    ITrackingData resultROI = ITrackingData(MRectF(MPointF(0, 0), MSizeF(0, 0)));
    request->mTimer.startTracking();
    markNodeEnter(request);
    TRACE_FUNC("Frame %d in TrackingNode", request->mRequestNo);
    MY_LOGD_IF(mDebugLog, "Frame %d in TrackingNode", request->mRequestNo);
    if(!makeTrackingImage(request))
    {
        MY_LOGE("makeTrackingImage fail, handleData default value");
        ITrackingData invalidROI = ITrackingData(MRectF(MPointF(0, 0), MSizeF(0, 0)));
        markNodeExit(request);
        request->mTimer.stopTracking();
        handleData(ID_TRACKING_TO_HELPER, HelperData(HelpReq(FeaturePipeParam::MSG_TRACKING_DONE), request));
        return MTRUE;
    }
    mAITracking.init(mTrackingImage, request->mMasterID);
    getInitROI(request, resultROI);
    if( mTrackingImage.mAIODInitial || mAITracking.mTouchTriggerHint)
    {
        ret = runAdditionalDetection(resultROI);
    }
    if(mTrackingImage.mNeedToKeepImg)
    {
        keepTrackingImage(mTrackingImage);
    }
    else if(mTrackingImage.mReplaceImg && mKeptImage.pImg.get())
    {
        mTrackingImage = mKeptImage;
    }

    MY_LOGD_IF(mDebugLog, "resultROI.mTouchTrigger: %d", resultROI.mTouchTrigger);

    ret &= calcROI(request, resultROI);
    MINT32 sensorRot = calculateRotateDegree();
    convertTrackingMeta(request, resultROI.mTrackingROI, mPipeUsage.supportTrackingFocus());
    markNodeExit(request);
    request->mTimer.stopTracking();
    if(ret)
    {
        handleData(ID_TRACKING_TO_HELPER, HelperData(HelpReq(FeaturePipeParam::MSG_TRACKING_DONE), request));
    }
    else
    {
        MY_LOGE_IF(mDebugLog, "calcROI fail,"
                " handleData default value");
        ITrackingData invalidROI = ITrackingData(MRectF(MPointF(0, 0), MSizeF(0, 0)));
        invalidROI.mTouchTrigger = resultROI.mTouchTrigger;

        handleData(ID_TRACKING_TO_HELPER, HelperData(HelpReq(FeaturePipeParam::MSG_TRACKING_DONE), request));
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL TrackingNode::keepTrackingImage(const TrackingImage& img)
{
   if ( mKeptImage.pImg.get() == 0 )
   {
       MY_LOGE("NULL Buffer");
       return MFALSE;
   }
   auto bufVa = mKeptImage.AddrY;

   if (img.planes == 1)
   {
       memcpy(bufVa, img.AddrY, img.w * img.h * 2);
   }

   mKeptImage.w                 = img.w;
   mKeptImage.h                 = img.h;
   mKeptImage.format            = img.format;
   mKeptImage.planes            = img.planes;
   mKeptImage.mTimeStamp        = img.mTimeStamp;
   mKeptImage.mSensorRotate     = img.mSensorRotate;
   mKeptImage.mAIODInitial      = img.mAIODInitial;
   mKeptImage.mTargetObjectType = img.mTargetObjectType;
   mKeptImage.mNeedToKeepImg    = MFALSE;
   mKeptImage.mReplaceImg       = MFALSE;

   return MTRUE;
}

MBOOL TrackingNode::runAdditionalDetection(ITrackingData &resultROI)
{
    MBOOL forceOutputResult;
    MBOOL useAIODTouchTarget;
    std::vector<AIOD_CATEGORY_ENUM> type;
    MBOOL ret;

    if(mPipeUsage.supportTrackingFocus())
    {
        forceOutputResult = MFALSE;
        useAIODTouchTarget = MTRUE;
        type.push_back((AIOD_CATEGORY_ENUM) (ALL));
    }
    ret = mAITracking.doObjectDetection(mTrackingImage, resultROI.mInitROI, type, forceOutputResult, useAIODTouchTarget);
    resultROI.mTouchTrigger = ret;
    return ret;
}

MBOOL TrackingNode::makeTrackingImage(const RequestPtr &request)
{
    const MUINT32 sensorID = request->mMasterID;
    const SFPSensorInput &sensorIn = request->getSensorInput(sensorID);
    IMetadata* InHalMetadata = sensorIn.mHalIn;

    sp<IImageBuffer> imgi = get(sensorIn.mAITrackingYuv);
    if(imgi == nullptr)
    {
        MY_LOGE("FDYuv imgi null");
        return MFALSE;
    }
    // dumpBuffer(imgi.get());
    mTrackingImage = TrackingImage();
    mTrackingImage.w = imgi->getImgSize().w;
    mTrackingImage.h = imgi->getImgSize().h;
    MY_LOGD_IF(mDebugLog, "FDYuv imgi size: %dx%d", imgi->getImgSize().w, imgi->getImgSize().h);
    mTrackingImage.planes = imgi->getPlaneCount();
    mTrackingImage.pImg = imgi;

    if(mTrackingImage.planes == 3)
    {
        mTrackingImage.AddrY = (MUINT8 *)imgi->getBufVA(0);
        mTrackingImage.AddrU = (MUINT8 *)imgi->getBufVA(1);
        mTrackingImage.AddrV = (MUINT8 *)imgi->getBufVA(2);
    }
    else if (mTrackingImage.planes == 1)
    {
        mTrackingImage.AddrY = (MUINT8 *)imgi->getBufVA(0);
        mTrackingImage.PAddrY = (MUINT8 *)imgi->getBufPA(0);
    }
    mTrackingImage.pImg->syncCache(eCACHECTRL_FLUSH);

    // get timestamp
    IMetadata::IEntry timeStampEntry = InHalMetadata->entryFor(MTK_P1NODE_FRAME_START_TIMESTAMP);
    MINT64 timestamp = timeStampEntry.itemAt(0, Type2Type< MINT64 >());
    MINT32 sensorRot = calculateRotateDegree();
    mTrackingImage.mTimeStamp = timestamp;
    MY_LOGD_IF(mDebugLog, "TrackingNode image timestamp: %ld", mTrackingImage.mTimeStamp);

    mTrackingImage.mSensorRotate = sensorRot;

    // force TrackingFocus using AIOD initial
    if( mPipeUsage.supportTrackingFocus() )
    {
        mTrackingImage.mAIODInitial = MTRUE;
    }
    return MTRUE;
}

MBOOL TrackingNode::getInitROI(const RequestPtr &request, ITrackingData &resultROI)
{
    const MUINT32 sensorID = request->mMasterID;
    const SFPSensorInput &sensorIn = request->getSensorInput(sensorID);
    IMetadata* InAppMetadata = sensorIn.mAppIn;
    IMetadata::IEntry AFTrigerEntry = InAppMetadata->entryFor(MTK_CONTROL_AF_TRIGGER);
    MUINT8 touchTrigger = AFTrigerEntry.itemAt(0, Type2Type< MUINT8 >());

    bool fdimageDump = ::property_get_bool("vendor.debug.fdimageDump", false);
    if(fdimageDump)
    {
        // dumpBuffer(mTrackingImage.pImg.get(), request->mRequestNo);
        MY_LOGD_IF(mDebugLog, "current frameNo: %d", request->mRequestNo);
        dumpBuffer(mTrackingImage.pImg.get(), mDumpFDIndex);
        mDumpFDIndex ++;
        mDumpFDIndex = mDumpFDIndex < MAX_DUMP_BUFFER? mDumpFDIndex : 0;
    }

    // get touch region from APP metadata
    {
        MINT32 iTouchLeft, iTouchTop, iTouchRight, iTouchBottom;
        if( mPipeUsage.supportTrackingFocus() )
        {
            MRect AFRegion;
            IMetadata::IEntry AFRegionEntry = InAppMetadata->entryFor(MTK_TRACKINGAF_REGION);
            if(!AFRegionEntry.isEmpty())
            {
                AFRegion = AFRegionEntry.itemAt(0, Type2Type<MRect>());
                iTouchLeft     = AFRegion.p.x;
                iTouchTop      = AFRegion.p.y;
                iTouchRight    = AFRegion.s.w;
                iTouchBottom   = AFRegion.s.h;
                MY_LOGD("Use TrackingFocus touch!");
            }
        }

        if( touchTrigger && mEnableDebugTouch )
        {
            MRect AFRegion;
            IMetadata::IEntry AFRegionEntry = InAppMetadata->entryFor(MTK_CONTROL_AF_REGIONS);
            if(!AFRegionEntry.isEmpty())
            {
                IMetadata::IEntry AFRegionEntry = InAppMetadata->entryFor(MTK_CONTROL_AF_REGIONS);
                iTouchLeft     = AFRegionEntry.itemAt(0, Type2Type< MINT32 >());
                iTouchTop      = AFRegionEntry.itemAt(1, Type2Type< MINT32 >());
                iTouchRight    = AFRegionEntry.itemAt(2, Type2Type< MINT32 >());
                iTouchBottom   = AFRegionEntry.itemAt(3, Type2Type< MINT32 >());
                MY_LOGD("Use default touch AF!");
            }
        }

        if(iTouchLeft == 0 && iTouchRight == 0 && iTouchTop == 0 && iTouchBottom == 0)
            return MTRUE;
        else
        {
            if(fdimageDump)
            {
                MY_LOGD_IF(mDebugLog, "current frameNo: %d", request->mRequestNo);
                dumpBuffer(mTrackingImage.pImg.get(), mDumpFDIndex, MTRUE);
            }
            MY_LOGD("touchTrigger! touch Left(%d) Top(%d) Right(%d) Bottom!(%d)",
                      iTouchLeft, iTouchTop, iTouchRight, iTouchBottom);
        }

        MRectF UIcropRegion;
        {
            IMetadata::IEntry entry = InAppMetadata->entryFor(MTK_SCALER_CROP_REGION);
            if( !entry.isEmpty() )
            {
                MRect UIcropRegionI = entry.itemAt(0, Type2Type<MRect>());
                UIcropRegion.p.x = UIcropRegionI.p.x;
                UIcropRegion.p.y = UIcropRegionI.p.y;
                UIcropRegion.s.w = UIcropRegionI.s.w;
                UIcropRegion.s.h = UIcropRegionI.s.h;
            }
        }

        auto peelOffEISMargin = [&](float& point, float ratio)
        {
            float dis = point - 0.5;
            dis = ratio * dis;
            point = 0.5 + dis;
        };

        float middleX = (float(iTouchLeft + iTouchRight)/2.0 - UIcropRegion.p.x) / UIcropRegion.s.w;
        float middleY = (float(iTouchTop + iTouchBottom)/2.0 - UIcropRegion.p.y) / UIcropRegion.s.h;
        float middleWidth = float(iTouchRight - iTouchLeft) / UIcropRegion.s.w;
        float middleHeight = float(iTouchBottom - iTouchTop) / UIcropRegion.s.h;
        MFLOAT eisWidthRatio = request->getVar<MFLOAT>(SFP_VAR::EIS_WIDTH_RATIO, 1.0);
        MFLOAT eisHeightRatio = request->getVar<MFLOAT>(SFP_VAR::EIS_HEIGHT_RATIO, 1.0);
        MY_LOGD_IF(mDebugLog, "eisWidthRatio, eisHeightRatio: %f, %f", eisWidthRatio, eisHeightRatio);
        if(eisWidthRatio > 0 && eisHeightRatio > 0)
        {
            peelOffEISMargin(middleX, eisWidthRatio);
            peelOffEISMargin(middleY, eisHeightRatio);
        }
        middleX *= mTrackingImage.w;
        middleY *= mTrackingImage.h;
        middleWidth *= (mTrackingImage.w * eisWidthRatio);
        middleHeight *= (mTrackingImage.h * eisHeightRatio);

        if(resultROI.mInitROI.s.w == 0 || resultROI.mInitROI.s.h == 0)
        {
            resultROI.mInitROI.p.x = middleX - middleWidth/2;
            resultROI.mInitROI.p.y = middleY - middleHeight/2;
            resultROI.mInitROI.s.w = middleWidth;
            resultROI.mInitROI.s.h = middleHeight;
        }

        MY_LOGD_IF(mDebugLog, "initROI: %f, %f, %fx%f",
                               resultROI.mInitROI.p.x,
                               resultROI.mInitROI.p.y,
                               resultROI.mInitROI.s.w,
                               resultROI.mInitROI.s.h);

        resultROI.mTouchTrigger = MTRUE;
    }

    return MTRUE;
}

MBOOL TrackingNode::calcROI(const RequestPtr &request, ITrackingData &resultROI, MBOOL resetTracking)
{
    MBOOL ret = MTRUE;
    const MUINT32 sensorID = request->mMasterID;
    const SFPSensorInput &sensorIn = request->getSensorInput(sensorID);
    IMetadata* InAppMetadata = sensorIn.mAppIn;
    MINT32 sensorRot = calculateRotateDegree();

    MY_LOGD_IF(mDebugLog, "sensorRot = %d", sensorRot);
    mAITracking.i4SensorID = request->mMasterID;
    TrackingResult result;
    result.mInitROI = resultROI.mInitROI;
    IMetadata::IEntry resetEntry;

    if( mPipeUsage.supportTrackingFocus() )
        resetEntry = InAppMetadata->entryFor(MTK_TRACKINGAF_CANCEL);

    MINT32 reset = resetEntry.itemAt(0, Type2Type< MINT32 >());
    reset |= (mAITracking.getDetectionSt() == S_AIOD_UNIFORM_AREA);

    ret = mAITracking.process(mTrackingImage, result, reset);

    resultROI.mResetTracking = reset;
    fillResultToROIdata(resultROI, result);

    mTrackingImage.pImg = NULL;
    return ret;
}

MVOID TrackingNode::fillResultToROIdata(ITrackingData &resultROI, TrackingResult &result)
{
    resultROI.mInitROI     = result.mInitROI;
    resultROI.mTrackingROI = result.mTrackingROI;
    resultROI.mCandiNum    = result.mTrustValueList.size();
    if( resultROI.mCandiNum != 0 )
    {
        resultROI.mCandiTracking  = result.mCandiTrackingROI;
        resultROI.mTrustValueList = result.mTrustValueList;
    }
}

MINT32 TrackingNode::calculateRotateDegree()
{
    SensorData acceDa;
    MINT32 PhoneDevRot = 0;
    MBOOL acceDaVd = mpSensorProvider->getLatestSensorData( SENSOR_TYPE_ACCELERATION, acceDa);

    if( acceDaVd && acceDa.timestamp)
    {
        float X,Y,Z;
        float OneEightyOverPi = 57.29577957855f;
        float magnitude;
        float angle;
        int ori = 0;
        X = -(acceDa.acceleration[0]);
        Y = -(acceDa.acceleration[1]);
        Z = -(acceDa.acceleration[2]);
        magnitude = X*X + Y*Y;
        if(magnitude * 4 >= Z*Z)
        {
            angle = atan2(-Y, X) * OneEightyOverPi;
            ori = 180 - round(angle);

            while( ori >= 360 ) {
                ori -= 360;
            }
            while( ori < 0 ) {
                ori += 360;
            }

            mAngle = ori;

            if( ori >= 45 && ori < 135 ) {
                ori = 90;
            } else if ( ori >= 135 && ori < 225 ) {
                ori = 180;
            } else if ( ori >= 225 && ori < 315 ) {
                ori = 270;
            } else
            {
                ori = 0;
            }
            PhoneDevRot = ori + mSensorRot;
        }
        else
        {
            MY_LOGW("magnitude too small, cannot trust");
        }
    }
    return PhoneDevRot;
}

MBOOL TrackingNode::calculateP1FDCrop(const RequestPtr &request, MRect& UIcropRegion)
{
    const SFPIOMap &generalIO = request->mSFPIOManager.getFirstGeneralIO();
    IMetadata* OutAppMetadata = generalIO.mAppOut;
    const MUINT32 sensorID = request->mMasterID;
    const SFPSensorInput &sensorIn = request->getSensorInput(sensorID);
    IMetadata* InHalMetadata = sensorIn.mHalIn;
    IMetadata* InAppMetadata = sensorIn.mAppIn;
    IImageBuffer *imgi = get(sensorIn.mRRZO);
    IMetadata::IEntry YUVCropEntry = InHalMetadata->entryFor(MTK_P1NODE_YUV_RESIZER1_CROP_REGION);
    IMetadata::IEntry P1CropEntry = InHalMetadata->entryFor(MTK_P1NODE_SENSOR_CROP_REGION);
    IMetadata::IEntry P1ResizeEntry = InHalMetadata->entryFor(MTK_P1NODE_RESIZER_SIZE);
    IMetadata::IEntry SensorModeEntry = InHalMetadata->entryFor(MTK_P1NODE_SENSOR_MODE);
    if (!YUVCropEntry.isEmpty() && !P1CropEntry.isEmpty() && !SensorModeEntry.isEmpty() && imgi)
    {
        MINT32 sensorMode = SensorModeEntry.itemAt(0, Type2Type<MINT32>());
        HwTransHelper trans(sensorID);
        HwMatrix Sensor2Active;
        if (!trans.getMatrixToActive(sensorMode, Sensor2Active))
        {
            return false;
        }
        MRect P1Crop = P1CropEntry.itemAt(0, Type2Type<MRect>());
        MRect FDYUVCrop = YUVCropEntry.itemAt(0, Type2Type<MRect>());
        MSize Rrzo = imgi->getImgSize();
        // map to P1 sensor domain
        FDYUVCrop.s.w = FDYUVCrop.s.w * P1Crop.s.w / Rrzo.w;
        FDYUVCrop.s.h = FDYUVCrop.s.h * P1Crop.s.h / Rrzo.h;
        FDYUVCrop.p.x = FDYUVCrop.p.x * P1Crop.s.w / Rrzo.w + P1Crop.p.x;
        FDYUVCrop.p.y = FDYUVCrop.p.y * P1Crop.s.h / Rrzo.h + P1Crop.p.y;
        Sensor2Active.transform(FDYUVCrop, UIcropRegion);
        return true;
    }
    return false;
}

MVOID TrackingNode::convertTrackingMeta(const RequestPtr &request, MRectF resultROI, MBOOL needToUpdateMeta)
{
    const SFPIOMap &generalIO = request->mSFPIOManager.getFirstGeneralIO();
    IMetadata* OutAppMetadata = generalIO.mAppOut;
    const MUINT32 sensorID = request->mMasterID;
    const SFPSensorInput &sensorIn = request->getSensorInput(sensorID);
    IMetadata* InAppMetadata = sensorIn.mAppIn;
    IMetadata::IEntry entry = InAppMetadata->entryFor(MTK_SCALER_CROP_REGION);
    MRectF UIcropRegion;
    if( !entry.isEmpty() )
    {
        MRect UIcropRegionI = entry.itemAt(0, Type2Type<MRect>());
        calculateP1FDCrop(request, UIcropRegionI);
        UIcropRegion.p.x = UIcropRegionI.p.x;
        UIcropRegion.p.y = UIcropRegionI.p.y;
        UIcropRegion.s.w = UIcropRegionI.s.w;
        UIcropRegion.s.h = UIcropRegionI.s.h;
    }

    auto plusEISMargin = [&](float& point, float ratio)
    {
        float dis = point - 0.5;
        dis = dis / ratio;
        point = 0.5 + dis;
    };

    MFLOAT eisWidthRatio = request->getVar<MFLOAT>(SFP_VAR::EIS_WIDTH_RATIO, 1.0);
    MFLOAT eisHeightRatio = request->getVar<MFLOAT>(SFP_VAR::EIS_HEIGHT_RATIO, 1.0);
    plusEISMargin(resultROI.p.x, eisWidthRatio);
    plusEISMargin(resultROI.p.y, eisHeightRatio);
    resultROI.s.w = resultROI.s.w / eisWidthRatio;
    resultROI.s.h = resultROI.s.h / eisHeightRatio;

    MRectF UIdptzroi;
    UIdptzroi.p.x = resultROI.p.x * UIcropRegion.s.w + UIcropRegion.p.x;
    UIdptzroi.p.y = resultROI.p.y * UIcropRegion.s.h + UIcropRegion.p.y;
    UIdptzroi.s.w = resultROI.s.w * UIcropRegion.s.w + UIdptzroi.p.x;
    UIdptzroi.s.h = resultROI.s.h * UIcropRegion.s.h + UIdptzroi.p.y;

    // update noobject meta
    if(mAITracking.getDetectionSt() == S_AIOD_UNIFORM_AREA)
    {
        mAITracking.resetDetectionSt();
        MY_LOGD("send toast, reqNo: %d", request->mRequestNo);
        IMetadata::IEntry noObject_tag(MTK_TRACKINGAF_NOOBJECT);
        MINT32 noObject = 1;
        noObject_tag.push_back(noObject, Type2Type<MINT32>());
        OutAppMetadata->update(MTK_TRACKINGAF_NOOBJECT, noObject_tag);
        if(needToUpdateMeta)
            InAppMetadata->update(MTK_TRACKINGAF_NOOBJECT, noObject_tag);
    }

    if(UIdptzroi.p.x == 0 && UIdptzroi.p.y == 0 &&
       UIdptzroi.s.w == UIcropRegion.s.w &&
       UIdptzroi.s.h == UIcropRegion.s.h)
    {
        MY_LOGE_IF(mDebugLog, "bad ROI data: %f, %f, %f, %f", UIdptzroi.p.x, UIdptzroi.p.y, UIdptzroi.s.w, UIdptzroi.s.h);
        return;
    }

    // update Tracking Rect meta
    MRect _trackingRect;
    {
        IMetadata::IEntry tracking_rect_tag(MTK_TRACKINGAF_TARGET);
        IMetadata::IEntry face_rect_tag(MTK_STATISTICS_FACE_RECTANGLES);
        _trackingRect.p.x = UIdptzroi.p.x;
        _trackingRect.p.y = UIdptzroi.p.y;
        _trackingRect.s.w = UIdptzroi.s.w;
        _trackingRect.s.h = UIdptzroi.s.h;
        tracking_rect_tag.push_back(_trackingRect, Type2Type<MRect>());
        if(needToUpdateMeta)
            InAppMetadata->update(MTK_TRACKINGAF_TARGET, tracking_rect_tag);
    }

    // set AF
    {
        NS3Av3::OTInfo_T ot_info;
        ot_info.enable = true;
        ot_info.left   = _trackingRect.p.x;
        ot_info.top    = _trackingRect.p.y;
        ot_info.right  = _trackingRect.s.w;
        ot_info.bottom = _trackingRect.s.h;
        auto& p3AHal = mvp3AHal.at(sensorID);
        p3AHal->send3ACtrl(NS3Av3::E3ACtrl_OTFeature4AF, (MINTPTR)&ot_info, 0);
    }
}

MVOID TrackingNode::dumpBuffer(IImageBuffer* dumpImage,
                               MUINT32 requestNo,
                               MBOOL templateImageBool)
{
    if( !dumpImage )
    {
        MY_LOGE("dump buffer fail, dumpImage null");
    }

    char filename[256] = {0};
    TuningUtils::FILE_DUMP_NAMING_HINT hint;
    hint.UniqueKey = 0;
    hint.RequestNo = requestNo;
    hint.FrameNo   = requestNo;

    auto DumpRawBuffer = [&](IImageBuffer* pImgBuf) -> MVOID
    {
        // extract(&hint, pImgBuf);
        // genFileName_YUV(filename, sizeof(filename), &hint, YUV_PORT_CRZO1);
        char requestNoChar[5];
        char subFilename[] = "_640x416_640x474.yuy2";
        char templateImage[] = "templateImage";
        char fileDirectory[] = "/data/vendor/camera_dump/";
        sprintf(requestNoChar, "%d", requestNo);
        strncat(filename, fileDirectory, strlen(fileDirectory));
        if(templateImageBool)
        {
            strncat(filename, templateImage, strlen(templateImage));
        }
        else
        {
            strncat(filename, requestNoChar, strlen(requestNoChar));
        }
        strncat(filename, subFilename, strlen(subFilename));
        pImgBuf->saveToFile(filename);
        MY_LOGD_IF(mDebugLog, "Dump image: %s", filename);
    };
    MY_LOGD_IF(mDebugLog, "inbuffer size: %d x %d, format: 0x%x",
                           dumpImage->getImgSize().w,
                           dumpImage->getImgSize().h,
                           dumpImage->getImgFormat());
    DumpRawBuffer(dumpImage);
}

/******************************************************************************
*
******************************************************************************/
AITracking::AITracking()
{}

MVOID AITracking::init(const TrackingImage &trackingImage, MUINT32 sensorId)
{
    if ( pAIOTProc == NULL )
    {
        i4SensorID = sensorId;
        gInitInfo.image_info.i4image_width = trackingImage.w;
        gInitInfo.image_info.i4image_height = trackingImage.h;
        gInitInfoD.image_info.i4image_width = trackingImage.w;
        gInitInfoD.image_info.i4image_height = trackingImage.h;
        gInitInfo.eimage_type  = AIOT_IMG_TYPE_YUYV;
        gInitInfoD.eimage_type = AIOD_IMG_TYPE_YUYV;
        MY_LOGD("OThal::createInstance() +");
        pAIOTProc = OThal::createInstance("TrackingNode");
        pAIOTProc->ObjectTrackingInit(&gInitInfo, i4SensorID);
        pAIOTProc->ObjectDectectionInit(&gInitInfoD);
        MY_LOGD("OThal::createInstance() -");
    }
}

AITracking::~AITracking()
{
    if(pAIOTProc)
    {
        pAIOTProc->ObjectTrackingReset();
        pAIOTProc->destroyInstance(pAIOTProc);
        pAIOTProc = NULL;
    }
    mStartTracking = MFALSE;
    mTouchTriggerHint = MFALSE;
    AFTrigger = MFALSE;
}

MBOOL AITracking::process(const TrackingImage &trackingImage,
                          TrackingResult& result,
                          MBOOL resetAITracking)
{
    MY_LOGD_IF(mDebugLog, "AITracking process +");
    MY_LOGD_IF(mDebugLog, "initROI: %f, %f, %fx%f",
               result.mInitROI.p.x, result.mInitROI.p.y,
               result.mInitROI.s.w, result.mInitROI.s.h);
    MBOOL ret = MTRUE;
    if(!this->prepareInput(trackingImage, result.mInitROI))
        return MFALSE;

    // don't do aitracking
    if( resetAITracking )
    {
        mStartTracking = MFALSE;
        AFTrigger = MFALSE;
        gResultInfo.sOutSmoothBBox.fcx     = gInitInfo.image_info.i4image_width / 2.0;
        gResultInfo.sOutSmoothBBox.fwidth  = gInitInfo.image_info.i4image_width;
        gResultInfo.sOutSmoothBBox.fcy     = gInitInfo.image_info.i4image_height / 2.0;
        gResultInfo.sOutSmoothBBox.fheight = gInitInfo.image_info.i4image_height;
        MY_LOGD_IF(mDebugLog, "reset AITracking");
    }

    if(pAIOTProc == nullptr)
    {
        MY_LOGD_IF(mDebugLog, "pAIOTProc null");
        return MFALSE;
    }

    if(mStartTracking)
    {
        ret = pAIOTProc->DoObjectTracking(&gProcIn, &gResultInfo);
        if(ret)
            return MFALSE;
        gProcIn.image_info.pimage_input = 0;
    }

    if((ret = this->isValidOut()) == MTRUE)
    {
        fillResult(result);
    }

    MY_LOGD_IF(mDebugLog, "AITracking process -");
    return ret;
}

MBOOL AITracking::touchTrigger(const MRectF &initROI)
{
    MBOOL ret = MFALSE;
    if(gProcIn.image_info.pimage_input != 0)
    {
        // gProcIn.i4TempImage_timestamp = 0;
        MINT32 setRet = pAIOTProc->SetObjectTrackingTarget(&gProcIn);
        MY_LOGD_IF(mDebugLog, "SetObjectTrackingTarget result: %d", setRet);
        ret = (setRet == S_AIOT_OK);
        if(!ret)   return MFALSE;
        gProcIn.sObjBox.fcx     = 0;
        gProcIn.sObjBox.fcy     = 0;
        gProcIn.sObjBox.fwidth  = 0;
        gProcIn.sObjBox.fheight = 0;
    }
    else
        MY_LOGW_IF(mDebugLog, "gProcIn.pvInput NULL!");
    return ret;
}

MBOOL AITracking::prepareInput(const TrackingImage &trackingImage, const MRectF &initROI)
{
    MY_LOGD_IF(mDebugLog, "prepareInput +");
    MY_LOGD_IF(mDebugLog, "initROI: %f, %f, %fx%f", initROI.p.x, initROI.p.y, initROI.s.w, initROI.s.h);
    if(!trackingImage.pImg)
    {
        MY_LOGE("input image null");
        return MFALSE;
    }

    gInitInfo.image_info.i4image_width = trackingImage.w;
    gInitInfo.image_info.i4image_height = trackingImage.h;
    gInitInfoD.image_info.i4image_width = trackingImage.w;
    gInitInfoD.image_info.i4image_height = trackingImage.h;

    gInitInfo.eimage_type = AIOT_IMG_TYPE_YUYV;
    gInitInfoD.eimage_type = AIOD_IMG_TYPE_YUYV;

    if (pAIOTProc == NULL)
    {
        MY_LOGE("pAIOTProc == NULL");
        return MFALSE;
    }

    gProcIn.image_info.pimage_input = (MUINT8 *)trackingImage.AddrY;
    gProcIn.image_info.i4rotate = trackingImage.mSensorRotate;
    gProcIn.image_info.i4image_timestamp = trackingImage.mTimeStamp;
    gProcIn.image_info.i4image_width = trackingImage.w;
    gProcIn.image_info.i4image_height = trackingImage.h;

    if((initROI.s.w > 0 && initROI.s.h > 0))
    {
        AFTrigger = MTRUE;
        gProcIn.i4ObjType = trackingImage.mTargetObjectType;
        gProcIn.sObjBox.fcx = initROI.p.x + initROI.s.w / 2.0;
        gProcIn.sObjBox.fcy = initROI.p.y + initROI.s.h / 2.0;
        gProcIn.sObjBox.fwidth = initROI.s.w;
        gProcIn.sObjBox.fheight = initROI.s.h;

        gResultInfod.sTargetBBox.fcx     = initROI.p.x + initROI.s.w / 2.0;
        gResultInfod.sTargetBBox.fcy     = initROI.p.y + initROI.s.h / 2.0;
        gResultInfod.sTargetBBox.fwidth  = initROI.s.w;
        gResultInfod.sTargetBBox.fheight = initROI.s.h;
        if( !touchTrigger(initROI) )
        {
            return MFALSE;
        }
        mStartTracking = MTRUE;
    }
    MY_LOGD_IF(mDebugLog, "prepareInput -");
    return MTRUE;
}

MBOOL AITracking::fillResult(TrackingResult& result)
{
    result.mTrackingROI.p.x = mNormalizedROI.x_min;
    result.mTrackingROI.p.y = mNormalizedROI.y_min;
    result.mTrackingROI.s.w = mNormalizedROI.x_max - mNormalizedROI.x_min;
    result.mTrackingROI.s.h = mNormalizedROI.y_max - mNormalizedROI.y_min;
    MINT32 iAvailableNum = gResultInfo.i4AvailableNum;
    for( int i = 0; i < iAvailableNum; i++ )
    {
        sBBox _box = gResultInfo.sOutBBoxList[i];
        MRectF _TrackingROI;
        _TrackingROI.p.x = (_box.fcx - _box.fwidth/2.0)  / gInitInfo.image_info.i4image_width;
        _TrackingROI.p.y = (_box.fcy - _box.fheight/2.0) / gInitInfo.image_info.i4image_height;
        _TrackingROI.s.w =        _box.fwidth            / gInitInfo.image_info.i4image_width;
        _TrackingROI.s.h =        _box.fheight           / gInitInfo.image_info.i4image_height;

        result.mCandiTrackingROI.push_back(_TrackingROI);
        result.mTrustValueList.push_back(gResultInfo.fTrustValueList[i]);
    }
    return MTRUE;
}

MBOOL AITracking::isValidOut()
{
    MBOOL ret = MFALSE;
    auto inRange = [](float in) -> MBOOL
    {
        if( in >= 0 && in <= 1)
            return MTRUE;
        else
            return MFALSE;
    };
    float _fwidth  = gResultInfo.sOutSmoothBBox.fwidth;
    float _fheight = gResultInfo.sOutSmoothBBox.fheight;
    float _iwidth  = gInitInfo.image_info.i4image_width;
    float _iheight = gInitInfo.image_info.i4image_height;
    mNormalizedROI.x_min = (gResultInfo.sOutSmoothBBox.fcx - _fwidth/2.0)  / _iwidth;
    mNormalizedROI.x_max = (gResultInfo.sOutSmoothBBox.fcx + _fwidth/2.0)  / _iwidth;
    mNormalizedROI.y_min = (gResultInfo.sOutSmoothBBox.fcy - _fheight/2.0) / _iheight;
    mNormalizedROI.y_max = (gResultInfo.sOutSmoothBBox.fcy + _fheight/2.0) / _iheight;

    MY_LOGD_IF(mDebugLog, "mNormalizedROI.x_min: %f, mNormalizedROI.x_max: %f, mNormalizedROI.y_min: %f, mNormalizedROI.y_max: %f",
             mNormalizedROI.x_min, mNormalizedROI.x_max, mNormalizedROI.y_min, mNormalizedROI.y_max);

    if(gResultInfo.fTrustValue <= mTrustValueTh)
    {
        mNormalizedROI.x_min = 0.0;
        mNormalizedROI.x_max = 1.0;
        mNormalizedROI.y_min = 0.0;
        mNormalizedROI.y_max = 1.0;
        MY_LOGD_IF(mDebugLog, "trust value(%f) too small, lost tracking", gResultInfo.fTrustValue);
    }

    if(    inRange(mNormalizedROI.x_min)
        && inRange(mNormalizedROI.x_max)
        && inRange(mNormalizedROI.y_min)
        && inRange(mNormalizedROI.y_max) )
    {
        ret = MTRUE;
    }
    else
        ret = MFALSE;

    return ret;
}

MBOOL AITracking::doObjectDetection(TrackingImage &trackingImage,
                                    MRectF &initROI,
                                    std::vector<AIOD_CATEGORY_ENUM> type,
                                    MBOOL forceOutputDefault,
                                    MBOOL useTouchTarget)
{
    if (pAIOTProc == NULL)
    {
        MY_LOGE("pAIOTProc == NULL");
        return MFALSE;
    }
    if(!trackingImage.pImg)
    {
        MY_LOGE("input image null");
        return MFALSE;
    }

    if((initROI.s.w > 0 && initROI.s.h > 0) || mTouchTriggerHint)
    {
        if(mTouchTriggerHint && !(initROI.s.w > 0 && initROI.s.h > 0))
        {}
        else
        {
            mTouchTriggerHint = MTRUE;
            gProcInD.image_info.pimage_input = (MUINT8 *)trackingImage.AddrY;
            gProcInD.touch_info.ftouch_x = initROI.p.x + initROI.s.w / 2.0;
            gProcInD.touch_info.ftouch_y = initROI.p.y + initROI.s.h / 2.0;
            mBackupInitROI = initROI;
            initROI.p.x = 0.0;
            initROI.p.y = 0.0;
            initROI.s.w = 0.0;
            initROI.s.h = 0.0;
            gResultInfod.vCandiBBox.clear();
            trackingImage.mNeedToKeepImg = MTRUE;
        }

        if( mTouchTriggerHint )
        {
            gProcInD.image_info.i4image_width = trackingImage.w;
            gProcInD.image_info.i4image_height = trackingImage.h;
            gProcInD.image_info.i4rotate = trackingImage.mSensorRotate;
            mDetectionSt = (pAIOTProc->DoObjectDectection(&gProcInD, &gResultInfod, AIOD_TOUCH_PROC));
            MY_LOGD_IF(mDebugLog, "object detection result: %d", mDetectionSt);
            MBOOL ret = (mDetectionSt == S_AIOD_OK);

            // if no object, stop doing object detection for this touch hint
            if(mDetectionSt == S_AIOD_UNIFORM_AREA)
            {
                mTouchTriggerHint = MFALSE;
                if(forceOutputDefault)
                {
                    initROI = mBackupInitROI;
                    mDetectionSt = S_AIOD_OK;
                }
                else
                {
                    initROI.p.x = 0.0;
                    initROI.p.y = 0.0;
                    initROI.s.w = 0.0;
                    initROI.s.h = 0.0;
                }
                MY_LOGD_IF(mDebugLog, "AIOD initROI, use APP init: %f, %f, %fx%f",
                                       initROI.p.x,
                                       initROI.p.y,
                                       initROI.s.w,
                                       initROI.s.h);
                return MTRUE;
            }

            if(!ret)   return MFALSE;

            initROI = mBackupInitROI;
            MINT32 targetObjectType = -1;
            // use touch target selected by AIObject Detection
            if( useTouchTarget )
            {
                initROI.p.x = gResultInfod.sTargetBBox.fcx - gResultInfod.sTargetBBox.fwidth/2.0;
                initROI.p.y = gResultInfod.sTargetBBox.fcy - gResultInfod.sTargetBBox.fheight/2.0;
                initROI.s.w = gResultInfod.sTargetBBox.fwidth;
                initROI.s.h = gResultInfod.sTargetBBox.fheight;
                targetObjectType = gResultInfod.sTargetBBox.i4TargetType;
            }
            else
            {
                MPointF touchCenter;
                touchCenter.x = mBackupInitROI.p.x + mBackupInitROI.s.w/2.0;
                touchCenter.y = mBackupInitROI.p.y + mBackupInitROI.s.h/2.0;
                auto allObj = gResultInfod.vCandiBBox;
                MBOOL findCandidate = MFALSE;
                for( int i = 0; i < allObj.size(); i++)
                {
                    if(allObj[i].fwidth != 0)
                    {
                        MBOOL typeChecked = (std::find(type.begin(), type.end(), allObj[i].i4TargetType) != type.end());
                        typeChecked |= (std::find(type.begin(), type.end(), (AIOD_CATEGORY_ENUM)ALL) != type.end());
                        if( !typeChecked )
                            continue;
                        MRect _temp;
                        _temp.p.x = allObj[i].fcx - allObj[i].fwidth/2.0;
                        _temp.p.y = allObj[i].fcy - allObj[i].fheight/2.0;
                        _temp.s.w = allObj[i].fwidth;
                        _temp.s.h = allObj[i].fheight;

                        if(touchCenter.x > _temp.p.x && touchCenter.x < _temp.p.x + _temp.s.w
                           && touchCenter.y > _temp.p.y && touchCenter.y < _temp.p.y + _temp.s.h)
                        {
                            MY_LOGD_IF(mDebugLog, "use this detection as initial value: %d, %d, %dx%d",
                            _temp.p.x,
                            _temp.p.y,
                            _temp.s.w,
                            _temp.s.h);
                            initROI.p.x = _temp.p.x;
                            initROI.p.y = _temp.p.y;
                            initROI.s.w = _temp.s.w;
                            initROI.s.h = _temp.s.h;
                            findCandidate = MTRUE;
                            targetObjectType = allObj[i].i4TargetType;
                            break;
                        }
                    }
                }
                if(!findCandidate)
                {
                    MY_LOGW("can not find a proper candidate, use backup init");
                }
            }
            // gResultInfod.vCandiBBox.clear();

            if(!(initROI.p.x > 0 && initROI.p.y > 0 && initROI.s.w && initROI.s.h))
            {
                MY_LOGE("something wrong for object detection!: %f, %f, %f, %f",
                        initROI.p.x, initROI.p.y, initROI.s.w, initROI.s.h);
                return MFALSE;
            }
            trackingImage.mTargetObjectType = targetObjectType;
        }
        mTouchTriggerHint = MFALSE;
        trackingImage.mReplaceImg = MTRUE;
        MY_LOGD_IF(mDebugLog, "AIOD initROI: %f, %f, %fx%f, targetType: %d",
                               initROI.p.x,
                               initROI.p.y,
                               initROI.s.w,
                               initROI.s.h,
                               trackingImage.mTargetObjectType);
    }
    return MTRUE;
}

MINT32 AITracking::getDetectionSt()
{
    return mDetectionSt;
}

MVOID AITracking::resetDetectionSt()
{
    mDetectionSt = S_AIOD_OK;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam