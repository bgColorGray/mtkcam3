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

#ifndef _CHDR_POLICY_HELPER_H_
#define _CHDR_POLICY_HELPER_H_

#include <mtkcam3/feature/vhdr/HDRPolicyHelper.h>

#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/IMetadata.h>

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace featuresetting {
/******************************************************************************
 *
 ******************************************************************************/
class CHDRPolicyHelper
{
  public:
    CHDRPolicyHelper(const uint32_t& hdrHalMode);

    void updateInfo(std::shared_ptr<HDRPolicyHelper>& hdrPolicyHelper);

    void updateMetadata(
            std::shared_ptr<IMetadata>& dstMeta,
            const std::shared_ptr<IMetadata>& srcMeta,
            const bool bIsZsl);

    uint32_t getHDRHalMode();

    bool isHDR();

    uint32_t getValidExposureNum();

  private:
    void updateStatusMetadata(
            std::shared_ptr<IMetadata>& dstMeta,
            const std::shared_ptr<IMetadata>& srcMeta);

    void updateSeamlessSwitchMetadata(
            std::shared_ptr<IMetadata>& dstMeta,
            const std::shared_ptr<IMetadata>& srcMeta);

  private:
    uint32_t mHDRHalMode;
    bool     mbIsHDR;
    uint32_t mValidExpNum;
};

/******************************************************************************
 *
 ******************************************************************************/
SensorMap<std::shared_ptr<CHDRPolicyHelper>>
createCHDRPolicyHelper(
        const SensorMap<std::shared_ptr<HDRPolicyHelper>>& vHdrPolicyHelper);

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace featuresetting
};  // namespace policy
};  // namespace pipeline
};  // namespace v3
};  // namespace NSCam

#endif  // _CHDR_POLICY_HELPER_H_