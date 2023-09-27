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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_FAKE_POOL_T_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_FAKE_POOL_T_H_

#include <string>

class IFakeData
{
public:
  enum STATE
  {
    STATE_INVALID = 0,
    STATE_VALID = 1,
  };

public:
  IFakeData();
  IFakeData(STATE state);
  IFakeData(const std::string &name, unsigned num, STATE state = STATE_INVALID);
  ~IFakeData();
  bool operator==(const IFakeData &rhs) const;
  bool operator!=(const IFakeData &rhs) const;
  bool isNum(unsigned frame) const;
  operator bool() const;
  const char* c_str() const;
  bool isValid() const;

private:
  std::string mName = "";
  unsigned mNum = 0;
  STATE mState = STATE_INVALID;
};

template <typename T>
class FakeData : public IFakeData
{
public:
  FakeData();
  FakeData(STATE state);
  FakeData(const std::string &name, unsigned num, STATE state = STATE_INVALID);
  ~FakeData();
  bool operator==(const FakeData &rhs) const;
  bool operator!=(const FakeData &rhs) const;
};

template <typename T>
class FakePool
{
public:
  FakePool();
  FakePool(const std::string &name);
  ~FakePool();
  T request();
  T request(const unsigned num);

private:
  T request(const std::string &name, const unsigned num);

private:
  std::string mName = "unknown";
  unsigned mCounter = 0;
};

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_FAKE_POOL_T_H_
