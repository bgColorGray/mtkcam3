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

#define LOG_TAG "mtkcam-RequestMetadataPolicy"

#include "RequestMetadataPolicy.h"
//
#include "MyUtils.h"
#include <algorithm>
//
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/drv/iopipe/CamIO/IHalCamIO.h>
#include <mtkcam/aaa/aaa_hal_common.h>
#include <camera_custom_stereo.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;


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
//
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define PrintCrop(crop) MY_LOGI("%s = (%d, %d), %dx%d", #crop, crop.p.x, crop.p.y, crop.s.w, crop.s.h)

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace requestmetadata {


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
evaluateP1YuvCrop(
    EvaluateRequestParams const& params
) -> int
{
    // if support P1 direct yuv, need calculate P1 FOV crop
    if (mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV && params.FDYuvSize.size() != 0 && params.isFdEnabled)
    {
        int Id = params.targetFd;
        MRect ActiveArray = mvActiveArray[Id];
        MSize SensorSize = params.pSensorSize->at(params.targetFd);
        MSize rrzoBufSize = params.RrzoSize.at(params.targetFd);
        MRect FDCrop;
        if (ActiveArray.s.size() == 0)
        {
            MY_LOGE("cannot get active array size, cam id : %d", Id);
            return -1;
        }

        IMetadata::IEntry P1Crop = params.pAdditionalHal->at(params.targetFd)->entryFor(MTK_P1NODE_SENSOR_CROP_REGION);
        if(P1Crop.isEmpty())
        {
            // if there is no p1 crop in hal metadata, p1node always use app crop and won't use FD scaler crop
            //MY_LOGD("have no p1 crop region");
            return OK;
        }
        MRect P1CropRegion = P1Crop.itemAt(0, Type2Type<MRect>());
        IMetadata::IEntry AppCrop = params.pRequest_AppControl->entryFor(MTK_SCALER_CROP_REGION);
        if (AppCrop.isEmpty())
        {
            MY_LOGW("cannot get scaler crop region");
            return OK;
        }

        /*
        if (rrzoBufSize.w != P1CropRegion.s.w || rrzoBufSize.h != P1CropRegion.s.h)
        {
            // current FD crop only be support while rrzo crop is same as rrzo dst size
            return OK;
        }*/

        /*
            Calculate P1 FD YUV crop. Because P1 FD crop is on sensor domain,
            calcluate sensor domain crop for P1 and active domain crop for FD node.
            The FD crop is inside P1 crop region
        */
        MRect AppCropRegion = AppCrop.itemAt(0, Type2Type<MRect>());
        int ratio_1024x_w = (AppCropRegion.s.w << 10) / ActiveArray.s.w;
        int ratio_1024x_h = (AppCropRegion.s.h << 10) / ActiveArray.s.h;
        MSize FDYuvSize = params.FDYuvSize;
        // add p1 hw margin
        FDYuvSize.w = FDYuvSize.w * MTKCAM_P1_HW_MARGIN / 10;
        FDYuvSize.h = FDYuvSize.h * MTKCAM_P1_HW_MARGIN / 10;
        MY_LOGD("FDYuvSize : %dx%d", FDYuvSize.w, FDYuvSize.h);
        FDCrop.s.w = MAX(MIN((SensorSize.w * ratio_1024x_w) >> 10, rrzoBufSize.w), FDYuvSize.w);
        FDCrop.s.h = MAX(MIN((SensorSize.h * ratio_1024x_h) >> 10, rrzoBufSize.h), FDYuvSize.h);
        if (rrzoBufSize.w < FDYuvSize.w || rrzoBufSize.h < FDYuvSize.h) {
            FDCrop.s.w = rrzoBufSize.w;
            FDCrop.s.h = rrzoBufSize.h;
        }
        FDCrop.p.x = ((rrzoBufSize.w - FDCrop.s.w) >> 1);//P1CropRegion.p.x + ((P1CropRegion.s.w - FDCrop.s.w) >> 1);
        FDCrop.p.y = ((rrzoBufSize.h - FDCrop.s.h) >> 1);//P1CropRegion.p.y + ((P1CropRegion.s.h - FDCrop.s.h) >> 1);
        PrintCrop(P1CropRegion);
        PrintCrop(FDCrop);
        IMetadata::setEntry<MRect>(params.pAdditionalHal->at(params.targetFd).get(), MTK_P1NODE_YUV_RESIZER1_CROP_REGION, FDCrop);
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
updateRawImageInfo(
    EvaluateRequestParams const& params
) -> int
{
    for (const auto& [_sensorId, _addtionalHal] : *(params.pAdditionalHal))
    {
        IMetadata::IEntry entry(MTK_HAL_REQUEST_RAW_IMAGE_INFO);
        MSize imgoSize;
        MINT32 fmt = 0;
        MINT32 stride = 0;
        std::shared_ptr<IMetadata> meta = _addtionalHal;
        if(meta == nullptr) continue;
        if(params.additionalRawInfo != nullptr &&
           params.additionalRawInfo->count(_sensorId))
        {
            auto& data = params.additionalRawInfo->at(_sensorId);
            imgoSize = data.imgSize;
            fmt = data.format;
            stride = data.stride;
            entry.push_back(fmt, Type2Type<MINT32>());
            entry.push_back(stride, Type2Type<MINT32>());
            entry.push_back(imgoSize.w, Type2Type<MINT32>());
            entry.push_back(imgoSize.h, Type2Type<MINT32>());
        }
        else
        {
            // update raw info from request image stream.
            if(params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() != 0)
            {
                // logical raw stream
                auto iter =
                            params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.find(
                                                mPolicyParams.pPipelineStaticInfo->openId);
                if(iter != params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.end())
                {
                    auto imgoStreamInfo = iter->second;
                    imgoSize = imgoStreamInfo->getImgSize();
                    fmt = imgoStreamInfo->getImgFormat();

                    IMetadata::IEntry packedEntry = (params.pRequest_AppControl)->entryFor(MTK_CONTROL_CAPTURE_PACKED_RAW_ENABLE);
                    if( !packedEntry.isEmpty() && packedEntry.itemAt(0, Type2Type<MINT32>()) > 0){
                        stride = imgoStreamInfo->getImageBufferInfo().bufPlanes.planes[0].rowStrideInBytes;
                    }
                    else {
                        stride = imgoStreamInfo->getBufPlanes().planes[0].rowStrideInBytes;
                    }
                    entry.push_back(fmt, Type2Type<MINT32>());
                    entry.push_back(stride, Type2Type<MINT32>());
                    entry.push_back(imgoSize.w, Type2Type<MINT32>());
                    entry.push_back(imgoSize.h, Type2Type<MINT32>());
                }
            }
            if(params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.size() != 0)
            {
                auto iter = params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.find(_sensorId);
                if(iter != params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.end())
                {
                    imgoSize = iter->second[0]->getImgSize();
                    fmt = iter->second[0]->getImgFormat();
                    stride = iter->second[0]->getBufPlanes().planes[0].rowStrideInBytes;
                    entry.push_back(fmt, Type2Type<MINT32>());
                    entry.push_back(stride, Type2Type<MINT32>());
                    entry.push_back(imgoSize.w, Type2Type<MINT32>());
                    entry.push_back(imgoSize.h, Type2Type<MINT32>());
                }
                else
                {
                    MY_LOGE("cannot find id(%d) in vAppImage_Output_RAW16_Physical", _sensorId);
                    continue;
                }
            }
            // for raw 10
            if(params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() != 0)
            {
                // logical raw stream
                auto iter =
                            params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.find(
                                                mPolicyParams.pPipelineStaticInfo->openId);
                if(iter != params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.end())
                {
                    auto imgoStreamInfo = iter->second;
                    imgoSize = imgoStreamInfo->getImgSize();
                    fmt = imgoStreamInfo->getImgFormat();

                    IMetadata::IEntry packedEntry = (params.pRequest_AppControl)->entryFor(MTK_CONTROL_CAPTURE_PACKED_RAW_ENABLE);
                    if( !packedEntry.isEmpty() && packedEntry.itemAt(0, Type2Type<MINT32>()) > 0){
                        stride = imgoStreamInfo->getImageBufferInfo().bufPlanes.planes[0].rowStrideInBytes;
                    }
                    else {
                        stride = imgoStreamInfo->getBufPlanes().planes[0].rowStrideInBytes;
                    }
                    entry.push_back(fmt, Type2Type<MINT32>());
                    entry.push_back(stride, Type2Type<MINT32>());
                    entry.push_back(imgoSize.w, Type2Type<MINT32>());
                    entry.push_back(imgoSize.h, Type2Type<MINT32>());
                }
            }
            if(params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.size() != 0)
            {
                auto iter = params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.find(_sensorId);
                if(iter != params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.end())
                {
                    imgoSize = iter->second[0]->getImgSize();
                    fmt = iter->second[0]->getImgFormat();
                    stride = iter->second[0]->getBufPlanes().planes[0].rowStrideInBytes;
                    entry.push_back(fmt, Type2Type<MINT32>());
                    entry.push_back(stride, Type2Type<MINT32>());
                    entry.push_back(imgoSize.w, Type2Type<MINT32>());
                    entry.push_back(imgoSize.h, Type2Type<MINT32>());
                }
                else
                {
                    MY_LOGE("cannot find id(%d) in vAppImage_Output_RAW10_Physical", _sensorId);
                    continue;
                }
            }
        }
        if(meta->count() > 0)
        {
            meta->update(entry.tag(), entry);
            MY_LOGI("raw output: s(%dx%d) f(%x) stride(%d)",
                            imgoSize.w,
                            imgoSize.h,
                            fmt,
                            stride);
        }
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
updateMulticamControlMeta(
    EvaluateRequestParams const& params
) -> int
{
    if(params.pMultiCamReqOutputParams == nullptr ||
       params.pvNeedP1Node == nullptr ||
       params.pAdditionalHal == nullptr)
    {
        return OK;
    }
    updateMulticamSyncMeta(params);
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
updateMulticamSyncMeta(
    EvaluateRequestParams const& params
) -> int
{
    size_t p1EnableCount = 0;
    for (const auto& [_sensorId, _needP1] : *(params.pvNeedP1Node))
    {
        if(_needP1)
            p1EnableCount++;
    }
    auto masterId = params.pMultiCamReqOutputParams->masterId;
    if(p1EnableCount > 1)
    {
        // need sync tag
        auto streamList = params.pMultiCamReqOutputParams->streamingSensorList;
        auto streamListSize = streamList.size();
        auto capStreamListSize = params.pMultiCamReqOutputParams->prvStreamingSensorList.size();
        if(params.needExif && (streamListSize <= capStreamListSize))
        {
            // capture, and stream size is equal to capture.
            masterId = params.pMultiCamReqOutputParams->prvMasterId;
            streamList = params.pMultiCamReqOutputParams->prvStreamingSensorList;
        }
        updateSync3AMeta(params, masterId, streamList);
        updateSyncHelperMeta(params, masterId, streamList);
    }
    else
    {
        auto const& pParsedMultiCamInfo =
                            mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
        if(pParsedMultiCamInfo->mDualDevicePath != DualDevicePath::Single)
        {
            // set default sync tag.
            for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
            {
                if(_pAdditionalHal != nullptr)
                {
                    IMetadata::setEntry<MINT32>(_pAdditionalHal.get(), MTK_STEREO_SYNC2A_MODE, NS3Av3::E_SYNC2A_MODE::E_SYNC2A_MODE_NONE);
                    IMetadata::setEntry<MINT32>(_pAdditionalHal.get(), MTK_STEREO_SYNCAF_MODE, NS3Av3::E_SYNCAF_MODE::E_SYNCAF_MODE_OFF);
                    IMetadata::setEntry<MINT32>(_pAdditionalHal.get(), MTK_STEREO_HW_FRM_SYNC_MODE, NS3Av3::E_HW_FRM_SYNC_MODE::E_HW_FRM_SYNC_MODE_OFF);
                }
            }
        }
    }

    // set realMasterId
    // In mutlicam capture scenario, it has to decide new master id to avoide get
    // incorect image set that using different master cam id for multiframe image output.
    // Because, each capture request uses imgo to output capture image, dicide new master
    // id is fine. (normal preview and non-zsl capture still use current masterId)
    if (params.needZslFlow == MTRUE)
    {
        masterId = params.pMultiCamReqOutputParams->prvMasterId;
    }

    // add realMaster for metadata result decesion
    for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
    {
        if(_pAdditionalHal != nullptr)
        {
            IMetadata::setEntry<MINT32>(_pAdditionalHal.get(), MTK_DUALZOOM_REAL_MASTER, masterId);
        }
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
updateSync3AMeta(
    EvaluateRequestParams const& params,
    uint32_t const& masterId,
    std::vector<uint32_t> const& streamingList
) -> int
{
    // check need sync AF
    IMetadata::IEntry syncAfEntry(MTK_STEREO_SYNCAF_MODE);
    if(params.pMultiCamReqOutputParams->needSyncAf)
    {
        syncAfEntry.push_back(NS3Av3::E_SYNCAF_MODE::E_SYNCAF_MODE_ON, Type2Type<MINT32>());
    }
    else
    {
        syncAfEntry.push_back(NS3Av3::E_SYNCAF_MODE::E_SYNCAF_MODE_OFF, Type2Type<MINT32>());
    }


    IMetadata::IEntry masterSlaveEntry(MTK_STEREO_SYNC2A_MASTER_SLAVE);
    IMetadata::IEntry sync2AEntry(MTK_STEREO_SYNC2A_MODE);
    if(params.pMultiCamReqOutputParams->needSync2A)
    {
        auto const& pParsedMultiCamInfo =
                            mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
        if(pParsedMultiCamInfo->mDualDevicePath != DualDevicePath::Single)
        {
            android::String8 val("");
            masterSlaveEntry.push_back(masterId, Type2Type<MINT32>());
            val += android::String8::format("%" PRIu32 ",", masterId);
            for(auto const& id:streamingList)
            {
                if(masterId != id)
                {
                    masterSlaveEntry.push_back(id, Type2Type<MINT32>());
                    val += android::String8::format("%" PRIu32 ",", id);
                }
            }
            MY_LOGI("[%" PRIu32 "] masterid [%s]",
                        params.requestNo, val.string());
            if(pParsedMultiCamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF)
            {
                sync2AEntry.push_back(NS3Av3::E_SYNC2A_MODE::E_SYNC2A_MODE_VSDOF_BY_FRAME, Type2Type<MINT32>());
            }
            else
            {
                sync2AEntry.push_back(NS3Av3::E_SYNC2A_MODE::E_SYNC2A_MODE_DUAL_ZOOM_BY_FRAME, Type2Type<MINT32>());
            }
        }
    }
    else
    {
        sync2AEntry.push_back(NS3Av3::E_SYNC2A_MODE::E_SYNC2A_MODE_NONE, Type2Type<MINT32>());
    }

    for (const auto& [_sensorId, _needP1] : *(params.pvNeedP1Node))
    {
        if(_needP1 && params.pAdditionalHal->at(_sensorId) != nullptr)
        {
            params.pAdditionalHal->at(_sensorId)->update(syncAfEntry.tag(), syncAfEntry);
            params.pAdditionalHal->at(_sensorId)->update(sync2AEntry.tag(), sync2AEntry);
            if(masterSlaveEntry.count())
            {
                params.pAdditionalHal->at(_sensorId)->update(masterSlaveEntry.tag(), masterSlaveEntry);
            }
        }
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
updateSyncHelperMeta(
    EvaluateRequestParams const& params,
    uint32_t const& masterId __attribute__((unused)),
    std::vector<uint32_t> const& streamingList
) -> int
{
    if(!params.pMultiCamReqOutputParams->needFramesync)
    {
        return OK;
    }

    MINT64 hwsyncToleranceTime = ::getFrameSyncToleranceTime();
    MINT32 syncFailBehavior = MTK_FRAMESYNC_FAILHANDLE_CONTINUE;

    for (const auto& sensorId : streamingList)
    {
        IMetadata::IEntry tag(MTK_FRAMESYNC_ID);
        android::String8 camstr;
        MINT32 syncType = Utils::Imp::HW_RESULT_CHECK;
        auto index = mPolicyParams.pPipelineStaticInfo->getIndexFromSensorId(sensorId);

        if((index == -1 || params.pAdditionalHal->at(sensorId) == nullptr))
        {
            MY_LOGE("id: %d is null hal metadata", sensorId);
            continue;
        }

        auto &metadata = params.pAdditionalHal->at(sensorId);
        for (const auto& otherId : streamingList)
        {
            if(sensorId != otherId)
            {
                tag.push_back(otherId, Type2Type<MINT32>());
                camstr += android::String8::format("%d,", otherId);
            }
        }
        metadata->update(tag.tag(), tag);
        MY_LOGI_IF(tag.count() > 0,
                    "[%" PRIu32 "] set sync tag [%s] for camId[%" PRIu32 "]",
                    params.requestNo, camstr.string(), sensorId);
        if(params.pMultiCamReqOutputParams->needSynchelper_3AEnq)
        {
            syncType |= Utils::Imp::ENQ_HW;
        }
        // set tolerance time
        IMetadata::setEntry<MINT64>(metadata.get(), MTK_FRAMESYNC_TOLERANCE, hwsyncToleranceTime);
        // set sync fail behavior
        IMetadata::setEntry<MINT32>(metadata.get(), MTK_FRAMESYNC_FAILHANDLE, syncFailBehavior);
        IMetadata::setEntry<MINT32>(metadata.get(), MTK_FRAMESYNC_TYPE, syncType);
        if(params.pMultiCamReqOutputParams->needFramesync)
        {
            MY_LOGD("[%" PRIu32 "]set framesync tag", params.requestNo);
            IMetadata::setEntry<MINT32>(metadata.get(), MTK_STEREO_HW_FRM_SYNC_MODE, NS3Av3::E_HW_FRM_SYNC_MODE::E_HW_FRM_SYNC_MODE_ON);
        }
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
RequestMetadataPolicy_Default::
RequestMetadataPolicy_Default(
    CreationParams const& params
)
    :mPolicyParams(params)
{
    for (size_t i = 0; i < params.pPipelineStaticInfo->sensorId.size(); i++)
    {
        mvTargetRrzoSize.push_back(MSize(0, 0));
        int Id = params.pPipelineStaticInfo->sensorId[i];
        MRect ActiveArray;
        sp<IMetadataProvider> pMetadataProvider = NSMetadataProviderManager::valueFor(Id);
        if( ! pMetadataProvider.get() ) {
            CAM_ULOGME(" ! pMetadataProvider.get(), id : %d ", Id);
            mvActiveArray.emplace(Id, ActiveArray);
            continue;
        }

        IMetadata static_meta = pMetadataProvider->getMtkStaticCharacteristics();
        {
            IMetadata::IEntry active_array_entry = static_meta.entryFor(MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION);
            if( !active_array_entry.isEmpty() ) {
                ActiveArray = active_array_entry.itemAt(0, Type2Type<MRect>());
            } else {
                CAM_ULOGME("no static info: MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION");
            }
            mvActiveArray.emplace(Id, ActiveArray);
        }
    }
    mUniqueKey = NSCam::Utils::TimeTool::getReadableTime();
    mvAppStreamId.clear();
    mForceExif = property_get_int32("vendor.debug.hal3av3.forcedump", 0);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_Default::
evaluateRequest(
    EvaluateRequestParams const& params
) -> int
{
    // update NDD metadata
    {
        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
        {
            if(_pAdditionalHal != nullptr) {
                *(_pAdditionalHal) += *params.pNDDMetadata;
            }
        }
    }

    // update request unique key
    if (params.pAdditionalHal->size() > 0)
    {
        // check any entry contain MTK_PIPELINE_UNIQUE_KEY or not.
        IMetadata::IEntry entry(MTK_PIPELINE_UNIQUE_KEY);
        IMetadata::IEntry entry1;

        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
        {
            if(_pAdditionalHal != nullptr)
            {
                entry1 = _pAdditionalHal->entryFor(MTK_PIPELINE_UNIQUE_KEY);
                break;
            }
        }
        if(entry1.isEmpty())
        {
            entry.push_back(mUniqueKey, Type2Type<MINT32>());
        }
        else
        {
            if (entry1.count() > 0)
            {
                MINT32 ukey = entry1.itemAt(0, Type2Type<MINT32>());
                MY_LOGD("Setting has had uniquekey(%d)", ukey);
            }
            entry = entry1;
        }

        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
        {
            if(_pAdditionalHal != nullptr) {
                _pAdditionalHal->update(entry.tag(), entry);
            }
        }
    }

    // common metadata
    {
        MINT64 iMinFrmDuration = 0;
        std::vector<int64_t>  vStreamId;
        vStreamId.clear();
        for (auto const& it : params.pRequest_AppImageStreamInfo->vAppImage_Output_Proc)
        {
            StreamId_T const streamId = it.first;
            auto minFrameDuration = mPolicyParams.pPipelineUserConfiguration->vMinFrameDuration.find(streamId);
            vStreamId.push_back(streamId);
            if ( minFrameDuration == mPolicyParams.pPipelineUserConfiguration->vMinFrameDuration.end() )
            {
                MY_LOGD("Request App stream %#" PRIx64 "may be customize stream", streamId);
                continue;
            }
            if ( std::find(mvAppStreamId.begin(), mvAppStreamId.end(), streamId) == mvAppStreamId.end())
            {
                continue;
            }
            if ( CC_UNLIKELY(minFrameDuration->second < 0) ) {
                MY_LOGE("Request App stream %#" PRIx64 "have not configured yet", streamId);
                continue;
            }
            iMinFrmDuration = ( minFrameDuration->second > iMinFrmDuration ) ?
                                minFrameDuration->second : iMinFrmDuration;
        }

        if ( params.pRequest_ParsedAppMetaControl != nullptr) {
            if (params.pRequest_ParsedAppMetaControl->control_captureIntent !=
                MTK_CONTROL_CAPTURE_INTENT_STILL_CAPTURE &&
                params.pRequest_ParsedAppMetaControl->control_captureIntent !=
                MTK_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
                mvAppStreamId = vStreamId;
            }
        }
        //MY_LOGD( "The min frame duration is %" PRId64, iMinFrmDuration);
        IMetadata::IEntry entry(MTK_P1NODE_MIN_FRM_DURATION);
        entry.push_back(iMinFrmDuration, Type2Type<MINT64>());
        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal)) {
            if(_pAdditionalHal != nullptr) {
                _pAdditionalHal->update(entry.tag(), entry);
            }
        }
    }
    MUINT8 bRepeating = (MUINT8)(params.pRequest_ParsedAppMetaControl ? params.pRequest_ParsedAppMetaControl->repeating : false);
    {
        IMetadata::IEntry entry(MTK_HAL_REQUEST_REPEAT);
        entry.push_back(bRepeating, Type2Type< MUINT8 >());
        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal)) {
            if(_pAdditionalHal != nullptr) {
                _pAdditionalHal->update(entry.tag(), entry);
            }
        }
        //MY_LOGD("Control AppMetadata is repeating(%d)", bRepeating);
    }
    {
        if ( params.isZSLMode
          || params.pRequest_AppImageStreamInfo->pAppImage_Jpeg != nullptr
          || params.pRequest_AppImageStreamInfo->pAppImage_Output_Priv != nullptr
          || params.needExif
          || mForceExif
           )
        {
            //MY_LOGD("set MTK_HAL_REQUEST_REQUIRE_EXIF = 1");
            IMetadata::IEntry entry2(MTK_HAL_REQUEST_REQUIRE_EXIF);
            entry2.push_back(1, Type2Type<MUINT8>());
            for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal)) {
                if(_pAdditionalHal != nullptr) {
                    _pAdditionalHal->update(entry2.tag(), entry2);
                }
            }
        }

        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
        {
            if(_pAdditionalHal != nullptr) {
                IMetadata::IEntry rHalSensorSize = _pAdditionalHal->entryFor(MTK_HAL_REQUEST_SENSOR_SIZE);
                if(rHalSensorSize.isEmpty())
                {
                    // if there is no hal request sensor size in hal metadata, update it with intput param
                    IMetadata::IEntry entry3(MTK_HAL_REQUEST_SENSOR_SIZE);
                    entry3.push_back(params.pSensorSize->at(_sensorId), Type2Type<MSize>());
                    _pAdditionalHal->update(entry3.tag(), entry3);
                }

                IMetadata::IEntry entrySenId(MTK_HAL_REQUEST_SENSOR_ID);
                entrySenId.push_back(_sensorId, Type2Type<MINT32>());
                _pAdditionalHal->update(entrySenId.tag(), entrySenId);

                IMetadata::IEntry entryDevId(MTK_HAL_REQUEST_DEVICE_ID);
                entryDevId.push_back(mPolicyParams.pPipelineStaticInfo->openId, Type2Type<MINT32>());
                _pAdditionalHal->update(entryDevId.tag(), entryDevId);
            }
        }
    }
    if (params.pRequest_AppImageStreamInfo->hasVideoConsumer)
    {
        IMetadata::IEntry entry(MTK_PIPELINE_VIDEO_RECORD);
        entry.push_back(1, Type2Type< MINT32 >());
        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal)) {
            if(_pAdditionalHal != nullptr) {
                _pAdditionalHal->update(entry.tag(), entry);
            }
        }
        //MY_LOGD("Control AppMetadata is repeating(%d)", bRepeating);
    }
    // for process raw
    int force_process = ::property_get_int32("vendor.debug.camera.app.process.raw", 0);
    if ( params.pRequest_ParsedAppMetaControl != nullptr) {
        if ( (params.pRequest_ParsedAppMetaControl->control_processRaw == 1 ||
        force_process == 1) && !(mPolicyParams.
        pPipelineStaticInfo->isNeedP2ProcessRaw))
        {
            IMetadata::IEntry entry(MTK_P1NODE_RAW_TYPE);
            entry.push_back(NSIoPipe::NSCamIOPipe::EPipe_PROCESSED_RAW, Type2Type< MINT32 >());
            for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal)) {
                if (CC_LIKELY(_pAdditionalHal != nullptr))
                {
                    if ((_pAdditionalHal->entryFor(MTK_P1NODE_RAW_TYPE)).isEmpty())
                    {
                        _pAdditionalHal->update(entry.tag(), entry);
                    }
                    else
                    {
                        MY_LOGI("MTK_P1NODE_RAW_TYPE already existed, id(%d)", _sensorId);
                    }
                }
            }
        }
    }
    #if 0
    {
        for(size_t i = 0; i < params.pAdditionalHal->size(); ++i)
        {
            if (!params.needReconfigure)
            {
                if (!bRepeating || mvTargetRrzoSize[i].size() == 0)
                {
                    IMetadata::IEntry Crop = (*params.pAdditionalHal)[i]->entryFor(MTK_P1NODE_SENSOR_CROP_REGION);
                    if( Crop.isEmpty() ) {
                        Crop = params.pRequest_AppControl->entryFor(MTK_SCALER_CROP_REGION);
                        if (Crop.isEmpty())
                        {
                            MY_LOGW("cannot get scaler crop region, index : %zu", i);
                            continue;
                        }
                    }
                    MSize rrzoBufSize = params.RrzoSize[i];
                    MRect cropRegion = Crop.itemAt(0, Type2Type<MRect>());
                    #define ALIGN16(x) (((x) + 15) & ~(15))
                    if((cropRegion.s.w * rrzoBufSize.h) > (cropRegion.s.h * rrzoBufSize.w))
                    {
                        MINT32 temp = rrzoBufSize.h;
                        rrzoBufSize.h = ALIGN16(rrzoBufSize.w * cropRegion.s.h / cropRegion.s.w);
                        if (rrzoBufSize.h > temp)
                        {
                            rrzoBufSize.h = temp;
                        }
                    }
                    else
                    {
                        MINT32 temp = rrzoBufSize.w;
                        rrzoBufSize.w = ALIGN16(rrzoBufSize.h * cropRegion.s.w / cropRegion.s.h);
                        if (rrzoBufSize.w > temp)
                        {
                            rrzoBufSize.w = temp;
                        }
                    }
                    #undef ALIGN16
                    //mvTargetRrzoSize.push_back(rrzoBufSize);
                    mvTargetRrzoSize[i].w = rrzoBufSize.w;
                    mvTargetRrzoSize[i].h = rrzoBufSize.h;
                }
                IMetadata::IEntry Rrzotag(MTK_P1NODE_RESIZER_SET_SIZE);
                Rrzotag.push_back(mvTargetRrzoSize[i], Type2Type<MSize>());
                (*params.pAdditionalHal)[i]->update(MTK_P1NODE_RESIZER_SET_SIZE, Rrzotag);
            }
            else
            {
                mvTargetRrzoSize[i].w = 0;
                mvTargetRrzoSize[i].h = 0;
            }
        }
    }
    #endif

    if (params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() == 0 &&
        params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.size() == 0 &&
        params.pSensorId != nullptr && !params.needReconfigure)
    {
        do
        {
            for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal))
            {
                if(_pAdditionalHal != nullptr)
                {
                    MRect ActiveArray = mvActiveArray[_sensorId];
                    if (ActiveArray.s.size() == 0)
                    {
                        MY_LOGE("cannot get active array size, cam id : %d", _sensorId);
                        continue;
                    }

                    IMetadata::IEntry Crop = params.pRequest_AppControl->entryFor(MTK_SCALER_CROP_REGION);
                    if (Crop.isEmpty())
                    {
                        MY_LOGW("cannot get scaler crop region, id : %d", _sensorId);
                        continue;
                    }
                    //MSize sensorSize = (*params.pSensorSize)[i];
                    MRect cropRegion = Crop.itemAt(0, Type2Type<MRect>());
                    MSize toSensorSize;
                    MSize FixedRRZOSize;
                    toSensorSize.w = params.pSensorSize->at(_sensorId).w * cropRegion.s.w / ActiveArray.s.w;
                    toSensorSize.h = params.pSensorSize->at(_sensorId).h * cropRegion.s.h / ActiveArray.s.h;

                    // set P2Node sensor crop region if is single cam
                    if (mPolicyParams.pPipelineStaticInfo->sensorId.size() == 1) {
                        auto entryForP2SensorCrop = _pAdditionalHal->entryFor(MTK_P2NODE_SENSOR_CROP_REGION);
                        // MTK_P2NODE_SENSOR_CROP_REGION may be set at FeatureSetting for in-sensor-zoom
                        if (entryForP2SensorCrop.isEmpty()) {
                            MSize p2SensorSize;
                            IMetadata::IEntry zoomEntry = params.pRequest_AppControl->entryFor(MTK_CONTROL_ZOOM_RATIO);
                            if (!zoomEntry.isEmpty()) {
                                float zoomRatio = zoomEntry.itemAt(0, Type2Type<MFLOAT>());
                                p2SensorSize.w = (float) params.pSensorSize->at(_sensorId).w / zoomRatio;
                                p2SensorSize.h = (float) params.pSensorSize->at(_sensorId).h / zoomRatio;
                            } else {
                                p2SensorSize.w = params.pSensorSize->at(_sensorId).w * cropRegion.s.w / ActiveArray.s.w;
                                p2SensorSize.h = params.pSensorSize->at(_sensorId).h * cropRegion.s.w / ActiveArray.s.w;
                            }
                            MRect P2SensorCropRegion(p2SensorSize.w, p2SensorSize.h);
                            P2SensorCropRegion.p.x = (params.pSensorSize->at(_sensorId).w - p2SensorSize.w) / 2;
                            P2SensorCropRegion.p.y = (params.pSensorSize->at(_sensorId).h - p2SensorSize.h) / 2;
                            IMetadata::setEntry<MRect>(_pAdditionalHal.get(), MTK_P2NODE_SENSOR_CROP_REGION, P2SensorCropRegion);
                        }
                    }

                    Crop = _pAdditionalHal->entryFor(MTK_P1NODE_SENSOR_CROP_REGION);
                    if( !(Crop.isEmpty()) )
                    {
                        continue;
                    }

                    if (params.fixedRRZOSize.find(_sensorId) != params.fixedRRZOSize.end())
                    {
                        if (params.fixedRRZOSize.at(_sensorId).w != 0 && params.fixedRRZOSize.at(_sensorId).h != 0)
                        {
                            FixedRRZOSize = params.fixedRRZOSize.at(_sensorId);
                        }
                    }
                    else
                    {
                        FixedRRZOSize = params.RrzoSize.at(_sensorId);
                    }

                    if (toSensorSize.w > FixedRRZOSize.w || toSensorSize.h > FixedRRZOSize.h)
                    {
                        if(mPolicyParams.pPipelineStaticInfo->isAdditionalCroppingEnabled)
                        {
                            if (cropRegion.p.x == 0 &&
                                cropRegion.p.y == 0 &&
                                cropRegion.s.w == ActiveArray.s.w &&
                                cropRegion.s.h == ActiveArray.s.h)
                            {
                                cropRegion.p.x = 2;
                                cropRegion.p.y = 2;
                                cropRegion.s.w = params.pSensorSize->at(_sensorId).w - 4;
                                cropRegion.s.h = params.pSensorSize->at(_sensorId).h - 4;
                                IMetadata::setEntry<MRect>(_pAdditionalHal.get(), MTK_P1NODE_SENSOR_CROP_REGION, cropRegion);
                            }
                        }
                    }
                    else
                    {
                        MY_LOGD("fix rrzo size : %dx%d, toSensorSize : %dx%d", FixedRRZOSize.w, FixedRRZOSize.h
                                                                                , toSensorSize.w, toSensorSize.h);

                        if (FixedRRZOSize.w <= params.pSensorSize->at(_sensorId).w &&
                            FixedRRZOSize.h <= params.pSensorSize->at(_sensorId).h)
                        {
                            cropRegion.s.w = FixedRRZOSize.w;
                            cropRegion.s.h = FixedRRZOSize.h;
                            cropRegion.p.x = (params.pSensorSize->at(_sensorId).w - FixedRRZOSize.w) >> 1;
                            cropRegion.p.y = (params.pSensorSize->at(_sensorId).h - FixedRRZOSize.h) >> 1;
                            IMetadata::setEntry<MRect>(_pAdditionalHal.get(), MTK_P1NODE_SENSOR_CROP_REGION, cropRegion);
                        }
                    }
                }
            }
        } while(0);

    }

    evaluateP1YuvCrop(params);

    if(params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() > 0 ||
       params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.size() > 0 ||
       params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10.size() > 0 ||
       params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW10_Physical.size() > 0)
    {
        updateRawImageInfo(params);
    }

    updateMulticamControlMeta(params);

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto makePolicy_RequestMetadata_Default(
    CreationParams const& params
) -> std::shared_ptr<IRequestMetadataPolicy>
{
    return std::make_shared<RequestMetadataPolicy_Default>(params);
}


/******************************************************************************
 *
 ******************************************************************************/
RequestMetadataPolicy_DebugDump::
RequestMetadataPolicy_DebugDump(
    CreationParams const& params
)
    :mPolicyParams(params)
{
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_DebugDump::
evaluateRequest(
    EvaluateRequestParams const& params
) -> int
{
    if ( CC_LIKELY(mPolicyParams.pRequestMetadataPolicy != nullptr) )
    {
        mPolicyParams.pRequestMetadataPolicy->evaluateRequest(params);
    }

    //
    int debugRayType = property_get_int32("vendor.debug.camera.raw.type", -1);
    if(debugRayType >= 0)
    {
        if(debugRayType == 0 && mPolicyParams.pPipelineStaticInfo->isNeedP2ProcessRaw)
        {
            MY_LOGD("vendor.debug.camera.raw.type==0, but isNeedP2ProcessRaw is true, so can't set processed raw, reset debugRayType=1");
            debugRayType = 1;
        }
        MY_LOGD("set vendor.debug.camera.raw.type(%d) => MTK_P1NODE_RAW_TYPE(%d)  0:processed-raw 1:pure-raw",debugRayType,debugRayType);
        IMetadata::IEntry entry(MTK_P1NODE_RAW_TYPE);
        entry.push_back(debugRayType, Type2Type< int >());
        for (const auto& [_sensorId, _pAdditionalHal] : *(params.pAdditionalHal)) {
            if(CC_LIKELY(_pAdditionalHal != nullptr)) {
                _pAdditionalHal->update(entry.tag(), entry);
            }
        }
    }
    return OK;
}
/******************************************************************************
 *
 ******************************************************************************/
auto makePolicy_RequestMetadata_DebugDump(
    CreationParams const& params
) -> std::shared_ptr<IRequestMetadataPolicy>
{
    return std::make_shared<RequestMetadataPolicy_DebugDump>(params);
}
};  //namespace requestmetadata
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam

