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

#define LOG_TAG "MtkCam/MasterCamSyncDataUpdater"
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

#include "MasterCamSyncDataUpdater.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_P1_SYNCHELPER);

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%4d) [%s] " fmt, __LINE__, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

using namespace NSCam::v3::Utils::Imp;

#define ENABLE_ISP_QUALITY_CHECK (1)
/******************************************************************************
 *
 ******************************************************************************/
MasterCamSyncDataUpdater::
MasterCamSyncDataUpdater(
    uint64_t timestamp
) : ISyncDataUpdater(timestamp)
{
    mp3rdReader = NSCam::IGenericContainer<std::shared_ptr<Customer_FOVA_Streaming_Data>>::makeReader(
                                                            LOG_TAG,
                                                            mTimestamp,
                                                            (uint32_t)IGenericContainerKey::VENDOR_FOVA);
}
/******************************************************************************
 *
 ******************************************************************************/
bool
MasterCamSyncDataUpdater::
update(
    SyncData &data
)
{
    MY_LOGA_IF((data.mSyncTarget.size() == 0), "Invalid operation.");
    auto updateMasterCamId = [&data](MINT32 masterCamId){
        for(auto&& item:data.mvOutputParams) {
            // for timestamp debug.
            // only master cam can set true.
            if(item.first == masterCamId) {
                item.second->bMasterCam = true;
            }
            else {
                item.second->bMasterCam = false;
            }
            IMetadata::setEntry<MINT32>(
                                        item.second->HalDynamic,
                                        MTK_DUALZOOM_REAL_MASTER,
                                        masterCamId);
        }
        MY_LOGI("master id(%d) f(%d)", masterCamId, data.mFrameNo);
    };
    auto updateFDCamId = [&data](MINT32 fdCamId){
        for(auto&& item:data.mvOutputParams) {
            IMetadata::setEntry<MINT32>(
                                        item.second->HalDynamic,
                                        MTK_DUALZOOM_FD_REAL_MASTER,
                                        fdCamId);
        }
        MY_LOGD("fd master id(%d) f(%d)", fdCamId, data.mFrameNo);
    };
    auto getFDTargetId = [&data](){
        MINT32 fdTargetId = -1;
        for(auto&& item:data.mvInputParams) {
            if(IMetadata::getEntry<MINT32>(
                                        item.second->HalControl,
                                        MTK_DUALZOOM_FD_TARGET_MASTER,
                                        fdTargetId)) {
                break;
            }
        }
        return fdTargetId;
    };
    //
    int first_cam_id = data.mSyncTarget[0];
    auto inputParams_first = data.mvInputParams[first_cam_id];
    MINT32 masterId = -1;
    // get share data from staeis plugin
    if(inputParams_first != nullptr) {
        // if this frame is multicam frame, it shound have MTK_STEREO_SYNC2A_MASTER_SLAVE.
        // other, it will be treat as single cam.
        if(!IMetadata::getEntry<MINT32>(inputParams_first->HalControl, MTK_STEREO_SYNC2A_MASTER_SLAVE, masterId))
        {
            // if metadata doesn't contain MTK_STEREO_SYNC2A_MASTER_SLAVE and MTK_DUALZOOM_REAL_MASTER
            MINT32 realMasterId = -1;
            if(IMetadata::getEntry<MINT32>(inputParams_first->HalControl, MTK_DUALZOOM_REAL_MASTER, realMasterId))
            {
                //use MTK_DUALZOOM_REAL_MASTER directly
                MY_LOGD("already set real master id(%d) f(%d)", realMasterId, data.mFrameNo);
                updateMasterCamId(realMasterId);
                updateFDCamId(realMasterId);
            }
            else
            {
                // set MTK_DUALZOOM_REAL_MASTER to this camera id.
                updateMasterCamId(first_cam_id);
                updateFDCamId(first_cam_id);
            }
            return true;
        }
        if(data.mSyncTarget.size() == 1 || data.mvOutputParams.size() == 1) {
            // align mw setting.
            updateMasterCamId(masterId);
            updateFDCamId(masterId);
            return true;
        }
        auto targetFDId = getFDTargetId();
#if ENABLE_ISP_QUALITY_CHECK
        // check mw assign master cam is high quality or not.
        auto mw_master_data = data.mvOutputParams[masterId];
        MINT32 ispQuality = MTK_P1_RESIZE_QUALITY_LEVEL_UNKNOWN;
        if(!IMetadata::getEntry<MINT32>(mw_master_data->HalDynamic, MTK_P1NODE_RESIZE_QUALITY_LEVEL, ispQuality)){
            // if MTK_P1NODE_RESIZE_QUALITY_LEVEL not exist.
            // assign MTK_DUALZOOM_REAL_MASTER according to MW assign value.
            updateMasterCamId(masterId);
            updateFDCamId(targetFDId);
        }
        if(ispQuality == MTK_P1_RESIZE_QUALITY_LEVEL_H) {
            // mw assigned master id quality is high.
            // assign MTK_DUALZOOM_REAL_MASTER according to MW assign value.
            updateMasterCamId(masterId);
            updateFDCamId(targetFDId);
        }
        else {
            // check exist high quality in input params or not.
            // if find anyone is high, it will break.
            bool existHightQuality = false;
            for(auto&& item : data.mvOutputParams) {
                if(item.first != masterId) {
                    IMetadata::getEntry<MINT32>(item.second->HalDynamic, MTK_P1NODE_RESIZE_QUALITY_LEVEL, ispQuality);
                    if(ispQuality == MTK_P1_RESIZE_QUALITY_LEVEL_H) {
                        updateMasterCamId(item.first);
                        updateFDCamId(targetFDId);
                        existHightQuality = true;
                        break;
                    }
                }
            }
            // no high quality exist, set to mw assigned value.
            if(!existHightQuality) {
                // mw assigned master id quality is high.
                // assign MTK_DUALZOOM_REAL_MASTER according to MW assign value.
                updateMasterCamId(masterId);
                updateFDCamId(targetFDId);
            }
        }
#else
        MBOOL isZoomControlUpdate;
        // not support ENABLE_ISP_QUALITY_CHECK.
        // assign MTK_DUALZOOM_REAL_MASTER according to MW assign value.
        if(!IMetadata::getEntry<MBOOL>(inputParams_first->HalControl, MTK_DUALZOOM_MASTER_UPDATE_MODE, isZoomControlUpdate))
        {
            MY_LOGE(" something error !");
        }
        else{
            // check update mode
            if(!isZoomControlUpdate)
            {
                if(mp3rdReader)
                {
                    std::shared_ptr<NSCam::v3::Customer_FOVA_Streaming_Data> data;
                    if(mp3rdReader->cloneBack(data))
                    {
                        masterId = (MINT32)data->masterId;
                        MY_LOGD("override master id (%d)", masterId);
                    }
                }
            }
        }
        updateMasterCamId(masterId);
        updateFDCamId(targetFDId);
#endif
    }
    return true;
}
