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

void genTableDSDN25Preview(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &mMSS = table.masterMSS;

    // frame 1
    mSW.add(1, 1, { I_AMET(1), I_HMET(1),                                  O_TUN(1), O_TUND(1) });
    mSW.add(1, 2, { I_AMET(1), I_HMET(1),                                  O_TUN(2), O_TUND(2) });
    mSW.add(1, 3, { I_AMET(1), I_HMET(1), I_LCSO(1), O_AMET(1), O_HMET(1), O_TUN(3), O_TUND(3) });

    mHW.add(1, 1, { I_DS2(1),  I_DS1(1),  I_TUN(1), I_TUND(1), O_DN1(1)});
    mHW.add(1, 2, { I_DN1(1),  I_DS0(1),  I_TUN(2), I_TUND(2), O_DN0(1)});
    mHW.add(1, 3, { I_DN0(1),  I_RRZO(1), I_TUN(3), I_TUND(3), O_DISP(1), O_FD(1) });

    mMSS.add(1, 1, { I_RRZO(1),  O_DS0(1),  O_DS1(1), O_DS2(1) });

    // frame 2
    mSW.add(2, 1, { I_AMET(2), I_HMET(2),                                  O_TUN(4), O_TUND(4) });
    mSW.add(2, 2, { I_AMET(2), I_HMET(2),                                  O_TUN(5), O_TUND(5) });
    mSW.add(2, 3, { I_AMET(2), I_HMET(2), I_LCSO(2), O_AMET(2), O_HMET(2), O_TUN(6), O_TUND(6) });

    mHW.add(2, 1, { I_DS2(2),  I_DS1(2),  I_TUN(4), I_TUND(4), O_DN1(2)});
    mHW.add(2, 2, { I_DN1(2),  I_DS0(2),  I_TUN(5), I_TUND(5), O_DN0(2)});
    mHW.add(2, 3, { I_DN0(2),  I_RRZO(2), I_TUN(6), I_TUND(6), O_DISP(2), O_FD(2) });

    mMSS.add(2, 1, { I_RRZO(2),  O_DS0(2),  O_DS1(2), O_DS2(2) });

    // frame 3
    mSW.add(3, 1, { I_AMET(3), I_HMET(3),                                  O_TUN(7), O_TUND(7) });
    mSW.add(3, 2, { I_AMET(3), I_HMET(3),                                  O_TUN(8), O_TUND(8) });
    mSW.add(3, 3, { I_AMET(3), I_HMET(3), I_LCSO(3), O_AMET(3), O_HMET(3), O_TUN(9), O_TUND(9) });

    mHW.add(3, 1, { I_DS2(3),  I_DS1(3),  I_TUN(7), I_TUND(7), O_DN1(3)});
    mHW.add(3, 2, { I_DN1(3),  I_DS0(3),  I_TUN(8), I_TUND(8), O_DN0(3)});
    mHW.add(3, 3, { I_DN0(3),  I_RRZO(3), I_TUN(9), I_TUND(9), O_DISP(3), O_FD(3) });

    mMSS.add(3, 1, { I_RRZO(3),  O_DS0(3),  O_DS1(3), O_DS2(3) });
}

void genTableDSDN25Record(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &mMSS = table.masterMSS;

    // frame 1
    mSW.add(1, 1, { I_AMET(1), I_HMET(1),                                  O_TUN(1), O_TUND(1) });
    mSW.add(1, 2, { I_AMET(1), I_HMET(1),                                  O_TUN(2), O_TUND(2) });
    mSW.add(1, 3, { I_AMET(1), I_HMET(1), I_LCSO(1), O_AMET(1), O_HMET(1), O_TUN(3), O_TUND(3) });

    mHW.add(1, 1, { I_DS2(1),  I_DS1(1),  I_TUN(1), I_TUND(1), O_DN1(1)});
    mHW.add(1, 2, { I_DN1(1),  I_DS0(1),  I_TUN(2), I_TUND(2), O_DN0(1)});
    mHW.add(1, 3, { I_DN0(1),  I_RRZO(1), I_TUN(3), I_TUND(3), O_DISP(1), O_FD(1), O_RECO(1) });

    mMSS.add(1, 1, { I_RRZO(1),  O_DS0(1),  O_DS1(1), O_DS2(1) });

    // frame 2
    mSW.add(2, 1, { I_AMET(2), I_HMET(2),                                  O_TUN(4), O_TUND(4) });
    mSW.add(2, 2, { I_AMET(2), I_HMET(2),                                  O_TUN(5), O_TUND(5) });
    mSW.add(2, 3, { I_AMET(2), I_HMET(2), I_LCSO(2), O_AMET(2), O_HMET(2), O_TUN(6), O_TUND(6) });

    mHW.add(2, 1, { I_DS2(2),  I_DS1(2),  I_TUN(4), I_TUND(4), O_DN1(2)});
    mHW.add(2, 2, { I_DN1(2),  I_DS0(2),  I_TUN(5), I_TUND(5), O_DN0(2)});
    mHW.add(2, 3, { I_DN0(2),  I_RRZO(2), I_TUN(6), I_TUND(6), O_DISP(2), O_FD(2), O_RECO(2) });

    mMSS.add(2, 1, { I_RRZO(2),  O_DS0(2),  O_DS1(2), O_DS2(2) });

    // frame 3
    mSW.add(3, 1, { I_AMET(3), I_HMET(3),                                  O_TUN(7), O_TUND(7) });
    mSW.add(3, 2, { I_AMET(3), I_HMET(3),                                  O_TUN(8), O_TUND(8) });
    mSW.add(3, 3, { I_AMET(3), I_HMET(3), I_LCSO(3), O_AMET(3), O_HMET(3), O_TUN(9), O_TUND(9) });

    mHW.add(3, 1, { I_DS2(3),  I_DS1(3),  I_TUN(7), I_TUND(7), O_DN1(3)});
    mHW.add(3, 2, { I_DN1(3),  I_DS0(3),  I_TUN(8), I_TUND(8), O_DN0(3)});
    mHW.add(3, 3, { I_DN0(3),  I_RRZO(3), I_TUN(9), I_TUND(9), O_DISP(3), O_FD(3), O_RECO(3) });

    mMSS.add(3, 1, { I_RRZO(3),  O_DS0(3),  O_DS1(3), O_DS2(3) });
}

void genTableDSDN25Nr3dRecord(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &mMSS = table.masterMSS;

    // frame 1
    mSW.add(1, 1, { I_AMET(1), I_HMET(1),                                  O_TUN(1), O_TUND(1) });
    mSW.add(1, 2, { I_AMET(1), I_HMET(1),                                  O_TUN(2), O_TUND(2) });
    mSW.add(1, 3, { I_AMET(1), I_HMET(1), I_LCSO(1), O_AMET(1), O_HMET(1), O_TUN(3), O_TUND(3) });

    mHW.add(1, 1, { I_DS2(1),  I_DS1(1),  I_TUN(1), I_TUND(1), O_DN1(1)});
    mHW.add(1, 2, { I_DN1(1),  I_DS0(1),  I_TUN(2), I_TUND(2), O_DN0(1)});
    mHW.add(1, 3, { I_DN0(1),  I_RRZO(1), I_TUN(3), I_TUND(3), O_FULLI(1), O_DISP(1), O_FD(1), O_RECO(1) });

    mMSS.add(1, 1, { I_RRZO(1),  O_DS0(1),  O_DS1(1), O_DS2(1) });

    // frame 2
    mSW.add(2, 1, { I_AMET(2), I_HMET(2),                                  O_TUN(4), O_TUND(4) });
    mSW.add(2, 2, { I_AMET(2), I_HMET(2),                                  O_TUN(5), O_TUND(5) });
    mSW.add(2, 3, { I_AMET(2), I_HMET(2), I_LCSO(2), O_AMET(2), O_HMET(2), O_TUN(6), O_TUND(6) });

    mHW.add(2, 1, { I_VIPI2(1), I_DS2(2),  I_DS1(2),  I_TUN(4), I_TUND(4), O_DN1(2)});
    mHW.add(2, 2, { I_VIPI1(1), I_DN1(2),  I_DS0(2),  I_TUN(5), I_TUND(5), O_DN0(2)});
    mHW.add(2, 3, { I_VIPI0(1), I_DN0(2),  I_RRZO(2), I_TUN(6), I_TUND(6), O_FULLI(2), O_DISP(2), O_FD(2), O_RECO(2) });

    mMSS.add(2, 1, { I_RRZO(2),  O_DS0(2),  O_DS1(2), O_DS2(2) });

    // frame 3
    mSW.add(3, 1, { I_AMET(3), I_HMET(3),                                  O_TUN(7), O_TUND(7) });
    mSW.add(3, 2, { I_AMET(3), I_HMET(3),                                  O_TUN(8), O_TUND(8) });
    mSW.add(3, 3, { I_AMET(3), I_HMET(3), I_LCSO(3), O_AMET(3), O_HMET(3), O_TUN(9), O_TUND(9) });

    mHW.add(3, 1, { I_VIPI2(2), I_DS2(3),  I_DS1(3),  I_TUN(7), I_TUND(7), O_DN1(3)});
    mHW.add(3, 2, { I_VIPI1(2), I_DN1(3),  I_DS0(3),  I_TUN(8), I_TUND(8), O_DN0(3)});
    mHW.add(3, 3, { I_VIPI0(2), I_DN0(3),  I_RRZO(3), I_TUN(9), I_TUND(9), O_FULLI(3), O_DISP(3), O_FD(3), O_RECO(3) });

    mMSS.add(3, 1, { I_RRZO(3),  O_DS0(3),  O_DS1(3), O_DS2(3) });
}

void genTableDSDN25Nr3dDceRecord(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &mMSS = table.masterMSS;

    // frame 1
    mSW.add(1, 1, { I_AMET(1), I_HMET(1),                                  O_TUN(1), O_TUND(1) });
    mSW.add(1, 2, { I_AMET(1), I_HMET(1),                                  O_TUN(2), O_TUND(2) });
    mSW.add(1, 3, { I_AMET(1), I_HMET(1), I_LCSO(1), O_AMET(1), O_HMET(1), O_TUN(3), O_TUND(3) });

    mHW.add(1, 1, { I_DS2(1),  I_DS1(1),  I_TUN(1), I_TUND(1), O_DN1(1)});
    mHW.add(1, 2, { I_DN1(1),  I_DS0(1),  I_TUN(2), I_TUND(2), O_DN0(1)});
    mHW.add(1, 3, { I_DN0(1),  I_RRZO(1), I_TUN(3), I_TUND(3), O_FULLI(1), O_DISP(1), O_FD(1), O_RECO(1), O_DCESO(1) });

    mMSS.add(1, 1, { I_RRZO(1),  O_DS0(1),  O_DS1(1), O_DS2(1) });

    // frame 2
    mSW.add(2, 1, { I_AMET(2), I_HMET(2),                                  O_TUN(4), O_TUND(4) });
    mSW.add(2, 2, { I_AMET(2), I_HMET(2),                                  O_TUN(5), O_TUND(5) });
    mSW.add(2, 3, { I_AMET(2), I_HMET(2), I_LCSO(2), O_AMET(2), O_HMET(2), O_TUN(6), O_TUND(6) });

    mHW.add(2, 1, { I_VIPI2(1), I_DS2(2),  I_DS1(2),  I_TUN(4), I_TUND(4), O_DN1(2)});
    mHW.add(2, 2, { I_VIPI1(1), I_DN1(2),  I_DS0(2),  I_TUN(5), I_TUND(5), O_DN0(2)});
    mHW.add(2, 3, { I_VIPI0(1), I_DN0(2),  I_RRZO(2), I_TUN(6), I_TUND(6), O_FULLI(2), O_DISP(2), O_FD(2), O_RECO(2), O_DCESO(2) });

    mMSS.add(2, 1, { I_RRZO(2),  O_DS0(2),  O_DS1(2), O_DS2(2) });

    // frame 3
    mSW.add(3, 1, { I_AMET(3), I_HMET(3),                                              O_TUN(7), O_TUND(7) });
    mSW.add(3, 2, { I_AMET(3), I_HMET(3),                                              O_TUN(8), O_TUND(8) });
    mSW.add(3, 3, { I_AMET(3), I_HMET(3), I_LCSO(3), I_DCESO(1), O_AMET(3), O_HMET(3), O_TUN(9), O_TUND(9) });

    mHW.add(3, 1, { I_VIPI2(2), I_DS2(3),  I_DS1(3),  I_TUN(7), I_TUND(7), O_DN1(3)});
    mHW.add(3, 2, { I_VIPI1(2), I_DN1(3),  I_DS0(3),  I_TUN(8), I_TUND(8), O_DN0(3)});
    mHW.add(3, 3, { I_VIPI0(2), I_DN0(3),  I_RRZO(3), I_TUN(9), I_TUND(9), O_FULLI(3), O_DISP(3), O_FD(3), O_RECO(3), O_DCESO(3) });

    mMSS.add(3, 1, { I_RRZO(3),  O_DS0(3),  O_DS1(3), O_DS2(3) });
}

void runDSDN25Preview(MyTest &test)
{
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io(P2G::Feature::FID::DSDN25);
        MSize size;
        //io.mIn.mDSDNSizes.push_back( MSize(640, 480) );
        //io.mIn.mDSDNSizes.push_back( MSize(320, 240) );
        //io.mIn.mDSDNSizes.push_back( MSize(160, 120) );
        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD);
        test.run(io, i+1);
    }
}

void runDSDN25Record(MyTest &test)
{
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io(P2G::Feature::FID::DSDN25);
        MSize size;
        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_RECORD|OUT_FD);
        test.run(io, i+1);
    }
}

void runDSDN25Nr3dRecord(MyTest &test)
{
    P2G::IO::LoopData loopData = P2G::IO::makeLoopData();
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io(P2G::Feature::DSDN25 | P2G::Feature::NR3D);
        MSize size;

        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        io.mLoopIn = loopData;
        io.mLoopOut = loopData = P2G::IO::makeLoopData();
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_RECORD|OUT_FD);
        test.run(io, i+1);
        loopData = io.mLoopOut;
    }
}

void runDSDN25Nr3dDceRecord(MyTest &test)
{
    P2G::IO::LoopData loopData = P2G::IO::makeLoopData();
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io(P2G::Feature::DSDN25 | P2G::Feature::NR3D | P2G::Feature::DCE);
        MSize size;

        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        io.mIn.mDSDNSizes.push_back( size );
        io.mLoopIn = loopData;
        io.mLoopOut = loopData = P2G::IO::makeLoopData();
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_RECORD|OUT_FD);
        test.run(io, i+1);
        loopData = io.mLoopOut;
    }
}

void test_dsdn(MyTest &test)
{
    MY_LOGD("\n<<<<test_DSDN>>>>");
    bool bAllPass = true;

    MY_LOGD("\n<<<<test_DSDN25_preview>>>>");
    genTableDSDN25Preview(test.mAnsTable);
    runDSDN25Preview(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_DSDN25_preview", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.reset();
    MY_LOGD("\n<<<<test_DSDN25_record>>>>");
    genTableDSDN25Record(test.mAnsTable);
    runDSDN25Record(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_DSDN25_record", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.reset();
    MY_LOGD("\n<<<<test_DSDN25_NR3D_record>>>>");
    genTableDSDN25Nr3dRecord(test.mAnsTable);
    runDSDN25Nr3dRecord(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_DSDN25_NR3D_record", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.reset();
    MY_LOGD("\n<<<<test_DSDN25_NR3D_DCE_record>>>>");
    genTableDSDN25Nr3dDceRecord(test.mAnsTable);
    runDSDN25Nr3dDceRecord(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_DSDN25_NR3D_DCE_record", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.appendRetLog("test_DSDN", bAllPass);
}

