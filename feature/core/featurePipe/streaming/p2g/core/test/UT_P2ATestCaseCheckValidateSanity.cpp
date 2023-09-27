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
#include "UT_P2AValidate.h"

enum CASE_TABLE
{
    CASE_TABLE_NONE,
    CASE_TABLE_BASIC_3_BEGIN,
    CASE_TABLE_BASIC_3_COR = CASE_TABLE_BASIC_3_BEGIN,
    CASE_TABLE_BASIC_3_WRN_A,
    CASE_TABLE_BASIC_3_WRN_B,
    CASE_TABLE_BASIC_3_WRN_C,
    CASE_TABLE_BASIC_3_WRN_D,
    CASE_TABLE_BASIC_3_END,
};

void genTableBasic3Cor(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1), });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });
    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3), });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3) });
}

void genTableBasic3WrnA(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1), });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });
    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3), });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    /*mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2) });*/
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3) });
}

void genTableBasic3WrnB(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1), });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });
    mSW.add(3, { /*I_LCSO(3),*/ I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3), });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3) });
}

void genTableBasic3WrnC(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1), });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });
    mSW.add(3, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3) });
}

void genTableBasic3WrnD(AnsTable &table)
{
    AnsTable::AnsVec &mSW = table.masterSW;
    AnsTable::AnsVec &mHW = table.masterHW;

    mSW.add(1, { I_LCSO(1), I_AMET(1), I_HMET(1), O_AMET(1), O_HMET(1), O_TUN(1), O_TUND(1), I_POD(1), });
    mSW.add(2, { I_LCSO(2), I_AMET(2), I_HMET(2), O_AMET(2), O_HMET(2), O_TUN(2), O_TUND(2), });
    mSW.add(3, { I_LCSO(3), I_AMET(3), I_HMET(3), O_AMET(3), O_HMET(3), O_TUN(3), O_TUND(3), });

    mHW.add(1, { I_RRZO(1), I_TUN(1), I_TUND(1), O_FD(1), O_DISP(1) });
    mHW.add(2, { I_RRZO(2), I_TUN(2), I_TUND(2), O_FD(2), O_DISP(2) });
    mHW.add(3, { I_RRZO(3), I_TUN(3), I_TUND(3), O_FD(3), O_DISP(3) });
}

bool getExpectRet(CASE_TABLE expCase)
{
    bool expectRet;
    switch( expCase ) {
        case CASE_TABLE_BASIC_3_COR:
            expectRet = true;
            break;
        case CASE_TABLE_BASIC_3_WRN_A:
        case CASE_TABLE_BASIC_3_WRN_B:
        case CASE_TABLE_BASIC_3_WRN_C:
        case CASE_TABLE_BASIC_3_WRN_D:
            expectRet = false;
            break;
        default:
            expectRet = false;
            MY_LOGE("Case table not found...");
            break;
    }
    return expectRet;
}

bool genTable(CASE_TABLE expCase, AnsTable &table)
{
    bool ret = false;

    switch( expCase ) {
        case CASE_TABLE_BASIC_3_COR:   // correct case : normal
            genTableBasic3Cor(table);
            ret = true;
            break;
        case CASE_TABLE_BASIC_3_WRN_A:   // remove frame 2 all "P2G_XXX#2" IO data
            genTableBasic3WrnA(table);
            ret = true;
            break;
        case CASE_TABLE_BASIC_3_WRN_B:   // remove "P2G_IN_LCSO#3" IO data
            genTableBasic3WrnB(table);
            ret = true;
            break;
        case CASE_TABLE_BASIC_3_WRN_C:   // set frame 3 "P2G_IO#2"
            genTableBasic3WrnC(table);
            ret = true;
            break;
        case CASE_TABLE_BASIC_3_WRN_D:   // add "P2G_IN_POD#1" (remember to add correspond IO FakePool like FakePODPool and request fake data)
            genTableBasic3WrnD(table);
            ret = true;
            break;
        default:
            ret = false;
            MY_LOGE("Case table not found...");
            break;
    }
    return ret;
}

void checkValidateSanityBasic3(MyTest &test)
{
    // one answer table
    genTableBasic3Cor(test.mAnsTable);

    // mutiple expect table
    unsigned passCaseCount = 0;
    unsigned totalExpCase = CASE_TABLE_BASIC_3_END - CASE_TABLE_BASIC_3_BEGIN;
    for( unsigned expCase = CASE_TABLE_BASIC_3_BEGIN ; expCase < CASE_TABLE_BASIC_3_END ; ++expCase )
    {
        MY_LOGD("expCase(%u) <<<<checkValidateSanity>>>>", expCase);
        AnsTable expTable;
        genTable(static_cast<CASE_TABLE>(expCase), expTable);
        bool expectRet = getExpectRet(static_cast<CASE_TABLE>(expCase));

        // start to check validate is work
        unsigned frame = 0;
        test.mVldRet = true;
        for( unsigned i = 0; i < 3; ++i )
        {
            ++frame;
            P2G::Path ansPath;
            test.fillAnswerPath(ansPath, test.mAnsTable, frame);
            P2G::Path expPath;
            test.fillAnswerPath(expPath, expTable, frame);
            MY_LOGD("[validate] Frame(%u) start", frame);
            bool ret = validate(expPath, ansPath);
            MY_LOGD("[validate] Frame(%u) result: %s", frame, ret ? "Pass" : "Fail");
            test.mVldRet = ret && test.mVldRet;
        }
        if(test.mVldRet == expectRet)
            ++passCaseCount;
        MY_LOGD("expCase(%u) <<<<checkValidateSanity>>>> validate sanity %s, validate ret:(%d) expect ret(%d)\n",
                 expCase, (test.mVldRet == expectRet) ? "pass" : "fail", test.mVldRet, expectRet);
    }
    bool bAllPass = false;
    if(passCaseCount == totalExpCase)   bAllPass = true;
    test.appendRetLog("checkValidateSanity", bAllPass);
    MY_LOGD("Test <<<<checkValidateSanity>>>> validate sanity %s, pass case count: (%u/%u)",
            bAllPass ? "pass" : "fail", passCaseCount, totalExpCase);
}

#define CHECK_FAKE_DATA_EQU(a, b, exp) (( a == b ) == exp)
#define CHECK_FAKE_DATA_NEQ(a, b, exp) (( a != b ) == exp)
#define CHECK_FAKE_DATA_COMP(a, b, exp) (compare(log, a, b, "test") == exp)
void checkBasicValidateApi()
{
    // check Basic Validate
    MY_LOGD("\n<<<<checkBasicValidateApi>>>>");
    // IFakeData::STATE_VALID
    FakeData<int> a1 = FakeData<int>("data#1", IFakeData::STATE_VALID);
    FakeData<int> a2 = FakeData<int>("data#2", IFakeData::STATE_VALID);
    // IFakeData::STATE_INVALID
    FakeData<int> b0 = FakeData<int>();
    FakeData<int> b1 = FakeData<int>("data#1", IFakeData::STATE_INVALID);
    FakeData<int> b2 = FakeData<int>("data#2", IFakeData::STATE_INVALID);

    bool bCheck;
    // check operator ==
    bCheck = ( CHECK_FAKE_DATA_EQU(a1, a1, true)  && CHECK_FAKE_DATA_EQU(a1, a2, false) &&
               CHECK_FAKE_DATA_EQU(a1, b0, false) && CHECK_FAKE_DATA_EQU(a1, b1, false) &&
               CHECK_FAKE_DATA_EQU(b0, b0, true)  && CHECK_FAKE_DATA_EQU(b0, b1, true)  &&
               CHECK_FAKE_DATA_EQU(b1, b1, true)  && CHECK_FAKE_DATA_EQU(b1, b2, true) );
    MY_LOGD("check fake data operator == : %s", bCheck ? "pass" : "fail");
    // check operator !=
    bCheck = ( CHECK_FAKE_DATA_NEQ(a1, a1, false) && CHECK_FAKE_DATA_NEQ(a1, a2, true)  &&
               CHECK_FAKE_DATA_NEQ(a1, b0, true)  && CHECK_FAKE_DATA_NEQ(a1, b1, true)  &&
               CHECK_FAKE_DATA_NEQ(b0, b0, false) && CHECK_FAKE_DATA_NEQ(b0, b1, false) &&
               CHECK_FAKE_DATA_NEQ(b1, b1, false) && CHECK_FAKE_DATA_NEQ(b1, b2, false) );
    MY_LOGD("check fake data operator != : %s", bCheck ? "pass" : "fail");
    // check validate compare api
    LogString log("check validate compare:");
    bCheck = ( CHECK_FAKE_DATA_COMP(a1, a1, true)  && CHECK_FAKE_DATA_COMP(a1, a2, false) &&
               CHECK_FAKE_DATA_COMP(a1, b0, false) && CHECK_FAKE_DATA_COMP(a1, b1, false) &&
               CHECK_FAKE_DATA_COMP(b0, b0, true)  && CHECK_FAKE_DATA_COMP(b0, b1, true)  &&
               CHECK_FAKE_DATA_COMP(b1, b1, true)  && CHECK_FAKE_DATA_COMP(b1, b2, true) );
    log.flush();
    MY_LOGD("check validate compare function : %s", bCheck ? "pass" : "fail");
}

void checkValidateSanity(MyTest &test)
{
    MY_LOGD("\n<<<<checkValidateSanity>>>>");

    checkBasicValidateApi();

    checkValidateSanityBasic3(test);
}

