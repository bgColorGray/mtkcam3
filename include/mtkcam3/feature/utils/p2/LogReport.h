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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#ifndef _MTKCAM_FEATURE_UTILS_P2_LOG_REPORT_H_
#define _MTKCAM_FEATURE_UTILS_P2_LOG_REPORT_H_

#include <mtkcam3/feature/utils/p2/P2Pack.h>

#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>

namespace NSCam {
namespace Feature {
namespace P2Util {

class RecorderWrapper
{
public:
    template <typename... Args>
    static void print(const Feature::P2Util::P2Pack &p2Pack, const uint32_t &moduleId,
        bool needHeadline, const char *fmt, Args... args)
    {
        NSCam::TuningUtils::scenariorecorder::ResultParam resultParam;
        const P2SensorData &sensorData = p2Pack.getSensorData();
        resultParam.moduleId = moduleId;
        resultParam.dbgNumInfo.uniquekey = sensorData.mMWUniqueKey;
        resultParam.dbgNumInfo.magicNum = sensorData.mMagic3A;
        resultParam.dbgNumInfo.reqNum = p2Pack.getFrameData().mMWFrameRequestNo;
        resultParam.decisionType = NSCam::TuningUtils::scenariorecorder::DECISION_FEATURE;
        resultParam.sensorId = sensorData.mSensorID;
        resultParam.writeToHeadline = needHeadline;
        WRITE_EXECUTION_RESULT_INTO_FILE(resultParam, fmt, args...);
    }

    static MBOOL getUseScenarioRecorder()
    {
        return NSCam::TuningUtils::scenariorecorder::IScenarioRecorder::getInstance()->isScenarioRecorderOn();
    }
};

} // namespace P2Util
} // namespace Feature
} // namespace NSCam

#endif // _MTKCAM_FEATURE_UTILS_P2_LOG_REPORT_H_
