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

#include "UT_LogUtil.h"

#include <cstdio>

void newline()
{
  printf("\n");
}

void cls()
{
  system("clear >/dev/null 2>&1");
}

LogString::LogString()
{
}

LogString::LogString(const char *fmt, ...)
{
  if( fmt && !mFull )
  {
    va_list args;
    va_start(args, fmt);
    process(fmt, args);
    va_end(args);
  }
}

LogString::~LogString()
{
}

void LogString::printf(const char *fmt, ...)
{
  if( fmt && !mFull )
  {
    va_list args;
    va_start(args, fmt);
    process(fmt, args);
    va_end(args);
  }
}

void LogString::flush()
{
  ::printf("%s\n", mStr);
}

void LogString::flush(const char *fmt, ...)
{
  if( fmt && !mFull )
  {
    va_list args;
    va_start(args, fmt);
    process(fmt, args);
    va_end(args);
  }
  ::printf("%s\n", mStr);
}

void LogString::clear()
{
  mStr[0] = '\0';
  mLen = 0;
  mFull = false;
}

void LogString::reset(const char *fmt, ...)
{
  clear();
  if( fmt && !mFull )
  {
    va_list args;
    va_start(args, fmt);
    process(fmt, args);
    va_end(args);
  }
}

const char* LogString::c_str() const
{
  return mStr;
}

void LogString::process(const char *fmt, va_list &args)
{
  int len = vsnprintf(mStr+mLen, mSize-mLen, fmt, args);
  if( len < 0 )
  {
    mFull = true;
  }
  else
  {
    mLen += len;
    if( mLen >= mSize )
    {
      mFull = true;
      mLen = mSize - 1;
    }
  }
}

