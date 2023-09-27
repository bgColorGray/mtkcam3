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
 * MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_PRODUCER_WARP_NODE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_PRODUCER_WARP_NODE_H_

#include "StreamingFeatureNode.h"
#if (SUPPORT_TRACKING == 1)
#include <mtkcam/utils/sys/SensorProvider.h>
#include <mtkcam/utils/hw/IGenericContainer.h>
#include <mtkcam/aaa/aaa_hal_common.h>
#include <mtkcam/utils/hw/HwTransform.h>
#include <OThal.h>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

struct TrackingResult
{
    MRectF mTrackingROI;
    MRectF mInitROI;
    std::vector<MRectF> mMultiTrackingROI;
    std::vector<MRectF> mCandiTrackingROI;
    std::vector<MFLOAT> mTrustValueList;
};

struct TrackingImage
{
    int w                    = 0;
    int h                    = 0;
    MUINT8* AddrY            = nullptr;
    MUINT8* AddrU            = nullptr;
    MUINT8* AddrV            = nullptr;
    MUINT8* PAddrY           = nullptr;
    MINT32 format            = eImgFmt_UNKNOWN;
    MINT32 planes            = 0;
    sp<IImageBuffer> pImg    = nullptr;
    MINT64 mTimeStamp        = 0;
    MINT32 mSensorRotate     = 0;
    MBOOL mAIODInitial       = MFALSE;
    MINT32 mTargetObjectType = -1;

    // AIOD customization
    MBOOL mNeedToKeepImg = MFALSE;
    MBOOL mReplaceImg = MFALSE;

    TrackingImage(){};
};

struct NormalizedROI
{
    float x_min = 0;
    float y_min = 0;
    float x_max = 0;
    float y_max = 0;
};

class TrackingNode;
class AITracking
{
    friend TrackingNode;
public:
    AITracking();
    ~AITracking();
    MVOID init(const TrackingImage &trackingImage, MUINT32 sensorId);
    MVOID updateTrustValTh(MFLOAT th) {mTrustValueTh = th;}
    MBOOL process(const TrackingImage &dupImage,
                  TrackingResult& result,
                  MBOOL resetAITracking=false);
    MBOOL touchTrigger(const MRectF &initROI);
    MBOOL prepareInput(const TrackingImage &imgi, const MRectF &initROI);
    MBOOL fillResult(TrackingResult& result);
    MBOOL isValidOut();
    MBOOL doObjectDetection(TrackingImage &trackingImage,
                            MRectF &initROI,
                            std::vector<AIOD_CATEGORY_ENUM> type,
                            MBOOL forceOutputDefault=MTRUE,
                            MBOOL useTouchTarget=MFALSE);
    MINT32 getDetectionSt();
    MVOID resetDetectionSt();

public:
    MINT32                          mTouchTriggerHint = MFALSE;
private:
    MUINT32                         i4SensorID  = -1;
    std::shared_ptr<OThal>          pAIOTProc   = nullptr;
    MBOOL                           AFTrigger   = MFALSE;
    MBOOL                           mStartTracking = MFALSE;
    TrackingInitIn                  gInitInfo;
    DectectionInitIn                gInitInfoD;
    ObjectTrackingIn                gProcIn;
    ObjectDectectionIn              gProcInD;
    ObjectTrackingOut               gResultInfo;
    ObjectDectectionOut             gResultInfod;
    AIOT_RESULT_INFO_STRUCT         gResultInfo_init;
    struct NormalizedROI            mNormalizedROI;
    MINT32                          delayedFrame;
    MBOOL                           mDebugLog      = property_get_bool("vendor.debug.TrackingNodeLog", MFALSE);
    MBOOL                           useAIODInitial = property_get_bool("vendor.debug.tracking.useAIODInitial", false);
    MINT32                          mDetectionSt = S_AIOD_OK;
    MRectF                          mBackupInitROI;
    MFLOAT                          mTrustValueTh = 0.0;

};

/*******************************************************************************
*
********************************************************************************/
#define MAX_DUMP_BUFFER 300
class TrackingNode : public virtual StreamingFeatureNode
{
public:
    TrackingNode(const char *name);
    virtual ~TrackingNode();
    virtual MBOOL onData(DataID id, const RequestPtr &data);

protected:
    virtual MBOOL onInit();
    virtual MBOOL onUninit();
    virtual MBOOL onThreadStart();
    virtual MBOOL onThreadStop();
    virtual MBOOL onThreadLoop();

private:
    MBOOL makeTrackingImage(const RequestPtr &request);
    MBOOL keepTrackingImage(const TrackingImage& img);
    MBOOL runAdditionalDetection(ITrackingData &resultROI);
    MBOOL getInitROI(const RequestPtr &request, ITrackingData &resultROI);
    MBOOL calcROI(const RequestPtr &request, ITrackingData &resultROI, MBOOL resetTracking=MFALSE);
    MVOID fillResultToROIdata(ITrackingData &resultROI, TrackingResult& result);
    MINT32 calculateRotateDegree();
    MBOOL calculateP1FDCrop(const RequestPtr &request, MRect& UIcropRegion);
    MVOID convertTrackingMeta(const RequestPtr &request, MRectF resultROI, MBOOL needToUpdateMeta = MFALSE);
    MVOID dumpBuffer(IImageBuffer* imgi, MUINT32 requestNo, MBOOL templateImage=MFALSE);

private:
    MBOOL                           mEnableDebugTouch = property_get_bool("vendor.debug.enableDebugTouch", MFALSE);
    WaitQueue<RequestPtr>           mAITrackingYuvIn;
    TrackingImage                   mTrackingImage;
    TrackingImage                   mKeptImage;
    IImageBufferAllocator*          mAllocator;
    MBOOL                           mDebugLog         = MFALSE;
    // AI tracking
    AITracking                      mAITracking;
    // calc sensor rot angle
    android::sp<Utils::SensorProvider> mpSensorProvider;
    // acceleration in image coordinate
    float mAngle                                      = 0.0;
    MINT32                          mSensorRot        = 0;
    MUINT32                         mDumpFDIndex      = 0;

    // for TrackingFocus
    std::unordered_map<MUINT32, NS3Av3::IHal3A*>   mvp3AHal;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

# else
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
    typedef NullNode TrackingNode;
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
# endif   // support AI Tracking
#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_PRODUCER_WARP_NODE_H_