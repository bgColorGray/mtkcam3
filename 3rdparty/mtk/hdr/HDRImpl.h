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

#ifndef  _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_HDRPLUGIN_H_
#define  _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_HDRPLUGIN_H_

#include <stdlib.h>

#include <vector>

#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>

#include <mtkcam/aaa/IHal3A.h>

using namespace NS3Av3;

namespace NSCam {
namespace NSPipelinePlugin{
/******************************************************************************
 *
 ******************************************************************************/
class HDRPluginProviderImp : public MultiFramePlugin::IProvider
{
    typedef MultiFramePlugin::Property Property;
    typedef MultiFramePlugin::Selection Selection;
    typedef MultiFramePlugin::Request::Ptr RequestPtr;
    typedef MultiFramePlugin::RequestCallback::Ptr RequestCallbackPtr;

public:
    HDRPluginProviderImp();

    virtual ~HDRPluginProviderImp();

    virtual const Property& property() override;

    virtual MERROR negotiate(Selection& sel) override;

    virtual void init() override;

    virtual MERROR process(RequestPtr pRequest,
                           RequestCallbackPtr pCallback = nullptr) override;

    virtual void abort(std::vector<RequestPtr>& pRequests) override;

    virtual void uninit() override;

private:
    bool getCaptureParam(const MINT32& sensorId, const MUINT8& requestCount);

    void dumpCaptureParam();

    void dumpBuffer(const IMetadata* pHalMeta,
                    const MINT32 sensorId,
                    IImageBuffer* pIImgBuffer,
                    const char* pUserString = "");

    void writeFeatureDecisionLog(int32_t sensorId,
                                 IMetadata* halInMeta,
                                 const char* log);

private:
    enum {
        YuvDomainHDR = 0,
        RawDomainHDR = 1
    };

    enum ProcessRawType
    {
        eProcessRaw = 0,
        ePureRaw = 1
    };

    enum PackRawType
    {
        eUnpackRaw = 0,
        ePackRaw = 1
    };

    enum BitDepth
    {
        e10Bits = 0,
        e12Bits = 1
    };

    MINT32                          mEnable;
    MINT32                          mAlgoType;
    MINT32                          mProcessRawType;
    MINT32                          mPackRawType;
    MINT32                          mBitDepth;

    MINT32                          mDumpBuffer;

    std::vector<CaptureParam_T>     mvCaptureParam;
    std::vector<RequestPtr>         mvpRequest;
    RequestCallbackPtr              mpCallback;
};
/******************************************************************************
 *
 ******************************************************************************/
};  // namespace NSPipelinePlugin
};  // namespace NSCam

#endif  // _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_HDRPLUGIN_H_
