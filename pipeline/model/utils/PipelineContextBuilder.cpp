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

#define LOG_TAG "mtkcam-PipelineContextBuilder"

#include <impl/PipelineContextBuilder.h>
//
#include <mtkcam/drv/IHalSensor.h>
//
#include <mtkcam/utils/gralloc/types.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/hw/HwInfoHelper.h>
#include <mtkcam/utils/feature/PIPInfo.h>
#include <mtkcam/drv/IHalSensor.h>
#include <camera_custom_eis.h>
//
#include <mtkcam3/pipeline/hwnode/NodeId.h>
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam3/pipeline/hwnode/P1Node.h>
#include <mtkcam3/pipeline/hwnode/P2StreamingNode.h>
#include <mtkcam3/pipeline/hwnode/P2CaptureNode.h>
#include <mtkcam3/pipeline/hwnode/FDNode.h>
#include <mtkcam3/pipeline/hwnode/JpegNode.h>
#include <mtkcam3/pipeline/hwnode/RAW16Node.h>
#include <mtkcam3/pipeline/hwnode/PDENode.h>
#include <mtkcam3/pipeline/hwnode/IspTuningDataPackNode.h>
//
#include <mtkcam3/pipeline/utils/SyncHelper/ISyncHelper.h>
#include <mtkcam3/pipeline/utils/streaminfo/ImageStreamInfo.h>
#include <mtkcam3/feature/stereo/StereoCamEnum.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
//
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
#include <aee.h>
#endif
//
#include "MyUtils.h"
#include <mtkcam3/feature/fsc/fsc_defs.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/feature/PIPInfo.h>
//
#include <mtkcam/utils/imgbuf/ISecureImageBufferHeap.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_MODEL);


/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3::pipeline::model;
using namespace NSCam::EIS;
using namespace NSCam::Utils;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::pipeline::NSPipelineContext;
using namespace NSCam::v3::pipeline;
using NSCam::FSC::EFSC_MODE_MASK_FSC_EN;


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
//
#define FUNC_START  MY_LOGD("+")
#define FUNC_END    MY_LOGD("-")

#define VALIDATE_STRING(string) (string!=nullptr ? string : "")

/******************************************************************************
 *
 ******************************************************************************/
#if (MTKCAM_HAVE_AEE_FEATURE == 1)
    #define TRIGGER_AEE(_ops_)                                                  \
    do {                                                                        \
        aee_system_warning(                                                     \
            LOG_TAG,                                                            \
            NULL,                                                               \
            DB_OPT_DEFAULT,                                                     \
            _ops_);                                                             \
    } while(0)
#else
    #define TRIGGER_AEE(_ops_)
#endif

#define CHECK_ERROR(_err_, _ops_)                                               \
    do {                                                                        \
        int const err = (_err_);                                                \
        if( CC_UNLIKELY(err != 0) ) {                                           \
            MY_LOGE("err:%d(%s) ops:%s", err, ::strerror(-err), _ops_);         \
            TRIGGER_AEE(_ops_);                                                 \
            return err;                                                         \
        }                                                                       \
    } while(0)

#define RETURN_IF_ERROR(_err_, _ops_)                                \
    do {                                                  \
        int const err = (_err_);                          \
        if( CC_UNLIKELY(err != 0) ) {                     \
            MY_LOGE("err:%d(%s) ops:%s", err, ::strerror(-err), _ops_); \
            return err;                                   \
        }                                                 \
    } while(0)

#ifndef RETURN_ERROR_IF_FALSE
#define RETURN_ERROR_IF_FALSE(_expr_, _err_, ...)                               \
    do {                                                                        \
        if( CC_UNLIKELY(!(_expr_)) ) {                                          \
            MY_LOGE(__VA_ARGS__);                                               \
            return (_err_);                                                     \
        }                                                                       \
    } while(0)
#endif

/******************************************************************************
 *
 ******************************************************************************/
namespace {


struct InternalCommonInputParams
{
    IPipelineContextBuilder*                    pPipelineContextBuilder = nullptr;
    PipelineStaticInfo const*                   pPipelineStaticInfo = nullptr;
    PipelineUserConfiguration const*            pPipelineUserConfiguration = nullptr;
    PipelineUserConfiguration2 const*           pPipelineUserConfiguration2 = nullptr;
    android::sp<IResourceConcurrency>           pP1NodeResourceConcurrency = nullptr;
    bool const                                  bIsReConfigure;
    MBOOL const                                 bEnableCaptureFlow = MFALSE;
    MBOOL const                                 bEnableQuadCode = MFALSE;
};


}//namespace


/******************************************************************************
 *
 ******************************************************************************/
static int32_t queryNodeInitOrder(const char* nodeName, int32_t defaultOrder = -1)
{
    static const std::string prefix("persist.vendor.camera3.init-order.");
    const std::string key = prefix + VALIDATE_STRING(nodeName);
    int32_t order = property_get_int32(key.c_str(), -1);
    if ( -1 < order ) {
        MY_LOGD("\"%s\"=%d", key.c_str(), order);
    }
    else
    if ( -1 < defaultOrder ) {
        MY_LOGD("\"%s\" doesn't exist; default order:%d > -1", key.c_str(), defaultOrder);
        order = defaultOrder;
    }
    else {
        MY_LOGD("\"%s\" doesn't exist; -1 by default", key.c_str());
        order = -1;
    }
    return order;
}


/******************************************************************************
 *
 ******************************************************************************/
static int64_t queryNodeInitTimeout(const char* nodeName)
{
    static const std::string prefix("persist.vendor.camera3.init-timeout.");
    const std::string key = prefix + VALIDATE_STRING(nodeName);
    int64_t timeout = property_get_int64(key.c_str(), -1);

#if   MTKCAM_TARGET_BUILD_VARIANT=='e'
    // eng build: no timeout
    MY_LOGD("eng build: no timeout");
    timeout = -1;

#else
    // user/userdebug build
    if ( 0 <= timeout ) {
        MY_LOGD("\"%s\"=%" PRId64 "", key.c_str(), timeout);
    }
    else {
        int64_t defaultTimeout = 25000000000;// 25 sec
        timeout = defaultTimeout;
        MY_LOGD("\"%s\" doesn't exist; default timeout:%" PRId64 " ns", key.c_str(), defaultTimeout);
    }

#endif

    return timeout;
}


/******************************************************************************
 *
 ******************************************************************************/
#define add_stream_to_set(_set_, _IStreamInfo_)                                 \
    do {                                                                        \
        if( _IStreamInfo_.get() ) { _set_.add(_IStreamInfo_->getStreamId()); }  \
    } while(0)
//
#define setImageUsage( _IStreamInfo_, _usg_ )                                   \
    do {                                                                        \
        if( _IStreamInfo_.get() ) {                                             \
            builder.setImageStreamUsage( _IStreamInfo_->getStreamId(), _usg_ ); \
        }                                                                       \
    } while(0)


/******************************************************************************
 *
 ******************************************************************************/
template <class DstStreamInfoT, class SrcStreamInfoT>
static void add_meta_stream(
    StreamSet& set,
    DstStreamInfoT& dst,
    SrcStreamInfoT const& src
)
{
    dst = src;
    if ( src != nullptr ) {
        set.add(src->getStreamId());
    }
}

template <class DstStreamInfoT, class SrcStreamInfoT>
static void add_meta_stream(
    StreamSet& set,
    android::Vector<DstStreamInfoT>& dst,
    SrcStreamInfoT const& src
)
{
    dst.push_back(src);
    if ( src != nullptr ) {
        set.add(src->getStreamId());
    }
}


template <class DstStreamInfoT, class SrcStreamInfoT>
static void add_image_stream(
    StreamSet& set,
    DstStreamInfoT& dst,
    SrcStreamInfoT const& src
)
{
    dst = src;
    if ( src != nullptr ) {
        set.add(src->getStreamId());
    }
}


template <class DstStreamInfoT, class SrcStreamInfoT>
static void add_image_stream(
    StreamSet& set,
    android::Vector<DstStreamInfoT>& dst,
    SrcStreamInfoT const& src
)
{
    dst.push_back(src);
    if ( src != nullptr ) {
        set.add(src->getStreamId());
    }
}


/******************************************************************************
 *
 ******************************************************************************/
static void addStream(
    IPipelineContextBuilder* pPipelineContextBuilder,
    uint32_t attribute,
    android::sp<IMetaStreamInfo>const& pStreamInfo
)
{
    using StreamT = IPipelineContextBuilder::MetaStream;
    pPipelineContextBuilder->addStream(
            StreamT{
                .pStreamInfo = pStreamInfo,
                .attribute = attribute,
        });
}


static void addStream(
    IPipelineContextBuilder* pPipelineContextBuilder,
    uint32_t attribute,
    android::sp<IImageStreamInfo>const& pStreamInfo,
    std::shared_ptr<IImageStreamBufferProvider>const& provider = nullptr
)
{
    using StreamT = IPipelineContextBuilder::ImageStream;
    pPipelineContextBuilder->addStream(
            StreamT{
                .pStreamInfo = pStreamInfo,
                .attribute = attribute,
                .pProvider = provider,
        });
}


static
int
configContextLocked_Streams(
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    StreamingFeatureSetting const* pStreamingFeatureSetting,
    bool bIsZslEnabled,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    std::vector<std::set<StreamId_T>> const* pPoolSharedStreams,
    InternalCommonInputParams const* pCommon
)
{
    FUNC_START;
    CAM_TRACE_CALL();
    auto const  pPipelineContextBuilder     = pCommon->pPipelineContextBuilder;
    auto const& pPipelineStaticInfo         = pCommon->pPipelineStaticInfo;
    auto const& pPipelineUserConfiguration  = pCommon->pPipelineUserConfiguration;
    auto const& pPipelineUserConfiguration2 = pCommon->pPipelineUserConfiguration2;

    auto const& pParsedAppConfiguration     = pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo   = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto const& pParsedMultiCamInfo         = pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
    auto const& pAppImageStreamBufferProvider   = pPipelineUserConfiguration2->pImageStreamBufferProvider;


    // A list of image stream sets; the streams in a set share the same buffer pool.
    if ( pPoolSharedStreams != nullptr ) {
        pPipelineContextBuilder->setPoolSharedStreams(
            std::make_shared<std::vector<std::set<StreamId_T>>>(*pPoolSharedStreams)
        );
    }

#define BuildStream(_type_, _pIStreamInfo_)                                     \
    do {                                                                        \
        if ( _pIStreamInfo_ != nullptr ) {                                      \
            addStream(                                                          \
                pPipelineContextBuilder,                                        \
                _type_, _pIStreamInfo_                                          \
            );                                                                  \
        }                                                                       \
    } while(0)

    // Non-P1
    // app meta stream
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_Control);
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2StreamNode);
    for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP2StreamNode_Physical) {
        BuildStream(eStreamType_META_APP, n.second);
    }
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode);
    for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP2CaptureNode_Physical) {
        BuildStream(eStreamType_META_APP, n.second);
    }
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicFD);
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicJpeg);
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicRAW16);
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicP1ISPPack);
    BuildStream(eStreamType_META_APP, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2ISPPack);
    for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP1ISPPack_Physical) {
        BuildStream(eStreamType_META_APP, n.second);
    }
    for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP2ISPPack_Physical) {
        BuildStream(eStreamType_META_APP, n.second);
    }
    for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicRAW16_Physical) {
        BuildStream(eStreamType_META_APP, n.second);
    }
    // hal meta stream
    BuildStream(eStreamType_META_HAL, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2StreamNode);
    BuildStream(eStreamType_META_HAL, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
    BuildStream(eStreamType_META_HAL, pParsedStreamInfo_NonP1->pHalMeta_DynamicPDE);

    // hal image stream
    BuildStream(eStreamType_IMG_HAL_POOL   , pParsedStreamInfo_NonP1->pHalImage_FD_YUV);
    // for tunning &debug
    BuildStream(eStreamType_IMG_HAL_RUNTIME,pParsedStreamInfo_NonP1->pHalImage_Jpeg);
    int32_t enable = property_get_int32("vendor.jpeg.rotation.enable", 1);
    MY_LOGD("Jpeg Rotation enable: %d support packed jpeg: %d",
                                                enable,
                                                pParsedMultiCamInfo->mSupportPackJpegImages);
    int32_t imgo_use_pool = true;
    uint32_t main1SensorId = pPipelineStaticInfo->sensorId[0];
    if(pParsedStreamInfo_P1->count(main1SensorId) &&
       pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rrzo != nullptr)
    {
        imgo_use_pool = (pPipelineStaticInfo->isSupportBurstCap && !(pCommon->bIsReConfigure)
                         && (pPipelineUserConfiguration->pParsedAppImageStreamInfo->maxYuvSize.size() >
                             pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rrzo->getImgSize().size()))
                         || pParsedAppConfiguration->isType3PDSensorWithoutPDE
                         || bIsZslEnabled
                         || pStreamingFeatureSetting->bForceIMGOPool;
    }

    if ( ! (enable & 0x1) )
    {
        BuildStream(eStreamType_IMG_HAL_POOL   , pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV);
        if(pParsedMultiCamInfo->mSupportPackJpegImages)
        {
            BuildStream(eStreamType_IMG_HAL_POOL   , pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV);
        }
    }
    else
    {
        BuildStream(eStreamType_IMG_HAL_RUNTIME, pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV);
        if(pParsedMultiCamInfo->mSupportPackJpegImages)
        {
            BuildStream(eStreamType_IMG_HAL_RUNTIME   , pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV);
        }
    }
    // for depth in hal
    if(pParsedMultiCamInfo->mSupportPackJpegImages)
    {
        BuildStream(eStreamType_IMG_HAL_RUNTIME, pParsedStreamInfo_NonP1->pHalImage_Depth_YUV);
    }

    BuildStream(eStreamType_IMG_HAL_RUNTIME, pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV);

    // P1
    int32_t seamless_imgo_use_pool = property_get_int32("vendor.debug.camera.seamless.imgo_use_pool", 1);
    for (const auto& [_sensorId, _streamInfo_P1] : (*pParsedStreamInfo_P1))
    {
        MY_LOGD("id : %d", _sensorId);
        BuildStream(eStreamType_META_APP, _streamInfo_P1.pAppMeta_DynamicP1);
        BuildStream(eStreamType_META_HAL, _streamInfo_P1.pHalMeta_Control);
        BuildStream(eStreamType_META_HAL, _streamInfo_P1.pHalMeta_DynamicP1);
        MY_LOGD("Build P1 stream");

        BuildStream(imgo_use_pool ? eStreamType_IMG_HAL_POOL : eStreamType_IMG_HAL_RUNTIME, _streamInfo_P1.pHalImage_P1_Imgo);
        BuildStream(seamless_imgo_use_pool ? eStreamType_IMG_HAL_POOL : eStreamType_IMG_HAL_RUNTIME, _streamInfo_P1.pHalImage_P1_SeamlessImgo);
        BuildStream(eStreamType_IMG_HAL_POOL, _streamInfo_P1.pHalImage_P1_Rrzo);
        BuildStream(eStreamType_IMG_HAL_POOL, _streamInfo_P1.pHalImage_P1_Lcso);
        BuildStream(eStreamType_IMG_HAL_POOL, _streamInfo_P1.pHalImage_P1_Rsso);
        BuildStream(eStreamType_IMG_HAL_POOL, _streamInfo_P1.pHalImage_P1_FDYuv);
        BuildStream(eStreamType_IMG_HAL_POOL, _streamInfo_P1.pHalImage_P1_ScaledYuv);
        BuildStream(eStreamType_IMG_HAL_POOL, _streamInfo_P1.pHalImage_P1_RssoR2);
        MY_LOGD("New: p1 full raw(%p); resized raw(%p), bIsZslEnabled(%s)",
                _streamInfo_P1.pHalImage_P1_Imgo.get(), _streamInfo_P1.pHalImage_P1_Rrzo.get(), bIsZslEnabled ? "1" : "0");
    }
#undef BuildStream

    ////////////////////////////////////////////////////////////////////////////
    //  app image stream
#define BuildAppImageStream(_pIStreamInfo_)                                     \
    do {                                                                        \
        if ( _pIStreamInfo_ != nullptr ) {                                      \
            auto type = (uint32_t)(pAppImageStreamBufferProvider!=nullptr       \
                                    ? StreamAttribute::APP_IMAGE_PROVIDER       \
                                    : StreamAttribute::APP_IMAGE_USER);         \
            addStream(                                                          \
                pPipelineContextBuilder,                                        \
                type, _pIStreamInfo_,                                           \
                pAppImageStreamBufferProvider                                   \
            );                                                                  \
        }                                                                       \
    } while(0)

    {
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc ) {
            BuildAppImageStream(n.second);
        }
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
            for(auto&& item:n.second)
                BuildAppImageStream(item);
        }
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Depth);
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Input_Yuv);
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Input_Priv);
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Output_Priv);
        //
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Input_RAW16);
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW16 ) {
            BuildAppImageStream(n.second);
        }
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW10 ) {
            BuildAppImageStream(n.second);
        }
        for( const auto& elm : pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical ) {
            for( const auto& n : elm.second) {
                BuildAppImageStream(n);
            }
        }
        for( const auto& elm : pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical ) {
            for( const auto& n : elm.second) {
                BuildAppImageStream(n);
            }
        }
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Jpeg);

        // isp tuning data stream
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw);
        BuildAppImageStream(pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Yuv);
        for( const auto& elm : pParsedAppImageStreamInfo->vAppImage_Output_IspTuningData_Raw_Physical ) {
            BuildAppImageStream(elm.second);
        }
        for( const auto& elm : pParsedAppImageStreamInfo->vAppImage_Output_IspTuningData_Yuv_Physical ) {
            BuildAppImageStream(elm.second);
        }
    }
#undef  BuildAppImageStream
    ////////////////////////////////////////////////////////////////////////////
    RETURN_ERROR_IF_FALSE(
        pPipelineContextBuilder->buildStreams(),
        UNKNOWN_ERROR, "IPipelineContextBuilder::buildStreams");

    FUNC_END;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
template <class INITPARAM_T, class CONFIGPARAM_T>
static
bool
compareParamsLocked_P1Node(
    INITPARAM_T const& initParam1,  INITPARAM_T const& initParam2,
    CONFIGPARAM_T const& cfgParam1, CONFIGPARAM_T const& cfgParam2
)
{
    FUNC_START;
    if ( initParam1.openId != initParam2.openId ||
         initParam1.nodeId != initParam2.nodeId ||
         strcmp(initParam1.nodeName, initParam2.nodeName) )
        return false;
    //
    if ( cfgParam1.sensorParams.mode        != cfgParam2.sensorParams.mode ||
         cfgParam1.sensorParams.size        != cfgParam2.sensorParams.size ||
         cfgParam1.sensorParams.fps         != cfgParam2.sensorParams.fps ||
         cfgParam1.sensorParams.pixelMode   != cfgParam2.sensorParams.pixelMode ||
         cfgParam1.sensorParams.vhdrMode    != cfgParam2.sensorParams.vhdrMode )
        return false;
    //
    if ( ! cfgParam1.pInAppMeta.get()  || ! cfgParam2.pInAppMeta.get() ||
         ! cfgParam1.pOutAppMeta.get() || ! cfgParam2.pOutAppMeta.get() ||
         ! cfgParam1.pOutHalMeta.get() || ! cfgParam2.pOutHalMeta.get() ||
         cfgParam1.pInAppMeta->getStreamId()  != cfgParam2.pInAppMeta->getStreamId() ||
         cfgParam1.pOutAppMeta->getStreamId() != cfgParam2.pOutAppMeta->getStreamId() ||
         cfgParam1.pOutHalMeta->getStreamId() != cfgParam2.pOutHalMeta->getStreamId() )
        return false;
    //
    if ( ! cfgParam1.pOutImage_resizer.get() || ! cfgParam2.pOutImage_resizer.get() ||
        cfgParam1.pOutImage_resizer->getStreamId() != cfgParam2.pOutImage_resizer->getStreamId() )
        return false;
    //
    if ( cfgParam1.pOutImage_lcso.get() != cfgParam2.pOutImage_lcso.get()
        || cfgParam1.enableLCS != cfgParam2.enableLCS){
        return false;
    }
    //
    if ( cfgParam1.pOutImage_rsso.get() != cfgParam2.pOutImage_rsso.get()
        || cfgParam1.enableRSS != cfgParam2.enableRSS){
        return false;
    }
    //[Pertrus] Rsso_R2
    if ( cfgParam1.pOutImage_mono.get() != cfgParam2.pOutImage_mono.get()){
        return false;
    }
    //
    if ( cfgParam1.pOutImage_full_HAL != cfgParam2.pOutImage_full_HAL ) {
        if ( cfgParam1.pOutImage_full_HAL == nullptr )
            return false;
        if ( cfgParam2.pOutImage_full_HAL == nullptr )
            return false;
        if ( cfgParam1.pOutImage_full_HAL->getStreamId() != cfgParam2.pOutImage_full_HAL->getStreamId() )
            return false;
    }
    //
    if ( cfgParam1.pOutImage_full_APP != cfgParam2.pOutImage_full_APP ) {
        if ( cfgParam1.pOutImage_full_APP == nullptr )
            return false;
        if ( cfgParam2.pOutImage_full_APP == nullptr )
            return false;
        if ( cfgParam1.pOutImage_full_APP->getStreamId() != cfgParam2.pOutImage_full_APP->getStreamId() )
            return false;
    }
    //
    FUNC_END;

    return false;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_P1Node(
    android::sp<IPipelineContext> pContext,
    android::sp<IPipelineContext> const& pOldPipelineContext,
    StreamingFeatureSetting const* pStreamingFeatureSetting,
    SeamlessSwitchFeatureSetting const* pSeamlessSwitchFeatureSetting,
    PipelineNodesNeed const* pPipelineNodesNeed,
    SensorMap<ParsedStreamInfo_P1> const* pvParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    SensorMap<SensorSetting> const* pvSensorSetting,
    SensorMap<P1HwSetting> const* pvP1HwSetting,
    uint32_t const sensorId,
    SensorMap<uint32_t> const* pvBatchSize,
    bool bMultiDevice,
    bool bMultiCam_CamSvPath,
    bool   bIsZslEnabled __unused,
    InternalCommonInputParams const* pCommon,
    MBOOL isReConfig,
    bool bIsP1OnlyOutImgo,
    MBOOL LaggingLaunch,
    MBOOL ConfigureSensor,
    bool bIsSwitchSensor,
    int32_t initRequest,
    BuildPipelineContextOutputParams *outParam
)
{
    typedef P1Node                  NodeT;

    auto const& pPipelineStaticInfo         = pCommon->pPipelineStaticInfo;
    auto const& pPipelineUserConfiguration  = pCommon->pPipelineUserConfiguration;
    auto const& pParsedAppConfiguration     = pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo   = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto const& pParsedMultiCamInfo          = pParsedAppConfiguration->pParsedMultiCamInfo;
    auto const& pParsedSMVRBatchInfo        = pParsedAppConfiguration->pParsedSMVRBatchInfo;
    auto const  pParsedStreamInfo_P1        = &(pvParsedStreamInfo_P1->at(sensorId));
    auto const  pSensorSetting              = &(pvSensorSetting->at(sensorId));
    auto const  pP1HwSetting                = &(pvP1HwSetting->at(sensorId));
    auto const  pBatchSize                  = &(pvBatchSize->at(sensorId));
    //
    auto const physicalSensorIndex = pPipelineStaticInfo->getIndexFromSensorId(sensorId);
    if (physicalSensorIndex < 0)
    {
        MY_LOGE("sensor id(%d) not exist in sensor list", sensorId);
        return UNKNOWN_ERROR;
    }
    int32_t const physicalSensorId = sensorId;
    //
    //
    NodeId_T nodeId = NodeIdUtils::getP1NodeId(physicalSensorIndex);
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = physicalSensorId;
        initParam.nodeId = nodeId;
        initParam.nodeName = "P1Node";
    }

    if(outParam != nullptr && outParam->mvRootNodeOpenIdList != nullptr)
    {
        outParam->mvRootNodeOpenIdList->insert({nodeId, initParam.openId});
    }

    StreamSet inStreamSet;
    StreamSet outStreamSet;
    NodeT::ConfigParams cfgParam;
    {
        cfgParam.openId = pPipelineStaticInfo->openId;

        // set openId for 3A HAL
        int32_t openId3A;
        if (sensorId == 0) // if sensor == wide
        {
            openId3A = pPipelineStaticInfo->openId;
        }
        else
        {
            openId3A = physicalSensorId;
        }
        if(IMetadata::setEntry<MINT32>(&cfgParam.cfgHalMeta, MTK_3A_OPEN_ID, openId3A))
        {
            MY_LOGE("set MTK_3A_OPEN_ID fail");
        }
        //

        NodeT::SensorParams sensorParam(
        /*mode     : */pSensorSetting->sensorMode,
        /*size     : */pSensorSetting->sensorSize,
        /*fps      : */pSensorSetting->sensorFps,
        /*pixelMode: */pP1HwSetting->pixelMode,
        /*vhdrMode : */pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrSensorMode
        );
        //
        add_meta_stream ( inStreamSet, cfgParam.pInAppMeta,         pParsedStreamInfo_NonP1->pAppMeta_Control);
        add_meta_stream ( inStreamSet, cfgParam.pInHalMeta,         pParsedStreamInfo_P1->pHalMeta_Control);
        add_meta_stream (outStreamSet, cfgParam.pOutAppMeta,        pParsedStreamInfo_P1->pAppMeta_DynamicP1);
        add_meta_stream (outStreamSet, cfgParam.pOutHalMeta,        pParsedStreamInfo_P1->pHalMeta_DynamicP1);

        if (bIsP1OnlyOutImgo) {
            cfgParam.bImgoOnly = true;
        } else {
            add_image_stream(outStreamSet, cfgParam.pOutImage_resizer,  pParsedStreamInfo_P1->pHalImage_P1_Rrzo);
            add_image_stream(outStreamSet, cfgParam.pOutImage_lcso,     pParsedStreamInfo_P1->pHalImage_P1_Lcso);
            add_image_stream(outStreamSet, cfgParam.pOutImage_rsso,     pParsedStreamInfo_P1->pHalImage_P1_Rsso);
        }
        // [After ISP 6.0] scaled yuv begin
        add_image_stream(outStreamSet, cfgParam.pOutImage_yuv_resizer2, pParsedStreamInfo_P1->pHalImage_P1_ScaledYuv);
        // [After ISP 6.0] scaled yuv end
        // RssoR2
        add_image_stream(outStreamSet, cfgParam.pOutImage_mono,     pParsedStreamInfo_P1->pHalImage_P1_RssoR2);
        if (auto s = pParsedAppConfiguration->aovImageStreamInfo) {
            cfgParam.pOutImage_mono = s;
        }
        if  ( auto pImgoStreamInfo = pParsedStreamInfo_P1->pHalImage_P1_Imgo.get() )
        {
            // P1Node need to config driver default format
            auto const& setting = pP1HwSetting->imgoDefaultRequest;

            IImageStreamInfo::BufPlanes_t bufPlanes;
            auto infohelper = IHwInfoHelperManager::get()->getHwInfoHelper(physicalSensorId);
            MY_LOGF_IF(infohelper==nullptr, "getHwInfoHelper");
            bool ret = infohelper->getDefaultBufPlanes_Imgo(bufPlanes, setting.format, setting.imageSize);
            MY_LOGF_IF(!ret, "IHwInfoHelper::getDefaultBufPlanes_Imgo");

            auto pStreamInfo =
                ImageStreamInfoBuilder(pImgoStreamInfo)
                .setBufPlanes(bufPlanes)
                .setImgFormat(setting.format)
                .setImgSize(setting.imageSize)
                .build();
            MY_LOGI("%s", pStreamInfo->toString().c_str());
            add_image_stream(outStreamSet, cfgParam.pOutImage_full_HAL, pStreamInfo);
        }
        if (pSeamlessSwitchFeatureSetting->isSeamlessEnable) {
            if (auto pStreamInfo = pParsedStreamInfo_P1->pHalImage_P1_SeamlessImgo) {
                add_image_stream(outStreamSet,
                                 cfgParam.pOutImage_full_seamless_HAL,
                                 pParsedStreamInfo_P1->pHalImage_P1_SeamlessImgo);
                MY_LOGI("Seamless Switch: [Config Stream] %s", pStreamInfo->toString().c_str());
            }

            for (const auto& modeId : pPipelineStaticInfo->seamlessSwitchSensorModes) {
                cfgParam.seamlessSensorModes.push_back(modeId);
            }
            const auto& defaultImgoSize = pP1HwSetting->imgoDefaultRequest.imageSize;
            const auto& SeamlessFullImgoSize = pP1HwSetting->imgoSeamlessRequest.imageSize;
            if ((defaultImgoSize.w * defaultImgoSize.h) > (SeamlessFullImgoSize.w * SeamlessFullImgoSize.h)) {
                cfgParam.maxResolution = defaultImgoSize;
            } else {
                cfgParam.maxResolution = SeamlessFullImgoSize;
            }
            MY_LOGD("Seamless Switch: [Config Stream] set max resolution = %dx%d",
                cfgParam.maxResolution.w, cfgParam.maxResolution.h);
        }
        if ( ! pPipelineNodesNeed->needRaw16Node)
        {
            auto searchRaw16NodeStreamInfo = pParsedAppImageStreamInfo->vAppImage_Output_RAW16.find(pPipelineStaticInfo->openId);
            if (searchRaw16NodeStreamInfo != pParsedAppImageStreamInfo->vAppImage_Output_RAW16.end())
            {
                // P1Node is in charge of outputing App RAW16 if Raw16Node is not configured.
                {
                    auto pRaw16StreamInfo = searchRaw16NodeStreamInfo->second;
                    MY_LOGI("P1Node << App Image Raw16 %s", pRaw16StreamInfo->toString().c_str());
                    add_image_stream(outStreamSet, cfgParam.pOutImage_full_APP, pRaw16StreamInfo);
                }
            }

            searchRaw16NodeStreamInfo = pParsedAppImageStreamInfo->vAppImage_Output_RAW10.find(pPipelineStaticInfo->openId);
            if (searchRaw16NodeStreamInfo != pParsedAppImageStreamInfo->vAppImage_Output_RAW10.end())
            {
                // P1Node is in charge of outputing App RAW10 if Raw16Node is not configured.
                {
                    auto pRaw10StreamInfo = searchRaw16NodeStreamInfo->second;
                    MY_LOGI("P1Node << App Image Raw10 %s", pRaw10StreamInfo->toString().c_str());
                    add_image_stream(outStreamSet, cfgParam.pOutImage_full_APP, pRaw10StreamInfo);
                }
            }

            auto iterPhysicalRaw16 = pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.find(physicalSensorId);
            if (iterPhysicalRaw16 != pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical.end())
            {
                // P1Node is in charge of outputing App RAW16 if Raw16Node is not configured.
                for( auto pRaw16StreamInfo : iterPhysicalRaw16->second ) {
                    MY_LOGI("P1Node << App Image Raw16(Physical) %s", pRaw16StreamInfo->toString().c_str());
                    add_image_stream(outStreamSet, cfgParam.pOutImage_full_APP, pRaw16StreamInfo);
                }
            }

            auto iterPhysicalRaw10 = pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical.find(physicalSensorId);
            if (iterPhysicalRaw10 != pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical.end())
            {
                // P1Node is in charge of outputing App RAW10 if Raw16Node is not configured.
                for( auto pRaw10StreamInfo : iterPhysicalRaw10->second ) {
                    MY_LOGI("P1Node << App Image Raw10(Physical) %s", pRaw10StreamInfo->toString().c_str());
                    add_image_stream(outStreamSet, cfgParam.pOutImage_full_APP, pRaw10StreamInfo);
                }
            }
        }

        MBOOL needLMV = pStreamingFeatureSetting->bNeedLMV;
        MBOOL isLMVSupport = MFALSE;
        MBOOL isRSSSupport = MFALSE;
        {
            NSCamHW::HwInfoHelper infohelper(physicalSensorId);
            MUINT32 max_p1_num = pParsedAppConfiguration->pParsedMultiCamInfo->mSupportPass1Number;
            isLMVSupport = infohelper.getLMVSupported(physicalSensorIndex, pP1HwSetting->camResConfig.Bits.targetTG, max_p1_num);
            isRSSSupport = infohelper.getRSSSupported(physicalSensorIndex, pP1HwSetting->camResConfig.Bits.targetTG, max_p1_num);
        }
        cfgParam.enableLCS          = (pParsedStreamInfo_P1->pHalImage_P1_Lcso.get() && !bIsP1OnlyOutImgo) ? MTRUE : MFALSE;
        cfgParam.enableRSS          = (pParsedStreamInfo_P1->pHalImage_P1_Rsso.get() && isRSSSupport && !bIsP1OnlyOutImgo) ? MTRUE : MFALSE;
        cfgParam.enableEIS          = ((needLMV||cfgParam.enableRSS) && isLMVSupport) ? MTRUE : MFALSE;
        cfgParam.enableFSC          = (pStreamingFeatureSetting->fscMode & EFSC_MODE_MASK_FSC_EN) ? MTRUE : MFALSE;
        MY_LOGD("(%d)enableLCS:%d,enableRSS:%d,enableEIS:%d,enableFSC:%d", physicalSensorIndex,
                cfgParam.enableLCS ? 1 : 0, cfgParam.enableRSS ? 1 : 0,
                cfgParam.enableEIS ? 1 : 0, cfgParam.enableFSC ? 1 : 0);

        if (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode != MTK_HDR_FEATURE_HDR_HAL_MODE_OFF)
        {
            IMetadata::setEntry<MINT32>(&cfgParam.cfgHalMeta, MTK_HDR_FEATURE_HDR_HAL_MODE, static_cast<MINT32>(pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode));
            int debugProcRaw = property_get_int32("vendor.debug.camera.ProcRaw", 0);
            if(debugProcRaw > 0){
                cfgParam.rawProcessed = MTRUE;
            }
            else
                cfgParam.rawProcessed = MFALSE;
        }
        IMetadata::setEntry<MINT32>(&cfgParam.cfgHalMeta, MTK_3A_FEATURE_AE_TARGET_MODE, static_cast<MINT32>(pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).aeTargetMode));
        IMetadata::setEntry<MINT32>(&cfgParam.cfgHalMeta, MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM, static_cast<MINT32>(pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).validExposureNum));
        IMetadata::setEntry<MINT32>(&cfgParam.cfgHalMeta, MTK_3A_ISP_FUS_NUM, static_cast<MINT32>(pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).fusNum));
        // AIShutter
        if(pParsedAppConfiguration->supportAIShutter)
        {
            IMetadata::setEntry<MBOOL>(
                                    &cfgParam.cfgHalMeta,
                                    MTK_3A_AI_SHUTTER,
                                    MTRUE);
        }
        if(pStreamingFeatureSetting->dsdn30GyroEnable)
        {
            IMetadata::setEntry<MBOOL>(
                                    &cfgParam.cfgHalMeta,
                                    MTK_SW_GYRO_ENABLE,
                                    MTRUE);
        }

        if (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) {
            IMetadata::IEntry entry(MTK_STAGGER_BLOB_IMGO_ORDER);
            entry.push_back(MTK_STAGGER_IMGO_NE, Type2Type<MUINT8>());
            entry.push_back(MTK_STAGGER_IMGO_ME, Type2Type<MUINT8>());
            if (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP) {
                entry.push_back(MTK_STAGGER_IMGO_SE, Type2Type<MUINT8>());
            }
            cfgParam.cfgHalMeta.update(MTK_STAGGER_BLOB_IMGO_ORDER, entry);

            if (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP) {
                cfgParam.staggerNum = 2;
            }
            else if (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP) {
                cfgParam.staggerNum = 3;
            }
        }

        {
            /**
             * enum RAW_DEF_TYPE
             * {
             *     RAW_DEF_TYPE_PROCESSED_RAW  = 0x0000,   // Processed raw
             *     RAW_DEF_TYPE_PURE_RAW       = 0x0001,   // Pure raw
             *     RAW_DEF_TYPE_AUTO           = 0x000F    // if (rawProcessed ||
             *                                             //     post-proc-raw-unsupported)
             *                                             //     PROCESSED_RAW
             *                                             // else
             *                                             //     PURE_RAW
             * };
             */
            auto rawDefType2String = [](uint32_t value) -> std::string {
                switch (value) {
                    case NSCam::v3::P1Node::RAW_DEF_TYPE_PROCESSED_RAW: return std::string("proc");
                    case NSCam::v3::P1Node::RAW_DEF_TYPE_PURE_RAW:      return std::string("pure");
                    case NSCam::v3::P1Node::RAW_DEF_TYPE_AUTO:          return std::string("auto");
                    default: break;
                }
                return std::string("unknown(") + std::to_string(value) + ")";
            };

            int debugProcRaw = property_get_int32("vendor.debug.camera.cfg.ProcRaw", -1);
            auto& pipInfo = PIPInfo::getInstance();
            if (debugProcRaw >= 0)
            {
                // for CCT dump
                MY_LOGD("set vendor.debug.camera.ProcRaw(%d) => 0:config pure raw  1:config processed raw",debugProcRaw);
                cfgParam.rawProcessed = debugProcRaw;
                cfgParam.rawDefType   = NSCam::v3::P1Node::RAW_DEF_TYPE_AUTO;
            }
            else if ( bMultiDevice )
            {
                // multicam: the settings is as before (i.e. use default settings)
                //cfgParam.rawProcessed = false;
                //cfgParam.rawDefType   = NSCam::v3::P1Node::RAW_DEF_TYPE_AUTO;
                MY_LOGD("multicam -> P1Node{rawProcessed=%d rawDefType=%s}",
                    cfgParam.rawProcessed, rawDefType2String(cfgParam.rawDefType).c_str());
            }
            else if (pipInfo.getMaxOpenCamDevNum() >= 2 && !isReConfig && !pCommon->bIsReConfigure)
            {
                cfgParam.rawProcessed = true;                                       // whether proc-raw support or not depends on the (sensor) settings
                cfgParam.rawDefType   = NSCam::v3::P1Node::RAW_DEF_TYPE_PURE_RAW;   // PURE (or AUTO)  -> Pure RAW by default
                cfgParam.tgNum = 2;
                // set PIP resource config
                cfgParam.camResConfig = pP1HwSetting->camResConfig;
                // set PIP p1 drv isp quality
                if(pP1HwSetting->ispQuality == eCamIQ_L) {
                    cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_L;
                }
                else if(pP1HwSetting->ispQuality == eCamIQ_H) {
                    cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_H;
                }
                else {
                    cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_UNKNOWN;
                }
                // force resize quality to high
                cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_H;
                MY_LOGD("set PIP camRes: id(%d) Raw(%d) TargetTG(%d) isOffTwin(%d) rsv(%d) ispQty(%d) isHighQty(%d)",
                    physicalSensorIndex, cfgParam.camResConfig.Raw, cfgParam.camResConfig.Bits.targetTG,
                    cfgParam.camResConfig.Bits.isOffTwin, cfgParam.camResConfig.Bits.rsv,
                    cfgParam.resizeQuality, cfgParam.resizeQuality == NSCam::v3::P1Node::RESIZE_QUALITY_H);
                MY_LOGD("PIP flow, set p1 raw type is pure raw");
            }
            else
            {
                auto infohelper = IHwInfoHelperManager::get()->getHwInfoHelper(physicalSensorId);
                MY_LOGF_IF(infohelper==nullptr, "getHwInfoHelper");
                IHwInfoHelper::QueryRawTypeSupportParams params;
                params.in.push_back(IHwInfoHelper::QueryRawTypeSupportParams::CameraInputParams{
                    .sensorId = physicalSensorId,
                    .sensorMode = pSensorSetting->sensorMode,
                    .minProcessingFps = (pSensorSetting->sensorFps < 30 ? pSensorSetting->sensorFps : 30),
                    .rrzoImgSize = (pParsedStreamInfo_P1->pHalImage_P1_Rrzo != nullptr
                                  ? pParsedStreamInfo_P1->pHalImage_P1_Rrzo->getImgSize()
                                  : MSize(0, 0)),
                });
                infohelper->queryRawTypeSupport(params);
                if ( ! params.isPostProcRawSupport ) {
                    // ISP3 (P2 doesn't support post-proc raw)
                    cfgParam.rawProcessed = true;                                       // don't care
                    cfgParam.rawDefType   = NSCam::v3::P1Node::RAW_DEF_TYPE_AUTO;       // AUTO or PROCESS -> PROCESS RAW
                }
                else {
                    // ISP4 or above
                    cfgParam.rawProcessed = params.isProcImgoSupport;                   // whether proc-raw support or not depends on the (sensor) settings
                    cfgParam.rawDefType   = NSCam::v3::P1Node::RAW_DEF_TYPE_PURE_RAW;   // PURE (or AUTO)  -> Pure RAW by default
                }
                cfgParam.camResConfig.Bits.Min_Useful_FRZ_ratio = pP1HwSetting->camResConfig.Bits.Min_Useful_FRZ_ratio;
                MY_LOGD("isPostProcRawSupport=%d isProcImgoSupport=%d -> P1Node{rawProcessed=%d rawDefType=%s}",
                    params.isPostProcRawSupport, params.isProcImgoSupport,
                    cfgParam.rawProcessed, rawDefType2String(cfgParam.rawDefType).c_str());
            }
        }
        // for 4cell multi-thread capture
        if(pCommon->pP1NodeResourceConcurrency != nullptr)
            cfgParam.pResourceConcurrency = pCommon->pP1NodeResourceConcurrency;
        // set quadCode pattern enable for p1
        cfgParam.enableQuadCode = pCommon->bEnableQuadCode;
        cfgParam.enableCaptureFlow = pCommon->bEnableCaptureFlow;

        if (physicalSensorIndex == 0)
        {
            add_image_stream( inStreamSet, cfgParam.pInImage_yuv,       pParsedAppImageStreamInfo->pAppImage_Input_Yuv);
            add_image_stream( inStreamSet, cfgParam.pInImage_raw,       pParsedAppImageStreamInfo->pAppImage_Input_RAW16);
            add_image_stream( inStreamSet, cfgParam.pInImage_opaque,    pParsedAppImageStreamInfo->pAppImage_Input_Priv);
            add_image_stream( outStreamSet, cfgParam.pOutImage_opaque,   pParsedAppImageStreamInfo->pAppImage_Output_Priv);
        }
        cfgParam.sensorParams = sensorParam;
        {
            int64_t ImgoId;
            int64_t RrzoId;
            int64_t LcsoId;
            int64_t RssoId;
            int64_t ScaledYuvId;
            switch (physicalSensorIndex)
            {
                case 0:
                    ImgoId = eSTREAMID_IMAGE_PIPE_RAW_OPAQUE_00;
                    RrzoId = eSTREAMID_IMAGE_PIPE_RAW_RESIZER_00;
                    LcsoId = eSTREAMID_IMAGE_PIPE_RAW_LCSO_00;
                    RssoId = eSTREAMID_IMAGE_PIPE_RAW_RSSO_00;
                    ScaledYuvId = eSTREAMID_IMAGE_PIPE_P1_SCALED_YUV_00;
                    break;
                case 1:
                    ImgoId = eSTREAMID_IMAGE_PIPE_RAW_OPAQUE_01;
                    RrzoId = eSTREAMID_IMAGE_PIPE_RAW_RESIZER_01;
                    LcsoId = eSTREAMID_IMAGE_PIPE_RAW_LCSO_01;
                    RssoId = eSTREAMID_IMAGE_PIPE_RAW_RSSO_01;
                    ScaledYuvId = eSTREAMID_IMAGE_PIPE_P1_SCALED_YUV_01;
                    break;
                case 2:
                    ImgoId = eSTREAMID_IMAGE_PIPE_RAW_OPAQUE_02;
                    RrzoId = eSTREAMID_IMAGE_PIPE_RAW_RESIZER_02;
                    LcsoId = eSTREAMID_IMAGE_PIPE_RAW_LCSO_02;
                    RssoId = eSTREAMID_IMAGE_PIPE_RAW_RSSO_02;
                    ScaledYuvId = eSTREAMID_IMAGE_PIPE_P1_SCALED_YUV_02;
                    break;
                default:
                    ImgoId = eSTREAMID_IMAGE_PIPE_RAW_OPAQUE_00;
                    RrzoId = eSTREAMID_IMAGE_PIPE_RAW_RESIZER_00;
                    LcsoId = eSTREAMID_IMAGE_PIPE_RAW_LCSO_00;
                    RssoId = eSTREAMID_IMAGE_PIPE_RAW_RSSO_00;
                    MY_LOGE("not support p1 node number large than 2");
                    break;
            }
        }
        cfgParam.packedEisInfo = const_cast<NSCam::EIS::EisInfo*>(&pStreamingFeatureSetting->eisInfo)->toPackedData();
        cfgParam.bEnableGyroMV = pStreamingFeatureSetting->bEnableGyroMV;
        // configure video size
        IMetadata::setEntry<MSize>(&cfgParam.cfgHalMeta, MTK_HAL_REQUEST_VIDEO_SIZE, pParsedAppImageStreamInfo->videoImageSize);
        // cfgParam.eisMode = mParams->mEISMode;
        //
        if ( pParsedAppImageStreamInfo->hasVideo4K )
            cfgParam.receiveMode = NodeT::REV_MODE::REV_MODE_CONSERVATIVE;
        // config initframe
        if (pCommon->bIsReConfigure)
        {
            if (bIsSwitchSensor)
            {
                MY_LOGD("Is sensor switch flow, force init request = 3");
                initRequest = 3; // force 3
            }
            else
            {
                MY_LOGD("Is Reconfig flow, force init request = 0");
                initRequest = 0;
            }
        }
        else if (pStreamingFeatureSetting->bDisableInitRequest)
        {
            MY_LOGD("Disable the init request flow, force init request = 0");
            initRequest = 0;
        }
        else
        {
            initRequest = pParsedAppConfiguration->initRequest;
        }
        cfgParam.initRequest = initRequest;
        cfgParam.cfgAppMeta = pParsedAppConfiguration->sessionParams;

        // batchNum and burstNum are mutual exclusive for P1Node
        if (pParsedSMVRBatchInfo) // SMVRBatch
        {
            cfgParam.batchNum = pParsedSMVRBatchInfo->p1BatchNum;
            cfgParam.burstNum = 0;
            cfgParam.groupNum = 0;
        }
        else // SMVRConstaint
        {
            if( pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW )
            {
                cfgParam.batchNum = 0;
                cfgParam.burstNum = 0;
                cfgParam.groupNum = *pBatchSize;
            }
            else
            {
                cfgParam.batchNum = 0;
                cfgParam.burstNum = *pBatchSize;
                cfgParam.groupNum = 0;
            }
        }
        MY_LOGD("(%d)SMVRBatch: p1BatchNum(SMVRBatch)=%d, p1BurstNum(SMVRConstraint)=%d,p1GroupNum(MStreamFusion)=%d", physicalSensorId, cfgParam.batchNum, cfgParam.burstNum, cfgParam.groupNum);
        //
        std::shared_ptr<NSCamHW::HwInfoHelper> infohelper = std::make_shared<NSCamHW::HwInfoHelper>(physicalSensorId);
        bool ret = infohelper != nullptr && infohelper->updateInfos();
        if ( CC_UNLIKELY(!ret) ) {
            MY_LOGE("HwInfoHelper");
        }
        if ( infohelper->getDualPDAFSupported(pSensorSetting->sensorMode) ) {
            cfgParam.enableDualPD = MTRUE;
            if ( ! bMultiDevice )
            {
                cfgParam.disableFrontalBinning = MTRUE;
            }
            MY_LOGD("PDAF supported for sensor mode:%d - enableDualPD:%d disableFrontalBinning:%d",
                    pSensorSetting->sensorMode, cfgParam.enableDualPD, cfgParam.disableFrontalBinning);
        }
        // set multi-cam config info
        if(bMultiDevice)
        {
            int32_t enableSync = property_get_int32("vendor.multicam.sync.enable", 1);
            cfgParam.pSyncHelper = (enableSync) ? pContext->getMultiCamSyncHelper() : nullptr;
            cfgParam.enableFrameSync = MTRUE;
            cfgParam.tgNum = 2;
            // for multi-cam case, it has to set quality high.
            // Only ISP 5.0 will check this value.
            cfgParam.resizeQuality = StereoSettingProvider::getVSDoFP1ResizeQuality();
            if(pParsedMultiCamInfo->mDualDevicePath == NSCam::v3::pipeline::policy::DualDevicePath::MultiCamControl)
            {
                IMetadata::setEntry<MINT32>(
                                    &cfgParam.cfgAppMeta,
                                    MTK_MULTI_CAM_FEATURE_MODE,
                                    pParsedMultiCamInfo->mDualFeatureMode);
                // for vsdof, it has to check EIS.
                // If EIS on, force set EIS to main1.
                if(MTK_MULTI_CAM_FEATURE_MODE_VSDOF == pParsedMultiCamInfo->mDualFeatureMode)
                {
                    if(cfgParam.enableEIS && physicalSensorIndex == 0)
                    {
                        cfgParam.forceSetEIS = MTRUE;
                    }
                    else
                    {
                        cfgParam.forceSetEIS = MFALSE;
                    }
                }
                // update framsync mode.
                auto frameSyncMode = StereoSettingProvider::getFrameSyncType(pPipelineStaticInfo->openId);
                MUINT8 frameSyncMode_tag = MTK_FRAMESYNC_MODE_VSYNC_ALIGNMENT;
                bool isSupportVHDR =
                          (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER) ||
                          (pStreamingFeatureSetting->p1ConfigParam.at(physicalSensorId).hdrHalMode & MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_PREVIEW);
                if(isSupportVHDR) {
                  frameSyncMode_tag = MTK_FRAMESYNC_MODE_NE_ALIGNMENT;
                }
                else {
                  if(frameSyncMode == E_VSYNC_ALIGN) {
                    MY_LOGD("framesync mode: vsync");
                    frameSyncMode_tag = MTK_FRAMESYNC_MODE_VSYNC_ALIGNMENT;
                  }
                  else {
                    MY_LOGD("framesync mode: read out center");
                    frameSyncMode_tag = MTK_FRAMESYNC_MODE_READOUT_CENTER_ALIGNMENT;
                  }
                }
                IMetadata::setEntry<MUINT8>(
                                    &cfgParam.cfgHalMeta,
                                    MTK_FRAMESYNC_MODE,
                                    frameSyncMode_tag);
            }
            // for dual cam case, if rrzo is null, it means camsv path.
            if(bMultiCam_CamSvPath)
            {
                IMetadata::setEntry<MBOOL>(&cfgParam.cfgHalMeta, MTK_STEREO_WITH_CAMSV, MTRUE);
            }
            // set cam p1 drv isp quality
            if(pP1HwSetting->ispQuality == eCamIQ_L) {
                cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_L;
            }
            else if(pP1HwSetting->ispQuality == eCamIQ_H) {
                cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_H;
            }
            else {
                cfgParam.resizeQuality = NSCam::v3::P1Node::RESIZE_QUALITY_UNKNOWN;
            }
            // set cam tg

           cfgParam.camResConfig = pP1HwSetting->camResConfig;
           MY_LOGD("set multicam camRes: id(%d) Raw(%d) TargetTG(%d) isOffTwin(%d) rsv(%d) ispQty(%d) isHighQty(%d)",
                physicalSensorIndex, cfgParam.camResConfig.Raw, cfgParam.camResConfig.Bits.targetTG,
                cfgParam.camResConfig.Bits.isOffTwin, cfgParam.camResConfig.Bits.rsv,
                cfgParam.resizeQuality, cfgParam.resizeQuality == NSCam::v3::P1Node::RESIZE_QUALITY_H);

            // lagging start while the pass1 index >= mSupportPass1Number
            cfgParam.laggingLaunch = LaggingLaunch;
            if(cfgParam.laggingLaunch)
            {
                // for lagging case, it cannot support init request.
                cfgParam.initRequest = 0;
            }
            cfgParam.skipSensorConfig = !ConfigureSensor;
        }
        // config P1 for ISP 6.0
        //MY_LOGE("Need to do config P1 for ISP 6.0 face detection!!!!!!!");
        if (pParsedStreamInfo_P1->pHalImage_P1_FDYuv != nullptr)
        {
            cfgParam.pOutImage_yuv_resizer1    = pParsedStreamInfo_P1->pHalImage_P1_FDYuv;
        }
        // secure flow
        cfgParam.secType = pParsedAppImageStreamInfo->secureInfo.type;
        cfgParam.enableSecurity = (cfgParam.secType != SecType::mem_normal);
        if(cfgParam.enableSecurity)
        {
            size_t StatusBufSize = 16;
            IImageBufferAllocator::ImgParam allocImgParam(StatusBufSize,0);
            sp<ISecureImageBufferHeap> secureStatusHeap = ISecureImageBufferHeap::create(
                                    "SecureStatus",
                                    allocImgParam,
                                    ISecureImageBufferHeap::AllocExtraParam(0,1,0,MFALSE,pParsedAppImageStreamInfo->secureInfo.type),
                                    MFALSE
                                );
            if (secureStatusHeap == nullptr)
            {
                MY_LOGE("Fail to create secureStatusHeap");
                cfgParam.statusSecHeap = nullptr;
            }
            else
            {
                secureStatusHeap->lockBuf("SecureStatus", eBUFFER_USAGE_SW_MASK|eBUFFER_USAGE_HW_MASK);
                cfgParam.statusSecHeap = std::shared_ptr<IImageBufferHeap>(
                                                                    secureStatusHeap.get(),
                                                                    [pHeap=secureStatusHeap](IImageBufferHeap*){
                                                                    pHeap->unlockBuf("SecureStatus");
                                                                    MY_LOGD("release secureStatusHeap");
                                                                });
            }
        }
        else
        {
            cfgParam.statusSecHeap = nullptr;
        }
        MY_LOGD("enableSecurity(%d) secType(%d) statusSecHeap(%p)->getBufVA(0)(%" PRIXPTR ")",
            cfgParam.enableSecurity,cfgParam.secType,
            cfgParam.statusSecHeap.get(), cfgParam.statusSecHeap.get() ? cfgParam.statusSecHeap->getBufVA(0) : 0);
        // ISP Hidl
        if (pPipelineNodesNeed->needP1RawIspTuningDataPackNode ||
            pPipelineNodesNeed->needP2YuvIspTuningDataPackNode ||
            pPipelineNodesNeed->needP2RawIspTuningDataPackNode)
        {
            cfgParam.bAospOpaque = MFALSE;
        }
    }
    //
    android::sp<INodeActor> pNode;
    if (isReConfig) {
        MY_LOGD("mvhdr reconfig(%d) and config P1 node only", isReConfig);
        pNode = pContext->queryINodeActor(nodeId);
        pNode->setConfigParam(&cfgParam);
        pNode->reconfig();
    }else{
        {
            if ( pOldPipelineContext != nullptr ) {
                android::sp<INodeActor> pOldNode = pOldPipelineContext->queryINodeActor(nodeId);
                if ( pOldNode != nullptr ) {
                    pOldNode->uninit(); // must uninit old P1 before call new P1 config
                }
            }
            //
            pNode = makeINodeActor( NodeT::createInstance() );
        }
    }
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName)); //P1Node
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    // ISP 6.0
    add_stream_to_set(outStreamSet, pParsedStreamInfo_P1->pHalImage_P1_FDYuv);
    //
    NodeBuilder builder(nodeId, pNode);
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    if (physicalSensorIndex == 0)
    {
        setImageUsage(cfgParam.pInImage_yuv,        eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(cfgParam.pInImage_raw,        eBUFFER_USAGE_SW_READ_OFTEN);
        setImageUsage(cfgParam.pInImage_opaque,     eBUFFER_USAGE_SW_READ_OFTEN);
        setImageUsage(cfgParam.pOutImage_opaque,    eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }
    //
    setImageUsage(cfgParam.pOutImage_full_HAL,  eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutImage_full_APP,  eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutImage_resizer,   eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutImage_lcso,      eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutImage_rsso,      eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutImage_mono,      eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    // ISP 6.0
    setImageUsage(pParsedStreamInfo_P1->pHalImage_P1_FDYuv   , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(pParsedStreamInfo_P1->pHalImage_P1_ScaledYuv   , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(pParsedStreamInfo_P1->pHalImage_P1_RssoR2   , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutImage_full_seamless_HAL,  eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    //
    int err = builder.build(pContext);
    if( err != OK ) {
        MY_LOGE("build node %s failed", toHexString(nodeId).c_str());
    }
    //
    pContext->setInitFrameCount(initRequest);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_P2SNode(
    android::sp<IPipelineContext> pContext,
    StreamingFeatureSetting const* pStreamingFeatureSetting,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    SensorMap<uint32_t> const* pBatchSize,
    uint32_t useP1NodeCount __unused,
    bool bHasMonoSensor __unused,
    InternalCommonInputParams const* pCommon
)
{
    typedef P2StreamingNode         NodeT;
    auto const& pPipelineStaticInfo         = pCommon->pPipelineStaticInfo;
    auto const& pPipelineUserConfiguration  = pCommon->pPipelineUserConfiguration;
    auto const& pParsedAppConfiguration     = pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedAppImageStreamInfo   = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto const& pParsedMultiCamInfo          = pParsedAppConfiguration->pParsedMultiCamInfo;
    auto const& pParsedSMVRBatchInfo        = pParsedAppConfiguration->pParsedSMVRBatchInfo;
    auto const  main1SensorId = pPipelineStaticInfo->sensorId[0];
    //
    NodeId_T const nodeId = eNODEID_P2StreamNode;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = pPipelineStaticInfo->sensorId[0];
        initParam.nodeId = nodeId;
        initParam.nodeName = "P2StreamNode";
        for (size_t i = 1; i < useP1NodeCount; i++)
        {
            initParam.subOpenIdList.push_back(pPipelineStaticInfo->sensorId[i]);
        }
    }
    NodeT::ConfigParams cfgParam;
    {
        cfgParam.pInAppMeta    = pParsedStreamInfo_NonP1->pAppMeta_Control;
        cfgParam.pInAppRetMeta = pParsedStreamInfo_P1->at(main1SensorId).pAppMeta_DynamicP1;
        cfgParam.pInHalMeta    = pParsedStreamInfo_P1->at(main1SensorId).pHalMeta_DynamicP1;
        cfgParam.pOutAppMeta   = pParsedStreamInfo_NonP1->pAppMeta_DynamicP2StreamNode;
        for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP2StreamNode_Physical) {
            struct NodeT::ConfigParams::PhysicalStream phyStreams;
            phyStreams.pOutAppPhysicalMeta = n.second;
            phyStreams.sensorId = (MUINT32)n.first;
            cfgParam.vPhysicalStreamsInfo.push_back(phyStreams);
        }
        cfgParam.pOutHalMeta   = pParsedStreamInfo_NonP1->pHalMeta_DynamicP2StreamNode;
        //
        if( pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Imgo.get() )
            cfgParam.pvInFullRaw.push_back(pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Imgo);
        //
        if( pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_SeamlessImgo.get() )
            cfgParam.pvInSeamlessFullRaw.push_back(pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_SeamlessImgo);
        //
        cfgParam.pInResizedRaw = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rrzo;
        //
        cfgParam.pInLcsoRaw = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Lcso;
        //
        cfgParam.pInRssoRaw = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rsso;
        //
        cfgParam.pInResizedYuv2 = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_ScaledYuv;
        cfgParam.pInRssoR2Raw = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_RssoR2;
        //
        for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
        {
            if(_sensorId == main1SensorId)
                continue;
            struct NodeT::ConfigParams::P1SubStreams subStreams;

            subStreams.pInAppRetMeta_Sub = _streamInfo_P1.pAppMeta_DynamicP1;
            subStreams.pInHalMeta_Sub = _streamInfo_P1.pHalMeta_DynamicP1;
            subStreams.pInFullRaw_Sub = _streamInfo_P1.pHalImage_P1_Imgo;
            subStreams.pInResizedRaw_Sub = _streamInfo_P1.pHalImage_P1_Rrzo;
            subStreams.pInLcsoRaw_Sub = _streamInfo_P1.pHalImage_P1_Lcso;
            subStreams.pInRssoRaw_Sub = _streamInfo_P1.pHalImage_P1_Rsso;
            subStreams.pInResizedYuv2_Sub = _streamInfo_P1.pHalImage_P1_ScaledYuv;
            subStreams.pInRssoR2Raw_Sub = _streamInfo_P1.pHalImage_P1_RssoR2;
            if (pParsedAppConfiguration->hasTrackingFocus)
            {
                subStreams.pInTrackingYuv_Sub = _streamInfo_P1.pHalImage_P1_FDYuv;
            }
            subStreams.sensorId = _sensorId;

            cfgParam.vP1SubStreamsInfo.push_back(subStreams);
        }
        //
        if( pParsedAppImageStreamInfo->pAppImage_Output_Priv.get() )
        {
            cfgParam.pvInOpaque.push_back(pParsedAppImageStreamInfo->pAppImage_Output_Priv);
        }
        //
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
        {
            cfgParam.vOutImage.push_back(n.second);
        }
        //
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
            for(auto&& item:n.second)
                cfgParam.vOutImage.push_back(item);
        }
        //
        if( pParsedAppImageStreamInfo->pAppImage_Depth.get() )
        {
            cfgParam.pOutAppDepthImage = pParsedAppImageStreamInfo->pAppImage_Depth;
        }
        //
        if( pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV.get() )
        {
            cfgParam.vOutImage.push_back(pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV);
        }
        if( pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV.get() )
        {
            cfgParam.vOutImage.push_back(pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV);
        }
        //
        if( pParsedStreamInfo_NonP1->pHalImage_FD_YUV.get() )
        {
            cfgParam.pOutFDImage = pParsedStreamInfo_NonP1->pHalImage_FD_YUV;
        }
        //
        cfgParam.burstNum = (*pBatchSize).at(main1SensorId);

        if (pStreamingFeatureSetting->bSupportPQ)
        {
            cfgParam.customOption |= P2Common::CUSTOM_OPTION_PQ_SUPPORT;
        }
        if (pStreamingFeatureSetting->bSupportCZ)
        {
            cfgParam.customOption |= P2Common::CUSTOM_OPTION_CLEAR_ZOOM_SUPPORT;
        }
        if (pStreamingFeatureSetting->bSupportDRE)
        {
            cfgParam.customOption |= P2Common::CUSTOM_OPTION_DRE_SUPPORT;
        }
        if (pStreamingFeatureSetting->bSupportHFG)
        {
            cfgParam.customOption |= P2Common::CUSTOM_OPTION_HFG_SUPPORT;
        }
        // for TrackingFocus
        if (pParsedAppConfiguration->hasTrackingFocus)
        {
            cfgParam.pInTrackingYuv   = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_FDYuv;
        }
    }

    P2Common::UsageHint p2Usage;
    {
        p2Usage.mP2NodeType = P2Common::P2_NODE_COMMON;
        p2Usage.m3DNRMode   = pStreamingFeatureSetting->nr3dMode;
        p2Usage.mAISSMode = (pParsedAppConfiguration->supportAIShutter) ? 1 : 0;
        const auto &pipInfo = pStreamingFeatureSetting->pipInfo;
        if ( pipInfo.isValid && !pipInfo.isAISSOn && p2Usage.mAISSMode )
        {
            MY_LOGW("pip: !!warn:: AISSMode off by policy");
            p2Usage.mAISSMode = 0;
        }
        p2Usage.mPresetPQIdx = pStreamingFeatureSetting->pipInfo.PQIdx;
        p2Usage.mFSCMode   = pStreamingFeatureSetting->fscMode;
        p2Usage.mUseTSQ     = pStreamingFeatureSetting->bEnableTSQ;
        p2Usage.mEnlargeRsso = pStreamingFeatureSetting->bNeedLargeRsso;
        p2Usage.mTP = pStreamingFeatureSetting->supportedScenarioFeatures;
        p2Usage.mTPMarginRatio = pStreamingFeatureSetting->targetRrzoRatio;
        p2Usage.mTotalMarginRatio = pStreamingFeatureSetting->totalMarginRatio;
        p2Usage.mAppSessionMeta = pParsedAppConfiguration->sessionParams;
        p2Usage.mDsdnHint = pStreamingFeatureSetting->dsdnHint;

        p2Usage.mInCfg.mReqFps = pStreamingFeatureSetting->reqFps;
        p2Usage.mInCfg.mP1Batch = pParsedSMVRBatchInfo ? pParsedSMVRBatchInfo->p1BatchNum : 1;

        p2Usage.mPackedEisInfo = const_cast<NSCam::EIS::EisInfo*>(&pStreamingFeatureSetting->eisInfo)->toPackedData();
        // for TrackingFocus
        if(pParsedAppConfiguration->hasTrackingFocus)
        {
            p2Usage.mTrackingFocusOn = true;
        }
        if( pParsedAppImageStreamInfo->hasVideoConsumer )
        {
            p2Usage.mHasVideo = MTRUE;
            p2Usage.mAppMode = P2Common::APP_MODE_VIDEO;
            p2Usage.mOutCfg.mVideoSize = pParsedAppImageStreamInfo->videoImageSize;
        }
        if( pParsedAppConfiguration->isConstrainedHighSpeedMode )
        {
            p2Usage.mAppMode = P2Common::APP_MODE_HIGH_SPEED_VIDEO;
        }
        if( pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rrzo != NULL )
        {
            p2Usage.mStreamingSize = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rrzo->getImgSize();
        }
        if (pParsedSMVRBatchInfo)
        {
            p2Usage.mAppMode = P2Common::APP_MODE_BATCH_SMVR;
            p2Usage.mSMVRSpeed = pParsedSMVRBatchInfo->p2BatchNum;

            MY_LOGD("SMVRBatch: p2BatchNum=%d", pParsedSMVRBatchInfo->p2BatchNum);
        }
        if(useP1NodeCount > 1)
        {
            // for dual cam device, it has to config other parameter.
            // 1. set all resized raw image size
            // if rrzo stream info is nullptr, set imgo size
            MSize maxStreamingSize = MSize(0, 0);
            MSize tempSize;
            for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
            {
                if ( _streamInfo_P1.pHalImage_P1_Rrzo != nullptr )
                {
                    tempSize = _streamInfo_P1.pHalImage_P1_Rrzo->getImgSize();
                    p2Usage.mResizedRawMap.insert(
                                    std::pair<MUINT32, MSize>(
                                            _sensorId,
                                            tempSize));
                }
                else
                {
                    MY_LOGD("rrzo stream info is null, set imgo size to resized raw map.");
                    tempSize = _streamInfo_P1.pHalImage_P1_Imgo->getImgSize();
                    p2Usage.mResizedRawMap.insert(
                                    std::pair<MUINT32, MSize>(
                                            _sensorId,
                                            tempSize));
                }
                if(tempSize.w > maxStreamingSize.w) maxStreamingSize.w = tempSize.w;
                if(tempSize.h > maxStreamingSize.h) maxStreamingSize.h = tempSize.h;
            }
            // for multi-cam case, it has to get max width and height for streaming size.
            p2Usage.mStreamingSize = maxStreamingSize;
            // 2. set feature mode
            p2Usage.mDualFeatureMode = StereoSettingProvider::getStereoFeatureMode();
            // 3. set sensor module mode
            p2Usage.mSensorModule = (bHasMonoSensor) ?  NSCam::v1::Stereo::BAYER_AND_MONO :  NSCam::v1::Stereo::BAYER_AND_BAYER;
            MY_LOGI("sensor module(%d) multicamFeatureMode(%d)", p2Usage.mSensorModule, p2Usage.mDualFeatureMode);
        }

        p2Usage.mOutCfg.mMaxOutNum  = cfgParam.vOutImage.size();
        auto oProcSize =
            pParsedAppImageStreamInfo->vAppImage_Output_Proc.size();
        auto oProcPhySize =
            pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical.size();
        p2Usage.mOutSizeVector.reserve(oProcSize+oProcPhySize);
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
        {
            p2Usage.mOutSizeVector.push_back(n.second->getImgSize());
        }
        //
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
            for(auto&& item:n.second)
                p2Usage.mOutSizeVector.push_back(item->getImgSize());
        }
        for(auto&& out : cfgParam.vOutImage)
        {
            if(out->getImgSize().w > p2Usage.mStreamingSize.w || out->getImgSize().h > p2Usage.mStreamingSize.h)
            {
                p2Usage.mOutCfg.mHasLarge = MTRUE;
            }
                p2Usage.mOutCfg.mHasPhysical =
                    (pParsedMultiCamInfo->mDualDevicePath == NSCam::v3::pipeline::policy::DualDevicePath::MultiCamControl);
        }
        if(cfgParam.pOutFDImage != NULL)
        {
            p2Usage.mOutCfg.mFDSize = cfgParam.pOutFDImage->getImgSize();
        }
        // secure flow
        p2Usage.mSecType = pParsedAppImageStreamInfo->secureInfo.type;
        // HDR10
        p2Usage.mIsHdr10 = pStreamingFeatureSetting->bIsHdr10 ? MTRUE : MFALSE;
        p2Usage.mHdr10Spec = pStreamingFeatureSetting->hdr10Spec;
        //
        p2Usage.mTPICustomConfig.mTimestamp = (MINT64)pParsedAppConfiguration->configTimestamp;
        //
        p2Usage.mDispStreamId = pStreamingFeatureSetting->dispStreamId;

        MY_LOGI("operation_mode=%d p2_type=%d p2_node_type=%d app_mode=%d fd=%dx%d 3dnr_mode=0x%x fsc_mode=0x%x eis_mode=0x%x eis_factor=%d stream_size=%dx%d video=%dx%d p2TpiMargin(%f), p2TpiMargin(%f), dsdnHint(%d) mSecType(%d) timestamp(%" PRId64 "), reqFps(%d) HDR10(%d/%d), DispStreamId(0x%09" PRIx64 "), PQIdx(%d)",
                pParsedAppConfiguration->operationMode, p2Usage.mP2NodeType, p2Usage.mP2NodeType, p2Usage.mAppMode, p2Usage.mOutCfg.mFDSize.w, p2Usage.mOutCfg.mFDSize.h,
                p2Usage.m3DNRMode, p2Usage.mFSCMode, pStreamingFeatureSetting->eisInfo.mode,
                pStreamingFeatureSetting->eisInfo.factor, p2Usage.mStreamingSize.w, p2Usage.mStreamingSize.h, p2Usage.mOutCfg.mVideoSize.w, p2Usage.mOutCfg.mVideoSize.h, p2Usage.mTPMarginRatio, p2Usage.mTotalMarginRatio, p2Usage.mDsdnHint,p2Usage.mSecType,
                p2Usage.mTPICustomConfig.mTimestamp, p2Usage.mInCfg.mReqFps, p2Usage.mIsHdr10, p2Usage.mHdr10Spec, p2Usage.mDispStreamId, p2Usage.mPresetPQIdx);
    }
    cfgParam.mUsageHint = p2Usage;

    //
    auto pNode = makeINodeActor( NodeT::createInstance(P2StreamingNode::PASS2_STREAM, p2Usage) );
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName, 1)); //P2StreamNode (default order: P2StreamNode -> P2CaptureNode)
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    StreamSet inStreamSet;
    StreamSet outStreamSet;
    //
    add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);
    for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
    {
        add_stream_to_set(inStreamSet, _streamInfo_P1.pAppMeta_DynamicP1);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalMeta_DynamicP1);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Imgo);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_SeamlessImgo);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Rrzo);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Lcso);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Rsso);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_ScaledYuv);
        add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_RssoR2);
        if (pParsedAppConfiguration->hasTrackingFocus) {
           add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_FDYuv);
        }

    }
    add_stream_to_set(inStreamSet, pParsedAppImageStreamInfo->pAppImage_Output_Priv);
    //
    add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2StreamNode);
    for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP2StreamNode_Physical) {
        add_stream_to_set(outStreamSet, n.second);
    }
    add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2StreamNode);
    add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV);
    add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV);
    add_stream_to_set(outStreamSet, pParsedAppImageStreamInfo->pAppImage_Depth);
    //
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
        add_stream_to_set(outStreamSet, n.second);
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
        for(auto&& item:n.second)
            add_stream_to_set(outStreamSet, item);
    }
    //
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
        for(auto&& item:n.second)
            add_stream_to_set(outStreamSet, item);
    }
    //
    add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pHalImage_FD_YUV);
    //
    NodeBuilder builder(nodeId, pNode);
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    setImageUsage(pParsedAppImageStreamInfo->pAppImage_Output_Priv, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
    {
        setImageUsage(_streamInfo_P1.pHalImage_P1_Imgo, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_SeamlessImgo, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_Rrzo, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_Lcso, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_Rsso, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_ScaledYuv, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_RssoR2, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        if (pParsedAppConfiguration->hasTrackingFocus) {
            setImageUsage(_streamInfo_P1.pHalImage_P1_FDYuv, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        }
    }
    //
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
        setImageUsage(n.second, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
        for(auto&& item:n.second)
            setImageUsage(item, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }
    //
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
        for(auto&& item:n.second)
            setImageUsage(item, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }
    //
    setImageUsage(pParsedAppImageStreamInfo->pAppImage_Depth      , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV     , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(pParsedStreamInfo_NonP1->pHalImage_FD_YUV       , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_P2CNode(
    android::sp<IPipelineContext> pContext,
    CaptureFeatureSetting const* pCaptureFeatureSetting,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount,
    PipelineNodesNeed const* pPipelineNodesNeed,
    bool bIsZslEnabled,
    bool bIsSW4Cell,
    InternalCommonInputParams const* pCommon
)
{
    typedef P2CaptureNode           NodeT;

    auto const& pPipelineStaticInfo         = pCommon->pPipelineStaticInfo;
    auto const& pPipelineUserConfiguration  = pCommon->pPipelineUserConfiguration;
    auto const& pParsedAppImageStreamInfo   = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto const& pParsedAppConfiguration     = pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedMultiCamInfo          = pParsedAppConfiguration->pParsedMultiCamInfo;
    auto const  main1SensorId = pPipelineStaticInfo->sensorId[0];
    MSize postviewSize(0,0);
    //
    NodeId_T const nodeId = eNODEID_P2CaptureNode;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = main1SensorId;
        initParam.nodeId = nodeId;
        initParam.nodeName = "P2CaptureNode";
        // according p1 node count to add open id.
        for (size_t i = 1; i < useP1NodeCount; i++)
        {
            initParam.subOpenIdList.push_back(pPipelineStaticInfo->sensorId[i]);
        }
    }
    IMetadata::IEntry PVentry = pParsedAppConfiguration->sessionParams.entryFor(MTK_CONTROL_CAPTURE_POSTVIEW_SIZE);
    if( !PVentry.isEmpty() )
    {
        postviewSize = PVentry.itemAt(0, Type2Type<MSize>());
        MY_LOGD("AP set post view size : %dx%d", postviewSize.w, postviewSize.h);
    }
    //
    StreamSet inStreamSet;
    StreamSet outStreamSet;
    NodeT::ConfigParams cfgParam;
    {
        add_meta_stream (outStreamSet, cfgParam.pOutAppMeta,    pParsedStreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode);
        add_meta_stream (outStreamSet, cfgParam.pOutHalMeta,    pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
        add_meta_stream ( inStreamSet, cfgParam.pInAppMeta,     pParsedStreamInfo_NonP1->pAppMeta_Control);
        for(auto&& n:pParsedStreamInfo_NonP1->vAppMeta_DynamicP2CaptureNode_Physical) {
            struct NodeT::ConfigParams::PhysicalStream phyStreams;
            phyStreams.pInAppPhysicalMeta = nullptr; // no needs for current state.
            phyStreams.pOutAppPhysicalMeta = n.second;
            phyStreams.sensorId = (MUINT32)n.first;
            cfgParam.vPhysicalStreamsInfo.push_back(phyStreams);
        }

        add_meta_stream ( inStreamSet, cfgParam.pInAppRetMeta,  pParsedStreamInfo_P1->at(main1SensorId).pAppMeta_DynamicP1);
        add_meta_stream ( inStreamSet, cfgParam.pInHalMeta,     pParsedStreamInfo_P1->at(main1SensorId).pHalMeta_DynamicP1);

        if (pParsedAppImageStreamInfo->pAppImage_Input_RAW16 != nullptr) {
            add_image_stream( inStreamSet, cfgParam.pvInFullRaw, pParsedAppImageStreamInfo->pAppImage_Input_RAW16);
        }
        if (auto pImgoStreamInfo = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Imgo.get()) {
            add_image_stream( inStreamSet, cfgParam.pvInFullRaw, pImgoStreamInfo);
        }
        if (auto pImgoStreamInfo = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_SeamlessImgo.get()) {
            add_image_stream( inStreamSet, cfgParam.pvInSeamlessFullRaw, pImgoStreamInfo);
        }

        if ( pPipelineUserConfiguration->pParsedAppImageStreamInfo->hasRawOut )
        {
            for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW16 )
            {
                if (pPipelineNodesNeed->needRaw16Node == false)
                {
                    MY_LOGI("P2Node-in << App Image Raw16 %s", n.second->toString().c_str());
                    add_image_stream(inStreamSet, cfgParam.pvInFullRaw, n.second);
                }
                if (bIsZslEnabled || pPipelineStaticInfo->isNeedP2ProcessRaw || bIsSW4Cell)
                {
                    MY_LOGI("P2Node-out << App Image Raw16 %s", n.second->toString().c_str());
                    add_image_stream(outStreamSet, cfgParam.pvOutRaw, n.second);
                }
            }
            for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW10 )
            {
                if (pPipelineNodesNeed->needRaw16Node == false)
                {
                    MY_LOGI("P2Node-in << App Image Raw10 %s", n.second->toString().c_str());
                    add_image_stream(inStreamSet, cfgParam.pvInFullRaw, n.second);
                }
                if (bIsZslEnabled || pPipelineStaticInfo->isNeedP2ProcessRaw)
                {
                    MY_LOGI("P2Node-out << App Image Raw10 %s", n.second->toString().c_str());
                    add_image_stream(outStreamSet, cfgParam.pvOutRaw, n.second);
                }
            }
            for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical ) {
                for(auto&& item:n.second)
                {
                    if (pPipelineNodesNeed->needRaw16Node == false)
                    {
                        MY_LOGI("P2Node-in << App Image Raw16(Physical) %s", item->toString().c_str());
                        add_image_stream(inStreamSet, cfgParam.pvInFullRaw, item);
                    }
                    if (bIsZslEnabled || pPipelineStaticInfo->isNeedP2ProcessRaw || bIsSW4Cell)
                    {
                        MY_LOGI("P2Node-out << App Image Raw16(Physical) %s", item->toString().c_str());
                        add_image_stream(outStreamSet, cfgParam.pvOutRaw, item);
                    }
                }
            }
            for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW10_Physical ) {
                for(auto&& item:n.second)
                {
                    if (pPipelineNodesNeed->needRaw16Node == false)
                    {
                        MY_LOGI("P2Node-in << App Image Raw10(Physical) %s", item->toString().c_str());
                        add_image_stream(inStreamSet, cfgParam.pvInFullRaw, item);
                    }
                    if (bIsZslEnabled || pPipelineStaticInfo->isNeedP2ProcessRaw)
                    {
                        MY_LOGI("P2Node-out << App Image Raw10(Physical) %s", item->toString().c_str());
                        add_image_stream(outStreamSet, cfgParam.pvOutRaw, item);
                    }
                }
            }
            if(pParsedAppImageStreamInfo->pAppImage_Output_Priv)
            {
                if (bIsZslEnabled || pPipelineStaticInfo->isNeedP2ProcessRaw || bIsSW4Cell)
                {
                    MY_LOGI("P2Node-out << App Image Priv Raw %s", pParsedAppImageStreamInfo->pAppImage_Output_Priv->toString().c_str());
                    add_image_stream( outStreamSet, cfgParam.pvOutRaw, pParsedAppImageStreamInfo->pAppImage_Output_Priv);
                }
            }
        }
        //
        add_image_stream( inStreamSet, cfgParam.pInResized, pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Rrzo);
        add_image_stream( inStreamSet, cfgParam.pInLcsoRaw, pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Lcso);
        //
        #if 1 // capture node not support main2 yet
        for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
        {
            if(_sensorId == main1SensorId)
                continue;
            struct NodeT::ConfigParams::P1SubStreams subStreams;

            add_meta_stream ( inStreamSet, subStreams.pInAppRetMetaSub, _streamInfo_P1.pAppMeta_DynamicP1);
            add_meta_stream ( inStreamSet, subStreams.pInHalMetaSub,    _streamInfo_P1.pHalMeta_DynamicP1);

            add_image_stream( inStreamSet, subStreams.pInFullRawSub,    _streamInfo_P1.pHalImage_P1_Imgo);
            add_image_stream( inStreamSet, subStreams.pInResizedSub,    _streamInfo_P1.pHalImage_P1_Rrzo);
            add_image_stream( inStreamSet, subStreams.pInLcsoRawSub,    _streamInfo_P1.pHalImage_P1_Lcso);

            subStreams.sensorId = _sensorId;

            cfgParam.vP1SubStreamsInfo.push_back(subStreams);
        }
        #endif
        //
        add_image_stream( inStreamSet, cfgParam.pInFullYuv, pParsedAppImageStreamInfo->pAppImage_Input_Yuv);
        add_image_stream( inStreamSet, cfgParam.pvInOpaque, pParsedAppImageStreamInfo->pAppImage_Input_Priv);
        add_image_stream( inStreamSet, cfgParam.pvInOpaque, pParsedAppImageStreamInfo->pAppImage_Output_Priv);
        //
        cfgParam.pOutPostView = nullptr;
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc )
        {
            if (postviewSize == n.second->getImgSize() && cfgParam.pOutPostView == nullptr)
            {
                add_image_stream(outStreamSet, cfgParam.pOutPostView, n.second);
            }
            else
            {
                add_image_stream(outStreamSet, cfgParam.pvOutImage, n.second);
            }
        }
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
            for(auto&& item:n.second)
                add_image_stream(outStreamSet, cfgParam.pvOutImage, item);
        }
        //
        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc_Physical ) {
            for(auto&& item:n.second)
                add_image_stream(outStreamSet, cfgParam.pvOutImage, item);
        }
        //
        add_image_stream(outStreamSet, cfgParam.pOutJpegYuv,        pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV);
        add_image_stream(outStreamSet, cfgParam.pOutThumbnailYuv,   pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV);
        // [Jpeg packed]
        if(pParsedMultiCamInfo->mSupportPackJpegImages)
        {
            std::bitset<NSCam::v1::Stereo::CallbackBufferType::E_DUALCAM_JPEG_ENUM_SIZE> outBufList =
                                                StereoSettingProvider::getBokehJpegBufferList();
            /*
             * currect combinations only have two types:
             * 1. bokeh image, clean image and depth.
             * 2. relighting bokeh image, bokeh image and depth.
             * For 1., check contain clean image or not.
             * For 2., check contain relighting bokeh image or not.
             */
            // check 1.
            if(outBufList.test(NSCam::v1::Stereo::E_DUALCAM_JPEG_CLEAN_IMAGE))
            {
                // for this case, main image is bokeh image.
                // so, it has to config clean yuv, and prepare pack into jpeg image.
                add_image_stream(outStreamSet, cfgParam.pOutClean, pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV);
            }
            // check 2.
            if(outBufList.test(NSCam::v1::Stereo::E_DUALCAM_JPEG_RELIGHTING_BOKEH_IMAGE))
            {
                // for this case, main image is relight bokeh image.
                // so, it has to config bokeh yuv, and prepare pack into jpeg image.
                add_image_stream(outStreamSet, cfgParam.pOutBokeh, pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV);
            }
            // depth image
            add_image_stream(outStreamSet, cfgParam.pOutDepth, pParsedStreamInfo_NonP1->pHalImage_Depth_YUV);
        }
    }
    //
    MBOOL isSupportedBGPreRelease = MFALSE;
    IMetadata::IEntry BGPentry = pParsedAppConfiguration->sessionParams.entryFor(MTK_BGSERVICE_FEATURE_PRERELEASE);
    if( !BGPentry.isEmpty() )
    {
        isSupportedBGPreRelease = (BGPentry.itemAt(0, Type2Type<MINT32>()) == MTK_BGSERVICE_FEATURE_PRERELEASE_MODE_ON);
        MY_LOGD("Is supported background preRelease : %d", isSupportedBGPreRelease);
    }
    //
    P2Common::Capture::UsageHint p2Usage;
    {
        p2Usage.mSupportedScenarioFeatures = pCaptureFeatureSetting->supportedScenarioFeatures;
        p2Usage.mIsSupportedBGPreRelease = isSupportedBGPreRelease;
        p2Usage.mDualFeatureMode = pCaptureFeatureSetting->dualFeatureMode;
        p2Usage.mPluginUniqueKey = pCaptureFeatureSetting->pluginUniqueKey;
        p2Usage.mAppSessionMeta  = pParsedAppConfiguration->sessionParams;
    }
    cfgParam.mUsageHint = p2Usage;
    //
    auto pNode = makeINodeActor( NodeT::createInstance(P2CaptureNode::PASS2_TIMESHARING, p2Usage) );
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName, 2)); //P2CaptureNode (default order: P2StreamNode -> P2CaptureNode)
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    NodeBuilder builder(nodeId, pNode);
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    setImageUsage(cfgParam.pInFullYuv  , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    for (size_t i = 0; i < cfgParam.pvInOpaque.size(); i++) {
        setImageUsage(cfgParam.pvInOpaque[i], eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    //
    for (size_t i = 0; i < cfgParam.pvInFullRaw.size(); i++) {
        setImageUsage(cfgParam.pvInFullRaw[i], eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    for (size_t i = 0; i < cfgParam.pvInSeamlessFullRaw.size(); i++) {
        setImageUsage(cfgParam.pvInSeamlessFullRaw[i], eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    setImageUsage(cfgParam.pInResized, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    setImageUsage(cfgParam.pInLcsoRaw,    eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);

    for (size_t i = 0; i < cfgParam.vP1SubStreamsInfo.size(); i++) {
        setImageUsage(cfgParam.vP1SubStreamsInfo[i].pInFullRawSub, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(cfgParam.vP1SubStreamsInfo[i].pInResizedSub, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(cfgParam.vP1SubStreamsInfo[i].pInLcsoRawSub, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    //
    setImageUsage(cfgParam.pOutPostView, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    for (size_t i = 0; i < cfgParam.pvOutImage.size(); i++) {
        setImageUsage(cfgParam.pvOutImage[i], eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }
    //
    setImageUsage(cfgParam.pOutJpegYuv,      eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutThumbnailYuv, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    if(pParsedMultiCamInfo->mSupportPackJpegImages)
    {
        std::bitset<NSCam::v1::Stereo::CallbackBufferType::E_DUALCAM_JPEG_ENUM_SIZE> outBufList =
                                                StereoSettingProvider::getBokehJpegBufferList();
        if(outBufList.test(NSCam::v1::Stereo::E_DUALCAM_JPEG_CLEAN_IMAGE))
        {
            setImageUsage(cfgParam.pOutClean, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
        }
        if(outBufList.test(NSCam::v1::Stereo::E_DUALCAM_JPEG_RELIGHTING_BOKEH_IMAGE))
        {
            setImageUsage(cfgParam.pOutBokeh, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
        }
        setImageUsage(cfgParam.pOutDepth, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE | eBUFFER_USAGE_SW_WRITE_OFTEN);
    }
    for (size_t i = 0; i < cfgParam.pvOutRaw.size(); i++) {
        setImageUsage(cfgParam.pvOutRaw[i], eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_FdNode(
    android::sp<IPipelineContext> pContext,
    StreamingFeatureSetting const* pStreamingFeatureSetting,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount,
    InternalCommonInputParams const* pCommon,
    int const sensorId
)
{
    typedef FdNode                  NodeT;
    //
    auto const& pPipelineStaticInfo __unused = pCommon->pPipelineStaticInfo;
    auto idx = pPipelineStaticInfo->getIndexFromSensorId(sensorId);
    NodeId_T const nodeId = NodeIdUtils::getFDNodeId(idx);
    auto const& pPipelineUserConfiguration  = pCommon->pPipelineUserConfiguration;
    auto const& pParsedAppImageStreamInfo   = pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = sensorId;
        initParam.nodeId = nodeId;
        initParam.nodeName = "FDNode";

        for (size_t i = 0; i < useP1NodeCount; i++)
        {
            if (i != idx) {
                initParam.subOpenIdList.push_back(pCommon->pPipelineStaticInfo->sensorId[i]);
            }
        }
    }
    NodeT::ConfigParams cfgParam;
    {
        cfgParam.pInAppMeta    = pParsedStreamInfo_NonP1->pAppMeta_Control;
        cfgParam.pOutAppMeta   = pParsedStreamInfo_NonP1->pAppMeta_DynamicFD;
        cfgParam.preview_W     = pParsedAppImageStreamInfo->previewSize.w;
        cfgParam.preview_H     = pParsedAppImageStreamInfo->previewSize.h;
        if ( pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV )
        {
            cfgParam.pInHalMeta    = pParsedStreamInfo_P1->at(sensorId).pHalMeta_DynamicP1;
            cfgParam.vInImage      = pParsedStreamInfo_P1->at(sensorId).pHalImage_P1_FDYuv;
            cfgParam.P1_Rrzo       = pParsedStreamInfo_P1->at(sensorId).pHalImage_P1_Rrzo;
        }
        else
        {
            cfgParam.pInHalMeta    = pParsedStreamInfo_NonP1->pHalMeta_DynamicP2StreamNode;
            cfgParam.vInImage      = pParsedStreamInfo_NonP1->pHalImage_FD_YUV;
        }
        cfgParam.fscEnable = EFSC_FSC_ENABLED(pStreamingFeatureSetting->fscMode);
        cfgParam.isEisOn = pStreamingFeatureSetting->bIsEIS;
        cfgParam.isDirectYuv = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV;
        cfgParam.isVideoMode = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo->hasVideoConsumer;
        cfgParam.isVsdof = (MTK_MULTI_CAM_FEATURE_MODE_VSDOF == pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo->mDualFeatureMode);
    }
    //
    auto pNode = makeINodeActor( NodeT::createInstance() );
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName)); //FDNode
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    StreamSet inStreamSet;
    StreamSet outStreamSet;
    //
    add_stream_to_set(inStreamSet, cfgParam.pInAppMeta);
    add_stream_to_set(inStreamSet, cfgParam.pInHalMeta);
    add_stream_to_set(inStreamSet, cfgParam.vInImage);
    //
    add_stream_to_set(outStreamSet, cfgParam.pOutAppMeta);
    //
    NodeBuilder builder(nodeId, pNode);
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    setImageUsage(cfgParam.vInImage , eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_JpegNode(
    android::sp<IPipelineContext> pContext,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount,
    InternalCommonInputParams const* pCommon
)
{
    typedef JpegNode                NodeT;
    auto const& pParsedAppImageStreamInfo = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    auto const& pParsedAppConfiguration   = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration;
    auto const& pParsedMultiCamInfo        = pParsedAppConfiguration->pParsedMultiCamInfo;
    //
    NodeId_T const nodeId = eNODEID_JpegNode;
    //
    MBOOL isSupportedBGPrerelease = MFALSE;
    IMetadata::IEntry BGPentry = pParsedAppConfiguration->sessionParams.entryFor(MTK_BGSERVICE_FEATURE_PRERELEASE);
    if( !BGPentry.isEmpty() )
    {
        isSupportedBGPrerelease = (BGPentry.itemAt(0, Type2Type<MINT32>()) == MTK_BGSERVICE_FEATURE_PRERELEASE_MODE_ON);
        MY_LOGD("Is supported background prerelease : %d", isSupportedBGPrerelease);
    }
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = pCommon->pPipelineStaticInfo->sensorId[0];
        initParam.nodeId = nodeId;
        initParam.nodeName = "JpegNode";
        for (size_t i = 1; i < useP1NodeCount; i++)
        {
            initParam.subOpenIdList.push_back(pCommon->pPipelineStaticInfo->sensorId[i]);
        }
    }
    NodeT::ConfigParams cfgParam;

    cfgParam.pInAppMeta                 = pParsedStreamInfo_NonP1->pAppMeta_Control;
    cfgParam.pInHalMeta_capture         = pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode;
    cfgParam.pInHalMeta_streaming       = pParsedStreamInfo_NonP1->pHalMeta_DynamicP2StreamNode;
    cfgParam.pOutAppMeta                = nullptr;
    cfgParam.pInYuv_Thumbnail           = nullptr;
    cfgParam.pInAppYuv_Main             = nullptr;
    cfgParam.pOutInternalJpeg           = nullptr;
    if( pParsedAppImageStreamInfo->pAppImage_Jpeg.get())
    {
        cfgParam.pOutAppMeta       = pParsedStreamInfo_NonP1->pAppMeta_DynamicJpeg;
        cfgParam.pInYuv_Thumbnail  = pParsedStreamInfo_NonP1->pHalImage_Thumbnail_YUV;
        auto imgFormat = pParsedAppImageStreamInfo->pAppImage_Jpeg->getImgFormat();
        if (imgFormat == eImgFmt_JPEG || imgFormat == eImgFmt_HEIF) {
            cfgParam.pInYuv_Main       = pParsedStreamInfo_NonP1->pHalImage_Jpeg_YUV;
            cfgParam.pOutJpeg          = pParsedAppImageStreamInfo->pAppImage_Jpeg;
        } else { // Heic capture
            cfgParam.pOutJpegAppSegment = pParsedAppImageStreamInfo->pAppImage_Jpeg;
            auto getYuvSize = [=]() {
                MSize size(0, 0);
                for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc ) {
                    auto pImageStreamInfo = n.second;
                    if (pImageStreamInfo != nullptr) {
                        if (static_cast<Dataspace>(pImageStreamInfo->getDataSpace()) == Dataspace::HEIF) {
                            return pImageStreamInfo->getImgSize();
                        }
                    }
                }
                return size;
            };
            cfgParam.heicYuvSize = getYuvSize();
        }
        cfgParam.bIsPreReleaseEnable = isSupportedBGPrerelease;
        // [jpeg packed]
        if(pParsedMultiCamInfo->mSupportPackJpegImages)
        {
            cfgParam.pInYuv_Main2 = pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV;
            cfgParam.pInYuv_Y16 = pParsedStreamInfo_NonP1->pHalImage_Depth_YUV;
        }
    }
    //configure haljpeg output for tuning & debug
    if ( pParsedStreamInfo_NonP1->pHalImage_Jpeg.get() )
    {
        android::sp<IImageStreamInfo> pInYuv_Main = nullptr;

        for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_Proc ) {
            android::sp<IImageStreamInfo> pImageStreamInfo = n.second;
            if (pInYuv_Main == nullptr) {
               pInYuv_Main = pImageStreamInfo;
               continue;
            }
            //max. size?
            if (pInYuv_Main->getImgSize().w * pInYuv_Main->getImgSize().h
              < pImageStreamInfo->getImgSize().w * pImageStreamInfo->getImgSize().h )
            {
                pInYuv_Main = pImageStreamInfo;
            }
        }

        cfgParam.pInAppYuv_Main         = pInYuv_Main;
        cfgParam.pOutInternalJpeg       = pParsedStreamInfo_NonP1->pHalImage_Jpeg;
        MY_LOGD("add hal jpeg for dump, input app yuv size(%dx%d)", pInYuv_Main->getImgSize().w, pInYuv_Main->getImgSize().h);
    }

    //
    auto pNode = makeINodeActor( NodeT::createInstance() );
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName)); //JpegNode
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    StreamSet inStreamSet;
    StreamSet outStreamSet;
    //
    add_stream_to_set(inStreamSet, cfgParam.pInAppMeta);
    add_stream_to_set(inStreamSet, cfgParam.pInHalMeta_capture);
    add_stream_to_set(inStreamSet, cfgParam.pInHalMeta_streaming);
    add_stream_to_set(inStreamSet, cfgParam.pInYuv_Main);
    add_stream_to_set(inStreamSet, cfgParam.pInYuv_Thumbnail);
    add_stream_to_set(inStreamSet, cfgParam.pInAppYuv_Main);
    // [jpeg packed]
    if(pParsedMultiCamInfo->mSupportPackJpegImages)
    {
        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV);
        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pHalImage_Depth_YUV);
    }
    //
    add_stream_to_set(outStreamSet, cfgParam.pOutAppMeta);
    add_stream_to_set(outStreamSet, cfgParam.pOutJpeg);
    add_stream_to_set(outStreamSet, cfgParam.pOutJpegAppSegment);
    add_stream_to_set(outStreamSet, cfgParam.pOutInternalJpeg);
    //
    NodeBuilder builder(nodeId, pNode);
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    setImageUsage(cfgParam.pInYuv_Main, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    setImageUsage(cfgParam.pInYuv_Thumbnail, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    setImageUsage(cfgParam.pInAppYuv_Main, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    if(pParsedMultiCamInfo->mSupportPackJpegImages)
    {
        setImageUsage(pParsedStreamInfo_NonP1->pHalImage_Jpeg_Sub_YUV, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(pParsedStreamInfo_NonP1->pHalImage_Depth_YUV, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    setImageUsage(cfgParam.pOutJpeg, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    setImageUsage(cfgParam.pOutInternalJpeg, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}

/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_P1RawIspTuningDataPackNode(
    android::sp<IPipelineContext> pContext,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount __unused,
    InternalCommonInputParams const* pCommon
)
{
    typedef IspTuningDataPackNode                NodeT;
    auto const& pParsedAppImageStreamInfo = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    //auto const& pParsedAppConfiguration   = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration;
    //
    NodeId_T const nodeId = eNODEID_P1RawIspTuningDataPackNode;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = pCommon->pPipelineStaticInfo->openId;
        initParam.nodeId = nodeId;
        initParam.nodeName = "P1IspTuningDataPackNode";
    }

    auto pNode = makeINodeActor( NodeT::createInstance() );
    NodeBuilder builder(nodeId, pNode);
    NodeT::ConfigParams cfgParam;
    cfgParam.sourceType = IspTuningDataPackNode::RawFromP1;

    StreamSet inStreamSet;
    StreamSet outStreamSet;

    add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);

    // Logical
    {
        // NOTE: For multicam, the stream info will change according to the runtime active sensor
        NodeT::ConfigParams::PackStreamInfo streamInfo;
        streamInfo.pvInAppCtlMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_Control);
        for (auto&& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
        {
            streamInfo.pvInP1AppMetaInfo.push_back(_streamInfo_P1.pAppMeta_DynamicP1);
            streamInfo.pvInHalMetaInfo.push_back(_streamInfo_P1.pHalMeta_DynamicP1);
            streamInfo.pvInImgInfo_rrzo.push_back(_streamInfo_P1.pHalImage_P1_Rrzo);
            streamInfo.pvInImgInfo_statistics.push_back(_streamInfo_P1.pHalImage_P1_Lcso);

            add_stream_to_set(inStreamSet, _streamInfo_P1.pAppMeta_DynamicP1);
            add_stream_to_set(inStreamSet, _streamInfo_P1.pHalMeta_DynamicP1);
            add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Rrzo);
            add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Lcso);
        }

        streamInfo.pOutImgInfo     = pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw;
        streamInfo.pOutAppMetaInfo = pParsedStreamInfo_NonP1->pAppMeta_DynamicP1ISPPack;

        cfgParam.pvStreamInfoMap.emplace(pCommon->pPipelineStaticInfo->openId, streamInfo);

        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);
        //
        add_stream_to_set(outStreamSet, pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw);
        add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pAppMeta_DynamicP1ISPPack);

        setImageUsage(pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }

    // Physical
    for(auto sensorId : pCommon->pPipelineStaticInfo->sensorId)
    {
        auto &vIspTuningDataRawPhysicalImg = pParsedAppImageStreamInfo->vAppImage_Output_IspTuningData_Raw_Physical;
        auto iterImg = vIspTuningDataRawPhysicalImg.find(sensorId);
        auto &vIspTuningDataRawPhysicalMeta = pParsedStreamInfo_NonP1->vAppMeta_DynamicP1ISPPack_Physical;
        auto iterMeta = vIspTuningDataRawPhysicalMeta.find(sensorId);
        auto pIspTuningDataMetaStreamInfo = (iterMeta != vIspTuningDataRawPhysicalMeta.end()) ? iterMeta->second : nullptr;
        auto pIspTuningDataImgStreamInfo = (iterImg != vIspTuningDataRawPhysicalImg.end()) ? iterImg->second : nullptr;

        if( pIspTuningDataMetaStreamInfo == nullptr &&
            pIspTuningDataImgStreamInfo == nullptr )
        {
            MY_LOGD("No physical ISP tuning buffer and metadata for sensor %d", sensorId);
            continue;
        }

        NodeT::ConfigParams::PackStreamInfo streamInfo;
        auto &p1StreamInfo = pParsedStreamInfo_P1->at(sensorId);
        streamInfo.pvInAppCtlMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_Control);
        streamInfo.pvInP1AppMetaInfo.push_back(p1StreamInfo.pAppMeta_DynamicP1);
        streamInfo.pvInHalMetaInfo.push_back(p1StreamInfo.pHalMeta_DynamicP1);
        streamInfo.pvInImgInfo_rrzo.push_back(p1StreamInfo.pHalImage_P1_Rrzo);
        streamInfo.pvInImgInfo_statistics.push_back(p1StreamInfo.pHalImage_P1_Lcso);

        streamInfo.pOutImgInfo     = pIspTuningDataImgStreamInfo;
        streamInfo.pOutAppMetaInfo = pIspTuningDataMetaStreamInfo;

        cfgParam.pvStreamInfoMap.emplace(sensorId, streamInfo);
        //
        add_stream_to_set(inStreamSet, p1StreamInfo.pAppMeta_DynamicP1);
        add_stream_to_set(inStreamSet, p1StreamInfo.pHalMeta_DynamicP1);
        add_stream_to_set(inStreamSet, p1StreamInfo.pHalImage_P1_Rrzo);
        add_stream_to_set(inStreamSet, p1StreamInfo.pHalImage_P1_Lcso);
        //
        add_stream_to_set(outStreamSet, pIspTuningDataMetaStreamInfo);
        add_stream_to_set(outStreamSet, pIspTuningDataImgStreamInfo);

        setImageUsage(pIspTuningDataImgStreamInfo, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }
    //
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName));
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
    {
        setImageUsage(_streamInfo_P1.pHalImage_P1_Rrzo, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_Lcso, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    //
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_P2YuvIspTuningDataPackNode(
    android::sp<IPipelineContext> pContext,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount __unused,
    InternalCommonInputParams const* pCommon
)
{
    typedef IspTuningDataPackNode                NodeT;
    auto const& pParsedAppImageStreamInfo = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    //auto const& pParsedAppConfiguration   = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration;
    //
    NodeId_T const nodeId = eNODEID_P2YuvIspTuningDataPackNode;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = pCommon->pPipelineStaticInfo->openId;
        initParam.nodeId = nodeId;
        initParam.nodeName = "P2YuvIspTuningDataPackNode";
    }

    auto pNode = makeINodeActor( NodeT::createInstance() );
    NodeBuilder builder(nodeId, pNode);
    NodeT::ConfigParams cfgParam;
    cfgParam.sourceType = IspTuningDataPackNode::YuvFromP2;

    StreamSet inStreamSet;
    StreamSet outStreamSet;

    add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);

    // Logical
    {
        NodeT::ConfigParams::PackStreamInfo streamInfo;
        for (auto&& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
        {
            streamInfo.pvInP1AppMetaInfo.push_back(_streamInfo_P1.pAppMeta_DynamicP1);
            streamInfo.pvInImgInfo_statistics.push_back(_streamInfo_P1.pHalImage_P1_Lcso);

            add_stream_to_set(inStreamSet, _streamInfo_P1.pAppMeta_DynamicP1);
            add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Lcso);
        }
        streamInfo.pvInAppCtlMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_Control);
        streamInfo.pvInP2AppMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode);
        streamInfo.pvInHalMetaInfo.push_back(pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);

        streamInfo.pOutImgInfo     = pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Yuv;
        streamInfo.pOutAppMetaInfo = pParsedStreamInfo_NonP1->pAppMeta_DynamicP2ISPPack;

        cfgParam.pvStreamInfoMap.emplace(pCommon->pPipelineStaticInfo->openId, streamInfo);

        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);
        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode);
        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
        //
        add_stream_to_set(outStreamSet, pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Yuv);
        add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2ISPPack);

        setImageUsage(pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Yuv, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }

    // Physical
    for(auto sensorId : pCommon->pPipelineStaticInfo->sensorId)
    {
        auto &vIspTuningDataYuvPhysicalImg = pParsedAppImageStreamInfo->vAppImage_Output_IspTuningData_Yuv_Physical;
        auto iterImg = vIspTuningDataYuvPhysicalImg.find(sensorId);
        auto &vIspTuningDataYuvPhysicalMeta = pParsedStreamInfo_NonP1->vAppMeta_DynamicP2ISPPack_Physical;
        auto iterMeta = vIspTuningDataYuvPhysicalMeta.find(sensorId);

        auto pIspTuningDataMetaStreamInfo = (iterMeta != vIspTuningDataYuvPhysicalMeta.end()) ? iterMeta->second : nullptr;
        auto pIspTuningDataImgStreamInfo = (iterImg != vIspTuningDataYuvPhysicalImg.end()) ? iterImg->second : nullptr;

        if( pIspTuningDataMetaStreamInfo == nullptr &&
            pIspTuningDataImgStreamInfo == nullptr )
        {
            MY_LOGD("No physical ISP tuning buffer and metadata for sensor %d", sensorId);
            continue;
        }

        int const p1Index = pCommon->pPipelineStaticInfo->getIndexFromSensorId(sensorId);
        auto appMetaIter = pParsedStreamInfo_NonP1->vAppMeta_DynamicP2CaptureNode_Physical.find(sensorId);
        NodeT::ConfigParams::PackStreamInfo streamInfo;
        if ( CC_UNLIKELY(p1Index < 0) )
        {
            MY_LOGE("Cannot find p1 index from sensorId(%d)", sensorId);
            continue;
        }
        else
        {
            auto &p1StreamInfo = pParsedStreamInfo_P1->at(sensorId);
            streamInfo.pvInAppCtlMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_Control);
            streamInfo.pvInP1AppMetaInfo.push_back(p1StreamInfo.pAppMeta_DynamicP1);
            if(appMetaIter != pParsedStreamInfo_NonP1->vAppMeta_DynamicP2CaptureNode_Physical.end())
            {
                streamInfo.pvInP2AppMetaInfo.push_back(appMetaIter->second);
                add_stream_to_set(inStreamSet, appMetaIter->second);
            }
            // streamInfo.pvInHalMetaInfo.push_back(pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
            streamInfo.pvInHalMetaInfo.push_back(p1StreamInfo.pHalMeta_DynamicP1);
            streamInfo.pvInImgInfo_statistics.push_back(p1StreamInfo.pHalImage_P1_Lcso);

            streamInfo.pOutImgInfo     = pIspTuningDataImgStreamInfo;
            streamInfo.pOutAppMetaInfo = pIspTuningDataMetaStreamInfo;

            cfgParam.pvStreamInfoMap.emplace(sensorId, streamInfo);
            //
            add_stream_to_set(inStreamSet, p1StreamInfo.pAppMeta_DynamicP1);
            // add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
            add_stream_to_set(inStreamSet, p1StreamInfo.pHalMeta_DynamicP1);
            add_stream_to_set(inStreamSet, p1StreamInfo.pHalImage_P1_Lcso);
            //
            add_stream_to_set(outStreamSet, pIspTuningDataMetaStreamInfo);
            add_stream_to_set(outStreamSet, pIspTuningDataImgStreamInfo);
            //
            setImageUsage(pIspTuningDataImgStreamInfo, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
        }
    }
    //
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName));
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
    {
        setImageUsage(_streamInfo_P1.pHalImage_P1_Lcso, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}

/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_P2RawIspTuningDataPackNode(
    android::sp<IPipelineContext> pContext,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount __unused,
    InternalCommonInputParams const* pCommon
)
{
    typedef IspTuningDataPackNode                NodeT;
    auto const& pParsedAppImageStreamInfo = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    //auto const& pParsedAppConfiguration   = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration;
    //
    NodeId_T const nodeId = eNODEID_P2RawIspTuningDataPackNode;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = pCommon->pPipelineStaticInfo->openId;
        initParam.nodeId = nodeId;
        initParam.nodeName = "P2RawIspTuningDataPackNode";
    }

    auto pNode = makeINodeActor( NodeT::createInstance() );
    NodeBuilder builder(nodeId, pNode);
    NodeT::ConfigParams cfgParam;
    cfgParam.sourceType = IspTuningDataPackNode::RawFromP2;

    StreamSet inStreamSet;
    StreamSet outStreamSet;

    add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);

    // Logical
    {
        // NOTE: For multicam, the stream info will change according to the runtime active sensor
        NodeT::ConfigParams::PackStreamInfo streamInfo;
        for (auto&& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
        {
            streamInfo.pvInP1AppMetaInfo.push_back(_streamInfo_P1.pAppMeta_DynamicP1);
            streamInfo.pvInImgInfo_rrzo.push_back(_streamInfo_P1.pHalImage_P1_Rrzo);
            streamInfo.pvInImgInfo_statistics.push_back(_streamInfo_P1.pHalImage_P1_Lcso);

            add_stream_to_set(inStreamSet, _streamInfo_P1.pAppMeta_DynamicP1);
            add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Rrzo);
            add_stream_to_set(inStreamSet, _streamInfo_P1.pHalImage_P1_Lcso);
        }
        streamInfo.pvInAppCtlMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_Control);
        streamInfo.pvInP2AppMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode);
        streamInfo.pvInHalMetaInfo.push_back(pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);

        streamInfo.pOutImgInfo     = pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw;
        streamInfo.pOutAppMetaInfo = pParsedStreamInfo_NonP1->pAppMeta_DynamicP2ISPPack;

        cfgParam.pvStreamInfoMap.emplace(pCommon->pPipelineStaticInfo->openId, streamInfo);

        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_Control);
        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2CaptureNode);
        add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
        //
        add_stream_to_set(outStreamSet, pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw);
        add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pAppMeta_DynamicP2ISPPack);

        setImageUsage(pParsedAppImageStreamInfo->pAppImage_Output_IspTuningData_Raw, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }

    // Physical
    for(auto sensorId : pCommon->pPipelineStaticInfo->sensorId)
    {
        auto &vIspTuningDataRawPhysicalImg = pParsedAppImageStreamInfo->vAppImage_Output_IspTuningData_Raw_Physical;
        auto iterImg = vIspTuningDataRawPhysicalImg.find(sensorId);
        auto &vIspTuningDataYuvPhysicalMeta = pParsedStreamInfo_NonP1->vAppMeta_DynamicP2ISPPack_Physical;
        auto iterMeta = vIspTuningDataYuvPhysicalMeta.find(sensorId);
        auto pIspTuningDataMetaStreamInfo = (iterMeta != vIspTuningDataYuvPhysicalMeta.end()) ? iterMeta->second : nullptr;
        auto pIspTuningDataImgStreamInfo = (iterImg != vIspTuningDataRawPhysicalImg.end()) ? iterImg->second : nullptr;

        if( pIspTuningDataMetaStreamInfo == nullptr &&
            pIspTuningDataImgStreamInfo == nullptr )
        {
            MY_LOGD("No physical ISP tuning buffer and metadata for sensor %d", sensorId);
            continue;
        }

        auto appMetaIter = pParsedStreamInfo_NonP1->vAppMeta_DynamicP2CaptureNode_Physical.find(sensorId);
        NodeT::ConfigParams::PackStreamInfo streamInfo;
        auto &p1StreamInfo = pParsedStreamInfo_P1->at(sensorId);
        {
            streamInfo.pvInAppCtlMetaInfo.push_back(pParsedStreamInfo_NonP1->pAppMeta_Control);
            streamInfo.pvInP1AppMetaInfo.push_back(p1StreamInfo.pAppMeta_DynamicP1);
            if(appMetaIter != pParsedStreamInfo_NonP1->vAppMeta_DynamicP2CaptureNode_Physical.end())
            {
                streamInfo.pvInP2AppMetaInfo.push_back(appMetaIter->second);
                add_stream_to_set(inStreamSet, appMetaIter->second);
            }
            // streamInfo.pvInHalMetaInfo.push_back(pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
            streamInfo.pvInHalMetaInfo.push_back(p1StreamInfo.pHalMeta_DynamicP1);
            streamInfo.pvInImgInfo_rrzo.push_back(p1StreamInfo.pHalImage_P1_Rrzo);
            streamInfo.pvInImgInfo_statistics.push_back(p1StreamInfo.pHalImage_P1_Lcso);

            streamInfo.pOutImgInfo     = pIspTuningDataImgStreamInfo;
            streamInfo.pOutAppMetaInfo = pIspTuningDataMetaStreamInfo;
            cfgParam.pvStreamInfoMap.emplace(sensorId, streamInfo);
        }
        //
        add_stream_to_set(inStreamSet, p1StreamInfo.pAppMeta_DynamicP1);
        // add_stream_to_set(inStreamSet, pParsedStreamInfo_NonP1->pHalMeta_DynamicP2CaptureNode);
        add_stream_to_set(inStreamSet, p1StreamInfo.pHalMeta_DynamicP1);
        add_stream_to_set(inStreamSet, p1StreamInfo.pHalImage_P1_Rrzo);
        add_stream_to_set(inStreamSet, p1StreamInfo.pHalImage_P1_Lcso);
        //
        add_stream_to_set(outStreamSet, pIspTuningDataMetaStreamInfo);
        add_stream_to_set(outStreamSet, pIspTuningDataImgStreamInfo);
        //
        setImageUsage(pIspTuningDataImgStreamInfo, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_WRITE);
    }

    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName));
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);

    for (const auto& [_sensorId, _streamInfo_P1] : *pParsedStreamInfo_P1)
    {
        setImageUsage(_streamInfo_P1.pHalImage_P1_Rrzo, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
        setImageUsage(_streamInfo_P1.pHalImage_P1_Lcso, eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_HW_CAMERA_READ);
    }
    //
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_Raw16Node(
    android::sp<IPipelineContext> pContext,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount __unused,
    InternalCommonInputParams const* pCommon
)
{
    typedef RAW16Node               NodeT;
    int err = 0;
    auto const& pParsedAppImageStreamInfo = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    //
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW16 )
    {
        // get physical sensor list
        for(auto&& psi:pCommon->pPipelineStaticInfo->sensorId)
        {
            int const p1Index = pCommon->pPipelineStaticInfo->getIndexFromSensorId(psi);
            if(p1Index < 0)
            {
                err = UNKNOWN_ERROR;
                MY_LOGE("sensor id(%d) not exist in sensor list", psi);
                break;
            }
            NodeId_T const nodeId = NodeIdUtils::getRaw16NodeId(p1Index);
            //
            NodeT::InitParams initParam;
            {
                initParam.openId = psi;
                initParam.nodeId = nodeId;
                initParam.nodeName = "Raw16Node";
                for (auto&& _psi:pCommon->pPipelineStaticInfo->sensorId)
                {
                    if(psi != _psi)
                        initParam.subOpenIdList.push_back(_psi);
                }
            }
            NodeT::ConfigParams cfgParam;
            {
                cfgParam.pInAppMeta = pParsedStreamInfo_P1->at(psi).pAppMeta_DynamicP1;
                cfgParam.pInHalMeta = pParsedStreamInfo_P1->at(psi).pHalMeta_DynamicP1;
                cfgParam.pOutAppMeta  = pParsedStreamInfo_NonP1->pAppMeta_DynamicRAW16;
            }
            //
            auto pNode = makeINodeActor( NodeT::createInstance() );
            pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName)); //Raw16Node
            pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
            pNode->setInitParam(initParam);
            pNode->setConfigParam(&cfgParam);
            //
            StreamSet inStreamSet;
            StreamSet outStreamSet;
            //
            add_stream_to_set(inStreamSet, pParsedStreamInfo_P1->at(psi).pHalImage_P1_Imgo);
            add_stream_to_set(inStreamSet, cfgParam.pInHalMeta);
            add_stream_to_set(inStreamSet, cfgParam.pInAppMeta);
            //
            add_stream_to_set(outStreamSet, n.second);
            add_stream_to_set(outStreamSet, cfgParam.pOutAppMeta);
            //
            NodeBuilder builder(nodeId, pNode);
            builder
                .addStream(NodeBuilder::eDirection_IN, inStreamSet)
                .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
            //
            setImageUsage(pParsedStreamInfo_P1->at(psi).pHalImage_P1_Imgo, eBUFFER_USAGE_SW_READ_OFTEN);
            setImageUsage(n.second, eBUFFER_USAGE_SW_WRITE_OFTEN);
            //
            err = builder.build(pContext);
            if( err != OK )
                MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
        }
    }
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_Raw16Node_Physical(
    android::sp<IPipelineContext> pContext,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount __unused,
    InternalCommonInputParams const* pCommon
)
{
    typedef RAW16Node               NodeT;
    int err = 0;
    auto const& pParsedAppImageStreamInfo = pCommon->pPipelineUserConfiguration->pParsedAppImageStreamInfo;
    //
    for( const auto& n : pParsedAppImageStreamInfo->vAppImage_Output_RAW16_Physical )
    {
        int sensorId = n.first;
        int p1Index = pCommon->pPipelineStaticInfo->getIndexFromSensorId(sensorId);
        if (p1Index == -1)
        {
            MY_LOGD("p1Index == -1, cannot map raw16 stream to a real sensor id, force p1Index = 0");
            p1Index = 0;
        }
        NodeId_T const nodeId = NodeIdUtils::getRaw16NodeId(p1Index);
        //
        NodeT::InitParams initParam;
        {
            initParam.openId = sensorId;
            initParam.nodeId = nodeId;
            initParam.nodeName = "Raw16Node_Phy";
            for (auto&& psi:pCommon->pPipelineStaticInfo->sensorId)
            {
                if(sensorId != psi)
                    initParam.subOpenIdList.push_back(psi);
            }
        }
        NodeT::ConfigParams cfgParam;
        {
            cfgParam.pInAppMeta = pParsedStreamInfo_P1->at(sensorId).pAppMeta_DynamicP1;
            cfgParam.pInHalMeta = pParsedStreamInfo_P1->at(sensorId).pHalMeta_DynamicP1;
            auto iter = pParsedStreamInfo_NonP1->vAppMeta_DynamicRAW16_Physical.find(sensorId);
            if(iter != pParsedStreamInfo_NonP1->vAppMeta_DynamicRAW16_Physical.end())
            {
                cfgParam.pOutAppMeta = iter->second;
            }
            else
            {
                cfgParam.pOutAppMeta = nullptr;
            }
        }
        //
        auto pNode = makeINodeActor( NodeT::createInstance() );
        pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName)); //Raw16Node
        pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
        pNode->setInitParam(initParam);
        pNode->setConfigParam(&cfgParam);
        //
        StreamSet inStreamSet;
        StreamSet outStreamSet;
        //
        add_stream_to_set(inStreamSet, pParsedStreamInfo_P1->at(sensorId).pHalImage_P1_Imgo);
        add_stream_to_set(inStreamSet, cfgParam.pInHalMeta);
        add_stream_to_set(inStreamSet, cfgParam.pInAppMeta);
        //
        for( const auto& stream : n.second) {
            add_stream_to_set(outStreamSet, stream);
        }
        add_stream_to_set(outStreamSet, cfgParam.pOutAppMeta);
        //
        NodeBuilder builder(nodeId, pNode);
        builder
            .addStream(NodeBuilder::eDirection_IN, inStreamSet)
            .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
        //
        setImageUsage(pParsedStreamInfo_P1->at(sensorId).pHalImage_P1_Imgo, eBUFFER_USAGE_SW_READ_OFTEN);
        for( const auto& stream : n.second) {
            setImageUsage(stream, eBUFFER_USAGE_SW_WRITE_OFTEN);
        }

        //
        err = builder.build(pContext);
        if( err != OK )
            MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    }
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_PDENode(
    android::sp<IPipelineContext> pContext,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    uint32_t useP1NodeCount,
    InternalCommonInputParams const* pCommon
)
{
    typedef PDENode               NodeT;
    auto const main1SensorId = pCommon->pPipelineStaticInfo->sensorId[0];
    //
    NodeId_T const nodeId = eNODEID_PDENode;
    //
    NodeT::InitParams initParam;
    {
        initParam.openId = pCommon->pPipelineStaticInfo->sensorId[0];
        initParam.nodeId = nodeId;
        initParam.nodeName = "PDENode";
        for (size_t i = 1; i < useP1NodeCount; i++)
        {
            initParam.subOpenIdList.push_back(pCommon->pPipelineStaticInfo->sensorId[i]);
        }
    }
    NodeT::ConfigParams cfgParam;
    {
        cfgParam.pInP1HalMeta = pParsedStreamInfo_P1->at(main1SensorId).pHalMeta_DynamicP1;
        cfgParam.pOutHalMeta  = pParsedStreamInfo_NonP1->pHalMeta_DynamicPDE;
        cfgParam.pInRawImage  = pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Imgo;
    }
    //
    auto pNode = makeINodeActor( NodeT::createInstance() );
    pNode->setInitOrder(queryNodeInitOrder(initParam.nodeName)); //PDENode
    pNode->setInitTimeout(queryNodeInitTimeout(initParam.nodeName));
    pNode->setInitParam(initParam);
    pNode->setConfigParam(&cfgParam);
    //
    StreamSet inStreamSet;
    StreamSet outStreamSet;
    //
    add_stream_to_set(inStreamSet, pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Imgo);
    add_stream_to_set(inStreamSet, pParsedStreamInfo_P1->at(main1SensorId).pHalMeta_DynamicP1);
    //
    add_stream_to_set(outStreamSet, pParsedStreamInfo_NonP1->pHalMeta_DynamicPDE);
    //
    NodeBuilder builder(nodeId, pNode);
    builder
        .addStream(NodeBuilder::eDirection_IN, inStreamSet)
        .addStream(NodeBuilder::eDirection_OUT, outStreamSet);
    //
    setImageUsage(pParsedStreamInfo_P1->at(main1SensorId).pHalImage_P1_Imgo, eBUFFER_USAGE_HW_CAMERA_READ | eBUFFER_USAGE_SW_READ_OFTEN);
    //
    int err = builder.build(pContext);
    if( err != OK )
        MY_LOGE("build node %#" PRIxPTR " failed", nodeId);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_Nodes(
    android::sp<IPipelineContext> pContext,
    android::sp<IPipelineContext> const& pOldPipelineContext,
    StreamingFeatureSetting const* pStreamingFeatureSetting,
    CaptureFeatureSetting const* pCaptureFeatureSetting,
    SeamlessSwitchFeatureSetting const* pSeamlessSwitchFeatureSetting,
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1,
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1,
    PipelineNodesNeed const* pPipelineNodesNeed,
    SensorMap<SensorSetting> const* pSensorSetting,
    SensorMap<P1HwSetting> const* pvP1HwSetting,
    SensorMap<uint32_t> const* pBatchSize,
    bool bIsZslEnabled,
    bool bIsSW4Cell,
    bool bIsP1OnlyOutImgo,
    InternalCommonInputParams const* pCommon,
    BuildPipelineContextOutputParams *outParam
)
{
    CAM_TRACE_CALL();

    auto const& pPipelineStaticInfo = pCommon->pPipelineStaticInfo;

    uint32_t useP1NodeCount = 0;
    bool bMultiCam_CamSvPath = false;
    bool isReConfig          = false;
    for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node ) {
        if (_needP1) useP1NodeCount++;
    }

    // if useP1NodeCount more than 1, it has to create sync helper
    // and assign to p1 config param.
    if(useP1NodeCount > 1)
    {
        // 1. set sync helper.
        uint64_t timestamp = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->configTimestamp;
        auto initRequest = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->initRequest;
        typedef NSCam::v3::Utils::Imp::ISyncHelper  MultiCamSyncHelper;
        bool enableCropOverride = false;
        {
            auto const& pParsedMultiCamInfo = pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->pParsedMultiCamInfo;
            if(pParsedMultiCamInfo != nullptr &&
               pParsedMultiCamInfo->mDualFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_ZOOM)
            {
                //enableCropOverride = true;
                enableCropOverride = false;
            }
        }
        sp<MultiCamSyncHelper> pSyncHelper = MultiCamSyncHelper::createInstance(
                                                                    pPipelineStaticInfo->openId,
                                                                    timestamp,
                                                                    initRequest,
                                                                    enableCropOverride);
        if(pSyncHelper.get())
        {
            pContext->setMultiCamSyncHelper(pSyncHelper);
        }
        // 2. check may use camsv flow or not.
        for (auto&& [_sensorId, _streamInfo_P1] : (*pParsedStreamInfo_P1))
        {
            if(_streamInfo_P1.pHalImage_P1_Rrzo == nullptr)
            {
                bMultiCam_CamSvPath = true;
            }
        }
    }

    for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node) {
        bool laggingConfig = false;
        if( pPipelineNodesNeed->vNeedLaggingConfig.count(_sensorId) ) {
            laggingConfig = pPipelineNodesNeed->vNeedLaggingConfig.at(_sensorId);
        }

        if (_needP1) {
            CHECK_ERROR( configContextLocked_P1Node(
                            pContext,
                            pOldPipelineContext,
                            pStreamingFeatureSetting,
                            pSeamlessSwitchFeatureSetting,
                            pPipelineNodesNeed,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            pSensorSetting,
                            pvP1HwSetting,
                            _sensorId,
                            pBatchSize,
                            useP1NodeCount > 1,
                            bMultiCam_CamSvPath,
                            bIsZslEnabled,
                            pCommon,
                            isReConfig,
                            bIsP1OnlyOutImgo,
                            laggingConfig,
                            true,
                            false,
                            0,
                            outParam),
                      "\nconfigContextLocked_P1Node");
        }
    }
    if( pPipelineNodesNeed->needP2StreamNode ) {
        bool hasMonoSensor = false;
        for(auto const v : pPipelineStaticInfo->sensorRawType) {
            if(SENSOR_RAW_MONO == v) {
                hasMonoSensor = true;
                break;
            }
        }
        CHECK_ERROR( configContextLocked_P2SNode(
                            pContext,
                            pStreamingFeatureSetting,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            pBatchSize,
                            useP1NodeCount,
                            hasMonoSensor,
                            pCommon),
                      "\nconfigContextLocked_P2SNode");
    }
    if( pPipelineNodesNeed->needP2CaptureNode ) {
        CHECK_ERROR( configContextLocked_P2CNode(
                            pContext,
                            pCaptureFeatureSetting,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pPipelineNodesNeed,
                            bIsZslEnabled,
                            bIsSW4Cell,
                            pCommon),
                      "\nconfigContextLocked_P2CNode");
    }
    if( pPipelineNodesNeed->needFDNode ) {
        // useP1DirectFDYUV
        if (pCommon->pPipelineUserConfiguration->pParsedAppConfiguration->useP1DirectFDYUV) {
            for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node) {
                if (_needP1) {
                    CHECK_ERROR( configContextLocked_FdNode(
                                        pContext,
                                        pStreamingFeatureSetting,
                                        pParsedStreamInfo_P1,
                                        pParsedStreamInfo_NonP1,
                                        useP1NodeCount,
                                        pCommon,
                                        _sensorId),
                                  "\nconfigContextLocked_FdNode");
                }
            }
        }
        else {
            CHECK_ERROR( configContextLocked_FdNode(
                                pContext,
                                pStreamingFeatureSetting,
                                pParsedStreamInfo_P1,
                                pParsedStreamInfo_NonP1,
                                useP1NodeCount,
                                pCommon,
                                pPipelineStaticInfo->sensorId[0]),
                          "\nconfigContextLocked_FdNode");
        }
    }
    if( pPipelineNodesNeed->needJpegNode ) {
        CHECK_ERROR( configContextLocked_JpegNode(
                            pContext,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                      "\nconfigContextLocked_JpegNode");
    }
    if( pPipelineNodesNeed->needRaw16Node ) {
        CHECK_ERROR( configContextLocked_Raw16Node(
                            pContext,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                      "\nconfigContextLocked_Raw16Node");

        CHECK_ERROR( configContextLocked_Raw16Node_Physical(
                            pContext,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                      "\nconfigContextLocked_Raw16Node_Physical");
    }
    if( pPipelineNodesNeed->needPDENode ) {
        CHECK_ERROR( configContextLocked_PDENode(
                            pContext,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                     "\nconfigContextLocked_PDENode");
    }
    if( pPipelineNodesNeed->needP1RawIspTuningDataPackNode ) {
        CHECK_ERROR( configContextLocked_P1RawIspTuningDataPackNode(
                            pContext,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                      "\nconfigContextLocked_P1RawIspTuningDataPackNode");
    }
    if( pPipelineNodesNeed->needP2YuvIspTuningDataPackNode ) {
        CHECK_ERROR( configContextLocked_P2YuvIspTuningDataPackNode(
                            pContext,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                      "\nconfigContextLocked_P2YuvIspTuningDataPackNode");
    }
    if( pPipelineNodesNeed->needP2RawIspTuningDataPackNode ) {
        CHECK_ERROR( configContextLocked_P2RawIspTuningDataPackNode(
                            pContext,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            useP1NodeCount,
                            pCommon),
                      "\nconfigContextLocked_P2RawIspTuningDataPackNode");
    }
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
configContextLocked_Pipeline(
    android::sp<IPipelineContext> pContext,
    PipelineTopology const* pPipelineTopology
)
{
    CAM_TRACE_CALL();
    //
    CHECK_ERROR(
            PipelineBuilder()
            .setRootNode(pPipelineTopology->roots)
            .setNodeEdges(pPipelineTopology->edges)
            .build(pContext),
            "\nPipelineBuilder.build(pContext)");
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace model {
auto buildPipelineContext(
    android::sp<IPipelineContext>& out,
    BuildPipelineContextInputParams const& in,
    BuildPipelineContextOutputParams *outParam
) -> int
{
    if ( in.pOldPipelineContext.get() ) {
        MY_LOGD("old PipelineContext - getStrongCount:%d", in.pOldPipelineContext->getStrongCount());
        CHECK_ERROR(in.pOldPipelineContext->waitUntilNodeDrained(
                        0x01),
                    "\npipelineContext->waitUntilNodeDrained");
    }

    //
    auto pNewPipelineContext = IPipelineContextManager::get()->create(in.pipelineName.c_str());
    auto pPipelineContextBuilder = IPipelineContextBuilder::make();
    MY_LOGF_IF(pPipelineContextBuilder==nullptr, "Bad IPipelineContextBuilder");

    RETURN_ERROR_IF_FALSE(
        pPipelineContextBuilder->setPipelineContext(pNewPipelineContext),
        UNKNOWN_ERROR, "IPipelineContextBuilder::setPipelineContext");

    InternalCommonInputParams const common{
        .pPipelineContextBuilder    = pPipelineContextBuilder.get(),
        .pPipelineStaticInfo        = in.pPipelineStaticInfo,
        .pPipelineUserConfiguration = in.pPipelineUserConfiguration,
        .pPipelineUserConfiguration2= in.pPipelineUserConfiguration2,
        .pP1NodeResourceConcurrency = in.pP1NodeResourceConcurrency,
        .bIsReConfigure             = in.bIsReconfigure,
        .bEnableCaptureFlow         = in.bEnableCaptureFlow,
        .bEnableQuadCode            = in.bEnableQuadCode,
    };

    CHECK_ERROR(pNewPipelineContext->beginConfigure(
                    in.pOldPipelineContext),
                "\npipelineContext->beginConfigure");

    // config Streams
    CHECK_ERROR(configContextLocked_Streams(
                    in.pParsedStreamInfo_P1,
                    in.pStreamingFeatureSetting,
                    in.bIsZslEnabled,
                    in.pParsedStreamInfo_NonP1,
                    in.pPoolSharedStreams,
                    &common),
                "\nconfigContextLocked_Streams");

    // config Nodes
    CHECK_ERROR(configContextLocked_Nodes(
                    pNewPipelineContext,
                    in.pOldPipelineContext,
                    in.pStreamingFeatureSetting,
                    in.pCaptureFeatureSetting,
                    in.pSeamlessSwitchFeatureSetting,
                    in.pParsedStreamInfo_P1,
                    in.pParsedStreamInfo_NonP1,
                    in.pPipelineNodesNeed,
                    in.pSensorSetting,
                    in.pvP1HwSetting,
                    in.pBatchSize,
                    in.bIsZslEnabled,
                    in.bIsSW4Cell,
                    in.bIsP1OnlyOutImgo,
                    &common,
                    outParam),
                "\nconfigContextLocked_Nodes");

    // config pipeline
    CHECK_ERROR(configContextLocked_Pipeline(
                    pNewPipelineContext,
                    in.pPipelineTopology),
                "\npipelineContext->configContextLocked_Pipeline");
    //
    {
    CAM_TRACE_NAME("setDataCallback");
    CHECK_ERROR(pNewPipelineContext->setDataCallback(
                    in.pDataCallback),
                "\npipelineContext->setDataCallback");
    }
    //
    {
        static constexpr char key1[] = "vendor.debug.camera.pipelinecontext.build.multithreadcfg";
        static constexpr char key2[] = "vendor.debug.camera.pipelinecontext.build.parallelnode";
        int32_t bUsingMultiThreadToBuildPipelineContext = ::property_get_int32(key1, -1);;
        int32_t bUsingparallelNodeToBuildPipelineContext = ::property_get_int32(key2, -1);
        if (CC_UNLIKELY(-1 != bUsingparallelNodeToBuildPipelineContext)) {
            MY_LOGD("%s=%d", key2, bUsingparallelNodeToBuildPipelineContext);
        }
        else {
            bUsingparallelNodeToBuildPipelineContext = in.bUsingParallelNodeToBuildPipelineContext;
        }
        //
        if (CC_UNLIKELY(-1 != bUsingMultiThreadToBuildPipelineContext)) {
            MY_LOGD("%s=%d", key1, bUsingMultiThreadToBuildPipelineContext);
        }
        else {
            bUsingMultiThreadToBuildPipelineContext = in.bUsingMultiThreadToBuildPipelineContext;
        }

        RETURN_IF_ERROR(pNewPipelineContext->endConfigure(
                        bUsingparallelNodeToBuildPipelineContext, bUsingMultiThreadToBuildPipelineContext),
                    "\npipelineContext->endConfigure");
    }

    out = pNewPipelineContext;
    return OK;
}


/******************************************************************************
 *
 ******************************************************************************/
auto reconfigureP1ForPipelineContext(
    android::sp<IPipelineContext> pContext,
    BuildPipelineContextInputParams const& in,
    BuildPipelineContextOutputParams *outParam
) -> int
{
    InternalCommonInputParams const common{
        .pPipelineContextBuilder    = nullptr,
        .pPipelineStaticInfo        = in.pPipelineStaticInfo,
        .pPipelineUserConfiguration = in.pPipelineUserConfiguration,
        .pPipelineUserConfiguration2= in.pPipelineUserConfiguration2,
        .pP1NodeResourceConcurrency = in.pP1NodeResourceConcurrency,
        .bIsReConfigure             = in.bIsReconfigure,
    };


    android::sp<IPipelineContext> const& pOldPipelineContext    = in.pOldPipelineContext;
    StreamingFeatureSetting const* pStreamingFeatureSetting     = in.pStreamingFeatureSetting;
    SeamlessSwitchFeatureSetting const* pSeamlessSwitchFeatureSetting
                                                                = in.pSeamlessSwitchFeatureSetting;
    SensorMap<ParsedStreamInfo_P1> const* pParsedStreamInfo_P1  = in.pParsedStreamInfo_P1;
    ParsedStreamInfo_NonP1 const* pParsedStreamInfo_NonP1       = in.pParsedStreamInfo_NonP1;
    PipelineNodesNeed const* pPipelineNodesNeed                 = in.pPipelineNodesNeed;
    SensorMap<SensorSetting> const* pSensorSetting              = in.pSensorSetting;
    SensorMap<P1HwSetting> const* pvP1HwSetting                 = in.pvP1HwSetting;
    SensorMap<uint32_t> const* pBatchSize                       = in.pBatchSize;
    bool bMultiCam_CamSvPath                                    = false;
    InternalCommonInputParams const* pCommon                    = &common;
    bool isReConfig                                             = true;
    bool LaggingLaunch                                          = false;
    bool isConfigureSensor                                      = false;
    bool isSensorSwitch                                         = in.bIsSwitchSensor;
    int32_t  initRequest                                        = in.initRequest;

    for (const auto& [_sensorId, _needP1] : pPipelineNodesNeed->needP1Node) {
        if (_needP1) {
            CHECK_ERROR( configContextLocked_P1Node(
                            pContext,
                            pOldPipelineContext,
                            pStreamingFeatureSetting,
                            pSeamlessSwitchFeatureSetting,
                            pPipelineNodesNeed,
                            pParsedStreamInfo_P1,
                            pParsedStreamInfo_NonP1,
                            pSensorSetting,
                            pvP1HwSetting,
                            _sensorId,
                            pBatchSize,
                            pPipelineNodesNeed->needP1Node.size() > 1,
                            bMultiCam_CamSvPath,
                            in.bIsZslEnabled,
                            pCommon,
                            isReConfig,
                            in.bIsP1OnlyOutImgo,
                            LaggingLaunch,
                            isConfigureSensor,
                            isSensorSwitch,
                            initRequest,
                            outParam),
                      "\nconfigContextLocked_P1Node");
        }
    }

    return OK;
}
/******************************************************************************
 *
 ******************************************************************************/
auto preconfigureP1ForPipelineContext(
    android::sp<IPipelineContext> pContext,
    PreConfigInputParams const& in
) -> int
{
    auto pNodeActor = pContext->queryINodeActor(in.nodeId);
    {
        if(pNodeActor == nullptr)
        {
            MY_LOGW("get NodeActor(%" PRIdPTR ") fail", in.nodeId);
            goto lbExit;
        }
        IPipelineNode* node = pNodeActor->getNode();
        if( node == nullptr )
        {
            MY_LOGW("get node(%" PRIdPTR ") fail", in.nodeId);
            goto lbExit;
        }
        node->preConfig(NSCam::v3::P1Node::RAW_DEF_TYPE_PROCESSED_RAW,
                        reinterpret_cast<void*>(const_cast<bool*>(&in.bEnablePreQueue)));
    }
lbExit:
    return OK;

}
};  //namespace model
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
