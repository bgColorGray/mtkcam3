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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#ifndef _MTK_CAMERA_CAPTURE_FEATURE_PIPE_P2A_NODE_H_
#define _MTK_CAMERA_CAPTURE_FEATURE_PIPE_P2A_NODE_H_

#include "../../CaptureFeatureNode.h"

#include <featurePipe/vsdof/util/P2Operator.h>
#include <featurePipe/core/include/CamThreadNode.h>
#include <featurePipe/core/include/Timer.h>

#include <mtkcam/aaa/IHalISP.h>
#include <thread>
#include <future>
#include <mtkcam3/feature/utils/ISPProfileMapper.h>
#include <mtkcam/drv/def/mfbcommon_v20.h>

using namespace NS3Av3;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

class P2ANode : public CaptureFeatureNode {
    friend class IEnqueISPWorker;
    friend class EnqueISPWorker_R2R;
    friend class EnqueISPWorker_R2Y;
    friend class EnqueISPWorker_Y2Y;
public:
    P2ANode(NodeID_T nid, const char *name, MINT32 policy = SCHED_NORMAL, MINT32 priority = DEFAULT_CAMTHREAD_PRIORITY);
    virtual ~P2ANode();
    MVOID setBufferPool(const android::sp<CaptureBufferPool>& pool);

public:
    virtual MBOOL onData(DataID id, const RequestPtr& pRequest);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();
    virtual MERROR evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInference);


public:
    struct RequestHolder : public IHolder
    {
        RequestHolder(RequestPtr pRequest)
            : mpRequest(pRequest)
            , mStatus(OK)
        {};

        ~RequestHolder() {};

        Vector<android::sp<IIBuffer>>   mpBuffers;
        RequestPtr                      mpRequest;
        std::shared_ptr<RequestHolder>  mpPrecedeOver;
        MERROR                          mStatus;

        std::vector<std::unique_ptr<IHolder>> mpHolders;
    };

    struct SharedData
    {
        // tuning param
        SmartTuningBuffer imgoDualSynData = NULL;
        SmartTuningBuffer rrzoDualSynData = NULL;
    };

    // image processes
    MBOOL onRequestProcess(NodeID_T nodeId, RequestPtr&);
    // routines
    MVOID onRequestFinish(NodeID_T nodeId, RequestPtr&);


private:
    // request meta update
    MVOID updateFovCropRegion(RequestPtr& pRequest, MINT32 sensorIndex, MetadataID_T metaId);

    inline MBOOL hasSubSensor()
    {
        return mSensorIndex2 >= 0;
    }

    struct RequestPack
    {
        RequestPtr                          mpRequest;
        NodeID_T                            mNodeId = NULL_NODE;
    };

    WaitQueue<RequestPack>                  mRequestPacks;

    struct ISPOperator
    {
        IHalISP*                            mIspHal = NULL;
        sp<P2Operator>                      mP2Opt  = NULL;
    };

    std::map<MINT32, ISPOperator>           mISPOperators;

    android::sp<CaptureBufferPool>          mpBufferPool;
    struct DebugItem
    {
        MINT32 mDebugPerFrame = property_get_int32("vendor.debug.camera.p2c.perframe", 0);
        MINT32 mDebugCZ       = property_get_int32("vendor.debug.camera.cz.enable", -1);
        MINT32 mDebugDRE      = property_get_int32("vendor.debug.camera.dre.enable", -1);
        MINT32 mDebugDCE      = property_get_int32("vendor.debug.camera.dce.enable", -1);
        MINT32 mDebugHFG      = property_get_int32("vendor.debug.camera.hfg.enable", -1);
        DebugItem()
        {
            if(mDebugPerFrame)
            {
                mDebugDRE         = property_get_int32("vendor.debug.camera.dre.enable", -1);
                mDebugCZ          = property_get_int32("vendor.debug.camera.cz.enable", -1);
                mDebugHFG         = property_get_int32("vendor.debug.camera.hfg.enable", -1);
            }
        };
    };
    DebugItem                               mDebugItem;

    struct DumpControl
    {
        MBOOL    mDebugLoadIn     = (property_get_int32("vendor.debug.camera.dumpin.en", 0) == 2);
        MBOOL    mDebugDump       = property_get_int32("vendor.debug.camera.p2.dump", 0) > 0;
        MBOOL    mDumpImg2o       = property_get_int32("vendor.debug.camera.img2o.dump", 0) > 0;
        MBOOL    mDumpImg3o       = property_get_int32("vendor.debug.camera.img3o.dump", 0) > 0;
        MBOOL    mDumpTimgo       = property_get_int32("vendor.debug.camera.timgo.dump", 0) > 0;
        MUINT32  mDumpTimgoType   = property_get_int32("vendor.debug.camera.timgo.type", 0);
        MBOOL    mDumpIMGI        = property_get_int32("vendor.debug.camera.imgi.dump", 0) > 0;
        MBOOL    mDumpLCEI        = property_get_int32("vendor.debug.camera.lcei.dump", 0) > 0;
        MUINT32  mDebugDumpFilter = 0;
        MBOOL    mDebugUnpack     = MFALSE;
        MBOOL    mDumpMFNRYuv     = property_get_int32("vendor.debug.camera.mfnr.yuv.dump", 0) > 0;

    #if MTKCAM_TARGET_BUILD_VARIANT != 'u'
        MBOOL mDebugDumpMDP       = MTRUE;
    #else
        MBOOL mDebugDumpMDP       = property_get_int32("vendor.debug.camera.mdp.dump", 0) > 0;
    #endif
        DumpControl()
        {
            // Enable Dump
            if (mDebugDump)
            {
                mDebugDumpFilter  = property_get_int32("vendor.debug.camera.p2.dump.filter", 0xFFFF);
                mDebugUnpack      = property_get_int32("vendor.debug.camera.upkraw.dump", 0) > 0;
            }
            else
            {
                mDebugDumpFilter    = 0;
                if (mDumpImg3o)
                {
                    mDebugDumpFilter |= DUMP_ROUND1_IMG3O;
                    mDebugDumpFilter |= DUMP_ROUND2_IMG3O;
                    mDebugDump      = MTRUE;
                }
                if (mDumpImg2o)
                {
                    mDebugDumpFilter |= DUMP_ROUND1_IMG2O;
                    mDebugDumpFilter |= DUMP_ROUND2_IMG2O;
                    mDebugDump      = MTRUE;
                }
                if (mDumpTimgo)
                {
                    mDebugDumpFilter |= DUMP_ROUND1_TIMGO;
                    mDebugDumpFilter |= DUMP_ROUND2_TIMGO;
                    mDebugDump      = MTRUE;
                }
                if(mDumpIMGI)
                {
                    mDebugDumpFilter |= DUMP_ROUND1_IMGI;
                    mDebugDumpFilter |= DUMP_ROUND2_IMGI;
                    mDebugDump      = MTRUE;
                }
                if(mDumpLCEI)
                {
                    mDebugDumpFilter |= DUMP_ROUND1_LCEI;
                    mDebugDump      = MTRUE;
                }
            }
        };
    };
    DumpControl                             mDumpControl;

    struct SizeConfig
    {
        // ISP6.0
        MSize                               mDCES_Size         = MSize(0, 0);
        MUINT32                             mDCES_Format       = 0;
        // ISP5.0, ISP6.0
        MUINT32                             mDualSyncInfo_Size = 0;
        // ISP6S
        MUINT32                             mLCE2CALTM_Size    = 0;

        void configSize(NS3Av3::Buffer_Info info)
        {
            if(info.DCESO_Param.bSupport)
            {
                mDCES_Size   = info.DCESO_Param.size;
                mDCES_Format = info.DCESO_Param.format;
                mDualSyncInfo_Size = info.u4DualSyncInfoSize;
            }
            // LCE to CALTM buffer size
            mLCE2CALTM_Size = info.u4LCE2CALTMInfoSize;
        }
    };
    SizeConfig                              mSizeConfig;
    std::weak_ptr<RequestHolder>            mpLastHolder;

    // Postview Delay
    wp<CaptureFeatureRequest>               mwpDelayRequest;
    SmartTuningBuffer                       mpKeepTuningData;
    IMetadata                               mKeepMetaApp;
    IMetadata                               mKeepMetaHal;
    MINT32                                  mLastRequestNo      = -1;
    std::shared_ptr<SharedData>             mDualSharedData;
    ISPProfileMapper*                       mpISPProfileMapper  = nullptr;
};

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_CAPTURE_FEATURE_PIPE_P2A_NODE_H_
