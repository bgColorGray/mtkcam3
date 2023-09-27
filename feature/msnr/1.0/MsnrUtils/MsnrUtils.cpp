/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#define LOG_TAG "MSNRCore"

#include <mtkcam/utils/std/Log.h> // CAM_LOGD
#include <mtkcam/utils/std/ULog.h>

// STL
#include <memory>
#include <mutex>
#include <chrono>
#include <unordered_map>
// AOSP
#include <utils/String8.h>

#include "MsnrUtils.h"

// ----------------------------------------------------------------------------
// MY_LOG
// ----------------------------------------------------------------------------
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_MSNR);
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)

#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
static const unsigned int MSNR_AEE_DB_FLAGS = DB_OPT_NE_JBT_TRACES | DB_OPT_PROCESS_COREDUMP | DB_OPT_PROC_MEM | DB_OPT_PID_SMAPS |
                                              DB_OPT_LOW_MEMORY_KILLER | DB_OPT_DUMPSYS_PROCSTATS | DB_OPT_FTRACE;

#define MY_LOGA(fmt, arg...)                                                            \
        do {                                                                            \
            android::String8 const str = android::String8::format(fmt, ##arg);          \
            MY_LOGE("ASSERT(%s)", str.c_str());                                         \
            aee_system_exception(LOG_TAG, NULL, MSNR_AEE_DB_FLAGS, str.c_str());        \
            int err = raise(SIGKILL);                                                   \
            if (err != 0) {                                                             \
                MY_LOGE("raise SIGKILL fail! err:%d(%s)", err, ::strerror(-err));       \
            }                                                                           \
        } while(0)
#else
#define MY_LOGA(fmt, arg...)                                                            \
        do {                                                                            \
            android::String8 const str = android::String8::format(fmt, ##arg);          \
            MY_LOGE("ASSERT(%s)", str.c_str());                                         \
        } while(0)
#endif

//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//
#define __DEBUG // enable debug


#ifdef __DEBUG
#include <mtkcam/utils/std/Trace.h>
#define FUNCTION_TRACE()                            CAM_TRACE_CALL()
#define FUNCTION_TRACE_NAME(name)                   CAM_TRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)                  CAM_TRACE_BEGIN(name)
#define FUNCTION_TRACE_END()                        CAM_TRACE_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)    CAM_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)      CAM_TRACE_ASYNC_END(name, cookie)
#else
#define FUNCTION_TRACE()
#define FUNCTION_TRACE_NAME(name)
#define FUNCTION_TRACE_BEGIN(name)
#define FUNCTION_TRACE_END()
#define FUNCTION_TRACE_ASYNC_BEGIN(name, cookie)
#define FUNCTION_TRACE_ASYNC_END(name, cookie)
#endif

using namespace android;

//-----------------------------------------------------------------------------
// static variable definition
//-----------------------------------------------------------------------------

TuningSyncHelper::HelperMap TuningSyncHelper::sHelperMap;

//-----------------------------------------------------------------------------
// TuningSyncHelper
//-----------------------------------------------------------------------------

TuningSyncHelper::HelperPtr
TuningSyncHelper::
getInstance(std::string username)
{
    auto it = sHelperMap.find(username);
    if (it != sHelperMap.end()) {
        MY_LOGD("found %s", username.c_str());
        return it->second;
    } else {
        MY_LOGD("create new:%s", username.c_str());
        sHelperMap.emplace(username, std::make_shared<TuningSyncHelper>());

        auto newit = sHelperMap.find(username);
        if (CC_UNLIKELY(newit == sHelperMap.end())) {
            MY_LOGA("fail to create new helper:%s", username.c_str());
            return nullptr;
        }
        return newit->second;
    }
}

bool
TuningSyncHelper::
generateKey(const IMetadata &pHalMeta, std::string *outKey)
{
    int uniqueKey = 0;
    if (!IMetadata::getEntry<MINT32>(&pHalMeta, MTK_PIPELINE_UNIQUE_KEY, uniqueKey)) {
        MY_LOGE("fail to get unique key");
        return false;
    }

    int requestNum = 0;
    if (!IMetadata::getEntry<MINT32>(&pHalMeta, MTK_PIPELINE_REQUEST_NUMBER, requestNum)) {
        MY_LOGE("fail to get requet number");
        return false;
    }

    *outKey = std::to_string(uniqueKey) + "-" + std::to_string(requestNum);
    return true;
}

bool
TuningSyncHelper::
createDualSyncInfo(const std::string &key, const int size, DataPtr *outptr)
{
    std::lock_guard<std::mutex> __l(mDualSyncInfoLock);
    // to check if the item exists
    for (auto &&existInfo : mDualSyncInfoList) {
        if (existInfo.first == key) {
            MY_LOGW("item(%s, %p) is already existed", key.c_str(), existInfo.second.get());
            *outptr = existInfo.second;
            return true;
        }
    }
    // before adding new item, chekc if size exceeds MAX_DUAL_SYNC_COUNT
    while (mDualSyncInfoList.size() >= MAX_DUAL_SYNC_COUNT) {
      // remove oldest item to keep size less than max count
      auto back = mDualSyncInfoList.back();
      MY_LOGD("remove:(%s,%p)", back.first.c_str(), back.second.get());
      mDualSyncInfoList.pop_back();
    }
    // add new item
    DataPtr temp = std::shared_ptr<char>(new char[size]{0}, std::default_delete<char[]>());
    mDualSyncInfoList.push_front(std::make_pair(key, temp));
    MY_LOGD("add item:(%s,%p)", key.c_str(), temp.get());
    *outptr = temp;

    return true;
}

bool
TuningSyncHelper::
getDualSyncInfo(const std::string &key, DataPtr *outptr)
{
    std::lock_guard<std::mutex> __l(mDualSyncInfoLock);
    bool found = false;
    for (auto &&dualinfo : mDualSyncInfoList) {
        if (dualinfo.first == key) {
            MY_LOGD("found syncinfo:(%s, %p)", dualinfo.first.c_str(), dualinfo.second.get());
            *outptr = dualinfo.second;
            found = true;
            break;
        }
    }

    if (!found) {
        MY_LOGE("key: %s not found!", key.c_str());
        return false;
    }
    return true;
}

const enum MsnrMode
TuningSyncHelper::
setModeSyncInfo(const std::string &key, const enum MsnrMode &mode)
{
    std::lock_guard<std::mutex> __l(mModeSyncInfoLock);
    // to check if the item exists
    for (auto &&existmode : mModeSyncList) {
        if (existmode.first == key) {
            MY_LOGI("key:(%s) existed, old mode:(%d) new mode:(%d)",
                     key.c_str(), existmode.second, mode);
            existmode.second = mode;
            return mode;
        }
    }

    // before adding new item, chekc if size exceeds MAX_MODE_SYNC_COUNT
    while (mModeSyncList.size() >= MAX_MODE_SYNC_COUNT) {
        // remove oldest item to keep size less than max count
        auto back = mModeSyncList.back();
        MY_LOGD("remove:(%s,%d)", back.first.c_str(), back.second);
        mModeSyncList.pop_back();
    }
    // add new item
    mModeSyncList.push_front(std::make_pair(key, mode));
    return mode;
}

bool
TuningSyncHelper::
getModeSyncInfo(const std::string &key, enum MsnrMode *mode)
{
    std::lock_guard<std::mutex> __l(mModeSyncInfoLock);
    bool found = false;
    for (auto &&syncmode : mModeSyncList) {
        if (syncmode.first == key) {
            MY_LOGD("found mode:(%s, %d)", syncmode.first.c_str(), syncmode.second);
            *mode = syncmode.second;
            found = true;
            break;
        }
    }

    if (!found) {
        MY_LOGE("key: %s not found!", key.c_str());
        return false;
    }
    return true;
}

auto
RecorderUtils::
initDecisionParam(const MINT32 &openId, const IMetadata &halMeta, DecisionParam *decParm)
-> void
{
  MINT32 UniqueKey = 0;
  if (!IMetadata::getEntry<MINT32>(&halMeta, MTK_PIPELINE_UNIQUE_KEY, UniqueKey)) {
      MY_LOGW("cannot get UniqueKey");
  }

  MINT32 RequestNo = 0;
  if (!IMetadata::getEntry<MINT32>(&halMeta, MTK_PIPELINE_REQUEST_NUMBER, RequestNo)) {
      MY_LOGW("cannot get RequestNo");
  }

  MINT32 MagicNo = 0;
  if (!IMetadata::getEntry<MINT32>(&halMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, MagicNo)) {
      MY_LOGW("cannot get MagicNo");
  }

  DebugSerialNumInfo &dbgNumInfo = decParm->dbgNumInfo;
  dbgNumInfo.uniquekey = UniqueKey;
  dbgNumInfo.reqNum = RequestNo;
  dbgNumInfo.magicNum = MagicNo;
  decParm->sensorId = openId;
  decParm->decisionType = DECISION_FEATURE;
  decParm->moduleId = NSCam::Utils::ULog::MOD_LIB_MSNR;
  decParm->isCapture = true;

  return;
}

auto
RecorderUtils::
initExecutionParam(const MINT32 openId, const DecisionType decisionType,
    const DebugSerialNumInfo &dbgNumInfo, ResultParam *resParm)
-> void
{
  resParm->dbgNumInfo = dbgNumInfo;
  resParm->sensorId = openId;
  resParm->decisionType = decisionType;
  resParm->moduleId = NSCam::Utils::ULog::MOD_LIB_MSNR;
  resParm->isCapture = true;
}

auto
RecorderUtils::
toOrdinal(int number)
-> std::string
{
  auto num = std::to_string(number);
  switch (number) {
    case 1:
      return (num + "st");
    break;
    case 2:
      return (num + "nd");
    break;
    case 3:
      return (num + "rd");
    break;
    default:
      return (num + "th");
    break;
  }
  // abnormal case
  return num;
}