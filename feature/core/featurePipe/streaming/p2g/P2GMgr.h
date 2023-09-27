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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_MGR_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_MGR_H_

#include <algorithm>
#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam/aaa/IIspMgr.h>
#include <mtkcam3/feature/utils/p2/IImageInfo.h>

#include "core/P2GDefine.h"
#include "core/PathEngine.h"
#include "P2HWUtil.h"
#include "P2SWUtil.h"
#include "GImg.h"

#include "../StreamingFeatureData.h"
#include "../FrameControl.h"
#include "../DcesoHal.h"
#include "../DreHal.h"
#include "../DsdnHal.h"
#include "../SMVRHal.h"
#include "../TimgoHal.h"
#include <mtkcam/utils/sys/SensorProvider.h>
#include <common/3dnr/3dnr_hal_base.h>
#include <mtkcam3/feature/3dnr/3dnr_defs.h>
using NSCam::Utils::GyroMVResult;
using NSCam::Utils::SensorProvider;
using NSCam::Utils::SENSOR_TYPE_GYRO;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

namespace P2G {

class P2GMgr
{
public:
    using DIPParams = ::NSCam::Feature::P2Util::DIPParams;

public:
    P2GMgr();
    ~P2GMgr();
    MBOOL init(const StreamingFeaturePipeUsage &usage);
    MBOOL uninit();

    MVOID allocateP2GInBuffer(MBOOL delay = MFALSE);
    MVOID allocateP2GOutBuffer(MBOOL delay = MFALSE);
    MVOID allocateP2SWBuffer(MBOOL delay = MFALSE);
    MVOID allocateP2HWBuffer(MBOOL delay = MFALSE);
    MVOID allocateDsBuffer(MBOOL delay = MFALSE);
    MVOID allocatePDsBuffer(MBOOL delay = MFALSE);

    MBOOL isOneP2G() const;
    MBOOL needPrint() const;
    MBOOL isSupport(Feature::FID) const;
    MBOOL isSupportDCE() const;
    MBOOL isSupportDRETun() const;
    MBOOL isSupportTIMGO() const;

    const char* getTimgoTypeStr() const;
    P2F::ImageInfo getFullImageInfo() const;

    TunBuffer requestSyncTuningBuffer();
    ImgBuffer requestFullImgBuffer();
    ImgBuffer requestDsBuffer(unsigned index);
    ImgBuffer requestDnBuffer(unsigned index);

    std::vector<IO> replaceLoopIn(const std::vector<IO> &ioList);
    Path calcPath(const std::vector<IO> &ioList, MUINT32 reqNo = 0);
    DIPParams runP2HW(const std::vector<P2HW> &p2hw);
    MVOID runP2SW(const std::vector<P2SW> &p2sw, P2SW::TuningType type = P2SW::P2);

    MVOID waitFrameDepDone(MUINT32 requestNo);
    MVOID notifyFrameDone(MUINT32 requestNo);

    IO::LoopData makeInitLoopData(const Feature &feature);
    POD makePOD(const RequestPtr &request, MUINT32 sensorIndex, Scene scene);

    ILazy<NR3DMotion> makeMotion() const;
    MBOOL runUpdateMotion(const RequestPtr &request, const P2SWHolder &sw, const RSCResult &rsc, MBOOL bSlave = MFALSE);

    ILazy<DSDNInfo> makeDSDNInfo() const;
    struct DsdnQueryOut
    {
        std::vector<MSize> dsSizes;
        std::vector<MSize> p2Sizes;
        P2G::ILazy<DSDNInfo> outInfo;
        P2G::ILazy<P2G::GImg> omc;
        MSize confSize;
        MSize idiSize;
    };
    DsdnQueryOut queryDSDNInfos(MUINT32 sensorID, const RequestPtr &request, const MSize &next = MSize(0,0), MBOOL lastRun = MFALSE);

    SMVRHal getSMVRHal() const;

private:
    MVOID initSensorProvider();
    MVOID uninitSensorProvider();
    MVOID initHalISP();
    MVOID uninitHalISP();
    MVOID initDcesoHal();
    MVOID uninitDcesoHal();
    MVOID initDreHal();
    MVOID uninitDreHal();
    MVOID initTimgoHal();
    MVOID uninitTimgoHal();
    MVOID init3DNR();
    MVOID uninit3DNR();
    Hal3dnrBase *get3dnrHalInstance(MUINT32 sensorID);
    MVOID initDsdnHal();
    MVOID initDsdnPools();
    MVOID uninitDsdnHal();
    MVOID initSMVR();
    MVOID uninitSMVR();

    static MVOID uninitDsdnPools(DataPool& pool);

    MVOID initPool();
    MVOID uninitPool();
    MVOID initBufferNum();
    MVOID initHwOptimize();
    MVOID allocateGyroBuffer();
    MVOID freeGyroBuffer();
    android::sp<TuningBufferPool> createTuningPool(const char *name);
    android::sp<TuningBufferPool> createSyncTuningPool(const char *name);
    android::sp<IBufferPool> createFullImgPool(const char *name);
    android::sp<IBufferPool> createDcesoPool(const char *name);
    android::sp<TuningBufferPool> createDreTuningPool(const char *name);
    android::sp<IBufferPool> createTimgoPool(const char *name);
    android::sp<IBufferPool> createConfPool(const char *name, const std::vector<MSize> &dsSizes);
    android::sp<IBufferPool> createIdiPool(const char *name, const std::vector<MSize> &dsSizes);
    android::sp<IBufferPool> createOmcPool(const char *name, const std::vector<MSize> &dsSizes);
    std::vector<LazyImgBufferPool> createMultiPools(const char *name, const std::vector<MSize> &sizes, const EImageFormat &fmt);
    android::sp<IBufferPool> createFatPool(const char *name, const MSize &size, const EImageFormat &fmt) const;

private:
    StreamingFeaturePipeUsage mPipeUsage;
    MUINT32 mSensorIndex = (MUINT32)(-1);
    DataPool mPool;

    MUINT32 mTuningByteSize = 0;
    MUINT32 mSyncTuningByteSize = 0;
    MUINT32 mDreTuningByteSize = 0;
    MUINT32 mMsfTuningByteSize = 0;
    MUINT32 mMsfSramByteSize = 0;
    MUINT32 mOmcTuningByteSize = 0;

    std::shared_ptr<SensorContextMap> mSensorContextMap;
    NS3Av3::IIspMgr *mIspMgr = NULL;
    NS3Av3::Buffer_Info mISPBufferInfo;
    MBOOL mHasISPBufferInfo = MFALSE;

    DcesoHal mDcesoHal;
    DreHal mDreHal;
    TimgoHal mTimgoHal;
    SMVRHal mSMVRHal;
    DsdnHal mDsdnHal;

    PathEngine mPathEngine;
    P2SWUtil mP2SWUtil;
    P2HWUtil mP2HWUtil;

    FrameControl mFrameControl;

    Feature mFeatureSet;

    MUINT32 mP2HWOptimizeLevel = 0;
    MBOOL   mNoFat = MFALSE;
    MBOOL   mNeedPrint = MFALSE;
    MINT32  mForceThreadRelease = 0;

    Hal3dnrBase *mp3dnr[HAL_3DNR_INST_MAX_NUM] = { NULL };

    android::sp<SensorProvider> mpSensorProvider = NULL;
    MUINT32 intervalInMs = 3;
    GyroMVResult mGyroMVResult;
};

PMDPReq makePMDPReq(const std::vector<PMDP> &list);

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_MGR_H_
