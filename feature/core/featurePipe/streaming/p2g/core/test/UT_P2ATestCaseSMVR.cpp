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

#include "UT_P2ATestCase.h"
#include "UT_LogUtil.h"

#include <queue>
#include <vector>
#include <algorithm>

using std::vector;

void genTableSMVR7Bur2(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;

    // frame 1
    mSW.add(1, 1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1), });
    mHW.add(1, 1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_DISP(1), O_RECO(1) });
    // frame 2
    mSW.add(2, 1, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });
    mHW.add(2, 1, { I_RRZO(2), I_TUN(2), I_TUND(2), O_DISP(2), O_RECO(2) });
    // frame 3
    // vecSW
    mSW.add(3, 1, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3), });
    mSW.add(3, 2, { I_LCSO(4), LI_TUND(3),                                 O_TUN(3), O_TUND(4), });
    mSW.add(3, 3, { I_LCSO(5), LI_TUND(3),                                 O_TUN(3), O_TUND(5), });
    mSW.add(3, 4, { I_LCSO(6), LI_TUND(3),                                 O_TUN(3), O_TUND(6), });
    // vecHW
    mHW.add(3, 1, { I_RRZO(3), I_TUN(3), I_TUND(3), O_DISP(3), O_RECO(3) });
    mHW.add(3, 2, { I_RRZO(4), I_TUN(3), I_TUND(4),            O_RECO(4) });
    mHW.add(3, 3, { I_RRZO(5), I_TUN(3), I_TUND(5),            O_RECO(5) });
    mHW.add(3, 4, { I_RRZO(6), I_TUN(3), I_TUND(6),            O_RECO(6) });
    // frame 4
    // vecSW
    mSW.add(4, 1, { I_LCSO(11), I_AMET(4), I_HMET(4), O_AMET(4), O_HMET(4), O_TUN(4), O_TUND(7), });
    mSW.add(4, 2, { I_LCSO(7), LI_TUND(3),                                  O_TUN(3), O_TUND(8), });
    mSW.add(4, 3, { I_LCSO(8), LI_TUND(3),                                  O_TUN(3), O_TUND(9), });
    mSW.add(4, 4, { I_LCSO(9), LI_TUND(3),                                  O_TUN(3), O_TUND(10), });
    mSW.add(4, 5, { I_LCSO(10),LI_TUND(3),                                  O_TUN(3), O_TUND(11), });
    // vecHW
    mHW.add(4, 1, { I_RRZO(11), I_TUN(4), I_TUND(7), O_DISP(4) });
    mHW.add(4, 2, { I_RRZO(7),  I_TUN(3), I_TUND(8),            O_RECO(7) });
    mHW.add(4, 3, { I_RRZO(8),  I_TUN(3), I_TUND(9),            O_RECO(8) });
    mHW.add(4, 4, { I_RRZO(9),  I_TUN(3), I_TUND(10),           O_RECO(9) });
    mHW.add(4, 5, { I_RRZO(10), I_TUN(3), I_TUND(11),           O_RECO(10) });
    // frame 5
    // vecSW
    mSW.add(5, 1, { I_LCSO(19), I_AMET(5), I_HMET(5), O_AMET(5), O_HMET(5), O_TUN(5), O_TUND(12), });
    mSW.add(5, 2, { I_LCSO(12), LI_TUND(7),                                 O_TUN(4), O_TUND(13), });
    mSW.add(5, 3, { I_LCSO(13), LI_TUND(7),                                 O_TUN(4), O_TUND(14), });
    mSW.add(5, 4, { I_LCSO(14), LI_TUND(7),                                 O_TUN(4), O_TUND(15), });
    // vecHW
    mHW.add(5, 1, { I_RRZO(19), I_TUN(5), I_TUND(12), O_DISP(5) });
    mHW.add(5, 2, { I_RRZO(11), I_TUN(4), I_TUND(7),            O_RECO(11) });
    mHW.add(5, 3, { I_RRZO(12), I_TUN(4), I_TUND(13),           O_RECO(12) });
    mHW.add(5, 4, { I_RRZO(13), I_TUN(4), I_TUND(14),           O_RECO(13) });
    mHW.add(5, 5, { I_RRZO(14), I_TUN(4), I_TUND(15),           O_RECO(14) });
    // frame 6
    // vecSW
    mSW.add(6, 1, { I_LCSO(20), I_AMET(6), I_HMET(6), O_AMET(6), O_HMET(6), O_TUN(6), O_TUND(16), });
    mSW.add(6, 2, { I_LCSO(15), LI_TUND(7),                                 O_TUN(4), O_TUND(17), });
    mSW.add(6, 3, { I_LCSO(16), LI_TUND(7),                                 O_TUN(4), O_TUND(18), });
    mSW.add(6, 4, { I_LCSO(17), LI_TUND(7),                                 O_TUN(4), O_TUND(19), });
    mSW.add(6, 5, { I_LCSO(18), LI_TUND(7),                                 O_TUN(4), O_TUND(20), });
    // vecHW
    mHW.add(6, 1, { I_RRZO(20), I_TUN(6), I_TUND(16), O_DISP(6) });
    mHW.add(6, 2, { I_RRZO(15), I_TUN(4), I_TUND(17),            O_RECO(15) });
    mHW.add(6, 3, { I_RRZO(16), I_TUN(4), I_TUND(18),            O_RECO(16) });
    mHW.add(6, 4, { I_RRZO(17), I_TUN(4), I_TUND(19),            O_RECO(17) });
    mHW.add(6, 5, { I_RRZO(18), I_TUN(4), I_TUND(20),            O_RECO(18) });
    // frame 7
    // vecSW
    mSW.add(7, 1, { I_LCSO(21), I_AMET(7), I_HMET(7), O_AMET(7), O_HMET(7), O_TUN(7), O_TUND(21), });
    // vecHW
    mHW.add(7, 1, { I_RRZO(21), I_TUN(7), I_TUND(21), O_DISP(7) });
    mHW.add(7, 2, { I_RRZO(19), I_TUN(5), I_TUND(12),            O_RECO(19) });
    mHW.add(7, 3, { I_RRZO(20), I_TUN(6), I_TUND(16),            O_RECO(20) });
    mHW.add(7, 4, { I_RRZO(21), I_TUN(7), I_TUND(21),            O_RECO(21) });
}

void runSMVR7Bur2(MyTest &test)
{
    struct smvr_frame
    {
      unsigned mNumIn;
      unsigned mNumOut;
    } frames[] =
    {
      { 1, 1 },
      { 1, 1 },
      { 8, 4 },
      { 8, 4 },
      { 1, 4 },
      { 1, 4 },
      { 1, 4 },
    };

    struct smvrQ
    {
    P2G::IO::BatchData_const mBatch;
    P2G::IO::InData mIn;
    smvrQ() {}
    smvrQ(const P2G::IO::BatchData_const &batch, const P2G::IO &in)
      : mBatch(batch), mIn(in.mIn)
    {}
    };

    P2G::IO::LoopData loopM, loopS;
    P2G::IO::BatchData batch;
    std::queue<smvrQ> queue;
    unsigned nFrame = 0;
    for( const smvr_frame &f : frames )
    {
      vector<P2G::IO> ioList;
      vector<P2G::IO> rrzo;
      vector<P2G::IO> record;
      unsigned numRRZO = std::max<unsigned>(1, f.mNumIn);
      unsigned numRecord = f.mNumOut;
      unsigned optimize = (numRecord && queue.empty()) ? 1 : 0;

      rrzo.resize(numRRZO);
      std::generate_n(rrzo.begin(), numRRZO, [&]() { return test.gen(IN_BUFFER); });
      record.resize(numRecord);
      std::generate_n(record.begin(), numRecord, [&]() { return test.gen(OUT_RECORD); });

      P2G::IO main(P2G::Feature::SMVR_MAIN);
      main.mIn = rrzo[0].mIn;
      test.fillIO(main, IN_META|IN_DATA|OUT_META|OUT_DISPLAY);
      if( optimize )
      {
        main.mOut.mRecord = record[0].mOut.mRecord;
      }
      main.mBatchOut = batch = P2G::IO::makeBatchData();
      main.mLoopIn = loopM;
      main.mLoopOut = loopM = P2G::IO::makeLoopData();
      ioList.push_back(main);

      std::for_each(rrzo.begin(), rrzo.end(),
                   [&](P2G::IO &r){ test.copy(r, main, NONE); });

      std::for_each(rrzo.begin(), rrzo.end(),
                    [&](const P2G::IO &r){ queue.push(smvrQ(batch, r)); });

      for( unsigned i = 0; i < numRecord && queue.size(); ++i )
      {
        if( optimize && i == 0 )
        {
          loopS = loopM;
        }
        else
        {
          P2G::IO sub(P2G::Feature::SMVR_SUB);
          sub.mOut.mRecord = record[i].mOut.mRecord;
          sub.mBatchIn = queue.front().mBatch;
          sub.mIn = queue.front().mIn;
          sub.mLoopIn = loopS;
          sub.mLoopOut = loopS = P2G::IO::makeLoopData();
          ioList.push_back(sub);
        }
        queue.pop();
      }

      if( loopS && queue.empty() )
      {
        loopM = loopS;
      }

      test.run(ioList, ++nFrame);
    }
}

void test_smvr(MyTest &test)
{
  MY_LOGD("\n<<<<test_smvr>>>>");

  genTableSMVR7Bur2(test.mAnsTable);
  runSMVR7Bur2(test);

  test.appendRetLog("test_smvr", test.mVldRet);
}


