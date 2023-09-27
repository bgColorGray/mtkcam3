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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_TEST_TOOL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_TEST_TOOL_H_

#include "../PathEngine.h"

#include <map>
#include <vector>
#include <string>

using namespace NSCam::NSCamFeature::NSFeaturePipe;

enum TestPoolID
{
  NONE            = 0,
  IN_RRZO         = 1 << 0,     // in
  IN_LCSO         = 1 << 1,
  IN_SYNC_TUN     = 1 << 2,
  IN_APP_META     = 1 << 3,
  IN_HAL_META     = 1 << 4,
  IN_POD          = 1 << 5,
  OUT_DISPLAY     = 1 << 6,     // out
  OUT_RECORD      = 1 << 7,
  OUT_PREV_CB     = 1 << 8,
  OUT_SNAPSHOT    = 1 << 9,
  OUT_THUMBNAIL   = 1 << 10,
  OUT_FD          = 1 << 11,
  OUT_FULL        = 1 << 12,
  OUT_LARGE       = 1 << 13,
  OUT_PHY         = 1 << 14,
  OUT_APP_META    = 1 << 19,
  OUT_HAL_META    = 1 << 20,
  OUT_EXTRA       = 1 << 21,
};
#define IN_BUFFER (IN_RRZO|IN_LCSO)
#define IN_META   (IN_APP_META|IN_HAL_META)
#define IN_DATA   (IN_POD)
#define OUT_META  (OUT_APP_META|OUT_HAL_META)

// P2G ID for answer table fill
enum P2G_ID : uint64_t
{
  P2G_NONE            = 0ULL,
  // In
  P2G_IN_BEGIN        = 1ULL << 0,
  P2G_IN_POD          = P2G_IN_BEGIN,
  P2G_IN_LCSO         = 1ULL << 1,
  P2G_IN_APP_META     = 1ULL << 2,
  P2G_IN_HAL_META     = 1ULL << 3,
  P2G_IN_RRZO         = 1ULL << 4,
  P2G_IN_TUN          = 1ULL << 5,
  P2G_IN_TUN_DATA     = 1ULL << 6,
  P2G_IN_SYNC_TUN     = 1ULL << 7,
  P2G_IN_DS0          = 1ULL << 8,
  P2G_IN_DS1          = 1ULL << 9,
  P2G_IN_DS2          = 1ULL << 10,
  P2G_IN_DN0          = 1ULL << 11,
  P2G_IN_DN1          = 1ULL << 12,
  P2G_IN_DN2          = 1ULL << 13,
  P2G_IN_VIPI0        = 1ULL << 14,
  P2G_IN_VIPI1        = 1ULL << 15,
  P2G_IN_VIPI2        = 1ULL << 16,
  P2G_IN_DCESO        = 1ULL << 17,
  P2G_IN_FULL_IMG     = 1ULL << 18,
  // Out
  P2G_OUT_BEGIN       = 1ULL << 19,
  P2G_OUT_APP_META    = P2G_OUT_BEGIN,
  P2G_OUT_HAL_META    = 1ULL << 20,
  P2G_OUT_TUN         = 1ULL << 21,
  P2G_OUT_TUN_DATA    = 1ULL << 22,
  P2G_OUT_FD          = 1ULL << 23,
  P2G_OUT_DISPLAY     = 1ULL << 24,
  P2G_OUT_RECORD      = 1ULL << 25,
  P2G_OUT_FULL        = 1ULL << 26,
  P2G_OUT_LARGE       = 1ULL << 27,
  P2G_OUT_PHY         = 1ULL << 28,
  P2G_OUT_DS0         = 1ULL << 29,
  P2G_OUT_DS1         = 1ULL << 30,
  P2G_OUT_DS2         = 1ULL << 31,
  P2G_OUT_DN0         = 1ULL << 32,
  P2G_OUT_DN1         = 1ULL << 33,
  P2G_OUT_DN2         = 1ULL << 34,
  P2G_OUT_FULL_IMG    = 1ULL << 35,
  P2G_OUT_DCESO       = 1ULL << 36,
  P2G_OUT_VSS         = 1ULL << 37,
  P2G_OUT_THUMB       = 1ULL << 38,
  // LoopIn
  P2G_LOOPIN_BEGIN    = 1ULL << 39,
  P2G_LOOPIN_TUN_DATA = P2G_LOOPIN_BEGIN,
};

#define I_POD(x)  Ans{P2G_IN_POD, x}
#define I_LCSO(x) Ans{P2G_IN_LCSO, x}
#define I_AMET(x) Ans{P2G_IN_APP_META, x}
#define I_HMET(x) Ans{P2G_IN_HAL_META, x}
#define I_RRZO(x) Ans{P2G_IN_RRZO, x}
#define I_TUN(x)  Ans{P2G_IN_TUN, x}
#define I_TUND(x) Ans{P2G_IN_TUN_DATA, x}
#define I_SYNT(x) Ans{P2G_IN_SYNC_TUN, x}
#define I_DS0(x)  Ans{P2G_IN_DS0, x}
#define I_DS1(x)  Ans{P2G_IN_DS1, x}
#define I_DS2(x)  Ans{P2G_IN_DS2, x}
#define I_DN0(x)  Ans{P2G_IN_DN0, x}
#define I_DN1(x)  Ans{P2G_IN_DN1, x}
#define I_DN2(x)  Ans{P2G_IN_DN2, x}
#define I_VIPI0(x) Ans{P2G_IN_VIPI0, x}
#define I_VIPI1(x) Ans{P2G_IN_VIPI1, x}
#define I_VIPI2(x) Ans{P2G_IN_VIPI2, x}
#define I_DCESO(x) Ans{P2G_IN_DCESO, x}
#define I_FULLI(x) Ans{P2G_IN_FULL_IMG, x}

#define O_AMET(x) Ans{P2G_OUT_APP_META, x}
#define O_HMET(x) Ans{P2G_OUT_HAL_META, x}
#define O_TUN(x)  Ans{P2G_OUT_TUN, x}
#define O_TUND(x) Ans{P2G_OUT_TUN_DATA, x}
#define O_FD(x)   Ans{P2G_OUT_FD, x}
#define O_DISP(x) Ans{P2G_OUT_DISPLAY, x}
#define O_RECO(x) Ans{P2G_OUT_RECORD, x}
#define O_FULL(x) Ans{P2G_OUT_FULL, x}
#define O_LARG(x) Ans{P2G_OUT_LARGE, x}
#define O_PHY(x)  Ans{P2G_OUT_PHY, x}
#define O_DS0(x)  Ans{P2G_OUT_DS0, x}
#define O_DS1(x)  Ans{P2G_OUT_DS1, x}
#define O_DS2(x)  Ans{P2G_OUT_DS2, x}
#define O_DN0(x)  Ans{P2G_OUT_DN0, x}
#define O_DN1(x)  Ans{P2G_OUT_DN1, x}
#define O_DN2(x)  Ans{P2G_OUT_DN2, x}
#define O_FULLI(x) Ans{P2G_OUT_FULL_IMG, x}
#define O_DCESO(x) Ans{P2G_OUT_DCESO, x}
#define O_VSS(x)   Ans{P2G_OUT_VSS, x}
#define O_THUMB(x) Ans{P2G_OUT_THUMB, x}

#define LI_TUND(x) Ans{P2G_LOOPIN_TUN_DATA, x}

typedef uint64_t IO_ID;
typedef unsigned IO_VAL;
struct Ans
{
    IO_ID id = 0;
    IO_VAL val = 0;
    Ans(IO_ID id, IO_VAL val):
        id(id),
        val(val)
        {}
};

struct AnsTable
{
    struct AnsMap
    {
        std::map<IO_ID, IO_VAL> mMap;
        IO_ID mMask;
    };
    struct AnsVec
    {
        std::vector<std::vector<AnsMap>> mVecMap;
        void add(unsigned frame, unsigned subFrame, IO_ID id, IO_VAL value);
        void add(unsigned frame, IO_ID id, IO_VAL value);
        void add(unsigned frame, unsigned subFrame, Ans ans);
        void add(unsigned frame, Ans ans);
        void add(unsigned frame, unsigned subFrame, const std::vector<Ans> &vecAns);
        void add(unsigned frame, const std::vector<Ans> &vecAns);
    };
    AnsVec masterSW;
    AnsVec masterHW;
    AnsVec masterPMDP;
    AnsVec masterMSS;
    AnsVec slaveSW;
    AnsVec slaveHW;
    AnsVec slavePMDP;
    AnsVec slaveMSS;
};

struct TestPool
{
  TestPool(std::string prefix = "")
    : mPrefix(prefix)
  {}

  std::string mPrefix = "";

  FakeMetaPool mInAppMeta               = FakeMetaPool(mPrefix + "appIn");
  FakeMetaPool mInHalMeta               = FakeMetaPool(mPrefix + "halIn");
  FakeImgPool mInRRZO                   = FakeImgPool(mPrefix + "rrzo");
  FakeImgPool mInLCSO                   = FakeImgPool(mPrefix + "lcso");
  FakePODPool mInPOD                    = FakePODPool(mPrefix + "pod");
  FakeRecordCropPool mInRecordCrop      = FakeRecordCropPool(mPrefix + "record_crop");
  FakeMetaPool mOutAppMeta              = FakeMetaPool(mPrefix + "appOut");
  FakeMetaPool mOutHalMeta              = FakeMetaPool(mPrefix + "halOut");
  FakeImgPool mOutDisplay               = FakeImgPool(mPrefix + "display");
  FakeImgPool mOutRecord                = FakeImgPool(mPrefix + "record");
  FakeImgPool mOutPreviewCB             = FakeImgPool(mPrefix + "prevCB");
  FakeImgPool mOutSnapshot              = FakeImgPool(mPrefix + "snapshot");
  FakeImgPool mOutThumbnail             = FakeImgPool(mPrefix + "thumbnail");
  FakeImgPool mOutFD                    = FakeImgPool(mPrefix + "fd");
  FakeImgPool mOutFull                  = FakeImgPool(mPrefix + "full");
  FakeImgPool mOutLarge                 = FakeImgPool(mPrefix + "large");
  FakeImgPool mOutPhysical              = FakeImgPool(mPrefix + "physical");

// working buffer pool for AnsPath
  FakeTunPool mOutTun                   = FakeTunPool("tun");
  FakeTunDataPool mOutTunData           = FakeTunDataPool("tunData");

  FakeTunPool mInTun                    = FakeTunPool("tun");
  FakeTunDataPool mInTunData            = FakeTunDataPool("tunData");

  FakeTunDataPool mLoopInTunData        = FakeTunDataPool("tunData");

// VSS working buffer pool for AnsPath
  FakeImgPool mInFullImg                = FakeImgPool("fullImg");

// DSDN working buffer pool for AnsPath
  FakeImgPool mDS0                      = FakeImgPool("ds0");
  FakeImgPool mDS1                      = FakeImgPool("ds1");
  FakeImgPool mDS2                      = FakeImgPool("ds2");
  FakeImgPool mDN0                      = FakeImgPool("dn0");
  FakeImgPool mDN1                      = FakeImgPool("dn1");
  FakeImgPool mDN2                      = FakeImgPool("dn2");

// NR3D working buffer pool for AnsPath
  FakeImgPool mVIPI2                    = FakeImgPool("dn1");
  FakeImgPool mVIPI1                    = FakeImgPool("dn0");
  FakeImgPool mVIPI0                    = FakeImgPool("fullImg");

  FakeImgPool mOutFullImg               = FakeImgPool("fullImg");

// DCE working buffer pool for AnsPath
  FakeImgPool mDCESO                    = FakeImgPool("dceso");

// common buffer pool
  FakeTunBufPool mInSyncTun;

  void fillIO(P2G::IO &io, unsigned mask);
  void fillP2SW(P2G::P2SW &p2sw, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap);
  void fillP2HW(P2G::P2HW &p2hw, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap);
  void fillPMDP(P2G::PMDP &pmdp, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap);
  void fillMSS(P2G::MSS &mss, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap);
};

enum VDT_TYPE
{
  VDT_BEGIN,
  VDT_BYPASS = VDT_BEGIN,
  VDT_ALL,
  VDT_CMP_ANS,
  VDT_LOGICAL,
  VDT_END
};

class MyTest
{
public:
  MyTest();

  void reset();
  void appendRetLog(const char *testName, bool bResult);
  void checkValidateSanity();
  void checkBasicValidateApi();
  void checkValidateSanityNormal3();
  void run(const P2G::IO &io, unsigned frame);
  void run(const std::vector<P2G::IO> &io, unsigned frame);
  void fillIO(P2G::IO &io, unsigned mask);
  void fillAnswerPath(P2G::Path &path, const AnsTable &ansTable, unsigned frame, P2G::Scene scene = P2G::Scene::GM);
  void copy(P2G::IO &dst, P2G::IO &src, unsigned mask);
  P2G::IO gen(unsigned mask);

private:
  void initDataPool();
  void fillP2SW(P2G::Path &path, TestPool &ansPool, const AnsTable::AnsVec &swVec, unsigned frame);
  void fillP2HW(P2G::Path &path, TestPool &ansPool, const AnsTable::AnsVec &hwVec, unsigned frame);
  void fillPMDP(P2G::Path &pmdp, TestPool &ansPool, const AnsTable::AnsVec &pmdpVec, unsigned frame);
  void fillMSS(P2G::Path &mss, TestPool &ansPool, const AnsTable::AnsVec &mssVec, unsigned frame);

  void fillP2SW(P2G::P2SW &p2sw, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame);
  void fillP2HW(P2G::P2HW &p2hw, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame);
  void fillPMDP(P2G::PMDP &pmdp, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame);
  void fillMSS(P2G::MSS &mss, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame);

public:
  AnsTable mAnsTable;
  bool mVldRet = true;
  std::string mStrRet = "";
  VDT_TYPE mVldType;

private:
  P2G::PathEngine mEngine;
  P2G::DataPool mDataPool;
  TestPool mTestPoolM = TestPool("m_");
  TestPool mTestPoolS = TestPool("s_");
  TestPool mAnsPoolM = TestPool("m_");
  TestPool mAnsPoolS = TestPool("s_");
};

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_TEST_TOOL_H_