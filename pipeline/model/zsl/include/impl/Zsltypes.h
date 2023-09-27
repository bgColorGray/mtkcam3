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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_INCLUDE_IMPL_TYPES_H_
#define _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_INCLUDE_IMPL_TYPES_H_

#include <mtkcam3/pipeline/model/types.h>
#include <mtkcam3/pipeline/model/IPipelineModelCallback.h>
#include <mtkcam3/pipeline/policy/types.h>
#include <mtkcam3/pipeline/policy/IPipelineSettingPolicy.h>
#include <mtkcam3/pipeline/pipeline/IPipelineNode.h>
#include <mtkcam3/pipeline/pipeline/IPipelineContextBuilder.h>

#include <unordered_set>
#include <vector>
#include <utils/RefBase.h>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace model {

using IFrameBuilder = NSCam::v3::pipeline::NSPipelineContext::IFrameBuilder;
using GroupFrameType = NSCam::v3::pipeline::NSPipelineContext::GroupFrameType;
using FrameBuildersOfOneRequest = std::vector<std::shared_ptr<IFrameBuilder>>;

/******************************************************************************
 *  Configuration Parameters
 ******************************************************************************/
struct ZslConfigParams
{
    SensorMap<policy::ParsedStreamInfo_P1> const*   pParsedStreamInfo_P1 = nullptr;

    android::wp<IPipelineModelCallback>             pCallback;
};


/******************************************************************************
 *  Request Parameters
 ******************************************************************************/
struct ZslRequestParams
{
    uint32_t                                        requestNo = 0;

    /**
     * Policy to select zsl buffers.
     */
    policy::ZslPolicyParams                         zslPolicyParams;

    /**
     * To get historyBuffer.
     */
    std::shared_ptr<NSPipelineContext::IHistoryBufferProvider>
                                                    pHistoryBufferProvider;

    /**
     * RequestResultParams of zsl frames.
     */
    std::vector<std::shared_ptr<policy::pipelinesetting::RequestResultParams>>
                                                    vReqResult;

    /**
     * FrameBuilders of zsl frames.
     *
     * Note:
     *  ReqResult & FrameBuilder is a pair. (#vReqResult == #vFrameBuilder)
     */
    FrameBuildersOfOneRequest                       vFrameBuilder;

    /**
     * pvSensorMode stores requested sensor modes.
     */
    SensorMap<uint32_t> const*                      pvSensorMode;

    /**
     * pStreamingSensorList stores the streaming sensors
     *  @note used for generate required sensor mode of a capture request.
     */
    std::vector<uint32_t> const*                    pStreamingSensorList;

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace model
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_INCLUDE_IMPL_TYPES_H_

