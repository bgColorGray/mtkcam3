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

#ifndef _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_PIPELINEMODELIMPL_SINGLEHW_H_
#define _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_PIPELINEMODELIMPL_SINGLEHW_H_

#include <memory>
#include <mutex>
#include <string>
//
#include <mtkcam3/pipeline/pipeline/IPipelineContext.h>
#include <mtkcam3/main/standalone_isp/IISPPipelineModel.h>
#include "HW_Handler/HW_Handler.h"

//
using namespace std;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace isppipeline {
namespace model {

/******************************************************************************
 *
 ******************************************************************************/
class PipelineModelSingleHW
    : public IISPPipelineModel
    , public NSCam::v3::pipeline::IPipelineBufferSetFrameControl::IAppCallback
    , public NSCam::v3::pipeline::NSPipelineContext::DataCallbackBase
{
public:
    using HalMetaStreamBuffer = NSCam::v3::Utils::HalMetaStreamBuffer;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Instantiation data (initialized at the creation stage).
    //
    int32_t                                 mLogLevel = 0;

protected:  ////    Open data (initialized at the open stage).
    android::wp<IISPPipelineModelCallback>     mCallback;
    std::string                                mModelName;
    android::sp<HW_Handler>                    mpHandler = nullptr;

protected:  ////    Configuration data and members
    std::timed_mutex                        mLock;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation.
                    PipelineModelSingleHW();

public:
    // build pipelinecontext
    struct ConfigureInfo
    {
        android::sp<IMetaStreamInfo>                pAppMeta_Control = nullptr; // p2c/jpeg/fd input app meta, camera app control

        std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> streamInfo;

    };
    struct ConfigureInfo                            mConfigureInfo;

public:     ////    Operations.

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineModel Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Interfaces.

    virtual auto    open(
                        std::string const& Name,
                        android::wp<IISPPipelineModelCallback> const& callback
                    ) -> int override;
    virtual auto    close() -> void override;
    virtual auto    configure(
                        std::shared_ptr<UserConfigurationParams>const& params
                    ) -> int override;
    virtual auto    submitRequest(
                        std::vector<std::shared_ptr<UserRequestParams>>const& requests,
                        uint32_t& numRequestProcessed
                    ) -> int override;
    virtual auto    beginFlush() -> int override;
    virtual auto    endFlush() -> void override;
    virtual auto    queryFeature(IMetadata const* pInMeta, std::vector<std::shared_ptr<IMetadata>> &outSetting) -> void override;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPipelineBufferSetFrameControl::IAppCallback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    virtual MVOID   updateFrame(
                        MUINT32 const requestNo,
                        MINTPTR const userId,
                        Result const& result
                    ) override;
    virtual MVOID   onMetaCallback(
                        MUINT32              requestNo __unused,
                        Pipeline_NodeId_T    nodeId __unused,
                        StreamId_T           streamId __unused,
                        IMetadata const&     rMetaData __unused,
                        MBOOL                errorResult __unused
                    ) override;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace model
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_PIPELINEMODELIMPL_SINGLEHW_H_

