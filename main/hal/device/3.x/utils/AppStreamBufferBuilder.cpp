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

#include "include/AppStreamBufferBuilder.h"
#include "MyUtils.h"
// #include <mtkcam3/pipeline/stream/StreamId.h>

#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/std/ULog.h>

#include <utils/RefBase.h>
#include <sstream>
#include <string>

using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::Utils::ULog;
using namespace NSCam::v3::Utils;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)

//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#define VALIDATE_STRING(string) (string!=nullptr ? string : "UNKNOWN")

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {


static
auto
convertToSensorColorOrder(uint8_t colorFilterArrangement) -> int32_t
{
    switch (colorFilterArrangement)
    {
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
        return (int32_t)SENSOR_FORMAT_ORDER_RAW_R;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
        return (int32_t)SENSOR_FORMAT_ORDER_RAW_Gr;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
        return (int32_t)SENSOR_FORMAT_ORDER_RAW_Gb;
    case MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
        return (int32_t)SENSOR_FORMAT_ORDER_RAW_B;
    default:
        break;
    }
    MY_LOGE("Unsupported Color Filter Arrangement:%d", colorFilterArrangement);
    return -1;
}


/******************************************************************************
 *
 ******************************************************************************/
static
auto
createImageStreamBuffer(
    IImageStreamInfo const* pStreamInfo,
    const std::shared_ptr<IIonDevice> pIonDevice,
    IMetadata const& staticMetadata,
    const std::string& bufferName,
    const StreamBuffer& streamBuffer,
    buffer_handle_t bufferHandle,
    int const acquire_fence,
    int const release_fence
) -> AppImageStreamBuffer*
{
    if ( pStreamInfo == nullptr ) {
        MY_LOGE("Null ImageStreamInfo, failed to allocate streamBuffer, return null");
        return nullptr;
    }

    int heapFormat = pStreamInfo->getAllocImgFormat();
    int stride = 0;

    if (heapFormat == static_cast<int>(eImgFmt_BLOB)) {
        if (pStreamInfo->getImgFormat() == static_cast<int>(eImgFmt_JPEG) ||
            pStreamInfo->getImgFormat() == static_cast<int>(eImgFmt_JPEG_APP_SEGMENT))
            heapFormat = pStreamInfo->getImgFormat();
    }

    std::string strName = VALIDATE_STRING(pStreamInfo->getStreamName());
    sp<IGraphicImageBufferHeap> pImageBufferHeap =
        IGraphicImageBufferHeap::create(
            (bufferName + ":" + strName).c_str(),
            pStreamInfo->getUsageForAllocator(),
            pStreamInfo->getAllocImgFormat() == eImgFmt_BLOB ? MSize(pStreamInfo->getAllocBufPlanes().planes[0].rowStrideInBytes, 1): pStreamInfo->getImgSize(),
            heapFormat,
            &bufferHandle,
            acquire_fence,
            release_fence,
            stride,
            pStreamInfo->getSecureInfo().type,
            pIonDevice
        );
    if ( pImageBufferHeap == nullptr ){
        ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG, "createIGraphicImageBufferHeap-Fail");
        // mFrameHandler->dumpState(logPrinter, std::vector<std::string>());
        MY_LOGF("IGraphicImageBufferHeap::create \"%s:%s\", handle: %p, fd: %d",
                bufferName.c_str(), pStreamInfo->getStreamName(), bufferHandle, bufferHandle->data[0]);
    }

    if ( ( pStreamInfo->getAllocImgFormat() == HAL_PIXEL_FORMAT_RAW16
        || pStreamInfo->getAllocImgFormat() == HAL_PIXEL_FORMAT_CAMERA_OPAQUE )
        && pStreamInfo->getStreamType() == eSTREAMTYPE_IMAGE_IN )
    {
        uint8_t colorFilterArrangement = 0;
        // IMetadata const& staticMetadata = mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics();
        bool ret = IMetadata::getEntry(&staticMetadata, MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, colorFilterArrangement);
        MY_LOGF_IF(!ret, "no static info: MTK_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT");
        auto sensorColorOrder = convertToSensorColorOrder(colorFilterArrangement);
        pImageBufferHeap->setColorArrangement(sensorColorOrder);
    }

    if (pStreamInfo->getUsageForAllocator() & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
        android_dataspace_t dataspace = (android_dataspace_t)pStreamInfo->getDataSpace();
        bool isBT601 = false, isBT709 = false, isLimit = false, isFull = false;
        eColorProfile colorProfile = eCOLORPROFILE_UNKNOWN;
        if (    (dataspace == HAL_DATASPACE_BT601_625)
             || (dataspace == HAL_DATASPACE_BT601_525) ) {
            isBT601 = true;
            isLimit = true;
        }
        else if ( (dataspace == HAL_DATASPACE_BT709) ) {
            isBT709 = true;
        }
        else if ( (dataspace & HAL_DATASPACE_STANDARD_BT709) ) {
            isBT709 = true;
            if ( dataspace & HAL_DATASPACE_RANGE_LIMITED )
                isLimit = true;
            else if ( dataspace & HAL_DATASPACE_RANGE_FULL )
                isFull = true;
        }
        else if (    (dataspace & HAL_DATASPACE_STANDARD_BT601_625)
                  || (dataspace & HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED)
                  || (dataspace & HAL_DATASPACE_STANDARD_BT601_525)
                  || (dataspace & HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED) ) {
            isBT601 = true;
            if ( dataspace & HAL_DATASPACE_RANGE_LIMITED )
                isLimit = true;
            else if ( dataspace & HAL_DATASPACE_RANGE_FULL )
                isFull = true;
        }
        // select color profile
        if (isBT601 && isLimit)
            colorProfile = eCOLORPROFILE_BT601_LIMITED;
        else if (isBT601 && isFull)
            colorProfile = eCOLORPROFILE_BT601_FULL;
        else if (isBT709)
            colorProfile = eCOLORPROFILE_BT709;
        // set color profile
        if (colorProfile != eCOLORPROFILE_UNKNOWN)
            pImageBufferHeap->setColorProfile(colorProfile);
    }

    AppImageStreamBuffer* pStreamBuffer =
    AppImageStreamBuffer::Allocator(const_cast<IImageStreamInfo*>(pStreamInfo))(streamBuffer.bufferId, pImageBufferHeap.get(), bufferHandle, streamBuffer.appBufferHandleHolder);
    return pStreamBuffer;
}

/******************************************************************************
 *
 ******************************************************************************/
auto makeAppImageStreamBuffer(
    BuildImageStreamBufferInputParams const& in,
    std::stringstream& log
) -> android::sp<AppImageStreamBuffer>
{
    auto queryBufferName = [=](auto const& buffer) -> std::string {
        return ("halbuf:b"+std::to_string(buffer.bufferId));
    };

    auto duplicateFenceFD = [=](buffer_handle_t handle, int& fd)
    {
        if (handle == nullptr || handle->numFds == 0) {
            fd = -1;
            return true;
        }
        if (handle->numFds != 1) {
            MY_LOGE("invalid fence handle with %d file descriptors", handle->numFds);
            fd = -1;
            return false;
        }
        if (handle->data[0] < 0) {
            fd = -1;
            return true;
        }
        fd = ::dup(handle->data[0]);
        if (fd < 0) {
            MY_LOGE("failed to dup fence fd %d", handle->data[0]);
            return false;
        }
        return true;
    };

    log << "StreamBuffer:"
        << reinterpret_cast<void*>(const_cast<StreamBuffer*>(&in.streamBuffer))
        << " " << toString(in.streamBuffer) << " / ";

    if  ( in.streamBuffer.streamId == -1 || in.streamBuffer.bufferId == 0 ) {
        MY_LOGE("invalid streamBuffer streamId:%d bufferId:%" PRIu64 " handle:%p",
            in.streamBuffer.streamId, in.streamBuffer.bufferId, in.streamBuffer.buffer);
        return nullptr;
    }

    buffer_handle_t bufferHandle = in.streamBuffer.buffer;
    int acquire_fence = -1;

    if ( in.streamBuffer.acquireFenceOp.type == FenceType::HDL) {
        if ( ! duplicateFenceFD(in.streamBuffer.acquireFenceOp.hdl, acquire_fence) ) {
            MY_LOGE("streamId:%d bufferId:%" PRIu64 " acquire fence is invalid", in.streamBuffer.streamId, in.streamBuffer.bufferId);
            ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG, "importStreamBuffer-Fail");
            return nullptr;
        }
        // MY_LOGD("createImageStreamBuffer acquire_fence(%d) from hdl", acquire_fence);
    } else {
        acquire_fence = in.streamBuffer.acquireFenceOp.fd;
        // MY_LOGD("createImageStreamBuffer acquire_fence(%d) from fd", acquire_fence);
    }

    auto pStreamBuffer = createImageStreamBuffer(
                            in.pStreamInfo,
                            in.pIonDevice,
                            in.staticMetadata,
                            queryBufferName(in.streamBuffer),
                            in.streamBuffer,
                            bufferHandle,
                            acquire_fence,
                            -1/*release_fence*/);

    if  ( pStreamBuffer == nullptr ) {
        ULogPrinter logPrinter(__ULOG_MODULE_ID, LOG_TAG, DetailsType::DETAILS_DEBUG, "createImageStreamBuffer-Fail");
        MY_LOGE("fail to create AppImageStreamBuffer - streamId:%u bufferId:%" PRIu64 " ", in.streamBuffer.streamId, in.streamBuffer.bufferId);
        return nullptr;
    }
    return pStreamBuffer;

}

};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
