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

#include "PipelineModelImpl_SingleHW.h"
#include "MyUtils.h"

#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
//
#include <mtkcam3/pipeline/hwnode/StreamId.h>

#include <mtkcam3/pipeline/utils/streaminfo/ImageStreamInfo.h>
#include <mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>


/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::isppipeline::model;
using namespace NSCam::v3::pipeline;

//using namespace NSCamHW;


/******************************************************************************
 *
 ******************************************************************************/

CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_PIPELINE_MODEL);
#define MY_ERROR(level, fmt, arg...) \
    do { \
        CAM_LOG##level("[%u:%s] " fmt, mModel, __FUNCTION__, ##arg); \
        mErrorPrinter->printFormatLine(#level" [%u:%s] " fmt, mModel, __FUNCTION__, ##arg); \
    } while(0)

#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGE("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#define MY_LOGD1(...)               MY_LOGD_IF(           (mLogLevel>=1), __VA_ARGS__)
#define MY_LOGD2(...)               MY_LOGD_IF(CC_UNLIKELY(mLogLevel>=2), __VA_ARGS__)
#define MY_LOGD3(...)               MY_LOGD_IF(CC_UNLIKELY(mLogLevel>=3), __VA_ARGS__)

#if 1
#define FUNC_START MY_LOGD("%s +", __FUNCTION__);
#define FUNC_END MY_LOGD("%s -", __FUNCTION__);
#else
#define FUNC_START
#define FUNC_END
#endif

/******************************************************************************
 *
 ******************************************************************************/
PipelineModelSingleHW::
PipelineModelSingleHW()
    : mLogLevel(0)
    , mModelName()

{
    mLogLevel = ::property_get_int32("vendor.debug.camera.log", 0);
    if ( mLogLevel == 0 ) {
        mLogLevel = ::property_get_int32("vendor.debug.camera.log.pipelinemodel.hw", 0);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
open(
    std::string const& userName,
    android::wp<IISPPipelineModelCallback> const& callback
) -> int
{
    MY_LOGD("+");
    //
    {
        std::lock_guard<std::timed_mutex> _l(mLock);

        mModelName = userName;
        mCallback = callback;
    }
    //
    MY_LOGD("-");
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
configure(
    std::shared_ptr<UserConfigurationParams>const& params
) -> int
{
    int err = OK;
    MY_LOGD("+");
    {
        std::lock_guard<std::timed_mutex> _l(mLock);

        mpHandler = HWHandlerFactory::createHWHandler(params->HWModule);
        if (mpHandler == nullptr)
        {
            err = -1;
            MY_LOGE("create hw handler failed");
            return err;
        }
        mConfigureInfo.streamInfo = params->vImageStreams;
        auto stream = params->vMetaStreams.find(eSTREAMID_META_APP_CONTROL);
        if (stream != params->vMetaStreams.end())
        {
            mConfigureInfo.pAppMeta_Control = stream->second;
        }
        err = mpHandler->init(params->sessionParams, mConfigureInfo.streamInfo);
    }
    MY_LOGD("- err:%d", err);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
close() -> void
{
    MY_LOGI("+");
    {
        std::lock_guard<std::timed_mutex> _l(mLock);

        mpHandler->uninit();
        mpHandler = nullptr;

        mModelName.clear();
    }
    MY_LOGI("-");
}


/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
submitRequest(
    std::vector<std::shared_ptr<UserRequestParams>>const& requests,
    uint32_t& numRequestProcessed __unused
) -> int
{
    int err = OK;

    MY_LOGI("+");
    {
        std::lock_guard<std::timed_mutex> _l(mLock);
        if ( CC_UNLIKELY( mpHandler==nullptr ) ) {
            MY_LOGE("null Context");
            err = DEAD_OBJECT;
        }
        else {
            shared_ptr<RequestData> reqData = nullptr;
            struct RequestParams param;
            IMetadata *pAppMetaControl = nullptr;
            param.pAppCallback = this;
            param.pDataCallback = this;
            for (auto const& req : requests)
            {
                auto reqNo = static_cast<int32_t>(req->requestNo);
                CAM_TRACE_ASYNC_BEGIN("ISP HIDL capture", reqNo);
                param.requestNo = req->requestNo;
                auto buf = req->vIMetaBuffers.find(eSTREAMID_META_APP_CONTROL);
                if (buf == req->vIMetaBuffers.end())
                {
                    MY_LOGE("cannot find eSTREAMID_META_APP_CONTROL");
                    return -EINVAL;
                }
                pAppMetaControl = buf->second->tryReadLock(LOG_TAG);
                param.pAppControl = pAppMetaControl;
                param.inBufs = req->vIImageBuffers;
                param.outBufs = req->vOImageBuffers;
                reqData = mpHandler->createReqData(param);
                buf->second->unlock(LOG_TAG, pAppMetaControl);
                err = mpHandler->enqueReq(reqData);
            }
        }
    }

    MY_LOGI("- err:%d", err);
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
beginFlush() -> int
{
    int err = OK;
    MY_LOGD1("+");
    MY_LOGD1("-");
    return err;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
endFlush() -> void
{
    MY_LOGD1("+");
    MY_LOGD1("-");
}

/******************************************************************************
 *
 ******************************************************************************/
auto
PipelineModelSingleHW::
queryFeature(IMetadata const* pInMeta, std::vector<std::shared_ptr<IMetadata>> &outSetting) -> void
{
    MY_LOGD1("+");
    MY_LOGD1("-");
}


/******************************************************************************
 *
 ******************************************************************************/
MVOID
PipelineModelSingleHW::
updateFrame(
    MUINT32 const requestNo,
    MINTPTR const userId,
    Result const& result
)
{
    sp<IISPPipelineModelCallback> pCallback;
    pCallback = mCallback.promote();
    if ( CC_UNLIKELY(! pCallback.get()) ) {
        MY_LOGE("Have not set callback to ISP device");
        return;
    }

    {
        UserOnFrameUpdated params;
        params.requestNo = requestNo;
        params.userId = userId;
        params.nOutMetaLeft = 0;
        params.timestampStartOfFrame = 0;
        params.vOutMeta.clear();

        params.bFrameEnd = result.bFrameEnd;
        params.bResultError = result.bResultError;
        pCallback->onFrameUpdated(params);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
MVOID
PipelineModelSingleHW::
onMetaCallback(
    MUINT32              requestNo,
    Pipeline_NodeId_T    nodeId,
    StreamId_T           streamId __unused,
    IMetadata const&     rMetaData,
    MBOOL                errorResult __unused
)
{
    sp<IISPPipelineModelCallback> pCallback;
    pCallback = mCallback.promote();
    if ( CC_UNLIKELY(! pCallback.get()) ) {
        MY_LOGE("Have not set callback to ISP device");
        return;
    }

    {
        UserOnFrameUpdated params;
        params.requestNo = requestNo;
        params.userId = nodeId;
        params.bFrameEnd = false;
        params.bisEarlyCB = true;
        params.pMetadata = &rMetaData;
        pCallback->onFrameUpdated(params);
    }
}

