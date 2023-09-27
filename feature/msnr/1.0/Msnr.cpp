/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "MSNRCore"

// Msnr Core Lib
#include <mtkcam3/feature/msnr/IMsnr.h>
#include "RawMsnr/RawMsnr.h"
#include "YuvMsnr/YuvMsnr.h"
#include "MsnrUtils/MsnrUtils.h"

#include <mtkcam/utils/std/Log.h> // CAM_LOGD
#include <mtkcam/utils/std/ULog.h>
// Allocate working buffer. Be aware of that we use AOSP library
#include <mtkcam3/feature/utils/ImageBufferUtils.h>
// STL
#include <memory>
#include <mutex>
#include <future>
#include <chrono>
#include <unordered_map>
#include <deque> // std::deque
//tuning utils
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <cutils/properties.h>
#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam/utils/exif/DebugExifUtils.h> // for debug exif
#include <mtkcam/custom/ExifFactory.h>
// MTKCAM
#include <mtkcam/aaa/INvBufUtil.h>
#include <mtkcam3/feature/lcenr/lcenr.h>
#include <mtkcam3/3rdparty/mtk/mtk_feature_type.h>
//
// Isp profile mapper
#include <mtkcam3/feature/utils/ISPProfileMapper.h>
//
#include <mtkcam/utils/TuningUtils/CommonRule.h>

using namespace NS3Av3;
using namespace NSCam::NSIoPipe;
using namespace NSCam::TuningUtils;

using NSCam::TuningUtils::getIspProfileName;
// ----------------------------------------------------------------------------
// MY_LOG
// ----------------------------------------------------------------------------
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_MSNR);
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
static const unsigned int MSNR_AEE_DB_FLAGS = DB_OPT_NE_JBT_TRACES | DB_OPT_PROCESS_COREDUMP | DB_OPT_PROC_MEM | DB_OPT_PID_SMAPS |
                                              DB_OPT_LOW_MEMORY_KILLER | DB_OPT_DUMPSYS_PROCSTATS | DB_OPT_FTRACE;

#define MY_LOGA(fmt, arg...)                                                            \
        do {                                                                            \
            android::String8 const str = android::String8::format(fmt, ##arg);          \
            MY_LOGE("ASSERT(%s)", str.c_str());                                         \
            aee_system_exception(LOG_TAG, NULL, MSNR_AEE_DB_FLAGS, str.c_str());        \
            int err = raise(SIGKILL);                                                   \
            if (err != 0) {                                                             \
                MY_LOGE("raise SIGKILL fail! err:%d(%s)", err, ::strerror(-err));       \
            }                                                                           \
        } while(0)
#else
#define MY_LOGA(fmt, arg...)                                                            \
        do {                                                                            \
            android::String8 const str = android::String8::format(fmt, ##arg);          \
            MY_LOGE("ASSERT(%s)", str.c_str());                                         \
        } while(0)
#endif

//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//
#define __DEBUG // enable debug


#ifdef __DEBUG
#include <mtkcam/utils/std/Trace.h>
#define FUNCTION_TRACE()                            CAM_TRACE_CALL()
#define FUNCTION_TRACE_NAME(name)                   CAM_TRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)                  CAM_TRACE_BEGIN(name)
#define FUNCTION_TRACE_END()                        CAM_TRACE_END()
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

using namespace NSCam::NSPipelinePlugin;

//-----------------------------------------------------------------------------
// IMsnrCore methods
//-----------------------------------------------------------------------------
std::shared_ptr<IMsnr> IMsnr::createInstance(enum MsnrMode mode)
{
    CAM_ULOGM_APILIFE();
    std::shared_ptr<IMsnr> pMsnrCore = nullptr;
    if (mode == MsnrMode_Raw) {
        pMsnrCore = std::make_shared<MsnrCore>();
        if (pMsnrCore.get() == nullptr) {
            MY_LOGA("create raw msnr fail");
        }
        MY_LOGD("create raw msnr(%p)", pMsnrCore.get());
    } else if (mode == MsnrMode_YUV) {
        pMsnrCore = std::make_shared<YuvMsnrCore>();
        if (pMsnrCore.get() == nullptr) {
            MY_LOGA("create yuv msnr fail");
        }
        MY_LOGD("create yuv msnr(%p)", pMsnrCore.get());
    } else {
        MY_LOGA("create msnr fail, unknown mode(%d)", mode);
    }
    return pMsnrCore;
}

void* getTuningFromNvram(MUINT32 openId, MINT32 magicNo, const IMetadata* pInHalMeta, EIspProfile_T profile)
{
    CAM_ULOGM_FUNCLIFE();

    int err;
    NVRAM_CAMERA_FEATURE_STRUCT *pNvram  = nullptr;
    const void *pNRNvram = nullptr;
    MUINT32 idx;

    // load some setting from nvram
    MUINT sensorDev = MAKE_HalSensorList()->querySensorDevIdx(openId);
    IdxMgr* pMgr = IdxMgr::createInstance(static_cast<ESensorDev_T>(sensorDev));
    if (pMgr == nullptr) {
        MY_LOGE("get IndexMgr fail");
        return nullptr;
    }
    MBOOL bIspHidl = MFALSE;
    IMetadata::Memory mapInfo;
    CAM_IDX_QRY_COMB rMapping_Info = {};
    CAM_IDX_QRY_COMB *pMapInfo = nullptr;

    // check if it is isp hidl scenario or not
    if (IMetadata::getEntry<IMetadata::Memory>(pInHalMeta, MTK_ISP_ATMS_MAPPING_INFO, mapInfo)) {
        bIspHidl = MTRUE;
    } else {
        pMgr->getMappingInfo(static_cast<ESensorDev_T>(sensorDev), rMapping_Info, magicNo);
    }
    pMapInfo = bIspHidl ? reinterpret_cast<CAM_IDX_QRY_COMB*>(mapInfo.editArray()) : &rMapping_Info;
    // manual setting profile for flexible tuning
    pMapInfo->eIspProfile = profile;
    idx = pMgr->query(static_cast<ESensorDev_T>(sensorDev), NSIspTuning::EModule_MSNR_THRES, *pMapInfo, __func__);

    if (idx >= NVRAM_MSNR_THRES_TBL_NUM) {
        MY_LOGE("wrong nvram idx:%d table num:%d", idx, NVRAM_MSNR_THRES_TBL_NUM);
        return nullptr;
    }
    MY_LOGD("query nvram(%d) index: %d ispprofile: %d openid:%d sensor: %d profile: %d",
                    magicNo, idx, pMapInfo->eIspProfile, openId, pMapInfo->eSensorMode, profile);

    auto pNvBufUtil = MAKE_NvBufUtil();
    if (pNvBufUtil == nullptr) {
        MY_LOGE("pNvBufUtil==0");
        return nullptr;
    }

    auto result = pNvBufUtil->getBufAndRead(
        CAMERA_NVRAM_DATA_FEATURE,
        sensorDev, (void*&)pNvram);
    if (result != 0) {
        MY_LOGE("read buffer chunk fail");
        return nullptr;
    }

    pNRNvram = &(pNvram->MSNR_THRES[idx]);

    return (void*)pNRNvram;
}

enum MsnrMode IMsnr::getMsnrMode(const IMsnr::ModeParams &param)
{
    CAM_ULOGM_APILIFE();

    enum MsnrMode returnMode = MsnrMode_Disable;
    bool bNeedRecorder = param.needRecorder;
    // get synchelper
    auto phelper = TuningSyncHelper::getInstance(LOG_TAG);
    DecisionParam decParm = {};
    RecorderUtils::initDecisionParam(param.openId, *param.pHalMeta, &decParm);

    std::string key = "";
    if(!TuningSyncHelper::generateKey(*param.pHalMeta, &key)) {
        MY_LOGA("fail to generate key");
    }

    // In slave case, get master mode from sync helper
    if (!param.isMaster) {
        if (phelper->getModeSyncInfo(key, &returnMode)) {
            MY_LOGD("slave msnrmode follows the setting of master msnrmode(%d)", returnMode);
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "sub cam follows main cam mode(%d)", returnMode);
            return returnMode;
        } else {
            MY_LOGE("cannot get mastermode, set slave mode as disable");
            returnMode = MsnrMode_Disable;
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "sub cam cannot find main cam mode, LPNR is OFF");
            return returnMode;
        }
    }

    // GKI workaround ++
    auto ionVersion = NSCam::IImageBufferAllocator::queryIonVersion();
    if (ionVersion == NSCam::eION_VERSION_AOSP) {
        MY_LOGE("Disable msnr because of GKI temp requirement(%d)", ionVersion);
        returnMode = MsnrMode_Disable;
        phelper->setModeSyncInfo(key, returnMode);
        return returnMode;
    }
    // GKI workaround --

    int enable = ::property_get_int32("vendor.debug.camera.msnr.enable", -1);
    if (0 == enable) {
        MY_LOGD("Disable msnr because of manual command(%d)", enable);
        returnMode = MsnrMode_Disable;
        phelper->setModeSyncInfo(key, returnMode);
        IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
            "adb cmd(%d) disables LPNR, LPNR is OFF", enable);
        return returnMode;
    } else if (1 == enable) {
        MY_LOGD("Try to Force Enable msnr");
    }

    if(param.pHalMeta == nullptr || param.pDynMeta == nullptr) {
        MY_LOGE("metadata does not exit (Hal:%p,Dyn:%p)", param.pHalMeta, param.pDynMeta);
        returnMode = MsnrMode_Disable;
        phelper->setModeSyncInfo(key, returnMode);
        IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
            "lack of metadata, LPNR is OFF");
        return returnMode;
    }

    int threshold = 0;
    MINT32 iso = 0;
    MINT32 magic = 0;
    IMetadata::getEntry<MINT32>(param.pDynMeta, MTK_SENSOR_SENSITIVITY, iso);
    IMetadata::getEntry<MINT32>(param.pHalMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, magic);

    // profile mapping key
    auto mapper = ISPProfileMapper::getInstance();
    ESensorCombination sensorCom = (param.isDualMode && param.isBayerBayer) ? eSensorComb_BayerBayer :
                                   (param.isDualMode && param.isBayerMono) ? eSensorComb_BayerMono :
                                   (!param.isDualMode) ? eSensorComb_Single : eSensorComb_Invalid;

    auto mappingKey = mapper->queryMappingKey(param.pAppMeta, param.pHalMeta, param.isPhysical, eMappingScenario_Capture, sensorCom);

    EIspProfile_T profile;
    if(!mapper->mappingProfile(mappingKey, eStage_MSNR_BFBLD, profile)) {
        MY_LOGW("Failed to get ISP Profile, use EIspProfile_Capture_Before_Blend as default");
        profile = EIspProfile_Capture_Before_Blend;
    }
    IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_ATMS,
        "LPNR uses profile:%s(%d) to query ISO threshold",
        getIspProfileName(profile), profile);
    NVRAM_CAMERA_FEATURE_MSNR_THRES_STRUCT* p = (NVRAM_CAMERA_FEATURE_MSNR_THRES_STRUCT*)getTuningFromNvram(
                                                            param.openId, magic, param.pHalMeta, profile);
    if(p!=nullptr) {
        threshold = p->iso_th;
    } else {
        MY_LOGE("Disable msnr because of NVRAM pointer is null");
        returnMode = MsnrMode_Disable;
        phelper->setModeSyncInfo(key, returnMode);
        IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
            "lack of NVRAM, LPNR is OFF");
        return returnMode;
    }

    MY_LOGD("threshold:%d iso:%d, magic:%d", threshold, iso, magic);
    if (iso < threshold && enable == -1) {
        MY_LOGD("Disable msnr because iso is not greater than threshold");
        returnMode = MsnrMode_Disable;
        phelper->setModeSyncInfo(key, returnMode);
        IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
            "iso(%d) < %d, LPNR is OFF", iso, threshold);
        return returnMode;
    } else {
      IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
          "get image iso(%d) %s threshold(%d)",
              iso, (iso > threshold)? ">":
                   (iso == threshold)? "=":"<", threshold);
    }

    MINT64 iFeatureID = 0;
    if (!param.isPhysical && IMetadata::getEntry<MINT64>(param.pHalMeta, MTK_FEATURE_CAPTURE, iFeatureID)) {
        MY_LOGD("get MTK_FEATURE_CAPTURE:0x%" PRIx64 " ", iFeatureID);
    } else if (param.isPhysical && IMetadata::getEntry<MINT64>(param.pHalMeta , MTK_FEATURE_CAPTURE_PHYSICAL, iFeatureID)) {
        MY_LOGD("get MTK_FEATURE_CAPTURE_PHYSICAL:0x%" PRIx64 " ", iFeatureID);
    } else {
        MY_LOGW("cannot get feature set, isPhy:%d", param.isPhysical);
    }
    if (iFeatureID != 0) {
        if (iFeatureID & MTK_FEATURE_MFNR) {
            // In ISP6s, we don't combine MFNR and MSNR in same shot
            MY_LOGD("Disable msnr because MFNR is on");
            returnMode = MsnrMode_Disable;
            phelper->setModeSyncInfo(key, returnMode);
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "MFNR shot, LPNR is OFF");
            return returnMode;
        } else if (!(iFeatureID & MTK_FEATURE_NR)) {
            // If there is no NR in combination
            // This static API is for any cases, even no NR case
            MY_LOGD("Disable msnr because NR is off");
            returnMode = MsnrMode_Disable;
            phelper->setModeSyncInfo(key, returnMode);
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "FeatureCombination lacks LPNR, LPNR is OFF");
            return returnMode;
        }
    }

    // parse app session param to judge nr mode
    MINT32 yuvnrmode = 0;
    if (IMetadata::getEntry<MINT32>(param.pSesMeta, MTK_CONTROL_CAPTURE_SINGLE_YUV_NR_MODE, yuvnrmode)) {
        MY_LOGD("get nrmode:%d", yuvnrmode);
    } else {
        MY_LOGD("get nrmode fail");
    }

    // parse app meta to judge nr mode
    MINT32 reqyuvnrmode = 0;
    if (IMetadata::getEntry<MINT32>(param.pAppMeta, MTK_CONTROL_CAPTURE_SINGLE_YUV_NR_MODE, reqyuvnrmode)) {
        MY_LOGD("get reqnrmode:%d", reqyuvnrmode);
    } else {
        MY_LOGD("get reqnrmode fail");
    }

    // parse app meta to judge app mode
    MINT32 appmode = 0;
    if (IMetadata::getEntry<MINT32>(param.pAppMeta, MTK_CONFIGURE_SETTING_PROPRIETARY, appmode)) {
        MY_LOGD("get apmode:%d", appmode);
    } else {
        MY_LOGD("get apmode fail");
    }

    if (appmode != 1 && yuvnrmode != 1 && reqyuvnrmode != 1 && !param.isHidlIsp && enable != 1) {
        MY_LOGD("Disable msnr, nrmode:%d reqmode:%d apmode:%d hidlmode:%d forceon:%d",
                                yuvnrmode, reqyuvnrmode, appmode, param.isHidlIsp, enable);
        returnMode = MsnrMode_Disable;
        phelper->setModeSyncInfo(key, returnMode);
        IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
            "APP metadata asks LPNR to be off, LPNR is OFF");
        return returnMode;
    }

    MUINT8 nrmode = MTK_NOISE_REDUCTION_MODE_OFF;
    if (IMetadata::getEntry<MUINT8>(param.pAppMeta, MTK_NOISE_REDUCTION_MODE, nrmode)) {
        MY_LOGD("app noise reduction mode= %d forceon:%d", nrmode, enable);
        if (nrmode == MTK_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG && enable != 1) {
            returnMode = MsnrMode_Disable;
            phelper->setModeSyncInfo(key, returnMode);
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "APP metadata asks LPNR to be off, LPNR is OFF");
            return returnMode;
        }
    }

    IMsnr::DecideModeParams decideparam = {
        .openId         = param.openId,
        .isRawInput     = param.isRawInput,
        .multiframeType = param.multiframeType,
        .needRecorder   = param.needRecorder,
        .pHalMeta       = param.pHalMeta,
        .pAppMeta       = param.pAppMeta
    };
    returnMode = IMsnr::decideMsnrMode(decideparam);
    phelper->setModeSyncInfo(key, returnMode);
    return returnMode;
}

enum MsnrMode IMsnr::decideMsnrMode(const IMsnr::DecideModeParams &decideparam)
{
    CAM_ULOGM_APILIFE();
    bool bNeedRecorder = decideparam.needRecorder;
    DecisionParam decParm = {};
    RecorderUtils::initDecisionParam(decideparam.openId, *decideparam.pHalMeta, &decParm);

    MINT32 hintForSigleYuvNr = -1;
    if (IMetadata::getEntry<MINT32>(decideparam.pAppMeta, MTK_CONTROL_CAPTURE_SINGLE_YUV_NR, hintForSigleYuvNr)) {
        MY_LOGD("get MTK_CONTROL_CAPTURE_SINGLE_YUV_NR(%d)", hintForSigleYuvNr);
        if (hintForSigleYuvNr == 0) {
            MY_LOGW("Disable msnr because vendor NR tag is off");
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "APP metadata asks LPNR to be off, LPNR is OFF");
            return MsnrMode_Disable;
        }
    }

    int enableYuvMode = ::property_get_int32("vendor.debug.camera.msnr.yuv.enable", -1);
    if (1 == enableYuvMode) {
        MY_LOGD("force ymsnr because of manual command");
        IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
            "adb cmd(%d) force on YUV reprocess LPNR, YUV reprocess LPNR is ON", enableYuvMode);
        return MsnrMode_YUV;
    }

    if(decideparam.isRawInput) {
        if (decideparam.multiframeType == 1) {
            MY_LOGD("enable ymsnr, input type: %d multiframe type: %d", decideparam.isRawInput, decideparam.multiframeType);
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "Multi YUV shot, YUV reprocess LPNR is ON");
            return MsnrMode_YUV;
        } else {
            MY_LOGD("enable rmsnr, input type: %d multiframe type: %d", decideparam.isRawInput, decideparam.multiframeType);
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "LPNR is ON");
            return MsnrMode_Raw;
        }
    } else {
        if (hintForSigleYuvNr == 1) {
            MY_LOGD("enable ymsnr");
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "YUV reprocess with APP Metadata ON, YUV reprocess LPNR is ON");
            return MsnrMode_YUV;
        } else {
            MY_LOGD("Disable msnr because of yuv request");
            IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
                "YUV reprocess withAPP Metadata OFF, LPNR is OFF");
            return MsnrMode_Disable;
        }
    }

    // should not go here
    MY_LOGE("Disable msnr");
    IF_WRITE_DECISION_RECORD_INTO_FILE(bNeedRecorder, decParm, DECISION_FEATURE,
        "WEIRD status, LPNR is OFF");
    return MsnrMode_Disable;
}
