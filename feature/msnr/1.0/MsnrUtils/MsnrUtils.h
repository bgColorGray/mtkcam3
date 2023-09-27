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

#ifndef __MSNRUTILS_H__
#define __MSNRUTILS_H__

// Msnr Core Lib
#include <mtkcam3/feature/msnr/IMsnr.h>
// STL
#include <memory>
#include <mutex>
#include <chrono>
#include <list>
#include <algorithm>
#include <unordered_map>

#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>

using NSCam::TuningUtils::scenariorecorder::DecisionParam;
using NSCam::TuningUtils::scenariorecorder::ResultParam;
using NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo;
using NSCam::TuningUtils::scenariorecorder::DecisionType;
using NSCam::TuningUtils::scenariorecorder::DecisionType::DECISION_FEATURE;
using NSCam::TuningUtils::scenariorecorder::DecisionType::DECISION_ATMS;
using NSCam::TuningUtils::scenariorecorder::DecisionType::DECISION_ISP_FLOW;

#define IF_WRITE_DECISION_RECORD_INTO_FILE(cond, _Param, type, _fmt, _args...) \
  do {                                                                         \
    if ((cond)) {                                                              \
      auto cpy_Param = _Param;                                                 \
      cpy_Param.decisionType = type;                                           \
      WRITE_DECISION_RECORD_INTO_FILE(cpy_Param, _fmt, ##_args);               \
    }                                                                          \
  } while (0)

class Pass2Async final
{
public:
    Pass2Async() = default;
    ~Pass2Async() = default;
    int index = -1;
public:
    inline std::unique_lock<std::mutex> uniqueLock()
    {
        return std::unique_lock<std::mutex>(mx);
    }
    inline std::mutex& getLocker() { return mx; }
    inline void lock()      { mx.lock(); }
    inline void unlock()    { mx.unlock(); }
    inline void notifyOne()  { cv.notify_one(); }
    inline void notifyAll()  { cv.notify_all(); }
    inline void wait(std::unique_lock<std::mutex>&& l) { cv.wait(l); }
    inline std::cv_status wait_for(std::unique_lock<std::mutex>&& l, int timeout) { return cv.wait_for(l, std::chrono::milliseconds(timeout)); }
    inline void setStatus(bool Status) { status.store(Status, std::memory_order_relaxed); }
    inline bool getStatus() const { return status.load(std::memory_order_relaxed); }
private:
    // for thread sync
    std::mutex              mx;
    std::condition_variable cv;
    std::atomic<bool>       status = false;
};

class TuningSyncHelper
{
public:
    typedef std::shared_ptr<TuningSyncHelper> HelperPtr;
    typedef std::unordered_map<std::string , HelperPtr> HelperMap;
    typedef std::shared_ptr<char> DataPtr;
    typedef std::list<std::pair<std::string, DataPtr> > DualSyncInfoList;
    typedef std::list<std::pair<std::string, enum MsnrMode> > ModeSyncList;

public:
    static HelperPtr getInstance(std::string username);
    static bool generateKey(const IMetadata &pHalMeta, std::string *outKey);

public:
    TuningSyncHelper() = default;
    ~TuningSyncHelper() = default;

public:
    /*dualsyncinfo API*/
    //create a new dualsyncinfo and share it
    bool createDualSyncInfo(const std::string &key, const int size, DataPtr *outptr);
    //share existed dualsyncinfo
    bool getDualSyncInfo(const std::string &key, DataPtr *outptr);

    /*modesyncinfo API*/
    //create a new modesyncinfo and store it.
    // if the size reaches max count, it will first remove the oldest item.
    const enum MsnrMode setModeSyncInfo(const std::string &key, const enum MsnrMode &mode);
    //get existed modesyncinfo
    bool getModeSyncInfo(const std::string &key, enum MsnrMode *mode);

public:
    static HelperMap sHelperMap;

private:
    /*dualsyncinfo member*/
    std::mutex mDualSyncInfoLock;
    const int MAX_DUAL_SYNC_COUNT = 10;
    DualSyncInfoList mDualSyncInfoList;
    /*modesyncinfo member*/
    std::mutex mModeSyncInfoLock;
    const int MAX_MODE_SYNC_COUNT = 10;
    ModeSyncList mModeSyncList;
};

namespace RecorderUtils
{
  auto initDecisionParam(const MINT32 &openId, const IMetadata &halMeta, DecisionParam *decParm)-> void;
  auto initExecutionParam(const MINT32 openId, const DecisionType decisionType,
      const DebugSerialNumInfo &dbgNumInfo, ResultParam *resParm)-> void;
  auto toOrdinal(int number)-> std::string;
};

using RecorderUtils::initDecisionParam;
using RecorderUtils::initExecutionParam;
using RecorderUtils::toOrdinal;

#define ORDINAL(number) toOrdinal(number).c_str()

#endif //__MSNRUTILS_H__