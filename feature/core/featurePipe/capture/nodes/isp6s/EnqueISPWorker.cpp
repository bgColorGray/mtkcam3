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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#include "P2ANode.h"
#include "EnqueISPWorker.h"
#define PIPE_CLASS_TAG "P2ANode"
#define PIPE_TRACE TRACE_P2A_NODE
#include <featurePipe/core/include/PipeLog.h>

#include <mtkcam/aaa/IIspMgr.h>
#include <isp_tuning/isp_tuning.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

#include <mtkcam/drv/iopipe/SImager/IImageTransform.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <mtkcam/utils/exif/DebugExifUtils.h>
#include <mtkcam/utils/TuningUtils/CommonRule.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>

//
#include <mtkcam3/feature/lcenr/lcenr.h>
#include <mtkcam3/feature/msnr/IMsnr.h>
#ifdef SUPPORT_AINR
#include <mtkcam3/feature/ainr/IAinrCore.h>
#endif
//
#include <cmath>
#include <camera_custom_nvram.h>
#include <debug_exif/cam/dbg_cam_param.h>
//
#include "../../thread/CaptureTaskQueue.h"
#include <MTKDngOp.h>
#include <mtkcam/aaa/IDngInfo.h>
//
#include <mtkcam3/feature/utils/ISPProfileMapper.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAPTURE_P2A);

using namespace NSCam::NSCamFeature::NSFeaturePipe;
using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSMFB20;
using namespace NSCam::TuningUtils;
using namespace NSCam::NSIoPipe::NSSImager;
using namespace NSCam::NSPipelinePlugin;
using namespace NSIspTuning;

using NSCam::TuningUtils::scenariorecorder::DecisionParam;
using NSCam::TuningUtils::scenariorecorder::ResultParam;
using NSCam::TuningUtils::scenariorecorder::IScenarioRecorder;
using NSCam::TuningUtils::scenariorecorder::DECISION_ATMS;
using NSCam::TuningUtils::scenariorecorder::DECISION_ISP_FLOW;
using NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

// flag for workround that resize yuv force use the time sharing mode
static MBOOL USEING_TIME_SHARING = MTRUE;

enum DeubgMode
{
    AUTO    = -1,
    OFF     = 0,
    ON      = 1,
};

/*******************************************************************************
 *
 ********************************************************************************/

MBOOL
IEnqueISPWorker::enqueISP(shared_ptr<P2EnqueData>& pEnqueData, const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    P2EnqueData& enqueData = *pEnqueData.get();
    RequestPtr& pRequest   = mInfo->mpRequest;
    P2ANode* pNode         = mInfo->mNode;
    sp<CaptureFeatureNodeRequest> pNodeReq = pRequest->getNodeRequest(pEnqueData->mNodeId);
    // Trigger Dump
    enqueData.mDebugDump       = pNode->mDumpControl.mDebugDump;
    enqueData.mDebugDumpMDP    = pNode->mDumpControl.mDebugDumpMDP;
    enqueData.mDebugUnpack     = pNode->mDumpControl.mDebugUnpack;
    enqueData.mDebugDumpFilter = pNode->mDumpControl.mDebugDumpFilter;

    EnquePackage* pPackage = NULL;

    #define ASSERT(predict, mesg)     \
        do {                          \
            if (!(predict)) {         \
                if (pPackage != NULL) \
                    delete pPackage;  \
                CAM_ULOGM_FATAL(mesg);\
                return MFALSE;        \
            }                         \
        } while(0)

    ASSERT(getBuffer(pEnqueData), "getBuffer error");

    IImageBuffer* pIMG2O     = enqueData.mIMG2O.mpBuf;
    IImageBuffer* pIMG3O     = enqueData.mIMG3O.mpBuf;
    IImageBuffer* pTIMGO     = enqueData.mTIMGO.mpBuf;
    IImageBuffer* pWROTO     = enqueData.mWROTO.mpBuf;
    IImageBuffer* pWDMAO     = enqueData.mWDMAO.mpBuf;
    IImageBuffer* pCopy1     = enqueData.mCopy1.mpBuf;
    IImageBuffer* pCopy2     = enqueData.mCopy2.mpBuf;
    IImageBuffer* pMSSO      = enqueData.mMSSO.mpBuf;
    IImageBuffer* pIMGI      = enqueData.mIMGI.mpBuf;
    IImageBuffer* pDCESI     = enqueData.mDCESI.mpBuf;
    //
    MBOOL bIMGIRawFmt = NSCam::isHalRawFormat((EImageFormat)pIMGI->getImgFormat());
    MINT32 isMainSensor = (pRequest->getMainSensorIndex() == enqueData.mSensorId);
    MBOOL bHasOutYuv = pIMG2O || pIMG3O || pWROTO || pWDMAO;

    enqueData.mDebugLoadIn = pNode->mDumpControl.mDebugLoadIn && bIMGIRawFmt && bHasOutYuv && isMainSensor;

    MY_LOGD_IF(pNode->mLogLevel, "Online device tuning:%d, mDebugLoadIn:%d bIMGIRawFmt:%d bHasOutYuv:%d isMainSensor:%d",
                enqueData.mDebugLoadIn, pNode->mDumpControl.mDebugLoadIn, bIMGIRawFmt, bHasOutYuv, isMainSensor);

    // read data for online-device tuning
    if (enqueData.mDebugLoadIn)
    {
       onDeviceTuning(pEnqueData);
    }

    // to invalidate raw buffer because it is HW write before P2ANode
    if (pNode->mDumpControl.mDebugDump)
        pIMGI->syncCache(eCACHECTRL_INVALID);

    // Prepare enque package
    SmartTuningBuffer pBufTuning;
    if (pBufTuning == NULL)
        pBufTuning = pNode->mpBufferPool->getTuningBuffer();

    if(enqueData.mEnableMFB)
    {
        if(enqueData.mbMainFrame)
        {
            pBufTuning = pNode->mpKeepTuningData;
        }
        else
        {
            pBufTuning = pNode->mpBufferPool->getTuningBuffer();
            memcpy((void*)pBufTuning->mpVA, (void*)pNode->mpKeepTuningData->mpVA, pNode->mpBufferPool->getTuningBufferSize());
        }
    }
    else
    {
        pBufTuning = pNode->mpBufferPool->getTuningBuffer();
    }

    PQParam* pPQParam = new PQParam();
    MSSConfig* pMSSConfig = (pMSSO != nullptr) ? new MSSConfig() : nullptr;

    // create enquePackage
    pPackage = new EnquePackage();
    pPackage->mpEnqueData = pEnqueData;
    pPackage->mpTuningData = pBufTuning;
    pPackage->mpPQParam = pPQParam;
    pPackage->mpMSSConfig = pMSSConfig;
    pPackage->mpNode = pNode;
    pPackage->mbNeedToDump = pNode->needToDump(pRequest);

    if (pNode->mDumpControl.mDebugUnpack)
    {
        pPackage->mUnpackBuffer = pNode->mpBufferPool->getImageBuffer(pIMGI->getImgSize(), eImgFmt_BAYER10_UNPAK);
    }

    const MSize& rImgiSize = pIMGI->getImgSize();
    MY_LOGD("Sensor(%d) Resized(%d) R/F Num: %d/%d, EnQ: IMGI Fmt(0x%x) Size(%dx%d) VA/PA(%#" PRIxPTR "/%#" PRIxPTR ")",
               enqueData.mSensorId,
               enqueData.mResized,
               pRequest->getRequestNo(),
               pRequest->getFrameNo(),
               pIMGI->getImgFormat(),
               rImgiSize.w, rImgiSize.h,
               pIMGI->getBufVA(0),
               pIMGI->getBufPA(0));

    // Crop Calculation
    sp<CropCalculator::Factor> pFactor;
    ret &= getCropRegion(pEnqueData, pFactor);
    ASSERT(ret, "getCropRegion fail");

    // ISP tuning
    TuningParam tuningParam = {NULL};

    // wait for the previous task P2 callback
    if(waitForTiming != NULL && timing==BEFORE_SETP2ISP && pDCESI != NULL)
    {
        MY_LOGD_IF(pNode->mLogLevel ,"waitForTiming: BEFORE_SETP2ISP");
        sem_wait(waitForTiming.get());
    }

    // setP2ISP
    ret &= setP2ISP(pPackage, tuningParam);
    ASSERT(ret, "setP2ISP fail");

    // create enque param
    NSIoPipe::QParams qParam;
    ret &= createQParam(pPackage, tuningParam, qParam);
    ASSERT(ret, "create QParam fail");

    P2Output* pFirstHit = NULL;
    if (pCopy1 != NULL)
    {
        if (isCovered(enqueData.mIMG2O, enqueData.mCopy1))
            pFirstHit = &enqueData.mIMG2O;
        else if (isCovered(enqueData.mWROTO, enqueData.mCopy1))
            pFirstHit = &enqueData.mWROTO;
        else if (isCovered(enqueData.mWDMAO, enqueData.mCopy1))
            pFirstHit = &enqueData.mWDMAO;
        else
            MY_LOGE("Copy1's FOV is not covered by P2 first round output");
    }

    if (pCopy2 != NULL)
    {
        if (pFirstHit != NULL && isCovered(*pFirstHit, enqueData.mCopy2))
        {
            MY_LOGD("Use the same output to serve two MDP outputs");
        }
        else if (pFirstHit != &enqueData.mIMG2O && isCovered(enqueData.mIMG2O, enqueData.mCopy2))
        {
            if (!isCovered(enqueData.mIMG2O, enqueData.mCopy1))
                MY_LOGD("Use different output to serve two MDP outputs");
        }
        else if (pFirstHit != &enqueData.mWROTO && isCovered(enqueData.mWROTO, enqueData.mCopy2))
        {
            if (isCovered(enqueData.mWROTO, enqueData.mCopy1))
                MY_LOGD("Use different output to serve two MDP outputs");
        }
        else if (pFirstHit != &enqueData.mWDMAO && isCovered(enqueData.mWDMAO, enqueData.mCopy2))
        {
            if (!isCovered(enqueData.mWDMAO, enqueData.mCopy1))
                MY_LOGD("Use different output to serve two MDP outputs");
        }
        else
        {
            MY_LOGE("Copy2's FOV is not covered by P2 first round output");
        }
    }

    // if have next task which needs to wait
    if((mWaitForTiming) != NULL)
    {
        pPackage->mWait = mWaitForTiming;
    }

    // enque
    pPackage->mTimer.start();

    // callbacks
    qParam.mpfnCallback = onP2SuccessCallback;
    qParam.mpfnEnQFailCallback = onP2FailedCallback;
    qParam.mpCookie = (MVOID*) pPackage;

    CAM_TRACE_ASYNC_BEGIN("P2_Enque", (enqueData.mFrameNo << 3) + enqueData.mTaskId);

    // MDP QoS: using full performance
    if (PlatCap::isSupport(PlatCap::HWCap_MDPQoS))
    {
        struct timeval current;
        gettimeofday(&current, NULL);
        for (size_t i = 0, n = qParam.mvFrameParams.size(); i < n; ++i)
            qParam.mvFrameParams.editItemAt(i).ExpectedEndTime = current;
    }

    // dump QParams if mLogLevel >= 3
    dumpQParams(pNode->mLogLevel, qParam);

    // p2 enque
    if(enqueData.mbdirectRawBufCopy)
    {
        ret = directBufCopy(pIMGI, pTIMGO, qParam);
        if(ret != OK)
        {
            MY_LOGE("fail to copy buffer");
            return MFALSE;
        }
    }
    else
    {
        ret = pNode->mISPOperators[enqueData.mSensorId].mP2Opt->enque(qParam, LOG_TAG);
        ASSERT(ret == OK, "fail to enque P2");
    }

    TRACE_FUNC_EXIT();
    return MTRUE;

#undef ASSERT
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID
IEnqueISPWorker::onP2SuccessCallback(QParams& rParams)
{
    EnquePackage* pPackage = (EnquePackage*) (rParams.mpCookie);
    P2EnqueData& rData = *pPackage->mpEnqueData.get();
    P2ANode* pNode = pPackage->mpNode;
    CAM_TRACE_ASYNC_END("P2_Enque", (rData.mFrameNo << 3) + rData.mTaskId);
    pPackage->mTimer.stop();

    MY_LOGI("(%d) R/F/S Num: %d/%d/%d, task:%d, timeconsuming: %dms",
            rData.mNodeId,
            rData.mRequestNo,
            rData.mFrameNo,
            rData.mSensorId,
            rData.mTaskId,
            pPackage->mTimer.getElapsed());
    // show the real hardware processing time
    if(pNode->mLogLevel)
    {
        static timeval lastCallbacktime = {0};
        static int lastRequNo = rData.mRequestNo;
        // reset lastCallbacktime when different request comes
        if(lastRequNo != rData.mRequestNo)
            lastCallbacktime = {0};
        struct timeval callback_time;
        gettimeofday(&callback_time, NULL);
        if (lastCallbacktime.tv_sec != 0)
        {
            // hardwareProctime unit: us
            unsigned long hardwareProcTime = (callback_time.tv_sec-lastCallbacktime.tv_sec)*1e6 + callback_time.tv_usec - lastCallbacktime.tv_usec;
            MY_LOGD("(%d) R/F/S Num: %d/%d/%d, task:%d, hardware timeconsuming: %lums",
                rData.mNodeId,
                rData.mRequestNo,
                rData.mFrameNo,
                rData.mSensorId,
                rData.mTaskId,
                hardwareProcTime/1000);
            lastCallbacktime = callback_time;
        }
        else
        {
            lastCallbacktime = callback_time;
        }
        lastRequNo = rData.mRequestNo;
    }

    if(pPackage->mWait != nullptr && rData.mDCESO.mpBuf != NULL)
    {
        sem_post(pPackage->mWait.get());
    }

    if (pPackage->mbNeedToDump && rData.mDebugDump ) {
        char filename[256] = {0};
        FILE_DUMP_NAMING_HINT hint;
        hint.UniqueKey          = rData.mUniqueKey;
        hint.RequestNo          = rData.mRequestNo;
        hint.FrameNo            = rData.mFrameNo;

        extract_by_SensorOpenId(&hint, rData.mSensorId);

        auto DumpYuvBuffer = [&](IImageBuffer* pImgBuf, YUV_PORT port, const char* pStr, MBOOL bLoadIn) -> MVOID {
            if (pImgBuf == NULL)
                return;
            {
                extract(&hint, pImgBuf);

                // reset FrameNo, UniqueKey and RequestNo
                if (bLoadIn) {
                    FileReadRule rule;
                    MINT32 index = 0;
                    tryGetMetadata<MINT32>(rData.mpIMetaHal, MTK_HAL_REQUEST_INDEX_BSS, index);
                    rule.on_device_dump_file_rename(index, rData.mEnableMFB ? "MFNR" : "single_capture", &hint, NULL);

                    trySetMetadata<MINT32>(rData.mpIMetaHal, MTK_PIPELINE_DUMP_UNIQUE_KEY,     hint.UniqueKey);
                    trySetMetadata<MINT32>(rData.mpIMetaHal, MTK_PIPELINE_DUMP_REQUEST_NUMBER, hint.RequestNo);
                    trySetMetadata<MINT32>(rData.mpIMetaHal, MTK_PIPELINE_DUMP_FRAME_NUMBER,   hint.FrameNo);
                }
                if (pStr == NULL) {
                    genFileName_YUV(filename, sizeof(filename), &hint, port);
                } else {
                    genFileName_YUV(filename, sizeof(filename), &hint, port, pStr);
                }

                // restore FrameNo, UniqueKey and RequestNo
                if (bLoadIn) {
                    hint.UniqueKey = rData.mUniqueKey;
                    hint.RequestNo = rData.mRequestNo;
                    hint.FrameNo   = rData.mFrameNo;
                }

                pImgBuf->saveToFile(filename);
                MY_LOGD("Dump YUV: %s", filename);
            }
        };

        auto DumpLcsBuffer = [&](IImageBuffer* pImgBuf, const char* pStr) -> MVOID {
            if (pImgBuf == NULL)
                return;

            extract(&hint, pImgBuf);
            genFileName_LCSO(filename, sizeof(filename), &hint, pStr);
            pImgBuf->saveToFile(filename);
            MY_LOGD("Dump LCEI: %s", filename);
        };

        auto DumpRawBuffer = [&](IImageBuffer* pImgBuf, RAW_PORT port, const char* pStr) -> MVOID {
            if (pImgBuf == NULL)
                return;

            extract(&hint, pImgBuf);
            genFileName_RAW(filename, sizeof(filename), &hint, port, pStr);
            pImgBuf->saveToFile(filename);
            MY_LOGD("Dump RAW: %s", filename);
        };

        // MFNR Dump
        if (rData.mEnableMFB) {
            String8 str;
            if (rData.mIMG3O.mTypeId != TID_MAN_FULL_YUV) {// node's working for dump
                DumpYuvBuffer(rData.mIMG3O.mpBuf, YUV_PORT_IMG3O, NULL, rData.mDebugLoadIn);
            }

            // do NOT show sensor name for MFNR naming
            hint.SensorDev = -1;

            MINT32 iso = 0;
            MINT64 exp = 0;
            tryGetMetadata<MINT32>(rData.mpIMetaDynamic, MTK_SENSOR_SENSITIVITY, iso);
            tryGetMetadata<MINT64>(rData.mpIMetaDynamic, MTK_SENSOR_EXPOSURE_TIME, exp);

            // convert ns into us
            exp /= 1000;

            auto DumpMFNRBuffer = [&](P2Output& o) {

                switch (o.mTypeId) {
                    case TID_MAN_FULL_YUV:
                        if(!pNode->mDumpControl.mDumpMFNRYuv) return;
                        str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-yuv", iso, exp);
                        break;
                    case TID_MAN_SPEC_YUV:
                        str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-qyuv", iso, exp);
                        break;
                    case TID_MAN_MSS_YUV:
                        if(!pNode->mDumpControl.mDumpMFNRYuv) return;
                        str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-mss", iso, exp);
                        break;
                    case TID_MAN_IMGO_RSZ_YUV:
                        if(!pNode->mDumpControl.mDumpMFNRYuv) return;
                        str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-rsz", iso, exp);
                        break;
                    default:
                        return;
                }
                DumpYuvBuffer(o.mpBuf, YUV_PORT_NULL, str.string(), rData.mDebugLoadIn);
            };

            if (rData.mDebugDumpFilter & DUMP_ROUND1_IMG3O)
                DumpMFNRBuffer(rData.mIMG3O);

            if (rData.mDebugDumpFilter & DUMP_ROUND1_WDMAO)
                DumpMFNRBuffer(rData.mWDMAO);

            if (rData.mDebugDumpFilter & DUMP_ROUND1_WROTO)
                DumpMFNRBuffer(rData.mWROTO);

            DumpMFNRBuffer(rData.mMSSO);

            if (rData.mDebugDumpFilter & DUMP_ROUND1_IMG2O)
                DumpYuvBuffer(rData.mIMG2O.mpBuf, YUV_PORT_IMG2O, NULL, rData.mDebugLoadIn);

            if (rData.mDebugDumpFilter & DUMP_ROUND1_LCEI)
            {
                IImageBuffer* pLCEI = rData.mLCEI.mpBuf;
                if (pLCEI != NULL) {
                    str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-lcso", iso, exp);
                    DumpLcsBuffer(pLCEI, str.string());
                }
            }

            if (rData.mDebugDumpFilter & DUMP_ROUND1_IMGI) {
                str = String8::format("mfll-iso-%d-exp-%" PRId64 "-bfbld-raw", iso, exp);
                if (rData.mDebugUnpack) {
                    android::sp<IIBuffer> pWorkingBuffer = pPackage->mUnpackBuffer;
                    unpackRawBuffer(rData.mIMGI.mpBuf,pWorkingBuffer->getImageBufferPtr());
                    DumpRawBuffer(pWorkingBuffer->getImageBufferPtr(), RAW_PORT_NULL, str.string());
                } else {
                    DumpRawBuffer(rData.mIMGI.mpBuf, RAW_PORT_NULL, str.string());
                }
            }
        }
        else
        // Standard case dump
        {
            String8 str;

            if (rData.mNodeId == NID_P2B)
            {
                str += "single";
            }
            if (rData.mRound > 1)
            {
                if (str.length() != 0)
                    str += "-";

                str += "round2";
            }

            const char* pStr = str.string();
            if ((rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_IMG3O) ||
                (rData.mRound == 2 && rData.mDebugDumpFilter & DUMP_ROUND2_IMG3O))
            {
                DumpYuvBuffer(rData.mIMG3O.mpBuf, YUV_PORT_IMG3O, pStr, rData.mDebugLoadIn);
            }

            if ((rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_IMG2O) ||
                (rData.mRound == 2 && rData.mDebugDumpFilter & DUMP_ROUND2_IMG2O))
            {
                DumpYuvBuffer(rData.mIMG2O.mpBuf, YUV_PORT_IMG2O, pStr, rData.mDebugLoadIn);
            }

            if ((rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_WDMAO) ||
                (rData.mRound == 2 && rData.mDebugDumpFilter & DUMP_ROUND2_WDMAO))
            {
                DumpYuvBuffer(rData.mWDMAO.mpBuf, YUV_PORT_WDMAO, pStr, rData.mDebugLoadIn);
            }

            if ((rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_WROTO) ||
                (rData.mRound == 2 && rData.mDebugDumpFilter & DUMP_ROUND2_WROTO))
            {
                DumpYuvBuffer(rData.mWROTO.mpBuf, YUV_PORT_WROTO, pStr, rData.mDebugLoadIn);
            }

            if ((rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_TIMGO) ||
                (rData.mRound == 2 && rData.mDebugDumpFilter & DUMP_ROUND2_TIMGO))
            {
                DumpYuvBuffer(rData.mTIMGO.mpBuf, YUV_PORT_TIMGO, pStr, rData.mDebugLoadIn);
            }

            if (rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_LCEI)
            {
                DumpLcsBuffer(rData.mLCEI.mpBuf, NULL);
            }

            if (!rData.mYuvRep && rData.mRound < 2 && rData.mDebugDumpFilter & DUMP_ROUND1_IMGI) {
               if (rData.mDebugUnpack) {
                    android::sp<IIBuffer> pWorkingBuffer = pPackage->mUnpackBuffer;
                    unpackRawBuffer(rData.mIMGI.mpBuf,pWorkingBuffer->getImageBufferPtr());
                    DumpRawBuffer(
                           pWorkingBuffer->getImageBufferPtr(),
                           rData.mResized ? RAW_PORT_RRZO : RAW_PORT_IMGO,
                           NULL);
                } else {
                    DumpRawBuffer(
                           rData.mIMGI.mpBuf,
                           rData.mResized ? RAW_PORT_RRZO : RAW_PORT_IMGO,
                           NULL);
               }
            } else if (rData.mYuvRep && rData.mDebugDumpFilter & DUMP_ROUND1_IMGI) {
                   DumpYuvBuffer(rData.mIMGI.mpBuf, YUV_PORT_IMGI, pStr, rData.mDebugLoadIn);
            }
        }
    }

    // Update EXIF Metadata
    if (rData.mDebugDumpMDP && rData.mpOMetaHal)
    {

        auto GetMdpSetting = [] (void* p) -> MDPSetting*
        {
            if (p != nullptr)
            {
                DpPqParam* pParam = static_cast<DpPqParam*>(p);
                MDPSetting* pSetting = pParam->u.isp.p_mdpSetting;
                if (pSetting != nullptr)
                    return pSetting;
            }
            return NULL;
        };

        auto pSetting = GetMdpSetting(pPackage->mpPQParam->WDMAPQParam);

        if (pSetting == NULL)
            pSetting = GetMdpSetting(pPackage->mpPQParam->WROTPQParam);

        if (pSetting)
        {
            IMetadata exifMeta;
            tryGetMetadata<IMetadata>(rData.mpOMetaHal, MTK_3A_EXIF_METADATA, exifMeta);
            // Append MDP EXIF if required
            if (pSetting)
            {
                unsigned char* pBuffer = static_cast<unsigned char*>(pSetting->buffer);
                MUINT32 size = pSetting->size;
                if (DebugExifUtils::setDebugExif(
                        DebugExifUtils::DebugExifType::DEBUG_EXIF_RESERVE3,
                        static_cast<MUINT32>(MTK_RESVC_EXIF_DBGINFO_KEY),
                        static_cast<MUINT32>(MTK_RESVC_EXIF_DBGINFO_DATA),
                        size, pBuffer, &exifMeta) == nullptr)
                {
                    MY_LOGW("fail to set mdp debug exif to metadata");
                }
                else
                    MY_LOGI("Update Mdp debug info: addr %p, size %u %d %d", pBuffer, size, *pBuffer, *(pBuffer+1));
            }
            trySetMetadata<IMetadata>(rData.mpOMetaHal, MTK_3A_EXIF_METADATA, exifMeta);
        }
    }

    MBOOL hasCopyTask = MFALSE;
    sp<CaptureTaskQueue> pTaskQueue = pNode->mpTaskQueue;
    if (rData.mCopy1.mpBuf != NULL || rData.mCopy2.mpBuf != NULL)
    {
        pPackage->miSensorIdx = rData.mSensorId;
        if (pTaskQueue != NULL)
        {

            std::function<void()> func =
                [pPackage]()
                {
                    copyBuffers(pPackage);
                    delete pPackage;
                };

            pTaskQueue->addTask(func, MTRUE);
            hasCopyTask = MTRUE;
        }
    }

    if (!hasCopyTask)
    {
        // Could early callback only if there is no copy task
        if (rData.mWROTO.mEarlyRelease || rData.mWDMAO.mEarlyRelease)
        {

            RequestPtr pRequest = rData.mpHolder->mpRequest;
            sp<CaptureFeatureNodeRequest> pNodeReq = pRequest->getNodeRequest(rData.mNodeId);

            if (rData.mWROTO.mBufId != NULL_BUFFER)
                pNodeReq->releaseBuffer(rData.mWROTO.mBufId);
            if (rData.mWDMAO.mBufId != NULL_BUFFER)
                pNodeReq->releaseBuffer(rData.mWDMAO.mBufId);
        }

        // release source in another thread in order not to occupy P2 hardware thread
        if (pTaskQueue != NULL)
        {

            std::function<void()> func =
                [pPackage]()
                {
                    delete pPackage;
                };

            pTaskQueue->addTask(func, MTRUE);
        }
        else
        {
            delete pPackage;
        }
    }
}

/*******************************************************************************
 *
 ********************************************************************************/
MVOID
IEnqueISPWorker::onP2FailedCallback(QParams& rParams)
{
    EnquePackage* pPackage = (EnquePackage*) (rParams.mpCookie);
    P2EnqueData& rData = *pPackage->mpEnqueData.get();
    rData.mpHolder->mStatus = UNKNOWN_ERROR;

    CAM_TRACE_ASYNC_END("P2_Enque", (rData.mFrameNo << 3) + rData.mTaskId);
    pPackage->mTimer.stop();

    MY_LOGE("R/F/S Num: %d/%d/%d, task:%d, timeconsuming: %dms",
            rData.mRequestNo,
            rData.mFrameNo,
            rData.mSensorId,
            rData.mTaskId,
            pPackage->mTimer.getElapsed());

    delete pPackage;
}

/*******************************************************************************
 *
 ********************************************************************************/
IEnqueISPWorker::EnqueWorkerInfo::EnqueWorkerInfo(P2ANode* node, NodeID_T nodeId, RequestPtr& pRequest)
{
    mNode = node;
    mpRequest     = pRequest;
    mpNodeRequest = pRequest->getNodeRequest(nodeId);
    mRequestNo    = pRequest->getRequestNo();
    mFrameNo      = pRequest->getFrameNo();
    mSensorId     = pRequest->getMainSensorIndex();
    mSubSensorId  = pRequest->getSubSensorIndex();
    isPhysical    = pRequest->isPhysicalStream();
    mNodeId       = nodeId;

    mSensorFmt = (mSensorId >= 0) ? getSensorRawFmt(mSensorId) : SENSOR_RAW_FMT_NONE;
    mSubSensorFmt = (mSubSensorId >= 0) ? getSensorRawFmt(mSubSensorId) : SENSOR_RAW_FMT_NONE;
    MY_LOGD("mSensorId:%d -> sensorFmt:%#09x, mSubSensorId:%d -> sensorFmt2:%#09x",
        mSensorId, mSensorFmt, mSubSensorId, mSubSensorFmt);

    mpIMetaDynamic  = mpNodeRequest->acquireMetadata(MID_MAN_IN_P1_DYNAMIC);
    mpIMetaApp      = mpNodeRequest->acquireMetadata(MID_MAN_IN_APP);
    mpIMetaHal      = mpNodeRequest->acquireMetadata(MID_MAN_IN_HAL);
    mpOMetaApp      = mpNodeRequest->acquireMetadata(MID_MAN_OUT_APP);
    mpOMetaHal      = mpNodeRequest->acquireMetadata(MID_MAN_OUT_HAL);
    mpIMetaHal2     = nullptr;
    mpIMetaDynamic2 = nullptr;
    if (mSubSensorId >= 0)
    {
        mpIMetaHal2     = mpNodeRequest->acquireMetadata(MID_SUB_IN_HAL);
        mpIMetaDynamic2 = mpNodeRequest->acquireMetadata(MID_SUB_IN_P1_DYNAMIC);
    }

    tryGetMetadata<MINT32>(mpIMetaHal, MTK_PIPELINE_UNIQUE_KEY, mUniqueKey);

    isVsdof  = isVSDoFMode(node->mUsageHint);
    isZoom   = isZoomMode(node->mUsageHint);

    const MBOOL isBayerBayer = (mSensorFmt != SENSOR_RAW_MONO) && (mSubSensorFmt != SENSOR_RAW_MONO);
    const MBOOL isBayerMono  = (mSensorFmt != SENSOR_RAW_MONO) && (mSubSensorFmt == SENSOR_RAW_MONO);
    MY_LOGD("isBayerBayer: %d, isBayerMono: %d", isBayerBayer, isBayerMono);
    MBOOL bIsDualMode = ((node->mUsageHint.mDualMode != 0) && (mSubSensorId >= 0));
    sensorCom = (bIsDualMode && isBayerBayer) ? eSensorComb_BayerBayer :
                                   (bIsDualMode && isBayerMono) ? eSensorComb_BayerMono :
                                   (!bIsDualMode) ? eSensorComb_Single : eSensorComb_Invalid;

    mMappingKey   = node->mpISPProfileMapper->queryMappingKey(mpIMetaApp, mpIMetaHal, isPhysical, eMappingScenario_Capture, sensorCom);

    // MSNR check
    isRunMSNR = pRequest->hasFeature(FID_NR);
    isRawInput = pRequest->hasParameter(PID_REPROCESSING) ? MFALSE:MTRUE;

    isForceMargin = (property_get_int32("vendor.debug.camera.p2capture.forceMargin", 0) > 0);

    IMsnr::ModeParams param =
    {
        .openId         = mSensorId,
        .isRawInput     = isRawInput,
        .isHidlIsp      = node->mUsageHint.mIsHidlIsp,
        .isPhysical     = isPhysical,
        .isDualMode     = bIsDualMode,
        .isBayerBayer   = isBayerBayer,
        .isBayerMono    = isBayerMono,
        .isMaster       = pRequest->isMasterIndex(mSensorId),
        .multiframeType = pRequest->getParameter(PID_MULTIFRAME_TYPE),
        .pHalMeta       = mpIMetaHal,
        .pDynMeta       = mpIMetaDynamic,
        .pAppMeta       = mpIMetaApp,
        .pSesMeta       = &(node->mUsageHint.mAppSessionMeta)
    };
    mMsnrmode = IMsnr::getMsnrMode(param);

    MY_LOGD("MSNR mode:%d", static_cast<int>(mMsnrmode));
    if (mMsnrmode == MsnrMode_Disable || !isRunMSNR)
    {
        isRunMSNR = MFALSE;
        mMsnrmode = MsnrMode_Disable;
    }

    // DCE check - only Main Frame/Raw domain
    isYuvRep = mpNodeRequest->mapBufferID(TID_MAN_FULL_YUV, INPUT) != NULL_BUFFER;
    hasYUVOutput = checkHasYUVOutput(mpNodeRequest);

    isCShot = pRequest->hasParameter(PID_CSHOT_REQUEST);

    if(pRequest->isMainFrame() && pRequest->hasFeature(FID_DCE) && !isYuvRep && !isCShot && hasYUVOutput && mMsnrmode != MsnrMode_Raw)
    {
        // check meta: MTK_FEATURE_CAP_PIPE_DCE_CONTROL to disable dce
        MUINT8 dceControl = MTK_FEATURE_CAP_PIPE_DCE_DEFAULT_APPLY;
        if(!tryGetMetadata<MUINT8>(mpIMetaHal, MTK_FEATURE_CAP_PIPE_DCE_CONTROL, dceControl) ||
            dceControl != MTK_FEATURE_CAP_PIPE_DCE_MANUAL_DISABLE)
        {
            isRunDCE = MTRUE;
            if(dceControl == MTK_FEATURE_CAP_PIPE_DCE_ENABLE_BUT_NOT_APPLY)
            {
                isNotApplyDCE = MTRUE;
            }
        }
        MY_LOGD_IF(node->mLogLevel, "R/F/S:%d/%d/%d DCE_Control:%d", mRequestNo, mFrameNo, mSensorId, dceControl);
        if(!isRunDCE && mpNodeRequest->mapBufferID(TID_MAN_DCES, INPUT) != NULL_BUFFER)
        {
            MY_LOGE("R/F/S:%d/%d/%d plugin needs DCESO but metadata control to disable it, DCE_control:%d",
                    mRequestNo, mFrameNo, mSensorId, dceControl);
        }
    }
}

MBOOL
IEnqueISPWorker::getBuffer(shared_ptr<P2EnqueData>& pEnqueData)
{
    P2EnqueData& enqueData = *pEnqueData.get();
    auto GetBuffer = [&](IImageBuffer*& rpBuf, BufferID_T bufId)
    {
        if (bufId != NULL_BUFFER && rpBuf == NULL)
        {
            rpBuf = mInfo->mpNodeRequest->acquireBuffer(bufId);
        }
    };

    MBOOL ret = MTRUE;
    GetBuffer(enqueData.mIMG2O.mpBuf , enqueData.mIMG2O.mBufId);
    GetBuffer(enqueData.mIMG3O.mpBuf , enqueData.mIMG3O.mBufId);
    GetBuffer(enqueData.mTIMGO.mpBuf , enqueData.mTIMGO.mBufId);
    GetBuffer(enqueData.mDCESO.mpBuf , enqueData.mDCESO.mBufId);
    GetBuffer(enqueData.mWROTO.mpBuf , enqueData.mWROTO.mBufId);
    GetBuffer(enqueData.mWDMAO.mpBuf , enqueData.mWDMAO.mBufId);
    GetBuffer(enqueData.mCopy1.mpBuf , enqueData.mCopy1.mBufId);
    GetBuffer(enqueData.mCopy2.mpBuf , enqueData.mCopy2.mBufId);
    GetBuffer(enqueData.mMSSO.mpBuf  , enqueData.mMSSO.mBufId);
    GetBuffer(enqueData.mIMGI.mpBuf  , enqueData.mIMGI.mBufId);
    GetBuffer(enqueData.mLCEI.mpBuf  , enqueData.mLCEI.mBufId);
    GetBuffer(enqueData.mLCESHO.mpBuf, enqueData.mLCESHO.mBufId);
    GetBuffer(enqueData.mDCESI.mpBuf , enqueData.mDCESI.mBufId);

    auto RemapBuffer = [&](IImageBuffer*& rpBuf, BufferID_T bufId)
    {
        if (bufId != NULL_BUFFER)
        {
            MBOOL isMulti = false;
            TypeID_T typeId = NULL_TYPE;
            Direction dir = INPUT;

            mInfo->mpNodeRequest->queryTypeId(bufId, typeId, dir);
            if (typeId == TID_MAN_FULL_RAW && dir == INPUT) {
                isMulti = mInfo->mpNodeRequest->isMultiBuffer(bufId);
            }

            if (isMulti) {
                auto vpBuf = mInfo->mpNodeRequest->acquireBufferAll(bufId);

                // query bufferIndex
                int bufferIndex = 0;
                if (!IMetadata::getEntry<MINT32>(mInfo->mpIMetaHal,
                        MTK_FEATURE_VALID_IMAGE_INDEX, bufferIndex)) {
                    IMetadata::IEntry entry =
                            mInfo->mpIMetaHal->entryFor(
                            MTK_STAGGER_BLOB_IMGO_ORDER);
                    if (!entry.isEmpty()) {
                        MUINT8 entrySize = entry.count();
                        for (int i = 0; i < entrySize; ++i) {
                            auto tmp = entry.itemAt(i, Type2Type<MUINT8>());
                            if (tmp == MTK_STAGGER_IMGO_NE) {
                                bufferIndex = i;
                                break;
                            }
                        }
                    } else {
                        MY_LOGW("Can not get bufferIndex from metadata!!!");
                    }
                }

                rpBuf = vpBuf.at(bufferIndex);
                MY_LOGD("RemapBuffer to bufferIndex = %d", bufferIndex);
            }
        }
    };

    RemapBuffer(enqueData.mIMGI.mpBuf, enqueData.mIMGI.mBufId);

    // check if output buffer needed
    ret = (!!(enqueData.mIMG2O.mpBuf) ||
           !!(enqueData.mIMG3O.mpBuf) ||
           !!(enqueData.mWROTO.mpBuf) ||
           !!(enqueData.mWDMAO.mpBuf) ||
           !!(enqueData.mTIMGO.mpBuf));
    if(!ret)
    {
        MY_LOGE("do not have output buffer");
        return ret;
    }

    ret = !!enqueData.mIMGI.mpBuf;
    if(!ret)
    {
        MY_LOGE("do not have input buffer");
        return ret;
    }

    return ret;
}

MBOOL
IEnqueISPWorker::setP2ISP(EnquePackage* pPackage, TuningParam& tuningParam)
{
    shared_ptr<P2EnqueData>& pEnqueData = pPackage->mpEnqueData;
    P2EnqueData& enqueData = *pEnqueData.get();
    P2ANode* pNode = mInfo->mNode;
    RequestPtr& pRequest = mInfo->mpRequest;
    MBOOL master = pRequest->isMasterIndex(enqueData.mSensorId);
    MERROR ret = OK;
    // ISP tuning
    // LCE to CALTM buffer, this tuning buffer will be used in MDPNode
    SmartTuningBuffer pBufLCE2CALTM;
    if(   pRequest->hasFeature(FID_DRE)                       // CALTM feature is on
       && pNode->mSizeConfig.mLCE2CALTM_Size != 0
       && enqueData.mLCEI.mpBuf != NULL                       // LCE feature is on
       && enqueData.mLCESHO.mpBuf != NULL)
    {
        pBufLCE2CALTM = pNode->mpBufferPool->getTuningBuffer(pNode->mSizeConfig.mLCE2CALTM_Size);
        pBufLCE2CALTM->mSize = pNode->mSizeConfig.mLCE2CALTM_Size;
        pRequest->addTuningBuffer(BID_LCE2CALTM_TUNING, pBufLCE2CALTM);
        MY_LOGD("pBufLCE2CALTM: %p, pBufLCE2CALTM->mSiz: %d", pBufLCE2CALTM->mpVA, pBufLCE2CALTM->mSize);
    }
    else
    {
        pBufLCE2CALTM = nullptr;
        MY_LOGW("pBufLCE2CALTM is NULL, hasFeature(DRE): %d, mLCE2CALTM_Size: %d", pRequest->hasFeature(FID_DRE), pNode->mSizeConfig.mLCE2CALTM_Size);
    }

    // config MTK_ISP_P2_IN_IMG_FMT/ISP PROFILE
    {
        const MBOOL isImgo = !enqueData.mResized;

        IspProfileInfo profileInfo(getIspProfileName((EIspProfile_T)enqueData.mIspProfile), enqueData.mIspProfile);
        // The value should be used
        MINT32 iP2InputFormat = 0;
        // The value stored in HAL metadata. Should update HAL metadata if the two values are no equal
        MINT32 iP2CurrentFormat = 0;
        // set MTK_ISP_P2_IN_IMG_FMT to 1 for YUV2YUV processing
        MBOOL bYuvTunning = enqueData.mRound == 2 || enqueData.mYuvRep;
        if (bYuvTunning)
        {
            iP2InputFormat = 1;
        }
        // 0/Empty: RAW->YUV, 1: YUV->YUV
        tryGetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_ISP_P2_IN_IMG_FMT, iP2CurrentFormat);
        if (iP2CurrentFormat != iP2InputFormat)
            trySetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_ISP_P2_IN_IMG_FMT, iP2InputFormat);

        if (IScenarioRecorder::getInstance()->isScenarioRecorderOn()) {
            int32_t magicNumber = -1;
            tryGetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNumber);

            DecisionParam decParam;
            DebugSerialNumInfo &dbgNumInfo = decParam.dbgNumInfo;
            dbgNumInfo.uniquekey = mInfo->mUniqueKey;
            dbgNumInfo.reqNum = mInfo->mRequestNo;
            dbgNumInfo.frameNum = mInfo->mFrameNo;
            dbgNumInfo.magicNum = magicNumber;
            decParam.decisionType = DECISION_ATMS;
            decParam.moduleId = NSCam::Utils::ULog::MOD_CAPTURE_P2A;
            decParam.sensorId = enqueData.mSensorId;
            decParam.isCapture = true;
            WRITE_DECISION_RECORD_INTO_FILE(decParam,
                    "P2 capture profile mapper chooses profile(%s)",
                    getIspProfileName(static_cast<EIspProfile_T>(profileInfo.mValue)));

            ResultParam flowResult;
            flowResult.dbgNumInfo = decParam.dbgNumInfo;
            flowResult.decisionType = DECISION_ISP_FLOW;
            flowResult.moduleId = NSCam::Utils::ULog::MOD_CAPTURE_P2A;
            flowResult.sensorId = enqueData.mSensorId;
            flowResult.isCapture = true;
            flowResult.stageId = enqueData.mIspStage;
            WRITE_EXECUTION_RESULT_INTO_FILE(flowResult,
                "ISP is processing by isp profile(%s)",
                getIspProfileName(static_cast<EIspProfile_T>(profileInfo.mValue)));
        }

        // check if Y2J need customize profile
        if (pNode->mUsageHint.mIsHidlIsp) {
            bool isYUVin = (mInfo->mpNodeRequest->acquireBuffer(BID_MAN_IN_YUV) != nullptr);
            bool hasJpegOut = (mInfo->mpNodeRequest->acquireBuffer(BID_MAN_OUT_JPEG) != nullptr);
            if (isYUVin && hasJpegOut) {
                MUINT8 customizeProfile = 0;
                if (!tryGetMetadata<MUINT8>(enqueData.mpIMetaApp, MTK_CONTROL_CAPTURE_HIDL_JPEGYUV_TUNING, customizeProfile) || customizeProfile == 0) {
                    enqueData.mIspProfile = ISP_PROFILE_WITH_IDENTITY_EFFECT;
                    profileInfo.mName = getIspProfileName((EIspProfile_T)enqueData.mIspProfile);
                    profileInfo.mValue = enqueData.mIspProfile;
                }
            }
        }

        trySetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_ISP_STAGE, enqueData.mIspStage);
        trySetMetadata<MUINT8>(enqueData.mpIMetaHal, MTK_3A_ISP_PROFILE, profileInfo.mValue);
        MY_LOGD("set ispProfile, R/F Num: %d/%d, taskId:%d, profile:%d(%s), master:%d, imgo:%d, yuvRep:%d, mfb:%d, vsdof:%d, zoom:%d, lcei:%d, lcesho:%d, dcei:%d, dceso:%d, timgo:%d",
                mInfo->mRequestNo, mInfo->mFrameNo, enqueData.mTaskId,
                profileInfo.mValue, profileInfo.mName,
                master, isImgo,
                enqueData.mYuvRep, enqueData.mEnableMFB, enqueData.mEnableVSDoF, enqueData.mEnableZoom
                , enqueData.mLCEI.mpBuf  !=nullptr
                , enqueData.mLCESHO.mpBuf!=nullptr
                , enqueData.mDCESI.mpBuf !=nullptr
                , enqueData.mDCESO.mpBuf !=nullptr
                , enqueData.mTIMGO.mpBuf !=nullptr);
    }

    // check if the Raw buffer can be copied directly (pure raw to pure raw with same format)
    {
        if(enqueData.mTIMGO.mpBuf != nullptr && !pNode->mDumpControl.mDumpTimgo)
        {
            MINT infmt = enqueData.mTIMGO.mpBuf->getImgFormat();
            MINT outfmt = pEnqueData->mIMGI.mpBuf->getImgFormat();
            MINT32 iAPRawType = -1;
            // check the returned RAW type set by AP, P1 only output pure raw after isp6s
            tryGetMetadata<MINT32>(enqueData.mpIMetaApp, MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE, iAPRawType);
            if (isHalRawFormat(static_cast<EImageFormat>(infmt)) && (iAPRawType <= 0) && infmt == outfmt)
            {
                    MY_LOGD("bdirectRawBufCopy is setting to MTRUE");
                    enqueData.mbdirectRawBufCopy = MTRUE;
            }
        }
    }

    // Construct tuning parameter + setP2Isp
    {
        tuningParam.pRegBuf = reinterpret_cast<void*>(pPackage->mpTuningData->mpVA);
        if(pBufLCE2CALTM != nullptr)
        {
            tuningParam.pLce4CALTM = reinterpret_cast<void*>(pBufLCE2CALTM->mpVA);
        }

        // LCEI + LCESHO
        if (enqueData.mLCEI.mpBuf != NULL)
        {
            if(enqueData.mResized)
            {
                MY_LOGE("use LCSO for RRZO buffer, should not happened!");
                return MFALSE;
            }
            tuningParam.pLcsBuf = reinterpret_cast<void*>(enqueData.mLCEI.mpBuf);

            // Dual sync info to slave when LCE buffer exists
            const MBOOL isNeedDualSynData = (enqueData.mEnableVSDoF || enqueData.mEnableZoom);
            if (isNeedDualSynData)
                dualSyncInfo(tuningParam, enqueData, master);
        }
        if (enqueData.mLCESHO.mpBuf != NULL)
        {
            if(enqueData.mResized)
            {
                MY_LOGE("use LCESHO for RRZO buffer, should not happened!");
                return MFALSE;
            }
            tuningParam.pLceshoBuf = reinterpret_cast<void*>(enqueData.mLCESHO.mpBuf);
        }

        // DCES
        if (enqueData.mDCESI.mpBuf != NULL)
        {
            if(enqueData.mResized)
            {
                MY_LOGE("use DCESI for RRZO buffer, should not happened!");
                return MFALSE;
            }
            MINT32 iMagicNum = 0;
            if (!tryGetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_P1NODE_PROCESSOR_MAGICNUM, iMagicNum))
            {
                MY_LOGW("can not get magic number");
            }
            tuningParam.pDcsBuf = reinterpret_cast<void*>(enqueData.mDCESI.mpBuf);
            tuningParam.i4DcsMagicNo = iMagicNum;
        }

        // LTM setting
        if(enqueData.mLTMSettings.mbExist)
        {
            auto& settings = enqueData.mLTMSettings;
            tuningParam.pUserLTM_Setting = reinterpret_cast<MUINT32*>(settings.mLTMData.editArray());
            tuningParam.u4UserLTM_Setting_Size = settings.mLTMDataSize;
            MY_LOGD_IF(pNode->mLogLevel, "LTM Exist, size=%d pointer=%p",
                tuningParam.u4UserLTM_Setting_Size,  tuningParam.pUserLTM_Setting);
            MY_LOGD("LTM Exist, size=%d pointer=%p",
                tuningParam.u4UserLTM_Setting_Size,  tuningParam.pUserLTM_Setting);
        }
        // USE resize raw-->set PGN 0
        if (enqueData.mResized)
        {
            trySetMetadata<MUINT8>(enqueData.mpIMetaHal, MTK_3A_PGN_ENABLE, 0);
        }
        else
        {
            trySetMetadata<MUINT8>(enqueData.mpIMetaHal, MTK_3A_PGN_ENABLE, 1);
        }
        // used to send to setP2ISP
        MetaSet_T inMetaSet;
        // sub frame
        if (!enqueData.mbMainFrame && enqueData.mEnableMFB)
        {
            // For MFNR,
            // sub frame uses same metadata and tuning data as main frame,
            // but must restore some metadata of sub frame
            trySetMetadata<MINT32>(&pNode->mKeepMetaHal, MTK_PIPELINE_FRAME_NUMBER, pRequest->getFrameNo());
            trySetMetadata<MUINT8>(&pNode->mKeepMetaHal, MTK_ISP_P2_TUNING_UPDATE_MODE, 2);
            trySetMetadata<MINT32>(&pNode->mKeepMetaHal, MTK_HAL_REQUEST_INDEX_BSS, pRequest->getParameter(PID_FRAME_INDEX_FORCE_BSS));
            inMetaSet.appMeta = pNode->mKeepMetaApp;
            inMetaSet.halMeta = pNode->mKeepMetaHal;
        }
        else
        {
            inMetaSet.appMeta = *enqueData.mpIMetaApp;
            inMetaSet.halMeta = *enqueData.mpIMetaHal;
        }

        MetaSet_T outMetaSet;
        MetaSet_T* outMetaPtr = nullptr;

        MY_LOGD_IF(pNode->mLogLevel, "Generate output meta(%d) TaskId=%d pOMetaApp:%d pOMetaHal:%d",
                    enqueData.mbNeedOutMeta, enqueData.mTaskId, enqueData.mpOMetaApp != NULL, enqueData.mpOMetaHal != NULL);
        if(enqueData.mbNeedOutMeta)
        {
            if (enqueData.mpOMetaApp != NULL) {
                outMetaSet.appMeta = *enqueData.mpOMetaApp;
            }
            if(!pRequest->isPhysicalStream() && enqueData.mpOMetaHal != NULL) {
                outMetaSet.halMeta = *enqueData.mpOMetaHal;
            }
            outMetaPtr = &outMetaSet;
        }
        {
            CAM_TRACE_BEGIN("p2a:setIsp");
            // check profile to set identity
            if(enqueData.mIspProfile == ISP_PROFILE_WITH_IDENTITY_EFFECT)
                trySetMetadata<MUINT8>(&inMetaSet.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, EIdenditySetting);

            // SetP2Isp
            // if directly copy Raw Buffer, skip setP2ISP for efficiency purpose
            if(!enqueData.mbdirectRawBufCopy)
            {
                ret = pNode->mISPOperators[enqueData.mSensorId].mIspHal->setP2Isp(0, inMetaSet, &tuningParam, outMetaPtr);
            }
            CAM_TRACE_END();

            if (enqueData.mbMainFrame && enqueData.mEnableMFB)
            {
                pNode->mKeepMetaApp = *enqueData.mpIMetaApp;
                pNode->mKeepMetaHal = *enqueData.mpIMetaHal;
            }

        }
        if(ret != OK)
        {
            MY_LOGE("fail to set ISP");
            return MFALSE;
        }

        if (enqueData.mpOMetaHal != NULL)
        {
            (*enqueData.mpOMetaHal) += outMetaSet.halMeta;

            #if (MTKCAM_ISP_SUPPORT_COLOR_SPACE)
                // If flag on, the NVRAM will always prepare tuning data for P3 color space
                trySetMetadata<MINT32>(enqueData.mpOMetaHal, MTK_ISP_COLOR_SPACE, MTK_ISP_COLOR_SPACE_DISPLAY_P3);
            #endif
        }
        else
        {
            MY_LOGD("No need output metadata.");
            MY_LOGD_IF(pNode->mLogLevel, "No need output metadata.");
        }

        if (enqueData.mpOMetaApp != NULL)
            (*enqueData.mpOMetaApp) += outMetaSet.appMeta;
    }
    return MTRUE;
}

MVOID
IEnqueISPWorker::prepareLTMSettingData(
    IImageBuffer* pInBuffer,
    IMetadata* pInHalMeta,
    LTMSettings& outLTMSetting
)
{
    P2ANode* pNode = mInfo->mNode;
    if(pInBuffer == nullptr)
    {
        MY_LOGE("input buffer NULL");
        return ;
    }
    const int dip_valid_bitsize = 12;
    auto checkNeedDRC = [&]() -> MBOOL
    {
        if(pInBuffer->getImgBitsPerPixel() > dip_valid_bitsize)
            return MTRUE;
        else
            return MFALSE;
    };
    //
    EImageFormat fmt = static_cast<EImageFormat>(pInBuffer->getImgFormat());
    if(NSCam::isHalRawFormat(fmt) && checkNeedDRC())
    {
        if (IMetadata::getEntry<IMetadata::Memory>(
                pInHalMeta,
                MTK_ISP_DRC_CURVE,
                outLTMSetting.mLTMData)
            && IMetadata::getEntry<MINT32>(
                pInHalMeta,
                MTK_ISP_DRC_CURVE_SIZE,
                outLTMSetting.mLTMDataSize)
        )
        {
            MY_LOGD("Get DRC Curve, size=%d", outLTMSetting.mLTMDataSize);
            outLTMSetting.mbExist = MTRUE;
            // Clean related tag after fetching it
            pInHalMeta->remove(MTK_ISP_DRC_CURVE);
            pInHalMeta->remove(MTK_ISP_DRC_CURVE_SIZE);
        }
    }
    else
    {

        MY_LOGD_IF(pNode->mLogLevel, "isRawFmt:%d NeedDRC:%d BifPerPixel:%lu",
            NSCam::isHalRawFormat(fmt), checkNeedDRC(), pInBuffer->getImgBitsPerPixel());
    }
}

IImageBuffer*
IEnqueISPWorker::acquireDCESOBuffer(MBOOL isMain)
{
    sp<CaptureFeatureNodeRequest> pNodeRequest = mInfo->mpNodeRequest;
    shared_ptr<P2ANode::RequestHolder> pHolder = mInfo->pHolder;
    IImageBuffer* pDCES_Buf = nullptr;
    P2ANode* pNode = mInfo->mNode;
    BufferID_T uOBufDCE;
    if(isMain)
        uOBufDCE = pNodeRequest->mapBufferID(TID_MAN_DCES, OUTPUT);
    else
        uOBufDCE = pNodeRequest->mapBufferID(TID_SUB_DCES, OUTPUT);

    if (uOBufDCE != NULL_BUFFER)
    {
        pDCES_Buf = pNodeRequest->acquireBuffer(uOBufDCE);
    }
    else
    {
        android::sp<IIBuffer> pWorkingBuffer = pNode->mpBufferPool->getImageBuffer(pNode->mSizeConfig.mDCES_Size, (EImageFormat) pNode->mSizeConfig.mDCES_Format);
        pHolder->mpBuffers.push_back(pWorkingBuffer);
        pDCES_Buf = pWorkingBuffer->getImageBufferPtr();
    }
    if (pDCES_Buf)
    {
        for (size_t i = 0 ; i < pDCES_Buf->getPlaneCount() ; i++)
        {
            ::memset((void *)pDCES_Buf->getBufVA(i), 0, pDCES_Buf->getBufSizeInBytes(i));
        }
        pDCES_Buf->syncCache(eCACHECTRL_FLUSH);
    }
    return pDCES_Buf;
}

MBOOL
IEnqueISPWorker::checkHasYUVOutput(sp<CaptureFeatureNodeRequest> pNodeReq)
{
    // check if there is any output in request
    MBOOL hasYUVOutput = MFALSE;
    TypeID_T typeId;
    for (typeId = TID_YUV_START; typeId < TID_OTHER; typeId++)
    {
        BufferID_T bufId = pNodeReq->mapBufferID(typeId, OUTPUT);
        if (bufId == NULL_BUFFER)
          continue;

        IImageBuffer* pBuf = pNodeReq->acquireBuffer(bufId);
        if (pBuf != NULL)
        {
            MY_LOGI("request has yuv output. type:%d, buf:%d",typeId, bufId);
            hasYUVOutput = MTRUE;
            break;
        }
    }
    return hasYUVOutput;
}

MVOID
IEnqueISPWorker::dualSyncInfo(TuningParam& tuningParam, P2EnqueData& enqueData, MBOOL isMaster)
{
    MINT32& frameNo = enqueData.mFrameNo;
    MINT32& requestNo = enqueData.mRequestNo;
    P2ANode* pNode = mInfo->mNode;

    tuningParam.bSlave = !isMaster;
    if (!tuningParam.bSlave) // master
    {
        if (mTaskId == TASK_RSZ_MAIN)
        {
            if (enqueData.mpSharedData->rrzoDualSynData == NULL)
            {
                enqueData.mpSharedData->rrzoDualSynData = pNode->mpBufferPool->getTuningBuffer(pNode->mSizeConfig.mDualSyncInfo_Size);
                tuningParam.pDualSynInfo = reinterpret_cast<void*>(enqueData.mpSharedData->rrzoDualSynData->mpVA);
                MY_LOGD("create rrzo dual sync data, R/F Num: %d/%d, addr:%p, size:%u, taskId:%d",
                    requestNo, frameNo,
                    tuningParam.pDualSynInfo, pNode->mSizeConfig.mDualSyncInfo_Size, enqueData.mTaskId);
            }
            else
            {
                MY_LOGD("rrzo dual sync data is not null, R/F Num: %d/%d, addr:%p, taskId:%d",
                    requestNo, frameNo,
                    enqueData.mpSharedData->rrzoDualSynData->mpVA, enqueData.mTaskId);
            }
        }
        else
        {
            if (enqueData.mpSharedData->imgoDualSynData == NULL)
            {
                enqueData.mpSharedData->imgoDualSynData = pNode->mpBufferPool->getTuningBuffer(pNode->mSizeConfig.mDualSyncInfo_Size);
                tuningParam.pDualSynInfo = reinterpret_cast<void*>(enqueData.mpSharedData->imgoDualSynData->mpVA);
                MY_LOGD("create imgo dual sync data, R/F Num: %d/%d, addr:%p, size:%u, taskId:%d",
                    requestNo, frameNo,
                    tuningParam.pDualSynInfo, pNode->mSizeConfig.mDualSyncInfo_Size, enqueData.mTaskId);
            }
            else
            {
                MY_LOGW("imgo dual sync data is not null, R/F Num: %d/%d, addr:%p, taskId:%d",
                    requestNo, frameNo,
                    enqueData.mpSharedData->imgoDualSynData->mpVA, enqueData.mTaskId);
            }
        }
    }
    else
    { // slave
        if (mTaskId == TASK_RSZ_SUB)
        {
            if (enqueData.mpSharedData->rrzoDualSynData != NULL)
            {
                tuningParam.pDualSynInfo = reinterpret_cast<void*>(enqueData.mpSharedData->rrzoDualSynData->mpVA);
                MY_LOGD("set rrzo dual sync data, R/F Num: %d/%d, addr:%p, taskId:%d",
                    requestNo, frameNo,
                    tuningParam.pDualSynInfo, enqueData.mTaskId);
            }
            else
            {
                MY_LOGW("failed to set rrzo dual sync data, R/F Num: %d/%d, taskId:%d", requestNo, frameNo, enqueData.mTaskId);
            }
        }
        else
        {
            if (enqueData.mpSharedData->imgoDualSynData != NULL)
            {
                tuningParam.pDualSynInfo = reinterpret_cast<void*>(enqueData.mpSharedData->imgoDualSynData->mpVA);
                MY_LOGD("set imgo dual sync data, R/F Num: %d/%d, addr:%p, taskId:%d",
                    requestNo, frameNo,
                    tuningParam.pDualSynInfo, enqueData.mTaskId);
            }
            else
            {
                MY_LOGW("failed to set imgo dual sync data, R/F Num: %d/%d, taskId:%d", requestNo, frameNo, enqueData.mTaskId);
            }
        }
    }
}

// copy buffer by using cpu
MERROR
IEnqueISPWorker::directBufCopy(const IImageBuffer* pSrcBuf, IImageBuffer* pDstBuf, QParams& rParams)
{
    CAM_TRACE_BEGIN("p2a:directBufCopy");
    if(pSrcBuf->getImgSize().w > pDstBuf->getImgSize().w || pSrcBuf->getImgSize().h > pDstBuf->getImgSize().h)
    {
        MY_LOGE("directBufCopy fail due to source buffer larger than destination, pSrcBuf(%dx%d)/pDstBuf(%dx%d)"
            , pSrcBuf->getImgSize().w, pSrcBuf->getImgSize().h, pDstBuf->getImgSize().w, pDstBuf->getImgSize().h);
        this->onP2FailedCallback(rParams);
        CAM_TRACE_END();
        return BAD_VALUE;
    }
    size_t src_stride = pSrcBuf->getBufStridesInBytes(0);
    size_t dst_stride = pDstBuf->getBufStridesInBytes(0);
    MY_LOGD("directBufCopy start, pSrcBuf/stride: %p/%zu, pDstBuf/strid: %p/%zu", pSrcBuf, src_stride, pDstBuf, dst_stride);
    if(src_stride == dst_stride)
    {
        memcpy(reinterpret_cast<void*>(pDstBuf->getBufVA(0)), reinterpret_cast<void*>(pSrcBuf->getBufVA(0)), pSrcBuf->getBufSizeInBytes(0));
    }
    else
    {
        char* psrcBufferVa = reinterpret_cast<char*>(pSrcBuf->getBufVA(0));
        char* pdstBufferVa = reinterpret_cast<char*>(pDstBuf->getBufVA(0));
        for(int i=0 ; i<pSrcBuf->getImgSize().h ; i++)
        {
            if(src_stride > dst_stride) {
                memcpy(reinterpret_cast<void*>(pdstBufferVa + i*dst_stride), reinterpret_cast<void*>(psrcBufferVa + i*src_stride), dst_stride);
            } else {
                memcpy(reinterpret_cast<void*>(pdstBufferVa + i*dst_stride), reinterpret_cast<void*>(psrcBufferVa + i*src_stride), src_stride);
            }
        }
    }
    pDstBuf->syncCache(eCACHECTRL_FLUSH);
    MY_LOGD("directBufCopy finished");
    this->onP2SuccessCallback(rParams);
    CAM_TRACE_END();
    return OK;
}

MVOID
IEnqueISPWorker::addDebugBuffer(
    std::shared_ptr<P2EnqueData>& pEnqueData)
{
    P2ANode* pNode = mInfo->mNode;
    std::shared_ptr<P2ANode::RequestHolder>& pHolder = mInfo->pHolder;
    const MSize& rSize = pEnqueData->mIMGI.mpBuf->getImgSize();
    // Debug: Dump TIMGO only for ISP 6.0
    if (pNode->mDumpControl.mDumpTimgo)
    {
        MUINT32 timgoFormat = eImgFmt_UNKNOWN;
        if (pNode->mDumpControl.mDumpTimgoType == EDIPTimgoDump_AFTER_LTM)
        {

            MINT fmt = pEnqueData->mIMGI.mpBuf->getImgFormat();
            if ((fmt & eImgFmt_RAW_START) == eImgFmt_RAW_START)
            {
                timgoFormat = eImgFmt_BAYER10;
            }
        }
        if (pNode->mDumpControl.mDumpTimgoType == EDIPTimgoDump_AFTER_GGM)
            timgoFormat = eImgFmt_RGB48;

        if (timgoFormat != eImgFmt_UNKNOWN)
        {
            android::sp<IIBuffer> pDebugBuffer = pNode->mpBufferPool->getImageBuffer(rSize, timgoFormat, MSize(4, 0));
            pHolder->mpBuffers.push_back(pDebugBuffer);
            pEnqueData->mTIMGO.mpBuf = pDebugBuffer->getImageBufferPtr();
        }
    }

    {
        // Debug: Dump IMG3O
        if (pNode->mDumpControl.mDumpImg3o && pEnqueData->mIMG3O.mpBuf == NULL)
        {
            MUINT32 img3oFormat = eImgFmt_MTK_YUYV_Y210;
            MUINT32 img3oAlign = 16;
            android::sp<IIBuffer> pDebugBuffer = pNode->mpBufferPool->getImageBuffer(rSize, img3oFormat, MSize(img3oAlign, 0));
            pHolder->mpBuffers.push_back(pDebugBuffer);
            pEnqueData->mIMG3O.mpBuf = pDebugBuffer->getImageBufferPtr();
        }
    }
}

MBOOL
IEnqueISPWorker::copyBuffers(EnquePackage* pPackage)
{
    P2EnqueData& rData = *pPackage->mpEnqueData.get();
    MINT32 requestNo = rData.mRequestNo;
    MINT32 frameNo = rData.mFrameNo;
    MINT32 sensorId = rData.mSensorId;
    NodeID_T nodeId = rData.mNodeId;
    CAM_TRACE_FMT_BEGIN("p2a:copy|r%df%ds%d", requestNo, frameNo, sensorId);
    MY_LOGD("(%d) +, R/F/S Num: %d/%d/%d", nodeId, requestNo, frameNo, sensorId);


    IImageBuffer* pSource1  = rData.mCopy1.mpSource;
    IImageBuffer* pSource2  = rData.mCopy2.mpSource;
    IImageBuffer* pCopy1    = rData.mCopy1.mpBuf;
    IImageBuffer* pCopy2    = rData.mCopy2.mpBuf;
    MUINT32 uTrans1         = rData.mCopy1.mSourceTrans;
    MUINT32 uTrans2         = rData.mCopy2.mSourceTrans;
    MRect& rCrop1           = rData.mCopy1.mSourceCrop;
    MRect& rCrop2           = rData.mCopy2.mSourceCrop;
    MBOOL hasCopy2          = pCopy2 != NULL;
    MBOOL hasSameSrc        = hasCopy2 ? pSource1 == pSource2 : MFALSE;

    if (pSource1 == NULL || pCopy1 == NULL)
    {
        MY_LOGE("should have source1 & copy1 buffer here. src:%p, dst:%p", pSource1, pCopy1);
        return MFALSE;
    }

    if (hasCopy2 && pSource2 == NULL)
    {
        MY_LOGE("should have source2 buffer here. src:%p", pSource1);
        return MFALSE;
    }

    String8 strCopyLog;

    auto InputLog = [&](const char* sPort, IImageBuffer* pBuf) mutable
    {
        strCopyLog += String8::format("Sensor(%d) Resized(%d) R/F Num: %d/%d, Copy: %s Fmt(0x%x) Size(%dx%d) VA/PA(%#" PRIxPTR "/%#" PRIxPTR ")",
            rData.mSensorId,
            rData.mResized,
            requestNo,
            frameNo,
            sPort,
            pBuf->getImgFormat(),
            pBuf->getImgSize().w, pBuf->getImgSize().h,
            pBuf->getBufVA(0),
            pBuf->getBufPA(0));
    };

    auto OutputLog = [&](const char* sPort, MDPOutput& rOut) mutable
    {
        strCopyLog += String8::format(", %s Rot(%d) Crop(%d,%d)(%dx%d) Size(%dx%d) VA/PA(%#" PRIxPTR "/%#" PRIxPTR ")",
            sPort, rOut.mSourceTrans,
            rOut.mSourceCrop.p.x, rOut.mSourceCrop.p.y,
            rOut.mSourceCrop.s.w, rOut.mSourceCrop.s.h,
            rOut.mpBuf->getImgSize().w, rOut.mpBuf->getImgSize().h,
            rOut.mpBuf->getBufVA(0), rOut.mpBuf->getBufPA(0));
    };

    InputLog("SRC1", pSource1);
    OutputLog("COPY1", rData.mCopy1);

    if (hasCopy2)
    {
        if (!hasSameSrc)
        {
            MY_LOGD("%s", strCopyLog.string());
            strCopyLog.clear();
            InputLog("SRC2", pSource2);
        }
        OutputLog("COPY2", rData.mCopy2);
    }
    MY_LOGD("%s", strCopyLog.string());

    // use IImageTransform to handle image cropping
    std::unique_ptr<IImageTransform, std::function<void(IImageTransform*)>> transform (
        IImageTransform::createInstance(PIPE_CLASS_TAG, pPackage->miSensorIdx, IImageTransform::OPMode_DisableWPE),
        [](IImageTransform *p)
        {
            if (p)
                p->destroyInstance();
        }
    );

    if (transform.get() == NULL)
    {
        MY_LOGE("IImageTransform is NULL, cannot generate output");
        return MFALSE;
    }

    MBOOL ret = MTRUE;
    transform->setDumpInfo(IImageTransform::DumpInfos(requestNo, frameNo, 0));
    struct timeval current;
    gettimeofday(&current, NULL);
    ret = transform->execute(
            pSource1,
            pCopy1,
            (hasSameSrc) ? pCopy2 : 0,
            rCrop1,
            (hasSameSrc) ? rCrop2 : 0,
            uTrans1,
            (hasSameSrc) ? uTrans2 : 0,
            3000,
            &current,
            pPackage->mbNeedToDump
        );

    if (!ret)
    {
        MY_LOGE("fail to do image transform: first round");
        return MFALSE;
    }

    if (hasCopy2 && !hasSameSrc)
    {
        ret = transform->execute(
                pSource2,
                pCopy2,
                0,
                rCrop2,
                0,
                uTrans2,
                0,
                3000,
                &current,
                pPackage->mbNeedToDump
            );

        if (!ret)
        {
            MY_LOGE("fail to do image transform: second round");
            return MFALSE;
        }
    }

    CAM_TRACE_FMT_END();
    MY_LOGD("(%d) -, R/F Num: %d/%d", nodeId, requestNo, frameNo);
    return MTRUE;
}

IEnqueISPWorker::EnquePackage::~EnquePackage()
{
    /***REFACTOR NOTE: try to use smart pointer to avoid manully delete**/
    if (mpPQParam != nullptr)
    {
        auto DeletePqParam = [] (void* p)
        {
            if (p != nullptr) {
                DpPqParam* pParam = static_cast<DpPqParam*>(p);
                MDPSetting* pSetting = pParam->u.isp.p_mdpSetting;
                if (pSetting != nullptr) {
                    if (pSetting->buffer != nullptr)
                        free(pSetting->buffer);
                    delete pSetting;
                }
                delete pParam;
            }
        };

        DeletePqParam(mpPQParam->WDMAPQParam);
        DeletePqParam(mpPQParam->WROTPQParam);
        delete mpPQParam;
        mpPQParam = NULL;
    }

    if(mpMSSConfig != nullptr)
        delete mpMSSConfig;

    if(mpCrspParam != nullptr)
        delete mpCrspParam;

    auto DeleteModuleInfo = [] (ModuleInfo* pModuleInfo)
    {
        if (pModuleInfo != nullptr) {
            if (pModuleInfo->moduleStruct != nullptr)
                delete static_cast<_SRZ_SIZE_INFO_*>(pModuleInfo->moduleStruct);
            delete pModuleInfo;
        }
    };

    DeleteModuleInfo(mpModuleSRZ4);
    DeleteModuleInfo(mpModuleSRZ3);
}

MVOID
IEnqueISPWorker::updateSensorCropRegion(RequestPtr& pRequest, MRect cropRegion)
{
    const MINT32 requestNo = pRequest->getRequestNo();
    const MINT32 frameNo = pRequest->getFrameNo();
    const MINT32 sensorId = pRequest->getMainSensorIndex();
    MetadataID_T metaId = MID_MAN_OUT_APP;
    sp<MetadataHandle> metaPtr = pRequest->getMetadata(metaId);
    IMetadata* pMetaPtr = (metaPtr != nullptr) ? metaPtr->native() : nullptr;
    if (pMetaPtr == nullptr)
    {
        MY_LOGW("failed to get meta, R/F/S Num: %d/%d/%d, metaId:%d", requestNo, frameNo, sensorId, metaId);
        return;
    }

    auto updateCropRegion = [&](IMetadata::IEntry& entry, IMetadata* meta)
    {
        entry.push_back(cropRegion.p.x, Type2Type<MINT32>());
        entry.push_back(cropRegion.p.y, Type2Type<MINT32>());
        entry.push_back(cropRegion.s.w, Type2Type<MINT32>());
        entry.push_back(cropRegion.s.h, Type2Type<MINT32>());
        meta->update(entry.tag(), entry);
    };

    IMetadata::IEntry sensorCropRegionEntry(MTK_MULTI_CAM_SENSOR_CROP_REGION);
    updateCropRegion(sensorCropRegionEntry, pMetaPtr);
    MY_LOGD("update sensor crop region, R/F Num: %d/%d, sensorIndex:%d, metaId:%d, region:(%d, %d)x(%d, %d)",
        requestNo, frameNo,
        sensorId, metaId,
        cropRegion.p.x, cropRegion.p.y,
        cropRegion.s.w, cropRegion.s.h);
}

MBOOL
IEnqueISPWorker::getCropRegion(shared_ptr<P2EnqueData>& pEnqueData, sp<CropCalculator::Factor> pFactor)
{
    P2EnqueData& enqueData = *pEnqueData.get();
    RequestPtr   pRequest = mInfo->mpRequest;
    const MSize& rImgiSize = enqueData.mIMGI.mpBuf->getImgSize();
    P2ANode* pNode = mInfo->mNode;
    android::sp<CropCalculator> pCropCalculator = pNode->mpCropCalculator;

    if (enqueData.mWDMAO.mHasCrop ||
        enqueData.mWROTO.mHasCrop ||
        enqueData.mIMG2O.mHasCrop ||
        enqueData.mCopy1.mHasCrop ||
        enqueData.mCopy2.mHasCrop)
    {
        pFactor = pCropCalculator->getFactor(enqueData.mpIMetaApp, enqueData.mpIMetaHal, enqueData.mSensorId);
        if(pFactor == nullptr)
        {
            MY_LOGE("can not get crop factor");
            return MFALSE;
        }

        if (enqueData.mpOMetaApp != NULL)
        {
            MRect cropRegion = pFactor->mActiveCrop;
            if (pFactor->mEnableEis)
            {
                cropRegion.p.x += pFactor->mActiveEisMv.p.x;
                cropRegion.p.y += pFactor->mActiveEisMv.p.y;
            }
            // Update crop region to output app metadata
            trySetMetadata<MRect>(enqueData.mpOMetaApp, MTK_SCALER_CROP_REGION, cropRegion);
        }
    }

    std::ostringstream oss;
    auto GetCropRegion = [&](const char* sPort, P2Output& rOut, IImageBuffer* pImg) mutable
    {
        if (pImg == NULL)
            return;
        if (rOut.mHasCrop)
        {
            // Pass if already have
            if (!rOut.mCropRegion.s)
            {
                MSize cropSize = pImg->getImgSize();
                if (rOut.mTrans & eTransform_ROT_90)
                    swap(cropSize.w, cropSize.h);
                /***REFACTOR NOTES: use different name: CropCalculator::evaluate, this is totally differnt function, cannot use same name which is too confuse, THIS IS FUNTION OVERLPOADING MISUSAGE **/
                if (pRequest->hasParameter(PID_IGNORE_CROP))
                {
                    CropCalculator::evaluate(rImgiSize, cropSize, rOut.mCropRegion);
                }
                else if (pRequest->hasParameter(PID_REPROCESSING))
                {
                    CropCalculator::evaluate(rImgiSize, cropSize, pFactor->mActiveCrop, rOut.mCropRegion);
                }
                else
                {
                    pCropCalculator->evaluate(pFactor, cropSize, rOut.mCropRegion, enqueData.mResized, MTRUE);
                }
            }
        }
        else
            rOut.mCropRegion = MRect(rImgiSize.w, rImgiSize.h);

        // output buffer no need to fulfill
        MBOOL isSetNoMargin = MTRUE;
        if (enqueData.mpIMetaApp != NULL)
        {
            tryGetMetadata<MINT32>(enqueData.mpIMetaApp, MTK_CONTROL_CAPTURE_YUV_NO_MARGIN, isSetNoMargin);
        }
        if ((!isSetNoMargin || mInfo->isForceMargin) && pRequest->isTargetBuffer(rOut.mBufId))
        {
            MY_LOGD("buffer Id(%d) is target buffer, setNoMargin(%d), forceMargin(%d)", rOut.mBufId, isSetNoMargin, mInfo->isForceMargin);
            MSize cropSize = rOut.mCropRegion.s;
            if (rOut.mTrans & eTransform_ROT_90)
            {
                swap(cropSize.w, cropSize.h);
            }
            enqueData.mpHolder->mpHolders.push_back(IHolder::createResizerInstance(pImg, cropSize));

            updateSensorCropRegion(pRequest, rOut.mCropRegion);
        }

        oss << sPort << " Rot(" << rOut.mTrans << ") Crop(" << std::dec << rOut.mCropRegion.p.x <<',' << rOut.mCropRegion.p.y<< ")(";
        oss << rOut.mCropRegion.s.w << 'x' << rOut.mCropRegion.s.h << ") Size(" << pImg->getImgSize().w << 'x' << pImg->getImgSize().h;
        oss << ") VA/PA(" << std::hex << std::showbase << pImg->getBufVA(0) << '/' << pImg->getBufPA(0) << ") ";
        oss << "Fmt(" << std::hex << std::showbase << pImg->getImgFormat() << ") ";
        oss << "BufType(" << TypeID2Name(rOut.mTypeId) << "), ";
    };
    GetCropRegion("WDMAO", enqueData.mWDMAO, enqueData.mWDMAO.mpBuf);
    GetCropRegion("WROTO", enqueData.mWROTO, enqueData.mWROTO.mpBuf);
    GetCropRegion("IMG2O", enqueData.mIMG2O, enqueData.mIMG2O.mpBuf);
    GetCropRegion("IMG3O", enqueData.mIMG3O, enqueData.mIMG3O.mpBuf);
    GetCropRegion("TIMGO", enqueData.mTIMGO, enqueData.mTIMGO.mpBuf);
    GetCropRegion("DCESO", enqueData.mDCESO, enqueData.mDCESO.mpBuf);
    GetCropRegion("COPY1", enqueData.mCopy1, enqueData.mCopy1.mpBuf);
    GetCropRegion("COPY2", enqueData.mCopy2, enqueData.mCopy2.mpBuf);

    MY_LOGD("%s", oss.str().c_str());
    return MTRUE;
}

MINT32
IEnqueISPWorker::fnQueryISPProfile(const ProfileMapperKey& key, const EProfileMappingStages& stage)
{
    EIspProfile_T ispProfile;
    P2ANode* pNode = mInfo->mNode;
    if(!pNode->mpISPProfileMapper->mappingProfile(key, stage, ispProfile))
    {
        MY_LOGD("Failed to get ISP Profile in EnqueISPWorker, stage:%d, use EIspProfile_Capture as default", stage);
        ispProfile = EIspProfile_Capture;
    }
    return ispProfile;
}



MBOOL
IEnqueISPWorker::createQParam(EnquePackage* pPackage, TuningParam& tuningParam, NSIoPipe::QParams& qParam)
{
    shared_ptr<P2EnqueData>& pEnqueData = pPackage->mpEnqueData;
    P2EnqueData& enqueData = *pEnqueData.get();
    P2ANode* pNode = mInfo->mNode;
    fillPQParam(pEnqueData, tuningParam, pPackage->mpPQParam);

    // Srz tuning for Imgo (LCE not applied to rrzo)
    if (!enqueData.mResized && enqueData.mLCEI.mpBuf != nullptr)
    {
        auto fillSRZ4 = [&]() -> ModuleInfo*
        {
            // srz4 config
            // ModuleInfo srz4_module;
            ModuleInfo* p = new ModuleInfo();
            p->moduleTag = EDipModule_SRZ4;
            p->frameGroup=0;

            _SRZ_SIZE_INFO_* pSrzParam = new _SRZ_SIZE_INFO_;
            if (enqueData.mLCEI.mpBuf)
            {
                pSrzParam->in_w = enqueData.mLCEI.mpBuf->getImgSize().w;
                pSrzParam->in_h = enqueData.mLCEI.mpBuf->getImgSize().h;
                pSrzParam->crop_w = enqueData.mLCEI.mpBuf->getImgSize().w;
                pSrzParam->crop_h = enqueData.mLCEI.mpBuf->getImgSize().h;
            }
            if (enqueData.mIMGI.mpBuf)
            {
                pSrzParam->out_w = enqueData.mIMGI.mpBuf->getImgSize().w;
                pSrzParam->out_h = enqueData.mIMGI.mpBuf->getImgSize().h;
            }
            p->moduleStruct = reinterpret_cast<MVOID*> (pSrzParam);
            MY_LOGD_IF(pNode->mLogLevel, "SRZ4(in_w/h:%d/%d out_w/h:%d/%d crop_x/y:%d/%d crop_floatX/floatY:%d/%d crop_w/h:%lu/%lu)",
                                          pSrzParam->in_w, pSrzParam->in_h, pSrzParam->out_w, pSrzParam->out_h,
                                          pSrzParam->crop_x, pSrzParam->crop_y, pSrzParam->crop_floatX,
                                          pSrzParam->crop_floatY, pSrzParam->crop_w, pSrzParam->crop_h);
            return p;
        };
        pPackage->mpModuleSRZ4 = fillSRZ4();
    }

    // Srz3 tuning
    if (tuningParam.pFaceAlphaBuf)
    {
        auto fillSRZ3 = [&]() -> ModuleInfo*
        {
            // srz3 config
            // ModuleInfo srz3_module;
            ModuleInfo* p = new ModuleInfo();
            p->moduleTag = EDipModule_SRZ3;
            p->frameGroup=0;

            _SRZ_SIZE_INFO_* pSrzParam = new _SRZ_SIZE_INFO_;

            FACENR_IN_PARAMS in;
            FACENR_OUT_PARAMS out;
            IImageBuffer *facei = (IImageBuffer*)tuningParam.pFaceAlphaBuf;
            in.p2_in    = enqueData.mIMGI.mpBuf->getImgSize();
            in.face_map = facei->getImgSize();
            calculateFACENRConfig(in, out);
            *pSrzParam = out.srz3Param;
            p->moduleStruct = reinterpret_cast<MVOID*>(pSrzParam);
            MY_LOGD_IF(pNode->mLogLevel, "SRZ3(in_w/h:%d/%d out_w/h:%d/%d crop_x/y:%d/%d crop_floatX/floatY:%d/%d crop_w/h:%lu/%lu)",
                                          pSrzParam->in_w, pSrzParam->in_h, pSrzParam->out_w, pSrzParam->out_h,
                                          pSrzParam->crop_x, pSrzParam->crop_y, pSrzParam->crop_floatX,
                                          pSrzParam->crop_floatY, pSrzParam->crop_w, pSrzParam->crop_h);
            return p;
        };
        pPackage->mpModuleSRZ3 = fillSRZ3();
    }

    // fill MSSConfig
    if(pPackage->mpMSSConfig != nullptr)
    {
        pPackage->mpMSSConfig->mss_scenario = NSIoPipe::NSMFB20::MSS_P2_DL_MODE;
        if(enqueData.mIMG3O.mpBuf == nullptr)
        {
            MY_LOGE("IMG3O is nullptr, no output for mss scale0.");
            return MFALSE;
        }
        pPackage->mpMSSConfig->mss_scaleFrame.push_back(enqueData.mIMG3O.mpBuf); //Scale 0
        pPackage->mpMSSConfig->mss_scaleFrame.push_back(enqueData.mMSSO.mpBuf);  //Scale 1
    }

    // QParam generate
    QParamGenerator qParamGen = QParamGenerator(enqueData.mSensorId,
                enqueData.mTimeSharing ? ENormalStreamTag_Vss : ENormalStreamTag_Cap);

    qParamGen.addInput(PORT_IMGI, enqueData.mIMGI.mpBuf);

    if (pPackage->mpModuleSRZ3 != NULL)
    {
        qParamGen.addModuleInfo(EDipModule_SRZ3, pPackage->mpModuleSRZ3->moduleStruct);
        qParamGen.addInput(PORT_YNR_FACEI, static_cast<IImageBuffer*>(tuningParam.pFaceAlphaBuf));
    }

    MBOOL bYuvTunning = enqueData.mRound == 2 || enqueData.mYuvRep;
    if (!bYuvTunning && !enqueData.mResized && tuningParam.pLsc2Buf != NULL)
    {
        qParamGen.addInput(PORT_LSCI, static_cast<IImageBuffer*>(tuningParam.pLsc2Buf));
    }

    if (!bYuvTunning && !enqueData.mResized && enqueData.mLCEI.mpBuf != NULL)
    {
        qParamGen.addInput(PORT_LCEI, enqueData.mLCEI.mpBuf);
        if (pPackage->mpModuleSRZ4 != NULL)
        {
            qParamGen.addModuleInfo(EDipModule_SRZ4, pPackage->mpModuleSRZ4->moduleStruct);
            qParamGen.addInput(PORT_YNR_LCEI, enqueData.mLCEI.mpBuf);
        }
    }

    if (!bYuvTunning && tuningParam.pBpc2Buf != NULL)
    {
        qParamGen.addInput(PORT_BPCI, static_cast<IImageBuffer*>(tuningParam.pBpc2Buf));
    }

    if (enqueData.mIMG2O.mpBuf != NULL)
    {
        qParamGen.addOutput(PORT_IMG2O, enqueData.mIMG2O.mpBuf, 0);
        qParamGen.addCrop(eCROP_CRZ, enqueData.mIMG2O.mCropRegion.p, enqueData.mIMG2O.mCropRegion.s, enqueData.mIMG2O.mpBuf->getImgSize());
    }

    if (enqueData.mIMG3O.mpBuf != NULL)
    {
        qParamGen.addOutput(PORT_IMG3O, enqueData.mIMG3O.mpBuf, 0);
        if (enqueData.mIMG3O.mHasCrop)
        {
            CrspInfo* pCrspParam = new CrspInfo();
            pPackage->mpCrspParam = pCrspParam;
            pCrspParam->m_CrspInfo.p_integral = enqueData.mIMG3O.mCropRegion.p;
            pCrspParam->m_CrspInfo.s = enqueData.mIMG3O.mCropRegion.s;
            qParamGen.addExtraParam(EPIPE_IMG3O_CRSPINFO_CMD, (MVOID*)pCrspParam);
            MY_LOGD("set IMG3O crop region (%d, %d)(%d x %d)",
                    pCrspParam->m_CrspInfo.p_integral.x, pCrspParam->m_CrspInfo.p_integral.y,
                    pCrspParam->m_CrspInfo.s.w, pCrspParam->m_CrspInfo.s.h);
        }
    }

    if (enqueData.mTIMGO.mpBuf != NULL)
    {
        qParamGen.addOutput(PORT_TIMGO, enqueData.mTIMGO.mpBuf, 0);
    }

    if (enqueData.mDCESO.mpBuf != NULL)
    {
        qParamGen.addOutput(PORT_DCESO, enqueData.mDCESO.mpBuf, 0);
    }

    if (enqueData.mWROTO.mpBuf != NULL)
    {
        qParamGen.addOutput(PORT_WROTO, enqueData.mWROTO.mpBuf, enqueData.mWROTO.mTrans);
        qParamGen.addCrop(eCROP_WROT, enqueData.mWROTO.mCropRegion.p, enqueData.mWROTO.mCropRegion.s, enqueData.mWROTO.mpBuf->getImgSize());
    }

    if (enqueData.mWDMAO.mpBuf != NULL)
    {
        qParamGen.addOutput(PORT_WDMAO, enqueData.mWDMAO.mpBuf, 0);
        qParamGen.addCrop(eCROP_WDMA, enqueData.mWDMAO.mCropRegion.p, enqueData.mWDMAO.mCropRegion.s, enqueData.mWDMAO.mpBuf->getImgSize());
    }

    qParamGen.addExtraParam(EPIPE_MDP_PQPARAM_CMD, (MVOID*)pPackage->mpPQParam);

    if (pNode->mDumpControl.mDumpTimgo)
    {
         qParamGen.addExtraParam(EPIPE_TIMGO_DUMP_SEL_CMD, (MVOID*)&pNode->mDumpControl.mDumpTimgoType);
         MY_LOGD("set debug TimgoType(%u)", pNode->mDumpControl.mDumpTimgoType);
    } else if (enqueData.mTIMGO.mpBuf != nullptr)
    {
        MINT fmt = enqueData.mTIMGO.mpBuf->getImgFormat();
        if(isHalRawFormat((EImageFormat)fmt))
        {
            pPackage->mTimgoType = EDIPTimgoDump_AFTER_LTM;
            qParamGen.addExtraParam(EPIPE_TIMGO_DUMP_SEL_CMD, (MVOID*)&(pPackage->mTimgoType));
            MY_LOGD("Raw format(%x), set TimgoType(%u)", fmt, pPackage->mTimgoType);
        }
        else
        {
            MY_LOGW("nonRaw format(%x) is set", fmt);
        }
    }

    // add for AINR 16bits
    if (enqueData.mpHolder->mpRequest->hasFeature(FID_AINR) &&
        enqueData.mIspStage == eStage_MSNR_BFBLD_NN16)
    {
        pPackage->mRawDepthType = EDIPRAWDepthType_BEFORE_DGN;
        pPackage->mRawHDRType = EDIPRawHDRType_ISPHDR;

        // add for dgn sel
        qParamGen.addExtraParam(EPIPE_RAW_DEPTHTYPE_CMD,
                (MVOID*)&(pPackage->mRawDepthType));
        // add for high bit mode
        qParamGen.addExtraParam(EPIPE_RAW_HDRTYPE_CMD,
                (MVOID*)&(pPackage->mRawHDRType));
        MY_LOGD("AINR-16bits R2Y set DGN in + high bits mode");
    }

    // MSS extra param
    if(pPackage->mpMSSConfig != nullptr)
        qParamGen.addExtraParam(EPIPE_MSS_INFO_CMD, (MVOID*)(pPackage->mpMSSConfig));

    if(enqueData.mTIMGO.mpBuf != nullptr && !pNode->mDumpControl.mDumpTimgo)
    {
        // check the current RAW type got from P1
        MINT32 iP2RawType = -1;
        tryGetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_ISP_P2_PROCESSED_RAW, iP2RawType);
        MINT32 iAPRawType = -1;
        tryGetMetadata<MINT32>(enqueData.mpIMetaApp, MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE, iAPRawType);
        // exclude the case that pTIMGO is set by dump debug TIMGO,
        // which tuning setiing should not be changed.
        MINT fmt = enqueData.mTIMGO.mpBuf->getImgFormat();
        if (isHalRawFormat((EImageFormat)fmt))
        {
            // this case should not be occurred
            if (iP2RawType == 1 && iAPRawType == 0)
            {
                CAM_ULOGM_FATAL("wrong APPMeta MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE might be used ");
            }
            // processed raw is needed
            else if (iAPRawType > 0)
            {
                qParamGen.insertTuningBuf(pPackage->mpTuningData->mpVA);
                trySetMetadata<MINT32>(enqueData.mpOMetaHal, MTK_P1NODE_RAW_TYPE, 0);
                // reset MTK_ISP_P2_PROCESSED_RAW to 0 after setP2ISP
                trySetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_ISP_P2_PROCESSED_RAW, 0);
                MY_LOGD("P2ANode output processed raw");
            }
            // pure raw
            else
            {
                qParamGen.insertTuningBuf(nullptr);
                MY_LOGD("P2ANode output pure raw");
            }
        }
        else
        {
            MY_LOGE("nonRaw format(%x) is set", fmt);
            qParamGen.insertTuningBuf(nullptr);
        }
    }
    else
    {
        qParamGen.insertTuningBuf(pPackage->mpTuningData->mpVA);
    }


    QParamGenerator::FrameInfo info = {.FrameNo=static_cast<MUINT32>(mInfo->mFrameNo),
                                       .RequestNo=static_cast<MUINT32>(mInfo->mRequestNo),
                                       .Timestamp=static_cast<MUINT32>(enqueData.mTaskId),
                                       .UniqueKey=enqueData.mUniqueKey,
                                       .IspProfile=enqueData.mIspProfile,
                                       .SensorID=enqueData.mSensorId };
    auto scenario = pPackage->mbNeedToDump ? DUMP_FEATURE_CAPTURE : DUMP_FEATURE_OFF;
    qParamGen.setInfo(info, scenario);
    auto ret = qParamGen.generate(qParam);

    return ret;
}

MVOID
IEnqueISPWorker::fillPQParam(shared_ptr<P2EnqueData>& pEnqueData, TuningParam& tuningParam, PQParam* pPQParam)
{
    P2EnqueData& enqueData = *pEnqueData.get();
    MINT32 iIsoValue = 0;
    P2ANode* pNode = mInfo->mNode;
    if (!tryGetMetadata<MINT32>(enqueData.mpIMetaDynamic, MTK_SENSOR_SENSITIVITY, iIsoValue))
    {
        MY_LOGW("can not get iso value");
    }

    MINT32 iMagicNum = 0;
    if (!tryGetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_P1NODE_PROCESSOR_MAGICNUM, iMagicNum))
    {
        MY_LOGW("can not get magic number");
    }

    MINT32 iRealLv = 0;
    if (!tryGetMetadata<MINT32>(enqueData.mpIMetaHal, MTK_REAL_LV, iRealLv))
    {
        MY_LOGW("can not get read lv");
    }

    auto fillPQ = [&](DpPqParam& rPqParam, PortID port, MBOOL enableCZ, MBOOL enableHFG, TypeID_T typeID)
    {
        String8 strEnqueLog;
        strEnqueLog += String8::format("Port(%d) Timestamp:%d",port.index ,enqueData.mUniqueKey);

        rPqParam.enable           = false;
        rPqParam.scenario         = MEDIA_ISP_CAPTURE;
        rPqParam.u.isp.iso        = iIsoValue;
        rPqParam.u.isp.lensId     = enqueData.mSensorId;
        rPqParam.u.isp.LV         = iRealLv;
        rPqParam.u.isp.timestamp  = enqueData.mUniqueKey;
        rPqParam.u.isp.frameNo    = enqueData.mFrameNo;
        rPqParam.u.isp.requestNo  = enqueData.mRequestNo;

        // Only the main full yuv need to notify the generate run/apply run,
        // bypass the RSZ edge enhance in "generate-run", to avoid the double-edge-enhance issue
        if(typeID == TID_MAN_FULL_YUV && PlatCap::isSupport(PlatCap::HWCap_DRE) && enqueData.mEnableDRE)
        {
            strEnqueLog += String8::format(" ISP_FIRST_RSZ_ETC_DISABLE ");
            rPqParam.u.isp.ispTimesInfo =  ISP_FIRST_RSZ_ETC_DISABLE;
        }
        else
        {
            strEnqueLog += String8::format(" ISP_SECOND_RSZ_ETC_ENABLE ");
            rPqParam.u.isp.ispTimesInfo =  ISP_SECOND_RSZ_ETC_ENABLE;
        }

        if (enqueData.mRound > 1)
        {
            strncpy(rPqParam.u.isp.userString, "round2", sizeof("round2"));
        }

        if (PlatCap::isSupport(PlatCap::HWCap_ClearZoom) && enableCZ)
        {
            rPqParam.enable = (PQ_ULTRARES_EN);
            ClearZoomParam& rCzParam = rPqParam.u.isp.clearZoomParam;

            rCzParam.captureShot = CAPTURE_SINGLE;

            // Pass this part if user load
            // Be here only if don't apply DRE.
            if (pNode->mDumpControl.mDebugDumpMDP)
            {
                rPqParam.u.isp.p_mdpSetting = new MDPSetting();
                rPqParam.u.isp.p_mdpSetting->size = MDPSETTING_MAX_SIZE;
                rPqParam.u.isp.p_mdpSetting->buffer = ::malloc(MDPSETTING_MAX_SIZE);
                rPqParam.u.isp.p_mdpSetting->offset = 0;
                if (rPqParam.u.isp.p_mdpSetting->buffer != NULL)
                    memset(rPqParam.u.isp.p_mdpSetting->buffer, 0xFF, MDPSETTING_MAX_SIZE);
                else
                    MY_LOGW("cannot allocate buffer for mdpSetting");
            }

            MUINT32 idx = 0;
            rCzParam.p_customSetting = (void*)getTuningFromNvram(enqueData.mSensorId, idx, iMagicNum, NVRAM_TYPE_CZ, enqueData.mpIMetaHal, false, pNode->mUsageHint.mIsHidlIsp);
            strEnqueLog += String8::format(" CZ Capture:%d idx:0x%x",
                                            rCzParam.captureShot,
                                            idx);
        }

        if (PlatCap::isSupport(PlatCap::HWCap_DRE) && enqueData.mEnableDRE)
        {
            rPqParam.enable |= (PQ_DRE_EN);
            rPqParam.scenario = MEDIA_ISP_CAPTURE;

            DpDREParam& rDreParam = rPqParam.u.isp.dpDREParam;
            rDreParam.cmd         = DpDREParam::Cmd::Initialize | DpDREParam::Cmd::Generate;
            rDreParam.userId      = generatePQUserID(rPqParam.u.isp.requestNo, rPqParam.u.isp.lensId, mInfo->mpRequest->isPhysicalStream());
            rDreParam.buffer      = nullptr;
            MUINT32 idx = 0;
            rDreParam.p_customSetting = (void*)getTuningFromNvram(enqueData.mSensorId, idx, iMagicNum, NVRAM_TYPE_DRE, enqueData.mpIMetaHal, false, pNode->mUsageHint.mIsHidlIsp);
            rDreParam.customIndex     = idx;
            strEnqueLog += String8::format(" DRE cmd:0x%x userId::%llu buffer:%p p_customSetting:%p idx:%d",
                            rDreParam.cmd, rDreParam.userId, rDreParam.buffer, rDreParam.p_customSetting, idx);
        }

        if (PlatCap::isSupport(PlatCap::HWCap_HFG) & enableHFG)
        {
            rPqParam.enable |= (PQ_HFG_EN);
            DpHFGParam& rHfgParam = rPqParam.u.isp.dpHFGParam;

            MUINT32 idx = 0;
            rHfgParam.p_lowerSetting = (void*)getTuningFromNvram(enqueData.mSensorId, idx, iMagicNum, NVRAM_TYPE_HFG, enqueData.mpIMetaHal, false, pNode->mUsageHint.mIsHidlIsp);
            rHfgParam.p_upperSetting = rHfgParam.p_lowerSetting ;
            rHfgParam.lowerISO = iIsoValue;
            rHfgParam.upperISO = iIsoValue;
            rHfgParam.p_slkParam = tuningParam.pRegBuf;

            strEnqueLog += String8::format(" HFG idx:%d", idx);
        }

        strEnqueLog += String8::format(" PQ Enable:0x%x scenario:%d iso:%d",
                                        rPqParam.enable,
                                        rPqParam.scenario,
                                        iIsoValue);

        MY_LOGD("%s", strEnqueLog.string());
    };
    // WROT
    if (enqueData.mWROTO.mpBuf != NULL)
    {
        DpPqParam* pIspParam_WROT = new DpPqParam();
        fillPQ(*pIspParam_WROT, PORT_WROTO, enqueData.mWROTO.mEnableCZ, enqueData.mWROTO.mEnableHFG, enqueData.mWROTO.mTypeId);
        pPQParam->WROTPQParam = static_cast<void*>(pIspParam_WROT);
    }

    // WDMA
    if (enqueData.mWDMAO.mpBuf != NULL)
    {
        DpPqParam* pIspParam_WDMA = new DpPqParam();
        fillPQ(*pIspParam_WDMA, PORT_WDMAO, enqueData.mWDMAO.mEnableCZ, enqueData.mWDMAO.mEnableHFG, enqueData.mWDMAO.mTypeId);
        pPQParam->WDMAPQParam = static_cast<void*>(pIspParam_WDMA);
    }
}

MVOID
IEnqueISPWorker::unpackRawBuffer(IImageBuffer* pInPackBuf, IImageBuffer* pOutUnpackBuf )
{
    DngOpResultInfo dngResult;
    DngOpImageInfo dngImage;

    int i4ImgWidth  = pInPackBuf->getImgSize().w;
    int i4ImgHeight = pInPackBuf->getImgSize().h;
    int i4BufSize   = pInPackBuf->getBufSizeInBytes(0);
    int i4ImgStride = pInPackBuf->getBufStridesInBytes(0);
    MUINT32 u4BufSize = DNGOP_BUFFER_SIZE(i4ImgWidth * 2, i4ImgHeight);

    // unpack algorithm
    MY_LOGD("Unpack +");
    MTKDngOp *pDngOp = MTKDngOp::createInstance(DRV_DNGOP_UNPACK_OBJ_SW);
    if (pDngOp == nullptr) {
         MY_LOGW("Create MTKDngOp failed");
         return;
     }
    dngImage.Width = i4ImgWidth;
    dngImage.Height = i4ImgHeight;
    dngImage.Stride_src = i4ImgStride;
    dngImage.Stride_dst = pOutUnpackBuf->getBufStridesInBytes(0);
    dngImage.BIT_NUM = 10;
    dngImage.BIT_NUM_DST = 10;
    dngImage.Buff_size = u4BufSize;
    dngImage.srcAddr     = (void *) (pInPackBuf->getBufVA(0));
    dngResult.ResultAddr = (void *) (pOutUnpackBuf->getBufVA(0));
    pDngOp->DngOpMain((void*)&dngImage, (void*)&dngResult);
    pDngOp->destroyInstance(pDngOp);
    MY_LOGD("Unpack -");
    MY_LOGD("unpack processing. va[in]:%p, va[out]:%p", dngImage.srcAddr, dngResult.ResultAddr);
    MY_LOGD("img size(%dx%d) src stride(%d) bufSize(%d) -> dst stride(%d) bufSize(%zu)", i4ImgWidth, i4ImgHeight,
             dngImage.Stride_src,i4BufSize, dngImage.Stride_dst , pOutUnpackBuf->getBufSizeInBytes(0));
}

MBOOL
IEnqueISPWorker::isCovered(P2Output& rSrc, MDPOutput& rDst)
{
    if (rSrc.mpBuf == NULL || rDst.mpBuf == NULL)
        return MFALSE;

    // Ignore grey image
    MINT srcFmt = rSrc.mpBuf->getImgFormat();
    if (srcFmt == eImgFmt_Y8)
        return MFALSE;

    MSize& srcCrop = rSrc.mCropRegion.s;
    MSize& dstCrop = rDst.mCropRegion.s;
    // Make sure that the source FOV covers the destination FOV
    #define FOV_THRES (1)
        if (srcCrop.w * (100 + FOV_THRES) < dstCrop.w * 100 ||
            srcCrop.h * (100 + FOV_THRES) < dstCrop.h * 100)
            return MFALSE;
    #undef FOV_THRES

    MSize srcSize = rSrc.mpBuf->getImgSize();
    MSize dstSize = rDst.mpBuf->getImgSize();
    if (rSrc.mTrans & eTransform_ROT_90)
        swap(srcSize.w, srcSize.h);

    if (rDst.mTrans & eTransform_ROT_90)
        swap(dstSize.w, dstSize.h);

    // Make sure that the source image is bigger than destination image
    #define SIZE_THRES (5)
        if (srcSize.w * (100 + SIZE_THRES) < dstSize.w * 100 ||
            srcSize.h * (100 + SIZE_THRES) < dstSize.h * 100)
            return MFALSE;
    #undef SIZE_THRES

    simpleTransform tranCrop2SrcBuf(rSrc.mCropRegion.p, rSrc.mCropRegion.s, srcSize);
    MRect& rCropRegion = rDst.mSourceCrop;
    rCropRegion = transform(tranCrop2SrcBuf, rDst.mCropRegion);

    // Boundary Check for FOV tolerance
    if (rCropRegion.p.x < 0)
        rCropRegion.p.x = 0;
    if (rCropRegion.p.y < 0)
        rCropRegion.p.y = 0;

    if ((rCropRegion.p.x + rCropRegion.s.w) > srcSize.w)
        rCropRegion.s.w = srcSize.w - rCropRegion.p.x;
    if ((rCropRegion.p.y + rCropRegion.s.h) > srcSize.h)
        rCropRegion.s.h = srcSize.h - rCropRegion.p.y;

    // Make 2 bytes alignment
    rCropRegion.s.w &= ~(0x1);
    rCropRegion.s.h &= ~(0x1);

    if (rSrc.mTrans & eTransform_ROT_90)
    {
        swap(rCropRegion.p.x, rCropRegion.p.y);
        swap(rCropRegion.s.w, rCropRegion.s.h);
    }

    rDst.mpSource = rSrc.mpBuf;
    MUINT32& rSrcTrans = rSrc.mTrans;
    MUINT32& rDstTrans = rDst.mTrans;
    // Use XOR to calucate the transform, where the source image has a rotation or flip
    rDst.mSourceTrans = rSrcTrans ^ rDstTrans;
    // MDP does image rotation after flip.
    // If the source has a rotation and the destinatio doesn't, do the exceptional rule
    if (rSrcTrans & eTransform_ROT_90 && ~rDstTrans & eTransform_ROT_90)
    {
        if ((rSrcTrans & eTransform_ROT_180) == (rDstTrans & eTransform_ROT_180))
            rDst.mSourceTrans = eTransform_ROT_90 | eTransform_FLIP_V | eTransform_FLIP_H;
        else if ((rSrcTrans & eTransform_ROT_180) == (~rDstTrans & eTransform_ROT_180))
            rDst.mSourceTrans = eTransform_ROT_90;
    }
    return MTRUE;
}

MVOID
IEnqueISPWorker::onDeviceTuning(shared_ptr<P2EnqueData>& pEnqueData)
{
    P2EnqueData& enqueData = *pEnqueData.get();
    FileReadRule rule;
    String8      profileName;
    MINT32       index;
    RequestPtr   pRequest = mInfo->mpRequest;

    if (   pRequest->hasFeature(FID_AINR)
        || pRequest->hasFeature(FID_AINR_YHDR)
        || pRequest->hasFeature(FID_AIHDR))
    {
        // P2B is for single path, P2A is for Main path
        MY_LOGD("AINR Main path");
        index = 0;
        profileName = String8::format("EIspProfile_AINR_Main");
        rule.getFile_LCSO(index, profileName.string(), enqueData.mLCEI.mpBuf, "P2Node", enqueData.mSensorId);
    }
    else
    {
        if (pRequest->hasFeature(FID_MFNR))
        { // MFNR
            profileName = String8::format("MFNR");
            index       = pRequest->getParameter(PID_FRAME_INDEX_FORCE_BSS);
        }
        else
        {
            profileName = String8::format("single_capture");
            index       = 0;
        }
        rule.getFile_RAW(index, profileName.string(), enqueData.mIMGI.mpBuf, "P2Node", enqueData.mSensorId);
        rule.getFile_LCSO(index, profileName.string(), enqueData.mLCEI.mpBuf, "P2Node", enqueData.mSensorId);
    }
}

MBOOL
IEnqueISPWorker::getCrop(P2EnqueData& rEnqueData, BufferID_T typeId, MRect& cropRegion, MUINT32& trans, MUINT32& scaleRatio, MBOOL& needCrop)
{
    P2ANode* pNode = mInfo->mNode;
    sp<CropCalculator::Factor> pFactor = pNode->mpCropCalculator->getFactor(mInfo->mpIMetaApp, mInfo->mpIMetaHal, mInfo->mSensorId);
    sp<CaptureFeatureNodeRequest> pNodeRequest = mInfo->mpNodeRequest;
    RequestPtr& pRequest = mInfo->mpRequest;
    BufferID_T bufId   = pNodeRequest->mapBufferID(typeId, OUTPUT);
    IImageBuffer* pBuf = pNodeRequest->acquireBuffer(bufId);
    MSize srcSize      = rEnqueData.mIMGI.mpBuf->getImgSize();
    const MBOOL isCroppedFSYUV = (pRequest->hasParameter(PID_CROPPED_FSYUV) ? (pRequest->getParameter(PID_CROPPED_FSYUV) > 0) : MFALSE);
    MBOOL bFullByImg3o = (pRequest->hasFeature(FID_NR) && (pRequest->getActiveFrameCount() == 1)) ||
                         (pRequest->hasFeature(FID_MFNR) && (pRequest->getActiveFrameCount() > 1));

    needCrop               = typeId == TID_JPEG             ||
                             typeId == TID_MAN_CROP1_YUV    ||
                             typeId == TID_MAN_CROP2_YUV    ||
                             typeId == TID_MAN_CROP3_YUV    ||
                             typeId == TID_MAN_FD_YUV       ||
                             typeId == TID_POSTVIEW         ||
                             typeId == TID_MAN_CLEAN        ||
                             typeId == TID_THUMBNAIL;

    MBOOL needDualCamCrop  = typeId == TID_MAN_FULL_YUV      ||
                             typeId == TID_MAN_CROP1_YUV     ||
                             typeId == TID_MAN_CROP2_YUV     ||
                             typeId == TID_MAN_IMGO_RSZ_YUV  ||
                             typeId == TID_POSTVIEW          ||
                             typeId == TID_MAN_CLEAN         ||
                             typeId == TID_MAN_FULL_PURE_YUV ||
                             typeId == TID_MAN_SPEC_YUV;


    auto GetCustomedCrop = [&](MRect& rCrop) mutable
    {
        FovCalculator::FovInfo fovInfo;

        if (pNode->mpFOVCalculator->getIsEnable() && pNode->mpFOVCalculator->getFovInfo(mInfo->mSensorId, fovInfo))
        {
            needCrop = MTRUE;
            rCrop.p = fovInfo.mFOVCropRegion.p;
            rCrop.s = fovInfo.mFOVCropRegion.s;

            if (typeId == TID_POSTVIEW)
            {
                // calculate crop region
                const MSize srcSize = MSize(fovInfo.mFOVCropRegion.s.w, fovInfo.mFOVCropRegion.s.h);
                const MSize imgSize = pBuf->getImgSize();
                const MSize dstSize = MSize(imgSize.w, imgSize.h);

                MRect srcCrop;
                // pillarbox
                if ((srcSize.w*dstSize.h) > (srcSize.h*dstSize.w))
                {
                    srcCrop.s.w = div_round(srcSize.h*dstSize.w, dstSize.h);
                    srcCrop.s.h = srcSize.h;
                    srcCrop.p.x = ((srcSize.w - srcCrop.s.w) >> 1);
                    srcCrop.p.y = 0;
                }
                // letterbox
                else
                {
                    srcCrop.s.w = srcSize.w;
                    srcCrop.s.h = div_round(srcSize.w*dstSize.h, dstSize.w);
                    srcCrop.p.x = 0;
                    srcCrop.p.y = ((srcSize.h - srcCrop.s.h) >> 1);
                }
                // add offset
                rCrop.p.x = fovInfo.mFOVCropRegion.p.x + srcCrop.p.x;
                rCrop.p.y = fovInfo.mFOVCropRegion.p.y + srcCrop.p.y;
                rCrop.s = srcCrop.s;
            }

            MY_LOGD("VSDOF Sensor(%d) FOV Crop(%d,%d)(%dx%d) TYPE Id(%s)",
                        mInfo->mSensorId,
                        fovInfo.mFOVCropRegion.p.x,
                        fovInfo.mFOVCropRegion.p.y,
                        fovInfo.mFOVCropRegion.s.w,
                        fovInfo.mFOVCropRegion.s.h,
                        TypeID2Name(typeId));
        }
    };

    auto GetScaleCrop = [&](MRect& rCrop, MUINT32& rScaleRatio) mutable
    {
        if (needCrop)
        {
            MSize imgSize = pBuf->getImgSize();
            if (trans & eTransform_ROT_90)
                swap(imgSize.w, imgSize.h);

            if (pRequest->hasParameter(PID_IGNORE_CROP))
            {
                CropCalculator::evaluate(srcSize, imgSize, rCrop);
            }
            else if (pRequest->hasParameter(PID_REPROCESSING))
            {
                CropCalculator::evaluate(srcSize, imgSize, pFactor->mActiveCrop, rCrop);
            }
            else
            {
                pNode->mpCropCalculator->evaluate(pFactor, imgSize, rCrop, MFALSE);
            }
            rScaleRatio = imgSize.w * 100 / rCrop.s.w;
        }
        else
        {
            rCrop = MRect(srcSize.w, srcSize.h);
            rScaleRatio = 1;
        }
    };

    #if MTKCAM_EARLY_FLIP_SUPPORT == 1
    {
        // Do the flip in P2A
        auto doFlipInP2A = [&](MUINT32& trans, MINT32 dvOri) -> MVOID
        {
            if (dvOri == 90 || dvOri == 270)
            {
                MY_LOGD("Capture vertical");
                trans = eTransform_FLIP_V;
            }
            if (dvOri == 0 || dvOri == 180)
            {
                MY_LOGD("Capture horizontal");
                trans = eTransform_FLIP_H;
            }
        };

        if(!bFullByImg3o && (typeId == TID_MAN_FULL_YUV))
        {
            MINT32 jpegFlipProp = property_get_int32("vendor.debug.camera.Jpeg.flip", 0);
            MINT32 jpegFlip = 0;
            if(!tryGetMetadata<MINT32>(mInfo->mpIMetaApp, MTK_CONTROL_CAPTURE_JPEG_FLIP_MODE, jpegFlip))
                MY_LOGD("cannot get MTK_CONTROL_CAPTURE_JPEG_FLIP_MODE");
            if (jpegFlip || jpegFlipProp)
            {
                MINT32 jpegOri = 0;
                tryGetMetadata<MINT32>(mInfo->mpIMetaApp, MTK_JPEG_ORIENTATION, jpegOri);
                MY_LOGD("before trans 0x%" PRIx64, trans);
                doFlipInP2A(trans, jpegOri);
                MY_LOGD("after trans 0x%" PRIx64, trans);
                trySetMetadata<MINT32>(mInfo->mpOMetaHal, MTK_FEATURE_FLIP_IN_P2A, 1);
            }
        }
    }
    #endif

    // Apply customized crop region to full-size domain
    if (mInfo->isVsdof && needDualCamCrop)
    {
        GetCustomedCrop(cropRegion);
    }
    else if(isCroppedFSYUV && (typeId == TID_MAN_FULL_YUV))
    {
        needCrop = MTRUE;
        MSize imgSize = pBuf->getImgSize();
        pNode->mpCropCalculator->evaluate(pFactor, imgSize, cropRegion, MFALSE);
        MY_LOGD("CropedFSYUV Sensor(%d) FOV Crop (%d, %d, %d, %d), dstSize:(%d, %d)",
                    mInfo->mSensorId,
                    cropRegion.p.x, cropRegion.p.y,
                    cropRegion.s.w, cropRegion.s.h,
                    imgSize.w, imgSize.h);
    }
    else
    {
        GetScaleCrop(cropRegion, scaleRatio);
    }

    return MTRUE;
}


MBOOL
IEnqueISPWorker::fMainFullCommon(shared_ptr<P2EnqueData>& pEnqueData, const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    P2EnqueData& rEnqueData = *pEnqueData.get();
    rEnqueData.mpHolder     = mInfo->pHolder;
    rEnqueData.mpSharedData = mInfo->pSharedData;
    RequestPtr& pRequest    = mInfo->mpRequest;
    P2ANode* pNode          = mInfo->mNode;
    sp<CaptureFeatureNodeRequest> pNodeRequest = mInfo->mpNodeRequest;
    prepareLTMSettingData(rEnqueData.mIMGI.mpBuf, mInfo->mpIMetaHal, rEnqueData.mLTMSettings);

    // the larger size has higher priority, the smaller size could using larger image to copy via MDP
    const TypeID_T typeIds[] =
    {
            TID_MAN_FULL_YUV,
            TID_MAN_FULL_PURE_YUV,
            TID_JPEG,
            TID_MAN_CROP1_YUV,
            TID_MAN_CROP2_YUV,
            TID_MAN_CROP3_YUV,
            TID_MAN_IMGO_RSZ_YUV,
            TID_MAN_FD_YUV,
            TID_POSTVIEW,
            TID_THUMBNAIL,
            TID_MAN_SPEC_YUV,
            TID_MAN_CLEAN
    };

    MBOOL hasCopyBuffer = MFALSE;
    MBOOL bFullByImg3o = pRequest->hasFeature(FID_NR)
                        || (pRequest->hasFeature(FID_MFNR) && (pRequest->getActiveFrameCount() > 1));

    // use to count the PQ support size
    MINT8 curUsablePQSize = SUPPORT_PQ_SIZE;
    MBOOL hasOutput = MFALSE;
    for (TypeID_T typeId : typeIds)
    {
        BufferID_T bufId = pNodeRequest->mapBufferID(typeId, OUTPUT);
        if (bufId == NULL_BUFFER)
            continue;

        IImageBuffer* pBuf = pNodeRequest->acquireBuffer(bufId);
        if (pBuf == NULL)
        {
            MY_LOGE("should not have null buffer. type:%d, buf:%d",typeId, bufId);
            continue;
        }

        MUINT32 trans = pNodeRequest->getImageTransform(bufId);
        MRect cropRegion;
        MUINT32 scaleRatio = 1;
        MBOOL needCrop = MFALSE;
        getCrop(rEnqueData, typeId, cropRegion, trans, scaleRatio, needCrop);

        auto SetOutput = [&](P2Output& o, MBOOL isMDPDMA=MFALSE)
        {
            hasOutput = MTRUE;
            o.mpBuf = pBuf;
            o.mBufId = bufId;
            o.mTypeId = typeId;
            o.mHasCrop = needCrop;
            if (isMDPDMA && curUsablePQSize>0 &&
                (typeId == TID_JPEG || typeId == TID_MAN_CROP1_YUV
                 || typeId == TID_MAN_CROP2_YUV || typeId == TID_MAN_CROP3_YUV))
            {
                o.mEnableCZ = pRequest->hasFeature(FID_CZ);
                o.mEnableHFG = pRequest->hasFeature(FID_HFG);
                curUsablePQSize--;
            }
            o.mTrans = trans;
            o.mCropRegion = cropRegion;
            o.mScaleRatio = scaleRatio;
        };

        auto UsedOutput = [&](P2Output& o) -> MBOOL
        {
            return o.mBufId != NULL_BUFFER || o.mpBuf != NULL;
        };

        auto AcceptedByIMG3O = [&]() -> MBOOL
        {
            const MSize& rDstSize = pBuf->getImgSize();
            const MSize& rSrcSize = rEnqueData.mIMGI.mpBuf->getImgSize();
            if (needCrop)
            {
                // IMG3O supports cropping but no resizing
                if (cropRegion.s == rDstSize)
                {
                    return MTRUE;
                }
            }
            else if (rSrcSize == rDstSize)
            {
                return MTRUE;
            }
            return MFALSE;
        };

        auto AcceptedByIMG2O = [&]() -> MBOOL {
            const MSize& rDstSize = pBuf->getImgSize();
            const MSize& cropSize = cropRegion.s;
            MBOOL ret = MTRUE;
            // size after crop cannot smaller than output size
            if (needCrop)
                ret = ((cropSize.w >= rDstSize.w) && (cropSize.h >= rDstSize.h));
            return ret;
        };

        // Use P2 resizer to serve FD or thumbnail buffer,
        if (!UsedOutput(rEnqueData.mIMG2O) && (typeId == TID_MAN_FD_YUV || typeId == TID_THUMBNAIL) && pBuf->getImgFormat() == eImgFmt_YUY2 && AcceptedByIMG2O())
        {
            SetOutput(rEnqueData.mIMG2O);
        }
        else if (typeId == TID_MAN_FULL_YUV && bFullByImg3o && AcceptedByIMG3O() && trans == 0)
        {
            SetOutput(rEnqueData.mIMG3O);
        }
        else if (!UsedOutput(rEnqueData.mWDMAO) && trans == 0)
        {
            SetOutput(rEnqueData.mWDMAO, MTRUE);
        }
        else if (!UsedOutput(rEnqueData.mWROTO))
        {
            SetOutput(rEnqueData.mWROTO, MTRUE);
        }
        else if (!UsedOutput(rEnqueData.mCopy1))
        {
            SetOutput(rEnqueData.mCopy1);
            hasCopyBuffer = MTRUE;
        }
        else if (!UsedOutput(rEnqueData.mCopy2))
        {
            SetOutput(rEnqueData.mCopy2);
            hasCopyBuffer = MTRUE;
        }
        else
        {
            MY_LOGE("the buffer is not served, type:%s", TypeID2Name(typeId));
        }
    }
    if(!hasOutput)
    {
        MY_LOGW("no output");
        return MTRUE;
    }
    // MSS YUV
    BufferID_T bufId = pNodeRequest->mapBufferID(TID_MAN_MSS_YUV, OUTPUT);
    if (bufId != NULL_BUFFER)
    {
        rEnqueData.mMSSO.mpBuf = pNodeRequest->acquireBuffer(bufId);
        rEnqueData.mMSSO.mBufId = bufId;
        rEnqueData.mMSSO.mTypeId = TID_MAN_MSS_YUV;
        rEnqueData.mMSSO.mHasCrop = MFALSE;
        rEnqueData.mMSSO.mTrans = 0;
        rEnqueData.mMSSO.mCropRegion = MRect(0, 0);
    }

    rEnqueData.mEnableMFB     = pRequest->hasFeature(FID_MFNR) && (pRequest->getActiveFrameCount() > 1);
    // mfnr & msnr does not apply DRE in P2ANode
    rEnqueData.mEnableDRE     = !rEnqueData.mEnableMFB && !mInfo->isRunMSNR && pRequest->hasFeature(FID_DRE) && (pRequest->isActiveFirstFrame());
    rEnqueData.mpIMetaDynamic = mInfo->mpIMetaDynamic;
    rEnqueData.mpIMetaApp     = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal     = mInfo->mpIMetaHal;
    rEnqueData.mSensorId      = mInfo->mSensorId;
    rEnqueData.mEnableVSDoF   = mInfo->isVsdof;
    rEnqueData.mEnableZoom    = mInfo->isZoom;
    rEnqueData.mUniqueKey     = mInfo->mUniqueKey;
    rEnqueData.mRequestNo     = mInfo->mRequestNo;
    rEnqueData.mFrameNo       = mInfo->mFrameNo;
    rEnqueData.mTaskId        = mTaskId;
    rEnqueData.mNodeId        = mInfo->mNodeId;
    rEnqueData.mTimeSharing   = MTRUE;
    addDebugBuffer(pEnqueData);

    if(rEnqueData.mEnableMFB)
    {
        // For MFNR, All subframe's APPMeta & HalMeta are copy from the main frame
        MUINT8 uTuningMode = 0;
        tryGetMetadata<MUINT8>(rEnqueData.mpIMetaHal, MTK_ISP_P2_TUNING_UPDATE_MODE, uTuningMode);
        // SmartTuningBuffer pBufTuning;
        switch (uTuningMode)
        {
            case 0:
                // pBufTuning = pNode->mpBufferPool->getTuningBuffer();
                pNode->mpKeepTuningData = pNode->mpBufferPool->getTuningBuffer();
                rEnqueData.mbMainFrame = MTRUE;
                break;
            // used in MFNR subFrames
            case 2:
                if (pNode->mpKeepTuningData == NULL)
                    MY_LOGE("Should have a previous tuning data");
                else
                {
                    // overwrite its own magic number
                    // MTK_P1NODE_PROCESSOR_MAGICNUM: the metadata used by P1Node and 3A Hal driver for communication purpose
                    MINT32 magicNum = -1;
                    if(tryGetMetadata<MINT32>(rEnqueData.mpIMetaHal, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum))
                        trySetMetadata<MINT32>(&pNode->mKeepMetaHal, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum);
                    else
                        MY_LOGW("Failed to get MTK_P1NODE_PROCESSOR_MAGICNUM of subframes");
                    rEnqueData.mbMainFrame = MFALSE;
                }
                break;
            default:
                pNode->mpKeepTuningData = NULL;
                break;
        }
    }


    EProfileMappingStages stage;
    if(mInfo->mMsnrmode == MsnrMode_Raw) {
#ifdef SUPPORT_AINR
        if (pRequest->hasFeature(FID_AINR))
            stage = ainr::IAinrCore::queryIspMapStage(
                    ainr::AINR_QUERY_MSNR_BFBLD_STAGE,
                    pEnqueData->mpIMetaHal);
        else
            stage = eStage_MSNR_BFBLD;
#else
        stage = eStage_MSNR_BFBLD;
#endif
    } else if(!mInfo->isRunDCE && mInfo->isYuvRep)
        stage = eStage_Y2Y_Main;
    else if(!mInfo->isRunDCE && !mInfo->isYuvRep)
        stage = eStage_R2Y_Main;
    else if(mInfo->isRunDCE && mInfo->isNotApplyDCE)
        stage = (mInfo->isYuvRep) ? eStage_Y2YDCE_1stRun : eStage_R2YDCE_1stRun;
    else
        stage = eStage_Y2YDCE_2ndRun;

    rEnqueData.mIspProfile = fnQueryISPProfile(mInfo->mMappingKey, stage);
    rEnqueData.mIspStage = stage;

    /* We need to set MTK_ISP_AINR_MDLA_MODE just before MSNR BFBLD to
     * avoid every ISP process use MDLAMode to config ISP
     * copy MTK_FEATURE_AINR_MDLA_MODE to MTK_ISP_AINR_MDLA_MODE
     */
    if (pRequest->hasFeature(FID_AINR)) {
        int MDLAMode = 0;

        if (!IMetadata::getEntry<MINT32>(pEnqueData->mpIMetaHal,
                    MTK_FEATURE_AINR_MDLA_MODE, MDLAMode))
            MY_LOGE("cannot get MDLA mode from hal meta");

        IMetadata::setEntry<MINT32>(pEnqueData->mpIMetaHal,
            MTK_ISP_AINR_MDLA_MODE, MDLAMode);
    }

    auto ret = enqueISP(pEnqueData, waitForTiming, timing);

    /* reset MTK_ISP_AINR_MDLA_MODE */
    if (pRequest->hasFeature(FID_AINR)) {
        IMetadata::setEntry<MINT32>(pEnqueData->mpIMetaHal,
            MTK_ISP_AINR_MDLA_MODE, 0);
    }

    if (!ret)
    {
        MY_LOGE("enqueISP failed!");
        return MFALSE;
    }

    return MTRUE;
}

/*******************************************************************************
*
********************************************************************************/

// Raw to Raw worker
MBOOL
EnqueISPWorker_R2R::run(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData = *pEnqueData.get();
    P2ANode* pNode = mInfo->mNode;
    sp<CaptureFeatureNodeRequest> pNodeRequest = mInfo->mpNodeRequest;

    rEnqueData.mpHolder             = mInfo->pHolder;
    rEnqueData.mpSharedData         = mInfo->pSharedData;
    rEnqueData.mIMGI.mBufId         = pNodeRequest->mapBufferID(TID_MAN_FULL_RAW, INPUT);
    rEnqueData.mIMGI.mpBuf          = pNodeRequest->acquireBuffer(rEnqueData.mIMGI.mBufId);
    rEnqueData.mpIMetaDynamic       = mInfo->mpIMetaDynamic;
    rEnqueData.mpIMetaApp           = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal           = mInfo->mpIMetaHal;
    rEnqueData.mpOMetaApp           = mInfo->mpOMetaApp;
    rEnqueData.mpOMetaHal           = mInfo->mpOMetaHal;

    // fix coverity
    if( rEnqueData.mIMGI.mpBuf == nullptr )
    {
        MY_LOGE("input buffer null!");
        return MFALSE;
    }

    {   // output TIMGO port
        MSize fullSize = rEnqueData.mIMGI.mpBuf->getImgSize();
        auto addRawOutput = [&]() -> MBOOL
        {
            TypeID_T typeId = TID_MAN_FULL_RAW;
            BufferID_T bufId = pNodeRequest->mapBufferID(typeId, OUTPUT);
            if (bufId == NULL_BUFFER)
                return MFALSE;

            IImageBuffer* pBuf = pNodeRequest->acquireBuffer(bufId);
            if (pBuf == NULL) {
                MY_LOGE("should not have null buffer. type:%d, buf:%d",typeId, bufId);
                return MFALSE;
            }

            MUINT32 trans = pNodeRequest->getImageTransform(bufId);
            MRect cropRegion = MRect(fullSize.w, fullSize.h);
            MUINT32 scaleRatio = 1;

            P2Output &o   = rEnqueData.mTIMGO;
            o.mpBuf       = pBuf;
            o.mBufId      = bufId;
            o.mTypeId     = typeId;
            o.mHasCrop    = MFALSE;
            o.mTrans      = trans;
            o.mCropRegion = cropRegion;
            o.mScaleRatio = scaleRatio;

            return MTRUE;
        };
        auto ret = addRawOutput();
        if(ret)
            MY_LOGD("add RawImg Fmt(0x%x) VA/PA(%#" PRIxPTR "/%#" PRIxPTR ") to output port",
                rEnqueData.mTIMGO.mpBuf->getImgFormat(),
                rEnqueData.mTIMGO.mpBuf->getBufVA(0),
                rEnqueData.mTIMGO.mpBuf->getBufPA(0));
    }

    rEnqueData.mSensorId  = mInfo->mSensorId;
    rEnqueData.mUniqueKey = mInfo->mUniqueKey;
    rEnqueData.mRequestNo = mInfo->mRequestNo;
    rEnqueData.mFrameNo   = mInfo->mFrameNo;
    rEnqueData.mTaskId    = mTaskId;
    rEnqueData.mNodeId    = mInfo->mNodeId;
    rEnqueData.mTimeSharing = MTRUE;
    rEnqueData.mIspProfile = fnQueryISPProfile(mInfo->mMappingKey, eStage_R2YDCE_1stRun);
    rEnqueData.mIspStage = eStage_R2YDCE_1stRun;

    // if processed raw out, MTK_ISP_P2_PROCESSED_RAW should inform setP2Isp 1
    MINT32 iAPRawType = -1;
    MINT32 iP1RawType = -1;
    tryGetMetadata<MINT32>(mInfo->mpIMetaApp, MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE, iAPRawType);
    tryGetMetadata<MINT32>(mInfo->mpIMetaHal, MTK_P1NODE_RAW_TYPE, iP1RawType);
    if(iAPRawType > 0 && iP1RawType != 0)
    {
        trySetMetadata<MINT32>(mInfo->mpIMetaHal, MTK_ISP_P2_PROCESSED_RAW, 1);
        MY_LOGD("MTK_ISP_P2_PROCESSED_RAW is set to 1");
    }

    // update DNG info to meta
    if (!pNode->mUsageHint.mIsHidlIsp) {
        if (rEnqueData.mpIMetaHal != NULL && rEnqueData.mpIMetaApp != NULL && rEnqueData.mpOMetaApp != NULL) {
            IMetadata rDngMeta = MAKE_DngInfo(LOG_TAG, rEnqueData.mSensorId)->getShadingMapFromHal(*rEnqueData.mpIMetaHal, *rEnqueData.mpIMetaApp);
            *rEnqueData.mpOMetaApp += rDngMeta;
            MY_LOGD("update DNG info to output app meta");
        }
    }

    auto ret = enqueISP(pEnqueData, waitForTiming, timing);
    if (!ret)
    {
        MY_LOGE("main sensor: full raw out failed!");
        return MFALSE;
    }

    if(mNextWorker != nullptr)
    {
        mNextWorker->run();
    }
    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::run(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    MY_LOGD("taskId: %d", mTaskId);
    switch(mTaskId)
    {
        case TASK_DCE_MAIN_1st:
            this->fDCE1stRound(waitForTiming, timing);
            break;
        case TASK_DCE_MAIN_2nd:
            this->fDCE2ndRound(waitForTiming, timing);
            break;
        case TASK_DCE_SUB_1st:
            this->fDCEsub1stround(waitForTiming, timing);
            break;
        case TASK_FULL_MAIN:
            this->fMainFullfunction(waitForTiming, timing);
            break;
        case TASK_FULL_SUB:
            this->fFullsub(waitForTiming, timing);
            break;
        case TASK_RSZ_MAIN:
            this->fRSZMain(waitForTiming, timing);
            break;
        case TASK_RSZ_SUB:
            this->fRSZsub(waitForTiming, timing);
            break;
        default:
            MY_LOGD("no matched raw to yuv process");
            return MFALSE;
            break;
    }
    if(mNextWorker != nullptr)
    {
        mNextWorker->run(mWaitForTiming, mTiming);
    }
    return MTRUE;
}


MBOOL
EnqueISPWorker_R2Y::fDCEsub1stround(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    sp<CaptureFeatureNodeRequest> pNodeReq = mInfo->mpNodeRequest;
    P2ANode* pNode = mInfo->mNode;
    P2EnqueData& rEnqueData   = *pEnqueData.get();
    rEnqueData.mpHolder       = mInfo->pHolder;
    rEnqueData.mpSharedData   = mInfo->pSharedData;
    rEnqueData.mIMGI.mBufId   = pNodeReq->mapBufferID(TID_SUB_FULL_RAW, INPUT);
    rEnqueData.mLCEI.mBufId   = pNodeReq->mapBufferID(TID_SUB_LCS, INPUT);
    rEnqueData.mLCESHO.mBufId = pNodeReq->mapBufferID(TID_SUB_LCESHO, INPUT);
    rEnqueData.mpIMetaDynamic = mInfo->mpIMetaDynamic2;
    rEnqueData.mpIMetaApp     = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal     = mInfo->mpIMetaHal2;
    rEnqueData.mEnableVSDoF   = mInfo->isVsdof;
    rEnqueData.mEnableZoom    = mInfo->isZoom;
    // prepare IMG3O output buffer
    rEnqueData.mIMGI.mpBuf  = pNodeReq->acquireBuffer(rEnqueData.mIMGI.mBufId);

    // fix coverity
    if( rEnqueData.mIMGI.mpBuf == nullptr )
    {
        MY_LOGE("input buffer null!");
        return MFALSE;
    }

    MSize fullSize = rEnqueData.mIMGI.mpBuf->getImgSize();
    mInfo->mpDCES_Buf_sub = acquireDCESOBuffer(MFALSE);
    rEnqueData.mDCESO.mpBuf = mInfo->mpDCES_Buf_sub;
    android::sp<IIBuffer> pWorkingBuffer = pNode->mpBufferPool->getImageBuffer(fullSize, eImgFmt_MTK_YUV_P010, MSize(16,4));
    // Push to resource holder
    mInfo->pHolder->mpBuffers.push_back(pWorkingBuffer);
    mInfo->mpRound1_sub = pWorkingBuffer->getImageBufferPtr();
    rEnqueData.mIMG3O.mpBuf = mInfo->mpRound1_sub;
    rEnqueData.mSensorId    = mInfo->mSubSensorId;
    rEnqueData.mUniqueKey   = mInfo->mUniqueKey;
    rEnqueData.mRequestNo   = mInfo->mRequestNo;
    rEnqueData.mFrameNo     = mInfo->mFrameNo;
    rEnqueData.mTaskId      = mTaskId;
    rEnqueData.mNodeId      = mInfo->mNodeId;
    rEnqueData.mTimeSharing = MTRUE;
    auto stage = eStage_R2YDCE_1stRun;
    rEnqueData.mIspProfile = fnQueryISPProfile(mInfo->mMappingKey, stage);
    rEnqueData.mIspStage = stage;
    // prearep LTM data
    prepareLTMSettingData(rEnqueData.mIMGI.mpBuf, mInfo->mpIMetaHal2, rEnqueData.mLTMSettings);
    //
    addDebugBuffer(pEnqueData);

    auto ret = enqueISP(pEnqueData, waitForTiming, timing);
    if (!ret)
    {
        MY_LOGE("sub sensor: downsize failed!");
        return MFALSE;
    }
    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::fFullsub(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData            = *pEnqueData.get();
    EProfileMappingStages stage        = eStage_R2Y_Sub;

    rEnqueData.mpHolder                    = mInfo->pHolder;
    rEnqueData.mpSharedData                = mInfo->pSharedData;
    sp<CaptureFeatureNodeRequest> pNodeReq = mInfo->mpNodeRequest;
    P2ANode* pNode                         = mInfo->mNode;
    BufferID_T uOBufId = pNodeReq->mapBufferID(TID_SUB_FULL_YUV, OUTPUT);

    if(mInfo->isZoom)
    {
        if (mInfo->isRunDCE && !mInfo->isNotApplyDCE)
        {
            if(mInfo->mpRound1_sub != NULL)
            {
                rEnqueData.mIMGI.mpBuf = mInfo->mpRound1_sub;
                rEnqueData.mRound = 2;
            }
            else
            {
                rEnqueData.mIMGI.mBufId  = pNodeReq->mapBufferID(TID_SUB_FULL_RAW, INPUT);
                rEnqueData.mLCEI.mBufId  = pNodeReq->mapBufferID(TID_SUB_LCS, INPUT);
                rEnqueData.mLCESHO.mBufId = pNodeReq->mapBufferID(TID_SUB_LCESHO, INPUT);
            }

            rEnqueData.mDCESI.mpBuf = mInfo->mpDCES_Buf_sub;
            rEnqueData.mEnableDCE   = MTRUE;
        }
        else
        {
            if(mInfo->isRunDCE && mInfo->isNotApplyDCE)
                rEnqueData.mDCESO.mpBuf = this->acquireDCESOBuffer(MFALSE);
            rEnqueData.mIMGI.mBufId  = pNodeReq->mapBufferID(TID_SUB_FULL_RAW, INPUT);
            rEnqueData.mLCEI.mBufId  = pNodeReq->mapBufferID(TID_SUB_LCS, INPUT);
            rEnqueData.mLCESHO.mBufId = pNodeReq->mapBufferID(TID_SUB_LCESHO, INPUT);
        }

        rEnqueData.mIMG3O.mBufId = uOBufId;
        // MSS YUV
        BufferID_T bufId = pNodeReq->mapBufferID(TID_SUB_MSS_YUV, OUTPUT);
        if (bufId != NULL_BUFFER)
        {
            rEnqueData.mMSSO.mpBuf = pNodeReq->acquireBuffer(bufId);
            rEnqueData.mMSSO.mBufId = bufId;
            rEnqueData.mMSSO.mTypeId = TID_SUB_MSS_YUV;
            rEnqueData.mMSSO.mHasCrop = MFALSE;
            rEnqueData.mMSSO.mTrans = 0;
            rEnqueData.mMSSO.mCropRegion = MRect(0, 0);
        }

        if(!mInfo->isRunDCE)
            stage = eStage_R2Y_Sub;
        else if(mInfo->isRunDCE && mInfo->isNotApplyDCE)
            stage = eStage_R2YDCE_1stRun;
        else
            stage = eStage_Y2YDCE_2ndRun;

    }
    else
    {
        rEnqueData.mIMGI.mBufId  = pNodeReq->mapBufferID(TID_SUB_FULL_RAW, INPUT);
        rEnqueData.mLCEI.mBufId  = pNodeReq->mapBufferID(TID_SUB_LCS, INPUT);
        rEnqueData.mLCESHO.mBufId = pNodeReq->mapBufferID(TID_SUB_LCESHO, INPUT);
        rEnqueData.mWDMAO.mBufId = uOBufId;
    }

    rEnqueData.mpIMetaApp   = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal   = mInfo->mpIMetaHal2;
    rEnqueData.mSensorId    = mInfo->mSubSensorId;
    rEnqueData.mEnableVSDoF = mInfo->isVsdof;
    rEnqueData.mEnableZoom  = mInfo->isZoom;
    rEnqueData.mUniqueKey   = mInfo->mUniqueKey;
    rEnqueData.mRequestNo   = mInfo->mRequestNo;
    rEnqueData.mFrameNo     = mInfo->mFrameNo;
    rEnqueData.mTaskId      = mTaskId;
    rEnqueData.mNodeId      = mInfo->mNodeId;
    rEnqueData.mTimeSharing = MTRUE;

    if (mInfo->isVsdof)
    {
        FovCalculator::FovInfo fovInfo;
        if (pNode->mpFOVCalculator->getIsEnable() && pNode->mpFOVCalculator->getFovInfo(mInfo->mSubSensorId, fovInfo))
        {
            rEnqueData.mWDMAO.mHasCrop      = mInfo->isVsdof;
            rEnqueData.mWDMAO.mCropRegion.p = fovInfo.mFOVCropRegion.p;
            rEnqueData.mWDMAO.mCropRegion.s = fovInfo.mFOVCropRegion.s;
            MY_LOGD("VSDOF Sensor(%d) FOV Crop(%d,%d)(%dx%d)",
                        mInfo->mSubSensorId,
                        fovInfo.mFOVCropRegion.p.x,
                        fovInfo.mFOVCropRegion.p.y,
                        fovInfo.mFOVCropRegion.s.w,
                        fovInfo.mFOVCropRegion.s.h);
        }
    }

    rEnqueData.mIspProfile = fnQueryISPProfile(mInfo->mMappingKey, stage);
    rEnqueData.mIspStage = stage;

    auto ret = enqueISP(pEnqueData, waitForTiming, timing);
    if (!ret)
    {
        MY_LOGE("sub sensor: full yuv failed!");
        return MFALSE;
    }
    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::fRSZMain(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    sp<CaptureFeatureNodeRequest> pNodeReq = mInfo->mpNodeRequest;
    BufferID_T uOResizedYuv = pNodeReq->mapBufferID(TID_MAN_RSZ_YUV, OUTPUT);
    P2EnqueData& rEnqueData  = *pEnqueData.get();
    rEnqueData.mpHolder      = mInfo->pHolder;
    rEnqueData.mpSharedData  = mInfo->pSharedData;
    rEnqueData.mIMGI.mBufId  = pNodeReq->mapBufferID(TID_MAN_RSZ_YUV, INPUT);
    rEnqueData.mWDMAO.mBufId = uOResizedYuv;
    rEnqueData.mpIMetaApp    = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal    = mInfo->mpIMetaHal;
    rEnqueData.mSensorId     = mInfo->mSensorId;
    rEnqueData.mResized      = MTRUE;
    rEnqueData.mEnableVSDoF  = mInfo->isVsdof;
    rEnqueData.mEnableZoom   = mInfo->isZoom;
    rEnqueData.mUniqueKey    = mInfo->mUniqueKey;
    rEnqueData.mRequestNo    = mInfo->mRequestNo;
    rEnqueData.mFrameNo      = mInfo->mFrameNo;
    rEnqueData.mTaskId       = mTaskId;
    rEnqueData.mNodeId       = mInfo->mNodeId;
    rEnqueData.mTimeSharing  = USEING_TIME_SHARING;
    rEnqueData.mIspProfile   = fnQueryISPProfile(mInfo->mMappingKey, eStage_RRZ_Y2Y_Main);
    rEnqueData.mIspStage     = eStage_RRZ_Y2Y_Main;
    auto ret = enqueISP(pEnqueData, waitForTiming, timing);
    if (!ret)
    {
        MY_LOGE("sub sensor: resized yuv failed!");
        return MFALSE;
    }
    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::fRSZsub(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    sp<CaptureFeatureNodeRequest> pNodeReq = mInfo->mpNodeRequest;
    BufferID_T uOBufId = pNodeReq->mapBufferID(TID_SUB_RSZ_YUV, OUTPUT);

    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData = *pEnqueData.get();

    rEnqueData.mpHolder      = mInfo->pHolder;
    rEnqueData.mpSharedData  = mInfo->pSharedData;
    rEnqueData.mIMGI.mBufId  = pNodeReq->mapBufferID(TID_SUB_RSZ_YUV, INPUT);
    rEnqueData.mWDMAO.mBufId = uOBufId;
    rEnqueData.mpIMetaApp    = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal    = mInfo->mpIMetaHal2;
    rEnqueData.mSensorId     = mInfo->mSubSensorId;
    rEnqueData.mResized      = MTRUE;
    rEnqueData.mEnableVSDoF  = mInfo->isVsdof;
    rEnqueData.mEnableZoom   = mInfo->isZoom;
    rEnqueData.mUniqueKey    = mInfo->mUniqueKey;
    rEnqueData.mRequestNo    = mInfo->mRequestNo;
    rEnqueData.mFrameNo      = mInfo->mFrameNo;
    rEnqueData.mTaskId       = mTaskId;
    rEnqueData.mNodeId       = mInfo->mNodeId;
    rEnqueData.mTimeSharing  = USEING_TIME_SHARING;
    rEnqueData.mIspProfile   = fnQueryISPProfile(mInfo->mMappingKey, eStage_RRZ_R2Y_Sub);
    rEnqueData.mIspStage     = eStage_RRZ_R2Y_Sub;
    auto ret = enqueISP(pEnqueData, waitForTiming, timing);
    if (!ret)
    {
        MY_LOGE("sub sensor: resized yuv failed!");
        return MFALSE;
    }
    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::fDCE2ndRound(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData = *pEnqueData.get();
    sp<CaptureFeatureNodeRequest> pNodeReq = mInfo->mpNodeRequest;
    if (mInfo->mpRound1 != NULL)
    {
        rEnqueData.mIMGI.mpBuf = mInfo->mpRound1;
        rEnqueData.mRound = 2;
    }
    else
    {
        MY_LOGW("can not find DCE first round buffer");
        // Enque RAW with LCS
        rEnqueData.mIMGI.mBufId   = pNodeReq->mapBufferID(TID_MAN_FULL_RAW, INPUT);
        rEnqueData.mLCEI.mBufId   = pNodeReq->mapBufferID(TID_MAN_LCS, INPUT);
        rEnqueData.mLCESHO.mBufId = pNodeReq->mapBufferID(TID_MAN_LCESHO, INPUT);
    }
    rEnqueData.mDCESI.mpBuf = mInfo->mpDCES_Buf;
    rEnqueData.mEnableDCE   = MTRUE;
    rEnqueData.mpOMetaApp = NULL;
    rEnqueData.mpOMetaHal = NULL;

    fMainFullCommon(pEnqueData, waitForTiming, timing);

    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::fMainFullfunction(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData = *pEnqueData.get();
    sp<CaptureFeatureNodeRequest> pNodeReq = mInfo->mpNodeRequest;
    rEnqueData.mIMGI.mBufId = pNodeReq->mapBufferID(TID_MAN_FULL_RAW, INPUT);
    P2ANode* pNode = mInfo->mNode;
    // support for generate DCE but not apply flow
    if(mInfo->isRunDCE && mInfo->isNotApplyDCE)
    {
        rEnqueData.mDCESO.mpBuf = acquireDCESOBuffer(MTRUE);
    }
    // Check Raw type
    auto IsPureRaw = [](IImageBuffer *pBuf) -> MBOOL
    {
        if (pBuf == NULL)
            return MFALSE;
        MINT64 rawType;
        if (pBuf->getImgDesc(eIMAGE_DESC_ID_RAW_TYPE, rawType))
            return rawType == eIMAGE_DESC_RAW_TYPE_PURE;
        return MFALSE;
    };
    rEnqueData.mIMGI.mpBuf    = pNodeReq->acquireBuffer(rEnqueData.mIMGI.mBufId);

    // fix coverity
    if( rEnqueData.mIMGI.mpBuf == nullptr )
    {
        MY_LOGE("input buffer null!");
        return MFALSE;
    }

    MBOOL isPureRaw           = IsPureRaw(rEnqueData.mIMGI.mpBuf);

    rEnqueData.mIMGI.mPureRaw = isPureRaw;
    rEnqueData.mLCEI.mBufId   = pNodeReq->mapBufferID(TID_MAN_LCS, INPUT);
    rEnqueData.mLCESHO.mBufId = pNodeReq->mapBufferID(TID_MAN_LCESHO, INPUT);
    rEnqueData.mpOMetaApp = mInfo->mpOMetaApp;
    rEnqueData.mpOMetaHal = mInfo->mpOMetaHal;

    MBOOL bHasOMeta = mInfo->mpOMetaApp != NULL || mInfo->mpOMetaHal != NULL;
    MBOOL bIMGIRawFmt = isHalRawFormat((EImageFormat)rEnqueData.mIMGI.mpBuf->getImgFormat());
    rEnqueData.mbNeedOutMeta  = bHasOMeta && (bIMGIRawFmt || !pNode->mUsageHint.mIsHidlIsp);

    fMainFullCommon(pEnqueData, waitForTiming, timing);
    return MTRUE;
}


MBOOL
EnqueISPWorker_Y2Y::fYuvReprofunction(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    P2ANode* pNode = mInfo->mNode;
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData = *pEnqueData.get();
    sp<CaptureFeatureNodeRequest> pNodeRequest = mInfo->mpNodeRequest;
    rEnqueData.mIMGI.mBufId = pNodeRequest->mapBufferID(TID_MAN_FULL_YUV, INPUT);
    rEnqueData.mYuvRep = MTRUE;
    rEnqueData.mIMGI.mpBuf    = pNodeRequest->acquireBuffer(rEnqueData.mIMGI.mBufId);
    rEnqueData.mIMGI.mPureRaw = MFALSE;
    rEnqueData.mLCEI.mBufId   = pNodeRequest->mapBufferID(TID_MAN_LCS, INPUT);
    rEnqueData.mLCESHO.mBufId = pNodeRequest->mapBufferID(TID_MAN_LCESHO, INPUT);
    rEnqueData.mpOMetaApp = mInfo->mpOMetaApp;
    rEnqueData.mpOMetaHal = mInfo->mpOMetaHal;

    sp<CropCalculator::Factor> pFactor = pNode->mpCropCalculator->getFactor(mInfo->mpIMetaApp, mInfo->mpIMetaHal, mInfo->mSensorId);
    if ((!pNode->mUsageHint.mIsHidlIsp || !pFactor->mbExistActiveCrop))
    {
        mInfo->mpRequest->addParameter(PID_IGNORE_CROP, 1);
    }
    fMainFullCommon(pEnqueData, waitForTiming, timing);
    return MTRUE;
}

MBOOL
EnqueISPWorker_R2Y::fDCE1stRound(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    shared_ptr<P2EnqueData> pEnqueData = make_shared<P2EnqueData>();
    P2EnqueData& rEnqueData = *pEnqueData.get();
    P2ANode* pNode = mInfo->mNode;
    sp<CaptureFeatureNodeRequest> pNodeRequest = mInfo->mpNodeRequest;
    rEnqueData.mpHolder       = mInfo->pHolder;
    rEnqueData.mpSharedData   = mInfo->pSharedData;
    rEnqueData.mIMGI.mBufId   = pNodeRequest->mapBufferID(TID_MAN_FULL_RAW, INPUT);
    rEnqueData.mLCEI.mBufId   = pNodeRequest->mapBufferID(TID_MAN_LCS, INPUT);
    rEnqueData.mLCESHO.mBufId = pNodeRequest->mapBufferID(TID_MAN_LCESHO, INPUT);
    rEnqueData.mpIMetaDynamic = mInfo->mpIMetaDynamic;
    rEnqueData.mpIMetaApp     = mInfo->mpIMetaApp;
    rEnqueData.mpIMetaHal     = mInfo->mpIMetaHal;
    rEnqueData.mpOMetaApp     = mInfo->mpOMetaApp;
    rEnqueData.mpOMetaHal     = mInfo->mpOMetaHal;
    rEnqueData.mEnableVSDoF   = mInfo->isVsdof;
    rEnqueData.mEnableZoom    = mInfo->isZoom;
    // prepare IMG3O output buffer
    rEnqueData.mIMGI.mpBuf  = pNodeRequest->acquireBuffer(rEnqueData.mIMGI.mBufId);

    // fix coverity
    if( rEnqueData.mIMGI.mpBuf == nullptr )
    {
        MY_LOGE("input buffer null!");
        return MFALSE;
    }

    MSize fullSize          = rEnqueData.mIMGI.mpBuf->getImgSize();
    mInfo->mpDCES_Buf = acquireDCESOBuffer(MTRUE);
    rEnqueData.mDCESO.mpBuf = mInfo->mpDCES_Buf;
    android::sp<IIBuffer> pWorkingBuffer = pNode->mpBufferPool->getImageBuffer(fullSize, eImgFmt_MTK_YUV_P010, MSize(16,4));
    // Push to resource holder
    mInfo->pHolder->mpBuffers.push_back(pWorkingBuffer);
    mInfo->mpRound1 = pWorkingBuffer->getImageBufferPtr();

    rEnqueData.mIMG3O.mpBuf   = mInfo->mpRound1;
    rEnqueData.mSensorId      = mInfo->mSensorId;
    rEnqueData.mUniqueKey     = mInfo->mUniqueKey;
    rEnqueData.mRequestNo     = mInfo->mRequestNo;
    rEnqueData.mFrameNo       = mInfo->mFrameNo;
    rEnqueData.mTaskId        = mTaskId;
    rEnqueData.mNodeId        = mInfo->mNodeId;
    rEnqueData.mTimeSharing   = MTRUE;

    MBOOL bHasOMeta = mInfo->mpOMetaApp != NULL || mInfo->mpOMetaHal != NULL;
    MBOOL bIMGIRawFmt = isHalRawFormat((EImageFormat)rEnqueData.mIMGI.mpBuf->getImgFormat());
    rEnqueData.mbNeedOutMeta    = bHasOMeta && (bIMGIRawFmt || !pNode->mUsageHint.mIsHidlIsp);

    auto stage = eStage_R2YDCE_1stRun;
    rEnqueData.mIspProfile = fnQueryISPProfile(mInfo->mMappingKey, stage);
    rEnqueData.mIspStage = stage;
    // prepare LTM data
    prepareLTMSettingData(rEnqueData.mIMGI.mpBuf, mInfo->mpIMetaHal, rEnqueData.mLTMSettings);
    //
    addDebugBuffer(pEnqueData);

    auto ret = enqueISP(pEnqueData, waitForTiming, timing);
    if (!ret)
    {
        MY_LOGE("main sensor: downsize failed!");
        return MFALSE;
    }

    return MTRUE;
}

MBOOL
EnqueISPWorker_Y2Y::run(const shared_ptr<sem_t>& waitForTiming, eWaitTiming timing)
{
    fYuvReprofunction(waitForTiming, timing);
    if(mNextWorker != nullptr)
    {
        mNextWorker->run(mWaitForTiming, mTiming);
    }
    return MTRUE;
}

} // SCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam


/*******************************************************************************
 *
 ********************************************************************************/
// QParam generator
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

QParamGenerator::QParamGenerator(MUINT32 iSensorIdx, ENormalStreamTag streamTag)
{
    mPass2EnqueFrame.mStreamTag = streamTag;
    mPass2EnqueFrame.mSensorIdx = iSensorIdx;
}

MVOID
QParamGenerator::addCrop(eCropGroup groupID, MPoint startLoc, MSize cropSize, MSize resizeDst, bool isMDPCrop)
{
    MCrpRsInfo cropInfo;
    cropInfo.mGroupID = (MUINT32) groupID;
    cropInfo.mCropRect.p_fractional.x=0;
    cropInfo.mCropRect.p_fractional.y=0;
    cropInfo.mCropRect.p_integral.x=startLoc.x;
    cropInfo.mCropRect.p_integral.y=startLoc.y;
    cropInfo.mCropRect.s=cropSize;
    cropInfo.mResizeDst=resizeDst;
    cropInfo.mMdpGroup = (isMDPCrop) ? 1 : 0;
    mPass2EnqueFrame.mvCropRsInfo.push_back(cropInfo);
}

MVOID
QParamGenerator::addInput(NSCam::NSIoPipe::PortID portID, IImageBuffer* pImgBuf)
{
    Input src;
    src.mPortID = portID;
    src.mBuffer = pImgBuf;
    mPass2EnqueFrame.mvIn.push_back(src);
}

MVOID
QParamGenerator::addOutput(NSCam::NSIoPipe::PortID portID, IImageBuffer* pImgBuf, MINT32 transform)
{
    Output out;
    out.mPortID = portID;
    out.mTransform = transform;
    out.mBuffer = pImgBuf;
    mPass2EnqueFrame.mvOut.push_back(out);
}

MVOID
QParamGenerator::addExtraParam(EPostProcCmdIndex cmdIdx, MVOID* param)
{
    ExtraParam extra;
    extra.CmdIdx = cmdIdx;
    extra.moduleStruct = param;
    mPass2EnqueFrame.mvExtraParam.push_back(extra);
}

MVOID
QParamGenerator::addModuleInfo(MUINT32 moduleTag, MVOID* moduleStruct)
{
    ModuleInfo moduleInfo;
    moduleInfo.moduleTag = moduleTag;
    moduleInfo.moduleStruct = moduleStruct;
    mPass2EnqueFrame.mvModuleData.push_back(moduleInfo);
}

MVOID
QParamGenerator::insertTuningBuf(MVOID* pTuningBuf)
{
    mPass2EnqueFrame.mTuningData = pTuningBuf;
}

MVOID
QParamGenerator::setInfo(const FrameInfo& frameInfo, MINT8 dumpFlag)
{
    mPass2EnqueFrame.FrameNo = frameInfo.FrameNo;
    mPass2EnqueFrame.RequestNo = frameInfo.RequestNo;
    mPass2EnqueFrame.Timestamp = frameInfo.Timestamp;
    mPass2EnqueFrame.UniqueKey = frameInfo.UniqueKey;
    mPass2EnqueFrame.IspProfile = frameInfo.IspProfile;
    IHalSensorList *sensorList = MAKE_HalSensorList();
    if(sensorList)
        mPass2EnqueFrame.SensorDev = sensorList->querySensorDevIdx(frameInfo.SensorID);
    else
        MY_LOGE("sensorList empty");
    mPass2EnqueFrame.NeedDump = dumpFlag;
}

MBOOL
QParamGenerator::generate(QParams& rQParam)
{
    if(checkValid())
    {
        rQParam.mvFrameParams.push_back(mPass2EnqueFrame);
        return MTRUE;
    }
    else
        return MFALSE;
}

MBOOL
QParamGenerator::checkValid()
{
    bool bAllSuccess = MTRUE;

    // check in/out size
    if((mPass2EnqueFrame.mvIn.size() == 0 &&  mPass2EnqueFrame.mvOut.size() != 0) ||
        (mPass2EnqueFrame.mvIn.size() != 0 &&  mPass2EnqueFrame.mvOut.size() == 0))
    {
        MY_LOGE("In/Out buffer size is not consistent, in:%zu out:%zu",
                    mPass2EnqueFrame.mvIn.size(), mPass2EnqueFrame.mvOut.size());
        bAllSuccess = MFALSE;
    }

    // for each mvOut, check the corresponding crop setting is ready
    for(Output output: mPass2EnqueFrame.mvOut)
    {
        MINT32 findTarget;
        if(output.mPortID.index == NSImageio::NSIspio::EPortIndex_WROTO)
            findTarget = (MINT32)eCROP_WROT;
        else if(output.mPortID.index == NSImageio::NSIspio::EPortIndex_WDMAO)
            findTarget = (MINT32)eCROP_WDMA;
        else if(output.mPortID.index == NSImageio::NSIspio::EPortIndex_IMG2O)
            findTarget = (MINT32)eCROP_CRZ;
        else
            continue;

        bool bIsFind = MFALSE;
        for(MCrpRsInfo rsInfo : mPass2EnqueFrame.mvCropRsInfo)
        {
            if(rsInfo.mGroupID == findTarget)
            {
                bIsFind = MTRUE;
                break;
            }
        }
        if(!bIsFind)
        {
            MY_LOGE("has output buffer with portID=%d, but has no the needed crop:%d",
                    output.mPortID.index, findTarget);
            bAllSuccess = MFALSE;
        }
    }

    // check duplicated input port
    for(Input input: mPass2EnqueFrame.mvIn)
    {
        bool bSuccess = MTRUE;
        int count = 0;
        for(Input chkIn : mPass2EnqueFrame.mvIn)
        {
            if(chkIn.mPortID.index == input.mPortID.index)
                count++;

            if(count>1)
            {
                bSuccess = MFALSE;
                MY_LOGE("Duplicated mvIn portID:%d!!", chkIn.mPortID.index);
                break;
            }
        }
        if(!bSuccess)
        {
            bAllSuccess = MFALSE;
        }
    }

    // check duplicated output port
    for(Output output: mPass2EnqueFrame.mvOut)
    {
        bool bSuccess = MTRUE;
        int count = 0;
        for(Output chkOut : mPass2EnqueFrame.mvOut)
        {
            if(chkOut.mPortID.index == output.mPortID.index)
                count++;

            if(count>1)
            {
                bSuccess = MFALSE;
                MY_LOGE("Duplicated mvOut portID:%d!!", chkOut.mPortID.index);
                break;
            }
        }
        if(!bSuccess)
        {
            bAllSuccess = MFALSE;
        }
    }
    return bAllSuccess;
}
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
