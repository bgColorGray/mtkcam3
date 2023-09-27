/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_ICONFIGP1DMANEEDPOLICY_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_ICONFIGP1DMANEEDPOLICY_H_
//
#include "types.h"

#include <functional>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {


/**
 * The function type definition.
 * It is used to decide P1 DMA need at the configuration stage.
 *
 * @param[out] pvOut: P1 DMA need
 * The value shows which dma are needed.
 * For example,
 *      pvOut->at(1) & P1_IMGO != 0 indicates that IMGO is needed for sensorId 1.
 *      pvOut->at(3) & P1_RRZO != 0 indicates that RRZO is needed for sensorId 3.
 *
 * @param[in] pP1HwSetting: the P1 hardware settings
 *
 * @param[in] pSensorSetting: the sensor settings
 *
 * @param[in] pStreamingFeatureSetting: the streaming feature settings.
 *
 * @param[in] PipelineStaticInfo: Pipeline static information.
 *
 * @param[in] PipelineUserConfiguration: Pipeline user configuration.
 *
 * @return
 *      0 indicates success; otherwise failure.
 */
using FunctionType_Configuration_P1DmaNeedPolicy
    = std::function<int(
        SensorMap<uint32_t>* /*pvOut*/,
        SensorMap<P1HwSetting> const* /*pP1HwSetting*/,
        StreamingFeatureSetting const* /*pStreamingFeatureSetting*/,
        PipelineStaticInfo const* /*pPipelineStaticInfo*/,
        PipelineUserConfiguration const* /*pPipelineUserConfiguration*/
    )>;


//==============================================================================


/**
 * Policy instance makers
 *
 */

// default version
FunctionType_Configuration_P1DmaNeedPolicy makePolicy_Configuration_P1DmaNeed_Default();

// multi-cam version
FunctionType_Configuration_P1DmaNeedPolicy makePolicy_Configuration_P1DmaNeed_MultiCam();

// security version
FunctionType_Configuration_P1DmaNeedPolicy makePolicy_Configuration_P1DmaNeed_Security();

// Always-On Vision version
FunctionType_Configuration_P1DmaNeedPolicy makePolicy_Configuration_P1DmaNeed_AOV();

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_POLICY_ICONFIGP1DMANEEDPOLICY_H_

