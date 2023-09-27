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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_TIMER_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_TIMER_H_

#include <featurePipe/core/include/Timer.h>
#include "DebugControl.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class StreamingFeatureTimer : public Timer
{
public:
    ADD_TIMER(PrepareIO);
    ADD_TIMER(TOF);
    ADD_TIMER(EnqueTOF);
    ADD_TIMER(Depth);
    ADD_TIMER(EnqueDepth);
    ADD_TIMER(P2A);
    ADD_TIMER(EnqueP2A);
    ADD_TIMER(P2ATuning);
    ADD_TIMER(P2SW);
    ADD_TIMER(P2SWTuning);
    ADD_TIMER(P2AMDP);
    ADD_TIMER(P2SM);
    ADD_TIMER(P2SMTuning);
    ADD_TIMER(P2SMEnque);
    ADD_TIMER(VNR);
    ADD_TIMER(VNREnque);
    ADD_TIMER(VNREnque_S);
    ADD_TIMER(Bokeh);
    ADD_TIMER(EnqueBokeh);
    ADD_TIMER(EIS);
    ADD_TIMER(Helper);
    ADD_TIMER(HelperMDP);
    ADD_TIMER(TPIMeta);
    ADD_TIMER_LIST(TPI, MAX_TPI_COUNT);
    ADD_TIMER_LIST(EnqueTPI, MAX_TPI_COUNT);
    ADD_TIMER_LIST(Vmdp, MAX_VMDP_NODE_COUNT);
    ADD_TIMER_LIST(VmdpWait, MAX_VMDP_NODE_COUNT);
    ADD_TIMER(TPIDisp);
    ADD_TIMER(EnqueTPIDisp);
    ADD_TIMER(Async);
    ADD_TIMER(EnqueAsync);
    ADD_TIMER(RSC);
    ADD_TIMER(EnqueRSC);
    ADD_TIMER(MSS);
    ADD_TIMER(EnqueMSS);
    ADD_TIMER(PMSS);
    ADD_TIMER(EnquePMSS);
    ADD_TIMER(MSF);
    ADD_TIMER(EnqueMSF);
    ADD_TIMER_LIST(Warp, MAX_WARP_NODE_COUNT);
    ADD_TIMER_LIST(WarpWait, MAX_WARP_NODE_COUNT);
    ADD_TIMER_LIST(EnqueWarp, MAX_WARP_NODE_COUNT);
    ADD_TIMER_LIST(WarpMDP, MAX_WARP_NODE_COUNT);
    ADD_TIMER(N3DP2);
    ADD_TIMER(N3D);
    ADD_TIMER(XNode);
    ADD_TIMER(Tracking);

#if defined(DEBUG_TIMER) && (DEBUG_TIMER == 1)
    ADD_TIMER(T1);
    ADD_TIMER(T2);
    ADD_TIMER(T3);
    ADD_TIMER(T4);
#endif

    StreamingFeatureTimer();
    MVOID markDisplayDone();
    MVOID markFrameDone();
    timespec getDisplayMark();
    timespec getFrameMark();
    MVOID print(MUINT32 requestNo, MUINT32 recordNo, double displayFPS = 0, double frameFPS = 0, MINT32 minFps = 0, MINT32 maxFps = 0);

private:
    MBOOL mDisplayReady;
    timespec mDisplayMark;
    timespec mFrameMark;

};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_TIMER_H_
