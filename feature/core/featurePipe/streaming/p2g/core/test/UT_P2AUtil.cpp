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

#include "UT_P2AUtil.h"
#include "UT_LogUtil.h"

void append(LogString &log, const IFakeData &data, const char* prefix)
{
  if( data.isValid() )
  {
    log.printf("%s=%s,", prefix, data.c_str());
  }
}

void append_if(LogString &log, const P2G::Feature &feature, const P2G::Feature::FID fid)
{
  if( feature.has(fid) )
  {
    log.printf("%s,", toName(fid));
  }
}

void printFeature(const P2G::Feature feature, const char *name, int index)
{
  LogString log("%s[%d]:feature(0x%x: ", name, index, feature);
  append_if(log, feature, P2G::Feature::DUAL);
  append_if(log, feature, P2G::Feature::NR3D);
  append_if(log, feature, P2G::Feature::SMVR_MAIN);
  append_if(log, feature, P2G::Feature::SMVR_SUB);
  log.flush(")");
}

void print(const P2G::IO_InData &data, const char *name, int index)
{
  LogString log("%s[%d]:in(", name, index);
  append(log, data.mBasic,      "pod");
  append(log, data.mIMGI,       "imgi");
  append(log, data.mLCSO,       "lcso");
  append(log, data.mLCSHO,      "lcsho");
  append(log, data.mSyncTun,    "syncTun");
  append(log, data.mAppInMeta,  "appInMeta");
  append(log, data.mHalInMeta,  "halInMeta");
  log.flush(")");
}

void print(const P2G::IO_OutData &data, const char *name, int index)
{
  LogString log("%s[%d]:out(", name, index);
  append(log, data.mFull,       "full");
  append(log, data.mPhy,        "physical");
  append(log, data.mDisplay,    "display");
  append(log, data.mRecord,     "record");
  append(log, data.mFD,         "fd");
  if( data.mExtra.size() )
  {
    log.printf("extra=%zu,", data.mExtra.size());
  }
  for( const ILazy<GImg> &extra : data.mExtra ) { append(log, extra, "extra"); }
  for( const ILazy<GImg> &large : data.mLarge ) { append(log, large, "large"); }
  append(log, data.mAppOutMeta, "appOutMeta");
  append(log, data.mHalOutMeta, "halOutMeta");
  log.flush(")");
}

void print(const P2G::IO_BatchData &data, const char *name, const char *prefix, int index)
{
  LogString log("%s[%d]:%s(", name, index, prefix);
  append(log, data.mTuning,         "tun_buffer");
  append(log, data.mTuningData,     "tun_data");
  append(log, data.mIMGI,           "imgi");
  log.flush(")");
}

void print(const P2G::IO_LoopData &data, const char *name, const char *prefix, int index)
{
  LogString log("%s[%d]:%s(", name, index, prefix);
  append(log, data.mIMG3O,        "img3o");
  append(log, data.mDCE_n_1,      "dce_n1");
  append(log, data.mDCE_n_2,      "dce_n2");
  for( const P2G::IO_LoopData::SubLoopData &subLoop : data.mSub ) { append(log, subLoop.mIMG3O, "sub_img3o"); }
  log.flush(")");
}

void print(const P2G::IO::BatchData &data, const char *name, const char *prefix, int index)
{
  if( data )
  {
    print(*data, name, prefix, index);
  }
  else
  {
    MY_LOGD("%s[%d]:%s = NULL", name, index, prefix);
  }
}

void print(const P2G::IO::BatchData_const &data, const char *name, const char *prefix, int index)
{
  if( data )
  {
    print(*data, name, prefix, index);
  }
  else
  {
    MY_LOGD("%s[%d]:%s = NULL", name, index, prefix);
  }
}

void print(const P2G::IO::LoopData &data, const char *name, const char *prefix, int index)
{
  if( data )
  {
    print(*data, name, prefix, index);
  }
  else
  {
    MY_LOGD("%s[%d]:%s = NULL", name, index, prefix);
  }
}

void print(const P2G::IO::LoopData_const &data, const char *name, const char *prefix, int index)
{
  if( data )
  {
    print(*data, name, prefix, index);
  }
  else
  {
    MY_LOGD("%s[%d]:%s = NULL", name, index, prefix);
  }
}

void print(const P2G::P2SW &path, const char *name, int index)
{
  LogString log("%s[%d]:loopIn(", name, index);
  append(log, path.mBatchIn.mTuningData,  "tun_data");
  log.flush(")");

  log.reset("%s[%d]:in(", name, index);
  append(log, path.mIn.mBasic,            "pod");
  append(log, path.mIn.mAppInMeta,        "app_in");
  append(log, path.mIn.mHalInMeta,        "hal_in");
  append(log, path.mIn.mLCSO,             "lcso");
  append(log, path.mIn.mLCSHO,            "lcsho");
  append(log, path.mIn.mSyncTun,          "sync_tun");
  append(log, path.mIn.mDCE_n_2,          "dce_n_2");
  log.flush(")");

  log.reset("%s[%d]:out(", name, index);
  append(log, path.mOut.mAppOutMeta,      "app_out");
  append(log, path.mOut.mHalOutMeta,      "hal_out");
  append(log, path.mOut.mTuning,          "tun");
  append(log, path.mOut.mTuningData,      "tun_data");
  log.flush(")");
}

void print(const P2G::P2HW &path, const char *name, int index)
{
  LogString log("%s[%d]:in(", name, index);
  append(log, path.mIn.mBasic,            "pod");
  append(log, path.mIn.mVIPI,             "vipi");
  append(log, path.mIn.mIMGI,             "imgi");
  append(log, path.mIn.mTuning,           "tun");
  append(log, path.mIn.mTuningData,       "tun_data");
  append(log, path.mIn.mDSI,              "dsi");
  log.flush(")");

  log.reset("%s[%d]:out(", name, index);
  append(log, path.mOut.mIMG2O,           "img2o");
  append(log, path.mOut.mIMG3O,           "img3o");
  append(log, path.mOut.mWROTO,           "wroto");
  append(log, path.mOut.mWDMAO,           "wdmao");
  append(log, path.mOut.mDCESO,           "dceso");
  log.flush(")");
}

void print(const P2G::PMDP &path, const char *name, int index)
{
  LogString log("%s[%d]:in(", name, index);
  append(log, path.mIn.mFull,            "full");
  append(log, path.mIn.mTuning,          "tun");
  log.flush(")");

  log.reset("%s[%d]:out(", name, index);
  for( const ILazy<GImg> &extra : path.mOut.mExtra ) append(log, extra, "extra");
  log.flush(")");
}

void print(const P2G::MSS &path, const char *name, int index)
{
  LogString log("%s[%d]:in(", name, index);
  append(log, path.mIn.mMSSI,            "mssi");
  log.flush(")");

  log.reset("%s[%d]:out(", name, index);
  for( const ILazy<GImg> &msso : path.mOut.mMSSOs ) append(log, msso, "msso");
  log.flush(")");
}

void print(const P2G::IO &io, const char *name, int index)
{
  printFeature(io.mFeature, name, index);
  print(io.mIn, name, index);
  print(io.mOut, name, index);
  print(io.mBatchIn, name, "batchIn", index);
  print(io.mBatchOut, name, "batchOut", index);
  print(io.mLoopIn, name, "loopIn", index);
  print(io.mLoopOut, name, "loopOut", index);
}

template<typename T>
void print(const std::vector<T> &paths, const char *name)
{
  int index = 0;
  for( const T &path : paths )
  {
    print(path, name, index++);
  }
}

void print(const P2G::IO &io, int index)
{
  print(io, "P2Group", index);
}

void print(const std::vector<P2G::IO> &ioList)
{
  print(ioList, "P2IO");
}

void print(const P2G::Path &path)
{
  print(path.mP2SW, "P2SW");
  print(path.mP2HW, "P2HW");
  print(path.mPMDP, "PMDP");
  print(path.mMSS,  "MSS");
}
