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
#include "AppStreamBufferBuilder.h"
#include <AppRequestUtil.h>
//
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam3/pipeline/stream/StreamId.h>
//
using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::Utils::ULog;
using namespace NSCam::v3::Utils;

#define ThisNamespace   AppRequestUtil

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

#define MY_DEBUG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mDebugPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_WARN(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mWarningPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR_ULOG(level, fmt, arg...) \
    do { \
        CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mErrorPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_ERROR(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
        mCommonInfo->mErrorPrinter->printFormatLine(#level" [%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, ##arg); \
    } while(0)

#define MY_LOGV(...)                MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...)                MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...)                MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...)                MY_WARN (W, __VA_ARGS__)
#define MY_LOGE(...)                MY_ERROR_ULOG(E, __VA_ARGS__)
#define MY_LOGA(...)                MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)

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
auto
IAppRequestUtil::
create(
    const CreationInfo& creationInfo
) -> IAppRequestUtil*
{
    auto pInstance = new AppRequestUtil(creationInfo);
    if  ( ! pInstance ) {
        delete pInstance;
        return nullptr;
    }
    return pInstance;
}


/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::
AppRequestUtil(
    const CreationInfo& creationInfo
)
    : mInstanceName{std::to_string(creationInfo.mInstanceId) + "-AppRequestUtil"}
    , mCommonInfo(std::make_shared<CommonInfo>())
    , mLatestSettings()
{
    if (   creationInfo.mErrorPrinter         == nullptr
        || creationInfo.mWarningPrinter       == nullptr
        || creationInfo.mDebugPrinter         == nullptr
        || creationInfo.mCameraDeviceCallback == nullptr
        || creationInfo.mMetadataProvider     == nullptr
        || creationInfo.mMetadataConverter    == nullptr )
    {
        MY_LOGE("mErrorPrinter:%p mWarningPrinter:%p mDebugPrinter:%p mCameraDeviceCallback:%p mMetadataProvider:%p mMetadataConverter:%p",
            creationInfo.mErrorPrinter.get(),
            creationInfo.mWarningPrinter.get(),
            creationInfo.mDebugPrinter.get(),
            creationInfo.mCameraDeviceCallback.get(),
            creationInfo.mMetadataProvider.get(),
            creationInfo.mMetadataConverter.get());
        mCommonInfo = nullptr;
        return;
    }

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
            loglevel = ::property_get_int32("vendor.debug.camera.log.AppStreamMgr", 0);
        }
        //
        //
        mCommonInfo->mLogLevel = loglevel;
        mCommonInfo->mInstanceId = creationInfo.mInstanceId;
        mCommonInfo->mErrorPrinter = creationInfo.mErrorPrinter;
        mCommonInfo->mWarningPrinter = creationInfo.mWarningPrinter;
        mCommonInfo->mDebugPrinter = creationInfo.mDebugPrinter;
        mCommonInfo->mDeviceCallback = creationInfo.mCameraDeviceCallback;
        mCommonInfo->mMetadataProvider = creationInfo.mMetadataProvider;
        mCommonInfo->mPhysicalMetadataProviders = creationInfo.mPhysicalMetadataProviders;
        mCommonInfo->mMetadataConverter = creationInfo.mMetadataConverter;
        mCommonInfo->mGrallocHelper = pGrallocHelper;
    }

    MY_LOGD("mImageConfigMap.size()=%lu, mMetaConfigMap.size()=%lu", mImageConfigMap.size(), mMetaConfigMap.size());

    mIonDevice = IIonDeviceProvider::get()->makeIonDevice("AppStreamMgr.AppRequestUtil", 0);
    if ( CC_UNLIKELY( mIonDevice == nullptr) ) {
        MY_LOGE("fail to make IonDevice");
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
destroy() -> void
{
    // mRequestMetadataQueue = nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
setConfigMap(
    ImageConfigMap& imageConfigMap,
    MetaConfigMap& metaConfigMap
) -> void
{
    Mutex::Autolock _l(mImageConfigMapLock);
    // reset first
    reset();
    // set Config Map
    mImageConfigMap.clear();
    mMetaConfigMap.clear();
    for (size_t i = 0; i < metaConfigMap.size(); i++) {
        mMetaConfigMap.add(metaConfigMap.keyAt(i), metaConfigMap.valueAt(i));
    }
    for (size_t i = 0; i < imageConfigMap.size(); i++) {
        mImageConfigMap.add(imageConfigMap.keyAt(i), imageConfigMap.valueAt(i));
    }
    MY_LOGD("[debug] mImageConfigMap.size()=%lu, mMetaConfigMap.size()=%lu",
                mImageConfigMap.size(), mMetaConfigMap.size());
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
reset() -> void
{
    //  An emtpy settings buffer cannot be used as the first submitted request
    //  after a configure_streams() call.
    mLatestSettings.clear();
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
createRequests(
    const vector<CaptureRequest>& requests,
    android::Vector<Request>& rRequests,
    const ImageConfigMap* imgCfgMap,
    const MetaConfigMap* metaCfgMap
)   -> int
{
    CAM_ULOGM_FUNCLIFE();

    if  ( requests.size() == 0 ) {
        MY_LOGE("empty requests list");
        return  -EINVAL;
    }

    int err = OK;
    rRequests.resize(requests.size());
    size_t i = 0;
    for (; i < requests.size(); i++) {
        err = checkOneRequest(requests[i]);
        if  ( OK != err ) {
            break;
        }

        // MY_LOGD("[debug] vBufferCache.size()=%zu", vBufferCache.size());
        err = createOneRequest(requests[i], rRequests.editItemAt(i), imgCfgMap, metaCfgMap);
        if  ( OK != err ) {
            break;
        }
    }

    if  ( OK != err ) {
        MY_LOGE("something happened - frameNo:%u request:%zu/%zu",
            requests[i].frameNumber, i, requests.size());
        return err;
    }

    return  OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
checkOneRequest(const CaptureRequest& request) const -> int
{
    //  there are 0 output buffers
    if ( request.outputBuffers.size() == 0 )
    {
        MY_LOGE("[frameNo:%u] outputBuffers.size()==0", request.frameNumber);
        return  -EINVAL;
    }
    //
    //  not allowed if the fmq size is zero and settings are NULL w/o the lastest setting.

    if ( request.settings==nullptr && mLatestSettings.isEmpty() )
    {
        MY_LOGE("[frameNo:%u] NULL request settings; "
                "however most-recently submitted request is also NULL after configureStreams",
                request.frameNumber);
        return  -EINVAL;
    }
    //
    return  OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
createOneRequest(
    const CaptureRequest& request,
    Request& rRequest,
    const ImageConfigMap* imgCfgMap,
    const MetaConfigMap* metaCfgMap
)   -> int
{
    auto queryBufferName = [=](auto const& buffer) -> std::string {
        return ("r"+std::to_string(request.frameNumber)+":b"+std::to_string(buffer.bufferId));
    };

    auto replaceStreamInfo = [&](sp<AppImageStreamInfo>& pStreamInfo) -> void {
        // TODO: prepare customization streaminfo for future requestStreamBuffer call
        int reqFormat  = pStreamInfo->getAllocImgFormat();
        size_t stride = 0;
        if (pStreamInfo->getOriImgFormat() == static_cast<int>(HAL_PIXEL_FORMAT_RAW16) ) {
            RawBufferInfo rawInfo = {0, 0, 0};
            // query info from metadata
            {
                IMetadata::IEntry const& e1 = mLatestSettings.entryFor(MTK_CONTROL_CAPTURE_PACKED_RAW_ENABLE);
                if ( ! e1.isEmpty() ) {
                    rawInfo.packedRaw = e1.itemAt(0, Type2Type<MINT32>());
                    MY_LOGD("packedRaw: %d", rawInfo.packedRaw);
                }

                IMetadata::IEntry const& e2 = mLatestSettings.entryFor(MTK_CONTROL_CAPTURE_RAW_BPP);
                if ( ! e2.isEmpty() ) {
                    rawInfo.bpp = e2.itemAt(0, Type2Type<MINT32>());
                    MY_LOGD("bppOfRaw: %d", rawInfo.bpp);
                }

                IMetadata::IEntry const& e3 = mLatestSettings.entryFor(MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE);
                if ( ! e3.isEmpty() ) {
                    rawInfo.processed = e3.itemAt(0, Type2Type<MINT32>());
                    MY_LOGD("processedRaw: %d", rawInfo.processed);
                }
            }
            if( rawInfo.packedRaw || rawInfo.bpp != 0 ) {
                if (rawInfo.packedRaw) {
                    reqFormat = eImgFmt_BAYER10;
                    MSize   imgoSize = pStreamInfo->getImgSize();
                    MINT sensorId = pStreamInfo->getPhysicalCameraId() != -1 ?
                                    pStreamInfo->getPhysicalCameraId() : 0;
                    std::shared_ptr<NSCamHW::HwInfoHelper> infohelper =
                        std::make_shared<NSCamHW::HwInfoHelper>(sensorId);
                    if (OK != infohelper->alignPass1HwLimitation(reqFormat, MTRUE, imgoSize, stride)) {
                        MY_LOGE("cannot align to hw limitation");
                    }
                } else if (rawInfo.bpp != 0) {
#define ADD_CASE(bitDepth) \
    case (bitDepth): \
        reqFormat = eImgFmt_BAYER##bitDepth##_UNPAK; \
        break;
                    switch (rawInfo.bpp) {
                    ADD_CASE(8)
                    ADD_CASE(10)
                    ADD_CASE(12)
                    ADD_CASE(14)
                    ADD_CASE(15)
                    ADD_CASE(16)
                    default:
                        MY_LOGE("unsupported MTK_CONTROL_CAPTURE_RAW_BPP = %d in request setting.", rawInfo.bpp);
                    }
#undef ADD_CASE
                }
                sp<AppImageStreamInfo> pPackedRawStreamInfo = new AppImageStreamInfo(*(pStreamInfo.get()), reqFormat, stride);
                pStreamInfo = pPackedRawStreamInfo; // replace request streamInfo
                MY_LOGD("change raw16 to format:%x, allocFormat:%x", pStreamInfo->getImgFormat(), pStreamInfo->getAllocImgFormat());
            }
        }
    };

    if ( imgCfgMap==nullptr ) {
        MY_LOGE("Bad input params");
        return -ENODEV;
    }

    CAM_ULOGM_FUNCLIFE();
    //
    std::stringstream os;
    rRequest.requestNo = request.frameNumber;
    os << "requestNumber[" << rRequest.requestNo << "]:";
    //
    //  vInputMetaBuffers <- request.settings
    {
        sp<IMetaStreamInfo> pStreamInfo =  getConfigMetaStream(eSTREAMID_END_OF_FWK, metaCfgMap);
        bool isRepeating = false;
        //
        if ( request.settings!=nullptr ) {   // read settings from metadata settings.
            isRepeating = false;
            mLatestSettings.clear();
            CAM_ULOGM_TAG_BEGIN("convert: request.settings -> mLatestSettings");
            if ( ! mCommonInfo->mMetadataConverter->convert(request.settings, mLatestSettings) ) {
                MY_LOGE("frameNo:%u convert: request.settings -> mLatestSettings", request.frameNumber);
                return -ENODEV;
            }
            CAM_ULOGM_TAG_END();
        }
        else {
            /**
             * As a special case, a NULL settings buffer indicates that the
             * settings are identical to the most-recently submitted capture request. A
             * NULL buffer cannot be used as the first submitted request after a
             * configure_streams() call.
             */
            isRepeating = true;
            MY_LOGD_IF(getLogLevel() >= 1,
                "frameNo:%u NULL settings -> most-recently submitted capture request",
                request.frameNumber);
        }

        sp<AppMetaStreamBuffer> pStreamBuffer = createMetaStreamBuffer(pStreamInfo, mLatestSettings, isRepeating);
        if ( CC_LIKELY(pStreamBuffer != nullptr && pStreamBuffer->getStreamInfo() != nullptr) ){
            rRequest.vInputMetaBuffers.add(pStreamBuffer->getStreamInfo()->getStreamId(), pStreamBuffer);
        }
        // to debug
        if ( ! isRepeating ) {

            if (getLogLevel() >= 2) {
                mCommonInfo->mMetadataConverter->dumpAll(mLatestSettings, request.frameNumber);
            } else if(getLogLevel() >= 1) {
                mCommonInfo->mMetadataConverter->dump(mLatestSettings, request.frameNumber);
            }

            IMetadata::IEntry const& e1 = mLatestSettings.entryFor(MTK_CONTROL_AF_TRIGGER);
            if ( ! e1.isEmpty() ) {
                MUINT8 af_trigger = e1.itemAt(0, Type2Type< MUINT8 >());
                if ( af_trigger==MTK_CONTROL_AF_TRIGGER_START )
                {
                    CAM_ULOGM_DTAG_BEGIN(true, "AF_state: %d", af_trigger);
                    MY_LOGD_IF(getLogLevel() >= 1, "AF_state: %d", af_trigger);
                    CAM_ULOGM_DTAG_END();
                }
            }
            //
            IMetadata::IEntry const& e2 = mLatestSettings.entryFor(MTK_CONTROL_AE_PRECAPTURE_TRIGGER);
            if ( ! e2.isEmpty() ) {
                MUINT8 ae_pretrigger = e2.itemAt(0, Type2Type< MUINT8 >());
                if ( ae_pretrigger==MTK_CONTROL_AE_PRECAPTURE_TRIGGER_START )
                {
                    CAM_ULOGM_DTAG_BEGIN(true, "ae precap: %d", ae_pretrigger);
                    MY_LOGD_IF(getLogLevel() >= 1, "ae precap: %d", ae_pretrigger);
                    CAM_ULOGM_DTAG_END();
                }
            }
            //
            IMetadata::IEntry const& e3 = mLatestSettings.entryFor(MTK_CONTROL_CAPTURE_INTENT);
            if ( ! e3.isEmpty() ) {
                MUINT8 capture_intent = e3.itemAt(0, Type2Type< MUINT8 >());
                CAM_ULOGM_DTAG_BEGIN(true, "capture intent: %d", capture_intent);
                MY_LOGD_IF(getLogLevel() >= 1, "capture intent: %d", capture_intent);
                CAM_ULOGM_DTAG_END();
            }
        }
    }
    //
    //  vOutputImageBuffers <- request.outputBuffers
    rRequest.vOutputImageBuffers.setCapacity(request.outputBuffers.size());
    for ( const auto& buffer : request.outputBuffers )
    {
        sp<AppImageStreamInfo> pStreamInfo = getConfigImageStream(buffer.streamId, imgCfgMap);
        replaceStreamInfo(pStreamInfo);

        if(buffer.bufferId == 0 && mCommonInfo->mSupportBufferManagement) {
            // sp<IImageStreamInfo> pStreamInfo = getConfigImageStream(buffer.streamId);
            rRequest.vOutputImageStreams.add(buffer.streamId, pStreamInfo.get());
        }
        else {
            sp<AppImageStreamBuffer> pStreamBuffer = nullptr;
            BuildImageStreamBufferInputParams const params = {
                .pStreamInfo        = pStreamInfo.get(),
                .pIonDevice         = mIonDevice,
                .staticMetadata     = mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics(),
                // .bufferName         = "",
                .streamBuffer       = buffer,
            };
            pStreamBuffer = makeAppImageStreamBuffer(params, os);

            if ( CC_LIKELY(pStreamBuffer != nullptr && pStreamBuffer->getStreamInfo() != nullptr) ){
                rRequest.vOutputImageBuffers.add(pStreamBuffer->getStreamInfo()->getStreamId(), pStreamBuffer);
                // DEBUG: still add imagestreams because we might keep imagebuffers in AppImageStreamBufferProvider
                rRequest.vOutputImageStreams.add(buffer.streamId, pStreamInfo.get());
            }
        }
    }
    //
    //  vInputImageBuffers <- request.inputBuffer
    bool hasInputBuf = (request.inputBuffer.streamId != -1 &&
                        request.inputBuffer.bufferId != 0);
    if  ( hasInputBuf )
    {
        sp<AppImageStreamInfo> pStreamInfo = getConfigImageStream(request.inputBuffer.streamId, imgCfgMap);
        replaceStreamInfo(pStreamInfo);
        sp<AppImageStreamBuffer> pStreamBuffer;

        BuildImageStreamBufferInputParams const params = {
            .pStreamInfo        = pStreamInfo.get(),
            .pIonDevice         = mIonDevice,
            .staticMetadata     = mCommonInfo->mMetadataProvider->getMtkStaticCharacteristics(),
            .streamBuffer       = request.inputBuffer,
        };
        pStreamBuffer = makeAppImageStreamBuffer(params, os);
        if ( pStreamBuffer==nullptr )
            return -ENODEV;

        if ( CC_LIKELY(pStreamBuffer != nullptr && pStreamBuffer->getStreamInfo() != nullptr) ){
            rRequest.vInputImageBuffers.add(pStreamBuffer->getStreamInfo()->getStreamId(), pStreamBuffer);
        }
    }
    //
    //  vInputMetaBuffers <- request.physicalCameraSettings
    {
        for ( const auto& physicSetting : request.physicalCameraSettings ){
            //convert physical metadata
            IMetadata physicMeta;
            if ( physicSetting.settings!=nullptr ) {   // read settings from metadata settings.
                CAM_TRACE_BEGIN("convert: request.physicalCameraSettings.settings -> physicMeta");
                if ( ! mCommonInfo->mMetadataConverter->convert(physicSetting.settings, physicMeta) ) {
                    MY_LOGE("frameNo:%u convert: request.physicalCameraSettings.settings -> mLatestSettings", request.frameNumber);
                    return -ENODEV;
                }
                CAM_TRACE_END();
            }
            else {
                MY_LOGE("frameNo:%u invalid physical metadata settings!", request.frameNumber);
            }

            StreamId_T const streamId = eSTREAMID_BEGIN_OF_PHYSIC_ID + (int64_t)(std::stoi((std::string)physicSetting.physicalCameraId));
            sp<IMetaStreamInfo> pMetaStreamInfo = getConfigMetaStream(streamId, metaCfgMap);
            if ( pMetaStreamInfo != nullptr ){
                sp<AppMetaStreamBuffer> pMetaStreamBuffer = createMetaStreamBuffer(pMetaStreamInfo, physicMeta, false /*isRepeating*/);
                rRequest.vInputMetaBuffers.add(pMetaStreamInfo->getStreamId(), pMetaStreamBuffer);
                //
                MY_LOGD("Duck: create physical meta frameNo:%u physicId:%d streamId:%#" PRIx64 "",
                    request.frameNumber, std::stoi((std::string)physicSetting.physicalCameraId), streamId);
            }
            //
            // to debug
            if (getLogLevel() >= 2) {
                MY_LOGI("Dump all physical metadata settings frameNo:%u", request.frameNumber);
                mCommonInfo->mMetadataConverter->dumpAll(physicMeta, request.frameNumber);
            } else if(getLogLevel() >= 1) {
                MY_LOGI("Dump partial physical metadata settings frameNo:%u", request.frameNumber);
                mCommonInfo->mMetadataConverter->dump(physicMeta, request.frameNumber);
            }
        }
    }
    // debug log
    MY_LOGD("%s", os.str().c_str());
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
getConfigMetaStream(StreamId_T streamId, const MetaConfigMap* metaCfgMap) const -> sp<AppMetaStreamInfo>
{

    ssize_t const index = metaCfgMap->indexOfKey(streamId);
    if  ( 0 <= index ) {
        return metaCfgMap->valueAt(index);
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
ThisNamespace::
getConfigImageStream(StreamId_T streamId, const ImageConfigMap* imgCfgMap) const -> android::sp<AppImageStreamInfo>
{
    Mutex::Autolock _l(mImageConfigMapLock);
    ssize_t const index = imgCfgMap->indexOfKey(streamId);
    if  ( 0 <= index ) {
        return imgCfgMap->valueAt(index);
    }
    return nullptr;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
ThisNamespace::
createMetaStreamBuffer(
    android::sp<IMetaStreamInfo> pStreamInfo,
    IMetadata const& rSettings,
    bool const repeating
)   const -> AppMetaStreamBuffer*
{
    AppMetaStreamBuffer* pStreamBuffer =
    AppMetaStreamBuffer::Allocator(pStreamInfo.get())(rSettings);
    //
    if ( CC_LIKELY(pStreamBuffer != nullptr) ){
        pStreamBuffer->setRepeating(repeating);
    }
    //
    return pStreamBuffer;
}
