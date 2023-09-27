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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#ifndef _MTK_CAMERA_CAPTURE_FEATURE_PIPE_YUV_NODE_H_
#define _MTK_CAMERA_CAPTURE_FEATURE_PIPE_YUV_NODE_H_

#include "CaptureFeatureNode.h"

#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>

#include <future>


namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {


class YUVNode : public CaptureFeatureNode
{
public:
    typedef NSCam::NSPipelinePlugin::YuvPlugin YuvPlugin;
    typedef YuvPlugin::IProvider::Ptr ProviderPtr;
    typedef YuvPlugin::IInterface::Ptr InterfacePtr;
    typedef YuvPlugin::Selection Selection;
    typedef YuvPlugin::RequestCallback::Ptr RequestCallbackPtr;

public:
    YUVNode(NodeID_T nid, const char *name, MINT32 policy = SCHED_NORMAL, MINT32 priority = DEFAULT_CAMTHREAD_PRIORITY, MBOOL hasTwinNodes = MFALSE);
    virtual ~YUVNode();
    MVOID setBufferPool(const android::sp<CaptureBufferPool> &pool);
    MVOID getSensorFmt(MINT32& sensorIndex, MINT32& sensorIndex2, MUINT* sensorFmt, MUINT* sensorFmt2);

public:
    typedef NSCam::NSPipelinePlugin::YuvPlugin::Request::Ptr PluginRequestPtr;

    virtual MBOOL onData(DataID id, const RequestPtr& pRequest);
    virtual MERROR evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInference);
    virtual MBOOL evaluateSelection(
                    ProviderPtr pProvider,
                    CaptureFeatureInferenceData& rInfer,
                    Vector<CaptureFeatureInferenceData::SrcData>& rSrcData,
                    Vector<CaptureFeatureInferenceData::DstData>& rDstData,
                    Vector<MetadataID_T>& rMetadatas,
                    Selection& sel,
                    SensorAliasName sensorType);
    virtual RequestPtr findRequest(PluginRequestPtr&);
    virtual MBOOL onRequestRepeat(RequestPtr&);
    virtual MBOOL onRequestProcess(RequestPtr&);
    virtual MVOID onRequestFinish(RequestPtr&);
    virtual MVOID processRequest(RequestPtr& pRequest, sp<CaptureFeatureNodeRequest> pNodeReq, FeatureID_T featureId, ProviderPtr pProvider, SensorAliasName sensorType);
    virtual MVOID onRequestProcessSub(RequestPtr&);
    virtual std::string getStatus(std::string& strDispatch);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:
    class NegotiatedCacher;
    class YuvNodeDataType final
    {
    #define GET_BUF_TYPE(sensorType)    \
        tid_full_yuv        = TID_##sensorType##_FULL_YUV;  \
        tid_full_pure_yuv   = TID_MAN_FULL_PURE_YUV;  \
        tid_depth           = TID_MAN_DEPTH;  \
        tid_dces            = TID_##sensorType##_DCES;  \
        tid_lcs             = TID_##sensorType##_LCS;  \
        tid_lcesho          = TID_##sensorType##_LCESHO;  \
        tid_mss_yuv         = TID_##sensorType##_MSS_YUV;  \
        tid_imgo_rsz_yuv    = TID_MAN_IMGO_RSZ_YUV;  \
        tid_rsz_yuv         = TID_##sensorType##_RSZ_YUV;  \
        tid_fd              = TID_MAN_FD;    \
        mid_in_p1_dynamic   = MID_##sensorType##_IN_P1_DYNAMIC;  \
        mid_in_app          = MID_MAN_IN_APP;    \
        mid_in_hal          = MID_##sensorType##_IN_HAL;    \
        mid_out_app         = MID_##sensorType##_OUT_APP;    \
        mid_out_hal         = MID_MAN_OUT_HAL;    \

    public:
        YuvNodeDataType(SensorAliasName sensorType) {
            if(sensorType == eSAN_Master) {
                GET_BUF_TYPE(MAN);
            } else {
                GET_BUF_TYPE(SUB);
            }
        }
    public:
        TypeID_T      tid_full_yuv;
        TypeID_T      tid_full_pure_yuv;
        TypeID_T      tid_depth;
        TypeID_T      tid_dces;
        TypeID_T      tid_lcs;
        TypeID_T      tid_lcesho;
        TypeID_T      tid_mss_yuv;
        TypeID_T      tid_imgo_rsz_yuv;
        TypeID_T      tid_rsz_yuv;
        TypeID_T      tid_fd;
        MetadataID_T  mid_in_p1_dynamic;
        MetadataID_T  mid_in_app;
        MetadataID_T  mid_in_hal;
        MetadataID_T  mid_out_app;
        MetadataID_T  mid_out_hal;
    };
private:
    typedef std::future<void> InitT;
    typedef std::map<FeatureID_T,InitT> InitMapT;
    typedef std::shared_ptr<NegotiatedCacher> NegotiatedCacherPtr;

    struct RequestPair {
        RequestPtr           mPipe;
        PluginRequestPtr     mPlugin;
    };

    struct ProviderPair {
        ProviderPtr          mpProvider;
        FeatureID_T          mFeatureId;
    };

    android::sp<CaptureBufferPool>              mpBufferPool;

    YuvPlugin::Ptr                              mPlugin;
    InterfacePtr                                mpInterface;
    Vector<ProviderPair>                        mProviderPairs;
    RequestCallbackPtr                          mpCallback;
    InitMapT                                    mInitMap;
    android::BitSet64                           mInitFeatures;

    NegotiatedCacherPtr                         mpNegotiatedCacher;

    WaitQueue<RequestPtr>                       mRequests;
    Vector<RequestPair>                         mRequestPairs;

    ProviderPtr                                 mpCurProvider;

    MBOOL                                       mHasTwinNodes;
    mutable Mutex                               mPairLock;
};

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_CAPTURE_FEATURE_PIPE_YUV_NODE_H_
