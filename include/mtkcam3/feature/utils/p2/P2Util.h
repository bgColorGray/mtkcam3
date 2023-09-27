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

#ifndef _MTKCAM_FEATURE_UTILS_P2_UTIL_H_
#define _MTKCAM_FEATURE_UTILS_P2_UTIL_H_

#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
#include <mtkcam/drv/def/ISyncDump.h>
#include <mtkcam/drv/def/mfbcommon_v20.h>
#include <DpDataType.h>

#include <mtkcam3/feature/utils/log/ILogger.h>
#include <mtkcam3/feature/utils/p2/P2Pack.h>
#include <mtkcam3/feature/utils/p2/P2IO.h>
#include <mtkcam3/feature/utils/p2/PQCtrl.h>
#include <mtkcam3/feature/utils/p2/DIPStream.h>

using NSImageio::NSIspio::EPortIndex;

using NSCam::NSIoPipe::PortID;
using NSCam::NSIoPipe::QParams;
using NSCam::NSIoPipe::FrameParams;
using NSCam::NSIoPipe::Input;
using NSCam::NSIoPipe::Output;
using NSCam::NSIoPipe::EPortCapbility;
using NSCam::NSIoPipe::MCropRect;
using NSCam::NSIoPipe::MCrpRsInfo;
using NSCam::NSIoPipe::ExtraParam;
using NSCam::NSIoPipe::NSPostProc::ENormalStreamTag;
using NSCam::NSIoPipe::PQParam;

using NS3Av3::TuningParam;

//#define CROP_IMGO  1
//#define CROP_IMG2O 1
//#define CROP_IMG3O 1
//#define CROP_WDMAO 2
//#define CROP_WROTO 3


namespace NSCam {
namespace Feature {
namespace P2Util {

constexpr const MINT32 INVALID_P2_PROFILE = -1;
constexpr const char* DUMP_CAM_PIPE_PROPERTIES = "vendor.debug.camera.dump.campipe";

enum
{
    CROP_IMGO  = 1,
    CROP_IMG2O = 1,
    CROP_IMG3O = 1,
    CROP_WDMAO = 2,
    CROP_WROTO = 3,
};

class P2ObjPtr
{
public:
    MINT32                          profile = INVALID_P2_PROFILE;
    MBOOL                           hasPQ = MTRUE;
    NSCam::NSIoPipe::P2_RUN_INDEX   run = NSIoPipe::P2_RUN_UNKNOWN;

public:
    _SRZ_SIZE_INFO_                 *srz3 = NULL;
    _SRZ_SIZE_INFO_                 *srz4 = NULL;
    NSCam::NSIoPipe::NSMFB20::MSFConfig *msfConfig = NULL;
    MUINT32                         *timgoParam = NULL;
    NSCam::NSIoPipe::PQParam        *pqParam = NULL;
};

class P2Obj
{
public:
    MINT32                          mProfile = INVALID_P2_PROFILE;
    NSCam::NSIoPipe::P2_RUN_INDEX   mRunIndex = NSIoPipe::P2_RUN_UNKNOWN;
public:
    _SRZ_SIZE_INFO_                 mSRZ3;
    _SRZ_SIZE_INFO_                 mSRZ4;
    NSCam::NSIoPipe::NSMFB20::MSFConfig mMSFConfig;
    MUINT32                         mTimgoParam = NSIoPipe::EDIPTimgoDump_NONE;
private:
    NSCam::NSIoPipe::PQParam        mPQParam;
    DpPqParam                       mWdmaPQ;
    DpPqParam                       mWrotPQ;

public:
    P2Obj()
    {
        mPQParam.WDMAPQParam = NULL;
        mPQParam.WROTPQParam = NULL;
    }
    P2Obj(const P2Obj&) = delete;
    P2Obj& operator=(const P2Obj&) = delete;

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

    P2ObjPtr toPtrTable()
    {
        P2ObjPtr ptr;
        ptr.profile = mProfile;
        ptr.run = mRunIndex;

        ptr.srz3 = &mSRZ3;
        ptr.srz4 = &mSRZ4;
        ptr.msfConfig = &mMSFConfig;
        ptr.timgoParam = &mTimgoParam;
        ptr.pqParam = &mPQParam;
        return ptr;
    }
};

// === p2 readback define: start ===
#define P2RB_BIT_ENABLE       0x1
#define P2RB_BIT_SIM_P1OUTBUF 0x2
#define P2RB_BIT_READ_P2INBUF 0x4
// append bit from here
#define P2RB_BIT_END          0x80000000 // bit-31
// === p2 readback define: end ===

// Camera common function
MVOID initBuffer(IImageBuffer *buf);
MBOOL is4K2K(const MSize &size);
MCropRect getCropRect(const MRectF &rectF);
template <typename T>
MBOOL tryGet(const IMetadata &meta, MUINT32 tag, T &val)
{
    MBOOL ret = MFALSE;
    IMetadata::IEntry entry = meta.entryFor(tag);
    if( !entry.isEmpty() )
    {
        val = entry.itemAt(0, Type2Type<T>());
        ret = MTRUE;
    }
    return ret;
};
template <typename T>
MBOOL tryGet(const IMetadata *meta, MUINT32 tag, T &val)
{
    return (meta != NULL) ? tryGet<T>(*meta, tag, val) : MFALSE;
}
template <typename T>
MBOOL trySet(IMetadata &meta, MUINT32 tag, const T &val)
{
    MBOOL ret = MFALSE;
    IMetadata::IEntry entry(tag);
    entry.push_back(val, Type2Type<T>());
    ret = (meta.update(tag, entry) == android::OK);
    return ret;
}
template <typename T>
MBOOL trySet(IMetadata *meta, MUINT32 tag, const T &val)
{
    return (meta != NULL) ? trySet<T>(*meta, tag, val) : MFALSE;
}
template <typename T>
T getMeta(const IMetadata &meta, MUINT32 tag, const T &val)
{
    T temp;
    return tryGet(meta, tag, temp) ? temp : val;
}
template <typename T>
T getMeta(const IMetadata *meta, MUINT32 tag, const T &val)
{
    T temp;
    return tryGet(meta, tag, temp) ? temp : val;
}

// P2IO function
Output toOutput(const P2IO &io);
Output toOutput(const P2IO &io, MUINT32 index);
MCropRect toMCropRect(const P2IO &io);
MCrpRsInfo toMCrpRsInfo(const P2IO &io);
MVOID applyDMAConstrain(MCropRect &crop, MUINT32 constrain);

// Tuning function
void* allocateRegBuffer(MBOOL zeroInit = MFALSE);
MVOID releaseRegBuffer(void* &buffer);
TuningParam makeTuningParam(const ILog &log, const P2Pack &p2Pack, NS3Av3::IHalISP *halISP, NS3Av3::MetaSet_T &inMetaSet,
                NS3Av3::MetaSet_T *pOutMetaSet, MBOOL resized, void *regBuffer, IImageBuffer *lcso, MINT32 dcesoMagic, IImageBuffer *dceso, MBOOL isSlave, void *syncTuningBuf, IImageBuffer *lcsho);

// Metadata function
MVOID updateExtraMeta(const P2Pack &p2Pack, IMetadata &outHal);
MVOID updateDebugExif(const P2Pack &p2Pack, const IMetadata &inHal, IMetadata &outHal);
MBOOL updateCropRegion(IMetadata &outHal, const MRect &rect);

// DIPParams util function
const char* toName(MUINT32 index);
const char* toName(EPortIndex index);
const char* toName(const PortID &port);
const char* toName(const Input &input);
const char* toName(const Output &output);
MBOOL is(const PortID &port, EPortIndex index);
MBOOL is(const Input &input, EPortIndex index);
MBOOL is(const Output &output, EPortIndex index);
MBOOL is(const PortID &port, const PortID &rhs);
MBOOL is(const Input &input, const PortID &rhs);
MBOOL is(const Output &output, const PortID &rhs);
MBOOL findInput(const DIPFrameParams &frame, const PortID &portID, Input &input);
MBOOL findOutput(const DIPFrameParams &frame, const PortID &portID, Output &output);
IImageBuffer* findInputBuffer(const DIPFrameParams &frame, const PortID &portID);
MVOID printDIPParams(const ILog &log, const DIPParams &params);
MVOID printTuningParam(const ILog &log, const TuningParam &tuning, unsigned index = 0);
MVOID printTuningParam(const TuningParam &tuning);
MVOID printPQCtrl(const char *str, const IPQCtrl_const &pqCtrl, const char* caller);
MVOID printDpPqParam(const char *str, const DpPqParam *pq);

MVOID push_in(DIPFrameParams &frame, const PortID &portID, IImageBuffer *buffer);
MVOID push_in(DIPFrameParams &frame, const PortID &portID, const P2IO &in);
MVOID push_out(DIPFrameParams &frame, const PortID &portID, IImageBuffer *buffer);
MVOID push_out(DIPFrameParams &frame, const PortID &portID, IImageBuffer *buffer, P2IO::TYPE type, MINT32 transform);
MVOID push_out(DIPFrameParams &frame, const PortID &portID, const P2IO &out);
MVOID push_crop(DIPFrameParams &frame, MUINT32 cropID, const MCropRect &crop, const MSize &size);
MVOID push_crop(DIPFrameParams &frame, MUINT32 cropID, const MRectF &crop, const MSize &size, const MUINT32 dmaConstrain = DMAConstrain::DEFAULT);

// DIPParams function
EPortCapbility toCapability(P2IO::TYPE type);
MVOID updateDIPParams(DIPParams &dipParams, const P2Pack &p2Pack, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning);
DIPParams makeDIPParams(const P2Pack &p2Pack, ENormalStreamTag tag, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning);
MVOID updateDIPFrameParams(DIPFrameParams &frame, const P2Pack &p2Pack, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning);
DIPFrameParams makeDIPFrameParams(const P2Pack &p2Pack, ENormalStreamTag tag, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning, MBOOL dumpReg = MFALSE);
DIPFrameParams makeOneDIPFrameParam(const P2Pack &p2Pack, ENormalStreamTag tag, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning, MBOOL dumpReg = MFALSE);

// DParams uitil function
eP2_PQ_PATH toP2PQPath(const MUINT32 type);
DpPqParam makeDpPqParam(const P2Pack &p2Pack, MUINT32 sensorID);
DpPqParam makeDpPqParam(const PQCtrl *pqCtrl, P2IO::TYPE type, IImageBuffer *buffer, const char* caller);

// If expectFps = 0, using ms to set ExpectedEndTime.
// Else using default ExpectedEndTime.
MVOID updateExpectEndTime(DIPParams &dipParams, MUINT32 ms, MUINT32 expectFps = 0);
MVOID updateScenario(DIPParams &dipParams, MUINT32 fps, MUINT32 resolutionWidth);

MVOID push_PQParam(DIPFrameParams &frame, const P2ObjPtr &obj);
MVOID push_PQParam(DIPFrameParams &frame, void *pqParam);

void debug_p2rb_saveToFile(
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    MBOOL bMultiPlane, const string &key, IImageBuffer *pImgBuf);
MVOID readbackBuf(
    const char *pUserName,
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    const MBOOL bReadbackBuf, const MINT32 &magic3A,
    const std::string &key, IImageBuffer *out_pImgBuf);
std::string p2rb_findRawKey(const MINT32 magic3A);

} // namespace P2Util
} // namespace Feature
} // namespace NSCam

#endif // _MTKCAM_FEATURE_UTILS_P2_UTIL_H_
