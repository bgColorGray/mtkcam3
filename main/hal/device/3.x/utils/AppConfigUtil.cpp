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

#include "MyUtils.h"
#include <AppConfigUtil.h>
//
#include <unordered_set>
#include <cutils/properties.h>
#include <mtkcam/utils/std/TypeTraits.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
//
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
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


/******************************************************************************
 *
 ******************************************************************************/
static MINT findStreamSensorId(int32_t DevId __unused, const Stream& rStream)
{

    auto pHalLogicalDeviceList = MAKE_HalLogicalDeviceList();
    if(CC_UNLIKELY(pHalLogicalDeviceList == nullptr)){
        return -1;
    }
    char const* sensorId = rStream.physicalCameraId.c_str();
    MINT deviceId;
    if(strlen(sensorId)>0){
        MINT vid = atoi(sensorId);
        deviceId = pHalLogicalDeviceList->getDeviceIdByVID(vid);
    }else{
        deviceId = -1;
    }
    // 0-length string is not a physical output stream
    CAM_ULOGMI("stream id len : %lu", strlen(sensorId));
    if (strlen(sensorId) != 0)
    {
        return deviceId;
    }
    return -1;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
IAppConfigUtil::
create(const CreationInfo& creationInfo) -> IAppConfigUtil*
{
    auto pInstance = new AppConfigUtil(creationInfo);
    if  ( ! pInstance /*|| ! pInstance->initialize()*/ ) {
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}


/******************************************************************************
 *
 ******************************************************************************/
AppConfigUtil::
AppConfigUtil(const CreationInfo& creationInfo)
    : mCommonInfo(std::make_shared<CommonInfo>())
    , mInstanceName{std::to_string(creationInfo.mInstanceId) + ":AppConfigUtil"}
    , mStreamInfoBuilderFactory(std::make_shared<Utils::Camera3StreamInfoBuilderFactory>())
    // , mImageConfigMapLock(creationInfo.mImageConfigMapLock)
    // , mImageConfigMap(creationInfo.mImageConfigMap)
    // , mMetaConfigMap(creationInfo.mMetaConfigMap)
{
    IGrallocHelper* pGrallocHelper = IGrallocHelper::singleton();
    if ( pGrallocHelper == nullptr ) {
        MY_LOGE("IGrallocHelper::singleton(): nullptr");
        mCommonInfo = nullptr;
        return;
    }

    if ( mCommonInfo != nullptr )
    {
        int32_t loglevel = ::property_get_int32("vendor.debug.camera.log", 0);
        if ( loglevel == 0 ) {
            loglevel = ::property_get_int32("vendor.debug.camera.log.AppConfigUtil", 0);
        }
        //
        //
        mCommonInfo->mLogLevel = loglevel;
        mCommonInfo->mInstanceId = creationInfo.mInstanceId;
        mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
        mCommonInfo->mPhysicalMetadataProviders = creationInfo.mPhysicalMetadataProviders;
        mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
        mCommonInfo->mGrallocHelper = pGrallocHelper;
    }

    mEntryMinDuration =
        mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_SCALER_AVAILABLE_MIN_FRAME_DURATIONS);
    if  ( mEntryMinDuration.isEmpty() ) {
        MY_LOGE("no static MTK_SCALER_AVAILABLE_MIN_FRAME_DURATIONS");
    }
    mEntryStallDuration =
        mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_SCALER_AVAILABLE_STALL_DURATIONS);
    if  ( mEntryStallDuration.isEmpty() ) {
        MY_LOGE("no static MTK_SCALER_AVAILABLE_STALL_DURATIONS");
    }
    MY_LOGD("mImageConfigMap.size()=%lu, mMetaConfigMap.size()=%lu", mImageConfigMap.size(), mMetaConfigMap.size());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
destroy() -> void
{
    MY_LOGW("AppConfigUtil::destroy");
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
beginConfigureStreams(
    const StreamConfiguration& requestedConfiguration,
    HalStreamConfiguration& halConfiguration,
    std::shared_ptr<pipeline::model::UserConfigurationParams>& rCfgParams
)   -> ::android::status_t
{
    auto addFrameDuration = [this](auto& rStreams, auto const pStreamInfo) {
        for (size_t j = 0; j < mEntryMinDuration.count(); j+=4) {
            if (mEntryMinDuration.itemAt(j    , Type2Type<MINT64>()) == (MINT64)pStreamInfo->getOriImgFormat() &&
                mEntryMinDuration.itemAt(j + 1, Type2Type<MINT64>()) == (MINT64)pStreamInfo->getLandscapeSize().w &&
                mEntryMinDuration.itemAt(j + 2, Type2Type<MINT64>()) == (MINT64)pStreamInfo->getLandscapeSize().h)
            {
                rStreams->vMinFrameDuration.emplace(
                    pStreamInfo->getStreamId(),
                    mEntryMinDuration.itemAt(j + 3, Type2Type<MINT64>())
                );
                rStreams->vStallFrameDuration.emplace(
                    pStreamInfo->getStreamId(),
                    mEntryStallDuration.itemAt(j + 3, Type2Type<MINT64>())
                );
                MY_LOGI("[addFrameDuration] format:%" PRId64 " size:%" PRId64 "x%" PRId64 " min_duration:%" PRId64 ", stall_duration:%" PRId64 ,
                    mEntryMinDuration.itemAt(j, Type2Type<MINT64>()),
                    mEntryMinDuration.itemAt(j + 1, Type2Type<MINT64>()),
                    mEntryMinDuration.itemAt(j + 2, Type2Type<MINT64>()),
                    mEntryMinDuration.itemAt(j + 3, Type2Type<MINT64>()),
                    mEntryStallDuration.itemAt(j + 3, Type2Type<MINT64>()));
                break;
            }
        }
        return;
    };

    {
        StreamId_T const streamId = eSTREAMID_END_OF_FWK;
        std::string const streamName = "Meta:App:Control";
        auto pStreamInfo = createMetaStreamInfo(streamName.c_str(), streamId);
        addConfigStream(pStreamInfo);

        rCfgParams->vMetaStreams.emplace(streamId, pStreamInfo);
    }

    //
    mspParsedSMVRBatchInfo = extractSMVRBatchInfo(requestedConfiguration);
    //
    mE2EHDROn = extractE2EHDRInfo(requestedConfiguration);
    //
    mDngFormat = extractDNGFormatInfo(requestedConfiguration);
    //
    mRaw10Format = extractRAW10FormatInfo(requestedConfiguration);
    //
    mAovPipelineConfig = extractAOVInfo(requestedConfiguration);
    //
    halConfiguration.streams.resize(requestedConfiguration.streams.size());
    // rCfgParams->vImageStreams.setCapacity(requestedConfiguration.streams.size());
    for ( size_t i = 0; i < requestedConfiguration.streams.size(); i++ )
    {
        const auto& srcStream = requestedConfiguration.streams[i];
              auto& dstStream = halConfiguration.streams[i];
        StreamId_T streamId = srcStream.id;
        //
        sp<AppImageStreamInfo> pStreamInfo = getConfigImageStream(streamId);
        if ( pStreamInfo == nullptr )
        {
            pStreamInfo = createImageStreamInfo(srcStream, dstStream);
            if ( pStreamInfo == nullptr ) {
                MY_LOGE("createImageStreamInfo failed - Stream=%s", toString(srcStream).c_str());
                return -ENODEV;
            }
            addConfigStream(pStreamInfo.get()/*, false*//*keepBufferCache*/);
        }
        else
        {
            auto generateLogString = [=]() {
                return String8::format("streamId:%d type(%d:%d) "
                    "size(%dx%d:%dx%d) format(%d:%d) dataSpace(%d:%d) "
                    "usage(%#" PRIx64 ":%#" PRIx64 ")",
                    srcStream.id, pStreamInfo->getStream().streamType, srcStream.streamType,
                    pStreamInfo->getStream().width, pStreamInfo->getStream().height, srcStream.width, srcStream.height,
                    pStreamInfo->getStream().format, srcStream.format,
                    pStreamInfo->getStream().dataSpace, srcStream.dataSpace,
                    pStreamInfo->getStream().usage, srcStream.usage
                );
            };

            MY_LOGI("stream re-configuration: %s", generateLogString().c_str());

            // refer to google default wrapper implementation:
            // width/height/format must not change, but usage/rotation might need to change
            bool check1 = (srcStream.streamType == pStreamInfo->getStream().streamType
                        && srcStream.width      == pStreamInfo->getStream().width
                        && srcStream.height     == pStreamInfo->getStream().height
                        // && srcStream.dataSpace  == pStreamInfo->getStream().dataSpace
                        );
            bool check2 = true || (srcStream.format == pStreamInfo->getStream().format
                        ||(static_cast<android_pixel_format_t>(srcStream.format) == static_cast<android_pixel_format_t>(pStreamInfo->getImgFormat()) &&
                           HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == static_cast<android_pixel_format_t>(pStreamInfo->getStream().format))
                      //||(pStreamInfo->getStream().format == real format of srcStream.format &&
                      //   HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == srcStream.format)
                        );
            if ( ! check1 || ! check2 ) {
                MY_LOGE("stream configuration changed! %s", generateLogString().c_str());
                return -ENODEV;
            }

            // If usage is chaged, it implies that
            // the real format (flexible yuv/implementation_defined)
            // and the buffer layout may change.
            // In this case, should HAL and Frameworks have to cleanup the buffer handle cache?
            if ( pStreamInfo->getStream().usage != srcStream.usage ) {
                MY_LOGW("stream usage changed! %s", generateLogString().c_str());
                MY_LOGW("shall HAL and Frameworks have to clear buffer handle cache?");
            }

            // Create a new stream to override the old one, since usage/rotation might need to change.
            pStreamInfo = createImageStreamInfo(srcStream, dstStream);
            if ( pStreamInfo == nullptr ) {
                MY_LOGE("createImageStreamInfo failed - Stream=%s", toString(srcStream).c_str());
                return -ENODEV;
            }
            addConfigStream(pStreamInfo.get()/*, true*//*keepBufferCache*/);
        }

        // check hidl_stream contain physic id or not.
        if(srcStream.physicalCameraId.size() != 0)
        {
            auto& sensorList = rCfgParams->vPhysicCameras;
            MINT32 vid = std::stoi((std::string)srcStream.physicalCameraId);
            auto pcId = MAKE_HalLogicalDeviceList()->getDeviceIdByVID(vid);
            MY_LOGD("pcid(%d:%s)", pcId, ((std::string)srcStream.physicalCameraId).c_str());
            auto iter = std::find(
                                    sensorList.begin(),
                                    sensorList.end(),
                                    pcId);
            if(iter == sensorList.end())
            {
                sensorList.push_back(pcId);
                //physical settings
                String8 const streamName = String8::format("Meta:App:Physical_%s", ((std::string)srcStream.physicalCameraId).c_str());
                StreamId_T const streamId = eSTREAMID_BEGIN_OF_PHYSIC_ID + (int64_t)pcId;
                auto pMetaStreamInfo = createMetaStreamInfo(streamName.c_str(), streamId, pcId);
                addConfigStream(pMetaStreamInfo);
                rCfgParams->vMetaStreams_physical.emplace(pcId, pMetaStreamInfo);
            }
        }

        rCfgParams->vImageStreams.emplace(streamId, pStreamInfo);
        addFrameDuration(rCfgParams, pStreamInfo);

        rCfgParams->pStreamInfoBuilderFactory = getAppStreamInfoBuilderFactory();

        MY_LOGD_IF(getLogLevel()>=2, "Stream: id:%d streamType:%d %dx%d format:0x%x usage:0x%" PRIx64 " dataSpace:0x%x rotation:%d",
                srcStream.id, srcStream.streamType, srcStream.width, srcStream.height,
                srcStream.format, srcStream.usage, srcStream.dataSpace, srcStream.rotation);
    }
    //
    // remove unused config stream
    std::unordered_set<StreamId_T> usedStreamIds;
    usedStreamIds.reserve(halConfiguration.streams.size());
    for (size_t i = 0; i < halConfiguration.streams.size(); i++) {
        usedStreamIds.insert(halConfiguration.streams[i].id);
    }
    {
        Mutex::Autolock _l(mImageConfigMapLock);
        for (ssize_t i = mImageConfigMap.size() - 1; i >= 0; i--)
        {
            auto const streamId = mImageConfigMap.keyAt(i);
            auto const it = usedStreamIds.find(streamId);
            if ( it == usedStreamIds.end() ) {
                //  remove unsued stream
                MY_LOGD("remove unused ImageStreamInfo, streamId:%#" PRIx64 "", streamId);
                mImageConfigMap.removeItemsAt(i);
            }
        }
    }
    //
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
createImageStreamInfo(
    const Stream& rStream,
    HalStream& rOutStream
)   const -> AppImageStreamInfo*
{
    int err = OK;
    const bool isSecureCameraDevice = ((MAKE_HalLogicalDeviceList()->getSupportedFeature(
                mCommonInfo->mInstanceId) & DEVICE_FEATURE_SECURE_CAMERA) &
                DEVICE_FEATURE_SECURE_CAMERA)>0;
    const bool isSecureUsage = ((rStream.usage & GRALLOC_USAGE_PROTECTED) & GRALLOC_USAGE_PROTECTED)>0;
    const bool isSecureFlow = isSecureCameraDevice && isSecureUsage;
    const bool isVedioStream = ((rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) &
                GRALLOC_USAGE_HW_VIDEO_ENCODER) > 0;
    if (isSecureCameraDevice && !isSecureUsage)
    {
        MY_LOGE("Not support insecure stream for a secure camera device");
        return nullptr;
    }
    //
    // For secure camera device, the HAL client is expected to set
    // GRALLOC1_PRODUCER_USAGE_PROTECTED, meaning that the buffer is protected from
    // direct CPU access outside the isolated execution environment
    // (e.g. TrustZone or Hypervisor-based solution) or
    // being read by non-secure hardware.
    //
    // Moreover, this flag is incompatible with CPU read and write flags.
    MUINT64 secureUsage = (GRALLOC1_PRODUCER_USAGE_PROTECTED|GRALLOC_USAGE_SW_READ_NEVER|GRALLOC_USAGE_SW_WRITE_NEVER);
    MUINT64 const usageForHal = GRALLOC1_PRODUCER_USAGE_CAMERA |
        (isSecureFlow ? (secureUsage) :
         isVedioStream? (GRALLOC_USAGE_SW_READ_OFTEN) :
                        (GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN));
    MUINT64 const usageForHalClient = rStream.usage;
    MUINT64 usageForAllocator = usageForHal | usageForHalClient;
    MY_LOGD("isSecureCameraDevice=%d usageForHal=0x%#" PRIx64 " usageForHalClient=0x%#" PRIx64 "",
            isSecureCameraDevice, usageForHal, usageForHalClient);
    MINT32  const formatToAllocate  = static_cast<MINT32>(rStream.format);
    //
    usageForAllocator = (rStream.streamType==StreamType::OUTPUT) ? usageForAllocator : usageForAllocator | GRALLOC_USAGE_HW_CAMERA_ZSL;
    //
    IGrallocHelper* pGrallocHelper = mCommonInfo->mGrallocHelper;
    mcam::GrallocStaticInfo   grallocStaticInfo;
    mcam::GrallocRequest      grallocRequest;
    grallocRequest.usage  = usageForAllocator;
    if (mspParsedSMVRBatchInfo != nullptr)
    {
       if (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
       {
           grallocRequest.usage |= to32Bits(NSCam::BufferUsage::SMVR);
       }
    }
    grallocRequest.format = formatToAllocate;
    // customization for End2End HDR (NOT support SMVR)
    if( mE2EHDROn == 1 &&
        mspParsedSMVRBatchInfo == nullptr &&
        rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
    {
        MY_LOGD("Add Usage for E2E HDR");
        grallocRequest.usage |= to32Bits(NSCam::BufferUsage::E2E_HDR);
    }
    MY_LOGD("grallocRequest.format=%d, grallocRequest.usage = 0x%x ", grallocRequest.format, grallocRequest.usage);

    if  ( HAL_PIXEL_FORMAT_BLOB == formatToAllocate ) {
        auto const dataspace = (android_dataspace_t)rStream.dataSpace;
        auto const bufferSz  = rStream.bufferSize;
        // For BLOB format with dataSpace DEPTH, this must be zero and and HAL must
        // determine the buffer size based on ANDROID_DEPTH_MAX_DEPTH_SAMPLES.
        if ( HAL_DATASPACE_DEPTH == dataspace ) {
            // should be return in checkStream.
            MY_LOGE("Not support depth dataspace %s", toString(rStream).c_str());
            return nullptr;
        }
        // For BLOB format with dataSpace JFIF, this must be non-zero and represent the
        // maximal size HAL can lock using android.hardware.graphics.mapper lock API.
        else if ( android_dataspace_t::HAL_DATASPACE_V0_JFIF == dataspace ) {
            if ( CC_UNLIKELY(bufferSz==0) ) {
                MY_LOGW("HAL_DATASPACE_V0_JFIF with bufferSize(0)");
                IMetadata::IEntry const& entry = mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_JPEG_MAX_SIZE);
                if  ( entry.isEmpty() ) {
                    MY_LOGW("no static JPEG_MAX_SIZE");
                    grallocRequest.widthInPixels = rStream.width * rStream.height * 2;
                }
                else {
                    grallocRequest.widthInPixels = entry.itemAt(0, Type2Type<MINT32>());
                }
            } else {
                grallocRequest.widthInPixels = bufferSz;
            }
            grallocRequest.heightInPixels = 1;
            MY_LOGI("BLOB with widthInPixels(%d), heightInPixels(%d), bufferSize(%u)",
                    grallocRequest.widthInPixels, grallocRequest.heightInPixels, rStream.bufferSize);
        }
        else if (static_cast<int>(HAL_DATASPACE_JPEG_APP_SEGMENTS) == static_cast<int>(dataspace)) {
            if ( CC_UNLIKELY(bufferSz == 0) ) {
                MY_LOGW("HAL_DATASPACE_JPEG_APP_SEGMENTS with bufferSize(0)");
                IMetadata::IEntry const& entry =
                    mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_HEIC_INFO_MAX_JPEG_APP_SEGMENTS_COUNT);
                if (entry.isEmpty()) {
                    MY_LOGW("no static JPEG_APP_SEGMENTS_COUNT");
                    grallocRequest.widthInPixels = rStream.width * rStream.height;
                } else {
                    size_t maxAppsSegment = entry.itemAt(0, Type2Type<MINT32>());
                    grallocRequest.widthInPixels = maxAppsSegment * (2 + 0xFFFF) + sizeof(struct CameraBlob);
                }
            } else {
                grallocRequest.widthInPixels = bufferSz;
            }
            grallocRequest.heightInPixels = 1;
            MY_LOGI("BLOB with widthInPixels(%d), heightInPixels(%d), bufferSize(%u)",
                    grallocRequest.widthInPixels, grallocRequest.heightInPixels, rStream.bufferSize);
        }
        else {
            if ( bufferSz!=0 )
                grallocRequest.widthInPixels = bufferSz;
            else
                grallocRequest.widthInPixels = rStream.width * rStream.height * 2;
            grallocRequest.heightInPixels = 1;
            MY_LOGW("undefined dataspace(0x%x) with bufferSize(%u) in BLOB format -> %dx%d",
                    static_cast<int>(dataspace), bufferSz, grallocRequest.widthInPixels, grallocRequest.heightInPixels);
        }
    }
    else {
        grallocRequest.widthInPixels  = rStream.width;
        grallocRequest.heightInPixels = rStream.height;
    }
    //
    if( (grallocRequest.usage & to32Bits(NSCam::BufferUsage::E2E_HDR)) &&
        (grallocRequest.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) )
    {
        MY_LOGD("force to re-create stream");
        err = pGrallocHelper->query(&grallocRequest, &grallocStaticInfo, true);
    }
    else
    {
        err = pGrallocHelper->query(&grallocRequest, &grallocStaticInfo);
    }
    if  ( OK != err ) {
        MY_LOGE("IGrallocHelper::query - err:%d(%s)", err, ::strerror(-err));
        return NULL;
    }
    //
    //  stream name = s<stream id>:d<device id>:App:<format>:<hal client usage>
    String8 s8StreamName = String8::format("s%d:d%d:App:", rStream.id, mCommonInfo->mInstanceId);
    String8 const s8FormatAllocated  = pGrallocHelper->queryPixelFormatName(grallocStaticInfo.format);
    switch  (grallocStaticInfo.format)
    {
    case HAL_PIXEL_FORMAT_BLOB:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCRCB_420_SP:
    case HAL_PIXEL_FORMAT_YCBCR_422_I:
    case HAL_PIXEL_FORMAT_NV12:
    case HAL_PIXEL_FORMAT_YCBCR_P010:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RAW10:
    case HAL_PIXEL_FORMAT_RAW16:
    case HAL_PIXEL_FORMAT_Y8:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case HAL_PIXEL_FORMAT_CAMERA_OPAQUE:
        s8StreamName += s8FormatAllocated;
        break;
    case HAL_PIXEL_FORMAT_Y16:
    default:
        MY_LOGE("Unsupported format:0x%x(%s), grallocRequest.format=%d, grallocRequest.usage = %d ",
            grallocStaticInfo.format, s8FormatAllocated.c_str(), grallocRequest.format, grallocRequest.usage);
        return NULL;
    }
    MY_LOGD("grallocStaticInfo.format=%d", grallocStaticInfo.format);
    //
    s8StreamName += ":";
    s8StreamName += pGrallocHelper->queryGrallocUsageName(usageForHalClient);
    //
    IImageStreamInfo::BufPlanes_t bufPlanes;
    bufPlanes.count = grallocStaticInfo.planes.size();
    for (size_t i = 0; i < bufPlanes.count; i++)
    {
        IImageStreamInfo::BufPlane& plane = bufPlanes.planes[i];
        plane.sizeInBytes      = grallocStaticInfo.planes[i].sizeInBytes;
        plane.rowStrideInBytes = grallocStaticInfo.planes[i].rowStrideInBytes;
    }
    //
    rOutStream.id = rStream.id;
    rOutStream.physicalCameraId = rStream.physicalCameraId;
    rOutStream.overrideFormat =
        (  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == static_cast<android_pixel_format_t>(rStream.format)
        //[ALPS03443045] Don't override it since there's a bug in API1 -> HAL3.
        //StreamingProcessor::recordingStreamNeedsUpdate always return true for video stream.
        && (GRALLOC_USAGE_HW_VIDEO_ENCODER & rStream.usage) == 0
        //we don't have input stream's producer usage to determine the real format.
        && StreamType::OUTPUT == rStream.streamType  )
            ? static_cast<decltype(rOutStream.overrideFormat)>(grallocStaticInfo.format)
            : rStream.format;
    rOutStream.producerUsage = (rStream.streamType==StreamType::OUTPUT) ? usageForHal : 0;
    // SMVRBatch: producer usage
    if (mspParsedSMVRBatchInfo != nullptr)
    {
       if (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
       {
           rOutStream.producerUsage |= to32Bits(NSCam::BufferUsage::SMVR);
       }
    }
    // HDR10+: producer usage
    else
    {
        if( (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
            && mE2EHDROn)
        {
           rOutStream.producerUsage |= to32Bits(NSCam::BufferUsage::E2E_HDR);
        }
    }
    rOutStream.consumerUsage = (rStream.streamType==StreamType::OUTPUT) ? 0 : usageForHal;
    rOutStream.maxBuffers    = 1;
    rOutStream.overrideDataSpace = rStream.dataSpace;
    // color profile HAL_DATASPACE_STANDARD_BT2020 is not supported,
    // use BT601 as the overrideDataSpace
    if((rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) > 0 &&
       (rStream.dataSpace & HAL_DATASPACE_STANDARD_BT2020) > 0 ) {
        rOutStream.overrideDataSpace = (android_dataspace_t)(rStream.dataSpace & ~(HAL_DATASPACE_STANDARD_BT2020) | HAL_DATASPACE_STANDARD_BT601_625);
        MY_LOGI("overwrite recording streamId(0x%x) overrideDataSpace from(0x%x) to (0x%x)(remove STANDARD_BT2020 and add STANDARD_BT601_625)",
            rStream.id, rStream.dataSpace, rOutStream.overrideDataSpace);
    }

    auto const& pStreamInfo = getConfigImageStream(rStream.id);

    uint32_t t_imgFmt = 0;
    MINT imgFormat = grallocStaticInfo.format;
    if (grallocStaticInfo.format == HAL_PIXEL_FORMAT_BLOB) {
        if (rStream.dataSpace == static_cast<int32_t>(HAL_DATASPACE_V0_JFIF)){
            t_imgFmt = eImgFmt_JPEG;
        } else if (rStream.dataSpace == static_cast<int32_t>(HAL_DATASPACE_JPEG_APP_SEGMENTS)){
            t_imgFmt = eImgFmt_JPEG_APP_SEGMENT;
        }
        imgFormat = static_cast<MINT>(t_imgFmt);
    }

    imgFormat = (grallocStaticInfo.format == HAL_PIXEL_FORMAT_RAW16) ? mDngFormat : imgFormat;
    imgFormat = (grallocStaticInfo.format == HAL_PIXEL_FORMAT_RAW10) ? mRaw10Format : imgFormat;
    imgFormat = (mAovPipelineConfig == MTK_AOVSERVICE_FEATURE_PARTIAL_PIPELINE) ?
            HAL_PIXEL_FORMAT_Y8 : imgFormat;

    MINT allocImgFormat = static_cast<MINT>(grallocStaticInfo.format);
    IImageStreamInfo::BufPlanes_t allocBufPlanes = bufPlanes;

    // SMVRBatch:: handle blob layout
    uint32_t oneImgTotalSizeInBytes_32align = 0;
    if (mspParsedSMVRBatchInfo != nullptr)
    {
        if (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
        {
            uint32_t oneImgTotalSizeInBytes = 0;
            uint32_t oneImgTotalStrideInBytes = 0;

            allocBufPlanes.count = 1;
            allocImgFormat = static_cast<MINT>(HAL_PIXEL_FORMAT_BLOB); // for smvr-batch mode

            for (size_t i = 0; i < bufPlanes.count; i++)
            {
                MY_LOGD("SMVRBatch: idx=%zu, (sizeInBytes, rowStrideInBytes)=(%zu,%zu)", i, grallocStaticInfo.planes[i].sizeInBytes, grallocStaticInfo.planes[i].rowStrideInBytes);
                oneImgTotalSizeInBytes += grallocStaticInfo.planes[i].sizeInBytes;
    //                oneImgTotalStrideInBytes += grallocStaticInfo.planes[i].rowStrideInBytes;
                oneImgTotalStrideInBytes = oneImgTotalSizeInBytes;
            }
            oneImgTotalSizeInBytes_32align = (((oneImgTotalSizeInBytes-1)>>5)+1)<<5;
            allocBufPlanes.planes[0].sizeInBytes = oneImgTotalSizeInBytes_32align  * mspParsedSMVRBatchInfo->p2BatchNum;
            allocBufPlanes.planes[0].rowStrideInBytes = allocBufPlanes.planes[0].sizeInBytes;

            // debug message
            MY_LOGD_IF(mspParsedSMVRBatchInfo->logLevel >= 2, "SMVRBatch: %s: isVideo=%" PRIu64 " \n"
                "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%#" PRIx64 ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
                "\t grallocStaticInfo(format=0x%x) \n"
                "\t HalStream-info(producerUsage= %#" PRIx64 ",  consumerUsage= %#" PRIx64 ", overrideFormat=0x%x ) \n"
                "\t Blob-info(imgFmt=0x%x, allocImgFmt=0x%x, vOutBurstNum=%d, oneImgTotalSizeInBytes(%d, 32align-%d ), oneImgTotalStrideInBytes=%d, allocBufPlanes(size=%zu, sizeInBytes=%zu, rowStrideInBytes=%zu) \n"
                "\t Misc-info(usageForAllocator=%#" PRIx64 ", GRALLOC1_USAGE_SMVR=%#" PRIx64 ") \n"
                , __FUNCTION__, (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
                , rStream.id, rStream.format, rStream.usage, StreamType::OUTPUT, rStream.streamType
                , static_cast<android_pixel_format_t>(grallocStaticInfo.format)
                , rOutStream.producerUsage, rOutStream.consumerUsage, rOutStream.overrideFormat
                , imgFormat, allocImgFormat, mspParsedSMVRBatchInfo->p2BatchNum, oneImgTotalSizeInBytes, oneImgTotalSizeInBytes_32align, oneImgTotalStrideInBytes, allocBufPlanes.count, allocBufPlanes.planes[0].sizeInBytes, allocBufPlanes.planes[0].rowStrideInBytes
                , usageForAllocator, NSCam::BufferUsage::SMVR
                );
        }
    } else if (grallocStaticInfo.format == HAL_PIXEL_FORMAT_RAW16  // Alloc Format: Raw16 -> Blob
               || grallocStaticInfo.format == HAL_PIXEL_FORMAT_RAW10) { // Alloc Format: Raw10 -> Blob
        allocBufPlanes.count = 1;
        allocImgFormat = static_cast<MINT>(HAL_PIXEL_FORMAT_BLOB);
        allocBufPlanes.planes[0].sizeInBytes      = grallocStaticInfo.planes[0].sizeInBytes;
        allocBufPlanes.planes[0].rowStrideInBytes = grallocStaticInfo.planes[0].sizeInBytes;
        MY_LOGD("Raw16: (sizeInBytes, rowStrideInBytes)=(%zu,%zu)", allocBufPlanes.planes[0].sizeInBytes, allocBufPlanes.planes[0].rowStrideInBytes);
    }

    NSCam::ImageBufferInfo imgBufferInfo;
    imgBufferInfo.count   = 1;
    imgBufferInfo.bufStep = 0;
    imgBufferInfo.startOffset = 0;
    if (mspParsedSMVRBatchInfo != nullptr)
    {
        // SMVRBatch:: buffer offset setting
        if (rStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
        {
            imgBufferInfo.count   = mspParsedSMVRBatchInfo->p2BatchNum;
            imgBufferInfo.bufStep = oneImgTotalSizeInBytes_32align;
        }
    }

    // !!NOTES: bufPlanes, imgFormat should be maintained as original format, ref: Camera3ImageStreamInfo.cpp
    imgBufferInfo.bufPlanes = bufPlanes;
    imgBufferInfo.imgFormat = imgFormat;

    imgBufferInfo.imgWidth  = rStream.width;
    imgBufferInfo.imgHeight = rStream.height;

    MINT sensorId = findStreamSensorId(mCommonInfo->mInstanceId, rStream);

    AppImageStreamInfo::CreationInfo creationInfo =
    {
        .mStreamName        = s8StreamName,
        .mImgFormat         = allocImgFormat,                 /* alloc stage, TBD if it's YUV format for batch mode SMVR */
        .mOriImgFormat      = (pStreamInfo.get())? pStreamInfo->getOriImgFormat() : formatToAllocate,
        .mRealAllocFormat   = grallocStaticInfo.format,
        .mStream            = rStream,
        .mHalStream         = rOutStream,
        .mImageBufferInfo   = imgBufferInfo,
        .mSensorId          = sensorId,
        .mSecureInfo        = SecureInfo{
            isSecureFlow ? SecType::mem_protected : SecType::mem_normal }
    };
    /* alloc stage, TBD if it's YUV format for batch mode SMVR */
    creationInfo.mvbufPlanes = allocBufPlanes;

    //fill in the secure info if and only if it's a secure camera device
    // debug message
    MY_LOGD("rStream.v_32.usage(0x%" PRIx64 ") \n"
            "\t rStream-Info(sid=0x%x, format=0x%x, usage=0x%" PRIx64 ", StreamType::OUTPUT=0x%x, streamType=0x%x) \n"
            "\t grallocStaticInfo(format=0x%x) \n"
            "\t HalStream-info(producerUsage= %#" PRIx64 ",  consumerUsage= %#" PRIx64 ", overrideFormat=0x%x ) \n"
            "\t Blob-info(imgFmt=0x%x, allocImgFmt=0x%x, allocBufPlanes(size=%zu, sizeInBytes=%zu, rowStrideInBytes=%zu) \n"
            "\t Misc-info(usageForAllocator=%#" PRIx64 ", NSCam::BufferUsage::PROT_CAMERA_BIDIRECTIONAL=%#" PRIx64 ") NSCam::BufferUsage::E2E_HDR=%#" PRIx64 ")\n"
            "\t creationInfo(secureInfo=0x%x)\n"
            , rStream.usage
            , rStream.id, rStream.format, rStream.usage
            , StreamType::OUTPUT, rStream.streamType
            , static_cast<android_pixel_format_t>(grallocStaticInfo.format)
            , rOutStream.producerUsage
            , rOutStream.consumerUsage
            , rOutStream.overrideFormat
            , imgFormat, allocImgFormat, allocBufPlanes.count, allocBufPlanes.planes[0].sizeInBytes
            , allocBufPlanes.planes[0].rowStrideInBytes
            , usageForAllocator, NSCam::BufferUsage::PROT_CAMERA_BIDIRECTIONAL, NSCam::BufferUsage::E2E_HDR
            , toLiteral(creationInfo.mSecureInfo.type)
            );

    AppImageStreamInfo* pStream = new AppImageStreamInfo(creationInfo);
    return pStream;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
createMetaStreamInfo(
    char const* metaName,
    StreamId_T  suggestedStreamId,
    MINT        physicalId
)   const -> AppMetaStreamInfo*
{
    return new AppMetaStreamInfo(
        metaName,
        suggestedStreamId,
        eSTREAMTYPE_META_IN,
        0,
        0,
        physicalId
    );
}



/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
extractSMVRBatchInfo(
    const StreamConfiguration& requestedConfiguration
)   -> std::shared_ptr<AppConfigUtil::ParsedSMVRBatchInfo>
{
    IMetadata metaSessionParams;
    if ( ! mCommonInfo->mMetadataConverter->convert(requestedConfiguration.sessionParams, metaSessionParams) )
    {
        MY_LOGE("SMVRBatch::Bad Session parameters");
        return nullptr;
    }
    const MUINT32 operationMode = (const MUINT32) requestedConfiguration.operationMode;

    MINT32 customP2BatchNum = 1;
    MINT32 p2IspBatchNum = 1;
    std::shared_ptr<ParsedSMVRBatchInfo> pParsedSMVRBatchInfo = nullptr;
    int isFromApMeta = 0;

    IMetadata::IEntry const& entry = metaSessionParams.entryFor(MTK_SMVR_FEATURE_SMVR_MODE);

    if (getLogLevel()>=2)
    {
        MY_LOGD("SMVRBatch: chk metaSessionParams.count()=%d, MTK_SMVR_FEATURE_SMVR_MODE: count: %d", metaSessionParams.count(), entry.count());
        metaSessionParams.dump();
    }

    if  ( !entry.isEmpty() && entry.count() >= 2 )
    {
        isFromApMeta = 1;
        pParsedSMVRBatchInfo = std::make_shared<ParsedSMVRBatchInfo>();
        if  ( CC_UNLIKELY(pParsedSMVRBatchInfo == nullptr) ) {
            CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>", __FUNCTION__);
            return nullptr;
        }
        // get image w/h
        for ( size_t i = 0; i < requestedConfiguration.streams.size(); i++ )
        {
            const auto& srcStream = requestedConfiguration.streams[i];
    //        if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
            if (pParsedSMVRBatchInfo->imgW == 0) // !!NOTES: assume previewSize = videoOutSize
            {
                // found
                pParsedSMVRBatchInfo->imgW = srcStream.width;
                pParsedSMVRBatchInfo->imgH = srcStream.height;
//                break;
            }
            MY_LOGD("SMVRBatch: vImageStreams[%zu]=%dx%d, isVideo=%" PRIu64 "", i, srcStream.width, srcStream.height, (srcStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER));
        }

        if  ( !entry.isEmpty() && entry.count() >= 2)
        {
            pParsedSMVRBatchInfo->maxFps = entry.itemAt(0, Type2Type< MINT32 >()); // meta[0]: LmaxFps
            customP2BatchNum = entry.itemAt(1, Type2Type< MINT32 >());             // meta[1]: customP2BatchNum
        }
        pParsedSMVRBatchInfo->maxFps = ::property_get_int32("vendor.debug.smvrb.maxFps", pParsedSMVRBatchInfo->maxFps);
//        if (pParsedSMVRBatchInfo->maxFps <= 120)
//        {
//            MY_LOGE("SMVRBatch: !!err: only support slow motion more than 120fps: curr-maxFps=%d", pParsedSMVRBatchInfo->maxFps);
//            return nullptr;
//        }
        // determine final P2BatchNum
        #define min(a,b)  ((a) < (b) ? (a) : (b))
        MUINT32 vOutSize = pParsedSMVRBatchInfo->imgW * pParsedSMVRBatchInfo->imgH;
        if (vOutSize <= 640*480) // vga
        {
            p2IspBatchNum = ::property_get_int32("ro.vendor.smvr.p2batch.vga", 1);
        }
        else if (vOutSize <= 1280*736) // hd
        {
            p2IspBatchNum = ::property_get_int32("ro.vendor.smvr.p2batch.hd", 1);
        }
        else if (vOutSize <= 1920*1088) // fhd
        {
            p2IspBatchNum = ::property_get_int32("ro.vendor.smvr.p2batch.fhd", 1);
        }
        else
        {
           p2IspBatchNum = 1;
        }
        // change p2IspBatchNum by debug adb if necessary
        p2IspBatchNum = ::property_get_int32("vendor.debug.smvrb.P2BatchNum", p2IspBatchNum);
        // final P2BatchNum
        pParsedSMVRBatchInfo->p2BatchNum = min(p2IspBatchNum, customP2BatchNum);
        #undef min

        // P1BatchNum
        pParsedSMVRBatchInfo->p1BatchNum = pParsedSMVRBatchInfo->maxFps/30;
        // operatioin mode
        pParsedSMVRBatchInfo->opMode = operationMode;

        // log level
        pParsedSMVRBatchInfo->logLevel = ::property_get_int32("vendor.debug.smvrb.loglevel", 0);
    }
    else
    {
        MINT32 propSmvrBatchEnable = ::property_get_int32("vendor.debug.smvrb.enable", 0);
        if (propSmvrBatchEnable)
        {
            pParsedSMVRBatchInfo = std::make_shared<ParsedSMVRBatchInfo>();
            if  ( CC_UNLIKELY(pParsedSMVRBatchInfo == nullptr) ) {
                CAM_LOGE("[%s] Fail on make_shared<pParsedSMVRBatchInfo>", __FUNCTION__);
                return nullptr;
            }

            // get image w/h
            for ( size_t i = 0; i < requestedConfiguration.streams.size(); i++ )
            {
                const auto& srcStream = requestedConfiguration.streams[i];
        //        if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
                if (pParsedSMVRBatchInfo->imgW == 0) // !!NOTES: assume previewSize = videoOutSize
                {
                    // found
                    pParsedSMVRBatchInfo->imgW = srcStream.width;
                    pParsedSMVRBatchInfo->imgH = srcStream.height;
    //                break;
                }
                MY_LOGD("SMVRBatch: vImageStreams[%lu]=%dx%d, isVideo=%" PRIu64 "", i, srcStream.width, srcStream.height, (srcStream.usage & GRALLOC_USAGE_HW_VIDEO_ENCODER));
            }

            pParsedSMVRBatchInfo->maxFps = ::property_get_int32("vendor.debug.smvrb.maxFps", pParsedSMVRBatchInfo->maxFps);
//            if (pParsedSMVRBatchInfo->maxFps <= 120)
//            {
//                MY_LOGE("SMVRBatch: !!err: only support slow motion more than 120fps: curr-maxFps=%d", pParsedSMVRBatchInfo->maxFps);
//                return nullptr;
//            }

            pParsedSMVRBatchInfo->p2BatchNum = ::property_get_int32("vendor.debug.smvrb.P2BatchNum", 1);
            pParsedSMVRBatchInfo->p1BatchNum = ::property_get_int32("vendor.debug.smvrb.P1BatchNum", 1);
            pParsedSMVRBatchInfo->opMode     = operationMode;
            pParsedSMVRBatchInfo->logLevel   = ::property_get_int32("vendor.debug.smvrb.loglevel", 0);
        }
    }

    if (pParsedSMVRBatchInfo != nullptr)
    {
         MY_LOGD("SMVRBatch: isFromApMeta=%d, vOutImg=%dx%d, meta-info(maxFps=%d, customP2BatchNum=%d), p2IspBatchNum=%d, final-P2BatchNum=%d, p1BatchNum=%d, opMode=%d, logLevel=%d",
             isFromApMeta,
             pParsedSMVRBatchInfo->imgW, pParsedSMVRBatchInfo->imgH,
             pParsedSMVRBatchInfo->maxFps, customP2BatchNum,
             p2IspBatchNum, pParsedSMVRBatchInfo->p2BatchNum,
             pParsedSMVRBatchInfo->p1BatchNum,
             pParsedSMVRBatchInfo->opMode,
             pParsedSMVRBatchInfo->logLevel
         );

    }
    else
    {
         MY_LOGD("SMVRBatch: no need");
    }

    return pParsedSMVRBatchInfo;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
extractE2EHDRInfo(
    const StreamConfiguration& requestedConfiguration
)   -> MINT32
{
    IMetadata metaSessionParams;
    MINT32 iEnableE2EHDR = 0;
    if ( ! mCommonInfo->mMetadataConverter->convert(requestedConfiguration.sessionParams, metaSessionParams) )
    {
        MY_LOGE("SMVRBatch::Bad Session parameters");
        return 0;
    }
    IMetadata::IEntry const& entry = metaSessionParams.entryFor(MTK_STREAMING_FEATURE_HDR10);
    if( entry.isEmpty() )
    {
        MY_LOGW("query tag MTK_STREAMING_FEATURE_HDR10 failed, E2E HDR is not applied");
    }
    else
    {
        iEnableE2EHDR = entry.itemAt(0, Type2Type< MINT32 >());
    }
    return iEnableE2EHDR;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
extractDNGFormatInfo(
    const StreamConfiguration& requestedConfiguration
) -> MINT
{
    IMetadata metaSessionParams;
    MINT32 dngBit = 0;
    MINT imageFormat = eImgFmt_UNKNOWN;
    if ( requestedConfiguration.sessionParams != nullptr &&
         ! mCommonInfo->mMetadataConverter->convert(requestedConfiguration.sessionParams, metaSessionParams) )
    {
        MY_LOGE("DNGFormat::Bad Session parameters");
        return imageFormat;
    }
    // todo: confirm these two metadata tags
    IMetadata::IEntry const& vendorTagEntry = metaSessionParams.entryFor(MTK_SENSOR_INFO_WHITE_LEVEL);
    IMetadata::IEntry const& staticMetaEntry =
        mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics().entryFor(MTK_SENSOR_INFO_WHITE_LEVEL);
    if( !vendorTagEntry.isEmpty() )
    {
        dngBit = vendorTagEntry.itemAt(0, Type2Type<MINT32>());
        MY_LOGD("found DNG bit size = %d in vendor tag", dngBit);
    }
    else if ( !staticMetaEntry.isEmpty() )
    {
        dngBit = staticMetaEntry.itemAt(0, Type2Type<MINT32>());
        MY_LOGD("found DNG bit size = %d in static meta", dngBit);
    }
    else
    {
        MY_LOGD("no need to overwrite stream format for DNG");
    }
    //
#define ADD_CASE(bitDepth) \
    case ((1<<bitDepth)-1): \
        imageFormat = eImgFmt_BAYER##bitDepth##_UNPAK; \
        break;
    //
    switch (dngBit)
    {
    ADD_CASE(8)
    ADD_CASE(10)
    ADD_CASE(12)
    ADD_CASE(14)
    ADD_CASE(15)
    ADD_CASE(16)
    default:
        MY_LOGE("unsupported DNG bit size = %d in metadata.", dngBit);
    }
#undef ADD_CASE
    return imageFormat;
}



/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
extractRAW10FormatInfo(
    const StreamConfiguration& requestedConfiguration
) -> MINT
{
    IMetadata metaSessionParams;
    MINT32 raw10ConvertFmt = MTK_CONTROL_CAPTURE_RAW10_CONVERT_MIPI_RAW10;
    MINT imageFormat = eImgFmt_BAYER10_MIPI;
    if ( requestedConfiguration.sessionParams != nullptr &&
         ! mCommonInfo->mMetadataConverter->convert(requestedConfiguration.sessionParams, metaSessionParams) )
    {
        MY_LOGE("RAW10Format::Bad Session parameters");
        return imageFormat;
    }
    IMetadata::IEntry const& vendorTagEntry =
        metaSessionParams.entryFor(MTK_CONTROL_CAPTURE_RAW10_CONVERT_FMT);
    if (!vendorTagEntry.isEmpty()) {
        raw10ConvertFmt = vendorTagEntry.itemAt(0, Type2Type<MINT32>());
        MY_LOGD("found RAW10 convert format = %d", raw10ConvertFmt);
        switch (raw10ConvertFmt) {
        case MTK_CONTROL_CAPTURE_RAW10_CONVERT_MIPI_RAW10:
            imageFormat = eImgFmt_BAYER10_MIPI;
            break;
        case MTK_CONTROL_CAPTURE_RAW10_CONVERT_MTK_BAYER10:
            imageFormat = eImgFmt_BAYER10;
            break;
        default:
            MY_LOGE("unsupported RAW10 convert format = %d", raw10ConvertFmt);
        }
    } else {
        MY_LOGD("default RAW10 format = BAYER10_MIPI");
    }
    return imageFormat;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
extractAOVInfo(
    const StreamConfiguration& requestedConfiguration
) -> MINT32
{
    IMetadata metaSessionParams;
    MINT32 pipelineConfig = MTK_AOVSERVICE_FEATURE_FULL_PIPELINE;
    if ( requestedConfiguration.sessionParams != nullptr &&
         ! mCommonInfo->mMetadataConverter->convert(requestedConfiguration.sessionParams, metaSessionParams) )
    {
        MY_LOGE("Bad Session parameters");
        return pipelineConfig;
    }
    IMetadata::IEntry const& aovModeEntry =
            metaSessionParams.entryFor(MTK_AOVSERVICE_FEATURE_AOV_MODE);
    if (!aovModeEntry.isEmpty() &&
            aovModeEntry.itemAt(0, Type2Type<MINT32>()) == MTK_AOVSERVICE_FEATURE_AOV_ON) {
        IMetadata::IEntry const& pipelineConfigEntry =
                metaSessionParams.entryFor(MTK_AOVSERVICE_FEATURE_PIPELINE_CONFIG);
        if (!pipelineConfigEntry.isEmpty()) {
            pipelineConfig = pipelineConfigEntry.itemAt(0, Type2Type<MINT32>());
            MY_LOGD("found pipelineConfig = %d in vendor tag", pipelineConfig);
        }
    }
    return pipelineConfig;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
addConfigStream(
    AppImageStreamInfo* pStreamInfo/*,
    bool keepBufferCache*/
) -> void
{
    Mutex::Autolock _l(mImageConfigMapLock);
    ssize_t const index = mImageConfigMap.indexOfKey(pStreamInfo->getStreamId());
    if  ( index >= 0 ) {
        mImageConfigMap.replaceValueAt(index, pStreamInfo);
        // 2nd modification
        // auto& item = mImageConfigMap.editValueAt(index);
        // item.pStreamInfo = pStreamInfo;

        // 1st modification
        // auto& item = mImageConfigMap.editValueAt(index);
        // if  ( keepBufferCache ) {
            // item.pStreamInfo = pStreamInfo;
            // item.vItemFrameQueue.clear();
        // }
        // else {
        //     item.pStreamInfo = pStreamInfo;
        //     item.vItemFrameQueue.clear();

        //     // MY_LOGF_IF(item.pBufferHandleCache==nullptr, "streamId:%#" PRIx64 " has no buffer handle cache", pStreamInfo->getStreamId());
        //     // item.pBufferHandleCache->clear();
        // }
    }
    else {
        mImageConfigMap.add(pStreamInfo->getStreamId(), pStreamInfo );
        // 2nd modification
        // mImageConfigMap.add(
        //     pStreamInfo->getStreamId(),
        //     ImageConfigItem{
        //         .pStreamInfo = pStreamInfo,
        //         // .pBufferHandleCache = std::make_shared<BufferHandleCache>( pStreamInfo->getStreamId() ),
        // });
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
addConfigStream(
    AppMetaStreamInfo* pStreamInfo
) -> void
{
    // MetaConfigItem item;
    // item.pStreamInfo = pStreamInfo;
    // mMetaConfigMap.add(pStreamInfo->getStreamId(), item);
    mMetaConfigMap.add(pStreamInfo->getStreamId(), pStreamInfo);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
getConfigImageStream(
    StreamId_T streamId
) const -> android::sp<AppImageStreamInfo>
{
    Mutex::Autolock _l(mImageConfigMapLock);
    ssize_t const index = mImageConfigMap.indexOfKey(streamId);
    if  ( 0 <= index ) {
        return mImageConfigMap.valueAt(index);
        // return mImageConfigMap.valueAt(index).pStreamInfo;
    }
    return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
getConfigMetaStream(
    StreamId_T streamId
) const -> sp<AppMetaStreamInfo>
{
    ssize_t const index = mMetaConfigMap.indexOfKey(streamId);
    if  ( 0 <= index ) {
        return mMetaConfigMap.valueAt(index);
        // return mMetaConfigMap.valueAt(index).pStreamInfo;
    }
    else{
        MY_LOGE("Cannot find MetaStreamInfo for stream %#" PRIx64 " in mMetaConfigMap", streamId);
        return nullptr;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
getAppStreamInfoBuilderFactory() const -> std::shared_ptr<IStreamInfoBuilderFactory>
{
    return mStreamInfoBuilderFactory;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
AppConfigUtil::
getConfigMap(
    ImageConfigMap& imageConfigMap,
    MetaConfigMap& metaConfigMap
) -> void
{
    imageConfigMap.clear();
    metaConfigMap.clear();
    for (size_t i = 0; i < mImageConfigMap.size(); ++i) {
        imageConfigMap.add(mImageConfigMap.keyAt(i), mImageConfigMap.valueAt(i));
    }
    for (size_t i = 0; i < mMetaConfigMap.size(); ++i) {
        metaConfigMap.add(mMetaConfigMap.keyAt(i), mMetaConfigMap.valueAt(i));
    }
}
