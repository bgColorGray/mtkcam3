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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_APPIMAGESTREAMBUFFERPROVIDER_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_APPIMAGESTREAMBUFFERPROVIDER_H_

#include "IHalBufHandler.h"

#include <mtkcam3/pipeline/stream/IStreamBufferProvider.h>

#include <utils/RefBase.h>

#include <map>
#include <memory>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {

/**
 * AppImageStreamBufferProvider
 *
 * Implementation of IImageStreamBufferProvider, which is used by pipeline module to
 * request image streambuffers.
 *
 * With supporting HIDL_DEVICE_3_5 of android.info.supportedBufferManagementVersion,
 * Camera HAL will recieve StreamBuffer "without" bufferId(buffer handle is not allocated yet) in
 * ICameraDeviceSession::processCaptureRequests(), which indicates Camera HAL must
 * request StreamBuffer by ICameraDeviceCallback::requestStreamBuffers() when Camera HAL needs
 * these StreamBuffers. It happens that pipeline module requests these buffer handles for ISP
 * outputs by IImageStreamBufferProvider::requestStreamBuffer().
 *
 * Without supporting HIDL_DEVICE_3_5 of android.info.supportedBufferManagementVersion,
 * Camera HAL will recieve StreamBuffer "with" bufferId(buffer handle is allocated in Framework) in
 * ICameraDeviceSession::processCaptureRequests(), which indicates Camera HAL already own these
 * buffer handles for ISP outputs. In this situation, DeviceSession has two ways to provide pipeline
 * module theses buffer handles for ISP output:
 * (1) directly pass these buffer handles to pipeline module; or,
 * (2) store these buffer handles by storeStreamBuffers(), and then pipeline module will requests
 *     these buffer handles by IImageStreamBufferProvider::requestStreamBuffer(). It's similar to
 *     support HIDL_DEVICE_3_5 flow.
 */

class AppImageStreamBufferProvider
    : public IImageStreamBufferProvider
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
    static auto     create(
                        android::sp<IHalBufHandler> pHalBufHandler
                    ) -> std::shared_ptr<AppImageStreamBufferProvider>;

    virtual auto    destroy() -> void                                           = 0;

    /**
     * storeStreamBuffers:
     *
     * Store StreamBuffer(with allocated buffer handle) in this provider.
     *
     * Return values:
     * @return status Status code for the operation, one of:
     *     0:
     *         On a successful operation.
     *     -1:
     *         An unexpected internal error occurred, and no store operation happens.
     *
     */
    virtual auto    storeStreamBuffers(
                        const uint32_t requestNumber,
                        const android::KeyedVector<StreamId_T,android::sp<AppImageStreamBuffer>>& vInputImageBuffers,
                        const android::KeyedVector<StreamId_T,android::sp<AppImageStreamBuffer>>& vOutputImageBuffers
                        // const std::map<int32_t, android::sp<IImageStreamBuffer>>& buffers
                    ) -> int                                                    = 0;

    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int              = 0;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IImageStreamBufferProvider Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
    /**
     * requestStreamBuffer
     *
     * Synchronous call for the pipeline (as the producer) to ask for an (empty)
     * output stream buffer from the provider.
     *
     * This call doesn't return until an empty buffer is available or it timed out.
     *
     * @param rpStreamBuffer: It's assigned to an empty stream buffer by the provider on the call.
     *
     * @param in: The input argument.
     *
     * @return An error code.
     *      0: success
     *      -ETIMEDOUT: This call timed out.
     */
    virtual auto    requestStreamBuffer(
                        requestStreamBuffer_cb cb,
                        RequestStreamBuffer const& in
                    ) -> int                                                    = 0;
};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_UTILS_INCLUDE_APPIMAGESTREAMBUFFERPROVIDER_H_