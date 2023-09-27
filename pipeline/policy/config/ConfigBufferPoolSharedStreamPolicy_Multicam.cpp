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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#define LOG_TAG "mtkcam-ConfigBufferPoolSharedStreamPolicy_Multicam"

#include <set>

#include <mtkcam3/pipeline/policy/IConfigBufferPoolSharedStreamPolicy.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
//
#include "MyUtils.h"
CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);


/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam::v3::pipeline::policy;


/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam::v3::pipeline::policy {

static bool ConfigBufferPoolSharedStream(Configuration_BufferPoolSharedStream_Params const& params)
{
    auto& out = *params.pOut;
    auto const& streamsP1 = *params.pParsedStreamInfo_P1;
    std::map<int32_t, std::set<int32_t>> groupMap; // map<groupId, set of sensor IDs>
    bool needShared = false;
    auto pPipelineStaticInfo = params.pPipelineStaticInfo;

    // find group
    for (auto const& [_sensorId, _groupId] : *(params.pExclusiveGroup))
    {
        MY_LOGD("Sensor %d GroupId : %d", _sensorId, _groupId);
        if (groupMap.find(_groupId) == groupMap.end())
        {
            groupMap[_groupId] = std::set<int32_t>();
        }
        else
        {
            // If here, it mean find the sensors have the same group
            needShared = true;
        }
        groupMap[_groupId].emplace(_sensorId);
    }

    if (needShared)
    {
        for (const auto& [_groupId, _sensorSet] : groupMap)
        {
            if (_sensorSet.size() < 2)
            {
                // only 1, can not shared
                continue;
            }
            std::set<StreamId_T> imgoStreamSet;
            std::set<StreamId_T> rrzoStreamSet;
            std::set<StreamId_T> lcsoStreamSet;
            for (auto sensorId : _sensorSet)
            {
                if (streamsP1.count(sensorId))
                {
                    imgoStreamSet.emplace(streamsP1.at(sensorId).pHalImage_P1_Imgo->getStreamId());
                    rrzoStreamSet.emplace(streamsP1.at(sensorId).pHalImage_P1_Rrzo->getStreamId());
                    lcsoStreamSet.emplace(streamsP1.at(sensorId).pHalImage_P1_Lcso->getStreamId());
                }
            }
            out.push_back(imgoStreamSet);
            out.push_back(rrzoStreamSet);
            out.push_back(lcsoStreamSet);
        }

        if (out.size() == 0)
        {
            MY_LOGE("Can not find sensors which groups are same !");
            return false;
        }
    }

    if (pPipelineStaticInfo->isSeamlessSwitchSupported) {
        MUINT32 main1SensorId = pPipelineStaticInfo->sensorId[0];
        if (streamsP1.count(main1SensorId)) {
            std::set<StreamId_T> imgoStreamSet;
            const auto& main1StreamsP1 = streamsP1.at(main1SensorId);
            imgoStreamSet.emplace(main1StreamsP1.pHalImage_P1_Imgo->getStreamId());
            if(main1StreamsP1.pHalImage_P1_SeamlessImgo != nullptr) {
                imgoStreamSet.emplace(main1StreamsP1.pHalImage_P1_SeamlessImgo->getStreamId());
            }
            out.push_back(imgoStreamSet);
        }
    }
    return true;
}


/**
 * multicam version
 */
FunctionType_Configuration_BufferPoolSharedStream
makePolicy_Configuration_BufferPoolSharedStream_Multicam()
{
    return [](Configuration_BufferPoolSharedStream_Params const& params) -> int {
        auto pOut = params.pOut;
        auto pParsedStreamInfo_P1 = params.pParsedStreamInfo_P1;

        if ( pOut == nullptr || pParsedStreamInfo_P1 == nullptr ) {
            return OK;
        }

        if ( !ConfigBufferPoolSharedStream(params) )
        {
            MY_LOGE("Config buffer pool shared stream fail !");
        }

        return OK;
    };
}


};  //namespace NSCam::v3::pipeline::policy

