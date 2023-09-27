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

#include "DsdnHal.h"
#include "StreamingFeature_Common.h"
#include <isp_tuning/isp_tuning.h>
#include <mtkcam3/feature/utils/ISPProfileMapper.h>
#include <mtkcam3/3rdparty/mtk/mtk_feature_type.h>

#define PIPE_CLASS_TAG "DsdnHal"
#define PIPE_TRACE TRACE_DSDN_HAL
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

#define VER_NUM_DSDN20  (20)
#define VER_NUM_DSDN21  (21)
#define VER_NUM_DSDN25  (25)
#define VER_NUM_DSDN30  (30)

#define FMT_DSDN20_FULL eImgFmt_NV21
#define FMT_DSDN20_DS   eImgFmt_YUY2
#define FMT_DSDN20_DN   eImgFmt_NV21
#define FMT_DSDN20_DN_10BIT   eImgFmt_YVU_P010

#define FMT_DSDN25_FULL         eImgFmt_NV21
#define FMT_DSDN30_PDS          eImgFmt_MTK_YUV_P012
#define FMT_DSDN25_DS           eImgFmt_MTK_YUV_P010
#define FMT_DSDN25_DN           eImgFmt_MTK_YUV_P010
#define FMT_DSDN30_OMCMV        eImgFmt_STA_4BYTE
#define FMT_DSDN30_CONF         eImgFmt_STA_BYTE
#define FMT_DSDN30_IDI          eImgFmt_NV12
#define FMT_DSDN30_WEIGHT       eImgFmt_STA_BYTE
#define FMT_DSDN30_DS_WEIGHT    eImgFmt_STA_BYTE

// Valid Duation = 100ms
#define VALID_DURATION_NS  (100*1000*1000)

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

DsdnHal::DsdnHal()
{
    TRACE_FUNC_ENTER();
    mDebugRatio.mMul = property_get_int32("vendor.debug.fpipe.dsdn.multiple", 0);
    mDebugRatio.mDiv = property_get_int32("vendor.debug.fpipe.dsdn.divider", 0);
    mDebugFmtNV21 = property_get_int32("vendor.debug.fpipe.dsdn.fmt.nv21", 0);
    mMinSize = property_get_int32("vendor.debug.fpipe.dsdn.minsize", 0);
    mForceLoopValid = property_get_int32("vendor.debug.fpipe.dsdn.force.valid", 0);
    TRACE_FUNC_EXIT();
}

DsdnHal::~DsdnHal()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MVOID DsdnHal::init(const DSDNParam &param, const MSize &fullSize, const MSize &fdSize)
{
    TRACE_FUNC_ENTER();
    if( param.isDSDN() )
    {
        mCfg = param;
        mMaxFullSize = align(fullSize, BUF_ALLOC_ALIGNMENT_BIT);
        mMaxFDSize = align(fdSize, BUF_ALLOC_ALIGNMENT_BIT);

        if ( param.isDSDN25() || param.isDSDN30() )
        {
            mVer = param.isDSDN25() ? VER_NUM_DSDN25 : VER_NUM_DSDN30;
            mFullFmt = FMT_DSDN25_FULL;
            mDSFmt = mDebugFmtNV21 ? eImgFmt_NV21 : FMT_DSDN25_DS;
            mDNFmt = mDebugFmtNV21 ? eImgFmt_NV21 : FMT_DSDN25_DN;
            mPDSFmt = mDebugFmtNV21 ? eImgFmt_NV21 : FMT_DSDN30_PDS;
            if( mDebugFmtNV21 ) MY_LOGI("Use debug ds/dn fmt: eImgFmt_NV21");
        }
        else if( mCfg.isDSDN20() )
        {
            mVer = param.isDSDN20Bit10() ? VER_NUM_DSDN21 : VER_NUM_DSDN20;
            initDSDN20Profile();
            mMaxRatio = param.mMaxDsRatio;
            if( mDebugRatio )
            {
                mMaxRatio = mDebugRatio;
                MY_LOGI("Use debug ratio(m/d): (%d/%d) => (%d/%d)",
                        param.mMaxDsRatio.mMul, param.mMaxDsRatio.mDiv,
                        mDebugRatio.mMul, mDebugRatio.mDiv);
            }
            mFullFmt = FMT_DSDN20_FULL;
            mDSFmt = FMT_DSDN20_DS;
            mDNFmt = param.isDSDN20Bit10() ? FMT_DSDN20_DN_10BIT : FMT_DSDN20_DN;
            mCfg.mMaxDSLayer = 1;
        }
    }
    MY_LOGD("mode=%s layer=%d fmt(full/ds/dn)=%s/%s/%s dsdn20Prof(%d), ver(%d), forceLoopValid(%d)",
            mCfg.toModeStr(), mCfg.mMaxDSLayer,
            toName(mFullFmt), toName(mDSFmt), toName(mDNFmt),
            mDSDN20Profile, mVer, mForceLoopValid);
    TRACE_FUNC_EXIT();
}

MVOID DsdnHal::uninit()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL DsdnHal::isSupportDSDN20() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return mCfg.isDSDN20();
}

MBOOL DsdnHal::isSupportDSDN25() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return mCfg.isDSDN25();
}

MBOOL DsdnHal::isSupportDSDN30() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return mCfg.isDSDN30();
}

EImageFormat DsdnHal::getFormat(eDSDNImg img) const
{
    TRACE_FUNC_ENTER();
    EImageFormat fmt = eImgFmt_UNKNOWN;
    switch(img)
    {
    case DsdnHal::FULL:         fmt = mFullFmt; break;
    case DsdnHal::DS:           fmt = mDSFmt; break;
    case DsdnHal::DN:           fmt = mDNFmt; break;
    case DsdnHal::PDS:          fmt = mPDSFmt; break;
    case DsdnHal::OMCMV:        fmt = FMT_DSDN30_OMCMV; break;
    case DsdnHal::CONF:         fmt = FMT_DSDN30_CONF; break;
    case DsdnHal::IDI:          fmt = FMT_DSDN30_IDI; break;
    case DsdnHal::MSF_WEIGHT:   fmt = FMT_DSDN30_WEIGHT; break;
    case DsdnHal::DS_WEIGHT:    fmt = FMT_DSDN30_DS_WEIGHT; break;
    default:                    fmt = eImgFmt_UNKNOWN; break;
    }
    TRACE_FUNC_EXIT();
    return fmt;
}

MUINT32 DsdnHal::getVersion() const
{
    return mVer;
}

MUINT32 DsdnHal::getMaxDSLayer() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    // max downscale layer:
    // for { f0, f1, f2, f3 } => max DS layer = 3
    return mCfg.mMaxDSLayer;
}

std::vector<MSize> DsdnHal::getMaxDSSizes() const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return getDSSizes(mMaxFullSize, mMaxFullSize, mMaxFDSize, mMaxRatio);
}

MSize DsdnHal::getSize(const std::vector<MSize> &dsSizes, eDSDNImg img) const
{
    switch(img)
    {
    case DsdnHal::OMCMV:    return getOmcSize(dsSizes);
    case DsdnHal::CONF:     return getConfSize(dsSizes);
    case DsdnHal::IDI:      return getIdiSize(dsSizes);
    default:                return MSize(0,0);
    }
}

MSize DsdnHal::getConfSize(const std::vector<MSize> &dsSizes) const
{
    MSize size;
    if( isSupportDSDN30() && dsSizes.size() )
    {
        size.w = (dsSizes[0].w + 31) >> 5;
        size.h = (dsSizes[0].h + 31) >> 5;
        size = align(size, 1); // 2 byte align
    }
    return size;
}

MSize DsdnHal::getIdiSize(const std::vector<MSize> &dsSizes) const
{
    MSize size;
    if( isSupportDSDN30() && !dsSizes.empty() )
    {
        size = dsSizes.back();
    }
    return size;
}

MSize DsdnHal::getOmcSize(const std::vector<MSize> &dsSizes) const
{
    MSize size;
    if( isSupportDSDN30() && dsSizes.size() )
    {
        size.w = (dsSizes[0].w + 15) >> 4;
        size.h = (dsSizes[0].h + 15) >> 4;
    }
    return size;
}

std::vector<MSize> DsdnHal::getDSSizes(const MSize &inSize, const MSize &nextSize, const MSize &fdSize, const DSDNRatio &ratio) const
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
    return (isSupportDSDN25() || isSupportDSDN30()) ? getDSSizes_25(inSize) :
           isSupportDSDN20() ? getDSSizes_20(nextSize.size() ? nextSize : inSize, fdSize, ratio) :
           std::vector<MSize>();
}

std::vector<MINT32> DsdnHal::getProfiles(MINT32 hdrHalMode) const
{
    std::vector<MINT32> out;
    if( isSupportDSDN25() || isSupportDSDN30() )
    {
        out.reserve(mCfg.mMaxDSLayer);
        for( uint32_t layer = 0; layer < mCfg.mMaxDSLayer ; layer++)
        {
            out.push_back(getDSDN25Profile(layer, hdrHalMode));
        }
    }
    else if( isSupportDSDN20() )
    {
        out.reserve(2);
        out.push_back(-1);
        out.push_back(mDSDN20Profile);
    }
    return out;
}

MBOOL DsdnHal::isLoopValid(const MSize &curInSize, MINT64 curTime, MBOOL curOn,
                         const MSize &preInSize, MINT64 preTime, MBOOL preOn) const
{
    MBOOL valid = MTRUE;
    if( isSupportDSDN30() )
    {
        valid = (preOn && curOn) && (curInSize == preInSize) && (mForceLoopValid || ((curTime - preTime) < VALID_DURATION_NS));
        MY_LOGD_IF(!valid , "loop not valid, need reset! cur size/t/run(%dx%d/%" PRId64 "/%d), pre(%dx%d/%" PRId64 "/%d)",
                        curInSize.w, curInSize.h, curTime, curOn, preInSize.w, preInSize.h, preTime, preOn);
    }
    return valid;
}

std::vector<MSize> DsdnHal::getDSSizes_25(const MSize &fullSize) const
{
    std::vector<MSize> dsSizes;
    dsSizes.reserve(mCfg.mMaxDSLayer);
    MSize full = fullSize, ds;

    // 5 layer => { 1/2, 1/4, 1/8, 1/16, 1/32 }
    for( MUINT32 i = 0; i < mCfg.mMaxDSLayer; ++i )
    {
        ds = MSize( ((full.w + 3)/4 * 2), ((full.h + 3)/4 * 2) );
        if( mMinSize && (ds.w < mMinSize || ds.h < mMinSize) )
        {
            MY_LOGW("DSDN layer[%d]=(%dx%d) reach min size(%d), skip",
                    i, ds.w, ds.h, mMinSize);
            break;
        }
        dsSizes.push_back(ds);
        full = ds;
    }
    return dsSizes;
}

std::vector<MSize> DsdnHal::getDSSizes_20(const MSize &fullSize, const MSize &/*fdSize*/, DSDNRatio ratio) const
{
    TRACE_FUNC_ENTER();
    std::vector<MSize> dsSizes;
    if( (ratio.mMul * mMaxRatio.mDiv) > (mMaxRatio.mMul * ratio.mDiv) )
    {
        MY_LOGW("invalid ratio, use max ratio: (%d/%d) => (%d/%d)",
                ratio.mMul, ratio.mDiv, mMaxRatio.mMul, mMaxRatio.mDiv);
        ratio = mMaxRatio;
    }

    MSize size = fullSize.size() ? fullSize : mMaxFullSize;
    if( ratio.mDiv )
    {
        size.w = (fullSize.w * ratio.mMul + ratio.mDiv - 1) / ratio.mDiv;
        size.h = (fullSize.h * ratio.mMul + ratio.mDiv - 1) / ratio.mDiv;
    }
    size = align(size, 1); // 2 byte align
    dsSizes.push_back(size);
    TRACE_FUNC_EXIT();
    return dsSizes;
}

MVOID DsdnHal::initDSDN20Profile()
{
    ISPProfileMapper *profMapper = ISPProfileMapper::getInstance();
    bool success = false;
    NSIspTuning::EIspProfile_T outProf = NSIspTuning::EIspProfile_Preview;
    if( profMapper )
    {
        ProfileMapperKey key;
        key.type = eMappingKey_Proprietary;
        key.itemKey.miFeatureID = NSPipelinePlugin::NO_FEATURE_NORMAL;
        key.itemKey.mScenario = eMappingScenario_Preview;
        success = profMapper->mappingProfile(key, eStage_DSDN20_2ndRun, outProf);
    }

    if( success )
    {
        mDSDN20Profile = (MINT32)outProf;
    }
    else
    {
        mDSDN20Profile = -1;
        MY_LOGW("Can not get DSDN20 2nd run ISP Profile !!, profMapper(%p)", profMapper);
    }
}


MINT32 DsdnHal::getDSDN25Profile(uint32_t layer, MINT32 hdrHalMode)
{
    TRACE_FUNC_ENTER();
    MINT32 prof = -1;
    MBOOL hdrOn = hdrHalMode;
    (void)hdrOn;
    switch (layer)
    {
#if SUPPORT_DSDN25
#define GET_DSDN_PROF(hdr, layer) \
    ( hdr ? (MINT32)NSIspTuning::EIspProfile_VHDR_DSDN_MSF_##layer : (MINT32)NSIspTuning::EIspProfile_DSDN_MSF_##layer)

    case 0:   prof = GET_DSDN_PROF(hdrOn,0); break;
    case 1:   prof = GET_DSDN_PROF(hdrOn,1); break;
    case 2:   prof = GET_DSDN_PROF(hdrOn,2); break;
    case 3:   prof = GET_DSDN_PROF(hdrOn,3); break;
    case 4:   prof = GET_DSDN_PROF(hdrOn,4); break;
#undef GET_DSDN_PROF
#endif
    default: MY_LOGE("missing dsdn25 profile: layer(%d)", layer); break;
    }
    TRACE_FUNC_EXIT();
    return prof;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

