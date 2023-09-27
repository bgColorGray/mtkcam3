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
#define LOG_TAG "MFNRPlugin"

//
#include <mtkcam/utils/std/ULog.h>
//
#include <stdlib.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <sstream>
#include <unordered_map> // std::unordered_map
//
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <custom/feature/mfnr/camera_custom_mfll.h>
//zHDR
#include <mtkcam/utils/hw/HwInfoHelper.h> // NSCamHw::HwInfoHelper
#include <mtkcam3/feature/utils/FeatureProfileHelper.h> //ProfileParam
#include <mtkcam/drv/IHalSensor.h>
//
#include <mtkcam/utils/hw/FleetingQueue.h>
//
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
//
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/std/Time.h>
//
#include <mtkcam3/pipeline/hwnode/NodeId.h>
//
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metastore/ITemplateRequest.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>
#include "MFNRShotInfo.h"
#include "inc/IMFNRCapability.h"
//
#include <isp_tuning/isp_tuning.h>  //EIspProfile_T, EOperMode_*
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>
#include <mtkcam/utils/TuningUtils/ScenarioRecorderDef.h>
//

using namespace NSCam;
using namespace plugin;
using namespace android;
using namespace mfll;
using namespace std;
using namespace NSCam::NSPipelinePlugin;
using namespace NSIspTuning;
/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_MFNR);
#undef MY_LOGV
#undef MY_LOGD
#undef MY_LOGI
#undef MY_LOGW
#undef MY_LOGE
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
#define ASSERT(cond, msg)           do { if (!(cond)) { printf("Failed: %s\n", msg); return; } }while(0)


#define __DEBUG // enable debug

#ifdef __DEBUG
#include <memory>
#define FUNCTION_SCOPE \
auto __scope_logger__ = [](char const* f)->std::shared_ptr<const char>{ \
    CAM_ULOGMD("(%d)[%s] + ", ::gettid(), f); \
    return std::shared_ptr<const char>(f, [](char const* p){CAM_ULOGMD("(%d)[%s] -", ::gettid(), p);}); \
}(__FUNCTION__)
#else
#define FUNCTION_SCOPE
#endif


/******************************************************************************
*
******************************************************************************/
class MFNRProviderImpl : public MultiFramePlugin::IProvider
{
    typedef MultiFramePlugin::Property Property;
    typedef MultiFramePlugin::Selection Selection;
    typedef MultiFramePlugin::Request::Ptr RequestPtr;
    typedef MultiFramePlugin::RequestCallback::Ptr RequestCallbackPtr;
    typedef MultiFramePlugin::RecommendedParam RecommendedParam;

public:


    virtual const Property& property()
    {
        FUNCTION_SCOPE;
        static Property prop;
        static bool inited;
        static MINT32 mrpMode = 0;
#ifdef MFB_MAX_FRAME
        static MINT32 maxFrame = MFB_MAX_FRAME;
#else
        static MINT32 maxFrame = 8;
#endif
        if (!inited) {
            /******************************************************************
            * shout NOT set any MFNR plugin member variable in this scope!!
            ******************************************************************/
            prop.mName              = "MTK MFNR";
            prop.mFeatures          = MTK_FEATURE_MFNR;
            prop.mThumbnailTiming   = eTiming_P2;
            prop.mPriority          = ePriority_Highest;
            prop.mZsdBufferMaxNum   = maxFrame; // maximum frames requirement
            prop.mNeedRrzoBuffer    = MTRUE; // rrzo requirement for BSS
            prop.mBoost             = eBoost_CPU;
            inited                  = MTRUE;

            maxFrame = property_get_int32("vendor.debug.plugin.mfll.maxframe", prop.mZsdBufferMaxNum);
            if (maxFrame != prop.mZsdBufferMaxNum) {
                MY_LOGD("Max Frame is set to %d by adb command", maxFrame);
                prop.mZsdBufferMaxNum = maxFrame;
            }

            //Memory Reduction Plan support since MFLL_MAKE_REVISION(4, 0, 0)
            mrpMode = property_get_int32("vendor.debug.plugin.mfll.mrpmode", 0);
            if (mCapability == nullptr) {
                mCapability = IMFNRCapabilityFactory::createCapability();
            }
        }
        mMrpMode = (mrpMode < 0 || mrpMode >= MrpMode_Size)?MrpMode_BestPerformance:(MrpMode)mrpMode;
        mMaxFrame = maxFrame;

        return prop;
    };

    virtual MERROR negotiate(Selection& sel)
    {
        FUNCTION_SCOPE;
        int SelIndex = -1, SelCount = -1;
        MINT64 HidlQueryIndex = -1;
        MBOOL needUpdateStrategy = MTRUE;
        mOpenId = sel.mSensorId;
        NSCam::TuningUtils::scenariorecorder::DecisionParam decParm;

        if (mCapability == nullptr) {
            MY_LOGW("MFNR Capability is null, create new one");
            mCapability = IMFNRCapabilityFactory::createCapability();
        }

        {
            SelIndex = mCapability->getSelIndex(sel);
            SelCount = mCapability->getSelCount(sel);
            HidlQueryIndex = mCapability->getTuningIndex(sel);
        }

        MY_LOGD("Collected Selection:(%d/%d), ISP mode: %d", SelIndex, SelCount, sel.mIspHidlStage);

        // create MFNRShotInfo
        if (SelIndex == 0) {

            mZSDMode = sel.mState.mZslRequest && sel.mState.mZslPoolReady;
            mFlashOn = sel.mState.mFlashFired;
            mRealIso = sel.mState.mRealIso;
            mShutterTime = sel.mState.mExposureTime;
            mSensorMode = -1;
            // update sensor mode if 4 cell is enabled
            if (CC_LIKELY(sel.mIMetadataHal.getControl() != NULL)) {
                IMetadata* pHalMeta = sel.mIMetadataHal.getControl().get();
                MUINT8 enable = 0;
                if ( !IMetadata::getEntry<MUINT8>(pHalMeta, MTK_HAL_REQUEST_REMOSAIC_ENABLE, enable) ) {
                    MY_LOGW("%s: cannot retrieve MTK_HAL_REQUEST_REMOSAIC_ENABLE from HAL additional metadata, assume it to 0", __FUNCTION__);
                }
                if (enable)
                    mSensorMode = sel.mState.mSensorMode;
            }
            if (mRealIso <= 0)
                mRealIso = 0;
            if (mShutterTime <= 0)
                mShutterTime = 0;

            if (CC_UNLIKELY( mFlashOn && mZSDMode )) {
                MY_LOGD("do not use ZSD buffers due to Flash MFNR");
                mZSDMode = MFALSE;
            }

            {
                setUniqueKey(mCapability->getUniqueKey(sel));
                {
                    std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
                    needUpdateStrategy = mCapability->getRecommendShot(mShots, sel, HidlQueryIndex);
                }
            }
            setScenarioInfo(decParm, sel);
#ifdef MFB_SUPPORT
            MINT32 mfbSupport = property_get_int32("vendor.debug.plugin.mfb.support", MFB_SUPPORT);
            if (mfbSupport == 0) {
                MY_LOGD("MFB SUPPORT is 0, disable MFNR.");
                WRITE_DECISION_RECORD_INTO_FILE(decParm, "disable MFNR because MFB_SUPPORT is 0");
                return BAD_VALUE;
            }
#endif
            if (CC_LIKELY(sel.mIMetadataApp.getControl() != NULL)) {
                IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();
                mMfbMode = updateMfbMode(pAppMeta, mZSDMode);
            }

            if (CC_UNLIKELY( mMfbMode == MfllMode_Off )) {
                if (MfllProperty::isForceMfll() == 0)
                    WRITE_DECISION_RECORD_INTO_FILE(decParm, "disable MFNR because adb command force MFNR off");
                else
                    WRITE_DECISION_RECORD_INTO_FILE(decParm, "disable MFNR because MfbMode is MfllMode_Off");
                return BAD_VALUE;
            }

            createShotInfo(sel.mIspHidlStage, mOpenId, sel.mIMetadataApp.getControl() != NULL ? sel.mIMetadataApp.getControl().get() : NULL);
#ifdef __DEBUG
            dumpShotInfo();
#endif

            sel.mDecision.mZslEnabled = isZsdMode(mMfbMode);
            sel.mDecision.mZslPolicy.mPolicy = ZslPolicy::AFStable
                                             | ZslPolicy::AESoftStable
                                             | ZslPolicy::ContinuousFrame
                                             | ZslPolicy::ZeroShutterDelay;
            sel.mDecision.mZslPolicy.mTimeouts = 1000;
        }

        //get MFNRShotInfo
        std::shared_ptr<MFNRShotInfo> pShotInfo = getShotInfo(getUniqueKey());
        if (CC_UNLIKELY(pShotInfo.get() == nullptr)) {
            removeShotInfo(getUniqueKey());
            MY_LOGE("get MFNRShotInfo instance failed! cannot apply MFNR shot.");
            return BAD_VALUE;
        }

        //update zsl timeout value from 3a
        if (SelIndex == 0) {
            sel.mDecision.mZslPolicy.mTimeouts = pShotInfo->getZslTimeoutMs();
        }

        //update policy
        if (SelIndex == 0) {
            {
                mCapability->setSrcSize(pShotInfo, sel);
            }

            {
                if (!mCapability->updateMultiCamInfo(pShotInfo, sel)) {
                    removeShotInfo(getUniqueKey());
                    MY_LOGE("MFNR Provider is not support for mfnrcore(%s)", pShotInfo->getMfnrCoreVersionString().c_str());
                    return BAD_VALUE;
                }
            }

            pShotInfo->setMaxFrameNum(mMaxFrame);
            if (needUpdateStrategy) {
                pShotInfo->setSensorMode(mSensorMode);
                pShotInfo->updateMfllStrategy();
            }

            MY_LOGD("realIso = %d, shutterTime = %d, finalRealIso = %d, finalShutterTime = %d"
                , pShotInfo->getRealIso()
                , pShotInfo->getShutterTime()
                , pShotInfo->getFinalIso()
                , pShotInfo->getFinalShutterTime());
            mRecommendedKey = mCapability->generateRecommendedKey(pShotInfo->getBssNvramIndex());
        }

        SelCount = mCapability->updateSelection(pShotInfo, sel);

        if (!mCapability->getEnableMfnr(pShotInfo, sel)
            || SelCount == 0
            || !mCapability->updateInputBufferInfo(pShotInfo, sel)) {
            WRITE_DECISION_RECORD_INTO_FILE(decParm, "disable MFNR because iso:%d < threshold", pShotInfo->getFinalIso());
            removeShotInfo(getUniqueKey());
            return BAD_VALUE;
        }

        mYuvAlign = mCapability->getYUVAlign();
        mQYuvAlign = mCapability->getQYUVAlign();
        mMYuvAlign = mCapability->getMYUVAlign();

        sel.mIBufferFull.setRequired(MTRUE).setAlignment(mYuvAlign.w, mYuvAlign.h);
        sel.mIBufferSpecified.setRequired(MTRUE).setAlignment(mQYuvAlign.w, mQYuvAlign.h);
        sel.mIMetadataDynamic.setRequired(MTRUE);
        sel.mIMetadataApp.setRequired(MTRUE);
        sel.mIMetadataHal.setRequired(MTRUE);


        // Without control metadata, it's no need to append additional metadata
        // Use default mfnr setting
        if (sel.mIMetadataApp.getControl() != NULL) {
            //per frame
            {
                MetadataPtr pAppAddtional = make_shared<IMetadata>();
                MetadataPtr pHalAddtional = make_shared<IMetadata>();

                IMetadata* pAppMeta = pAppAddtional.get();
                IMetadata* pHalMeta = pHalAddtional.get();

                // clone partial control metadata to additional metadata
                {
                    IMetadata* pAppMetaControl = sel.mIMetadataApp.getControl().get();
                    // flash mode
                    {
                        MUINT8 flashMode = MTK_FLASH_MODE_OFF;
                        if ( !IMetadata::getEntry<MUINT8>(pAppMetaControl, MTK_FLASH_MODE, flashMode) ) {
                            MY_LOGW("%s: cannot retrieve MTK_FLASH_MODE from APP control metadata, assume "\
                                    "it to MTK_FLASH_MODE_OFF", __FUNCTION__);
                        }
                        IMetadata::setEntry<MUINT8>(pAppMeta, MTK_FLASH_MODE, flashMode);
                    }
                }

                // update unique key
                IMetadata::setEntry<MINT32>(pHalMeta, MTK_PIPELINE_UNIQUE_KEY, getUniqueKey());
                IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_MFNR_NVRAM_DECISION_ISO, pShotInfo->getFinalIso());

                {
                    mCapability->updateInputMeta(pShotInfo, sel, pHalMeta, mRecommendedKey);
                }

                IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_MFNR_FINAL_EXP, pShotInfo->getFinalShutterTime());
                IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_MFNR_OPEN_ID, mOpenId);
                if (mCapability->isNeedDCESO(pShotInfo) == MTRUE)
                    IMetadata::setEntry<MUINT8>(pHalMeta, MTK_FEATURE_CAP_PIPE_DCE_CONTROL, SelIndex?MTK_FEATURE_CAP_PIPE_DCE_MANUAL_DISABLE:MTK_FEATURE_CAP_PIPE_DCE_ENABLE_BUT_NOT_APPLY);
                updatePerFrameMetadata(pShotInfo.get(), pAppMeta, pHalMeta, (SelIndex+1 == SelCount));

                sel.mIMetadataApp.setAddtional(pAppAddtional);
                sel.mIMetadataHal.setAddtional(pHalAddtional);
            }

            //dummy frame
            {
                MetadataPtr pAppDummy = make_shared<IMetadata>();
                MetadataPtr pHalDummy = make_shared<IMetadata>();

                IMetadata* pHalMeta = pHalDummy.get();

                //first frame
                if (SelIndex == 0) {
                    sel.mFrontDummy = pShotInfo->getDummyFrameNum();
                    if (pShotInfo->getIsFlashOn()) {
                        IMetadata::setEntry<MBOOL>(pHalMeta, MTK_3A_DUMMY_BEFORE_REQUEST_FRAME, 1);
                    }
                    // need pure raw for MFNR flow
                    IMetadata::setEntry<MINT32>(pHalMeta, MTK_P1NODE_RAW_TYPE, 1);
                }

                //last frame
                if (SelIndex+1 == SelCount) {
                    sel.mPostDummy = pShotInfo->getDelayFrameNum();
                    IMetadata::setEntry<MBOOL>(pHalMeta, MTK_3A_DUMMY_AFTER_REQUEST_FRAME, 1);
                    IMetadata::setEntry<MINT32>(pHalMeta, MTK_P1NODE_RAW_TYPE, 1);
                }

                sel.mIMetadataApp.setDummy(pAppDummy);
                sel.mIMetadataHal.setDummy(pHalDummy);
            }
        }

        if (SelIndex == 0) {
            writeScenarioLog(decParm, pShotInfo);

            if (!mCapability->updateOutputBufferInfo(sel)) {
                removeShotInfo(getUniqueKey());
                MY_LOGE("MFNR Provider is not support for mfnrcore(%s)", pShotInfo->getMfnrCoreVersionString().c_str());
                return BAD_VALUE;
            }

            sel.mOMetadataApp.setRequired(MTRUE);
            sel.mOMetadataHal.setRequired(MTRUE);

        } else {
            sel.mOBufferFull.setRequired(MFALSE);
            sel.mOMetadataApp.setRequired(MFALSE);
            sel.mOMetadataHal.setRequired(MFALSE);
        }

        if (SelIndex+1 == SelCount) {
            bool isPublish = false;
            {
                std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
                isPublish = mCapability->publishShotInfo(pShotInfo, sel, mRecommendedKey);
            }
            if (isPublish) {
                removeShotInfo(getUniqueKey());
            }

#ifdef __DEBUG
            dumpShotInfo();
#endif
        }

        return OK;
    };

    virtual void init()
    {
        FUNCTION_SCOPE;
        //nothing to do for MFNR
    };

    virtual MERROR queryRecommendedFrameSetting(RecommendedParam& param)
    {
        FUNCTION_SCOPE;

        if (mCapability == nullptr) {
            mCapability = IMFNRCapabilityFactory::createCapability();
        }
#ifdef ISP_HIDL_TEMP_SOL_WITHOUT_TUNING_INDEX_HINT
        MY_LOGW("%s: this function is not support due to temp solution is work.", __FUNCTION__);
#endif

        MINT32 hint = 0;
        if (!IMetadata::getEntry<MINT32>(param.pAppMetadata, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, hint)) {
            MY_LOGE("cannot get MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING");
            return android::BAD_VALUE;
        }
        if (hint != MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_MFNR) {
            MY_LOGD("tining hint is NOT MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_MFNR");
            return android::BAD_VALUE;
        }

        MUINT8 AEmode = MTK_CONTROL_AE_MODE_OFF;
        MINT32 Sensor_Sensitivity = 0;
        MINT64 Sensor_Exposure_Time = 0;
        MINT32 Cam_ID = 0;
        if (!IMetadata::getEntry<MUINT8>(param.pAppMetadata, MTK_CONTROL_AE_MODE, AEmode)) {
            MY_LOGW("cannot get MTK_CONTROL_AE_MODE, use default mode off");
        } else {
            MY_LOGD("MTK_CONTROL_AE_MODE: %d", AEmode);
        }

        if (!IMetadata::getEntry<MINT32>(param.pAppMetadata, MTK_SENSOR_SENSITIVITY, Sensor_Sensitivity)) {
            MY_LOGW("cannot get MTK_SENSOR_SENSITIVITY");
        } else {
            MY_LOGD("MTK_SENSOR_SENSITIVITY: %d", Sensor_Sensitivity);
        }

        if (!IMetadata::getEntry<MINT64>(param.pAppMetadata, MTK_SENSOR_EXPOSURE_TIME, Sensor_Exposure_Time)) {
            MY_LOGW("cannot get MTK_SENSOR_EXPOSURE_TIME");
        } else {
            MY_LOGD("MTK_SENSOR_EXPOSURE_TIME: %" PRIi64 "", Sensor_Exposure_Time);
        }

        if (!IMetadata::getEntry<MINT32>(param.pAppMetadata, MTK_MULTI_CAM_STREAMING_ID, Cam_ID)) {
            MY_LOGW("cannot get MTK_MULTI_CAM_STREAMING_ID");
        } else {
            MY_LOGD("MTK_MULTI_CAM_STREAMING_ID: %d", Cam_ID);
        }

        std::shared_ptr<MFNRShotInfo> pShotInfo;
        {
            std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
            pShotInfo = std::make_shared<MFNRShotInfo>(static_cast<MINT32>(NSCam::Utils::TimeTool::getReadableTime()), Cam_ID,
                MfllMode_NormalMfll, Sensor_Sensitivity, Sensor_Exposure_Time, (AEmode == MTK_CONTROL_AE_MODE_ON_ALWAYS_FLASH), const_cast<IMetadata*>(param.pAppMetadata));
            if (CC_UNLIKELY(pShotInfo.get() == nullptr)) {
                MY_LOGE("get MFNRShotInfo instance failed! cannot process MFNR shot.");
                return BAD_VALUE;
            }
        }
        pShotInfo->setMaxFrameNum(mMaxFrame);
        pShotInfo->updateMfllStrategy();
        int CaptureNum = pShotInfo->getCaptureNum();
        MY_LOGD("CaptureNum: %d", CaptureNum);
        if (CaptureNum <= 1) {
            MY_LOGD("Do Not recommend to do MFNR");
            return BAD_VALUE;
        }

        MINT64 recommendKey = mCapability->generateRecommendedKey(pShotInfo->getBssNvramIndex());
        //store shot instance to mRecommendQueue
        {
            std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);

            mCapability->publishShotInfo(pShotInfo, recommendKey);
        }

        for(int i = 0; i < CaptureNum; i++) {
            std::shared_ptr<IMetadata> pAppAddtional = make_shared<IMetadata>();
            IMetadata* pAppMeta = pAppAddtional.get();
            IMetadata::setEntry<MINT32>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX, i);
            IMetadata::setEntry<MINT32>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX, CaptureNum);
            IMetadata::setEntry<MINT32>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_MFNR);
            IMetadata::setEntry<MINT64>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX, recommendKey);
            param.framesSetting.push_back(pAppAddtional);
        }
        MY_LOGD("RecommendedParam.framesSetting size: %zu, tuning index :%d", param.framesSetting.size(), pShotInfo->getBssNvramIndex());

        return OK;
    }

    virtual MERROR process(RequestPtr pRequest, RequestCallbackPtr pCallback)
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mProcessMx)> lk(mProcessMx);

        // restore callback function for abort API
        if (pCallback != nullptr) {
           m_callbackprt = pCallback;
        }

        //maybe need to keep a copy in member<sp>
        IMetadata* pAppMeta = pRequest->mIMetadataApp->acquire();
        IMetadata* pHalMeta = pRequest->mIMetadataHal->acquire();
        IMetadata* pHalMetaDynamic = pRequest->mIMetadataDynamic->acquire();
        MINT32 processUniqueKey = 0;
        MINT32 ISP_TuningHint = MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_DEFAULT_NONE;

#if (SUPPORT_YUV_BSS==0)
        MINT8   RequestIndex = pRequest->mRequestBSSIndex;
        MINT8   RequestCount = pRequest->mRequestBSSCount;
#else
        MUINT8  RequestIndex = pRequest->mRequestIndex;
        MUINT8  RequestCount = pRequest->mRequestCount;
#endif

        // clean to zero
        IMetadata::setEntry<MUINT8>(pHalMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, 0);

        if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_PIPELINE_UNIQUE_KEY, processUniqueKey)) {
            MY_LOGE("cannot get unique about MFNR capture");
            return BAD_VALUE;
        }

        if (!IMetadata::getEntry<MINT32>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, ISP_TuningHint)) {
            MY_LOGW("cannot get capture hint for ISP tuning");
        }

        //get MFNRShotInfo
        std::shared_ptr<MFNRShotInfo> pShotInfo = getShotInfo(processUniqueKey);
        if (CC_UNLIKELY(pShotInfo.get() == nullptr)) {
            removeShotInfo(processUniqueKey);
            MY_LOGE("get MFNRShotInfo instance failed! cannot process MFNR shot.");
            return BAD_VALUE;
        }

#if 1
        if (RequestIndex == 0) {
            MY_LOGE("TODO: please fix it. <initMfnrCore timing>");
            {
                std::lock_guard<std::mutex> __l(m_futureExeMx);
                auto t1 = std::async(std::launch::async, [this, pShotInfo, ISP_TuningHint]() {
                        MFLL_THREAD_NAME("initMfnrCore");
                        pShotInfo->setIspTuningHint(ISP_TuningHint);
                        pShotInfo->setMrpMode(mMrpMode);
                        pShotInfo->initMfnrCore(); // init MFNR controller
                    });
                m_futureExe = std::shared_future<void>(std::move(t1));
            }
        }
#endif

        //wait initMfnrCore() done
        if (RequestIndex == 0) {
            pShotInfo->setRealBlendNum(RequestCount);
            pShotInfo->setMainFrameHalMetaIn(pRequest->mIMetadataHal->acquire());
            pShotInfo->setMainFrameHalMetaOut(pRequest->mOMetadataHal->acquire());
            std::shared_future<void> t1;
            {
                std::lock_guard<std::mutex> __l(m_futureExeMx);
                t1 = m_futureExe;
            }

            if (t1.valid())
                t1.wait();

            pShotInfo->execute();
        }

        /* create IMfllImageBuffer of Full/ Quarter YUV */
        sp<IMfllImageBuffer> mfllImgBuf_yuv_full = IMfllImageBuffer::createInstance("FullYuv");
        sp<IMfllImageBuffer> mfllImgBuf_yuv_quarter = IMfllImageBuffer::createInstance("QuarterYuv");
        sp<IMfllImageBuffer> mfllImgBuf_yuv_mss = IMfllImageBuffer::createInstance("MssYuv");
        sp<IMfllImageBuffer> mfllImgBuf_dceso = IMfllImageBuffer::createInstance("Dceso");
        sp<IMfllImageBuffer> mfllMixWorkingBuf = IMfllImageBuffer::createInstance("MixWorking");

        if (pRequest->mIBufferFull != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mIBufferFull->acquire();
            MY_LOGD("[IN] Full image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
            mfllImgBuf_yuv_full->setImageBuffer(pImgBuffer);
            mfllImgBuf_yuv_full->setAligned(mYuvAlign.w, mYuvAlign.h);
        }
        if (pRequest->mIBufferSpecified != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mIBufferSpecified->acquire();
            MY_LOGD("[IN] Quarter image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
            mfllImgBuf_yuv_quarter->setImageBuffer(pImgBuffer);
            mfllImgBuf_yuv_quarter->setAligned(mQYuvAlign.w, mQYuvAlign.h);
        }
/*TO-DO: apply real buffer frome p2a */
#if 1
        if (pRequest->mIBufferResized != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mIBufferResized->acquire();
            MY_LOGD("[IN] Mss image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
            mfllImgBuf_yuv_mss->setImageBuffer(pImgBuffer);
            mfllImgBuf_yuv_mss->setAligned(mMYuvAlign.w, mMYuvAlign.h);
#else
        if (pRequest->mRequestBSSIndex == 0) {
            mfllImgBuf_yuv_mss->setImageFormat(ImageFormat_YUV_P012);
            mfllImgBuf_yuv_mss->setResolution((pShotInfo->getSizeSrc().w+3)/4*2, (pShotInfo->getSizeSrc().h+3)/4*2);
            mfllImgBuf_yuv_mss->setAligned(mMYuvAlign.w, mMYuvAlign.h);
            mfllImgBuf_yuv_mss->initBuffer();
            IImageBuffer* pImgBuffer = (IImageBuffer*)mfllImgBuf_yuv_mss->getImageBuffer();
            MY_LOGD("[IN] Mss image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
#endif
        }
        /*TO-DO: apply real buffer frome p2a */
#if 1
        if (pRequest->mIBufferDCES  != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mIBufferDCES ->acquire();
            MY_LOGD("[IN] DCESO image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
            mfllImgBuf_dceso->setImageBuffer(pImgBuffer);
            mfllImgBuf_dceso->setAligned(mMYuvAlign.w, mMYuvAlign.h);
#else
        if (pRequest->mRequestBSSIndex == 0) {
            mfllImgBuf_dceso->setImageFormat(ImageFormat_Sta32);
            mfllImgBuf_dceso->setResolution(2560, 1);
            mfllImgBuf_dceso->setAligned(1, 1);
            mfllImgBuf_dceso->initBuffer();
            IImageBuffer* pImgBuffer = (IImageBuffer*)mfllImgBuf_dceso->getImageBuffer();
            MY_LOGD("[IN] DCESO image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
#endif
        }
        if (pRequest->mOBufferFull != nullptr) {
            IImageBuffer* pImgBuffer = pRequest->mOBufferFull->acquire();
            MY_LOGD("[OUT] Full image VA: 0x%p, Size(%dx%d)", (void*)pImgBuffer->getBufVA(0), pImgBuffer->getImgSize().w, pImgBuffer->getImgSize().h);
            mfllMixWorkingBuf->setImageBuffer(pImgBuffer);
            pShotInfo->setOutputBufToMfnrCore(mfllMixWorkingBuf);
        }

        if (pRequest->mIMetadataDynamic != nullptr) {
            IMetadata *meta = pRequest->mIMetadataDynamic->acquire();
            if (meta != NULL)
                MY_LOGD("[IN] Dynamic metadata count: %" PRId32 "", meta->count());
            else
                MY_LOGD("[IN] Dynamic metadata Empty");
        }

        MY_LOGD("collected request(%d/%d)",
                RequestIndex,
                RequestCount);

        if (CC_UNLIKELY( mvRequests.size() != RequestIndex ))
            MY_LOGE("Input sequence of requests from P2A is wrong");

        mvRequests.push_back(pRequest);

        MfllMotionVector mv = pShotInfo->calMotionVector(pHalMeta, RequestIndex);
        pShotInfo->addDataToMfnrCore(mfllImgBuf_yuv_full, mfllImgBuf_yuv_quarter, mfllImgBuf_yuv_mss, mfllImgBuf_dceso, mv, pAppMeta, pHalMeta, pHalMetaDynamic, RequestIndex);

        if (RequestIndex == RequestCount - 1)
        {
            pShotInfo->waitExecution();
            MY_LOGD("have collected all requests");
            for (size_t idx = 0; idx < mvRequests.size() ; idx++) {
                auto req = mvRequests.editItemAt(idx);
                if (req == nullptr) {
                    MY_LOGE("req in mvRequests is nullptr!!");
                    continue;
                }
                MY_LOGD("callback request(%d/%d) %p",
#if (SUPPORT_YUV_BSS==0)
                        req->mRequestBSSIndex,
                        req->mRequestBSSCount,
#else
                        req->mRequestIndex,
                        req->mRequestCount,
#endif
                        pCallback.get());
                if (pCallback != nullptr) {
                    pCallback->onCompleted(req, 0);
                }
            }
            mvRequests.clear();
            removeShotInfo(processUniqueKey);
#ifdef __DEBUG
            dumpShotInfo();
#endif
        }

        return 0;
    };

    virtual void abort(vector<RequestPtr>& pRequests)
    {
        FUNCTION_SCOPE;

#if (SUPPORT_YUV_BSS==0)
        MY_LOGD("not support abort() for RAW BSS");
#else
        std::lock_guard<decltype(mProcessMx)> lk(mProcessMx);

        bool bAbort = false;
        IMetadata *pHalMeta;
        MINT32 processUniqueKey = 0;
        std::shared_ptr<MFNRShotInfo> pShotInfo;

        for(auto req:pRequests) {
            bAbort = false;
            pHalMeta = req->mIMetadataHal->acquire();
            if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_PIPELINE_UNIQUE_KEY, processUniqueKey)) {
                MY_LOGW("cannot get unique about MFNR capture");
            }
            pShotInfo = getShotInfo(processUniqueKey);

            /* if MFNR plugin receives abort, it will cancel mfll immediately */
            if (pShotInfo != nullptr) {
               if (!pShotInfo->getDoCancelStatus()) {
                    pShotInfo->doCancel();
                    pShotInfo->waitExecution(); //wait mfll done
                    pShotInfo->setDoCancelStatus(true);
               }
            }else{
               MY_LOGW("pShotInfo is null");
            }

            if (m_callbackprt != nullptr) {
               MY_LOGD("m_callbackprt is %p", m_callbackprt.get());
               /*MFNR plugin callback request to MultiFrameNode */
               for (Vector<RequestPtr>::iterator it = mvRequests.begin() ; it != mvRequests.end(); it++) {
                    if ((*it) == req) {
                        mvRequests.erase(it);
                        m_callbackprt->onAborted(req);
                        bAbort = true;
                        break;
                    }
               }
            }else{
               MY_LOGW("callbackptr is null");
            }

            if (!bAbort) {
               MY_LOGW("Desire abort request[%d] is not found", req->mRequestIndex);
            }

            if (mvRequests.empty()) {
               removeShotInfo(processUniqueKey);
               MY_LOGD("abort() cleans all the requests");
            }else {
               MY_LOGW("abort() does not clean all the requests");
            }
        }
#endif
    };

    virtual void uninit()
    {
        FUNCTION_SCOPE;
        //nothing to do for MFNR
    };

    virtual ~MFNRProviderImpl()
    {
        FUNCTION_SCOPE;
        removeAllShotInfo();
        mCapability = nullptr;
#ifdef __DEBUG
        dumpShotInfo();
#endif
    };

private:
    MINT32 getUniqueKey()
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
        return mUniqueKey;
    }

    void setUniqueKey(MINT32 key)
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
        mUniqueKey = key;
    }

    void createShotInfo(IspHidlStage HidlStage, MINT32 sensorId, IMetadata* appMetaIn)
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);

        if (HidlStage != ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL || mShots[mUniqueKey] == nullptr)
            mShots[mUniqueKey] = std::make_shared<MFNRShotInfo>(mUniqueKey, sensorId, mMfbMode, mRealIso, mShutterTime, mFlashOn, appMetaIn);

        MY_LOGD("Create ShotInfos: %d sensorId: %d", mUniqueKey, sensorId);
    };

    void removeShotInfo(MINT32 key)
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
        if (mShots.count(key)) {
            mShots[key] = nullptr;
            mShots.erase(key);

            MY_LOGD("Remvoe ShotInfos: %d", key);
        }
    };

    void removeAllShotInfo()
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
        for (auto it = mShots.begin(); it != mShots.end(); ++it) {
            it->second = nullptr;
            MY_LOGD("Remvoe ShotInfos: %d", it->first);
        }
        mShots.clear();
    };

    std::shared_ptr<MFNRShotInfo> getShotInfo(MINT32 key)
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);
        return mShots[key];
    };

    void dumpShotInfo()
    {
        FUNCTION_SCOPE;

        std::lock_guard<decltype(mShotInfoMx)> lk(mShotInfoMx);

        std::string usage;
        for ( auto it = mShots.begin(); it != mShots.end(); ++it )
            usage += " " + std::to_string(it->first);

        MY_LOGD("All ShotInfos:%s", usage.c_str());
    };

    void updatePerFrameMetadata(const MFNRShotInfo* pShotInfo, IMetadata* pAppMeta, IMetadata* pHalMeta, bool bLastFrame)
    {
        FUNCTION_SCOPE;

        ASSERT(pShotInfo, "updatePerFrameMetadata::pShotInfo is null.");
        ASSERT(pAppMeta,  "updatePerFrameMetadata::pAppMeta is null.");
        ASSERT(pHalMeta,  "updatePerFrameMetadata::pHalMeta is null.");

        // update_app_setting(pAppMeta, pShotInfo);
        {
            bool bNeedManualAe = [&](){
                // if using ZHDR, cannot apply manual AE
                if (mfll::isZhdrMode(pShotInfo->getMfbMode()))
                    return false;
                // if using FLASH, cannot apply menual AE
                if (pShotInfo->getIsFlashOn())
                    return false;
                // if uisng MFNR (since MFNR uses ZSD buffers), we don't need manual AE
                // but if MFNR using non-ZSD flow, we need to apply manual AE
                if (mfll::isMfllMode(pShotInfo->getMfbMode())) {
                    if (mfll::isZsdMode(pShotInfo->getMfbMode()))
                        return false;
                    else
                        return true;
                }
                /// otherwise, we need it
                return true;
            }();

            if (bNeedManualAe) {
                //manualAE cannot apply flash
                {
                    MUINT8 flashMode = MTK_FLASH_MODE_OFF;
                    if ( !IMetadata::getEntry<MUINT8>(pAppMeta, MTK_FLASH_MODE, flashMode) ) {
                        MY_LOGW("%s: cannot retrieve MTK_FLASH_MODE from APP additional metadata, assume "\
                                "it to MTK_FLASH_MODE_OFF", __FUNCTION__);
                    }
                    if (flashMode != MTK_FLASH_MODE_TORCH)
                        IMetadata::setEntry<MUINT8>(pAppMeta, MTK_FLASH_MODE, MTK_FLASH_MODE_OFF);
                }
                //
                IMetadata::setEntry<MUINT8>(pAppMeta, MTK_CONTROL_AE_MODE, MTK_CONTROL_AE_MODE_OFF);
                IMetadata::setEntry<MINT32>(pAppMeta, MTK_SENSOR_SENSITIVITY, pShotInfo->getFinalIso());
                IMetadata::setEntry<MINT64>(pAppMeta, MTK_SENSOR_EXPOSURE_TIME, pShotInfo->getFinalShutterTime() * 1000); // ms->us
                IMetadata::setEntry<MUINT8>(pAppMeta, MTK_CONTROL_AWB_LOCK, MTRUE);
            }

            IMetadata::setEntry<MBOOL>(pHalMeta, MTK_P1NODE_ISNEED_GMV, MTRUE);
        }

        {
            MUINT8 bOriFocusPause  = 0;
            if ( !IMetadata::getEntry<MUINT8>(pHalMeta, MTK_FOCUS_PAUSE, bOriFocusPause) ) {
                MY_LOGW("%s: cannot retrieve MTK_FOCUS_PAUSE from HAL metadata, assume "\
                        "it to 0", __FUNCTION__);
            }

            // update ISP profile for zHDR (single frame default)
            if (mfll::isZhdrMode(pShotInfo->getMfbMode())) {
                MY_LOGE("Zhdr is not support in MFNR plugin");
                bool isAutoHDR = mfll::isAutoHdr(pShotInfo->getMfbMode());
                MUINT sensorMode = SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
                MSize sensorSize;
                NSCamHW::HwInfoHelper helper(pShotInfo->getOpenId());
                if (!helper.getSensorSize(sensorMode, sensorSize)) {
                    MY_LOGW("cannot get sensor size");
                }
                else {
                    // Prepare query Feature Shot ISP Profile
                    ProfileParam profileParam
                    {
                        sensorSize,
                        SENSOR_VHDR_MODE_ZVHDR, /*VHDR mode*/
                        sensorMode,
                        ProfileParam::FLAG_NONE,
                        ((isAutoHDR) ? (ProfileParam::FMASK_AUTO_HDR_ON) : (ProfileParam::FMASK_NONE)),
                    };

                    MUINT8 profile = 0;
                    if (FeatureProfileHelper::getShotProf(profile, profileParam))
                    {
                        MY_LOGD("ISP profile is set(%u)", profile);
                        // modify hal control metadata for zHDR
                        IMetadata::setEntry<MUINT8>(
                            pHalMeta , MTK_3A_ISP_PROFILE , profile);
                        IMetadata::setEntry<MUINT8>(
                            pHalMeta, MTK_3A_AE_CAP_SINGLE_FRAME_HDR, 1);
                    }
                    else
                    {
                        MY_LOGW("ISP profile is not set(%u)", profile);
                    }
                }
            }

            // pause AF for (N - 1) frames and resume for the last frame
            IMetadata::setEntry<MUINT8>(pHalMeta, MTK_FOCUS_PAUSE, bLastFrame ? bOriFocusPause : 1);
            IMetadata::setEntry<MUINT8>(pHalMeta, MTK_HAL_REQUEST_REQUIRE_EXIF, 1);
            IMetadata::setEntry<MUINT8>(pHalMeta, MTK_HAL_REQUEST_DUMP_EXIF, 1);

            // need pure raw for MFNR flow
            IMetadata::setEntry<MINT32>(pHalMeta, MTK_P1NODE_RAW_TYPE, 1);

            MINT32 customHintInMFNR = mfll::getCustomHint(pShotInfo->getMfbMode());
            MINT32 customHintInHal;
            // check customHint in metadata for customize feature
            if ( !IMetadata::getEntry<MINT32>(pHalMeta, MTK_PLUGIN_CUSTOM_HINT, customHintInHal) ) {
                MY_LOGW("%s: cannot retrieve MTK_PLUGIN_CUSTOM_HINT from HAL metadata, assume "\
                        "it to %d", __FUNCTION__, customHintInMFNR);
                IMetadata::setEntry<MINT32>( pHalMeta, MTK_PLUGIN_CUSTOM_HINT, customHintInMFNR);
            }
            else if (customHintInMFNR != customHintInHal) { // query and check the result
                MY_LOGW("%s: MTK_PLUGIN_CUSTOM_HINT in MFNR(%d) and Hal(%d) setting are different"
                        , __FUNCTION__, customHintInMFNR, customHintInHal);
            }
        }

        //AE Flare Enable, due to MTK_CONTROL_AE_MODE_OFF will disable AE_Flare
        // MFNR must set MTK_3A_FLARE_IN_MANUAL_CTRL_ENABLE to enable AE_Flare
        // TODO: hal3a need to implement this command for each platform.
        {
            IMetadata::setEntry<MBOOL>(pHalMeta, MTK_3A_FLARE_IN_MANUAL_CTRL_ENABLE, MTRUE);
        }
    }

    MfllMode updateMfbMode(IMetadata* pAppMeta, MBOOL isZSDMode) {

        IMetadata::IEntry const eIspMFNR       = pAppMeta->entryFor(MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING);
        IMetadata::IEntry const eMfb           = pAppMeta->entryFor(MTK_MFNR_FEATURE_MFB_MODE);
        IMetadata::IEntry const eAis           = pAppMeta->entryFor(MTK_MFNR_FEATURE_AIS_MODE);
        IMetadata::IEntry const eSceneMode     = pAppMeta->entryFor(MTK_CONTROL_SCENE_MODE);
        if (!eIspMFNR.isEmpty()) {
            MY_LOGD("hint for ISP tuning mode: %d", eIspMFNR.itemAt(0, Type2Type<MINT32>()));
        } else {
            MY_LOGD("MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING is empty");
        }

        int mfbMode = [&]()
        {
            if ( ! eIspMFNR.isEmpty() && eIspMFNR.itemAt(0, Type2Type<MINT32>()) != MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_MFNR) {
                MY_LOGD("HINT_FOR_ISP_TUNING has value but is NOT MFNR, set mfllMode = MfllMode_Off");
                return MTK_MFB_MODE_OFF;
            }
            // If MTK specific parameter AIS on or MFB mode is AIS, set to AIS mode (2)
            else if (( ! eMfb.isEmpty() && eMfb.itemAt(0, Type2Type<MINT32>()) == MTK_MFNR_FEATURE_MFB_AIS)  ||
                     ( ! eAis.isEmpty() && eAis.itemAt(0, Type2Type<MINT32>()) == MTK_MFNR_FEATURE_AIS_ON)) {
                return MTK_MFB_MODE_AIS;
            }
            // Scene mode is Night or MFB mode is MFLL, set to MFLL mode (1)
            else if (( ! eMfb.isEmpty() && eMfb.itemAt(0, Type2Type<MINT32>()) == MTK_MFNR_FEATURE_MFB_MFLL) ||
                     ( ! eSceneMode.isEmpty() && eSceneMode.itemAt(0, Type2Type<MUINT8>()) == MTK_CONTROL_SCENE_MODE_NIGHT)) {
                return MTK_MFB_MODE_MFLL;
            }
            else if (( ! eMfb.isEmpty() && eMfb.itemAt(0, Type2Type<MINT32>()) == MTK_MFNR_FEATURE_MFB_AUTO)) {
                MY_LOGD("in MTK_MFNR_FEATURE_MFB_AUTO");
#ifdef CUST_MFLL_AUTO_MODE
                static_assert( ((CUST_MFLL_AUTO_MODE >= MTK_MFB_MODE_OFF)&&(CUST_MFLL_AUTO_MODE < MTK_MFB_MODE_NUM)),
                               "CUST_MFLL_AUTO_MODE is invalid in custom/feature/mfnr/camera_custom_mfll.h" );

                MY_LOGD("CUST_MFLL_AUTO_MODE:%d", CUST_MFLL_AUTO_MODE);
                return static_cast<mtk_platform_metadata_enum_mfb_mode>(CUST_MFLL_AUTO_MODE);
#else
#error "CUST_MFLL_AUTO_MODE is no defined in custom/feature/mfnr/camera_custom_mfll.h"
#endif
            }
            else if ( ! eIspMFNR.isEmpty() && eIspMFNR.itemAt(0, Type2Type<MINT32>()) == MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_MFNR) {
                MY_LOGD("return MTK_MFB_MODE_MFLL");
                return MTK_MFB_MODE_MFLL;
            }
            // Otherwise, set MFB off (0)
            else {
                if ( ! eIspMFNR.isEmpty())
                    MY_LOGD("eIspMFNRis not empty, value:%d", eIspMFNR.itemAt(0, Type2Type<MINT32>()));
                return MTK_MFB_MODE_OFF;
            }
        }();

        MY_LOGD("mfbMode value:%d", mfbMode);

        int bForceMfb = MfllProperty::isForceMfll();
        if (CC_UNLIKELY( bForceMfb == 0 )) {
            MY_LOGD("Force disable MFNR");
            mfbMode = MTK_MFB_MODE_OFF;
        }
        else if (CC_UNLIKELY(bForceMfb > 0)) {
            MY_LOGD("Force MFNR (bForceMfb:%d)", bForceMfb);
            mfbMode = bForceMfb;
        }

        MfllMode mfllMode = MfllMode_NormalMfll;

        // 0: Not specific, 1: MFNR, 2: AIS
        switch (mfbMode) {
            case MTK_MFB_MODE_MFLL:
                mfllMode = (CC_LIKELY(isZSDMode) ? MfllMode_ZsdMfll : MfllMode_NormalMfll);
                break;
            case MTK_MFB_MODE_AIS:
                mfllMode = MfllMode_NormalAis;//(CC_LIKELY(isZSDMode) ? MfllMode_ZsdAis : MfllMode_NormalAis);
                break;
            case MTK_MFB_MODE_OFF:
                mfllMode = MfllMode_Off;
                break;
            default:
                mfllMode = (CC_LIKELY(isZSDMode) ? MfllMode_ZsdMfll : MfllMode_NormalMfll);
                break;
        }

        MY_LOGD("MfllMode(0x%X), mfbMode(%d), isZsd(%d)",
                mfllMode, mfbMode, isZSDMode);

        return mfllMode;
    }

    void setScenarioInfo(NSCam::TuningUtils::scenariorecorder::DecisionParam& decParm, Selection& sel)
    {
        NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo &dbgNumInfo = decParm.dbgNumInfo;

        MY_LOGD("uniquekey: %d", getUniqueKey());
        MINT32 uniqueKey = -1, requestNum = -1, magicNum = -1;

        if (sel.mIMetadataHal.getControl() != NULL) {
            IMetadata* pHalMeta = sel.mIMetadataHal.getControl().get();

            if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
                MY_LOGW("cannot get MTK_PIPELINE_UNIQUE_KEY");
            } else {
                MY_LOGD("get MTK_PIPELINE_UNIQUE_KEY: %d", uniqueKey);
            }

            if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_PIPELINE_REQUEST_NUMBER, requestNum)) {
                MY_LOGW("cannot get MTK_PIPELINE_REQUEST_NUMBER");
            } else {
                MY_LOGD("get MTK_PIPELINE_REQUEST_NUMBER: %d", requestNum);
            }

            if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum)) {
                MY_LOGW("cannot get MTK_P1NODE_PROCESSOR_MAGICNUM");
            } else {
                MY_LOGD("get MTK_P1NODE_PROCESSOR_MAGICNUM: %d", magicNum);
            }
        } else {
                MY_LOGE("Error: mIMetadataHal is NULL!!");
        }

        dbgNumInfo.uniquekey = getUniqueKey();
        dbgNumInfo.reqNum = requestNum;
        dbgNumInfo.magicNum = magicNum;
        decParm.decisionType = NSCam::TuningUtils::scenariorecorder::DECISION_FEATURE;
        decParm.moduleId = NSCam::Utils::ULog::MOD_LIB_MFNR;
        decParm.sensorId = sel.mSensorId;
        decParm.isCapture = true;
    }

    void writeScenarioLog(NSCam::TuningUtils::scenariorecorder::DecisionParam& decParm, std::shared_ptr<MFNRShotInfo> pShotInfo)
    {
        WRITE_DECISION_RECORD_INTO_FILE(decParm, "enable MFNR because iso:%d > threshold, isZSL:%d",
            pShotInfo->getFinalIso(), mfll::isZsdMode(pShotInfo->getMfbMode()));
        WRITE_DECISION_RECORD_INTO_FILE(decParm, "MFNR capture num:%d, blend num:%d, sensor ID:%d, isFlash:%d",
            pShotInfo->getCaptureNum(), pShotInfo->getBlendNum(), pShotInfo->getOpenId(), pShotInfo->getIsFlashOn());
        WRITE_DECISION_RECORD_INTO_FILE(decParm, "Bss nvramIdx = %d, Mfnr nvramIdx = %d, MfnrTh nvramIdx = %d",
            pShotInfo->getBssNvramIndex(), pShotInfo->getMfnrNvramIndex(), pShotInfo->getMfnrThNvramIndex());

        bool bNeedManualAe = [&](){
            // if using ZHDR, cannot apply manual AE
            if (mfll::isZhdrMode(pShotInfo->getMfbMode()))
                return false;
            // if using FLASH, cannot apply menual AE
            if (pShotInfo->getIsFlashOn())
                return false;
            // if uisng MFNR (since MFNR uses ZSD buffers), we don't need manual AE
            // but if MFNR using non-ZSD flow, we need to apply manual AE
            if (mfll::isMfllMode(pShotInfo->getMfbMode())) {
                if (mfll::isZsdMode(pShotInfo->getMfbMode()))
                    return false;
                else
                    return true;
            }
            /// otherwise, we need it
            return true;
        }();

        if (bNeedManualAe) {
            WRITE_DECISION_RECORD_INTO_FILE(decParm, "MFNR trigger manualAE, set iso to:%d , exp to %d",
                                                    pShotInfo->getFinalIso(), pShotInfo->getFinalShutterTime());
        }
    }


private:

    MINT32                          mUniqueKey;
    MINT32                          mOpenId;
    MINT32                          mSensorMode;
    MfllMode                        mMfbMode;
    MrpMode                         mMrpMode;
    MINT32                          mRealIso;
    MINT32                          mShutterTime;
    MBOOL                           mZSDMode;
    MBOOL                           mFlashOn;
    MSize                           mYuvAlign;
    MSize                           mQYuvAlign;
    MSize                           mMYuvAlign;
    MINT64                          mRecommendedKey;
    MINT32                          mMaxFrame;

    Vector<RequestPtr>              mvRequests;
    std::mutex                      mShotInfoMx; // protect MFNRShotInfo
    std::mutex                      mProcessMx; // protect MFNRShotInfo
    std::shared_future<void>        m_futureExe;
    mutable std::mutex              m_futureExeMx;

    std::unordered_map<MUINT32, std::shared_ptr<MFNRShotInfo>>
                                    mShots;
    std::shared_ptr<IMFNRCapability> mCapability;

    RequestCallbackPtr              m_callbackprt;

};

REGISTER_PLUGIN_PROVIDER(MultiFrame, MFNRProviderImpl);

