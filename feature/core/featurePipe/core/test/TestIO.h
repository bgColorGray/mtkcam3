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

#ifndef _TEST_IO_H_
#define _TEST_IO_H_

#include <vector>
#include <featurePipe/core/include/IOUtil.h>

namespace NSCam{
namespace NSCamFeature{
namespace NSFeaturePipe{

#define NODEA_NAME   "NodeA"
#define NODEB_NAME   "NodeB"
#define NODEC_NAME   "NodeC"
#define NODED_NAME   "NodeD"
#define NODEE_NAME   "NodeE"

#define NODEDA_NAME   "NodeDA"
#define NODEDB_NAME   "NodeDB"


enum NextPathID {
    /*
    * IOUtil use. If set default, the buffer path may comes from other P or R or....
    */
    N_PATH_DEFAULT,
    N_PATH_P,
    N_PATH_R,
    N_PATH_TINY,
};

enum TargetPathID { // TODO change to use SFP handle data id in the future
    /*
    * Default just for initial value
    */
    N_TARGET_DEFAULT,
    N_TARGET_P,
    N_TARGET_R,
    N_TARGET_TINY,
};

enum NextImgID {
    N_IMG_NORMAL,
    N_IMG_EXTRA_1,
    N_IMG_EXTRA_2,
    N_IMG_EXTRA_3,
    N_IMG_ID_MAX,
};
enum P1SourceID {
    P1_SRC_IMGO,
    P1_SRC_RRZO,
};

class NextAttr {
public:
    enum AppInplaceImg {
        APP_INPLACE_NONE,
        APP_INPLACE_PREVIEW,
        APP_INPLACE_RECORD,
    };
    enum PQType {
        PQ_TYPE_NONE,
        PQ_TYPE_PREVIEW,
        PQ_TYPE_RECORD,
        PQ_TYPE_EXTRA,
    };

    IBufferPool *mPoolToken = NULL; // Only Use to compare NextImg buffer pool
    MSize mResize;
    MRectF mCrop; // invalid if mRatioCrop = true
    /*
    * If set true, this buffer can be used by previeous node & this node in the same time.
    * It can reduce MDP throughput, but user must ensure no inplace happen in this buffer
    * It will be invalid if PQType / mCrop / mResize / is set.
    */
    MBOOL mCanShare = MFALSE;
    /*
    * User can ask previous node draw in app display/record buffer, and put in this NextImg.
    * If set this attribute, pq will be auto applied and other attribute will be invalid, EXCEPT mCrop & mSource.
    */
    AppInplaceImg mAppInplace = APP_INPLACE_NONE;
    /*
    * If set false, previous node will try to use non-MDP port to output this buffer(e.g. IMG2O) to improve
    * performance. The value false will be invalid in :
    *   a. non-MDP port is not valid, e.g. FD buffer occupied
    *   b. PQType is set
    */
    MBOOL mHighQuality = MTRUE;
    /*
    * If set, previous node will try to center crop input to match mResize W/H ratio.
    * For example, input(1920x1440) + mResize(640x360), it will crop (0,180,1920*1080) and then resize to 640x360.
    * This attr set true will make mCrop invalid.
    */
    MBOOL mRatioCrop = MFALSE;
    PQType mPQType = PQ_TYPE_NONE;
public:
    NextAttr(const android::sp<IBufferPool> &pool = NULL)
    : mPoolToken(pool.get())
    {}
    bool needBuffer() const;   // Need implement for IOUtil
    bool operator==(const NextAttr &other);   // Need implement for IOUtil
    std::string toStr() const;  // Need implement for IOUtil
    static const char* toName(const AppInplaceImg &app);
    static const char* toName(const PQType &pq);
};

class NextID {
public:

    struct P1Info
    {
        P1SourceID mSrc = P1_SRC_RRZO;
        MUINT32 mBatch = 0;
        bool operator==(const P1Info &other) const
        {
            return (mSrc == other.mSrc) && (mBatch == other.mBatch);
        }
        bool operator<(const P1Info &other) const
        {
            return (mSrc < other.mSrc) ||
                   ((mSrc == other.mSrc) && (mBatch < other.mBatch));
        }
        P1Info(P1SourceID p1Src = P1_SRC_RRZO, MUINT32 batch = 0)
        : mSrc(p1Src)
        , mBatch(batch)
        {}
    };
    P1Info mP1;
    NextImgID mImgID = N_IMG_NORMAL;
    // Must Declare for IOUtil
    NextPathID mPath = N_PATH_DEFAULT;

public:
    NextID();
    NextID(NextImgID id, NextPathID path = N_PATH_DEFAULT, P1SourceID p1Src = P1_SRC_RRZO, MUINT32 batch = 0);
    bool operator<(const NextID &o) const;   // Need implement for IOUtil
    bool canMerge(const NextID &o) const; //Need implement for IOUTI
    std::string toStr() const;  // Need implement for IOUtil
    static const char* toName(const NextImgID &imgID);
    static const char* toName(const P1SourceID &src);
};

class TestIOUser
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
// =========================


class TestReqInfo{
public:
    MUINT32 mFrameNo;
    MUINT32 mFeatureMask;
    MUINT32 mMasterId;
    MUINT32 mSensorId;
    MVOID makeDebugStr() {
        str = android::String8::format("No(%d), fmask(0x%08x), sId(%d), masterId(%d)",mFrameNo, mFeatureMask, mSensorId, mMasterId);
    }
    const char* dump() const { return str.c_str();}

    TestReqInfo(MUINT32 fno, MUINT32 mask, MUINT32 mId, MUINT32 sId)
    : mFrameNo(fno)
    , mFeatureMask(mask)
    , mMasterId(mId)
    , mSensorId(sId)
    {
        makeDebugStr();
    }
private:
    android::String8 str = android::String8("");
};

struct TestOutputConfig
{
    bool record;
    bool display;
    bool extra;
    bool phy;
    bool tiny;
};

struct TestNodeConfig
{
    bool A;
    bool A_3; // A + 3DNR
    bool B;
    bool B_N; // B + InOut_NEXT
    bool C;
    bool C_A; // Need input buf
    bool C_A3; // Need 3 input buf
    bool C_I; // Inplace, no use
    bool C_P; // C + PhysicalOut
    bool C_T; // C + Tiny
    bool D;
    bool D_A; // Need Input 1
    bool E;
    bool E_A; // Need Input 1
    bool E_AC; // Need Input 1 and crop
    bool DA;  // Dual In Single Out
    bool DB;  // Dual In Dual Out

    unsigned pack()
    {
        unsigned value = 0;
        int bit = 0;
        if(A)   value |= (1<<bit); bit++;
        if(A_3) value |= (1<<bit); bit++;
        if(B)   value |= (1<<bit); bit++;
        if(B_N) value |= (1<<bit); bit++;
        if(C)   value |= (1<<bit); bit++;
        if(C_A) value |= (1<<bit); bit++;
        if(C_A3) value |= (1<<bit); bit++;
        if(C_I) value |= (1<<bit); bit++;
        if(C_P) value |= (1<<bit); bit++;
        if(C_T) value |= (1<<bit); bit++;
        if(D)   value |= (1<<bit); bit++;
        if(D_A) value |= (1<<bit); bit++;
        if(E)   value |= (1<<bit); bit++;
        if(E_A) value |= (1<<bit); bit++;
        if(E_AC) value |= (1<<bit); bit++;
        if(DA)  value |= (1<<bit); bit++;
        if(DB)  value |= (1<<bit); bit++;

        //printf("pack 0x%08X\n", value);
        return value;
    }
    void unpack(unsigned value)
    {
        int bit = 0;
        A =    (value & (1<<bit)); bit++;
        A_3 =  (value & (1<<bit)); bit++;
        B =    (value & (1<<bit)); bit++;
        B_N =  (value & (1<<bit)); bit++;
        C =    (value & (1<<bit)); bit++;
        C_A =  (value & (1<<bit)); bit++;
        C_A3 = (value & (1<<bit)); bit++;
        C_I =  (value & (1<<bit)); bit++;
        C_P =  (value & (1<<bit)); bit++;
        C_T =  (value & (1<<bit)); bit++;
        D =    (value & (1<<bit)); bit++;
        D_A =  (value & (1<<bit)); bit++;
        E =    (value & (1<<bit)); bit++;
        E_A =  (value & (1<<bit)); bit++;
        E_AC = (value & (1<<bit)); bit++;
        DA =   (value & (1<<bit)); bit++;
        DB =   (value & (1<<bit)); bit++;

        //printf("unpack 0x%08X\n", value);
    }
};

struct TestNodeExpect
{
    bool record;
    bool display;
    bool extra;
    bool phy;
    bool tiny; // no need check because no need IORequest::needTiny() API
    bool full;
    bool AFull;
    bool A3Full;
    bool A3Full2P; // 2 path buffer can merged
    bool A3Full2P2L;// 2 path buffer can not merge
    bool DFull;
};

class TestNode;

struct TestCase
{
    //config
    TestOutputConfig hasOutput;
    TestNodeConfig needNode;

    //expect
    std::vector<TestNodeExpect> nodeExpect;
};

typedef IOPolicy<TestIOUser> TestIOPolicy;
typedef NextBuf<NextAttr> TestNImg;
class TestIONode
{
public:
    typedef TestIONode* NodeID_T;

    TestIONode(const char *name);
    TestIONode();
    const char* getName() const;
    TestIOPolicy getIOPolicy(const TestReqInfo& reqInfo) const;
    void setInputBufferPool(android::sp<IBufferPool>& pool);
    void setExtraBufferPool_1(android::sp<IBufferPool>& pool);
    void setExtraBufferPool_2(android::sp<IBufferPool>& pool);
    void setExtraBufferPool_T(android::sp<IBufferPool>& pool);
private:
    TestIOPolicy getNodePolicy_A(TestNodeConfig &config) const;
    TestIOPolicy getNodePolicy_B(TestNodeConfig &config) const;
    TestIOPolicy getNodePolicy_C(TestNodeConfig &config) const;
    TestIOPolicy getNodePolicy_D(TestNodeConfig &config) const;
    TestIOPolicy getNodePolicy_E(TestNodeConfig &config) const;


    const char* mName;
    android::sp<IBufferPool> mInputBufferPool;
    android::sp<IBufferPool> mExtraBufferPool_1;
    android::sp<IBufferPool> mExtraBufferPool_2;
    android::sp<IBufferPool> mExtraBufferPool_T;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _TEST_IO_H_
