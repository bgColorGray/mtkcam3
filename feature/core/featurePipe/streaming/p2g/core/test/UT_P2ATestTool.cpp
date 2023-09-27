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
#include "UT_P2AUtil.h"
#include "UT_LogUtil.h"
#include "UT_TypeWrapper.h"
#include "UT_P2AValidate.h"

#include <queue>
#include <vector>
#include <algorithm>
#include <map>

#include <iostream>
#include <string.h>

using namespace NSCam::NSCamFeature::NSFeaturePipe;
using std::vector;

void AnsTable::AnsVec::add(unsigned frame, unsigned subFrame, IO_ID id, IO_VAL value)
{
    if( frame >= 1 && subFrame >= 1)
    {
        //MY_LOGD("$$$ mVecMap.capacity():%lu, index:%u, id:%u, value:%u", mVecMap.capacity(), index, id, value);
        if( mVecMap.size() < (frame) )
            mVecMap.resize(frame);
        if( mVecMap[frame-1].size() < (subFrame) )
            mVecMap[frame-1].resize(subFrame);
        mVecMap[frame-1][subFrame-1].mMap[id] = value;
        mVecMap[frame-1][subFrame-1].mMask |= id;
    }
}
void AnsTable::AnsVec::add(unsigned frame, IO_ID id, IO_VAL value)
{
    add(frame, 1, id, value);
}
void AnsTable::AnsVec::add(unsigned frame, unsigned subFrame, Ans ans)
{
    add(frame, subFrame, ans.id, ans.val);
}
void AnsTable::AnsVec::add(unsigned frame, Ans ans)
{
    add(frame, 1, ans.id, ans.val);
}
void AnsTable::AnsVec::add(unsigned frame, unsigned subFrame, const std::vector<Ans> &vecAns)
{
    for( const Ans ans : vecAns ) { add(frame, subFrame, ans); }
}
void AnsTable::AnsVec::add(unsigned frame, const std::vector<Ans> &vecAns)
{
    for( const Ans ans : vecAns ) { add(frame, 1, ans); }
}

void MyTest::copy(P2G::IO &dst, P2G::IO &src, unsigned mask)
{
  if( mask & IN_RRZO )        dst.mIn.mIMGI = src.mIn.mIMGI;
  if( mask & IN_LCSO )        dst.mIn.mLCSO = src.mIn.mLCSO ;
  if( mask & IN_SYNC_TUN )    dst.mIn.mSyncTun = src.mIn.mSyncTun;
  if( mask & IN_APP_META )    dst.mIn.mAppInMeta = src.mIn.mAppInMeta ;
  if( mask & IN_HAL_META )    dst.mIn.mHalInMeta = src.mIn.mHalInMeta ;
  if( mask & OUT_DISPLAY )    dst.mOut.mDisplay = src.mOut.mDisplay ;
  if( mask & OUT_RECORD )     dst.mOut.mRecord = src.mOut.mRecord ;
  if( mask & OUT_EXTRA )      dst.mOut.mExtra = src.mOut.mExtra ;
  if( mask & OUT_FD )         dst.mOut.mFD = src.mOut.mFD ;
  if( mask & OUT_FULL )       dst.mOut.mFull = src.mOut.mFull ;
  if( mask & OUT_LARGE )      dst.mOut.mExtra = src.mOut.mExtra ;
  if( mask & OUT_PHY )        dst.mOut.mPhy = src.mOut.mPhy ;
  if( mask & OUT_APP_META )   dst.mOut.mAppOutMeta = src.mOut.mAppOutMeta;
  if( mask & OUT_HAL_META )   dst.mOut.mHalOutMeta = src.mOut.mHalOutMeta;
}

void TestPool::fillIO(P2G::IO &io, unsigned mask)
{
  if( mask & IN_RRZO )        io.mIn.mIMGI = mInRRZO.request();
  if( mask & IN_LCSO )        io.mIn.mLCSO = mInLCSO.request();
  if( mask & IN_SYNC_TUN )    io.mIn.mSyncTun = mInSyncTun.request();
  if( mask & IN_APP_META )    io.mIn.mAppInMeta = mInAppMeta.request();
  if( mask & IN_HAL_META )    io.mIn.mHalInMeta = mInHalMeta.request();
  if( mask & OUT_DISPLAY )    io.mOut.mDisplay = mOutDisplay.request();
  if( mask & OUT_RECORD )     io.mOut.mRecord = mOutRecord.request();
  if( mask & OUT_PREV_CB )    io.mOut.mExtra.push_back(mOutPreviewCB.request());
  if( mask & OUT_SNAPSHOT )   io.mOut.mExtra.push_back(mOutSnapshot.request());
  if( mask & OUT_THUMBNAIL )  io.mOut.mExtra.push_back(mOutThumbnail.request());
  if( mask & OUT_FD )         io.mOut.mFD = mOutFD.request();
  if( mask & OUT_FULL )       io.mOut.mFull = mOutFull.request();
  if( mask & OUT_LARGE )      io.mOut.mExtra.push_back(mOutLarge.request());
  if( mask & OUT_PHY )        io.mOut.mPhy = mOutPhysical.request();
  if( mask & OUT_APP_META )   io.mOut.mAppOutMeta = mOutAppMeta.request();
  if( mask & OUT_HAL_META )   io.mOut.mHalOutMeta = mOutHalMeta.request();
}

void TestPool::fillP2SW(P2G::P2SW &p2sw, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap)
{
  // LoopIn
  if( mask & P2G_LOOPIN_TUN_DATA ) p2sw.mBatchIn.mTuningData = mLoopInTunData.request(fillMap.at(P2G_LOOPIN_TUN_DATA));
  // In
  if( mask & P2G_IN_POD )          p2sw.mIn.mBasic = mInPOD.request(fillMap.at(P2G_IN_POD));
  if( mask & P2G_IN_LCSO )         p2sw.mIn.mLCSO = mInLCSO.request(fillMap.at(P2G_IN_LCSO));
  if( mask & P2G_IN_SYNC_TUN )     p2sw.mIn.mSyncTun = mInSyncTun.request(fillMap.at(P2G_IN_SYNC_TUN));
  if( mask & P2G_IN_APP_META )     p2sw.mIn.mAppInMeta = mInAppMeta.request(fillMap.at(P2G_IN_APP_META));
  if( mask & P2G_IN_HAL_META )     p2sw.mIn.mHalInMeta = mInHalMeta.request(fillMap.at(P2G_IN_HAL_META));
  if( mask & P2G_IN_DCESO )        p2sw.mIn.mDCE_n_2 = mDCESO.request(fillMap.at(P2G_IN_DCESO));
  if( mask & P2G_OUT_APP_META )    p2sw.mOut.mAppOutMeta = mOutAppMeta.request(fillMap.at(P2G_OUT_APP_META));
  if( mask & P2G_OUT_HAL_META )    p2sw.mOut.mHalOutMeta = mOutHalMeta.request(fillMap.at(P2G_OUT_HAL_META));
  if( mask & P2G_OUT_TUN )         p2sw.mOut.mTuning = mOutTun.request(fillMap.at(P2G_OUT_TUN));
  if( mask & P2G_OUT_TUN_DATA )    p2sw.mOut.mTuningData = mOutTunData.request(fillMap.at(P2G_OUT_TUN_DATA));
}

#define FILL_P2_OUT_VACANT(fkData) do {\
    if( p2hw.mOut.mWDMAO.isValid() ) p2hw.mOut.mWROTO = fkData;\
    else                             p2hw.mOut.mWDMAO = fkData;\
} while(0)
void TestPool::fillP2HW(P2G::P2HW &p2hw, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap)
{
  if( mask & P2G_IN_RRZO )      p2hw.mIn.mIMGI = mInRRZO.request(fillMap.at(P2G_IN_RRZO));
  if( mask & P2G_IN_TUN )       p2hw.mIn.mTuning = mInTun.request(fillMap.at(P2G_IN_TUN));
  if( mask & P2G_IN_TUN_DATA )  p2hw.mIn.mTuningData = mInTunData.request(fillMap.at(P2G_IN_TUN_DATA));
  if( mask & P2G_IN_DS0 )       p2hw.mIn.mIMGI = mDS0.request(fillMap.at(P2G_IN_DS0));
  if( mask & P2G_IN_DS1 )       p2hw.mIn.mIMGI = mDS1.request(fillMap.at(P2G_IN_DS1));
  if( mask & P2G_IN_DS2 )       p2hw.mIn.mDSI = mDS2.request(fillMap.at(P2G_IN_DS2));
  if( mask & P2G_IN_DN0 )       p2hw.mIn.mDSI = mDN0.request(fillMap.at(P2G_IN_DN0));
  if( mask & P2G_IN_DN1 )       p2hw.mIn.mDSI = mDN1.request(fillMap.at(P2G_IN_DN1));
  if( mask & P2G_IN_DN2 )       p2hw.mIn.mDSI = mDN2.request(fillMap.at(P2G_IN_DN2));
  if( mask & P2G_IN_VIPI0 )     p2hw.mIn.mVIPI = mVIPI0.request(fillMap.at(P2G_IN_VIPI0));
  if( mask & P2G_IN_VIPI1 )     p2hw.mIn.mVIPI = mVIPI1.request(fillMap.at(P2G_IN_VIPI1));
  if( mask & P2G_IN_VIPI2 )     p2hw.mIn.mVIPI = mVIPI2.request(fillMap.at(P2G_IN_VIPI2));
  if( mask & P2G_OUT_FD )       p2hw.mOut.mIMG2O = mOutFD.request(fillMap.at(P2G_OUT_FD));
  if( mask & P2G_OUT_DISPLAY )  p2hw.mOut.mWDMAO = mOutDisplay.request(fillMap.at(P2G_OUT_DISPLAY));
  if( mask & P2G_OUT_RECORD )   FILL_P2_OUT_VACANT(mOutRecord.request(fillMap.at(P2G_OUT_RECORD)));
  if( mask & P2G_OUT_FULL )     p2hw.mOut.mIMG3O = mOutFull.request(fillMap.at(P2G_OUT_FULL));
  if( mask & P2G_OUT_LARGE )    FILL_P2_OUT_VACANT(mOutLarge.request(fillMap.at(P2G_OUT_LARGE)));
  if( mask & P2G_OUT_PHY )      FILL_P2_OUT_VACANT(mOutPhysical.request(fillMap.at(P2G_OUT_PHY)));
  if( mask & P2G_OUT_DN0 )      p2hw.mOut.mIMG3O = mDN0.request(fillMap.at(P2G_OUT_DN0));
  if( mask & P2G_OUT_DN1 )      p2hw.mOut.mIMG3O = mDN1.request(fillMap.at(P2G_OUT_DN1));
  if( mask & P2G_OUT_DN2 )      p2hw.mOut.mIMG3O = mDN2.request(fillMap.at(P2G_OUT_DN2));
  if( mask & P2G_OUT_FULL_IMG ) p2hw.mOut.mIMG3O = mOutFullImg.request(fillMap.at(P2G_OUT_FULL_IMG));
  if( mask & P2G_OUT_DCESO )    p2hw.mOut.mDCESO = mDCESO.request(fillMap.at(P2G_OUT_DCESO));
}

void TestPool::fillPMDP(P2G::PMDP &pmdp, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap)
{
  if( mask & P2G_IN_FULL_IMG )  pmdp.mIn.mFull = mInFullImg.request(fillMap.at(P2G_IN_FULL_IMG));
  if( mask & P2G_IN_TUN )       pmdp.mIn.mTuning = mInTun.request(fillMap.at(P2G_IN_TUN));
  if( mask & P2G_OUT_VSS )      pmdp.mOut.mExtra.push_back(mOutSnapshot.request(fillMap.at(P2G_OUT_VSS)));
  if( mask & P2G_OUT_THUMB )    pmdp.mOut.mExtra.push_back(mOutThumbnail.request(fillMap.at(P2G_OUT_THUMB)));
}

void TestPool::fillMSS(P2G::MSS &mss, IO_ID mask, const std::map<IO_ID, IO_VAL> &fillMap)
{
  if( mask & P2G_IN_RRZO )      mss.mIn.mMSSI = mInRRZO.request(fillMap.at(P2G_IN_RRZO));
  if( mask & P2G_OUT_DS0 )      mss.mOut.mMSSOs.push_back(mDS0.request(fillMap.at(P2G_OUT_DS0)));
  if( mask & P2G_OUT_DS1 )      mss.mOut.mMSSOs.push_back(mDS1.request(fillMap.at(P2G_OUT_DS1)));
  if( mask & P2G_OUT_DS2 )      mss.mOut.mMSSOs.push_back(mDS2.request(fillMap.at(P2G_OUT_DS2)));
}

MyTest::MyTest()
{
  reset();
}

void MyTest::reset()
{
  initDataPool();
  mEngine.uninit();
  mEngine.init(mDataPool, P2G::Feature::NONE, false, false);

  mTestPoolM = TestPool("m_");
  mTestPoolS = TestPool("s_");
  FakeTunBufPool syncTun = FakeTunBufPool("syncTun");
  mTestPoolM.mInSyncTun = syncTun;
  mTestPoolS.mInSyncTun = syncTun;
  FakeTunBufPool ansSyncTun = FakeTunBufPool("syncTun");
  mAnsPoolM = TestPool("m_");
  mAnsPoolS = TestPool("s_");
  mAnsPoolM.mInSyncTun = ansSyncTun;
  mAnsPoolS.mInSyncTun = ansSyncTun;
  mAnsTable = AnsTable();
  mVldRet = true;
}

void MyTest::appendRetLog(const char *testName, bool bResult)
{
  char retLog[100];
  snprintf( retLog, sizeof(retLog), "Test <<<<%s>>>> %s\n", testName, bResult ? "pass" : "fail");
  MY_LOGD("%s", retLog);
  mStrRet.append(retLog);
}

void MyTest::run(const P2G::IO &io, unsigned frame)
{
  std::vector<P2G::IO> ioList;
  ioList.push_back(io);
  run(ioList, frame);
}

void MyTest::run(const std::vector<P2G::IO> &ioList, unsigned frame)
{
  P2G::Scene scene = ioList[0].mScene;
  const char *strDual = isMaster(scene) ? "Master" : "Slave";
  //MY_LOGD("\nframe %d ", frame);
  //MY_LOGD("io.mScene: %u ", io[0].mScene);
  P2G::Path path = mEngine.run(ioList);
  print(ioList);
  MY_LOGD("---- io print end ----");
  print(path);
  MY_LOGD("---- path print end ----");
  P2G::Path ansPath;
  fillAnswerPath(ansPath, mAnsTable, frame, scene);
  //print(ansPath);
  //MY_LOGD("---- ans path print end ----");
  if( mVldType == VDT_CMP_ANS || mVldType == VDT_ALL)
  {
      MY_LOGD("\n[validate] Frame(%u) Scene(%u) %s start", frame, scene, strDual);
      bool ret = validate(path, ansPath);
      MY_LOGD("[validate] Frame(%u) Scene(%u) %s result: %s", frame, scene, strDual, ret ? "Pass" : "Fail");
      mVldRet = ret && mVldRet;
  }
  if( mVldType == VDT_LOGICAL || mVldType == VDT_ALL)
  {
      // test logical
      mVldRet = validateLogicalRule(path, ioList, frame) && mVldRet;
  }
}

void MyTest::initDataPool()
{
  mDataPool = P2G::DataPool();

  mDataPool.mTuning = LazyTunBufferPool("tun");
  mDataPool.mSyncTuning = LazyTunBufferPool("syncTun");
  mDataPool.mFullImg = LazyImgBufferPool("fullImg");
  mDataPool.mDceso = LazyImgBufferPool("dceso");
  mDataPool.mTimgo = LazyImgBufferPool("timgo");
  mDataPool.mTuningData = LazyDataPool<TuningData>("tunData");
  mDataPool.mP2Obj = LazyDataPool<P2Obj>("p2obj");

  unsigned dsdn = 5;
  mDataPool.mDsImgs.resize(dsdn);
  mDataPool.mDnImgs.resize(dsdn);
  for( unsigned i = 0 ; i < dsdn; ++i )
  {
    char ds[10] = {0}, dn[10] = {0};
    snprintf(ds, sizeof(ds)-1, "ds%d", i);
    snprintf(dn, sizeof(dn)-1, "dn%d", i);
    mDataPool.mDsImgs[i] = LazyImgBufferPool(ds);
    mDataPool.mDnImgs[i] = LazyImgBufferPool(dn);
  }
}

void MyTest::fillIO(P2G::IO &io, unsigned mask)
{
    if(isMaster(io.mScene))
        mTestPoolM.fillIO(io, mask);
    else
        mTestPoolS.fillIO(io, mask);
}

void MyTest::fillAnswerPath(P2G::Path &path, const AnsTable &ansTable, unsigned frame, P2G::Scene scene)
{
    if(isMaster(scene))
    {
        fillP2SW(path, mAnsPoolM, ansTable.masterSW, frame);
        fillP2HW(path, mAnsPoolM, ansTable.masterHW, frame);
        fillPMDP(path, mAnsPoolM, ansTable.masterPMDP, frame);
        fillMSS(path, mAnsPoolM, ansTable.masterMSS, frame);
    }
    else
    {
        fillP2SW(path, mAnsPoolS, ansTable.slaveSW, frame);
        fillP2HW(path, mAnsPoolS, ansTable.slaveHW, frame);
        fillPMDP(path, mAnsPoolS, ansTable.slavePMDP, frame);
        fillMSS(path, mAnsPoolS, ansTable.slaveMSS, frame);
    }
}

void MyTest::fillP2SW(P2G::Path &path, TestPool &ansPool, const AnsTable::AnsVec &swVec,unsigned frame)
{
    //    fillP2SW
    if(frame <= swVec.mVecMap.size() && frame >= 1)
    {
        for(unsigned subFrame = 1; subFrame <= swVec.mVecMap[frame-1].size(); ++subFrame)
        {
            P2G::P2SW pathP2SW;
            fillP2SW(pathP2SW, ansPool, swVec, frame, subFrame);
            path.mP2SW.push_back(pathP2SW);
        }
    }
}

void MyTest::fillP2HW(P2G::Path &path, TestPool &ansPool, const AnsTable::AnsVec &hwVec,unsigned frame)
{
    //    fillP2HW
    if(frame <= hwVec.mVecMap.size() && frame >= 1)
    {
        for(unsigned subFrame = 1; subFrame <= hwVec.mVecMap[frame-1].size(); ++subFrame)
        {
            P2G::P2HW pathP2HW;
            fillP2HW(pathP2HW, ansPool, hwVec, frame, subFrame);
            path.mP2HW.push_back(pathP2HW);
        }
    }
}

void MyTest::fillPMDP(P2G::Path &path, TestPool &ansPool, const AnsTable::AnsVec &pmdpVec,unsigned frame)
{
    //    fillMSS
    if(frame <= pmdpVec.mVecMap.size() && frame >= 1)
    {
        for(unsigned subFrame = 1; subFrame <= pmdpVec.mVecMap[frame-1].size(); ++subFrame)
        {
            P2G::PMDP pathPMDP;
            fillPMDP(pathPMDP, ansPool, pmdpVec, frame, subFrame);
            path.mPMDP.push_back(pathPMDP);
        }
    }
}

void MyTest::fillMSS(P2G::Path &path, TestPool &ansPool, const AnsTable::AnsVec &mssVec,unsigned frame)
{
    //    fillMSS
    if(frame <= mssVec.mVecMap.size() && frame >= 1)
    {
        for(unsigned subFrame = 1; subFrame <= mssVec.mVecMap[frame-1].size(); ++subFrame)
        {
            P2G::MSS pathMSS;
            fillMSS(pathMSS, ansPool, mssVec, frame, subFrame);
            path.mMSS.push_back(pathMSS);
        }
    }
}

void MyTest::fillP2SW(P2G::P2SW &p2sw, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame)
{
    if(frame <= fillVec.mVecMap.size() && frame >= 1 && subFrame <= fillVec.mVecMap[frame-1].size() && subFrame >= 1)
        ansPool.fillP2SW(p2sw, fillVec.mVecMap[frame-1][subFrame-1].mMask, fillVec.mVecMap[frame-1][subFrame-1].mMap);
}

void MyTest::fillP2HW(P2G::P2HW &p2hw, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame)
{
    if(frame <= fillVec.mVecMap.size() && frame >= 1 && subFrame <= fillVec.mVecMap[frame-1].size() && subFrame >= 1)
        ansPool.fillP2HW(p2hw, fillVec.mVecMap[frame-1][subFrame-1].mMask, fillVec.mVecMap[frame-1][subFrame-1].mMap);
}

void MyTest::fillPMDP(P2G::PMDP &pmdp, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame)
{
    if(frame <= fillVec.mVecMap.size() && frame >= 1 && subFrame <= fillVec.mVecMap[frame-1].size() && subFrame >= 1)
        ansPool.fillPMDP(pmdp, fillVec.mVecMap[frame-1][subFrame-1].mMask, fillVec.mVecMap[frame-1][subFrame-1].mMap);
}

void MyTest::fillMSS(P2G::MSS &mss, TestPool &ansPool, const AnsTable::AnsVec &fillVec, unsigned frame, unsigned subFrame)
{
    if(frame <= fillVec.mVecMap.size() && frame >= 1 && subFrame <= fillVec.mVecMap[frame-1].size() && subFrame >= 1)
        ansPool.fillMSS(mss, fillVec.mVecMap[frame-1][subFrame-1].mMask, fillVec.mVecMap[frame-1][subFrame-1].mMap);
}

P2G::IO MyTest::gen(unsigned mask)
{
  P2G::IO io;
  mTestPoolM.fillIO(io, mask);
  return io;
}
