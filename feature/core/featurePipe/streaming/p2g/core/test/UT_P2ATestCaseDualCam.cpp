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

void genTableDualGeneral3(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &sSW = table.slaveSW;
    AnsTable::AnsVec &sHW = table.slaveHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    sSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(2), O_TUND(2) });
    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    sHW.add(1, { I_RRZO(1), I_TUN(2), I_TUND(2), O_FD(1), O_DISP(1) });

    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(3), O_TUND(3) });
    sSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(4), O_TUND(4) });
    mHW.add(2, { I_RRZO(2), I_TUN(3), I_TUND(3), O_FD(2), O_DISP(2) });
    sHW.add(2, { I_RRZO(2), I_TUN(4), I_TUND(4), O_FD(2), O_DISP(2) });

    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(5), O_TUND(5) });
    sSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(6), O_TUND(6) });
    mHW.add(3, { I_RRZO(3), I_TUN(5), I_TUND(5), O_FD(3), O_DISP(3) });
    sHW.add(3, { I_RRZO(3), I_TUN(6), I_TUND(6), O_FD(3), O_DISP(3) });
}

void genTableDualFull3(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &sSW = table.slaveSW;
    AnsTable::AnsVec &sHW = table.slaveHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    sSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(2), O_TUND(2) });
    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1), O_FULL(1) });
    sHW.add(1, { I_RRZO(1), I_TUN(2), I_TUND(2), O_FD(1), O_DISP(1), O_FULL(1) });

    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(3), O_TUND(3) });
    sSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(4), O_TUND(4) });
    mHW.add(2, { I_RRZO(2), I_TUN(3), I_TUND(3), O_FD(2), O_DISP(2), O_FULL(2) });
    sHW.add(2, { I_RRZO(2), I_TUN(4), I_TUND(4), O_FD(2), O_DISP(2), O_FULL(2) });

    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(5), O_TUND(5) });
    sSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(6), O_TUND(6) });
    mHW.add(3, { I_RRZO(3), I_TUN(5), I_TUND(5), O_FD(3), O_DISP(3), O_FULL(3) });
    sHW.add(3, { I_RRZO(3), I_TUN(6), I_TUND(6), O_FD(3), O_DISP(3), O_FULL(3) });
}

void genTableDualLarge3(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &sSW = table.slaveSW;
    AnsTable::AnsVec &sHW = table.slaveHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    sSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(2), O_TUND(2) });
    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_LARG(1) });
    sHW.add(1, { I_RRZO(1), I_TUN(2), I_TUND(2), O_FD(1), O_LARG(1) });

    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(3), O_TUND(3) });
    sSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(4), O_TUND(4) });
    mHW.add(2, { I_RRZO(2), I_TUN(3), I_TUND(3), O_FD(2), O_LARG(2) });
    sHW.add(2, { I_RRZO(2), I_TUN(4), I_TUND(4), O_FD(2), O_LARG(2) });

    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(5), O_TUND(5) });
    sSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(6), O_TUND(6) });
    mHW.add(3, { I_RRZO(3), I_TUN(5), I_TUND(5), O_FD(3), O_LARG(3) });
    sHW.add(3, { I_RRZO(3), I_TUN(6), I_TUND(6), O_FD(3), O_LARG(3) });
}

void genTableDualPhysical3(AnsTable &table)   // Wade need modify
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &sSW = table.slaveSW;
    AnsTable::AnsVec &sHW = table.slaveHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    sSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), I_SYNT(1), O_AMET(1), O_HMET(1), O_TUN(2), O_TUND(2) });
    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1), O_PHY(1) });
    sHW.add(1, { I_RRZO(1), I_TUN(2), I_TUND(2), O_FD(1), O_DISP(1), O_PHY(1) });

    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(3), O_TUND(3) });
    sSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), I_SYNT(2), O_AMET(2), O_HMET(2), O_TUN(4), O_TUND(4) });
    mHW.add(2, { I_RRZO(2), I_TUN(3), I_TUND(3), O_FD(2), O_DISP(2), O_PHY(2) });
    sHW.add(2, { I_RRZO(2), I_TUN(4), I_TUND(4), O_FD(2), O_DISP(2), O_PHY(2) });

    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(5), O_TUND(5) });
    sSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), I_SYNT(3), O_AMET(3), O_HMET(3), O_TUN(6), O_TUND(6) });
    mHW.add(3, { I_RRZO(3), I_TUN(5), I_TUND(5), O_FD(3), O_DISP(3), O_PHY(3) });
    sHW.add(3, { I_RRZO(3), I_TUN(6), I_TUND(6), O_FD(3), O_DISP(3), O_PHY(3) });
}

void runDualGeneralPreview(MyTest &test)
{
  for( unsigned i = 0; i < 3; ++i )
  {
    P2G::IO ioGM(P2G::Scene::GM, P2G::Feature::FID::DUAL);
    test.fillIO(ioGM, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                 OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD);
    test.run(ioGM, i+1);
    P2G::IO ioGS(P2G::Scene::GS, P2G::Feature::FID::DUAL);
    test.fillIO(ioGS, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                 OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD);
    test.run(ioGS, i+1);
  }
}

void runDualFullPreview(MyTest &test)
{
    for( unsigned i = 0; i < 3; ++i )
    {
      P2G::IO ioGM(P2G::Scene::GM, P2G::Feature::FID::DUAL);
      test.fillIO(ioGM, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                   OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD|OUT_FULL);
      test.run(ioGM, i+1);
      P2G::IO ioGS(P2G::Scene::GS, P2G::Feature::FID::DUAL);
      test.fillIO(ioGS, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                   OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD|OUT_FULL);
      test.run(ioGS, i+1);
    }
}

void runDualLargePreview(MyTest &test)
{
  for( unsigned i = 0; i < 3; ++i )
  {
    P2G::IO ioGM(P2G::Scene::LM, P2G::Feature::FID::DUAL);
    test.fillIO(ioGM, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                 OUT_APP_META|OUT_HAL_META|OUT_FD|OUT_LARGE);
    test.run(ioGM, i+1);
    P2G::IO ioGS(P2G::Scene::LS, P2G::Feature::FID::DUAL);
    test.fillIO(ioGS, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                 OUT_APP_META|OUT_HAL_META|OUT_FD|OUT_LARGE);
    test.run(ioGS, i+1);
  }
}

void runDualPhysicalPreview(MyTest &test)
{
  for( unsigned i = 0; i < 3; ++i )
  {
    P2G::IO ioGM(P2G::Scene::PM, P2G::Feature::FID::DUAL);
    test.fillIO(ioGM, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                 OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD|OUT_PHY);
    test.run(ioGM, i+1);
    P2G::IO ioGS(P2G::Scene::PS, P2G::Feature::FID::DUAL);
    test.fillIO(ioGS, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|IN_SYNC_TUN|
                 OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD|OUT_PHY);
    test.run(ioGS, i+1);
  }
}

void testDualGeneralPreview(MyTest &test)
{
  MY_LOGD("\n<<<<testDualGeneralPreview>>>>");

  genTableDualGeneral3(test.mAnsTable);
  runDualGeneralPreview(test);

  test.mStrRet.append("    ");
  test.appendRetLog("testDualGeneralPreview", test.mVldRet);
}

void testDualFullPreview(MyTest &test)
{
  MY_LOGD("\n<<<<testDualFullPreview>>>>");

  genTableDualFull3(test.mAnsTable);
  runDualFullPreview(test);

  test.mStrRet.append("    ");
  test.appendRetLog("testDualFullPreview", test.mVldRet);
}

void testDualLargePreview(MyTest &test)
{
  MY_LOGD("\n<<<<testDualLargePreview>>>>");

  genTableDualLarge3(test.mAnsTable);
  runDualLargePreview(test);

  test.mStrRet.append("    ");
  test.appendRetLog("testDualLargePreview", test.mVldRet);
}

void testDualPhysicalPreview(MyTest &test)
{
  MY_LOGD("\n<<<<testDualPhysicalPreview>>>>");

  genTableDualPhysical3(test.mAnsTable);
  runDualPhysicalPreview(test);

  test.mStrRet.append("    ");
  test.appendRetLog("testDualPhysicalPreview", test.mVldRet);
}

void test_dual_cam(MyTest &test)
{
  MY_LOGD("\n<<<<test_dual_cam>>>>");

  bool bAllPass = true;
  test.reset();
  testDualGeneralPreview(test);
  bAllPass = bAllPass && test.mVldRet;
  test.reset();
  testDualFullPreview(test);
  bAllPass = bAllPass && test.mVldRet;
  test.reset();
  testDualLargePreview(test);
  bAllPass = bAllPass && test.mVldRet;
  test.reset();
  testDualPhysicalPreview(test);
  bAllPass = bAllPass && test.mVldRet;

  test.appendRetLog("test_dual_cam", bAllPass);
}

