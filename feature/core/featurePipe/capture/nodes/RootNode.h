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

#ifndef _MTK_CAMERA_CAPTURE_FEATURE_PIPE_ROOT_NODE_H_
#define _MTK_CAMERA_CAPTURE_FEATURE_PIPE_ROOT_NODE_H_

#include "CaptureFeatureNode.h"
#include <semaphore.h>
#include <utils/KeyedVector.h>

#ifdef SUPPORT_MFNR
// ALGORITHM
#include <MTKBss.h>
#include <mtkcam/utils/hw/IFDContainer.h>
#endif

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NSCapture {

class RootNode : public CaptureFeatureNode
{
#ifdef SUPPORT_MFNR
    /*
     *  Tuning Param for EIS.
     *  Should not be configure by customer
     */
    static const int MFC_GMV_CONFX_TH = 25;
    static const int MFC_GMV_CONFY_TH = 25;
    static const int MAX_GMV_CNT = MAX_FRAME_NUM;

    struct GMV{
        MINT32 x = 0;
        MINT32 y = 0;
    };
#endif
public:
    RootNode(NodeID_T nid, const char *name, MINT32 policy = SCHED_NORMAL, MINT32 priority = DEFAULT_CAMTHREAD_PRIORITY);
    virtual ~RootNode();

public:
    virtual MBOOL onData(DataID id, const RequestPtr& pRequest);
    virtual MBOOL onAbort(RequestPtr &data);
    virtual MERROR evaluate(NodeID_T nodeId, CaptureFeatureInferenceData& rInference);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadLoop();
    virtual MBOOL onThreadStop();

    MBOOL onRequestProcess(RequestPtr&);
    MVOID onRequestFinish(const RequestPtr&);
private:

#ifdef SUPPORT_MFNR
    MBOOL doBss(Vector<RequestPtr>& rvToDoRequests);
    // sync golden frame metadata to all the subframe
    MVOID syncGoldenFrameMeta(Vector<RequestPtr>& rvOrderedRequests, size_t i4SkipFrmCnt, MetadataID_T metaId);
    // update the metadata which need to be in-order after crossing
    MVOID updateInOrderMetadata(RequestPtr pGoldenRequest, RequestPtr pFirstPipelineRequest);
    MVOID updateFaceData(RequestPtr pGoldenRequest, RequestPtr pFirstPipelineRequest);
#endif
    MVOID reorder(Vector<RequestPtr>& rvRequests, Vector<RequestPtr>& rvOrderedRequests, size_t i4SkipFrmCnt);

private:

    mutable Mutex                       mLock;
    mutable Condition                   mWaitCondition;
    bool                                mbWait = false;
    WaitQueue<RequestPtr>               mRequests;
    Vector<RequestPtr>                  mvPendingRequests;
    Vector<RequestPtr>                  mvRestoredRequests;
    Vector<RequestPtr>                  mvBypassBSSRequest;

    class FDContainerWraper;
    UniquePtr<FDContainerWraper>        mFDContainerWraperPtr;

    // logica MFNR + physical slave
    //std::vector<RequestPtr>             mPhysicPendingRequests;
    std::pair<int, int>                 mGoldenInfo;  // (frame index, frame number)
    struct BssWaitingList {
        std::vector<RequestPtr>         mPhysicPendingRequests;
        std::future<void>               mGetPhysicalBss;
        std::shared_ptr<sem_t>          mWaitForBss = nullptr;
    };

    std::map<MINT32, BssWaitingList>    mPhysicWaitingMap;

    mutable Mutex                       mNvramChunkLock;
    MINT32                              mDebugLevel;
    MINT32                              mDebugDump;
    MINT32                              mEnableBSSOrdering;
    MINT32                              mDebugDrop;
    MINT32                              mDebugLoadIn;
};

} // NSCapture
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_CAPTURE_FEATURE_PIPE_ROOT_NODE_H_
