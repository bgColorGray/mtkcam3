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

#include <chrono>
#include <thread>

#include "P2_Param.h"
#include "P2_CaptureProcessor.h"
#include "P2_DebugControl.h"

using namespace NSCam;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::NSCamFeature::NSFeaturePipe::NSCapture;
using namespace NSCam::Utils::ULog;

#define P2_CAPTURE_THREAD_NAME "p2_capture"
#define INVALID_SENSOR_ID (MUINT32)(-1)

namespace P2
{

#include "P2_DebugControl.h"
#define P2_CLASS_TAG    ReleaseBundle
#define P2_TRACE        TRACE_RELEASE_BUNDLE
#include "P2_LogHeader.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_P2_CAP_PROC);

class ReleaseBundle : public virtual RefBase
{
public:
    ReleaseBundle(const char* pName, MINT32 requestNo, MINT32 frameNo, CaptureProcessor* pProcessor, const ILog& log, MBOOL isOutput = MFALSE)
        : mName(pName ? pName : "")
        , mRequestNo(requestNo)
        , mFrameNo(frameNo)
        , mIsOutput(isOutput)
        , mpProcessor(pProcessor)
        , mLog(log)
    {
        TRACE_S_FUNC_ENTER(mLog, "name:%s, R/F Num:%d/%d",mName.c_str(), mRequestNo, mFrameNo);
        TRACE_S_FUNC_EXIT(mLog, "name:%s, R/F Num:%d/%d",mName.c_str(), mRequestNo, mFrameNo);
    }

    ~ReleaseBundle()
    {
        MY_S_LOGI_IF(mLog.getLogLevel(), mLog, "+, name:%s, R/F Num:%d/%d, isOutput:%d",mName.c_str(), mRequestNo, mFrameNo, mIsOutput);
        if (mpProcessor != NULL) {
            mpProcessor->notifyRelease(mRequestNo, mFrameNo, mIsOutput);
        }
        MY_S_LOGI_IF(mLog.getLogLevel(), mLog, "-, name:%s, R/F Num:%d/%d",mName.c_str(), mRequestNo, mFrameNo);
    };

private:
    const std::string       mName;
    const MINT32            mRequestNo;
    const MINT32            mFrameNo;
    const MBOOL             mIsOutput;

    CaptureProcessor*       mpProcessor;
    ILog                    mLog;
};

#include "P2_DebugControl.h"
#define P2_CLASS_TAG    P2BufferHandle
#define P2_TRACE        TRACE_P2_BUFFER_HANDLE
#include "P2_LogHeader.h"

class P2BufferHandle : public virtual BufferHandle
{
public:
    P2BufferHandle(const sp<P2Request>& pRequest, ID_IMG id, sp<ReleaseBundle> pBundle, CaptureProcessor* pProcessor, const ILog& log)
        : mpRequest(pRequest)
        , mpP2Img(NULL)
        , mImgId(id)
        , mpImageBuffer(NULL)
        , mpReleaseBundle(pBundle)
        , mpProcessor(pProcessor)
        , mLog(log)
    {
        TRACE_S_FUNC_ENTER(mLog, "imgId:0x%" PRIx32, mImgId);
        TRACE_S_FUNC_EXIT(mLog);
    }

    P2BufferHandle(const sp<P2Request> pRequest, sp<P2Img> pP2Img, CaptureProcessor* pProcessor)
        : mpRequest(pRequest)
        , mpP2Img(pP2Img)
        , mImgId(OUT_YUV)
        , mpImageBuffer(NULL)
        , mpProcessor(pProcessor)
    {
        TRACE_S_FUNC_ENTER(mLog, "imgId:0x%" PRIx32, mImgId);
        TRACE_S_FUNC_EXIT(mLog);
    }

    virtual MERROR acquire(MINT usage)
    {
        (void)usage;
        TRACE_S_FUNC_ENTER(mLog, "imgId:0x%" PRIx32 "usage:0x%" PRIx32, mImgId, usage);
        if (mImgId == OUT_YUV) {
            mpP2Img->updateResult(MTRUE);
            mpImageBuffer = mpP2Img->getIImageBufferPtr();
            mvpImageBuffer = mpP2Img->getIImageBufferPtrs();
        }
        else if (mpRequest->isValidImg(mImgId)) {
            mpP2Img = mpRequest->getImg(mImgId);
            mpP2Img->updateResult(MTRUE);
            mpImageBuffer = mpP2Img->getIImageBufferPtr();
            mvpImageBuffer = mpP2Img->getIImageBufferPtrs();
        }
        else {
            MY_S_LOGW(mLog, "failed to acquire, imgId:0x%" PRIx32, mImgId);
            return BAD_VALUE;
        }
        TRACE_S_FUNC_EXIT(mLog);
        return OK;
    }

    virtual IImageBuffer* native()
    {
        return mpImageBuffer;
    }

    virtual std::vector<IImageBuffer*> nativeAll()
    {
        return mvpImageBuffer;
    }

    virtual MBOOL isMultiBuffer()
    {
        if (mvpImageBuffer.size() > 1) {
            return MTRUE;
        }
        return MFALSE;
    }

    virtual void release()
    {
        TRACE_S_FUNC_ENTER(mLog, "imgId:0x%" PRIx32, mImgId);
        mpImageBuffer = NULL;
        mvpImageBuffer.clear();
        mpP2Img = NULL;

        if (mpRequest != NULL) {
            // For now, do not support OUT_YUV early release
            if (mImgId != OUT_YUV && mImgId != OUT_JPEG_YUV && mImgId != OUT_RAW) {
                mpProcessor->releaseImage(mpRequest, mImgId);
            }
            mpReleaseBundle = NULL;
            mpRequest = NULL;
        }

        TRACE_S_FUNC_EXIT(mLog);
    }

    virtual MUINT32 getTransform()
    {
        if (mpP2Img == NULL)
            return 0;
        return mpP2Img->getTransform();
    }

    virtual ~P2BufferHandle()
    {
        TRACE_S_FUNC_ENTER(mLog, "imgId:0x%" PRIx32 " ", mImgId);
        if (mpRequest != NULL) {
            MY_S_LOGD_IF(mLog.getLogLevel(), mLog, "buffer(0x%" PRIx32 ") not released", mImgId);
            release();
        }
        TRACE_S_FUNC_EXIT(mLog);
    }

private:
    sp<P2Request>               mpRequest;
    sp<P2Img>                   mpP2Img;
    ID_IMG                      mImgId;
    IImageBuffer*               mpImageBuffer;
    std::vector<IImageBuffer*>  mvpImageBuffer;
    sp<ReleaseBundle>           mpReleaseBundle;
    CaptureProcessor*           mpProcessor;
    ILog                        mLog;
};

#include "P2_DebugControl.h"
#define P2_CLASS_TAG    P2MetadataHandle
#define P2_TRACE        TRACE_P2_METADATA_HANDLE
#include "P2_LogHeader.h"

class P2MetadataHandle : public virtual MetadataHandle
{
public:
    P2MetadataHandle(const sp<P2Request>& pRequest, ID_META id, CaptureProcessor* pProcessor, const ILog& log)
        : mpRequest(pRequest)
        , mpP2Meta(NULL)
        , mMetaId(id)
        , mpMetadata(NULL)
        , mpProcessor(pProcessor)
        , mLog(log)
    {
        TRACE_S_FUNC_ENTER(mLog, "metaId:0x%" PRIx32, mMetaId);
        TRACE_S_FUNC_EXIT(mLog);
    }

    virtual MERROR acquire()
    {
        TRACE_S_FUNC_ENTER(mLog, "metaId:0x%" PRIx32, mMetaId);
        if (mpRequest->isValidMeta(mMetaId)) {
            mpP2Meta = mpRequest->getMeta(mMetaId);
            mpP2Meta->updateResult(MTRUE);
            mpMetadata = mpP2Meta->getIMetadataPtr();
        }
        else
        {
            MY_S_LOGW(mLog, "falied to acquire, metaId:0x%" PRIx32, mMetaId);
            return BAD_VALUE;
        }
        TRACE_S_FUNC_EXIT(mLog);
        return OK;
    }

    virtual IMetadata* native()
    {
        return mpMetadata;
    }

    virtual void release()
    {
        MY_S_LOGI_IF(mLog.getLogLevel(), mLog, "release metaId:0x%" PRIx32, mMetaId);
        TRACE_S_FUNC_ENTER(mLog, "metaId:0x%" PRIx32, mMetaId);
        mpMetadata = NULL;
        mpP2Meta = NULL;
        if( mpRequest != NULL ) {
            // For now, do not support app output meta early release
            if (mMetaId != OUT_APP && mMetaId != OUT_APP_PHY && mMetaId != OUT_APP_PHY_2) {
                mpProcessor->releaseMeta(mpRequest, mMetaId);
            }
            mpRequest = NULL;
        }
        TRACE_S_FUNC_EXIT(mLog);
    }

    virtual ~P2MetadataHandle()
    {
        MY_S_LOGI_IF(mLog.getLogLevel(), mLog, "dtor metaId:0x%" PRIx32 " ", mMetaId);
        TRACE_S_FUNC_ENTER(mLog, "metaId:0x%" PRIx32 " ", mMetaId);
        if (mpRequest != NULL) {
            MY_S_LOGW(mLog, "metadata(%d) not released", mMetaId);
            release();
        }
        TRACE_S_FUNC_EXIT(mLog);
    }

private:
    sp<P2Request>     mpRequest;
    sp<P2Meta>        mpP2Meta;
    ID_META           mMetaId;
    IMetadata*        mpMetadata;
    CaptureProcessor* mpProcessor;
    ILog              mLog;
};

#include "P2_DebugControl.h"
#define P2_CLASS_TAG    CaptureProcessor
#define P2_TRACE        TRACE_CAPTURE_PROCESSOR
#include "P2_LogHeader.h"

CaptureProcessor::CaptureProcessor()
    : Processor(P2_CAPTURE_THREAD_NAME)
    , mAbortingRequestNo(-1)
    , mLastRequestNo(-1)
    , mLastFrameCount(0)
    , mLastFrameIndex(0)
    , mLastMasterIndex(INVALID_SENSOR_ID)
    , mLastSlaveIndex(INVALID_SENSOR_ID)
{
    MY_LOG_FUNC_ENTER();
    mDebugDrop = property_get_int32("vendor.debug.camera.p2c.drop", 0xFF);
    mNextCaptureCB = std::make_pair(-1, false);
    MY_LOGI("(%p) ctor", this);
    MY_LOG_FUNC_EXIT();
}

CaptureProcessor::~CaptureProcessor()
{
    TRACE_S_FUNC_ENTER(mLog);
    MY_LOGI("(%p) dtor", this);
    this->uninit();
    TRACE_S_FUNC_EXIT(mLog);
}

MBOOL CaptureProcessor::onInit(const P2InitParam &param)
{
    ILog log = param.mP2Info.mLog;
    TRACE_S_FUNC_ENTER(log);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:init()");

    MBOOL ret = MTRUE;
    mP2Info = param.mP2Info;
    mLog = mP2Info.mLog;
    MY_S_LOGI(mLog, "(%p) uniqueKey:%d", this, mP2Info.getConfigInfo().mUsageHint.mPluginUniqueKey);

    P2_CAM_TRACE_BEGIN(TRACE_DEFAULT, "P2_Capture:FeaturePipe create & init");
    const MBOOL isDualCam = (mP2Info.getConfigInfo().mAllSensorID.size() > 1);
    const std::vector<MUINT32> sensorList = mP2Info.getConfigInfo().mAllSensorID;
    MY_S_LOGI(mLog, "create captureFeaturePipe, isDualCam:%d, sensor count:%zu", isDualCam, sensorList.size());
    ICaptureFeaturePipe::UsageHint usage = ICaptureFeaturePipe::UsageHint();
    usage.mSupportedScenarioFeatures = param.mP2Info.getConfigInfo().mUsageHint.mTP;
    usage.mDualMode = param.mP2Info.getConfigInfo().mUsageHint.mDualMode;
    usage.mPluginUniqueKey = mP2Info.getConfigInfo().mUsageHint.mPluginUniqueKey;
    usage.mIsHidlIsp = mP2Info.getConfigInfo().mUsageHint.mIsHidlIsp;
    usage.mAppSessionMeta = mP2Info.getConfigInfo().mUsageHint.mAppSessionMeta;
    MY_S_LOGI(mLog, "supportSceneroiFeatures:0x%" PRIx64 " dualMode:%d", usage.mSupportedScenarioFeatures, usage.mDualMode);

    mpFeaturePipe = ICaptureFeaturePipe::createInstance(usage, sensorList);

    if (mpFeaturePipe == NULL) {
        MY_S_LOGE(mLog, "OOM: cannot create FeaturePipe");
    } else {
        mpFeaturePipe->init();
    }

    mpCallback = new CaptureRequestCallback(this);
    mpFeaturePipe->setCallback(mpCallback);

    P2_CAM_TRACE_END(TRACE_DEFAULT);
    TRACE_S_FUNC_EXIT(log);
    return ret;
}

MVOID CaptureProcessor::onUninit()
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:uninit()");
    MY_S_LOGI(mLog, "addr:%p, uniqueKey:%d", this, mP2Info.getConfigInfo().mUsageHint.mPluginUniqueKey);

    mpFeaturePipe->uninit();
    mpFeaturePipe = NULL;

    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::onThreadStart()
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:threadStart()");
    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::onThreadStop()
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:threadStop()");
    TRACE_S_FUNC_EXIT(mLog);
}

MBOOL CaptureProcessor::onConfig(const P2ConfigParam &param)
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:config()");
    mP2Info = param.mP2Info;
    TRACE_S_FUNC_EXIT(mLog);
    return MTRUE;
}

MBOOL CaptureProcessor::onEnque(const sp<P2FrameRequest> &pP2FrameRequest)
{
    const ILog log = spToILog(pP2FrameRequest);
    TRACE_S_FUNC_ENTER(log);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:enque()");
    MBOOL ret = MTRUE;

    const MBOOL isDualCam = (mP2Info.getConfigInfo().mAllSensorID.size() > 1);
    const MUINT32 dualMode = mP2Info.getConfigInfo().mUsageHint.mDualMode;

    const MBOOL isZoom = (isDualCam && (dualMode & v1::Stereo::E_DUALCAM_FEATURE_ZOOM));
    const MBOOL isVsdof = (isDualCam && (dualMode & v1::Stereo::E_STEREO_FEATURE_CAPTURE));
    const MBOOL isMulticam = (isDualCam && (dualMode & v1::Stereo::E_STEREO_FEATURE_MULTI_CAM));
    const MBOOL isHidlIsp = mP2Info.getConfigInfo().mUsageHint.mIsHidlIsp;

    MY_S_LOGD(mLog, "isZoom %d, isVsdof %d, isMultiCam %d, isHidlIsp %d", isZoom, isVsdof, isMulticam, isHidlIsp);

    std::vector<sp<P2Request>> pP2Requests = pP2FrameRequest->extractP2Requests();
    std::vector<sp<P2Request>> pReorderRequests;
    MUINT32 masterId = INVALID_SENSOR_ID;
    bool hasLogical = false;
    if ((pP2Requests.size() > 0) && (pP2Requests.front() != NULL)) {
        masterId = pP2Requests.front()->mP2Pack.getFrameData().mMasterSensorID;
        reorderRequest(pP2Requests, pReorderRequests, hasLogical);
    }

    sp<P2Request> pTmpRequest;
    size_t reqCount = pReorderRequests.size();
    size_t numOfReq = 0;
    for (auto& p : pReorderRequests) {
        numOfReq++;
        pTmpRequest = p;
        if (pTmpRequest == NULL) {
            MY_S_LOGE(log, "P2Request is NULL!");
            return MFALSE;
        }
        // pRequest(had be detached) hold the all resources, except that doesn't hold a MWFrame object
        const sp<P2Request> pRequest = pTmpRequest->makeDetachP2Request();
        // pP2Frame only hold the MWFrame object
        pTmpRequest->releaseResourceWithLock(P2Request::RES_ALL);
        const sp<IP2Frame> pP2Frame = pTmpRequest;

        const P2FrameData& rFrameData = pRequest->mP2Pack.getFrameData();
        const MINT32 requestNo = rFrameData.mMWFrameRequestNo;
        const MINT32 frameNo = rFrameData.mMWFrameNo;
        const MBOOL isPhysicalStream = pRequest->isPhysic();
        const MBOOL isNeedPhysicalControl = (isPhysicalStream && !pRequest->isValidMeta(IN_APP));
        MINT32 frameCount = 0;
        MINT32 frameIndex = -1;
        MINT32 sensorId = pRequest->getSensorID();
        MINT32 subSensorId = pRequest->getSlaveSensorID();
        sp<ICaptureFeatureRequest> pCapRequest = mpFeaturePipe->acquireRequest();
        pCapRequest->addParameter(PID_REQUEST_NUM, requestNo);
        pCapRequest->addParameter(PID_FRAME_NUM, rFrameData.mMWFrameNo);
        pCapRequest->setIsPhysicalStream(isPhysicalStream);
        pCapRequest->setSensorIndex(masterId, sensorId, subSensorId);

        MY_LOGD("P2 frame number(%d) main sensorId(%d) sub sensor(%d) isPhyStream(%d) isPhyControl(%d)",
                frameNo, sensorId, subSensorId, isPhysicalStream, isNeedPhysicalControl);

        MINT64 feature = 0;
        if (pRequest->isValidMeta(IN_P1_HAL, sensorId)) {
            sp<P2Meta> meta = pRequest->getMeta(IN_P1_HAL, sensorId);

            if(meta.get()) {
                if(isHidlIsp)   //request doesn't go through P1 in ispHidl flow, set request number for MSNR key generation in negotiation stage
                    meta->trySet(MTK_PIPELINE_REQUEST_NUMBER, requestNo);
                meta->trySet(MTK_PIPELINE_FRAME_NUMBER, frameNo);
            }

            if (tryGet<MINT32>(meta, MTK_HAL_REQUEST_COUNT, frameCount) &&
                tryGet<MINT32>(meta, MTK_HAL_REQUEST_INDEX, frameIndex)) {
                pCapRequest->addParameter(PID_FRAME_COUNT, frameCount);
                pCapRequest->addParameter(PID_FRAME_INDEX, frameIndex);
            }

            MBOOL ret = MFALSE;
            if (isPhysicalStream)
                ret = tryGet<MINT64>(meta, MTK_FEATURE_CAPTURE_PHYSICAL, feature);
            else
                ret = tryGet<MINT64>(meta, MTK_FEATURE_CAPTURE, feature);

            if (ret) {

                MY_S_LOGI(log, "Index/Count:%d/%d Feature:0x%" PRIx64, frameIndex, frameCount, feature);

                if (feature & MTK_FEATURE_REMOSAIC)
                    pCapRequest->addFeature(FID_REMOSAIC);
                if (feature & MTK_FEATURE_NR)
                    pCapRequest->addFeature(FID_NR);
                if (feature & MTK_FEATURE_FB)
                    pCapRequest->addFeature(FID_FB);
                if (feature & MTK_FEATURE_ABF)
                    pCapRequest->addFeature(FID_ABF);
                if (feature & MTK_FEATURE_AINR)
                    pCapRequest->addFeature(FID_AINR);
                if (feature & MTK_FEATURE_MFNR)
                    pCapRequest->addFeature(FID_MFNR);
                if (feature & MTK_FEATURE_HDR)
                    pCapRequest->addFeature(FID_HDR);
                if (feature & TP_FEATURE_FB)
                    pCapRequest->addFeature(FID_FB_3RD_PARTY);
                if (feature & TP_FEATURE_MFNR)
                    pCapRequest->addFeature(FID_MFNR_3RD_PARTY);
                if (feature & TP_FEATURE_HDR)
                    pCapRequest->addFeature(FID_HDR_3RD_PARTY);
                if (feature & MTK_FEATURE_DRE)
                    pCapRequest->addFeature(FID_DRE);
                if (feature & MTK_FEATURE_CZ)
                    pCapRequest->addFeature(FID_CZ);
                if (feature & MTK_FEATURE_HFG)
                    pCapRequest->addFeature(FID_HFG);
                if (feature & MTK_FEATURE_DCE)
                    pCapRequest->addFeature(FID_DCE);
                if (feature & MTK_FEATURE_AINR_YUV)
                    pCapRequest->addFeature(FID_AINR_YUV);
                if (feature & MTK_FEATURE_AINR_YHDR)
                    pCapRequest->addFeature(FID_AINR_YHDR);
                if (feature & MTK_FEATURE_AIHDR)
                    pCapRequest->addFeature(FID_AIHDR);
                if (feature & TP_FEATURE_ZSDHDR)
                    pCapRequest->addFeature(FID_ZSDHDR_3RD_PARTY);
            } else {
                MY_LOGW("cannot get feature combination %s", isPhysicalStream ? "MTK_FEATURE_CAPTURE_PHYSICAL" : "MTK_FEATURE_CAPTURE");
            }
        }

        MINT32 isEarlyNotificationTrigger = -1;
        MINT32 isCshot = 0;
        if (pRequest->isValidMeta(IN_APP) || (isNeedPhysicalControl && pRequest->isValidMeta(IN_APP_PHY, sensorId))) {
            sp<P2Meta> meta = isNeedPhysicalControl ? pRequest->getMeta(IN_APP_PHY, sensorId) : pRequest->getMeta(IN_APP);
            bool setNextCapture = hasLogical ? (numOfReq == 1) : (numOfReq == reqCount);
            if (setNextCapture && tryGet<MINT32>(meta, MTK_CONTROL_CAPTURE_EARLY_NOTIFICATION_TRIGGER, isEarlyNotificationTrigger))
            {
                isEarlyNotificationTrigger = isEarlyNotificationTrigger > 0 ? 1 : 0;
                if ((feature & MTK_FEATURE_REMOSAIC) && (frameCount > 1)) {
                    // remosaic with multi-frame needs a large deal of memory, so force to disable next capture
                    isEarlyNotificationTrigger = 0;
                }
                pCapRequest->addParameter(PID_ENABLE_NEXT_CAPTURE, isEarlyNotificationTrigger);
            }
            if (tryGet<MINT32>(meta, MTK_CSHOT_FEATURE_CAPTURE, isCshot)) {
                if (isCshot) {
                    pCapRequest->addParameter(PID_CSHOT_REQUEST, isCshot);
                }
            }
        }
        MY_S_LOGI(log, "Index/Count:%d/%d isEarlyNotificationTrigger:%d, isCshot:%d, feature:%s",
                    frameIndex, frameCount, isEarlyNotificationTrigger, isCshot, pCapRequest->getFeatureString().c_str());

        MBOOL bDropRequest = MFALSE;
        // Error handling of logical multiple frames
        {
            // Debug
            if (frameIndex == mDebugDrop) {
                MY_S_LOGW(log, "Force to abort, Index/Count:%d/%d", frameIndex, frameCount);
                abortRequest(requestNo);
                mAbortingRequestNo = requestNo;
                bDropRequest = MTRUE;
            // All frames should have full-size input
            }
            #if MTKCAM_SEAMLESS_SWITCH_SUPPORT
            else if (frameCount > 1 && ((!pRequest->isValidImg(IN_FULL, sensorId) && !pRequest->isValidImg(IN_SEAMLESS_FULL, sensorId)) || (!pRequest->isValidImg(IN_RESIZED, sensorId) && !isHidlIsp))) {
                MY_S_LOGW(log, "Multi-frame request has no IN_FULL(%d), SEAMLESS_FULL(%d) or IN_RESIZED(%d). Abort request.",
                        pRequest->isValidImg(IN_FULL, sensorId),
                        pRequest->isValidImg(IN_SEAMLESS_FULL, sensorId),
                        pRequest->isValidImg(IN_RESIZED, sensorId));
                abortRequest(requestNo);
                mAbortingRequestNo = requestNo;
                bDropRequest = MTRUE;
            // Abort the rest frames
            }
            #else
            else if (frameCount > 1 && (!pRequest->isValidImg(IN_FULL, sensorId) || (!pRequest->isValidImg(IN_RESIZED, sensorId) && !isHidlIsp))) {
                MY_S_LOGW(log, "Multi-frame request has no IN_FULL(%d) or IN_RESIZED(%d). Abort request.",
                        pRequest->isValidImg(IN_FULL, sensorId),
                        pRequest->isValidImg(IN_RESIZED, sensorId));
                abortRequest(requestNo);
                mAbortingRequestNo = requestNo;
                bDropRequest = MTRUE;
            // Abort the rest frames
            }
            #endif
            else if (mAbortingRequestNo == requestNo) {
                MY_S_LOGW(log, "Request is aborting, Index/Count:%d/%d", frameIndex, frameCount);
                bDropRequest = MTRUE;
            } else {
                if (!isPhysicalStream) {
                    if ((frameCount > 1) && mLastRequestNo == requestNo) {
                        if (mLastFrameIndex + 1 != frameIndex) {
                            // out-of-order frame or previous frame dropped
                            MY_S_LOGW(log, "Out-of-order Frame Index/Count:%d/%d", frameIndex, frameCount);
                            abortRequest(requestNo);
                            mAbortingRequestNo = requestNo;
                            bDropRequest = MTRUE;
                        } else if(isZoom && (mLastMasterIndex != sensorId)) {
                            // Different master sensor ID in ZOOM mode
                            MY_S_LOGW(log, "Multi-frame request with different master sensor ID Request Num/Index/Count:%d/%d/%d",
                                    requestNo, frameIndex, frameCount);
                            abortRequest(requestNo);
                            mAbortingRequestNo = requestNo;
                            bDropRequest = MTRUE;
                        } else if(isZoom && (mLastSlaveIndex != subSensorId)) {
                            // Different slave sensor ID in ZOOM mode
                            MY_S_LOGW(log, "Multi-frame request with different slave sensor ID Request Num/Index/Count:%d/%d/%d",
                                    requestNo, frameIndex, frameCount);
                            abortRequest(requestNo);
                            mAbortingRequestNo = requestNo;
                            bDropRequest = MTRUE;
                        }
                    } else {
                        if (mLastFrameCount != 0 && mLastFrameCount != mLastFrameIndex + 1) {
                            // Previous blending request is not completed
                            MY_S_LOGW(log, "Abort Previous Request Num/Index/Count:%d/%d/%d",
                                    mLastRequestNo, mLastFrameIndex, mLastFrameCount);
                            abortRequest(mLastRequestNo);
                            mAbortingRequestNo = mLastRequestNo;
                        }
                        if (frameCount > 1 && frameIndex != 0) {
                            // The first of multi-frame request is not to start with index = 0
                            MY_S_LOGW(log, "The first(index:0) of multi-frame Index/Count:%d/%d", frameIndex, frameCount);
                            abortRequest(requestNo);
                            mAbortingRequestNo = requestNo;
                            bDropRequest = MTRUE;
                        }
                    }
                } else {
                    if ((frameCount > 1) && mLastRequestNo == requestNo) {
                        if (mLastFrameIndex != frameIndex) {
                            // out-of-order frame or previous frame dropped
                            MY_S_LOGW(log, "physical Out-of-order Frame Index/Count:%d/%d", frameIndex, frameCount);
                            abortRequest(requestNo);
                            mAbortingRequestNo = requestNo;
                            bDropRequest = MTRUE;
                        }
                    }
                }
            }
        }

        // Check: Mandatory Stream Buffer
        {
            MBOOL bInvalidStream = MFALSE;
            // check if input streams are valid except VSDOF's main2
            #if MTKCAM_SEAMLESS_SWITCH_SUPPORT
            if (  !(pRequest->isValidImg(IN_FULL, sensorId) || pRequest->isValidImg(IN_OPAQUE, sensorId) || pRequest->isValidImg(IN_REPROCESS) || pRequest->isValidImg(IN_SEAMLESS_FULL,sensorId))
               || !(pRequest->isValidMeta(IN_P1_APP, sensorId) && pRequest->isValidMeta(IN_P1_HAL, sensorId))) {
                MY_S_LOGW(log, "Request has no master IN_FULL(%d), OPAQUE(%d) ,REPROCESS(%d), SEAMLESS_FULL(%d), P1APPMETA(%d), or P1HALMETA(%d). Skip frame.",
                        pRequest->isValidImg(IN_FULL, sensorId),
                        pRequest->isValidImg(IN_OPAQUE, sensorId),
                        pRequest->isValidImg(IN_REPROCESS),
                        pRequest->isValidImg(IN_SEAMLESS_FULL),
                        pRequest->isValidMeta(IN_P1_APP, sensorId),
                        pRequest->isValidMeta(IN_P1_HAL, sensorId));
                bInvalidStream = MTRUE;
            }
            #else
            if (  !(pRequest->isValidImg(IN_FULL, sensorId) || pRequest->isValidImg(IN_OPAQUE, sensorId) || pRequest->isValidImg(IN_REPROCESS))
               || !(pRequest->isValidMeta(IN_P1_APP, sensorId) && pRequest->isValidMeta(IN_P1_HAL, sensorId))) {
                MY_S_LOGW(log, "Request has no master IN_FULL(%d), OPAQUE(%d) ,REPROCESS(%d), P1APPMETA(%d), or P1HALMETA(%d). Skip frame.",
                        pRequest->isValidImg(IN_FULL, sensorId),
                        pRequest->isValidImg(IN_OPAQUE, sensorId),
                        pRequest->isValidImg(IN_REPROCESS),
                        pRequest->isValidMeta(IN_P1_APP, sensorId),
                        pRequest->isValidMeta(IN_P1_HAL, sensorId));
                bInvalidStream = MTRUE;
            }
            #endif

            // check if VSDOF's or ZoomFusion's main2 input streams are valid
            if (   !isPhysicalStream && (isVsdof || (isZoom && subSensorId >= 0))
              && (!pRequest->isValidImg(IN_FULL, subSensorId) || !pRequest->isValidMeta(IN_P1_APP, subSensorId) || !pRequest->isValidMeta(IN_P1_HAL, subSensorId)) ) {

                MY_S_LOGW(log, "Request has no slave IN_FULL(%d), P1APPMETA(%d), or P1HALMETA(%d). Skip frame.",
                        pRequest->isValidImg(IN_FULL, subSensorId),
                        pRequest->isValidMeta(IN_P1_APP, subSensorId),
                        pRequest->isValidMeta(IN_P1_HAL, subSensorId));
                bInvalidStream = MTRUE;
            }

            if (bInvalidStream) {
                abortRequest(requestNo);
                mAbortingRequestNo = requestNo;
                bDropRequest = MTRUE;
            }

            if (frameIndex < 1 && !pRequest->hasOutput()) {
                MY_S_LOGW(log, "Non-blending request has no output(%d)", pRequest->hasOutput());
                bDropRequest = MTRUE;
            }
        }

        if (bDropRequest) {
            {   // TODO(MTK): callback next capture when abort request for switch sensor condition
                std::lock_guard<std::mutex> lock(mNextCaptureLock);
                bool needCallNextCapture = false;
                if ((mNextCaptureCB.first == requestNo)) {
                    if (mNextCaptureCB.second == true) {
                        MY_LOGD("request %d already callback next capture", requestNo);
                    } else {
                        if (mRequestRestored.count(requestNo) == 0) {
                            mNextCaptureCB = std::make_pair(requestNo, true);
                            needCallNextCapture = true;
                        }
                    }
                } else {
                    if (mRequestRestored.count(requestNo) == 0) {
                        mNextCaptureCB = std::make_pair(requestNo, true);
                        needCallNextCapture = true;
                    }
                }

                if (needCallNextCapture) {
                    pP2Frame->notifyNextCapture(static_cast<MUINT32>(mRequestPairs.size()), pCapRequest->getParameter(PID_CSHOT_REQUEST)>=0);
                    MY_S_LOGI(log, "notify next capture when abort request, request count:%zu, R/F Num:%d/%d",
                              mRequestPairs.size(), requestNo, frameNo);
                    if (mRequestRestored.count(requestNo) > 0)
                        mRequestRestored.erase(requestNo);
                }
            }

            pRequest->updateResult(MFALSE);
            pRequest->beginBatchRelease();
            pRequest->releaseResourceWithLock(P2Request::RES_ALL);
            pRequest->endBatchRelease();

            return MFALSE;
        }

        // Reset mLastRequestNo. RequestNo is brought to HAL by APP in hidlIsp flow. Might have consecutive requests with same requestNo
        MBOOL shouldReset = (isHidlIsp && frameIndex == frameCount -1);
        mLastRequestNo = (shouldReset)? 0: requestNo;
        mLastFrameCount = (shouldReset)? 0: frameCount;
        mLastFrameIndex = (shouldReset)? 0: frameIndex;
        if (!isPhysicalStream) {
            mLastMasterIndex = sensorId;
            mLastSlaveIndex = subSensorId;
        }

        // Metadata
        auto MapMetadata = [&](ID_META id, CaptureMetadataID metaId, MUINT32 sensorId) -> void {
            ID_META realId = id;
            // non output meta id, get mapping id with sensorId
            if(!(id == IN_APP || id == OUT_APP || id == OUT_HAL))
                realId = pRequest->getMetaID(id, sensorId);
            if (pRequest->isValidMeta(realId))
                pCapRequest->addMetadata(metaId, new P2MetadataHandle(pRequest, realId, this, log));
        };

        MapMetadata(IN_P1_APP, MID_MAN_IN_P1_DYNAMIC, sensorId);
        MapMetadata(IN_P1_HAL, MID_MAN_IN_HAL, sensorId);
        // add physical input app meta when there is physical control
        if (isNeedPhysicalControl)
            MapMetadata(IN_APP_PHY, MID_MAN_IN_APP, sensorId);
        else
            MapMetadata(IN_APP, MID_MAN_IN_APP, sensorId);
        // add physical output app meta when there is physical output
        if (isPhysicalStream)
            MapMetadata(OUT_APP_PHY, MID_MAN_OUT_APP, sensorId);
        else
            MapMetadata(OUT_APP, MID_MAN_OUT_APP, sensorId);
        MapMetadata(OUT_HAL, MID_MAN_OUT_HAL, sensorId);

        // Image
        auto MapBuffer = [&](ID_IMG id, CaptureBufferID bufId, MUINT32 sensorId, const sp<ReleaseBundle>& pBundle) -> MBOOL {
            ID_IMG realId = id;
            // non output image id, get mapping id with sensorId
            if((id < OUT_IMG_START) && (id > IN_MAIN_IMG_START))
                realId = pRequest->getImgID(id, sensorId);
            if (pRequest->isValidImg(realId)) {
                pCapRequest->addBuffer(bufId, new P2BufferHandle(pRequest, realId, pBundle, this, log));
                return MTRUE;
            }
            return MFALSE;
        };

        sp<ReleaseBundle> pReleaseRaw = new ReleaseBundle("releaseRaw", requestNo, frameNo, this, log);
        MBOOL hasOpaque = MapBuffer(IN_OPAQUE, BID_MAN_IN_FULL, sensorId, pReleaseRaw);
        #if MTKCAM_SEAMLESS_SWITCH_SUPPORT
        MBOOL hasRaw = MapBuffer(IN_FULL, BID_MAN_IN_FULL, sensorId, pReleaseRaw)
                    || MapBuffer(IN_SEAMLESS_FULL, BID_MAN_IN_FULL, sensorId, pReleaseRaw);
        #else
        MBOOL hasRaw = MapBuffer(IN_FULL, BID_MAN_IN_FULL, sensorId, pReleaseRaw);
        #endif
        if (hasRaw && hasOpaque)
            MY_S_LOGW(log, "have opaque and raw input, using raw as input");

        // IN_RESIZED could be raw or yuv, so as BID_MAN_IN_RSZ
        MapBuffer(IN_RESIZED,   BID_MAN_IN_RSZ,         sensorId, pReleaseRaw);
        MapBuffer(IN_LCSO,      BID_MAN_IN_LCS,         sensorId, pReleaseRaw);
        MapBuffer(IN_LCSHO,     BID_MAN_IN_LCESHO,      sensorId, pReleaseRaw);
        MapBuffer(IN_REPROCESS, BID_MAN_IN_YUV,         sensorId, NULL);
        MapBuffer(IN_REPROCESS2,BID_SUB_IN_YUV,         sensorId, NULL);
        MapBuffer(OUT_JPEG_YUV, BID_MAN_OUT_JPEG,       sensorId, NULL);
        MapBuffer(OUT_THN_YUV,  BID_MAN_OUT_THUMBNAIL,  sensorId, NULL);
        MapBuffer(OUT_RAW,      BID_MAN_OUT_RAW,        sensorId, NULL);

        sp<ReleaseBundle> pReleasePostview = new ReleaseBundle("releasePostview", requestNo, frameNo, this, log, MTRUE);
        MapBuffer(OUT_POSTVIEW, BID_MAN_OUT_POSTVIEW, sensorId, pReleasePostview);

        size_t n = pRequest->mImgOutArray.size();
        if (n > 3) {
            MY_S_LOGE(log, "can NOT support more than 3 yuv streams: %zu", n);
            n = 3;
        }
        for (size_t i = 0; i < n; i++) {
            pCapRequest->addBuffer(BID_MAN_OUT_YUV00 + i,
                    new P2BufferHandle(pRequest, pRequest->mImgOutArray[i], this));
        }
        // Trigger Batch Release
        pP2Frame->beginBatchRelease();

        if (!isPhysicalStream) {
            if (isVsdof)
            {
                // Feature
                {
                    if (feature & MTK_FEATURE_DEPTH)
                        pCapRequest->addFeature(FID_DEPTH);
                    if (feature & MTK_FEATURE_BOKEH)
                        pCapRequest->addFeature(FID_BOKEH);
                    if (feature & TP_FEATURE_DEPTH)
                        pCapRequest->addFeature(FID_DEPTH_3RD_PARTY);
                    if (feature & TP_FEATURE_BOKEH)
                        pCapRequest->addFeature(FID_BOKEH_3RD_PARTY);
                    if (feature & TP_FEATURE_FUSION)
                        pCapRequest->addFeature(FID_FUSION_3RD_PARTY);
                    if (feature & TP_FEATURE_HDR_DC)
                        pCapRequest->addFeature(FID_HDR2_3RD_PARTY);
                    if (feature & TP_FEATURE_PUREBOKEH)
                        pCapRequest->addFeature(FID_PUREBOKEH_3RD_PARTY);
                    if (feature & TP_FEATURE_RELIGHTING)
                        pCapRequest->addFeature(FID_RELIGHTING_3RD_PARTY);
                }
                MY_S_LOGD(log, "ducam vsdof feature support, features: %s", pCapRequest->getFeatureString().c_str());
                // Meta
                {
                    MapMetadata(IN_P1_APP,      MID_SUB_IN_P1_DYNAMIC,  subSensorId);
                    MapMetadata(IN_P1_HAL,      MID_SUB_IN_HAL,         subSensorId);
                }
                // Image
                {
                    sp<ReleaseBundle> pReleaseSubYUV = new ReleaseBundle("releaseSubRaw", requestNo, frameNo, this, log);
                    MapBuffer(IN_FULL,      BID_SUB_IN_FULL,    subSensorId, pReleaseSubYUV);
                    MapBuffer(IN_RESIZED,   BID_SUB_IN_RSZ,     subSensorId, pReleaseSubYUV);
                    MapBuffer(IN_LCSO,      BID_SUB_IN_LCS,     subSensorId, pReleaseSubYUV);
                    MapBuffer(IN_LCSHO,     BID_SUB_IN_LCESHO,  subSensorId, pReleaseSubYUV);
                    MapBuffer(OUT_CLEAN,    BID_MAN_OUT_CLEAN,  subSensorId, NULL);
                    MapBuffer(OUT_DEPTH,    BID_MAN_OUT_DEPTH,  subSensorId, NULL);
                    MapBuffer(OUT_BOKEH,    BID_MAN_OUT_BOKEH,  subSensorId, NULL);
                }
            }
            else if (isZoom && (subSensorId != INVALID_SENSOR_ID))
            {
                // Feature
                {
                    if (feature & TP_FEATURE_ZOOM_FUSION) {
                        pCapRequest->addFeature(FID_FUSION_ZOOM_3RD_PARTY);
                        MY_S_LOGD(log, "ducam zoom feature support, features: %s", pCapRequest->getFeatureString().c_str());
                    }
                    else
                    {
                        subSensorId = INVALID_SENSOR_ID;
                        pCapRequest->setSensorIndex(masterId, sensorId, subSensorId);
                        MY_S_LOGW(log, "zoom without fusion plugin, not deal with slave sensor data");
                    }
                }
                if (subSensorId != INVALID_SENSOR_ID) {
                    // set frame number to sub meta for MSNR key coherence
                    if (pRequest->isValidMeta(IN_P1_HAL, subSensorId)) {
                        sp<P2Meta> meta = pRequest->getMeta(IN_P1_HAL, subSensorId);

                        if(meta.get()) {
                            meta->trySet(MTK_PIPELINE_FRAME_NUMBER, frameNo);
                        }
                    }
                    // Meta
                    {
                        MapMetadata(IN_P1_APP,      MID_SUB_IN_P1_DYNAMIC,  subSensorId);
                        MapMetadata(IN_P1_HAL,      MID_SUB_IN_HAL,         subSensorId);
                    }
                    // Image
                    {
                        sp<ReleaseBundle> pReleaseSubYUV = new ReleaseBundle("releaseSubRaw", requestNo, frameNo, this, log);
                        MapBuffer(IN_FULL,      BID_SUB_IN_FULL,    subSensorId, pReleaseSubYUV);
                        MapBuffer(IN_RESIZED,   BID_SUB_IN_RSZ,     subSensorId, pReleaseSubYUV);
                        MapBuffer(IN_LCSO,      BID_SUB_IN_LCS,     subSensorId, pReleaseSubYUV);
                        MapBuffer(IN_LCSHO,     BID_SUB_IN_LCESHO,  subSensorId, pReleaseSubYUV);
                    }
                }
            }
        }

        // Request Pair
        {
            Mutex::Autolock _l(mPairLock);
            auto& rPair = mRequestPairs.editItemAt(mRequestPairs.add());
            rPair.mpP2Frame = pP2Frame;
            rPair.mpDetachP2Request = pRequest;
            rPair.mpCapRequest = pCapRequest;
        }

        MY_S_LOGI(log, "enqued request to captureFeaturePipe, R/F Num:%d/%d", pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
        CAM_ULOG_SUBREQS(MOD_P2_CAP_PROC, REQ_P2_CAP_REQUEST, pCapRequest->getFrameNo(), REQ_CAP_FPIPE_REQUEST, pCapRequest->getFrameNo());
        //CAM_ULOG_ENTER(MOD_FPIPE_CAPTURE,REQ_CAP_FPIPE_REQUEST, pCapRequest->getFrameNo());

        mpFeaturePipe->enque(pCapRequest);
    }

    TRACE_S_FUNC_EXIT(log);
    return ret;
}

MVOID CaptureProcessor::abortRequest(MINT32 reqrstNo)
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:abort()");

    Vector<MUINT32> masterIDs;
    Vector<sp<ICaptureFeatureRequest>> vpCapRequests;
    {
        Mutex::Autolock _l(mPairLock);
        auto it = mRequestPairs.begin();
        for (; it != mRequestPairs.end(); it++) {

            sp<P2Request>& pP2Request = (*it).mpDetachP2Request;
            const P2FrameData& rFrameData = pP2Request->mP2Pack.getFrameData();
            sp<ICaptureFeatureRequest>& pCapRequest = (*it).mpCapRequest;

            if (rFrameData.mMWFrameRequestNo == reqrstNo && !(*it).mAborting) {
                (*it).mAborting = MTRUE;
                vpCapRequests.push_back(pCapRequest);
                // mark error
                pP2Request->updateResult(MFALSE);
            }
        }
    }

    mLastRequestNo = 0;
    mLastFrameCount = 0;
    mLastFrameIndex = 0;
    mLastMasterIndex = INVALID_SENSOR_ID;
    mLastSlaveIndex = INVALID_SENSOR_ID;

    for (auto& pCapRequest : vpCapRequests) {
        MY_S_LOGW(mLog, "abort request, R/F Num:%d/%d",
                pCapRequest->getRequestNo(),
                pCapRequest->getFrameNo());
        mpFeaturePipe->abort(pCapRequest);
    }
    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::notifyRelease(MINT32 requestNo, MINT32 frameNo, MBOOL isOutput)
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:notifyRelease()");

    Mutex::Autolock _l(mPairLock);
    auto it = mRequestPairs.begin();
    for (; it != mRequestPairs.end(); it++) {
        const sp<ICaptureFeatureRequest>& pCapRequest = (*it).mpCapRequest;
        MBOOL isGetCross = !isOutput;
        if ((pCapRequest->getRequestNo() == requestNo) && (pCapRequest->getFrameNo(isGetCross) == frameNo)) {
            const sp<IP2Frame>& pP2Frame = (*it).mpP2Frame;
            pP2Frame->endBatchRelease();
            pP2Frame->beginBatchRelease();

            MY_S_LOGI(mLog, "notify release, R/F Num:%d/%d", requestNo, frameNo);
            break;
        }
    }

    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::releaseImage(sp<P2Request> pRequest, ID_IMG imgId)
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:releaseImage()");
    Mutex::Autolock _l(mPairLock);
    pRequest->releaseImageWithLock(imgId);
    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::releaseMeta(sp<P2Request> pRequest, ID_META metaId)
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:releaseMeta()");
    Mutex::Autolock _l(mPairLock);
    pRequest->releaseMetaWithLock(metaId);
    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::onNotifyFlush()
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:notifyFlush()");
    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::waitRequestDone()
{
    TRACE_S_FUNC_ENTER(mLog);
    android::Mutex::Autolock _l(mPairLock);
    while (mRequestPairs.size())
    {
        MY_S_LOGI(mLog, "wait +, size: %zu", mRequestPairs.size());
        mPairCondition.wait(mPairLock);
        MY_S_LOGI(mLog, "wait -, size: %zu", mRequestPairs.size());
    }
    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::waitRequestAborted()
{
    TRACE_S_FUNC_ENTER(mLog);
    android::Mutex::Autolock _l(mPairLock);

    auto CountAboringRequest = [&]() -> MUINT32 {
        MUINT32 count = 0;
        auto it = mRequestPairs.begin();
        if (!it)
            MY_S_LOGW(mLog, "mRequestPairs is nullptr!");
        for (; it != mRequestPairs.end(); it++) {
            if ((*it).mAborting && !(*it).mRestored) {
                count++;
            }
        }
        return count;
    };

    size_t aborting;
    while ((aborting = CountAboringRequest()))
    {
        MY_S_LOGI(mLog, "wait +, size:%zu aborting: %zu", mRequestPairs.size(), aborting);
        mPairCondition.wait(mPairLock);
        MY_S_LOGI(mLog, "wait -, size:%zu", mRequestPairs.size());
    }

    TRACE_S_FUNC_EXIT(mLog);
}

MVOID CaptureProcessor::reorderRequest(const vector<sp<P2Request>> &requests, vector<sp<P2Request>>& reorderRequests, bool& hasLogical)
{
    MUINT32 masterId = requests.front()->mP2Pack.getFrameData().mMasterSensorID;
    vector<sp<P2Request>> pLogical;
    vector<sp<P2Request>> pPhyMaster;
    vector<sp<P2Request>> pPhySlave;
    for (auto& req : requests) {
        if (req->isPhysic()) {
            MINT32 sensorId = req->getSensorID();
            if (masterId == sensorId) {
                pPhyMaster.push_back(req);
            } else {
                pPhySlave.push_back(req);
            }
        } else {
            pLogical.push_back(req);
        }
    }

    // reorder the request with order logical -> physicalMaster -> physicalSlave
    if (pLogical.size() > 0) {
        reorderRequests.insert(reorderRequests.end(), pLogical.begin(), pLogical.end());
        hasLogical = true;
    }
    if (pPhyMaster.size() > 0)
        reorderRequests.insert(reorderRequests.end(), pPhyMaster.begin(), pPhyMaster.end());
    if (pPhySlave.size() > 0)
        reorderRequests.insert(reorderRequests.end(), pPhySlave.begin(), pPhySlave.end());
}

MVOID CaptureProcessor::onWaitFlush()
{
    TRACE_S_FUNC_ENTER(mLog);
    P2_CAM_TRACE_NAME(TRACE_DEFAULT, "P2_Capture:waitFlush()");
    // That could happened. The multi-frame request could be flushed before all frames are received.
    if (mLastFrameCount > 0 && mLastFrameCount != mLastFrameIndex + 1) {
        MY_S_LOGI(mLog, "abort a request[%d] whose all frames are not queued.", mLastRequestNo);
        mAbortingRequestNo = mLastRequestNo;
        abortRequest(mLastRequestNo);
    }

    const MBOOL bgPreRelease = mP2Info.getConfigInfo().mUsageHint.mBGPreRelease;
    MY_S_LOGI(mLog, "(%p) flush +, bgPreRelease:%d", this, bgPreRelease);
    if (bgPreRelease) {
        waitRequestAborted();

        Mutex::Autolock _l(mPairLock);
        const size_t requestPairsSize = mRequestPairs.size();

        // MUST wait all requests inference done before dispatching frame
        // because mfll's request is delay inference and
        // request will alloc input pipeline buffer in cross request's inference
        for (size_t i = 0; i < mRequestPairs.size(); ++i) {
           if (!mRequestPairs[i].mDetached) {
              mRequestPairs[i].mpCapRequest->waitInference();
           }
        }

        for (size_t i = 0; i < mRequestPairs.size(); ++i)
        {
            const sp<IP2Frame>& pP2Frame = mRequestPairs[i].mpP2Frame;
            const sp<P2Request>& pDetachRequest = mRequestPairs[i].mpDetachP2Request;
            const MBOOL hadDetached = mRequestPairs[i].mDetached;
            const sp<ICaptureFeatureRequest>& pCapRequest = mRequestPairs[i].mpCapRequest;
            const ILog log = spToILog(pDetachRequest);
            if (!hadDetached) {
                pDetachRequest->detachResourceWithLock(P2Request::RES_ALL);
                pP2Frame->endBatchRelease();
                auto& rPair = mRequestPairs.editItemAt(i);
                sp<IP2Frame> p2FrameTmp = rPair.mpP2Frame;
                rPair.mpP2Frame = new P2FrameHolder(NULL);
                rPair.mDetached = MTRUE;
                MY_S_LOGI(log, "detach request all resource, index:%zu/%zu, capReq-R/F Num:%d/%d",
                    i, requestPairsSize, pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
            } else {
                MY_S_LOGI(log, "request had be detached, index:%zu/%zu, capReq-R/F Num:%d/%d",
                    i, requestPairsSize, pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
            }
        }

        //REMOVE ME!! Temporarily to avoid timing issue, onDispatchFrame is called in another thread so that
        // JpegNode has little probability to be called by flush(frame) after node's flush()
        if (mRequestPairs.size() > 0) {
            usleep(1000); //1ms
        }
    }
    else {
        if (mpFeaturePipe != NULL) {
            mpFeaturePipe->flush();
        }
    }
    if (!bgPreRelease) {
        waitRequestDone();
    }

    MY_S_LOGI(mLog, "flush -");
    TRACE_S_FUNC_EXIT(mLog);
}

std::string CaptureProcessor::onGetStatus()
{
    return mpFeaturePipe ? mpFeaturePipe->getStatus() : std::string();
}

CaptureRequestCallback::CaptureRequestCallback(CaptureProcessor* pProcessor)
    : mpProcessor(pProcessor)
{

}

void CaptureRequestCallback::onMetaResultAvailable(android::sp<ICaptureFeatureRequest> pCapRequest, IMetadata* partialMeta)
{
    TRACE_S_FUNC_ENTER(mpProcessor->mLog);
    Mutex::Autolock _l(mpProcessor->mPairLock);
    auto it = mpProcessor->mRequestPairs.begin();
    for (; it != mpProcessor->mRequestPairs.end(); it++) {
        if ((*it).mpCapRequest == pCapRequest) {
            const sp<IP2Frame>& pP2Frame = (*it).mpP2Frame;
            const sp<P2Request>& pDetachRequest = (*it).mpDetachP2Request;
            const ILog log = spToILog(pDetachRequest);

            if (pP2Frame != NULL) {
                pP2Frame->metaResultAvailable(partialMeta);
                MY_S_LOGI(log, "(%p) partial metadata callback, request count:%zu, R/F Num:%d/%d",
                    this, mpProcessor->mRequestPairs.size(), pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
                break;
            } else {
                MY_LOGW("No need to callback partial meta, P2Frame is already detached!!");
            }
        }
    }
    TRACE_S_FUNC_EXIT(mpProcessor->mLog);
}

void CaptureRequestCallback::onContinue(sp<ICaptureFeatureRequest> pCapRequest)
{
    TRACE_S_FUNC_ENTER(mpProcessor->mLog);
    Mutex::Autolock _l(mpProcessor->mPairLock);
    auto it = mpProcessor->mRequestPairs.begin();
    for (; it != mpProcessor->mRequestPairs.end(); it++) {
        if ((*it).mpCapRequest == pCapRequest) {
            const sp<IP2Frame>& pP2Frame = (*it).mpP2Frame;
            const sp<P2Request>& pDetachRequest = (*it).mpDetachP2Request;
            const ILog log = spToILog(pDetachRequest);
            std::lock_guard<std::mutex> lock(mpProcessor->mNextCaptureLock);
            if ((mpProcessor->mNextCaptureCB.first == pCapRequest->getRequestNo()) &&
                (mpProcessor->mNextCaptureCB.second == true)) {
                MY_LOGD("request %d already callback next capture", pCapRequest->getRequestNo());
            } else {
                mpProcessor->mNextCaptureCB = std::make_pair(pCapRequest->getRequestNo(), true);
                pP2Frame->notifyNextCapture(static_cast<MUINT32>(mpProcessor->mRequestPairs.size()), pCapRequest->getParameter(PID_CSHOT_REQUEST)>=0);
                MY_S_LOGI(log, "(%p) notify next capture, request count:%zu, R/F Num:%d/%d",
                    this, mpProcessor->mRequestPairs.size(), pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
            }
            break;
        }
    }
    TRACE_S_FUNC_EXIT(mpProcessor->mLog);
}

void CaptureRequestCallback::onRestored(sp<ICaptureFeatureRequest> pCapRequest)
{
    TRACE_S_FUNC_ENTER(mpProcessor->mLog);
    Mutex::Autolock _l(mpProcessor->mPairLock);
    auto it = mpProcessor->mRequestPairs.begin();
    for (; it != mpProcessor->mRequestPairs.end(); it++) {
        if ((*it).mpCapRequest == pCapRequest) {
            const sp<P2Request>& pDetachRequest = (*it).mpDetachP2Request;
            const ILog log = spToILog(pDetachRequest);
            MY_S_LOGI(log, "(%p) restore an aborting(%d) request, request count:%zu, R/F Num:%d/%d",
                this,
                (*it).mAborting, mpProcessor->mRequestPairs.size(),
                pCapRequest->getRequestNo(), pCapRequest->getFrameNo());

            (*it).mRestored = MTRUE;
            mpProcessor->mRequestRestored.insert(pCapRequest->getRequestNo());
            // restore
            pDetachRequest->updateResult(MTRUE);
            mpProcessor->mPairCondition.broadcast();

            break;
        }
    }
    TRACE_S_FUNC_EXIT(mpProcessor->mLog);
}


void CaptureRequestCallback::onAborted(sp<ICaptureFeatureRequest> pCapRequest)
{
    TRACE_S_FUNC_ENTER(mpProcessor->mLog);
    Mutex::Autolock _l(mpProcessor->mPairLock);
    auto it = mpProcessor->mRequestPairs.begin();
    for (; it != mpProcessor->mRequestPairs.end(); it++) {
        if ((*it).mpCapRequest == pCapRequest) {
            CAM_ULOG_DISCARD(MOD_P2_CAP_PROC, REQ_P2_CAP_REQUEST, pCapRequest->getFrameNo());
            const sp<IP2Frame>& pP2Frame = (*it).mpP2Frame;
            const sp<P2Request>& pDetachRequest = (*it).mpDetachP2Request;
            const ILog log = spToILog(pDetachRequest);
            pDetachRequest->updateResult(MFALSE);
            pDetachRequest->updateMetaResult(MFALSE);
            pDetachRequest->releaseResourceWithLock(P2Request::RES_ALL);
            pP2Frame->endBatchRelease();
            mpProcessor->mRequestPairs.erase(it);
            mpProcessor->mPairCondition.broadcast();
            MY_S_LOGI(log, "(%p) erased an aborted request, request count:%zu, R/F Num:%d/%d",
                this, mpProcessor->mRequestPairs.size(), pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
            break;
        }
    }
    TRACE_S_FUNC_EXIT(mpProcessor->mLog);
}

void CaptureRequestCallback::onCompleted(sp<ICaptureFeatureRequest> pCapRequest, MERROR ret)
{
    TRACE_S_FUNC_ENTER(mpProcessor->mLog);
    Mutex::Autolock _l(mpProcessor->mPairLock);
    auto it = mpProcessor->mRequestPairs.begin();
    for (; it != mpProcessor->mRequestPairs.end(); it++) {
        if ((*it).mpCapRequest == pCapRequest) {
            const sp<IP2Frame>& pP2Frame = (*it).mpP2Frame;
            const sp<P2Request>& pDetachRequest = (*it).mpDetachP2Request;
            CAM_ULOG_EXIT(MOD_P2_CAP_PROC, REQ_P2_CAP_REQUEST, pCapRequest->getFrameNo());
            const ILog log = spToILog(pDetachRequest);
            pDetachRequest->updateResult(ret == OK);
            pDetachRequest->updateMetaResult(ret == OK);
            pDetachRequest->releaseResourceWithLock(P2Request::RES_ALL);
            pP2Frame->endBatchRelease();
            mpProcessor->mRequestPairs.erase(it);
            mpProcessor->mPairCondition.broadcast();
            MY_S_LOGI(log, "(%p) erased a completed request, request count:%zu, R/F Num:%d/%d",
                this, mpProcessor->mRequestPairs.size(), pCapRequest->getRequestNo(), pCapRequest->getFrameNo());
            break;
        }
    }
    TRACE_S_FUNC_EXIT(mpProcessor->mLog);
}
} // namespace P2
