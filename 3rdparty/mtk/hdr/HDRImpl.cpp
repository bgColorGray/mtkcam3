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

#define LOG_TAG "HDRPlugin"

#include "HDRImpl.h"

#include <stdlib.h>
#include <sys/prctl.h>
#include <cutils/properties.h>
#include <cutils/compiler.h>
#include <utils/Errors.h>
#include <utils/String8.h>

#include <memory>
#include <vector>

#include <mtkcam/drv/iopipe/SImager/ISImager.h>

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>

using namespace NSCam;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::TuningUtils;
using namespace NS3Av3;
using namespace NSIoPipe;
using NSCam::TuningUtils::scenariorecorder::DecisionParam;
using NSCam::TuningUtils::scenariorecorder::IScenarioRecorder;
using NSCam::TuningUtils::scenariorecorder::DECISION_FEATURE;
using NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_HDR);
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//
#define ASSERT(cond, msg)           do { if (!(cond)) { printf("Failed: %s\n", msg); return; } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
REGISTER_PLUGIN_PROVIDER(MultiFrame, HDRPluginProviderImp);

/******************************************************************************
 *
 ******************************************************************************/
HDRPluginProviderImp::
HDRPluginProviderImp()
    : mDumpBuffer(0)
    , mvCaptureParam()
    , mvpRequest()
    , mpCallback()
{
    CAM_ULOGM_APILIFE();
    mEnable = ::property_get_int32("vendor.debug.camera.hdr.enable", -1);
    mAlgoType = ::property_get_int32("vendor.camera.hdr.type", 0);  // 0: YuvDomainHDR, 1: RawDomainHDR
    mProcessRawType = ::property_get_int32("vendor.camera.hdr.processRawType", 0);  // 0: eProcessRaw, 1: ePureRaw
    mPackRawType = ::property_get_int32("vendor.camera.hdr.packRawType", 0);  // 0: eUnpackRaw, 1: ePackRaw
    mBitDepth = ::property_get_int32("vendor.camera.hdr.bitDepth", 0);  // 0: e10Bits, 1: e12Bits
    mDumpBuffer = ::property_get_int32("vendor.debug.camera.hdr.dump", 0);
}

/******************************************************************************
 *
 ******************************************************************************/
HDRPluginProviderImp::
~HDRPluginProviderImp()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
const
HDRPluginProviderImp::Property&
HDRPluginProviderImp::
property()
{
    CAM_ULOGM_APILIFE();
    static Property prop;
    static bool inited;

    if (!inited) {
        prop.mName = "THIRD PARTY HDR";
        prop.mFeatures = TP_FEATURE_HDR;
        prop.mZsdBufferMaxNum = 3;  // maximum full raw/yuv buffer requirement
        prop.mThumbnailTiming = eTiming_P2;
        prop.mPriority = ePriority_Highest;
        inited = true;
    }

    return prop;
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
HDRPluginProviderImp::
negotiate(Selection& sel)
{
    CAM_ULOGM_APILIFE();

    // check meta
    IMetadata* pAppInMeta = nullptr;
    IMetadata* pHalInMeta = nullptr;
    bool isAppMeta = sel.mIMetadataApp.getControl() != NULL;
    bool isHalMeta = sel.mIMetadataHal.getControl() != NULL;
    if (isAppMeta) {
        pAppInMeta = sel.mIMetadataApp.getControl().get();
    }
    if (isHalMeta) {
        pHalInMeta = sel.mIMetadataHal.getControl().get();
    }

    if (mEnable == 0) {
        MY_LOGD("force HDR off");
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta, "force HDR off");
        }
        return BAD_VALUE;
    }

    // HDR is off on low RAM devices
    bool isLowRamDevice = ::property_get_bool("ro.config.low_ram", false);
    if (isLowRamDevice) {
        MY_LOGI("low RAM device, force HDR off");
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                    "low RAM device, force HDR off");
        }
        return BAD_VALUE;
    }

    // HDR on/off based on HDR scenario
    if (isAppMeta) {
        MUINT8 sceneMode = 0;
        bool ret = IMetadata::getEntry<MUINT8>(pAppInMeta, MTK_CONTROL_SCENE_MODE, sceneMode);

        if (CC_UNLIKELY(!ret)) {
            MY_LOGE("cannot get MTK_CONTROL_SCENE_MODE");
            if (isHalMeta) {
                writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                        "cannot get MTK_CONTROL_SCENE_MODE");
            }
            return BAD_VALUE;
        }

        if (sceneMode != MTK_CONTROL_SCENE_MODE_HDR) {
            if (mEnable == 1) {
                MY_LOGD("force HDR on");
                if (isHalMeta) {
                    writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                            "force HDR on");
                }
            } else {
                MY_LOGD("no need to execute HDR");
                if (isHalMeta) {
                    writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                            "no need to execute HDR");
                }
                return BAD_VALUE;
            }
        }
    } else {
        MY_LOGE("cannot get App Meta");
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                    "cannot get App Meta");
        }
        return BAD_VALUE;
    }

    sel.mRequestCount = 3;

    // get capture param
    if (sel.mRequestIndex == 0) {
        if (!getCaptureParam(sel.mSensorId, sel.mRequestCount)) {
            MY_LOGE("get capture param fail");
            if (isHalMeta) {
                writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                        "get capture param fail");
            }
            return BAD_VALUE;
        }
    }

    // decide input/outputt format
    int inputFormat = eImgFmt_NV21;
    int outputFormat = eImgFmt_NV21;
    if (mAlgoType == RawDomainHDR) {
        if (mProcessRawType == eProcessRaw) {
            sel.mProcessRaw = MTRUE;
        }

        if (mPackRawType == eUnpackRaw) {
            if (mBitDepth == e10Bits) {
                inputFormat = eImgFmt_BAYER10_UNPAK; // default format
                outputFormat = eImgFmt_BAYER10_UNPAK; // default format
            } else {
                inputFormat = eImgFmt_BAYER12_UNPAK;
                outputFormat = eImgFmt_BAYER12_UNPAK;
            }
            sel.mDecision.mNeedUnpackRaw = true;
        } else {
            if (mBitDepth == e10Bits) {
                // 10 bits
                inputFormat = eImgFmt_BAYER10;
                outputFormat = eImgFmt_BAYER10;
            } else {
                // 12 bits
                inputFormat = eImgFmt_BAYER12;
                outputFormat = eImgFmt_BAYER12;
            }
        }
    }

    sel.mIBufferFull.setRequired(MTRUE)
                    .addAcceptedFormat(inputFormat)
                    .addAcceptedSize(eImgSize_Full);
    sel.mIMetadataDynamic.setRequired(MTRUE);
    sel.mIMetadataApp.setRequired(MTRUE);
    sel.mIMetadataHal.setRequired(MTRUE);

    // Only main frame has output buffer
    if (sel.mRequestIndex == 0) {
        sel.mOBufferFull.setRequired(MTRUE)
                        .addAcceptedFormat(outputFormat)
                        .addAcceptedSize(eImgSize_Full);
        sel.mOMetadataApp.setRequired(MTRUE);
        sel.mOMetadataHal.setRequired(MTRUE);
    } else {
        sel.mOBufferFull.setRequired(MFALSE);
        sel.mOMetadataApp.setRequired(MFALSE);
        sel.mOMetadataHal.setRequired(MFALSE);
    }

    if (isAppMeta && isHalMeta) {
        MetadataPtr pAppAdditional = std::make_shared<IMetadata>();
        MetadataPtr pHalAddtional = std::make_shared<IMetadata>();

        //IMetadata* pAppMeta = pAppAdditional.get();
        IMetadata* pHalMeta = pHalAddtional.get();

        CaptureParam_T& captureParam = mvCaptureParam.at(sel.mRequestIndex);

        // set capture param
        IMetadata::Memory capParam;
        capParam.resize(sizeof(CaptureParam_T));
        memcpy(capParam.editArray(), &(captureParam), sizeof(CaptureParam_T));
        IMetadata::setEntry<IMetadata::Memory>(pHalMeta, MTK_3A_AE_CAP_PARAM, capParam);

        // set AF
        // pause AF for N-1 frames and resume for the last frame
        bool isLastFrame = sel.mRequestIndex == (sel.mRequestCount-1) ? 1 : 0;
        IMetadata::setEntry<MUINT8>(pHalMeta, MTK_FOCUS_PAUSE, isLastFrame ? 0 : 1);

        IMetadata::setEntry<MUINT8>(pHalMeta, MTK_HAL_REQUEST_REQUIRE_EXIF, 1);

        //
        if (mAlgoType == RawDomainHDR) {
            IMetadata::setEntry<MINT32>(pHalMeta, MTK_HAL_REQUEST_IMG_IMGO_FORMAT, eImgFmt_BAYER10_UNPAK);
        }

        sel.mIMetadataApp.setAddtional(pAppAdditional);
        sel.mIMetadataHal.setAddtional(pHalAddtional);
    }

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
void
HDRPluginProviderImp::
init()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
HDRPluginProviderImp::
process(RequestPtr pRequest,
        RequestCallbackPtr pCallback)
{
    CAM_ULOGM_APILIFE();

    // set thread's name
    ::prctl(PR_SET_NAME, "HDR Plugin", 0, 0, 0);

    // restore callback function for abort API
    if (pCallback != nullptr) {
        mpCallback = pCallback;
    }

    // check buffer and meta
    bool isIBufferFull = pRequest->mIBufferFull != nullptr;
    bool isOBufferFull = pRequest->mOBufferFull != nullptr;
    bool isIMetaHal = pRequest->mIMetadataHal != nullptr;

    // get input buffer
    if (isIBufferFull) {
        IImageBuffer* pIImgBuffer = pRequest->mIBufferFull->acquire();
        MY_LOGD("[IN] Full image VA: 0x%p size:(%d,%d) fmt: 0x%x planecount: %zu",
                (void*)pIImgBuffer->getBufVA(0),
                pIImgBuffer->getImgSize().w, pIImgBuffer->getImgSize().h,
                pIImgBuffer->getImgFormat(),
                pIImgBuffer->getPlaneCount());

        if (mDumpBuffer) {
            // dump input buffer
            const char* pUserString = String8::format("hdr-in-%d",
                                                      pRequest->mRequestIndex).c_str();
            IMetadata* pHalMeta = pRequest->mIMetadataHal->acquire();
            dumpBuffer(pHalMeta, pRequest->mSensorId, pIImgBuffer, pUserString);
        }
    }

    // get output buffer
    if (isOBufferFull) {
        IImageBuffer* pOImgBuffer = pRequest->mOBufferFull->acquire();
        MY_LOGD("[OUT] Full image VA: 0x%p size:(%d,%d) fmt: 0x%x planecount: %zu",
                (void*)pOImgBuffer->getBufVA(0),
                pOImgBuffer->getImgSize().w, pOImgBuffer->getImgSize().h,
                pOImgBuffer->getImgFormat(),
                pOImgBuffer->getPlaneCount());

        // get input buffer
        IImageBuffer* pIImgBuffer = nullptr;
        int colorOrder = -1;
        if (isIBufferFull) {
            pIImgBuffer = pRequest->mIBufferFull->acquire();
            colorOrder = pIImgBuffer->getColorArrangement();
        } else {
            MY_LOGE("[IN] Full image is null");
            return BAD_VALUE;
        }

        // copy input buffer to output buffer
        if (mAlgoType == YuvDomainHDR) {
            NSSImager::ISImager *pISImager = nullptr;
            pISImager = NSSImager::ISImager::createInstance(pIImgBuffer);
            if (CC_LIKELY(pISImager)) {
                if (!pISImager->setTargetImgBuffer(pOImgBuffer)) {
                    MY_LOGW("SImager::setTargetImgBuffer fail");
                }
                if (!pISImager->execute()) {
                    MY_LOGW("SImager::execute fail");
                }
                pISImager->destroyInstance();
            } else {
                MY_LOGW("ISImager::createInstance fail");
            }
        } else {
            for (size_t i = 0; i < pOImgBuffer->getPlaneCount(); ++i) {
                size_t planeBufSize = pOImgBuffer->getBufSizeInBytes(i);
                MUINT8* srcPtr = (MUINT8*)pIImgBuffer->getBufVA(i);
                void* dstPtr = (void*)pOImgBuffer->getBufVA(i);
                memcpy(dstPtr, srcPtr, planeBufSize);
            }
        }

        // set color arrangement
        pOImgBuffer->setColorArrangement(colorOrder);

        if (mDumpBuffer) {
            // dump otuput buffer
            const char* pUserString = String8::format("hdr-out-%d",
                                                      pRequest->mRequestIndex).c_str();
            IMetadata* pHalMeta = pRequest->mIMetadataHal->acquire();
            dumpBuffer(pHalMeta, pRequest->mSensorId, pOImgBuffer, pUserString);
        }
    }

    if (isIMetaHal) {
        IMetadata* pIMetaHal = pRequest->mIMetadataHal->acquire();
        if (pIMetaHal != NULL) {
            MY_LOGD("[IN] IMetadataHal count: %d", pIMetaHal->count());

            // get capture param
            IMetadata::Memory capParam;
            CaptureParam_T exposureParam;
            if (IMetadata::getEntry<IMetadata::Memory>(pIMetaHal,
                                                       MTK_3A_AE_CAP_PARAM, capParam)) {
                if (capParam.size() == sizeof(CaptureParam_T)) {
                    memcpy(&exposureParam, capParam.editArray(), sizeof(CaptureParam_T));
                    MY_LOGD("========= Request Capture Param [%d] =========",
                            pRequest->mRequestIndex);
                    MY_LOGD("u4Eposuretime = %u", exposureParam.u4Eposuretime);
                    MY_LOGD("u4AfeGain = %u", exposureParam.u4AfeGain);
                    MY_LOGD("u4IspGain = %u", exposureParam.u4IspGain);
                }
            }
        }
    }

    mvpRequest.push_back(pRequest);
    MY_LOGD("collect request(%d/%d)", pRequest->mRequestIndex+1, pRequest->mRequestCount);

    if (pRequest->mRequestIndex == pRequest->mRequestCount - 1) {
        MY_LOGD("have collected all requests");

        // perform callback
        for (auto& req : mvpRequest) {
            MY_LOGD("callback request(%d/%d) %p",
                    req->mRequestIndex+1, req->mRequestCount, pCallback.get());
            if (pCallback != nullptr) {
                pCallback->onCompleted(req, 0);
            }
        }

        // clear data
        mvpRequest.clear();
    }

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
void
HDRPluginProviderImp::
abort(std::vector<RequestPtr>& pRequests)
{
    CAM_ULOGM_APILIFE();

    bool isAbort = false;

    if (mpCallback == nullptr) {
        MY_LOGA("callbackptr is null");
    }

    for (auto& req: pRequests) {
        isAbort = false;

        for (std::vector<RequestPtr>::iterator it = mvpRequest.begin();
             it != mvpRequest.end(); ++it) {
            if ((*it) == req) {
                mvpRequest.erase(it);
                mpCallback->onAborted(req);
                isAbort = true;
                break;
            }
        }

        if (!isAbort) {
            MY_LOGW("desire abort request[%d] is not found", req->mRequestIndex);
        }
    }

    if (!mvpRequest.empty()) {
        MY_LOGW("abort() does not clean all the requests");
    } else {
        MY_LOGW("abort() cleans all the requests");
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
HDRPluginProviderImp::
uninit()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
bool
HDRPluginProviderImp::
getCaptureParam(const MINT32& sensorId, const MUINT8& requestCount)
{
    CAM_ULOGM_APILIFE();

    std::unique_ptr<IHal3A, std::function<void(IHal3A*)>> hal3a (
        MAKE_Hal3A(sensorId, "HDR Plugin"),
        [](IHal3A* p) {
            if (p) {
                p->destroyInstance("HDR Plugin");
            }
        }
        );

    if (hal3a.get() == nullptr) {
        MY_LOGE("create IHal3A instance failed");
        return false;
    }

    // get capture param
    mvCaptureParam.clear();
    CaptureParam_T tmpCaptureParam;
    hal3a->send3ACtrl(E3ACtrl_GetExposureParam,
                      reinterpret_cast<MINTPTR>(&tmpCaptureParam), 0);
    for (MINT32 i = 0; i < requestCount; ++i) {
        mvCaptureParam.push_back(tmpCaptureParam);
    }

    // dump debug info
    dumpCaptureParam();

    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
void
HDRPluginProviderImp::
dumpCaptureParam()
{
    for (MINT32 i = 0; i < mvCaptureParam.size(); ++i) {
        CaptureParam_T& captureParam = mvCaptureParam.at(i);
        MY_LOGD("========= Capture Param [%d] =========", i);
        MY_LOGD("u4ExposureMode = %u", captureParam.u4ExposureMode);
        MY_LOGD("u4Eposuretime = %u", captureParam.u4Eposuretime);
        MY_LOGD("u4AfeGain = %u", captureParam.u4AfeGain);
        MY_LOGD("u4IspGain = %u", captureParam.u4IspGain);
        MY_LOGD("u4RealISO = %u", captureParam.u4RealISO);
        MY_LOGD("u4FlareGain = %u", captureParam.u4FlareGain);
        MY_LOGD("u4FlareOffset = %u", captureParam.u4FlareOffset);
        MY_LOGD("i4LightValue_x10 = %u", captureParam.i4LightValue_x10);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
HDRPluginProviderImp::
dumpBuffer(const IMetadata* pHalMeta,
           const MINT32 sensorId,
           IImageBuffer* pIImgBuffer,
           const char* pUserString)
{
    char fileName[256] = "";

    FILE_DUMP_NAMING_HINT hint{};
    memset(&hint, 0, sizeof(FILE_DUMP_NAMING_HINT));

    // extract hint from metadata
    extract(&hint, pHalMeta);

    // extract hint from sensor
    extract_by_SensorOpenId(&hint, sensorId);

    // extract hint from buffer
    extract(&hint, pIImgBuffer);

    if (mAlgoType == RawDomainHDR) {
        genFileName_RAW(fileName, sizeof(fileName), &hint, RAW_PORT_NULL, pUserString);
    } else {
        genFileName_YUV(fileName, sizeof(fileName), &hint, YUV_PORT_NULL, pUserString);
    }

    pIImgBuffer->saveToFile(fileName);
}

void
HDRPluginProviderImp::
writeFeatureDecisionLog(int32_t sensorId,
                        IMetadata* pHalInMeta,
                        const char* log)
{
    if (IScenarioRecorder::getInstance()->isScenarioRecorderOn()
        && log) {
        int32_t requestNum = -1;
        if (!IMetadata::getEntry<MINT32>(pHalInMeta,
                                         MTK_PIPELINE_REQUEST_NUMBER, requestNum)) {
            MY_LOGW("Get MTK_PIPELINE_REQUEST_NUMBER fail");
        }

        int32_t uniquekey = 0;
        if (!IMetadata::getEntry<MINT32>(pHalInMeta,
                                         MTK_PIPELINE_UNIQUE_KEY, uniquekey)) {
            MY_LOGW("Get MTK_PIPELINE_UNIQUE_KEY fail");
        }

        DecisionParam decParam;
        DebugSerialNumInfo& dbgNumInfo = decParam.dbgNumInfo;
        dbgNumInfo.uniquekey = uniquekey;
        dbgNumInfo.reqNum = requestNum;
        decParam.decisionType = DECISION_FEATURE;
        decParam.moduleId = NSCam::Utils::ULog::MOD_LIB_HDR;
        decParam.sensorId = sensorId;
        decParam.isCapture = true;
        IScenarioRecorder::getInstance()->submitDecisionRecord(decParam, log);
    }
}
