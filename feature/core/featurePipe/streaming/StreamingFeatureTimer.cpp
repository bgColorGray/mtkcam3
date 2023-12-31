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

#include "StreamingFeatureTimer.h"

#include "DebugControl.h"
#define PIPE_CLASS_TAG "Timer"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_TIMER
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

StreamingFeatureTimer::StreamingFeatureTimer()
    : Timer()
    , mDisplayReady(MFALSE)
    , mDisplayMark()
    , mFrameMark()
{
}

MVOID StreamingFeatureTimer::markDisplayDone()
{
    if( !mDisplayReady )
    {
        mDisplayReady = MTRUE;
        mDisplayMark = Timer::getTimeSpec();
    }
}

MVOID StreamingFeatureTimer::markFrameDone()
{
    mFrameMark = Timer::getTimeSpec();
}

timespec StreamingFeatureTimer::getDisplayMark()
{
    return mDisplayMark;
}

timespec StreamingFeatureTimer::getFrameMark()
{
    return mFrameMark;
}

MVOID StreamingFeatureTimer::print(MUINT32 requestNo, MUINT32 recordNo, double displayFPS, double frameFPS, MINT32 minFps, MINT32 maxFps)
{
    MUINT32 total       = getElapsed();
    MUINT32 io          = getElapsedPrepareIO();
    MUINT32 tof         = getElapsedTOF();
    MUINT32 tofEn       = getElapsedEnqueTOF();
    MUINT32 depth       = getElapsedDepth();
    MUINT32 depthEn     = getElapsedEnqueDepth();
    MUINT32 bokeh       = getElapsedBokeh();
    MUINT32 bokehEn     = getElapsedEnqueBokeh();
    MUINT32 mss         = getElapsedMSS();
    MUINT32 mssEn       = getElapsedEnqueMSS();
    MUINT32 pmss        = getElapsedPMSS();
    MUINT32 pmssEn      = getElapsedEnquePMSS();
    MUINT32 msf         = getElapsedMSF();
    MUINT32 msfEn       = getElapsedEnqueMSF();
    MUINT32 p2a         = getElapsedP2A();
    MUINT32 p2aEn       = getElapsedEnqueP2A();
    MUINT32 p2aTun      = getElapsedP2ATuning();
    MUINT32 p2sw        = getElapsedP2SW();
    MUINT32 p2swTun     = getElapsedP2SWTuning();
    MUINT32 vnr         = getElapsedVNR();
    MUINT32 vnrEn       = getElapsedVNREnque();
    MUINT32 vnrEnS      = getElapsedVNREnque_S();
    MUINT32 p2aMDP      = getElapsedP2AMDP();
    MUINT32 p2sm        = getElapsedP2SM();
    MUINT32 p2smEn      = getElapsedP2SMEnque();
    MUINT32 p2smTun     = getElapsedP2SMTuning();
    MUINT32 eis         = getElapsedEIS();
    MUINT32 helper      = getElapsedHelper();
    MUINT32 helperMDP   = getElapsedHelperMDP();
    MUINT32 tpiMeta     = getElapsedTPIMeta();
    MUINT32 async       = getElapsedAsync();
    MUINT32 asyncEn     = getElapsedEnqueAsync();
    MUINT32 disp        = getElapsedTPIDisp();
    MUINT32 dispEn      = getElapsedEnqueTPIDisp();
    MUINT32 rsc         = getElapsedRSC();
    MUINT32 rscEn       = getElapsedEnqueRSC();
    MUINT32 xNode       = getElapsedXNode();

    const MUINT TPI_TIMER_COUNT = 3;
    MUINT32 tpi[TPI_TIMER_COUNT] = { 0 };
    MUINT32 tpiEn[TPI_TIMER_COUNT] = { 0 };
    MUINT32 tpiCount    = std::min(TPI_TIMER_COUNT, MAX_TPI_COUNT);
    for(unsigned i = 0; i < tpiCount; ++i )
    {
        tpiEn[i] = getElapsedEnqueTPI(i);
        tpi[i] = getElapsedTPI(i);
    }

    MUINT32 warp[MAX_WARP_NODE_COUNT] = { 0 };
    MUINT32 warpWait[MAX_WARP_NODE_COUNT] = { 0 };
    MUINT32 warpEn[MAX_WARP_NODE_COUNT] = { 0 };
    MUINT32 warpMDP[MAX_WARP_NODE_COUNT] = { 0 };
    for(unsigned i = 0; i < MAX_WARP_NODE_COUNT; ++i )
    {
        warp[i] = getElapsedWarp(i);
        warpWait[i] = getElapsedWarpWait(i);
        warpEn[i] = getElapsedEnqueWarp(i);
        warpMDP[i] = getElapsedWarpMDP(i);
    }

    MUINT32 vmdp[MAX_VMDP_NODE_COUNT] = { 0 };
    MUINT32 vmdpWait[MAX_VMDP_NODE_COUNT] = { 0 };
    for(unsigned i = 0; i < MAX_VMDP_NODE_COUNT; ++i )
    {
        vmdp[i] = getElapsedVmdp(i);
        vmdpWait[i] = getElapsedVmdpWait(i);
    }

    MY_LOGD("Frame timer [#%5d/%4d][t%4d][io%4d][tof%3d/%3d][d%3d/%3d][mss%3d/%3d/%3d/%3d][a%3d/%3d/%3d][tun%3d/%3d][msf%3d/%3d][vnr%3d/%3d/%3d][am%3d][b%3d/%3d]"
            "[tpi(%3d/%3d)/(%3d/%3d)/(%3d/%3d)][vmdp_A%3d/%3d vmdp_B%3d/%3d][h%3d/%3d/%3d][e%3d][r%3d/%3d][w_A%3d/%3d/%3d/%3d w_B%3d/%3d/%3d/%3d]"
            "[x%3d][disp%3d/%3d][async%3d/%3d][p2sm%3d/%3d/%3d][fps%.2f/%.2f/%3d/%3d]",
            requestNo, recordNo, total, io, tof, tofEn, depth, depthEn, mss, mssEn, pmss, pmssEn, p2a, p2aTun, p2aEn, p2sw, p2swTun, msf, msfEn, vnr, vnrEn, vnrEnS, p2aMDP, bokeh, bokehEn, tpi[0], tpiEn[0], tpi[1], tpiEn[1], tpi[2], tpiEn[2], vmdp[0], vmdpWait[0], vmdp[1], vmdpWait[1], helper, tpiMeta, helperMDP, eis, rsc, rscEn, warp[0], warpWait[0], warpEn[0], warpMDP[0], warp[1], warpWait[1], warpEn[1], warpMDP[1], xNode, disp, dispEn, async, asyncEn, p2sm, p2smTun, p2smEn, displayFPS, frameFPS, minFps, maxFps);

#if defined(DEBUG_TIMER) && (DEBUT_TIMER == 1)
    {
        MUINT32 t1 = getElapsedT1();
        MUINT32 t2 = getElapsedT2();
        MUINT32 t3 = getElapsedT3();
        MUINT32 t4 = getElapsedT4();
        MY_LOGD("Frame [t1%5d][t2%5d][t3%5d][t4%5d]",
                requestNo, t1, t2, t3, t4);
    }
#endif
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
