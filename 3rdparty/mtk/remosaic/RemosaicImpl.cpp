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
 * MediaTek Inc. (C) 2018. All rights reserved.
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
#define LOG_TAG "RemosaicProvider"
//static const char* __CALLERNAME__ = LOG_TAG;
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
//
#include <stdlib.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <sstream>
//
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
//
//
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
//
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/std/Format.h>
//
#include <mtkcam3/pipeline/hwnode/NodeId.h>

#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metastore/ITemplateRequest.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>
#include <mtkcam/utils/hw/IPlugProcessing.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>
#include <MTKDngOp.h>
#include <cutils/properties.h>

static MTKDngOp *MyDngop;
static DngOpResultInfo MyDngopResultInfo;
static DngOpImageInfo MyDngopImgInfo;

//
using namespace NSCam;
using namespace android;
using namespace std;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCamHW;
using namespace NSCam::TuningUtils::scenariorecorder;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
//
#define FUNCTION_SCOPE \
auto __scope_logger__ = [](char const* f)->std::shared_ptr<const char>{ \
    CAM_ULOGMD("(%d)[%s] + ", ::gettid(), f); \
    return std::shared_ptr<const char>(f, [](char const* p){CAM_ULOGMD("(%d)[%s] -", ::gettid(), p);}); \
}(__FUNCTION__)
//
#define __DEBUG // enable debug
#ifdef __DEBUG
#define ATRACE_TAG                                  ATRACE_TAG_CAMERA
#include <utils/Trace.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAPTURE_RAW);
#define FUNCTION_TRACE()                            ATRACE_CALL()
#define FUNCTION_TRACE_NAME(name)                   ATRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)                  ATRACE_BEGIN(name)
#define FUNCTION_TRACE_END()                        ATRACE_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)    ATRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)      ATRACE_ASYNC_END(name, cookie)
#else
#define FUNCTION_TRACE()
#define FUNCTION_TRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)
#define FUNCTION_TRACE_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)
#endif
//
#define ASSERT(cond, msg)           do { if (!(cond)) { printf("Failed: %s\n", msg); return; } }while(0)
#define ISO_THRESHOLD_4CellSENSOR       (800)  //ISO threshold
/******************************************************************************
 *
 ******************************************************************************/
template <typename T>
inline MBOOL
tryGetMetadata(
    IMetadata const* metadata,
    MUINT32 const tag,
    T & rVal
)
{
    if( metadata == NULL ) {
        MY_LOGE("InMetadata == NULL");
        return MFALSE;
    }

    IMetadata::IEntry entry = metadata->entryFor(tag);
    if( !entry.isEmpty() ) {
        rVal = entry.itemAt(0, Type2Type<T>());
        return MTRUE;
    }
    return MFALSE;
}


/******************************************************************************
 *
 ******************************************************************************/
IImageBuffer*
createWorkingBuffer(
    char const *szName __unused,
    MINT32 format,
    MSize size)
{
    FUNCTION_TRACE();
    // query format
    MUINT32 plane = NSCam::Utils::Format::queryPlaneCount(format);
    size_t bufBoundaryInBytes[3] = {0, 0, 0};
    size_t bufStridesInBytes[3] = {0};

    for (MUINT32 i = 0; i < plane; i++) {
        bufStridesInBytes[i] = NSCam::Utils::Format::queryPlaneWidthInPixels(format, i, size.w) *
                               NSCam::Utils::Format::queryPlaneBitsPerPixel(format, i) / 8;
    }
    // create buffer
    IImageBufferAllocator::ImgParam imgParam =
            IImageBufferAllocator::ImgParam(
                    (EImageFormat) format,
                    size, bufStridesInBytes,
                    bufBoundaryInBytes, plane);

    sp<IImageBufferHeap> pHeap =
            IIonImageBufferHeap::create(LOG_TAG, imgParam);
    if (pHeap == NULL) {
        MY_LOGE("working buffer[%s]: create heap failed", LOG_TAG);
        return NULL;
    }
    IImageBuffer* pImageBuffer = pHeap->createImageBuffer();
    if (pImageBuffer == NULL) {
        MY_LOGE("working buffer[%s]: create image buffer failed", LOG_TAG);
        return NULL;
    }

    // lock buffer
    MUINT const usage = (GRALLOC_USAGE_SW_READ_OFTEN |
                         GRALLOC_USAGE_SW_WRITE_OFTEN |
                         GRALLOC_USAGE_HW_CAMERA_READ |
                         GRALLOC_USAGE_HW_CAMERA_WRITE);
    if (!(pImageBuffer->lockBuf(LOG_TAG, usage))) {
        MY_LOGE("working buffer[%s]: lock image buffer failed", LOG_TAG);
        return NULL;
    }

    return pImageBuffer;
}


/******************************************************************************
 *
 ******************************************************************************/
void dumpBuffer(IImageBuffer* pBuf, const char* filename, const char* fileext)
{
#define dumppath "/data/vendor/camera_dump/RemosaicProc"
    char fname[256];
    if(sprintf(fname, "%s/%s_%dx%d.%s",
            dumppath,
            filename,
            pBuf->getImgSize().w,
            pBuf->getImgSize().h,
            fileext
            ) >= 0)
    {pBuf->saveToFile(fname);}
#undef dumppath
}


/******************************************************************************
*
******************************************************************************/
class RemosaicProviderImpl : public RawPlugin::IProvider
{
    typedef RawPlugin::Property Property;
    typedef RawPlugin::Selection Selection;
    typedef RawPlugin::Request::Ptr RequestPtr;
    typedef RawPlugin::RequestCallback::Ptr RequestCallbackPtr;
public:

    virtual void set(MINT32 iOpenId, MINT32 iOpenId2)
    {
        MY_LOGD("set openId:%d openId2:%d", iOpenId, iOpenId2);
        MY_LOGE("TODO: need to be removed");
    }

    virtual const Property& property()
    {
        FUNCTION_SCOPE;
        if (!minited)
        {
            mprop.mName = "MTK Remosaic";
            mprop.mFeatures = MTK_FEATURE_REMOSAIC;
            mprop.mInPlace = MTRUE;
            minited = true;
        }
        return mprop;
    };

    virtual MERROR negotiate(Selection& sel)
    {
        FUNCTION_SCOPE;
        FUNCTION_TRACE();

        mDumpBuffer = property_get_int32("vendor.debug.camera.RemosaicDump", 0);
        mEnable = property_get_int32("vendor.debug.camera.remo.enable", -1);
        int config_proc_raw = property_get_int32("vendor.debug.camera.cfg.ProcRaw", -1);
        int raw_type = property_get_int32("vendor.debug.camera.raw.type", -1);
        int afraw = property_get_int32("vendor.debug.camera.afraw", -1);
        if (mEnable == -1 && (config_proc_raw == 1 || raw_type == 0 || afraw == 1))
        {
            mEnable = 0;
        }

        NSCamHW::HwInfoHelper helper(sel.mSensorId);
        if( ! helper.updateInfos() )
        {
            MY_LOGE("cannot properly update infos");
        }
        //
        MSize captureSize;
        MSize customModeSize;
        mFullSensorMode = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;

        MBOOL ret = MTRUE;
        ret = helper.getSensorSize(SENSOR_SCENARIO_ID_NORMAL_CAPTURE, captureSize);
        if(ret)
            MY_LOGD("sensor capture Size is %dx%d",captureSize.w, captureSize.h);

        for(size_t idx=SENSOR_SCENARIO_ID_CUSTOM1; idx<= SENSOR_SCENARIO_ID_CUSTOM15; idx++)
        {
            ret = helper.getSensorSize(idx, customModeSize);
            if(ret)
                MY_LOGD("mode:%lu size is %dx%d", idx, customModeSize.w, customModeSize.h);
            if( customModeSize.w > captureSize.w && customModeSize.h > captureSize.h )
            {
                mFullSensorMode = idx;
                mIsSensorModeHided = true;
                MY_LOGD("Update full sensor mode to %lu",idx);
                break;
            }
        }

        // scenario recorder
        auto pScenarioRecorder = IScenarioRecorder::getInstance();
        DecisionParam decParm = {};
        DebugSerialNumInfo &dbgNumInfo = decParm.dbgNumInfo;

        dbgNumInfo.reqNum = 0;
        //dbgNumInfo.uniquekey = uniqueKey;
        //dbgNumInfo.magicNum = magicNum;
        decParm.decisionType = DECISION_FEATURE;  // Decision type which you want to record it
        decParm.moduleId = NSCam::Utils::ULog::MOD_LIB_REMO;
        decParm.sensorId = sel.mSensorId;
        decParm.isCapture = true;
        std::string debugString;

        //
        if(CC_LIKELY(sel.mIMetadataHal.getControl() != NULL))
        {
            IMetadata* pHalMeta = sel.mIMetadataHal.getControl().get();
            IMetadata::IEntry const eReqNo = pHalMeta->entryFor(MTK_PIPELINE_REQUEST_NUMBER);
            if(!eReqNo.isEmpty())
            {
                dbgNumInfo.reqNum = eReqNo.itemAt(0, Type2Type<MINT32>());
            }
        }

        HwInfoHelper::e4CellSensorPattern fcellSensorPattern = helper.get4CellSensorPattern();

        switch(fcellSensorPattern)
        {
            case HwInfoHelper::e4CellSensorPattern_Unknown:
                debugString = "Remosaic not triggered: it's not a 4Cell Sensor";
                pScenarioRecorder->submitDecisionRecord(decParm,debugString);
                return BAD_VALUE;

            case HwInfoHelper::e4CellSensorPattern_Unpacked:
            {
                sp<IPlugProcessing> pPlugProcessing = IPlugProcessing::createInstance(
                (MUINT32) IPlugProcessing::PLUG_ID_FCELL,
                (NSCam::IPlugProcessing::DEV_ID) sel.mSensorId);

                MBOOL ack = MFALSE;
                pPlugProcessing->sendCommand(NSCam::NSCamPlug::ACK, (MINTPTR) &ack);
                if( !ack )
                {
                    debugString = "Remosaic not triggered: remosaic library is not ported";
                    pScenarioRecorder->submitDecisionRecord(decParm,debugString);

                    MY_LOGE("FIX ME: have no ACK of SW-remosaic plugin!!");
                    return NO_INIT;
                }
                MY_LOGD("Update InPlace property for sw-remosaic");
                mprop.mInPlace = MFALSE;
                break;
            }
            case HwInfoHelper::e4CellSensorPattern_Packed:
                break;
            default:
                break;
        }

        auto AppStreamInfo = sel.mState.pParsedAppImageStreamInfo;
        if( AppStreamInfo != nullptr && (AppStreamInfo->vAppImage_Output_RAW16.size() > 0 || AppStreamInfo->vAppImage_Output_RAW10.size() > 0 ))
        {
            auto& SensorSize = (mFullSensorMode == SENSOR_SCENARIO_ID_NORMAL_CAPTURE) ?
                                captureSize : customModeSize;
            MSize AppRawSize;
            if(AppStreamInfo->vAppImage_Output_RAW16.size() > 0)
                AppRawSize = AppStreamInfo->vAppImage_Output_RAW16.begin()->second->getImgSize();
            else
                AppRawSize = AppStreamInfo->vAppImage_Output_RAW10.begin()->second->getImgSize();

            if (AppRawSize.w < SensorSize.w && AppRawSize.h < SensorSize.h)
            {
                MY_LOGE("disable remosaic [AppRawSize(%ux%u) SensorSize(%ux%u)]",
                        AppRawSize.w, AppRawSize.h, SensorSize.w, SensorSize.h);
                debugString = "Remosaic not triggered: app raw size is equal to current sensor size";
                pScenarioRecorder->submitDecisionRecord(decParm,debugString);
                return BAD_VALUE;
            }
            MY_LOGI("Full Size Raw Required for dng [AppRawSize(%ux%u) SensorSize(%ux%u)]",
                    AppRawSize.w, AppRawSize.h, SensorSize.w, SensorSize.h);
            mIsDng = true;
        }

        if(CC_LIKELY(sel.mIMetadataApp.getControl() != NULL))
        {
            IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();
            IMetadata::IEntry const eCap_intent = pAppMeta->entryFor(MTK_CONTROL_CAPTURE_INTENT);
            if(!eCap_intent.isEmpty() &&
                (eCap_intent.itemAt(0, Type2Type<MUINT8>()) == MTK_CONTROL_CAPTURE_INTENT_VIDEO_RECORD ||
                 eCap_intent.itemAt(0, Type2Type<MUINT8>()) == MTK_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT))
            {
                MY_LOGD("disable remosaic for VSS");
                debugString = "Remosaic not triggered: combination of VSS and remosaic is not supported.";
                pScenarioRecorder->submitDecisionRecord(decParm,debugString);
                return BAD_VALUE;
            }
            IMetadata::IEntry const eDoRemosaic = pAppMeta->entryFor(MTK_CONTROL_CAPTURE_REMOSAIC_EN);
            if( (eDoRemosaic.isEmpty() ||
                eDoRemosaic.itemAt(0, Type2Type<MINT32>()) != 1) && !mIsDng)
            {
                MY_LOGD("disable remosaic because AP didn't enable");
                debugString = "Remosaic not triggered: vendor tag MTK_CONTROL_CAPTURE_REMOSAIC_EN is not set by AP";
                pScenarioRecorder->submitDecisionRecord(decParm,debugString);
                return BAD_VALUE;
            }
            IMetadata::IEntry const eisCshot = pAppMeta->entryFor(MTK_CSHOT_FEATURE_CAPTURE);
            if(!eisCshot.isEmpty() && eisCshot.itemAt(0, Type2Type<MINT32>()) == 1)
            {
                MY_LOGD("disable remosaic for Cshot");
                debugString = "Remosaic not triggered: combination of cshot and remosaic is not supported.";
                pScenarioRecorder->submitDecisionRecord(decParm,debugString);
                return BAD_VALUE;
            }
        }

        //TODO: flashOn
        mbFlashOn = sel.mState.mFlashFired;
        mRealIso = sel.mState.mRealIso;
        if(!IsEnableRemosaic(debugString))
        {
            pScenarioRecorder->submitDecisionRecord(decParm,debugString);
            MY_LOGE("disable remosaic");
            return BAD_VALUE;
        }

        pScenarioRecorder->submitDecisionRecord(decParm,debugString);
        //update MTK_HAL_REQUEST_REMOSAIC_ENABLE for ae_mgr
        {
            MetadataPtr pHalAddtional = make_shared<IMetadata>();
            IMetadata* pHalMeta = pHalAddtional.get();
            IMetadata::setEntry<MUINT8>(pHalMeta, MTK_HAL_REQUEST_REMOSAIC_ENABLE, (MUINT8)1);
            sel.mIMetadataHal.setAddtional(pHalAddtional);
        }

        sel.mDecision.mSensorMode = mFullSensorMode;
        sel.mIBufferFull
            .setRequired(MTRUE)
            .addAcceptedFormat(eImgFmt_BAYER10_MIPI)
            .addAcceptedFormat(eImgFmt_BAYER12_UNPAK)
            .addAcceptedFormat(eImgFmt_BAYER10_UNPAK)
            .addAcceptedFormat(eImgFmt_BAYER12)
            .addAcceptedFormat(eImgFmt_BAYER10)
            .addAcceptedFormat(eImgFmt_BAYER16_APPLY_LSC)
            .addAcceptedSize(eImgSize_Full);

        sel.mOBufferFull
            .setRequired(MTRUE)
            .addAcceptedFormat(eImgFmt_BAYER10_MIPI)
            .addAcceptedFormat(eImgFmt_BAYER12_UNPAK)
            .addAcceptedFormat(eImgFmt_BAYER10_UNPAK)
            .addAcceptedFormat(eImgFmt_BAYER12)
            .addAcceptedFormat(eImgFmt_BAYER10)
            .addAcceptedFormat(eImgFmt_BAYER16_APPLY_LSC)
            .addAcceptedSize(eImgSize_Full);
        sel.mAlignInputFmt = true;


        sel.mIMetadataDynamic.setRequired(MTRUE);
        sel.mIMetadataApp.setRequired(MTRUE);
        sel.mIMetadataHal.setRequired(MTRUE);
        sel.mOMetadataApp.setRequired(MTRUE);
        sel.mOMetadataHal.setRequired(MTRUE);

        return OK;
    };

    virtual void init()
    {
        FUNCTION_SCOPE;
        FUNCTION_TRACE();
    };

    virtual MERROR process(RequestPtr pRequest,
                           RequestCallbackPtr pCallback = nullptr)
    {
        FUNCTION_SCOPE;
        FUNCTION_TRACE();

        IImageBuffer *pInBuf = pRequest->mIBufferFull->acquire();
        if(mDumpBuffer)
        {
            dumpBuffer(pInBuf,"4cell-OriginalRawBuf","raw");
        }

        NSCamHW::HwInfoHelper helper(pRequest->mSensorId);
        if( ! helper.updateInfos() )
        {
            MY_LOGE("cannot properly update infos");
        }
        HwInfoHelper::e4CellSensorPattern fcellSensorPattern = helper.get4CellSensorPattern();

        if( fcellSensorPattern == HwInfoHelper::e4CellSensorPattern_Packed)
        {
            MY_LOGD(" by-pass emplace buffer for HW-remosaic");
            if (pCallback != nullptr) {
                MY_LOGD("callback request");
                pCallback->onCompleted(pRequest, 0);
            }
            return 0;
        }

        if (pRequest->mIBufferFull != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mIBufferFull->acquire();
            MY_LOGD("[IN] Full image VA: 0x%" PRIxPTR "", pImgBuffer->getBufVA(0));
        }

        if (pRequest->mOBufferFull != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mOBufferFull->acquire();
            MY_LOGD("[OUT] Full image VA: 0x%" PRIxPTR "", pImgBuffer->getBufVA(0));
        }

        if (pRequest->mIMetadataDynamic != nullptr) {
            IMetadata *meta = pRequest->mIMetadataDynamic->acquire();
            if (meta != NULL)
                MY_LOGD("[IN] Dynamic metadata count: %u", meta->count());
            else
                MY_LOGD("[IN] Dynamic metadata empty");
        }

        void *pInVa    = (void *) (pInBuf->getBufVA(0));
        int nImgWidth  = pInBuf->getImgSize().w;
        int nImgHeight = pInBuf->getImgSize().h;
        int nBufSize   = pInBuf->getBufSizeInBytes(0);
        int nImgStride = pInBuf->getBufStridesInBytes(0);


#if MTKCAM_REMOSAIC_UNPACK_INOUTPUT_SUPPORT
        sp<IImageBuffer> pUpkOutBuf = pInBuf;
        void *pUpkOutVa = pInVa;
#else
        sp<IImageBuffer> pUpkOutBuf = createWorkingBuffer("Fcell",eImgFmt_BAYER10_UNPAK,pInBuf->getImgSize());
        if(pUpkOutBuf.get() == NULL)
        {
            MY_LOGE("createWorkingBuffer fails, out-of-memory?");
            return -1;
        }
        void *pUpkOutVa = (void *) (pUpkOutBuf->getBufVA(0));
        // unpack algorithm
        MY_LOGD("Unpack +");
        MyDngop = MyDngop->createInstance(DRV_DNGOP_UNPACK_OBJ_SW);
        MyDngopImgInfo.Width = nImgWidth;
        MyDngopImgInfo.Height = nImgHeight;
        MyDngopImgInfo.Stride_src = nImgStride;
        MyDngopImgInfo.Stride_dst = pUpkOutBuf->getBufStridesInBytes(0);
        MyDngopImgInfo.BIT_NUM = 10;
        MyDngopImgInfo.BIT_NUM_DST = 10;
        MUINT32 buf_size = DNGOP_BUFFER_SIZE(nImgWidth * 2, nImgHeight);
        MyDngopImgInfo.Buff_size = buf_size;
        MyDngopImgInfo.srcAddr = pInVa;
        MyDngopResultInfo.ResultAddr = pUpkOutVa;
        MyDngop->DngOpMain((void*)&MyDngopImgInfo, (void*)&MyDngopResultInfo);
        MyDngop->destroyInstance(MyDngop);
        MY_LOGD("Unpack -");
        MY_LOGD("unpack processing. va[in]:%p, va[out]:%p", MyDngopImgInfo.srcAddr, MyDngopResultInfo.ResultAddr);
        MY_LOGD("img size(%dx%d) src stride(%d) bufSize(%d) -> dst stride(%d) bufSize(%zu)", nImgWidth, nImgHeight,
                  MyDngopImgInfo.Stride_src,nBufSize, MyDngopImgInfo.Stride_dst , pUpkOutBuf->getBufSizeInBytes(0));
        if(mDumpBuffer)
        {
            dumpBuffer(pUpkOutBuf.get(),"4cell-UnpackedBuf","raw");
        }
#endif
        //Step 1: Init Fcell Library
        PlugInitParam initParam;
        initParam.openId = pRequest->mSensorId;
        initParam.img_w = nImgWidth;
        initParam.img_h = nImgHeight;
        initParam.sensor_mode = mFullSensorMode;

        sp<IPlugProcessing> pPlugProcessing = IPlugProcessing::createInstance(
                (MUINT32) IPlugProcessing::PLUG_ID_FCELL,
                (NSCam::IPlugProcessing::DEV_ID) pRequest->mSensorId);
        pPlugProcessing->sendCommand(NSCam::NSCamPlug::SET_PARAM, NSCam::IPlugProcessing::PARAM_INIT, (MINTPTR)&initParam);

        MERROR res = OK;
        res = pPlugProcessing->init(IPlugProcessing::OP_MODE_SYNC);
        if(res == OK)
            MY_LOGD("REMOSAIC - Libinit");
        pPlugProcessing->waitInitDone();

        //Step 2: Prepare parameters to call Fcell library
        MINT32 sensor_order;
        pPlugProcessing->sendCommand(NSCam::NSCamPlug::GET_PARAM, (MINTPTR)&sensor_order);
        MY_LOGD("Sensor order get from lib (%d)", sensor_order);

#if !MTKCAM_REMOSAIC_UNPACK_INOUTPUT_SUPPORT
        sp<IImageBuffer> pFcellUpkOutBuf = createWorkingBuffer("Fcell",eImgFmt_BAYER10_UNPAK,pInBuf->getImgSize());
        if(pFcellUpkOutBuf.get() == NULL)
        {
            MY_LOGE("createWorkingBuffer fails, out-of-memory?");
            return -1;
        }
        void *pFcellUpkOutVa = (void *) (pFcellUpkOutBuf->getBufVA(0));
        MY_LOGD("fcell processing: src buffer size(%zu) dst buffer size(%zu)",pUpkOutBuf->getBufSizeInBytes(0), pFcellUpkOutBuf->getBufSizeInBytes(0));
#else
        sp<IImageBuffer> pFcellUpkOutBuf = pRequest->mOBufferFull->acquire();
        void *pOutVa = (void *) (pFcellUpkOutBuf->getBufVA(0));
        void *pFcellUpkOutVa = pOutVa;
#endif
        int16_t *pFcellImage = static_cast<int16_t*>(pUpkOutVa);
        if(pFcellImage == NULL)
        {
            MY_LOGE("pFcellImage allocate fail.");
        }
        int16_t *pOutImage = static_cast<int16_t*>(pFcellUpkOutVa);
        if(pOutImage == NULL)
        {
            MY_LOGE("pOutImage allocate fail.");
        }

        //Step 3: Process Fcell.
        struct timeval start, end;
        gettimeofday( &start, NULL );
        MY_LOGD("fcellprocess begin");
        PlugProcessingParam ProcessingParam;
        ProcessingParam.src_buf_fd = pUpkOutBuf->getFD(0);
        ProcessingParam.dst_buf_fd = pFcellUpkOutBuf->getFD(0);
        ProcessingParam.img_w = nImgWidth;
        ProcessingParam.img_h = nImgHeight;
        ProcessingParam.src_buf_size = pUpkOutBuf->getBufSizeInBytes(0);
        ProcessingParam.dst_buf_size = pFcellUpkOutBuf->getBufSizeInBytes(0);
        ProcessingParam.src_buf = (unsigned short* )pFcellImage;
        ProcessingParam.dst_buf = (unsigned short* )pOutImage;
        {
            int analog_gain = 0, awb_rgain = 0, awb_ggain = 0, awb_bgain = 0;
            bool ret = 1;
            IMetadata *inHalmeta = pRequest->mIMetadataHal->acquire();
            ret &= tryGetMetadata<MINT32>(inHalmeta, MTK_ANALOG_GAIN, analog_gain);
            ret &= tryGetMetadata<MINT32>(inHalmeta, MTK_AWB_RGAIN, awb_rgain);
            ret &= tryGetMetadata<MINT32>(inHalmeta, MTK_AWB_GGAIN, awb_ggain);
            ret &= tryGetMetadata<MINT32>(inHalmeta, MTK_AWB_BGAIN, awb_bgain);
            MY_LOGD("ret = %d, analog_gain = %d, awb_rgain = %d, awb_ggain = %d, awb_bgain = %d", ret,  analog_gain, awb_rgain, awb_ggain, awb_bgain);
            ProcessingParam.gain_awb_r = awb_rgain;
            ProcessingParam.gain_awb_gr = awb_ggain;
            ProcessingParam.gain_awb_gb = awb_ggain;
            ProcessingParam.gain_awb_b = awb_bgain;
            ProcessingParam.gain_analog = analog_gain;
        }

        if(pPlugProcessing->sendCommand(NSCam::NSCamPlug::PROCESS, (MINTPTR)(&ProcessingParam), (MINTPTR)0, (MINTPTR)0, (MINTPTR)0) != OK)
        {
            MY_LOGE("fcellprocess fails!!!\n");
            return -1;
        }
        gettimeofday( &end, NULL );
        MY_LOGD("fcell process finish, time: %ld ms.\n", 1000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec)/1000);
        if(mDumpBuffer)
        {
            dumpBuffer(pFcellUpkOutBuf.get(),"4cell-RemosaicBuf","raw");
        }
#if !MTKCAM_REMOSAIC_UNPACK_INOUTPUT_SUPPORT
        // pack algorithm
        IImageBuffer *pOutBuf = pRequest->mOBufferFull->acquire();
        void *pOutVa = (void *) (pOutBuf->getBufVA(0));
        MY_LOGD("Pack +");
        MyDngop = MyDngop->createInstance(DRV_DNGOP_PACK_OBJ_SW);
        MyDngopImgInfo.Width = nImgWidth;
        MyDngopImgInfo.Height = nImgHeight;
        MyDngopImgInfo.Stride_src = nImgWidth * 2;
        MyDngopImgInfo.Stride_dst = pOutBuf->getBufStridesInBytes(0);
        int bit_depth = 10;
        MyDngopImgInfo.BIT_NUM = bit_depth;
        MyDngopImgInfo.Bit_Depth = bit_depth;
        buf_size = DNGOP_BUFFER_SIZE(pOutBuf->getBufStridesInBytes(0), nImgHeight);
        MyDngopImgInfo.Buff_size = buf_size;
        MyDngopImgInfo.srcAddr = pFcellUpkOutVa;
        MyDngopResultInfo.ResultAddr = pOutVa;
        MyDngop->DngOpMain((void*)&MyDngopImgInfo, (void*)&MyDngopResultInfo);
        MyDngop->destroyInstance(MyDngop);
        MY_LOGD("Pack -");
        if(mDumpBuffer)
        {
            dumpBuffer(pOutBuf,"4cell-PackedBuf","raw");
        }
        MY_LOGD("pack processing. va[in]:%p, va[out]:%p", MyDngopImgInfo.srcAddr, MyDngopResultInfo.ResultAddr);
        MY_LOGD("img size(%dx%d) src stride(%d) bufSize(%d) -> dst stride(%d) bufSize(%zu)", nImgWidth, nImgHeight,
                  MyDngopImgInfo.Stride_src,MyDngopImgInfo.Buff_size, MyDngopImgInfo.Stride_dst , pOutBuf->getBufSizeInBytes(0));
        pUpkOutBuf->unlockBuf(LOG_TAG);
        pFcellUpkOutBuf->unlockBuf(LOG_TAG);
#endif
        IImageBuffer* pOutImgBuffer = pRequest->mOBufferFull->acquire();
        // need to set sensor order before enque to p2 driver
        pOutImgBuffer->setColorArrangement(sensor_order);
        pOutImgBuffer->syncCache(eCACHECTRL_FLUSH);

        if (pCallback != nullptr) {
            MY_LOGD("callback request");
            pCallback->onCompleted(pRequest, 0);
        }

        return 0;
    };

    virtual void abort(vector<RequestPtr>& pRequests __unused)
    {
        FUNCTION_SCOPE;
    };

    virtual void uninit()
    {
        FUNCTION_SCOPE;
        FUNCTION_TRACE();
    };

    virtual ~RemosaicProviderImpl()
    {
        FUNCTION_SCOPE;
    };
private:
    //functions
    bool IsEnableRemosaic(std::string& debugString)
    {
        FUNCTION_SCOPE;
        int threshold_4Cell = property_get_int32("vendor.debug.camera.threshold_4Cell", -1);
        int iso_threshold = (threshold_4Cell>=0)? threshold_4Cell : ISO_THRESHOLD_4CellSENSOR;
        MY_LOGE("TODO: please fix it. <appISOSpeed checking>");
        int appISOSpeed = 0;
        MY_LOGD("4cell flow condition: debug:threshold_4Cell(%d), apply:iso_threshold(%d), def:ISO_THRESHOLD_4CellSENSOR(%d), mbFlashOn(%d), currentIso(%d), appISOSpeed(%d)",
            threshold_4Cell, iso_threshold, ISO_THRESHOLD_4CellSENSOR, mbFlashOn, mRealIso, appISOSpeed);

        switch(__builtin_expect(mEnable,-1)){
            case 1:
                MY_LOGD("Force enable remosaic processing");
                debugString = "Remosaic triggered: force enabled by adb command";
                return true;
            case 0:
                MY_LOGD("Force disable remosaic processing");
                debugString = "Remosaic not triggered: force disabled by adb command";
                return false;
            default:
                break;
        }

        string statusString;
        if(mIsDng)
            statusString = "full size dng needed.";
        else
        {
            statusString = "status:actual(expect) isFlashOn:"+std::to_string(mbFlashOn)+"(0) currentIso:"+
                std::to_string(mRealIso)+"(<"+std::to_string(iso_threshold)+")";
        }
        if((!mbFlashOn && mRealIso < iso_threshold && appISOSpeed < iso_threshold) || mIsDng)
        {
            debugString = "Remosaic triggered: "+statusString;
            return true;
        }

        debugString = "Remosaic not triggered: "+statusString;
        return false;
    }

    //variables
    MINT32                      mRealIso;
    MBOOL                       mbFlashOn;
    MINT32                      mDumpBuffer;
    MINT32                      mEnable = 0;
    Property                    mprop;
    bool                        minited = false;
    bool                        mIsSensorModeHided = false;
    bool                        mIsDng = false;
    MUINT32                     mFullSensorMode = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
};

REGISTER_PLUGIN_PROVIDER(Raw, RemosaicProviderImpl);

