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

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_LEGACY_LEGACYCAMERADEVICE3_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_LEGACY_LEGACYCAMERADEVICE3_H_

#include "ILegacyHalDevice.h"
#include "../../../device/3.x/include/ICameraDevice3Session.h"
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IVendorTagDescriptor.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <hardware/camera3.h>
#include <memory>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace legacy_dev3 {


/******************************************************************************
 *
 ******************************************************************************/
class LegacyCameraDevice3
    : public ILegacyHalDevice
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Instantiation.
    virtual             ~LegacyCameraDevice3();
                        LegacyCameraDevice3(int32_t deviceId, android::sp<ICameraDevice3Session> session);

public:    ////        Operations.
    virtual auto        open(
                            camera_module_callbacks_t const* callbacks
                        ) -> int;

private:
    virtual auto        createSessionCallbacks() -> std::shared_ptr<v3::CameraDevice3SessionCallback>;

private:
    virtual auto        processCaptureResult(std::vector<v3::CaptureResult>& results) -> void;

    virtual auto        notify(const std::vector<v3::NotifyMsg>& msgs) -> void;

    virtual auto        requestStreamBuffers(
                            const std::vector<v3::BufferRequest>& vBufferRequests,
                            std::vector<v3::StreamBufferRet>* pvBufferReturns
                        ) -> v3::BufferRequestStatus;

    virtual auto        returnStreamBuffers(const std::vector<v3::StreamBuffer>& buffers) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Attributes.
    //
    static  inline LegacyCameraDevice3*  getDevice(camera3_device const* device)
                                    {
                                        return (NULL == device)
                                                ? NULL
                                                : (LegacyCameraDevice3*)((device)->priv)
                                                ;
                                    }

    static  inline LegacyCameraDevice3*  getDevice(hw_device_t* device)
                                    {
                                        return  getDevice((camera3_device const*)device);
                                    }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Initialization.

    virtual auto    i_closeDevice() -> int;

    /**
     * One-time initialization to pass framework callback function pointers to
     * the HAL. Will be called once after a successful open() call, before any
     * other functions are called on the camera3_device_ops structure.
     *
     * Return values:
     *
     *  0:     On successful initialization
     *
     * -ENODEV: If initialization fails. Only close() can be called successfully
     *          by the framework after this.
     */
    virtual auto    i_initialize(
                        camera3_callback_ops const* callback_ops
                    ) -> int;
    /**
     * Uninitialize the device resources owned by this object. Note that this
     * is *not* done in the destructor.
     *
     * This may be called at any time, although the call may block until all
     * in-flight captures have completed (all results returned, all buffers
     * filled). After the call returns, no more callbacks are allowed.
     */
    virtual auto    i_uninitialize() -> int;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
public:     ////                    Stream management

    virtual auto    i_configure_streams(
                        camera3_stream_configuration_t* stream_list
                    ) -> int;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
public:     ////                    Request creation and submission

    virtual auto    i_construct_default_request_settings(
                        int type
                    ) -> camera_metadata const*;

    virtual auto    i_process_capture_request(
                        camera3_capture_request_t* request
                    ) -> int;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
public:     ////                    Miscellaneous methods

    virtual auto    i_flush() -> int;

    virtual auto    i_dump(int fd) -> void;

    virtual auto    i_signal_stream_flush(
                        uint32_t num_streams,
                        const camera3_stream_t* const* streams
                    ) -> void;

    virtual auto    i_is_reconfiguration_required(
                        const camera_metadata_t* old_session_params,
                        const camera_metadata_t* new_session_params
                    ) -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICamDevice Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual hw_device_t const*      get_hw_device() const { return &mDevice.common; }

    virtual void                    set_hw_module(hw_module_t const* module)
                                    {
                                        mDevice.common.module = const_cast<hw_module_t*>(module);
                                    }

    // virtual void                    set_module_callbacks(camera_module_callbacks_t const* callbacks)
    //                                 {
    //                                      mpModuleCallbacks = callbacks;
    //                                 }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICamDevice Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//public:

//    virtual auto                    getResourceCost(v3::CameraResourceCost resCost) -> int;

//    virtual auto                    setTorchMode(bool enabled) -> int;

//    virtual auto                    getCameraCharacteristics() -> const camera_metadata*;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    struct camera3Stream : public camera3_stream {
        int mId;
    };

protected:  ////
    camera_module_callbacks_t const*            mpModuleCallbacks;
    camera3_device                              mDevice;
    int32_t                                     mDeviceId;
    camera3_device_ops                          mDeviceOps;         //  which is pointed to by mDevice.ops.

    camera3_callback_ops_t const*               mpDeviceCallbacks;
    std::map<int, camera3_stream_t*>            mStreamMap;
    int32_t                                     mConvertTimeDebug;

private:
    // std::unique_ptr<v3::CameraDevice3Session>  mSession = nullptr;
    android::sp<ICameraDevice3Session>          mSession = nullptr;

private:    ////        interface/structure conversion helper

    virtual auto        convertToHalStreamConfiguration(
                            const camera3_stream_configuration_t *srcStreams,
                            NSCam::v3::StreamConfiguration *dstStreams)
                        -> ::android::status_t;

    virtual auto        convertToLegacyStreamConfiguration(
                            const NSCam::v3::HalStreamConfiguration& srcStreams,
                            camera3_stream_configuration_t* dstStreams)
                        -> ::android::status_t;

    virtual auto        convertToHalCaptureRequest(
                            camera3_capture_request_t* request,
                            NSCam::v3::CaptureRequest* hal_request)
                        -> ::android::status_t;

    virtual auto        convertToLegacyErrorMessage(
                            const v3::ErrorMsg& hal_error,
                            camera3_error_msg_t* leg_error)
                        -> ::android::status_t;

    virtual auto        convertToLegacyShutterMessage(
                            const v3::ShutterMsg& hal_shutter,
                            camera3_shutter_msg* leg_shutter)
                        -> ::android::status_t;

    virtual auto        getMaxJpegResolution() const -> MSize;

    virtual auto        getJpegBufferSize(uint32_t width, uint32_t height) const -> ssize_t;

};
/******************************************************************************
 *
 ******************************************************************************/
};  //namespace legacy_dev3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_LEGACY_LEGACYCAMERADEVICE3_H_

