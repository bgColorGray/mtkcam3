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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_BOKEH_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_BOKEH_NODE_H_
//
#include "DIPStreamBase.h"
#include "MtkHeader.h"
#include "NullNode.h"

#include <utils/KeyedVector.h>
#include <mtkcam3/feature/stereo/pipe/IBokehPipe.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>
#include <mtkcam3/feature/stereo/hal/stereo_common.h>
//
#include <mtkcam/def/BuiltinTypes.h>
#include <mtkcam/drv/iopipe/PortMap.h>
#include <mtkcam/utils/std/JobQueue.h>
// Dump naming rule
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
//
#include <DpBlitStream.h>
#include <DpIspStream.h>
#include <mtkcam3/feature/core/featurePipe/vsdof/util/vsdof_util.h>
//
#include "StreamingFeatureNode.h"

#include <mtkcam3/feature/utils/p2/DIPStream.h>
#include "ColorizedDepthUtil.h"

#if (1 == VSDOF_SUPPORTED)
#include <blending_hal.h>
#endif

//=======================================================================================
#if SUPPORT_VSDOF
//=======================================================================================

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

using namespace android;
using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam::NSIoPipe;
using namespace StereoHAL;
using namespace NS3Av3;

using _bokehNode_ispTuningConfig_ = struct ISPTuningConfig {
    MINT32           reqNo;
    MBOOL            bInputResizeRaw;

    IMetadata*       pInAppMeta;
    IMetadata*       pInHalMeta;
    IMetadata*       pOutApp;
    IMetadata*       pOutHal;

    NS3Av3::IHalISP* pHalIsp;
    NS3Av3::IHal3A*  p3AHAL;
};

struct BokehOutImgs
{
    struct OutImgs
    {
        IImageBuffer *pP2Out       = nullptr;
        IImageBuffer *pBlendingOut = nullptr;
        IImageBuffer *pMdpOut      = nullptr;
    };
    OutImgs pvBufs;
    OutImgs vrBufs;
    MUINT8 iOutLevel = 0;  // 0 = P2, 1 = Blending, 2 = Mdp
};

//
class BokehEnqueData : public WatchDogData
{
public:
    RequestPtr mRequest;
    BasicImg  mInYuvImg;
    ImgBuffer mDMBG;
    ImgBuffer mDebugImg;
    BasicImg  mOutFullImg;
    BasicImg  mOutNextFullImg;
    NextIO::NextReq mNextReq;
    std::vector<P2IO>  mRemainingOutputs; // Master may Need additional MDP to generate output
    void* cookies = nullptr;
};


class BokehNode
    : public StreamingFeatureNode
    , public DIPStreamBase<BokehEnqueData>
    , public ColorizedDepthUtil
{
public:
    BokehNode(const char *name);
    virtual ~BokehNode();

    MVOID setOutputBufferPool(const android::sp<IBufferPool> &pool, MUINT32 allocate = 0);

public:
    virtual MBOOL onData(DataID id, const DepthImgData &data);
    virtual MBOOL onData(DataID id, const BasicImgData &data);
    virtual NextIO::Policy getIOPolicy(const NextIO::ReqInfo &reqInfo) const;

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

protected:
    virtual MVOID onDIPStreamBaseCB(const Feature::P2Util::DIPParams &params, const BokehEnqueData &request);

private:
    MBOOL prepareBokehInput(Feature::P2Util::DIPFrameParams &frame, const RequestPtr &request,
                            BokehEnqueData &data);
    MBOOL prepareBokehOutputs(Feature::P2Util::DIPFrameParams &frame, const RequestPtr &request,
                              BokehEnqueData &data);
    MBOOL configBokehEnuqeData(Feature::P2Util::DIPFrameParams &frame, const RequestPtr &request,
                              BokehEnqueData &data);
    MVOID prepareFullImg(Feature::P2Util::DIPFrameParams &frame, const RequestPtr &request,
                         BokehEnqueData &data, const FrameInInfo &inInfo);
    MVOID prepareNextFullOut(P2IO &output, const RequestPtr &request,
                                BokehEnqueData &data, const FrameInInfo &inInfo);
    MVOID handleResultData(const RequestPtr &request, const BokehEnqueData &data);
    MVOID enqueFeatureStream(Feature::P2Util::DIPParams &params, BokehEnqueData &data);
    MBOOL needFullForExtraOut(std::vector<P2IO> &outList);

    // TODO : DSDN, add new WaitQueue
    WaitQueue<DepthImgData> mDepthDatas;
    WaitQueue<BasicImgData> mCleanYuvDatas;

    Feature::P2Util::DIPStream *mDIPStream = NULL;
    MUINT32 mFullImgPoolAllocateNeed = 0;
    android::sp<IBufferPool> mFullImgPool;

private:
        //
        MUINT32 mapToBufferID(NSCam::NSIoPipe::PortID const portId,
                                const MUINT32 scenarioID);

        MBOOL setSRZInfo(Feature::P2Util::DIPFrameParams& frameParam, MINT32 modulTag,
                            MSize inputSize, MSize outputSize);

        MVOID outputDepthMap(IImageBuffer* depthMap, IImageBuffer* displayResult);

        MVOID shiftDepthMapValue(IImageBuffer* depthMap, MUINT8 shiftValue);

        IImageBuffer* resizeIntoDebugSize(IImageBuffer* depthMap);

        MVOID dumpBuff(const BokehEnqueData& req, const Feature::P2Util::DIPParams& rParams, BokehOutImgs &bokehOutImgs);

        MBOOL executeMdp(IImageBuffer* input,
                         IImageBuffer* output1,
                         DpPqParam *dpPqParam1,
                         MRectF cropRect1,
                         MUINT32 transform1,
                         IImageBuffer* output2 = nullptr,
                         DpPqParam *dpPqParam2 = nullptr,
                         MRectF cropRect2 = MRectF(0, 0),
                         MUINT32 transform2 = 0);

        MBOOL processOutputBuffer(const Feature::P2Util::DIPParams &params,
                                const BokehEnqueData &data,
                                BokehOutImgs &bokehOutImgs);

        MBOOL createBufferPool( android::sp<ImageBufferPool>& pPool,
                                MUINT32 width,
                                MUINT32 height,
                                NSCam::EImageFormat format,
                                MUINT32 bufCount,
                                const char* caller,
                                MUINT32 bufUsage = ImageBufferPool::USAGE_HW,
                                MBOOL continuesBuffer = MTRUE);

        MBOOL onHandlePostProcess(const Feature::P2Util::DIPParams &params, const BokehEnqueData &data);

        MBOOL executeSwBlending(
                                const BokehEnqueData &data,
                                IImageBuffer *pBokehInBuf,
                                IImageBuffer *pDstPvBuffer,
                                IImageBuffer *pDstVrBuffer = nullptr);

        NS3Av3::TuningParam applyISPTuning(const char* name,
                                           SmartTuningBuffer& targetTuningBuf,
                                           const ISPTuningConfig& ispConfig);

        MBOOL copyBuffer(IImageBuffer* pSrcBuffer, IImageBuffer* pDstBuffer);

        // TODO : apply ALGO's new requirment, depth second overlap on bokeh yuv

        MSize        msDmgi          = -1;
        int          mbPipeLogEnable = 0;
        int          mbShowDepthMap  = 0;
        int          mbDumpImgeBuf   = 0;
        int          mbDumpDIPParam  = 0;
        int          mbDebugColor    = 0;
        int          miDumpBufSize   = 0;
        int          miDumpStartIdx  = 0;
        DpBlitStream *mpDpStream   = nullptr;
        // MDP stream
        DpIspStream *mpDpIspStream = nullptr;

        /**
         * @brief : MDP Feature implemented on :
         * PV : BokehNode
         * VR - EIS ON  : WarpNode
         *    \ EIS OFF - TK path   : BokehNode
         *              \ 3rd Bokeh : WarpNode
         */
        // Job queue for Sw blending
        std::shared_ptr<NSCam::JobQueue<void()>> mJobQueue = nullptr;
        // Apply MDP Feature
        MBOOL mbApplyMDPFeature       = MFALSE;
        // Apply SW Blending
        MBOOL mbApplyBlending         = MFALSE;
        BLENDING_HAL *mpBlendingHal   = nullptr;
        int miTestSwBelnding          = 0;
        // Working scenario
        BokehScenario mWorkingScenario = eBOKEH_PREVIEW;


        NS3Av3::IHalISP *mpIspHal_Main1 = nullptr;
        NS3Av3::IHal3A  *mp3AHal_Main1  = nullptr;

        TuningUtils::FILE_DUMP_NAMING_HINT mDumpHint;

        BokehFlowType mBokehFlowType = eBOKEH_FLOW_TYPE_P2_ONLY;

        android::sp<TuningBufferPool> mDynamicTuningPool    = nullptr;
        android::sp<ImageBufferPool>  mpBokehYUVBufPool     = nullptr;
        android::sp<ImageBufferPool>  mpBlendingBufPool     = nullptr;
        android::sp<ImageBufferPool>  mpDepthMapBufPool     = nullptr;
};

MVOID debugDIPParams(const Feature::P2Util::DIPParams& rInputQParam);

template <typename T>
inline MVOID trySetMetadata( IMetadata* pMetadata, MUINT32 const tag, T const& val);

template <typename T>
inline MBOOL tryGetMetadata(IMetadata* pMetadata, MUINT32 const tag, T & rVal);

template <typename T>
inline MVOID updateEntry( IMetadata* pMetadata, MUINT32 const tag, T const& val);
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

//=======================================================================================
#else //SUPPORT_VSDOF
//=======================================================================================

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
    typedef NullNode BokehNode;
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
//=======================================================================================
#endif //SUPPORT_VSDOF
//=======================================================================================
#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_BOKEH_NODE_H_
