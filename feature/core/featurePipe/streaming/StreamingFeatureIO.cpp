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

#include "StreamingFeatureNode.h"

#define PIPE_CLASS_TAG "IO"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_IO
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace NextIO {

const NextID NORMAL(EIGHTCC("full"));

const char* toName(InP1Src p1)
{
    switch(p1)
    {
        case IN_IMGO:           return "p1_imgo";
        case IN_RRZO:           return "p1_rrzo";
        default:                return "p1_unknown";
    }
}

const char* toName(InMetaType meta)
{
    switch(meta)
    {
        case IN_META_APP:       return "meta_app";
        case IN_META_P1_APP:    return "meta_p1_app";
        case IN_META_P1_HAL:    return "meta_p1_hal";
        default:                return "meta_unknown";
    }
}

const char* toName(PQType pq)
{
    switch(pq)
    {
        case PQ_NONE:           return "pq_none";
        case PQ_PREVIEW:        return "pq_preview";
        case PQ_RECORD:         return "pq_record";
        default:                return "pq_unknown";
    }
}

const char* toName(OutType type)
{
    switch(type)
    {
        case OUT_NONE:          return "out_none";
        case OUT_PREVIEW:       return "out_preview";
        case OUT_RECORD:        return "out_record";
        case OUT_EXTRA:         return "out_extra";
        case OUT_PHY:           return "out_phy";
        default:                return "out_unknown";
    }
}

const char* toName(NextPathID path)
{
    switch(path)
    {
        case N_PATH_DEFAULT:    return "default";
        case N_PATH_P:          return "preview";
        case N_PATH_R:          return "record";
        case N_PATH_TINY:       return "tiny";
        default:                return "unknown";
    }
}

MVOID print(const char *prefix, const ReqInfo &req)
{
    const auto mIt = req.mOutInfoMap.find(req.mMasterID);
    const auto sIt = req.mOutInfoMap.find(req.mSlaveID);
    size_t mCount = (mIt != req.mOutInfoMap.end()) ? mIt->second.size() : 0;
    size_t sCount = (sIt != req.mOutInfoMap.end()) ? sIt->second.size() : 0;

    MY_LOGD("%s#%d: fmask(0x%08x), m/s(%d/%d), in[%zu], out[%zu](%zu/%zu)",
            prefix, req.mFrameNo, req.mFeatureMask,
            req.mMasterID, req.mSlaveID,
            req.mInInfoMap.size(), req.mOutInfoMap.size(),
            mCount, sCount);
}

InInfo::InInfo()
{
}

InInfo::InInfo(InP1Src p1Src, const MSize &size, const MRect &crop)
    : mP1Src(p1Src)
    , mSize(size)
    , mSensorCrop(crop)
{
}

InInfo::operator bool() const
{
    return mSize.w > 0 && mSize.h > 0;
}

OutInfo::OutInfo()
{
}

OutInfo::OutInfo(OutType outType, const MSize &size, const MRectF &crop)
    : mType(outType)
    , mSize(size)
    , mCrop(crop)
{
}

OutInfo::operator bool() const
{
    return mSize.w > 0 && mSize.h > 0;
}

NextID::NextID(const EIGHTCC &name, InP1Src p1Src, MUINT32 batchIndex, NextPathID path)
    : mName(name)
    , mP1Src(p1Src)
    , mBatchIndex(batchIndex)
    , mPath(path)
{
}

bool NextID::operator==(const NextID &rhs) const
{
    return (mP1Src == rhs.mP1Src) &&
           (mBatchIndex == rhs.mBatchIndex) &&
           (mName == rhs.mName) &&
           (mPath == rhs.mPath);
}

bool NextID::operator<(const NextID &rhs) const
{
    return std::tie(mName, mP1Src, mBatchIndex, mPath) <
           std::tie(rhs.mName, rhs.mP1Src, rhs.mBatchIndex, rhs.mPath);
}

bool NextID::canMerge(const NextID &rhs) const
{
    return (mP1Src == rhs.mP1Src) && (mBatchIndex == rhs.mBatchIndex);
}

std::string NextID::toStr() const
{
    StringUtil s;
    s.printf("%s(%s[%d]/%s)", mName.c_str(), toName(mP1Src), mBatchIndex, toName(mPath));
    const char *ss = s.c_str();
    return std::string(ss != NULL ? ss : "unknown");
}

void NextAttr::setAttrByPool(const android::sp<IBufferPool> &pool)
{
    mPoolToken = (void*)pool.get();
}

bool NextAttr::needBuffer() const
{
    return (mInplaceOutBuffer == OUT_NONE);
}

bool NextAttr::operator==(const NextAttr &rhs) const
{
    return (mPoolToken == rhs.mPoolToken) &&
           (mPQ == rhs.mPQ) &&
           (mInplaceOutBuffer == rhs.mInplaceOutBuffer) &&
           (mReadOnly == rhs.mReadOnly) &&
           (mHighQuality == rhs.mHighQuality) &&
           (mRatioCrop == rhs.mRatioCrop) &&
           (mCrop == rhs.mCrop) &&
           (mResize == rhs.mResize);
}

std::string NextAttr::toStr() const
{
    StringUtil s;
    s.printf("pool(%p), resize(%dx%d), crop(%.2f,%.2f,%.2fx%.2f),"
             " share(%d), appInplace(%s), highQ(%d), ratioCrop(%d), pq(%s)",
                    mPoolToken, mResize.w, mResize.h,
                    mCrop.p.x, mCrop.p.y, mCrop.s.w, mCrop.s.h,
                    mReadOnly, toName(mInplaceOutBuffer), mHighQuality,
                    mRatioCrop, toName(mPQ));
    const char *ss = s.c_str();
    return std::string(ss != NULL ? ss : "unknown");
}

///////////////////////////////////////////////////////////
// TODO: XNode check dependency
///////////////////////////////////////////////////////////

const char* SFPIOUser::toPathName(const NextPathID &path)
{
    switch(path)
    {
        case N_PATH_DEFAULT:    return "path_default";
        case N_PATH_P:          return "path_Prev";
        case N_PATH_R:          return "path_Rec";
        case N_PATH_TINY:       return "path_Tiny";
        default:                return "path_unknown";
    }
}

const char* SFPIOUser::toTargetName(const TargetPathID &path)
{
    switch(path)
    {
        case N_TARGET_DEFAULT:    return "tar_default";
        case N_TARGET_P:          return "tar_Prev";
        case N_TARGET_R:          return "tar_Rec";
        case N_TARGET_TINY:       return "tar_Tiny";
        default:                  return "tar_unknown";
    }
}

NextPathID SFPIOUser::toPathID(StreamType stream)
{
    switch(stream)
    {
        case STREAMTYPE_RECORD: return N_PATH_R;
        case STREAMTYPE_TINY:   return N_PATH_TINY;
        default:                return N_PATH_P; // default path is PATH_P
    }
}

TargetPathID SFPIOUser::toTargetID(NextPathID id)
{
    switch(id)
    {
        case N_PATH_P:      return N_TARGET_P;
        case N_PATH_R:      return N_TARGET_R;
        case N_PATH_TINY:   return N_TARGET_TINY;
        default:            return N_TARGET_P;
    }
}

ReqInfo::ReqInfo(MUINT32 frame, MUINT32 mask, MUINT32 master, MUINT32 slave)
    : mFrameNo(frame)
    , mFeatureMask(mask)
    , mMasterID(master)
    , mSlaveID(slave)
{
    if( hasMaster() ) mSensorIDs.push_back(mMasterID);
    if( hasSlave() ) mSensorIDs.push_back(mSlaveID);
}

MBOOL ReqInfo::hasMaster() const
{
    return mMasterID != INVALID_SENSOR;
}

MBOOL ReqInfo::hasSlave() const
{
    return mSlaveID != INVALID_SENSOR;
}

InInfo ReqInfo::getInInfo(InP1Src p1Src, MUINT32 sensor) const
{
    InInfo info;
    auto it = mInInfoMap.find((sensor == INVALID_SENSOR) ? mMasterID : sensor);
    if( it != mInInfoMap.end() )
    {
        auto info_it = it->second.find(p1Src);
        if( info_it != it->second.end() )
        {
            info = info_it->second;
        }
    }
    return info;
}

OutInfo ReqInfo::getOutInfo(OutType outType, MUINT32 sensor) const
{
    OutInfo info;
    auto it = mOutInfoMap.find((sensor == INVALID_SENSOR) ? mMasterID : sensor);
    if( it != mOutInfoMap.end() )
    {
        for( const OutInfo &out : it->second )
        {
            if( out.mType == outType )
            {
                info = out;
                break;
            }
        }
    }
    return info;
}

IMetadata* ReqInfo::getInMeta(InMetaType type, MUINT32 sensor) const
{
    IMetadata *meta = NULL;
    auto it = mMetaMap.find((sensor == INVALID_SENSOR) ? mMasterID : sensor);
    if( it != mMetaMap.end() )
    {
        auto meta_it = it->second.find(type);
        if( meta_it != it->second.end() )
        {
            meta = meta_it->second;
        }
    }
    return meta;
}


} // namespace NextIO
} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
