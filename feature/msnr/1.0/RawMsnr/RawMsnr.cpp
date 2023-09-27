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
#define LOG_TAG "RAWMSNRCore"

#include "RawMsnr.h"
#include "../MsnrUtils/MsnrUtils.h"

#include <mtkcam/utils/std/Log.h> // CAM_LOGD
#include <mtkcam/utils/std/ULog.h>
// Allocate working buffer. Be aware of that we use AOSP library
#include <mtkcam3/feature/utils/ImageBufferUtils.h>
//

// platform dependent headers
#include <dip_reg.h> // dip_x_reg_t
// ISP profile
//#include <tuning_mapping/cam_idx_struct_ext.h>
//tuning utils
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
#include <cutils/properties.h>
#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam/utils/exif/DebugExifUtils.h> // for debug exif
#include <mtkcam/custom/ExifFactory.h>
// Custom debug exif
#include <debug_exif/dbg_id_param.h>
#include <debug_exif/cam/dbg_cam_param.h>
// Std
#include <unordered_map> // std::unordered_map
#include <deque> // std::deque
// MTKCAM
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/iopipe/PostProc/IEgnStream.h>
#include <mtkcam/drv/def/mfbcommon_v20.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
#include <mtkcam/aaa/INvBufUtil.h>
//
#include <mtkcam/utils/TuningUtils/CommonRule.h>
//
#include <mtkcam3/feature/lcenr/lcenr.h>
#include <mtkcam3/3rdparty/mtk/mtk_feature_type.h>
//
using namespace NS3Av3;
using namespace NSCam::NSIoPipe;
using namespace NSCam::TuningUtils;
using namespace NSCam::NSIoPipe::NSEgn;
using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam::NSIoPipe::NSMFB20;
using namespace NSCam::NSPipelinePlugin;

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
//-----------------------------------------------------------------------------
// MSNR constant
//-----------------------------------------------------------------------------
#define MSS_BUF_COUNT 8
#define MSS_RAW_IN_START 2
#define MSF_BUF_COUNT 7
#define P2_TUNING_COUNT MSF_BUF_COUNT
#define HW_BYTE_ALIGNMENT 4

#define MIX_PORT_IN_BASE_FRAME PORT_IMGI
#define MIX_PORT_OUT_MAIN      PORT_WDMAO
#define MIX_PORT_OUT_THUMBNAIL PORT_WROTO

#define PASS2_CROPPING_ID_CRZ       1
#define PASS2_CROPPING_ID_WDMAO     2
#define PASS2_CROPPING_ID_WROTO     3

/* Cropping group ID is related port ID, notice this ... */
#define RAW2YUV_GID_OUT             PASS2_CROPPING_ID_CRZ
#define RAW2YUV_GID_OUT2            PASS2_CROPPING_ID_WROTO
#define RAW2YUV_GID_OUT3            PASS2_CROPPING_ID_WDMAO

#define MIX_MAIN_GID_OUT            PASS2_CROPPING_ID_WDMAO
#define MIX_THUMBNAIL_GID_OUT       PASS2_CROPPING_ID_WROTO

#define MSNR_ISP_PROFILE_ERROR 0xFFFFFFFF
#define HW_RETRY_TIME 60
#define HW_TIMEOUT_TIME 1500

#define ALIGN(w, a) (((w + (a-1)) / a) * a)
//-----------------------------------------------------------------------------
// MSNR util
//-----------------------------------------------------------------------------

/* global option to enable DRE or not */
#define DRE_PROPERTY_NAME "vendor.camera.mdp.dre.enable"
static MINT32 gSupportDRE = []() {
    MINT32 supportDRE = ::property_get_int32(DRE_PROPERTY_NAME, -1);
    if (supportDRE < 0) {
        supportDRE = 0;
        MY_LOGW("Get property: " DRE_PROPERTY_NAME " fail, default set to 0");
    }
    return supportDRE;
}();
#undef DRE_PROPERTY_NAME

#if MTK_CAM_NEW_NVRAM_SUPPORT
// {{{
template <NSIspTuning::EModule_T module>
inline void* _getNVRAMBuf(NVRAM_CAMERA_FEATURE_STRUCT* /*pNvram*/, size_t /*idx*/)
{
    MY_LOGE("_getNVRAMBuf: unsupport module(%d)", module);
    *(volatile uint32_t*)(0x00000000) = 0xDEADC0DE;
    return nullptr;
}

template<>
inline void* _getNVRAMBuf<NSIspTuning::EModule_CA_LTM>(
        NVRAM_CAMERA_FEATURE_STRUCT*    pNvram,
        size_t                          idx
        )
{
    return &(pNvram->CA_LTM[idx]);
}

// magicNo is from HAL metadata of MTK_P1NODE_PROCESSOR_MAGICNUM <MINT32>
template<NSIspTuning::EModule_T module>
inline std::tuple<void*, int>
getTuningTupleFromNvram(const IMetadata* pInHalMeta, MUINT32 openId, MUINT32 idx, MINT32 magicNo, MINT32 ispProfile = -1)
{
    CAM_ULOGM_FUNCLIFE();
    NVRAM_CAMERA_FEATURE_STRUCT *pNvram = nullptr;
    void* pNRNvram              = nullptr;
    std::tuple<void*, int>      emptyVal(nullptr, -1);

    if (__builtin_expect( idx >= EISO_NUM, false )) {
        MY_LOGE("wrong nvram idx %d", idx);
        return emptyVal;
    }

    // load some setting from nvram
    MUINT sensorDev = MAKE_HalSensorList()->querySensorDevIdx(openId);
    IdxMgr* pMgr = IdxMgr::createInstance(static_cast<ESensorDev_T>(sensorDev));
    if (pMgr == nullptr) {
        MY_LOGE("get IndexMgr fail");
        return emptyVal;
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

    // tricky: force set ISP profile since new NVRAM SW limitation
    if (ispProfile >= 0) {
        pMapInfo->eIspProfile = static_cast<EIspProfile_T>( ispProfile );
        pMgr->setMappingInfo(static_cast<ESensorDev_T>(sensorDev), *pMapInfo, magicNo);
        MY_LOGD("%s: force set ISP profile to %#x", __FUNCTION__, ispProfile);
    }

    // query NVRAM index by mapping info
    idx = pMgr->query(static_cast<ESensorDev_T>(sensorDev), module, *pMapInfo, __FUNCTION__);
    MY_LOGD("query nvram DRE mappingInfo index: %d", idx);

    auto pNvBufUtil = MAKE_NvBufUtil();
    if (__builtin_expect( pNvBufUtil == nullptr, false )) {
        MY_LOGA("pNvBufUtil==0");
        return emptyVal;
    }

    auto result = pNvBufUtil->getBufAndRead(
        CAMERA_NVRAM_DATA_FEATURE,
        sensorDev, (void*&)pNvram);
    if (__builtin_expect( result != 0, false )) {
        MY_LOGE("read buffer chunk fail");
        return emptyVal;
    }

    pNRNvram = _getNVRAMBuf<module>(pNvram, static_cast<size_t>(idx));
    return std::make_tuple( pNRNvram,  idx );
}

#endif

//-----------------------------------------------------------------------------
// MsnrCore implementation
//-----------------------------------------------------------------------------
MsnrCore::MsnrCore()
    : mRegTableSize(0)
    , mNddDumpL1(-1)
    , mNddDumpL2(-1)
    , mIsInited(false)
    , mpHalIsp(nullptr)
    , mpISPProfileMapper(nullptr)
{
    CAM_ULOGM_APILIFE();
    mNddDumpL1 = ::property_get_int32("vendor.debug.camera.msnr.ndd.l1", 0);
    mNddDumpL2 = ::property_get_int32("vendor.debug.camera.msnr.ndd.l2", 0);
    mOnlineTuning = ::property_get_int32("vendor.debug.camera.dumpin.en", 0);
    mEnableDebugBuf = ::property_get_int32("vendor.debug.camera.msnr.debugbuf", 0);
    // sync LSCO dump with P2A
    mP2Dump = ::property_get_int32("vendor.debug.camera.p2.dump", 0);
    mCapPipeDump = ::property_get_int32("vendor.debug.camera.dump.campipe", 0);
}
//-----------------------------------------------------------------------------
MsnrCore::~MsnrCore()
{
    CAM_ULOGM_APILIFE();

    if(!mIsInited)
        return;

    // wait alloc working future done (timeout 1 second)
    if (CC_LIKELY( mMssAllocThread.valid() )) {
        auto _status = mMssAllocThread.wait_for(std::chrono::seconds(1));
        if (CC_UNLIKELY( _status != std::future_status::ready )) {
            MY_LOGE("wait mss future status is not ready");
        }
    }
    deallocateMssBuf();

    if (CC_LIKELY( mMsfAllocThread.valid() )) {
        auto _status = mMsfAllocThread.wait_for(std::chrono::seconds(1));
        if (CC_UNLIKELY( _status != std::future_status::ready )) {
            MY_LOGE("wait msf future status is not ready");
        }
    }
    deallocateMsfBuf();

    if (CC_LIKELY( mTunPrepareThread.valid() )) {
        auto _status = mTunPrepareThread.wait_for(std::chrono::seconds(1));
        if (CC_UNLIKELY( _status != std::future_status::ready )) {
            MY_LOGE("wait tun future status is not ready");
        }
    }
    releaseTunBuf();

    if (mEnableDebugBuf || mNddDumpL1 ==1 || mNddDumpL2 == 1) {
        deallocateDebugBuf();
    }
}
//-----------------------------------------------------------------------------
enum MsnrErr MsnrCore::init(IMsnr::ConfigParams const& params)
{
    CAM_ULOGM_APILIFE();
    enum MsnrErr err = MsnrErr_Ok;

    int sensorId = params.openId;

    mConfig = params;

    // Start to check params content
    if(CC_UNLIKELY(checkParams(mConfig) != MsnrErr_Ok)) {
        MY_LOGE("check param fail");
        return MsnrErr_BadArgument;
    }

     /* RAII for INormalStream */
    mpNormalStream = decltype(mpNormalStream)(
        INormalStream::createInstance(sensorId),
        [](INormalStream* p) {
            if (!p) return;
            p->uninit(LOG_TAG);
            p->destroyInstance();
        }
    );

    if (CC_UNLIKELY( mpNormalStream.get() == nullptr )) {
        MY_LOGE("create INormalStream fail");
        return MsnrErr_BadArgument;
    } else {
        auto bResult = mpNormalStream->init(LOG_TAG);
        if (CC_UNLIKELY(bResult == MFALSE)) {
            MY_LOGE("init INormalStream returns MFALSE");
            return MsnrErr_BadArgument;
        }
    }

    // get tuning size
    mRegTableSize = mpNormalStream->getRegTableSize();

     /* RAII for IEgnStream<MSSConfig> */
    mpMssStream = decltype(mpMssStream)(
        IEgnStream<MSSConfig>::createInstance(LOG_TAG),
        [](IEgnStream<MSSConfig>* p) {
            if (!p) return;
            p->uninit();
            p->destroyInstance(LOG_TAG);
        }
    );
    /* init IEgnStream<MSSConfig> */
    if (CC_UNLIKELY( mpMssStream.get() == nullptr )) {
        MY_LOGE("create IEgnStream of MSS fail");
        return MsnrErr_BadArgument;
    } else {
        auto bResult = mpMssStream->init();
        if (CC_UNLIKELY(bResult == MFALSE)) {
            MY_LOGE("init IEgnStream of MSS returns MFALSE");
            return MsnrErr_BadArgument;
        }
    }

    /* RAII for IHalISP */
    mpHalIsp = decltype(mpHalIsp)(
        MAKE_HalISP(sensorId, "LOG_TAG"),
        [](NS3Av3::IHalISP* p) {
            if(CC_LIKELY(p))
                p->destroyInstance("LOG_TAG");
        }
    );
    if (CC_UNLIKELY(mpHalIsp.get() == nullptr)) {
        MY_LOGE("create IHalIsp failed");
        return MsnrErr_BadArgument;
    }

    // profile mapping key
    mpISPProfileMapper = ISPProfileMapper::getInstance();
    ESensorCombination sensorCom = (mConfig.isDualMode && mConfig.isBayerBayer) ? eSensorComb_BayerBayer :
                                   (mConfig.isDualMode && mConfig.isBayerMono) ? eSensorComb_BayerMono :
                                   (!mConfig.isDualMode) ? eSensorComb_Single : eSensorComb_Invalid;

    mMappingKey = mpISPProfileMapper->queryMappingKey(mConfig.appMeta, mConfig.halMeta, mConfig.isPhysical, eMappingScenario_Capture, sensorCom);

    // Allocate MSS working buffer
    mMssAllocThread = std::async(std::launch::async, &MsnrCore::allocateMssBuf, this);
    // Allocate MSF working buffer
    mMsfAllocThread = std::async(std::launch::async, &MsnrCore::allocateMsfBuf, this);
    // prepare Tuning working buffer
    mTunPrepareThread = std::async(std::launch::async, &MsnrCore::prepareTunBuf, this);

    mIsInited = true;

    return err;
}

enum MsnrErr MsnrCore::doMsnr()
{
    CAM_ULOGM_APILIFE();
    enum MsnrErr err = MsnrErr_Ok;
    DecisionParam decParm = {};
    RecorderUtils::initDecisionParam(mConfig.openId, *mConfig.halMeta, &decParm);
    ResultParam resParam = {};
    initExecutionParam(mConfig.openId, DecisionType::DECISION_ISP_FLOW,
        decParm.dbgNumInfo, &resParam);

    // Input/Output buffer
    IImageBuffer* inputBuffer    = mConfig.inputBuff;
    IImageBuffer* inputRSZBuffer = mConfig.inputRSZBuff;
    IImageBuffer* outputBuffer   = mConfig.outputBuff;
    //********************** MSS ****************************
    if (CC_LIKELY( mMssAllocThread.valid() )) {
        if (CC_UNLIKELY( mMssAllocThread.get() != 0 )) {
            MY_LOGA("MSS allocate working buffer failed");
            return MsnrErr_BadArgument;
        }
    }
    {
        EGNParams<MSSConfig> rMssParams;
        rMssParams.mpEngineID = eMSS;

        struct MSSConfig mssconfig;
        mssconfig.mss_scenario = MSS_BASE_S2_MODE; // raw in
        mssconfig.tag = EStreamTag_VSS;
        mssconfig.scale_Total = mvMssBuf.size();

        FILE_DUMP_NAMING_HINT mssDumpHint{};
        generateMssNDDHint(mConfig.halMeta, mssDumpHint);
        mssconfig.dumphint = &mssDumpHint;

        mssconfig.dump_en = DUMP_FEATURE_CAPTURE;
        // std::vector<IImageBuffer*>
        std::vector<IImageBuffer*> mssOutput(mvMssBuf.size());
        for(int i = 0; i < mssOutput.size(); i++) {
            switch (i) {
                case 0:
                    mssOutput.at(i) = inputBuffer;
                break;

                case 1:
                    mssOutput.at(i) = inputRSZBuffer;
                break;

                default:
                    mssOutput.at(i) = mvMssBuf.at(i).get();
            }
        }
        mssconfig.mss_scaleFrame = mssOutput;

        rMssParams.mEGNConfigVec.push_back(mssconfig);

        // Callback
        Pass2Async p2Async;
        rMssParams.mpCookie = static_cast<void*>(&p2Async);
        rMssParams.mpfnCallback = [](EGNParams<MSSConfig>& rParams)->MVOID
        {
            if (CC_UNLIKELY(rParams.mpCookie == nullptr))
                return;

            Pass2Async* pAsync = static_cast<Pass2Async*>(rParams.mpCookie);
            std::lock_guard<std::mutex> __l(pAsync->getLocker());
            pAsync->setStatus(true);
            pAsync->notifyOne();
            MY_LOGD("Msnr MSS done");
        };

        // Start to enque request to pass2
        {
            FUNCTION_TRACE_NAME("MSNR processing");
            auto __l = p2Async.uniqueLock();
            MBOOL bEnqueResult = MTRUE;
            {
                if (CC_UNLIKELY(mpMssStream.get() == nullptr)) {
                    MY_LOGE("IEgnStream of MSS instance is NULL");
                    return MsnrErr_BadArgument;
                }
                MY_LOGD("Msnr MSS enque");
                bEnqueResult = mpMssStream->EGNenque(rMssParams);
            }

            if (CC_UNLIKELY(!bEnqueResult)) {
                MY_LOGA("Msnr enque fail");
                return MsnrErr_BadArgument;
            } else {
                struct timeval startTime;
                gettimeofday(&startTime, NULL);

                while (p2Async.getStatus() != true) {
                    struct timeval currentTime, elapsedTime;
                    gettimeofday(&currentTime, NULL);
                    timersub(&currentTime, &startTime, &elapsedTime);

                    int timeDiff = elapsedTime.tv_sec*1000 + elapsedTime.tv_usec/1000; //ms
                    if (CC_UNLIKELY(timeDiff > HW_TIMEOUT_TIME)) {
                        MY_LOGA("MSS wait timeout: %d ms diff: %d ms", HW_TIMEOUT_TIME, timeDiff);
                    }

                    auto status = p2Async.wait_for(std::move(__l), HW_RETRY_TIME /*ms*/);
                    if (CC_UNLIKELY(status == std::cv_status::timeout)) {
                        MY_LOGI("MSS wait timeout, wait again");
                    }
                }
            }
        }

        if(mNddDumpL1 == 1 || mNddDumpL2 == 1) {
            for (int i = 0; i < mvMssBuf.size(); i++) {
                std::string str = "lpnr-msfp2-L" + std::to_string(i) + "-in";
                const char* pUserString = str.c_str();
                switch(i) {
                    case 0:
                        NddDump(inputBuffer, mssDumpHint, YUV_PORT_NULL, pUserString);
                    break;

                    case 1:
                        NddDump(inputRSZBuffer, mssDumpHint, YUV_PORT_NULL, pUserString);
                    break;

                    default:
                        NddDump(mvMssBuf.at(i).get(), mssDumpHint, YUV_PORT_NULL, pUserString);
                }
            }
        }
    }
    //********************* MSF-P2 **************************
    if (CC_LIKELY( mTunPrepareThread.valid() )) {
        if (CC_UNLIKELY( mTunPrepareThread.get() != 0 )) {
            MY_LOGA("Prepare tuning buffer failed");
            return MsnrErr_BadArgument;
        }
    }

    {
        std::vector<QParams>               qParam(MSF_BUF_COUNT);
        std::vector<FrameParams>           params(MSF_BUF_COUNT);
        std::vector<FILE_DUMP_NAMING_HINT> msfDumpHint(MSF_BUF_COUNT);
        std::vector<EGNParams<MSFConfig> > msfParams(MSF_BUF_COUNT);
        std::unique_ptr<ModuleInfo, std::function<void(ModuleInfo*)>> pSRZ4module;
        std::unique_ptr<ModuleInfo, std::function<void(ModuleInfo*)>> pSRZ3module;
        // static allocation
        Pass2Async                         vp2Async[MSF_BUF_COUNT];

        // 1. MSF param
        for(int stage = MSF_BUF_COUNT-1; stage >= 0; stage--) {

            MSFConfig msfconfig;
            msfconfig.msf_scenario = MSF_MSNR_DL_MODE;
            msfconfig.frame_Idx = 0;
            msfconfig.frame_Total = 1;
            msfconfig.scale_Idx = stage;
            msfconfig.scale_Total = MSF_BUF_COUNT + 1;

            msfconfig.msf_dsFrame = (stage == MSF_BUF_COUNT-1)?mvMssBuf.at(MSS_BUF_COUNT-1).get():mvMsfBuf.at(stage+1).get(); //dsi
            switch(stage) {
                case 0:
                    msfconfig.msf_baseFrame.push_back(inputBuffer); //basei
                break;

                case 1:
                    msfconfig.msf_baseFrame.push_back(inputRSZBuffer); //basei
                break;

                default:
                    msfconfig.msf_baseFrame.push_back(mvMssBuf.at(stage).get()); //basei
            }

            generateMsfNDDHint(mConfig.halMeta, stage, msfDumpHint.at(stage));
            IF_WRITE_DECISION_RECORD_INTO_FILE(true, decParm, DECISION_ATMS,
                "%s DIP uses profile:%s(%d)", ORDINAL(MSF_BUF_COUNT-stage),
                    getIspProfileName(static_cast<EIspProfile_T>(msfDumpHint.at(stage).IspProfile)),
                    msfDumpHint.at(stage).IspProfile);
            resParam.stageId = getIspStage(stage);
            WRITE_EXECUTION_RESULT_INTO_FILE(resParam,
                "%s LPNR DIP uses profile:%s(%d)", ORDINAL(MSF_BUF_COUNT-stage),
                    getIspProfileName(static_cast<EIspProfile_T>(msfDumpHint.at(stage).IspProfile)),
                    msfDumpHint.at(stage).IspProfile);
            msfconfig.dumphint = &msfDumpHint.at(stage);
            msfconfig.dump_en = DUMP_FEATURE_CAPTURE;

            msfParams.at(stage).mEGNConfigVec.push_back(msfconfig);
            msfParams.at(stage).mpEngineID = eMSF;
            {
                ExtraParam extra;
                extra.CmdIdx = EPIPE_MSF_INFO_CMD;
                extra.moduleStruct = static_cast<void*>(&msfParams.at(stage).mEGNConfigVec.back());
                params.at(stage).mvExtraParam.push_back(extra);
            }

            /* execute pass 2 operation */
            {
                //Tuning data
                params.at(stage).mTuningData = static_cast<MVOID*>(mvTunBuf.at(stage).get()); // adding tuning data

                //NDD dump hint
                {

                    int64_t timestamp = 0; // set to 0 if not found.
#if MTK_CAM_DISPAY_FRAME_CONTROL_ON
                    IMetadata::getEntry<int64_t>(mConfig.halMeta, MTK_P1NODE_FRAME_START_TIMESTAMP, timestamp);
#else
                    IMetadata::getEntry<int64_t>(mConfig.appMeta, MTK_SENSOR_TIMESTAMP, timestamp);
#endif
                    params.at(stage).FrameNo    = msfDumpHint.at(stage).FrameNo;
                    params.at(stage).RequestNo  = msfDumpHint.at(stage).RequestNo;
                    params.at(stage).Timestamp  = timestamp;
                    params.at(stage).UniqueKey  = msfDumpHint.at(stage).UniqueKey;
                    params.at(stage).IspProfile = msfDumpHint.at(stage).IspProfile;
                    params.at(stage).SensorDev  = msfDumpHint.at(stage).SensorDev;
                    params.at(stage).NeedDump   = 2;
                }

                // output: main yuv
                {
                    MY_LOGD("config main yuv during msf-p2");
                    Output p;
                    p.mBuffer          = mvMsfBuf.at(stage).get();
                    p.mPortID          = PORT_IMG3O;
                    p.mPortID.group    = 0; // always be 0
                    params.at(stage).mvOut.push_back(p);

                    /**
                     *  Cropping info, only works with input port is IMGI port.
                     *  mGroupID should be the index of the MCrpRsInfo.
                     */
                    MCrpRsInfo crop;
                    crop.mGroupID       = MIX_MAIN_GID_OUT;
                    crop.mCropRect.p_integral.x = 0; // position pixel, in integer
                    crop.mCropRect.p_integral.y = 0;
                    crop.mCropRect.p_fractional.x = 0; // always be 0
                    crop.mCropRect.p_fractional.y = 0;

                    switch(stage) {
                        case 0:
                            crop.mCropRect.s = inputBuffer->getImgSize();
                            crop.mResizeDst  = inputBuffer->getImgSize();
                        break;

                        case 1:
                            crop.mCropRect.s = inputRSZBuffer->getImgSize();
                            crop.mResizeDst  = inputRSZBuffer->getImgSize();
                        break;

                        default:
                            crop.mCropRect.s = mvMssBuf.at(stage).get()->getImgSize();
                            crop.mResizeDst  = mvMssBuf.at(stage).get()->getImgSize();
                    }

                    crop.mFrameGroup    = 0;
                    params.at(stage).mvCropRsInfo.push_back(crop);

                    MY_LOGD("main: srcCrop (x,y,w,h)=(%d,%d,%d,%d)",
                            crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
                            crop.mCropRect.s.w, crop.mCropRect.s.h);
                    MY_LOGD("main: dstSize (w,h)=(%d,%d)",
                            crop.mResizeDst.w, crop.mResizeDst.h);

                    if (stage == 0) {
                        /*destructor of SRZ3 and SRZ4*/
                        auto DeleteModuleInfo = [] (ModuleInfo* pModuleInfo) {
                            if (pModuleInfo != nullptr) {
                                if (pModuleInfo->moduleStruct != nullptr)
                                    delete static_cast<_SRZ_SIZE_INFO_*>(pModuleInfo->moduleStruct);
                                delete pModuleInfo;
                            }
                        };

                        /* apply SRZ3*/
                        if (mvTunParam.at(stage)->pFaceAlphaBuf) {
                            auto fillSRZ3 = [&]() -> ModuleInfo* {
                                // srz3 config
                                // ModuleInfo srz3_module;
                                ModuleInfo* p = new ModuleInfo();
                                p->moduleTag = EDipModule_SRZ3;
                                p->frameGroup=0;

                                _SRZ_SIZE_INFO_* pSrzParam = new _SRZ_SIZE_INFO_;

                                FACENR_IN_PARAMS in;
                                FACENR_OUT_PARAMS out;
                                IImageBuffer *facei = (IImageBuffer*)mvTunParam.at(stage)->pFaceAlphaBuf;
                                in.p2_in    = mConfig.buffSize;
                                in.face_map = facei->getImgSize();
                                calculateFACENRConfig(in, out);
                                *pSrzParam = out.srz3Param;
                                p->moduleStruct = reinterpret_cast<MVOID*>(pSrzParam);
                                MY_LOGD("SRZ3(in_w/h:%d/%d out_w/h:%d/%d crop_x/y:%d/%d crop_floatX/floatY:%d/%d crop_w/h:%lu/%lu)",
                                pSrzParam->in_w, pSrzParam->in_h, pSrzParam->out_w, pSrzParam->out_h,
                                pSrzParam->crop_x, pSrzParam->crop_y, pSrzParam->crop_floatX,
                                pSrzParam->crop_floatY, pSrzParam->crop_w, pSrzParam->crop_h);
                                return p;
                            };
                            pSRZ3module = decltype(pSRZ3module)(fillSRZ3(), DeleteModuleInfo);
                            if (pSRZ3module) {
                                ModuleInfo moduleInfo;
                                moduleInfo.moduleTag = EDipModule_SRZ3;
                                moduleInfo.moduleStruct = pSRZ3module->moduleStruct;
                                params.at(stage).mvModuleData.push_back(moduleInfo);

                                Input src;
                                src.mPortID = PORT_YNR_FACEI;
                                src.mBuffer = static_cast<IImageBuffer*>(mvTunParam.at(stage)->pFaceAlphaBuf);
                                params.at(stage).mvIn.push_back(src);
                                MY_LOGD("in:YNR_FACEI fmt:%x", static_cast<IImageBuffer*>(mvTunParam.at(stage)->pFaceAlphaBuf)->getImgFormat());
                            }
                        }
                        /* apply SRZ4*/
                        IImageBuffer* lcso = mConfig.inputLCSBuff;
                        {
                            Input src;
                            src.mPortID = PORT_LCEI;
                            src.mBuffer = lcso;
                            params.at(stage).mvIn.push_back(src);
                        }

                        auto fillSRZ4 = [&]() -> ModuleInfo* {
                            // srz4 config
                            // ModuleInfo srz4_module;
                            ModuleInfo* p = new ModuleInfo();
                            p->moduleTag  = EDipModule_SRZ4;
                            p->frameGroup=0;

                            _SRZ_SIZE_INFO_* pSrzParam = new _SRZ_SIZE_INFO_;

                            if (lcso) {
                                pSrzParam->in_w   = lcso->getImgSize().w;
                                pSrzParam->in_h   = lcso->getImgSize().h;
                                pSrzParam->crop_w = lcso->getImgSize().w;
                                pSrzParam->crop_h = lcso->getImgSize().h;
                            }
                            pSrzParam->out_w = mConfig.buffSize.w;
                            pSrzParam->out_h = mConfig.buffSize.h;

                            p->moduleStruct = reinterpret_cast<MVOID*> (pSrzParam);
                            MY_LOGD("SRZ4(in_w/h:%d/%d out_w/h:%d/%d crop_x/y:%d/%d crop_floatX/floatY:%d/%d crop_w/h:%lu/%lu)",
                            pSrzParam->in_w, pSrzParam->in_h, pSrzParam->out_w, pSrzParam->out_h,
                            pSrzParam->crop_x, pSrzParam->crop_y, pSrzParam->crop_floatX,
                            pSrzParam->crop_floatY, pSrzParam->crop_w, pSrzParam->crop_h);
                            return p;
                        };
                        pSRZ4module = decltype(pSRZ4module)(fillSRZ4(), DeleteModuleInfo);
                        if (pSRZ4module) {
                            ModuleInfo moduleInfo;
                            moduleInfo.moduleTag = EDipModule_SRZ4;
                            moduleInfo.moduleStruct = pSRZ4module->moduleStruct;
                            params.at(stage).mvModuleData.push_back(moduleInfo);

                            Input src;
                            src.mPortID = PORT_YNR_LCEI;
                            src.mBuffer = lcso;
                            params.at(stage).mvIn.push_back(src);
                            MY_LOGD("in:LCEI format:%x", lcso->getImgFormat());
                        }

                        /* generate dceso */
                        if (mConfig.hasDCE) {
                            NS3Av3::Buffer_Info info;
                            MSize   dcesoSize = 0;
                            MUINT32 dcesoFormat = 0;

                            if (!mpHalIsp->queryISPBufferInfo(info)) {
                                MY_LOGA("fail to get buffer info");
                                return MsnrErr_BadArgument;
                            }

                            if (info.DCESO_Param.bSupport) {
                                dcesoSize   = info.DCESO_Param.size;
                                dcesoFormat = info.DCESO_Param.format;
                                MY_LOGD("dceso wxh:%dx%d format:%x", dcesoSize.w, dcesoSize.h, dcesoFormat);

                                MBOOL isContinuous = MFALSE;
                                auto ret = ImageBufferUtils::getInstance().allocBuffer(
                                    mDcesoBuf, dcesoSize.w, dcesoSize.h, dcesoFormat, isContinuous);
                                if(CC_UNLIKELY(ret == MFALSE || mDcesoBuf.get() == nullptr)) {
                                    MY_LOGE("dceso allocating fail");
                                    return MsnrErr_BadArgument;
                                }
                                {
                                    Output p;
                                    p.mBuffer          = mDcesoBuf.get();
                                    p.mPortID          = PORT_DCESO;
                                    p.mTransform       = 0;
                                    p.mPortID.group    = 0; // always be 0
                                    params.at(stage).mvOut.push_back(p);
                                }
                            } else {
                                MY_LOGD("not support dce");
                            }
                        } else {
                            MY_LOGW("DCE is not on");
                        }
                    } // if stage == 0
                }
            }

            // enque driver
            {
                struct timeval current;
                gettimeofday(&current, NULL);
                params.at(stage).ExpectedEndTime = current;
                params.at(stage).mStreamTag = ENormalStreamTag_Vss;
                //
                qParam.at(stage).mpfnCallback = [](QParams& rParams)->MVOID
                {
                    if (CC_UNLIKELY(rParams.mpCookie == nullptr))
                        return;

                    Pass2Async* pAsync = static_cast<Pass2Async*>(rParams.mpCookie);
                    std::lock_guard<std::mutex> __l(pAsync->getLocker());
                    pAsync->setStatus(true);
                    pAsync->notifyOne();
                    MY_LOGD("MSF-P2 done(%d)", pAsync->index);
                };
                qParam.at(stage).mpfnEnQFailCallback = [](QParams& rParams)->MVOID
                {
                    if (CC_UNLIKELY(rParams.mpCookie == nullptr))
                        return;

                    Pass2Async* pAsync = static_cast<Pass2Async*>(rParams.mpCookie);
                    std::lock_guard<std::mutex> __l(pAsync->getLocker());
                    pAsync->setStatus(true);
                    pAsync->notifyOne();
                    MY_LOGE("MSF-P2 fail(%d)", pAsync->index);
                };
                vp2Async[stage].index = stage;
                qParam.at(stage).mpCookie = static_cast<void*>(&vp2Async[stage]);
                qParam.at(stage).mvFrameParams.push_back(params.at(stage));

                if (CC_UNLIKELY( ! mpNormalStream->enque(qParam.at(stage)) )) {
                    MY_LOGA("msf-p2 enque returns fail(%d)", stage);
                    err = MsnrErr_BadArgument;
                }
            }
        } // for loop

        {
            struct timeval startTime;
            gettimeofday(&startTime, NULL);

            while (vp2Async[0].getStatus() != true) {
                struct timeval currentTime, elapsedTime;
                gettimeofday(&currentTime, NULL);
                timersub(&currentTime, &startTime, &elapsedTime);

                int timeDiff = elapsedTime.tv_sec*1000 + elapsedTime.tv_usec/1000; //ms
                //MY_LOGI("Time Diff: %d diffs: %d diffus:%d", timeDiff, elapsedTime.tv_sec, elapsedTime.tv_usec);
                if (CC_UNLIKELY(timeDiff > HW_TIMEOUT_TIME*7)) {
                    MY_LOGA("MSF-P2 wait timeout: %d ms diff: %d ms", HW_TIMEOUT_TIME*7, timeDiff);
                }

                MY_LOGI("wait for MSF-P2");
                auto __l = vp2Async[0].uniqueLock();
                auto status = vp2Async[0].wait_for(std::move(__l), HW_RETRY_TIME /*ms*/);
                if (CC_UNLIKELY(status == std::cv_status::timeout)) {
                    MY_LOGI("MSF-P2 wait timeout, wait again");
                }
            }
            MY_LOGI("MSF-P2 done");
        }


        if (mNddDumpL1 == 1 || mNddDumpL2 == 1) {
            for (int i = 0; i < mvMsfBuf.size(); i++) {
                if (i == 0) {
                    // L0 output
                    std::string str = "lpnr-msfp2-L" + std::to_string(i) + "-out";
                    const char* pUserString = str.c_str();
                    NddDump(mvMsfBuf.at(i).get(), msfDumpHint.at(i), YUV_PORT_NULL, pUserString);
                } else if (mNddDumpL2 == 1) {
                    // L1~L6 output
                    std::string str = "lpnr-msfp2-L" + std::to_string(i) + "-out";
                    const char* pUserString = str.c_str();
                    NddDump(mvMsfBuf.at(i).get(), msfDumpHint.at(i), YUV_PORT_NULL, pUserString);
                }
            }
        }
    }
    //********************* P2-MDP **************************
    {
        // prepare tuning buffer
        prepareDCETunBuf();
    }
    {
        FILE_DUMP_NAMING_HINT p2DumpHint{};
        Pass2Async  p2Async;
        QParams     qParams;
        FrameParams params;
        PQParam     p2PqParam;
        DpPqParam   mdpPqDre;

        /* execute pass 2 operation */
        {
            //Tuning data
            params.mTuningData = static_cast<MVOID*>(mP2TunBuf.get()); // adding tuning data

            //NDD dump hint
            {
                generateP2NDDHint(mConfig.halMeta, p2DumpHint);
                IF_WRITE_DECISION_RECORD_INTO_FILE(true, decParm, DECISION_ATMS,
                    "%s DIP uses profile:%s(%d)",
                    ORDINAL(MSF_BUF_COUNT + 1),
                    getIspProfileName(static_cast<EIspProfile_T>(p2DumpHint.IspProfile)),
                    p2DumpHint.IspProfile);
                resParam.stageId = getIspStage(-1);  // -1 indicates DCE
                WRITE_EXECUTION_RESULT_INTO_FILE(resParam,
                    "%s LPNR DIP uses profile:%s(%d)",
                    ORDINAL(MSF_BUF_COUNT + 1),
                    getIspProfileName(static_cast<EIspProfile_T>(p2DumpHint.IspProfile)),
                    p2DumpHint.IspProfile);
                int64_t timestamp = 0; // set to 0 if not found.
#if MTK_CAM_DISPAY_FRAME_CONTROL_ON
                IMetadata::getEntry<int64_t>(mConfig.halMeta, MTK_P1NODE_FRAME_START_TIMESTAMP, timestamp);
#else
                IMetadata::getEntry<int64_t>(mConfig.appMeta, MTK_SENSOR_TIMESTAMP, timestamp);
#endif
                params.FrameNo    = p2DumpHint.FrameNo;
                params.RequestNo  = p2DumpHint.RequestNo;
                params.Timestamp  = timestamp;
                params.UniqueKey  = p2DumpHint.UniqueKey;
                params.IspProfile = p2DumpHint.IspProfile;
                params.SensorDev  = p2DumpHint.SensorDev;
                params.NeedDump   = 2;
            }
            // input
            {
                Input p;
                p.mBuffer          = mvMsfBuf.at(0).get();
                p.mPortID          = PORT_IMGI;
                p.mPortID.group    = 0; // always be 0
                params.mvIn.push_back(p);
            }
            // output: main yuv
            {
                Output p;
                p.mBuffer          = outputBuffer;
                p.mPortID          = MIX_PORT_OUT_MAIN;
                p.mPortID.group    = 0; // always be 0
                params.mvOut.push_back(p);

                // debug IMG3O
                if (mEnableDebugBuf || mNddDumpL1 ==1 || mNddDumpL2 == 1) {
                    MBOOL isContinuous = MFALSE;
                    MSize inputsize = mvMsfBuf.at(0)->getImgSize();
                    auto ret = ImageBufferUtils::getInstance().allocBuffer(
                        mDebugBuf, inputsize.w, inputsize.h, eImgFmt_MTK_YUYV_Y210,
                            isContinuous, 16);
                    if(CC_UNLIKELY(ret == MFALSE || mDebugBuf.get() == nullptr)) {
                        MY_LOGE("Debug Buffer allocating fail");
                    } else {
                        Output debugport;
                        debugport.mBuffer       = mDebugBuf.get();
                        debugport.mPortID       = PORT_IMG3O;
                        debugport.mPortID.group = 0; // always be 0
                        params.mvOut.push_back(debugport);
                    }
                }
                /**
                 *  Cropping info, only works with input port is IMGI port.
                 *  mGroupID should be the index of the MCrpRsInfo.
                 */
                MCrpRsInfo crop;
                crop.mGroupID       = MIX_MAIN_GID_OUT;
                crop.mCropRect.p_integral.x = 0; // position pixel, in integer
                crop.mCropRect.p_integral.y = 0;
                crop.mCropRect.p_fractional.x = 0; // always be 0
                crop.mCropRect.p_fractional.y = 0;
                crop.mCropRect.s.w  = outputBuffer->getImgSize().w;
                crop.mCropRect.s.h  = outputBuffer->getImgSize().h;
                crop.mResizeDst.w   = outputBuffer->getImgSize().w;
                crop.mResizeDst.h   = outputBuffer->getImgSize().h;
                crop.mFrameGroup    = 0;
                params.mvCropRsInfo.push_back(crop);

                MY_LOGD("main: srcCrop (x,y,w,h)=(%d,%d,%d,%d)",
                        crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
                        crop.mCropRect.s.w, crop.mCropRect.s.h);
                MY_LOGD("main: dstSize (w,h)=(%d,%d)",
                        crop.mResizeDst.w, crop.mResizeDst.h);

                // p2-mdp direct link
                /* ask MDP to generate CA-LTM histogram */
                bool bEnCALTM = !!gSupportDRE && (mConfig.isMasterIndex || mConfig.isPhysical);
                bool bIsMdpPort = (p.mPortID == PORT_WDMAO) || (p.mPortID == PORT_WROTO);
                if ( bEnCALTM && bIsMdpPort ) {
                    int iso = 0;
                    IMetadata::getEntry<MINT32>(mConfig.appDynamic, MTK_SENSOR_SENSITIVITY, iso);
                    EIspProfile_T profile = getIspProfile(-1);//EIspProfile_Capture_DCE;
                    bool bGeneratedMdpParam = generateMdpDrePQParam(
                            mdpPqDre,
                            iso,
                            mConfig.openId,
                            mConfig.halMeta,// use the first metadata for DRE, due to
                                                   // others use main metadata to apply DRE.
                            profile
                            );
                    if ( ! bGeneratedMdpParam ) {
                        MY_LOGE("generate DRE DRE param failed");
                    } else {
                        MY_LOGD("generate DRE histogram.");
                        if (p.mPortID == PORT_WDMAO) {
                            p2PqParam.WDMAPQParam = static_cast<void*>(&mdpPqDre);
                        } else if (p.mPortID == PORT_WROTO) {
                            p2PqParam.WROTPQParam = static_cast<void*>(&mdpPqDre);
                        } else {
                            MY_LOGD("attach failed, the port is neither WDMAO nor WROTO");
                        }

                        ExtraParam extraP;
                        extraP.CmdIdx = EPIPE_MDP_PQPARAM_CMD;
                        extraP.moduleStruct = static_cast<void*>(&p2PqParam);
                        params.mvExtraParam.push_back(extraP);
                    }
                } else {
                    MY_LOGD("DRE disabled or port is not MDP port");
                }
            }
        }

        // enque driver
        {
            struct timeval current;
            gettimeofday(&current, NULL);
            params.ExpectedEndTime = current;
            params.mStreamTag = ENormalStreamTag_Vss;
            //
            qParams.mpfnCallback = [](QParams& rParams)->MVOID
            {
                if (CC_UNLIKELY(rParams.mpCookie == nullptr))
                    return;

                Pass2Async* pAsync = static_cast<Pass2Async*>(rParams.mpCookie);
                std::lock_guard<std::mutex> __l(pAsync->getLocker());
                pAsync->setStatus(true);
                pAsync->notifyOne();
                MY_LOGD("P2-MDP done");
            };
            qParams.mpfnEnQFailCallback = [](QParams& rParams)->MVOID
            {
                if (CC_UNLIKELY(rParams.mpCookie == nullptr))
                    return;

                Pass2Async* pAsync = static_cast<Pass2Async*>(rParams.mpCookie);
                std::lock_guard<std::mutex> __l(pAsync->getLocker());
                pAsync->setStatus(true);
                pAsync->notifyOne();
                MY_LOGE("P2-MDP fail");
            };
            qParams.mpCookie = static_cast<void*>(&p2Async);
            qParams.mvFrameParams.push_back(params);

            if (CC_UNLIKELY( ! mpNormalStream->enque(qParams) )) {
                MY_LOGA("P2-MDP enque returns fail");
                err = MsnrErr_BadArgument;
            }
        }
        // waiting for p2-mdp finish
        {
            struct timeval startTime;
            gettimeofday(&startTime, NULL);

            while (p2Async.getStatus() != true) {
                struct timeval currentTime, elapsedTime;
                gettimeofday(&currentTime, NULL);
                timersub(&currentTime, &startTime, &elapsedTime);

                int timeDiff = elapsedTime.tv_sec*1000 + elapsedTime.tv_usec/1000; //ms
                if (CC_UNLIKELY(timeDiff > HW_TIMEOUT_TIME)) {
                    MY_LOGA("P2-MDP wait timeout: %d ms diff: %d ms", HW_TIMEOUT_TIME, timeDiff);
                }

                MY_LOGI("wait for P2-MDP");
                auto __l = p2Async.uniqueLock();
                auto status = p2Async.wait_for(std::move(__l), HW_RETRY_TIME /*ms*/);
                if (CC_UNLIKELY(status == std::cv_status::timeout)) {
                    MY_LOGI("P2-MDP wait timeout, wait again");
                }
            }
            MY_LOGI("P2-MDP done");
        }

        if (mNddDumpL1 == 1) {
            NddDump(outputBuffer, p2DumpHint, YUV_PORT_WDMAO, nullptr, true);
            if (mEnableDebugBuf || mNddDumpL1 ==1 || mNddDumpL2 == 1) {
                std::string str = "lpnr-p2-out";
                const char* pUserString = str.c_str();
                NddDump(mDebugBuf.get(), p2DumpHint, YUV_PORT_IMG3O, pUserString);
            }
        }
    }

lbExit:
    return err;
}

enum MsnrErr MsnrCore::makeDebugInfo(IMetadata* metadata)
{
    CAM_ULOGM_APILIFE();
    bool haveExif = false;
    {
        IMetadata::IEntry entry = metadata->entryFor(MTK_HAL_REQUEST_REQUIRE_EXIF);
        if (! entry.isEmpty()  && entry.itemAt(0, Type2Type<MUINT8>()) )
                haveExif = true;
    }
    //
    if (haveExif)
    {
        IMetadata::Memory memory_dbgInfo;
        memory_dbgInfo.resize(sizeof(DEBUG_RESERVEA_INFO_T));
        DEBUG_RESERVEA_INFO_T& dbgInfo =
            *reinterpret_cast<DEBUG_RESERVEA_INFO_T*>(memory_dbgInfo.editArray());
        ssize_t idx = 0;
#define addPair(debug_info, index, id, value)           \
        do{                                             \
            debug_info.Tag[index].u4FieldID = (0x01000000 | id); \
            debug_info.Tag[index].u4FieldValue = value; \
            index++;                                    \
        } while(0)
        //
        addPair(dbgInfo , idx , LPCNR_ENABLE, 1);
        //
#undef addPair
        //
        IMetadata exifMeta;
        // query from hal metadata first
        {
            IMetadata::IEntry entry = metadata->entryFor(MTK_3A_EXIF_METADATA);
            if (! entry.isEmpty() )
                exifMeta = entry.itemAt(0, Type2Type<IMetadata>());
        }
        // update
        IMetadata::IEntry entry_key(MTK_POSTNR_EXIF_DBGINFO_NR_KEY);
        entry_key.push_back(DEBUG_EXIF_MID_CAM_RESERVE1, Type2Type<MINT32>());
        exifMeta.update(entry_key.tag(), entry_key);
        //
        IMetadata::IEntry entry_data(MTK_POSTNR_EXIF_DBGINFO_NR_DATA);
        entry_data.push_back(memory_dbgInfo, Type2Type<IMetadata::Memory>());
        exifMeta.update(entry_data.tag(), entry_data);
        //
        IMetadata::IEntry entry_exif(MTK_3A_EXIF_METADATA);
        entry_exif.push_back(exifMeta, Type2Type<IMetadata>());
        metadata->update(entry_exif.tag(), entry_exif);
    }
    else
    {
        MY_LOGD("no need to dump exif");
    }
    //
    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::checkParams(IMsnr::ConfigParams const& params)
{
    CAM_ULOGM_FUNCLIFE();

    enum MsnrErr err = MsnrErr_Ok;

    if(CC_UNLIKELY(params.appMeta == nullptr)) {
        MY_LOGE("appMeta is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.halMeta == nullptr)) {
        MY_LOGE("halMeta is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.appDynamic == nullptr)) {
        MY_LOGE("appDynamic is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.inputBuff == nullptr)) {
        MY_LOGE("inputBuff is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.inputRSZBuff == nullptr)) {
        MY_LOGE("inputRSZBuff is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.inputLCSBuff == nullptr)) {
        MY_LOGE("inputLCSBuff is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.inputLCESHOBuff == nullptr)) {
        MY_LOGE("inputLCESHOBuff is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

    if(CC_UNLIKELY(params.outputBuff == nullptr)) {
        MY_LOGE("outputBuff is null");
        err = MsnrErr_BadArgument;
        goto lbExit;
    }

lbExit:
    return err;
}

enum MsnrErr MsnrCore::allocateMssBuf()
{
    CAM_ULOGM_FUNCLIFE();

    mvMssBuf.resize(MSS_BUF_COUNT);
    int width = mConfig.buffSize.w;
    int height = mConfig.buffSize.h;

    for(int i = 0; i < MSS_BUF_COUNT; i++) {
        //update width and height: S(n+1) = (S(n) + 3)/4*2
        if (i > 0) {
            width = (width+3)/4*2;
            height = (height+3)/4*2;
        }
        // s0 and s1 are already provided
        if (i < MSS_RAW_IN_START)
            continue;

        // final buffer format should be unpacked yuv
        MBOOL isContinuous = MFALSE;
        auto ret = ImageBufferUtils::getInstance().allocBuffer(
            mvMssBuf.at(i), width, height, eImgFmt_MTK_YUV_P010, isContinuous, HW_BYTE_ALIGNMENT);
        if(CC_UNLIKELY(ret == MFALSE || mvMssBuf.at(i).get() == nullptr)) {
            MY_LOGE("MSSBuf vector allocating fail");
            return MsnrErr_BadArgument;
        }
    }
    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::deallocateMssBuf()
{
    CAM_ULOGM_FUNCLIFE();

    for(int i = 0; i < mvMssBuf.size(); i++) {
        // s0 and s1 are already provided
        if (i < MSS_RAW_IN_START)
            continue;

        ImageBufferUtils::getInstance().deallocBuffer(mvMssBuf.at(i).get());
        mvMssBuf.at(i) = nullptr;
    }
    mvMssBuf.clear();
    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::allocateMsfBuf()
{
    CAM_ULOGM_FUNCLIFE();

    mvMsfBuf.resize(MSF_BUF_COUNT);
    int width = mConfig.buffSize.w;
    int height = mConfig.buffSize.h;

    for(int i = 0; i < MSF_BUF_COUNT; i++) {
        //update width and height: S(n+1) = (S(n) + 3)/4*2
        if (i > 0) {
            width = (width+3)/4*2;
            height = (height+3)/4*2;
        }

        // final buffer format should be unpacked yuv
        MBOOL isContinuous = MFALSE;
        auto ret = ImageBufferUtils::getInstance().allocBuffer(
            mvMsfBuf.at(i), width, height, eImgFmt_MTK_YUV_P010, isContinuous, HW_BYTE_ALIGNMENT);
        if(CC_UNLIKELY(ret == MFALSE || mvMsfBuf.at(i).get() == nullptr)) {
            MY_LOGE("MSFBuf vector allocating fail");
            return MsnrErr_BadArgument;
        }
    }
    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::deallocateMsfBuf()
{
    CAM_ULOGM_FUNCLIFE();

    for(int i = 0; i < mvMsfBuf.size(); i++) {
        ImageBufferUtils::getInstance().deallocBuffer(mvMsfBuf.at(i).get());
        mvMsfBuf.at(i) = nullptr;
    }
    mvMsfBuf.clear();
    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::deallocateDebugBuf()
{
    CAM_ULOGM_FUNCLIFE();

    ImageBufferUtils::getInstance().deallocBuffer(mDebugBuf.get());
    mDebugBuf = nullptr;
    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::prepareTunBuf()
{
    CAM_ULOGM_FUNCLIFE();

    MsnrErr err = MsnrErr_Ok;
    DecisionParam decParm = {};
    RecorderUtils::initDecisionParam(mConfig.openId, *mConfig.halMeta, &decParm);

    // this thread operation depends on the result of msf allocation result
    if (CC_LIKELY( mMsfAllocThread.valid() )) {
        if (CC_UNLIKELY( mMsfAllocThread.get() != 0 )) {
            MY_LOGA("MSF allocate working buffer failed");
            return MsnrErr_BadArgument;
        }
    }

    if (CC_UNLIKELY(mpHalIsp.get() == nullptr)) {
        MY_LOGE("get IHalIsp failed");
        return MsnrErr_BadArgument;
    }

    EIspProfile_T profile = (EIspProfile_T)0; // which will be set later.
    MetaSet_T inMetaSet(*(mConfig.appMeta), *(mConfig.halMeta));
    MetaSet_T outMetaSet;

    // set MTK_ISP_P2_IN_IMG_FMT to 1 (1 means YUV->YUV)
    IMetadata::setEntry<MINT32>(&inMetaSet.halMeta, MTK_ISP_P2_IN_IMG_FMT, 1);

    /**
     * Retrieve MSF tuning data
     */
    //mvTunBuf
    mvTunBuf.resize(P2_TUNING_COUNT);
    mvTunParam.resize(P2_TUNING_COUNT);
    for (size_t stage = 0 ; stage < mvTunBuf.size(); stage++) {
        int32_t ispStage = getIspStage(stage);
        IMetadata::setEntry<MINT32>(&inMetaSet.halMeta, MTK_ISP_STAGE, ispStage);

        profile = getIspProfile(stage);
        mvTunBuf.at(stage) = std::unique_ptr<char[]>(new char[mRegTableSize]{0});
        if (mvTunBuf.at(stage).get() == nullptr) {
            MY_LOGE("allocate tuning buffer fail(%zu)", stage);
            return MsnrErr_BadArgument;
        }

        /* assign tuning buffers */
        mvTunParam.at(stage) = std::unique_ptr<TuningParam>(new TuningParam());
        if (mvTunParam.at(stage).get() == nullptr) {
            MY_LOGE("allocate tuning param fail(%zu)", stage);
            return MsnrErr_BadArgument;
        }
        mvTunParam.at(stage)->pRegBuf = static_cast<void*>(mvTunBuf.at(stage).get());

        /* last frame, s0 apply lce*/
        if (stage == 0) {
            // apply lce
            IImageBuffer* lcso   = mConfig.inputLCSBuff;
            IImageBuffer* lcesho = mConfig.inputLCESHOBuff;
            MY_LOGD("lcsoBuf=%p lceshoBuf=%p", lcso, lcesho);

            if(lcso && lcesho) {
                mvTunParam.at(stage)->pLcsBuf    = reinterpret_cast<void*>(lcso);
                mvTunParam.at(stage)->pLceshoBuf = reinterpret_cast<void*>(lcesho);
            }
            // generate lce2caltm data
            if(mConfig.bufLce2Caltm != nullptr){
                MY_LOGD("L2C VA: 0x%p", mConfig.bufLce2Caltm);
                mvTunParam.at(stage)->pLce4CALTM = reinterpret_cast<void*>(mConfig.bufLce2Caltm);
            } else {
                MY_LOGW("L2C is nullptr");
            }

            // set FOC crop and resize info for lce
            if (mConfig.isVSDoFMode == MTRUE) {
                IMetadata::setEntry<MINT32>(&inMetaSet.appMeta, MTK_CONTROL_CAPTURE_P2_RAW_CROP_RESIZE_ENABLE_CUSTOMIZE, 1);
                IMetadata::setEntry<MSize>(&inMetaSet.appMeta, MTK_CONTROL_CAPTURE_P2_RESIZER_SIZE_CUSTOMIZE, mConfig.fovBufferSize);
                IMetadata::setEntry<MRect>(&inMetaSet.appMeta, MTK_CONTROL_CAPTURE_P2_CROP_REGION_CUSTOMIZE, mConfig.fovCropRegion);
                MY_LOGI("update fovInfo, region(x, y, w, h):(%d, %d, %d, %d), dstSize:(%d, %d)",
                        mConfig.fovCropRegion.p.x, mConfig.fovCropRegion.p.y,
                        mConfig.fovCropRegion.s.w, mConfig.fovCropRegion.s.h,
                        mConfig.fovBufferSize.w, mConfig.fovBufferSize.h);
            }
            /* store dual sync info if it's needed in next sub request*/
            if (mConfig.isMasterIndex) {
                // if request is from master sensor
                auto phelper = TuningSyncHelper::getInstance(LOG_TAG);
                std::shared_ptr<char> pSyncData;
                NS3Av3::Buffer_Info info;

                if (!mpHalIsp->queryISPBufferInfo(info)) {
                    MY_LOGE("fail to get buffer info");
                    return MsnrErr_BadArgument;
                }

                std::string key = "";
                if(!TuningSyncHelper::generateKey(inMetaSet.halMeta, &key)) {
                    MY_LOGE("fail to generate key");
                    return MsnrErr_BadArgument;
                }

                if(!phelper->createDualSyncInfo(key, info.u4DualSyncInfoSize, &pSyncData)) {
                    MY_LOGE("fail to create sync info");
                    return MsnrErr_BadArgument;
                }

                MY_LOGD("set dualsyncinfo key:%s size:%d addr:%p", key.c_str(), info.u4DualSyncInfoSize, pSyncData.get());
                mvTunParam.at(stage)->bSlave = false;
                mvTunParam.at(stage)->pDualSynInfo = reinterpret_cast<void*>(pSyncData.get());
            } else if (!mConfig.isMasterIndex) {
                // if request is from slave sensor
                auto phelper = TuningSyncHelper::getInstance(LOG_TAG);
                std::shared_ptr<char> pSyncedData;

                std::string key = "";
                if(!TuningSyncHelper::generateKey(inMetaSet.halMeta, &key)) {
                    MY_LOGE("fail to generate key");
                    return MsnrErr_BadArgument;
                }

                if(!phelper->getDualSyncInfo(key, &pSyncedData)) {
                    MY_LOGE("fail to get sync info, skip syncing");
                } else {
                    MY_LOGD("get dualsyncinfo key:%s addr:%p", key.c_str(), pSyncedData.get());
                    mvTunParam.at(stage)->bSlave = true;
                    mvTunParam.at(stage)->pDualSynInfo = reinterpret_cast<void*>(pSyncedData.get());
                    IF_WRITE_DECISION_RECORD_INTO_FILE(true, decParm, DECISION_FEATURE,
                        "sub cam applies main cam's dual sync info");
                }
            }
        }

        /* update Metadata */
        // USE resize raw-->set PGN 0
        IMetadata::setEntry<MUINT8>(&inMetaSet.halMeta, MTK_3A_PGN_ENABLE, MTRUE);
        IMetadata::setEntry<MUINT8>(&inMetaSet.halMeta, MTK_3A_ISP_PROFILE, profile);
        MY_LOGI("stage(%zu) profile: %d",stage ,profile);
        IMetadata::setEntry<MINT32>(&inMetaSet.halMeta, MTK_MSF_SCALE_INDEX, stage);

        int width  = (stage == 0)? mConfig.buffSize.w : mvMsfBuf.at(stage)->getImgSize().w;
        int height = (stage == 0)? mConfig.buffSize.h : mvMsfBuf.at(stage)->getImgSize().h;
        MINT32 resizeYUV = (width & 0x0000FFFF) | ((height & 0x0000FFFF)<<16);
        MY_LOGD("calc resize yuv for msf SLK, w = %x, h = %x, calc = %x", width, height, resizeYUV);
        IMetadata::setEntry<MINT32>(&inMetaSet.halMeta, MTK_ISP_P2_IN_IMG_RES_REVISED, resizeYUV);

        /* Before setIsp we check whether execute re-install flow */
        if (mOnlineTuning == DeviceModeOn && stage == 0) {
            auto LcsBuff = mConfig.inputLCSBuff;
            if (LcsBuff == nullptr)
                return MsnrErr_BadArgument;

            MY_LOGD("Reinstall msnr lcso");
            FileReadRule rule {};
            constexpr int reqNum = 0;
            int sensorId = mConfig.openId;
            rule.getFile_LCSO(reqNum, "MSF_0", LcsBuff, "MsnrStage0", sensorId);
        }

        /* setIsp to retrieve tuning buffer */
        auto ret = mpHalIsp->setP2Isp(0, inMetaSet, mvTunParam.at(stage).get(), nullptr /*outMetaSet*/);
        if(0 > ret) {
            MY_LOGE("tuning buffer initing fail(%zu,%d)", stage, ret);
        } else {
            MY_LOGD("tuning buffer is inited(%zu)", stage);
        }

        if (stage == 0 && (mNddDumpL1 == 1 || mNddDumpL2 == 1
                || (mP2Dump == 1 && mCapPipeDump == 1))) {
            FILE_DUMP_NAMING_HINT hint{};
            char fileName[256] = "";
            generateNDDHintFromMetadata(hint, &inMetaSet.halMeta);

            auto LcsBuff = mConfig.inputLCSBuff;
            if (LcsBuff == nullptr)
                return MsnrErr_BadArgument;

            extract(&hint, LcsBuff);
            genFileName_LCSO(fileName, sizeof(fileName), &hint, "MsnrStage0");
            LcsBuff->saveToFile(fileName);
            MY_LOGD("Dump LCEI: %s", fileName);
        }
    }

lbExit:
    return err;
}

enum MsnrErr MsnrCore::prepareDCETunBuf()
{
    CAM_ULOGM_FUNCLIFE();

    MsnrErr err = MsnrErr_Ok;

    {
        MetaSet_T inMetaSet(*(mConfig.appMeta), *(mConfig.halMeta));
        MetaSet_T outMetaSet;
        size_t stage = -1; // DCE
        EIspProfile_T profile = getIspProfile(stage);//EIspProfile_Capture_DCE ;
        int32_t ispStage = getIspStage(stage);
        IMetadata::setEntry<MINT32>(&inMetaSet.halMeta, MTK_ISP_STAGE, ispStage);

        mP2TunBuf = std::unique_ptr<char[]>(new char[mRegTableSize]{0});
        if (mP2TunBuf.get() == nullptr) {
            MY_LOGE("allocate afterblend tuning buffer fail");
            return MsnrErr_BadArgument;
        }
        /* assign tuning buffers */
        TuningParam tuningParam;
        tuningParam.pRegBuf = static_cast<void*>(mP2TunBuf.get());
        // apply dce
        if (mConfig.hasDCE) {
            MINT32 p1Magic = -1;
            IMetadata::getEntry<MINT32>(&inMetaSet.halMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, p1Magic);
            MY_LOGD("DCESO i4DcsMagicNo=%d, dcesoBuf=%p", p1Magic, mDcesoBuf.get());

            if (p1Magic >= 0 && mDcesoBuf.get()) {
                tuningParam.pDcsBuf = mDcesoBuf.get();
                tuningParam.i4DcsMagicNo = p1Magic;
            }
        } else {
            MY_LOGW("DCE is not on");
        }
        // seting metadata
        IMetadata::setEntry<MUINT8>(&inMetaSet.halMeta, MTK_3A_PGN_ENABLE, MTRUE);
        IMetadata::setEntry<MUINT8>(&inMetaSet.halMeta, MTK_3A_ISP_PROFILE, profile);
        MY_LOGI("afterblend profile: %d",profile);
        IMetadata::setEntry<MINT32>(&inMetaSet.halMeta, MTK_ISP_P2_IN_IMG_FMT, 1);

        /* setIsp to retrieve tuning buffer */
        if (CC_UNLIKELY(mpHalIsp.get() == nullptr)) {
            MY_LOGE("get IHalIsp failed");
            return MsnrErr_BadArgument;
        }
        auto ret = mpHalIsp->setP2Isp(0, inMetaSet, &tuningParam, nullptr /*outMetaSet*/);
        if(0 > ret) {
            MY_LOGE("tuning buffer initing fail(%d)", ret);
        } else {
            MY_LOGD("afterblend tuning buffer is inited");
        }
    }

lbExit:
    return err;
}

enum MsnrErr MsnrCore::releaseTunBuf()
{
    CAM_ULOGM_FUNCLIFE();

    if (mvTunBuf.size() != mvTunParam.size()) {
        MY_LOGE("size is different(%zu,%zu)", mvTunBuf.size(), mvTunParam.size());
    }
    for (size_t stage = 0 ; stage < mvTunBuf.size(); stage++) {
        mvTunBuf.at(stage) = nullptr;
        mvTunParam.at(stage) = nullptr;
    }
    mvTunBuf.clear();
    mvTunParam.clear();
    mP2TunBuf = nullptr;

    if (mDcesoBuf.get()) {
        ImageBufferUtils::getInstance().deallocBuffer(mDcesoBuf.get());
        mDcesoBuf = nullptr;
    }

    return MsnrErr_Ok;
}

enum MsnrErr MsnrCore::generateMsfNDDHint(const IMetadata* pInHalMeta, int stage, FILE_DUMP_NAMING_HINT& hint)
{
    CAM_ULOGM_FUNCLIFE();

    enum MsnrErr err = MsnrErr_Ok;

    memset(&hint, 0 , sizeof(FILE_DUMP_NAMING_HINT));

    generateNDDHintFromMetadata(hint, pInHalMeta);

    hint.IspProfile = static_cast<int>(getIspProfile(stage));

    extract_by_SensorOpenId(&hint, mConfig.openId);

lbExit:
    return err;
}

enum MsnrErr MsnrCore::generateMssNDDHint(const IMetadata* pInHalMeta, FILE_DUMP_NAMING_HINT& hint)
{
    CAM_ULOGM_FUNCLIFE();

    enum MsnrErr err = MsnrErr_Ok;

    memset(&hint, 0 , sizeof(FILE_DUMP_NAMING_HINT));

    generateNDDHintFromMetadata(hint, pInHalMeta);

    //hint.IspProfile = EIspProfile_Capture_Before_Blend;

    extract_by_SensorOpenId(&hint, mConfig.openId);

lbExit:
    return err;
}

enum MsnrErr MsnrCore::generateP2NDDHint(const IMetadata* pInHalMeta, FILE_DUMP_NAMING_HINT& hint)
{
    CAM_ULOGM_FUNCLIFE();

    enum MsnrErr err = MsnrErr_Ok;

    memset(&hint, 0 , sizeof(FILE_DUMP_NAMING_HINT));

    generateNDDHintFromMetadata(hint, pInHalMeta);

    hint.IspProfile = getIspProfile(-1);//EIspProfile_Capture_DCE;

    extract_by_SensorOpenId(&hint, mConfig.openId);

lbExit:
    return err;
}

bool MsnrCore::generateMdpDrePQParam(
        DpPqParam&          rMdpPqParam,
        int                 iso,
        int                 openId,
        const IMetadata*    pMetadata,
        EIspProfile_T       ispProfile)
{
    CAM_ULOGM_FUNCLIFE();

    // dump info
    MINT32 uniqueKey = 0;
    MINT32 frameNum  = 0;
    MINT32 magicNum  = 0;
    MINT32 lv_value  = 0;

    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
        MY_LOGW("get MTK_PIPELINE_UNIQUE_KEY failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_FRAME_NUMBER, frameNum)) {
        MY_LOGW("get MTK_PIPELINE_FRAME_NUMBER failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_REAL_LV, lv_value)) {
        MY_LOGW("get MTK_REAL_LV failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum)) {
        MY_LOGW("get MTK_P1NODE_PROCESSOR_MAGICNUM failed, cannot generate DRE's tuning "\
                "data, won't apply");
        return false;
    }
    if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_UNIQUE_KEY, uniqueKey)) {
        MY_LOGD("get MTK_PIPELINE_DUMP_UNIQUE_KEY, update uniqueKey to %d", uniqueKey);
    }

    /* check ISO */
    if (__builtin_expect( iso <= 0, false )) {
        MY_LOGE("ISO information is wrong, set to 0");
        iso = 0;
    }

    /* PQ param */
    rMdpPqParam.enable     |= PQ_DRE_EN;
    rMdpPqParam.scenario    = MEDIA_ISP_CAPTURE;

    /* ISP param */
    DpIspParam  &ispParam = rMdpPqParam.u.isp;
    //
    ispParam.timestamp  = uniqueKey;
    ispParam.requestNo  = 0; // use 0 as MDP generating histogram
    ispParam.frameNo    = 1000; // use 1000 as MDP generating histogram
    ispParam.iso        = static_cast<uint32_t>(iso);
    ispParam.lensId     = static_cast<uint32_t>(openId);
    ispParam.LV         = static_cast<int>(lv_value);
    ispParam.ispTimesInfo = ISP_FIRST_RSZ_ETC_DISABLE;

    /* DRE param */
    DpDREParam &dreParam = rMdpPqParam.u.isp.dpDREParam;
    //
    dreParam.cmd        = DpDREParam::Cmd::Initialize | DpDREParam::Cmd::Generate;
    dreParam.buffer     = nullptr; // no need buffer
    // get user id from meta, default use frameNum
    MINT64 userId = 0;
    if(IMetadata::getEntry<MINT64>(pMetadata, MTK_FEATURE_CAP_PQ_USERID, userId))
        dreParam.userId     = userId;
    else
        dreParam.userId     = static_cast<unsigned long long>(frameNum);

    /* idx0: void*, buffer address, idx1: int, NVRAM index */
    const size_t I_BUFFER = 0, I_NVRAMIDX = 1;
    std::tuple<void*, int> nvramData = getTuningTupleFromNvram < NSIspTuning::EModule_CA_LTM>(
            pMetadata,
            openId,
            0, // no need index
            magicNum,
            static_cast<MINT32>( ispProfile )
            );

    dreParam.p_customSetting = std::get<I_BUFFER>   (nvramData);
    dreParam.customIndex     = std::get<I_NVRAMIDX> (nvramData);

    MY_LOGD("generate DREParam with userId: %llx, iso: %d, sensorId: %d, magicNum: %d",
            dreParam.userId, iso, openId, magicNum);
    return true;
}

inline EIspProfile_T MsnrCore::getIspProfile(int stage)
{
    CAM_ULOGM_FUNCLIFE();

    // ISP Profile mapper query
    auto fnQueryISPProfile = [&](const EProfileMappingStages& profileStage, EIspProfile_T defaultValue = EIspProfile_Capture) -> EIspProfile_T {
        EIspProfile_T ispProfile;
        if(!mpISPProfileMapper->mappingProfile(mMappingKey, profileStage, ispProfile))
        {
            MY_LOGW("Failed to get ISP Profile, stage:%d, use EIspProfile_Capture as default", profileStage);
            ispProfile = defaultValue;
        }
        return ispProfile;
    };

    switch(stage) {
        case -1:
            return fnQueryISPProfile(eStage_MSNR_DCE, EIspProfile_Capture_DCE);//EIspProfile_Capture_DCE;
        break;
        case 0:
            return fnQueryISPProfile(eStage_MSNR_MSF_0, EIspProfile_Capture_MSF_0);//EIspProfile_Capture_MSF_0;
        break;
        case 1:
            return fnQueryISPProfile(eStage_MSNR_MSF_1, EIspProfile_Capture_MSF_1);//EIspProfile_Capture_MSF_1;
        break;
        case 2:
            return fnQueryISPProfile(eStage_MSNR_MSF_2, EIspProfile_Capture_MSF_2);//EIspProfile_Capture_MSF_2;
        break;
        case 3:
            return fnQueryISPProfile(eStage_MSNR_MSF_3, EIspProfile_Capture_MSF_3);//EIspProfile_Capture_MSF_3;
        break;
        case 4:
            return fnQueryISPProfile(eStage_MSNR_MSF_4, EIspProfile_Capture_MSF_4);//EIspProfile_Capture_MSF_4;
        break;
        case 5:
            return fnQueryISPProfile(eStage_MSNR_MSF_5, EIspProfile_Capture_MSF_5);//EIspProfile_Capture_MSF_5;
        break;
        case 6:
            return fnQueryISPProfile(eStage_MSNR_MSF_6, EIspProfile_Capture_MSF_6);//EIspProfile_Capture_MSF_6;
        break;
        default:
            MY_LOGE("not known stage:%d", stage);
            return static_cast<EIspProfile_T>(MSNR_ISP_PROFILE_ERROR);
    }
}

inline int32_t MsnrCore::getIspStage(int32_t iterationId)
{
    switch(iterationId) {
        case -1:
            return eStage_MSNR_DCE;
        break;
        case 0:
            return eStage_MSNR_MSF_0;
        break;
        case 1:
            return eStage_MSNR_MSF_1;
        break;
        case 2:
            return eStage_MSNR_MSF_2;
        break;
        case 3:
            return eStage_MSNR_MSF_3;
        break;
        case 4:
            return eStage_MSNR_MSF_4;
        break;
        case 5:
            return eStage_MSNR_MSF_5;
        break;
        case 6:
            return eStage_MSNR_MSF_6;
        break;
        default:
            MY_LOGW("not known stage:%d", iterationId);
            return -1;
    }
}

void MsnrCore::generateNDDHintFromMetadata(FILE_DUMP_NAMING_HINT& hint, const IMetadata* pMetadata)
{
    CAM_ULOGM_FUNCLIFE();

    // dump info
    MINT32 uniqueKey = 0;
    MINT32 frameNum  = 0;
    MINT32 requestNo = 0;

    extract(&hint, pMetadata);

    if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_UNIQUE_KEY, uniqueKey)) {
        MY_LOGD("get MTK_PIPELINE_DUMP_UNIQUE_KEY, update uniqueKey to %d", uniqueKey);
        hint.UniqueKey = uniqueKey;
        if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_FRAME_NUMBER, frameNum)) {
            MY_LOGD("get MTK_PIPELINE_DUMP_FRAME_NUMBER, update frameNum to %d", frameNum);
            hint.FrameNo = frameNum;
        }
        if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_REQUEST_NUMBER, requestNo)) {
            MY_LOGD("get MTK_PIPELINE_DUMP_REQUEST_NUMBER, update requestNo to %d", requestNo);
            hint.RequestNo = requestNo;
        }
    }
}

void
MsnrCore::
NddDump(IImageBuffer* buff, FILE_DUMP_NAMING_HINT hint, YUV_PORT type, const char *pUserString, bool bPrintSensor)
{
    CAM_ULOGM_FUNCLIFE();
    // dump input buffer
    char                      fileName[512] = "";
    //
    MUINT8 ispProfile = NSIspTuning::EIspProfile_Capture;

    if(!buff) {
        MY_LOGE("buff is nullptr, dump fail");
        return;
    }

    // IspProfile
    hint.IspProfile = ispProfile; //EIspProfile_Capture;
    // Extract buffer information and fill up file name;
    extract(&hint, buff);
    //
    hint.BufWidth  = ALIGN(buff->getImgSize().w, HW_BYTE_ALIGNMENT);
    hint.BufHeight = ALIGN(buff->getImgSize().h, HW_BYTE_ALIGNMENT);
    //
    if (bPrintSensor == false) {
        hint.SensorDev = -1;
    }
    genFileName_YUV(fileName, sizeof(fileName), &hint, type, pUserString);
    buff->saveToFile(fileName);

    return;
}

