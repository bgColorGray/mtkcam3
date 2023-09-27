/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2018. All rights reserved.
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
#define LOG_TAG "MfllCore/Mfb"

#include "MfllCore.h"
#include "MfllMfb.h"
#include <mtkcam3/feature/mfnr/MfllLog.h>
#include <mtkcam3/feature/mfnr/MfllProperty.h>
#include "MfllFeatureDump.h"
#include "MfllIspProfiles.h"
#include "MfllOperationSync.h"

// MTKCAM
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/drv/iopipe/SImager/IImageTransform.h> // IImageTransform
#include <mtkcam/drv/def/Dip_Notify_datatype.h> // _SRZ_SIZE_INFO_
#include <mtkcam/aaa/INvBufUtil.h> // new NVRAM mechanism
#include <mtkcam/utils/mapping_mgr/cam_idx_mgr.h> // new NVRAM mechanism
#include <mtkcam/drv/IHalSensor.h> // MAKE_HalSensorList

// MTKCAM-driver (platform dependent)
#include <mfb_reg.h> // new MFB register table, only supports ISP ver 5 (and upon)

// DpFramework
#include <DpDataType.h>

// AOSP
#include <utils/Mutex.h> // android::Mutex
#include <cutils/compiler.h> //

// STL
#include <memory>
#include <tuple>
#include <fstream>


#define DRV_MSS_REG_NEED_DUMP   DUMP_FEATURE_CAPTURE
#define DRV_MSF_REG_NEED_DUMP   DUMP_FEATURE_CAPTURE
#define DRV_P2_REG_NEED_DUMP    DUMP_FEATURE_CAPTURE

#define MFNR_EARLY_PORTING      0
#define FAKE_DRIVER_MSS         0
#define FAKE_DRIVER_MSF         0
#define FAKE_DRIVER_MSF_P2      0

#define DEBUG_MSF_P2_DUMP       0
#define MSF_TUNING_IDENTICAL_UPDATE 0




#if (FAKE_DRIVER_MSS == 1) || (FAKE_DRIVER_MSF == 1) || (FAKE_DRIVER_MSF_P2 == 1)
#include <future>
#endif


//
// To print more debug information of this module
// #define __DEBUG

// Enque frame timeout
#define MFLL_MFB_ENQUE_TIMEOUT_MS 1500
#define MFLL_MSS_ENQUE_TIMEOUT_MS 1300
#define MFLL_MSF_ENQUE_TIMEOUT_MS 1900

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
#include <aaa_log.h>
#include <aaa_error_code.h>
#include <aaa_common_custom.h>
#define AEE_ASSERT_MFB(error_log) \
        do { \
            mfllLogE("%s: %s", __FUNCTION__, error_log); \
            aee_system_exception( \
                "\nCRDISPATCH_KEY:" LOG_TAG, \
                NULL, \
                DB_OPT_NE_JBT_TRACES | DB_OPT_PROCESS_COREDUMP | DB_OPT_PROC_MEM | DB_OPT_PID_SMAPS | DB_OPT_LOW_MEMORY_KILLER | DB_OPT_DUMPSYS_PROCSTATS | DB_OPT_FTRACE, \
                error_log); \
            int res = raise(SIGKILL); \
        } while(0)
#else // MTKCAM_HAVE_AEE_FEATURE != 1
#define AEE_ASSERT_MFB(error_log) \
        do { \
            mfllLogE("%s: %s", __FUNCTION__, error_log); \
            *(volatile uint32_t*)(0x00000000) = 0xdeadbeef; \
        } while(0)
#endif // MTKCAM_HAVE_AEE_FEATURE

//
// To workaround device hang of MFB stage
// #define WORKAROUND_MFB_STAGE

// helper macro to check if ISP profile mapping ok or not
#define IS_WRONG_ISP_PROFILE(p) (p == static_cast<EIspProfile_T>(MFLL_ISP_PROFILE_ERROR))

/* platform dependent headers, which needs defines in MfllMfb.cpp */
#include "MfllMfb_platform.h"
#include "MfllMfb_algo.h"

using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam;
using namespace NSCam::Utils::Format;
using namespace NS3Av3;
using namespace mfll;
using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSMFB20;
using namespace NSCam::TuningUtils;

using android::sp;
using android::Mutex;
using NSCam::IImageBuffer;
using NSCam::NSIoPipe::EPortType_Memory;
using NSCam::NSIoPipe::NSSImager::IImageTransform;
using NSCam::NSIoPipe::Input;
using NSCam::NSIoPipe::Output;
using NSCam::NSIoPipe::QParams;
using NSCam::NSIoPipe::PortID;

/* global option to enable DRE or not */
#define DRE_PROPERTY_NAME "vendor.camera.mdp.dre.enable"
static MINT32 gSupportDRE = []() {
    MINT32 supportDRE = ::property_get_int32(DRE_PROPERTY_NAME, -1);
    if (supportDRE < 0) {
        supportDRE = 0;
        mfllLogW("Get property: " DRE_PROPERTY_NAME " fail, default set to 0");
    }
    return supportDRE;
}();
#undef DRE_PROPERTY_NAME


#ifndef MFLL_PYRAMID_SIZE
#error "MFLL_PYRAMID_SIZE is not defined."
#elif (MFLL_PYRAMID_SIZE > MFLL_MAX_PYRAMID_SIZE)
#error "MFLL_PYRAMID_SIZE is larger than MFLL_MAX_PYRAMID_SIZE."
#else
#define MFLL_MSF_STAGE_MAX (MFLL_PYRAMID_SIZE-1)
#endif


static void dumpQParams(const QParams& rQParam, bool bForcedDump = false)
{
    if ( !bForcedDump ) {
        return;
    }

    mfllLogI("%s: Frame size = %zu", __FUNCTION__, rQParam.mvFrameParams.size());
    for(size_t index = 0; index < rQParam.mvFrameParams.size(); ++index) {
        auto& frameParam = rQParam.mvFrameParams.itemAt(index);
        mfllLogI("%s: =================================================", __FUNCTION__);
        mfllLogI("%s: Frame index = %zu", __FUNCTION__, index);
        mfllLogI("%s: mStreamTag=%d mSensorIdx=%d", __FUNCTION__, frameParam.mStreamTag, frameParam.mSensorIdx);
        mfllLogI("%s: FrameNo=%d RequestNo=%d Timestamp=%d", __FUNCTION__, frameParam.FrameNo, frameParam.RequestNo, frameParam.Timestamp);
        mfllLogI("%s: === mvIn section ===", __FUNCTION__);
        for ( size_t index2 = 0; index2 < frameParam.mvIn.size(); ++index2 ) {
            Input data = frameParam.mvIn[index2];
            mfllLogI("%s: Index = %zu", __FUNCTION__, index2);
            mfllLogI("%s: mvIn.PortID.index = %d", __FUNCTION__, data.mPortID.index);
            mfllLogI("%s: mvIn.PortID.type = %d", __FUNCTION__, data.mPortID.type);
            mfllLogI("%s: mvIn.PortID.inout = %d", __FUNCTION__, data.mPortID.inout);

            mfllLogI("%s: mvIn.mBuffer=%p", __FUNCTION__, (void*)data.mBuffer);
            if ( data.mBuffer != NULL ) {
                mfllLogI("%s: mvIn.mBuffer->getImgSize = %dx%d", __FUNCTION__, data.mBuffer->getImgSize().w, data.mBuffer->getImgSize().h);

                mfllLogI("%s: mvIn.mBuffer->getImgFormat = %x", __FUNCTION__, data.mBuffer->getImgFormat());
                mfllLogI("%s: mvIn.mBuffer->getImageBufferHeap = %p", __FUNCTION__, (void*)data.mBuffer->getImageBufferHeap());
                mfllLogI("%s: mvIn.mBuffer->getPlaneCount = %zu", __FUNCTION__, data.mBuffer->getPlaneCount());
                for(int j = 0; j < data.mBuffer->getPlaneCount(); j++ ) {
                    mfllLogI("%s: mvIn.mBuffer->getBufVA(%d) = %p", __FUNCTION__, j, (void*)data.mBuffer->getBufVA(j));
                    mfllLogI("%s: mvIn.mBuffer->getBufPA(%d) = %p", __FUNCTION__, j, (void*)data.mBuffer->getBufPA(j));
                    mfllLogI("%s: mvIn.mBuffer->getBufStridesInBytes(%d) = %zu", __FUNCTION__, j, data.mBuffer->getBufStridesInBytes(j));
                }
            }
            else {
                mfllLogI("%s: mvIn.mBuffer is NULL!!", __FUNCTION__);
            }
            mfllLogI("%s: mvIn.mTransform = %d", __FUNCTION__, data.mTransform);
        }

        mfllLogI("%s: === mvOut section ===", __FUNCTION__);
        for ( size_t index2 = 0; index2 < frameParam.mvOut.size(); index2++ ) {
            Output data = frameParam.mvOut[index2];
            mfllLogI("%s: Index = %zu", __FUNCTION__, index2);

            mfllLogI("%s: mvOut.PortID.index = %d", __FUNCTION__, data.mPortID.index);
            mfllLogI("%s: mvOut.PortID.type = %d", __FUNCTION__, data.mPortID.type);
            mfllLogI("%s: mvOut.PortID.inout = %d", __FUNCTION__, data.mPortID.inout);

            mfllLogI("%s: mvOut.mBuffer=%p", __FUNCTION__, (void*)data.mBuffer);
            if( data.mBuffer != NULL ) {
                mfllLogI("%s: mvOut.mBuffer->getImgSize = %dx%d", __FUNCTION__, data.mBuffer->getImgSize().w, data.mBuffer->getImgSize().h);
                mfllLogI("%s: mvOut.mBuffer->getImgFormat = %x", __FUNCTION__, data.mBuffer->getImgFormat());
                mfllLogI("%s: mvOut.mBuffer->getImageBufferHeap = %p", __FUNCTION__, (void*)data.mBuffer->getImageBufferHeap());
                mfllLogI("%s: mvOut.mBuffer->getPlaneCount = %zu", __FUNCTION__, data.mBuffer->getPlaneCount());
                for ( size_t j = 0; j < data.mBuffer->getPlaneCount(); j++ ) {
                    mfllLogI("%s: mvOut.mBuffer->getBufVA(%zu) = %p", __FUNCTION__, j, (void*)data.mBuffer->getBufVA(j));
                    mfllLogI("%s: mvOut.mBuffer->getBufPA(%zu) = %p", __FUNCTION__, j, (void*)data.mBuffer->getBufPA(j));
                    mfllLogI("%s: mvOut.mBuffer->getBufStridesInBytes(%zu) = %zu", __FUNCTION__, j, data.mBuffer->getBufStridesInBytes(j));
                }
            }
            else {
                mfllLogI("%s: mvOut.mBuffer is NULL!!", __FUNCTION__);
            }
            mfllLogI("%s: mvOut.mTransform = %d", __FUNCTION__, data.mTransform);
        }

        mfllLogI("%s: === mvCropRsInfo section ===", __FUNCTION__);
        for ( size_t i = 0; i < frameParam.mvCropRsInfo.size(); i++ ) {
            MCrpRsInfo data = frameParam.mvCropRsInfo[i];
            mfllLogI("%s: Index = %zu", __FUNCTION__, i);
            mfllLogI("%s: CropRsInfo.mGroupID=%d", __FUNCTION__, data.mGroupID);
            mfllLogI("%s: CropRsInfo.mMdpGroup=%d", __FUNCTION__, data.mMdpGroup);
            mfllLogI("%s: CropRsInfo.mResizeDst=%dx%d", __FUNCTION__, data.mResizeDst.w, data.mResizeDst.h);
            mfllLogI("%s: CropRsInfo.mCropRect.p_fractional=(%d,%d)", __FUNCTION__, data.mCropRect.p_fractional.x, data.mCropRect.p_fractional.y);
            mfllLogI("%s: CropRsInfo.mCropRect.p_integral=(%d,%d)", __FUNCTION__, data.mCropRect.p_integral.x, data.mCropRect.p_integral.y);
            mfllLogI("%s: CropRsInfo.mCropRect.s=%dx%d ", __FUNCTION__, data.mCropRect.s.w, data.mCropRect.s.h);
        }

        mfllLogI("%s: === mvModuleData section ===", __FUNCTION__);
        for( size_t i = 0; i < frameParam.mvModuleData.size(); i++ ) {
            ModuleInfo data = frameParam.mvModuleData[i];
            mfllLogI("%s: Index = %zu", __FUNCTION__, i);
            mfllLogI("%s: ModuleData.moduleTag=%d", __FUNCTION__, data.moduleTag);

            _SRZ_SIZE_INFO_ *SrzInfo = (_SRZ_SIZE_INFO_ *) data.moduleStruct;
            mfllLogI("%s: SrzInfo->in_w=%d", __FUNCTION__, SrzInfo->in_w);
            mfllLogI("%s: SrzInfo->in_h=%d", __FUNCTION__, SrzInfo->in_h);
            mfllLogI("%s: SrzInfo->crop_w=%lu", __FUNCTION__, SrzInfo->crop_w);
            mfllLogI("%s: SrzInfo->crop_h=%lu", __FUNCTION__, SrzInfo->crop_h);
            mfllLogI("%s: SrzInfo->crop_x=%d", __FUNCTION__, SrzInfo->crop_x);
            mfllLogI("%s: SrzInfo->crop_y=%d", __FUNCTION__, SrzInfo->crop_y);
            mfllLogI("%s: SrzInfo->crop_floatX=%d", __FUNCTION__, SrzInfo->crop_floatX);
            mfllLogI("%s: SrzInfo->crop_floatY=%d", __FUNCTION__, SrzInfo->crop_floatY);
            mfllLogI("%s: SrzInfo->out_w=%d", __FUNCTION__, SrzInfo->out_w);
            mfllLogI("%s: SrzInfo->out_h=%d", __FUNCTION__, SrzInfo->out_h);
        }
        mfllLogI("%s: TuningData=%p", __FUNCTION__, (void*)frameParam.mTuningData);
        mfllLogI("%s: === mvExtraData section ===", __FUNCTION__);
        for ( size_t i = 0; i < frameParam.mvExtraParam.size(); i++ ) {
            auto extraParam = frameParam.mvExtraParam[i];
            if ( extraParam.CmdIdx == EPIPE_FE_INFO_CMD ) {
                FEInfo *feInfo = (FEInfo*) extraParam.moduleStruct;
                mfllLogI("%s: mFEDSCR_SBIT=%d  mFETH_C=%d  mFETH_G=%d", __FUNCTION__, feInfo->mFEDSCR_SBIT, feInfo->mFETH_C, feInfo->mFETH_G);
                mfllLogI("%s: mFEFLT_EN=%d  mFEPARAM=%d  mFEMODE=%d", __FUNCTION__, feInfo->mFEFLT_EN, feInfo->mFEPARAM, feInfo->mFEMODE);
                mfllLogI("%s: mFEYIDX=%d  mFEXIDX=%d  mFESTART_X=%d", __FUNCTION__, feInfo->mFEYIDX, feInfo->mFEXIDX, feInfo->mFESTART_X);
                mfllLogI("%s: mFESTART_Y=%d  mFEIN_HT=%d  mFEIN_WD=%d", __FUNCTION__, feInfo->mFESTART_Y, feInfo->mFEIN_HT, feInfo->mFEIN_WD);

            }
            else if ( extraParam.CmdIdx == EPIPE_FM_INFO_CMD ) {
                FMInfo *fmInfo = (FMInfo*) extraParam.moduleStruct;
                mfllLogI("%s: mFMHEIGHT=%d  mFMWIDTH=%d  mFMSR_TYPE=%d", __FUNCTION__, fmInfo->mFMHEIGHT, fmInfo->mFMWIDTH, fmInfo->mFMSR_TYPE);
                mfllLogI("%s: mFMOFFSET_X=%d  mFMOFFSET_Y=%d  mFMRES_TH=%d", __FUNCTION__, fmInfo->mFMOFFSET_X, fmInfo->mFMOFFSET_Y, fmInfo->mFMRES_TH);
                mfllLogI("%s: mFMSAD_TH=%d  mFMMIN_RATIO=%d", __FUNCTION__, fmInfo->mFMSAD_TH, fmInfo->mFMMIN_RATIO);
            }
            else if( extraParam.CmdIdx == EPIPE_MDP_PQPARAM_CMD ) {
                PQParam* param = reinterpret_cast<PQParam*>(extraParam.moduleStruct);
                if ( param->WDMAPQParam != nullptr ) {
                    DpPqParam* dpPqParam = (DpPqParam*)param->WDMAPQParam;
                    DpIspParam& ispParam = dpPqParam->u.isp;
                    VSDOFParam& vsdofParam = dpPqParam->u.isp.vsdofParam;
                    mfllLogI("%s: WDMAPQParam %p enable = %d, scenario=%d", __FUNCTION__, (void*)dpPqParam, dpPqParam->enable, dpPqParam->scenario);
                    mfllLogI("%s: WDMAPQParam iso = %d, frameNo=%d requestNo=%d", __FUNCTION__, ispParam.iso , ispParam.frameNo, ispParam.requestNo);
                    mfllLogI("%s: WDMAPQParam lensId = %d, isRefocus=%d defaultUpTable=%d",
                             __FUNCTION__, ispParam.lensId , vsdofParam.isRefocus, vsdofParam.defaultUpTable);
                    mfllLogI("%s: WDMAPQParam defaultDownTable = %d, IBSEGain=%d", __FUNCTION__, vsdofParam.defaultDownTable, vsdofParam.IBSEGain);
                }
                if( param->WROTPQParam != nullptr ) {
                    DpPqParam* dpPqParam = (DpPqParam*)param->WROTPQParam;
                    DpIspParam&ispParam = dpPqParam->u.isp;
                    VSDOFParam& vsdofParam = dpPqParam->u.isp.vsdofParam;
                    mfllLogI("%s: WROTPQParam %p enable = %d, scenario=%d", __FUNCTION__, (void*)dpPqParam, dpPqParam->enable, dpPqParam->scenario);
                    mfllLogI("%s: WROTPQParam iso = %d, frameNo=%d requestNo=%d", __FUNCTION__, ispParam.iso , ispParam.frameNo, ispParam.requestNo);
                    mfllLogI("%s: WROTPQParam lensId = %d, isRefocus=%d defaultUpTable=%d",
                             __FUNCTION__, ispParam.lensId , vsdofParam.isRefocus, vsdofParam.defaultUpTable);
                    mfllLogI("%s: WROTPQParam defaultDownTable = %d, IBSEGain=%d", __FUNCTION__, vsdofParam.defaultDownTable, vsdofParam.IBSEGain);
                }
            }
        }
    }
}

/* Make sure pass 2 is thread-safe, basically it's not ... (maybe) */
static Mutex gMutexPass2Lock;

template <class T>
static inline MVOID __P2Cb(T& rParams)
{
    mfllLogI("mpfnCallback [+]");
    std::atomic_thread_fence(std::memory_order_acquire); // LoadLoad

    using namespace MfllMfb_Imp;
    P2Cookie* p = static_cast<P2Cookie*>(rParams.mpCookie);
    if (__builtin_expect( p == nullptr, false )) {
        mfllLogE("%s: P2Cookie is nullptr", __FUNCTION__);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        mfllLogI("mpfnCallback [-]");
        return;
    }

    mfllLogI("%s:mpCookie=%p", __FUNCTION__, rParams.mpCookie);

    // check if the P2Cookie is valid or not.
    if (__builtin_expect( !isValidCookie(*p), false )) {
        mfllLogE("%s: P2Cookie is invalid", __FUNCTION__);
        checkSumDump(*p);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        return;
    }

    // P2Cookie is valid, but still may visible side-effect since we cannot
    // synchronize rParams.
    size_t triedCnt = 0;
    do {
        bool locked = p->__mtx.try_lock();
        if (locked == false) { // lock failed
            if (triedCnt >= 10) { // try until timeout
                mfllLogE("%s: try to lock P2Cookie's mutex failed, "\
                        "ignore lock and notify condition_variable",
                        __FUNCTION__);
            }
            else {
                mfllLogW("%s: acquire lock failed, may have performance issue, "\
                         "try again (cnt=%zu)", __FUNCTION__, triedCnt);
                triedCnt++;
                // wait a while and try again (10ms).
                std::this_thread::sleep_for( std::chrono::milliseconds(10) );
                continue; // continue loop
            }
        }
        p->__signaled.store(1, std::memory_order_seq_cst); // avoid visible side-effect
        p->__cv.notify_all();
        // unlock mutex if necessary
        if (__builtin_expect( locked, true )) {
            p->__mtx.unlock();
        }
        break; // break loop
    } while(true);

    mfllLogD3("P2 callback signaled");
    mfllLogI("mpfnCallback [-]");
}

template <class T>
static inline MVOID __P2FailCb(T& rParams)
{
    using namespace MfllMfb_Imp;
    P2Cookie* p = static_cast<P2Cookie*>(rParams.mpCookie);
    if (__builtin_expect( p == nullptr, false )) {
        mfllLogE("%s: P2Cookie is nullptr", __FUNCTION__);
        *(volatile uint32_t*)(0x00000000) = 0xDEADBEEF;
        return;
    }

    dumpQParams(rParams, true);
    AEE_ASSERT_MFB("FATAL: invalid QParams, P2 failed callback is triggered!");
}


static inline void updateP2RevisedTag(IImageBuffer* pSrc, IMetadata* pMetadata)
{
    MINT32 resolution = pSrc->getImgSize().w | (pSrc->getImgSize().h << 16);
    IMetadata::setEntry<MINT32>(
            pMetadata,
            MTK_ISP_P2_IN_IMG_RES_REVISED,
            resolution
            );

    mfllLogD3("P2 revised img size=%dx%d",
            resolution & 0x0000FFFF,
            resolution >> 16
            );
}
static inline void removeP2RevisedTag(IMetadata* pMetadata)
{
    pMetadata->remove(MTK_ISP_P2_IN_IMG_RES_REVISED);
}


static MRect calCrop(MRect const &rSrc, MRect const &rDst, uint32_t /* ratio */)
{
    #define ROUND_TO_2X(x) ((x) & (~0x1))

    MRect rCrop;

    // srcW/srcH < dstW/dstH
    if (rSrc.s.w * rDst.s.h < rDst.s.w * rSrc.s.h)
    {
        rCrop.s.w = rSrc.s.w;
        rCrop.s.h = rSrc.s.w * rDst.s.h / rDst.s.w;
    }
    // srcW/srcH > dstW/dstH
    else if (rSrc.s.w * rDst.s.h > rDst.s.w * rSrc.s.h)
    {
        rCrop.s.w = rSrc.s.h * rDst.s.w / rDst.s.h;
        rCrop.s.h = rSrc.s.h;
    }
    // srcW/srcH == dstW/dstH
    else
    {
        rCrop.s.w = rSrc.s.w;
        rCrop.s.h = rSrc.s.h;
    }

    rCrop.s.w =  ROUND_TO_2X(rCrop.s.w);
    rCrop.s.h =  ROUND_TO_2X(rCrop.s.h);

    rCrop.p.x = (rSrc.s.w - rCrop.s.w) / 2;
    rCrop.p.y = (rSrc.s.h - rCrop.s.h) / 2;

    rCrop.p.x += ROUND_TO_2X(rSrc.p.x);
    rCrop.p.y += ROUND_TO_2X(rSrc.p.y);

    #undef ROUND_TO_2X
    return rCrop;
}


static DpPqParam generateMdpDumpPQParam(
        const IMetadata* pMetadata
        )
{
    DpPqParam pq;
    pq.enable   = false; // we dont need pq here.
    pq.scenario = MEDIA_ISP_CAPTURE;

    // dump info
    MINT32 uniqueKey = 0;
    MINT32 frameNum  = 0;
    MINT32 requestNo = 0;

    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
        mfllLogW("get MTK_PIPELINE_UNIQUE_KEY failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_FRAME_NUMBER, frameNum)) {
        mfllLogW("get MTK_PIPELINE_FRAME_NUMBER failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_REQUEST_NUMBER, requestNo)) {
        mfllLogW("get MTK_PIPELINE_REQUEST_NUMBER failed, set to 0");
    }
    if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_UNIQUE_KEY, uniqueKey)) {
        mfllLogD("get MTK_PIPELINE_DUMP_UNIQUE_KEY, update uniqueKey to %d", uniqueKey);
        if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_FRAME_NUMBER, frameNum)) {
            mfllLogD("get MTK_PIPELINE_DUMP_FRAME_NUMBER, update frameNum to %d", frameNum);
        }
        if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_REQUEST_NUMBER, requestNo)) {
            mfllLogD("get MTK_PIPELINE_DUMP_REQUEST_NUMBER, update requestNo to %d", requestNo);
        }
    }

    pq.u.isp.timestamp = static_cast<uint32_t>(uniqueKey);
    pq.u.isp.frameNo   = static_cast<uint32_t>(frameNum);
    pq.u.isp.requestNo = static_cast<uint32_t>(requestNo);

    return pq;
}

static void generateNDDHintFromMetadata(FILE_DUMP_NAMING_HINT& hint, const IMetadata* pMetadata)
{
    // dump info
    MINT32 uniqueKey = 0;
    MINT32 frameNum  = 0;
    MINT32 requestNo = 0;

    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
        mfllLogW("get MTK_PIPELINE_UNIQUE_KEY failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_FRAME_NUMBER, frameNum)) {
        mfllLogW("get MTK_PIPELINE_FRAME_NUMBER failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_REQUEST_NUMBER, requestNo)) {
        mfllLogW("get MTK_PIPELINE_REQUEST_NUMBER failed, set to 0");
    }
    if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_UNIQUE_KEY, uniqueKey)) {
        mfllLogD("get MTK_PIPELINE_DUMP_UNIQUE_KEY, update uniqueKey to %d", uniqueKey);
        if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_FRAME_NUMBER, frameNum)) {
            mfllLogD("get MTK_PIPELINE_DUMP_FRAME_NUMBER, update frameNum to %d", frameNum);
        }
        if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_REQUEST_NUMBER, requestNo)) {
            mfllLogD("get MTK_PIPELINE_DUMP_REQUEST_NUMBER, update requestNo to %d", requestNo);
        }
    }

    hint.UniqueKey          = uniqueKey;
    hint.RequestNo          = requestNo;
    hint.FrameNo            = frameNum;
}

#if MTK_CAM_NEW_NVRAM_SUPPORT
// {{{
template <NSIspTuning::EModule_T module>
inline void* _getNVRAMBuf(NVRAM_CAMERA_FEATURE_STRUCT* /*pNvram*/, size_t /*idx*/)
{
    mfllLogE("_getNVRAMBuf: unsupport module(%d)", module);
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
getTuningFromNvram(MUINT32 openId, MUINT32 idx, MINT32 magicNo, MINT32 ispProfile = -1)
{
    mfllTraceCall();

    NVRAM_CAMERA_FEATURE_STRUCT *pNvram;
    void* pNRNvram              = nullptr;
    std::tuple<void*, int>      emptyVal(nullptr, -1);

    if (__builtin_expect( idx >= EISO_NUM, false )) {
        mfllLogE("wrong nvram idx %d", idx);
        return emptyVal;
    }

    // load some setting from nvram
    MUINT sensorDev = MAKE_HalSensorList()->querySensorDevIdx(openId);
    IdxMgr* pMgr = IdxMgr::createInstance(static_cast<ESensorDev_T>(sensorDev));
    CAM_IDX_QRY_COMB rMapping_Info;

    // query mapping info
    mfllLogD3("getMappingInfo with magicNo(%d)", magicNo);
    pMgr->getMappingInfo(static_cast<ESensorDev_T>(sensorDev), rMapping_Info, magicNo);

    // tricky: force set ISP profile since new NVRAM SW limitation
    if (ispProfile >= 0) {
        rMapping_Info.eIspProfile = static_cast<EIspProfile_T>( ispProfile );
        pMgr->setMappingInfo(static_cast<ESensorDev_T>(sensorDev), rMapping_Info, magicNo);
        mfllLogD3("%s: force set ISP profile to %#x", __FUNCTION__, ispProfile);
    }

    // query NVRAM index by mapping info
    idx = pMgr->query(static_cast<ESensorDev_T>(sensorDev), module, rMapping_Info, __FUNCTION__);
    mfllLogD3("query nvram DRE mappingInfo index: %d", idx);

    auto pNvBufUtil = MAKE_NvBufUtil();
    if (__builtin_expect( pNvBufUtil == NULL, false )) {
        mfllLogE("pNvBufUtil==0");
        return emptyVal;
    }

    auto result = pNvBufUtil->getBufAndRead(
        CAMERA_NVRAM_DATA_FEATURE,
        sensorDev, (void*&)pNvram);
    if (__builtin_expect( result != 0, false )) {
        mfllLogE("read buffer chunk fail");
        return emptyVal;
    }

    pNRNvram = _getNVRAMBuf<module>(pNvram, static_cast<size_t>(idx));
    return std::make_tuple( pNRNvram,  idx );
}
// }}}
#endif


static bool generateMdpDrePQParam(
        DpPqParam&          rMdpPqParam,
        int                 iso,
        int                 openId,
        const IMetadata*    pMetadata,
        EIspProfile_T       ispProfile
        )
{
    // dump info
    MINT32 uniqueKey = 0;
    MINT32 frameNum  = 0;
    MINT32 magicNum  = 0;
    MINT32 lv_value  = 0;

    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
        mfllLogW("get MTK_PIPELINE_UNIQUE_KEY failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_FRAME_NUMBER, frameNum)) {
        mfllLogW("get MTK_PIPELINE_FRAME_NUMBER failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_REAL_LV, lv_value)) {
        mfllLogW("get MTK_REAL_LV failed, set to 0");
    }
    if (!IMetadata::getEntry<MINT32>(pMetadata, MTK_P1NODE_PROCESSOR_MAGICNUM, magicNum)) {
        mfllLogW("get MTK_P1NODE_PROCESSOR_MAGICNUM failed, cannot generate DRE's tuning "\
                "data, won't apply");
        return false;
    }
    if (IMetadata::getEntry<MINT32>(pMetadata, MTK_PIPELINE_DUMP_UNIQUE_KEY, uniqueKey)) {
        mfllLogD("get MTK_PIPELINE_DUMP_UNIQUE_KEY, update uniqueKey to %d", uniqueKey);
    }

    /* check ISO */
    if (__builtin_expect( iso <= 0, false )) {
        mfllLogE("ISO information is wrong, set to 0");
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
    std::tuple<void*, int> nvramData = getTuningFromNvram<NSIspTuning::EModule_CA_LTM>(
            openId,
            0, // no need index
            magicNum,
            static_cast<MINT32>( ispProfile )
            );

    dreParam.p_customSetting = std::get<I_BUFFER>   (nvramData);
    dreParam.customIndex     = std::get<I_NVRAMIDX> (nvramData);

    mfllLogD3("generate DREParam with userId: %llu, iso: %d, sensorId: %d, magicNum: %d",
            dreParam.userId, iso, openId, magicNum);
    return true;
}


static bool attachMDPPQParam2Pass2drv(
        PortID          portId,
        FrameParams*    pFrameParams,
        PQParam*        pP2Pq,
        DpPqParam*      pMdpPQ
        )
{
    if (portId == PORT_WDMAO) {
        pP2Pq->WDMAPQParam = static_cast<void*>(pMdpPQ);
    }
    else if (portId == PORT_WROTO) {
        pP2Pq->WROTPQParam = static_cast<void*>(pMdpPQ);
    }
    else {
        mfllLogD("%s: attach failed, the port is neither WDMAO nor WROTO", __FUNCTION__);
        return false;
    }

    ExtraParam extraP;
    extraP.CmdIdx = EPIPE_MDP_PQPARAM_CMD;
    extraP.moduleStruct = static_cast<void*>(pP2Pq);
    pFrameParams->mvExtraParam.push_back(extraP);

    return true;
}

// Set MTK_ISP_P2_IN_IMG_FMT to value and return the original value (if not exists returns 0).
//  @param halMeta      HAL metadata to set and query.
//  @param value        Value to set.
//  @param pRestorer    An unique_ptr<void*> with a custom deleter, the deleter is to reset
//                      MTK_ISP_P2_IN_IMG_FMT of halMeta back to original value, where the deleter
//                      is being invoked while pRestorer is being deleted.
//  @note While invoking the deleter of pRestorer, the reference of halMeta must be available,
//        or unexpected behavior will happen.
inline static MINT32
setIspP2ImgFmtTag(
        IMetadata&                                              halMeta,
        MINT32                                                  value,
        std::unique_ptr<void, std::function<void(void*)> >*     pRestorer = nullptr
        )
{
    MINT32 tagIspP2InFmt = 0;

    // try to retrieve MTK_ISP_P2_IN_IMG_FMT (for backup)
    IMetadata::getEntry<MINT32>(&halMeta, MTK_ISP_P2_IN_IMG_FMT, tagIspP2InFmt);

    // set MTK_ISP_P2_IN_IMG_FMT to 1 (1 means YUV->YUV)
    IMetadata::setEntry<MINT32>(&halMeta, MTK_ISP_P2_IN_IMG_FMT, value);
    mfllLogD3("set MTK_ISP_P2_IN_IMG_FMT to %d", value);

    if (pRestorer) {
        auto myRestorer = [&halMeta, tagIspP2InFmt](void*) mutable -> void
        {
            mfllLogD3("restore MTK_ISP_P2_IN_IMG_FMT back to the original one -> %d", tagIspP2InFmt);
            // restore metaset.halMeta's MTK_ISP_P2_IN_IMG_FMT back to the original one (default is 0)
            IMetadata::setEntry<MINT32>(&halMeta, MTK_ISP_P2_IN_IMG_FMT, tagIspP2InFmt);
        };

        // declare an unique_ptr and gives a value, the main purpose is invoking a custom deleter
        // while destroying _pRestorer (we will move _r to *pRestorer).
        std::unique_ptr<void, std::function<void(void*)> > _r(
                reinterpret_cast<void*>(0x00BADBAD), // we WON'T delete this pointer. Just gives it a value to invoke custom deleter
                std::move(myRestorer)
                );

        *pRestorer = std::move( _r );
    }

    return tagIspP2InFmt;
}


static bool isRawFormat(const EImageFormat fmt)
{
    /* RAW format is in range of eImgFmt_RAW_START -> eImgFmt_BLOB_START */
    /* see include/mtkcam/def/ImageFormat.h */
    return (fmt >= eImgFmt_RAW_START) && (fmt < eImgFmt_BLOB_START);
}

//-----------------------------------------------------------------------------
/**
 *  M F L L    M F B
 */
IMfllMfb* IMfllMfb::createInstance(void)
{
    return (IMfllMfb*)new MfllMfb();
}
//
//-----------------------------------------------------------------------------
//
void IMfllMfb::destroyInstance(void)
{
    decStrong((void*)this);
}
//
//-----------------------------------------------------------------------------
//
MfllMfb::MfllMfb(void)
    : m_sensorId(0)
    , m_shotMode((enum MfllMode)0)
    , m_nrType(NoiseReductionType_None)
    , m_syncPrivateData(nullptr)
    , m_syncPrivateDataSize(0)
    , m_pCore(nullptr)
    , m_pMixDebugBuffer(nullptr)
    , m_encodeYuvCount(0)
    , m_blendCount(0)
    , m_bIsInited(false)
    , m_bExifDumpped{0}
    , m_pMainMetaApp(nullptr)
    , m_pMainMetaHal(nullptr)
    , m_pBPCI(nullptr)
    , m_tuningBuf()
    , m_tuningBufMsf{}
    , m_tuningBufMsfTbl{}
    , m_tuningBufMsfP2{}
    , m_tuningBufMss{}
    , m_nddHintMss()
    , m_nddHintMsf()
{
    mfllAutoLogFunc();
}
//
//-----------------------------------------------------------------------------
//
MfllMfb::~MfllMfb(void)
{
    mfllAutoLogFunc();
    m_pNormalStream = nullptr;
    m_pHalISP = nullptr;
}
//
//-----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::init(int sensorId)
{
    enum MfllErr err = MfllErr_Ok;
    Mutex::Autolock _l(m_mutex);

    mfllAutoLogFunc();

    if (m_bIsInited) { // do not init twice
        goto lbExit;
    }

    mfllLogD3("Init MfllMfb with sensor id --> %d", sensorId);
    m_sensorId = sensorId;

    /* RAII for INormalStream */
    m_pNormalStream = decltype(m_pNormalStream)(
        INormalStream::createInstance(sensorId),
        [](INormalStream* p) {
            if (!p) return;
            p->uninit(LOG_TAG);
            p->destroyInstance();
        }
    );

    if (CC_UNLIKELY( m_pNormalStream.get() == nullptr )) {
        mfllLogE("create INormalStream fail");
        err = MfllErr_UnexpectedError;
        goto lbExit;
    }
    else {
        MBOOL bResult = m_pNormalStream->init(LOG_TAG);
        if (CC_UNLIKELY(bResult == MFALSE)) {
            mfllLogE("init INormalStream returns MFALSE");
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }
    }

    /* RAII for IHalISP instance */
    m_pHalISP = decltype(m_pHalISP)(
            MAKE_HalISP(sensorId, LOG_TAG),
            [](IHalISP* p){ if (p) p->destroyInstance(LOG_TAG); }
    );

    if (CC_UNLIKELY( m_pHalISP.get() == nullptr )) {
        mfllLogE("create IHalISP fail");
        err = MfllErr_UnexpectedError;
        goto lbExit;
    }


    /* RAII for IEgnStream instance */
    m_pMsfStream = decltype(m_pMsfStream)(
            IEgnStream<MSFConfig>::createInstance(LOG_TAG),
            [](IEgnStream<MSFConfig>* p) {
                if (!p) return;
                p->uninit();
                p->destroyInstance(LOG_TAG);
            }
    );
    /* init IEgnStream */
    if (CC_UNLIKELY( m_pMsfStream.get() == nullptr )) {
        mfllLogE("create IEgnStream failed");
        err = MfllErr_UnexpectedError;
        goto lbExit;
    }
    else {
        MBOOL bResult = m_pMsfStream->init();
        if (CC_UNLIKELY( bResult == MFALSE )) {
            mfllLogE("init IEgnStream returns MFALSE");
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }
    }

    /* RAII for IEgnStream instance */
    m_pMssStream = decltype(m_pMssStream)(
            IEgnStream<MSSConfig>::createInstance(LOG_TAG),
            [](IEgnStream<MSSConfig>* p) {
                if (!p) return;
                p->uninit();
                p->destroyInstance(LOG_TAG);
            }
    );
    /* init IEgnStream */
    if (CC_UNLIKELY( m_pMssStream.get() == nullptr )) {
        mfllLogE("create IEgnStream failed");
        err = MfllErr_UnexpectedError;
        goto lbExit;
    }
    else {
        MBOOL bResult = m_pMssStream->init();
        if (CC_UNLIKELY( bResult == MFALSE )) {
            mfllLogE("init IEgnStream returns MFALSE");
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }
    }


    /* mark as inited */
    m_bIsInited = true;

    /* create ISP tuning buffer chunks, these buffer chunks are supposed to be accessed w/o race condition */
    {
        size_t regTableSize = m_pNormalStream->getRegTableSize();
        if (__builtin_expect( regTableSize <= 0, false )) {
            mfllLogE("%s: INormalStream::getRegTableSize() returns 0", __FUNCTION__);
            *(uint32_t*)(0x00000000) = 0xdeadfeed;
            return MfllErr_UnexpectedError;
        }
        m_tuningBuf.data = std::make_unique<char[]>(regTableSize);
        m_tuningBuf.size = regTableSize;
    }

    /* create ISP(MFB) tuning buffer chunk */
    {
        constexpr const size_t MFB_TUNING_BUF_SIZE = sizeof( mfb_reg_t );
        constexpr const size_t MSS_TUNING_BUF_SIZE = sizeof( mss_reg_t );
        constexpr const size_t MFB_TBL_TUNING_BUF_SIZE = 256*sizeof( unsigned int );
        size_t regTableSize = m_pNormalStream->getRegTableSize();

        static_assert( MFB_TUNING_BUF_SIZE > 0, "sizeof( mfb_reg_t ) is 0");
        static_assert( MSS_TUNING_BUF_SIZE > 0, "sizeof( mss_reg_t ) is 0");
        static_assert( MFB_TBL_TUNING_BUF_SIZE > 0, "sizeof( mss_reg_t ) is 0");
        if (__builtin_expect( regTableSize <= 0, false )) {
            mfllLogE("%s: INormalStream::getRegTableSize() returns 0", __FUNCTION__);
            *(uint32_t*)(0x00000000) = 0xdeadfeed;
            return MfllErr_UnexpectedError;
        }

        for (size_t f = 0 ; f < MFLL_MAX_FRAMES ; f++) {
            for (size_t s = 0 ; s < MFLL_MSF_STAGE_MAX ; s++) {
                m_tuningBufMsf[f][s].data = std::make_unique<char[]>(MFB_TUNING_BUF_SIZE);
                m_tuningBufMsf[f][s].size = MFB_TUNING_BUF_SIZE;
                m_tuningBufMsfTbl[f][s].data = std::make_unique<char[]>(MFB_TBL_TUNING_BUF_SIZE);
                m_tuningBufMsfTbl[f][s].size = MFB_TBL_TUNING_BUF_SIZE;
            }
            m_tuningBufMss[f].data = std::make_unique<char[]>(MSS_TUNING_BUF_SIZE);
            m_tuningBufMss[f].size = MSS_TUNING_BUF_SIZE;
        }
        for (size_t s = 0 ; s < MFLL_MSF_STAGE_MAX ; s++) {
            m_tuningBufMsfP2[s].data = std::make_unique<char[]>(regTableSize);
            m_tuningBufMsfP2[s].size = regTableSize;
        }
    }

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
void MfllMfb::setMfllCore(
        IMfllCore* c
        )
{
    Mutex::Autolock _l(m_mutex);
    m_pCore = c;
}
//
// ----------------------------------------------------------------------------
//
void MfllMfb::setShotMode(
        const enum MfllMode& mode
        )
{
    Mutex::Autolock _l(m_mutex);
    m_shotMode = mode;
}
//
// ----------------------------------------------------------------------------
//
void MfllMfb::setPostNrType(
        const enum NoiseReductionType& type
        )
{
    Mutex::Autolock _l(m_mutex);
    m_nrType = type;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::blend(IMfllImageBuffer *base, IMfllImageBuffer *ref, IMfllImageBuffer *out, IMfllImageBuffer *wt_in, IMfllImageBuffer *wt_out)
{
    mfllAutoLogFunc();

    /* It MUST NOT be NULL, so don't do error handling */
    if (base == NULL) {
        mfllLogE("%s: base is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (ref == NULL) {
        mfllLogE("%s: ref is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (out == NULL) {
        mfllLogE("%s: out is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (wt_out == NULL) {
        mfllLogE("%s: wt_out is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    return blend(
            (IImageBuffer*)base->getImageBuffer(),
            (IImageBuffer*)ref->getImageBuffer(),
            (IImageBuffer*)out->getImageBuffer(),
            wt_in ? (IImageBuffer*)wt_in->getImageBuffer() : NULL,
            (IImageBuffer*)wt_out->getImageBuffer()
            );
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::blend(
        IMfllImageBuffer* base,
        IMfllImageBuffer* ref,
        IMfllImageBuffer* conf,
        IMfllImageBuffer* out,
        IMfllImageBuffer* wt_in,
        IMfllImageBuffer* wt_out
        )
{
    mfllAutoLogFunc();

    /* It MUST NOT be NULL, so don't do error handling */
    if (base == NULL) {
        mfllLogE("%s: base is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (ref == NULL) {
        mfllLogE("%s: ref is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (conf == NULL) {
        mfllLogE("%s: confidence map is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (out == NULL) {
        mfllLogE("%s: out is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (wt_out == NULL) {
        mfllLogE("%s: wt_out is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    return blend(
            (IImageBuffer*)base->getImageBuffer(),
            (IImageBuffer*)ref->getImageBuffer(),
            (IImageBuffer*)out->getImageBuffer(),
            wt_in ? (IImageBuffer*)wt_in->getImageBuffer() : NULL,
            (IImageBuffer*)wt_out->getImageBuffer(),
            (IImageBuffer*)conf->getImageBuffer()
            );
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::blend(
        IMfllImageBuffer* base,
        IMfllImageBuffer* ref,
        IMfllImageBuffer* conf,
        IMfllImageBuffer* mcmv,
        IMfllImageBuffer* out,
        IMfllImageBuffer* wt_in,
        IMfllImageBuffer* wt_out
        )
{
    mfllAutoLogFunc();

    /* It MUST NOT be NULL, so don't do error handling */
    if (base == NULL) {
        mfllLogE("%s: base is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (ref == NULL) {
        mfllLogE("%s: ref is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (conf == NULL) {
        mfllLogE("%s: confidence map is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (mcmv == NULL) {
        mfllLogE("%s: motion compensation mv is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (out == NULL) {
        mfllLogE("%s: out is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (wt_out == NULL) {
        mfllLogE("%s: wt_out is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    return blend(
            (IImageBuffer*)base->getImageBuffer(),
            (IImageBuffer*)ref->getImageBuffer(),
            (IImageBuffer*)out->getImageBuffer(),
            wt_in ? (IImageBuffer*)wt_in->getImageBuffer() : NULL,
            (IImageBuffer*)wt_out->getImageBuffer(),
            (IImageBuffer*)conf->getImageBuffer(),
            (IImageBuffer*)mcmv->getImageBuffer()
            );
}


enum MfllErr MfllMfb::blend(
        IImageBuffer *base,
        IImageBuffer *ref,
        IImageBuffer *out,
        IImageBuffer *wt_in,
        IImageBuffer *wt_out,
        IImageBuffer *confmap, /* = nullptr */
        IImageBuffer *mcmv /* = nullptr */)
{
    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    enum MfllErr err = MfllErr_NotImplemented;

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::generateMssParam
(
        vector<IImageBuffer *>  pylamid,
        IImageBuffer*           omc,
        IImageBuffer*           mcmv,
        int                     index,
        EGNParams<MSSConfig>&   rMssParams)
{
    enum MfllErr err = MfllErr_Ok;

    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    if (pylamid.size() <= 0 )
        return MfllErr_BadArgument;

    /**
     *  Stand alone MSS
     *
     *  Configure input parameters and execute
     */
    {
        MSSConfig mssCfg;
        mssCfg.scale_Total = pylamid.size();
        mssCfg.mss_scaleFrame = pylamid;

        if (omc != nullptr && mcmv != nullptr && index > 0) {
            mssCfg.mss_scenario = MSS_REFE_S1_MODE;
            mssCfg.mss_mvMap = mcmv;
            mssCfg.mss_omcFrame = omc;
            // omc tuning
            if (!m_tuningBufMss[index-1].isInited) {
                mfllLogE("mss(%d) tuning data is not init", index-1);
                err = MfllErr_BadArgument;
                goto lbExit;
            }
            mssCfg.omc_tuningBuf = static_cast<unsigned int*>((void*)m_tuningBufMss[index-1].data.get());
        } else {
            mssCfg.mss_scenario = MSS_BASE_S2_MODE;
            mssCfg.mss_mvMap = nullptr;
            mssCfg.mss_omcFrame = nullptr;
            // omc tuning
            mssCfg.omc_tuningBuf = nullptr;
        }


        /*****************************************************************************
         * Param dumnp start                                                         *
         *****************************************************************************/
        mfllLogI("%s is ready for index(%d)", __FUNCTION__, index);

        mfllLogD3("input(mss_scenario): %d", (int)mssCfg.mss_scenario);
        mfllLogD3("input(scale_Total): %d", (int)mssCfg.scale_Total);

        /* input: mss_omcFrame frame */
        if (mssCfg.mss_omcFrame){
            mfllLogD3("input(mss_omcFrame): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)mssCfg.mss_omcFrame->getBufVA(0), (void*)mssCfg.mss_omcFrame->getBufPA(0),
                    // fmt
                    mssCfg.mss_omcFrame->getImgFormat(),
                    // size
                    mssCfg.mss_omcFrame->getImgSize().w, mssCfg.mss_omcFrame->getImgSize().h,
                    // planes, stride
                    mssCfg.mss_omcFrame->getPlaneCount(), mssCfg.mss_omcFrame->getBufStridesInBytes(0)
                    );
        }

        /* input: mss_mvMap frame */
        if (mssCfg.mss_mvMap){
            mfllLogD3("input(mss_mvMap): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)mssCfg.mss_mvMap->getBufVA(0), (void*)mssCfg.mss_mvMap->getBufPA(0),
                    // fmt
                    mssCfg.mss_mvMap->getImgFormat(),
                    // size
                    mssCfg.mss_mvMap->getImgSize().w, mssCfg.mss_mvMap->getImgSize().h,
                    // planes, stride
                    mssCfg.mss_mvMap->getPlaneCount(), mssCfg.mss_mvMap->getBufStridesInBytes(0)
                    );
        }


        /* input: pyramid frame */
        for (int i = 0 ; i < mssCfg.mss_scaleFrame.size() ; i++) {
            if (mssCfg.mss_scaleFrame[i] == nullptr) {
                err = MfllErr_BadArgument;
                goto lbExit;
            }
            mfllLogD3("input(pylamid[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)mssCfg.mss_scaleFrame[i]->getBufVA(0), (void*)mssCfg.mss_scaleFrame[i]->getBufPA(0),
                    // fmt
                    mssCfg.mss_scaleFrame[i]->getImgFormat(),
                    // size
                    mssCfg.mss_scaleFrame[i]->getImgSize().w, mssCfg.mss_scaleFrame[i]->getImgSize().h,
                    // planes, stride
                    mssCfg.mss_scaleFrame[i]->getPlaneCount(), mssCfg.mss_scaleFrame[i]->getBufStridesInBytes(0)
                    );
        }

        {
            err = generateNDDHint(m_nddHintMss, index);
            if (err != MfllErr_Ok)
                return err;
            mssCfg.dumphint = &m_nddHintMss;
            mssCfg.dump_en = DRV_MSS_REG_NEED_DUMP;
        }

        rMssParams.mEGNConfigVec.push_back(mssCfg);
        rMssParams.mpEngineID = eMSS;
    }

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::generateMsfParam
(
        vector<IImageBuffer *>  base,
        vector<IImageBuffer *>  ref,
        vector<IImageBuffer *>  out,
        vector<IImageBuffer *>  wt_in,
        vector<IImageBuffer *>  wt_out,
        vector<IImageBuffer *>  wt_ds,
        IImageBuffer*           difference,
        IImageBuffer*           confmap,
        IImageBuffer*           dsFrame,
        EGNParams<MSFConfig>&   rMsfParams)
{
    enum MfllErr err = MfllErr_Ok;

    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    if (base.size() <= 0 || ref.size() <= 0 || out.size() <= 0
        || wt_in.size() <= 0 || wt_out.size() <= 0 || wt_ds.size() <= 0
        || difference == nullptr || confmap == nullptr || dsFrame == nullptr) {
        mfllLogE("msf input buffer size is null.");
        return MfllErr_BadArgument;
    }

    if (base.size() != ref.size() || base.size() != out.size()) {
        mfllLogE("msf pyramid size is not sync.");
        return MfllErr_BadArgument;
    }

    if (wt_in.size() != wt_out.size() || wt_in.size() != wt_ds.size()) {
        mfllLogE("msf weighting table size is not sync.");
        return MfllErr_BadArgument;
    }

    /**
     *  Stand alone MSF
     *
     *  Configure input parameters and execute
     */
    {
        MSFConfig msfCfg;
        msfCfg.msf_scenario = MSF_MFNR_MODE_CAP;
        msfCfg.frame_Idx = m_blendCount+1;
        msfCfg.frame_Total = m_pCore->getFrameCapturedNum();
        msfCfg.scale_Total = base.size();

        mfllLogD3("MSFConfig.msf_scenario = %d", (int)msfCfg.msf_scenario);
        mfllLogD3("MSFConfig.frame_Idx = %u", msfCfg.frame_Idx);
        mfllLogD3("MSFConfig.frame_Total = %u", msfCfg.frame_Total);
        mfllLogD3("MSFConfig.scale_Total = %u",  msfCfg.scale_Total);

        // tuning
        msfCfg.msf_tuningBuf.clear();
        for (size_t stage = 0 ; stage < base.size()-1 ; stage++) {
            if (!m_tuningBufMsf[m_blendCount][stage].isInited) {
                mfllLogE("msf(%u, %zu) tuning data is not init", m_blendCount, stage);
                err = MfllErr_BadArgument;
                goto lbExit;
            }
            msfCfg.msf_tuningBuf.push_back(static_cast<unsigned int*>((void*)m_tuningBufMsf[m_blendCount][stage].data.get()));
        }
        // msf tbl
        msfCfg.msf_sramTable.clear();
        for (size_t stage = 0 ; stage < base.size()-1 ; stage++) {
            if (!m_tuningBufMsfTbl[m_blendCount][stage].isInited) {
                mfllLogE("msftbl(%u, %zu) is not init", m_blendCount, stage);
                err = MfllErr_BadArgument;
                goto lbExit;
            }
            msfCfg.msf_sramTable.push_back(static_cast<unsigned int*>((void*)m_tuningBufMsfTbl[m_blendCount][stage].data.get()));
        }

        msfCfg.msf_imageDifference = difference;
        msfCfg.msf_confidenceMap = confmap;
        msfCfg.msf_dsFrame = dsFrame;

        msfCfg.msf_baseFrame = base;
        msfCfg.msf_refFrame = ref;
        msfCfg.msf_outFrame = out;
        msfCfg.msf_weightingMap_in = wt_in;
        msfCfg.msf_weightingMap_out = wt_out;
        msfCfg.msf_weightingMap_ds = wt_ds;


        /*****************************************************************************
         * Param dumnp start                                                         *
         *****************************************************************************/
        mfllLogI("%s is ready for index(%u)", __FUNCTION__, m_blendCount);

        /* input: msf_imageDifference frame */
        if (msfCfg.msf_imageDifference){
            mfllLogD3("input(msf_imageDifference): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)msfCfg.msf_imageDifference->getBufVA(0), (void*)msfCfg.msf_imageDifference->getBufPA(0),
                    // fmt
                    msfCfg.msf_imageDifference->getImgFormat(),
                    // size
                    msfCfg.msf_imageDifference->getImgSize().w, msfCfg.msf_imageDifference->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_imageDifference->getPlaneCount(), msfCfg.msf_imageDifference->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_confidenceMap frame */
        if (msfCfg.msf_confidenceMap){
            mfllLogD3("input(msf_confidenceMap): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)msfCfg.msf_confidenceMap->getBufVA(0), (void*)msfCfg.msf_confidenceMap->getBufPA(0),
                    // fmt
                    msfCfg.msf_confidenceMap->getImgFormat(),
                    // size
                    msfCfg.msf_confidenceMap->getImgSize().w, msfCfg.msf_confidenceMap->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_confidenceMap->getPlaneCount(), msfCfg.msf_confidenceMap->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_dsFrame frame */
        if (msfCfg.msf_dsFrame){
            mfllLogD3("input(msf_dsFrame): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)msfCfg.msf_dsFrame->getBufVA(0), (void*)msfCfg.msf_dsFrame->getBufPA(0),
                    // fmt
                    msfCfg.msf_dsFrame->getImgFormat(),
                    // size
                    msfCfg.msf_dsFrame->getImgSize().w, msfCfg.msf_dsFrame->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_dsFrame->getPlaneCount(), msfCfg.msf_dsFrame->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_baseFrame frame */
        for (int i = 0 ; i < msfCfg.msf_baseFrame.size() ; i++) {
            if (msfCfg.msf_baseFrame[i] == nullptr) {
                mfllLogD3("input(msf_baseFrame[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("input(msf_baseFrame[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_baseFrame[i]->getBufVA(0), (void*)msfCfg.msf_baseFrame[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_baseFrame[i]->getImgFormat(),
                    // size
                    msfCfg.msf_baseFrame[i]->getImgSize().w, msfCfg.msf_baseFrame[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_baseFrame[i]->getPlaneCount(), msfCfg.msf_baseFrame[i]->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_refFrame frame */
        for (int i = 0 ; i < msfCfg.msf_refFrame.size() ; i++) {
            if (msfCfg.msf_refFrame[i] == nullptr) {
                mfllLogD3("input(msf_refFrame[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("input(msf_refFrame[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_refFrame[i]->getBufVA(0), (void*)msfCfg.msf_refFrame[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_refFrame[i]->getImgFormat(),
                    // size
                    msfCfg.msf_refFrame[i]->getImgSize().w, msfCfg.msf_refFrame[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_refFrame[i]->getPlaneCount(), msfCfg.msf_refFrame[i]->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_weightingMap_in frame */
        for (int i = 0 ; i < msfCfg.msf_weightingMap_in.size() ; i++) {
            if (msfCfg.msf_weightingMap_in[i] == nullptr) {
                mfllLogD3("input(msf_weightingMap_in[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("input(msf_weightingMap_in[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_weightingMap_in[i]->getBufVA(0), (void*)msfCfg.msf_weightingMap_in[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_weightingMap_in[i]->getImgFormat(),
                    // size
                    msfCfg.msf_weightingMap_in[i]->getImgSize().w, msfCfg.msf_weightingMap_in[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_weightingMap_in[i]->getPlaneCount(), msfCfg.msf_weightingMap_in[i]->getBufStridesInBytes(0)
                    );
        }

        /* output: msf_outFrame frame */
        for (int i = 0 ; i < msfCfg.msf_outFrame.size() ; i++) {
            if (msfCfg.msf_outFrame[i] == nullptr) {
                mfllLogD3("output(msf_outFrame[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("output(msf_outFrame[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_outFrame[i]->getBufVA(0), (void*)msfCfg.msf_outFrame[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_outFrame[i]->getImgFormat(),
                    // size
                    msfCfg.msf_outFrame[i]->getImgSize().w, msfCfg.msf_outFrame[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_outFrame[i]->getPlaneCount(), msfCfg.msf_outFrame[i]->getBufStridesInBytes(0)
                    );
        }

        /* output: msf_weightingMap_out frame */
        for (int i = 0 ; i < msfCfg.msf_weightingMap_out.size() ; i++) {
            if (msfCfg.msf_weightingMap_out[i] == nullptr) {
                mfllLogD3("output(msf_weightingMap_out[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("output(msf_weightingMap_out[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_weightingMap_out[i]->getBufVA(0), (void*)msfCfg.msf_weightingMap_out[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_weightingMap_out[i]->getImgFormat(),
                    // size
                    msfCfg.msf_weightingMap_out[i]->getImgSize().w, msfCfg.msf_weightingMap_out[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_weightingMap_out[i]->getPlaneCount(), msfCfg.msf_weightingMap_out[i]->getBufStridesInBytes(0)
                    );
        }

        /* output: msf_weightingMap_ds frame */
        for (int i = 0 ; i < msfCfg.msf_weightingMap_ds.size() ; i++) {
            if (msfCfg.msf_weightingMap_ds[i] == nullptr) {
                mfllLogD3("output(msf_weightingMap_ds[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("output(msf_weightingMap_ds[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_weightingMap_ds[i]->getBufVA(0), (void*)msfCfg.msf_weightingMap_ds[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_weightingMap_ds[i]->getImgFormat(),
                    // size
                    msfCfg.msf_weightingMap_ds[i]->getImgSize().w, msfCfg.msf_weightingMap_ds[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_weightingMap_ds[i]->getPlaneCount(), msfCfg.msf_weightingMap_ds[i]->getBufStridesInBytes(0)
                    );
        }

        {
            err = generateNDDHint(m_nddHintMsf, m_blendCount);
            if (err != MfllErr_Ok)
                return err;
            msfCfg.dumphint = &m_nddHintMsf;
            msfCfg.dump_en = DRV_MSF_REG_NEED_DUMP;
        }

        rMsfParams.mEGNConfigVec.push_back(msfCfg);
        rMsfParams.mpEngineID = eMSF;
    }

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::generateMsfP2Param
(
        vector<IImageBuffer *>  base,
        vector<IImageBuffer *>  ref,
        vector<IImageBuffer *>  out,
        vector<IImageBuffer *>  wt_in,
        vector<IImageBuffer *>  wt_out,
        vector<IImageBuffer *>  wt_ds,
        IImageBuffer*           difference,
        IImageBuffer*           confmap,
        IImageBuffer*           dsFrame,
        EGNParams<MSFConfig>&   rMsfParams,
        MINT32                  stage)
{
    enum MfllErr err = MfllErr_Ok;

    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    if (base.size() <= 0 || ref.size() <= 0 || out.size() <= 0
        || wt_in.size() <= 0 || wt_out.size() <= 0 || wt_ds.size() <= 0
        || difference == nullptr || confmap == nullptr || dsFrame == nullptr) {
        mfllLogE("msf input buffer size is null.");
        return MfllErr_BadArgument;
    }

    if (base.size() != ref.size() || base.size() != out.size()) {
        mfllLogE("msf pyramid size is not sync.");
        return MfllErr_BadArgument;
    }

    if (wt_in.size() != wt_out.size() || wt_in.size() != wt_ds.size()) {
        mfllLogE("msf weighting table size is not sync.");
        return MfllErr_BadArgument;
    }

    MINT32 lastStage = base.size() -1;

    if (stage < 0 || stage >= lastStage) {
        mfllLogE("msf stage(%d) is out of range(0 ~ %d).", stage, lastStage);
        return MfllErr_BadArgument;
    }

    /**
     *  Stand alone MSF
     *
     *  Configure input parameters and execute
     */
    {
        MSFConfig msfCfg;
        msfCfg.msf_scenario = MSF_MFNR_DL_MODE;
        msfCfg.frame_Idx = m_blendCount+1;
        msfCfg.frame_Total = m_pCore->getFrameCapturedNum();
        msfCfg.scale_Idx = stage;
        msfCfg.scale_Total = base.size();

        mfllLogD3("MSFConfig.msf_scenario = %d", (int)msfCfg.msf_scenario);
        mfllLogD3("MSFConfig.frame_Idx = %u", msfCfg.frame_Idx);
        mfllLogD3("MSFConfig.frame_Total = %u", msfCfg.frame_Total);
        mfllLogD3("MSFConfig.scale_Idx = %u", msfCfg.scale_Idx);
        mfllLogD3("MSFConfig.scale_Total = %u",  msfCfg.scale_Total);

        // tuning
        msfCfg.msf_tuningBuf.clear();
        {
            if (!m_tuningBufMsf[m_blendCount][stage].isInited) {
                mfllLogE("msf(%u, %d) tuning data is not init", m_blendCount, stage);
                err = MfllErr_BadArgument;
                goto lbExit;
            }
            msfCfg.msf_tuningBuf.push_back(static_cast<unsigned int*>((void*)m_tuningBufMsf[m_blendCount][stage].data.get()));
        }
        // msf tbl
        msfCfg.msf_sramTable.clear();
        {
            if (!m_tuningBufMsfTbl[m_blendCount][stage].isInited) {
                mfllLogE("msftbl(%u, %d) is not init", m_blendCount, stage);
                err = MfllErr_BadArgument;
                goto lbExit;
            }
            msfCfg.msf_sramTable.push_back(static_cast<unsigned int*>((void*)m_tuningBufMsfTbl[m_blendCount][stage].data.get()));
        }

        msfCfg.msf_imageDifference = difference;
        msfCfg.msf_confidenceMap = confmap;
        msfCfg.msf_dsFrame = (stage+2 < base.size())?out[stage+1]:base[stage+1]; //base-s6, out-s5, out-s4,... , out-s1

        msfCfg.msf_baseFrame.clear();
        msfCfg.msf_refFrame.clear();
        msfCfg.msf_outFrame.clear();
        msfCfg.msf_weightingMap_in.clear();
        msfCfg.msf_weightingMap_out.clear();
        msfCfg.msf_weightingMap_ds.clear();

        msfCfg.msf_baseFrame.push_back(base[stage]);
        msfCfg.msf_refFrame.push_back(ref[stage]);
        //msfCfg.msf_outFrame keeps empty
        msfCfg.msf_weightingMap_in.push_back(wt_in[stage]);
        //msfCfg.msf_weighting keeps empty
        msfCfg.msf_weightingMap_ds.push_back((stage>0)?wt_ds[stage]:nullptr);
        msfCfg.msf_weightingMap_ds.push_back((stage+1<wt_ds.size())?wt_ds[stage+1]:nullptr);


        /*****************************************************************************
         * Param dumnp start                                                         *
         *****************************************************************************/
        mfllLogI("%s is ready for index(%u-%d)", __FUNCTION__, m_blendCount, stage);

        /* input: msf_imageDifference frame */
        if (msfCfg.msf_imageDifference){
            mfllLogD3("input(msf_imageDifference): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)msfCfg.msf_imageDifference->getBufVA(0), (void*)msfCfg.msf_imageDifference->getBufPA(0),
                    // fmt
                    msfCfg.msf_imageDifference->getImgFormat(),
                    // size
                    msfCfg.msf_imageDifference->getImgSize().w, msfCfg.msf_imageDifference->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_imageDifference->getPlaneCount(), msfCfg.msf_imageDifference->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_confidenceMap frame */
        if (msfCfg.msf_confidenceMap){
            mfllLogD3("input(msf_confidenceMap): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)msfCfg.msf_confidenceMap->getBufVA(0), (void*)msfCfg.msf_confidenceMap->getBufPA(0),
                    // fmt
                    msfCfg.msf_confidenceMap->getImgFormat(),
                    // size
                    msfCfg.msf_confidenceMap->getImgSize().w, msfCfg.msf_confidenceMap->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_confidenceMap->getPlaneCount(), msfCfg.msf_confidenceMap->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_dsFrame frame */
        if (msfCfg.msf_dsFrame){
            mfllLogD3("input(msf_dsFrame): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    // va/pa
                    (void*)msfCfg.msf_dsFrame->getBufVA(0), (void*)msfCfg.msf_dsFrame->getBufPA(0),
                    // fmt
                    msfCfg.msf_dsFrame->getImgFormat(),
                    // size
                    msfCfg.msf_dsFrame->getImgSize().w, msfCfg.msf_dsFrame->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_dsFrame->getPlaneCount(), msfCfg.msf_dsFrame->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_baseFrame frame */
        for (int i = 0 ; i < msfCfg.msf_baseFrame.size() ; i++) {
            if (msfCfg.msf_baseFrame[i] == nullptr) {
                mfllLogD3("input(msf_baseFrame[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("input(msf_baseFrame[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_baseFrame[i]->getBufVA(0), (void*)msfCfg.msf_baseFrame[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_baseFrame[i]->getImgFormat(),
                    // size
                    msfCfg.msf_baseFrame[i]->getImgSize().w, msfCfg.msf_baseFrame[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_baseFrame[i]->getPlaneCount(), msfCfg.msf_baseFrame[i]->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_refFrame frame */
        for (int i = 0 ; i < msfCfg.msf_refFrame.size() ; i++) {
            if (msfCfg.msf_refFrame[i] == nullptr) {
                mfllLogD3("input(msf_refFrame[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("input(msf_refFrame[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_refFrame[i]->getBufVA(0), (void*)msfCfg.msf_refFrame[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_refFrame[i]->getImgFormat(),
                    // size
                    msfCfg.msf_refFrame[i]->getImgSize().w, msfCfg.msf_refFrame[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_refFrame[i]->getPlaneCount(), msfCfg.msf_refFrame[i]->getBufStridesInBytes(0)
                    );
        }

        /* input: msf_weightingMap_in frame */
        for (int i = 0 ; i < msfCfg.msf_weightingMap_in.size() ; i++) {
            if (msfCfg.msf_weightingMap_in[i] == nullptr) {
                mfllLogD3("input(msf_weightingMap_in[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("input(msf_weightingMap_in[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_weightingMap_in[i]->getBufVA(0), (void*)msfCfg.msf_weightingMap_in[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_weightingMap_in[i]->getImgFormat(),
                    // size
                    msfCfg.msf_weightingMap_in[i]->getImgSize().w, msfCfg.msf_weightingMap_in[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_weightingMap_in[i]->getPlaneCount(), msfCfg.msf_weightingMap_in[i]->getBufStridesInBytes(0)
                    );
        }

        /* output: msf_outFrame frame */
        for (int i = 0 ; i < msfCfg.msf_outFrame.size() ; i++) {
            if (msfCfg.msf_outFrame[i] == nullptr) {
                mfllLogD3("output(msf_outFrame[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("output(msf_outFrame[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_outFrame[i]->getBufVA(0), (void*)msfCfg.msf_outFrame[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_outFrame[i]->getImgFormat(),
                    // size
                    msfCfg.msf_outFrame[i]->getImgSize().w, msfCfg.msf_outFrame[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_outFrame[i]->getPlaneCount(), msfCfg.msf_outFrame[i]->getBufStridesInBytes(0)
                    );
        }

        /* output: msf_weightingMap_out frame */
        for (int i = 0 ; i < msfCfg.msf_weightingMap_out.size() ; i++) {
            if (msfCfg.msf_weightingMap_out[i] == nullptr) {
                mfllLogD3("output(msf_weightingMap_out[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("output(msf_weightingMap_out[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_weightingMap_out[i]->getBufVA(0), (void*)msfCfg.msf_weightingMap_out[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_weightingMap_out[i]->getImgFormat(),
                    // size
                    msfCfg.msf_weightingMap_out[i]->getImgSize().w, msfCfg.msf_weightingMap_out[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_weightingMap_out[i]->getPlaneCount(), msfCfg.msf_weightingMap_out[i]->getBufStridesInBytes(0)
                    );
        }

        /* output: msf_weightingMap_ds frame */
        for (int i = 0 ; i < msfCfg.msf_weightingMap_ds.size() ; i++) {
            if (msfCfg.msf_weightingMap_ds[i] == nullptr) {
                mfllLogD3("output(msf_weightingMap_ds[%d]): nullptr", i);
                continue;
            }
            mfllLogD3("output(msf_weightingMap_ds[%d]): va/pa(%p/%p) fmt(%#x), size=(%d,%d), planes(%zu), stride(%zu)",
                    i,
                    // va/pa
                    (void*)msfCfg.msf_weightingMap_ds[i]->getBufVA(0), (void*)msfCfg.msf_weightingMap_ds[i]->getBufPA(0),
                    // fmt
                    msfCfg.msf_weightingMap_ds[i]->getImgFormat(),
                    // size
                    msfCfg.msf_weightingMap_ds[i]->getImgSize().w, msfCfg.msf_weightingMap_ds[i]->getImgSize().h,
                    // planes, stride
                    msfCfg.msf_weightingMap_ds[i]->getPlaneCount(), msfCfg.msf_weightingMap_ds[i]->getBufStridesInBytes(0)
                    );
        }

        {
            err = generateNDDHint(m_nddHintMsf, m_blendCount/*, stage*/); //MSF keep the same naming
            if (err != MfllErr_Ok)
                return err;
            msfCfg.dumphint = &m_nddHintMsf;
            msfCfg.dump_en = DRV_MSF_REG_NEED_DUMP;
        }

#if DEBUG_MSF_P2_DUMP
        //dump binary
        auto dump2Binary = [this](MINT32 uniqueKey, MINT32 requestNum, MINT32 frameNum, MINT32 stage, const char* buf, size_t size, string fn) -> bool {
            char filepath[256] = {0};
            snprintf(filepath, sizeof(filepath)-1, "%s/%09d-%04d-%04d-stage%d-%s.bin", "/data/vendor/camera_dump", uniqueKey, requestNum, frameNum, stage, fn.c_str());
            std::ofstream ofs (filepath, std::ofstream::binary);
            if (!ofs.is_open()) {
                mfllLogW("dump2Binary: open file(%s) fail", filepath);
                return false;
                }
            ofs.write(buf, size);
            ofs.close();
            return true;
        };

        if (!dump2Binary(m_nddHintMsf.UniqueKey, m_nddHintMsf.RequestNo, m_nddHintMsf.FrameNo, stage, (const char*)(m_tuningBufMsf[m_blendCount][stage].data.get()),    m_tuningBufMsf[m_blendCount][stage].size,   "msf-tuning"))
            mfllLogE("dump msf tuning fail...");
        if (!dump2Binary(m_nddHintMsf.UniqueKey, m_nddHintMsf.RequestNo, m_nddHintMsf.FrameNo, stage, (const char*)(m_tuningBufMsfTbl[m_blendCount][stage].data.get()), m_tuningBufMsfTbl[m_blendCount][stage].size,"msf-tbl"))
            mfllLogE("dump msf tbl fail...");
        if (!dump2Binary(m_nddHintMsf.UniqueKey, m_nddHintMsf.RequestNo, m_nddHintMsf.FrameNo, stage, (const char*)(m_tuningBufMsfP2[stage].data.get()),                m_tuningBufMsfP2[stage].size,               "p2-tuning"))
            mfllLogE("dump pt tuning fail...");
#endif

        rMsfParams.mEGNConfigVec.push_back(msfCfg);
        rMsfParams.mpEngineID = eMSF;
    }

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
MfllErr MfllMfb::generateNDDHint(FILE_DUMP_NAMING_HINT& hint, MINT32 frameIndex, MINT32 stage)
{
    enum MfllErr err = MfllErr_Ok;

    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    mfllLogD3("%s: frameIndex = %d, stage = %d", __FUNCTION__, frameIndex, stage);

    if (frameIndex < 0 || frameIndex >= MFLL_MAX_FRAMES) {
        mfllLogE("%s: frameIndex(%d) is out of range", __FUNCTION__, frameIndex);
        return MfllErr_UnexpectedError;
    }

    if (stage < 0 || stage >= MFLL_MSF_STAGE_MAX) {
        mfllLogE("%s: stage(%d) is out of range", __FUNCTION__, stage);
        return MfllErr_UnexpectedError;
    }

    /* get member resource here */
    m_mutex.lock();

    size_t index = m_pCore->getIndexByNewIndex(static_cast<const unsigned int>(m_blendCount));

    /* get metaset */
    if (index >= m_vMetaSet.size()) {
        mfllLogE("%s: index(%zu) is out of size of metaset(%zu)", __FUNCTION__, index, m_vMetaSet.size());
        m_mutex.unlock();
        return MfllErr_UnexpectedError;
    }
    MetaSet_T metaset = getMetasetByIdx(frameIndex);

    m_mutex.unlock();

    /******************************************************************************
     * Generate NDD hint                                                          *
     *****************************************************************************/

    memset(&hint, 0 , sizeof(FILE_DUMP_NAMING_HINT));

    generateNDDHintFromMetadata(hint, &metaset.halMeta);

    hint.IspProfile = get_isp_profile(eMfllIspProfile_Mfb, m_shotMode, stage);

    extract_by_SensorOpenId(&hint, m_sensorId);

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
MfllErr MfllMfb::initMfbTuningBuf(const vector<IImageBuffer *>& pylamid, IImageBuffer* dceso, MINT32 frameIndex)
{
    enum MfllErr err = MfllErr_Ok;

    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    std::unique_ptr<void, std::function<void(void*)> > _tagIspP2InFmtRestorer = nullptr;

    mfllLogD3("%s: frameIndex = %d", __FUNCTION__, frameIndex);

    if (frameIndex < 0 || frameIndex >= MFLL_MAX_FRAMES) {
        mfllLogE("%s: frameIndex(%d) is out of range", __FUNCTION__, frameIndex);
        return MfllErr_UnexpectedError;
    }

    bool bLastFrame = (frameIndex + 2 >= m_pCore->getFrameCapturedNum());
    if (bLastFrame && dceso == nullptr) {
        mfllLogE("%s: by-pass due to last frame without dceso", __FUNCTION__);
        return MfllErr_UnexpectedError;
    }

    /* get member resource here */
    m_mutex.lock();

    EIspProfile_T profile = (EIspProfile_T)0; // which will be set later.
    size_t index = m_pCore->getIndexByNewIndex(static_cast<const unsigned int>(m_blendCount));

    /* get metaset */
    if (index >= m_vMetaSet.size()) {
        mfllLogE("%s: index(%zu) is out of size of metaset(%zu)", __FUNCTION__, index, m_vMetaSet.size());
        m_mutex.unlock();
        return MfllErr_UnexpectedError;
    }
    MetaSet_T metaset = getMetasetByIdx(frameIndex);

    m_mutex.unlock();


    // MIX stage is always YUV->YUV
    MINT32 tagIspP2InFmt = setIspP2ImgFmtTag(
            metaset.halMeta,
            1,
            &_tagIspP2InFmtRestorer);

#if MSF_TUNING_IDENTICAL_UPDATE
    // P2 tuning update mdoe to bypass p2 for msf standalone (performace optimal)
    MUINT8 ispP2TuningUpdateMode = 0;
    {
        if (IMetadata::getEntry<MUINT8>(&metaset.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ispP2TuningUpdateMode))
            mfllLogD3("%s: ISP P2 Tuning Update Mode=%#x", __FUNCTION__, ispP2TuningUpdateMode);
        else
            mfllLogW("%s: ISP P2 Tuning Update Mode is nullptr, ISP manager is supposed to use default one", __FUNCTION__);

        // MTK_ISP_P2_TUNING_UPDATE_MODE = 1: EPartKeep         (msf stanalone)
        // MTK_ISP_P2_TUNING_UPDATE_MODE = 8: EIdenditySetting  (msf-p2 direct link)
        MUINT8 ispP2TuningUpdateMode_new = (frameIndex+2 < m_pCore->getFrameCapturedNum())?1:8;
        IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ispP2TuningUpdateMode_new);
        mfllLogD3("%s: set MTK_ISP_P2_TUNING_UPDATE_MODE to %#x", __FUNCTION__, ispP2TuningUpdateMode_new);
    }
#endif

    /**
     * Retrieve MSF tuning data
     */
    for (size_t stage = 0 ; stage < pylamid.size()-1; stage++) {
        profile = get_isp_profile(eMfllIspProfile_Mfb, m_shotMode, stage);
        if (!m_tuningBufMsf[frameIndex][stage].isInited) {
            if (pylamid[stage] == nullptr)
                return MfllErr_UnexpectedError;

            /* assign tuning buffers */
            TuningParam rTuningParam;
            rTuningParam.pMfbBuf = static_cast<void*>( m_tuningBufMsf[frameIndex][stage].data.get() );
            rTuningParam.pMsfTbl = static_cast<void*>( m_tuningBufMsfTbl[frameIndex][stage].data.get() );
            rTuningParam.pRegBuf = static_cast<void*>( m_tuningBufMsfP2[stage].data.get() );
            if (stage == 0)
                rTuningParam.pMssBuf = static_cast<void*>( m_tuningBufMss[frameIndex].data.get() );

            /* last frame, s0 apply dceso buffer*/
            if (bLastFrame && stage == 0) {
                MINT32 p1Magic = -1;
                IMetadata::getEntry<MINT32>(&metaset.halMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, p1Magic);
                mfllLogD3("%s: DCESO i4DcsMagicNo=%d, dcesoBuf=%p", __FUNCTION__, p1Magic, dceso);

                if (p1Magic >= 0 && dceso) {
                    rTuningParam.pDcsBuf = dceso;
                    rTuningParam.i4DcsMagicNo = p1Magic;
                }
            }

            /* update Metadata */
            IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_3A_PGN_ENABLE, MTRUE);
            IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_3A_ISP_PROFILE, profile);
            IMetadata::setEntry<MINT32>(&metaset.halMeta, MTK_MSF_SCALE_INDEX, stage);
            IMetadata::setEntry<MINT32>(&metaset.halMeta, MTK_MSF_FRAME_NUM, frameIndex);
            IMetadata::setEntry<MINT32>(&metaset.halMeta, MTK_TOTAL_MULTI_FRAME_NUM_CAPTURED  , m_pCore->getFrameCapturedNum()); //after drop
            IMetadata::setEntry<MINT32>(&metaset.halMeta, MTK_TOTAL_MULTI_FRAME_NUM           , m_pCore->getCaptureFrameNum());  //before drop
            MINT32 resizeYUV = (pylamid[stage]->getImgSize().w & 0x0000FFFF) | ((pylamid[stage]->getImgSize().h & 0x0000FFFF)<<16);
            mfllLogD3("%s: calc resize yuv for msf SLK, w = %x, h = %x, calc = %x", __FUNCTION__, pylamid[stage]->getImgSize().w, pylamid[stage]->getImgSize().h, resizeYUV);
            IMetadata::setEntry<MINT32>(&metaset.halMeta, MTK_ISP_P2_IN_IMG_RES_REVISED, resizeYUV);

            /* setIsp to retrieve tuning buffer */
            MetaSet_T rMetaSet; // saving output of setIsp
            MINT32 retSetIsp = 0;

            mfllTraceBegin("SetIsp");
            setIspP2ImgFmtTag(metaset.halMeta, 1); // set format to 1 (YUV->YUV)
            retSetIsp = this->setIsp(0, metaset, &rTuningParam, &rMetaSet);
            mfllTraceEnd(); // AfterSetIsp

            if (retSetIsp == 0) {
                MfbPlatformConfig pltcfg;
                pltcfg[ePLATFORMCFG_STAGE]          = STAGE_MFB;
                pltcfg[ePLATFORMCFG_DIP_X_REG_T]    = reinterpret_cast<intptr_t>((volatile void*)rTuningParam.pMfbBuf);
                //
                mfllTraceBegin("refine_register");
                if (!refine_register(pltcfg)) {
                    mfllLogE("%s: refine_register returns failed", __FUNCTION__);
                }
                else {
#ifdef __DEBUG
                    mfllLogD3("%s: refine_register ok", __FUNCTION__);
#endif
                }
                mfllTraceEnd();
            }
            else {
                mfllLogE("%s: set ISP profile(MFB) failed...", __FUNCTION__);
                err = MfllErr_UnexpectedError;
                goto lbExit;
            }
            mfllLogD3("%s: tuning buffer is ininted(%d)(%zu)", __FUNCTION__, frameIndex, stage);

            m_tuningBufMsf[frameIndex][stage].isInited = true; // marks initialized
            m_tuningBufMsfTbl[frameIndex][stage].isInited = true; // marks initialized
            m_tuningBufMss[frameIndex].isInited = true; // marks initialized
        }
    }

#if MSF_TUNING_IDENTICAL_UPDATE
    //restore p2 tuning update mode
    {
        IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ispP2TuningUpdateMode);
        mfllLogD3("%s: Restore ISP P2 Tuning Update Mode=%#x", __FUNCTION__, ispP2TuningUpdateMode);
    }
#endif

lbExit:
    // restore MTK_ISP_P2_IN_IMG_FMT
    _tagIspP2InFmtRestorer = nullptr;
    return err;
}
//
// ----------------------------------------------------------------------------
//
MfllErr MfllMfb::updateMSFTuningBuf(const vector<IImageBuffer *>& pylamid, const unsigned int meanLuma)
{
    enum MfllErr err = MfllErr_Ok;

    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    mfllLogD3("%s: meanLuma is %u", __FUNCTION__, meanLuma);

    if (__builtin_expect( m_pHalISP.get() == nullptr, false )) {
        mfllLogE("%s: m_pHalISP is nullptr", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    for (int stage = pylamid.size()-2 ; stage >= 0 ; stage--) {
        if (!m_tuningBufMsf[m_blendCount][stage].isInited) {
            mfllLogE("msf(%u, %d) tuning data is not init", m_blendCount, stage);
            err = MfllErr_BadArgument;
            goto lbExit;
        }
        m_pHalISP->sendIspCtrl(EISPCtrl_GetMsfTuning_With_Luma, reinterpret_cast<MINTPTR>(m_tuningBufMsf[m_blendCount][stage].data.get()), reinterpret_cast<MINTPTR>(&meanLuma));
    }

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
IMetadata* MfllMfb::getMainAppMetaLocked() const
{
    /* always use the best frame metadata */
    size_t index = m_pCore->getIndexByNewIndex(0);
    /* get metaset */
    if (__builtin_expect( index >= m_vMetaSet.size(), false )) {
        mfllLogE("%s: index(%zu) is out of size of metaset(%zu)", __FUNCTION__, index, m_vMetaSet.size());
        return nullptr;
    }
    return m_vMetaApp[index];
}
//
// ----------------------------------------------------------------------------
//
IMetadata* MfllMfb::getMainHalMetaLocked() const
{
    /* always use the best frame metadata */
    size_t index = m_pCore->getIndexByNewIndex(0);
    /* get metaset */
    if (__builtin_expect( index >= m_vMetaSet.size(), false )) {
        mfllLogE("%s: index(%zu) is out of size of metaset(%zu)", __FUNCTION__, index, m_vMetaSet.size());
        return nullptr;
    }
    return m_vMetaHal[index];
}
//
// ----------------------------------------------------------------------------
//
MetaSet_T MfllMfb::getMainMetasetLocked() const
{
    /* always use the best frame metadata */
    size_t index = m_pCore->getIndexByNewIndex(0);
    /* get metaset */
    if (__builtin_expect( index >= m_vMetaSet.size(), false )) {
        mfllLogE("%s: index(%zu) is out of size of metaset(%zu)", __FUNCTION__, index, m_vMetaSet.size());
        return MetaSet_T();
    }
    return m_vMetaSet[index];
}
//
// ----------------------------------------------------------------------------
//
MetaSet_T MfllMfb::getMetasetByIdx(MINT32 idx) const
{
    size_t index = m_pCore->getIndexByNewIndex(idx);
    /* get metaset */
    if (__builtin_expect( index >= m_vMetaSet.size(), false )) {
        mfllLogE("%s: index(%zu) is out of size of metaset(%zu)", __FUNCTION__, index, m_vMetaSet.size());
        return MetaSet_T();
    }
    return m_vMetaSet[index];

}
//
// ----------------------------------------------------------------------------
//
MINT32 MfllMfb::setIsp(
        MINT32          flowType,
        MetaSet_T&      control,
        TuningParam*    pTuningBuf,
        MetaSet_T*      pResult /* = nullptr */,
        IImageBuffer*   pSrcImage /* = nullptr */)
{
    MINT32 r = MTRUE;

    if (__builtin_expect( m_pHalISP.get() == nullptr, false )) {
        mfllLogE("%s: m_pHalISP is nullptr", __FUNCTION__);
        mfllDumpStack(__FUNCTION__);
        return -1;
    }

    if (pSrcImage) {
        // since P2 input image may be not a full size image, the tuning manager
        // must know the input image resolution for updating the parameters from
        // full size domain to customized domain.
        updateP2RevisedTag(pSrcImage, &control.halMeta);
    }

    r = m_pHalISP->setP2Isp(flowType, control, pTuningBuf, pResult);

    if (pSrcImage) {
        // make sure these P2 revised tag has been removed to avoid ambiguous of
        // the following users.
        removeP2RevisedTag(&control.halMeta);
        if (pResult)
            removeP2RevisedTag(&pResult->halMeta);
    }
    return r;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::mix(IMfllImageBuffer *base, IMfllImageBuffer *ref, IMfllImageBuffer *out, IMfllImageBuffer *wt)
{
    return mix(base, ref, out, wt, nullptr);
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::mix(IMfllImageBuffer *base, IMfllImageBuffer *ref, IMfllImageBuffer *out, IMfllImageBuffer *wt, IMfllImageBuffer *dceso)
{
    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    enum MfllErr err = MfllErr_NotImplemented;

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::mix(
        IMfllImageBuffer *base,
        IMfllImageBuffer *ref,
        IMfllImageBuffer *out_main,
        IMfllImageBuffer *out_thumbnail,
        IMfllImageBuffer *wt,
        const MfllRect_t& output_thumb_crop
        )
{
        return mix(base, ref, out_main, out_thumbnail, wt, nullptr, output_thumb_crop);
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::mix(
        IMfllImageBuffer *base,
        IMfllImageBuffer *ref,
        IMfllImageBuffer *out_main,
        IMfllImageBuffer *out_thumbnail,
        IMfllImageBuffer *wt,
        IMfllImageBuffer *dceso,
        const MfllRect_t& output_thumb_crop
        )
{
    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    enum MfllErr err = MfllErr_NotImplemented;

lbExit:
    return err;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::setSyncPrivateData(const std::deque<void*>& dataset)
{
    mfllLogD3("dataset size=%zu", dataset.size());
    if (dataset.size() <= 0) {
        mfllLogW("set sync private data receieve NULL, ignored");
        return MfllErr_Ok;
    }

    Mutex::Autolock _l(m_mutex);

    /**
     *  dataset is supposed to be contained two address, the first one is the
     *  address of App IMetadata, the second one is Hal. Please make sure the
     *  caller also send IMetadata addresses.
     */
    if (dataset.size() < 3) {
        mfllLogE(
                "%s: missed data in dataset, dataset size is %zu, " \
                "and it's supposed to be: "                         \
                "[0]: addr of app metadata, "                       \
                "[1]: addr of hal metadata, "                       \
                "[2]: addr of an IImageBuffer of LCSO",
                __FUNCTION__, dataset.size());
        return MfllErr_BadArgument;
    }
    IMetadata *pAppMeta = static_cast<IMetadata*>(dataset[0]);
    IMetadata *pHalMeta = static_cast<IMetadata*>(dataset[1]);
    IImageBuffer *pLcsoImg = static_cast<IImageBuffer*>(dataset[2]);

    MetaSet_T m;
    m.halMeta = *pHalMeta;
    m.appMeta = *pAppMeta;
    m_vMetaSet.push_back(m);
    m_vMetaApp.push_back(pAppMeta);
    m_vMetaHal.push_back(pHalMeta);
    m_vImgLcsoRaw.push_back(pLcsoImg);
    mfllLogD3("saves MetaSet_T, size = %zu", m_vMetaSet.size());
    return MfllErr_Ok;
}
//
// ----------------------------------------------------------------------------
//
enum MfllErr MfllMfb::setMixDebug(sp<IMfllImageBuffer> buffer)
{
    Mutex::Autolock _l(m_mutex);

    m_pMixDebugBuffer = buffer;

    return MfllErr_Ok;
}

/******************************************************************************
 * encodeRawToYuv
 *
 * Interface for encoding a RAW buffer to an YUV buffer
 *****************************************************************************/
enum MfllErr MfllMfb::encodeRawToYuv(IMfllImageBuffer *input, IMfllImageBuffer *output, const enum YuvStage &s)
{
    return encodeRawToYuv(
            input,
            output,
            NULL,
            MfllRect_t(),
            MfllRect_t(),
            s);
}

/******************************************************************************
 * encodeRawToYuv
 *
 * Interface for encoding a RAW buffer to two YUV buffers
 *****************************************************************************/
enum MfllErr MfllMfb::encodeRawToYuv(
            IMfllImageBuffer *input,
            IMfllImageBuffer *output,
            IMfllImageBuffer *output_q,
            const MfllRect_t& output_crop,
            const MfllRect_t& output_q_crop,
            enum YuvStage s /* = YuvStage_RawToYuy2 */)
{
    enum MfllErr err = MfllErr_Ok;
    IImageBuffer *iimgOut2 = NULL;

    if (input == NULL) {
        mfllLogE("%s: input buffer is NULL", __FUNCTION__);
        err = MfllErr_BadArgument;
    }
    if (output == NULL) {
        mfllLogE("%s: output buffer is NULL", __FUNCTION__);
        err = MfllErr_BadArgument;
    }
    if (err != MfllErr_Ok)
        goto lbExit;

    if (output_q) {
        iimgOut2 = (IImageBuffer*)output_q->getImageBuffer();
    }

    err = encodeRawToYuv(
            (IImageBuffer*)input->getImageBuffer(),
            (IImageBuffer*)output->getImageBuffer(),
            iimgOut2,
            NULL, // dst3
            NULL,
            output_crop,
            output_q_crop,
            output_q_crop,
            s);

lbExit:
    return err;

}

/******************************************************************************
 * encodeRawToYuv 3
 *
 * Interface for encoding a RAW buffer to two 3 YUV buffers
 *****************************************************************************/
enum MfllErr MfllMfb::encodeRawToYuv(
            IMfllImageBuffer *input,
            IMfllImageBuffer *output,
            IMfllImageBuffer *output2,
            IMfllImageBuffer *output3,
            const MfllRect_t& output2_crop,
            const MfllRect_t& output3_crop,
            enum YuvStage s /* = YuvStage_RawToYuy2 */)
{
    enum MfllErr err = MfllErr_Ok;

    if (input == NULL || output == NULL || output2 == NULL) {
        mfllLogE("%s: input or output buffer is NULL", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    if (input->getWidth() != output->getWidth() ||
        input->getHeight() != output->getHeight()) {
        mfllLogD("%s: the size between input and output is not the same", __FUNCTION__);
    }


    MfllRect_t r;

    err = encodeRawToYuv(
            (IImageBuffer*)input    ->getImageBuffer(),
            (IImageBuffer*)output   ->getImageBuffer(),
            (IImageBuffer*)output2  ->getImageBuffer(),
            output3 ? (IImageBuffer*)output3  ->getImageBuffer() : NULL,
            NULL,
            r,
            output2_crop,
            output3_crop,
            s);

    return err;
}
/******************************************************************************
 * encodeRawToYuv 3 + dceso
 *
 * Interface for encoding a RAW buffer to two 3 YUV buffers + 1 DCESO buffer
 *****************************************************************************/
enum MfllErr MfllMfb::encodeRawToYuv(
            IMfllImageBuffer *input,
            IMfllImageBuffer *output,
            IMfllImageBuffer *output2,
            IMfllImageBuffer *output3,
            IMfllImageBuffer *dceso,
            const MfllRect_t& output2_crop,
            const MfllRect_t& output3_crop,
            enum YuvStage s /* = YuvStage_RawToYuy2 */)
{
    enum MfllErr err = MfllErr_Ok;

    if (input == NULL || output == NULL || dceso == NULL) {
        mfllLogE("%s: input or output or dceso buffer is NULL", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    if (input->getWidth() != output->getWidth() ||
        input->getHeight() != output->getHeight()) {
        mfllLogD("%s: the size between input and output is not the same", __FUNCTION__);
    }


    MfllRect_t r;

    err = encodeRawToYuv(
            (IImageBuffer*)input    ->getImageBuffer(),
            (IImageBuffer*)output   ->getImageBuffer(),
            output2 ? (IImageBuffer*)output2->getImageBuffer() : NULL,
            output3 ? (IImageBuffer*)output3->getImageBuffer() : NULL,
            (IImageBuffer*)dceso    ->getImageBuffer(),
            r,
            output2_crop,
            output3_crop,
            s);

    return err;
}
/******************************************************************************
 * encodeRawToYuv
 *
 * Real implmentation that to control PASS 2 drvier for encoding RAW to YUV
 *****************************************************************************/
enum MfllErr MfllMfb::encodeRawToYuv(
        IImageBuffer *src,
        IImageBuffer *dst,
        IImageBuffer *dst2,
        IImageBuffer *dst3,
        IImageBuffer *dceso,
        const MfllRect_t& output_crop,
        const MfllRect_t& output_q_crop,
        const MfllRect_t& output_3_crop,
        const enum YuvStage &s)
{
    enum MfllErr err = MfllErr_Ok;
    MBOOL bRet = MTRUE;
    EIspProfile_T profile = get_isp_profile(eMfllIspProfile_BeforeBlend, m_shotMode);

    mfllAutoLogFunc();
    mfllTraceCall();

    /* If it's encoding base RAW ... */
    const bool bBaseYuvStage = (s == YuvStage_BaseYuy2 || s == YuvStage_GoldenYuy2) ? true : false;

    m_mutex.lock();
    int index = m_encodeYuvCount++; // describes un-sorted index
    int sensorId = m_sensorId;
    IImageBuffer *pInput = src;
    IImageBuffer *pOutput = dst;
    IImageBuffer *pOutputQ = dst2;
    IImageBuffer *pOutput3 = dst3;
    IImageBuffer *pDceso = dceso;
    IImageBuffer *pImgLcsoRaw = nullptr;
    /* Do not increase YUV stage if it's encoding base YUV or golden YUV */
    if (bBaseYuvStage) {
        m_encodeYuvCount--;
        index = m_pCore->getIndexByNewIndex(0); // Use the first metadata
        pImgLcsoRaw = m_vImgLcsoRaw[index]; // use the mapped LCSO buffer
    }
    else {
        /* RAW may be sorted, retrieve the new order by index */
        pImgLcsoRaw = m_vImgLcsoRaw[m_pCore->getIndexByNewIndex(index)];
    }

    /* check if metadata is ok */
    if ((size_t)index >= m_vMetaSet.size()) {
        mfllLogE("%s: index(%d) is out of metadata set size(%zu) ",
                __FUNCTION__,
                index,
                m_vMetaSet.size());
        m_mutex.unlock();
        return MfllErr_UnexpectedError;
    }
    MetaSet_T metaset = getMainMetasetLocked(); // uses main frame's metadata set.
    IMetadata *pHalMeta = getMainHalMetaLocked(); // uses main frame's HAL metadata.
    m_mutex.unlock();

    bool bIsYuv2Yuv = !isRawFormat(static_cast<EImageFormat>(pInput->getImgFormat()));

    /**
     *  Select profile based on Stage:
     *  1) Raw to Yv16
     *  2) Encoding base YUV
     *  3) Encoding golden YUV
     *
     */
    mfllTraceBegin("get_isp_profile");
    switch(s) {
    case YuvStage_RawToYuy2:
    case YuvStage_RawToYv16:
    case YuvStage_BaseYuy2:
        if (isZsdMode(m_shotMode)) {
            profile = get_isp_profile(eMfllIspProfile_BeforeBlend_Zsd, m_shotMode);// Not related with MNR
            if (IS_WRONG_ISP_PROFILE(profile)) {
                mfllLogE("%s: error isp profile mapping", __FUNCTION__);
            }
        }
        else {
            profile = get_isp_profile(eMfllIspProfile_BeforeBlend, m_shotMode);
            if (IS_WRONG_ISP_PROFILE(profile)) {
                mfllLogE("%s: error isp profile mapping", __FUNCTION__);
            }
        }
        break;
    case YuvStage_GoldenYuy2:
        if (isZsdMode(m_shotMode)) {
            profile = (m_nrType == NoiseReductionType_SWNR)
                ? get_isp_profile(eMfllIspProfile_Single_Zsd_Swnr, m_shotMode)
                : get_isp_profile(eMfllIspProfile_Single_Zsd, m_shotMode);
            if (IS_WRONG_ISP_PROFILE(profile)) {
                mfllLogE("%s: error isp profile mapping", __FUNCTION__);
            }
        }
        else {
            profile = (m_nrType == NoiseReductionType_SWNR)
                ? get_isp_profile(eMfllIspProfile_Single_Swnr, m_shotMode)
                : get_isp_profile(eMfllIspProfile_Single, m_shotMode);
            if (IS_WRONG_ISP_PROFILE(profile)) {
                mfllLogE("%s: error isp profile mapping", __FUNCTION__);
            }
        }
        break;
    default:
        // do nothing
        break;
    } // switch
    mfllTraceEnd(); // get_isp_profile

    /* query input/output port mapping */
    MfllMfbPortInfoMap portMap;
    if (!get_port_map(m_pCore, s, portMap)) {
        mfllLogE("%s: cannot get correct port mapping for stage(%d)", __FUNCTION__, s);
        return MfllErr_UnexpectedError;
    }
    else{
        if(portMap.inputPorts.size() < 1) {
            mfllLogE("%s: input port map size(%zu) is invalid for stage(%d)",
                     __FUNCTION__, portMap.inputPorts.size(), s);
            return MfllErr_UnexpectedError;
        }

        if(portMap.outputPorts.size() < 3) {
            mfllLogE("%s: output port map size(%zu) is invalid for stage(%d)",
                     __FUNCTION__, portMap.outputPorts.size(), s);
            return MfllErr_UnexpectedError;
        }
    }


    /**
     *  Ok, we're going to configure P2 driver
     */
    QParams     qParams;
    FrameParams params;
    PQParam     p2PqParam;
    std::unique_ptr<_SRZ_SIZE_INFO_> srzParam(new _SRZ_SIZE_INFO_); // for SRZ4


    /* If using ZSD mode, to use Vss profile can improve performance */
    params.mStreamTag = (isZsdMode(m_shotMode)
            ? NSCam::NSIoPipe::NSPostProc::ENormalStreamTag_Vss
            : NSCam::NSIoPipe::NSPostProc::ENormalStreamTag_Normal
            );

    /* get ready to operate P2 driver, it's a long stroy ... */
    {
        std::unique_ptr<char[]>& tuning_reg = m_tuningBuf.data;


        MUINT8 ispP2TuningUpdateMode = 0;
        {
            if (IMetadata::getEntry<MUINT8>(&metaset.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ispP2TuningUpdateMode)) {
                mfllLogD3("%s: ISP P2 Tuning Update Mode=%#x", __FUNCTION__, ispP2TuningUpdateMode);
            }
            else {
                mfllLogW("%s: ISP P2 Tuning Update Mode is nullptr, ISP manager is supposed to use default one", __FUNCTION__);
            }
        }

        // Fix BFBLD parameter
        if (CC_LIKELY( !bBaseYuvStage )) {
            if (CC_LIKELY( m_tuningBuf.isInited )) {
                // MTK_ISP_P2_TUNING_UPDATE_MODE = 2: keep all existed parameters (force mode)
                IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, 2);
                mfllLogD("%s: set MTK_ISP_P2_TUNING_UPDATE_MODE to 2", __FUNCTION__);
            }
            else {
                mfllLogD("%s: no need to set MTK_ISP_P2_TUNING_UPDATE_MODE since m_tuningBuf not inited", __FUNCTION__);
            }
            m_tuningBuf.isInited = true;
        }

        void *tuning_lsc;

        MfbPlatformConfig pltcfg;
        pltcfg[ePLATFORMCFG_STAGE] = [s]
        {
            switch (s) {
            case YuvStage_RawToYuy2:
            case YuvStage_RawToYv16:
            case YuvStage_BaseYuy2:     return STAGE_BASE_YUV;
            case YuvStage_GoldenYuy2:   return STAGE_GOLDEN_YUV;
            case YuvStage_Unknown:
            case YuvStage_Size:
                mfllLogE("Not support YuvStage %d", s);
            }
            return 0;
        }();
        pltcfg[ePLATFORMCFG_INDEX] = index;

        {
            TuningParam rTuningParam;
            rTuningParam.pRegBuf = tuning_reg.get();
            rTuningParam.pLcsBuf = pImgLcsoRaw; // gives LCS buffer, even though it's NULL.
            rTuningParam.pBpc2Buf= m_pBPCI; // always use the previouse BPCI buffer
            IMetadata::setEntry<MUINT8>(
                    &metaset.halMeta,
                    MTK_3A_PGN_ENABLE,
                    MTRUE);

            // if shot mode is single frame, do not use MFNR related ISP profile
            mfllLogD3("%s: shotMode=%#x", __FUNCTION__, m_shotMode);
            if (m_shotMode & (1 << MfllMode_Bit_SingleFrame))
                mfllLogD("%s: do not use Mfll related ISP profile for SF",__FUNCTION__);
            else
                IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_3A_ISP_PROFILE, profile);

            // tell ISP manager that this stage is YUV->YUV
            std::unique_ptr<void, std::function<void(void*)> > _tagIspP2InFmtRestorer;
            MINT32 tagIspP2InFmt = 0;
            if (bIsYuv2Yuv) {
                tagIspP2InFmt = setIspP2ImgFmtTag(metaset.halMeta, 1, &_tagIspP2InFmtRestorer);
            }

            // debug {{{
            {
                MUINT8 __profile = 0;
                if (IMetadata::getEntry<MUINT8>(&metaset.halMeta, MTK_3A_ISP_PROFILE, __profile)) {
                    mfllLogD3("%s: isp profile=%#x", __FUNCTION__, __profile);
                }
                else {
                    mfllLogW("%s: isp profile is nullptr, ISP manager is supposed to use default one", __FUNCTION__);
                }
            }
            // }}}

            MetaSet_T rMetaSet;

            mfllTraceBegin("SetIsp");
            auto ret_setIsp = this->setIsp(0, metaset, &rTuningParam, &rMetaSet, pInput);
            if (CC_LIKELY( m_tuningBuf.isInited )) {
                IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ispP2TuningUpdateMode);
                IMetadata::setEntry<MUINT8>(&rMetaSet.halMeta, MTK_ISP_P2_TUNING_UPDATE_MODE, ispP2TuningUpdateMode);
            }
            // check rTuningParam.pBpc2Buf first
            if (__builtin_expect( rTuningParam.pBpc2Buf == nullptr, false )) {
                mfllLogW("%s: rTuningParam.pBpc2Buf is null", __FUNCTION__);
            }

            // update BPC buffer address at the first time
            if (m_pBPCI == nullptr)
                m_pBPCI = rTuningParam.pBpc2Buf;

            if (ret_setIsp == 0) {

                mfllLogD3("%s: get tuning data, reg=%p, lsc=%p",
                        __FUNCTION__, rTuningParam.pRegBuf, rTuningParam.pLsc2Buf);

                // restore MTK_ISP_P2_IN_IMG_FMT first
                IMetadata::setEntry<MINT32>(&rMetaSet.halMeta, MTK_ISP_P2_IN_IMG_FMT, tagIspP2InFmt);

                // refine register
                pltcfg[ePLATFORMCFG_DIP_X_REG_T] =
                    reinterpret_cast<intptr_t>((volatile void*)rTuningParam.pRegBuf);

                mfllTraceBegin("RefineReg");
                if (!refine_register(pltcfg)) {
                    mfllLogE("%s: refine_register returns fail", __FUNCTION__);
                }
                else {
#ifdef __DEBUG
                    mfllLogD("%s: refine_register ok", __FUNCTION__);
#endif
                }
                mfllTraceEnd(); // "RefineReg"

                // update Exif info
                switch (s) {
                case YuvStage_BaseYuy2: // stage bfbld
                    if (m_bExifDumpped[STAGE_BASE_YUV] == 0) {
                        dump_exif_info(m_pCore, tuning_reg.get(), STAGE_BASE_YUV);
                        m_bExifDumpped[STAGE_BASE_YUV] = 1;
                    }
                    break;

                case YuvStage_RawToYuy2: // stage bfbld
                case YuvStage_RawToYv16: // stage bfbld
                    if (m_bExifDumpped[STAGE_BASE_YUV] == 0) {
                        dump_exif_info(m_pCore, tuning_reg.get(), STAGE_BASE_YUV);
                        m_bExifDumpped[STAGE_BASE_YUV] = 1;
                        /* update metadata within the one given by IHalISP only in stage bfbld only once */
                        *pHalMeta += rMetaSet.halMeta;
                    }
                    break;

                case YuvStage_GoldenYuy2: // stage sf
                    if (m_bExifDumpped[STAGE_GOLDEN_YUV] == 0) {
                        dump_exif_info(m_pCore, tuning_reg.get(), STAGE_GOLDEN_YUV);
                        m_bExifDumpped[STAGE_GOLDEN_YUV] = 1;
                    }
                    // if single frame, update metadata
                    if (m_shotMode & (1 << MfllMode_Bit_SingleFrame))
                        *pHalMeta += rMetaSet.halMeta;

                    break;

                default:
                    // do nothing
                    break;
                }

                params.mTuningData = static_cast<MVOID*>(tuning_reg.get()); // adding tuning data

                /* LSC tuning data is constructed as an IImageBuffer, and we don't need to release */
                IImageBuffer* pSrc = static_cast<IImageBuffer*>(rTuningParam.pLsc2Buf);
                if ((bIsYuv2Yuv == false) && (pSrc != NULL)) {
                    Input __src;
                    __src.mPortID         = PORT_IMGCI;
                    __src.mPortID.group   = 0;
                    __src.mBuffer         = pSrc;
                    params.mvIn.push_back(__src);
                }
                else {
                    // check if it's process RAW, if yes, there's no LSC.
                    MINT32 __rawType = NSIspTuning::ERawType_Pure;
                    if (!IMetadata::getEntry<MINT32>(
                                &metaset.halMeta,
                                MTK_P1NODE_RAW_TYPE,
                                __rawType)) {
                        mfllLogW("get p1 node RAW type fail, assume it's pure RAW");
                    }

                    // check if LSC table should exists or not.
                    if (__rawType == NSIspTuning::ERawType_Proc) {
                        mfllLogD3("process RAW, no LSC table");
                    }
                    else if (bIsYuv2Yuv) {
                        mfllLogD3("Yuv->Yuv, no LSC table");
                    }
                    else {
                        mfllLogE("create LSC image buffer fail");
                        err = MfllErr_UnexpectedError;
                    }
                }

                IImageBuffer* pBpcBuf = static_cast<IImageBuffer*>(rTuningParam.pBpc2Buf);
                if ((bIsYuv2Yuv == false) && (pBpcBuf != nullptr)) {
                    Input __src;
                    __src.mPortID         = PORT_IMGBI;
                    __src.mPortID.group   = 0;
                    __src.mBuffer         = pBpcBuf;
                    params.mvIn.push_back(__src);
                }
                else {
                    // BPC buffer may be NULL even for pure RAW processing
                    mfllLogI("No need set Bpc buffer if No BPC buffer(%p) or It's yuv to yuv stage(%d)",
                            pBpcBuf, bIsYuv2Yuv);
                }

                // LCSO buffer may be removed by ISP manager
                // only needed while RAW->YUV
                IImageBuffer* pLcsoBuf = static_cast<IImageBuffer*>(rTuningParam.pLcsBuf);
                if ((bIsYuv2Yuv == false) && (pLcsoBuf != nullptr)) {
                    Input __src;
                    __src.mPortID         = RAW2YUV_PORT_LCE_IN;
                    __src.mPortID.group   = 0;
                    __src.mBuffer         = pLcsoBuf;
                    params.mvIn.push_back(__src);

                    /* one more port needs LCSO buffer since ISP5.0 */
                    __src.mPortID  = PORT_DEPI;
                    params.mvIn.push_back(__src);

                    /* this SRZ4 info is for PORT_DEPI */
                    ModuleInfo srz4_module;
                    srz4_module.moduleTag       = EDipModule_SRZ4;
                    srz4_module.frameGroup      = 0;
                    srzParam->in_w              = pLcsoBuf->getImgSize().w; // resolution of LCSO
                    srzParam->in_h              = pLcsoBuf->getImgSize().h; // ^^^^^^^^^^^^^^^^^^
                    srzParam->crop_floatX       = 0;
                    srzParam->crop_floatY       = 0;
                    srzParam->crop_x            = 0;
                    srzParam->crop_y            = 0;
                    srzParam->crop_w            = pLcsoBuf->getImgSize().w; // resolution of LCSO
                    srzParam->crop_h            = pLcsoBuf->getImgSize().h; // ^^^^^^^^^^^^^^^^^^
                    srzParam->out_w             = pInput->getImgSize().w; // input raw width
                    srzParam->out_h             = pInput->getImgSize().h; // input raw height
                    srz4_module.moduleStruct    = static_cast<MVOID*>(srzParam.get());
                    //
                    params.mvModuleData.push_back(srz4_module);

                    /* no cropping info need */
                    mfllLogD3("enable LCEI port for LCSO buffer, size = %d x %d",
                            pImgLcsoRaw->getImgSize().w,
                            pImgLcsoRaw->getImgSize().h);
                }

            }
            else {
                mfllLogE("%s: set ISP profile failed...", __FUNCTION__);
                err = MfllErr_UnexpectedError;
                goto lbExit;
            }
            mfllTraceEnd(); // "SetIsp"

            // restore MTK_ISP_P2_IN_IMG_FMT
            _tagIspP2InFmtRestorer = nullptr;
        }

        DpPqParam   mdpPqDump = generateMdpDumpPQParam(&metaset.halMeta);

        mfllTraceBegin("BeforeP2Enque");
        /* input: source frame [IMGI port] */
        {
            Input p;
            p.mBuffer          = pInput;
            p.mPortID          = portMap.inputPorts[0].portId;
            p.mPortID.group    = 0; // always be 0
            params.mvIn.push_back(p);
            mfllLogI("input: va/pa(%p/%p) fmt(%#x), size=(%d,%d), srcCrop(x,y,w,h)=(%d,%d,%d,%d), YuvStage=(%d)",
                    // va/pa
                    (void*)pInput->getBufVA(0), (void*)pInput->getBufPA(0),
                    // fmt
                    pInput->getImgFormat(),
                    // size
                    pInput->getImgSize().w, pInput->getImgSize().h,
                    // srcCrop
                    0, 0,
                    pInput->getImgSize().w, pInput->getImgSize().h,
                    // Yuv Stage
                    s);
        }

        /* output: destination frame [WDMAO port] */
        {
            Output p;
            p.mBuffer          = pOutput;
            p.mPortID          = portMap.outputPorts[0].portId;
            p.mPortID.group    = 0; // always be 0
            params.mvOut.push_back(p);

            /**
             *  Check resolution between input and output, if the ratio is different,
             *  a cropping window should be applied.
             */
            MRect srcCrop =
                (
                 output_crop.size() <= 0
                 // original size of source
                 ? MRect(MPoint(0, 0), pInput->getImgSize())
                 // user specified cropping window
                 : MRect(MPoint(output_crop.x, output_crop.y),
                     MSize(output_crop.w, output_crop.h))
                );

            srcCrop = calCrop(
                    srcCrop,
                    MRect(MPoint(0,0), pOutput->getImgSize()),
                    100);

            /**
             *  Cropping info, only works with input port is IMGI port.
             *  mGroupID should be the index of the MCrpRsInfo.
             */
            MCrpRsInfo crop;
            crop.mGroupID       = portMap.outputPorts[0].groupId;
            crop.mCropRect.p_integral.x = srcCrop.p.x; // position pixel, in integer
            crop.mCropRect.p_integral.y = srcCrop.p.y;
            crop.mCropRect.p_fractional.x = 0; // always be 0
            crop.mCropRect.p_fractional.y = 0;
            crop.mCropRect.s.w  = srcCrop.s.w;
            crop.mCropRect.s.h  = srcCrop.s.h;
            crop.mResizeDst.w   = pOutput->getImgSize().w;
            crop.mResizeDst.h   = pOutput->getImgSize().h;
            crop.mFrameGroup    = 0;
            params.mvCropRsInfo.push_back(crop);
            mfllLogI("output: va/pa(%p/%p) fmt(%#x), size=(%d,%d), srcCrop(x,y,w,h)=(%d,%d,%d,%d), dstSize(w,h)=(%d,%d), YuvStage=(%d)",
                    // va/pa
                    (void*)pOutput->getBufVA(0), (void*)pOutput->getBufPA(0),
                    // fmt
                    pOutput->getImgFormat(),
                    // size
                    pOutput->getImgSize().w, pOutput->getImgSize().h,
                    // srcCrop
                    crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
                    crop.mCropRect.s.w, crop.mCropRect.s.h,
                    // dstSize
                    crop.mResizeDst.w, crop.mResizeDst.h,
                    // yuv stage
                    s);
        }

        /* output2: destination frame [WROTO prot] */
        if (pOutputQ) {
            Output p;
            p.mBuffer          = pOutputQ;
            p.mPortID          = portMap.outputPorts[1].portId;
            p.mPortID.group    = 0; // always be 0
            params.mvOut.push_back(p);

            /**
             *  Check resolution between input and output, if the ratio is different,
             *  a cropping window should be applied.
             */

            MRect srcCrop =
                (
                 output_q_crop.size() <= 0
                 // original size of source
                 ? MRect(MPoint(0, 0), pInput->getImgSize())
                 // user specified crop
                 : MRect(MPoint(output_q_crop.x, output_q_crop.y),
                     MSize(output_q_crop.w, output_q_crop.h))
                );
            srcCrop = calCrop(
                    srcCrop,
                    MRect(MPoint(0,0), pOutputQ->getImgSize()),
                    100);

            /**
             *  Cropping info, only works with input port is IMGI port.
             *  mGroupID should be the index of the MCrpRsInfo.
             */
            MCrpRsInfo crop;
            crop.mGroupID       = portMap.outputPorts[1].groupId;
            crop.mCropRect.p_integral.x = srcCrop.p.x; // position pixel, in integer
            crop.mCropRect.p_integral.y = srcCrop.p.y;
            crop.mCropRect.p_fractional.x = 0; // always be 0
            crop.mCropRect.p_fractional.y = 0;
            crop.mCropRect.s.w  = srcCrop.s.w;
            crop.mCropRect.s.h  = srcCrop.s.h;
            crop.mResizeDst.w   = pOutputQ->getImgSize().w;
            crop.mResizeDst.h   = pOutputQ->getImgSize().h;
            crop.mFrameGroup    = 0;
            params.mvCropRsInfo.push_back(crop);

            mfllLogI("output2: va/pa(%p/%p) fmt(%#x), size=(%d,%d), srcCrop(x,y,w,h)=(%d,%d,%d,%d), dstSize(w,h)=(%d,%d), YuvStage=(%d)",
                    // va/pa
                    (void*)pOutputQ->getBufVA(0), (void*)pOutputQ->getBufPA(0),
                    // fmt
                    pOutputQ->getImgFormat(),
                    // size
                    pOutputQ->getImgSize().w, pOutputQ->getImgSize().h,
                    // srcCrop
                    crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
                    crop.mCropRect.s.w, crop.mCropRect.s.h,
                    // dstSize
                    crop.mResizeDst.w, crop.mResizeDst.h,
                    // Yuv stage
                    s);

            /* If pOutputQ is resized Y8, dump MDP info */
            if (pOutputQ->getImgFormat() == eImgFmt_Y8) {
                attachMDPPQParam2Pass2drv(p.mPortID, &params, &p2PqParam, &mdpPqDump);
            }
        }

        /* output3: destination frame*/
        if (pOutput3) {
            Output p;
            p.mBuffer          = pOutput3;
            p.mPortID          = portMap.outputPorts[2].portId;
            p.mPortID.group    = 0; // always be 0
            params.mvOut.push_back(p);

            /**
             *  Check resolution between input and output, if the ratio is different,
             *  a cropping window should be applied.
             */

            MRect srcCrop;
            if ((output_3_crop.size() <= 0) ||
                (pInput->getImgSize().h < output_3_crop.h) ||
                (pInput->getImgSize().w < output_3_crop.w)) {
                // original size of source
                srcCrop = MRect(MPoint(0, 0), pInput->getImgSize());
                mfllLogW("output3: invalid!! crop(w,h)=(%d,%d), pInput:(w,h)=(%d,%d)",
                          output_3_crop.w, output_3_crop.h, pInput->getImgSize().w, pInput->getImgSize().h);
            }
            else {
                // user specified crop
                srcCrop = MRect(MPoint(output_3_crop.x, output_3_crop.y),
                                MSize(output_3_crop.w, output_3_crop.h));
            }
            srcCrop = calCrop(
                    srcCrop,
                    MRect(MPoint(0,0), pOutput3->getImgSize()),
                    100);

            /**
             *  Cropping info, only works with input port is IMGI port.
             *  mGroupID should be the index of the MCrpRsInfo.
             */
            MCrpRsInfo crop;
            crop.mGroupID       = portMap.outputPorts[2].groupId;
            crop.mCropRect.p_integral.x = srcCrop.p.x; // position pixel, in integer
            crop.mCropRect.p_integral.y = srcCrop.p.y;
            crop.mCropRect.p_fractional.x = 0; // always be 0
            crop.mCropRect.p_fractional.y = 0;
            crop.mCropRect.s.w  = srcCrop.s.w;
            crop.mCropRect.s.h  = srcCrop.s.h;
            crop.mResizeDst.w   = pOutput3->getImgSize().w;
            crop.mResizeDst.h   = pOutput3->getImgSize().h;
            crop.mFrameGroup    = 0;
            params.mvCropRsInfo.push_back(crop);

            mfllLogI("output3: va/pa(%p/%p) fmt(%#x), size=(%d,%d), srcCrop(x,y,w,h)=(%d,%d,%d,%d), dstSize(w,h)=(%d,%d), YuvStage=(%d)",
                    // va/pa
                    (void*)pOutput3->getBufVA(0), (void*)pOutput3->getBufPA(0),
                    // fmt
                    pOutput3->getImgFormat(),
                    // size
                    pOutput3->getImgSize().w, pOutput3->getImgSize().h,
                    // srcCrop
                    crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
                    crop.mCropRect.s.w, crop.mCropRect.s.h,
                    // dstSize
                    crop.mResizeDst.w, crop.mResizeDst.h,
                    // Yuv Stage
                    s);
        }

        /* dceso: dceso buffer*/
        if (pDceso) {
            Output p;
            p.mBuffer          = pDceso;
            p.mPortID          = PORT_DCESO;
            p.mPortID.group    = 0; // always be 0
            params.mvOut.push_back(p);

            mfllLogI("dceso: va/pa(%p/%p) fmt(%#x), size=(%d,%d), YuvStage=(%d)",
                    // va/pa
                    (void*)pDceso->getBufVA(0), (void*)pDceso->getBufPA(0),
                    // fmt
                    pDceso->getImgFormat(),
                    // size
                    pDceso->getImgSize().w, pDceso->getImgSize().h,
                    s);
        }

        mfllTraceEnd(); // "BeforeP2Enque"


        Mutex::Autolock _l(gMutexPass2Lock);
        /**
         *  Finally, we're going to invoke P2 driver to encode Raw. Notice that,
         *  we must use async function call to trigger P2 driver, which means
         *  that we have to give a callback function to P2 driver.
         *
         *  E.g.:
         *      QParams::mpfnCallback = CALLBACK_FUNCTION
         *      QParams::mpCookie --> argument that CALLBACK_FUNCTION will get
         *
         *  Due to we wanna make the P2 driver as a synchronized flow, here we
         *  simply using Mutex to sync async call of P2 driver.
         */
        using MfllMfb_Imp::P2Cookie;
        P2Cookie cookie;
        //
        qParams.mpfnCallback        = __P2Cb<QParams>;
        qParams.mpfnEnQFailCallback = __P2FailCb<QParams>;
        qParams.mpCookie        = reinterpret_cast<void*>(&cookie);
        qParams.mvFrameParams   .push_back(params);
        //
        mfllLogD3("%s: P2 enque", __FUNCTION__);
        mfllLogI("%s: mpCookie=%p", __FUNCTION__, qParams.mpCookie);
        mfllTraceBegin("P2");
#if 1
        usleep(50*1000);
#else
        if (CC_UNLIKELY( ! m_pNormalStream->enque(qParams) )) {
            mfllLogE("%s: pass 2 enque fail", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            mfllTraceEnd();
            goto lbExit;
        }

        // to synchronize qParams
        std::atomic_thread_fence(std::memory_order_release);

        /* wait unitl P2 deque */
        mfllLogI("%s: wait P2(BFBLD/SF) deque [+]", __FUNCTION__);
        {
            std::unique_lock<std::mutex> lk(cookie.__mtx);
            if (0 == cookie.__signaled.load(std::memory_order_seq_cst)) {
                /* wait for a second */
                auto status = cookie.__cv.wait_for(lk, std::chrono::milliseconds(MFLL_MFB_ENQUE_TIMEOUT_MS));
                if (CC_UNLIKELY( status == std::cv_status::timeout && 0 == cookie.__signaled.load(std::memory_order_seq_cst) )) {

                    mfllLogE("%s: p2 deque timed out (%d ms), index=%d, yuv stage=%d, manually dump QParams:", __FUNCTION__,
                            MFLL_MFB_ENQUE_TIMEOUT_MS, index, s);

                    __P2FailCb<QParams>(qParams);


                    std::string _log = std::string("FATAL: wait timeout of dequing from BFBLD or SF: ") + \
                                       std::to_string(MFLL_MFB_ENQUE_TIMEOUT_MS) + std::string("ms");
                    AEE_ASSERT_MFB(_log.c_str());
                }
            }
        }
#endif
        mfllLogI("%s: wait P2(BFBLD/SF) deque [-]", __FUNCTION__);
        mfllTraceEnd();
    }

lbExit:
    return err;
}

enum MfllErr MfllMfb::mss(
        vector<IMfllImageBuffer *>& pylamid,
        IMfllImageBuffer*       omc,
        IMfllImageBuffer*       mcmv,
        IMfllImageBuffer*       dceso,
        MINT32                  index,
        enum YuvStage           s /* = YuvStage_Mss */)
{
    mfllAutoLogFunc();

    vector<IImageBuffer *> vecPylamid;

    /* It MUST NOT be NULL, so don't do error handling */
    if (pylamid.empty()) {
        mfllLogE("%s: pylamid is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        vecPylamid.resize(pylamid.size());
        for (size_t i = 0 ; i < pylamid.size() ; i++) {
            if (pylamid[i] == NULL) {
                mfllLogE("%s: pylamid[%zu] is NULL", __FUNCTION__, i);
                return MfllErr_NullPointer;
            }
            vecPylamid[i] = (IImageBuffer*)pylamid[i]->getImageBuffer();
        }
    }

    if (mcmv == NULL && omc != NULL) {
        mfllLogE("%s: motion compensation mv is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (mcmv != NULL && omc == NULL) {
        mfllLogE("%s: omc frame is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }
#if MFNR_EARLY_PORTING
    if (omc != nullptr) {
        mfllLogD("MSSConfig.mss_scenario = MSS_MFNR_REFE_MODE");
        mfllLogD("MSSConfig.scale_Total = %d",  pylamid.size());
        mfllLogD("MSSConfig.omc_tuningBuf = MSF_0 tuning");
        mfllLogD("MSSConfig.mss_mvMap = doMss_JR_mcmv");
        mfllLogD("MSSConfig.mss_omcFrame = doMss_JR_omc");
    } else {
        mfllLogD("MSSConfig.mss_scenario = MSS_MFNR_BASE_MODE");
        mfllLogD("MSSConfig.scale_Total = %d",  pylamid.size());
        mfllLogD("MSSConfig.omc_tuningBuf = nullptr");
        mfllLogD("MSSConfig.mss_mvMap = nullptr");
        mfllLogD("MSSConfig.mss_omcFrame = nullptr");
    }
    mfllLogD("MSSConfig.mss_scaleFrame = doMss_JR_S");

    //To-do: remove it
    for (size_t i = 0 ; i < pylamid.size() ; i++) {
        char name[20];
        memset(name, 0 , 20);
        sprintf(name, "doMss_JR_S%d", i);
        mfllLogD("mss dump = %s, debug = %p", name, pylamid[i]);
        pylamid[i]->dumpDebugLog(name);
    }
    if (omc) {
        char name[20];
        memset(name, 0 , 20);
        sprintf(name, "doMss_JR_omc");
        mfllLogD("mss dump = %s, debug = %p", name, omc);
        omc->dumpDebugLog(name);
    }
    if (mcmv) {
        char name[20];
        memset(name, 0 , 20);
        sprintf(name, "doMss_JR_mcmv");
        mfllLogD("mss dump = %s, debug = %p", name, mcmv);
        mcmv->dumpDebugLog(name);
    }
#endif
    return mss(
            vecPylamid,
            omc  ? (IImageBuffer*)omc->getImageBuffer()  : NULL,
            mcmv ? (IImageBuffer*)mcmv->getImageBuffer() : NULL,
            dceso? (IImageBuffer*)dceso->getImageBuffer() : NULL,
            index,
            s);
}

enum MfllErr MfllMfb::mss(
        vector<IImageBuffer *>& pylamid,
        IImageBuffer*           omc,
        IImageBuffer*           mcmv,
        IImageBuffer*           dceso,
        int                     index,
        enum YuvStage           s /* = YuvStage_Mss */)
{
    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    enum MfllErr err = MfllErr_Ok;

    // 1. check tuning buffer is ok
    {
        if (index > 0)
            err = initMfbTuningBuf(pylamid, dceso, index-1);
        if (err != MfllErr_Ok)
            return err;
    }

    // 2. generate mss param
    {
        m_vMssConfig = EGNParams<MSSConfig>();
        err = generateMssParam(pylamid, omc, mcmv, index, m_vMssConfig);
        if (err != MfllErr_Ok)
            return err;
    }

    // 3. enque to driver
    {
        m_vMssConfig.mpCookie = reinterpret_cast<void*>(&m_mssCookie[index]);
        m_vMssConfig.mpfnCallback = __P2Cb< EGNParams<MSSConfig> >;

        mfllLogD3("%s: MSS EGNenque", __FUNCTION__);
        mfllLogI("%s: mpCookie=%p", __FUNCTION__, m_vMssConfig.mpCookie);

        mfllTraceBegin("MSS");

#if (FAKE_DRIVER_MSS == 1)
        mfllLogD("To-Do: link to mss driver");
        auto fu = std::async(std::launch::async, [this, omc]() mutable {
            if (omc != nullptr)
                usleep(35*1000);
            usleep(15*1000);
            return __P2Cb(m_vMssConfig);
        });
#else
        if (CC_UNLIKELY( ! m_pMssStream->EGNenque(m_vMssConfig) )) {
            mfllLogE("%s: mss enque returns fail", __FUNCTION__);
            err = MfllErr_UnexpectedError;
        }
#endif
        if (err == MfllErr_Ok) {
            //init next frames' tuning data and don't use other thread
            if (index < m_pCore->getFrameCapturedNum()-1) {
                initMfbTuningBuf(pylamid, dceso, index);
            }

            // to synchronize qParams
            std::atomic_thread_fence(std::memory_order_release);

            /* wait unitl P2 deque */
            mfllLogI("%s: wait MSS deque [+]", __FUNCTION__);
            {
                std::unique_lock<std::mutex> lk(m_mssCookie[index].__mtx);
                if (0 == m_mssCookie[index].__signaled.load(std::memory_order_seq_cst)) {
                    /* wait for a second */
                    auto status = m_mssCookie[index].__cv.wait_for(lk, std::chrono::milliseconds(MFLL_MSS_ENQUE_TIMEOUT_MS));
                    if (CC_UNLIKELY( status == std::cv_status::timeout && 0 == m_mssCookie[index].__signaled.load(std::memory_order_seq_cst) )) {
                        std::string _log = std::string("FATAL: wait timeout of dequing from MSS: ") + \
                                           std::to_string(MFLL_MSS_ENQUE_TIMEOUT_MS) + std::string("ms");
                        AEE_ASSERT_MFB(_log.c_str());
                    }
                    else {
                        mfllLogD("%s: MSS status:%d, signal:%d", __FUNCTION__, status, m_mssCookie[index].__signaled.load(std::memory_order_seq_cst));
                    }
                }
                else {
                    mfllLogD("%s: MSS signal:%d", __FUNCTION__, m_mssCookie[index].__signaled.load(std::memory_order_seq_cst));
                }
            }
            mfllLogI("%s: wait MSS deque [-]", __FUNCTION__);
        }

        mfllTraceEnd();
    }

lbExit:
    if (err != MfllErr_Ok)
        mfllLogE("%s: error code(%d)", __FUNCTION__, (int)err);

    return err;
}


enum MfllErr MfllMfb::msf(
        vector<IMfllImageBuffer *>& base,
        vector<IMfllImageBuffer *>& ref,
        vector<IMfllImageBuffer *>& out,
        vector<IMfllImageBuffer *>& wt_in,
        vector<IMfllImageBuffer *>& wt_out,
        vector<IMfllImageBuffer *>& wt_ds,
        IMfllImageBuffer*           difference,
        IMfllImageBuffer*           conf,
        IMfllImageBuffer*           dsFrame,
        IMfllImageBuffer*           dceso,
        int                         index,
        enum YuvStage               s /* = YuvStage_Msf */)
{
    mfllAutoLogFunc();

    enum MfllErr err = MfllErr_Ok;

    vector<IImageBuffer *> vecBase;
    vector<IImageBuffer *> vecRef;
    vector<IImageBuffer *> vecOut;
    vector<IImageBuffer *> vecWt_in;
    vector<IImageBuffer *> vecWt_out;
    vector<IImageBuffer *> vecWt_ds;

    /* It MUST NOT be NULL, so don't do error handling */
    auto bufferTrans = [this](vector<IMfllImageBuffer *>& input, vector<IImageBuffer *>& output, const char* name) -> enum MfllErr {
        output.resize(input.size());
        for (size_t i = 0 ; i < input.size() ; i++) {
            if (input[i] == NULL) {
                mfllLogE("%s: %s[%zu] is NULL", __FUNCTION__, name, i);
                return MfllErr_NullPointer;
            }
            output[i] = (IImageBuffer*)input[i]->getImageBuffer();
        }
        return MfllErr_Ok;
    };
    //
    if (base.empty()) {
        mfllLogE("%s: base is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        err = bufferTrans(base, vecBase, "base");
        if (err != MfllErr_Ok)
            return err;
    }
    //
    if (ref.empty()) {
        mfllLogE("%s: ref is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        err = bufferTrans(ref, vecRef, "ref");
        if (err != MfllErr_Ok)
            return err;
    }
    //
    if (out.empty()) {
        mfllLogE("%s: out is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        auto err = bufferTrans(out, vecOut, "out");
        if (err != MfllErr_Ok)
            return err;
    }
    //
    if (wt_in.empty()) {
        mfllLogE("%s: wt_in is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        auto err = bufferTrans(wt_in, vecWt_in, "wt_in");
        if (err != MfllErr_Ok)
            return err;
    }
    //
    if (wt_out.empty()) {
        mfllLogE("%s: wt_out is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        err = bufferTrans(wt_out, vecWt_out, "wt_out");
        if (err != MfllErr_Ok)
            return err;

    }
    //
    if (wt_ds.empty()) {
        mfllLogE("%s: wt_ds is empty", __FUNCTION__);
        return MfllErr_BadArgument;
    } else {
        err = bufferTrans(wt_ds, vecWt_ds, "wt_ds");
        if (err != MfllErr_Ok)
            return err;
    }

    if (difference == NULL) {
        mfllLogE("%s: image difference is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (conf == NULL) {
        mfllLogE("%s: confidence map is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }

    if (dsFrame == NULL) {
        mfllLogE("%s: dsFrame is NULL", __FUNCTION__);
        return MfllErr_NullPointer;
    }
#if MFNR_EARLY_PORTING
    mfllLogD("MSFConfig.msf_scenario = MSF_MFNR_MODE_CAP");
    mfllLogD("MSFConfig.frame_Idx = %u", m_blendCount+1);
    mfllLogD("MSFConfig.frame_Total = %d", m_pCore->getFrameCapturedNum());
    mfllLogD("MSFConfig.scale_Idx = unknow");
    mfllLogD("MSFConfig.scale_Total = %d",  base.size());
    mfllLogD("MSFConfig.msf_tuningBuf, vector size = %d", base.size()-1);
    mfllLogD("MSFConfig.msf_sramTable, vector size = %d", base.size()-1);
    mfllLogD("MSFConfig.msf_imageDifference = doMsf_JR_difference");
    mfllLogD("MSFConfig.msf_confidenceMap = doMsf_JR_conf");
    mfllLogD("MSFConfig.msf_dsFrame = doMsf_JR_dsFrame");
    mfllLogD("MSFConfig.msf_baseFrame = doMsf_JR_base_S");
    mfllLogD("MSFConfig.msf_refFrame = doMsf_JR_ref_S");
    mfllLogD("MSFConfig.msf_outFrame = doMsf_JR_out_S");
    mfllLogD("MSFConfig.msf_weightingMap_in = doMsf_JR_wt_in_S");
    mfllLogD("MSFConfig.msf_weightingMap_out = doMsf_JR_wt_out_S");
    mfllLogD("MSFConfig.msf_weightingMap_ds = doMsf_JR_wt_ds_S");

    for (size_t i = 0 ; i < base.size() ; i++) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_base_S%d", i);
        mfllLogD("msf dump = %s, debug = %p", name, base[i]);
        base[i]->dumpDebugLog(name);
    }
    for (size_t i = 0 ; i < ref.size() ; i++) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_ref_S%d", i);
        mfllLogD("msf dump = %s, debug = %p", name, ref[i]);
        ref[i]->dumpDebugLog(name);
    }
    for (size_t i = 0 ; i < out.size() ; i++) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_out_S%d", i);
        mfllLogD("msf dump = %s, debug = %p", name, out[i]);
        out[i]->dumpDebugLog(name);
    }
    for (size_t i = 0 ; i < wt_in.size() ; i++) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_wt_in_S%d", i);
        mfllLogD("msf dump = %s, debug = %p", name, wt_in[i]);
        if (wt_in[i] != nullptr)
            wt_in[i]->dumpDebugLog(name);
    }
    for (size_t i = 0 ; i < wt_out.size() ; i++) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_wt_out_S%d", i);
        mfllLogD("msf dump = %s, debug = %p", name, wt_out[i]);
        wt_out[i]->dumpDebugLog(name);
    }
    for (size_t i = 0 ; i < wt_ds.size() ; i++) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_wt_ds_S%d", i);
        mfllLogD("msf dump = %s, debug = %p", name, wt_ds[i]);
        wt_ds[i]->dumpDebugLog(name);
    }
    if (difference) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_difference");
        mfllLogD("msf dump = %s, debug = %p", name, difference);
        difference->dumpDebugLog(name);
    }
    if (conf) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_conf");
        mfllLogD("msf dump = %s, debug = %p", name, conf);
        conf->dumpDebugLog(name);
    }
    if (dsFrame) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_dsFrame");
        mfllLogD("msf dump = %s, debug = %p", name, dsFrame);
        dsFrame->dumpDebugLog(name);
    }

    if (dceso) {
        char name[100];
        memset(name, 0 , 100);
        sprintf(name, "doMsf_JR_dceso");
        mfllLogD("dceso dump = %s, debug = %p", name, dceso);
        dceso->dumpDebugLog(name);
    }
#endif

    if (m_blendCount+2 >= m_pCore->getFrameCapturedNum()) { // index + 2 = blended + 1 = capture num, but this frame is not blended, so it equals to index
        if (dceso == NULL) {
            mfllLogE("%s: dceso is NULL", __FUNCTION__);
            return MfllErr_NullPointer;
        }
        mfllLogD3("%s: MFB direct link at blend(%u), total_frames(%d)", __FUNCTION__, m_blendCount+1, m_pCore->getFrameCapturedNum());
        return msf_p2(
                vecBase,
                vecRef,
                vecOut,
                vecWt_in,
                vecWt_out,
                vecWt_ds,
                (IImageBuffer*)difference->getImageBuffer(),
                (IImageBuffer*)conf->getImageBuffer(),
                (IImageBuffer*)dsFrame->getImageBuffer(),
                (IImageBuffer*)dceso->getImageBuffer(),
                index,
                s);
    }

    return msf(
            vecBase,
            vecRef,
            vecOut,
            vecWt_in,
            vecWt_out,
            vecWt_ds,
            (IImageBuffer*)difference->getImageBuffer(),
            (IImageBuffer*)conf->getImageBuffer(),
            (IImageBuffer*)dsFrame->getImageBuffer(),
            (IImageBuffer*)dceso->getImageBuffer(),
            index,
            s);

}

enum MfllErr MfllMfb::msf(
        vector<IImageBuffer *>& base,
        vector<IImageBuffer *>& ref,
        vector<IImageBuffer *>& out,
        vector<IImageBuffer *>& wt_in,
        vector<IImageBuffer *>& wt_out,
        vector<IImageBuffer *>& wt_ds,
        IImageBuffer*           difference,
        IImageBuffer*           confmap,
        IImageBuffer*           dsFrame,
        IImageBuffer*           dceso,
        int                     index,
        enum YuvStage           s /* = YuvStage_Msf */)
{
    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    enum MfllErr err = MfllErr_Ok;

    // 1. check tuning buffer is ok
    {
        err = initMfbTuningBuf(base, dceso, index);
        if (err != MfllErr_Ok)
            return err;
    }

    // 2. calc idi
    {
        mfllTraceBegin("IDI");
        mfllLogD3("%s: enable IDI calc for msf(%u)", __FUNCTION__, m_blendCount);
        //ref
        unsigned int meanLuma = 0;
        calc_msf_idi_differnce_map(
            static_cast<unsigned char*>( (void *)base.back()->getBufVA(0) ),
            static_cast<unsigned char*>( (void *)base.back()->getBufVA(1) ),
            static_cast<unsigned char*>( (void *)ref.back()->getBufVA(0) ),
            static_cast<unsigned char*>( (void *)ref.back()->getBufVA(1) ),
            static_cast<unsigned char*>( (void *)difference->getBufVA(0) ),
            static_cast<unsigned char*>( (void *)difference->getBufVA(1) ),
            base.back()->getImgSize().w,
            base.back()->getImgSize().h,
            base.back()->getBufStridesInBytes(0),
            base.back()->getPlaneBitsPerPixel(0),
            difference->getBufStridesInBytes(0),
            difference->getPlaneBitsPerPixel(0),
            meanLuma
        );
        mfllLogD3("%s: difference syncCache for msf(%u)", __FUNCTION__, m_blendCount);
        difference->syncCache();
        // update msf tuning by mean_luma
        err = updateMSFTuningBuf(base, meanLuma);
        if (err != MfllErr_Ok )
            return err;
        mfllTraceEnd();
    }

    // 3. generate msf param
    {
        //reset
        m_vMsfConfig = EGNParams<MSFConfig>();
        err = generateMsfParam(base, ref, out, wt_in, wt_out, wt_ds, difference, confmap, dsFrame, m_vMsfConfig);
        if (err != MfllErr_Ok )
            return err;
    }

    // 4. enque driver
    {
        m_vMsfConfig.mpCookie = reinterpret_cast<void*>(&m_msfCookie[index]);
        m_vMsfConfig.mpfnCallback = __P2Cb< EGNParams<MSFConfig> >;

        mfllLogD3("%s: MSF EGNenque", __FUNCTION__);
        mfllLogI("%s: mpCookie=%p", __FUNCTION__, m_vMsfConfig.mpCookie);

        mfllTraceBegin("MSF");

#if (FAKE_DRIVER_MSF == 1)
        mfllLogD("To-Do: link to msf driver");
        auto fu = std::async(std::launch::async, [this]() mutable {
            usleep(45*1000);
            return __P2Cb(m_vMsfConfig);
        });
#else
        if (CC_UNLIKELY( ! m_pMsfStream->EGNenque(m_vMsfConfig) )) {
            mfllLogE("%s: msf enque returns fail", __FUNCTION__);
            err = MfllErr_UnexpectedError;
        }
#endif
        if (err == MfllErr_Ok) {
            // to synchronize qParams
            std::atomic_thread_fence(std::memory_order_release);

            /* last stage of Pyramid */
            if (err == MfllErr_Ok && !base.empty() && !out.empty()) {
                mfllTraceBegin("S6");
                IImageBuffer* baseImg = base.back();
                IImageBuffer* outImg = out.back();
                if (baseImg && outImg && baseImg->getPlaneCount() == outImg->getPlaneCount()) {
                    for (int i = 0 ; err == MfllErr_Ok && i < baseImg->getPlaneCount() ; i++) {
                        if (baseImg->getBufVA(i) && outImg->getBufVA(i) && baseImg->getBufSizeInBytes(i) == outImg->getBufSizeInBytes(i))
                            std::memcpy((void*)outImg->getBufVA(i), (void*)baseImg->getBufVA(i), baseImg->getBufSizeInBytes(i));
                        else
                            err = MfllErr_BadArgument;
                    }
                    if (err == MfllErr_Ok)
                        outImg->syncCache();
                } else {
                    err = MfllErr_BadArgument;
                }
                mfllTraceEnd();
            }

            /* wait unitl P2 deque */
            mfllLogI("%s: wait MSF deque [+]", __FUNCTION__);
            {
                std::unique_lock<std::mutex> lk(m_msfCookie[index].__mtx);
                if (0 == m_msfCookie[index].__signaled.load(std::memory_order_seq_cst)) {
                    /* wait for a second */
                    auto status = m_msfCookie[index].__cv.wait_for(lk, std::chrono::milliseconds(MFLL_MSF_ENQUE_TIMEOUT_MS));
                    if (CC_UNLIKELY( status == std::cv_status::timeout && 0 == m_msfCookie[index].__signaled.load(std::memory_order_seq_cst) )) {
                        std::string _log = std::string("FATAL: wait timeout of dequing from MSF: ") + \
                                           std::to_string(MFLL_MSF_ENQUE_TIMEOUT_MS) + std::string("ms");
                        AEE_ASSERT_MFB(_log.c_str());
                    }
                    else {
                        mfllLogD("%s: MSF status:%d, signal:%d", __FUNCTION__, status, m_msfCookie[index].__signaled.load(std::memory_order_seq_cst));
                    }
                }
                else {
                    mfllLogD("%s: MSF signal:%d", __FUNCTION__, m_msfCookie[index].__signaled.load(std::memory_order_seq_cst));
                }
            }
            mfllLogI("%s: wait MSF deque [-]", __FUNCTION__);
        }

        mfllTraceEnd();
    }

    m_blendCount++;

lbExit:
    if (err != MfllErr_Ok)
        mfllLogE("%s: error code(%d)", __FUNCTION__, (int)err);

    return err;
}

enum MfllErr MfllMfb::msf_p2(
        vector<IImageBuffer *>& base,
        vector<IImageBuffer *>& ref,
        vector<IImageBuffer *>& out,
        vector<IImageBuffer *>& wt_in,
        vector<IImageBuffer *>& wt_out,
        vector<IImageBuffer *>& wt_ds,
        IImageBuffer*           difference,
        IImageBuffer*           confmap,
        IImageBuffer*           dsFrame,
        IImageBuffer*           dceso,
        int                     index,
        enum YuvStage           s /* = YuvStage_Msf */)
{
    mfllAutoLogFunc();
    mfllAutoTraceFunc();

    m_mutex.lock();
    EIspProfile_T profile = (EIspProfile_T)0; // which will be set later.
    int metaIdx = m_pCore->getIndexByNewIndex(0);
    /* get metaset */
    if (metaIdx >= static_cast<int>(m_vMetaSet.size())) {
        mfllLogE("%s: index(%d) is out of size of metaset(%zu)", __FUNCTION__, metaIdx, m_vMetaSet.size());
        m_mutex.unlock();
        return MfllErr_UnexpectedError;
    }
    MetaSet_T metaset = m_vMetaSet[metaIdx];
    m_mutex.unlock();

    enum MfllErr err = MfllErr_Ok;

    // 1. check tuning buffer is ok
    {
        err = initMfbTuningBuf(base, dceso, index);
        if (err != MfllErr_Ok)
            return err;
    }

    // 2. calc idi
    {
        mfllTraceBegin("IDI");
        mfllLogD3("%s: enable IDI calc for msf(%u)", __FUNCTION__, m_blendCount);
        //ref
        unsigned int meanLuma = 0;
        calc_msf_idi_differnce_map(
            static_cast<unsigned char*>( (void *)base.back()->getBufVA(0) ),
            static_cast<unsigned char*>( (void *)base.back()->getBufVA(1) ),
            static_cast<unsigned char*>( (void *)ref.back()->getBufVA(0) ),
            static_cast<unsigned char*>( (void *)ref.back()->getBufVA(1) ),
            static_cast<unsigned char*>( (void *)difference->getBufVA(0) ),
            static_cast<unsigned char*>( (void *)difference->getBufVA(1) ),
            base.back()->getImgSize().w,
            base.back()->getImgSize().h,
            base.back()->getBufStridesInBytes(0),
            base.back()->getPlaneBitsPerPixel(0),
            difference->getBufStridesInBytes(0),
            difference->getPlaneBitsPerPixel(0),
            meanLuma
        );
        mfllLogD3("%s: difference syncCache for msf(%u)", __FUNCTION__, m_blendCount);
        difference->syncCache();
        // update msf tuning by mean_luma
        err = updateMSFTuningBuf(base, meanLuma);
        if (err != MfllErr_Ok )
            return err;
        mfllTraceEnd();
    }

    Mutex::Autolock _l(gMutexPass2Lock);

    using MfllMfb_Imp::P2Cookie;
    P2Cookie cookie[MFLL_MAX_PYRAMID_SIZE];
    EGNParams<MSFConfig> vecMsfConfig[MFLL_MAX_PYRAMID_SIZE];

    //must keep life-cycle until deque
    QParams     qParams[MFLL_MAX_PYRAMID_SIZE];
    FrameParams params[MFLL_MAX_PYRAMID_SIZE];
    PQParam     p2PqParam[MFLL_MAX_PYRAMID_SIZE];
    DpPqParam   mdpPqDre[MFLL_MAX_PYRAMID_SIZE];

    err = generateNDDHint(m_nddHintMsf, m_blendCount);
    if (err != MfllErr_Ok)
        return err;

    // enque driver from s5 to s0, s5 = base.size()-2
    for (MINT32 stage = base.size()-2 ; stage >= 0 && err == MfllErr_Ok; stage--) {
        mfllLogD3("%s: prepare MSF-P2 of stage(%d) params", __FUNCTION__, stage);
        profile = get_isp_profile(eMfllIspProfile_Mfb, m_shotMode, stage);

        // generate msf param
        {
            //reset
            vecMsfConfig[stage] = EGNParams<MSFConfig>();
            err = generateMsfP2Param(base, ref, out, wt_in, wt_out, wt_ds, difference, confmap, dsFrame, vecMsfConfig[stage], stage);
            if (err != MfllErr_Ok )
                break;
        }

        /**
         * P A S S 2
         */
        {
            IImageBuffer* img_src = base[stage];
            IImageBuffer* img_dst = out[stage];

#if 1
            /* execute pass 2 operation */
            {
                //Tuning data
                params[stage].mTuningData = static_cast<MVOID*>(m_tuningBufMsfP2[stage].data.get()); // adding tuning data


                //NDD dump hint
                {

                    int64_t timestamp = 0; // set to 0 if not found.
#if MTK_CAM_DISPAY_FRAME_CONTROL_ON
                    IMetadata::getEntry<int64_t>(&metaset.halMeta, MTK_P1NODE_FRAME_START_TIMESTAMP, timestamp);
#else
                    NSCam::IMetadata::getEntry<int64_t>(&metaset.appMeta, MTK_SENSOR_TIMESTAMP, timestamp);
#endif
                    params[stage].FrameNo = m_nddHintMsf.FrameNo;
                    params[stage].RequestNo = m_nddHintMsf.RequestNo;
                    params[stage].Timestamp = timestamp;
                    params[stage].UniqueKey = m_nddHintMsf.UniqueKey;
                    params[stage].IspProfile = get_isp_profile(eMfllIspProfile_Mfb, m_shotMode, stage);
                    params[stage].SensorDev = m_nddHintMsf.SensorDev;
                    params[stage].NeedDump = DRV_P2_REG_NEED_DUMP;
                }

                mfllTraceBegin("BeforeP2Enque");

                // output: main yuv
                {
                    mfllLogD3("config main yuv during msf-p2");
                    Output p;
                    p.mBuffer          = img_dst;
                    p.mPortID          = (stage==0)?MIX_PORT_OUT_MAIN:PORT_IMG3O;
                    p.mPortID.group    = 0; // always be 0
                    params[stage].mvOut.push_back(p);

                    //IMG3O buffer for debug
                    if (stage == 0 && m_pMixDebugBuffer.get() && m_pMixDebugBuffer->getImageBuffer() != nullptr && MIX_PORT_OUT_MAIN != PORT_IMG3O) {
                        mfllLogD("config mix debug buffer during mixing");
                        Output p;
                        p.mBuffer          = (IImageBuffer*)(m_pMixDebugBuffer->getImageBuffer());
                        p.mPortID          = PORT_IMG3O;
                        p.mPortID.group    = 0; // always be 0
                        params[stage].mvOut.push_back(p);
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
                    crop.mCropRect.s.w  = img_src->getImgSize().w;
                    crop.mCropRect.s.h  = img_src->getImgSize().h;
                    crop.mResizeDst.w   = img_src->getImgSize().w;
                    crop.mResizeDst.h   = img_src->getImgSize().h;
                    crop.mFrameGroup    = 0;
                    params[stage].mvCropRsInfo.push_back(crop);

                    mfllLogD3("main: srcCrop (x,y,w,h)=(%d,%d,%d,%d)",
                            crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
                            crop.mCropRect.s.w, crop.mCropRect.s.h);
                    mfllLogD3("main: dstSize (w,h)=(%d,%d)",
                            crop.mResizeDst.w, crop.mResizeDst.h);

                    // s0 is msf-p2-mdp direct link
                    if (stage == 0) {
                        /* ask MDP to generate CA-LTM histogram */
                        bool bEnCALTM = !!gSupportDRE;
                        bool bIsMdpPort = (p.mPortID == PORT_WDMAO) || (p.mPortID == PORT_WROTO);
                        if (bEnCALTM && bIsMdpPort ) {
                            bool bGeneratedMdpParam = generateMdpDrePQParam(
                                    mdpPqDre[stage],
                                    m_pCore->getCurrentIso(),
                                    m_sensorId,
                                    &m_vMetaSet[0].halMeta,// use the first metadata for DRE, due to
                                                           // others use main metadata to apply DRE.
                                    profile
                                    );
                            if ( ! bGeneratedMdpParam ) {
                                mfllLogE("%s: generate DRE DRE param failed", __FUNCTION__);
                            }
                            else {
                                mfllLogD3("%s: generate DRE histogram.", __FUNCTION__);
                                attachMDPPQParam2Pass2drv(p.mPortID, &params[stage], &p2PqParam[stage], &mdpPqDre[stage]);
                            }
                        }
                        else {
                            mfllLogD3("%s: DRE disabled or port is not MDP port", __FUNCTION__);
                        }
                    }
                }
#endif
                //attach MSF config
                {
                    ExtraParam extraP;
                    extraP.CmdIdx = EPIPE_MSF_INFO_CMD;

                    extraP.moduleStruct  = static_cast<void*>(&(vecMsfConfig[stage].mEGNConfigVec.back()));
                    params[stage].mvExtraParam.push_back(extraP);
                    mfllLogD3("%s: pMsfInfo=%p\n", __FUNCTION__, extraP.moduleStruct);
                    mfllLogD("%s: attch MSF pararm for MS-P2 direct link", __FUNCTION__);
                }

                mfllTraceEnd(); // BeforeP2Enque
            }

            // enque driver
            {
                struct timeval current;
                gettimeofday(&current, NULL);
                params[stage].ExpectedEndTime = current;
                params[stage].mStreamTag = NSCam::NSIoPipe::NSPostProc::ENormalStreamTag_Vss;
                //
                qParams[stage].mpfnCallback        = __P2Cb<QParams>;
                qParams[stage].mpfnEnQFailCallback = __P2FailCb<QParams>;
                qParams[stage].mpCookie            = reinterpret_cast<void*>(&cookie[stage]);
                qParams[stage].mvFrameParams       .push_back(params[stage]);
                //
                mfllLogD3("%s: P2 EGNenque", __FUNCTION__);
                mfllLogI("%s: mpCookie=%p", __FUNCTION__, qParams[stage].mpCookie);

                mfllTraceBegin("MSF-P2-Enque");

#if (FAKE_DRIVER_MSF_P2 == 1)
                mfllLogD("To-Do: link to msf-p2 driver");
                MINT32 factor = base.size()-2-stage;
                auto fu = std::async(std::launch::async, [this, qParams, stage, factor]() mutable {
                    usleep(pow(2, factor)*715); // total about 45 ms
                    return __P2Cb(qParams[stage]);
                });
#else
                if (CC_UNLIKELY( ! m_pNormalStream->enque(qParams[stage]) )) {
                    mfllLogE("%s: msf-p2 enque returns fail(%d)", __FUNCTION__, stage);
                    err = MfllErr_UnexpectedError;
                }
#endif
                mfllTraceEnd();
            }
        }
    }

    // waiting per stage finish
    for (MINT32 stage = base.size()-2 ; stage >= 0 ; stage--) {
        mfllTraceBegin("MSF-P2-Wait");
        if (err == MfllErr_Ok) {
            // to synchronize qParams
            std::atomic_thread_fence(std::memory_order_release);

            /* wait unitl P2 deque */
            mfllLogI("%s: wait MSF-P2-S%d deque [+]", __FUNCTION__, stage);
            {
                std::unique_lock<std::mutex> lk(cookie[stage].__mtx);
                if (0 == cookie[stage].__signaled.load(std::memory_order_seq_cst)) {
                    /* wait for a second */
                    auto status = cookie[stage].__cv.wait_for(lk, std::chrono::milliseconds(MFLL_MSF_ENQUE_TIMEOUT_MS));
                    if (CC_UNLIKELY( status == std::cv_status::timeout && 0 == cookie[stage].__signaled.load(std::memory_order_seq_cst) )) {
                        std::string _log = std::string("FATAL: wait timeout of dequing from MSF-P2: ") + \
                                           std::to_string(MFLL_MSF_ENQUE_TIMEOUT_MS) + std::string("ms");
                        AEE_ASSERT_MFB(_log.c_str());
                    }
                }
            }
            mfllLogI("%s: wait MSF-P2-S%d deque [-]", __FUNCTION__, stage);
        }
        mfllTraceEnd();
    }

    m_blendCount++;

lbExit:
    if (err != MfllErr_Ok)
        mfllLogE("%s: error code(%d)", __FUNCTION__, (int)err);

    return err;
}

enum MfllErr MfllMfb::convertYuvFormatByMdp(
        IMfllImageBuffer* input,
        IMfllImageBuffer* output,
        IMfllImageBuffer* output_q,
        const MfllRect_t& output_crop,
        const MfllRect_t& output_q_crop,
        enum YuvStage s /* = YuvStage_Unknown */)
{
    mfllAutoLog("convertYuvFormatByMdp_itr");

    if (input == nullptr || output == nullptr) {
        mfllLogE("%s: input or output image buffer is NULL", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    // use full size crop if cropping window is zero
    if (output_crop.w <= 0 || output_crop.h <= 0) {
        mfllLogE("%s: cropping window is 0 (invalid)", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    if (output_q && (output_q_crop.w <= 0 || output_q_crop.h <= 0)) {
        mfllLogE("%s: cropping window 2 is 0 (invalid)", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    return convertYuvFormatByMdp(
            reinterpret_cast<IImageBuffer*>(input->getImageBuffer()),
            reinterpret_cast<IImageBuffer*>(output->getImageBuffer()),
            (output_q == nullptr) ? nullptr : reinterpret_cast<IImageBuffer*>(output_q->getImageBuffer()),
            output_crop,
            output_q_crop,
            s);
}


enum MfllErr MfllMfb::convertYuvFormatByMdp(
        IImageBuffer* src,
        IImageBuffer* dst,
        IImageBuffer* dst2,
        const MfllRect_t& output_crop,
        const MfllRect_t& output_q_crop,
        const enum YuvStage s)
{
    mfllAutoLogFunc();

    MfllErr err = MfllErr_Ok;
    MRect crop = MRect(MPoint(output_crop.x, output_crop.y), MSize(output_crop.w, output_crop.h));
    MRect crop2 = MRect(MPoint(output_q_crop.x, output_q_crop.y), MSize(output_q_crop.w, output_q_crop.h));

    // check arguments
    if (src == nullptr || dst == nullptr) {
        mfllLogE("%s: source or dst image is NULL", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    // create IImageTransform
    std::unique_ptr<IImageTransform, std::function<void(IImageTransform*)>> t(
            IImageTransform::createInstance(),
            [](IImageTransform *p)mutable->void { if (p) p->destroyInstance(); });

    if (t.get() == nullptr) {
        mfllLogE("%s: create IImageTransform failed", __FUNCTION__);
        return MfllErr_UnexpectedError;
    }

    auto bRet = (dst2 == nullptr)
        ? t->execute(src, dst, nullptr, crop, 0, 3000)
        : t->execute(src, dst, dst2, crop, crop2, 0, 0, 3000);

    if (!bRet) {
        mfllLogE("%s: IImageTransform::execute returns fail", __FUNCTION__);
        return MfllErr_UnexpectedError;
    }

    // At this stage, we need to save Debug Exif
    bool bUpdateExifBfbld = (s == YuvStage_BaseYuy2);

    if (bUpdateExifBfbld) {
        // {{{
        auto index = m_pCore->getIndexByNewIndex(0); // Use the first metadata
        MetaSet_T metaset;
        IMetadata* pHalMeta = nullptr;
        IImageBuffer* pImgLcsoRaw = nullptr;
        EIspProfile_T profile = static_cast<EIspProfile_T>(MFLL_ISP_PROFILE_ERROR);

        // retrieve data
        {
            Mutex::Autolock __l(m_mutex);

            // check if metadata is ok
            if (static_cast<size_t>(index) >= m_vMetaSet.size()) {
                mfllLogE("%s: index(%d) is out of metadata set size(%zu) ",
                        __FUNCTION__, index, m_vMetaSet.size());
                return MfllErr_UnexpectedError;
            }

            metaset = getMainMetasetLocked();
            pHalMeta = getMainHalMetaLocked();
            pImgLcsoRaw = m_vImgLcsoRaw[index]; // get LCSO RAW
        }

        // get profile
        if (isZsdMode(m_shotMode)) {
            profile = get_isp_profile(eMfllIspProfile_BeforeBlend_Zsd, m_shotMode);// Not related with MNR
            if (IS_WRONG_ISP_PROFILE(profile)) {
                mfllLogE("%s: error isp profile mapping", __FUNCTION__);
            }
        }
        else {
            profile = get_isp_profile(eMfllIspProfile_BeforeBlend, m_shotMode);
            if (IS_WRONG_ISP_PROFILE(profile)) {
                mfllLogE("%s: error isp profile mapping", __FUNCTION__);
            }
        }

        // get size of dip_x_reg_t
        size_t regTableSize = m_pNormalStream->getRegTableSize();
        if (regTableSize <= 0) {
            mfllLogE("%s: unexpected tuning buffer size: %zu", __FUNCTION__, regTableSize);
            return MfllErr_UnexpectedError;
        }

        /* add tuning data is necessary */
        std::unique_ptr<char[]> tuning_reg(new char[regTableSize]());
        void *tuning_lsc = nullptr;
        TuningParam rTuningParam;
        rTuningParam.pRegBuf = tuning_reg.get();
        rTuningParam.pLcsBuf = pImgLcsoRaw; // gives LCS buffer, even though it's NULL.
        // PGN enable due to BFBLD stage is RAW->YUV
        IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_3A_PGN_ENABLE, MTRUE);
        IMetadata::setEntry<MUINT8>(&metaset.halMeta, MTK_3A_ISP_PROFILE, profile);

        // Set ISP to retrieve dip_x_reg_t
        if (this->setIsp(0, metaset, &rTuningParam, nullptr, src) == 0) {
#ifdef __DEBUG
            mfllLogD("%s: get tuning data, reg=%p, lsc=%p",
                    __FUNCTION__, rTuningParam.pRegBuf, rTuningParam.pLsc2Buf);
#endif

            MfbPlatformConfig pltcfg;
            pltcfg[ePLATFORMCFG_STAGE] = STAGE_BASE_YUV;
            pltcfg[ePLATFORMCFG_INDEX] = 0;
            pltcfg[ePLATFORMCFG_DIP_X_REG_T] =
                reinterpret_cast<intptr_t>((volatile void*)rTuningParam.pRegBuf);

            if (!refine_register(pltcfg)) {
                mfllLogE("%s: refine_register returns fail", __FUNCTION__);
            }
            else {
#ifdef __DEBUG
                mfllLogD("%s: refine_register ok", __FUNCTION__);
#endif
            }
            // update Exif info
            if (m_bExifDumpped[STAGE_BASE_YUV] == 0) {
                dump_exif_info(m_pCore, tuning_reg.get(), STAGE_BASE_YUV);
                m_bExifDumpped[STAGE_BASE_YUV] = 1;
            }
        }
        else {
            mfllLogE("%s: setIsp returns fail, Exif may be lost", __FUNCTION__);
        }
        // }}}
    } // fi (YuvStage_BaseYuy2)

    return err;
}
