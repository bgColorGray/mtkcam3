/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2021. All rights reserved.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/**
 * @file CHDRPolicyHelper.cpp
 * @parse HDR info from HDRPolicyHelper and do decisions for CHDR
 */

#define LOG_TAG "mtkcam-CHDRPolicyHelper"

#include "mtkcam3/feature/chdr/CHDRPolicyHelper.h"

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CHDR_HAL);

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
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {
/******************************************************************************
 *
 ******************************************************************************/
CHDRPolicyHelper::
CHDRPolicyHelper(const uint32_t& hdrHalMode)
    : mHDRHalMode(hdrHalMode)
    , mbIsHDR(false)
    , mValidExpNum(MTK_STAGGER_VALID_EXPOSURE_NON) {}

/******************************************************************************
 *
 ******************************************************************************/
void
CHDRPolicyHelper::
updateInfo(std::shared_ptr<HDRPolicyHelper>& hdrPolicyHelper) {
    mbIsHDR = hdrPolicyHelper->isHDR();
    mValidExpNum = hdrPolicyHelper->getValidExposureNum();
    MY_LOGD("mbIsHDR:%d, mValidExpNum:%d", mbIsHDR, mValidExpNum);
}

/******************************************************************************
 *
 ******************************************************************************/
uint32_t
CHDRPolicyHelper::
getHDRHalMode() {
    return mHDRHalMode;
}

/******************************************************************************
 *
 ******************************************************************************/
bool
CHDRPolicyHelper::
isHDR() {
    return mbIsHDR;
}

/******************************************************************************
 *
 ******************************************************************************/
uint32_t
CHDRPolicyHelper::
getValidExposureNum() {
    return mValidExpNum;
}

/******************************************************************************
 *
 ******************************************************************************/
void
CHDRPolicyHelper::
updateSeamlessSwitchMetadata(
        std::shared_ptr<IMetadata>& dstMeta,
        const std::shared_ptr<IMetadata>& srcMeta) {
    if (isHDR()) {
        switch(getHDRHalMode()) {
            case MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP:
            case MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP:
            case MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER:
                {
                    int32_t aeTargetMode = -1, validExpNum = -1, ispFusNum = -1;

                    bool ret = true;
                    ret &= !IMetadata::getEntry<MINT32>(
                            srcMeta.get(), MTK_3A_FEATURE_AE_TARGET_MODE, aeTargetMode);
                    ret &= !IMetadata::getEntry<MINT32>(
                            srcMeta.get(), MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM, validExpNum);
                    ret &= !IMetadata::getEntry<MINT32>(
                            srcMeta.get(), MTK_3A_ISP_FUS_NUM, ispFusNum);

                    if (ret) {
                        IMetadata::setEntry<MINT32>(
                                dstMeta.get(), MTK_3A_FEATURE_AE_TARGET_MODE,
                                MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL);
                        IMetadata::setEntry<MINT32>(
                                dstMeta.get(), MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM,
                                MTK_STAGGER_VALID_EXPOSURE_1);
                        IMetadata::setEntry<MINT32>(
                                dstMeta.get(), MTK_3A_ISP_FUS_NUM,
                                MTK_3A_ISP_FUS_NUM_1);
                        MY_LOGD("update seamless-switch-related metadata");
                    } else {
                        MY_LOGD("seamless-switch-related metadata already exist "
                                "(aeTargetMode:%d, validExpNum:%d, ispFusNum:%d)",
                                aeTargetMode, validExpNum, ispFusNum);
                    }
                }
                break;
            case MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE:
            case MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW:
            case MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE_PREVIEW:
                {
                    uint8_t hdrHalAppMode = 0;
                    int32_t hdrHalMode = -1, aeTargetMode = -1, ispFusNum = -1;

                    bool ret = true;
                    ret &= !IMetadata::getEntry<MUINT8>(
                            srcMeta.get(), MTK_3A_HDR_MODE, hdrHalAppMode);
                    ret &= !IMetadata::getEntry<MINT32>(
                            srcMeta.get(), MTK_HDR_FEATURE_HDR_HAL_MODE, hdrHalMode);
                    ret &= !IMetadata::getEntry<MINT32>(
                            srcMeta.get(), MTK_3A_FEATURE_AE_TARGET_MODE, aeTargetMode);
                    ret &= !IMetadata::getEntry<MINT32>(
                            srcMeta.get(), MTK_3A_ISP_FUS_NUM, ispFusNum);

                    if (ret) {
                        IMetadata::setEntry<MUINT8>(
                                dstMeta.get(), MTK_3A_HDR_MODE,
                                static_cast<MUINT8>(HDRMode::OFF));
                        IMetadata::setEntry<MINT32>(
                                dstMeta.get(), MTK_HDR_FEATURE_HDR_HAL_MODE,
                                MTK_HDR_FEATURE_HDR_HAL_MODE_OFF);
                        IMetadata::setEntry<MINT32>(
                                dstMeta.get(), MTK_3A_FEATURE_AE_TARGET_MODE,
                                MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL);
                        IMetadata::setEntry<MINT32>(
                                dstMeta.get(), MTK_3A_ISP_FUS_NUM,
                                MTK_3A_ISP_FUS_NUM_NON);
                        MY_LOGD("update seamless-switch-related metadata");
                    } else {
                        MY_LOGD("seamless-switch-related metadata already exist "
                                "(hdrHalAppMode:%d, hdrHalMode:%d, "
                                "aeTargetMode:%d, ispFusNum:%d)",
                                hdrHalAppMode, hdrHalMode, aeTargetMode, ispFusNum);
                    }
                }
                break;
            default:
                break;
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
CHDRPolicyHelper::
updateStatusMetadata (
        std::shared_ptr<IMetadata>& dstMeta,
        const std::shared_ptr<IMetadata>& srcMeta) {
    if (isHDR()) {
        int32_t status = -1;
        switch(getValidExposureNum()) {
            case MTK_STAGGER_VALID_EXPOSURE_3:
                status = MTK_FEATURE_VHDR_STATUS_3EXP;
                break;
            case MTK_STAGGER_VALID_EXPOSURE_2:
                status = MTK_FEATURE_VHDR_STATUS_2EXP;
                break;
            case MTK_STAGGER_VALID_EXPOSURE_1:
                status = MTK_FEATURE_VHDR_STATUS_1EXP;
                break;
            case MTK_STAGGER_VALID_EXPOSURE_NON:
                status = MTK_FEATURE_VHDR_STATUS_NON;
                break;
            default:
                status = MTK_FEATURE_VHDR_STATUS_NON;
                break;
        }

        int32_t preStatus = -1;
        IMetadata::getEntry<MINT32>(
                srcMeta.get(), MTK_FEATURE_VHDR_STATUS, preStatus);
        IMetadata::setEntry<MINT32>(
                dstMeta.get(), MTK_FEATURE_VHDR_STATUS, status);
        MY_LOGD("set MTK_FEATURE_VHDR_STATUS %d -> %d", preStatus, status);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
CHDRPolicyHelper::
updateMetadata (
        std::shared_ptr<IMetadata>& dstMeta,
        const std::shared_ptr<IMetadata>& srcMeta,
        const bool bIsZsl) {
    if (bIsZsl) {
        updateStatusMetadata(dstMeta, srcMeta);
    } else {
        updateSeamlessSwitchMetadata(dstMeta, srcMeta);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
SensorMap<std::shared_ptr<CHDRPolicyHelper>>
createCHDRPolicyHelper(
        const SensorMap<std::shared_ptr<HDRPolicyHelper>>& vHdrPolicyHelper) {
    SensorMap<std::shared_ptr<CHDRPolicyHelper>> vCHDRPolicyHelper;
    for (auto& it : vHdrPolicyHelper) {
        auto& sensorId = it.first;
        auto& hdrPolicyHelper = it.second;

        auto hdrHalMode = hdrPolicyHelper->getHDRHalMode();
        auto chdrPolicyHelper = std::make_shared<CHDRPolicyHelper>(hdrHalMode);

        vCHDRPolicyHelper.emplace(sensorId, chdrPolicyHelper);
    }
    return vCHDRPolicyHelper;
}

};  // namespace featuresetting
};  // namespace policy
};  // namespace pipeline
};  // namespace v3
};  // namespace NSCam
