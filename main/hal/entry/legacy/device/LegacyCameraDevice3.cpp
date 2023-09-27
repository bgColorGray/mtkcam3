/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include "LegacyCameraDevice3.h"
#include "MyUtils.h"
#include <cutils/properties.h>
#include <cutils/native_handle.h>
#include <mtkcam/utils/std/ULog.h> // will include <log/log.h> if LOG_TAG was defined
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/DebugTimer.h>
#include <mtkcam/utils/debug/debug.h>
#include <errno.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
//
using android::status_t;
using android::OK;
using android::BAD_VALUE;
using android::FdPrinter;
using namespace NSCam;
using namespace NSCam::legacy_dev3;
using namespace NSCam::v3;
using namespace NSCam::Utils;
using namespace NSCam::Utils::ULog;

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
static
inline
LegacyCameraDevice3*
getDevice(camera3_device const* device)
{
    return  LegacyCameraDevice3::getDevice(device);
}


////////////////////////////////////////////////////////////////////////////////
//  Implementation of hw_device_t
////////////////////////////////////////////////////////////////////////////////
static
int
camera_close_device(hw_device_t* device)
{
    if ( cc_unlikely(device==nullptr) )
    {
        return  -EINVAL;
    }
    return  LegacyCameraDevice3::getDevice(device)->i_closeDevice();
}


/******************************************************************************
 *
 ******************************************************************************/
static
hw_device_t const
gHwDevice =
{
    /** tag must be initialized to HARDWARE_DEVICE_TAG */
    .tag        = HARDWARE_DEVICE_TAG,
    /** version number for hw_device_t */
    .version    = CAMERA_DEVICE_API_VERSION_3_5,
    /** reference to the module this device belongs to */
    .module     = NULL,
    /** padding reserved for future use */
    .reserved   = {0},
    /** Close this device */
    .close      = camera_close_device,
};


////////////////////////////////////////////////////////////////////////////////
//  Implementation of camera3_device_ops
////////////////////////////////////////////////////////////////////////////////
static
int
camera_initialize(
    camera3_device const*           device,
    camera3_callback_ops_t const*   callback_ops
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return -ENODEV;
    }

    return  pDev->i_initialize(callback_ops);
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
camera_configure_streams(
    camera3_device const*           device,
    camera3_stream_configuration_t* stream_list
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return -ENODEV;
    }

    return  pDev->i_configure_streams(stream_list);
}


/******************************************************************************
 *
 ******************************************************************************/
static
camera_metadata_t const*
camera_construct_default_request_settings(
    camera3_device const*   device,
    int                     type
)
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    CAM_TRACE_CALL();

    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return nullptr;
    }

    return  pDev->i_construct_default_request_settings(type);
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
camera_process_capture_request(
    camera3_device const*       device,
    camera3_capture_request_t*  request
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return -ENODEV;
    }

    return  pDev->i_process_capture_request(request);
}


/******************************************************************************
 *
 ******************************************************************************/
static
void
camera_dump(
    camera3_device const*   device,
    int                     fd
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return;
    }

    pDev->i_dump(fd);
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
camera_flush(
    camera3_device const*   device
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return -ENODEV;
    }

    return  pDev->i_flush();
}


/******************************************************************************
 *
 ******************************************************************************/
static
void
camera_signal_stream_flush(
    camera3_device const*   device,
    uint32_t num_streams,
    const camera3_stream_t* const* streams
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return;
    }

    pDev->i_signal_stream_flush(num_streams, streams);
}


/******************************************************************************
 *
 ******************************************************************************/
static
int
camera_is_reconfiguration_required(
    camera3_device const*   device,
    const camera_metadata_t* old_session_params,
    const camera_metadata_t* new_session_params
)
{
    LegacyCameraDevice3*const pDev = getDevice(device);
    if ( cc_unlikely(pDev==nullptr) )
    {
        MY_LOGE("invalid legacy camera device3");
        return -ENODEV;
    }

    return  pDev->i_is_reconfiguration_required(old_session_params, new_session_params);
}


/******************************************************************************
 *
 ******************************************************************************/
static camera3_device_ops const gCameraDevOps = {
    #define OPS(name) .name= camera_##name
    OPS(initialize),
    OPS(configure_streams),
    .register_stream_buffers        = NULL,
    OPS(construct_default_request_settings),
    OPS(process_capture_request),
    .get_metadata_vendor_tag_ops    = NULL,
    OPS(dump),
    OPS(flush),
    OPS(signal_stream_flush),
    OPS(is_reconfiguration_required),
    #undef  OPS
    .reserved = {0},
};


/******************************************************************************
 *
 ******************************************************************************/
LegacyCameraDevice3::
~LegacyCameraDevice3()
{
    MY_LOGD("dtor");
}


/******************************************************************************
 *
 ******************************************************************************/
LegacyCameraDevice3::
LegacyCameraDevice3(int32_t deviceId, android::sp<ICameraDevice3Session> session)
    : ILegacyHalDevice()
    , mpModuleCallbacks(NULL)
    , mDevice()
    , mDeviceId(deviceId)
    , mDeviceOps()
    , mSession(session)
{
    MY_LOGD("ctor");
    mConvertTimeDebug = property_get_int32("vendor.debug.camera.log.convertTime", 0);
    ::memset(&mDevice, 0, sizeof(mDevice));
    mDevice.priv    = this;
    mDevice.common  = gHwDevice;
    mDevice.ops     = &mDeviceOps;
    mDeviceOps      = gCameraDevOps;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
open(
    camera_module_callbacks_t const* callbacks
) -> int
{
    CAM_TRACE_CALL();
    mpModuleCallbacks = callbacks;

    auto status = mSession->open(createSessionCallbacks());
    if ( CC_UNLIKELY(status!=OK) ) {
        MY_LOGE("LegacyCameraDevice3 initialization fail");
    }
    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_closeDevice() -> int
{
    CAM_TRACE_CALL();
    return i_uninitialize();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_initialize(camera3_callback_ops const* callback_ops) -> int
{
    CAM_TRACE_CALL();
    mpDeviceCallbacks = callback_ops;
    return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_uninitialize() -> int
{
    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return -EINVAL;
    }
    return mSession->close();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_configure_streams(camera3_stream_configuration_t* stream_list) -> int
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    CAM_TRACE_CALL();

    // a map to keep camera3_stream
    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return -EINVAL;
    }

    v3::StreamConfiguration inStreams;
    v3::HalStreamConfiguration outStreams;
    //
    auto ret = convertToHalStreamConfiguration(stream_list, &inStreams);
    MY_LOGE_IF(ret != android::OK, "convertToHalStreamConfiguration failed(%d)", ret);;
    //
    int status = mSession->configureStreams(inStreams, outStreams);
    //
    ret = convertToLegacyStreamConfiguration(outStreams, stream_list);
    MY_LOGE_IF(ret != android::OK, "convertToLegacyStreamConfiguration failed(%d)", ret);

    return status;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_construct_default_request_settings(int type) -> camera_metadata const*
{
    // CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE);
    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return nullptr;
    }
    return mSession->constructDefaultRequestSettings(static_cast<v3::RequestTemplate>(type));
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_process_capture_request(camera3_capture_request_t* request) -> int
{
    CAM_ULOG_APILIFE_GUARD(MOD_CAMERA_DEVICE, 30000);
    CAM_TRACE_CALL();
    // wrap structure from camera3_stream map to Stream
    NSCam::v3::CaptureRequest hal_request;

    auto ret = convertToHalCaptureRequest(request, &hal_request);
    MY_LOGE_IF(ret != android::OK, "convertToHalCaptureRequest failed(%d)", ret);;

    std::vector<v3::CaptureRequest> hal_requests;
    hal_requests.push_back(hal_request);
    uint32_t numRequests = 1;

    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return -EINVAL;
    }
    return mSession->processCaptureRequest(hal_requests, numRequests);
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_flush() -> int
{
    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return -EINVAL;
    }
    return mSession->flush();
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_dump(int fd) -> void
{
    FdPrinter printer(fd);
    std::vector<std::string> options;
    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return;
    }
    mSession->dumpState(printer, options);
    return;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_signal_stream_flush(
    uint32_t num_streams,
    const camera3_stream_t* const* streams
) -> void
{
    CAM_TRACE_CALL();

    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return;
    }

    std::vector<int32_t> streamIds;
    streamIds.resize(num_streams);

    ssize_t map_size = mStreamMap.size();

    for (ssize_t i = 0; i < num_streams; i++) {
        const camera3_stream_t* const stream = streams[i];
        for (std::map<int, camera3_stream_t*>::iterator it = mStreamMap.begin(); it != mStreamMap.end(); ++it) {
            if (stream == it->second) {
                streamIds.push_back(it->first);
                break;
            }
        }
    }

    // since camera device have checked with streamCounter,
    // we just make the params greater than mStreamCounter = 1
    uint32_t streamCounter = 2;
    mSession->signalStreamFlush(streamIds, streamCounter);
    return;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
i_is_reconfiguration_required(
    const camera_metadata_t* old_session_params,
    const camera_metadata_t* new_session_params
) -> int
{
    CAM_TRACE_CALL();

    if ( mSession==nullptr ) {
        MY_LOGE("mSession nullptr");
        return -EINVAL;
    }

    bool needReconfig = true;
    auto ret = mSession->isReconfigurationRequired(*old_session_params, *new_session_params, needReconfig);
    MY_LOGE_IF(ret != android::OK, "is_reconfiguration_required failed(%d)", ret);;

    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
createSessionCallbacks()
-> std::shared_ptr<v3::CameraDevice3SessionCallback>
{
    CAM_TRACE_CALL();
    auto sessionCallbacks = std::make_shared<v3::CameraDevice3SessionCallback>();
    {
        sessionCallbacks->processCaptureResult = v3::ProcessCaptureResultFunc(
            [this](std::vector<v3::CaptureResult>& results) {
                /*mCameraDeviceCallback->*/processCaptureResult(results);
            }
        );
        sessionCallbacks->notify = v3::NotifyFunc(
            [this](const std::vector<v3::NotifyMsg>& msgs) {
                /*mCameraDeviceCallback->*/notify(msgs);
            }
        );
        sessionCallbacks->requestStreamBuffers = v3::RequestStreamBuffersFunc(
            [this](const std::vector<v3::BufferRequest>& vBufferRequests,
                    std::vector<v3::StreamBufferRet>* pvBufferReturns) {
                return /*mCameraDeviceCallback->*/requestStreamBuffers(vBufferRequests, pvBufferReturns);
            }
        );
        sessionCallbacks->returnStreamBuffers = v3::ReturnStreamBuffersFunc(
            [this](const std::vector<v3::StreamBuffer>& buffers) {
                /*mCameraDeviceCallback->*/returnStreamBuffers(buffers);
            }
        );
        return sessionCallbacks;
    }
    MY_LOGD("fail to create CameraDevice3SessionCallback");
    return nullptr;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
processCaptureResult(std::vector<v3::CaptureResult>& results)
-> void
{
    CAM_TRACE_CALL();
    auto convert = [=](auto const& srcBuf, auto& dstBuf){
            // camera3Stream dstStream;
            MY_LOGD("processCaptureResult streamId= %d", srcBuf.streamId);

            camera3_stream_t *srcStream = mStreamMap[srcBuf.streamId];
            buffer_handle_t tmphdl = srcBuf.buffer;
            dstBuf.stream = srcStream;
            dstBuf.buffer = &tmphdl;
            dstBuf.status = static_cast<int>(srcBuf.status);
            /*dstBuf.acquire_fence = srcBuf.acquireFence->data[0];
            dstBuf.release_fence = srcBuf.releaseFence->data[0];*/
            dstBuf.acquire_fence = -1;
            dstBuf.release_fence = srcBuf.releaseFenceOp.fd;
    };

    for ( auto& result : results ) {
        MY_LOGD("frameNum[%u]", result.frameNumber);
        // 1. for meta
        if (result.result) {
            MY_LOGD("processCaptureResult meta [%d]", result.frameNumber);

            camera3_capture_result_t capture_result;
            uint32_t num_physcam_size = result.physicalCameraMetadata.size();
            capture_result.frame_number = result.frameNumber;

            capture_result.result = result.result;
            capture_result.num_output_buffers = 0;
            capture_result.output_buffers = nullptr;
            capture_result.input_buffer = nullptr;
            capture_result.partial_result = result.partialResult;
            capture_result.num_physcam_metadata = num_physcam_size;

            if (num_physcam_size > 0) {
                capture_result.physcam_ids = new const char *[num_physcam_size];
                capture_result.physcam_metadata = new const camera_metadata_t *[num_physcam_size];

                for (size_t i = 0; i < result.physicalCameraMetadata.size(); i++) {
                       auto const& pMetadata = result.physicalCameraMetadata[i];

                       capture_result.physcam_ids[i] = pMetadata.physicalCameraId.c_str();
                       capture_result.physcam_metadata[i] = pMetadata.metadata;
                }
            } else {
                capture_result.physcam_ids = NULL;
                capture_result.physcam_metadata = NULL;
            }

            mpDeviceCallbacks->process_capture_result(mpDeviceCallbacks, &capture_result);

        } else {
        // 2. for image
            MY_LOGD("processCaptureResult image [%d]", result.frameNumber);

            camera3_capture_result_t buffer_result;
            buffer_result.frame_number = result.frameNumber;
            buffer_result.result = nullptr;
            buffer_result.num_output_buffers = result.outputBuffers.size();

            std::vector<camera3_stream_buffer_t> vOutBuf;
            vOutBuf.clear();
            vOutBuf.resize(buffer_result.num_output_buffers);

            for (int i = 0; i < result.outputBuffers.size(); i++) {
                NSCam::v3::StreamBuffer halOutBuf = result.outputBuffers[i];
                convert(halOutBuf, vOutBuf.at(i));
            }
            buffer_result.output_buffers = vOutBuf.data();

            if (result.inputBuffer.streamId == -1) {
                buffer_result.input_buffer = nullptr;
            } else {
                camera3_stream_buffer_t inbuf;
                NSCam::v3::StreamBuffer halInBuf = result.inputBuffer;
                convert(halInBuf, inbuf);
                buffer_result.input_buffer = &inbuf;
            }

            buffer_result.partial_result = 0;
            buffer_result.num_physcam_metadata = 0;
            buffer_result.physcam_ids = nullptr;
            buffer_result.physcam_metadata = nullptr;

            mpDeviceCallbacks->process_capture_result(mpDeviceCallbacks, &buffer_result);
        }
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
notify(const std::vector<v3::NotifyMsg>& msgs)
-> void
{
    CAM_TRACE_CALL();
    for ( auto& hal_msg : msgs ) {
        camera3_notify_msg_t legacy_msg;
        switch (hal_msg.type) {
            case v3::MsgType::ERROR:
            {
                legacy_msg.type = CAMERA3_MSG_ERROR;
                auto res_error = convertToLegacyErrorMessage(
                                            hal_msg.msg.error,
                                            &legacy_msg.message.error);
                if (res_error != OK) {
                    MY_LOGE("Converting to Legacy error message failed: %s(%d)",
                            strerror(-res_error), res_error);
                    return;
                }
                break;
            }
            case v3::MsgType::SHUTTER:
            {
                legacy_msg.type = CAMERA3_MSG_SHUTTER;
                auto res_shutter = convertToLegacyShutterMessage(
                                                hal_msg.msg.shutter,
                                                &legacy_msg.message.shutter);
                if (res_shutter != OK) {
                    MY_LOGE("Converting to Legacy shutter message failed: %s(%d)",
                            strerror(-res_shutter), res_shutter);
                    return;
                }
                break;
            }
            default:
            {
                MY_LOGE("Unknown message type: %u", hal_msg.type);
                return;
            }
        }
        mpDeviceCallbacks->notify(mpDeviceCallbacks, &legacy_msg);
    }
}


/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
requestStreamBuffers(
    const std::vector<v3::BufferRequest>& vBufferRequests,
    std::vector<v3::StreamBufferRet>* pvBufferReturns
) -> v3::BufferRequestStatus
{
    CAM_TRACE_CALL();
    camera3_buffer_request_status_t status = CAMERA3_BUF_REQ_OK;
    camera3_buffer_request_t* legacy_buffer_reqs;
    uint32_t *num_returned_buf_reqs = 0;
    camera3_stream_buffer_ret_t* returned_buf_reqs;

    status = mpDeviceCallbacks->request_stream_buffers(
        mpDeviceCallbacks,
        vBufferRequests.size(),
        legacy_buffer_reqs,
        /*out*/num_returned_buf_reqs,
        /*out*/returned_buf_reqs
    );

    return static_cast<v3::BufferRequestStatus>(status);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
returnStreamBuffers(const std::vector<v3::StreamBuffer>& buffers)
-> void
{
    CAM_TRACE_CALL();
    camera3_stream_buffer_t** legacy_buffers;
    mpDeviceCallbacks->return_stream_buffers(
        mpDeviceCallbacks,
        buffers.size(),
        legacy_buffers
    );
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
convertToHalStreamConfiguration(
    const camera3_stream_configuration_t *srcStreams,
    NSCam::v3::StreamConfiguration *dstStreams)
-> ::android::status_t {

    if (srcStreams == nullptr) {
        MY_LOGE("ConvertToHalStreamConfiguration srcStream Null");
        return BAD_VALUE;
    }

    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    int size = srcStreams->num_streams;
    dstStreams->streams.resize(size);

    for (int i = 0; i < size; i++) {
        camera3_stream_t *srcStream = srcStreams->streams[i];
        NSCam::v3::Stream *dstStream = &(dstStreams->streams[i]);
        //
#warning "we use addr to represent id"
        // ref as below main/hal/legacy_hal/app/AppStreamMgr.ConfigHandler.cpp
        // StreamId_T streamId = mStreamIdToConfig++;
        // dstStream.id = reinterpret_cast<int32_t>(srcStream);
        dstStream->id = static_cast<camera3Stream*>(srcStream)->mId;
        dstStream->streamType = static_cast<v3::StreamType> (srcStream->stream_type);
        dstStream->width = srcStream->width;
        dstStream->height = srcStream->height;
        dstStream->format = static_cast<android_pixel_format_t> (srcStream->format);
        if (static_cast<MINT32>(srcStream->format) ==  HAL_PIXEL_FORMAT_BLOB) {
            dstStream->bufferSize = getJpegBufferSize(srcStream->width, srcStream->height);
        }
        dstStream->usage = srcStream->usage;
        dstStream->dataSpace = srcStream->data_space;
        dstStream->rotation = static_cast<v3::StreamRotation> (srcStream->rotation);
        //
        dstStream->physicalCameraId = srcStream->physical_camera_id;
        MY_LOGD("Stream info id:%d streamType:%d %dx%d format:0x%x usage:0x%" PRIx64 " dataSpace:0x%x rotation:%d",
                 dstStream->id, dstStream->streamType,
                 dstStream->width, dstStream->height,
                 dstStream->format, dstStream->usage,
                 dstStream->dataSpace, dstStream->rotation);

        mStreamMap[dstStream->id] = static_cast<camera3_stream_t*>(srcStream);
    }

    dstStreams->sessionParams = srcStreams->session_parameters;
    dstStreams->operationMode = static_cast<v3::StreamConfigurationMode> (srcStreams->operation_mode);

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert StreamConfiguration From legacy : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
convertToLegacyStreamConfiguration(
    const NSCam::v3::HalStreamConfiguration& srcStreams,
    camera3_stream_configuration_t* dstStreams)
-> ::android::status_t {

    if (dstStreams == nullptr) {
        MY_LOGE("ConvertToHalStreamConfiguration srcStream Null");
        return BAD_VALUE;
    }

    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;
    timer.start();

    int size = srcStreams.streams.size();

    for (int i = 0; i < size; i++) {
        auto& srcStream = srcStreams.streams[i];
        camera3_stream_t *dstStream = dstStreams->streams[i];
        //
#warning "Config Stream Configuration priv(streamInfo) usage?"
        // ref as below main/hal/legacy_hal/app/AppStreamMgr.ConfigHandler.cpp
        dstStream->usage |= ((static_cast<v3::StreamType>(dstStream->stream_type) == v3::StreamType::OUTPUT) ?
                srcStream.producerUsage : srcStream.consumerUsage);
        dstStream->max_buffers = srcStream.maxBuffers;

        // establish mStreamMap;
        (mStreamMap[srcStream.id])->priv = mStreamMap[srcStream.id];
        (mStreamMap[srcStream.id])->usage = dstStream->usage;
        (mStreamMap[srcStream.id])->max_buffers = dstStream->max_buffers ;
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert StreamConfiguration to legacy : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
convertToHalCaptureRequest(
    camera3_capture_request_t* request,
    NSCam::v3::CaptureRequest* hal_request
) -> ::android::status_t {

    CAM_TRACE_CALL();
    NSCam::Utils::DebugTimer timer;

    auto createNativeHandle = [](int dup_fd) -> native_handle_t* {
        if  ( -1 != dup_fd ) {
            auto handle = ::native_handle_create(/*numFds*/1, /*numInts*/0);
            if  ( CC_LIKELY(handle) ) {
                handle->data[0] = dup_fd;
                return handle;
            }
        }
        return nullptr;
    };

    timer.start();

    hal_request->frameNumber = request->frame_number;
    hal_request->settings = request->settings;

    // 1.inputBuffers
    if (request->input_buffer) {
#warning "we use addr to represent id"
        camera3_stream_buffer inBuf = *(request->input_buffer);
        NSCam::v3::StreamBuffer *dstBuf = &(hal_request->inputBuffer);
        dstBuf->streamId = static_cast<camera3Stream*>(inBuf.stream)->mId;
        // dstBuf->streamId = reinterpret_cast<uint64_t>(inBuf.stream);
        dstBuf->bufferId = reinterpret_cast<uint64_t>(inBuf.buffer);
        dstBuf->buffer = *(inBuf.buffer);
        dstBuf->status = static_cast<v3::BufferStatus>(inBuf.status);
        dstBuf->acquireFenceOp.type = v3::FenceType::FD;
        dstBuf->releaseFenceOp.type = v3::FenceType::FD;
        dstBuf->acquireFenceOp.fd = inBuf.acquire_fence;
        dstBuf->releaseFenceOp.fd = -1;
        // (ignore) hal_request->inputBuffer.appBufferHandleHolder = ;
        // (ignore) hal_request->inputBuffer.freeByOthers = ;
    }

    // 2.outputBuffers
    hal_request->outputBuffers.resize(request->num_output_buffers);
    for ( int i = 0 ; i < request->num_output_buffers; i++) {
#warning "we use addr to represent id"
        const camera3_stream_buffer srcBuf = request->output_buffers[i];
        NSCam::v3::StreamBuffer *dstBuf = &(hal_request->outputBuffers[i]);
        dstBuf->streamId = static_cast<camera3Stream*>(srcBuf.stream)->mId;
        // dstBuf->streamId = reinterpret_cast<uint64_t>(srcBuf.stream);
        dstBuf->bufferId = reinterpret_cast<uint64_t>(srcBuf.buffer);
        dstBuf->buffer = *(srcBuf.buffer);
        dstBuf->status = static_cast<v3::BufferStatus>(srcBuf.status);
        dstBuf->acquireFenceOp.type = v3::FenceType::FD;
        dstBuf->releaseFenceOp.type = v3::FenceType::FD;
        dstBuf->acquireFenceOp.fd = srcBuf.acquire_fence;
        dstBuf->releaseFenceOp.fd = -1;
        // (ignore) hal_request->outputBuffers[i].appBufferHandleHolder = ;
        // (ignore) hal_request->outputBuffers[i].freeByOthers = ;
        MY_LOGD("StreamBuffer id:%d bufferId:%d bufffer:0x%" PRIx64 " status%d acq_fenc:%d rel_fenc:%d",
                 dstBuf->streamId, dstBuf->bufferId,
                 dstBuf->buffer, dstBuf->status,
                 dstBuf->acquireFenceOp.fd, dstBuf->releaseFenceOp.fd);
    }

    // 3.physicalCameraMetadata
    hal_request->physicalCameraSettings.resize(request->num_physcam_settings);
    for ( int i = 0 ; i < request->num_physcam_settings; i++) {
        hal_request->physicalCameraSettings[i].physicalCameraId = request->physcam_id[i];
        hal_request->physicalCameraSettings[i].settings = request->physcam_settings[i];
    }

    timer.stop();
    MY_LOGD_IF(timer.getElapsed() >= 5 | mConvertTimeDebug >=1 , "convert all legacy CaptureRequest to HAL : %d ms(%d us)", timer.getElapsed(), timer.getElapsedU());

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
convertToLegacyErrorMessage(
const v3::ErrorMsg& hal_error,
camera3_error_msg_t* leg_error)
-> ::android::status_t {

    if (leg_error == nullptr) {
        MY_LOGE("leg_error is nullptr.");
        return BAD_VALUE;
    }

    leg_error->frame_number = hal_error.frameNumber;
#warning "error stream"
    // leg_error->error_stream = hal_error.errorStreamId;

    switch (hal_error.errorCode) {
        case v3::ErrorCode::ERROR_DEVICE:
            leg_error->error_code = CAMERA3_MSG_ERROR_DEVICE;
            break;
        case v3::ErrorCode::ERROR_REQUEST:
            leg_error->error_code = CAMERA3_MSG_ERROR_REQUEST;
            break;
        case v3::ErrorCode::ERROR_RESULT:
            leg_error->error_code = CAMERA3_MSG_ERROR_RESULT;
            break;
        case v3::ErrorCode::ERROR_BUFFER:
            leg_error->error_code = CAMERA3_MSG_ERROR_BUFFER;
            break;
        default:
            MY_LOGE("Unknown error code: %u", hal_error.errorCode);
            return BAD_VALUE;
    }

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
convertToLegacyShutterMessage(
const v3::ShutterMsg& hal_shutter,
camera3_shutter_msg* leg_shutter)
-> ::android::status_t {

    if (leg_shutter == nullptr) {
        MY_LOGE("leg_shutter is nullptr.");
        return BAD_VALUE;
    }

    leg_shutter->frame_number = hal_shutter.frameNumber;
    leg_shutter->timestamp = hal_shutter.timestamp;
    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
getMaxJpegResolution() const -> MSize
{
    int32_t maxJpegWidth = 0, maxJpegHeight = 0;

    auto pStaticMeta = NSMetadataProviderManager::valueForByDeviceId(mDeviceId);
    if ( CC_UNLIKELY(!pStaticMeta) ) {
        MY_LOGE("get deviceInfo id(%d) fail: StaticMetadata(%p)", mDeviceId,  pStaticMeta);
        return -EINVAL;
    }

    IMetadata::IEntry const& entryScaler = pStaticMeta->getMtkStaticCharacteristics().entryFor(MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    if ( entryScaler.isEmpty() )
    {
        MY_LOGW("no static MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS");
        return MSize(0, 0);
    }

    // Get max jpeg size (area-wise).
    for (MUINT i = 0; i < entryScaler.count(); i += 4 )
    {
        MINT32 format       = entryScaler.itemAt(i, Type2Type< MINT32 >());
        MINT32 scalerWidth  = entryScaler.itemAt(i + 1, Type2Type< MINT32 >());
        MINT32 scalerHeight = entryScaler.itemAt(i + 2, Type2Type< MINT32 >());
        MINT32 isInput      = entryScaler.itemAt(i + 3, Type2Type< MINT32 >());
        if (isInput == MTK_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT
          && format == HAL_PIXEL_FORMAT_BLOB && (scalerWidth * scalerHeight > maxJpegWidth * maxJpegHeight)) {
            maxJpegWidth  = scalerWidth;
            maxJpegHeight = scalerHeight;
        }
    }
    return MSize(maxJpegWidth, maxJpegHeight);
}

/******************************************************************************
 *
 ******************************************************************************/
auto
LegacyCameraDevice3::
getJpegBufferSize(
  uint32_t width,
  uint32_t height
) const -> ssize_t
{
    struct Camera3JpegBlob {
         uint16_t jpeg_blob_id;
         uint32_t jpeg_size;
    };

    ssize_t kMinJpegBufferSize = 256 * 1024 + sizeof(Camera3JpegBlob);
    // Get max jpeg size (area-wise).
    MSize maxJpegResolution = getMaxJpegResolution();
    if (maxJpegResolution.w == 0) {
        MY_LOGW("Can't find valid available jpeg sizes in static metadata!");
        return 0;
    }

    auto pStaticMeta = NSMetadataProviderManager::valueForByDeviceId(mDeviceId);
    if ( CC_UNLIKELY(!pStaticMeta) ) {
        MY_LOGE("get deviceInfo id(%d) fail: StaticMetadata(%p)", mDeviceId,  pStaticMeta);
        return -EINVAL;
    }

    // Get max jpeg buffer size
    ssize_t maxJpegBufferSize = 0;
    IMetadata::IEntry const& entry = pStaticMeta->getMtkStaticCharacteristics().entryFor(MTK_JPEG_MAX_SIZE);
    if  ( entry.isEmpty() ) {
        MY_LOGW("no static JPEG_MAX_SIZE");
        return 0;
    }
    maxJpegBufferSize = entry.itemAt(0, Type2Type<MINT32>());
    assert(kMinJpegBufferSize < maxJpegBufferSize);

    // Calculate final jpeg buffer size for the given resolution.
    float scaleFactor = ((float) (width * height)) / (maxJpegResolution.w * maxJpegResolution.h);
    ssize_t jpegBufferSize = scaleFactor * (maxJpegBufferSize - kMinJpegBufferSize) +
            kMinJpegBufferSize;
    if (jpegBufferSize > maxJpegBufferSize) {
        jpegBufferSize = maxJpegBufferSize;
    }

    return jpegBufferSize;
}
