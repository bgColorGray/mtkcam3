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

void genTableBasicPreview3(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
#if 1
    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2) });
    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3) });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3) });
#else
    for( unsigned frame = 1 ; frame <= 3 ; ++frame )
    {
        mSW.add(frame, I_LCSO(frame));
        mSW.add(frame, I_AMET(frame));
        mSW.add(frame, I_HMET(frame));
        mSW.add(frame, O_AMET(frame));
        mSW.add(frame, O_HMET(frame));
        mSW.add(frame, O_TUN(frame));
        mSW.add(frame, O_TUND(frame));

        mHW.add(frame, I_RRZO(frame));
        mHW.add(frame, I_TUN(frame));
        mHW.add(frame, I_TUND(frame));
        mHW.add(frame, O_FD(frame));
        mHW.add(frame, O_DISP(frame));
    }
#endif
}

void genTableBasicRecord3(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
#if 1
    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2) });
    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3) });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1), O_RECO(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2), O_RECO(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3), O_RECO(3) });
#else
    for( unsigned frame = 1 ; frame <= 3 ; ++frame )
    {
        mSW.add(frame, I_LCSO(frame));
        mSW.add(frame, I_AMET(frame));
        mSW.add(frame, I_HMET(frame));
        mSW.add(frame, O_AMET(frame));
        mSW.add(frame, O_HMET(frame));
        mSW.add(frame, O_TUN(frame));
        mSW.add(frame, O_TUND(frame));

        mHW.add(frame, I_RRZO(frame));
        mHW.add(frame, I_TUN(frame));
        mHW.add(frame, I_TUND(frame));
        mHW.add(frame, O_FD(frame));
        mHW.add(frame, O_DISP(frame));
        mHW.add(frame, O_RECO(frame));
    }
#endif
}

void genTableBasicVss(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;
    AnsTable::AnsVec &mPMDP = table.masterPMDP;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1) });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2) });
    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3) });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1), O_RECO(1), O_FULLI(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2), O_RECO(2), O_FULLI(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3), O_RECO(3), O_FULLI(3) });

    mPMDP.add(1, { I_FULLI(1), I_TUN(1), O_VSS(1), O_THUMB(1) });
    mPMDP.add(2, { I_FULLI(2), I_TUN(2), O_VSS(2), O_THUMB(2) });
    mPMDP.add(3, { I_FULLI(3), I_TUN(3), O_VSS(3), O_THUMB(3) });
}

void runBasicPreview3(MyTest &test)
{
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io;
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_FD);
        test.run(io, i+1);
    }
}

void runBasicRecord3(MyTest &test)
{
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io;
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_RECORD|OUT_FD);
        test.run(io, i+1);
    }
}

void runBasicVss(MyTest &test)
{
    for( unsigned i = 0; i < 3; ++i )
    {
        P2G::IO io;
        test.fillIO(io, IN_RRZO|IN_LCSO|IN_APP_META|IN_HAL_META|IN_POD|
                        OUT_APP_META|OUT_HAL_META|OUT_DISPLAY|OUT_RECORD|OUT_FD|OUT_SNAPSHOT|OUT_THUMBNAIL);
        test.run(io, i+1);
    }
}

void test_basic(MyTest &test)
{
    MY_LOGD("\n<<<<test_basic>>>>");
    bool bAllPass = true;

    MY_LOGD("\n<<<<test_basic_preview>>>>");
    genTableBasicPreview3(test.mAnsTable);
    runBasicPreview3(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_basic_preview", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.reset();
    MY_LOGD("\n<<<<test_basic_record>>>>");
    genTableBasicRecord3(test.mAnsTable);
    runBasicRecord3(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_basic_record", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.reset();
    MY_LOGD("\n<<<<test_basic_vss>>>>");
    genTableBasicVss(test.mAnsTable);
    runBasicVss(test);
    test.mStrRet.append("    ");
    test.appendRetLog("test_basic_vss", test.mVldRet);
    bAllPass = bAllPass && test.mVldRet;

    test.appendRetLog("test_basic", bAllPass);
}

