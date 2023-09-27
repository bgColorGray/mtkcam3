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

int getTestCase()
{
  int testCase = 0;
  do {
      MY_LOGD("P2A test case : \n\
        0: bypass\n\
        1: test all case\n\
        2: checkValidateSanity\n\
        3: test basic preview, record, vss\n\
        4: test dual cam\n\
        5: test smvr\n\
        6: test dsdn\n\
        Enter test case(0/1/2/3/4/5/6) :");
      std::cin >> testCase;
      if(testCase >= TEST_BEGIN && testCase < TEST_END)
      {
        MY_LOGD("Input test case(%d)", testCase);
        return testCase;
      }
      else
      {
        MY_LOGW("Input number(%d) out of range!", testCase);
      }
  } while( true );
}

VDT_TYPE getValidateType()
{
  int validateType = 0;
  do {
      MY_LOGD("Run path validate type : \n\
        0: don't run validate\n\
        1: run both type\n\
        2: run compare answer validate\n\
        3: run logical validate\n\
        Enter test case(0/1/2/3) :");
      std::cin >> validateType;
      if(validateType >= VDT_BEGIN && validateType < VDT_END)
      {
        MY_LOGD("Input validate type(%d)", validateType);
        return static_cast<VDT_TYPE>(validateType);
      }
      else
      {
        MY_LOGW("Input validate type(%d) out of range!", validateType);
      }
  } while( true );
}

int run_suite_p2a(int argc, char *argv[])
{
  bool defaultTest = false;
  bool interactive = false;
  if(argc == 1) { defaultTest = true; }
  else if(argc == 2 && !strcmp(argv[1], "-i")) { interactive = true; }
  else { MY_LOGE("test_p2a argument error, bypass this test.\nTry using one '-i' argument to run interactive mode !"); }

  MyTest test;
  int testCase = defaultTest ? TEST_ALL : (interactive ? getTestCase() : TEST_BYPASS);
  test.mVldType = defaultTest ? VDT_ALL : (interactive ? getValidateType() : VDT_BYPASS);

  if( testCase == TEST_CHECK_VALIDATE || testCase == TEST_ALL )
  {
      // 1. check validate sanity
      test.reset();
      checkValidateSanity(test);
      if( testCase == TEST_CHECK_VALIDATE) MY_LOGD("TEST_CHECK_VALIDATE result:\n%s", test.mStrRet.c_str());
  }
  if( testCase == TEST_BASIC_PREVIEW || testCase == TEST_ALL )
  {
      // 2. validate path engine
      // 2-1. validate path engine : basic preview
      test.reset();
      test_basic(test);
      if( testCase == TEST_BASIC_PREVIEW) MY_LOGD("TEST_BASIC_PREVIEW result:\n%s", test.mStrRet.c_str());
  }
  if( testCase == TEST_DUAL || testCase == TEST_ALL )
  {
      // 2-2. validate path engine : dual cam basic preview
      test.reset();
      test_dual_cam(test);
      if( testCase == TEST_DUAL) MY_LOGD("TEST_DUAL result:\n%s", test.mStrRet.c_str());
  }
  if( testCase == TEST_SMVR || testCase == TEST_ALL )
  {
      // 2-3. validate path engine : basic smvr
      test.reset();
      test_smvr(test);
      if( testCase == TEST_SMVR) MY_LOGD("TEST_SMVR result:\n%s", test.mStrRet.c_str());
  }
  if( testCase == TEST_DSDN || testCase == TEST_ALL )
  {
      // 2-4. validate path engine : basic dsdn
      test.reset();
      test_dsdn(test);
      if( testCase == TEST_DSDN) MY_LOGD("TEST_DSDN result:\n%s", test.mStrRet.c_str());
  }

  if( testCase == TEST_ALL )
  {
    MY_LOGD("TEST_ALL result:\n%s", test.mStrRet.c_str());
  }
}
