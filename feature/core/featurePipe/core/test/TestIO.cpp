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
 * MediaTek Inc. (C) 2017. All rights reserved.
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

#include "TestIO.h"
#include <featurePipe/core/include/StringUtil.h>

namespace NSCam{
namespace NSCamFeature{
namespace NSFeaturePipe{

static const MUINT32 MAIN_SENSOR = 0;

// TestUser =======
const char* TestIOUser::toPathName(const NextPathID &path)
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

const char* TestIOUser::toTargetName(const TargetPathID &path)
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

NextPathID TestIOUser::toPathID(StreamType stream)
{
    switch(stream)
    {
        case STREAMTYPE_RECORD: return N_PATH_R;
        case STREAMTYPE_TINY:   return N_PATH_TINY;
        default:                return N_PATH_P; // default path is PATH_P
    }
}

TargetPathID TestIOUser::toTargetID(NextPathID id)
{
    switch(id)
    {
        case N_PATH_P:      return N_TARGET_P;
        case N_PATH_R:      return N_TARGET_R;
        case N_PATH_TINY:   return N_TARGET_TINY;
        default:            return N_TARGET_P;
    }
}

bool NextAttr::needBuffer() const
{
    return mAppInplace == APP_INPLACE_NONE;
}

bool NextAttr::operator==(const NextAttr &other)
{
    return (mPoolToken == other.mPoolToken) && (mResize == other.mResize) && (mCrop == other.mCrop)
            && (mCanShare == other.mCanShare) && (mAppInplace == other.mAppInplace) && (mHighQuality == other.mHighQuality)
            && (mRatioCrop == other.mRatioCrop) && (mPQType == other.mPQType);
}

std::string NextAttr::toStr() const
{
    StringUtil s;
    s.printf("pool(%p), resize(%dx%d), crop(%f,%f,%fx%f),"
                "share(%d), appInplace(%s), highQ(%d), ratioCrop(%d), pq(%s)",
                    mPoolToken, mResize.w, mResize.h,
                    mCrop.p.x, mCrop.p.y, mCrop.s.w, mCrop.s.h,
                    mCanShare, toName(mAppInplace), mHighQuality,
                    mRatioCrop, toName(mPQType));
    return std::string(s.c_str());
}

const char* NextAttr::toName(const AppInplaceImg &app)
{
    switch(app)
    {
        case APP_INPLACE_NONE:    return "none";
        case APP_INPLACE_PREVIEW: return "prev";
        case APP_INPLACE_RECORD:  return "rec";
        default:                  return "unknown";
    }
}
const char* NextAttr::toName(const PQType &pq)
{
    switch(pq)
    {
        case PQ_TYPE_NONE:      return "none";
        case PQ_TYPE_PREVIEW:   return "prev";
        case PQ_TYPE_RECORD:    return "rec";
        case PQ_TYPE_EXTRA:     return "extra";
        default:                return "unknown";
    }
}

NextID::NextID()
{}

NextID::NextID(NextImgID id, NextPathID path, P1SourceID p1Src, MUINT32 batch)
: mP1(p1Src, batch)
, mImgID(id)
, mPath(path)
{}

bool NextID::operator<(const NextID &o) const {
    return std::tie(mP1, mImgID, mPath) < std::tie(o.mP1, o.mImgID, o.mPath);
}

bool NextID::canMerge(const NextID &o) const {
    return mP1 == o.mP1;
}

std::string NextID::toStr() const
{
    StringUtil s;
    s.printf("%s, batch(%d), %s, %s", toName(mImgID), mP1.mBatch, toName(mP1.mSrc), TestIOUser::toPathName(mPath));
    return std::string(s.c_str());
}
const char* NextID::toName(const NextImgID &imgID)
{
    switch(imgID)
    {
        case N_IMG_NORMAL:   return "img_normal";
        case N_IMG_EXTRA_1:  return "img_ext1";
        case N_IMG_EXTRA_2:  return "img_ext2";
        case N_IMG_EXTRA_3:  return "img_ext3";
        default:             return "unknown";
    }
}

const char* NextID::toName(const P1SourceID &src)
{
    switch(src)
    {
        case P1_SRC_IMGO:   return "imgo";
        case P1_SRC_RRZO:   return "rrzo";
        default:            return "unknown";
    }
}

// TestIONode ======
TestIONode::TestIONode(const char *name)
    : mName(name)
    , mInputBufferPool(NULL)
    {}

TestIONode::TestIONode()
{

}

const char* TestIONode::getName() const
{
    return mName;
}

TestIOPolicy TestIONode::getIOPolicy(const TestReqInfo& reqInfo) const
{
    TestNodeConfig needNode;
    needNode.unpack(reqInfo.mFeatureMask);

    if( strcmp(getName(), NODEA_NAME) == 0 )
    {
        return getNodePolicy_A(needNode);
    }
    if( strcmp(getName(), NODEB_NAME) == 0 )
    {
        return getNodePolicy_B(needNode);
    }
    if( strcmp(getName(), NODEC_NAME) == 0 )
    {
        return getNodePolicy_C(needNode);
    }
    if( strcmp(getName(), NODED_NAME) == 0 )
    {
        return getNodePolicy_D(needNode);
    }
    if( strcmp(getName(), NODEE_NAME) == 0 )
    {
        return getNodePolicy_E(needNode);
    }
    return TestIOPolicy();
}

TestIOPolicy TestIONode::getNodePolicy_A(TestNodeConfig &config) const
{
    TestIOPolicy out;
    if( config.A )
    {
        out.enableAll(MAIN_SENSOR);

        if( config.A_3 )
            out.setLoopBack(MTRUE, MAIN_SENSOR);
    }
    return out;
}

TestIOPolicy TestIONode::getNodePolicy_B(TestNodeConfig &config) const
{
    TestIOPolicy out;
    if( config.B )
    {
        out.enableAll(MAIN_SENSOR);

        if( config.B_N )
            out.setNoMDP(MTRUE);
    }
    return out;
}

TestIOPolicy TestIONode::getNodePolicy_C(TestNodeConfig &config) const
{
    TestIOPolicy out;
    StreamMask run;
    if( config.C )
    {
        run.enableAll();
        if( !config.C_P )
        {
            run.disable(STREAMTYPE_PHYSICAL);
        }
        out.setRun(run, MAIN_SENSOR);
        NextID nextID(N_IMG_NORMAL);

        if( config.C_A )
        {
            NextAttr attr(mInputBufferPool);
            attr.mCrop = MRectF(MPointF(0,2), MSizeF(6,6));
            out.add(MAIN_SENSOR, nextID, attr, mInputBufferPool);
        }

        if( config.C_A3 )
        {
            // 0
            NextAttr attr(mInputBufferPool);
            attr.mResize = MSize(8,8);
            out.add(MAIN_SENSOR, nextID, attr, mInputBufferPool);

            // 1
            nextID.mImgID = N_IMG_EXTRA_1;
            attr = NextAttr(mExtraBufferPool_1);
            attr.mCrop = MRectF(MPointF(0,2), MSizeF(6,6));
            attr.mResize = MSize(4,4);
            out.add(MAIN_SENSOR, nextID, attr, mExtraBufferPool_1);

            // 2
            nextID.mImgID = N_IMG_EXTRA_2;
            attr = NextAttr(mExtraBufferPool_2);
            attr.mHighQuality = MFALSE;
            attr.mResize = MSize(2,2);
            out.add(MAIN_SENSOR, nextID, attr, mExtraBufferPool_2);

            // 3 imgo + preview inplace
            nextID.mImgID = N_IMG_EXTRA_3;
            nextID.mP1.mSrc = P1_SRC_IMGO;
            attr = NextAttr(NULL);
            attr.mAppInplace = NextAttr::APP_INPLACE_PREVIEW;
            out.add(MAIN_SENSOR, nextID, attr, NULL);
        }

        if( config.C_T )
        {
            NextAttr attr(mExtraBufferPool_T);
            attr.mCrop = MRectF(MPointF(0,2), MSizeF(5,5));
            attr.mHighQuality = MFALSE;
            attr.mResize = MSize(3,3);
            out.add(MAIN_SENSOR, NextID(N_IMG_EXTRA_2, N_PATH_TINY), attr, mExtraBufferPool_T);
        }

    }
    return out;
}

TestIOPolicy TestIONode::getNodePolicy_D(TestNodeConfig &config) const
{
    TestIOPolicy out;
    if( config.D )
    {
        out.enableAll(MAIN_SENSOR);
        if( config.D_A )
        {
            NextAttr attr(mInputBufferPool);
            out.add(MAIN_SENSOR, NextID(N_IMG_NORMAL), attr, mInputBufferPool);
        }
    }
    return out;
}

TestIOPolicy TestIONode::getNodePolicy_E(TestNodeConfig &config) const
{
    TestIOPolicy out;
    if( config.E )
    {
        out.enableAll(MAIN_SENSOR);
        if( config.E_A || config.E_AC )
        {
            NextAttr attr(mInputBufferPool);
            attr.mCrop = config.E_AC ? MRectF(MPointF(0.5f ,0.5f), MSizeF(4,4)) : MRectF();
            out.add(MAIN_SENSOR, NextID(N_IMG_NORMAL), attr, mInputBufferPool);
        }
    }
    return out;
}

void TestIONode::setInputBufferPool(android::sp<IBufferPool>& pool)
{
    mInputBufferPool = pool;
}

void TestIONode::setExtraBufferPool_1(android::sp<IBufferPool>& pool)
{
    mExtraBufferPool_1 = pool;
}

void TestIONode::setExtraBufferPool_2(android::sp<IBufferPool>& pool)
{
    mExtraBufferPool_2 = pool;
}

void TestIONode::setExtraBufferPool_T(android::sp<IBufferPool>& pool)
{
    mExtraBufferPool_T = pool;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
