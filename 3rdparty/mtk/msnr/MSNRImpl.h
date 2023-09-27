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
#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_MSNRPLUGIN_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_MSNRPLUGIN_H_

#include <stdlib.h>
#include <sys/prctl.h>
#include <utils/KeyedVector.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <future>
#include <map>
#include <unordered_map>
#include <string>
//
#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>
//
#include <mtkcam3/feature/msnr/IMsnr.h>
//
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h> // tuning file naming
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>

using NSCam::TuningUtils::scenariorecorder::ResultParam;
using NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo;
using NSCam::TuningUtils::scenariorecorder::DecisionType::DECISION_FEATURE;
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace NSPipelinePlugin {

/******************************************************************************
*
******************************************************************************/
class MsnrPluginProviderImp : public YuvPlugin::IProvider
{
    typedef YuvPlugin::Property Property;
    typedef YuvPlugin::Selection Selection;
    typedef YuvPlugin::Request::Ptr RequestPtr;
    typedef YuvPlugin::RequestCallback::Ptr RequestCallbackPtr;

public:

    MsnrPluginProviderImp();

    virtual ~MsnrPluginProviderImp();

    virtual const Property&        property() override;

    virtual MERROR                 negotiate(Selection& sel) override;

    virtual void                   init() override;

    virtual MERROR                 process(RequestPtr pRequest,
                                           RequestCallbackPtr pCallback = nullptr) override;

    virtual void                   abort(std::vector<RequestPtr>& pRequests) override;

    virtual void                   uninit() override;

protected:

    bool                           checkImg(IImageBuffer** bufptr, BufferHandle::Ptr bufhandle, const char* io, const char* name);

    bool                           checkMeta(IMetadata** metaptr, MetadataHandle::Ptr metahandle, const char* io, const char* name);

    void                           bufferDump(const IMetadata *halMeta, IImageBuffer* buff, MINT32 openId, TuningUtils::YUV_PORT type, const char *pUserString);

    void                           initResultParam(const MINT32 &openId, const IMetadata &halMeta, ResultParam *resParm);

private:

    MINT32                                          muDumpBuffer = 0;
    MINT32                                          mEnable = -1;
    MSize                                           mRequestSize = MSize(0, 0);
};
/******************************************************************************
*
******************************************************************************/
};  //namespace NSPipelinePlugin
};  //namespace NSCam
#endif //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_MSNRPLUGIN_H_


