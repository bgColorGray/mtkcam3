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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#include "NextIOUtil.h"

#include <numeric>

#define PIPE_CLASS_TAG "NextIO"
#define PIPE_TRACE TRACE_X_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

NextIO::NextBuf getFirst(const NextIO::NextReq &req)
{
    NextIO::NextBuf img;
    return req.mList.size() ? req.mList.front().mImg : NextIO::NextBuf();
}

NextImgMap initNextMap(const NextIO::NextReq &req)
{
    NextImgMap map;
    for( const auto &target_pair : req.mMap )
    {
        // TODO: check target
        for( const auto &id_pair : target_pair.second )
        {
            NextIO::NextID id = id_pair.first;
            //map[id].mImg = id_pair.second.mImg;
            map[id].mAttr = id_pair.second.mAttr;
        }
    }
    return map;
}

MVOID updateFirst(NextImgMap &map, const NextIO::NextReq &req, const BasicImg &img)
{
    if( req.mList.size() )
    {
        updateNextMap(map, img, *req.mList.begin());
    }
}

MVOID addMaster(NextResult &result, MUINT32 sensorID, const NextImgMap &map)
{
    result.mMasterID = sensorID;
    result.mMap[sensorID] = map;
}

MVOID addSlave(NextResult &result, MUINT32 sensorID, const NextImgMap &map)
{
    result.mSlaveID = sensorID;
    result.mMap[sensorID] = map;
}

MVOID print(const char *prefix, const NextResult &result)
{
    MY_LOGD("<%s> map(%zu) m/s=%d/%d",
            prefix, result.mMap.size(), result.mMasterID, result.mSlaveID);
    for( const auto &sensor_map : result.mMap )
    {
        MUINT32 sid = sensor_map.first;
        for( const auto &img_map : sensor_map.second )
        {
            const auto &id = img_map.first;
            const auto &img = img_map.second.mImg;
            const auto &attr = img_map.second.mAttr;
            MY_LOGD("<%s> [%d] id(%s)=%p attr(%s)",
                    prefix, sid, id.toStr().c_str(),
                    img.getImageBufferPtr(), attr.toStr().c_str());
        }
    }
}

MVOID print(const char *prefix, const NextIO::NextID &id, const NextImg &img)
{
    MY_LOGD("<%s> [%s]=%d attr=%s",
            prefix, id.toStr().c_str(), img.isValid(),
            img.mAttr.toStr().c_str());
}

MVOID print(const char *prefix, const NextIO::NextReq &req)
{
    //print(req.mMap);
    //print(req.mList);
    size_t total = 0;
    total = std::accumulate(req.mMap.begin(), req.mMap.end(), (size_t)0,
            [](size_t n, const decltype(req.mMap)::value_type &m)
            { return m.second.size() + n; } );

    MY_LOGD("<%s> list=%zu map=%zu/%zu",
            prefix, req.mList.size(), req.mMap.size(), total);

    for( const auto &output : req.mList )
    {
        MY_LOGD("<%s> id=%s img=%p attr=%s user(%zu)",
                prefix, output.mID.toStr().c_str(),
                output.mImg.mBuffer.get(),
                output.mImg.mAttr.toStr().c_str(),
                output.mUsers.size());
    }
}

MVOID print(const char *prefix, const NextIO::NextBuf &buf)
{
    MY_LOGD("<%s> buffer(%p/%p) attr=%s",
            prefix, buf.mBuffer.get(),
            buf.mBuffer ? buf.mBuffer->getImageBufferPtr() : NULL,
            buf.mAttr.toStr().c_str());
}

MVOID print(const char *prefix, const NextImgMap &map)
{
    for( const auto &id_pair : map )
    {
        const auto &id = id_pair.first;
        const auto &img = id_pair.second;
        MY_LOGD("<%s> id=%s img(%p)=%s",
                prefix, id.toStr().c_str(),
                img.mImg.getImageBufferPtr(), img.mAttr.toStr().c_str());
    }
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
