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

#define LOG_TAG "mtkcam-RequestMetadataPolicy_SMVRBatch"

#include "RequestMetadataPolicy.h"
//
#include "MyUtils.h"
//
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/std/ULog.h>

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

#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

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

class RequestMetadataPolicy_SMVRBatch : public IRequestMetadataPolicy
{
public:
    virtual auto    evaluateRequest(
                        EvaluateRequestParams const& params
                    ) -> int;

public:
    // RequestMetadataPolicy Interfaces.
    RequestMetadataPolicy_SMVRBatch(CreationParams const& params);
    virtual ~RequestMetadataPolicy_SMVRBatch() { }

private:
    CreationParams mPolicyParams;
    MINT32         mUniqueKey;
    std::vector<MSize>    mvTargetRrzoSize;
    std::unordered_map<int, MRect> mvActiveArray;

    auto    evaluateP1YuvCrop(
                    EvaluateRequestParams const& params
                    ) -> int;
};


/******************************************************************************
 *
 ******************************************************************************/
RequestMetadataPolicy_SMVRBatch::
RequestMetadataPolicy_SMVRBatch(
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
}


/******************************************************************************
 *
 ******************************************************************************/
auto
RequestMetadataPolicy_SMVRBatch::
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
            MY_LOGD("have no p1 crop region");
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
        int ratio_1024x = (AppCropRegion.s.w << 10) / ActiveArray.s.w;
        MSize FDYuvSize = params.FDYuvSize;
        // add p1 hw margin
        FDYuvSize.w = FDYuvSize.w * MTKCAM_P1_HW_MARGIN / 10;
        FDYuvSize.h = FDYuvSize.h * MTKCAM_P1_HW_MARGIN / 10;
        MY_LOGD("FDYuvSize : %dx%d", FDYuvSize.w, FDYuvSize.h);
        FDCrop.s.w = MAX(MIN((SensorSize.w * ratio_1024x) >> 10, rrzoBufSize.w), FDYuvSize.w);
        FDCrop.s.h = MAX(MIN((SensorSize.h * ratio_1024x) >> 10, rrzoBufSize.h), FDYuvSize.h);
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
RequestMetadataPolicy_SMVRBatch::
evaluateRequest(
    EvaluateRequestParams const& params
) -> int
{
    auto main1SensorId = mPolicyParams.pPipelineStaticInfo->sensorId[0];

    // update request unique key
    if (params.pAdditionalHal->count(main1SensorId))
    {
        IMetadata::IEntry entry(MTK_PIPELINE_UNIQUE_KEY);
        IMetadata::IEntry entry1 = (*params.pAdditionalHal).at(main1SensorId)->entryFor(MTK_PIPELINE_UNIQUE_KEY);
        if (entry1.isEmpty())
        {
            entry.push_back(mUniqueKey, Type2Type<MINT32>());
        }
        else
        {
            entry = entry1;
        }

        for (const auto& [_sensorId, _additionalHal] : (*params.pAdditionalHal)) {
            _additionalHal->update(entry.tag(), entry);
        }
    }

    // update request id
    {
        IMetadata::IEntry entry(MTK_PIPELINE_REQUEST_NUMBER);
        entry.push_back(params.requestNo, Type2Type<MINT32>());
        for (const auto& [_sensorId, _additionalHal] : (*params.pAdditionalHal)) {
            _additionalHal->update(entry.tag(), entry);
        }
    }


    MUINT8 bRepeating = (MUINT8) params.pRequest_ParsedAppMetaControl->repeating;
    {
        IMetadata::IEntry entry(MTK_HAL_REQUEST_REPEAT);
        entry.push_back(bRepeating, Type2Type< MUINT8 >());
        for (const auto& [_sensorId, _additionalHal] : (*params.pAdditionalHal)) {
            _additionalHal->update(entry.tag(), entry);
        }
        //MY_LOGD("Control AppMetadata is repeating(%d)", bRepeating);
    }
    {

        if ( // params.isZSLMode || /* SMVRBatch: no need for ZSL */
             params.pRequest_AppImageStreamInfo->pAppImage_Jpeg != nullptr
             // || params.pRequest_AppImageStreamInfo->pAppImage_Output_Priv != nullptr /* SMVRBatch: no need for private/Paque reproc */
             //|| params.needExif /* SMVRBatch: no need for Exif */
           )
        {
            //MY_LOGD("set MTK_HAL_REQUEST_REQUIRE_EXIF = 1");
            IMetadata::IEntry entry2(MTK_HAL_REQUEST_REQUIRE_EXIF);
            entry2.push_back(1, Type2Type<MUINT8>());
            for (const auto& [_sensorId, _additionalHal] : (*params.pAdditionalHal)) {
                _additionalHal->update(entry2.tag(), entry2);
            }
        }

        for (const auto& [_sensorId, _additionalHal] : (*params.pAdditionalHal))
        {
            IMetadata::IEntry entry3(MTK_HAL_REQUEST_SENSOR_SIZE);
            entry3.push_back((*params.pSensorSize).at(_sensorId), Type2Type<MSize>());
            _additionalHal->update(entry3.tag(), entry3);
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

    //!!NOTES:
    // In SMVRBatch case, RRZO size should be decided by streams output size
    // Setting MTK_P1NODE_SENSOR_CROP_REGION will cause P1Node to re-calculate RRZO size, which should not be applied
    /*
    auto pParsedAppConfiguration = mPolicyParams.pPipelineUserConfiguration->pParsedAppConfiguration;
    auto pParsedSMVRBatchInfo = (pParsedAppConfiguration != nullptr) ? pParsedAppConfiguration->pParsedSMVRBatchInfo : nullptr;
    if (pParsedSMVRBatchInfo != nullptr)
    {
        MY_LOGD_IF(2 <= pParsedSMVRBatchInfo->logLevel, "SMVRBatch: no need to change rrzo size");
    }
    */
    if (params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16.size() == 0 &&
        params.pRequest_AppImageStreamInfo->vAppImage_Output_RAW16_Physical.size() == 0 &&
        params.pSensorId != nullptr && !params.needReconfigure &&
        params.isFdEnabled)
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
                        MY_LOGD("sensor size: %dx%d", params.pSensorSize->at(_sensorId).w, params.pSensorSize->at(_sensorId).h);

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

    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto makePolicy_RequestMetadata_SMVRBatch(
    CreationParams const& params
) -> std::shared_ptr<IRequestMetadataPolicy>
{
    return std::make_shared<RequestMetadataPolicy_SMVRBatch>(params);
}


};  //namespace requestmetadata
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
