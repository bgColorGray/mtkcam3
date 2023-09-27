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

#include "UT_P2AValidate.h"
#include "UT_LogUtil.h"

bool compare(LogString &log, const IFakeData &vldData, const IFakeData &ansData, const char* prefix)
{
  bool ret = false;
  std::string result;
  if(vldData == ansData)
  {
    ret = true;
    result = "Pass";
  }
  else
  {
    ret = false;
    result = "Fail";
  }
  if(vldData.isValid()||ansData.isValid())
    log.printf("\n%s: %s %s[%s] vs %s[%s]", result.c_str(), prefix,
                vldData.isValid() ? "" : "(InValid)", vldData.c_str(),
                ansData.isValid() ? "" : "(InValid)", ansData.c_str());
  return ret;
}

#define COMP_LOOP_IN(data, prefix) compare(log, vldPath.mBatchIn.data, ansPath.mBatchIn.data,prefix)
#define COMP_IN(data, prefix) compare(log, vldPath.mIn.data, ansPath.mIn.data,prefix)
#define COMP_OUT(data, prefix) compare(log, vldPath.mOut.data, ansPath.mOut.data,prefix)

bool validate(const P2G::P2SW &vldPath, const P2G::P2SW &ansPath, const char *name, int index)
{
  bool ret = true;
  LogString log("%s[subF#%2d]:loopIn(", name, index+1);
  ret = COMP_LOOP_IN(mTuningData, "tun_data") && ret;
  log.flush(")");

  log.reset("%s[subF#%2d]:in(", name, index+1);
  ret = COMP_IN(mBasic,     "pod")      && ret;
  ret = COMP_IN(mAppInMeta, "app_in")   && ret;
  ret = COMP_IN(mHalInMeta, "hal_in")   && ret;
  ret = COMP_IN(mLCSO,      "lcso")     && ret;
  ret = COMP_IN(mLCSHO,     "lcsho")    && ret;
  ret = COMP_IN(mSyncTun,   "sync_tun") && ret;
  ret = COMP_IN(mDCE_n_2,   "dce_n_2")  && ret;
  log.flush(")");

  log.reset("%s[subF#%2d]:out(", name, index+1);
  ret = COMP_OUT(mAppOutMeta, "app_out")  && ret;
  ret = COMP_OUT(mHalOutMeta, "hal_out")  && ret;
  ret = COMP_OUT(mTuning,     "tun")      && ret;
  ret = COMP_OUT(mTuningData, "tun_data") && ret;
  log.flush(")\n%s: subF#%2d %s", name, index+1, ret ? "Pass": "Fail");
  return ret;
}

bool validate(const P2G::P2HW &vldPath, const P2G::P2HW &ansPath, const char *name, int index)
{
  bool ret = true;
  LogString log("%s[subF#%2d]:in(", name, index+1);
  ret = COMP_IN(mBasic,      "pod")      && ret;
  ret = COMP_IN(mVIPI,       "vipi")     && ret;
  ret = COMP_IN(mIMGI,       "imgi")     && ret;
  ret = COMP_IN(mTuning,     "tun")      && ret;
  ret = COMP_IN(mTuningData, "tun_data") && ret;
  ret = COMP_IN(mDSI,        "dsi")      && ret;
  log.flush(")");

  log.reset("%s[subF#%2d]:out(", name, index+1);
  ret = COMP_OUT(mIMG2O, "img2o") && ret;
  ret = COMP_OUT(mIMG3O, "img3o") && ret;
  ret = COMP_OUT(mWROTO, "wroto") && ret;
  ret = COMP_OUT(mWDMAO, "wdmao") && ret;
  ret = COMP_OUT(mDCESO, "dceso") && ret;
  log.flush(")\n%s: subF#%2d %s", name, index+1, ret ? "Pass": "Fail");
  return ret;
}

bool validate(const P2G::PMDP &vldPath, const P2G::PMDP &ansPath, const char *name, int index)
{
  bool ret = true;
  LogString log("%s[subF#%2d]:in(", name, index+1);
  ret = COMP_IN(mFull,   "full") && ret;
  ret = COMP_IN(mTuning, "tun")  && ret;
  log.flush(")");

  int vExtraSize = vldPath.mOut.mExtra.size(), aExtraSize = ansPath.mOut.mExtra.size();
  if( vExtraSize != aExtraSize )
  {
    ret = false;
    MY_LOGE("%s: Path Extra Size(%d) != answer Path Extra Size(%d)", name, vExtraSize, aExtraSize);
  }
  log.reset("%s[subF#%2d]:out(", name, index+1);
  for( unsigned i = 0 ; i < vExtraSize && i < aExtraSize ; ++i )
  {
    ret = compare(log, vldPath.mOut.mExtra[i], ansPath.mOut.mExtra[i], "extra") && ret;
  }
  log.flush(")\n%s: subF#%2d %s", name, index+1, ret ? "Pass": "Fail");
  return ret;
}

bool validate(const P2G::MSS &vldPath, const P2G::MSS &ansPath, const char *name, int index)
{
  bool ret = true;
  LogString log("%s[subF#%2d]:in(", name, index+1);
  ret = COMP_IN(mMSSI,      "mssi")      && ret;
  log.flush(")");

  int vMssoSize = vldPath.mOut.mMSSOs.size(), aMssoSize = ansPath.mOut.mMSSOs.size();
  if( vMssoSize != aMssoSize )
  {
    ret = false;
    MY_LOGE("%s: Path MSSO Size(%d) != answer Path MSSO Size(%d)", name, vMssoSize, aMssoSize);
  }
  log.reset("%s[subF#%2d]:out(", name, index+1);
  for( unsigned i = 0 ; i < vMssoSize && i < aMssoSize ; ++i )
  {
    ret = compare(log, vldPath.mOut.mMSSOs[i], ansPath.mOut.mMSSOs[i], "msso") && ret;
  }
  log.flush(")\n%s: subF#%2d %s", name, index+1, ret ? "Pass": "Fail");
  return ret;
}

template<typename T>
bool validate(const std::vector<T> &vldPaths, const std::vector<T> &ansPaths, const char *name)
{
  int index = 0;
  bool ret = true;
  int vPathSize = vldPaths.size(), aPathSize = ansPaths.size();
  if(vPathSize != aPathSize)
  {
    ret = false;
    MY_LOGE("%s: PathSize(%d) != answer PathSize(%d)", name, vPathSize, aPathSize);
  }
  for( uint32_t i = 0 ; i < vPathSize && i < aPathSize ; ++i )
  {
    ret = validate(vldPaths[i], ansPaths[i], name, index++) && ret;
  }
  MY_LOGD("%s: %s", name, ret ? "Pass": "Fail");
  return ret;
}

bool validate(const P2G::Path &vldPath, const P2G::Path &ansPath)
{
  bool ret = true;
  MY_LOGD("------------ validate P2SW ------------");
  ret = validate(vldPath.mP2SW, ansPath.mP2SW, "P2SW") && ret;
  MY_LOGD("------------ validate P2HW ------------");
  ret = validate(vldPath.mP2HW, ansPath.mP2HW, "P2HW") && ret;
  MY_LOGD("------------ validate PMDP ------------");
  ret = validate(vldPath.mPMDP, ansPath.mPMDP, "PMDP") && ret;
  MY_LOGD("------------ validate MSS -------------");
  ret = validate(vldPath.mMSS, ansPath.mMSS, "MSS") && ret;
  MY_LOGD("---------------------------------------");
  return ret;
}


#define HIT_LOOP_IN(pathData) do { if(ioData == vldPath.mBatchIn.pathData) hitCount++; } while(0)
#define HIT_IN(pathData) do { if(ioData == vldPath.mIn.pathData) hitCount++; } while(0)
#define HIT_OUT(pathData) do { if(ioData == vldPath.mOut.pathData) hitCount++; } while(0)

unsigned hitInPath(const IFakeData &ioData, const P2G::P2SW &vldPath)
{
    unsigned hitCount = 0;
    if(ioData.isValid())
    {
        HIT_LOOP_IN(mTuningData);

        HIT_IN(mBasic);
        HIT_IN(mAppInMeta);
        HIT_IN(mHalInMeta);
        HIT_IN(mLCSO);
        HIT_IN(mLCSHO);
        HIT_IN(mSyncTun);
        HIT_IN(mDCE_n_2);

        HIT_OUT(mAppOutMeta);
        HIT_OUT(mHalOutMeta);
        HIT_OUT(mTuning);
        HIT_OUT(mTuningData);
    }
    return hitCount;
}

unsigned hitInPath(const IFakeData &ioData, const P2G::P2HW &vldPath)
{
    unsigned hitCount = 0;
    if(ioData.isValid())
    {
        HIT_IN(mBasic);
        HIT_IN(mVIPI);
        HIT_IN(mIMGI);
        HIT_IN(mTuning);
        HIT_IN(mTuningData);

        HIT_OUT(mIMG2O);
        HIT_OUT(mIMG3O);
        HIT_OUT(mWROTO);
        HIT_OUT(mWDMAO);
        HIT_OUT(mDCESO);
    }
    return hitCount;
}

unsigned hitInPath(const IFakeData &ioData, const P2G::PMDP &vldPath)
{
    unsigned hitCount = 0;
    if(ioData.isValid())
    {
        //HIT_IN(mFull);
        //HIT_IN(mTuning);

        for( const ILazy<GImg> &extra : vldPath.mOut.mExtra ) { if(ioData == extra) hitCount++; }
    }
    return hitCount;
}

bool isHitPath(const IFakeData &ioData, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    unsigned hitCount = 0;
    for( const P2G::P2SW &sw : vldPath.mP2SW ) hitCount += hitInPath(ioData, sw);
    for( const P2G::P2HW &hw : vldPath.mP2HW ) hitCount += hitInPath(ioData, hw);
    for( const P2G::PMDP &pmdp : vldPath.mPMDP ) hitCount += hitInPath(ioData, pmdp);

    MY_LOGD("io index[%d]: ioData: %s, hitCount: %d", index, ioData.c_str(), hitCount);

    return checkOnce ? (hitCount == 1) : (hitCount > 0);
}

bool checkDataInPath(const IFakeData &ioData, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    return ioData.isValid() ? isHitPath(ioData, vldPath, index, checkOnce) : true;
}

bool checkIoPath(const P2G::IO_InData &data, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    bool ret = true;
    ret = checkDataInPath(data.mBasic,      vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mIMGI,       vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mLCSO,       vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mLCSHO,      vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mSyncTun,    vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mAppInMeta,  vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mHalInMeta,  vldPath, index, checkOnce) && ret;
    return ret;
}

bool checkIoPath(const P2G::IO_OutData &data, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    bool ret = true;
    ret = checkDataInPath(data.mFull,       vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mDisplay,    vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mRecord,     vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mFD,         vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mAppOutMeta, vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mHalOutMeta, vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mPhy,        vldPath, index, checkOnce) && ret;
    for( const ILazy<GImg> &extra : data.mExtra ) { ret = checkDataInPath(extra, vldPath, index, checkOnce) && ret; }
    for( const ILazy<GImg> &large : data.mLarge ) { ret = checkDataInPath(large, vldPath, index, checkOnce) && ret; }
    return ret;
}

bool checkIoPath(const P2G::IO_BatchData &data, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    bool ret = true;
    ret = checkDataInPath(data.mTuning,     vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mTuningData, vldPath, index, checkOnce) && ret;
    ret = checkDataInPath(data.mIMGI,       vldPath, index, checkOnce) && ret;
    return ret;
}

bool checkIoPath(const P2G::IO_LoopData &data, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    bool ret = true;
    ret = checkDataInPath(data.mIMG3O,      vldPath, index, checkOnce) && ret;
    for( const P2G::IO_LoopData::SubLoopData &subData : data.mSub ) { ret = checkDataInPath(subData.mIMG3O, vldPath, index, checkOnce) && ret; }
    return ret;
}

bool checkDceLoopIoPath(const P2G::IO &io, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    bool ret = true;
    if(io.mLoopIn)  ret = checkDataInPath(io.mLoopIn->mDCE_n_2,    vldPath, index, checkOnce) && ret;
    if(io.mLoopOut) ret = checkDataInPath(io.mLoopOut->mDCE_n_1,   vldPath, index, checkOnce) && ret;
    return ret;
}

bool checkIoPath(const P2G::IO &io, const P2G::Path &vldPath, unsigned index, bool checkOnce)
{
    bool ret = true;
    ret = checkIoPath(io.mIn,  vldPath, index, checkOnce) && ret;
    ret = checkIoPath(io.mOut, vldPath, index, checkOnce) && ret;

    if(io.mBatchIn)  ret = checkIoPath(*io.mBatchIn,  vldPath, index, checkOnce) && ret;
    if(io.mBatchOut) ret = checkIoPath(*io.mBatchOut, vldPath, index, checkOnce) && ret;
    if(io.mLoopIn)   ret = checkIoPath(*io.mLoopIn,   vldPath, index, checkOnce) && ret;
    if(io.mLoopOut)  ret = checkIoPath(*io.mLoopOut,  vldPath, index, checkOnce) && ret;

    ret = checkDceLoopIoPath(io, vldPath, index, checkOnce) && ret;

    return ret;
}

bool validateBasicRule(const P2G::Path &vldPath, const std::vector<P2G::IO> &ioList)
{
    MY_LOGD("<<<<validateBasicRule>>>> start");
    bool ret = true;
    bool checkOnce = true;
    unsigned index = 0;
    if( ioList[0].mFeature.hasDSDN25() )  checkOnce = false;
    for( const P2G::IO &io : ioList )
    {
        ret = checkIoPath(io, vldPath, index++, checkOnce) && ret;
    }
    MY_LOGD("<<<<validateBasicRule>>>> ret: %s", ret ? "pass" : "fail");
    return ret;
}

#define IS_NUM_LOOP_IN(pathData, frameNum) (vldPath.mBatchIn.pathData.isNum(frameNum))
#define IS_NUM_IN(pathData, frameNum) (vldPath.mIn.pathData.isNum(frameNum))
#define IS_NUM_OUT(pathData, frameNum) (vldPath.mOut.pathData.isNum(frameNum))

bool checkDualDataNumber(const P2G::P2SW &vldPath, unsigned frame, bool isMaster)
{
    bool ret = true;

    unsigned wkBufN = isMaster ? (frame * 2 - 1) : (frame * 2);

    ret = IS_NUM_LOOP_IN(mTuningData, frame) && ret;

    ret = IS_NUM_IN(mBasic, frame) && ret;
    ret = IS_NUM_IN(mAppInMeta, frame) && ret;
    ret = IS_NUM_IN(mHalInMeta, frame) && ret;
    ret = IS_NUM_IN(mLCSO, frame) && ret;
    ret = IS_NUM_IN(mLCSHO, frame) && ret;
    ret = IS_NUM_IN(mSyncTun, frame) && ret;

    ret = IS_NUM_OUT(mAppOutMeta, frame) && ret;
    ret = IS_NUM_OUT(mHalOutMeta, frame) && ret;
    ret = IS_NUM_OUT(mTuning, wkBufN) && ret;
    ret = IS_NUM_OUT(mTuningData, wkBufN) && ret;

    return ret;
}

bool checkDualDataNumber(const P2G::P2HW &vldPath, unsigned frame, bool isMaster)
{
    bool ret = true;

    unsigned wkBufN = isMaster ? (frame * 2 - 1) : (frame * 2);

    ret = IS_NUM_IN(mBasic, frame) && ret;
    ret = IS_NUM_IN(mVIPI, frame) && ret;
    ret = IS_NUM_IN(mIMGI, frame) && ret;
    ret = IS_NUM_IN(mTuning, wkBufN) && ret;
    ret = IS_NUM_IN(mTuningData, wkBufN) && ret;

    ret = IS_NUM_OUT(mIMG2O, frame) && ret;
    ret = IS_NUM_OUT(mIMG3O, frame) && ret;
    ret = IS_NUM_OUT(mWROTO, frame) && ret;
    ret = IS_NUM_OUT(mWDMAO, frame) && ret;

    return ret;
}

bool validateDualRule(const P2G::Path &vldPath, unsigned frame, unsigned index, bool isMaster)
{
    bool ret = true;
    ret = checkDualDataNumber(vldPath.mP2SW[index], frame, isMaster) && ret;
    ret = checkDualDataNumber(vldPath.mP2HW[index], frame, isMaster) && ret;
    return ret;
}

bool validateFeatureRule(const P2G::Path &vldPath, const std::vector<P2G::IO> &ioList, unsigned frame)
{
    MY_LOGD("<<<<validateFeatureRule>>>> start");
    bool ret = true;
    int index = 0;
    for( const P2G::IO &io : ioList )
    {
        MY_LOGD("index(%d), scene(%d) isMaster(%d) ,feature DUAL(%d), SMVR_M(%d), SMVR_S(%d), NR3D(%d), DCE(%d), TIMGO(%d), DSDN25(%d)",
            index, io.mScene, isMaster(io.mScene), io.mFeature.hasDUAL(), io.mFeature.hasSMVR_MAIN(), io.mFeature.hasSMVR_SUB(),
            io.mFeature.hasNR3D(), io.mFeature.hasDCE(), io.mFeature.hasTIMGO(), io.mFeature.hasDSDN25());

        if( ioList[0].mFeature.hasDUAL() )
            ret = validateDualRule(vldPath, frame, index, isMaster(io.mScene)) && ret;
        if( ioList[0].mFeature.hasDSDN25() )
            MY_LOGW("TODO: DSDN25 path logical test");
        index++;
    }
    MY_LOGD("<<<<validateFeatureRule>>>> ret: %s", ret ? "pass" : "fail");
    return ret;

}

bool validateLogicalRule(const P2G::Path &vldPath, const std::vector<P2G::IO> &ioList, unsigned frame)
{
  bool ret = true;
  MY_LOGD("------------ validateLogicalRule ------------");
  // validate basic rule
  if( !ioList[0].mFeature.hasSMVR_MAIN() )
      ret = validateBasicRule(vldPath, ioList) && ret;
  // validate feature rule
  validateFeatureRule(vldPath, ioList, frame);
  MY_LOGD("---------------------------------------------");
  return ret;
}

