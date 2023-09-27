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

#ifndef _MTK_CAMERA_WPE_WARP_STREAM_H_
#define _MTK_CAMERA_WPE_WARP_STREAM_H_

#include "WarpStream.h"
#include "Dummy_WPEWarpStream.h"
#include "DIPStreamBase.h"
#include "WarpBase.h"
#include "WPEWarp.h"

#include <mtkcam/drv/iopipe/PostProc/IHalWpePipe.h>
#include <mtkcam3/feature/utils/p2/DIPStream.h>

typedef  NSCam::NSIoPipe::NSWpe::WPEQParams WPEQParams;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class WPEObj
{
public:
    WPEQParams mWPEQParam;
};

class PQObj
{
public:
    MVOID setWdmaPQ(const DpPqParam &pq)
    {
        mWdmaPQ = pq;
        mPQParam.WDMAPQParam = &mWdmaPQ;
    }
    MVOID setWrotPQ(const DpPqParam &pq)
    {
        mWrotPQ = pq;
        mPQParam.WROTPQParam = &mWrotPQ;
    }
    void* getPQParamPtr() const
    {
        return (void*)&mPQParam;
    }

private:
    PQParam mPQParam;
    DpPqParam mWdmaPQ;
    DpPqParam mWrotPQ;
};

class WPEEnqueData
{
public:
    WPEEnqueData() {}
    WPEEnqueData(const WarpParam &warpParam) : mWarpParam(warpParam) {}
public:
    WarpParam mWarpParam;
    std::shared_ptr<WPEObj> mWPEObj;
    std::list<std::shared_ptr<PQObj>> mPQObjList;
};

class WPEWarpStream : public virtual WarpStream , public virtual DIPStreamBase<WPEEnqueData>
{
private:
    using DIPParams = Feature::P2Util::DIPParams;
    using DIPFrameParams = Feature::P2Util::DIPFrameParams;

public:
    static WPEWarpStream* createInstance();

public:
    WPEWarpStream();
    virtual ~WPEWarpStream();
    MBOOL init(const MUINT32 &sensorIdx, const MUINT32 &timerIdx, const MSize &maxImageSize, const MSize &maxWarpSize);
    MVOID uninit();
    MBOOL enque(WarpParam param);

private:
    MVOID prepareDIPParams(DIPParams &param, WPEEnqueData &data);
    MVOID enqueNormalStream(const DIPParams &param, const WPEEnqueData &data);
    virtual MVOID onDIPStreamBaseCB(const DIPParams &param, const WPEEnqueData &data);

private:
    MVOID checkMDPOut(WPEEnqueData &data);
    MVOID setBasic(DIPFrameParams &fparam, const WarpParam &warpParam);
    MVOID setInBuffer(DIPFrameParams &fparam, const WarpParam &warpParam);
    MVOID setStandaloneOutput(DIPFrameParams &fparam, const WarpParam &warpParam);
    MVOID adjustCropSetting(WarpParam &warpParam);
    MBOOL pushIO(DIPFrameParams &frame, const P2IO &io, const PortID &portID, MUINT32 cropID);
    MBOOL pushPQ(DIPFrameParams &frame, WPEEnqueData &data, const std::shared_ptr<PQObj> &pqObj, const P2IO &wdmao, const P2IO &wroto);
    MVOID setWPEDIPParams(DIPFrameParams &fparam, WPEQParams &wpeQParam, const WarpParam &warpParam);

    MVOID setWPEMode(WPEQParams &wpeQParam);
    MVOID setWPECrop(WPEQParams &wpeQParam, const WarpParam &warpParam);
    MVOID setWPEBuffer(WPEQParams &wpeQParam, const WarpParam &warpParam);

    static MSize toWPEOutSize(const WarpParam &param);
    static MRectF toWPEScaleCrop(const MSize &wpeOutSize, const MRectF &srcCrop, MBOOL print);

private:
    Feature::P2Util::DIPStream *mDIPStream = NULL;

private:
    class ProcessThread : public android::Thread
    {
    public:
        ProcessThread(const MUINT32 &timerIdx);
        virtual ~ProcessThread();
        MVOID threadEnque(const WarpParam &param);
        MVOID signalStop();

    public:
        android::status_t readyToRun();
        bool threadLoop();

    private:
        MBOOL waitParam(WarpParam &param);
        MVOID processParam(WarpParam param);

    private:
        std::queue<WarpParam> mQueue;
        android::Mutex mThreadMutex;
        android::Condition mThreadCondition;
        MBOOL mStop;
        MDPWrapper mMDP;
        MUINT32 mTimerIdx = 0;
    };

    android::sp<ProcessThread> mProcessThread;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_WPE_WARP_STREAM_H_
