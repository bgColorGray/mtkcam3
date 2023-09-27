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

#define LOG_TAG "mtkcam-ConfigStreamInfoPolicy_SMVRBatch"

#include <mtkcam3/pipeline/policy/IConfigStreamInfoPolicy.h>
//
//
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam3/plugin/streaminfo/types.h>
//
#include "MyUtils.h"
#include "P1StreamInfoBuilder.h"

#include <mtkcam/def/ImageFormat.h>
#include <mtkcam/aaa/IIspMgr.h>
#include <mtkcam/aaa/IHalISP.h>
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);

using namespace NS3Av3;
//
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3::pipeline::policy;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;


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
#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {


/**
 * default version
 */
FunctionType_Configuration_StreamInfo_P1 makePolicy_Configuration_StreamInfo_P1_SMVRBatch()
{
    return [](Configuration_StreamInfo_P1_Params const& params) -> int {
        auto pvOut = params.pvOut;
        auto pvSensorSetting = params.pvSensorSetting;
        auto pvP1HwSetting = params.pvP1HwSetting;
        auto pvP1DmaNeed = params.pvP1DmaNeed;
        auto pPipelineNodesNeed = params.pPipelineNodesNeed;
        auto pStreamingFeatureSetting = params.pStreamingFeatureSetting;
        auto pCaptureFeatureSetting = params.pCaptureFeatureSetting;
        auto pPipelineStaticInfo = params.pPipelineStaticInfo;
        auto pPipelineUserConfiguration = params.pPipelineUserConfiguration;
        auto const& pParsedAppConfiguration __unused = pPipelineUserConfiguration->pParsedAppConfiguration;

        // smvr-batch
        auto const& pParsedSMVRBatchInfo = pParsedAppConfiguration->pParsedSMVRBatchInfo;

        if(pParsedSMVRBatchInfo)
        {
            MINT32 iBurstNum=1;

            MINT32 supportDurationInSec = 1; // mkdbg-todo: SMVRBatch-burst mode: 1 sec, which might adjust later
            //!!NOTES:
            //  x = extraNumBufferToConsumePerFrame = (p1BatchNum - p2BatchNum)
            //  y = extraNumBufferToConsumeInOneSec = 30 * x
            //  z = totalExtraBufferToConsumeInSupportDuration = y * supportDureation = 30 * (p1BatchNum - p2BatchNum) * supportDurationInSec
            //  --> maxFrameQueuedToHandle =  z / p2batchNum = 30*(p1BatchNum-p2BatchNum)*supportDuration / p2BatchNum
            MINT32 maxFrameQueuedToHandle = 30*((pParsedSMVRBatchInfo->p1BatchNum - pParsedSMVRBatchInfo->p2BatchNum)*supportDurationInSec) / pParsedSMVRBatchInfo->p2BatchNum;

            MY_LOGD("maxFrameQueuedToHandler=%d, durationInSec=%d, extraNumBufferToConsumeInOneSec=30*(%d-%d)=%d",
               maxFrameQueuedToHandle,
               supportDurationInSec,
               pParsedSMVRBatchInfo->p1BatchNum, pParsedSMVRBatchInfo->p2BatchNum, 30*(pParsedSMVRBatchInfo->p1BatchNum - pParsedSMVRBatchInfo->p2BatchNum)
            );


            MINT32 p1BatchNum = pParsedSMVRBatchInfo->p1BatchNum;
            for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node)
            {
                ParsedStreamInfo_P1 config;
                int32_t nSensorIdx = pPipelineStaticInfo->getIndexFromSensorId(_sensorId);

                if (_needP1)
                {
                    if (nSensorIdx < 0)
                    {
                        MY_LOGE("SMVRBatch: no matched sensorIdx from sensorId:%d", _sensorId);
                        continue;
                    }
                    size_t sensorIdx = static_cast<size_t>(nSensorIdx);

                    //MetaData
                    {
                        P1MetaStreamInfoBuilder builder(sensorIdx);
                        builder.setP1AppMetaDynamic_Default().setMaxBufNum(10*iBurstNum + maxFrameQueuedToHandle);
                        config.pAppMeta_DynamicP1 = builder.build();
                    }
                    {
                        P1MetaStreamInfoBuilder builder(sensorIdx);
                        builder.setP1HalMetaDynamic_Default().setMaxBufNum(10*iBurstNum + maxFrameQueuedToHandle );
                        config.pHalMeta_DynamicP1 = builder.build();
                    }
                    {
                        P1MetaStreamInfoBuilder builder(sensorIdx);
                        builder.setP1HalMetaControl_Default().setMaxBufNum(10*iBurstNum + maxFrameQueuedToHandle);
                        config.pHalMeta_Control = builder.build();
                    }

                    std::shared_ptr<IHwInfoHelper> infohelper = IHwInfoHelperManager::get()->getHwInfoHelper(_sensorId);
                    MY_LOGF_IF(infohelper==nullptr, "getHwInfoHelper");

                    P1ImageStreamInfoBuilder::CtorParams const builderCtorParams{
                            .index = sensorIdx,
                            .pHwInfoHelper = infohelper,
                            .pCaptureFeatureSetting = pCaptureFeatureSetting,
                            .pStreamingFeatureSetting = pStreamingFeatureSetting,
                            .pPipelineStaticInfo = pPipelineStaticInfo,
                            .pPipelineUserConfiguration = pPipelineUserConfiguration,
                        };

                    MINT32 P1_streamBufCnt = 8;
                    //RRZO
                    if (pvP1DmaNeed->at(_sensorId) & P1_RRZO)
                    {
                        // get rrzo size, format, stride
                        MSize   rrzoSize = pvP1HwSetting->at(_sensorId).rrzoDefaultRequest.imageSize;
                        IImageStreamInfo::BufPlanes_t rrzoBufPlanes;
                        int32_t rrzoFormat = pvP1HwSetting->at(_sensorId).rrzoDefaultRequest.format;
                        if (! infohelper->getDefaultBufPlanes_Rrzo(rrzoBufPlanes, rrzoFormat, rrzoSize))
                        {
                            MY_LOGE("SMVRBatch: !!err: getDefaultBufPlanes wrong: rrzoFormat=0x%x, rrzoSize=%dx%d", rrzoFormat, rrzoSize.w, rrzoSize.h);
                            return BAD_VALUE;
                        }

                        // prepare P1ImagestreamInfoBuilder default image info
                        P1ImageStreamInfoBuilder builder(builderCtorParams);
                        builder.setP1Rrzo_Default(P1_streamBufCnt*iBurstNum + maxFrameQueuedToHandle, pvP1HwSetting->at(_sensorId));

                        // prepare P1ImagestreamInfoBuilder alloc info: (format, bufPlanes, bufOffset)
                        builder.setAllocImgFormat(eImgFmt_BLOB);

                        size_t allocSizeInBytes = [](IImageStreamInfo::BufPlanes_t const& bp, MINT32 logLevel) {
                                size_t size = 0;
                                for (size_t i = 0; i < bp.count ; i++) {
                                    size += bp.planes[i].sizeInBytes;
                                    MY_LOGD_IF(logLevel >= 2, "SMVRBatch: rrzoPlane[%zu]: size=%zu", i, size);
                                }
                                return size;
                            } (rrzoBufPlanes, pParsedSMVRBatchInfo->logLevel);
                        IImageStreamInfo::BufPlanes_t allocBufPlanes;
                        allocBufPlanes.count = 1;
                        allocBufPlanes.planes[0] = {allocSizeInBytes*p1BatchNum, allocSizeInBytes*p1BatchNum};
                        builder.setAllocBufPlanes(allocBufPlanes);
                        builder.setBufCount(p1BatchNum);
                        builder.setStartOffset(0);
                        builder.setBufStep(allocSizeInBytes);

                        config.pHalImage_P1_Rrzo = builder.build();
                        MY_LOGD_IF(pParsedSMVRBatchInfo->logLevel >= 2,
                            "SMVRBatch: rrzo_batchNum=%d, maxFrameQueuedToHandle=%d, rrzo-info(fmt=0x%x, allocFmt=0x%x, size=%dx%d, plane-allocSizeInBytes=%zu, allocBufPlane-sizeInBytes=%zu)",
                            p1BatchNum, maxFrameQueuedToHandle, rrzoFormat, eImgFmt_BLOB, rrzoSize.w, rrzoSize.h, allocSizeInBytes, allocBufPlanes.planes[0].sizeInBytes);
                    }
                    //STT
                    if (pvP1DmaNeed->at(_sensorId) & P1_LCSO)
                    {
                        P1STTImageStreamInfoBuilder builder(builderCtorParams);
                        using QueryParam = NSCam::plugin::streaminfo::P1STT_QueryParam;
                        QueryParam queryParam{
                                    .p1Batch = (MUINT32)p1BatchNum
                        };
                        builder.setPluginQueryParam(&queryParam);
                        builder.setMaxBufNum(P1_streamBufCnt*iBurstNum + maxFrameQueuedToHandle);
                        builder.setP1Stt_Default(_sensorId);

#if 0
                        // set alloc-info (format, bufPlanes)
                        builder.setAllocImgFormat(eImgFmt_BLOB);
                        NS3Av3::LCSO_Param lcsoParam;
                        NS3Av3::Buffer_Info lcsoInfo;
                        lcsoParam.bSupport = false;
                        auto ispHal = MAKE_HalISP(pPipelineStaticInfo->sensorId[i], LOG_TAG);
                        if (ispHal->queryISPBufferInfo(lcsoInfo))
                        {
                            MY_LOGD("queryISPBufferInfo, support : %d, format : %d, size : %dx%d",
                                    lcsoInfo.LCESO_Param.bSupport, lcsoInfo.LCESO_Param.format,
                                    lcsoInfo.LCESO_Param.size.w, lcsoInfo.LCESO_Param.size.h);
                            lcsoParam.format = lcsoInfo.LCESO_Param.format;
                            lcsoParam.size = lcsoInfo.LCESO_Param.size;
                            lcsoParam.stride = lcsoInfo.LCESO_Param.stride;
                        }
                        else if ( auto ispMgr = MAKE_IspMgr() )
                        {
                            ispMgr->queryLCSOParams(lcsoParam);
                        }
                        else {
                            MY_LOGE("Query IIspMgr FAILED!");
                        }

                        if (ispHal != nullptr)
                        {
                            ispHal->destroyInstance(LOG_TAG);
                            ispHal = nullptr;
                        }


                        IImageStreamInfo::BufPlanes_t lcsoBufPlanes;
                        if ( ! infohelper->getDefaultBufPlanes_Pass1(lcsoBufPlanes, lcsoParam.format, lcsoParam.size, lcsoParam.stride) )
                        {
                            MY_LOGE("!!err: infohelper->getDefaultBufPlanes_Pass1: FAILED!");
                        }

                        size_t allocSizeInBytes = [](IImageStreamInfo::BufPlanes_t const& bp, MINT32 logLevel) {
                                size_t size = 0;
                                for (size_t i = 0; i < bp.count ; i++) {
                                    size += bp.planes[i].sizeInBytes;
                                    MY_LOGD_IF(logLevel >= 2, "SMVRBatch: lcsoPlane[%d]: size=%zu", i, size);
                                }
                                return size;
                            } (lcsoBufPlanes, pParsedSMVRBatchInfo->logLevel);
                        IImageStreamInfo::BufPlanes_t allocBufPlanes;
                        allocBufPlanes.count = 1;
                        allocBufPlanes.planes[0] = {allocSizeInBytes*p1BatchNum, allocSizeInBytes*p1BatchNum};
                        builder.setAllocBufPlanes(allocBufPlanes);
                        builder.setBufCount(p1BatchNum);
                        builder.setStartOffset(0);
                        builder.setBufStep(allocSizeInBytes);

                        config.pHalImage_P1_Lcso = builder.build();

                        MY_LOGD_IF(pParsedSMVRBatchInfo->logLevel >= 2,
                            "SMVRBatch:lcso_batchNum=%d, maxFrameQueuedToHandle=%d, lcso-info(fmt=0x%x, allocFmt=0x%x, size=%dx%d, plane-allocSizeInBytes=%zu, allocBufPlane-sizeInBytes=%d)",
                            p1BatchNum, maxFrameQueuedToHandle, lcsoParam.format, eImgFmt_BLOB, lcsoParam.size.w, lcsoParam.size.h, allocSizeInBytes, allocBufPlanes.planes[0].sizeInBytes);
#else
                        config.pHalImage_P1_Lcso = builder.build();
#endif
                    }
                    //CRZO
                    if (pPipelineNodesNeed->needFDNode && (pvP1DmaNeed->at(_sensorId) & P1_FDYUV))
                    {
                        P1ImageStreamInfoBuilder builder(builderCtorParams);
                        builder.setP1FDYuv_Default(pvP1HwSetting->at(_sensorId), pvSensorSetting->at(_sensorId).sensorFps);
                        config.pHalImage_P1_FDYuv = builder.build();
                    }
                } // if (pPipelineNodesNeed->needP1Node[i])
                pvOut->emplace(_sensorId, std::move(config));
            } // for-loop
        }
       return OK;
    };
}


};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam


