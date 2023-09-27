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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_FAKE_POOL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_FAKE_POOL_H_

#include "UT_FakePool_t.h"

#define MAX_NAME_LEN 16

template<typename T>
FakeData<T>::FakeData()
{
}

template<typename T>
FakeData<T>::FakeData(STATE state)
  : IFakeData(state)
{
}

template<typename T>
FakeData<T>::FakeData(const std::string &name, unsigned num, STATE state)
  : IFakeData(name, num, state)
{
}

template<typename T>
FakeData<T>::~FakeData()
{
}

template<typename T>
bool FakeData<T>::operator==(const FakeData &rhs) const
{
  return IFakeData::operator==(rhs);
}

template<typename T>
bool FakeData<T>::operator!=(const FakeData &rhs) const
{
  return IFakeData::operator!=(rhs);
}

template<typename T>
FakePool<T>::FakePool()
{
}

template<typename T>
FakePool<T>::FakePool(const std::string &name)
  : mName(name)
{
}

template<typename T>
FakePool<T>::~FakePool()
{
}

template<typename T>
T FakePool<T>::request()
{
  return request(mName, ++mCounter);
}

template<typename T>
T FakePool<T>::request(const unsigned num)
{
  return request(mName, num);
}

template<typename T>
T FakePool<T>::request(const std::string &name, const unsigned num)
{
  char dataName[MAX_NAME_LEN] = { 0 };
  snprintf(dataName, sizeof(dataName) - 1, "%s#%d", name.c_str(), num);
  return T(dataName, num, IFakeData::STATE_VALID);
}

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2A_GROUP_FAKE_POOL_H_
