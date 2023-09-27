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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPSTREAMMANAGER_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPSTREAMMANAGER_H_
//
#include "IAppCommonStruct.h"
#include <mtkcam3/main/hal/device/3.x/device/types.h>

#include <vector>

using namespace NSCam::v3::Utils;
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {


/**
 * An interface of App stream manager.
 */
class IAppStreamManager
    : public virtual android::RefBase
{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.

    static auto     create(
                        const CreationInfo& creationInfo
                    ) -> IAppStreamManager*;


    /**
     * Destroy the instance.
     */
    virtual auto    destroy() -> void                                       = 0;

    /**
     * Dump debugging state.
     */
    virtual auto    dumpState(
                        android::Printer& printer,
                        const std::vector<std::string>& options
                        ) -> void                                           = 0;

    // copy config image/meta map into AppStreamMgr.FrameHandler
    virtual auto    setConfigMap(
                        ImageConfigMap& imageConfigMap,
                        MetaConfigMap& metaConfigMap
                        )  -> void                                           = 0;
    /**
     * Configure streams.
     *
     * @param[in] requestedConfiguration: the stream configuration requested by
     *  the framework.
     *
     * @param[out] halConfiguration: the HAL's response to each requested stream
     *  configuration.
     *
     * @param[out] rStreams: contains all configured streams.
     *
     * @return
     *      0 indicates success; otherwise failure.
     */
    virtual auto    configureStreams(
                        const StreamConfiguration& requestedConfiguration,
                        /*const V3_4::HalStreamConfiguration& halConfiguration    __unused,*/
                        const ConfigAppStreams& rStreams
                        ) -> int                                            = 0;

    /**
     * Retrieves the fast message queue used to pass request metadata.
     *
     * If client decides to use fast message queue to pass request metadata,
     * it must:
     * - Call getCaptureRequestMetadataQueue to retrieve the fast message queue;
     * - In each of the requests sent in processCaptureRequest, set
     *   fmqSettingsSize field of CaptureRequest to be the size to read from the
     *   fast message queue; leave settings field of CaptureRequest empty.
     *
     * @return queue the queue that client writes request metadata to.
     */
    // virtual auto    getCaptureRequestMetadataQueue() -> const ::android::hardware::MQDescriptorSync<uint8_t>&
    //                                                                         = 0;

    /**
     * Retrieves the fast message queue used to read result metadata.

     * Clients to ICameraDeviceSession must:
     * - Call getCaptureRequestMetadataQueue to retrieve the fast message queue;
     * - In implementation of ICameraDeviceCallback, test whether
     *   .fmqResultSize field is zero.
     *     - If .fmqResultSize != 0, read result metadata from the fast message
     *       queue;
     *     - otherwise, read result metadata in CaptureResult.result.
     *
     * @return queue the queue that implementation writes result metadata to.
     */
    // virtual auto    getCaptureResultMetadataQueue() -> const ::android::hardware::MQDescriptorSync<uint8_t>&
    //                                                                         = 0;

    /**
     * Flush requests.
     *
     * @param[in] requests: the requests to flush.
     *
     */
    virtual auto    flushRequest(
                        const std::vector<CaptureRequest>& requests
                        ) -> void                                           = 0;

    /**
     * Remove the
     *
     * @param[in] cachesToRemove: The cachesToRemove argument contains a list of
     *  buffer caches to be removed
     */
    // virtual auto    removeBufferCache(
    //                     const hidl_vec<BufferCache>& cachesToRemove
    //                     ) -> void                                           = 0;

    /**
     * Submit a set of requests.
     * This call is valid only after streams are configured successfully.
     *
     * @param[in] rRequests: a set of requests, created by CameraDeviceSession,
     *  associated with the given CaptureRequest.
     *
     * @return
     *      0 indicates success; otherwise failure.
     */
    virtual auto    submitRequests(
                        /*const hidl_vec<V3_4::CaptureRequest>& requests,*/
                        const android::Vector<Request>& rRequests
                        ) -> int                                            = 0;


    virtual auto    updateStreamBuffer(
                        uint32_t frameNo,
                        StreamId_T streamId,
                        android::sp<IImageStreamBuffer> const pBuffer
                        )   -> int                                          = 0;

    /**
     * Wait until all the registered requests have finished returning.
     *
     * @param[in] timeout
     */
    virtual auto    waitUntilDrained(nsecs_t const timeout) -> int          = 0;
    // virtual auto    getHalBufManagerStreamProvider(::android::sp<IAppStreamManager> pAppStreamManager) ->
    //                     std::shared_ptr<IImageStreamBufferProvider> = 0;
    // virtual auto    requestStreamBuffer( android::sp<IImageStreamBuffer>& rpStreamBuffer,
    //                     IImageStreamBufferProvider::RequestStreamBuffer const& in) -> int = 0;
    virtual auto    markStreamError(uint32_t requestNo, StreamId_T streamId) -> void = 0;
    // virtual auto    waitHalBufCntAvailable(const hidl_vec<V3_4::CaptureRequest>& requests) -> void = 0;
    // virtual auto    waitHalBufCntAvailable(const std::vector<CaptureRequest>& requests) -> void = 0;

    // struct  PhysicalResult
    //     : public android::RefBase
    // {
    //     /**
    //      * @param[in] physic camera id
    //      */
    //      std::string physicalCameraId;

    //     /**
    //      * @param[in] physic result partial metadata to update.
    //      */
    //      android::sp<IMetaStreamBuffer> physicalResultMeta;
    // };

    /**
     * abort reuqests failed to submit to pipeline
     *
     * @param[in] requests: a set of given requests in terms of a form of
     *  IAppStreamManager::Request
     *
     */
    virtual auto    abortRequest(
                        const android::Vector<Request>& requests
                        ) -> void                                           = 0;

    /**
     * extract specific metadata and early callback to framework.
     *
     * @param[in] request: a set of given request in terms of a form of
     *  IAppStreamManager::Request
     *
     */
    virtual auto    earlyCallbackMeta(
                        const Request& request
                        ) -> void                                           = 0;


    virtual auto flush() -> void                                            = 0;

    /**
     * Update a given result frame.
     *
     */
    // struct UpdateResultParams
    // {
    //     /**
    //      * @param[in] the frame number to update.
    //      */
    //     uint32_t                                    requestNo = 0;

    //     /**
    //      * @param[in] user id.
    //      *  In fact, it is pipeline node id from the viewpoint of pipeline implementation,
    //      *  but the pipeline users have no such knowledge.
    //      */
    //     intptr_t                                    userId = 0;

    //     /**
    //      * @param[in] hasLastPartial: contain last partial metadata in result partial metadata vector.
    //      */
    //     bool                                        hasLastPartial = false;

    //     /**
    //      * @param[in] timestampStartOfFrame: referenced by De-jitter mechanism.
    //      */
    //     int64_t                                     timestampStartOfFrame = 0;

    //     /**
    //      * @param[in] realTimePartial: contain real-time partial metadata in result partial metadata vector.
    //      *  If true, then the result would be callbacked instantly.
    //      *  Or the result callback would be pending util the next real-time result arrives
    //      *  (or the last result arrives)
    //      */
    //     bool                                       realTimePartial = false;

    //     /**
    //      * @param[in] result partial metadata to update.
    //      */
    //     android::Vector<android::sp<IMetaStreamBuffer>> resultMeta;

    //     /**
    //      * @param[in] phsycial result to update.
    //      */
    //     android::Vector<android::sp<PhysicalResult>> physicalResult;


    struct UpdateAvailableParams
        : public android::LightRefBase<UpdateAvailableParams>
    {
        /**
         * @param[in] the frame number to update.
         */
        uint32_t                    frameNo = 0;
        /**
         * @param[in] the callerName.
         */
        std::string                 callerName;
        /**
         * @param[in] available metadata for early callback
         */
        IMetadata                   resultMetadata;
        /**
         * @param[in] user id.
         *  In fact, it is pipeline node id from the viewpoint of pipeline implementation,
         *  but the pipeline users have no such knowledge.
         */
        intptr_t                    userId;
    };
    virtual auto    updateResult(UpdateResultParams const& params) -> void  = 0;
    virtual auto    updateAvailableMetaResult(const android::sp<UpdateAvailableParams>& param) -> void = 0;


    virtual auto    getAppStreamInfoBuilderFactory() const -> std::shared_ptr<IStreamInfoBuilderFactory>  = 0;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPSTREAMMANAGER_H_

