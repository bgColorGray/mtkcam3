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
#define LOG_TAG "MFNRCapability_Basic"
static const char* __CALLERNAME__ = LOG_TAG;

#include <mtkcam/utils/std/ULog.h>
#include <stdlib.h>
#include <mtkcam/utils/std/Time.h>

#include "MFNRCapability_Basic.h"

using namespace NSCam;
using namespace plugin;
using namespace android;
using namespace mfll;
using namespace std;
using namespace NSCam::NSPipelinePlugin;

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
#define MY_LOGD3(...)               do { if ( MY_DBG_COND(3) ) MY_LOGD(__VA_ARGS__); } while(0)


#define __DEBUG // enable debug

#ifdef __DEBUG
#include <mtkcam/utils/std/Trace.h>
#define FUNCTION_TRACE()                            CAM_ULOGM_FUNCLIFE()
#define FUNCTION_TRACE_NAME(name)                   CAM_ULOGM_TAGLIFE(name)
#define FUNCTION_TRACE_BEGIN(name)                  CAM_ULOGM_TAG_BEGIN(name)
#define FUNCTION_TRACE_END()                        CAM_ULOGM_TAG_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)    CAM_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)      CAM_TRACE_ASYNC_END(name, cookie)
#else
#define FUNCTION_TRACE()
#define FUNCTION_TRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)
#define FUNCTION_TRACE_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)
#endif

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


MFNRCapability_Basic::MFNRCapability_Basic()
{
    FUNCTION_SCOPE;
    m_dbgLevel = mfll::MfllProperty::getDebugLevel();
}

MFNRCapability_Basic::~MFNRCapability_Basic()
{
    FUNCTION_SCOPE;
}

int MFNRCapability_Basic::getSelIndex(Selection& sel)
{
    int idx = -1;
    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
            idx = sel.mRequestIndex;
            break;

        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            if (CC_LIKELY(sel.mIMetadataApp.getControl() != NULL)) {
                IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();
                if (!IMetadata::getEntry<MINT32>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX, idx)) {
                    MY_LOGE("cannot get ISP_FRAME_INDEX");
                }
            } else {
                MY_LOGE("Error: mIMetadataApp is NULL!!");
            }
            break;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            idx = sel.mStateIspHidl.frameIndex;
            break;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
    }

    return idx;
}

int MFNRCapability_Basic::getSelCount(Selection& sel)
{
    int cnt = -1;
    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
            cnt = sel.mRequestCount;
            break;

        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            if (CC_LIKELY(sel.mIMetadataApp.getControl() != NULL)) {
                IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();
                if (!IMetadata::getEntry<MINT32>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_COUNT, cnt)) {
                    MY_LOGE("cannot get ISP_FRAME_COUNT");
                }
            } else {
                MY_LOGE("Error: mIMetadataApp is NULL!!");
            }
            break;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            cnt = sel.mStateIspHidl.frameCount;
            break;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
    }

    return cnt;
}

MINT64 MFNRCapability_Basic::getTuningIndex(Selection& sel)
{
    MINT64 tuningIdx = -1;

    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
            break;
        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            if (CC_LIKELY(sel.mIMetadataApp.getControl() != NULL)) {
                IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();
                if (!IMetadata::getEntry<MINT64>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX, tuningIdx)) {
                    MY_LOGW("cannot get MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX");
                } else {
                    MY_LOGD("get MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX:0x%jx", tuningIdx);
                }
            } else {
                MY_LOGE("Error: mIMetadataApp is NULL!!");
            }
            break;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
#ifdef ISP_HIDL_TEMP_SOL_WITHOUT_TUNING_INDEX_HINT
            if (CC_LIKELY(sel.mIMetadataHal.getControl() != NULL)) {
                IMetadata* pHalMeta = sel.mIMetadataHal.getControl().get();
                if (!IMetadata::getEntry<MINT64>(pHalMeta, MTK_FEATURE_MFNR_TUNING_INDEX_HINT, tuningIdx)) {
                    MY_LOGW("cannot get MTK_FEATURE_MFNR_TUNING_INDEX_HINT");
                } else {
                    MY_LOGD("get MTK_FEATURE_MFNR_TUNING_INDEX_HINT: 0x%jx", tuningIdx);
                }
            }
#endif
            if (tuningIdx == -1 && CC_LIKELY(sel.mIMetadataApp.getControl() != NULL)) {
                IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();
                if (!IMetadata::getEntry<MINT64>(pAppMeta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX, tuningIdx)) {
                    MY_LOGW("cannot get MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX");
                } else {
                    MY_LOGD("get MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_TUNING_INDEX: 0x%jx", tuningIdx);
                }
            }
            break;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
    }

    return tuningIdx;
}

MINT32 MFNRCapability_Basic::getUniqueKey(Selection& sel)
{
    MINT32 uniqueKey = static_cast<MINT32>(NSCam::Utils::TimeTool::getReadableTime());
    MY_LOGD("IspHidlStage is :%d", sel.mIspHidlStage);
    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            break;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            if (sel.mIMetadataHal.getControl() != NULL) {
                IMetadata* pHalMeta = sel.mIMetadataHal.getControl().get();
                if (!IMetadata::getEntry<MINT32>(pHalMeta, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
                    MY_LOGW("cannot get PIPELINE_UNIQUE_KEY");
                } else {
                    MY_LOGD("MTK_PIPELINE_UNIQUE_KEY: %d", uniqueKey);
                }
            } else {
                MY_LOGE("Error: mIMetadataHal is NULL!!");
            }
            break;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
    }

    return uniqueKey;
}

MBOOL MFNRCapability_Basic::getRecommendShot(std::unordered_map<MUINT32, std::shared_ptr<MFNRShotInfo>>& Shots, Selection& sel, MINT64 HidlQueryIndex)
{
    MBOOL needUpdateStrategy = MTRUE;

    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            return needUpdateStrategy;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            if (sel.mIMetadataHal.getControl() != NULL) {
                MINT32 SensorId = sel.mSensorId;
                MY_LOGD("SensorId in ISP_HIDL: %d", SensorId);

                if (HidlQueryIndex != -1) {
                    MINT32 UniqueKey = getUniqueKey(sel);
                    vector<RecommendShooter*> shooter = mRecommendQueue.getInfo(__CALLERNAME__, HidlQueryIndex, HidlQueryIndex);
                    if (!shooter.empty() && shooter[0]->shotInstance != nullptr) {
                        if (Shots[UniqueKey] == nullptr) {
                            if (shooter[0]->shotInstance->getOpenId() == SensorId) {
                                MY_LOGD("replace shot instance to recommended one.");
                                Shots[UniqueKey] = shooter[0]->shotInstance;
                                shooter[0]->shotInstance = nullptr;
                                Shots[UniqueKey]->updateUniqueKey(UniqueKey);
                                needUpdateStrategy = MFALSE;
                            } else {
                                MY_LOGW("Reommended OpenId:%d is not the same with camera stage ID:%d", shooter[0]->shotInstance->getOpenId(), SensorId);
                            }
                        } else {
                            MY_LOGW("Reommended shot is already exist.");
                        }
                    } else {
                        MY_LOGW("Reommended shot(%jx) is not found.", HidlQueryIndex);
                    }
                    mRecommendQueue.returnInfo(__CALLERNAME__, shooter);
                }
            }
            else {
                MY_LOGE("Error: mIMetadataHal is NULL!!");
            }
            return needUpdateStrategy;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
            break;
    }

    return needUpdateStrategy;
}

void MFNRCapability_Basic::setSrcSize(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel)
{
    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            pShotInfo->setSizeSrc(sel.mState.mSensorSize);
            break;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            if (sel.mStateIspHidl.pAppImage_Input_Yuv != nullptr) {
                pShotInfo->setSizeSrc(sel.mStateIspHidl.pAppImage_Input_Yuv->getImgSize());
            }
            else if (sel.mStateIspHidl.pAppImage_Input_RAW16 != nullptr) {
                pShotInfo->setSizeSrc(sel.mStateIspHidl.pAppImage_Input_RAW16->getImgSize());
            }
            else {
                MY_LOGE("Error: ISP HIDL no available input buffer");
            }
            break;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
            break;
    }
    MY_LOGD("set source size = %dx%d", pShotInfo->getSizeSrc().w, pShotInfo->getSizeSrc().h);
    return;
}

void MFNRCapability_Basic::updateInputMeta(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel, IMetadata* pHalMeta, MINT64 key)
{
    if (sel.mIspHidlStage == ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA) {
        IMetadata::setEntry<MUINT8>(pHalMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, 0);
        if (getTuningIndex(sel) != -1) {
            MINT32 queryIdx = getQueryIndexFromRecommendedKey(getTuningIndex(sel));
            MY_LOGD("queryIdx:%d", queryIdx);
            IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_MFNR_NVRAM_QUERY_INDEX, queryIdx);
        }
        else {
            MY_LOGD("fail to get tuning index, use %d from shotInfo instead", pShotInfo->getBssNvramIndex());
            IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_MFNR_NVRAM_QUERY_INDEX, pShotInfo->getBssNvramIndex());
        }
        #if (SUPPORT_YUV_BSS == 0)
        /* MTK_FEATURE_BSS_SELECTED_FRAME_COUNT for raw domain bss */
        IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_BSS_SELECTED_FRAME_COUNT, getSelCount(sel));
        IMetadata::setEntry<MBOOL>(pHalMeta, MTK_FEATURE_BSS_DOWNSAMPLE, MTRUE);
        IMetadata::setEntry<MUINT8>(pHalMeta, MTK_FEATURE_PACK_RRZO, 1);
        IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_BSS_PROCESS, MTK_FEATURE_BSS_PROCESS_ENABLE);
        IMetadata::setEntry<MBOOL>(pHalMeta, MTK_FEATURE_BSS_REORDER, MTRUE);
        #endif
#ifdef ISP_HIDL_TEMP_SOL_WITHOUT_TUNING_INDEX_HINT
        IMetadata::setEntry<MINT64>(pHalMeta, MTK_FEATURE_MFNR_TUNING_INDEX_HINT, key);
#endif
    }
    else {
        IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_MFNR_NVRAM_QUERY_INDEX, pShotInfo->getBssNvramIndex());
        IMetadata::setEntry<MUINT8>(pHalMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, getSelIndex(sel)?2:0);
        IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_BSS_SELECTED_FRAME_COUNT, pShotInfo->getBlendNum());
        IMetadata::setEntry<MINT32>(pHalMeta, MTK_FEATURE_BSS_PROCESS, MTK_FEATURE_BSS_PROCESS_ENABLE);
        IMetadata::setEntry<MBOOL>(pHalMeta, MTK_FEATURE_BSS_REORDER, MTRUE);
    }
    return;
}

int MFNRCapability_Basic::updateSelection(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel)
{
    int SelCount = 0;

    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
            sel.mRequestCount = pShotInfo->getCaptureNum();
            SelCount = sel.mRequestCount;
            MY_LOGD("Mfll apply = %d, frames = %d", pShotInfo->getIsEnableMfnr(), sel.mRequestCount);
            break;

        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
            sel.mDecision.mProcess = false;
            sel.mRequestCount = 1;
            SelCount = getSelCount(sel);
            sel.mDecision.mZslEnabled = isZsdMode(pShotInfo->getMfbMode());
            sel.mDecision.mZslPolicy.mPolicy = ZslPolicy::AFStable
                                                | ZslPolicy::ContinuousFrame
                                                | ZslPolicy::ZeroShutterDelay;
            sel.mDecision.mZslPolicy.mTimeouts = 1000;
            sel.mDecision.mZslPolicy.burstIndex = getSelIndex(sel);
            sel.mDecision.mZslPolicy.totalFrameSize = getSelCount(sel);
            break;

        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            sel.mDecision.mProcess = false;
            sel.mRequestCount = 1;
            SelCount = getSelCount(sel);
            break;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
            break;
    }

    return SelCount;
}

bool MFNRCapability_Basic::getEnableMfnr(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel)
{
    switch (sel.mIspHidlStage) {
        case ISP_HIDL_STAGE_DISABLED:
            return pShotInfo->getIsEnableMfnr();

        case ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA:
        case ISP_HIDL_STAGE_PROCESS_FRAME_IN_ISP_HIDL:
            return true;

        default:
            MY_LOGE("IspHidlStage is unknown:%d", sel.mIspHidlStage);
            return false;
    }
}

MINT64 MFNRCapability_Basic::generateRecommendedKey(MINT32 queryIndex)
{
    std::lock_guard<decltype(mRecommendCountMx)> lk(mRecommendCountMx);

    mRecommendCount++;
    MINT64 ret = (MINT64(mRecommendCount) << 32) | (MINT64(queryIndex) & 0xFFFFFFFF);
    MY_LOGD("%s: 0x%x --> 0x%jx ", __FUNCTION__, queryIndex, ret);

    return ret;
}

bool MFNRCapability_Basic::publishShotInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel, MINT64 key)
{
    if(sel.mIspHidlStage == ISP_HIDL_STAGE_REUEST_FRAME_FROM_CAMERA)
    {
#ifdef ISP_HIDL_TEMP_SOL_WITHOUT_TUNING_INDEX_HINT
        MY_LOGD("Last selection (%d/%d) of ISP_HIDL_CAMERA, move ShotInfo to fleeting queue", getSelIndex(sel), getSelCount(sel));
        //store shot instance to mRecommendQueue
        RecommendShooter* shooter = mRecommendQueue.editInfo(__CALLERNAME__, key);
        if (CC_LIKELY(shooter != nullptr)) {
            shooter->shotInstance = pShotInfo;
            MY_LOGD("move ShotInfo to fleeting queue, key:0x%jx", key);
        } else {
            MY_LOGW("Reommended shot is not kept.");
        }
        mRecommendQueue.publishInfo(__CALLERNAME__, shooter);
#else
        MY_LOGD("Last selection (%d/%d) of ISP_HIDL_CAMERA, remove ShotInfo", getSelIndex(sel), getSelCount(sel));
#endif
        return true;
    }
    return false;
}

void MFNRCapability_Basic::publishShotInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, MINT64 key)
{
    //store shot instance to mRecommendQueue
    RecommendShooter* shooter = mRecommendQueue.editInfo(__CALLERNAME__, key);
    if (CC_LIKELY(shooter != nullptr)) {
        shooter->shotInstance = pShotInfo;
        MY_LOGD("move ShotInfo to fleeting queue, key:0x%jx", key);
    } else {
        MY_LOGW("Reommended shot is not kept.");
    }
    mRecommendQueue.publishInfo(__CALLERNAME__, shooter);
}

MSize MFNRCapability_Basic::calcDownScaleSize(const MSize& m, int dividend, int divisor)
{
    if (CC_LIKELY( divisor ))
        return MSize(m.w * dividend / divisor, m.h * dividend / divisor);

    MY_LOGW("%s: divisor is zero", __FUNCTION__);
    return m;
}

MINT32 MFNRCapability_Basic::getQueryIndexFromRecommendedKey(MINT64 recommendKey)
{
    MINT32 ret = MINT32(recommendKey & 0xFFFFFFFF);
    MY_LOGD("%s: 0x%jx --> 0x%x ", __FUNCTION__, recommendKey, ret);
    return ret;
}
