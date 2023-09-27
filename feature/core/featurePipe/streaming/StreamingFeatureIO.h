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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_IO_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_IO_H_


#include <featurePipe/core/include/IOUtil.h>
#include <mtkcam3/feature/utils/p2/EIGHTCC.h>

using NSCam::P2F::EIGHTCC;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class StreamingFeatureNode;

namespace NextIO {

enum InP1Src
{
    IN_UNKNOWN,
    IN_RRZO,
    IN_IMGO,
};

enum InMetaType
{
    IN_META_UNKNOWN,
    IN_META_APP,
    IN_META_P1_APP,
    IN_META_P1_HAL,
};

enum OutType
{
    OUT_NONE,
    OUT_UNKNOWN,
    OUT_PREVIEW,
    OUT_RECORD,
    OUT_EXTRA,
    OUT_PHY,
};

enum PQType
{
    PQ_NONE,
    PQ_PREVIEW,
    PQ_RECORD,
};

enum NextPathID {
    N_PATH_DEFAULT,
    N_PATH_P,
    N_PATH_R,
    N_PATH_TINY,
};

enum TargetPathID {
    N_TARGET_DEFAULT,
    N_TARGET_P,
    N_TARGET_R,
    N_TARGET_TINY,
};

class InInfo
{
public:
    InInfo();
    InInfo(InP1Src p1Src, const MSize &size, const MRect &crop);
    operator bool() const;
public:
    InP1Src mP1Src = IN_UNKNOWN;
    MSize mSize;
    MRect mSensorCrop;
};

class OutInfo
{
public:
    OutInfo();
    OutInfo(OutType outType, const MSize &size, const MRectF &crop);
    operator bool() const;
public:
    OutType mType = OUT_UNKNOWN;
    MSize mSize;
    MRectF mCrop;
};

class ReqInfo
{
public:
    MUINT32 mFrameNo = 0;
    MUINT32 mFeatureMask = 0;
    MUINT32 mMasterID = INVALID_SENSOR;
    MUINT32 mSlaveID = INVALID_SENSOR;
    std::list<MUINT32> mSensorIDs;
    std::map<MUINT32, std::map<InP1Src, InInfo>> mInInfoMap;
    std::map<MUINT32, std::map<InMetaType, IMetadata*>> mMetaMap;
    std::map<MUINT32, std::list<OutInfo>> mOutInfoMap;

public:
    ReqInfo(MUINT32 frame, MUINT32 mask, MUINT32 master, MUINT32 slave);
    MBOOL hasMaster() const;
    MBOOL hasSlave() const;
    InInfo getInInfo(InP1Src p1Src, MUINT32 sensor = INVALID_SENSOR) const;
    OutInfo getOutInfo(OutType outType, MUINT32 sensor = INVALID_SENSOR) const;
    IMetadata* getInMeta(InMetaType metaType, MUINT32 sensor = INVALID_SENSOR) const;
};

class NextID
{
public:
    NextID(const EIGHTCC &name, InP1Src p1Src = IN_RRZO, MUINT32 batchIndex = 0, NextPathID path = N_PATH_DEFAULT);
    bool operator==(const NextID &rhs) const;
    bool operator<(const NextID &rhs) const;
    bool canMerge(const NextID &rhs) const;
    std::string toStr() const;

public:
    EIGHTCC mName;
    InP1Src mP1Src = IN_RRZO;
    MUINT32 mBatchIndex = 0;
    NextPathID mPath = N_PATH_DEFAULT;
};

extern const NextID NORMAL;

class NextAttr
{
public:
    MSize mResize;
    MRectF mCrop;
    PQType mPQ = PQ_NONE;
    MBOOL mReadOnly = MFALSE;             // img3o hint
    MBOOL mHighQuality = MTRUE;           // img2o hint
    MBOOL mRatioCrop = MFALSE;            // force src crop ratio match mResize ratio
    OutType mInplaceOutBuffer = OUT_NONE; // not used

public:
    void setAttrByPool(const android::sp<IBufferPool> &pool);

public:
    bool needBuffer() const;
    bool operator==(const NextAttr &rhs) const;
    std::string toStr() const;

private:
    void *mPoolToken = NULL;
};

class SFPIOUser
{
public:
    // For IOUtil
    static const char* toPathName(const NextPathID &path);
    static const char* toTargetName(const TargetPathID &path);
    static NextPathID toPathID(StreamType stream);
    static TargetPathID toTargetID(NextPathID path);

    typedef NextPathID IO_PathID;
    typedef TargetPathID IO_TargetID;
    typedef NextID IO_NextID;
    typedef NextAttr IO_NextAttr;
    static const NextPathID DefaultPathID_ID = N_PATH_DEFAULT;
    static const TargetPathID DefaultTargetID_ID = N_TARGET_DEFAULT;
};

typedef IOControl<NSFeaturePipe::StreamingFeatureNode, ReqInfo, SFPIOUser> Control;
typedef IORequest<NSFeaturePipe::StreamingFeatureNode, ReqInfo, SFPIOUser> Request;
typedef IOPolicy<SFPIOUser> Policy;

typedef Request::NextCollection NextReq;
typedef Control::NextImgReq NextImgReq;
typedef NextBuf<NextAttr> NextBuf;

MVOID print(const char *prefix, const ReqInfo &info);

} // namespace nextIO


} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_STREAMING_FEATURE_IO_H_
